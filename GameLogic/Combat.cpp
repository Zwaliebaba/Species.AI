#include "pch.h"

#include "Combat.h"
#include "DrawPurpose.h"
#include "RegionSimulation.h"
#include "Weapons.h"

#include <cmath>
#include <limits>
#include <numbers>
#include <optional>

namespace Species
{

namespace
{

// The unit direction from _from to _to, rotated about the vertical by _angle: its (x, z) turns as (cos θ, sin θ) does
// (ADR-103's angle convention) and its y stays.
Neuron::Vec3 TurnedDirection(const Neuron::Vec3& _from, const Neuron::Vec3& _to, double _angle) noexcept
{
  const Neuron::Vec3 direction = Neuron::Normalized(_to - _from);
  const double c = std::cos(_angle);
  const double s = std::sin(_angle);
  return {direction.x * c - direction.z * s, direction.y, direction.x * s + direction.z * c};
}

// The position of the threat while it resolves: a live actor, or a grenade that still holds its slot.
std::optional<Neuron::Vec3> ThreatPosition(const Figure& _worker, const RegionSimulation& _region)
{
  switch (_worker.threat.kind)
  {
  case ThreatKind::Actor:
  {
    const Figure* actor = _region.Resolve({_worker.threat.id, _worker.threat.slot, _worker.threat.team});
    return actor != nullptr && !actor->dead ? std::optional<Neuron::Vec3>(actor->position) : std::nullopt;
  }
  case ThreatKind::Grenade:
  {
    const Effect* grenade = _region.GrenadeAt(_worker.threat.slot, _worker.threat.id);
    return grenade != nullptr ? std::optional<Neuron::Vec3>(grenade->position) : std::nullopt;
  }
  case ThreatKind::None:
    break;
  }
  return std::nullopt;
}

void ClearThreat(Figure& _worker) noexcept
{
  _worker.threat = {};
  _worker.threatSound = false; // the threat sounds stop
}

// R-UNIT-104: the grenade timer counts down every Fighting tick; when it expires it restarts and the Worker decides.
void GrenadeDecision(Figure& _worker, const Neuron::Vec3& _threat, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  const TuningMvp& tuning = region.Tuning();
  const auto& worker = tuning.ref.unit.worker;
  if (_worker.inFormation || _worker.threat.kind != ThreatKind::Actor)
  {
    return; // a formation member never throws, and the MVP throws only at actors (ADR-105)
  }
  --_worker.grenadeTimerTicks;
  if (_worker.grenadeTimerTicks > 0)
  {
    return;
  }
  const auto& interval = worker.grenadeIntervalTicks;
  _worker.grenadeTimerTicks = TicksOf(
    interval.base, region.Draws().For(DrawPurpose::GrenadeTimer, _worker.id).NextUnit(interval.spreadSeconds), tuning.ref.sim.tickPeriod);
  const Neuron::TerrainField& field = region.Terrain()->field;
  if (Neuron::Length(_threat - _worker.position) <= worker.grenadeMinDistance ||
      field.Normal(_worker.position.x, _worker.position.z).y <= worker.grenadeNormalMin ||
      field.Normal(_threat.x, _threat.z).y <= worker.grenadeNormalMin || _worker.position.y - _threat.y <= worker.grenadeHeightDiffMin)
  {
    return;
  }
  // Step 3 for the MVP's targets, Workers and Leaders: the crowd rule, counted over whole cells (DEC-094).
  const std::uint8_t team = _context.self.team;
  const ActorGrid& grid = region.Grid();
  if (grid.CountInCells(_threat.x, _threat.z, worker.grenadeCrowdRadius, RegionSimulation::EnemiesOf(team)) > worker.grenadeCrowdEnemies &&
      grid.CountInCells(_threat.x, _threat.z, worker.grenadeCrowdRadius, RegionSimulation::FriendsOf(team)) < worker.grenadeCrowdFriends)
  {
    ThrowGrenade(region, team, _worker.position, _threat);
  }
}

} // namespace

bool ThreatSearch(Figure& _worker, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  const auto& worker = region.Tuning().ref.unit.worker;
  const std::uint8_t team = _context.self.team;
  // Step 1: the range relaxes towards the threat range at every search; a formation member takes it whole.
  _worker.threatRange = _worker.inFormation
                          ? worker.threatRange
                          : worker.threatRange * worker.threatRangeRelax + _worker.threatRange * (1.0 - worker.threatRangeRelax);
  // Step 3: the nearest grenade of any team within the projectile threat range, in effect slot order (no Fury exists).
  const SlicedPool<Effect>& effects = region.Effects();
  std::optional<std::uint32_t> grenade;
  double nearestSquared = worker.projectileThreatRange * worker.projectileThreatRange;
  for (std::uint32_t slot = 0; slot < effects.Capacity(); ++slot)
  {
    if (!effects.Used(slot) || effects.At(slot).kind != EffectKind::Grenade)
    {
      continue;
    }
    const double squared = Neuron::LengthSquared(effects.At(slot).position - _worker.position);
    if (squared < nearestSquared)
    {
      nearestSquared = squared;
      grenade = slot;
    }
  }
  if (grenade)
  {
    const Effect& effect = effects.At(*grenade);
    const Threat threat{ThreatKind::Grenade, effect.team, *grenade, effect.id};
    _worker.workerState = WorkerState::Fighting;
    _worker.scared = true;
    if (threat != _worker.threat)
    {
      region.Emit({EffectEventKind::ThreatBark, team, _worker.id, _worker.position, {}, 1.0});
    }
    _worker.threat = threat;
    return true;
  }
  // Steps 7 and 8: the enemies in the cells of the search circle; the cached query's live candidates that are not
  // burning, and the nearest to the Worker's own position (DEC-094).
  const Neuron::Vec3 center = _worker.position;
  const double range = _worker.threatRange;
  std::optional<ActorRef> nearest;
  std::uint32_t enemies = 0;
  if (region.Grid().AnyInCells(center.x, center.z, range, RegionSimulation::EnemiesOf(team)))
  {
    double best = std::numeric_limits<double>::infinity();
    for (const ActorRef& ref : region.CachedEnemies(center.x, center.z, range, team))
    {
      const Figure* candidate = region.Resolve(ref);
      if (candidate == nullptr || candidate->dead ||
          (candidate->kind == FigureKind::Worker && candidate->workerState == WorkerState::Burning))
      {
        continue;
      }
      ++enemies;
      const double squared = Neuron::LengthSquared(candidate->position - _worker.position);
      if (squared < best)
      {
        best = squared;
        nearest = ref;
      }
    }
  }
  if (!nearest)
  {
    ClearThreat(_worker); // step 10
    return false;
  }
  // Step 9: Fighting; every MVP Worker is a soldier (R-UNIT-062), scared only by numbers with no friend near, and no
  // threat of the MVP is a heavy unit.
  const Threat threat{ThreatKind::Actor, nearest->team, nearest->slot, nearest->id};
  const Figure& target = *region.Resolve(*nearest);
  _worker.workerState = WorkerState::Fighting;
  _worker.scared = enemies > worker.scaredEnemyCount &&
                   !region.Grid().AnyInCells(_worker.position.x, _worker.position.z, range, RegionSimulation::FriendsOf(team));
  if (threat != _worker.threat && !_worker.threatSound)
  {
    region.Emit({EffectEventKind::ThreatBark, team, _worker.id, _worker.position, {}, _worker.scared ? 1.0 : 0.0});
    _worker.threatSound = true;
  }
  if (_worker.orderSet && !_worker.playerOrders && Neuron::Length(_worker.position - _worker.orderPosition) < worker.orderCancelRadius)
  {
    _worker.orderSet = false;
  }
  _worker.threatRange = Neuron::Length(target.position - _worker.position);
  _worker.threat = threat;
  return true;
}

void FightingStep(Figure& _worker, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  const TuningMvp& tuning = region.Tuning();
  const auto& worker = tuning.ref.unit.worker;
  // R-UNIT-101: a threat that is gone or dead ends the fight; the Leader of FollowingLeader comes with T7.
  const std::optional<Neuron::Vec3> threat = ThreatPosition(_worker, region);
  if (!threat)
  {
    _worker.workerState = WorkerState::Idle;
    _worker.retargetTimerTicks = 0;
    ClearThreat(_worker);
    return;
  }
  // R-UNIT-102: the movement, each waypoint turned by a draw within ±π/4, pushed with kill set and snapped to the ground.
  Neuron::SimRng& draw = region.Draws().For(DrawPurpose::CombatMove, _worker.id);
  const double distance = Neuron::Length(*threat - _worker.position);
  std::optional<Neuron::Vec3> waypoint;
  if (_worker.scared)
  {
    if (distance > worker.fearRange)
    {
      _worker.scared = false;
      ClearThreat(_worker);
    }
    else
    {
      waypoint = _worker.position + TurnedDirection(*threat, _worker.position, draw.NextSigned(std::numbers::pi / 2.0)) * worker.combatStep;
    }
  }
  else if (distance < worker.retreatRange && _worker.position.y - threat->y > worker.retreatHeightDiff)
  {
    waypoint = _worker.position + TurnedDirection(*threat, _worker.position, draw.NextSigned(std::numbers::pi / 2.0)) * worker.combatStep;
  }
  else
  {
    const Neuron::Vec3 towards = TurnedDirection(_worker.position, *threat, draw.NextSigned(std::numbers::pi / 2.0));
    waypoint = _worker.position + towards * (distance - (worker.standoffBase + draw.NextUnit(worker.standoffSpread)));
  }
  const MovementContext& movement = *_context.movement;
  if (waypoint)
  {
    Neuron::Vec3 pushed = WorkerPush(_worker, *waypoint, DrawPurpose::ShoreCombat, movement);
    pushed.y = movement.Height(pushed.x, pushed.z);
    _worker.waypoint = pushed;
  }
  static_cast<void>(WorkerMoveToWaypoint(_worker, false, movement));
  if (_worker.threat.kind != ThreatKind::Actor)
  {
    return; // only an actor threat is shot at, and the MVP throws only at actors
  }
  // R-UNIT-103 outside a formation: one fire roll in ⌊attack divisor ÷ s⌋ per tick, the region's s being 1.
  const auto divisor = static_cast<std::uint32_t>(std::floor(worker.attackDivisor));
  if (divisor >= 1 && region.Draws().For(DrawPurpose::FireRoll, _worker.id).NextUint() % divisor == 0)
  {
    FireAtPoint(_worker, *threat + Neuron::Vec3{0.0, worker.aimHeight, 0.0}, _context);
  }
  GrenadeDecision(_worker, *threat, _context);
}

void BurningStep(Figure& _worker, const FigureContext& _context)
{
  RegionSimulation& region = *_context.region;
  const auto& worker = region.Tuning().ref.unit.worker;
  if (!_worker.onGround)
  {
    return;
  }
  Neuron::SimRng& draw = region.Draws().For(DrawPurpose::BurnStep, _worker.id);
  const double x = draw.NextSigned(worker.burnWander);
  const double z = draw.NextSigned(worker.burnWander);
  const MovementContext& movement = *_context.movement;
  _worker.waypoint = WorkerPush(_worker, _worker.position + Neuron::Vec3{x, 0.0, z}, DrawPurpose::ShoreBurn, movement);
  static_cast<void>(WorkerMoveToWaypoint(_worker, false, movement));
  if (draw.NextUnit(worker.burnChance.rollRange) < worker.burnChance.threshold)
  {
    static_cast<void>(region.Damage(_context.self, -static_cast<std::int32_t>(worker.burnDamage), DamageKind::Unresistable));
  }
}

void FireAtPoint(Figure& _figure, const Neuron::Vec3& _target, const FigureContext& _context)
{
  if (_figure.reloadTimerTicks > 0)
  {
    return;
  }
  RegionSimulation& region = *_context.region;
  const auto& attack = region.Tuning().ref.unit.attack;
  Neuron::SimRng& draw = region.Draws().For(DrawPurpose::AimJitter, _figure.id);
  const double x = draw.NextSigned(attack.aimJitter);
  const double z = draw.NextSigned(attack.aimJitter);
  const Neuron::Vec3 aim{_target.x + x, _target.y, _target.z + z};
  const Neuron::Vec3 from = _figure.position + Neuron::Vec3{0.0, attack.muzzleHeight, 0.0};
  FireLaser(region, _context.self.team, from, Neuron::WithLength(aim - from, attack.laserSpeed));
  _figure.reloadTimerTicks = _figure.reloadIntervalTicks;
}

} // namespace Species
