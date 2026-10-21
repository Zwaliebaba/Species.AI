#pragma once

#include <cstdint>

namespace Species
{

// The kinds of figure the MVP builds (MVP-040): the Worker and the Leader of the reference crowd rules. The client
// reads the kind of a replicated figure to draw it, so it is a shared type.
enum class FigureKind : std::uint8_t
{
  Worker,
  Leader
};

inline constexpr std::uint8_t FIGURE_KIND_MAX = static_cast<std::uint8_t>(FigureKind::Leader);

// R-UNIT-067: the Worker's behaviour states that the MVP builds; the values are state, never renumbered. The client reads
// a replicated figure's state as its task, to animate it (ADR-111), as it does the Leader's below.
enum class WorkerState : std::uint8_t
{
  Idle,
  OnWaypointOrder,
  Drowning,
  Fighting,
  Burning,
  FollowingLeader
};

// R-UNIT-164: the Leader's behaviour states (§2.4); the values are state, never renumbered.
enum class LeaderState : std::uint8_t
{
  Idle,
  ToWaypoint,
  GivingOrders
};

// R-UNIT-164: the Leader's order, the state machine of diagram 3-B, independent of its behaviour state. The client reads
// it to choose a Leader's banner (R-UNIT-191).
enum class LeaderOrder : std::uint8_t
{
  None,
  PrepareWaypoint,
  Waypoint,
  Follow
};

inline constexpr std::uint8_t LEADER_ORDER_MAX = static_cast<std::uint8_t>(LeaderOrder::Follow);

} // namespace Species
