#pragma once

#include "GameMessages.h"

#include <cstddef>
#include <cstdint>

namespace Species
{

/// The server's tick statistics for the development overlay (M1 task T15b, ADR-120), sent in Debug builds to each
/// connection on the reliable channel once a second of ticks: the world's seed and latest tick, the ticks of the period,
/// and the mean and longest of their wall time from the world's step to the end of its replication (ADR-080 item 6). A
/// Release server sends none.
struct ServerStats
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::ServerStats);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 31; // the 3-byte header, two 64-bit and three 32-bit fields

  std::uint64_t worldSeed = 0;
  std::uint64_t tick = 0;
  std::uint32_t ticks = 0;
  std::uint32_t tickMeanMicroseconds = 0;
  std::uint32_t tickMaxMicroseconds = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

} // namespace Species
