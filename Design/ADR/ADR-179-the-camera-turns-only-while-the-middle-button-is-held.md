---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (owner decision O107)
---

# The camera turns only while the middle button is held

## Context and Problem Statement

The settlement view's camera ([ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md)) is the reference's free movement (R-CTRL-471). Each frame its step 8 turns the camera's front towards the ground point under the pointer. Its step 9, the cursor lock (R-CTRL-464), then moves the pointer so that it stays on that ground point. As the camera turns towards the point, the point moves towards the screen's centre and the pointer moves with it. The reference says so: the camera "always turns to face the point under the pointer", and "there is no edge scrolling".

The reference's interface opened as a separate overlay, so its pointer had nothing else to reach. The Species HUD ([ADR-152](ADR-152-the-settlement-hud-at-m2.md), [ADR-166](ADR-166-the-relations-panel-and-the-notifications-of-m3.md)) is the opposite: panels and buttons stand at the screen's edges and need a free pointer. On the owner's M3 build the pointer could not be held over any of them, and the owner found nothing in the game they could click.

## Decision Drivers

* The steward reaches every HUD panel and button with the mouse, without a mode to remember.
* The camera can still be aimed with the mouse, as the reference's can.
* Esc stays the world's menu (ADR-170) and the UI's close action (UI-204).
* The keys keep flying the camera as O90 made them.

## Considered Options

1. **The middle button held to look.** The pointer is free by default. While `action.camera.look` (the middle button) is held, the camera turns towards the point under the pointer and the cursor lock runs.
2. **A key that toggles between camera and pointer modes** (Tab).
3. **Mouse-look paused while the pointer is over a HUD panel.**

## Decision Outcome

Chosen option: 1 (O107). It is the convention of strategy and settlement games.
- Option 2 hides a mode the steward must remember.
- Option 3 leaves the HUD reachable only by a quick flick, before the turn draws the pointer back.

What is built:

* **The action.** `action.camera.look` is table 3.6-A row 66: Boolean, `mouse:middle:held` (`GameAction::CameraLook`, `GameClient/InputBindings`). A steward may rebind it through `binding.action.camera.look` (INP-070). `action.camera.zoom` keeps `key:z:held` alone.
* **The camera.** `SettlementCamera::Advance` (`GameClient/SettlementCamera`) turns the camera (step 8) only while `CameraControls::look` or `lookAround` is set. The lock (step 9) records the pointer's ground point and moves the pointer with it under the same condition, so a free pointer stays where it is on the screen while the keys move the camera.
  - Holding the look turns the camera towards the pointer's point, and the pointer drifts towards the centre with it, as before.
  - Look-around (F2, R-CTRL-472) still follows the pointer while it freezes the position.
  - Entering the world still centres the pointer (R-CTRL-470).
* **The view.** `SettlementView` reads the action into the camera's controls.

### Consequences

* Good, because the HUD is reachable at all times, and the camera can still be aimed with the mouse.
* Good, because a free pointer no longer slides as the camera flies, which the battle verbs' circle and orders also benefit from.
* Bad, because the reference's single-handed mouse flight now needs the middle button held, and a mouse with no middle button must rebind the action.
* Bad, because O90's descent onto the point under the pointer now needs the look held.
* It forecloses nothing: a toggle can be bound to the same action later.

### Confirmation

* `GameClientTests/SettlementCameraTests`:
  - the pointer alone does not turn the camera, and the look held turns it towards the pointer;
  - a free pointer stays put while the keys move the camera, and the look held moves it with its ground point;
  - the forward key with the look held lands on the point under the pointer;
  - with the look held, the camera stays inside the steward's areas.
* `GameClientTests/InputBindingsTests`: the layout and user layers still apply.
* The owner's run: the HUD's buttons can be clicked, and the middle button held aims the camera.

## Pros and Cons of the Options

### 1 — hold to look

* Good, because it has no mode, and it is familiar.
* Bad, because it takes a button the zoom shared.

### 2 — a toggle

* Good, because it keeps the reference's flight whole.
* Bad, because the steward has to know which mode the game is in.

### 3 — paused over the HUD

* Good, because it changes the least.
* Bad, because the pointer cannot be brought to the HUD reliably.

## More Information

* Owner decisions O90, O107; R-CTRL-460, R-CTRL-464, R-CTRL-470, R-CTRL-471, R-CTRL-472; INP-070.
* Amends [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md) (the camera and its bindings) and [ADR-142](ADR-142-the-camera-flies-along-the-view-within-the-stewards-areas.md) (the forward key's descent onto the pointer's point).
* The code: `GameClient/InputBindings`, `GameClient/SettlementCamera`, `GameClient/SettlementView`, `GameClientTests/SettlementCameraTests`.
