#include "pch.h"

#include "InProcessTransport.h"
#include "MessageCodec.h"

#include <array>
#include <exception>
#include <utility>

namespace Neuron
{

/// The state the two ends share, behind one lock.
struct InProcessLink
{
  struct Inbound
  {
    Channel channel = Channel::Reliable;
    std::uint64_t datagram = 0;
    std::vector<std::byte> bytes;
  };

  struct Side
  {
    std::deque<Inbound> inbound;          // sent by the other end, not yet received here
    std::size_t inboundReliableBytes = 0; // the other end's reliable backlog
    std::size_t inboundDatagrams = 0;
    std::vector<DatagramOutcome> outcomes; // the fates of this end's own datagrams
    std::uint64_t nextDatagram = 1;
    bool closed = false;
    CloseReason reason = CloseReason::None;
    TransportStats stats;
  };

  InProcessTransport::Desc desc;
  mutable std::mutex mutex;
  std::array<Side, 2> sides;
};

std::pair<std::unique_ptr<InProcessTransport>, std::unique_ptr<InProcessTransport>> InProcessTransport::CreatePair(const Desc& _desc)
{
  auto link = std::make_shared<InProcessLink>();
  link->desc = _desc;
  return {std::unique_ptr<InProcessTransport>(new InProcessTransport(link, 0)),
          std::unique_ptr<InProcessTransport>(new InProcessTransport(link, 1))};
}

InProcessTransport::InProcessTransport(std::shared_ptr<InProcessLink> _link, std::size_t _side) noexcept
  : m_link(std::move(_link)),
    m_side(_side)
{
}

InProcessTransport::~InProcessTransport()
{
  try
  {
    Close();
    // The datagrams this end will now never receive are lost for their sender.
    const std::lock_guard lock(m_link->mutex);
    InProcessLink::Side& self = m_link->sides[m_side];
    InProcessLink::Side& peer = m_link->sides[1 - m_side];
    for (const InProcessLink::Inbound& entry : self.inbound)
    {
      if (entry.channel == Channel::State)
      {
        peer.outcomes.push_back({entry.datagram, DatagramFate::Lost});
        ++peer.stats.datagramsLost;
      }
    }
    self.inbound.clear();
    self.inboundReliableBytes = 0;
    self.inboundDatagrams = 0;
  }
  catch (...)
  {
    std::terminate(); // a lock or an allocation that fails here is a defect, and it ends through the fatal path
  }
}

std::expected<std::uint64_t, SendError> InProcessTransport::Send(Channel _channel, std::span<const std::byte> _message)
{
  const std::lock_guard lock(m_link->mutex);
  InProcessLink::Side& self = m_link->sides[m_side];
  InProcessLink::Side& peer = m_link->sides[1 - m_side];
  if (self.closed)
  {
    return std::unexpected(SendError::Closed);
  }
  if (_message.empty())
  {
    return std::unexpected(SendError::Empty);
  }
  if (_message.size() > MESSAGE_MAX_BYTES || (_channel == Channel::State && _message.size() > STATE_DATAGRAM_MAX_BYTES))
  {
    return std::unexpected(SendError::TooLarge);
  }
  std::uint64_t datagram = 0;
  if (_channel == Channel::Reliable)
  {
    if (peer.inboundReliableBytes + _message.size() > m_link->desc.reliableBacklogBytes)
    {
      return std::unexpected(SendError::Backlogged);
    }
    peer.inboundReliableBytes += _message.size();
  }
  else
  {
    datagram = self.nextDatagram++;
    if (peer.inboundDatagrams >= m_link->desc.stateQueueDatagrams)
    {
      // A full queue drops the oldest datagram, as a congested link would: the newest state is the one that matters.
      for (auto entry = peer.inbound.begin(); entry != peer.inbound.end(); ++entry)
      {
        if (entry->channel == Channel::State)
        {
          self.outcomes.push_back({entry->datagram, DatagramFate::Lost});
          ++self.stats.datagramsLost;
          peer.inbound.erase(entry);
          --peer.inboundDatagrams;
          break;
        }
      }
    }
    ++peer.inboundDatagrams;
  }
  peer.inbound.push_back({_channel, datagram, std::vector<std::byte>(_message.begin(), _message.end())});
  ++self.stats.messagesSent;
  self.stats.bytesSent += _message.size();
  return datagram;
}

std::optional<ReceivedMessage> InProcessTransport::Receive()
{
  const std::lock_guard lock(m_link->mutex);
  InProcessLink::Side& self = m_link->sides[m_side];
  InProcessLink::Side& peer = m_link->sides[1 - m_side];
  if (self.inbound.empty())
  {
    return std::nullopt;
  }
  InProcessLink::Inbound entry = std::move(self.inbound.front());
  self.inbound.pop_front();
  if (entry.channel == Channel::Reliable)
  {
    self.inboundReliableBytes -= entry.bytes.size();
  }
  else
  {
    --self.inboundDatagrams;
    peer.outcomes.push_back({entry.datagram, DatagramFate::Acknowledged});
    ++peer.stats.datagramsAcknowledged;
  }
  ++self.stats.messagesReceived;
  self.stats.bytesReceived += entry.bytes.size();
  return ReceivedMessage{entry.channel, std::move(entry.bytes)};
}

std::vector<DatagramOutcome> InProcessTransport::TakeDatagramOutcomes()
{
  const std::lock_guard lock(m_link->mutex);
  return std::exchange(m_link->sides[m_side].outcomes, {});
}

void InProcessTransport::Close()
{
  const std::lock_guard lock(m_link->mutex);
  InProcessLink::Side& self = m_link->sides[m_side];
  InProcessLink::Side& peer = m_link->sides[1 - m_side];
  if (!self.closed)
  {
    self.closed = true;
    self.reason = CloseReason::ClosedLocally;
  }
  if (!peer.closed)
  {
    peer.closed = true;
    peer.reason = CloseReason::ClosedByPeer;
  }
}

ConnectionState InProcessTransport::State() const
{
  const std::lock_guard lock(m_link->mutex);
  return m_link->sides[m_side].closed ? ConnectionState::Closed : ConnectionState::Open;
}

CloseReason InProcessTransport::Reason() const
{
  const std::lock_guard lock(m_link->mutex);
  return m_link->sides[m_side].reason;
}

TransportStats InProcessTransport::Stats() const
{
  const std::lock_guard lock(m_link->mutex);
  TransportStats stats = m_link->sides[m_side].stats;
  stats.reliableBacklogBytes = m_link->sides[1 - m_side].inboundReliableBytes;
  return stats;
}

InProcessListener::InProcessListener(const InProcessTransport::Desc& _desc)
  : m_desc(_desc)
{
}

std::unique_ptr<Transport> InProcessListener::Connect()
{
  std::pair ends = InProcessTransport::CreatePair(m_desc);
  const std::lock_guard lock(m_mutex);
  m_arrived.push_back(std::move(ends.second));
  return std::move(ends.first);
}

std::unique_ptr<Transport> InProcessListener::Accept()
{
  const std::lock_guard lock(m_mutex);
  if (m_arrived.empty())
  {
    return nullptr;
  }
  std::unique_ptr<Transport> connection = std::move(m_arrived.front());
  m_arrived.pop_front();
  return connection;
}

} // namespace Neuron
