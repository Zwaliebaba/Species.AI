# Decision register: verdicts and fixes

> Status: Reference decisions · Range: DEC-001 to DEC-119 · Related: [decisions index](README.md), [conventions](../CONVENTIONS.md)

This file is the canonical register for the scope and behaviour decisions of the reference library. It holds the headline verdicts on the reference core loop, simulation timing and hook-only features; the simulation-integrity rules; the scope verdict for each reference subsystem; the verdicts on individual mechanics, which keep their rules as the reference states them; one decision for every reference defect that Species fixes; and the design-critical uncertainties and the values to confirm by measurement or playtest. Each entry states the behaviour the reference documents specify now. A **Resolved** entry states the corrected rule that the documents apply; an **Open** entry states the faithful reference behaviour as the default, with a recommendation; a **Dropped** entry records content the library does not specify. The networking model, numeric model, simulation threading, tick rate, sessions and services, lobby protocol, persistence and terrain runtime model are design-phase topics recorded under `decisions/deferred/`; engine-technology choices are ADRs under `ADR/`. Other decision files cite the entries here instead of repeating them.

## Index

The generated [register index](README.md) lists every entry of this file with its title, status and link. The sections of this file:

| Section | Entries | Count |
|---|---|---|
| [1 Headline verdicts](#1-headline-verdicts) | DEC-001 to DEC-006 | 6 |
| [2 Simulation-integrity rules](#2-simulation-integrity-rules) | DEC-007 to DEC-016 | 10 |
| [3 Scope verdicts for the reference subsystems](#3-scope-verdicts-for-the-reference-subsystems) | DEC-017 to DEC-029 | 13 |
| [4 Rules kept as the reference states them](#4-rules-kept-as-the-reference-states-them) | DEC-030 to DEC-044 | 15 |
| [5 Features designed by the design phase (hooks only)](#5-features-designed-by-the-design-phase-hooks-only) | DEC-045 to DEC-051 | 7 |
| [6 Content outside the reference](#6-content-outside-the-reference) | DEC-052 to DEC-063 | 12 |
| [7 Reference defects that Species fixes](#7-reference-defects-that-species-fixes) | DEC-064 to DEC-105 | 42 |
| [8 Design-critical uncertainties and values to confirm by measurement](#8-design-critical-uncertainties-and-values-to-confirm-by-measurement) | DEC-106 to DEC-117 | 12 |
| [9 Further integrity rules](#9-further-integrity-rules) | DEC-118, DEC-119 | 2 |

## 1 Headline verdicts

### DEC-001 Reference core loop: crowd-and-territory economy
- Status: Open
- Affects: reference/00_reference_overview.md (R-VIS), reference/05_population_flow.md (R-ECON), reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: The reference game has no build queue and no currency. A team's army is the population that its held Spawn Nodes produce: a Spawn Node is captured by a single team with at least 3 operators on its 5 ports and no enemy on any port, activates 300 ticks after an ownership change, and produces in bursts and pauses up to a per-team population cap. Workers fight and re-plan on their own; Leaders act as signposts and formation leaders; Supply crates open to outcomes that are 90 % beneficial and 10 % hostile and feed six Power slots; six modes share one rules skeleton; bots play on a node graph with perfect information and no other advantage. The reference library specifies this loop in full and keeps it as the rule set of the skirmish mode.
- Options: A — the world game keeps the reference loop unchanged; B — the world game adds settlement building and a resource economy on top of essence-driven growth, while skirmish keeps the reference loop; C — the world game replaces the loop entirely.
- Recommendation: B. The vision brief overrides "no build queue and no currency" for the persistent world, where settlements gather resources, build and grow, but keeps a skirmish mode that is the combat balance test bed (vision O8). Keeping the reference loop authoritative for skirmish preserves the tuned combat and population behaviour; the design phase decides how settlement growth uses Essence orbs and whether a resource layer exists.
- Decide by: design phase

### DEC-002 Integer tick timers and the 11-tick mode second
- Status: Resolved
- Affects: reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/05_population_flow.md (R-ECON), reference/03_buildings_and_world_objects/README.md (R-BLDG), reference/04_supply_crates_powers_and_weapons/README.md (R-CRATE), reference/02_units/README.md (R-UNIT)
- Specified behaviour: Every rule timer, cooldown, lifetime and probability in the simulation is an integer number of ticks, or a per-second rate converted to ticks when the data loads; no simulation timer is a floating-point countdown. The **mode second** is an explicit named constant of 11 ticks (1.1 s at the reference tick rate of 10 ticks per second). The match clock, elimination countdowns, Relic timers, reinforcement waves and rank checks advance once per mode second. The reference tick counts, with each nominal duration, are: mode second (1 s) 11; grace period (3.0 s) 30; victory timers (10, 15 and 5 s) 101, 151 and 51; Comeback Bonus interval (45 s) 450; production pause (10.0 s) 101; burst request interval (0.2 s) 2; rocket turret reload (1.5 s) 15; Convert Turret reload (0.5 s) 6; mortar turret reload (4.0 s) 40; Spawn Node activation (30.0 s) 300; Essence orb birth 61; Relay transit at most 10h + 1 for h hops (11 for the first hop, 10 for each further hop; the same-tick rule that sets the exact count is DEC-455). Further reference tick counts: burst pause 101–150 (mean 125.5); first production timer 21–40; first ownership evaluation 1–20, later evaluations 21–30; reinforcement wave every 660, with a first gap of 649; Spawn Control elimination 330; first Comeback Bonus grant 51, for a team already eliminated on the first tick after the grace period; Spawn Surge 50 or 51 (DEC-304); Essence orb floating 301–500 and death ascent 901; Collector and Builder idle retarget 11; bot Collector decision 11; Worker idle retarget 11–20; corpse fade 25 on the ground and 50 in the air; Population Limiter recount 91–110; Hive orb-to-ant conversion 10; Crawler egg hatch 151; Collector delivery one Essence orb per tick. The three mode-second timers start at 0, so the first mode second ends on the first tick after the grace period. A rule that compares match time with a deadline of T seconds counts T ÷ `ref.sim.tick_period` ticks (reference/03 R-BLDG-403), so the Hive orb-to-ants conversion and the Population Limiter overpopulation period are each 10 ticks, not an alternation of 10 and 11 (reference/05 R-ECON-064, R-ECON-084). Seconds appear only as a derived display value next to the tick count.
- Options: A — integer tick counts with the mode second as an explicit 11-tick constant; B — real-time countdowns in seconds; C — integer ticks with a 10-tick second from the start.
- Recommendation: A. It keeps every tuned value of the reference valid for the first playable, removes floating-point accumulation from rule timing, and makes each timer testable: a table-driven test checks the tick count of every timer, and a static check rejects floating-point timer fields in simulation code. Re-tuning to a 10-tick second is a separate, later decision (DEC-003).

### DEC-003 Mode-second length after playtest
- Status: Open
- Owner decision: O17 (2026-10-17): keep the 11-tick mode second for the first playable; run an A/B playtest of 10 against 11 ticks before launch.
- Affects: reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/05_population_flow.md (R-ECON)
- Specified behaviour: The mode second is 11 ticks (DEC-002). Every per-second rule, wave and elimination timer, the on-screen match clock and all balance values are tuned under it, so displayed seconds run 10 % slower than real seconds.
- Options: A — keep the 11-tick mode second permanently; B — change the mode second to 10 ticks and re-tune every value that depends on it; C — keep 11 ticks internally but display real seconds.
- Recommendation: A for the first playable, then an A/B playtest of 10-tick and 11-tick seconds. Changing the constant alone shifts all mode timings and the economy by 10 %, so B is acceptable only with a deliberate re-tune of the affected tuning keys.
- Decide by: playtest

### DEC-004 Reference tuning values adopted as the Species baseline
- Status: Resolved
- Affects: every reference document tuning table (R-VIS, R-MODE, R-UNIT, R-BLDG, R-CRATE, R-ECON, R-CTRL, R-BOT, R-MAP, R-META, R-AUD, R-TUT, R-SIM)
- Specified behaviour: The reference documents keep every numeric tuning value: stats, timings, caps and probabilities. Species may adopt these values as its starting values. Tuning tables carry no "provisional" flag on account of where a value comes from. A legal check of the adopted values is required before any commercial release.
- Options: A — adopt the reference values as the starting baseline; B — keep the structure and re-tune every number before use.
- Recommendation: A, by owner decision (2026-10-17). A rule that this register keeps as the reference states it therefore means "same rule and same numbers" for the first tuning baseline; the design phase re-tunes freely afterwards.

### DEC-005 The reference library is the specification of record
- Status: Resolved
- Affects: every reference document (R-VIS, R-MODE, R-UNIT, R-BLDG, R-CRATE, R-ECON, R-CTRL, R-BOT, R-MAP, R-META, R-AUD, R-TUT, R-SIM)
- Specified behaviour: Where a reference document specifies a rule or a value, that statement is the reference behaviour. Species sets no parity target against any other edition or build of the reference game, and no requirement depends on such a comparison.
- Options: A — the reference library is authoritative; B — verify the library against another edition of the reference game before use.
- Recommendation: A. Species is a new game; the library is complete for its purpose, and the decisions in this register record every place where the reference behaviour is uncertain or changed.

### DEC-006 Reference quirks are kept as explicit, named rules
- Status: Resolved
- Affects: every reference document (R-VIS, R-MODE, R-UNIT, R-BLDG, R-CRATE, R-ECON, R-CTRL, R-BOT, R-MAP, R-META, R-AUD, R-TUT, R-SIM)
- Specified behaviour: A reference behaviour that looks accidental and is not corrected by a Resolved decision is specified as an explicit rule with a named tuning key or constant, never left implicit in an algorithm. Each such quirk is decided individually in this register (for example DEC-003, DEC-065, DEC-079, DEC-081 and DEC-094) before content tuning starts. Where both variants are cheap to build, both are playtested.
- Options: A — name and specify every kept quirk, and decide each one before tuning; B — correct quirks silently during implementation.
- Recommendation: A. The reference balance is tuned under these quirks (the 11-tick second, the doubled reinforcement advance, the binary catch-up bonus, the per-team cap divisor and the approximate grid queries), so changing one changes the game; naming them makes each change a visible decision and protects the kept rules and the fixes of §7 from late rework.

## 2 Simulation-integrity rules

### DEC-007 Presentation never writes simulation state; rules emit presentation events
- Status: Resolved
- Affects: reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/10_audio_design/ (R-AUD), reference/09_front_end_and_match_flow/README.md (R-META), architecture/components/renderer/README.md (RND), architecture/components/ui_framework/README.md (UI)
- Specified behaviour: The simulation step changes only simulation state. Messages, sounds, help notifications, achievement progress and kill totals that a rule produces are emitted as presentation events at the tick where the rule fires. The renderer, audio and UI consume these events and read immutable snapshots of simulation state; they never write simulation state. Tutorial and help logic changes the world only through sequenced simulation events (DEC-103). Spider leg and foot state, with its wall-clock swing timing, is presentation only, and no simulation rule reads it. The reference render paths that write fields of simulated objects are listed in DEC-222 item 2.
- Options: A — rules emit presentation events, and presentation is read-only; B — the reference behaviour, in which the rules step executes messages, sounds, help notifications, achievement checks and kill totals directly, and presentation code can write simulation state.
- Recommendation: A. Presentation or local state crossing into the simulation is the cause of every divergence source that reference/12 lists. A hard boundary also lets the simulation run headless and be replayed.

### DEC-008 Player intent reaches the simulation only as commands
- Status: Resolved
- Affects: reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM), reference/06_controls_camera_and_commands/ (R-CTRL), reference/02_units/strike_team.md (R-UNIT), architecture/components/input/ (INP), architecture/components/camera_and_sequences.md (CAM)
- Specified behaviour: Every player interaction with the world is either a command of the player command catalogue or a field of the continuous control state. The simulation never reads the local camera, local input devices, UI state or client preferences. Direct control is available only for the special units and structures: Strike team, Transport, Collector, Builder, turrets and the Beam Dish. The sending client converts every camera-relative quantity of direct control (aim direction, movement direction and the look-around state that steers a Strike team) to world space before it enters the control state.
- Options: A — commands and world-space control state only; B — the reference behaviour, in which Strike team direct control reads the local camera's look-around flag inside the simulation.
- Recommendation: A. Reading the camera inside the simulation makes the outcome depend on one client's view. Converting on the sender keeps the control feel and keeps the simulation a function of its commands alone. Command authority, that is, who may issue which command, is a networking-model question (DEC-202).

### DEC-009 Simulation time comes only from the tick counter
- Status: Resolved
- Affects: reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM), reference/01_skirmish_rules_and_modes/siege.md (R-MODE), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG)
- Specified behaviour: Simulation code reads time only from the tick counter. No rule, achievement flag or actor behaviour reads the wall clock or a render-frame clock; the Siege flag that records a complete defence uses the tick counter. The launch spectacle, in which nearby Workers turn to watch a launching Launch Vessel, is inactive in matches. Presentation uses its own presentation clock (DEC-024).
- Options: A — tick counter only; B — the reference behaviour, in which the Siege complete-defence flag reads the wall clock.
- Recommendation: A. Wall-clock reads differ between machines and between runs; the tick counter is the only time base that a replay reproduces.

### DEC-010 Process-global settings never feed the simulation
- Status: Resolved
- Affects: reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM), reference/02_units/README.md (R-UNIT), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META)
- Specified behaviour: A setting that changes simulation results exists only as a match option inside the match setup. The reference has a global difficulty level that scales only the actor speed stat; it is 0 in every match, because every map load resets it to 0 and no match path changes it. The difficulty value that a map lists is a descriptive rating, not a setting. Species specifies actor speeds at difficulty level 0 and has no process-global difficulty. Instant-unit copies of the centipedes of team slot 1 exist only at a load difficulty factor above 1, which never occurs in a match, so no map rule specifies them.
- Options: A — simulation-relevant settings only as match options, with difficulty level 0 built into the stats; B — keep a client preference that the simulation reads.
- Recommendation: A. A preference read by the simulation is safe only while every load overwrites it, and a modified client could change it unnoticed. As a match option, every simulation input is part of the recorded match setup.

### DEC-011 Per-match state is owned and reset at every match start
- Status: Resolved
- Affects: reference/12_simulation_model/02_match_lifecycle_and_state_reset.md (R-SIM), reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/02_units/transport.md (R-UNIT), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE)
- Specified behaviour: All mutable simulation state belongs to the match world. It is created at match start and destroyed at match end, and no simulation state survives between matches. In particular: the Control Zone counter that Flag Chain initialisation adds to starts at zero in every match; the per-team Spawn Node counts, including the count for team slot 5, start at zero; the Transport weapon-cycle debounce belongs to each Transport for one match; the tick clock is reset at match start to game speed 0 with no request, no hold and no lock, so no Time Dilation or Time Rush hold reaches the next match (R-SIM-551). The actor-group offset table is cleared at every match start and end, and regenerated at a fixed step of match initialisation (DEC-652). The Source Node registry id belongs to the match as well: it starts as none, and each Source Node writes it at the start of its advance, before any request of the match reads it, so a value left by an earlier match has no effect (R-SIM-024). The teleport transit table is per-match state (DEC-223 item 2). The Spawn Node count of team slot 5 matters because a Spawn Node skips its 300-tick activation delay when it is its team's only Spawn Node.
- Options: A — per-match ownership of all mutable state, with a static-analysis rule that forbids mutable statics in simulation code; B — the reference behaviour, in which three counters and a speed hold can survive into the next match: the Control Zone counter is process-wide and recomputed only at the start of a Zone Control match, the per-team Spawn Node counts are not reset, and the weapon-cycle debounce is one value shared by every Transport.
- Recommendation: A. A surviving counter makes a match depend on the session history; the Control Zone counter diverges whenever two participants played different earlier matches.

### DEC-012 Collision volumes are map and asset data
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), architecture/components/asset_pipeline_and_resources.md (AST), architecture/components/data_schemas/ (DAT)
- Specified behaviour: Every collision and hit volume that the simulation uses, tree hit spheres included, comes from map or asset data produced by the content build. No simulation volume is derived from render geometry, and no load-time generation of simulation data draws from the presentation generator. Hit-test geometry is a separate collision asset that the content build validates. The reference tree crown hit centre ignores the tree's rotation, so the baked collision data states each centre explicitly. The mesh checks of the content build are in ADR-059; the Relic mesh is chosen by a simulation draw among 8.
- Options: A — collision volumes as baked, validated data; B — the reference behaviour, in which tree hit spheres are generated from render geometry at load and draw from the shared, non-thread-safe presentation generator.
- Recommendation: A. The reference path makes simulation data depend on render code and on the order of presentation draws at load. Baked data is identical everywhere and can be tested.

### DEC-013 Random-stream discipline: per-match seed and a separate presentation generator
- Status: Resolved
- Affects: reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), architecture/components/animation_and_vfx/ (VFX), architecture/components/audio_engine/ (AEN)
- Specified behaviour: The simulation generator is seeded once per match, at match start, and the seed is part of the recorded match setup. No simulation draw happens before match start. Cosmetic effects, audio variation and all other presentation randomness draw from a separate presentation generator, never from the simulation generator. Simulation draws follow the draw discipline of reference/12: per-system sub-streams, one draw per statement and no draws inside argument lists. The draw order of the reference generator is not reproduced. The cosmetic draws that move from the simulation stream to the presentation generator are listed in DEC-222 item 7, and the audio engine has its own presentation-generator instance (ADR-022).
- Options: A — per-match seed, per-system simulation sub-streams and a separate presentation generator; B — the reference behaviour, in which one stream serves both cosmetic and gameplay draws, and an unsynchronised generator is used inside simulation code and by audio and tree generation.
- Recommendation: A. Moving cosmetic draws off the simulation stream changes nothing visible and removes the coupling between presentation and simulation. Bit parity with the reference stream has no value for a new game.

### DEC-014 Bounded algorithms with a deterministic fallback
- Status: Resolved
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md (R-CRATE), reference/02_units/hazards_and_projectiles.md (R-UNIT), reference/07_bots/ (R-BOT), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM)
- Specified behaviour: No simulation algorithm loops without bound. Every rejection loop or search has a capped number of attempts and a deterministic fallback, for example the nearest valid point from an explicit candidate list. This covers the beneficial-outcome draw when a Supply crate opens, the Missile Barrage sea search, the Abductor landing and wander searches, the land searches of lightning strikes and dust vortices, the bot placement loops, and the bot colour choice, which picks from the list of free colours instead of re-rolling. The Collector release loop and the Builder drop loop remove a carried entry that has become invalid, so each ends after one pass over its entries; in the reference they end only when every carried entry is valid, so an invalid entry makes them spin. The spawn-network link walk is bounded by DEC-456. Each cap is a tuning key with its fallback (DEC-857: for example `ref.unit.abductor.wander_attempts`, 2000 attempts), so a search on an authored map that lacks the terrain it looks for ends at its cap and takes its fallback instead of hanging. A content validator runs every crate outcome and every bot handler on every map at build time and reports a map on which a search reaches its cap.
- Options: A — capped attempts with a deterministic fallback; B — the reference behaviour, unbounded rejection loops that terminate only because of the map data.
- Recommendation: A. An unbounded loop hangs the simulation as soon as a map lacks the terrain it searches for. A cap sized so that a search on a valid map almost never falls back (DEC-857), with a fallback, keeps the reference outcome distribution on every valid map; empty candidate lists are DEC-858.

### DEC-015 Content is validated at build time, and no load path is fatal
- Status: Resolved
- Affects: architecture/components/data_schemas/ (DAT), architecture/components/asset_pipeline_and_resources.md (AST), architecture/components/build_tooling_and_distribution.md (BLD), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
- Specified behaviour: The content build validates every data file against its schema and every asset reference against the asset set. A missing mandatory field, an unterminated block, a stray blank line or a missing asset is a build error with a diagnostic, and so is a key listed twice, such as a second population-cap line in a map, where the reference takes the last line (DEC-801). At run time a loader never passes a missing token on as a value and never hangs on malformed text, and a missing asset is reported as a diagnostic with a fallback, never a fatal assertion. Loaders are fuzz-tested.
- Options: A — schema validation at build time and fault-tolerant loaders; B — tolerate malformed authoring text at load and leave its effect to each loader.
- Recommendation: A. Malformed content that reaches a loader can stop or hang the program; the content build rejects it with a diagnostic (ADR-059), so broken maps and data are caught before release.

### DEC-016 Tie-breaks are explicit ordered rules
- Status: Resolved
- Affects: reference/12_simulation_model/04_containers_and_update_order.md (R-SIM), reference/01_skirmish_rules_and_modes/zone_control.md (R-MODE), reference/01_skirmish_rules_and_modes/launch_race.md (R-MODE), reference/01_skirmish_rules_and_modes/relic_run.md (R-MODE), reference/07_bots/ (R-BOT), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE-114), reference/00_reference_overview.md (R-VIS-076)
- Specified behaviour: Every tie that container iteration order decides in the reference is an explicit rule over a stable sort key: a Control Zone ownership tie goes to the lowest team id; simultaneous Launch Vessel launches go to the lowest team id; when several Relic Spawn Zones are equally far, the Relic spawns at the first in stable-id order; bots consider structures in stable structure-index order; the port occupant is the first operating Worker in grid result order, whose stable key is cell x, then cell z, then team 0 to 5, then actor serial (DEC-260); a Supply crate's majority tie goes to the lower team slot (R-CRATE-114); the single-round Siege winner is the first slot of the winning side; the Relic Run co-op winner is the last slot of the group; the Launch Vessel lookup takes the last matching structure; and Command Console ownership goes to the first qualifying team in slot order and changes only to a team with strictly more operators. Containers iterate in their documented slot order, never in hash order (DEC-260).
- Options: A — explicit ordered rules sorted by stable id; B — leave ties to container order.
- Recommendation: A. It reproduces the reference outcomes exactly while leaving the implementation free to choose its containers.

## 3 Scope verdicts for the reference subsystems

### DEC-017 Match rules and modes: one rules skeleton, typed rulesets
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/ (R-MODE: the README and all six mode files), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
- Specified behaviour: Six modes vary only the territory unit and the score on one shared skeleton: the world updates every tick, the mode rules evaluate once per mode second, and a declared winner starts the end-of-match sequence. The territory units are Spawn Nodes (Spawn Control), Control Zones (Zone Control), carried Relics (Relic Run), the Core Device and its Command Consoles (Siege), the Launch Vessel and its fuel chain (Launch Race) and chains of Flag posts (Flag Chain). Each mode is a typed ruleset with a score function, an elimination predicate, a win predicate, a time-up decider, a sudden-death policy, a typed option schema and a HUD formatter. The option set has 38 option definitions, plus the lobby settings and the per-map overrides for reinforcements, Transports, turrets, instant capture, Beam Dish rules, the Visitor faction, the airstrike threshold, excluded crate outcomes and population caps. The six mode schemas hold 38 option definitions; the reference defines one further, the lives option of the debug-only mode, which is not specified (DEC-058). The rule decisions DEC-064 to DEC-078 and DEC-082 apply.
- Options: A — keep every rule and number, implemented as typed rulesets; B — redesign the modes.
- Recommendation: A. The skeleton is the right abstraction and every rule is specified; the corrections remove accidents without changing the intended rules. Balance re-tuning follows the mode-second decision (DEC-003) and the reinforcement correction (DEC-065).

### DEC-018 Population flow: keep the model and its numbers
- Status: Resolved
- Affects: reference/05_population_flow.md (R-ECON), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/02_units/essence_orbs.md (R-UNIT)
- Specified behaviour: Territory produces a flow of bodies. A Spawn Node belongs to a single team with at least 3 operators on its 5 ports and no enemy on any port, and ownership is re-evaluated every 2.0 + U[0,1) s. After an ownership change production waits 300 ticks, except at a team's only Spawn Node or with the instant-capture option. An active Spawn Node issues one request every 2 ticks for int(20 × H) requests, where H is the Catch-up bonus multiplier, then pauses for 10 + U[0,5) s (the fixed 10 s part takes 101 ticks); at the default multiplier this yields about 1.22 Workers per second per Spawn Node. Essence orbs travel along Relays in at most 10h + 1 ticks for h hops (DEC-455) and are discarded if the Spawn Node has turned neutral. Every death yields one Essence orb, collectable for 30–50 s, and a Collector recycles orbs into its own team's Workers. Transit Gates release reinforcement waves every 60 mode seconds, 40 Workers per owned Transit Gate in Launch Race and Flag Chain, with Transport, turret and airstrike variants. The per-team cap, the Collector bypass, the catch-up curve and Hive accounting are DEC-079, DEC-080, DEC-081 and DEC-084.
- Options: A — keep the model and numbers; B — re-tune the flow.
- Recommendation: A. The flow from territory to population is the defining feel of the reference. Income is linear in Spawn Nodes held, so snowballing is governed by the cap and catch-up decisions, not by the flow itself.

### DEC-019 Units and actors: keep the roster, adapt the data representation
- Status: Resolved
- Affects: reference/02_units/ (R-UNIT: every file of the folder), architecture/components/data_schemas/ (DAT)
- Specified behaviour: The roster and its behaviour are kept: the autonomous Worker with 25 states and a staggered task search every 1–2 s; the Leader; the Strike team; the Transport; the Collector; the Builder; the airstrike and Missile Barrage weapons; the crate-spawned creatures and hazards (Crawler, spider, centipede, ant, Brood Drifter, Reaper Serpent, Seed Pod, Crawler egg, meteor); and the Abductor with its Visitor-faction drop. Every Worker is armed, because the match research level of Workers is fixed at 4. Actor stats are floating-point values (DEC-090), and every per-frame or per-tick rule is a per-second rate scheduled in ticks (DEC-092). Actor kinds are typed schema entries, not row positions in a stat table; a missing record or an unknown kind is a build error, and a stat table whose row count differs from the kind count fails the content build. Actor kinds that no match creates are not specified (DEC-062).
- Options: A — keep the roster and behaviour, with typed floating-point data; B — keep the reference data representation as well.
- Recommendation: A. The roster and behaviour make the crowd feel; the representation changes only remove truncation and positional binding. The Worker task search is the largest simulation cost at 2,000 units, so its budget is measured early (DEC-106).

### DEC-020 Structures and world objects: keep the function, apply the corrections
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/ (R-BLDG: every file of the folder)
- Specified behaviour: The function, capture rules, ports and links of the 34 structure kinds that matches use are kept. The structure kinds are those of reference/03; no kind exists for a campaign only (DEC-052). A map's team column is a map position, remapped to the match team at load (DEC-025). Energy from Solar Arrays carried through grid pylons is the only active switch for Barrier Emitters. Walls are destroyed only by a Breach Charge or by their defender's scuttle (DEC-043). The structure decisions DEC-088, DEC-089 and DEC-105 apply, including binding port occupancy to the (structure, port) pair.
- Options: A — keep the function and rules and apply the corrections; B — keep the defects for parity.
- Recommendation: A. Several defects silently shape play, the invisible smoke-column obstacle among them (decided separately in DEC-089); listing each correction keeps its effect on play visible.

### DEC-021 Supply crates, powers and weapons: keep the outcome set, bound the selection
- Status: Resolved
- Affects: reference/04_supply_crates_powers_and_weapons/ (R-CRATE: every file of the folder)
- Specified behaviour: The outcome set, its probabilities and its timing are kept. A Supply crate drops at intervals of U(0.5, 1.5] × the crate drop-rate option (default 40 s), with at most int(map width ÷ 500 wu) crates on the map at once, and its placement is score-weighted towards lower-scoring teams. A crate opens for the majority team of the Workers within 35 wu, the team with the highest count, ties going to the lower team slot (DEC-016), after at least 51 ticks with at least 50 Workers; a neutral crate that nobody opens expires after 60 s. There are 31 live outcomes, split 90 % beneficial and 10 % hostile; hostile outcomes are events against everyone and are owned by the Rogue faction. A beneficial outcome is lost if the opening team's Power slots are full (DEC-033). A map may exclude outcomes, a lobby option limits drops to the basic outcomes, and each team may start with 0 to 3 powers. The starting-powers draw excludes Allegiance Scrambler, Supply Storm, Reinforcements, Frenzy, Spawn Surge, Sickness, Rocket Troops, Abductor, Swarm, Spitter Plants, Time Rush, Time Dilation, the Essence Grove, the outcome of DEC-057 and invalid values, and every starting power is locked for 30 s with a visible countdown. The outcome is chosen by a bounded draw over an explicit candidate list (DEC-014), and cosmetic crate effects draw from the presentation generator (DEC-013).
- Options: A — keep the outcomes, probabilities and timing, with bounded selection; B — redesign the crate economy.
- Recommendation: A. The Supply crate is the strategic wildcard and the built-in catch-up lever, and its rules are fully specified.

### DEC-022 Bots: keep the design, apply the corrections
- Status: Resolved
- Affects: reference/07_bots/ (R-BOT)
- Specified behaviour: Bots keep the node-graph design: bot navigation nodes linked within 600 wu, each owned by head count, with per-team priority fields; a dispatcher in which only friendly nodes release troops; Leader conveyors and formations; one handler per power; and one strategy per mode. Difficulty is tempo and randomness only: decision periods of 200, 101 and 11 ticks and randomness of 100 %, 50 % and 0 % for the easy, standard and hard bot. Bots have perfect information and no other advantage. The Rogue and Visitor factions always have a bot controller, and it drives their Power slots. Bots run inside the simulation. The bot corrections DEC-014 and DEC-087 apply. Rebuilding the navigation graph after Supply crate, turret and Beam Dish events may be incremental if the resulting graph is identical.
- Options: A — keep the design with the corrections; B — a new bot architecture.
- Recommendation: A. The design is cheap, deterministic and readable, and matches the crowd model. Per-bot difficulty settings and fair-sensing tiers for the world game, where bots play under fog of war, are design-phase work.

### DEC-023 Controls and commands: keep the command model and control scheme
- Status: Resolved
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM), architecture/components/input/ (INP)
- Specified behaviour: Player control splits into discrete commands (select, order, promote, place, terminate, toggle) and a continuous control state sampled at the simulation rate. The selection circle, Leader orders and the context rules of the secondary pointer button are kept; the context resolver reproduces the reference priority order of that button's five meanings exactly. Camera-relative quantities are converted to world space on the sender (DEC-008). The power bar is clickable and power ids are wide (DEC-083), and key rebinding is reachable (DEC-050). Gesture input and the icon bar are not specified (DEC-055). The input stack is an engine choice recorded as an ADR.
- Options: A — keep the command model and control scheme with these adaptations; B — a new control scheme.
- Recommendation: A. The command split is clean and suits any networking model; reproducing the exact context priority order is what keeps the scheme feeling the same.

### DEC-024 Camera: keep the free camera, clamp it and use one presentation clock
- Status: Resolved
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL), architecture/components/camera_and_sequences.md (CAM)
- Specified behaviour: The cursor-follow free camera pans at 250 wu/s (1,000 wu/s with the speed modifier held), and zooming narrows the vertical field of view from 60° to 15°. Each map has camera views: a start view and one view per player slot. Every match start cuts to the start view and then to the player's view. Camera distances are floating-point values, and the camera is clamped to the map bounds. The camera is local and is never read by the simulation (DEC-008). All camera motion uses one presentation clock. Scripted camera moves land exactly on their target view, with no overshoot, and interpolate orientation spherically; the reference blends orientation through a midpoint rotated by half the sine of the angle, an approximation of half the angle, with quadratic ease-in and ease-out, and forces the up vector to world up every frame. During scripted moves the field of view eases with a 10 s time constant. The live player-camera modes are those of R-CTRL-457. The building focus and approach exist only as timeline operations (CAM-053 to CAM-058), and the free fly mode only as a developer tool of Debug builds and the map editor (CAM-080 to CAM-084); no other camera mode is specified.
- Options: A — keep the free camera and its views with these corrections; B — a new camera design.
- Recommendation: A. The camera is distinctive and cheap; the corrections remove an unclamped edge, integer distances, overshooting moves and mixed clocks.

### DEC-025 Maps and level design: keep the capability set and the generator look
- Status: Resolved
- Affects: reference/08_maps_and_level_design/README.md (R-MAP), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), reference/03_buildings_and_world_objects/README.md (R-BLDG), architecture/components/terrain/ (TER)
- Specified behaviour: A map is one definition. Its start positions are map positions, remapped to the match teams at load, so one map serves 2 to 4 players. A map declares typed mode objects, per-map rule overrides, lights, camera views and instant units, and the bot-navigation inputs of the reference maps: node priority modifiers, links placed by hand and cliff checks. The look of the terrain generator is kept: per-tile diamond-square with guide grids, max-composition of tiles and five flatten types. Every map is authored new. Whether terrain is generated at load or baked is a design-phase topic (DEC-246).
- Options: A — keep the capability set and the generator look; B — a new map model.
- Recommendation: A. One definition serving several player counts is cheap map reuse, and the generator look is part of the visual language.

### DEC-026 Front end and match flow: keep the flow and its page catalogue
- Status: Resolved
- Affects: reference/09_front_end_and_match_flow/ (R-META: every file of the folder), architecture/components/ui_framework/pages_and_screen_recipes.md (UI)
- Specified behaviour: The front end is a single-window, data-driven page flow over the page catalogue of the UI framework (UI-320 to UI-322): the 15 pages of R-META-003 and the hook pages that the design phase designs, with one lobby page shared by single-player, host and client use, and a ready-up step. Every lobby change is a sequenced update of one lobby state, applied atomically and versioned; which participant validates it is the design-phase question of lobby authority (DEC-243). The server browser for custom matches keeps its filters, and every filter has its own key, so the password filter works. The loading view shows progress and keeps the connection alive. Achievements, leaderboards and quick match are designed by the design phase (DEC-047, DEC-050), and the reference has no front-end results page (DEC-048).
- Options: A — keep the flow and pages with these adaptations; B — a new front end.
- Recommendation: A. The data-driven single-window flow is simple and complete; lobby authority, discovery and late join are deferred topics.

### DEC-027 Audio design: keep the binding model, replace priorities and the mix
- Status: Resolved
- Affects: reference/10_audio_design/ (R-AUD), architecture/components/audio_engine/ (AEN)
- Specified behaviour: Game code fires (source, event) pairs, and data decides what plays. Each binding resolves to a sound blueprint of layered blocks with friend and foe variants, sample groups, generative looping ambience beds shaped by attack, decay, sustain and release envelopes, linked parameters, and an ambience crossfade driven by camera altitude. 182 (sound source, event) bindings fire in a match (reference/10). Voices carry authored priority classes and are virtualised rather than culled by an accidental radius; volume is controlled per bus rather than by one master volume; effects are applied per bus rather than per event. Every sound is authored new, and the event list is authored first so that a validator flags missing bindings. Music is designed new (DEC-049); the victory and defeat stings are bound by DEC-097 (R-AUD-307).
- Options: A — keep the binding model and replace the priority model and the mix; B — a new audio design.
- Recommendation: A. The binding model is clean and data-driven; the priority, culling and single-volume issues are corrected by construction (DEC-097, DEC-109).

### DEC-028 Simulation model: keep the object model, replace weak mechanisms
- Status: Resolved
- Affects: reference/12_simulation_model/ (R-SIM: the README and files 03, 04 and 05)
- Specified behaviour: The simulation has three object families: actors, structures and effects. An object's advance returns an explicit result that can request its removal. A change of ownership destroys the object and spawns a new one. Systems run in a fixed, documented order, and every object advances exactly once per tick; the reference spreads object updates over 10 slices per tick, and that update order is reference behaviour. Spatial queries use per-team uniform grids with 32 wu cells, an obstacle grid and a node-graph router. Every object kind, Essence orbs and laser bolts included, is referenced through generational handles. Scratch storage, the carryable-structure collision list included, is per call, never a shared buffer, and has no fixed limit, so it holds every result of its call (R-SIM-026, R-SIM-203). Outside-cell ids are checked like every handle, so an actor that enters a teleport structure, or a Strike team under direct control, whose outside-cell id is stale finds no object, a failed look-up rather than a crash. A Collector or Builder drops a carried entry that has become invalid (DEC-014). The obstacle grid is updated incrementally (DEC-095), and whether grid queries are exact is DEC-094. How work is scheduled within a tick is a design-phase topic (DEC-224).
- Options: A — keep the object model, order, grids and router, with generational handles and per-call storage; B — also keep unchecked handle look-ups and shared scratch storage of a fixed size, so that a stale id or a full list changes a result.
- Recommendation: A. The model is cheap and deterministic; the replacements remove stale-handle and shared-buffer hazards without changing behaviour.

### DEC-029 Visual language is kept
- Status: Resolved
- Affects: architecture/components/renderer/visual_language.md (RND), architecture/components/renderer/entity_and_overlay_renderers.md (RND), architecture/components/animation_and_vfx/ (VFX), reference/00_reference_overview.md (R-VIS)
- Specified behaviour: The look is kept: flat-shaded facets, additive glow on black, darkening layers, black fog, eight team colours, and small figures drawn as camera-facing cards that blend to billboards with distance. Shadows are decal blobs and flattened sprite shadows. Explosions shatter shapes into shards, and spiders walk on procedurally animated legs. Facet colour is baked per triangle, so the flat-shaded result never depends on a vertex-ordering convention. All art is authored new, and the rendering technique of each element is an ADR.
- Options: A — keep the visual language; B — a new style.
- Recommendation: A. The style is distinctive and cheap at crowd scale, and the vision keeps it unchanged (vision O9).

## 4 Rules kept as the reference states them

### DEC-030 Worker autonomy and armament
- Status: Resolved
- Affects: reference/02_units/worker.md (R-UNIT)
- Specified behaviour: Every Worker re-plans on its own every 1–2 s, staggered across Workers, while it is on the ground and not burning. It chooses among ports, Supply crates, Relics, Launch Vessels, Leaders and threats, and it fights on its own. Every Worker is armed with a laser dealing 20–39 damage at a range of about 90 wu with a 3 s reload, and with grenades.
- Options: A — keep as specified; B — redesign Worker behaviour.
- Recommendation: A. The crowd keeps doing useful local work without micro-management, and the fixed armament is the reference combat model.

### DEC-031 Leader signposts and formations
- Status: Resolved
- Affects: reference/02_units/leader_and_formations.md (R-UNIT), reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: A player promotes a Worker to Leader. A Leader set as a signpost sends Workers within 100 wu on along its waypoint, which turns Spawn Node output into a conveyor. A Leader set to formation gathers up to 90 members within 150 wu into rows of 6 to 15. While it is not walking, the Leader's facing turns towards its target facing by 0.06 per tick; that facing places the formation slots, and each member centres its threat search 0.66 × weapon range (59 wu) ahead along it, so the Leader's facing steers formation fire.
- Options: A — keep as specified; B — redesign.
- Recommendation: A. The signpost conveyor and the formation are the reference's strategic verbs; the formation is its only massed-manoeuvre tool.

### DEC-032 Selection circle and crowd-order interval
- Status: Resolved
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: Holding the primary button grows a selection circle at 100 wu/s up to 100 wu; the secondary button sends the selected Workers to the pointed destination, and with the route modifier held the route is straight. A team issues crowd orders at most once per 1.0 s.
- Options: A — keep as specified; B — redesign.
- Recommendation: A. The circle is the core verb; the order interval is part of the feel and bounds command traffic.

### DEC-033 Six Power slots, and a grant is lost when they are full
- Status: Resolved
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: A team has six Power slots, or two while it is eliminated. Every pending, placed or running power occupies one slot. A power granted while every slot is full is lost. In matches, Power-slot capacity ignores research. Where a mode has the turret-cap option, a placed turret's power keeps its Power slot while the turret exists and counts towards the cap; without the option the power ends at placement.
- Options: A — keep as specified; B — more or unlimited slots.
- Recommendation: A. Hoarding pressure and lost crates are deliberate tension.

### DEC-034 Co-op alliances
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/01_skirmish_rules_and_modes/siege.md (R-MODE), reference/05_population_flow.md (R-ECON)
- Specified behaviour: Maps that allow it support 2v2 alliances, and a Siege map can force co-op. Whether an alliance's population caps are pooled is DEC-079.
- Options: A — keep co-op; B — drop it.
- Recommendation: A. Co-op is cheap variety, provided cap pooling is specified explicitly.

### DEC-035 A leaving player is replaced by a bot
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META), reference/07_bots/ (R-BOT)
- Specified behaviour: A human who leaves during a match is replaced immediately by a bot that takes over the team. Reconnection that reclaims the slot depends on the networking model (DEC-205, DEC-244).
- Options: A — keep; B — leave the slot idle.
- Recommendation: A. Matches stay alive and no slot goes dead.

### DEC-036 Ready-up barrier, countdown and grace period
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META)
- Specified behaviour: A match starts behind a ready-up barrier: the world is held at its start state until every human is ready, then a 3 s countdown runs, and it restarts if anyone withdraws readiness. The grace period lasts 30 ticks (DEC-002). No world tick runs during the grace period; the Supply crate drop rule still runs, so a Supply crate can drop before the match runs (DEC-116).
- Options: A — keep; B — start without a barrier.
- Recommendation: A. The barrier gives every participant the same clean start.

### DEC-037 Onboarding helpers
- Status: Resolved
- Affects: reference/11_tutorial_and_help.md (R-TUT), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META)
- Specified behaviour: Contextual hints are kept as a concept, including a suspicion counter for stray clicks that triggers a hint and a hint to hold the speed modifier while panning; animated mode explainers play during ready-up. All hint text and explainer art are new, and the descriptions of what each explainer animation shows are briefs for the new art.
- Options: A — keep the concepts with new text and art; B — drop them.
- Recommendation: A. They teach the game without a campaign.

### DEC-038 Pause and the speed ramp
- Status: Resolved
- Affects: reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META)
- Specified behaviour: A match can be paused only when no remote human takes part. Game speed changes through a 2.1 s speed ramp. Pause is a sequenced command that takes effect at the next tick.
- Options: A — keep, with pause as a sequenced command; B — allow pause in every match.
- Recommendation: A. A pause that one remote participant could impose on the others is not wanted, and sequencing keeps the pause point identical for everyone.

### DEC-039 Speed crates are tick scheduling
- Status: Resolved
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM)
- Specified behaviour: Time Dilation runs the game at 0.2–0.4× and Time Rush at 1.3–2.0× for a hold of 151 world ticks (DEC-265), and the meteor shower adds a slow-motion phase. Game speed changes how many ticks run per unit of real time; the tick length never changes, so every rule sees the same tick arithmetic at every speed.
- Options: A — speed as tick-rate scheduling; B — a variable time step.
- Recommendation: A. A variable time step breaks integer tick timers (DEC-002); scheduling keeps every rule identical at every speed.

### DEC-040 Inverted ink-on-paper style
- Status: Open
- Affects: architecture/components/renderer/options_and_budgets.md (RND), architecture/components/renderer/visual_language.md (RND)
- Specified behaviour: The renderer has an inverted, ink-on-paper style preference. No user interface exposes it.
- Options: A — drop the style; B — expose it as an optional accessibility or style toggle.
- Recommendation: B, as a low-cost accessibility option, evaluated against the accessibility guidelines; drop it if testing shows no benefit.
- Decide by: design phase

### DEC-041 Development and diagnostics tooling
- Status: Resolved
- Affects: architecture/components/diagnostics_test_and_performance/ (DBG), architecture/components/platform_layer.md (PLT)
- Specified behaviour: The diagnostics are those of the diagnostics component (DBG): a network statistics window in Debug builds, with correct units; cheats as sequenced debug commands, available only in sessions flagged for cheats; per-system profile markers with GPU timestamps, and PIX markers (ADR-068); the development overlay; a headless soak test in continuous integration in which bot teams play complete matches; screenshots and a poster renderer through GPU read-back and a standard image encoder; and a process-relative high-resolution clock with injectable fake time, used for capture and replays (ADR-074). Consented telemetry takes the place of benchmark upload (DEC-060). The cheat that grants all research has no effect in a match, because match start fixes the research levels (DEC-115), so the debug-command set sets the fixed levels or omits it. Which cheats change simulation state is listed in DEC-202 item 8.
- Options: A — keep the concepts and rebuild them; B — no in-game diagnostics.
- Recommendation: A. Deterministic replay and a headless soak test are the main regression tools; the techniques are ADRs.

### DEC-042 Volumes are authored in decibels
- Status: Resolved
- Affects: architecture/components/audio_engine/ (AEN), reference/10_audio_design/ (R-AUD)
- Specified behaviour: Sound volumes are authored in decibels. The reference authoring scale of 0 to 10 at 5 dB per step is not reproduced; where a reference volume is cited, it is converted at 5 dB per step.
- Options: A — author in decibels; B — keep the 0–10 scale.
- Recommendation: A. No reference audio data is imported, so the reference scale has no value, and decibels are the audio engine's native unit.

### DEC-043 Wall destructibility
- Status: Open
- Affects: reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), reference/01_skirmish_rules_and_modes/siege.md (R-MODE)
- Specified behaviour: A wall is destroyed only by a Breach Charge or when its defending team scuttles it.
- Options: A — keep; B — let other weapons damage walls.
- Recommendation: A for the reference and for skirmish, because Siege balance rests on it; the world game decides separately.
- Decide by: design phase

### DEC-044 Commonly expected options
- Status: Open
- Affects: reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), architecture/components/ui_framework/pages_and_screen_recipes.md (UI), architecture/components/renderer/options_and_budgets.md (RND)
- Specified behaviour: The reference options have no vertical-sync, anti-aliasing, gamma, key-rebinding or UI-scale settings. Separate volumes are decided: each bus has its own slider (DEC-570).
- Options: A — add these options, together with mouse inversion and subtitles, to the Species front end; B — keep the reference option set.
- Recommendation: A. Players expect them on PC and they are cheap; rebinding is part of DEC-050.
- Decide by: design phase

## 5 Features designed by the design phase (hooks only)

### DEC-045 Map editor
- Status: Open
- Affects: reference/08_maps_and_level_design/map_editor_hooks.md (R-MAP), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/data_schemas/ (DAT)
- Specified behaviour: The reference has no reachable map editor. The library records its hooks only: what a map can express and an editor edits (terrain tiles, flatten areas, guide grids, lights, structure placement and links, routes, camera views and their animations, instant units and per-map options), the editor's entry and exit flow, and custom-map handling. Map data is a versioned, schema-validated map asset with a separately readable header. One map parser, shared by the game and the editor, reads it for the menus and for play; there is no compressed map format, and every decompression of a pack entry is bounds-checked. A load, save and load round trip changes no value and no map identity (the lossless writers of DEC-800), and loaders are fuzz-tested (DEC-015).
- Options: A — a new editor with undo, lossless round-trip, selection by stable id, link tools and in-editor playtest of the real simulation, with the per-mode map validation rules (Relic Run, Flag Chain, Siege, Launch Race) built in; B — no editor: maps are authored with external tools and validated by the content build.
- Recommendation: A, after the first playable. A new editor built on the one map parser and the lossless writers keeps every round trip exact by construction. The per-mode validation rules are needed under either option, or broken maps reach players.
- Decide by: design phase

### DEC-046 Replays
- Status: Open
- Affects: reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), architecture/components/diagnostics_test_and_performance/ (DBG)
- Specified behaviour: The reference records and plays back the command history behind a hidden preference; the record holds no seed, and playback starts after the replaying session's own match start. The library records the hooks only: command-history recording and playback, and the determinism they rely on (DEC-013).
- Options: A — a formal replay file holding build identity, content hashes, the seed, the team layout, the command stream and periodic snapshots, with fast-forward and seek through snapshots; the same machinery drives reconnect, spectating and regression testing, and every CI build replays a corpus and compares final hashes; B — no replays.
- Recommendation: A. Replays are the main regression tool and the base for spectating and reconnect.
- Decide by: design phase

### DEC-047 Achievements, statistics and leaderboards
- Status: Open
- Affects: reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: The rules evaluate 12 achievement conditions, but no achievement is ever awarded, and the leaderboards and the Skill rating page have no data source. The library records the hooks: the 12 conditions (Total Control, Stockpiler, Crowned, Clean Launch, Iron Defence, Giant Slayer, Completionist, Tourist, Massacre, Flag Sweep, Friendly Fire, Annihilator), where the rules evaluate each of them, and the statistics counters. Achievement progress is a presentation event (DEC-007).
- Options: A — platform achievements and leaderboards, starting from the 12 conditions; B — no achievements.
- Recommendation: A, with every threshold restated: where a condition's description and its evaluation give different thresholds, the redesign picks one; the Flag Sweep time test is always true, so the redesign defines it as a win within a stated number of ticks. The platform service is a deferred topic (DEC-248).
- Decide by: design phase

### DEC-048 Post-match results screen
- Status: Open
- Affects: reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META), reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: A match ends when a winner is declared; the match then freezes under a game-over overlay, and players leave only through the in-game menu. There is no results page. The library records the end-of-match sequence and the data available at match end (scores, kills and times) as hooks.
- Options: A — a post-match results screen with rematch; B — keep the overlay.
- Recommendation: A.
- Decide by: design phase

### DEC-049 Music
- Status: Open
- Affects: reference/10_audio_design/ (R-AUD), reference/00_reference_overview.md (R-VIS-042), architecture/components/audio_engine/ (AEN)
- Specified behaviour: The audio engine has a music channel and the sound data names music events, but no rule raises a music event, so no music plays at the front end, in the lobby or in a match. The match end plays the victory and defeat stings and the game-over announcement bound by DEC-097 (R-AUD-307); no other music exists. The library records these hooks only; Species music is composed new (GameAssets §16).
- Options: A — a state-driven score, newly composed, with layers for the front end, the lobby, the match introduction, low and high match intensity (driven by the units in combat near the camera's focus or by the score margin), the last minute, victory, defeat and draw; played on the streamed, two-voice, cross-fading music bus (R-AUD-414, R-AUD-415) with transitions synchronised to bars, its own player volume, and stings that duck the music; the camera-altitude ambience mix of R-AUD-323 stays alongside it; its character follows assets/art_and_audio_briefs.md §15; B — no music.
- Recommendation: A.
- Decide by: design phase

### DEC-050 Front-end features designed new
- Status: Open
- Affects: reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), reference/06_controls_camera_and_commands/ (R-CTRL), reference/11_tutorial_and_help.md (R-TUT-013), reference/01_skirmish_rules_and_modes/ (R-MODE-261, R-MODE-314, R-MODE-373, R-MODE-418, R-MODE-471), architecture/components/ui_framework/pages_and_screen_recipes.md (UI)
- Specified behaviour: Five front-end features exist only as hooks: a key-rebinding window that is unreachable; a contextual control-help panel that is computed in matches but never displayed; in-match objective tabs that are computed every frame but never drawn; a spectator team and its UI, which only the soak test uses; and a quick match that is unreachable. The library records their entry points, page slots and the data each would show. The objective cards draw nothing in a match and have no picture assets; their pictures are created only if the design phase adopts objective cards.
- Options: A — design each anew: a reachable rebinding UI that saves canonical control names; on-screen control help for gamepad players; an options control for the automatic-camera setting (R-CTRL-480); objective cards; spectating on the replay and snapshot machinery (DEC-046); matchmaking on the platform service, or none; B — drop them.
- Recommendation: A for rebinding and control help, which players expect; a design-phase choice for the others, since spectating and quick match depend on the networking and services decisions.
- Decide by: design phase

### DEC-051 Tutorials and presentation timelines
- Status: Open
- Affects: reference/11_tutorial_and_help.md (R-TUT), architecture/components/camera_and_sequences.md (CAM)
- Specified behaviour: Two tutorials run on fixed Zone Control maps, driven by a help-tab director. The library records their steps and triggers and the animated mode panels as hooks; tutorial maps, text and structure references are new content.
- Options: A — tutorials as declarative step graphs keyed by object tags, whose world-changing effects are sequenced simulation events (DEC-103), plus a small data-driven presentation timeline (captions, camera moves that land exactly on views, fades) only if needed; B — no tutorials.
- Recommendation: A.
- Decide by: design phase

## 6 Content outside the reference

### DEC-052 Single-player campaign and narrator
- Status: Dropped
- Affects: reference/00_reference_overview.md (R-VIS)
- Specified behaviour: Not specified. The campaign maps and missions, the global world map, campaign research, campaign saves, the structures and units that only the campaign places, and the Advisor narrator are outside the reference. In a match, Workers have no threat reaction to turret aiming markers; that threat type is evaluated only outside matches.
- Options: none.
- Recommendation: Dropped: no match reaches this content.

### DEC-053 Script interpreter and cut-scene scripts
- Status: Dropped
- Affects: reference/11_tutorial_and_help.md (R-TUT)
- Specified behaviour: Not specified. Species has no script interpreter and no cut-scene scripts; tutorials are step graphs (DEC-051, DEC-103), and presentation sequences are timelines of the camera and sequences component (DEC-051).
- Options: none.
- Recommendation: Dropped: no match path runs a script.

### DEC-054 Loader mini-programs, intro start sequence and attract mode
- Status: Dropped
- Affects: reference/09_front_end_and_match_flow/README.md (R-META)
- Specified behaviour: Not specified.
- Options: none.
- Recommendation: Dropped: none of these is visible to a player.

### DEC-055 Gesture input and the icon power bar
- Status: Dropped
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: Not specified. The gesture recogniser (eight symbols) and the icon power bar are outside the reference.
- Options: none.
- Recommendation: Dropped: both are unreachable; gesture input would be a new optional feature with new symbols and training data.

### DEC-056 Alternative control scheme
- Status: Dropped
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: Not specified. Only the match control scheme exists.
- Options: none.
- Recommendation: Dropped: the alternative scheme is always switched off when preferences load.

### DEC-057 Rift Horror outcome and inactive crate rewards
- Status: Dropped
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE)
- Specified behaviour: Not specified. The Rift Horror outcome, with its Rift and Rift tentacles, and the inactive duel-vehicle and formation-size rewards are outside the reference.
- Options: none.
- Recommendation: Dropped: these outcomes never occur in a match.

### DEC-058 Vehicle Duel mode
- Status: Dropped
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: Not specified. The reference has six modes; the debug-only mode and its one match option, a lives count, are not specified.
- Options: none.
- Recommendation: Dropped: a debug-only mode whose arenas are not part of the reference and whose vehicle is unreachable.

### DEC-059 Pixelation, deform and reflection effects
- Status: Dropped
- Affects: architecture/components/renderer/README.md (RND)
- Specified behaviour: Not specified. The pixelation, deform and reflection effects are not specified; the renderer is specified by its component and ADRs.
- Options: none.
- Recommendation: Dropped: none of these runs in a match.

### DEC-060 Benchmark recorder, upload and in-game video capture
- Status: Dropped
- Affects: architecture/components/diagnostics_test_and_performance/ (DBG)
- Specified behaviour: Not specified. Consented telemetry replaces benchmark upload, and external capture tools replace in-game video capture; the fake clock is kept (DEC-041).
- Options: none.
- Recommendation: Dropped: neither feature is present in a playable build.

### DEC-061 User profiles and mods
- Status: Dropped
- Affects: reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META)
- Specified behaviour: Not specified.
- Options: none.
- Recommendation: Dropped: user profiles and mods are not part of the reference product; modding is a later Species topic.

### DEC-062 Actor kinds that no match creates
- Status: Dropped
- Affects: reference/02_units/README.md (R-UNIT)
- Specified behaviour: Not specified. The Summoner, the Tri-walker, the Rift tentacle, the Vehicle Duel vehicle, and the campaign-only infantry, drop-ship and lander kinds are outside the reference.
- Options: none.
- Recommendation: Dropped: these kinds are unreachable in a match (the Summoner removes itself on its first tick); any of them returns only as a deliberate new design.

### DEC-063 Reference map shapes and map identities
- Status: Dropped
- Affects: reference/08_maps_and_level_design/README.md (R-MAP), architecture/components/terrain/ (TER)
- Specified behaviour: Not specified. The terrain heights and the map identities of the reference maps are not reproduced; every Species map is authored new. A terrain-look reference serves only as conformance tooling for the look of a new generator.
- Options: none.
- Recommendation: Dropped: the map shapes and identities are intellectual property of the reference and carry no rule.

## 7 Reference defects that Species fixes

### DEC-064 One fixed recount period
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/README.md (R-BLDG-003), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG), reference/01_skirmish_rules_and_modes/zone_control.md (R-MODE), reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE), reference/01_skirmish_rules_and_modes/relic_run.md (R-MODE)
- Specified behaviour: The recounts of Control Zones, Flag posts and Relics, which are re-armed from the match clock, run on one fixed integer period: 11 ticks for a recount written with a 1-second period, and 21 ticks for one written with a 2-second period. Timers that count down from a fixed reset, such as the 20-tick recounts of bot navigation nodes and bot objective markers, keep their own tick counts and are not covered.
- Options: A — one fixed period; B — the reference behaviour, in which the period depends on the re-arm time: at normal speed it is 10 ticks for re-arm times in [0.4, 3.0), [15.5, 63.2) and [255.4, 1023.0) s and from 4,095.5 s on, and 11 ticks otherwise (20 or 21 ticks for a 2-second period).
- Recommendation: A. The irregular cadence has no design intent; the 11-tick period matches the mode second (DEC-002).

### DEC-065 Transit Gates advance once per tick, and map values override lobby defaults
- Status: Resolved
- Affects: reference/05_population_flow.md (R-ECON), reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
- Specified behaviour: Transit Gates advance exactly once per tick on every map. Where a map sets a value, the map value overrides the lobby default; a typed override keeps 0 as a value, where the reference silently drops a per-map override whose value is 0.
- Options: A — the corrected rule; B — the reference behaviour, in which Transit Gates advance twice per tick on three Flag Chain maps, doubling their waves, and lobby values override map values; with the routine run twice per tick every countdown step takes 5.5 ticks on average: waves 330 ticks apart with a first gap of 324, and 40 Workers per gate every 33.0 s (about 72.7 per minute).
- Recommendation: A. Maps whose balance relies on doubled waves are tuned through their reinforcement values instead: on the three Flag Chain maps that set them, a wave is 100 Workers with up to 3 Transports, not the lobby's 40 and 2.
- See also: DEC-120 (how a map value of 0, an absent value and "unlimited" are written).

### DEC-066 Multi-round Siege
- Status: Open
- Affects: reference/01_skirmish_rules_and_modes/siege.md (R-MODE), reference/12_simulation_model/02_match_lifecycle_and_state_reset.md (R-SIM-029)
- Specified behaviour: Siege is a single-round match. Multi-round Siege, in which the sides rotate between rounds and the best defence time wins, is fully specified in the rules, but no match setup can select it. The round reset runs at a tick boundary (DEC-261) and leaves the object serial counter unchanged.
- Options: A — drop multi-round Siege; B — expose it in the lobby, with a round reset that also resets the object serial counter.
- Recommendation: A, the stated default, unless playtests show demand. If B is chosen, the round reset is never applied outside a tick.
- Decide by: design phase

### DEC-067 Map defaults are applied once, by the host
- Status: Resolved
- Affects: reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META), reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: When a map is selected, the host applies the map's default play time and its other defaults to the match setup once, authoritatively; no client applies them on its own.
- Options: A — the corrected rule; B — the reference behaviour, in which the map's play time and the default time limit race in the lobby, so the result depends on message order.
- Recommendation: A.

### DEC-068 The Flag Chain time-up draw uses valid team indices
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE)
- Specified behaviour: The Flag Chain time-up draw check uses only valid team indices. An entity with no team holds an empty team (no sentinel value), which is never used as a roster index.
- Options: A — validate indices; B — the reference behaviour, in which the draw check reads a roster entry for "no team", outside the roster.
- Recommendation: A. A read outside the roster is a crash defect.

### DEC-069 The co-op game-over line shows the declared result
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META)
- Specified behaviour: In a co-op match the game-over line shows the declared result, draws included.
- Options: A — the corrected rule; B — the reference behaviour, in which the line names the alliance with the higher score total, which can differ from the declared winner.
- Recommendation: A.

### DEC-070 The Spawn Control co-op winner check covers every slot
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/spawn_control.md (R-MODE)
- Specified behaviour: The Spawn Control co-op winner check iterates over every team slot.
- Options: A — the corrected rule; B — the reference behaviour, in which the check iterates only over as many slots as there are teams in the match, so a team in a higher slot is missed.
- Recommendation: A.

### DEC-071 Flag Chain base-under-attack announcement interval
- Status: Open
- Affects: reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE), reference/10_audio_design/ (R-AUD)
- Specified behaviour: The Flag Chain announcement that a team's base is under attack repeats at most about every 210 s.
- Options: A — keep about 210 s; B — a throttle of 20 s, counted in ticks.
- Recommendation: B, the stated default: 20 s is the intended interval.
- Decide by: playtest

### DEC-072 The final countdown sound plays at time-up
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/10_audio_design/ (R-AUD)
- Specified behaviour: The countdown sound for the last tick plays at time-up.
- Options: A — the corrected rule; B — the reference behaviour, in which the sound for the zero tick never plays.
- Recommendation: A.

### DEC-073 Typed match options with explicit per-mode defaults
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), architecture/components/data_schemas/ (DAT)
- Specified behaviour: Every match option is typed and has an explicit default for each mode; an option that a mode does not set takes that typed default. The Comeback Bonus is on by default in every mode where it applies: an eliminated team receives a comeback power every 450 ticks (45 s) and holds at most two. It runs in Siege and Launch Race too but never grants there, because no team counts as eliminated in those modes. In the reference it is a check box on the host's advanced-options page for Spawn Control, Zone Control, Relic Run and Flag Chain, and an absent value counts as on. The typed defaults reproduce every effect of an absent option: no time limit; sudden death off; reinforcement interval 60 mode seconds; reinforcement count and Transport cap from the map, with no Transports when the map sets none; no reinforcement turrets; turret cap 2; no crate drops; random crate placement; Catch-up multiplier 1; and no starting powers.
- Options: A — typed options with explicit defaults; B — the reference behaviour, in which an absent option reads as −1, a Boolean test treats −1 as true, and the Comeback Bonus is therefore on in every mode.
- Recommendation: A. The mechanism is corrected and the Comeback Bonus default is kept explicitly per mode, so play does not change where the bonus applies.

### DEC-074 Rank under ties
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: A team's announced rank is the number of teams with a strictly better score, plus 1.
- Options: A — the corrected rule; B — the reference behaviour, in which the rank announcement uses the team's own list index when scores tie.
- Recommendation: A.

### DEC-075 The Core Device result is fixed at detonation
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/siege.md (R-MODE), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG)
- Specified behaviour: When the Core Device detonates, the Siege result is fixed, and Command Console capture is no longer evaluated.
- Options: A — the corrected rule; B — the reference behaviour, in which Command Console capture checks continue after detonation.
- Recommendation: A.

### DEC-076 Launch Vessel Flight is invulnerable and terminal
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/launch_race.md (R-MODE), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG)
- Specified behaviour: Once a Launch Vessel enters Flight it cannot be damaged and cannot return to refuelling; Flight ends only in the launch result.
- Options: A — Flight is invulnerable and terminal; B — Flight is terminal, and damage during Flight destroys the vessel; C — the reference behaviour, in which the vessel can be damaged during Flight and can refuel in mid-air.
- Recommendation: A. A launch is the climax of Launch Race, and an invulnerable, terminal Flight makes its outcome unambiguous.

### DEC-077 The bot-mode option accepts only defined values
- Status: Resolved
- Affects: reference/07_bots/ (R-BOT), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META)
- Specified behaviour: The range check of the bot-mode option accepts only the defined values 0 to 2 and rejects the value 3.
- Options: A — the corrected check; B — the reference behaviour, in which the check accepts 3.
- Recommendation: A.

### DEC-078 No-team colour, under-attack status and the neutral Home post
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG), architecture/components/renderer/visual_language.md (RND)
- Specified behaviour: The no-team colour is an opaque grey. A structure's under-attack status is set while it is attacked. A neutral Home post never reports itself under threat.
- Options: A — the corrected rules; B — the reference behaviour, in which the no-team colour is black with alpha 1/255, the under-attack status is never set and a neutral Home post reports itself under threat.
- Recommendation: A.

### DEC-079 Per-team population cap
- Status: Resolved
- Owner decision: O14 (2026-10-17): every player slot is occupied; empty slots are filled with bots by default, and the host may close slots before the match.
- Affects: reference/05_population_flow.md (R-ECON), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/01_skirmish_rules_and_modes/siege.md (R-MODE-369), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
- Specified behaviour: Every open player slot of a map is occupied: at match start, empty open slots are filled with bots, and a participant who leaves is taken over by a bot. Before the match, the host may close slots. The per-team cap is the map's population cap divided by the number of **open** slots (the teams playing), so it never changes during a match. Alliances are not pooled. The bodies counted are listed explicitly: the team's bot controller, Transports, Leaders, Crawler eggs and fading corpses count; Crawlers do not. In Siege each defending team has the defender cap, and the attackers share (map cap − defenders × defender cap) ÷ attackers by integer division; a Siege map with no defender cap uses map cap ÷ open slots for both sides. In a Siege match whose map sets a defender cap and an unlimited population cap, a defending team is locked at the defender cap and an attacking team is decided by the Population Limiter fallback (reference/05 R-ECON-052 rule 2); no reference map has this combination. A map that sets no population cap states "no cap" explicitly (DEC-120).
- Options: A — the reference rule; B — a cap from the open slots, per team and never pooled within an alliance, with an explicit list of what counts as a body.
- Recommendation: B, the stated default. It keeps army size proportional to the teams present and makes co-op caps explicit; confirm by playtest across player counts and on co-op maps.

### DEC-080 Collector collection of any team's Essence orbs
- Status: Resolved
- Owner decision: O15 (2026-10-17): option B.
- Affects: reference/02_units/collector.md (R-UNIT), reference/05_population_flow.md (R-ECON)
- Specified behaviour: A Collector collects Essence orbs of any team and delivers them to its own team. The delivery is subject to the receiving team's population cap (DEC-079) and to the activation rules of the receiving Spawn Node: orbs that would exceed the cap are not converted. A Collector at its Spawn Node delivers an orb only on a tick on which that node may produce, that is, its activation delay is 0 and it is not population-locked (R-ECON-052); on any other tick it keeps its orbs and waits at the node, so no orb is lost and none becomes a Worker above the cap. A node that has no team still spawns nothing and the orb is still removed. Whether a waiting Collector releases its orbs instead is decided by playtest.
- Options: A — the reference rule; B — keep any-team collection and apply the population cap on delivery; C — own-team orbs only.
- Recommendation: B, the stated default. Any-team collection is a designed comeback lever, and the cap on delivery bounds it.

### DEC-081 Catch-up bonus curve
- Status: Resolved (superseded by DEC-124)
- Owner decision: O16 (2026-10-17): option B.
- Affects: reference/05_population_flow.md (R-ECON), reference/01_skirmish_rules_and_modes/ (R-MODE)
- Specified behaviour: The Catch-up bonus is a proportional curve of a team's share deficit, with the Rogue and Visitor factions excluded. DEC-124 states the curve.
- Options: A — the reference step function; B — proportional catch-up.
- Recommendation: B, by owner decision; see DEC-124.

### DEC-082 Rogue and Visitor factions never win and never block elimination
- Status: Resolved
- Affects: reference/01_skirmish_rules_and_modes/ (R-MODE: the README and all six mode files, among them R-MODE-307 of Relic Run)
- Specified behaviour: The Rogue faction (team slot 4) and the Visitor faction (team slot 5) are neutral-hostile in every mode: they never win a match and never block an elimination win. Hostile Supply crate outcomes stay owned by the Rogue faction, and the Visitor faction still arrives with the Abductor.
- Options: A — one rule for both factions in every mode; B — the reference behaviour, in which they block the last-team-standing win of Spawn Control and can win Zone Control and Flag Chain on score at time-up.
- Recommendation: A. The reference behaviour differs from mode to mode; one rule keeps both factions as hazards rather than competitors.

### DEC-083 Clickable power bar and wide power ids
- Status: Resolved
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM), architecture/components/ui_framework/pages_and_screen_recipes.md (UI)
- Specified behaviour: The power bar is clickable and shows empty slots, and it stays selectable by keys and gamepad. Power ids in commands are at least 16 bits wide. The six-slot limit is kept (DEC-033).
- Options: A — the corrected rule; B — the reference behaviour, in which the bar is selectable only by keys, tab and bracket keys and shoulder buttons, and power ids are one byte.
- Recommendation: A.

### DEC-084 Hive Essence orbs and Hive ants
- Status: Open
- Affects: reference/05_population_flow.md (R-ECON), reference/02_units/creatures.md (R-UNIT), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG)
- Specified behaviour: A destroyed Hive releases no Essence orbs, and a Hive's ants do not count against the Rogue faction's cap. No death of a Rogue faction actor leaves an Essence orb; a Hive's sortie check counts only the Rogue faction's other bodies (its bot controller, crate creatures and eggs), and a Hive releases one ant per 0.2 s.
- Options: A — keep, documented as a rule: the Rogue faction is outside the essence economy; B — Hives release orbs and their ants count against the cap.
- Recommendation: A, the stated default.
- Decide by: playtest

### DEC-085 Builder Essence-orb delivery to friendly Spawn Nodes
- Status: Resolved
- Affects: reference/02_units/builder.md (R-UNIT-447, R-UNIT-454, R-UNIT-457), reference/05_population_flow.md (R-ECON-082), reference/00_reference_overview.md (R-VIS)
- Specified behaviour: A Builder collects Essence orbs of any team in Birth or Floating, up to its capacity, and delivers them to the nearest Spawn Node, by plain distance, whose owner is a friend of its team: one orb per tick, each becoming one Worker of the node's current owner, with no population-cap or activation check (DEC-453). Carried orbs never expire; a Builder that dies drops them to Floating.
- Options: A — the delivery rule as specified; B — remove the Builder's orb collection.
- Recommendation: A. The collected orbs have a destination on every map with Spawn Nodes, and the Builder outcome is valid only on such maps, so the behaviour has a clear economic effect and needs no change.

### DEC-086 Separate demote and scrap actions for Leaders
- Status: Resolved
- Affects: reference/06_controls_camera_and_commands/ (R-CTRL), reference/02_units/leader_and_formations.md (R-UNIT)
- Specified behaviour: A player can demote a selected Leader back to a Worker and, as a separate action, scrap one of its own units. The cancel key does not kill a Leader.
- Options: A — the corrected rule; B — the reference behaviour, in which the cancel key kills a selected Leader and players cannot demote.
- Recommendation: A.

### DEC-087 Bot corrections
- Status: Resolved
- Affects: reference/07_bots/ (R-BOT)
- Specified behaviour: Bots behave as reference/07 specifies, with every correction it lists applied. The corrections include: correct ids where wrong ids are passed, including the current power id instead of a stale one; no NaN priorities; no stall of Leader promotion while the bot team has a selection; Leader powers left without a Leader expire instead of accumulating in Power slots; an initial navigation graph that accounts for Barrier Emitters, because the obstacle grid is built before the bot target links; crossings stored independently of a Barrier Emitter's state, so a toggle needs no graph rebuild; a bot turret placed at its most threatened qualifying frontier node, not the last one in structure order; turret placement around the whole base rather than in one quadrant; a power-placement centroid that divides the Worker sum by the Worker count, not by the count of all friendly actors; a Relic un-stick centroid that no longer divides by the count of all collisions; a random walk onto a sea node that no longer leaves the army in place; a recall rate that does not scale with array capacity; a filled Siege bot Transport objective list, so that the mobile-bomb branch of the Siege Transport behaviour runs; defending formations turned towards the threat; a Siege defence objective that does not fall back at once (map validation binds its marker, DEC-561); and a bot controller placed inside the map rather than at its corner. The position of the bot controller and the expiry of Leader powers are DEC-862, and the unbounded bot loops are bounded (DEC-014, DEC-857).
- Options: A — apply every correction; B — keep the reference behaviour for parity.
- Recommendation: A. Bot strength is then calibrated by playtest in every mode, Siege included (DEC-112).

### DEC-088 Structure corrections
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/ (R-BLDG: every file of the folder)
- Specified behaviour: Structures behave as reference/03 specifies, with every correction it lists applied. The corrections include: port occupancy bound to the (structure, port) pair; Barrier Emitter collision geometry that matches its visible geometry, so the obstacle-grid band that marks each Barrier Emitter line one cell wider on every side never blocks movement beyond the visible line and serves at most to select collision candidates; per-tick rates that scale with game speed (DEC-092); no team-change sounds from map template structures at load; turret line of sight sampled from the turret towards the target; Worker grenades against structures thrown for the Worker's own team (DEC-105); and tree hit volumes taken from asset data (DEC-012). The smoke-column obstacle is DEC-089.
- Options: A — apply every correction; B — keep the reference behaviour for parity.
- Recommendation: A.

### DEC-089 Smoke-column obstacle
- Status: Open
- Affects: reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
- Specified behaviour: A smoke-column marker is an invisible obstacle 13 wu in size that blocks movement.
- Options: A — the reference behaviour; B — markers do not collide unless a map explicitly marks the obstacle as wanted.
- Recommendation: B, the stated default.
- Decide by: design phase

### DEC-090 Floating-point actor stats and the Abductor speed
- Status: Resolved
- Affects: reference/02_units/README.md (R-UNIT), reference/02_units/hazards_and_projectiles.md (R-UNIT), architecture/components/data_schemas/ (DAT)
- Specified behaviour: Actor stats are floating-point values, and speed multipliers (×2, ×1.5, ×0.75, ÷2) apply exactly, without truncation. The Abductor's speed is an explicit tuning value whose default is its effective reference speed of 244.
- Options: A — floating-point stats and an explicit Abductor speed; B — the reference behaviour, in which stats are 8-bit values, so boosted speeds truncate and the Abductor's listed speed of 500 wraps to 244.
- Recommendation: A. Boosted units become fractionally faster than in the reference, and the Abductor keeps its effective speed until a playtest re-tunes it.

### DEC-091 Dormant Crawler egg lifetime
- Status: Open
- Affects: reference/02_units/creatures.md (R-UNIT)
- Specified behaviour: A dormant Crawler egg never decays; its 120 s lifetime limit has no effect.
- Options: A — the reference behaviour; B — apply the 120 s lifetime, counted in ticks.
- Recommendation: B, the stated default.
- Decide by: playtest

### DEC-092 Per-tick rules scale with game speed
- Status: Resolved
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/02_units/creatures.md (R-UNIT), reference/03_buildings_and_world_objects/README.md (R-BLDG), reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM)
- Specified behaviour: Every per-frame or per-tick probability, growth and rate is a per-second rate scheduled in ticks, so the speed crates change it like every other rule. This covers the lightning roll of 1 % per tick, the Blight Grove growth of 2 % per tick, the Supply crate capture cap of 0.05 per tick, the Solar Array energy surge, the gravity of Seed Pods and spiders, the empty-Hive counter, the volcano vent eruption, tree drops, the Spitter Plant shrink, Collector delivery (one Essence orb per tick), Spawn Surge requests (one per tick), the Launch Vessel passenger release, the Relay overshoot and the game-speed hold counters. Under DEC-039 every one of them scales the same way.
- Options: A — per-second rates scheduled in ticks; B — the reference behaviour, in which these rules ignore game speed.
- Recommendation: A.

### DEC-093 Explosion damage falloff
- Status: Resolved
- Affects: reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md (R-CRATE), reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM)
- Specified behaviour: The damage factor at distance d from an explosion of range r is ((2r − d) ÷ r) × 2. This is the tuned curve and is documented as the rule.
- Options: A — keep the curve; B — the apparent intent, (2r − d) ÷ 2r.
- Recommendation: A, unless explosions are re-tuned as a whole: the curve is stronger than the apparent intent, and all explosive balance rests on it.

### DEC-094 Exact or approximate actor-grid queries
- Status: Open
- Affects: reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), reference/02_units/worker.md (R-UNIT), reference/07_bots/ (R-BOT), reference/01_skirmish_rules_and_modes/zone_control.md (R-MODE)
- Specified behaviour: Neighbour queries go through an actor grid cache whose results can be up to about 1.1 s old; a query with half the range of a cached query reuses the cached result; presence checks use cell capacity rather than exact membership. Port occupancy is exact: a strict dx² + dz² < 25 test (radius 5 wu), where the occupant is the first operating Worker in grid result order (stable key in DEC-016). Count queries count whole cells of the square that bounds the range, so they over-count by up to one cell beyond it; a cell that has ever held an actor answers a presence check as occupied for the rest of the match; and a cache hit returns the cached list unfiltered, including actors outside the requested range.
- Options: A — the reference cached and approximate queries, specified as explicit rules; B — exact queries, or an explicit per-tick memo with identical results.
- Recommendation: B, the stated default. Run an A/B playtest of the same scenarios with the cache on and off, because exact queries change targeting, grenade decisions, zone contests and bot under-attack tests; if the softness is wanted, specify it as an explicit rule.
- Decide by: playtest

### DEC-095 Obstacle grid and route upkeep; route previews kept separate
- Status: Resolved
- Affects: reference/12_simulation_model/05_space_routing_and_physics.md (R-SIM), reference/06_controls_camera_and_commands/ (R-CTRL)
- Specified behaviour: The obstacle grid is updated incrementally, and its outside cell is cleared like every other cell. When a structure is built or destroyed, the affected route-graph edges are rebuilt. Route previews that the UI draws are computed in a separate store and never take entries from the simulation's route storage.
- Options: A — the corrected rules; B — the reference behaviour, in which the outside cell is never cleared, route tables are never rebuilt after load, and preview routes share the simulation's free list.
- Recommendation: A. Shared storage lets presentation change simulation results (DEC-007).

### DEC-096 Input corrections
- Status: Resolved
- Affects: architecture/components/input/ (INP), reference/06_controls_camera_and_commands/ (R-CTRL), architecture/components/camera_and_sequences.md (CAM)
- Specified behaviour: Input behaves as reference/06 specifies, with every correction it lists applied. The corrections include: a press shorter than a frame is never lost; an unknown condition word in a binding definition is rejected at load, not read as a held key; the camera is clamped horizontally; the Strike team camera distance is a floating-point value; and one click produces one selection command (DEC-119). The input corrections that this entry does not name are DEC-554. The composable binding graph of drivers, combination operators and aliases is kept.
- Options: A — apply every correction; B — keep the reference behaviour for parity.
- Recommendation: A.

### DEC-097 Audio corrections
- Status: Resolved
- Affects: reference/10_audio_design/ (R-AUD), architecture/components/audio_engine/ (AEN)
- Specified behaviour: Audio behaves as reference/10 specifies, with all 24 corrections it lists applied. The corrections include: the Supply crate decay tone names a real sound; the Core Device loop stops; the victory and defeat stings are bound to the match-end events (R-AUD-307, DEC-049); crowd sounds are audible according to an authored priority class rather than an accidental cull radius (DEC-109); voice stealing compares priorities; a change of ownership to no team plays only the lost sting, not both the captured and the lost sting; and a missing sample is a diagnostic, not a fatal assertion (DEC-015). The behaviour of attached sounds when their emitter is removed and the start value of smoothed parameters are open questions (DEC-321, DEC-322), and loop keep-alive is DEC-323.
- Options: A — apply every correction; B — keep the reference behaviour for parity.
- Recommendation: A.

### DEC-098 Texture cache keyed by asset and variant
- Status: Resolved
- Affects: architecture/components/asset_pipeline_and_resources.md (AST), architecture/components/renderer/README.md (RND)
- Specified behaviour: Textures are cached by the pair (asset, variant), where the variant names the mip and mask processing, and an import manifest declares each variant. Two requests for different processing of one image get two cache entries. Species asset ids are lower case, so one image has one name and one entry per variant.
- Options: A — the corrected cache; B — the reference behaviour, in which the cache is keyed by name, so the first request fixes the processing for every later one: the colour marker image can show as a white alpha mask, and the laser and cloud textures drawn without mipmaps can sample an incomplete texture after the cache is flushed.
- Recommendation: A.

### DEC-099 Localisation fallback and language completeness
- Status: Resolved
- Affects: reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), architecture/components/ui_framework/layout_rendering_input_and_localisation.md (UI)
- Specified behaviour: A string that a language lacks falls back, per key, to the base language (R-META-464). A language is offered only when its table is complete (R-META-463); the content build reports the key coverage of every language (ADR-063), and the development language test lists every missing id (R-META-473). The start-up language falls back from the language preference to the operating-system locale's language, then to the base language, then to the first language file present.
- Options: A — the corrected rule; B — the reference behaviour, in which a missing string shows its key and a language is offered whatever its coverage.
- Recommendation: A.

### DEC-100 One defaults table for preferences
- Status: Resolved
- Affects: architecture/components/platform_layer.md (PLT), architecture/components/data_schemas/ (DAT)
- Specified behaviour: Preference defaults come from one table, in code or in a schema, that is available before any pack file is mounted, so installed and development builds use the same defaults (ADR-061). Each setting has one default, which every reader of the setting, the options pages included, uses; the settings are those of the settings table of the data schemas (DAT-193), each with a Species effect: among them the render size, the window mode, vertical sync, the bus volumes, the language and the bindings. Colour depth, refresh rate, shader switches and the audio device's buffers, mixing rate and voice count are not settings (DEC-869). The network keys that nothing reads are listed in DEC-250.
- Options: A — one defaults table; B — the reference behaviour, in which the default-preferences file is read before the main pack is mounted, so packaged installs fall back to code defaults, and the readers of one setting can disagree on its default.
- Recommendation: A.

### DEC-101 Seasonal theme
- Status: Open
- Affects: architecture/components/animation_and_vfx/ (VFX), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM)
- Specified behaviour: The reference has a seasonal theme (seasonal hats on Workers and ice ramps) that each client switches on from its own date check, from 24 to 26 December local time, and whose flag is relayed through the command stream into the simulation. Whether the theme is active is fixed once per match and identical for every participant (DEC-118); this entry decides whether Species keeps a seasonal theme and who chooses it. Until it is decided, the host's match setup applies the reference date rule once, from the host's local date.
- Options: A — the host decides the theme in the match setup, and it is sequenced like any other match option; B — drop the seasonal theme.
- Recommendation: A, the stated default, with new seasonal art; under either option the seasonal state follows DEC-118.
- Decide by: design phase

### DEC-102 UI framework corrections by construction
- Status: Resolved
- Affects: architecture/components/ui_framework/ (UI: README.md, core_and_widgets.md, layout_rendering_input_and_localisation.md, pages_and_screen_recipes.md)
- Specified behaviour: The Species UI framework avoids by construction every defect of the window core and of the framework that the UI framework component lists, each held by a test of its defect suite DT-01 to DT-33 (UI-014, UI-490). The window-core corrections include: a press and its release act on the same widget, with pointer capture; a held button repeats on explicit timers (ADR-032); a relayout keeps view-model state (ADR-033); and the maximise path never accesses a missing object, a crash defect. The reference framework supports tooltips, but nothing registers one, so no tooltip appears; Species specifies none, and a tooltip feature is front-end design for the design phase. The framework corrections include: Escape cancels and never commits; gamepad Back is handled as input, not polled during rendering; a percent sign is an ordinary character of message text and text fields; a colour or side row has one availability rule for drawing and pressing; no fixed-size array limits a list; a disabled widget never acts, so a host button stays inactive while hosting is not permitted (DEC-243); and list rows are removed by identity, so shrinking a list never removes the wrong row, such as the Back entry.
- Options: A — a new framework that avoids every listed defect; B — reproduce the reference framework.
- Recommendation: A.

### DEC-103 Tutorials are step graphs with simulation-sequenced effects
- Status: Resolved
- Affects: reference/11_tutorial_and_help.md (R-TUT), reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM)
- Specified behaviour: Tutorial steps form a graph keyed by object tags, never by fixed team or structure ids. Tutorial state resets at every run. Every world-changing tutorial effect, such as inserting a Supply crate or changing an option, is a sequenced simulation event at a tick, never an action of the render loop or of a wall-clock timer. The director is evaluated once per tick of match time on the tutorial player's participant, issues each world change as a tutorial event, and tests the tutorial player's team, never team 0 (R-TUT-061, R-TUT-062, R-ECON-052).
- Options: A — the corrected design; B — the reference behaviour, in which the second tutorial checks ownership against team 0, tutorial state persists between runs, and the tutorial director changes the world from the render loop on wall-clock timers.
- Recommendation: A.

### DEC-104 Formation volley rhythm in integer arithmetic
- Status: Resolved
- Affects: reference/02_units/leader_and_formations.md (R-UNIT)
- Specified behaviour: A formation member's volley phase is (tick count + member index) modulo (formation size ÷ 4), with the firing condition of reference/02. The phase is evaluated exactly in integer arithmetic, for example as (4 × (tick count + member index)) modulo formation size, so formations whose size is not a multiple of 4 keep the reference rhythm.
- Options: A — integer arithmetic with the reference rhythm; B — the reference single-precision floating-point evaluation.
- Recommendation: A.

### DEC-105 Worker grenade team and the Hive and Spitter Plant attack target
- Status: Resolved
- Affects: reference/02_units/worker.md (R-UNIT), reference/02_units/creatures.md (R-UNIT), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG)
- Specified behaviour: Grenades that Workers throw at structures belong to the thrower's team. Attacks by Hive ants and Spitter Plants on a structure pass the id of that structure.
- Options: A — the corrected rules; B — the reference behaviour, in which these grenades are thrown as team 1 and these attacks pass an obstacle index in place of a structure id.
- Recommendation: A.

## 8 Design-critical uncertainties and values to confirm by measurement

### DEC-106 Performance envelope at the population cap
- Status: Open
- Affects: reference/12_simulation_model/README.md (R-SIM), reference/05_population_flow.md (R-ECON), architecture/components/diagnostics_test_and_performance/ (DBG), architecture/components/renderer/options_and_budgets.md (RND), architecture/components/animation_and_vfx/ (VFX)
- Specified behaviour: The reference population caps allow 1,400 to 2,000 simulated bodies per map, plus Essence orbs and projectiles. No frame time, draw-call count, particle count or per-system cost is known for this population, and neither is the population at which the frame rate falls below 30 or 15 frames per second. The Worker task search is the largest expected simulation cost.
- Options: A — keep the reference caps and effect counts; B — lower them to fit a measured budget.
- Recommendation: A provisionally. Measure the first prototype with a synthetic 2,000-unit match on the Species renderer, set per-system budgets, and lower caps only if a budget is missed. The world tick rate and per-tick budget are deferred topics (DEC-227, DEC-228).
- Decide by: prototype

### DEC-107 Hostile Supply crate outcomes: ownership and filtering
- Status: Resolved
- Owner decision: O18 (2026-10-17): hazards stay team-blind, every outcome respects the map's exclusion list, and Brood ownership is decided in playtest (DEC-121).
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/02_units/hazards_and_projectiles.md (R-UNIT)
- Specified behaviour: The meteor shower, Sickness and Missile Barrage outcomes affect every team (team-blind). Every Supply crate outcome, hostile ones included, respects the map's list of excluded outcomes, and so does the Comeback Bonus draw (DEC-452). The ownership of Brood creatures is decided separately (DEC-121). The step that sets up the Rogue faction for the opening team has no effect and is not specified.
- Options: A — keep each behaviour as specified; B — decide per outcome: hostile outcomes respect the map's exclusion list, Brood creatures belong to the Rogue faction, and each outcome's team filter is explicit.
- Recommendation: A for the hazards that ignore teams, since they are the risk of the 10 % hostile pool (DEC-021); B for the exclusion list, so that map designers control every outcome. Decide Brood ownership in playtest.

### DEC-108 Launch Vessel survival under attack
- Status: Open
- Affects: reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG), reference/01_skirmish_rules_and_modes/launch_race.md (R-MODE)
- Specified behaviour: Grenades thrown at a Launch Vessel aim 50 wu above its base; damage is measured to the base and scaled by 0.2; the vessel's damage decays at 0.33 per second. How long a besieged vessel survives is derived from these rules, not specified as a value.
- Options: A — keep the rules as specified and tune them by playtest; B — specify vessel survival directly.
- Recommendation: A. Simulate the race between attack and refuelling offline from the specified ballistics first, then confirm by playtest.
- Decide by: playtest

### DEC-109 Crowd-sound audibility radius
- Status: Open
- Affects: reference/10_audio_design/ (R-AUD), architecture/components/audio_engine/ (AEN)
- Specified behaviour: Crowd sounds that play only from their nearest emitter (Worker creation, Worker attack and the Launch Vessel's distress sound) are audible only when an emitter is within about 316 wu of the camera, which can silence them at strategic camera heights. Their audibility is set by an authored priority class (DEC-097), and the default radius of that class is the reference value of about 316 wu.
- Options: A — keep about 316 wu; B — a larger authored radius, or a rule that depends on camera altitude.
- Recommendation: B, after a playtest at several camera heights establishes what players should hear from a 1,000-unit battle.
- Decide by: playtest

### DEC-110 Second tutorial start condition
- Status: Open
- Affects: reference/11_tutorial_and_help.md (R-TUT)
- Specified behaviour: The lessons of the second tutorial begin only when idle player Workers wander within port range of its Command Console, 224.6 wu from a designated player Spawn Node, and the bot captures a designated Spawn Node first, so the lessons may not appear in order.
- Options: A — reproduce the condition; B — the tutorial step graph sets up these states explicitly (DEC-103).
- Recommendation: B.
- Decide by: design phase

### DEC-111 Values derived from flat-terrain calculation
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md (R-CRATE), reference/02_units/README.md (R-UNIT), reference/02_units/hazards_and_projectiles.md (R-UNIT), reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG)
- Specified behaviour: These values follow from the rules on flat terrain and are confirmed by measurement in the prototype: weapon ranges, the timing of airstrikes and of the Missile Barrage, and Launch Vessel survival (DEC-108). The Collector throughput of about 2.5 Workers per second for a 500 wu round trip is such a value, from a movement law with turn rate 9.5 × dt, velocity filter 0.92 / 0.08 and arrival radius 5 wu; the rocket's outcome of 8 or 9 moves also depends on its effect slot (DEC-455). The formulas are the specification.
- Options: A — adopt the formulas and tune by playtest; B — derive new values before use.
- Recommendation: A, validating the most important values in the first prototype.
- Decide by: playtest

### DEC-112 Bot strength calibration
- Status: Open
- Affects: reference/07_bots/ (R-BOT)
- Specified behaviour: Bot difficulty is set by decision period and randomness (DEC-022). At every difficulty, reference bot strength is shaped by behaviours that DEC-087 corrects: a stall in Leader promotion that fills Power slots with orphaned Leader powers, so the bot loses Supply crate rewards; a stale power id; the Siege defence fall-back; and NaN priorities. Two corrections change Siege and bot defence most: the mobile-bomb branch of the Siege bot Transport behaviour runs only after its correction, and defending formations turn towards the threat, with a threat search centred about 59 wu ahead along the facing and a radius of 90 wu, so defenders miss enemies more than about 31 wu behind them.
- Options: A — keep the reference difficulty parameters after the corrections; B — re-calibrate the parameters by playtest, per mode.
- Recommendation: B. Apply the corrections first, then calibrate each difficulty by playtest rather than by parity, in every mode; Siege attack and Siege defence are calibrated explicitly, because the corrections above change them most.
- Decide by: playtest

### DEC-113 Abductor wander-target area
- Status: Open
- Affects: reference/02_units/hazards_and_projectiles.md (R-UNIT)
- Specified behaviour: An Abductor accepts wander targets only in the quarter of the map where x is in [0, X/2) and z is in [0, Z/2), with X and Z the map size, because its target offsets cover [−r/2, r/2] and every negative coordinate reads as a sea cell.
- Options: A — the reference behaviour; B — sample targets over the whole land area with a bounded candidate list (DEC-014).
- Recommendation: B. The restriction has no visible purpose and concentrates Abductors in one corner of the map.
- Decide by: playtest

### DEC-114 Position of score-weighted crate drops
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE)
- Specified behaviour: A score-weighted Supply crate drop takes its first candidate point whatever the terrain there, so it can land in water, or up to 100 wu outside the map when the structure is near an edge (R-CRATE-106 item 7).
- Options: A — the reference behaviour; B — validate the candidate against land of at least `ref.crate.drop.min_land_height` inside the map, for up to `ref.crate.drop.weighted_attempts` (1000) candidates, with a deterministic fallback (DEC-014).
- Recommendation: B.
- Decide by: playtest

### DEC-115 Fixed match research levels
- Status: Open
- Affects: reference/02_units/README.md (R-UNIT), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META)
- Specified behaviour: Match start sets the research levels; the match setup carries no research settings (R-META-069, R-MODE-082). Every research level is set to 3, then Strike team to 1, laser, grenade, rocket and airstrike to 2, and Worker to 4. At laser level 2 a Convert Turret's laser lasts 1.2 s.
- Options: A — keep the fixed levels, with no research settings in the match setup; B — add research settings to the match setup and let them apply.
- Recommendation: A. The fixed levels are the tuned combat model (DEC-030).
- Decide by: design phase

### DEC-116 Supply crates during the grace period
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM), reference/12_simulation_model/02_match_lifecycle_and_state_reset.md (R-SIM-012), reference/12_simulation_model/04_containers_and_update_order.md (R-SIM-218), reference/00_reference_overview.md (R-VIS-052)
- Specified behaviour: The Supply crate drop rule has no grace-period gate. While the world is held for the start of a match it still runs, once per command frame, so its countdown runs while the teams ready up and during the grace countdown, and a Supply crate can drop before the grace period is over; such a crate stays at its start height until the first world tick. A pause stops the drop rule (R-CRATE-102, R-MODE-044, R-VIS-052, R-SIM-550).
- Options: A — the reference: the drop rule runs during the start hold; B — a grace-period gate: the drop rule runs only on world ticks, so no Supply crate exists when the match starts.
- Recommendation: B. Every other per-tick rule waits for the first world tick (DEC-039, DEC-092), and under A a long ready-up lets crates accumulate up to the map's cap before anyone can move.
- Decide by: playtest

### DEC-117 Reference rules: Strike team grenades, silent Solar Array changes, structure advance and team unit lists
- Status: Resolved
- Affects: reference/02_units/strike_team.md (R-UNIT), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/05_population_flow.md (R-ECON), reference/12_simulation_model/04_containers_and_update_order.md (R-SIM), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM)
- Specified behaviour: These behaviours are specified as rules: a Strike team always throws grenades in a match, because weapon cycling is not available in matches and no map sets another weapon; an empty Solar Array's per-tick team change is silent, because a team-change sound is bound only for Control Zones and Relics, and with one no-team value (DEC-464) the change itself disappears, because it comes from comparing two different no-team values; every structure advances exactly once per tick; a team's list of ungrouped actors holds every actor spawned without an actor group, its bot controller included, while Strike teams, Bombers, centipedes and Hive ants live inside actor groups; and a team's list of special units never holds a removed actor, because each special unit registers when it starts and unregisters when it is removed.
- Options: none.
- Recommendation: Resolved: the owning reference document states each of them as a rule.

## 9 Further integrity rules

### DEC-118 The seasonal state is decided once per match
- Status: Resolved
- Affects: reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM), reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META), architecture/components/animation_and_vfx/ (VFX)
- Specified behaviour: Whether the seasonal theme is active is match state. It is decided once, in the match setup before match start, recorded with the setup and identical for every participant; no client decides it from its own date or clock, and nothing changes it during the match. Seasonal effects such as snow draw from the presentation generator (DEC-013). Whether Species has a seasonal theme, and who chooses it, is DEC-101.
- Options: A — one seasonal state per match, fixed in the match setup and identical for all participants; B — the reference behaviour, in which each client switches the theme on from its own date check and relays the flag through the command stream into the simulation.
- Recommendation: A. A flag that each participant derives from its own clock makes a match depend on local time and local state, which DEC-009 and DEC-010 exclude; a state fixed in the match setup is part of the recorded match and replays identically.

### DEC-119 Simulation code never issues player commands
- Status: Resolved
- Affects: reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM), reference/06_controls_camera_and_commands/ (R-CTRL), architecture/components/input/ (INP)
- Specified behaviour: Player commands come only from a player's input or from an authorised command source, such as a bot or a deputy, and every command passes the same validation path (DEC-202). Executing a command or advancing the simulation never issues a further player command as a side effect: a rule that needs a follow-up change applies it inside the same simulation step. One Leader toggle therefore yields one command and one selection change on every participant.
- Options: A — the simulation never issues player commands, and follow-up changes happen inside the step; B — the reference behaviour, in which executing one Leader toggle command makes every participant issue a further selection command, so one click becomes as many identical commands as there are participants.
- Recommendation: A. A command that the simulation issues is input that no player gave: it multiplies with the participant count and is harmless only while the sequencer keeps one selection per sender and tick. Keeping the simulation a function of the commands it receives is the rule that DEC-008 sets for player intent.
