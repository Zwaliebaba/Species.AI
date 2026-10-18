# P6-RD report (Phase 6, wave 1)

## Summary

- Packet findings: 52. **Fixed 45, fixed-differently 7, rejected 0, request-only 0.** Plus one relayed request from P6-RA, applied.
- Files edited (all owned): `design_next/reference/09_front_end_and_match_flow/options_localisation_and_hooks.md`, `screen_flow_and_match_setup.md`, `front_end_presentation.md`; `design_next/reference/10_audio_design/README.md`, `event_model_and_sound_data.md`, `event_catalogue.md`, `positional_priorities_and_mix.md`, `front_end_hud_voice_and_ambience.md`, `music_hooks_and_sound_design_brief.md`.
- Gates: `python3 -I Tools/DesignCheck/CheckDesign.py design_next --skip-mermaid`: all PASS. Mermaid diagrams were changed (options 3-C, screen flow 3-H and 3-I), so I also ran it without `--skip-mermaid`: all 11 gates PASS (266 blocks, 0 failed).
- **Removed requirement ids are kept as stubs.** Every requirement id I removed is still written as `**ID** (removed: <reason>)`. The generated `requirements_index/` cites every id, so deleting one fails gate 5 until the index is regenerated, and several of the ids are also cited by files I do not own. The stubs are R-META-070, 416, 418, 419, 420, 428, 430, 431, 432, 438, 492, 499, 500, 501, 502, 503 and 504, R-AUD-219 and R-AUD-408. The coordinator may delete the stubs that only the index cites (R-META-070, 428, 438, 503, 504; R-AUD-408) once it regenerates the index.
- **New ids:** R-AUD-414 and R-AUD-415 (music path and attack timing under option A of DEC-049). **New tuning keys:** `ref.meta.connect.host_silence_limit`; `ref.meta.explainer.<mode>.canvas_size` and `.loop_span` for the six modes; `ref.meta.explainer.key_default.{time,picture,position,colour_role,scale,angle,flip,alpha}`; `ref.aud.variety.{files,variants_per_event,bark_variants,sting_variants}`.
- **Deleted tuning keys:** `ref.meta.options.screen.*` (all 7), `ref.meta.options.sound.{mix_rate_choices,mix_rate_default,voice_choices,voices_page_default}`, `ref.meta.achievement.row_*` (4), `ref.meta.leaderboard.*` (4), `ref.meta.setup.mode_option_slots`, `ref.aud.reference.*` (3), `ref.meta.explainer.<mode>` (6), `ref.meta.explainer.key_defaults`.
- **Deleted string ids:** `ui.options.screen.{refresh,refresh_value,colour_bits,depth_bits,bits_value,fullscreen_only}`, `ui.options.sound.{output,output_platform,mix_rate,rate_value,voices,voices_value}`, `ui.dialog.screen_mode_failed`. The achievement and leaderboard captions are kept as starting text for DEC-047.
- **Temporary anchor:** `music_hooks_and_sound_design_brief.md` §2.1 is renamed "Music hooks" (the KH-MUSIC tag is gone). It carries `<a id="21-music-hooks-kh-music"></a>` so that the generated index's links still resolve. The coordinator removes that line after regenerating the index (Requests, item 22).

## Findings

| Id | Status | Note |
|---|---|---|
| R2a-5 | fixed | R-META-475 now reads: "The companion faces are selected for Japanese and Korean only; no Chinese string table exists, so no Chinese variant is offered (DEC-099)." |
| R1b-2 | fixed | Checked at ORIG 27_audio_engine.md:480 (the world origin and no emitter when no object is given). The R-AUD-021 cell now gives the origin, no emitter, and the 2d, fix and att outcomes (R-AUD-209). Added AC-EVM-13. AEN-130 is a request to P6-ENG1. The two-dimensional form is not wanted, so no DEC is needed. |
| R1a-4 | fixed | Checked at ORIG 10_audio_design.md:1612. The R-AUD-411 Supply-crates row now asks for the 100 % tone and for 31 outcome stings, including the six without a sting of their own. `supply_crate.capture_progress_100` is now "silent (DEC-565)". R-AUD-139 says DEC-565 decides whether the six outcomes raise `crate_outcome.rocket_turret`, `.flame_turret`, `.firebomb_run`, `.builder`, `.time_dilation` or `.time_rush`. DEC-565 and the six manifest groups are requests. I list the 100 % tone separately rather than "0 to 100 % rising and falling", because the 100 % event is raised only when the crate opens. |
| R2a-72 | fixed-differently | The screen options page is now the Species set: screen size and play in a window (RND-442, ADR-075), with one informative sentence after R-META-414. R-META-416, 418, 419 and 420 are kept as removed stubs, because PLT-029, PLT-073, RND-452 and DAT table 3.1-A cite them. R-META-421 to 424 are restated as Species rules: renderer reconfiguration (RND-447) and the PLT-112 fallback chain with `ui.error.resolution_rejected` and `ui.error.display_mode`. All `plt.display.reference.*` citations and all `ref.meta.options.screen.*` keys are gone, including `windowed_default`, which is redundant under DEC-100's one default. **Diagram 3-C is replaced by the Species apply flow instead of deleted**, which keeps the numbering of diagrams 3-D to 3-F. The settings register drops the refresh, colour, depth, vsync and overscan rows. `ui.dialog.screen_mode_failed` is removed in favour of the platform notice. Requests go to P6-ENG2, P6-DAT, P6-RND and P6-ENG1. |
| R2a-73 | fixed | The sound page keeps master volume, bus volumes and swap, with the informative sentence suggested (hardware positioning left out of it, because no page ever showed it). R-META-430 to 432 and 438 are stubs. R-META-436, 437 and 439 are restated: the apply never restarts or cuts a sound (AEN-259, AEN-261). R-META-454 keeps only the effects switch. The 4 `ref.meta.options.sound.*` keys and the AEN reader-default citations are deleted. The register and §5 are updated, and R-AUD-226 is aligned. Requests: DEC-869 becomes Dropped (ruling 4), plus AEN-076 and DAT. |
| R2a-75 | fixed | Sentence deleted. |
| R2a-78 | fixed-differently | R-META-491 and R-META-498 are restated as the proposed hooks: no achievements page, no leaderboard page, and the board types and per-mode boards as inputs to DEC-047. R-META-492 and 499 to 504 are stubs, because UI page-catalogue rows and INP action rows cite them. The 8 `ref.meta.achievement.row_*` and `ref.meta.leaderboard.*` keys are deleted. **The achievement and leaderboard strings are kept** as starting text for DEC-047, which avoids orphaning the CONVENTIONS row for Skill rating and the GameAssets counts. AC-24 is replaced. |
| R3a-16 | fixed | New key `ref.meta.connect.host_silence_limit` (5.0 s of real time; named this way instead of `…_s`). There is a new R-META-059 row (`ui.error.connection`, back to the title page) and new exits in R-META-056 (after the host's first message only; still no timeout while connecting, as in the reference) and R-META-092. Diagrams 3-H and 3-I, §5 item 8 and AC-META-23 are updated. The DEC-205 text is a request. |
| R2a-88 | fixed | Clause removed. The last sentence is now "These five groups are new groups of the asset manifest (R-AUD-003, R-AUD-413)", and "bound in Species by" is now "bound by". |
| R2a-89 | fixed | R-AUD-143 uses the proposed text. |
| R2a-80 | fixed | R-AUD-077 now gives the 14 kinds in order, by function. The I3DL2 name and the parameter ranges and defaults of standard kinds 0 to 8 are gone, and those kinds point to AEN-070 under ADR-024. Parameter names and order point to DAT table 3.5-A. The own-algorithm kinds 9 to 13 keep their ranges, because they are the reference's own designs and R-AUD-079 needs them. R-AUD-050 is repointed. Requests to P6-ENG1 (AEN-070, 071, 235) carry the removed values. |
| R2a-82 | fixed | The Id column is dropped, and each binding is named by its event and layer. The 15 catalogue effect cells, §5 and AC-EVM-11 are updated. AEN-071 and DAT T54 are requests. |
| R2a-96 | fixed-differently | Merged with R3a-62. R-AUD-314 now states the Species rule: the voiced moments `leader.order_follow`, `leader.order_none`, `relic.lift` and `relic.carry` stay silent until DEC-565 binds them, with the voice-line sizing kept as the proposed text has it. I dropped "1 announcement of a crate opened by another team", because no event raises it, and the match-end announcements, which are bound now (R-AUD-307). |
| R2a-98 | fixed | Merged with the P6-RA request (see Requests). |
| R3a-14 | fixed | R-AUD-403 now says the R-AUD-414 path replaces the hook channel under DEC-049 option A. The cross-fading voice pair moved from R-AUD-405 to the new R-AUD-414 (ADR-024, ADR-025, DEC-049 option A). |
| R3a-15 | fixed | R-AUD-404 keeps the reference rule. The Species rule moved to the new R-AUD-415. AC-BRF-07 is split into AC-BRF-07 and AC-BRF-08. |
| R2a-99 | fixed-differently | R-AUD-404 keeps the reference hook rule, not the proposed Species-only text. R3a-15 asks for that, and AEN-360 runs the hook channel with this rule until DEC-049 decides. The reference envelope statistics are deleted. The Species rule is R-AUD-415, and the AC is split. |
| R2a-102 | fixed | R-AUD-407 now says the two stings are needed now and the theme and beds only under option A (a "Needed" column is added). R-AUD-408 is a stub, and its brief goes to DEC-049 option A (request). |
| R3a-13 | fixed | Same change as R2a-102. |
| R2a-83 | fixed-differently | The tag is deleted from the 10 README (file row and §1.1), the catalogue's source row 7, and the music file (§1, heading, R-AUD-400, diagram caption). A hidden anchor keeps the generated index's link alive until the index is regenerated. The audio-engine §8.2 heading is a request. |
| R2a-91 | fixed | R-AUD-219 is a stub, because AEN-156 cites it. Its 3 keys are deleted, the last sentence of R-AUD-215 is deleted, and "Dense fights" keeps only the DEC-570 sentence. AEN-156 is a request. |
| R2a-71 | fixed | Sentence deleted. §8 now cites DEC-226 item 4. |
| R3a-49 | fixed | 12 per-mode keys (`.canvas_size` in canvas units, `.loop_span` in s of real time). `ref.meta.explainer.key_defaults` is also split into 8 per-field keys. DAT table 3.10-B is a request. |
| R3a-74 | fixed | DEC-245 added to §8. |
| R2a-74 | fixed | R-META-456 is now "Settings not listed in §2.3 to §2.8 are not specified (DEC-250)": §2.8 is included because it holds R-META-511. R-META-428 and 438 are stubs. R-META-426 compares only the four detail levels, and the register rows for the inert terrain-detail and pixel-effect entries are dropped. R-META-458 item 5 is repointed. |
| R2a-76 | fixed | Proposed text used. |
| R2a-77 | fixed | Sentence deleted. |
| R2a-79 | fixed-differently | Item 2 is deleted. **Item 1 is rewritten** as the DEC-100 rule that each setting has one default, which every reader uses: most of its content had already gone with R2a-72 and R2a-73. In item 5 the reference narrative was the *first* sentence, so that sentence was removed and the ADR-063 sentence kept. The cross-references in R-META-403, 404 and 463 are updated. §5 is renumbered; nothing cites items above 1. |
| R2a-67 | fixed | Proposed text used. |
| R2a-68 | fixed | Clause deleted. |
| R2a-69 | fixed | Sentence and key deleted. The UI widget-binding row is a request. |
| R2a-70 | fixed | R-META-070 is a stub. The table row and the R-META-074 item are deleted, and the §8 DEC-115 line is removed (DEC-115 stays cited elsewhere). Consistency requests go to P6-RA, P6-RB and P6-DAT. |
| R2a-85 | fixed | Proposed text used. |
| R2a-86 | fixed | Sentence deleted. The mortar turret is still specified in 03, so its silent shot `turret.fire_grenade` is now stated in R-AUD-145 (citing R-BLDG-476). Repointing the 03 citation is a request to P6-RB. |
| R2a-87 | fixed | Lead-in replaced, and the column is renamed "Binding". |
| R2a-90 | fixed | Proposed text used. |
| R3a-75 | fixed | DEC-049 added to §8. |
| R2a-84 | fixed | Narratives deleted from R-AUD-031, 041 and 044 and from §5 "Neutral Flag posts"; the rules keep their DEC-097 citations. |
| R3a-76 | fixed | DEC-317 added to §8. |
| R3a-62 | fixed | Merged with R2a-96: R-AUD-314 now states which events stay silent. DEC-565 added to §8 for R-AUD-314. |
| R2a-97 | fixed | Sentence added to R-AUD-319 (citing R-MAP-033), and the R-AUD-313 row is marked "not reached in a match". |
| R3a-77 | fixed | DEC-003 and DEC-049 added to §8. |
| R2a-100 | fixed-differently | R-AUD-405 now has no reference-versus-Species contrast and states the music bus with its own volume (DEC-570). The "streamed by a voice pair that cross-fades" clause went to R-AUD-414 instead, because R3a-14 shows it contradicts R-AUD-403 while DEC-049 is Open. |
| R2a-101 | fixed | Proposed text used. |
| R3a-63 | fixed | Recast as a SHOULD variety target with 4 `ref.aud.variety.*` keys in a new §4 tuning table. |
| R2a-103 | fixed | Merged with R3a-63: the reference file count is now a target, and the first-playable milestone is removed. |
| R2a-104 | fixed | Proposed text used. |
| R2a-94 | fixed | Proposed text used. |
| R3a-51 | fixed | R-AUD-211 cites `aen.pitch_bend.time_constant` (0.17 s), and the §4 note is updated. |
| R2a-95 | fixed | The note is now "the crowd class radius". |
| R2a-92 | fixed | Item deleted. |
| R2a-93 | fixed | Item deleted. |
| request from P6-RA | fixed | In R-AUD-400 the packaged-data clause is replaced by "and the sound data names music events that no rule raises", merged with R2a-98: "no menu music and no victory or defeat cue apart from the stings of R-AUD-307". **Conflict:** the DEC-049 restatement that P6-RA asked of P6-DEC says "No match-end stings exist", which contradicts R-AUD-307: match_ui.victory and match_ui.defeat play bound stings (DEC-097), and match_ui.game_over plays a new game-over announcement. Requests, item 14, gives a correction. |

## Requests

1. **P6-ENG2, `architecture/components/platform_layer.md` PLT-029 and table 3.4-A.** Replace PLT-029 with: "The screen settings that the store holds are the render size and the window mode ([renderer options](renderer/options_and_budgets.md) RND-442; [options, localisation and hooks](../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) R-META-414, R-META-424)." Delete table 3.4-A, or keep only its render-size and window-mode rows. The citations `ref.meta.options.screen.colour_default`, `ref.meta.options.screen.refresh_default`, R-META-418 and R-META-419 no longer exist (those are stubs now). This is ruling 4.
2. **P6-ENG2, `platform_layer.md` PLT-073.** Replace "the size and refresh lists of the reference screen-options page (R-META-415, R-META-416)" with "the size list of the screen-options page (R-META-415)".
3. **P6-ENG2, `platform_layer.md` PLT-113, table 6.1-A, the `plt.display.reference.*` keys of §4.16 and the sentence that introduces them.** Delete them (ruling 4). R-META-423 and R-META-424 now cite PLT-112 only. Also confirm in PLT-112 that the chain runs both at start-up and when the screen options apply a new size or window mode, which R-META-423 now states.
4. **P6-DAT, `architecture/components/data_schemas/config_input_and_strings.md` table 3.1-A.**
   - Delete these rows: refresh rate, colour bits, depth bits, vertical-sync request and overscan, terrain detail and pixel effect, audio output, mixing rate, voices, sample-memory level, output buffer length, hardware positioning.
   - In the window-mode row, delete "; the page's reference value is `ref.meta.options.screen.windowed_default`" (the key no longer exists).
   - In the control-help/difficulty and seasonal-theme rows, replace "R-META-456" with "R-META-449".
   - The caption's "audio restart … (R-META-437)": R-META-437 now says the apply never restarts or cuts a sound, so the scope column of the remaining audio rows needs review with P6-ENG1.
5. **P6-DAT, `config_input_and_strings.md` DAT-193, last sentence.** Replace it with: "Each setting has one default, which every reader of the setting, the options pages included, uses ([reference/09](…) §5 item 1)."
6. **P6-DAT, `data_schemas/entity_audio_geometry_and_panel_schemas.md`.**
   - Table 3.5-A caption: replace "ranges and declared defaults are R-AUD-077's" with "ranges and declared defaults are AEN-070's for kinds 0 to 8 and R-AUD-077's for kinds 9 to 13".
   - DAT-271: replace "the parameter's declared default of R-AUD-077 (AEN-071)" with "the parameter's declared default (AEN-070, R-AUD-077; AEN-071)".
   - T54: replace "every binding FX-01 to FX-15 of R-AUD-082" with "every effect binding of R-AUD-082".
   - Table 3.10-B caption: replace "their defaults are `ref.meta.explainer.key_defaults` (R-META-226)" with "their defaults are the `ref.meta.explainer.key_default.<field>` keys (R-META-226)".
7. **P6-DAT, `data_schemas/map_schema.md` DAT-100.** Delete "research levels" from the list in parentheses; the match setup no longer carries them (R2a-70).
8. **P6-RND, `architecture/components/renderer/options_and_budgets.md` RND-452.** Replace "the screen options page shows the render size, the window mode, the refresh rate, the colour bits and the depth bits ([options_localisation_and_hooks.md](…) R-META-414 to R-META-420), and of these the Species engine has settings for the render size and the window mode only (RND-443)" with "the screen options page shows the render size and the window mode ([options_localisation_and_hooks.md](…) R-META-414)".
9. **P6-ENG1, `architecture/components/audio_engine/README.md` AEN-070.** R-AUD-077 no longer states the parameter ranges and declared defaults of the standard kinds 0 to 8 (R2a-80), so AEN-070 becomes their owner.
   - Replace "in the order of [event model] R-AUD-077" with "in the order of [data schemas] table 3.5-A".
   - Add the ranges and declared defaults that R-AUD-077 held, as range and declared default; i = integer (or Species values of the engine's own choosing under ADR-024):
     - chorus: wet/dry mix 0 to 100, 50; depth 0 to 100, 10; feedback −99 to 99, 25; frequency 0 to 10, 1.1; waveform 0 to 1, 1 (i); delay 0 to 20, 16; phase 0 to 4, 3 (i).
     - compressor: gain −60 to 60, 0; attack 0.01 to 500, 10; release 50 to 3000, 200; threshold −60 to 0, −20; ratio 1 to 100, 3; pre-delay 0 to 4, 4.
     - distortion: gain −60 to 0, −18; edge 0 to 100, 15; post-equaliser centre 100 to 8000, 2400; post-equaliser bandwidth 100 to 8000, 2400; pre-low-pass cut-off 100 to 8000, 8000.
     - stereo echo: wet/dry mix 0 to 100, 50; feedback 0 to 100, 50; left delay 1 to 2000, 500; right delay 1 to 2000, 500; pan delay 0 to 1, 0 (i).
     - flanger: wet/dry mix 0 to 100, 50; depth 0 to 100, 100; feedback −99 to 99, −50; frequency 0 to 10, 0.25; waveform 0 to 1, 1 (i); delay 0 to 4, 2; phase 0 to 4, 2 (i).
     - standard gargle: rate 1 to 100 Hz, 20 (i); wave shape 0 to 1, 0 (i).
     - room reverb: room −10000 to 0, −1000 (i); room high-frequency −10000 to 0, −100 (i); room roll-off factor 0 to 10, 0; decay time 0.1 to 20, 1.49; decay high-frequency ratio 0.1 to 2, 0.83; reflections −10000 to 1000, −2602 (i); reflections delay 0 to 0.3, 0.01; reverb −10000 to 2000, 200 (i); reverb delay 0 to 0.1, 0.01; diffusion 0 to 100, 100; density 0 to 100, 100; high-frequency reference 20 to 20000, 5000; quality 0 to 3, 2 (i).
     - parametric equaliser: centre 80 to 16000, 8000; bandwidth 1 to 36, 12; gain −15 to 15, 0.
     - studio reverb: input gain −96 to 0, 0; reverb mix −96 to 0, 0; reverb time 0.01 to 3000, 1000; high-frequency time ratio 0.01 to 0.99, 0.01.
   - AEN-071: replace "its declared default of R-AUD-077" with "its declared default (AEN-070 for kinds 0 to 8, R-AUD-077 for kinds 9 to 13)"; in item 2, replace "FX-01 to FX-15" with "the 15 effect bindings".
10. **P6-ENG1, `audio_engine/spatialisation_mixing_and_streaming.md` AEN-235.** Replace "implements its parameter set of R-AUD-077" with "implements its parameter set of AEN-070". AEN-260 (the reference restart from the options, with its hardware-positioning retry) describes driver-level internals of options that no longer exist (ruling 4): delete it or reduce it. AEN-261 may keep `RestartDevice`, but the only remaining applied setting is swap, and R-META-437 now states only that no sound restarts or is cut.
11. **P6-ENG1, `audio_engine/README.md` AEN-076, AEN-077, §8.2 and AEN-361.**
    - AEN-076: delete the table rows audio output, voices, mixing rate, output buffer length, hardware positioning and sample-memory level (R-META-429, R-META-454; DEC-869 option B; ruling 4).
    - AEN-077: the reference sample-memory model goes with them.
    - §8.2: rename the heading "Music hook (KH-MUSIC)" to "Music hook".
    - AEN-361: repoint "(ADR-024, R-AUD-405)" to "(ADR-024, R-AUD-414)".
12. **P6-ENG1, `audio_engine/instances_voices_and_envelopes.md` AEN-156 and AEN-130.**
    - AEN-156 cites R-AUD-219, which is now a removed stub, and the deleted `ref.aud.reference.*` keys. Delete the reference-allocator model or restate it as engine-internal history-free text, and repoint it to R-AUD-217 and R-AUD-218. Check AEN-265 (start latency, reference model) for the same.
    - AEN-130: append "With no object, the instance starts at the world origin with no emitter, and its layer's position kind applies: a `2d` layer plays at the listener, a `fix` layer at the origin, and an `att` layer is removed at the first update (R-AUD-021, R-AUD-209)."
13. **P6-ENG1, `ui_framework/`.**
    - `core_and_widgets.md`: replace the mode-option row "one entry of the mode option array of `ref.meta.setup.mode_option_slots` (64) entries; set requests that entry, clamped" with "one option of the current mode's typed schema ([reference/01] R-MODE-073); set requests that option, clamped" (the key is deleted). In UI-090, replace the example `ui.dialog.screen_mode_failed` (deleted) with another string, for example `ui.dialog.settings_save_failed`, and delete it from the "String ids cited from other documents" list.
    - `pages_and_screen_recipes.md`: the `Achievements` row becomes "hook: R-META-491 (DEC-047)", with no row loader. `LeaderboardSelect` and `Leaderboard` become "hook: R-META-498 (DEC-047)", without selector bindings. Delete the "discards the achievement rows (R-META-492)" row, the loading caption of the `Achievements` hook page in UI-392, the achievement-row slide-in row and the `ref.meta.achievement.row_slide_speed` citation (the key is deleted).
    - `input/controls_and_default_bindings.md` rows 10 to 14: `action.menu.cycle_up`, `cycle_down`, `select_previous`, `select_next` and `toggle_period` cite R-META-500 to 502, now stubs. Repoint them to R-META-498 (a DEC-047 hook), or drop them if nothing else uses them.
14. **P6-DEC.**
    - **DEC-205:** add to the specified behaviour: "A joining participant that has received a message from the host and then receives none for `ref.meta.connect.host_silence_limit` (5.0 s of real time) ends its session and shows `ui.error.connection` with the title page as its back page, on any page and in a match; while it is still connecting it waits without a timeout (R-META-056, R-META-059, R-META-092)." Add R-META-059 to Affects.
    - **DEC-869:** make it Dropped (ruling 4), with the statement: "The Species sound options page offers the master and bus volumes and the swap of left and right. The output device, the mixing rate and the voice count are fixed by the audio engine (ADR-022, ADR-025, DEC-570), so no page offers them (R-META-429)." Keep the id.
    - **DEC-565:** add to the specified behaviour: "the 100 % capture-progress tone `supply_crate.capture_progress_100`; and a sting of their own for the rocket turret, flame turret and Firebomb Run outcomes, which play the gun-turret and airstrike stings, and for the Builder, Time Dilation and Time Rush outcomes, which play none (R-AUD-139, R-AUD-411)". Delete the sentence "The reference sound set holds unbound recordings of …" (provenance, as in R2a-88, 89 and 96).
    - **DEC-049:** option A takes the brief removed from R-AUD-408: "A — a state-driven score, newly composed, with layers for the front end, the lobby, the match introduction, low and high match intensity (driven by the units in combat near the camera's focus or by the score margin), the last minute, victory, defeat and draw; played on the streamed, two-voice, cross-fading music bus (R-AUD-414, R-AUD-415) with transitions synchronised to bars, its own player volume, and stings that duck the music; the camera-altitude ambience mix of R-AUD-323 stays alongside it; its character follows assets/art_and_audio_briefs.md §15." In the restated specified behaviour that P6-RA requested, replace "No match-end stings exist." with "The match end plays the victory and defeat stings and the game-over announcement bound by DEC-097 (R-AUD-307); no other music exists." The current text contradicts R-AUD-307.
15. **P6-GLO, `assets/asset_manifest.csv` and `assets/GameAssets.md`.** Add six sting groups: `sfx.crate.outcome.rocket_turret`, `sfx.crate.outcome.flame_turret`, `sfx.crate.outcome.firebomb_run`, `sfx.crate.outcome.builder`, `sfx.crate.outcome.time_dilation`, `sfx.crate.outcome.time_rush`. Use the same category and format as `sfx.crate.outcome.gun_turret`, consumers `R-AUD-139;R-AUD-411`, role "Announcement sting when a Supply crate yields <outcome>; bound only if DEC-565 binds it", and priority `post-launch` (later than the 25 bound groups) (R1a-4). My documents do not cite these ids yet, so the gate stays green until the rows exist.
16. **P6-GLO, `assets/GameAssets.md` string counts.** `ui.options.*` loses 12 strings: 6 screen strings (`refresh`, `refresh_value`, `colour_bits`, `depth_bits`, `bits_value`, `fullscreen_only`) and 6 sound strings (`output`, `output_platform`, `mix_rate`, `rate_value`, `voices`, `voices_value`). `ui.dialog.*` loses `ui.dialog.screen_mode_failed`. Update the counts in the string tables near lines 1063 to 1066, 1132 and 1149.
17. **P6-RB, `reference/03_buildings_and_world_objects/defences_and_props.md` table 2.10-A, row `turret.fire_grenade`.** Repoint "R-AUD-136" to "R-AUD-145"; the mortar turret's silent shot is now stated there.
18. **P6-RB, `reference/02_units/README.md` R-UNIT-008 (consistency with R2a-70, optional).** Replace "after any research setting of the match setup has been applied, so the match setup's research settings have no effect in a match" with "at match start"; the match setup carries no research levels (R-META-069).
19. **P6-RA, `reference/01_skirmish_rules_and_modes/README.md` R-MODE-082 table, row "research levels" (consistency with R2a-70, optional).** Restate it as "research levels | not part of the match setup | — | fixed at every match start: …", keeping the fixed values.
20. **P6-RA:** request applied; see the last row of the table above.
21. **Coordinator:** regenerate `REQUIREMENTS_INDEX.md` and `requirements_index/`. It will pick up R-AUD-414 and R-AUD-415 and the new first sentences of the stubbed and rewritten requirements.
22. **Coordinator:** after regenerating the index, delete the line `<a id="21-music-hooks-kh-music"></a>` in `reference/10_audio_design/music_hooks_and_sound_design_brief.md` §2.1. Optionally delete the stubs that only the index cites: R-META-070, 428, 438, 503 and 504, and R-AUD-408.

## Out of scope, noted only

The same reference-versus-Species narration remains in text my packet does not list. I left it unchanged:

- R-META-467: "In the reference five languages have a name string …".
- §5 item 3 of the options file: "the reference apply would then fail".
- §5 item 7 of the options file: "In the reference both the graphics apply …".
- The Species contrast in R-META-427.
- In event_model_and_sound_data.md, the "the reference …" clauses of R-AUD-022, R-AUD-050, R-AUD-061, R-AUD-062, R-AUD-063 and R-AUD-065. R-AUD-065 names the low-memory setting, which falls under ruling 4.
