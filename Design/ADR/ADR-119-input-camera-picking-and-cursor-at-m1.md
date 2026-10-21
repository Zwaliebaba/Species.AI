---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M1 task T14), owner (O75, O76)
---

# Input, the player camera, picking and the game cursor at M1

## Context and Problem Statement

M1 task T14 builds the input router that replaces O65's stand-in, the player camera of the settlement view, picking, and the game cursors (M1 plan table 2). Its run is the owner flying the valley with the reference keys and wheel, while the cursor follows the pointer and changes over a Leader.

The input component specifies a full binding graph: a parser and a writer over one name table, cooked binding-set records in four layers, repeat timers, latches, idle and setting nodes, and a gamepad back-end on GameInput. O75 compiles M1's bindings in instead.

The reference camera (R-CTRL-450 to R-CTRL-497) is written for a skirmish map. The camera component's world amendments leave four things open, which O76 settles:
- the first pose in a world;
- the bounds while the sandbox has no territory;
- the cursor sizes against the owner's 128 px art;
- where O65's three stand-in keys go.

Two more things are missing:
- The hover over "a selectable Leader" needs the client's own team, which nothing sends.
- Nothing draws a cursor, and the system pointer is hidden (O65).

## Decision Drivers

* The reference rules as written, wherever M1 reaches them (MVP-101, CAM-002), and each deviation named.
* No new dependency (AGENTS.md R14), and no gamepad at M1 (MVP-100).
* The engine knows no game action by name (AGENTS.md R9).
* Every camera position and every pick in binary64, camera-relative (WLD-246, CAM-095 amendment).

## Considered Options

* 1 — the router's M1 subset with the bindings compiled in, the camera in GameClient, picking helpers in NeuronClient, and the own team in the terrain message
* 2 — the input component's whole binding graph, with its parser, name table and cooked binding-set records
* 3 — as 1, with the own team a constant of GameShared cited as the sandbox's

## Decision Outcome

Chosen option: "1".
- Option 2 builds the parser, the writer, the layers and the settings store's user layer, for a set that O75 compiles in.
- Option 3 is wrong as soon as a second client commands another team, and the server already knows the answer.

* **The router** (`NeuronClient/InputRouter`, `InputEvents`) in its M1 form:
  - **Events.** It keeps one ordered event queue per frame (ADR-029), and applies the frame's events to each key's and button's state: held after the last event, and the presses and releases in the frame. The mouse's movement and wheel are summed.
  - **Conditions.** `down`, `up` and `held` on keys and buttons (INP-051, INP-052), and `move` and `read` on the movement and the wheel. An auto-repeat never presses (INP-053).
  - **Nodes.** A device leaf, `all` and `chord` (INP-060, INP-137). The bindings are validated once, and one that breaks the node rules is logged and left out.
  - **The game's actions.** The game gives the router its actions, in action-id order, with their bindings. The engine holds no action name but for the log.
  - **What waits.** The name table, the parser and writer, the layout and user layers, repeat, latch, `not`, `alias`, `idle`, `setting` and `value`, the input mode and idle time, and the queue's capacity (INP-116). They come with the menus and the settings store of M2, which need repeat and a user layer.
* **The compiled-in set** (`GameClient/InputBindings`, O75): the rows of table 3.6-A that the camera uses (rows 1 to 4, 6 to 10, 12 and 13), keyboard and mouse only, at the US and UK key positions. With no layout layer, an AZERTY or QWERTZ keyboard has the labels of table 3.2-A, not the reference positions of table 4.13-B. T17 adds its verbs' rows.
* **O76's interim actions**, each an M1 form that ends when its home exists:
  - `action.window.toggle_mode`, `all(key:alt:held, key:enter:down)`, until the screen options (RND-442);
  - in Debug builds `action.debug.remove_device`, `chord(key:control, key:shift, key:f11)`, until the debug-command window's "reset render resources" (DBG-117 row 16);
  - the sandbox restart's entry, which T15b adds.
* **The keyboard and mouse back-end** (`NeuronClient/KeyboardMouseBackEnd`, ADR-028):
  - **Keys and buttons.** The window's key, system-key, button, movement and wheel messages become events in arrival order, with the pump's timestamps (PLT-008). A key's code is its virtual-key code. A modifier covers both its keys, so the second of a pair adds no press and the first released releases nothing.
  - **The wheel** is read in notches of `inp.mouse.wheel_units_per_notch` (120), unrounded.
  - **Focus loss.** It releases everything held (INP-111). The releases follow the events already collected, so a key pressed and the window left within one frame ends the frame released. Without focus, presses and movement are dropped.
  - **The quit.** The close request is the quit (PLT-009), and no binding raises it (INP-076).
  - **Not at M1.** Raw mouse input is not registered, since no binding reads `raw_motion` (ADR-092's deferral continues), and there is no GameInput (MVP-100).
* **The pointer** (INP-155 to INP-159, PLT-068):
  - It follows the mouse's client position, clamped to the client area. With the mouse alone, that is INP-156's aim added to the position.
  - `WarpPointer` moves the system pointer. The movement message this causes arrives at the position the back-end already holds, so it makes no event.
  - `ConfinePointer` clips the system pointer to the client area. The clip is applied again after a move, a resize and a return of focus, and suspended without focus and on `WM_CANCELMODE`.
  - The settlement view confines the pointer while the camera advances, and the frame loop releases the clip when it ends.
* **The player camera** (`GameClient/SettlementCamera`, ADR-090's "the settlement view"):
  - **Pose.** It holds a front vector. With no roll at M1, its up vector is rebuilt from world up and its pose is a yaw and a pitch (`CameraPose`).
  - **Modes.** It is static until a world is entered, then in free movement. Tracking and the aim modes are out of reach (MVP-100, no structures to aim).
  - **What runs.** R-CTRL-456 steps 2, 3, 4, 6 and 7, and R-CTRL-471 steps 2, 3 and 5 to 10. There is no scripted move (DEC-051), no shake before T16's laser bolts, and no speed-up hint (no tutorials).
  - **Time.** The camera advances only while the window has focus, by the real frame step clamped to `ref.sim.frame.delta_clamp` (CAM-030 to CAM-032). Smoothing is unclamped as DEC-864 item 2 states.
  - **World entry (O76, CAM-060 amendment).** The bounds become the region's square ± `ref.sim.match_entry.camera_border` (200 wu). The camera cuts to `fallback_camera_height` (1,000 wu) along `fallback_camera_direction` (−1, −0.7, −1), placed so that the screen centre falls on the centre of the site `mvp.sandbox.steward_site`. It enters free movement and asks the pointer to the screen centre. A new terrain, as a sandbox restart sends, is a new world entered.
  - **Bounds on the target too.** R-CTRL-471 step 10 clamps the camera's position; the target is clamped as well. Otherwise a pan held against the bound winds the target ever further beyond it, and the pan back waits for it.
  - **The steepest front is 89° from the horizontal.** The reference states no limit. Mouse-look turns the camera to the point under the pointer, and the cursor lock holds that point while a pan carries the camera over it. A front through the vertical then reverses its ground projection, so the pan turns back and forth in place. At the limit the front keeps its heading, and the pan carries on past the point.
  - **The pointer's ground point (R-CTRL-058).** It is the terrain's `CastRay` hit ahead of the ray's start; TER-017 also reports hits behind it. A miss gives the far meeting with the sphere about the region's centre of radius `ref.ctrl.pointer.miss_sphere_factor` × the region's side (204,800 wu), the restatement of the map's sphere.
  - **Its keys** are the presentation record's (ADR-116): `ref.ctrl.camera.*`, `ref.sim.match_entry.*`, `ref.sim.frame.delta_clamp`, `ref.ctrl.pointer.*` and `cam.pick.row_offset`.
* **Picking** (`NeuronClient/Picking`, CAM-090 to CAM-093, RND-216):
  - `PickRayAt` unprojects the pointer at depth 1 and 0 through the binary64 inverse of the frame's camera-relative view-projection, and adds the camera's binary64 position. The ray runs through the lower-left corner of the pointer's pixel, `cam.pick.row_offset` (1 px), as DEC-956 states.
  - `ProjectToScreen` maps a world point to overlay pixels, and `RaySphere` gives a ray's first meeting with a sphere ahead of it.
* **The hover** (`SettlementView`, R-CTRL-055 for M1's one kind):
  - **Candidates.** The client's own living Leaders, each tested as R-UNIT-192's sphere: 4 wu above it, radius 8 wu. The radius is scaled with the camera's distance by R-UNIT-142's factor at `rnd.option.worker_scale_factor.default` (2.0), a named constant until the settings store exists.
  - **The winner** is the smallest screen distance between a sphere's projected centre and its projected hit, the earlier on a tie.
* **The own team** (ADR-102's message): `WorldTerrain` carries the team the connection commands as a steward, the lowest one, or none (version 2, 44 bytes). The server sends it with the terrain at connection and after a restart. A client whose team another steward takes over (ADR-109) keeps the old team until the next terrain. M1 has one client.
* **The game cursor** (`NeuronClient/CursorRenderer`, the shaders `Cursor`, pass 20):
  - **Which.** `cursor.highlight` over an own Leader. `cursor.unavailable` while the pointer's ray misses the terrain or its ground point lies outside the region (O76). `cursor.world` otherwise.
  - **Size.** At `mvp.cursor.world_size`, `highlight_size` and `unavailable_size` (48, 48 and 60 physical px, O76), not scaled by the display. RND-367 sizes cursors in physical pixels at every resolution.
  - **Hotspot.** The pointer's hotspot is the tip of the owner's arrow, `mvp.cursor.world_hotspot` (0.113, 0.082), where `ref.ctrl.cursor.set.world_hotspot` misses it by 7 texels. The other two are centred (`ref.ctrl.cursor.set.*_hotspot`).
  - **Colour.** White, and the unavailable cursor red (`ref.ctrl.cursor.set.unavailable_color`).
  - **The masks** are the owner's one-level BC4 files (O64), read as intensity in `.r` (ADR-115's rule for masks). The 128 px art is drawn at about a third of its size, so the pixel shader averages a 3 × 3 grid of bilinear taps across each pixel's footprint rather than reading one.
  - **Drawing.** The authored shadow first, with `Darkening` (the destination × (1 − mask)), which is RND-058's `Ink` at source alpha 0. Then the art, added in its colour with `Additive`.
  - **Pass 20** is on the back buffer, after the final pass and before pass 29's text. The frame without a world draws the cursor too, because the system pointer is hidden from M0 on (O65). The highlight effect's rings (R-CTRL-363) are not drawn at M1.
* **Release builds** hold the router, the camera, picking and the cursor. The development action exists only in Debug builds (INP-094).

### Consequences

* Good, because the owner's T14 run flies the reference camera over the valley with the reference keys and wheel, and T17's verbs build on the router, the picking and the cursor.
* Good, because the render check of ADR-117 draws the cursor's pass on WARP with GPU-based validation.
* Bad, because the binding graph's other node kinds, its layers and its parser wait for M2, so the bindings change only by a rebuild.
* Bad, because a non-US keyboard has its own labels for the camera keys until the layout layers exist.
* Bad, because the camera's 89° limit and the target's clamp are not the reference's; the owner's run judges both.
* Neutral, because at the top of the settlement view, 5,000 wu, most of the valley lies beyond the fog's end of 6,120 wu of view depth (O59). MVP scope §5.3 leaves the fog end open, and the owner's run shows it.

### Confirmation

* NeuronClientTests:
  - `InputRouterTests`: a press and a release in one frame, auto-repeat, the focus loss, chords and conjunctions, the pointer's movement, wheel and warp, and a binding that breaks the rules;
  - `PickingTests`: the centre ray, a pick projected back to its pixel with the row offset, and ray–sphere.
* GameClientTests, `SettlementCameraTests`:
  - the world entry over the site;
  - AC-CTRL-CA-01 to AC-CTRL-CA-05 (pan speed, zoom, height, bounds, cursor lock);
  - the turn towards the pointer and looking around;
  - the hover over an own Leader;
  - the cursor's quads.
* GameSharedTests, GameLogicTests and GameClientTests: the commanded team round-trips, the sandbox names team 0 at connection and after a restart, and the session keeps it.
* The render check of ADR-117 draws the cursor on WARP.
* The owner's run: the valley flown with the keys and wheel, the cursor on the pointer and over a Leader, and Alt+Enter and Ctrl+Shift+F11 working.

## Pros and Cons of the Options

### 1 — the M1 subset, compiled in

* Good, because it builds only what M1 reaches, each piece where its component puts it.
* Bad, because the parser, layers and nodes that follow must fit the router's shape, as INP-060 already states it.

### 2 — the whole binding graph

* Good, because the bindings would be data from the first task.
* Bad, because it builds the name table, the parser, the writer and four layers before any menu or settings store exists.

### 3 — a constant own team

* Good, because it needs no message change.
* Bad, because it is wrong for any second client, and the server already knows the answer.

## More Information

* Related: [ADR-028](ADR-028-input-device-back-ends.md), [ADR-029](ADR-029-input-event-model-and-binding-contract.md), [ADR-016](ADR-016-view-projection-and-depth-conventions.md), [ADR-090](ADR-090-solution-and-project-layout.md), [ADR-102](ADR-102-region-terrain-and-the-terrain-check-at-connection.md), [ADR-109](ADR-109-battle-sandbox-command-journal-and-seed-switch.md), [ADR-115](ADR-115-the-world-frame-at-m1.md), [ADR-116](ADR-116-presentation-tuning-record.md), [ADR-117](ADR-117-render-check-on-warp.md).
* Owner decisions O64, O65, O75 and O76.
* The code: `NeuronClient/InputEvents`, `InputRouter`, `KeyboardMouseBackEnd`, `Picking`, `CursorRenderer`, `AppWindow` (the pointer services), `FrameRenderer` (pass 20); `GameClient/InputBindings`, `SettlementCamera`, `SettlementView`, `WorldPresentation`; `GameShared/WorldTerrain`; `Species/Species.cpp`.
* **Amendment 2026-10-21 (M1 task T17a, owner decision O78, [ADR-124](ADR-124-battle-verbs-at-m1.md)).** The battle verbs' rows of table 3.6-A join the compiled-in bindings, and the settlement view runs the verbs after the camera. The highlight cursor also shows over the Worker that a secondary press would promote, the cue R-CTRL-356's ring gives in the reference.
