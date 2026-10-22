---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T14)
---

# The DirectWrite text system at M2

## Context and Problem Statement

Owner decision O92 (2026-10-22) moves Species' text to DirectWrite now, which O56 had left for later:
- the game's text is set in Bahnschrift and rasterized in smooth grayscale at its final size;
- both bitmap faces, GameFont and EditorFont, are retired;
- the development overlay draws in Cascadia Mono, or Consolas where that is not installed.

[ADR-034](ADR-034-text-rendering-and-glyph-cache.md)'s option 1 is the specification. This record states how M2 builds it, between T14's second step ([ADR-146](ADR-146-the-string-table-and-its-formatting-at-m2.md)) and its third. It also settles what ADR-034 leaves open:
- **Fonts.** Where the fonts come from, now that no font file is delivered.
- **Upload.** How the atlas pages reach the GPU. The factory's copy queue writes beside the frames in flight ([ADR-078](ADR-078-gpu-resource-factory.md)), and a full cache clears a page and reuses its slots.
- **Size.** What replaces O63's whole-number scales, and how the DPI scale enters a text's size.
- **Overlay.** How the development overlay's text is drawn once EditorFont is gone.
- **Coverage.** How the content build checks the string tables' coverage without a glyph table.

## Decision Drivers

* No third-party code (AGENTS.md R14): DirectWrite is the operating system's, as ICU is (ADR-146).
* No font file in the content: O92 chose fonts that every supported Windows installs ([ADR-050](ADR-050-client-platform-and-minimum-os.md)).
* What the coverage check passes is what draws. A character outside a role's chain is a build error, never a fallback to an arbitrary system font (UI-199).
* No frame draws a glyph from a slot that another glyph has taken, whatever the copy queue's timing.
* The headless server links no DirectWrite (BLD-045).

## Considered Options

For the fonts:
1. **System font families by name**: each role has a chain, and the rest of the chain is the role's only fallback.
2. **A custom font collection** of font files delivered in the content, as ADR-034's base text states.

For the pages on the GPU:
3. **A new texture for each revision of a page**, drawn from once its upload completes.
4. **One texture per page**, written in place by the copy queue.

For the development overlay's text:
5. **The UI renderer's glyph batches**, in pass 29 after the overlay's rectangles.
6. **A text pipeline of the overlay's own** that reads the atlas.

## Decision Outcome

Chosen options: 1, 3 and 5.
- Option 2 needs a font file licensed or delivered, which O92 declined.
- Option 4 lets the copy queue write a texture that a frame in flight may be reading; it would need a fence wait every frame.
- Option 6 copies the UI's glyph pipelines for text that is drawn like the UI's.

* **Fonts and roles** (`GameShared/GameFonts.h`, `NeuronClient/TextSystem`):
  - The title, body and caption roles draw from Bahnschrift, then Segoe UI. The development role draws from Cascadia Mono, then Consolas.
  - A role's face is the first family of its chain that DirectWrite's system collection holds, at normal weight; a first family that is missing is logged at start.
  - A character the face lacks is drawn from the rest of the chain, through a fallback built with `IDWriteFontFallbackBuilder`. The system's fallback is never applied, because the MVP has no player-entered text (UI-199).
  - Every role is set at normal weight, and the spacing factors (UI-196) and the horizontal scale (UI-197) are 1.0: Bahnschrift keeps its own widths. Whether the reference's condensation returns is DEC-1003, which is Open.
* **Layout** (`TextSystem::Layout`):
  - Each text gets an `IDWriteTextLayout` in its role's format at its final size, with en-US's shaping rules (O91) and no wrapping. A text breaks into lines only at its own line breaks; UI-201's wrapping arrives with T14's layout step.
  - A renderer of the system's own (`IDWriteTextRenderer`) records each glyph run's face, size, glyph indices and pen positions, with the baseline snapped to whole pixels. It lives on the stack for one `Draw`, so its reference count owns nothing.
  - The measured width is the layout's, trailing spaces included (UI-200). A text's height is its lines', and its baseline is its first line's.
  - A laid-out text is kept by role, size and text for 120 frames, so static text is laid out once. A kept text is laid out again once the atlas has cleared a page since.
* **Rasterization and the atlas** (`NeuronClient/GlyphAtlas`):
  - A glyph not yet cached is rasterized by `IDWriteFactory2::CreateGlyphRunAnalysis`, and its 1×1 alpha texture holds the coverage. The settings are natural symmetric rendering, grid fitting on and grayscale antialiasing (O92), with the pen's fraction of a pixel in steps of 1 ÷ `ui.glyph.subpixel_steps` (4). ClearType is never used.
  - The key is the face, the glyph index, the size in sixteenths of a pixel and the sub-pixel step (UI-198). The horizontal scale is left out of the key, since it is 1.
  - There are at most `ui.glyph.page_count` (8) pages, each `ui.glyph.page_size` (1024) texels a side, with glyphs packed on shelves and a texel of space around each. When no page has room, the least recently used page is cleared and reused.
  - The atlas lives on the CPU. Each page carries a revision, raised on every change, and the revision of its last clear.
* **The pages on the GPU** (`NeuronClient/UiRenderer`):
  - When a page's revision is not the one on the GPU, the page gets a new texture through the factory, uploaded whole, and draws from it once the copy completes. The texture it replaces is released through the factory, which keeps it until no frame in flight reads it.
  - A glyph batch draws from its page's texture only when that texture is at least as new as the page's last clear; until then the batch waits. A texture newer than the last clear but older than the page lacks only the glyphs added since, which appear with the next upload a frame or two later.
  - After a device rebuild, every page is uploaded anew from the atlas, which keeps the coverage; no glyph is rasterized again (UI-198's base text has them rasterized again).
  - A glyph batch names its page in its `image` field, so glyphs of two pages make two batches (UI-179).
* **Sizes** (UI-171, UI-170):
  - A text's final size is its layout size × the text scale × the DPI scale, raised to the accessibility minimum. The text scale is 1 until T15's settings store holds it.
  - DirectWrite rasterizes the text at that final size, so text is never scaled as a bitmap and no whole-number scale remains.
  - `WorldPresentation::Frame` now receives the window's DPI scale, which the test page, the badge's count and the overlay take.
  - The badge's count is `ui.text.default_size` (12 px) in the body role.
* **The development overlay** (DBG-095, DBG-096):
  - Its lines are laid out in the development role at `dbg.overlay.text_size` (14 px, a new key) × the DPI scale, with their anchors × the DPI scale.
  - The UI renderer's alpha glyph pipeline draws them in pass 29, after the rectangles that `OverlayRenderer` draws at the DPI scale. The development dots' sides take the DPI scale too, in place of a whole-number scale.
  - The `development_text` pipeline and its pixel shader are removed, and `development_solid` is pass 29's representative pipeline. The rectangles' shaders are renamed `DevelopmentRectVS` and `DevelopmentRectPS`.
* **The content build** (`Tools/SpeciesContentBuild/FontCoverage`):
  - The en-US table's coverage (UI-199, UI-233) is checked against the chain of the player's roles in DirectWrite's system collection (`IDWriteFont::HasCharacter`). A character is covered when a family of the chain that the build machine holds has it, as the client's fallback draws it. CI's Windows image holds Segoe UI, as every supported Windows does.
  - `FontCoverage.cpp` is the one source outside NeuronClient and GameClient that links `dwrite.lib`. Rule 14 of `Build/CheckProjectFiles.py` names it as the one exception, and a fixture shows that any other tool source doing so is refused.
  - Removed with the bitmap faces:
    - the font packer and the cooked font format (`NeuronCore/FontAsset`);
    - the bitmap text layout (`NeuronClient/TextLayout`);
    - the glyph tables' schema `text.glyph_table` and its generated header;
    - `GameData/Fonts/`;
    - six glyph fixtures of `Build\CheckContentBuild.py`, and its fixture for an upper-cased form, since Segoe UI draws every capital of Latin-1.
  - The coverage fixture uses U+6F22, which no family of the chain draws. The content type `font` stays in the cooked index's list, unused, so the list keeps its numbering (AST-014).
  - AST-089's check that a glyph table names its face lapses with the faces. So do AST-088's checks of a face's sheet, while its coverage of the currency signs is met by the chain. The asset pipeline's specification is at its size limit, so it carries no amendment of its own; this record states it.
* **What waits:**
  - Rasterizing the language's common characters while loading (UI-198, UI-241). The first frame that shows a text rasterizes its glyphs; T14's run checks the frame time.
  - The roles' weights and sizes in the HUD (T16), and the horizontal scale and spacing factors (DEC-1003).
  - Inline objects for binding glyphs (UI-224, T15).
  - The Japanese and Korean companions, and player-entered text (UI-234): the MVP has neither.

### Consequences

* Good, because text is sharp at every size, shaped and kerned, and every Latin, Greek and Cyrillic character draws, with no font file delivered and nothing drawn by hand.
* Good, because the owner's strings may use £, typographic quotes, dashes and the ellipsis, which GameFont lacked.
* Good, because no glyph is drawn from a slot that another glyph took, whatever the copy queue's timing.
* Bad, because the client depends on the fonts Windows installs: a Windows without Bahnschrift draws Segoe UI, and the look follows Windows servicing.
* Bad, because rasterizing a new glyph costs time on the frame thread at its first use, and each new revision of a page costs a whole page's upload (1 MiB).
* Bad, because the content build's check depends on the fonts of the machine that runs it: a machine without Bahnschrift checks against Segoe UI alone.
* It forecloses: authored bitmap faces as the text path, whole-number text scales and the cooked font format.

### Confirmation

* `NeuronClientTests`:
  - `GlyphAtlasTests`: a glyph's slot with its border; the shelves and the pages; an oversized and an empty glyph; and the least recently used page cleared, with the revision of its clear kept.
  - `TextSystemTests`, on DirectWrite:
    - a text laid out in its role, every glyph on whole pixels and rasterized once;
    - kept glyphs reused, and the final size rasterized rather than scaled;
    - a line's metrics;
    - a character drawn by the chain;
    - a role without a family drawing nothing.
  - `UiTextTests`: the final size with the DPI scale and the minimum; glyph pages starting batches; the outline pass; and the overlay's lines anchored at the DPI scale.
* `GameClientTests`, `UiLookTests`: the default size at the minimum and the DPI scale, the badge's count, and the test page drawing every UI pipeline inside its client area.
* `Build\CheckContentBuild.py`: a string holding U+6F22 is refused; 29 fixtures in all.
* `Build\CheckProjectFiles.py`: DirectWrite linked by another tool source is refused.
* CI's render check of the settlement world draws the test page on WARP through DirectWrite.
* The owner's run on `Debug|ARM64`, in T14's checklist, covers:
  - the test page of `--ui-test`, with Bahnschrift from 12 to 32 px and the fallback line;
  - the badge's count;
  - the overlay (F5) in Cascadia Mono.

## Pros and Cons of the Options

### 1 — system font families by name

* Good, because nothing is delivered or licensed, and DirectWrite's system collection is the client's and the content build's alike.
* Bad, because the families are Windows', so their look and coverage can change with Windows servicing.

### 2 — a custom font collection

* Good, because the look is fixed by the files the game delivers.
* Bad, because a font file must be chosen, licensed or delivered under its license, which O92 declined.

### 3 — a new texture for each revision

* Good, because the copy queue never writes a texture a frame may read.
* Bad, because each revision uploads a whole page and creates a texture.

### 4 — one texture per page, written in place

* Good, because one texture and one upload per change.
* Bad, because the write races the frames in flight unless the frame waits for the copy.

### 5 — the UI renderer's glyph batches

* Good, because the overlay's text is drawn by the same pipelines and atlas as the UI's.
* Bad, because pass 29 binds the UI's pipelines as well as the overlay's.

### 6 — a text pipeline of the overlay's own

* Good, because the overlay owns its pass.
* Bad, because it repeats the UI's glyph pipelines.

## More Information

* Related: [ADR-034](ADR-034-text-rendering-and-glyph-cache.md), whose option 1 this builds; [ADR-050](ADR-050-client-platform-and-minimum-os.md), [ADR-078](ADR-078-gpu-resource-factory.md), [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md), [ADR-118](ADR-118-development-overlay-at-m1.md), [ADR-120](ADR-120-frame-page-toggle-and-sandbox-restart-at-m1.md), [ADR-145](ADR-145-the-ui-pass-and-gamefont-text-at-m2.md), [ADR-146](ADR-146-the-string-table-and-its-formatting-at-m2.md).
* The rules: UI-170, UI-171, UI-195 to UI-202, UI-224, UI-233, UI-234, UI-241; DBG-095, DBG-096; DAT-219; AST-088, AST-089; BLD-045.
* Owner decisions O56, O62, O63, O91 and O92; DEC-1003.
* The code:
  - `NeuronClient/TextSystem`, `GlyphAtlas`, `UiText`, `UiRenderer`, `OverlayRenderer`, `FrameRenderer`, `WorldScene.h`, `Shader/DevelopmentRect*`;
  - `GameShared/GameFonts.h`;
  - `GameClient/UiLook`, `WorldPresentation`, `DevelopmentOverlay`;
  - `Tools/SpeciesContentBuild/FontCoverage`, `StringTableCooker`, `ContentBuild`;
  - `Build/CheckProjectFiles.py`, `Build/CheckContentBuild.py`.
* ADR-034, ADR-108, ADR-118, ADR-120, ADR-145 and ADR-146 are amended with this record.
