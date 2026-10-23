#include "pch.h"

#include "CommandJournal.h"
#include "Construction.h"
#include "MessageCodec.h"
#include "RegionSimulation.h"
#include "SettlementMessages.h"
#include "Statistics.h"
#include "TestSupport.h"
#include "Upkeep.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_TEAM = 2; // SettledWorld's settlement, whose team is its site
constexpr Neuron::SenderId STEWARD = 7;
constexpr Neuron::SipHashKey STATISTICS_TEST_KEY{0x57a7157c57a7157c, 0x0000000000000064};

// The committed tuning with a sample every 30 s of world time, 300 ticks, so that a short run takes several.
const Species::TuningMvp& ShortSampleTuning()
{
  static const Species::TuningMvp TUNING = []
  {
    Species::TuningMvp tuning = CommittedTuning();
    tuning.mvp.stats.sampleMinutes = 0.5;
    return tuning;
  }();
  return TUNING;
}

// SettledWorld on _tuning.
std::unique_ptr<Species::WorldSimulation> SettledWorldOn(const Species::TuningMvp& _tuning)
{
  Species::WorldStart start;
  start.key = STATISTICS_TEST_KEY;
  start.seed = ValleyTerrain()->seed;
  start.stewardSite = SETTLED_TEAM;
  return Species::StartWorld(start, ValleyTerrain(), _tuning, &TestRecords());
}

std::vector<Species::StatisticEntry> OfKind(const Species::RegionSimulation& _region, Species::StatisticKind _kind)
{
  std::vector<Species::StatisticEntry> entries;
  std::ranges::copy_if(_region.Statistics(), std::back_inserter(entries),
                       [_kind](const Species::StatisticEntry& _entry) { return _entry.kind == _kind; });
  return entries;
}

// A settled world on the short samples, its settlement commanded by the steward, run for _ticks with a recorded command
// stream: a dwelling placed round the Wellspring at tick 20, wherever the first of twelve placements passes, and new
// priorities at tick 200. Its statistics at the end.
std::vector<Species::StatisticEntry> RecordedRun(std::size_t _workers, std::uint64_t _ticks)
{
  const std::unique_ptr<Species::WorldSimulation> world = SettledWorldOn(ShortSampleTuning());
  Species::RegionSimulation& region = world->Region(0);
  region.SetCommander(SETTLED_TEAM, STEWARD, false);
  const Species::Settlement& settlement = region.Settlements().front();
  const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
  Neuron::WorkerPool pool(_workers);
  std::uint64_t sequence = 0;
  for (std::uint64_t tick = 0; tick < _ticks; ++tick)
  {
    if (tick == 20)
    {
      for (int turn = 0; turn < 12; ++turn)
      {
        const double angle = turn * 3.141592653589793 / 6.0;
        const Species::PlaceStructureCommand place{
          SETTLED_TEAM, Species::StructureKind::Dwelling, {wellspring.x + 150.0 * std::cos(angle), wellspring.z + 150.0 * std::sin(angle)}};
        Assert::IsTrue(region.Commands().Submit(STEWARD, sequence++, Neuron::EncodeMessage(place)), L"a placement");
      }
    }
    if (tick == 200)
    {
      Assert::IsTrue(
        region.Commands().Submit(STEWARD, sequence++, Neuron::EncodeMessage(Species::SetPrioritiesCommand{SETTLED_TEAM, {30, 60, 10}})),
        L"the priorities");
    }
    world->Step(tick, pool);
  }
  return region.Statistics();
}

} // namespace

TEST_CLASS(StatisticsTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp064TheServerRecordsSamplesAndGrowthSteps)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-064")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-038")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-055")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp064TheServerRecordsSamplesAndGrowthSteps)
  {
    // A sample every mvp.stats.sample_minutes from tick 0 with the Worker count and strength; a growth step for a
    // structure built, for each new multiple of mvp.stats.population_step, and for each resource site the territory
    // takes in, each once.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorldOn(ShortSampleTuning());
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    ExpectEqual(20, settlement.populationMark, L"the founding Workers' multiple");
    Neuron::WorkerPool pool(0);
    std::uint64_t tick = 0;
    world->Step(tick++, pool);
    const std::vector<Species::StatisticEntry> first = region.Statistics();
    ExpectEqual(1, static_cast<std::int64_t>(first.size()), L"the sample of tick 0 alone");
    Assert::IsTrue(first[0].kind == Species::StatisticKind::Sample && first[0].tick == 0 && first[0].site == SETTLED_TEAM, L"a sample");
    ExpectEqual(20, first[0].workers, L"its Worker count");
    ExpectEqual(Species::StrengthMilliRu(region, settlement), first[0].strengthMilliRu, L"its strength");

    // A dwelling built in this tick's structure step, before the settlement's own.
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    const std::uint32_t dwelling = region.AddStructure(Species::StructureKind::Dwelling, SETTLED_TEAM, std::nullopt, wellspring.x + 150.0,
                                                       wellspring.z, {1.0, 0.0, 0.0});
    region.Structures().At(dwelling).completedTick = tick;
    // Ten Workers more reach the next multiple, and twenty more the two after it.
    for (int worker = 0; worker < 10; ++worker)
    {
      static_cast<void>(region.SpawnAtDoor(settlement));
    }
    world->Step(tick++, pool);
    const std::vector<Species::StatisticEntry> built = OfKind(region, Species::StatisticKind::StructureCompleted);
    Assert::IsTrue(built.size() == 1 && built[0].tick == 1 &&
                     built[0].detail == static_cast<std::uint32_t>(Species::StructureKind::Dwelling),
                   L"the dwelling built at tick 1");
    std::vector<Species::StatisticEntry> steps = OfKind(region, Species::StatisticKind::PopulationStep);
    Assert::IsTrue(steps.size() == 1 && steps[0].detail == 30 && steps[0].workers == 30, L"30 Workers");
    for (int worker = 0; worker < 20; ++worker)
    {
      static_cast<void>(region.SpawnAtDoor(settlement));
    }
    world->Step(tick++, pool);
    steps = OfKind(region, Species::StatisticKind::PopulationStep);
    Assert::IsTrue(steps.size() == 3 && steps[1].detail == 40 && steps[2].detail == 50, L"40 and 50 Workers in one tick");
    ExpectEqual(50, settlement.populationMark, L"the mark");

    // A hundred Workers more widen the territory by 400 wu; each resource site it takes in is recorded once, by index.
    const std::vector<std::uint32_t> heldBefore = Species::HeldResourceSites(region, settlement);
    for (int worker = 0; worker < 100; ++worker)
    {
      static_cast<void>(region.SpawnAtDoor(settlement));
    }
    world->Step(tick++, pool);
    std::vector<std::uint32_t> taken;
    std::ranges::set_difference(Species::HeldResourceSites(region, settlement), heldBefore, std::back_inserter(taken));
    std::vector<std::uint32_t> recorded;
    for (const Species::StatisticEntry& entry : OfKind(region, Species::StatisticKind::ResourceSiteTaken))
    {
      recorded.push_back(entry.detail);
    }
    Assert::IsFalse(taken.empty(), L"the territory takes in a resource site of the outer ring");
    Assert::IsTrue(recorded == taken, L"each site taken in, recorded");
    const std::size_t multiples = OfKind(region, Species::StatisticKind::PopulationStep).size();
    ExpectEqual(13, static_cast<std::int64_t>(multiples), L"every multiple from 30 to 150");

    // Without ten Workers more there is no new multiple; the next sample comes at tick 300.
    for (; tick <= 300; ++tick)
    {
      world->Step(tick, pool);
    }
    ExpectEqual(static_cast<std::int64_t>(multiples),
                static_cast<std::int64_t>(OfKind(region, Species::StatisticKind::PopulationStep).size()), L"no new multiple");
    const std::vector<Species::StatisticEntry> samples = OfKind(region, Species::StatisticKind::Sample);
    Assert::IsTrue(samples.size() == 2 && samples[1].tick == 300, L"the samples of ticks 0 and 300");
    // From M3 task T8 a territory also records the halfway lines it reaches (ADR-163); a lone settlement has no other event.
    Assert::IsTrue(std::ranges::all_of(region.Statistics(),
                                       [](const Species::StatisticEntry& _entry)
                                       {
                                         return _entry.kind == Species::StatisticKind::Sample || Species::IsGrowthStep(_entry.kind) ||
                                                _entry.kind == Species::StatisticKind::HalfwayReached;
                                       }),
                   L"every entry a sample, a growth step or a halfway line reached");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp064TheStatisticsOfARecordedCommandStreamAreTheSameOnTwoRuns)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-064")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp064TheStatisticsOfARecordedCommandStreamAreTheSameOnTwoRuns)
  {
    // The statistics are state, made by the tick alone: the same command stream gives the same entries on a second
    // run and at another worker count.
    const std::vector<Species::StatisticEntry> first = RecordedRun(0, 900);
    Assert::IsTrue(first.size() >= 3, L"at least the three samples");
    Assert::IsTrue(RecordedRun(0, 900) == first, L"the same on a second run");
    Assert::IsTrue(RecordedRun(2, 900) == first, L"and at two worker threads");
  }
};

} // namespace GameLogicTests
