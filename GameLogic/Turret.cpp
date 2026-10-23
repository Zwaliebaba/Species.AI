#include "pch.h"

#include "DrawPurpose.h"
#include "RegionSimulation.h"
#include "Settlement.h"
#include "Turret.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <vector>

namespace Species
{

namespace
{

constexpr double FULL_TURN_RAD = 6.283185307179586;
constexpr Neuron::Vec3 UP{0.0, 1.0, 0.0};
// R-BLDG-404: the direction normalised from a zero vector.
constexpr Neuron::Vec3 DEFAULT_FRONT{0.0, 0.0, 1.0};

Neuron::Vec3 HorizontalUnit(const Neuron::Vec3& _v) noexcept
{
  return Neuron::NormalizedOr({_v.x, 0.0, _v.z}, DEFAULT_FRONT);
}

// The right of a horizontal front, with +y up: up × front, as a structure's (Structure.cpp).
Neuron::Vec3 RightOf(const Neuron::Vec3& _front) noexcept
{
  return {_front.z, 0.0, -_front.x};
}

Neuron::Vec3 CrossProduct(const Neuron::Vec3& _a, const Neuron::Vec3& _b) noexcept
{
  return {_a.y * _b.z - _a.z * _b.y, _a.z * _b.x - _a.x * _b.z, _a.x * _b.y - _a.y * _b.x};
}

// A horizontal vector turned about the vertical by _angle: +z turns towards +x for a positive angle.
Neuron::Vec3 TurnedBy(const Neuron::Vec3& _v, double _angle) noexcept
{
  const double cosine = std::cos(_angle);
  const double sine = std::sin(_angle);
  return {_v.x * cosine + _v.z * sine, _v.y, _v.z * cosine - _v.x * sine};
}

// The signed horizontal angle that TurnedBy turns the horizontal unit vector _from by to reach _to.
double SignedAngle(const Neuron::Vec3& _from, const Neuron::Vec3& _to) noexcept
{
  return std::atan2(_from.z * _to.x - _from.x * _to.z, _from.x * _to.x + _from.z * _to.z);
}

// The segment from _origin along the unit vector _direction for _length passes strictly within _radius of _center.
bool SegmentNear(const Neuron::Vec3& _origin, const Neuron::Vec3& _direction, double _length, const Neuron::Vec3& _center,
                 double _radius) noexcept
{
  const double along = std::clamp(Neuron::Dot(_center - _origin, _direction), 0.0, std::max(_length, 0.0));
  return Neuron::LengthSquared(_center - (_origin + _direction * along)) < _radius * _radius;
}

// R-BLDG-408: the operators of the ports per team; the candidate is the first team in slot order with an operator,
// replaced by any later team with strictly more; no team when an operator is no friend of the candidate's, or when
// there is no operator.
std::optional<std::uint8_t> OwnershipOf(const RegionSimulation& _region, const Structure& _structure)
{
  std::array<std::uint32_t, TEAM_SLOTS_MAX> operators{};
  for (const StructurePort& port : _structure.ports)
  {
    if (port.occupant)
    {
      ++operators[port.occupant.value_or(ActorRef{}).team];
    }
  }
  std::optional<std::uint8_t> candidate;
  for (std::size_t team = 0; team < operators.size(); ++team)
  {
    if (operators[team] > 0 && (!candidate || operators[team] > operators[candidate.value_or(0)]))
    {
      candidate = static_cast<std::uint8_t>(team);
    }
  }
  if (!candidate)
  {
    return std::nullopt;
  }
  const TeamMask enemies = _region.EnemiesOf(candidate.value_or(0));
  for (std::size_t team = 0; team < operators.size(); ++team)
  {
    if (operators[team] > 0 && enemies.test(team))
    {
      return std::nullopt;
    }
  }
  return candidate;
}

// R-BLDG-411's search of a turret of every type but the mortar: the enemies within the maximum range in result order,
// skipping the dead, the nearest at no less than the minimum range with a clear line of sight; the first on equal
// distance. The MVP has no bot controller or Collector to skip.
std::optional<ActorRef> SearchTarget(const RegionSimulation& _region, const Structure& _structure, std::uint8_t _team)
{
  const auto& gun = _region.Tuning().ref.bldg.turret.gun;
  std::optional<ActorRef> best;
  double bestDistance = 0.0;
  for (const ActorRef& ref : _region.InRange(_structure.position.x, _structure.position.z, gun.maxRange, _region.EnemiesOf(_team)))
  {
    const Figure* candidate = _region.Resolve(ref);
    if (candidate == nullptr || candidate->dead)
    {
      continue;
    }
    const double distance = Neuron::Length(candidate->position - _structure.position);
    if (distance < gun.minRange || (best && distance >= bestDistance) ||
        !TurretSightClear(_region, _structure, _structure.position, candidate->position))
    {
      continue;
    }
    best = ref;
    bestDistance = distance;
  }
  return best;
}

// R-BLDG-412: a search without a target turns the aim direction about the vertical by U[-sweep_turn / 2,
// sweep_turn / 2) and moves the aim height by U[-sweep_height_change / 2, sweep_height_change / 2), in that order.
void Sweep(RegionSimulation& _region, const Structure& _structure, TurretState& _turret)
{
  const auto& keys = _region.Tuning().ref.bldg.turret;
  Neuron::SimRng& draw = _region.Draws().For(DrawPurpose::TurretSweep, _structure.id);
  const double turn = draw.NextSigned(keys.sweepTurn);
  const double rise = draw.NextSigned(keys.sweepHeightChange);
  const Neuron::Vec3 offset = _turret.aimPoint - _structure.position;
  const Neuron::Vec3 turned = TurnedBy({offset.x, 0.0, offset.z}, turn);
  _turret.aimPoint = _structure.position + Neuron::Vec3{turned.x, offset.y + rise, turned.z};
}

// R-BLDG-418 and R-BLDG-420 for the gun turret: nothing while the fire timer is above 0; otherwise the barrels from the
// cursor, ref.bldg.turret.barrels_per_fire_step of them, the cursor passing each; each whose port has an occupant fires
// a round from its muzzle along the barrels' front, half a tick of travel behind the muzzle, and sets the fire timer.
// The three draws of step 1 serve only the mortar turret, so the gun turret does not make them (ADR-133).
void FireStep(RegionSimulation& _region, Structure& _structure, TurretState& _turret, std::uint8_t _team)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto& keys = tuning.ref.bldg.turret;
  const auto barrels = static_cast<std::uint32_t>(_structure.ports.size());
  if (_turret.fireTicks > 0 || barrels == 0)
  {
    return;
  }
  for (std::int64_t examined = 0; examined < keys.barrelsPerFireStep; ++examined)
  {
    const std::uint32_t barrel = _turret.barrelCursor % barrels;
    _turret.barrelCursor = static_cast<std::uint8_t>((barrel + 1) % barrels);
    if (!_structure.ports[barrel].occupant)
    {
      continue;
    }
    const Neuron::Vec3 velocity = BarrelFront(_turret) * keys.gun.roundSpeed;
    const Neuron::Vec3 muzzle = MuzzleOf(_structure, _region.Records(), barrel);
    const Neuron::Vec3 start = muzzle - velocity * (tuning.ref.sim.tickPeriod * keys.gun.startBackFraction);
    FireTurretRound(_region, _team, start, velocity, _structure.id, muzzle, barrel);
    _turret.fireTicks = static_cast<std::int32_t>(keys.gun.reloadTicks);
  }
}

// The head's barrel mount in the world, where the barrels pitch: its socket at the head's turn, or the turret's
// position without the records.
Neuron::Vec3 MountOf(const Structure& _structure, const StructureRecords* _records, const TurretState& _turret)
{
  const std::vector<const Neuron::SimulationSocket*> mounts = _records != nullptr
                                                                ? Neuron::SocketsOf(_records->turretHead, Neuron::SocketKind::BarrelMount)
                                                                : std::vector<const Neuron::SimulationSocket*>{};
  if (mounts.empty())
  {
    return _structure.position;
  }
  const std::array<double, 3> mount = Neuron::RestPositionOf(_records->turretHead, *mounts.front());
  return _structure.position + RightOf(_turret.headFront) * mount[0] + UP * mount[1] + _turret.headFront * mount[2];
}

// R-BLDG-416: an aim point nearer than the minimum aim distance moves out to it; the head turns by the yaw gain × the
// horizontal angle to the aim point, and the barrels' up vector blends towards the aim direction, from the barrel mount
// (ADR-133), by the pitch blend and is kept of unit length.
void Turn(const RegionSimulation& _region, const Structure& _structure, TurretState& _turret)
{
  const auto& keys = _region.Tuning().ref.bldg.turret;
  const Neuron::Vec3 offset = _turret.aimPoint - _structure.position;
  if (Neuron::Length(offset) < keys.minAimDistance)
  {
    _turret.aimPoint = _structure.position + Neuron::NormalizedOr(offset, DEFAULT_FRONT) * keys.minAimDistance;
  }
  const Neuron::Vec3 aim = _turret.aimPoint - _structure.position;
  _turret.headFront = HorizontalUnit(TurnedBy(_turret.headFront, keys.yawGain * SignedAngle(_turret.headFront, HorizontalUnit(aim))));
  const Neuron::Vec3 fromMount = _turret.aimPoint - MountOf(_structure, _region.Records(), _turret);
  const Neuron::Vec3 blended =
    _turret.barrelUp * (1.0 - keys.pitchBlend) + Neuron::NormalizedOr(fromMount, _turret.headFront) * keys.pitchBlend;
  _turret.barrelUp = Neuron::NormalizedOr(blended, _turret.headFront);
}

} // namespace

Neuron::Vec3 BarrelFront(const TurretState& _turret) noexcept
{
  const double horizontal = std::sqrt(_turret.barrelUp.x * _turret.barrelUp.x + _turret.barrelUp.z * _turret.barrelUp.z);
  return Neuron::NormalizedOr(_turret.headFront * horizontal + UP * _turret.barrelUp.y, _turret.headFront);
}

Neuron::Vec3 MuzzleOf(const Structure& _structure, const StructureRecords* _records, std::uint32_t _barrel)
{
  if (_records == nullptr || !_structure.turret)
  {
    return _structure.position;
  }
  const TurretState& turret = _structure.turret.value_or(TurretState{});
  const std::vector<const Neuron::SimulationSocket*> muzzles = Neuron::SocketsOf(_records->turretBarrel, Neuron::SocketKind::Muzzle);
  const std::array<double, 3> muzzle =
    _barrel < muzzles.size() ? Neuron::RestPositionOf(_records->turretBarrel, *muzzles[_barrel]) : std::array<double, 3>{};
  // The head turns about the vertical; the barrels pitch about the head's right at the barrel mount.
  const Neuron::Vec3 right = RightOf(turret.headFront);
  const Neuron::Vec3 barrelFront = BarrelFront(turret);
  const Neuron::Vec3 barrelUp = CrossProduct(barrelFront, right);
  return MountOf(_structure, _records, turret) + right * muzzle[0] + barrelUp * muzzle[1] + barrelFront * muzzle[2];
}

bool TurretSightClear(const RegionSimulation& _region, const Structure& _turret, const Neuron::Vec3& _from, const Neuron::Vec3& _to)
{
  const Neuron::Vec3 line = _to - _from;
  const double distance = Neuron::Length(line);
  if (distance <= 0.0)
  {
    return true;
  }
  const Neuron::Vec3 direction = line * (1.0 / distance);
  const auto samples = static_cast<std::int64_t>(std::floor(distance / _region.Tuning().ref.bldg.turret.losStep));
  const ObstacleGrid& obstacles = _region.Obstacles();
  std::vector<std::uint32_t> tested;
  const auto blocked = [&](const Neuron::Vec3& _sample)
  {
    for (const std::uint32_t index : obstacles.StructuresAt(_sample.x, _sample.z))
    {
      if (std::ranges::find(tested, index) != tested.end())
      {
        continue;
      }
      tested.push_back(index);
      const ObstacleShape& shape = obstacles.Shape(index);
      const bool self = shape.structure && _region.Structures().Used(shape.structure.value_or(0)) &&
                        _region.Structures().At(shape.structure.value_or(0)).id == _turret.id;
      if (!self && RayHits(shape, _from, direction, distance))
      {
        return true;
      }
    }
    return false;
  };
  if (samples == 0)
  {
    return !blocked(_from) && !blocked(_to);
  }
  for (std::int64_t k = 0; k <= samples; ++k)
  {
    if (blocked(_from + line * (static_cast<double>(k) / static_cast<double>(samples))))
    {
      return false;
    }
  }
  return true;
}

void AdvanceTurret(RegionSimulation& _region, Structure& _structure)
{
  if (!_structure.turret)
  {
    return;
  }
  TurretState& turret = *_structure.turret;
  const TuningMvp& tuning = _region.Tuning();
  const auto& keys = tuning.ref.bldg.turret;
  // R-BLDG-412: the first idle aim point, drawn once at the turret's initialisation, which is its first advance as a
  // built turret (ADR-133): an angle, a distance and a height, in that order.
  if (!turret.swept)
  {
    Neuron::SimRng& draw = _region.Draws().For(DrawPurpose::TurretSweep, _structure.id);
    const double angle = draw.NextUnit(FULL_TURN_RAD);
    const double distance = keys.sweepDistanceBase + draw.NextUnit(keys.sweepDistanceSpread);
    const double height = draw.NextUnit(keys.sweepHeightFirst);
    turret.aimPoint = _structure.position + Neuron::Vec3{std::sin(angle) * distance, height, std::cos(angle) * distance};
    turret.swept = true;
  }
  // Step 2: the ownership evaluation.
  if (--turret.recountTicks <= 0)
  {
    turret.recountTicks = static_cast<std::int32_t>(keys.recountTicks);
    const std::optional<std::uint8_t> team = OwnershipOf(_region, _structure);
    if (team != turret.team)
    {
      turret.team = team;
      turret.target.reset();
      turret.hoseTicks = 0;
    }
  }
  if (!turret.team)
  {
    return; // a turret with no team only evaluates its ownership and its ports
  }
  const std::uint8_t team = turret.team.value_or(0);
  // Step 3: the fire timer.
  turret.fireTicks = std::max(turret.fireTicks - 1, 0);
  // Step 4: targeting, aiming and the automatic fire decision.
  if (--turret.searchTicks <= 0)
  {
    turret.searchTicks = static_cast<std::int32_t>(keys.recountTicks);
    const std::optional<ActorRef> found = SearchTarget(_region, _structure, team);
    if (found && found != turret.target)
    {
      const Figure* sighted = _region.Resolve(found.value_or(ActorRef{}));
      _region.Emit(
        {EffectEventKind::TurretTargetSighted, team, _structure.id, sighted != nullptr ? sighted->position : _structure.position, {}, 0.0});
    }
    turret.target = found;
    if (!found)
    {
      Sweep(_region, _structure, turret);
    }
  }
  const Figure* target = turret.target ? _region.Resolve(turret.target.value_or(ActorRef{})) : nullptr;
  if (target != nullptr && target->dead)
  {
    target = nullptr;
  }
  if (target == nullptr)
  {
    turret.target.reset();
  }
  if (target != nullptr)
  {
    // R-BLDG-414: the wobble over match time.
    const double seconds = static_cast<double>(_region.Draws().Tick()) * tuning.ref.sim.tickPeriod;
    const Neuron::Vec3 wobble{keys.wobbleXz * std::sin(keys.wobbleRateXz * seconds),
                              std::abs(keys.wobbleY * std::sin(keys.wobbleRateY * seconds)),
                              keys.wobbleXz * std::cos(keys.wobbleRateXz * seconds)};
    turret.aimPoint = target->position + wobble;
    turret.hoseTicks = static_cast<std::int32_t>(keys.hoseWindowTicks);
    // R-BLDG-415: the horizontal angle between the head's front and the aim direction, its cosine clamped (DEC-653).
    const double cosine = std::clamp(Neuron::Dot(turret.headFront, HorizontalUnit(turret.aimPoint - _structure.position)), -1.0, 1.0);
    if (std::acos(cosine) < keys.fireCone)
    {
      FireStep(_region, _structure, turret, team);
    }
  }
  else if (turret.hoseTicks > 0)
  {
    --turret.hoseTicks;
    FireStep(_region, _structure, turret, team);
  }
  // Step 6: aim clamping and turning. The flame turret's guard and the overheat presentation (steps 7 and 8) are not
  // the gun turret's simulation.
  Turn(_region, _structure, turret);
}

void DamageTurret(RegionSimulation& _region, Structure& _structure, std::int32_t _amount)
{
  if (_structure.kind != StructureKind::Turret || !_structure.built || !_structure.turret)
  {
    return;
  }
  TurretState& turret = *_structure.turret;
  turret.health += _amount;
  if (turret.health > 0)
  {
    return;
  }
  // R-BLDG-423: the head and barrels shatter; MVP-034: the turret leaves its construction site, which its settlement
  // may rebuild at a fraction of its cost (RebuildTurret).
  _region.Emit({EffectEventKind::TurretDestroyed, turret.team.value_or(0), _structure.id, _structure.position, {}, 0.0});
  const auto& cost = _region.Tuning().mvp.cost;
  _structure.built = false;
  _structure.workTicks = 0;
  _structure.costMilliRu = MilliRuOf(cost.turretRu * cost.turretRebuildFraction);
  _structure.turret = NewTurret(_structure, _region.Tuning());
}

void FireTurretRound(RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position, const Neuron::Vec3& _velocity,
                     StableId _source, const Neuron::Vec3& _muzzle, std::uint32_t _barrel)
{
  Effect round;
  round.id = _region.IssueId();
  round.kind = EffectKind::TurretRound;
  round.team = _team;
  round.position = _position;
  round.velocity = _velocity;
  round.fuseTicks = static_cast<std::int32_t>(_region.Tuning().ref.weapon.turretRound.lifeTicks);
  round.source = _source;
  static_cast<void>(_region.Effects().Take(round));
  _region.Emit({EffectEventKind::TurretFired, _team, round.id, _muzzle, _velocity, static_cast<double>(_barrel)});
}

bool AdvanceTurretRound(RegionSimulation& _region, Effect& _round)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto& keys = tuning.ref.weapon.turretRound;
  --_round.fuseTicks;
  const Neuron::Vec3 previous = _round.position;
  const Neuron::Vec3 step = _round.velocity * tuning.ref.sim.tickPeriod;
  const Neuron::Vec3 next = previous + step;
  _round.position = next;
  const double length = Neuron::Length(step);
  const Neuron::Vec3 direction = Neuron::NormalizedOr(step, DEFAULT_FRONT);
  const Neuron::Vec3 middle = (previous + next) * 0.5;
  // Step 1: every enemy within the step's length of its middle that the step's ray hits takes the round's damage, so
  // the round pierces. A Worker is a sphere of ref.sim.ray.sprite_radius (R-SIM-387); a Leader's is its radius, when
  // larger (ADR-133).
  std::uint32_t kills = 0;
  for (const ActorRef& ref : _region.InRange(middle.x, middle.z, length, _region.EnemiesOf(_round.team)))
  {
    Figure* victim = _region.Resolve(ref);
    if (victim == nullptr || victim->dead ||
        !SegmentNear(previous, direction, length, victim->position, std::max(tuning.ref.sim.ray.spriteRadius, victim->radius)))
    {
      continue;
    }
    const bool burning = victim->kind == FigureKind::Worker && victim->workerState == WorkerState::Burning;
    const HealthChange change = _region.Damage(ref, -static_cast<std::int32_t>(keys.damage), DamageKind::TurretRound);
    // Step 2: a kill counts 1, or 0 for a Worker already burning, for the round's team.
    if (change.killed)
    {
      kills += burning ? 0U : 1U;
      _region.Emit({EffectEventKind::LaserKill, _round.team, victim->id, victim->position, {}, 0.0});
    }
    // Step 3: an actor on the ground is pushed away from the round with a jitter, to the push's length, and becomes
    // airborne; a Leader takes no knock-back, as from an explosion.
    if (change.registered && victim->onGround && victim->kind != FigureKind::Leader)
    {
      Neuron::SimRng& draw = _region.Draws().For(DrawPurpose::RoundPush, _round.id);
      Neuron::Vec3 away = victim->position - _round.position;
      away.x += draw.NextSigned(keys.pushJitter);
      away.y += draw.NextSigned(keys.pushJitter);
      away.z += draw.NextSigned(keys.pushJitter);
      victim->velocity = victim->velocity + Neuron::WithLength(away, keys.push);
      victim->onGround = false;
    }
  }
  _region.TeamAt(_round.team).kills += kills;
  // Step 4: the step's ray meeting a structure of its end's obstacle-grid cell, other than the turret that fired it,
  // which its first steps leave (ADR-133), ends the round.
  const ObstacleGrid& obstacles = _region.Obstacles();
  for (const std::uint32_t index : obstacles.StructuresAt(next.x, next.z))
  {
    const ObstacleShape& shape = obstacles.Shape(index);
    const bool source = shape.structure && _region.Structures().Used(shape.structure.value_or(0)) &&
                        _region.Structures().At(shape.structure.value_or(0)).id == _round.source;
    if (!source && RayHits(shape, previous, direction, length))
    {
      _region.Emit({EffectEventKind::TurretRoundHit, _round.team, _round.id, next, _round.velocity, 1.0});
      return true;
    }
  }
  // Step 5: the end or the middle of the step below the terrain ends it at the ray's terrain hit.
  const Neuron::TerrainField& field = _region.Terrain()->field;
  if (next.y < field.Height(next.x, next.z) || middle.y < field.Height(middle.x, middle.z))
  {
    const std::optional<Neuron::Vec3> hit = field.CastRay(previous, step);
    _region.Emit({EffectEventKind::TurretRoundHit, _round.team, _round.id, hit.value_or(next), _round.velocity, 0.0});
    return true;
  }
  // Step 6: its life spent, or off the map.
  return _round.fuseTicks <= 0 || !_region.Movement().OnMap(next.x, next.z);
}

} // namespace Species
