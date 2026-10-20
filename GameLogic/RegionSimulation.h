#pragma once

#include "CommandQueue.h"
#include "StableId.h"
#include "StableIdCounter.h"

#include <cstdint>

namespace Species
{

/// The owner of one region (ADR-083 item 4, ADR-085 item 1): every mutable value inside the region's square. It steps
/// as one region job and reads no other owner's state (ADR-085 item 2). At M0 the region is empty: it has its command
/// queue, its id counter and its tick, and the settlements, figures and battles of M1 and M2 move in here (MVP-096).
class RegionSimulation
{
public:
  RegionSimulation(std::uint16_t _index, const Neuron::SipHashKey& _simulationKey, std::uint64_t _nextTick);

  // One region job of tick _tick: its commands, then its update slices in order (ADR-079 item 2), of which M0 has none.
  void Step(std::uint64_t _tick);

  [[nodiscard]] std::uint16_t Index() const noexcept
  {
    return m_index;
  }
  [[nodiscard]] OwnerId Owner() const noexcept
  {
    return m_ids.Owner();
  }
  // The next tick to step: the region's tick clock.
  [[nodiscard]] std::uint64_t NextTick() const noexcept
  {
    return m_nextTick;
  }
  [[nodiscard]] Neuron::CommandQueue& Commands() noexcept
  {
    return m_commands;
  }
  [[nodiscard]] const StableIdCounter& Ids() const noexcept
  {
    return m_ids;
  }

private:
  std::uint16_t m_index;
  StableIdCounter m_ids;
  Neuron::CommandQueue m_commands;
  std::uint64_t m_nextTick;
};

} // namespace Species
