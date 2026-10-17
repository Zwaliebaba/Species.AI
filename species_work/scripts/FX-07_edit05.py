"""FX-07: scripted edits of reference/12/05 (routes, DEC citations, A-31, O23 names). Usage: python3 -I FX-07_edit05.py <file>"""
import sys
p = sys.argv[1]
s = open(p, encoding="utf-8").read()
creat = "../02_units/creatures.md"
reps = [
# DEC-260: R-SIM-307 last sentence
("The slot order of a cell is the within-cell order of every query result (R-SIM-310, DEC-016).",
 "The slot array and its free list are storage only and decide no order: the entries of a cell are reported in ascending actor serial (R-SIM-310; see DEC-260)."),
# R-SIM-309 pseudocode and signature
("**R-SIM-309** (MUST) The exact range query `ActorGrid::InRange(x, z, r, teamMask)` returns the handles of the actors of the included teams whose current position lies strictly within distance r of (x, z) in the horizontal plane:",
 "**R-SIM-309** (MUST) The exact range query `ActorGrid::InRange(_x, _z, _r, _teamMask)` returns the handles of the actors of the included teams whose current position lies strictly within distance `_r` of (`_x`, `_z`) in the horizontal plane:"),
("""cells := every cell overlapping the square [x - r, x + r] x [z - r, z + r], clamped to the grid
result := empty list
for cellX ascending, then cellZ ascending:
    for team 0 .. kMaxTeams-1 with teamMask[team]:
        for slot 0 .. capacity-1 of that cell and layer:
            h := handle in slot; skip an empty slot
            skip h if h is already in result         # an actor with a hit shape sits in several cells
            a := resolve(h); skip h if it resolves to nothing
            if (a.x - x)^2 + (a.z - z)^2 < r^2: append h to result
return result""",
"""cells := every cell overlapping the square [_x - _r, _x + _r] x [_z - _r, _z + _r], clamped to the grid
result := empty list
for cellX ascending, then cellZ ascending:
    for team 0 .. MAX_TEAMS-1 with _teamMask[team]:
        for each handle h of that cell and layer, in ascending actor serial (R-SIM-137; DEC-260):
            skip h if h is already in result         # an actor with a hit shape sits in several cells
            a := resolve(h); skip h if it resolves to nothing
            if (a.x - _x)^2 + (a.z - _z)^2 < _r^2: append h to result
return result"""),
# R-SIM-310
("**R-SIM-310** (MUST) The result order of every actor-grid query is the stable key of DEC-016: cell x ascending, then cell z ascending, then team 0 to 5, then cell slot order (R-SIM-307).",
 "**R-SIM-310** (MUST) The result order of every actor-grid query is the stable key of DEC-016: cell x ascending, then cell z ascending, then team 0 to 5, then actor serial ascending (R-SIM-137), never the order of the cell's slots (R-SIM-307). The key depends only on which actors are registered in which cells, never on when or in which order they registered, so the registration schedule of DEC-650 has no observable effect (see DEC-260)."),
# R-SIM-311 table signatures
("| `InRange(x, z, r, teamMask)` | the mask | circular, exact (R-SIM-309) | handles |", "| `InRange(_x, _z, _r, _teamMask)` | the mask | circular, exact (R-SIM-309) | handles |"),
("| `InRange(x, z, r)` | all | circular, exact | handles |", "| `InRange(_x, _z, _r)` | all | circular, exact | handles |"),
("| `Enemies(x, z, r, team)` | every team that is not a friend of `team` | circular, exact | handles |", "| `Enemies(_x, _z, _r, _team)` | every team that is not a friend of `_team` | circular, exact | handles |"),
("| `Friends(x, z, r, team)` | every friend of `team` | circular, exact | handles |", "| `Friends(_x, _z, _r, _team)` | every friend of `_team` | circular, exact | handles |"),
("| `NearestEnemy(x, z, minR, maxR, team)` | enemies | exact within `maxR`; a candidate replaces the best so far only if minR² ≤ d² < best d² |",
 "| `NearestEnemy(_x, _z, _minR, _maxR, _team)` | enemies | exact within `_maxR`; a candidate replaces the best so far only if `_minR`² ≤ d² < best d² |"),
("| `CountInCells(x, z, r, teamMask)` | the mask | **none** | the number of distinct handles in every cell that overlaps the square [x ± r] × [z ± r] |",
 "| `CountInCells(_x, _z, _r, _teamMask)` | the mask | **none** | the number of distinct handles in every cell that overlaps the square [`_x` ± `_r`] × [`_z` ± `_r`] |"),
("| `CountEnemiesInCells(x, z, r, team)`, `CountFriendsInCells(x, z, r, team)` |", "| `CountEnemiesInCells(_x, _z, _r, _team)`, `CountFriendsInCells(_x, _z, _r, _team)` |"),
("| `AnyInCells(x, z, r[, teamMask])` |", "| `AnyInCells(_x, _z, _r[, _teamMask])` |"),
("| `EnemiesInCells(x, z, r, team)`, `FriendsInCells(x, z, r, team)` |", "| `EnemiesInCells(_x, _z, _r, _team)`, `FriendsInCells(_x, _z, _r, _team)` |"),
# RB-004 (6) neighbour-result growth, appended to R-SIM-309 note
("The test uses each actor's current position, which may already have moved during this tick, and the comparison is strict. The query uses per-call scratch storage with no fixed size limit (DEC-028).",
 "The test uses each actor's current position, which may already have moved during this tick, and the comparison is strict. The query uses per-call scratch storage with no fixed size limit (DEC-028): when the storage is full it grows by `ref.sim.actor_grid.result_step` (100) entries, or by the number of entries the caller asks for when that is larger. The growth step has no effect on any result."),
# RA-218 bot controller row; RA-097 Crawler row; RA-294 tutorial director row
("| bot controller | `EnemiesInCells(marker, 450 wu)`; `CountFriendsInCells(structure, area) < 30` and `CountEnemiesInCells(structure, area) > 50` | the under-attack test and the losing-position test | [../07_bots/README.md](../07_bots/README.md) |",
 "| bot controller | `EnemiesInCells(marker, 450 wu)` (`ref.bot.siege.engage_radius`); `CountFriendsInCells(node, 420 wu) < 30` and `CountEnemiesInCells(node, 420 wu) > 50`, where 420 wu = `ref.bot.power.meteor_factor` × `ref.power.effect_range` | the Siege defender's engagement test around markers; the meteor-shower target test | [../07_bots/05_mode_strategies_and_tutorials.md](../07_bots/05_mode_strategies_and_tutorials.md) R-BOT-136; [../07_bots/04_powers_and_unit_policies.md](../07_bots/04_powers_and_unit_policies.md) R-BOT-121 |"),
("| Crawler | `EnemiesInCells(centre, search radius)` | whether to search | [../02_units/README.md](../02_units/README.md) |",
 "| Crawler | `EnemiesInCells(centre, search radius)` | whether to search | [../02_units/creatures.md](../02_units/creatures.md) |"),
("| hint overlay | counts | presentation only; no simulation effect | [../11_tutorial_and_help.md](../11_tutorial_and_help.md) |",
 "| hint overlay | counts | presentation only; no simulation effect | [../11_tutorial_and_help.md](../11_tutorial_and_help.md) |\n| tutorial director | `Friends`, `CountEnemiesInCells` | the instruction-tab choice; world effects only through tutorial events ([06_teams_and_player_commands.md](06_teams_and_player_commands.md) R-SIM-462) | [../11_tutorial_and_help.md](../11_tutorial_and_help.md) |"),
# R-SIM-315 cache container (RB-004 (5))
("**R-SIM-315** (MUST) The actor grid cache holds a list of entries.",
 "**R-SIM-315** (MUST) The actor grid cache holds a list of entries, with room for `ref.sim.grid_cache.initial_entries` (20) entries at creation, growing by `ref.sim.grid_cache.entry_step` (20) entries when full; the capacity has no effect on any result."),
# R-SIM-316
("**R-SIM-316** (MUST) `ActorGridCache::CachedEnemies(x, z, r, team)` answers as follows:", "**R-SIM-316** (MUST) `ActorGridCache::CachedEnemies(_x, _z, _r, _team)` answers as follows:"),
("1. **Hit.** Among the entries of the same team, consider those whose centre lies strictly within the entry's half range of (x, z), that is d² < stored half range². If there is any, take the one whose centre is nearest to (x, z) and return its handle list unchanged. The requested range r is ignored on a hit,",
 "1. **Hit.** Among the entries of the same team, consider those whose centre lies strictly within the entry's half range of (`_x`, `_z`), that is d² < stored half range². If there is any, take the one whose centre is nearest to (`_x`, `_z`) and return its handle list unchanged. The requested range `_r` is ignored on a hit,"),
("2. **Miss.** Otherwise run the exact query `Enemies(x, z, r, team)` (R-SIM-311), store a new entry (x, z, r², (0.5 r)², team, count, current tick, handles), and return its list.",
 "2. **Miss.** Otherwise run the exact query `Enemies(_x, _z, _r, _team)` (R-SIM-311), store a new entry (`_x`, `_z`, `_r`², (0.5 `_r`)², `_team`, count, current tick, handles), and return its list."),
# R-SIM-321 SphereHit -> SphereHits
("        if structure.SphereHit(centre, cellSize * ref.sim.obstacle_grid.cell_test_fraction):", "        if structure.SphereHits(centre, cellSize * ref.sim.obstacle_grid.cell_test_fraction):"),
# R-SIM-335 signature, pseudocode, report variant
("**R-SIM-335** (MUST) `IsPathClear(from, to, evaluateCliffs = false, permitSteepDownhill = false)` decides whether a straight walk is possible, and also reports an enabled Barrier Emitter on the way:",
 "**R-SIM-335** (MUST) `IsPathClear(_from, _to, _evaluateCliffs = false, _permitSteepDownhill = false)` decides whether a straight walk is possible, and also reports an enabled Barrier Emitter on the way:"),
("""if from.y <= seaLevel or to.y <= seaLevel: return not clear      # the y values passed in, not the terrain
step := ref.sim.walk.step                                          # 20 wu
numSteps := trunc(|to - from| / step)
if numSteps == 0: return clear                                     # R-SIM-337
diff := (to - from) / numSteps
pos := from; wet := 0
repeat numSteps times:                                             # samples from, from + diff, ..., one step short of to
    pos.y := terrain height at pos
    if pos.y <= seaLevel: wet += step
    if wet >= ref.sim.walk.wet_limit: return not clear             # two wet samples
    if evaluateCliffs:
        g := (pos.y - from.y) / step                               # relative to the start point, never advanced
        if g > ref.sim.walk.max_climb_gradient: return not clear   # more than 48 wu above the start
        if not permitSteepDownhill or g > 0:
            if terrain normal y at pos < ref.sim.walk.min_normal_y: return not clear
    for each structure b in ObstacleGrid.StructuresAt(pos):
        if b is a wall and b.SphereHit(pos, step * ref.sim.walk.wall_clearance_factor):   # 13.2 wu
            return not clear
        if b is an enabled Barrier Emitter and b.SphereHit(pos, ref.sim.walk.emitter_report_radius):
            report b                                               # reported; it does not block""",
"""if _from.y <= seaLevel or _to.y <= seaLevel: return not clear    # the y values passed in, not the terrain
step := ref.sim.walk.step                                          # 20 wu
numSteps := trunc(|_to - _from| / step)
if numSteps == 0: return clear                                     # R-SIM-337
diff := (_to - _from) / numSteps
pos := _from; wet := 0
repeat numSteps times:                                             # samples _from, _from + diff, ..., one step short of _to
    pos.y := terrain height at pos
    if pos.y <= seaLevel: wet += step
    if wet >= ref.sim.walk.wet_limit: return not clear             # two wet samples
    if _evaluateCliffs:
        g := (pos.y - _from.y) / step                              # relative to the start point, never advanced
        if g > ref.sim.walk.max_climb_gradient: return not clear   # more than 48 wu above the start
        if not _permitSteepDownhill or g > 0:
            if terrain normal y at pos < ref.sim.walk.min_normal_y: return not clear
    for each structure b in ObstacleGrid.StructuresAt(pos):
        if b is a wall and b.SphereHits(pos, step * ref.sim.walk.wall_clearance_factor):  # 13.2 wu
            return not clear
        if b is an enabled Barrier Emitter and b.SphereHits(pos, ref.sim.walk.emitter_report_radius):
            report b                                               # reported; it does not block"""),
("Sea level is `ref.sim.physics.sea_level` (R-SIM-365).\n",
 "Sea level is `ref.sim.physics.sea_level` (R-SIM-365).\n\nThe bot link build uses a report variant of this test: it reports every Barrier Emitter whose hit volume meets the report sphere at a sample, enabled or not, and the link records the last emitter reported as its crossed emitter ([../07_bots/01_navigation_graph.md](../07_bots/01_navigation_graph.md) R-BOT-031, DEC-087). The variant blocks and passes exactly as the test above; only its report differs.\n"),
("The test is direction-dependent: `IsPathClear(a, b)` and `IsPathClear(b, a)` can differ.", "The test is direction-dependent: `IsPathClear(a, b)` and `IsPathClear(b, a)` can differ."),
# R-SIM-338
("**R-SIM-338** (MUST) `WalkCost(from, to, checkObstacles = true)` returns", "**R-SIM-338** (MUST) `WalkCost(_from, _to, _checkObstacles = true)` returns"),
("""numSteps := trunc(|to - from| / ref.sim.walk.cost_step)            # 40 wu
cost := 0; wet := 0; prev := from
for i := 1 .. numSteps:
    p := from + (to - from) * i / numSteps, with p.y := terrain height at p
    if p.y <= seaLevel: wet += ref.sim.walk.cost_step
    if wet >= ref.sim.walk.cost_wet_limit: return -1               # one wet sample
    if checkObstacles:
        for each structure b in ObstacleGrid.StructuresAt(p):
            if (b is a wall or b is an enabled Barrier Emitter) and b.SphereHit(p, ref.sim.walk.cost_obstacle_radius):""",
"""numSteps := trunc(|_to - _from| / ref.sim.walk.cost_step)          # 40 wu
cost := 0; wet := 0; prev := _from
for i := 1 .. numSteps:
    p := _from + (_to - _from) * i / numSteps, with p.y := terrain height at p
    if p.y <= seaLevel: wet += ref.sim.walk.cost_step
    if wet >= ref.sim.walk.cost_wet_limit: return -1               # one wet sample
    if _checkObstacles:
        for each structure b in ObstacleGrid.StructuresAt(p):
            if (b is a wall or b is an enabled Barrier Emitter) and b.SphereHits(p, ref.sim.walk.cost_obstacle_radius):"""),
# R-SIM-339
("Here walkable(i, j) = `IsPathClear(p_i, p_j, evaluateCliffs = checkCliffs_i and checkCliffs_j)`, and walkCost(i, j) = `WalkCost(p_i, p_j, checkObstacles = false)`,",
 "Here walkable(i, j) = `IsPathClear(p_i, p_j, _evaluateCliffs = checkCliffs_i and checkCliffs_j)`, and walkCost(i, j) = `WalkCost(p_i, p_j, _checkObstacles = false)`,"),
# R-SIM-345 to R-SIM-351
("**R-SIM-345** (MUST) `RouteGraph::BuildRoute(from, to, directRoute = false, permitClimbing = true, noRounding = false)` returns the handle of a new route, built in this order:\n\n1. The route starts with the point `from`.\n2. Find the target node (nearest to `to`) and the start node (nearest to `from`) by R-SIM-346.\n3. A complex route is possible when both nodes were found and `directRoute` is false.",
 "**R-SIM-345** (MUST) `RouteGraph::BuildRoute(_from, _to, _directRoute = false, _permitClimbing = true, _noRounding = false)` returns the handle of a new route, built in this order:\n\n1. The route starts with the point `_from`.\n2. Find the target node (nearest to `_to`) and the start node (nearest to `_from`) by R-SIM-346.\n3. A complex route is possible when both nodes were found and `_directRoute` is false."),
("5. Append the point `to`.\n6. If the route has more than 2 points and `noRounding` is false, smooth the corners (R-SIM-349).",
 "5. Append the point `_to`.\n6. If the route has more than 2 points and `_noRounding` is false, smooth the corners (R-SIM-349)."),
("a candidate qualifies only if `IsPathClear(position, node position, evaluateCliffs = true, permitSteepDownhill = false)` holds",
 "a candidate qualifies only if `IsPathClear(position, node position, _evaluateCliffs = true, _permitSteepDownhill = false)` holds"),
("**R-SIM-347** (MUST) The corridor test checks three lanes: the segment from `from` to `to`, and the same segment offset by `ref.sim.route.corridor_offset` (40 wu) to each side, along the perpendicular (from − to) × up scaled to that length. Each lane is tested with `IsPathClear(lane start, lane end, evaluateCliffs = true, permitSteepDownhill = permitClimbing)`, and `straightWalkable` holds only if all three lanes are clear. `straightCost` is `WalkCost(from, to)` with obstacles checked.",
 "**R-SIM-347** (MUST) The corridor test checks three lanes: the segment from `_from` to `_to`, and the same segment offset by `ref.sim.route.corridor_offset` (40 wu) to each side, along the perpendicular (`_from` − `_to`) × up scaled to that length. Each lane is tested with `IsPathClear(lane start, lane end, _evaluateCliffs = true, _permitSteepDownhill = _permitClimbing)`, and `straightWalkable` holds only if all three lanes are clear. `straightCost` is `WalkCost(_from, _to)` with obstacles checked."),
("If the route reaches `ref.sim.route.max_points` (100) points, a diagnostic is reported and the route is reset to the single point `from`.",
 "If the route reaches `ref.sim.route.max_points` (100) points, a diagnostic is reported and the route is reset to the single point `_from`."),
("(lanes as R-SIM-347, tested with evaluateCliffs = true and permitSteepDownhill = true)", "(lanes as R-SIM-347, tested with `_evaluateCliffs = true` and `_permitSteepDownhill = true`)"),
("and `IsPathClear(point, successor, evaluateCliffs = true)` holds,", "and `IsPathClear(point, successor, _evaluateCliffs = true)` holds,"),
("the direct-route override applies: with climbing := not `straightWalkable` and `straightCost` > 0, if `straightCost` ≥ 0 and (not climbing or `permitClimbing`) and `straightCost` ≤ the route's walking distance (R-SIM-334), the route is replaced by the two points [from, to]. The override applies only when the corridor test of R-SIM-345 step 3 ran; otherwise the route keeps the points that merging left (R-SIM-350).",
 "the direct-route override applies: with climbing := not `straightWalkable` and `straightCost` > 0, if `straightCost` ≥ 0 and (not climbing or `_permitClimbing`) and `straightCost` ≤ the route's walking distance (R-SIM-334), the route is replaced by the two points [`_from`, `_to`]. The override applies only when the corridor test of R-SIM-345 step 3 ran; otherwise the route keeps the points that merging left (R-SIM-350), so a direct route between two land points less than `ref.sim.route.merge_distance` apart with a clear path collapses to their midpoint (see DEC-263)."),
# RB-004 (3) route-generation warning in R-SIM-352
("`BuildRoute` draws no random value; its result depends only on the terrain, the structures and the node tables.",
 "`BuildRoute` draws no random value; its result depends only on the terrain, the structures and the node tables. A route generation that takes longer than `ref.sim.route.slow_generation_warning` (10 ms of real time) reports a performance diagnostic; the diagnostic changes no route and no simulation state."),
# R-SIM-353 table
("| crowd order to Workers ([06_teams_and_player_commands.md](06_teams_and_player_commands.md)) | land | `directRoute` from the command |",
 "| crowd order to Workers ([06_teams_and_player_commands.md](06_teams_and_player_commands.md)) | land | `_directRoute` from the command |"),
("| land | `directRoute` from the order; a direct route for the highlight |", "| land | `_directRoute` from the order; a direct route for the highlight |"),
("| Relic | land | `permitClimbing = false` |", "| Relic | land | `_permitClimbing = false` |"),
("| Breach Charge | land | `permitClimbing = true` |", "| Breach Charge | land | `_permitClimbing = true` |"),
("| Transport | sea | `permitClimbing = true`, `noRounding = true` |", "| Transport | sea | `_permitClimbing = true`, `_noRounding = true` |"),
# A-31 R-SIM-357
("The store therefore grows for the whole match, and the growth costs memory only, because no rule reads a route that no actor or order holds. This reference behaviour is the default of the Open DEC-651.",
 "The store therefore grows for the whole match, and the growth costs memory only, because no rule reads a route that no actor or order holds; the renderer fades a drawn route out on its own presentation timer, at `rnd.route.fade_rate` (0.25 per second, [../../architecture/components/renderer/entity_and_overlay_renderers.md](../../architecture/components/renderer/entity_and_overlay_renderers.md) RND-352). This reference behaviour is the default of the Open DEC-651. The reference does not define the moment at which a Transport frees one of its sea routes; the default is that a Transport frees its previous sea route when it creates a new route for itself, and its current sea route when it is removed (DEC-651)."),
# RA-097 R-SIM-363
("**R-SIM-363** (MUST) Every constant that a rule applies once per tick, such as the gravity of Crawler eggs and spiders and the per-tick counters of [../02_units/README.md](../02_units/README.md), is a per-second rate scheduled in ticks, so the speed outcomes change it exactly as they change every other rule (DEC-092).",
 "**R-SIM-363** (MUST) Every constant that a rule applies once per tick, such as the per-tick gravity of Seed Pods (`ref.unit.seed_pod.gravity_per_tick`) and of pouncing spiders (`ref.unit.spider.pounce_gravity_per_tick`) of [../02_units/creatures.md](../02_units/creatures.md) and the per-tick counters of [../02_units/README.md](../02_units/README.md), is a per-second rate scheduled in ticks, so the speed outcomes change it exactly as they change every other rule (DEC-092). Crawler eggs fall by the airborne step (R-SIM-370). A Crawler above the terrain falls with its own gravity `ref.unit.crawler.fall_gravity` (10 wu/s², [../02_units/creatures.md](../02_units/creatures.md) R-UNIT-530), integrated as R-SIM-362 states."),
# RB-003 R-SIM-368
("**R-SIM-368** (MUST) The valid-spawn-position search makes up to `ref.sim.spawn.search_attempts` (10) attempts around the requested point, each drawing `ref.sim.spawn.draws_per_attempt` (2) values from the simulation generator; an attempt that lies off the map is never valid.",
 "**R-SIM-368** (MUST) The valid-spawn-position search makes up to `ref.sim.spawn.search_attempts` (10) attempts around the requested point, each drawing `ref.sim.spawn.draws_per_attempt` (2) values from the simulation generator: a distance `SimRng::NextUnit(spread)` and then an angle `SimRng::NextUnit(2π)` around the requested point. An attempt is valid when it lies on the map and the terrain height there is above `ref.sim.physics.sea_level`; an attempt that lies off the map is never valid ([04_containers_and_update_order.md](04_containers_and_update_order.md) R-SIM-226 step 3)."),
# R-SIM-370 pseudocode unchanged; RA-061 R-SIM-372
("while its dead fade lowers its health by `ref.sim.physics.dead_fade_per_tick` (2) per tick, from 100 to 0 in 50 ticks;",
 "while its dead fade lowers its health by `ref.unit.corpse.fade_air_per_tick` (2) per tick ([../02_units/README.md](../02_units/README.md) R-UNIT-034), from 100 to 0 in 50 ticks;"),
("| `ref.sim.physics.dead_fade_per_tick` | 2 | health per tick | 100 to 0 in 50 ticks while airborne | R-SIM-372 | DEC-313 |\n", ""),
# R-SIM-380
("**R-SIM-380** (MUST) `PushFromObstacles(candidate, kill = true)` returns", "**R-SIM-380** (MUST) `PushFromObstacles(_candidate, _kill = true)` returns"),
("""result := candidate
if onGround: result.y := terrain height at result""", """result := _candidate
if onGround: result.y := terrain height at result"""),
("""    hit := (actor has a hit shape and b.ShapeHit(actor shape at transform(front, up, result)))
        or ((actor has no hit shape or actor is a Leader) and b.SphereHit(result, ref.sim.push.contact_radius))
    if not hit:
        previous := actor position - actor velocity * dt
        hit := b.RayTest(previous, actor front, |actor position - previous|)       # anti-tunnelling
    if not hit: next structure
    if b is an enabled Barrier Emitter and kill:""",
"""    hit := (actor has a hit shape and b.MeshHits(actor shape, transform(front, up, result)))
        or ((actor has no hit shape or actor is a Leader) and b.SphereHits(result, ref.sim.push.contact_radius))
    if not hit:
        previous := actor position - actor velocity * dt
        hit := b.RayHits(previous, actor front, |actor position - previous|)       # anti-tunnelling
    if not hit: next structure
    if b is an enabled Barrier Emitter and _kill:"""),
("    while b.SphereHit(result, ref.sim.push.clearance_radius):                   # 2.0 wu",
 "    while b.SphereHits(result, ref.sim.push.clearance_radius):                  # 2.0 wu"),
("**R-SIM-381** (MUST) When an actor touches an enabled Barrier Emitter with `kill` set:", "**R-SIM-381** (MUST) When an actor touches an enabled Barrier Emitter with `_kill` set:"),
("**R-SIM-383** (MUST) `Structure::PushFrom(position, r)` moves a position directly away from the structure's position in steps of `ref.sim.push.structure_step` (2.0 wu) while a sphere of radius r at the position hits the structure. Structure port placement calls it with r = `ref.sim.push.port_radius` (5.0 wu)",
 "**R-SIM-383** (MUST) `Structure::PushFrom(_position, _r)` moves a position directly away from the structure's position in steps of `ref.sim.push.structure_step` (2.0 wu) while a sphere of radius `_r` at the position hits the structure. Structure port placement calls it with `_r` = `ref.sim.push.port_radius` (5.0 wu)"),
# R-SIM-385 Seed Pod link
("The Seed Pod and thrown weapons bounce ([../02_units/README.md](../02_units/README.md), [../04_supply_crates_powers_and_weapons/weapons_and_projectiles.md]",
 "The Seed Pod and thrown weapons bounce ([../02_units/creatures.md](../02_units/creatures.md), [../04_supply_crates_powers_and_weapons/weapons_and_projectiles.md]"),
# R-SIM-386
("**R-SIM-386** (MUST) `TryEnterTeleport(required = none)` examines the structures of the obstacle cell under the actor's position, or only the `required` structure when one is given,",
 "**R-SIM-386** (MUST) `TryEnterTeleport(_required = none)` examines the structures of the obstacle cell under the actor's position, or only the `_required` structure when one is given,"),
# RB-057 R-SIM-387
("| terrain ray hit | the line-of-sight test, and the pointer's ground hit on the input side ([../../architecture/components/terrain.md](../../architecture/components/terrain.md)) |",
 "| terrain ray hit | the line-of-sight test, the Transport, weapon terrain hits, and the pointer's ground hit on the input side ([../../architecture/components/terrain.md](../../architecture/components/terrain.md) TER-018) |"),
# R-SIM-388
("**R-SIM-388** (MUST) `Explode(position, range r, damage, team, fromStructure)` is the only area-damage primitive. The team and structure filters and the per-weapon parameters are specified in [../04_supply_crates_powers_and_weapons/weapons_and_projectiles.md](../04_supply_crates_powers_and_weapons/weapons_and_projectiles.md). It applies, in this order of effect:",
 "**R-SIM-388** (MUST) `Explode(_position, _range, _damage, _team, _fromStructure)` is the only area-damage primitive. The team and structure filters and the per-weapon parameters are specified in [../04_supply_crates_powers_and_weapons/weapons_and_projectiles.md](../04_supply_crates_powers_and_weapons/weapons_and_projectiles.md). With r the range and damage the damage it is given, it applies, in this order of effect:"),
# Figure 2
('    A["BuildRoute from, to"] --> B["Route holds from"]', '    A["BuildRoute _from, _to"] --> B["Route holds _from"]'),
('    C --> D{"Both nodes found and directRoute false?"}', '    C --> D{"Both nodes found and _directRoute false?"}'),
('    D -->|no| G["Append to"]', '    D -->|no| G["Append _to"]'),
('    I -->|yes| J["Reset the route to from"]', '    I -->|yes| J["Reset the route to _from"]'),
('    O -->|yes| P["Route becomes from, to"]', '    O -->|yes| P["Route becomes _from, _to"]'),
# tuning table new keys
("| `ref.sim.actor_grid.footprint_factor` | 0.5 | × radius | half-width of the footprint square of an actor with a hit shape | R-SIM-308 | DEC-650 |\n",
 "| `ref.sim.actor_grid.footprint_factor` | 0.5 | × radius | half-width of the footprint square of an actor with a hit shape | R-SIM-308 | DEC-650 |\n| `ref.sim.actor_grid.result_step` | 100 | entries | growth of a query's result storage, or the requested count when larger; no fixed limit | R-SIM-309 | DEC-028 |\n| `ref.sim.grid_cache.initial_entries` | 20 | entries | capacity of the cache's entry list at creation; no effect on results | R-SIM-315 | — |\n| `ref.sim.grid_cache.entry_step` | 20 | entries | growth of the entry list when full | R-SIM-315 | — |\n"),
("| `ref.sim.route.trigger_distance` | 10 | wu | a position point counts as reached | R-SIM-354, R-SIM-355 | — |\n",
 "| `ref.sim.route.trigger_distance` | 10 | wu | a position point counts as reached | R-SIM-354, R-SIM-355 | — |\n| `ref.sim.route.slow_generation_warning` | 10 | ms of real time | performance diagnostic only; no effect on the route | R-SIM-352 | — |\n"),
("The tick period `ref.sim.tick_period` and the update slice count `ref.sim.update_slices` are defined in [01_tick_timing_and_main_loop.md](01_tick_timing_and_main_loop.md); the team-slot count `ref.sim.max_teams` is defined in [06_teams_and_player_commands.md](06_teams_and_player_commands.md).",
 "The tick period `ref.sim.tick_period` and the update slice count `ref.sim.update_slices` are defined in [01_tick_timing_and_main_loop.md](01_tick_timing_and_main_loop.md); the team-slot count `ref.sim.max_teams` is defined in [06_teams_and_player_commands.md](06_teams_and_player_commands.md); the corpse fade `ref.unit.corpse.fade_air_per_tick` is defined in [../02_units/README.md](../02_units/README.md); the creature gravities `ref.unit.seed_pod.gravity_per_tick`, `ref.unit.spider.pounce_gravity_per_tick` and `ref.unit.crawler.fall_gravity` are defined in [../02_units/creatures.md](../02_units/creatures.md); the bot radii `ref.bot.siege.engage_radius` and `ref.bot.power.meteor_factor` are defined in [../07_bots/](../07_bots/README.md) and `ref.power.effect_range` in [../04_supply_crates_powers_and_weapons/README.md](../04_supply_crates_powers_and_weapons/README.md); the route fade `rnd.route.fade_rate` is defined in [../../architecture/components/renderer/entity_and_overlay_renderers.md](../../architecture/components/renderer/entity_and_overlay_renderers.md)."),
# AC-SIM-05-02
("**AC-SIM-05-02 — Cell slot rule and result order** (R-SIM-307, R-SIM-310). Given an empty cell in one layer, when A, B and C are registered, then A holds slot 0, B slot 1 and C slot 2, and the capacity is 4. When A is unregistered and D registered, then D holds slot 0, and a query that covers the cell returns D, B, C in that order.",
 "**AC-SIM-05-02 — Cell slot rule and result order** (R-SIM-307, R-SIM-310). Given an empty cell in one layer and actors A, B, C and D with ascending actor serials, when A, B and C are registered, then A holds slot 0, B slot 1 and C slot 2, and the capacity is 4. When A is unregistered and D registered, then D holds slot 0, and a query that covers the cell returns B, C, D in that order, the order of their actor serials, whatever slots they hold."),
("a query with `directRoute` true returns [from, to];", "a query with `_directRoute` true returns [from, to];"),
("A query with `directRoute` true between two land points less than 50 wu apart", "A query with `_directRoute` true between two land points less than 50 wu apart"),
# dependencies
("[../02_units/README.md](../02_units/README.md), [../02_units/worker.md](../02_units/worker.md), [../02_units/leader_and_formations.md](../02_units/leader_and_formations.md), [../02_units/transport.md](../02_units/transport.md), [../02_units/strike_team.md](../02_units/strike_team.md), [../02_units/README.md](../02_units/README.md);",
 "[../02_units/README.md](../02_units/README.md), [../02_units/worker.md](../02_units/worker.md), [../02_units/leader_and_formations.md](../02_units/leader_and_formations.md), [../02_units/transport.md](../02_units/transport.md), [../02_units/strike_team.md](../02_units/strike_team.md), [../02_units/creatures.md](../02_units/creatures.md), [../02_units/hazards_and_projectiles.md](../02_units/hazards_and_projectiles.md);"),
("[../07_bots/README.md](../07_bots/README.md); [../08_maps_and_level_design/rules_and_conventions.md]",
 "[../07_bots/README.md](../07_bots/README.md), [../07_bots/01_navigation_graph.md](../07_bots/01_navigation_graph.md), [../07_bots/04_powers_and_unit_policies.md](../07_bots/04_powers_and_unit_policies.md), [../07_bots/05_mode_strategies_and_tutorials.md](../07_bots/05_mode_strategies_and_tutorials.md); [../08_maps_and_level_design/rules_and_conventions.md]"),
("- **Engine components:** [../../architecture/components/terrain.md](../../architecture/components/terrain.md) (height, normal and ray queries),",
 "- **Engine components:** [../../architecture/components/terrain.md](../../architecture/components/terrain.md) (height, normal and ray queries; TER-018),"),
("(test-build checks), [../../architecture/components/renderer/entity_and_overlay_renderers.md](../../architecture/components/renderer/entity_and_overlay_renderers.md) (route display).",
 "(test-build checks), [../../architecture/components/renderer/entity_and_overlay_renderers.md](../../architecture/components/renderer/entity_and_overlay_renderers.md) (route display and fade, RND-352)."),
# open decisions
("| DEC-247 | Terrain scale and large-world terrain inputs | one set of grids and route graphs per map (R-SIM-304) |\n",
 "| DEC-247 | Terrain scale and large-world terrain inputs | one set of grids and route graphs per map (R-SIM-304) |\n| DEC-263 | Route and actor-group edge cases | a route built without a corridor test keeps the points that merging leaves (R-SIM-345, R-SIM-350, R-SIM-351) |\n"),
("| DEC-651 | Lifetime of simulation routes | routes kept until the match world is destroyed (R-SIM-357) |",
 "| DEC-651 | Lifetime of simulation routes | routes kept until the match world is destroyed; a Transport frees its previous sea route on a new route and its current one on removal (R-SIM-357) |"),
("Resolved decisions applied in §2: DEC-004, DEC-005, DEC-006, DEC-007, DEC-008, DEC-012, DEC-014, DEC-016, DEC-028, DEC-039, DEC-087, DEC-092, DEC-093, DEC-095, DEC-650, DEC-653.",
 "Resolved decisions applied in §2: DEC-004, DEC-005, DEC-006, DEC-007, DEC-008, DEC-012, DEC-014, DEC-016, DEC-028, DEC-039, DEC-087, DEC-092, DEC-093, DEC-095, DEC-260, DEC-650, DEC-653."),
]
bad = 0
for a, b in reps:
    n = s.count(a)
    if n != 1:
        print("COUNT", n, "for:", a[:90].replace("\n", "|"))
        bad += 1
        continue
    s = s.replace(a, b)
if bad:
    sys.exit(1)
open(p, "w", encoding="utf-8").write(s)
print("ok", len(s.encode()))
