#pragma once

#include "GameMessages.h"
#include "Sha256.h"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace Species
{

/// The world's terrain as the server built it (MVP-011, ADR-102): the seed that gives it and the digest of its region,
/// sent once to each connection on the reliable channel before its first tick report. The client builds the region
/// from the seed with its own library and data, and refuses the world when its digest differs. It also names the team
/// that the connection commands (ADR-106), the client's own team, which its picking and cursor read (ADR-119).
struct WorldTerrain
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::WorldTerrain);
  static constexpr std::uint8_t VERSION = 2;
  static constexpr std::size_t MAX_BYTES = 44;  // the 3-byte header, the 64-bit seed, the 256-bit digest and the team
  static constexpr std::uint8_t NO_TEAM = 0xFF; // on the wire: the connection commands no team

  std::uint64_t seed = 0;
  Neuron::Sha256Digest digest{};
  std::optional<std::uint8_t> commandedTeam;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

} // namespace Species
