---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T18)
---

# Text input and the page text field at M2

## Context and Problem Statement

M2's T18 builds MVP-103's front end, and its new world takes a seed that the steward may type (MVP-005). The seed is shown in the world's panel as its digits, so that it can be typed again ([ADR-152](ADR-152-the-settlement-hud-at-m2.md)).

The UI core of [ADR-149](ADR-149-the-ui-core-pages-focus-and-layout-at-m2.md) has no text field, and nothing carries characters to it:
- the window forwards `WM_CHAR` to the keyboard and mouse back-end, which drops it ([ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md));
- the router has no text channel.

The design is complete:
- **Text input.** It reaches the UI as characters, never as keys, and never through the bindings (UI-007, INP-006, INP-112).
- **The text field.** It edits on a click; Enter applies, Escape cancels, and a click on another field abandons (UI-064, UI-066).
- **Caret and clipboard.** The field has a caret that moves, a selection, the clipboard and an input method's composition (UI-067).
- **The page text field** draws a row box, its caption, its value right-aligned, and a blinking caret at a fixed place (UI-120, UI-121). On Enter it does not also activate (UI-123).

## Decision Drivers

* A seed of up to 20 digits, typed and typed again (MVP-005).
* One key press never both edits and navigates (UI-123).
* No text passes through the binding graph (INP-112).
* Only what the front end needs at M2, with the rest of UI-067 waiting.

## Considered Options

1. **Characters through the router to the UI core, and a `TextField` kind of the page family** with UI-066's characters, the caret at the end of the text.
2. **The whole of UI-064 to UI-068 now**: caret movement, selection, the clipboard and the input method's composition.
3. **No text field: the seed drawn by a button and stepped by a cycler.**

## Decision Outcome

Chosen option: 1.
- **Option 2** builds the clipboard and the input method's composition for one field of digits that needs neither.
- **Option 3** breaks MVP-005: a seed shown in the world's panel could not be entered again.

* **Characters** (`NeuronClient/KeyboardMouseBackEnd`, `InputRouter`):
  - **The back-end.** It turns each `WM_CHAR` into a Unicode scalar value (`TextCharacter`), with Shift, Ctrl and Alt as held when it arrives (UI-007). `JoinUtf16` joins a surrogate pair and drops a lone half, so UTF-16 exists only at the Win32 boundary. The back-end is the router's `TextSource`.
  - **The router.** In `AdvanceFrame` it takes the frame's characters into `Text()` and drops them while the window has no focus (INP-112). They never enter the event queue or satisfy a binding. The client sets the source; the frame bench sets none.
* **The text field** (`NeuronClient/UiCore`, `UiWidgetKind::TextField`):
  - **Its fields.** A value (UTF-8), a maximum length in characters (UI-068, none when 0), and a digits-only flag. The flag is M2's form of a numeric field whose value is wider than UI-065's bounds.
  - **Starting an edit.** A click or an activation makes the field the UI core's text-edit target, with its value as the typed text (UI-038, UI-064). `IsTextEditing` reports it (UI-039).
  - **The characters** follow UI-066's order of tests:
    - Backspace deletes the last character;
    - Enter applies the typed text to the value with a `Changed` event;
    - Escape cancels;
    - a control character, or one typed with Ctrl, inserts nothing;
    - otherwise a character is inserted at the end when the flag allows it and the length is below the maximum.
  - **Navigation.** While a field is edited, and in the frame its edit ends by Enter or Escape, the menu keys do not navigate, activate or close. So Enter does not also activate and Escape does not also leave the page (UI-123's skip of the next update).
  - **Leaving the edit.**
    - A press on another text field abandons the edit (UI-064).
    - A press anywhere else applies it first. UI-064 does not say what such a press does; applying keeps a seed typed before Start is pressed.
    - A field that leaves the focused window, by a page change or a layer, is abandoned.
  - **The drawing** is UI-121's first style. The row box is in the focus band while highlighted or edited; the caption sits at the caption inset; the value, or the typed text while editing, is right-aligned one font size from the right. While editing, the caret line is drawn at the fixed place, blinking.
* **The keys.** The presentation record gains the UI framework's keys for the caret: `ui.field.caret_blink_period`, `ui.field.caret_duty`, `ui.field.caret_margin`, `ui.page_field.caret_width` and `ui.page_field.caret_offset`.
* **What waits:**
  - UI-067's caret movement, selection, clipboard and composition;
  - the editing commands of INP-113;
  - UI-068's parse and clamp, which the page that reads a numeric field does at M2;
  - UI-122's second style;
  - the frame gate of INP-156 for the world's actions, since no world runs under the front end's fields.

### Consequences

* Good, because a seed is typed, edited and applied with the keys the design names, and no key press does two things.
* Good, because characters reach the UI by the route INP-112 states, ready for the chat entry and the input method later.
* Bad, because the caret stays at the end of the text, so a digit in the middle is changed by deleting back to it.
* It forecloses: text that passes through the bindings; a field that applies its edit on a key the bindings give.

### Confirmation

* NeuronClientTests:
  - `UiCoreTests`:
    - a field edited on activation takes the digits, deletes, stops at its maximum, and applies on Enter, which does not also activate it;
    - Ctrl's commands and control characters insert nothing;
    - Escape cancels and leaves the page;
    - a press on a button applies the edit and the button clicks;
    - a press on another field abandons the edit;
    - a page change ends the edit;
    - the caret draws in the first half of its period and not in the second.
  - `InputRouterTests`: characters reach `Text()` in order with Shift, satisfy no binding, last one frame and are dropped while unfocused; `JoinUtf16` joins a pair and drops a lone half.
* The owner's T18 run: the new world's seed field takes typed digits, Backspace and Enter, and Escape cancels.

## Pros and Cons of the Options

### 1 — characters through the router and a page text field

* Good, because it is the design's route at its smallest.
* Bad, because the caret does not move.

### 2 — the whole text field

* Good, because UI-067 is met.
* Bad, because the clipboard and composition have no user at M2.

### 3 — no text field

* Good, because nothing new is needed.
* Bad, because a seed cannot be typed again (MVP-005).

## More Information

* Narrows [ADR-032](ADR-032-ui-focus-graph-page-stack-and-input.md)'s text fields to M2's subset; extends [ADR-028](ADR-028-input-device-back-ends.md)'s keyboard and mouse back-end and [ADR-149](ADR-149-the-ui-core-pages-focus-and-layout-at-m2.md)'s page family.
* MVP-005; UI-007, UI-038, UI-039, UI-064 to UI-070, UI-120, UI-121, UI-123; INP-006, INP-112, INP-113, INP-156.
* The code: `NeuronClient/InputEvents` (`TextCharacter`, `TextSource`), `KeyboardMouseBackEnd` (`JoinUtf16`), `InputRouter` (`SetTextSource`, `Text`), `UiCore` (`TextField`); `GameClient/GameUi`, `UiLook`; `Species/Species.cpp`.
