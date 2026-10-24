---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M3 task T20), owner (O108)
---

# The verbs under touch

## Context and Problem Statement

[ADR-181](ADR-181-touch-input.md) turns touch into gestures. This record decides what each gesture does in the world, on the HUD and in the menus, so that a steward plays without a keyboard (O108). It is written with O108, ahead of its code at M3 task T20.

The verbs were built for two mouse buttons and a hovering pointer:
- **The primary press acts at once** (`BattleVerbs::Primary`, [ADR-127](ADR-127-left-button-verbs-and-order-feedback-at-m1.md)). On an own Leader it selects it. With a selection it orders the selection to the pointer's ground point. Only on empty ground with nothing selected does it begin the selection circle, which grows with time and with the drag.
- **The secondary press** toggles the Leader under the pointer, else releases the selection, else promotes the Worker nearest the pointer (`BattleVerbs::Secondary`).
- **Hover shows** the Leader under the pointer, the Worker a secondary press would promote, the route preview to the pointer, and the placing ghost with the refusal foreseen.
- **Keys modify and act**: Shift (move only) and Control (straight) modify an order; Space releases, C demotes and Escape opens the world's menu.

A finger has no hover and one way to press, and its drag must also move the camera. So a touch cannot act when it goes down: it must wait until the recognizer knows whether it is a tap, a drag or a hold.

## Decision Drivers

* Every verb of MVP-101 and every keyboard action of play has a touch path (O108).
* The meanings O108 gives the gestures, and the server's commands unchanged: a touch sends what a press would.
* The mouse's verbs keep acting on the press, with no delay.
* A rule of the game, such as the circle's largest radius, is the same for every device.

## Considered Options

1. **The gestures of O108 mapped onto the verbs' meanings**: tap, drag, hold, two fingers, two-finger tap and double tap, with a strip of buttons for the keys.
2. **A mode switch**: a HUD toggle between moving the camera and selecting, each one-finger drag read by the mode.
3. **Windows' promotion kept for the verbs**, touch read only for the camera's two-finger gestures.

## Decision Outcome

Chosen option: 1 (O108).
- Option 2 hides a mode the steward must remember, as O107 found for the camera.
- Option 3 is undefined behavior by Windows' rule (ADR-181), and leaves the circle on every drag.

What T20 builds:
* **The camera** (`SettlementCamera`, [ADR-180](ADR-180-the-camera-over-a-point-on-the-ground.md)), built with T19 ([ADR-181](ADR-181-touch-input.md)), whose run follows one finger and two.
  - A one-finger drag holds the ground under the finger, as the middle button's drag does, by the same level shift.
  - Two fingers hold the ground under their center, multiply the height by the inverse of their spans' ratio with that ground as the zoom's anchor, and turn the heading by their angle's change past its threshold, all in the same frame.
* **A tap** is the primary press at its point, sent when the tap ends: the select, the order with its modifiers, the HUD's button, the placement, the front end's control. T19 builds it in the router, as the primary button's press and release in the tap's frame (ADR-181).
* **A hold.**
  - With nothing selected it begins the selection circle at its point, which grows as BattleVerbs' circle grows, with time and with the drag, up to `ref.ctrl.select.circle_max_radius` as for the mouse.
  - With a selection the route preview follows the finger, and the order is sent where it lifts.
* **A double tap** is the secondary press at its point, by BattleVerbs' order:
  - on an own Leader not otherwise selected, it toggles the Leader;
  - with a selection, it releases it;
  - with nothing selected, it promotes the Worker nearest the point.

  Its first tap has already acted as a tap. So a double tap on a Worker with a selection orders there and then releases.
* **A two-finger tap** releases the selection.
* **Placing.** While a structure is being placed, one finger moves its ghost, which shows the refusal the client foresees as it moves. Place and Cancel buttons beside the construction bar send the placement or leave it. Two fingers still move the camera.
* **The touch strip.** While the input mode is touch, the HUD shows a strip of buttons for the keyboard's actions of play:
  - the world's menu, Escape's (ADR-170);
  - release, Space's, and demote, C's;
  - move only, Shift's, and straight, Control's, as toggles that hold until the next order is sent.

  Alt+Enter and the development overlay stay on the keyboard.
* **Picking under a finger.**
  - In touch mode an own Leader is hit within `inp.touch.pick_radius` (24 effective px) of its projected center, where R-UNIT-192's sphere gives less.
  - Every HUD button a tap reaches is at least `ui.touch.target_min` (40 effective px) on each side: the touch strip by its layout, the HUD's rows by DEC-1005's layout, retuned on T20's run.
* **No hover.** No highlight cursor and no promotion ring show before a touch, and the badge counts the selection as it does.
* **The UI.** The page stack (ADR-149) takes a tap as the pointer's press and release at its point, and a drag on a list as its scroll. The front end, the options, the world's menu and the epilogue are played by taps, and the seed is typed on the touch keyboard (ADR-181).
* **The keys** `inp.touch.pick_radius` and `ui.touch.target_min` join the presentation record with T20, with DEC-1011's first values.

### Consequences

* Good, because every verb and every key of play is reachable with a finger, and the server sees the same commands.
* Good, because the mouse keeps its verbs on the press, with no delay.
* Bad, because a tap's order waits for the finger to lift, up to `inp.touch.tap_ms` later than a click's.
* Bad, because the circle's largest radius is 100 wu. Computed from ADR-180's keys for a 1080-row view, the full circle is 150 px across at the entry pose, about 23 mm on a 13.3-inch 1080p screen, but 34 px at the highest zoom, about 5 mm, under a fingertip. Touch selects crowds from the lower half of the zoom.
* Bad, because the first tap of a double tap acts, which the release after an order shows.
* It forecloses: a mode switch for one-finger drags; a circle or pick radius of touch's own in the simulation's rules.

### Confirmation

* `GameClientTests`: each gesture's verb at a scripted point (a tap selects and orders; a hold begins the circle or orders on the lift; a double tap toggles, releases and promotes by BattleVerbs' order; a two-finger tap releases), the commands equal to the mouse's, the placement moved by a drag and sent by Place, and the touch strip's toggles modifying the next order once.
* The owner's run of T20 on the ARM64 tablet, without a keyboard: a world started from the front end with a typed seed, a structure placed, a crowd selected and ordered, a Worker promoted, an attack declared and the world left from its menu.

## Pros and Cons of the Options

### 1 — the gestures mapped onto the verbs

* Good, because one gesture means one thing everywhere, and the mouse keeps its own.
* Bad, because a tap acts on its end, and a double tap's first tap acts.

### 2 — a mode switch

* Good, because a one-finger drag can both pan and draw the circle.
* Bad, because the steward must know the mode.

### 3 — the promotion kept for the verbs

* Good, because the verbs need no change.
* Bad, because Windows leaves it undefined, and a drag still draws the circle.

## More Information

* Owner decision O108; DEC-1005, DEC-1011; MVP-101; R-CTRL-156, R-UNIT-192.
* Related: [ADR-124](ADR-124-battle-verbs-at-m1.md), [ADR-127](ADR-127-left-button-verbs-and-order-feedback-at-m1.md), [ADR-149](ADR-149-the-ui-core-pages-focus-and-layout-at-m2.md), [ADR-153](ADR-153-the-settlement-verbs-at-m2.md), [ADR-170](ADR-170-the-world-menu-the-epilogue-page-and-leaving-at-m3.md), [ADR-180](ADR-180-the-camera-over-a-point-on-the-ground.md), [ADR-181](ADR-181-touch-input.md).
