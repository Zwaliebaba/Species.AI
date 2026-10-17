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
