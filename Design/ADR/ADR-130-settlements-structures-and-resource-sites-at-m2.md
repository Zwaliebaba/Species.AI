---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M2 task T4)
---

# Settlements, structures and resource sites at M2

## Context and Problem Statement

M2 task T4 puts the settlement into the world: the steward's settlement at the site the steward picks, with the other three sites free (MVP-012); its Cladan (MVP-013); its Wellspring, from which its first Workers come (MVP-020, MVP-021); its stores in integer thousandths of a resource unit (MVP-030); its population limit (MVP-022); its founding territory (MVP-020); and the region's resource sites with their gathering ports (WLD-080 to WLD-085), with WLD-163's check 7 (ADR-102 left it for M2). The state hash covers all of it, and the region job owns it (MVP-096).

Until T4 a region held teams of figures and nothing that stands still. The structure kinds of T5 (the dwelling, the storehouse and the turret), the steps of T6 and T7, and the replication of T11 all read what T4 decides:
- where a settlement and a structure live, and how they are numbered;
- how a structure is placed from its mesh's simulation data record (ADR-129);
- how the first Workers arrive;
- where the resource sites come from, and what check 7 checks.

## Decision Drivers

* The region job owns its settlements and their Workers (MVP-096, ADR-085 item 1), and reads no other owner's state.
* Every value is state the hash covers, or derived data rebuilt from state and the seed (ADR-081, ADR-083); amounts are exact (MVP-030, R16).
* The simulation reads a mesh only through its record (AST-043, ADR-129), and a structure's hit volume is what the obstacle grid and the pushes test (ADR-103).
* A resource site is world data that never moves (WLD-084), and every site is given the same (WLD-042).
* T5 to T7 extend what T4 builds, rather than replace it; M3 adds three AI settlements to the same region.

## Considered Options

1. **The region owns a settlement list and a structure pool.** A settlement is a record of the region, one per site; its Workers are the figures of the team slot that is its site; every structure, the resource sites included, is an entry of one sliced pool.
2. **A settlement owner of its own** beside the regions, which holds the stores and the territory and sends messages to the region whose figures it counts.
3. **The team is the settlement.** The stores, the territory and the Wellspring are fields of the region's team slot, and the resource sites are derived data rebuilt from the seed, their ports apart.

## Decision Outcome

Chosen option: 1. MVP-096 makes the region job the owner of its settlements, so option 2 would cross owners on every gathering and every new Worker, with messages that ADR-085 keeps for what crosses a region. Option 3 ties a settlement to a team slot, which the sandbox and M3's battles use for things that are no settlement, and makes the resource sites' manned ports state with no structure to hold them.

* **The settlement** (`Settlement`, GameLogic).
  - Its local site and its team slot, which is its site, so that the four settlements of a valley never share a slot and need no table between them.
  - Whether a steward runs it (PIL-007): the steward who connects commands the team of each such settlement (`ConnectSteward`).
  - Its Cladan: its colour, the site's index, so that the four Cladans of a world differ (MVP-013); its temperament, none for the steward's (MVP-057's three come with M3); and its modifiers, an empty list of a stat's tuning key and a factor, the place that O7 and O11 ask for (MVP-099).
  - Its fidelity, always full (PIL-057, MVP-096); its Wellspring by stable id and slot; its stores in integer thousandths of a resource unit, every tuning amount rounded once (`MilliRuOf`); and its territory's radius, which starts at `wld.territory.founding_radius` and never shrinks (MVP-026; T6 grows it).
  - Pure functions give the stores' capacity, a deposit that a full store refuses whole, a payment that the stores make whole or not at all, and the population limit (MVP-021, MVP-022, MVP-031, MVP-032).
* **The structure** (`Structure`, GameLogic), an entry of the region's structure pool, which grows by `ref.sim.container.structure_step` (R-SIM-204).
  - Its stable id from the region's counter, its kind (the Wellspring and the two resource sites; T5 adds three), the team of the settlement it belongs to or none, the resource site it stands for, its position on the terrain and its horizontal front.
  - Its centre and radius are the bounding sphere of its record's collision asset at its transform, or its position and `ref.sim.structure.default_radius` without one (R-BLDG-014, R-SIM-118). That sphere is its hit volume, the form of ADR-103.
  - A mesh's +z lies along the front, +y up and +x to the front's right, the left-handed frame of the renderer.
  - Its ports are made once from its record's port sockets in index order (R-BLDG-016, R-BLDG-019, R-BLDG-020). Each starts at its socket's place and is pushed clear: while its sphere of `ref.bldg.port.clearance_wu` hits the structure, it steps `ref.bldg.structure.push_step_wu` directly away from the structure's position. It then stands on the terrain. A port holds its occupant and one reservation count per team slot that has reserved it; T5 evaluates them and T7 reserves them.
  - T4 adds no structure to the obstacle grid and has no structure step; both are T5's (R-SIM-218 step 4, R-BLDG-030).
* **The Wellspring.** It stands at its site's centre, facing out along the site's outward diagonal, where the home resource site at 0° stands (WLD-082). Its record must have a door socket.
* **The first Workers.** A Worker that the Wellspring makes appears at its door socket as R-ECON-043 says a Spawn Node's does (MVP-021), and so do the settlement's first `mvp.settlement.start_workers`, one after another when it is founded:
  - with no position spread;
  - airborne;
  - with the velocity (door front + j) × `ref.econ.spawn_node.door_speed`, each component of j drawn from U[−`ref.econ.spawn_node.door_jitter`, +`ref.econ.spawn_node.door_jitter`). The draw is `DrawPurpose::DoorJitter`, and its subject is the Worker's own id, the region's next.
* **The resource sites** (`ResourceSitePlace`, GameShared).
  - The world generator places them from the layout as part of the region's terrain, so the client and the server build the same ones from the seed (WLD-084, ADR-102).
  - Each local site has its nine home resource sites and the two at the centres of its valley border areas. The centre of a border area is the centroid of the half of the lens of WLD-062 nearer the site, 894.3 wu along the line to the neighbour. The central resource site stands at the valley's centre.
  - A ring of n sites stands at bearings 180°/n apart, symmetric about the outward diagonal: WLD-082's −60°, 0° and +60°, and −72° to +72°, so its angles need no keys.
  - The ninth stands at `wld.resource.extra_bearing` (DEC-1000).
  - A new world that is no sandbox has a structure for each, of no settlement, in the terrain's order. Its ports are the record's, so a resource site's mesh carries `mvp.resource.ports_per_unit` ports for each unit.
  - The server starts only when the records hold this and the Wellspring's has its door (`CheckStructureRecords`). A tuning that changed the ports per unit without the meshes is refused rather than met with sites of the wrong size.
* **Check 7.** With one resource kind (O57) and every home resource site one unit, check 7 compares the four sites' totals within `wld.resource.home_yield_tolerance`. A resource site at or below the shore height, or on ground steeper than walkable, yields nothing there, since no Worker reaches it (WLD-002, DEC-1000).
* **The world's start.** `WorldStart` gains the steward's site, which the journal writes (its format becomes 4). A new world that is no sandbox has its resource sites placed and, with that site, the steward's settlement founded there (`StartWorld`). `WorldSimulation::Desc`, `StartWorld`, `ReplayJournal` and `RestoreWorld` take the structure records, which the world server already holds (ADR-129).
* **The state.** Each region writes its settlements in founding order and then its structure pool after its bot navigation nodes. `WORLD_STATE_VERSION` becomes 9, and an older region migrates to one with neither (ADR-099). The reader refuses:
  - a second settlement on one site;
  - a settlement whose team or Wellspring the region does not hold;
  - a structure of a team it does not hold.

  The golden hashes of DBG-147 change with the two empty lists.

### Consequences

* Good, because T5 to T7 add structure kinds, a structure step, obstacles and Worker tasks to a settlement and a pool that already exist, are hashed and are saved, and M3's AI settlements are three more founded settlements.
* Good, because the stores cannot drift: every amount is an integer of thousandths, and a load or a cost is taken whole or refused.
* Good, because a resource site's ports, the meshes and the tuning are checked against each other when the server starts.
* Bad, because the sphere is a coarse hit volume. The Wellspring's is 33.8 wu, and the storehouse's 18.3 wu sphere holds its drop-off entrance, authored 14 wu out on the loading platform. Whether T5 keeps spheres for the obstacle grid, or tests the collision asset's triangles (R-SIM-123), decides where Workers can stand.
* Bad, because a settlement's team slot is its site, so a region's teams 0 to 3 belong to its settlements. The sandbox, which has no settlement, keeps its own sides.
* Neutral, because the replication still sends no structure and no settlement; T11 adds them, and with them the presentation events of a structure's creation (R-BLDG-029).

### Confirmation

* GameSharedTests:
  - the resource sites of seed 1 in their pattern and order, 11 units for each site and 3 for the central one;
  - every resource site of seeds 1 to 8 on walkable ground, 200 wu from every other, with each seed passing at its first attempt;
  - check 7 refusing a home site moved into the sea, a site with one home site fewer and a tightened tolerance, and not counting a border site.
* GameLogicTests (`SettlementTests`):
  - the population limit and its maximum;
  - fifteen thousand loads of 0.1 ru filling the Wellspring exactly, a full store refusing a load, and a cost paid to the thousandth;
  - the steward's settlement at the site it picked with the others free, its Wellspring at the site's centre facing out, and its first 20 Workers airborne at the door with their own jittered velocities;
  - every resource site a structure with its gathering ports;
  - a port inside its structure pushed clear and one outside left where it is;
  - a settled world restored from its snapshot giving the same hash on every tick;
  - the steward commanding its settlement.
* GameLogicTests (`WorldTests`): a server whose resource-site mesh disagrees with its tuning does not start.
* Measured, 2026-10-21, on the committed meshes cooked by the content build, with the committed tuning on seed 1: every port stands where its socket is authored. The smallest clearance between a port's sphere and its structure's is 0.68 wu, on the dwelling. ADR-128's resource site and storehouse had their ports at 16 and 20 wu, where the push moved them 2 and 4 wu, so their recipes now put them at 18 and 24 wu.

## Pros and Cons of the Options

### 1 — the region's settlement list and structure pool

* Good, because a gathering, a deposit and a new Worker change one owner's state, as R-SIM-219 orders a region's steps.
* Bad, because a settlement's identity is its site, which suits the MVP's one region and needs revisiting where a site can change hands.

### 2 — a settlement owner of its own

* Good, because a world-level view of each settlement would serve the strategic view of B6.
* Bad, because every Worker made and every load delivered would cross owners within a tick, against ADR-085's messages between owners.

### 3 — the team is the settlement

* Good, because no new record is needed.
* Bad, because the sandbox's teams and M3's attacking expeditions are teams that are no settlement, and resource sites rebuilt from the seed would need their port state held somewhere else.

## More Information

* DEC-1000 holds the owner's question on the ninth home resource site: on M1's terrain its place on the diagonal is in the sea or on a slope on 93 of seeds 1 to 100.
* ADR-099's state version, ADR-102's check 7 and ADR-109's journal format are amended with this record.
* **Amendment 2026-10-22 (owner decision O87, DEC-1000 resolved).** The owner chose option A: the ninth home resource site stays at `wld.resource.extra_bearing` (84°), which is now final. Nothing in the generator, check 7 or the state changes.
