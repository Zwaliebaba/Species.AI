#include "pch.h"

#include "Construction.h"
#include "Territory.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <numbers>
#include <utility>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_SITE = 2; // the site of SettledWorld's settlement

// Every held cell of a territory, as its centre.
std::vector<Neuron::Vec3> HeldCells(const Species::CellRaster& _territory)
{
  std::vector<Neuron::Vec3> cells;
  for (std::int32_t z = 0; z < _territory.CellsPerAxis(); ++z)
  {
    for (std::int32_t x = 0; x < _territory.CellsPerAxis(); ++x)
    {
      if (_territory.HoldsCell(x, z))
      {
        cells.push_back(_territory.CellCenter(x, z));
      }
    }
  }
  return cells;
}

} // namespace

TEST_CLASS(TerritoryTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp026AFoundingTerritoryIsTheFoundingDiscWithinTheBounds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-026")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-060")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp026AFoundingTerritoryIsTheFoundingDiscWithinTheBounds)
  {
    // The founding territory holds the Wellspring's cell and every held cell lies within the founding radius and the
    // bounds; with nothing in its way it is the disc of 500 wu, within a cell's ring of π r².
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    const Species::RegionSimulation& region = world->Region(0);
    const Species::Settlement& settlement = region.Settlements().front();
    const Species::CellRaster* territory = region.TerritoryOf(SETTLED_SITE);
    Assert::IsNotNull(territory, L"the settlement's territory");
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    Assert::IsTrue(territory->Holds(wellspring.x, wellspring.z), L"its Wellspring's cell");
    for (const Neuron::Vec3& cell : HeldCells(*territory))
    {
      Assert::IsTrue(Neuron::HorizontalDistance(cell, wellspring) <= 500.0, L"within the founding radius");
      Assert::IsTrue(Species::WithinTerritoryBounds(region, SETTLED_SITE, cell.x, cell.z), L"within the bounds");
    }
    const double cellArea = territory->CellSize() * territory->CellSize();
    const double disc = std::numbers::pi * 500.0 * 500.0 / cellArea;
    const double ring = 2.0 * std::numbers::pi * 500.0 / territory->CellSize();
    Assert::IsTrue(static_cast<double>(territory->Count()) > disc - ring && static_cast<double>(territory->Count()) < disc + ring,
                   L"about the founding disc");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld060ATerritoryAtItsLargestStopsAtTheHalfwayLinesAndTheClearances)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-026")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-060")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-061")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-062")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld060ATerritoryAtItsLargestStopsAtTheHalfwayLinesAndTheClearances)
  {
    // At wld.territory.max_radius the territory stays on its own side of each valley neighbour's halfway line, out of the
    // coast bands, 600 wu from every other site's centre (its core and the site clearance), clear of the central resource
    // site by the wild clearance and on dry ground; and it reaches a halfway line, 1,040 wu from its site.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    region.Settlements().front().territoryRadius = CommittedTuning().wld.territory.maxRadius;
    region.RecomputeTerritory(SETTLED_SITE);
    const Species::CellRaster& territory = *region.TerritoryOf(SETTLED_SITE);
    const Species::RegionLayout& layout = region.Terrain()->layout;
    const auto& tuning = CommittedTuning();
    const Neuron::Vec3 own = layout.sites[SETTLED_SITE];
    bool reached = false;
    for (const Neuron::Vec3& cell : HeldCells(territory))
    {
      const double ownDistance = Neuron::HorizontalDistance(cell, own);
      for (const auto& pair : Species::VALLEY_PAIRS)
      {
        if (pair[0] != SETTLED_SITE && pair[1] != SETTLED_SITE)
        {
          continue;
        }
        const double neighbor = Neuron::HorizontalDistance(cell, layout.sites[pair[0] == SETTLED_SITE ? pair[1] : pair[0]]);
        Assert::IsTrue(ownDistance < neighbor, L"on its own side of the halfway line");
        reached = reached || neighbor - ownDistance < 2.0 * territory.CellSize();
      }
      for (std::uint8_t site = 0; site < 4; ++site)
      {
        Assert::IsTrue(site == SETTLED_SITE || Neuron::HorizontalDistance(cell, layout.sites[site]) >=
                                                 tuning.wld.site.coreRadius + tuning.wld.territory.siteClearance,
                       L"clear of every other site's core");
      }
      Assert::IsTrue(cell.x >= layout.originX + tuning.wld.region.coastBand && cell.z >= layout.originZ + tuning.wld.region.coastBand &&
                       cell.x <= layout.originX + layout.side - tuning.wld.region.coastBand &&
                       cell.z <= layout.originZ + layout.side - tuning.wld.region.coastBand,
                     L"out of the coast bands");
      Assert::IsTrue(region.Terrain()->field.Height(cell.x, cell.z) > tuning.ref.unit.worker.shoreHeight, L"on dry ground");
    }
    Assert::IsTrue(reached, L"it reaches a halfway line");
    const auto& structures = region.Structures();
    for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
    {
      if (structures.Used(slot) && structures.At(slot).kind == Species::StructureKind::ResourceSiteCentral)
      {
        const Species::Structure& central = structures.At(slot);
        for (const Neuron::Vec3& cell : HeldCells(territory))
        {
          Assert::IsTrue(Neuron::HorizontalDistance(cell, central.center) >= central.radius + tuning.wld.resource.wildClearance,
                         L"short of the central resource site");
        }
      }
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp032AStructureOverTheTerritorysEdgeIsOutside)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-032")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp032AStructureOverTheTerritorysEdgeIsOutside)
  {
    // A footprint well inside the founding territory is in it; one whose disc reaches more than a cell past its edge is
    // not, as the test reads the cells whose centres the disc covers (ADR-134).
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    const Species::RegionSimulation& region = world->Region(0);
    const Species::Settlement& settlement = region.Settlements().front();
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    const Neuron::Vec3 inside = wellspring + Neuron::Vec3{0.0, 0.0, 200.0};
    const Neuron::Vec3 edge = wellspring + Neuron::Vec3{0.0, 0.0, 480.0};
    Assert::IsTrue(Species::InTerritory(region, settlement, inside, 30.0) ||
                     !Species::WithinTerritoryBounds(region, SETTLED_SITE, inside.x, inside.z),
                   L"well inside");
    Assert::IsFalse(Species::InTerritory(region, settlement, edge, 50.0), L"over the edge by 30 wu");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr134ARestoredWorldMakesTheSameTerritory)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-026")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr134ARestoredWorldMakesTheSameTerritory)
  {
    // The territory is derived: a world restored from its snapshot makes the same cells from the radius it holds.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    world->Region(0).Settlements().front().territoryRadius = 800.0;
    world->Region(0).RecomputeTerritory(SETTLED_SITE);
    auto restored = Species::RestoreWorld(Species::SnapshotOf(*world), CommittedTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value(), L"restored");
    const std::unique_ptr<Species::WorldSimulation> back = std::move(restored.value());
    Assert::IsNull(back->Region(0).TerritoryOf(SETTLED_SITE), L"none before its terrain");
    back->Region(0).AttachTerrain(ValleyTerrain());
    const Species::CellRaster* territory = back->Region(0).TerritoryOf(SETTLED_SITE);
    Assert::IsNotNull(territory, L"its territory");
    ExpectEqual(static_cast<std::int64_t>(world->Region(0).TerritoryOf(SETTLED_SITE)->Count()),
                static_cast<std::int64_t>(territory->Count()), L"the same cells");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp027TwoSettlementsComeIntoContactAtTheirHalfwayLine)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-027")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-064")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-066")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp027TwoSettlementsComeIntoContactAtTheirHalfwayLine)
  {
    // A fixture of two settlements, at sites 2 and 3, valley neighbours: at their founding radius neither reaches their
    // halfway line and there is no contact; once both territories reach it, the step records one contact event with its
    // tick, the two sites and their border pair, and no second one after; the snapshot holds it.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    static_cast<void>(region.FoundSettlement(3, false));
    std::uint64_t tick = 0;
    world->Step(tick++, pool);
    Assert::IsTrue(region.ContactEvents().empty(), L"no contact at the founding radius");
    for (Species::Settlement& settlement : region.Settlements())
    {
      settlement.territoryRadius = CommittedTuning().wld.territory.maxRadius;
      region.RecomputeTerritory(settlement.site);
    }
    Assert::IsTrue(Species::ReachesHalfwayLine(region, 2, 1) && Species::ReachesHalfwayLine(region, 3, 1), L"both reach the line");
    const std::uint64_t contactTick = tick;
    world->Step(tick++, pool);
    world->Step(tick++, pool);
    ExpectEqual(1, static_cast<std::int64_t>(region.ContactEvents().size()), L"one contact event");
    const Species::ContactEvent expected{contactTick, Species::ContactEventKind::Contact, 2, 3, 1};
    Assert::IsTrue(region.ContactEvents().front() == expected, L"its tick, its two settlements and its border pair");
    auto restored = Species::RestoreWorld(Species::SnapshotOf(*world), CommittedTuning(), &TestRecords());
    Assert::IsTrue(restored.has_value() && (*restored)->Region(0).ContactEvents() == region.ContactEvents(), L"the snapshot holds it");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp016TheSeenLayerIsTheTerritoryAndTheAreaAroundItsFiguresAndStructures)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-016")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-097")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp016TheSeenLayerIsTheTerritoryAndTheAreaAroundItsFiguresAndStructures)
  {
    // The seen layer holds the territory and the area within mvp.vision.unit_radius of the settlement's figures; a
    // Worker sent 900 wu out sees around itself from the next step; and the layer is made once in every step.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    Assert::IsNull(region.SeenLayerOf(SETTLED_SITE), L"none before the first step");
    std::uint64_t tick = 0;
    world->Step(tick++, pool);
    const Species::Settlement& settlement = region.Settlements().front();
    const Neuron::Vec3 wellspring = region.Structures().At(settlement.wellspringSlot).position;
    const Species::CellRaster& seen = *region.SeenLayerOf(SETTLED_SITE);
    Assert::IsTrue(seen.Holds(wellspring.x, wellspring.z), L"its Wellspring");
    const Species::CellRaster& territory = *region.TerritoryOf(SETTLED_SITE);
    for (std::int32_t z = 0; z < seen.CellsPerAxis(); ++z)
    {
      for (std::int32_t x = 0; x < seen.CellsPerAxis(); ++x)
      {
        const Neuron::Vec3 center = seen.CellCenter(x, z);
        Assert::IsTrue(!territory.Holds(center.x, center.z) || seen.HoldsCell(x, z), L"its territory");
      }
    }
    const Neuron::Vec3 out = wellspring + Neuron::Vec3{0.0, 0.0, -900.0};
    Assert::IsFalse(seen.Holds(out.x, out.z), L"not 900 wu out");
    const auto& figures = region.TeamAt(SETTLED_SITE).figures;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      if (figures.Used(slot))
      {
        Species::Figure& scout = region.TeamAt(SETTLED_SITE).figures.At(slot);
        scout.position = {out.x, region.Terrain()->field.Height(out.x, out.z), out.z};
        break;
      }
    }
    world->Step(tick++, pool);
    Assert::IsTrue(region.SeenLayerOf(SETTLED_SITE)->Holds(out.x, out.z), L"around the Worker sent out");
    for (int step = 0; step < 8; ++step)
    {
      world->Step(tick++, pool);
    }
    ExpectEqual(static_cast<std::int64_t>(tick), static_cast<std::int64_t>(region.SeenLayerUpdates()), L"once in every step");
  }
};

} // namespace GameLogicTests
