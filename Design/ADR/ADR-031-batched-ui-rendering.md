---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Batched UI rendering

## Context and Problem Statement

The Species UI draws panels, buttons, icons, lines, bars and text over the 3-D scene, with three blend modes that are part of the look: **alpha** for panels, **additive** for glowing elements and normal text (text brightens what is behind it), and **ink**, a darkening mode used for outlines, shadows and dark panels. Clipped scroll areas need clipping rectangles, and page transitions slide the previous page's image away, which needs that image as a texture. A busy screen (a lobby, an options page, the match HUD with power slots and objectives) holds hundreds of elements, most of which change little from frame to frame, but some of which (timers, counters, the cursor) change every frame.

The problem is how the UI framework's output (ADR-030) is turned into GPU work.

## Decision Drivers

- A handful of draw calls per frame for the whole UI, whatever the number of elements.
- The three UI blend modes reproduced exactly, in painter's order.
- Clipping by rectangle, nested.
- Captured page images for transitions, through the frame graph.
- Crisp, pixel-aligned UI at the window's native resolution, independent of the scene's render scale and anti-aliasing.
- Simple: no per-widget GPU resources to manage.

## Considered Options

1. **One batched dynamic vertex stream per frame**, with three blend pipeline states, a scissor stack and batches merged in painter's order.
2. **Retained GPU buffers per widget**, updated only when a widget changes.
3. **One draw call per widget.**

## Decision Outcome

Chosen option: **1, one batched dynamic vertex stream per frame**, because the whole UI then costs one upload and a few draws, and there is no per-widget GPU state to keep consistent.

- **Stream.** Each frame, the UI framework emits every visible quad, line and glyph quad into one vertex stream in the upload ring (ADR-008), in painter's order. Each vertex carries position (in pixels), texture coordinates, colour, and the atlas or texture index.
- **Batches.** Consecutive items that share blend mode, texture or atlas page and clipping rectangle form one batch; a change of any of them starts a new batch. Order is never changed for batching, so blending stays correct.
- **Blend states.** Three UI pipeline states from the catalogue (ADR-005): **alpha** (`SRC_ALPHA`, `INV_SRC_ALPHA`), **additive** (`SRC_ALPHA`, `ONE`) and **ink** (`SRC_ALPHA`, `INV_SRC_COLOR`). Glyph coverage from the glyph cache (ADR-034) multiplies the item's colour and alpha, so text uses the same three states: normal text additive, outlines and shadows ink. An ink item with source alpha 0 has the arithmetic of the catalogue's darkening state, and the renderer may draw it with that state.
- **Clipping.** The framework keeps a stack of clipping rectangles; each batch carries the intersection of the stack, applied as the scissor rectangle.
- **Captured images.** A page transition samples a declared frame-graph target that holds the previous page's image (ADR-004); the UI never reads the back buffer in place.
- **Placement.** The UI pass runs after the scene's resolve, tone mapping and brightness passes, at the window's native resolution; with HDR on, at the SDR reference white level (ADR-014). UI geometry is snapped to whole pixels.
- **World-space text and markers.** Labels and markers that live in the world are drawn in the world passes of the pass table with the same glyph quads and blend states, depth-tested or not as their pass specifies.

### Consequences

- Good, because the UI's GPU cost is one upload and a small, bounded number of draws.
- Good, because the blend modes and painter's order are exact.
- Good, because the UI is always sharp, whatever the scene's render scale.
- Bad, because the whole UI stream is rebuilt every frame, which costs CPU time proportional to the visible element count (small for Species screens).
- Bad, because a screen that alternates blend modes element by element produces many small batches; the UI component's recipes order elements to avoid that.

### Confirmation

- Golden-image tests render reference screens (a page with alpha panels, additive text, ink outlines, clipped scroll lists and a page transition) and compare them with reference images.
- A performance test renders the busiest screen and checks its draw count and CPU time against the UI component's budget.
- A test changes the render-resolution scale and anti-aliasing setting and checks that UI images are unchanged.

## Pros and Cons of the Options

### 1. One batched stream per frame

- Good, because it is simple, cheap on the GPU and has no stale state.
- Bad, because the CPU rebuilds the stream every frame.

### 2. Retained per-widget GPU buffers

- Good, because unchanged widgets cost nothing to re-emit.
- Bad, because every widget owns GPU memory whose lifetime and updates must be managed, and draw order across widgets still has to be resolved every frame.

### 3. One draw call per widget

- Good, because each widget is independent.
- Bad, because draw count grows with the element count, and state changes between draws dominate the UI's cost.

## More Information

- Related: ADR-004 (captured targets), ADR-005 (UI blend states), ADR-008 (upload ring), ADR-014 (HDR and SDR white), ADR-030 (UI framework), ADR-033 (layout and pixel scale), ADR-034 (glyph cache).
- The UI framework component (`architecture/components/ui_framework/`) specifies the vertex layout, the batching rules and the screen recipes.
- **Amendment 2026-10-17.** The ink state with source alpha 0 equals the darkening state of ADR-005 (UI-180, RND-058). The UI budget of the performance test is the UI framework's: 32 draws, 65 536 vertices and 1.0 ms of main-thread time on the busiest screen (UI-240, `ui.budget.*`).
- **Amendment 2026-10-22 ([ADR-145](ADR-145-the-ui-pass-and-gamefont-text-at-m2.md)).** The stream is of instanced quads: each primitive is a 48-byte instance holding its rectangle, its texture coordinates and its colour, and each batch is one instanced draw whose texture's heap index is in its draw constants. The colour, the batching, the three blend states, the scissor and the placement are as above. UI-178 is amended, and UI-240's budget in vertices counts six per quad.
