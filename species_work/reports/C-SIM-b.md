# C-SIM-b report

Package: `species_work/packages/C-SIM-b.json` (R-SIM 300–499). All 22 input ranges are rewritten. Both pattern scans print nothing. The package checker (`species_work/scripts/C-SIM-b_check.py`) passes: IDs are in range and unique; tuning keys are unique; every cited DEC and ADR exists; every `decs_to_cite` entry is cited inside a requirement; all template headings are present; every diagram has a caption; no truncation words.

## Files written

| File | Size | Requirements | Tuning keys | Diagrams |
|---|---|---|---|---|
| `design_next/reference/12_simulation_model/05_space_routing_and_physics.md` | 75.2 KB | 90 (R-SIM-300–389) | 60 | 4 |
| `design_next/reference/12_simulation_model/06_teams_and_player_commands.md` | 68.9 KB | 71 (R-SIM-390–460) | 56 | 5 |

Totals: 161 requirements, 116 tuning keys, 9 Mermaid diagrams, 37 acceptance criteria (AC-SIM-05-01…19, AC-SIM-06-01…18), 23 + 23 edge cases. There are 26 tables, and they enumerate: the spatial structures, the actor-grid query catalogue, the 15 callers of approximate queries, the obstacle-grid users, the route graphs, the route-point kinds, the `BuildRoute` callers, the ray tests, team ids, team kinds, lobby-slot fields, match-team fields, ownership per object family, the 10-entry colour table, the `TeamCommandState` fields, 17 match commands, 9 match-setup commands, 3 reserved commands and 8 string-id rows. R-SIM-499 is the top of the range; IDs 461–499 are still free.

## Open DECs cited

- From `decs_to_cite`: DEC-089, DEC-094, DEC-200, DEC-202, DEC-228, DEC-241, DEC-243, DEC-244, DEC-246, DEC-247, DEC-313, DEC-450, DEC-451, DEC-557, DEC-651.
- Also cited where they apply: DEC-080, DEC-107, DEC-203, DEC-205, DEC-206, DEC-207, DEC-311, DEC-558.
- Resolved DECs stated as fixed rules: DEC-002, 004, 005, 006, 007, 008, 011, 012, 014, 016, 023, 028, 031, 032, 035, 036, 038, 039, 082, 083, 086, 087, 092, 093, 095, 117, 118, 119, 464, 555, 650, 652, 653.

## New DEC requests

1. **Order within a cell of actor-grid results.** DEC-016 makes "cell slot" the last key of the stable result order, but slot order depends on registration history (LIFO slot reuse). DEC-650 removes the per-tick re-insertion of shaped actors and says the refresh schedule has no observable effect. Those two statements hold together only if the order within a cell is history-free. R-SIM-307/310 keep cell-slot order under DEC-650's registration events. The decision: keep that, or order by stable actor id within a cell.
2. **`BuildRoute` when no corridor test runs** (`directRoute`, or a node not found). The inputs do not give the default values of the straight-walk flag and cost. R-SIM-351 applies the override only after a corridor test ran. As a result, a direct route between two points less than 50 wu apart collapses to one midpoint in the merge step (AC-SIM-05-09). Needs confirming.
3. **Actor group with no reporting member in slice 0.** The centre is computed as sum ÷ count, and this case is not in DEC-653's list. R-SIM-427 keeps the previous centre and velocity. Proposed: add it to DEC-653.
4. **`ActorGroup::Attack` with no candidate.** The candidate draw would index an empty list. R-SIM-428 says that no member attacks and no draw is made. Needs confirming.
5. **Obstacle push on a structure's base point.** The input infers an endless push loop, but DEC-241 item 6 (setting the length of a near-zero vector sets only x) makes the loop end. R-SIM-382 follows DEC-241 under DEC-014. Needs confirming.
6. **Co-op setup colour command.** After co-op setup, the local participant sends `SetColour` for its own team. This is a command issued from the match-setup path, the same family of problem as DEC-119. R-SIM-420 states that setup applies the colour itself; decide whether Species drops the extra command.

Interpretations to confirm (no DEC needed unless rejected):
- the turret fire delay on selection, 0.5 s, is taken as 6 ticks by the DEC-002 countdown arithmetic;
- drowning: 101 ticks; the input says "after 10 s, −500", and R-SIM-376 applies the damage on every tick after 100 ticks in water;
- the `WalkCost` gradient is taken as rise over run between samples;
- the shared walkability memo answer is the lower-index → higher-index direction, which follows from the loop order;
- a node chain includes the target node's position.

## ADR conflicts

None. ADR-0059 is cited for validated collision data.

## Missing assets

None. `assets_for_you` was empty. The teleport-entry and selection sounds are cited as presentation events through `../10_audio_design.md`, without asset ids.

## Cross-document requests

- **C-SIM-c (01, 07):** `ref.sim.tick_period` and `ref.sim.update_slices` are cited from 01, not defined here. 06 defines `ref.sim.max_teams`, the colour table (`ref.sim.team_colour.*`) and `ref.sim.command.control_state_period`. 05 defines the grid, walkability and formation constants (`ref.sim.actor_grid.*`, `ref.sim.obstacle_grid.*`, `ref.sim.walk.*`, `ref.sim.group.*`). Their constant tables (ORIG 20 §12.2 and 22 §16) should cite these keys rather than define them again. 06 R-SIM-458 cites "the pause ramp of 01" for the speed-request rate of 0.2.
- **C-SIM-a (02, 03, 04):** 05 and 06 link to 04 for the init step order (obstacle grid before route graphs and bot links), grid upkeep after each advance, slice 7 (cache eviction) and team initialisation. 05 defines `ref.sim.grid_cache.evict_slice` = 7. The actor-grid registration rule (DEC-650) is R-SIM-308, so 04 should cite it. 05 R-SIM-368 states the spawn search only as far as the inputs give it (10 attempts × 2 draws, off-map attempts invalid, fallback clamp); the full validity test of an attempt belongs to C-SIM-a's §5 if it has one.
- **G06 (R-CTRL):** the command ids to cite are `PlayerCommand::*` (06 R-SIM-450–452). The `TeamCommandState` field names are in R-SIM-439. Promote, demote and scrap are separate commands (DEC-086). The selection circle and the crowd-order timestamp are local input state (R-SIM-403). The selection presentation feedback (cursor arrows ×2.0) is referenced from R-SIM-405.
- **G02a1 (worker, leader):** the Leader's route rebuild "more than 50 wu from its route start" and the Worker grenade thresholds (radius 50, more than 5 enemies, fewer than 2 friends) are repeated in 05 R-SIM-313/353 without keys, so they need keys in the owning documents. The 25-tick ground corpse fade is theirs; the 2-per-tick airborne fade is defined here as `ref.sim.physics.dead_fade_per_tick`.
- **G04:** 05 defines the thrown-weapon gravity as `ref.sim.physics.thrown_gravity` (98 wu/s²) and the explosion primitive's reach and gain keys. G04 should cite these keys or the coordinator should dedupe them. The explosion-to-actor vector `dir` used in knock-back is deferred to G04.
- **G03b:** the smoke-column size (13 wu, DEC-089) is referenced, not defined. The turret fire delay on selection is `ref.sim.team.turret_select_delay` (6 ticks).
- **G07:** the bot controller's in-map position (DEC-087) and the radii and thresholds of the bot approximate-query tests (450 wu, < 30 friends, > 50 enemies) are referenced.
- **G09a (R-META):** the match-setup commands (`SetMatchOption`, `SetMatchOptionElement`, `SetMatchOptionText`, `SetMatchOptionArray`, `SetTeamName`, `SetColour`, `SetAchievementMask`, `SetSeasonalTheme`, `StartMatch`, `ToggleReady`, `Chat`) are defined in 06 R-SIM-451. Option lists and version rules are left to R-META and DEC-243.
- **X-AST (pass 2), string ids:** `bot.label`, `match.team_left`, `match.player_left`, `ui.lobby.player_joined`, `ui.team.invalid_name`, `faction.rogue.name`, `faction.visitor.name`, `team.colour.{green,red,yellow,blue,orange,purple,cyan,pink}`.

## Names needed that are not in the ledger (proposals)

| Proposed Species name | Kind | For |
|---|---|---|
| `PlayerCommand` and its members (`ControlState`, `Select`, `AimBeamDish`, `ToggleBarrierEmitter`, `TargetPower`, `SelectPower`, `CancelPower`, `ScrapActor`, `SelectWorkers`, `OrderWorkers`, `PromoteWorker`, `DemoteLeader`, `OrderUnit`, `ToggleUnit`, `ToggleReady`, `Pause`, `Chat`, `StartMatch`, `SetTeamName`, `SetColour`, `SetAchievementMask`, `ReportScores`, `SetMatchOption`, `SetMatchOptionElement`, `SetMatchOptionText`, `SetSeasonalTheme`, `SetMatchOptionArray`, `NextMode`, `PreviousMode`) | enumeration | the command catalogue; the ledger note already names `PlayerCommand` |
| `RoutePath` | engine type | a generated route |
| `RouteHop`, `RoutePointKind` | engine type | a route-table entry; the route-point kinds |
| `GroupFormation::Rectangle`, `GroupFormation::Airstrike` | enumeration | the actor-group formations |
| `IsPathClear`, `WalkCost`, `BuildRoute`, `PushFromObstacles`, `TryEnterTeleport`, `Bounce`, `Explode`, `Structure::PushFrom`, `ObstacleGrid::StructuresAt`, `ActorGridCache::CachedEnemies` | functions | the walkability, route, push, teleport, bounce and explosion primitives |
| `ActorGrid::InRange`, `Enemies`, `Friends`, `NearestEnemy`, `CountInCells`, `CountEnemiesInCells`, `CountFriendsInCells`, `AnyInCells`, `EnemiesInCells`, `FriendsInCells` | functions | the actor-grid queries |
| the `TeamCommandState` fields `move_target` … `direct_target_vector` | fields | R-SIM-439 |
| "no-team value", with an identifier such as `TeamId::None` | value | the neutral team (DEC-464) |
| "Visitor faction's pulse wave" | effect | the conversion wave of the Visitor faction; it is unclear whether this is `CorePulseWave` |
| "valid-spawn-position search" | function | the spawn search of R-SIM-368 |

## Dropped as out of scope, one line per group

- **Single-player paths:** the 64 wu single-player obstacle cell, level-file routes and the single-player route refusal, the single-player friendship table, and identity start positions outside matches. Reason: campaign only.
- **Transport and session material:** the control-state bit flags, packing and value types; message codes and field codes; the relay bookkeeping (addresses, sequence ids, history, acknowledgements, advertising); the join, leave, team-request, spectator-request and spectator-assign, synchronise, ping (no caller) and file-send commands. Reason: networking and lobby protocol (DEC-200, DEC-207, DEC-244).
- **Commands of dropped or unreachable features:** the test-only set-ready command, the summon command of the dropped Summoner (DEC-062), Leader follow and stop (single-player Leader controls), and run-Power-by-kind (icon and gesture interfaces, DEC-055).
- **Dropped kinds and dead helpers:** the infantry-group target stepping, the bridge teleport, specials of dropped kinds, clamp exceptions for dropped kinds, the hatchery push caller, the cliff push and the actor–actor push (no live callers).
- **Presentation and debug rendering inside simulation code:** the grid and cache debug renderers, the explosion deformation punch (alternative renderer), the Crawler sprite atlas built at team initialisation, the route-rendering fields (not in range), the slow-route performance log, and the turret-aim camera mode. The last is kept only as a presentation event.
- **Licensing and diagnostics fields of the lobby slot:** the trial-player flag and the sync-forensics records.
- **Container and language mechanics:** query output growth, container step sizes, the reference binding of team fields (replaced by the shared-record rule R-SIM-397) and the leaking team-type string helper.
- **Provenance:** every source citation, evidence and build tag, the docs-vs-code note on virtual players, and the resolved review notes.
