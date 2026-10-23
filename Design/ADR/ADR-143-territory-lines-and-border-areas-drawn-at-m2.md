---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T13), owner (O90)
---

# Territory lines and border areas drawn at M2

## Context and Problem Statement

M2 task T13's second step draws what MVP-084 asks of the settlement view, apart from the fog:
- a settlement's territory, as a glowing line on the terrain in its Cladan's colour, of `fx.beam_long`;
- the border areas, marked on it.

[ADR-139](ADR-139-replicating-the-settlement-at-m2.md) replicates each territory as chunks of 8 by 8 cells of `mvp.territory.cell_size` (16 wu), to every client, since territories are of the public layer (WLD-242). Nothing replicates the border areas. They are static and follow from the layout and the tuning (WLD-062): the lens where the discs of `wld.territory.max_radius` about two valley neighbours overlap, inside the region and outside the coast bands, split at the halfway line.

O90 makes the looks provisional, as O89 made T12's. These decisions are open:
- where a line runs through a raster of cells;
- which pass and catalogue entry draw the line and the marks;
- what is marked of a border area, when no holder is replicated (WLD-063);
- where the valley pairs, which the server's territory code holds, live for the client.

## Decision Drivers

* The client draws what the server sends and what the layout gives, and computes no rule of the simulation (MVP-090, DEC-007).
* No new pass, entry or shader where an existing one serves: the HLSL is compiled first by CI.
* Every look is a key of the presentation record (DEC-1002).
* The frame thread's work is measured.

## Considered Options

For the line:
1. **Marching squares over the cell centres**, the line halfway between held and free cells, as camera-facing segments of `fx.beam_long`.
2. **The cells' edges**, a staircase along the cell boundaries.
3. **A circle of the replicated radius** about the Wellspring.

For the border areas:
4. **Dotted spots** of `fx.particle_dot` on each lens's halfway line and edge, in a neutral colour.
5. **The strategic view's marks** of WLD-241's table 4, in the holder's colour.

## Decision Outcome

Chosen options: 1 and 4.
- Option 2 draws every diagonal of the territory as steps of 16 wu.
- Option 3 is wrong wherever WLD-060 clips the territory.
- Option 5 needs the holders, which M2 does not replicate, and WLD-241's marks belong to the strategic view.

* **The territory lines** (`GameClient/TerritoryLook`):
  - **Which.** Every settlement of the replica, in site order. A settlement's line is built again only when its territory chunks differ from those it was built from, and it goes with its settlement.
  - **Where.** Marching squares over the cell centres of the region's raster: in each square of four centres, the cases of the held corners give up to two pieces between the midpoints of its sides. Two held corners on a diagonal are taken as apart, so a territory that touches itself at a corner draws two closed lines. A territory is one connected area (WLD-060), so its line is closed.
  - **Height.** Each end at the terrain's height there + `mvp.territory_line.lift` (2 wu). The pieces are 16 wu or a half-diagonal long, so the line follows the terrain's 16 wu cells.
  - **Drawing.** In pass 10a, with the effects' entry `effect_additive` (ADR-123): a camera-facing segment (RND-306) of `fx.beam_long` for each piece, of half-width `mvp.territory_line.half_width` (3 wu). The beam's glow lies across the strip and is even along it, so the pieces join without seams.
  - **Why that entry.** It adds, tests depth and reads the mip chain. The laser's `laser_bolt` reads the first mip level alone (RND-339), so a far line would shimmer, and the route's `route_line` tests no depth (RND-355), so a spot would show through a hill.
  - **Colour.** The Cladan's colour (MVP-013), `TeamColor` of the settlement's colour index, at alpha `mvp.territory_line.alpha` (0.8), added.
* **The border areas' marks:**
  - **Where.** For each valley pair, the halfway line runs between the two circles' meetings. The edge is each circle's arc inside the other disc. Spots are spaced evenly along each, no further apart than `mvp.border_mark.spacing` (24 wu). A spot inside a coast band is left out.
  - **Drawing.** In pass 10a, with `effect_additive`: a camera-facing spot of `fx.particle_dot` of half-size `mvp.border_mark.spot_size` (4 wu), lifted by its size over the terrain, added.
  - **Colour.** `mvp.border_mark.color` (0.55, 0.55, 0.55) at alpha `mvp.border_mark.alpha` (0.6), the same for every half. The holders and the claims are marked with M3's claims (MVP-053), when the server sends them.
  - **When.** The spots are placed once per terrain, and drawn while the replica holds a settlement. The sandbox has none, so M1's scenes and the frame bench's measurement of them (ADR-126) are unchanged.
* **The valley pairs.** `VALLEY_PAIRS` moves from GameLogic's `Territory.h` to GameShared's `RegionTerrain.h`, beside the layout whose sites it pairs, so that the client reads the same pairs. Nothing else in it changes.
* **The texture.** `fx.beam_long` joins the scene's textures (`SceneTexture::BeamLong`, `textures/fx/beamlong`), the owner's file packed as authored.
* **The cost**, measured on a Linux build at `-O2` of the look, on the development container's Xeon at 2.8 GHz, best of 20 runs, for a territory disc of 1,400 wu on seed 1:
  - building its line, 700 pieces, takes 0.51 ms;
  - a frame with the chunks unchanged takes 6 µs to read them;
  - writing the frame's quads, the 700 pieces and the 1,000 border spots of seed 1, takes 0.06 ms.

  The Windows build is unmeasured, and T19's frame work measures the client on the reference machine.

### Consequences

* Good, because the steward sees the territory grow, in the Cladan's colour, cell for cell as the server holds it, and where its border areas lie.
* Good, because no pass, entry, shader or message is added.
* Bad, because the line is as fine as the 16 wu cells. A diagonal edge shows chamfers of a half-diagonal, and the server's own boundary is not smoother.
* Bad, because the border areas are unmarked by holder until M3, so the owner's run cannot show WLD-063's holding.
* Neutral, because each camera-facing segment turns about its own axis, so at a low view the line reads as a ribbon standing on the ground rather than lying on it.
* It forecloses: a territory drawn from its radius; the strategic view's marks in the settlement view.

### Confirmation

* `GameClientTests`, `TerritoryLookTests`:
  - one cell's line is four pieces half a cell about it, at the lift over the terrain;
  - a square of 5 by 5 cells with a free centre gives two closed lines of the expected length;
  - the line is pass 10a's depth-tested segments of `fx.beam_long` in the Cladan's colour at the line's width and alpha;
  - a line is built again for new chunks, takes its settlement's colour, and goes with its settlement; the border spots stay placed, and with no settlement nothing is drawn;
  - every border spot of seed 1 lies outside the coast bands at its height, on a valley pair's halfway line inside both discs or on one circle inside the other disc, and the spots are pass 10a's depth-tested dots in the mark's size and colour.
* The owner's run on `Debug|ARM64`, in T13's checklist.

## Pros and Cons of the Options

### 1 — marching squares and beam segments

* Good, because the line follows the held cells and is closed.
* Bad, because it is as coarse as the raster.

### 2 — the cells' edges

* Good, because it is the raster exactly.
* Bad, because it is a staircase.

### 3 — a circle of the radius

* Good, because it is smooth and costs nothing.
* Bad, because it crosses halfway lines and coasts that the territory stops at.

### 4 — dotted spots in a neutral colour

* Good, because it needs nothing the server does not send.
* Bad, because nothing tells whose a half is.

### 5 — the strategic view's marks

* Good, because they show holders and claims.
* Bad, because they need replicated holders, and they are the strategic view's.

## More Information

* Related: [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md), [ADR-123](ADR-123-weapons-effects-and-particles-at-m1.md), [ADR-125](ADR-125-battle-marks-route-preview-and-world-markers-at-m1.md), [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md), [ADR-139](ADR-139-replicating-the-settlement-at-m2.md), [ADR-142](ADR-142-the-camera-flies-along-the-view-within-the-stewards-areas.md).
* The rules: MVP-013, MVP-053, MVP-084, MVP-090; WLD-060, WLD-062, WLD-063, WLD-241, WLD-242; RND-306, RND-339, RND-355.
* Owner decision O90; DEC-1002.
* The code: `GameClient/TerritoryLook`, `CrowdLook` (the texture), `WorldPresentation`; `GameShared/RegionTerrain.h` (`VALLEY_PAIRS`); the presentation record's `mvp.territory_line.*` and `mvp.border_mark.*`.
* **Amendment 2026-10-23 (M3 task T11, [ADR-166](ADR-166-the-relations-panel-and-the-notifications-of-m3.md)).** Each spot on a half's edge knows its area, and takes its claimant's colour while a claim on it stands, else its holder's from the valley's status, else the neutral mark's; the halfway line stays neutral.
