#include "pch.h"

#include "TickReport.h"

namespace Species
{

void TickReport::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(tick, 64);
}

void TickReport::Decode(Neuron::BitReader& _reader)
{
  tick = _reader.ReadBits(64);
}

} // namespace Species
