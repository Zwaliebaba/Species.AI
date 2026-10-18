# Adoption ledger: 12 simulation model

> Status: Draft · Owner: area D2 · Reference: [12_simulation_model](../../reference/12_simulation_model/README.md) · Rules: `prompts/species-design-phase.md` §5.3

87 sections, 424 requirements. A section row covers every requirement under its heading; an exception row (scope: one `R-` id) overrides it. Verdicts: `adopt`, `adopt-skirmish`, `adapt` (names the Species requirements that replace it), `drop` (gives the reason).

| Scope | Verdict | Species requirements | Note |
|---|---|---|---|
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.1` | pending | | The reference tick rate: 6 requirements (R-SIM-500 to R-SIM-505) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.2` | pending | | The tick and its update slices: 7 requirements (R-SIM-510 to R-SIM-516) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.3` | pending | | The reference tick schedule: 6 requirements (R-SIM-520 to R-SIM-525) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.4` | pending | | Slices per render frame and catch-up: 7 requirements (R-SIM-530 to R-SIM-536) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.5` | pending | | The tick clock and game speed: 10 requirements (R-SIM-540 to R-SIM-549) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.6` | pending | | The held world, the start hold, pause and the end of a match: 8 requirements (R-SIM-550 to R-SIM-557) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.7` | pending | | Render-time extrapolation: 6 requirements (R-SIM-560 to R-SIM-565) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.8` | pending | | Clocks: 4 requirements (R-SIM-570 to R-SIM-573) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.9` | pending | | Render rate and the frame-rate counter: 2 requirements (R-SIM-575 to R-SIM-576) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.10` | pending | | The in-match frame: 5 requirements (R-SIM-580 to R-SIM-584) |
| `12_simulation_model/02_match_lifecycle_and_state_reset.md §2.1` | pending | | Lifetime classes: 5 requirements (R-SIM-001 to R-SIM-005) |
| `12_simulation_model/02_match_lifecycle_and_state_reset.md §2.2` | pending | | Match start (simulation side): 8 requirements (R-SIM-006 to R-SIM-013) |
| `12_simulation_model/02_match_lifecycle_and_state_reset.md §2.3` | pending | | Match entry: 2 requirements (R-SIM-014 to R-SIM-015) |
| `12_simulation_model/02_match_lifecycle_and_state_reset.md §2.4` | pending | | Match-world initialisation: 3 requirements (R-SIM-016 to R-SIM-018) |
| `12_simulation_model/02_match_lifecycle_and_state_reset.md §2.5` | pending | | Per-match state and its reset points: 10 requirements (R-SIM-019 to R-SIM-032) |
| `12_simulation_model/02_match_lifecycle_and_state_reset.md §2.6` | pending | | Teardown: 1 requirement (R-SIM-028) |
| `12_simulation_model/02_match_lifecycle_and_state_reset.md §2.7` | pending | | Multi-round Siege rebuild: 2 requirements (R-SIM-029 to R-SIM-030) |
| `12_simulation_model/02_match_lifecycle_and_state_reset.md §2.8` | pending | | Persistence: 1 requirement (R-SIM-031) |
| `12_simulation_model/03_entity_model_and_identity.md §2.1` | pending | | Object families: 4 requirements (R-SIM-101 to R-SIM-104) |
| `12_simulation_model/03_entity_model_and_identity.md §2.2` | pending | | Type hierarchy (Species types): 1 requirement (R-SIM-105) |
| `12_simulation_model/03_entity_model_and_identity.md §2.3.1` | pending | | Simulated object: 2 requirements (R-SIM-106 to R-SIM-107) |
| `12_simulation_model/03_entity_model_and_identity.md §2.3.2` | pending | | Actor: 9 requirements (R-SIM-108 to R-SIM-116) |
| `12_simulation_model/03_entity_model_and_identity.md §2.3.3` | pending | | Structure: 7 requirements (R-SIM-117 to R-SIM-123) |
| `12_simulation_model/03_entity_model_and_identity.md §2.3.4` | pending | | Effects, laser bolts and Essence orbs: 5 requirements (R-SIM-124 to R-SIM-128) |
| `12_simulation_model/03_entity_model_and_identity.md §2.3.5` | pending | | Actor group: 1 requirement (R-SIM-129) |
| `12_simulation_model/03_entity_model_and_identity.md §2.3.6` | pending | | Drawing: 1 requirement (R-SIM-130) |
| `12_simulation_model/03_entity_model_and_identity.md §2.4` | pending | | Identity and handles: 13 requirements (R-SIM-131 to R-SIM-143) |
| `12_simulation_model/03_entity_model_and_identity.md §2.5` | pending | | Population of simulated objects: 2 requirements (R-SIM-144 to R-SIM-145) |
| `12_simulation_model/04_containers_and_update_order.md §2.1` | pending | | The match world: 3 requirements (R-SIM-201 to R-SIM-203) |
| `12_simulation_model/04_containers_and_update_order.md §2.2` | pending | | Simulation containers: 2 requirements (R-SIM-204 to R-SIM-205) |
| `12_simulation_model/04_containers_and_update_order.md §2.3` | pending | | Iteration order and slot reuse: 7 requirements (R-SIM-206 to R-SIM-212) |
| `12_simulation_model/04_containers_and_update_order.md §2.4` | pending | | Slice partitioning: 5 requirements (R-SIM-213 to R-SIM-217) |
| `12_simulation_model/04_containers_and_update_order.md §2.5` | pending | | Update order within a tick: 8 requirements (R-SIM-218 to R-SIM-225) |
| `12_simulation_model/04_containers_and_update_order.md §2.6` | pending | | Creation rules: 5 requirements (R-SIM-226 to R-SIM-230) |
| `12_simulation_model/04_containers_and_update_order.md §2.7` | pending | | Removal rules: 2 requirements (R-SIM-231 to R-SIM-232) |
| `12_simulation_model/04_containers_and_update_order.md §2.8` | pending | | Map-effect spawners: 6 requirements (R-SIM-233 to R-SIM-238) |
| `12_simulation_model/04_containers_and_update_order.md §2.9` | pending | | Order-dependent outcomes: 1 requirement (R-SIM-239) |
| `12_simulation_model/04_containers_and_update_order.md §2.10` | pending | | Match-world query and service operations: 1 requirement (R-SIM-240) |
| `12_simulation_model/05_space_routing_and_physics.md §2.1` | pending | | General: 2 requirements (R-SIM-300 to R-SIM-301) |
| `12_simulation_model/05_space_routing_and_physics.md §2.2` | pending | | Spatial structures of the match world: 3 requirements (R-SIM-302 to R-SIM-304) |
| `12_simulation_model/05_space_routing_and_physics.md §2.3` | pending | | Actor grid: layout, cells and registration: 4 requirements (R-SIM-305 to R-SIM-308) |
| `12_simulation_model/05_space_routing_and_physics.md §2.4` | pending | | Actor grid queries: 6 requirements (R-SIM-309 to R-SIM-314) |
| `12_simulation_model/05_space_routing_and_physics.md §2.5` | pending | | Actor grid cache: 5 requirements (R-SIM-315 to R-SIM-319) |
| `12_simulation_model/05_space_routing_and_physics.md §2.6` | pending | | Obstacle grid: 10 requirements (R-SIM-320 to R-SIM-329) |
| `12_simulation_model/05_space_routing_and_physics.md §2.7` | pending | | Route graphs: data model: 5 requirements (R-SIM-330 to R-SIM-334) |
| `12_simulation_model/05_space_routing_and_physics.md §2.8` | pending | | Walkability primitives: 4 requirements (R-SIM-335 to R-SIM-338) |
| `12_simulation_model/05_space_routing_and_physics.md §2.9` | pending | | Route graph construction: 6 requirements (R-SIM-339 to R-SIM-344) |
| `12_simulation_model/05_space_routing_and_physics.md §2.10` | pending | | Path query: 9 requirements (R-SIM-345 to R-SIM-353) |
| `12_simulation_model/05_space_routing_and_physics.md §2.11` | pending | | Following a route: 3 requirements (R-SIM-354 to R-SIM-356) |
| `12_simulation_model/05_space_routing_and_physics.md §2.12` | pending | | Route lifetime and handles: 3 requirements (R-SIM-357 to R-SIM-359) |
| `12_simulation_model/05_space_routing_and_physics.md §2.13` | pending | | Integration model and clamps: 10 requirements (R-SIM-360 to R-SIM-369) |
| `12_simulation_model/05_space_routing_and_physics.md §2.14` | pending | | Falling: 5 requirements (R-SIM-370 to R-SIM-374) |
| `12_simulation_model/05_space_routing_and_physics.md §2.15` | pending | | Water: 5 requirements (R-SIM-375 to R-SIM-379) |
| `12_simulation_model/05_space_routing_and_physics.md §2.16` | pending | | Obstacle push and structure contact: 5 requirements (R-SIM-380 to R-SIM-384) |
| `12_simulation_model/05_space_routing_and_physics.md §2.17` | pending | | Bounce: 1 requirement (R-SIM-385) |
| `12_simulation_model/05_space_routing_and_physics.md §2.18` | pending | | Teleport entry: 1 requirement (R-SIM-386) |
| `12_simulation_model/05_space_routing_and_physics.md §2.19` | pending | | Ray tests: 1 requirement (R-SIM-387) |
| `12_simulation_model/05_space_routing_and_physics.md §2.20` | pending | | Explosions: 2 requirements (R-SIM-388 to R-SIM-389) |
| `12_simulation_model/06_teams_and_player_commands.md §2.1` | pending | | General: 2 requirements (R-SIM-390 to R-SIM-391) |
| `12_simulation_model/06_teams_and_player_commands.md §2.2` | pending | | Team slots, ids and kinds: 5 requirements (R-SIM-392 to R-SIM-396) |
| `12_simulation_model/06_teams_and_player_commands.md §2.3` | pending | | Lobby slots and match teams: 11 requirements (R-SIM-397 to R-SIM-407) |
| `12_simulation_model/06_teams_and_player_commands.md §2.4` | pending | | Ownership of objects: 4 requirements (R-SIM-408 to R-SIM-411) |
| `12_simulation_model/06_teams_and_player_commands.md §2.5` | pending | | Friendship: 3 requirements (R-SIM-412 to R-SIM-414) |
| `12_simulation_model/06_teams_and_player_commands.md §2.6` | pending | | Colours, start positions and alliances: 9 requirements (R-SIM-415 to R-SIM-423) |
| `12_simulation_model/06_teams_and_player_commands.md §2.7` | pending | | Actor groups: 9 requirements (R-SIM-424 to R-SIM-432) |
| `12_simulation_model/06_teams_and_player_commands.md §2.8` | pending | | Bots as team slots: 5 requirements (R-SIM-433 to R-SIM-437) |
| `12_simulation_model/06_teams_and_player_commands.md §2.9` | pending | | The team command state: 6 requirements (R-SIM-438 to R-SIM-443) |
| `12_simulation_model/06_teams_and_player_commands.md §2.10` | pending | | Execution order of commands: 5 requirements (R-SIM-444 to R-SIM-448) |
| `12_simulation_model/06_teams_and_player_commands.md §2.11` | pending | | Player command catalogue: 14 requirements (R-SIM-449 to R-SIM-462) |
| `12_simulation_model/07_randomness_and_integrity.md §2.1` | pending | | Scope of the integrity rules: 2 requirements (R-SIM-600 to R-SIM-601) |
| `12_simulation_model/07_randomness_and_integrity.md §2.2` | pending | | Random streams: 9 requirements (R-SIM-605 to R-SIM-613) |
| `12_simulation_model/07_randomness_and_integrity.md §2.3` | pending | | Draw discipline: 9 requirements (R-SIM-620 to R-SIM-628) |
| `12_simulation_model/07_randomness_and_integrity.md §2.4` | pending | | What each object family draws: 2 requirements (R-SIM-630 to R-SIM-631) |
| `12_simulation_model/07_randomness_and_integrity.md §2.5` | pending | | Simulation state and presentation state: 3 requirements (R-SIM-635 to R-SIM-637) |
| `12_simulation_model/07_randomness_and_integrity.md §2.6` | pending | | Presentation randomness inside simulation code: 1 requirement (R-SIM-640) |
| `12_simulation_model/07_randomness_and_integrity.md §2.7` | pending | | Tree crown hit volumes: 2 requirements (R-SIM-650 to R-SIM-651) |
| `12_simulation_model/07_randomness_and_integrity.md §2.8` | pending | | Presentation never writes simulation state: 2 requirements (R-SIM-655 to R-SIM-656) |
| `12_simulation_model/07_randomness_and_integrity.md §2.9` | pending | | Local-only state never enters the simulation: 10 requirements (R-SIM-660 to R-SIM-669) |
| `12_simulation_model/07_randomness_and_integrity.md §2.10` | pending | | Layering rules: 6 requirements (R-SIM-670 to R-SIM-675) |
| `12_simulation_model/07_randomness_and_integrity.md §2.11` | pending | | Rules the object model states: 6 requirements (R-SIM-680 to R-SIM-685) |
| `12_simulation_model/07_randomness_and_integrity.md §2.12` | pending | | Bots and integrity: 5 requirements (R-SIM-690 to R-SIM-694) |
| `12_simulation_model/07_randomness_and_integrity.md §2.13` | pending | | Further per-subsystem rules: 12 requirements (R-SIM-700 to R-SIM-711) |
| `12_simulation_model/07_randomness_and_integrity.md §2.14` | pending | | Non-finite values: 3 requirements (R-SIM-715 to R-SIM-717) |
| `12_simulation_model/07_randomness_and_integrity.md §2.15` | pending | | Gameplay is computed only by the simulation: 1 requirement (R-SIM-720) |
| `12_simulation_model/07_randomness_and_integrity.md §2.16` | pending | | Settings, seasonal state and tutorials: 3 requirements (R-SIM-725 to R-SIM-727) |
| `12_simulation_model/07_randomness_and_integrity.md §2.17` | pending | | Ownership, reset and threads: 3 requirements (R-SIM-730 to R-SIM-732) |
| `12_simulation_model/07_randomness_and_integrity.md §2.18` | pending | | Hooks: command-stream replay: 4 requirements (R-SIM-740 to R-SIM-743) |
