# P6-RC report

## Summary

- **Findings:** 51 (3 High, 6 Medium, 42 Low): **44 fixed**, **7 fixed differently**, **0 rejected**. No finding is left as a request only; 22 fixes need a follow-up change in a file I do not own, listed under **Requests**.
- **Relayed requests applied:** P6-RB requests 12 and 13, P6-RE request 1 (rows at the end of the table).
- **Files edited** (all under `design_next/reference/`): `04_supply_crates_powers_and_weapons/README.md`, `crates_and_powers.md`, `weapons_and_projectiles.md`; `06_controls_camera_and_commands/actions_and_bindings.md`, `camera.md`, `cursor_and_markers.md`, `selection_and_power_slots.md`; `07_bots/README.md`, `02_priority_model.md`, `03_decision_loop_and_leaders.md`, `04_powers_and_unit_policies.md`, `05_mode_strategies_and_tutorials.md`; `08_maps_and_level_design/rules_and_conventions.md`, `map_editor_hooks.md`. Helper scripts: `species_work/scripts/P6-RC_replace_lines.py` (replaces a block only when the file hash matches the hash it was read with) and `species_work/scripts/P6-RC_meh_rewrite.py` (the `map_editor_hooks.md` changes; every replacement must match an exact number of times or nothing is written).
- **Gates:** `python3 -I Tools/DesignCheck/CheckDesign.py design_next` with Mermaid: gate 4 PASS (266 blocks, 0 failed). I changed three diagrams: crates_and_powers figure 3-3, the rules_and_conventions load sequence and map_editor_hooks figure 3.1. The last `--skip-mermaid` run: gates 1–3 and 6–11 PASS. Gate 5 has one failure, `requirements_index/bld.md:25 cites BLD-017`, which comes from another packet's change to the generated index. Earlier runs showed transient failures in `ADR-019` and `platform_layer.md` while their owners were editing them. None of the failures is in or from my files.
- **Requirement ids:** none renumbered or reused. Removed: R-CTRL-375. It stays as a "(removed: …)" stub, because the generated index cites it and links to its section. R-CTRL-176, R-CRATE-146, R-MAP-033, R-MAP-371, R-MAP-373, R-MAP-387 and R-MAP-393 keep their ids with reduced text.
- **Tuning keys:**
  - Removed: `ref.weapon.turret_round.life_by_level`, `ref.ctrl.cursor.selection_boost`, the five `ref.map.editor.camera_animation.*` keys.
  - Split, one key per number: 25 composite keys of reference/06. Each old key is gone and its new keys take a sub-segment of the old name.
  - Moved out of the tuning tables: about 100 `ref.map.editor.*` input-range, panel and drawing keys. They are now informative values: inline in the operation tables, and table 4.5-A.
  - Kept: the editor's operation keys, 22 in all.
- **Assets:** dropped from the §7 lists that no rule draws: `help.hint.supply_crates`, `help.hint.reinforcements`, `icon.power.slots_upgrade`, `icon.power.laser`, `icon.power.grenade`, `icon.power.rocket` (reference/04 README), `icon.hud.slot_discard`, `cursor.select`, `cursor.move_target`, `cursor.enter`, their three shadows and `cursor.pointer`. The manifest is P6-GLO's (request 13).
- **Additional own-file fixes** (same defect class as a finding, in the same files):
  - weapons §8 listed the Open DEC-241 as Resolved; it is now in the Open list.
  - DEC-115, DEC-117 and DEC-096 are removed from §8 lists where no rule applies them any more.
  - `ref.ctrl.camera.dish_aim`, `move_behind`, `ref.ctrl.popup.halo`, `ref.ctrl.bar.compass_alpha` and `ref.ctrl.cursor.ring_widths` are split like their listed twins. `ref.ctrl.camera.shake` stays one key, because VFX-232 cites it; a candidate for P6-RND.

## Findings

| Id | Status | Note |
|---|---|---|
| R1b-1 | fixed | Checked against the reference at 41:104 (it confirms that Supply crates drop during the grace period). The change: <br>• R-CRATE-102 now states the reference: no grace-period gate. While the world is held for the start, the drop rule runs once per command frame, like the match-rules update, so a crate can drop before the grace period ends. Such a crate stays at its start height until the first world tick. A pause stops the rule. <br>• §5 item 1, figure 3-3 and the new AC-C-19 say the same. <br>• §8 lists DEC-116 as Open. <br>• P6-RA has already aligned R-VIS-052 and R-MODE-044. <br>• Requests 1 (DEC-116 re-opened, DEC-036) and 6 (R-SIM-550, R-SIM-549, R-SIM-012). |
| R2a-62 | fixed-differently | R-MAP-392: the parenthesis is deleted. In R-MAP-393 the narration of the reference loader is gone. It keeps the observable behaviour that R-MAP-308 and table 2.16-A cite: reading restores the map text and registers the embedded images, and a text map under the container's extension fails to load. Then follows the Species rule as proposed. |
| R2a-61 | fixed | Table 2.16-A rows 1, 9, 12 and 13 are deleted. R-MAP-317 and R-MAP-362 carried no code-level wording beyond those rows. R-MAP-371 is reduced to the observable rule (R2a-64). The table is renumbered (13 rows), and every item citation is updated (R-MAP-305, R-MAP-311, R-MAP-356, §8.1). |
| R4b-51 | fixed | Table 2.2-A row 4: "a numeric Transport cap of 1 or more". |
| R4b-59 | fixed | The R-CRATE-100 row reads "none, or 10 to 300 s …; none means no Supply crates (R-MODE-074)". The pseudocode reads `if D is none`, and figure 3-3 reads "Drop period none?". |
| R2a-55 | fixed | R-BOT-150's last sentence is replaced as proposed, and DEC-123 is removed from 05. |
| R2a-63 | fixed-differently | Deleted: the `null` sentence of R-MAP-385, AC-MEH-16, §5 items 12 and 13, and the byte-level text of R-MAP-387. R-MAP-387 keeps its observable consequences: a round trip moves structures, changes the terrain and the content identity, and a 0-valued map option reads back as absent. DEC-204, §5 item 1 and the terrain README rely on those. It ends with "Every map tool writes by DEC-800 (R-MAP-388)". |
| R2a-64 | fixed-differently | A reduction to data, writers, validation and §8.1 would drop what DEC-045 itself names as the hooks: the entry and exit flow, and what the editor edits. It would also orphan about 30 citations from the terrain, camera, UI, build and schema documents. Instead: <br>• Deleted: the declared-but-unused and never-opened items: the finer selection, the create tool, the "existence test" and the panels present but never opened. The camera-animation panels become R-MAP-371/373 "no camera-animation authoring". <br>• Moved out of the tuning tables: every input range, now inline in the operation tables marked "informative". <br>• Moved out of the tuning tables: every panel size and drawing value, now the informative table 4.5-A, one value per row with its unit. <br>• R-MAP-300 and §1.1 say these values are informative. <br>• 22 operation keys stay (new-tile defaults, scale factors, brush, rotation, clone variance and similar). |
| R2a-66 | fixed | The sentence is deleted; §8.1 item 11 is "The editor's drawing style (R-MAP-380, table 4.5-A)". |
| R2a-35 | fixed | R-CRATE-009 is replaced as proposed. |
| R2a-36 | fixed | R-CRATE-014: "The capacity is fixed for the whole match (DEC-033)." The key note is "per team". The two icons are dropped from §7, and DEC-115 from README §8. Request 13. |
| R3a-32 | fixed | `Interval(D) := ceil(D / ref.sim.tick_period * …)`; the key is added to the §4.6 cited-keys row. |
| R3a-50 | fixed | "mod (the number of entries of `ref.power.comeback.pool`, 6 in the reference)". |
| R2a-37 | fixed | R-CRATE-146 is replaced as proposed, and the pseudocode line is deleted. `help.hint.supply_crates` (and `help.hint.reinforcements`, which no rule here draws) are dropped from §7. Requests 9 and 13. |
| R4b-13 | fixed | "nominal 15 s" in the row and in R-CRATE-191. |
| R4b-14 | fixed | "nominal 15 s" in the row and in R-CRATE-213. |
| R3a-54 | fixed | Unit "pop-timer units (presentation state)"; the note says that it starts the bubble pop on landing and falls per presentation step. |
| R4b-15 | fixed | "nominal 5 s" in the row and in R-CRATE-210; reference/05 keeps its derived 5.1. |
| R3a-72 | fixed | DEC-241 and DEC-565 are added to §8. |
| R2a-38 | fixed | R-CRATE-450 is replaced as proposed; `ref.weapon.turret_round.life_by_level` is deleted. |
| R3a-33 | fixed | A = ⌈s ÷ `ref.sim.tick_period`⌉ − 1; the key is added to §4.5. Request 3 for the same formula in DEC-859. |
| R2a-39 | fixed | R-CRATE-489 drops the research-level condition. |
| R4b-18 | fixed | "nominal 1.2 s". A replay of the binary64 countdown (subtract 0.1 per tick, stop at or below 0) gives 13 ticks for 1.2 s and reproduces every pair of DEC-241 item 3. The row cites DEC-241 item 3. Request 2 adds the pair. |
| R4b-19 | fixed | "nominal 10 s" in the row and in R-CRATE-485. |
| R4b-25 | fixed-differently | The key is deleted under R2a-38, so no projectile life is left in seconds. |
| R2a-43 | fixed | `action.camera.zoom_out` and row A11 are deleted; R-CTRL-460 no longer cites row A11. Request 10. |
| R2a-44 | fixed | Both action ids and row T8 are deleted, and R-CTRL-225 no longer names row T8. Request 10. |
| R2a-45 | fixed | The paragraph after R-CTRL-104 is deleted, with the `endSetTarget` field of R-CTRL-101, which no other rule reads. Also deleted: its only source `action.direct.set_target_end`, and the same move marker in R-CTRL-160 (step 4 deleted; step 5 is now 4). Requests 4, 6 and 10. |
| R2a-50 | fixed | "Camera modes other than those of table 2.2-A are not specified (DEC-024)." |
| R3a-47 | fixed | Split: `initial_pose` (5 keys), `zoom_rates` (4), `cliff_probe` (3), `turret_aim` (4); also `dish_aim` (3) and `move_behind` (3). The rules cite the new keys. Request 11 (CAM-049 and the key list cite `zoom_rates`). |
| R2a-49 | fixed | Deleted: the three rows, the boost key and the three shadows in R-CTRL-350. The seven assets are dropped from §7, and `cursor.select` from reference/04 README §7. R-CTRL-375 becomes a removed stub (cited by the generated index). Requests 12 and 13. |
| R3a-46 | fixed | Split: `cursor.set` (12 keys), `count_text` (4), `area_cylinder` (5), `type_constants` (11), `alpha_rules` (8), `size_rules` (11); also `ring_widths` (2). R-CTRL-350, R-CTRL-363, R-CTRL-366, R-CTRL-381, R-CTRL-384 and R-CTRL-385 cite them. Request 12 (RND-367 cites `ref.ctrl.cursor.set`). |
| R2a-46 | fixed | Table 2.7-A keeps the kinds Leader to Fury without Sickness. That is 18 kinds, not 19: the proposal counted one too many. They stay in the reference relative order, renumbered 1 to 18. Also changed: <br>• R-CTRL-175 points to reference/12 table 2.4-B for the power levels. <br>• R-CTRL-176 keeps only "a request for a kind that the table does not list fails" and DEC-552. <br>• Sickness is removed from R-CTRL-180 item 3 and cursor row 1c. <br>• Request 13 (manifest consumers of `icon.power.grenade` and `icon.power.rocket`). |
| R2a-47 | fixed | The three messages are deleted from R-CTRL-221 and from the string table; the "other messages carry their sender's duration" clause goes with them. They are also deleted from the reference/04 README §4.2 list. Request 13 (CONVENTIONS §8.10). |
| R2a-48 | fixed | The sentence is deleted, and `icon.hud.slot_discard` is dropped from §7 here and in reference/04 README §7. Requests 11 (UI-462) and 13. |
| R3a-45 | fixed | All ten keys are split, plus `popup.halo` and `bar.compass_alpha`; each new key has its own unit. R-CTRL-222, R-CTRL-231, R-CTRL-232, R-CTRL-233, R-CTRL-234, R-CTRL-236 and R-CTRL-238 cite them. |
| R4a-60 | fixed | `dishNeighbor` (three places). |
| R2a-53 | fixed | As proposed. |
| R2a-54 | fixed | The clause and its DEC-087 citation are deleted. |
| R3a-61 | fixed | As proposed. |
| R3a-73 | fixed-differently | DEC-051 is added to 05 §8. DEC-123 is not: R2a-55 removes its only citation from this file. |
| R2a-51 | fixed | The README sentence is deleted, and R-BOT-008 reads "placed inside the map (DEC-087; see DEC-862)". Request 6 (the reference/12 key row). |
| R2a-52 | fixed | §5 item 8 is deleted. |
| R2a-65 | fixed | Deleted: every campaign mention (R-MAP-306, R-MAP-307, the campaign-flag row, the last sentence of R-MAP-348, R-MAP-385, R-MAP-386, table rows 10, 11 and 15, §5 item 11, figure 3.1). §1 gains "Campaign content is not specified (DEC-052)." The §8 Dropped line now reads "(campaign content is not specified)". |
| R4a-66 | fixed | "Firebomb Run" in R-MAP-376 (twice), §5 item 5 and AC-MEH-13. |
| R3a-48 | fixed-differently | The eight composite keys are panel and drawing values. Under R2a-64 they moved out of the tuning table into the informative table 4.5-A, where each number is its own row with a unit. |
| R2a-56 | fixed | R-MAP-032 is replaced as proposed. |
| R2a-57 | fixed | The primary-objectives row is dropped from table 2.4-A, and R-MAP-033 is replaced as proposed. The load sequence diagram no longer adds a never-true objective. AC-MAP-08 now tests that an unlisted block is rejected. DEC-331 is removed from the §8 Resolved list. R-MAP-303 and R-MAP-386 of the editor hooks no longer name objectives. Requests 5, 7, 8 and 9 (worker.md, R-AUD-319, map schema, DEC-331). |
| R2a-58 | fixed-differently | Every figure over the 57 files is gone: <br>• Table 2.8-B is kept as an envelope of the match maps, because each range still bounds them. Its file counts are dropped, and R-MAP-085 cites DEC-247 and DEC-334 once. <br>• Old table 2.11-A is deleted. The 49-map table 2.11-C becomes table 2.11-A, so that DAT-161's citation stays valid; its "see table 2.11-A" cells read "not specified". <br>• The 57-file column of table 2.11-B is gone, as are the 57-file clauses of R-MAP-086, R-MAP-109, R-MAP-111 items 3 and 5, R-MAP-128 and R-MAP-200. |
| R2a-60 | fixed | "see DEC-123" and the §8 line are deleted. DEC-123 stays cited by R-VIS-174 (gate 6 passes; request 3 notes it). |
| R2a-59 | fixed | Note: "maximum camera-view name length; the schema enforces it (DEC-801)". |
| request from P6-RB 12 | fixed | `crates_and_powers.md`: the blight delay "(101 ticks, nominal 10 s)". The packet named R-CRATE-206; the text is in the blighted-tree rule. |
| request from P6-RB 13 | fixed | Every `normalise(` → `Normalize(` in `crates_and_powers.md` (4), `weapons_and_projectiles.md` (6) and reference/07 files 03, 04 and 05 (4). reference/06 still writes `normalise(` in two formulas (R-CTRL-452, R-CTRL-366), which were outside the request. |
| request from P6-RE 1 | fixed | R-CRATE-226, R-CRATE-228, the §5.1 matrix row, AC-C-17 and R-CRATE-203 now state 151 world ticks (40 world ticks for the meteor), "5 % of the gap per command frame", 37.75 s to 75.5 s, and 7.55 s to about 11.6 s. Nothing else in reference/04 names the hold. |

## Requests

1. **P6-DEC — `decisions/verdicts_and_fixes.md` DEC-116** (R1b-1). Rewrite the entry:
   - Status: Open.
   - Affects: add `reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM)` and `reference/00_reference_overview.md (R-VIS)`.
   - Specified behaviour: "The Supply crate drop rule has no grace-period gate. While the world is held for the start of a match it still runs, once per command frame, so its countdown runs while the teams ready up and during the grace countdown, and a Supply crate can drop before the grace period is over; such a crate stays at its start height until the first world tick. A pause stops the drop rule (R-CRATE-102, R-MODE-044, R-VIS-052, R-SIM-550)."
   - Options: "A — the reference: the drop rule runs during the start hold; B — a grace-period gate: the drop rule runs only on world ticks, so no Supply crate exists when the match starts."
   - Recommendation: "B. Every other per-tick rule waits for the first world tick (DEC-039, DEC-092), and under A a long ready-up lets crates accumulate up to the map's cap before anyone can move."
   - Decide by: playtest.
   - Set its README index row to Open.
   - In DEC-036, replace "No world tick runs during the grace period, so no Supply crate drops in it (DEC-116)." with "No world tick runs during the grace period; the Supply crate drop rule still runs, so a Supply crate can drop before the match runs (DEC-116)."
2. **P6-DEC — `decisions/deferred/numerics_lobby_terrain_services.md` DEC-241 item 3** (R4b-18). Add "1.2 → 13" to the list of counts. A binary64 replay of "subtract 0.1 per tick, stop at or below 0" gives 13, and the same replay reproduces every pair already listed.
3. **P6-DEC — other entries:**
   - DEC-859 item 3: "A = ⌈10 × s⌉ − 1" → "A = ⌈s ÷ `ref.sim.tick_period`⌉ − 1" (R3a-33).
   - DEC-123 Affects: remove R-MAP-158 and R-BOT-150 (R2a-55, R2a-60). R-VIS-174 is now its only citing requirement; if P6-RA removes that one too, gate 6 fails for DEC-123.
4. **P6-DEC — DEC-096** (R2a-45). Delete "the move-marker list is bounded;" from the specified behaviour; move markers are no longer specified.
5. **P6-DEC — `decisions/open_questions.md` DEC-331** (R2a-57). Replace "A map with no primary objectives receives one objective that never completes, and the victory celebration of idle Workers starts only when every objective is complete, so it never starts on such a map." with "A map defines no objective (R-MAP-033), so the victory celebration of idle Workers never starts in a match." Remove `reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)` from Affects.
6. **P6-RE — `reference/12_simulation_model/`** (R1b-1, R2a-45, R2a-51). Through the coordinator, since P6-RE has finished.
   - `01_tick_timing_and_main_loop.md` R-SIM-550, third bullet: append "; while the world is held for the start of the match (R-SIM-551, R-SIM-552), the Supply crate drop rule also runs, in slice `ref.sim.slice.supply_crates` ([reference/04](../04_supply_crates_powers_and_weapons/crates_and_powers.md) R-CRATE-102, DEC-116)".
   - R-SIM-549: after "the match-rules update while the world is held (R-SIM-511)" insert "and the Supply crate drop rule while it is held for the start (R-CRATE-102)".
   - `02_match_lifecycle_and_state_reset.md` R-SIM-012: after "although the match is still frozen" insert "; the Supply crate drop rule also runs while the match is frozen for its start ([reference/04](../04_supply_crates_powers_and_weapons/crates_and_powers.md) R-CRATE-102, DEC-116)".
   - The fidelity reviewer also asked which other world steps ran during the reference's start hold. The reference gates the world step only on its paused state. I could not check the reference beyond the ±20 lines of 41:104, so this is for whoever holds the R-SIM fidelity rows.
   - `06_teams_and_player_commands.md`: delete the tuning row `ref.sim.bot.controller_spawn_request`, and the `endSetTarget` row of the team command state table.
   - R-SIM-455: "R-CTRL-160 step 5" → "R-CTRL-160 step 4".
7. **P6-RB — `reference/02_units/worker.md` R-UNIT-088** (R2a-57). Replace "and every primary objective of the map is complete, it hops: …" and the closing sentence with a rule that does not depend on map objectives. For example: "A match never meets the celebration condition, because a map defines no objective (R-MAP-033), so the Worker never hops; each Idle Worker of team slot 0 still makes the draw at every retarget (DEC-331)". Keep the draw if the simulation stream must stay as stated.
8. **P6-RD — `reference/10_audio_design/front_end_hud_voice_and_ambience.md`** (R2a-57). R-AUD-319: replace the two sentences about primary objectives with "A map defines no objective (08 rules and conventions R-MAP-033), so `worker.victory_jump` never plays in a match (DEC-331)". AC-FEV-10: "Given any map, then no Worker plays `worker.victory_jump` during a match."
9. **P6-DAT — `architecture/components/data_schemas/`** (R2a-57):
   - `map_schema.md`: delete DAT-095 (or reduce it to "(removed: a map defines no objective, R-MAP-033)" if something cites it); delete the `primary_objectives` row of the block table (line 226); retitle §3.9 "Routes" (keep its anchor in mind).
   - `README.md` table 1-A row `maps.definition`: drop "primary objectives".
   - **P6-RA — `reference/11_tutorial_and_help.md` R-TUT-013** (R2a-37): drop "a Supply crate opened, R-CRATE-146;" from its list of events that feed the objective cards; R-CRATE-146 now states that Supply crates raise no hint card in a match.
10. **P6-ENG1 — `architecture/components/input/`** (R2a-43, R2a-44, R2a-45). Delete the actions `action.camera.zoom_out`, `action.power.panel_show`, `action.power.panel_hide` and `action.direct.set_target_end`:
    - `controls_and_default_bindings.md`: rows 13, 41, 42 and 57, and their entries in the per-control table (`key:c`, `key:tab`, `pad:face_east`, `pad:view`, `mouse:secondary`, `pad:face_west`);
    - `binding_layers_and_rebinding.md`: line 52 and the INP-179 list;
    - `README.md`: T28, if it names them.
    - In `input/README.md` failure table row 22 ("an unbounded move-marker list"), delete the row: move markers are no longer specified.
11. **P6-ENG1 — other files:**
    - `camera_and_sequences.md` CAM-049: "runs at rate 1 of `ref.ctrl.camera.zoom_rates`" → "runs at the rate `ref.ctrl.camera.zoom_rates.scripted` (1)" (R3a-47). In the cited-keys paragraph (line 647), `ref.ctrl.camera.zoom_rates` → `ref.ctrl.camera.zoom_rates.scripted`.
    - §8.2 item 2: replace with "the reference editor offers no camera-animation authoring (R-MAP-371, R-MAP-373)" (R2a-64).
    - CAM-071: drop "and the map editor's preview and stop operations (R-MAP-371)".
    - `ui_framework/pages_and_screen_recipes.md` UI-462: delete "; the discard button `icon.hud.slot_discard` belongs to the overlay screen that a match never shows (R-CTRL-231)" (R2a-48).
12. **P6-RND — renderer and terrain:**
    - `renderer/entity_and_overlay_renderers.md` RND-367: "(`ref.ctrl.cursor.set`)" → "(`ref.ctrl.cursor.set.world_size`)" (R3a-46).
    - `renderer/visual_language.md` RND-422: delete `cursor.select.shadow`, `cursor.move_target.shadow` and `cursor.enter.shadow` from the list (R2a-49).
    - `terrain/README.md` line 501, the "Writer precision" bullet: replace it with "The reference editor's writer is lossy (R-MAP-387); every map tool writes by DEC-800 (R-MAP-388)". The byte-level precision list describes the reference writer, which R2a-63 removes from R-MAP-387.
    - Candidate, not required: split `ref.ctrl.camera.shake` (VFX-232 cites it) only if reference/06 is asked to.
13. **P6-GLO — `assets/` and `CONVENTIONS.md`:**
    - `asset_manifest.csv` and `GameAssets.md`, remove the rows that no rule draws now: `cursor.select`, `cursor.move_target`, `cursor.enter`, `cursor.select.shadow`, `cursor.move_target.shadow`, `cursor.enter.shadow`, `icon.power.laser`, `icon.power.slots_upgrade` and `icon.hud.slot_discard` (R2a-36, R2a-48, R2a-49).
    - In the same files, update consumers:
      - `cursor.pointer`: drop R-CTRL-375; UI-076 and UI-465 remain.
      - `icon.power.grenade` and `icon.power.rocket`: drop R-CTRL-191, R-CTRL-231, R-CTRL-232, R-CTRL-366, R-CTRL-381, RND-378 and reference/04 README, because grenade and rocket are no longer Power kinds. strike_team.md still names `icon.power.grenade`; P6-RB decides whether any rule draws it.
      - `help.hint.supply_crates`: drop R-CRATE-146.
      - `cursor.highlight` and `cursor.place`: no change.
    - `CONVENTIONS.md` §8.10: delete the rows "Improved" (`ui.power_slots.improved`), "Objectives complete" (`ui.match.objectives_complete`) and "Unlocked" (`ui.power_slots.unlocked`) (R2a-47).
    - `glossary/m_to_z.md`, the Supply crate entry: "(see DEC-116)" is attached to the expiry sentence, which DEC-116 does not decide. Repoint it to DEC-859, or drop it.
