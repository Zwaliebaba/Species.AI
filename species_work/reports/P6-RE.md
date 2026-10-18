# P6-RE report — reference/12 simulation model

## Summary

- Findings: 44 (6 High, 5 Medium, 33 Low). **35 fixed, 8 fixed differently, 1 rejected, 0 left as request only.** Every finding with a change outside my files also has a numbered request below.
- Requests from P6-RB applied (coordinator relay): P6-RB requests 6, 7 and 8 (07 R-SIM-622 Wisp row split, 07 Table 1 "tree shape generator" row as given, 03 R-SIM-109 initial reload draw in ticks as given).
- Files edited: `design_next/reference/12_simulation_model/01_tick_timing_and_main_loop.md`, `02_match_lifecycle_and_state_reset.md`, `03_entity_model_and_identity.md`, `04_containers_and_update_order.md`, `05_space_routing_and_physics.md`, `06_teams_and_player_commands.md`, `07_randomness_and_integrity.md`, `README.md`. Headings and anchors unchanged; no requirement id removed or added.
- Tuning keys retired (ids not reused): `ref.sim.frame.low_fps_audio_threshold`, `ref.sim.weather.shared_timer_initial`, `ref.sim.weather.first_run_threshold`, `ref.sim.bot.controller_spawn_request`. Value changed: `ref.sim.speed.crate_hold` 150 → 151 world ticks.
- Diagrams changed: 01 Figure 4 (node C7 removed, steps renumbered) and Figure 5 (tick-clock and slice labels); 03 diagram 2.2-A (`Render` removed).
- Gates: after my edits, `python3 -I Tools/DesignCheck/CheckDesign.py design_next --skip-mermaid` passed every gate, and so did the full run with Mermaid (266 blocks, 0 failed).
  - A final re-run, after one more one-line edit in 07, failed gates 1 and 2 on four hits, all in files I do not own and that another packet is editing now: `ADR/ADR-019-mesh-source-format-and-cooked-meshes.md:88` (`docs/…` paths) and `ADR/ADR-017-texture-container-and-block-compression.md:97` ("legacy", "porting").
  - That re-run found no hit in `reference/12_simulation_model/`, and gates 3 and 5 to 11 pass.
- ORIG read for fidelity only: `design/04_crates_powerups_and_weapons.md` 910–950 (R1a-1, sample row 96) and `design/03_buildings_and_world_objects.md` 1176–1216 (R1a-2, sample row 71). R1a-2's proposed fix asked to check the source simulation-model section's obstacle-grid rule first; I located it with one grep and read `design/21_simulation_and_entity_model.md` 1080–1095 and 1736–1740 and `design/41_risks_and_open_questions.md` 769–773 for that check only. Nothing was copied.
- Helper scripts: `species_work/scripts/P6-RE_tickclock.py` (the reference tick clock, float and integer forms) and `species_work/scripts/P6-RE_a6_species.py` (the A6 vectors under tick scheduling).

### How the tick clock was settled (R4b-1, R4b-2, R1a-1, R4b-3)

- **Reference behaviour checked.** At ORIG 04:931 the hold of 15.0 s goes down by 0.1 per tick, starting on the tick that reaches the target. Because of the floating-point remainder it ends after **151** ticks. A simulation of that arithmetic gives the A6 values the library had: back at 1.0 after 316 ticks for a target of 0.3, and 117 ticks for the meteor, whose 40-tick hold is right. With a hold of 150 every crate row would end one tick early. So the key is now **151**: "15 s as the integer count 151, like the other 15 s timers of DEC-002".
- **Requirements now describe the Species model (DEC-039, Resolved).**
  - The tick-clock update runs once per command frame and does two things: it records the tick-take time and it blends the speed.
  - At the end of every world tick, match time goes up by `ref.sim.tick_period` and an integer hold counts down by 1.
  - A command frame's world ticks run at the speed that the frame's update set.
  - R-SIM-547 defines match time from the completed world ticks. R-SIM-548 says that within a frame, *s* is the speed set by that frame's update. R-SIM-549 lists which steps follow the real-time cadence and which count world ticks.
  - The "reference structure" clauses (variable time step) are gone: under ruling 3 they belong in DEC-039 option B.
- **A6 is now a Species vector table, so it agrees with the rules.**
  - It counts command frames and world ticks, under one stated test condition: the world ticks are the whole part of the running speed integral, from no carried fraction.
  - Ramps are unchanged (86/83/80/67/82/90/39 frames, 21 frames for pause, resume and match won).
  - Holds now count world ticks (DEC-265 default A). For example, a Time Dilation to 0.3 is back at 1.0 after 666 frames (66.6 s), against 316 in the reference.
  - The "game time elapsed" column became "world ticks run". The ramp game times of R-SIM-554 and R-SIM-555 changed from 0.495 s and 1.605 s (match time read with the speed before the blend, a variable-step effect) to speed integrals of 3.95 and 17.05 world ticks.
  - The reference 15.1 s hold and its vectors belong in DEC-265 option B (request 2).
- R-SIM-557 gives the real durations with 151: 37.75 s to 75.5 s for a Time Dilation, 7.55 s to about 11.6 s for a Time Rush. It now also names the meteor hold (40 world ticks, 10 s at 0.4×), which counts world ticks under the same rule.

## Findings

| Id | Status | Note |
|---|---|---|
| R4b-1 | fixed-differently | Step 2 removed. Match time is now +`ref.sim.tick_period` at the end of each world tick (R-SIM-541 step 3), and R-SIM-547 defines it from completed world ticks. The "× *s*" reference-structure clause was dropped, not kept "marked as not built": a description of the reference's variable step belongs in DEC-039 option B (ruling 3). R-SIM-510, R-SIM-515, the R-SIM-570 clock table, R-SIM-711 and Figure 5 were aligned. |
| R4b-2 | fixed | R-SIM-541 step 4: the integer hold, loaded from its key, decreases by 1 at the end of each world tick; at 0 it requests normal speed, which the next frame starts to blend. Hold column of R-SIM-545 and both key units are now "world ticks". |
| R1a-1 | fixed-differently | Checked at ORIG 04:931: 151 ticks. `ref.sim.speed.crate_hold` = 151 and step 4 is an integer countdown, as proposed. A6 was not "kept unchanged": its vectors were those of the reference cadence, where the hold counts command frames, so they contradicted R-SIM-548/549 (the hold counts world ticks, DEC-265 A). A6 is now restated in command frames and world ticks (computed by `P6-RE_a6_species.py`), with the ramps unchanged. R-SIM-557 recomputed with 151. The other quoted 150s are requests 1–3. |
| R1a-2 | fixed-differently | The finding is wrong about observable behaviour. ORIG 03:1196 gives the intended rule (extra cells only at diagonal steps). The source simulation-model obstacle-grid rule states that this intent is defeated, so every sample with non-zero cell x and z adds its four neighbours. That is R-SIM-322 as written. Rule and `ref.sim.obstacle_grid.emitter_dilation` kept. As the finding says, the dilation does change coarse-sampled tests, so R-SIM-322's last sentence and §5 item 7 now say so (turret line of sight, R-BLDG-413). Following the proposal's own condition, a new Open DEC (default per-sample, option B the edge-connected line) is request 2d. |
| R2a-127 | fixed | R-SIM-656 restated as the Species rule. It lists the simulation fields that drawing only reads and gives the content-build rule for dangling links (DEC-015). Table 4 is now a two-column list of the presentation fields and their owners (structure, actor, effect and route-line renderers, match HUD, cloth). A9 aligned. |
| R2a-126 | fixed | R-SIM-650 states the crown hit volume as a simulation input (fire spread, R-BLDG-610; hit tests, R-BLDG-607). R-SIM-651 matches R-BLDG-603: baked for map trees, and computed once with a tree-local generator for trees created in a match. A12 has no thread clause. Table 2 and Table 3 tree rows aligned. |
| R4b-3 | fixed | R-SIM-524 reworded as proposed. The reference-structure clause is dropped (DEC-039 option B), not kept as a note. |
| R2a-109 | fixed | R-SIM-571 now gives the DEC-955 rule. R-SIM-573's reference sentence and "In Species" removed. The §5 "Paused presentation" item rewritten. The presentation frame time and frame delta rows of R-SIM-570 now match DEC-955. |
| R4a-20 | fixed | `+Render` removed from diagram 2.2-A. Table 2.3-A's render row reads "a simulated object has no render hook". R-SIM-130 now says presentation draws from the presentation snapshot (DEC-222) through the game-object renderers. Table 2.3-C names `StructureRenderer`, `ActorRenderer`, `EffectRenderer`, `RouteLineRenderer` and the Essence-orb pass. README document map aligned. The heading "2.3.6 Render hooks" is kept so the anchor stays valid (request 10). |
| R2a-129 | fixed | R-SIM-607 keeps the draw forms, the frame-thread re-seed with `ref.sim.match.presentation_seed`, and the self-seeded presentation content (TER-185, R-BLDG-604). The "In the reference" bullets, R-SIM-610's last sentence, R-SIM-613's clause, R-SIM-626's narrative sentence (its rule sentence kept), R-SIM-627's clause, the §5 "Instrumented builds" item and the key note ("presentation draws") were all deleted or reworded. R-SIM-608 unchanged. |
| R2a-128 | fixed | The §1 clause was deleted. Every "In the reference" sentence of R-SIM-661, 663, 665, 666, 667, 672, 673, 674, 675, 705, 725, 726, 730 and 732 was deleted, keeping or restating its rule. R-SIM-726 now uses the date rule of R-META-102. The §5 "Sounds off the main thread" item was deleted. R-SIM-664's clause and R-SIM-691's code inventory were also restated. |
| R2a-108 | fixed | "Its length is open (DEC-003); the default is 11 ticks (DEC-002)." |
| R3a-36 | fixed | Both 10s in R-SIM-531 now use `ref.sim.update_slices`. |
| R2a-111 | fixed | R-SIM-575 as proposed. Two other documents still cite it for the vsync claim (requests 5, 6). |
| R2a-110 | fixed | Steps 11.8 and 11.16, the key, the R-SIM-576 reader and node C7 deleted. Steps renumbered to 11.1–11.14, with R-SIM-583 and Figure 4 aligned. Step 11.12 (was 11.13) keeps the own-thread audio statement (AEN-116). The R-SIM-570 audio row restated as the audio clock (AEN-006, AEN-116, AEN-120). Citations elsewhere: requests 4–6. |
| R2a-112 | fixed | Single-precision sentence deleted. |
| R4a-52 | fixed | "the constant `TICK_PERIOD_SECONDS`" added to the row. |
| R3a-79 | fixed | DEC-051 added to §8. |
| R2a-116 | fixed-differently | "edition" is kept, because the permitted maps are a field of a Species build edition (BLD-053 `maps`). Only the reference-edition clause was replaced, with "an edition whose `maps` field is `all` permits every map it holds". AC-18 now reads "an edition whose `maps` field lists permitted maps that exclude map M". BLD-053's own last sentence is request 7. |
| R2a-113 | fixed | Both rows deleted. No cursor-marker row was added, because R-SIM-666 makes the marker a presentation event that names its team. |
| R2a-115 | fixed | The three §5 items are reduced to their rules (R-SIM-024, R-SIM-020, R-SIM-019). |
| R2a-114 | fixed | R-SIM-021 gains "No input of a match sends the weapon-cycle commands (R-SIM-452); the debounce applies to such commands when a participant or a test harness sends them anyway". It cites R-SIM-452 because no R-CTRL rule names that input. The §5 item is deleted. |
| R3a-37 | fixed | R-SIM-213 is written with `ref.sim.update_slices` − 2 and "the last slice". |
| R4a-68 | fixed | "Relic Spawn Zone". |
| R3a-52 | fixed-differently | Key retired; see R2a-117. No duration in seconds remains. |
| R3a-53 | fixed-differently | Key retired; see R2a-117. |
| R2a-117 | fixed | A first-run flag replaces the sentinel (R-SIM-233): the shared timer starts at 0 ticks, and the flag is set at match-world creation and cleared by the timer's first count-down. Behaviour is unchanged, because the first count-down from 0 or from −999 both reach ≤ 0, and on a snow-and-dust map the snow spawner's count-down still prevents the dust burst. R-SIM-234, R-SIM-235, table 2.1-A and AC-14 updated. New AC-17 covers snow plus dust. |
| R4b-23 | fixed-differently | Keys retired; see R2a-117. |
| R2a-118 | fixed | The row now states the rule and its key, with the owning link R-BOT-090 (DEC-087). |
| R2a-119 | fixed | Supersession clause deleted. |
| R2a-120 | rejected | The kind does have a source: R-MAP-120 gives map routes "ground position (x, z)" points, and DAT-093 cooks the `ground` field to `RoutePointKind::GroundPosition`. Deleting it would break DAT-093. The row now names that source and adds that no match follows a map's route (R-MAP-121). CONVENTIONS needs no change. |
| R2a-122 | fixed | Step 3 as proposed. The key note now reads "the unused-slot colours: (draw mod 8) div 2", requirement R-SIM-422, DEC-450. R-SIM-422 no longer points to step 3. |
| R2a-123 | fixed | Stated as a rule: after co-op setup the local lobby sends `SetColor`, and executing it changes nothing. §5 item 14 aligned. |
| R2a-121 | fixed | Sentence and key deleted. 07 R-SIM-628 and the 07 key list no longer name the key. Nothing outside 12 cites it. |
| R2a-124 | fixed | Second option: the row is kept and says why. It carries the score report that R-MODE-057 step 2 issues and that R-META-497 says no statistic consumes (DEC-047). §5 item 20 aligned. See request 9 for a tension with R-SIM-454. |
| R2a-125 | fixed-differently | "or the research levels" removed from both option commands. The Leader-mode clause replaced by "with any other selection the command does nothing". The Strike-team sentence moved into R-SIM-450 (DEC-117). R-SIM-452 is **kept**, restated without reference narrative, for two reasons. First, transport.md R-UNIT-394 and AC-TR-09 (P6-RB) specify `NextMode`'s Transport effect, and R-SIM-021's debounce, which R2a-114 keeps, acts on it. Second, DEC-202 (Open) needs a defined result for every command a participant can send. §5 item 21 aligned. |
| R3a-80 | fixed | DEC-205, DEC-206 and DEC-207 added to §8 (cited in R-SIM-448). |
| R2a-130 | fixed | Column renamed "Presentation draws", with the lead restated. R-SIM-623 now reads "These loops … They are: …" with the list kept. The §5 item reworded as proposed ("Overridden draws"). |
| R2a-132 | fixed | As proposed. |
| R3a-64 | fixed | Rule kept; the divergence narrative deleted. DEC-008 option B already records the reference behaviour, so nothing needed moving. |
| R3a-65 | fixed | Rule kept; narrative deleted. DEC-011 option B already records it; extra detail is optional request 2c. |
| R2a-133 | fixed | R-SIM-740 and A17 as proposed. R-SIM-741 restated: the recording holds only the command history, not the setup or seed (DEC-046). |
| R2a-131 | fixed | Table 3 rows (centipede copies, online-service registration) deleted. The Table 2 orb-rain clause deleted. The R-SIM-669 launch-spectacle clause deleted. R-SIM-725 keeps only its first sentence. The §5 "Unreachable crossings" item deleted. |
| R3a-81 | fixed | DEC-114, DEC-220 and DEC-225 added to §8. |
| P6-RB request 6 | fixed | R-SIM-622 row split into snowflake wobble (unchanged) and Wisp wobble (three `NextSigned(1)`, R-UNIT-596). Table 1's snowflake/Wisp row split the same way. |
| P6-RB request 7 | fixed | Table 1 row "tree shape generator" added exactly as given. |
| P6-RB request 8 | fixed | R-SIM-109 initial reload draw replaced exactly as given. |

**Further edits made to keep my files consistent (no finding id):**
- 02 R-SIM-025, the table 2.5-A tick-clock row and AC-5 said "normal speed" at match start. They now agree with R-SIM-551, R-SIM-008 and R-SIM-012: speed 0, no request, no hold, no lock.
- 01 R-SIM-552, R-SIM-554 and R-SIM-555, the A9 and A10 wording and the pause and game-over key notes now count command frames. New §5 item "Holds at low speed".
- 07 Table 2: the "simulation-resident presentation" class became "presentation, keyed by handle" (R-SIM-635 and R-SIM-636 restated to match). The rows for route slots, audio (no simulation rule reads sound state) and the camera now state Species rules.
- 07 R-SIM-668 restated as keyed by the team. R-SIM-715's lead reworded.

## Requests

1. **P6-RC**: `reference/04_supply_crates_powers_and_weapons/crates_and_powers.md`.
   - R-CRATE-226: replace "`ref.sim.speed.crate_hold` (150 ticks)" with "`ref.sim.speed.crate_hold` (151 world ticks)". Replace "approaches its target by 5 % of the gap per tick" with "approaches its target by 5 % of the gap per command frame". Replace "37.5 s of real time at 0.4 × to 75 s at 0.2 ×" with "37.75 s of real time at 0.4 × to 75.5 s at 0.2 ×".
   - R-CRATE-228: replace "Its hold of 150 world ticks lasts from 7.5 s of real time at 2.0 × to about 11.5 s at 1.3 ×" with "Its hold of 151 world ticks lasts from 7.55 s of real time at 2.0 × to about 11.6 s at 1.3 ×".
   - Table row "Time Dilation, Time Rush \| 150-tick hold; …": replace with "151-world-tick hold; …".
   - AC-C-17: replace "the hold lasts 150 world ticks" with "the hold lasts 151 world ticks".
   - R-CRATE-203: replace "(40 ticks)" with "(40 world ticks)".
2. **P6-DEC**:
   - (a) DEC-039, Specified behaviour: replace "for a hold of 150 ticks (DEC-265)" with "for a hold of 151 world ticks (DEC-265)".
   - (b) DEC-265:
     - Specified behaviour: replace "(150) ticks of the world" with "(151) world ticks", and "from 37.5 s … 75 s … 7.5 s … about 11.5 s" with "from 37.75 s … 75.5 s … 7.55 s … about 11.6 s".
     - Replace the last sentence with "In the reference, where one tick runs per 0.1 s of real time at every speed, the hold lasts 151 command frames (15.1 s of real time) at every speed".
     - Add the meteor hold: `ref.sim.speed.meteor_hold`, 40 world ticks, which last 10 s at 0.4× under A and 4 s under B. Add R-UNIT-733 and R-CRATE-203 to Affects.
     - Option B: replace "(15 s)" with "(15.1 s)". Add "under B a request is back at 1.0 after 322, 316, 310, 284, 314 and 330 command frames for the targets 0.2, 0.3, 0.4, 1.3, 1.65 and 2.0, and the meteor after 117 (the A6 ramps of reference/12 01 plus a hold of 151 or 40 command frames)".
   - (c) DEC-011, Specified behaviour: replace "the tick-clock speed state, including any Time Dilation or Time Rush hold, returns to normal speed at match start, so game speed can never run away to an invalid value in the next match" with "the tick clock is reset at match start to game speed 0 with no request, no hold and no lock, so no Time Dilation or Time Rush hold reaches the next match (R-SIM-551)". Optional, option B: name the two specifics, "the Control Zone counter is process-wide and recomputed only at the start of a Zone Control match; the weapon-cycle debounce is one value shared by every Transport".
   - (d) New Open DEC "Barrier Emitter line in the obstacle grid":
     - Affects: reference/12 05 R-SIM-322; reference/03 defences_and_props R-BLDG-413.
     - Specified behaviour: a linked emitter adds its handle to the cell of every sample of its line and, for every sample whose cell x and z are both non-zero, to the four edge-adjacent cells, so the line occupies a band about three cells wide.
     - Options: A — as stated; B — the edge-connected line: where two consecutive samples lie in cells that differ in both x and z, only the two cells edge-adjacent to both also receive the handle.
     - Recommendation: A for the reference library. B changes which emitters the turret line of sight finds.
     - Decide by: design phase.
     - Then append "(see DEC-NNN)" to the second sentence of R-SIM-322 (my file; the coordinator may apply it or relay it back to me).
3. **P6-GLO**:
   - `glossary/m_to_z.md`, Time Dilation and Time Rush: replace "holds for 150 ticks of the world" with "holds for 151 world ticks".
   - `glossary/a_to_l.md`, game speed: replace "The multiplier on game time per tick (1 = normal)." with "How many world ticks run per tick period of real time (1 = normal, 0 = stopped); every world tick has the same length (DEC-039)."
4. **P6-ENG1**:
   - `camera_and_sequences.md` CAM-032: replace "at step 11.9 of R-SIM-581" with "at step 11.8 of R-SIM-581".
   - CAM-103: replace "(between steps 11.8 and 11.9 of R-SIM-581)" with "(between steps 11.7 and 11.8 of R-SIM-581)".
   - `audio_engine/README.md` AEN-115: delete "and once more after rendering when the frame rate is below `ref.sim.frame.low_fps_audio_threshold` ([tick timing](…) R-SIM-581)", because the key is retired.
   - AEN-006 and AEN-121 cite R-SIM-570/R-SIM-571 for the reference accumulator and for "the last frame delta keeps being added (R-SIM-571)". Those requirements now state the Species clocks (DEC-955), so drop the citations or the reference-model narration.
5. **P6-RND**: `renderer/options_and_budgets.md` RND-453.
   - Delete the row "slow frames \| below `ref.sim.frame.low_fps_audio_threshold` …" (key retired).
   - In the "frame pacing" row and in the present-mode row (line 88), drop the citation "(R-SIM-575)". R-SIM-575 no longer states the reference vertical-sync behaviour.
6. **P6-ENG2**:
   - `diagnostics_test_and_performance/performance_baseline.md`, line 37: delete the "slow frames … `ref.sim.frame.low_fps_audio_threshold`" row (key retired).
   - `platform_layer.md`, line 775: drop "(R-SIM-575)" from the vertical-sync row.
7. **P6-ADR**: `build_tooling_and_distribution.md` BLD-053, delete the last sentence "The reference edition permits every map it holds." (the `all` value already states it).
8. **P6-RB**, optional wording:
   - `reference/02_units/transport.md` R-UNIT-394: replace "No reference input issues these commands" with "No input of a match issues these commands".
   - `hazards_and_projectiles.md` R-UNIT-733: replace "(40 ticks)" with "(40 world ticks)".
9. **P6-RA** (observation): `reference/01_skirmish_rules_and_modes/README.md` R-MODE-057 step 2 has the result declaration, a simulation rule, "issue the score report". But R-SIM-454 says that advancing the simulation never issues a player command. Restate the score report either as a presentation event of the declaration, or as a `ReportScores` that the host's presentation issues on that event. R-SIM-451 needs no change in either case.
10. **Coordinator**:
    - Regenerate the requirements index: the one-line texts of R-SIM-541, 547, 557, 571, 575, 656 and others changed.
    - Optional: rename 03 §2.3.6 "Render hooks" to "Drawing" in the same commit as the regeneration, so the index anchor follows.
