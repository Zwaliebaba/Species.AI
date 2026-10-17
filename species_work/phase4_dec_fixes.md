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
