# Randomness and integrity

> Status: Reference · Prefix: R-SIM · Related: [Simulation model](README.md) · [Tick timing and main loop](01_tick_timing_and_main_loop.md) · [Match lifecycle and state reset](02_match_lifecycle_and_state_reset.md) · [Entity model and identity](03_entity_model_and_identity.md) · [Containers and update order](04_containers_and_update_order.md) · [Teams and player commands](06_teams_and_player_commands.md)

## 1 Purpose and player experience

The reference simulation is a deterministic function of three inputs: the match setup (map, team layout and match options), the seed of the simulation random stream, and the ordered stream of command frames. Every participant runs the whole simulation and obtains the same match. This document specifies what keeps it deterministic:

- the random streams: the simulation generator, the presentation generators and the private terrain generators, with their seeding and lifetime;
- the draw discipline, and what every object family draws and what each draw decides;
- the classification of every subsystem's state as simulation state or presentation state;
- the **simulation-integrity rules**: presentation never writes simulation state, local-only state never reaches the simulation, no presentation generator feeds the simulation, and no simulation state survives between matches. Each place where the reference breaks one of these rules is listed with its fixed rule;
- the integrity duties of the bots, and further per-subsystem rules;
- the hooks for recording and replaying the command stream.

**Player experience.** Every participant sees the same battle: the same Worker wins the same fight, the same Supply crate outcome appears, and the same tree catches fire. Cosmetic variety (particle sprays, sound choice, the wobble of snowflakes) may differ between participants without consequence. A participant's camera, preferences, frame rate, local clock and window focus never change what happens in the match.

**Scope.** The integrity rules are fixed. The generator algorithm and stream derivation (DEC-242), the numeric model (DEC-240), the threading model (DEC-224), the mechanism that enforces the simulation–presentation boundary (DEC-222), the ownership of world, region and match state (DEC-223), the networking model (DEC-200), divergence detection (DEC-206) and the replay feature (DEC-046) are decided by the design phase. Tick timing is [01](01_tick_timing_and_main_loop.md); the per-match reset points are [02](02_match_lifecycle_and_state_reset.md).

## 2 Rules

### 2.1 Scope of the integrity rules

**R-SIM-600** (MUST) **Simulation** is everything that a tick executes (command application, the tick-clock update, the world step and the match-rules update, [01](01_tick_timing_and_main_loop.md)) and the match-world initialisation that runs when a map loads. **Presentation** is everything else: rendering, audio, UI, camera, input handling, particles and shatter effects. Presentation may differ freely between participants and never feeds the simulation.

**R-SIM-601** (MUST) Given the same match setup, the same seed and the same command frames, every value that feeds simulation state is identical for every participant and for every run: positions and velocities, timers, stat-derived values, terrain heights and normals, random draws and every comparison derived from them. Presentation results never need to agree. Which builds and data paths must agree bit for bit, and the numeric model that guarantees it, are decided by DEC-240.

### 2.2 Random streams

**R-SIM-605** (MUST) The **simulation generator** (`SimRng`) is one sequential stream per match, owned by the match. It offers three draw forms:

| Draw | Result | Reference definition |
|---|---|---|
| `SimRng::NextUint` | an unsigned 32-bit integer, uniform over 0 to 2³² − 1 | one draw of the stream |
| `SimRng::NextUnit(r)` | uniform over [0, *r*) for *r* > 0 | one integer draw × *r* / 2³² |
| `SimRng::NextSigned(r)` | uniform over [−*r*/2, *r*/2): half of *r* on either side of 0 | (`NextUnit(1)` − 0.5) × *r* |

A 3-vector built from draws takes its x draw first, then y, then z.

**R-SIM-606** (MUST) The generator algorithm, its seeding function and the stream structure are not part of the reference library; the default is one sequential stream, owned by the match world and seeded once per match (DEC-242). The draw order of the reference generator is not reproduced, and every distribution is (DEC-013).

**R-SIM-607** (MUST) The **presentation generators** (`FxRng`) serve cosmetic effects, audio variation and every other presentation randomness. Their draw forms are `FxRng::NextUint` (a non-negative integer), `FxRng::NextUnit(r)` (uniform over [0, *r*], *r* included) and `FxRng::NextSigned(r)` (uniform over [−*r*/2, *r*/2], both ends included), and `FxRng::Seed` sets a state. In the reference:

- one process-wide presentation generator is re-seeded with the value 1 at every match-world initialisation, after the map definition is read and before the terrain is built; each tree geometry pass re-seeds it from the tree's seed, and the terrain colour pass re-seeds it at every vertex from the vertex coordinates and its own previous output; nothing else re-seeds it;
- that generator is shared by audio, terrain colours, tree geometry and an online-service thread, and it is not safe for concurrent use;
- a second, per-thread runtime generator, never seeded in a match, serves tree leaf particles, Sickness particles, the Allegiance Scrambler's particles and the hint overlay's choice of tab.

**R-SIM-608** (MUST) Every presentation consumer draws from its own presentation generator instance, and no generator instance is shared between threads: one presentation generator per thread at least, with the audio engine owning its own instance (ADR-0022). No presentation generator is reachable from simulation code (DEC-013, DEC-224, DEC-242).

**R-SIM-609** (MUST) Each terrain tile owns a private generator, re-seeded from the tile's seed in the map definition at the start of every generation of that tile. Its output, the height field, is simulation static data, independent of call history and of every other consumer. The terrain algorithm and its generator are specified by the [terrain component](../../architecture/components/terrain.md); the terrain runtime model is DEC-246.

**R-SIM-610** (MUST) The simulation generator is seeded once per match, at match start, and the seed is part of the recorded match setup. No simulation draw happens before match start (DEC-013). In the reference, the host derives a seed from host-local entropy for each session, sends it to each participant when it joins, and records it nowhere; every match runs in a fresh session, so no generator state carries from one match into the next.

**R-SIM-611** (MUST) No simulation draw happens in the lobby, and no UI, rendering, audio or input code draws from the simulation generator. The reference has no such draw: the lobby advances no match world, and none of the simulation generator's call sites is in UI, render, audio or input code.

**R-SIM-612** (MUST) The draws of match-world initialisation happen at one logical point for every participant: a participant that processes the command frame that starts a match takes no further frame until the initialisation has finished ([01](01_tick_timing_and_main_loop.md) R-SIM-533). Initialisation draws, in this order: the colours of unused team slots (a retry loop, bounded by DEC-014), the start positions of all six team slots in team-index order, the bot colours, the co-op pairing, the Siege sides, the initialisation of structures and bot controllers, and the starting Powers. Every participant loads with identical lobby-slot state (colour ids and slot kinds), and a loader that runs on another thread, or concurrently with other work, keeps this draw order (DEC-224).

**R-SIM-613** (MUST) Instrumentation never changes a draw. A build that records each draw with its call site and a running count produces exactly the values of a build without it; in the reference the draw-tracking build computes the signed draw by a different formula and cannot play with an untracked build. The draw log keeps the most recent draws with their call sites, which is the tool for finding the first draw at which two runs differ (DEC-206).

### 2.3 Draw discipline

**R-SIM-620** (MUST) Every gameplay decision draws from the simulation generator: Supply crate drops, captures, outcomes and effects, bot choices, spawn timers, damage rolls, conversions and scatter.

**R-SIM-621** (MUST) Every expression that combines several draws has a defined draw order. Each draw is its own statement; no draw sits inside a constructor, an argument list or the condition of a variable-length loop (DEC-013, DEC-242). The reference fixes the order in three ways, whose distributions are kept: a vector draw takes x, then y, then z; the tree generation of a Supply crate outcome makes its `ref.sim.rng.crate_tree_draws` (8) integer draws one statement at a time; and the spawners draw the height on its own statement before the x and z coordinates.

**R-SIM-622** (MUST) The number of simulation draws never depends on local or presentation state. Every cosmetic draw is a presentation draw (DEC-013); the reference makes the draws below from the simulation stream, and each one moves to a presentation generator (DEC-222):

| Cosmetic draw | Reference draws |
|---|---|
| ant leg shape | one integer draw, modulo 3, per ant per tick |
| snowflake and Wisp wobble | three `NextSigned(1)` and one `NextUnit(20)` per object per tick |
| meteor fire size | five `NextSigned(60)` per tick |
| Spitter Plant burning-particle size | one `NextSigned(100 × size)` per burning tick |
| turret shell casings | three `NextSigned(0.5)` and one `NextUnit(20)` per shot (`ref.sim.rng.turret_casing_draws_per_barrel`, 4) |
| turret overheat smoke | one unit draw per particle |
| Trooper shell casings | per shot |
| Worker abduction spark | one `NextUnit(1)` compared with 0.5 per abducted tick |
| volcano vent | one integer draw per idle tick and one `NextUnit(4)` |
| Barrier Emitter sparks | one `NextUnit(4)` per spark |
| Essence orb break particles | fifteen `NextUnit(5)` once |
| thrown-weapon trail, airstrike target marker smoke, rocket smoke, Transport exhaust | per particle |

**R-SIM-623** (MUST) Every rejection loop and search in the simulation has a capped number of attempts and a deterministic fallback (DEC-014). In the reference these loops are deterministic for identical inputs but consume a data-dependent number of draws, and several are unbounded: the Supply crate capture when no valid outcome exists; the starting-Powers choice; the Missile Barrage sea search; the Abductor land search; the bot target-area loops; the Brood spawn-type choice; the target-structure retries; the valid-point loops of the Strike team, Transport, Collector and Builder outcomes; the turret spacing of the Flag Chain turret placement; the Launch Race drop-off point search; the colour loop for unused slots at match-world initialisation; the bot colour choice, unbounded when every bot colour is taken; and the land searches of lightning strikes and dust vortices.

**R-SIM-624** (MUST) A draw whose occurrence depends on a condition is its own statement inside that condition, and the owning rule states the condition (DEC-652). For example, the Supply crate capture roll is drawn only when no outcome is forced, so a forced outcome shortens the stream.

**R-SIM-625** (MUST) No presentation generator feeds simulation state. In the reference the one exception is the tree crown hit volume (R-SIM-650); every collision and hit volume that the simulation uses is map or asset data (DEC-012).

**R-SIM-626** (MUST) No simulation object draws from the simulation generator while it is being constructed. Every draw that initialises a new object happens once, in the explicit spawn or initialisation step that creates it (DEC-652). This covers: the evaluation timer and the spawn timer of a Spawn Node (2 draws); the timer of a Population Limiter (1); the initial values of a bot controller (4); the random position search of a turret at initialisation (3); the initial values of Workers (2), Builders, snowflakes (4), Wisps (4), Spitter Plants, Seed Pods, Crawlers and bot objective markers; and the rate that an actor draws when its kind is set, which a Worker draws once per spawn. Reading a map definition creates no simulation object and draws nothing; each structure that the map places is created once, at match-world initialisation. In the reference, a constructor draws, a map-placed structure is constructed twice (a template, then the real object), so each map-placed bot objective marker draws twice at load, and a Worker draws its rate twice per spawn.

**R-SIM-627** (MUST) The actor-group formation offset table holds `ref.sim.rng.formation_offset_draws` (100) values from `SimRng::NextUnit(1)`. It is cleared at every match start and end (DEC-011) and generated at a fixed step of match initialisation, never on its first use (DEC-652); the reference generates it on the first formation query after a map load.

**R-SIM-628** (MUST) A spawn-position search makes `ref.sim.rng.spawn_search_draws_per_try` (2) draws per try, `NextUnit(spread)` for the distance and `NextUnit(2π)` for the angle, for at most `ref.sim.rng.spawn_search_tries` (10) tries; a search that finds no valid position clamps the requested position to at least `ref.sim.rng.spawn_edge_clamp` (20 wu) inside every map edge. A bot team's bot controller is requested at `ref.sim.rng.bot_controller_spawn_point` (−100, 0, −100) wu with a spread of 0, so its search always fails after 10 tries and 20 draws, and the controller is placed at (20, 0, 20) wu.

### 2.4 What each object family draws

**R-SIM-630** (MUST) The simulation draws of the reference are those of Table 1, made at the stated points; the draws marked "presentation" move to a presentation generator by R-SIM-622. The route graph, the actor grid, the obstacle grid, the base simulated object and the procedural legs make no draw. The ranges in Table 1 repeat, for readability, values that the owning documents define in their tuning tables; *r* stands for a radius or range given there.

Table 1 — Draws per object family (simulation draws unless marked presentation).

| Object family and step | Draws per occurrence | When | Owner |
|---|---|---|---|
| any actor: kind assignment | `NextUnit(rate of the actor kind)` | every spawn; once per spawn for a Worker (R-SIM-626) | [reference/02](../02_units/README.md) |
| any actor: attack | two `NextSigned(15)` | each shot | reference/02 |
| any actor: push out of obstacles | `NextSigned(1)` | when its height is 1 wu or less | reference/02 |
| spawn-position search | per try `NextUnit(spread)` and `NextUnit(2π)`, at most 10 tries (R-SIM-628); a Worker's initial facing: a vector of two `NextSigned(1)` | spawn | reference/02 |
| Worker: initial values | `NextUnit(5)`, `NextUnit(60)` | spawn step | reference/02 |
| Worker: retarget | `NextUnit(1)` | every retarget, every 1 to 2 s | reference/02 |
| Worker: approaching a carryable structure | `NextUnit(2π)` | when its waypoint is far | reference/02 |
| Worker: riding a Transport | a vector of two `NextSigned(1)` and `NextUnit(20)` when the Transport is destroyed; `NextSigned(π/2)` and `NextUnit(10)` when unloaded | those events | reference/02 |
| Worker: combat | `NextSigned(π/2)` for movement every combat tick; `NextUnit(20)` on approach; `NextUint` for the fire roll outside a formation; two `NextSigned(60)` under Fury; `NextUnit(8)` for the grenade timer; two `NextUnit(4)` for the rocket jitter of a Worker driven insane by Sickness | every combat tick | reference/02 |
| Worker: under crowd control | `NextUnit(0.5)`, `NextUnit(number of Workers under control)`, `NextUnit(2π)` | every 0.5 to 1 s | reference/02 |
| Worker: following a waypoint order | `NextUnit(10)`, `NextUnit(2π)` | the jitter at a teleport structure | reference/02 |
| Worker: following a Leader | `NextUnit(r)`, `NextUnit(2π)` | the 5 s retarget and on arrival | reference/02 |
| Worker: attacking a structure | `NextUnit(50)` | every tick | reference/02 |
| Worker: idle position search | `NextUnit(1)`, then `NextSigned(2π)` in 70 % of searches | idle | reference/02 |
| Worker: Supply crate search | `NextUnit(2 × 35/3)`, `NextUnit(2π)`, `NextUnit(35/3)` | when it has found a Supply crate | reference/02 |
| Worker: zone search | a vector of two `NextSigned(20)` | when it has found a zone | reference/02 |
| Worker: search for a Transport, a Leader, a structure port (twice) or a carryable structure | `NextUint` | when candidates exist; the first draw of the port search is then overridden by the nearest-structure loop | reference/02 |
| Worker: giving orders, following a route, searching for a Leader | `NextUnit(r)`, `NextUnit(2π)` | — | reference/02 |
| Worker: threat search | `NextUint` (the Hive check, 1 in 500) | every search that passes the tree check | reference/02 |
| Worker: victory dance | `NextUnit(5)` (team slot 0 only), `NextUnit(15)` | each idle retarget | reference/02 |
| Worker: moving to a target while sick | `NextUint` | every tick while it has Sickness | reference/02 |
| Worker: on fire | a vector of two `NextSigned(100)`, `NextUnit(10)` | every burning tick | reference/02 |
| Worker: being abducted | `NextUnit(1)` spark | every abducted tick — presentation | reference/02 |
| Worker: Sickness spread | `NextUnit(5)`, `NextUint` per neighbour, `NextUint`, `NextUnit(10)` | every 5 to 10 s | reference/02 |
| Worker: infected by Sickness | `NextUnit(10)` | the infection event | reference/02 |
| Worker: Convert shot | two `NextSigned(15)` | each shot | reference/02 |
| Leader: bot re-order | `NextUnit(20)`, `NextUnit(2π)` | each bot re-order | [reference/07](../07_bots.md) |
| Essence orb: spawn | `NextUnit(10)`, three `NextUnit(2)` | spawn | reference/02 |
| Essence orb: step | three `NextSigned(1)` every tick; `NextUnit(0.5)` while being born; `NextUnit(10)` at the high point of its birth; `NextSigned(60)` from birth to floating; `NextUnit(2)` at death | every orb, every tick | reference/02 |
| Essence orb: break | fifteen `NextUnit(5)` | once — presentation | reference/02 |
| actor-group formation offsets | 100 `NextUnit(1)` | match initialisation (R-SIM-627) | [03](03_entity_model_and_identity.md) |
| ant: leg shape | `NextUint` modulo 3 | every tick per ant — presentation | reference/02 |
| ant: moving to a target | `NextSigned(0.2)` heading jitter | every tick | reference/02 |
| meteor: fire size | five `NextSigned(60)` | every tick — presentation; its slow-motion request is simulation ([01](01_tick_timing_and_main_loop.md)) | [reference/04](../04_supply_crates_powers_and_weapons/README.md) |
| Spitter Plant: burning particles | `NextSigned(100 × size)` | while burning — presentation | [reference/03](../03_buildings_and_world_objects/README.md) |
| snowflake, Wisp: wobble | three `NextSigned(1)` and `NextUnit(20)` | every object, every tick — presentation | reference/02, [reference/08](../08_maps_and_level_design/README.md) |
| map-effect spawners (snow, dust storm, lightning) | unit draws and vector draws for spawn positions; the dust vortex and lightning strikes are gameplay, snow is decoration | slice 3 ([01](01_tick_timing_and_main_loop.md)) | reference/08 |
| lightning strike | `NextUnit(1)` compared with 0.01: the strike roll, 1 % per tick, which sets fire | every tick | reference/02 |
| Crawler: nearby egg | ⌊`NextUnit(n)`⌋ | each search | reference/02 |
| tree: Essence orb drop | `NextUint` test, then `NextUnit(h)` for x, `NextUnit(2π)` for the yaw, then `NextUnit(6)` repeated until it names a valid team; the drop timer `NextUnit(5)` | each drop | reference/03 |
| tree: offspring | `NextUnit(H)`, then `NextSigned(π/4)` or `NextUnit(2π)`, then `NextUnit(99999)` for the seed, then `NextSigned(0.2)` for the height | each offspring | reference/03 |
| Blight Grove tree | `NextUnit(10)` every tick; its Wisp `NextSigned(1)` and `NextUnit(25)`; corruption `NextUnit(5)` | — | reference/04 |
| Barrier Emitter | `NextUnit(10)` at initialisation; `NextUnit(4)` per spark (presentation) | — | reference/03 |
| volcano vent | `NextUint` every idle tick and `NextUnit(4)` | — presentation | reference/08 |
| Allegiance Scrambler effect | `NextUnit(2.5)`, `NextUint` per converted Worker | each conversion | reference/04 |
| turret: primary fire | for every examined manned barrel of every turret type, `ref.sim.rng.turret_sim_draws_per_barrel` (3) draws: two `NextSigned(target radius)` and one `NextSigned(0.1)` (the mortar target and force, drawn also for the other types); then the shell ejection (presentation, R-SIM-622); an unmanned barrel draws nothing; overheat smoke per particle (presentation); initialisation 3 draws | each shot: up to two barrels per trigger, one for a rocket turret | reference/03 |
| Solar Array | `NextUnit(20)` every tick, also while unmanned | every tick | reference/03 |
| Beam Dish | the beam-loss kick | when a beam is lost | reference/03 |
| Spawn Node, Population Limiter | 2 and 1 draws | creation (R-SIM-626) | reference/03 |
| weapons and special units | laser damage 20 + `NextUnit(20)` and its push; the shockwave ignition chance; the airstrike bomb count 3 + `NextUint` modulo 5; the airstrike target-marker scatter, a vector of `NextSigned(100)`; the barrage-missile scatter `NextSigned(300)`; the Strike team's target choice, aim and grenade chance; the Transport's bot choices and conversion jitter; the Collector's wander and Essence orb snap offsets; the Builder's idle rotation rate | — | reference/02, reference/04 |
| Supply crates | spawning, falling, capture, outcome choice and every outcome effect | — | reference/04 |
| bot controllers and bot objectives | the bot navigation-node priorities and every bot decision | — | reference/07 |
| match rules | `NextUint` for the Relic spawn choice, the Comeback Bonus reward, the bot colour and the Relic mesh (one of 8); `NextUnit` for the Relic timer and the Transit Gate spawn offsets and velocities; a vector draw for the Relic facing | slice 8 ([01](01_tick_timing_and_main_loop.md)) | [reference/01](../01_skirmish_rules_and_modes/README.md) |

