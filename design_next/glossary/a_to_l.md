# Glossary A–L

> Status: Reference · Glossary · Related: [Glossary index](README.md), [Glossary M–Z](m_to_z.md), [Conventions](../CONVENTIONS.md)

This file defines the Species terms that start with a digit or with the letters A to L. [README.md](README.md) explains how to read an entry; Species identifiers and string-id prefixes for each term are in [CONVENTIONS.md §8](../CONVENTIONS.md#8-naming).

Jump to: [**Numbers**](#numbers) · [**A**](#a) · [**B**](#b) · [**C**](#c) · [**D**](#d) · [**E**](#e) · [**F**](#f) · [**G**](#g) · [**H**](#h) · [**I**](#i) · [**J**](#j) · [**K**](#k) · [**L**](#l) · [**M–Z**](m_to_z.md)

## Numbers

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| 2-D grid | engine term | A two-dimensional array laid over the world's horizontal plane, holding one value per cell; height fields and spatial grids are built on it. | [Terrain](../architecture/components/terrain.md); [Space, routing and physics](../reference/12_simulation_model/05_space_routing_and_physics.md) |
| 3-vector | engine term | A vector of three world coordinates (x, y, z), with y vertical; sea level is y = 0. | [Entity model and identity](../reference/12_simulation_model/03_entity_model_and_identity.md) |
| 3x4 matrix | engine term | An affine transform made of three axis vectors and a translation, used for object and mesh-node transforms. | [Entity model and identity](../reference/12_simulation_model/03_entity_model_and_identity.md) |

## A

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| Abductor | player concept | The hovering craft summoned by the Abductor outcome: for 600–1200 ticks it abducts Workers, then drops as many Workers of the Visitor faction as it abducted, at most 100. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| Abductor (outcome) | player concept | A bad-pool Supply crate outcome, drawn at most once per match and never in Siege, that summons an Abductor. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| achievement | player concept | One of 12 named goals that the match rules evaluate at fixed points: Annihilator, Clean Launch, Completionist, Crowned, Flag Sweep, Friendly Fire, Giant Slayer, Iron Defence, Massacre, Stockpiler, Total Control and Tourist. The reference evaluates the conditions but awards nothing; the library records them as hooks (see DEC-047). | [Options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| achievement events | engine term | The hook through which the match rules report achievement conditions and progress: maps visited, maps won, the running kill total and the Giant Slayer holder flag. Achievement progress is a presentation event and awards nothing (see DEC-047). | [Options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| action bindings | engine term | The table that binds each of the 164 named input actions (for example firing a unit's weapon at a target, or toggling ready) to an input expression over keyboard, mouse and gamepad inputs, and to the icon that shows the binding. | [Input](../architecture/components/input.md); [Data schemas](../architecture/components/data_schemas.md) |
| activation delay | player concept | The 300 ticks after any ownership change before a Spawn Node produces. It is skipped for a team's only Spawn Node and on maps whose options make Spawn Node capture instant. | [Population flow](../reference/05_population_flow.md) |
| actor | engine term | The base type of every mobile unit and creature. An actor has an actor kind, stats from the actor stat table, health, damage, a team-coloured mesh and behaviour in water and through teleport structures; a dying actor leaves an Essence orb unless its kind leaves none. | [Units](../reference/02_units/README.md); [Entity model and identity](../reference/12_simulation_model/03_entity_model_and_identity.md) |
| actor grid | engine term | A per-team uniform grid of actor handles in 32 wu cells, kept up to date as actors move; it answers neighbour and enemy-count queries. | [Space, routing and physics](../reference/12_simulation_model/05_space_routing_and_physics.md) |
| actor grid cache | engine term | A memo of repeated actor-grid queries, valid within one tick. | [Space, routing and physics](../reference/12_simulation_model/05_space_routing_and_physics.md) |
| actor group | engine term | A team-owned group of actors of one kind that move together: a Strike team, an airstrike flight, a Crawler group, the segments of a centipede, or starting forces placed by the map. | [Teams and player commands](../reference/12_simulation_model/06_teams_and_player_commands.md) |
| actor kind | engine term | The enumeration of actor types. Species defines its own values and keeps their order wherever the order carries meaning. | [Entity model and identity](../reference/12_simulation_model/03_entity_model_and_identity.md) |
| actor stat table | data term | The static table that gives health, speed and rate for each actor kind, keyed by actor kind. | [Units](../reference/02_units/README.md); [Data schemas](../architecture/components/data_schemas.md) |
| advanced outcome | player concept | Any of the 24 Supply crate outcomes that is not a basic outcome. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| airstrike | player concept | A basic Supply crate outcome: 3–7 airstrike target markers each summon an airstrike flight of 2 Bombers, which drop one bomb each (explosion radius 25 wu, damage 100). | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| airstrike flight | engine term | The actor group of 2 Bombers that one airstrike target marker summons; it flies in, bombs the marker and leaves. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| airstrike target marker | engine term | The smoke marker thrown by an airstrike; once it has been at rest for 20 ticks it summons an airstrike flight. | [Weapons and projectiles](../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |
| All power slots full | player concept | The power-slot message shown when a team is granted a Power while every one of its Power slots is occupied; the grant is rejected. | [Controls, camera and commands](../reference/06_controls_camera_and_commands.md) |
| Allegiance Scrambler | player concept | A bad-pool Supply crate outcome: for 150 ticks it reassigns Workers within 100 wu to random teams, through an Allegiance Scrambler effect. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Allegiance Scrambler effect | engine term | The effect object, not a structure, that the Allegiance Scrambler places; it reassigns Workers to random teams for 150 ticks. | [Defences and props](../reference/03_buildings_and_world_objects/defences_and_props.md) |
| alliance | player concept | A group of teams that the friendship test treats as friendly: a co-op pair, or the attacker or defender side in Siege. It is not a separate object. | [Teams and player commands](../reference/12_simulation_model/06_teams_and_player_commands.md) |
| always-on feature flag | engine term | A constant input value that always reads as true, available to binding expressions. The reference bindings do not use it. | [Input](../architecture/components/input.md) |
| Annihilator | player concept | Achievement: the running total of enemy Workers killed across matches reaches 1,000,000. The total starts at −1 and adds the local team's kills whenever a winner is declared; the stated goal is 100,000 kills, and the design phase restates the threshold (see DEC-047). | [Options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| ant | player concept | A fast Rogue-faction creature released by a Hive. It collects Essence orbs, carries Workers back to the Hive to be killed and bites other enemies. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| application | engine term | The process-wide object that owns every engine subsystem (renderer, audio engine, input, match world and front end) and the current mode state. | [Platform layer](../architecture/components/platform_layer.md) |
| asset cache | engine term | The engine's resource cache. It resolves content through the mounted pack files and caches loaded textures and meshes; profile and shipping builds read packed content only, with no loose-file override (ADR-0058). | [Asset pipeline and resources](../architecture/components/asset_pipeline_and_resources.md) |
| attackers and defenders | player concept | The two sides of a Siege map: the start positions that the map marks as attacking attack, and the remaining start positions defend the Core Device. | [Match rules](../reference/01_skirmish_rules_and_modes/README.md) |
| audio decoder | engine term | The audio engine's streaming decoder: it decodes streamed sounds in fixed-size blocks into a small ring of buffers per playing stream (ADR-0025). | [Audio engine](../architecture/components/audio_engine.md) |
| audio engine | engine term | The subsystem that loads sound source and sound event definitions, triggers sound events into sound voices and updates its voices 20 times per second (ADR-0022). | [Audio engine](../architecture/components/audio_engine.md) |

## B

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| bad outcome | player concept | The branch an opened Supply crate takes with probability 0.1: an outcome from the bad pool, created for the Rogue faction instead of the opening team (for the Abductor, the Visitor faction). A match with basic outcomes only never takes it. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| barrage missile | engine term | One warhead of a Missile Barrage: explosion radius 50 wu, damage 100. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| Barrier Emitter | player concept | A post that, when raised, projects an energy curtain to the next post; the curtain kills or ignites enemies that touch it and blocks routes. Energy surges from the energy grid switch it. | [Defences and props](../reference/03_buildings_and_world_objects/defences_and_props.md) |
| basic outcome | player concept | One of the 7 basic Supply crate outcomes: Strike team, Builder, Transport, airstrike, Reinforcements, gun turret and Essence Grove. Every other outcome is an advanced outcome. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| basic outcomes only | player concept | A match option under which only the 7 basic outcomes can be drawn and bad outcomes are disabled. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Beam Dish | player concept | A one-way long-range transporter: units that enter its door travel along its beam to the receiving dish it is aimed at. It has no capture rule of its own and takes the team of a linked Command Console. | [Economy and spawn objects](../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) |
| black box | engine term | A small ring buffer of recent engine markers (the last frame and update markers, the current map and the build identity) that every crash report carries and that a failed assertion records before the process ends (ADR-0065). | [Diagnostics, test and performance](../architecture/components/diagnostics_test_and_performance.md) |
| Blight Grove | player concept | A Supply crate outcome in the good and bad pools: 2–4 hostile trees near the target that consume Essence orbs. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Bomber | player concept | One aircraft of an airstrike or a Firebomb Run; it leaves no Essence orb. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| Bot | player concept | A computer-controlled team that fills an empty lobby slot, steered by a bot controller. Every Bot team in a match uses the same bot difficulty. | [Bots](../reference/07_bots.md) |
| bot controller | engine term | The invisible, invulnerable actor of every Bot team that drives that team's Leaders, Powers and crowds of Workers. | [Bots](../reference/07_bots.md) |
| bot difficulty | player concept | The lobby option for Bots, with three levels (easy bot, standard bot and hard bot); one setting applies to every Bot team. | [Bots](../reference/07_bots.md) |
| bot navigation node | engine term | A node of the bots' navigation and priority graph, linked to neighbours within 600 wu. It holds per-team counts and a per-team priority that a mode-specific formula recomputes every 20 ticks; Bot teams send Leaders and crowds towards high-priority nodes. | [Bots](../reference/07_bots.md) |
| bot objective | engine term | A node of the objective chain of Siege (and of the Launch Race markers) that directs attacking and defending Bot teams. | [Bots](../reference/07_bots.md) |
| bot objective marker | engine term | A point of interest of a bot objective; its ownership decides whether the objective is complete. | [Bots](../reference/07_bots.md) |
| bot team | engine term | The team kind of a team that a bot controller drives. | [Teams and player commands](../reference/12_simulation_model/06_teams_and_player_commands.md) |
| Breach Charge | player concept | A Siege carryable explosive charge, lifted by 10–30 Workers, that breaches walls. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| Brood | player concept | A Supply crate outcome: 2–4 Seed Pods, owned by the team that placed the Power, each of which hatches creatures. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Brood Drifter | player concept | A hovering four-tailed creature that drifts near the point where it appeared and lays Crawler eggs near Essence orbs. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| Builder | player concept | A hovering drone that collects Essence orbs one at a time and returns each to a friendly Spawn Node, where it becomes a Worker. | [Builder](../reference/02_units/builder.md) |
| Builder (outcome) | player concept | A basic Supply crate outcome, valid only for a team that owns Spawn Nodes: a placed Builder. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| burst and pause cycle | player concept | The request rhythm of a Spawn Node. While it is owned, activated and under the population cap, it requests one transit orb every 2 ticks until it has requested int(20 × Catch-up bonus multiplier), which is 20 without the bonus, and then pauses for 100–150 ticks; the timer freezes while the cap is reached. | [Population flow](../reference/05_population_flow.md) |

## C

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| camera view | data term | A named camera pose (position, front and up vectors) in a map definition. `spawn_view_<n>` is the start view of start position n, to which the camera cuts once the local team exists. | [Camera and sequences](../architecture/components/camera_and_sequences.md); [Map rules and conventions](../reference/08_maps_and_level_design/rules_and_conventions.md) |
| Cancelled | player concept | The power-slot message shown when a Power is terminated. | [Controls, camera and commands](../reference/06_controls_camera_and_commands.md) |
| capture | player concept | A change of a structure's owner by occupation or proximity. Each capturable kind has its own rule and period: a Spawn Node needs a single team with at least 3 of its 5 ports occupied and is re-evaluated every 20–30 ticks; a turret is re-evaluated every 10 ticks; a Solar Array and a Command Console every tick; zones and carryable structures every 10 ticks. | [Buildings and world objects](../reference/03_buildings_and_world_objects/README.md) |
| carryable structure | engine term | The base type of objects that a group of Workers lifts and carries (the Relic and the Breach Charge), with a lifter count, a carry radius and a speed scale. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| Catch-up bonus | player concept | A match option (0–90 %, default 90 %) that multiplies the spawn output of a team owning fewer than two thirds of the other teams' average number of Spawn Nodes by 1 + H/100, where H is the option value (see DEC-081). | [Match rules](../reference/01_skirmish_rules_and_modes/README.md); [Population flow](../reference/05_population_flow.md) |
| centipede | player concept | A chain of segments led by a head that hunts enemies, eats Essence orbs and grows; killing a middle segment splits it in two. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| cheats | engine term | Development-build debug commands that change match state, available only in sessions flagged for cheats (see DEC-041). | [Diagnostics, test and performance](../architecture/components/diagnostics_test_and_performance.md) |
| CJK | abbreviation | Chinese, Japanese and Korean scripts, which the glyph renderer serves from bundled fonts. | [UI layout, rendering, input and localisation](../architecture/components/ui_framework/layout_rendering_input_and_localisation.md) |
| Clean Launch | player concept | Achievement: the local team wins Launch Race with its Launch Vessel's damage at most 0.01, while no other occupied player team's Launch Vessel holds more than 50 fuel. | [Options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| cloth flag | engine term | A cloth render primitive (a 6 × 6 vertex grid) carried by Flag posts, Transports and Leaders; it is presentation only. | [Animation and VFX](../architecture/components/animation_and_vfx.md) |
| co-op | player concept | Two alliances of two players, each alliance sharing a colour family, on maps whose options allow it; allied teams are friends for targeting and scoring. | [Match rules](../reference/01_skirmish_rules_and_modes/README.md) |
| Collector | player concept | A flying unit that sucks up floating Essence orbs of any team and converts each into a Worker of its own team at a friendly Spawn Node, outside the population cap. | [Collector](../reference/02_units/collector.md) |
| Collector (outcome) | player concept | An advanced Supply crate outcome, valid only for a team that owns Spawn Nodes: a placed Collector. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Comeback Bonus | player concept | The comeback rule: every 450 ticks a team that counts as eliminated receives a random free Power, and it holds at most 2 Powers while eliminated (see DEC-452). | [Match rules](../reference/01_skirmish_rules_and_modes/README.md); [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Comeback powers: slots full | player concept | The power-slot message shown when an eliminated team is granted a comeback Power while both of the Power slots it may use are occupied. | [Controls, camera and commands](../reference/06_controls_camera_and_commands.md) |
| Command Console | player concept | A 5-port console, with 3 ports when it is linked to a Core Device, whose owner controls a linked structure. In Siege, attackers who own every Command Console linked to the Core Device destroy it. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| Completionist | player concept | Achievement: the local team has won a match on every one of the 49 maps that count. | [Options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| content build | engine term | The command-line tool, run by developers and CI, that validates authored content against its schemas and references, cooks it per asset type and writes the pack files (ADR-0059); it generates a stand-in for every manifest asset that has no authored file yet. | [Build tooling and distribution](../architecture/components/build_tooling_and_distribution.md); [Assets](../assets/README.md) |
| control scheme | player concept | The set of input bindings in use. The reference has one match control scheme; the alternative classic scheme is not specified (see DEC-023 and DEC-056). | [Controls, camera and commands](../reference/06_controls_camera_and_commands.md) |
| Control Zone | player concept | A Zone Control ring that scores for the team with the most Workers inside it. The same structure type also implements the Relic Base, the Relic Spawn Zone and Flag posts. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| Convert | player concept | A Supply crate outcome: for up to 300 ticks the team's own Workers within 70 wu fire laser bolts that convert enemy Workers to their team. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Convert Turret | player concept | The turret type whose laser bolts convert enemy Workers to the owner's team. | [Defences and props](../reference/03_buildings_and_world_objects/defences_and_props.md) |
| Core Device | player concept | The Siege objective: a countdown device of the defending team. It is destroyed when the attackers own every linked Command Console; if its countdown reaches zero first, it detonates with a Core Device pulse wave and the defenders win. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md); [Siege](../reference/01_skirmish_rules_and_modes/siege.md) |
| Core Device pulse wave | engine term | The detonation wave of the Core Device, with an initial radius of 200 wu. | [Weapons and projectiles](../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |
| countdown | player concept | The 30-tick start display at the end of the grace period. It decreases by 0.1 each tick, plays a tick sound at each whole number and a final sound at zero. | [Match rules](../reference/01_skirmish_rules_and_modes/README.md); [Audio design](../reference/10_audio_design.md) |
| crash reporter | engine term | The client's crash path: unhandled exceptions on every thread reach the operating system's error reporting together with the registered log, settings file and black box, and collection follows the player's diagnostic-data consent (ADR-0065). | [Build tooling and distribution](../architecture/components/build_tooling_and_distribution.md) |
| crate drop mode | player concept | The match option that places Supply crate drops either at random or weighted towards lower-scoring teams. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| crate drop rate | player concept | The match option that sets the interval between Supply crate drops, in seconds (default 40; 0 means no drops). | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Crawler | player concept | A fast red worm creature with a glowing trail that bites enemies and ferries Essence orbs into Crawler eggs. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| Crawler egg | player concept | A small egg that hatches 4 Crawlers 150 ticks after an Essence orb is placed in it. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| Crawler group | engine term | The actor group that draws a group of Crawlers in one batch and pre-checks it for nearby enemies. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| Crawler trail sample | engine term | One sample of a Crawler's glowing trail: position, side vector, glow step and distance. | [Creatures and hazards](../reference/02_units/creatures_and_hazards.md) |
| crowd order | player concept | A move order given to the selected Workers: they walk to the point as a loose group, optionally by the direct route. | [Controls, camera and commands](../reference/06_controls_camera_and_commands.md) |
| Crowned | player concept | Achievement: the local team wins Zone Control at time-up with a score above 1.5 × the second-highest score. | [Options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| CSV | abbreviation | Comma-separated values, the format of the asset manifest. | [Assets](../assets/README.md) |

## D

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| D3D12 | abbreviation | Direct3D 12, the Species graphics API (ADR-0001). | [Renderer](../architecture/components/renderer/README.md) |
| default camera view | data term | The camera view `default_view` that the camera targets once a map has loaded, before it cuts to the start view of the local team; a map without one leaves the camera at a fixed fallback pose. | [Camera and sequences](../architecture/components/camera_and_sequences.md); [Map rules and conventions](../reference/08_maps_and_level_design/rules_and_conventions.md) |
| direct control | player concept | Steering a single unit or structure yourself (a Strike team, a Transport, a Collector, a Builder, a turret's aim or a Beam Dish's aim) through the continuous team command state. | [Controls, camera and commands](../reference/06_controls_camera_and_commands.md) |
| draw | player concept | A tied result at time-up while sudden death is off, declared with no winning team. | [Match rules](../reference/01_skirmish_rules_and_modes/README.md) |
| DSP | abbreviation | Digital signal processing: the audio filter chain built from DSP filter definitions. In the reference no filter runs, so sound is heard dry. | [Audio engine](../architecture/components/audio_engine.md) |
| DSP filter definition | data term | One of the 14 audio filter types that the DSP chain can apply, with its parameters. | [Audio engine](../architecture/components/audio_engine.md) |
| dt | abbreviation | The game time that one tick advances: the tick period, 0.1 game seconds × game speed, applied once per tick to every object that updates in slices. | [Tick timing and main loop](../reference/12_simulation_model/01_tick_timing_and_main_loop.md) |
| dust vortex | engine term | A dust-storm particle and vortex of the dust-wind map effect. | [Weapons and projectiles](../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |

## E

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| easy bot | engine term | The lowest bot difficulty. | [Bots](../reference/07_bots.md) |
| effect | engine term | A simulated object that is neither an actor nor a structure, kept in its own container: grenades, markers, shockwave rings, order markers, weather and Wisps. Laser bolts are kept in a separate pool. | [Containers and update order](../reference/12_simulation_model/04_containers_and_update_order.md) |
| elimination | player concept | Two notions: the hard elimination that the match rules set (Spawn Control, Flag Chain), and the soft test "counts as eliminated" (Zone Control, Relic Run: no Spawn Nodes and fewer than 50 Workers) that drives the Comeback Bonus. Eliminating a team also kills its units. | [Match rules](../reference/01_skirmish_rules_and_modes/README.md) |
| energy grid | player concept | The network of energy-grid structures (Solar Arrays, grid pylons, grid splitters and grid terminals) along which energy surges travel to fuel pumps and Barrier Emitters. | [Economy and spawn objects](../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) |
| energy-grid structure | engine term | The base type of the energy grid: its grid link and the list of energy surges travelling along it. | [Economy and spawn objects](../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) |
| energy surge | player concept | A unit of energy that a Solar Array emits into the energy grid; a fuel pump turns surges into fuel pressure (surges ÷ 20), and surges switch Barrier Emitters. | [Economy and spawn objects](../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) |
| Essence Grove | player concept | A basic Supply crate outcome, applied at once: 1–2 trees near the crate that drop Essence orbs. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Essence orb | player concept | What a dying actor leaves behind; it is stored by value, not as an actor. It rises for 60 ticks, floats for 300–500 ticks during which it can be collected, then ascends for 900 ticks and vanishes. | [Essence orbs](../reference/02_units/essence_orbs.md) |
| excluded outcomes | data term | A map's list of Supply crate outcomes that its draws never pick. Comeback Bonus grants ignore the list (see DEC-452). | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md); [Map rules and conventions](../reference/08_maps_and_level_design/rules_and_conventions.md) |
| exclusion zone | player concept | An invisible circle inside which Supply crates cannot land and which can block Power targeting. | [Defences and props](../reference/03_buildings_and_world_objects/defences_and_props.md) |
| explainer animation | player concept | A front-end explainer of a mode, played from a keyframe script; there is one per mode. | [Screen flow and match setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md); [Animation and VFX](../architecture/components/animation_and_vfx.md) |
| explosion | engine term | The shared explosion: it damages actors in range, pushes them, and spawns a shockwave ring and debris. Grenades, bombs, warheads and meteors use it. | [Weapons and projectiles](../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |

## F

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| feature flag | engine term | An input value that reads as true while the settings value of the same name is above 0, available to binding expressions. The reference bindings do not use it. | [Input](../architecture/components/input.md) |
| fireball | engine term | The flame-turret projectile: it grows as it flies and sets Workers of any team on fire. | [Weapons and projectiles](../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |
| firebomb canister | engine term | The incendiary bomb of a Firebomb Run: it ignites every Worker within 62.5 wu and damages trees and Spitter Plants, without a shared explosion. | [Weapons and projectiles](../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |
| Firebomb Run | player concept | A Supply crate outcome: an airstrike whose bombs are firebomb canisters, which ignite Workers within 62.5 wu and burn trees. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Flag Chain | player concept | The mode in which teams capture Flag posts linked in a graph. A team is eliminated when an enemy captures its Home post while holding an unbroken chain of owned Flag posts back to the enemy's own Home post; at time-up, Flag posts held plus 10 per elimination decide. | [Flag Chain](../reference/01_skirmish_rules_and_modes/flag_chain.md) |
| Flag post | player concept | A Flag Chain zone that a majority of Workers captures; Flag posts are linked into a graph. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| Flag Sweep | player concept | Achievement: the local team wins Flag Chain within a time test that, as evaluated, is always true; the design phase restates it as a win within a stated number of ticks (see DEC-047). | [Options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| flame turret | player concept | The turret type that fires fireballs. | [Defences and props](../reference/03_buildings_and_world_objects/defences_and_props.md) |
| flame turret (outcome) | player concept | An advanced Supply crate outcome: a placed flame turret, built neutral and captured by manning. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| formation | player concept | A block of up to 90 Workers marching in rows of 6 to 15 in front of a Leader and fighting as one, set with the Leader's formation order. | [Leader and formations](../reference/02_units/leader_and_formations.md) |
| formation slot | engine term | One member's place in a Leader's formation. It holds the member's handle and a timer that starts at 50 ticks; the member adds the tick period each tick it registers and the Leader subtracts it, so a member that stops registering drops out when its banked time runs out. The slot index is the member's formation position. | [Leader and formations](../reference/02_units/leader_and_formations.md) |
| FOV | abbreviation | Field of view of the camera projection. | [View, lighting and fog](../architecture/components/renderer/view_lighting_and_fog.md) |
| FPS | abbreviation | Frames per second, the render rate, which is distinct from the simulation's 10 ticks per second. | [Tick timing and main loop](../reference/12_simulation_model/01_tick_timing_and_main_loop.md) |
| frame renderer | engine term | The renderer's top-level object: it composes each frame from the ordered pass table, applies fades and the pause banner, and takes screenshots. | [Renderer](../architecture/components/renderer/README.md) |
| Frenzy | player concept | A Supply crate outcome, applied at once to the whole map: for 150 ticks every Worker of every team behaves as under Fury. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| Friendly Fire | player concept | Achievement: in co-op, a Missile Barrage warhead of the local team explodes within 120 wu of a Spawn Node owned by a friendly team, the team's own included; a message also appears. | [Options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| front end | player concept | Every screen outside a match: title, mode and map selection, the lobby, the server browser, Help and Options, and credits, built as pages of the front-end screen and driven by the front-end controller. | [Screen flow and match setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| front-end controller | engine term | The object that owns the map catalogue and the lifetime of the front end. | [Screen flow and match setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| front-end screen | engine term | The full-screen paged screen of the front end: 18 live pages in 38 numbered page slots. | [UI pages and screen recipes](../architecture/components/ui_framework/pages_and_screen_recipes.md) |
| fuel | player concept | The pressure that Launch Race passes down the chain Solar Array → energy grid → fuel pump → fuel lines → fuel depot → Launch Vessel; a Launch Vessel needs 95 % fuel to load passengers. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| fuel-chain structure | engine term | The base type of the Launch Race fuel chain: its fuel link and its current fuel level. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| fuel depot | player concept | The boarding gate of a Launch Vessel and the last hop of the fuel chain; while the vessel loads, it sends idle Workers of its team aboard. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| fuel line | player concept | A relay segment of the fuel chain. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| fuel pump | player concept | The structure that turns energy surges into fuel pressure (surges ÷ 20). | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| Fury | player concept | A Supply crate outcome: for up to 300 ticks the team's own Workers within 70 wu fire every tick, take 75 % damage and move 1.5 × faster. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |

## G

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| game speed | engine term | The multiplier on game time per tick (1 = normal). Pause, the Time Dilation and Time Rush outcomes, meteor slow motion and the end of a match change it, always deterministically. | [Tick timing and main loop](../reference/12_simulation_model/01_tick_timing_and_main_loop.md) |
| game time | engine term | Deterministic simulation time, advanced by the tick clock by dt each tick. Wall-clock time comes from the platform clock and never feeds the simulation (ADR-0074). | [Tick timing and main loop](../reference/12_simulation_model/01_tick_timing_and_main_loop.md); [Platform layer](../architecture/components/platform_layer.md) |
| Giant Slayer | player concept | Achievement: the local team wins a non-co-op match against another human team that already holds Giant Slayer. Winning also marks the local player as a holder, and the holder status is shared with the other players. | [Options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| glyph renderer | engine term | Draws text from a raster glyph cache built from bundled fonts, with a font role and fallback chain per language (ADR-0034). | [UI layout, rendering, input and localisation](../architecture/components/ui_framework/layout_rendering_input_and_localisation.md) |
| good outcome | player concept | The branch an opened Supply crate takes with probability 0.9: an outcome drawn uniformly from the valid good outcomes and given to the opening team. A match with basic outcomes only always takes it. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| GPU | abbreviation | Graphics processing unit. | [Renderer](../architecture/components/renderer/README.md) |
| grace period | player concept | The frozen pre-match phase (game speed 0) in which every human team must ready up. When all are ready, a 30-tick countdown runs, and un-readying restarts it; Bot teams are never waited for. | [Match rules](../reference/01_skirmish_rules_and_modes/README.md) |
| grenade | engine term | The Worker and Trooper grenade: a 30-tick fuse and power 25, ending in a shared explosion of radius 12.5 wu and damage 50. Mortar rounds explode on impact instead. | [Weapons and projectiles](../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |
| grid pylon | player concept | A plain relay segment of the energy grid. | [Economy and spawn objects](../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) |
| grid splitter | player concept | An energy-grid router that forwards each surge to the first linked grid pylon of the surge's team. | [Economy and spawn objects](../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) |
| grid terminal | player concept | The invisible end of a grid line, which delivers a surge at once to a fuel pump or a Barrier Emitter. | [Economy and spawn objects](../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) |
| guide grid | data term | An optional coarse grid of 8-bit heights in a terrain tile definition that pins the fractal generator to a designed shape. | [Terrain](../architecture/components/terrain.md) |
| gun turret | player concept | The standard turret type. A turret is a 4-port manned weapon emplacement owned by whoever mans it; it fires automatically and its owner can aim it. | [Defences and props](../reference/03_buildings_and_world_objects/defences_and_props.md) |
| gun turret (outcome) | player concept | A basic Supply crate outcome: a placed gun turret, built neutral and captured by manning. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |

## H

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| hard bot | engine term | The highest bot difficulty. | [Bots](../reference/07_bots.md) |
| hint overlay | player concept | A contextual in-match hint: a short prompt about selection, promotion, Supply crates or the mode being played. | [Tutorial and help](../reference/11_tutorial_and_help.md) |
| Hive | player concept | A Rogue-faction nest placed by the Hive outcome: it sends ants out and converts the Essence orbs they collect into more ants. | [Defences and props](../reference/03_buildings_and_world_objects/defences_and_props.md) |
| Hive (outcome) | player concept | A Supply crate outcome in the good and bad pools: 2 Rogue-faction Hives that release 51–75 ants each; at most 4 Hives exist at once. | [Supply crates and powers](../reference/04_supply_crates_powers_and_weapons/crates_and_powers.md) |
| HLSL | abbreviation | High Level Shading Language; Species shaders are written in HLSL and compiled offline for Shader Model 6 (ADR-0003). | [Shaders](../architecture/components/renderer/shaders.md) |
| Home post | player concept | Each team's base Flag post in Flag Chain. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| hostile workers | player concept | Workers of the Rogue faction. | [Worker](../reference/02_units/worker.md) |
| HP | abbreviation | Health points, the unit of actor health and of damage. | [Units](../reference/02_units/README.md) |
| HUD | abbreviation | Heads-up display: the in-match overlay with scores, timers, Power slots and messages. | [Match rules](../reference/01_skirmish_rules_and_modes/README.md) |

## I

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| IK | abbreviation | Inverse kinematics; the two-bone leg solver of the procedural leg. | [Animation and VFX](../architecture/components/animation_and_vfx.md) |
| IME | abbreviation | Input method editor, through which players type non-Latin text. | [UI layout, rendering, input and localisation](../architecture/components/ui_framework/layout_rendering_input_and_localisation.md) |
| Improved | player concept | The power-slot message shown when a Power is upgraded. | [Controls, camera and commands](../reference/06_controls_camera_and_commands.md) |
| input router | engine term | Answers the game's queries for named input actions by resolving each through a graph of input source nodes over keyboard, mouse, gamepad, settings values, aliases, conjunctions and chords. | [Input](../architecture/components/input.md) |
| input source node | engine term | One node type of the binding graph: conjunction, chord, inversion, idle, keyboard or mouse input, gamepad input, settings value, constant value or alias. | [Input](../architecture/components/input.md) |
| Iron Defence | player concept | Achievement: a Siege match ends with the local team defending, and defending teams have owned every objective (the Core Device and its linked Command Consoles) on every tick since the start. | [Options, localisation and hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |

## J

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| job queue | engine term | A queue of jobs served by its own worker thread; the engine loads sounds in the background through one. | [Platform layer](../architecture/components/platform_layer.md) |
| jump pad | player concept | An invisible launcher that throws Workers of every team standing within 40 wu into a ballistic jump, every 3 ticks. | [Defences and props](../reference/03_buildings_and_world_objects/defences_and_props.md) |

## K

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| keyboard layout | data term | Key-name overrides for one keyboard layout, with a table that maps the system's language id to a layout. | [Data schemas](../architecture/components/data_schemas.md) |

## L

| Term | Kind | Definition | Owning document |
|---|---|---|---|
| land route graph | engine term | The waypoint graph over walkable land, built when a map loads; routes between two points are generated over it. | [Space, routing and physics](../reference/12_simulation_model/05_space_routing_and_physics.md) |
| laser bolt | engine term | The pooled laser bolt that Workers, Troopers and Convert weapons fire. It lives for 6 ticks and ricochets once off terrain; a converting bolt changes its target's team instead of damaging it. | [Weapons and projectiles](../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) |
| Launch Race | player concept | The mode in which teams capture Solar Arrays to power the fuel chain into their Launch Vessel, load 100 Workers once it holds 95 % fuel, and launch; the first Launch Vessel above world height y = 600 wu wins. It has no time limit and no draw. | [Launch Race](../reference/01_skirmish_rules_and_modes/launch_race.md) |
| Launch Vessel | player concept | Each team's Launch Race vessel: it refuels, loads 100 passengers, counts down and launches; the first above y = 600 wu wins. | [Mode objects](../reference/03_buildings_and_world_objects/mode_objects.md) |
| LCG | abbreviation | Linear congruential generator; the presentation random generator and each terrain tile's private generator are LCGs. | [Randomness and integrity](../reference/12_simulation_model/07_randomness_and_integrity.md); [Terrain](../architecture/components/terrain.md) |
| Leader | player concept | A promoted Worker that never fights: either a Waypoint beacon that redirects idle Workers within 100 wu, or the leader of a formation. Leaders are created only by promotion. | [Leader and formations](../reference/02_units/leader_and_formations.md) |
| lightning strike | engine term | A storm strike of the lightning map effect; it is not a Supply crate outcome. | [Weapons and projectiles](../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md); [Animation and VFX](../architecture/components/animation_and_vfx.md) |
| LMB / RMB | abbreviation | Left and right mouse button. | [Input](../architecture/components/input.md) |
| loading view | engine term | The loading frame, with a spinner and progress, drawn while a match loads. | [Screen flow and match setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| lobby | player concept | The match-setup page where the host picks the map and options, players pick colour and side, Bots fill empty slots and everyone readies up before the start; one page serves single-player, host and client use. | [Screen flow and match setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| lobby slot | engine term | The persistent roster record of one of the 6 team slots (identity, client, team kind, colour and ready flag); it survives the lobby, the match and the results. | [Teams and player commands](../reference/12_simulation_model/06_teams_and_player_commands.md) |
| local player team | engine term | The team kind of the team controlled on this machine. | [Teams and player commands](../reference/12_simulation_model/06_teams_and_player_commands.md) |
| LOD | abbreviation | Level of detail: render simplification by frustum and distance; it never feeds the simulation. | [View, lighting and fog](../architecture/components/renderer/view_lighting_and_fog.md) |
| LRU | abbreviation | Least recently used, the eviction rule of the decoded-sample cache once it holds more than 60 MiB. | [Audio engine](../architecture/components/audio_engine.md) |
| LT / RT / LB / RB | abbreviation | The gamepad's left and right triggers and left and right shoulder buttons. | [Input](../architecture/components/input.md) |
