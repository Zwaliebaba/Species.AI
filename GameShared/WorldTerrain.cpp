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
  _writer.WriteBits(commandedTeam.value_or(NO_TEAM), 8);
}

void WorldTerrain::Decode(Neuron::BitReader& _reader)
{
  seed = _reader.ReadBits(64);
  for (std::byte& value : digest)
  {
    value = static_cast<std::byte>(_reader.ReadBits(8));
  }
  const auto team = static_cast<std::uint8_t>(_reader.ReadBits(8));
  commandedTeam = team == NO_TEAM ? std::nullopt : std::optional(team);
}

} // namespace Species
