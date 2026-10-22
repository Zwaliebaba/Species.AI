#pragma once

#include "EffectEvent.h"
#include "GameMessages.h"
#include "Transport.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Species
{

// ADR-084 item 5: a droppable presentation event older than 500 ms is not played.
inline constexpr double PRESENTATION_EVENT_LIFE_SECONDS = 0.5;
inline constexpr std::uint8_t EFFECT_EVENT_KIND_MAX = static_cast<std::uint8_t>(EffectEventKind::TurretDestroyed);

/// The droppable presentation events of one tick for one client (ADR-084 item 5, ADR-111), in the order the rules
/// emitted them, on the state channel: a lost datagram loses its events, and nothing is sent again. A position travels
/// at the replicated 1/16 wu, a vector and a value as 32-bit floats; a field the event leaves at zero is not sent.
struct PresentationEvents
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::PresentationEvents);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = Neuron::STATE_DATAGRAM_MAX_BYTES;
  static constexpr std::uint32_t TICK_BITS = 48;
  static constexpr std::size_t MAX_EVENTS = 511;
  static constexpr std::size_t HEADER_BITS =
    Neuron::MESSAGE_TYPE_BITS + Neuron::MESSAGE_VERSION_BITS + TICK_BITS + Neuron::BitsFor(MAX_EVENTS);

  std::uint64_t tick = 0;
  std::vector<EffectEvent> events;

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);

  [[nodiscard]] static std::size_t EventBits(const EffectEvent& _event);
};

// _events of tick _tick in as few messages as hold them, in order.
[[nodiscard]] std::vector<PresentationEvents> PackPresentationEvents(std::uint64_t _tick, std::span<const EffectEvent> _events);

// The kinds a client sees only for the team it commands (ADR-106): the selection and order events of R-UNIT-136 to
// R-UNIT-193.
[[nodiscard]] bool IsCommandersEvent(EffectEventKind _kind) noexcept;

} // namespace Species
