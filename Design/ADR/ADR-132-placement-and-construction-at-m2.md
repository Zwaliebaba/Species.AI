---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M2 task T5)
---

# Placement and construction at M2

## Context and Problem Statement

The steward places a structure in the settlement's territory. A placement is refused when the structure lies in the core's centre ring, on ground that is not walkable, or over another structure, a resource site or a port. A placement pays its cost at once and queues a construction site, at most `mvp.construction.queue_max` of them, built in order (MVP-032). Construction is a Worker task at the ports of the first queued site. The site completes when its Workers have given `mvp.construction.work_per_ru` Worker-seconds for each resource unit of its cost (MVP-033). The dwelling raises the population limit and costs more for every dwelling the settlement has; the storehouse adds to the stores' capacity (MVP-034).

The second part of M2 task T5 builds these rules on ADR-131's structures and ports. The requirements leave open:
- the radius of the centre ring;
- what "walkable", "overlapping" and "in the territory" measure;
- where a construction site's progress lives;
- what happens to a dwelling's or storehouse's ports once it is built.

The command that places a structure, with its outcome message, is T8's (MVP-094, MVP-101), and the task search that sends Workers to build is T7's.

## Decision Drivers

* A refusal says which rule failed, in a fixed order, because T8 sends it to the steward (MVP-101).
* The construction site is a structure from the moment it is placed: it is seen rising (MVP-083), it stands in the way (ADR-131), and it can be counted.
* Amounts and progress are integers, so that a run reproduces exactly (MVP-030, R16).
* Every number is a key (MVP-099).

## Considered Options

1. **The site is a structure of its kind with construction state.** It is built when its work is done, and its settlement's queue holds it by id and slot.
2. **A separate construction-site record** in the settlement, replaced by the structure on completion.
3. **Construction outside the structure step**, counted by the settlement from its Workers' tasks.

## Decision Outcome

Chosen option: 1. A site already has the ports its Workers man and the footprint that blocks, so option 2 would duplicate both and swap identities at completion, and option 3 would count work that the port evaluation already establishes (R-BLDG-021).

* **The structure** gains whether it is built, the Worker-ticks given to it, its cost in thousandths of a resource unit and whether it is active (MVP-035, T7). A structure that the world places is built and costs nothing. The settlement gains its queue: the sites in the order they are built, by stable id and slot.
* **Placement** (`PlaceStructure`) places a dwelling, a storehouse or a turret ([ADR-133](ADR-133-the-gun-turret-at-m2.md)) for the settlement at a site. The structure is centred at the point the steward gives and faces away from the Wellspring. The checks run in this order, and the first that fails is the refusal (`PlacementRefusal`):
  1. no settlement at the site, or a kind the steward does not place;
  2. the queue is full;
  3. the structure does not lie wholly inside the territory: its footprint, the horizontal disc of its bounding sphere, inside the disc of the territory's radius about the Wellspring. T6 replaces the disc with the territory's bounds (WLD-060).
  4. the footprint reaches into the core's centre ring, of `mvp.construction.center_ring_radius` (60 wu) about the Wellspring. The Wellspring's ports stand at 38 wu with their 5 wu radius, so 60 wu leaves room to stand beside them.
  5. its position or one of its ports stands on ground at or below the shore height, or steeper than `ref.sim.walk.min_normal_y`, the walkable ground of R-SIM-335 and WLD-163 check 6;
  6. its footprint meets another structure's (resource sites included), or one of its ports comes within `ref.bldg.port.radius_wu` of another's footprint, or one of the other's ports of its own;
  7. the stores do not hold the cost.

  A placement that passes pays the cost, inserts the structure as a site with the next stable id, rebuilds the obstacle grid (ADR-131) and queues the site.
* **The cost** of a dwelling is `mvp.cost.dwelling_ru` × `mvp.cost.dwelling_escalation` to the power of the dwellings the settlement already has, built or queued, rounded once to a thousandth. A storehouse costs `mvp.cost.storehouse_ru` and a turret `mvp.cost.turret_ru`.
* **Construction.**
  - A site needs `mvp.construction.work_per_ru` Worker-seconds for each resource unit of its cost, in Worker-ticks at `ref.sim.tick_period`, rounded once: 3,000 for a dwelling of 300 ru.
  - In the structure step, after its port evaluation, a site gains one Worker-tick for each occupied port while it is in its settlement's queue, so that a destroyed turret's site waits for its rebuild (ADR-133).
  - On the tick it has them it is built and leaves its settlement's queue. It raises the presentation event `StructureCompleted`, the completion flash of MVP-083 and the growth step of O81, which T8's statistics count.
* **Construction ports.** A dwelling's and a storehouse's ports are construction ports: the port evaluation gives them no occupant once the structure is built. Their Workers then find they are no longer the occupant and become Idle (R-UNIT-070). The Wellspring's and the resource sites' ports are manned always. MVP-033 sets the turret's construction ports to be its manning ports, which stay manned (T5's turret).
* **What counts.** `ActiveCount` counts a settlement's built, active structures of a kind. The population limit takes its dwellings (MVP-022), and the stores' capacity its storehouses (MVP-021, MVP-034).
* **The records** must give the dwelling `mvp.construction.ports_dwelling` and the storehouse `mvp.construction.ports_storehouse` port sockets, which the server checks when it starts, as it checks the resource sites' (ADR-130).
* **State.** The structure's four fields and the settlement's queue are part of version 9 (ADR-099), with ADR-130's and ADR-131's.

### Consequences

* Good, because a site is seen, blocks and is counted as the structure it becomes, and its progress is an integer that the hash covers.
* Good, because every refusal has its reason for T8 and the steward.
* Bad, because the footprint is the bounding sphere's disc, which is larger than a gabled dwelling's corners. Sites stand a little further apart than their walls need.
* Bad, because until T6 the territory is a disc, so a placement near a coast or a border area may pass a test that T6 tightens.

### Confirmation

GameLogicTests (`StructureTests`):
- every refusal with its reason;
- the costs of a first and second dwelling (300 and 345 ru) and of a storehouse (200 ru);
- a queued site, and the queue full at 8;
- a placement refused on unwalkable ground;
- 3,000 Worker-ticks for a dwelling: six Workers give 6 a tick, completion leaves the queue with its event, the Workers go Idle and the limit rises from 40 to 70;
- a built storehouse adding 1,500 ru of capacity;
- a route the same before and after a dwelling is placed across it (R-SIM-343, ADR-131).

## Pros and Cons of the Options

### 1 — a structure with construction state

* Good, because the site and the structure are one identity, with one set of ports and one footprint.
* Bad, because every structure carries four fields that most need only at construction.

### 2 — a separate site record

* Good, because a finished structure carries no construction state.
* Bad, because the ports, the footprint and the identity change at completion.

### 3 — work counted by the settlement

* Good, because the structure step stays as ADR-131 left it.
* Bad, because it repeats the port evaluation's answer of who is working.
