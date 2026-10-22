---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T6)
---

# Territory, Essence growth, contact and the seen layer at M2

## Context and Problem Statement

M2 task T6 builds a settlement's territory, its growth from Essence, its contact with a neighbour and what it sees (MVP-016, MVP-023 to MVP-027, MVP-097).
- The territory's radius grows with its Workers (MVP-026).
- The territory stays inside the bounds of WLD-060: the coast, the halfway line of each valley border area, the edge of the home cell, the clearances round the other sites' cores and the central resource sites. It is always one connected area.
- The steward places structures wholly inside it (MVP-032). Until T6, ADR-132 measured that against the disc of the territory's radius.

The requirements say where the territory stops, but not how the simulation holds an area that a disc, half-planes, circles and the terrain's shoreline cut. This record decides that first, then how orbs reach the Wellspring and the Wellspring grows (MVP-023 to MVP-025), when two settlements are in contact (MVP-027) and what each sees (MVP-016).

## Decision Drivers

* The area is one connected piece (WLD-060), so a disc cut by water or by a halfway line keeps only the part joined to the Wellspring.
* A placement asks whether a footprint lies inside the area; an orb and the seen layer ask whether a point does (MVP-023, MVP-016).
* The answer reproduces exactly from the state, so it must be derived only from values the state holds (MVP-093).
* Every number is a key (MVP-099).

## Considered Options

1. **A raster of the region**, cells of `mvp.territory.cell_size`, filled from the Wellspring's cell across the cells' sides, recomputed when the radius grows.
2. **A predicate**: the disc and the bounds tested at the point asked, with no area held.
3. **A polygon**, the disc's outline clipped by the bounds.

## Decision Outcome

Chosen option: 1. Option 2 cannot keep the territory connected: a point across a lake or past a neck of land would pass every test. Option 3 needs a robust clipper against the terrain's shoreline, which is not a polygon.

* **The raster** (`CellRaster`) covers the region's square in cells of `mvp.territory.cell_size` (16 wu, a height-field cell's side, O83). A cell is held when its centre is. The territory is derived from the settlement's radius, the terrain and the region's structures. Neither the hash nor the snapshot holds it: it is made when a settlement is founded, when a restored region's terrain is attached, after a snapshot is read, and when the radius changes (`RecomputeTerritory`).
* **The cells it holds** are those reached from the Wellspring's cell across their sides, breadth first, whose centre:
  1. lies within the settlement's radius of its Wellspring;
  2. lies inside the coast bands, `wld.region.coast_band` from the region's border (WLD-021, WLD-061);
  3. lies within `wld.territory.max_radius` of its site's centre;
  4. lies nearer its site's centre than either valley neighbour's (WLD-062). Its home cell and its own halves of its valley border areas are exactly these points, and the halfway line is where it stops;
  5. lies at least `wld.site.core_radius` + `wld.territory.site_clearance` from every other site's centre;
  6. lies at least `wld.resource.wild_clearance` outside every central resource site's footprint, the horizontal disc of its bounding sphere (WLD-083);
  7. stands on ground above the shore height, `ref.unit.worker.shore_height`, so that the territory stops at the coast and at the shores of any water inside its bounds.

  The MVP's world is one region with no crossing (MVP-010), so the crossing border areas of WLD-060 are not built.
* **A footprint inside it** (MVP-032, `InTerritory`): the cell of its centre and every cell whose centre the footprint's disc covers are held. A footprint may therefore overhang the territory's edge by less than a cell.
* **Orbs** (MVP-023, MVP-025). The orb gains two states, `Drifting` (3) and `Waiting` (4), and the site of the settlement it is bound for.
  - A floating orb inside a settlement's territory drifts to that settlement's Wellspring: each tick its velocity is the horizontal direction to the Wellspring at `mvp.essence.drift_speed`, and its wobble stays. An orb finishes its birth first (R-UNIT-311), so that a death is still seen where it happened. An orb outside every territory keeps the reference's states and ascends.
  - A drifting or waiting orb takes no structure push (R-UNIT-321), which would hold it off the Wellspring's hit volume.
  - It reaches the Wellspring within the Wellspring's radius, its bounding sphere's.
    - Below the population limit it becomes a Worker at the Wellspring's door, as the founding Workers appear (R-ECON-043).
    - Otherwise it waits there while fewer than `mvp.essence.wait_max` of that settlement's orbs wait, and ascends if not.
  - A waiting orb becomes a Worker as soon as there is room, in the orb pool's order.
  - The population is the team's living figures; a Leader counts as the Worker it was.
* **The settlement step.** After its region's slices and AI, each settlement, in founding order:
  - sets its radius to `wld.territory.founding_radius` plus `mvp.territory.radius_per_worker` for each of its population above `mvp.settlement.start_workers`, up to `wld.territory.max_radius`. The radius never falls, and a radius that grows makes the territory again (MVP-026);
  - counts its growth timer down while it is below its population limit and its stores hold `mvp.essence.orb_cost_ru`, and holds it otherwise. At 0 its Wellspring pays that cost for one orb at its centre, and the timer restarts at `mvp.essence.wellspring_interval_s`, 120 ticks (MVP-024). A founded settlement starts its timer there.

  The orb reaches the Wellspring once born, so each Worker follows its payment by the orb's birth, about 6 s, whose rise varies by a few ticks.
* **Contact** (MVP-027, WLD-064, WLD-066).
  - A territory reaches the halfway line of one of its valley border pairs when it holds a cell whose centre lies within a cell's side of the line, inside the lens of the two sites' discs of `wld.territory.max_radius` (`ReachesHalfwayLine`).
  - After the settlements' step, a pair whose two sites are both settled and whose two territories both reach its line gets a contact event, unless it has one: its tick, its kind, its two sites and the pair.
  - A territory never shrinks in the MVP, so contact never ends and a pair has one event at most. The events are the region's state, in the order recorded. Claims that move a border area and border areas that return to their side are B5's kinds of event (WLD-066).
* **The seen layer** (MVP-016, MVP-097). After contact, every step makes each settlement's seen layer once, as a `CellRaster` in cells of `mvp.vision.cell_size` (64 wu). It holds:
  - the cells whose centres the settlement's territory holds;
  - every cell whose centre lies within `mvp.vision.unit_radius` of one of its living figures or of a structure it owns, built or not.

  The layer is derived and never state, and a region has none before its first step. Its readers, the AI and the replication narrowed to it, are M3's (O81); T11 replicates it for the fog. The public layer of WLD-242 is the world's own data and needs nothing made.
* **State.** The orb's two states and its settlement, the settlement's growth timer and the region's contact events are part of version 9 (ADR-099). The events change the golden hash of DBG-147's empty world, which now writes an empty list.

### Consequences

* Good, because the territory is one connected area that stops at every bound, and a point or a footprint is a lookup.
* Good, because it is derived, so neither the hash nor the snapshot changes.
* Bad, because a radius that grows a little makes the whole raster again. A territory at its largest radius holds at most π × 1,400² ÷ 16², about 24,000 cells, and the radius grows at most once for each Worker the settlement gains.
* Bad, because the edge is a cell's width coarse: a footprint may overhang it by less than 16 wu.

### Confirmation

GameLogicTests (`TerritoryTests`):
- the founding territory: the Wellspring's cell, every held cell within the founding radius and the bounds, and about π r² in cells;
- a territory at its largest radius on its own side of each halfway line, out of the coast bands, clear of the other sites' cores and of the central resource site, on dry ground, and reaching a halfway line;
- a footprint well inside the territory accepted, and one that overhangs its edge by 30 wu refused;
- a restored world making the same cells.

GameLogicTests (`EssenceTests`):
- a settlement of 20 Workers with full stores paying for an orb exactly every 120 ticks, and growing a Worker about as often to its limit of 40 and no further, for 20 orbs' cost;
- an orb 300 wu inside the territory closing on the Wellspring at the drift speed and becoming a Worker there, while an orb outside every territory does not drift;
- at the limit, 20 of 21 arriving orbs waiting and the 21st ascending, and a waiting orb taking the room a death leaves;
- a radius of 540 wu for 10 Workers above 20, which stays when five of them die.

GameLogicTests (`TerritoryTests`), on a fixture of two settlements and on one:
- no contact at the founding radius, then one contact event with its tick, its two sites and their pair once both territories reach their halfway line, none after, and the snapshot holding it;
- a seen layer that holds the Wellspring and every cell of the territory, sees around a Worker sent 900 wu out from the next step, and is made once in every step.

## Pros and Cons of the Options

### 1 — a raster filled from the Wellspring

* Good, because connectivity, every bound and the shoreline come from one fill.
* Bad, because it is made again whenever the radius changes.

### 2 — a predicate

* Good, because nothing is held or recomputed.
* Bad, because it cannot see whether a point is joined to the Wellspring.

### 3 — a polygon

* Good, because its edge is exact.
* Bad, because the shoreline is a contour of the height field, which a polygon clipper does not take.

## More Information

* **Amendment 2026-10-22 (M3 task T4, [ADR-159](ADR-159-grievances-anger-and-their-fade-at-m3.md)).** Contact is territory pressure's cause (MVP-051). The relations read whether a pair is in contact now, both territories reaching their halfway line, and not the event recorded once, so that a border area a claim moves (M3 task T5) can end it.
