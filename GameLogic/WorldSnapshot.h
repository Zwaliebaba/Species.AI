#pragma once

#include "Snapshot.h"
#include "WorldSimulation.h"

#include <cstdint>
#include <expected>
#include <memory>

namespace Species
{

// ADR-099, MVP-095: the version of the world's state as a snapshot holds it. A change to any owner's state layout
// raises it, and adds the step that migrates the previous version; a version below the oldest that a step reaches is
// refused.
inline constexpr std::uint32_t WORLD_STATE_VERSION = 4;
inline constexpr std::uint32_t WORLD_STATE_OLDEST_VERSION = 1;

// The world's whole state: its simulation key, its region count, the world owner's state and each region's.
[[nodiscard]] Neuron::SnapshotContents SnapshotOf(const WorldSimulation& _world);

// The world a snapshot holds, migrated to WORLD_STATE_VERSION, or why it is refused; never a world in part. Version 2
// added each region's teams and figures (M1 task T2), and a version-1 region migrates to one with no teams; version 3
// added the world seed (M1 task T3), and an older world migrates to seed 0. The terrain is not state: the world server
// builds it again from the seed. Version 4 added each figure's movement state and each region's actor-grid history and
// cache (M1 task T4); an older figure stands Idle where it was, and the grid registers it there. _tuning outlives the
// world.
[[nodiscard]] std::expected<std::unique_ptr<WorldSimulation>, Neuron::SnapshotError> RestoreWorld(const Neuron::SnapshotContents& _snapshot,
                                                                                                  const TuningMvp& _tuning);

} // namespace Species
