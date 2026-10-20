#include "pch.h"

#include "DrawPurpose.h"
#include "FigureAdvance.h"
#include "LeaderOrders.h"
#include "Movement.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "TestSupport.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <memory>
#include <numbers>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey MOVEMENT_TEST_KEY{0x7e57ab1e7e57ab1e, 0x0123401234012340};
constexpr double REGION_MIN = 30720.0; // mvp.world.region_cell × wld.region.side
constexpr Neuron::Vec3 MIDDLE{33000.0, 50.0, 33000.0};

// A region at tick 0 over a level terrain at _height.
Species::RegionSimulation FlatRegion(double _height = 50.0)
{
  return Species::RegionSimulation(0, MOVEMENT_TEST_KEY, 0, CommittedTuning(), FlatTerrain(_height));
}

Species::Figure& Spawn1(Species::RegionSimulation& _region, Species::FigureKind _kind, const Neuron::Vec3& _position)
{
  Species::RegionSimulation::Spawn spawn;
  spawn.kind = _kind;
  spawn.position = _position;
  const std::uint32_t slot = _region.SpawnFigures(spawn).front();
  return _region.TeamAt(0).figures.At(slot);
}

// Steps the region from its next tick until _done or _limit ticks; the ticks it stepped.
template <class Done> std::uint64_t StepUntil(Species::RegionSimulation& _region, Done&& _done, std::uint64_t _limit)
{
  std::uint64_t stepped = 0;
  while (!_done() && stepped < _limit)
  {
    _region.Step(_region.NextTick());
    ++stepped;
  }
  return stepped;
}

// R-SIM-308's test-build check: every figure is registered over the cells of its position and radius.
void ExpectRegistered(const Species::RegionSimulation& _region)
{
  for (std::size_t team = 0; team < _region.TeamCount(); ++team)
  {
    const auto& figures = _region.TeamAt(team).figures;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      if (!figures.Used(slot))
      {
        continue;
      }
      const Species::Figure& figure = figures.At(slot);
      Assert::IsTrue(figure.cells == _region.Grid().FootprintOf(figure.position.x, figure.position.z, figure.radius), L"recorded cells");
      const std::vector<Species::ActorRef> entries =
        _region.Grid().EntriesAt(static_cast<std::uint8_t>(team), figure.cells.x0, figure.cells.z0);
      Assert::IsTrue(std::ranges::count(entries, figure.id, &Species::ActorRef::id) == 1, L"one entry in its cell");
    }
  }
}

} // namespace

TEST_CLASS(MovementTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit042TheSlopeFactorIsClampedPerKind)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-042")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit042TheSlopeFactorIsClampedPerKind)
  {
    const Species::TuningMvp& tuning = CommittedTuning();
    const Species::FigureKind worker = Species::FigureKind::Worker;
    Assert::IsTrue(Species::SlopeFactor(worker, 50.0, 50.0, tuning) == 1.0, L"level");
    Assert::IsTrue(Species::SlopeFactor(worker, 50.0, 51.5, tuning) == 0.5, L"1 − 1.5 ÷ 3 uphill");
    Assert::IsTrue(Species::SlopeFactor(worker, 50.0, 53.0, tuning) == 0.3, L"a Worker's minimum");
    Assert::IsTrue(Species::SlopeFactor(worker, 56.0, 50.0, tuning) == 2.0, L"the maximum downhill");
    Assert::IsTrue(Species::SlopeFactor(Species::FigureKind::Leader, 50.0, 53.0, tuning) == 0.1, L"a Leader's minimum");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim368TheSpawnSearchAndTheWorldClampKeepFiguresOnTheMap)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-368")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-366")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim368TheSpawnSearchAndTheWorldClampKeepFiguresOnTheMap)
  {
    // AC-SIM-05-18 on the region's square: a search off the map makes 20 draws and returns the point the edge margin
    // inside the corner; one on dry land takes its first attempt; the clamp brings x back to the edge.
    Species::RegionSimulation region = FlatRegion();
    const Species::MovementContext context = region.Movement();
    const Neuron::Vec3 off = Species::SpawnPosition(77, {REGION_MIN - 100.0, 0.0, REGION_MIN - 100.0}, 0.0, context);
    Assert::IsTrue(off == Neuron::Vec3{REGION_MIN + 20.0, 0.0, REGION_MIN + 20.0}, L"clamped 20 wu inside the corner");
    ExpectEqual(20, static_cast<std::int64_t>(context.draws->For(Species::DrawPurpose::SpawnSearch, 77).Ordinal()), L"20 draws");
    const Neuron::Vec3 dry = Species::SpawnPosition(78, MIDDLE, 40.0, context);
    ExpectEqual(2, static_cast<std::int64_t>(context.draws->For(Species::DrawPurpose::SpawnSearch, 78).Ordinal()), L"one attempt");
    Assert::IsTrue(Neuron::HorizontalDistance(dry, MIDDLE) < 40.0, L"within the spread");
    Species::Figure figure;
    figure.position = {REGION_MIN - 5.0, 50.0, MIDDLE.z};
    Species::WorldClamp(figure, context);
    Assert::IsTrue(figure.position.x == REGION_MIN && figure.position.z == MIDDLE.z, L"x clamped to the region's edge");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim382ThePushLoopEndsOutsideTheClearance)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-382")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-380")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-114")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim382ThePushLoopEndsOutsideTheClearance)
  {
    // AC-SIM-05-15: a candidate 1 wu inside a structure ends outside its 2 wu clearance after whole 4 wu steps
    // directly away from its base point; at the base point it ends outside with every component finite. A Worker's
    // own push does the same in its steps, on the terrain.
    Species::RegionSimulation region = FlatRegion();
    const Species::ObstacleShape shape{MIDDLE, 10.0};
    static_cast<void>(region.Obstacles().Add(shape, [](double, double) { return 50.0; }));
    const Species::MovementContext context = region.Movement();
    Species::Figure leader;
    leader.id = 5;
    leader.kind = Species::FigureKind::Leader;
    leader.onGround = true;
    leader.front = {1.0, 0.0, 0.0};
    const Neuron::Vec3 inside{MIDDLE.x + 9.0, 50.0, MIDDLE.z};
    leader.position = inside;
    const Neuron::Vec3 pushed = Species::PushFromObstacles(leader, inside, Species::DrawPurpose::ShoreMove, context);
    Assert::IsTrue(pushed == Neuron::Vec3{MIDDLE.x + 13.0, 50.0, MIDDLE.z}, L"one 4 wu step away, to 13 wu");
    leader.position = MIDDLE;
    const Neuron::Vec3 fromBase = Species::PushFromObstacles(leader, MIDDLE, Species::DrawPurpose::ShoreMove, context);
    Assert::IsTrue(std::isfinite(fromBase.x) && std::isfinite(fromBase.y) && std::isfinite(fromBase.z), L"finite");
    Assert::IsTrue(fromBase == Neuron::Vec3{MIDDLE.x - 12.0, 50.0, MIDDLE.z}, L"three steps along the degenerate direction");
    Species::Figure worker;
    worker.id = 6;
    worker.kind = Species::FigureKind::Worker;
    worker.onGround = true;
    const Neuron::Vec3 workerPushed = Species::WorkerPush(worker, inside, Species::DrawPurpose::ShoreMove, context);
    Assert::IsTrue(workerPushed == Neuron::Vec3{MIDDLE.x + 13.0, 50.0, MIDDLE.z}, L"the Worker's push, re-snapped");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit112AWorkerSearchesTheShoreFrom40Wu)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-112")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit112AWorkerSearchesTheShoreFrom40Wu)
  {
    // A candidate at the shore height draws a = NextSigned(1) and takes the first land sample, here the first one, 40 wu
    // away at the angle 40 × a × π; in a sea with no land it stays.
    Species::RegionSimulation region = FlatRegion();
    Species::Figure worker;
    worker.id = 9;
    worker.kind = Species::FigureKind::Worker;
    const Neuron::Vec3 wet{MIDDLE.x, 0.5, MIDDLE.z};
    const Neuron::Vec3 found = Species::WorkerPush(worker, wet, Species::DrawPurpose::ShoreMove, region.Movement());
    Neuron::SimRng draw(MOVEMENT_TEST_KEY, Species::PurposeOf(Species::DrawPurpose::ShoreMove), 9, 0);
    const double angle = 40.0 * draw.NextSigned(1.0) * std::numbers::pi;
    Assert::IsTrue(found == Neuron::Vec3{MIDDLE.x + std::cos(angle) * 40.0, 0.5, MIDDLE.z + std::sin(angle) * 40.0}, L"the 40 wu sample");
    Species::RegionSimulation sea = FlatRegion(-10.0);
    Assert::IsTrue(Species::WorkerPush(worker, wet, Species::DrawPurpose::ShoreMove, sea.Movement()) == wet, L"no land: it stays");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim370AFallingFigureLandsJustAboveTheTerrain)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-370")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim370AFallingFigureLandsJustAboveTheTerrain)
  {
    // AC-SIM-05-13: a Worker falling onto terrain of height 50 comes to rest at 50.2 with no velocity.
    Species::RegionSimulation region = FlatRegion();
    Species::Figure& worker = Spawn1(region, Species::FigureKind::Worker, {MIDDLE.x, 80.0, MIDDLE.z});
    const std::uint64_t ticks = StepUntil(region, [&] { return worker.onGround; }, 100);
    Assert::IsTrue(ticks > 1 && worker.onGround, L"it fell, then landed");
    Assert::IsTrue(std::abs(worker.position.y - 50.2) < 1e-9 && worker.velocity == Neuron::Vec3{}, L"at rest 0.2 wu above");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit161ALeaderInTheSeaDrownsAtItsFiftyFirstTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-161")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-376")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-080")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit161ALeaderInTheSeaDrownsAtItsFiftyFirstTick)
  {
    // AC-UNIT-L11 and AC-SIM-05-14: a Leader that comes down in the sea at tick t swims at 5 wu/s, two steps a tick, and
    // takes its first drowning damage at tick t + 50; a Worker that comes down in it dies on the tick after.
    Species::RegionSimulation region = FlatRegion(-10.0);
    Species::Figure& leader = Spawn1(region, Species::FigureKind::Leader, {MIDDLE.x, 5.0, MIDDLE.z});
    static_cast<void>(StepUntil(region, [&] { return leader.inWaterTicks >= 0; }, 100));
    ExpectEqual(2, leader.inWaterTicks, L"tick t: it came down and swam twice");
    for (int tick = 1; tick < 50; ++tick)
    {
      region.Step(region.NextTick());
      Assert::IsTrue(std::abs(Neuron::Length(leader.velocity) - 5.0) < 1e-12, L"the swim speed");
    }
    Assert::IsFalse(leader.dead, L"alive at t + 49");
    region.Step(region.NextTick());
    Assert::IsTrue(leader.dead, L"500 damage at t + 50");

    Species::RegionSimulation other = FlatRegion(-10.0);
    Species::Figure& worker = Spawn1(other, Species::FigureKind::Worker, {MIDDLE.x, 5.0, MIDDLE.z});
    static_cast<void>(StepUntil(other, [&] { return worker.workerState == Species::WorkerState::Drowning; }, 100));
    Assert::IsFalse(worker.dead, L"Drowning, not yet dead");
    other.Step(other.NextTick());
    Assert::IsTrue(worker.dead, L"dead on the next tick");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit105AWorkerWalksToItsWaypointAndArrives)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-105")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-111")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-073")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit105AWorkerWalksToItsWaypointAndArrives)
  {
    // A waypoint order puts the waypoint within the order disc of the target, on the terrain; the Worker walks at most
    // its speed × dt a tick, slows near it, snaps the last metre and reports arrival on the tick after, then is Idle.
    Species::RegionSimulation region = FlatRegion();
    Species::Figure& worker = Spawn1(region, Species::FigureKind::Worker, MIDDLE);
    region.Step(region.NextTick());
    Assert::IsTrue(worker.onGround, L"landed");
    const Neuron::Vec3 target{MIDDLE.x + 100.0, 50.0, MIDDLE.z + 20.0};
    Species::GiveWaypointOrder(worker, target, 0, region.Movement());
    Assert::IsTrue(worker.workerState == Species::WorkerState::OnWaypointOrder && worker.orderSet, L"the order");
    Assert::IsTrue(Neuron::HorizontalDistance(worker.waypoint, target) < 20.0 && worker.waypoint.y == 50.0, L"in the disc, on the terrain");
    double longest = 0.0;
    const std::uint64_t ticks = StepUntil(
      region,
      [&]
      {
        longest = std::max(longest, Neuron::Length(worker.velocity) * 0.1);
        return worker.workerState == Species::WorkerState::Idle;
      },
      300);
    Assert::IsTrue(worker.workerState == Species::WorkerState::Idle && ticks > 50, L"arrived after walking 100 wu");
    Assert::IsTrue(longest <= 2.0 + 1e-9, L"at most 20 wu/s × 0.1 s a tick");
    Assert::IsTrue(Neuron::HorizontalDistance(worker.position, worker.waypoint) == 0.0 && !worker.orderSet, L"on its waypoint");
    ExpectRegistered(region);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit157ALeaderWalksToItsWaypoint)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-157")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-158")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit157ALeaderWalksToItsWaypoint)
  {
    Species::RegionSimulation region = FlatRegion();
    Species::Figure& leader = Spawn1(region, Species::FigureKind::Leader, MIDDLE);
    region.Step(region.NextTick());
    const Neuron::Vec3 target{MIDDLE.x - 60.0, 50.0, MIDDLE.z + 80.0};
    Species::AssignWaypoint(leader, target);
    static_cast<void>(StepUntil(region, [&] { return leader.leaderState == Species::LeaderState::Idle; }, 300));
    Assert::IsTrue(leader.leaderState == Species::LeaderState::Idle, L"arrived");
    Assert::IsTrue(Neuron::HorizontalDistance(leader.position, target) <= 10.0 && leader.position.y == 50.0,
                   L"within 10 wu, on the ground");
    Assert::IsTrue(leader.radius > 13.0, L"R-UNIT-159: twice its mesh radius");
    ExpectRegistered(region);
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp040ACrowdOf200WalksToAPointOnGeneratedTerrain)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-040")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-105")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp040ACrowdOf200WalksToAPointOnGeneratedTerrain)
  {
    // M1 plan T4's done-when: 200 Workers spawned around site 0 of seed 1's valley, ordered to the valley centre, all
    // arrive there alive and dry, and the grid follows every one of them.
    const auto terrain = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(terrain.has_value(), L"seed 1's valley");
    Species::RegionSimulation region(0, MOVEMENT_TEST_KEY, 0, CommittedTuning());
    region.AttachTerrain(std::make_shared<const Species::RegionTerrain>(*terrain));
    Species::RegionSimulation::Spawn spawn;
    spawn.position = terrain->layout.sites[0];
    spawn.count = 200;
    spawn.spread = 60.0;
    const std::vector<std::uint32_t> slots = region.SpawnFigures(spawn);
    region.Step(region.NextTick());
    const Neuron::Vec3 center = terrain->layout.valleyCenter;
    for (const std::uint32_t slot : slots)
    {
      Species::GiveWaypointOrder(region.TeamAt(0).figures.At(slot), center, 0, region.Movement());
    }
    const auto walking = [&]
    {
      return std::ranges::count_if(slots, [&](std::uint32_t _slot)
                                   { return region.TeamAt(0).figures.At(_slot).workerState == Species::WorkerState::OnWaypointOrder; });
    };
    const std::uint64_t ticks = StepUntil(region, [&] { return walking() == 0; }, 2000);
    Assert::IsTrue(walking() == 0, std::format(L"every Worker arrived; {} ticks", ticks).c_str());
    for (const std::uint32_t slot : slots)
    {
      const Species::Figure& worker = region.TeamAt(0).figures.At(slot);
      Assert::IsTrue(!worker.dead && worker.onGround && worker.position.y > 0.0, L"alive and dry");
      Assert::IsTrue(Neuron::HorizontalDistance(worker.position, center) < 60.0, L"at the centre");
    }
    ExpectRegistered(region);
  }
};

} // namespace GameLogicTests
