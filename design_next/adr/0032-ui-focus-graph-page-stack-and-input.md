---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# UI focus graph, page stack and input handling

## Context and Problem Statement

Every Species screen must work with the mouse, the keyboard and the gamepad alike. The kept UI patterns use **one navigation cursor** shared by mouse hover and the gamepad (hover moves it only in mouse mode), a **controller navigation order** per page, **Back** as the universal way out of a page, captions that change with the input device, and option widgets that show the value the authority confirmed while sending the value the player requested. Modal messages (errors, confirmations) must own input until they are dismissed. Lists longer than their panel must scroll the focused item into view. Held buttons and scroll arrows repeat. Text fields need a caret, selection, clipboard, IME composition and validation.

Navigation that is derived from drawing order, input read while drawing, and actions that take effect when a page is left by Escape all make a UI behave differently on each device and at each frame rate.

The problem is how focus, navigation, page flow and UI input handling work in the UI framework (ADR-0030).

## Decision Drivers

- Every page fully usable with each device class, with the same results.
- Navigation order authored where it matters and computed where it does not.
- Back and Escape never apply a change.
- UI input handled in the update phase, never while drawing.
- Repeats and clicks independent of frame rate.
- Modal layers that cannot leak input to pages below them.

## Considered Options

1. An **explicit focus graph per page**, an **explicit page stack** with a deferred switch, and **UI input handled in the update phase**.
2. **Geometric navigation only**: the nearest focusable widget in the pressed direction, with no authored neighbours.
3. A **linear tab order** per page.

## Decision Outcome

Chosen option: **1, an explicit focus graph, an explicit page stack and update-phase input**, because it reproduces the kept navigation patterns exactly and makes every device behave the same way.

- **Focus scopes.** Each page and each modal layer is a focus scope. Within a scope, every focusable widget has up, down, left and right neighbours. Neighbours given in the layout data (ADR-0030) win; missing ones are computed geometrically from the layout, once per layout pass. The page's navigation order (for cycling) is part of its layout data.
- **One cursor.** There is one focused widget. In mouse mode, hovering a focusable widget moves focus to it; in gamepad and keyboard-navigation mode, only navigation moves focus. The input mode comes from the input router (ADR-0029).
- **Lists.** A focused item inside a scrolling list is scrolled into view.
- **Pointer.** A press captures the pointer to the pressed widget; a **click** is a press and a release on the same widget. Held buttons and scroll arrows repeat with explicit timers (an initial delay and a rate, UI tuning values) measured in presentation time.
- **Update phase.** UI input is processed once per frame in the UI's update phase, before layout and drawing, from the input router's actions. Drawing never reads input and never changes state.
- **Back.** Each page handles Back in the update phase; the default handler pops the page. Back and Escape cancel; only an explicit confirm applies a change.
- **Page stack.** Pages are pushed, popped and replaced on an explicit stack; a request made during a frame takes effect at the start of the next frame, so a page is never torn down while it is handling input.
- **Modal layers.** A modal (error message, confirmation, text-entry overlay) is a layer on the page stack that owns focus and Back until dismissed; no input reaches the layers below it.
- **Text fields.** A focused text field takes text events (ADR-0028), shows IME composition inline, and supports a caret, selection and the clipboard (UTF-16 at the Win32 boundary, UTF-8 inside). Validation rules (numeric ranges, maximum length, allowed characters) run while typing and again when the entry is confirmed.
- **Captions and confirmed values.** Widgets can bind a caption per input mode, and an option widget can bind a requested field and a confirmed field of its view-model, sending the first and showing the second.

### Consequences

- Good, because mouse, keyboard and gamepad reach every widget in the same, authored order.
- Good, because UI behaviour is independent of frame rate and of what is drawn.
- Good, because modals cannot lose or leak input, and Back never applies a change by accident.
- Bad, because layouts carry navigation data that authors must keep up to date (the validator checks that neighbours exist).
- Bad, because the deferred switch means a page change appears one frame after the request.

### Confirmation

- UI tests navigate every page with the gamepad only, the keyboard only and the mouse only, and check that every focusable widget is reachable and that the authored order is followed.
- A test presses Back and Escape on every page with a pending change and checks that nothing is applied.
- A test opens a modal over a page and sends input of every kind, checking that the page below receives none.
- A repeat test holds a scroll arrow at 30 and 144 frames per second and checks that the number of repeats over the same time is equal.

## Pros and Cons of the Options

### 1. Explicit focus graph, page stack, update-phase input

- Good, because it reproduces the kept patterns and is device-independent.
- Bad, because navigation data must be maintained.

### 2. Geometric navigation only

- Good, because no navigation data is needed.
- Bad, because the result changes whenever a layout moves slightly, and the authored order of the kept pages cannot be expressed.

### 3. Linear tab order

- Good, because it is simple and predictable for the keyboard.
- Bad, because grids and two-dimensional pages are awkward to navigate with a gamepad.

## More Information

- Related: ADR-0028 (text input), ADR-0029 (input mode and actions), ADR-0030 (UI framework), ADR-0033 (layout), ADR-0063 (localisation), ADR-0064 (accessibility: UI navigation and focus guidelines).
- The UI framework component (`architecture/components/ui_framework/`) specifies the page graph, the navigation order of each page and the widget behaviours.
