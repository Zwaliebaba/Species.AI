#include "pch.h"

#include "Construction.h"
#include "EssenceOrb.h"
#include "Growth.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <numbers>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_SITE = 2; // SettledWorld's settlement, whose team is its site

std::size_t OrbsIn(const Species::RegionSimulation& _region, Species::OrbState _state)
{
  std::size_t count = 0;
  const auto& orbs = _region.Orbs();
  for (std::uint32_t slot = 0; slot < orbs.Capacity(); ++slot)
  {
    count += orbs.Used(slot) && orbs.At(slot).state == _state ? 1 : 0;
  }
  return count;
}

// The held cell of the settlement's territory nearest _distance from its Wellspring along +z.
Neuron::Vec3 HeldPointAhead(const Species::RegionSimulation& _region, double _distance)
{
  const Species::Settlement& settlement = _region.Settlements().front();
  const Neuron::Vec3 wellspring = _region.Structures().At(settlement.wellspringSlot).position;
  const Species::CellRaster& territory = *_region.TerritoryOf(SETTLED_SITE);
  for (int turn = 0; turn < 36; ++turn)
  {
    const double angle = turn * std::numbers::pi / 18.0;
    const Neuron::Vec3 point = wellspring + Neuron::Vec3{_distance * std::sin(angle), 0.0, _distance * std::cos(angle)};
    if (territory.Holds(point.x, point.z))
    {
      return {point.x, _region.Terrain()->field.Height(point.x, point.z), point.z};
    }
  }
  Assert::Fail(L"a held point at the distance");
}

} // namespace

TEST_CLASS(EssenceTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp024ASettlementWithFullStoresGrowsAWorkerEveryIntervalToItsLimit)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-024")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-025")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-022")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp024ASettlementWithFullStoresGrowsAWorkerEveryIntervalToItsLimit)
  {
    // The M2 plan's T6: a settlement of 20 Workers with full stores grows by one Worker every
    // mvp.essence.wellspring_interval_s to its limit of 40, paying mvp.essence.orb_cost_ru for each, and no further. The
    // Wellspring pays for its orbs exactly an interval apart; each Worker follows its orb's birth, whose rise varies by a
    // few ticks (R-UNIT-311).
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    const std::int64_t stores = Species::MilliRuOf(1000.0);
    settlement.storesMilliRu = stores;
    settlement.priorities = {0, 0, 100}; // guard duty alone, so that nothing is gathered
    const std::int32_t interval = Species::GrowthIntervalTicks(CommittedTuning());
    std::vector<std::uint64_t> grown;
    std::vector<std::uint64_t> paid;
    std::int64_t held = settlement.storesMilliRu;
    std::uint32_t population = Species::Population(region, settlement);
    ExpectEqual(20, population, L"the starting Workers");
    for (std::uint64_t tick = 0; tick < 60 + 22 * static_cast<std::uint64_t>(interval); ++tick)
    {
      world->Step(tick, pool);
      if (settlement.storesMilliRu < held)
      {
        paid.push_back(tick);
      }
      held = settlement.storesMilliRu;
      const std::uint32_t now = Species::Population(region, settlement);
      if (now > population)
      {
        ExpectEqual(population + 1, now, L"one Worker at a time");
        grown.push_back(tick);
      }
      population = now;
    }
    ExpectEqual(40, population, L"its limit");
    ExpectEqual(20, static_cast<std::int64_t>(grown.size()), L"20 Workers grown");
    ExpectEqual(20, static_cast<std::int64_t>(paid.size()), L"20 orbs paid for");
    for (std::size_t step = 1; step < paid.size(); ++step)
    {
      ExpectEqual(interval, static_cast<std::int64_t>(paid[step] - paid[step - 1]), L"an orb every interval");
      Assert::IsTrue(std::abs(static_cast<double>(grown[step] - grown[step - 1]) - interval) < interval / 10.0, L"a Worker every interval");
    }
    ExpectEqual(stores - 20 * Species::MilliRuOf(CommittedTuning().mvp.essence.orbCostRu), settlement.storesMilliRu, L"20 orbs paid");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp023AnOrbInTheTerritoryDriftsToItsWellspringAndBecomesAWorker)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-023")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-025")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp023AnOrbInTheTerritoryDriftsToItsWellspringAndBecomesAWorker)
  {
    // An orb released 300 wu from the Wellspring inside the territory drifts home once born, closing at the drift
    // speed, and becomes a Worker there; an orb outside every territory never drifts.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    const Species::Settlement& settlement = region.Settlements().front();
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    const Neuron::Vec3 inside = HeldPointAhead(region, 300.0);
    const Neuron::Vec3& outside = region.Terrain()->layout.sites[0];
    Species::SpawnOrb(region, inside, {}, 0, 0);
    Species::SpawnOrb(region, {outside.x, region.Terrain()->field.Height(outside.x, outside.z), outside.z}, {}, 0, 0);
    const std::uint32_t population = Species::Population(region, settlement);
    std::uint64_t tick = 0;
    for (; tick < 400 && OrbsIn(region, Species::OrbState::Drifting) == 0; ++tick)
    {
      world->Step(tick, pool);
    }
    ExpectEqual(1, static_cast<std::int64_t>(OrbsIn(region, Species::OrbState::Drifting)), L"the orb inside drifts");
    const auto distance = [&]
    {
      const auto& orbs = region.Orbs();
      for (std::uint32_t slot = 0; slot < orbs.Capacity(); ++slot)
      {
        if (orbs.Used(slot) && orbs.At(slot).state == Species::OrbState::Drifting)
        {
          return Neuron::HorizontalDistance(orbs.At(slot).position, wellspring);
        }
      }
      return 0.0;
    };
    const double start = distance();
    for (int step = 0; step < 50; ++step)
    {
      world->Step(tick++, pool);
    }
    const double closing = (start - distance()) / (50 * CommittedTuning().ref.sim.tickPeriod);
    Assert::IsTrue(std::abs(closing - CommittedTuning().mvp.essence.driftSpeed) < 3.0, L"at the drift speed");
    for (int step = 0; step < 200 && OrbsIn(region, Species::OrbState::Drifting) != 0; ++step)
    {
      world->Step(tick++, pool);
    }
    ExpectEqual(population + 1, Species::Population(region, settlement), L"a Worker at its Wellspring");
    ExpectEqual(1, static_cast<std::int64_t>(region.Orbs().Count()), L"the other orb is still out");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp025AtItsLimitOrbsWaitUpToTheBoundAndTheRestAscend)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-025")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp025AtItsLimitOrbsWaitUpToTheBoundAndTheRestAscend)
  {
    // At its limit of 40, 21 orbs reaching the Wellspring: 20 wait and the 21st ascends. When a Worker dies, a waiting
    // orb becomes a Worker at once.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    const Species::Settlement& settlement = region.Settlements().front();
    for (int worker = 0; worker < 20; ++worker)
    {
      static_cast<void>(region.SpawnAtDoor(settlement));
    }
    ExpectEqual(40, Species::Population(region, settlement), L"at its limit");
    const Species::Structure& wellspring = region.Structures().At(settlement.wellspringSlot);
    for (int orb = 0; orb < 21; ++orb)
    {
      Species::SpawnOrb(region, wellspring.center, {}, SETTLED_SITE, wellspring.id);
    }
    std::uint64_t tick = 0;
    for (; tick < 200 && OrbsIn(region, Species::OrbState::Waiting) + OrbsIn(region, Species::OrbState::Ascent) < 21; ++tick)
    {
      world->Step(tick, pool);
    }
    ExpectEqual(20, static_cast<std::int64_t>(OrbsIn(region, Species::OrbState::Waiting)), L"mvp.essence.wait_max wait");
    ExpectEqual(1, static_cast<std::int64_t>(OrbsIn(region, Species::OrbState::Ascent)), L"the rest ascend");
    ExpectEqual(40, Species::Population(region, settlement), L"still at its limit");

    const auto& figures = region.TeamAt(SETTLED_SITE).figures;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      if (figures.Used(slot) && !figures.At(slot).dead)
      {
        static_cast<void>(region.Damage({figures.At(slot).id, slot, SETTLED_SITE}, -1000, Species::DamageKind::Unresistable));
        break;
      }
    }
    world->Step(tick++, pool);
    ExpectEqual(40, Species::Population(region, settlement), L"a waiting orb takes the room");
    ExpectEqual(19, static_cast<std::int64_t>(OrbsIn(region, Species::OrbState::Waiting)), L"one fewer waits");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp026TheTerritoryGrowsWithItsWorkersAndNeverShrinks)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-026")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp026TheTerritoryGrowsWithItsWorkersAndNeverShrinks)
  {
    // Ten Workers above the starting 20 give a radius of 500 + 10 × 4 wu and a larger territory; five of them dying
    // leave it as it was.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    const Species::Settlement& settlement = region.Settlements().front();
    const std::size_t founding = region.TerritoryOf(SETTLED_SITE)->Count();
    std::vector<std::uint32_t> added;
    added.reserve(10);
    for (int worker = 0; worker < 10; ++worker)
    {
      added.push_back(region.SpawnAtDoor(settlement));
    }
    std::uint64_t tick = 0;
    world->Step(tick++, pool);
    const auto& tuning = CommittedTuning();
    Assert::IsTrue(
      std::abs(settlement.territoryRadius - (tuning.wld.territory.foundingRadius + 10 * tuning.mvp.territory.radiusPerWorker)) < 1e-9,
      L"4 wu for each Worker above 20");
    const std::size_t grown = region.TerritoryOf(SETTLED_SITE)->Count();
    Assert::IsTrue(grown > founding, L"a larger territory");
    for (std::size_t worker = 0; worker < 5; ++worker)
    {
      const Species::Figure& figure = region.TeamAt(SETTLED_SITE).figures.At(added[worker]);
      static_cast<void>(region.Damage({figure.id, added[worker], SETTLED_SITE}, -1000, Species::DamageKind::Unresistable));
    }
    world->Step(tick++, pool);
    Assert::IsTrue(
      std::abs(settlement.territoryRadius - (tuning.wld.territory.foundingRadius + 10 * tuning.mvp.territory.radiusPerWorker)) < 1e-9,
      L"never shrinks");
    ExpectEqual(static_cast<std::int64_t>(grown), static_cast<std::int64_t>(region.TerritoryOf(SETTLED_SITE)->Count()), L"the same cells");
  }
};

} // namespace GameLogicTests
