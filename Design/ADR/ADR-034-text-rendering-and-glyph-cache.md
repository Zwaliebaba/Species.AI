---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Text rendering: authored bitmap glyph atlases, with DirectWrite shaping and a glyph cache as the upgrade

## Context and Problem Statement

Species text appears in menus, the match HUD, captions, chat, player and map names, and labels in the world. The supported languages need, at least, Latin with its European extensions, Cyrillic, the currency-symbol block (U+20A0–U+20AF), CJK punctuation and kana, CJK ideographs, and Hangul; player-entered text can contain any Unicode character, including characters outside the Basic Multilingual Plane. The text look is part of the visual language: three font roles (captions, body text, titles), a **horizontal scale per language** that makes glyphs narrower or wider relative to their height (Latin and Cyrillic text is drawn condensed, and Latin characters stay condensed inside CJK text), additive text, and outlines and shadows drawn in the darkening ink mode (ADR-031). Text must be shaped, kerned and laid out bidirectionally where a script needs it, measured exactly for layout, and sharp at every pixel size that DPI and the text-scale setting produce (ADR-033). Fonts are newly licensed for embedding.

The problem is how text is shaped, laid out and rasterised, and how glyphs reach the GPU.

## Decision Drivers

- Full Unicode text with shaping, kerning, bidirectional layout, line breaking and font fallback.
- Script coverage for every supported language from bundled fonts; system fonts only as a fallback for player-entered text.
- Sharp text at the exact final pixel size; no bitmap scaling.
- The per-language horizontal scale, the font roles, additive text and ink outlines reproduced as specified.
- Bounded GPU memory for glyphs; only glyphs in use are rasterised.
- An in-box, maintained text stack on Windows; the dedicated server never renders text.

## Considered Options

1. **DirectWrite** for layout and shaping, with a **raster glyph cache**: glyphs rasterised by DirectWrite at their final size into `R8_UNORM` atlas pages.
2. DirectWrite for layout, with a **signed-distance-field atlas** built from glyph outlines for all text.
3. **FreeType and HarfBuzz** (with separate bidirectional and line-breaking libraries) and a raster glyph cache.
4. **Offline-baked glyph atlases** per font, size and script. (Since the amendment of 2026-10-19, in an authored form, the default of this record.)

## Decision Outcome

Proposed option (amended 2026-10-19, owner decision O56; see the amendment of that date under [More Information](#more-information)): **4, offline-authored glyph atlases** of two bitmap faces that the owner draws, with option **1, DirectWrite layout and a raster glyph cache**, as the recorded upgrade. Until that amendment the proposed option was 1. **Default while Proposed** (amended 2026-10-19): the text stack is built as the amendment of 2026-10-19 states. The default of 2026-10-17, which now describes the upgrade and is not built until the amendment's trigger is met, read: the text stack is built this way, because DirectWrite is in-box and covers shaping, bidirectional text, line breaking and fallback, and raster glyphs at the final size give the sharpest text. **Confirmed when** (amended 2026-10-19): the checks that the amendment of 2026-10-19 gives the Confirmation section pass on the screens of the first playable, the MVP's M3 build (amendment of 2026-10-18); they are checked before the first screen is built on the glyph atlases. The line of 2026-10-17, which now confirms the upgrade when it is built, read: the text, coverage, scale and cache tests of the Confirmation section pass for the base language and for the bidirectional and CJK samples on the screens of the first playable (O20); it is checked before the first screen is built on the glyph cache.

*The bullets below, the Consequences and the Confirmation section specify option 1. Since the amendment of 2026-10-19 they are the specification of the recorded upgrade, not of the default.*

- **Fonts.** The game's fonts are loaded from its packages into a custom font collection. Each font role (caption, body, title) maps, per language, to a font family and a fallback chain built with `IDWriteFontFallbackBuilder`, so every script of every supported language is covered by bundled fonts. The system font fallback (`IDWriteFactory2::GetSystemFontFallback`) is appended only for player-entered text.
- **Layout.** `IDWriteTextLayout` shapes, kerns, breaks lines and orders bidirectional runs. A custom `IDWriteTextRenderer` receives positioned glyph runs in `DrawGlyphRun` and turns them into glyph quads for the batched UI stream (ADR-031). Inline icons (for example the glyph of a bound key in a caption) are DirectWrite inline objects. Measured widths come from the layout, never from a per-character estimate.
- **Horizontal scale.** Each text style carries a horizontal scale factor, set per language and font role in the UI component's tuning. It is applied as a horizontal transform on the glyph run when it is laid out and rasterised, so advances and glyph shapes agree. A per-script override keeps Latin characters at the Latin scale inside text of another script, as the visual language specifies.
- **Rasterisation.** For each glyph not yet in the cache, `IDWriteFactory3::CreateGlyphRunAnalysis` runs with a natural-symmetric raster mode, the run's transform and `DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE`; `CreateAlphaTexture` writes the coverage into a staging buffer; one channel is copied into an `R8_UNORM` atlas page through the upload ring (ADR-008). A unit test confirms that the channels returned in grayscale mode are equal. ClearType is not used.
- **Cache.** The cache key is: font face, glyph index, final pixel size, horizontal scale, and subpixel x-offset quantised to a quarter pixel. Atlas pages have a fixed size, 1024 × 1024 `R8_UNORM` pixels, and at most 8 pages exist (UI framework tuning values); when the cache is full, the least recently used page is cleared and reused. Glyphs for the current language's common characters are rasterised during loading. Atlas pages are never block-compressed (ADR-017), and are rebuilt rather than restored after device removal (ADR-008).
- **Blending and effects.** Glyph coverage multiplies the text colour; normal text draws additive, and outlines and shadows draw extra offset glyph quads in the ink state, as the visual-language requirements specify.
- **World-space labels.** Labels in the world use the same cache at the pixel size they cover. A signed-distance-field atlas built from `GetGlyphRunOutline` (option 2) is an allowed later addition for labels at the strategic zoom, by a revision of this ADR.
- The dedicated server does not link DirectWrite.

### Consequences

- Good, because shaping, bidirectional text, line breaking and font fallback come from the OS, for every script.
- Good, because text is rasterised at its final size, so it is sharp at every DPI, text scale and horizontal scale.
- Good, because glyph memory is bounded and holds only glyphs in use.
- Good, because characters outside the Basic Multilingual Plane render like any other.
- Bad, because rasterising new glyphs costs CPU time at first use, which is visible as a short delay when a new language or size first appears (mitigated by pre-rasterising common glyphs during loading).
- Bad, because each distinct pixel size needs its own glyphs, so animated text scaling costs cache space.

### Confirmation

Amended 2026-10-19: the four checks below confirm the DirectWrite upgrade when it is built; the default is confirmed by the checks that the amendment of 2026-10-19 under More Information names.

- Text tests lay out and render reference strings in every supported language, including bidirectional and astral-plane samples, and compare them with golden images.
- A coverage check walks every string table of every supported language and fails when a character is not covered by that language's bundled font chain.
- A unit test checks that a style's horizontal scale changes both advances and rasterised glyph width, and that Latin characters keep the Latin scale inside CJK text.
- A cache test fills the atlas beyond capacity and checks LRU page reuse with no visible corruption.

## Pros and Cons of the Options

### 1. DirectWrite layout and raster glyph cache

- Good, because it is in-box and complete for every script.
- Good, because raster glyphs at the final size are the sharpest option for UI text.
- Bad, because glyphs are rasterised at run time.

### 2. Signed-distance-field atlas

- Good, because one atlas serves every size, which suits zooming world labels.
- Bad, because small UI text is softer than raster text, and thin strokes in CJK glyphs degrade.

### 3. FreeType and HarfBuzz

- Good, because they are portable and widely used.
- Bad, because bidirectional layout, line breaking and fallback need further libraries and integration, which DirectWrite already provides on Species' only client platform.

### 4. Offline-baked glyph atlases

- Good, because there is no run-time rasterisation.
- Bad, because every size, scale and script must be baked in advance, which costs memory for CJK and cannot follow arbitrary DPI and text-scale settings, and player-entered text can contain characters that were never baked.

## More Information

- Technology baseline item 9 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://learn.microsoft.com/windows/win32/api/dwrite_3/nf-dwrite_3-idwritefactory3-createglyphrunanalysis · https://learn.microsoft.com/windows/win32/api/dwrite/nf-dwrite-idwriteglyphrunanalysis-createalphatexture · https://learn.microsoft.com/windows/win32/api/dwrite_1/ne-dwrite_1-dwrite_text_antialias_mode · https://learn.microsoft.com/windows/win32/directwrite/glyphs-and-glyph-runs · https://learn.microsoft.com/windows/win32/api/dwrite_2/nn-dwrite_2-idwritefactory2.
- Related: ADR-008 (upload ring, device removal), ADR-017 (atlases never compressed), ADR-031 (glyph quads and blend states), ADR-033 (final pixel size), ADR-063 (string tables and language tags), ADR-064 (text scale and minimum sizes).
- The UI framework component (`architecture/components/ui_framework/`) specifies the font roles, the per-language horizontal scales and the text styles; the glyph renderer (`GlyphRenderer`) implements this decision.
- **Amendment 2026-10-17.** The atlas page size and page count are the UI framework's baseline values `ui.glyph.page_size` (1024 × 1024) and `ui.glyph.page_count` (8) (UI-198).
- **Amendment 2026-10-18 (owner decision O55).** Since O55 the first playable of the "Confirmed when" line is the MVP's M3 build ([MVP scope](../implementation/mvp_scope.md) §3, §5.3), not the skirmish of O20, and the evidence is gathered on that build: the screens are its front-end page and HUD (MVP-102, MVP-103), and since the MVP has British English only (MVP scope §5.1), the bidirectional and CJK samples are test strings shown on those screens.
- **Amendment 2026-10-19 (owner decision O56).** Species draws its text from **two bitmap faces that the owner draws for Species**, a title face and a body face, in the condensed style of the Species look (UI-197); the reference game's fonts are not used (O9), and no licensed typeface is needed. **Option 4, offline-authored glyph atlases, is the default of this record**, in an authored form: the glyphs are drawn by hand as bitmaps at two sizes instead of being baked from an outline font. Its costs in the list of options (sizes and scripts fixed in advance, nothing drawn for a character that was never drawn) are accepted, because the MVP has British English only and no player-entered text (MVP scope §5.1). Of the decision drivers, full Unicode text, script coverage beyond Latin and font fallback wait for the upgrade, and the owner accepts scaling between two drawn sizes in place of rasterising at the exact final size. **Option 1, DirectWrite with a raster glyph cache, is the recorded upgrade**, and the Decision Outcome's bullets, the Consequences and the Confirmation section stay its specification. The record stays Proposed. The title states the amended decision; the file name keeps its slug, because ids and file names are stable. The values below are defaults that the owner may change.
  - **Faces and roles.** The three font roles of UI-195 map onto two faces: Title uses `font.ui.primary`; Body and Caption use `font.ui.compact`. `font.ui.stylised` is an optional later third face for the Caption role, with the priority `launch`; until it exists the Caption role uses `font.ui.compact`. The Japanese and Korean companions keep `launch` and belong to the upgrade. The font-role map names one face per role, with no fallback chain.
  - **Source format.** Per face, a **glyph sheet**: a greyscale coverage PNG that the owner draws at the face's design pixel size on a fixed grid, once at 1x and once at 2x, and a **glyph table** in the authoring syntax of ADR-062 (JSON validated by its schema), which holds the design size and, per glyph, the code point, the cell on the sheet, the advance, the bearing and the baseline, in 1x pixels. An optional kerning-pair table is allowed.
  - **Runtime format.** `R8_UNORM` coverage atlas pages, one per face and drawn size, never block-compressed (ADR-017), and the cooked glyph table. A **font packer**, a development tool under `Tools/` that the content build runs on the font sources (ADR-059), turns the sheets into the pages and the table; nothing of it ships or links into the executable (R14, ADR-055). The UI framework creates the pages through the GPU resource factory (ADR-078) when it starts, fills them from the cooked fonts, and keeps them resident: nothing is rasterised and nothing is evicted at run time, and after a device rebuild the re-created pages are filled again from the cooked fonts (ADR-008).
  - **Sizes.** The UI chooses the page, 1x or 2x, whose size is nearest the final pixel size of ADR-033 (UI-171): the 1x page up to 1.5 times the design size, the 2x page above it. It draws that page with linear filtering, scaled by the final size over the page's size. Scaling between the two pages is accepted, and so is the softness it brings; at the two drawn sizes, with glyph quads snapped to whole pixels, the text is exact. Measured widths come from the glyph table in 1x pixels, so they do not depend on the page.
  - **Horizontal scale.** The faces are drawn condensed, so the per-language horizontal scale of UI-197 is 1.0 for en-GB with these faces. The factor stays in the UI framework's data for the upgrade.
  - **Coverage for the MVP.** U+0020 to U+007E, U+00A0 to U+00FF, and the general punctuation that the en-GB strings use: U+2013, U+2014, U+2018, U+2019, U+201C, U+201D, U+2022 and U+2026. Cyrillic and the currency block move to the upgrade.
  - **Lost with bitmaps.** Shaping, bidirectional layout and complex-script fallback, and system-font fallback for player-entered text; kerning is the optional pair table only, and lines break at spaces. The MVP needs none of them, and they return with the upgrade.
  - **Kept.** Measured widths, from the glyph table's advances (UI-200); additive text; outlines and shadows as offset glyph quads in the ink state (ADR-031); inline icons as quads placed in the glyph run at their own advance; world-space labels, drawn from the same pages at the pixel size they cover. The dedicated server links no DirectWrite, and with the default neither does the client.
  - **Placeholder.** A generated block font: a tool draws it from a glyph table written in the tool's own source, newly authored, so a missing face never blocks a build. An edition marked for release never contains it (AST-085).
  - **Confirmation of the default.** In place of the four checks of the Confirmation section, which confirm the upgrade: **text tests** draw the en-GB strings of the MVP's front-end page and HUD (MVP-102, MVP-103) on those screens with both faces, on both pages, outlined and filled, and compare them with golden images; the **coverage check** walks the en-GB string table, and the upper-cased form of every string (titles are upper-cased, UI-233), against the glyph tables of the faces that its roles use, and fails on a missing glyph; and the **page-choice test** checks the page and the scale chosen at final sizes below, at and above 1.5 times the design size, that each drawn size maps one texel to one pixel, and that a string's measured width grows in proportion to the size across the change of page. The bidirectional and CJK samples of the amendment of 2026-10-18 belong to the upgrade's evidence, not to the default's.
  - **Trigger for the upgrade.** This record is amended to build option 1 before the first build that offers a language whose strings drawn Latin faces cannot cover (Cyrillic, kana and CJK ideographs, Hangul, a right-to-left or another complex script) or that needs the currency block, or that shows player-entered text (player names, chat or any free-text field). Whether the drawn faces then stay for Latin text beside DirectWrite, or Latin text moves to outline faces, is decided in that amendment.
  - The UI framework's text requirements carry dated notes that state this default ([layout_rendering_input_and_localisation.md](../architecture/components/ui_framework/layout_rendering_input_and_localisation.md) §4.9), and the font rows of the asset list follow ([GameAssets](../assets/GameAssets.md) §17).
- **Amendment 2026-10-19 (owner decisions O62 and O63).** The owner has drawn the two faces, and they change the default of the amendment above in five points; everything it states that these points do not change holds. The record stays Proposed.
  - **Faces and roles (O62).** One drawn face draws every player-facing text: **GameFont**, the asset `font.ui.primary`, serves the Title, Body and Caption roles. A fourth role, **Development**, draws the development overlay, the development windows and the text of the development drawing queue, in Debug builds only (DBG-004), with **EditorFont**, the asset `font.dev.editor`. `font.ui.compact` and `font.ui.stylised` are optional later faces with the priority `launch`.
  - **Source format (O63).** Per face, **one glyph sheet** and a glyph table. The owner's sheets are 32-bit DDS files, 256 × 224 px, white with the coverage in the alpha channel (0 or 255), on a grid of 16 × 16 px cells, 16 to a row, starting at U+0020 in the top-left cell. The glyph table maps each code point to its cell, so a cell's position need not be its code point. The font packer reads either a sheet of this kind or an 8-bit greyscale PNG.
  - **Runtime format and sizes (O63).** One `R8_UNORM` page per face, never block-compressed. Text of final size s (ADR-033) in a face of design size d is drawn at the whole-number scale k: s ÷ d rounded half up, at least 1, and raised until k × d reaches the accessibility minimum of ADR-064. Text is drawn with the point sampler, and every glyph quad is k × the cell in size, with its top-left corner on a whole pixel, so each texel covers exactly k × k pixels and the glyphs keep their drawn pixels at every scale. Text never scales between two whole numbers, so the softness that the amendment above accepted does not occur. Text oriented freely in the world, the one form whose size varies across a glyph, samples linearly.
  - **Advances.** Both faces are monospaced on the 16 px cell, as drawn: each glyph's advance is the cell width, and its quad is the whole cell. The role spacing factors hold 1.0, so that every advance is a whole number of pixels at every scale (UI-196 amendment).
  - **Coverage.** A face covers the code points that its glyph table maps. GameFont maps U+0020 to U+007E and U+00BF to U+00FF. EditorFont maps U+0020 to U+007E, U+00BF and U+00C0 to U+00FF; its cells 0x80 to 0xBF hold letters in a second order, which its table leaves unmapped apart from U+00BF. Neither face has the eight general-punctuation marks of the amendment above or U+00A1 to U+00BE (among them £ and °). Until the owner draws them, the en-GB strings use ASCII punctuation, and the coverage check holds them to it. U+00A0 maps to the blank cell of U+0020.
  - **Confirmation of the default.** The page-choice test is replaced by a **scale-choice test**: it checks the scale chosen at final sizes below, at and above each half-way point, that every texel maps to k × k pixels, and that a string's measured width is k times its width at 1. The text tests draw every scale that the MVP's screens use.
- **Amendment 2026-10-20 (M1 task T11, [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md)).** The font packer is part of the content build tool. It reads a face's DDS sheet and its glyph table, checks them against their rows and DAT-219, and writes the face's one `Font` entry (`FontAsset` version 1 of ADR-108): the metrics, the glyphs sorted by code point and one R8_UNORM page. A greyscale PNG sheet waits for a PNG decoder, and kerning joins the glyph table's schema as version 1.1 with the first face that kerns.
