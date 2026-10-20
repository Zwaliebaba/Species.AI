#pragma once

#include "GameMessages.h"

#include <cstddef>
#include <cstdint>

namespace Species
{

/// The server's first message (M0 task T7): the tick it has just stepped, sent to every connection after each tick on
/// the state channel, where a lost report is replaced by the next. The client shows it until the UI exists.
struct TickReport
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::TickReport);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 11; // the 3-byte header and the 64-bit tick

  std::uint64_t tick = 0;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

} // namespace Species
