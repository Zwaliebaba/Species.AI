---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T13), owner (O90)
---

# The camera flies along the view, within the steward's areas

## Context and Problem Statement

M2 task T13's first step changes the settlement view's camera in two ways.

The first is O90. At M1's T17 run the owner found the camera unlike the design, and O90 answers it:
- the forward and back keys fly the camera along its view;
- the side keys still pan it level;
- the pointer stays free, and the view keeps turning towards the point under it, as R-CTRL-471 step 8 states.

The second is WLD-244. The settlement view shows only the steward's areas, and the camera's ground point stays inside them. M1 bounded the camera by the region's square ± `ref.sim.match_entry.camera_border` ([ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md)), since the sandbox had no territory. The areas are:
- the steward's own territory;
- its surroundings up to `wld.view.settlement_margin` (800 wu) beyond it;
- the battlefield of each of its battles, which M2 does not have.

These decisions are open:
- where the fly's vertical part goes among R-CTRL-471's steps;
- what "the camera's ground point" is;
- how the client knows the areas, and how the camera is kept inside them.

## Decision Drivers

* The client draws and steers from what the server sends, and computes no rule of the simulation (MVP-090, DEC-007). The territory arrives as ADR-139's chunks.
* R-CTRL-471's steps stay as written wherever O90 does not change them, and each change is named.
* The camera advances on the frame thread, so what it adds there is measured.

## Considered Options

For the fly's vertical part:
1. **After step 5's cliff test**, with step 6's ground clearance after it.
2. **In step 3**, with the level part.

For the ground point:
3. **The point under the camera**, at its position and at its target.
4. **The point under the screen's centre.**

For the areas:
5. **A raster of cells**, built from the replicated territory by an exact distance transform. A move that would leave the areas slides along one axis.
6. **A disc** of the territory's replicated radius plus the margin about the Wellspring.
7. **The region's square alone** until M3's neighbours.

## Decision Outcome

Chosen options: 1, 3 and 5.
- Option 2 lowers the height before the cliff test. The test then finds the land ahead above the height at every sample, cuts the whole step and lifts the camera, so a descent never moves forward.
- Option 4 leaves the areas whenever the view looks out over the fog, which WLD-244 draws so that it can be seen.
- Option 6 is wrong wherever WLD-060 clips the territory, at the border areas' halfway lines and the coast bands.
- Option 7 leaves WLD-244 unbuilt.

* **The fly** (`SettlementCamera`, R-CTRL-471 as amended):
  - In step 3, the forward key adds the front's horizontal part × dt × rate to the target, and the back key subtracts it. The rate is `ref.ctrl.camera.pan_speed`, × `fast_pan_factor` while the fast pan is held. The side keys pan along the level left vector, as before.
  - After step 5, the front's vertical part × dt × rate is added to the height, or subtracted. The height stays at most `ref.ctrl.camera.max_height`, and step 6 keeps it at least the land + `ground_clearance`.
  - The wheel and the raise and lower actions are unchanged (R-CTRL-461). So are the mouse-look (step 8), the cursor lock (step 9, R-CTRL-464) and ADR-119's steepest front of 89°.
* **The ground point** is the point of the ground under the camera's position and under its target. Step 10 keeps both inside the bounds, and both inside the areas.
* **The steward's areas** (`GameClient/StewardAreas`):
  - **The territory.** The settlement whose team is the client's commanded team (ADR-130's team slot is its site), and the territory chunks of that site in the latest replica. They are read each frame through the id range of ADR-139's chunk ids, and rebuilt only when the chunks differ.
  - **The raster.** Cells of `mvp.territory.cell_size` (16 wu), over the region's square widened by ⌈margin ÷ cell⌉ (50) cells on each side, 420 cells a side. The margin reaches past the square: a territory can come within `wld.region.coast_band` (320 wu) of its edge.
  - **The margin.** A cell is in the areas when its centre lies within the margin of a territory cell's centre. Felzenszwalb and Huttenlocher's exact squared distance transform gives each cell's distance, row by row and then column by column, over the territory's box widened by the margin.
  - **Unknown areas.** Without a commanded team, a settlement of it, or a chunk of its territory, the areas are unknown and every point is inside. So the sandbox, and a world before its first territory chunk arrives, are bounded by the region alone, as at M1.
* **The bound** (step 10):
  - **The square.** The square ± the camera border still bounds the position and the target first. The MVP's world is one region, and the client holds no terrain beyond it, so the areas' margin past the square is cut at 200 wu.
  - **Sliding.** Then, with known areas, a move that would leave them:
    - keeps its x and its old z, if that point is inside;
    - else keeps its old x and its z, if that point is inside;
    - else does not happen.
  - **A point outside** goes to the centre of the nearest cell of the areas. That happens only if the areas shrink under the camera, which M2's territory never does.
  - **Order.** The target is kept inside before the position, each from its place at the frame's start.
* **The cost**, measured on a Linux build at `-O2` of the class alone on the development container's Xeon at 2.8 GHz, best of 20 runs:
  - a rebuild for a territory of the largest radius, 1,400 wu (416 chunks), takes 1.03 ms;
  - a frame with the territory unchanged takes 3.4 µs;
  - a nearest-cell search takes 0.22 ms.

  The territory changes as its radius grows, about once a Worker. The Windows build is unmeasured, and T19's frame work measures the client on the reference machine.

### Consequences

* Good, because looking down and pressing the forward key descends, as the owner asked, and the side keys and every verb of M1 keep their meaning.
* Good, because the camera keeps to the steward's own territory and its margin from the first chunk on, with no message added. The step-2 territory lines and step-3 fog of T13 can read the same raster.
* Bad, because of the forward key held with the pointer still. The cursor lock keeps the pointer on its ground point, and the mouse-look turns the view to it, so the camera flies down onto that point. It comes to rest above it at the ground clearance, looking down, until the mouse moves the pointer on.
  - Measured by `SettlementCameraTests`: from the entry pose on seed 1, after 10 s of the fast pan the camera is within 50 wu of the point and its front more than 81° below the horizontal.
  - The owner judges this on T13's run, against the options named in its checklist.
* Bad, because a move along a diagonal edge of the areas slides in steps of a cell, which the position's smoothing (step 7) softens.
* Neutral, because the bound is the client's alone: nothing on the server stops a client from looking elsewhere. M2 replicates the whole region anyway (ADR-111), and M3's interest set narrows it.
* It forecloses: the fly's descent tested as a cliff; a bound on the point under the screen's centre.

### Confirmation

* `GameClientTests`, `StewardAreasTests` (WLD-244):
  - one territory cell's areas: the cell 800 wu away is in, the next out, and the margin is a disc;
  - with two cells, the nearer decides;
  - the margin reaches past the region's square;
  - areas are unknown without a team, for another team's settlement, and without a chunk, and every point is then inside;
  - they are rebuilt only when a chunk changes;
  - a point outside goes to the nearest cell.
* `GameClientTests`, `SettlementCameraTests`:
  - AC-CTRL-CA-01 as amended: one second of the forward key moves the target 250 wu, or 1,000 wu with the fast pan, along the view, and the side key as far at a constant height;
  - one second of the forward key descends by the front's downward share, and the back key climbs back;
  - the forward key held brings the camera onto the point under the pointer, at the ground clearance and looking down, and the back key held climbs to the highest height;
  - AC-CTRL-CA-04: flying towards a point beyond the region's corner stops on the bound;
  - with a territory of 300 wu, flying towards a point beyond the areas keeps the camera inside them every frame, and stops it at their edge.
* The owner's run on `Debug|ARM64`, in T13's checklist.

## Pros and Cons of the Options

### 1 — the vertical part after the cliff test

* Good, because the cliff test sees the land ahead against the height the camera holds, as at M1.
* Bad, because a descent into rising land is stopped by step 6, not slowed by step 5.

### 2 — the vertical part in step 3

* Good, because the whole fly is one step.
* Bad, because every descent near the ground reads as a cliff, and the camera climbs instead.

### 3 — the point under the camera

* Good, because the view can look out over the fog while the camera stays inside.
* Bad, because the screen's centre can rest on land outside the areas.

### 4 — the point under the screen's centre

* Good, because it is what WLD-240 calls the ground point for the change of view.
* Bad, because a view along the horizon has no such point, or one kilometres away.

### 5 — a raster from the replicated territory

* Good, because it follows the territory's real shape, cell for cell.
* Bad, because a rebuild costs about a millisecond on the frame thread.

### 6 — a disc about the Wellspring

* Good, because it costs nothing.
* Bad, because it overstates the territory wherever WLD-060 clips it.

### 7 — the region alone

* Good, because nothing changes.
* Bad, because WLD-244 is not met.

## More Information

* Related: [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md), [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md), [ADR-130](ADR-130-settlements-structures-and-resource-sites-at-m2.md), [ADR-134](ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md), [ADR-139](ADR-139-replicating-the-settlement-at-m2.md).
* The rules: WLD-060, WLD-240, WLD-244; R-CTRL-461, R-CTRL-464, R-CTRL-471; MVP-101.
* Owner decision O90.
* The tuning key `wld.view.settlement_margin` joins the MVP's record, with WLD-244's value.
* The code: `GameClient/StewardAreas`, `SettlementCamera`, `SettlementView`.
* ADR-119 is amended with this record.
* **Amendment 2026-10-23 (M3 task T12, [ADR-167](ADR-167-the-conflict-verbs-and-markers-at-m3.md)).** The battlefield of each battle that the valley status lists with the steward's settlement as a side joins the steward's areas while it is listed (WLD-244), so the camera may follow the expedition into its battle.
