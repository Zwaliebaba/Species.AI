# X-DEC-4b report: decision register for controls, bots, maps, meta systems and audio

## Files written

- `design_next/decisions/controls_bots_maps_meta_audio.md`: heading, scope paragraph, generated index table, 21 entries (DEC-550 to DEC-570) in the §6.5 format, in four sections (controls and powers, bots, maps and level data, audio design). 32.4 KB. DEC-571 to DEC-649 are unused.
- `species_work/dec_map_X-DEC-4b.csv`: one row per DEC (`dec_id, title, status, affects_prefix, affects_files, orig_refs`), written with the csv module (`lineterminator='\n'`); `orig_refs` are `file:line-line` per cited ORIG row.
- `species_work/scripts/X-DEC-4b_build.py` (run with `python3 -I`, paths as arguments): checks ids, field order, status and "Decide by" rules, cross-references and Affects paths, writes the index table and the CSV.
- This report.

No Artifact, Docs, Drive, Gamma or GitHub tool was used; nothing was published. No git write command was run.

## Counts

| Status | Entries |
|---|---|
| Resolved | 10 (DEC-553, 554, 555, 560, 561, 563, 564, 568, 569, 570) |
| Open | 10 (DEC-550, 551, 556, 557, 558, 559, 562, 565, 566, 567) |
| Dropped | 1 (DEC-552) |
| **Total** | **21** |

Open entries all carry "Decide by" (design phase 8, playtest 2); Resolved and Dropped entries carry none. Input rows: 22 ranges; 4 open-question ranges skipped (X-DEC-3); the 18 body ranges hold about 185 rows, of which about 135 are covered by existing entries only and about 50 lead to one of the 21 new entries.

## Gate checks

- `scan_patterns.py forbidden_patterns.tsv` on the register: **0 hits**. `soft_patterns.tsv`: **0 hits**, nothing to justify. Truncation words: none.
- On the CSV, the forbidden scan reports only the ORIG file names in the `orig_refs` column (class provenance). That column is required by the coordinator's format, and the wave-2a maps give the same hits; the CSV is a WORK file.
- Build script: ids 550–570 unique and ascending; field order and the Open-only "Decide by" rule pass; every cited DEC id exists in `design_next/decisions/`; every Affects path is a package target in `INDEX.md` with its registered prefix.

## Input-row mapping

"cov" = covered by an existing entry (no new entry written). "X-DEC-3" = also an open question of that document's open-questions section, which X-DEC-3 handles through ORIG 41 §3.

### 06 §3.10 task-core edge cases (lines 520–533)

| Row | Mapping |
|---|---|
| E1 | DEC-550 |
| E2 | DEC-551 |
| E3 | DEC-552 (Dropped; the vehicle power is also DEC-057) |
| E4 | cov DEC-033 (two slots while eliminated), DEC-073 (Comeback Bonus holds at most two) |
| E5 | DEC-551 (stated there: the life limit counts down only above zero; instant-unit Strike teams have none) |
| E6 | cov DEC-021 (0–3 starting powers); extra fact 3 |
| E7 | cov DEC-115 (fixed match research levels), DEC-010, DEC-011 (no process-global simulation state) |
| E8 | DEC-551; ordering consequence DEC-556 |
| E9 | cov DEC-062 (the Summoner is not specified) |

### 06 §12.5 binding-file lines (2131–2174)

DEC-553 (effective default bindings). Parser rule cov DEC-096; gesture lines cov DEC-055. The lines of the third binding file belong to a non-Windows platform build (drop class DL-PLATFORM) and need no entry.

### 06 §13 input defects (2287–2323)

| Row | Mapping |
|---|---|
| X1 | cov DEC-008 (DEC-200 constraint 4); 06-U7 X-DEC-3 |
| X2 | cov DEC-008, DEC-200 constraint 6; the switch structure is campaign-only (DEC-052) |
| X3 | cov DEC-095; 06-U4 X-DEC-3 |
| X4 | cov DEC-007 |
| X5 | cov DEC-007 |
| X6 | cov DEC-007 (DEC-200 constraint 3) |
| X7 | cov DEC-009 (DEC-200 constraint 3) |
| X8 | cov DEC-008, DEC-200 constraint 6, DEC-055 |
| X9 | cov DEC-096 ("one click produces one selection command"), DEC-202 constraint 5; extra fact 2 |
| X10 | cov DEC-083 (DEC-202 constraint 4) |
| X11 | cov DEC-202 |
| X12 | cov DEC-024, DEC-096 |
| X13 | cov DEC-096; effective bindings DEC-553 |
| X14 | DEC-554; exact overflow result 06-U2 X-DEC-3 |
| X15 | cov DEC-050 (rebinding UI saves canonical names); key-name table DEC-554; 06-U3 X-DEC-3 |
| X16 | cov DEC-096 |
| X17, X18, X19, X20 | DEC-554 |
| X21 | cov DEC-083 |
| X22 | cov DEC-096 |
| X23 | DEC-555; 06-U9 X-DEC-3 |
| X24 | cov DEC-096 |
| X25 | cov DEC-024 (debug-advancer grace frames not specified) |
| X26 | cov DEC-024, DEC-096 |
| X27 | DEC-554 |
| X28 | DEC-564 (selection sounds play only for the local player) |
| X29 | cov DEC-050 |
| X30 | cov DEC-015 (a missing asset is never fatal) |

### 06 §14 re-implementation notes (2324–2336)

| Note | Mapping |
|---|---|
| 1 command model | cov DEC-023, DEC-008, DEC-007 |
| 2 authority | cov DEC-202, DEC-083 |
| 3 context resolver | cov DEC-023 |
| 4 input layer | cov DEC-096, DEC-050; new DEC-553, DEC-554; input stack is ADR scope |
| 5 camera | cov DEC-024 |
| 6 power bar | cov DEC-083; slot order DEC-556 |
| 7 gestures | cov DEC-055 (DL-CUT) |

### 07 §16 bot defects (1237–1269)

| Row | Mapping |
|---|---|
| B1 | cov DEC-014; 07-U4 X-DEC-3 |
| B2, B3, B4 | cov DEC-087 (correct ids) |
| B5, B6, B8, B10, B19, B23 | cov DEC-087 (umbrella; not named there, extra fact 1) |
| B7 | cov DEC-014 |
| B9 | cov DEC-087; 07-U5 X-DEC-3 |
| B11 | cov DEC-087 (not named; extra fact 1) |
| B12 | cov DEC-087, DEC-112; 07-U2 X-DEC-3 |
| B13, B24 | cov DEC-087, DEC-112; 07-U1 X-DEC-3 |
| B14 | cov DEC-087, DEC-112; 07-U8 X-DEC-3 |
| B15 | cov DEC-087 (not named), DEC-031; 07-U6 resolved |
| B16 | cov DEC-087 |
| B17 | cov DEC-087, DEC-112; 07-U10 X-DEC-3 |
| B18 | cov DEC-087 (crash defect); 07-U11 X-DEC-3 |
| B20 | cov DEC-077 |
| B21 | cov DEC-087 |
| B22 | cov DEC-087; 07-U7 X-DEC-3 |

### 07 §20 re-implementation guide (1368–1405)

| Item | Mapping |
|---|---|
| 20.1 items 1–7 (graph, priorities, dispatcher, Leaders, power handlers, mode strategies, difficulty) | cov DEC-022; extra fact 8 |
| 20.1 item 8 (deterministic discipline) | cov DEC-022, DEC-013, DEC-016, DEC-200 constraint 8 |
| 20.1 item 9 (perfect information) | cov DEC-022 (fog-of-war tiers are design-phase work) |
| 20.2 bugs, selection coupling, stale ids, NaN handling | cov DEC-087 |
| 20.2 unbounded loops | cov DEC-014 |
| 20.2 ids and indices | cov DEC-028 |
| 20.2 graph rebuilds | cov DEC-022, DEC-087 |
| 20.2 float timers | cov DEC-002 |
| 20.2 tie-breaks | cov DEC-016 |
| 20.2 movement | DEC-557 |
| 20.2 information, difficulty | cov DEC-022, DEC-112 |
| 20.2 neutral-hostile bots | DEC-558 |
| 20.3 module split | DEC-559 |

### 08 §9 level-design guide (2001–2016)

| Item | Mapping |
|---|---|
| 1 one file, header first | cov DEC-045, DEC-015; extra fact 6 |
| 2 content-hash identity | cov DEC-204, DEC-246 constraint 9 |
| 3 start positions | DEC-560 |
| 4 typed mode objects and validation | DEC-561 (typed mode objects also DEC-025) |
| 5 baked terrain plus hash | cov DEC-246 |
| 6 lighting and weather presets | DEC-562 |
| 7 typed per-map overrides | cov DEC-073, DEC-017, DEC-065; extra fact 7 |
| 8 lossless editor round trip | cov DEC-045; 08-U8 X-DEC-3 |
| 9 editor picking and tools | cov DEC-045 |
| 10 engine thumbnails, no orphan assets | DEC-563 |
| 11 map index packaged with the data | DEC-563; 08-U1 X-DEC-3 |

### 09 (843–851, 2420–2432, 2544–2560)

| Row | Mapping |
|---|---|
| §4.5 map play time against the time limit | cov DEC-067, DEC-243; 09-U3 X-DEC-3 |
| §11.13 call-site defaults | cov DEC-100; extra fact 4 |
| §13 item 1 lobby validation | cov DEC-026, DEC-243 |
| §13 item 2 atomic setup state | cov DEC-026, DEC-243, DEC-067 |
| §13 item 3 key registry | cov DEC-026, DEC-245 constraint 6 |
| §13 item 4 entitlement, quick match, browser | cov DEC-248, DEC-249, DEC-245; extra fact 5 |
| §13 item 5 achievements | cov DEC-047 |
| §13 item 6 hints, explainers, objective cards | cov DEC-037, DEC-050 |
| §13 item 7 tutorial side effects | cov DEC-103 |
| §13 item 8 loading | cov DEC-026, DEC-226 |
| §13 item 9 option defaults and expected options | cov DEC-100, DEC-044; extra fact 5 |
| §13 item 10 localisation | cov DEC-099; glyph cache is ADR scope |
| §13 item 11 entitlement token storage | cov DEC-248, DEC-250 |

No 09 row needed a new entry.

### 10 audio (952–975, 1285–1295, 1333–1339, 1342–1353, 1545–1575, 1580–1598, 1627–1639)

| Row | Mapping |
|---|---|
| §5.2 match end | cov DEC-049, DEC-097 |
| §5.2 crate expiry | DEC-565 |
| §5.2 crate capture tones | cov DEC-097 |
| §5.2 laser create, hit and kill | DEC-564 |
| §5.2 Strike team throw | cov DEC-097 |
| §5.2 turret, Barrier Emitter, Relic, Transport, Leader, creature, Transit Gate, selection, Builder, damage and exit rows | DEC-565 (selection scoping DEC-564) |
| §8.8 listener, distance law, Doppler and occlusion | DEC-566 |
| §8.8 lifetime | cov DEC-097 |
| §8.8 crowd limiting | cov DEC-109 |
| §8.8 game-speed bend | DEC-569 |
| §9.5 voice-budget implications | DEC-570 (recorded as its reference option); stealing cov DEC-097 |
| §10.1 effects gating | DEC-567; the stale effect-handle defect on switching effects off is engine scope (ORIG 27 §12.1, in X-DEC-5b's package), see cross-agent notes |
| §13 #1–#3, #5–#10, #12–#18, #23, #24 | cov DEC-097 (all 24 corrections); also #3 DEC-049, #5 DEC-109, #7 DEC-570, #10 DEC-226, #12 DEC-567, #14 DEC-007, #15 DEC-569; #2, #5, #6, #8, #9, #14, #16, #24 have open questions with X-DEC-3 |
| §13 #4 | cov DEC-072 |
| §13 #11 | cov DEC-015 |
| §13 #19 | cov DEC-097, DEC-045 |
| §13 #20 | cov DEC-042, DEC-097 |
| §13 #21 | cov DEC-052 (Advisor narrator, Dropped) |
| §13 #22 | cov DEC-054 (loader mini-programs, Dropped) |
| §14.1 binding model, layered blocks, generative loops | cov DEC-027 (team-255 rule DEC-097) |
| §14.1 sample groups, linked parameters | DEC-568 |
| §14.1 position types | cov DEC-097 |
| §14.1 monophonic crowd voices | cov DEC-109, DEC-027 |
| §14.1 priority, single master volume | cov DEC-027, DEC-097; budgets DEC-570 |
| §14.1 per-event effects | cov DEC-027; DEC-567 |
| §14.1 music | cov DEC-049 |
| §14.1 volume scale | cov DEC-042 |
| §14.1 game-speed bend | DEC-569 |
| §14.1 render-driven triggers, decay-tone name, match end, silent laser | cov DEC-097, DEC-049; DEC-564 |
| §14.4 mix, priorities and voice budgets | DEC-570 |

### Skipped (open-questions ranges, X-DEC-3)

06 2337–2357, 07 1406–1422, 08 2017–2034, 09 2561–2584, 10 1652–1672. I read them only to deduplicate (rule 3) and wrote no entry for them.

## Extra facts for existing entries (other agents' files, not edited)

1. **DEC-087** names only some bot corrections. Not named there: B5 (a bot's turret goes to the last qualifying own frontier node in structure order, not the most threatened), B6 (the power-placement centroid divides the Worker sum by the count of all friendly actors, so it lands short and can be NaN), B8 (a random walk onto a sea node leaves the army in place), B10 (no graph rebuild when a Barrier Emitter toggles; store crossings independent of its state), B11, B15, B16, B17, B18 (crash), B19 (recall rate scales with array capacity), B23 (Relic un-stick centroid divides by all collisions), B24 (orphan Leader powers accumulate; make them expire). B11's correction **activates** a Siege bot Transport behaviour that never runs in the reference, and B15's correction turns defending formations towards the threat; their threat search is centred about 59 wu ahead along the facing with a radius of 90 wu, so defenders now miss enemies more than about 31 wu behind them. DEC-112 calibration must include Siege and bot defence.
2. **DEC-096 / DEC-202**: the rule "simulation code never issues player commands" (X9) exists only as constraint 5 of the Open, deferred DEC-202. The coordinator may want it stated in a Resolved integrity entry (DEC-007 or DEC-008). DEC-554 lists the input corrections that DEC-096 does not name.
3. **DEC-021**: the starting-powers draw (E6) excludes Allegiance Scrambler, Supply Storm, Reinforcements, Frenzy, Spawn Surge, Sickness, Rocket Troops, Abductor, Swarm, Spitter Plants, Time Rush, Time Dilation, one forest outcome (ORIG gives it only as "Forest", so whether it means Essence Grove or Blight Grove needs checking), the Rift Horror outcome and invalid values. Every starting power is then locked for 30 s with a visible countdown. **DEC-033**: Power-slot capacity in matches ignores research (E4).
4. **DEC-100**: the defaults table must pick one value where call sites disagree: screen refresh 75 or 60; colour depth 32 or 16; water detail none or 1 (with the key missing, the match shows flat water while the options page shows high); pixel-shader, structure-detail and help-enabled keys have no default or 1; sound buffer 512 or 2000; mix frequency 22,050 or 44,100; sound channels 32 or 16 (with the key missing, the Apply button shows as active). The effective reference value is the one the runtime reader uses. Colour depth, buffer size and mix frequency have no Species equivalent.
5. **DEC-044** should also list mouse inversion and subtitles (09 §13 item 9). **DEC-245** should record the recommendation of sortable browser columns and a ping column (09 §13 item 4).
6. **DEC-045 / DEC-015**: one parser reads the map header for menus and for play; the reference has two parsers with different case sensitivity (08 §9 item 1).
7. **DEC-073 / DEC-065**: the reference silently drops a per-map override whose value is 0. A typed override table keeps 0 as a value (08 §9 item 7).
8. **DEC-022 / DEC-025**: new maps need the bot-navigation knobs of the reference maps: node priority modifiers, links placed by hand and cliff checks (07 §20.1 item 2). The map schema should carry them.

## Cross-agent notes (possible overlap)

- **X-DEC-5b** (ORIG 27 §12.1, §12.6, §5, §7, §11.5) may write engine-level entries on the stale effect handles left on voices when effects are switched off, voice stealing, the per-call pitch smoother and the voice leak with no audio device. DEC-567, DEC-569 and DEC-570 are design-level. The coordinator should cross-link them, or merge where the two overlap.
- **X-DEC-5a** (ORIG 22 §14.8.6, NaN producers) may cover the empty crowd order (X23). DEC-555 states the corrected rule for R-CTRL.
- Checked at the end of my run (headings only): X-DEC-5b's draft DEC-751 (the game-speed pitch state starts every match at normal speed) complements DEC-569, and X-DEC-5a's draft DEC-653 (simulation state never holds a non-finite value) is the general rule behind DEC-555. Neither duplicates my entries. The coordinator may cross-cite them once their ids are final.

## Names used that are not ledger entries

All are descriptive phrases, not proposed terms: "duel-vehicle power" (follows DEC-057), "map index" (the list of map ids), "show-cursor control", "Essence-orb rain" (a weather effect), and the three Leader order modes "follow, absorb and none" (plain words). No new term is proposed.

## Dropped content

- E3: two power kinds with empty targeting behaviour, unreachable in a match (DEC-552).
- 06 §12.5: binding lines for an unsupported device, for a non-Windows platform and for gestures (DEC-553, DEC-055).
- 10 §13 #21 and #22: data errors of the narrator and of the loader mini-programs (DEC-052, DEC-054).

## Process notes

- Two early printouts showed a few heading fragments outside my ranges (07 lines 1276–1364; 10 lines 1300–1325, 1603–1616 and 1640–1651). Facts first drawn from 10 §14.6 (the crate-expiry fix and the stress-scene figures) were taken out again; every entry rests on in-range text or on existing entries.
