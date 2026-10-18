# Adoption ledger: 08 maps and level design

> Status: Draft · Owner: area A1 · Reference: [08_maps_and_level_design](../../reference/08_maps_and_level_design/README.md) · Rules: `prompts/species-design-phase.md` §5.3

42 sections, 185 requirements. A section row covers every requirement under its heading; an exception row (scope: one `R-` id) overrides it. Verdicts: `adopt`, `adopt-skirmish`, `adapt` (names the Species requirements that replace it), `drop` (gives the reason).

| Scope | Verdict | Species requirements | Note |
|---|---|---|---|
| `08_maps_and_level_design/map_editor_hooks.md §2.1` | pending | | Scope, reachability and verdict: 4 requirements (R-MAP-300 to R-MAP-303) |
| `08_maps_and_level_design/map_editor_hooks.md §2.2` | pending | | Entry and exit: 7 requirements (R-MAP-305 to R-MAP-311) |
| `08_maps_and_level_design/map_editor_hooks.md §2.3` | pending | | Modes, selection and tools: 9 requirements (R-MAP-315 to R-MAP-323) |
| `08_maps_and_level_design/map_editor_hooks.md §2.4` | pending | | Terrain authoring: 11 requirements (R-MAP-325 to R-MAP-335) |
| `08_maps_and_level_design/map_editor_hooks.md §2.5` | pending | | Map lights: 2 requirements (R-MAP-340 to R-MAP-341) |
| `08_maps_and_level_design/map_editor_hooks.md §2.6` | pending | | Structure placement and links: 5 requirements (R-MAP-345 to R-MAP-349) |
| `08_maps_and_level_design/map_editor_hooks.md §2.7` | pending | | Trees: 2 requirements (R-MAP-355 to R-MAP-356) |
| `08_maps_and_level_design/map_editor_hooks.md §2.8` | pending | | Starting forces: 3 requirements (R-MAP-360 to R-MAP-362) |
| `08_maps_and_level_design/map_editor_hooks.md §2.9` | pending | | Routes: 1 requirement (R-MAP-365) |
| `08_maps_and_level_design/map_editor_hooks.md §2.10` | pending | | Camera views and camera animations: 3 requirements (R-MAP-370 to R-MAP-373) |
| `08_maps_and_level_design/map_editor_hooks.md §2.11` | pending | | Map options and excluded outcomes: 3 requirements (R-MAP-375 to R-MAP-377) |
| `08_maps_and_level_design/map_editor_hooks.md §2.12` | pending | | Editor drawing: 1 requirement (R-MAP-380) |
| `08_maps_and_level_design/map_editor_hooks.md §2.13` | pending | | Writing a map: 6 requirements (R-MAP-385 to R-MAP-390) |
| `08_maps_and_level_design/map_editor_hooks.md §2.14` | pending | | Custom maps and the custom-map container: 4 requirements (R-MAP-391 to R-MAP-394) |
| `08_maps_and_level_design/map_editor_hooks.md §2.15` | pending | | Validation when a map tool saves: 1 requirement (R-MAP-400) |
| `08_maps_and_level_design/map_editor_hooks.md §2.16` | pending | | Defects and gaps of the reference editor: 1 requirement (R-MAP-405) |
| `08_maps_and_level_design/map_editor_hooks.md §2.17` | pending | | String ids: 1 requirement (R-MAP-410) |
| `08_maps_and_level_design/rules_and_conventions.md §2.1` | pending | | Scope and shared conventions: 7 requirements (R-MAP-001 to R-MAP-007) |
| `08_maps_and_level_design/rules_and_conventions.md §2.2` | pending | | Map identity and the map catalogue: 8 requirements (R-MAP-010 to R-MAP-017) |
| `08_maps_and_level_design/rules_and_conventions.md §2.3` | pending | | Map display in match setup: 4 requirements (R-MAP-020 to R-MAP-023) |
| `08_maps_and_level_design/rules_and_conventions.md §2.4` | pending | | The map definition: 5 requirements (R-MAP-030 to R-MAP-034) |
| `08_maps_and_level_design/rules_and_conventions.md §2.5` | pending | | Level load and start positions: 10 requirements (R-MAP-040 to R-MAP-049) |
| `08_maps_and_level_design/rules_and_conventions.md §2.6` | pending | | The match options of the map header: 5 requirements (R-MAP-060 to R-MAP-064) |
| `08_maps_and_level_design/rules_and_conventions.md §2.7` | pending | | Map options and map effects: 6 requirements (R-MAP-070 to R-MAP-075) |
| `08_maps_and_level_design/rules_and_conventions.md §2.8` | pending | | Terrain authoring: 8 requirements (R-MAP-080 to R-MAP-086) |
| `08_maps_and_level_design/rules_and_conventions.md §2.9` | pending | | Map lights: 2 requirements (R-MAP-087 to R-MAP-088) |
| `08_maps_and_level_design/rules_and_conventions.md §2.10` | pending | | Map effects in a match: 4 requirements (R-MAP-090 to R-MAP-093) |
| `08_maps_and_level_design/rules_and_conventions.md §2.11` | pending | | Structure placement: 14 requirements (R-MAP-100 to R-MAP-112) |
| `08_maps_and_level_design/rules_and_conventions.md §2.12` | pending | | Routes: 2 requirements (R-MAP-120 to R-MAP-121) |
| `08_maps_and_level_design/rules_and_conventions.md §2.13` | pending | | Camera views and camera animations: 4 requirements (R-MAP-125 to R-MAP-128) |
| `08_maps_and_level_design/rules_and_conventions.md §2.14` | pending | | Starting forces: 5 requirements (R-MAP-130 to R-MAP-134) |
| `08_maps_and_level_design/rules_and_conventions.md §2.15` | pending | | Fixed research levels and what a map cannot express: 3 requirements (R-MAP-140 to R-MAP-142) |
| `08_maps_and_level_design/rules_and_conventions.md §2.16` | pending | | Spawn Control maps: 3 requirements (R-MAP-150 to R-MAP-152) |
| `08_maps_and_level_design/rules_and_conventions.md §2.17` | pending | | Zone Control maps: 5 requirements (R-MAP-155 to R-MAP-199) |
| `08_maps_and_level_design/rules_and_conventions.md §2.18` | pending | | Relic Run maps: 4 requirements (R-MAP-160 to R-MAP-163) |
| `08_maps_and_level_design/rules_and_conventions.md §2.19` | pending | | Siege maps: 4 requirements (R-MAP-165 to R-MAP-168) |
| `08_maps_and_level_design/rules_and_conventions.md §2.20` | pending | | Launch Race maps: 3 requirements (R-MAP-170 to R-MAP-172) |
| `08_maps_and_level_design/rules_and_conventions.md §2.21` | pending | | Flag Chain maps: 4 requirements (R-MAP-175 to R-MAP-178) |
| `08_maps_and_level_design/rules_and_conventions.md §2.22` | pending | | The reference map distribution: 5 requirements (R-MAP-180 to R-MAP-184) |
| `08_maps_and_level_design/rules_and_conventions.md §2.23` | pending | | Supply crate context of the reference maps: 4 requirements (R-MAP-190 to R-MAP-193) |
| `08_maps_and_level_design/rules_and_conventions.md §2.24` | pending | | Population caps of the reference maps: 1 requirement (R-MAP-195) |
| `08_maps_and_level_design/rules_and_conventions.md §2.25` | pending | | Transit Gate configuration of the reference maps: 1 requirement (R-MAP-197) |
