#include "pch.h"

#include "WorldRecord.h"

namespace Species
{

namespace
{

// The freezes a record holds: one a minute for a run of 8 hours is 480.
constexpr std::uint64_t FREEZES_MAX = 65'535;

} // namespace

void WriteWorldRecord(Neuron::BitWriter& _writer, const WorldRecord& _record)
{
  _writer.WriteSigned(_record.savedAtSeconds, 64);
  _writer.WriteBounded(_record.freezes.size(), FREEZES_MAX);
  for (const FreezeRecord& freeze : _record.freezes)
  {
    _writer.WriteBits(freeze.tick, 64);
    _writer.WriteBounded(static_cast<std::uint64_t>(freeze.cause), FREEZE_CAUSE_MAX);
    _writer.WriteBits(freeze.seconds, 64);
  }
}

bool ReadWorldRecord(Neuron::BitReader& _reader, WorldRecord& _record)
{
  _record.savedAtSeconds = _reader.ReadSigned(64);
  _record.freezes.resize(_reader.ReadBounded(FREEZES_MAX));
  for (FreezeRecord& freeze : _record.freezes)
  {
    freeze.tick = _reader.ReadBits(64);
    freeze.cause = static_cast<FreezeCause>(_reader.ReadBounded(FREEZE_CAUSE_MAX));
    freeze.seconds = _reader.ReadBits(64);
  }
  return !_reader.Failed();
}

} // namespace Species
