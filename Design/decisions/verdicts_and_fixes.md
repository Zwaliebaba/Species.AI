# Decision register: verdicts and fixes

> Status: Reference decisions · Range: DEC-001 to DEC-063 · Related: [decisions index](README.md), [conventions](../CONVENTIONS.md)

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

Sections 7 to 9 (DEC-064 to DEC-119) are in [verdicts_and_fixes_3.md](verdicts_and_fixes_3.md).

## 1 Headline verdicts

### DEC-001 Reference core loop: crowd-and-territory economy
- Status: Resolved
- Affects: reference/00_reference_overview.md (R-VIS), reference/05_population_flow.md (R-ECON), reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: The reference game has no build queue and no currency. A team's army is the population that its held Spawn Nodes produce: a Spawn Node is captured by a single team with at least 3 operators on its 5 ports and no enemy on any port, activates 300 ticks after an ownership change, and produces in bursts and pauses up to a per-team population cap. Workers fight and re-plan on their own; Leaders act as signposts and formation leaders; Supply crates open to outcomes that are 90 % beneficial and 10 % hostile and feed six Power slots; six modes share one rules skeleton; bots play on a node graph with perfect information and no other advantage. The reference library specifies this loop in full and keeps it as the rule set of the skirmish mode.
- Options: A — the world game keeps the reference loop unchanged; B — the world game adds settlement building and a resource economy on top of essence-driven growth, while skirmish keeps the reference loop; C — the world game replaces the loop entirely.
- Recommendation: B. The vision brief overrides "no build queue and no currency" for the persistent world, where settlements gather resources, build and grow, but keeps a skirmish mode that is the combat balance test bed (vision O8). Keeping the reference loop authoritative for skirmish preserves the tuned combat and population behaviour; the design phase decides how settlement growth uses Essence orbs and whether a resource layer exists.
- Resolution (2026-10-18, design phase, area A0): option B. Skirmish keeps the reference core loop as this entry specifies it, with no build queue and no currency (PIL-002, PIL-003). The world game adds settlements that gather resources, store them, build and maintain structures and grow, on top of essence-driven growth in which deaths yield Essence orbs and Essence becomes new Workers, so a resource layer exists (PIL-004, PIL-050 to PIL-058); B1 specifies growth through Essence, B2 the resources and B3 construction. The verdicts are in [the adoption ledger of reference/00](../gdd/adoption_ledger/00_reference_overview_ledger.md); the R-ECON and R-MODE verdicts follow in the ledgers of B1 and B9. Specified in [Vision, pillars and core loops](../gdd/00_vision_pillars_and_core_loops.md) §2.1.

### DEC-002 Integer tick timers and the 11-tick mode second
- Status: Resolved
- Affects: reference/12_simulation_model/01_tick_timing_and_main_loop.md (R-SIM), reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/05_population_flow.md (R-ECON), reference/03_buildings_and_world_objects/README.md (R-BLDG), reference/04_supply_crates_powers_and_weapons/README.md (R-CRATE), reference/02_units/README.md (R-UNIT)
- Specified behaviour: Every rule timer, cooldown, lifetime and probability in the simulation is an integer number of ticks, or a per-second rate converted to ticks when the data loads; no simulation timer is a floating-point countdown. The **mode second** is an explicit named constant of 11 ticks (1.1 s at the reference tick rate of 10 ticks per second). The match clock, elimination countdowns, Relic timers, reinforcement waves and rank checks advance once per mode second. The reference tick counts, with each nominal duration, are: mode second (1 s) 11; grace period (3.0 s) 30; victory timers (10, 15 and 5 s) 101, 151 and 51; Comeback Bonus interval (45 s) 450; production pause (10.0 s) 101; burst request interval (0.2 s) 2; rocket turret reload (1.5 s) 15; Convert Turret reload (0.5 s) 6; mortar turret reload (4.0 s) 40; Spawn Node activation (30.0 s) 300; Essence orb birth 61; Relay transit at most 10h + 1 for h hops (11 for the first hop, 10 for each further hop; the same-tick rule that sets the exact count is DEC-455). Further reference tick counts: burst pause 101–150 (mean 125.5); first production timer 21–40; first ownership evaluation 1–20, later evaluations 21–30; reinforcement wave every 660, with a first gap of 649; Spawn Control elimination 330; first Comeback Bonus grant 51, for a team already eliminated on the first tick after the grace period; Spawn Surge 51 (50 or 51 requests, DEC-304); Essence orb floating 301–500 and death ascent 901; Collector and Builder idle retarget 11; bot Collector decision 11; Worker idle retarget 11–20; corpse fade 25 on the ground and 50 in the air; Population Limiter recount 91–110; Hive orb-to-ant conversion 10; Crawler egg hatch 151; Collector delivery one Essence orb per tick. The three mode-second timers start at 0, so the first mode second ends on the first tick after the grace period. A rule that compares match time with a deadline of T seconds counts T ÷ `ref.sim.tick_period` ticks (reference/03 R-BLDG-403), so the Hive orb-to-ants conversion and the Population Limiter overpopulation period are each 10 ticks, not an alternation of 10 and 11 (reference/05 R-ECON-064, R-ECON-084). Seconds appear only as a derived display value next to the tick count.
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
- Affects: architecture/components/data_schemas/ (DAT), architecture/components/asset_pipeline_and_resources.md (AST), architecture/components/build_tooling_and_distribution/ (BLD), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
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
- Status: Resolved
- Affects: reference/08_maps_and_level_design/map_editor_hooks.md (R-MAP), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/data_schemas/ (DAT)
- Specified behaviour: The reference has no reachable map editor. The library records its hooks only: what a map can express and an editor edits (terrain tiles, flatten areas, guide grids, lights, structure placement and links, routes, camera views and their animations, instant units and per-map options), the editor's entry and exit flow, and custom-map handling. Map data is a versioned, schema-validated map asset with a separately readable header. One map parser, shared by the game and the editor, reads it for the menus and for play; there is no compressed map format, and every decompression of a pack entry is bounds-checked. A load, save and load round trip changes no value and no map identity (the lossless writers of DEC-800), and loaders are fuzz-tested (DEC-015).
- Options: A — a new editor with undo, lossless round-trip, selection by stable id, link tools and in-editor playtest of the real simulation, with the per-mode map validation rules (Relic Run, Flag Chain, Siege, Launch Race) built in; B — no editor: maps are authored with external tools and validated by the content build.
- Recommendation: A, after the first playable. A new editor built on the one map parser and the lossless writers keeps every round trip exact by construction. The per-mode validation rules are needed under either option, or broken maps reach players.
- Resolution (2026-10-18, design phase, area A1): B now, A later. No map editor in player builds in v1: text assets, content-build validation, a Debug-build preview (WLD-260, WLD-261). After the skirmish first playable, a Debug-build editor with undo, stable ids, lossless writers, validation on save and a play test (WLD-262). A player editor waits for modding. Specified in `gdd/01_world_and_regions/skirmish_maps_and_map_tools.md`.

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
