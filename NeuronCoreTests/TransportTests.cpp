#include "pch.h"

#include "InProcessTransport.h"
#include "MessageCodec.h"
#include "PlatformThread.h"
#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <thread>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

std::vector<std::byte> Message(std::uint32_t _value, std::size_t _size = 4)
{
  std::vector<std::byte> bytes(_size);
  std::memcpy(bytes.data(), &_value, std::min(_size, sizeof(_value)));
  return bytes;
}

std::uint32_t ValueOf(const std::vector<std::byte>& _bytes)
{
  std::uint32_t value = 0;
  std::memcpy(&value, _bytes.data(), std::min(_bytes.size(), sizeof(value)));
  return value;
}

Neuron::InProcessTransport::Desc Bounds(std::size_t _reliableBytes, std::size_t _datagrams)
{
  Neuron::InProcessTransport::Desc desc;
  desc.reliableBacklogBytes = _reliableBytes;
  desc.stateQueueDatagrams = _datagrams;
  return desc;
}

// The smallest message: a header and no fields.
struct TestPing
{
  static constexpr Neuron::MessageTypeId TYPE = 1;
  static constexpr std::uint8_t VERSION = 1;
  static constexpr std::size_t MAX_BYTES = 8;

  void Encode(Neuron::BitWriter&) const {}
  void Decode(Neuron::BitReader&) {}
};

bool IsError(const std::expected<std::uint64_t, Neuron::SendError>& _result, Neuron::SendError _error)
{
  return !_result.has_value() && _result.error() == _error;
}

} // namespace

TEST_CLASS(TransportTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090SendAndReceiveBothWaysOnBothChannels)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090SendAndReceiveBothWaysOnBothChannels)
  {
    auto [client, server] = Neuron::InProcessTransport::CreatePair({});
    const std::array<std::pair<Neuron::Transport*, Neuron::Transport*>, 2> directions{
      std::pair<Neuron::Transport*, Neuron::Transport*>{client.get(), server.get()}, {server.get(), client.get()}};
    for (const auto& [sender, receiver] : directions)
    {
      ExpectEqual(0, static_cast<std::int64_t>(sender->Send(Neuron::Channel::Reliable, Message(1)).value_or(99)), L"reliable");
      ExpectEqual(1, static_cast<std::int64_t>(sender->Send(Neuron::Channel::State, Message(2)).value_or(99)), L"the first datagram");
      ExpectEqual(0, static_cast<std::int64_t>(sender->Send(Neuron::Channel::Reliable, Message(3)).value_or(99)), L"reliable");
      ExpectEqual(2, static_cast<std::int64_t>(sender->Send(Neuron::Channel::State, Message(4)).value_or(99)), L"the second datagram");
      Assert::IsTrue(sender->TakeDatagramOutcomes().empty(), L"no fate before the datagrams are received");
      const std::array expected{std::pair{Neuron::Channel::Reliable, 1u}, std::pair{Neuron::Channel::State, 2u},
                                std::pair{Neuron::Channel::Reliable, 3u}, std::pair{Neuron::Channel::State, 4u}};
      for (const auto& [channel, value] : expected)
      {
        const std::optional<Neuron::ReceivedMessage> message = receiver->Receive();
        Assert::IsTrue(message.has_value() && message->channel == channel && ValueOf(message->bytes) == value,
                       L"each message arrives once, in order, on its channel");
      }
      Assert::IsFalse(receiver->Receive().has_value(), L"and nothing more");
      const std::vector<Neuron::DatagramOutcome> outcomes = sender->TakeDatagramOutcomes();
      Assert::IsTrue(outcomes.size() == 2 && outcomes[0].datagram == 1 && outcomes[0].fate == Neuron::DatagramFate::Acknowledged &&
                       outcomes[1].datagram == 2 && outcomes[1].fate == Neuron::DatagramFate::Acknowledged,
                     L"both datagrams are acknowledged");
      const Neuron::TransportStats stats = sender->Stats();
      Assert::IsTrue(stats.messagesSent == 4 && stats.bytesSent == 16 && stats.datagramsAcknowledged == 2 &&
                       stats.reliableBacklogBytes == 0,
                     L"the sender's figures");
      ExpectEqual(4, static_cast<std::int64_t>(receiver->Stats().messagesReceived), L"the receiver's figures");
    }
    Assert::IsTrue(client->State() == Neuron::ConnectionState::Open && server->Reason() == Neuron::CloseReason::None, L"still open");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090EitherEndCloses)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090EitherEndCloses)
  {
    for (const bool clientCloses : {true, false})
    {
      auto [client, server] = Neuron::InProcessTransport::CreatePair({});
      Neuron::Transport& closer = clientCloses ? *client : *server;
      Neuron::Transport& other = clientCloses ? *server : *client;
      Assert::IsTrue(closer.Send(Neuron::Channel::Reliable, Message(7)).has_value(), L"sent before the close");
      closer.Close();
      Assert::IsTrue(closer.State() == Neuron::ConnectionState::Closed && closer.Reason() == Neuron::CloseReason::ClosedLocally,
                     L"closed here");
      Assert::IsTrue(other.State() == Neuron::ConnectionState::Closed && other.Reason() == Neuron::CloseReason::ClosedByPeer,
                     L"closed by the other end");
      const std::optional<Neuron::ReceivedMessage> last = other.Receive();
      Assert::IsTrue(last.has_value() && ValueOf(last->bytes) == 7, L"a message sent before the close still arrives");
      Assert::IsTrue(IsError(closer.Send(Neuron::Channel::Reliable, Message(8)), Neuron::SendError::Closed), L"no send after the close");
      Assert::IsTrue(IsError(other.Send(Neuron::Channel::State, Message(9)), Neuron::SendError::Closed), L"nor from the other end");
      closer.Close();
      Assert::IsTrue(closer.Reason() == Neuron::CloseReason::ClosedLocally, L"closing twice changes nothing");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090FullStateQueueDropsTheOldest)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090FullStateQueueDropsTheOldest)
  {
    auto [client, server] = Neuron::InProcessTransport::CreatePair(Bounds(1024, 2));
    for (std::uint32_t value = 1; value <= 3; ++value)
    {
      Assert::IsTrue(server->Send(Neuron::Channel::State, Message(value)).has_value(), L"a datagram is never refused for a full queue");
    }
    const std::vector<Neuron::DatagramOutcome> lost = server->TakeDatagramOutcomes();
    Assert::IsTrue(lost.size() == 1 && lost[0].datagram == 1 && lost[0].fate == Neuron::DatagramFate::Lost, L"the oldest is lost");
    const std::optional<Neuron::ReceivedMessage> second = client->Receive();
    const std::optional<Neuron::ReceivedMessage> third = client->Receive();
    Assert::IsTrue(second.has_value() && ValueOf(second->bytes) == 2 && third.has_value() && ValueOf(third->bytes) == 3,
                   L"the newest arrive");
    ExpectEqual(1, static_cast<std::int64_t>(server->Stats().datagramsLost), L"counted as lost");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090ReliableBacklogIsBounded)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090ReliableBacklogIsBounded)
  {
    auto [client, server] = Neuron::InProcessTransport::CreatePair(Bounds(100, 8));
    Assert::IsTrue(client->Send(Neuron::Channel::Reliable, Message(1, 60)).has_value(), L"60 of 100 bytes");
    Assert::IsTrue(IsError(client->Send(Neuron::Channel::Reliable, Message(2, 60)), Neuron::SendError::Backlogged),
                   L"120 of 100 bytes waits with its sender");
    ExpectEqual(60, static_cast<std::int64_t>(client->Stats().reliableBacklogBytes), L"the backlog");
    Assert::IsTrue(server->Receive().has_value(), L"received");
    Assert::IsTrue(client->Send(Neuron::Channel::Reliable, Message(2, 60)).has_value(), L"sent once the backlog has room");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090OversizedAndEmptyMessagesAreRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090OversizedAndEmptyMessagesAreRefused)
  {
    auto [client, server] = Neuron::InProcessTransport::CreatePair({});
    const std::vector<std::byte> datagram(Neuron::STATE_DATAGRAM_MAX_BYTES + 1);
    Assert::IsTrue(IsError(server->Send(Neuron::Channel::State, datagram), Neuron::SendError::TooLarge), L"1,201 bytes of state");
    Assert::IsTrue(server->Send(Neuron::Channel::Reliable, datagram).has_value(), L"which the reliable channel takes");
    const std::vector<std::byte> message(Neuron::MESSAGE_MAX_BYTES + 1);
    Assert::IsTrue(IsError(server->Send(Neuron::Channel::Reliable, message), Neuron::SendError::TooLarge), L"above the message limit");
    Assert::IsTrue(IsError(server->Send(Neuron::Channel::Reliable, {}), Neuron::SendError::Empty), L"an empty message");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090AnEndThatGoesAwayLosesItsUnreadDatagrams)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090AnEndThatGoesAwayLosesItsUnreadDatagrams)
  {
    auto [client, server] = Neuron::InProcessTransport::CreatePair({});
    Assert::IsTrue(server->Send(Neuron::Channel::State, Message(1)).has_value() &&
                     server->Send(Neuron::Channel::State, Message(2)).has_value(),
                   L"two datagrams sent");
    client.reset();
    const std::vector<Neuron::DatagramOutcome> outcomes = server->TakeDatagramOutcomes();
    Assert::IsTrue(outcomes.size() == 2 && outcomes[0].fate == Neuron::DatagramFate::Lost && outcomes[1].fate == Neuron::DatagramFate::Lost,
                   L"both datagrams are lost");
    Assert::IsTrue(server->Reason() == Neuron::CloseReason::ClosedByPeer, L"and the connection is closed");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090ListenerAcceptsWhatConnects)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090ListenerAcceptsWhatConnects)
  {
    Neuron::InProcessListener listener({});
    Assert::IsTrue(listener.Accept() == nullptr, L"nothing to accept");
    const std::unique_ptr<Neuron::Transport> client = listener.Connect();
    const std::unique_ptr<Neuron::Transport> server = listener.Accept();
    Assert::IsTrue(server != nullptr && listener.Accept() == nullptr, L"one connection, accepted once");
    Assert::IsTrue(client->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(TestPing{})).has_value(), L"sent");
    Neuron::MessageDispatcher dispatcher;
    bool pinged = false;
    dispatcher.Register<TestPing>([&pinged](const TestPing&) { pinged = true; });
    const std::optional<Neuron::ReceivedMessage> message = server->Receive();
    Assert::IsTrue(message.has_value() && dispatcher.Dispatch(message->bytes) && pinged, L"an encoded message crosses whole");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp090OrderHoldsBetweenTwoThreads)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp090OrderHoldsBetweenTwoThreads)
  {
    // The integrated server's thread and the client's: one sends while the other receives, through a small backlog.
    auto [client, server] = Neuron::InProcessTransport::CreatePair(Bounds(256, 8));
    constexpr std::uint32_t MESSAGES = 20'000;
    Neuron::Transport& sender = *server;
    Neuron::PlatformThread thread =
      Neuron::PlatformThread::Start("test.sender",
                                    [&sender](const std::stop_token&)
                                    {
                                      for (std::uint32_t value = 0; value < MESSAGES;)
                                      {
                                        if (sender.Send(Neuron::Channel::Reliable, Message(value)).has_value())
                                        {
                                          ++value;
                                        }
                                        else
                                        {
                                          std::this_thread::yield();
                                        }
                                      }
                                    });
    std::uint32_t expected = 0;
    bool ordered = true;
    while (expected < MESSAGES)
    {
      if (const std::optional<Neuron::ReceivedMessage> message = client->Receive())
      {
        ordered = ordered && ValueOf(message->bytes) == expected;
        ++expected;
      }
      else
      {
        std::this_thread::yield();
      }
    }
    thread.Join();
    Assert::IsTrue(ordered, L"every message arrives in its order");
  }
};

} // namespace NeuronCoreTests
