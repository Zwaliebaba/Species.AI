---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# UI layout, scaling and DPI

## Context and Problem Statement

Species runs in windows of any size, on monitors from 1080p at 100 % scaling to 4K at 200 % and beyond, in 16:9, 16:10, 21:9 and 4:3 shapes, and players may move the window between monitors with different DPI. The kept front-end layout places every widget on a **screen-fraction grid**, so pages keep their proportions at any resolution, and the in-match HUD uses an orthographic space 600 units high (ADR-016). Text must stay legible: the accessibility baseline sets minimum text sizes and a player text-scale setting (ADR-064), and the glyph cache rasterises text at its final pixel size (ADR-034). The window and its per-monitor DPI awareness belong to the platform layer (ADR-075), which reports size and DPI changes.

The problem is how UI layout is expressed and converted into pixels, and how window size, DPI and the player's scale settings affect it.

## Decision Drivers

- Pages keep the proportions of the kept layouts at every resolution and aspect ratio.
- Text and thin elements stay legible and sharp at every DPI.
- Player UI-scale and text-scale settings (accessibility).
- Relayout on resize, DPI change, setting change or language change, without losing state.
- One conversion from layout units to pixels, shared by layout, rendering and the glyph cache.

## Considered Options

1. **Screen-fraction layout in data**, converted to pixels by the window size, with **DPI-scaled minimum sizes** and player UI-scale and text-scale settings.
2. A **fixed virtual canvas** (one reference resolution) scaled uniformly to the window, letterboxed where the aspect differs.
3. **Device-independent pixels**: layout in DPI-scaled pixels, as desktop applications do.

## Decision Outcome

Chosen option: **1, screen-fraction layout with DPI-scaled minimums and player scale settings**, because it keeps the kept layouts' proportions and uses DPI exactly where it matters: legibility and line sharpness.

- **Layout units.** Page layouts (ADR-030) give positions and sizes on the screen-fraction grid, anchored to the window; the in-match HUD uses its 600-unit orthographic space. One layout function converts both to pixels from the window's client size.
- **Scale settings.** A **UI-scale** setting multiplies element sizes around their anchors; a **text-scale** setting multiplies text sizes. Both are player options with ranges set by the accessibility baseline (ADR-064).
- **DPI.** The platform layer reports the window's DPI scale (ADR-075). Text sizes are clamped below by the accessibility minimum for the current pixel density; line widths, borders and focus outlines are multiplied by the DPI scale and snapped to whole pixels; icons choose the source size closest to their final pixel size. Developer tool windows, whose layouts are given in pixels, multiply every length by the DPI scale and the UI scale, so a tool window keeps its physical size on a high-density monitor.
- **Pixel snapping.** Element rectangles are snapped to whole pixels after conversion, so edges stay crisp (ADR-031).
- **Text size.** The final text pixel size (after layout, UI scale, text scale and the minimum clamp) is what the glyph cache rasterises (ADR-034); text is never scaled as a bitmap.
- **Relayout.** A resize, DPI change, scale-setting change or language change marks every page for relayout at the next update phase. Layout is recomputed from data; view-model state is untouched (ADR-030).
- **Overflow.** When a string or a scaled element does not fit its layout cell, the layout component applies the cell's declared overflow rule (wrap, shrink to the minimum, or scroll); a cell without a rule is reported by the validator for the languages that overflow it.

### Consequences

- Good, because pages look like their kept layouts at every resolution and aspect ratio.
- Good, because text is rasterised at the exact pixel size, so it is sharp at every DPI and scale.
- Good, because UI and text scaling are accessibility settings, not a different layout.
- Bad, because large text-scale values can overflow fixed-fraction cells; overflow rules and validation per language are needed.
- Bad, because ultra-wide and narrow aspect ratios stretch fraction layouts; any safe-area rule for them belongs to the UI component's page recipes.

### Confirmation

- Layout tests render reference pages at 1280×720, 1920×1080, 2560×1080, 3840×2160 and 1024×768, at 100 % and 200 % DPI and at the extremes of both scale settings, and check element positions, minimum text sizes and that no cell overflows without a rule.
- A test moves the window between monitors of different DPI and checks that text is re-rasterised at the new size and that page state is unchanged.
- A validator run lays out every page in every supported language at the largest text scale and lists cells that overflow.

## Pros and Cons of the Options

### 1. Screen-fraction layout with DPI-scaled minimums

- Good, because it keeps the kept proportions and handles DPI where it matters.
- Bad, because overflow at large text scales needs rules.

### 2. Fixed virtual canvas, letterboxed

- Good, because layouts are pixel-designed once.
- Bad, because letterboxing wastes screen area, text is either scaled as a bitmap or re-rasterised at sizes the design never saw, and ultra-wide screens show bars.

### 3. Device-independent pixels

- Good, because it matches desktop conventions and DPI awareness directly.
- Bad, because pages would no longer keep their proportions: a large monitor at low DPI shows a small UI in a corner of a large screen.

## More Information

- Related: ADR-016 (HUD space), ADR-030 (UI framework and layouts), ADR-031 (pixel snapping), ADR-034 (glyph cache), ADR-063 (localisation), ADR-064 (accessibility), ADR-075 (window and DPI awareness).
- The UI framework component (`architecture/components/ui_framework/`) specifies the grid, the anchors and the overflow rules.
- Explainer-animation panels (ADR-035) follow the relayout rule: they are laid out again on a resize, DPI change or language change, whereas the reference lays a panel out once, when it is created.
- **Amendment 2026-10-17.** Tool-window lengths scale by the DPI scale and the UI scale (UI-170). A language change relays out every open panel without losing state; the options page's apply step then rebuilds the front end as the front-end reference specifies (UI-238, R-META-450).
