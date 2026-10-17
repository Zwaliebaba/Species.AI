---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Instanced crowds, sprite cards and shadows

## Context and Problem Statement

The defining image of Species is a crowd: hundreds to thousands of small figures (Workers, Leaders, Essence orbs, creatures), each drawn as a flat card that turns towards the camera and blends into a billboard with distance, together with sprites, lasers, glow quads and blob shadows. Buildings, turrets, trees and scenery repeat the same few meshes many times. In a large battle, the number of visible figures is far larger than the number of distinct things to draw.

Submitting each figure, sprite or shadow on its own makes the CPU the bottleneck at crowd scale. Species needs a crowd renderer whose CPU cost depends on the number of *kinds* of thing on screen, not on how many of each there are, and that keeps the card, billboard and shadow look exactly.

The problem is how crowds, sprites, repeated meshes and their shadows are submitted to the GPU.

## Decision Drivers

- CPU cost per frame proportional to the number of unit types and passes, not to the number of figures.
- The card-to-billboard look, distance scaling and the decal and flattened-sprite shadows are kept exactly (visual-language requirements of the renderer component).
- Works at the minimum capability of ADR-0002 (no mesh shaders required).
- Additive and darkening layers stay unsorted; only passes that need order are sorted.
- Team identity comes from data per instance, not from per-team copies of assets.

## Considered Options

1. **GPU-driven instancing**: per-type instance buffers, frustum and distance culling in a compute pass, and `ExecuteIndirect` draws; quads expanded in the vertex shader.
2. **CPU-culled instancing**: the CPU builds a visible-instance list per type and issues one instanced draw per type.
3. **Mesh-shader crowds**: amplification and mesh shaders expand and cull figures.
4. **One draw per figure.**

## Decision Outcome

Chosen option: **1, GPU-driven instancing**, because it removes per-figure CPU work entirely while running on every GPU that meets the minimum capability.

- **Instance data.** Each frame, the presentation view of every visible-candidate figure (position, facing, size, colour, team tint index, orientation mode, animation phase) is written to a per-type instance buffer in the upload ring (ADR-0008), or produced on the GPU where an effect owns it (ADR-0010).
- **Culling.** A compute pass culls each type's instances against the view frustum and the type's distance limits, compacts the survivors with wave operations, and writes the indirect draw arguments. One `ExecuteIndirect` call then draws each type in each pass that draws it.
- **Quad expansion.** Sprite cards, billboards, glow quads, laser and trail segments and shadow quads use one shared index buffer with the pattern 0, 1, 2, 0, 2, 3 and no per-vertex buffer: the vertex shader builds the four corners from the instance data and the instance's **orientation mode**. The modes are: camera-facing billboard; upright card turning about the vertical axis towards the camera; world-oriented quad; and segment aligned between two points. The card-to-billboard blend is computed per instance from camera distance in the vertex shader.
- **Repeated meshes.** Buildings, turrets, trees and scenery are static per-mesh buffers (ADR-0019) drawn instanced, with per-instance transforms and per-node transforms in structured buffers, and the team tint applied from a per-instance palette index (ADR-0020). Tree geometry is generated from its parameters when the map loads, as data, never as a by-product of drawing.
- **Shadows.** Blob shadows and flattened sprite shadows are instanced quads projected onto the terrain, drawn in a depth-biased darkening pass (ADR-0005). Shadow maps are not part of the baseline; adding them is a deliberate change of the look and needs an ADR.
- **Ordering.** Additive and darkening passes are drawn unsorted. A pass that the pass table marks as sorted (alpha-blended buildings) is sorted back to front on the GPU by a key written during culling, or on the CPU when its item count is small; the renderer component chooses per pass.
- Mesh shaders and amplification shaders may be added later as an optional fast path behind a capability check (ADR-0002), producing the same image; they are never the only path.

### Consequences

- Good, because a crowd of any size costs a fixed number of CPU calls per type and pass.
- Good, because the card, billboard and shadow techniques are each one shader path with a documented orientation mode.
- Good, because team colours need no per-team asset copies.
- Bad, because culling and draw-argument generation move onto the GPU, which makes debugging harder than CPU lists (mitigated by a development view that reads back the compacted counts).
- Bad, because writing per-type instance data every frame costs upload bandwidth proportional to the visible-candidate count.

### Confirmation

- A performance test renders a stress crowd (the population figures of the renderer component's budget) on the minimum-capability GPU and checks that CPU frame time does not grow with the number of figures of an existing type.
- Golden-image tests cover each orientation mode, the card-to-billboard transition at the boundary distances, and both shadow kinds.
- A culling test compares the GPU-compacted instance set with a CPU reference cull for recorded camera paths.

## Pros and Cons of the Options

### 1. GPU-driven instancing with `ExecuteIndirect`

- Good, because CPU cost is independent of crowd size.
- Good, because it runs at feature level 12_1.
- Bad, because the culling pass and indirect arguments add engine complexity.

### 2. CPU-culled instancing

- Good, because it is simple and easy to debug.
- Bad, because culling thousands of figures per frame on the CPU competes with the simulation and the rest of presentation for CPU time.

### 3. Mesh-shader crowds

- Good, because expansion and culling happen in one GPU stage with no intermediate buffers.
- Bad, because mesh shaders are not part of the minimum capability, so a second path would be needed anyway.

### 4. One draw per figure

- Good, because it is the simplest possible code.
- Bad, because draw-submission cost grows with every figure, which is the main rendering cost at crowd scale.

## More Information

- Technology baseline item 4.2 (compute culling plus `ExecuteIndirect` at 12_1; see [the index](README.md#technology-baseline)).
- Related: ADR-0002 (minimum capability), ADR-0005 (blend and depth-bias states), ADR-0006 (instance buffers through heap indices), ADR-0008 (upload ring), ADR-0010 (GPU particles), ADR-0019 (mesh nodes), ADR-0020 (team tint).
- The renderer component (`architecture/components/renderer/`) specifies the instance layout, the orientation modes, the card-to-billboard distances and the shadow parameters as visual-language requirements.
