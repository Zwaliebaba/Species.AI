---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M3 task T18), owner (O108)
---

# The camera over a point on the ground

## Context and Problem Statement

The settlement view's camera is the reference's free movement (R-CTRL-471, [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md)), with two owner amendments laid on it:
- O90 ([ADR-142](ADR-142-the-camera-flies-along-the-view-within-the-stewards-areas.md)) made the forward and back keys fly the camera along its view;
- O107 ([ADR-179](ADR-179-the-camera-turns-only-while-the-middle-button-is-held.md)) limited the mouse-look to while the middle button is held, so that the HUD can be reached.

On the M3 build the owner found the result a flight camera, and asked for one that is more still and suits touch. The camera had six controls that act apart: the level position, the absolute height, a free yaw and pitch up to 89°, and the field of view. Measured on the code before this record:
- **The forward key changes the height.** From the entry pose, 26° below the horizon, it moves the camera 224 wu forward and 111 wu down each second, so it reaches the ground in about 9 s.
- **The pan's speed is fixed.** It is 250 wu/s at every height. The view at the entry pose is about 4,600 wu wide at the screen's center, so the picture moves by 5 % of its width a second; 100 wu up, by 54 %.
- **The look is a rate, not a position.** Holding the middle button turns the front towards the point under the pointer at a speed that grows with the pointer's offset, a half-life of 5 frames at 60 frames a second. The cursor lock then moves the system pointer back towards the center.
- **There are two zooms.** The wheel changes the height, by its square root, and Z narrows the field of view. Neither moves towards the point under the pointer.
- **The design needs a ground point under the screen's center.** WLD-240 enters the strategic view from the settlement view keeping it, and a free pitch can face the horizon, where there is none.

O108 settles the shape: a camera held over a point on the ground, which a touchscreen's drag, pinch and twist can move directly (ADR-181, ADR-182 build touch at T19 and T20). These questions stay:
- how the tilt follows the zoom;
- how fast the keys and the wheel move it;
- how the drag and the zoom keep the ground under the pointer;
- what bounds it;
- what happens to the look-around, the cursor lock and the frame bench's pose.

## Decision Drivers

* The picture moves alike at every zoom.
* The ground under the pointer stays under it while the pointer grabs or zooms, as a finger's will.
* The HUD's pointer is never moved, except by looking around, which the steward asks for.
* WLD-240's ground point under the screen's center exists at every pose, and WLD-244 bounds it.
* The camera's advance stays within `cam.budget.advance_ms` (CAM-120).
* The rendered scenes that the owner's M1, M2 and M3 evidence measures stay comparable.

## Considered Options

For the camera's state:
1. **A ground point, a height above it and a heading**, the tilt a function of the height.
2. **A ground point, a distance, a heading and a free tilt**, which a touch gesture or a key changes.
3. **The free movement kept**, with its pan scaled by the height and its forward key level again.

For the tilt:
4. **Linear in the height's logarithm**, between two tilts at the lowest and highest heights.
5. **Linear in the height.**

For the drag and the wheel's zoom:
6. **The ground held under its pixel**: after each move the ground point shifts level until the held point lies under the pixel again.
7. **The ground point moved by the pointer's movement × a scale**, and the zoom about the screen's center.

## Decision Outcome

Chosen options: 1, 4 and 6.
- Option 2 adds a control that the steward must keep right, and lets the view face the horizon again.
- Option 3 keeps the six controls, and the horizon.
- Option 5 spends most of the wheel's notches at the top of the range: from 40 to 5,000 wu, a notch's tilt step would be ten times larger near the ground than near the top.
- Option 7 slides the ground under the pointer whenever the terrain or the tilt changes the scale, which a finger notices at once.

What is built (`GameClient/SettlementCamera`):
* **The state.** The ground point (its height eased onto the land under it), the height above it, and the heading, the yaw about world up. Each has a goal it closes on by min(1, `cam.orbit.smoothing` × dt) of the difference a frame. The heading closes the short way. The field of view and R-CTRL-460's zoom stay as they were.
* **The tilt** below the horizon is `cam.orbit.pitch_low` (30°) at `cam.orbit.height_min` (40 wu) and `cam.orbit.pitch_high` (65°) at `ref.ctrl.camera.max_height` (5,000 wu), linear in the share ln(h ÷ 40) ÷ ln(5,000 ÷ 40) and held beyond the ends. Every wheel notch therefore tilts the camera by the same step.
* **The pose.** The front is the heading tilted down by the tilt. The position is the ground point less the front × h ÷ sin(tilt), so the camera stands h above the ground point.
  - R-CTRL-471 step 6 as amended: when the land under that position comes within `ref.ctrl.camera.ground_clearance` of it, the camera is lifted to that clearance and the front turned back onto the ground point.
* **The keys.**
  - W, A, S and D and the arrows move the ground point's goal level, along the heading and across it, by `cam.orbit.pan_rate` (0.5) × the camera's distance from the ground point each second. Shift multiplies that by `cam.orbit.fast_pan_factor` (3).
  - Q and E (`action.camera.turn_left`, `turn_right`) turn the heading at `cam.orbit.turn_rate` (90° a second).
  - Page Up and Page Down (`action.camera.raise`, `lower`) zoom out and in at `cam.orbit.zoom_key_notches` (4) notches a second, about the screen's center.
  - Home (`action.camera.home`) sends every goal back to the entry pose, and Backspace (`action.camera.reset_heading`) sends the heading back.
* **The wheel.**
  - A notch forward multiplies the height's goal by `cam.orbit.zoom_step` (0.85), and a notch back divides it, between 40 and 5,000 wu.
  - The ground point under the pointer becomes the zoom's anchor. Each frame until the height is within 0.1 % of its goal, the ground point shifts level so that the anchor lies under the same pixel, on the level plane at the anchor's height.
  - Moving the pointer, panning or dragging lets the anchor go.
* **The horizontal wheel** (`action.camera.pan_wheel`, the new mouse control `wheel_horizontal` of table 3.2-B, from `WM_MOUSEHWHEEL`) moves the goal across by `cam.orbit.wheel_pan_share` (0.1) of the distance a notch. A touchpad's two-finger slide sideways arrives as it, its slide forward and back as the wheel, and its pinch as the wheel with Control held.
* **The drag.** The middle button held (`action.camera.drag`) grabs the ground under the pointer when it goes down. Each frame after, the ground point and its goal shift level so that the grabbed point lies under the pointer, on the level plane at its height.
  - A level move of the camera moves a ray's meeting with a level plane by as much, so one shift a frame is exact on that plane.
  - With Alt held as well (`action.camera.turn_drag`), both actions fire (INP-085). The camera then turns the heading by the pointer's horizontal movement × `cam.orbit.drag_turn` (0.3° a pixel) at once, the view following the pointer, and grabs nothing.
* **The bounds** (R-CTRL-471 step 10 as amended).
  - The region's square ± `ref.sim.match_entry.camera_border` bounds the ground point and its goal; the position is not bounded.
  - In a world with the steward's settlement both stay inside the steward's areas by ADR-142's slide, each from its place at the frame's start.
  - WLD-244's "camera's ground point" is this point, the one point under the screen's center.
* **The pointer.** It is never moved, except at the entry (R-CTRL-470) and while looking around.
  - Looking around (F2, R-CTRL-472) holds the camera still and turns a separate look front towards the point under the pointer, by step 8's rate and ADR-119's steepest front of 89°, with the cursor lock of R-CTRL-464.
  - Released, the front blends back onto the ground point at `cam.orbit.smoothing`.
* **The entry** (O76 as amended): the ground point is the center of the steward's site, at `ref.sim.match_entry.fallback_camera_height` (1,000 wu) above the land there, with the heading of `fallback_camera_direction` (45°). The tilt at that height is 53.3°, and that pose is the home.
* **The frame bench's pose.** `SettlementView::EnterAt`, which only the bench calls (ADR-126), enters the world and then holds O76's old pose onto the fight (`HoldEntryPose`): 1,000 wu up along (−1, −0.7, −1). The bench never advances its camera, so its rendered scenes are those that the M1, M2 and M3 evidence measured.
* **The bindings** (`GameClient/InputBindings`): table 3.6-A rows 67 to 73 (O108's amendment). Row 66, `action.camera.look`, is retired. The layout layers of table 4.13-B move the turns in place of the raise and lower keys.
* **The keys** are the presentation record's `cam.orbit.*` (ADR-116), with the values DEC-1011 states.

### Consequences

* Good, because the picture moves by 24.4 % of its width a second at every height: 40 wu/s at 40 wu up, 623 wu/s at the entry and 2,758 wu/s at 5,000 wu. These are computed from the keys for a 16:9 view at 60°, where the width at the ground point is 164, 2,559 and 11,325 wu.
* Good, because the ground under the pointer stays under it while the steward zooms or drags, which ADR-182's pinch and drag reuse unchanged.
* Good, because the HUD's pointer is free, and the camera turns only on a key, the drag with Alt, or the look-around the steward holds.
* Good, because WLD-240's ground point exists at every pose: the strategic view can continue the same zoom above 5,000 wu.
* Good, because one advance costs 5.3 µs with keys, a turn and the wheel moving and 2.6 µs at rest, against CAM-120's 50 µs. It was measured on a Linux build at `-O2` of the class with the committed records and seed 1's terrain, on the development container's Xeon at 2.8 GHz, over 20,000 advances each. The Windows build is unmeasured.
* Bad, because the reference's free flight is gone: the lowest pose is 40 wu above the ground at 30°, and only the look-around can face the horizon.
* Bad, because a notch forward now zooms in, where the reference's raised the camera (R-CTRL-461).
* Bad, because the frame bench's scenes keep a pose more oblique than the player's camera reaches at 1,000 wu, which shows more terrain and figures. Their figures bound the player's view from above rather than equal it.
* Neutral, because the tilt's ends and every rate are provisional keys (DEC-1011), which the owner retunes after T18's run.
* It forecloses: a free tilt; the reference's free movement in the settlement view; a camera position that leaves the steward's areas' bound by itself.

### Confirmation

* `GameClientTests/SettlementCameraTests`:
  - the entry over the site at 1,000 wu and 45°, the screen's center on the site, and the bounds (CAM-060);
  - the tilt at both ends, halfway at the heights' geometric mean, and held beyond them;
  - a second of the keys moves the ground point by 0.5 × the distance, or 1.5 × with the fast pan, along the heading or across it, at the same height;
  - the wheel's notch forward to 850 wu and two back to 1,176 wu with the ground under the pointer within two pixels, and the ends at 40 and 5,000 wu;
  - at 40 wu a pan keeps the camera at least the ground clearance above the land;
  - Page Down for a second gives four notches about the center;
  - Q for a second turns 90° about the ground point, which stays under the screen's center; Backspace restores 45°, and three seconds of E turn 270°;
  - the drag keeps the grabbed ground under the moving pointer within two pixels, never moves the pointer, and leaves no drift; with Alt, 100 px turn 30° at once;
  - a horizontal notch moves the ground point 0.1 × the distance to the right;
  - Home returns to the entry pose;
  - AC-CTRL-CA-02 unchanged, AC-CTRL-CA-04 on the ground point, and WLD-244's areas on the ground point every frame;
  - the pointer stays put while the keys pan; looking around holds the camera, turns the front, moves the pointer by the lock, and returns;
  - the bench's held pose.
* `GameClientTests/InputBindingsTests`: the French turn to the left on A, and the zoom out on Page Up under the layout layer.
* `NeuronClientTests/InputRouterTests` and `BindingNotationTests`: the horizontal wheel summed apart from the vertical one, and `wheel_horizontal` written back as parsed.
* The suite of `SettlementCameraTests`, but for its three tests that need the client's views, was also built and run on Linux against the committed records before the push.
* The owner's run of T18 on `Debug|ARM64`.

## Pros and Cons of the Options

### 1 — a ground point, a height and a heading

* Good, because one control, the zoom, sets both the height and the tilt.
* Bad, because a steward cannot look along the valley from close by, except by looking around.

### 2 — a free tilt

* Good, because every view the reference allowed stays reachable.
* Bad, because the tilt is one more thing to set right, and the horizon has no ground point.

### 3 — the free movement retuned

* Good, because it changes the least code.
* Bad, because it keeps what the owner found wrong.

### 4 — the tilt in the height's logarithm

* Good, because each notch, a ratio of the height, tilts by the same step.
* Bad, because two keys, the ends, shape the whole curve.

### 5 — the tilt in the height

* Good, because it is the simplest.
* Bad, because the tilt hardly changes near the ground, where the steward watches crowds.

### 6 — the ground held under its pixel

* Good, because it is exact on the held point's plane at every height and tilt.
* Bad, because it casts one more ray a frame while a drag or a zoom runs.

### 7 — a scaled move

* Good, because it needs no ray.
* Bad, because the ground slides under the pointer.

## More Information

* Owner decision O108; DEC-1011; R-CTRL-460, R-CTRL-461, R-CTRL-464, R-CTRL-470, R-CTRL-471, R-CTRL-472; WLD-240, WLD-244; CAM-060, CAM-120; INP-085; MVP-101.
* Amends [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md), [ADR-142](ADR-142-the-camera-flies-along-the-view-within-the-stewards-areas.md) and [ADR-179](ADR-179-the-camera-turns-only-while-the-middle-button-is-held.md). Related: [ADR-116](ADR-116-presentation-tuning-record.md), [ADR-126](ADR-126-frame-bench-and-rendered-scenes.md), [ADR-181](ADR-181-touch-input.md), [ADR-182](ADR-182-the-verbs-under-touch.md).
* The code: `NeuronClient/InputEvents`, `InputNames`, `InputRouter`, `KeyboardMouseBackEnd`; `GameClient/InputBindings`, `SettlementCamera`, `SettlementView`; `GameShared/TuningPresentation.h`, generated from `GameData/Schemas/tuning.presentation.schema.json`.
