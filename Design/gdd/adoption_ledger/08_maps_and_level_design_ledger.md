# Adoption ledger: 08 maps and level design

> Status: Draft · Owner: area A1 (B9 for the skirmish maps) · Reference: [08_maps_and_level_design](../../reference/08_maps_and_level_design/README.md) · Species design: [01 world and regions](../01_world_and_regions.md) · Rules: `prompts/species-design-phase.md` §5.3

42 sections, 185 requirements. A section row covers every requirement under its heading; an exception row (scope: one `R-` id) overrides it. Verdicts: `adopt`, `adopt-skirmish`, `adapt` (names the Species requirements that replace it), `drop` (gives the reason).

**How to read this file.** Reference/08 specifies the maps of a match, so almost all of it binds the skirmish only (`adopt-skirmish`): the world game has no map definition, no match setup and no start positions, and it builds its terrain from the terrain component through the world generator (WLD-160 to WLD-167). Three groups change. The decisions of area A1 change some skirmish rules (`adapt` to WLD-260 to WLD-270: no map editor in player builds and a development editor later, DEC-045; a visibility field, DEC-802; build errors for data with no effect, DEC-865; no population cap of 0, DEC-866; lighting and weather presets, DEC-562; unbiased generator variants, DEC-324). The map editor hooks become the design input of the development editor (`adapt` to WLD-260), except the writer and validation rules that bind every map tool (`adopt`). The per-mode map conventions and the figures of the reference map set are `adapt` to PIL-003, as in the ledger of reference/00: B9 sets the skirmish map set, the clauses for Spawn Control bind the skirmish, and the clauses for another mode wait until that mode is scheduled. B9 confirms the skirmish rows or sends A1 a row change.

**Effective verdicts:** adopt 7, adopt-skirmish 70, adapt 108, drop 0, pending 0 (42 section rows, 25 exception rows).

| Scope | Verdict | Species requirements | Note |
|---|---|---|---|
| `08_maps_and_level_design/map_editor_hooks.md §2.1` | adapt | WLD-260, WLD-261 | Scope, reachability and verdict: 4 requirements (R-MAP-300 to R-MAP-303). No map editor in player builds in v1, and no player build loads a map outside its content sets (DEC-045 resolved: text authoring and a Debug-build preview first, a development editor after the first playable); the hooks become the development editor's design input |
| `08_maps_and_level_design/map_editor_hooks.md §2.2` | adapt | WLD-260 | Entry and exit: 7 requirements (R-MAP-305 to R-MAP-311). The reference hub page, pickers, new-map page and editor session are design input, not built as written; the development editor tracks unsaved changes and has undo |
| `08_maps_and_level_design/map_editor_hooks.md §2.3` | adapt | WLD-260 | Modes, selection and tools: 9 requirements (R-MAP-315 to R-MAP-323). Design input; the development editor selects by stable id instead of list index |
| `08_maps_and_level_design/map_editor_hooks.md §2.4` | adapt | WLD-260 | Terrain authoring: 11 requirements (R-MAP-325 to R-MAP-335). Design input; the development editor regenerates on demand and offers the unbiased variants. Exception below |
| `R-MAP-335` | adapt | WLD-268 | The generator's arithmetic and biases are kept exactly; generation method 3 and flatten type 5 are added as unbiased choices for map tools and the world generator (DEC-324, option B) |
| `08_maps_and_level_design/map_editor_hooks.md §2.5` | adapt | WLD-183, WLD-260 | Map lights: 2 requirements (R-MAP-340 to R-MAP-341). Lights come from named presets that a map selects and may override (DEC-562, option B) |
| `08_maps_and_level_design/map_editor_hooks.md §2.6` | adapt | WLD-260 | Structure placement and links: 5 requirements (R-MAP-345 to R-MAP-349). Design input; the development editor adds link removal and per-kind link checks |
| `08_maps_and_level_design/map_editor_hooks.md §2.7` | adapt | WLD-260 | Trees: 2 requirements (R-MAP-355 to R-MAP-356). Design input. Exception below |
| `R-MAP-356` | adopt | | Every map tool draws its randomness from a generator instance of its own (WLD-262 cites it for the world generator too) |
| `08_maps_and_level_design/map_editor_hooks.md §2.8` | adapt | WLD-260 | Starting forces: 3 requirements (R-MAP-360 to R-MAP-362). Design input |
| `08_maps_and_level_design/map_editor_hooks.md §2.9` | adapt | WLD-260, WLD-265 | Routes: 1 requirement (R-MAP-365). A map holds no routes (DEC-865, option B), so no tool authors them |
| `08_maps_and_level_design/map_editor_hooks.md §2.10` | adapt | WLD-260 | Camera views and camera animations: 3 requirements (R-MAP-370 to R-MAP-373). Design input; camera-animation authoring waits for DEC-803 (C1), and the lossless writers keep every camera animation (R-MAP-388) |
| `08_maps_and_level_design/map_editor_hooks.md §2.11` | adapt | WLD-260 | Map options and excluded outcomes: 3 requirements (R-MAP-375 to R-MAP-377). Design input; the fields are those of rules and conventions table 2.6-A as WLD-264 and WLD-266 change it |
| `08_maps_and_level_design/map_editor_hooks.md §2.12` | adapt | WLD-260 | Editor drawing: 1 requirement (R-MAP-380). Design input |
| `08_maps_and_level_design/map_editor_hooks.md §2.13` | adapt | WLD-260, WLD-262 | Writing a map: 6 requirements (R-MAP-385 to R-MAP-390). The reference editor's two writers are not built; map tools write text assets of the map schema family. Exceptions below |
| `R-MAP-388` | adopt | | The lossless writer rules of DEC-800 bind every map tool, the world generator's writer included (WLD-262) |
| `R-MAP-389` | adopt | | The value forms of DEC-120 are kept by every map tool (WLD-262) |
| `R-MAP-390` | adopt | | Each field once, explicit arrays, length limits refused and never truncated (WLD-262) |
| `08_maps_and_level_design/map_editor_hooks.md §2.14` | adapt | WLD-261 | Custom maps and the custom-map container: 4 requirements (R-MAP-391 to R-MAP-394). No container, no player map folder and no custom-map transfer in v1; a Debug build reads plain map definitions from a development folder; player-made maps wait for modding (vision §7) |
| `08_maps_and_level_design/map_editor_hooks.md §2.15` | adopt | | Validation when a map tool saves: 1 requirement (R-MAP-400). Every map tool validates on save with the content build's rules (WLD-262) |
| `08_maps_and_level_design/map_editor_hooks.md §2.16` | adapt | WLD-260 | Defects and gaps of the reference editor: 1 requirement (R-MAP-405). The list is the development editor's checklist: undo, unsaved changes, stable ids, play test, links, regeneration on demand |
| `08_maps_and_level_design/map_editor_hooks.md §2.17` | adapt | WLD-260 | String ids: 1 requirement (R-MAP-410). A player build has no map-editor entry; the development editor's text is new and is specified with it |
| `08_maps_and_level_design/rules_and_conventions.md §2.1` | adopt-skirmish | | Scope and shared conventions: 7 requirements (R-MAP-001 to R-MAP-007). Skirmish maps as written; world templates are their own record type (WLD-167). Exception below |
| `R-MAP-007` | adapt | WLD-263 | New authorship extends to world templates, set pieces and generated worlds |
| `08_maps_and_level_design/rules_and_conventions.md §2.2` | adopt-skirmish | | Map identity and the map catalogue: 8 requirements (R-MAP-010 to R-MAP-017). The tie order is C2's (DEC-314); the custom-map clause of R-MAP-015 is reached only in Debug builds (WLD-261). Exceptions below |
| `R-MAP-013` | adapt | WLD-265 | A mode named twice is a build error (DEC-865 item 1, option B) |
| `R-MAP-014` | adapt | WLD-264 | Listing follows an explicit visibility field, listed or test (DEC-802, option B) |
| `08_maps_and_level_design/rules_and_conventions.md §2.3` | adopt-skirmish | | Map display in match setup: 4 requirements (R-MAP-020 to R-MAP-023) |
| `08_maps_and_level_design/rules_and_conventions.md §2.4` | adopt-skirmish | | The map definition: 5 requirements (R-MAP-030 to R-MAP-034) |
| `08_maps_and_level_design/rules_and_conventions.md §2.5` | adopt-skirmish | | Level load and start positions: 10 requirements (R-MAP-040 to R-MAP-049). A world has no start positions; a site takes their place (WLD-040, PIL-008) |
| `08_maps_and_level_design/rules_and_conventions.md §2.6` | adopt-skirmish | | The match options of the map header: 5 requirements (R-MAP-060 to R-MAP-064). B9 sets each map's cap (PIL-003). Exceptions below |
| `R-MAP-060` | adapt | WLD-264, WLD-266 | The header gains the visibility field, and the population cap loses its 0 form |
| `R-MAP-062` | adapt | WLD-266 | A population cap of 0 is a build error (DEC-866 item 2, option B); the division over open slots, the Siege split and the unlimited form stay as written |
| `08_maps_and_level_design/rules_and_conventions.md §2.7` | adopt-skirmish | | Map options and map effects: 6 requirements (R-MAP-070 to R-MAP-075). Exception below |
| `R-MAP-073` | adapt | WLD-265, WLD-267 | The Essence-orb rain flag is a build error (DEC-865 item 2); the other four effects are grouped into weather presets, and an effect that changes play stays an explicit flag (DEC-562) |
| `08_maps_and_level_design/rules_and_conventions.md §2.8` | adopt-skirmish | | Terrain authoring: 8 requirements (R-MAP-080 to R-MAP-086, R-MAP-198). Skirmish terrain as written; the world's terrain comes from the terrain component through the world generator (WLD-160). Exceptions below |
| `R-MAP-080` | adapt | WLD-265 | A map that is not square is a build error (DEC-865 item 4, option B) |
| `R-MAP-081` | adapt | WLD-268 | Generation method 3, the unbiased copy of a parent, joins methods 0 to 2 (DEC-324) |
| `R-MAP-083` | adapt | WLD-268 | Flatten type 5, the smooth that does not lower, joins types 0 to 4 (DEC-324) |
| `R-MAP-085` | adapt | WLD-162 | The terrain envelope of the reference maps bounds the world generator's parameters, with a larger tile and peak; a skirmish map is not bound to it |
| `R-MAP-086` | adopt | | The terrain colour assets and the seasonal replacement serve the skirmish and the biomes of the world (WLD-180, WLD-185) |
| `R-MAP-198` | adopt | | Sea along the whole border, for skirmish maps and for every world (WLD-001) |
| `08_maps_and_level_design/rules_and_conventions.md §2.9` | adapt | WLD-183, WLD-267 | Map lights: 2 requirements (R-MAP-087 to R-MAP-088). Lighting presets in the key-and-fill pattern, selected by skirmish maps and biomes (DEC-562, option B) |
| `08_maps_and_level_design/rules_and_conventions.md §2.10` | adopt-skirmish | | Map effects in a match: 4 requirements (R-MAP-090 to R-MAP-093). In a world, weather is presentation only and lightning and dust vortices never act (WLD-184). Exception below |
| `R-MAP-092` | adapt | WLD-265 | The Essence-orb rain flag is a build error instead of an effect with no behaviour (DEC-865 item 2) |
| `08_maps_and_level_design/rules_and_conventions.md §2.11` | adopt-skirmish | | Structure placement: 14 requirements (R-MAP-100 to R-MAP-112, R-MAP-200). The level-design patterns stay tools (DEC-315, option A; WLD-269). Exceptions below |
| `R-MAP-110` | adapt | PIL-003 | The structure census of the reference maps describes the reference set; B9's maps carry their own |
| `R-MAP-200` | adapt | PIL-003 | The bot-structure census of the reference maps describes the reference set; the bot rules that read the structures stay with B7 |
| `08_maps_and_level_design/rules_and_conventions.md §2.12` | adapt | WLD-265 | Routes: 2 requirements (R-MAP-120 to R-MAP-121). A block of routes is a build error (DEC-865 item 3, option B); a starting-forces route id still indexes the match's route table |
| `08_maps_and_level_design/rules_and_conventions.md §2.13` | adopt-skirmish | | Camera views and camera animations: 4 requirements (R-MAP-125 to R-MAP-128). A world has per-settlement views instead (WLD-240) |
| `08_maps_and_level_design/rules_and_conventions.md §2.14` | adopt-skirmish | | Starting forces: 5 requirements (R-MAP-130 to R-MAP-134). Exception below |
| `R-MAP-131` | adapt | WLD-265 | A Trooper record whose in-group flag is off is a build error (DEC-865 item 6, option B) |
| `08_maps_and_level_design/rules_and_conventions.md §2.15` | adopt-skirmish | | Fixed research levels and what a map cannot express: 3 requirements (R-MAP-140 to R-MAP-142). A world has no time of day either (WLD-185) |
| `08_maps_and_level_design/rules_and_conventions.md §2.16` | adopt-skirmish | | Spawn Control maps: 3 requirements (R-MAP-150 to R-MAP-152). The map rule of the v1 mode binds the skirmish. Exceptions below |
| `R-MAP-151` | adapt | PIL-003 | The economy variations of the reference Spawn Control maps describe the reference set; B9 sets the Species set |
| `R-MAP-152` | adapt | PIL-003, WLD-269 | Twin terrains are a level-design tool; the reference pairs are not reproduced |
| `08_maps_and_level_design/rules_and_conventions.md §2.17` | adapt | PIL-003 | Zone Control maps: 5 requirements (R-MAP-155 to R-MAP-199). Wait until Zone Control is scheduled; the tutorial maps are B9's (DEC-123) and C5's |
| `08_maps_and_level_design/rules_and_conventions.md §2.18` | adapt | PIL-003 | Relic Run maps: 4 requirements (R-MAP-160 to R-MAP-163). Wait until Relic Run is scheduled |
| `08_maps_and_level_design/rules_and_conventions.md §2.19` | adapt | PIL-003 | Siege maps: 4 requirements (R-MAP-165 to R-MAP-168). Wait until Siege is scheduled |
| `08_maps_and_level_design/rules_and_conventions.md §2.20` | adapt | PIL-003 | Launch Race maps: 3 requirements (R-MAP-170 to R-MAP-172). Wait until Launch Race is scheduled |
| `08_maps_and_level_design/rules_and_conventions.md §2.21` | adapt | PIL-003 | Flag Chain maps: 4 requirements (R-MAP-175 to R-MAP-178). Wait until Flag Chain is scheduled |
| `08_maps_and_level_design/rules_and_conventions.md §2.22` | adapt | PIL-003, WLD-263 | The reference map distribution: 5 requirements (R-MAP-180 to R-MAP-184). B9 sets the map set and its player counts; the `mapfile.*` slots hold it, every map new (DEC-063) |
| `08_maps_and_level_design/rules_and_conventions.md §2.23` | adopt-skirmish | | Supply crate context of the reference maps: 4 requirements (R-MAP-190 to R-MAP-193). The drop cap and the derivation of the pools bind the skirmish. Exceptions below |
| `R-MAP-191` | adapt | PIL-003 | The exclusion zones of the reference maps describe the reference set |
| `R-MAP-192` | adapt | PIL-003 | The pool sizes of the reference maps describe the reference set; B9's maps take theirs by R-MAP-193 |
| `08_maps_and_level_design/rules_and_conventions.md §2.24` | adapt | PIL-003, WLD-266 | Population caps of the reference maps: 1 requirement (R-MAP-195). B9 sets each map's cap; the division over open slots is R-MAP-062's |
| `08_maps_and_level_design/rules_and_conventions.md §2.25` | adapt | PIL-003 | Transit Gate configuration of the reference maps: 1 requirement (R-MAP-197). B9's maps set their own wave values, which R-MAP-071 applies |
