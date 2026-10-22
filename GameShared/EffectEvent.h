#pragma once

#include "StableId.h"
#include "Vec3.h"

#include <cstdint>

namespace Species
{

// ADR-036, ADR-084 item 5, ADR-105: the kinds of presentation event a region emits as its rules run. An event is never
// state and no rule reads it: presentation may drop one with no effect on the game. The values are part of the event
// stream the client receives, so they are never renumbered.
enum class EffectEventKind : std::uint8_t
{
  LaserFired = 1,     // R-UNIT-052, R-CRATE-405: a bolt and its muzzle flash; position the muzzle, vector the bolt's velocity
  LaserRicochet = 2,  // R-CRATE-441 step 4: the bolt's first ground contact; position the contact, vector the new velocity
  LaserKill = 3,      // R-CRATE-443, R-CRATE-451 step 2: a bolt or a turret round killed a Worker; position the victim
  GrenadeThrown = 4,  // R-CRATE-405: the grenade's spawn event and the throw's muzzle flash; vector the throw's velocity
  GrenadeBounced = 5, // R-CRATE-411: a thrown weapon's bounce sound
  Explosion = 6,      // R-CRATE-406 step 1: value the range r; vector.x the damage D
  FigureHurt = 7,     // R-UNIT-026: <kind>.hurt; subject the figure
  FigureDied = 8,     // R-UNIT-026: <kind>.die; subject the figure
  WorkerIgnited = 9,  // R-UNIT-121: the Worker is set on fire
  ThreatBark = 10,    // R-UNIT-098 step 9: a new threat; value 1 for the flee bark, 0 for the attack bark
  // From M1 task T7 (ADR-106); the client shows a selection or order event to the commanding team only.
  WorkerPromoted = 11,         // R-UNIT-153: the control flash and leader.spawn; subject the new Leader
  LeaderSelected = 12,         // R-UNIT-193: leader.selected; subject the Leader
  LeaderOrderWaypoint = 13,    // R-UNIT-171 step 3: leader.order_waypoint; position the order position
  LeaderOrderFormation = 14,   // R-UNIT-172: leader.order_formation and the preview; position the target
  WorkersSelected = 15,        // R-UNIT-136: worker.selected; value the number of Workers selected
  WorkerOrdered = 16,          // R-UNIT-096 step 3: worker.ordered, a Worker that took a Leader's order; subject the Worker
  WorkersOrderedByPlayer = 17, // R-UNIT-140 step 5: worker.ordered_by_player; position the target
  // From M2 task T5 (ADR-131):
  LaserHitStructure = 18,  // R-CRATE-441 step 6: the spark and laser_bolt.hit_structure; position where the bolt ended
  StructureCompleted = 19, // MVP-033, MVP-083: a construction site is built; subject the structure (ADR-132)
  // From M2 task T5's turret (ADR-133):
  TurretTargetSighted = 20, // R-BLDG-411: turret.target_sighted; subject the turret, position its new target's
  TurretFired = 21,         // R-BLDG-418, R-BLDG-420: a round, its muzzle flash and casing; position the muzzle, vector the
                            // round's velocity, value the barrel's index
  TurretRoundHit = 22,      // R-CRATE-451 steps 4 and 5: the round's particles; position the hit, value 1 on a structure
  TurretDestroyed = 23      // R-BLDG-423: the head and barrels shatter; subject the turret
};

/// One presentation event, in the order the region's rules emitted it within its tick (ADR-036's stable identity is the
/// tick, the region and this order).
struct EffectEvent
{
  EffectEventKind kind = EffectEventKind::LaserFired;
  std::uint8_t team = 0;
  StableId subject = 0;
  Neuron::Vec3 position;
  Neuron::Vec3 vector;
  double value = 0.0;
};

} // namespace Species
