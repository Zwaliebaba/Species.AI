#include "pch.h"

#include "CommandJournal.h"
#include "Construction.h"
#include "GrowthScene.h"
#include "MarkerTimings.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "SettlementCommands.h"
#include "Snapshot.h"
#include "Territory.h"
#include "WorkerPool.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <cmath>
#include <utility>

namespace Performance
{

namespace
{

// The scenes' simulation key, as every other scene's world uses one.
constexpr Neuron::SipHashKey GROWTH_KEY{0x67726f7774680001, 0x0123456789abcdef};

// The exit's figures (the scope's table 2, the M2 plan §5): a growth step at least every pil.settlement.growth_step_minutes
// in the first 5 hours, and a plateau after 4 to 6 hours, an hour with no step within 95 % of the largest Worker count.
constexpr double STEP_MINUTES = 20.0;
constexpr double EARLY_HOURS = 5.0;
constexpr double PLATEAU_MIN_HOURS = 4.0;
constexpr double PLATEAU_MAX_HOURS = 6.0;
constexpr double PLATEAU_QUIET_MINUTES = 60.0;
constexpr double PLATEAU_SHARE = 0.95;
constexpr double SECONDS_PER_MINUTE = 60.0;
constexpr double SECONDS_PER_HOUR = 3600.0;

std::uint32_t CountOf(const Species::RegionSimulation& _region, std::uint8_t _team, Species::StructureKind _kind)
{
  std::uint32_t count = 0;
  const Species::SlicedPool<Species::Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    count += structures.Used(slot) && structures.At(slot).kind == _kind && structures.At(slot).owner == std::optional<std::uint8_t>(_team)
               ? 1U
               : 0U;
  }
  return count;
}

bool ReachesAHalfwayLine(const Species::RegionSimulation& _region, std::uint8_t _site)
{
  for (std::size_t pair = 0; pair < Species::VALLEY_PAIRS.size(); ++pair)
  {
    if ((Species::VALLEY_PAIRS[pair][0] == _site || Species::VALLEY_PAIRS[pair][1] == _site) &&
        Species::ReachesHalfwayLine(_region, _site, pair))
    {
      return true;
    }
  }
  return false;
}

} // namespace

GrowthRun RunGrowth(std::uint64_t _seed, const Species::TuningMvp& _tuning, const Species::StructureRecords& _records, std::uint64_t _ticks,
                    std::unique_ptr<Species::WorldSimulation>* _world)
{
  GrowthRun run;
  run.seed = _seed;
  run.ticks = _ticks;
  auto generated = Species::GenerateRegionTerrain(_seed, _tuning);
  if (!generated)
  {
    return run;
  }
  const auto terrain = std::make_shared<const Species::RegionTerrain>(std::move(*generated));
  const auto site = static_cast<std::uint8_t>(std::clamp<std::int64_t>(_tuning.mvp.sandbox.stewardSite, 0, 3));
  Species::WorldStart start;
  start.key = GROWTH_KEY;
  start.seed = terrain->seed;
  start.stewardSite = site;
  std::unique_ptr<Species::WorldSimulation> world = Species::StartWorld(start, terrain, _tuning, &_records);
  Species::RegionSimulation& region = world->Region(0);
  static_cast<void>(region.AddScriptedSteward(site));
  const std::int64_t sampleTicks = std::max<std::int64_t>(1, Species::SampleTicks(_tuning));
  const auto minuteTicks =
    static_cast<std::uint64_t>(std::max<long long>(1, std::llround(SECONDS_PER_MINUTE / _tuning.ref.sim.tickPeriod)));
  Neuron::WorkerPool serial(0);
  std::int64_t stepNanoseconds = 0;
  std::int64_t steppedTicks = 0;
  for (std::uint64_t tick = 0; tick < _ticks; ++tick)
  {
    const std::int64_t begin = Neuron::MarkerClockNanoseconds();
    world->Step(tick, serial);
    stepNanoseconds += Neuron::MarkerClockNanoseconds() - begin;
    ++steppedTicks;
    const Species::Settlement& settlement = region.Settlements().front();
    if (!run.halfwayTick && tick % minuteTicks == 0 && ReachesAHalfwayLine(region, site))
    {
      run.halfwayTick = tick;
    }
    if (tick % static_cast<std::uint64_t>(sampleTicks) == 0)
    {
      const Species::SettlementStatus status = Species::StatusOf(region, settlement, tick);
      GrowthSample sample;
      sample.tick = tick;
      sample.workers = status.population;
      sample.limit = status.populationLimit;
      sample.storesMilliRu = status.storesMilliRu;
      sample.strengthMilliRu = status.strengthMilliRu;
      sample.tasks = status.tasks;
      sample.idle = status.idle;
      sample.dwellings = CountOf(region, site, Species::StructureKind::Dwelling);
      sample.storehouses = CountOf(region, site, Species::StructureKind::Storehouse);
      sample.stepMicroseconds = steppedTicks == 0 ? 0.0 : static_cast<double>(stepNanoseconds) / static_cast<double>(steppedTicks) / 1000.0;
      run.samples.push_back(sample);
      stepNanoseconds = 0;
      steppedTicks = 0;
    }
    run.largestWorkers = std::max(run.largestWorkers, Species::Population(region, settlement));
  }
  for (const Species::StatisticEntry& entry : region.Statistics())
  {
    if (Species::IsGrowthStep(entry.kind))
    {
      run.steps.push_back(entry);
    }
  }
  if (_world != nullptr)
  {
    *_world = std::move(world);
  }
  return run;
}

GrowthJudgement JudgeGrowth(const GrowthRun& _run, double _tickPeriodSeconds)
{
  GrowthJudgement judgement;
  const auto ticksOf = [_tickPeriodSeconds](double _seconds)
  { return static_cast<std::uint64_t>(std::llround(_seconds / _tickPeriodSeconds)); };
  const std::uint64_t quiet = ticksOf(PLATEAU_QUIET_MINUTES * SECONDS_PER_MINUTE);
  for (std::size_t index = 0; index < _run.steps.size(); ++index)
  {
    const std::uint64_t at = _run.steps[index].tick;
    const std::uint64_t following = index + 1 < _run.steps.size() ? _run.steps[index + 1].tick : _run.ticks;
    if (following - at >= quiet &&
        static_cast<double>(_run.steps[index].workers) >= PLATEAU_SHARE * static_cast<double>(_run.largestWorkers))
    {
      judgement.plateauTick = at;
      break;
    }
  }
  const std::uint64_t early = ticksOf(EARLY_HOURS * SECONDS_PER_HOUR);
  const std::uint64_t end = std::min(early, judgement.plateauTick.value_or(_run.ticks));
  std::uint64_t previous = 0;
  for (const Species::StatisticEntry& step : _run.steps)
  {
    if (step.tick > end)
    {
      break;
    }
    if (step.tick - previous > judgement.largestGapTicks)
    {
      judgement.largestGapTicks = step.tick - previous;
      judgement.gapFromTick = previous;
    }
    previous = step.tick;
  }
  if ((!judgement.plateauTick || *judgement.plateauTick > end) && end > previous && end - previous > judgement.largestGapTicks)
  {
    judgement.largestGapTicks = end - previous;
    judgement.gapFromTick = previous;
  }
  judgement.gapMet = judgement.largestGapTicks <= ticksOf(STEP_MINUTES * SECONDS_PER_MINUTE);
  judgement.plateauMet = judgement.plateauTick && *judgement.plateauTick >= ticksOf(PLATEAU_MIN_HOURS * SECONDS_PER_HOUR) &&
                         *judgement.plateauTick <= ticksOf(PLATEAU_MAX_HOURS * SECONDS_PER_HOUR);
  return judgement;
}

std::optional<SnapshotCost> MeasureSnapshot(const Species::WorldSimulation& _world, const std::filesystem::path& _folder)
{
  SnapshotCost cost;
  const std::int64_t start = Neuron::MarkerClockNanoseconds();
  const Neuron::SnapshotContents contents = Species::SnapshotOf(_world);
  const std::vector<std::byte> file = Neuron::EncodeSnapshot(contents);
  cost.encodeNanoseconds = Neuron::MarkerClockNanoseconds() - start;
  cost.bytes = file.size();
  Neuron::SnapshotStore store(_folder, 1);
  const std::int64_t saving = Neuron::MarkerClockNanoseconds();
  if (!store.Save(contents))
  {
    return std::nullopt;
  }
  cost.saveNanoseconds = Neuron::MarkerClockNanoseconds() - saving;
  return cost;
}

} // namespace Performance
