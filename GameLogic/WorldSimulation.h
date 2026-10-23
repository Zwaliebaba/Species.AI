#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "CommandQueue.h"
#include "RegionSimulation.h"
#include "SipHash.h"
#include "StableIdCounter.h"
#include "StructureRecords.h"
#include "TuningMvp.h"
#include "WorkerPool.h"
#include "WorldRecord.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Species
{

/// The owner of the world-level state (ADR-083 item 4, ADR-085 item 1) and of the world's regions. It holds the world
/// tick and the simulation key, and steps a tick whole (ADR-079 item 2): every region as a job on the workers, then its
/// own step. The MVP's world is one region (MVP-001, MVP-096).
class WorldSimulation
{
public:
  struct Desc
  {
    Neuron::SipHashKey simulationKey; // ADR-082 item 1: never shown, sent or logged
    std::uint64_t nextTick = 0;
    std::uint16_t regionCount = 1;
    const TuningMvp* tuning = nullptr;            // MVP-099: read when the server started, and outliving the world
    std::uint64_t worldSeed = 0;                  // MVP-005, WLD-161: the public seed the world's terrain is built from
    const StructureRecords* structures = nullptr; // ADR-130: what its regions build structures from, outliving the world
  };

  // The world owner's state as a snapshot of WORLD_STATE_VERSION 3 and later holds it, with the world seed (ADR-102).
  static constexpr std::uint32_t STATE_VERSION_WITH_SEED = 3;
  // ... and of version 15 and later, with its run (ADR-163).
  static constexpr std::uint32_t STATE_VERSION_WITH_RUN = 15;

  /// MVP-060 and MVP-063 (ADR-163): the world's run: the tick it ends at, mvp.run.length_hours after the tick it began,
  /// whether it has ended, which it does at that tick once no battle is in progress, and the tick it ended.
  struct Run
  {
    std::uint64_t endTick = 0;
    bool ended = false;
    std::uint64_t endedTick = 0;

    bool operator==(const Run&) const = default;
  };

  explicit WorldSimulation(const Desc& _desc);

  // One tick: every region as a job, then the world owner's own step and its run's end (MVP-060). A world whose run has
  // ended is read-only (MVP-063): its tick advances and nothing else.
  void Step(std::uint64_t _tick, Neuron::WorkerPool& _workers);

  // The world owner's own values, canonically: the world tick, the world seed, its id counter, its queued commands and
  // its run.
  // The simulation key is held apart: a snapshot saves it, and no hash covers it (ADR-082 item 1).
  void WriteState(Neuron::BitWriter& _writer) const;
  // The world owner's values and then each region's, as a snapshot of _stateVersion holds them (ADR-099), into a world
  // made with the same key, tick and region count; false when they do not read whole.
  [[nodiscard]] bool ReadState(Neuron::BitReader& _reader, std::uint32_t _stateVersion);

  // The world tick: the next tick to step.
  [[nodiscard]] std::uint64_t NextTick() const noexcept
  {
    return m_nextTick;
  }
  [[nodiscard]] const Neuron::SipHashKey& SimulationKey() const noexcept
  {
    return m_key;
  }
  // The seed the world's terrain is built from (ADR-102); public, unlike the key.
  [[nodiscard]] std::uint64_t WorldSeed() const noexcept
  {
    return m_worldSeed;
  }
  [[nodiscard]] std::size_t RegionCount() const noexcept
  {
    return m_regions.size();
  }
  [[nodiscard]] RegionSimulation& Region(std::size_t _index);
  [[nodiscard]] const RegionSimulation& Region(std::size_t _index) const;
  // World-level commands, such as a declared attack (ADR-087 item 1).
  [[nodiscard]] Neuron::CommandQueue& Commands() noexcept
  {
    return m_commands;
  }
  [[nodiscard]] const Neuron::CommandQueue& Commands() const noexcept
  {
    return m_commands;
  }
  // The world owner issues the stable ids of what belongs to no region: the stewards among them.
  [[nodiscard]] StableIdCounter& Ids() noexcept
  {
    return m_ids;
  }
  [[nodiscard]] const StableIdCounter& Ids() const noexcept
  {
    return m_ids;
  }
  // MVP-060 (ADR-163): the world's run, and the ticks left in it.
  [[nodiscard]] const Run& RunState() const noexcept
  {
    return m_run;
  }
  [[nodiscard]] std::uint64_t TicksLeft() const noexcept
  {
    return m_run.endTick > m_nextTick ? m_run.endTick - m_nextTick : 0;
  }
  // The run ends at _endTick from now on, which the regions read for its quiet minutes; for the tests and tools that
  // shorten a run.
  void SetRunEnd(std::uint64_t _endTick);
  // ADR-137: the world record, which no state hash covers and the snapshot holds.
  [[nodiscard]] WorldRecord& Record() noexcept
  {
    return m_record;
  }
  [[nodiscard]] const WorldRecord& Record() const noexcept
  {
    return m_record;
  }

private:
  Neuron::SipHashKey m_key;
  std::uint64_t m_nextTick;
  std::uint64_t m_worldSeed;
  StableIdCounter m_ids;
  Neuron::CommandQueue m_commands;
  std::vector<std::unique_ptr<RegionSimulation>> m_regions;
  WorldRecord m_record;
  Run m_run;
};

// MVP-060: the ticks of a run, mvp.run.length_hours at the session pace (MVP-070).
[[nodiscard]] std::uint64_t RunTicks(const TuningMvp& _tuning) noexcept;

} // namespace Species
