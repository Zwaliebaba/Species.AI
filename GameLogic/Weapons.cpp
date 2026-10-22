#include "pch.h"

#include "DrawPurpose.h"
#include "OrderMarkers.h"
#include "RegionSimulation.h"
#include "StateValues.h"
#include "Turret.h"
#include "Weapons.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace Species
{

namespace
{

constexpr std::uint64_t EFFECT_KIND_MAX_BEFORE_MARKERS = static_cast<std::uint64_t>(EffectKind::ShockwaveRing);
constexpr std::uint64_t EFFECT_KIND_MAX_BEFORE_ROUNDS = static_cast<std::uint64_t>(EffectKind::OrderTrailMark);
// The first state version whose effects hold a marker's route (M1 task T7), and the first that has turret rounds (M2
// task T5).
constexpr std::uint32_t EFFECT_STATE_VERSION_MARKERS = 7;
constexpr std::uint32_t EFFECT_STATE_VERSION_ROUNDS = 9;

// The horizontal distance from _point to the segment from _a to _b (R-CRATE-441 step 7's 2-D distance).
double SegmentDistance2D(const Neuron::Vec3& _point, const Neuron::Vec3& _a, const Neuron::Vec3& _b) noexcept
{
  const double dx = _b.x - _a.x;
  const double dz = _b.z - _a.z;
  const double lengthSquared = dx * dx + dz * dz;
  double t = 0.0;
  if (lengthSquared > 0.0)
  {
    t = std::clamp(((_point.x - _a.x) * dx + (_point.z - _a.z) * dz) / lengthSquared, 0.0, 1.0);
  }
  const double px = _a.x + dx * t - _point.x;
  const double pz = _a.z + dz * t - _point.z;
  return std::sqrt(px * px + pz * pz);
}

// The reflection of _velocity about the unit normal _normal.
Neuron::Vec3 Reflect(const Neuron::Vec3& _velocity, const Neuron::Vec3& _normal) noexcept
{
  return _velocity - _normal * (2.0 * Neuron::Dot(_velocity, _normal));
}

TeamMask AllTeams() noexcept
{
  return TeamMask{}.set();
}

// R-CRATE-442 and R-CRATE-443 for a bolt that hit the figure _ref names: the damage, the kill count, and the push.
void HitByLaser(RegionSimulation& _region, const LaserBolt& _bolt, const ActorRef& _ref, Figure& _victim)
{
  const auto& laser = _region.Tuning().ref.weapon.laser;
  Neuron::SimRng& draw = _region.Draws().For(DrawPurpose::LaserHit, _bolt.id);
  const auto amount = static_cast<std::int32_t>(std::floor(static_cast<double>(laser.damageBase) + draw.NextUnit(laser.damageSpread)));
  const bool worker = _victim.kind == FigureKind::Worker;
  const HealthChange change = _region.Damage(_ref, -amount, DamageKind::Laser);
  if (change.killed && worker)
  {
    ++_region.TeamAt(_bolt.team).kills;
    _region.Emit({EffectEventKind::LaserKill, _bolt.team, _victim.id, _victim.position, {}, 0.0});
  }
  if (change.registered)
  {
    // The push along the bolt; it sets the victim's front, as a Trooper's and a Crawler's would not.
    Neuron::Vec3 push = Neuron::Normalized(_bolt.velocity) * draw.NextUnit(laser.pushSpread);
    push.y = laser.pushLiftBase + draw.NextUnit(laser.pushLiftSpread);
    _victim.front = Neuron::Normalized(push);
    _victim.velocity = _victim.velocity + push;
  }
}

// R-CRATE-411 with R-SIM-385's bounce: gravity, the move, and a bounce that reflects about the terrain normal with the
// restitution and scales by the bounce force.
void ThrownStep(RegionSimulation& _region, Effect& _thrown)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto& thrown = tuning.ref.weapon.thrown;
  const double dt = tuning.ref.sim.tickPeriod;
  if (_thrown.force <= thrown.restForce)
  {
    return;
  }
  const Neuron::TerrainField& field = _region.Terrain()->field;
  _thrown.velocity.y -= tuning.ref.sim.physics.thrownGravity * dt;
  _thrown.position = _thrown.position + _thrown.velocity * dt;
  const double ground = field.Height(_thrown.position.x, _thrown.position.z);
  if (_thrown.position.y < ground + thrown.bounceClearance)
  {
    const Neuron::Vec3 normal = field.Normal(_thrown.position.x, _thrown.position.z);
    _thrown.position.y = ground;
    _thrown.velocity = Reflect(_thrown.velocity, normal) * tuning.ref.sim.physics.restitution;
    _thrown.force *= thrown.bounceForceFactor;
    _thrown.velocity = _thrown.velocity * _thrown.force;
    _thrown.position.y = std::max(_thrown.position.y, ground + thrown.bounceClearance);
    _region.Emit({EffectEventKind::GrenadeBounced, _thrown.team, _thrown.id, _thrown.position, {}, 0.0});
  }
}

// R-CRATE-470, R-CRATE-471: one active tick of a ring, or its removal after the last.
bool RingStep(RegionSimulation& _region, Effect& _ring)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto& shockwave = tuning.ref.weapon.shockwave;
  const double dt = tuning.ref.sim.tickPeriod;
  const auto activeTicks = static_cast<std::int32_t>(std::ceil(_ring.sizeSeconds / dt)) - 1;
  ++_ring.activeTick;
  if (_ring.activeTick > activeTicks)
  {
    return true;
  }
  const double k = _ring.activeTick;
  const double radius = shockwave.startRadius + shockwave.growth * k * dt;
  const double strength = 1.0 - k * dt / _ring.sizeSeconds;
  std::uint32_t kills = 0;
  for (const ActorRef& ref : _region.InRange(_ring.position.x, _ring.position.z, radius, AllTeams()))
  {
    Figure* victim = _region.Resolve(ref);
    if (victim == nullptr || std::abs(Neuron::Length(victim->position - _ring.position) - radius) >= shockwave.band)
    {
      continue;
    }
    if (victim->kind == FigureKind::Worker &&
        _region.Draws().For(DrawPurpose::RingIgnite, _ring.id).NextUnit(shockwave.igniteDrawRange) < shockwave.igniteThreshold)
    {
      _region.SetOnFire(ref);
      continue;
    }
    const HealthChange change = _region.Damage(ref, -static_cast<std::int32_t>(shockwave.damage * strength), DamageKind::Explosive);
    if (change.killed && ref.team != _ring.team && victim->kind == FigureKind::Worker)
    {
      ++kills;
    }
    if (change.registered && victim->onGround && victim->kind != FigureKind::Leader)
    {
      Neuron::Vec3 away = victim->position - _ring.position;
      away.y = shockwave.pushLift;
      victim->velocity = victim->velocity + Neuron::Normalized(away) * (shockwave.push * strength);
      victim->onGround = false;
    }
  }
  _region.TeamAt(_ring.team).kills += kills;
  return false;
}

} // namespace

void CreateShockwaveRing(RegionSimulation& _region, const Neuron::Vec3& _position, double _sizeSeconds, std::uint8_t _team)
{
  Effect ring;
  ring.id = _region.IssueId();
  ring.kind = EffectKind::ShockwaveRing;
  ring.team = _team;
  ring.position = _position;
  ring.sizeSeconds = _sizeSeconds;
  static_cast<void>(_region.Effects().Take(ring));
}

void FireLaser(RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position, const Neuron::Vec3& _velocity)
{
  LaserBolt bolt;
  bolt.id = _region.IssueId();
  bolt.team = _team;
  bolt.position = _position;
  bolt.velocity = _velocity;
  bolt.movesLeft = static_cast<std::int32_t>(_region.Tuning().ref.weapon.laser.lifeTicks);
  static_cast<void>(_region.LaserBolts().Take(bolt));
  _region.Emit({EffectEventKind::LaserFired, _team, bolt.id, _position, _velocity, 0.0});
}

void ThrowGrenade(RegionSimulation& _region, std::uint8_t _team, const Neuron::Vec3& _position, const Neuron::Vec3& _target)
{
  const auto& weapon = _region.Tuning().ref.weapon;
  const double force = std::min(weapon.throwing.forceFactor * std::sqrt(Neuron::Length(_target - _position)), weapon.throwing.maxForce);
  Neuron::Vec3 front = Neuron::Normalized(_target - _position);
  front.y = weapon.throwing.frontLift;
  front = Neuron::Normalized(front);
  Effect grenade;
  grenade.id = _region.IssueId();
  grenade.kind = EffectKind::Grenade;
  grenade.team = _team;
  grenade.position = _position;
  grenade.velocity = front * force;
  grenade.force = weapon.thrown.initialForce;
  grenade.power = weapon.grenade.power;
  grenade.fuseTicks = static_cast<std::int32_t>(weapon.grenade.fuseTicks);
  static_cast<void>(_region.Effects().Take(grenade));
  _region.Emit({EffectEventKind::GrenadeThrown, _team, grenade.id, _position, grenade.velocity, 0.0});
}

std::uint32_t Explode(RegionSimulation& _region, const Neuron::Vec3& _position, double _range, double _damage, std::uint8_t _team)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto& explosion = tuning.ref.sim.explosion;
  _region.Emit({EffectEventKind::Explosion, _team, 0, _position, {_damage, 0.0, 0.0}, _range});
  const double reach = explosion.actorReach * _range;
  std::uint32_t kills = 0;
  // R-SIM-389: the actors in actor-grid result order, every team's, the exploding team's own included.
  for (const ActorRef& ref : _region.InRange(_position.x, _position.z, reach, AllTeams()))
  {
    Figure* victim = _region.Resolve(ref);
    if (victim == nullptr)
    {
      continue;
    }
    const double distance = Neuron::Length(victim->position - _position);
    const double factor = std::clamp(explosion.actorGain * (reach - distance) / _range, 0.0, 1.0);
    const HealthChange change = _region.Damage(ref, -static_cast<std::int32_t>(_damage * factor), DamageKind::Explosive);
    if (change.registered && victim->kind != FigureKind::Leader)
    {
      // R-SIM-374: the knock-back makes the actor airborne.
      const Neuron::Vec3 away = victim->position - _position;
      const double lift = victim->onGround ? _damage : -1.0;
      victim->velocity = victim->velocity + Neuron::Normalized({away.x, lift, away.z}) * factor;
      victim->onGround = false;
    }
    if (change.killed && ref.team != _team && victim->kind == FigureKind::Worker)
    {
      ++kills;
    }
  }
  // Step 2 (R-BLDG-035): every structure whose position lies within the structure reach takes −D × (1 − d ÷ 3r), with no
  // team filter, in slot order.
  const double structureReach = explosion.structureReach * _range;
  SlicedPool<Structure>& structures = _region.Structures();
  for (std::uint32_t slot = 0; slot < structures.Capacity(); ++slot)
  {
    if (!structures.Used(slot))
    {
      continue;
    }
    Structure& structure = structures.At(slot);
    const double distance = Neuron::Length(structure.position - _position);
    if (distance < structureReach)
    {
      DamageTurret(_region, structure, -static_cast<std::int32_t>(_damage * (1.0 - distance / structureReach)));
    }
  }
  _region.TeamAt(_team).kills += kills;
  CreateShockwaveRing(_region, _position, _range / tuning.ref.weapon.shockwave.sizeDivisor, _team);
  return kills;
}

bool AdvanceLaserBolt(RegionSimulation& _region, LaserBolt& _bolt)
{
  const TuningMvp& tuning = _region.Tuning();
  const auto& laser = tuning.ref.weapon.laser;
  if (_bolt.movesLeft <= 0)
  {
    return true; // R-CRATE-440: removed on the advance after its last move
  }
  --_bolt.movesLeft;
  const Neuron::TerrainField& field = _region.Terrain()->field;
  const Neuron::Vec3 previous = _bolt.position;
  const Neuron::Vec3 step = _bolt.velocity * tuning.ref.sim.tickPeriod;
  Neuron::Vec3 next = previous + step;
  Neuron::Vec3 legStart = previous;
  if (next.y <= field.Height(next.x, next.z))
  {
    // R-CRATE-441 step 4: the second ground contact ends the bolt; the first reflects it about the terrain normal at
    // the contact that a ray from its previous position finds, and it flies the rest of its step from there (ADR-105).
    if (_bolt.ricocheted)
    {
      return true;
    }
    _bolt.ricocheted = true;
    const std::optional<Neuron::Vec3> hit = field.CastRay(previous, step);
    const Neuron::Vec3 contact = hit ? *hit : Neuron::Vec3{next.x, field.Height(next.x, next.z), next.z};
    _bolt.velocity = Reflect(_bolt.velocity, field.Normal(contact.x, contact.z));
    const double rest = std::max(0.0, Neuron::Length(step) - Neuron::Length(contact - previous));
    next = rest > 0.0 ? contact + Neuron::WithLength(_bolt.velocity, rest) : contact;
    legStart = contact;
    _region.Emit({EffectEventKind::LaserRicochet, _bolt.team, _bolt.id, contact, _bolt.velocity, 0.0});
  }
  _bolt.position = next;
  if (!_region.Movement().OnMap(next.x, next.z))
  {
    return true; // R-CRATE-441 step 5
  }
  // Step 6: the step's ray meeting a structure of its obstacle-grid cell ends the bolt, with its spark and sound and no
  // damage to the structure.
  const double length = Neuron::Length(next - legStart);
  if (length > 0.0)
  {
    const Neuron::Vec3 direction = (next - legStart) * (1.0 / length);
    for (const std::uint32_t index : _region.Obstacles().StructuresAt(next.x, next.z))
    {
      if (RayHits(_region.Obstacles().Shape(index), legStart, direction, length))
      {
        _region.Emit({EffectEventKind::LaserHitStructure, _bolt.team, _bolt.id, next, _bolt.velocity, 0.0});
        return true;
      }
    }
  }
  if (_bolt.harmless)
  {
    return false;
  }
  // Step 7: the enemies within the step's length of its middle; a hit is within the hit height of the bolt and the hit
  // radius of the step's leg in the horizontal plane, the first in actor-grid result order.
  const Neuron::Vec3 middle = (legStart + next) * 0.5;
  for (const ActorRef& ref : _region.InRange(middle.x, middle.z, Neuron::Length(next - legStart), RegionSimulation::EnemiesOf(_bolt.team)))
  {
    Figure* victim = _region.Resolve(ref);
    if (victim == nullptr || std::abs(victim->position.y - next.y) >= laser.hitHeight ||
        SegmentDistance2D(victim->position, legStart, next) >= laser.hitRadius)
    {
      continue;
    }
    HitByLaser(_region, _bolt, ref, *victim);
    _bolt.harmless = true;
    break;
  }
  return false;
}

bool AdvanceEffect(RegionSimulation& _region, Effect& _effect)
{
  switch (_effect.kind)
  {
  case EffectKind::ShockwaveRing:
    return RingStep(_region, _effect);
  case EffectKind::OrderMarker:
    return AdvanceOrderMarker(_region, _effect);
  case EffectKind::OrderTrailMark:
    return AdvanceTrailMark(_region, _effect);
  case EffectKind::TurretRound:
    return AdvanceTurretRound(_region, _effect);
  case EffectKind::Grenade:
    break;
  }
  // R-CRATE-415: the fuse counts down before the throw physics; at 0 the grenade explodes and is removed.
  --_effect.fuseTicks;
  if (_effect.fuseTicks <= 0)
  {
    const auto& grenade = _region.Tuning().ref.weapon.grenade;
    static_cast<void>(
      Explode(_region, _effect.position, _effect.power * grenade.rangeFactor, _effect.power * grenade.damageFactor, _effect.team));
    return true;
  }
  ThrownStep(_region, _effect);
  return false;
}

void WriteLaserBolt(Neuron::BitWriter& _writer, const LaserBolt& _bolt)
{
  _writer.WriteBits(_bolt.id, 64);
  _writer.WriteBits(_bolt.team, 8);
  WriteVec3(_writer, _bolt.position);
  WriteVec3(_writer, _bolt.velocity);
  _writer.WriteSigned(_bolt.movesLeft, 32);
  _writer.WriteBool(_bolt.ricocheted);
  _writer.WriteBool(_bolt.harmless);
}

bool ReadLaserBolt(Neuron::BitReader& _reader, LaserBolt& _bolt)
{
  _bolt.id = _reader.ReadBits(64);
  _bolt.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  bool finite = ReadVec3(_reader, _bolt.position);
  finite = ReadVec3(_reader, _bolt.velocity) && finite;
  _bolt.movesLeft = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _bolt.ricocheted = _reader.ReadBool();
  _bolt.harmless = _reader.ReadBool();
  return !_reader.Failed() && finite && _bolt.id != 0;
}

void WriteEffect(Neuron::BitWriter& _writer, const Effect& _effect)
{
  _writer.WriteBits(_effect.id, 64);
  _writer.WriteBounded(static_cast<std::uint64_t>(_effect.kind), EFFECT_KIND_MAX);
  _writer.WriteBits(_effect.team, 8);
  WriteVec3(_writer, _effect.position);
  WriteVec3(_writer, _effect.velocity);
  WriteDouble(_writer, _effect.force);
  WriteDouble(_writer, _effect.power);
  _writer.WriteSigned(_effect.fuseTicks, 32);
  WriteDouble(_writer, _effect.sizeSeconds);
  _writer.WriteSigned(_effect.activeTick, 32);
  _writer.WriteBits(_effect.route, 32);
  _writer.WriteSigned(_effect.routePoint, 32);
  _writer.WriteBool(_effect.arrived);
  _writer.WriteBits(_effect.createdTick, 64);
  _writer.WriteBits(_effect.source, 64);
}

bool ReadEffect(Neuron::BitReader& _reader, Effect& _effect, std::uint32_t _stateVersion)
{
  const bool markers = _stateVersion >= EFFECT_STATE_VERSION_MARKERS;
  _effect.id = _reader.ReadBits(64);
  const std::uint64_t kindMax = _stateVersion >= EFFECT_STATE_VERSION_ROUNDS ? EFFECT_KIND_MAX
                                : markers                                    ? EFFECT_KIND_MAX_BEFORE_ROUNDS
                                                                             : EFFECT_KIND_MAX_BEFORE_MARKERS;
  _effect.kind = static_cast<EffectKind>(_reader.ReadBounded(kindMax));
  _effect.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  bool finite = ReadVec3(_reader, _effect.position);
  finite = ReadVec3(_reader, _effect.velocity) && finite;
  finite = ReadDouble(_reader, _effect.force) && finite;
  finite = ReadDouble(_reader, _effect.power) && finite;
  _effect.fuseTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  finite = ReadDouble(_reader, _effect.sizeSeconds) && finite;
  _effect.activeTick = static_cast<std::int32_t>(_reader.ReadSigned(32));
  if (markers)
  {
    _effect.route = static_cast<RouteHandle>(_reader.ReadBits(32));
    _effect.routePoint = static_cast<std::int32_t>(_reader.ReadSigned(32));
    _effect.arrived = _reader.ReadBool();
    _effect.createdTick = _reader.ReadBits(64);
  }
  _effect.source = _stateVersion >= EFFECT_STATE_VERSION_ROUNDS ? _reader.ReadBits(64) : 0;
  // A ring's size divides its strength.
  return !_reader.Failed() && finite && _effect.id != 0 && (_effect.kind != EffectKind::ShockwaveRing || _effect.sizeSeconds > 0.0);
}

} // namespace Species
