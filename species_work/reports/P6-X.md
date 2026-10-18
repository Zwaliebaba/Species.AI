# P6-X report (Phase 6, round 2 fixer for P6-RA, P6-RB, P6-RC, P6-RD, P6-RE and P6-DAT)

## Summary

- **Sources read:** the "## Requests" sections of every `species_work/reports/P6-*.md` that exists now (P6-RA, P6-RB, P6-RC, P6-RD, P6-RE, P6-DAT, and P6-ADR and P6-ENG2, which finished during this run), and `species_work/p6_round2_queue.md`.
- **Skipped as already applied:** the target's own "applied" table or the coordinator covers these.
  - P6-RA requests 1 and 2 (P6-RB applied them) and 5 (P6-RD).
  - P6-RB requests 1 to 3 (P6-RA), 6 to 8 (P6-RE), 9 and 10 (P6-DAT), 12 and 13 (P6-RC).
  - P6-RD requests 4 to 7 (P6-DAT) and 20.
  - P6-RE request 1 (P6-RC).
  - P6-DAT requests 5 and 6 (the coordinator).
  - The queue's coordinator items are not addressed to my packets.
- **Result:** 28 requests handled: **23 applied, 4 applied differently, 1 not applied**. The one not applied was already done.
- **Files edited**, all owned by me:
  - `reference/01_skirmish_rules_and_modes/README.md`;
  - `reference/02_units/README.md`, `worker.md`, `transport.md`, `hazards_and_projectiles.md`;
  - `reference/03_buildings_and_world_objects/defences_and_props.md`;
  - `reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md`, `options_localisation_and_hooks.md`;
  - `reference/10_audio_design/front_end_hud_voice_and_ambience.md`;
  - `reference/11_tutorial_and_help.md`;
  - `reference/12_simulation_model/01_tick_timing_and_main_loop.md`, `02_match_lifecycle_and_state_reset.md`, `04_containers_and_update_order.md`, `05_space_routing_and_physics.md`, `06_teams_and_player_commands.md`, `07_randomness_and_integrity.md`;
  - `architecture/components/data_schemas/README.md`, `map_schema.md`, `entity_audio_geometry_and_panel_schemas.md` (the §8.1 ADR-076 row and T57 only; not the DSP filter-kind declaration that P6-ENG1 holds), `test_and_telemetry_records.md`.

  No helper script was used.
- **Ids:**
  - DAT-095 is removed and kept as a "(removed: …)" stub, because the generated index cites it.
  - No other id was removed, renumbered or added. No tuning key changed.
  - `map_schema.md` §3.9 is renamed "Routes" and keeps the old anchor through `<a id="39-routes-and-primary-objectives"></a>` (see new request 5).
- **Diagrams:** no Mermaid block changed.
- **Gate** (`python3 -I Tools/DesignCheck/CheckDesign.py design_next --skip-mermaid`, last run after the last edit): gates 1 and 5 to 11 PASS. Every failure is in `decisions/`, which P6-DEC is editing now:
  - Gate 3 FAILs with 24 missing anchors, all in `decisions/README.md`, `decisions/open_questions.md`, `decisions/engine_gap_rules.md` and `decisions/gameplay_gap_rules.md`, where P6-DEC is retitling DEC entries.
  - Gate 2 FAILs with one hit, `decisions/deferred/numerics_lobby_terrain_services.md:72`, renamed term "RR".

  None is in or caused by my files.
  - During the run, gate 5 showed RND-018/019/035 in `renderer/README.md` and `ADR-078` (P6-RND, in progress).
  - Gate 6 showed "Open DEC-267 is not cited", which my R-SIM-322 citation fixes.

### The decision on the score report (P6-RE request 9)

R-MODE-057 step 2 had the result declaration, a simulation rule, "issue the score report". That is a player command issued by the simulation, which R-SIM-454 forbids.

**Decision:** the score report is a **presentation event** of the declaration. It carries the six scores and the declared result, and nothing consumes it (R-META-497, DEC-047). This is consistent with R-SIM-454 and with R-SIM-674 ("the simulation … emits presentation events").

As a result, nothing issues `PlayerCommand::ReportScores` any more. Its row therefore moves from the R-SIM-451 table, where its issuer was "the end of a match", to the R-SIM-452 table. That table holds the commands no input issues and that have a defined, state-free effect when a participant sends one anyway (DEC-202). The command stays in the catalogue, so CONVENTIONS §8.12's `PlayerCommand` list stays valid.

These texts that called the score report a command were aligned:

- reference/01 §5 item 3;
- reference/12 06 §5 item 20;
- reference/09 R-META-105 item 1;
- the "each team's score" row of the reference/09 options file.

### The start hold and the drop rule (P6-RC request 6)

The world-hold rules now let the Supply crate drop rule run while the world is held for the start of the match, from match start to its first world tick, once per command frame (R-CRATE-102, DEC-116). A pause still stops it.

The same exception appears in R-SIM-511 step 1 and R-SIM-218 step 9. Otherwise those two rules would still say that the whole world step is skipped while the world is held.

On the fidelity question that P6-RC passed on ("which other world steps ran during the reference's start hold"), the fidelity row (sample 508) gives the reference's world step as gated only on its paused state. Under DEC-039 (tick scheduling) and DEC-092, Species runs no world tick at game speed 0. So R-SIM-550's first item now states that every other rule of the world step waits for the first world tick, and cites DEC-039 and DEC-092 for the difference (ruling 3). I read no ORIG file.

## Requests applied

| Request (report and number) | Target file | Status | Note |
|---|---|---|---|
| P6-RC 6, R-SIM-550 | reference/12 `01_tick_timing_and_main_loop.md` | applied | Third item extended as given, with "from match start to its first world tick" added to say when the start hold ends. The first item now says every other world-step rule waits for the first world tick (DEC-039, DEC-092), excepting the drop rule. The same exception was added to R-SIM-511 step 1. §8 now lists DEC-116, and DEC-264's default names the start-hold drop rule. |
| P6-RC 6, R-SIM-549 | same file | applied-differently | Inserted as a fourth member of the list ("…, the match-rules update while the world is held (R-SIM-511) and the Supply crate drop rule while it is held for the start (R-CRATE-102) keep …"). Inserting it after the third member with a second "and" would not read. |
| P6-RC 6, R-SIM-012 | reference/12 `02_match_lifecycle_and_state_reset.md` | applied-differently | Added to the first sentence: "The match opens frozen …; while it is frozen for its start, the Supply crate drop rule still runs in its slice of every command frame (R-CRATE-102, DEC-116)". The proposed place, after "although the match is still frozen", sits inside the "when every team is ready" sentence and would imply the drop rule waits for readiness; it does not. §8 lists DEC-116. |
| P6-RC 6, which other world steps run in the start hold | reference/12 01 R-SIM-550 | applied | Answered as described above. Also R-SIM-218 step 9 (`04_containers_and_update_order.md`): during the start hold the drop rule runs alone and the Spawn Surge and Frenzy timers do not count. §8 of 04 lists DEC-116. |
| P6-RC 6, delete the tuning row `ref.sim.bot.controller_spawn_request` | reference/12 `06_teams_and_player_commands.md` | not applied | Already done. P6-RE retired the key in its own round, and no file defines or cites it. |
| P6-RC 6, delete the `endSetTarget` row | same file, R-SIM-439 table | applied | |
| P6-RC 6, R-SIM-455 "step 5" → "step 4" | same file | applied | R-CTRL-160's selection release is step 4 now. |
| P6-RC 7 (R2a-57), R-UNIT-088 | reference/02 `worker.md` | applied-differently | Restated with no objective condition. The draw is kept and stated as part of the simulation stream. No rule of a match acts on it, so no Worker hops and `worker.victory_jump` is never raised (DEC-331). "A map defines no objective" (R-MAP-033) is given only as the reason the hook has no trigger. The hop is kept as a hook so that its four tuning keys stay specified. The §5 edge case ("no Worker ever hops in a match") and AC-UNIT-W23 ("Given any map") are aligned. |
| P6-RC 8 (R2a-57), R-AUD-319 and AC-FEV-10 | reference/10 `front_end_hud_voice_and_ambience.md` | applied | The first sentence also conditioned on objectives, so the whole requirement now reads: `worker.victory_jump` is the sound of a hook that no rule triggers (R-UNIT-088); a map defines no objective (R-MAP-033), so it never plays in a match (DEC-331). AC-FEV-10 reads "Given any map". |
| P6-RC 9 (R2a-57), `map_schema.md` | data_schemas | applied | Changes: <br>• the `primary_objectives` block row is deleted; <br>• DAT-095 becomes "(removed: a map defines no objective, R-MAP-033; a `primary_objectives` block is an unknown block and a build error, DAT-075)"; <br>• the §6 failure row "an objective condition other than `never`" is deleted, and T20's range ends at DAT-094; <br>• the intro no longer names primary objectives; <br>• §3.9 is "Routes", with the old anchor kept for the generated index. |
| P6-RC 9 (R2a-57), `README.md` table 1-A row `maps.definition` | data_schemas | applied | Also the folder-index row of `map_schema.md`. |
| P6-RC 9 (R2a-37), R-TUT-013 | reference/11 `11_tutorial_and_help.md` | applied | "a Supply crate opened, R-CRATE-146;" is removed from the objective-card events. |
| P6-RD 17, table 2.10-A row `turret.fire_grenade` | reference/03 `defences_and_props.md` | applied | R-AUD-136 → R-AUD-145. |
| P6-RD 18 (optional), R-UNIT-008 | reference/02 `README.md` | applied-differently | "at match start" would repeat "Match start …", so it reads "Match start sets the research levels of every team; the match setup carries no research levels." The last sentence pointed to a match-setup side that no longer exists (R-META-070 is removed); it now cites R-META-069. AC-UNIT-12 ("Given any match setup") and the §8 DEC-115 row ("every match starts with the fixed levels") are aligned. |
| P6-RD 19 (optional), R-MODE-082 "research levels" row | reference/01 `README.md` | applied | "\| research levels \| not part of the match setup \| — \| fixed at every match start: … (DEC-115) \|". |
| P6-RE 2(d), R-SIM-322 "(see DEC-NNN)" | reference/12 `05_space_routing_and_physics.md` | applied | P6-DEC has created DEC-267 "Barrier Emitter line in the obstacle grid". R-SIM-322 now cites it, and the §8 table has its row. Its Affects also names R-BLDG-413, so R-BLDG-413 (`defences_and_props.md`) gains one sentence on the dilated band with "see DEC-267", and that file's §8 lists it. This also clears the gate 6 failure "Open DEC-267 is not cited". |
| P6-RE 8 (optional), R-UNIT-394 | reference/02 `transport.md` | applied | "No input of a match issues these commands". |
| P6-RE 8 (optional), R-UNIT-733 | reference/02 `hazards_and_projectiles.md` | applied | "(40 world ticks)". AC-HAZ-17 also reads "40 world ticks". The sequence-diagram label "hold 40 ticks" is unchanged, to avoid a diagram edit; it is not wrong. |
| P6-RE 9 (observation), R-MODE-057 step 2 | reference/01 `README.md` | applied | Decision as above: step 2 reads "emit the score report, a presentation event that carries the six scores and t and that nothing consumes (R-META-497, DEC-047); it is not a player command, because advancing the simulation issues none (R-SIM-454)". The §5 item 3 "issues" becomes "emits". |
| P6-RE 9, consequence for the command catalogue | reference/12 `06_teams_and_player_commands.md` | applied | The `ReportScores` row moves from R-SIM-451 to R-SIM-452. Its effect: none; the score report of a result declaration is a presentation event, not this command. §5 item 20 is restated. P6-RE expected no change to R-SIM-451, but once the command has no issuer it belongs in R-SIM-452. |
| P6-RE 9, consequence for reference/09 | `screen_flow_and_match_setup.md` R-META-105 item 1; `options_localisation_and_hooks.md`, statistics table row "each team's score" | applied | Both now call the score report a presentation event that nothing consumes, citing R-MODE-057 and R-SIM-454 instead of R-SIM-451. |
| P6-RB 4 (its note on R-SIM-622 "Essence orb break particles", addressed to P6-RE) | reference/12 `07_randomness_and_integrity.md` | applied | No rule breaks an Essence orb (R-UNIT-308 removed, DEC-871). Two rows are deleted: the R-SIM-622 row "Essence orb break particles" and the R-SIM-630 Table 1 row "Essence orb: break". VFX-124 itself is P6-RND's (P6-RB request 4). |
| P6-ADR 5(a), DAT-019 | data_schemas `README.md` | applied | The text as given: the parser is hand-written in a project with no generated code, and the validator is generated. |
| P6-ADR 5(b), §2 preamble | same file | applied | "by the Species schema code generator, a tool project of the one solution under `Tools/` beside the content build tool". |
| P6-ADR 5(c), a schema names its project folder | same file, DAT-015 | applied | The request left a choice to the coordinator. I chose the annotation over a table 1-A column. ADR-062 says "each schema names the project folder", and the project layout is not settled yet (`AGENTS.md` §2, BLD-006), so table 1-A could not name the folders now. DAT-015's closed list gains `x-species-project` ("the project folder that owns the record type's generated header, the only folder the generator writes it to; ADR-062"). The coordinator may reverse this. |
| P6-ADR 5(d), ADR-076 row | data_schemas `entity_audio_geometry_and_panel_schemas.md` §8.1 | applied | "Proposed" → "Accepted" (now line 450). |
| P6-ENG2 3, `clients` row and T70 | data_schemas `test_and_telemetry_records.md` | applied | The text as given. A DEC-200 row is added to §8.1 because the row now cites it. |
| P6-ENG2 7, T57 | data_schemas `entity_audio_geometry_and_panel_schemas.md` | applied | The text as given. Build tooling table 4.1-B has no row 14 yet; it exists once the coordinator applies P6-ENG2 request 1(b). |

### Noted, not requests

- P6-RC noted that reference/06 still writes `normalise(` in formulas, which P6-RB request 13 did not cover. The places are `camera.md` R-CTRL-452 and §2 items 8, 1 and 2, `cursor_and_markers.md` R-CTRL-366 and its `icon_offset` tuning row, and reference/12 R-SIM-374. Nobody requested the change, so I left them. If the coordinator wants R4a-59 applied everywhere, it is the same mechanical change in my files.
- P6-DAT's note on `render.worker_scale_factor` depends on a rename that P6-RND has not made. It is new request 4.

## Requests

1. **P6-DEC — `decisions/simulation_gap_rules.md` DEC-264** (follows P6-RC request 6).
   - Specified behaviour: replace "While the world is held at speed 0, three activities keep a real-time cadence of one per `ref.sim.tick_period` of real time: command frames, the speed blend of the tick clock, and the match-rules update, which runs the grace countdown." with "While the world is held at speed 0, these activities keep a real-time cadence of one per `ref.sim.tick_period` of real time: command frames, the speed blend of the tick clock, the match-rules update, which runs the grace countdown, and, while the world is held for the start of a match, the Supply crate drop rule (R-CRATE-102, DEC-116)."
   - Option A: append "and the start-hold drop rule" after "the held-world rules update".
2. **P6-DEC — `decisions/verdicts_and_fixes.md` DEC-116**, Affects. Add `reference/12_simulation_model/02_match_lifecycle_and_state_reset.md (R-SIM-012)` and `reference/12_simulation_model/04_containers_and_update_order.md (R-SIM-218)`. Both now cite it.
3. **P6-DEC — `decisions/deferred/networking.md` DEC-202**, optional. Add `reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM-452: ReportScores)` to Affects, or name `ReportScores` among the commands no input issues, if the entry lists them.
4. **P6-RND — `renderer/options_and_budgets.md`.** If ruling 7 renames `workerScaleFactor` or `rnd.option.worker_scale_factor.default`, tell the coordinator. The data-schemas row "Worker distance scale factor" (`render.worker_scale_factor`, `config_input_and_strings.md` table 3.1-A) and its §9 citation must then follow (P6-DAT note). Also, reference/12 no longer lists an Essence orb break draw (R-SIM-622, R-SIM-630). VFX-124 should therefore be removed as P6-RB request 4 asks.
5. **Coordinator.**
   - Regenerate the requirements index. These first sentences changed: R-SIM-012, R-UNIT-008, R-UNIT-088, R-AUD-319; DAT-095 is a stub.
   - Then delete the line `<a id="39-routes-and-primary-objectives"></a>` in `architecture/components/data_schemas/map_schema.md` §3.9.
   - Delete the DAT-095 stub if only the index cites it.
6. **Coordinator (P6-ADR has finished), optional — `ADR/ADR-062-authoring-syntax-and-schema-language.md` line 34, the Schemas bullet.** After "presence, reference kinds and enumeration order" add "and the project folder that owns the generated header (DAT-015)". That bullet would then match the closed annotation list, which now carries `x-species-project`.
7. **Coordinator:** apply P6-ENG2 request 1(b), build tooling table 4.1-B row 14. Data-schemas T57 now cites it.
