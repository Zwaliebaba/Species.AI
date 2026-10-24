---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead
---

# A drop-off point pushed clear of its structure

## Context and Problem Statement

A gatherer puts its load down at the nearest drop-off point (MVP-031, [ADR-135](ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md)): the Wellspring's `door` socket or a storehouse's `entrance` socket, set on the terrain. It deposits only once it has arrived there (R-UNIT-105). On its way, the Worker's structure push (R-UNIT-114) moves every step that comes within `ref.unit.worker.structure_clear_radius` (2 wu) of a structure's collision triangles away from that structure.

The owner's meshes of 2026-10-23 moved the storehouse's `entrance` to 16 wu in front of its centre. Its collision asset now covers the loading platform and its step with one box up to 15 wu and 4.8 wu high. On level ground the socket is 1 wu from that box, inside the Worker's clearance. A carrier on its last metres is pushed back on each step and never comes within `ref.unit.worker.snap_distance` (1 wu) of the point, so it never arrives. In the owner's M3 build every carrier sent to a new storehouse stood at its door with its load and did not move again.

The growth scene did not show it. Seed 1's scripted steward put its four storehouses on slopes that lower their doors 1.75 to 3.39 wu below their bases, which left the sockets 2.05 to 3.55 wu clear. The tests' records have no collision asset.

A port has the guarantee that the drop-off point lacks: R-BLDG-020 pushes each port clear of its structure by `ref.bldg.port.clearance_wu` when the structure is made ([ADR-130](ADR-130-settlements-structures-and-resource-sites-at-m2.md), [ADR-131](ADR-131-structure-step-collision-shapes-and-ports-at-m2.md)).

## Decision Drivers

* A carrier always reaches its drop-off, whatever the mesh and whatever the ground under the structure.
* The meshes are the owner's: no rule decides where they put a socket.
* The owner's saved worlds continue, and their waiting carriers walk on, with no change to the state.
* A drop point that is already clear does not move, so the tuned economy stays.

## Considered Options

1. **The drop point pushed clear in the simulation**, as a port is.
2. **The storehouse's `entrance` moved out** in the mesh kit's recipe, from 16 to 18 wu.
3. **Arrival within a radius:** a carrier deposits once it is within some distance of its drop point.

## Decision Outcome

Chosen option: 1.
- Option 2 mends this mesh only, and the next change of the owner's art can bring the fault back.
- Option 3 changes the Worker's arrival (R-UNIT-105) for one task, and needs a new key large enough for any mesh.

What is built:

* **The rule.** `DropPointOf` (`GameLogic/Structure`) takes the door or entrance socket at rest, set on the terrain. Then, as R-BLDG-020 pushes a port, it steps the point `ref.bldg.structure.push_step_wu` (2 wu) at a time directly away from the structure's position.
  - It steps while a sphere of `structure_clear_radius` + `snap_distance` (3 wu) at the point hits the structure's shape, tested on the terrain where the Worker stands.
  - A point clear by that sum leaves every place within the snap distance of it clear of the push, so the Worker's last step is never turned back.
  - The steps are bounded as a port's are.
  - A kind with neither socket keeps its position on the terrain, as before.
* **The cache.** The region derives each structure's drop point with its obstacle grid, in `RebuildObstacles`, by structure slot (`RegionSimulation::DropPointAt`). It is rebuilt whenever a structure is added or removed and when a region is read, so neither the hash nor the snapshot holds it.
* **The carry.** `StartCarrying` and `CarryingStep` (`GameLogic/SettlementTasks`) read the drop point by slot. A carrier on its last leg takes it again each tick, so one walking to an old point turns to the new one.

### Consequences

* Good, because a carrier reaches every drop-off, and the owner's world recovers when it is next loaded.
* Good, because a carrier's search for its drop-off no longer reads each drop-off's record. Each point is a look-up.
* Bad, because the point can stand a few wu in front of the socket the art marks: 2 wu on level ground for the owner's storehouse.
* Bad, because the drop points of seed 1's growth scene that stood within 3 wu move out 2 wu (two of its four storehouses), so the scene's figures change a little from this build on.
* It forecloses nothing: a mesh can still place its sockets anywhere.

### Confirmation

* `GameLogicTests/StructureTests`, `Mvp031ADropPointIsPushedClearOfItsStructureAndAWorkerReachesIt`, uses the owner's platform box with the entrance 1 wu in front, on level ground:
  - a Worker sent to the socket from 32 wu does not arrive in 600 ticks;
  - the drop point is clear by 3 wu;
  - a Worker sent to the drop point arrives.
* A probe stepped the growth scene's world of seed 1 for 2.6 hours on the cooked meshes. Every drop point was clear by 3.05 wu or more. Its storehouses took 262 to 575 deposits each, and no carrier stood near a drop point without depositing for 60 s.
* The owner's run: carriers deposit at a storehouse built on level ground.

## Pros and Cons of the Options

### 1 — pushed clear in the simulation

* Good, because it holds for any mesh and any ground, by the rule ports already follow.
* Bad, because the point the Workers use is not exactly the socket the art marks.

### 2 — the socket moved

* Good, because the art and the point agree.
* Bad, because it mends one mesh, and only on level ground.

### 3 — arrival within a radius

* Good, because nothing moves.
* Bad, because it bends the Worker's arrival for one task and needs a radius that suits every mesh.

## More Information

* MVP-031, MVP-034; R-BLDG-020, R-UNIT-105, R-UNIT-114.
* Amends [ADR-135](ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md) (the drop-off point).
* The code: `GameLogic/Structure`, `GameLogic/RegionSimulation`, `GameLogic/SettlementTasks`, `GameLogicTests/StructureTests`.
