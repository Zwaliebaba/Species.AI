# Gap rules: engine

> Status: Engine baseline decisions · Range: DEC-950 to DEC-999 · Related: [decisions index](README.md), [simulation, terrain, rendering and assets](simulation_terrain_render_assets.md), [animation and VFX, audio engine and UI](vfx_audio_ui.md), [data, sequences, platform and diagnostics](data_sequences_platform_debug.md), [conventions](../CONVENTIONS.md)

This file records the engine-baseline rules of the engine components that no other register entry holds: rules that a component fixes where the reference is undefined, frame-rate dependent or defective; values that the reference does not record; and reference behaviours that a component keeps as its default but that need a decision of their own. Related rules of one component or mechanic are grouped into one entry with numbered items. Each entry states the rule the documents specify now. A **Resolved** entry states an engine-baseline rule that the component fixes; an **Open** entry states the current default, which is the reference behaviour wherever the reference defines one, with a recommendation.

## Index

| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-950](#dec-950-keyboard-operation-of-the-menus) | Keyboard operation of the menus | Resolved | UI, INP, R-META |
| [DEC-951](#dec-951-repeat-and-increment-timing-of-the-ui-and-of-input) | Repeat and increment timing of the UI and of input | Resolved | UI, INP |
| [DEC-952](#dec-952-ui-framework-reference-behaviours-kept-as-defaults) | UI framework reference behaviours kept as defaults | Open | UI |
| [DEC-953](#dec-953-input-back-end-rules) | Input back-end rules | Resolved | INP, R-CTRL |
| [DEC-954](#dec-954-input-reference-behaviours-kept-as-defaults) | Input reference behaviours kept as defaults | Open | INP |
| [DEC-955](#dec-955-presentation-clocks-while-the-world-is-held-and-camera-shake) | Presentation clocks while the world is held, and camera shake | Resolved | CAM, VFX, R-SIM, R-CTRL |
| [DEC-956](#dec-956-pick-ray-pixel-convention) | Pick-ray pixel convention | Open | CAM |
| [DEC-957](#dec-957-every-draw-sets-its-complete-pipeline-state) | Every draw sets its complete pipeline state | Resolved | RND |
| [DEC-958](#dec-958-renderer-reference-looks-kept-as-defaults) | Renderer reference looks kept as defaults | Open | RND |
| [DEC-959](#dec-959-renderer-values-the-reference-does-not-record) | Renderer values the reference does not record | Open | RND |
| [DEC-960](#dec-960-effects-advance-in-fixed-steps) | Effects advance in fixed steps | Resolved | VFX, R-CTRL, R-BLDG |
| [DEC-961](#dec-961-explainer-panels-lay-out-again-on-every-screen-change) | Explainer panels lay out again on every screen change | Resolved | VFX, R-META |
| [DEC-962](#dec-962-effect-values-the-reference-does-not-record) | Effect values the reference does not record | Open | VFX |
| [DEC-963](#dec-963-animation-and-vfx-reference-looks-kept-as-defaults) | Animation and VFX reference looks kept as defaults | Open | VFX, R-UNIT |
| [DEC-964](#dec-964-accessibility-limits-for-camera-shake-and-flashes) | Accessibility limits for camera shake and flashes | Open | VFX, RND |
| [DEC-965](#dec-965-audio-engine-reference-behaviours-kept-as-defaults) | Audio engine reference behaviours kept as defaults | Open | AEN |
| [DEC-966](#dec-966-terrain-query-edge-cases) | Terrain query edge cases | Open | TER |
| [DEC-967](#dec-967-reproducible-terrain-colours-and-no-regeneration-of-the-height-field) | Reproducible terrain colours and no regeneration of the height field | Resolved | TER, RND |
| [DEC-968](#dec-968-content-build-validation-of-terrain-map-and-record-data) | Content-build validation of terrain, map and record data | Resolved | TER, DAT, R-BLDG |
| [DEC-969](#dec-969-co-op-group-lists-in-ascending-position-order) | Co-op group lists in ascending position order | Resolved | DAT, R-MODE |
| [DEC-970](#dec-970-system-summary-and-crash-context-records) | System summary and crash-context records | Resolved | PLT, DBG |
| [DEC-971](#dec-971-reference-test-machine-and-minimum-specification-hardware) | Reference test machine and minimum-specification hardware | Open | DBG, RND, AST, BLD |
| [DEC-972](#dec-972-what-the-cheats-used-marker-suppresses) | What the cheats-used marker suppresses | Open | DBG, R-META |

## 1 UI framework and input

### DEC-950 Keyboard operation of the menus
- Status: Resolved
- Affects: architecture/components/ui_framework/layout_rendering_input_and_localisation.md (UI-203, UI-204), architecture/components/input.md (INP-077, table 3.6-B), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META-014); ADR-0032, ADR-0064
- Specified behaviour: Every screen is usable with the keyboard alone. The keyboard defaults of the menu actions are the arrow keys, with repeat, for `action.menu.up`, `action.menu.down`, `action.menu.left` and `action.menu.right`; Enter for `action.menu.activate`; and Esc for `action.menu.close` outside a match. In a match Esc stays `action.match.menu`, which closes every window at once (R-META-014). The gamepad bindings and the pointer are unchanged. The UI framework defines the actions and their context rules (UI-203, UI-204), and the input component states the defaults in its binding table (INP-077, table 3.6-B).
- Options: A — as stated; B — the reference behaviour: the default control scheme binds the menu actions to the gamepad only, and keyboard players navigate with the pointer; the arrow keys and Enter exist only in an alternative control set that is never selected.
- Recommendation: A. ADR-0032 requires keyboard-only operation of every screen, and the accessibility baseline of ADR-0064 lists it. The keys come from the reference's own alternative control set, so they add no new convention, and no mouse or gamepad binding changes.

### DEC-951 Repeat and increment timing of the UI and of input
- Status: Resolved
- Affects: architecture/components/ui_framework/core_and_widgets.md (UI-072, UI-077), architecture/components/ui_framework/layout_rendering_input_and_localisation.md (UI-216), architecture/components/input.md (INP-133); DEC-102, ADR-0029, ADR-0074
- Specified behaviour:
  1. **Clock.** Every repeat timer of the UI and of input bindings runs on real time read from the platform clock (ADR-0074), never on the presentation clock, which game speed scales and which stops while the world is held. Menus therefore repeat at the same rate while a match is paused and at every game speed.
  2. **Due times.** A repeat's next due time moves on by the repeat period from the previous due time, never from the frame in which the repeat fired, so the number of repeats over a hold does not depend on the frame rate, and a press and release within one frame still fire once (INP-133, ADR-0029).
  3. **Scroll arrows.** A held scroll-bar arrow steps once when it is pressed and then once every `ui.scroll.arrow_repeat_period` (1/60 s), with no initial delay (UI-077).
  4. **Float increments.** A held increment on a float or double value changes it at the rate change × `ui.increment.float_gain` (0.2) × elapsed × `ui.increment.float_reference_rate` (60) per second (UI-072).
- Options: A — as stated; B — the reference behaviour: the scroll arrows step once per rendered frame with no delay, and the float increment adds change × elapsed × 0.2 per rendered frame, so both run faster at higher frame rates; C — option A with the menu repeat on the presentation clock, so that menus stop repeating while a match is paused.
- Recommendation: A. DEC-102 corrects the frame-rate-dependent repeats, and the rates of items 3 and 4 are the reference rates at 60 frames per second, the rate that the reference feel was tuned at. Real time is the one clock that runs while a match is paused, when the pause menu needs its repeat.

### DEC-952 UI framework reference behaviours kept as defaults
- Status: Open
- Affects: architecture/components/ui_framework/core_and_widgets.md (UI-066, UI-082, UI-083), architecture/components/ui_framework/layout_rendering_input_and_localisation.md (UI-194), architecture/components/ui_framework/pages_and_screen_recipes.md (UI-401, UI-404)
- Specified behaviour: The UI framework keeps these reference behaviours as its defaults:
  1. **Caps Lock.** An upper-case letter typed into a text-type field is stored lower-cased unless Shift is held, so Caps Lock alone gives lower case (UI-066).
  2. **Automatic option values.** A drop-down option added without a value takes the next value of the drop-down's counter, which starts at 0 and returns to 0 when the list is cleared (UI-082). The reference does not record the counter's initial value, so 0 is the default.
  3. **Selecting an absent value.** `Select(value)` writes the bound value even when no option has it, and the caption then shows the drop-down's own label (UI-083).
  4. **Centred 3-D text.** Centred and right-aligned 3-D text is shifted by the product of the CJK term and the Latin term instead of their sum, so a label made of one script only is drawn left-aligned (UI-194 item 3).
  5. **Upward rotated text.** Text rotated a quarter turn to read upwards is drawn without its outline pass (UI-194 item 2).
  6. **Scrolling-text overflow.** The content height that decides whether a scrolling text scrolls uses the top margin ⌊`ui.scroll_text.top_margin` × h⌋ before it is raised to `ui.scroll_text.min_top_margin` × the font size, while the lines start at the raised margin, so in a small widget the last line can be clipped by up to 1.1 × the font size − ⌊0.05 × h⌋ without the text scrolling (UI-401, UI-404).
- Options: A — keep each behaviour, as stated; B — correct items 1 and 3 to 6: store letters as the platform delivers them; leave the binding unchanged when no option has the value; shift centred 3-D text by the sum of the two terms; draw the outline of upward text; use the raised margin in both places. Item 2 has no reference value to correct and keeps 0 under both options.
- Recommendation: B. None of these behaviours has play value, and no Species content relies on them: items 4 and 5 misplace or weaken labels, item 6 hides text, item 3 shows a label for a value the player never chose, and item 1 turns a typed upper-case letter into lower case, which text entry with input method editors (ADR-0028, ADR-0063) should not do.
- Decide by: design phase

### DEC-953 Input back-end rules
- Status: Resolved
- Affects: architecture/components/input.md (INP-111, INP-133, INP-145 to INP-149, table 3.2-A, table 3.6-A), reference/06_controls_camera_and_commands/actions_and_bindings.md (R-CTRL-033, R-CTRL-047); DEC-554, ADR-0028, ADR-0029
- Specified behaviour:
  1. **Gamepad assignment by the first press.** An unassigned, connected gamepad is claimed by its first button press for the lowest-numbered local player that has no assigned gamepad or whose assigned gamepad is disconnected, and the claiming press counts as that player's input in the same frame; stick or trigger movement alone never claims a gamepad. An assignment survives a disconnection, a reconnection of the same device resumes it, and a press on another unassigned gamepad while the assigned one is disconnected transfers the assignment. A player's connection actions follow its own assigned gamepad only (INP-145 to INP-149). This is the assignment rule of DEC-554 item 5; the number of local players is a design-phase input, and the reference has one.
  2. **No device input while unfocused.** When the window loses focus, both back-ends release every held control; while the window has no focus they deliver no presses and no analogue samples away from rest, and connection notices still apply. A control held when focus returns produces no press until it is released and pressed again (INP-111, ADR-0028).
  3. **Extensions that change no default binding.** `repeat` is accepted on every Boolean view, keys, mouse buttons and triggers as well as gamepad buttons (INP-133); the separate trigger-button controls are folded into each trigger's Boolean view (table 3.6-A); the `any` key and mouse control names are rejected at load, because they never fire; the name table adds F13 to F24, print screen, the application key and raw mouse motion (table 3.2-A); and the input component sets no fixed gamepad count.
- Options: A — as stated; B — the reference behaviour: any connected gamepad drives the single local player, the first whose input matches a binding winning (DEC-554 option B); gamepads are read whether or not the window has focus; repeat exists only on gamepad buttons; and the `any` names load and never fire.
- Recommendation: A. DEC-554 item 5 requires a per-player assigned gamepad, and assignment by the first press is the convention players expect. Reading gamepads while another window has focus sends input to a game the player is not using, and ADR-0028 already releases every held control on focus loss. The extensions only widen what a binding may state.

### DEC-954 Input reference behaviours kept as defaults
- Status: Open
- Affects: architecture/components/input.md (INP-082, INP-157, table 3.6-A)
- Specified behaviour: The input component keeps two reference behaviours as its defaults:
  1. **Aiming-stick pointer speed.** The aiming stick moves the pointer by its stick-unit value in every frame, so the stick's pointer speed in pixels per second is proportional to the frame rate (INP-157).
  2. **Camera height glyphs.** `action.camera.raise` shows the left-stick glyph and `action.camera.lower` the right-stick glyph, although both actions are bound to the triggers on the gamepad (INP-082).
- Options: A — keep both; B — scale the stick's pointer movement by the frame time at a reference rate of 60 frames per second, and show the trigger glyphs for the two height actions.
- Recommendation: B. Both are visible defects without play value: the pointer speed changes with the player's hardware, and the glyphs point the player at the wrong control.
- Decide by: design phase

## 2 Camera and presentation clocks

### DEC-955 Presentation clocks while the world is held, and camera shake
- Status: Resolved
- Affects: architecture/components/camera_and_sequences.md (CAM-012, CAM-030), architecture/components/animation_and_vfx/README.md (VFX-009, VFX-016), architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md (VFX-139), architecture/components/animation_and_vfx/particles_shatter_and_screen_effects.md (VFX-232), reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM-570, R-SIM-571, R-SIM-573), reference/06_controls_camera_and_commands/camera.md (R-CTRL-463); DEC-024, ADR-0036, ADR-0074
- Specified behaviour:
  1. **Three readings of one clock.** One presentation clock reads the platform clock (ADR-0074) once per frame and gives three readings: the scaled reading follows game speed and stops while the world is held; the unscaled reading stops while the world is held; the real reading never stops. A frame step is the real time since the previous frame, clamped to `ref.sim.frame.delta_clamp` (0.25 s) (VFX-016).
  2. **Readers.** Particles and the lifetimes of presentation-only effects read the scaled reading; shatter, procedural legs, cloth and glow pulses read the unscaled reading, so they hold still during a pause; explainer playback, the selection-circle rotation, UI animation and every repeat timer read real time (VFX-009, DEC-951).
  3. **Camera clock.** The camera clock advances by the real, clamped frame step, is never scaled by game speed and keeps advancing while the world is held, so the camera stays live during a pause (CAM-030). No camera or UI motion advances by a stale frame delta.
  4. **Cloth while held.** A cloth flag holds its shape while the world is held, because its step is the unscaled frame step (VFX-139).
  5. **Camera shake.** Shake is an additive rotation offset of the frame's view, computed from the shake value of the screen effects and never stored in the camera's pose, so it leaves no residue when it ends (VFX-232, CAM-012).
- Options: A — as stated; B — the reference behaviour: while the world is held the presentation frame delta keeps its last value, so the camera and the UI keep moving at the pace of the last frame before the pause (R-SIM-571); a cloth flag keeps relaxing with a step of `vfx.cloth.still_step` (0.1 s) per frame; and shake turns the camera's stored front and up in every frame, so in the static and scripted modes it leaves a random-walk residue in the pose (R-CTRL-463).
- Recommendation: A. DEC-024 asks for one presentation clock for camera motion and ADR-0036 defines it. A stale frame delta makes the paused camera's speed depend on the last frame before the pause, and a shake that changes the stored pose can leave the camera off its view after a scripted move has landed, which DEC-024 forbids.

### DEC-956 Pick-ray pixel convention
- Status: Open
- Affects: architecture/components/camera_and_sequences.md (CAM-090)
- Specified behaviour: The pick ray passes through the lower-left corner of the pointer's pixel: the pointer's row is offset by `cam.pick.row_offset` (1 px) before it is mapped to normalised device coordinates, so the ray lies one pixel below the pixel's top edge (CAM-090).
- Options: A — keep the reference offset; B — the pixel centre, an offset of 0.5 px in both axes.
- Recommendation: B. The offset is a by-product of a flip between window coordinate conventions and has no play value. The pixel centre is the convention of the Species overlay space (RND-215), so a pick then matches what the pointer is drawn over.
- Decide by: design phase

## 3 Renderer

### DEC-957 Every draw sets its complete pipeline state
- Status: Resolved
- Affects: architecture/components/renderer/entity_and_overlay_renderers.md (RND-303, RND-324), architecture/components/renderer/frame_graph_and_passes.md (RND-052, RND-055); ADR-0005
- Specified behaviour: Every draw of the entity and overlay passes names one complete pipeline-catalogue entry (ADR-0005, RND-052). An actor's draw never inherits a blend, depth, texture or other state that an earlier draw left; the renderer reproduces exactly the states that each actor kind lists (RND-303, RND-324).
- Options: A — as stated; B — the reference behaviour: the actors of passes 7a and 7b are drawn one after another, and an actor that sets no state of its own inherits what the previous actor left, so blending left on after an Abductor beam or a barrage missile makes later lit meshes blend in an order that depends on the draw order of the frame.
- Recommendation: A. Inherited state makes the look of an actor depend on which actor happened to be drawn before it, which changes with every spawn and removal. Complete catalogue entries are the form that pipeline state objects take anyway (ADR-0005).

### DEC-958 Renderer reference looks kept as defaults
- Status: Open
- Affects: architecture/components/renderer/world_renderers.md (RND-167), architecture/components/renderer/view_lighting_and_fog.md (RND-214), architecture/components/renderer/entity_and_overlay_renderers.md (RND-326), architecture/components/renderer/frame_graph_and_passes.md (RND-055)
- Specified behaviour: The renderer keeps these reference looks as its defaults:
  1. **Ownership lights.** A team's ownership lights show in the command frames n where ⌊(n mod `rnd.structure.light.blink_cycle`) ÷ `rnd.structure.light.blink_frames`⌋ equals the team's index. The quotient ranges from 0 to 4, so the team in slot 5, the Visitor faction's slot (DEC-082), never shows ownership lights (RND-167).
  2. **Poster field of view.** Poster tiles use the scale φ ÷ `rnd.view.poster_fov_divisor` (78) on both axes, so a poster frames the scene differently from the screen, narrower horizontally at 16:9 (RND-214).
  3. **Scaled centipede segments.** A centipede segment is lit with its uniform scale σ left in its normals, so a larger segment is lit darker and a smaller one brighter before the clamp (RND-326). Every other scaled actor mesh, ants included, and every scaled wall section is lit with unit normals (RND-055); the reference look of scaled walls is not recorded.
- Options: A — keep each look; B — correct each: a blink formula that covers every team slot; poster tiles that subdivide the on-screen frustum exactly; unit normals for centipede segments.
- Recommendation: B for items 1 and 2, which are defects without play value; item 3 after a visual comparison, because the shading of segments by size is part of how the reference centipede reads.
- Decide by: design phase

### DEC-959 Renderer values the reference does not record
- Status: Open
- Affects: architecture/components/renderer/world_renderers.md (RND-172, RND-173, RND-199, RND-200), architecture/components/renderer/entity_and_overlay_renderers.md (RND-324, RND-353)
- Specified behaviour: The renderer states these defaults where the reference records no value:
  1. **Barrier Emitter burn layer.** An emitter that is not solar-linked draws its burn layer with *t₀* = 0 and *th* = 1, one repeat without scrolling (RND-199, RND-200); the reference gives the range only for solar-linked emitters.
  2. **Clock of three cosmetic animations.** The Barrier Emitter scroll (the presentation frame time ÷ `rnd.barrier.scroll_period`), the jump-pad pulse and the Solar Array glow pulse read the presentation frame time (R-SIM-570), the clock of render-time animation (RND-172, RND-173, RND-199); the reference names no clock for the two pulses.
  3. **Crawler egg shadow.** A Crawler egg's blob shadow has the half-size `ref.unit.crawler_egg.card_size` ÷ 2, 4 wu (RND-324).
  4. **Route-line spots.** Route spots are placed one spot size apart, with arrow heads along each leg showing the direction (RND-353); the reference gives the spot size and its pulse, not the spacing or where the arrows go.
- Options: A — the stated defaults; B — values set by art direction in a visual review.
- Recommendation: A as the baseline, so that every draw is specified; revisit them in the look review of the design phase, where the route arrows also get their placement.
- Decide by: design phase

## 4 Animation and VFX

### DEC-960 Effects advance in fixed steps
- Status: Resolved
- Affects: architecture/components/animation_and_vfx/README.md (VFX-018), architecture/components/animation_and_vfx/particles_shatter_and_screen_effects.md (VFX-209), architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md (VFX-139), reference/06_controls_camera_and_commands/cursor_and_markers.md (R-CTRL-370), reference/03_buildings_and_world_objects/trees_and_crate_structures.md (R-BLDG-654); ADR-0036, DEC-106
- Specified behaviour: No effect depends on the frame rate (ADR-0036). A reference behaviour that runs once per rendered frame runs as a fixed-rate step on the unscaled presentation clock at `vfx.emit.step_rate` (60 steps per second): each step does what the reference does in one frame, a frame runs as many due steps as its clock advance holds, and the remainder carries to the next frame. A per-frame blend factor k becomes 1 − (1 − k)^(60 × Δ) for a frame step Δ (VFX-018). This covers per-frame emission, such as the control-flash particles that the selection circle creates every frame (R-CTRL-370) and the Allegiance Scrambler's flashes (R-BLDG-654), and per-frame blend factors, such as cloth relaxation (VFX-139). Particles integrate in sub-steps of `vfx.particle.sub_step` (1/60 s of scaled presentation time), at most `vfx.particle.max_sub_steps` (30) per frame, which covers a 0.25 s frame at game speed 2.0 (VFX-209); DEC-106 confirms the cap together with the particle caps.
- Options: A — as stated; B — the reference behaviour: per-frame emission and blending, so effect density and decay grow with the frame rate.
- Recommendation: A. ADR-0036 requires effects that do not depend on the frame rate, and 60 steps per second reproduce each reference effect exactly at 60 frames per second.

### DEC-961 Explainer panels lay out again on every screen change
- Status: Resolved
- Affects: architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md (VFX-108), reference/09_front_end_and_match_flow/front_end_presentation.md (R-META-224); ADR-0033
- Specified behaviour: An explainer panel's place and size are those of R-META-224. The panel is a widget of the UI framework and is laid out again whenever the screen size, the DPI or the text scale changes (VFX-108, ADR-0033).
- Options: A — as stated; B — the reference behaviour: the place and size are computed once, when the panel is created, so a panel open across a resize keeps the old layout.
- Recommendation: A. ADR-0033 lays every widget out again on these changes, and a panel computed once is drawn in the wrong place after a resize.

### DEC-962 Effect values the reference does not record
- Status: Open
- Affects: architecture/components/animation_and_vfx/particles_shatter_and_screen_effects.md (VFX-219, table 4.7-A; VFX-229), architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md (VFX-122), architecture/components/animation_and_vfx/README.md (VFX-025)
- Specified behaviour: The animation and VFX component states these defaults where the reference records no value:
  1. **Emitters** (table 4.7-A): `vfx.emit.control.count` (1) control flash when a Worker is taken into or released from crowd control; the demotion burst reuses the promotion burst; one blue spark per tick of a Worker opening a Supply crate; the fireball-launch flame sizes of `vfx.emit.fireball.*`; `vfx.emit.round_hit.terrain_count` (1) trail when a turret round meets the terrain; the spiralling rocket's burst uses the explosion-core rule in one team colour drawn per burst; an order marker emits once per tick.
  2. **Shard edges:** the edge line width `vfx.shatter.edge_width`, 1/64 in barycentric units (VFX-229).
  3. **Eaten-orb display:** the orbit offset `vfx.serpent.eaten_offset`, ±10 wu on each axis (VFX-122).
- Options: A — the stated defaults; B — values set by art direction in a visual review against the visual language of DEC-029.
- Recommendation: A as the baseline, so that every effect is specified; revisit them in the look review of the design phase.
- Decide by: design phase

### DEC-963 Animation and VFX reference looks kept as defaults
- Status: Open
- Affects: architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md (VFX-126, VFX-138, VFX-139), architecture/components/animation_and_vfx/README.md (VFX-025), reference/02_units/README.md (R-UNIT-051)
- Specified behaviour: The animation and VFX component keeps these reference looks as its defaults:
  1. **Brood Drifter tails.** One smoothed tail velocity is shared by every Brood Drifter and updated once per drawn Brood Drifter at every effect step, so the tails of all Brood Drifters drag the same way (VFX-138).
  2. **Large and selected flags.** For a cloth flag larger than `ref.bldg.cloth_flag.damped_size_wu`, or highlighted or selected, the fixed factor `ref.bldg.cloth_flag.damped_factor` (0.7) replaces the per-column relaxation factor instead of scaling it, so such a flag looks stiffer (VFX-139).
  3. **Damage-flicker cadence.** An object's damage-flicker flag is evaluated once per `vfx.flicker.step`, one `ref.sim.tick_period` (0.1 s) of scaled presentation time (VFX-126); reference/02 R-UNIT-051 describes the flicker as decided in each render frame.
  The frame-quantised explainer loop is DEC-750, and the saturated bubble colours are DEC-328.
- Options: A — keep each look; B — correct each: one smoothed velocity per Brood Drifter; scale the per-column factor; evaluate the flicker at every effect step of DEC-960.
- Recommendation: B for item 1, whose shared state makes one Brood Drifter's tails follow another's motion; A for items 2 and 3, which are part of the reference look. The two documents of item 3 state the cadence of the reference differently, and the one that is wrong is corrected.
- Decide by: design phase

### DEC-964 Accessibility limits for camera shake and flashes
- Status: Open
- Affects: architecture/components/animation_and_vfx/particles_shatter_and_screen_effects.md (VFX-232, VFX-235), architecture/components/animation_and_vfx/README.md (VFX-025), architecture/components/renderer/visual_language.md (RND-248); ADR-0064
- Specified behaviour: With reduced motion, the camera-shake offset is scaled by `vfx.screen.reduced_motion_scale` (0.25) (VFX-232). A flash whose alpha reaches `vfx.screen.flash_count_threshold` (0.1) counts as an onset; at most `vfx.screen.flash_onsets` (3) onsets are shown in any `vfx.screen.flash_window` (1 s) of real time; and reduced flashing multiplies every flash alpha by `vfx.screen.reduced_flash_scale` (0.25) (VFX-235, RND-248). No setting turns camera shake off entirely. The reference has none of these settings; they are the engine baseline under ADR-0064, which is proposed.
- Options: A — as stated; B — option A, plus a setting that turns camera shake off entirely; C — values set by an accessibility review against the platform's accessibility guidelines.
- Recommendation: B, with the values reviewed as in C before release. Turning shake off is a common accessibility setting and costs one multiplier of 0; the flash limits protect photosensitive players at no cost to the look.
- Decide by: design phase

## 5 Audio engine

### DEC-965 Audio engine reference behaviours kept as defaults
- Status: Open
- Affects: architecture/components/audio_engine.md (AEN-133, AEN-138, AEN-140, AEN-178)
- Specified behaviour: The audio engine keeps these reference behaviours as its defaults:
  1. **Next-note timing.** When a `note` instance is released not finally and its loop delay is not below ε, its next note is created at the first wrap of the releasing note's member at or after the re-trigger time + the loop delay, not exactly when the delay ends (AEN-178).
  2. **Audibility during an attack.** An instance that has ever been real ranks by its current envelope level, so a sound in a slow attack ranks low and can be stolen (AEN-140).
  3. **Shared note layers.** A duplicate of a shared note layer merges into the note it was copied from and plays no next note (AEN-133).
  4. **No pause.** A paused match does not pause audio: every sound keeps playing and the interface keeps raising its sounds (AEN-138).
- Options: A — keep each behaviour; B — start the next note exactly when the delay ends; rank an instance in its attack by its level target; reject a layer that combines `note` with shared sharing in the content build; pause the world-sound buses while a match is paused and keep the interface bus live.
- Recommendation: B for items 2 and 3, which lose sounds without intent; items 1 and 4 by listening test, because the note timing is part of the reference loops' rhythm and a pause rule depends on the pause model of DEC-264.
- Decide by: design phase

## 6 Terrain

### DEC-966 Terrain query edge cases
- Status: Open
- Affects: architecture/components/terrain.md (TER-017, TER-076)
- Specified behaviour: The terrain component states these defaults where the reference result is not established:
  1. **Axis-parallel rays.** The grid walk of a ray whose x or z component is zero visits only the column or the row it lies in (TER-017).
  2. **Highest sample of a non-square grid.** `Highest()` bounds both of its loops by the row count (TER-076). This is exact on a square grid, and every tile and every reference terrain is square; on a grid with more columns than rows the scan ignores the last columns, and with fewer columns it reads the outside value.
- Options: A — the stated defaults; B — item 1 reproduces the reference's quadrant branch for a zero component once it is established, and item 2 scans columns × rows.
- Recommendation: A for item 1, which visits exactly the cells that the ray crosses; B for item 2, which only a non-square Species terrain can reach (DEC-247) and whose reference scan gives a wrong highest sample there.
- Decide by: design phase

### DEC-967 Reproducible terrain colours and no regeneration of the height field
- Status: Resolved
- Affects: architecture/components/terrain.md (TER-185, TER-186, TER-187, TER-221), architecture/components/renderer/world_renderers.md (RND-031); DEC-013, DEC-246
- Specified behaviour:
  1. **Colour chains.** The colour pass of one renderer build owns its own presentation generator instance, never shared with another consumer or thread, seeded with `ref.sim.match.presentation_seed` at the start of every build. Each triangle draws a value, re-seeds the instance from its centre and that value and draws its jitter, so the colours form a chain in build order; the shore noise of each sea-detail level draws from its own copy of the instance taken at the end of the colour pass. Every build of the same field, after device removal included, therefore gives the same colours and shore noise, with the reference distributions (TER-185, TER-186). A stateless function of position and seed may replace the chains if the design phase so decides (TER-187, DEC-246 item 11).
  2. **No regeneration.** The height field is never regenerated during a match and never under a different numeric environment; applying graphics or other options rebuilds presentation data only (TER-221, RND-031).
- Options: A — as stated; B — the reference behaviour: one shared generator serves the terrain colours, audio, tree geometry and a service thread, so concurrent draws can change the colours, and applying options in the middle of a match regenerates the field and the sea, which redraws the colours without re-seeding.
- Recommendation: A. A colour field that changes when an option is applied or when another thread draws is a visible defect, and a height field rebuilt in the middle of a match is a determinism hazard (DEC-246 item 8). Simulation-observable behaviour is unchanged, because the reference regeneration gives identical heights.

### DEC-968 Content-build validation of terrain, map and record data
- Status: Resolved
- Affects: architecture/components/terrain.md (TER-072, TER-112, TER-124, TER-250), architecture/components/data_schemas/map_structure_records.md (DAT-150, DAT-159, tables 3.1-A and 3.2-A), reference/03_buildings_and_world_objects/trees_and_crate_structures.md (R-BLDG-633, R-BLDG-645); DEC-014, DEC-015, DEC-241, DEC-247
- Specified behaviour: The content build (ADR-0059) applies these rules, each with a diagnostic naming the map and the record (DEC-015):
  1. **Terrain records** (TER-250). It rejects a cell size that is not finite and above 0; a world size below 1 wu; a grid of more than `ter.grid.max_samples_per_side` (32 768) columns or rows, the size up to which the index semantics of TER-072 hold (reference grids have at most 334 samples per side, DEC-247); a decimal field that is not finite; a whole-number field outside the 32-bit signed range; a tile smaller than 2 cell sizes, a generation method other than 0, 1 or 2, and a guide grid whose cell count is not (2^*p* − 1)² (TER-112); a tile whose start index is −1 or below while its footprint overlaps the grid (TER-124); a flatten type outside 0 to 4; a ramp that is not 64 × 64; and a ramp, gradient or sea-tile file number that the asset manifest does not hold. It warns about a tile with *k* < *p* and a tile whose footprint lies wholly outside the grid.
  2. **Barrier Emitter rings.** A ring of Barrier Emitters whose every emitter starts in `relay_only` mode is a build error, because a forwarded request would circle the ring without end; every other loop of a chain is valid data (DAT-150, DEC-014).
  3. **Hive contents.** A Hive's ants-inside value outside [0, `ref.bldg.hive.map_ants_max`] is a build error, so the run-time replacement by 0 of R-BLDG-633 never meets validated content (DAT-159).
  4. **Spitter Plant reload.** A Spitter Plant's reload satisfies ⌊reload ÷ 10⌋ ≥ 1, so the first-deadline draw of R-BLDG-645 has at least one value (map_structure_records.md table 3.2-A).
  5. **Zero facing.** A structure facing of zero length is valid data and is normalised by the degenerate-vector rule of DEC-241 item 6 (map_structure_records.md table 3.1-A).
- Options: A — as stated; B — the reference behaviour: each of these inputs is accepted, and its result is undefined or silent: the index semantics fail above 32 768 samples per side, the tile results of TER-112 and TER-124 are undefined, a relay-only ring forwards a request without end, an out-of-range ants value is replaced by 0 at run time, and a reload below 10 ticks leaves the first-deadline draw without a range.
- Recommendation: A. None of these inputs occurs in the reference content, so validation changes no reference behaviour, and each turns an undefined or endless result into a diagnostic with a location.

### DEC-969 Co-op group lists in ascending position order
- Status: Resolved
- Affects: architecture/components/data_schemas/map_schema.md (DAT-087, DAT-088), reference/01_skirmish_rules_and_modes/README.md (R-MODE-020); DEC-560
- Specified behaviour: A map's group-1 and group-2 position lists are derived from its start-position records, each list in ascending position order, and each co-op group holds at most 2 positions (DAT-087, DAT-088). Alliance assignment therefore reads G1[0] and G2[0] as the lower position of their group (R-MODE-020).
- Options: A — as stated; B — the reference form: the map lists the group positions explicitly, and alliance assignment reads them in the order authored.
- Recommendation: A. With typed start-position records (DEC-560) each position names its group, so the order of a group's list is derived, not authored, and ascending order is the one derived order that a validator can state. No reference map is reproduced (DEC-063), so no content depends on an authored order.

## 7 Platform and diagnostics

### DEC-970 System summary and crash-context records
- Status: Resolved
- Affects: architecture/components/platform_layer.md (PLT-035, PLT-093, PLT-119, PLT-125), architecture/components/diagnostics_test_and_performance.md (DBG-047); ADR-0065, ADR-0066
- Specified behaviour:
  1. **Never fatal.** A system-summary query that fails leaves its field marked unknown and is written to the log; it never stops the program (PLT-093).
  2. **No personal data.** The system summary and the crash-context record never hold the Windows user name, chat text, passwords or account tokens, and every path in them is written relative to the install directory or the user-data root, with the root shown as a placeholder (PLT-035, PLT-125). The black box follows the same rule (DBG-047), and memory that may hold personal data is excluded from crash dumps (PLT-119, ADR-0065).
- Options: A — as stated; B — the reference behaviour: a failed locale query stops the program, and the crash log records the user name and the program path.
- Recommendation: A. A diagnostic record must never be the reason a program fails to start, and a crash report that carries an account name or a home-folder path leaks personal data into every report that a player sends (ADR-0066).

### DEC-971 Reference test machine and minimum-specification hardware
- Status: Open
- Affects: architecture/components/diagnostics_test_and_performance.md (DBG-205, DBG-208, DBG-209, `dbg.machine.developer_class`), architecture/components/renderer/options_and_budgets.md (RND-454), architecture/components/asset_pipeline_and_resources.md (AST-125), architecture/components/build_tooling_and_distribution.md (BLD table 5-A); DEC-106, DEC-228, ADR-0002, ADR-0050
- Specified behaviour: Every performance budget of the component specifications refers to one pinned and recorded reference test machine: a processor, a memory size, a graphics adapter, a driver version, a display mode and an OS build at the minimum of ADR-0050 and ADR-0002, run as a pinned CI configuration whose hardware is recorded with each result; a change of the machine re-baselines every recorded result (DBG-209). The renderer's client target is measured on minimum-specification hardware (RND-454), and the developer machine class of the build budgets is the reference test machine with a local solid-state disk (`dbg.machine.developer_class`). The exact hardware is not named. The reference defines no test machine and records no measured frame time (DEC-228).
- Options: A — as stated: a pinned CI configuration at the ADR minimum, named once the first prototype is measured; B — a named minimum-specification hardware configuration chosen now and bought for the purpose; C — budgets per hardware tier (minimum and recommended), with a machine for each tier.
- Recommendation: A until the first measurement of the prototype (DEC-106, DEC-229), then B for the minimum tier. Budgets without a named machine cannot fail, but a machine chosen before any measurement fixes the population cap by guesswork.
- Decide by: design phase

### DEC-972 What the cheats-used marker suppresses
- Status: Open
- Affects: architecture/components/diagnostics_test_and_performance.md (DBG-118), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META: achievement, leaderboard and statistics hooks); DEC-041, DEC-047
- Specified behaviour: The first accepted debug command of a session sets its cheats-used marker, which the lobby and the match results show (`ui.match.cheats_used`) and the match statistics carry (DBG-118). Nothing is suppressed: the achievement, leaderboard and statistics hooks run as in any other session.
- Options: A — as stated: show and record the marker only; B — also suppress achievement awards, leaderboard submissions and statistics for a session with the marker; C — suppress achievement awards and leaderboard submissions, and keep the statistics with the marker.
- Recommendation: C. Achievements and leaderboards compare players, so results reached with debug commands must not enter them, while statistics with the marker still serve testing. The hooks are designed with DEC-047.
- Decide by: design phase
