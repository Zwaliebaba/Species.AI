#pragma once

#include "GameMessages.h"
#include "ReplicatedState.h"
#include "Transport.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Species
{

// ADR-086 item 3: the server keeps each client's sent states for 32 ticks, and a record whose acknowledged state is
// older goes whole; the client keeps what it received for twice as long, so a baseline the server names is always there.
inline constexpr std::uint64_t STATE_BASELINE_TICKS_MAX = 32;
inline constexpr std::uint64_t STATE_HISTORY_TICKS = 2 * STATE_BASELINE_TICKS_MAX;

/// One state datagram of a tick's update for one client (ADR-086 item 3, ADR-088 item 3, ADR-111), on the state
/// channel: Species' 8-byte header, the update's tick and the age of its baseline, then records in stable-id order. Each
/// datagram decodes alone: a delta record is against the client's state of its object at the baseline tick, which the
/// client has acknowledged, and a whole or removal record needs no baseline. An age of 0 names no baseline, and then the
/// update holds no delta.
struct StateUpdate
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::StateUpdate);
  static constexpr std::uint8_t VERSION = 2; // 2: a Leader's flag-pointer target (ADR-125)
  static constexpr std::size_t MAX_BYTES = Neuron::STATE_DATAGRAM_MAX_BYTES;
  static constexpr std::uint32_t TICK_BITS = 48;
  static constexpr std::uint32_t BASELINE_AGE_BITS = 16;
  static constexpr std::size_t MAX_RECORDS = 1023;
  // The message header, Species' header and the record count: what a datagram holds besides its records.
  static constexpr std::size_t HEADER_BITS =
    Neuron::MESSAGE_TYPE_BITS + Neuron::MESSAGE_VERSION_BITS + TICK_BITS + BASELINE_AGE_BITS + Neuron::BitsFor(MAX_RECORDS);

  std::uint64_t tick = 0;
  std::uint16_t baselineAge = 0;
  std::vector<StateRecord> records;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);

  // The baseline tick a delta of this update is against; none at age 0.
  [[nodiscard]] std::optional<std::uint64_t> BaselineTick() const noexcept;
};

// _records, in stable-id order, in as few updates of tick _tick and baseline age _baselineAge as hold them, each within
// StateUpdate::MAX_BYTES and MAX_RECORDS, filled in order.
[[nodiscard]] std::vector<StateUpdate> PackStateUpdates(std::uint64_t _tick, std::uint16_t _baselineAge,
                                                        std::span<const StateRecord> _records);

} // namespace Species
