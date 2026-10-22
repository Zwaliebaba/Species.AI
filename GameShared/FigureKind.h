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
  FollowingLeader,
  ToPort,  // R-UNIT-069, from M2 task T5
  Manning, // R-UNIT-070
  Carrying // MVP-031: a load to its drop-off point, from M2 task T7
};

// MVP-036: the settlement task a Worker holds, which its task search chose by the settlement's shares (ADR-135); the
// values are state, never renumbered.
enum class WorkerTask : std::uint8_t
{
  None,
  Gather,
  Construct,
  Guard
};

inline constexpr std::uint8_t WORKER_TASK_MAX = static_cast<std::uint8_t>(WorkerTask::Guard);

// MVP-037: why a Worker of a settlement is idle, which the settlement panel shows; the values are state, never
// renumbered.
enum class IdleReason : std::uint8_t
{
  None,
  NoFreePort, // its task has no free port
  QueueEmpty, // construction has no site
  StoreFull,  // a carrier waits at its drop-off point
  ShareMet    // every task with work has its share
};

inline constexpr std::uint8_t IDLE_REASON_MAX = static_cast<std::uint8_t>(IdleReason::ShareMet);

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
