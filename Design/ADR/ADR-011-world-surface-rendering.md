---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# World-surface rendering: chunked terrain, single-pass wire overlay and GPU water

## Context and Problem Statement

The Species world surface is a flat-shaded height-field terrain with a glowing wire-grid overlay, surrounded and cut by animated water, under a sky drawn into a black, fogged world. The terrain and water fill most of the screen in every frame, and the wire overlay is a defining part of the look. At the strategic zoom level of a large world, far more terrain is in view than in a skirmish arena.

How terrain heights are *produced* (generated when a map loads or baked into the map asset), and how terrain streams for a large world, are deferred to the design phase (the terrain runtime model; see [the index](README.md#deferred-to-the-design-phase)). Whatever the answer, the renderer receives a height field and one colour per terrain facet for each terrain region and has to draw them efficiently, together with the water and the sky.

The problem is how the renderer draws the terrain, its wire overlay, the water and the sky.

## Decision Drivers

- The terrain is drawn from static GPU buffers and does not cost CPU time per frame.
- Only the visible part of the terrain is drawn, which matters more as worlds grow.
- The wire overlay looks exactly as specified, without a second full-screen-sized pass where one pass can do it.
- Water motion is computed on the GPU and costs no per-frame CPU rebuild.
- Independence from the deferred terrain runtime model: the renderer accepts any producer of height and colour data.

## Considered Options

1. **Chunked static terrain buffers** with per-chunk culling, and the **wire overlay as a grid term** in the terrain pixel shader (one pass).
2. Chunked static terrain buffers with the wire overlay drawn as a **second, depth-biased pass**.
3. A **continuous-LOD terrain** (geometry clipmaps or GPU tessellation) with the edge term in its pixel shader.

## Decision Outcome

Chosen option: **1, chunked static terrain with a single-pass grid term**, because it is the cheapest way to draw the terrain and its overlay at the reference look, and it leaves the choice of a level-of-detail scheme to the design phase's terrain runtime model.

- **Terrain buffers.** Each terrain region is split into chunks of a fixed grid size. Each chunk's vertices (position, facet colour, facet normal, grid coordinates and barycentric coordinates) and indices live in default-heap buffers created when the region's height and colour data become available (ADR-008). Each chunk carries a bounding box.
- **Culling.** Chunks are culled against the view frustum each frame, and the visible chunks are drawn with one instanced or indirect draw per pass.
- **Wire overlay.** The terrain pixel shader adds the wire-grid term to the surface, so surface and overlay are drawn in one pass. The term has two sources, selected by a permutation of the terrain shader (ADR-003): the **stylised grid texture**, mapped once onto each height-field cell so that its lines do not follow the triangle edges, which is the default (the reference look, DEC-326 option A); and the **edge term** computed from barycentric coordinates, which follows the triangle edges (DEC-326 option B). Either source is lit with the grid's own specular material, separate from the surface material (ADR-003). Where the look needs the overlay with a different blend from the surface, the renderer component may specify a second pass instead, as a documented exception in the pass table.
- **Height-field texture.** The renderer also keeps each region's heights in a single-channel float texture, which particles (ADR-010) and shadow projection (ADR-009) sample.
- **Water.** Wave heights and colours are computed in the water vertex shader from the presentation time and per-level wave parameters, over a static grid mesh; a compute pass that writes a displaced vertex buffer is allowed where the vertex-shader form proves too expensive. Flat water layers are one draw with two textures and an explicit combine mode (ADR-003). A planar water reflection is not part of the baseline.
- **Sky.** The sky and clouds are one draw with additive blending, with fog computed in the shader.
- **Level of detail.** The baseline draws chunks at full resolution. A level-of-detail scheme for the strategic zoom (coarser chunk meshes, or option 3) is chosen together with the terrain runtime model and recorded in a new ADR.

### Consequences

- Good, because the terrain costs no per-frame CPU work beyond chunk culling.
- Good, because one pass draws surface and overlay, halving terrain pixel work compared with a separate overlay pass.
- Good, because water animation runs entirely on the GPU.
- Good, because the renderer depends only on height and colour data, not on how they are produced.
- Bad, because full-resolution chunks are not enough for the large-world strategic zoom; that needs the follow-up decision.
- Bad, because barycentric attributes require the terrain mesh to be split per triangle, which increases the vertex count.

### Confirmation

- Golden-image tests compare the single-pass terrain and overlay with the reference images of the renderer component at several camera distances and angles, for both grid sources.
- A performance test on the minimum-capability GPU measures terrain and water GPU time for the largest skirmish map and for a synthetic large region.
- A culling test checks that chunks outside the frustum issue no draws.

## Pros and Cons of the Options

### 1. Chunked terrain with a single-pass grid term

- Good, because it is simple, cheap and exact for the look.
- Neutral, because LOD is left to a later decision.
- Bad, because chunk meshes are duplicated per triangle for barycentrics.

### 2. Chunked terrain with a second overlay pass

- Good, because the overlay can have its own blend state and the surface mesh can stay indexed.
- Bad, because the terrain is rasterised twice, and the overlay needs a depth bias to avoid fighting the surface.

### 3. Continuous-LOD terrain

- Good, because it scales to very large visible areas at the strategic zoom.
- Bad, because the edge term must stay stable while the mesh changes resolution, which is harder to make match the look.
- Bad, because it depends on the terrain runtime model, which is not yet decided.

## More Information

- Related: ADR-003 (grid term, edge mask and combine modes), ADR-008 (static buffers), ADR-009 (shadow projection), ADR-010 (particles on terrain).
- The terrain component (`architecture/components/terrain/README.md`) and the renderer component (`architecture/components/renderer/`) specify the terrain algorithm, the chunk size, the wire-grid parameters and the water parameters.
- **Amendment 2026-10-17.** The wire overlay is a grid term with two sources: the stylised grid texture is the default and the barycentric edge term is the alternative permutation, matching the renderer component (RND-135, RND-266) and DEC-326. The grid term has its own specular material (RND-233, RND-266). The terrain supplies one colour per facet (RND-134).
- **Amendment 2026-10-20 (M1 task T13, owner decision O75, [ADR-115](ADR-115-the-world-frame-at-m1.md)).** M1 culls the terrain's 1,024 wu chunks on the CPU and draws each visible chunk with its own draw, and evaluates the coastline waves on the CPU each frame into the upload ring, in place of the GPU culling, the single indirect draw and the compute pass. Each M1 form ends with the milestone whose world draws more than one region's terrain or sea, or earlier if a frame measurement shows it over its share; the `terrain_heights` texture comes with its first reader, the particles and shadows of M1 task T16.
