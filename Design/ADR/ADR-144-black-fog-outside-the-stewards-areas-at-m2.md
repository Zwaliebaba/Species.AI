---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T13), owner (O90)
---

# Black fog outside the steward's areas at M2

## Context and Problem Statement

WLD-244 limits the settlement view to the steward's areas. Beyond them it draws the terrain and the public layer under black fog, with no Worker, structure or expedition, and MVP-084 asks the same of the MVP. M2 task T13's third step builds it. Its first step gives the client the areas, in [ADR-142](ADR-142-the-camera-flies-along-the-view-within-the-stewards-areas.md)'s raster of cells of 16 wu, and its second draws the public layer's territory lines and border areas over the terrain ([ADR-143](ADR-143-territory-lines-and-border-areas-drawn-at-m2.md)).

The renderer has level fog by view depth (RND-222) in the terrain's and the sea's shaders, and no fog by place. These decisions are open:
- how the shaders learn where the areas are;
- how the fog's edge looks;
- what is left undrawn outside, and where that is decided;
- how a mask that changes as the territory grows reaches the GPU without stalling a frame.

O90 makes T13's looks provisional.

## Decision Drivers

* The engine knows no game concept (AGENTS.md R9): "steward" and "areas" stay in GameClient.
* No new pass. The shaders are compiled first by CI, and its WARP render check draws a settlement world with the debug layer (ADR-117).
* A frame never waits for an upload (ADR-114).
* Every look is a key of the presentation record (DEC-1002).
* The frame thread's work is measured.

## Considered Options

For the fog:
1. **A view mask texture**, an R8 raster of the areas that the terrain, flat-sea and wave shaders read by place, darkening towards black.
2. **The terrain's vertex colours** darkened on the CPU, its chunks uploaded again when the areas change.
3. **A full-screen pass** that darkens by the depth buffer's reconstructed place.

For what is undrawn:
4. **On the CPU**, before the batches are built.
5. **In the shaders**, by the same mask.

## Decision Outcome

Chosen options: 1 and 4.
- Option 2 uploads every terrain chunk again at every growth of the territory, and darkens the sea not at all.
- Option 3 adds a pass and darkens the public layer's lines with the ground under them.
- Option 5 would draw Workers and structures dimmed rather than not at all, and costs their pixel shaders a texture read.

* **The view mask** (`NeuronClient/WorldScene.h`, `ViewMask`), the engine's own idea:
  - a square raster of cells from its corner of least x and z, 255 where the ground is clear and 0 under fog, with values between at the edge;
  - and the fog's strength. `FrameInputs::viewMask` carries it, and none leaves the ground clear everywhere.
* **On the GPU** (`WorldRenderer`):
  - **Upload.** A new mask is uploaded into an R8 texture of its own, of one level. The mask shown stays until the new one's upload completes, and is then released after the frames in flight.
  - **Failures.** A creation or upload refused for want of staging memory or during a rebuild is tried again on a later frame. After a device rebuild, the mask shown is uploaded again, and the ground is clear until it completes.
  - **Ready.** The world's readiness does not wait for the mask, so a growing territory never blanks a frame.
* **The frame constants** (`ShaderLayouts.h`) gain two registers:
  - `viewMask`: the mask's corner relative to the camera, 1 ÷ its side in wu, and the fog's strength, 0 with no mask;
  - `viewMaskIndex`: its heap index.
* **The shaders.** The terrain's, the flat sea's and the waves' vertex shaders pass on the camera-relative place.
  - Each pixel shader reads the mask there with linear clamped filtering at its one level, and multiplies its fogged colour by 1 − strength × (1 − mask). The terrain and the flat sea are darkened towards black; the waves, which are added, are added the less.
  - The read is skipped while the strength is 0, which is the sandbox.
  - The three shaders compute it inline rather than share an include, since a shader's include serves its own stages and permutations (AGENTS.md §2).
* **The client's mask** (`GameClient/StewardAreas`, `BuildViewMask`):
  - The areas' cells, 255 in them and 0 outside, blurred by a box of `mvp.view_fog.edge_width` (64 wu, 5 cells) along the rows and then the columns. The fog's edge is a ramp centred on the areas' edge, and its last cell reads 112.
  - It is built again only when the areas' revision changes. Unknown areas give none.
  - The strength is `mvp.view_fog.strength` (0.85): the ground and the sea outside keep 15 % of their colour, so the terrain still reads under the fog, as WLD-244 draws it.
* **What is undrawn** (`WorldPresentation`, `EraseOutside`):
  - Workers, Leaders, orbs and structures whose place lies outside the areas are left out of the frame's batches, the resource sites among them.
  - The territory lines and border marks of the public layer are drawn in pass 10a after the terrain, with no fog of their own, so they lie on top of it.
  - Effects are not filtered: M2 has no battle outside the areas, and M3's battlefields join the areas (WLD-244).
* **The cost**, measured on a Linux build at `-O2` on the development container's Xeon at 2.8 GHz, best of 20 runs, for a territory disc of 1,400 wu: building the mask takes 0.39 ms, once for each change of the areas. Their rebuild takes 1.03 ms (ADR-142). The shaders' cost is unmeasured; T19's frame work measures the client on the reference machine.

### Consequences

* Good, because the settlement view shows the steward's areas clear and the rest of the valley dark, with the territory lines and border areas over it, and no Worker or structure beyond.
* Good, because the engine gains a mask over the ground that names nothing of the game, and that M3's battlefields only widen.
* Bad, because three shaders and the frame constants change with no local compile here; CI's build and render check are their first test.
* Bad, because the fog is the client's alone. The server still sends the whole region (ADR-111), so a modified client could show more, until M3's interest set narrows the replication to what the seen layer allows.
* Neutral, because the sky is not fogged: it lies above the ground and over the whole valley alike.
* It forecloses: the fog by vertex colours; Workers and structures dimmed rather than left out.

### Confirmation

* `GameClientTests`, `StewardAreasTests`:
  - the mask is none while the areas are unknown;
  - it has the areas' raster and the fog's strength, 255 inside, 0 well outside, and 112 at the areas' last cell along x;
  - a figure outside the areas is left out, and every figure stays while the areas are unknown.
* CI's render check of a settlement world on WARP (`Build\CheckWorldFrame.py --world`) draws the fogged frame with the debug layer silent.
* The owner's run on `Debug|ARM64`, in T13's checklist.

## Pros and Cons of the Options

### 1 — a view mask texture

* Good, because one small texture serves the terrain and both sea layers.
* Bad, because three shaders and the frame constants change.

### 2 — darkened vertex colours

* Good, because no shader changes.
* Bad, because every chunk is uploaded again as the territory grows, and the sea is untouched.

### 3 — a full-screen pass

* Good, because one pass fogs whatever lies on the ground.
* Bad, because it fogs the public layer too, and adds a pass and a depth read.

### 4 — undrawn on the CPU

* Good, because nothing outside costs the GPU anything.
* Bad, because each kind of object is filtered where its batches are built.

### 5 — undrawn in the shaders

* Good, because one rule fogs everything.
* Bad, because Workers would be dimmed, not absent.

## More Information

* Related: [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md), [ADR-114](ADR-114-gpu-resources-at-m1.md), [ADR-115](ADR-115-the-world-frame-at-m1.md), [ADR-117](ADR-117-render-check-on-warp.md), [ADR-142](ADR-142-the-camera-flies-along-the-view-within-the-stewards-areas.md), [ADR-143](ADR-143-territory-lines-and-border-areas-drawn-at-m2.md).
* The rules: WLD-242, WLD-244; MVP-084; RND-136, RND-143, RND-146, RND-222.
* Owner decision O90; DEC-1002.
* The code: `NeuronClient/WorldScene.h` (`ViewMask`), `ShaderLayouts.h`, `WorldRenderer`, `FrameRenderer`, `Shader/Terrain*`, `Shader/SeaFlat*`, `Shader/SeaWaves*`; `GameClient/StewardAreas`, `WorldPresentation`; the presentation record's `mvp.view_fog.*`.
* ADR-115 is amended with this record.
* **Amendment 2026-10-23 (M3 task T12, [ADR-167](ADR-167-the-conflict-verbs-and-markers-at-m3.md)).** The battlefields of the steward's battles join the areas, and the view mask is built again when they do, so a battle away is fought in the clear.
