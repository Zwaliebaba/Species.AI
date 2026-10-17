# X-DEC-2c report: deferred decisions on numerics, lobby, terrain runtime, services and persistence

## Files written

| File | Content |
|---|---|
| `design_next/decisions/deferred/numerics_lobby_terrain_services.md` | heading, scope paragraph, index table, 12 Open DEC entries (DEC-240 to DEC-251), about 72 KB |
| `species_work/dec_map_X-DEC-2c.csv` | 12 rows: `dec_id, title, status, affects_prefix, affects_files, orig_refs` (csv module, `\n` line ends) |
| `species_work/scripts/X-DEC-2c_extract.py` | prints the package's ORIG ranges (read-only helper) |
| `species_work/scripts/X-DEC-2c_terrain_stats.py` | aggregates the terrain survey table without printing map names |
| `species_work/scripts/X-DEC-2c_decmap.py` | writes the CSV; checks titles against the register and that every package input range maps to a DEC (result: complete, 62 of 62) |

Nothing was published or uploaded. No Artifact, Gamma, Drive, Docs or GitHub tool was used.

## Counts

- DEC entries: 12, all Open, all "Decide by: design phase". Each has the §6.5 fields plus "Constraints and lessons" (numbered) and "Reference behaviour" (one paragraph).
  - Numeric model: DEC-240 (numeric model and determinism scope), DEC-241 (numeric semantics that rules depend on, including data and lobby parsing), DEC-242 (generator algorithm and stream derivation).
  - Lobby and sessions: DEC-243 (lobby authority and match-setup state), DEC-244 (join, admission, reconnection, spectators), DEC-245 (discovery, advertised record, session browser).
  - Terrain runtime: DEC-246 (runtime model and content identity), DEC-247 (scale figures and large-world inputs).
  - Services: DEC-248 (identity and entitlement), DEC-249 (directory, rendezvous, relay, matchmaking).
  - Persistence: DEC-250 (persisted data inventory, client-local storage), DEC-251 (world-state persistence, migration, rollback).
- DEC IDs used: 240 to 251. 252 to 259 are free.
- Requirements, tuning keys, diagrams: 0. A register file carries none of these.
- Constraint and lesson items: 129 in total (DEC-240: 11, DEC-241: 13, DEC-242: 6, DEC-243: 12, DEC-244: 12, DEC-245: 14, DEC-246: 13, DEC-247: 12, DEC-248: 11, DEC-249: 8, DEC-250: 6, DEC-251: 11). Register file size is 71.8 KB.

## Notes on content decisions

- **Bit-exactness (DEC-240).** States what must agree and why: transcendental functions are not correctly rounded; extended precision, the control state and third-party side effects affect rounding; contraction, reassociation and vectorised reductions affect ordering. It gives the binary32-midpoint figure (≈ 2⁻²⁹ per call per ulp over 10⁸ to 10⁹ calls). No instruction set, compiler flag or runtime library is named.
- **Terrain scale figures (DEC-247)** are aggregates over the reference match maps. Test, stress, benchmark and sandbox terrains, the dropped mode's arenas, the cut map and campaign maps are excluded. No map is named. The derived world sizes for 16 and 200 players, and the coordinate-precision spacings, are computed values, labelled as derived.
- **Services (DEC-248, DEC-249)** name no service provider, host, port, key format or storefront. They describe what entitlement, identity and discovery must provide.
- **Persistence.** F2c routed no ORIG rows to DD-PERSIST. DEC-250 builds the inventory of what is saved from this package's ranges: settings-store keys in the browser and advertising sections, the achievement-mask cadence, the replay property of the command log, and the terrain-detail setting. DEC-251 takes its constraints from the vision brief (O1, O3, O4, O6, O8, O10; §6 items 1 and 7). Its `orig_refs` list only the lesson sources: exact serialisation, no session resumption, content hash and seed recording.
- **Cross-references** to other registers: `decisions/verdicts_and_fixes.md` for the 11-tick mode second, cited by file only because I do not know its DEC ID. `networking.md` and `process_tick_and_roadmap.md` are cited by file name in the same folder.

## DEC requests

None outside my range. Possible later uses of 252 to 259: none needed now.

## ADR requests

None. All topics are superseded verdicts (design phase only).

## Cross-document requests

Every Open DEC must be cited by at least one requirement (gate §7.3 item 6). The rewrite agents should cite these DECs:

| Package | Target | Cite |
|---|---|---|
| C-SIM-c | `reference/12_simulation_model/07_randomness_and_integrity.md`, `01_tick_timing_and_main_loop.md` | DEC-240, DEC-241, DEC-242 |
| C-SIM-a | `reference/12_simulation_model/02_match_lifecycle_and_state_reset.md`, `03_entity_model_and_identity.md` | DEC-241, DEC-251 |
| C-SIM-b | `reference/12_simulation_model/05_space_routing_and_physics.md`, `06_teams_and_player_commands.md` | DEC-241, DEC-244, DEC-246, DEC-247 |
| C-TER | `architecture/components/terrain.md` §8 (Design-phase inputs) | DEC-246, DEC-247 |
| C-DAT | `architecture/components/data_schemas.md` (numeric field parsing, map schema, preferences schema, schema versioning) | DEC-241, DEC-246, DEC-250, DEC-251 |
| C-PLT | `architecture/components/platform_layer.md` (per-thread floating-point state, settings store, network services) | DEC-240, DEC-248, DEC-249, DEC-250 |
| C-BLD | `architecture/components/build_tooling_and_distribution.md` (simulation library build settings, distribution and trial) | DEC-240, DEC-248 |
| C-DBG | `architecture/components/diagnostics_test_and_performance.md` (replay hashing on several CPUs) | DEC-240, DEC-242, DEC-250 |
| C-AST | `architecture/components/asset_pipeline_and_resources.md` (content hashes) | DEC-246 |
| C-RND-a / C-CAM | `renderer/world_renderers.md`, `camera_and_sequences.md` (strategic zoom, camera-relative coordinates) | DEC-247 |
| C-UI-b | `ui_framework/pages_and_screen_recipes.md` (browser and lobby pages) | DEC-243, DEC-245 |
| C-INP | `architecture/components/input.md` (bindings file) | DEC-250 |
| G09a | `reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md` (lobby, join, password, browser pages) | DEC-243, DEC-244, DEC-245, DEC-248, DEC-249 |
| G09b | `reference/09_front_end_and_match_flow/options_localisation_and_hooks.md` (settings, achievement and replay hooks) | DEC-250 |
| G01 | `reference/01_skirmish_rules_and_modes/README.md` (match options) | DEC-243 |
| G02a1 | `reference/02_units/leader_and_formations.md` (formation volley timing) | DEC-241 |
| G03a / G03b | turret aim cone, Beam Dish yaw speed | DEC-241 |
| G08a / G08b | `reference/08_maps_and_level_design/` (map identity, terrain limits) | DEC-246, DEC-247 |

Requests to other register owners and to the coordinator:

- **X-DEC-2a (networking.md).** My entries point to `networking.md` for divergence detection, full-history catch-up and the region partition. Please keep the failure modes there and avoid duplicating the lobby authority (DEC-243) and admission (DEC-244) lessons.
- **X-DEC-1.** The bot-difficulty off-by-one is mentioned in DEC-243 item 4 as a lesson only; its decision stays with your register. If you assign an ID to the integer-tick and 11-tick-mode-second decision, the coordinator can add it to DEC-241 item 3.
- **Coordinator (CONVENTIONS.md §9 Allowed terms).** Please add: `Linux` — `decisions/deferred/numerics_lobby_terrain_services.md` — names the dedicated-server build only (server hosting, vision §6 item 10).
- **Coordinator (CONVENTIONS.md §3.1).** The prefix registry lists R-MAP and R-META as single files, while the packages create folders `reference/08_maps_and_level_design/` and `reference/09_front_end_and_match_flow/`.

## Names needed but not in the ledger

- **3×3 rotation matrix.** The ledger has `Mat3x4` but no entry for the 3×3 type. I wrote "the 3×3 rotation type" in prose. Proposed: "3x3 matrix", `Mat3x3`.
- **"settlement", "deputy", "world server", "region".** These are vision vocabulary, used as-is. Proposed: add them to the glossary through the design phase, not to the reference ledger.
- **"carried structure".** Prose for the ledger's carryable structure (`CarryableStructure`). It needs no new entry.

## Scan results

- Hard-fail scan (`forbidden_patterns.tsv`): 0 hits.
- Soft scan (`soft_patterns.tsv`): 4 hits, all `Linux` (lines 28, 38, 226, 281). Each one is the phrase "Linux dedicated-server build", which is the allowed server-hosting context. Justified above, and an allowed-terms entry is requested. These hits were fixed: "shipped" (rephrased), "used to" (rephrased), and two quoted strings that matched UI text (rephrased as prose).
- My own extra check for provider names, toolchain and instruction-set names, port numbers, key formats and ORIG item codes also found 0 hits.

## Dropped content, by group

- **Per-map terrain survey rows and golden digests, and every map name.** They are reference-map-specific IP; only the aggregate figures are kept (DEC-247).
- **Lobby option numeric ids, message names, port numbers, service host names and directory field keys.** These are wire details and services of the reference, replaced by Species-term descriptions.
- **Credential format, sanity-check rules, trial-restriction listing, the trial filter and the store page.** These belong to trial and credential gating that is not kept; DEC-248 states what entitlement must provide instead.
- **Code-census counts (call sites per function, float-field inventories by class).** These are provenance, not design; only the design-relevant workload figures are kept (transcendental calls per match, power calls per terrain).
- **The member-by-member maths-library semantics** (constructors, debug output, unused helpers, conversions to the old graphics matrix layout, broken debug format strings). They matter only for bit-compatibility with the reference. DEC-241 keeps the degenerate-input, tolerance, convention and ordering hazards.
- **The browser's pixel layout, colour table and controller scroll details.** These are user-interface layout of a page the design phase redesigns. Rows, columns, sort order, filters and click rules are kept as lessons (DEC-245).
- **The unused scored sort helper and the unused spectator-join path.** They have no caller. The spectator path is mentioned in one clause.
- **Toolchain-specific evidence** (project attributes, per-configuration flags, the secondary graphics build, other-platform compiler options). These are legacy technology; the hazards are kept without naming it (DEC-240 items 3, 4, 7 and 8).
- **Unknowns about the reference itself** (whether builds or platforms ever played together, CRT accuracy of a specific build, bit-equality of the generator re-implementation). They are irrelevant because Species maps are new and Species builds its own maths library. They are reflected as DEC-246 item 13 and DEC-240 items 2 and 8.
