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

// R-UNIT-098 for what the MVP builds: the range relaxation, the projectile threats (grenades of any team), the bot
// census shortcut, the cached enemy query with its filter, centred ahead of a formation member, and the found or cleared
// threat. Steps 2, 4 and 5 have nothing to find in the MVP. True when the Worker has a threat.
bool ThreatSearch(Figure& _worker, const FigureContext& _context);

// R-UNIT-101 to R-UNIT-104: one tick of Fighting: the threat resolved, or the fight's end back to the Worker's Leader;
// the movement, a formation member's to its slot; the shot, a formation member's by the volley rhythm; and the grenade
// decision outside a formation.
void FightingStep(Figure& _worker, const FigureContext& _context);

// R-UNIT-079: the burning step, on the ground: the panicked wander, run at full speed, and the damage roll.
void BurningStep(Figure& _worker, const FigureContext& _context);

// R-UNIT-052: the base attack, a laser bolt at _target, when the reload allows it.
void FireAtPoint(Figure& _figure, const Neuron::Vec3& _target, const FigureContext& _context);

} // namespace Species
