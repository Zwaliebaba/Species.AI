#include "pch.h"

#include "Check.h"
#include "StateUpdate.h"

#include <optional>

namespace Species
{

void StateUpdate::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(tick, TICK_BITS);
  _writer.WriteBits(baselineAge, BASELINE_AGE_BITS);
  _writer.WriteBounded(records.size(), MAX_RECORDS);
  std::optional<StableId> previous;
  for (const StateRecord& record : records)
  {
    ENGINE_CHECK(record.kind != RecordKind::Delta || baselineAge != 0, "a delta needs a baseline");
    WriteRecord(_writer, record, previous);
    previous = record.id;
  }
}

void StateUpdate::Decode(Neuron::BitReader& _reader)
{
  tick = _reader.ReadBits(TICK_BITS);
  baselineAge = static_cast<std::uint16_t>(_reader.ReadBits(BASELINE_AGE_BITS));
  const auto count = static_cast<std::size_t>(_reader.ReadBounded(MAX_RECORDS));
  records.clear();
  if (_reader.Failed())
  {
    return;
  }
  // The count is bounded, so the records take a bounded allocation whatever the datagram holds.
  records.reserve(count);
  std::optional<StableId> previous;
  for (std::size_t n = 0; n < count && !_reader.Failed(); ++n)
  {
    StateRecord record;
    if (!ReadRecord(_reader, record, previous))
    {
      return;
    }
    // A delta against no baseline, or a baseline before the first tick, cannot be applied.
    if (record.kind == RecordKind::Delta && (baselineAge == 0 || baselineAge > tick))
    {
      _reader.Fail();
      return;
    }
    previous = record.id;
    records.push_back(record);
  }
}

std::optional<std::uint64_t> StateUpdate::BaselineTick() const noexcept
{
  if (baselineAge == 0 || baselineAge > tick)
  {
    return std::nullopt;
  }
  return tick - baselineAge;
}

std::vector<StateUpdate> PackStateUpdates(std::uint64_t _tick, std::uint16_t _baselineAge, std::span<const StateRecord> _records)
{
  constexpr std::size_t LIMIT_BITS = StateUpdate::MAX_BYTES * 8;
  std::vector<StateUpdate> updates;
  std::size_t bits = 0;
  for (const StateRecord& record : _records)
  {
    StateUpdate* current = updates.empty() ? nullptr : &updates.back();
    if (current != nullptr)
    {
      const std::size_t more = RecordBits(record, current->records.back().id);
      if (current->records.size() < StateUpdate::MAX_RECORDS && bits + more <= LIMIT_BITS)
      {
        current->records.push_back(record);
        bits += more;
        continue;
      }
    }
    // A record opens a datagram with its whole id; one record always fits an empty datagram.
    updates.push_back(StateUpdate{_tick, _baselineAge, {record}});
    bits = StateUpdate::HEADER_BITS + RecordBits(record, std::nullopt);
    ENGINE_CHECK(bits <= LIMIT_BITS, "a record fits a datagram alone");
  }
  return updates;
}

} // namespace Species
