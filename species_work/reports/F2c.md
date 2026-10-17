# F2c — Routing plan for ORIG 40, 41 and 90

## Files written

- `species_work/routing_F2c.csv`: 220 rows (40: 129, 41: 80, 90: 11). Columns as briefed. `target_file` is relative to `design_next/`, and `adr/` stands for the ADR set.
- `species_work/scripts/F2c_routing.py`: the row specs, the CSV writer and the coverage check. To re-run: `python3 -I F2c_routing.py design species_work/headings.csv species_work/routing_F2c.csv`.
- `species_work/scripts/F2c_packages.py`: sizes of the proposed package splits.

## Coverage (gate G1)

- For each file, the rows cover every line from the H1 heading to the H1 end line in `headings.csv`: 40 to line 806, 41 to 1042 and 90 to 1111. Each of these is one past `wc -l`, because of the trailing newline. `bytes` is computed exactly as `headings.py` computes it.
- Every line has exactly one **primary** row. The primary rows' bytes sum to the file size: 201,649, 338,850 and 215,669.
- Every overlap is deliberate, and its note says "(shared)" or "shared with …". There are two kinds:
  - A single-line or sub-range row that gives one table row of a primary H3 row a second consumer. Examples: a D-row going to X-ADR, or an F-row going to the DD-NET DEC.
  - A full-range pair:
    - DP-CLOSING + TO-DEC for 40's open questions;
    - KEEP + TO-DEC for 40 §5.1;
    - DP-CLOSING + TO-ASSETS for 41 §5.2.
- Routing is at H2 by default and at H3 where an H2 mixes actions. Where one H3 table mixes actions, I split it into contiguous line ranges, one per table row group:
  - 40 §2.2 and §3.1;
  - 40 §4.8;
  - 41 §3.12, §3.14 and §3.15.
- 40 §4 is all DEFER-DESIGN, but I routed it per H3 so that each design-phase topic gets its own ranges.

## Counts

| action | rows (40 / 41 / 90) | bytes, all rows | bytes, primary rows |
|---|---|---|---|
| TO-DEC | 44 (15 / 29 / 0) | 247,937 | 233,439 |
| DEFER-DESIGN | 78 (56 / 22 / 0) | 92,725 | 66,806 |
| TO-ADR | 25 (24 / 1 / 0) | 49,371 | 36,903 |
| KEEP | 4 (2 / 0 / 2) | 186,817 | 185,944 |
| KEEP-HOOKS | 17 (15 / 2 / 0) | 8,681 | 0 (all shared) |
| MERGE | 6 (0 / 6 / 0) | 2,936 | 0 (all shared) |
| TO-ASSETS | 4 (1 / 3 / 0) | 8,035 | 0 (all shared) |
| DROP-PROVENANCE | 41 (15 / 17 / 9) | 233,076 | 233,076 |
| DROP-LEGACY | 1 (1 / 0 / 0) | 929 | 0 (shared) |

| owner_package | rows | bytes, all rows | unique-line input |
|---|---|---|---|
| X-DEC | 139 | 349,343 | about 300 KB, so it needs a split (below) |
| X-GLO | 2 | 178,941 | 178,941, so it needs a split (below) |
| X-ADR | 25 | 49,371 | 48.2 KiB |
| X-AST | 4 | 8,035 | 7.8 KiB |
| G00 | 2 | 7,876 | 7.7 KiB |
| G02a / G02b / G04 / G10 | 2 / 2 / 1 / 1 | 850 / 875 / 1,060 / 151 | MERGE rows only |
| (none: DROP rows) | 42 | 234,005 | — |

## Proposed package split (each 150 KB of input or less)

Sizes are unique-line bytes from `F2c_packages.py`, counting only my three files.

| Package | Takes | Input |
|---|---|---|
| **X-DEC-1**: verdicts, keep-list, fix-list, risks | every TO-DEC and KEEP-HOOKS row of 40, plus those of 41 §1–§2 (41 lines < 167) | 117.5 KiB |
| **X-DEC-2**: design-phase topics | every DEFER-DESIGN row, giving one DEC per DD topic (list below) | 85.8 KiB, before F2b's DEFER rows for 20 and 22 |
| **X-DEC-3**: open-question register | TO-DEC and KEEP-HOOKS rows of 41 §3 (lines 167–603) | 125.1 KiB |
| **X-GLO-1** / **X-GLO-2** | 90 lines 96–475 (A–L) / 476–844 (M–Z) | 91.8 / 83.0 KiB |
| X-ADR, X-AST, G00 | as routed | 48.2 / 7.8 / 7.7 KiB |

- **X-DEC-2 may need a further split.** F2b's DEFER rows for 22 (300 KB of netcode) will land in X-DEC-2. If they do, split it by topic: DD-NET in one package, and DD-LOBBY, DD-NUMERIC, DD-THREADS, DD-SERVICES and DD-TERRAIN-RUNTIME in the other.
- **The three X-DEC agents share one register.** The coordinator assigns each a range of DEC numbers.

## DEFER-DESIGN topics and the rows that feed them

Format: `file:start-end`. A `*` marks a row whose note names the topic second, after another DD code. X-DEC writes one Open DEC per topic, with "Decide by: design phase".

- **DD-NET** (39 rows): the failure modes the networking model must prevent, plus the bandwidth and population figures.
  - 40:44 (D2), 40:84 (C10), 40:120 (R17), 40:168, 170, 174 (G28, G30, G34), 40:196, 40:198-199, 40:207 (G56, G58–G59, G67), 40:243 (T21).
  - 40:326-327, 40:328-347 (§4.1: scale figures), 40:348-364, 40:365-370, 40:371-392 (P1–P17), 40:393-443, 40:444-469 (§4.6: the failure-mode list), 40:470-482 (§4.7: tick, input delay and budgets), 40:483-488, 40:492-496.
  - 40:529 (K25), 40:552 (F16), 40:566-567 (F30–F31), 40:595 (F59), 40:616-675 (§7 lessons), 40:682 (40-U1), 40:687-688 (40-U6, 40-U7).
  - 41:83, 41:85 (top-15 ranks 11 and 13), 41:141 (R05), 41:143-145 (R07–R09), 41:147 (R11), 41:165 (R29), 41:389-395 (20-U1 to 20-U3), 41:430-433, 41:435, 41:442-448 (22-U*), 41:558 (29-U14), 41:600 (32-I1).
- **DD-LOBBY** (6 rows):
  - 40:49* (D7), 40:109 (R11), 40:120* (R17), 40:178 (G38), 40:244-245* (T22), 40:489-490 (§4.8 lobby and discovery).
- **DD-NUMERIC** (14 rows):
  - 40:45 (D3), 40:119 (R16), 40:232-233 (T10–T11), 40:346, 40:377-378 (P3–P4), 40:453, 40:566-567* (F31), 40:616-675*, 40:690 (40-U9), 40:696-697 (40-U15, 40-U16).
  - 41:137 (R01), 41:434 (22-U7), 41:436-440 (22-U10 to 22-U15), 41:456 (23-I1).
- **DD-THREADS** (12 rows):
  - 40:46 (D4), 40:117 (R14), 40:248-249 (T26–T27), 40:255 (T33), 40:379-380 (P5–P6), 40:390 (P16), 40:393-443*, 40:468, 40:616-675*.
  - 41:398-401, 41:403-406 (20-U*), 41:601 (32-I2).
- **DD-SERVICES** (12 rows):
  - 40:49 (D7), 40:109*, 40:133-134 (R30), 40:177 (G37), 40:179 (G39), 40:244-245 (T22–T23), 40:491, 40:616-675*, 40:689 (40-U8), 40:698 (40-U17).
  - 41:146 (R10), 41:424-429 (22-U1, 22-U2).
- **DD-TERRAIN-RUNTIME** (6 rows):
  - 40:50 (D8, terrain part), 40:107 (R9), 40:616-675*, 40:692 (40-U11), 41:436-440* (22-U12), 41:449-453 (23-U1).
- **DD-PERSIST**: no rows in 40, 41 or 90.

**Integrity material outside the DEFER rows.** Some simulation-integrity items sit next to DEFER material, so I routed them TO-DEC (shared): integer timers, per-match statics, presentation writing simulation state, shared generators, and bounded loops. They are 40:376, 40:388, 40:454-457, 40:460, 40:467, 41:396-397, 41:402 and 41:441. Following the X-DEC brief, they are always Resolved, not deferred.

## KEEP-HOOKS features (owner X-DEC; the hooks themselves come from 08, 09, 10 and 01 via F2a)

- KH-EDITOR: 40:52 (D10), 40:108 (R10), 40:210 (G70), 40:591 (F55)
- KH-REPLAY: 40:52, 40:197 (G57), 40:389 (P15)
- KH-ACHIEVE: 40:52, 40:109 (R11), 40:176 (G36), 40:543 (F7), 40:693 (40-U12), 41:356 (09-U1)
- KH-RESULTS: 40:52, 40:180 (G40), 40:559 (F23)
- KH-MUSIC: 40:52, 40:110 (R12), 40:194 (G54), 40:585 (F49), 41:382 (10-U12)

## How the brief's guidance was applied

- **40 §1.2 (lines 39–53)** is one H3, routed once as TO-DEC with the D-row split in its note. Shared single-line rows give each D-row its other consumer:
  - D1 → KEEP for G00;
  - D2, D3 and D4 → DEFER;
  - D5 and D9 → TO-ADR;
  - D7 → DEFER (DD-SERVICES, also DD-LOBBY);
  - D8 → TO-ADR plus DEFER (DD-TERRAIN-RUNTIME);
  - D10 → KEEP-HOOKS plus DROP-LEGACY (DL-CUT).
  
  D6 stays in the TO-DEC row, as Resolved: integer ticks, with the mode second at 11 ticks.
- **40 §3.1** is TO-ADR, except T10–T11 (DD-NUMERIC), T21 (DD-NET), T22–T23 (DD-SERVICES) and T26, T27 and T33 (DD-THREADS). The row numbers are in each note. §3.2 is TO-DEC and §3.3 is TO-ADR. The §3.3 note tells X-ADR that the N3, N4 and N7 nodes are superseded topics.
- **40 §4** is DEFER-DESIGN. A few rows are shared with other routes: P2 and P14 (TO-DEC), P10 (TO-ADR) and P15 (KEEP-HOOKS).
- **40 §5.1** is KEEP for G00 plus TO-DEC. **40 §5.2** is TO-DEC.
- **40 §6** is DP-CLOSING, consumed by the F1 ledger. Two of its lines, the Art and Audio rows, are also shared as TO-ASSETS, for count cross-checks only.
- **40 §7** is DEFER-DESIGN. See request 1 below.
- **41:**
  - §1.1 and §1.2 are DP-METHOD.
  - §1.3, §1.4, §2 and §3.1–§3.24 are TO-DEC, except the netcode rows of §3.12, §3.14 and §3.15, which are DEFER.
  - §3.0 is DP-METHOD: recovery-phase questions whose resolutions already live in the owning documents.
  - §4 is DP-CLOSING.
  - §5 is DP-CLOSING, consumed by the F1 ledger.
  - §5.2 is also TO-ASSETS, for map counts only: 57 level files, 53 of them with a menu entry.
  - The IP48 line (907) is also TO-ASSETS, for the art briefs: distinctive expressions that need a new presentation.
- **90:**
  - §1 is DP-METHOD.
  - §2 is KEEP for X-GLO, in two halves.
  - §3 is DP-CLOSING, consumed by the F1 ledger.
  - The closing sections are DP-CLOSING. None of them holds an in-scope open question.
- **Reason codes.** I used only the codes in `drop_topics.md`. The brief's "DROP-PROVENANCE" sections without a closing-section, method or index role (40 §6, 41 §5 and 90 §3) carry DP-CLOSING, the "IP items" class.

## DEC requests (summary; the details are in the row notes)

- **Resolved** (40 recommends a fix):
  - D6;
  - every "Fix" row of the fix-list (F1–F59) and the "Keep for feel" rows F1 and F42;
  - the integrity fixes: 40 C1, C8 and §4.6, and 41 §2 rows R02–R04 and R06;
  - P14 bounded loops.
- **Open** (reference default):
  - the "Decide" rows F5, F14, F25, F26, F27, F34, F40, F43 and F53, and G25, G43 and G44;
  - the confirm-DECs from 41 §1.3 ranks 1, 5–10, 12, 14 and 15;
  - 41 §3, entry by entry, as noted per H3.
- **One DD-NET entry for lockstep-only defects:** F16, F30, F31 and F59, 40 §4.6, and 41 R05, R07–R09, R11 and R29.
- **Dropped:** the 40 drop rows (listed in the 40:135-214 note) and the legacy-only 41 §3 rows, flagged "→ Dropped" with their DL class in each note.
- **For the coordinator: 40-U14.** It asks whether the reference tuning values may be used verbatim. It decides whether "Keep" means the same numbers or only the same structure. Prompt §0 "Fidelity" already says to keep every number, so I routed it TO-DEC as an Open owner question rather than escalating it. Log it, or escalate it if you judge that it changes scope (§10 item 3).

## ADR requests (summary)

The TO-ADR rows cover these topics:

- the renderer (D5, T1–T9, R18–R22, G46–G52);
- audio (D9, T12–T14, R13, G55);
- input (R7, T19–T20, G14);
- the UI and glyph cache (T16, T24, R23, F54);
- textures and pack files (T15, T17, D8, C7, G72, F50);
- the data schema format (T25, R10, R24, F52);
- the content build and hashes (T29, P10, G67);
- localisation (R28, F51);
- the platform layer, clock and window (T9, T18, T32, G71);
- the toolchain, build and CI, packaging and crash reporting (T28–T31, R26, R29);
- diagnostics, profiling, logging and telemetry (T35–T37, R27, G61–G66);
- the mesh pipeline (T34);
- screenshots (T38);
- 41 R18–R21.

For X-ADR's "Deferred to the design phase" list, the one-line problem statements come from the D2, D3, D4 and D7 rows and the terrain part of D8.

## Cross-document requests

1. **Coordinator or F1: there is no reason code for a roadmap.** `drop_topics.md` has no DEFER code for "build order and roadmap". The 40 §7 row (616–675) therefore carries `DD-NET` first, then DD-NUMERIC, DD-THREADS, DD-SERVICES and DD-TERRAIN-RUNTIME, for the per-topic lessons. If you want §7 as its own topic, F1 can add `DD-ROADMAP`, and I re-tag the row.
2. **Coordinator: the 40 §5.2 fix-list has 59 rows (F1–F59), not 61.** There is no F60 or F61 anywhere in 40.
3. **X-DEC: use 41 §3 as the master open-question list.** 41 §3 de-duplicates every document's "Open questions and unknowns" section: 322 source rows become 235 entries, with status and impact. Use the per-document closing-section TO-DEC rows from F2a and F2b as cross-checks, not as new entries.
4. **Coordinator: read-only context for the packages.**
   - Give X-DEC and X-ADR 40 lines 28–38, the verdict legend.
   - Give X-GLO 90 lines 42–63 (entry columns and kinds) and 1102–1111 (vocabulary by build tag, to use as its exclusion list).
5. **Coordinator: some owners fall outside the brief's list.**
   - **MERGE rows.** These are the 41 §1.4 lines 97–104, facts resolved only in 41. Their owners are G02a, G02b, G04 and G10, which come from §9.3 but are outside the brief's X-DEC, X-ADR, X-AST, X-GLO and G00 list. Those packages should state the facts as rules: the open question in their own document is resolved.
   - **KEEP-HOOKS rows** are owned by X-DEC, because they hold 40's verdicts. The hooks themselves are specified by G08, G09, G10 and G01 from their own documents.
6. **X-AST: the retail map count differs between documents.** 41 §5.2 has 53 menu maps, while 40 §4.1 says 49 retail maps. `X4_levels.csv` stays the source.

## Names needed but not in the ledger

None. Routing introduces no Species names. The CSV notes use some original terms; it is a WORK file and is deleted at the cutover.

## Dropped, by group

- **DP-INDEX:** the title blocks and contents lists of all three files, and the bare H2 heading lines (40 §1, §3, §5; 41 §1).
- **DP-METHOD:**
  - 40 §1.1, §1.3, §1.4 and the §2 reading notes;
  - 41 §1.1, §1.2, the §3 intro and §3.0;
  - 90 §1.
  
  Reason: method, statistics or recovery history.
- **DP-CLOSING:**
  - every closing section of the three files;
  - 40 §6;
  - 41 §4 and §5;
  - 90 §3.
  
  Reason: the IP registers and placeholders are consumed by the F1 ledger, and the docs-vs-code registers are never carried.
- **DL-CUT:** the D10 row (cut-scene interpreter, narrator, loader mini-programs, gesture recogniser). Legacy-only drops inside TO-DEC tables are not split out. Their DL class is named in the row note, so X-DEC can record each as Dropped.
