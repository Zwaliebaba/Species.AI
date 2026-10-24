#include "pch.h"

#include "AwayMessages.h"
#include "CommandJournal.h"
#include "CommandMessages.h"
#include "JoinMessages.h"
#include "LinkScene.h"
#include "MessageCodec.h"
#include "PlatformClock.h"
#include "PresentationEvents.h"
#include "RegionSimulation.h"
#include "RegionTerrain.h"
#include "RelationMessages.h"
#include "ServerCertificate.h"
#include "ServerHost.h"
#include "ServerStats.h"
#include "SettlementMessages.h"
#include "StateHash.h"
#include "StateReplica.h"
#include "StateSender.h"
#include "StateUpdate.h"
#include "Tuning.h"
#include "WorkerPool.h"
#include "WorldServer.h"
#include "WorldTerrain.h"

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <format>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace Performance
{

namespace
{

constexpr std::wstring_view LINK_CERTIFICATE_NAME = L"Species transport test"; // ADR-185: the tests' own, reused
constexpr std::size_t SERVER_WORKERS = 2;
constexpr std::uint64_t WORLD_SEED = 1;
constexpr std::uint8_t AI_SETTLEMENTS = 3;
constexpr std::uint64_t TRUTH_TICKS = 128;           // the server's views kept to compare the replica with
constexpr std::uint64_t SAMPLE_TICKS = 10;           // each server end's MsQuic figures, read once a second
constexpr std::uint64_t OVERHEAD_SETTLE_TICKS = 100; // a connection's handshake and first whole states, left out of the window
constexpr std::uint64_t REQUEST_TICKS = 10;          // the statuses asked for once a second, as the HUD asks (ADR-152, ADR-166)
constexpr Neuron::Nanoseconds POLL{1'000'000};
constexpr Neuron::Nanoseconds JOIN_WAIT{30'000'000'000};
constexpr Neuron::Nanoseconds STOP_WAIT{5'000'000'000};
constexpr double NANOSECONDS_PER_SECOND = 1e9;
constexpr double IPV4_UDP_HEADER_BYTES = 28.0; // IPv4 20 and UDP 8 (ADR-088 item 3)

using View = std::map<Species::StableId, Species::ReplicatedState>;

double Seconds(Neuron::Nanoseconds _duration)
{
  return static_cast<double>(_duration.count()) / NANOSECONDS_PER_SECOND;
}

// What a server end's counters hold at one sample.
struct EndCounters
{
  std::uint64_t reliableBytes = 0;
  Neuron::QuicPathStats path;
};

struct ServerEnd
{
  Neuron::QuicTransport* transport = nullptr; // the host's, alive until the world hears of its disconnection
  std::uint64_t connectedTick = 0;
  EndCounters counted;
  std::optional<EndCounters> first; // at OVERHEAD_SETTLE_TICKS after its connection
  std::optional<EndCounters> latest;
};

struct SentDatagram
{
  std::uint64_t tick = 0;
  std::vector<Species::StableId> ids;
};

/// The server's listener, as the host sees it, keeping each end it accepts in order, which is the order of the host's
/// connection numbers (ServerHost.h).
class ObservedListener final : public Neuron::TransportListener
{
public:
  explicit ObservedListener(Neuron::QuicListener& _listener)
    : m_listener(_listener)
  {
  }

  [[nodiscard]] std::unique_ptr<Neuron::Transport> Accept() override
  {
    std::unique_ptr<Neuron::Transport> accepted = m_listener.Accept();
    if (accepted)
    {
      m_accepted.push_back(dynamic_cast<Neuron::QuicTransport*>(accepted.get()));
    }
    return accepted;
  }

  // The end the host numbered _connection, from 1.
  [[nodiscard]] Neuron::QuicTransport* EndOf(Neuron::ConnectionId _connection) const
  {
    return _connection >= 1 && _connection <= m_accepted.size() ? m_accepted[_connection - 1] : nullptr;
  }

private:
  Neuron::QuicListener& m_listener;
  std::vector<Neuron::QuicTransport*> m_accepted;
};

/// The world server as the host runs it, with what the scene measures beside it, all on the tick thread: each tick's
/// hash and the view of its latest steward's client, each state datagram's tick and objects, their fates, and each
/// server end's figures. The views are shared with the client's thread under their lock; the rest is read once the host
/// has stopped.
class ObservedWorld final : public Neuron::ServerWorld
{
public:
  ObservedWorld(Species::WorldServer& _world, const ObservedListener& _listener)
    : m_world(_world),
      m_listener(_listener)
  {
  }

  [[nodiscard]] std::uint64_t Open() override
  {
    return m_world.Open();
  }

  void Connect(Neuron::ConnectionId _connection) override
  {
    m_ends[_connection] = {m_listener.EndOf(_connection), m_tick, {}, {}, {}};
    m_openEnds.fetch_add(1, std::memory_order_release);
    m_world.Connect(_connection);
  }

  void Disconnect(Neuron::ConnectionId _connection) override
  {
    // The host still holds the end: its last figures are read before it goes.
    if (const auto found = m_ends.find(_connection); found != m_ends.end())
    {
      Sample(found->second);
      found->second.transport = nullptr;
      m_openEnds.fetch_sub(1, std::memory_order_release);
    }
    m_world.Disconnect(_connection);
  }

  void Receive(Neuron::ConnectionId _connection, const Neuron::ReceivedMessage& _message) override
  {
    m_world.Receive(_connection, _message);
  }

  void DatagramFates(Neuron::ConnectionId _connection, std::span<const Neuron::DatagramOutcome> _outcomes) override
  {
    for (const Neuron::DatagramOutcome& outcome : _outcomes)
    {
      const auto found = m_sent.find({_connection, outcome.datagram});
      if (found == m_sent.end())
      {
        continue;
      }
      if (outcome.fate == Neuron::DatagramFate::Lost)
      {
        ++m_lost;
        for (const Species::StableId id : found->second.ids)
        {
          m_lostTicks[id].push_back(found->second.tick);
        }
      }
      else
      {
        ++m_acknowledged;
      }
      m_sent.erase(found);
    }
    m_world.DatagramFates(_connection, _outcomes);
  }

  void Step(std::uint64_t _tick, Neuron::WorkerPool& _workers) override
  {
    m_tick = _tick;
    m_world.Step(_tick, _workers);
    const Species::WorldSimulation* simulation = m_world.Simulation();
    if (simulation == nullptr)
    {
      return;
    }
    // A Release server hashes only at its saves, so the scene hashes every tick itself (ADR-081 item 8).
    m_hashes.emplace_back(_tick, Species::HashWorld(*simulation).world);
    // The view of the latest steward's client, as Publish builds it after this step (ADR-111, ADR-158).
    if (const std::optional<Species::StableId> steward = LatestSteward())
    {
      const Species::RegionSimulation& region = simulation->Region(0);
      View view;
      for (const Species::ReplicatedObject& object : Species::ViewForSteward(Species::CollectReplicatedObjects(region), region, *steward))
      {
        view.emplace(object.id, object.state);
      }
      const std::lock_guard lock(m_viewsMutex);
      m_views.insert_or_assign(_tick, std::move(view));
      while (!m_views.empty() && m_views.begin()->first + TRUTH_TICKS < _tick)
      {
        m_views.erase(m_views.begin());
      }
    }
    if (_tick % SAMPLE_TICKS == 0)
    {
      for (auto& [connection, end] : m_ends)
      {
        Sample(end);
      }
    }
  }

  void Publish(std::uint64_t _tick, Neuron::ConnectionSender& _sender) override
  {
    TappedSender tapped(_sender, *this, _tick);
    m_world.Publish(_tick, tapped);
  }

  void Stalled(const Neuron::ScheduleStall& _stall) override
  {
    ++m_stalls;
    m_world.Stalled(_stall);
  }

  void Freeze(std::uint64_t _nextTick) override
  {
    m_world.Freeze(_nextTick);
  }

  // The server's view at _tick, copied; none when it was not kept. Any thread.
  [[nodiscard]] std::optional<View> ViewAt(std::uint64_t _tick) const
  {
    const std::lock_guard lock(m_viewsMutex);
    const auto found = m_views.find(_tick);
    return found == m_views.end() ? std::nullopt : std::optional<View>(found->second);
  }

  // The server ends the host holds. Any thread.
  [[nodiscard]] std::size_t OpenEnds() const noexcept
  {
    return m_openEnds.load(std::memory_order_acquire);
  }

  // Read once the host has stopped.
  [[nodiscard]] const std::vector<std::pair<std::uint64_t, std::uint64_t>>& Hashes() const noexcept
  {
    return m_hashes;
  }
  [[nodiscard]] const std::map<Species::StableId, std::vector<std::uint64_t>>& LostTicks() const noexcept
  {
    return m_lostTicks;
  }
  [[nodiscard]] const std::map<Neuron::ConnectionId, ServerEnd>& Ends() const noexcept
  {
    return m_ends;
  }
  [[nodiscard]] std::uint64_t Datagrams() const noexcept
  {
    return m_datagrams;
  }
  [[nodiscard]] std::uint64_t Acknowledged() const noexcept
  {
    return m_acknowledged;
  }
  [[nodiscard]] std::uint64_t Lost() const noexcept
  {
    return m_lost;
  }
  [[nodiscard]] std::uint64_t Unresolved() const noexcept
  {
    return m_sent.size();
  }
  [[nodiscard]] std::uint64_t Stalls() const noexcept
  {
    return m_stalls;
  }

private:
  /// The host's sender, through which the scene sees each message the world sends.
  class TappedSender final : public Neuron::ConnectionSender
  {
  public:
    TappedSender(Neuron::ConnectionSender& _sender, ObservedWorld& _world, std::uint64_t _tick)
      : m_sender(_sender),
        m_world(_world),
        m_tick(_tick)
    {
    }

    [[nodiscard]] std::optional<std::uint64_t> Send(Neuron::ConnectionId _connection, Neuron::Channel _channel,
                                                    std::span<const std::byte> _message) override
    {
      const std::optional<std::uint64_t> number = m_sender.Send(_connection, _channel, _message);
      if (number)
      {
        m_world.Sent(_connection, _channel, _message, *number, m_tick);
      }
      return number;
    }

    void Close(Neuron::ConnectionId _connection) override
    {
      m_sender.Close(_connection);
    }

  private:
    Neuron::ConnectionSender& m_sender;
    ObservedWorld& m_world;
    std::uint64_t m_tick = 0;
  };

  void Sent(Neuron::ConnectionId _connection, Neuron::Channel _channel, std::span<const std::byte> _message, std::uint64_t _number,
            std::uint64_t _tick)
  {
    const auto found = m_ends.find(_connection);
    ServerEnd* end = found != m_ends.end() ? &found->second : nullptr;
    if (_channel == Neuron::Channel::Reliable)
    {
      if (end != nullptr)
      {
        end->counted.reliableBytes += _message.size();
      }
      return;
    }
    Neuron::BitReader reader(_message);
    const Neuron::MessageHeader header = Neuron::ReadMessageHeader(reader);
    if (reader.Failed() || header.type != Species::StateUpdate::TYPE)
    {
      return;
    }
    Species::StateUpdate update;
    update.Decode(reader);
    SentDatagram sent{.tick = _tick, .ids = {}};
    sent.ids.reserve(update.records.size());
    for (const Species::StateRecord& record : update.records)
    {
      sent.ids.push_back(record.id);
    }
    m_sent.insert_or_assign({_connection, _number}, std::move(sent));
    ++m_datagrams;
  }

  void Sample(ServerEnd& _end) const
  {
    if (_end.transport == nullptr)
    {
      return;
    }
    if (const std::optional<Neuron::QuicPathStats> path = _end.transport->PathStats())
    {
      EndCounters counters = _end.counted;
      counters.path = *path;
      if (!_end.first && m_tick >= _end.connectedTick + OVERHEAD_SETTLE_TICKS)
      {
        _end.first = counters;
      }
      _end.latest = counters;
    }
  }

  // The steward of the latest connection the world admitted: the sender of its journal's latest connection (ADR-109).
  [[nodiscard]] std::optional<Species::StableId> LatestSteward()
  {
    const std::optional<Species::CommandJournal>& journal = m_world.Journal();
    if (!journal)
    {
      return std::nullopt;
    }
    for (std::size_t i = m_journalRead; i < journal->entries.size(); ++i)
    {
      if (journal->entries[i].connection)
      {
        m_steward = journal->entries[i].sender;
      }
    }
    m_journalRead = journal->entries.size();
    return m_steward;
  }

  Species::WorldServer& m_world;
  const ObservedListener& m_listener;
  std::uint64_t m_tick = 0;
  std::vector<std::pair<std::uint64_t, std::uint64_t>> m_hashes;
  std::size_t m_journalRead = 0;
  std::optional<Species::StableId> m_steward;
  mutable std::mutex m_viewsMutex;
  std::map<std::uint64_t, View> m_views;
  std::map<std::pair<Neuron::ConnectionId, std::uint64_t>, SentDatagram> m_sent;
  std::map<Species::StableId, std::vector<std::uint64_t>> m_lostTicks;
  std::map<Neuron::ConnectionId, ServerEnd> m_ends;
  std::atomic<std::size_t> m_openEnds{0};
  std::uint64_t m_datagrams = 0;
  std::uint64_t m_acknowledged = 0;
  std::uint64_t m_lost = 0;
  std::uint64_t m_stalls = 0;
};

// One object's freeze in progress: its first stale tick and its stale ticks so far.
struct OpenFreeze
{
  std::uint64_t firstTick = 0;
  std::uint32_t ticks = 0;
  bool figure = false;
};

struct Freeze
{
  Species::StableId id = 0;
  std::uint64_t firstTick = 0;
  std::uint32_t ticks = 0;
  bool figure = false;
};

/// The client: one connection at a time over QuicTransport, its join, its replica and the requests the HUD makes, and
/// the comparison of the replica with the server's view at each tick, made when the first datagram of a later tick
/// arrives, so that every datagram of the tick that arrived in time has been applied.
class LinkClient
{
public:
  LinkClient(const Neuron::PlatformClock& _clock, const ObservedWorld& _world, Neuron::QuicTransport::ConnectDesc _connect,
             std::uint8_t _team)
    : m_clock(_clock),
      m_world(_world),
      m_connect(_connect),
      m_team(_team)
  {
    m_dispatcher.Register<Species::JoinAnswer>([this](const Species::JoinAnswer& _answer)
                                               { m_joinRefused = _answer.refusal != Species::JoinRefusal::None; });
    m_dispatcher.Register<Species::StateUpdate>([this](const Species::StateUpdate& _update) { Apply(_update); });
    m_dispatcher.Register<Species::WorldTerrain>([this](const Species::WorldTerrain&) { ++run.terrains; });
    m_dispatcher.Register<Species::PresentationEvents>([](const Species::PresentationEvents&) {});
    m_dispatcher.Register<Species::CommandReport>([this](const Species::CommandReport&) { ++run.answers; });
    m_dispatcher.Register<Species::SettlementStatus>([this](const Species::SettlementStatus&) { ++run.answers; });
    m_dispatcher.Register<Species::ValleyStatus>([this](const Species::ValleyStatus&) { ++run.answers; });
    m_dispatcher.Register<Species::SettlementNotice>([](const Species::SettlementNotice&) {});
    m_dispatcher.Register<Species::BattleResultNotice>([](const Species::BattleResultNotice&) {});
    m_dispatcher.Register<Species::ServerStats>([](const Species::ServerStats&) {});
    m_dispatcher.Register<Species::AwayReport>([](const Species::AwayReport&) {});
    m_dispatcher.Register<Species::Epilogue>([](const Species::Epilogue&) {});
  }

  // A new connection and its join; the replica starts empty, and no freeze runs across it.
  [[nodiscard]] std::expected<void, std::string> Join(const Species::JoinIdentity& _identity)
  {
    CloseFreezes(false);
    m_replica.Clear();
    m_firstState.reset();
    m_joinRefused = false;
    std::expected<std::unique_ptr<Neuron::QuicTransport>, std::string> transport = Neuron::QuicTransport::Connect(m_connect);
    if (!transport)
    {
      return std::unexpected(transport.error());
    }
    m_transport = std::move(*transport);
    m_connectedAt = m_clock.Now();
    // ADR-186: the join is the client's first message; it waits in the end's backlog for the stream.
    if (!m_transport->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(Species::JoinCommand{_identity})).has_value())
    {
      return std::unexpected(std::string("the join cannot be sent"));
    }
    return {};
  }

  // Leaves: the connection closed after what was sent, the end gone.
  void Leave()
  {
    if (m_transport)
    {
      m_transport->Close();
      run.clientPath = m_transport->PathStats().value_or(run.clientPath);
      m_transport.reset();
    }
  }

  // Everything that arrived, then the statuses asked for at their period of the server's ticks.
  void Poll(std::uint64_t _serverTicks)
  {
    if (!m_transport)
    {
      return;
    }
    while (std::optional<Neuron::ReceivedMessage> message = m_transport->Receive())
    {
      if (!m_dispatcher.Dispatch(message->bytes))
      {
        ++run.messagesDropped;
      }
    }
    if (m_firstState && _serverTicks >= m_nextRequest)
    {
      m_nextRequest = _serverTicks + REQUEST_TICKS;
      if (m_transport->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(Species::ReadSettlementStatusCommand{m_team})).has_value())
      {
        ++run.requestsSent;
      }
      if (m_transport->Send(Neuron::Channel::Reliable, Neuron::EncodeMessage(Species::ReadValleyStatusCommand{m_team})).has_value())
      {
        ++run.requestsSent;
      }
    }
    if (const std::optional<Neuron::QuicPathStats> path = m_transport->PathStats())
    {
      run.clientPath = *path;
    }
  }

  // The time from the connection's start to its first state, once it has one.
  [[nodiscard]] std::optional<Neuron::Nanoseconds> JoinTime() const
  {
    return m_firstState ? std::optional<Neuron::Nanoseconds>(*m_firstState - m_connectedAt) : std::nullopt;
  }

  [[nodiscard]] bool JoinRefused() const noexcept
  {
    return m_joinRefused;
  }

  [[nodiscard]] bool Closed() const
  {
    return !m_transport || m_transport->State() == Neuron::ConnectionState::Closed;
  }

  // The freezes still running at the end, closed as they stand.
  void Finish()
  {
    CloseFreezes(true);
  }

  [[nodiscard]] const std::vector<Freeze>& Freezes() const noexcept
  {
    return m_freezes;
  }

  LinkRun run;

private:
  void Apply(const Species::StateUpdate& _update)
  {
    if (!m_firstState)
    {
      m_firstState = m_clock.Now();
      m_nextEvaluated = _update.tick;
      m_baseTotals = m_replica.Totals();
    }
    else if (!m_replica.LatestTick() || _update.tick > *m_replica.LatestTick())
    {
      // The ticks before this one are done: each compared with the replica as it stands.
      for (std::uint64_t tick = m_nextEvaluated; tick < _update.tick; ++tick)
      {
        Evaluate(tick);
      }
      m_nextEvaluated = std::max(m_nextEvaluated, _update.tick);
    }
    m_replica.Apply(_update);
  }

  void Evaluate(std::uint64_t _tick)
  {
    const std::optional<View> truth = m_world.ViewAt(_tick);
    if (!truth)
    {
      ++run.unevaluatedTicks;
      return;
    }
    ++run.evaluatedTicks;
    const Species::StateReplica::View& replica = m_replica.Latest();
    for (const auto& [id, state] : *truth)
    {
      const auto held = replica.find(id);
      const bool stale = held == replica.end() || !(held->second == state);
      const auto open = m_open.find(id);
      if (stale)
      {
        if (open == m_open.end())
        {
          m_open.emplace(id, OpenFreeze{_tick, 1, std::holds_alternative<Species::ReplicatedFigure>(state)});
        }
        else
        {
          ++open->second.ticks;
        }
      }
      else if (open != m_open.end())
      {
        m_freezes.push_back({id, open->second.firstTick, open->second.ticks, open->second.figure});
        m_open.erase(open);
      }
    }
    // An object that left the view while frozen ends its freeze there.
    for (auto open = m_open.begin(); open != m_open.end();)
    {
      if (!truth->contains(open->first))
      {
        m_freezes.push_back({open->first, open->second.firstTick, open->second.ticks, open->second.figure});
        open = m_open.erase(open);
      }
      else
      {
        ++open;
      }
    }
  }

  // Ends every freeze in progress: kept when the run ends, dropped at a rejoin, whose gap is no freeze.
  void CloseFreezes(bool _keep)
  {
    if (_keep)
    {
      for (const auto& [id, open] : m_open)
      {
        m_freezes.push_back({id, open.firstTick, open.ticks, open.figure});
      }
    }
    m_open.clear();
    run.missingBaselines += m_replica.Totals().missingBaselines - m_baseTotals.missingBaselines;
    run.lateUpdates += m_replica.Totals().lateUpdates - m_baseTotals.lateUpdates;
    m_baseTotals = m_replica.Totals();
  }

  const Neuron::PlatformClock& m_clock;
  const ObservedWorld& m_world;
  Neuron::QuicTransport::ConnectDesc m_connect;
  std::uint8_t m_team = 0;
  Neuron::MessageDispatcher m_dispatcher;
  std::unique_ptr<Neuron::QuicTransport> m_transport;
  Species::StateReplica m_replica;
  Species::StateReplica::Counts m_baseTotals;
  Neuron::Nanoseconds m_connectedAt{0};
  std::optional<Neuron::Nanoseconds> m_firstState;
  std::uint64_t m_nextEvaluated = 0; // the first tick not yet compared, from the connection's first state
  std::uint64_t m_nextRequest = 0;
  bool m_joinRefused = false;
  std::map<Species::StableId, OpenFreeze> m_open;
  std::vector<Freeze> m_freezes;
};

void Count(Freezes& _freezes, std::uint32_t _ticks)
{
  ++_freezes.count;
  _freezes.longestTicks = std::max(_freezes.longestTicks, _ticks);
  ++_freezes.byLength[std::min<std::size_t>(_ticks, FREEZE_LENGTHS) - 1];
}

// Each freeze sorted by the datagrams that carried its object and were lost in it, from its first tick to its last.
void Classify(LinkRun& _run, const std::vector<Freeze>& _freezes, const std::map<Species::StableId, std::vector<std::uint64_t>>& _lost)
{
  for (const Freeze& freeze : _freezes)
  {
    if (!freeze.figure)
    {
      Count(_run.others, freeze.ticks);
      continue;
    }
    std::size_t losses = 0;
    if (const auto found = _lost.find(freeze.id); found != _lost.end())
    {
      const std::uint64_t last = freeze.firstTick + freeze.ticks - 1;
      losses = static_cast<std::size_t>(
        std::ranges::count_if(found->second, [&](std::uint64_t _tick) { return _tick >= freeze.firstTick && _tick <= last; }));
    }
    Count(losses == 0 ? _run.figuresNoLoss : losses == 1 ? _run.figuresSingleLoss : _run.figuresMultipleLoss, freeze.ticks);
  }
}

std::string FreezesJson(const Freezes& _freezes)
{
  std::string lengths;
  for (const std::uint64_t count : _freezes.byLength)
  {
    lengths += std::format("{}{}", lengths.empty() ? "" : ", ", count);
  }
  return std::format("{{\"count\": {}, \"longest_ticks\": {}, \"by_length\": [{}]}}", _freezes.count, _freezes.longestTicks, lengths);
}

std::string PathJson(const Neuron::QuicPathStats& _path)
{
  return std::format("{{\"rtt_us\": {}, \"rtt_variance_us\": {}, \"congestion_window_bytes\": {}, \"path_mtu_bytes\": {}, "
                     "\"packets_sent\": {}, \"packets_suspected_lost\": {}, \"packets_spuriously_lost\": {}, \"packets_received\": {}, "
                     "\"udp_bytes_sent\": {}, \"udp_bytes_received\": {}, \"stream_bytes_sent\": {}, \"stream_bytes_received\": {}, "
                     "\"congestion_events\": {}, \"datagrams_sent\": {}, \"datagram_bytes_sent\": {}, \"datagrams_enabled\": {}, "
                     "\"datagram_max_send_bytes\": {}, \"datagrams_refused\": {}}}",
                     _path.roundTripMicroseconds, _path.roundTripVarianceMicroseconds, _path.congestionWindowBytes, _path.pathMtuBytes,
                     _path.packetsSent, _path.packetsSuspectedLost, _path.packetsSpuriouslyLost, _path.packetsReceived, _path.udpBytesSent,
                     _path.udpBytesReceived, _path.streamBytesSent, _path.streamBytesReceived, _path.congestionEvents, _path.datagramsSent,
                     _path.datagramBytesSent, _path.datagramsEnabled ? "true" : "false", _path.datagramMaxSendBytes,
                     _path.datagramsRefused);
}

} // namespace

std::expected<LinkRun, std::string> RunLink(const LinkSceneDesc& _desc)
{
  const Neuron::PlatformClock clock;
  const std::expected<Neuron::ServerCertificate, std::string> certificate = Neuron::ServerCertificate::OpenOrCreate(LINK_CERTIFICATE_NAME);
  if (!certificate)
  {
    return std::unexpected(std::format("the certificate cannot be made: {}", certificate.error()));
  }
  std::expected<std::unique_ptr<Neuron::QuicListener>, std::string> listener =
    Neuron::QuicListener::Start({_desc.serverPort, &*certificate});
  if (!listener)
  {
    return std::unexpected(std::format("the listener cannot start: {}", listener.error()));
  }
  // A valley of four settlements, the steward's run by its scripted steward and watched, so that its figures move for
  // the whole session (ADR-140, ADR-157); the server's identity makes the client join (ADR-186).
  const auto stewardSite = static_cast<std::uint8_t>(std::clamp<std::int64_t>(_desc.tuning->mvp.sandbox.stewardSite, 0, 3));
  const Species::JoinIdentity identity{};
  Species::WorldServer::Desc worldDesc;
  worldDesc.tuning = *_desc.tuning;
  worldDesc.structures = *_desc.structures;
  worldDesc.worldSeed = WORLD_SEED;
  worldDesc.stewardSite = stewardSite;
  worldDesc.aiSettlements = AI_SETTLEMENTS;
  worldDesc.watched = true;
  worldDesc.identity = identity;
  Species::WorldServer world(worldDesc);
  ObservedListener observedListener(**listener);
  ObservedWorld observed(world, observedListener);
  Neuron::ServerHost::Desc hostDesc;
  hostDesc.clock = &clock;
  hostDesc.tickPeriod = Species::TickPeriodOf(world.Tuning());
  hostDesc.workers = SERVER_WORKERS;
  hostDesc.listener = &observedListener;
  Neuron::ServerHost host(std::move(hostDesc), observed);
  host.Start();

  const std::uint16_t port = _desc.linkPort != 0 ? _desc.linkPort : (*listener)->Port();
  LinkClient client(clock, observed, {port, certificate->Fingerprint()}, stewardSite);
  const auto fail = [&](std::string _why) -> std::expected<LinkRun, std::string>
  {
    client.Leave();
    host.Stop();
    return std::unexpected(std::move(_why));
  };
  // Waits for the connection's first state, or fails.
  const auto join = [&]() -> std::expected<Neuron::Nanoseconds, std::string>
  {
    if (std::expected<void, std::string> joined = client.Join(identity); !joined)
    {
      return std::unexpected(joined.error());
    }
    const Neuron::Nanoseconds started = clock.Now();
    while (!client.JoinTime())
    {
      client.Poll(host.CompletedTicks());
      if (client.JoinRefused())
      {
        return std::unexpected(std::string("the join was refused"));
      }
      if (clock.Now() - started > JOIN_WAIT)
      {
        return std::unexpected(std::format("no state arrived within {} s of connecting to port {}", Seconds(JOIN_WAIT), port));
      }
      clock.WaitUntil(clock.Now() + POLL);
    }
    return *client.JoinTime();
  };
  const std::expected<Neuron::Nanoseconds, std::string> first = join();
  if (!first)
  {
    return fail(first.error());
  }
  client.run.joinSeconds = Seconds(*first);
  while (host.CompletedTicks() < _desc.ticks)
  {
    if (_desc.rejoinTick != 0 && !client.run.rejoined && host.CompletedTicks() >= _desc.rejoinTick)
    {
      // ADR-086's rejoin: the client leaves, and joins again on a new connection.
      client.run.rejoined = true;
      client.Leave();
      const std::expected<Neuron::Nanoseconds, std::string> again = join();
      if (!again)
      {
        return fail(std::format("the rejoin failed: {}", again.error()));
      }
      client.run.rejoinSeconds = Seconds(*again);
    }
    client.Poll(host.CompletedTicks());
    if (client.Closed())
    {
      return fail("the connection closed during the session");
    }
    clock.WaitUntil(clock.Now() + POLL);
  }
  client.Finish();
  client.Leave();
  // The server sees the close before it stops, so that its end's last figures are read.
  const Neuron::Nanoseconds closing = clock.Now();
  while (observed.OpenEnds() > 0 && clock.Now() - closing < STOP_WAIT)
  {
    clock.WaitUntil(clock.Now() + POLL);
  }
  const std::uint64_t ran = host.CompletedTicks();
  host.Stop();

  LinkRun run = client.run;
  run.ticks = ran;
  run.stateDatagrams = observed.Datagrams();
  run.datagramsAcknowledged = observed.Acknowledged();
  run.datagramsLost = observed.Lost();
  run.datagramsUnresolved = observed.Unresolved();
  run.stalls = observed.Stalls();
  Classify(run, client.Freezes(), observed.LostTicks());
  for (const auto& [connection, end] : observed.Ends())
  {
    if (connection == 1 && end.latest)
    {
      run.serverPath = end.latest->path;
    }
    if (end.first && end.latest)
    {
      run.window.stateDatagrams += end.latest->path.datagramsSent - end.first->path.datagramsSent;
      run.window.datagramBytesSent += end.latest->path.datagramBytesSent - end.first->path.datagramBytesSent;
      run.window.reliableBytes += end.latest->reliableBytes - end.first->reliableBytes;
      run.window.packetsSent += end.latest->path.packetsSent - end.first->path.packetsSent;
      run.window.udpBytesSent += end.latest->path.udpBytesSent - end.first->path.udpBytesSent;
      run.window.streamBytesSent += end.latest->path.streamBytesSent - end.first->path.streamBytesSent;
    }
  }

  // ADR-088: the session's journal replayed headless, with no transport at all, gives the same hash on every tick.
  const std::optional<Species::CommandJournal>& journal = world.Journal();
  const std::vector<std::pair<std::uint64_t, std::uint64_t>>& hashes = observed.Hashes();
  std::expected<Species::RegionTerrain, Species::TerrainFailure> terrain =
    journal ? Species::GenerateRegionTerrain(journal->start.seed, world.Tuning()) : std::unexpected(Species::TerrainFailure{});
  if (journal && terrain && !hashes.empty())
  {
    Neuron::WorkerPool pool(SERVER_WORKERS);
    std::vector<std::pair<std::uint64_t, std::uint64_t>> replayed;
    std::uint64_t replayTick = journal->start.firstTick;
    static_cast<void>(Species::ReplayJournal(*journal, std::make_shared<const Species::RegionTerrain>(std::move(*terrain)), world.Tuning(),
                                             &world.Structures(), hashes.back().first + 1, pool, [&](const Species::WorldSimulation& _world)
                                             { replayed.emplace_back(replayTick++, Species::HashWorld(_world).world); }));
    run.hashesCompared = std::min(hashes.size(), replayed.size());
    run.hashesMatch = hashes == replayed;
  }
  return run;
}

std::string LinkJson(const LinkRun& _run)
{
  const LinkWindow& window = _run.window;
  // ADR-088 item 3: what a state datagram costs on the link besides its records: every byte the server sent in the window
  // but the reliable channel's payloads and the datagrams' own, over the datagrams sent, and the message's and Species'
  // headers inside each datagram (ADR-111).
  const double datagrams = static_cast<double>(std::max<std::uint64_t>(1, window.stateDatagrams));
  const double wire = static_cast<double>(window.udpBytesSent) + IPV4_UDP_HEADER_BYTES * static_cast<double>(window.packetsSent);
  const double headerBytes = static_cast<double>(Species::StateUpdate::HEADER_BITS) / 8.0;
  const double recordBytes = static_cast<double>(window.datagramBytesSent) - headerBytes * static_cast<double>(window.stateDatagrams);
  const double overhead =
    (wire - static_cast<double>(window.streamBytesSent) - static_cast<double>(window.datagramBytesSent)) / datagrams + headerBytes;
  return std::format(
    "{{\"ticks\": {}, \"evaluated_ticks\": {}, \"unevaluated_ticks\": {}, \"state_datagrams\": {}, \"datagrams_acknowledged\": {}, "
    "\"datagrams_lost\": {}, \"datagrams_unresolved\": {}, \"figures_single_loss\": {}, \"figures_multiple_loss\": {}, "
    "\"figures_no_loss\": {}, \"others\": {}, \"window\": {{\"state_datagrams\": {}, \"record_bytes\": {:.1f}, \"reliable_bytes\": {}, "
    "\"packets_sent\": {}, \"udp_bytes_sent\": {}, \"stream_bytes_sent\": {}, \"overhead_bytes_per_datagram\": {:.2f}, "
    "\"packets_per_datagram\": {:.3f}}}, \"server_path\": {}, \"client_path\": {}, \"join_s\": {:.3f}, \"rejoined\": {}, "
    "\"rejoin_s\": {:.3f}, \"terrains\": {}, \"requests_sent\": {}, \"answers\": {}, \"missing_baselines\": {}, \"late_updates\": {}, "
    "\"messages_dropped\": {}, \"hashes_compared\": {}, \"hashes_match\": {}, \"stalls\": {}}}",
    _run.ticks, _run.evaluatedTicks, _run.unevaluatedTicks, _run.stateDatagrams, _run.datagramsAcknowledged, _run.datagramsLost,
    _run.datagramsUnresolved, FreezesJson(_run.figuresSingleLoss), FreezesJson(_run.figuresMultipleLoss), FreezesJson(_run.figuresNoLoss),
    FreezesJson(_run.others), window.stateDatagrams, recordBytes, window.reliableBytes, window.packetsSent, window.udpBytesSent,
    window.streamBytesSent, overhead, static_cast<double>(window.packetsSent) / datagrams, PathJson(_run.serverPath),
    PathJson(_run.clientPath), _run.joinSeconds, _run.rejoined ? "true" : "false", _run.rejoinSeconds, _run.terrains, _run.requestsSent,
    _run.answers, _run.missingBaselines, _run.lateUpdates, _run.messagesDropped, _run.hashesCompared, _run.hashesMatch ? "true" : "false",
    _run.stalls);
}

} // namespace Performance
