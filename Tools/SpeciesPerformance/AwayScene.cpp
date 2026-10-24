#include "pch.h"

#include "AwayScene.h"
#include "Battles.h"
#include "CommandJournal.h"
#include "Construction.h"
#include "MarkerTimings.h"
#include "PlatformThread.h"
#include "Presence.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "Upkeep.h"
#include "WorkerPool.h"
#include "WorldPace.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <format>
#include <memory>
#include <thread>
#include <utility>

namespace Performance
{

namespace
{

// The away runs' simulation key, as every other scene's world uses one.
constexpr Neuron::SipHashKey AWAY_KEY{0x6177617972756e01, 0x0123456789abcdef};
constexpr double SECONDS_PER_MINUTE = 60.0;
constexpr double SECONDS_PER_HOUR = 3600.0;
constexpr double HOURS_PER_DAY = 24.0;
// The strengths sampled every world hour, for each absence's regain.
constexpr double SAMPLE_HOURS = 1.0;

/// One visit of a pattern: its start, in hours into each world day from the run's start, and its length in minutes.
struct Visit
{
  double startHours = 0.0;
  double minutes = 0.0;
};

// PIL-102: the typical pattern's `pil.session.typical_active_minutes` (60) of active play, at the day's start, and its
// `pil.session.typical_check_ins` (2) check-ins, in the morning and at midday of a day whose play is in the evening; the
// check-in pattern's one check-in. A check-in lasts 6 minutes, the middle of `pil.session.check_in_minutes`' 2 to 10
// (PIL-074).
constexpr double CHECK_IN_MINUTES = 6.0;
constexpr std::array<Visit, 3> TYPICAL_VISITS{{{0.0, 60.0}, {12.0, CHECK_IN_MINUTES}, {17.5, CHECK_IN_MINUTES}}};
constexpr std::array<Visit, 1> CHECK_IN_VISITS{{{0.0, CHECK_IN_MINUTES}}};

// Whether the steward is present at _tick of a world at _pattern.
bool Present(AwayPattern _pattern, std::uint64_t _tick, double _tickSeconds)
{
  const double hours = std::fmod(static_cast<double>(_tick) * _tickSeconds / SECONDS_PER_HOUR, HOURS_PER_DAY);
  const auto within = [hours](const Visit& _visit)
  { return hours >= _visit.startHours && hours < _visit.startHours + _visit.minutes / SECONDS_PER_MINUTE; };
  return _pattern == AwayPattern::Typical ? std::ranges::any_of(TYPICAL_VISITS, within) : std::ranges::any_of(CHECK_IN_VISITS, within);
}

const Species::Settlement* SettlementAt(const Species::RegionSimulation& _region, std::uint8_t _site)
{
  const auto found = std::ranges::find(_region.Settlements(), _site, &Species::Settlement::site);
  return found == _region.Settlements().end() ? nullptr : &*found;
}

std::int64_t StrengthAt(const Species::RegionSimulation& _region, std::uint8_t _site)
{
  const Species::Settlement* settlement = SettlementAt(_region, _site);
  return settlement == nullptr ? 0 : Species::StrengthMilliRu(_region, *settlement);
}

// The AI settlements' mean strength.
std::int64_t AiStrength(const Species::RegionSimulation& _region, std::uint8_t _stewardSite)
{
  std::int64_t sum = 0;
  std::int64_t count = 0;
  for (const Species::Settlement& settlement : _region.Settlements())
  {
    if (settlement.site != _stewardSite)
    {
      sum += Species::StrengthMilliRu(_region, settlement);
      ++count;
    }
  }
  return count == 0 ? 0 : sum / count;
}

} // namespace

AwayRun RunAway(std::uint64_t _seed, AwayPattern _pattern, const Species::TuningMvp& _tuning, const Species::StructureRecords& _records,
                std::uint64_t _ticks)
{
  AwayRun run;
  run.seed = _seed;
  run.pattern = _pattern;
  auto generated = Species::GenerateRegionTerrain(_seed, _tuning);
  if (!generated)
  {
    return run;
  }
  const auto terrain = std::make_shared<const Species::RegionTerrain>(std::move(*generated));
  run.stewardSite = static_cast<std::uint8_t>(std::clamp<std::int64_t>(_tuning.mvp.sandbox.stewardSite, 0, 3));
  Species::WorldStart start;
  start.key = AWAY_KEY;
  start.seed = terrain->seed;
  start.stewardSite = run.stewardSite;
  start.aiSettlements = 3;
  start.pace = Species::WorldPace::AlwaysOn; // PIL-104 (ADR-184): the world's values are the always-on pace's
  std::unique_ptr<Species::WorldSimulation> world = Species::StartWorld(start, terrain, _tuning, &_records);
  Species::RegionSimulation& region = world->Region(0);
  const double tickSeconds = world->Tuning().ref.sim.tickPeriod;
  run.ticks = _ticks != 0 ? _ticks : Species::RunTicks(world->Tuning());
  // ADR-164: while present, the steward's settlement makes a Territorial settlement's decisions through its scripted
  // steward, as the valley run's does.
  const std::uint8_t team = run.stewardSite;
  std::ranges::find(region.Settlements(), run.stewardSite, &Species::Settlement::site)->cladan.temperament =
    Species::Temperament::Territorial;
  bool present = true;
  static_cast<void>(region.AddScriptedSteward(team));

  const auto dayTicks = static_cast<std::uint64_t>(std::llround(HOURS_PER_DAY * SECONDS_PER_HOUR / tickSeconds));
  const auto sampleTicks = static_cast<std::uint64_t>(std::llround(SAMPLE_HOURS * SECONDS_PER_HOUR / tickSeconds));
  std::vector<std::size_t> regaining; // the absences whose strength has not come back yet
  Neuron::WorkerPool serial(0);
  std::int64_t stepNanoseconds = 0;
  for (std::uint64_t tick = 0; tick < run.ticks; ++tick)
  {
    if (tick % dayTicks == 0)
    {
      run.stewardStrengthMilliRu.push_back(StrengthAt(region, run.stewardSite));
      run.aiStrengthMilliRu.push_back(AiStrength(region, run.stewardSite));
      const Species::Settlement* own = SettlementAt(region, run.stewardSite);
      run.stewardWorkers.push_back(own != nullptr ? Species::Population(region, *own) : 0);
    }
    // MVP-072 (ADR-185): presence changes between ticks, as the world server applies them.
    if (const bool now = Present(_pattern, tick, tickSeconds); now != present)
    {
      present = now;
      if (!present)
      {
        AwayAbsence absence;
        absence.fromTick = tick;
        absence.strengthBeforeMilliRu = StrengthAt(region, run.stewardSite);
        run.absences.push_back(absence);
        region.RemoveScriptedStewards(team);
        static_cast<void>(Species::SetStewardPresence(*world, true));
      }
      else
      {
        AwayAbsence& absence = run.absences.back();
        const Species::Absence* open = Species::OpenAbsence(region, team);
        absence.deputyActions = open != nullptr ? static_cast<std::uint32_t>(open->actions.size()) + open->actionsDropped : 0;
        static_cast<void>(Species::SetStewardPresence(*world, false));
        static_cast<void>(region.AddScriptedSteward(team));
        absence.toTick = tick;
        absence.strengthAfterMilliRu = StrengthAt(region, run.stewardSite);
        regaining.push_back(run.absences.size() - 1);
      }
    }
    if (tick % sampleTicks == 0 && !regaining.empty())
    {
      const std::int64_t strength = StrengthAt(region, run.stewardSite);
      std::erase_if(regaining,
                    [&](std::size_t _index)
                    {
                      AwayAbsence& absence = run.absences[_index];
                      if (strength >= absence.strengthBeforeMilliRu)
                      {
                        absence.regainTick = tick;
                        return true;
                      }
                      return false;
                    });
    }
    const std::int64_t begin = Neuron::MarkerClockNanoseconds();
    world->Step(tick, serial);
    stepNanoseconds += Neuron::MarkerClockNanoseconds() - begin;
  }
  run.stewardStrengthMilliRu.push_back(StrengthAt(region, run.stewardSite));
  run.aiStrengthMilliRu.push_back(AiStrength(region, run.stewardSite));
  const Species::Settlement* own = SettlementAt(region, run.stewardSite);
  run.stewardWorkers.push_back(own != nullptr ? Species::Population(region, *own) : 0);
  if (!run.absences.empty() && run.absences.back().toTick == 0)
  {
    run.absences.pop_back(); // still open at the run's end: no return to measure
  }
  run.runEnded = world->RunState().ended;
  run.meanStepMicroseconds = run.ticks == 0 ? 0.0 : static_cast<double>(stepNanoseconds) / static_cast<double>(run.ticks) / 1000.0;
  // The battles the steward's settlement was a side of, and in each absence those it defended that began in it.
  for (const Species::BattleResult& result : region.BattleResults())
  {
    if (result.attacker != run.stewardSite && result.defender != run.stewardSite)
    {
      continue;
    }
    ++run.stewardBattles;
    for (AwayAbsence& absence : run.absences)
    {
      absence.battles +=
        result.defender == run.stewardSite && result.startTick >= absence.fromTick && result.startTick < absence.toTick ? 1U : 0U;
    }
  }
  return run;
}

std::vector<AwayRun> RunAways(std::uint64_t _firstSeed, std::uint64_t _count, const Species::TuningMvp& _tuning,
                              const Species::StructureRecords& _records, std::uint64_t _ticks)
{
  const std::uint64_t jobs = _count * 2;
  std::vector<AwayRun> runs(jobs);
  std::atomic<std::uint64_t> next = 0;
  const std::uint64_t threadCount = std::clamp<std::uint64_t>(std::thread::hardware_concurrency(), 1, std::max<std::uint64_t>(jobs, 1));
  std::vector<Neuron::PlatformThread> threads;
  threads.reserve(threadCount);
  for (std::uint64_t index = 0; index < threadCount; ++index)
  {
    threads.push_back(Neuron::PlatformThread::Start(
      std::format("perf.away.{}", index),
      [&](const std::stop_token&)
      {
        for (std::uint64_t job = next++; job < jobs; job = next++)
        {
          runs[job] = RunAway(_firstSeed + job / 2, job % 2 == 0 ? AwayPattern::Typical : AwayPattern::CheckIn, _tuning, _records, _ticks);
        }
      }));
  }
  for (Neuron::PlatformThread& thread : threads)
  {
    thread.Join();
  }
  return runs;
}

std::string AwayJson(const std::vector<AwayRun>& _runs, double _tickSeconds)
{
  const auto hours = [_tickSeconds](std::uint64_t _ticks) { return static_cast<double>(_ticks) * _tickSeconds / SECONDS_PER_HOUR; };
  const auto list = [](const auto& _values, const auto& _format)
  {
    std::string text;
    for (const auto& value : _values)
    {
      text += std::format("{}{}", text.empty() ? "" : ", ", _format(value));
    }
    return std::format("[{}]", text);
  };
  const auto ru = [](std::int64_t _milliRu) { return std::format("{:.1f}", static_cast<double>(_milliRu) / 1000.0); };
  std::string json;
  for (const AwayRun& run : _runs)
  {
    const std::string absences =
      list(run.absences,
           [&](const AwayAbsence& _absence)
           {
             return std::format("[{:.3f}, {:.3f}, {}, {}, {}, {}, {}]", hours(_absence.fromTick), hours(_absence.toTick), _absence.battles,
                                _absence.deputyActions, ru(_absence.strengthBeforeMilliRu), ru(_absence.strengthAfterMilliRu),
                                _absence.regainTick ? std::format("{:.3f}", hours(*_absence.regainTick)) : std::string("null"));
           });
    json += std::format("{}\n    {{\"seed\": {}, \"pattern\": \"{}\", \"ticks\": {}, \"hours\": {:.3f}, \"steward_site\": {}, "
                        "\"steward_battles\": {}, \"run_ended\": {}, \"mean_step_us\": {:.1f}, \"steward_strength\": {}, "
                        "\"ai_strength\": {}, \"steward_workers\": {}, \"absences\": {}}}",
                        json.empty() ? "" : ",", run.seed, run.pattern == AwayPattern::Typical ? "typical" : "check_in", run.ticks,
                        hours(run.ticks), run.stewardSite, run.stewardBattles, run.runEnded ? "true" : "false", run.meanStepMicroseconds,
                        list(run.stewardStrengthMilliRu, ru), list(run.aiStrengthMilliRu, ru),
                        list(run.stewardWorkers, [](std::uint32_t _workers) { return std::to_string(_workers); }), absences);
  }
  return std::format("[{}\n  ]", json);
}

} // namespace Performance
