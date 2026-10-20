#pragma once

#include "GameMessages.h"
#include "Sha256.h"

#include <cstddef>
#include <cstdint>

namespace Species
{

/// The world's terrain as the server built it (MVP-011, ADR-102): the seed that gives it and the digest of its region,
/// sent once to each connection on the reliable channel before its first tick report. The client builds the region
/// from the seed with its own library and data, and refuses the world when its digest differs.
struct WorldTerrain
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::WorldTerrain);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 43; // the 3-byte header, the 64-bit seed and the 256-bit digest

  std::uint64_t seed = 0;
  Neuron::Sha256Digest digest{};

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

} // namespace Species
