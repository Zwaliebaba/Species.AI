# Gap rules: simulation

> Status: Reference decisions · Range: DEC-260 to DEC-299 · Related: [decisions index](README.md), [simulation, terrain, rendering and assets](simulation_terrain_render_assets.md), [verdicts and fixes](verdicts_and_fixes.md), [conventions](../CONVENTIONS.md)

This file records the simulation-model rules of reference/12 that the documents had to choose because the reference does not define them, the reconciliations between simulation decisions, and the simulation behaviours that need a decision of their own. Related gap rules of one document or mechanic are grouped into one entry with numbered items. Each entry states the rule the documents specify now. An **Open** entry states that rule as the default, with a recommendation; a **Resolved** entry states a rule that is settled.

## Index

| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-260](#dec-260-order-within-an-actor-grid-cell-and-container-iteration-order) | Order within an actor-grid cell and container iteration order | Resolved | R-SIM, R-BLDG |
| [DEC-261](#dec-261-multi-round-siege-rebuilds-the-match-world-at-a-tick-boundary) | Multi-round Siege rebuilds the match world at a tick boundary | Resolved | R-SIM, R-MODE |
| [DEC-262](#dec-262-generations-of-laser-bolt-and-essence-orb-handles-and-the-team-selection) | Generations of laser-bolt and Essence-orb handles, and the team selection | Resolved | R-SIM |
| [DEC-263](#dec-263-route-and-actor-group-edge-cases) | Route and actor-group edge cases | Open | R-SIM |
| [DEC-264](#dec-264-what-runs-while-game-speed-is-0) | What runs while game speed is 0 | Open | R-SIM, R-MODE |
| [DEC-265](#dec-265-real-duration-of-the-speed-crate-holds) | Real duration of the speed-crate holds | Open | R-SIM, R-CRATE |
| [DEC-266](#dec-266-decorative-objects-in-the-simulated-pools) | Decorative objects in the simulated pools | Open | R-SIM, VFX |
| [DEC-267](#dec-267-barrier-emitter-line-in-the-obstacle-grid) | Barrier Emitter line in the obstacle grid | Open | R-SIM, R-BLDG |

## 1 Containers, grids and identity

### DEC-260 Order within an actor-grid cell and container iteration order
- Status: Resolved
- Affects: reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM-307, R-SIM-310), reference/12_simulation_model/04_containers_and_update_order.md (R-SIM-206 to R-SIM-211, R-SIM-239), reference/03_buildings_and_world_objects/README.md (R-BLDG: the port occupant); DEC-016, DEC-455, DEC-650
- Specified behaviour:
  1. **Within an actor-grid cell.** The entries of one cell of one team layer are reported in ascending actor serial (R-SIM-137), never in the order of the cell's slots. The stable key of every actor-grid result is therefore cell x ascending, then cell z ascending, then team 0 to 5, then actor serial ascending. This key depends only on which actors are registered in which cells, never on when or in which order they registered, so the registration schedule of DEC-650 has no observable effect. The slot array and free list of a cell (R-SIM-307) are storage only and decide no order.
  2. **Rules that read the order.** The port occupant, area damage and its kill credit, and every nearest-actor selection that keeps the first actor found on a tie (table 2.9-A of `04_containers_and_update_order.md`) use the key of item 1.
  3. **Containers.** Simulation pools advance their objects in the slot order that the documented allocation rules give: ascending slot order, last-freed-first reuse and growth blocks appended after the highest slot (R-SIM-206 to R-SIM-209). Every participant computes this order identically, because every creation and removal happens inside the tick, so it is the stable key of container iteration that DEC-016 requires. No container iterates in hash order or memory order. Whether an object created or handed over during a tick advances in that tick, and whether container order stops depending on allocation history, is DEC-455.
- Options: A — as stated: a history-free order within a cell and the documented slot order for containers; B — keep the cell's slot order as the last key of actor-grid results, so that a result depends on the history of registrations, and the per-tick re-insertion of shaped actors that DEC-650 removes changes which Worker occupies a port and the order in which area damage is applied.
- Recommendation: A. DEC-016 asks for stable keys and DEC-650 for a registration schedule without visible effect, and a history-free order within a cell is the only way both hold. Actor serials are unique within a match and identical on every participant, so the order costs one comparison. Containers keep the documented slot order, because that order decides the same-tick advances that DEC-455 leaves to the design phase.

### DEC-261 Multi-round Siege rebuilds the match world at a tick boundary
- Status: Resolved
- Affects: reference/12_simulation_model/02_match_lifecycle_and_state_reset.md (R-SIM-029, R-SIM-030), reference/01_skirmish_rules_and_modes/siege.md (R-MODE); DEC-066
- Specified behaviour: When multi-round Siege is enabled (DEC-066), the match rules raise a rebuild request at the end of a round that has further rounds to play. The rebuild runs at a tick boundary: after every slice of the tick in which the round ended has run, and before the commands of the next tick apply. No participant advances the old match world after the request, so every participant rebuilds from the same state and none consumes a simulation draw that another does not. The rebuild rotates the team positions, destroys the match world and creates and initialises a new one without a loading view, restores the last processed slice index, resets the tick clock and sets the grace timer to `ref.match.grace_period` (R-SIM-029). It does not reset the object serial counter; whether it should is DEC-066.
- Options: A — the rebuild at a tick boundary, as stated; B — the reference behaviour, in which each participant rebuilds after it has advanced a participant-dependent number of update slices from the round-end flag, and those slices consume simulation draws.
- Recommendation: A. A rebuild point that depends on each participant's pacing makes the draws of the next round differ between participants. The tick boundary is the one point that every participant reaches with identical state.

### DEC-262 Generations of laser-bolt and Essence-orb handles, and the team selection
- Status: Resolved
- Affects: reference/12_simulation_model/03_entity_model_and_identity.md (R-SIM-127, R-SIM-138, R-SIM-142), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM: the team selection); DEC-028
- Specified behaviour: These rules apply the generational handles of DEC-028 to the two object kinds whose reference handles carry no usable serial, and to the team selection:
  1. **Laser bolts and Essence orbs.** The generation that a laser-bolt or Essence-orb handle carries is a per-slot counter of its pool that changes whenever the slot is reused. It is never taken from the object serial counter, so creating laser bolts and Essence orbs never changes the serial of an actor or an effect, and the serials that rules read (R-SIM-138) keep their reference values (R-SIM-127).
  2. **Team selection.** A team's selected actor is held as a generational handle and resolved again on every use. A selection whose actor has been removed resolves to "none" and never designates a later occupant of the same slot. Destroying an actor group clears the team's selected group when it names that group (R-SIM-142).
- Options: A — as stated; B — take the generation of laser-bolt and Essence-orb handles from the shared object serial counter, which shifts the serial of every later actor and effect; C — the reference behaviour, in which laser bolts, Essence orbs and the selected actor are bare slots, so a selection outlives the actor's death and designates the slot's next occupant.
- Recommendation: A. DEC-028 requires generational handles for every kind, and a per-slot counter provides them without moving the serials that the Barrier Emitter clearance, the dust-vortex lift height, formation slots and the Worker presentation variations read. A selection that moves silently to another actor is the stale-handle defect that DEC-028 removes.

## 2 Routes, groups and game speed

### DEC-263 Route and actor-group edge cases
- Status: Open
- Affects: reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM-345, R-SIM-350, R-SIM-351), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM-428)
- Specified behaviour: The reference defines no result for these cases, and the documents state these defaults:
  1. **A route built without a corridor test.** When `BuildRoute` runs no corridor test, because `_directRoute` is set or because the start or the target node is not found, the direct-route override does not apply, and the route keeps the points that merging leaves (R-SIM-345, R-SIM-350, R-SIM-351). The straight-walk flag and the straight cost that the override reads are defined only by the corridor test. A direct route between two land points less than `ref.sim.route.merge_distance` (50 wu) apart with a clear path therefore collapses to their midpoint, a route of one point.
  2. **A group attack with no candidate.** When `ActorGroup::Attack` finds no member that is enabled, alive and not reloading, no member attacks and no candidate draw is made (R-SIM-428). The reference would index an empty candidate list.
- Options: A — the stated defaults; B — a route built without a corridor test keeps its two end points, from and to, whatever their distance, and item 2 as stated.
- Recommendation: B for item 1 and A for item 2. A direct route is asked for to walk straight to the target, and a route that ends at the midpoint stops the walker short of a target less than 50 wu away; B gives the result that the override gives after every corridor test that finds a clear straight walk. Item 2 has no alternative that keeps the draw stream unchanged.
- Decide by: design phase

### DEC-264 What runs while game speed is 0
- Status: Open
- Affects: reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM-541, R-SIM-548 to R-SIM-552), reference/01_skirmish_rules_and_modes/README.md (R-MODE: the grace period); DEC-039, DEC-092, DEC-225
- Specified behaviour: Game speed is tick scheduling (DEC-039): every tick has the length `ref.sim.tick_period`, and game speed sets how many ticks run per unit of real time, so that over any interval of real time the number of ticks run equals the integral of the game speed over the interval divided by the tick period, with any fraction carried to the next interval and never dropped (R-SIM-548). While the world is held at speed 0, these activities keep a real-time cadence of one per `ref.sim.tick_period` of real time: command frames, the speed blend of the tick clock, the match-rules update, which runs the grace countdown, and, while the world is held for the start of a match, the Supply crate drop rule (R-CRATE-102, DEC-116). A held match can therefore resume, and the speed ramps keep their real-time lengths. The hold countdown of a speed request counts the ticks the world runs (R-SIM-549, DEC-092). The exact scheduling and rounding rule of the world ticks within that cadence is decided with DEC-225.
- Options: A — as stated: one real-time cadence for commands, the speed blend and the held-world rules update and the start-hold drop rule, with world ticks scheduled within it; B — the start hold and the pause become states of the match outside the tick clock, entered and left by sequenced commands that take effect at once (DEC-225 item 5), so game speed is never 0 inside the clock and nothing needs a separate real-time cadence.
- Recommendation: A for the reference library, which keeps the reference ramps and the grace countdown unchanged; the design phase weighs B together with DEC-225, because a persistent world has no pause (vision O4) and B leaves one clock to reason about.
- Decide by: design phase

### DEC-265 Real duration of the speed-crate holds
- Status: Open
- Affects: reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM-541, R-SIM-557), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE-203; R-CRATE: Time Dilation and Time Rush), reference/02_units/hazards_and_projectiles.md (R-UNIT-733); DEC-039, DEC-092
- Specified behaviour: The hold of a Time Dilation or a Time Rush lasts `ref.sim.speed.crate_hold` (151) world ticks, because the hold countdown is a per-tick rule that scales with game speed (DEC-092, R-SIM-557); the speed ramps keep their real-time lengths. Its real duration therefore depends on the speed that the outcome sets: from 37.75 s of real time at 0.4× to 75.5 s at 0.2× for a Time Dilation, and from 7.55 s at 2.0× to about 11.6 s at 1.3× for a Time Rush. The slow-motion hold of the meteor shower, `ref.sim.speed.meteor_hold`, is 40 world ticks, which last 10 s of real time at 0.4× under A and 4 s under B. In the reference, where one tick runs per 0.1 s of real time at every speed, the hold lasts 151 command frames (15.1 s of real time) at every speed.
- Options: A — as stated: the hold counts world ticks; B — the hold counts command frames, the real-time cadence of DEC-264, so it lasts `ref.sim.speed.crate_hold` × `ref.sim.tick_period` (15.1 s) of real time at every speed, as in the reference; under B a request is back at 1.0 after 322, 316, 310, 284, 314 and 330 command frames for the targets 0.2, 0.3, 0.4, 1.3, 1.65 and 2.0, and the meteor after 117 (the ramps of acceptance criterion A6 of reference/12 01 plus a hold of 151 or 40 command frames).
- Recommendation: B. The hold is the duration of the speed effect itself, and players read it in real time: under A a Time Dilation slows the match for two and a half to five times as long as the reference outcome, and a Time Rush ends up to twice as fast. B keeps every rule that DEC-092 scales unchanged, because only the counter of the speed request moves to the real-time cadence, which already carries the speed blend.
- Decide by: playtest

## 3 Simulation and presentation

### DEC-266 Decorative objects in the simulated pools
- Status: Open
- Affects: reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM: Table 1 and Table 2), reference/12_simulation_model/03_entity_model_and_identity.md (R-SIM-124, R-SIM-138), architecture/components/animation_and_vfx/README.md (VFX); DEC-013, DEC-222
- Specified behaviour: Snowflakes, Wisps, muzzle flashes and shell casings are effects in the simulated effect pool: each takes a slot and an object serial, and creating one shifts the serial of every later actor and effect, which rules read (R-SIM-124, R-SIM-138). Their random draws come from the presentation generator (DEC-222 item 7), but the objects themselves stay simulation objects, created identically on every participant. The hover draws of Essence orbs stay simulation draws, because an orb's position is simulation state that collection reads.
- Options: A — as stated; B — move snowflakes, Wisps, muzzle flashes and shell casings into presentation pools driven by presentation events (DEC-007), so that they take no object serial; the serial of every later actor then changes, and with it which actor receives which serial-dependent value (the Barrier Emitter clearance, the dust-vortex lift height, formation slots and the Worker presentation variations), while every distribution stays the same.
- Recommendation: B, decided together with the simulation and presentation boundary (DEC-222). Decorative objects in simulation containers cost tick time and make rule outcomes depend on cosmetic content, so that adding or removing a cosmetic effect changes play; under B no cosmetic change can reach a rule. The Essence-orb hover draws stay in the simulation under both options.
- Decide by: design phase

## 4 Obstacle grid

### DEC-267 Barrier Emitter line in the obstacle grid
- Status: Open
- Affects: reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM-322), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG-413); DEC-088
- Specified behaviour: A linked Barrier Emitter adds its handle to the obstacle-grid cell of every sample of the line to its linked emitter and, for every sample whose cell x and cell z are both non-zero, to the four edge-adjacent cells, so the line occupies a band about three cells wide (R-SIM-322). Users that test every cell of their query find no extra hit; the turret line of sight, which samples cells along its path, finds the emitter in more of its sample cells (R-BLDG-413).
- Options: A — as stated; B — the edge-connected line: where two consecutive samples lie in cells that differ in both x and z, only the two cells edge-adjacent to both also receive the handle.
- Recommendation: A for the reference library. B changes which emitters the turret line of sight finds, and so which turret shots a Barrier Emitter line blocks.
- Decide by: design phase
