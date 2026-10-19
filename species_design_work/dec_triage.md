# Decision triage

148 Open decisions. Rules: `prompts/species-design-phase.md` §5.4. Each `design phase` decision ends Resolved, Dropped or re-dated in its owning area; `playtest` and `prototype` decisions stay Open with their defaults.

| Area | Open | design phase | playtest | prototype |
|---|---|---|---|---|
| A0 | 1 | 1 | 0 | 0 |
| A1 | 8 | 8 | 0 | 0 |
| A2 | 20 | 20 | 0 | 0 |
| B1 | 3 | 0 | 3 | 0 |
| B3 | 7 | 6 | 1 | 0 |
| B4 | 26 | 16 | 10 | 0 |
| B7 | 9 | 6 | 3 | 0 |
| B9 | 8 | 5 | 3 | 0 |
| C1 | 7 | 5 | 2 | 0 |
| C2 | 12 | 12 | 0 | 0 |
| C4 | 12 | 7 | 5 | 0 |
| C5 | 4 | 4 | 0 | 0 |
| D1 | 1 | 1 | 0 | 0 |
| D10 | 3 | 3 | 0 | 0 |
| D2 | 12 | 8 | 3 | 1 |
| D4 | 3 | 3 | 0 | 0 |
| D5 | 2 | 2 | 0 | 0 |
| D8 | 8 | 8 | 0 | 0 |
| E2 | 2 | 2 | 0 | 0 |

| DEC | Title | Decide by | Area | Also affects | Assigned by | Register file |
|---|---|---|---|---|---|---|
| DEC-001 | Reference core loop: crowd-and-territory economy | Resolved (A0, 2026-10-18) | A0 | B1, B9 | Affects | decisions/verdicts_and_fixes.md |
| DEC-045 | Map editor | Resolved (A1, 2026-10-18) | A1 | D4 | Affects | decisions/verdicts_and_fixes.md |
| DEC-315 | Level-design patterns of the reference maps | Resolved (A1, 2026-10-18) | A1 | B9 | Affects | decisions/open_questions.md |
| DEC-324 | Terrain generator parameter semantics and biases | Resolved (A1, 2026-10-18) | A1 |  | Affects | decisions/open_questions.md |
| DEC-562 | Map lighting and weather presets | Resolved (A1, 2026-10-18) | A1 | D8, B4, D4 | Affects | decisions/controls_bots_maps_meta_audio.md |
| DEC-802 | Maps that match setup does not list | Resolved (A1, 2026-10-18) | A1 | C2, D4, D1, D10 | Affects | decisions/data_sequences_platform_debug.md |
| DEC-865 | Map data without a reference case | Resolved (A1, 2026-10-18) | A1 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-866 | Population-cap values of a map | Resolved (A1, 2026-10-18) | A1 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-966 | Terrain query edge cases | Resolved (A1, 2026-10-18) | A1 |  | Affects | decisions/engine_gap_rules.md |
| DEC-200 | Replication and authority model | Resolved (A2, 2026-10-19) | A2 | D2, B7, B9, D1, D10 | deferred topic: Networking model | decisions/deferred/networking.md |
| DEC-201 | Replication bandwidth and population budget | Resolved (A2, 2026-10-19) | A2 | B1, D2, D10 | deferred topic: Networking model | decisions/deferred/networking.md |
| DEC-202 | Command validation and authority | Resolved (A2, 2026-10-19) | A2 | D2, C1, C2, D10 | deferred topic: Networking model | decisions/deferred/networking.md |
| DEC-203 | Input timing, coalescing and latency | Resolved (A2, 2026-10-19) | A2 | D2, C1 | deferred topic: Networking model | decisions/deferred/networking.md |
| DEC-204 | Content and version identity | Resolved (A2, 2026-10-19) | A2 | A1, E2, D1, D4 | deferred topic: Networking model | decisions/deferred/networking.md |
| DEC-205 | Join, reconnect, leave and session continuity | Resolved (A2, 2026-10-19) | A2 | B9, C2 | deferred topic: Networking model | decisions/deferred/networking.md |
| DEC-206 | Divergence detection and integrity checking | Resolved (A2, 2026-10-19) | A2 | D2, D10 | deferred topic: Networking model | decisions/deferred/networking.md |
| DEC-207 | Transport, message format and robustness | Resolved (A2, 2026-10-19) | A2 | D1, D4, A1 | deferred topic: Networking model | decisions/deferred/networking.md |
| DEC-208 | Network diagnostics and test tooling | Resolved (A2, 2026-10-19) | A2 | D10 | deferred topic: Networking model | decisions/deferred/networking.md |
| DEC-221 | Composition root, service lifetimes and the application state machine | Resolved (A2, 2026-10-19) | A2 | D1, C2, C4, D2 | deferred topic: Simulation threading and process architecture | decisions/deferred/process_tick_and_roadmap.md |
| DEC-222 | Simulation and presentation boundary | Resolved (A2, 2026-10-19) | A2 | D2, D8, C4, C2, C1, D1 | deferred topic: Simulation threading and process architecture | decisions/deferred/process_tick_and_roadmap.md |
| DEC-223 | World, region and match state ownership and reset | Resolved (A2, 2026-10-19) | A2 | D2, D4 | deferred topic: Simulation threading and process architecture | decisions/deferred/process_tick_and_roadmap.md |
| DEC-224 | Threading and job model | Resolved (A2, 2026-10-19) | A2 | D2, D1, D8, C4, E2, D10 | deferred topic: Simulation threading and process architecture | decisions/deferred/process_tick_and_roadmap.md |
| DEC-225 | Frame loop, simulation pacing and catch-up | Resolved (A2, 2026-10-19) | A2 | D2, C2, D1, D10 | deferred topic: Simulation threading and process architecture | decisions/deferred/process_tick_and_roadmap.md |
| DEC-226 | Loading, streaming and asynchronous work | Resolved (A2, 2026-10-19) | A2 | E2, D8, C4, A1, C2 | deferred topic: Simulation threading and process architecture | decisions/deferred/process_tick_and_roadmap.md |
| DEC-227 | Simulation tick rate | Resolved (A2, 2026-10-19) | A2 | D2 | deferred topic: Tick rate | decisions/deferred/process_tick_and_roadmap.md |
| DEC-228 | Per-tick CPU budget, population caps and stress populations | Resolved (A2, 2026-10-19) | A2 | D10, D8, B1, D2, B9 | deferred topic: Tick rate | decisions/deferred/process_tick_and_roadmap.md |
| DEC-240 | Simulation numeric model and determinism scope | Resolved (A2, 2026-10-19) | A2 | D2, A1, D4, D1, D10 | deferred topic: Simulation numeric model | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-241 | Numeric semantics that reference rules depend on | Resolved (A2, 2026-10-19) | A2 | D2, B4, B3, D4, C2, D1 | deferred topic: Simulation numeric model | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-242 | Simulation random generator algorithm and stream derivation | Resolved (A2, 2026-10-19) | A2 | D2 | deferred topic: Simulation numeric model | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-084 | Hive Essence orbs and Hive ants | playtest | B1 | B4, B3 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-453 | Population-cap enforcement points and uncapped sources | playtest | B1 | B4, B3 | Affects | decisions/units_buildings_crates_economy.md |
| DEC-851 | Population-flow rule details | playtest | B1 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-043 | Wall destructibility | design phase | B3 | B9 | Affects | decisions/verdicts_and_fixes.md |
| DEC-089 | Smoke-column obstacle | design phase | B3 | D2, A1 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-108 | Launch Vessel survival under attack | playtest | B3 | B9 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-309 | Global flag on Source Nodes | design phase | B3 | A1, D4 | Affects | decisions/open_questions.md |
| DEC-454 | Population Limiter as a fallback cap | design phase | B3 | B1, A1 | Affects | decisions/units_buildings_crates_economy.md |
| DEC-462 | Turret facing from map data | design phase | B3 | A1, D4 | Affects | decisions/units_buildings_crates_economy.md |
| DEC-860 | Structure rule details | design phase | B3 | D8 | Affects | decisions/gameplay_gap_rules.md |
| DEC-091 | Dormant Crawler egg lifetime | playtest | B4 |  | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-101 | Seasonal theme | design phase | B4 | C2, D2 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-111 | Values derived from flat-terrain calculation | playtest | B4 | B3 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-113 | Abductor wander-target area | playtest | B4 |  | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-114 | Position of score-weighted crate drops | playtest | B4 |  | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-115 | Fixed match research levels | design phase | B4 | B3, C2 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-116 | Supply crates during the grace period | playtest | B4 | B9, D2, A0 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-121 | Ownership of Brood creatures | playtest | B4 |  | Affects | decisions/verdicts_and_fixes_2.md |
| DEC-302 | Transport unload order over sea | playtest | B4 | C1 | Affects | decisions/open_questions.md |
| DEC-303 | Missile Barrage arrival spread | playtest | B4 |  | Affects | decisions/open_questions.md |
| DEC-304 | Spawn Surge request count | design phase | B4 | B1 | Affects | decisions/open_questions.md |
| DEC-305 | Essence Grove orbs of team slots not in play | design phase | B4 | B1 | Affects | decisions/open_questions.md |
| DEC-307 | Essence orbs held by a removed Collector | design phase | B4 | B1 | Affects | decisions/open_questions.md |
| DEC-328 | Saturated team colours on effect bubbles | design phase | B4 | D8 | Affects | decisions/open_questions.md |
| DEC-551 | Units that outlive their Power slot | design phase | B4 | C1 | Affects | decisions/controls_bots_maps_meta_audio.md |
| DEC-750 | Explainer-animation loop and visibility rules | design phase | B4 | C2, D4 | Affects | decisions/vfx_audio_ui.md |
| DEC-852 | Game-speed divisors in Worker rules | design phase | B4 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-853 | Worker and Trooper rule details | playtest | B4 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-854 | Transport, Collector and Strike team rule details | design phase | B4 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-855 | Creature and hazard rule quirks | playtest | B4 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-856 | Creature presentation quirks | design phase | B4 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-858 | Empty outcome lists | design phase | B4 | B1 | Affects | decisions/gameplay_gap_rules.md |
| DEC-859 | Tick counts chosen for reference durations | design phase | B4 | B3, B9 | Affects | decisions/gameplay_gap_rules.md |
| DEC-962 | Species effect defaults for emitters, shard edges and the eaten-orb display | design phase | B4 |  | Affects | decisions/engine_gap_rules.md |
| DEC-963 | Animation and VFX reference looks kept as defaults | design phase | B4 |  | Affects | decisions/engine_gap_rules.md |
| DEC-964 | Accessibility limits for camera shake and flashes | design phase | B4 | D8 | Affects | decisions/engine_gap_rules.md |
| DEC-112 | Bot strength calibration | playtest | B7 |  | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-122 | Bot and deputy actions outside the player command catalogue | design phase | B7 | D2, B4, B3 | Affects | decisions/verdicts_and_fixes_2.md |
| DEC-311 | Bot controller as a target | design phase | B7 | B4, B3 | Affects | decisions/open_questions.md |
| DEC-312 | Bot decisions and node recounts within a tick | design phase | B7 | D2 | Affects | decisions/open_questions.md |
| DEC-557 | Bot troop movement between navigation nodes | design phase | B7 | D2 | Affects | decisions/controls_bots_maps_meta_audio.md |
| DEC-558 | Bot controller of the Rogue and Visitor factions | design phase | B7 | B9 | Affects | decisions/controls_bots_maps_meta_audio.md |
| DEC-559 | Bot module structure | design phase | B7 |  | Affects | decisions/controls_bots_maps_meta_audio.md |
| DEC-861 | Bot rule quirks | playtest | B7 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-862 | Bot rule details | playtest | B7 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-066 | Multi-round Siege | design phase | B9 | D2 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-071 | Flag Chain base-under-attack announcement interval | playtest | B9 | C4 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-123 | Tutorial for the first playable | design phase | B9 | A0 | Affects | decisions/verdicts_and_fixes_2.md |
| DEC-300 | Return of teams wiped out in sudden death | playtest | B9 |  | Affects | decisions/open_questions.md |
| DEC-301 | Solar Array knock-out in Siege | playtest | B9 | B3 | Affects | decisions/open_questions.md |
| DEC-450 | Start positions and the colours of bot and unused slots | design phase | B9 | C2, D2 | Affects | decisions/units_buildings_crates_economy.md |
| DEC-451 | Co-op friendship of slots outside the two alliances | design phase | B9 | D2 | Affects | decisions/units_buildings_crates_economy.md |
| DEC-850 | Match-rule gap rules | design phase | B9 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-308 | Orders to Workers that operate a port | playtest | C1 | B3, B1 | Affects | decisions/open_questions.md |
| DEC-310 | Camera tracking after selecting a Leader or a Transport | design phase | C1 |  | Affects | decisions/open_questions.md |
| DEC-556 | Power slot order | playtest | C1 | B4, C2 | Affects | decisions/controls_bots_maps_meta_audio.md |
| DEC-803 | Camera animations without content | design phase | C1 | A1, C5, D4 | Affects | decisions/data_sequences_platform_debug.md |
| DEC-864 | Pointer fallback and camera smoothing | design phase | C1 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-954 | Input reference behaviours kept as defaults | design phase | C1 |  | Affects | decisions/engine_gap_rules.md |
| DEC-956 | Pick-ray pixel convention | design phase | C1 |  | Affects | decisions/engine_gap_rules.md |
| DEC-044 | Commonly expected options | design phase | C2 | D8 | Affects | decisions/verdicts_and_fixes.md |
| DEC-047 | Achievements, statistics and leaderboards | design phase | C2 | B9 | Affects | decisions/verdicts_and_fixes.md |
| DEC-048 | Post-match results screen | design phase | C2 | B9 | Affects | decisions/verdicts_and_fixes.md |
| DEC-050 | Front-end features designed new | design phase | C2 | C1, C5, B9 | Affects | decisions/verdicts_and_fixes.md |
| DEC-243 | Lobby authority and the match-setup state | design phase | C2 | B9, D2 | deferred topic: Sessions | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-244 | Session join, admission, reconnection and spectators | design phase | C2 | D2 | deferred topic: Sessions | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-245 | Session discovery, the advertised session record and the session browser | design phase | C2 |  | deferred topic: Sessions | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-314 | Map list order under ties | design phase | C2 | A1 | Affects | decisions/open_questions.md |
| DEC-316 | Front-end text blend and page transition | design phase | C2 | D8 | Affects | decisions/open_questions.md |
| DEC-867 | Closed player slots | design phase | C2 | A1 | Affects | decisions/gameplay_gap_rules.md |
| DEC-868 | Front-end reference quirks | design phase | C2 | A1 | Affects | decisions/gameplay_gap_rules.md |
| DEC-952 | UI framework reference behaviours kept as defaults | design phase | C2 |  | Affects | decisions/engine_gap_rules.md |
| DEC-049 | Music | design phase | C4 | A0 | Affects | decisions/verdicts_and_fixes.md |
| DEC-109 | Crowd-sound audibility radius | playtest | C4 |  | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-317 | Voice set language | design phase | C4 | C2 | Affects | decisions/open_questions.md |
| DEC-318 | Sound triggers tied to the frame rate | design phase | C4 |  | Affects | decisions/open_questions.md |
| DEC-319 | Ambience during loading views | design phase | C4 | C2 | Affects | decisions/open_questions.md |
| DEC-320 | Match-end pitch drop | playtest | C4 |  | Affects | decisions/open_questions.md |
| DEC-321 | Attached sounds when their emitter is removed | playtest | C4 |  | Affects | decisions/open_questions.md |
| DEC-322 | Start value of smoothed sound parameters | playtest | C4 |  | Affects | decisions/open_questions.md |
| DEC-565 | Sounds for the other silent gameplay events | design phase | C4 |  | Affects | decisions/controls_bots_maps_meta_audio.md |
| DEC-566 | Listener position and distance limits | playtest | C4 |  | Affects | decisions/controls_bots_maps_meta_audio.md |
| DEC-567 | Dry mix or applied effects | design phase | C4 |  | Affects | decisions/controls_bots_maps_meta_audio.md |
| DEC-965 | Audio engine reference behaviours kept as defaults | design phase | C4 |  | Affects | decisions/engine_gap_rules.md |
| DEC-051 | Tutorials and presentation timelines | design phase | C5 | C1 | Affects | decisions/verdicts_and_fixes.md |
| DEC-110 | Second tutorial start condition | design phase | C5 |  | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-550 | Power cancellation in the tutorials | design phase | C5 | B4, C1 | Affects | decisions/controls_bots_maps_meta_audio.md |
| DEC-870 | Tutorial director details | design phase | C5 |  | Affects | decisions/gameplay_gap_rules.md |
| DEC-220 | Process roles and the headless server build | design phase | D1 | D10, D2 | deferred topic: Hosting | decisions/deferred/process_tick_and_roadmap.md |
| DEC-229 | Build order, milestones and work breakdown | design phase | D10 | D1 | Affects | decisions/deferred/process_tick_and_roadmap.md |
| DEC-971 | Reference test machine and minimum-specification hardware | design phase | D10 | D8, E2, D1, C4, C1, C2, D4, A1 | Affects | decisions/engine_gap_rules.md |
| DEC-972 | What the cheats-used marker suppresses | design phase | D10 | C2 | Affects | decisions/engine_gap_rules.md |
| DEC-003 | Mode-second length after playtest | playtest | D2 | B9, B1 | Affects | decisions/verdicts_and_fixes.md |
| DEC-046 | Replays | design phase | D2 | C2, D10 | Affects | decisions/verdicts_and_fixes.md |
| DEC-094 | Exact or approximate actor-grid queries | playtest | D2 | B4, B7, B9 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-106 | Performance envelope at the population cap | prototype | D2 | B1, D10, D8, B4 | Affects | decisions/verdicts_and_fixes_3.md |
| DEC-263 | Route and actor-group edge cases | design phase | D2 |  | Affects | decisions/simulation_gap_rules.md |
| DEC-264 | What runs while game speed is 0 | design phase | D2 | B9 | Affects | decisions/simulation_gap_rules.md |
| DEC-265 | Real duration of the speed-crate holds | playtest | D2 | B4 | Affects | decisions/simulation_gap_rules.md |
| DEC-266 | Decorative objects in the simulated pools | design phase | D2 | B4 | Affects | decisions/simulation_gap_rules.md |
| DEC-267 | Barrier Emitter line in the obstacle grid | design phase | D2 | B3 | Affects | decisions/simulation_gap_rules.md |
| DEC-313 | Dead airborne actors | design phase | D2 | B4 | Affects | decisions/open_questions.md |
| DEC-455 | Same-tick advance of objects created or handed over during a tick | design phase | D2 | B1, B4, B3 | Affects | decisions/units_buildings_crates_economy.md |
| DEC-651 | Lifetime of simulation routes | design phase | D2 | B4, D8 | Affects | decisions/simulation_terrain_render_assets.md |
| DEC-250 | Persisted data inventory and client-local storage | design phase | D4 | C2, D2, D1, C1, D10 | deferred topic: Persistence | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-251 | World-state persistence, schema migration and rollback | design phase | D4 | D2, D1, D10 | deferred topic: Persistence | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-969 | Co-op group lists in ascending position order | design phase | D4 | B9 | Affects | decisions/engine_gap_rules.md |
| DEC-248 | Player identity and entitlement | design phase | D5 | D1, C2 | deferred topic: Online services | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-249 | Directory, rendezvous, relay and matchmaking services | design phase | D5 | D1, C2 | deferred topic: Online services | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-040 | Inverted ink-on-paper style | design phase | D8 |  | Affects | decisions/verdicts_and_fixes.md |
| DEC-246 | Terrain runtime model and terrain content identity | design phase | D8 | A1, E2, D4, D2 | deferred topic: Terrain runtime model | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-247 | Terrain scale and large-world terrain inputs | design phase | D8 | A1, C1, D2 | deferred topic: Terrain runtime model | decisions/deferred/numerics_lobby_terrain_services.md |
| DEC-325 | Light direction and the camera transform | design phase | D8 |  | Affects | decisions/open_questions.md |
| DEC-326 | Terrain grid overlay | design phase | D8 | A1 | Affects | decisions/open_questions.md |
| DEC-327 | Structure render-detail variants | design phase | D8 | B3 | Affects | decisions/open_questions.md |
| DEC-958 | Renderer reference looks kept as defaults | design phase | D8 |  | Affects | decisions/engine_gap_rules.md |
| DEC-959 | Renderer defaults for the burn layer, pulse clocks, egg shadow, route spots and countdown screens | design phase | D8 |  | Affects | decisions/engine_gap_rules.md |
| DEC-329 | Assets of objects spawned only at run time | design phase | E2 |  | Affects | decisions/open_questions.md |
| DEC-330 | Mesh sockets referenced only by data | design phase | E2 | B3 | Affects | decisions/open_questions.md |
