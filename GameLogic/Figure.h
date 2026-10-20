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

// R-UNIT-067: the Worker's behaviour states that the MVP builds so far; the values are state, never renumbered.
enum class WorkerState : std::uint8_t
{
  Idle,
  OnWaypointOrder,
  Drowning
};

// R-UNIT-164: the Leader's behaviour states that the MVP builds so far; GivingOrders comes with its orders (M1 plan T7).
enum class LeaderState : std::uint8_t
{
  Idle,
  ToWaypoint
};

/// One Worker or Leader of a region (R-SIM-101, R-SIM-105): the simulation state that the figure rules read and write.
/// Its team, pool and slot are its container's (R-SIM-131); its stable id stands for the reference's serial (ADR-083
/// item 5). A dead figure's health is its corpse fade (R-UNIT-026).
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
  bool inFormation = false; // R-UNIT-116: a Worker in a Leader's formation
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
};

// R-UNIT-011 and R-UNIT-015 (R-SIM-109): sets the kind, the stats of the stat table, the actor-grid radius
// (R-UNIT-159), the reload interval and the initial reload timer, from one draw of purpose ReloadTimer.
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
// kind. A kill makes the figure dead with its health at the corpse fade start; the Essence orb of R-UNIT-038 and the
// presentation events are the caller's, from the result.
HealthChange ApplyHealthChange(Figure& _figure, std::int32_t _amount, DamageKind _kind, const TuningMvp& _tuning);

// The figure canonically (ADR-081 item 7): every field in declaration order, binary64 as its 64 bits.
void WriteFigure(Neuron::BitWriter& _writer, const Figure& _figure);
// The values WriteFigure wrote in a snapshot of _stateVersion; false when they do not read whole or name no kind. A
// figure of a version before 4 takes the fields that version 4 added at their spawn values, its waypoint at its
// position and its radius from its kind, registered nowhere until its region registers it; one before 5 follows no route.
[[nodiscard]] bool ReadFigure(Neuron::BitReader& _reader, Figure& _figure, std::uint32_t _stateVersion, const TuningMvp& _tuning);

// The first state versions whose figures hold the fields of M1 tasks T4 and T5.
inline constexpr std::uint32_t FIGURE_STATE_VERSION_MOVEMENT = 4;
inline constexpr std::uint32_t FIGURE_STATE_VERSION_ROUTES = 5;

} // namespace Species
