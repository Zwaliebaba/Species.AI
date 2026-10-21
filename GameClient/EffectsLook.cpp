#include "pch.h"

#include "CrowdLook.h"
#include "DevelopmentOverlay.h"
#include "EffectKind.h"
#include "EffectsLook.h"
#include "FigureKind.h"
#include "Geometry.h"
#include "Profile.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <string_view>

namespace Species
{

namespace
{

// The catalogue entries of passes 10a and 10b (RND-308, ADR-123).
constexpr std::string_view EFFECT_ADDITIVE = "effect_additive";
constexpr std::string_view EFFECT_ADDITIVE_NODEPTH = "effect_additive_nodepth";
constexpr std::string_view EFFECT_GRADIENT_NODEPTH = "effect_gradient_nodepth";
constexpr std::string_view EFFECT_MESH_LIT = "effect_mesh_lit";
constexpr std::string_view LASER_BOLT = "laser_bolt";

// VFX-011: the emitter handles of the generators that are not an event's: the screen effects' steps, the pool's own draws
// and the events' bursts; a per-tick emitter seeds from the tick, the object's id and its purpose.
constexpr std::uint64_t SCREEN_EMITTER = 1;
constexpr std::uint64_t POOL_EMITTER = 2;
constexpr std::uint64_t EVENT_EMITTER = 3;
enum class TickPurpose : std::uint64_t
{
  Trail = 1,
  Burning,
  Control,
  MarkerStep
};
// The new ticks that run the per-tick emitters at most in one frame, when the replica jumps.
constexpr std::uint64_t MAX_TICK_EMITTER_RUNS = 3;
// The rounding of a sum of frame steps that a fixed step tolerates, s.
constexpr double STEP_TOLERANCE_SECONDS = 1e-9;
// The grenade's view-test radius, wu: mesh.projectile.thrown_weapon fits within it (AST-076).
constexpr float GRENADE_CULL_RADIUS = 4.0f;

constexpr Neuron::Vec3 UP{0.0, 1.0, 0.0};

std::array<float, 3> Rgb255(double _r, double _g, double _b) noexcept
{
  return {static_cast<float>(_r / 255.0), static_cast<float>(_g / 255.0), static_cast<float>(_b / 255.0)};
}

template <class Colors> std::array<float, 3> First(const Colors& _colors) noexcept
{
  return Rgb255(_colors.first.r, _colors.first.g, _colors.first.b);
}

template <class Colors> std::array<float, 3> Second(const Colors& _colors) noexcept
{
  return Rgb255(_colors.second.r, _colors.second.g, _colors.second.b);
}

template <class Type> Neuron::ParticleType TypeOf(const Type& _type, Neuron::ParticleBlend _blend, std::uint8_t _class)
{
  Neuron::ParticleType type;
  type.lifeSeconds = _type.life;
  type.size = _type.size;
  type.gravity = _type.gravity;
  type.friction = _type.friction;
  type.color1 = First(_type.colors);
  type.color2 = Second(_type.colors);
  type.blend = _blend;
  type.priorityClass = _class;
  return type;
}

std::uint16_t TextureOf(SceneTexture _texture) noexcept
{
  return static_cast<std::uint16_t>(_texture);
}

// The batch of _pass, _entry and _texture in _batches, made at the end when there is none, its view-test radius at least
// _radius.
Neuron::QuadBatch& BatchOf(std::vector<Neuron::QuadBatch>& _batches, Neuron::ScenePass _pass, std::string_view _entry,
                           SceneTexture _texture, float _radius)
{
  const std::uint16_t texture = TextureOf(_texture);
  for (Neuron::QuadBatch& batch : _batches)
  {
    if (batch.pass == _pass && batch.entry == _entry && batch.texture == texture)
    {
      batch.cullRadius = std::max(batch.cullRadius, _radius);
      return batch;
    }
  }
  return _batches.emplace_back(Neuron::QuadBatch{_pass, _entry, texture, _radius, 0.0f, {}});
}

Neuron::SceneQuad Facing(const Neuron::Vec3& _at, double _halfSize, const std::array<float, 4>& _color) noexcept
{
  Neuron::SceneQuad quad;
  quad.position = _at;
  quad.halfWidth = static_cast<float>(_halfSize);
  quad.halfHeight = quad.halfWidth;
  quad.color = _color;
  quad.endColor = _color;
  return quad;
}

double DistanceToSegment(const Neuron::Vec3& _point, const Neuron::Vec3& _from, const Neuron::Vec3& _to) noexcept
{
  const Neuron::Vec3 along = _to - _from;
  const double lengthSquared = Neuron::LengthSquared(along);
  const double t = lengthSquared > 0.0 ? std::clamp(Neuron::Dot(_point - _from, along) / lengthSquared, 0.0, 1.0) : 0.0;
  return Neuron::Length(_point - (_from + along * t));
}

// RND-234's point-in-view test: a cone of the full vertical field of view, widened by the tolerance.
bool PointInView(const Neuron::Vec3& _point, const EffectsView& _view, double _tolerance) noexcept
{
  const Neuron::Vec3 towards = _point - _view.camera;
  if (Neuron::LengthSquared(towards) <= 0.0)
  {
    return true;
  }
  return Neuron::Dot(Neuron::Normalized(towards), _view.cameraFront) >= std::cos(_view.verticalFovRadians) - _tolerance;
}

bool OnField(const Neuron::TerrainField& _field, const Neuron::Vec3& _point) noexcept
{
  const double x = _point.x - _field.Heights().OriginX();
  const double z = _point.z - _field.Heights().OriginZ();
  return x >= 0.0 && z >= 0.0 && x < _field.ExtentX() && z < _field.ExtentZ();
}

} // namespace

std::vector<Neuron::ParticleType> ParticleTypesOf(const TuningPresentation& _presentation)
{
  const TuningPresentation::Vfx::Particle& p = _presentation.vfx.particle;
  // VFX-207's table 4.2-A: the batch and the priority class of each type.
  std::vector<Neuron::ParticleType> types(static_cast<std::size_t>(ParticleKind::Count));
  types[static_cast<std::size_t>(ParticleKind::ExplosionCore)] = TypeOf(p.explosionCore, Neuron::ParticleBlend::Glow, 1);
  types[static_cast<std::size_t>(ParticleKind::ExplosionDebris)] = TypeOf(p.explosionDebris, Neuron::ParticleBlend::Smoke, 1);
  types[static_cast<std::size_t>(ParticleKind::MuzzleFlare)] = TypeOf(p.muzzleFlash, Neuron::ParticleBlend::Glow, 0);
  types[static_cast<std::size_t>(ParticleKind::ControlFlash)] = TypeOf(p.controlFlash, Neuron::ParticleBlend::Glow, 0);
  types[static_cast<std::size_t>(ParticleKind::MissileTrail)] = TypeOf(p.missileTrail, Neuron::ParticleBlend::Smoke, 2);
  types[static_cast<std::size_t>(ParticleKind::WorkerFire)] = TypeOf(p.workerFire, Neuron::ParticleBlend::Glow, 1);
  // VFX-210: explosion debris puffs missile trails as it flies.
  types[static_cast<std::size_t>(ParticleKind::ExplosionDebris)].puffType = static_cast<std::uint8_t>(ParticleKind::MissileTrail);
  return types;
}

Neuron::ParticleLook ParticleLookOf(const TuningPresentation& _presentation, const TuningMvp& _tuning) noexcept
{
  const TuningPresentation::Vfx::Particle& p = _presentation.vfx.particle;
  Neuron::ParticleLook look;
  look.sizeDivisor = p.sizeDivisor;
  look.subStepSeconds = p.subStep;
  look.maxSubSteps = static_cast<std::uint32_t>(std::max<std::int64_t>(p.maxSubSteps, 1));
  look.tickSeconds = _tuning.ref.sim.tickPeriod;
  look.puffRate = p.debris.puffRate;
  look.puffVelocityDivisor = p.debris.puffVelocityDivisor;
  look.puffSizeDivisor = p.debris.puffSizeDivisor;
  look.capacity = static_cast<std::size_t>(std::max<std::int64_t>(p.poolCapacity, 0));
  look.typeCap = static_cast<std::size_t>(std::max<std::int64_t>(p.typeCap, 0));
  look.classFillLimits = {p.classFillLimits.class0, p.classFillLimits.class1, p.classFillLimits.class2, p.classFillLimits.class3};
  look.lodFullDistance = p.lod.fullDistance;
  look.lodCutoffDistance = p.lod.cutoffDistance;
  look.alpha = p.alpha;
  look.fadeStart = p.fadeStart;
  look.smokeShadeDivisor = p.smokeShadeDivisor;
  return look;
}

EffectsLook::EffectsLook(const TuningPresentation& _presentation, const TuningMvp& _tuning)
  : m_presentation(&_presentation),
    m_tuning(&_tuning),
    m_particles(ParticleTypesOf(_presentation), ParticleLookOf(_presentation, _tuning),
                Neuron::FxSeed(static_cast<std::uint64_t>(_presentation.ref.sim.match.presentationSeed), POOL_EMITTER, 0))
{
}

void EffectsLook::Clear()
{
  m_particles.Clear();
  m_waiting.clear();
  m_sequenceTick.reset();
  m_sequence = 0;
  m_bolts.clear();
  m_flashes.clear();
  m_rings.clear();
  m_recoils.clear();
  m_controlled.clear();
  m_trailBorn.clear();
  m_lastTick.reset();
  m_circleCarry = 0.0;
  m_shake = 0.0;
  m_shakeYaw = 0.0;
  m_shakeRoll = 0.0;
  m_shakeCarry = 0.0;
  m_flashOnsets.clear();
  m_flashAbove = false;
  m_flashSuppressed = false;
}

void EffectsLook::Update(std::span<const ReceivedEvent> _events, const StateReplica& _replica, std::span<const InterpolatedFigure> _figures,
                         const EffectsView& _view)
{
  for (const ReceivedEvent& received : _events)
  {
    if (m_sequenceTick != received.tick)
    {
      m_sequenceTick = received.tick;
      m_sequence = 0;
    }
    m_waiting.push_back({received, m_sequence++});
  }
  // ADR-084 item 5: an event of tick t happens between the states of ticks t − 1 and t, so it plays as the display time
  // reaches t − 1, in the order it arrived.
  std::size_t played = 0;
  while (played < m_waiting.size() && static_cast<double>(m_waiting[played].received.tick) - 1.0 <= _view.displayTick)
  {
    Play(m_waiting[played], _view);
    ++played;
  }
  m_waiting.erase(m_waiting.begin(), m_waiting.begin() + static_cast<std::ptrdiff_t>(played));

  if (const std::optional<std::uint64_t> latest = _replica.LatestTick(); latest.has_value() && latest != m_lastTick)
  {
    const std::uint64_t runs = m_lastTick.has_value() && *latest > *m_lastTick ? std::min(*latest - *m_lastTick, MAX_TICK_EMITTER_RUNS) : 1;
    for (std::uint64_t run = 0; run < runs; ++run)
    {
      RunTickEmitters(*latest - runs + 1 + run, _replica, _figures, _view);
    }
    m_lastTick = latest;
  }
  {
    ENGINE_PROFILE_SCOPE("effects.particles.advance");
    m_particles.Advance(_view.scaledSeconds);
  }
}

void EffectsLook::Emit(ParticleKind _kind, const Neuron::Vec3& _at, const Neuron::Vec3& _velocity, std::optional<double> _size,
                       std::optional<std::array<float, 3>> _color, const EffectsView& _view)
{
  static_cast<void>(m_particles.Emit({static_cast<std::uint8_t>(_kind), _at, _velocity, _size, _color}, _view.scaledSeconds));
}

void EffectsLook::EmitBurst(Neuron::FxRng& _rng, double _count, const Neuron::Vec3& _at, const EffectsView& _view, ParticleKind _kind,
                            const Neuron::Vec3& _velocity, std::optional<double> _size, std::optional<std::array<float, 3>> _color)
{
  const std::uint32_t count = m_particles.BurstCount(_count, Neuron::Length(_at - _view.camera), _rng);
  for (std::uint32_t n = 0; n < count; ++n)
  {
    Emit(_kind, _at, _velocity, _size, _color, _view);
  }
}

void EffectsLook::Play(const Waiting& _waiting, const EffectsView& _view)
{
  const TuningPresentation& presentation = *m_presentation;
  const TuningMvp& tuning = *m_tuning;
  const EffectEvent& event = _waiting.received.event;
  const double dueTick = static_cast<double>(_waiting.received.tick) - 1.0;
  const double tick = tuning.ref.sim.tickPeriod;
  const auto& flash = presentation.ref.weapon.muzzleFlash;
  Neuron::FxRng rng(Neuron::FxSeed(_waiting.received.tick, EVENT_EMITTER, _waiting.sequence));
  switch (event.kind)
  {
  case EffectEventKind::LaserFired:
  {
    // R-CRATE-440, ADR-123: the bolt flies from the muzzle as the event plays; it is not replicated (ADR-111).
    m_bolts.push_back({event.subject, event.team, dueTick, {{event.position, event.vector, dueTick}}});
    // R-CRATE-405: its muzzle flash, of the laser size factor × the bolt's life in seconds.
    const double lifeSeconds = static_cast<double>(tuning.ref.weapon.laser.lifeTicks) * tick;
    m_flashes.push_back({event.position, Neuron::Normalized(event.vector), dueTick, flash.laserSizeFactor * lifeSeconds, flash.laserLife});
    const Neuron::Vec3 shot{event.vector.x, 0.0, event.vector.z};
    m_recoils.push_back(
      {event.team, event.position - UP * tuning.ref.unit.attack.muzzleHeight, Neuron::Normalized(shot), _view.scaledSeconds, std::nullopt});
    break;
  }
  case EffectEventKind::LaserRicochet:
  {
    // R-CRATE-441 step 4: the bolt's second flight starts at the contact when its first flight reaches it.
    const auto bolt = std::ranges::find(m_bolts, event.subject, &Bolt::id);
    if (bolt != m_bolts.end() && bolt->legs.size() == 1)
    {
      const Leg& first = bolt->legs.front();
      const double speed = Neuron::Length(first.velocity) * tick;
      const double ticks = speed > 0.0 ? Neuron::Length(event.position - first.origin) / speed : 0.0;
      bolt->legs.push_back({event.position, event.vector, first.originTick + ticks});
    }
    break;
  }
  case EffectEventKind::GrenadeThrown:
    m_flashes.push_back({event.position, Neuron::Normalized(event.vector), dueTick, flash.throwSize, flash.throwLife});
    break;
  case EffectEventKind::Explosion:
  {
    // VFX-219's Explosion: the cores and the ring of debris, of the range r and the damage D.
    const auto& explosion = presentation.ref.weapon.explosion;
    const auto& emit = presentation.vfx.emit.explosion;
    const double range = event.value;
    const double load = range * event.vector.x * explosion.particleFactor;
    const double cores = std::floor(std::max(0.0, load));
    const double extra = std::floor(rng.NextUnit(cores));
    const Neuron::Vec3 coreAt = event.position + UP * (explosion.coreLift * range);
    const std::uint32_t coreCount = m_particles.BurstCount(cores + extra, Neuron::Length(coreAt - _view.camera), rng);
    for (std::uint32_t n = 0; n < coreCount; ++n)
    {
      const double size = emit.coreSize.base + rng.NextUnit(emit.coreSize.spread);
      const Neuron::Vec3 velocity{rng.NextSigned(2.0 * emit.coreVelocity.xSpread),
                                  emit.coreVelocity.yBase + rng.NextUnit(emit.coreVelocity.ySpread),
                                  rng.NextSigned(2.0 * emit.coreVelocity.zSpread)};
      Emit(ParticleKind::ExplosionCore, coreAt, velocity, size, std::nullopt, _view);
    }
    const double debris = std::floor(std::clamp(load, explosion.debrisMin, std::max(explosion.debrisMin, explosion.debrisMax)));
    const Neuron::Vec3 debrisAt = event.position + UP * (emit.debrisLift * range);
    const std::uint32_t debrisCount = m_particles.BurstCount(debris, Neuron::Length(debrisAt - _view.camera), rng);
    for (std::uint32_t n = 0; n < debrisCount; ++n)
    {
      const double angle = static_cast<double>(n) * 2.0 * std::numbers::pi / std::max(debris, 1.0);
      const double rise = emit.debrisRise.base + rng.NextUnit(emit.debrisRise.spread);
      const double speed = emit.debrisSpeed.base + rng.NextUnit(emit.debrisSpeed.spread);
      const double size = emit.debrisSize.base + rng.NextUnit(emit.debrisSize.spread);
      Emit(ParticleKind::ExplosionDebris, debrisAt, Neuron::Normalized({std::cos(angle), rise, std::sin(angle)}) * speed, size,
           std::nullopt, _view);
    }
    break;
  }
  case EffectEventKind::WorkerPromoted:
  {
    // R-CTRL-071: the promotion's control flashes, rising at a drawn velocity.
    const auto& promote = presentation.ref.ctrl.promote;
    const auto modulus = static_cast<std::uint32_t>(std::max(1.0, promote.flashCount.modulus));
    const double count = promote.flashCount.base + static_cast<double>(rng.NextUint() % modulus);
    const std::uint32_t emitted = m_particles.BurstCount(count, Neuron::Length(event.position - _view.camera), rng);
    for (std::uint32_t n = 0; n < emitted; ++n)
    {
      const Neuron::Vec3 velocity{rng.NextSigned(promote.flashVelocity.xRange), rng.NextUnit(promote.flashVelocity.yRange),
                                  rng.NextSigned(promote.flashVelocity.zRange)};
      Emit(ParticleKind::ControlFlash, event.position, velocity, std::nullopt, std::nullopt, _view);
    }
    break;
  }
  case EffectEventKind::WorkersOrderedByPlayer:
  case EffectEventKind::LeaderOrderWaypoint:
  {
    // VFX-219's CrowdOrder and LeaderOrder: two muzzle flares in the team colour at the target.
    const std::array<float, 4> team = TeamColor(event.team);
    const std::array<float, 3> color{team[0], team[1], team[2]};
    const auto& sizes = presentation.vfx.emit.order.sizes;
    for (const double size : {sizes.first, sizes.second})
    {
      EmitBurst(rng, 1.0, event.position, _view, ParticleKind::MuzzleFlare, {}, size, color);
    }
    break;
  }
  default:
    break;
  }
}

void EffectsLook::RunTickEmitters(std::uint64_t _tick, const StateReplica& _replica, std::span<const InterpolatedFigure> _figures,
                                  const EffectsView& _view)
{
  const TuningPresentation& presentation = *m_presentation;
  const double tick = m_tuning->ref.sim.tickPeriod;
  const auto seed = [_tick](StableId _id, TickPurpose _purpose)
  { return Neuron::FxSeed(_tick, _id, static_cast<std::uint64_t>(_purpose)); };
  // VFX-219's ThrownWeaponTrail and OrderMarkerStep, from the replica's effect objects.
  const auto& thrown = presentation.ref.weapon.thrown;
  for (const InterpolatedEffect& effect : InterpolateEffects(_replica, _view.fraction))
  {
    if (effect.state.kind == EffectKind::Grenade && !effect.entered && Neuron::LengthSquared(effect.tickMotion) > 1e-6)
    {
      // R-CRATE-414: at p − v × dt, the previous state's position; at M1 the game speed is 1.
      Neuron::FxRng rng(seed(effect.id, TickPurpose::Trail));
      if (rng.NextUnit(1.0) < 1.0 - thrown.trailSkip)
      {
        const Neuron::Vec3 velocity = effect.tickMotion * (1.0 / tick);
        const Neuron::Vec3 at = effect.position - effect.tickMotion * _view.fraction;
        const Neuron::Vec3 jitter{rng.NextSigned(thrown.trailJitter), rng.NextSigned(thrown.trailJitter),
                                  rng.NextSigned(thrown.trailJitter)};
        EmitBurst(rng, 1.0, at, _view, ParticleKind::MissileTrail, velocity * 0.25 + jitter, thrown.trailSize, std::nullopt);
      }
    }
    else if (effect.state.kind == EffectKind::OrderMarker && effect.state.timer == 0)
    {
      // R-UNIT-185: a travelling marker's step flare in its team colour.
      Neuron::FxRng rng(seed(effect.id, TickPurpose::MarkerStep));
      const std::array<float, 4> team = TeamColor(effect.state.team);
      EmitBurst(rng, 1.0, effect.position, _view, ParticleKind::MuzzleFlare, {}, presentation.vfx.emit.orderMarker.size,
                std::array<float, 3>{team[0], team[1], team[2]});
    }
  }
  // VFX-219's WorkerBurning and ControlGained or ControlLost, from the frame's figures.
  const auto& burning = presentation.vfx.emit.burning;
  std::map<StableId, bool> controlled;
  for (const InterpolatedFigure& figure : _figures)
  {
    if (figure.state.kind != FigureKind::Worker || figure.state.dead)
    {
      continue;
    }
    if (figure.state.task == static_cast<std::uint8_t>(WorkerState::Burning))
    {
      Neuron::FxRng rng(seed(figure.id, TickPurpose::Burning));
      const auto modulus = static_cast<std::uint32_t>(std::max<std::int64_t>(burning.countMod, 1));
      const std::uint32_t count = m_particles.BurstCount(rng.NextUint() % modulus, Neuron::Length(figure.position - _view.camera), rng);
      const Neuron::Vec3 velocity = figure.tickMotion * (1.0 / tick);
      for (std::uint32_t n = 0; n < count; ++n)
      {
        const ParticleKind kind = n < static_cast<std::uint32_t>(burning.fireShare) ? ParticleKind::WorkerFire : ParticleKind::MissileTrail;
        const Neuron::Vec3 at = figure.position + UP * rng.NextUnit(burning.lift);
        const double size = burning.size.base + rng.NextUnit(burning.size.spread);
        const Neuron::Vec3 rise = UP * (burning.rise.base + rng.NextUnit(burning.rise.spread));
        Emit(kind, at, velocity * burning.velocityFactor + rise, size, std::nullopt, _view);
      }
    }
    const auto was = m_controlled.find(figure.id);
    if (was != m_controlled.end() && was->second != figure.state.underControl)
    {
      Neuron::FxRng rng(seed(figure.id, TickPurpose::Control));
      EmitBurst(rng, static_cast<double>(presentation.vfx.emit.control.count), figure.position, _view, ParticleKind::ControlFlash, {},
                std::nullopt, std::nullopt);
    }
    controlled.emplace(figure.id, figure.state.underControl);
  }
  m_controlled = std::move(controlled);
}

void EffectsLook::EmitCircle(const std::optional<SelectionCircle>& _circle, const Neuron::TerrainField& _field, const EffectsView& _view)
{
  if (!_circle.has_value())
  {
    m_circleCarry = 0.0;
    return;
  }
  // Each step of vfx.emit.step_rate on the unscaled clock emits the circle's control flashes of its radius's size, evenly
  // spaced on it and turning with the real clock, lifted above the terrain; a presentation emitter is not thinned (VFX-213).
  const auto& cursor = m_presentation->ref.ctrl.cursor;
  const double step = 1.0 / std::max(m_presentation->vfx.emit.stepRate, 1.0);
  const auto count = static_cast<std::size_t>(std::max<std::int64_t>(cursor.circleParticles, 0));
  m_circleCarry += _view.unscaledStep;
  // A step due within rounding of the frames' sum is taken, so the rate does not depend on the frame rate (VFX-220).
  for (int steps = 0; m_circleCarry >= step - STEP_TOLERANCE_SECONDS && steps < 30; ++steps)
  {
    m_circleCarry -= step;
    for (std::size_t n = 0; n < count; ++n)
    {
      const double angle =
        _view.realSeconds * cursor.circleSpin + static_cast<double>(n) * 2.0 * std::numbers::pi / static_cast<double>(count);
      Neuron::Vec3 at = _circle->center + Neuron::Vec3{std::cos(angle) * _circle->radius, 0.0, std::sin(angle) * _circle->radius};
      at.y = _field.Height(at.x, at.z) + cursor.circleLift;
      Emit(ParticleKind::ControlFlash, at, {}, _circle->radius, std::nullopt, _view);
    }
  }
  m_circleCarry = std::clamp(m_circleCarry, 0.0, step);
}

void EffectsLook::ApplyRecoil(std::vector<InterpolatedFigure>& _figures, const EffectsView& _view)
{
  const auto& worker = m_presentation->vfx.worker;
  std::erase_if(m_recoils, [&](const Recoil& _recoil) { return _view.scaledSeconds - _recoil.firedSeconds >= worker.recoilTime; });
  for (Recoil& recoil : m_recoils)
  {
    if (!recoil.shooter.has_value())
    {
      // The shot's shooter: the nearest living Worker of its team whose foot lies within the match range of the muzzle's.
      StableId best = 0;
      double bestDistance = worker.recoilMatchRange;
      for (const InterpolatedFigure& figure : _figures)
      {
        if (figure.state.kind == FigureKind::Worker && !figure.state.dead && figure.state.team == recoil.team)
        {
          const double distance = Neuron::HorizontalDistance(figure.position, recoil.foot);
          if (distance <= bestDistance)
          {
            best = figure.id;
            bestDistance = distance;
          }
        }
      }
      recoil.shooter = best;
    }
    if (*recoil.shooter == 0)
    {
      continue;
    }
    const auto figure = std::ranges::lower_bound(_figures, *recoil.shooter, {}, &InterpolatedFigure::id);
    if (figure != _figures.end() && figure->id == *recoil.shooter)
    {
      const double left = 1.0 - (_view.scaledSeconds - recoil.firedSeconds) / worker.recoilTime;
      figure->position = figure->position - recoil.direction * (worker.recoilDistance * std::clamp(left, 0.0, 1.0));
    }
  }
}

void EffectsLook::DrawBolts(const Neuron::TerrainField& _field, const EffectsView& _view, EffectDraws& _draws, double& _shakeRequest)
{
  const TuningPresentation& presentation = *m_presentation;
  const auto& laser = presentation.ref.weapon.laser;
  const double tick = m_tuning->ref.sim.tickPeriod;
  const double lifeTicks = static_cast<double>(m_tuning->ref.weapon.laser.lifeTicks);
  const auto halfWidth = static_cast<float>(presentation.rnd.laser.halfWidth);
  std::erase_if(m_bolts,
                [&](const Bolt& _bolt)
                {
                  // R-CRATE-440: a bolt flies its life's advances.
                  if (_view.displayTick - _bolt.firedTick >= lifeTicks)
                  {
                    return true;
                  }
                  const bool second = _bolt.legs.size() > 1 && _view.displayTick >= _bolt.legs.back().originTick;
                  const Leg& leg = second ? _bolt.legs.back() : _bolt.legs.front();
                  const double flown = std::max(0.0, _view.displayTick - leg.originTick);
                  const Neuron::Vec3 at = leg.origin + leg.velocity * (flown * tick);
                  // R-CRATE-441 steps 4 and 5: its second ground contact or the map's edge ends it.
                  return !OnField(_field, at) || (second && flown > 0.0 && at.y < _field.Height(at.x, at.z));
                });
  if (m_bolts.empty())
  {
    return;
  }
  Neuron::QuadBatch& batch = BatchOf(_draws.quads, Neuron::ScenePass::Lasers, LASER_BOLT, SceneTexture::BeamShort, halfWidth);
  for (const Bolt& bolt : m_bolts)
  {
    const bool second = bolt.legs.size() > 1 && _view.displayTick >= bolt.legs.back().originTick;
    const Leg& leg = second ? bolt.legs.back() : bolt.legs.front();
    const Neuron::Vec3 at = leg.origin + leg.velocity * (std::max(0.0, _view.displayTick - leg.originTick) * tick);
    const Neuron::Vec3 end = at + Neuron::Normalized(leg.velocity) * laser.billboardLength;
    // RND-339: a segment of the beam in the source team's colour, u along it; RND-338: drawn in identical layers.
    const std::array<float, 4> team = TeamColor(bolt.team);
    Neuron::SceneQuad quad = Facing(at, halfWidth, team);
    quad.endPoint = end;
    quad.orientation = Neuron::QuadOrientation::Segment;
    for (std::int64_t layer = 0; layer < presentation.rnd.laser.layers; ++layer)
    {
      batch.quads.push_back(quad);
    }
    // RND-340: a bolt passing near the camera shakes it, requested as its instances are written.
    if (DistanceToSegment(_view.camera, at, end) * DistanceToSegment(_view.camera, at, end) <= laser.shakeRangeSq)
    {
      _shakeRequest = std::max(_shakeRequest, laser.shake);
    }
  }
}

void EffectsLook::DrawFlashes(const EffectsView& _view, EffectDraws& _draws)
{
  const auto& flash = m_presentation->ref.weapon.muzzleFlash;
  const double tick = m_tuning->ref.sim.tickPeriod;
  const auto lifeAt = [&](const Flash& _flash)
  { return _flash.life - flash.decay * std::max(0.0, _view.displayTick - _flash.startTick) * tick; };
  // R-CRATE-475: a flash is removed once its life is at or below 0.
  std::erase_if(m_flashes, [&](const Flash& _flash) { return lifeAt(_flash) <= 0.0; });
  for (const Flash& muzzle : m_flashes)
  {
    const double seconds = std::max(0.0, _view.displayTick - muzzle.startTick) * tick;
    const Neuron::Vec3 at = muzzle.origin + muzzle.front * (flash.speed * seconds);
    const auto alpha = static_cast<float>(std::clamp(lifeAt(muzzle) * flash.alphaFactor, 0.0, 1.0));
    // VFX-222: below the depth-test size it is drawn with no depth test.
    const std::string_view entry = muzzle.size < flash.depthTestSize ? EFFECT_ADDITIVE_NODEPTH : EFFECT_ADDITIVE;
    Neuron::QuadBatch& batch =
      BatchOf(_draws.quads, Neuron::ScenePass::Effects, entry, SceneTexture::MuzzleFlash, static_cast<float>(muzzle.size));
    double halfSize = muzzle.size;
    for (std::int64_t quad = 0; quad < flash.quads; ++quad)
    {
      batch.quads.push_back(Facing(at, halfSize, {1.0f, 1.0f, 1.0f, alpha}));
      halfSize *= flash.shrink;
    }
  }
}

void EffectsLook::DrawGrenades(std::span<const InterpolatedEffect> _effects, const EffectsView& _view, EffectDraws& _draws)
{
  const auto& thrown = m_presentation->ref.weapon.thrown;
  const double tick = m_tuning->ref.sim.tickPeriod;
  Neuron::MeshBatch meshes{
    Neuron::ScenePass::Effects, EFFECT_MESH_LIT, static_cast<std::uint16_t>(SceneMesh::ThrownWeapon), GRENADE_CULL_RADIUS, {}};
  for (const InterpolatedEffect& effect : _effects)
  {
    if (effect.state.kind != EffectKind::Grenade)
    {
      continue;
    }
    // R-CRATE-410, R-CRATE-414: the mesh, tumbling about its right axis on the scaled clock from a phase of its id.
    const double yaw = YawRadians(effect.state.yaw);
    const Neuron::Vec3 heading{std::sin(yaw), 0.0, std::cos(yaw)};
    const Neuron::Vec3 right = Neuron::Cross(UP, heading);
    const double angle = m_presentation->vfx.thrown.tumbleRate * _view.scaledSeconds + static_cast<double>(effect.id % 64);
    const double c = std::cos(angle);
    const double s = std::sin(angle);
    Neuron::SceneMeshInstance instance;
    instance.position = effect.position;
    instance.axisX = right;
    instance.axisY = UP * c + heading * s;
    instance.axisZ = heading * c - UP * s;
    meshes.instances.push_back(instance);
    // The fuse blink: in the last share of each second of its age, a starburst in its colour and a white inner flash, of a
    // size that falls with the square root of the camera distance.
    const double ageSeconds = std::max(0.0, static_cast<double>(m_tuning->ref.weapon.grenade.fuseTicks) -
                                              static_cast<double>(effect.state.timer) - 1.0 + _view.fraction) *
                              tick;
    if (ageSeconds - std::floor(ageSeconds) >= 1.0 - thrown.innerFlashFraction)
    {
      const double distance = std::max(1.0, Neuron::Length(effect.position - _view.camera));
      const double halfSize = thrown.flashSizeFactor / std::sqrt(distance);
      const auto alpha = static_cast<float>(thrown.flashAlpha / 255.0);
      const std::array<float, 3> color = Rgb255(thrown.color.r, thrown.color.g, thrown.color.b);
      Neuron::QuadBatch& batch =
        BatchOf(_draws.quads, Neuron::ScenePass::Effects, EFFECT_ADDITIVE, SceneTexture::Starburst, static_cast<float>(halfSize));
      batch.quads.push_back(Facing(effect.position, halfSize, {color[0], color[1], color[2], alpha}));
      batch.quads.push_back(Facing(effect.position, halfSize * m_presentation->vfx.thrown.innerFlashScale, {1.0f, 1.0f, 1.0f, alpha}));
    }
  }
  if (!meshes.instances.empty())
  {
    _draws.meshes.push_back(std::move(meshes));
  }
}

void EffectsLook::DrawRings(const EffectsView& _view, EffectDraws& _draws, double& _flashAlpha, double& _shakeRequest)
{
  const TuningPresentation& presentation = *m_presentation;
  const auto& shockwave = presentation.ref.weapon.shockwave;
  const auto& look = presentation.rnd.shockwave;
  const auto& simulated = m_tuning->ref.weapon.shockwave;
  const double tick = m_tuning->ref.sim.tickPeriod;
  const auto ageOf = [&](const Ring& _ring) { return std::max(0.0, _view.displayTick - _ring.bornTick) * tick; };
  std::erase_if(m_rings, [&](const Ring& _ring) { return ageOf(_ring) >= std::max(_ring.sizeSeconds, shockwave.billboardTime); });
  Neuron::TriangleBatch discs{Neuron::ScenePass::Effects, EFFECT_GRADIENT_NODEPTH, 0, 0, false, false, {}};
  const auto segments = static_cast<std::size_t>(std::max<std::int64_t>(look.discSegments, 3));
  for (const Ring& ring : m_rings)
  {
    const double age = ageOf(ring);
    const double distance = std::max(1.0, Neuron::Length(ring.position - _view.camera));
    if (age < ring.sizeSeconds && ring.sizeSeconds > 0.0)
    {
      // RND-333: the fan disc above the centre, its radius growing with the age, its rim fading with the life left.
      const double radius = simulated.startRadius + simulated.growth * age;
      const double left = ring.sizeSeconds - age;
      const Neuron::Vec3 center = ring.position + UP * shockwave.discLift;
      const std::array<float, 4> centerColor{static_cast<float>(look.centerColor.r), static_cast<float>(look.centerColor.g),
                                             static_cast<float>(look.centerColor.b), static_cast<float>(look.centerColor.a)};
      const std::array<float, 4> rimColor{static_cast<float>(look.rimColor.r), static_cast<float>(look.rimColor.g),
                                          static_cast<float>(look.rimColor.b),
                                          static_cast<float>(shockwave.discAlpha * left / ring.sizeSeconds)};
      const auto rim = [&](std::size_t _n)
      {
        const double angle = static_cast<double>(_n) * 2.0 * std::numbers::pi / static_cast<double>(segments);
        return Neuron::SceneVertex{center + Neuron::Vec3{std::cos(angle) * radius, 0.0, std::sin(angle) * radius}, {0.0f, 0.0f}, rimColor};
      };
      for (std::size_t n = 0; n < segments; ++n)
      {
        discs.vertices.push_back({center, {0.0f, 0.0f}, centerColor});
        discs.vertices.push_back(rim(n));
        discs.vertices.push_back(rim(n + 1));
      }
    }
    if (age < shockwave.billboardTime)
    {
      // RND-334: the blast, stacked starbursts and then glows, fading over its first second.
      const std::array<float, 4> color{static_cast<float>(look.blastColor.r), static_cast<float>(look.blastColor.g),
                                       static_cast<float>(look.blastColor.b), static_cast<float>(1.0 - age / shockwave.billboardTime)};
      for (const auto& [texture, scale] :
           {std::pair{SceneTexture::Starburst, look.blastStarScale}, std::pair{SceneTexture::GlowRound, look.blastGlowScale}})
      {
        double halfSize = ring.sizeSeconds * scale / std::sqrt(distance);
        const double largest =
          halfSize * std::pow(look.blastLayerGrowth, static_cast<double>(std::max<std::int64_t>(look.blastLayers - 1, 0)));
        Neuron::QuadBatch& batch = BatchOf(_draws.quads, Neuron::ScenePass::Effects, EFFECT_ADDITIVE, texture, static_cast<float>(largest));
        for (std::int64_t layer = 0; layer < look.blastLayers; ++layer)
        {
          batch.quads.push_back(Facing(ring.position, halfSize, color));
          halfSize *= look.blastLayerGrowth;
        }
      }
    }
    if (age < shockwave.flashTime && PointInView(ring.position, _view, presentation.rnd.frustum.pointTolerance))
    {
      // R-CRATE-473, VFX-233, VFX-234: the full-screen flash while the centre is in view, and the shake of its alpha.
      const double alpha = (1.0 - age / shockwave.flashTime) * std::max(0.0, 1.0 - distance / shockwave.flashRange);
      _flashAlpha = std::max(_flashAlpha, alpha);
      _shakeRequest = std::max(_shakeRequest, alpha);
    }
  }
  if (!discs.vertices.empty())
  {
    _draws.triangles.push_back(std::move(discs));
  }
}

void EffectsLook::DrawMarkers(std::span<const InterpolatedEffect> _effects, const EffectsView& _view, EffectDraws& _draws) const
{
  const auto halfSize = m_presentation->vfx.motion.orderMarkerFlare;
  const double fadeTicks = std::max(1.0, static_cast<double>(m_tuning->ref.unit.leader.markerFadeTicks));
  for (const InterpolatedEffect& effect : _effects)
  {
    if (effect.state.kind != EffectKind::OrderMarker)
    {
      continue;
    }
    // VFX-143: the travelling starburst, fading over the marker's last ticks once it has arrived.
    const double alpha =
      effect.state.timer == 0 ? 1.0 : std::clamp((static_cast<double>(effect.state.timer) - _view.fraction) / fadeTicks, 0.0, 1.0);
    std::array<float, 4> color = TeamColor(effect.state.team);
    color[3] = static_cast<float>(alpha);
    BatchOf(_draws.quads, Neuron::ScenePass::Effects, EFFECT_ADDITIVE, SceneTexture::Starburst, static_cast<float>(halfSize))
      .quads.push_back(Facing(effect.position + UP * m_tuning->ref.unit.leader.markerLift, halfSize, color));
  }
}

void EffectsLook::DrawTrailMarks(std::span<const InterpolatedEffect> _effects, const Neuron::TerrainField& _field, const EffectsView& _view,
                                 EffectDraws& _draws)
{
  // R-UNIT-186 (ADR-125): a ground-aligned marker.order_trail in the team colour, lowered below its carrier, its front
  // the carrier's velocity and its right front × the terrain normal, sized by the camera's distance and fading with its
  // age, which runs from the display tick it was first seen. The server sends only the local team's (ADR-106). Drawn
  // with no depth test, since it lies on the terrain.
  const auto& leader = m_presentation->ref.unit.leader;
  const double tick = m_tuning->ref.sim.tickPeriod;
  std::map<StableId, double> born;
  for (const InterpolatedEffect& effect : _effects)
  {
    if (effect.state.kind != EffectKind::OrderTrailMark)
    {
      continue;
    }
    const auto kept = m_trailBorn.find(effect.id);
    const double first = kept != m_trailBorn.end() ? kept->second : _view.displayTick - _view.fraction;
    born.emplace(effect.id, first);
    const double age = std::max(0.0, (_view.displayTick - first) * tick);
    const double alpha = leader.trailAlpha * (1.0 - age);
    if (alpha <= 0.0)
    {
      continue;
    }
    const Neuron::Vec3 at = effect.position - UP * m_tuning->ref.unit.leader.trailLift;
    const double yaw = YawRadians(effect.state.yaw);
    const Neuron::Vec3 front{std::sin(yaw), 0.0, std::cos(yaw)};
    const Neuron::Vec3 right = Neuron::NormalizedOr(Neuron::Cross(front, _field.Normal(at.x, at.z)), {1.0, 0.0, 0.0});
    const double halfSize =
      std::max(leader.trailSizeMin, leader.trailSizeScale * Neuron::Length(at - _view.camera) / leader.trailSizeDivisor);
    std::array<float, 4> color = TeamColor(effect.state.team);
    color[3] = static_cast<float>(alpha);
    Neuron::SceneQuad quad = Facing(at, halfSize, color);
    quad.orientation = Neuron::QuadOrientation::WorldOriented;
    quad.axisRight = right;
    quad.axisUp = front;
    BatchOf(_draws.quads, Neuron::ScenePass::Effects, EFFECT_ADDITIVE_NODEPTH, SceneTexture::OrderTrail, static_cast<float>(halfSize))
      .quads.push_back(quad);
  }
  m_trailBorn = std::move(born);
}

void EffectsLook::DrawScreen(double _flashAlpha, double _shakeRequest, const EffectsView& _view, EffectDraws& _draws)
{
  const TuningPresentation& presentation = *m_presentation;
  const auto& screen = presentation.vfx.screen;
  // VFX-235: a flash that reaches the threshold is an onset; at most the onsets of the window are shown, and a refused
  // onset's flash is not drawn until it falls below the threshold again.
  std::erase_if(m_flashOnsets, [&](double _onset) { return _view.realSeconds - _onset >= screen.flashWindow; });
  if (_flashAlpha >= screen.flashCountThreshold)
  {
    if (!m_flashAbove)
    {
      m_flashSuppressed = static_cast<std::int64_t>(m_flashOnsets.size()) >= screen.flashOnsets;
      if (!m_flashSuppressed)
      {
        m_flashOnsets.push_back(_view.realSeconds);
      }
    }
    m_flashAbove = true;
  }
  else
  {
    m_flashAbove = false;
    m_flashSuppressed = false;
  }
  if (_flashAlpha > 0.0 && !m_flashSuppressed)
  {
    // VFX-234: one white full-screen quad, additive, with the frame's largest alpha.
    const std::array<float, 4> white{1.0f, 1.0f, 1.0f, static_cast<float>(std::min(_flashAlpha, 1.0))};
    Neuron::TriangleBatch flash{Neuron::ScenePass::Effects, EFFECT_GRADIENT_NODEPTH, 0, 0, false, true, {}};
    for (const auto& [x, y] : {std::pair{-1.0, -1.0}, std::pair{1.0, -1.0}, std::pair{1.0, 1.0}, std::pair{-1.0, -1.0}, std::pair{1.0, 1.0},
                               std::pair{-1.0, 1.0}})
    {
      flash.vertices.push_back({{x, y, 0.0}, {0.0f, 0.0f}, white});
    }
    _draws.triangles.push_back(std::move(flash));
  }
  // VFX-232: the shake decays on the unscaled clock, a request raises it to at least its intensity, and at each fixed
  // step a yaw and a roll are drawn and held.
  const auto& shake = presentation.ref.ctrl.camera.shake;
  m_shake = std::max(std::max(0.0, m_shake - shake.decay * _view.unscaledStep), _shakeRequest);
  const double step = 1.0 / std::max(presentation.vfx.emit.stepRate, 1.0);
  m_shakeCarry += _view.unscaledStep;
  for (int steps = 0; m_shakeCarry >= step && steps < 30; ++steps)
  {
    m_shakeCarry -= step;
    ++m_shakeSteps;
    m_screenRng.Seed(Neuron::FxSeed(m_shakeSteps, SCREEN_EMITTER, 0));
    m_shakeYaw = m_screenRng.NextSigned(shake.yawFactor * m_shake);
    m_shakeRoll = shake.rollFactor * m_screenRng.NextSigned(m_shake);
  }
  m_shakeCarry = std::min(m_shakeCarry, step);
  if (m_shake <= 0.0)
  {
    m_shakeYaw = 0.0;
    m_shakeRoll = 0.0;
  }
  _draws.shakeYawRadians = m_shakeYaw;
  _draws.shakeRollRadians = m_shakeRoll;
}

void EffectsLook::Draw(std::span<const InterpolatedEffect> _effects, const Neuron::TerrainField& _field, const EffectsView& _view,
                       EffectDraws& _draws)
{
  // A ring met for the first time is copied, its birth at the display tick where its active tick was 0, so its blast
  // outlives the replicated ring (RND-334).
  for (const InterpolatedEffect& effect : _effects)
  {
    if (effect.state.kind == EffectKind::ShockwaveRing && std::ranges::find(m_rings, effect.id, &Ring::id) == m_rings.end())
    {
      m_rings.push_back({effect.id, effect.position, _view.displayTick - _view.fraction - static_cast<double>(effect.state.timer),
                         effect.state.size / EFFECT_SIZE_STEPS_PER_SECOND});
    }
  }
  double flashAlpha = 0.0;
  double shakeRequest = 0.0;
  DrawGrenades(_effects, _view, _draws);
  DrawFlashes(_view, _draws);
  DrawRings(_view, _draws, flashAlpha, shakeRequest);
  DrawMarkers(_effects, _view, _draws);
  DrawTrailMarks(_effects, _field, _view, _draws);
  DrawBolts(_field, _view, _draws, shakeRequest);
  m_particles.AppendBatches(_view.scaledSeconds, TextureOf(SceneTexture::ParticleDot), _draws.quads);
  DrawScreen(flashAlpha, shakeRequest, _view, _draws);
}

} // namespace Species
