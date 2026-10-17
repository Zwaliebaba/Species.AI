# X-GLO-1 report: glossary A–L and glossary index

## Files written

| File | Size | Content |
|---|---|---|
| `design_next/glossary/a_to_l.md` | 48,050 bytes | 180 entries: "Numbers" (terms starting with a digit) and letters A–L |
| `design_next/glossary/README.md` | 3,371 bytes | Folder index (links `a_to_l.md` and `m_to_z.md`), entry columns, the four kinds, reading conventions (order, units, numbers, cross-references, identifiers, player-facing text) |
| `species_work/scripts/X-GLO-1_owners.py` | helper | Maps each ORIG glossary row's primary link to the Phase 3 package input and target file that covers it |
| `species_work/scripts/X-GLO-1_check.py` | helper | Order per section, duplicates, kinds, link targets (existing file or INDEX target), DEC and ADR existence, truncation words, size |

Entry format (set in README.md, for X-GLO-2 too): one table per letter with columns `Term | Kind | Definition | Owning document`; kinds are `player concept`, `engine term`, `data term`, `abbreviation`.

## Counts

- Requirements: 0 (glossary; no ID range). Tuning keys: 0 (no tuning table). Diagrams: 0.
- Entries: 180 = 95 player concepts, 60 engine terms, 7 data terms, 18 abbreviations.
- Per section: Numbers 3, A 28, B 22, C 35, D 8, E 12, F 25, G 14, H 9, I 6, J 2, K 1, L 15.
- Enumerations carried in definitions: the 12 achievements; the 7 basic outcomes; the 3 bot difficulties; the input source node types (9); the actor-group members (5); the effect members (6); the energy-grid structures (4); the fuel chain (6 hops); the front-end screens (6); the capture periods per capturable kind (4); the subsystems the application owns (5).
- Owning documents linked: 47 distinct targets, every one a package target in `species_work/packages/INDEX.md` (or `assets/README.md`, which exists).

## Checks

- `scan_patterns.py forbidden_patterns.tsv` on both files: no output.
- `scan_patterns.py soft_patterns.tsv` on both files: no output (one hit fixed, see cross-document request 4).
- `X-GLO-1_check.py`: 0 problems (alphabetical order word by word, hyphen read as a space, qualifiers in brackets ignored; no duplicates; all link targets are INDEX targets or existing files; all DEC and ADR ids exist; no truncation words; sizes under 120 KB).
- No original term, mapping or code identifier appears; a case-insensitive sweep for the renamed original vocabulary finds only the Species document titles "Entity model and identity" and "Buildings and world objects".

## DECs and ADRs cited

- Open: DEC-047 (achievements), DEC-081 (Catch-up bonus curve), DEC-452 (Comeback Bonus pool and excluded outcomes).
- Resolved: DEC-023 (control scheme kept), DEC-041 (diagnostics tooling, cheats). Dropped: DEC-056 (alternative control scheme).
- ADRs: ADR-0001, ADR-0003, ADR-0022, ADR-0025, ADR-0034, ADR-0058, ADR-0059, ADR-0065, ADR-0074.
- `decs_to_cite` was empty.

## New DEC requests

None.

## ADR conflicts

None. Definitions of engine terms (asset cache, audio decoder, audio engine, glyph renderer, crash reporter, black box, content build, game time, D3D12, HLSL) follow the ADRs named in them.

## Missing assets

None; the glossary cites no asset ids.

## Cross-document requests

1. **To X-GLO-2: terms from the A–L source range whose Species name starts with M–Z.** ORIG `90_glossary.md` line → Species term → owning document:
   - 106 → Massacre, Stockpiler, Total Control, Tourist (achievements; unlock conditions are ORIG 09 §7.1 rows 8, 1, 0, 7) → `reference/09_front_end_and_match_flow/options_localisation_and_hooks.md`
   - 129 → presentation random integer, presentation random fraction, presentation random signed fraction, presentation generator seed → `reference/12_simulation_model/07_randomness_and_integrity.md`
   - 134 → world surface grid → `architecture/components/terrain.md`
   - 137 → Siege → `reference/01_skirmish_rules_and_modes/siege.md`
   - 165 → RGBA8 image → `architecture/components/asset_pipeline_and_resources.md`
   - 178, 179 → structure, structure port → `reference/03_buildings_and_world_objects/README.md`
   - 187 → match camera → `reference/06_controls_camera_and_commands.md`, `architecture/components/renderer/view_lighting_and_fog.md`
   - 188 → previous camera view: the pose before a camera animation started; animation playback is not specified, so it is likely a skip
   - 190, 225 → Relic Run → `reference/01_skirmish_rules_and_modes/relic_run.md`
   - 197, 198 → seasonal theme (see DEC-101, DEC-118) → `reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md`, `architecture/components/animation_and_vfx.md`
   - 202 → sky renderer → `architecture/components/renderer/world_renderers.md`
   - 204 → terrain colour ramp → `architecture/components/terrain.md`
   - 215, 217, 218 → Supply crate, Supply Storm, supply-crate outcome → `reference/04_supply_crates_powers_and_weapons/crates_and_powers.md`
   - 219, 458 → map id → `architecture/components/data_schemas.md`, `architecture/components/build_tooling_and_distribution.md`
   - 238 → Worker → `reference/02_units/worker.md`
   - 239 → Reinforcements (outcome) → `reference/04_supply_crates_powers_and_weapons/crates_and_powers.md`
   - 240, 276, 277 → UI window, UI button, UI core, UI panel base, UI button base → `architecture/components/ui_framework/core_and_widgets.md`
   - 241 → pack file → `architecture/components/asset_pipeline_and_resources.md`
   - 259 → Spawn Control → `reference/01_skirmish_rules_and_modes/spawn_control.md`
   - 262 → Relic Base, Relic Spawn Zone → `reference/03_buildings_and_world_objects/mode_objects.md`
   - 266, 346 → tick period, tick clock → `reference/12_simulation_model/01_tick_timing_and_main_loop.md`
   - 290 → procedural leg, procedural foot → `reference/02_units/creatures_and_hazards.md`, `architecture/components/animation_and_vfx.md`
   - 292 → volcano vent → `reference/03_buildings_and_world_objects/defences_and_props.md`
   - 295 → sound event definition → `reference/10_audio_design.md`, `architecture/components/audio_engine.md`
   - 296 → Rogue faction → `reference/01_skirmish_rules_and_modes/README.md`
   - 297 → shatter effects → `architecture/components/animation_and_vfx.md`
   - 322 → MSAA → `architecture/components/renderer/options_and_budgets.md` (ADR-0015)
   - 330, 331 → Visitor faction → `reference/01_skirmish_rules_and_modes/README.md`, `reference/02_units/creatures_and_hazards.md`
   - 338 → match options (the 14 option names need Species names) → `reference/01_skirmish_rules_and_modes/README.md`
   - 340 → mode → `reference/01_skirmish_rules_and_modes/README.md`
   - 342 → world cursor → `reference/06_controls_camera_and_commands.md`, `architecture/components/renderer/entity_and_overlay_renderers.md`
   - 345 → options screen → `architecture/components/ui_framework/` (C-UI-a, front-end page widgets)
   - 356 → power and research levels → `reference/02_units/README.md`
   - 360 → Waypoint beacon, waypoint order → `reference/02_units/leader_and_formations.md`, `reference/06_controls_camera_and_commands.md`
   - 367 → rocket turret (outcome) → `reference/04_supply_crates_powers_and_weapons/crates_and_powers.md`
   - 368 → turret, mortar turret, rocket turret → `reference/03_buildings_and_world_objects/defences_and_props.md`
   - 382 → trail point → `reference/02_units/strike_team.md`
   - 386 → Speed Boost; 405 → Swarm, Swarm streak → `reference/04_supply_crates_powers_and_weapons/crates_and_powers.md`
   - 410 → Strike team → `reference/02_units/strike_team.md`
   - 411 → starting forces (name not in the ledger; see "Names") → `reference/08_maps_and_level_design/rules_and_conventions.md`
   - 432, 433 → Zone Control → `reference/01_skirmish_rules_and_modes/zone_control.md`
   - 444, 445, 447 → terrain height field, terrain definition, terrain flatten area, terrain tile → `architecture/components/terrain.md`; 446 → terrain renderer → `architecture/components/renderer/world_renderers.md`
   - 448, 449 → string table, Species string table → `architecture/components/ui_framework/layout_rendering_input_and_localisation.md`, `architecture/components/data_schemas.md`
   - 456, 459 → map effect, map option → `reference/08_maps_and_level_design/rules_and_conventions.md`
   - 457 → map definition → `architecture/components/data_schemas.md`
   - 461 → map light → `architecture/components/renderer/README.md`
   - 468 → match world → `reference/12_simulation_model/04_containers_and_update_order.md`
   - 469, 470 → map editor (hooks) → `reference/08_maps_and_level_design/map_editor_hooks.md`
2. **To X-GLO-2: M–Z terms that A–L definitions rely on**, so they need entries in `m_to_z.md`: Massacre, meteor and meteor shower, Missile Barrage, mode, mortar turret, order marker, pack file, population cap, Power, Power slot, Power slots, promotion to Leader, Reinforcements, Relic, Relic Base, Relic Run, Relic Spawn Zone, Rogue faction, sample cache, Seed Pod, shockwave ring, Siege, Solar Array, sound event definition, sound source definition, sound voice, Spawn Control, Spawn Node, Spitter Plant, standard bot, starting forces, Stockpiler, Strike team, structure, sudden death, Supply crate, team command state, teleport structure, tick clock, tick period, Time Dilation, Time Rush, Total Control, Tourist, transit orb, Transport, Trooper, turret, Visitor faction, Waypoint beacon, Wisp, Worker, Zone Control.
3. **To X-GLO-2: format.** `README.md` fixes the four columns, the four kinds and the ordering rule (word by word, hyphen read as a space, qualifiers in brackets ignored); terms starting with a digit are in `a_to_l.md`. In a "Jump to" line, write letter links as `[**C**](#c)`: a bare `[C]` or `[I]` matches the evidence-tag hard-fail pattern.
4. **To the coordinator (F1 lists):** `species_work/original_strings.txt` contains the new Species texts "all power slots full" and "comeback powers slots full" (ledger rows for the power-slot messages), so quoting a Species message raises a false `orig-string` soft flag. I avoided the quotes; the list should drop Species replacement strings.
5. **To the coordinator (paths):** the glossary links to INDEX targets that do not exist yet. Two packages may change their target shape: G06 may become `reference/06_controls_camera_and_commands/` and C-DAT plans a `data_schemas/` folder; links to `reference/06_controls_camera_and_commands.md` (7 entries) and `architecture/components/data_schemas.md` (3 entries) then need updating. `CONVENTIONS.md` §3.1 still lists the pre-split document paths for R-CRATE, R-MAP, R-META, R-SIM, UI and DAT.
6. **To the coordinator (owner of the job queue):** its only described use is background sound loading, and threading is deferred; the glossary links `architecture/components/platform_layer.md`. C-PLT or C-AEN may own it instead.

## Names needed that are not in the ledger

Generic, non-IP terms used as glossary entries but absent from the ledger and from CONVENTIONS §8. Proposal: accept them as generic terms, or list them in CONVENTIONS §8 as "generic terms":
activation delay, advanced outcome, alliance, attackers and defenders, bad outcome, basic outcome, basic outcomes only, black box, bot difficulty, burst and pause cycle, capture, cheats, co-op, content build, countdown, crate drop mode, crate drop rate, crowd order, direct control, draw, effect, elimination, excluded outcomes, explosion, formation, front end, fuel, game speed, game time, good outcome, keyboard layout, lobby.
- "excluded outcomes" follows the decision register's wording ("the map's list of excluded outcomes").
- "basic outcomes only": the register uses both "basic crates only" and "basic outcomes"; proposal: "basic outcomes only" everywhere.
- "starting forces" (for X-GLO-2): proposal for the map-placed starting units, with identifier `StartingForces` and string-id prefix `map.starting_forces`.

## Dropped as out of scope (one line per group)

- Provenance vocabulary: build tags, evidence tags, register ids, carry-forward verdict names, the licence-posture term, the rights abbreviation.
- Brand, studio, character, partner, fiction-unit and original achievement-name rows (Species names are used instead).
- Superseded technology: the earlier graphics API and its extension and binding families, the fixed-function pipeline, compiled command lists, the emulation renderer and its shaders and distortion effect, hardware 3-D sound mixing, the x87 floating-point unit, the C run-time, bitmap pixel layouts and the bitmap format, the video grabber, DLL.
- Networking, sessions and lockstep (superseded; design-phase topics): the message container and wire format, the input-packet field code, letters, client and server classes, the sync byte, desync and desync dumps, late join, host and client, LAN and WAN play and discovery, the bandwidth counter and link simulator, blob transfer, network game-option ids, the IP address, authentication keys and key files, the deterministic maths wrappers, binary32 rounding and the floating-point build switch.
- Campaign and blocked content: the blocking switch and blocked types, every campaign building and unit row, cut-scene scripts, the campaign world files, global buildings and the global world, the campaign difficulty setting (always 0), the campaign power generator and its output unit (and EMA, whose only cited use is that generator), research collection buildings, gesture recognition and its data, the mentor and the logo splash, boot-loader screens, credit lists.
- Debug-only and dead tooling not kept by a decision: the tools menu, the video grabber and its switch, the sound and gesture editors and their envelope sequencer, attract mode, the benchmark recorder and benchmark maps, the debug configurations, the unused archive-aware file system, the debug-only crate outcome, the commented-out outcomes, the demo version and its limits. The cheat window is kept as "cheats" (DEC-041).
- Product-target and feature macros (crate, tutorial, mode, editor and multiplayer switches, the demo gate macro).
- Original container and reader classes replaced by standard containers or typed schemas, and the text-reader abbreviations tied to them: ASCII, BOM, CRLF / LF, EOF, EBNF, INI, API, GDF, EULA, FTP.
- Unreachable objects: the single-player turret aim target, the proximity mine, the bridge, the chess prototype, the incubator, the drop ship and the beach lander.
- Folded into another entry rather than dropped: the AI and CPU abbreviations (Bot), AI priority (bot navigation node), harvesting (Collector), help overlays and control help (hint overlay), the grace-time constant (grace period), the input binding file and the control-name list (action bindings), the animation script format (explainer animation), font atlases (glyph renderer), the diagnostic file names (crash reporter and black box), the filter-blueprint file (DSP filter definition), the data build pipeline (content build), the game-time and wall-clock functions (game time).
- Not given an entry: the classic bindings file pattern of CONVENTIONS §8.11, because the alternative control scheme is Dropped (DEC-056).
