#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"
#include "CommandMessages.h"
#include "Formation.h"
#include "RegionSimulation.h"
#include "StateSender.h"
#include "TestSupport.h"
#include "WorldSnapshot.h"
#include "WorldTerrain.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <variant>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::SipHashKey COMMAND_TEST_KEY{0x7c0aa4d000000001, 0x7c0aa4d000000002};
constexpr Neuron::Vec3 MIDDLE{33000.0, 50.0, 33000.0};
constexpr Neuron::SenderId STEWARD = 7;
constexpr Neuron::SenderId AI = 9;

Species::GroundPoint Ground(const Neuron::Vec3& _point)
{
  return {_point.x, _point.z};
}

// A region on level ground whose team 0 the steward commands and whose team 1 the AI commands, and the senders'
// sequence numbers.
struct Battle
{
  Species::RegionSimulation region{0, COMMAND_TEST_KEY, 0, CommittedTuning(), FlatTerrain(50.0)};
  std::uint64_t sequence = 0;

  Battle()
  {
    region.SetCommander(0, STEWARD, false);
    region.SetCommander(1, AI, true);
  }

  template <class T> bool Submit(Neuron::SenderId _sender, const T& _command)
  {
    return region.Commands().Submit(_sender, sequence++, Neuron::EncodeMessage(_command));
  }
  void Step()
  {
    region.Step(region.NextTick());
  }
  void Step(int _ticks)
  {
    for (int tick = 0; tick < _ticks; ++tick)
    {
      Step();
    }
  }
  // The queued commands executed alone, as the next step starts; that step then finds none left.
  std::uint64_t ExecuteCommands()
  {
    const std::uint64_t tick = region.NextTick();
    region.Commands().Execute(tick);
    return tick;
  }
  // The one outcome since the last call.
  Neuron::CommandOutcome Outcome()
  {
    const std::vector<Neuron::CommandOutcome> outcomes = region.Commands().TakeOutcomes();
    ExpectEqual(1, static_cast<std::int64_t>(outcomes.size()), L"one outcome");
    return outcomes.front();
  }
  Species::ActorRef Spawn(std::uint8_t _team, Species::FigureKind _kind, const Neuron::Vec3& _position)
  {
    Species::RegionSimulation::Spawn spawn;
    spawn.team = _team;
    spawn.kind = _kind;
    spawn.position = _position;
    const std::uint32_t slot = region.SpawnFigures(spawn).front();
    Species::Figure& figure = region.TeamAt(_team).figures.At(slot);
    figure.position = _position;
    figure.onGround = true;
    return {figure.id, slot, _team};
  }
  Species::Figure& At(const Species::ActorRef& _ref)
  {
    Species::Figure* figure = region.Resolve(_ref);
    Assert::IsNotNull(figure, L"the figure resolves");
    return *figure;
  }
  std::int64_t Count(std::uint8_t _team, Species::FigureKind _kind) const
  {
    const auto& figures = region.TeamAt(_team).figures;
    std::int64_t count = 0;
    for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
    {
      count += figures.Used(slot) && figures.At(slot).kind == _kind ? 1 : 0;
    }
    return count;
  }
  // A Leader of team 0 at _position that the steward has selected.
  Species::ActorRef SelectedLeader(const Neuron::Vec3& _position)
  {
    const Species::ActorRef leader = Spawn(0, Species::FigureKind::Leader, _position);
    Assert::IsTrue(Submit(STEWARD, Species::SelectCommand{0, leader.id}));
    Step();
    static_cast<void>(region.Commands().TakeOutcomes());
    return leader;
  }
};

// The region's state as its snapshot holds it and its hash covers it (ADR-083 item 4).
std::vector<std::byte> StateOf(const Species::RegionSimulation& _region)
{
  Neuron::BitWriter writer;
  _region.WriteState(writer);
  return writer.Take();
}

void ExpectOutcome(const Neuron::CommandOutcome& _outcome, Neuron::CommandEffect _effect, std::uint64_t _tick, std::wstring_view _what)
{
  Assert::IsTrue(!_outcome.rejection && _outcome.effect == _effect, std::wstring(_what).c_str());
  ExpectEqual(static_cast<std::int64_t>(_tick), static_cast<std::int64_t>(_outcome.tick), L"the outcome carries its tick");
}

} // namespace

TEST_CLASS(CommandTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit153PromotionMakesASelectedLeader)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-153")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-139")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit153PromotionMakesASelectedLeader)
  {
    // AC-UNIT-L01: the Leader stands where the Worker stood, with its velocity, and is selected in the same tick; the
    // Worker goes at its advance, with no Essence orb. A Burning Worker is not promoted.
    Battle battle;
    const Species::ActorRef worker = battle.Spawn(0, Species::FigureKind::Worker, MIDDLE);
    battle.At(worker).velocity = {2.0, 0.0, 0.0};
    Assert::IsTrue(battle.Submit(STEWARD, Species::PromoteWorkerCommand{0, worker.id}));
    const std::uint64_t tick = battle.ExecuteCommands();
    const Species::ActorRef selected = battle.region.SelectedLeaderRef(0).value_or(Species::ActorRef{});
    Assert::IsTrue(selected.id != 0, L"a Leader is selected");
    const Species::Figure& leader = battle.At(selected);
    Assert::IsTrue(leader.kind == Species::FigureKind::Leader && leader.position == MIDDLE &&
                     leader.velocity == Neuron::Vec3{2.0, 0.0, 0.0},
                   L"at the Worker's position with its velocity");
    Assert::IsTrue(battle.At(worker).promoted, L"the Worker is promoted");
    ExpectOutcome(battle.Outcome(), Neuron::CommandEffect::Applied, tick, L"applied");
    battle.Step();
    Assert::IsFalse(battle.region.FindFigure(0, worker.id).has_value(), L"the Worker removed at its advance");
    ExpectEqual(0, battle.region.Orbs().Count(), L"no Essence orb");

    const Species::ActorRef burning = battle.Spawn(0, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{20.0, 0.0, 0.0});
    battle.region.SetOnFire(burning);
    Assert::IsTrue(battle.Submit(STEWARD, Species::PromoteWorkerCommand{0, burning.id}));
    const std::uint64_t next = battle.ExecuteCommands();
    ExpectOutcome(battle.Outcome(), Neuron::CommandEffect::NoEffect, next, L"a Burning Worker is not promoted");
    ExpectEqual(1, battle.Count(0, Species::FigureKind::Leader), L"one Leader");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit154DemotionMakesAWorkerAndClearsTheSelection)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-154")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit154DemotionMakesAWorkerAndClearsTheSelection)
  {
    // AC-UNIT-L02.
    Battle battle;
    const Species::ActorRef leader = battle.SelectedLeader(MIDDLE);
    Assert::IsTrue(battle.region.SelectedLeaderRef(0).has_value(), L"selected");
    Assert::IsTrue(battle.Submit(STEWARD, Species::DemoteLeaderCommand{0, leader.id}));
    const std::uint64_t tick = battle.ExecuteCommands();
    ExpectOutcome(battle.Outcome(), Neuron::CommandEffect::Applied, tick, L"applied");
    Assert::IsFalse(battle.region.SelectedLeaderRef(0).has_value(), L"no selection");
    ExpectEqual(1, battle.Count(0, Species::FigureKind::Worker), L"a Worker appears");
    battle.Step();
    Assert::IsFalse(battle.region.FindFigure(0, leader.id).has_value(), L"the Leader removed at its next advance");
    ExpectEqual(0, battle.region.Orbs().Count(), L"no Essence orb");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit171ANearClickFormsUpAndAFarOneGivesAWaypointOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-170")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-171")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-172")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-175")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-191")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit171ANearClickFormsUpAndAFarOneGivesAWaypointOrder)
  {
    // AC-UNIT-L07: a click 40 wu from an idle Leader enters formation facing it; 60 wu gives a waypoint order with a new
    // route, which the Leader hands on without walking, and clears the selection.
    Battle battle;
    const Species::ActorRef nearby = battle.SelectedLeader(MIDDLE);
    Assert::IsTrue(battle.Submit(STEWARD, Species::OrderUnitCommand{0, false, false, Ground(MIDDLE + Neuron::Vec3{40.0, 0.0, 0.0})}));
    battle.Step();
    const Species::Figure& formed = battle.At(nearby);
    Assert::IsTrue(Species::InFormationMode(formed), L"formation mode");
    Assert::IsTrue(formed.front.x > 0.99, L"facing the click");

    const Neuron::Vec3 from = MIDDLE + Neuron::Vec3{0.0, 0.0, 400.0};
    const Species::ActorRef distant = battle.SelectedLeader(from);
    const Neuron::Vec3 target = from + Neuron::Vec3{60.0, 0.0, 0.0};
    Assert::IsTrue(battle.Submit(STEWARD, Species::OrderUnitCommand{0, false, false, Ground(target)}));
    battle.Step();
    const Species::Figure& ordering = battle.At(distant);
    Assert::IsTrue(ordering.order == Species::LeaderOrder::Waypoint && !ordering.formation && ordering.route != 0, L"a waypoint order");
    Assert::IsTrue(Neuron::HorizontalDistance(ordering.orderPosition, target) < 1e-9, L"its order position");
    Assert::IsFalse(battle.region.SelectedLeaderRef(0).has_value(), L"the selection cleared");
    // ADR-125: the replicated state aims the flag pointer at the route's first point after its start, and a Leader in
    // formation has none.
    const std::vector<Neuron::Vec3>* points = battle.region.Routes().Find(ordering.route);
    Assert::IsTrue(points != nullptr && points->size() > 1, L"a route of two points or more");
    for (const Species::ReplicatedObject& object : Species::CollectReplicatedObjects(battle.region))
    {
      if (object.id == distant.id)
      {
        const auto& state = std::get<Species::ReplicatedFigure>(object.state);
        Assert::IsTrue(state.target == Species::QuantizePosition((*points)[1]), L"the waypoint Leader's target");
      }
      if (object.id == nearby.id)
      {
        Assert::IsFalse(std::get<Species::ReplicatedFigure>(object.state).target.has_value(), L"no target in formation");
      }
    }
    battle.Step(30);
    Assert::IsTrue(Neuron::HorizontalDistance(battle.At(distant).position, from) < 1e-9, L"the Leader does not walk");
    Assert::IsTrue(battle.At(distant).leaderState == Species::LeaderState::GivingOrders, L"it gives orders");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit096AWaypointLeaderIsASignpost)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-096")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-152")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit096AWaypointLeaderIsASignpost)
  {
    // AC-UNIT-L08: an Idle Worker that passes within 100 wu takes the Leader's order position and route.
    Battle battle;
    const Species::ActorRef leader = battle.SelectedLeader(MIDDLE);
    const Neuron::Vec3 target = MIDDLE + Neuron::Vec3{500.0, 0.0, 0.0};
    Assert::IsTrue(battle.Submit(STEWARD, Species::OrderUnitCommand{0, false, false, Ground(target)}));
    battle.Step();
    const Species::ActorRef worker = battle.Spawn(0, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{0.0, 0.0, 60.0});
    battle.At(worker).retargetTimerTicks = 1;
    battle.Step();
    const Species::Figure& ordered = battle.At(worker);
    Assert::IsTrue(ordered.workerState == Species::WorkerState::OnWaypointOrder && ordered.orderSet, L"on the Leader's order");
    Assert::IsTrue(ordered.orderPosition == battle.At(leader).orderPosition && ordered.route == battle.At(leader).route,
                   L"its order position and route");
    Assert::IsFalse(ordered.playerOrders, L"not the player's order");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit074WorkersGatherIntoAFormationAndLeaveIt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-074")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-096")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-173")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-183")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit074WorkersGatherIntoAFormationAndLeaveIt)
  {
    // ToggleUnit forms the Leader up facing forward and selects it; Idle Workers within 150 wu follow it into slots,
    // stand at them and are in the formation; a second ToggleUnit cancels its orders, and they are Idle again.
    Battle battle;
    const Species::ActorRef leader = battle.Spawn(0, Species::FigureKind::Leader, MIDDLE);
    std::vector<Species::ActorRef> workers;
    workers.reserve(30);
    for (int index = 0; index < 30; ++index)
    {
      const double angle = static_cast<double>(index) * 0.7;
      workers.push_back(
        battle.Spawn(0, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{std::cos(angle) * 80.0, 0.0, std::sin(angle) * 80.0}));
    }
    Assert::IsTrue(battle.Submit(STEWARD, Species::ToggleUnitCommand{0, leader.id}));
    const std::uint64_t tick = battle.region.NextTick();
    battle.Step();
    ExpectOutcome(battle.Outcome(), Neuron::CommandEffect::Applied, tick, L"applied");
    Assert::IsTrue(Species::InFormationMode(battle.At(leader)), L"in formation");
    Assert::IsTrue(battle.region.SelectedLeaderRef(0).value_or(Species::ActorRef{}).id == leader.id, L"and selected");
    battle.Step(300);
    const Species::Figure& formed = battle.At(leader);
    std::int64_t members = 0;
    double farthest = 0.0;
    for (const Species::ActorRef& ref : workers)
    {
      const Species::Figure& worker = battle.At(ref);
      if (!battle.region.InFormation(worker, 0))
      {
        continue;
      }
      ++members;
      const std::uint32_t index = *Species::SlotOf(formed, worker.id);
      const Neuron::Vec3 slot = Species::FormationSlotPosition(formed.position, index, formed.front, formed.slots.size(),
                                                               battle.region.Terrain()->field, CommittedTuning());
      farthest = std::max(farthest, Neuron::HorizontalDistance(worker.position, slot));
    }
    ExpectEqual(30, members, L"every Worker holds a slot");
    Assert::IsTrue(farthest < 1.0, L"each at its slot");

    Assert::IsTrue(battle.Submit(STEWARD, Species::ToggleUnitCommand{0, leader.id}));
    battle.Step(2);
    Assert::IsFalse(Species::InFormationMode(battle.At(leader)), L"out of formation");
    for (const Species::ActorRef& ref : workers)
    {
      Assert::IsTrue(battle.At(ref).workerState != Species::WorkerState::FollowingLeader && !battle.region.InFormation(battle.At(ref), 0),
                     L"no longer following");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit185AWaypointOrderLeavesMarkersAndTrails)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-163")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-185")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-186")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-187")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit185AWaypointOrderLeavesMarkersAndTrails)
  {
    // AC-UNIT-L10: over 100 ticks of a waypoint order, a marker every 20 ticks, each dropping a trail mark every 2 of its
    // advances, and every trail mark removed 10 ticks after its creation.
    Battle battle;
    static_cast<void>(battle.SelectedLeader(MIDDLE));
    battle.Step(25); // more than a marker period after tick 0
    Assert::IsTrue(battle.Submit(STEWARD, Species::OrderUnitCommand{0, false, false, Ground(MIDDLE + Neuron::Vec3{900.0, 0.0, 0.0})}));
    std::vector<std::uint64_t> markerTicks;
    std::set<Species::StableId> markers;
    std::map<Species::StableId, std::uint64_t> created;
    std::map<Species::StableId, std::uint64_t> lifetimes;
    std::map<Species::StableId, std::int64_t> dropped;
    for (int step = 0; step < 100; ++step)
    {
      const std::uint64_t tick = battle.region.NextTick();
      battle.Step();
      const auto& effects = battle.region.Effects();
      std::set<Species::StableId> marks;
      for (std::uint32_t slot = 0; slot < effects.Capacity(); ++slot)
      {
        if (!effects.Used(slot))
        {
          continue;
        }
        const Species::Effect& effect = effects.At(slot);
        if (effect.kind == Species::EffectKind::OrderMarker && markers.insert(effect.id).second)
        {
          markerTicks.push_back(tick);
        }
        if (effect.kind == Species::EffectKind::OrderTrailMark)
        {
          marks.insert(effect.id);
          if (created.try_emplace(effect.id, effect.createdTick).second)
          {
            ExpectEqual(static_cast<std::int64_t>(tick), static_cast<std::int64_t>(effect.createdTick),
                        L"created in the step it first appears");
          }
        }
      }
      for (const auto& [id, at] : created)
      {
        if (!marks.contains(id) && !lifetimes.contains(id))
        {
          lifetimes[id] = tick - at;
        }
      }
    }
    ExpectEqual(5, static_cast<std::int64_t>(markerTicks.size()), L"five markers");
    for (std::size_t index = 1; index < markerTicks.size(); ++index)
    {
      ExpectEqual(20, static_cast<std::int64_t>(markerTicks[index] - markerTicks[index - 1]), L"one every 20 ticks");
    }
    Assert::IsTrue(created.size() >= 100, L"trail marks every 2 ticks of each marker");
    Assert::IsFalse(lifetimes.empty(), L"some removed");
    for (const auto& [id, life] : lifetimes)
    {
      ExpectEqual(10, static_cast<std::int64_t>(life), L"removed 10 ticks after its creation");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit140ACrowdOrderSendsTheSelectedWorkers)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-136")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-137")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-140")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-101")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit140ACrowdOrderSendsTheSelectedWorkers)
  {
    // The circle selects the team's live Workers in it that are not Burning; the order sends them along one route as the
    // player's order and releases them; with none under control it does nothing.
    Battle battle;
    std::vector<Species::ActorRef> inside;
    inside.reserve(10);
    for (int index = 0; index < 10; ++index)
    {
      inside.push_back(battle.Spawn(0, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{static_cast<double>(index) * 4.0, 0.0, 0.0}));
    }
    const Species::ActorRef burning = inside.back();
    inside.pop_back();
    battle.region.SetOnFire(burning);
    const Species::ActorRef outside = battle.Spawn(0, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{200.0, 0.0, 0.0});
    const Species::ActorRef enemy = battle.Spawn(1, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{0.0, 0.0, 10.0});
    Assert::IsTrue(battle.Submit(STEWARD, Species::SelectWorkersCommand{0, Ground(MIDDLE), 50.0}));
    std::uint64_t tick = battle.ExecuteCommands();
    ExpectOutcome(battle.Outcome(), Neuron::CommandEffect::Applied, tick, L"selected");
    for (const Species::ActorRef& ref : inside)
    {
      Assert::IsTrue(battle.At(ref).underControl, L"under control");
    }
    Assert::IsFalse(battle.At(burning).underControl || battle.At(outside).underControl || battle.At(enemy).underControl,
                    L"not the Burning Worker, one outside the circle or an enemy");
    battle.Step();

    const Neuron::Vec3 target = MIDDLE + Neuron::Vec3{0.0, 0.0, 600.0};
    Assert::IsTrue(battle.Submit(STEWARD, Species::OrderWorkersCommand{0, false, Ground(target)}));
    tick = battle.ExecuteCommands();
    ExpectOutcome(battle.Outcome(), Neuron::CommandEffect::Applied, tick, L"ordered");
    const Species::RouteHandle route = battle.At(inside.front()).route;
    Assert::IsTrue(route != 0, L"a route");
    for (const Species::ActorRef& ref : inside)
    {
      const Species::Figure& worker = battle.At(ref);
      Assert::IsTrue(worker.workerState == Species::WorkerState::OnWaypointOrder && worker.playerOrders && worker.route == route &&
                       worker.orderPosition == Neuron::Vec3{target.x, 50.0, target.z} && !worker.underControl,
                     L"on the player's order along the one route, released");
    }
    Assert::IsTrue(battle.At(outside).workerState != Species::WorkerState::OnWaypointOrder, L"the Worker outside is not ordered");
    battle.Step();
    Assert::IsTrue(battle.Submit(STEWARD, Species::OrderWorkersCommand{0, false, Ground(target)}));
    tick = battle.ExecuteCommands();
    ExpectOutcome(battle.Outcome(), Neuron::CommandEffect::NoEffect, tick, L"nobody under control");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RCtrl158TheCircleAddsWorkersAndASelectReleasesThem)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-156")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-158")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-CTRL-161")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RCtrl158TheCircleAddsWorkersAndASelectReleasesThem)
  {
    // A second circle that no longer holds the first Worker adds the second and keeps the first; a Select releases both.
    Battle battle;
    const Species::ActorRef first = battle.Spawn(0, Species::FigureKind::Worker, MIDDLE);
    const Species::ActorRef second = battle.Spawn(0, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{80.0, 0.0, 0.0});
    Assert::IsTrue(battle.Submit(STEWARD, Species::SelectWorkersCommand{0, Ground(MIDDLE), 20.0}));
    std::uint64_t tick = battle.ExecuteCommands();
    ExpectOutcome(battle.Outcome(), Neuron::CommandEffect::Applied, tick, L"the first circle");
    Assert::IsTrue(battle.At(first).underControl && !battle.At(second).underControl, L"the first Worker");
    battle.Step();
    Assert::IsTrue(battle.Submit(STEWARD, Species::SelectWorkersCommand{0, Ground(MIDDLE + Neuron::Vec3{80.0, 0.0, 0.0}), 20.0}));
    tick = battle.ExecuteCommands();
    ExpectOutcome(battle.Outcome(), Neuron::CommandEffect::Applied, tick, L"the second circle");
    Assert::IsTrue(battle.At(first).underControl && battle.At(second).underControl, L"added, none removed");
    battle.Step();
    Assert::IsTrue(battle.Submit(STEWARD, Species::SelectCommand{0, 0}));
    tick = battle.ExecuteCommands();
    ExpectOutcome(battle.Outcome(), Neuron::CommandEffect::Applied, tick, L"selected nothing");
    Assert::IsFalse(battle.At(first).underControl || battle.At(second).underControl, L"the group released");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit168ASelectedLeaderFacesThePointer)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-168")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-167")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit168ASelectedLeaderFacesThePointer)
  {
    // The control state sets the team's pointer; the selected Leader that prepares a waypoint turns to it; deselected,
    // its order flickers back to None and it turns no more.
    Battle battle;
    const Species::ActorRef leader = battle.SelectedLeader(MIDDLE);
    Assert::IsTrue(battle.Submit(STEWARD, Species::ControlStateCommand{0, Ground(MIDDLE + Neuron::Vec3{-300.0, 0.0, 0.0})}));
    battle.Step(30);
    Assert::IsTrue(battle.region.TeamAt(0).pointer == Neuron::Vec3{MIDDLE.x - 300.0, 50.0, MIDDLE.z}, L"the pointer on the ground");
    Assert::IsTrue(battle.At(leader).order == Species::LeaderOrder::PrepareWaypoint && battle.At(leader).front.x < -0.99, L"facing it");
    Assert::IsTrue(battle.Submit(STEWARD, Species::SelectCommand{0, 0}));
    Assert::IsTrue(battle.Submit(STEWARD, Species::ControlStateCommand{0, Ground(MIDDLE + Neuron::Vec3{0.0, 0.0, 300.0})}));
    battle.Step(30);
    Assert::IsTrue(battle.At(leader).front.x < -0.99, L"no longer turning");
    Assert::IsTrue(battle.At(leader).order == Species::LeaderOrder::None, L"None at the end of each tick");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp094EveryRefusalIsCountedWithItsTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-094")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp094EveryRefusalIsCountedWithItsTick)
  {
    // A steward commands only its own team and sends no AI command; a field out of its bounds is malformed (DEC-122,
    // ADR-087 item 2). Every refusal is counted and names the tick it would have executed in.
    Battle battle;
    const Species::ActorRef own = battle.Spawn(0, Species::FigureKind::Worker, MIDDLE);
    const Species::ActorRef theirs = battle.Spawn(1, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{0.0, 0.0, 300.0});
    const std::uint64_t tick = battle.region.NextTick();
    const auto refused = [&](std::wstring_view _what, Neuron::CommandRejection _reason, auto _submit)
    {
      Assert::IsFalse(_submit(), std::wstring(_what).c_str());
      const Neuron::CommandOutcome outcome = battle.Outcome();
      Assert::IsTrue(outcome.rejection == _reason, std::wstring(_what).c_str());
      ExpectEqual(static_cast<std::int64_t>(tick), static_cast<std::int64_t>(outcome.tick), L"the tick it would have executed in");
    };
    refused(L"another team", Neuron::CommandRejection::NotPermitted,
            [&] { return battle.Submit(STEWARD, Species::PromoteWorkerCommand{1, theirs.id}); });
    refused(L"an AI command from a steward", Neuron::CommandRejection::NotPermitted,
            [&] { return battle.Submit(STEWARD, Species::AiOrderWorkerCommand{0, own.id, Ground(MIDDLE)}); });
    refused(L"a sender who commands nothing", Neuron::CommandRejection::NotPermitted,
            [&] { return battle.Submit(42, Species::SelectCommand{0, 0}); });
    refused(L"a team that nobody commands", Neuron::CommandRejection::NotPermitted,
            [&] { return battle.Submit(STEWARD, Species::SelectCommand{5, 0}); });
    refused(L"an id of 0", Neuron::CommandRejection::Malformed,
            [&] { return battle.Submit(STEWARD, Species::PromoteWorkerCommand{0, 0}); });
    refused(L"a negative radius", Neuron::CommandRejection::Malformed,
            [&] { return battle.Submit(STEWARD, Species::SelectWorkersCommand{0, Ground(MIDDLE), -1.0}); });
    refused(
      L"a point that is not finite", Neuron::CommandRejection::Malformed,
      [&] { return battle.Submit(STEWARD, Species::OrderWorkersCommand{0, false, {std::numeric_limits<double>::quiet_NaN(), MIDDLE.z}}); });
    refused(L"a message that is no command", Neuron::CommandRejection::UnknownType,
            [&] { return battle.Submit(STEWARD, Species::WorldTerrain{}); });
    Assert::IsTrue(battle.region.Commands().Submit(STEWARD, 1000, Neuron::EncodeMessage(Species::SelectCommand{0, 0})));
    refused(L"a sequence number used twice", Neuron::CommandRejection::Duplicate,
            [&] { return battle.region.Commands().Submit(STEWARD, 1000, Neuron::EncodeMessage(Species::SelectCommand{0, 0})); });
    const Neuron::CommandRejectionCounts& counts = battle.region.Commands().Rejections();
    ExpectEqual(9, static_cast<std::int64_t>(counts.Total()), L"every refusal counted");
    ExpectEqual(4, static_cast<std::int64_t>(counts.notPermitted), L"not permitted");
    ExpectEqual(3, static_cast<std::int64_t>(counts.malformed), L"malformed");

    // The AI's own command from the AI, and a command whose figure has gone by its tick.
    Assert::IsTrue(battle.Submit(AI, Species::AiOrderWorkerCommand{1, theirs.id, Ground(MIDDLE + Neuron::Vec3{0.0, 0.0, 500.0})}));
    Assert::IsTrue(battle.Submit(STEWARD, Species::PromoteWorkerCommand{0, own.id}));
    battle.At(own).destroyed = true;
    battle.Step();
    const std::vector<Neuron::CommandOutcome> outcomes = battle.region.Commands().TakeOutcomes();
    ExpectEqual(3, static_cast<std::int64_t>(outcomes.size()), L"three executed");
    for (const Neuron::CommandOutcome& outcome : outcomes)
    {
      ExpectEqual(static_cast<std::int64_t>(tick), static_cast<std::int64_t>(outcome.tick), L"each with its tick");
      const bool promotion = outcome.sender == STEWARD && outcome.sequence != 1000;
      Assert::IsTrue(outcome.effect == (promotion ? Neuron::CommandEffect::NoEffect : Neuron::CommandEffect::Applied),
                     L"the promotion of a figure that has gone does nothing");
    }
    Assert::IsTrue(battle.At(theirs).workerState == Species::WorkerState::OnWaypointOrder && battle.At(theirs).route == 0,
                   L"the AI's waypoint order, with no route");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dec122TheAiOrdersItsLeaders)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-188")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-163")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-170")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dec122TheAiOrdersItsLeaders)
  {
    // The AI's Leader operations and fields (R-BOT-015 through DEC-122's command types): a formation, the bot target
    // front that a Leader turns to while it does not walk, and the no-formations flag, which makes a near click a
    // waypoint order.
    Battle battle;
    const Species::ActorRef formed = battle.Spawn(1, Species::FigureKind::Leader, MIDDLE);
    Assert::IsTrue(battle.Submit(AI, Species::AiOrderLeaderCommand{1, formed.id, Species::LeaderOperation::FormUp, false,
                                                                   Ground(MIDDLE + Neuron::Vec3{0.0, 0.0, -50.0})}));
    battle.Step();
    Assert::IsTrue(Species::InFormationMode(battle.At(formed)) && battle.At(formed).front.z < -0.99, L"formed up, facing the point");
    Assert::IsTrue(battle.Submit(AI, Species::AiLeaderFieldsCommand{1, formed.id, true, true, {1.0, 0.0}}));
    battle.Step(60);
    Assert::IsTrue(battle.At(formed).botTurn && battle.At(formed).front.x > 0.95, L"turned to its bot target front");

    const Species::ActorRef flagged = battle.Spawn(1, Species::FigureKind::Leader, MIDDLE + Neuron::Vec3{0.0, 0.0, 400.0});
    Assert::IsTrue(battle.Submit(AI, Species::AiLeaderFieldsCommand{1, flagged.id, true, false, {}}));
    Assert::IsTrue(battle.Submit(AI, Species::AiOrderLeaderCommand{1, flagged.id, Species::LeaderOperation::AssignOrders, true,
                                                                   Ground(MIDDLE + Neuron::Vec3{30.0, 0.0, 400.0})}));
    battle.Step();
    Assert::IsTrue(battle.At(flagged).order == Species::LeaderOrder::Waypoint && !battle.At(flagged).botTurn,
                   L"no formation toggle for a Leader without formations");
    for (const Neuron::CommandOutcome& outcome : battle.region.Commands().TakeOutcomes())
    {
      Assert::IsTrue(!outcome.rejection && outcome.effect == Neuron::CommandEffect::Applied, L"every AI command applied");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(RUnit184AFormationHoldsItsSlotsAndFiresInVolleys)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-184")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-102")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-UNIT-116")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(RUnit184AFormationHoldsItsSlotsAndFiresInVolleys)
  {
    // Members that fight hold their slots, fire, throw no grenade, and take explosive damage × 0.6.
    Battle battle;
    const Species::ActorRef leader = battle.Spawn(0, Species::FigureKind::Leader, MIDDLE);
    battle.At(leader).front = {0.0, 0.0, 1.0};
    std::vector<Species::ActorRef> members;
    members.reserve(12);
    for (int index = 0; index < 12; ++index)
    {
      const int rank = index / 4;
      members.push_back(
        battle.Spawn(0, Species::FigureKind::Worker,
                     MIDDLE + Neuron::Vec3{static_cast<double>(index % 4) * 6.0 - 9.0, 0.0, -20.0 - static_cast<double>(rank) * 6.0}));
    }
    Assert::IsTrue(battle.Submit(STEWARD, Species::ToggleUnitCommand{0, leader.id}));
    battle.Step(150);
    for (const Species::ActorRef& ref : members)
    {
      Assert::IsTrue(battle.region.InFormation(battle.At(ref), 0), L"in the formation");
    }
    const Species::ActorRef enemy = battle.Spawn(1, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{0.0, 0.0, 85.0});
    battle.At(enemy).grenadeTimerTicks = 100'000;
    std::int64_t fighting = 0;
    std::int64_t shots = 0;
    for (int step = 0; step < 40; ++step)
    {
      battle.Step();
      const auto& effects = battle.region.Effects();
      for (std::uint32_t slot = 0; slot < effects.Capacity(); ++slot)
      {
        Assert::IsFalse(effects.Used(slot) && effects.At(slot).kind == Species::EffectKind::Grenade, L"no grenade");
      }
      for (const Species::ActorRef& ref : members)
      {
        fighting += battle.At(ref).workerState == Species::WorkerState::Fighting ? 1 : 0;
        Assert::IsTrue(battle.region.InFormation(battle.At(ref), 0), L"holding its slot while it fights");
      }
      for (const Species::EffectEvent& event : battle.region.Events())
      {
        shots += event.kind == Species::EffectEventKind::LaserFired && event.team == 0 ? 1 : 0;
      }
    }
    Assert::IsTrue(fighting > 0, L"members fight the enemy ahead");
    Assert::IsTrue(shots > 0, L"and fire at it");
    const Species::ActorRef member = members.front();
    const std::int32_t before = battle.At(member).health;
    static_cast<void>(battle.region.Damage(member, -31, Species::DamageKind::Explosive));
    ExpectEqual(before - 18, battle.At(member).health, L"explosive damage × 0.6, truncated");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp095LeadersFormationsAndMarkersSurviveARestore)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-095")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp095LeadersFormationsAndMarkersSurviveARestore)
  {
    // A region with commanders, a formation and its members, a waypoint Leader with markers and trail marks, a
    // selection, a pointer and a queued command reads back to the same state and steps on alike.
    Battle battle;
    const Species::ActorRef formed = battle.Spawn(0, Species::FigureKind::Leader, MIDDLE);
    for (int index = 0; index < 20; ++index)
    {
      static_cast<void>(
        battle.Spawn(0, Species::FigureKind::Worker, MIDDLE + Neuron::Vec3{static_cast<double>(index) * 5.0 - 50.0, 0.0, -40.0}));
    }
    Assert::IsTrue(battle.Submit(STEWARD, Species::ToggleUnitCommand{0, formed.id}));
    battle.Step(40);
    const Species::ActorRef signpost = battle.Spawn(1, Species::FigureKind::Leader, MIDDLE + Neuron::Vec3{0.0, 0.0, 900.0});
    Assert::IsTrue(battle.Submit(AI, Species::AiOrderLeaderCommand{1, signpost.id, Species::LeaderOperation::AssignOrders, false,
                                                                   Ground(MIDDLE + Neuron::Vec3{700.0, 0.0, 900.0})}));
    Assert::IsTrue(battle.Submit(STEWARD, Species::ControlStateCommand{0, Ground(MIDDLE + Neuron::Vec3{10.0, 0.0, 10.0})}));
    battle.Step(30);
    Assert::IsTrue(battle.Submit(STEWARD, Species::SelectWorkersCommand{0, Ground(MIDDLE), 100.0}));

    const std::vector<std::byte> bytes = StateOf(battle.region);
    Species::RegionSimulation restored(0, COMMAND_TEST_KEY, battle.region.NextTick(), CommittedTuning(), FlatTerrain(50.0));
    Neuron::BitReader reader(bytes);
    Assert::IsTrue(restored.ReadState(reader, Species::WORLD_STATE_VERSION) && reader.Finish(), L"reads whole");
    Assert::IsTrue(StateOf(restored) == bytes, L"the same state");
    for (int step = 0; step < 40; ++step)
    {
      battle.Step();
      restored.Step(restored.NextTick());
      Assert::IsTrue(StateOf(restored) == StateOf(battle.region), L"and steps on alike");
    }
    std::int64_t markers = 0;
    for (std::uint32_t slot = 0; slot < restored.Effects().Capacity(); ++slot)
    {
      markers += restored.Effects().Used(slot) && restored.Effects().At(slot).kind == Species::EffectKind::OrderMarker ? 1 : 0;
    }
    Assert::IsTrue(markers > 0, L"with its order markers");
  }
};

} // namespace GameLogicTests
