#include "pch.h"

#include "BitReader.h"
#include "JoinMessages.h"
#include "MessageCodec.h"
#include "Presence.h"
#include "ServerHost.h"
#include "SettlementMessages.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldSimulation.h"
#include "WorldTerrain.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <optional>
#include <span>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr std::uint8_t SETTLED_SITE = 2;
constexpr Neuron::ConnectionId CLIENT = 1;

Species::JoinIdentity ServerIdentity()
{
  Species::JoinIdentity identity;
  identity.build.fill(std::byte{0x42});
  identity.content.fill(std::byte{0x17});
  return identity;
}

// A world server of the valley with the steward's settlement at the always-on pace, given its identity.
Species::WorldServer::Desc JoinDesc()
{
  Species::WorldServer::Desc desc;
  desc.tuning = CommittedTuning();
  desc.structures = TestRecords();
  desc.worldSeed = ValleyTerrain()->seed;
  desc.stewardSite = SETTLED_SITE;
  desc.pace = Species::WorldPace::AlwaysOn;
  desc.identity = ServerIdentity();
  return desc;
}

template <class T> Neuron::ReceivedMessage MessageOf(const T& _message)
{
  return Neuron::ReceivedMessage{Neuron::Channel::Reliable, Neuron::EncodeMessage(_message)};
}

// What a world server sends and closes, by connection: each reliable message's bytes, in order.
class JoinSender final : public Neuron::ConnectionSender
{
public:
  std::optional<std::uint64_t> Send(Neuron::ConnectionId _connection, Neuron::Channel _channel,
                                    std::span<const std::byte> _message) override
  {
    if (_channel == Neuron::Channel::Reliable)
    {
      m_sent[_connection].emplace_back(_message.begin(), _message.end());
      return 0;
    }
    return ++m_datagrams;
  }

  void Close(Neuron::ConnectionId _connection) override
  {
    closed.push_back(_connection);
  }

  // The type of each reliable message sent to _connection, in order.
  [[nodiscard]] std::vector<Neuron::MessageTypeId> TypesTo(Neuron::ConnectionId _connection)
  {
    std::vector<Neuron::MessageTypeId> types;
    for (const std::vector<std::byte>& message : m_sent[_connection])
    {
      Neuron::BitReader reader(message);
      types.push_back(Neuron::ReadMessageHeader(reader).type);
    }
    return types;
  }

  // The messages of type T sent to _connection, decoded, in order.
  template <class T> [[nodiscard]] std::vector<T> Sent(Neuron::ConnectionId _connection)
  {
    std::vector<T> decoded;
    Neuron::MessageDispatcher dispatcher;
    dispatcher.Register<T>([&decoded](const T& _message) { decoded.push_back(_message); });
    for (const std::vector<std::byte>& message : m_sent[_connection])
    {
      static_cast<void>(dispatcher.Dispatch(message));
    }
    return decoded;
  }

  std::vector<Neuron::ConnectionId> closed;

private:
  std::map<Neuron::ConnectionId, std::vector<std::vector<std::byte>>> m_sent;
  std::uint64_t m_datagrams = 0;
};

// One tick: the step, then the publication.
void Tick(Species::WorldServer& _server, std::uint64_t _tick, Neuron::WorkerPool& _pool, JoinSender& _sender)
{
  _server.Step(_tick, _pool);
  _server.Publish(_tick, _sender);
}

std::size_t ConnectionsJournaled(const Species::WorldServer& _server)
{
  const std::vector<Species::JournalEntry>& entries = _server.Journal().value_or(Species::CommandJournal{}).entries;
  return static_cast<std::size_t>(std::ranges::count_if(entries, [](const Species::JournalEntry& _entry) { return _entry.connection; }));
}

} // namespace

TEST_CLASS(JoinTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AConnectionIsAStewardOnlyOnceItsJoinIsAccepted)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-072")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AConnectionIsAStewardOnlyOnceItsJoinIsAccepted)
  {
    // ADR-189: before its join a connection is sent nothing and counts for nothing: its commands are dropped, it is no
    // steward's presence and the journal holds no connection. A join of the server's identity is answered first, then the
    // connection is a steward, whose commands are numbered from 0.
    Species::WorldServer server(JoinDesc());
    static_cast<void>(server.Open());
    Neuron::WorkerPool pool(0);
    JoinSender sender;
    server.Connect(CLIENT);
    server.Receive(CLIENT, MessageOf(Species::SetPrioritiesCommand{SETTLED_SITE, {40, 40, 20}}));
    for (std::uint64_t tick = 0; tick < 5; ++tick)
    {
      Tick(server, tick, pool, sender);
    }
    Assert::IsTrue(sender.TypesTo(CLIENT).empty(), L"nothing is sent before the join");
    Assert::IsTrue(Species::OpenAbsence(server.Simulation()->Region(0), SETTLED_SITE) != nullptr, L"the steward stays away");
    ExpectEqual(0, static_cast<std::int64_t>(ConnectionsJournaled(server)), L"no connection is journaled");

    server.Receive(CLIENT, MessageOf(Species::JoinCommand{ServerIdentity()}));
    server.Receive(CLIENT, MessageOf(Species::SetPrioritiesCommand{SETTLED_SITE, {40, 40, 20}}));
    Tick(server, 5, pool, sender);
    const std::vector<Neuron::MessageTypeId> types = sender.TypesTo(CLIENT);
    Assert::IsTrue(!types.empty() && types.front() == Species::JoinAnswer::TYPE, L"the answer goes first");
    Assert::IsTrue(std::ranges::find(types, Species::WorldTerrain::TYPE) != types.end(), L"then the world's terrain");
    const std::vector<Species::JoinAnswer> answers = sender.Sent<Species::JoinAnswer>(CLIENT);
    Assert::IsTrue(answers.size() == 1 && answers[0].refusal == Species::JoinRefusal::None && answers[0].server == ServerIdentity(),
                   L"the client joined, with the server's identity");
    Assert::IsTrue(sender.closed.empty(), L"and stays connected");
    const std::vector<Species::CommandReport> reports = sender.Sent<Species::CommandReport>(CLIENT);
    Assert::IsTrue(reports.size() == 1 && reports[0].sequence == 0, L"its first command after the join is numbered 0");
    Assert::IsTrue(Species::OpenAbsence(server.Simulation()->Region(0), SETTLED_SITE) == nullptr, L"the steward is present");
    ExpectEqual(1, static_cast<std::int64_t>(ConnectionsJournaled(server)), L"its connection is journaled");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AClientOfAnotherBuildIsRefusedWithItsReason)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AClientOfAnotherBuildIsRefusedWithItsReason)
  {
    // ADR-189: a join of another protocol, build or content, or one that does not read, is answered with its refusal and
    // the server's identity, and its connection is closed; it never becomes a steward.
    Species::JoinIdentity protocol = ServerIdentity();
    protocol.protocol = Species::GAME_PROTOCOL_VERSION + 1;
    Species::JoinIdentity build = ServerIdentity();
    build.build[0] = std::byte{0x43};
    Species::JoinIdentity content = ServerIdentity();
    content.content[31] = std::byte{0x18};
    // A join cut short by its last byte.
    Neuron::ReceivedMessage cut = MessageOf(Species::JoinCommand{ServerIdentity()});
    cut.bytes.pop_back();
    const std::vector<std::pair<Neuron::ReceivedMessage, Species::JoinRefusal>> cases{
      {MessageOf(Species::JoinCommand{protocol}), Species::JoinRefusal::Protocol},
      {MessageOf(Species::JoinCommand{build}), Species::JoinRefusal::Build},
      {MessageOf(Species::JoinCommand{content}), Species::JoinRefusal::Content},
      {cut, Species::JoinRefusal::Protocol}};
    for (const auto& [join, expected] : cases)
    {
      Species::WorldServer server(JoinDesc());
      static_cast<void>(server.Open());
      Neuron::WorkerPool pool(0);
      JoinSender sender;
      server.Connect(CLIENT);
      server.Receive(CLIENT, join);
      server.Receive(CLIENT, MessageOf(Species::SetPrioritiesCommand{SETTLED_SITE, {40, 40, 20}}));
      Tick(server, 0, pool, sender);
      Tick(server, 1, pool, sender);
      const std::vector<Neuron::MessageTypeId> types = sender.TypesTo(CLIENT);
      const std::vector<Species::JoinAnswer> answers = sender.Sent<Species::JoinAnswer>(CLIENT);
      Assert::IsTrue(types.size() == 1 && answers.size() == 1 && answers[0].refusal == expected && answers[0].server == ServerIdentity(),
                     L"the answer alone, with the refusal and the server's identity");
      Assert::IsTrue(sender.closed == std::vector<Neuron::ConnectionId>{CLIENT}, L"and the connection is closed");
      ExpectEqual(0, static_cast<std::int64_t>(ConnectionsJournaled(server)), L"no connection is journaled");
      Assert::IsTrue(Species::OpenAbsence(server.Simulation()->Region(0), SETTLED_SITE) != nullptr, L"the steward stays away");
      server.Disconnect(CLIENT);
    }
  }
};

} // namespace GameLogicTests
