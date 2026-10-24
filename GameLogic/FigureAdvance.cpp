#include "pch.h"

#include "DrawPurpose.h"
#include "FigureAdvance.h"
#include "Formation.h"
#include "LeaderOrders.h"
#include "Navigation.h"
#include "RegionSimulation.h"
#include "SettlementTasks.h"
#include "StructureAdvance.h"

#include <cmath>
#include <cstdint>
#include <limits>
#include <numbers>
#include <optional>
#include <vector>

namespace Species
{

namespace
{

constexpr double TWO_PI = 2.0 * std::numbers::pi;

// R-UNIT-074, R-UNIT-096 step 2: the Leader's position plus a point of the follow disc, radius then angle, pushed with
// kill clear (R-UNIT-115).
Neuron::Vec3 FollowPoint(const Figure& _worker, const Neuron::Vec3& _leader, const MovementContext& _context)
{
  Neuron::SimRng& disc = _context.draws->For(DrawPurpose::FollowDisc, _worker.id);
  const double radius = disc.NextUnit(_context.tuning->ref.unit.worker.followDiscRadius);
  const double angle = disc.NextUnit(TWO_PI);
  return WorkerPush(_worker, {_leader.x + std::cos(angle) * radius, _leader.y, _leader.z + std::sin(angle) * radius},
                    DrawPurpose::ShoreFollow, _context);
}

// R-UNIT-096 steps 2 and 3, over the team's live Leaders in slot order: the nearest in follow mode within the follow
// range whose formation is not full, when it is walkable from the Worker; otherwise one of those with a waypoint order
// within the signpost range, by a draw. True when the Worker found a Leader.
bool LeaderSearch(Figure& _worker, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  const MovementContext& movement = *_context.movement;
  const TuningMvp& tuning = *movement.tuning;
  const auto& worker = tuning.ref.unit.worker;
  const SlicedPool<Figure>& figures = region.TeamAt(_context.self.team).figures;
  std::optional<std::uint32_t> nearest;
  double nearestDistance = std::numeric_limits<double>::infinity();
  std::vector<std::uint32_t> signposts;
  for (std::uint32_t slot = 0; slot < figures.Capacity(); ++slot)
  {
    if (!figures.Used(slot))
    {
      continue;
    }
    const Figure& leader = figures.At(slot);
    if (leader.kind != FigureKind::Leader || leader.dead || leader.demoted)
    {
      continue;
    }
    const double distance = Neuron::Length(leader.position - _worker.position);
    if (leader.order == LeaderOrder::Follow && distance < worker.leaderFollowRange && distance < nearestDistance &&
        !FormationFull(leader, tuning))
    {
      nearest = slot;
      nearestDistance = distance;
    }
    if (leader.order == LeaderOrder::Waypoint && distance < worker.leaderSignpostRange)
    {
      signposts.push_back(slot);
    }
  }
  if (nearest && IsPathClear(_worker.position, figures.At(*nearest).position, true, false, *movement.terrain, tuning))
  {
    const Figure& leader = figures.At(*nearest);
    _worker.workerState = WorkerState::FollowingLeader;
    _worker.leaderId = leader.id;
    _worker.leaderSlot = *nearest;
    _worker.followTimerTicks = static_cast<std::int32_t>(worker.followRetargetTicks);
    _worker.waypoint = FollowPoint(_worker, leader.position, movement);
    return true;
  }
  if (signposts.empty())
  {
    return false;
  }
  // Step 3, the signpost: the Leader's order position, route and structure become the pending order.
  const std::uint64_t pick = movement.draws->For(DrawPurpose::SignpostPick, _worker.id).NextUint() % signposts.size();
  const Figure& leader = figures.At(signposts[static_cast<std::size_t>(pick)]);
  _worker.orderPosition = leader.orderPosition;
  _worker.orderSet = true;
  _worker.route = leader.route;
  _worker.routePoint = -1;
  Neuron::SimRng& disc = movement.draws->For(DrawPurpose::OrderDisc, _worker.id);
  const double radius = disc.NextUnit(worker.orderDiscRadius);
  const double angle = disc.NextUnit(TWO_PI);
  const Neuron::Vec3 point{leader.orderPosition.x + std::cos(angle) * radius, leader.orderPosition.y,
                           leader.orderPosition.z + std::sin(angle) * radius};
  _worker.waypoint = WorkerPush(_worker, point, DrawPurpose::ShoreOrder, movement);
  _worker.workerState = WorkerState::OnWaypointOrder;
  region.Emit({EffectEventKind::WorkerOrdered, _context.self.team, _worker.id, _worker.position, {}, 0.0});
  return true;
}

// R-UNIT-074.
void FollowingLeaderStep(Figure& _worker, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  const MovementContext& movement = *_context.movement;
  const TuningMvp& tuning = *movement.tuning;
  Figure* leader = region.LeaderOf(_worker, _context.self.team);
  if (!LeaderLeads(_worker, leader, tuning))
  {
    StopFollowing(_worker);
    return;
  }
  --_worker.followTimerTicks;
  if (_worker.followTimerTicks <= 0)
  {
    _worker.followTimerTicks = static_cast<std::int32_t>(tuning.ref.unit.worker.followRetargetTicks);
    if (!IsPathClear(_worker.position, leader->position, true, false, *movement.terrain, tuning))
    {
      StopFollowing(_worker); // the MVP has no teleport for it to follow the Leader into
      return;
    }
    _worker.waypoint = FollowPoint(_worker, leader->position, movement);
  }
  FollowRoute(_worker, movement);
  const bool formation = InFormationMode(*leader);
  if (formation)
  {
    // It registers with the formation and walks to its slot, every tick; a full formation gives the zero vector.
    _worker.waypoint = SlotPositionFor(*leader, _worker.id, *movement.terrain, tuning);
    RefreshSlot(*leader, _worker.id);
  }
  if (WorkerMoveToWaypoint(_worker, false, movement) && formation)
  {
    _worker.front = leader->front;
  }
}

// R-UNIT-087 for the searches the MVP builds: the threat search in every state that searches (R-UNIT-089); then, for
// an Idle Worker, the Leader search (R-UNIT-096) and the random-position search (R-UNIT-097), which always succeeds. The
// other searches have nothing to find in the MVP. A manning Worker searches nothing, unless the player-orders flag lets
// it search for threats (table 2.5-A, R-UNIT-089).
void TaskSearch(Figure& _figure, const FigureContext& _context)
{
  // Table 2.5-A: a manning or carrying Worker searches for nothing (MVP-031 for the carrier).
  if ((_figure.workerState == WorkerState::Manning || _figure.workerState == WorkerState::Carrying) && !_figure.playerOrders)
  {
    return;
  }
  if (ThreatSearch(_figure, _context) || _figure.workerState != WorkerState::Idle)
  {
    return;
  }
  const MovementContext& movement = *_context.movement;
  const auto& worker = movement.tuning->ref.unit.worker;
  if (_figure.orderSet)
  {
    // The pending order's position, or its route's current point, plus a point of the order disc.
    Neuron::Vec3 target = _figure.orderPosition;
    const std::vector<Neuron::Vec3>* points = movement.routes != nullptr ? movement.routes->Find(_figure.route) : nullptr;
    if (points != nullptr && _figure.routePoint >= 0 && static_cast<std::size_t>(_figure.routePoint) < points->size())
    {
      target = (*points)[static_cast<std::size_t>(_figure.routePoint)];
    }
    Neuron::SimRng& disc = movement.draws->For(DrawPurpose::OrderDisc, _figure.id);
    const double radius = disc.NextUnit(worker.orderDiscRadius);
    const double angle = disc.NextUnit(TWO_PI);
    const Neuron::Vec3 point{target.x + std::cos(angle) * radius, target.y, target.z + std::sin(angle) * radius};
    _figure.waypoint = WorkerPush(_figure, point, DrawPurpose::ShoreOrder, movement);
    _figure.workerState = WorkerState::OnWaypointOrder;
    return;
  }
  if (LeaderSearch(_figure, _context) || SettlementTaskSearch(_figure, _context))
  {
    return;
  }
  Neuron::SimRng& draw = movement.draws->For(DrawPurpose::Wander, _figure.id);
  const double chance = draw.NextUnit(1.0);
  if (chance < worker.wanderChance)
  {
    const double angle = draw.NextSigned(TWO_PI);
    const Neuron::Vec3 point{_figure.position.x + std::cos(angle) * worker.wanderDistance, _figure.position.y,
                             _figure.position.z + std::sin(angle) * worker.wanderDistance};
    _figure.waypoint = WorkerPush(_figure, point, DrawPurpose::ShoreWander, movement);
  }
}

// R-UNIT-069: a Worker walking to its port operates it for its team every tick and becomes Manning on arrival, facing
// the port's front and still; it becomes Idle when its structure is gone or another Worker occupies the port, unless it
// is a gatherer walking to its home port, which waits its turn (O110, ADR-184).
void ToPortStep(Figure& _worker, const FigureContext& _context)
{
  StructurePort* port = TargetPort(*_context.region, _worker);
  const bool taken = port != nullptr && port->occupant.has_value() && port->occupant.value_or(ActorRef{}).id != _worker.id;
  // O110 (ADR-184): a gatherer walking to its home port while another Worker mans it waits its turn there.
  const bool waits = taken && _worker.task == WorkerTask::Gather && _worker.homeStructureId == _worker.portStructureId &&
                     _worker.homePortIndex == _worker.portIndex;
  if (port == nullptr || (taken && !waits))
  {
    _worker.workerState = WorkerState::Idle;
    return;
  }
  OperatePort(*port, _context.self.team, *_context.movement->tuning);
  // A settlement's task gives the walk a route (ADR-135); its end is the port itself.
  if (_worker.route != 0)
  {
    FollowRoute(_worker, *_context.movement);
  }
  if (_worker.route == 0)
  {
    _worker.waypoint = port->position;
  }
  // It waits ref.unit.worker.arrival_slow_radius from the port, where an arriving Worker begins to slow.
  if (waits && Neuron::HorizontalDistance(_worker.position, port->position) <= _context.movement->tuning->ref.unit.worker.arrivalSlowRadius)
  {
    _worker.velocity = {};
    return;
  }
  if (WorkerMoveToWaypoint(_worker, false, *_context.movement) && !waits)
  {
    _worker.front = port->front;
    _worker.velocity = {};
    _worker.workerState = WorkerState::Manning;
  }
}

// R-UNIT-070: a manning Worker stands still while it is its port's occupant, and becomes Idle when its structure is
// gone or the port's occupant is not this Worker; a gatherer counts its time at the port (MVP-031).
void ManningStep(Figure& _worker, const FigureContext& _context)
{
  const StructurePort* port = TargetPort(*_context.region, _worker);
  if (port == nullptr || port->occupant.value_or(ActorRef{}).id != _worker.id)
  {
    _worker.workerState = WorkerState::Idle;
    return;
  }
  _worker.velocity = {};
  GatherAtPort(_worker, _context);
}

} // namespace

bool BaseAdvance(Figure& _figure, const FigureContext& _context)
{
  const MovementContext& movement = *_context.movement;
  const TuningMvp& tuning = *movement.tuning;
  if (!_figure.enabled)
  {
    return false;
  }
  if (_figure.destroyed)
  {
    return true;
  }
  if (_figure.dead)
  {
    // R-UNIT-034: the corpse fades by its rate on the ground or in the air and goes on the tick it reaches 0.
    const auto& corpse = tuning.ref.unit.corpse;
    _figure.health -= static_cast<std::int32_t>(_figure.onGround ? corpse.fadeGroundPerTick : corpse.fadeAirPerTick);
    if (_figure.health <= 0)
    {
      return true;
    }
  }
  if (_figure.reloadTimerTicks > 0)
  {
    --_figure.reloadTimerTicks;
  }
  if (_figure.kind != FigureKind::Worker && _figure.inWaterTicks >= 0 && SwimStep(_figure, movement))
  {
    static_cast<void>(
      _context.region->Damage(_context.self, -static_cast<std::int32_t>(tuning.ref.sim.water.drownDamage), DamageKind::Unresistable));
  }
  WorldClamp(_figure, movement);
  return false;
}

bool WorkerAdvance(Figure& _figure, const FigureContext& _context)
{
  const MovementContext& movement = *_context.movement;
  const TuningMvp& tuning = *movement.tuning;
  const auto& worker = tuning.ref.unit.worker;
  const double dt = tuning.ref.sim.tickPeriod;
  if (_figure.promoted)
  {
    return true; // R-UNIT-065 step 1: removed with no Essence orb
  }
  if (BaseAdvance(_figure, _context))
  {
    return true;
  }
  if (!_figure.enabled)
  {
    return false;
  }
  if (!_figure.dead && _figure.onGround && _figure.workerState != WorkerState::Burning)
  {
    --_figure.retargetTimerTicks;
    if (_figure.retargetTimerTicks <= 0)
    {
      TaskSearch(_figure, _context);
      // R-UNIT-065 step 3.1: 11 + ⌊NextUnit(1) ÷ tick period⌋ ticks, 11 to 20. The victory celebration's draw of
      // R-UNIT-088 starts nothing in a match, so with keyed draws it is not made (ADR-105).
      const double spread = movement.draws->For(DrawPurpose::RetargetTimer, _figure.id).NextUnit(worker.retargetTicks.spreadSeconds);
      _figure.retargetTimerTicks = TicksOf(worker.retargetTicks.base, spread, dt);
    }
    switch (_figure.workerState)
    {
    case WorkerState::Idle:
      static_cast<void>(WorkerMoveToWaypoint(_figure, true, movement));
      break;
    case WorkerState::OnWaypointOrder:
      FollowRoute(_figure, movement);
      if (WorkerMoveToWaypoint(_figure, false, movement))
      {
        // R-UNIT-073: arrival without a teleport structure.
        _figure.workerState = WorkerState::Idle;
        _figure.retargetTimerTicks = 0;
        _figure.orderSet = false;
        _figure.playerOrders = false;
      }
      break;
    case WorkerState::Fighting:
      FightingStep(_figure, _context);
      break;
    case WorkerState::FollowingLeader:
      FollowingLeaderStep(_figure, _context);
      break;
    case WorkerState::ToPort:
      ToPortStep(_figure, _context);
      break;
    case WorkerState::Manning:
      ManningStep(_figure, _context);
      break;
    case WorkerState::Carrying:
      CarryingStep(_figure, _context);
      break;
    case WorkerState::Drowning:
    case WorkerState::Burning:
      break;
    }
  }
  if (!_figure.onGround)
  {
    AirborneStep(_figure, movement);
  }
  // R-UNIT-065 step 6, for a Worker that is still alive.
  if (!_figure.dead && _figure.workerState == WorkerState::Burning)
  {
    BurningStep(_figure, _context);
  }
  if (!_figure.dead && _figure.workerState == WorkerState::Drowning)
  {
    // R-UNIT-080: an unresistable change that kills and releases an Essence orb.
    static_cast<void>(_context.region->Damage(_context.self, -static_cast<std::int32_t>(worker.drownDamage), DamageKind::Unresistable));
  }
  if (_figure.position.y < tuning.ref.sim.physics.seaLevel)
  {
    // R-UNIT-065 step 7: it dies on the next tick.
    _figure.workerState = WorkerState::Drowning;
    _figure.onGround = false;
  }
  if (_figure.dead && _figure.onGround)
  {
    _figure.velocity = _figure.velocity * worker.corpseSlide;
    _figure.position = _figure.position + _figure.velocity * dt;
  }
  return false;
}

bool LeaderAdvance(Figure& _figure, const FigureContext& _context)
{
  const MovementContext& movement = *_context.movement;
  const TuningMvp& tuning = *movement.tuning;
  if (_figure.demoted)
  {
    return true; // R-UNIT-154: removed with no Essence orb
  }
  if (_figure.enabled && !_figure.onGround)
  {
    AirborneStep(_figure, movement);
  }
  if (BaseAdvance(_figure, _context))
  {
    return true;
  }
  if (!_figure.enabled)
  {
    return false;
  }
  if (_figure.inWaterTicks >= 0 && SwimStep(_figure, movement))
  {
    static_cast<void>(
      _context.region->Damage(_context.self, -static_cast<std::int32_t>(tuning.ref.sim.water.drownDamage), DamageKind::Unresistable));
  }
  if (_figure.onGround && !_figure.dead)
  {
    _figure.position.y = movement.Height(_figure.position.x, _figure.position.z);
  }
  // Step 2: in a match the order None becomes PrepareWaypoint every tick.
  if (_figure.order == LeaderOrder::None)
  {
    _figure.order = LeaderOrder::PrepareWaypoint;
  }
  if (!_figure.dead && _figure.onGround && _figure.inWaterTicks < 0)
  {
    LeaderBehavior(_figure, _context); // step 3
  }
  if (_figure.dead)
  {
    _figure.velocity.y -= tuning.ref.unit.leader.corpseSinkPerTick; // step 4
  }
  LeaderOrderSteps(_figure, _context); // steps 5, 6, 8 and 9; the MVP has no teleport for step 7
  return false;
}

bool AdvanceFigure(Figure& _figure, const FigureContext& _context)
{
  return _figure.kind == FigureKind::Worker ? WorkerAdvance(_figure, _context) : LeaderAdvance(_figure, _context);
}

void FollowRoute(Figure& _worker, const MovementContext& _context)
{
  if (_worker.route == 0)
  {
    return;
  }
  const std::vector<Neuron::Vec3>* points = _context.routes != nullptr ? _context.routes->Find(_worker.route) : nullptr;
  const auto& worker = _context.tuning->ref.unit.worker;
  std::int32_t next = 0;
  if (points == nullptr)
  {
    next = -1;
  }
  else if (_worker.routePoint < 0)
  {
    next = 1;
  }
  else if (Neuron::HorizontalDistance(_worker.position, _worker.waypoint) <= worker.routeReachRadius)
  {
    next = _worker.routePoint + 1;
  }
  else
  {
    return;
  }
  if (points == nullptr || static_cast<std::size_t>(next) >= points->size())
  {
    _worker.route = 0;
    _worker.routePoint = -1;
    return;
  }
  _worker.routePoint = next;
  Neuron::SimRng& draw = _context.draws->For(DrawPurpose::RouteJitter, _worker.id);
  const double radius = draw.NextUnit(worker.routeJitterRadius);
  const double angle = draw.NextUnit(TWO_PI);
  const Neuron::Vec3& point = (*points)[static_cast<std::size_t>(next)];
  _worker.waypoint = WorkerPush(_worker, {point.x + std::cos(angle) * radius, point.y, point.z + std::sin(angle) * radius},
                                DrawPurpose::ShoreRoute, _context);
}

void GiveWaypointOrder(Figure& _worker, const Neuron::Vec3& _target, RouteHandle _route, const MovementContext& _context)
{
  const auto& worker = _context.tuning->ref.unit.worker;
  _worker.orderPosition = _target;
  _worker.orderSet = true;
  Neuron::SimRng& draw = _context.draws->For(DrawPurpose::OrderDisc, _worker.id);
  const double radius = draw.NextUnit(worker.orderDiscRadius);
  const double angle = draw.NextUnit(TWO_PI);
  const Neuron::Vec3 point{_target.x + std::cos(angle) * radius, _target.y, _target.z + std::sin(angle) * radius};
  _worker.waypoint = WorkerPush(_worker, point, DrawPurpose::ShoreOrder, _context);
  _worker.route = _route;
  _worker.routePoint = -1;
  _worker.workerState = WorkerState::OnWaypointOrder;
}

} // namespace Species
