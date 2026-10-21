---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T13)
---

# The world frame at M1: camera-relative surfaces, the region's terrain, sea and sky, fog and the two lights

## Context and Problem Statement

M1 task T13 draws the valley of a seed (MVP-080, MVP-081): its terrain facets and wire grid, the sea ring and its shore waves, the sky, fog and the lights.

The renderer's and the terrain's rules are written for a reference map at the origin; the MVP's region stands at 30,720 wu (ADR-102). Several rules leave the form open:
- the terrain's vertex form and its culling;
- where the waves are evaluated;
- how the sky covers a region;
- what the lights are, since DEC-562 is Open and owner decision O75 asks for a key and a fill light proposed in data.

The specs also contradict each other in five places. M1 needs one answer for each.

## Decision Drivers

* WLD-246: every point of the reserved extent renders with an error of at most 0.01 wu.
* The colours are a function of the field alone, so a rebuild gives the same picture (TER-185, TER-221).
* CI must be able to test every CPU derivation without a GPU.
* O75's M1 forms: CPU culling, and the simpler form where a baseline asks for GPU work.

## Considered Options

* 1 — CPU-built surfaces in engine code (NeuronCore and NeuronClient): per-triangle terrain vertices in 1,024 wu chunks culled on the CPU, the waves evaluated on the CPU each frame, and the sky placed about the region and the camera
* 2 — the same surfaces, with the wave heights in a compute pass (RND-145) and the chunks drawn by one indirect draw (RND-138)
* 3 — a structured buffer per triangle read through `SV_PrimitiveID` in place of per-triangle vertices

## Decision Outcome

Chosen option: "1". It is the M1 form O75 asks for, and every derivation is a pure function that CI tests. Option 2's compute pass and indirect draw add a UAV and a barrier to a frame that no CI job runs. Option 3 saves memory that a 16 MB terrain does not need.

* **Render space** (WLD-246): every position a shader sees is relative to the camera.
  - The view matrix holds the camera's rotation alone.
  - Each draw's translation is its origin minus the camera, taken in binary64 and rounded once (`CameraRelativeTransform`).
  - Vertices are local to their object: a chunk, or the sea plane.
  - Texture coordinates never come from a render-space position: the grid's are baked, and the sea's come from plane coordinates.
* **The terrain** (`NeuronCore/TerrainSurface`, RND-132 to RND-134, TER-180 to TER-185):
  - RND-132's needed pairs and their two triangles per step; the sink of RND-133; facet normals in binary32 (TER-077).
  - TER-181's colours in build order from `terrain.ramp`, with the region's absolute coordinates in the colour chain's seed. TER-180's normaliser is the grid's highest sample, because a region states no maximum height.
  - Each triangle carries its facet colour and normal on all three corners (ADR-020) in a 36-byte vertex, with its grid coordinate in cells from the region's origin.
  - A pair at the end of a row never joins the first pair of the next row, because a region has sea along its border. This narrows RND-132's row-wrap triangles.
  - Seed 1's valley has 146,224 triangles in 25 chunks of 1,024 wu (`wld.terrain.chunk_side`), 15.8 MB of vertices, built in 27 ms. The sea takes 5 ms more. These are measured by the T13 probe on this container's g++ at -O1; the owner's Debug build will be slower.
  - The wire grid is RND-135 as written: `fx.terrain_grid` once per cell through the linear-wrap sampler (DEC-326 option A). The owner's texture draws one diagonal that crosses the mesh's; whether to mirror it is a question for the owner's run.
* **Culling** (RND-138, O75): each chunk's box is tested against the view volume's six planes on the CPU, conservatively (RND-236). Each visible chunk is one draw. GPU culling and the single indirect draw of ADR-011 are deferred (ADR-011 amendment).
* **The sea** (`NeuronCore/SeaSurface`, TER-195 to TER-205, RND-140 to RND-147) is restated about the region:
  - the plane runs from the region's origin less half a side, over twice the side;
  - the shore mask's pixel (x, z) lies at the plane origin plus (x, z) × 80 wu;
  - the depth map, the tiles and the wave pairs stand on that origin, and the wave tables measure from it.

  The shore mask is a one-level R8 texture, read in its red channel. RND-142's "all mip levels" has one level to read.

  The waves at sea detail 1 are evaluated on the CPU each frame into the upload ring (`EvaluateWaves`). That is 0.11 ms on the probe for seed 1's 4,229 pairs. The compute pass of RND-145 is deferred (ADR-011 amendment).

  TER-201's "consecutive emitted pairs" would bridge a gap in a row, and every row change, with long triangles, so a wave pair joins only its neighbour one cell earlier in its row. The colour chain still runs through every emitted pair in build order, as TER-205's loop has it.

  `sea_waves` writes depth, as RND-061's table states, where RND-417's wording says a non-solid layer never does; it matters only once figures stand at the waterline (T16).
* **The sky** (`NeuronClient/SkyLayers`, RND-150 to RND-154): the reference's extents are a 2,000 wu map's, so in a region:
  - The grid's lines stand on the 80 wu lattice of the region's origin. The 125 positions nearest the camera are drawn on each axis, each line reaching 5,000 wu either side of the camera, beyond its 4,000 wu fog end. So the lines are fixed in the world and never end in view.
  - The five cloud layers span 17,000 wu about the region's centre, beyond their 5,000 wu fog end from anywhere over the region.
  - The cloud offset is one tick's worth of `rnd.sky.velocity` per tick displayed since the world's first, in texture repeats (RND-152).
  - A BC4 mask's texel is its intensity on all three channels with alpha 1, so the additive layers add the colour times the intensity times the layer's alpha.
* **Fog** (RND-222 with O59): the fog ends at max(`rnd.fog.min_end`, `wld.region.side` + `rnd.fog.edge_margin`) = 6,120 wu of view depth and starts a third of that before the camera, at 2,040 wu. The terrain and the sea fog to `rnd.frame.clear_color`, and each sky layer to black over its own range.
* **The lights** (RND-226, O75): the key and the fill light are `mvp.light.key.*` and `mvp.light.fill.*` of the presentation record (ADR-116). The values proposed are the renderer's test rig: a warm key towards (−0.04, 0.43, 0.90) in (2.0, 1.0, 0.4), and a white fill towards (0.07, 0.35, 0.93). After the T17 run they are a softer warm key towards (−0.04, 0.60, 0.80) in (1.2, 1.0, 0.65) and a grey fill of 0.25 towards (0.60, 0.40, −0.60) (amendment below). DEC-562 stays Open with these as its stated behaviour until the owner's values are in the record. The light lag of RND-227 is moot with T13's still camera.
* **The development view** (`GameClient/WorldPresentation`) is a stand-in for T14's camera, like O65's stand-ins:
  - an eye 400 wu beyond the region's +z coast at a height of 700 wu, below the lowest cloud layer;
  - looking along the reference's initial front, normalize(0, −0.5, −1);
  - a 60° field of view.

### Consequences

* Good, because the surfaces are pure functions of the field and the record, tested in CI and identical after a rebuild.
* Good, because the region's restatements keep every rule's behaviour and name its origin once.
* Bad, because the waves cost CPU time every frame and 387 KB of upload a frame, which the compute form avoids.
* Bad, because the terrain keeps a 16 MB CPU copy of its vertices for the rebuild, until the asset cache can produce them again.
* It forecloses nothing: the chunk draws, the wave evaluation and the sky instances are the inputs the GPU forms take.

### Confirmation

* CI: `NeuronCoreTests/TerrainSurfaceTests` (the needed pairs, the sink, the normals, the ramp's columns and rows, two equal builds, the chunks; the masks, the tiles, the wave cells and their motion) and `NeuronClientTests/WorldFrameTests` (reversed depth, the Hor+ field and its clamp, WLD-246's precision, the culling, the sky's layers). `GameClientTests/WorldPresentationTests` checks the fog figures, the development view and seed 1's world.
* The owner's T13 run: the valley of a seed is drawn, the same for the same seed, with the debug layer silent (M1 plan T13).

## Pros and Cons of the Options

### 1 — CPU surfaces, CPU culling, CPU waves

* Good, because every step is tested without a GPU.
* Bad, because the CPU does each frame what a compute pass could.

### 2 — the compute and indirect forms

* Good, because it is RND-138, RND-145 and ADR-011 as written.
* Bad, because its UAV, barrier and indirect arguments meet their first test on the owner's machine.

### 3 — triangle data through SV_PrimitiveID

* Good, because a vertex would be 12 bytes, not 36.
* Bad, because it adds a structured buffer and an index to every terrain draw to save memory that is not short.

## More Information

* Related: [ADR-011](ADR-011-world-surface-rendering.md), [ADR-016](ADR-016-view-projection-and-depth-conventions.md), [ADR-020](ADR-020-flat-shading-and-team-tint.md), [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-102](ADR-102-region-terrain-and-the-terrain-check-at-connection.md), [ADR-113](ADR-113-the-frame-at-m1.md), [ADR-114](ADR-114-gpu-resources-at-m1.md), [ADR-116](ADR-116-presentation-tuning-record.md).
* The rules: RND-132 to RND-155, RND-210 to RND-233, RND-266 to RND-268, TER-180 to TER-205, WLD-246; owner decisions O59, O75; DEC-326, DEC-562.
* The code is in NeuronCore (`TerrainSurface`, `SeaSurface`), NeuronClient (`WorldRenderer`, `SkyLayers`, `FrameView`, `Shader/`) and GameClient (`WorldPresentation`).
* **Amendment 2026-10-21 (M1 task T14, [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md)).** The player camera of the settlement view replaces the fixed development view, which is removed; the frame takes its pose from the camera.
* **Amendment 2026-10-21 (M1 task T17c, [ADR-127](ADR-127-left-button-verbs-and-order-feedback-at-m1.md)).** The two lights are retuned after the owner's T17 run, in WLD-183's pattern: a warm key towards (−0.04, 0.60, 0.80) in (1.2, 1.0, 0.65) and a neutral grey fill of 0.25 towards (0.60, 0.40, −0.60), from the side opposite the key. The test rig's key, low and strongly orange, lit the facets at a grazing angle in stair-step blocks, turned the ramp's pale shore rows into an orange band and the valley olive, and its white fill from nearly the key's direction added no shape. The values were chosen on an offline reproduction of the terrain pass (`TerrainPS`'s lighting, grid and fog over the client's own `BuildTerrainSurface` of seed 1121316016, from the camera's entry pose and a closer one), not on the device; they stay provisional until the owner's run. `ter.color.jitter_numerator` keeps the reference's 0.45, and the grid gains no fade with distance: under these lights the reproduction showed neither change to matter.
