---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M2 task T5)
---

# The structure step, collision shapes and manned ports at M2

## Context and Problem Statement

M1 left these rules waiting for M2's structures (the M2 plan's §2):
- the structures' step of R-SIM-218 and their shapes in the obstacle grid (ADR-103);
- a bolt that stops on a structure (R-CRATE-441 step 6, ADR-105);
- an orb pushed out of a structure (R-UNIT-321);
- the route rebuild of R-SIM-343 (ADR-104);
- the important nodes of R-BOT-049 (ADR-107).

ADR-130 gave the region its structures, with ports that nothing yet evaluates and no Worker yet mans. The first part of M2 task T5 makes the structures part of the world's step before placement, construction and the turret build on them. Three questions need deciding:
- what a structure's hit volume is;
- how the obstacle grid keeps it;
- what of the Worker's port behaviour comes now and what waits for the task search of T7.

## Decision Drivers

* A structure's sphere, mesh and ray tests read its collision asset, never its render mesh (R-SIM-123, AST-044), and the simulation reads the mesh only through its records (ADR-129).
* A world restored from its snapshot steps to the hashes of the world never stopped (ADR-099), so derived data is made the same way after a load as during play.
* A Worker that walks to a port and mans it must reach the port. With ADR-103's interim sphere, the storehouse's drop-off entrance lies inside its own 18.3 wu sphere (ADR-130).
* Every rule M1 left waiting is built or recorded as having nothing to do, with the rule that says so.

## Considered Options

1. **The collision asset's triangles**, tested inside the bounding sphere, which stays the shape's centre and radius for the pushes.
2. **ADR-103's sphere**, the bounding sphere of each structure's collision asset.
3. **A sphere for each collision node**, from the records' node spheres (ADR-129).

## Decision Outcome

Chosen option: 1, because it is the rule (R-SIM-123), its cost is bounded by the bounding sphere's early-out, and it lets a Worker reach the ports, doors and entrances that the recipes put against a structure's walls. Option 2 makes the Wellspring a 33.8 wu ball and puts the storehouse's entrance inside its own volume. Option 3 is coarser than the mesh where it matters, at a wall's face.

* **The shape** (`ObstacleShape`). It holds:
  - the structure's centre and the radius of its bounding sphere, from which the pushes measure (R-UNIT-114, R-SIM-380);
  - the triangles of its collision asset in the world, from the structure's transform (ADR-130);
  - the structure's slot.

  A sphere test hits when the sphere overlaps the bounding sphere and comes strictly within its radius of a triangle, the nearest point by Ericson's regions. A segment test hits when the segment passes within the bounding sphere and crosses a triangle (Möller and Trumbore). A shape with no triangle is its sphere, as M1's tests' shapes and a structure whose record names no collision asset are. As in the reference, the tests are of the surface: a small sphere wholly inside a mesh hits nothing.
* **The records.** `LoadStructureRecords` also loads the collision asset each record names, by the key the record holds (ADR-129), into `StructureRecords::collisions`. The records the tests write name none, so their structures are spheres.
* **The obstacle grid.** The grid is derived data, made again from every structure in slot order:
  - when a structure is added (R-BLDG-030);
  - when the structure step removes one (R-SIM-218 step 4, DEC-095);
  - when a restored region's terrain is attached.

  Rebuilding in slot order, rather than appending as the reference's incremental update does, keeps each cell's list in structure order (R-SIM-321) after a load as during play, which the hashes need. A region holds tens of structures, so the rebuild costs little.
* **The structure step** (`AdvanceStructure`), in R-SIM-218 step 4, each structure once a tick in its slice range, followed by the bot navigation nodes, which ADR-107 placed in step 4. It runs R-BLDG-031's base advance, which is the port evaluation:
  - a port's occupant is the first Worker, of any team, in actor-grid result order that mans that very port on that very structure, strictly within `ref.bldg.port.radius_wu` horizontally (R-BLDG-021);
  - an occupied port's reservation counts are all 0, and an unoccupied port's fall by `ref.bldg.port.reservation_decay_per_tick` (R-BLDG-022).

  A Worker's operation adds 1 to its team's count up to `ref.bldg.port.reservation_cap`, and a port is available below `ref.bldg.port.reservation_limit` (R-BLDG-023). One Worker's reservation a tick is drained by the step that follows it, so a reservation turns a second Worker away only within the tick (R-BLDG-025).
* **The Worker's port states.**
  - `WorkerState::ToPort` (R-UNIT-069): the Worker operates its port for its team every tick and walks to it. On arrival it faces the port's front, stands still and becomes Manning. It becomes Idle when the structure is gone or another Worker occupies the port.
  - `WorkerState::Manning` (R-UNIT-070): it stands still while it is the occupant, and becomes Idle otherwise. A manning Worker runs no task search unless its player-orders flag is set (table 2.5-A, R-UNIT-089).

  The Worker records the structure by id and slot and the port by index (`SendToPort`). The two states are the last values the replicated task's three bits hold (ADR-111). The searches that choose a port are T7's (R-UNIT-090, MVP-031).
* **A bolt** that meets a structure of its cell ends there. The meeting is the segment of its step against the structure's shape, after the terrain and the map bounds. It raises the new presentation event `LaserHitStructure` and deals no damage (R-CRATE-441 step 6). The client draws nothing for it until it draws structures (T12).
* **An orb** is pushed out of a structure by the existing push (R-UNIT-321), which now finds structures in the grid.
* **R-SIM-343's rebuild** has no edge to rebuild in the MVP. The route graph's walkability tests block only on walls and enabled Barrier Emitters (R-SIM-335, R-SIM-338), and the MVP has neither (ADR-104). A structure therefore changes no edge, and Workers pass round structures by the obstacle push, as in the reference.
* **R-BOT-049.** A node is important when a structure with at least one port stands strictly within `ref.bot.node.important_radius` of it, in slot order. The MVP has no other kind that makes a node important.
* **State.** Each figure writes its port's structure id and slot and the port's index after its nearest node. The Worker's state bound rises to Manning. All of this is part of version 9 (ADR-099), which no world has yet been saved at (the M2 plan's §8, T9).

### Consequences

* Good, because a Worker reaches a port, a door or an entrance against a wall, a bolt stops at a structure's face, and an orb leaves a structure's footprint, each against the collision asset that the content build checked.
* Good, because the grid after a load is the grid of the world that never stopped.
* Bad, because a sphere test now costs up to one nearest-point computation per triangle inside a bounding sphere: 52 for the Wellspring and 16 to 40 for the others. The Worker at work is measured again at T10 (ADR-080).
* Bad, because the replicated task has no room for another Worker state; a Carrying state at T7 widens it (ADR-111).
* Neutral, because the reference's incremental grid update is replaced by a rebuild with the same result in slot order.

### Confirmation

GameLogicTests (`StructureTests`):
- a sphere and a segment against one triangle, within and outside the bounding sphere, and a shape with no triangle as its sphere;
- a Worker sent to a resource site's port walking to it, manning it and being its occupant, a second Worker sent there turned away, and the port free once the first stops;
- ten reservations draining by 4 a tick to 0, the port unavailable to that team only, and the cap of 50;
- a bolt fired at the Wellspring stopped with its event;
- a node important exactly when a structure with a port stands within 100 wu.

The settled world restored from its snapshot still gives the hashes of the world never stopped (`SettlementTests`).

## Pros and Cons of the Options

### 1 — the collision asset's triangles

* Good, because it is R-SIM-123's rule, and the recipes' sockets were placed against these shapes.
* Bad, because it costs a nearest-point computation per triangle in reach.

### 2 — the bounding sphere

* Good, because it is cheapest and is ADR-103's interim.
* Bad, because the Wellspring's door and entrance and the storehouse's entrance lie inside or against its sphere.

### 3 — a sphere per collision node

* Good, because it reads only the records.
* Bad, because a node's sphere bounds a whole wall, so a face is as far out as its corners.

## More Information

* **Amendment 2026-10-24 (owner decision O112, [ADR-195](ADR-195-gatherers-home-ports-and-their-turns.md)).** A gatherer walking to its home port while another Worker mans it does not become Idle. It stands `ref.unit.worker.arrival_slow_radius` from the port until the port is free, then mans it.
