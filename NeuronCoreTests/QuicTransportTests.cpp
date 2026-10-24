#include "pch.h"

#include "MessageCodec.h"
#include "PlatformClock.h"
#include "QuicTransport.h"
#include "ServerCertificate.h"
#include "TestSupport.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <optional>
#include <set>
#include <thread>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

// The certificate the tests' servers present, kept in the user's store between runs as a server's is (ADR-188).
constexpr std::wstring_view TEST_CERTIFICATE = L"Species transport test";
// How long a test waits for MsQuic's threads before it fails: far above a loopback round trip.
constexpr std::chrono::milliseconds WAIT{10'000};

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

bool IsError(const std::expected<std::uint64_t, Neuron::SendError>& _result, Neuron::SendError _error)
{
  return !_result.has_value() && _result.error() == _error;
}

// Polls _done every millisecond until it holds or WAIT has passed.
template <class Fn> bool WaitFor(Fn&& _done)
{
  const Neuron::CounterTimeSource clock;
  while (!_done())
  {
    if (clock.Now() > WAIT)
    {
      return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  return true;
}

// The next message of an end, waited for.
std::optional<Neuron::ReceivedMessage> NextOf(Neuron::Transport& _end)
{
  std::optional<Neuron::ReceivedMessage> message;
  static_cast<void>(WaitFor([&] { return (message = _end.Receive()).has_value(); }));
  return message;
}

Neuron::ServerCertificate TestCertificate()
{
  std::expected<Neuron::ServerCertificate, std::string> certificate = Neuron::ServerCertificate::OpenOrCreate(TEST_CERTIFICATE);
  Assert::IsTrue(certificate.has_value(), L"the test certificate opens or is made");
  return std::move(certificate).value();
}

// A listener on a port the system chooses, its client and the server's end of their connection, once the handshake has
// completed and the peer takes datagrams.
struct Loopback
{
  Neuron::ServerCertificate certificate;
  std::unique_ptr<Neuron::QuicListener> listener;
  std::unique_ptr<Neuron::QuicTransport> client;
  std::unique_ptr<Neuron::Transport> server;
};

std::unique_ptr<Neuron::QuicListener> StartListener(const Neuron::ServerCertificate& _certificate)
{
  std::expected<std::unique_ptr<Neuron::QuicListener>, std::string> listener = Neuron::QuicListener::Start({0, &_certificate});
  Assert::IsTrue(listener.has_value(), L"the listener starts");
  return std::move(listener).value();
}

std::unique_ptr<Neuron::QuicTransport> ConnectTo(const Neuron::QuicListener& _listener, const Neuron::Sha256Digest& _fingerprint)
{
  std::expected<std::unique_ptr<Neuron::QuicTransport>, std::string> client =
    Neuron::QuicTransport::Connect({_listener.Port(), _fingerprint});
  Assert::IsTrue(client.has_value(), L"the client's connection starts");
  return std::move(client).value();
}

Loopback Connected()
{
  Loopback loopback{TestCertificate(), nullptr, nullptr, nullptr};
  loopback.listener = StartListener(loopback.certificate);
  Assert::IsTrue(loopback.listener->Port() != 0, L"it listens on a port");
  loopback.client = ConnectTo(*loopback.listener, loopback.certificate.Fingerprint());
  Assert::IsTrue(WaitFor([&] { return (loopback.server = loopback.listener->Accept()) != nullptr; }), L"the server accepts it");
  Assert::IsTrue(WaitFor(
                   [&]
                   {
                     const std::optional<Neuron::QuicPathStats> path = loopback.client->PathStats();
                     return loopback.client->Connected() && path.has_value() && path->datagramsEnabled;
                   }),
                 L"the client is connected and the server takes datagrams");
  return loopback;
}

} // namespace

TEST_CLASS(QuicTransportTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071ALoopbackConnectionCarriesBothChannelsBothWays)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071ALoopbackConnectionCarriesBothChannelsBothWays)
  {
    // ADR-188: each reliable message arrives once and in order, each datagram at most once, and each datagram's fate is
    // reported once; the figures count what was sent and received. Across the two channels QUIC keeps no order.
    Loopback loopback = Connected();
    const std::array<std::pair<Neuron::Transport*, Neuron::Transport*>, 2> directions{
      std::pair<Neuron::Transport*, Neuron::Transport*>{loopback.client.get(), loopback.server.get()},
      {loopback.server.get(), loopback.client.get()}};
    for (const auto& [sender, receiver] : directions)
    {
      ExpectEqual(0, static_cast<std::int64_t>(sender->Send(Neuron::Channel::Reliable, Message(1)).value_or(99)), L"reliable");
      ExpectEqual(1, static_cast<std::int64_t>(sender->Send(Neuron::Channel::State, Message(2)).value_or(99)), L"the first datagram");
      ExpectEqual(0, static_cast<std::int64_t>(sender->Send(Neuron::Channel::Reliable, Message(3)).value_or(99)), L"reliable");
      ExpectEqual(2, static_cast<std::int64_t>(sender->Send(Neuron::Channel::State, Message(4)).value_or(99)), L"the second datagram");
      std::vector<std::uint32_t> reliable;
      std::vector<std::uint32_t> state;
      Assert::IsTrue(WaitFor(
                       [&]
                       {
                         while (std::optional<Neuron::ReceivedMessage> message = receiver->Receive())
                         {
                           (message->channel == Neuron::Channel::Reliable ? reliable : state).push_back(ValueOf(message->bytes));
                         }
                         return reliable.size() == 2 && state.size() == 2;
                       }),
                     L"all four arrive on a loopback link");
      Assert::IsTrue(reliable == std::vector<std::uint32_t>{1, 3}, L"the reliable messages in their order");
      std::ranges::sort(state);
      Assert::IsTrue(state == std::vector<std::uint32_t>{2, 4}, L"each datagram once");
      std::vector<Neuron::DatagramOutcome> outcomes;
      Assert::IsTrue(WaitFor(
                       [&]
                       {
                         const std::vector<Neuron::DatagramOutcome> taken = sender->TakeDatagramOutcomes();
                         outcomes.insert(outcomes.end(), taken.begin(), taken.end());
                         return outcomes.size() >= 2;
                       }),
                     L"both fates are decided");
      std::ranges::sort(outcomes, {}, &Neuron::DatagramOutcome::datagram);
      Assert::IsTrue(outcomes.size() == 2 && outcomes[0].datagram == 1 && outcomes[0].fate == Neuron::DatagramFate::Acknowledged &&
                       outcomes[1].datagram == 2 && outcomes[1].fate == Neuron::DatagramFate::Acknowledged,
                     L"both datagrams are acknowledged, once each");
      Assert::IsTrue(WaitFor([&] { return sender->Stats().reliableBacklogBytes == 0; }), L"the reliable bytes are acknowledged");
      const Neuron::TransportStats stats = sender->Stats();
      Assert::IsTrue(stats.messagesSent == 4 && stats.bytesSent == 16 && stats.datagramsAcknowledged == 2, L"the sender's figures");
      ExpectEqual(4, static_cast<std::int64_t>(receiver->Stats().messagesReceived), L"the receiver's figures");
    }
    Assert::IsTrue(loopback.client->State() == Neuron::ConnectionState::Open && loopback.server->Reason() == Neuron::CloseReason::None,
                   L"still open");
    const std::optional<Neuron::QuicPathStats> path = loopback.client->PathStats();
    Assert::IsTrue(path.has_value() && path->packetsSent > 0 && path->udpBytesSent > path->packetsSent, L"MsQuic's figures");
    // ADR-088 item 3: a state datagram of STATE_DATAGRAM_MAX_BYTES never depends on path MTU discovery.
    Assert::IsTrue(path.has_value() && path->datagramMaxSendBytes >= Neuron::STATE_DATAGRAM_MAX_BYTES,
                   L"MsQuic sends a datagram of the largest state datagram");
    Assert::IsTrue(loopback.client->Send(Neuron::Channel::State, std::vector<std::byte>(Neuron::STATE_DATAGRAM_MAX_BYTES)).has_value(),
                   L"and the client sends one");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071ReliableMessagesKeepTheirOrderAcrossTheStream)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071ReliableMessagesKeepTheirOrderAcrossTheStream)
  {
    // ADR-088 item 5: each message is framed by its length, however QUIC cuts the stream; the largest message crosses too.
    Loopback loopback = Connected();
    constexpr std::uint32_t COUNT = 1'000;
    for (std::uint32_t value = 1; value <= COUNT; ++value)
    {
      Assert::IsTrue(loopback.client->Send(Neuron::Channel::Reliable, Message(value, 4 + (value % 300))).has_value(), L"sent");
    }
    Assert::IsTrue(loopback.client->Send(Neuron::Channel::Reliable, Message(COUNT + 1, Neuron::MESSAGE_MAX_BYTES)).has_value(),
                   L"the largest message is sent");
    for (std::uint32_t value = 1; value <= COUNT + 1; ++value)
    {
      const std::optional<Neuron::ReceivedMessage> message = NextOf(*loopback.server);
      const std::size_t size = value <= COUNT ? 4 + (value % 300) : Neuron::MESSAGE_MAX_BYTES;
      Assert::IsTrue(message.has_value() && message->channel == Neuron::Channel::Reliable && ValueOf(message->bytes) == value &&
                       message->bytes.size() == size,
                     L"each message whole, once, in order");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071EitherEndClosesAfterItsMessages)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071EitherEndClosesAfterItsMessages)
  {
    // The reliable messages sent before a Close still reach the other end, which then sees the connection closed by its
    // peer; the end that closed refuses to send more. The client's first message opens the reliable channel, as its join
    // does (ADR-189): a server end has none before it.
    for (const bool clientCloses : {true, false})
    {
      Loopback loopback = Connected();
      Assert::IsTrue(loopback.client->Send(Neuron::Channel::Reliable, Message(1)).has_value(), L"the client's first message");
      const std::optional<Neuron::ReceivedMessage> first = NextOf(*loopback.server);
      Assert::IsTrue(first.has_value() && ValueOf(first->bytes) == 1, L"opens the reliable channel");
      Neuron::Transport& closer = clientCloses ? static_cast<Neuron::Transport&>(*loopback.client) : *loopback.server;
      Neuron::Transport& other = clientCloses ? *loopback.server : static_cast<Neuron::Transport&>(*loopback.client);
      Assert::IsTrue(closer.Send(Neuron::Channel::Reliable, Message(7)).has_value(), L"the last message is sent");
      closer.Close();
      Assert::IsTrue(closer.State() == Neuron::ConnectionState::Closed && closer.Reason() == Neuron::CloseReason::ClosedLocally,
                     L"the end that closed is closed at once");
      Assert::IsTrue(IsError(closer.Send(Neuron::Channel::Reliable, Message(8)), Neuron::SendError::Closed), L"and sends no more");
      const std::optional<Neuron::ReceivedMessage> last = NextOf(other);
      Assert::IsTrue(last.has_value() && ValueOf(last->bytes) == 7, L"the message sent before the close arrives");
      Assert::IsTrue(WaitFor([&] { return other.State() == Neuron::ConnectionState::Closed; }), L"the other end sees the close");
      Assert::IsTrue(other.Reason() == Neuron::CloseReason::ClosedByPeer, L"by its peer");
      Assert::IsTrue(IsError(other.Send(Neuron::Channel::State, Message(9)), Neuron::SendError::Closed), L"and sends no more");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AChangedCertificateIsRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AChangedCertificateIsRefused)
  {
    // ADR-088 item 4: the client takes only the server whose certificate it pins, and refuses another before any message.
    const Neuron::ServerCertificate certificate = TestCertificate();
    const std::unique_ptr<Neuron::QuicListener> listener = StartListener(certificate);
    Neuron::Sha256Digest other = certificate.Fingerprint();
    other[0] ^= std::byte{0x01};
    const std::unique_ptr<Neuron::QuicTransport> client = ConnectTo(*listener, other);
    Assert::IsTrue(WaitFor([&] { return client->State() == Neuron::ConnectionState::Closed; }), L"the connection closes");
    Assert::IsTrue(client->CertificateRefused() && !client->Connected(), L"because the certificate is not the pinned one");
    Assert::IsTrue(client->Reason() == Neuron::CloseReason::ProtocolError, L"which is a protocol error");
    Assert::IsTrue(listener->Accept() == nullptr, L"and the server accepts no connection");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071OversizedAndEmptyMessagesAreRefused)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071OversizedAndEmptyMessagesAreRefused)
  {
    Loopback loopback = Connected();
    Assert::IsTrue(IsError(loopback.client->Send(Neuron::Channel::Reliable, {}), Neuron::SendError::Empty), L"an empty message");
    Assert::IsTrue(IsError(loopback.client->Send(Neuron::Channel::Reliable, std::vector<std::byte>(Neuron::MESSAGE_MAX_BYTES + 1)),
                           Neuron::SendError::TooLarge),
                   L"a reliable message above the limit");
    Assert::IsTrue(IsError(loopback.client->Send(Neuron::Channel::State, std::vector<std::byte>(Neuron::STATE_DATAGRAM_MAX_BYTES + 1)),
                           Neuron::SendError::TooLarge),
                   L"a state datagram above its limit");
    ExpectEqual(0, static_cast<std::int64_t>(loopback.client->Stats().messagesSent), L"none of them counts as sent");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071EachDatagramHasOneFate)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071EachDatagramHasOneFate)
  {
    // ADR-088 item 2: every datagram sent is reported acknowledged or lost exactly once, also when the connection ends
    // while some are in flight.
    Loopback loopback = Connected();
    constexpr std::uint64_t COUNT = 200;
    for (std::uint32_t value = 1; value <= COUNT; ++value)
    {
      ExpectEqual(static_cast<std::int64_t>(value),
                  static_cast<std::int64_t>(loopback.client->Send(Neuron::Channel::State, Message(value, 1'000)).value_or(0)),
                  L"each datagram's number");
    }
    loopback.server.reset();
    std::vector<Neuron::DatagramOutcome> outcomes;
    Assert::IsTrue(WaitFor(
                     [&]
                     {
                       const std::vector<Neuron::DatagramOutcome> taken = loopback.client->TakeDatagramOutcomes();
                       outcomes.insert(outcomes.end(), taken.begin(), taken.end());
                       return outcomes.size() >= COUNT;
                     }),
                   L"every fate is decided");
    std::set<std::uint64_t> numbers;
    for (const Neuron::DatagramOutcome& outcome : outcomes)
    {
      numbers.insert(outcome.datagram);
    }
    Assert::IsTrue(outcomes.size() == COUNT && numbers.size() == COUNT && *numbers.begin() == 1 && *numbers.rbegin() == COUNT,
                   L"once for each datagram");
    const Neuron::TransportStats stats = loopback.client->Stats();
    ExpectEqual(static_cast<std::int64_t>(COUNT), static_cast<std::int64_t>(stats.datagramsAcknowledged + stats.datagramsLost),
                L"the figures count each fate");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AReceiverThatTakesNothingStopsTheSender)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AReceiverThatTakesNothingStopsTheSender)
  {
    // ADR-188: an end holds at most QUIC_RELIABLE_BACKLOG_BYTES received and not taken before QUIC's flow control holds
    // the sender, whose own backlog is bounded the same, so the sender is told to wait; once the receiver takes the
    // messages, every one arrives in order and the sender goes on.
    Loopback loopback = Connected();
    constexpr std::size_t SIZE = Neuron::MESSAGE_MAX_BYTES;
    constexpr std::uint32_t LIMIT = 1'000; // 64 MiB: far beyond both bounds and MsQuic's own windows
    const Neuron::CounterTimeSource clock;
    std::uint32_t sent = 0;
    Neuron::Nanoseconds heldSince = clock.Now();
    bool held = false;
    while (sent < LIMIT)
    {
      if (loopback.client->Send(Neuron::Channel::Reliable, Message(sent + 1, SIZE)).has_value())
      {
        ++sent;
        heldSince = clock.Now();
        continue;
      }
      if (clock.Now() - heldSince > std::chrono::seconds(1))
      {
        held = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    Assert::IsTrue(held, L"the sender is held before it has sent without bound");
    Assert::IsTrue(loopback.client->Stats().reliableBacklogBytes <= Neuron::QUIC_RELIABLE_BACKLOG_BYTES, L"within its backlog");
    for (std::uint32_t value = 1; value <= sent; ++value)
    {
      const std::optional<Neuron::ReceivedMessage> message = NextOf(*loopback.server);
      Assert::IsTrue(message.has_value() && ValueOf(message->bytes) == value && message->bytes.size() == SIZE,
                     L"each message once, in order, once the receiver takes them");
    }
    Assert::IsTrue(WaitFor([&] { return loopback.client->Send(Neuron::Channel::Reliable, Message(sent + 1)).has_value(); }),
                   L"and the sender goes on");
    const std::optional<Neuron::ReceivedMessage> after = NextOf(*loopback.server);
    Assert::IsTrue(after.has_value() && ValueOf(after->bytes) == sent + 1, L"with the next message");
  }
};

} // namespace NeuronCoreTests
