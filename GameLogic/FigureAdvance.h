#pragma once

#include "Combat.h"
#include "Figure.h"
#include "Movement.h"
#include "Vec3.h"

namespace Species
{

// R-SIM-111 with the region's movement context: steps 1 to 3, the water step of a figure that swims (step 5, never a
// Worker's: R-SIM-377) and the world clamp (step 6); true when the figure asks to be removed. Route following, step 8,
// is the Worker's own (R-UNIT-110); the flicker of step 4 is presentation, and no group exists for step 7.
[[nodiscard]] bool BaseAdvance(Figure& _figure, const FigureContext& _context);

// R-UNIT-065 for what the MVP builds: the removal of a promoted Worker; the base advance; on the ground, alive and not
// burning, the retarget timer, the task search (R-UNIT-087: the threat search, the Leader search of R-UNIT-096 and the
// random-position search) and the state's behaviour (Idle, OnWaypointOrder, Fighting, FollowingLeader); the airborne
// step; the burning and drowning steps (R-UNIT-079, R-UNIT-080) and the drowning test; the corpse's slide. True when it
// asks to be removed.
[[nodiscard]] bool WorkerAdvance(Figure& _figure, const FigureContext& _context);

// R-UNIT-163: the removal of a demoted Leader; the airborne step, the base advance, the second water step (R-UNIT-161)
// and the snap to the terrain; PrepareWaypoint for the order None; the behaviour (R-UNIT-165 to R-UNIT-168); the corpse's
// sinking; the order markers, the formation upkeep, the bot target front and the rebuilt route. True when it asks to be
// removed.
[[nodiscard]] bool LeaderAdvance(Figure& _figure, const FigureContext& _context);

// The advance of the figure's kind.
[[nodiscard]] bool AdvanceFigure(Figure& _figure, const FigureContext& _context);

// R-UNIT-111 for what the MVP builds so far: a waypoint order to _target, its disc point passed through the Worker's
// push with kill clear (R-UNIT-115), and the route to follow, before its start; the MVP has no Transport, carryable or
// Beam Dish.
void GiveWaypointOrder(Figure& _worker, const Neuron::Vec3& _target, RouteHandle _route, const MovementContext& _context);

// R-UNIT-110: the Worker's route following in OnWaypointOrder and FollowingLeader; the first step jumps to route point 1, and each later
// one comes within the reach radius, each new waypoint jittered and pushed with kill clear; past the end the route is
// cleared and the waypoint keeps its last point. A route the store does not hold is cleared (R-SIM-354).
void FollowRoute(Figure& _worker, const MovementContext& _context);

} // namespace Species
