#include "pch.h"

#include "Construction.h"
#include "RegionSimulation.h"
#include "StateHash.h"
#include "Statistics.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <algorithm>
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
constexpr std::uint64_t TICKS_PER_MINUTE = 600;

// SettledWorld run by the scripted steward for _ticks on _workers, and the hash after each of its first _hashed ticks.
std::unique_ptr<Species::WorldSimulation> Grown(std::uint64_t _ticks, std::size_t _workers, std::uint64_t _hashed,
                                                std::vector<std::uint64_t>& _hashes)
{
  std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
  static_cast<void>(world->Region(0).AddScriptedSteward(SETTLED_TEAM));
  Neuron::WorkerPool pool(_workers);
  for (std::uint64_t tick = 0; tick < _ticks; ++tick)
  {
    world->Step(tick, pool);
    if (tick < _hashed)
    {
      _hashes.push_back(Species::HashWorld(*world).world);
    }
  }
  return world;
}

} // namespace

TEST_CLASS(GrowthSceneTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp064TheFirstHourOfTheGrowthSceneStaysInsideItsBounds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-064")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-056")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp064TheFirstHourOfTheGrowthSceneStaysInsideItsBounds)
  {
    // The M2 plan §5's regression in CI (ADR-138): the growth scene's first hour of play on the tests' valley and
    // records, at 2 worker threads, whose first five minutes hash alike at 0. In the hour: no gap of more than 20
    // minutes between growth steps from its start, a dwelling built by 50 minutes, and between 50 and 75 Workers at its
    // end. On T10's tuning a scene of this valley took its first step at 15 minutes, built its first dwelling at 40 and
    // had 60 Workers at the hour.
    std::vector<std::uint64_t> parallel;
    std::vector<std::uint64_t> serial;
    const std::unique_ptr<Species::WorldSimulation> world = Grown(60 * TICKS_PER_MINUTE, 2, 5 * TICKS_PER_MINUTE, parallel);
    static_cast<void>(Grown(5 * TICKS_PER_MINUTE, 0, 5 * TICKS_PER_MINUTE, serial));
    Assert::IsTrue(serial == parallel, L"the same hash on every tick at 0 and 2 worker threads");

    const Species::RegionSimulation& region = world->Region(0);
    std::vector<Species::StatisticEntry> steps;
    std::ranges::copy_if(region.Statistics(), std::back_inserter(steps),
                         [](const Species::StatisticEntry& _entry) { return Species::IsGrowthStep(_entry.kind); });
    Assert::IsFalse(steps.empty(), L"growth steps");
    std::uint64_t previous = 0;
    for (const Species::StatisticEntry& step : steps)
    {
      Assert::IsTrue(step.tick - previous <= 20 * TICKS_PER_MINUTE, L"no gap of more than 20 minutes");
      previous = step.tick;
    }
    Assert::IsTrue(60 * TICKS_PER_MINUTE - previous <= 20 * TICKS_PER_MINUTE, L"nor at the hour's end");
    Assert::IsTrue(std::ranges::any_of(steps,
                                       [](const Species::StatisticEntry& _step)
                                       {
                                         return _step.kind == Species::StatisticKind::StructureCompleted &&
                                                _step.detail == static_cast<std::uint32_t>(Species::StructureKind::Dwelling) &&
                                                _step.tick <= 50 * TICKS_PER_MINUTE;
                                       }),
                   L"a dwelling built by 50 minutes");
    const std::uint32_t workers = Species::Population(region, region.Settlements().front());
    Assert::IsTrue(workers >= 50 && workers <= 75, L"50 to 75 Workers after an hour");
  }
};

} // namespace GameLogicTests
