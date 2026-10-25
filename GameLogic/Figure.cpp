#include "pch.h"

#include "DrawPurpose.h"
#include "Figure.h"
#include "StateValues.h"

#include <algorithm>
#include <bit>
#include <cmath>

namespace Species
{

namespace
{

Neuron::Vec3 DefaultFront(const TuningMvp& _tuning) noexcept
{
  const auto& front = _tuning.ref.sim.spawn.defaultFront;
  return {front.x, front.y, front.z};
}

// The state's bounds of the enumerations it holds.
constexpr std::uint64_t WORKER_STATE_MAX = static_cast<std::uint64_t>(WorkerState::Carrying);
constexpr std::uint64_t WORKER_STATE_MAX_BEFORE_PORTS = static_cast<std::uint64_t>(WorkerState::FollowingLeader);
constexpr std::uint64_t WORKER_STATE_MAX_BEFORE_LEADERS = static_cast<std::uint64_t>(WorkerState::Burning);
constexpr std::uint64_t THREAT_KIND_MAX = static_cast<std::uint64_t>(ThreatKind::Grenade);
constexpr std::uint64_t LEADER_STATE_MAX = static_cast<std::uint64_t>(LeaderState::GivingOrders);
constexpr std::uint64_t LEADER_STATE_MAX_BEFORE_ORDERS = static_cast<std::uint64_t>(LeaderState::ToWaypoint);

} // namespace

double GridRadiusOf(FigureKind _kind, const TuningMvp& _tuning) noexcept
{
  // R-UNIT-159: the mesh radius that mvp.unit.leader_mesh_radius holds until the cooked mesh is read (M1 plan T11).
  return _kind == FigureKind::Leader ? _tuning.mvp.unit.leaderMeshRadius * _tuning.ref.unit.leader.radiusFactor : 0.0;
}

void SetKind(Figure& _figure, FigureKind _kind, const TuningMvp& _tuning, TickDraws& _draws)
{
  const auto& unit = _tuning.ref.unit;
  std::int64_t health = 0;
  double rate = 0.0;
  if (_kind == FigureKind::Worker)
  {
    health = unit.worker.health;
    _figure.speed = unit.worker.speed;
    rate = unit.worker.rate;
  }
  else
  {
    health = unit.leader.health;
    _figure.speed = unit.leader.speed;
    rate = unit.leader.rate;
  }
  _figure.kind = _kind;
  _figure.radius = GridRadiusOf(_kind, _tuning);
  _figure.health = static_cast<std::int32_t>(std::min(health, unit.healthMax));
  const double tickPeriod = _tuning.ref.sim.tickPeriod;
  _figure.reloadIntervalTicks = static_cast<std::int32_t>(std::llround(rate / tickPeriod));
  const double seconds = _draws.For(DrawPurpose::ReloadTimer, _figure.id).NextUnit(rate);
  _figure.reloadTimerTicks = static_cast<std::int32_t>(std::ceil(seconds / tickPeriod));
  if (_kind == FigureKind::Worker)
  {
    // R-UNIT-060: the first grenade timer, 1 + ⌊NextUnit(5) ÷ tick period⌋ ticks; R-UNIT-064: the full threat range.
    const auto& first = unit.worker.grenadeFirstTicks;
    _figure.grenadeTimerTicks =
      TicksOf(first.base, _draws.For(DrawPurpose::GrenadeTimer, _figure.id).NextUnit(first.spreadSeconds), tickPeriod);
    _figure.threatRange = unit.worker.threatRange;
  }
}

std::int32_t TicksOf(std::int64_t _base, double _drawSeconds, double _tickPeriod) noexcept
{
  return static_cast<std::int32_t>(_base) + static_cast<std::int32_t>(std::floor(_drawSeconds / _tickPeriod));
}

Neuron::Vec3 SpawnFront(const Figure& _figure, const Neuron::Vec3& _velocity, const TuningMvp& _tuning, TickDraws& _draws)
{
  const Neuron::Vec3 fallback = DefaultFront(_tuning);
  if (Neuron::LengthSquared(_velocity) > 0.0)
  {
    return Neuron::NormalizedOr(_velocity, fallback);
  }
  if (_figure.kind != FigureKind::Worker)
  {
    return fallback;
  }
  Neuron::SimRng& draw = _draws.For(DrawPurpose::SpawnFront, _figure.id);
  const double x = draw.NextSigned(1.0);
  const double z = draw.NextSigned(1.0);
  return Neuron::NormalizedOr({x, 0.0, z}, fallback);
}

HealthChange ApplyHealthChange(Figure& _figure, std::int32_t _amount, DamageKind _kind, const TuningMvp& _tuning, bool _inFormation)
{
  const auto& unit = _tuning.ref.unit;
  std::int32_t amount = _amount;
  // R-UNIT-116 step 3; the Shield, the Transport, Fury and the Visitor faction of steps 1, 2 and 4 are not built.
  if (_figure.kind == FigureKind::Worker && _inFormation && _kind == DamageKind::Explosive)
  {
    amount = static_cast<std::int32_t>(static_cast<double>(amount) * unit.worker.formationExplosiveFactor);
  }
  HealthChange change{true, false};
  if (_figure.dead)
  {
    return change;
  }
  const std::int64_t result = std::int64_t{_figure.health} + amount;
  if (result <= 0)
  {
    _figure.health = static_cast<std::int32_t>(unit.corpse.fadeStart);
    _figure.dead = true;
    change.killed = true;
  }
  else if (result > unit.healthMax)
  {
    _figure.health = static_cast<std::int32_t>(unit.healthMax);
  }
  else
  {
    _figure.health = static_cast<std::int32_t>(result);
  }
  return change;
}

void WriteFigure(Neuron::BitWriter& _writer, const Figure& _figure)
{
  _writer.WriteBits(_figure.id, 64);
  _writer.WriteBounded(static_cast<std::uint64_t>(_figure.kind), FIGURE_KIND_MAX);
  WriteVec3(_writer, _figure.position);
  WriteVec3(_writer, _figure.velocity);
  WriteVec3(_writer, _figure.front);
  WriteVec3(_writer, _figure.roamCenter);
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_figure.roamRange), 64);
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_figure.speed), 64);
  _writer.WriteSigned(_figure.health, 32);
  _writer.WriteSigned(_figure.reloadIntervalTicks, 32);
  _writer.WriteSigned(_figure.reloadTimerTicks, 32);
  _writer.WriteBool(_figure.onGround);
  _writer.WriteBool(_figure.enabled);
  _writer.WriteBool(_figure.destroyed);
  _writer.WriteBool(_figure.dead);
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_figure.radius), 64);
  WriteVec3(_writer, _figure.waypoint);
  _writer.WriteBounded(static_cast<std::uint64_t>(_figure.workerState), WORKER_STATE_MAX);
  _writer.WriteBounded(static_cast<std::uint64_t>(_figure.leaderState), LEADER_STATE_MAX);
  _writer.WriteSigned(_figure.retargetTimerTicks, 32);
  _writer.WriteSigned(_figure.inWaterTicks, 32);
  _writer.WriteBool(_figure.orderSet);
  _writer.WriteBool(_figure.playerOrders);
  WriteVec3(_writer, _figure.orderPosition);
  _writer.WriteSigned(_figure.cells.x0, 32);
  _writer.WriteSigned(_figure.cells.z0, 32);
  _writer.WriteSigned(_figure.cells.x1, 32);
  _writer.WriteSigned(_figure.cells.z1, 32);
  _writer.WriteBits(_figure.route, 32);
  _writer.WriteSigned(_figure.routePoint, 32);
  _writer.WriteBounded(static_cast<std::uint64_t>(_figure.threat.kind), THREAT_KIND_MAX);
  _writer.WriteBits(_figure.threat.team, 8);
  _writer.WriteBits(_figure.threat.slot, 32);
  _writer.WriteBits(_figure.threat.id, 64);
  WriteDouble(_writer, _figure.threatRange);
  _writer.WriteBool(_figure.scared);
  _writer.WriteBool(_figure.threatSound);
  _writer.WriteSigned(_figure.grenadeTimerTicks, 32);
  _writer.WriteBool(_figure.promoted);
  _writer.WriteBool(_figure.underControl);
  _writer.WriteBits(_figure.leaderId, 64);
  _writer.WriteBits(_figure.leaderSlot, 32);
  _writer.WriteSigned(_figure.followTimerTicks, 32);
  _writer.WriteBounded(static_cast<std::uint64_t>(_figure.order), LEADER_ORDER_MAX);
  _writer.WriteBool(_figure.demoted);
  _writer.WriteBool(_figure.noFormations);
  _writer.WriteBool(_figure.botTurn);
  WriteVec3(_writer, _figure.botFront);
  _writer.WriteBits(_figure.lastMarkerTick, 64);
  _writer.WriteBool(_figure.formation);
  _writer.WriteBool(_figure.formationAngleSet);
  _writer.WriteBounded(_figure.slots.size(), FORMATION_SLOTS_STATE_MAX);
  for (const FormationSlot& slot : _figure.slots)
  {
    _writer.WriteBits(slot.member, 64);
    _writer.WriteSigned(slot.timerTicks, 32);
  }
  _writer.WriteSigned(_figure.nearestNode, 32);
  _writer.WriteBits(_figure.portStructureId, 64);
  _writer.WriteBits(_figure.portStructureSlot, 32);
  _writer.WriteBits(_figure.portIndex, 32);
  _writer.WriteBounded(static_cast<std::uint64_t>(_figure.task), WORKER_TASK_MAX);
  _writer.WriteBounded(static_cast<std::uint64_t>(_figure.idleReason), IDLE_REASON_MAX);
  _writer.WriteSigned(_figure.taskTicks, 32);
  _writer.WriteSigned(_figure.carriedMilliRu, 64);
}

bool ReadFigure(Neuron::BitReader& _reader, Figure& _figure, std::uint32_t _stateVersion, const TuningMvp& _tuning)
{
  _figure.id = _reader.ReadBits(64);
  _figure.kind = static_cast<FigureKind>(_reader.ReadBounded(FIGURE_KIND_MAX));
  bool finite = ReadVec3(_reader, _figure.position);
  finite = ReadVec3(_reader, _figure.velocity) && finite;
  finite = ReadVec3(_reader, _figure.front) && finite;
  finite = ReadVec3(_reader, _figure.roamCenter) && finite;
  finite = ReadDouble(_reader, _figure.roamRange) && finite;
  finite = ReadDouble(_reader, _figure.speed) && finite;
  _figure.health = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.reloadIntervalTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.reloadTimerTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.onGround = _reader.ReadBool();
  _figure.enabled = _reader.ReadBool();
  _figure.destroyed = _reader.ReadBool();
  _figure.dead = _reader.ReadBool();
  if (_stateVersion < FIGURE_STATE_VERSION_LEADERS)
  {
    static_cast<void>(_reader.ReadBool()); // the formation flag, which nothing set before M1 task T7
  }
  if (_stateVersion < FIGURE_STATE_VERSION_MOVEMENT)
  {
    // MVP-095: a figure saved before M1 task T4 stands where it was, Idle, with its kind's radius, registered nowhere.
    _figure.radius = GridRadiusOf(_figure.kind, _tuning);
    _figure.waypoint = _figure.position;
    return !_reader.Failed() && finite && _figure.id != 0;
  }
  finite = ReadDouble(_reader, _figure.radius) && finite;
  finite = ReadVec3(_reader, _figure.waypoint) && finite;
  _figure.workerState =
    static_cast<WorkerState>(_reader.ReadBounded(_stateVersion < FIGURE_STATE_VERSION_LEADERS ? WORKER_STATE_MAX_BEFORE_LEADERS
                                                 : _stateVersion < FIGURE_STATE_VERSION_PORTS ? WORKER_STATE_MAX_BEFORE_PORTS
                                                                                              : WORKER_STATE_MAX));
  _figure.leaderState = static_cast<LeaderState>(
    _reader.ReadBounded(_stateVersion < FIGURE_STATE_VERSION_LEADERS ? LEADER_STATE_MAX_BEFORE_ORDERS : LEADER_STATE_MAX));
  _figure.retargetTimerTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.inWaterTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.orderSet = _reader.ReadBool();
  _figure.playerOrders = _reader.ReadBool();
  finite = ReadVec3(_reader, _figure.orderPosition) && finite;
  _figure.cells.x0 = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.cells.z0 = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.cells.x1 = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.cells.z1 = static_cast<std::int32_t>(_reader.ReadSigned(32));
  if (_stateVersion >= FIGURE_STATE_VERSION_ROUTES)
  {
    _figure.route = static_cast<RouteHandle>(_reader.ReadBits(32));
    _figure.routePoint = static_cast<std::int32_t>(_reader.ReadSigned(32));
  }
  if (_stateVersion < FIGURE_STATE_VERSION_COMBAT)
  {
    // MVP-095: a figure saved before M1 task T6 has no threat, the full threat range and its first grenade timer's base.
    if (_figure.kind == FigureKind::Worker)
    {
      _figure.threatRange = _tuning.ref.unit.worker.threatRange;
      _figure.grenadeTimerTicks = static_cast<std::int32_t>(_tuning.ref.unit.worker.grenadeFirstTicks.base);
    }
    return !_reader.Failed() && finite && _figure.id != 0;
  }
  _figure.threat.kind = static_cast<ThreatKind>(_reader.ReadBounded(THREAT_KIND_MAX));
  _figure.threat.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  _figure.threat.slot = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _figure.threat.id = _reader.ReadBits(64);
  finite = ReadDouble(_reader, _figure.threatRange) && finite;
  _figure.scared = _reader.ReadBool();
  _figure.threatSound = _reader.ReadBool();
  _figure.grenadeTimerTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  if (_stateVersion < FIGURE_STATE_VERSION_LEADERS)
  {
    return !_reader.Failed() && finite && _figure.id != 0; // MVP-095: no Leader followed, no order and no formation
  }
  _figure.promoted = _reader.ReadBool();
  _figure.underControl = _reader.ReadBool();
  _figure.leaderId = _reader.ReadBits(64);
  _figure.leaderSlot = static_cast<std::uint32_t>(_reader.ReadBits(32));
  _figure.followTimerTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
  _figure.order = static_cast<LeaderOrder>(_reader.ReadBounded(LEADER_ORDER_MAX));
  _figure.demoted = _reader.ReadBool();
  _figure.noFormations = _reader.ReadBool();
  _figure.botTurn = _reader.ReadBool();
  finite = ReadVec3(_reader, _figure.botFront) && finite;
  _figure.lastMarkerTick = _reader.ReadBits(64);
  _figure.formation = _reader.ReadBool();
  _figure.formationAngleSet = _reader.ReadBool();
  const std::uint64_t slots = _reader.ReadBounded(FORMATION_SLOTS_STATE_MAX);
  _figure.slots.clear();
  _figure.slots.reserve(static_cast<std::size_t>(slots));
  for (std::uint64_t index = 0; index < slots && !_reader.Failed(); ++index)
  {
    FormationSlot slot;
    slot.member = _reader.ReadBits(64);
    slot.timerTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
    _figure.slots.push_back(slot);
  }
  if (_stateVersion >= FIGURE_STATE_VERSION_BOTS)
  {
    _figure.nearestNode = static_cast<std::int32_t>(_reader.ReadSigned(32));
  }
  if (_stateVersion >= FIGURE_STATE_VERSION_PORTS)
  {
    _figure.portStructureId = _reader.ReadBits(64);
    _figure.portStructureSlot = static_cast<std::uint32_t>(_reader.ReadBits(32));
    _figure.portIndex = static_cast<std::uint32_t>(_reader.ReadBits(32));
    _figure.task = static_cast<WorkerTask>(_reader.ReadBounded(WORKER_TASK_MAX));
    _figure.idleReason = static_cast<IdleReason>(_reader.ReadBounded(IDLE_REASON_MAX));
    _figure.taskTicks = static_cast<std::int32_t>(_reader.ReadSigned(32));
    _figure.carriedMilliRu = _reader.ReadSigned(64);
  }
  return !_reader.Failed() && finite && _figure.id != 0 && _figure.nearestNode >= -1 && _figure.carriedMilliRu >= 0;
}

void WriteHomePort(Neuron::BitWriter& _writer, const Figure& _figure)
{
  _writer.WriteBool(_figure.homeStructureId != 0);
  if (_figure.homeStructureId != 0)
  {
    _writer.WriteBits(_figure.homeStructureId, 64);
    _writer.WriteBits(_figure.homeStructureSlot, 32);
    _writer.WriteBits(_figure.homePortIndex, 32);
  }
}

bool ReadHomePort(Neuron::BitReader& _reader, Figure& _figure)
{
  _figure.homeStructureId = 0;
  _figure.homeStructureSlot = 0;
  _figure.homePortIndex = 0;
  if (_reader.ReadBool())
  {
    _figure.homeStructureId = _reader.ReadBits(64);
    _figure.homeStructureSlot = static_cast<std::uint32_t>(_reader.ReadBits(32));
    _figure.homePortIndex = static_cast<std::uint32_t>(_reader.ReadBits(32));
    return !_reader.Failed() && _figure.homeStructureId != 0;
  }
  return !_reader.Failed();
}

} // namespace Species
