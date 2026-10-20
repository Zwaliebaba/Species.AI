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
inline constexpr std::uint32_t WORLD_STATE_VERSION = 2;
inline constexpr std::uint32_t WORLD_STATE_OLDEST_VERSION = 1;

// The world's whole state: its simulation key, its region count, the world owner's state and each region's.
[[nodiscard]] Neuron::SnapshotContents SnapshotOf(const WorldSimulation& _world);

// The world a snapshot holds, migrated to WORLD_STATE_VERSION, or why it is refused; never a world in part. Version 2
// added each region's teams and figures (M1 task T2); a version-1 region migrates to one with no teams. _tuning
// outlives the world.
[[nodiscard]] std::expected<std::unique_ptr<WorldSimulation>, Neuron::SnapshotError> RestoreWorld(const Neuron::SnapshotContents& _snapshot,
                                                                                                  const TuningMvp& _tuning);

} // namespace Species
