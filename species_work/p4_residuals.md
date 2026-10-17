# Phase 4 residuals (after wave B)

Items the wave B fixers could not apply, collected for one residual pass after round 2.

| # | From | Item | Needs |
|---|---|---|---|
| 1 | FX-06 | RB-204: camera-view name limit 62 or 63 characters | X-ARB entry from ORIG |
| 2 | FX-06 | RA-315 (build-details part), RB-256: no options page shows build details, so BLD-061 has nowhere to be cited | decide: add a build-details line to the options page (UI-b / G09b) or drop the request |
| 3 | FX-06 | RA-221 partly done: the field and team distributions in reference/08 are stated over 57 files, not the 49 in-scope maps | recount over the 49 maps (X-ARB or FX-06 follow-up) |
| 4 | FX-06 | `screen_flow_and_match_setup.md` is 166 bytes under 115 KiB | split before any further addition |
| 5 | FX-07 | RB-004 items 1 and 2: cliff push and separation push listed by C-SIM-a, contradicted by R-UNIT-041 ("no separation step exists") | X-ARB entry from ORIG |
| 6 | FX-07 | DEC-090 (Resolved: actor stats are floating point, R-SIM-707) against DEC-241 item 2 and R-SIM-145 (unsigned 8-bit stats) | reconcile the two DECs, then align R-SIM-145 or R-SIM-707 |
| 7 | FX-07 | DEC-263 item 1 names `directRoute`; reference/12 05 spells the parameter `_directRoute` | edit DEC-263 |
| 8 | FX-07 | `rename_map.tsv` rows 673–675 give `kMaxTeams`, `kUpdateSlicesPerTick`, `kTickPeriod`; CONVENTIONS and reference/12 use `MAX_TEAMS`, `UPDATE_SLICES_PER_TICK`, `TICK_PERIOD_SECONDS` | update the ledger |
| 9 | FX-07 | camera_and_sequences.md: one snake-case team-command field; CAM-030 to cite DEC-955 | FX-10 (give in its prompt) |
| 10 | FX-01 | RA-040 (general port keys) waits for the paired reference/03 row | check after FX-04 |
| 11 | FX-01 | collector.md must cap its delivery (DEC-080) | verify FX-03's change matches DEC-080 ("waits while the node is capped or activating") |
| 12 | FX-04 | RA-040 pair: reference/03 now defines the general port keys; reference/05 still defines its own (same values, two names) | remove reference/05's port keys and cite reference/03's (residual pass; reference/05 has about 0.6 KB room) |
| 13 | FX-04 | RA-145: no document states how a turret draws its ports | X-ARB entry from ORIG, or a DEC |
| 14 | FX-04 | small corrections in DEC-960, DEC-968, DEC-550 and the asset string table (see reports/FX-04.md) | coordinator |
| 15 | FX-05 | two asset-manifest rows need reference/06 or 07 files as consumers (reports/FX-05.md §11) | X-AST pass 2b |
| 16 | FX-05 | links to the diagnostics document broke when FX-11 split it into a folder | `p4_fix_links.py --apply` after wave B |
| 17 | FX-05 | RA-249 not applied (reference/09 uses `ui.power_slots.map_editor` itself); RB-047 part (particle colour, 1,000 iterations of an out-of-scope effect) not applied | none (closed with reason) |
| 18 | FX-02 | R-CRATE-100 still says a crate drop period of 0 means no Supply crates; use the explicit none value (DEC-120) | coordinator edit in reference/04 |
| 19 | FX-02 | the match message and the Power slots panel's centre message share one layout: share keys or not | coordinator: keep separate keys, each owner's document states its own (no change) |
| 20 | FX-02 | `ref.unit.transport.bot_turret_search_default` (200 wu) is the search radius for a target that is not a Flag post, not a default Flag post size | coordinator edit of the key's notes in reference/02 transport.md |
| 21 | FX-02 | RA-257 (turret frequency 0) and RB-070 (8 px line) not applied, with reasons | closed |
