#include "pch.h"

#include "ServerStats.h"

namespace Species
{

void ServerStats::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(worldSeed, 64);
  _writer.WriteBits(tick, 64);
  _writer.WriteBits(ticks, 32);
  _writer.WriteBits(tickMeanMicroseconds, 32);
  _writer.WriteBits(tickMaxMicroseconds, 32);
}

void ServerStats::Decode(Neuron::BitReader& _reader)
{
  worldSeed = _reader.ReadBits(64);
  tick = _reader.ReadBits(64);
  ticks = static_cast<std::uint32_t>(_reader.ReadBits(32));
  tickMeanMicroseconds = static_cast<std::uint32_t>(_reader.ReadBits(32));
  tickMaxMicroseconds = static_cast<std::uint32_t>(_reader.ReadBits(32));
}

} // namespace Species
