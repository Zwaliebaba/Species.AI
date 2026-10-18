# P6-ENG1 report

## Summary

- **Findings:** 39 in the packet. 32 fixed, 7 fixed differently, 0 rejected, 0 request-only. Several fixed rows also carry a request for the other side, listed in the table.
- **Requests from other packets:** 15 applied (table below).
- **Files edited** (paths under `design_next/architecture/components/`):
  - `audio_engine/README.md`, `audio_engine/instances_voices_and_envelopes.md`, `audio_engine/spatialisation_mixing_and_streaming.md`;
  - `input/README.md`, `input/controls_and_default_bindings.md`, `input/devices_frames_and_queries.md`, `input/binding_layers_and_rebinding.md`;
  - `ui_framework/README.md`, `ui_framework/core_and_widgets.md`, `ui_framework/layout_rendering_input_and_localisation.md`, `ui_framework/pages_and_screen_recipes.md`;
  - `camera_and_sequences.md`;
  - under the coordinator's grant for R2b-166 and R2b-192: `data_schemas/entity_audio_geometry_and_panel_schemas.md`. I changed only the `DspFilterKind` declaration of §2 (it now has nine enumerators, and its comment changed) and table 3.5-A (its rows and caption). It is still the one declaration of the type.
- **The decision behind R2b-166 (the Species DSP filter kinds).** The Species set has nine kinds, which are the kinds that the bus effects of ADR-024 realise:
  - the three that XAudio2's own effects realise: stereo echo (`FXEcho`, or `StereoEchoFx` when the delays differ), room reverb (the XAudio2 reverb) and parametric equaliser (`FXEQ`);
  - studio reverb, which the event set binds (R-AUD-082);
  - the five own-algorithm kinds of R-AUD-079.

  They keep the relative order of R-AUD-077, so the order that R-AUD-080 relies on is unchanged. Five kinds are dropped: chorus, compressor, distortion, flanger and standard gargle. No effect binding names them and no XAudio2 effect realises them, and their parameter sets are those of a retired effect API (ruling 4). Effects are off by default (DEC-567), so nothing a player hears changes. AEN-070 now owns the parameters of the four standard kinds: names by function, ranges, declared defaults and units, as engine values of ADR-024. AEN-071's count is now 14 of 36 parameters.

  I did not cut the set to the four bound kinds. That would also drop the resonant low-pass, bit crusher and simple reverb, which R-AUD-079 specifies, and the room reverb and equaliser that ADR-024 names for the bus chains and that DEC-567 option B needs.

  The R-AUD-077, DEC-339, DEC-567, CONVENTIONS and DAT count halves are requests 1 to 4.
- **Reference-model requirements (R2b-163).** Every "reference model" requirement of the audio engine is gone.
  - Where a Species rule belonged under the same id, I restated it: AEN-077, AEN-110, AEN-121, AEN-179, AEN-246, AEN-251, AEN-260 and AEN-261.
  - Otherwise the id is kept as "(removed: …)" with a pointer to the Species rule: AEN-101, AEN-115, AEN-122, AEN-156, AEN-158, AEN-163, AEN-198, AEN-217, AEN-227, AEN-240, AEN-241, AEN-265, AEN-305 and AEN-362. Nothing but the generated index cites them; request 9 lets the coordinator delete them after the regeneration.
  - Reference clauses inside Species requirements are deleted, or replaced by the observable rule with its DEC: AEN-002, 006, 045, 103, 112, 131, 132, 134, 136 to 138, 164, 167, 175, 178, 180, 182, 185, 196, 202, 207, 210, 211, 215, 221, 222, 224, 225, 228, 230, 234, 235, 304, 321, 322 and 324.
  - AEN-131 differed from R-AUD-029 with no DEC: it merged only into an instance with the same kind of sharing. It now merges into any live shared instance of the same full event id, as R-AUD-029 states, so no DEC is needed.
- **Tuning keys deleted** (nothing else cites them): `aen.reference.voice.position_threshold`, the five `aen.reference.cache.*` keys, `aen.reference.sample_memory.level2_fraction` and `level3_fraction`, `aen.settings.output_default`, `voices_default`, `mix_rate_reader_default`, `output_mix_rate_reader_default`, `output_buffer_first_run`, `output_buffer_reader_default` and `sample_memory_level_default`, and `cam.move.reference_progress_floor` and `cam.move.reference_overshoot`.
- **Actions deleted** (P6-RC request 10 and P6-RD request 13): `action.camera.zoom_out`, `action.power.panel_show`, `action.power.panel_hide`, `action.direct.set_target_end`, and the five leaderboard actions `action.menu.cycle_up`, `cycle_down`, `select_previous`, `select_next` and `toggle_period`. The leaderboard page is a hook that no player reaches (R-META-498), so these actions had no use.
  - Table 3.6-A is renumbered (65 rows); table 3.6-B keeps rows 1 to 9.
  - The reverse index, INP-079, INP-181, INP-182 (now 11, 25 and 36 actions), INP-179 and the layout table 4.13-B follow.
  - Row 22 of table 6.3-A is deleted, and rows 23 to 30 become 22 to 29. Only rows 11 and 16 are cited, and both keep their numbers.
- **Sketches.** Every changed sketch is clang-format 18.1.3 clean: audio, input, camera, UI layout, Recipe A, and the DAT block I changed. The audio sketch is reformatted as a whole (R4a-31).
- **Temporary anchors** keep the generated index's links valid after two heading renames. They are `<a id="82-music-hook-kh-music"></a>` in `audio_engine/README.md` and `<a id="47-the-reference-allocation-and-emitter-selection"></a>` in `audio_engine/instances_voices_and_envelopes.md` (request 9).
- **Helper scripts:**
  - `species_work/scripts/P6-ENG1_replace_block.py` replaces one exact block in one Markdown file;
  - `species_work/scripts/P6-ENG1_drop_rows.py` deletes table rows by action id and renumbers the table;
  - `species_work/scripts/P6-DAT_format.py` (read-only) checked the formatting.

  Each script wrote only the file passed to it. Short inline replacements, each asserting one exact match, were run on my own files only.
- **Gates.**
  - `python3 -I Tools/DesignCheck/CheckDesign.py design_next` with Mermaid, run after the input class diagram changed: gate 4 passes (266 blocks).
  - Final run with `--skip-mermaid`: gates 1, 2 and 5 to 11 pass. Gate 3 has 24 failures, all missing DEC anchors in `decisions/`; they come from P6-DEC's work in progress, not from my files.
  - An earlier run showed RND-018/019/035 citations in `renderer/` and `ADR/ADR-078-…` (P6-RND, in progress) and an uncited Open DEC-267 (P6-DEC). The final run no longer shows either.
  - No failure is in a file I edited.

## Findings

| Id | Status | Note |
|---|---|---|
| R2b-172 | fixed-differently | Coordinator ruling: AEN-251 states the Species decoder behaviour, so it keeps its id with new text. The engine contains no decoder (ADR-025, R14). Resident members are 16-bit PCM and streams are MS-ADPCM blocks that XAudio2 decodes. The cooker (AST-079) rejects other sources. A stream that is corrupt at run time retires its instance (AEN-256) and never stops the program. Nothing of the reference decoder remains. |
| R2b-164 | fixed | The reference-to-Species device table is deleted. AEN-045 now states the one device layer (XAudio2 with X3DAudio, ADR-022, ADR-027) and its operations by sketch name. |
| R2b-167 | fixed-differently | AEN-076 keeps master volume, bus volumes, swap and the effects switch, the four audio settings of DAT table 3.1-A. There is no output-device setting: the engine plays on the default device (ADR-022, AEN-260; DEC-869), so the proposal's "keep the output device" does not apply. Seven `aen.settings.*` keys are deleted. Settings are immediate and never restart a sound (R-META-437). |
| R2b-168 | fixed | AEN-122 is a removed stub; AEN-123 is the update. |
| R2b-174 | fixed | AEN-304 states the three budgets and that the content build measures every cooked bank against them at every build (T25). The run-time memory is shown by the overlay and checked by T27. The sizing figures are the manifest durations; the reference-library statistics are deleted. |
| R2b-163 | fixed | See the summary. The introduction now says that every requirement states the Species engine, and that a deliberate change cites its DEC. |
| R2b-166 | fixed-differently | Cut to nine kinds, not four; see the summary. AEN-070 defines the standard kinds' parameters. AEN-224 states the chain order as the effect bus's XAPO chain (R-AUD-080). AEN-227 is removed and AEN-235 is restated. DAT declaration and table 3.5-A edited under the grant. Requests 1 to 4. |
| R2b-171 | fixed | AEN-240 and AEN-241 are removed stubs; the five `aen.reference.cache.*` keys are deleted. Residency is AEN-243 (now titled "Residency"), ADR-025. AEN-246 states the Species bank facts that its mapping table held. |
| R2b-173 | fixed | AEN-260: on a default-device change or a device arrival (PLT-103) the listener issues `RestartDevice`, and the engine recreates its mastering voice and graph on the new default device by AEN-259 (ADR-022). |
| R4a-24 | fixed (+ request) | Value-node tokens are data names: `value:always_true` and `value:feature:<flag>`, the JSON `{"value": "feature", "flag": "tutorial"}`. The setting notation names a real key, `setting:interface.tips` (DAT table 3.1-A), and the setting node takes a `<group>.<setting>` key. The 8.12-A rows are request 4. |
| R2b-184 | fixed | `value:feature:<flag>` is true while the edition sets an on/off flag of BLD table 3.2-B to `on` (INP-143, BLD-016, ADR-056). A list flag or an unknown flag is the new load diagnostic `UnknownFlag` (table 6.1-A). INP-021 lists the flags as a router input. INP-184 says flags are not settings. |
| R2b-178 | fixed | UI-322: "The pages are exactly the page ids of table 3.1-A; no other page exists." It adds one line that help is the help-and-options windows. The DEC-333 row and the `live_pages` citation are deleted from the pages file; the DEC-244/248 row is reworded. |
| R2b-165 | fixed | AEN-002: "Layer 2 is the one device layer: XAudio2 2.9 with X3DAudio (ADR-022, ADR-027)." |
| R2b-176 | fixed | AEN-103: the clause about the reference loader's builds is deleted; the requirement now reads "The content build validates the sound data". |
| R2b-175 | fixed | AEN-305 is a removed stub pointing to AEN-020, AEN-025 and AEN-301 (ADR-026). |
| R2b-177 | fixed | AEN-322: "The audio markers follow ADR-068", with the `audio.` markers of the DBG catalogue (request 6 on that catalogue). |
| R3b-56 | fixed (+ request) | Declared in §2.1: `SourceId`, `EventId`, `BankId`, `RequestId`, `ResultKind` (with its enumerators), `AudioSettings`, `VoiceFormat`, `SendLevel` and `OperationSetId`. `SoundDataSet`, `BusGraphDef`, `SampleCache` and `SourceVoiceSlot` are forward-declared with a comment naming their section. `AudioResult` carries the emitter of a tracked-emitter result (AEN-041) instead of an unused `SoundVoiceId`. Registry: request 4. |
| R4a-31 | fixed | The audio sketch is clang-format 18.1.3 clean: one enumerator per line, data tokens as trailing comments. |
| R4a-56 | fixed | `SoundPosition::Screen2D`, matching `Grid2D`; no other use existed. |
| R4a-43 | fixed-differently | `Seconds` becomes `AudioSeconds`. `Op` becomes the declared `OperationSetId`, the operation set passed to `IXAudio2::CommitChanges`. `Decibels` was declared and never used, so it is deleted rather than registered. Registry: request 4. |
| R4a-46 | fixed | Same declarations as R3b-56. The `BusGraphDef` comment and the AEN-060 `AudioBusGraph` row state that `VoiceBackend::Build` creates the `AudioBusGraph` from the cooked `BusGraphDef`. |
| R2b-169 | fixed | AEN-182: the reference start sentence is deleted. It now says that a layer whose group or member is missing never takes a voice (R-AUD-046). |
| R2b-170 | fixed | Parenthesis deleted from AEN-196. |
| R2b-189 | fixed | CAM-006 (now MUST): the modes, operations and services are exactly those of §2 and §4 (DEC-024). It names the four Dropped DECs of §8.1, and cameras of a world game are a design-phase input. |
| R2b-188 | fixed | Both `cam.move.reference_*` keys are deleted. CAM-050: "A scripted move follows CAM-046 to CAM-049 and no other move law (DEC-024)." |
| R3b-52 | fixed | T12 states the vector set: generated once by the test from the CAM-057 steps in double precision and committed beside the test, from a fixed start pose, structure and terrain. Tolerances are 0.001 wu for the position and 0.0001 per front component. |
| R3b-57 | fixed | Declared `Engine::StatementArgs` (the argument fields of table 3.3-A) and `Species::TimelineFrameInputs` (the camera clock, the snapshot, the input router, the fade completion and the audio command writer). Both are already registered under CAM. |
| R2b-187 | fixed | INP-009: the clause is deleted. |
| R2b-185 | fixed-differently | The list of reference defects is deleted. INP-221 says "the default set is exactly the entries of tables 3.6-A and 3.6-B", not 3.11-A as proposed, because 3.11-A is the development layer. It keeps the content-build failures as testable rules. |
| R3b-55 | fixed-differently | No new types. `Signal<>` becomes `AddModeListener` and `AddLayoutListener`, `std::function` listeners like `AppWindow::AddListener`. `LogSink` is dropped: `DumpBindings()` writes to `LogCategory::Input` (DBG §2.1, ADR-067). `PlatformServices` becomes `AppWindow&`, whose input sink the keyboard and mouse back-end registers (INP-108). `EventSink` becomes the registered `InputEventQueue`, now forward-declared. INP-150, INP-166, UI core list item 5 and diagram 3.10-A follow. |
| R4a-48 | fixed-differently | As R3b-55; nothing is left to register. |
| R2b-186 | fixed | INP-095: the reference key list is deleted; table 3.11-A is the one list of development bindings. |
| R2b-181 | fixed | Core §8 input 2 keeps the question (DEC-045) and the reusable recipe facts. The reference page details and the RGBA figures are deleted. |
| R2b-182 | fixed | Input 6: "Whether Species adds Chinese is a design-phase language choice; no Chinese string table exists today (UI-234)." |
| R2b-183 | fixed | Input 7: "coverage is GameAssets §17 (U+0410 to U+044F in all three faces)". The art-direction question stays. |
| R2b-179 | fixed | UI-341 is now positive: the pages hold only the widgets of UI-340 (the front-end widget family and table 3.3-A), and no other page widget exists. The map-editor sentence stays. |
| R2b-180 | fixed | The DEC-333 sentence is deleted from UI-411. |
| R4a-55 | fixed | Recipe A is wrapped in `namespace Engine`, with `EXAMPLE_WIDTH_PIXELS`, `Apply` and `RefreshPreview` declared. The prose says that a tool window naming a game concept, such as a map-editor window, is the same code in `Species` with the `Engine::` names qualified. |
| R3b-59 | fixed | DT-27 builds each widget kind in storage that the test first fills with a non-zero byte pattern, and checks that every field equals its declared default. No sanitiser is needed. |

## Requests from other packets applied

| From | Item | What I did |
|---|---|---|
| P6-RD 9 | AEN-070, AEN-071 | AEN-070 cites DAT table 3.5-A for the order and owns the ranges and defaults. It keeps P6-RD's values for the four standard kinds it retains, with units added; the values of the five kinds dropped by R2b-166 are not carried. AEN-071 now says "its declared default (AEN-070 for the standard kinds, R-AUD-077 for the own-algorithm kinds)" and "the 15 effect bindings". |
| P6-RD 10 | AEN-235, AEN-260, AEN-261 | AEN-235 cites AEN-070. AEN-260 is restated as the default-device change (R2b-173). AEN-261: the apply sends commands, never `RestartDevice`, and no sound restarts or is cut. |
| P6-RD 11 | AEN-076, AEN-077, §8.2, AEN-361 | Rows deleted; AEN-077 restated as the Species all-members rule. §8.2 is titled "Music hook", with a temporary anchor for the index. AEN-361 cites R-AUD-414. |
| P6-RD 12 | AEN-156, AEN-265, AEN-130 | AEN-156 and AEN-265 are removed stubs that point to R-AUD-217, R-AUD-218 and DEC-570. AEN-130 has the no-object sentence (R-AUD-021, R-AUD-209). |
| P6-RD 13 | `ui_framework/`, input rows 10 to 14 | The mode-option row cites R-MODE-073. UI-090's example is `ui.dialog.settings_save_failed` (R-META-407), and the cited-string list is updated. The `Achievements`, `LeaderboardSelect` and `Leaderboard` rows are hooks of R-META-491 or R-META-498. The UI-363 leave-hook row, the UI-392 loading caption, the slide-in row and the `row_slide_speed` citation are deleted. The five leaderboard actions are deleted from input (no use remains). |
| P6-RB 14 | `normalise(` → `Normalize(` | 12 places in `camera_and_sequences.md`, and AEN-209 for consistency. |
| P6-DAT 9 | DSP filter kinds | DAT enum and table 3.5-A cut under the grant. The counts in DAT-271, T55 and DAT README table 1-A are request 1. |
| P6-DAT 10 | camera §4.8 | `step.durationSeconds` → `step.duration` (two places). |
| P6-DAT 11 | UI-441 | "immediate or a renderer reconfiguration". |
| P6-DAT 12 | AEN-076 | The effects-switch row matches DAT table 3.1-A ("immediate", AEN-221); `aen.settings.output_default` is deleted. |
| P6-RE 4 | CAM-032, CAM-103, AEN-115, AEN-006, AEN-121 | Steps 11.8 and "between 11.7 and 11.8" (checked against R-SIM-581). AEN-115 is a removed stub, so the retired key is gone. AEN-006 and AEN-121 no longer narrate the reference accumulator or cite R-SIM-571. |
| P6-RC 10 | input actions | The four actions are deleted from tables 3.6-A and 4.13-B, the reverse index, INP-179 and T28. Row 22 of table 6.3-A is deleted (rows renumbered). The counts in INP-181 and INP-182 are corrected. |
| P6-RC 11 | camera, UI-462 | CAM-049 and the cited-keys paragraph use `ref.ctrl.camera.zoom_rates.scripted`. §8.2 item 2 is replaced as asked. CAM-071 drops the map-editor callers. UI-462 drops the discard-button clause. |
| Coordinator (R4a-22) | link names | AEN-041 and AEN-189 use `groups_nearby` and `objects_nearby`. |
| Ruling 2 | glyph pages | UI-003 and UI-198: the glyph cache creates its atlas pages only through `Engine::GpuResourceFactory` (renderer README §2, ADR-078). It writes them through the upload ring by the UI pass that imports them by name, and refills them after `DeviceRebuilt`. The data-model row and the failure row follow, and `GlyphRenderer` takes the factory (forward-declared). |

## Requests

1. **Coordinator (P6-DAT has finished), DSP-kind counts in the data schemas:**
   - `data_schemas/entity_audio_geometry_and_panel_schemas.md` DAT-271: "21 of the 63 parameters" → "14 of the 36 parameters".
   - The same file, T55: "14 kinds and 63 parameters" → "9 kinds and 36 parameters".
   - `data_schemas/README.md` table 1-A, row `sound.effect_kinds`: "the parameter descriptors of the 14 effect kinds" → "the parameter descriptors of the nine DSP filter kinds (AEN-070)".

   `std::array<SoundParam, 13>` in `EffectBinding` stays: room reverb still has 13 parameters, and `aen.dsp.max_params` is still 13.
2. **Coordinator (P6-RD has finished), `reference/10_audio_design/event_model_and_sound_data.md` R-AUD-077, the paragraph after the table.** Replace "The parameter sets of the standard kinds 0 to 8, their ranges and declared defaults, and their processing are the [audio engine](…)'s (AEN-070, ADR-024); the effect bindings of R-AUD-082 name every parameter they set. The parameter names and their order for every kind are the sound-event schema's (… table 3.5-A)." with: "Species realises nine of these kinds: the stereo echo, room reverb, parametric equaliser and studio reverb, and the own-algorithm kinds 9 to 13. Their parameter sets, the ranges and declared defaults of the standard kinds, and their processing are the [audio engine](…)'s (AEN-070, ADR-024). The chorus, compressor, distortion, flanger and standard gargle are named by no effect binding of R-AUD-082 and are not kinds of the Species sound data (DEC-339). The effect bindings of R-AUD-082 name every parameter they set. The parameter names and their order for every Species kind are the sound-event schema's (… table 3.5-A)." Keep the last sentence about kinds 9 to 13.
3. **P6-DEC:**
   - **DEC-339:** append to the Specified behaviour: "The Species DSP filter kinds are the nine of AEN-070. The chorus, compressor, distortion, flanger and standard gargle kinds of the reference effect set are named by no effect binding and realised by no XAudio2 effect, so they are not Species kinds, and a binding that names one is a content-build error." Add `architecture/components/audio_engine/README.md (AEN-070)` and `data_schemas/entity_audio_geometry_and_panel_schemas.md (table 3.5-A)` to Affects. The status stays Dropped.
   - **DEC-567:** "lies outside the declared range of 21 of the 63 parameters" → "lies outside the declared range of 14 of the 36 parameters".
4. **P6-GLO, `CONVENTIONS.md`:**
   - **8.12-B, AEN "types" row:** add `AudioSeconds` (alias of audio time in s), `SourceId`, `EventId`, `RequestId`, `OperationSetId`, `AudioSettings`, `SoundDataSet`, `BusGraphDef`, `VoiceFormat` and `SendLevel`.
   - **8.12-B, AEN "enumerations" row:** add `ResultKind` (`Count`, `Playing`, `DeviceReady`, `InterfaceReady`, `MatchReady`, `EmitterTracked`, `EmitterUntracked`), and `SoundPosition` with its enumerators (`Screen2D`, `WorldFixed`, `Attached`). Add the constant `AUDIO_BUS_COUNT` where the AEN constants are registered.
   - **8.12-B, AEN "Species XAPOs" row:** remove `ChorusFx`, `CompressorFx`, `DistortionFx`, `FlangerFx` and `StandardGargleFx`.
   - **The DAT "sound records (generated)" group of P6-DAT's request 1:** `DspFilterKind` has these enumerators only: `StereoEcho`, `RoomReverb`, `ParametricEqualizer`, `StudioReverb`, `ResonantLowPass`, `BitCrusher`, `Gargle`, `Echo`, `SimpleReverb`.
   - **8.12-A:** delete the rows "always-on feature flag" `AlwaysTrue` and "feature flag" `FeatureFlagX`; no such type exists. Delete the rows "pad menu cycle down" `PadMenuCycleDown` and "pad menu cycle up" `PadMenuCycleUp`; the actions are deleted.
   - **8.12-B, INP "data names" row:** remove the action ids `action.menu.cycle_up`, `action.menu.cycle_down`, `action.menu.select_previous`, `action.menu.select_next` and `action.menu.toggle_period`, leaving the nine of the UI framework. Add the value-node tokens `always_true` and `feature`. Add the load diagnostic `UnknownFlag` if the INP diagnostic classes are registered.
5. **P6-ADR, `ADR/ADR-024-audio-buses-and-per-bus-effects.md`, More Information (optional).** Add: "Amendment 2026-10-18. The DSP filter kinds of the sound data are the nine that the bus chains realise: the stereo echo, room reverb and parametric equaliser on XAudio2's own effects, and the studio reverb and the five own-algorithm kinds as Species XAPOs (AEN-070). The other standard kinds of the reference effect set are not Species kinds (DEC-339)."
6. **P6-ENG2, `diagnostics_test_and_performance/logging_checks_and_profiling.md`, marker-catalogue row "audio" (line 41).**
   - The engine contains no decoder (AEN-251), so `audio.decode` → `audio.stream`, the stream task (AEN-302, AEN-322).
   - Replace "the reference's instance-start sections map onto `audio.instance.start`, …, and its effect sections onto `audio.effect.resonant_low_pass`, …" with "the instance start has the markers `audio.instance.start`, `audio.instance.open_stream`, `audio.instance.set_parameters`, `audio.instance.set_frequency_volume_position` and `audio.instance.setup_effects`, and the own-algorithm effects `audio.effect.resonant_low_pass`, `audio.effect.gargle` and `audio.effect.echo`" (ruling 3).
7. **P6-RND, `renderer/README.md` §2 (information).** The UI glyph cache now depends on two things the factory must provide: a `DeviceRebuilt` notice (UI-198 refills on it) and the import of a factory texture by name into the UI pass (ADR-078, "Same-frame writes"). `ui_framework/layout_rendering_input_and_localisation.md` §2 forward-declares `class GpuResourceFactory;` and cites renderer README §2. If the factory's name or namespace differs from `Engine::GpuResourceFactory`, tell me (round 2).
8. **Coordinator: regenerate the requirements index.** First sentences changed for many AEN requirements (among them AEN-006, 045, 070, 076, 077, 110, 121, 179, 246, 251, 260, 261, 304) and for INP-079, 143, 182, 221, CAM-006, CAM-050, UI-322 and UI-341.
9. **Coordinator: after regenerating the index:**
   - Delete the removed AEN stubs that only the index cites: AEN-101, 115, 122, 156, 158, 163, 198, 217, 227, 240, 241, 265, 305 and 362. AEN-265 and AEN-362 also appear only in the end points of the folder index's ranges ("AEN-264, AEN-265", "AEN-360 to AEN-362"); change those cells to "AEN-264" and "AEN-360, AEN-361" when the stubs go.
   - Delete the temporary anchors `<a id="82-music-hook-kh-music"></a>` in `audio_engine/README.md` and `<a id="47-the-reference-allocation-and-emitter-selection"></a>` in `audio_engine/instances_voices_and_envelopes.md`; they only keep the old index links valid.

## Noted, out of scope

These are in my files but outside my findings, and I left them:

- `input/binding_layers_and_rebinding.md` INP-176 ends with "The reference rebinding page captures keyboard keys only; capturing mouse buttons and gamepad buttons is available to it." This is reference narration (ruling 3).
- `ui_framework/core_and_widgets.md` §8 design-phase input 3 begins with "The reference binds the menu actions to the gamepad only".
- `input/README.md` §8.3 item 7 names `glyph.key.tab`, whose only binding left in the default set is `action.power.next`. It is still correct.
