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

The requirements say where the territory stops, but not how the simulation holds an area that a disc, half-planes, circles and the terrain's shoreline cut. This record decides that first; T6's later parts add the orbs and growth, contact and the seen layer.

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

* **The raster** (`TerritoryMap`) covers the region's square in cells of `mvp.territory.cell_size` (16 wu, a height-field cell's side, O83). A cell is held when its centre is. The territory is derived from the settlement's radius, the terrain and the region's structures. Neither the hash nor the snapshot holds it: it is made when a settlement is founded, when a restored region's terrain is attached, after a snapshot is read, and when the radius changes (`RecomputeTerritory`).
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
