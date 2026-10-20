#pragma once

#include "Transport.h"

#include <cstddef>
#include <deque>
#include <memory>
#include <mutex>
#include <utility>

namespace Neuron
{

// The bounds of the queues between the two ends (ADR-093).
inline constexpr std::size_t IN_PROCESS_RELIABLE_BACKLOG_BYTES = std::size_t{4} * 1024 * 1024;
inline constexpr std::size_t IN_PROCESS_STATE_QUEUE_DATAGRAMS = 64;

struct InProcessLink;

/// The transport between the client and the integrated server in one process (ADR-088 item 1, O10): bounded queues of
/// encoded buffers, with the channels, the order and the datagram fates of the remote transport, so both sides run the
/// same code over either. A state datagram is acknowledged when the other end receives it and lost when a full queue
/// drops it, the oldest first, or when the other end goes away before receiving it.
class InProcessTransport final : public Transport
{
public:
  struct Desc
  {
    std::size_t reliableBacklogBytes = IN_PROCESS_RELIABLE_BACKLOG_BYTES;
    std::size_t stateQueueDatagrams = IN_PROCESS_STATE_QUEUE_DATAGRAMS;
  };

  // The two ends of a new connection: the client's first, the server's second.
  [[nodiscard]] static std::pair<std::unique_ptr<InProcessTransport>, std::unique_ptr<InProcessTransport>> CreatePair(const Desc& _desc);

  InProcessTransport(const InProcessTransport&) = delete;
  InProcessTransport& operator=(const InProcessTransport&) = delete;
  ~InProcessTransport() override;

  [[nodiscard]] std::expected<std::uint64_t, SendError> Send(Channel _channel, std::span<const std::byte> _message) override;
  [[nodiscard]] std::optional<ReceivedMessage> Receive() override;
  [[nodiscard]] std::vector<DatagramOutcome> TakeDatagramOutcomes() override;
  void Close() override;
  [[nodiscard]] ConnectionState State() const override;
  [[nodiscard]] CloseReason Reason() const override;
  [[nodiscard]] TransportStats Stats() const override;

private:
  InProcessTransport(std::shared_ptr<InProcessLink> _link, std::size_t _side) noexcept;

  std::shared_ptr<InProcessLink> m_link;
  std::size_t m_side;
};

/// The integrated server's listener: the client connects through it, and the server accepts the other end.
class InProcessListener final : public TransportListener
{
public:
  explicit InProcessListener(const InProcessTransport::Desc& _desc);

  // The client's end of a new connection, whose server end the next Accept returns.
  [[nodiscard]] std::unique_ptr<Transport> Connect();
  [[nodiscard]] std::unique_ptr<Transport> Accept() override;

private:
  InProcessTransport::Desc m_desc;
  std::mutex m_mutex;
  std::deque<std::unique_ptr<Transport>> m_arrived;
};

} // namespace Neuron
