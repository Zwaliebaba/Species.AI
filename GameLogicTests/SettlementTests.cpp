#include "pch.h"

#include "CommandJournal.h"
#include "Settlement.h"
#include "StateHash.h"
#include "Structure.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"

#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t STEWARD_SITE = 2;

const Species::StructureRecords& Records()
{
  return TestRecords();
}

} // namespace

TEST_CLASS(SettlementTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp022ThePopulationLimitCountsActiveDwellingsUpToItsMaximum)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-022")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp022ThePopulationLimitCountsActiveDwellingsUpToItsMaximum)
  {
    // 40 and 30 for each active dwelling, never above 300: the ninth dwelling's 310 is held at the maximum.
    const Species::TuningMvp& tuning = CommittedTuning();
    ExpectEqual(40, Species::PopulationLimit(tuning, 0), L"the base");
    ExpectEqual(70, Species::PopulationLimit(tuning, 1), L"one dwelling");
    ExpectEqual(280, Species::PopulationLimit(tuning, 8), L"eight");
    ExpectEqual(300, Species::PopulationLimit(tuning, 9), L"the ninth reaches the maximum");
    ExpectEqual(300, Species::PopulationLimit(tuning, 1000), L"and stays there");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp030TheStoresHoldExactThousandthsUpToTheirCapacity)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-030")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-021")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-031")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp030TheStoresHoldExactThousandthsUpToTheirCapacity)
  {
    // The Wellspring holds 1,500 ru and each active storehouse 1,500 more. Fifteen thousand loads of a tenth of a unit
    // fill the Wellspring exactly, where binary fractions would drift; a load that does not fit is refused whole, and
    // the stores pay a cost only whole.
    const Species::TuningMvp& tuning = CommittedTuning();
    ExpectEqual(1'500'000, Species::StoreCapacityMilliRu(tuning, 0), L"the Wellspring's capacity");
    ExpectEqual(4'500'000, Species::StoreCapacityMilliRu(tuning, 2), L"and two storehouses'");
    ExpectEqual(100, Species::MilliRuOf(0.1), L"a tenth of a unit");
    Species::Settlement settlement;
    const std::int64_t capacity = Species::StoreCapacityMilliRu(tuning, 0);
    for (int load = 0; load < 15'000; ++load)
    {
      Assert::IsTrue(Species::Deposit(settlement, Species::MilliRuOf(0.1), capacity), L"each load fits");
    }
    ExpectEqual(capacity, settlement.storesMilliRu, L"exactly full");
    Assert::IsFalse(Species::Deposit(settlement, 1, capacity), L"a full store refuses a load");
    ExpectEqual(capacity, settlement.storesMilliRu, L"unchanged");
    Assert::IsFalse(Species::Withdraw(settlement, capacity + 1), L"a cost above the stores is refused");
    Assert::IsTrue(Species::Withdraw(settlement, Species::MilliRuOf(tuning.mvp.cost.dwellingRu)), L"a dwelling's cost is paid");
    ExpectEqual(1'200'000, settlement.storesMilliRu, L"to the thousandth");
    Assert::IsTrue(Species::Deposit(settlement, 300'000, capacity) && !Species::Deposit(settlement, 1, capacity), L"full again");
    Assert::IsFalse(Species::Deposit(settlement, -1, capacity) || Species::Withdraw(settlement, -1), L"no negative amount");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp020TheStewardsSettlementStartsAtItsSite)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-012")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-013")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-020")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-096")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-099")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp020TheStewardsSettlementStartsAtItsSite)
  {
    // The site the steward picked holds its settlement and the other three are free; the settlement's team and its
    // Cladan's colour are its site, a steward runs it, its fidelity is full, its Cladan has no modifier, its stores are
    // empty, its territory has the founding radius, and its Wellspring stands at the site's centre facing out along the
    // site's outward diagonal, with the ports of its record.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    const Species::RegionSimulation& region = world->Region(0);
    ExpectEqual(1, static_cast<std::int64_t>(region.Settlements().size()), L"one settlement");
    for (std::uint8_t site = 0; site < 4; ++site)
    {
      Assert::IsTrue((region.SettlementAt(site) != nullptr) == (site == STEWARD_SITE), L"at the steward's site only");
    }
    const Species::Settlement& settlement = region.Settlements().front();
    Assert::IsTrue(settlement.team == STEWARD_SITE && settlement.cladan.color == STEWARD_SITE && settlement.stewarded,
                   L"its team, its colour, its steward");
    Assert::IsTrue(settlement.fidelity == Species::Fidelity::Full && settlement.cladan.temperament == Species::Temperament::None &&
                     settlement.cladan.modifiers.empty(),
                   L"full fidelity, no temperament and an empty place for modifiers");
    ExpectEqual(0, settlement.storesMilliRu, L"empty stores");
    Assert::IsTrue(settlement.territoryRadius == 500.0, L"the founding territory");

    const Species::Structure& wellspring = region.Structures().At(settlement.wellspringSlot);
    const Neuron::Vec3& center = ValleyTerrain()->layout.sites[STEWARD_SITE];
    const Neuron::Vec3& valley = ValleyTerrain()->layout.valleyCenter;
    Assert::IsTrue(wellspring.id == settlement.wellspringId && wellspring.kind == Species::StructureKind::Wellspring &&
                     wellspring.owner == std::optional<std::uint8_t>(STEWARD_SITE),
                   L"its Wellspring");
    Assert::IsTrue(wellspring.position.x == center.x && wellspring.position.z == center.z &&
                     wellspring.position.y == ValleyTerrain()->field.Height(center.x, center.z),
                   L"at the site's centre, on the terrain");
    const double outward = std::atan2(center.z - valley.z, center.x - valley.x);
    Assert::IsTrue(std::abs(std::atan2(wellspring.front.z, wellspring.front.x) - outward) < 1e-12, L"facing out along the diagonal");
    ExpectEqual(5, static_cast<std::int64_t>(wellspring.ports.size()), L"its record's ports");
    Assert::IsTrue(wellspring.radius == 15.0 && std::abs(wellspring.center.y - wellspring.position.y - 5.0) < 1e-12,
                   L"its bounding sphere at its transform");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp021NewWorkersLeaveTheWellspringsDoor)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-020")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-021")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-ECON-043")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp021NewWorkersLeaveTheWellspringsDoor)
  {
    // The 20 starting Workers appear at the door socket, 25 wu along the Wellspring's front, with no position spread,
    // airborne, each with the velocity (front + j) x 10 wu/s, j in [-0.5, 0.5) on each axis, and no two alike.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    const Species::RegionSimulation& region = world->Region(0);
    const Species::Settlement& settlement = region.Settlements().front();
    const Species::Structure& wellspring = region.Structures().At(settlement.wellspringSlot);
    const Neuron::Vec3 door = wellspring.position + wellspring.front * 25.0;
    const auto& workers = region.TeamAt(settlement.team).figures;
    ExpectEqual(20, workers.Count(), L"mvp.settlement.start_workers");
    std::vector<Neuron::Vec3> velocities;
    for (std::uint32_t slot = 0; slot < workers.Capacity(); ++slot)
    {
      if (!workers.Used(slot))
      {
        continue;
      }
      const Species::Figure& worker = workers.At(slot);
      Assert::IsTrue(worker.kind == Species::FigureKind::Worker && !worker.onGround, L"an airborne Worker");
      Assert::IsTrue(Neuron::LengthSquared(worker.position - door) < 1e-18, L"at the door");
      const Neuron::Vec3 jitter = worker.velocity * 0.1 - wellspring.front;
      Assert::IsTrue(std::abs(jitter.x) <= 0.5 && std::abs(jitter.y) <= 0.5 && std::abs(jitter.z) <= 0.5, L"jittered within half a unit");
      for (const Neuron::Vec3& other : velocities)
      {
        Assert::IsTrue(Neuron::LengthSquared(other - worker.velocity) > 0.0, L"each Worker its own draw");
      }
      velocities.push_back(worker.velocity);
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Wld080TheWorldHoldsItsResourceSitesWithTheirGatheringPorts)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-080")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"WLD-084")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-030")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Wld080TheWorldHoldsItsResourceSitesWithTheirGatheringPorts)
  {
    // Every resource site of the terrain is a structure of no settlement at its place, in the terrain's order: a home or
    // border site with 4 gathering ports, the central one with 12 (mvp.resource.ports_per_unit for each unit).
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    const Species::RegionSimulation& region = world->Region(0);
    const std::vector<Species::ResourceSitePlace>& places = ValleyTerrain()->resourceSites;
    std::size_t found = 0;
    for (std::uint32_t slot = 0; slot < region.Structures().Capacity(); ++slot)
    {
      if (!region.Structures().Used(slot) || region.Structures().At(slot).kind == Species::StructureKind::Wellspring)
      {
        continue;
      }
      const Species::Structure& site = region.Structures().At(slot);
      Assert::IsTrue(site.resourceSite == std::optional<std::uint32_t>(static_cast<std::uint32_t>(found)) && !site.owner,
                     L"the terrain's next resource site, of no settlement");
      const Species::ResourceSitePlace& place = places[found];
      Assert::IsTrue(site.position.x == place.position.x && site.position.z == place.position.z, L"at its place");
      const std::uint32_t ports = place.units * static_cast<std::uint32_t>(CommittedTuning().mvp.resource.portsPerUnit);
      ExpectEqual(ports, static_cast<std::int64_t>(site.ports.size()), L"four gathering ports per unit");
      ++found;
    }
    ExpectEqual(static_cast<std::int64_t>(places.size()), static_cast<std::int64_t>(found), L"every resource site");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBldg020APortInsideItsStructureIsPushedClear)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-020")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-014")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBldg020APortInsideItsStructureIsPushedClear)
  {
    // A port socket 10 wu out on the mesh's +x, inside a bounding sphere of 15 wu, moves away from the structure's
    // position in steps of 2 wu until its 5 wu sphere no longer hits: 10 wu further, on the structure's right. A port
    // socket outside stays where its socket is. Both stand on the terrain.
    Neuron::SimulationRecord record = Records().resourceSite;
    record.bounds = Neuron::BoundingSphere{{0.0F, 0.0F, 0.0F}, 15.0F};
    record.sockets.resize(2);
    record.sockets[0].transform[12] = 10.0F;
    record.sockets[0].transform[14] = 0.0F;
    record.sockets[1].transform[12] = 0.0F;
    record.sockets[1].transform[14] = 30.0F;
    const std::shared_ptr<const Species::RegionTerrain> flat = FlatTerrain(50.0);
    const double origin = static_cast<double>(CommittedTuning().mvp.world.regionCell) * CommittedTuning().wld.region.side;
    const double x = origin + 1000.0;
    const double z = origin + 1000.0;
    const Species::Structure structure = Species::MakeStructure(Species::StructureKind::ResourceSite, std::nullopt, std::nullopt, x, z,
                                                                {0.0, 0.0, 1.0}, record, nullptr, flat->field, CommittedTuning());
    ExpectEqual(2, static_cast<std::int64_t>(structure.ports.size()), L"both ports");
    const Neuron::Vec3& pushed = structure.ports[0].position;
    Assert::IsTrue(std::abs(pushed.x - (x + 20.0)) < 1e-9 && std::abs(pushed.z - z) < 1e-9 && pushed.y == 50.0, L"pushed to 20 wu");
    const Neuron::Vec3& clear = structure.ports[1].position;
    Assert::IsTrue(std::abs(clear.x - x) < 1e-9 && std::abs(clear.z - (z + 30.0)) < 1e-9 && clear.y == 50.0, L"left where it is");
    Assert::IsTrue(Neuron::LengthSquared(structure.ports[0].front - Neuron::Vec3{0.0, 0.0, 1.0}) < 1e-18, L"facing the socket's front");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095ASettledWorldIsRestoredAsItWasSaved)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-147")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095ASettledWorldIsRestoredAsItWasSaved)
  {
    // The hash covers the settlement and the structures: a world restored from its snapshot hashes as the world saved,
    // and both step to the same hashes; a world founded at another site does not hash the same.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    for (std::uint64_t tick = 0; tick < 20; ++tick)
    {
      world->Step(tick, pool);
    }
    auto restored = Species::RestoreWorld(Species::SnapshotOf(*world), CommittedTuning(), &Records());
    Assert::IsTrue(restored.has_value(), L"the snapshot restores");
    std::unique_ptr<Species::WorldSimulation> back = std::move(restored.value());
    back->Region(0).AttachTerrain(ValleyTerrain());
    Assert::IsTrue(Species::HashWorld(*back).world == Species::HashWorld(*world).world, L"the same hash");
    ExpectEqual(1, static_cast<std::int64_t>(back->Region(0).Settlements().size()), L"its settlement");
    for (std::uint64_t tick = 20; tick < 40; ++tick)
    {
      world->Step(tick, pool);
      back->Step(tick, pool);
      Assert::IsTrue(Species::HashWorld(*back).world == Species::HashWorld(*world).world, L"the same hash on every tick");
    }

    const std::unique_ptr<Species::WorldSimulation> elsewhere = SettledWorld(1);
    Assert::IsTrue(Species::HashWorld(*elsewhere).world != Species::HashWorld(*SettledWorld()).world, L"another site, another hash");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094TheStewardCommandsTheSettlementItRuns)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-007")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094TheStewardCommandsTheSettlementItRuns)
  {
    // A steward who connects to the world commands the team of the settlement a steward runs, and no other.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    const Species::StableId steward = Species::ConnectSteward(*world, false);
    const Species::RegionSimulation& region = world->Region(0);
    Assert::IsTrue(region.CommanderOf(STEWARD_SITE).sender == steward && !region.CommanderOf(STEWARD_SITE).ai, L"its settlement's team");
    Assert::IsTrue(region.TeamCommandedBy(steward) == std::optional<std::uint8_t>(STEWARD_SITE), L"and only that one");
  }
};

} // namespace GameLogicTests
