#include "pch.h"

#include "ActorGridCache.h"

#include <bit>
#include <cmath>
#include <utility>

namespace Species
{

namespace
{

// The cache holds at most one entry per Worker query of the last ticks; a state that claims more is no state.
constexpr std::uint64_t ENTRIES_MAX = std::uint64_t{1} << 24;
constexpr std::uint64_t REFS_MAX = std::uint64_t{1} << 24;

void WriteDouble(Neuron::BitWriter& _writer, double _value)
{
  _writer.WriteBits(std::bit_cast<std::uint64_t>(_value), 64);
}

bool ReadDouble(Neuron::BitReader& _reader, double& _value)
{
  _value = std::bit_cast<double>(_reader.ReadBits(64));
  return std::isfinite(_value);
}

} // namespace

void ActorGridCache::Evict(std::uint64_t _tick)
{
  std::erase_if(m_entries, [this, _tick](const Entry& _entry) { return _tick - _entry.tick > m_desc.maxAgeTicks; });
}

void ActorGridCache::WriteState(Neuron::BitWriter& _writer) const
{
  _writer.WriteBounded(m_entries.size(), ENTRIES_MAX);
  for (const Entry& entry : m_entries)
  {
    WriteDouble(_writer, entry.x);
    WriteDouble(_writer, entry.z);
    WriteDouble(_writer, entry.rangeSquared);
    WriteDouble(_writer, entry.halfRangeSquared);
    _writer.WriteBits(entry.team, 8);
    _writer.WriteBits(entry.tick, 64);
    _writer.WriteBounded(entry.refs.size(), REFS_MAX);
    for (const ActorRef& ref : entry.refs)
    {
      _writer.WriteBits(ref.id, 64);
      _writer.WriteBits(ref.slot, 32);
      _writer.WriteBits(ref.team, 8);
    }
  }
}

bool ActorGridCache::ReadState(Neuron::BitReader& _reader)
{
  m_entries.clear();
  const std::uint64_t count = _reader.ReadBounded(ENTRIES_MAX);
  for (std::uint64_t n = 0; n < count && !_reader.Failed(); ++n)
  {
    Entry entry;
    bool finite = ReadDouble(_reader, entry.x);
    finite = ReadDouble(_reader, entry.z) && finite;
    finite = ReadDouble(_reader, entry.rangeSquared) && finite;
    finite = ReadDouble(_reader, entry.halfRangeSquared) && finite;
    entry.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
    entry.tick = _reader.ReadBits(64);
    const std::uint64_t refs = _reader.ReadBounded(REFS_MAX);
    for (std::uint64_t r = 0; r < refs && !_reader.Failed(); ++r)
    {
      ActorRef ref;
      ref.id = _reader.ReadBits(64);
      ref.slot = static_cast<std::uint32_t>(_reader.ReadBits(32));
      ref.team = static_cast<std::uint8_t>(_reader.ReadBits(8));
      entry.refs.push_back(ref);
    }
    if (!finite)
    {
      return false;
    }
    m_entries.push_back(std::move(entry));
  }
  return !_reader.Failed();
}

} // namespace Species
