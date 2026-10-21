---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T5)
---

# Navigation nodes, the land route graph and routes

## Context and Problem Statement

Task T5 of the [M1 plan](../implementation/m1_plan.md) gives the region its routes. It covers:
- the navigation nodes of MVP-017, which the generator places in place of a map's hand-placed nodes (O70);
- the path tests, the land route graph, its next-hop tables and route building (R-SIM-330 to R-SIM-352);
- the route store (R-SIM-357, R-SIM-359, DEC-651) and the Worker's route following (R-UNIT-110);
- check 8 of WLD-163, which measures the routes against the travel network of WLD-100.

These decisions are open:
- where the nodes stand and in what order, since MVP-017 states what they are and not their positions;
- which library builds the graph, since the terrain check at connection (ADR-102) runs on both the client and the server;
- how to build the next-hop tables within the time a region's generation allows. R-SIM-341 states the propagation as N passes over every entry of every next hop. Over seeds 1 to 3, at 193 to 199 nodes, those loops took 2.9 to 3.5 s per valley. That was measured in the local build described under Confirmation. A valley may be built up to 64 times before one attempt passes: 8 attempts on each of 8 seeds (WLD-163, MVP-011, ADR-102);
- what a route handle is, and what a region keeps of its routes;
- what check 8 measures.

## Decision Drivers

* The routes and their ties are tuned gameplay and stay as the reference states them, equal-cost choices included (R-SIM-341).
* The client and the server reach the same terrain attempt from the same seed (ADR-102), so whatever a check reads is built by both.
* A restored world gives the hashes of a world never stopped (ADR-085, ADR-099), so what a figure follows is state.
* Every tuning value lives in data (MVP-099), and every draw is keyed (ADR-082, ADR-097).

## Considered Options

* 1 — the nodes on a lattice and at the region's named places; the graph in GameShared; the reference's propagation with the offers that cannot be accepted left out; a route store per region kept as state
* 2 — the propagation as R-SIM-341 writes it, with no pruning
* 3 — a shortest-path algorithm in place of the propagation, such as Dijkstra's from each node
* 4 — the graph in GameLogic only, with check 8 run by the server alone

## Decision Outcome

Chosen option: "1".
- Option 2 spends seconds per attempt on the server and again on the client at connection.
- Option 3 gives the same totals but not the same next hops. Among equal-cost paths it keeps the one its own order finds, not the one R-SIM-341's order finds first, so a level valley would route otherwise than the reference.
- Option 4 lets a client accept an attempt that the server refused, or refuse the one the server kept, which is the divergence ADR-102's check at connection exists to catch.

* **The nodes** (`PlaceNavigationNodes`, MVP-017). Every node's check-cliffs flag is the reference default, true. The nodes are numbered in this order, with no draw:
  - **The lattice.** The cell centres of a lattice of spacing `mvp.nav.node_spacing` (320 wu) over the region's square, z outer and x inner: 16 × 16 points at a side of 5,120 wu. A point is a node when its terrain is above sea level and its normal's y component is at least `ref.sim.walk.min_normal_y`. That is the slope test that R-SIM-335 applies to a sample, since a single point has no segment to walk.
  - **The four site centres**, in local order.
  - **The central resource site.**
  - **The middle of each valley border area.** The border area of two valley neighbours is the lens of their largest territories, of radius `wld.territory.max_radius`, on the line between their centres (WLD-062). It spans d − R to R from the first site and is split at d ÷ 2. The node of each half stands at the middle of the half's extent on that line, the nearer site's half first. The pairs come in the order 0–1, 2–3, 0–2, 1–3.
  - Over seeds 1 to 100 a valley has 188 to 206 nodes.
* **The graph** (`LandRouteGraph`, in GameShared) is built with the terrain, after the region's samples and before its checks. Both the client and the server build it, since check 8 reads it. It is not state, because the seed rebuilds it.
  - The MVP has no wall and no Barrier Emitter, so the path tests read the terrain alone and the emitter report is not built. R-SIM-343's rebuild waits for M2's structures.
  - The edges and the symmetric memo are R-SIM-339 and R-SIM-340 as written: every ordered pair within `ref.sim.route.link_range`, strictly below it, i = j included.
  - The nearest node (R-SIM-346) sorts the nodes by 3-D squared distance, then index, and takes the first walkable one, or the first of all when none is walkable. That is the result of the reference's two scans, but it tests only as many paths as it must.
  - The sea route graph of R-SIM-342 serves the Transport, which the MVP does not build.
* **The propagation** runs R-SIM-341's loops in the reference's order, live tables included. Each node keeps a record of the table slots it has changed, in the order it changed them. A node i that reaches hop h offers only the slots that h recorded since i last scanned h. Three kinds of offer are left out, each of which the reference makes and rejects:
  - **The self hop**, h = i. It offers i's own totals plus walkCost(i, i), which is 0, so every offer ties.
  - **A second scan of the same hop** within one visit of i. h's table cannot change while i is visited, and i's totals only fall, so every offer ties or loses.
  - **An entry of h unchanged since i's last scan of h.** i was offered it then, and its total there has only fallen since.
  
  A rejected offer changes nothing, so leaving it out changes nothing. The remaining offers are the reference's accepted offers, in an order that differs only where the order cannot matter:
  - The values of i's entries are the same, because each target is offered at most once per scan.
  - Table order is set by appends. An entry that is new to i cannot have existed in h at i's last scan of h, or i would hold it. Its first appearance in the record is therefore its append, and appends are recorded in slot order.
  
  The passes stop at the first that accepts nothing, since every later pass would make the same rejected offers. None of this depends on the sign of a cost, so it holds even where `WalkCost` gives an edge −1 (R-SIM-337). No node of seeds 1 to 100 has a negative total.
* **Building a route** (`LandRouteGraph::BuildRoute`) follows R-SIM-345 to R-SIM-351 step by step: the corridor test, the node chain, the corner smoothing, the merge, the snap, and then the override, applied only after a corridor test (DEC-263's stated behaviour). It draws nothing.
* **The route store** (`RouteStore`, in GameLogic) belongs to a region and holds the routes the region's simulation has built and something still holds (DEC-651, resolved by O71 as its option B; R-SIM-357 amendment).
  - A handle numbers the routes from 1 in the order they were built, and 0 is none. The store keeps the last handle it gave, and a handle is never given twice, so a freed handle never resolves again (R-SIM-359). A counter that never goes back is DEC-028's generational handle with nothing to reuse, so it carries no generation of its own.
  - At the end of each step, after the last slice, the region frees every route that no figure holds (`FreeUnheldRoutes`). The figures are the only holders the MVP builds so far; T7 adds the Leaders' orders, and every later holder joins the same sweep. No rule reads a route that nothing holds (R-SIM-357), so freeing changes no simulation result, and a snapshot, which is taken between ticks, holds only routes in use. The presentation that fades a drawn route keeps its own copy of the points (RND-352).
  - A route keeps its points only. No rule of the MVP reads R-SIM-334's status.
  - At M1 nothing builds a route except tests; the commands of T7 build them (R-SIM-353).
* **Route following** (`FollowRoute`). A Worker on a waypoint order follows its route before its movement step (R-UNIT-110). The FollowingLeader state comes with T7.
  - With no point yet, the Worker goes to point 1, since point 0 is the start. After that it moves to the next point once its horizontal distance from its waypoint is at most `ref.unit.worker.route_reach_radius`, that distance included, as ADR-103 reads "within".
  - Each new waypoint draws its jitter's radius and then its angle from the purpose `RouteJitter`, keyed by the Worker and the tick. The waypoint keeps the route point's height and passes through the Worker's push, whose shore search draws from `ShoreRoute`. With no Barrier Emitter, the kill is clear.
  - Past the last point, the route and its point index are cleared and the waypoint stays. A handle that the store does not hold is cleared too (R-SIM-354).
  - A waypoint order takes a route handle, possibly 0, and starts it before its first point (R-UNIT-111).
* **State.** A region writes its route store after its cache: the last handle, the count, then each route's handle, point count and points as binary64 (ADR-081 item 7). A store whose handles do not ascend, or pass the last handle, is refused. Each figure writes its route handle and point index after T4's fields.
  - `WORLD_STATE_VERSION` becomes 5. A region of an older version reads with no routes, and its figures with none.
  - A region refuses a snapshot in which a figure holds a handle that the region's store does not, or a route point outside its route, as it refuses any damaged snapshot.
  - The golden run of DBG-147 changes once, by each region's empty route store and its last handle, 0. An independent model of the layout computes the same values.
* **Check 8** runs in `CheckRegionTerrain` with checks 1 and 3 to 6 (ADR-102). The edges of WLD-100 inside one region are the four valley-neighbour pairs, in the order above, and each site with the central resource site. The MVP's single region has no crossing.
  - Each edge's route is built once, from its first end to its second, since routes depend on direction (R-SIM-336).
  - Its length is the 3-D length of its polyline, which must not exceed the horizontal distance between the ends × `wld.travel.valley_path_factor`.
  - The client runs the check too, because it builds the same graph.
* **Values in data.** The record gains:
  - `mvp.nav.node_spacing`;
  - the `ref.sim.walk.*` and `ref.sim.route.*` keys these rules read, under their reference names;
  - `ref.unit.worker.route_reach_radius` and `ref.unit.worker.route_jitter_radius`;
  - `wld.territory.max_radius` and `wld.travel.valley_path_factor`.

### Consequences

* Good, because the tables, their order and their ties are the reference's, at a cost a region's generation and a client's connection can afford.
* Good, because the client and the server agree on every check, routes included, from the seed alone.
* Good, because a route a figure follows survives a restore, and a damaged reference to one is refused rather than followed.
* Bad, because the propagation's equivalence rests on an argument and on tests, not on the reference's literal text. A change to the offer rule of R-SIM-341, such as accepting ties, invalidates the argument and has to rerun the tests.
* Bad, because the graph costs about 10 ms of every terrain attempt on both sides, and N² slots and up to N² entries in memory. At the largest valley of seeds 1 to 100, with 206 nodes, that is 42,436 slots of 4 bytes and 42,436 entries of 16 bytes: about 0.85 MB.
* Bad, because every step sweeps the region's figures for the routes they hold, a cost of the figure count that T10 measures against ADR-080's per-figure budget.
* It forecloses: a shortest-path algorithm with its own tie order; a graph that only the server builds; a route freed while a figure could still hold it; a route kept after nothing holds it.

### Confirmation

* The local build is GCC 13 at -O1 in a 4-core 2.1 GHz Xeon cloud container. It builds the libraries that do not reach Windows, and sweep programs over `BuildRegionTerrain`, `CheckRegionTerrain` and `LandRouteGraph::Build`. It is not the reference test machine (O58). Its measurements:
  - Every one of seeds 1 to 100 passes checks 1, 3 to 6 and 8 at its first attempt. The slowest attempt takes 250 ms with its graph.
  - The graph alone takes 10.4 ms on average and 17.2 ms at worst.
  - The longest route measures 1.0017 times its edge's horizontal distance, against the factor's 1.15.
  - On the full node sets of seeds 1 to 3, the pruned propagation gives tables identical, entry by entry, to the literal loops: 8.6 to 12.0 ms against 2.9 to 3.5 s.
* `GameSharedTests` checks:
  - AC-SIM-05-06 to AC-SIM-05-09;
  - the pruned tables against the literal loops, entry by entry, on a level lattice, where equal-cost paths are everywhere, and on 40 nodes of seed 1;
  - a route round a pond through the nodes, with every leg clear and dry and the corners smoothed;
  - the nodes' order and places;
  - a route between every pair of the five centres on seeds 1 to 5, on the terrain, dry and the same on a second build;
  - check 8's failing fixture.
* `GameLogicTests` checks:
  - a Worker's steps along a route with its keyed jitter;
  - handles counting from 1, a route that no figure holds freed at the end of the step, a later route taking the next handle and never a freed one, a region's routes and its figures' routes read back to the same state, and a damaged route reference refused;
  - 200 Workers following one route with a corner from site 3 to site 0 of seed 1's valley, all arriving alive and dry, in 1,523 ticks in the local build, and the route freed once none holds it;
  - the golden run's new values.
* T10 confirms the graph's cost and the sweep's on the reference test machine.

## Pros and Cons of the Options

### 1 — lattice nodes, a shared graph, pruned propagation, a store per region

* Good, because every answer is the reference's, and both sides compute it.
* Bad, because the nodes are placed by a rule rather than by hand, so a narrow pass between lattice points has no node. `mvp.nav.node_spacing` is the lever.

### 2 — the literal propagation

* Good, because it needs no argument.
* Bad, because it costs seconds per attempt.

### 3 — a shortest-path algorithm

* Good, because it is the textbook answer and costs less still.
* Bad, because its ties differ from the reference's.

### 4 — the graph on the server only

* Good, because the client does less at connection.
* Bad, because the client cannot check what the server checked.

## More Information

* Related: [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-085](ADR-085-region-partitioned-world-simulation.md), [ADR-097](ADR-097-keyed-draws-and-state-hash.md), [ADR-099](ADR-099-world-snapshots.md), [ADR-102](ADR-102-region-terrain-and-the-terrain-check-at-connection.md), [ADR-103](ADR-103-region-grids-draw-contexts-and-figure-movement.md).
* The rules: R-SIM-330 to R-SIM-359 ([space, routing and physics](../reference/12_simulation_model/05_space_routing_and_physics.md)), R-UNIT-110 and R-UNIT-111 ([Worker](../reference/02_units/worker.md)), MVP-017 and MVP-040 ([MVP scope](../implementation/mvp_scope.md)), WLD-062 and WLD-100 ([world and sites](../gdd/01_world_and_regions/world_and_sites.md)), WLD-163 ([terrain, time and views](../gdd/01_world_and_regions/terrain_time_and_views.md)).
* The code is in GameShared (`Navigation`, `RegionTerrain`) and GameLogic (`RouteStore`, `FigureAdvance`, `RegionSimulation`).
* **Amendment 2026-10-21 (M2 task T5, [ADR-131](ADR-131-structure-step-collision-shapes-and-ports-at-m2.md)).** R-SIM-343's rebuild has no edge to rebuild in the MVP: the walkability tests block only on walls and enabled Barrier Emitters, so a structure of the MVP changes no edge, and Workers pass round structures by the obstacle push.
