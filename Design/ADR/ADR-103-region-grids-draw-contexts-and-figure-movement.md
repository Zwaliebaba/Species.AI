---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T4)
---

# Region grids, draw contexts and figure movement

## Context and Problem Statement

Task T4 of the [M1 plan](../implementation/m1_plan.md) gives the region its spatial structures and moves its figures by the reference crowd rules (MVP-040). It covers:
- the actor grid, its enemy cache and the obstacle grid (R-SIM-300 to R-SIM-329);
- integration, falling, water and the obstacle push (R-SIM-360 to R-SIM-389);
- the Worker's and the Leader's movement, with the slope factor (R-UNIT-040 to R-UNIT-042, R-UNIT-105 to R-UNIT-115, R-UNIT-156 to R-UNIT-163).

The reference describes these structures for a whole match world, keyed by a map's extent, with handles that carry a team and a slot. These decisions are open:
- what extent the structures cover in a world of regions;
- what a handle is;
- which of the grids' contents are state;
- how the rules draw when one rule draws for the same figure more than once in a tick;
- the readings that the reference's prose leaves to the implementation.

## Decision Drivers

* The approximate queries are tuned gameplay and stay as the reference states them (DEC-094's stated behaviour).
* A restored world gives the hashes of a world never stopped (ADR-085, ADR-099), so whatever a later answer depends on is state.
* A region reads no other owner's state (ADR-085 item 2).
* Every tuning value lives in data (MVP-099), and every draw is keyed (ADR-082, ADR-097).

## Considered Options

* 1 — the region's own grids over its square; the grids' history in the region's state; one draw context per purpose, figure and tick
* 2 — one grid over the world's whole extent of 71,680 wu, shared by the regions
* 3 — grids rebuilt from the figures after a restore, with no history saved

## Decision Outcome

Chosen option: "1".
- Option 2 makes every region job write a structure that the others read, which ADR-085 item 2 forbids. A layer would also hold 5.0 million cells for 25 thousand cells of land.
- Option 3 loses the capacities that the presence queries answer from (R-SIM-312 item 3) and the cache's entries (R-SIM-316). A restored world would then answer otherwise than the world it was.

* **Extent.** Each region owns its actor grid, its cache and its obstacle grid over its square: the MVP region's cell of `mvp.world.region_cell` at side `wld.region.side` (ADR-102). For the rules of R-SIM-366 and R-SIM-368, the square is the map:
  - the world clamp keeps x and z inside it;
  - the spawn-position search's "on the map" means inside it, and its fallback clamps the edge margin inside its edges.
  - Cell indices count from the square's origin, as the terrain's do (TER-002 amendment). The actor grid has trunc(5,120 ÷ 32) + 1 = 161 cells per axis, and the obstacle grid ceil(5,120 ÷ 32) = 160.
* **The actor grid** (`ActorGrid`).
  - An entry (`ActorRef`) is the figure's stable id, its slot among its team's figures and its team. The stable id is the serial that orders a cell (R-SIM-310).
  - A layer per team slot is made at the team's first registration.
  - A cell keeps its entries sorted by stable id, and the capacity that the reference's slot array would have: 0, then 2, then doubling, never shrinking. The slot array decides no order (R-SIM-307), so only its capacity is kept.
  - Count queries count the distinct entries of the covered cells. Presence queries test the capacity (R-SIM-312).
  - A figure records the cells it registered over (DEC-650). After each advance that keeps it, its region updates the registration when the cells change. A figure that leaves or is disabled leaves the grid (R-SIM-114).
* **The cache** (`ActorGridCache`) is the reference's: a hit returns the nearest entry's unfiltered list, and an entry older than `ref.sim.grid_cache.max_age` goes in slice `ref.sim.grid_cache.evict_slice`, after the teams.
* **The obstacle grid** (`ObstacleGrid`) is derived from the structures, so it is not state.
  - The MVP builds no structure before M2, so a region's obstacle grid is empty at M1. A structure's hit volume is a sphere (`ObstacleShape`) until the structures bring their shapes.
  - The pushes of R-SIM-380 and R-UNIT-114 run against it. Their tests place a sphere in it.
  - No Barrier Emitter, wall or Beam Dish exists in the MVP, so their branches are not built.
* **State.** A region writes, after its teams, its actor grid's capacities and then its cache.
  - The capacities are, per layer, the cells whose capacity is not the initial one, in cell order. The cache writes every entry in order.
  - The entries themselves are not written: each figure's recorded cells register it again.
  - `WORLD_STATE_VERSION` becomes 4, with the figure's new fields: radius, waypoint, Worker and Leader state, retarget timer, in-water count, pending order and recorded cells.
  - A region of an older version reads with no grid history, and its figures register at their positions, Idle in place.
  - The golden run of DBG-147 changes once, by the empty grid and cache of each region. An independent model of the layout computes the same values.
* **Draw contexts** (`TickDraws`, replacing T2's `DrawScope`). A region keeps one context per purpose and figure for the tick, made at its first use. Every later use continues it, so two waypoint orders for one Worker in one tick draw the next values rather than the same ones. ADR-082 item 3's rule that a second context is a defect still holds, because no second context is ever made.
  - The contexts belong to the tick being stepped. A spawn or an order outside a step takes the next tick's.
  - A snapshot holds none, because it is taken between ticks.
  - The new purposes are numbered after T2's: `SpawnSearch`, `RetargetTimer`, `Wander`, `OrderDisc`, and one purpose per call site of the shore search (`ShoreMove`, `ShoreFall`, `ShoreWander`, `ShoreOrder`), so that a context never spans two rules.
* **Readings of the reference's prose**, kept as rules here:
  - The slope factor's next height (R-UNIT-042) is the terrain's at the position the step would reach before the slope factor. For a Worker that is along its blended direction; for a Leader, along its blended front.
  - A walking Leader blends its front towards its waypoint by `ref.unit.leader.turn_walk`, renormalises it, and steps along it, through the push of R-SIM-380 as a sphere (R-UNIT-160). It has arrived within `ref.unit.leader.arrival_distance`, that distance included.
  - An angle θ of a spawn search, shore search, random position or order disc gives the direction (cos θ, 0, sin θ).
  - The shore search's point keeps the candidate's height. The next step's snap puts a figure on the ground there.
  - Every push loop is bounded at ceil(reach ÷ step) + 2 passes, a pass more than the geometry needs, and keeps its last candidate if it ever reaches the bound (DEC-014, R-UNIT-115).
  - Normalisation and set-length follow DEC-241 item 6: (0, 0, 1) below a squared length of 10⁻¹⁰, and the x axis below a magnitude of 10⁻⁶. They are `Neuron::Normalized` and `Neuron::WithLength`.
  - A spawned figure keeps the requested height, so it falls and lands in its first advance.
* **What the Worker and the Leader do at T4.**
  - A Worker runs R-UNIT-065 in its order: its base advance, its retarget timer and task search, Idle and OnWaypointOrder, its airborne step, drowning and its test, and its corpse's slide.
    - In the task search, Idle ends in the random-position search; the threat and Leader searches come with T6 and T7.
    - The idle timer, which never expires in a match, is not kept.
  - A Leader runs R-UNIT-163 steps 1, 3 and 4. It runs the swimming step twice per tick.
  - Waypoint orders (R-UNIT-111) and Leader waypoints exist as functions for the commands of T7. Routes come with T5.
* **Values in data.** The `ref.*` keys these rules read are in the record under their reference names.
  - `ref.unit.worker.retarget_ticks` (11 to 20 ticks) is an object of its base, 11 ticks, and its spread, 1 second, as ADR-101 made the default front an object.
  - The Leader's mesh radius is `mvp.unit.leader_mesh_radius`, 6.7252 wu. It is the bounding radius that `LeaderFigure.cmo` records in its mesh extents, which matches its vertices' farthest distance from the extents' centre. The radius is read with a Python reader of the CMO layout, and it stands in the record until T11's cooked mesh gives it.

### Consequences

* Good, because the queries' softness, their order and their history reproduce the reference, and a restored region answers as the saved one.
* Good, because a rule that draws twice in a tick can never repeat a value, and a new rule needs no thought about which context to share.
* Bad, because the hash and the snapshot grow by the grid's history: a few bytes per cell ever occupied, and the cache's lists. A development build hashes them every tick, so T10 measures them.
* Bad, because every draw looks its context up in a map, which T10 measures against ADR-080's per-figure budget.
* Bad, because the obstacle grid and the pushes are tested only against test spheres until M2's structures arrive.
* It forecloses: one grid shared by the regions; a grid rebuilt from positions after a restore; a draw context made per call.

### Confirmation

* `GameLogicTests` checks:
  - AC-SIM-05-01 to AC-SIM-05-05, the exact query's strict range and order with a figure in four cells reported once, and the grid's history read back;
  - the obstacle grid's cells, its nearest-cell look-up and its outside cell, with its duplicate per off-grid cell;
  - AC-SIM-05-13 to AC-SIM-05-15 and AC-SIM-05-18 on the region's square, AC-UNIT-L11, and the Worker's shore search from 40 wu;
  - the slope factor's clamps, a Worker's walk, slow-down, snap and arrival, a Leader's walk;
  - 200 Workers walking from site 0 of seed 1's valley to its centre, all arriving alive and dry within 2,000 ticks (781 in the local run), with every figure registered over its cells;
  - a region saved at version 3 restoring with its figure Idle and registered where it stood, and the golden run's new values.
* T10 confirms the cost of the grids' upkeep and of the draw contexts.

## Pros and Cons of the Options

### 1 — region grids, history as state, a context per purpose, figure and tick

* Good, because each region job keeps its own structures, as ADR-085 requires.
* Bad, because a figure that crosses into another region has to be handed over with its registration, when regions have neighbours.

### 2 — one world grid

* Good, because a query near a region's edge sees the neighbour's figures.
* Bad, because the regions would share a written structure.

### 3 — no history

* Good, because the state is smaller.
* Bad, because a restore changes the gameplay's answers.

## More Information

* Related: [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-085](ADR-085-region-partitioned-world-simulation.md), [ADR-097](ADR-097-keyed-draws-and-state-hash.md), [ADR-099](ADR-099-world-snapshots.md), [ADR-101](ADR-101-region-teams-figures-and-update-slices.md), [ADR-102](ADR-102-region-terrain-and-the-terrain-check-at-connection.md).
* The rules: R-SIM-300 to R-SIM-389 ([space, routing and physics](../reference/12_simulation_model/05_space_routing_and_physics.md)), R-UNIT-040 to R-UNIT-042 ([units](../reference/02_units/README.md)), R-UNIT-063 to R-UNIT-115 ([Worker](../reference/02_units/worker.md)), R-UNIT-156 to R-UNIT-163 ([Leader and formations](../reference/02_units/leader_and_formations.md)).
* The code is in NeuronCore (`Vec3`'s `Normalized` and `WithLength`) and GameLogic (`ActorGrid`, `ActorGridCache`, `ObstacleGrid`, `TickDraws`, `Movement`, `FigureAdvance`, `RegionSimulation`).
* **Amendment 2026-10-20 (M1 task T5, [ADR-104](ADR-104-navigation-nodes-land-route-graph-and-routes.md)).** `MovementContext` carries the region's route store. A Worker on a waypoint order follows its route before its movement step (R-UNIT-110), drawing its jitter from the purpose `RouteJitter` and its shore search from `ShoreRoute`, and a waypoint order takes a route handle (R-UNIT-111).
