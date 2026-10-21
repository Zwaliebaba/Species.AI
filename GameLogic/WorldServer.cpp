#include "pch.h"

#include "Check.h"
#include "Development.h"
#include "Files.h"
#include "Log.h"
#include "MarkerTimings.h"
#include "Profile.h"
#include "SecureRandom.h"
#include "PresentationEvents.h"
#include "ServerStats.h"
#include "WorldServer.h"
#include "WorldSnapshot.h"
#include "WorldTerrain.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <stdexcept>
#include <utility>

namespace Species
{

namespace
{

// MVP-005: a seed the server draws; it has 32 bits, so that it can be typed again.
std::uint64_t DrawWorldSeed()
{
  std::array<std::byte, 4> drawn{};
  Neuron::FillSecureRandom(drawn);
  return std::bit_cast<std::uint32_t>(drawn);
}

// ADR-106: the outcome as the client reads it.
CommandResult ResultOf(const Neuron::CommandOutcome& _outcome) noexcept
{
  if (!_outcome.rejection)
  {
    return _outcome.effect == Neuron::CommandEffect::Applied ? CommandResult::Applied : CommandResult::NoEffect;
  }
  switch (*_outcome.rejection)
  {
  case Neuron::CommandRejection::UnknownType:
    return CommandResult::UnknownType;
  case Neuron::CommandRejection::WrongVersion:
    return CommandResult::WrongVersion;
  case Neuron::CommandRejection::Oversized:
    return CommandResult::Oversized;
  case Neuron::CommandRejection::Malformed:
    return CommandResult::Malformed;
  case Neuron::CommandRejection::NotPermitted:
    return CommandResult::NotPermitted;
  case Neuron::CommandRejection::Duplicate:
    break;
  }
  return CommandResult::Duplicate;
}

} // namespace

WorldServer::WorldServer(const Desc& _desc)
  : m_desc(_desc)
{
  // ADR-126: a synthetic battle is a sandbox whose two sides the AI commands.
  if (m_desc.syntheticFigures > 0)
  {
    m_desc.sandbox = true;
    m_desc.watched = true;
  }
}

std::uint64_t WorldServer::Open()
{
  if (!m_desc.snapshotFolder.empty())
  {
    // MVP-075: the newest snapshot that validates; a refused one is logged and the next older one tried.
    const Neuron::SnapshotStore store(m_desc.snapshotFolder, SnapshotsKept());
    for (const std::filesystem::path& snapshot : store.Snapshots())
    {
      auto restored = Neuron::SnapshotStore::Load(snapshot).and_then(
        [this](const Neuron::SnapshotContents& _contents) { return RestoreWorld(_contents, m_desc.tuning, &m_desc.structures); });
      if (!restored)
      {
        Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Warning, "the snapshot {} is refused: {}",
                    snapshot.filename().string(), Neuron::SnapshotErrorText(restored.error()));
        continue;
      }
      // The terrain is not state: the restored world builds it again from its seed (ADR-102).
      std::shared_ptr<const RegionTerrain> terrain = BuildTerrain((*restored)->WorldSeed());
      if (!terrain)
      {
        Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Warning, "the snapshot {} is refused: its seed gives no terrain",
                    snapshot.filename().string());
        continue;
      }
      Adopt(std::move(*restored), std::move(terrain));
      m_journal.reset(); // the journal of a restored world comes with saving at M2 (MVP-076)
      Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the world opens from {} at tick {}",
                  snapshot.filename().string(), m_world->NextTick());
      return m_world->NextTick();
    }
  }
  // MVP-005: a seed the steward gave, or one the server draws.
  if (!StartNew(m_desc.worldSeed ? *m_desc.worldSeed : DrawWorldSeed(), 0))
  {
    throw std::runtime_error("no seed that the re-roll reaches gives a region that passes every check");
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "a new {} opens with {} region(s) from seed {}",
              m_desc.sandbox ? "sandbox" : "world", m_desc.regionCount, m_world->WorldSeed());
  return m_world->NextTick();
}

bool WorldServer::StartNew(std::uint64_t _seed, std::uint64_t _firstTick)
{
  std::shared_ptr<const RegionTerrain> terrain = BuildTerrain(_seed);
  if (!terrain)
  {
    return false;
  }
  // A new world draws its simulation key from the system's cryptographic generator (ADR-082 item 1), and holds the
  // seed that passed (MVP-011), which gives the same terrain at its first attempt. Its journal starts with it.
  CommandJournal journal;
  journal.start =
    WorldStart{Neuron::DrawSecretKey(),          terrain->seed,           _firstTick,        m_desc.regionCount, m_desc.sandbox,
               m_desc.sandbox && m_desc.watched, m_desc.syntheticFigures, m_desc.stewardSite};
  std::unique_ptr<WorldSimulation> world = StartWorld(journal.start, terrain, m_desc.tuning, &m_desc.structures);
  Adopt(std::move(world), std::move(terrain));
  m_journal = std::move(journal);
  return true;
}

bool WorldServer::RestartSandbox(bool _newSeed)
{
  const std::uint64_t seed = _newSeed ? DrawWorldSeed() : m_world->WorldSeed();
  const std::uint64_t tick = m_world->NextTick();
  // The refusals the replaced world's queues made on arrival go to their senders; a command they accepted for the next
  // tick goes with that world, without an outcome.
  CollectAllReports();
  if (!StartNew(seed, tick))
  {
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Error, "the sandbox cannot restart from seed {}", seed);
    return false;
  }
  // Each steward is connected to the new world, in connection order, and receives its terrain.
  for (auto& [connection, steward] : m_stewards)
  {
    steward.id = ConnectSteward(*m_world, m_desc.sandbox, m_desc.watched);
    steward.terrainSent = false;
    steward.state.Forget(); // the new world's ids start again (ADR-111)
    if (m_journal)
    {
      m_journal->entries.push_back({tick, true, JOURNAL_WORLD_OWNER, steward.id, 0, {}});
    }
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the sandbox restarts before tick {} from seed {}", tick,
              m_world->WorldSeed());
  return true;
}

std::shared_ptr<const RegionTerrain> WorldServer::BuildTerrain(std::uint64_t _seed) const
{
  ENGINE_PROFILE_SCOPE("world.terrain.build");
  auto built = GenerateRegionTerrain(_seed, m_desc.tuning);
  if (!built)
  {
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Error, "no terrain from seed {}: seed {} failed {} check(s) last", _seed,
                built.error().seed, built.error().checks.size());
    return nullptr;
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the terrain of seed {} passed at attempt {}, digest {}",
              built->seed, built->attempt, Neuron::ShortHex(built->digest));
  return std::make_shared<const RegionTerrain>(std::move(*built));
}

void WorldServer::Adopt(std::unique_ptr<WorldSimulation> _world, std::shared_ptr<const RegionTerrain> _terrain)
{
  // The MVP's generator builds the one region of MVP-010; a world of more regions, which only tests make, leaves the
  // others without terrain.
  _world->Region(0).AttachTerrain(_terrain);
  m_world = std::move(_world);
  m_terrain = std::move(_terrain);
}

std::size_t WorldServer::SnapshotsKept() const noexcept
{
  // mvp.persist.snapshots_kept; a value below 1 keeps the one snapshot a restart needs.
  return static_cast<std::size_t>(std::max<std::int64_t>(1, m_desc.tuning.mvp.persist.snapshotsKept));
}

void WorldServer::Connect(Neuron::ConnectionId _connection)
{
  // ADR-109: the next id of the world owner and, in the sandbox, the steward's side.
  const StableId steward = ConnectSteward(*m_world, m_desc.sandbox, m_desc.watched);
  m_stewards.emplace(_connection, Steward{steward, 0, false, {}, {}});
  if (m_journal)
  {
    m_journal->entries.push_back({m_world->NextTick(), true, JOURNAL_WORLD_OWNER, steward, 0, {}});
  }
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "connection {} is steward {:#x}", _connection, steward);
}

void WorldServer::Disconnect(Neuron::ConnectionId _connection)
{
  m_stewards.erase(_connection);
}

void WorldServer::Receive(Neuron::ConnectionId _connection, const Neuron::ReceivedMessage& _message)
{
  const auto found = m_stewards.find(_connection);
  if (found == m_stewards.end() || _message.channel != Neuron::Channel::Reliable)
  {
    return; // the control states of the state channel arrive with M1's input (ADR-087 item 5)
  }
  // A command enters the queue of the owner it addresses (ADR-087 item 1): a battle command the queue of the MVP's one
  // region (MVP-001, ADR-106), any other the world owner's, which refuses a type it does not know with its counter.
  Neuron::BitReader reader(_message.bytes);
  const Neuron::MessageHeader header = Neuron::ReadMessageHeader(reader);
  Steward& steward = found->second;
  const std::uint64_t sequence = steward.commandsReceived++;
#if ENGINE_DEVELOPMENT
  // ADR-109: the sandbox's restart replaces the world at once, between ticks, rather than entering a queue; what arrives
  // after it enters the new world.
  if (!reader.Failed() && header.type == SandboxRestartCommand::TYPE && m_desc.sandbox)
  {
    SandboxRestartCommand restart;
    restart.Decode(reader);
    CommandResult result = CommandResult::Malformed;
    if (header.version != SandboxRestartCommand::VERSION)
    {
      result = CommandResult::WrongVersion;
    }
    else if (_message.bytes.size() > SandboxRestartCommand::MAX_BYTES)
    {
      result = CommandResult::Oversized;
    }
    else if (reader.Finish())
    {
      result = RestartSandbox(restart.newSeed) ? CommandResult::Applied : CommandResult::NoEffect;
    }
    steward.reports.push_back({sequence, m_world->NextTick(), result});
    return;
  }
#endif
  Neuron::CommandQueue& region = m_world->Region(0).Commands();
  const bool regional = !reader.Failed() && region.Handles(header.type);
  Neuron::CommandQueue& queue = regional ? region : m_world->Commands();
  if (queue.Submit(steward.id, sequence, _message.bytes) && m_journal)
  {
    // MVP-076: the command with the tick it executes in, which is the world's next.
    m_journal->entries.push_back({m_world->NextTick(), false, regional ? std::uint16_t{1} : JOURNAL_WORLD_OWNER, steward.id, sequence,
                                  std::vector<std::byte>(_message.bytes.begin(), _message.bytes.end())});
  }
}

void WorldServer::CollectAllReports()
{
  CollectReports(m_world->Commands());
  for (std::size_t region = 0; region < m_world->RegionCount(); ++region)
  {
    CollectReports(m_world->Region(region).Commands());
  }
}

void WorldServer::CollectReports(Neuron::CommandQueue& _queue)
{
  for (const Neuron::CommandOutcome& outcome : _queue.TakeOutcomes())
  {
    for (auto& [connection, steward] : m_stewards)
    {
      if (steward.id == outcome.sender)
      {
        steward.reports.push_back({outcome.sequence, outcome.tick, ResultOf(outcome)});
      }
    }
  }
}

void WorldServer::DatagramFates(Neuron::ConnectionId _connection, std::span<const Neuron::DatagramOutcome> _outcomes)
{
  // ADR-086 item 3: an acknowledged state datagram makes its records the client's baselines.
  if (const auto found = m_stewards.find(_connection); found != m_stewards.end())
  {
    found->second.state.Fates(_outcomes, m_world->NextTick());
  }
}

void WorldServer::Step(std::uint64_t _tick, Neuron::WorkerPool& _workers)
{
#if ENGINE_DEVELOPMENT
  // ADR-120: the tick's wall time starts here, for the overlay; a measurement the simulated state never reads.
  m_statsTickStart = Neuron::MarkerClockNanoseconds();
#endif
  m_world->Step(_tick, _workers);
#if ENGINE_DEVELOPMENT
  // Debug and test runs hash every tick (ADR-081 item 8); a Release server hashes at its saves.
  WorldHashes hashes = HashWorld(*m_world);
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Trace, "tick {} hash {:016x}", _tick, hashes.world);
  m_lastHashes = std::move(hashes);
#endif
}

void WorldServer::Publish(std::uint64_t _tick, Neuron::ConnectionSender& _sender)
{
  CollectAllReports();
  // ADR-111: the whole battle of the MVP's one region, until the seen layer of M3 narrows it (MVP-097).
  const RegionSimulation& region = m_world->Region(0);
  const std::vector<ReplicatedObject> objects = CollectReplicatedObjects(region);
  for (auto& [connection, steward] : m_stewards)
  {
    // ADR-095: each outcome on the reliable channel, in order; one the backlog refuses waits with those after it.
    std::size_t sent = 0;
    while (sent < steward.reports.size() &&
           _sender.Send(connection, Neuron::Channel::Reliable, Neuron::EncodeMessage(steward.reports[sent])).has_value())
    {
      ++sent;
    }
    steward.reports.erase(steward.reports.begin(), steward.reports.begin() + static_cast<std::ptrdiff_t>(sent));
    if (!steward.terrainSent && m_terrain)
    {
      // MVP-011: the seed and digest go once on the reliable channel, with the team the steward commands (ADR-119); a
      // send the backlog refuses is tried next tick.
      const WorldTerrain terrain{m_terrain->seed, m_terrain->digest, region.TeamCommandedBy(steward.id)};
      steward.terrainSent = _sender.Send(connection, Neuron::Channel::Reliable, Neuron::EncodeMessage(terrain)).has_value();
    }
    // ADR-086 item 3: the tick's state datagrams against what the client has acknowledged; one the transport refuses
    // is lost, and its records go again next tick.
    for (StateSender::Outgoing& outgoing : steward.state.Build(_tick, ViewForSteward(objects, region, steward.id)))
    {
      if (const std::optional<std::uint64_t> datagram =
            _sender.Send(connection, Neuron::Channel::State, Neuron::EncodeMessage(outgoing.update)))
      {
        steward.state.Sent(*datagram, std::move(outgoing));
      }
    }
    // ADR-084 item 5: the tick's droppable events, a team's selections and orders to its commander only (ADR-106).
    std::vector<EffectEvent> events;
    for (const EffectEvent& event : region.Events())
    {
      if (!IsCommandersEvent(event.kind) || region.CommanderOf(event.team).sender == steward.id)
      {
        events.push_back(event);
      }
    }
    for (const PresentationEvents& message : PackPresentationEvents(_tick, events))
    {
      static_cast<void>(_sender.Send(connection, Neuron::Channel::State, Neuron::EncodeMessage(message)));
    }
  }
#if ENGINE_DEVELOPMENT
  // ADR-120: the tick's wall time from its step to the end of its replication, and once a second of ticks their
  // statistics to every steward.
  if (m_statsTickStart != 0)
  {
    const std::int64_t tickNanoseconds = Neuron::MarkerClockNanoseconds() - std::exchange(m_statsTickStart, 0);
    ++m_statsTicks;
    m_statsTickNanoseconds += tickNanoseconds;
    m_statsTickMaxNanoseconds = std::max(m_statsTickMaxNanoseconds, tickNanoseconds);
  }
  const auto ticksPerSecond = static_cast<std::uint32_t>(std::max(1.0, std::round(1.0 / m_desc.tuning.ref.sim.tickPeriod)));
  if (m_statsTicks >= ticksPerSecond)
  {
    ServerStats stats;
    stats.worldSeed = m_world->WorldSeed();
    stats.tick = _tick;
    stats.ticks = m_statsTicks;
    stats.tickMeanMicroseconds = static_cast<std::uint32_t>(m_statsTickNanoseconds / m_statsTicks / 1'000);
    stats.tickMaxMicroseconds = static_cast<std::uint32_t>(m_statsTickMaxNanoseconds / 1'000);
    const std::vector<std::byte> bytes = Neuron::EncodeMessage(stats);
    for (const auto& [connection, steward] : m_stewards)
    {
      static_cast<void>(_sender.Send(connection, Neuron::Channel::Reliable, bytes));
    }
    m_statsTicks = 0;
    m_statsTickNanoseconds = 0;
    m_statsTickMaxNanoseconds = 0;
  }
#endif
}

void WorldServer::Stalled(const Neuron::ScheduleStall& _stall)
{
  if (_stall.freeze)
  {
    // The world record that holds freezes comes with saving at M2 (PIL-131, MVP-075); the loop has logged it.
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Warning, "freeze before tick {}, cause server stall", _stall.tick);
  }
}

void WorldServer::Freeze(std::uint64_t _nextTick)
{
  ENGINE_ASSERT(m_world == nullptr || m_world->NextTick() == _nextTick);
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the world freezes before tick {}", _nextTick);
  if (m_world == nullptr || m_desc.snapshotFolder.empty())
  {
    return;
  }
  // Freezing writes a final snapshot and loses nothing (ADR-083 item 9; MVP-075's snapshot stands in for the cut).
  Neuron::SnapshotStore store(m_desc.snapshotFolder, SnapshotsKept());
  if (const auto saved = store.Save(SnapshotOf(*m_world)); saved)
  {
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the world is saved in {}", saved->filename().string());
  }
  else
  {
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Error, "the world could not be saved in {}",
                m_desc.snapshotFolder.string());
  }
  // MVP-076: the journal of a world opened new in this run, beside the snapshots.
  if (m_journal && !Neuron::ReplaceFileAtomically(m_desc.snapshotFolder / JOURNAL_FILE_NAME, WriteJournal(*m_journal)))
  {
    Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Error, "the command journal could not be written");
  }
}

} // namespace Species
