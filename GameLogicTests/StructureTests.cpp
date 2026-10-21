#include "pch.h"

#include "BotNavigation.h"
#include "ObstacleGrid.h"
#include "StructureAdvance.h"
#include "TestSupport.h"
#include "Weapons.h"
#include "WorkerPool.h"
#include "WorldSimulation.h"

#include <algorithm>
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
  return 0;
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
      bool near = false;
      for (std::uint32_t slot = 0; slot < region.Structures().Capacity(); ++slot)
      {
        if (!region.Structures().Used(slot))
        {
          continue;
        }
        const Species::Structure& structure = region.Structures().At(slot);
        near = near || (!structure.ports.empty() && Neuron::Length(structure.position - region.Bots().Position(node)) < 100.0);
      }
      Assert::IsTrue(Species::Important(region, node) == near, L"important exactly when near");
      important += near ? 1 : 0;
    }
    Assert::IsTrue(important > 0 && important < region.Bots().Size(), L"some nodes and not all");
  }
};

} // namespace GameLogicTests
