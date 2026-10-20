#include "pch.h"

#include "Movement.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <numbers>

namespace Species
{

namespace
{

constexpr double TWO_PI = 2.0 * std::numbers::pi;

// R-SIM-379 and R-UNIT-112: when the candidate is at or below _shoreHeight, one signed fraction a is drawn, and the
// first point at distance d from _first to _last in steps of _step, at the angle d × a × π, whose terrain is above
// _shoreHeight becomes the candidate; with none, the candidate stays.
Neuron::Vec3 ShoreSearch(const Figure& _figure, const Neuron::Vec3& _candidate, double _shoreHeight, double _first, double _last,
                         double _step, DrawPurpose _purpose, const MovementContext& _context)
{
  if (_candidate.y > _shoreHeight)
  {
    return _candidate;
  }
  const double fraction = _context.draws->For(_purpose, _figure.id).NextSigned(1.0);
  const auto samples = static_cast<std::int64_t>(std::floor((_last - _first) / _step)) + 1;
  for (std::int64_t sample = 0; sample < samples; ++sample)
  {
    const double distance = _first + _step * static_cast<double>(sample);
    const double angle = distance * fraction * std::numbers::pi;
    const double x = _candidate.x + std::cos(angle) * distance;
    const double z = _candidate.z + std::sin(angle) * distance;
    if (_context.Height(x, z) > _shoreHeight)
    {
      return {x, _candidate.y, z};
    }
  }
  return _candidate;
}

// R-SIM-382 and R-UNIT-115: a push of _step directly away from a sphere's centre leaves a sphere of reach _reach after
// at most ceil(_reach ÷ _step) + 1 passes; the bound adds one more, and a loop that reaches it keeps its last
// candidate (DEC-014).
std::int64_t PushPasses(double _reach, double _step) noexcept
{
  return static_cast<std::int64_t>(std::ceil(_reach / _step)) + 2;
}

} // namespace

double SlopeFactor(FigureKind _kind, double _heightNow, double _heightNext, const TuningMvp& _tuning) noexcept
{
  const auto& unit = _tuning.ref.unit;
  if (_kind == FigureKind::Worker)
  {
    return std::clamp(1.0 + (_heightNow - _heightNext) / unit.worker.slopeDivisor, unit.worker.slopeMin, unit.worker.slopeMax);
  }
  return std::clamp(1.0 + (_heightNow - _heightNext) / unit.leader.slopeDivisor, unit.leader.slopeMin, unit.leader.slopeMax);
}

void WorldClamp(Figure& _figure, const MovementContext& _context) noexcept
{
  _figure.position.x = std::clamp(_figure.position.x, _context.minX, _context.maxX);
  _figure.position.z = std::clamp(_figure.position.z, _context.minZ, _context.maxZ);
}

Neuron::Vec3 SpawnPosition(StableId _subject, const Neuron::Vec3& _requested, double _spread, const MovementContext& _context)
{
  const auto& sim = _context.tuning->ref.sim;
  Neuron::SimRng& draw = _context.draws->For(DrawPurpose::SpawnSearch, _subject);
  for (std::int64_t attempt = 0; attempt < sim.spawn.searchAttempts; ++attempt)
  {
    const double distance = draw.NextUnit(_spread);
    const double angle = draw.NextUnit(TWO_PI);
    const Neuron::Vec3 point{_requested.x + std::cos(angle) * distance, _requested.y, _requested.z + std::sin(angle) * distance};
    if (_context.OnMap(point.x, point.z) && _context.Height(point.x, point.z) > sim.physics.seaLevel)
    {
      return point;
    }
  }
  const double margin = sim.spawn.edgeMargin;
  return {std::clamp(_requested.x, _context.minX + margin, _context.maxX - margin), _requested.y,
          std::clamp(_requested.z, _context.minZ + margin, _context.maxZ - margin)};
}

Neuron::Vec3 PushFromObstacles(const Figure& _figure, Neuron::Vec3 _candidate, DrawPurpose _shorePurpose, const MovementContext& _context)
{
  const auto& sim = _context.tuning->ref.sim;
  if (_figure.onGround)
  {
    _candidate.y = _context.Height(_candidate.x, _candidate.z);
  }
  _candidate = ShoreSearch(_figure, _candidate, sim.water.shoreHeight, 0.0, sim.water.shoreSearchRadius, sim.water.shoreSearchStep,
                           _shorePurpose, _context);
  const double dt = sim.tickPeriod;
  for (const std::uint32_t index : _context.obstacles->StructuresAt(_candidate.x, _candidate.z))
  {
    const ObstacleShape& shape = _context.obstacles->Shape(index);
    bool hit = SphereHits(shape, _candidate, sim.push.contactRadius);
    if (!hit)
    {
      // R-SIM-380: the anti-tunnelling ray from where the figure was a tick ago.
      const Neuron::Vec3 previous = _figure.position - _figure.velocity * dt;
      hit = RayHits(shape, previous, _figure.front, Neuron::Length(_figure.position - previous));
    }
    if (!hit)
    {
      continue;
    }
    Neuron::Vec3 offset = shape.position - _candidate;
    offset.y = 0.0;
    const Neuron::Vec3 push = Neuron::WithLength(offset, sim.push.step);
    const std::int64_t passes = PushPasses(shape.radius + sim.push.clearanceRadius, sim.push.step);
    for (std::int64_t pass = 0; pass < passes && SphereHits(shape, _candidate, sim.push.clearanceRadius); ++pass)
    {
      _candidate = _candidate - push;
    }
  }
  return _candidate;
}

Neuron::Vec3 WorkerPush(const Figure& _figure, Neuron::Vec3 _candidate, DrawPurpose _shorePurpose, const MovementContext& _context)
{
  const auto& worker = _context.tuning->ref.unit.worker;
  if (_figure.onGround)
  {
    _candidate.y = _context.Height(_candidate.x, _candidate.z);
  }
  _candidate = ShoreSearch(_figure, _candidate, worker.shoreHeight, worker.shoreSearchStart, worker.shoreSearchEnd, worker.shoreSearchStep,
                           _shorePurpose, _context);
  for (const std::uint32_t index : _context.obstacles->StructuresAt(_candidate.x, _candidate.z))
  {
    // R-UNIT-114: a structure whose centre is within the push range pushes the candidate directly away from it in the
    // horizontal plane, re-snapped to the terrain after each step.
    const ObstacleShape& shape = _context.obstacles->Shape(index);
    if (Neuron::Length(shape.position - _candidate) >= worker.structurePushRange)
    {
      continue;
    }
    Neuron::Vec3 away = _candidate - shape.position;
    away.y = 0.0;
    const Neuron::Vec3 push = Neuron::WithLength(away, worker.structurePushStep);
    const std::int64_t passes = PushPasses(shape.radius + worker.structureClearRadius, worker.structurePushStep);
    for (std::int64_t pass = 0; pass < passes && SphereHits(shape, _candidate, worker.structureClearRadius); ++pass)
    {
      _candidate = _candidate + push;
      _candidate.y = _context.Height(_candidate.x, _candidate.z);
    }
  }
  return _candidate;
}

Neuron::Vec3 PushFor(const Figure& _figure, const Neuron::Vec3& _candidate, DrawPurpose _shorePurpose, const MovementContext& _context)
{
  return _figure.kind == FigureKind::Worker ? WorkerPush(_figure, _candidate, _shorePurpose, _context)
                                            : PushFromObstacles(_figure, _candidate, _shorePurpose, _context);
}

void AirborneStep(Figure& _figure, const MovementContext& _context)
{
  const auto& sim = _context.tuning->ref.sim;
  const double dt = sim.tickPeriod;
  _figure.velocity.y -= sim.physics.gravity * dt;
  _figure.position = _figure.position + _figure.velocity * dt;
  // R-SIM-371: the push runs for its shore search's draw, and its result is discarded.
  static_cast<void>(PushFor(_figure, _figure.position, DrawPurpose::ShoreFall, _context));
  if (_figure.dead)
  {
    return; // R-SIM-372: a corpse keeps falling, through the terrain
  }
  const double ground = _context.Height(_figure.position.x, _figure.position.z) + sim.physics.landThreshold;
  if (_figure.position.y <= ground)
  {
    _figure.onGround = true;
    _figure.velocity = {};
    _figure.position.y = ground + sim.physics.landOffset;
  }
  if (_figure.position.y <= sim.physics.seaLevel)
  {
    if (_figure.inWaterTicks < 0)
    {
      _figure.inWaterTicks = 0;
    }
    _figure.velocity = {};
  }
}

bool SwimStep(Figure& _figure, const MovementContext& _context)
{
  const auto& sim = _context.tuning->ref.sim;
  const double dt = sim.tickPeriod;
  ++_figure.inWaterTicks;
  const bool drowning = _figure.inWaterTicks > sim.water.drownAfter;
  // No figure of the MVP is in an actor group, so no steering: the speed is set and the direction kept.
  _figure.velocity = Neuron::WithLength(_figure.velocity, sim.water.swimSpeed);
  _figure.position = _figure.position + _figure.velocity * dt;
  const double seconds = static_cast<double>(_figure.inWaterTicks) * dt;
  _figure.position.y = -sim.water.bobAmplitude * std::sin(sim.water.bobRate * seconds) - sim.water.bobDepth;
  if (_context.Height(_figure.position.x, _figure.position.z) > sim.physics.seaLevel)
  {
    _figure.inWaterTicks = -1;
  }
  return drowning;
}

bool WorkerMoveToWaypoint(Figure& _figure, bool _idle, const MovementContext& _context)
{
  const TuningMvp& tuning = *_context.tuning;
  const auto& worker = tuning.ref.unit.worker;
  const double dt = tuning.ref.sim.tickPeriod;
  if (_figure.waypoint == Neuron::Vec3{})
  {
    return false; // R-UNIT-107: the zero vector holds the Worker still
  }
  const double distance = Neuron::HorizontalDistance(_figure.waypoint, _figure.position);
  if (distance == 0.0)
  {
    _figure.velocity = {};
    return true;
  }
  if (distance < worker.snapDistance)
  {
    _figure.velocity = (_figure.waypoint - _figure.position) * (1.0 / dt);
    _figure.position = _figure.waypoint;
    return false; // arrival is reported next tick
  }
  double speed = _figure.speed;
  if (_idle)
  {
    speed *= worker.idleSpeedFactor;
  }
  const Neuron::Vec3 toWaypoint = Neuron::Normalized(_figure.waypoint - _figure.position);
  const Neuron::Vec3 direction = Neuron::Normalized(_figure.front * (1.0 - worker.turnBlend) + toWaypoint * worker.turnBlend);
  // R-UNIT-042: h_next at the position the step would reach before the slope factor (ADR-103).
  const Neuron::Vec3 next = _figure.position + direction * (speed * dt);
  speed *=
    SlopeFactor(FigureKind::Worker, _context.Height(_figure.position.x, _figure.position.z), _context.Height(next.x, next.z), tuning);
  if (distance < worker.arrivalSlowRadius)
  {
    speed *= distance / worker.arrivalSlowRadius;
  }
  const double step = speed * dt;
  Neuron::Vec3 moved = WorkerPush(_figure, _figure.position + direction * step, DrawPurpose::ShoreMove, _context) - _figure.position;
  if (Neuron::LengthSquared(moved) > step * step)
  {
    moved = Neuron::WithLength(moved, step);
  }
  _figure.velocity = moved * (1.0 / dt);
  _figure.front = Neuron::Normalized(moved);
  _figure.position = _figure.position + moved;
  return false;
}

bool LeaderMoveToWaypoint(Figure& _figure, bool _formationMode, const MovementContext& _context)
{
  const TuningMvp& tuning = *_context.tuning;
  const auto& leader = tuning.ref.unit.leader;
  const double dt = tuning.ref.sim.tickPeriod;
  const double distance = Neuron::HorizontalDistance(_figure.waypoint, _figure.position);
  if (distance <= (_formationMode ? leader.formationArrivalDistance : leader.arrivalDistance))
  {
    _figure.velocity = {};
    return true;
  }
  // R-UNIT-158: the front blends towards the waypoint and is renormalised; the Leader walks along it.
  const double turn = _formationMode ? leader.turnFormation : leader.turnWalk;
  const Neuron::Vec3 toWaypoint = Neuron::Normalized(_figure.waypoint - _figure.position);
  _figure.front = Neuron::Normalized(_figure.front * (1.0 - turn) + toWaypoint * turn);
  double speed = _formationMode ? _figure.speed * leader.formationSpeedFactor : _figure.speed;
  const Neuron::Vec3 next = _figure.position + _figure.front * (speed * dt);
  speed *=
    SlopeFactor(FigureKind::Leader, _context.Height(_figure.position.x, _figure.position.z), _context.Height(next.x, next.z), tuning);
  if (distance < leader.arrivalSlowRadius)
  {
    speed *= distance / leader.arrivalSlowRadius;
  }
  const Neuron::Vec3 moved =
    PushFromObstacles(_figure, _figure.position + _figure.front * (speed * dt), DrawPurpose::ShoreMove, _context) - _figure.position;
  _figure.velocity = moved * (1.0 / dt);
  _figure.position = _figure.position + moved;
  return false;
}

} // namespace Species
