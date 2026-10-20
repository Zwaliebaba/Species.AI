#include "pch.h"

#include "WorldTerrain.h"

namespace Species
{

void WorldTerrain::Encode(Neuron::BitWriter& _writer) const
{
  _writer.WriteBits(seed, 64);
  for (const std::byte value : digest)
  {
    _writer.WriteBits(static_cast<std::uint64_t>(value), 8);
  }
}

void WorldTerrain::Decode(Neuron::BitReader& _reader)
{
  seed = _reader.ReadBits(64);
  for (std::byte& value : digest)
  {
    value = static_cast<std::byte>(_reader.ReadBits(8));
  }
}

} // namespace Species
