#pragma once

#include "BitReader.h"
#include "BitWriter.h"
#include "CommandQueue.h"
#include "Figure.h"
#include "SimRng.h"
#include "SlicedPool.h"
#include "StableId.h"
#include "StableIdCounter.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace Species
{

/// The owner of one region (ADR-083 item 4, ADR-085 item 1): every mutable value inside the region's square. It steps
/// as one region job and reads no other owner's state (ADR-085 item 2). It holds its teams with their figures, its
/// command queue, its id counter and its tick; the settlements, structures and battles of M2 and M3 move in here
/// (MVP-096).
class RegionSimulation
{
public:
  /// One team slot of the region (R-SIM-221): its ungrouped figures, the only actors of the MVP (R-UNIT-022).
  struct Team
  {
    SlicedPool<Figure> figures;
  };

  // A region's state as a snapshot of WORLD_STATE_VERSION 1 holds it, before its teams existed (ADR-099).
  static constexpr std::uint32_t STATE_VERSION_WITHOUT_TEAMS = 1;

  RegionSimulation(std::uint16_t _index, const Neuron::SipHashKey& _simulationKey, std::uint64_t _nextTick, const TuningMvp& _tuning);

  // One region job of tick _tick (R-SIM-510): its commands, then the update slices 0 to ref.sim.update_slices - 1
  // (R-SIM-511, R-SIM-218), each running the region's teams in index order (R-SIM-221). The weapons, structures and
  // Essence orbs of the slice follow the teams when they exist (R-SIM-512).
  void Step(std::uint64_t _tick);

  // The team slots 0 to _index, made when they do not exist yet.
  Team& EnsureTeam(std::uint8_t _index);
  [[nodiscard]] std::size_t TeamCount() const noexcept
  {
    return m_teams.size();
  }
  [[nodiscard]] Team& TeamAt(std::size_t _index);
  [[nodiscard]] const Team& TeamAt(std::size_t _index) const;

  struct Spawn
  {
    std::uint8_t team = 0;
    FigureKind kind = FigureKind::Worker;
    Neuron::Vec3 position;
    Neuron::Vec3 velocity;
    std::uint32_t count = 1;
    double roamRange = 0.0;
  };
  // R-SIM-226: creates the figures one after another into the team's ungrouped figures and returns their slots. Its
  // draws are the next tick's: inside a step, the tick being stepped. The spawn-position search and the actor-grid
  // registration arrive with the grids (M1 plan T4).
  std::vector<std::uint32_t> SpawnFigures(const Spawn& _spawn);

  // Every mutable value of the region, canonically (ADR-083 item 4): what its hash covers and its snapshot holds.
  void WriteState(Neuron::BitWriter& _writer) const;
  // The values WriteState wrote in a snapshot of _stateVersion, into a region made for the same index and tick; false
  // when they do not read whole. A version-1 region has no teams.
  [[nodiscard]] bool ReadState(Neuron::BitReader& _reader, std::uint32_t _stateVersion);

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
  [[nodiscard]] const TuningMvp& Tuning() const noexcept
  {
    return *m_tuning;
  }

#if ENGINE_DEVELOPMENT
  // A test's view of the update order: called before each figure's advance with its team, slot and slice.
  using AdvanceObserver = std::function<void(std::uint8_t, std::uint32_t, std::uint32_t)>;
  void ObserveAdvances(AdvanceObserver _observer)
  {
    m_advanceObserver = std::move(_observer);
  }
#endif

private:
  void StepSlice(std::uint32_t _slice);
  [[nodiscard]] DrawScope Draws() noexcept;

  std::uint16_t m_index;
  const TuningMvp* m_tuning;
  Neuron::SipHashKey m_key;
  StableIdCounter m_ids;
  Neuron::CommandQueue m_commands;
  std::uint64_t m_nextTick;
  std::vector<Team> m_teams;
#if ENGINE_DEVELOPMENT
  Neuron::DrawLedger m_ledger;
  AdvanceObserver m_advanceObserver;
#endif
};

} // namespace Species
