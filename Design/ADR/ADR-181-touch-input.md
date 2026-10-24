---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M3 task T19), owner (O108)
---

# Touch input

## Context and Problem Statement

O108 makes a touchscreen a full way to play, without a keyboard, judged on an ARM64 tablet with the ARM64 development build (ADR-094). MVP-100 had input for keyboard and mouse only, and [ADR-028](ADR-028-input-device-back-ends.md) has no touch back-end. This record decides how touch reaches the client. It is written with O108, ahead of its code at M3 task T19; [ADR-182](ADR-182-the-verbs-under-touch.md) decides what the gestures do, at T20.

Windows gives a window touch and pen contacts as `WM_POINTER` messages. A window that passes them to `DefWindowProc` gets Windows' own reading instead:
- the gestures of Windows 7 (`WM_GESTURE`): a pan as scroll messages, a pinch as the wheel with Control held, press-and-hold as a right click;
- otherwise mouse messages: a tap as a click, a drag as a left-button drag.

Windows documents that a window which takes some pointer messages and passes the rest on behaves in an undefined way. So once the client reads touch itself, it reads all of it, in the world, the HUD, the menus and the front end. Today touch reaches the game only by that promotion: a drag on the world draws the selection circle or gives an order, and a hold releases the selection.

## Decision Drivers

* No new dependency (AGENTS.md R14): the Windows SDK's own APIs.
* The engine knows no game action (R9): touch reaches the game as gestures, not as the game's actions.
* Every threshold is a key (MVP-099), and the recognizer is a pure function of the contacts and their times, tested without a touchscreen.
* A touchscreen and a mouse or touchpad on the same 2-in-1 both work, the last one used deciding the pointer's look.
* Three-finger and four-finger contacts stay Windows' own (the shell's gestures).

## Considered Options

1. **`WM_POINTER` for touch and pen in a back-end of its own, and a gesture recognizer in NeuronClient**, beside the binding graph.
2. **`WM_GESTURE`**, Windows' recognizer of pan, zoom, rotate, two-finger tap and press-and-tap.
3. **Windows' promotion to mouse messages**, as today, with the pinch arriving as the wheel with Control held.
4. **DirectManipulation**, Windows' pan and zoom of a scrolled viewport, with inertia.

## Decision Outcome

Chosen option: 1.
- Option 2 reports one gesture at a time, so a pinch and a turn cannot run together. It has no threshold of its own for a drag against a hold, and no double tap.
- Option 3 has no turn and no second finger. A drag is a left-button drag, so it cannot pan.
- Option 4 models a scrolled surface, not a camera over terrain, and gives no tap, hold or double tap.

What T19 builds, in two steps: the first all but the on-screen keyboard, the second the on-screen keyboard.
* **The window** (`NeuronClient/AppWindow`).
  - It takes `WM_POINTERDOWN`, `WM_POINTERUPDATE` and `WM_POINTERUP` for pointers of type touch and pen (`GetPointerType`), and every `WM_POINTERCAPTURECHANGED`. It forwards them to the input back-end and returns them handled, so `DefWindowProc` never reads them. The mouse stays on its window messages: `EnableMouseInPointer` is not called.
  - At its creation `SetWindowFeedbackSetting` turns off Windows' touch and pen feedback for it (contact, tap, double tap, press-and-hold, right tap, press-and-tap and the pen's barrel), since the client draws its own.
  - `ClientFromScreen` turns a pointer message's screen position into client pixels.
* **The back-end** is the window's own (`NeuronClient/KeyboardMouseBackEnd`, ADR-028), which reads touch beside the keyboard and the mouse, since the router has one back-end of window messages.
  - Each touch or pen pointer holds one of `TOUCH_CONTACT_COUNT` (10) contact slots from its down to its up. Each down, movement in contact and up becomes an event of the router's queue (`InputDevice::Touch`, its slot the control) with its client position and the pump's timestamp (PLT-008), in arrival order (ADR-029).
  - A capture change or a canceled pointer (`IS_POINTER_CANCELED_WPARAM`) ends its contact as a cancel (`InputEvent::Kind::Cancel`), and the loss of focus ends every contact so (INP-111).
* **The recognizer** (`NeuronClient/TouchGestures`), from the contacts to a frame's gestures, with its thresholds in effective pixels × the DPI scale (ADR-033):
  - a **tap**: one contact down and up within `inp.touch.tap_ms` (250 ms), moved less than `inp.touch.slop` (10 effective px);
  - a **double tap**: a second tap within `inp.touch.double_tap_ms` (300 ms) and `inp.touch.double_tap_slop` (24 effective px) of the first. The first is reported as a tap when it ends, never held back;
  - a **drag**: one contact moved past the slop before `inp.touch.hold_ms` (350 ms), with its start, its point and its movement each frame;
  - a **hold**: one contact still for the hold time, then its movements as a hold's drag until it lifts;
  - a **manipulation** of two contacts each frame: their center, its movement, the ratio of their spans and the change of their angle. The turn is reported only once its sum in the gesture passes `inp.touch.turn_threshold` (10°), so a pinch does not turn the view;
  - a **two-finger tap**: two contacts down and up within the tap time, neither past the slop;
  - three or more contacts: nothing.
* **The router** (`InputRouter`) gives the frame's gestures as `Touch()`, as it gives the frame's characters (ADR-154). Gestures are not bindings: INP-060 has no node for them and nothing rebinds them.
  - **A tap** is also the primary button's press and release at its point, in its frame, with the pointer moved there. So every reader of the primary press takes it as a click: the UI's pages and HUD (UI-035 takes a press released within its frame), the front end, the placement and the battle verbs. This is ADR-182's tap, built with the back-end, since Windows' promotion stops the day the back-end lands.
  - `SuppressTouch` withholds the frame's gestures, which `GameUi` calls while a window has the focus, as it suppresses the world's actions (UI-006).
  - `SetTouchThresholds` takes the keys × the DPI scale, which `GameUi` gives it each frame.
* **The input mode** (INP-151) gains touch, the mode of a frame with any touch event or contact down. While it is touch, the game cursor is not drawn (`WorldPresentation::SetTouchMode`) and the pointer is the last contact's point. A frame with keyboard or mouse activity and no touch returns it to keyboard and mouse.
* **The camera** takes one finger's drag and two fingers' manipulation (ADR-180, `CameraControls::touchDrag`, `touchManipulation`). The drag holds the ground where the finger went down under it, by the middle button's level shift. The manipulation holds the ground under the two fingers' center, multiplies the height by the inverse of their spans' ratio and turns the heading by their turn, all at once. A hold's drag moves no camera: it is ADR-182's. This is ADR-182's camera, built here, since T19's run follows one finger and two.
* **The on-screen keyboard** (the second step, `NeuronClient/TouchKeyboard`).
  - The client asks for the system's touch keyboard through `IInputPaneInterop::GetForWindow` and `IInputPane2::TryShow` in two cases: a page text field (UI-121) starts editing in a frame in touch mode, or a tap lands while one edits, which brings back a keyboard closed by its own key. It asks `TryHide` as the editing ends (`TouchKeyboardRequestOf`).
  - Typing on the keyboard returns the input mode to keyboard and mouse, and the keyboard stays open.
  - These are the Windows SDK's `Windows.UI.ViewManagement` interfaces, linked from `runtimeobject.lib`, with no package. Windows Runtime is initialized on the main thread at the first request to show the keyboard, so a session without touch never initializes it. The main thread is a single-threaded apartment, since it pumps the window's messages.
  - The keyboard is drawn over the window, and the page under it is neither moved nor resized.
  - The interfaces' behavior on the tablet in a desktop window is confirmed on T19's run. A failure to get the pane or to show the keyboard is logged once and leaves the field typed by any keyboard.
* **Not changed.** The server links none of it (ADR-028). Windows' edge swipes stay Windows'.
* **The keys** `inp.touch.*` join the presentation record and the input component's tuning table, with DEC-1011's first values.

### Consequences

* Good, because touch, pen, mouse and touchpad work on one 2-in-1, and each gesture's threshold is the client's own key.
* Good, because the recognizer is a pure function, tested from recorded contacts on CI without a touchscreen.
* Bad, because the client must read touch everywhere at once: the HUD, the menus and the front end lose Windows' promotion the day the back-end lands. A tap as the primary press keeps every click working, but a drag on a page scrolls nothing until ADR-182's.
* Bad, because the touch keyboard's interfaces are WinRT ones, whose desktop behavior varies across Windows builds, and CI cannot run them.
* Bad, because the touch keyboard covers the lower part of the window, and a field under it stays hidden until the page is moved above it, which T19 does not do.
* Neutral, because pen is read as touch: Species has no use for its pressure or hover at M3.
* It forecloses: Windows' own gesture messages and its mouse promotion in the client's window.

### Confirmation

* `NeuronClientTests/TouchGesturesTests`, from recorded contact streams: a tap, a double tap and two slow taps; a drag against a hold at the slop and the hold time; a manipulation's center, ratio and angle, with the turn held below its threshold; a two-finger tap; three contacts ignored; a canceled contact ending its gesture.
* `NeuronClientTests/InputRouterTests`: a tap pressing the primary button at its point in its frame alone, a drag pressing nothing, `SuppressTouch` withholding the frame's drag, the input mode switching to touch and back on the mouse's movement, and the contact ended by the loss of focus.
* `NeuronClientTests/TouchKeyboardTests`: the keyboard is asked for once as a field starts editing by touch, again on a tap while it edits, never for a click, and hidden as the editing ends.
* `GameClientTests/SettlementCameraTests`: one finger's drag keeping the ground under it within two pixels, with no drift after the lift, and a hold's drag moving nothing; two fingers spread to 1.25 times their span, turned 20° and moved 100 px giving 800 wu, the heading turned 20° and the ground under their center within two pixels.
* The owner's run of T19 on the ARM64 tablet: Windows draws no feedback of its own, a drag no longer draws the selection circle, and the touch keyboard opens for the seed.

## Pros and Cons of the Options

### 1 — `WM_POINTER` and a recognizer of the client's own

* Good, because every gesture O108 names, and their combination, is the client's to define and test.
* Bad, because the recognizer is code to write and tune.

### 2 — `WM_GESTURE`

* Good, because Windows recognizes the gestures.
* Bad, because one gesture at a time, with Windows' thresholds and no double tap.

### 3 — the promotion to mouse messages

* Good, because it costs nothing.
* Bad, because there is no second finger, so no pinch, no turn and no pan apart from the circle.

### 4 — DirectManipulation

* Good, because pan and zoom get Windows' inertia.
* Bad, because it models a scrolled surface, and has no tap or hold.

## More Information

* Owner decision O108; DEC-1011; MVP-100, MVP-101; INP-060, INP-111, INP-151; UI-121; PLT-008.
* Related: [ADR-028](ADR-028-input-device-back-ends.md), [ADR-029](ADR-029-input-event-model-and-binding-contract.md), [ADR-033](ADR-033-ui-layout-scaling-and-dpi.md), [ADR-094](ADR-094-arm64-development-build.md), [ADR-154](ADR-154-text-input-and-the-page-text-field-at-m2.md), [ADR-180](ADR-180-the-camera-over-a-point-on-the-ground.md), [ADR-182](ADR-182-the-verbs-under-touch.md).
