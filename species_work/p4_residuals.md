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
