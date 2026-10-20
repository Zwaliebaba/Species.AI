#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <optional>
#include <span>
#include <vector>

namespace Neuron
{

// ADR-088 item 3: one state datagram fills at most one QUIC DATAGRAM frame, 8 bytes of Species' header (the update's
// tick and its baseline tick) and at most 1,192 bytes of records.
inline constexpr std::size_t STATE_DATAGRAM_MAX_BYTES = 1200;
inline constexpr std::size_t STATE_DATAGRAM_HEADER_BYTES = 8;

// ADR-088 item 2: the reliable, ordered channel carries commands, guaranteed events, results and replies; the state
// channel carries state updates down and control states up, at most once each, in order, never resent.
enum class Channel : std::uint8_t
{
  Reliable,
  State
};

enum class ConnectionState : std::uint8_t
{
  Open,
  Closed
};

enum class CloseReason : std::uint8_t
{
  None,          // still open
  ClosedLocally, // this end called Close
  ClosedByPeer,  // the other end closed or went away
  ProtocolError  // the other end sent what the protocol forbids (ADR-088 item 5)
};

enum class SendError : std::uint8_t
{
  Closed,    // the connection is closed
  TooLarge,  // above MESSAGE_MAX_BYTES, or a state datagram above STATE_DATAGRAM_MAX_BYTES
  Empty,     // a message has at least its header
  Backlogged // the reliable backlog is full: the caller keeps the message and sends it later
};

// A state datagram's fate (ADR-088 item 2): an acknowledged update becomes the receiver's baseline (ADR-086).
enum class DatagramFate : std::uint8_t
{
  Acknowledged,
  Lost
};

struct DatagramOutcome
{
  std::uint64_t datagram = 0; // the number its Send returned
  DatagramFate fate = DatagramFate::Lost;
};

struct ReceivedMessage
{
  Channel channel = Channel::Reliable;
  std::vector<std::byte> bytes;
};

// ADR-088 item 7: the per-connection figures the transport itself knows.
struct TransportStats
{
  std::uint64_t messagesSent = 0;
  std::uint64_t messagesReceived = 0;
  std::uint64_t bytesSent = 0;
  std::uint64_t bytesReceived = 0;
  std::uint64_t datagramsAcknowledged = 0;
  std::uint64_t datagramsLost = 0;
  std::uint64_t reliableBacklogBytes = 0; // sent and not yet received
};

/// One end of one connection between a client and a server (ADR-088 item 1, ADR-084): the only way state and commands
/// cross between them. Messages are encoded buffers (MessageCodec.h); the transport neither reads nor changes them. Every
/// call returns without waiting. One thread uses an end at a time; the two ends of a connection may be on two threads.
class Transport
{
public:
  virtual ~Transport() = default;

  // Queues one message. Returns the datagram's number on the state channel, whose fate TakeDatagramOutcomes reports,
  // and 0 on the reliable channel.
  [[nodiscard]] virtual std::expected<std::uint64_t, SendError> Send(Channel _channel, std::span<const std::byte> _message) = 0;
  // The next message of either channel, in the order they arrived. Messages that arrived before the connection
  // closed can still be received after it.
  [[nodiscard]] virtual std::optional<ReceivedMessage> Receive() = 0;
  // The fates of this end's state datagrams decided since the last call, in the order they were decided.
  [[nodiscard]] virtual std::vector<DatagramOutcome> TakeDatagramOutcomes() = 0;
  // Closes both directions. Reliable messages already sent still reach the other end; nothing more is sent.
  virtual void Close() = 0;

  [[nodiscard]] virtual ConnectionState State() const = 0;
  [[nodiscard]] virtual CloseReason Reason() const = 0;
  [[nodiscard]] virtual TransportStats Stats() const = 0;
};

/// Where a server accepts its connections.
class TransportListener
{
public:
  virtual ~TransportListener() = default;
  // The next connection that has arrived, or nullptr; never waits.
  [[nodiscard]] virtual std::unique_ptr<Transport> Accept() = 0;
};

} // namespace Neuron
