#include "pch.h"

#include "ReplicatedState.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace Species
{

namespace
{

// A record's id after the first: the gap from the one before, less 1, in the narrowest of four widths.
constexpr std::array<std::uint32_t, 4> ID_GAP_BITS = {3, 7, 15, 64};
constexpr std::uint32_t ID_GAP_CLASS_BITS = 2;
// A delta's move, in the narrowest of four widths that holds all three axes: the widest holds any move between two
// positions of the fields.
constexpr std::array<std::uint32_t, 4> MOVE_HORIZONTAL_BITS = {7, 11, 15, POSITION_HORIZONTAL_BITS + 1};
constexpr std::array<std::uint32_t, 4> MOVE_VERTICAL_BITS = {7, 11, 15, POSITION_VERTICAL_BITS + 1};
constexpr std::uint32_t MOVE_CLASS_BITS = 2;
constexpr std::uint32_t TEAM_BITS = 8;
constexpr std::uint32_t HEALTH_BITS = 16;
constexpr std::uint32_t TIMER_BITS = 16;
constexpr std::uint32_t SIZE_BITS = 16;
constexpr std::uint32_t FIGURE_FLAG_BITS = 6;
constexpr std::uint32_t FIGURE_FIELD_BITS = 7;
constexpr std::uint32_t EFFECT_FIELD_BITS = 3;
constexpr std::uint32_t ORB_FIELD_BITS = 3;
// ADR-139's classes.
constexpr std::uint32_t STRUCTURE_FIELD_BITS = 5;
constexpr std::uint32_t STRUCTURE_FLAG_BITS = 2;
constexpr std::uint32_t PORTS_BITS = 16;
constexpr std::uint32_t SETTLEMENT_FIELD_BITS = 3;
constexpr std::uint32_t SITE_BITS = 8;
constexpr std::uint32_t COLOR_BITS = 8;
constexpr std::uint32_t COUNT_BITS = 16;
constexpr std::uint32_t CELLS_FIELD_BITS = 1;
constexpr std::uint32_t CHUNK_BITS = 8;
constexpr std::uint32_t CHUNK_CELL_BITS = 64;
constexpr std::uint64_t CELL_LAYER_MAX = static_cast<std::uint64_t>(CellLayer::Seen);
constexpr std::uint64_t RECORD_KIND_MAX = static_cast<std::uint64_t>(RecordKind::Removed);

constexpr bool FitsSigned(std::int64_t _value, std::uint32_t _bits) noexcept
{
  const std::int64_t limit = std::int64_t{1} << (_bits - 1);
  return _value >= -limit && _value < limit;
}

std::int32_t QuantizeAxis(double _value, std::uint32_t _bits) noexcept
{
  const double limit = static_cast<double>(std::int64_t{1} << (_bits - 1));
  const double steps = std::isfinite(_value) ? std::round(_value * POSITION_STEPS_PER_UNIT) : 0.0;
  return static_cast<std::int32_t>(std::clamp(steps, -limit, limit - 1.0));
}

std::size_t MoveClass(const QuantizedPosition& _move) noexcept
{
  for (std::size_t c = 0; c + 1 < MOVE_HORIZONTAL_BITS.size(); ++c)
  {
    if (FitsSigned(_move.x, MOVE_HORIZONTAL_BITS[c]) && FitsSigned(_move.y, MOVE_VERTICAL_BITS[c]) &&
        FitsSigned(_move.z, MOVE_HORIZONTAL_BITS[c]))
    {
      return c;
    }
  }
  return MOVE_HORIZONTAL_BITS.size() - 1;
}

std::size_t IdGapClass(std::uint64_t _gapLess1) noexcept
{
  for (std::size_t c = 0; c + 1 < ID_GAP_BITS.size(); ++c)
  {
    if (_gapLess1 < (std::uint64_t{1} << ID_GAP_BITS[c]))
    {
      return c;
    }
  }
  return ID_GAP_BITS.size() - 1;
}

std::size_t IdBits(StableId _id, std::optional<StableId> _previous) noexcept
{
  if (!_previous)
  {
    return 64;
  }
  return ID_GAP_CLASS_BITS + ID_GAP_BITS[IdGapClass(_id - *_previous - 1)];
}

constexpr std::size_t WHOLE_POSITION_BITS = 2 * POSITION_HORIZONTAL_BITS + POSITION_VERTICAL_BITS;

std::size_t MoveBits(const QuantizedPosition& _move) noexcept
{
  const std::size_t c = MoveClass(_move);
  return MOVE_CLASS_BITS + 2 * MOVE_HORIZONTAL_BITS[c] + MOVE_VERTICAL_BITS[c];
}

void WritePosition(Neuron::BitWriter& _writer, const QuantizedPosition& _position)
{
  _writer.WriteSigned(_position.x, POSITION_HORIZONTAL_BITS);
  _writer.WriteSigned(_position.y, POSITION_VERTICAL_BITS);
  _writer.WriteSigned(_position.z, POSITION_HORIZONTAL_BITS);
}

QuantizedPosition ReadPosition(Neuron::BitReader& _reader) noexcept
{
  QuantizedPosition position;
  position.x = static_cast<std::int32_t>(_reader.ReadSigned(POSITION_HORIZONTAL_BITS));
  position.y = static_cast<std::int32_t>(_reader.ReadSigned(POSITION_VERTICAL_BITS));
  position.z = static_cast<std::int32_t>(_reader.ReadSigned(POSITION_HORIZONTAL_BITS));
  return position;
}

void WriteMove(Neuron::BitWriter& _writer, const QuantizedPosition& _move)
{
  const std::size_t c = MoveClass(_move);
  _writer.WriteBits(c, MOVE_CLASS_BITS);
  _writer.WriteSigned(_move.x, MOVE_HORIZONTAL_BITS[c]);
  _writer.WriteSigned(_move.y, MOVE_VERTICAL_BITS[c]);
  _writer.WriteSigned(_move.z, MOVE_HORIZONTAL_BITS[c]);
}

QuantizedPosition ReadMove(Neuron::BitReader& _reader) noexcept
{
  const auto c = static_cast<std::size_t>(_reader.ReadBits(MOVE_CLASS_BITS));
  QuantizedPosition move;
  move.x = static_cast<std::int32_t>(_reader.ReadSigned(MOVE_HORIZONTAL_BITS[c]));
  move.y = static_cast<std::int32_t>(_reader.ReadSigned(MOVE_VERTICAL_BITS[c]));
  move.z = static_cast<std::int32_t>(_reader.ReadSigned(MOVE_HORIZONTAL_BITS[c]));
  return move;
}

QuantizedPosition Difference(const QuantizedPosition& _a, const QuantizedPosition& _b) noexcept
{
  return {_a.x - _b.x, _a.y - _b.y, _a.z - _b.z};
}

QuantizedPosition Sum(const QuantizedPosition& _a, const QuantizedPosition& _b) noexcept
{
  return {_a.x + _b.x, _a.y + _b.y, _a.z + _b.z};
}

std::uint64_t FigureFlags(const ReplicatedFigure& _figure) noexcept
{
  return (_figure.dead ? 1u : 0u) | (_figure.onGround ? 2u : 0u) | (_figure.underControl ? 4u : 0u) | (_figure.formation ? 8u : 0u) |
         (_figure.selected ? 16u : 0u) | (_figure.loaded ? 32u : 0u);
}

void SetFigureFlags(ReplicatedFigure& _figure, std::uint64_t _flags) noexcept
{
  _figure.dead = (_flags & 1u) != 0;
  _figure.onGround = (_flags & 2u) != 0;
  _figure.underControl = (_flags & 4u) != 0;
  _figure.formation = (_flags & 8u) != 0;
  _figure.selected = (_flags & 16u) != 0;
  _figure.loaded = (_flags & 32u) != 0;
}

std::uint64_t StructureFlags(const ReplicatedStructure& _structure) noexcept
{
  return (_structure.built ? 1u : 0u) | (_structure.active ? 2u : 0u);
}

void SetStructureFlags(ReplicatedStructure& _structure, std::uint64_t _flags) noexcept
{
  _structure.built = (_flags & 1u) != 0;
  _structure.active = (_flags & 2u) != 0;
}

// An optional team: a presence bit, then the team.
std::size_t OptionalTeamBits(const std::optional<std::uint8_t>& _team) noexcept
{
  return 1 + (_team.has_value() ? TEAM_BITS : 0);
}

void WriteOptionalTeam(Neuron::BitWriter& _writer, const std::optional<std::uint8_t>& _team)
{
  _writer.WriteBool(_team.has_value());
  if (_team)
  {
    _writer.WriteBits(*_team, TEAM_BITS);
  }
}

std::optional<std::uint8_t> ReadOptionalTeam(Neuron::BitReader& _reader) noexcept
{
  if (!_reader.ReadBool())
  {
    return std::nullopt;
  }
  return static_cast<std::uint8_t>(_reader.ReadBits(TEAM_BITS));
}

// The changed fields of a figure, and a delta's state with them.
std::uint8_t FigureChanges(const ReplicatedFigure& _state, const ReplicatedFigure& _baseline) noexcept
{
  unsigned changed = 0;
  changed |= _state.position != _baseline.position ? FIGURE_FIELD_POSITION : 0u;
  changed |= _state.yaw != _baseline.yaw ? FIGURE_FIELD_YAW : 0u;
  changed |= _state.health != _baseline.health ? FIGURE_FIELD_HEALTH : 0u;
  changed |= _state.task != _baseline.task ? FIGURE_FIELD_TASK : 0u;
  changed |= FigureFlags(_state) != FigureFlags(_baseline) ? FIGURE_FIELD_FLAGS : 0u;
  changed |= _state.order != _baseline.order ? FIGURE_FIELD_ORDER : 0u;
  changed |= _state.target != _baseline.target ? FIGURE_FIELD_TARGET : 0u;
  return static_cast<std::uint8_t>(changed);
}

std::uint8_t EffectChanges(const ReplicatedEffect& _state, const ReplicatedEffect& _baseline) noexcept
{
  unsigned changed = 0;
  changed |= _state.position != _baseline.position ? EFFECT_FIELD_POSITION : 0u;
  changed |= _state.yaw != _baseline.yaw ? EFFECT_FIELD_YAW : 0u;
  changed |= _state.timer != _baseline.timer ? EFFECT_FIELD_TIMER : 0u;
  return static_cast<std::uint8_t>(changed);
}

std::uint8_t OrbChanges(const ReplicatedOrb& _state, const ReplicatedOrb& _baseline) noexcept
{
  unsigned changed = 0;
  changed |= _state.position != _baseline.position ? ORB_FIELD_POSITION : 0u;
  changed |= _state.state != _baseline.state ? ORB_FIELD_STATE : 0u;
  changed |= _state.timer != _baseline.timer ? ORB_FIELD_TIMER : 0u;
  return static_cast<std::uint8_t>(changed);
}

// A structure's kind, owner, place and front never change: a delta never carries them (ADR-139).
std::uint8_t StructureChanges(const ReplicatedStructure& _state, const ReplicatedStructure& _baseline) noexcept
{
  unsigned changed = 0;
  changed |= StructureFlags(_state) != StructureFlags(_baseline) ? STRUCTURE_FIELD_FLAGS : 0u;
  changed |= _state.progress != _baseline.progress ? STRUCTURE_FIELD_PROGRESS : 0u;
  changed |= _state.mannedPorts != _baseline.mannedPorts ? STRUCTURE_FIELD_PORTS : 0u;
  changed |= _state.turretTeam != _baseline.turretTeam || _state.turretHealth != _baseline.turretHealth ? STRUCTURE_FIELD_TURRET : 0u;
  changed |= _state.aimYaw != _baseline.aimYaw || _state.aimPitch != _baseline.aimPitch ? STRUCTURE_FIELD_AIM : 0u;
  return static_cast<std::uint8_t>(changed);
}

// A settlement's site, team and colour never change.
std::uint8_t SettlementChanges(const ReplicatedSettlement& _state, const ReplicatedSettlement& _baseline) noexcept
{
  unsigned changed = 0;
  changed |=
    _state.storesMilliRu != _baseline.storesMilliRu || _state.capacityMilliRu != _baseline.capacityMilliRu ? SETTLEMENT_FIELD_STORES : 0u;
  changed |= _state.population != _baseline.population || _state.limit != _baseline.limit ? SETTLEMENT_FIELD_POPULATION : 0u;
  changed |= _state.radius != _baseline.radius ? SETTLEMENT_FIELD_RADIUS : 0u;
  return static_cast<std::uint8_t>(changed);
}

} // namespace

QuantizedPosition QuantizePosition(const Neuron::Vec3& _position) noexcept
{
  return {QuantizeAxis(_position.x, POSITION_HORIZONTAL_BITS), QuantizeAxis(_position.y, POSITION_VERTICAL_BITS),
          QuantizeAxis(_position.z, POSITION_HORIZONTAL_BITS)};
}

Neuron::Vec3 PositionOf(const QuantizedPosition& _position) noexcept
{
  return {_position.x / POSITION_STEPS_PER_UNIT, _position.y / POSITION_STEPS_PER_UNIT, _position.z / POSITION_STEPS_PER_UNIT};
}

std::uint16_t QuantizeYaw(double _x, double _z) noexcept
{
  if (!std::isfinite(_x) || !std::isfinite(_z) || (_x == 0.0 && _z == 0.0))
  {
    return 0;
  }
  const double turns = std::atan2(_x, _z) / (2.0 * std::numbers::pi);
  const auto steps = static_cast<std::int64_t>(std::round(turns * YAW_STEPS));
  return static_cast<std::uint16_t>((steps % YAW_STEPS + YAW_STEPS) % YAW_STEPS);
}

double YawRadians(std::uint16_t _yaw) noexcept
{
  return static_cast<double>(_yaw % YAW_STEPS) * (2.0 * std::numbers::pi / YAW_STEPS);
}

std::optional<StateRecord> MakeRecord(StableId _id, const ReplicatedState& _state, const ReplicatedState* _baseline)
{
  if (_baseline == nullptr || _baseline->index() != _state.index())
  {
    return StateRecord{_id, RecordKind::Whole, _state, 0, {}};
  }
  StateRecord record{_id, RecordKind::Delta, {}, 0, {}};
  if (const auto* figure = std::get_if<ReplicatedFigure>(&_state))
  {
    const auto& baseline = std::get<ReplicatedFigure>(*_baseline);
    ReplicatedFigure delta;
    record.changed = FigureChanges(*figure, baseline);
    delta.yaw = (record.changed & FIGURE_FIELD_YAW) != 0 ? figure->yaw : std::uint16_t{0};
    delta.health = (record.changed & FIGURE_FIELD_HEALTH) != 0 ? figure->health : std::uint16_t{0};
    delta.task = (record.changed & FIGURE_FIELD_TASK) != 0 ? figure->task : std::uint8_t{0};
    delta.order = (record.changed & FIGURE_FIELD_ORDER) != 0 ? figure->order : std::uint8_t{0};
    delta.target = (record.changed & FIGURE_FIELD_TARGET) != 0 ? figure->target : std::nullopt;
    if ((record.changed & FIGURE_FIELD_FLAGS) != 0)
    {
      SetFigureFlags(delta, FigureFlags(*figure));
    }
    record.move = Difference(figure->position, baseline.position);
    record.state = delta;
  }
  else if (const auto* effect = std::get_if<ReplicatedEffect>(&_state))
  {
    const auto& baseline = std::get<ReplicatedEffect>(*_baseline);
    ReplicatedEffect delta;
    record.changed = EffectChanges(*effect, baseline);
    delta.yaw = (record.changed & EFFECT_FIELD_YAW) != 0 ? effect->yaw : std::uint16_t{0};
    delta.timer = (record.changed & EFFECT_FIELD_TIMER) != 0 ? effect->timer : std::uint16_t{0};
    record.move = Difference(effect->position, baseline.position);
    record.state = delta;
  }
  else if (const auto* orb = std::get_if<ReplicatedOrb>(&_state))
  {
    const auto& baseline = std::get<ReplicatedOrb>(*_baseline);
    ReplicatedOrb delta;
    record.changed = OrbChanges(*orb, baseline);
    delta.state = (record.changed & ORB_FIELD_STATE) != 0 ? orb->state : OrbState::Birth;
    delta.timer = (record.changed & ORB_FIELD_TIMER) != 0 ? orb->timer : std::uint16_t{0};
    record.move = Difference(orb->position, baseline.position);
    record.state = delta;
  }
  else if (const auto* structure = std::get_if<ReplicatedStructure>(&_state))
  {
    const auto& baseline = std::get<ReplicatedStructure>(*_baseline);
    ReplicatedStructure delta;
    record.changed = StructureChanges(*structure, baseline);
    if ((record.changed & STRUCTURE_FIELD_FLAGS) != 0)
    {
      SetStructureFlags(delta, StructureFlags(*structure));
    }
    delta.progress = (record.changed & STRUCTURE_FIELD_PROGRESS) != 0 ? structure->progress : std::uint16_t{0};
    delta.mannedPorts = (record.changed & STRUCTURE_FIELD_PORTS) != 0 ? structure->mannedPorts : std::uint16_t{0};
    if ((record.changed & STRUCTURE_FIELD_TURRET) != 0)
    {
      delta.turretTeam = structure->turretTeam;
      delta.turretHealth = structure->turretHealth;
    }
    if ((record.changed & STRUCTURE_FIELD_AIM) != 0)
    {
      delta.aimYaw = structure->aimYaw;
      delta.aimPitch = structure->aimPitch;
    }
    record.state = delta;
  }
  else if (const auto* settlement = std::get_if<ReplicatedSettlement>(&_state))
  {
    const auto& baseline = std::get<ReplicatedSettlement>(*_baseline);
    ReplicatedSettlement delta;
    record.changed = SettlementChanges(*settlement, baseline);
    if ((record.changed & SETTLEMENT_FIELD_STORES) != 0)
    {
      delta.storesMilliRu = settlement->storesMilliRu;
      delta.capacityMilliRu = settlement->capacityMilliRu;
    }
    if ((record.changed & SETTLEMENT_FIELD_POPULATION) != 0)
    {
      delta.population = settlement->population;
      delta.limit = settlement->limit;
    }
    delta.radius = (record.changed & SETTLEMENT_FIELD_RADIUS) != 0 ? settlement->radius : std::uint32_t{0};
    record.state = delta;
  }
  else
  {
    const auto& cells = std::get<ReplicatedCells>(_state);
    const auto& baseline = std::get<ReplicatedCells>(*_baseline);
    ReplicatedCells delta;
    record.changed = cells.cells != baseline.cells ? CELLS_FIELD_CELLS : std::uint8_t{0};
    delta.cells = cells.cells;
    record.state = delta;
  }
  if (record.changed == 0)
  {
    return std::nullopt;
  }
  return record;
}

StateRecord RemovalRecord(StableId _id) noexcept
{
  return StateRecord{_id, RecordKind::Removed, {}, 0, {}};
}

std::optional<ReplicatedState> ApplyRecord(const StateRecord& _record, const ReplicatedState* _baseline)
{
  if (_record.kind == RecordKind::Whole)
  {
    return _record.state;
  }
  if (_record.kind != RecordKind::Delta || _baseline == nullptr || _baseline->index() != _record.state.index())
  {
    return std::nullopt;
  }
  ReplicatedState state = *_baseline;
  if (auto* figure = std::get_if<ReplicatedFigure>(&state))
  {
    const auto& delta = std::get<ReplicatedFigure>(_record.state);
    if ((_record.changed & FIGURE_FIELD_POSITION) != 0)
    {
      figure->position = Sum(figure->position, _record.move);
    }
    figure->yaw = (_record.changed & FIGURE_FIELD_YAW) != 0 ? delta.yaw : figure->yaw;
    figure->health = (_record.changed & FIGURE_FIELD_HEALTH) != 0 ? delta.health : figure->health;
    figure->task = (_record.changed & FIGURE_FIELD_TASK) != 0 ? delta.task : figure->task;
    figure->order = (_record.changed & FIGURE_FIELD_ORDER) != 0 ? delta.order : figure->order;
    figure->target = (_record.changed & FIGURE_FIELD_TARGET) != 0 ? delta.target : figure->target;
    if ((_record.changed & FIGURE_FIELD_FLAGS) != 0)
    {
      SetFigureFlags(*figure, FigureFlags(delta));
    }
  }
  else if (auto* effect = std::get_if<ReplicatedEffect>(&state))
  {
    const auto& delta = std::get<ReplicatedEffect>(_record.state);
    if ((_record.changed & EFFECT_FIELD_POSITION) != 0)
    {
      effect->position = Sum(effect->position, _record.move);
    }
    effect->yaw = (_record.changed & EFFECT_FIELD_YAW) != 0 ? delta.yaw : effect->yaw;
    effect->timer = (_record.changed & EFFECT_FIELD_TIMER) != 0 ? delta.timer : effect->timer;
  }
  else if (auto* orb = std::get_if<ReplicatedOrb>(&state))
  {
    const auto& delta = std::get<ReplicatedOrb>(_record.state);
    if ((_record.changed & ORB_FIELD_POSITION) != 0)
    {
      orb->position = Sum(orb->position, _record.move);
    }
    orb->state = (_record.changed & ORB_FIELD_STATE) != 0 ? delta.state : orb->state;
    orb->timer = (_record.changed & ORB_FIELD_TIMER) != 0 ? delta.timer : orb->timer;
  }
  else if (auto* structure = std::get_if<ReplicatedStructure>(&state))
  {
    const auto& delta = std::get<ReplicatedStructure>(_record.state);
    if ((_record.changed & STRUCTURE_FIELD_FLAGS) != 0)
    {
      SetStructureFlags(*structure, StructureFlags(delta));
    }
    structure->progress = (_record.changed & STRUCTURE_FIELD_PROGRESS) != 0 ? delta.progress : structure->progress;
    structure->mannedPorts = (_record.changed & STRUCTURE_FIELD_PORTS) != 0 ? delta.mannedPorts : structure->mannedPorts;
    if ((_record.changed & STRUCTURE_FIELD_TURRET) != 0)
    {
      structure->turretTeam = delta.turretTeam;
      structure->turretHealth = delta.turretHealth;
    }
    if ((_record.changed & STRUCTURE_FIELD_AIM) != 0)
    {
      structure->aimYaw = delta.aimYaw;
      structure->aimPitch = delta.aimPitch;
    }
  }
  else if (auto* settlement = std::get_if<ReplicatedSettlement>(&state))
  {
    const auto& delta = std::get<ReplicatedSettlement>(_record.state);
    if ((_record.changed & SETTLEMENT_FIELD_STORES) != 0)
    {
      settlement->storesMilliRu = delta.storesMilliRu;
      settlement->capacityMilliRu = delta.capacityMilliRu;
    }
    if ((_record.changed & SETTLEMENT_FIELD_POPULATION) != 0)
    {
      settlement->population = delta.population;
      settlement->limit = delta.limit;
    }
    settlement->radius = (_record.changed & SETTLEMENT_FIELD_RADIUS) != 0 ? delta.radius : settlement->radius;
  }
  else
  {
    auto& cells = std::get<ReplicatedCells>(state);
    cells.cells = (_record.changed & CELLS_FIELD_CELLS) != 0 ? std::get<ReplicatedCells>(_record.state).cells : cells.cells;
  }
  return state;
}

std::size_t RecordBits(const StateRecord& _record, std::optional<StableId> _previous)
{
  std::size_t bits = IdBits(_record.id, _previous) + Neuron::BitsFor(RECORD_KIND_MAX);
  if (_record.kind == RecordKind::Removed)
  {
    return bits;
  }
  bits += Neuron::BitsFor(REPLICATED_CLASS_MAX);
  const bool whole = _record.kind == RecordKind::Whole;
  if (std::holds_alternative<ReplicatedFigure>(_record.state))
  {
    // A target is a presence bit and, when present, a whole position.
    const auto& figure = std::get<ReplicatedFigure>(_record.state);
    const std::size_t targetBits = 1 + (figure.target.has_value() ? WHOLE_POSITION_BITS : 0);
    if (whole)
    {
      return bits + TEAM_BITS + Neuron::BitsFor(FIGURE_KIND_MAX) + WHOLE_POSITION_BITS + YAW_BITS + HEALTH_BITS +
             Neuron::BitsFor(FIGURE_TASK_MAX) + FIGURE_FLAG_BITS + Neuron::BitsFor(LEADER_ORDER_MAX) + targetBits;
    }
    bits += FIGURE_FIELD_BITS;
    bits += (_record.changed & FIGURE_FIELD_POSITION) != 0 ? MoveBits(_record.move) : 0;
    bits += (_record.changed & FIGURE_FIELD_YAW) != 0 ? YAW_BITS : 0;
    bits += (_record.changed & FIGURE_FIELD_HEALTH) != 0 ? HEALTH_BITS : 0;
    bits += (_record.changed & FIGURE_FIELD_TASK) != 0 ? Neuron::BitsFor(FIGURE_TASK_MAX) : 0;
    bits += (_record.changed & FIGURE_FIELD_FLAGS) != 0 ? FIGURE_FLAG_BITS : 0;
    bits += (_record.changed & FIGURE_FIELD_ORDER) != 0 ? Neuron::BitsFor(LEADER_ORDER_MAX) : 0;
    bits += (_record.changed & FIGURE_FIELD_TARGET) != 0 ? targetBits : 0;
    return bits;
  }
  if (std::holds_alternative<ReplicatedEffect>(_record.state))
  {
    if (whole)
    {
      return bits + TEAM_BITS + Neuron::BitsFor(EFFECT_KIND_MAX) + WHOLE_POSITION_BITS + YAW_BITS + TIMER_BITS + SIZE_BITS;
    }
    bits += EFFECT_FIELD_BITS;
    bits += (_record.changed & EFFECT_FIELD_POSITION) != 0 ? MoveBits(_record.move) : 0;
    bits += (_record.changed & EFFECT_FIELD_YAW) != 0 ? YAW_BITS : 0;
    bits += (_record.changed & EFFECT_FIELD_TIMER) != 0 ? TIMER_BITS : 0;
    return bits;
  }
  if (std::holds_alternative<ReplicatedOrb>(_record.state))
  {
    if (whole)
    {
      return bits + TEAM_BITS + WHOLE_POSITION_BITS + Neuron::BitsFor(ORB_STATE_MAX) + TIMER_BITS;
    }
    bits += ORB_FIELD_BITS;
    bits += (_record.changed & ORB_FIELD_POSITION) != 0 ? MoveBits(_record.move) : 0;
    bits += (_record.changed & ORB_FIELD_STATE) != 0 ? Neuron::BitsFor(ORB_STATE_MAX) : 0;
    bits += (_record.changed & ORB_FIELD_TIMER) != 0 ? TIMER_BITS : 0;
    return bits;
  }
  if (const auto* structure = std::get_if<ReplicatedStructure>(&_record.state))
  {
    const std::size_t turretBits = OptionalTeamBits(structure->turretTeam) + HEALTH_BITS;
    const std::size_t aimBits = YAW_BITS + PITCH_BITS;
    if (whole)
    {
      return bits + OptionalTeamBits(structure->owner) + Neuron::BitsFor(STRUCTURE_KIND_MAX) + WHOLE_POSITION_BITS + YAW_BITS +
             STRUCTURE_FLAG_BITS + Neuron::BitsFor(PROGRESS_PERMILLE_MAX) + PORTS_BITS + turretBits + aimBits;
    }
    bits += STRUCTURE_FIELD_BITS;
    bits += (_record.changed & STRUCTURE_FIELD_FLAGS) != 0 ? STRUCTURE_FLAG_BITS : 0;
    bits += (_record.changed & STRUCTURE_FIELD_PROGRESS) != 0 ? Neuron::BitsFor(PROGRESS_PERMILLE_MAX) : 0;
    bits += (_record.changed & STRUCTURE_FIELD_PORTS) != 0 ? PORTS_BITS : 0;
    bits += (_record.changed & STRUCTURE_FIELD_TURRET) != 0 ? turretBits : 0;
    bits += (_record.changed & STRUCTURE_FIELD_AIM) != 0 ? aimBits : 0;
    return bits;
  }
  if (std::holds_alternative<ReplicatedSettlement>(_record.state))
  {
    if (whole)
    {
      return bits + SITE_BITS + TEAM_BITS + COLOR_BITS + std::size_t{2} * STORES_BITS + std::size_t{2} * COUNT_BITS +
             SETTLEMENT_RADIUS_BITS;
    }
    bits += SETTLEMENT_FIELD_BITS;
    bits += (_record.changed & SETTLEMENT_FIELD_STORES) != 0 ? std::size_t{2} * STORES_BITS : 0;
    bits += (_record.changed & SETTLEMENT_FIELD_POPULATION) != 0 ? std::size_t{2} * COUNT_BITS : 0;
    bits += (_record.changed & SETTLEMENT_FIELD_RADIUS) != 0 ? SETTLEMENT_RADIUS_BITS : 0;
    return bits;
  }
  if (whole)
  {
    return bits + Neuron::BitsFor(CELL_LAYER_MAX) + SITE_BITS + std::size_t{2} * CHUNK_BITS + CHUNK_CELL_BITS;
  }
  return bits + CELLS_FIELD_BITS + ((_record.changed & CELLS_FIELD_CELLS) != 0 ? CHUNK_CELL_BITS : 0);
}

void WriteRecord(Neuron::BitWriter& _writer, const StateRecord& _record, std::optional<StableId> _previous)
{
  if (_previous)
  {
    const std::uint64_t gapLess1 = _record.id - *_previous - 1;
    const std::size_t c = IdGapClass(gapLess1);
    _writer.WriteBits(c, ID_GAP_CLASS_BITS);
    _writer.WriteBits(gapLess1, ID_GAP_BITS[c]);
  }
  else
  {
    _writer.WriteBits(_record.id, 64);
  }
  _writer.WriteBounded(static_cast<std::uint64_t>(_record.kind), RECORD_KIND_MAX);
  if (_record.kind == RecordKind::Removed)
  {
    return;
  }
  _writer.WriteBounded(_record.state.index(), REPLICATED_CLASS_MAX);
  const bool whole = _record.kind == RecordKind::Whole;
  if (const auto* figure = std::get_if<ReplicatedFigure>(&_record.state))
  {
    if (whole)
    {
      _writer.WriteBits(figure->team, TEAM_BITS);
      _writer.WriteBounded(static_cast<std::uint64_t>(figure->kind), FIGURE_KIND_MAX);
      WritePosition(_writer, figure->position);
    }
    else
    {
      _writer.WriteBits(_record.changed, FIGURE_FIELD_BITS);
      if ((_record.changed & FIGURE_FIELD_POSITION) != 0)
      {
        WriteMove(_writer, _record.move);
      }
    }
    if (whole || (_record.changed & FIGURE_FIELD_YAW) != 0)
    {
      _writer.WriteBits(figure->yaw, YAW_BITS);
    }
    if (whole || (_record.changed & FIGURE_FIELD_HEALTH) != 0)
    {
      _writer.WriteBits(figure->health, HEALTH_BITS);
    }
    if (whole || (_record.changed & FIGURE_FIELD_TASK) != 0)
    {
      _writer.WriteBounded(figure->task, FIGURE_TASK_MAX);
    }
    if (whole || (_record.changed & FIGURE_FIELD_FLAGS) != 0)
    {
      _writer.WriteBits(FigureFlags(*figure), FIGURE_FLAG_BITS);
    }
    if (whole || (_record.changed & FIGURE_FIELD_ORDER) != 0)
    {
      _writer.WriteBounded(figure->order, LEADER_ORDER_MAX);
    }
    if (whole || (_record.changed & FIGURE_FIELD_TARGET) != 0)
    {
      _writer.WriteBits(figure->target.has_value() ? 1u : 0u, 1);
      if (figure->target.has_value())
      {
        WritePosition(_writer, *figure->target);
      }
    }
    return;
  }
  if (const auto* effect = std::get_if<ReplicatedEffect>(&_record.state))
  {
    if (whole)
    {
      _writer.WriteBits(effect->team, TEAM_BITS);
      _writer.WriteBounded(static_cast<std::uint64_t>(effect->kind), EFFECT_KIND_MAX);
      WritePosition(_writer, effect->position);
    }
    else
    {
      _writer.WriteBits(_record.changed, EFFECT_FIELD_BITS);
      if ((_record.changed & EFFECT_FIELD_POSITION) != 0)
      {
        WriteMove(_writer, _record.move);
      }
    }
    if (whole || (_record.changed & EFFECT_FIELD_YAW) != 0)
    {
      _writer.WriteBits(effect->yaw, YAW_BITS);
    }
    if (whole || (_record.changed & EFFECT_FIELD_TIMER) != 0)
    {
      _writer.WriteBits(effect->timer, TIMER_BITS);
    }
    if (whole)
    {
      _writer.WriteBits(effect->size, SIZE_BITS);
    }
    return;
  }
  if (const auto* orb = std::get_if<ReplicatedOrb>(&_record.state))
  {
    if (whole)
    {
      _writer.WriteBits(orb->team, TEAM_BITS);
      WritePosition(_writer, orb->position);
    }
    else
    {
      _writer.WriteBits(_record.changed, ORB_FIELD_BITS);
      if ((_record.changed & ORB_FIELD_POSITION) != 0)
      {
        WriteMove(_writer, _record.move);
      }
    }
    if (whole || (_record.changed & ORB_FIELD_STATE) != 0)
    {
      _writer.WriteBounded(static_cast<std::uint64_t>(orb->state), ORB_STATE_MAX);
    }
    if (whole || (_record.changed & ORB_FIELD_TIMER) != 0)
    {
      _writer.WriteBits(orb->timer, TIMER_BITS);
    }
    return;
  }
  if (const auto* structure = std::get_if<ReplicatedStructure>(&_record.state))
  {
    if (whole)
    {
      WriteOptionalTeam(_writer, structure->owner);
      _writer.WriteBounded(static_cast<std::uint64_t>(structure->kind), STRUCTURE_KIND_MAX);
      WritePosition(_writer, structure->position);
      _writer.WriteBits(structure->yaw, YAW_BITS);
    }
    else
    {
      _writer.WriteBits(_record.changed, STRUCTURE_FIELD_BITS);
    }
    if (whole || (_record.changed & STRUCTURE_FIELD_FLAGS) != 0)
    {
      _writer.WriteBits(StructureFlags(*structure), STRUCTURE_FLAG_BITS);
    }
    if (whole || (_record.changed & STRUCTURE_FIELD_PROGRESS) != 0)
    {
      _writer.WriteBounded(structure->progress, PROGRESS_PERMILLE_MAX);
    }
    if (whole || (_record.changed & STRUCTURE_FIELD_PORTS) != 0)
    {
      _writer.WriteBits(structure->mannedPorts, PORTS_BITS);
    }
    if (whole || (_record.changed & STRUCTURE_FIELD_TURRET) != 0)
    {
      WriteOptionalTeam(_writer, structure->turretTeam);
      _writer.WriteBits(structure->turretHealth, HEALTH_BITS);
    }
    if (whole || (_record.changed & STRUCTURE_FIELD_AIM) != 0)
    {
      _writer.WriteBits(structure->aimYaw, YAW_BITS);
      _writer.WriteSigned(structure->aimPitch, PITCH_BITS);
    }
    return;
  }
  if (const auto* settlement = std::get_if<ReplicatedSettlement>(&_record.state))
  {
    if (whole)
    {
      _writer.WriteBits(settlement->site, SITE_BITS);
      _writer.WriteBits(settlement->team, TEAM_BITS);
      _writer.WriteBits(settlement->color, COLOR_BITS);
    }
    else
    {
      _writer.WriteBits(_record.changed, SETTLEMENT_FIELD_BITS);
    }
    if (whole || (_record.changed & SETTLEMENT_FIELD_STORES) != 0)
    {
      _writer.WriteBits(settlement->storesMilliRu, STORES_BITS);
      _writer.WriteBits(settlement->capacityMilliRu, STORES_BITS);
    }
    if (whole || (_record.changed & SETTLEMENT_FIELD_POPULATION) != 0)
    {
      _writer.WriteBits(settlement->population, COUNT_BITS);
      _writer.WriteBits(settlement->limit, COUNT_BITS);
    }
    if (whole || (_record.changed & SETTLEMENT_FIELD_RADIUS) != 0)
    {
      _writer.WriteBits(settlement->radius, SETTLEMENT_RADIUS_BITS);
    }
    return;
  }
  const auto& cells = std::get<ReplicatedCells>(_record.state);
  if (whole)
  {
    _writer.WriteBounded(static_cast<std::uint64_t>(cells.layer), CELL_LAYER_MAX);
    _writer.WriteBits(cells.site, SITE_BITS);
    _writer.WriteBits(cells.chunkX, CHUNK_BITS);
    _writer.WriteBits(cells.chunkZ, CHUNK_BITS);
  }
  else
  {
    _writer.WriteBits(_record.changed, CELLS_FIELD_BITS);
  }
  if (whole || (_record.changed & CELLS_FIELD_CELLS) != 0)
  {
    _writer.WriteBits(cells.cells, CHUNK_CELL_BITS);
  }
}

bool ReadRecord(Neuron::BitReader& _reader, StateRecord& _record, std::optional<StableId> _previous)
{
  _record = StateRecord{};
  if (_previous)
  {
    const auto c = static_cast<std::size_t>(_reader.ReadBits(ID_GAP_CLASS_BITS));
    const std::uint64_t gapLess1 = _reader.ReadBits(ID_GAP_BITS[c]);
    // Ids rise strictly through an update, and never wrap.
    if (gapLess1 >= ~*_previous)
    {
      _reader.Fail();
    }
    _record.id = *_previous + gapLess1 + 1;
  }
  else
  {
    _record.id = _reader.ReadBits(64);
  }
  if (_record.id == 0)
  {
    _reader.Fail();
  }
  _record.kind = static_cast<RecordKind>(_reader.ReadBounded(RECORD_KIND_MAX));
  if (_record.kind == RecordKind::Removed || _reader.Failed())
  {
    return !_reader.Failed();
  }
  const std::uint64_t objectClass = _reader.ReadBounded(REPLICATED_CLASS_MAX);
  const bool whole = _record.kind == RecordKind::Whole;
  if (objectClass == 0)
  {
    ReplicatedFigure figure;
    if (whole)
    {
      figure.team = static_cast<std::uint8_t>(_reader.ReadBits(TEAM_BITS));
      figure.kind = static_cast<FigureKind>(_reader.ReadBounded(FIGURE_KIND_MAX));
      figure.position = ReadPosition(_reader);
    }
    else
    {
      _record.changed = static_cast<std::uint8_t>(_reader.ReadBits(FIGURE_FIELD_BITS));
      if ((_record.changed & FIGURE_FIELD_POSITION) != 0)
      {
        _record.move = ReadMove(_reader);
      }
    }
    if (whole || (_record.changed & FIGURE_FIELD_YAW) != 0)
    {
      figure.yaw = static_cast<std::uint16_t>(_reader.ReadBits(YAW_BITS));
    }
    if (whole || (_record.changed & FIGURE_FIELD_HEALTH) != 0)
    {
      figure.health = static_cast<std::uint16_t>(_reader.ReadBits(HEALTH_BITS));
    }
    if (whole || (_record.changed & FIGURE_FIELD_TASK) != 0)
    {
      figure.task = static_cast<std::uint8_t>(_reader.ReadBounded(FIGURE_TASK_MAX));
    }
    if (whole || (_record.changed & FIGURE_FIELD_FLAGS) != 0)
    {
      SetFigureFlags(figure, _reader.ReadBits(FIGURE_FLAG_BITS));
    }
    if (whole || (_record.changed & FIGURE_FIELD_ORDER) != 0)
    {
      figure.order = static_cast<std::uint8_t>(_reader.ReadBounded(LEADER_ORDER_MAX));
    }
    if ((whole || (_record.changed & FIGURE_FIELD_TARGET) != 0) && _reader.ReadBits(1) != 0)
    {
      figure.target = ReadPosition(_reader);
    }
    _record.state = figure;
  }
  else if (objectClass == 1)
  {
    ReplicatedEffect effect;
    if (whole)
    {
      effect.team = static_cast<std::uint8_t>(_reader.ReadBits(TEAM_BITS));
      effect.kind = static_cast<EffectKind>(_reader.ReadBounded(EFFECT_KIND_MAX));
      effect.position = ReadPosition(_reader);
    }
    else
    {
      _record.changed = static_cast<std::uint8_t>(_reader.ReadBits(EFFECT_FIELD_BITS));
      if ((_record.changed & EFFECT_FIELD_POSITION) != 0)
      {
        _record.move = ReadMove(_reader);
      }
    }
    if (whole || (_record.changed & EFFECT_FIELD_YAW) != 0)
    {
      effect.yaw = static_cast<std::uint16_t>(_reader.ReadBits(YAW_BITS));
    }
    if (whole || (_record.changed & EFFECT_FIELD_TIMER) != 0)
    {
      effect.timer = static_cast<std::uint16_t>(_reader.ReadBits(TIMER_BITS));
    }
    if (whole)
    {
      effect.size = static_cast<std::uint16_t>(_reader.ReadBits(SIZE_BITS));
    }
    _record.state = effect;
  }
  else if (objectClass == 2)
  {
    ReplicatedOrb orb;
    if (whole)
    {
      orb.team = static_cast<std::uint8_t>(_reader.ReadBits(TEAM_BITS));
      orb.position = ReadPosition(_reader);
    }
    else
    {
      _record.changed = static_cast<std::uint8_t>(_reader.ReadBits(ORB_FIELD_BITS));
      if ((_record.changed & ORB_FIELD_POSITION) != 0)
      {
        _record.move = ReadMove(_reader);
      }
    }
    if (whole || (_record.changed & ORB_FIELD_STATE) != 0)
    {
      orb.state = static_cast<OrbState>(_reader.ReadBounded(ORB_STATE_MAX));
    }
    if (whole || (_record.changed & ORB_FIELD_TIMER) != 0)
    {
      orb.timer = static_cast<std::uint16_t>(_reader.ReadBits(TIMER_BITS));
    }
    _record.state = orb;
  }
  else if (objectClass == 3)
  {
    ReplicatedStructure structure;
    if (whole)
    {
      structure.owner = ReadOptionalTeam(_reader);
      structure.kind = static_cast<StructureKind>(_reader.ReadBounded(STRUCTURE_KIND_MAX));
      structure.position = ReadPosition(_reader);
      structure.yaw = static_cast<std::uint16_t>(_reader.ReadBits(YAW_BITS));
    }
    else
    {
      _record.changed = static_cast<std::uint8_t>(_reader.ReadBits(STRUCTURE_FIELD_BITS));
    }
    if (whole || (_record.changed & STRUCTURE_FIELD_FLAGS) != 0)
    {
      SetStructureFlags(structure, _reader.ReadBits(STRUCTURE_FLAG_BITS));
    }
    if (whole || (_record.changed & STRUCTURE_FIELD_PROGRESS) != 0)
    {
      structure.progress = static_cast<std::uint16_t>(_reader.ReadBounded(PROGRESS_PERMILLE_MAX));
    }
    if (whole || (_record.changed & STRUCTURE_FIELD_PORTS) != 0)
    {
      structure.mannedPorts = static_cast<std::uint16_t>(_reader.ReadBits(PORTS_BITS));
    }
    if (whole || (_record.changed & STRUCTURE_FIELD_TURRET) != 0)
    {
      structure.turretTeam = ReadOptionalTeam(_reader);
      structure.turretHealth = static_cast<std::uint16_t>(_reader.ReadBits(HEALTH_BITS));
    }
    if (whole || (_record.changed & STRUCTURE_FIELD_AIM) != 0)
    {
      structure.aimYaw = static_cast<std::uint16_t>(_reader.ReadBits(YAW_BITS));
      structure.aimPitch = static_cast<std::int16_t>(_reader.ReadSigned(PITCH_BITS));
    }
    _record.state = structure;
  }
  else if (objectClass == 4)
  {
    ReplicatedSettlement settlement;
    if (whole)
    {
      settlement.site = static_cast<std::uint8_t>(_reader.ReadBits(SITE_BITS));
      settlement.team = static_cast<std::uint8_t>(_reader.ReadBits(TEAM_BITS));
      settlement.color = static_cast<std::uint8_t>(_reader.ReadBits(COLOR_BITS));
    }
    else
    {
      _record.changed = static_cast<std::uint8_t>(_reader.ReadBits(SETTLEMENT_FIELD_BITS));
    }
    if (whole || (_record.changed & SETTLEMENT_FIELD_STORES) != 0)
    {
      settlement.storesMilliRu = _reader.ReadBits(STORES_BITS);
      settlement.capacityMilliRu = _reader.ReadBits(STORES_BITS);
    }
    if (whole || (_record.changed & SETTLEMENT_FIELD_POPULATION) != 0)
    {
      settlement.population = static_cast<std::uint16_t>(_reader.ReadBits(COUNT_BITS));
      settlement.limit = static_cast<std::uint16_t>(_reader.ReadBits(COUNT_BITS));
    }
    if (whole || (_record.changed & SETTLEMENT_FIELD_RADIUS) != 0)
    {
      settlement.radius = static_cast<std::uint32_t>(_reader.ReadBits(SETTLEMENT_RADIUS_BITS));
    }
    _record.state = settlement;
  }
  else
  {
    ReplicatedCells cells;
    if (whole)
    {
      cells.layer = static_cast<CellLayer>(_reader.ReadBounded(CELL_LAYER_MAX));
      cells.site = static_cast<std::uint8_t>(_reader.ReadBits(SITE_BITS));
      cells.chunkX = static_cast<std::uint8_t>(_reader.ReadBits(CHUNK_BITS));
      cells.chunkZ = static_cast<std::uint8_t>(_reader.ReadBits(CHUNK_BITS));
    }
    else
    {
      _record.changed = static_cast<std::uint8_t>(_reader.ReadBits(CELLS_FIELD_BITS));
    }
    if (whole || (_record.changed & CELLS_FIELD_CELLS) != 0)
    {
      cells.cells = _reader.ReadBits(CHUNK_CELL_BITS);
    }
    _record.state = cells;
  }
  // A delta carries at least one field, which the writer never leaves out.
  if (!whole && _record.changed == 0)
  {
    _reader.Fail();
  }
  return !_reader.Failed();
}

} // namespace Species
