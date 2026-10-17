# X-DEC-P4b report: decision triage, simulation and engine

Brief: `species_work/phase4_brief.md` §1, §2 and "X-DEC-P4b". Inputs: the "New DEC requests" and DEC remarks of C-SIM-a, C-SIM-b, C-SIM-c, C-TER, C-RND-a, C-RND-b, C-AST, C-VFX, C-AEN, C-UI-a, C-UI-b, C-INP, C-CAM, C-DAT, C-PLT, C-BLD and C-DBG; the `phase4_dec_fixes.md` items whose DEC lives in my files or that are simulation or engine rules; X-ARB's `p4_arbitration.md` (A-1 to A-24, final); X-DEC-P4a's hand-off. No git write, no publishing or upload tool used.

## 1 Files written

| File | Size (bytes) | What |
|---|---|---|
| `design_next/decisions/simulation_gap_rules.md` | 15 198 | **new**, "Gap rules: simulation", DEC-260 to DEC-266 (3 Resolved, 4 Open) |
| `design_next/decisions/engine_gap_rules.md` | 42 603 | **new**, "Gap rules: engine", DEC-950 to DEC-972 (10 Resolved, 13 Open) |
| `design_next/decisions/simulation_terrain_render_assets.md` | 16 896 | amended DEC-650, DEC-652, DEC-653, DEC-654; scope table |
| `design_next/decisions/vfx_audio_ui.md` | 11 016 | amended DEC-750, DEC-751 (Affects paths); scope table; §3 pointer to DEC-950–952 |
| `design_next/decisions/data_sequences_platform_debug.md` | 19 377 | amended DEC-800, DEC-801, DEC-802, DEC-803; scope table |
| `design_next/decisions/deferred/networking.md` | 51 470 | amended DEC-202; stale paths |
| `design_next/decisions/deferred/process_tick_and_roadmap.md` | 58 265 | amended DEC-222, DEC-225, DEC-229 |
| `design_next/decisions/deferred/numerics_lobby_terrain_services.md` | 73 643 | amended DEC-241, DEC-246, DEC-247, DEC-250; stale paths |
| `species_work/dec_map_X-DEC-P4b.csv` | 9 092 | 30 rows, one per new entry |
| `species_work/p4_dec_triage_X-DEC-P4b.csv` | 22 957 | 110 rows: 30 new, 44 merged, 21 amend, 15 none |
| `species_work/p4_dec_handoff_X-DEC-P4b.md` | 6 804 | H1–H6 for files owned by X-DEC-P4a (the coordinator applies them) |
| `species_work/scripts/X-DEC-P4b_req.py`, `_check.py`, `_tables.py` | helpers | requirement lookup; citation checker; generator of the two CSVs |

Deferred entries stay Open. No file is near 115 KB. The decisions index `decisions/README.md` (coordinator-generated) does not yet list DEC-260–266 or DEC-950–972.

## 2 New entries

| ID | Title | Status | Sources |
|---|---|---|---|
| DEC-260 | Order within an actor-grid cell and container iteration order | Resolved | items 9, 16; C-SIM-b 1; C-SIM-a 4 |
| DEC-261 | Multi-round Siege rebuilds the match world at a tick boundary | Resolved | item 15; C-SIM-a 1 |
| DEC-262 | Generations of laser-bolt and Essence-orb handles, and the team selection | Resolved | item 15; C-SIM-a 2, 3 |
| DEC-263 | Route and actor-group edge cases | Open | item 10; C-SIM-b 2, 4 |
| DEC-264 | What runs while game speed is 0 | Open | item 11; C-SIM-c 1 |
| DEC-265 | Real duration of the speed-crate holds | Open | item 11; C-SIM-c 2 |
| DEC-266 | Decorative objects in the simulated pools | Open | item 11; C-SIM-c 4, 5 |
| DEC-950 | Keyboard operation of the menus | Resolved | item 92; C-UI-a 1; C-INP 6 |
| DEC-951 | Repeat and increment timing of the UI and of input | Resolved | item 100; C-UI-a 2, 3; C-INP 7 |
| DEC-952 | UI framework reference behaviours kept as defaults | Open | items 94, 99; C-UI-a 4, 6–9; C-UI-b 1 |
| DEC-953 | Input back-end rules | Resolved | item 97; C-INP 1, 4, 7 |
| DEC-954 | Input reference behaviours kept as defaults | Open | item 97; C-INP 2, 3 |
| DEC-955 | Presentation clocks while the world is held, and camera shake | Resolved | items 11, 103; C-SIM-c 3; C-CAM 3, 4; C-VFX 5 |
| DEC-956 | Pick-ray pixel convention | Open | item 103; C-CAM 2 |
| DEC-957 | Every draw sets its complete pipeline state | Resolved | item 66; C-RND-b 6 |
| DEC-958 | Renderer reference looks kept as defaults | Open | items 41, 66; C-RND-a 1, 2, 5; C-RND-b 1 |
| DEC-959 | Renderer values the reference does not record | Open | items 41, 66; C-RND-a 3, 4; C-RND-b 2, 3 |
| DEC-960 | Effects advance in fixed steps | Resolved | item 91; C-VFX 4; C-RND-b 5 |
| DEC-961 | Explainer panels lay out again on every screen change | Resolved | item 91; C-VFX 1 |
| DEC-962 | Effect values the reference does not record | Open | item 91; C-VFX 2, 7, 8 |
| DEC-963 | Animation and VFX reference looks kept as defaults | Open | item 91; C-VFX 3, 9 |
| DEC-964 | Accessibility limits for camera shake and flashes | Open | item 91; C-VFX 6 |
| DEC-965 | Audio engine reference behaviours kept as defaults | Open | C-AEN 1–4 |
| DEC-966 | Terrain query edge cases | Open | item 78; C-TER 1, 2 |
| DEC-967 | Reproducible terrain colours and no regeneration of the height field | Resolved | item 78; C-TER 5, 6 |
| DEC-968 | Content-build validation of terrain, map and record data | Resolved | items 78, 118; C-TER 4, 7; C-DAT 1, 4, 5, 6; arbitration A-20 |
| DEC-969 | Co-op group lists in ascending position order | Open | item 118; C-DAT 2; arbitration A-19 (design choice) |
| DEC-970 | System summary and crash-context records | Resolved | item 115; C-PLT 1, 2 |
| DEC-971 | Reference test machine and minimum-specification hardware | Open | item 111; C-DBG 1 |
| DEC-972 | What the cheats-used marker suppresses | Open | item 111; C-DBG 2 |

Items 9 and 16 are reconciled in DEC-260 as the brief asks: within an actor-grid cell the order is ascending actor serial (history-free), so DEC-650's registration schedule has no effect; containers keep the documented slot order (R-SIM-206 to R-SIM-209), whose same-tick consequences stay in DEC-455. Every Open entry states the current document default; where the reference defines no behaviour the document's default stands in (stated in the entry).

## 3 Amendments in my files

- **DEC-650**: within-cell order cites DEC-260. **DEC-652**: the offset-table step is named (step 11 of table 2.4-A, C-SIM-a 5); forced crate outcome (X-DEC-P4a H-2); bots path. **DEC-653**: item 7 agrees with DEC-465 (item 47, X-DEC-P4a H-1 text); items 5 and 6 cite DEC-852 and DEC-862 (H-3); new item 10 (actor group with no reporting member, C-SIM-b 3) and item 11 (terrain ray entry with direction.x = 0, C-TER 3); option B extended. **DEC-654**: VFX path.
- **DEC-750, DEC-751**: Affects paths to the split folders.
- **DEC-800**: option B corrected twice: the reference settings writer re-emits comment and blank lines (item 71, G09b) and the two tile fields read back through space padding (arbitration A-24). **DEC-801**: Affects gains R-MODE-084 and `pages_and_screen_recipes.md` UI-372 (C-UI-b request 6; item 60: the DEC is right, R-MODE-084 changes). **DEC-802**: path. **DEC-803**: timeline-only structure focus and approach and the developer fly mode recorded, options and recommendation updated (item 101, C-CAM 1).
- **DEC-202** (deferred): interim debug-command authority, the host's own participant (item 111, C-DBG 3). **DEC-222**: decorative objects stay in the pools (DEC-266), orb hover draws stay simulation draws. **DEC-225**: points to DEC-264. **DEC-229**: Owner decision line for O20 (item 24, X-DEC-P4a H-4). **DEC-241** item 6: the obstacle push of R-SIM-382 relies on it (C-SIM-b 5); zero facing (C-DAT 6). **DEC-246** item 6: wording per TER-062 (C-TER request 7). **DEC-247** item 1: grid-size limit (DEC-968). **DEC-250**: new item 7, the settings text form until decided (C-DAT 3); the bindings file name in item 1 replaced by a description, because it looked like an original file name.
- Stale `Affects` paths repointed in all my files (`animation_and_vfx/`, `data_schemas/`, `07_bots/`, `06_controls_camera_and_commands/`, `10_audio_design/`).

## 4 Per item (phase4_dec_fixes.md)

| Item | Outcome |
|---|---|
| 9, 16 | new DEC-260 (Resolved); DEC-016 via hand-off H1, DEC-455 via H5; document fixes R-SIM-307, R-SIM-310, table 2.9-A, 07_bots README |
| 10 | DEC-263 (route without corridor test; attack without candidate); DEC-653 item 10; DEC-241 item 6; co-op colour command merged into DEC-119 |
| 11 | DEC-264, DEC-265 (DEC-039 wording via H4), DEC-955 (frame delta while paused), DEC-266 (with the orb hover draws) |
| 12 | X-DEC-P4a already amended DEC-116 (now Resolved) and DEC-036; document fixes R-MODE-044, R-VIS-052 |
| 13 | none (document fix: R-SIM-503 cites `ref.mode.mode_second`) |
| 14 | X-DEC-P4a already amended DEC-011 |
| 15 | DEC-261 (DEC-066 via H2), DEC-262, DEC-652 amended |
| 17 | none (cross-document fixes of R-SIM-612 and R-SIM-730) |
| 24 | DEC-229 Owner decision line |
| 41, 66 | DEC-957, DEC-958, DEC-959, DEC-960 (merged); C-RND-b 4 and 7 none (G06 R-CTRL-384 states the order; interpretations for R1) |
| 47 | DEC-653 item 7 amended; the DEC-550 part is X-DEC-P4a's |
| 60 | DEC-801 unchanged and right; document fix R-MODE-084 |
| 71 | DEC-800 option B corrected; DEC-044 and DEC-570 parts are X-DEC-P4a's |
| 78 | DEC-966, DEC-967, DEC-968; DEC-653 item 11 |
| 89 | X-DEC-P4a already amended DEC-567 |
| 91 | DEC-955, DEC-960 to DEC-964; Brood Drifter tail merged into X-DEC-P4a's DEC-856 item 4; particle sub-step cap merged into DEC-106 |
| 92 | DEC-950 (Resolved) |
| 93 | X-DEC-P4a already amended DEC-102 |
| 94, 99 | DEC-951, DEC-952; Back on an open list, edge scrolling and colour-row availability merged into DEC-102 (defect suite DT-18 to DT-20); DEC-102 Affects via H3 |
| 97 | DEC-953, DEC-954, DEC-950; C-INP 5 none (R1 check); DEC-554 cross-reference via H6 |
| 100 | DEC-951: repeat timers on real time; document fixes UI-216 and UI-077 |
| 101 | DEC-803 amended; DEC-024 already amended by X-DEC-P4a |
| 103 | DEC-956, DEC-955; R-SIM-581 is arbitration A-18 (document fix, no DEC of mine states the losing version); action ids are routing |
| 106 | none: the Phase 4 baseline gate 6 reports every Open DEC cited |
| 111 | DEC-971, DEC-972; DEC-202 constraint 8 |
| 115 | DEC-970 (Resolved, both rules) |
| 118 | DEC-968 items 2 to 5 (A-20: Hive build check kept, reference replacement kept for unvalidated records), DEC-969 Open (A-19 design choice), DEC-250 item 7 |

C-AST and C-BLD made no DEC requests (none). Every report request has its own row in the triage CSV, with `cite_in` (the requirement that must cite the DEC; every Open DEC has one) and `document_fix`.

## 5 Arbitration used

A-18 (R-SIM-581: document fix only), A-19 (DEC-969, now Open as a design choice), A-20 (DEC-968 item 3), A-24 (DEC-800 option B). A-2 and A-23 touch no statement in my files (DEC-222 item 7 already counts four Wisp draws).

## 6 Hand-off and open points

- `p4_dec_handoff_X-DEC-P4b.md`: H1 DEC-016 (items 9, 16), H2 DEC-066, H3 DEC-102 Affects, H4 DEC-039, H5 DEC-455, H6 DEC-554. H1–H4 change `verdicts_and_fixes.md` by −90 bytes in total; each old text was re-checked to occur exactly once in the current files.
- **Not settled, flagged:** the damage-flicker cadence (VFX-126 "per 0.1 s" vs R-UNIT-051 "each render frame") is a "verify in ORIG" conflict not on X-ARB's list; DEC-963 item 2 records it and the triage row asks for a check.
- **Gate 6** now fails only on uncited Open DECs, as expected until wave B adds the citations: DEC-263–266, DEC-952, 954, 956, 958, 959, 962–966, 969, 971, 972 (plus X-DEC-P4a's). No other gate failure names my files.
- **Sizes for wave B:** `terrain.md` is 117.2 KB, so its DEC citations may need the `terrain/` split; `diagnostics_test_and_performance.md` must be split first (item 113).
- **Process note:** while checking DEC-116, I read nine lines of `design/41_risks_and_open_questions.md` (l.100–108) through its dec_map reference. Nothing from them was used: the outcome follows the current documents and X-DEC-P4a's amendment. `orig_refs` in my dec map are taken from package input ranges and X-ARB's evidence lines, not from new ORIG reads.
