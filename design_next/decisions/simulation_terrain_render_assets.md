# Decision register: simulation, terrain, rendering and assets

> Status: Reference decisions · Range: DEC-650 to DEC-749 · Related: [decisions index](README.md), [verdicts and fixes](verdicts_and_fixes.md), [conventions](../CONVENTIONS.md)

This file records the decisions on the simulation model, the random-draw discipline, numeric robustness, terrain, rendering and the asset pipeline that no other register file holds. Most decisions of this scope exist already, and this file cites them instead of repeating them: the simulation-integrity rules, the scope verdict for the simulation model and the corrections to the actor grid, the obstacle grid, route previews, the texture cache and content validation are in `decisions/verdicts_and_fixes.md`; the numeric model, the generator algorithm, threading, simulation pacing, the simulation and presentation boundary, state ownership and the terrain runtime model are design-phase topics under `decisions/deferred/`; the open questions on the terrain generator and on the renderer are in `decisions/open_questions.md`; engine-technology choices are ADRs under `adr/`. The entries below add the remaining rules: actor-grid registration, the lifetime of simulation routes, random draws at object initialisation, non-finite values in simulation state, and a simulation that runs only on the CPU. Each entry states the behaviour that the reference documents specify now. A **Resolved** entry states the corrected rule that the documents apply; an **Open** entry states the faithful reference behaviour as the default, with a recommendation.

## Decisions of this scope recorded elsewhere

| Topic | Decision |
|---|---|
| Presentation never writes simulation state; rules emit presentation events | DEC-007 |
| Player intent reaches the simulation only as commands, including the Strike team look-around state | DEC-008 |
| Simulation time comes only from the tick counter | DEC-009 |
| Process-global settings, such as a difficulty level, never feed the simulation | DEC-010 |
| Per-match state is owned and reset, including the Control Zone counter, the Spawn Node count of the sixth team slot and the tick-clock speed state | DEC-011 |
| Tree hit volumes are asset data, and no load-time simulation data draws from a presentation generator | DEC-012 |
| Random-stream discipline: per-match seed, one draw per statement, a separate presentation generator | DEC-013 |
| Bounded searches with a deterministic fallback | DEC-014 |
| Build-time validation of content, including collision and hit-test geometry | DEC-015 |
| Ties and container iteration in stable-id order; the order within an actor-grid cell | DEC-016, DEC-260 |
| Object model, generational handles and per-call scratch storage | DEC-028 |
| Multi-round Siege and its round reset inside a tick | DEC-066 |
| Wide Power ids | DEC-083 |
| Bot corrections, including a navigation graph that accounts for Barrier Emitters and no NaN priorities | DEC-087 |
| Structure corrections, including Barrier Emitter collision geometry | DEC-088 |
| Exact or approximate actor-grid queries | DEC-094 |
| Incremental obstacle grid, a cleared outside cell, separate route previews | DEC-095 |
| Texture cache keyed by asset and variant | DEC-098 |
| Tutorial effects as sequenced simulation events | DEC-103 |
| Performance envelope at the population cap | DEC-106 |
| Networking model, command validation, content identity, divergence detection and transport | DEC-200 to DEC-208 |
| Simulation and presentation boundary, state ownership, threading and pacing | DEC-222 to DEC-225 |
| Numeric model, numeric semantics and the generator algorithm | DEC-240 to DEC-242 |
| Lobby state handlers without user-interface dependencies | DEC-243 |
| Terrain runtime model, terrain content identity and terrain scale | DEC-246, DEC-247 |
| Simulation gap rules: actor-grid order, the multi-round Siege rebuild, handle generations, route and actor-group edge cases, what runs at game speed 0, the speed-crate hold and decorative objects | DEC-260 to DEC-266 |
| Engine gap rules of terrain and rendering: full pipeline state per draw, renderer looks and values, terrain query edge cases, reproducible terrain colours and content-build validation | DEC-957 to DEC-959, DEC-966 to DEC-968 |

## Index

| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-650](#dec-650-actor-grid-registration-follows-the-recorded-footprint) | Actor-grid registration follows the recorded footprint | Resolved | R-SIM, DBG |
| [DEC-651](#dec-651-lifetime-of-simulation-routes) | Lifetime of simulation routes | Open | R-SIM, R-UNIT, RND |
| [DEC-652](#dec-652-random-draws-at-initialisation-happen-once-in-explicit-simulation-steps) | Random draws at initialisation happen once, in explicit simulation steps | Resolved | R-SIM, R-BLDG, R-CRATE, R-BOT |
| [DEC-653](#dec-653-simulation-state-never-holds-a-non-finite-value) | Simulation state never holds a non-finite value | Resolved | R-SIM, R-UNIT, R-CRATE, R-BLDG, TER, DBG |
| [DEC-654](#dec-654-gameplay-is-computed-only-by-the-cpu-simulation) | Gameplay is computed only by the CPU simulation | Resolved | R-SIM, VFX, RND |

## 1 Simulation model

### DEC-650 Actor-grid registration follows the recorded footprint
- Status: Resolved
- Affects: reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), reference/12_simulation_model/04_containers_and_update_order.md (R-SIM), architecture/components/diagnostics_test_and_performance.md (DBG); DEC-016, DEC-260
- Specified behaviour: An actor registers in its team's actor grid over the cells that its position and radius cover, and the registration records that cell set. Removing or moving the actor uses the recorded cell set, never a set recomputed from the actor's current radius at its old position. A change of radius is a re-registration: the actor leaves its recorded cells and registers over the cells of its new radius at once. An actor is re-registered whenever its cell set changes. Grid query results follow the stable order of DEC-016, in which the entries of one cell are ordered by actor serial, never by slot (DEC-260), so the schedule on which an implementation refreshes registrations has no observable effect. A test-build check after each tick confirms that every grid entry names a live actor whose recorded cell set contains that cell. Whether grid queries are exact or approximate is DEC-094.
- Options: A — registration by the recorded cell set, as stated; B — the reference behaviour, in which removal recomputes the cell set from the actor's current radius at its old position, so a radius change between two updates leaves stale entries in some cells and removes entries that belong to other cells, and every actor that carries a hit shape is removed and re-inserted on every tick.
- Recommendation: A. A grid that a radius change can corrupt returns query results that no rule specifies and that depend on when the change happened; with a recorded footprint the grid is a pure function of the registrations. The per-tick re-insertion of shaped actors costs time in proportion to their number times their cells, and A removes that cost without changing a result.

### DEC-651 Lifetime of simulation routes
- Status: Open
- Affects: reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), reference/02_units/transport.md (R-UNIT), architecture/components/renderer/entity_and_overlay_renderers.md (RND)
- Specified behaviour: A route that simulation code generates stays in route storage until the match world is destroyed; only a Transport frees its sea routes. Route storage therefore grows for the whole match. Nothing reads a route that no actor or order holds, and the renderer fades drawn routes out on its own timer, so the growth costs memory only. Route previews that the UI draws live in a separate store (DEC-095), and route ids are generational handles that are never reused within a match (DEC-028).
- Options: A — the reference behaviour: keep every simulation route until the match world is destroyed; B — free a route when the last actor or order that holds it releases it, and give the renderer a copy of a route's points for its fade (for example in the presentation event that announces the route), so freeing never cuts a fade short.
- Recommendation: B. A skirmish match ends and frees its routes, but a persistent world never ends (vision O1, O4), so storage that only grows is unbounded there. Freeing a route that nothing holds changes no simulation result, because no rule reads it; the fade then depends only on presentation data, which the simulation and presentation boundary needs anyway (DEC-222).
- Decide by: design phase

## 2 Random draws and numeric robustness

### DEC-652 Random draws at initialisation happen once, in explicit simulation steps
- Status: Resolved
- Affects: reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM), reference/12_simulation_model/02_match_lifecycle_and_state_reset.md (R-SIM), reference/12_simulation_model/03_entity_model_and_identity.md (R-SIM), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/07_bots/README.md and reference/07_bots/05_mode_strategies_and_tutorials.md (R-BOT)
- Specified behaviour: No simulation object draws from the simulation generator while it is being constructed. Every draw that initialises a new object happens once, in the explicit spawn or initialisation step of the simulation that creates the object. This covers: the evaluation timer and the spawn timer of a Spawn Node; the timer of a Population Limiter; the initial values of a bot controller; the random position search of a turret at initialisation; the initial values of Workers, Builders, snowflakes, Wisps, Spitter Plants, Seed Pods, Crawlers and bot objective markers; and the rate that an actor draws when its kind is set, which a Worker draws once per spawn. Reading a map definition creates no simulation object and draws nothing: each structure that the map places is created once, at match-world initialisation. The actor-group offset table, 100 unit-fraction draws, is generated in the last step of match initialisation (step 11 of table 2.4-A of `reference/12_simulation_model/02_match_lifecycle_and_state_reset.md`), after DEC-011 has cleared it, never on its first use. A draw whose occurrence depends on a condition is its own statement inside that condition, and the owning rule states the condition: for example, the Supply crate capture roll against 0.9 and the draw of a beneficial outcome are made only when no outcome is forced (R-CRATE-121). These rules extend the draw discipline of DEC-013; the draw order of the reference generator is not reproduced, and every distribution is unchanged. When a newly created object first advances is DEC-455.
- Options: A — initialisation draws once, in explicit simulation steps, as stated; B — the reference behaviour, in which constructors draw, the map loader constructs a template object for each map-placed structure and then the real object (so each map-placed bot objective marker draws twice at load), a Worker draws its rate twice per spawn, and the actor-group offset table is filled on the first formation query.
- Recommendation: A. Hidden draws tie the generator's position to construction details: an editor, a preview or a debug window that constructed one of these objects would shift every later draw, and the reference is safe from this only because none of them does. With explicit steps, every draw belongs to a known tick, system and object, which per-system sub-streams (DEC-013, DEC-242) and replays need.

### DEC-653 Simulation state never holds a non-finite value
- Status: Resolved
- Affects: reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM), reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM), reference/12_simulation_model/03_entity_model_and_identity.md (R-SIM), reference/02_units/worker.md (R-UNIT), reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md (R-CRATE), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), architecture/components/terrain.md (TER-016), architecture/components/diagnostics_test_and_performance.md (DBG); DEC-465
- Specified behaviour: No simulation step stores NaN or an infinity. Each site where the reference can produce one has an explicit guard that keeps the reference result wherever that result is defined:
  1. A team's selected-Worker centre exists only while at least one Worker is selected. With none selected, the team has no centre, and the UI elements that read it (the route preview, the selection arrows and the Power icon position) show nothing.
  2. A crowd order computes no route and no route start when no Worker is under control. Such an order then does nothing (DEC-555), under the handler rule of DEC-202 item 7.
  3. An actor-group formation offset uses a row length of at least 1.
  4. A walkability test whose start and end give zero steps returns walkable without dividing, which is the reference result.
  5. The Worker fire and combat rules and the thrown-weapon rule that divide by the game speed are never evaluated at a game speed of 0, because no tick runs while the match is paused (DEC-038, DEC-039); each of these divisions carries an explicit guard. Whether the Worker rules keep these divisions is DEC-852.
  6. A bot priority with no usable neighbours yields no NaN: its neighbour mean is 0 (DEC-087, DEC-862).
  7. A score-weighted Supply crate drop whose total score is 0 counts every score as equal: every weighted team has weight 1, and the share p that its first draw tests is max(1 ÷ n, 1 − 1 ÷ n) for n weighted teams (DEC-465, R-CRATE-106); no step divides by the zero total. The drop position is DEC-114.
  8. The turret aim cosine is clamped to the range −1 to 1 before its arccosine, so rounding never yields NaN; in the reference such a NaN makes every comparison with the aim angle false for that tick.
  9. Degenerate vector and geometric queries follow DEC-241 items 6 and 10.
  10. An actor group whose members report no position in update slice 0 does not evaluate its centre as the position sum divided by the count: its centre and velocity keep their previous values (R-SIM-427).
  11. A terrain ray that enters the grid from outside with a direction whose x component is 0 computes its entry parameter from the z components, so the entry height is defined (TER-016).
  In test builds, a check after every tick asserts that no simulation field holds NaN or an infinity, and the finiteness assertion rejects NaN as well as infinities.
- Options: A — a guard at every site and the per-tick check, as stated; B — the reference behaviour, in which NaN is stored every tick in the selection centre and can enter a route start, a formation offset, an actor-group centre, a Supply crate weight, a bot priority, a turret aim angle and the entry height of a terrain ray, and a NaN comparison silently decides an outcome.
- Recommendation: A. NaN is harmless in the reference only because the fields that hold it are read by the UI or because every comparison happens to fail the same way on every process. NaN propagation and its payload and sign differ between instruction sets, a NaN comparison decides an outcome without any rule saying so, and a value that is not finite cannot be checked, hashed or persisted with confidence.

### DEC-654 Gameplay is computed only by the CPU simulation
- Status: Resolved
- Affects: reference/12_simulation_model/README.md (R-SIM), reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM), architecture/components/animation_and_vfx/README.md (VFX), architecture/components/renderer/README.md (RND)
- Specified behaviour: Every value that feeds simulation state is computed by the simulation step on the CPU. GPU work serves presentation only: rendering, particles, shatter debris and animation. No physics library or other third-party component computes gameplay, and no third-party code runs inside a tick (DEC-240). The reference simulation is CPU-only, and the GPU only renders.
- Options: A — CPU-only gameplay, as stated; B — allow GPU compute or a third-party physics library for gameplay where it is shown to be deterministic.
- Recommendation: A. GPU results differ between vendors and driver versions, a headless dedicated server may have no GPU at all (DEC-220), and an outside physics library brings its own numeric model and threads into the tick. Keeping gameplay on the CPU keeps one numeric model (DEC-240) and lets the integrated and the dedicated server compute identical results.
