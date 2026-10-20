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
constexpr std::uint32_t FIGURE_FLAG_BITS = 5;
constexpr std::uint32_t FIGURE_FIELD_BITS = 6;
constexpr std::uint32_t EFFECT_FIELD_BITS = 3;
constexpr std::uint32_t ORB_FIELD_BITS = 3;
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
         (_figure.selected ? 16u : 0u);
}

void SetFigureFlags(ReplicatedFigure& _figure, std::uint64_t _flags) noexcept
{
  _figure.dead = (_flags & 1u) != 0;
  _figure.onGround = (_flags & 2u) != 0;
  _figure.underControl = (_flags & 4u) != 0;
  _figure.formation = (_flags & 8u) != 0;
  _figure.selected = (_flags & 16u) != 0;
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
  else
  {
    const auto& orb = std::get<ReplicatedOrb>(_state);
    const auto& baseline = std::get<ReplicatedOrb>(*_baseline);
    ReplicatedOrb delta;
    record.changed = OrbChanges(orb, baseline);
    delta.state = (record.changed & ORB_FIELD_STATE) != 0 ? orb.state : OrbState::Birth;
    delta.timer = (record.changed & ORB_FIELD_TIMER) != 0 ? orb.timer : std::uint16_t{0};
    record.move = Difference(orb.position, baseline.position);
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
  else
  {
    auto& orb = std::get<ReplicatedOrb>(state);
    const auto& delta = std::get<ReplicatedOrb>(_record.state);
    if ((_record.changed & ORB_FIELD_POSITION) != 0)
    {
      orb.position = Sum(orb.position, _record.move);
    }
    orb.state = (_record.changed & ORB_FIELD_STATE) != 0 ? delta.state : orb.state;
    orb.timer = (_record.changed & ORB_FIELD_TIMER) != 0 ? delta.timer : orb.timer;
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
    if (whole)
    {
      return bits + TEAM_BITS + Neuron::BitsFor(FIGURE_KIND_MAX) + WHOLE_POSITION_BITS + YAW_BITS + HEALTH_BITS +
             Neuron::BitsFor(FIGURE_TASK_MAX) + FIGURE_FLAG_BITS + Neuron::BitsFor(LEADER_ORDER_MAX);
    }
    bits += FIGURE_FIELD_BITS;
    bits += (_record.changed & FIGURE_FIELD_POSITION) != 0 ? MoveBits(_record.move) : 0;
    bits += (_record.changed & FIGURE_FIELD_YAW) != 0 ? YAW_BITS : 0;
    bits += (_record.changed & FIGURE_FIELD_HEALTH) != 0 ? HEALTH_BITS : 0;
    bits += (_record.changed & FIGURE_FIELD_TASK) != 0 ? Neuron::BitsFor(FIGURE_TASK_MAX) : 0;
    bits += (_record.changed & FIGURE_FIELD_FLAGS) != 0 ? FIGURE_FLAG_BITS : 0;
    bits += (_record.changed & FIGURE_FIELD_ORDER) != 0 ? Neuron::BitsFor(LEADER_ORDER_MAX) : 0;
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
  const auto& orb = std::get<ReplicatedOrb>(_record.state);
  if (whole)
  {
    _writer.WriteBits(orb.team, TEAM_BITS);
    WritePosition(_writer, orb.position);
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
    _writer.WriteBounded(static_cast<std::uint64_t>(orb.state), ORB_STATE_MAX);
  }
  if (whole || (_record.changed & ORB_FIELD_TIMER) != 0)
  {
    _writer.WriteBits(orb.timer, TIMER_BITS);
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
  else
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
  // A delta carries at least one field, which the writer never leaves out.
  if (!whole && _record.changed == 0)
  {
    _reader.Fail();
  }
  return !_reader.Failed();
}

} // namespace Species
