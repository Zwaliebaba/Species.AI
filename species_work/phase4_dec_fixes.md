# Phase 4: decision-register fixes collected from Phase 3 reports

| # | From | DEC | Issue | Proposed action |
|---|---|---|---|---|
| 1 | G05 | DEC-304 | DEC-304 says Spawn Surge affects only the opening team; the source says it affects every team, and G05 follows the source | correct DEC-304 to "every team" |
| 2 | G05 | DEC-065 | with "map values win", a map Transport cap of 0 now means unlimited Transports | new DEC (Open) on what a 0 cap means; default per G05 |
| 3 | G05 | DEC-065 | DEC-065 changes three Flag Chain maps' waves from 40 Workers to 100 (Transport cap 2→3) | record the consequence in DEC-065 |
| 4 | G05 | DEC-002 | tick ranges one tick lower than the replay gives (20–30 vs 21–30) | correct DEC-002 |
| 5 | G05 | DEC-014, DEC-103 | G05 applied them to the empty starting-power draw (grants nothing) and to the tutorial check (player's team, not team 0) | confirm in the DEC text |
| 6 | G05 | DEC-021 | "Forest" in the starting-power exclusions is the **Essence Grove** | tell G04; update DEC-021 |
| 7 | G00 | — | the overview duplicates owner values under `ref.vis.*` keys | replace with references to the owner keys |
| 8 | G05 | — | about 45 values repeated from other documents, marked "owner" | merge into references to the owner keys |
| 9 | C-SIM-b | DEC-016 vs DEC-650 | DEC-016 orders actor-grid results by cell slot, while DEC-650 says the registration schedule has no visible effect; both hold only if the order within a cell is history-free | reconcile: make the order within a cell history-free (sorted by a stable key) in a Resolved DEC |
| 10 | C-SIM-b | new | defaults chosen for gaps: the route query when no corridor test runs; an actor group that reports no positions in a tick; a group attack with no candidates; the obstacle push on a structure's base point; the colour command sent after co-op setup | five new Open DECs, with C-SIM-b's defaults (see reports/C-SIM-b.md) |
| 11 | C-SIM-c | new | how game-speed tick scheduling works at speed 0 (and the other gaps listed in reports/C-SIM-c.md: six requests in total) | new Open DECs with C-SIM-c's defaults |
| 12 | C-SIM-c | DEC-116 | conflict between DEC-116 and the start hold | reconcile in DEC-116 |
| 13 | C-SIM-c | — | the mode-second constant is owned by reference/01 (G01), and 01_tick_timing names it without a key until G01 defines one | Phase 4: link 01_tick_timing to G01's key |
| 14 | C-SIM-a | DEC-011 | **conflict resolved from ORIG:** the stale Source Node id has no effect. It is rewritten at the start of every Source Node advance; the first Spawn Node request comes ≥ 20 ticks in; with no Source Node, a stale id fails the kind check (R-SIM-024) | correct DEC-011's wording to match R-SIM-024 |
| 15 | C-SIM-a | new | multi-round Siege rebuild at a tick boundary; a per-slot generation for laser-bolt and Essence-orb handles (actor serials unchanged); the team selection as a generational handle; where the offset-table step goes | new DECs (see reports/C-SIM-a.md) |
| 16 | C-SIM-a | DEC-016 vs DEC-455 | iteration order | reconcile together with item 9 |
| 17 | C-SIM-a → C-SIM-c | — | order of initialisation draws; which rules read the Control Zone counter | cross-document fix between 02/04 and 07 |
| 18 | owner O14–O16 | DEC-079, 080, 081 | G05 (already written) states the reference rules for the cap, Collector delivery and catch-up | **Phase 4: update `reference/05_population_flow.md`** to the owner-resolved rules (and G00's overview), citing the DECs |
| 19 | owner O18–O19 | DEC-107, 120, 121 | G04, G08a and C-DAT (not started) read them from the register; G01 was told directly | check in Phase 4 |
| 20 | G01 vs C-SIM-b | DEC-078 | the colour of a co-op alliance with no colour: white (R-SIM-421) or opaque grey (G01) | decide from ORIG; fix the losing document |
| 21 | G01/G05 vs C-SIM-b | — | elimination damage: 2 × health **stat** (R-SIM-407) or 2 × **current** health (G01, G05) | decide from ORIG; fix the losing document |
| 22 | G01 | DEC-017 | it says 39 options; 38 remain once the debug-only mode's option is dropped | correct DEC-017; six G01 DEC requests in reports/G01.md |
| 23 | G01 | — | no game-over, victory or defeat sound assets in the manifest | X-AST pass 2 |
| 24 | G01 | — | O20 (first playable) has no DEC entry, so G01 cites DEC-229 | acceptable (DEC-229 is build order) or add an O20 note to DEC-229 |
| 25 | G02a2 | DEC-085 | **contradiction:** G02a2's input says that in a match the Builder delivers its orbs to the nearest friendly Spawn Node; DEC-085, R-ECON-082 and the G00 overview say the orbs reach no destination | verify in ORIG; correct DEC-085, R-ECON-082 and G00 if G02a2 is right |
| 26 | G02a2 | — | five guessed G02a1 keys (`ref.unit.base.corpse_fade_start`, `ref.unit.base.corpse_fade_ground_ticks`, three `ref.unit.worker.transport_*` keys); seven reference/05 mirror keys now point to G02a2 keys | reconcile with G02a1 and G05 |
| 27 | G02a2 | — | 10 DEC requests or clarifications; missing assets: a Builder placement sound and a UI unit-toggle sound; `icon.power.rocket` unused here | DEC triage; X-AST pass 2 |
| 28 | G02a1 vs G02a2 | — | eight stat and slope keys (Builder, Collector, Transport, Trooper) defined in both; values agree; the stat table belongs to G02a1's README | G02a2 cites the README keys; remove its duplicate definitions |
| 29 | G02a1 vs G05 | — | Workers leave a Spawn Node airborne (G05) or spawn on the ground (G02a1) | decide from ORIG; fix the losing document |
| 30 | G02a1 vs G00 | — | Worker task search 10–20 ticks (G00) or 11–20 (G02a1, from the source) | fix G00 |
| 31 | G02a1 | new | Worker rules that divide by game speed under tick scheduling; the fire-damage kill over-count; Trooper per-unit fire rate under floating-point stats; range of the Worker's Hive and Spitter Plant check | four new DECs |
| 32 | G02b | — | G02b split into `creatures.md` and `hazards_and_projectiles.md` | update the package targets, dec_map affects_files, the manifest consumers and the glossary links |
| 33 | G02b | — | the per-kind stat keys, slope keys and `ref.unit.bomber.colour` stay in G02a1's README (G02b cites them) | keep; there are no `ref.unit.base.*` keys, and G02a2's guessed `ref.unit.base.*` keys must be remapped to `ref.unit.corpse.*` and `ref.unit.health_max` |
| 34 | G02b | DEC-014 | search caps of 256 and 2000 attempts are G02b's own values, sized from DEC-014's acceptance rates | confirm in DEC-014, or flag as provisional |
| 35 | G02b | new | seven DEC requests (the Crawler's 20-per-tick bite, the Abductor's double time step, others); missing assets: the Abductor beam ring and the Reaper Serpent eaten-orb card | DEC triage; X-AST pass 2 |
| 36 | G02b split | — | links to `creatures_and_hazards.md` remain in decisions, the glossary, G00, `essence_orbs.md`, reference/12 and the manifest | global link update to the two new files |
| 37 | C-RND-a | ADR-0011 vs DEC-326 | the terrain grid is built from triangle edges (ADR-0011) or a grid texture (DEC-326 default); C-RND-a made both a shader option, with the texture as default | amend ADR-0011 |
| 38 | C-RND-a | ADR-0005 | no blend state for the cursor's ghost models | amend the PSO catalogue |
| 39 | C-RND-a | ADR-0018 | the fixed sampler set has no point-wrap sampler for the clouds | amend ADR-0018 |
| 40 | C-RND-a | ADR-0003, ADR-0013 | specular lighting for the terrain grid and the special lighting is not mentioned | amend the ADRs |
| 41 | C-RND-a | new | five DEC requests (team slot 5 never shows ownership lights; poster screenshots use a different FOV); two manifest rows need corrections | DEC triage; X-AST pass 2 |
| 42 | G03a vs G01 | — | **Core Device countdown: 900 (G03a) or 1,200 (siege.md)** | decide from ORIG; fix the losing document |
| 43 | G03a | DEC-459 vs DEC-561 | overlap (Open against Resolved) | supersede or cross-cite; 7 DEC requests (mostly unstated tie rules) in reports/G03a.md |
| 44 | G03a | — | missing asset: the Launch Vessel fuelling-loop sample group; wrong role: `mesh.structure.structure_default` is the port pad; fuel-chain consumer paths need correcting | X-AST pass 2 |
