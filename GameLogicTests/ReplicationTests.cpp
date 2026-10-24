#include "pch.h"

#include "CommandJournal.h"
#include "CommandMessages.h"
#include "Construction.h"
#include "Development.h"
#include "InProcessTransport.h"
#include "MessageCodec.h"
#include "PresentationEvents.h"
#include "RegionSimulation.h"
#include "Sandbox.h"
#include "ScriptedSteward.h"
#include "SeenLayer.h"
#include "ServerHost.h"
#include "ServerStats.h"
#include "SettlementMessages.h"
#include "StateHash.h"
#include "StateReplica.h"
#include "StateSender.h"
#include "RelationMessages.h"
#include "Snapshot.h"
#include "StateUpdate.h"
#include "TestSupport.h"
#include "ValleyAtWar.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldSimulation.h"
#include "WorldSnapshot.h"
#include "WorldTerrain.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <string>
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
constexpr std::uint8_t SETTLED_TEAM = 2; // the settlement of the tests' valley, whose team is its site

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
    m_dispatcher.Register<Species::ServerStats>([](const Species::ServerStats&) {});
    m_dispatcher.Register<Species::SettlementNotice>([this](const Species::SettlementNotice& _notice) { notices.push_back(_notice); });
    m_dispatcher.Register<Species::SettlementStatus>([this](const Species::SettlementStatus&) { ++statuses; });
    m_dispatcher.Register<Species::ValleyStatus>([this](const Species::ValleyStatus& _status) { valleys.push_back(_status); });
    m_dispatcher.Register<Species::BattleResultNotice>([this](const Species::BattleResultNotice& _result) { results.push_back(_result); });
    m_server.Connect(CLIENT_CONNECTION);
  }

  // What decides whether the nth state datagram of the run is dropped on the way; none drops nothing.
  std::function<bool(std::uint64_t)> drop;
  // What sees every message the server sends the client, on every channel; none sees nothing.
  std::function<void(std::span<const std::byte>)> inspect;

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
  std::uint64_t statuses = 0;
  std::vector<Species::SettlementNotice> notices;
  std::vector<Species::ValleyStatus> valleys;
  std::vector<Species::BattleResultNotice> results;

private:
  [[nodiscard]] std::optional<std::uint64_t> Send(Neuron::ConnectionId, Neuron::Channel _channel,
                                                  std::span<const std::byte> _message) override
  {
    if (inspect)
    {
      inspect(_message);
    }
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

  // A sender of a test closes nothing: its server is given no identity, so it refuses no join (ADR-190).
  void Close(Neuron::ConnectionId /*_connection*/) override {}

  Species::WorldServer& m_server;
  std::unique_ptr<Neuron::Transport> m_client;
  std::unique_ptr<Neuron::Transport> m_serverEnd;
  Neuron::MessageDispatcher m_dispatcher;
  std::vector<Neuron::DatagramOutcome> m_dropped;
  std::uint64_t m_stateDatagrams = 0;
};

// What the server's state is for the client of the steward who commands _team, by stable id.
std::map<Species::StableId, Species::ReplicatedState> ServerView(const Species::WorldServer& _server,
                                                                 std::uint8_t _team = Species::SANDBOX_STEWARD_TEAM)
{
  const Species::RegionSimulation& region = _server.Simulation()->Region(0);
  const Species::StableId steward = region.CommanderOf(_team).sender;
  std::map<Species::StableId, Species::ReplicatedState> view;
  for (const Species::ReplicatedObject& object : Species::ViewForSteward(Species::CollectReplicatedObjects(region), region, steward))
  {
    view.emplace(object.id, object.state);
  }
  return view;
}

// The bits of _bytes in order, each byte's from its lowest bit or from its highest, as characters '0' and '1'.
std::string BitsOf(std::span<const std::byte> _bytes, bool _lowestFirst)
{
  std::string bits;
  bits.reserve(_bytes.size() * 8);
  for (const std::byte value : _bytes)
  {
    for (int bit = 0; bit < 8; ++bit)
    {
      const int shift = _lowestFirst ? bit : 7 - bit;
      bits.push_back((std::to_integer<unsigned>(value) >> shift & 1U) != 0 ? '1' : '0');
    }
  }
  return bits;
}

// Whether the 64 bits of _value appear in _bytes at any bit offset, in either order of the bits within a byte and of
// the value's bits.
bool HoldsValue(std::span<const std::byte> _bytes, std::uint64_t _value)
{
  std::string lowestFirst;
  for (int bit = 0; bit < 64; ++bit)
  {
    lowestFirst.push_back((_value >> bit & 1U) != 0 ? '1' : '0');
  }
  const std::string highestFirst(lowestFirst.rbegin(), lowestFirst.rend());
  for (const bool order : {true, false})
  {
    const std::string bits = BitsOf(_bytes, order);
    if (bits.find(lowestFirst) != std::string::npos || bits.find(highestFirst) != std::string::npos)
    {
      return true;
    }
  }
  return false;
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

// The settlement of _team in the replica's latest view.
std::optional<Species::ReplicatedSettlement> SettlementOf(const Species::StateReplica& _replica, std::uint8_t _team)
{
  for (const auto& [id, state] : _replica.Latest())
  {
    if (const auto* settlement = std::get_if<Species::ReplicatedSettlement>(&state); settlement != nullptr && settlement->team == _team)
    {
      return *settlement;
    }
  }
  return std::nullopt;
}

// The per-tick hashes of _server's journal replayed headless, with presentation stubs: no client at all.
std::vector<std::uint64_t> ReplayedHashes(const Species::WorldServer& _server, std::uint64_t _endTick, Neuron::WorkerPool& _pool,
                                          const Species::TuningMvp& _tuning = CommittedTuning(),
                                          const Species::StructureRecords* _structures = nullptr)
{
  const Species::CommandJournal journal = _server.Journal().value_or(Species::CommandJournal{});
  const auto terrain = std::make_shared<const Species::RegionTerrain>(*Species::GenerateRegionTerrain(journal.start.seed, _tuning));
  std::vector<std::uint64_t> hashes;
  static_cast<void>(Species::ReplayJournal(journal, terrain, _tuning, _structures, _endTick, _pool,
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

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr139ASettlementAtWorkIsReplicatedAndChangesNoTicksHash)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr139ASettlementAtWorkIsReplicatedAndChangesNoTicksHash)
  {
    // ADR-084's test with a settlement at work (M2 task T11, ADR-139): a world with the steward's settlement and an
    // attached client, which places a dwelling once its replica shows the stores to pay for it, gives the headless
    // replay's hash on every tick, and its replica holds the server's state after every tick: the structures, the
    // settlement's stores, population and territory, its seen layer and its Workers' loads. The dwelling's completion
    // reaches the client as a notice on the reliable channel.
    Species::TuningMvp tuning = CommittedTuning();
    tuning.mvp.cost.dwellingRu = 3.0; // a dwelling that the first loads pay for, so that the test sees it built
    Species::WorldServer::Desc desc;
    desc.tuning = tuning;
    desc.structures = TestRecords();
    desc.worldSeed = ValleyTerrain()->seed;
    desc.stewardSite = SETTLED_TEAM;
    Species::WorldServer server(desc);
    const std::uint64_t first = server.Open();
    AttachedClient client(server);
    Neuron::WorkerPool pool(2);
    std::vector<std::uint64_t> hashes;
    bool placed = false;
    bool loadSeen = false;
    std::uint64_t tick = first;
    for (; tick < first + 3'000 && client.notices.empty(); ++tick)
    {
      const std::optional<Species::ReplicatedSettlement> own = SettlementOf(client.replica, SETTLED_TEAM);
      if (!placed && own && own->storesMilliRu >= 3'000)
      {
        const Species::RegionSimulation& region = server.Simulation()->Region(0);
        const Species::Settlement& settlement = region.Settlements().front();
        const std::vector<Neuron::Vec3> layout = Species::DwellingLayout(region, settlement);
        const auto place = std::ranges::find_if(
          layout, [&](const Neuron::Vec3& _place)
          { return !Species::CheckPlacement(region, settlement.site, Species::StructureKind::Dwelling, _place.x, _place.z).has_value(); });
        Assert::IsTrue(place != layout.end(), L"a place for the dwelling");
        client.SendCommand(Species::PlaceStructureCommand{SETTLED_TEAM, Species::StructureKind::Dwelling, {place->x, place->z}});
        placed = true;
      }
      client.Deliver();
      server.Step(tick, pool);
      hashes.push_back(Species::HashWorld(*server.Simulation()).world);
      client.Publish(tick);
      client.Read();
      Assert::IsTrue(client.replica.Latest() == ServerView(server, SETTLED_TEAM), L"the replica holds the server's state");
      loadSeen = loadSeen || std::ranges::any_of(client.replica.Latest(),
                                                 [](const auto& _object)
                                                 {
                                                   const auto* figure = std::get_if<Species::ReplicatedFigure>(&_object.second);
                                                   return figure != nullptr && figure->loaded;
                                                 });
    }
    Assert::IsTrue(placed, L"the replica showed the stores");
    ExpectEqual(1, static_cast<std::int64_t>(client.notices.size()), L"the dwelling's completion, noticed");
    const Species::SettlementNotice& notice = client.notices.front();
    Assert::IsTrue(notice.kind == Species::NoticeKind::StructureCompleted && notice.team == SETTLED_TEAM &&
                     notice.structure == Species::StructureKind::Dwelling && notice.tick == tick - 1,
                   L"of the dwelling, at the tick it was built");
    const auto* dwelling = std::get_if<Species::ReplicatedStructure>(&client.replica.Latest().at(notice.subject));
    Assert::IsTrue(dwelling != nullptr && dwelling->built && dwelling->kind == Species::StructureKind::Dwelling &&
                     dwelling->owner == std::optional<std::uint8_t>(SETTLED_TEAM),
                   L"its subject is the dwelling, built, in the replica");
    Assert::IsTrue(loadSeen, L"a Worker carried a load");
    const std::optional<Species::ReplicatedSettlement> own = SettlementOf(client.replica, SETTLED_TEAM);
    const Species::RegionSimulation& region = server.Simulation()->Region(0);
    Assert::IsTrue(own && std::uint32_t{own->population} == Species::Population(region, region.Settlements().front()) &&
                     own->limit == CommittedTuning().mvp.settlement.populationBase + CommittedTuning().mvp.dwelling.population,
                   L"the settlement's population, and the limit its dwelling raised");
    std::array<std::size_t, 2> chunks{};
    for (const auto& [id, state] : client.replica.Latest())
    {
      if (const auto* cells = std::get_if<Species::ReplicatedCells>(&state))
      {
        ++chunks[static_cast<std::size_t>(cells->layer)];
      }
    }
    Assert::IsTrue(chunks[0] > 0 && chunks[1] > 0, L"its territory and its seen layer");
    ExpectEqual(0, static_cast<std::int64_t>(client.replica.Totals().missingBaselines), L"every baseline was there");
    Assert::IsTrue(ReplayedHashes(server, tick, pool, tuning, &TestRecords()) == hashes, L"the same hash on every tick, headless");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr140AWatchedWorldsSettlementIsTheScriptedStewards)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-056")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr140AWatchedWorldsSettlementIsTheScriptedStewards)
  {
    // ADR-140: a Debug build's --watch in a settlement world hands the steward's settlement to the scripted steward of
    // ADR-138, and the attached steward commands nothing; the journal holds it, so the replay gives the same hashes.
    Species::WorldServer::Desc desc;
    desc.tuning = CommittedTuning();
    desc.structures = TestRecords();
    desc.worldSeed = ValleyTerrain()->seed;
    desc.stewardSite = SETTLED_TEAM;
    desc.watched = true;
    Species::WorldServer server(desc);
    const std::uint64_t first = server.Open();
    AttachedClient client(server);
    Neuron::WorkerPool pool(2);
    std::vector<std::uint64_t> hashes;
    std::uint64_t tick = first;
    for (; tick < first + 300; ++tick)
    {
      client.Deliver();
      server.Step(tick, pool);
      hashes.push_back(Species::HashWorld(*server.Simulation()).world);
      client.Publish(tick);
      client.Read();
    }
    const Species::RegionSimulation& region = server.Simulation()->Region(0);
    Assert::IsTrue(region.ScriptedStewards().size() == 1 && region.ScriptedStewards().front().team == SETTLED_TEAM,
                   L"the steward's settlement has its scripted steward");
    const Species::RegionSimulation::Commander commander = region.CommanderOf(SETTLED_TEAM);
    Assert::IsTrue(commander.sender == region.ScriptedStewards().front().id && commander.ai, L"which commands it, as an AI");
    Assert::IsTrue(std::ranges::any_of(client.replica.Latest(),
                                       [](const auto& _object)
                                       {
                                         const auto* structure = std::get_if<Species::ReplicatedStructure>(&_object.second);
                                         return structure != nullptr && structure->kind == Species::StructureKind::Wellspring;
                                       }),
                   L"the client still sees the settlement");
    Assert::IsTrue(ReplayedHashes(server, tick, pool, CommittedTuning(), &TestRecords()) == hashes,
                   L"the same hash on every tick, headless");
  }
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

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr139ASettlementsStoresAndSeenLayerGoToItsCommanderOnly)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-090")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-097")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr139ASettlementsStoresAndSeenLayerGoToItsCommanderOnly)
  {
    // A settlement's territory, its structures and the resource sites go to every client, its seen layer and its stores
    // and population to its commander's alone; the chunks of a raster hold every cell of it once.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    const Species::StableId steward = world->Ids().Issue();
    region.SetCommander(SETTLED_TEAM, steward, false);
    Neuron::WorkerPool pool(0);
    world->Step(0, pool); // the seen layers are made in the region's step
    const std::vector<Species::ReplicatedObject> objects = Species::CollectReplicatedObjects(region);
    Assert::IsTrue(std::ranges::is_sorted(objects, {}, &Species::ReplicatedObject::id), L"in stable-id order");
    const Species::Settlement& settlement = region.Settlements().front();
    std::array<std::size_t, 2> cells{};
    std::size_t structures = 0;
    for (const Species::ReplicatedObject& object : objects)
    {
      if (const auto* chunk = std::get_if<Species::ReplicatedCells>(&object.state))
      {
        Assert::IsTrue(object.id == Species::CellChunkObjectId(chunk->layer, chunk->site, chunk->chunkX, chunk->chunkZ), L"its id");
        cells[static_cast<std::size_t>(chunk->layer)] += static_cast<std::size_t>(std::popcount(chunk->cells));
      }
      structures += std::holds_alternative<Species::ReplicatedStructure>(object.state) ? 1 : 0;
    }
    ExpectEqual(static_cast<std::int64_t>(region.TerritoryOf(settlement.site)->Count()), static_cast<std::int64_t>(cells[0]),
                L"every cell of the territory once");
    ExpectEqual(static_cast<std::int64_t>(region.SeenLayerOf(settlement.site)->Count()), static_cast<std::int64_t>(cells[1]),
                L"every cell of the seen layer once");
    Assert::IsTrue(structures > 11, L"its Wellspring and the resource sites");

    const auto viewOf = [&](Species::StableId _steward)
    {
      std::size_t seen = 0;
      std::optional<Species::ReplicatedSettlement> state;
      for (const Species::ReplicatedObject& object : Species::ViewForSteward(objects, region, _steward))
      {
        const auto* chunk = std::get_if<Species::ReplicatedCells>(&object.state);
        seen += chunk != nullptr && chunk->layer == Species::CellLayer::Seen ? 1 : 0;
        if (const auto* replicated = std::get_if<Species::ReplicatedSettlement>(&object.state))
        {
          state = *replicated;
        }
      }
      return std::pair{seen, state};
    };
    const auto [ownSeen, own] = viewOf(steward);
    Assert::IsTrue(ownSeen > 0 && own && std::uint32_t{own->population} == Species::Population(region, settlement) && own->limit > 0 &&
                     own->capacityMilliRu > 0 && own->radius > 0,
                   L"its commander sees its seen layer, population, limit and stores");
    const Species::ReplicatedSettlement ownState = own.value_or(Species::ReplicatedSettlement{});
    const auto [otherSeen, other] = viewOf(world->Ids().Issue());
    Assert::IsTrue(otherSeen == 0 && other && other->population == 0 && other->limit == 0 && other->capacityMilliRu == 0 &&
                     other->radius == ownState.radius && other->color == ownState.color,
                   L"another steward sees its territory and colour alone");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr084AValleyClientHoldsNothingOutsideItsSeenLayer)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-016")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-097")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-084")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr084AValleyClientHoldsNothingOutsideItsSeenLayer)
  {
    // M3 plan table 1, ADR-084 and ADR-086 (ADR-158): in a valley of four settlements, the client of the steward's
    // holds the server's view after every tick, and that view holds no other team's figure, effect or orb and no other
    // settlement's structure outside its settlement's seen layer, nor another settlement's stores; the Wellsprings and
    // resource sites are the public layer, without the Workers at their ports where it does not see.
    Species::WorldServer::Desc desc;
    desc.tuning = CommittedTuning();
    desc.structures = TestRecords();
    desc.worldSeed = ValleyTerrain()->seed;
    desc.stewardSite = SETTLED_TEAM;
    desc.aiSettlements = 3;
    Species::WorldServer server(desc);
    const std::uint64_t first = server.Open();
    AttachedClient client(server);
    Neuron::WorkerPool pool(2);
    std::size_t hiddenMost = 0;
    for (std::uint64_t tick = first; tick < first + 600; ++tick)
    {
      client.Deliver();
      server.Step(tick, pool);
      client.Publish(tick);
      client.Read();
      Assert::IsTrue(client.replica.Latest() == ServerView(server, SETTLED_TEAM), L"the replica holds the server's view");
      const Species::RegionSimulation& region = server.Simulation()->Region(0);
      const Species::Sight sight = Species::Sight::OfSteward(region, region.CommanderOf(SETTLED_TEAM).sender);
      Assert::IsTrue(sight.Narrowed(), L"the steward's client sees through its settlement's seen layer");
      const auto sees = [&sight](const Species::QuantizedPosition& _position)
      {
        const Neuron::Vec3 position = Species::PositionOf(_position);
        return sight.Sees(position.x, position.z);
      };
      std::size_t figures = 0;
      std::size_t sites = 0;
      for (const auto& [id, state] : client.replica.Latest())
      {
        if (const auto* figure = std::get_if<Species::ReplicatedFigure>(&state))
        {
          ++figures;
          Assert::IsTrue(figure->team == SETTLED_TEAM || sees(figure->position), L"no other team's figure outside it");
        }
        if (const auto* effect = std::get_if<Species::ReplicatedEffect>(&state))
        {
          Assert::IsTrue(effect->team == SETTLED_TEAM || sees(effect->position), L"no other team's effect outside it");
        }
        if (const auto* orb = std::get_if<Species::ReplicatedOrb>(&state))
        {
          Assert::IsTrue(orb->team == SETTLED_TEAM || sees(orb->position), L"no other team's orb outside it");
        }
        if (const auto* structure = std::get_if<Species::ReplicatedStructure>(&state);
            structure != nullptr && structure->owner != std::optional<std::uint8_t>(SETTLED_TEAM) && !sees(structure->position))
        {
          Assert::IsTrue(structure->kind == Species::StructureKind::Wellspring || structure->kind == Species::StructureKind::ResourceSite ||
                           structure->kind == Species::StructureKind::ResourceSiteCentral,
                         L"no other settlement's structure outside it but the public sites");
          ExpectEqual(0, std::int64_t{structure->mannedPorts}, L"a public site shows no Worker where it is not seen");
        }
        if (const auto* settlement = std::get_if<Species::ReplicatedSettlement>(&state);
            settlement != nullptr && settlement->team != SETTLED_TEAM)
        {
          Assert::IsTrue(settlement->storesMilliRu == 0 && settlement->population == 0, L"no other settlement's stores");
        }
        sites += std::holds_alternative<Species::ReplicatedStructure>(state) ? 1 : 0;
      }
      std::size_t living = 0;
      for (std::size_t team = 0; team < region.TeamCount(); ++team)
      {
        living += region.TeamAt(team).figures.Count();
      }
      hiddenMost = std::max(hiddenMost, living - std::min(living, figures));
      Assert::IsTrue(sites >= region.Settlements().size() + region.Terrain()->layout.sites.size(), L"every Wellspring and site");
    }
    Assert::IsTrue(hiddenMost >= 40, L"the other settlements' Workers at home are not sent");
    ExpectEqual(0, static_cast<std::int64_t>(client.replica.Totals().missingBaselines), L"every baseline was there");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr084AValleyClientHoldsNothingOutsideItsSeenLayerInABattle)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-048")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-097")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-098")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-102")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr084AValleyClientHoldsNothingOutsideItsSeenLayerInABattle)
  {
    // M3 task T10 (ADR-165): in the valley at war, four settlements and two raids, one of them on the steward's
    // settlement, the steward's client holds the server's view after every tick and nothing outside its seen layer but
    // its own expedition's; its valley status lists both battles, but the figures of its own alone, and no grievance or
    // expedition that is not its settlement's; and as the battles end it is told its own battle's end with its whole
    // result, and only the other battle's public line.
    static const Species::TuningMvp TUNING = []
    {
      Species::TuningMvp tuning = CommittedTuning();
      tuning.mvp.conflict.warningMinutes = 0.1; // 60 ticks, so that both battles start as the valley settles
      tuning.pil.battle.maxMinutes = 0.5;       // 300 ticks
      return tuning;
    }();
    Species::ValleyAtWarDesc war;
    war.key = SETTLED_TEST_KEY;
    war.terrain = ValleyTerrain();
    war.tuning = &TUNING;
    war.structures = &TestRecords();
    war.population = 60;
    war.expedition = 10;
    war.settleTicks = 2'000;
    const std::unique_ptr<Species::WorldSimulation> world = Species::StartValleyAtWar(war);
    const TemporaryFolder folder;
    Assert::IsTrue(Neuron::SnapshotStore(folder.Path(), 1).Save(Species::SnapshotOf(*world)).has_value(), L"the valley at war saved");
    Species::WorldServer::Desc desc;
    desc.tuning = TUNING;
    desc.structures = TestRecords();
    desc.worldSeed = ValleyTerrain()->seed;
    desc.snapshotFolder = folder.Path();
    desc.restoreOnly = true;
    Species::WorldServer server(desc);
    std::uint64_t tick = server.Open();
    Assert::IsTrue(server.Opened().restored && tick == world->NextTick() && world->Region(0).Battles().size() == 2,
                   L"with both battles in progress, continued by the server");
    AttachedClient client(server);
    const auto steward = static_cast<std::uint8_t>(TUNING.mvp.sandbox.stewardSite);
    const std::uint8_t expedition = Species::ExpeditionTeamOf(steward);
    Neuron::WorkerPool pool(2);
    for (const std::uint64_t last = tick + 400; tick < last; ++tick)
    {
      if (tick % 50 == 0)
      {
        client.SendCommand(Species::ReadValleyStatusCommand{steward});
      }
      client.Deliver();
      server.Step(tick, pool);
      client.Publish(tick);
      client.Read();
      Assert::IsTrue(client.replica.Latest() == ServerView(server, steward), L"the replica holds the server's view");
      const Species::RegionSimulation& region = server.Simulation()->Region(0);
      const Species::Sight sight = Species::Sight::OfSteward(region, region.CommanderOf(steward).sender);
      const auto sees = [&sight](const Species::QuantizedPosition& _position)
      {
        const Neuron::Vec3 position = Species::PositionOf(_position);
        return sight.Sees(position.x, position.z);
      };
      for (const auto& [id, state] : client.replica.Latest())
      {
        if (const auto* figure = std::get_if<Species::ReplicatedFigure>(&state))
        {
          Assert::IsTrue(figure->team == steward || figure->team == expedition || sees(figure->position),
                         L"no other team's figure outside it, the attackers' among them");
        }
        if (const auto* settlement = std::get_if<Species::ReplicatedSettlement>(&state);
            settlement != nullptr && settlement->team != steward)
        {
          Assert::IsTrue(settlement->storesMilliRu == 0 && settlement->population == 0, L"no other settlement's stores");
        }
      }
    }
    Assert::IsTrue(client.valleys.size() >= 7, L"a valley status for each request");
    bool bothFought = false;
    for (const Species::ValleyStatus& status : client.valleys)
    {
      Assert::IsTrue(status.team == steward && status.renown.size() == 4, L"the steward's, with every settlement's renown");
      Assert::IsTrue(std::ranges::all_of(status.grievances, [&](const Species::GrievanceLine& _line)
                                         { return _line.holder == steward || _line.against == steward; }),
                     L"only the grievances held by or against its settlement");
      Assert::IsTrue(std::ranges::all_of(status.expeditions, [&](const Species::ExpeditionLine& _line)
                                         { return _line.attacker == steward || _line.target == steward; }),
                     L"only the expeditions it sent or that are declared on it");
      for (const Species::BattleLine& line : status.battles)
      {
        const bool side = line.attacker == steward || line.defender == steward;
        Assert::IsTrue(line.side == side, L"a battle is its side's when its settlement fights in it");
        Assert::IsTrue(side || (line.attackerLost == 0 && line.defenderLost == 0 && line.lossCap == 0 && line.fieldRadius == 0),
                       L"another battle shows its public line alone");
      }
      bothFought = bothFought || status.battles.size() == 2;
    }
    Assert::IsTrue(bothFought, L"both battles listed while they are fought");
    ExpectEqual(2, static_cast<std::int64_t>(client.results.size()), L"both battles ended, each with its result");
    for (const Species::BattleResultNotice& result : client.results)
    {
      const bool side = result.attacker == steward || result.defender == steward;
      Assert::IsTrue(result.side == side, L"the whole result of its own battle");
      Assert::IsTrue(side || (result.attackerLost == 0 && result.defenderLost == 0 && result.storesMovedMilliRu == 0 &&
                              result.essenceMoved == 0 && result.renown == 0),
                     L"and the other battle's public line alone");
    }
    const auto ended = std::ranges::count(client.notices, Species::NoticeKind::BattleEnded, &Species::SettlementNotice::kind);
    ExpectEqual(1, static_cast<std::int64_t>(ended), L"the end of its own battle is noticed, not the other's");
    const auto own = std::ranges::find(client.notices, Species::NoticeKind::BattleEnded, &Species::SettlementNotice::kind);
    Assert::IsTrue(own != client.notices.end() && own->team == steward && !own->ours && own->other.has_value(),
                   L"as the defender, against its attacker");
    ExpectEqual(0, static_cast<std::int64_t>(client.replica.Totals().missingBaselines), L"every baseline was there");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr158AnotherTeamsEventsGoOnlyWhereTheStewardSees)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-097")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-084")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr158AnotherTeamsEventsGoOnlyWhereTheStewardSees)
  {
    // ADR-158: a steward who commands a settlement is sent its teams' events wherever they are, and another team's only
    // where its settlement's seen layer holds them and when they are no selection or order (ADR-106); a steward who
    // commands no settlement is sent every event but the selections and orders of the teams it does not command.
    const std::unique_ptr<Species::WorldSimulation> world = SettledWorld();
    Species::RegionSimulation& region = world->Region(0);
    const Species::StableId steward = world->Ids().Issue();
    region.SetCommander(SETTLED_TEAM, steward, false);
    Neuron::WorkerPool pool(0);
    world->Step(0, pool);                  // the seen layers are made in the region's step
    constexpr std::uint8_t OTHER_TEAM = 9; // no settlement's nor its expedition's (ADR-161)
    const Neuron::Vec3 home = region.Terrain()->layout.sites[SETTLED_TEAM];
    const Neuron::Vec3 distant = region.Terrain()->layout.sites[(SETTLED_TEAM + 2) % 4];
    const std::vector<Species::EffectEvent> events = {{Species::EffectEventKind::LaserFired, OTHER_TEAM, 0, home, {}, 0.0},
                                                      {Species::EffectEventKind::LaserFired, OTHER_TEAM, 0, distant, {}, 0.0},
                                                      {Species::EffectEventKind::WorkersSelected, OTHER_TEAM, 0, home, {}, 1.0},
                                                      {Species::EffectEventKind::LaserFired, SETTLED_TEAM, 0, distant, {}, 0.0},
                                                      {Species::EffectEventKind::WorkersSelected, SETTLED_TEAM, 0, distant, {}, 1.0}};
    const std::vector<Species::EffectEvent> sent = Species::EventsForSteward(events, region, steward);
    ExpectEqual(3, static_cast<std::int64_t>(sent.size()), L"not the other team's distant shot or its selection");
    Assert::IsTrue(sent[0].team == OTHER_TEAM && sent[0].position.x == home.x, L"the other team's shot where it sees");
    Assert::IsTrue(sent[1].team == SETTLED_TEAM && sent[2].team == SETTLED_TEAM, L"its own team's events wherever they are");
    ExpectEqual(3, static_cast<std::int64_t>(Species::EventsForSteward(events, region, world->Ids().Issue()).size()),
                L"a steward of no settlement is sent every shot and no selection");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr082NothingAClientIsSentHoldsTheSimulationKey)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-097")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"R-SIM-605")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr082NothingAClientIsSentHoldsTheSimulationKey)
  {
    // M3 plan table 1, ADR-082: a client given every byte the steward's client receives cannot compute a later draw,
    // for no message it is sent holds either half of the simulation key, at any bit, in either order. Every kind of
    // message goes: the terrain, the state, the events, the outcomes, the notices and the status.
    Species::WorldServer::Desc desc;
    desc.tuning = CommittedTuning();
    desc.tuning.mvp.cost.dwellingRu = 3.0; // a dwelling that the first loads pay for, so that a notice is sent
    desc.structures = TestRecords();
    desc.worldSeed = ValleyTerrain()->seed;
    desc.stewardSite = SETTLED_TEAM;
    desc.aiSettlements = 3;
    Species::WorldServer server(desc);
    const std::uint64_t first = server.Open();
    AttachedClient client(server);
    const Neuron::SipHashKey key = server.Simulation()->SimulationKey();
    Assert::IsTrue(key.low != 0 && key.high != 0, L"a key drawn for the world");
    // The search finds a half wherever a message holds it: a terrain message whose seed field carried it.
    for (const std::uint64_t half : {key.low, key.high})
    {
      Species::WorldTerrain probe;
      probe.seed = half;
      Assert::IsTrue(HoldsValue(Neuron::EncodeMessage(probe), half), L"the search finds a half that is sent");
    }
    std::size_t messages = 0;
    std::size_t bytes = 0;
    client.inspect = [&](std::span<const std::byte> _message)
    {
      ++messages;
      bytes += _message.size();
      Assert::IsFalse(HoldsValue(_message, key.low) || HoldsValue(_message, key.high), L"no half of the key");
    };
    Neuron::WorkerPool pool(2);
    bool placed = false;
    for (std::uint64_t tick = first; tick < first + 1'500 && client.notices.empty(); ++tick)
    {
      const std::optional<Species::ReplicatedSettlement> own = SettlementOf(client.replica, SETTLED_TEAM);
      if (!placed && own && own->storesMilliRu >= 3'000)
      {
        const Species::RegionSimulation& region = server.Simulation()->Region(0);
        const Species::Settlement& settlement = *region.SettlementAt(SETTLED_TEAM);
        const std::vector<Neuron::Vec3> layout = Species::DwellingLayout(region, settlement);
        const auto place = std::ranges::find_if(
          layout, [&](const Neuron::Vec3& _place)
          { return !Species::CheckPlacement(region, settlement.site, Species::StructureKind::Dwelling, _place.x, _place.z).has_value(); });
        Assert::IsTrue(place != layout.end(), L"a place for the dwelling");
        client.SendCommand(Species::PlaceStructureCommand{SETTLED_TEAM, Species::StructureKind::Dwelling, {place->x, place->z}});
        client.SendCommand(Species::ReadSettlementStatusCommand{SETTLED_TEAM});
        placed = true;
      }
      client.Deliver();
      server.Step(tick, pool);
      client.Publish(tick);
      client.Read();
    }
    Assert::IsTrue(placed && client.reports > 0 && client.statuses > 0 && !client.notices.empty(),
                   L"outcomes, a status and a notice were sent");
    Assert::IsTrue(messages > 500 && bytes > 50'000, L"the client was sent the world");
  }
};

} // namespace GameLogicTests
