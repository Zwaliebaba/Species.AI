---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Text rendering: DirectWrite shaping and a glyph cache

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
4. **Offline-baked glyph atlases** per font, size and script.

## Decision Outcome

Proposed option: **1, DirectWrite layout and a raster glyph cache**. **Default while Proposed:** the text stack is built this way, because DirectWrite is in-box and covers shaping, bidirectional text, line breaking and fallback, and raster glyphs at the final size give the sharpest text.

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
