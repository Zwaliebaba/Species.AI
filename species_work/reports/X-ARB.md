# X-ARB report: arbitration from the reference (Phase 4)

## Files written

| File | Size | Content |
|---|---|---|
| `species_work/p4_arbitration.md` | 75.1 KB | 24 entries A-1 to A-24, each with question, statements, ORIG evidence (file and line), verdict and replacement text; summary table at the end |
| `species_work/scripts/X-ARB_extract_replacements.py` | 1.3 KB | pulls the replacement blocks out of the working file so they can be scanned on their own |
| `species_work/reports/X-ARB.md` | this file | |

No file in `design_next/` was edited. No publishing, upload or git write tool was used.

## Outcome per input item

| Item | Entry | Outcome |
|---|---|---|
| 20 | A-1 | Settled. Both values exist. A co-op group's shade request returns white when the group has no colour (R-SIM-421 stands). The DEC-078 grey is the member shade for colour id none, which is the no-team colour. Fix: R-MODE-017 wording and its tuning note. |
| 21 | A-2 | Settled: 2 × **current** health. ORIG 21's "StatHealth" is the entity's own health slot. Fix: R-SIM-407, its tuning unit, AC-SIM-06-14. Side note: the factor 2 is defined twice (`ref.sim.team.eliminate_damage_factor` and `ref.econ.elimination.damage_factor`). |
| 25 | A-3 | Settled for G02a2: a Builder delivers to the nearest friendly Spawn Node. ORIG 02 and ORIG 10 describe the delivery code and override the ORIG 05/40 summaries. Fix: R-ECON-082 and five related places in reference/05, the G00 glossary row and the builder.md §8 title. DEC-085 rewritten as Resolved (full text given for X-DEC-P4a). |
| 29 | A-4 | Settled: the door Worker is airborne. The spawn hook grounds it, then the Spawn Node clears the flag. The two documents agree; R-ECON-043 and the gate sentence of §2.21 now state the order. |
| 42 | A-5 | Settled: 900 on seven maps, 1,200 on one 4-player map. Fix: R-BLDG-317. |
| 46 (and 49) | A-6 | Verified: G03b is right on all three points (actual height, first tree in structure order, 101 ticks). R-CRATE-206 and R-CRATE-181 already agree. No change. |
| 48.1 | A-7 | Settled for G04: the whole-map Powers are the airstrike, Firebomb Run, Missile Barrage, Swarm, Essence Grove, Blight Grove, Hive, Brood and meteor shower, plus the turret test. Sickness is never a Power. Fix: R-BLDG-464. |
| 48.2 | A-8 | Settled: the rocket-turret rocket lives 11 ticks. Related finding: the flame-turret fireball also lives 11 ticks, not 10. R-CRATE-461 then needs its radius growth stated directly, with one new key. Fixes in defences_and_props.md and weapons_and_projectiles.md. |
| 48.3 | A-9 | Settled for G04: stopping a Transport's Power destroys the Transport (−1000), except in a tutorial. Fix: R-UNIT-370 and three dependent rows. |
| 48.4 | A-10 | Settled: 601 ticks, because the counter is binary32 (X-ARB replayed it). Fix: strike_team.md key, rule, edge case and acceptance test. |
| 48.5 | A-11 | Settled: the bad branch never yields an airstrike or Firebomb Run. Fix: remove R-UNIT-711 row 3, the §1 owner cell and the hostile-airstrike edge case. |
| 52 | A-12 | Settled: bot navigation nodes and objective markers recount every **20** ticks. Their timers count down and are reset, so DEC-064 does not cover them. The coordinator's proposed action ("G07 states 21") is **not** taken. Fix: R-BLDG-003 and two rows of reference/03 table 2.7-A. |
| 54 | A-13 | Settled for G07. The ferry pickup is the first qualifying node in structure order. The Strike team scan goes through the query results in order, keeping a running nearest distance. Fix: R-UNIT-407 item 2 and R-UNIT-360. |
| 61 | A-14 | Settled by derivation: 258 Spawn Nodes on 32 of the 49 match maps, split no team 120 and positions 0 to 3 as 48, 48, 28, 14. Fix: R-BLDG-132. Table 2.11-A of reference/08 stays, because it states its 57-file scope. |
| 62 | A-15 | Settled: two Zone Control maps and one Spawn Control map. Fix: RND-229. |
| 77 | A-16 | Settled: 256 levels (0 to 255). Fix: R-MAP-081 and its tuning row; the A to P letter form is dropped. |
| 98 (R-META-044) | A-17 | Settled for the UI framework: left and right never change a cycle-mode drop-down. Fix: R-META-044 row 3. |
| 98 (R-META-406) | A-22 | Settled for C-UI-b: the colour-row press stores the colour and saves the settings file. Fix: R-META-406 gains item 7; R-META-081 updated. |
| 103 | A-18 | Settled: the camera cuts once per build of the match world, and the in-place reset re-arms the cut. R-CTRL-497 and CAM-061 are right. Fix: R-SIM-581 steps 2 and 11.3. |
| 118 (co-op order) | A-19 | Reference content settled: the authored order is ascending on every reference map, and no group has more than 2 positions, so DAT-088 changes nothing there. Custom maps are a **design choice** for X-DEC-P4b (default A, ascending). Fix: R-MODE-020 states the order and the limit. |
| 118 (Hive) | A-20 | Reference rule settled: the run-time replacement by 0, bound 10,000. The build error is a Species check, a **design choice** for X-DEC-P4b (default A, keep both). Fix: R-BLDG-633 cross-cites DAT-159. |

Extra conflicts found in the Phase 3 reports that only ORIG could settle:
- A-21 (G03b to G01): a reference Siege map has 12 to 40 neutral gun turrets, not 7 to 40; the 7 comes from the stress-test file. Fix: R-MODE-354.
- A-23 (G02b to C-SIM-c): a Wisp's wobble makes three `NextSigned(1)` draws and one `NextUnit(20)` draw each tick. Fix: R-UNIT-596 and one new key.
- A-24 (C-TER to G08b): the tile writer puts no separator between two fields, but the field padding still separates them. Fix: a clause of R-MAP-387.

## Hand-offs

- **X-DEC-P4a:** the DEC-085 replacement text in A-3. A-12 means item 52's proposed action is not taken.
- **X-DEC-P4b:** the design-choice notes in A-19 and A-20, for the item 118 entry.
- **Document fixers / X-ROUTE:** every "Replacements" block. The summary table at the end of `p4_arbitration.md` lists, for each entry, the file and requirement ids that must change.
- **Not arbitrated** (no ORIG question): G07's request to align R-UNIT-359 with R-BOT-014. It belongs to the routing pass and to the Open DEC of item 50.

## Checks

- Replacement text: `X-ARB_extract_replacements.py` pulls all 118 replacement lines into a scratch file. `scan_patterns.py` with `forbidden_patterns.tsv` prints nothing on it, and with `soft_patterns.tsv` prints nothing either.
- Whole working file: it has hits (137 forbidden, plus soft-pattern hits). Every hit is on the header line or inside an ORIG evidence quote, which the X-ARB brief allows for this working file. Nothing in this file is meant for `design_next/`.

## Could not do

Nothing is left open. One entry rests on derived figures rather than a direct ORIG statement: A-14's 49-map owner split. It comes from the 57-file census minus the two non-match files, and the totals check exactly (286 − 24 − 4 = 258).
