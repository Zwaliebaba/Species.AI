#include "pch.h"

#include "Check.h"
#include "Log.h"
#include "Profile.h"
#include "SecureRandom.h"
#include "TickReport.h"
#include "WorldServer.h"
#include "WorldSnapshot.h"
#include "WorldTerrain.h"

#include <algorithm>
#include <array>
#include <bit>
#include <stdexcept>
#include <utility>

namespace Species
{

WorldServer::WorldServer(const Desc& _desc)
  : m_desc(_desc)
{
}

std::uint64_t WorldServer::Open()
{
  if (!m_desc.snapshotFolder.empty())
  {
    // MVP-075: the newest snapshot that validates; a refused one is logged and the next older one tried.
    const Neuron::SnapshotStore store(m_desc.snapshotFolder, SnapshotsKept());
    for (const std::filesystem::path& snapshot : store.Snapshots())
    {
      auto restored = Neuron::SnapshotStore::Load(snapshot).and_then([this](const Neuron::SnapshotContents& _contents)
                                                                     { return RestoreWorld(_contents, m_desc.tuning); });
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
      Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "the world opens from {} at tick {}",
                  snapshot.filename().string(), m_world->NextTick());
      return m_world->NextTick();
    }
  }
  // MVP-005: a seed the steward gave, or one the server draws; a drawn seed has 32 bits, so that it can be typed again.
  std::uint64_t seed = 0;
  if (m_desc.worldSeed)
  {
    seed = *m_desc.worldSeed;
  }
  else
  {
    std::array<std::byte, 4> drawn{};
    Neuron::FillSecureRandom(drawn);
    seed = std::bit_cast<std::uint32_t>(drawn);
  }
  std::shared_ptr<const RegionTerrain> terrain = BuildTerrain(seed);
  if (!terrain)
  {
    throw std::runtime_error("no seed that the re-roll reaches gives a region that passes every check");
  }
  // A new world draws its simulation key from the system's cryptographic generator (ADR-082 item 1), and holds the
  // seed that passed (MVP-011), which gives the same terrain at its first attempt.
  WorldSimulation::Desc desc;
  desc.simulationKey = Neuron::DrawSecretKey();
  desc.nextTick = 0;
  desc.regionCount = m_desc.regionCount;
  desc.tuning = &m_desc.tuning;
  desc.worldSeed = terrain->seed;
  Adopt(std::make_unique<WorldSimulation>(desc), std::move(terrain));
  Neuron::Log(Neuron::LogCategory::Simulation, Neuron::LogLevel::Info, "a new world opens with {} region(s) from seed {}",
              m_desc.regionCount, m_world->WorldSeed());
  return m_world->NextTick();
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
  const StableId steward = m_world->Ids().Issue();
  m_stewards.emplace(_connection, Steward{steward, 0, false});
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
  // A command enters the queue of the owner it addresses (ADR-087 item 1). M0 registers no command, so the world
  // owner's queue refuses each one as unknown, with its counter.
  Steward& steward = found->second;
  static_cast<void>(m_world->Commands().Submit(steward.id, steward.commandsReceived++, _message.bytes));
}

void WorldServer::DatagramFates(Neuron::ConnectionId, std::span<const Neuron::DatagramOutcome>)
{
  // The replication baseline of ADR-086 reads the fates from M1; the tick report needs none.
}

void WorldServer::Step(std::uint64_t _tick, Neuron::WorkerPool& _workers)
{
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
  const std::vector<std::byte> report = Neuron::EncodeMessage(TickReport{_tick});
  for (auto& [connection, steward] : m_stewards)
  {
    if (!steward.terrainSent && m_terrain)
    {
      // MVP-011: the seed and digest go once on the reliable channel; a send the backlog refuses is tried next tick.
      steward.terrainSent =
        _sender.Send(connection, Neuron::Channel::Reliable, Neuron::EncodeMessage(WorldTerrain{m_terrain->seed, m_terrain->digest}));
    }
    // A report that cannot be sent is replaced by the next tick's.
    static_cast<void>(_sender.Send(connection, Neuron::Channel::State, report));
  }
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
}

} // namespace Species
