#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "CommandQueue.h"
#include "RegionSimulation.h"
#include "SipHash.h"
#include "StableIdCounter.h"
#include "WorkerPool.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace Species
{

/// The owner of the world-level state (ADR-083 item 4, ADR-085 item 1) and of the world's regions. It holds the world
/// tick and the simulation key, and steps a tick whole (ADR-079 item 2): every region as a job on the workers, then its
/// own step. At M0 the world is one empty region (MVP-001, MVP-096).
class WorldSimulation
{
public:
  struct Desc
  {
    Neuron::SipHashKey simulationKey; // ADR-082 item 1: never shown, sent or logged
    std::uint64_t nextTick = 0;
    std::uint16_t regionCount = 1;
  };

  explicit WorldSimulation(const Desc& _desc);

  void Step(std::uint64_t _tick, Neuron::WorkerPool& _workers);

  // The world owner's own mutable values, canonically: the world tick, its id counter and its queued commands. The
  // simulation key is held apart: a snapshot saves it, and no hash covers it (ADR-082 item 1).
  void WriteState(Neuron::BitWriter& _writer) const;
  // The world owner's values and then each region's, as a snapshot holds them (ADR-099), into a world made with the same
  // key, tick and region count; false when they do not read whole.
  [[nodiscard]] bool ReadState(Neuron::BitReader& _reader);

  // The world tick: the next tick to step.
  [[nodiscard]] std::uint64_t NextTick() const noexcept
  {
    return m_nextTick;
  }
  [[nodiscard]] const Neuron::SipHashKey& SimulationKey() const noexcept
  {
    return m_key;
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

private:
  Neuron::SipHashKey m_key;
  std::uint64_t m_nextTick;
  StableIdCounter m_ids;
  Neuron::CommandQueue m_commands;
  std::vector<std::unique_ptr<RegionSimulation>> m_regions;
};

} // namespace Species
