#include "pch.h"

#include "PresentationEvents.h"
#include "ReplicatedState.h"

#include <bit>
#include <cmath>

namespace Species
{

namespace
{

constexpr std::uint32_t TEAM_BITS = 8;
constexpr std::uint32_t FLOAT_BITS = 32;
constexpr std::size_t WHOLE_POSITION_BITS = 2 * POSITION_HORIZONTAL_BITS + POSITION_VERTICAL_BITS;

bool IsZero(const Neuron::Vec3& _vector) noexcept
{
  return _vector.x == 0.0 && _vector.y == 0.0 && _vector.z == 0.0;
}

void WriteFloat(Neuron::BitWriter& _writer, double _value)
{
  _writer.WriteBits(std::bit_cast<std::uint32_t>(static_cast<float>(_value)), FLOAT_BITS);
}

// A float that is not finite is read as 0: an event is presentation, and drawing it never reads a NaN.
double ReadFloat(Neuron::BitReader& _reader) noexcept
{
  const float value = std::bit_cast<float>(static_cast<std::uint32_t>(_reader.ReadBits(FLOAT_BITS)));
  return std::isfinite(value) ? static_cast<double>(value) : 0.0;
}

} // namespace

void PresentationEvents::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(tick, TICK_BITS);
  _writer.WriteBounded(events.size(), MAX_EVENTS);
  for (const EffectEvent& event : events)
  {
    _writer.WriteBounded(static_cast<std::uint64_t>(event.kind), EFFECT_EVENT_KIND_MAX);
    _writer.WriteBits(event.team, TEAM_BITS);
    _writer.WriteBool(event.subject != 0);
    if (event.subject != 0)
    {
      _writer.WriteBits(event.subject, 64);
    }
    _writer.WriteBool(!IsZero(event.position));
    if (!IsZero(event.position))
    {
      const QuantizedPosition position = QuantizePosition(event.position);
      _writer.WriteSigned(position.x, POSITION_HORIZONTAL_BITS);
      _writer.WriteSigned(position.y, POSITION_VERTICAL_BITS);
      _writer.WriteSigned(position.z, POSITION_HORIZONTAL_BITS);
    }
    _writer.WriteBool(!IsZero(event.vector));
    if (!IsZero(event.vector))
    {
      WriteFloat(_writer, event.vector.x);
      WriteFloat(_writer, event.vector.y);
      WriteFloat(_writer, event.vector.z);
    }
    _writer.WriteBool(event.value != 0.0);
    if (event.value != 0.0)
    {
      WriteFloat(_writer, event.value);
    }
  }
}

void PresentationEvents::Decode(Neuron::BitReader& _reader)
{
  tick = _reader.ReadBits(TICK_BITS);
  const auto count = static_cast<std::size_t>(_reader.ReadBounded(MAX_EVENTS));
  events.clear();
  if (_reader.Failed())
  {
    return;
  }
  events.reserve(count);
  for (std::size_t n = 0; n < count && !_reader.Failed(); ++n)
  {
    EffectEvent event;
    const std::uint64_t kind = _reader.ReadBounded(EFFECT_EVENT_KIND_MAX);
    if (kind == 0)
    {
      _reader.Fail(); // the kinds start at 1
    }
    else
    {
      event.kind = static_cast<EffectEventKind>(kind);
    }
    event.team = static_cast<std::uint8_t>(_reader.ReadBits(TEAM_BITS));
    if (_reader.ReadBool())
    {
      event.subject = _reader.ReadBits(64);
    }
    if (_reader.ReadBool())
    {
      QuantizedPosition position;
      position.x = static_cast<std::int32_t>(_reader.ReadSigned(POSITION_HORIZONTAL_BITS));
      position.y = static_cast<std::int32_t>(_reader.ReadSigned(POSITION_VERTICAL_BITS));
      position.z = static_cast<std::int32_t>(_reader.ReadSigned(POSITION_HORIZONTAL_BITS));
      event.position = PositionOf(position);
    }
    if (_reader.ReadBool())
    {
      event.vector.x = ReadFloat(_reader);
      event.vector.y = ReadFloat(_reader);
      event.vector.z = ReadFloat(_reader);
    }
    if (_reader.ReadBool())
    {
      event.value = ReadFloat(_reader);
    }
    events.push_back(event);
  }
}

std::size_t PresentationEvents::EventBits(const EffectEvent& _event)
{
  std::size_t bits = Neuron::BitsFor(EFFECT_EVENT_KIND_MAX) + TEAM_BITS + 4;
  bits += _event.subject != 0 ? 64 : 0;
  bits += IsZero(_event.position) ? 0 : WHOLE_POSITION_BITS;
  bits += IsZero(_event.vector) ? 0 : 3 * FLOAT_BITS;
  bits += _event.value != 0.0 ? FLOAT_BITS : 0;
  return bits;
}

std::vector<PresentationEvents> PackPresentationEvents(std::uint64_t _tick, std::span<const EffectEvent> _events)
{
  constexpr std::size_t LIMIT_BITS = PresentationEvents::MAX_BYTES * 8;
  std::vector<PresentationEvents> messages;
  std::size_t bits = 0;
  for (const EffectEvent& event : _events)
  {
    const std::size_t more = PresentationEvents::EventBits(event);
    if (messages.empty() || messages.back().events.size() >= PresentationEvents::MAX_EVENTS || bits + more > LIMIT_BITS)
    {
      messages.push_back(PresentationEvents{_tick, {}});
      bits = PresentationEvents::HEADER_BITS;
    }
    messages.back().events.push_back(event);
    bits += more;
  }
  return messages;
}

bool IsCommandersEvent(EffectEventKind _kind) noexcept
{
  switch (_kind)
  {
  case EffectEventKind::WorkerPromoted:
  case EffectEventKind::LeaderSelected:
  case EffectEventKind::LeaderOrderWaypoint:
  case EffectEventKind::LeaderOrderFormation:
  case EffectEventKind::WorkersSelected:
  case EffectEventKind::WorkerOrdered:
  case EffectEventKind::WorkersOrderedByPlayer:
    return true;
  default:
    return false;
  }
}

} // namespace Species
