---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T14)
---

# The UI core: pages, focus and layout at M2

## Context and Problem Statement

M2 task T14 builds the retained UI framework of [ADR-030](ADR-030-ui-framework-build-or-buy.md) to [ADR-033](ADR-033-ui-layout-scaling-and-dpi.md) at the MVP's size. It lands in steps:
- [ADR-145](ADR-145-the-ui-pass-and-gamefont-text-at-m2.md) drew the UI pass;
- [ADR-146](ADR-146-the-string-table-and-its-formatting-at-m2.md) added the string table;
- [ADR-147](ADR-147-the-directwrite-text-system-at-m2.md) moved the text to DirectWrite.

This third step builds the page stack, focus, layout and DPI that the front end (T18) and the HUD (T16) are built on. The component specification is `architecture/components/ui_framework/`. As the code stands, six things do not hold:

1. **Where the page layouts live.** UI-313 asks for one JSON record per page, validated by its schema and by UI-372's validator. The schema generator writes keyed records, not arrays of widget entries, and no cooker or reader of page records exists.
2. **How navigation finds a neighbor.** ADR-032 computes missing neighbors geometrically. UI-381 lets authored neighbors win over the navigation order and has no geometric rule.
3. **Which widgets.** The specification has no slider, meter or settlement HUD layout. The page family it does specify is large: page buttons, drop-downs, text fields, check boxes, lists and scroll bars.
4. **Input.**
   - The input router has no repeat condition (ADR-119), which UI-216 needs for the menu directions.
   - No menu action is bound.
   - UI-035's release has no action of its own.
5. **Drawing.**
   - The page title's highlight beam (UI-114) and the fill box's darkening halo (UI-115) are gradients, which ADR-145's quad does not carry; the halo's sides are horizontal gradients drawn in the ink state.
   - The beam's and the glow's art are BC4 masks, which the image shader reads as red.
6. **The world.** A focused page must take the arrow keys and the pointer from the camera and the battle verbs (UI-006).

## Decision Drivers

* Every page works the same with the mouse and with the keyboard, its input handled in the update phase and never while drawing (ADR-032, UI-021).
* The layout is sharp and in proportion at every client size and DPI (ADR-033).
* M2's pages are few: the test screen now, then the front end with its options and the HUD. The frameworks they need come first, and the parts nobody uses yet wait.
* The engine knows no game concept (AGENTS.md R9): the UI core takes the game's pages, look and actions as data.

## Considered Options

1. **Pages declared in code at M2**: each page builds its widgets and places them on the grid in code, and UI-372's checks run on every page in the tests.
2. **Page records now**: a page schema with arrays of widget entries, its cooker, reader and validator in the content build.
3. **An immediate-mode UI** that rebuilds the widgets every frame from code.

## Decision Outcome

Chosen option: 1.
- Option 2 needs a schema form, a cooker and a reader that no page uses yet, before the first page exists; its reason to be, layouts tuned without a rebuild, has no page to serve until the front end.
- Option 3 is what ADR-030 rejected for player-facing UI: it loses the retained view-model, the deferred page switch and the focus that persists across frames.

Option 1 is an M2 form, as O75 made the key bindings at M1 (INP-070). Page records replace it when a page's layout is to be tuned without a rebuild, an owner decision.

* **The layout** (`NeuronClient/UiLayout`, ADR-033):
  - **The page grid.** `ComputePageGrid` is UI-163's, from `ref.meta.layout.*`, and gives its worked values at 800 × 600, 1,024 × 768, 1,280 × 720 and 1,920 × 1,080.
  - **The HUD space.** It is RND-377's, `ref.ctrl.panel.canvas_height` (600) units high.
  - **The metrics** (`UiMetrics`):
    - the UI scale and the text scale, at `ui.scale.ui_default` and `ui.scale.text_default` until the settings store holds them (T15);
    - the DPI scale, which multiplies line widths, borders and the text's outline offset, each rounded and at least 1 px (UI-169);
    - UI-171's minimum text size.
  - **Snapping.** Every widget rectangle is snapped to whole pixels (UI-169).
  - **Text in its row.** A line is centered in its row by its line box (UI-165's purpose); UI-193's draw offsets belong to the reference's glyph engine, not to DirectWrite's line boxes.
* **The UI core** (`NeuronClient/UiCore`):
  - **Windows.** A page or a layer is a window covering the client area (UI-330). Its widgets carry typed handles, serial numbers never given twice, so a handle of a window that has gone finds nothing (UI-036).
  - **The page stack** follows UI-331, UI-332, UI-350, UI-351 and UI-355:
    - push, pop, replace and unwind;
    - a page on the stack is unwound to, and a pop of the only page is refused;
    - one pending request, the last of a frame winning, applied at the start of the next.
  - **Layers** (UI-336) open and close at the start of the next frame. The top one takes every input, and a page change closes them.
  - **Focus.** The top layer, else the top page, takes the input. UI-036's rules for overlapping panels reduce to this while every window covers the client area.
  - **Navigation** (UI-051): up and down move through the navigation order, wrapping and skipping inactive entries. An authored neighbor wins over the order (UI-381), and left and right move only to authored neighbors (UI-210).
  - **The pointer.**
    - Moving onto a widget makes it the cursor in hover-highlight mode, which navigation ends (UI-056).
    - A press captures the widget under the pointer, and a release on it clicks it (UI-031, UI-035).
    - Both happen in the update phase.
  - **The close action** clicks the window's back action, or runs its page's back handler (UI-383); the root page ignores it.
  - **Events.** A click, and a check box's or a cycler's change, reach the page's update hook (UI-352 step 4).
  - **Relayout.** A change of the metrics lays every window out again without creating anything (UI-172, UI-174).
  - **The validator.** `ValidateWindow` runs UI-372's checks that apply to a window: unique ids, focusable widgets in the order, entries that can take the focus, authored neighbors in the order, and the back action in the window.
* **The widgets** at M2 (UI-105 to UI-125):
  - the label;
  - the page title over its highlight beam, upper-cased in Latin-1 as the content build's coverage check upper-cases (UI-049, ADR-147);
  - the fill box;
  - the page button;
  - the check box;
  - the cycler: a page drop-down in cycle mode.

  Each draws as its requirement states, background, then pictures, then outline and fill text (UI-474), with fill boxes behind every widget (UI-337). The look is `UiStyle`, which the game fills from the presentation record and its images.
* **What waits:**
  - skip-next-update and locks;
  - scrolling lists (UI-053, UI-074 to UI-079);
  - drop-down lists (UI-118);
  - text fields (UI-064 to UI-070, UI-120 to UI-123), which T18's seed entry builds with the window's text input;
  - the page buttons' birth cascade, pop and transitions (UI-106, UI-107, UI-109), with the front end;
  - their cues, with M3's sound set;
  - tooltips, which UI-045 never shows;
  - the gamepad's input mode, which the MVP lacks (MVP-100).

  T16 adds the HUD's meters, icons and lists.
* **The UI pass** (ADR-031, ADR-145 amendments):
  - **Gradients.** Its quads have their own instance, `UiQuadInstance` (80 bytes): the rectangle, the texture coordinates, a first and a last color, and the gradient's axis, top to bottom or left to right (UI-184). The cursor and the development overlay keep `TextGlyphInstance`.
  - **The ink solid.** `ui_solid_ink` draws UI-115's darkening halo.
  - **Masks.** An image of one channel, BC4 or R8, draws white with its channel as its alpha, so the beam and the glow are tinted by their quads' colors.
* **The input** (ADR-119 amendment):
  - **Repeat.** The router's `Repeat` condition (INP-133, INP-134) fires on the press, then at the press + `inp.repeat.initial_delay` and every `inp.repeat.period`, on the real time that `AdvanceFrame` takes; its value is the frame's firings.
  - **The actions bound:**
    - table 3.6-B's rows 1 to 5 and 7: the arrow keys with repeat, Enter and Esc (UI-204);
    - table 3.6-A's rows 60 and 61: the pointer's press and hold.

    The release is the hold's end, also of a press released within its frame (UI-035). `action.menu.dismiss`, `front_end_activate`, `scroll_speedup` and `action.ui.alt_pointer_down` are not bound, since nothing reads them yet.
* **The client's UI** (`GameClient/GameUi`):
  - **Ownership.** It holds the UI core with the game's pages, and `WorldPresentation` owns it and draws it in pass 19.
  - **Order in the frame.** It is updated after the router and before the camera and the verbs.
  - **The world's actions.** While a window has the focus, the camera's and the verbs' actions are suppressed for the frame (UI-006); every M2 window covers the client area.
* **The test screen** (`--ui-test`, a Debug build's start-up option), in place of ADR-145's single test page:
  - **The root page.** Fill boxes on the page grid, the page title, and a column of page buttons, a check box, a cycler and an inactive button. Its right box holds ADR-145's samples of every text style and blend.
  - **What it opens.** A second page with a Back button, and a modal layer.
  - **Coverage.** Every pipeline of the UI pass draws, the ink solid among them, and `Build\CheckWorldFrame.py --ui-test` draws it on WARP in CI.
* **The keys.** The presentation record gains every key the code reads, the values those of the specification's tables:
  - `ref.meta.layout.*` (the grid, the title and Back);
  - `ref.ctrl.panel.canvas_height`;
  - `inp.repeat.initial_delay` and `inp.repeat.period`;
  - `ui.scale.ui_default` and `ui.scale.text_default`;
  - the page family's `ui.palette.fe_*` colors, `ui.palette.text_normal` and `ui.palette.check_box`;
  - `ui.page_button.caption_inset`, `ui.page_title.size_factor` and `fit_margin`;
  - `ui.fill_box.*`, with its glow motion as one key of two values;
  - `ui.check.*`.

  The panel palette of the HUD (RND-403) stays the owner's choice at T16 (O88).

### Consequences

* Good, because every page behaves alike with the mouse and the keyboard, and nothing reads input while drawing.
* Good, because the pages keep the reference's proportions at every size by its grid, and their lines stay sharp at every DPI.
* Good, because the front end and the HUD add pages and widgets to a core that has its tests, not a core of their own.
* Bad, because a page's layout is code: a change is a rebuild, and UI-372's build failure is a test failure.
* Bad, because every UI quad, each glyph included, moves 80 bytes where it moved 48.
* Bad, because a focused window takes every camera and verb action, which suits full-screen windows only; a window that leaves the world in view needs UI-006's hit test.
* It forecloses: input read while drawing; geometric navigation.

### Confirmation

* NeuronClientTests:
  - `UiLayoutTests`: UI-163's worked values, UI-169's lines, UI-171's text, RND-377's space and UI-168's scale about an anchor.
  - `UiCoreTests`: the page stack's operations and deferral, navigation's wrap and skip, authored neighbors, hover and click, the close action, a modal layer, relayout, the validator, the cycler, and the drawing of the band, the row and the tick.
  - `InputRouterTests`: the repeat's press, delay, period and firings.
* GameClientTests:
  - `GameUiTests`: a focused window withholds the world's actions, the arrows repeat, and the test screen draws every pipeline of the UI pass, gradients among them.
  - `UiLookTests`: the samples in their area.
* CI: the settlement world's render check with `--ui-test` on WARP, with the debug layer silent.
* The owner's T14 run, on its checklist:
  - the test screen at several window sizes;
  - the arrows, Enter and Esc;
  - the mouse;
  - the layer.

## Pros and Cons of the Options

### 1 — pages declared in code at M2

* Good, because the core, the widgets and the first pages land with their tests now.
* Bad, because a layout change is a rebuild until page records exist.

### 2 — page records now

* Good, because it is UI-313's form from the start.
* Bad, because a schema form, a cooker, a reader and a validator come before any page uses them.

### 3 — an immediate-mode UI

* Good, because it is the least code.
* Bad, because it is the option ADR-030 rejected, and it loses the deferred switch and the retained state.

## More Information

* Related: [ADR-030](ADR-030-ui-framework-build-or-buy.md), [ADR-031](ADR-031-batched-ui-rendering.md), [ADR-032](ADR-032-ui-focus-graph-page-stack-and-input.md), [ADR-033](ADR-033-ui-layout-scaling-and-dpi.md), [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md), [ADR-145](ADR-145-the-ui-pass-and-gamefont-text-at-m2.md), [ADR-146](ADR-146-the-string-table-and-its-formatting-at-m2.md), [ADR-147](ADR-147-the-directwrite-text-system-at-m2.md).
* The rules: UI-006, UI-021, UI-031 to UI-036, UI-049, UI-051, UI-056, UI-080, UI-081, UI-105, UI-108, UI-113 to UI-117, UI-125, UI-163 to UI-174, UI-184, UI-204, UI-210, UI-216, UI-313, UI-330 to UI-336, UI-350 to UI-355, UI-372, UI-381, UI-383, UI-474; RND-377; INP-133, INP-134; R-META-200, R-META-201.
* Owner decisions O75 and O88.
* The code:
  - `NeuronClient/UiLayout`, `UiCore`, `UiRenderer`, `InputRouter`, `PipelineCatalog`, `ShaderLayouts.h`, `WorldScene.h`, `Shader/Ui*`;
  - `GameClient/GameUi`, `UiLook`, `InputBindings`, `WorldPresentation`;
  - `Species/Species.cpp`.
* ADR-030, ADR-031, ADR-032, ADR-033, ADR-119 and ADR-145 are amended with this record, and UI-313 and UI-372 carry amendments.
