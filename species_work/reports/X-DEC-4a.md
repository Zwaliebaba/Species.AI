# X-DEC-4a report: decisions for units, structures, crates and population flow

> Phase 2, wave 2b · Package `species_work/packages/X-DEC-4a.json` · ID range DEC-450 to DEC-549 (used: DEC-450 to DEC-466)

## 1 Files written

| File | Content |
|---|---|
| `design_next/decisions/units_buildings_crates_economy.md` | heading, scope paragraph, index table, 17 entries in the §6.5 format (25.6 KB) |
| `species_work/dec_map_X-DEC-4a.csv` | `dec_id, title, status, affects_prefix, affects_files, orig_refs`; 17 rows; csv module, `lineterminator='\n'` |
| `species_work/scripts/X-DEC-4a_build.py` | regenerates the index table and the CSV from the register file (idempotent; checks ids, range, status and affects) |
| `species_work/reports/X-DEC-4a.md` | this report |

No publishing tool and no git write command was used.

## 2 Counts

| Status | Count | Entries |
|---|---|---|
| Resolved | 8 | DEC-456, DEC-457, DEC-458, DEC-460, DEC-461, DEC-463, DEC-464, DEC-465 |
| Open | 8 | DEC-450, DEC-451, DEC-452, DEC-453, DEC-454, DEC-455, DEC-459, DEC-462 |
| Dropped | 1 | DEC-466 |
| **Total** | **17** | |

Resolved entries apply an ORIG fix recommendation (DEC-456 to DEC-458, DEC-460, DEC-461, DEC-463, DEC-464) or are integrity and crash defects (DEC-456, DEC-457, DEC-465). Every Open entry states the reference default and a recommendation, with "Decide by".

Most body items were already decided by wave 2a: of about 130 items in the 19 body rows, 17 new entries were needed.

## 3 Input rows and their disposition

Lines are ORIG line ranges of the package rows. "covered by" names the deciding entry in `verdicts_and_fixes.md` (DEC-0nn, DEC-1nn) or `decisions/deferred/` (DEC-2nn).

### 3.1 `00_overview.md`

| Row | Item | Disposition |
|---|---|---|
| 280–291 | Q1 11-tick mode second | covered by DEC-002, DEC-003 |
| 280–291 | Q2 Comeback Bonus on by default; absent option reads −1 = true | covered by DEC-073 |
| 280–291 | Q3 per-team cap from the map's player count | covered by DEC-079 |
| 280–291 | Q4 Collector bypasses the cap | covered by DEC-080 |
| 280–291 | Q4 the other uncapped sources and the three enforcement points | **DEC-453** |
| 280–291 | Q5 binary catch-up bonus | covered by DEC-081 |
| 280–291 | Q6 Rogue and Visitor factions in win checks | covered by DEC-082 |
| 296–303 | Q11 six Power slots, lost grants | covered by DEC-033; clickable bar and wide ids covered by DEC-083 |
| 296–303 | Q12 unbounded rejection loops | covered by DEC-014 |
| 296–303 | Q13 doubled reinforcement waves on three Flag Chain maps | covered by DEC-065 |
| 296–303 | Q14 presentation inside the simulation step; tree hit volumes | covered by DEC-007, DEC-012 |
| 296–303 | Q15 hook-only features | covered by DEC-047 (achievements, leaderboards), DEC-049 (music), DEC-066 (multi-round Siege) |
| 463–479 | open questions 00-U1 to 00-U10 | skipped (open questions) |

### 3.2 `01_game_modes.md`

| Row | Item | Disposition |
|---|---|---|
| 32–46 | mode second of 11 ticks; tick counts of N-second timers | covered by DEC-002 |
| 32–46 | recount period of 10 or 11 ticks | covered by DEC-064 |
| 32–46 | Rogue and Visitor factions present in every match with a bot controller | covered by DEC-022 (fact) |
| 32–46 | map positions remapped to teams | covered by DEC-025 |
| 182–282 | B10 unbounded bot colour draw | covered by DEC-014 |
| 182–282 | start position drawn lazily on first query; bot and unused-slot colours; unused slots can share a colour | **DEC-450** |
| 182–282 | co-op fallback steps that never apply; ungrouped slots and the two factions as co-op friends | **DEC-451** |
| 182–282 | no-team group colour is black at alpha 1/255 | covered by DEC-078 |
| 182–282 | spectator slots usable only by the soak test | covered by DEC-050 |
| 301–348 | binary catch-up bonus, no effect in Siege | covered by DEC-081 |
| 301–348 | absent option −1 = true, Comeback Bonus runs in every mode | covered by DEC-073 |
| 301–348 | comeback grant skips validity checks and the map's excluded outcomes | **DEC-452** |
| 301–348 | soft elimination reading a stale Visitor Spawn Node count | covered by DEC-011 |
| 630–652 | the "−1 = absent" rule and its read sites | covered by DEC-073 (extra facts in §4) |
| 666–668 | map play time against the default time limit | covered by DEC-067 |
| 907–967 | B5 multi-round Siege | covered by DEC-066 |
| 907–967 | B19 Command Console capture after detonation | covered by DEC-075 |
| 907–967 | B6 wall clock in the defender gate | covered by DEC-009 |
| 907–967 | single-round winner is the first slot of the side; Command Console ownership tie | covered by DEC-016 (extra facts in §4) |
| 1225–1243 | B1 / B2 / B3 / B4 / B5 | covered by DEC-002 and DEC-003 / DEC-064 / DEC-065 / DEC-011 / DEC-066 |
| 1225–1243 | B6 / B7 / B8 / B9 / B10 | covered by DEC-009 / DEC-047 / DEC-067 / DEC-047 / DEC-014 |
| 1225–1243 | B11 / B12 / B13 / B14 / B15 | covered by DEC-068 / DEC-069 / DEC-070 / DEC-071 / DEC-072 |
| 1245–1256 | B17 / B18 / B19 / B20 / B21 | covered by DEC-073 / DEC-074 / DEC-075 / DEC-076 / DEC-077 |
| 1245–1256 | B22 / B23 / B24 / B25 | covered by DEC-078 / DEC-078 / DEC-048 / DEC-007 |
| 1257–1270 | notes 1 ruleset / 2 one clock / 3 explicit ties | covered by DEC-017 / DEC-002 / DEC-016 (extra facts in §4) |
| 1257–1270 | notes 4 per-match state / 5 special factions / 6 authority / 7 Siege rotation | covered by DEC-011 and DEC-013 / DEC-082 / DEC-243 and DEC-067 / DEC-066 |
| 1367–1385 | open questions 01-U1 to 01-U12 | skipped (open questions) |

### 3.3 `02_units_and_entities.md`

| Row | Item | Disposition |
|---|---|---|
| 2863–2887 | open questions 02-U1 to 02-U18 | skipped (open questions) |

### 3.4 `03_buildings_and_world_objects.md` (§15, items 1–36)

| Item | Disposition |
|---|---|
| 1 smoke-column obstacle | covered by DEC-089 |
| 2 tree hit spheres from rendering and the presentation generator | covered by DEC-012 |
| 3 empty Solar Array re-announces its team every tick | covered by DEC-117 (note in §4) |
| 4 port occupancy not bound to (structure, port) | covered by DEC-088 |
| 5 spawn-network walk without cycle detection; one orb on several edges | **DEC-456** |
| 6 Source Node id never reset | covered by DEC-011 |
| 7 Control Zone counter survives between matches | covered by DEC-011 |
| 8 local switch that toggles a Barrier Emitter | covered by DEC-020 (energy is the only switch) and DEC-200 (lesson 6) |
| 9 toggle command with no sender | covered by DEC-020 |
| 10 Barrier Emitter rendering writes a link field | covered by DEC-007 |
| 11 Barrier Emitter lethal area uses one post's height | covered by DEC-088 |
| 12 Command Console relink leaks ports and re-runs initialisation | **DEC-458** |
| 13 Core Device capture check after detonation, double record | covered by DEC-075 |
| 13 Core Device with no linked Command Console destroyed at once | **DEC-459** |
| 14 unchecked energy-grid link conversion | **DEC-457** |
| 15 Launch Vessel damaged in Flight | covered by DEC-076 |
| 16 Hive and Spitter Plant attacks pass an obstacle index | covered by DEC-105 |
| 17 Worker grenades at structures thrown as team 1 | covered by DEC-105 |
| 18 turret kill sound bound under a different event name | covered by DEC-027 (event list validated) and DEC-097 |
| 19 turret cosmetic smoke and mortar scatter draw from the simulation generator | covered by DEC-013 |
| 20 fuel pump surge limit 20 in the simulation, 15 in rendering | **DEC-463** |
| 21 board-game prototype structure | **DEC-466** |
| 22 Allegiance Scrambler may pick an unused slot | covered by X-DEC-3 (03-U10, 04-U5) |
| 23 teleport table is process-static | covered by DEC-011 and DEC-223 (item 2) |
| 24 recount timers of 10 or 11 ticks | covered by DEC-064 |
| 25 per-tick probabilities and growth ignore game speed | covered by DEC-092 (extra facts in §4) |
| 26 map template structures fire team-change sounds at load | covered by DEC-088 |
| 27 a burning empty Hive releases an ant | **DEC-460** |
| 28 carryable-structure collision list is a static array of 32 | covered by DEC-028 (extra fact in §4) |
| 29 Relic billboard variant references a missing mesh | covered by DEC-015 (missing asset is a build error) and DEC-029 (all art new) |
| 30 Control Zone bot-target field and the wall objective reference unused | **DEC-466** |
| 31 turret ignores the map facing | **DEC-462** |
| 32 null dereferences and division by zero in campaign-only structures | **DEC-466** (with DEC-052) |
| 33 rendering changes orientation; unsynchronised draws in campaign-only constructors | **DEC-466** (with DEC-052) |
| 34 turret line of sight sampled behind the target | covered by DEC-088 |
| 35 turret sockets taken from the gun-turret mesh; casing draws on every shot | **DEC-461** |
| 36 neutral-start Siege Solar Array knocked out by its first capture | **DEC-464** (the intent of the knock-out itself is 03-U16, X-DEC-3) |
| open questions 03-U1 to 03-U17 (lines 2643–2664) | skipped (open questions) |

### 3.5 `04_crates_powerups_and_weapons.md`

| Row | Item | Disposition |
|---|---|---|
| 1456–1469 | shared simulation stream; explicit sequencing of draws | covered by DEC-013 (and DEC-242) |
| 1456–1469 | variable-length random consumption of rejection loops | covered by DEC-013, DEC-014 |
| 1456–1469 | unbounded loops | covered by DEC-014 |
| 1456–1469 | simulation draws spent on cosmetics; unsynchronised cosmetic draws | covered by DEC-013 |
| 1456–1469 | render code writing object state | covered by DEC-007 (extra fact in §4) |
| 1456–1469 | per-tick rules that ignore game speed; speed hold counters | covered by DEC-092 and DEC-039 (real-time durations: X-DEC-3, 04-U8) |
| 1456–1469 | effects created mid-tick advance in the same tick by slot position | **DEC-455** |
| 1456–1469 | not-a-number weight in score-weighted drops at a zero total | **DEC-465** |
| 1456–1469 | truncated trigonometry in simulation paths | covered by DEC-241 (extra fact in §4) |
| 1470–1486 | open questions 04-U1 to 04-U12 | skipped (open questions) |

### 3.6 `05_economy_and_population.md`

| Row | Item | Disposition |
|---|---|---|
| 221–243 | binary catch-up bonus, dilution by the two factions, inert while any Spawn Node is neutral | covered by DEC-081 (extra facts in §4) |
| 324–341 | cap = map cap ÷ map player count, integer division, never grows | covered by DEC-079 |
| 324–341 | Siege defender and attacker caps | covered by DEC-079 (extra fact in §4) |
| 324–341 | Population Limiter fallback, first-in-order lookup, id-0 exception | **DEC-454** |
| 324–341 | Hive sortie cap does not see the Hive's own ants | covered by DEC-084 |
| 324–341 | the last population-cap line of a map wins | covered by DEC-015 (extra fact in §4) |
| 324–341 | second-tutorial Spawn Node locks | covered by DEC-103, DEC-110 |
| 324–341 | no global actor limit | **DEC-453** |
| 450–462 | two uncapped co-op maps; caps per team, not per alliance; divisor | covered by DEC-079 |
| 450–462 | bot controller counts against the cap | covered by DEC-079 |
| 450–462 | Rogue faction: only Hives capped; its deaths leave no Essence orbs | covered by DEC-084 (extra fact in §4) and DEC-453 |
| 450–462 | Visitor Spawn Node count never reset | covered by DEC-011 |
| 450–462 | two population-cap lines on one map | covered by DEC-015 |
| 450–462 | Transport outcome invalid with Transport waves and in Launch Race | reference rule, no decision needed (DEC-021) |
| 620–651 | Transit Gates advance twice per tick on three Flag Chain maps | covered by DEC-065 |
| 620–651 | per-map wave table (maps, including test and cut maps) | covered by DEC-063 (map identities are not reproduced) |
| 772–841 | §8.1 slice model, each object once per tick | covered by DEC-028, DEC-117 |
| 772–841 | §8.2 nominal and actual tick counts | covered by DEC-002 (extra facts in §4) |
| 772–841 | §8.3 speed crates; per-tick behaviours that do not scale | covered by DEC-039, DEC-092 (extra facts in §4) |
| 772–841 | E1, E2, E3 counters that survive between matches | covered by DEC-011 |
| 772–841 | E4 float accumulation in countdowns | covered by DEC-002 |
| 772–841 | E5 container order: same-tick hand-offs, waves against the cap, Workers spawned into later slots | **DEC-455** |
| 772–841 | E6 random consumption scales with live orbs and spawns | covered by DEC-013 |
| 772–841 | E7 integer divisions | covered by DEC-241, DEC-079, DEC-081 |
| 772–841 | E8 Collector release and Builder drop loops spin on an invalid entry | covered by DEC-014 (no unbounded loop); reachability is 05-U8 and 02-U14 (X-DEC-3) — see §5 |
| 902–913 | notes 1 integer ticks / 2 catch-up intent | covered by DEC-002 and DEC-003 / DEC-081 |
| 902–913 | note 3 cap from teams in play, cap on arrival and per reinforcement unit, harvester and crates | covered by DEC-079, DEC-080; **DEC-453** |
| 902–913 | note 4 per-match state / 5 territory model / 6 essence economy | covered by DEC-011 / DEC-018 and DEC-094 / DEC-080 and DEC-085 |
| 914–934 | open questions 05-U1 to 05-U12 | skipped (open questions) |

## 4 Extra facts for existing entries (for the owning agents; no other file was edited)

- **DEC-002** (tick counts not yet listed): burst pause 101–150 ticks (mean 125.5); first production timer 20–40; first ownership evaluation 1–20, later evaluations 20–30; reinforcement wave every 660 ticks with a first gap of 649; Spawn Control elimination 330; Comeback Bonus first grant 51 ticks for a team already eliminated on the first post-grace tick; Spawn Surge about 50–51 ticks; Essence orb floating 300–500, death ascent 901; Collector and Builder idle retarget 11; bot Collector decision 11; Worker idle retarget 11–20; corpse fade 25 (ground) and 50 (air); Population Limiter recount 90–110; Hive orb-to-ant conversion 10–11; Crawler egg hatch 151; Collector delivery one orb per tick. The three mode-second timers start at 0, so the first mode second ends on the first post-grace tick.
- **DEC-007**: further render paths that write object state: Supply crate orientation and its bubble timer (per rendered frame), thrown-weapon orientation and its flash sound, the shell-casing facing that its advance reads, and the Barrier Emitter link reset.
- **DEC-011**: the Source Node registry id is stale until the new map's Source Node first advances, and production requests in that window are dropped; the teleport table is process-static (also DEC-223 item 2).
- **DEC-014**: the Collector release loop and the Builder drop loop end only when every carried entry is valid, so they must also remove stale entries; the spawn-network link walk is bounded by DEC-456.
- **DEC-015**: a map that lists the population cap twice takes the last line in the reference; a duplicate key should be a build error.
- **DEC-016**: further order-decided ties: the single-round Siege winner is the first slot of the winning side; the Relic Run co-op winner is the last slot of the group; the Launch Vessel lookup takes the last matching structure; Command Console ownership goes to the first qualifying team in slot order and changes only to a team with strictly more operators.
- **DEC-028**: the carryable-structure collision list is a static array of 32 entries; per-call storage must also have no fixed limit.
- **DEC-033 / DEC-073**: where a mode has the turret-cap option, a placed turret's power keeps its Power slot while the turret exists and counts towards the cap; without the option the power ends at placement.
- **DEC-073**: the typed defaults must reproduce each absent-option effect: no time limit; sudden death off; reinforcement interval 60 mode seconds; reinforcement count and Transport cap from the map (no Transports when the map sets none); no reinforcement turrets; turret cap 2; no crate drops; random crate placement; catch-up multiplier 1; no starting powers. The Comeback Bonus runs but never grants in Siege and Launch Race, because no team counts as eliminated there.
- **DEC-079**: Siege caps: each defending team has the defender cap, and the attackers share (map cap − defenders × defender cap) ÷ attackers, by integer division; two Siege maps set no defender cap and use map cap ÷ player count for both sides. Because empty slots below the player count are filled with bots at match start, the cap equals map cap ÷ teams in play at start, and only later departures leave shares unused.
- **DEC-081**: the multiplier is 1 while any Spawn Node on the map is neutral, when the map disables it, and 0 for the no-team; allies count as other teams; the average uses integer division. It can help only a team holding exactly one Spawn Node, and only on maps with at least 7 (two players), 9 (three) or 11 (four) Spawn Nodes. Burst size int(20 × H) is 20, 22, … 38 for settings 0 to 90 %. Only Spawn Control and Zone Control can use it (the option is absent in Relic Run, Launch Race and Flag Chain, and Siege maps have no Spawn Nodes).
- **DEC-084**: every death of a Rogue faction actor leaves no Essence orb, not only a destroyed Hive; a Hive's sortie check sees only the Rogue faction's other bodies (its bot controller, crate creatures and eggs); one ant per 0.2 s.
- **DEC-092**: further per-tick rules: volcano vent eruption, tree drops, Spitter Plant shrink; Collector delivery (one orb per tick), Spawn Surge requests (one per tick), Launch Vessel passenger release, Relay overshoot; the game-speed hold counters. Under DEC-039 every rule scales the same way.
- **DEC-117**: the empty Solar Array's per-tick team change comes from comparing two different no-team values; with one no-team value (as in DEC-464) the change disappears altogether, so a Species implementation need not reproduce it.
- **DEC-241**: truncated trigonometry also drives the Supply crate sway and the dust vortex in simulation paths.
- **DEC-018**: reference/05 should state explicitly how allied operators on a Spawn Node's ports count (05 §10 note 5).

## 5 Overlaps and notes for the coordinator

- **DEC-455** is a general rule. X-DEC-3 may write entries for its instances (04-U11 rocket move count, 05-U2 network latency, 05-U11 Spawn Surge count), and X-DEC-5a may cover the update order of reference/12. Please merge or cross-reference them at G2.
- **05 E8** is a potential hang (crash class). It is covered by DEC-014 here; if X-DEC-3 writes an entry for 05-U8 or 02-U14, it should be Resolved.
- **DEC-452** follows the recommendation of DEC-107 for the map's exclusion list.
- **DEC-466** groups the defects of content that no match places; the campaign itself is DEC-052.
- Open-question ranges hold three passages that are not numbered questions: the "resolved during synthesis" paragraph after 01-U12; the "resolved in this document" and "contradictions resolved" paragraphs after 05-U12. They hold settled facts for the body rewrites (G01, G05), not questions. Several numbered rows are marked resolved rather than open: 01-U8; 02-U3; 03-U1, U2, U4 to U7 and U15; 04-U1, U2, U6 and U12; 05-U1.

## 6 Requests

- DEC requests: none beyond the entries written.
- ADR requests: none.
- Cross-document requests: G01, G03a, G03b, G04, G05 and C-SIM-a cite the new entries in their §8 "Open decisions" (see each entry's Affects line).
- Names not in the ledger: none needed. DEC-466 describes campaign-only and prototype structures generically (board-game prototype, power station, mine-track terminus, scoring hub), because the ledger gives them no Species name.
- Dropped as legacy: DEC-466 (defects in structures no match places, and two unused fields).

## 7 Gates

- `scan_patterns.py` with `forbidden_patterns.tsv` on the register file: **0 hits**.
- `scan_patterns.py` with `soft_patterns.tsv` on the register file: **0 hits**; nothing to justify.
- The CSV's `orig_refs` column names ORIG files by design (the package requires `file:start-end`), so the forbidden scan reports those file names as provenance hits, as for the wave-2a maps. The CSV is a WORK file and holds no other hit.
