#pragma once

#include <cstdint>

namespace Species
{

// The purposes of the simulation's draws (ADR-082 item 3, ADR-097): one enumerator per draw-site family, which keys
// the draw with its subject and tick. A value is never renumbered or reused, because a changed value changes every
// draw of its family; a new family takes the next value.
enum class DrawPurpose : std::uint64_t
{
  ReloadTimer = 1,   // R-UNIT-015: an actor's initial reload timer when its kind is set
  SpawnFront = 2,    // R-UNIT-059: a Worker spawned at rest faces a random horizontal direction
  SpawnSearch = 3,   // R-SIM-368: the spawn-position search's distance and angle per attempt
  RetargetTimer = 4, // R-UNIT-065: the Worker's retarget timer after its task search
  Wander = 5,        // R-UNIT-097: the random-position search's chance and angle
  OrderDisc = 6,     // R-UNIT-111: a waypoint order's disc point
  ShoreMove = 7,     // R-SIM-379, R-UNIT-112: the shore search of the push of a figure's own move
  ShoreFall = 8,     // R-SIM-371: the shore search of the airborne step's discarded push
  ShoreWander = 9,   // R-UNIT-097: the shore search of the random-position search's push
  ShoreOrder = 10,   // R-UNIT-111: the shore search of a waypoint order's push
  RouteJitter = 11,  // R-UNIT-110: the jitter of a route's next waypoint
  ShoreRoute = 12    // R-UNIT-110: the shore search of that waypoint's push
};

[[nodiscard]] constexpr std::uint64_t PurposeOf(DrawPurpose _purpose) noexcept
{
  return static_cast<std::uint64_t>(_purpose);
}

} // namespace Species
