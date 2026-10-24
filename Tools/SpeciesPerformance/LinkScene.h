#pragma once

#include "QuicTransport.h"
#include "StructureRecords.h"
#include "TuningMvp.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <string>

namespace Performance
{

/// What the link scene runs (M4 task T11, ADR-192).
struct LinkSceneDesc
{
  const Species::TuningMvp* tuning = nullptr;
  const Species::StructureRecords* structures = nullptr;
  std::uint16_t serverPort = 0; // the listener's port; 0: one the system chooses
  std::uint16_t linkPort = 0;   // the port the client connects to, the link simulator's; 0: the listener's own
  std::uint64_t ticks = 0;      // the server's ticks the session runs
  std::uint64_t rejoinTick = 0; // the server's tick count at which the client leaves and joins again; 0: never
};

// The freezes of one kind: each a run of ticks in which an object of the steward's view was not in the replica as the
// server's view held it, ended when it was again; the lengths in ticks 1 to FREEZE_LENGTHS - 1, and FREEZE_LENGTHS for
// that many or more.
inline constexpr std::size_t FREEZE_LENGTHS = 8;

struct Freezes
{
  std::uint64_t count = 0;
  std::uint32_t longestTicks = 0;
  std::array<std::uint64_t, FREEZE_LENGTHS> byLength{};
};

// A server end's figures over the window that excludes each connection's first OVERHEAD_SETTLE_TICKS: MsQuic's own and
// what the world sent through it. A state datagram counts once MsQuic has put it in a packet, so one still queued, or
// canceled at the close, is not charged to the link.
struct LinkWindow
{
  std::uint64_t stateDatagrams = 0;    // MsQuic's datagrams sent
  std::uint64_t datagramBytesSent = 0; // and their bytes, the message's and Species' headers included
  std::uint64_t reliableBytes = 0;     // the reliable messages the world sent, before their framing
  std::uint64_t packetsSent = 0;       // MsQuic's QUIC packets
  std::uint64_t udpBytesSent = 0;      // MsQuic's UDP payloads
  std::uint64_t streamBytesSent = 0;   // MsQuic's stream payloads, the framing included
};

/// One run of the link scene: a valley of four settlements, the steward's run by its scripted steward and watched, served
/// by the headless server's host over QuicTransport to one client, whose replica is compared with the server's view
/// after every tick (ADR-086, ADR-088).
struct LinkRun
{
  std::uint64_t ticks = 0;            // the server's ticks run
  std::uint64_t evaluatedTicks = 0;   // the ticks at which the replica was compared with the server's view
  std::uint64_t unevaluatedTicks = 0; // ticks the client passed without a view of the server's to compare with
  std::uint64_t stateDatagrams = 0;
  std::uint64_t datagramsAcknowledged = 0;
  std::uint64_t datagramsLost = 0; // reported lost, from MsQuic's first suspicion (ADR-188)
  std::uint64_t datagramsUnresolved = 0;
  Freezes figuresSingleLoss;   // a figure's freezes over which one datagram that carried it was lost
  Freezes figuresMultipleLoss; // ... two or more
  Freezes figuresNoLoss;       // ... none: a datagram late or held back, or a baseline missing
  Freezes others;              // every other object's, of every kind
  LinkWindow window;
  Neuron::QuicPathStats serverPath; // the first connection's server end, at its last sample
  Neuron::QuicPathStats clientPath; // the client's end of the last connection, at the run's end
  double joinSeconds = -1.0;        // from the first connection's start to its first state; negative: never
  double rejoinSeconds = -1.0;      // from the rejoin's connection's start to its first state; negative: none
  bool rejoined = false;
  std::uint64_t terrains = 0;     // the terrain messages received, one per connection
  std::uint64_t requestsSent = 0; // the statuses asked for, as the HUD asks
  std::uint64_t answers = 0;      // the statuses and outcomes received
  std::uint64_t missingBaselines = 0;
  std::uint64_t lateUpdates = 0;
  std::uint64_t messagesDropped = 0; // received messages that did not decode
  std::uint64_t hashesCompared = 0;
  bool hashesMatch = false; // every tick's hash equals the journal's replay's
  std::uint64_t stalls = 0; // the host's tick loop gave up real time
};

// The link scene: the session for _desc.ticks of the server's ticks, then the journal's replay. Fails when the
// certificate, the listener or the connection cannot be made, or the client never joins.
[[nodiscard]] std::expected<LinkRun, std::string> RunLink(const LinkSceneDesc& _desc);

// _run as the tool's JSON.
[[nodiscard]] std::string LinkJson(const LinkRun& _run);

} // namespace Performance
