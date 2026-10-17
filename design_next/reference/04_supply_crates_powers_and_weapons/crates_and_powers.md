# Supply crates and powers

> Status: Reference · Prefix: R-CRATE · Related: [folder README](README.md) · [weapons and projectiles](weapons_and_projectiles.md) · [01 Skirmish rules and modes](../01_skirmish_rules_and_modes/README.md) · [02 Units](../02_units/README.md) · [hazards and projectiles](../02_units/hazards_and_projectiles.md) · [creatures](../02_units/creatures.md) · [03 Buildings and world objects](../03_buildings_and_world_objects/README.md) · [05 Population flow](../05_population_flow.md) · [07 Bots](../07_bots.md) · [12 Simulation model](../12_simulation_model/README.md)

This file specifies the life of a Supply crate from its drop to its opening or expiry, the choice of its outcome, the two other grant paths (starting powers and the Comeback Bonus), and the effect of every outcome of the catalogue ([README](README.md) table 2.2-A). The Power slots, Power targeting, the feedback conventions, the map's excluded outcomes and the basic-outcomes option are shared rules of the [README](README.md); the projectiles that outcomes release are in [weapons and projectiles](weapons_and_projectiles.md).

## 1 Purpose and player experience

A Supply crate appears high above the map, announced once per match, and drifts down for about half a minute inside a translucent bubble with a fiery trail, over a rotating landing marker on the ground. Players see where it will land and race Workers to it. On the ground, the team with the most Workers within a short radius takes ownership and fills an opening ring; a rival must first drain that ring by outnumbering the owner before its own ring starts. With 50 or more Workers a crate opens in about five seconds; with a handful it takes minutes. A crate that nobody holds expires after a minute on the ground.

When it opens, nine times in ten the opening team receives a good outcome: a Power to aim later (a Strike team, a Transport, an airstrike, a turret, a Shield and others) or an immediate effect (50 to 99 Workers, a grove of Essence-orb trees, a burst of extra crates, a map-wide Spawn Surge, a change of game speed). One time in ten a bad outcome erupts at the crate instead: a homing Swarm, Sickness, a Hive, a Blight Grove, Spitter Plants, the Allegiance Scrambler or, once per match, the Abductor, which threaten every team.

## 2 Rules

### 2.1 Options that control Supply crates

**R-CRATE-100** (MUST) These match and map settings control Supply crates. Each is a typed option with an explicit default per mode (DEC-073):

| Setting | Values and default | Owner | Effect here |
|---|---|---|---|
| crate drop period | seconds, 0 to 300 in steps of 10; default `ref.crate.drop.period_default_s` (40 s) in all six modes; 0 means no Supply crates | lobby option, [reference/01](../01_skirmish_rules_and_modes/README.md) | the mean drop interval (R-CRATE-103) |
| crate placement | random or weighted; default `ref.match.option.crate_placement` | lobby option, [reference/01](../01_skirmish_rules_and_modes/README.md) | the placement rule (R-CRATE-105) |
| starting powers | 0 to 3; default `ref.econ.start.powers` (0); Spawn Control and Zone Control only | lobby option, [reference/05](../05_population_flow.md) R-ECON-110 | §2.12 |
| Comeback Bonus | on or off; on by default | lobby option, [reference/01](../01_skirmish_rules_and_modes/README.md), [reference/05](../05_population_flow.md) R-ECON-112 | §2.13 |
| basic outcomes only | off by default | lobby option ([README](README.md) R-CRATE-031) | the pools and the bad branch |
| excluded outcomes | a set of outcomes; none by default | map definition ([README](README.md) R-CRATE-029) | validity (R-CRATE-129) |
| Transport cap of the map's Transit Gates | absent, 0, a number, or unlimited (DEC-120) | map definition, [reference/05](../05_population_flow.md) | validity of the Transport outcome (R-CRATE-129) |
| exclusion zones | map structures with a radius and a Power-blocking flag | map definition ([README](README.md) R-CRATE-032) | random placement (R-CRATE-107); Power targeting |

**R-CRATE-101** (MUST) A map definition never places a Supply crate, and no map data sets a Supply crate's outcome: Supply crates exist only as the drop rule, the Supply Storm outcome and tutorial steps create them ([reference/03](../03_buildings_and_world_objects/README.md) R-BLDG-011).

### 2.2 Drop timer

**R-CRATE-102** (MUST) The drop rule runs once per world tick in update slice `ref.sim.slice.supply_crates` (8). It does nothing during the first tutorial, and during the second tutorial until its bot is released ([reference/11](../11_tutorial_and_help.md)). It is not gated by the grace period; it runs on every tick on which the world runs, and no world tick runs while the world is held ([reference/12](../12_simulation_model/01_tick_timing_and_main_loop.md) R-SIM-550). Drops that the grace period does not stop are the reference default of DEC-116.

**R-CRATE-103** (MUST) Each run of the drop rule:

```
maxCrates := floor(world size x / ref.crate.drop.map_width_per_crate)      # 500 wu; the x size only
if liveCrates >= maxCrates: return                                           # the countdown is frozen at the cap
D := crate drop period in seconds; if D = 0: return                          # no Supply crates
if countdown is unset: countdown := Interval(D)                              # the first interval
countdown -= 1                                                               # one tick
if countdown <= 0:
    countdown := Interval(D)
    TryDrop()                                                                # R-CRATE-105; may place nothing
Interval(D) := ceil(10 * D * (ref.crate.drop.interval_factor_max - SimRng::NextUnit(ref.crate.drop.interval_factor_spread)))   # ticks
```

`liveCrates` counts every Supply crate in the world, falling or on the ground. The countdown is unset at match start (DEC-011).

**R-CRATE-104** (MUST) With `ref.crate.drop.interval_factor_max` (1.5) and `ref.crate.drop.interval_factor_spread` (1.0), an interval is uniform over (0.5, 1.5] × the drop period: 200 to 600 ticks (20 to 60 s), mean 400 ticks, at the default of 40 s. The cap is 4 crates on a map 2,000 wu wide, 6 at 3,000 wu and 10 at 5,000 wu. Because game speed only schedules ticks, Time Dilation and Time Rush stretch or compress the real time between drops (DEC-039).

### 2.3 Placement

**R-CRATE-105** (MUST) `TryDrop()` uses the weighted placement of R-CRATE-106 when the crate placement option is weighted, and the random placement of R-CRATE-107 otherwise, or when the weighted placement falls back to it.

**R-CRATE-106** (MUST) **Weighted placement** favours the structures of lower-scoring teams:

1. The weighted teams are the used teams that are neither the Rogue nor the Visitor faction, in slot order, with score s_t ([reference/01](../01_skirmish_rules_and_modes/README.md)). Let n be their number, total = Σ s_t, high = max s_t and low = min s_t.
2. When total > 0: p = max(high ÷ total, 1 − low ÷ total), and each team's weight is w_t = total ÷ s_t, or total when s_t = 0. When total = 0, every score counts as equal: p = max(1 ÷ n, 1 − 1 ÷ n) and every team's weight is 1 (DEC-465). No step divides by zero or stores a non-finite value (DEC-653).
3. Draw u = `SimRng::NextUnit(1)`. When u > p, fall back to random placement.
4. Draw v = `SimRng::NextUnit(1)` and choose the first team, in slot order, whose cumulative weight share Σ w ÷ Σ w reaches above v.
5. The candidates are every structure of the chosen team, of any kind. With none, fall back to random placement.
6. Choose a structure with `SimRng::NextUint` mod (candidate count). The drop point is its position + (`SimRng::NextSigned`(`ref.crate.drop.weighted_scatter`), 0, `SimRng::NextSigned`(`ref.crate.drop.weighted_scatter`)), within ±100 wu on x and z.
7. The first candidate point is taken whatever the terrain there: a weighted drop can land in water, or up to 100 wu outside the map when the structure is near an edge. This is the reference default of DEC-114; its option B validates the candidate against land of at least `ref.crate.drop.min_land_height` inside the map, for up to `ref.crate.drop.weighted_attempts` (1000) candidates, with a deterministic fallback (DEC-014).
8. A weighted drop tests neither exclusion zones nor the spacing to other crates.
9. Its start height is `ref.crate.drop.start_height` (1300 wu) + `SimRng::NextUnit`(`ref.crate.drop.start_height_spread`) (200 wu).

**R-CRATE-107** (MUST) **Random placement** draws up to `ref.crate.drop.random_attempts` (100) candidate points. Each candidate is (`SimRng::NextUnit`(world size x), `ref.crate.drop.start_height` + `SimRng::NextUnit`(`ref.crate.drop.start_height_spread`), `SimRng::NextUnit`(world size z)), with the height drawn after x and z. A candidate is accepted when all of these hold:

1. the terrain height at its (x, z) is at least `ref.crate.drop.min_land_height` (20 wu);
2. it is not restricted for drops by any exclusion zone ([README](README.md) R-CRATE-032), whether or not the zone blocks Powers;
3. no existing Supply crate lies within `ref.crate.drop.spacing` (350 wu) horizontally.

The first accepted candidate is the drop point. When none of the 100 is accepted, no crate drops this time, and the countdown, already reset, runs on. No candidate is ever the search's start position, because the terrain height at the map origin is below sea level on every reference map (DEC-014).

**R-CRATE-108** (MUST) A drop creates one Supply crate at the drop point, as a structure with no team in the structure pool ([reference/12](../12_simulation_model/04_containers_and_update_order.md)), keeps the drawn start height, starts its falling sound loop and registers its off-screen marker (R-CRATE-144). The first Supply crate of a match also shows the announcement `crate.drop_announce`, once per match.

### 2.4 Other sources of Supply crates

**R-CRATE-109** (MUST) The Supply Storm outcome drops extra crates by the placement rules (R-CRATE-208). The two tutorials each insert one Supply crate with a forced outcome, an airstrike in the first and a Transport in the second, as sequenced simulation events at positions given by tutorial step data ([reference/11](../11_tutorial_and_help.md), DEC-103). No other rule creates a Supply crate.

### 2.5 Falling

**R-CRATE-110** (MUST) Each tick while a Supply crate is not on the ground:

```
if p.y <= terrainHeight(p.x, p.z) + ref.crate.fall.land_clearance:         # 20 wu
    v := 0; onGround := true; stop the falling loop; emit the land sound
    return
t := match time in game seconds + crate serial * ref.crate.fall.sway_phase_step    # 20
sway := ref.crate.fall.sway_amplitude * p.y / ref.crate.drop.start_height         # 300 wu at 1300 wu up
target := (p.x + cos(t) * sway, terrainHeight(p.x + cos(t) * sway, p.z), p.z)    # sway on x only
target := PushFromObstacles(target)
step := normalise(target - p) * ref.crate.fall.speed                             # 40 wu/s
newP := PushFromObstacles(p + step)
newP := p + clampLength(newP - p, ref.crate.fall.speed * dt)                      # at most 4 wu per tick
v := (newP - p) / dt                                                              # read only by presentation
p := newP
```

The crate therefore descends at 40 wu per game second along a swaying, mostly vertical path whose sway shrinks as it descends. The sway reads match time, never a real clock (DEC-009), and its trigonometry is evaluated by the Species maths library (DEC-241).

**R-CRATE-111** (MUST) `PushFromObstacles(q)` examines every structure of the obstacle-grid cell under q whose hit volume meets a sphere of the crate's radius at q, and steps q horizontally away from that structure's centre by `ref.crate.fall.push_step` (2 wu) until the sphere no longer meets it. If the terrain height at the pushed point is at or below `ref.crate.fall.push_min_terrain` (10 wu), the push is discarded and q is returned unchanged, so a crate is never pushed out over water.

**R-CRATE-112** (MUST) From the start heights of 1300 to 1500 wu a crate falls for about 330 to 380 ticks (33 to 38 s): the path length divided by 4 wu per tick, the vertical share of each step being at least 0.97 because the sway never exceeds 300 wu at 1300 wu up. This value is derived by calculation (DEC-111).

### 2.6 Landing and the bot target

**R-CRATE-113** (MUST) On its first tick on the ground, a Supply crate creates a bot navigation node (`BotNavNode`) at its ground position, with priority modifier `ref.crate.bot.priority_modifier` (0.5) and the Supply crate flag set, and the bot navigation links are recalculated; this is how bots notice Supply crates ([reference/07](../07_bots.md) gives the priority formulas). The node is destroyed when the crate opens, with an immediate recalculation, or when it expires, with the recalculation made at the next structure advance.

### 2.7 Opening

**R-CRATE-114** (MUST) A Supply crate on the ground runs each tick, in this order:

1. **Count.** For every used team t, n_t is the number of Workers of team t at a horizontal distance of less than `ref.crate.open.scan_range` (35 wu) from the crate, exact per candidate, that are not dead and not inside a Transport. Only Workers count: Leaders, Troopers, Transports, Collectors, Builders and creatures do not. The Workers of the Rogue and Visitor factions count, so those factions can open a crate and receive its outcome. The **majority team** is the team with the highest count, ties going to the lower team slot, or none when every count is 0 (DEC-016); n is its count. The count is made every tick.
2. **Owner holds the majority** (the crate has an owner and it is the majority team): progress += min(n, `ref.crate.open.max_progress_per_tick`) (50 units), and the crate's life is reset to `ref.crate.lifetime.neutral_ticks` (R-CRATE-119). The crate **opens** on the first tick on which its progress exceeds `ref.crate.open.full_progress` (2500 units) (R-CRATE-121).
3. **Otherwise** (the owner lost the majority, the crate is contested, or it has no owner): progress := max(0, progress − `ref.crate.open.drain_per_tick`) (100 units). When progress is 0 and the owner is not the majority team, the owner becomes the majority team: when the new owner is none, the capture-lost sound plays and the capture-begin loop stops; otherwise the capture-begin loop and the 0 % progress sound start.

A crate with no owner and a majority team therefore becomes owned on the same tick, and a challenger must first drain the previous owner's progress before its own progress starts.

**R-CRATE-115** (MUST) Progress is an integer number of units. A Worker contributes one unit per tick, so the opening rate is n units per tick, capped at 50 units per tick: a crate opens after ⌊2500 ÷ min(n, 50)⌋ + 1 ticks of an unbroken majority. With 50 or more Workers that is 51 ticks (5.1 s) at every game speed, the reference minimum (DEC-021); with 25 Workers 101 ticks; with 10 Workers 251 ticks; with 1 Worker 2501 ticks. A full bar drains in 25 ticks (2.5 s). The cap is a per-second rate scheduled in ticks, so it scales with game speed like every other rule (DEC-092).

**R-CRATE-116** (MUST) The **opening fraction** of a crate is min(1, progress ÷ `ref.crate.open.full_progress`) while it has an owner and 0 otherwise. It fills the crate's marker ring (R-CRATE-144).

**R-CRATE-117** (MUST) While the owner holds the majority, whenever the progress bucket b = ⌊5 × progress ÷ `ref.crate.open.full_progress`⌋ changes to a value from 0 to 4, the previous progress sound stops and the progress sound of bucket b plays (`sfx.crate.supply_crate.capture_progress_00`, `_20`, `_40`, `_60` or `_80`). While progress drains, no progress sound plays.

**R-CRATE-118** (MUST) Idle Workers search for Supply crates on the ground within their crate search range and walk to a scatter point around them ([worker.md](../02_units/worker.md) R-UNIT-078, R-UNIT-094); a Worker leaves that state as soon as the crate no longer exists. Players steer Workers to crates with Leaders and orders ([reference/06](../06_controls_camera_and_commands.md)).

### 2.8 Lifetime and expiry

**R-CRATE-119** (MUST) A Supply crate's life starts at `ref.crate.lifetime.neutral_ticks` (600 ticks, 60 s). It counts down by one tick per tick only while the crate is on the ground and has no owner, and not during the first tutorial or the second tutorial before its bot is released. It is reset to 600 ticks on every tick on which the owner holds the majority (R-CRATE-114 step 2), and it does not count while a crate with an owner drains.

**R-CRATE-120** (MUST) When its life is at or below 0 and its progress is at or below 0, the crate is removed with no outcome: its bot navigation node is destroyed (R-CRATE-113) and the expiry presentation event is emitted, which no sound is bound to (DEC-565). Opening and expiry both show the shatter effect of `mesh.structure.supply_crate` (presentation).

### 2.9 Resolution on opening

**R-CRATE-121** (MUST) On the tick a crate opens, with T its owner:

```
if crate has a forced outcome:  GoodBranch(forced outcome)          # no draw (DEC-652)
else if basic outcomes only:     GoodBranch(DrawGood())
else:
    u := SimRng::NextUnit(1)
    if u < ref.crate.open.good_share: GoodBranch(DrawGood())          # 0.9
    else:                             BadBranch(DrawBad())             # 0.1
remove the crate; stop the progress sounds and the capture-begin loop; emit the captured sound
destroy its bot navigation node and recalculate the bot navigation links
if T is the local team: notify the hint overlay (presentation, reference/11)
```

`GoodBranch(o)` delivers outcome o to T by the delivery of [README](README.md) table 2.2-A. `BadBranch(o)` creates outcome o at the crate's position for the Rogue faction, or for the Visitor faction when o is the Abductor (R-CRATE-012), hostile to every team, T included. When the drawn candidate list is empty, the branch delivers nothing (R-CRATE-003).

**R-CRATE-122** (MUST) The bad branch never yields the airstrike, Missile Barrage, Reinforcements, Essence Grove, Brood, meteor shower, Firebomb Run, Time Dilation or Time Rush outcome: the bad pool is exactly the outcomes marked "Bad" in table 2.2-A.

**R-CRATE-123** (MUST) A team that opens a crate always receives the good outcome itself, whichever team it is; when the Rogue faction's Workers open a crate, the Rogue faction receives the good outcome and its announcement uses the Rogue-faction column of [README](README.md) table 2.6-A.

### 2.10 Outcome selection and validity

**R-CRATE-124** (MUST) `DrawGood()` builds the good candidate list: every outcome of the catalogue, in catalogue order, that is marked "Good" in table 2.2-A (every outcome except the Swarm, the Allegiance Scrambler, the Sickness, the Abductor and the Spitter Plants) and that is valid by R-CRATE-125. It draws `SimRng::NextUint` mod (list length) and returns that entry, so every valid good outcome has the same probability. An empty list returns nothing.

**R-CRATE-125** (MUST) An outcome is **valid** unless one of these holds, tested in this order:

1. basic outcomes only is on and the outcome is advanced;
2. it is the Collector, Builder or Spawn Surge outcome and the map has no Spawn Node of any team or of no team ([reference/05](../05_population_flow.md) R-ECON-027);
3. it is the Transport outcome, and the mode is Launch Race or the map sets a Transport cap of 1 or more; an absent cap, a cap of 0 and an unlimited cap leave it valid (DEC-120);
4. the mode is Siege and the outcome is the rocket turret or the Abductor;
5. it is the Abductor and the Abductor has already appeared in this match;
6. it is the Supply Storm and at least `ref.crate.supply_storm.max_live_crates` (4) Supply crates exist, the opening crate included;
7. it is the Hive and the number of Hives on the map + `ref.crate.hive.pending_weight` (2) × the number of Hive Powers held in the Power slots of every used team is at least `ref.crate.hive.limit` (4);
8. the map excludes it ([README](README.md) R-CRATE-030).

**R-CRATE-126** (MUST) `DrawBad()` returns nothing when basic outcomes only is on. Otherwise it builds the bad candidate list in catalogue order: the Swarm, the Allegiance Scrambler, the Hive, the Sickness, the Blight Grove, the Abductor while it has not appeared in this match and the mode is not Siege, and the Spitter Plants; it removes every outcome that the map excludes (DEC-107); it draws `SimRng::NextUint` mod (list length) and returns that entry. The other validity rules of R-CRATE-125 do not apply to the bad pool: in particular the Hive limit does not. An empty list returns nothing.

**R-CRATE-127** (MUST) The Hive and the Blight Grove belong to both pools. Drawn by the good branch, each is a Power of the opening team; drawn by the bad branch, each is immediate, at the crate, for the Rogue faction.

### 2.11 Probabilities

**R-CRATE-128** (MUST) With G the good candidate list, B the bad candidate list and g = `ref.crate.open.good_share`, the probability of outcome t at one opening is P(t) = g × [t ∈ G] ÷ |G| + (1 − g) × [t ∈ B] ÷ |B| when basic outcomes only is off, and P(t) = [t ∈ G] ÷ |G| when it is on (a forced outcome aside).

**R-CRATE-129** (MUST) These contexts give these probabilities at the start of a match with basic outcomes only off. Which reference maps fall in which row is map data ([reference/08](../08_maps_and_level_design/README.md)).

Table 2.11-A Probability by context.

| Context | |G| | Each good outcome | |B| | Each bad outcome | Notes |
|---|---|---|---|---|---|
| Spawn Control, Zone Control, Relic Run or Flag Chain; Spawn Nodes; no exclusions; no Transport cap | 26 | 0.9/26 = 3.462 % | 7 | 0.1/7 = 1.429 % | the Hive and the Blight Grove are in both pools: 4.890 % each in total, as two deliveries |
| the same, after the Abductor has appeared | 26 | 3.462 % | 6 | 1.667 % | |
| the same, while 4 or more Supply crates exist | 25 | 3.600 % | 7 | 1.429 % | the Supply Storm is invalid; dynamic |
| the same, while the Hive limit is reached | 25 | 3.600 % | 7 | 1.429 % | dynamic; with both limits 24 and 3.750 % |
| a map with no Spawn Node | 23 | 3.913 % | 7 | 1.429 % | without Collector, Builder and Spawn Surge |
| Flag Chain on a map with a Transport cap | 25 | 3.600 % | 7 | 1.429 % | without Transport |
| Launch Race on a map with Spawn Nodes | 25 | 3.600 % | 7 | 1.429 % | without Transport |
| Launch Race on a map with no Spawn Node | 22 | 4.091 % | 7 | 1.429 % | |
| Siege on a map with no Spawn Node, no exclusions and no Transport cap | 22 | 4.091 % | 6 | 1.667 % | without rocket turret, Collector, Builder and Spawn Surge; no Abductor |
| Siege, with the Transport invalid by a cap or excluded | 21 | 4.286 % | 6 | 1.667 % | |
| Siege, excluding the Strike team, airstrike, Missile Barrage and meteor shower | 18 | 5.000 % | 6 | 1.667 % | |
| the same, with a Transport cap | 17 | 5.294 % | 6 | 1.667 % | |
| basic outcomes only on; Spawn Nodes; no exclusions | 7 | 14.286 % | — | 0 % | no bad outcomes |
| basic outcomes only on; no Spawn Node | 6 | 16.667 % | — | 0 % | without Builder |

**R-CRATE-130** (MUST) In the first context of table 2.11-A each outcome has this total probability: the Strike team, Collector, Builder, Transport, airstrike, Missile Barrage, Reinforcements, Shield, Convert, Speed Boost, gun turret, Essence Grove, Brood, meteor shower, Supply Storm, Spawn Surge, Frenzy, Rocket Troops, Fury, rocket turret, flame turret, Firebomb Run, Time Dilation and Time Rush (24 outcomes) 3.462 % each, as good outcomes; the Hive and the Blight Grove 4.890 % each (3.462 % as a Power of the opening team and 1.429 % immediate for the Rogue faction); the Swarm, Allegiance Scrambler, Sickness, Spitter Plants and Abductor 1.429 % each, as bad outcomes. The good outcomes sum to 90 % and the bad to 10 %.

### 2.12 Starting powers

**R-CRATE-131** (MUST) At the initialisation of each used team that is neither the Rogue nor the Visitor faction, in a match whose mode has the starting-powers option ([reference/05](../05_population_flow.md) R-ECON-110), the team receives k starting powers, k being the option value. Each is drawn as follows: the candidate list is every outcome of `ref.power.start.pool`, in catalogue order, that is valid by R-CRATE-125; one `SimRng::NextUint` mod (list length) picks the outcome; and the outcome is granted to the team as a Power. With an empty list nothing is granted (DEC-014).

**R-CRATE-132** (MUST) `ref.power.start.pool` is the 18 outcomes Strike team, Collector, Builder, Transport, airstrike, Missile Barrage, Shield, Convert, Speed Boost, gun turret, Hive, Brood, meteor shower, Blight Grove, Fury, rocket turret, flame turret and Firebomb Run: the catalogue without `ref.power.start.excluded`, which is the Allegiance Scrambler, Supply Storm, Reinforcements, Frenzy, Spawn Surge, Sickness, Rocket Troops, Abductor, Swarm, Spitter Plants, Time Rush, Time Dilation and Essence Grove (DEC-021). Every starting power is therefore a Power. With basic outcomes only on, the valid pool is the Strike team, Builder, Transport, airstrike and gun turret.

**R-CRATE-133** (MUST) A Hive starting power counts toward the Hive limit as a pending Hive Power (R-CRATE-125 step 7): after two Hive starting powers have been granted across all teams, the Hive is invalid for every later draw.

**R-CRATE-134** (MUST) After its starting powers are granted, every Power the team holds receives the start lock of [README](README.md) R-CRATE-018.

**R-CRATE-135** (MUST) On a reference map with Spawn Nodes and no exclusions, each starting power is one of 18 outcomes with probability 1/18 (5.556 %); without Spawn Nodes, one of 16 (6.25 %); with basic outcomes only, one of 5 (20 %).

### 2.13 Comeback Bonus grants

**R-CRATE-136** (MUST) When the Comeback Bonus grants a power to an eliminated team ([reference/05](../05_population_flow.md) R-ECON-114, [reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-052), the outcome is the entry `SimRng::NextUint` mod 6 of `ref.power.comeback.pool`, which lists the airstrike, the airstrike, the meteor shower, the Essence Grove, the Blight Grove and the Brood; with basic outcomes only on, it is `ref.power.comeback.pool_basic`, the airstrike alone. The airstrike therefore has probability 2/6 and each other outcome 1/6.

**R-CRATE-137** (MUST) A Comeback grant is always a Power, the Essence Grove included, unlike the Supply crate's immediate Essence Grove; it applies none of the validity rules of R-CRATE-125 and ignores the map's excluded outcomes (the reference default of DEC-452). It records the Comeback Bonus as its source for the Power slots panel.

**R-CRATE-138** (MUST) A grant needs a free Power slot; an eliminated team has two (README R-CRATE-014). When both are occupied, no power is drawn or granted and the local team's player sees `ui.power_slots.comeback_full` ([reference/01](../01_skirmish_rules_and_modes/README.md) R-MODE-053). The Comeback Bonus is the only grant path of an Essence Grove Power; bots use it by the handlers of [reference/07](../07_bots.md).

