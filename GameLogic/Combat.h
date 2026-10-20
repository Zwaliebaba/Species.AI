#pragma once

#include "ActorGrid.h"
#include "Figure.h"
#include "Movement.h"
#include "Vec3.h"

namespace Species
{

class RegionSimulation;

/// What a figure's advance reads and writes beyond its movement (ADR-105): its region, for its weapons, its damage, its
/// queries and its events, and its own actor-grid entry.
struct FigureContext
{
  RegionSimulation* region = nullptr;
  const MovementContext* movement = nullptr;
  ActorRef self;
};

// R-UNIT-098 for what the MVP builds: the range relaxation, the projectile threats (grenades of any team), the cached
// enemy query with its filter, and the found or cleared threat. Steps 2, 4 and 5 have nothing to find in the MVP, and the
// bot census shortcut of step 6 arrives with the bot's node census (M1 plan T8). True when the Worker has a threat.
bool ThreatSearch(Figure& _worker, const FigureContext& _context);

// R-UNIT-101 to R-UNIT-104: one tick of Fighting, outside a formation: the threat resolved, the movement, the shot and the
// grenade decision.
void FightingStep(Figure& _worker, const FigureContext& _context);

// R-UNIT-079: the burning step, on the ground: the panicked wander, run at full speed, and the damage roll.
void BurningStep(Figure& _worker, const FigureContext& _context);

// R-UNIT-052: the base attack, a laser bolt at _target, when the reload allows it.
void FireAtPoint(Figure& _figure, const Neuron::Vec3& _target, const FigureContext& _context);

} // namespace Species
