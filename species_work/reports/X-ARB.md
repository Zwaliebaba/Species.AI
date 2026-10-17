# X-ARB report: arbitration from the reference (Phase 4)

## Files written

| File | Size | Content |
|---|---|---|
| `species_work/p4_arbitration.md` | 131.1 KB | 42 entries A-1 to A-42, each with question, statements, ORIG evidence (file and line), verdict and replacement text; summary table at the end |
| `species_work/scripts/` helper named X-ARB + `_extract_replacements.py` (the mandated `<ID>_*.py` form; its name trips one technology pattern of the forbidden list, a false positive) | 1.3 KB | pulls the replacement blocks out of the working file so they can be scanned on their own |
| `species_work/scripts/` helper named X-ARB + `_census.py` | 3.2 KB | recounts ORIG 08's per-file camera-mount and landscape tables over the 49 match maps; it first reproduces the 57-file figures of R-MAP-128 and R-MAP-086 exactly |
| `species_work/reports/X-ARB.md` | this file | |

No file in `design_next/` was edited. No publishing, upload or git write tool was used.

## Outcome per input item

| Item | Entry | Outcome |
|---|---|---|
| 20 | A-1 | Settled. Both values exist. A co-op group's shade request returns white when the group has no colour (R-SIM-421 stands). The DEC-078 grey is the member shade for colour id none, which is the no-team colour. Fix: R-MODE-017 wording and its tuning note. |
| 21 | A-2 | Settled: 2 × **current** health. the stat slot that ORIG 21 names is the entity's own health slot. Fix: R-SIM-407, its tuning unit, AC-SIM-06-14. Side note: the factor 2 is defined twice (`ref.sim.team.eliminate_damage_factor` and `ref.econ.elimination.damage_factor`). |
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

Coordinator request (after the first hand-back):
- A-25 (VFX-126 against R-UNIT-051): settled for VFX-126. ORIG computes the damage-flicker flag once per tick inside the per-tick advance; each render frame only reads it. The sine uses the unscaled wall time. Fix: R-UNIT-051 (full replacement in A-25).

Coordinator request 2 (conflicts flagged by the routing agents; route ids closed):
- A-26 (RA-066): only a Fury link gives the 0.75 damage factor; the Frenzy timer gives the combat behaviours only. Fix: R-UNIT-125, R-UNIT-116 step 4.
- A-27 (RA-073): the Worker's 20 wu stand distance is a separate constant; the Flag post's capture radius is 40 wu. Fix: R-UNIT-095, plus a new key `ref.unit.worker.flag_stand_range`.
- A-28 (RA-324): 38 option definitions in scope; the 39th belongs to the debug-only mode. Fix: R-VIS-019 and its tuning row.
- A-29 (RA-117): owner decision O18 (DEC-107) stands. ORIG only confirms that hostile outcomes ignore exclusions in the reference behaviour, which O18 replaces. Fix: R-VIS-124 and the §8 row.
- A-30 (RA-030): basic outcomes are switched on at the second tutorial's end step, the same step that releases the bots. Fix: R-CRATE-031 wording.
- A-31 (RA-093): ORIG does not say when a Transport frees its sea routes. **Design choice**: default is to free the previous route on each new route and the current one on removal. Note for X-DEC-P4b (DEC-651); R-SIM-357 gains one sentence.
- A-32 (RA-219, draw count only): the bot controller makes 2 draws of its own, after the rate draw that setting its kind makes. The reference total of 4 counts the rate draw twice. Fix: R-SIM-626 and R-BOT-009. The other points of RA-219 stay with the routing pass.
- A-33 (RA-166): reference/06 is right on all three points. A one-shot Power stays Waiting. The pop-up sources are Supply crate, reinforcement and Comeback Bonus. The marker is removed after 8 s of accumulated on-screen time. Fix: R-CRATE-021, R-CRATE-023, R-CRATE-028 and a tuning note.
- A-34 (RA-200 point b): both `Select` and `SelectPower` boost the selection arrows. Fix: R-CTRL-375.
- X-ROUTE-b: none of its X-ARB rows is a new conflict. RB-051, RB-056, RB-173, RB-174, RB-208, RB-221, RB-224 and RB-292 apply existing entries. RB-091 is a citation fix: A-1 confirms that R-BLDG-048 should cite `ref.match.colour.no_group`. RB-306 waits on X-DEC-P4b.

Coordinator request 3 (residual round, including the three FX-08 points):
- A-35 (RB-204): the limit stays 62. The reference declares 63, but the map reader copies a 63-character name without a terminator. Only the tuning note changes.
- A-36 (RA-221): recount over the 49 match maps. A new table 2.11-C gives the derivable owner splits (Command Console, turret, Control Zone, Core Device, grid terminal, Population Limiter, Spawn Node, Transit Gate, wall, and every kind with no records in the non-match files). Also recounted: turret types (189 gun, 11 flame), camera views (48, 48, 29, 12, 5) and terrain-asset use. The rest is not derivable from ORIG and keeps its stated 57-file scope: bot structure fields, Barrier Emitter modes and owners, pylon and Solar Array owners, Relay, Source Node and tree owners, and the Population Limiter range. Note for X-AST: one land ramp and one shore-wave gradient are used only by non-match files.
- A-37 (RB-004 items 1, 2): both pushes have no live caller in the reference, so R-UNIT-041 holds. No change.
- A-38 (RA-145): ORIG gives only the anchor (the mount's status sockets 01 to 04). The look is a **design choice**: the base port look, re-anchored. Note for X-DEC-P4a; a new rule goes in defences_and_props.md.
- A-39 (stat width): the reference stores 8-bit stats, and Resolved DEC-090 applies floating point. R-SIM-145 changes to the fixed rule, and its DEC-241 citation was wrong.
- A-40 (RB-044): the library is right. "160 × 512" is the reference atlas, holding the 32 px trail column plus the glow at 128 × 128. "32 copies" is the loop bound, of which 18 fit. RND-320 gains one optional sentence.
- A-41 (FX-08 §6.3): the ownership-light clock is the latest command frame **received**, and quad i has half-size 6 × i ÷ 10 (largest 10.8 wu across). R-BLDG-045 and RND-167 need fixing. Removing the duplicate `rnd.structure.light.*` keys is recommended, but it is the coordinator's call.
- A-42 (FX-08 §6.4): the values agree (occupied ÷ 5 ports; 30 s = 300 ticks; same colour). RND-169 should cite the reference keys in ticks, and its two duplicate keys should be removed.
- Naming: the replacement text uses no new code identifier and cites no ADR. Existing identifiers and keys keep their current spelling.

## Hand-offs

- **X-DEC-P4a:** the DEC-085 replacement text in A-3. A-12 means item 52's proposed action is not taken.
- **X-DEC-P4b:** the design-choice notes in A-19 and A-20 (item 118) and A-31 (DEC-651).
- **X-DEC-P4a:** the design-choice note in A-38 (turret port look).
- **Document fixers / X-ROUTE:** every "Replacements" block. The summary table at the end of `p4_arbitration.md` lists, for each entry, the file and requirement ids that must change.
- **Not arbitrated** (no ORIG question): G07's request to align R-UNIT-359 with R-BOT-014. It belongs to the routing pass and to the Open DEC of item 50.

## Checks

- Replacement text: the extraction helper pulls all 203 replacement lines into a scratch file. `scan_patterns.py` with `forbidden_patterns.tsv` prints nothing on it, and with `soft_patterns.tsv` prints nothing either.
- Whole working file: it has hits (137 forbidden, plus soft-pattern hits). Every hit is on the header line or inside an ORIG evidence quote, which the X-ARB brief allows for this working file. Nothing in this file is meant for `design_next/`.

## Could not do

Nothing is left open. One entry rests on derived figures rather than a direct ORIG statement: A-14's 49-map owner split. It comes from the 57-file census minus the two non-match files, and the totals check exactly (286 − 24 − 4 = 258).
