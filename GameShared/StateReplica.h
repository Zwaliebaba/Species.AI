#pragma once

#include "ReplicatedState.h"
#include "StableId.h"
#include "StateUpdate.h"

#include <cstdint>
#include <map>
#include <optional>
#include <vector>

namespace Species
{

/// The client's replica (ADR-084 item 3, ADR-111): the decoded state of every object of its interest set, keyed by
/// stable id, at the two latest ticks received. It applies each state datagram as it arrives: the first datagram of a
/// newer tick moves the latest view to the previous one, and every record changes the latest. An object no record names
/// keeps its state, so a lost datagram leaves its objects where they were for a tick. Each state received is kept for
/// STATE_HISTORY_TICKS, as the baseline a later delta may name. It is the decoding half of the codec, and holds nothing
/// of the server's; it lives beside the codec so that both halves are tested together.
class StateReplica
{
public:
  using View = std::map<StableId, ReplicatedState>;

  struct Counts
  {
    std::uint64_t updates = 0;
    std::uint64_t records = 0;
    std::uint64_t missingBaselines = 0; // a delta whose baseline the replica does not hold: its record is skipped
    std::uint64_t lateUpdates = 0;      // an update of a tick before the latest, kept as baselines only
  };

  void Apply(const StateUpdate& _update);
  // Forgets everything, as at a new connection.
  void Clear();

  [[nodiscard]] std::optional<std::uint64_t> LatestTick() const noexcept
  {
    return m_latestTick;
  }
  [[nodiscard]] std::optional<std::uint64_t> PreviousTick() const noexcept
  {
    return m_previousTick;
  }
  [[nodiscard]] const View& Latest() const noexcept
  {
    return m_latest;
  }
  // The view at the previous tick received, without the objects that entered whole at the latest: an object that
  // enters is drawn from its first state (ADR-084 item 4).
  [[nodiscard]] const View& Previous() const noexcept
  {
    return m_previous;
  }
  [[nodiscard]] const Counts& Totals() const noexcept
  {
    return m_counts;
  }

private:
  struct Received
  {
    std::uint64_t tick = 0;
    ReplicatedState state;
  };

  [[nodiscard]] const ReplicatedState* Baseline(StableId _id, std::uint64_t _tick) const;
  void Remember(StableId _id, std::uint64_t _tick, const ReplicatedState& _state);
  void Prune();

  std::map<StableId, std::vector<Received>> m_history; // each object's received states, oldest first
  View m_latest;
  View m_previous;
  std::optional<std::uint64_t> m_latestTick;
  std::optional<std::uint64_t> m_previousTick;
  Counts m_counts;
};

} // namespace Species
