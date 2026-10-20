#include "pch.h"

#include "CommandJournal.h"
#include "CommandMessages.h"
#include "Development.h"
#include "InProcessTransport.h"
#include "MessageCodec.h"
#include "PresentationEvents.h"
#include "RegionSimulation.h"
#include "Sandbox.h"
#include "ServerHost.h"
#include "StateHash.h"
#include "StateReplica.h"
#include "StateSender.h"
#include "StateUpdate.h"
#include "TestSupport.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldSimulation.h"
#include "WorldTerrain.h"

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameLogicTests
{

namespace
{

constexpr Neuron::ConnectionId CLIENT_CONNECTION = 1;
constexpr Neuron::SipHashKey REPLICATION_TEST_KEY{0x7777777777777777, 0x0f0f0f0f0f0f0f0f};
// The numbers this test gives the datagrams it drops, apart from the transport's own.
constexpr std::uint64_t DROPPED_DATAGRAM_FIRST = std::uint64_t{1} << 62;

Species::StableId Id(std::uint64_t _counter)
{
  return Species::MakeStableId(Species::REGION_OWNER_ID_FIRST, _counter);
}

/// The integrated client of ADR-084's M1 test: the client's end of an in-process connection and its replica, with the
/// host's part of each tick done by hand, as ServerHost does it, so that a test can drop datagrams and skip frames.
class AttachedClient final : private Neuron::ConnectionSender
{
public:
  explicit AttachedClient(Species::WorldServer& _server)
    : m_server(_server)
  {
    auto [client, server] = Neuron::InProcessTransport::CreatePair({});
    m_client = std::move(client);
    m_serverEnd = std::move(server);
    m_dispatcher.Register<Species::StateUpdate>([this](const Species::StateUpdate& _update) { replica.Apply(_update); });
    m_dispatcher.Register<Species::PresentationEvents>([this](const Species::PresentationEvents& _events)
                                                       { events += _events.events.size(); });
    m_dispatcher.Register<Species::CommandReport>([this](const Species::CommandReport&) { ++reports; });
    m_dispatcher.Register<Species::WorldTerrain>([](const Species::WorldTerrain&) {});
    m_server.Connect(CLIENT_CONNECTION);
  }

  // What decides whether the nth state datagram of the run is dropped on the way; none drops nothing.
  std::function<bool(std::uint64_t)> drop;

  // The client's command, on the reliable channel.
  template <class T> void SendCommand(const T& _command)
  {
    Assert::IsTrue(m_client->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(_command)).has_value(), L"the command is sent");
  }

  // The start of a tick: the messages that arrived, and the fates of the datagrams sent.
  void Deliver()
  {
    while (std::optional<Neuron::ReceivedMessage> message = m_serverEnd->Receive())
    {
      m_server.Receive(CLIENT_CONNECTION, *message);
    }
    std::vector<Neuron::DatagramOutcome> outcomes = m_serverEnd->TakeDatagramOutcomes();
    outcomes.insert(outcomes.end(), m_dropped.begin(), m_dropped.end());
    m_dropped.clear();
    if (!outcomes.empty())
    {
      m_server.DatagramFates(CLIENT_CONNECTION, outcomes);
    }
  }

  void Publish(std::uint64_t _tick)
  {
    m_server.Publish(_tick, *this);
  }

  // The client's frame: every message that has arrived.
  void Read()
  {
    while (std::optional<Neuron::ReceivedMessage> message = m_client->Receive())
    {
      Assert::IsTrue(m_dispatcher.Dispatch(message->bytes), L"every message the server sends decodes");
    }
  }

  Species::StateReplica replica;
  std::uint64_t events = 0;
  std::uint64_t reports = 0;

private:
  [[nodiscard]] std::optional<std::uint64_t> Send(Neuron::ConnectionId, Neuron::Channel _channel,
                                                  std::span<const std::byte> _message) override
  {
    if (_channel == Neuron::Channel::State && drop && drop(m_stateDatagrams++))
    {
      // Lost on the way: the transport reports its fate as lost.
      const std::uint64_t number = DROPPED_DATAGRAM_FIRST + m_stateDatagrams;
      m_dropped.push_back({number, Neuron::DatagramFate::Lost});
      return number;
    }
    const auto sent = m_serverEnd->Send(_channel, _message);
    return sent ? std::optional<std::uint64_t>{*sent} : std::nullopt;
  }

  Species::WorldServer& m_server;
  std::unique_ptr<Neuron::Transport> m_client;
  std::unique_ptr<Neuron::Transport> m_serverEnd;
  Neuron::MessageDispatcher m_dispatcher;
  std::vector<Neuron::DatagramOutcome> m_dropped;
  std::uint64_t m_stateDatagrams = 0;
};

// What the server's state is for the steward's client, by stable id.
std::map<Species::StableId, Species::ReplicatedState> ServerView(const Species::WorldServer& _server)
{
  const Species::RegionSimulation& region = _server.Simulation()->Region(0);
  const Species::StableId steward = region.CommanderOf(Species::SANDBOX_STEWARD_TEAM).sender;
  std::map<Species::StableId, Species::ReplicatedState> view;
  for (const Species::ReplicatedObject& object : Species::ViewForSteward(Species::CollectReplicatedObjects(region), region, steward))
  {
    view.emplace(object.id, object.state);
  }
  return view;
}

// The first figure of _team in the client's replica.
Species::StableId FirstFigureOf(const Species::StateReplica& _replica, std::uint8_t _team)
{
  for (const auto& [id, state] : _replica.Latest())
  {
    if (const auto* figure = std::get_if<Species::ReplicatedFigure>(&state); figure != nullptr && figure->team == _team)
    {
      return id;
    }
  }
  return 0;
}

// The per-tick hashes of _server's journal replayed headless, with presentation stubs: no client at all.
std::vector<std::uint64_t> ReplayedHashes(const Species::WorldServer& _server, std::uint64_t _endTick, Neuron::WorkerPool& _pool)
{
  const Species::CommandJournal journal = _server.Journal().value_or(Species::CommandJournal{});
  const auto terrain =
    std::make_shared<const Species::RegionTerrain>(*Species::GenerateRegionTerrain(journal.start.seed, CommittedTuning()));
  std::vector<std::uint64_t> hashes;
  static_cast<void>(Species::ReplayJournal(journal, terrain, CommittedTuning(), _endTick, _pool,
                                           [&hashes](const Species::WorldSimulation& _world)
                                           { hashes.push_back(Species::HashWorld(_world).world); }));
  return hashes;
}

} // namespace

TEST_CLASS(ReplicationTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr084AnIntegratedClientChangesNoTicksHash)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr084AnIntegratedClientChangesNoTicksHash)
  {
    // ADR-084's M1 test (DEC-222 item 4): the sandbox with an integrated client attached, which reads the state every
    // tick and steers its side from what its replica shows, gives the same hash on every tick as its command stream
    // replayed on the headless server with presentation stubs. The replica holds the server's state after every tick.
    Species::WorldServer server({.tuning = CommittedTuning(), .worldSeed = 1, .sandbox = true});
    const std::uint64_t first = server.Open();
    AttachedClient client(server);
    const Neuron::Vec3& aiSite = server.Terrain()->layout.sites[static_cast<std::size_t>(CommittedTuning().mvp.sandbox.aiSite)];
    const Neuron::Vec3& stewardSite = server.Terrain()->layout.sites[static_cast<std::size_t>(CommittedTuning().mvp.sandbox.stewardSite)];
    Neuron::WorkerPool pool(2);
    std::vector<std::uint64_t> hashes;
    for (std::uint64_t tick = first; tick < first + 300; ++tick)
    {
      switch (tick - first)
      {
      case 5:
        client.SendCommand(
          Species::PromoteWorkerCommand{Species::SANDBOX_STEWARD_TEAM, FirstFigureOf(client.replica, Species::SANDBOX_STEWARD_TEAM)});
        break;
      case 6:
        client.SendCommand(Species::OrderUnitCommand{Species::SANDBOX_STEWARD_TEAM, false, false, {aiSite.x, aiSite.z}});
        break;
      case 20:
        client.SendCommand(Species::SelectWorkersCommand{Species::SANDBOX_STEWARD_TEAM, {stewardSite.x, stewardSite.z}, 150.0});
        client.SendCommand(Species::OrderWorkersCommand{Species::SANDBOX_STEWARD_TEAM, true, {aiSite.x, aiSite.z}});
        break;
      default:
        break;
      }
      client.Deliver();
      server.Step(tick, pool);
      hashes.push_back(Species::HashWorld(*server.Simulation()).world);
      client.Publish(tick);
      client.Read();
      Assert::IsTrue(client.replica.LatestTick() == tick, L"the replica's latest tick is the server's");
      Assert::IsTrue(client.replica.Latest() == ServerView(server), L"the replica holds the server's state");
    }
    ExpectEqual(0, static_cast<std::int64_t>(client.replica.Totals().missingBaselines), L"every baseline was there");
    ExpectEqual(4, static_cast<std::int64_t>(client.reports), L"an outcome for each command");
    Assert::IsTrue(client.events > 0, L"the battle's events arrived");
    ExpectEqual(5, static_cast<std::int64_t>(server.Journal().value_or(Species::CommandJournal{}).entries.size()),
                L"the connection and the four commands");
    Assert::IsTrue(ReplayedHashes(server, first + 300, pool) == hashes, L"the same hash on every tick, headless");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr086ALossyLinkConvergesAndChangesNoHash)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr086ALossyLinkConvergesAndChangesNoHash)
  {
    // One state datagram in three lost, and a client that reads every third tick: every delta still finds its baseline,
    // the hashes are the headless run's, and once the link is clean the replica holds the server's state again.
    Species::WorldServer server({.tuning = CommittedTuning(), .worldSeed = 1, .sandbox = true});
    const std::uint64_t first = server.Open();
    AttachedClient client(server);
    client.drop = [](std::uint64_t _n) { return _n % 3 == 1; };
    Neuron::WorkerPool pool(2);
    std::vector<std::uint64_t> hashes;
    for (std::uint64_t tick = first; tick < first + 220; ++tick)
    {
      if (tick == first + 200)
      {
        client.drop = nullptr;
      }
      client.Deliver();
      server.Step(tick, pool);
      hashes.push_back(Species::HashWorld(*server.Simulation()).world);
      client.Publish(tick);
      if (tick >= first + 200 || (tick - first) % 3 == 0)
      {
        client.Read();
      }
    }
    ExpectEqual(0, static_cast<std::int64_t>(client.replica.Totals().missingBaselines), L"every baseline was there");
    Assert::IsTrue(client.replica.LatestTick() == first + 219, L"the latest tick");
    Assert::IsTrue(client.replica.Latest() == ServerView(server), L"converged on the server's state");
    Assert::IsTrue(ReplayedHashes(server, first + 220, pool) == hashes, L"the same hash on every tick, headless");
  }

#if ENGINE_DEVELOPMENT
  BEGIN_TEST_METHOD_ATTRIBUTE(Adr109ARestartedSandboxReplicatesTheNewWorld)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-005")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr109ARestartedSandboxReplicatesTheNewWorld)
  {
    // The new world's ids start again: the client is sent every object whole and every old one's removal, and holds the
    // new world's state from the next tick.
    Species::WorldServer server({.tuning = CommittedTuning(), .worldSeed = 1, .sandbox = true});
    const std::uint64_t first = server.Open();
    AttachedClient client(server);
    Neuron::WorkerPool pool(2);
    for (std::uint64_t tick = first; tick < first + 80; ++tick)
    {
      if (tick == first + 60)
      {
        client.SendCommand(Species::SandboxRestartCommand{true});
      }
      client.Deliver();
      server.Step(tick, pool);
      client.Publish(tick);
      client.Read();
      Assert::IsTrue(client.replica.Latest() == ServerView(server), L"the replica holds the server's state, before and after");
    }
    Assert::IsTrue(server.Journal().value_or(Species::CommandJournal{}).start.firstTick == first + 60, L"the world was replaced");
    ExpectEqual(0, static_cast<std::int64_t>(client.replica.Totals().missingBaselines), L"every baseline was there");
  }
#endif
};

TEST_CLASS(StateSenderTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr086DeltasAreAgainstTheAcknowledgedState)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr086DeltasAreAgainstTheAcknowledgedState)
  {
    Species::StateSender sender;
    std::uint64_t datagram = 0;
    // Sends every datagram of a tick and returns them, with the fate each gets.
    const auto tick = [&](std::uint64_t _tick, const std::vector<Species::ReplicatedObject>& _objects, Neuron::DatagramFate _fate)
    {
      std::vector<Species::StateUpdate> updates;
      std::vector<Neuron::DatagramOutcome> outcomes;
      for (Species::StateSender::Outgoing& outgoing : sender.Build(_tick, _objects))
      {
        updates.push_back(outgoing.update);
        outcomes.push_back({++datagram, _fate});
        sender.Sent(datagram, std::move(outgoing));
      }
      sender.Fates(outcomes, _tick + 1);
      return updates;
    };
    Species::ReplicatedFigure figure;
    figure.position = {100, 0, 100};
    std::vector<Species::ReplicatedObject> objects = {{Id(1), figure}, {Id(2), figure}, {Id(3), Species::ReplicatedOrb{}}};

    std::vector<Species::StateUpdate> updates = tick(10, objects, Neuron::DatagramFate::Acknowledged);
    Assert::IsTrue(updates.size() == 1 && updates[0].baselineAge == 0 && updates[0].records.size() == 3, L"the join: every object whole");
    updates = tick(11, objects, Neuron::DatagramFate::Acknowledged);
    Assert::IsTrue(updates.size() == 1 && updates[0].tick == 11 && updates[0].records.empty(), L"nothing changed: the tick alone");

    std::get<Species::ReplicatedFigure>(objects[1].state).position.x = 120;
    updates = tick(12, objects, Neuron::DatagramFate::Lost);
    Assert::IsTrue(updates.size() == 1 && updates[0].baselineAge == 2 && updates[0].records.size() == 1 &&
                     updates[0].records[0].kind == Species::RecordKind::Delta,
                   L"a delta against tick 10, the state the client acknowledged");
    updates = tick(13, objects, Neuron::DatagramFate::Acknowledged);
    Assert::IsTrue(updates.size() == 1 && updates[0].baselineAge == 3 && updates[0].records.size() == 1,
                   L"the lost one goes again, against tick 10");
    updates = tick(14, objects, Neuron::DatagramFate::Acknowledged);
    Assert::IsTrue(updates[0].records.empty(), L"then nothing");

    // Removed until the removal is acknowledged.
    objects.erase(objects.begin());
    updates = tick(15, objects, Neuron::DatagramFate::Lost);
    Assert::IsTrue(updates[0].records.size() == 1 && updates[0].records[0].kind == Species::RecordKind::Removed, L"a removal");
    updates = tick(16, objects, Neuron::DatagramFate::Acknowledged);
    Assert::IsTrue(updates[0].records.size() == 1 && updates[0].records[0].kind == Species::RecordKind::Removed, L"again");
    updates = tick(17, objects, Neuron::DatagramFate::Acknowledged);
    Assert::IsTrue(updates[0].records.empty(), L"then nothing");
    ExpectEqual(2, static_cast<std::int64_t>(sender.KnownObjects()), L"two objects left");

    // A change whose acknowledged state is older than the window goes whole.
    std::get<Species::ReplicatedFigure>(objects[0].state).health = 9;
    updates = tick(17 + Species::STATE_BASELINE_TICKS_MAX, objects, Neuron::DatagramFate::Acknowledged);
    Assert::IsTrue(updates[0].records.size() == 1 && updates[0].records[0].kind == Species::RecordKind::Whole, L"whole after 32 ticks");

    // A replaced world: every object whole again.
    sender.Forget();
    updates = tick(60, objects, Neuron::DatagramFate::Acknowledged);
    Assert::IsTrue(updates[0].baselineAge == 0 && updates[0].records.size() == 2, L"every object whole after Forget");
    ExpectEqual(0, static_cast<std::int64_t>(sender.DatagramsInFlight()), L"every datagram's fate is known");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr106ATeamsOwnFieldsGoToItsCommanderOnly)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr106ATeamsOwnFieldsGoToItsCommanderOnly)
  {
    Species::WorldSimulation world({REPLICATION_TEST_KEY, 0, 1, &CommittedTuning()});
    Species::RegionSimulation& region = world.Region(0);
    static_cast<void>(region.EnsureTeam(1));
    const Species::StableId steward = world.Ids().Issue();
    region.SetCommander(0, steward, false);
    Species::ReplicatedFigure ours;
    ours.underControl = true;
    Species::ReplicatedFigure theirs = ours;
    theirs.team = 1;
    theirs.selected = true;
    const std::vector<Species::ReplicatedObject> objects = {
      {Id(1), ours},
      {Id(2), theirs},
      {Id(3), Species::ReplicatedEffect{0, Species::EffectKind::OrderMarker, {}, 0, 0, 0}},
      {Id(4), Species::ReplicatedEffect{1, Species::EffectKind::OrderTrailMark, {}, 0, 0, 0}},
      {Id(5), Species::ReplicatedEffect{1, Species::EffectKind::Grenade, {}, 0, 0, 0}}};
    const std::vector<Species::ReplicatedObject> view = Species::ViewForSteward(objects, region, steward);
    ExpectEqual(4, static_cast<std::int64_t>(view.size()), L"the other team's trail mark is not sent");
    Assert::IsTrue(std::get<Species::ReplicatedFigure>(view[0].state).underControl, L"its own selection");
    Assert::IsFalse(std::get<Species::ReplicatedFigure>(view[1].state).selected, L"not the other team's");
    Assert::IsTrue(view[2].id == Id(3) && view[3].id == Id(5), L"its own marker, and every grenade");
  }
};

} // namespace GameLogicTests
