---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (owner decision O105)
---

# The HUD's panels as framed plates sized to their rows

## Context and Problem Statement

The owner's run of the M3 build showed two faults in the settlement HUD's world panel:
- **The background.** It showed eight squashed columns.
- **The text.** Its standing rows ran past the panel's right edge.

**The background.** UI-054's recipe, from the reference, draws a window body as one quad of the accent panel strip, with u from 0 to `ui.chrome.body_repeat` (8) across the width and v from 0 to 1 down the height. The owner's `uitex.panel_accent` (128 × 1,020 texels, O64) is not a seamless strip. It is one framed plate:
- a light bevel on the left and the top;
- thin dark edges on the right and the bottom;
- chamfered corners;
- rivets down the face, in two columns, every 128 rows.

So the HUD drew eight plates side by side, each squeezed from 1,020 texels to the panel's height. `uitex.panel_neutral` (128 × 1,024) is the same plate with a 6-texel frame on every side.

**The text.** Every HUD panel had the fixed width `mvp.hud.panel_width` (236 units), and no row was measured against it. The standing line of ADR-169, "1. Red 0: 0 defense, 0 claims, 0 easing, 0 building", is about 238 units at the HUD's text size. That is wider than the whole panel before its padding and swatch. The relations panel's claim rows, the notification cards (`mvp.hud.card_width`), the result card (`mvp.hud.result_width`) and the construction bar's entries (`mvp.hud.bar_button_width`) had the same fault. A larger text scale (UI-171, `interface.text_scale`) makes every one of them overflow.

The owner chose a framed plate for every panel (O105).

## Decision Drivers

* The art is the owner's, and it is drawn as one plate: its frame and its rivets keep their texels' size, and nothing is squeezed.
* No row of the HUD runs off its panel at any text or UI scale (UI-171, XAG 101).
* The layout measures what Draw draws: one source of each row's text, never two that can drift.
* The tuning stays data (MVP-099): the frame's slices and scale are keys.

## Considered Options

For the background:
1. **A nine-slice plate.** The corners keep their texels. The top and bottom edges and the face stretch across. The side edges and the face are cut down the box at the texels' scale, repeating when the box is taller than the image.
2. **The reference's strips at their display size.** A whole number of 64-unit strips across, each cut down the box with its top and bottom edges.
3. **The art redrawn as a seamless strip**, keeping UI-054 as the reference states it.

For the text:
4. **Each panel measured.** The layout lays each row out with the text system Draw uses, and sizes the panel to its widest row, never narrower than its key.
5. **Wider fixed widths in the keys.**

## Decision Outcome

Chosen options: 1 (O105) and 4.
- Option 2 keeps the columns the owner rejected.
- Option 3 asks the owner to redraw art that is drawn as intended.
- Option 5 fails again at the next text scale, translation or four-figure renown.

What is built:

* **The plate** (`NeuronClient/NineSlice`). A `NineSlice` is an image's size in texels and its four slices' widths:
  - `AppendNineSlice` draws it on a box at a scale in pixels per texel, its edges on whole pixels;
  - `NineSliceScale` gives the scale, shrunk where a box is too small to hold the slices;
  - `NineSliceInner` gives the box inside the frame.
* **The frames**, in the presentation record (UI-054):
  - `ui.chrome.accent_slice.*` (40, 25, 24 and 6 texels) and `ui.chrome.neutral_slice.*` (40, 6, 24 and 6), measured on the owner's art. Each side's slice holds its frame and, left and right, its column of rivets, so nothing round is stretched.
  - `ui.chrome.plate_texel` (0.5 HUD units per texel, the art's 2x authoring) for panels, and `ui.chrome.button_texel` (0.25) for buttons, whose frame is half as thick.
  - `ui.chrome.body_repeat` is no longer read, and leaves the record.
* **The image sizes.** `PlateOf` (`GameClient/UiLook`) reads each panel image's size from its DDS header in the loaded UI images. A missing image has no size and is drawn whole.
* **The HUD** (`GameClient/SettlementHud`). Each panel's and card's rows are built once by a function of the model (`HudRow`: the text, its colour and indent, the icon, a meter, the text at the row's end, and the room kept for buttons or arrows). Draw draws them and the layout measures them.
  - With the text system, strings and locale the presentation hands it (`SetText`), the layout widens each panel to its widest row, never below `mvp.hud.panel_width`. The same holds for the result card (`mvp.hud.result_width`), the notification cards (`mvp.hud.card_width`) and the construction bar's entries (`mvp.hud.bar_button_width`).
  - Each panel's content lies inside its plate's frame, which the plate adds around it.
  - The world panel (bottom left) and the relations panel (bottom right) stand above the construction bar when they would reach under it.
  - Every panel is a plate of the accent art, and the result card one of the neutral art. Each button, arrow and queue cell is a plate at the button scale: the neutral art, or the accent art for the entry being placed and the attack being declared.
* **The Debug test page** (`--ui-test`) draws its two panels as plates.

### Consequences

* Good, because the owner's panels look as drawn, at any size, and the frames are retuned in data.
* Good, because a row cannot run off its panel, at any text or UI scale or string length, and the test checks every glyph.
* Bad, because the panels' sizes move with their text. A panel grows as the renown reaches four figures, and on a narrow screen a bottom panel jumps above the construction bar.
* Bad, because the layout lays the HUD's text out twice a frame. The text system keeps its layouts, so the second costs a look-up.
* It forecloses nothing: a later window recipe takes the same `NineSlice`.

### Confirmation

* `NeuronClientTests/NineSliceTests`: a plate's corners keep their texels, its face stretches across and is cut down, a tall box repeats its middle rows, a small box shrinks its frame, and an image of no size is drawn whole.
* `GameClientTests/SettlementHudTests`, with the plates and the committed strings, for a HUD with four settlements at four-figure renown:
  - every glyph lies inside a zone;
  - the world panel widens past its least width, and neither bottom panel overlaps the construction bar;
  - each panel is one plate of nine slices.
* The owner's run: the HUD's panels as single framed plates, and no text off its panel (DEC-1005).

## Pros and Cons of the Options

### 1 — a nine-slice plate

* Good, because it is how the art is drawn.
* Bad, because the slices are measured by hand and must be measured again when the art changes.

### 2 — the strips at their size

* Good, because it keeps UI-054's look.
* Bad, because a panel is still a row of plates, which the owner rejected.

### 3 — the art redrawn

* Good, because the recipe stays the reference's.
* Bad, because it asks the owner to redraw art that is right as it is.

### 4 — each panel measured

* Good, because it holds at every scale and every string.
* Bad, because the panels' sizes move.

### 5 — wider fixed widths

* Good, because no code changes.
* Bad, because it fails again at the next scale or string.

## More Information

* Owner decisions O64, O95, O105; DEC-1005; UI-054, UI-171; MVP-062, MVP-102.
* Amends [ADR-145](ADR-145-the-ui-pass-and-gamefont-text-at-m2.md) (the panel strip's repeat) and [ADR-152](ADR-152-the-settlement-hud-at-m2.md) (the HUD's panels).
* The code:
  - `NeuronClient/NineSlice`;
  - `GameClient/UiLook` (`PlateOf`), `GameClient/SettlementHud`, `GameClient/WorldPresentation`;
  - `GameData/Records/tuning/presentation.json` and its schema;
  - `NeuronClientTests/NineSliceTests`, `GameClientTests/SettlementHudTests`.
