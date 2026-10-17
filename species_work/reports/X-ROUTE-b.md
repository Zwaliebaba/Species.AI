# X-ROUTE-b report: cross-document request routing (components, glossary, CONVENTIONS)

## 1 Files written

| File | Size | What |
|---|---|---|
| `species_work/p4_routes_X-ROUTE-b.csv` | 163,708 bytes | 328 routes, RB-001 to RB-328 |
| `species_work/scripts/X-ROUTE-b_append.py` | 2,015 bytes | appends rows, assigns route ids, validates kind and status |
| `species_work/scripts/X-ROUTE-b_edit.py` | 1,371 bytes | replaces fields of existing rows by route id |
| `species_work/scripts/X-ROUTE-b_glossary_check.py` | 1,152 bytes | lists which terms already have a glossary entry |
| `species_work/scripts/X-ROUTE-b_ledger_check.py` | 1,480 bytes | lists which proposed names the ledger or CONVENTIONS already holds |

Nothing in `design_next/` was edited. No git write, publishing or upload tool was used.

## 2 Method and conventions of the CSV

- **Inputs:** the "Cross-document requests" sections of C-SIM-a, C-SIM-b, C-SIM-c, C-TER, C-RND-a, C-RND-b, C-AST, C-VFX, C-AEN, C-UI-a, C-UI-b, C-INP, C-CAM, C-DAT (including its two follow-up sections), C-PLT, C-BLD, C-DBG, X-AST (§7 "Open questions and requests", items 1 to 10), X-GLO-1, X-GLO-2; the "Names needed that are not in the ledger" sections of the same reports (as `ledger` rows, because items 109 and 114 route them); and the items of `phase4_dec_fixes.md` whose target is a component, the glossary or CONVENTIONS.
- **`source`:** `<report>.<n>` is the n-th request of the report's section; where the report groups requests by target I used the target as tag (`C-VFX.G03a-1`, `C-AEN.G09b-2`, `C-DAT.PLT-2`, `C-SIM-c.T14` = row 14 of C-SIM-c's table). A matching `phase4_dec_fixes.md` item is added after a semicolon (`C-RND-b.1d; item 65`). Item-only rows start with `item`.
- **`to_package`:** the owner from `packages/*.json`, except where Phase 4 moved ownership: decision files go to **X-DEC-P4a / X-DEC-P4b** and ADRs to **X-ADR-P4** (the Phase 4 brief owners), the glossary to **X-GLO**, the asset files to **X-AST**, names to **ledger**, and gates, CONVENTIONS, working files and file splits to **coordinator**.
- **`status`:** `done` only where I found the requested id, key or statement in the target (the note names the evidence); "applied live" requests between C-BLD, C-DBG, C-PLT and C-DAT are marked done with the requirement ids that hold them. `superseded` where a later decision removed the need (2 rows). Everything else is `open`.
- **Arbitration and ADR results folded in:** X-ARB (`p4_arbitration.md`) already holds verdicts for several routed conflicts; the rows carry its replacement text and cite the entry: A-15 (RB-292, item 62), A-16 (RB-051), A-17 (RB-173), A-18 (RB-208), A-19 (RB-221), A-20 (RB-224), A-22 (RB-174), A-24 (RB-056). X-ADR-P4's component fixes are matched: F5 (RB-098), F6 (RB-102), F8 (RB-189); F1 to F4 and F7 are RB-296.

## 3 Outcome per input

| Input | Rows | Done | Open | Notes |
|---|---:|---:|---:|---|
| C-SIM-a | 22 + 1 ledger | 15 | 8 | 07 R-SIM-612 and R-SIM-730 (item 17), 05 R-SIM-368 and six movement constants, 06 R-SIM-422 formula, G10 kill-milestone event |
| C-SIM-b | 9 + 1 ledger | 7 | 3 | 04 to cite R-SIM-308; X-AST string ids (prefix mismatch `team.colour.*`/`faction.*` against GameAssets' `match.team.*`) |
| C-SIM-c | 19 + 1 ledger | 14 | 6 | R-SIM-503 key name (item 13); Crawler texture mismatch (RB-044); formation spacing 4.0 against 6 wu (RB-048); three crowd-order values not found (RB-047); global speed-literal sweep |
| C-TER | 9 + 1 ledger | 1 | 9 | R-MAP-081 (A-16), R-MAP-387 (A-24), RND-134/144/145, R-SIM-387, DBG terrain diagnostics, DEC-246 wording |
| C-RND-a | 18 + 1 ledger | 12 | 7 | sea border rule for G08a; version string for G09a; match message time for G01; hint-overlay lines and talk hint have no owner (RB-070) |
| C-RND-b | 20 + 1 ledger | 4 | 17 | RND-070/072/096/055/144/263; five same-value key pairs (RB-081); R-UNIT-539, R-UNIT-147, R-UNIT-050, R-BLDG-048, R-CRATE-497/445, two G06 marker facts |
| C-AST | 19 + 1 ledger | 8 | 12 | RND-162/406 (item 83), blur key name, RND-422, RND-393; AST citations from terrain and reference/12; X-AST corrections; glossary 'RGBA8 image' |
| C-VFX | 23 + 1 ledger | 1 | 23 | link paths in decisions and manifest; RND-082/270/306/308; eight reference-document fixes; placeholder neighbour types |
| C-AEN | 17 + 1 ledger | 4 | 14 | four G10 wording fixes; four G09b fixes; AST cooking rules; R-SIM-581 audio step; 14 glossary terms |
| C-UI-a | 15 + 1 ledger | 5 | 11 | `EditCommand` mapping; R-CTRL-035 link; R-META-208 link; G09b key citations (item 94); `ui.` key and string-id overlap rule |
| C-UI-b | 14 + 1 ledger | 4 | 11 | item 98 set (R-META-080, R-META-044 per A-17, R-META-406 per A-22, R-CTRL-223); UI-037; DEC-102 and DEC-801 Affects; manifest consumers |
| C-INP | 15 + 1 ledger | 2 | 14 | zoom-out binding; one binding owner (items 96, 100); UI-216 real time; selection lock owner; action-id pattern in CONVENTIONS |
| C-CAM | 16 + 1 ledger | 1 | 16 | R-CTRL-457/460, R-META-210/007, R-MAP-125/127/309/371, R-SIM-581 (A-18), R-SIM-014 step 9, developer camera actions |
| C-DAT | 22 + 1 ledger | 5 | 18 | stale PLT citations (item 120), G08a display-name split, Barrier Emitter `link`, G07 pickup names, INP-163, `DspFilterDef` (item 119), DBG soak and telemetry gaps |
| C-PLT | 12 + 1 ledger | 4 | 9 | INP-100 step 4, `WindowMode` duplicate, BLD-103 and BLD-113, R-META-423/424 keys, AST-105 option name, five `ui.error.*` ids and the app icon (item 117) |
| C-BLD | 10 + 1 ledger | 3 | 8 | AST-057 'offered languages', BLD-082/061/053/072/074 citations, `ui.repair.*` ids, 15 glossary terms (item 109) |
| C-DBG | 8 + 1 ledger | 2 | 7 | M5 'opens', development binding layer, cheats-used hook, R-SIM-740 key, marker-name collisions (item 114) |
| X-AST | 10 | 8 | 2 | role confirmations for G04 (`icon.power.laser`, `icon.power.slots_upgrade`) and G03a (barrel, alternate meshes, nav-node visibility) |
| X-GLO-1, X-GLO-2 | 10 + 1 ledger | 6 | 5 | missing A to L entries, 'dt' wording, G05 'starting units' check, the `orig-string` list false positives |
| Items | 21 | 8 | 11 (+2 superseded) | see §4 |
| Names-section string ids | 1 | 0 | 1 | RB-326 (to X-AST) |

Information-only bullets (answers, "already done" notes, "no change") were not routed: C-SIM-a cross-document notes on its own key removals; C-SIM-c table rows marked "present" and its "other requests" note on 04; C-RND-b 1c2 and 1g are routed as one dedup row; C-UI-a 3 (answers to G06), 4 (page grid, options-screen title), 6 (agreements), 12 (constants list); C-UI-b 5c and 13; C-INP "answered for others"; C-CAM C-VFX, G11 and C-RND-a items (the last folded into RB-077); C-DAT notes to C-CAM, C-VFX and C-UI; C-PLT 7, 9 and 10 (DEC-225 already names platform_layer.md); C-DBG 7; X-GLO-1 3.

## 4 Outcome per phase4_dec_fixes item (component, glossary and CONVENTIONS targets)

| Item | Outcome |
|---|---|
| 13 | RB-032 open (R-SIM-503 to cite `ref.mode.mode_second`; reference side is X-ROUTE-a's) |
| 17 | RB-001, RB-002 open (07 R-SIM-612 order, R-SIM-730 counter readers) |
| 19 | RB-289 done (C-DAT cites DEC-120 ten times; `excluded_outcomes` field exists) |
| 32, 36, 55, 56, 79, 90 | glossary links done (RB-290); plain-text paths remain in ten decision files (RB-117 X-DEC-P4a, RB-118 X-DEC-P4b), in manifest consumers (RB-119) and in `dec_map.csv` (RB-327) |
| 59 | RB-291 open (glossary 'population cap' still divides by the player count; DEC-079 divides by open slots) |
| 62 | RB-292 open, with X-ARB A-15 text (RND-229: two Zone Control maps and one Spawn Control map) |
| 65 | RB-082 open (RND-096 still linear) |
| 69 | RB-293 done for the component side |
| 76 | RB-294 open; **the proposed path `tools/` + `reference-models/` fails forbidden pattern line 147**: use `tools/design-models/terrain_generator.py` |
| 77 | RB-051 open, with X-ARB A-16 text |
| 83 | RB-098 open (renderer side; X-ADR-P4 F5 wording) |
| 84 | RB-295 superseded by the ADR-0018 amendment; RB-296 open (AST-021, sampler, AST-027, RND-262: X-ADR-P4 F1 to F4, F7) |
| 86 | the 15 C-AST requests are RB-098 to RB-116 |
| 89 | done inside RB-144 (DAT-271) |
| 91 | RB-328 done (CAM-012); reference side RB-130 open |
| 94 | RB-157, RB-158 done; RB-164 open (G09b key citations) |
| 96, 100 | RB-188, RB-189 open (one binding owner; UI-216 real time); RB-297 done (leaderboard actions) |
| 97 | RB-199 open (three glyphs for X-AST) |
| 98 | RB-172 to RB-175 open (A-17 and A-22 texts included) |
| 101 | RB-201 open (G06 side; the DEC amendments are X-DEC's) |
| 103 | RB-208 (A-18 text), RB-210, RB-211, RB-320 open |
| 104 | RB-238, RB-298 done |
| 106 | RB-299 open: **seven of the nine "uncited" keys are a gate defect**: gate 8 splits table rows on the escaped `\|` of absolute values, so it reads the wrong cell as the requirement column; the two `ref.bot.marker.*` keys cite "table 2.1-B" (G07, X-ROUTE-a). RB-300 open (DEC-208 needs a DBG requirement) |
| 107 | RB-301 done |
| 109 | RB-251 to RB-260 and RB-323 (3 done, 8 open) |
| 110 | RB-302 done |
| 113 | RB-303 open (split not yet made; six rows wait for it) |
| 114 | RB-262 done; RB-263, RB-264, RB-265, RB-267, RB-268 and RB-324 open (RB-266, the R-SIM-740 key, is C-DBG request 6) |
| 117 | RB-241 to RB-244, RB-246, RB-247, RB-249 and RB-304 open; RB-245 and RB-248 done (applied live); RB-305 superseded (same as RB-250) |
| 118 | RB-221, RB-224 (A-19, A-20 texts), RB-306 open (C-DAT to cite the item-118 DEC) |
| 119 | RB-233, RB-307 open (AST and AEN to cite DAT-273) |
| 120 | RB-218, RB-229, RB-237 open |

Items whose target is a reference document (7, 8, 18, 26, 28, 30, 33, 45, 53, 57, 60, 67, 68, 73, 74, 81, 105 and the reference halves of 59, 94, 98, 109, 117, 118) are X-ROUTE-a's; DEC, ADR, asset and arbitration items are the other Phase 4 agents'. Where one of my report rows touches such an item, the note says so.

## 5 Findings for the coordinator

1. **File sizes.** Open rows target files at the 115 KB edit threshold of the brief: `diagnostics_test_and_performance.md` (121,722 bytes; must be split first, item 113; six rows wait), `decisions/verdicts_and_fixes.md` (119,328 bytes while X-DEC-P4a edits it), `terrain.md` (117,221), `audio_engine.md` (117,057), `screen_flow_and_match_setup.md` (115,805), `input.md` (115,440). Each affected open row says so in its note. Note that the gate measures 120 KiB (122,880 bytes) while RUN_LOG figures are decimal KB.
2. **Same-value duplicate keys that gate 8 cannot see** (different names, same reference value): RB-081 (five renderer keys against reference/03 and reference/06), RB-101 (the blur value 10 keyed four times), RB-165 (renderer chat keys against `ref.meta.chat.scale_*`), RB-037 (`ref.sim.query.visibility_offset` and `ter.query.los_margin`), RB-077 (`rnd.hud.message.*` against `ref.ctrl.panel.message_*`).
3. **Dotted namespace collisions** (RB-268, RB-171, RB-321): the profiling-marker roots `camera.`, `render.`, `audio.` are also setting-key roots, `camera.` is also the string-id prefix `camera.view` and the schema root `camera.timeline`; UI tuning keys and UI string ids share `ui.`. No collision with an asset stem.
4. **Competing names for the ledger:** `MapEffect::OrbRain` (C-SIM-c) against `MapEffect::EssenceOrbRain` (X-GLO-2); `ControlHints` (C-SIM-a) against `ControlHintOverlay` (C-SIM-c); two identical `WindowMode` enumerations (renderer and platform, RB-242). CONVENTIONS §8.11 still lists the pattern `NET_GAME_*` for player command ids (RB-309 note).
5. **Wording to check:** RND-243 calls the Missile Barrage launcher sprite 'submarine', a design expression the ledger marks for renaming (RB-064 note).
6. **No owner** for the hint overlay's line widths and the chat talk hint (RB-070).
7. **Conditional rows** (wait for a DEC): RB-083 (centipede normals), RB-124 (flicker cadence), RB-132 (explainer relayout), RB-137 (reduced-flash and reduced-motion settings), RB-211 and RB-214 (timelines, DEC-051), RB-306 (item 118).

## 6 Limits

- "Done" is decided by finding the id, key or statement with grep in the current target; the row marked "spot-checked" in its note (RB-046) verified a representative subset of a long value list.
- The glossary and ledger checks are exact-term matches (scripts above); a term written differently in an entry would show as missing.
- The other Phase 4 agents run in parallel; rows that depend on their output (X-DEC-P4a/b DECs, X-AST-2a rows) are open and say what they wait for.

## 7 Checks

- `python3 -I species_work/scripts/scan_patterns.py species_work/forbidden_patterns.tsv` on the CSV, this report and the four scripts: no output.
- The same with `soft_patterns.tsv`: no output.
- CSV: 328 rows, unique ids, no empty cell, kinds and statuses from the allowed sets (212 open, 114 done, 2 superseded).
