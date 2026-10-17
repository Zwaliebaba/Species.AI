# C-INP report: input component

## Files written

| File | Bytes | Requirements | Tuning keys | Diagrams | Acceptance tests |
|---|---|---|---|---|---|
| `design_next/architecture/components/input.md` | 115,440 | 134 (INP-001 to INP-244, increasing, with gaps by section) | 18 (`inp.*`) | 6 | 31 (T1 to T31) |

Scripts: `species_work/scripts/C-INP_tables.py` (the default binding set as data; generates tables 3.6-A, 3.6-B and the reverse index 3.8-A, so they agree), `species_work/scripts/C-INP_check.py` (ids, keys, cited INP/R-*/DEC/ADR/asset/`PlayerCommand` ids, action ids against reference/06 table 2.1-A, headings, truncation words).

Checks: forbidden scan 0 hits; soft scan 0 hits; `C-INP_check.py` 0 problems (every cited `ref.*` key and R-* requirement exists in `design_next/`; every one of the 70 action ids of reference/06 has a catalogue row, `action.app.quit` stated as a platform close request); 6 of 6 Mermaid blocks parse with mermaid 11.4.1 (`G06_mermaid.mjs`); template headings present; file under 120 KB.

## Coordinator follow-up (C-UI menu actions)

Applied after the coordinator's note. input.md defines no UI navigation action of its own: INP-003, INP-009 and INP-077 cite the UI framework's UI-203 (the nine `action.menu.*` actions, their meanings and context rules) and UI-204 (the keyboard defaults: arrows, Enter, Escape outside a match). Rows 1 to 9 of table 3.6-B only state those defaults as binding nodes (order and primary binding), and their "Defined by" column cites UI-203 and UI-204. The only input-side additions are the five leaderboard hook-page actions in rows 10 to 14 (`action.menu.cycle_up` = `PadMenuCycleUp`, `action.menu.cycle_down` = `PadMenuCycleDown`, `select_previous`, `select_next`, `toggle_period`). They cite R-META-500 to R-META-502 and DEC-553, because no UI requirement lists them; C-UI or G09b may adopt them, or the coordinator may move them. Re-checked: `C-INP_check.py` 0 problems, both pattern scans 0 hits, `accept_package.py` PASS.

## Counts

- Requirements 134; tuning keys 18; diagrams 6 (frame sequence 4.1-A, query evaluation 4.9-A, gamepad-slot state machine 4.10-A, layer build 4.13-A, binding graph 4.17-A, class diagram 3.10-A); acceptance tests 31.
- Enumerations (about 30): value kinds (3); conditions (9); keyboard names (113 in 8 groups); mouse controls (6); gamepad controls (25); conditions per control kind; node kinds (9) and their nesting order; acceptance by action kind (3); JSON node shapes (11); match-action catalogue (69 rows plus `action.app.quit`); UI navigation and hook-page actions (14); glyphs by control (9); reverse index (67 physical inputs); router operations (11); helpers (4); start-up steps (6); `AdvanceFrame` steps (10); keyboard rules (5); mouse rules (5); stick pipeline (4 steps); repeat steps (3); pointer steps (5); keyboard-layout table (5); layout layers (8 actions × 2 layouts); rebinding-page actions (11); load diagnostic classes (14); run-time failures (7); fixed input-layer rules (30, one per reference defect X1–X30); design-phase inputs (7).

## Open DECs cited

DEC-203 and DEC-250 (the package's Open entries). Also cited: Open DEC-044, DEC-046, DEC-047, DEC-050, DEC-202; Resolved DEC-004, 007, 008, 009, 015, 023, 024, 083, 095, 096, 099, 119, 553, 554, 555, 564, 800; Dropped DEC-054, 055, 056, 338.

## New DEC requests

1. **Gamepad assignment by first press** (INP-146 to INP-149). DEC-554 item 5 requires a per-player assigned gamepad but no assignment rule. Specified: an unassigned gamepad is claimed by the first button press for the lowest-numbered local player without a (connected) gamepad; the claiming press counts; the assignment survives disconnection; a press on another gamepad while the assigned one is disconnected transfers it; `connection` (and so `action.device.pad_disconnected`/`pad_connected`) follows the player's own gamepad only. Please record as a Resolved addendum to DEC-554, or open a design-phase DEC with the local-player count (§8.2 item 1).
2. **Pointer speed of the aiming stick** (INP-157). The right stick adds its stick-unit value to the pointer in every frame, so its pointer speed scales with the frame rate. Kept as the reference rule; propose an Open DEC (option B: scale by frame time at a reference rate).
3. **Glyphs of the camera height actions** (INP-082). The reference assigns a left-stick glyph to the right-trigger raise action and a right-stick glyph to the left-trigger lower action. Kept; propose an Open DEC (option B: the trigger glyphs).
4. **Gamepads while unfocused** (INP-111). ADR-0028 releases every held key and button on focus loss; to keep that consistent the gamepad back-end also delivers no presses while the window is unfocused. The reference read gamepads regardless of focus. Please record (Resolved under ADR-0028, or Open).
5. **Primary-binding order** (INP-075). Within an action the keyboard and mouse bindings are taken to precede the gamepad bindings (the reference table gives them in separate columns). It matters only for the layout layer, the user layer and the rebinding hook; the layout layers confirm it for the camera keys. Reviewer (R1) check against the binding-file line order.
6. **Keyboard bindings of the menu actions** (INP-077): arrows with repeat, Enter and Escape (outside a match) on the six navigation actions are Species baseline bindings taken over from C-UI's UI-204 (ADR-0064); the reference binds them on the gamepad only. Record with DEC-050 or ADR-0064 if a register entry is wanted.
7. **Engine extensions, no default behaviour change** (ADR-0029): `repeat` is accepted on every Boolean view (keys, mouse buttons, triggers as well as gamepad buttons); repeat due times advance by the period, so repeats are independent of the frame rate and a press shorter than a frame still fires once; the `any` key and mouse control names are rejected (they never fired); the separate trigger-button controls are folded into the trigger's Boolean view (`action.direct.fire` = `pad:trigger_right:held`); F13 to F24, `print_screen`, `application` and `mouse:raw_motion` are added to the name table; no fixed gamepad count. Record if the reviewers want a DEC.

## ADR conflicts and clarifications

1. **GameInput scope.** The coordinator's brief asks for the input architecture "on GameInput: devices, keyboard, mouse, gamepads". ADR-0028 (Proposed, default option 1) uses GameInput for gamepads only, and window input forwarded by the platform layer for keyboard, mouse and text. input.md follows ADR-0028, does not name the window-message mechanism, and keeps both back-ends behind one `DeviceBackEnd` interface (INP-105), so ADR-0028 option 2 (GameInput for every device) can be adopted without changing the router (§8.2 item 6).
2. **Repeat clock.** ADR-0029 measures repeat timers "in presentation time", while ADR-0036 defines the presentation clock as scaled by game speed and stopped while paused. Menu repeat must work while a match is paused, so input timings use the unscaled real-time platform clock (ADR-0074). Suggest rewording ADR-0029 to "real time".
3. **Camera drag premise.** ADR-0028's context names camera drag as the reason for raw mouse deltas; the reference match has no camera drag (the camera follows the pointer). `mouse:raw_motion` exists with no default binding; `action.camera.aim` uses pointer pixels. No conflict; the ADR wording could be tightened.
4. **Feel change from the radial dead zone** (ADR-0029, accepted). The reference applied the cubic curve per axis after a square dead zone, so a full diagonal deflection gave about half the cardinal magnitude (about 19.7 instead of 39.3 stick units on the left stick). The radial, magnitude-based curve gives the full scale in every direction. Flag for playtesting of stick pan speed (`ref.ctrl.camera.stick_gain`).

## Missing assets

- `glyph.pad.stick_left`, `glyph.pad.stick_right` (overrides of `action.camera.pan_stick`, `action.camera.raise`, `action.camera.lower`, `action.camera.aim`, `action.direct.move_stick`) and `glyph.pad.dpad_horizontal` (override of `action.menu.left`), with a shadow each if the set keeps shadows. Until they exist the content build leaves those overrides out (INP-081, INP-222).
- New string ids (patterns) for X-AST pass 2: `ui.input.key.<name>` (113 names of table 3.2-A), `ui.input.mouse.<name>` (6), `ui.input.pad.<name>` (25), `ui.input.condition.<condition>` (9), `ui.input.not` (1).

## Cross-document requests

- **G06 (`reference/06_controls_camera_and_commands/`).**
  (a) Table 2.1-A gives `action.camera.zoom_out` as "C, down"; the reference binding is held (`key:c:held`, table 3.6-A). No rule reads it (row A11); please align.
  (b) Answers to your requests: the default binding of `action.direct.move_direction_change` is `pad:stick_left:move`, with no keyboard binding (table 3.6-A); stick units: left full deflection `inp.pad.stick_left.scale` (39.304), right `inp.pad.stick_right.scale` (19.683), forward negative y (INP-128, INP-183); the pointer is held inside the window by `ConfinePointer` (INP-159) and moved without delta by `WarpPointer` for the cursor lock (INP-158); per-player gamepad assignment is INP-145 to INP-149.
  (c) The UI navigation and leaderboard actions `action.menu.*` (table 3.6-B, 14 ids, nine of them shared with C-UI) extend the `action.<group>.<name>` namespace for what R-CTRL-035 leaves to the UI. If you prefer one list, the coordinator may move them into table 2.1-A; input.md would then cite them unchanged.
  (d) The second UI-pump pass of row M4 is `AdvanceRepeatPass` (INP-102); the press withholding of row M6 is `Suppress` (INP-093).
- **C-UI-a / C-UI-b (`ui_framework/`).** Your `layout_rendering_input_and_localisation.md` (UI-203, UI-204) and `pages_and_screen_recipes.md` were written while this package ran. input.md table 3.6-B now uses your nine ids unchanged (`action.menu.up`, `down`, `left`, `right`, `activate`, `front_end_activate`, `close`, `dismiss`, `scroll_speedup`) and adopts your keyboard defaults (arrows with repeat, Enter, Escape outside a match) as Species baseline bindings (INP-077). Requests: (a) cite table 3.6-B for the bindings instead of keeping a second binding table, and keep the context rules (Escape only outside a match, front-end-only activate); (b) UI-216 says the repeat runs in presentation time; input.md uses unscaled real time so that menus repeat while a match is paused (ADR clarification 2); (c) the leaderboard page uses `action.menu.cycle_up` (`PadMenuCycleUp`), `action.menu.cycle_down` (`PadMenuCycleDown`), `action.menu.select_previous`, `action.menu.select_next` and `action.menu.toggle_period`; (d) the dismiss action of the gamepad-disconnected dialog and G06's row M7 (`action.device.pad_connected` closes it) should agree. The UI supplies `FrameGates` (UI window open, text entry open) at the end of each frame (INP-156), calls `Suppress` and `AdvanceRepeatPass`, receives characters with modifier state (INP-112), draws glyph shadows first (INP-080), re-creates the Power slots panel variant on `on_mode_changed` while hidden (INP-152), and picks input-mode caption variants from `Mode(player)`.
- **C-PLT (`platform_layer.md`).** Forward the window's keyboard, pointer, wheel and raw mouse input to the keyboard and mouse back-end; report focus changes and keyboard-layout changes with the layout's BCP 47 tag; raise the close request (also Alt+F4) as `action.app.quit`; capture the pointer on a button press; implement pointer confinement and warping without a movement event; hide the system cursor over the client area; keep pumping the window during loading views (INP-027, INP-108 to INP-111, INP-158, INP-159).
- **C-DAT (`data_schemas/`).** The binding-set schema (INP-070 to INP-073): layer kind, layout tag, entries (action id, bindings, glyph), the JSON node shapes of the INP-071 table, the gamepad profile record (INP-131); the user layer as settings-store entries keyed by action id (key prefix for C-DAT to choose); the keyboard-layout setting; settings that setting nodes name; the feature flag `FeatureFlagX`.
- **C-CAM (`camera_and_sequences.md`).** The camera turns `ConfinePointer` on while a match mode advances and uses `WarpPointer` for the cursor lock.
- **G09b (`options_localisation_and_hooks.md`).** R-META-455 is covered: keyboard layout (INP-164), mouse-button count (INP-184 cites your `ref.meta.settings.mouse_buttons.default`), setting-reading bindings (INP-142). Item 3, the selection lock, is not an input-component rule; please confirm reference/06 (G06) specifies it.
- **G11 (`11_tutorial_and_help.md`).** The camera-help hint's layout variant reads `ActiveLayout()` (INP-166); the camera-moved condition reads the pointer's moved flag (INP-160).
- **C-DBG (`diagnostics_test_and_performance.md`).** Input counters (INP-241), the `DumpBindings` console command (INP-242), event recording and replay in test builds (INP-243).
- **X-AST pass 2.** The missing glyphs and string ids above.
- **Answered for others.** X-DEC-2b: the start-up driver order is INP-100 step 2; the controller-unplugged dialog is opened and closed by INP-148 (the dialog itself is G09's and C-UI's). G02a2 item 10: Strike team stick vectors are stick units (INP-183), converted to world space by R-SIM-440.
- **Coordinator.** Add the `action.menu` group to the `action.<group>.<name>` pattern in CONVENTIONS §8.11; register the `inp.` component key prefix if a key registry is kept.

## Names needed that are not in the ledger (proposals)

- Types: `InputEvent`, `ActionValue`, `FrameGates`, `PointerState`, `InputPointer`, `DeviceBackEnd`, `GamepadBackEnd`, `KeyboardMouseBackEnd`, `InputEventQueue`, `DeviceState`, `BindingNode`, `AllNode`, `ChordNode`, `NotNode`, `AliasNode`, `LatchNode`, `IdleNode`, `SettingNode`, `ValueNode`, `BindingStateTable`, `PadAssignment`, `LoadDiagnostic`, `LoadReport`, `BindingSources`, `BindingDescription`, `DeviceClass`, `DeviceClassMask`, `DeviceId`, `LocalPlayerId`, `ActionId`, `KeyCode`, `MouseControl`, `PadControl`, `ValueKind`, `Condition`, `InputMode`; helpers `ActionAxis`, `StickAxis`, `PriorityAxis`, `ActionTrigger`. Ledger names used: `InputRouter`, `InputSourceNode`, `ActionBindings`, `MatchInputPump`, `UiInputPump`, `TeamCommandState`, `SettingsStore`, `ControlScheme`, `WorldCursor`, `PadMenuCycleUp`, `PadMenuCycleDown`, `AlwaysTrue`, `FeatureFlagX`.
- Action ids `action.menu.left`, `right`, `up`, `down`, `activate`, `front_end_activate`, `close`, `dismiss`, `scroll_speedup` (the nine C-UI also uses, same names), and `cycle_up`, `cycle_down`, `select_previous`, `select_next`, `toggle_period` (leaderboard hook page, new).
- Control names of tables 3.2-A to 3.2-C (Species names by function and position) and the layout tags `de-DE`, `en-US`, `fr-FR`, `en-GB`, `en-CA`.

## Dropped as legacy

- The text binding grammar (driver words, operator tokens, separators, whitespace tokenisation), the parser state machine and its off-by-one error strings — DL-LEGACY-TECH; replaced by the JSON binding-set schema with the same semantics.
- The pipe driver and the input-filter subsystem — compiled but never registered, a stub.
- Window-message details of the reference back-end (message identifiers, the processor chain, the set-cursor fall-through, the no-op processor removal, the unused Alt-Tab hot-key) — DL-LEGACY-TECH; the ADR-0028 back-ends replace them, and the window parts are C-PLT's.
- The runtime-loaded legacy gamepad library, its four-pad limit and controller mask with no caller, its packet-number test and its SDK dead-zone constants — DL-LEGACY-TECH, DEC-338.
- The 256-entry key arrays, the never-set any-key sentinel and last-index bound, and the duplicate unused control-name table — implementation detail.
- The alternative control-scheme binding file and its differences, and the control-method preference — DEC-056, DL-CAMPAIGN.
- The other-platform binding file (Ctrl-click equivalents, system-key bindings that fail) — DL-PLATFORM.
- Gesture controls and bindings (including the misspelt condition and the two-key chord), the icon and gesture Power-bar controls (display, hide, quick-unit, new-unit, weapon-select, scroll, switch-task) — DEC-055, DL-CAMPAIGN.
- Loader mini-program, attract-mode (including its idle binding), advisor-help, skip-tutorial, skip-cutscene, skip-message, world-map drag and deselect, map-editor tile and file multi-select controls — DEC-054, DL-CAMPAIGN, DL-DEAD.
- Debug and cheat keys: drop-packet, camera fix-up, editor toggle, cheat menu, rendering toggle, debug quit, debug-camera rotate and slow-down, and the debug camera's use of the movement helper — DL-DEBUG, DEC-024.
- Controls with no reader or no binding: zoom-change, camera switch-mode, the three rotate controls, secondary aim, start-secondary-fire-directed, the two weapon-cycle controls, left-button-down, mouse movement, the pressed and released variants of the Power-bar display control, select-next-task and the plain display and hide controls — dead.
- Consumer file lists, the "use in full build" classification, generator scripts and verdict sources of the control table — provenance.
- The reference's numeric values for control indices, input-type bits, condition ids, parser states, key codes and language ids — Species enumerations and BCP 47 tags instead.

## Publishing check

Nothing was published or uploaded; no Artifact, Docs, Drive, Gamma or GitHub tool was used. No git write command was run.
