#include "pch.h"

#include "Construction.h"
#include "RegionSimulation.h"
#include "ScriptedSteward.h"
#include "StateHash.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_TEAM = 2; // SettledWorld's settlement, whose team is its site

std::int64_t Queued(const Species::RegionSimulation& _region, Species::StructureKind _kind)
{
  const Species::Settlement& settlement = _region.Settlements().front();
  return std::ranges::count_if(settlement.queue,
                               [&](const Species::StructureRef& _site)
                               {
                                 const Species::Structure* structure = _region.ResolveStructure(_site.slot, _site.id);
                                 return structure != nullptr && structure->kind == _kind;
                               });
}

// The settlement's population raised to _workers by Workers at its door.
void GrowTo(Species::RegionSimulation& _region, std::uint32_t _workers)
{
  const Species::Settlement& settlement = _region.Settlements().front();
  while (Species::Population(_region, settlement) < _workers)
  {
    static_cast<void>(_region.SpawnAtDoor(settlement));
  }
}

// One look of the steward and the tick that executes what it sent.
void LookAndExecute(Species::WorldSimulation& _world, Neuron::WorkerPool& _pool, std::uint64_t& _tick)
{
  const auto think = static_cast<int>(std::llround(CommittedTuning().mvp.steward.thinkS / CommittedTuning().ref.sim.tickPeriod));
  for (int step = 0; step <= think; ++step)
  {
    _world.Step(_tick++, _pool);
  }
}

} // namespace

TEST_CLASS(ScriptedStewardTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp056TheDwellingLayoutKeepsTheLanesToTheResourceSitesClear)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-056")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-032")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp056TheDwellingLayoutKeepsTheLanesToTheResourceSitesClear)
  {
    // Rings from mvp.steward.ring_first, mvp.steward.ring_step apart, none of whose places lies within
    // mvp.steward.lane_half_width of the line from the Wellspring to a home resource site; the storehouse spots are the
    // midpoints of the four pairs of neighbouring outer sites, none held by the founding territory.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    const Species::RegionSimulation& region = world->Region(0);
    const Species::Settlement& settlement = region.Settlements().front();
    const auto& steward = CommittedTuning().mvp.steward;
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    const std::vector<Neuron::Vec3> layout = Species::DwellingLayout(region, settlement);
    Assert::IsTrue(layout.size() > 40, L"places for every dwelling a settlement needs");
    double previous = 0.0;
    for (const Neuron::Vec3& place : layout)
    {
      const double distance = Neuron::HorizontalDistance(place, wellspring);
      const double ring = (distance - steward.ringFirst) / steward.ringStep;
      Assert::IsTrue(std::abs(ring - std::round(ring)) < 1e-6 && ring > -1e-6 && ring < static_cast<double>(steward.ringsMax),
                     L"on a ring");
      Assert::IsTrue(distance >= previous - 1e-6, L"ring by ring outwards");
      previous = distance;
      for (const Species::ResourceSitePlace& site : region.Terrain()->resourceSites)
      {
        if (site.siteClass != Species::ResourceSiteClass::Home || site.site != std::optional<std::uint8_t>(settlement.site))
        {
          continue;
        }
        const double dx = site.position.x - wellspring.x;
        const double dz = site.position.z - wellspring.z;
        const double along = std::clamp(((place.x - wellspring.x) * dx + (place.z - wellspring.z) * dz) / (dx * dx + dz * dz), 0.0, 1.0);
        Assert::IsTrue(std::hypot(place.x - wellspring.x - along * dx, place.z - wellspring.z - along * dz) >= steward.laneHalfWidth,
                       L"clear of every lane");
      }
    }
    const std::vector<Species::StorehouseSpot> spots = Species::StorehouseSpots(region, settlement);
    ExpectEqual(4, static_cast<std::int64_t>(spots.size()), L"four pairs of neighbouring outer sites");
    Assert::IsTrue(std::ranges::none_of(spots, &Species::StorehouseSpot::held), L"none held at the founding radius");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp056TheScriptedStewardBuildsADwellingNearTheLimitAndStorehousesWhenTheOuterSitesAreHeld)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-056")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-034")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp056TheScriptedStewardBuildsADwellingNearTheLimitAndStorehousesWhenTheOuterSitesAreHeld)
  {
    // The steward commands its settlement as an AI and acts through the queue. At 20 Workers of 40 it places nothing;
    // within 5 of the limit it places one dwelling, at the first place of its layout, and no second while that one is
    // queued. Once the territory holds two neighbouring outer resource sites it places a storehouse about their
    // midpoint, one at a time.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    const Species::StableId id = region.AddScriptedSteward(SETTLED_TEAM);
    Assert::IsTrue(region.MayCommand(id, SETTLED_TEAM, true), L"its team's AI commander");
    settlement.storesMilliRu = Species::MilliRuOf(1'000.0);
    Neuron::WorkerPool pool(0);
    std::uint64_t tick = 0;
    LookAndExecute(*world, pool, tick);
    ExpectEqual(0, static_cast<std::int64_t>(settlement.queue.size()), L"nothing at 20 Workers of 40");

    GrowTo(region, 35);
    const std::vector<Neuron::Vec3> layout = Species::DwellingLayout(region, settlement);
    const auto first = std::ranges::find_if(
      layout, [&](const Neuron::Vec3& _place)
      { return !Species::CheckPlacement(region, settlement.site, Species::StructureKind::Dwelling, _place.x, _place.z).has_value(); });
    Assert::IsTrue(first != layout.end(), L"a place in the layout");
    LookAndExecute(*world, pool, tick);
    ExpectEqual(1, Queued(region, Species::StructureKind::Dwelling), L"one dwelling within the margin");
    const Species::Structure& dwelling = region.Structures().At(settlement.queue.front().slot);
    Assert::IsTrue(Neuron::HorizontalDistance(dwelling.position, *first) < 1e-6, L"at the layout's first place");
    LookAndExecute(*world, pool, tick);
    ExpectEqual(1, Queued(region, Species::StructureKind::Dwelling), L"no second while it is queued");
    ExpectEqual(0, Queued(region, Species::StructureKind::Storehouse), L"no storehouse while no outer site is held");

    // 120 Workers widen the territory by 400 wu, over the outer sites at 800 wu.
    GrowTo(region, 120);
    settlement.storesMilliRu = Species::MilliRuOf(1'500.0);
    world->Step(tick++, pool); // the territory grows in the settlement's step
    const std::vector<Species::StorehouseSpot> spots = Species::StorehouseSpots(region, settlement);
    const auto held = std::ranges::find_if(spots, &Species::StorehouseSpot::held);
    Assert::IsTrue(held != spots.end(), L"a pair of outer sites held");
    LookAndExecute(*world, pool, tick);
    ExpectEqual(1, Queued(region, Species::StructureKind::Storehouse), L"one storehouse");
    const auto storehouse = std::ranges::find_if(settlement.queue, [&](const Species::StructureRef& _site)
                                                 { return region.Structures().At(_site.slot).kind == Species::StructureKind::Storehouse; });
    const auto& steward = CommittedTuning().mvp.steward;
    Assert::IsTrue(Neuron::HorizontalDistance(region.Structures().At(storehouse->slot).position, held->position) <=
                     steward.storehouseSearchStep * static_cast<double>(steward.storehouseSearchRings) + 1e-6,
                   L"about the first held spot");
    LookAndExecute(*world, pool, tick);
    ExpectEqual(1, Queued(region, Species::StructureKind::Storehouse), L"one at a time");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095TheScriptedStewardIsSavedAndStepsAlikeAtAnyWorkerCount)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095TheScriptedStewardIsSavedAndStepsAlikeAtAnyWorkerCount)
  {
    // The steward is region state: a world restored from its snapshot holds it and steps on with the same hashes as the
    // world saved, and its settlement's run hashes alike at 0, 1, 2 and 8 worker threads.
    const auto run = [](std::size_t _workers)
    {
      const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
      Species::RegionSimulation& region = world->Region(0);
      static_cast<void>(region.AddScriptedSteward(SETTLED_TEAM));
      region.Settlements().front().storesMilliRu = Species::MilliRuOf(1'000.0);
      GrowTo(region, 36);
      Neuron::WorkerPool pool(_workers);
      std::vector<std::uint64_t> hashes;
      for (std::uint64_t tick = 0; tick < 200; ++tick)
      {
        world->Step(tick, pool);
        hashes.push_back(Species::HashWorld(*world).world);
      }
      return hashes;
    };
    const std::vector<std::uint64_t> serial = run(0);
    for (const std::size_t workers : {std::size_t{1}, std::size_t{2}, std::size_t{8}})
    {
      Assert::IsTrue(run(workers) == serial, L"the same hashes at every worker count");
    }

    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    static_cast<void>(world->Region(0).AddScriptedSteward(SETTLED_TEAM));
    Neuron::WorkerPool pool(0);
    for (std::uint64_t tick = 0; tick < 30; ++tick)
    {
      world->Step(tick, pool);
    }
    auto restored = Species::RestoreWorld(Species::SnapshotOf(*world), CommittedTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value(), L"restores");
    std::unique_ptr<Species::WorldSimulation> copy = std::move(restored.value());
    copy->Region(0).AttachTerrain(ValleyTerrain());
    ExpectEqual(1, static_cast<std::int64_t>(copy->Region(0).ScriptedStewards().size()), L"its steward");
    for (std::uint64_t tick = 30; tick < 60; ++tick)
    {
      world->Step(tick, pool);
      copy->Step(tick, pool);
      Assert::IsTrue(Species::HashWorld(*world).world == Species::HashWorld(*copy).world, L"the same hash");
    }
  }
};

} // namespace GameLogicTests
