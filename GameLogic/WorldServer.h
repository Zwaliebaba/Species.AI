#pragma once

#include "CommandJournal.h"
#include "CommandMessages.h"
#include "RegionTerrain.h"
#include "ServerHost.h"
#include "SettlementMessages.h"
#include "Snapshot.h"
#include "StableId.h"
#include "StateHash.h"
#include "StateSender.h"
#include "StructureRecords.h"
#include "TuningMvp.h"
#include "WorldSimulation.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <vector>

namespace Species
{

/// The game's world as the server host runs it (ADR-083, ADR-095): it opens and freezes the world, turns each
/// connection into a steward, passes their messages to the command queue of the owner they address, and publishes each
/// tick to every connection. With a snapshot folder it opens the newest snapshot that validates, saves one every
/// mvp.persist.snapshot_seconds and when it freezes, cuts its journal at each, and records an unclean stop and every
/// stall that froze it in the world record (MVP-075, MVP-076, PIL-131, ADR-099, ADR-137); without one, or with none that
/// validates, the world opens new. Opening builds
/// the region's terrain from the world seed (MVP-011, ADR-102), outside every tick. Every connection is a steward; it
/// receives the world's terrain once, the outcome of each command it sent (ADR-106), the status of a settlement it
/// commands when it asks for it (MVP-098, ADR-136), and each tick the state of the
/// whole battle against what it has acknowledged, with the tick's presentation events (ADR-111). A
/// sandbox server makes each new world the M1 sandbox, gives its steward side to the latest steward, and restarts it on
/// the development command (ADR-109). It journals its connections and commands (MVP-076).
class WorldServer final : public Neuron::ServerWorld
{
public:
  struct Desc
  {
    std::uint16_t regionCount = 1;
    TuningMvp tuning;                        // read when the server started (MVP-099); the world reads it while it runs
    StructureRecords structures;             // the structure meshes' simulation data records, read when it started (ADR-129)
    std::optional<std::uint64_t> worldSeed;  // MVP-005: the seed a new world is built from; none: a seed the server draws
    std::filesystem::path snapshotFolder;    // where the world is saved when it freezes and restored when it opens; none: a new
                                             // world each run (ADR-099)
    bool sandbox = false;                    // M1 plan §6: a new world is the battle sandbox
    bool watched = false;                    // O77: the sandbox's AI commands both sides, and its stewards none
    std::uint32_t syntheticFigures = 0;      // ADR-126: the sandbox's sides are a synthetic battle of this many figures, watched
    std::optional<std::uint8_t> stewardSite; // MVP-012: a new world that is no sandbox has the steward's settlement here
    // The calendar time, for the world record alone (ADR-137, PLT-053); none: the record holds no wall times.
    std::function<std::chrono::system_clock::time_point()> calendar;
  };

  // The marker a world that froze cleanly leaves beside its snapshots, naming its final snapshot's tick (ADR-137).
  static constexpr const wchar_t* CLEAN_STOP_FILE_NAME = L"clean-stop.marker";

  // MVP-076 (ADR-137): the journal file that replays from tick _tick, beside the snapshot of that tick.
  [[nodiscard]] static std::filesystem::path JournalPath(const std::filesystem::path& _folder, std::uint64_t _tick);

  explicit WorldServer(const Desc& _desc);

  [[nodiscard]] std::uint64_t Open() override;
  void Connect(Neuron::ConnectionId _connection) override;
  void Disconnect(Neuron::ConnectionId _connection) override;
  void Receive(Neuron::ConnectionId _connection, const Neuron::ReceivedMessage& _message) override;
  void DatagramFates(Neuron::ConnectionId _connection, std::span<const Neuron::DatagramOutcome> _outcomes) override;
  void Step(std::uint64_t _tick, Neuron::WorkerPool& _workers) override;
  void Publish(std::uint64_t _tick, Neuron::ConnectionSender& _sender) override;
  void Stalled(const Neuron::ScheduleStall& _stall) override;
  void Freeze(std::uint64_t _nextTick) override;

  // The tick thread's, while the host runs; any thread once it has stopped.
  [[nodiscard]] const WorldSimulation* Simulation() const noexcept
  {
    return m_world.get();
  }
  // The hashes at the end of the last tick, which development builds compute every tick (ADR-081 item 8, DBG-147).
  [[nodiscard]] const std::optional<WorldHashes>& LastHashes() const noexcept
  {
    return m_lastHashes;
  }
  [[nodiscard]] const TuningMvp& Tuning() const noexcept
  {
    return m_desc.tuning;
  }
  [[nodiscard]] const StructureRecords& Structures() const noexcept
  {
    return m_desc.structures;
  }
  // The region's terrain once the world has opened; its seed is the one the world shows (MVP-005).
  [[nodiscard]] const RegionTerrain* Terrain() const noexcept
  {
    return m_terrain.get();
  }
  // The journal since the world opened or since its last snapshot (ADR-109, ADR-137).
  [[nodiscard]] const std::optional<CommandJournal>& Journal() const noexcept
  {
    return m_journal;
  }

private:
  [[nodiscard]] std::size_t SnapshotsKept() const noexcept;
  // MVP-075: the snapshot period in ticks, mvp.persist.snapshot_seconds at the tick period; 0 saves only at a freeze.
  [[nodiscard]] std::uint64_t SnapshotTicks() const noexcept;
  // The calendar time in whole seconds since 1970, or none.
  [[nodiscard]] std::optional<std::int64_t> CalendarSeconds() const;
  // MVP-075, MVP-076 (ADR-137): a snapshot of the world as it stands; when it is saved, the journal since the last cut
  // goes beside the snapshot it started from, a new journal starts from this one, and the journals of snapshots no longer
  // kept are removed. False, logged, when the snapshot is not saved.
  bool SaveCut();
  // The journal that starts from the world as it stands, restored from its snapshot.
  [[nodiscard]] CommandJournal RestoredJournal() const;
  // WLD-160 to WLD-163 from _seed; none, logged, when no seed that MVP-011's re-roll reaches passes every check.
  [[nodiscard]] std::shared_ptr<const RegionTerrain> BuildTerrain(std::uint64_t _seed) const;
  void Adopt(std::unique_ptr<WorldSimulation> _world, std::shared_ptr<const RegionTerrain> _terrain);
  // A new world from _seed's terrain, starting at _firstTick, with a new key, and its journal; false when no seed that
  // the re-roll reaches gives a region.
  [[nodiscard]] bool StartNew(std::uint64_t _seed, std::uint64_t _firstTick);
  // ADR-109: the sandbox again, from the next tick, from its seed or a drawn one; each steward is connected to it anew.
  // False, logged, when no seed that the re-roll reaches gives a region, and the world stays.
  bool RestartSandbox(bool _newSeed);

  struct Steward
  {
    StableId id = 0;
    std::uint64_t commandsReceived = 0;    // the sequence number of the next command (ADR-095)
    bool terrainSent = false;              // MVP-011: the world's terrain goes to each connection once
    std::vector<CommandReport> reports;    // the outcomes not yet sent, in the order they were made
    StateSender state;                     // ADR-111: what its client holds of the replicated state
    std::vector<std::uint8_t> statusTeams; // MVP-098: the settlements whose status it asked for, by team, not yet sent
    std::vector<SettlementNotice> notices; // ADR-139: the guaranteed events of its teams not yet sent, in order
  };

  // Each queue's outcomes of the tick, to the steward that sent each command; an AI's are its own (ADR-106).
  void CollectReports(Neuron::CommandQueue& _queue);
  // The outcomes of every queue of the world.
  void CollectAllReports();

  Desc m_desc;
  std::unique_ptr<WorldSimulation> m_world;
  std::shared_ptr<const RegionTerrain> m_terrain;
  std::map<Neuron::ConnectionId, Steward> m_stewards;
  std::optional<WorldHashes> m_lastHashes;
  std::optional<CommandJournal> m_journal;
  // ADR-120: the ticks of the current second, for the development overlay's statistics (Debug builds): when the tick
  // being measured started its step, none outside a tick, and the ticks' summed and longest wall time.
  std::int64_t m_statsTickStart = 0;
  std::uint32_t m_statsTicks = 0;
  std::int64_t m_statsTickNanoseconds = 0;
  std::int64_t m_statsTickMaxNanoseconds = 0;
};

} // namespace Species
