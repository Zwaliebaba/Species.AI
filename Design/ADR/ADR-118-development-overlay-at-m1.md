---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T15a)
---

# The development overlay at M1: its text, the frame-rate line and the figure dots

## Context and Problem Statement

M1 task T15a is the text part of the development overlay (O75, M1 plan): EditorFont text at whole-number scales, the frame-rate line of DBG-095, and a dot for each figure until T16 draws the crowds. T15b brings the rest after T14's input router: the toggle, the frame page, the budget rows and the restart.

T15a needs four answers that the design leaves open:
- how a face's glyphs reach the screen before the UI framework exists;
- where the frame-rate counter's values and the overlay's anchor are read from;
- what "live simulated objects" counts on a client that holds a replica (ADR-084);
- what of it a Release build holds (DBG-004).

## Decision Drivers

* Text comes from the owner's bitmap faces at whole-number scales with point sampling, and nothing is rasterised at run time (O62, O63, ADR-034 as amended).
* No new dependency (AGENTS.md R14): no DirectWrite, no GDI text.
* The overlay's facility is absent from a Release binary (DBG-004, DBG-098).
* The owner's T15a run must show the figures and the frame rate without a toggle, since the input router arrives with T14.

## Considered Options

* 1 — a CPU layout of each line into instanced glyph quads over the face's one page, drawn in pass 29 on the back buffer; and screen-sized squares at the figures, depth-tested in the scene
* 2 — the UI framework's text system of ADR-034 (glyph cache, text styles), built now
* 3 — the development drawing queue of DBG-130 and DBG-131, with its 2-D squares and point markers

## Decision Outcome

Chosen option: "1".
- Option 2 brings the UI framework forward from the milestone that needs it, for one line of text.
- Option 3 builds a queue with labels, lives and prefix search, for dots that T16 replaces.

* **Text** (`NeuronClient/TextLayout`, `OverlayRenderer`).
  - A line is read as UTF-8; a malformed sequence gives U+FFFD.
  - A character the face does not hold is drawn as the face's missing glyph.
  - Each glyph is laid out at the scale *s* = max(1, round(display scale)) of O63, its pen and rectangle in whole pixels.
  - The face's page is one `R8_UNORM` texture from the GPU resource factory (ADR-114), uploaded once per face and again after a device rebuild (RND-035).
  - The glyphs are one instanced draw per frame (`TextGlyphInstance`, 48 bytes). They are read with the point-clamp sampler and alpha-blended by coverage, in pass 29 (`pass_29_text_overlays`) on the back buffer after the final pass, so brightness and fog never touch them.
  - A frame with no world draws the line too, over its background.
* **The frame-rate counter** (R-SIM-576) is the renderer's: the frames presented in the last `ref.sim.frame.fps_window` (1 s) of real time, divided by the window, and `ref.sim.frame.fps_initial` (60) until the first window has passed. The client reads both keys from the presentation record (ADR-116) and gives them to the renderer with `Reconfigure` once the record is loaded.
* **The frame-rate line** (DBG-095).
  - It is right-aligned with its cell's bottom-right corner at (W − `dbg.overlay.frame_rate_right` × *s*, H − `dbg.overlay.frame_rate_bottom` × *s*), white.
  - Its text is `ui.dev.overlay.frame_rate_entities`, "{fps} fps · {count} objects", while the client holds a world and a replicated tick, and `ui.dev.overlay.frame_rate`, "{fps} fps", otherwise. The frame rate is rounded to a whole number.
  - The two strings are literals in `DevelopmentOverlay.cpp` that cite their ids, until the string table is data.
  - The count is the size of the replica's latest view: the client's interest set of figures, effects and orbs (ADR-086). A server's object count is not on the wire, and the client holds no other.
* **Shown by default in a Debug build until T15b.** DBG-095 makes the line's visibility the overlay flag of RND-011, off by default. The flag's toggle is `action.debug.overlay`, which needs the input router of T14. Until T15b binds it, a Debug build shows the line on every frame; T15b turns the default off as DBG-095 states.
* **The figure dots** (`DevelopmentOverlay.cpp`, `development_dots` in the catalogue).
  - Every living figure of the replica gets one, at its interpolated position (ADR-084 item 4) lifted `DOT_LIFT_WU` (3 wu).
  - Its colour is its team's base colour, its colour id its team slot outside co-op and Siege (R-SIM-415, R-SIM-416); a slot beyond the eight player colours is white.
  - The eight base colours `ref.sim.team_color.<name>.base` are a constant table that cites its keys, not fields of the presentation record. A record field named `base` would generate a type named `Base`, which R2's checker refuses; this is ADR-116's rule for a key the generator cannot name.
  - A dot is a square of `WORKER_DOT_PIXELS` (3) or, for a Leader, `LEADER_DOT_PIXELS` (6) pixels times *s*.
  - The dots are opaque and depth-tested against the scene, so the terrain hides a dot behind a hill. They are drawn as one instanced draw in pass 24 (`pass_24_development_drawing`), after the coastline waves and before the resolve.
  - The pass is in the frame's table in Debug builds only, as RND-076 marks pass 24.
* **What a Release build holds.**
  - The overlay's facility is compiled out under `ENGINE_DEVELOPMENT`: the face's load, the line, the dots and pass 24.
  - NeuronClient's text drawing stays in both configurations: it is the renderer's text path for pass 29, which also draws the version string (RND-076), and the UI's text builds on it.
  - The dots' pipeline state stays in the catalogue unused, one pipeline state object.
* **The middle dot.** EditorFont holds U+0020 to U+007E, U+00A0 and U+00BF to U+00FF, so the separator U+00B7 of `ui.dev.overlay.frame_rate_entities` draws as the face's missing glyph "?" until the face holds it. Adding the glyph to EditorFont or changing the string is the owner's choice: the face is the owner's asset (O64).

### Consequences

* Good, because the owner's run of T15a shows the frame rate and every figure, and the render check of ADR-117 draws both passes on WARP.
* Good, because the text path is the one the UI's text of ADR-034 needs: a face's page, a whole-number layout and point sampling.
* Bad, because the overlay is visible in every Debug frame until T15b.
* Bad, because the strings are code until the string table is data, which is the case for the window title too.
* Neutral, because the object count is the client's interest set: with the M1 sandbox's one region and no fog of war it is every object of the region.

### Confirmation

* NeuronClientTests: the text scale, the UTF-8 reading, the whole-pixel layout and the missing glyph (`TextLayoutTests`).
* GameClientTests: the line's text and anchor, the team colours and the dots (`DevelopmentOverlayTests`).
* The render check of ADR-117 on WARP.
* The owner's run: the line at the bottom right and a dot on every figure, in its side's colour.

## Pros and Cons of the Options

### 1 — glyph quads and screen-sized squares

* Good, because it needs no framework that M1 does not otherwise build.
* Bad, because pass 29's text is drawn outside the UI framework until it exists.

### 2 — the UI framework's text system

* Good, because the overlay would draw through the system that every later text uses.
* Bad, because it pulls the UI framework's text styles and glyph cache into M1.

### 3 — the development drawing queue

* Good, because DBG-099's world layers will use it.
* Bad, because its labels, lives and look-ups serve no M1 need.

## More Information

* Related: [ADR-034](ADR-034-text-rendering-and-glyph-cache.md), [ADR-056](ADR-056-products-configurations-and-feature-flags.md), [ADR-113](ADR-113-the-frame-at-m1.md), [ADR-114](ADR-114-gpu-resources-at-m1.md), [ADR-116](ADR-116-presentation-tuning-record.md), [ADR-117](ADR-117-render-check-on-warp.md).
* The code: `NeuronClient/TextLayout`, `NeuronClient/OverlayRenderer`, the shaders `DevelopmentText` and `DevelopmentDot`, `NeuronClient/FrameRenderer`, `GameClient/DevelopmentOverlay`, `GameClient/WorldPresentation`.
