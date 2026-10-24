#pragma once

#include "GameMessages.h"

#include <cstddef>
#include <cstdint>

namespace Species
{

// The messages of away play (M4, ADR-181), written by hand in the form the generated codecs will take (ADR-062, ADR-093),
// as the settlement's and the relations' are (ADR-136, ADR-160).

/// MVP-072 (ADR-181): the client's word that its steward gave input, sent on the reliable channel when the input router
/// sees input and no report has gone for PRESENCE_REPORT_SECONDS. It is read, not queued: the server notes the input's
/// tick for the steward's presence and answers it as it answers a request, and it changes no simulation state itself.
struct PresenceReportCommand
{
  static constexpr Neuron::MessageTypeId TYPE = TypeIdOf(GameMessageType::PresenceReport);
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 3; // the 3-byte header

  void Encode(Neuron::BitWriter& _writer) const;
  void Decode(Neuron::BitReader& _reader);
};

// How often a client that keeps receiving input reports it: well inside mvp.presence.idle_minutes, so that a steward who
// plays is never away, and seldom enough to cost nothing on the wire.
inline constexpr double PRESENCE_REPORT_SECONDS = 15.0;

} // namespace Species
