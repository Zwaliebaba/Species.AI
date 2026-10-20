#pragma once

#include "ActorGrid.h"
#include "BitReader.h"
#include "BitWriter.h"
#include "FigureKind.h"
#include "RouteStore.h"
#include "StableId.h"
#include "TickDraws.h"
#include "TuningMvp.h"
#include "Vec3.h"

#include <cstdint>
#include <vector>

namespace Species
{

// R-UNIT-025: the damage kinds of a health change; their order carries no meaning.
enum class DamageKind : std::uint8_t
{
  Unresistable,
  Laser,
  Explosive,
  TurretRound,
  Fire
};

// R-UNIT-067: the Worker's behaviour states that the MVP builds; the values are state, never renumbered.
enum class WorkerState : std::uint8_t
{
  Idle,
  OnWaypointOrder,
  Drowning,
  Fighting,
  Burning,
  FollowingLeader
};

// R-UNIT-064, R-UNIT-101: what a Worker's threat is. The MVP's threats are actors and grenades.
enum class ThreatKind : std::uint8_t
{
  None,
  Actor,
  Grenade
};

/// A Worker's current threat: an actor by its actor-grid entry, or a grenade by its slot in the region's effects and its
/// id. Either resolves only while the object still holds that slot.
struct Threat
{
  ThreatKind kind = ThreatKind::None;
  std::uint8_t team = 0;
  std::uint32_t slot = 0;
  StableId id = 0;

  friend constexpr bool operator==(const Threat& _a, const Threat& _b) noexcept = default;
};

// R-UNIT-164: the Leader's behaviour states (§2.4); the values are state, never renumbered.
enum class LeaderState : std::uint8_t
{
  Idle,
  ToWaypoint,
  GivingOrders
};

// R-UNIT-164: the Leader's order, the state machine of diagram 3-B, independent of its behaviour state.
enum class LeaderOrder : std::uint8_t
{
  None,
  PrepareWaypoint,
  Waypoint,
  Follow
};

/// One slot of a Leader's formation (R-UNIT-177): the stable id of its member Worker, 0 for a gap, and its timer.
struct FormationSlot
{
  StableId member = 0;
  std::int32_t timerTicks = 0;

  friend constexpr bool operator==(const FormationSlot& _a, const FormationSlot& _b) noexcept = default;
};

/// One Worker or Leader of a region (R-SIM-101, R-SIM-105): the simulation state that the figure rules read and write.
/// Its team, pool and slot are its container's (R-SIM-131); its stable id stands for the reference's serial (ADR-083
/// item 5). A dead figure's health is its corpse fade (R-UNIT-026). A Leader's order position and order route
/// (table 2.3-A) are its orderPosition and its route. Whether a Worker is in a formation is not a field: it holds a slot
/// of the Leader it follows while that Leader is in formation mode (R-UNIT-183, ADR-106).
struct Figure
{
  StableId id = 0;
  FigureKind kind = FigureKind::Worker;
  Neuron::Vec3 position;
  Neuron::Vec3 velocity;
  Neuron::Vec3 front;
  Neuron::Vec3 roamCenter; // R-UNIT-020: the spawn point, the position the spawn requested
  double roamRange = 0.0;
  double speed = 0.0;                   // R-UNIT-011: wu/s, from the stat table
  std::int32_t health = 0;              // R-UNIT-012: never above ref.unit.health_max
  std::int32_t reloadIntervalTicks = 0; // R-UNIT-011: the rate in ticks
  std::int32_t reloadTimerTicks = 0;    // R-UNIT-015: the figure fires only at 0
  bool onGround = false;
  bool enabled = true;
  bool destroyed = false; // R-SIM-113: removed at its next advance, with no corpse
  bool dead = false;
  // From state version 4 (M1 task T4):
  double radius = 0.0;   // R-SIM-308: 0 for a Worker, a sprite; a Leader's mesh radius × its radius factor
  Neuron::Vec3 waypoint; // R-UNIT-064, R-UNIT-164: where it walks; the zero vector holds a Worker still
  WorkerState workerState = WorkerState::Idle;
  LeaderState leaderState = LeaderState::Idle;
  std::int32_t retargetTimerTicks = 0; // R-UNIT-065: ticks to the Worker's next task search
  std::int32_t inWaterTicks = -1;      // R-SIM-375: negative out of water
  bool orderSet = false;               // R-UNIT-111: the pending order's set flag
  bool playerOrders = false;           // R-UNIT-089: the player-orders flag
  Neuron::Vec3 orderPosition;          // R-UNIT-111: the pending order's position
  CellRect cells;                      // R-SIM-308, DEC-650: the cells it is registered over; empty when registered nowhere
  // From state version 5 (M1 task T5):
  RouteHandle route = 0;        // R-UNIT-110: the route it follows; 0 for none
  std::int32_t routePoint = -1; // the route point it walks to; −1 before the start
  // From state version 6 (M1 task T6):
  Threat threat;                      // R-UNIT-064, R-UNIT-098
  double threatRange = 0.0;           // R-UNIT-098: the adaptive threat search radius
  bool scared = true;                 // R-UNIT-098: flee rather than fight
  bool threatSound = false;           // R-UNIT-064: the threat-sound latch
  std::int32_t grenadeTimerTicks = 0; // R-UNIT-104: ticks to the next grenade decision
  // From state version 7 (M1 task T7):
  bool promoted = false;                 // R-UNIT-065 step 1: its next advance removes it, with no Essence orb
  bool underControl = false;             // R-UNIT-136: a member of its team's crowd selection
  StableId leaderId = 0;                 // R-UNIT-064: the Leader it follows, a figure of its team; 0 for none
  std::uint32_t leaderSlot = 0;          // that Leader's slot, which resolves only while the Leader holds it
  std::int32_t followTimerTicks = 0;     // R-UNIT-074
  LeaderOrder order = LeaderOrder::None; // R-UNIT-164
  bool demoted = false;                  // R-UNIT-154: its next advance removes it, with no Essence orb
  bool noFormations = false;             // R-UNIT-170: set by the AI
  bool botTurn = false;                  // table 2.3-A: the bot target front is set; never cleared once set
  Neuron::Vec3 botFront;                 // table 2.3-A: the facing the AI gives the Leader (R-UNIT-163 step 8)
  std::uint64_t lastMarkerTick = 0;      // R-UNIT-163 step 5: the tick of its last order marker
  bool formation = false;                // R-UNIT-164: the formation flag
  bool formationAngleSet = false;        // R-UNIT-166: the formation has turned towards its waypoint
  std::vector<FormationSlot> slots;      // R-UNIT-177: in index order, gaps included
  // From state version 8 (M1 task T8):
  std::int32_t nearestNode = -1; // R-BOT-043: a Worker's nearest bot navigation node, which only recounts write; −1 for none
};

// R-UNIT-011 and R-UNIT-015 (R-SIM-109): sets the kind, the stats of the stat table, the actor-grid radius
// (R-UNIT-159), the reload interval and the initial reload timer, from one draw of purpose ReloadTimer; a Worker also
// takes its threat range and its first grenade timer (R-UNIT-060, R-UNIT-064).
void SetKind(Figure& _figure, FigureKind _kind, const TuningMvp& _tuning, TickDraws& _draws);

// R-SIM-226 step 6, R-UNIT-019, R-UNIT-059: the front of a figure spawned with _velocity.
[[nodiscard]] Neuron::Vec3 SpawnFront(const Figure& _figure, const Neuron::Vec3& _velocity, const TuningMvp& _tuning, TickDraws& _draws);

// R-UNIT-159: a Leader's actor-grid radius; a Worker's is 0.
[[nodiscard]] double GridRadiusOf(FigureKind _kind, const TuningMvp& _tuning) noexcept;

struct HealthChange
{
  bool registered = false; // R-UNIT-026: the caller knocks back only a registered hit
  bool killed = false;     // the change made the figure dead
};

// R-UNIT-026, R-UNIT-027 and R-UNIT-116 for the Worker and the Leader: an integer change of health of one damage
// kind, for a Worker that is _inFormation or not. A kill makes the figure dead with its health at the corpse fade start;
// the Essence orb of R-UNIT-038 and the presentation events are the caller's, from the result.
HealthChange ApplyHealthChange(Figure& _figure, std::int32_t _amount, DamageKind _kind, const TuningMvp& _tuning, bool _inFormation);

// The figure canonically (ADR-081 item 7): every field in declaration order, binary64 as its 64 bits.
void WriteFigure(Neuron::BitWriter& _writer, const Figure& _figure);
// The values WriteFigure wrote in a snapshot of _stateVersion; false when they do not read whole or name no kind. A
// figure of a version before 4 takes the fields that version 4 added at their spawn values, its waypoint at its
// position and its radius from its kind, registered nowhere until its region registers it; one before 5 follows no route;
// one before 6 has no threat, the full threat range, and the base of its first grenade timer; one before 7 follows no
// Leader, and a Leader of one has no order and no formation; one before 8 has no nearest node.
[[nodiscard]] bool ReadFigure(Neuron::BitReader& _reader, Figure& _figure, std::uint32_t _stateVersion, const TuningMvp& _tuning);

// The first state versions whose figures hold the fields of M1 tasks T4, T5, T6, T7 and T8. From version 7 a figure holds no
// formation flag of its own.
inline constexpr std::uint32_t FIGURE_STATE_VERSION_MOVEMENT = 4;
inline constexpr std::uint32_t FIGURE_STATE_VERSION_ROUTES = 5;
inline constexpr std::uint32_t FIGURE_STATE_VERSION_COMBAT = 6;
inline constexpr std::uint32_t FIGURE_STATE_VERSION_LEADERS = 7;
inline constexpr std::uint32_t FIGURE_STATE_VERSION_BOTS = 8;
// The most slots a saved formation holds: a bound for the reader, far above the formation size (R-UNIT-162).
inline constexpr std::uint64_t FORMATION_SLOTS_STATE_MAX = 65'535;

// R-UNIT-104: ticks from a draw of the spread in seconds, base + ⌊draw ÷ tick period⌋ (R-UNIT-060's first timer and each
// interval).
[[nodiscard]] std::int32_t TicksOf(std::int64_t _base, double _drawSeconds, double _tickPeriod) noexcept;

} // namespace Species
