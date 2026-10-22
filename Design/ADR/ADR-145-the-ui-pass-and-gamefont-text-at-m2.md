---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T14)
---

# The UI pass and GameFont's text at M2

## Context and Problem Statement

M2 task T14 builds the UI framework at the MVP's size (ADR-030 to ADR-034):
- the batched UI pass;
- GameFont's text at whole-number scales with point sampling (O62, O63);
- the two panel strips;
- the en-GB string table;
- the page stack, focus, layout and DPI scaling;
- the badge's count of selected Workers drawn as UI text, in place of the second overlay renderer of [ADR-127](ADR-127-left-button-verbs-and-order-feedback-at-m1.md).

It lands in three steps. This record is the first: the pass, the text and the strips, the badge's count, and a Debug test page that draws them. The string table is the second step and the page stack, focus, layout and DPI the third, each with its own record.

The frame graph so far draws in pass 16 the world's markers and in pass 20 the badge and the cursor, both from instanced quads of the `TextGlyph` layout over the back buffer (ADR-119, ADR-125). The development overlay draws EditorFont in pass 29 (ADR-118). ADR-031 asks for one batched vertex stream per frame in three blend states, and UI-178 states its vertex: 24 bytes, with a colour of 8 bits per channel and a texture index per vertex.

These decisions are open:
- the form in which the UI's primitives reach the GPU;
- which samplers they read with;
- the passes they are drawn in;
- the scale of player-facing text before the settings store exists;
- what the test page draws.

## Decision Drivers

* The engine knows no game concept (AGENTS.md R9): the UI's drawing is the engine's, and what it draws is the game's.
* The three blend states of UI-180, exact and in painter's order (UI-179).
* GameFont crisp at every scale: whole-number scales and point sampling (O63, UI-198).
* No new pass for the badge, which sits between the badge's icon and the pointer (R-CTRL-366).
* Every value in data (ADR-116), in the presentation record, which the server never reads.
* CI's render check sees every pipeline the UI adds (ADR-117).

## Considered Options

1. **Instanced quads**: each primitive a 48-byte instance of the `TextGlyph` layout (its rectangle, its texture coordinates and its colour, each four floats), and each batch one instanced draw with its texture's heap index in the draw constants.
2. **UI-178's vertex stream**: six 24-byte vertices per quad with a packed colour and a texture index per vertex, under a new input layout and vertex shader.
3. **The development overlay's renderer** widened to draw the UI.

## Decision Outcome

Chosen option: 1.
- Option 2 adds an input layout and a vertex shader, and moves 144 bytes per quad where option 1 moves 48. Its texture index per vertex buys nothing, since UI-179 starts a batch at each change of texture anyway.
- Option 3 joins the development overlay, which a Release build does not draw (DBG-098), to the player's UI, and the overlay knows one blend.

* **The UI's types** (`NeuronClient/WorldScene.h`):
  - `UiQuad`: a rectangle in pixels from the target's top-left corner, its texture coordinates and its colour.
  - `UiBatch`: a source, a blend, an image index, an optional scissor (UI-181) and its quads in painter's order.
  - The source is one of three:
    - `Solid`: the quad's colour;
    - `Glyphs`: the UI face's coverage page, read with the point sampler (O63);
    - `Image`: an image of the UI's set, read with the linear wrapping sampler (UI-054).
  - The blend is `Alpha`, `Additive` or `Ink` (UI-180).
  - `FrameInputs` carries the UI's face, its images, the batches of pass 19 (`ui`) and those of pass 20 (`uiCursor`).
* **The catalogue** gains `BlendState::Ink` (source alpha, one minus source colour) and seven pipeline states, `ui_solid_*`, `ui_glyphs_*` and `ui_image_*`. Solids and images draw alpha and additive, and glyphs draw all three. None tests or writes depth (UI-183).
  - The glyphs' pixel shader multiplies the colour and the alpha by the coverage (UI-180), so an ink glyph darkens only where it covers.
  - An image's colour is its texel times the quad's.
* **The drawing** (`UiRenderer`):
  - It holds the face's page, an R8 texture, and each image of the set, uploaded through the GPU resource factory once and again after a device rebuild (RND-034).
  - A batch whose texture is not on the GPU yet is not drawn.
  - Each batch is one instanced draw of its quads from the upload ring. A batch with a scissor sets it, and the whole target is set again after it.
* **The passes.**
  - Pass 19 (`pass_19_ui`) draws the `ui` batches over the final image and the markers of pass 16 (UI-008, UI-175).
  - Pass 20 draws the badge's icon, then the `uiCursor` batches, then the pointer.
  - Without a world, both lists are drawn over the background, as the cursor is.
* **The text** (`NeuronClient/UiText`):
  - `MinimumTextPixels` is UI-171's accessibility minimum for the client height.
  - `TextScaleForSize` is UI-198's whole-number scale as O63 amends it: ⌊s ÷ d + 0.5⌋, at least 1, raised until k × d reaches the minimum.
  - `AppendText` lays a line out with its corner on a whole pixel. With an outline, it emits UI-192's eight copies at ± the offset in the ink state before the fill.
  - `AppendQuad` joins a quad to the last batch only when its source, blend and image match and that batch has no scissor (UI-179).
  - The text scale and the UI scale are 1 until the settings store holds them (T15). The outline's offset is not yet multiplied by the DPI scale (UI-169), which arrives with the third step's layout; at a DPI scale of 1 the two agree.
* **The keys.** The presentation record gains the UI framework's keys that this step reads:
  - `ui.text.default_size` (12 px) and `ui.text.outline_offset` (1 px);
  - `ui.access.min_text_1080p` (9 px) and `ui.access.min_text_2160p` (19 px);
  - `ui.chrome.body_repeat` (8);
  - `ui.palette.window_body_tint` (1, 1, 1, 0.96) and `ui.palette.text_outline` (1, 1, 1, 0).

  `ui.text.outline_copies` is not made a key: UI-192 lists its eight positions, so no other value has a meaning.
* **The badge's count** (R-CTRL-366, `GameClient/UiLook`):
  - It moves into pass 20's UI batches, white and additive (UI-190), at the scale of `ui.text.default_size`. With GameFont's 16 px, that is 16 px up to a client height of 1,836 px and 32 px above it.
  - Its line's left edge is at the right edge of the badge's shadow, and its cell's middle level with the badge's centre.
  - ADR-127's second overlay renderer and its `OverlayCorner::PixelLeftMiddle` anchor are retired.
* **The test page** (`--ui-test`, a Debug build's start-up option; `AppendUiTestPage`), drawn in pass 19 every frame:
  - the accent strip as a window body (UI-054) with the neutral strip beside it;
  - GameFont outlined at the scales 1 to 4;
  - a line each filled additive, outlined and filled, inked alone and laid over;
  - a line of characters GameFont lacks, drawn as its missing glyph;
  - a solid and an image swatch in each blend they draw in.

  Every UI pipeline therefore draws. `Build\CheckWorldFrame.py --ui-test` passes it, and CI's settlement-world render check draws it on WARP with the debug layer.

### Consequences

* Good, because the UI draws in the three blend states, in painter's order, with GameFont at whole-number scales from its one page.
* Good, because the badge's count is player-facing text drawn as the UI's, and the second overlay renderer is gone.
* Good, because one input layout and one vertex shader serve the cursor, the overlay and the UI.
* Bad, because UI-178's vertex no longer describes the stream: an instance is twice a vertex's size but a third of a quad's six vertices. UI-178 is amended, and UI-240's budget in vertices counts six per quad.
* Bad, because the UI's batches are built and uploaded every frame, as ADR-031 accepts, and the shaders and the pass change with no local compile. CI's build and render check are their first test.
* Neutral, because UI-184's gradients and lines arrive with the third step's widgets: a gradient as two colours on an instance, a line as a quad.
* It forecloses: UI-178's vertex stream; drawing player-facing text with the development overlay.

### Confirmation

* `NeuronClientTests`, `UiTextTests`:
  - UI-171's minimum at and between the two reference heights;
  - UI-198's scale, raised to the minimum;
  - UI-179's batching, which never reorders;
  - UI-192's eight ink copies before the fill;
  - the missing glyph.
* `GameClientTests`, `UiLookTests`:
  - the default text's scale by client height;
  - the badge's count;
  - the test page drawing every pipeline inside a 1280 × 720 client area, with UI-054's body first.
* `NeuronCoreTests`, `StartupOptionsTests`: `--ui-test`.
* CI's render check of a settlement world with `--ui-test` on WARP, with the debug layer silent.
* The owner's run on `Debug|ARM64`, in T14's checklist: GameFont crisp at each scale on the test page.

## Pros and Cons of the Options

### 1 — instanced quads

* Good, because the layout, the vertex shader's shape and the upload already serve the cursor and the overlay.
* Bad, because UI-178 is amended.

### 2 — UI-178's vertex stream

* Good, because it is the reference library's form.
* Bad, because it adds a layout and a shader and moves three times the bytes per quad.

### 3 — the overlay's renderer

* Good, because it already draws text.
* Bad, because it joins a Debug tool to the player's UI and has one blend.

## More Information

* Related: [ADR-031](ADR-031-batched-ui-rendering.md), [ADR-034](ADR-034-text-rendering-and-glyph-cache.md), [ADR-114](ADR-114-gpu-resources-at-m1.md), [ADR-117](ADR-117-render-check-on-warp.md), [ADR-118](ADR-118-development-overlay-at-m1.md), [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md), [ADR-127](ADR-127-left-button-verbs-and-order-feedback-at-m1.md).
* The rules: UI-008, UI-054, UI-169, UI-171, UI-175, UI-178 to UI-184, UI-189 to UI-192, UI-198; R-CTRL-366; MVP-100.
* Owner decisions O62, O63.
* The code:
  - `NeuronClient/WorldScene.h` (`UiQuad`, `UiBatch`, `UiImages`), `UiText`, `UiRenderer`, `PipelineCatalog` (`BlendState::Ink`), `FrameRenderer`, `Shader/Ui*`;
  - `GameClient/UiLook`, `WorldPresentation`, `SettlementView`, `WorldMarkers`;
  - `NeuronCore/StartupOptions` (`--ui-test`);
  - the presentation record's `ui.*`.
* ADR-031 and ADR-127 are amended with this record.
