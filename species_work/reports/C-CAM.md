# C-CAM report

Package: `species_work/packages/C-CAM.json` (component template §6.4, prefix CAM, range 001–199).

## Files written

| File | Size |
|---|---|
| `design_next/architecture/components/camera_and_sequences.md` | 77,192 bytes |
| `species_work/scripts/C-CAM_check.py` (self-check, adapted from the C-TER script) | 6,523 bytes |

Checks run: the hard-fail scan prints nothing; the soft scan prints nothing; `C-CAM_check.py` finds no problem except one false positive (`AC-CTRL-CA-08` exists in camera.md, in the form `**AC-CTRL-CA-08 — Local only**`); all 4 Mermaid blocks parse (G06's validator, mermaid in the scratchpad). Requirement ids are unique, ascending and within 001–199; tuning keys are unique and each one is used; every cited DEC, ADR, R-*, RND-* and VFX-* id exists; all template headings and "Design-phase inputs" are present; there are no truncation words; links resolve, or point to planned targets (`data_schemas/README.md`).

## Counts

- Requirements: 84 (CAM-001 to CAM-126, with gaps between sections).
- Tuning keys: 23 `cam.*` keys (move 3, focus 11, developer 5, pick 1, timeline 1, budget 2). Keys of other documents are cited, not redefined: `ref.ctrl.camera.*`, `ref.sim.frame.delta_clamp`, `ref.sim.match_entry.*` (3), `ref.map.camera.*` (2), `rnd.view.*` (4), `rnd.fade.rate`.
- Diagrams: 4 Mermaid (the map camera records as a class diagram, the camera modes, the start-of-match sequence, the animation playback state machine) and 1 pseudocode listing (playback).
- Enumerations: 3 C++ enumerations (`CameraMode` 8, `CameraTransition` 2, `TimelineOp` 16) and 8 numbered tables (neighbours, record fields, camera state, timeline statements, modes, failure modes, acceptance tests (23), decisions), plus enumerated lists (ownership, dropped capabilities, editor hooks, design-phase inputs).

## Open DECs cited

All entries of `decs_to_cite` are cited. Open entries are cited by requirements: DEC-051 (CAM-100 to CAM-115), DEC-222 (CAM-004), DEC-247 (CAM-095), DEC-310 (CAM-038), DEC-803 (CAM-065 to CAM-071). Resolved entries are stated as fixed rules: DEC-008 (CAM-003, CAM-015), DEC-024 (CAM-002, CAM-030, CAM-046 to CAM-050), DEC-096 (CAM-002, CAM-060), DEC-560 (CAM-014, CAM-021, CAM-061). Also cited: DEC-007, DEC-009, DEC-015, DEC-045, DEC-046, DEC-052, DEC-053, DEC-054, DEC-059, DEC-103, DEC-240, DEC-800, DEC-801.

## New DEC requests

1. **Timeline-only camera operations (DEC-803 or DEC-051 amendment).** The package keeps the structure-focus orbit and the approach move (ORIG 30 §4.4). Both are reachable only from the dropped script interpreter, and DEC-024 and R-CTRL-457 say that the unreachable scripted modes (building focus among them) are not specified. I kept them as **timeline-only** operations (CAM-053 to CAM-058, table 4.2-A, "reachable only through a timeline"), so the reference motion law is not lost, and R-CTRL-457's list of live modes still holds. Proposal: extend DEC-803 (Affects: CAM-053 to CAM-058) so that option A keeps them as engine capabilities without content and option B removes them with the timeline. If the coordinator reads DEC-024 as excluding them, CAM-053 to CAM-058, table 3.3-A rows 7 and 8, the `cam.focus.*` keys and T12/T13 can be removed together; nothing else depends on them.
2. **Pick-ray pixel convention (Open; default = reference).** The reference ray passes through the lower-left corner of the pointer's pixel, a one-pixel vertical offset from the pixel's top edge; it is a by-product of the window-coordinate flip of the superseded API. CAM-090 keeps it (`cam.pick.row_offset` = 1 px). Options: A, keep it; B, use the pixel centre (offset 0.5 in both axes).
3. **Camera shake as a view offset (record the change).** R-CTRL-463 turns the stored front and up in every frame, so in the static and scripted modes the shake leaves a random-walk residue in the pose. ADR-0036 (Accepted) and VFX-232 apply it as a non-stored view offset, and CAM-012 follows them. This is a behavioural change: please record it in DEC-024, or in a new Resolved entry, so that R1 sees why the residue is gone.
4. **Camera clock while the world is held (record the change).** By R-SIM-571 the reference reuses the last frame delta during a pause. CAM-030 uses the real, clamped frame step, which is the same while frames are regular. The DEC-024 text ("one presentation clock") could name this.

The engine-baseline choices that fall under DEC-051 option A need no new entry, but here they are for visibility: the end of a timeline releases a camera it claimed (CAM-107); starting a timeline aborts the running one and resets its state (CAM-108); an abort also stops a camera animation (CAM-106); and a move node whose move is still under way lands when the node ends (CAM-067), which DEC-803's "moves land exactly on their view" requires.

## ADR conflicts

- **ADR-0016, More Information:** "the camera component owns the field-of-view, near and far values". In fact the renderer owns the planes (RND-211, `rnd.view.near_plane` and `rnd.view.far_plane`) and the clamp (RND-212), and camera.md agrees with the renderer. As the brief instructs, I follow the written renderer document (CAM-005, CAM-012). Request to X-ADR: reword ADR-0016 to say that the camera owns the field-of-view values and the renderer owns the planes and the clamp.
- There is no conflict with ADR-0036. Its presentation clock (scaled, stopped while paused) serves VFX. The camera clock is a separate, unscaled clock that keeps running while the world is held, as R-SIM-571 and R-SIM-573 already distinguish (CAM-030).

## Missing assets

None. `assets_for_you` is empty, and the component cites no asset.

## Cross-document requests

- **G06 (camera.md).**
  - (a) R-CTRL-457 and table 2.2-A: consider noting that the camera component adds a timeline-only structure focus (CAM-056) and a developer fly mode for development builds and the editor (CAM-080).
  - (b) R-CTRL-463: in Species the shake is a non-stored view offset (ADR-0036, VFX-232, CAM-012).
  - (c) R-CTRL-460: ORIG's field-of-view easing is also skipped "while the task manager is visible". The ledger maps that term to the Power slots panel. Please check whether R-CTRL-460 should say that the zoom component is skipped while the Power slots panel is visible. The structure focus uses rate 1, like the scripted move (CAM-058).
  - (d) R-CTRL-453 says that the camera is not advanced in the front end, but R-META-210 says that the backdrop "advances the camera". CAM-032 reads this as "the backdrop sets the view directly"; please align with G09.
- **G08a (rules_and_conventions.md).**
  - (a) ORIG's data model gives camera-view and animation names at most 63 characters in a 64-byte buffer, while R-MAP-125 gives 62 (`ref.map.camera.view_name_max_chars`); please confirm the value.
  - (b) Add a name-length limit for camera animations (CAM-020 uses the view-name limit).
  - (c) Add the content-build rule that a camera view with a zero-length front or up vector is an error (CAM-020).
- **G08b (map_editor_hooks.md).** R-MAP-309 says that the editor's free developer camera "is not specified (DEC-024)". It is now CAM-080 to CAM-083; please cite them. The preview and stop operations of R-MAP-371 are CAM-065 and CAM-068.
- **C-SIM-c (01_tick_timing_and_main_loop.md).** R-SIM-581 step 2 says that the cut to the start view "happens once", while R-CTRL-497 and ORIG repeat it after every reset that rebuilds the match world (CAM-061 follows R-CTRL-497); please align. If timelines are adopted, they advance between steps 11.8 and 11.9 (CAM-103).
- **C-SIM-a (02_match_lifecycle_and_state_reset.md).** In R-SIM-014 step 9, the editor enters the developer fly mode (CAM-060 step 6). The "automatic mode" of step 9 is the unspecified debug-mode setting.
- **C-INP (input.md).**
  - (a) Add the developer action ids `action.debug.camera_fast`, `action.debug.camera_slow` and `action.debug.camera_rotate`, for the `Debug` and `Development` flavours and the editor. In the reference, slow-down and rotate are both on the secondary pointer button; the fast key is not given in my ranges.
  - (b) Add `action.sequence.skip`, only if DEC-051 adopts timelines. The reference skip is Enter and the gamepad's east face button.
  - (c) The interface sketch cites INP-164 (`WarpPointer`) and INP-165 (`ConfinePointer`), which are not yet defined in input.md; I cite the functions by name only.
- **C-DAT (data_schemas).** The camera-view, camera-animation and node records (table 3.1-A, including the order Move, Cut) and a new presentation-timeline record (table 3.3-A, 16 statement kinds). `MapCameraData` is built from these records and from the start-position records (DEC-560).
- **C-RND-a.** Request 11 lists "70° scripted views" for RND-212. That value belongs to the dropped intro and outro flights; the camera's field-of-view values are 60° (free camera), 15° (zoom), 50° (turret aim), 60° (structure focus, `cam.focus.fov`) and 60° (editor, `cam.developer.editor_fov`). The pick ray (CAM-090) and `ViewProjectionFor` (CAM-092) follow RND-215, RND-216 and RND-218.
- **C-VFX.** VFX-003 and VFX-232 match CAM-012. If timelines are adopted, the catalogue of overlay effects (CAM-112) is yours, with the UI framework.
- **C-UI.** If timelines are adopted: a caption with a typewriter reveal, a text cursor, a layout and a display time (CAM-110).
- **G09 (front end).** R-META-007 names a front-end focus mode that the camera does not have (CAM-006). R-META-210 "advances the camera"; see the G06 item (d) above.
- **C-DBG.** The developer fly mode (CAM-080 to CAM-084) and the camera overlay (CAM-125) are specified here; list them among the developer tools, citing CAM.
- **G11.** Agreed: no tutorial plays a camera animation (CAM-071, CAM-100); the timeline baseline is CAM-100 to CAM-115.
- **X-GLO.** Terms to add: camera clock, scripted move, structure focus, approach (camera), developer fly mode, presentation timeline, camera trigger, level up vector. The entry "previous camera view" matches CAM-021 and CAM-065.

## Names not in the ledger (proposals)

- Engine types: `CameraMode` (enumerators `Static`, `ScriptedMove`, `FreeMovement`, `Tracking`, `BeamDishAim`, `TurretAim`, `StructureFocus`, `DeveloperFly`), `CameraTransition` (`Move`, `Cut`), `CameraPose`, `CameraView`, `CameraAnimationStep`, `CameraAnimationDef`, `MapCameraData`, `CameraFrameInputs`, `CameraFrameOutput`, `CameraState`, `PickRay`, `TimelineOp` (16 enumerators, table 3.3-A), `TimelineStatement`, `PresentationTimeline`, `TimelinePlayer`, `StatementArgs`, `SourceLocation`, `TimelineFrameInputs`. None repeats an identifier of the reference implementation. The animation types use `Def` and `Step` suffixes because the plain names are reference identifiers.
- `MatchCamera` methods: `SetTargetView`, `SetTargetPose`, `SetTargetAround`, `Cut`, `StartMove`, `FocusStructure`, `EnterMode`, `ReleaseToPlayer`, `SetFieldOfView`, `SetTargetFieldOfView`, `PlayCameraAnimation`, `StopCameraAnimation`, `IsBusy`, `PickRayAt`, `ViewProjectionFor`, `EnterMatch`, `OnLocalTeamKnown`, `LeaveMatch`.
- Action ids: `action.debug.camera_fast`, `action.debug.camera_slow`, `action.debug.camera_rotate`, `action.sequence.skip`.
- Terms: camera clock, structure focus, approach, developer fly mode, presentation timeline, camera trigger, level up vector.

## Dropped as legacy (one line per group)

- Spherical-world and world-map camera modes (orbit, scripted orbit, the focus used by the main menu, intro and outro flights, the outro's credits hand-over), and the 70° and 100° fields of view: campaign or cut content (DEC-024, DL-CAMPAIGN, DL-CUT, DEC-054).
- First-person and entity-follow modes: unreachable (DEC-024).
- Debug-advancer grace frames after a mode request, and the intro and outro debug fix-up key: DEC-024, DL-DEBUG.
- Developer speed derived from the global world size: campaign world map (DL-CAMPAIGN).
- Record and restore of a camera pose outside an animation, with its 3 s restore move: used only by a campaign tutorial (DL-CAMPAIGN). `previous_view` is kept through animation playback.
- Script-command mapping, the zero-duration-move-then-cut idiom, and the escape path that leaves an animation running (outside my ranges as well): DL-CUT, DEC-053.
- The "best building in view" query (speed gate, 1 s recompute, 200-unit reach): its only reader is the narrator's building help (DEC-052).
- The water-reflection pose negation: DEC-059.
- The unproject and project mechanism of the superseded graphics utility library, the identity reset of a degenerate matrix, and the window-coordinate flip, which is kept only as its observable pixel convention (CAM-090): DL-LEGACY-TECH.
- The text level-file block grammar, its sentinels and the writer's print formats; the map writer that omits animations; the mount ordering across mission files: DL-LEGACY-TECH (R10), DL-CAMPAIGN; replaced by typed records and lossless writers (DEC-800, CAM-022).
- Fatal assertions on bad records, and the assertion that compares against the wrong enumeration size: replaced by content-build validation (DEC-015).
- Editor window details (placeholder names, the previous-view literal-free defect): DL-EDITOR-UI. R-MAP-371 already records them; §8.2 keeps only the hooks.
- Mount census figures: R-MAP-128 owns them.
- §8 items 2 and 5 (TO-DEC, DEC-103), 6 (narrator) and 9 (drop list) (DL-CUT), and 8 (replays, KH-REPLAY with C-DBG): not in this package's scope.
