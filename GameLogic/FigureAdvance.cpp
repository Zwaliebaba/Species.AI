#include "pch.h"

#include "DrawPurpose.h"
#include "FigureAdvance.h"

#include <cmath>
#include <cstdint>
#include <numbers>

namespace Species
{

namespace
{

constexpr double TWO_PI = 2.0 * std::numbers::pi;

// R-UNIT-087 for the searches the MVP builds so far: Idle ends in the random-position search (R-UNIT-097), which
// always succeeds; OnWaypointOrder's searches (threats, ports, carryables) arrive with T6 or never.
void TaskSearch(Figure& _figure, const MovementContext& _context)
{
  if (_figure.workerState != WorkerState::Idle)
  {
    return;
  }
  const auto& worker = _context.tuning->ref.unit.worker;
  Neuron::SimRng& draw = _context.draws->For(DrawPurpose::Wander, _figure.id);
  const double chance = draw.NextUnit(1.0);
  if (chance < worker.wanderChance)
  {
    const double angle = draw.NextSigned(TWO_PI);
    const Neuron::Vec3 point{_figure.position.x + std::cos(angle) * worker.wanderDistance, _figure.position.y,
                             _figure.position.z + std::sin(angle) * worker.wanderDistance};
    _figure.waypoint = WorkerPush(_figure, point, DrawPurpose::ShoreWander, _context);
  }
}

} // namespace

bool BaseAdvance(Figure& _figure, const MovementContext& _context)
{
  const TuningMvp& tuning = *_context.tuning;
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
  if (_figure.kind != FigureKind::Worker && _figure.inWaterTicks >= 0 && SwimStep(_figure, _context))
  {
    static_cast<void>(
      ApplyHealthChange(_figure, -static_cast<std::int32_t>(tuning.ref.sim.water.drownDamage), DamageKind::Unresistable, tuning));
  }
  WorldClamp(_figure, _context);
  return false;
}

bool WorkerAdvance(Figure& _figure, const MovementContext& _context)
{
  const TuningMvp& tuning = *_context.tuning;
  const auto& worker = tuning.ref.unit.worker;
  const double dt = tuning.ref.sim.tickPeriod;
  if (BaseAdvance(_figure, _context))
  {
    return true;
  }
  if (!_figure.enabled)
  {
    return false;
  }
  if (!_figure.dead && _figure.onGround)
  {
    --_figure.retargetTimerTicks;
    if (_figure.retargetTimerTicks <= 0)
    {
      TaskSearch(_figure, _context);
      // R-UNIT-065 step 3.1: 11 + ⌊NextUnit(1) ÷ tick period⌋ ticks, 11 to 20.
      const double spread = _context.draws->For(DrawPurpose::RetargetTimer, _figure.id).NextUnit(worker.retargetTicks.spreadSeconds);
      _figure.retargetTimerTicks =
        static_cast<std::int32_t>(worker.retargetTicks.base) + static_cast<std::int32_t>(std::floor(spread / dt));
    }
    switch (_figure.workerState)
    {
    case WorkerState::Idle:
      static_cast<void>(WorkerMoveToWaypoint(_figure, true, _context));
      break;
    case WorkerState::OnWaypointOrder:
      if (WorkerMoveToWaypoint(_figure, false, _context))
      {
        // R-UNIT-073: arrival without a teleport structure.
        _figure.workerState = WorkerState::Idle;
        _figure.retargetTimerTicks = 0;
        _figure.orderSet = false;
        _figure.playerOrders = false;
      }
      break;
    case WorkerState::Drowning:
      break;
    }
  }
  if (!_figure.onGround)
  {
    AirborneStep(_figure, _context);
  }
  if (_figure.workerState == WorkerState::Drowning && !_figure.dead)
  {
    // R-UNIT-080: an unresistable change that kills; its Essence orb comes with the orbs (M1 plan T6).
    static_cast<void>(ApplyHealthChange(_figure, -static_cast<std::int32_t>(worker.drownDamage), DamageKind::Unresistable, tuning));
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

bool LeaderAdvance(Figure& _figure, const MovementContext& _context)
{
  const TuningMvp& tuning = *_context.tuning;
  if (_figure.enabled && !_figure.onGround)
  {
    AirborneStep(_figure, _context);
  }
  if (BaseAdvance(_figure, _context))
  {
    return true;
  }
  if (!_figure.enabled)
  {
    return false;
  }
  if (_figure.inWaterTicks >= 0 && SwimStep(_figure, _context))
  {
    static_cast<void>(
      ApplyHealthChange(_figure, -static_cast<std::int32_t>(tuning.ref.sim.water.drownDamage), DamageKind::Unresistable, tuning));
  }
  if (_figure.onGround && !_figure.dead)
  {
    _figure.position.y = _context.Height(_figure.position.x, _figure.position.z);
  }
  if (!_figure.dead && _figure.onGround && _figure.inWaterTicks < 0)
  {
    switch (_figure.leaderState)
    {
    case LeaderState::Idle:
      _figure.velocity = {};
      break;
    case LeaderState::ToWaypoint:
      if (LeaderMoveToWaypoint(_figure, _context))
      {
        _figure.leaderState = LeaderState::Idle;
      }
      break;
    }
  }
  if (_figure.dead)
  {
    _figure.velocity.y -= tuning.ref.unit.leader.corpseSinkPerTick;
  }
  return false;
}

bool AdvanceFigure(Figure& _figure, const MovementContext& _context)
{
  return _figure.kind == FigureKind::Worker ? WorkerAdvance(_figure, _context) : LeaderAdvance(_figure, _context);
}

void GiveWaypointOrder(Figure& _worker, const Neuron::Vec3& _target, const MovementContext& _context)
{
  const auto& worker = _context.tuning->ref.unit.worker;
  _worker.orderPosition = _target;
  _worker.orderSet = true;
  Neuron::SimRng& draw = _context.draws->For(DrawPurpose::OrderDisc, _worker.id);
  const double radius = draw.NextUnit(worker.orderDiscRadius);
  const double angle = draw.NextUnit(TWO_PI);
  const Neuron::Vec3 point{_target.x + std::cos(angle) * radius, _target.y, _target.z + std::sin(angle) * radius};
  _worker.waypoint = WorkerPush(_worker, point, DrawPurpose::ShoreOrder, _context);
  _worker.workerState = WorkerState::OnWaypointOrder;
}

void SendLeaderTo(Figure& _leader, const Neuron::Vec3& _waypoint) noexcept
{
  _leader.waypoint = _waypoint;
  _leader.leaderState = LeaderState::ToWaypoint;
}

} // namespace Species
