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
  ShoreRoute = 12,   // R-UNIT-110: the shore search of that waypoint's push
  GrenadeTimer = 13, // R-UNIT-060, R-UNIT-104: the Worker's first grenade timer and each later interval
  CombatMove = 14,   // R-UNIT-102: the combat step's turn and stand-off
  FireRoll = 15,     // R-UNIT-103: the fire roll outside a formation
  AimJitter = 16,    // R-UNIT-052: the base attack's aim jitter, x then z
  LaserHit = 17,     // R-CRATE-443: a bolt's damage and push; the subject is the bolt
  RingIgnite = 18,   // R-CRATE-471: a shockwave ring's ignition rolls; the subject is the ring
  OrbStart = 19,     // R-UNIT-307: an Essence orb's phase and axis rates; the subject is the orb
  OrbStep = 20,      // R-UNIT-311, R-UNIT-315, R-UNIT-319: an orb's draws of one tick
  BurnStep = 21,     // R-UNIT-079: the burning Worker's wander and damage roll
  ShoreCombat = 22,  // R-UNIT-102: the shore search of the combat waypoint's push
  ShoreBurn = 23,    // R-UNIT-079: the shore search of the burning waypoint's push
  FollowDisc = 24,   // R-UNIT-074, R-UNIT-096 step 2: the disc point around a followed Leader
  SignpostPick = 25, // R-UNIT-096 step 3: the choice among the Leaders with a waypoint order
  ShoreFollow = 26,  // R-UNIT-074, R-UNIT-096 step 2: the shore search of the follow waypoint's push
  ShoreLeader = 27,  // R-UNIT-171 step 3: the shore search of a Leader's order position's push; the subject is the Leader
  NodePhase = 28,    // R-BOT-028: a bot navigation node's first recount countdown; the subject is the node's index + 1
  AiCyclePhase = 29, // R-BOT-009: an AI's first main-cycle countdown; the subject is the AI
  AiRecall = 30,     // R-BOT-076: the stray recall's picks; the subject is the AI
  AiSelect = 31,     // R-BOT-088: the random target selection; the subject is the AI
  AiSend = 32,       // R-BOT-085: a dispatch candidate's send draw; the subject is the Worker
  AiOffset = 33,     // R-BOT-076, R-BOT-085, R-UNIT-188: an AI order point's offset; the subject is the figure ordered
  DoorJitter = 34    // R-ECON-043, MVP-021: a Worker's jitter at its Wellspring's door, x, y then z; the subject is the Worker
};

[[nodiscard]] constexpr std::uint64_t PurposeOf(DrawPurpose _purpose) noexcept
{
  return static_cast<std::uint64_t>(_purpose);
}

} // namespace Species
