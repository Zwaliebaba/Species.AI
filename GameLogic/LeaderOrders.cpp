#include "pch.h"

#include "DrawPurpose.h"
#include "Formation.h"
#include "Geometry.h"
#include "LeaderOrders.h"
#include "Movement.h"
#include "OrderMarkers.h"
#include "RegionSimulation.h"

#include <optional>
#include <vector>

namespace Species
{

namespace
{

// The unit direction from _from to _to, or _fallback when the two coincide.
Neuron::Vec3 DirectionOr(const Neuron::Vec3& _from, const Neuron::Vec3& _to, const Neuron::Vec3& _fallback) noexcept
{
  const Neuron::Vec3 offset = _to - _from;
  return Neuron::LengthSquared(offset) > Neuron::NORMALIZE_MIN_LENGTH_SQUARED ? Neuron::Normalized(offset) : _fallback;
}

// R-UNIT-158: the front blended towards the direction from _from to _to by _factor and renormalised.
Neuron::Vec3 TurnedTowards(const Neuron::Vec3& _front, const Neuron::Vec3& _from, const Neuron::Vec3& _to, double _factor) noexcept
{
  const Neuron::Vec3 direction = DirectionOr(_from, _to, _front);
  return Neuron::Normalized(_front * (1.0 - _factor) + direction * _factor);
}

// R-UNIT-158: the factor of facing an order target, reduced in formation mode.
double OrderTurn(const Figure& _leader, const TuningMvp& _tuning) noexcept
{
  const auto& leader = _tuning.ref.unit.leader;
  return InFormationMode(_leader) ? leader.turnOrder * leader.turnOrderFormationFactor : leader.turnOrder;
}

// R-UNIT-166.
void ToWaypoint(Figure& _leader, const FigureContext& _context)
{
  const MovementContext& movement = *_context.movement;
  const auto& leader = movement.tuning->ref.unit.leader;
  const bool formationMode = InFormationMode(_leader);
  if (formationMode && !_leader.formationAngleSet)
  {
    // The formation turns in place before it moves.
    _leader.velocity = {};
    _leader.front = TurnedTowards(_leader.front, _leader.position, _leader.waypoint, leader.turnPreMove);
    const Neuron::Vec3 direction = DirectionOr(_leader.position, _leader.waypoint, _leader.front);
    if (Neuron::Length(Neuron::Cross(direction, _leader.front)) < leader.preTurnTolerance)
    {
      _leader.formationAngleSet = true;
    }
  }
  bool arrived = false;
  if (!formationMode || _leader.formationAngleSet)
  {
    arrived = LeaderMoveToWaypoint(_leader, formationMode, movement);
  }
  if (_leader.formation)
  {
    _leader.orderPosition = _leader.position + _leader.front * leader.formationFaceDistance;
  }
  if (arrived)
  {
    _leader.leaderState = LeaderState::Idle;
  }
}

// R-UNIT-167.
void GivingOrders(Figure& _leader, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  const TuningMvp& tuning = region.Tuning();
  _leader.velocity = {};
  switch (_leader.order)
  {
  case LeaderOrder::Waypoint:
  {
    // It faces the first point after the start of its order route.
    const std::vector<Neuron::Vec3>* points = region.Routes().Find(_leader.route);
    if (points != nullptr && points->size() > 1)
    {
      _leader.front = TurnedTowards(_leader.front, _leader.position, (*points)[1], OrderTurn(_leader, tuning));
    }
    break;
  }
  case LeaderOrder::PrepareWaypoint:
  {
    const std::optional<ActorRef> selected = region.SelectedLeaderRef(_context.self.team);
    if (!selected || selected->id != _leader.id)
    {
      _leader.order = LeaderOrder::None;
    }
    break;
  }
  case LeaderOrder::Follow:
    if (_leader.orderPosition != Neuron::Vec3{})
    {
      _leader.front = TurnedTowards(_leader.front, _leader.position, _leader.orderPosition, OrderTurn(_leader, tuning));
      const Neuron::Vec3 direction = DirectionOr(_leader.position, _leader.orderPosition, _leader.front);
      if (Neuron::Dot(_leader.front, direction) > tuning.ref.unit.leader.followFaceTolerance)
      {
        _leader.orderPosition = {};
      }
    }
    break;
  case LeaderOrder::None:
    break;
  }
}

} // namespace

void AssignWaypoint(Figure& _leader, const Neuron::Vec3& _target) noexcept
{
  _leader.leaderState = LeaderState::ToWaypoint;
  if (InFormationMode(_leader) && _target != _leader.position)
  {
    _leader.formationAngleSet = false;
  }
  _leader.waypoint = _target;
}

void FormUp(RegionSimulation& _region, const ActorRef& _leader, const Neuron::Vec3& _target)
{
  Figure& leader = *_region.Resolve(_leader);
  if (InFormationMode(leader))
  {
    leader.orderPosition = _target;
  }
  else
  {
    leader.orderPosition = {};
    leader.front = DirectionOr(leader.position, _target, leader.front);
  }
  leader.order = LeaderOrder::Follow;
  leader.formation = true;
  leader.formationAngleSet = false;
  _region.Emit({EffectEventKind::LeaderOrderFormation, _leader.team, leader.id, _target, {}, 0.0});
}

void ClearOrders(Figure& _leader) noexcept
{
  _leader.order = LeaderOrder::None;
  _leader.formation = false;
  _leader.waypoint = _leader.position;
}

bool IsToggleClick(const Figure& _leader, const Neuron::Vec3& _click, const TuningMvp& _tuning) noexcept
{
  return !_leader.noFormations && Neuron::Length(_click - _leader.position) < _tuning.ref.unit.leader.formationToggleRadius;
}

void AssignOrders(RegionSimulation& _region, const ActorRef& _leader, const Neuron::Vec3& _target, bool _straight)
{
  Figure& leader = *_region.Resolve(_leader);
  const bool toggle = IsToggleClick(leader, _target, _region.Tuning());
  if (InFormationMode(leader) || toggle)
  {
    // Steps 1 and 2: a toggle click moves one unit length towards the click; a formation move clears the selection.
    const bool formationMove = InFormationMode(leader);
    const Neuron::Vec3 point = toggle ? leader.position + DirectionOr(leader.position, _target, leader.front) : _target;
    AssignWaypoint(leader, point);
    FormUp(_region, _leader, point);
    if (formationMove)
    {
      _region.Select(_leader.team, std::nullopt);
    }
    return;
  }
  // Step 3: a waypoint order, which the Leader hands to the Workers that pass it; it does not walk.
  leader.order = LeaderOrder::Waypoint;
  leader.formation = false;
  leader.route = _region.BuildRoute(leader.position, _target, _straight);
  const MovementContext movement = _region.Movement();
  leader.orderPosition = PushFromObstacles(leader, _target, DrawPurpose::ShoreLeader, movement);
  _region.Emit({EffectEventKind::LeaderOrderWaypoint, _leader.team, leader.id, leader.orderPosition, {}, 0.0});
  _region.Select(_leader.team, std::nullopt);
}

bool LeaderWalking(const Figure& _leader, const TuningMvp& _tuning) noexcept
{
  return Neuron::Length(_leader.velocity) >= _tuning.ref.ctrl.leader.facePointerSpeed;
}

void LeaderBehavior(Figure& _leader, const FigureContext& _context)
{
  switch (_leader.leaderState)
  {
  case LeaderState::Idle:
    // R-UNIT-165.
    if (_leader.order != LeaderOrder::None)
    {
      _leader.leaderState = LeaderState::GivingOrders;
    }
    _leader.velocity = {};
    break;
  case LeaderState::ToWaypoint:
    ToWaypoint(_leader, _context);
    break;
  case LeaderState::GivingOrders:
    GivingOrders(_leader, _context);
    break;
  }
  // R-UNIT-168: the selected Leader that prepares a waypoint and is not walking faces its team's pointer.
  RegionSimulation& region = *_context.region;
  const TuningMvp& tuning = region.Tuning();
  if (_leader.order != LeaderOrder::PrepareWaypoint || LeaderWalking(_leader, tuning))
  {
    return;
  }
  const std::optional<ActorRef> selected = region.SelectedLeaderRef(_context.self.team);
  if (selected && selected->id == _leader.id)
  {
    const Neuron::Vec3& pointer = region.TeamAt(_context.self.team).pointer;
    _leader.front = TurnedTowards(_leader.front, _leader.position, pointer, OrderTurn(_leader, tuning));
  }
}

void LeaderOrderSteps(Figure& _leader, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  const TuningMvp& tuning = region.Tuning();
  const auto& leader = tuning.ref.unit.leader;
  // Step 5: an order marker every marker period of a waypoint order, counted from the last one.
  const std::uint64_t tick = region.Draws().Tick();
  if (_leader.order == LeaderOrder::Waypoint && tick - _leader.lastMarkerTick >= static_cast<std::uint64_t>(leader.markerPeriod))
  {
    CreateOrderMarker(region, _context.self.team, _leader.position + Neuron::Vec3{0.0, leader.markerLift, 0.0}, _leader.route);
    _leader.lastMarkerTick = tick;
  }
  // Step 6.
  if (_leader.formation)
  {
    FormationUpkeep(_leader, tuning);
  }
  // Step 8: the bot target front, while it is not walking.
  if (_leader.botTurn && !LeaderWalking(_leader, tuning))
  {
    _leader.front = TurnedTowards(_leader.front, _leader.position, _leader.position + _leader.botFront, leader.turnBot);
  }
  // Step 9: a new route from where the Leader is to the old route's end, when it is more than the distance from the old
  // route's start; the old route stays while anything holds it (DEC-651, O71).
  if (_leader.order != LeaderOrder::Waypoint)
  {
    return;
  }
  const std::vector<Neuron::Vec3>* points = region.Routes().Find(_leader.route);
  if (points != nullptr && !points->empty() && Neuron::Length(_leader.position - points->front()) > leader.routeRegenDistance)
  {
    const Neuron::Vec3 end = points->back();
    _leader.route = region.BuildRoute(_leader.position, end, false);
  }
}

} // namespace Species
