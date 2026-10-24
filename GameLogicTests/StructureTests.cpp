#include "pch.h"

#include "BotNavigation.h"
#include "CollisionAsset.h"
#include "CommandJournal.h"
#include "Construction.h"
#include "Movement.h"
#include "ObstacleGrid.h"
#include "SimulationRecord.h"
#include "StructureAdvance.h"
#include "TestSupport.h"
#include "Weapons.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <cstdint>
#include <memory>
#include <optional>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_TEAM = 2; // the settlement of SettledWorld, whose team is its site

// The slot of the first structure of _kind, in slot order.
std::uint32_t FirstOf(const Species::RegionSimulation& _region, Species::StructureKind _kind)
{
  const auto& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (structures.Used(slot) && structures.At(slot).kind == _kind)
    {
      return slot;
    }
  }
  Assert::Fail(L"the region holds a structure of the kind");
}

// The settlement's Workers' slots, in slot order.
std::vector<std::uint32_t> WorkersOf(const Species::RegionSimulation& _region)
{
  std::vector<std::uint32_t> slots;
  const auto& figures = _region.TeamAt(SETTLED_TEAM).figures;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (figures.Used(slot))
    {
      slots.push_back(slot);
    }
  }
  return slots;
}

// The point _distance from the settlement's Wellspring at _degrees from +x.
Neuron::Vec3 AroundWellspring(const Species::RegionSimulation& _region, double _distance, double _degrees)
{
  const Species::Settlement& settlement = _region.Settlements().front();
  const Neuron::Vec3& center = _region.Structures().At(settlement.wellspringSlot).position;
  const double radians = _degrees * std::numbers::pi / 180.0;
  return {center.x + _distance * std::cos(radians), center.y, center.z + _distance * std::sin(radians)};
}

// A collision asset of one node holding the box from _low to _high in 12 triangles, as the mesh kit's boxes are.
Neuron::CollisionAsset BoxCollision(const std::array<float, 3>& _low, const std::array<float, 3>& _high)
{
  const auto corner = [&](int _bits)
  {
    return std::array<float, 3>{(_bits & 1) != 0 ? _high[0] : _low[0], (_bits & 2) != 0 ? _high[1] : _low[1],
                                (_bits & 4) != 0 ? _high[2] : _low[2]};
  };
  // Each face's four corners, numbered by the bits of x, y and z set high, one face per axis and side.
  constexpr std::array<std::array<int, 4>, 6> FACES{{{0, 2, 6, 4}, {1, 3, 7, 5}, {0, 1, 5, 4}, {2, 3, 7, 6}, {0, 1, 3, 2}, {4, 5, 7, 6}}};
  Neuron::CollisionNode node;
  node.name = "body";
  for (const std::array<int, 4>& face : FACES)
  {
    for (const int index : {face[0], face[1], face[2], face[0], face[2], face[3]})
    {
      node.corners.push_back(corner(index));
    }
  }
  Neuron::CollisionAsset asset;
  asset.nodes.push_back(node);
  return asset;
}

// The Worker in _slot moved on to the ground at _position, still, as if it had walked there.
void StandAt(Species::RegionSimulation& _region, std::uint32_t _slot, const Neuron::Vec3& _position)
{
  Species::Figure& worker = _region.TeamAt(SETTLED_TEAM).figures.At(_slot);
  worker.position = {_position.x, ValleyTerrain()->field.Height(_position.x, _position.z), _position.z};
  worker.velocity = {};
  worker.onGround = true;
  worker.waypoint = worker.position;
}

} // namespace

TEST_CLASS(StructureTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim123AStructuresTestsReadItsCollisionTriangles)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-123")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-041")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim123AStructuresTestsReadItsCollisionTriangles)
  {
    // One triangle in the plane y = 0 inside a bounding sphere of 10 wu: a sphere hits it only when it comes strictly
    // within its radius of the triangle, and a segment only when it crosses it; a shape with no triangle is its sphere.
    Species::ObstacleShape shape{{3.0, 0.0, 3.0}, 10.0, {{{{0.0, 0.0, 0.0}, {10.0, 0.0, 0.0}, {0.0, 0.0, 10.0}}}}, std::nullopt};
    Assert::IsTrue(Species::SphereHits(shape, {2.0, 1.0, 2.0}, 1.5), L"1 wu above the face");
    Assert::IsFalse(Species::SphereHits(shape, {2.0, 3.0, 2.0}, 1.5), L"inside the sphere, 3 wu from the face");
    Assert::IsFalse(Species::SphereHits(shape, {2.0, 1.0, 2.0}, 1.0), L"strictly within the radius");
    Assert::IsFalse(Species::SphereHits(shape, {30.0, 0.0, 30.0}, 5.0), L"outside the bounding sphere");
    Assert::IsTrue(Species::RayHits(shape, {2.0, 5.0, 2.0}, {0.0, -1.0, 0.0}, 10.0), L"a segment through the face");
    Assert::IsFalse(Species::RayHits(shape, {2.0, 5.0, 2.0}, {0.0, -1.0, 0.0}, 4.0), L"one that stops short of it");
    Assert::IsFalse(Species::RayHits(shape, {6.0, 5.0, 6.0}, {0.0, -1.0, 0.0}, 10.0), L"one beside the triangle");
    const Species::ObstacleShape sphere{{3.0, 0.0, 3.0}, 10.0, {}, std::nullopt};
    Assert::IsTrue(Species::SphereHits(sphere, {2.0, 3.0, 2.0}, 1.5) && Species::RayHits(sphere, {6.0, 5.0, 6.0}, {0.0, -1.0, 0.0}, 10.0),
                   L"the sphere's own tests");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBldg021AWorkerAtAPortIsItsOccupantAndAnotherIsTurnedAway)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-021")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-022")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-023")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-069")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-070")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBldg021AWorkerAtAPortIsItsOccupantAndAnotherIsTurnedAway)
  {
    // A Worker sent to a resource site's port operates it for its team while it walks, mans it on arrival and is its
    // occupant from that tick's structure step, which clears the port's counts; a second Worker sent to the occupied port
    // becomes Idle. When the first stops manning, the port has no occupant at the next evaluation.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    std::uint64_t tick = 0;
    for (; tick < 60; ++tick)
    {
      world->Step(tick, pool); // the first Workers land
    }
    const std::uint32_t siteSlot = FirstOf(region, Species::StructureKind::ResourceSite);
    const std::vector<std::uint32_t> workers = WorkersOf(region);
    const Neuron::Vec3 port = region.Structures().At(siteSlot).ports[0].position;
    StandAt(region, workers[0], port + Neuron::Vec3{8.0, 0.0, 0.0});
    StandAt(region, workers[1], port + Neuron::Vec3{0.0, 0.0, 8.0});
    Species::Figure& first = region.TeamAt(SETTLED_TEAM).figures.At(workers[0]);
    Species::SendToPort(first, region.Structures().At(siteSlot), siteSlot, 0);
    world->Step(tick++, pool);
    const Species::StructurePort& evaluated = region.Structures().At(siteSlot).ports[0];
    Assert::IsTrue(first.workerState == Species::WorkerState::ToPort && !evaluated.occupant, L"walking to it, not yet there");
    // R-BLDG-025: its one reservation a tick is drained by the 4 of the structure step that follows, so a reservation
    // turns a second Worker away only within the tick.
    Assert::IsTrue(evaluated.reservations.size() > SETTLED_TEAM && evaluated.reservations[SETTLED_TEAM] == 0, L"drained by the tick's end");
    for (int step = 0; step < 40 && first.workerState != Species::WorkerState::Manning; ++step)
    {
      world->Step(tick++, pool);
    }
    Assert::IsTrue(first.workerState == Species::WorkerState::Manning && first.velocity == Neuron::Vec3{}, L"manning, still");
    Assert::IsTrue(evaluated.occupant.value_or(Species::ActorRef{}).id == first.id && evaluated.reservations.empty(), L"its occupant");

    Species::Figure& second = region.TeamAt(SETTLED_TEAM).figures.At(workers[1]);
    Species::SendToPort(second, region.Structures().At(siteSlot), siteSlot, 0);
    world->Step(tick++, pool);
    Assert::IsTrue(second.workerState == Species::WorkerState::Idle, L"the second is turned away");
    Assert::IsTrue(first.workerState == Species::WorkerState::Manning, L"the first stays");

    first.workerState = Species::WorkerState::Idle;
    world->Step(tick++, pool);
    Assert::IsFalse(evaluated.occupant.has_value(), L"no occupant once it stops manning");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBldg022AnUnoccupiedPortsReservationsDecay)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-022")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-023")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBldg022AnUnoccupiedPortsReservationsDecay)
  {
    // Ten operations of team 3 count 10 and leave the port unavailable to team 3 and available to team 1; the counts of an
    // unoccupied port fall by 4 a tick and stop at 0; the operations stop at the cap of 50.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    const std::uint32_t siteSlot = FirstOf(region, Species::StructureKind::ResourceSiteCentral);
    Species::StructurePort& port = region.Structures().At(siteSlot).ports[5];
    for (int operation = 0; operation < 10; ++operation)
    {
      Species::OperatePort(port, 3, CommittedTuning());
    }
    ExpectEqual(10, port.reservations[3], L"ten reservations");
    Assert::IsTrue(!Species::PortAvailable(port, 3, CommittedTuning()) && Species::PortAvailable(port, 1, CommittedTuning()),
                   L"unavailable to its team only");
    std::uint64_t tick = 0;
    for (const int expected : {6, 2, 0, 0})
    {
      world->Step(tick++, pool);
      ExpectEqual(expected, region.Structures().At(siteSlot).ports[5].reservations[3], L"four fewer a tick, never below 0");
    }
    Species::StructurePort& capped = region.Structures().At(siteSlot).ports[6];
    for (int operation = 0; operation < 80; ++operation)
    {
      Species::OperatePort(capped, 0, CommittedTuning());
    }
    ExpectEqual(50, capped.reservations[0], L"ref.bldg.port.reservation_cap");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCrate441ABoltIsStoppedByAStructure)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CRATE-441")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCrate441ABoltIsStoppedByAStructure)
  {
    // A bolt fired at the Wellspring from behind it ends where its step meets the structure, with the structure-hit event,
    // and deals the structure no damage, since it has no health.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    const Species::Structure& wellspring = region.Structures().At(FirstOf(region, Species::StructureKind::Wellspring));
    const Neuron::Vec3 from = wellspring.center - wellspring.front * 80.0;
    Species::FireLaser(region, 1, from, wellspring.front * 150.0);
    bool stopped = false;
    for (std::uint64_t tick = 0; tick < 20 && !stopped; ++tick)
    {
      world->Step(tick, pool);
      stopped = std::ranges::any_of(region.Events(), [](const Species::EffectEvent& _event)
                                    { return _event.kind == Species::EffectEventKind::LaserHitStructure; });
    }
    Assert::IsTrue(stopped, L"the structure-hit event");
    ExpectEqual(0, region.LaserBolts().Count(), L"the bolt is removed");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp032EveryPlacementRefusalHasItsReason)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-032")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-034")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp032EveryPlacementRefusalHasItsReason)
  {
    // A placement is refused at a site with no settlement and for a kind the steward does not place; outside the
    // territory of 500 wu; inside the centre ring of 60 wu; on a footprint that meets a resource site's or a queued
    // site's; when the stores do not hold the cost; and when 8 sites are queued. A placement that passes pays 300 ru for
    // the first dwelling, 330 for the second (× 1.10) and 200 for a storehouse, and queues a construction site.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    using Species::PlacementRefusal;
    using Species::StructureKind;
    const auto refusal = [&](std::uint8_t _site, StructureKind _kind, const Neuron::Vec3& _at)
    {
      const auto placed = Species::PlaceStructure(region, _site, _kind, _at.x, _at.z);
      return placed ? std::optional<PlacementRefusal>() : std::optional<PlacementRefusal>(placed.error());
    };
    const Neuron::Vec3 good = AroundWellspring(region, 150.0, 0.0);
    Assert::IsTrue(refusal(0, StructureKind::Dwelling, good) == PlacementRefusal::NotBuildable, L"no settlement at site 0");
    Assert::IsTrue(refusal(2, StructureKind::Wellspring, good) == PlacementRefusal::NotBuildable, L"not a placeable kind");
    Assert::IsTrue(refusal(2, StructureKind::Dwelling, AroundWellspring(region, 600.0, 0.0)) == PlacementRefusal::OutsideTerritory,
                   L"outside the territory");
    Assert::IsTrue(refusal(2, StructureKind::Dwelling, AroundWellspring(region, 50.0, 0.0)) == PlacementRefusal::InCenterRing,
                   L"in the centre ring");
    // An inner home resource site of the settlement's own, 350 wu from its Wellspring.
    std::optional<Neuron::Vec3> inner;
    for (std::uint32_t slot = 0; slot < region.Structures().Capacity() && !inner; ++slot)
    {
      const Species::Structure& structure = region.Structures().At(slot);
      if (region.Structures().Used(slot) && structure.kind == StructureKind::ResourceSite &&
          Neuron::HorizontalDistance(structure.position, AroundWellspring(region, 0.0, 0.0)) < 400.0)
      {
        inner = structure.position;
      }
    }
    Assert::IsTrue(inner.has_value(), L"an inner resource site");
    Assert::IsTrue(refusal(2, StructureKind::Dwelling, inner.value_or(good)) == PlacementRefusal::Overlaps, L"on a resource site");
    Assert::IsTrue(refusal(2, StructureKind::Dwelling, good) == PlacementRefusal::CannotAfford, L"empty stores");
    ExpectEqual(0, settlement.storesMilliRu, L"nothing paid");

    settlement.storesMilliRu = 10'000'000;
    const auto first = Species::PlaceStructure(region, 2, StructureKind::Dwelling, good.x, good.z);
    Assert::IsTrue(first.has_value(), L"the first dwelling is placed");
    ExpectEqual(10'000'000 - 300'000, settlement.storesMilliRu, L"for 300 ru");
    const Species::Structure& queued = region.Structures().At(first.value_or(0));
    Assert::IsTrue(!queued.built && queued.workTicks == 0 && queued.costMilliRu == 300'000, L"a construction site");
    Assert::IsTrue(settlement.queue.size() == 1 && settlement.queue.front().id == queued.id, L"queued");
    Assert::IsTrue(refusal(2, StructureKind::Dwelling, good) == PlacementRefusal::Overlaps, L"on a queued site");
    const Neuron::Vec3 second = AroundWellspring(region, 150.0, 45.0);
    Assert::IsTrue(Species::PlaceStructure(region, 2, StructureKind::Dwelling, second.x, second.z).has_value(), L"the second");
    ExpectEqual(10'000'000 - 300'000 - 330'000, settlement.storesMilliRu, L"for 330 ru");
    const Neuron::Vec3 store = AroundWellspring(region, 150.0, 90.0);
    Assert::IsTrue(Species::PlaceStructure(region, 2, StructureKind::Storehouse, store.x, store.z).has_value(), L"a storehouse");
    ExpectEqual(10'000'000 - 830'000, settlement.storesMilliRu, L"for 200 ru");
    for (const double degrees : {135.0, 180.0, 225.0, 270.0, 315.0})
    {
      const Neuron::Vec3 at = AroundWellspring(region, 150.0, degrees);
      Assert::IsTrue(Species::PlaceStructure(region, 2, StructureKind::Storehouse, at.x, at.z).has_value(), L"up to 8 queued");
    }
    Assert::IsTrue(refusal(2, StructureKind::Storehouse, AroundWellspring(region, 260.0, 0.0)) == PlacementRefusal::QueueFull,
                   L"the ninth is refused");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp032ASiteOnUnwalkableGroundIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-032")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp032ASiteOnUnwalkableGroundIsRefused)
  {
    // With the walkable slope tightened past any ground, every place is refused as not walkable, before the stores.
    Species::TuningMvp steep = CommittedTuning();
    steep.ref.sim.walk.minNormalY = 1.01;
    Species::WorldStart start;
    start.key = SETTLED_TEST_KEY;
    start.seed = ValleyTerrain()->seed;
    start.stewardSite = std::uint8_t{2};
    const std::unique_ptr<Species::WorldSimulation> world = Species::StartWorld(start, ValleyTerrain(), steep, &TestRecords());
    Species::RegionSimulation& region = world->Region(0);
    const Neuron::Vec3 at = AroundWellspring(region, 150.0, 0.0);
    const auto placed = Species::PlaceStructure(region, 2, Species::StructureKind::Dwelling, at.x, at.z);
    Assert::IsTrue(!placed.has_value() && placed.error() == Species::PlacementRefusal::NotWalkable, L"not walkable");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp033ConstructionProgressesByWorkerSeconds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-033")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-022")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-034")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp033ConstructionProgressesByWorkerSeconds)
  {
    // A dwelling of 300 ru needs 300 Worker-seconds, 3,000 Worker-ticks. Six Workers at its six ports give 6 a tick; one
    // gives 1. When it has them it is built, leaves the queue and raises its event, its Workers go Idle, and the
    // population limit rises from 40 to 70. A built dwelling's ports are no longer manned.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    Species::Settlement& settlement = region.Settlements().front();
    std::uint64_t tick = 0;
    for (; tick < 60; ++tick)
    {
      world->Step(tick, pool);
    }
    settlement.storesMilliRu = 1'000'000;
    const Neuron::Vec3 at = AroundWellspring(region, 150.0, 30.0);
    const auto placed = Species::PlaceStructure(region, 2, Species::StructureKind::Dwelling, at.x, at.z);
    Assert::IsTrue(placed.has_value(), L"placed");
    const std::uint32_t slot = placed.value_or(0);
    ExpectEqual(3000, Species::RequiredWorkTicks(region.Structures().At(slot), CommittedTuning()), L"300 Worker-seconds");
    const std::vector<std::uint32_t> workers = WorkersOf(region);
    for (std::uint32_t port = 0; port < 6; ++port)
    {
      const Neuron::Vec3 position = region.Structures().At(slot).ports[port].position;
      const Neuron::Vec3 out = Neuron::NormalizedOr(position - region.Structures().At(slot).position, {1.0, 0.0, 0.0});
      StandAt(region, workers[port], position + out * 6.0);
      Species::SendToPort(region.TeamAt(SETTLED_TEAM).figures.At(workers[port]), region.Structures().At(slot), slot, port);
    }
    for (int step = 0; step < 20; ++step)
    {
      world->Step(tick++, pool);
    }
    const std::int64_t before = region.Structures().At(slot).workTicks;
    Assert::IsTrue(before > 0, L"the work has begun");
    for (int step = 0; step < 10; ++step)
    {
      world->Step(tick++, pool);
    }
    ExpectEqual(before + 60, region.Structures().At(slot).workTicks, L"six Worker-ticks a tick");
    ExpectEqual(40,
                Species::PopulationLimit(CommittedTuning(), Species::ActiveCount(region, SETTLED_TEAM, Species::StructureKind::Dwelling)),
                L"no dwelling yet");

    region.Structures().At(slot).workTicks = 3000 - 6;
    world->Step(tick++, pool);
    const Species::Structure& dwelling = region.Structures().At(slot);
    Assert::IsTrue(dwelling.built && settlement.queue.empty(), L"built and out of the queue");
    Assert::IsTrue(
      std::ranges::any_of(region.Events(), [&](const Species::EffectEvent& _event)
                          { return _event.kind == Species::EffectEventKind::StructureCompleted && _event.subject == dwelling.id; }),
      L"its event");
    ExpectEqual(70,
                Species::PopulationLimit(CommittedTuning(), Species::ActiveCount(region, SETTLED_TEAM, Species::StructureKind::Dwelling)),
                L"the limit rises");
    world->Step(tick++, pool);
    world->Step(tick++, pool);
    for (std::uint32_t port = 0; port < 6; ++port)
    {
      Assert::IsTrue(region.TeamAt(SETTLED_TEAM).figures.At(workers[port]).workerState == Species::WorkerState::Idle, L"its Workers go");
      Assert::IsFalse(dwelling.ports[port].occupant.has_value(), L"and its ports are not manned");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp034AStorehouseBuiltAddsToTheStoresCapacity)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-034")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-021")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp034AStorehouseBuiltAddsToTheStoresCapacity)
  {
    // A queued storehouse adds nothing; built, it adds 1,500 ru to the Wellspring's 1,500.
    Neuron::WorkerPool pool(0);
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    region.Settlements().front().storesMilliRu = 500'000;
    const Neuron::Vec3 at = AroundWellspring(region, 200.0, 200.0);
    const auto placed = Species::PlaceStructure(region, 2, Species::StructureKind::Storehouse, at.x, at.z);
    Assert::IsTrue(placed.has_value(), L"placed");
    const auto capacity = [&]
    {
      return Species::StoreCapacityMilliRu(CommittedTuning(),
                                           Species::ActiveCount(region, SETTLED_TEAM, Species::StructureKind::Storehouse));
    };
    ExpectEqual(1'500'000, capacity(), L"queued, nothing more");
    region.Structures().At(placed.value_or(0)).workTicks = 2000;
    world->Step(0, pool);
    ExpectEqual(3'000'000, capacity(), L"built, 1,500 ru more");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp031ADropPointIsPushedClearOfItsStructureAndAWorkerReachesIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-031")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BLDG-020")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-114")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp031ADropPointIsPushedClearOfItsStructureAndAWorkerReachesIt)
  {
    // The storehouse of the owner's meshes of 2026-10-23 on level ground: its entrance socket stands 1 wu in front of the
    // collision box of its loading platform, inside a Worker's structure clearance, so a Worker sent to the socket is
    // pushed back on every step and never arrives. Its drop point is pushed out by whole steps until a Worker within the
    // snap distance of it is clear of the box, and a Worker sent there from in front arrives (ADR-183).
    const Species::TuningMvp& tuning = CommittedTuning();
    const auto& walker = tuning.ref.unit.worker;
    const std::shared_ptr<const Species::RegionTerrain> terrain = FlatTerrain(50.0);
    constexpr double X = 33000.0;
    constexpr double Z = 33000.0;
    constexpr std::array<float, 16> IDENTITY{1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F,
                                             0.0F, 0.0F, 1.0F, 0.0F, 0.0F, 0.0F, 0.0F, 1.0F};
    Neuron::SimulationRecord record;
    record.nodes.push_back(Neuron::SimulationNode{"body", std::nullopt, IDENTITY});
    Neuron::SimulationSocket entrance{Neuron::SocketKind::Entrance, 0, 0, IDENTITY};
    entrance.transform[14] = 16.0F;
    record.sockets.push_back(entrance);
    record.bounds = Neuron::BoundingSphere{{0.0F, 7.0F, 2.0F}, 20.0F};
    const Neuron::CollisionAsset collision = BoxCollision({-7.0F, 0.0F, 10.1F}, {7.0F, 4.8F, 15.0F});
    const Species::Structure storehouse = Species::MakeStructure(Species::StructureKind::Storehouse, SETTLED_TEAM, std::nullopt, X, Z,
                                                                 {0.0, 0.0, 1.0}, record, &collision, terrain->field, tuning);
    const Species::ObstacleShape shape = Species::ShapeOf(storehouse, &collision);
    const Neuron::Vec3 socket{X, 50.0, Z + 16.0};
    Assert::IsTrue(Species::SphereHits(shape, socket, walker.structureClearRadius), L"the socket is within a Worker's clearance");
    const Neuron::Vec3 drop = Species::DropPointOf(storehouse, record, shape, terrain->field, tuning);
    Assert::IsTrue(drop.x == X && drop.y == 50.0 && drop.z > socket.z && drop.z <= socket.z + 4.0, L"pushed out ahead, on the terrain");
    Assert::IsFalse(Species::SphereHits(shape, drop, walker.structureClearRadius + walker.snapDistance), L"clear of the box");

    Species::RegionSimulation region(0, SETTLED_TEST_KEY, 0, tuning, terrain);
    static_cast<void>(region.Obstacles().Add(shape, [](double, double) { return 50.0; }));
    const auto arrives = [&](const Neuron::Vec3& _target)
    {
      Species::Figure worker;
      worker.id = 7;
      worker.kind = Species::FigureKind::Worker;
      worker.onGround = true;
      worker.speed = walker.speed;
      worker.position = {X, 50.0, Z + 48.0};
      worker.front = {0.0, 0.0, -1.0};
      worker.waypoint = _target;
      for (int tick = 0; tick < 600; ++tick)
      {
        if (Species::WorkerMoveToWaypoint(worker, false, region.Movement()))
        {
          return true;
        }
      }
      return false;
    };
    Assert::IsFalse(arrives(socket), L"sent to the socket, it never arrives");
    Assert::IsTrue(arrives(drop), L"sent to the drop point, it arrives");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim343AStructureChangesNoRouteOfTheMvp)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-343")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-335")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim343AStructureChangesNoRouteOfTheMvp)
  {
    // Only walls and Barrier Emitters block a route edge, and the MVP has neither, so a route across the place of a new
    // dwelling is the same before and after it is placed (ADR-131).
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    region.Settlements().front().storesMilliRu = 500'000;
    const Neuron::Vec3 from = AroundWellspring(region, 300.0, 100.0);
    const Neuron::Vec3 to = AroundWellspring(region, 300.0, 260.0);
    const Species::RouteHandle before = region.BuildRoute(from, to, false);
    const Neuron::Vec3 across = AroundWellspring(region, 120.0, 180.0);
    Assert::IsTrue(Species::PlaceStructure(region, 2, Species::StructureKind::Dwelling, across.x, across.z).has_value(), L"placed");
    const Species::RouteHandle after = region.BuildRoute(from, to, false);
    const std::vector<Neuron::Vec3>* a = region.Routes().Find(before);
    const std::vector<Neuron::Vec3>* b = region.Routes().Find(after);
    Assert::IsTrue(a != nullptr && b != nullptr && *a == *b, L"the same route");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RBot049ANodeNearAStructureWithPortsIsImportant)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-BOT-049")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RBot049ANodeNearAStructureWithPortsIsImportant)
  {
    // A node is important exactly when a structure with a port stands strictly within 100 wu of it; with no structure no
    // node is, as at M1.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    const Species::RegionSimulation& region = world->Region(0);
    std::uint32_t important = 0;
    for (std::uint32_t node = 0; node < region.Bots().Size(); ++node)
    {
      bool close = false;
      for (std::uint32_t slot = 0; slot < region.Structures().Capacity(); ++slot)
      {
        if (!region.Structures().Used(slot))
        {
          continue;
        }
        const Species::Structure& structure = region.Structures().At(slot);
        close = close || (!structure.ports.empty() && Neuron::Length(structure.position - region.Bots().Position(node)) < 100.0);
      }
      Assert::IsTrue(Species::Important(region, node) == close, L"important exactly when near");
      important += close ? 1 : 0;
    }
    Assert::IsTrue(important > 0 && important < region.Bots().Size(), L"some nodes and not all");
  }
};

} // namespace GameLogicTests
