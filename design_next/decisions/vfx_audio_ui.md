# Decision register: animation and VFX, audio engine and UI framework

> Status: Reference decisions · Range: DEC-750 to DEC-799 · Related: [decisions index](README.md), [verdicts and fixes](verdicts_and_fixes.md), [conventions](../CONVENTIONS.md)

This file records the decisions on animation and visual effects, the audio engine and the UI framework that no other register file holds. Most decisions of this scope exist already, and this file cites them instead of repeating them. The simulation-integrity rules (presentation never writes simulation state, simulation time comes only from the tick counter, tree hit volumes are asset data, cosmetic and audio randomness comes from the presentation generator, content is validated at build time) are in `decisions/verdicts_and_fixes.md`, together with the scope verdicts for the audio design and the visual language, the audio corrections, the localisation fallback, the seasonal theme and the UI framework corrections. The mix, the effect model and the game-speed pitch bend are in `decisions/controls_bots_maps_meta_audio.md`. The simulation and presentation boundary and the generator algorithm are design-phase topics under `decisions/deferred/`. The engine technology of these components (procedural animation on the presentation clock, effect events, virtual voices, buses and per-bus effects, the audio command queue, text input with input method editors, the focus graph, layout and data schemas) is decided by the ADRs under `adr/`. The confirmations that the reference behaviour of these components needs are in [open questions](open_questions.md). Each entry below states the behaviour the documents specify now.

## Decisions of this scope recorded elsewhere

| Topic | Decision |
|---|---|
| Presentation never writes simulation state; rules emit presentation events; a presentation flag such as the damage flicker is never stored in a simulated object | DEC-007 |
| Simulation time comes only from the tick counter; no actor behaviour reads the wall clock | DEC-009 |
| Tree hit volumes are asset data, never a by-product of render geometry | DEC-012 |
| Cosmetic effects, audio variation and other presentation randomness draw from the presentation generator; one draw per statement, none in argument lists | DEC-013 |
| Content is validated at build time and no load path is fatal, including the explainer-animation loader | DEC-015 |
| Audio design: binding model kept; authored priority classes, virtual voices and per-bus effects | DEC-027 |
| Visual language kept, including procedurally animated spider legs | DEC-029 |
| Actor kinds that no match creates | DEC-062 |
| Audio corrections, including voice stealing that compares priorities | DEC-097 |
| Localisation fallback per key to the base language | DEC-099 |
| Seasonal theme; the seasonal state is decided once per match | DEC-101, DEC-118 |
| UI framework corrections by construction | DEC-102 |
| Crowd-sound audibility radius | DEC-109 |
| Dry mix or applied effects | DEC-567 |
| Game-speed pitch bend of world sounds | DEC-569 |
| Buses, priority classes, voice budgets and start timing | DEC-570 |
| Simulation and presentation boundary | DEC-222 |
| Simulation generator algorithm; one presentation generator per thread | DEC-242 |
| Audio engine isolation and the presentation generator for audio | ADR-0022 |
| Virtual voices, ranking, steal margin and one-shot patience | ADR-0023 |
| Bus graph, per-bus effect chains and effect time constants at the bus rate | ADR-0024 |
| Audio command queue, fixed control rate, asynchronous results and time-based smoothing | ADR-0026 |
| Text entry with input method editors | ADR-0028, ADR-0063 |
| Focus graph, pointer capture, held-button repeat, Back handling and text fields with caret, selection and clipboard | ADR-0032 |
| Relayout on resize, DPI, scale or language change | ADR-0033 |
| Procedural animation on the client and the presentation clock; two-bone leg solving; explainer animations as a UI timeline | ADR-0035 |
| Effect events, the presentation clock and effect definitions | ADR-0036 |
| Data records bound by field name, with explicit defaults | ADR-0061 |
| Engine gap rules of the UI framework and input: keyboard operation of the menus, repeat and increment timing, UI and input reference behaviours kept as defaults, input back-end rules | DEC-950 to DEC-954 |
| Engine gap rules of animation and VFX: presentation clocks while the world is held and camera shake, fixed effect steps, explainer relayout, effect values and looks, accessibility limits | DEC-955, DEC-960 to DEC-964 |
| Audio engine reference behaviours kept as defaults | DEC-965 |

## Index

| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-750](#dec-750-explainer-animation-loop-and-visibility-rules) | Explainer-animation loop and visibility rules | Open | VFX, R-META, DAT |
| [DEC-751](#dec-751-game-speed-pitch-state-starts-every-match-at-normal-speed) | Game-speed pitch state starts every match at normal speed | Resolved | AEN, R-AUD |
| [DEC-752](#dec-752-audio-queries-and-reused-voices-carry-no-stale-state) | Audio queries and reused voices carry no stale state | Resolved | AEN |

## 1 Animation and VFX

### DEC-750 Explainer-animation loop and visibility rules
- Status: Open
- Affects: architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md (VFX), reference/09_front_end_and_match_flow/front_end_presentation.md (R-META), architecture/components/data_schemas/entity_audio_geometry_and_panel_schemas.md (DAT); DEC-963
- Specified behaviour: An explainer animation plays its keyframed objects on a timeline with a loop start and a loop end. The first pass starts at time 0, even when the loop start is later, so the span before the loop start plays once, as a lead-in; every later pass restarts at the loop start. When a frame's advance carries the time past the loop end, the excess is discarded and the next pass begins exactly at the loop start, so each pass lasts the loop length plus up to one frame. An object is hidden while the current time lies before its first key; authors rely on this to make objects appear part-way through a pass. Keys after the loop end are never reached; at least one reference explainer animation of each of the six modes holds such keys. Colour roles are resolved against the local player's team colour at draw time (ADR-0035).
- Options: A — the reference rules as specified; B — keep the lead-in and the hidden-before-first-key rule as named authoring rules, carry the excess past the loop end into the next pass so that the loop length does not depend on the frame rate, and make the content build warn about keys after the loop end; C — option B, and also start every pass, the first included, at the loop start.
- Recommendation: B. The lead-in and the late appearance are authoring tools that the reference content relies on, so they stay as rules. A loop whose length depends on the frame rate contradicts the presentation-clock rule that every other effect follows (ADR-0036), and a key that can never play is an authoring mistake that a validator catches cheaply. Every explainer animation is authored new, so no content depends on the frame-quantised loop.
- Decide by: design phase

## 2 Audio engine

### DEC-751 Game-speed pitch state starts every match at normal speed
- Status: Resolved
- Affects: architecture/components/audio_engine.md (AEN), reference/10_audio_design/positional_priorities_and_mix.md (R-AUD)
- Specified behaviour: The smoothed game-speed value that bends the pitch of world sounds (DEC-569) belongs to the match. It is 1.0 outside a match and throughout the grace period, it starts every match at 1.0, and it is discarded when the match ends. Its smoothing follows elapsed time that game speed does not scale, with one time constant, an audio engine tuning value whose starting value is 0.17 s, the time constant of the reference smoothing with 30 world sounds playing; the speed of the smoothing never depends on how many sounds are playing (ADR-0026). Nothing a match does to game speed, the slow-down at match end included, carries into the next match.
- Options: A — per-match state with time-based smoothing; B — the reference behaviour: one process-wide value that is never reset and moves 1 % of the way towards the current game speed on every pitch evaluation of a playing world sound and on every voice start, so its time constant ranges from about 0.17 s with 30 world sounds playing to about 5 s with one; because the slow-down at match end drives the value towards its floor, the next match can begin its world-sound bend near 0.1× pitch.
- Recommendation: A. State that survives between matches makes one match depend on the one before it, the failure class that DEC-011 removes from the simulation. A smoothing speed that depends on the number of playing sounds makes the same speed change sound different in a quiet scene and in a battle.

### DEC-752 Audio queries and reused voices carry no stale state
- Status: Resolved
- Affects: architecture/components/audio_engine.md (AEN)
- Specified behaviour: A count of the instances of a sound event, for one emitter or for any emitter, counts only live instances and is defined in every voice state, including a voice that holds no instance; it never fails. Game code receives such results from the audio engine's result queue (ADR-0026). A voice given to a new instance starts from that instance's own settings only: no filter, send level or effect of an earlier instance remains on it. Switching effects off (DEC-567) removes their processing at once, and no voice or bus keeps processing an effect that is no longer assigned to it.
- Options: A — the corrected rules; B — the reference behaviour: counting the playing instances of an event for one emitter fails as soon as it meets a voice that holds no instance (every reference caller avoids this by asking for any emitter), and removing all effects from the live instances leaves the effect processing attached to their voices, where it keeps colouring later, unrelated sounds until a sound with effects starts on that voice.
- Recommendation: A. The first is a crash defect and the second an audible one. Both disappear when the audio engine owns the full state of each voice and resets it on reuse, which the bus model of ADR-0024 makes simple, because no voice carries an effect chain of its own.

## 3 UI framework

Every UI framework decision of this scope is recorded elsewhere: the window-core quirks and the 33 framework defects are corrected by construction (DEC-102, ADR-0032, ADR-0033), a missing phrase falls back per key to the base language (DEC-099), and text entry supports input method editors, a caret, selection and the clipboard (ADR-0028, ADR-0032, ADR-0063). The keyboard operation of the menus, the repeat and increment timing and the UI reference behaviours kept as defaults are entries of the [engine gap rules](engine_gap_rules.md) (DEC-950 to DEC-952).
