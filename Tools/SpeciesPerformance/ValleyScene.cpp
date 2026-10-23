#include "pch.h"

#include "Battles.h"
#include "CommandJournal.h"
#include "Construction.h"
#include "MarkerTimings.h"
#include "PlatformThread.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "Renown.h"
#include "Statistics.h"
#include "Upkeep.h"
#include "ValleyScene.h"
#include "WorkerPool.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <format>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <utility>

namespace Performance
{

namespace
{

// The valley runs' simulation key, as every other scene's world uses one.
constexpr Neuron::SipHashKey VALLEY_KEY{0x76616c6c65790001, 0x0123456789abcdef};
// Playtest 1's lines (the MVP scope's table 5): battles of 4 to 12 minutes, conflicts settled within 7 hours of play, and
// the growth steps of the first 5 hours.
constexpr double BATTLE_SHORTEST_MINUTES = 4.0;
constexpr double BATTLE_LONGEST_MINUTES = 12.0;
constexpr double CONFLICT_HOURS = 7.0;
constexpr double EARLY_HOURS = 5.0;
constexpr double SECONDS_PER_MINUTE = 60.0;
constexpr double SECONDS_PER_HOUR = 3600.0;

// The figures counted on a battle's field every this many ticks.
constexpr std::uint64_t FIELD_COUNT_TICKS = 10;

// The living figures of _team within the battlefield of _battle (MVP-043).
std::uint32_t OnField(const Species::RegionSimulation& _region, const Species::Battle& _battle, std::uint8_t _team)
{
  const Species::SlicedPool<Species::Figure>& figures = _region.TeamAt(_team).figures;
  std::uint32_t count = 0;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    count += figures.Used(slot) && !figures.At(slot).dead &&
                 Neuron::HorizontalDistance(figures.At(slot).position, _battle.fieldCenter) <= _battle.fieldRadius
               ? 1U
               : 0U;
  }
  return count;
}

} // namespace

ValleyRun RunValley(std::uint64_t _seed, const Species::TuningMvp& _tuning, const Species::StructureRecords& _records, std::uint64_t _ticks,
                    Species::Temperament _policy)
{
  ValleyRun run;
  run.seed = _seed;
  run.ticks = _ticks;
  run.stewardPolicy = _policy;
  auto generated = Species::GenerateRegionTerrain(_seed, _tuning);
  if (!generated)
  {
    return run;
  }
  const auto terrain = std::make_shared<const Species::RegionTerrain>(std::move(*generated));
  run.stewardSite = static_cast<std::uint8_t>(std::clamp<std::int64_t>(_tuning.mvp.sandbox.stewardSite, 0, 3));
  Species::WorldStart start;
  start.key = VALLEY_KEY;
  start.seed = terrain->seed;
  start.stewardSite = run.stewardSite;
  start.aiSettlements = 3;
  std::unique_ptr<Species::WorldSimulation> world = Species::StartWorld(start, terrain, _tuning, &_records);
  Species::RegionSimulation& region = world->Region(0);
  // ADR-164: the run's stated conflict policy for the steward's settlement, the decisions of a temperament's, which its
  // scripted steward runs as an AI settlement's.
  std::ranges::find(region.Settlements(), run.stewardSite, &Species::Settlement::site)->cladan.temperament = _policy;
  static_cast<void>(region.AddScriptedSteward(run.stewardSite));

  // Each battle's figures, the most of both sides on its field at once, by its expedition.
  std::map<Species::StableId, std::uint32_t> figures;
  Neuron::WorkerPool serial(0);
  std::int64_t stepNanoseconds = 0;
  std::int64_t largest = 0;
  std::uint64_t stepped = 0;
  // The run's ticks, then as many more as its battles in progress need to finish (MVP-060).
  const auto maxBattleTicks =
    static_cast<std::uint64_t>(std::llround(_tuning.pil.battle.maxMinutes * SECONDS_PER_MINUTE / _tuning.ref.sim.tickPeriod));
  for (std::uint64_t tick = 0; tick < _ticks || (tick < _ticks + maxBattleTicks && !region.Battles().empty()); ++tick)
  {
    const std::int64_t begin = Neuron::MarkerClockNanoseconds();
    world->Step(tick, serial);
    const std::int64_t spent = Neuron::MarkerClockNanoseconds() - begin;
    stepNanoseconds += spent;
    largest = std::max(largest, spent);
    ++stepped;
    for (const Species::Battle& battle : region.Battles())
    {
      if (tick % FIELD_COUNT_TICKS == 0 || !figures.contains(battle.expedition))
      {
        const std::uint32_t both =
          OnField(region, battle, Species::ExpeditionTeamOf(battle.attacker)) + OnField(region, battle, battle.defender);
        figures[battle.expedition] = std::max(figures[battle.expedition], both);
      }
    }
    if (!run.firstContactTick && !region.ContactEvents().empty())
    {
      run.firstContactTick = tick;
    }
  }
  run.runEnded = world->RunState().ended;
  run.meanStepMicroseconds = stepped == 0 ? 0.0 : static_cast<double>(stepNanoseconds) / static_cast<double>(stepped) / 1000.0;
  run.largestStepMicroseconds = static_cast<double>(largest) / 1000.0;

  for (const Species::BattleResult& result : region.BattleResults())
  {
    ValleyBattle battle;
    battle.startTick = result.startTick;
    battle.endTick = result.endTick;
    battle.attacker = result.attacker;
    battle.defender = result.defender;
    battle.claim = result.claim != 0;
    battle.figures = figures.contains(result.expedition) ? figures[result.expedition] : 0;
    battle.end = result.end;
    battle.attackerWon = result.attackerWon;
    run.battles.push_back(battle);
  }
  // The claims and their ends, matched in order by claimant and area, from the server's statistics (MVP-064).
  for (const Species::StatisticEntry& entry : region.Statistics())
  {
    if (entry.kind == Species::StatisticKind::ClaimMade)
    {
      run.claims.push_back({entry.site, static_cast<std::uint8_t>(entry.detail), entry.tick, std::nullopt});
    }
    else if (entry.kind == Species::StatisticKind::ClaimWithdrawn || entry.kind == Species::StatisticKind::ClaimLost ||
             entry.kind == Species::StatisticKind::ClaimWon)
    {
      const auto open = std::ranges::find_if(run.claims, [&](const ValleyClaim& _claim)
                                             { return !_claim.endTick && _claim.claimant == entry.site && _claim.area == entry.detail; });
      if (open != run.claims.end())
      {
        open->endTick = entry.tick;
      }
    }
    else if (entry.site == run.stewardSite && Species::IsGrowthStep(entry.kind))
    {
      run.stewardGrowthSteps.push_back(entry.tick);
    }
  }
  for (const Species::Settlement& settlement : region.Settlements())
  {
    if (settlement.site < run.strengthMilliRu.size())
    {
      run.strengthMilliRu[settlement.site] = Species::StrengthMilliRu(region, settlement);
      run.workers[settlement.site] = Species::Population(region, settlement);
      const Species::SettlementRenown* renown = Species::RenownAt(region, settlement.site);
      run.renown[settlement.site] = renown != nullptr ? Species::RenownOf(_tuning, *renown).Total() : 0;
    }
  }
  return run;
}

std::vector<ValleyRun> RunValleys(std::uint64_t _firstSeed, std::uint64_t _count, const Species::TuningMvp& _tuning,
                                  const Species::StructureRecords& _records, std::uint64_t _ticks, Species::Temperament _policy)
{
  std::vector<ValleyRun> runs(_count);
  std::atomic<std::uint64_t> next = 0;
  const std::uint64_t threadCount = std::clamp<std::uint64_t>(std::thread::hardware_concurrency(), 1, std::max<std::uint64_t>(_count, 1));
  std::vector<Neuron::PlatformThread> threads;
  threads.reserve(threadCount);
  for (std::uint64_t index = 0; index < threadCount; ++index)
  {
    threads.push_back(Neuron::PlatformThread::Start(std::format("perf.valley.{}", index),
                                                    [&](const std::stop_token&)
                                                    {
                                                      for (std::uint64_t run = next++; run < _count; run = next++)
                                                      {
                                                        runs[run] = RunValley(_firstSeed + run, _tuning, _records, _ticks, _policy);
                                                      }
                                                    }));
  }
  for (Neuron::PlatformThread& thread : threads)
  {
    thread.Join();
  }
  return runs;
}

ValleyLines JudgeValley(const ValleyRun& _run, const Species::TuningMvp& _tuning)
{
  ValleyLines lines;
  const double tickSeconds = _tuning.ref.sim.tickPeriod;
  std::vector<std::uint32_t> stewardFigures;
  std::uint32_t within = 0;
  for (const ValleyBattle& battle : _run.battles)
  {
    const double minutes = static_cast<double>(battle.endTick - battle.startTick) * tickSeconds / SECONDS_PER_MINUTE;
    within += minutes >= BATTLE_SHORTEST_MINUTES && minutes <= BATTLE_LONGEST_MINUTES ? 1U : 0U;
    lines.longestBattleMinutes = std::max(lines.longestBattleMinutes, minutes);
    if (battle.attacker == _run.stewardSite || battle.defender == _run.stewardSite)
    {
      stewardFigures.push_back(battle.figures);
    }
  }
  lines.battles = static_cast<std::uint32_t>(_run.battles.size());
  lines.battlesWithinFourToTwelve = _run.battles.empty() ? 0.0 : static_cast<double>(within) / static_cast<double>(_run.battles.size());
  lines.stewardBattles = static_cast<std::uint32_t>(stewardFigures.size());
  if (!stewardFigures.empty())
  {
    std::ranges::sort(stewardFigures);
    lines.stewardFiguresMedian = stewardFigures[stewardFigures.size() / 2];
    lines.stewardFiguresLeast = stewardFigures.front();
  }
  // PIL-123 at the session pace, where an hour of play is a world day (MVP-070): its 7 days are 7 hours of play. A claim,
  // a conflict between neighbours with an end state, is judged when it ended or was made at least 7 hours before the run's
  // end.
  const double window = CONFLICT_HOURS * SECONDS_PER_HOUR / tickSeconds;
  std::uint32_t settled = 0;
  for (const ValleyClaim& claim : _run.claims)
  {
    const bool endedInTime = claim.endTick && static_cast<double>(*claim.endTick - claim.madeTick) <= window;
    if (endedInTime || static_cast<double>(claim.madeTick) + window <= static_cast<double>(_run.ticks))
    {
      ++lines.claimsJudged;
      settled += endedInTime ? 1U : 0U;
    }
  }
  lines.claimsSettled = lines.claimsJudged == 0 ? 0.0 : static_cast<double>(settled) / static_cast<double>(lines.claimsJudged);
  std::array<std::int64_t, 4> strengths = _run.strengthMilliRu;
  std::ranges::sort(strengths);
  const double median = static_cast<double>(strengths[1] + strengths[2]) / 2.0;
  lines.strongestToMedian = median > 0.0 ? static_cast<double>(strengths[3]) / median : 0.0;
  // The largest gap between the steward's growth steps, from the start, within the first 5 hours.
  const double early = EARLY_HOURS * SECONDS_PER_HOUR / tickSeconds;
  std::uint64_t previous = 0;
  for (const std::uint64_t step : _run.stewardGrowthSteps)
  {
    if (static_cast<double>(step) > early)
    {
      break;
    }
    lines.largestStepGapMinutes =
      std::max(lines.largestStepGapMinutes, static_cast<double>(step - previous) * tickSeconds / SECONDS_PER_MINUTE);
    previous = step;
  }
  return lines;
}

} // namespace Performance
