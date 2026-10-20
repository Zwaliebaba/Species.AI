#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "DrawPurpose.h"
#include "FigureAdvance.h"
#include "Movement.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "RouteStore.h"
#include "TestSupport.h"
#include "WorldSnapshot.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <format>
#include <memory>
#include <numbers>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey ROUTE_TEST_KEY{0x0a0b0c0d0e0f1011, 0x1213141516171819};

std::shared_ptr<const Species::RegionTerrain> SeedOneTerrain()
{
  static const std::shared_ptr<const Species::RegionTerrain> TERRAIN = []
  {
    auto generated = Species::GenerateRegionTerrain(1, CommittedTuning());
    Assert::IsTrue(generated.has_value(), L"seed 1's valley");
    return std::make_shared<const Species::RegionTerrain>(generated ? std::move(*generated) : Species::RegionTerrain{});
  }();
  return TERRAIN;
}

Neuron::Vec3 OnTerrain(const Species::RegionTerrain& _terrain, const Neuron::Vec3& _point)
{
  return {_point.x, _terrain.field.Height(_point.x, _point.z), _point.z};
}

} // namespace

TEST_CLASS(RouteTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit110AWorkerStepsAlongItsRouteWithJitter)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-110")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit110AWorkerStepsAlongItsRouteWithJitter)
  {
    // The first step jumps to point 1; the next comes within 20 wu of the waypoint, each a draw of its own purpose within
    // 40 wu of its point; past the last the route is cleared and the waypoint stays.
    Species::RegionSimulation region(0, ROUTE_TEST_KEY, 0, CommittedTuning(), FlatTerrain());
    Species::RouteStore store;
    const std::vector<Neuron::Vec3> points{{33000.0, 50.0, 33000.0}, {33200.0, 50.0, 33000.0}, {33200.0, 50.0, 33300.0}};
    const Species::RouteHandle route = store.Add(points);
    Species::MovementContext context = region.Movement();
    context.routes = &store;
    Species::Figure worker;
    worker.id = 21;
    worker.onGround = true;
    worker.position = points[0];
    worker.route = route;
    Species::FollowRoute(worker, context);
    ExpectEqual(1, worker.routePoint, L"the first step jumps to point 1");
    Neuron::SimRng draw(ROUTE_TEST_KEY, Species::PurposeOf(Species::DrawPurpose::RouteJitter), 21, 0);
    const double radius = draw.NextUnit(40.0);
    const double angle = draw.NextUnit(2.0 * std::numbers::pi);
    Assert::IsTrue(worker.waypoint == Neuron::Vec3{points[1].x + std::cos(angle) * radius, 50.0, points[1].z + std::sin(angle) * radius},
                   L"its keyed jitter, on the terrain");
    Species::FollowRoute(worker, context);
    ExpectEqual(1, worker.routePoint, L"far from the waypoint it keeps it");
    worker.position = worker.waypoint + Neuron::Vec3{15.0, 0.0, 0.0};
    Species::FollowRoute(worker, context);
    ExpectEqual(2, worker.routePoint, L"within 20 wu it steps on");
    Assert::IsTrue(Neuron::HorizontalDistance(worker.waypoint, points[2]) < 40.0, L"jittered within 40 wu");
    const Neuron::Vec3 last = worker.waypoint;
    worker.position = last;
    Species::FollowRoute(worker, context);
    Assert::IsTrue(worker.route == 0 && worker.routePoint == -1 && worker.waypoint == last, L"past the end: cleared, the waypoint kept");
    worker.route = 7;
    Species::FollowRoute(worker, context);
    Assert::IsTrue(worker.route == 0, L"a route the store does not hold is cleared (R-SIM-354)");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RSim359RoutesAreNumberedKeptAndState)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-359")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RSim359RoutesAreNumberedKeptAndState)
  {
    // Handles count from 1 and are never reused; every route stays (DEC-651's stated behaviour); a region's routes and
    // its figures' routes read back to the same state.
    const auto terrain = SeedOneTerrain();
    Species::RegionSimulation region(0, ROUTE_TEST_KEY, 0, CommittedTuning(), terrain);
    const Neuron::Vec3 from = OnTerrain(*terrain, terrain->layout.sites[0]);
    const Neuron::Vec3 to = OnTerrain(*terrain, terrain->layout.sites[3]);
    const Species::RouteHandle first = region.BuildRoute(from, to, false);
    const Species::RouteHandle second = region.BuildRoute(to, from, true);
    Assert::IsTrue(first == 1 && second == 2 && region.Routes().Count() == 2, L"1, then 2");
    Species::RegionSimulation::Spawn spawn;
    spawn.position = from;
    const std::uint32_t slot = region.SpawnFigures(spawn).front();
    region.Step(region.NextTick());
    Species::GiveWaypointOrder(region.TeamAt(0).figures.At(slot), to, first, region.Movement());
    region.Step(region.NextTick());
    Neuron::BitWriter before;
    region.WriteState(before);
    Species::RegionSimulation copy(0, ROUTE_TEST_KEY, region.NextTick(), CommittedTuning(), terrain);
    Neuron::BitReader reader(before.Bytes());
    Assert::IsTrue(copy.ReadState(reader, Species::WORLD_STATE_VERSION) && reader.Finish(), L"reads whole");
    Neuron::BitWriter again;
    copy.WriteState(again);
    Assert::IsTrue(std::ranges::equal(before.Bytes(), again.Bytes()), L"the same state");
    Assert::IsTrue(copy.TeamAt(0).figures.At(slot).route == first && copy.Routes().Find(first) != nullptr, L"the route it follows");
    // A damaged reference is refused: a route the store does not hold, a point past the route's end, a point of no route.
    const auto pastTheEnd = static_cast<std::int32_t>(region.Routes().Find(first)->size());
    for (const auto& [handle, point] : {std::pair<Species::RouteHandle, std::int32_t>{7, -1}, {first, pastTheEnd}, {first, -2}, {0, 0}})
    {
      Species::Figure& figure = region.TeamAt(0).figures.At(slot);
      figure.route = handle;
      figure.routePoint = point;
      Neuron::BitWriter damaged;
      region.WriteState(damaged);
      Species::RegionSimulation refused(0, ROUTE_TEST_KEY, region.NextTick(), CommittedTuning(), terrain);
      Neuron::BitReader damagedReader(damaged.Bytes());
      Assert::IsFalse(refused.ReadState(damagedReader, Species::WORLD_STATE_VERSION),
                      std::format(L"route {}, point {}", handle, point).c_str());
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp040ACrowdFollowsOneRouteAcrossTheValley)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-040")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-110")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp040ACrowdFollowsOneRouteAcrossTheValley)
  {
    // 200 Workers around site 3 of seed 1's valley, one route to site 0 across the centre, which bends through a node
    // on that side (the walk is clear the other way only): each follows it to its end and arrives, alive and dry.
    const auto terrain = SeedOneTerrain();
    Species::RegionSimulation region(0, ROUTE_TEST_KEY, 0, CommittedTuning(), terrain);
    Species::RegionSimulation::Spawn spawn;
    spawn.position = terrain->layout.sites[3];
    spawn.count = 200;
    spawn.spread = 60.0;
    const std::vector<std::uint32_t> slots = region.SpawnFigures(spawn);
    region.Step(region.NextTick());
    const Neuron::Vec3 target = OnTerrain(*terrain, terrain->layout.sites[0]);
    const Species::RouteHandle route = region.BuildRoute(OnTerrain(*terrain, terrain->layout.sites[3]), target, false);
    Assert::IsTrue(region.Routes().Find(route)->size() > 2, L"a route with a corner");
    for (const std::uint32_t slot : slots)
    {
      Species::GiveWaypointOrder(region.TeamAt(0).figures.At(slot), target, route, region.Movement());
    }
    const auto walking = [&]
    {
      return std::ranges::count_if(slots, [&](std::uint32_t _slot)
                                   { return region.TeamAt(0).figures.At(_slot).workerState == Species::WorkerState::OnWaypointOrder; });
    };
    std::uint64_t ticks = 0;
    while (walking() > 0 && ticks < 3000)
    {
      region.Step(region.NextTick());
      ++ticks;
    }
    Assert::IsTrue(walking() == 0, std::format(L"every Worker arrived; {} ticks", ticks).c_str());
    for (const std::uint32_t slot : slots)
    {
      const Species::Figure& worker = region.TeamAt(0).figures.At(slot);
      Assert::IsTrue(!worker.dead && worker.onGround && worker.position.y > 0.0 && worker.route == 0, L"alive, dry, its route done");
      Assert::IsTrue(Neuron::HorizontalDistance(worker.position, target) < 80.0, L"at site 0");
    }
  }
};

} // namespace GameLogicTests
