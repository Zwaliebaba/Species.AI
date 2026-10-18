# Adoption ledger: 12 simulation model

> Status: Draft · Owner: area D2 · Reference: [12_simulation_model](../../reference/12_simulation_model/README.md) · Rules: `prompts/species-design-phase.md` §5.3

87 sections, 424 requirements. A section row covers every requirement under its heading; an exception row (scope: one `R-` id) overrides it. Verdicts: `adopt`, `adopt-skirmish`, `adapt` (names the Species requirements that replace it), `drop` (gives the reason).

The rows of 01 (tick timing) and 07 (randomness and integrity) are area A2's, for the tick, the numeric model and determinism (2026-10-18; ADR-079 to ADR-088). Where A2 drops a reference requirement in favour of an ADR, the row names the ADR as its replacement, and area D2 turns it into `adapt` when its world-simulation requirements exist; the other rows are D2's.

| Scope | Verdict | Species requirements | Note |
|---|---|---|---|
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.1` | adopt |  | The reference tick rate: 6 requirements (R-SIM-500 to R-SIM-505). Species keeps 10 ticks per second for the world and for skirmish (ADR-079, DEC-227), so every tick-denominated reference value binds as written; the one rescaling rule of R-SIM-505 is fixed by ADR-079 item 5 for any later change |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.2` | adopt |  | The tick and its update slices: 7 requirements (R-SIM-510 to R-SIM-516). The server executes every tick whole, its ten slices back to back in slice order, in each state owner (ADR-079 item 2, ADR-083), so every slice-dependent order of the reference holds; no slice is spread over render frames |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.3` | drop |  | The reference tick schedule: 6 requirements (R-SIM-520 to R-SIM-525). The reference tick source is a sequencer polled by the host's loops, whose rate falls with the host's frame rate (R-SIM-523); Species replaces it with the server tick loop of ADR-083 item 7, which keeps the scheduled-time anchoring and the never-skipped tick of R-SIM-520 and R-SIM-522 and bounds catch-up. Replacement: ADR-083; area D2 restates it as world-simulation requirements and turns this row into adapt |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.4` | drop |  | Slices per render frame and catch-up: 7 requirements (R-SIM-530 to R-SIM-536). No client steps the simulation (ADR-079 item 2, ADR-084): a participant's slice pacing, its catch-up state and the render-rate-dependent latency of R-SIM-536 do not exist in Species. Replacement: the server catch-up of ADR-083 item 7 and the client interpolation of ADR-084 item 4; area D2 restates them and turns this row into adapt |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.5` | adopt-skirmish |  | The tick clock and game speed: 10 requirements (R-SIM-540 to R-SIM-549). The world game has no game speed and no speed request (PIL-085); skirmish keeps game speed as tick scheduling on the server's tick loop (ADR-079). R-SIM-547 binds both forms (exception row) |
| `R-SIM-547` | adopt |  | Simulation time is the tick counter in both forms of play; in the world game the world tick counts world time, which advances only while the server runs (PIL-066, PIL-076) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.6` | adopt-skirmish |  | The held world, the start hold, pause and the end of a match: 8 requirements (R-SIM-550 to R-SIM-557). A world has no pause, no start hold and no end of match (PIL-085); a frozen world is a stopped server, not a held world (PIL-076) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.7` | drop |  | Render-time extrapolation: 6 requirements (R-SIM-560 to R-SIM-565). Species interpolates every replicated object between its two latest received states, one tick period plus a jitter allowance behind the newest (ADR-084 item 4, ADR-079 item 4, DEC-222), so no extrapolation time exists; the rule of R-SIM-564 that presentation never stores a drawn position back holds for interpolated positions (ADR-084 item 3). Replacement: ADR-084; the renderer's owning area amends RND-002 and RND-018, and area D2 turns this row into adapt |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.8` | adopt |  | Clocks: 4 requirements (R-SIM-570 to R-SIM-573). Kept, except the extrapolation-time row of R-SIM-570's table, which the drop of §2.7 removes and the interpolation fraction of ADR-084 replaces; in the world game match time reads as world time (PIL-066) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.9` | adopt |  | Render rate and the frame-rate counter: 2 requirements (R-SIM-575 to R-SIM-576) |
| `12_simulation_model/01_tick_timing_and_main_loop.md §2.10` | adopt |  | The in-match frame: 5 requirements (R-SIM-580 to R-SIM-584). Kept for the client frame, except the step order of R-SIM-581 and its consequences in R-SIM-583 (exception rows) |
| `R-SIM-581` | drop |  | The in-match frame polls the sequencer (step 9) and runs the simulation's slices (step 11.2); a Species client does neither. Replacement: the client frame of ADR-084, which keeps the order of every presentation step and replaces steps 9 and 11.1 to 11.3 by sending the control state and the commands (ADR-087 item 5) and taking the newest replicated states from the transport; area D2 restates it |
| `R-SIM-583` | drop |  | States the consequences of R-SIM-581's order for input, simulation and rendering; replaced with it by ADR-084 (input is sent before the newest replicated state is taken, and rendering comes last) |
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
| `12_simulation_model/07_randomness_and_integrity.md §2.1` | adopt |  | Scope of the integrity rules: 2 requirements (R-SIM-600 to R-SIM-601). In Species "every participant" reads as every process that simulates: the server, the test runner and a defect reproduction; the builds and data paths that must agree bit for bit are ADR-081's Level 1 and Level 2 (DEC-240) |
| `12_simulation_model/07_randomness_and_integrity.md §2.2` | adopt |  | Random streams: 9 requirements (R-SIM-605 to R-SIM-613). The presentation generators, the terrain's private generators, the absence of lobby draws and the draw log are kept; the simulation generator's structure changes (exception rows) |
| `R-SIM-605` | drop |  | One sequential simulation stream per match is replaced by counter-based draws keyed by seed, purpose, subject, tick and ordinal (ADR-082, DEC-242); the draw forms `NextUint`, `NextUnit(r)` and `NextSigned(r)` and the x-then-y-then-z order of vector draws are kept on the keyed draw context `SimRng` (ADR-082 item 3). Area D2 restates it and turns this row into adapt |
| `R-SIM-606` | drop |  | Its default of one sequential stream is decided otherwise by DEC-242 (ADR-082); its statement that every distribution, not the reference draw order, is reproduced holds in ADR-082 |
| `R-SIM-610` | adopt-skirmish |  | A skirmish match is seeded once at match start, with the seed recorded. In the world game the world seed (WLD-006), recorded with the world constants (WLD-009), keys every simulation draw for the world's life (ADR-082) |
| `R-SIM-612` | adopt-skirmish |  | The draw order of match-world initialisation binds skirmish. World creation draws by WLD-161's keyed values, independent of order (ADR-082 item 4) |
| `12_simulation_model/07_randomness_and_integrity.md §2.3` | adopt |  | Draw discipline: 9 requirements (R-SIM-620 to R-SIM-628). One draw per statement, defined orders, capped rejection loops and no draw in a constructor bind the keyed draws of ADR-082 as written |
| `R-SIM-627` | adopt-skirmish |  | The actor-group offset table is generated at match initialisation; in the world game it is state of its owner, generated when the owner is created (ADR-083 item 4), which area D2 specifies |
| `12_simulation_model/07_randomness_and_integrity.md §2.4` | adopt |  | What each object family draws: 2 requirements (R-SIM-630 to R-SIM-631). Each row of R-SIM-630's table becomes one or more draw purposes of ADR-082 item 2 |
| `12_simulation_model/07_randomness_and_integrity.md §2.5` | adopt |  | Simulation state and presentation state: 3 requirements (R-SIM-635 to R-SIM-637) |
| `12_simulation_model/07_randomness_and_integrity.md §2.6` | adopt |  | Presentation randomness inside simulation code: 1 requirement (R-SIM-640) |
| `12_simulation_model/07_randomness_and_integrity.md §2.7` | adopt |  | Tree crown hit volumes: 2 requirements (R-SIM-650 to R-SIM-651) |
| `12_simulation_model/07_randomness_and_integrity.md §2.8` | adopt |  | Presentation never writes simulation state: 2 requirements (R-SIM-655 to R-SIM-656). The enforcing mechanism of DEC-222 is ADR-084: the client reaches the server only through the transport, and the build rejects forbidden includes |
| `12_simulation_model/07_randomness_and_integrity.md §2.9` | adopt |  | Local-only state never enters the simulation: 10 requirements (R-SIM-660 to R-SIM-669). Every local action reaches the server as a validated command (ADR-087) |
| `12_simulation_model/07_randomness_and_integrity.md §2.10` | adopt |  | Layering rules: 6 requirements (R-SIM-670 to R-SIM-675). Commands and state cross processes as ADR-086 and ADR-087 state; R-SIM-672's extrapolation clause changes (exception row) |
| `R-SIM-672` | drop |  | Presentation reads only replicated state and never writes it, but draws it interpolated (ADR-084 items 3 and 4), not extrapolated as R-SIM-672 states. Replacement: ADR-084; area D2 turns this row into adapt |
| `12_simulation_model/07_randomness_and_integrity.md §2.11` | adopt |  | Rules the object model states: 6 requirements (R-SIM-680 to R-SIM-685) |
| `12_simulation_model/07_randomness_and_integrity.md §2.12` | adopt-skirmish |  | Bots and integrity: 5 requirements (R-SIM-690 to R-SIM-694). Skirmish bots run inside the simulation on the server as written. In the world game the AI of AI settlements, the deputy and the aide reads only what its settlement sees (PIL-084) and issues validated commands that execute at the next tick (ADR-087 item 1); area B7 restates the integrity rules for it |
| `12_simulation_model/07_randomness_and_integrity.md §2.13` | adopt |  | Further per-subsystem rules: 12 requirements (R-SIM-700 to R-SIM-711). Real time decides only presentation and scheduling; the slice-pacing and extrapolation examples of R-SIM-700 and R-SIM-711 read as the server's tick scheduling and the client's interpolation (ADR-083, ADR-084) |
| `12_simulation_model/07_randomness_and_integrity.md §2.14` | adopt |  | Non-finite values: 3 requirements (R-SIM-715 to R-SIM-717). The guards hold under the numeric model of ADR-081 |
| `12_simulation_model/07_randomness_and_integrity.md §2.15` | adopt |  | Gameplay is computed only by the simulation: 1 requirement (R-SIM-720). No third-party code runs inside a tick (ADR-081 item 4) |
| `12_simulation_model/07_randomness_and_integrity.md §2.16` | adopt |  | Settings, seasonal state and tutorials: 3 requirements (R-SIM-725 to R-SIM-727). A world's settings (WLD-006) are its match options in this sense |
| `R-SIM-726` | adopt-skirmish |  | A skirmish match's seasonal state is match state. A world's seasonal state is world state decided by its server (WLD-185) |
| `12_simulation_model/07_randomness_and_integrity.md §2.17` | adopt |  | Ownership, reset and threads: 3 requirements (R-SIM-730 to R-SIM-732). The owners are those of ADR-083 item 4 (region, world, match), and the serial result at any thread count is ADR-081's Level 1 |
| `12_simulation_model/07_randomness_and_integrity.md §2.18` | adopt |  | Hooks: command-stream replay: 4 requirements (R-SIM-740 to R-SIM-743). R-SIM-742 and R-SIM-743 stand: a replay needs the build, the content hashes and the seed, and divergence checking is DEC-206, decided by ADR-081 item 8; the sequencer hooks change (exception rows) |
| `R-SIM-740` | drop |  | The developer hook records and plays back the sequencer's command history; Species has no sequencer. Replacement: every server keeps a command journal of the last 10 minutes with the checkpoint before it (ADR-088 item 7), which a developer build replays on the same build (ADR-081 Level 1); area D2 turns this row into adapt |
| `R-SIM-741` | drop |  | A recording without its seed and setup cannot reproduce a match; the Species journal starts from a checkpoint that holds the seed (ADR-088 item 7). Player-facing battle replays are recorded state streams (ADR-086 item 6, DEC-046) |
