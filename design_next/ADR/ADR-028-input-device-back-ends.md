---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Input device back-ends

## Context and Problem Statement

Species is played with mouse and keyboard first and with a gamepad as a full alternative. The mouse drives selection, orders and the camera through the pointer position; raw, unaccelerated movement deltas must also be available as a bindable control, for camera controls that a design may add (the reference control scheme binds none); the keyboard drives bindings, including Alt and function keys; text entry (player names, chat, search, map names) must support input method editors for every language the game ships; and gamepads must connect and disconnect at any time. Short presses that begin and end between two frames must still be seen. The binding model above the devices, a composable grammar of conditions over device controls, belongs to the input component (`architecture/components/input.md`) and to the controls reference (`reference/06_controls_camera_and_commands/`); this decision covers only how device input reaches it. The headless server reads no devices.

The problem is which Windows APIs supply keyboard, mouse, text and gamepad input.

## Decision Drivers

- Raw mouse deltas as a bindable control; system cursor position for the pointer.
- Every key bindable, including Alt, F10 and the numeric keypad, with no special cases in the game.
- Full IME and keyboard-layout support for text entry.
- Gamepad hot-plugging with stable device identities.
- Every press and release delivered in order, even within one frame (ADR-029).
- A supported, current API for game controllers, with a known redistribution model.

## Considered Options

1. **GameInput** for gamepads and other game devices, and **Win32 window messages** (with raw mouse input) for mouse, keyboard and text.
2. **GameInput for every device**, including keyboard and mouse, with window messages for text only.
3. **Raw Input** for every device, including gamepads read as HID devices.

## Decision Outcome

Proposed option: **1, GameInput for game devices and window messages for mouse, keyboard and text**. **Default while Proposed:** the input back-ends are built this way, because text entry and IME need window messages anyway, mouse and keyboard through window messages are the best-understood path on Windows, and GameInput is Microsoft's current controller API.

- **Gamepads.** GameInput, built against a pinned `Microsoft.GameInput` package and a pinned API namespace: **3.5.283 with `GameInput::v3`**, moving to the 4.0 line after its first point release (4.0.300 was published on 2026-10-15). `GameInputCreate` is called during initialisation. Device connection and disconnection arrive through GameInput's device callback; each device keeps its GameInput device identity, which input events carry (ADR-029). The back-end reads every reading since the previous frame, in order, so no button transition between frames is lost.
- **Redistribution.** The installer runs the `GameInputRedist.msi` from the same package version (or newer); the redistributable refuses to downgrade a newer installed runtime.
- **Mouse.** Raw mouse input (`WM_INPUT`, registered with `RegisterRawInputDevices`) supplies the raw movement deltas (a bindable control with no default binding); `WM_MOUSEMOVE` supplies the pointer position for the UI and the world cursor. Button messages capture the mouse on press and release it on the last button up. Wheel deltas are accumulated in the system's fine units, so high-resolution wheels are not rounded away.
- **Keyboard.** `WM_KEYDOWN`, `WM_KEYUP`, `WM_SYSKEYDOWN` and `WM_SYSKEYUP` are handled alike, so Alt, F10 and every other key produce both press and release events. Auto-repeat messages are marked as repeats and do not create new presses. `WM_INPUTLANGCHANGE` makes the input router re-apply bindings and key names for the new layout. Alt+F4 is handled by the platform layer as a normal close request.
- **Text.** `WM_CHAR` and the IME messages feed the focused text field (ADR-032) through the UI's text service; text input never passes through the binding graph.
- **Focus loss.** When the window loses focus, the back-ends emit release events for every held key and button, so nothing stays held.
- **Not linked by the server.** The dedicated server links neither GameInput nor the input back-ends.

### Consequences

- Good, because controllers use the current Windows game-input API with hot-plugging and stable identities.
- Good, because text entry gets full IME and layout support from the standard Windows path.
- Good, because every key, including Alt and F10, behaves like every other key.
- Bad, because there are two device APIs to test, and GameInput needs its redistributable in the installer.
- Bad, because GameInput's keyboard and mouse support, which could unify the paths, is not used in the baseline; adopting it later is a revision of this ADR.

### Confirmation

- An input test injects press-and-release pairs within one frame for keys (including Alt and F10), mouse buttons and gamepad buttons, and checks that both events reach the router in order.
- A hot-plug test connects and disconnects two gamepads during a match and checks that device identities stay stable and no input stays held.
- An IME test enters text in a CJK input method in a text field and checks the committed string.
- A packaging check confirms that the installer carries the GameInput redistributable of the pinned version or newer.

## Pros and Cons of the Options

### 1. GameInput for game devices, window messages for mouse, keyboard and text

- Good, because each device class uses its best-understood path.
- Bad, because two APIs feed the router.

### 2. GameInput for every device

- Good, because one API serves all devices, with timestamps and readings in one model.
- Bad, because text entry and IME still need window messages, and the keyboard path through GameInput is less proven for bindings and layouts.

### 3. Raw Input for every device

- Good, because one Win32 API serves all devices.
- Bad, because gamepads read as raw HID reports need per-controller mapping tables that GameInput already provides, and hot-plug handling becomes Species code.

## More Information

- Technology baseline item 6 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://www.nuget.org/packages/Microsoft.GameInput · https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-nuget · https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-versioning · https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-faq.
- Not verified: the licence terms of the GameInput redistributable, and whether the 4.0 package adds a `GameInput::v4` namespace (re-checked on 2026-10-17 without result).
- Related: ADR-029 (input event model and binding contract), ADR-032 (text fields and focus), ADR-071 (install layout and prerequisites), ADR-075 (application window).
- **Amendment 2026-10-17.** Raw mouse deltas are a bindable control (`raw_motion`) with no default binding; the reference camera follows the pointer and has no drag (input component, table of mouse controls). The input component keeps both back-ends behind one device back-end interface (INP-105), so option 2 can replace option 1 without changing the input router.
