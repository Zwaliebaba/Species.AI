#include "pch.h"

#include "Construction.h"
#include "TestSupport.h"
#include "Upkeep.h"
#include "WorldSimulation.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_TEAM = 2; // SettledWorld's settlement, whose team is its site

// A built dwelling of the settlement 150 wu out at _degrees, completed at _tick.
std::uint32_t DwellingAt(Species::RegionSimulation& _region, double _degrees, std::uint64_t _tick)
{
  const Species::Settlement& settlement = _region.Settlements().front();
  const Neuron::Vec3 wellspring = _region.Structures().At(settlement.wellspringSlot).position;
  const double radians = _degrees * 3.141592653589793 / 180.0;
  const std::uint32_t slot =
    _region.AddStructure(Species::StructureKind::Dwelling, SETTLED_TEAM, std::nullopt, wellspring.x + 150.0 * std::cos(radians),
                         wellspring.z + 150.0 * std::sin(radians), {1.0, 0.0, 0.0});
  _region.Structures().At(slot).completedTick = _tick;
  return slot;
}

} // namespace

TEST_CLASS(UpkeepTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp035UpkeepIsPaidExactlyFromTheStores)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-035")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-030")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp035UpkeepIsPaidExactlyFromTheStores)
  {
    // 20 Workers at 6 ru an hour pay exactly 12 ru in a tenth of an hour, 3,600 ticks; with a dwelling at 30 ru an hour,
    // exactly 15 ru the next tenth.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    settlement.storesMilliRu = Species::MilliRuOf(100.0);
    const std::int64_t tenth = Species::TicksPerHour(CommittedTuning()) / 10;
    for (std::int64_t tick = 0; tick < tenth; ++tick)
    {
      Species::PayUpkeep(region, settlement);
    }
    ExpectEqual(Species::MilliRuOf(88.0), settlement.storesMilliRu, L"20 Workers' upkeep");
    static_cast<void>(DwellingAt(region, 0.0, 0));
    for (std::int64_t tick = 0; tick < tenth; ++tick)
    {
      Species::PayUpkeep(region, settlement);
    }
    ExpectEqual(Species::MilliRuOf(73.0), settlement.storesMilliRu, L"and a dwelling's");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp035AnEmptyStoreTurnsTheNewestStructuresInactiveAndTheirReturnIsReversed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-035")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-022")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp035AnEmptyStoreTurnsTheNewestStructuresInactiveAndTheirReturnIsReversed)
  {
    // Three dwellings completed in turn and empty stores: every mvp.upkeep.inactive_step_s the most recently completed
    // active one becomes inactive and stops counting for the limit. Once the stores hold something, they become active
    // again in reverse, the earliest completed of the inactive first.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    const std::array<std::uint32_t, 3> dwellings{DwellingAt(region, 0.0, 10), DwellingAt(region, 120.0, 20), DwellingAt(region, 240.0, 30)};
    ExpectEqual(130, Species::PopulationLimitOf(region, settlement), L"40 and three dwellings");
    const auto step = static_cast<int>(CommittedTuning().mvp.upkeep.inactiveStepS / CommittedTuning().ref.sim.tickPeriod);
    const auto active = [&]
    {
      return std::array<bool, 3>{region.Structures().At(dwellings[0]).active, region.Structures().At(dwellings[1]).active,
                                 region.Structures().At(dwellings[2]).active};
    };
    const auto pay = [&](int _ticks)
    {
      for (int tick = 0; tick < _ticks; ++tick)
      {
        Species::PayUpkeep(region, settlement);
      }
    };
    settlement.storesMilliRu = 0;
    pay(step - 1);
    Assert::IsTrue(active() == std::array<bool, 3>{true, true, true}, L"not before a step");
    pay(1);
    Assert::IsTrue(active() == std::array<bool, 3>{true, true, false}, L"the newest first");
    ExpectEqual(100, Species::PopulationLimitOf(region, settlement), L"an inactive dwelling has no effect");
    pay(step);
    Assert::IsTrue(active() == std::array<bool, 3>{true, false, false}, L"then the next");
    settlement.storesMilliRu = Species::MilliRuOf(1000.0);
    pay(step);
    Assert::IsTrue(active() == std::array<bool, 3>{true, true, false}, L"the return reversed: the earliest of the inactive");
    pay(step);
    Assert::IsTrue(active() == std::array<bool, 3>{true, true, true}, L"then the last");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp038AStrengthIsItsWorkersStoresAndStructures)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-038")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp038AStrengthIsItsWorkersStoresAndStructures)
  {
    // 20 Workers at 20 ru, 400 ru of stores less a dwelling's 300 ru, and the dwelling at its cost: 800 ru.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    settlement.storesMilliRu = Species::MilliRuOf(400.0);
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    bool placed = false;
    for (int turn = 0; turn < 36 && !placed; ++turn)
    {
      const double angle = turn * 3.141592653589793 / 18.0;
      placed = Species::PlaceStructure(region, settlement.site, Species::StructureKind::Dwelling, wellspring.x + 150.0 * std::cos(angle),
                                       wellspring.z + 150.0 * std::sin(angle))
                 .has_value();
    }
    Assert::IsTrue(placed, L"a dwelling placed");
    ExpectEqual(Species::MilliRuOf(800.0), Species::StrengthMilliRu(region, settlement), L"its strength");
  }
};

} // namespace GameLogicTests
