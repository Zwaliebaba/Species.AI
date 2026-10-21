---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Input event model and the binding-graph contract

## Context and Problem Statement

Species maps device input to actions through a composable binding graph: leaf conditions on device controls (a key going down, a button held, a stick moved, a trigger released, an idle period), combined by conjunction, chords, inversion and aliases. The graph and the control scheme are specified by the input component and the controls reference; they are kept as they are. Underneath, the graph needs device input in a form that never loses a transition, treats every device alike, and makes every binding round-trip: a binding that the player sets in the rebinding screen must be written, read back and evaluated as the same binding. Analogue controls need dead zones and response curves that are smooth and symmetric. Repeating actions (holding a direction in a menu) must repeat at a rate independent of the frame rate.

The problem is the contract between the device back-ends (ADR-028) and the binding graph: the shape of input events, how conditions are evaluated from them, how names and bindings are parsed and serialised, and how analogue values are processed.

## Decision Drivers

- No transition is lost, however short.
- One event model for every device, with device identity preserved.
- Strict parsing: an unknown word is an error, never a silent fallback.
- Every binding round-trips between the rebinding screen, the saved settings and the graph.
- Analogue processing in floating point, symmetric, with radial dead zones.
- Repeat timing independent of frame rate, with no state shared between bindings or devices.

## Considered Options

1. **Timestamped, ordered input events** per frame, evaluated by the binding graph event by event, with one name table for parsing and serialising, and float analogue processing.
2. **Per-frame state snapshots** (held or not held, axis values), with edges derived by comparing consecutive snapshots.
3. **Callback-driven actions**: back-ends call action handlers directly as messages arrive.

## Decision Outcome

Chosen option: **1, ordered input events, event-by-event evaluation, one name table, float analogue processing**, because it is the only option that cannot lose a transition and keeps the binding graph's semantics independent of frame rate.

- **Events.** The back-ends write timestamped events into one ordered queue per frame: key and button down and up (with a repeat flag for operating-system auto-repeat), axis samples (stick, trigger, mouse delta, wheel), text characters (routed to the UI only), and device connect and disconnect. Every event carries its device identity.
- **Evaluation.** The input router (`InputRouter`) feeds each frame's events, in order, to the graph's leaf conditions (`InputSourceNode`). A down condition is true in a frame if a down event for its control occurred in that frame; an up condition likewise; a held condition reflects the state after the frame's last event. A press and release within one frame therefore make both the down and the up condition true for that frame. Operating-system auto-repeat events never satisfy a down condition.
- **Repeat.** The graph's repeat condition uses explicit timers with an initial delay and a rate (input component tuning values), measured in real time on the platform clock (ADR-074), never in game-speed-scaled presentation time, so repeats keep working while a match is paused. Each binding has its own repeat state per device; no repeat state is shared between bindings, controls or devices.
- **Names and parsing.** One table maps every bindable control (each key, mouse button, wheel, gamepad control) to exactly one bindable name. The binding parser and the binding writer both use it, so writing a binding and parsing it back yields the same binding. The parser rejects an unknown driver word, control name or condition word with an error that names the word and the binding; nothing falls back to a default condition. Display names shown to the player are localised string ids, never fed back into parsing.
- **Analogue processing.** Sticks and triggers arrive as floating-point values (−1 to 1 per stick axis, 0 to 1 per trigger). Sticks use a **radial** dead zone with rescaling, so motion is continuous at the dead-zone edge and symmetric in every direction; triggers use a threshold. A per-control **response curve**, defined as data (the reference curve is cubic), maps the result, and a per-binding sensitivity multiplier scales it, all in floating point with no truncation.
- **Input mode.** The router tracks the last active device class (keyboard and mouse, or gamepad) for input-mode-dependent captions in the UI.
- **Outputs.** The router produces actions and camera control for presentation and player commands for the simulation's command catalogue (`reference/12_simulation_model/`). Input never writes simulation state.

### Consequences

- Good, because sub-frame presses and releases are never lost, on any device.
- Good, because a binding saved from the rebinding screen always parses back to the same binding.
- Good, because data errors in binding files are found when they are loaded, with a clear message.
- Good, because analogue control is smooth and symmetric, and repeats behave the same at any frame rate.
- Neutral, because the radial dead zone and response curve give the full output at every deflection direction, diagonals included, so diagonal stick panning is faster than under per-axis processing; the stick pan gain is confirmed in playtesting.
- Bad, because event queues need bounded capacity per frame (an input component tuning value), and a burst beyond it is counted and dropped.
- Bad, because strict parsing turns tolerated typos in hand-edited binding files into load errors (by design).

### Confirmation

- Property tests generate random bindings from the grammar, write them with the binding writer and parse them back, and check equality.
- Unit tests feed event sequences (press and release in one frame; auto-repeat; two devices pressing the same control) and check every condition kind.
- Parser tests feed unknown driver words, control names and condition words and check that each is rejected with a message naming it.
- Unit tests check the radial dead zone (continuity at the edge, equal response at equal deflection in all directions) and the response curve against its data.

## Pros and Cons of the Options

### 1. Ordered events with event-by-event evaluation

- Good, because no transition can be lost, and evaluation is exact.
- Bad, because the graph's leaf conditions must be written against events, not states.

### 2. Per-frame state snapshots

- Good, because conditions are simple comparisons.
- Bad, because a press and release between two snapshots disappear, and edge detection depends on the frame rate.

### 3. Callback-driven actions

- Good, because actions fire with the lowest latency.
- Bad, because chords and conjunctions need state across callbacks, ordering between devices is lost, and actions could fire outside the frame's update phase.

## More Information

- Related: ADR-028 (device back-ends), ADR-032 (UI input in the update phase), ADR-064 (accessibility: remapping and hold or toggle options).
- The input component (`architecture/components/input/README.md`) specifies the binding grammar, the name table and the tuning values; the controls reference (`reference/06_controls_camera_and_commands/`) specifies the control scheme and the commands.
- **Amendment 2026-10-17.** Repeat and idle timings are real time on the platform clock, as the input component specifies (input component §1 terms, `inp.repeat.initial_delay`, `inp.repeat.period`, R-CTRL-006); menu navigation therefore repeats while a match is paused. The consequence of the radial dead zone for diagonal stick panning is recorded for playtesting (`ref.ctrl.camera.stick_gain`).
- **Amendment 2026-10-21 (O75; M1 task T14, [ADR-119](ADR-119-input-camera-picking-and-cursor-at-m1.md)).** M1 builds the router's subset with the bindings compiled in: the ordered event queue; `down`, `up` and `held` on keys and buttons, and `move` and `read` on the mouse's movement and wheel; and the device leaf, `all` and `chord` nodes. The game gives the router its actions and bindings, and the camera's rows of the default set are compiled in at the US and UK key positions. The name table, the parser and writer, the layout and user layers, repeat, the other node kinds, and the input mode and idle time wait for the menus and the settings store of M2. The property, parser and dead-zone tests of the Confirmation come with them.
