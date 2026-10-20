#pragma once

#include <cstdint>

namespace Species
{

// The purposes of the simulation's draws (ADR-082 item 3, ADR-097): one enumerator per draw-site family, which keys
// the draw with its subject and tick. A value is never renumbered or reused, because a changed value changes every
// draw of its family; a new family takes the next value.
enum class DrawPurpose : std::uint64_t
{
  ReloadTimer = 1, // R-UNIT-015: an actor's initial reload timer when its kind is set
  SpawnFront = 2   // R-UNIT-059: a Worker spawned at rest faces a random horizontal direction
};

[[nodiscard]] constexpr std::uint64_t PurposeOf(DrawPurpose _purpose) noexcept
{
  return static_cast<std::uint64_t>(_purpose);
}

} // namespace Species
