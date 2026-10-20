#pragma once

#include "ActorGrid.h"
#include "Combat.h"
#include "Figure.h"
#include "TuningMvp.h"
#include "Vec3.h"

namespace Species
{

class RegionSimulation;

// The Leader's operations and order steps (R-UNIT-163 to R-UNIT-176, ADR-106). The MVP has no tutorial, no Beam Dish
// and no teleport, so the rules' tests for them find nothing.

// R-UNIT-169: the Leader walks to _target; in formation mode the formation turns first, unless _target is its position.
void AssignWaypoint(Figure& _leader, const Neuron::Vec3& _target) noexcept;

// R-UNIT-172: the order Follow in formation, facing _target when the formation starts and holding _target as its order
// position when it is already in formation mode; raises leader.order_formation.
void FormUp(RegionSimulation& _region, const ActorRef& _leader, const Neuron::Vec3& _target);

// R-UNIT-173: the order None, out of formation, its waypoint its own position.
void ClearOrders(Figure& _leader) noexcept;

// R-UNIT-170: a click at _click is a formation toggle for _leader.
[[nodiscard]] bool IsToggleClick(const Figure& _leader, const Neuron::Vec3& _click, const TuningMvp& _tuning) noexcept;

// R-UNIT-171: a formation move, entering formation, or a waypoint order with a new route to _target, straight when
// _straight; the formation move and the waypoint order clear the team's selection.
void AssignOrders(RegionSimulation& _region, const ActorRef& _leader, const Neuron::Vec3& _target, bool _straight);

// R-UNIT-168: a Leader is walking while its speed is at least ref.ctrl.leader.face_pointer_speed; R-UNIT-163 step 8
// reads the same test (ADR-106).
[[nodiscard]] bool LeaderWalking(const Figure& _leader, const TuningMvp& _tuning) noexcept;

// R-UNIT-163 step 3 and R-UNIT-168, for a Leader that is alive, on the ground and not in water: its behaviour state
// (Idle, ToWaypoint, GivingOrders), then the facing of the pointer while it is selected.
void LeaderBehavior(Figure& _leader, const FigureContext& _context);

// R-UNIT-163 steps 5, 6, 8 and 9: the order markers of a waypoint order, the formation upkeep, the bot target front, and
// the route rebuilt when the Leader has moved away from its start.
void LeaderOrderSteps(Figure& _leader, const FigureContext& _context);

} // namespace Species
