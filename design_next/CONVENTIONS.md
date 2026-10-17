# Conventions

> Status: Draft (Phase 0). The naming section is completed by the rename-ledger pass, and the allowed-terms list grows during integration.

These conventions apply to every document in this design set and to every later change to it. The design checker in `Tools/DesignCheck/` enforces them (`AGENTS.md` §6).

## 1 What this set is

This set is the **Species reference library**. It is a clean, faithful specification of the reference game's mechanics, look and feel, and engine behaviour, with the engine retargeted to Windows, C++23 and Direct3D 12. It is **not** the Species game design. The Species design phase adopts, adapts or drops each reference requirement by ID. See `vision/species-vision-brief.md`.

## 2 Normative language

Requirement keywords follow RFC 2119 and RFC 8174: **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT**, **MAY**. They are written in capitals only when normative.

Documents are written in the present tense and describe what a mechanic or component does and must do. They never describe history, earlier versions or how something was found out.

## 3 Requirement IDs

Every normative statement is a requirement with a stable ID, written as `**<PREFIX>-<NNN>** (MUST) <statement>`. Numbers start at 001 within each prefix and are never reused.

### 3.1 Prefix registry

| Prefix | Scope | Document |
|---|---|---|
| `R-VIS` | Reference overview and core loop | `reference/00_reference_overview.md` |
| `R-MODE` | Skirmish match rules and modes | `reference/01_skirmish_rules_and_modes/` |
| `R-UNIT` | Units, creatures, hazards, projectiles | `reference/02_units/` |
| `R-BLDG` | Buildings and world objects | `reference/03_buildings_and_world_objects/` |
| `R-CRATE` | Supply crates, powers and weapons | `reference/04_supply_crates_powers_and_weapons/` |
| `R-ECON` | Population flow | `reference/05_population_flow.md` |
| `R-CTRL` | Controls, camera and commands | `reference/06_controls_camera_and_commands/` |
| `R-BOT` | Bots | `reference/07_bots/` |
| `R-MAP` | Maps and level design | `reference/08_maps_and_level_design/` |
| `R-META` | Front end and match flow | `reference/09_front_end_and_match_flow/` |
| `R-AUD` | Audio design | `reference/10_audio_design/` |
| `R-TUT` | Tutorial and help | `reference/11_tutorial_and_help.md` |
| `R-SIM` | Simulation model | `reference/12_simulation_model/` |
| `TER` | Terrain | `architecture/components/terrain/` |
| `RND` | Renderer | `architecture/components/renderer/` |
| `AST` | Asset pipeline and resources | `architecture/components/asset_pipeline_and_resources.md` |
| `VFX` | Animation and VFX | `architecture/components/animation_and_vfx/` |
| `AEN` | Audio engine | `architecture/components/audio_engine/` |
| `UI` | UI framework | `architecture/components/ui_framework/` |
| `INP` | Input | `architecture/components/input/` |
| `CAM` | Camera and sequences | `architecture/components/camera_and_sequences.md` |
| `DAT` | Data schemas | `architecture/components/data_schemas/` |
| `PLT` | Platform layer | `architecture/components/platform_layer.md` |
| `BLD` | Build tooling and distribution | `architecture/components/build_tooling_and_distribution.md` |
| `DBG` | Diagnostics, test and performance | `architecture/components/diagnostics_test_and_performance/` |

Decisions use `DEC-NNN` (index: `decisions/README.md`), and architecture decisions use `ADR-NNN` (`ADR/ADR-NNN-<slug>.md`, AGENTS.md §6).

## 4 Tuning tables

Every reference document has a **Tuning table** with the columns `key | value | unit | range or notes | requirement | DEC`. Keys are stable and dotted, for example `ref.unit.worker.move_speed`. Every number a rule uses appears exactly once in some tuning table; rules refer to the key and may repeat the value for readability.

## 5 Units

- Distance is in world units (`wu`).
- Every simulation duration is in **ticks**. The reference tick rate is defined once, in `reference/12_simulation_model/`. Seconds appear only as a derived display value next to a tick count.
- Rates are given per tick, or per second derived from ticks, and say which.

## 6 Content rules

A document in this set never contains:

- names of the reference product, its studio, characters, places, maps, user-interface text or code identifiers. Use the Species vocabulary in §8;
- references to source files, line numbers, evidence or build tags, verbatim excerpts, source maps, coverage figures or review statistics;
- narrative about how this set was produced, or about earlier versions of anything it describes;
- names of superseded technology, even as a contrast. Specify the Species technology positively; the rationale belongs in the ADR;
- player-facing text taken from the reference game. Write new text, keyed by Species string ids such as `ui.lobby.ready`;
- the reference's numeric type ids, message ids, magic numbers or wire layouts. Species defines its own enumerations, and keeps the order of enumerations whose order carries meaning;
- truncation words: "etc.", "and so on", "and others", "various", "similar to the above", or "…" standing in for list members.

Documents may say "the reference game" or "the reference" where the context needs it.

## 7 Templates and diagrams

- Reference documents use the reference template: Purpose and player experience · Rules · States and flows · Tuning table · Edge cases and interactions · Acceptance criteria · Dependencies · Open decisions.
- Engine components use the component template: Responsibilities and boundaries · Interfaces · Data model · Behaviour and algorithms · Concurrency and performance budget · Failure modes, diagnostics and telemetry · Acceptance tests · Open decisions (including "Design-phase inputs").
- ADRs use MADR 4. Decision-register entries (`decisions/`, index in `decisions/README.md`) use this format; an entry decided by the owner adds an `Owner decision:` line after the status:

  ```
  ### DEC-NNN <title>
  - Status: Open | Resolved | Dropped
  - Affects: <requirement ids and documents>
  - Specified behaviour: <the rule the documents state now; for an Open entry, the reference default>
  - Options: A — <option>; B — <option>
  - Recommendation: <option and a one-paragraph rationale>
  - Decide by: <milestone, "playtest" or "design phase">   (Open entries only)
  ```
- Diagrams are Mermaid only, validated, with a caption line, using Species names only. Component diagrams may use `C4Component`.
- No file is larger than 120 KB. A larger topic becomes a folder with a `README.md` index.
- The language is British English (en-GB).

## 8 Naming

Species names come from the rename ledger of the conversion run. A document uses only the names below; a name that is missing is proposed in the agent report, never invented in a document.

- **Terms.** Proper Species terms are capitalised as listed (Worker, Spawn Node, Supply crate). Generic words (tree, wall, spider, turret) stay lower case in prose. §8.14 lists the generic and component terms that the documents use without a row of their own.
- **Identifiers.** A code identifier (a type, enumerator, function, member, parameter, constant, macro, namespace or C++ source file), wherever a document writes it, in prose, a table, a diagram or an interface sketch, follows the naming convention of `AGENTS.md` §1, rules R1 to R11. The Identifier columns below are written to it. The design adds only these rules:
  - *Namespaces.* One namespace per layer (R9): `Engine` for engine code, which the components of `architecture/components/` specify, and `Species` for game code, which the reference documents and the simulation model specify. A test project's namespace is `<Project>Tests`. A type that has to know a game concept by name to do its job is game code, whichever document describes it. Engine macros are `ENGINE_*` (`ENGINE_ASSERT`, `ENGINE_PROFILE_SCOPE`). Because a layer has one namespace, a name is unique within its layer; when two documents need the same name, this section settles which one owns it (§8.12).
  - *Files.* A C++ source file is named for its primary type (R7, `BuildIdentity.h`). A shader name is PascalCase, because its source and compiled files carry it (`AGENTS.md` §2: `<Shader>VS.hlsl`, `<Shader>PS.hlsl`, `<Shader>CS.hlsl`, and `CompiledShader/<Shader>VS.h` with the byte array `g_<Shader>VS`; the permutation files of §8.11).
  - *Words of a term.* A generic word used as a type name takes its kind as a suffix (`TreeObject`, `SpiderCreature`). A term whose last word is one of the affixes AGENTS.md R2 bans (`Base`, `Impl`, `Abstract`) takes a different identifier, so the repository checker never needs an exception: the Relic Base is `RelicReceptacle`.
  - *Spelling.* An identifier spells the words of its term in American English (R11), while the term itself and all prose stay British: Iron Defence is `Achievement::IronDefense`, an RGBA colour is `ColorRgba`, a centre is a `Center`. Identifiers of an SDK keep the SDK's spelling (R4).
  - *Constants.* A compile-time constant is `UPPER_CASE` with its unit in the name where it has one (R3, R6): `MAX_TEAMS`, `UPDATE_SLICES_PER_TICK`, `TICK_PERIOD_SECONDS`, `PRODUCT_NAME`. An enumerator is PascalCase (`MapEffect::Snow`).
  - *Originality.* No Species identifier repeats an identifier of the reference implementation.
  - *Data names.* Asset ids, string ids, tuning keys, setting keys, schema ids, action ids, profiling-marker names, the names that a data table holds (pipeline catalogue entries, frame-graph resources, the enumerator strings of a schema, pack and variant ids), glTF attribute and material names (`_SPECIES_TINT`, `species_team_tint`) and the names of data files and folders are not code identifiers: they are lower case and keep their spelling. The game name is `Species` in namespaces and types and `species` in data names. A schema field whose name generates a C++ field (ADR-062) is spelt in American English, in snake case in JSON (`color_role`) and in camelCase in the generated code (`colorRole`).
  - *Layout.* An interface sketch uses the `.clang-format` layout of `AGENTS.md` §4: 2-space indent, Allman braces, 140 columns, `T* p` and `T& r`.
- **String ids** are dotted, `<prefix>.<item>`, using the prefix listed (`unit.worker.name`, `mode.flag_chain.objective`, `ui.lobby.ready`). Player-facing text is always new text.
- **Tuning keys** in reference documents are `ref.` followed by the same prefix and a parameter (`ref.unit.worker.move_speed`, `ref.mode.spawn_control.match_length`); in engine components they are the component prefix in lower case followed by a topic and a parameter (`ter.generator.octaves`, `dbg.blackbox.check_message_max_chars`).
- **Asset ids** are the `asset_id` values of `assets/asset_manifest.csv`, dotted, starting with one of these stems: `amb.`, `anim.`, `cursor.`, `font.`, `fx.`, `glyph.`, `help.`, `icon.`, `mapfile.`, `marker.`, `mesh.`, `music.`, `sfx.`, `sprite.`, `strings.`, `terrain.`, `thumb.`, `uitex.`. An asset stem is never also a string-id prefix or a tuning-key prefix, so a dotted id reads unambiguously as an asset, a string or a key; the gate checks this.
- **Setting keys, schema ids, action ids and profiling markers** are dotted lower-case names too. A setting key is `<group>.<setting>` (`audio.master_volume`, `binding.<action id>`), a schema id `<family>.<record>` (`maps.definition`), an action id `action.<group>.<name>` (`action.order.demote`, `action.menu.close`) and a profiling-marker name `<area>.<system>` with optional further steps (`render.sky.advance`). Their owners define them: the settings table and the record-type registry of the data schemas, the action tables of reference/06 and of the input component, and the marker catalogue of the diagnostics component.
- **Kinds of dotted id.** Asset ids are unique across every kind: their stems are disjoint from every root of table 8-A. The other kinds are separate namespaces that may share a root: `camera.view` is a string-id prefix, `camera.automatic` a setting key, `camera.timeline` a schema id and `camera.advance` a profiling marker. Three rules keep them apart:
  1. A document names the kind whenever it cites a dotted id that is not an asset id: the string id `ui.lobby.ready`, the tuning key `ui.chrome.title_height`, the setting `camera.automatic`, the schema `camera.timeline`, the marker `render.sky`, the action `action.sequence.skip`. A table column whose heading names the kind names it for every cell of the column.
  2. No dotted id has the same spelling in two kinds.
  3. Under the shared root `ui.` the second segment tells a tuning key of the UI framework from a UI string id. A `ui.` id is a tuning key exactly when a tuning table defines it; a key topic of table 8-B is never a string topic and a string topic is never a key topic; and the topics `ui.window` and `ui.dialog` are reserved for strings.

  The gates read an asset id by its stem and a tuning key by the tuning table that defines it, so no other kind is ever read as an asset or a key.

Table 8-A Kinds of dotted id and the roots each uses.

| Kind | Form | Roots | Defined in |
|---|---|---|---|
| asset id | `<stem>.<name>` | the 18 asset stems above | `assets/asset_manifest.csv` |
| string id | `<prefix>.<item>` | `achievement`, `bot`, `building`, `camera`, `crate`, `creature`, `faction`, `game`, `map`, `match`, `mode`, `power`, `team`, `tutorial`, `ui`, `unit` | the owning documents; the String-id prefix columns of §8.1 to §8.12; GameAssets §18.1 |
| tuning key | `ref.<prefix>.<parameter>`; `<component prefix>.<topic>.<parameter>` | `ref`; `ter`, `rnd`, `ast`, `vfx`, `aen`, `ui`, `inp`, `cam`, `dat`, `plt`, `bld`, `dbg` | the tuning tables (§4) |
| setting key | `<group>.<setting>` | `audio`, `binding`, `browser`, `camera` (only `camera.automatic`), `debug`, `input`, `interface`, `network`, `player`, `render`, `screen`, `telemetry`, `window` | the settings table 3.1-A of the data schemas |
| schema id | `<family>.<record>` | `actor`, `assets`, `camera`, `crate`, `edition`, `effects`, `geometry`, `import`, `input`, `interface`, `maps`, `mode`, `panels`, `settings`, `sound`, `telemetry`, `test`, `text` | the record-type registry, table 1-A of the data schemas |
| action id | `action.<group>.<name>` | `action`, with the groups `app`, `camera`, `debug`, `device`, `direct`, `match`, `menu`, `order`, `power`, `select`, `sequence`, `ui`, `view` | reference/06 table 2.1-A; the action tables of the input component |
| profiling marker | `<area>.<system>[.<step>]` | `audio`, `camera`, `devtools`, `effects`, `frame`, `input`, `net`, `render`, `sim` | the profile marker catalogue of the diagnostics component |

Table 8-B Topics under the root `ui.`.

| Kind | Topics |
|---|---|
| tuning key of the UI framework | `access`, `budget`, `button`, `chat_log`, `check`, `chrome`, `core`, `dropdown`, `field`, `fill_box`, `font`, `glyph`, `increment`, `lang`, `layout`, `map_row`, `menu_control`, `message_dialog`, `options_screen`, `page_back`, `page_button`, `page_dropdown`, `page_field`, `page_title`, `pages`, `palette` (the palette tokens), `picker`, `scale`, `scroll`, `scroll_text`, `text`, `value_control` |
| string id | `achievements`, `advanced_options`, `browser`, `chat`, `common`, `confirm_leave`, `connecting`, `credits`, `debug`, `dev`, `dialog`, `error`, `help`, `help_and_options`, `help_select`, `host_or_join`, `input`, `language`, `leaderboard`, `loading`, `lobby`, `main`, `map_editor`, `map_select`, `match`, `match_menu`, `menu`, `mode_select`, `options`, `player`, `player_options`, `power_slots`, `repair`, `settings`, `team`, `title`, `tutorials`, `window` |

### 8.1 Game, studio and factions

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| Bot | game | `Bot` | `bot` |
| bot controller | game | `BotController` | — |
| Rogue faction | game | `RogueFaction` | `faction.rogue` |
| Species | game | `Species` | `game` |
| Visitor faction | game | `VisitorFaction` | `faction.visitor` |

### 8.2 Units

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| Bomber | unit | `Bomber` | `unit.bomber` |
| Builder | unit | `Builder` | `unit.builder` |
| Collector | unit | `Collector` | `unit.collector` |
| Essence orb | unit | `EssenceOrb` | `unit.essence_orb` |
| hostile workers | unit | `Worker` | `unit.worker.hostile` |
| Leader | unit | `Leader` | `unit.leader` |
| order marker | unit | `OrderMarker` | `unit.leader.order_marker` |
| Strike team | unit | `StrikeTeam` | `unit.strike_team` |
| Transport | unit | `Transport` | `unit.transport` |
| Trooper | unit | `Trooper` | `unit.trooper` |
| Waypoint beacon | unit | `WaypointBeacon` | `unit.leader.waypoint_beacon` |
| Worker | unit | `Worker` | `unit.worker` |

### 8.3 Creatures and hazards

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| Abductor | creature | `Abductor` | `creature.abductor` |
| ant | creature | `AntCreature` | `creature.ant` |
| Brood Drifter | creature | `BroodDrifter` | `creature.brood_drifter` |
| centipede | creature | `CentipedeCreature` | `creature.centipede` |
| Crawler | creature | `Crawler` | `creature.crawler` |
| Crawler egg | creature | `CrawlerEgg` | `creature.crawler_egg` |
| Reaper Serpent | creature | `ReaperSerpent` | `creature.reaper_serpent` |
| Seed Pod | creature | `SeedPod` | `creature.seed_pod` |
| spider | creature | `SpiderCreature` | `creature.spider` |
| Wisp | creature | `Wisp` | `creature.wisp` |

### 8.4 Buildings and world objects

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| Barrier Emitter | world object | `BarrierEmitter` | `building.barrier_emitter` |
| Beam Dish | world object | `BeamDish` | `building.beam_dish` |
| Breach Charge | world object | `BreachCharge` | `building.breach_charge` |
| Command Console | world object | `CommandConsole` | `building.command_console` |
| Control Zone | world object | `ControlZone` | `building.control_zone` |
| Convert Turret | world object | `TurretType::Convert` | `building.turret.convert` |
| Core Device | world object | `CoreDevice` | `building.core_device` |
| energy grid | world object | `EnergyGrid` | `building.energy_grid` |
| energy surge | world object | `EnergySurge` | `building.energy_grid.surge` |
| exclusion zone | world object | `ExclusionZone` | `building.exclusion_zone` |
| Flag post | world object | `FlagPost` | `building.flag_post` |
| flame turret | world object | `TurretType::Flame` | `building.turret.flame` |
| fuel depot | world object | `FuelDepot` | `building.fuel_depot` |
| fuel line | world object | `FuelLine` | `building.fuel_line` |
| fuel pump | world object | `FuelPump` | `building.fuel_pump` |
| grid pylon | world object | `GridPylon` | `building.grid_pylon` |
| grid splitter | world object | `GridSplitter` | `building.grid_splitter` |
| grid terminal | world object | `GridTerminal` | `building.grid_terminal` |
| gun turret | world object | `TurretType::Gun` | `building.turret.gun` |
| Hive | world object | `Hive` | `building.hive` |
| Home post | world object | `HomePost` | `building.home_post` |
| jump pad | world object | `JumpPadObject` | `building.jump_pad` |
| Launch Vessel | world object | `LaunchVessel` | `building.launch_vessel` |
| mortar turret | world object | `TurretType::Mortar` | `building.turret.mortar` |
| Population Limiter | world object | `PopulationLimiter` | `building.population_limiter` |
| Relay | world object | `Relay` | `building.relay` |
| Relic | world object | `Relic` | `building.relic` |
| Relic Base | world object | `RelicReceptacle` | `building.relic_base` |
| Relic Spawn Zone | world object | `RelicSpawnZone` | `building.relic_spawn_zone` |
| rocket turret | world object | `TurretType::Rocket` | `building.turret.rocket` |
| Solar Array | world object | `SolarArray` | `building.solar_array` |
| Source Node | world object | `SourceNode` | `building.source_node` |
| spawn network | world object | `SpawnNetwork` | `building.spawn_network` |
| Spawn Node | world object | `SpawnNode` | `building.spawn_node` |
| Spitter Plant | world object | `SpitterPlant` | `building.spitter_plant` |
| Transit Gate | world object | `TransitGate` | `building.transit_gate` |
| tree | world object | `TreeObject` | `building.tree` |
| wall | world object | `WallObject` | `building.wall` |

### 8.5 Modes

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| Flag Chain | mode | `FlagChain`, `MatchMode::FlagChain` | `mode.flag_chain` |
| Launch Race | mode | `LaunchRace`, `MatchMode::LaunchRace` | `mode.launch_race` |
| mode | mode | `MatchMode` | `mode` |
| Relic Run | mode | `RelicRun`, `MatchMode::RelicRun` | `mode.relic_run` |
| Siege | mode | `Siege`, `MatchMode::Siege` | `mode.siege` |
| Spawn Control | mode | `SpawnControl`, `MatchMode::SpawnControl` | `mode.spawn_control` |
| Zone Control | mode | `ZoneControl`, `MatchMode::ZoneControl` | `mode.zone_control` |

### 8.6 Match rules

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| Catch-up bonus | match rule | `CatchUpBonus` | `match.catch_up_bonus` |
| Comeback Bonus | match rule | `ComebackBonus` | `match.comeback_bonus` |
| grace period | match rule | — | `match.grace_period` |
| kill milestone | match rule | — | `match.kill_milestone` |
| match options | match rule | `MatchOptions` | `match.options` |
| match rules | match rule | `MatchRules` | `match` |
| sudden death | match rule | — | `match.sudden_death` |

### 8.7 Supply crates and their outcomes

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| Abductor (outcome) | crate outcome | `CrateOutcome::Abductor` | `crate.abductor` |
| airstrike | crate outcome | `CrateOutcome::Airstrike` | `crate.airstrike` |
| Allegiance Scrambler | crate outcome | `CrateOutcome::AllegianceScrambler` | `crate.allegiance_scrambler` |
| Blight Grove | crate outcome | `CrateOutcome::BlightGrove` | `crate.blight_grove` |
| Brood | crate outcome | `CrateOutcome::Brood` | `crate.brood` |
| Builder (outcome) | crate outcome | `CrateOutcome::Builder` | `crate.builder` |
| Collector (outcome) | crate outcome | `CrateOutcome::Collector` | `crate.collector` |
| Convert | crate outcome | `CrateOutcome::Convert` | `crate.convert` |
| Essence Grove | crate outcome | `CrateOutcome::EssenceGrove` | `crate.essence_grove` |
| Firebomb Run | crate outcome | `CrateOutcome::FirebombRun` | `crate.firebomb_run` |
| Frenzy | crate outcome | `CrateOutcome::Frenzy` | `crate.frenzy` |
| Fury | crate outcome | `CrateOutcome::Fury` | `crate.fury` |
| Hive (outcome) | crate outcome | `CrateOutcome::Hive` | `crate.hive` |
| meteor shower | crate outcome | `CrateOutcome::MeteorShower` | `crate.meteor_shower` |
| Missile Barrage | crate outcome | `CrateOutcome::MissileBarrage` | `crate.missile_barrage` |
| Reinforcements | crate outcome | `CrateOutcome::Reinforcements` | `crate.reinforcements` |
| Rocket Troops | crate outcome | `CrateOutcome::RocketTroops` | `crate.rocket_troops` |
| Shield | crate outcome | `CrateOutcome::Shield` | `crate.shield` |
| Sickness | crate outcome | `CrateOutcome::Sickness` | `crate.sickness` |
| Spawn Surge | crate outcome | `CrateOutcome::SpawnSurge` | `crate.spawn_surge` |
| Speed Boost | crate outcome | `CrateOutcome::SpeedBoost` | `crate.speed_boost` |
| Spitter Plants (outcome) | crate outcome | `CrateOutcome::SpitterPlants` | `crate.spitter_plants` |
| Strike team (outcome) | crate outcome | `CrateOutcome::StrikeTeam` | `crate.strike_team` |
| Supply crate | crate outcome | `SupplyCrate` | `crate` |
| Supply Storm | crate outcome | `CrateOutcome::SupplyStorm` | `crate.supply_storm` |
| Swarm | crate outcome | `CrateOutcome::Swarm` | `crate.swarm` |
| Swarm streak | crate outcome | `SwarmStreak` | `crate.swarm` |
| Time Dilation | crate outcome | `CrateOutcome::TimeDilation` | `crate.time_dilation` |
| Time Rush | crate outcome | `CrateOutcome::TimeRush` | `crate.time_rush` |
| Transport (outcome) | crate outcome | `CrateOutcome::Transport` | `crate.transport` |

### 8.8 Powers

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| Power | power | `Power` | `power` |

### 8.9 Achievements

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| achievement | achievement | `Achievement` | `achievement` |
| Annihilator | achievement | `Achievement::Annihilator` | `achievement.annihilator` |
| Clean Launch | achievement | `Achievement::CleanLaunch` | `achievement.clean_launch` |
| Completionist | achievement | `Achievement::Completionist` | `achievement.completionist` |
| Crowned | achievement | `Achievement::Crowned` | `achievement.crowned` |
| Flag Sweep | achievement | `Achievement::FlagSweep` | `achievement.flag_sweep` |
| Friendly Fire | achievement | `Achievement::FriendlyFire` | `achievement.friendly_fire` |
| Giant Slayer | achievement | `Achievement::GiantSlayer` | `achievement.giant_slayer` |
| Iron Defence | achievement | `Achievement::IronDefense` | `achievement.iron_defence` |
| Massacre | achievement | `Achievement::Massacre` | `achievement.massacre` |
| Stockpiler | achievement | `Achievement::Stockpiler` | `achievement.stockpiler` |
| Total Control | achievement | `Achievement::TotalControl` | `achievement.total_control` |
| Tourist | achievement | `Achievement::Tourist` | `achievement.tourist` |

### 8.10 User interface, text and camera

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| All power slots full | UI or text | — | `ui.power_slots.full` |
| Cancelled | UI or text | — | `ui.power_slots.cancelled` |
| cheats-used marker | UI or text | — | `ui.match.cheats_used` |
| Comeback powers: slots full | UI or text | — | `ui.power_slots.comeback_full` |
| control scheme | UI or text | `ControlScheme` | `ui.options.control_scheme` |
| development caption | UI or text | — | `ui.dev` |
| explainer animation | UI or text | `ExplainerAnimation` | `ui.help.explainer` |
| Improved | UI or text | — | `ui.power_slots.improved` |
| map definition | UI or text | `MapDefinition` | `map` |
| map id | UI or text | `MapId` | `map` |
| match UI sound source | UI or text | `MatchUi` | — |
| Not available | UI or text | — | `ui.power_slots.not_available` |
| Objectives complete | UI or text | — | `ui.match.objectives_complete` |
| Power slot | UI or text | `PowerSlot` | `ui.power_slots` |
| Power slots | UI or text | `PowerSlots` | `ui.power_slots` |
| Ready | UI or text | — | `ui.power_slots.ready`, `ui.lobby.ready` |
| Skill rating | UI or text | `SkillRating` | `ui.leaderboard.skill_rating` |
| UI core | UI or text | `UiCore` | `ui` |
| Unlocked | UI or text | — | `ui.power_slots.unlocked` |

### 8.11 Naming patterns

The Pattern column gives a data-name pattern, except where it gives a code pattern (`PlayerCommand::<Command>`, `<Project>Tests`) or a file pattern of `AGENTS.md`.

| Species term | Kind | Pattern | String-id prefix |
|---|---|---|---|
| action id | naming pattern | `action.<group>.<name>` (groups in table 8-A) | — |
| bindings_classic.txt | naming pattern | `bindings_classic.txt` | — |
| camera view | naming pattern | `spawn_view_<n>` | `camera.view` |
| default camera view | naming pattern | `default_view` | — |
| map_<mode>_<players>p_<nn> | naming pattern | `map_<mode>_<players>p_<nn>` | `map` |
| map_flag_chain_<players>p_<nn> | naming pattern | `map_flag_chain_<players>p_<nn>` | `map` |
| map_launch_race_<players>p_<nn> | naming pattern | `map_launch_race_<players>p_<nn>` | `map` |
| map_relic_run_<players>p_<nn> | naming pattern | `map_relic_run_<players>p_<nn>` | `map` |
| map_siege_<players>p_<nn> | naming pattern | `map_siege_<players>p_<nn>` | `map` |
| map_spawn_control_<players>p_<nn> | naming pattern | `map_spawn_control_<players>p_<nn>` | `map` |
| map_zone_control_<players>p_<nn> | naming pattern | `map_zone_control_<players>p_<nn>` | `map` |
| player command ids | naming pattern | `PlayerCommand::<Command>` (the catalogue of reference/12 file 06; members in table 8.12-B) | — |
| previous camera view | naming pattern | `previous_view` | — |
| profiling marker | naming pattern | `<area>.<system>[.<step>]` (areas in table 8-A) | — |
| schema field (JSON key) | naming pattern | the snake case of the field name, in American spelling (`color_role`) | — |
| schema id | naming pattern | `<family>.<record>` (families in table 8-A) | — |
| setting key | naming pattern | `<group>.<setting>`; `binding.<action id>` (groups in table 8-A) | — |
| shader source | naming pattern | `<Shader><Stage>.hlsl` for the default permutation and `<Shader><Permutation><Stage>.hlsl` for any other, with the stages `VS`, `PS` and `CS`; the shared source `<Shader>.hlsli`, never compiled on its own; compiled `CompiledShader/<Shader><Permutation><Stage>.h` with the byte array `g_<Shader><Permutation><Stage>` (`AGENTS.md` §2). `<Permutation>` joins, in the order the shader lists its axes, one PascalCase part per axis that is not at its first value: the axis name for an on-off axis (`Fog`), the axis name and the number for a numeric axis (`Samples4`) and the value for a named axis (`Barycentric`), as in `TerrainInvertedBarycentricPS.hlsl` (renderer RND-261) | — |
| studio name (placeholder) | naming pattern | `<Studio>` | — |
| test project | naming pattern | `<Project>Tests`, with the placeholder suite `SuiteSmoke` (`AGENTS.md` §3) | — |
| tool-window string | naming pattern | — | `ui.debug.<tool>.<item>` |

### 8.12 Engine and simulation types

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| 2-D grid | engine type | `Grid2D` | — |
| 3-vector | engine type | `Vec3` | — |
| 3x4 matrix | engine type | `Mat3x4` | — |
| achievement events | engine type | `AchievementEvents` | — |
| action bindings | engine type | `ActionBindings` | — |
| actor | engine type | `Actor` | — |
| actor grid | engine type | `ActorGrid` | — |
| actor grid cache | engine type | `ActorGridCache` | — |
| actor group | engine type | `ActorGroup` | — |
| actor kind | engine type | `ActorKind` | — |
| actor stat table | engine type | `ActorStatTable` | — |
| Airstrike bomb | engine type | `ThrownWeaponKind::AirstrikeBomb` | — |
| airstrike flight | engine type | `AirstrikeFlight` | — |
| airstrike target marker | engine type | `AirstrikeTargetMarker` | — |
| airstrike threshold (map option) | engine type | `MapOption::AirstrikeThreshold` | — |
| Allegiance Scrambler effect | engine type | `AllegianceScramblerEffect` | — |
| always-on feature flag | engine type | `AlwaysTrue` | — |
| application | engine type | `Application` | — |
| application window | engine type | `AppWindow` (owned by the platform layer; the renderer draws into it) | — |
| asset cache | engine type | `AssetCache` | — |
| attacker reinforcement count (map option) | engine type | `MapOption::AttackerReinforcementCount` | — |
| attacker reinforcement power (map option) | engine type | `MapOption::AttackerReinforcementPower` | — |
| attacker reinforcement power option (map option) | engine type | `MapOption::AttackerReinforcementPowerOption` | — |
| audio engine | engine type | `AudioEngine` | — |
| barrage missile | engine type | `BarrageMissile` | — |
| bot navigation node | engine type | `BotNavNode` | — |
| bot objective | engine type | `BotObjective` | — |
| bot objective marker | engine type | `BotObjectiveMarker` | — |
| bot team | engine type | `TeamKind::Bot` | — |
| carryable structure | engine type | `CarryableStructure` | — |
| cloth flag | engine type | `ClothFlag` | — |
| control-hint overlay | engine type | `ControlHintOverlay` | — |
| Core Device pulse wave | engine type | `CorePulseWave` | — |
| crash reporter | engine type | `CrashReporter` | — |
| Crawler group | engine type | `CrawlerGroup` | — |
| Crawler trail sample | engine type | `CrawlerTrailSample` | — |
| defender reinforcement count (map option) | engine type | `MapOption::DefenderReinforcementCount` | — |
| defender reinforcement power (map option) | engine type | `MapOption::DefenderReinforcementPower` | — |
| defender reinforcement power option (map option) | engine type | `MapOption::DefenderReinforcementPowerOption` | — |
| DSP filter definition | engine type | `DspFilterDef` | — |
| dust object | engine type | `DustVortex` | — |
| dust vortex | engine type | — (the vortex that the dust-storm spawner holds; no type of its own) | — |
| dust wind (map effect) | engine type | `MapEffect::DustVortices` | — |
| easy bot | engine type | `BotDifficulty::Easy` | — |
| energy-grid structure | engine type | `GridStructure` | — |
| Essence-orb rain (map effect) | engine type | `MapEffect::EssenceOrbRain` | — |
| feature flag | engine type | `FeatureFlagX` | — |
| fireball | engine type | `FireballProjectile` | — |
| firebomb canister | engine type | `FirebombCanister` | — |
| formation slot | engine type | `FormationSlot` | — |
| frame renderer | engine type | `FrameRenderer` | — |
| front-end controller | engine type | `FrontEndController` | — |
| front-end screen | engine type | `FrontEndScreen` | — |
| fuel-chain structure | engine type | `FuelStructure` | — |
| glyph renderer | engine type | `GlyphRenderer` | — |
| grace period | engine type | `GracePeriod` | — |
| grenade | engine type | `GrenadeProjectile` | — |
| hard bot | engine type | `BotDifficulty::Hard` | — |
| hint overlay | engine type | `HintOverlay` | — |
| input router | engine type | `InputRouter` | — |
| input source node | engine type | `InputSourceNode` | — |
| instant Spawn Node capture (map option) | engine type | `MapOption::InstantSpawnNodeCapture` | — |
| job queue | engine type | `JobQueue` | — |
| land route graph | engine type | `LandRouteGraph` | — |
| laser bolt | engine type | `LaserBolt` | — |
| lightning (map effect) | engine type | `MapEffect::Lightning` | — |
| lightning strike | engine type | `LightningStrike` | — |
| loading view | engine type | `LoadingView` | — |
| lobby slot | engine type | `LobbySlot` | — |
| local player team | engine type | `TeamKind::LocalPlayer` | — |
| map editor | engine type | `MapEditor` | — |
| map editor hub page | engine type | `MapEditorHubPage` | — |
| map editor window | engine type | `MapEditorWindow` | — |
| map effect | engine type | `MapEffect` | — |
| map file picker | engine type | `MapFilePicker` | — |
| map light | engine type | `MapLight` | — |
| map option | engine type | `MapOption` | — |
| match camera | engine type | `MatchCamera` | — |
| match input pump | engine type | `MatchInputPump` | — |
| match statistics | engine type | `MatchStatistics` | — |
| match world | engine type | `MatchWorld` | — |
| mesh | engine type | `MeshAsset` | — |
| mesh node | engine type | `MeshNode` | — |
| mesh socket | engine type | `MeshSocket` | — |
| meteor | engine type | `MeteorRock` | — |
| muzzle flash | engine type | `MuzzleFlashEffect` | — |
| no Beam Dish aiming (map option) | engine type | `MapOption::NoBeamDishAiming` | — |
| no Catch-up bonus (map option) | engine type | `MapOption::NoCatchUpBonus` | — |
| no gun turrets (map option) | engine type | `MapOption::NoGunTurrets` | — |
| no-team value | engine type | `TeamId::None` | — |
| obstacle grid | engine type | `ObstacleGrid` | — |
| options screen | engine type | `OptionsScreen` | — |
| order trail mark | engine type | `OrderTrailMark` | — |
| pack file | engine type | `PackFile` | — |
| pad menu cycle down | engine type | `PadMenuCycleDown` | — |
| pad menu cycle up | engine type | `PadMenuCycleUp` | — |
| particle | engine type | `ParticleInstance` | — |
| particle pool | engine type | `ParticlePool` | — |
| player command | engine type | `PlayerCommand` | — |
| power and research levels | engine type | `PowerLevels` | — |
| power slots panel | engine type | `PowerSlotsPanel` | — |
| presentation generator | engine type | `FxRng` | — |
| presentation generator seed | engine type | `FxRng::Seed` | — |
| presentation random fraction | engine type | `FxRng::NextUnit` | — |
| presentation random integer | engine type | `FxRng::NextUint` | — |
| presentation random signed fraction | engine type | `FxRng::NextSigned` | — |
| procedural foot | engine type | `ProceduralFoot` | — |
| procedural leg | engine type | `ProceduralLeg` | — |
| product name | engine type | `PRODUCT_NAME` | — |
| remote player team | engine type | `TeamKind::RemotePlayer` | — |
| render backdrop | engine type | `RenderBackdrop` | — |
| RGBA colour | engine type | `ColorRgba` | — |
| RGBA8 image | engine type | `ImageRgba8` | — |
| rocket | engine type | `RocketProjectile` | — |
| route graph | engine type | `RouteGraph` | — |
| route graph node | engine type | `RouteGraphNode` | — |
| route point | engine type | `RoutePoint` | — |
| same-team Beam Dish links (map option) | engine type | `MapOption::SameTeamBeamDishLinks` | — |
| sample cache | engine type | `SampleCache` | — |
| scenery mesh | engine type | `SceneryMesh` | — |
| sea renderer | engine type | `SeaRenderer` | — |
| sea route graph | engine type | `SeaRouteGraph` | — |
| settings store | engine type | `SettingsStore` | — |
| shatter effects | engine type | `ShatterEffects` | — |
| shell casing | engine type | `ShellCasing` | — |
| shockwave ring | engine type | `ShockwaveRing` | — |
| simulated object | engine type | `SimObject` | — |
| simulated-object handle | engine type | `SimObjectId` | — |
| simulation generator | engine type | `SimRng` | — |
| simulation random fraction | engine type | `SimRng::NextUnit` | — |
| simulation random integer | engine type | `SimRng::NextUint` | — |
| simulation random signed fraction | engine type | `SimRng::NextSigned` | — |
| sky renderer | engine type | `SkyRenderer` | — |
| sliced pool | engine type | `SlicedPool` | — |
| smoke column | engine type | `SmokeColumn` | — |
| snow (map effect) | engine type | `MapEffect::Snow` | — |
| snowflake | engine type | `SnowFlake` | — |
| sound event definition | engine type | `SoundEventDef` | — |
| sound parameter | engine type | `SoundParam` | — |
| sound source definition | engine type | `SoundSourceDef` | — |
| sound voice | engine type | `SoundVoice` | — |
| spawn-network edge | engine type | `SpawnEdge` | — |
| spawn-network structure | engine type | `SpawnStructure` | — |
| special lighting (map effect) | engine type | `MapEffect::SpecialLightRig` | — |
| spectator | engine type | `TeamKind::Spectator` | — |
| spider gait | engine type | `SpiderGait` | — |
| standard bot | engine type | `BotDifficulty::Standard` | — |
| starting forces | engine type | `StartingForces` | `map.starting_forces` |
| string table | engine type | `StringTable` | — |
| structure | engine type | `Structure` | — |
| structure kind | engine type | `StructureKind` | — |
| structure port | engine type | `StructurePort` | — |
| supply-crate outcome | engine type | `CrateOutcome` | — |
| team | engine type | `MatchTeam` | — |
| team command state | engine type | `TeamCommandState` | — |
| team slots per match | engine type | `MAX_TEAMS` | — |
| teleport | engine type | `TeleportStructure` | — |
| terrain definition | engine type | `TerrainDefinition` | — |
| terrain flatten area | engine type | `TerrainFlattenArea` | — |
| terrain height field | engine type | `TerrainField` | — |
| terrain renderer | engine type | `TerrainRenderer` | — |
| terrain tile | engine type | `TerrainTile` | — |
| thrown weapon | engine type | `ThrownWeapon` | — |
| tick clock | engine type | `TickClock` | — |
| tick period | engine type | `TICK_PERIOD_SECONDS` | — |
| trail point | engine type | `TrailPoint` | — |
| Transit Gate reinforcement count (map option) | engine type | `MapOption::GateReinforcementCount` | — |
| Transit Gate Transport cap (map option) | engine type | `MapOption::GateTransportCap` | — |
| transit orb | engine type | `TransitOrb` | — |
| turret | engine type | `Turret` | — |
| turret round | engine type | `TurretRound` | — |
| UI alt pointer down | engine type | `UiAltPointerDown` | — |
| UI button | engine type | `UiButton` | — |
| UI button base | engine type | `UiWidget` | — |
| UI input pump | engine type | `UiInputPump` | — |
| UI panel base | engine type | `UiPanel` | — |
| UI pointer down | engine type | `UiPointerDown` | — |
| UI pointer held | engine type | `UiPointerHeld` | — |
| unused slot | engine type | `TeamKind::Unused` | — |
| update slices per tick | engine type | `UPDATE_SLICES_PER_TICK` | — |
| Visitor-faction position (map option) | engine type | `MapOption::VisitorFactionPosition` | — |
| volcano vent | engine type | `VolcanoVent` | — |
| waypoint order | engine type | `LeaderOrder::Waypoint` | — |
| window mode | engine type | `WindowMode` (owned by the platform layer; the renderer reads it) | — |
| world cursor | engine type | `WorldCursor` | — |
| world markers | engine type | `WorldMarkers` | — |
| world surface grid | engine type | `SurfaceGrid` | — |

Table 8.12-B registers the further names of the simulation model and the engine components, grouped by owner. A name is written once, under its owner; another document that uses it cites the owner. The namespace follows from the owner (R9). Names in the Data names group of a row are data names, not code identifiers.

Table 8.12-B Names by owner.

| Owner | Namespace | Group | Names |
|---|---|---|---|
| Simulation model (R-SIM) | `Species` | core members (entity model, diagram 2.2-A) | `MatchWorld::Advance`, `MatchWorld::Resolve`, `MatchWorld::SpawnActors`; `MatchTeam::Advance`, `MatchTeam::CreateActor`; `ActorGroup::AdvanceMembers`, `ActorGroup::Advance`; `SimObjectId::IsValid`; `SimObject::Advance`, `SimObject::Render`; `Actor::OnSpawn`, `Actor::Advance`, `Actor::ApplyHealthChange`, `Actor::PushOutOfObstacles`, `Actor::AdvanceAlongRoute`, `Actor::CreateActor`; `Structure::InitializeFromRecord`, `Structure::Advance`, `Structure::AssignTeam`, `Structure::SphereHits`, `Structure::MeshHits`, `Structure::RayHits`, `Structure::CreateStructure` |
| Simulation model (R-SIM) | `Species` | handles and kinds | `SimObjectId::Group` (`Ungrouped`, `StructurePool`, `EssenceOrbPool`, `LaserBoltPool`, `EffectPool`); `TeamId` (`TeamId::None`); `ThrownWeaponKind` (`AirstrikeBomb`) |
| Simulation model (R-SIM) | `Species` | player commands | `PlayerCommand` with `ControlState`, `Select`, `AimBeamDish`, `ToggleBarrierEmitter`, `TargetPower`, `SelectPower`, `CancelPower`, `ScrapActor`, `SelectWorkers`, `OrderWorkers`, `PromoteWorker`, `DemoteLeader`, `OrderUnit`, `ToggleUnit`, `ToggleReady`, `Pause`, `Chat`, `StartMatch`, `SetTeamName`, `SetColor`, `SetAchievementMask`, `SetSlotOpen` (team slot, open flag), `ReportScores`, `SetMatchOption`, `SetMatchOptionElement`, `SetMatchOptionText`, `SetSeasonalTheme`, `SetMatchOptionArray`, `NextMode`, `PreviousMode` |
| Simulation model (R-SIM) | `Species` | team command state fields (a public aggregate, R8) | `moveTarget`, `directMove`, `primaryFire`, `secondaryFire`, `targetDirected`, `secondaryFireDirected`, `cameraTracking`, `specialOrders`, `endSetTarget`, `gamepad`, `primaryHeld`, `pointerPosition`, `directMoveVector`, `directTargetVector` |
| Simulation model (R-SIM) | `Species` | routes, formations, space and physics | `RoutePath`, `RouteHop`, `RoutePointKind` (`Position`, `GroundPosition`, `Structure`); `GroupFormation` (`Rectangle`, `Airstrike`); the friendship function `IsFriend`; `IsPathClear`, `WalkCost`, `BuildRoute`, `PushFromObstacles`, `TryEnterTeleport`, `Bounce`, `Explode`, `Structure::PushFrom`, `ObstacleGrid::StructuresAt`, `ActorGridCache::CachedEnemies`; `ActorGrid::InRange`, `ActorGrid::Enemies`, `ActorGrid::Friends`, `ActorGrid::NearestEnemy`, `ActorGrid::CountInCells`, `ActorGrid::CountEnemiesInCells`, `ActorGrid::CountFriendsInCells`, `ActorGrid::AnyInCells`, `ActorGrid::EnemiesInCells`, `ActorGrid::FriendsInCells` |
| Simulation model (R-SIM) | — | data names | the sound source name `airstrike_bomb` |
| Terrain (TER) | `Engine` | types | `TileMethod` (`Average`, `RandomPair`, `CopyOne`), `FlattenOp` (`Absolute`, `Add`, `Subtract`, `Smooth`, `Subtract2`), `SeaMasks` (also read by the renderer), `WaveVertex`, `WaveMeshSource` |
| Terrain (TER) | `Engine` | functions | `TerrainField::Generate`, `TerrainField::Height`, `TerrainField::HeightAt`, `TerrainField::Normal`, `TerrainField::NormalAt`, `TerrainField::IsInside`, `TerrainField::CastRay`, `TerrainField::LineOfSight`; the free functions `BuildSeaMasks`, `SelectWaveVertexPairs` and `FacetColor`; `SurfaceGrid::At`, `SurfaceGrid::Set`, `SurfaceGrid::Sample`, `SurfaceGrid::Nearest`, `SurfaceGrid::IndexX`, `SurfaceGrid::IndexZ`, `SurfaceGrid::PositionX`, `SurfaceGrid::PositionZ`, `SurfaceGrid::Highest`; the private tile generator's `Seed`, `Next`, `Signed` |
| Renderer (RND) | `Engine` | frame, configuration and view | `FrameKind`, `PosterState`, `TransitionState`, `PresentMode`, `RenderConfig`, `ViewParams`, `FrameInputs`, `PresentationSnapshot`, `StartupError`, `ScissorRect`, `Ray`, `FrameView`, `DirectionalLight`, `LightRig`, `LitMaterial`, `Mat4`; the event `DeviceRebuilt` |
| Renderer (RND) | `Engine` | pipeline and frame graph | `BlendState` (`Opaque`, `Alpha`, `Premultiplied`, `Additive`, `Darkening`, `Ink`, `Ghost`), `DepthState`, `CullState`, `SortRule`, `DepthBiasVariant`, `PipelineDescriptor`, `PassTableEntry`, `FrameGraph`, `CompiledFrameGraph`, `FrameGraphError`, `ShaderPermutationId`, `RenderTargetFormats`, `CatalogKey`, `ResourceAccess`, `SettingGate`, `RecordFunction`, `PassContext` |
| Renderer (RND) | `Engine` | world, entity and overlay renderers | `StructureRenderer`, `TreeGeometry`, `TreeQuad`, `TreeParameters`, `TerrainColorRamp`, `SeaLook`, `SkyDetail`; `CrowdRenderer`, `ActorRenderer`, `EffectRenderer`, `RouteLineRenderer`, `WorldMarkerRenderer`, `WorldCursorRenderer`, `PowerSlotsPanelRenderer`, `MatchHudRenderer`, `MatchHudView`, `RoutePreview`, `RouteHandle`, `DevSphereMesh`; `OrientationMode` (`CameraFacing`, `UprightCard`, `WorldOriented`, `Segment`), `QuadAnchor` (`Center`, `BottomCenter`), `InstanceType`, `InstanceTypeId` |
| Renderer (RND) | `Engine` | shader layouts, options and look checks | `FrameConstants`, `DrawConstants`, `MeshInstance`, `QuadInstance`, `SkyLayerInstance`, `WaveVertexPair`, `CombineParams`; the sampler-index constants of the shared header `SAMPLER_LINEAR_CLAMP`, `SAMPLER_LINEAR_WRAP`, `SAMPLER_POINT_CLAMP`, `SAMPLER_POINT_MAGNIFY_CLAMP`; `RenderSettings`, `DetailLevel`, `AntiAliasing`, `QualityPreset`, `LookScene`, `LookCheck`, `LookError` |
| Renderer (RND) | `Engine` | functions | `ProjectToScreen`, `ScreenRay`, `ScissorFromWorld`, `PointInView`, `SphereInView`, `GenerateTreeGeometry`, `BuildDevSphere`, `PresetOf`, `ApplyPreset`, `Validated`, `RenderLookScene`, `ApplySettings`, `InitializeTransition`, `OnTeamChanged`, `OnRouteHighlighted`, `OnRouteIntensity`, `PreviewWalkable`; the tree generator's `Next` and `NextScaled` |
| Renderer (RND) | `Engine` | shaders and permutation macros | `Sky`, `Terrain`, `SeaFlat`, `SeaWavesUpdate`, `SeaWaves`, `MeshLit`, `UnlitTextured`, `UnlitDualTexture`, `UnlitVertexColor`, `ScreenLine`, `DepthCopy`, `Final`, `FullscreenFade`, `ScreenTransition`, `InstanceCull`, `WorldOverlay`; the permutation axes `INVERTED`, `GRID_SOURCE` (`GRID_TEXTURE`, `BARYCENTRIC`), `FOG`, `EDGE_MASK`, `CUTOUT`, `ALPHA_ONE`, `COMBINE` (`MODULATE`, `REPLACE_THEN_MODULATE`), `CLIP_PLANES`, `SAMPLES`, `HDR`, `EDGE_STRIP` and `SOURCE` (`VERTEX_COLOR`, `TEXTURED`, `MESH`); file names as §8.11 |
| Renderer (RND) | — | data names | catalogue entries `sky`, `terrain`, `terrain_inverted`, `sea_flat`, `sea_flat_inverted`, `sea_waves`, `mesh_lit_fog`, `port_pad`, `glow_additive_fog`, `glow_additive_fog_nodepth`, `alpha_fog`, `dual_alpha_fog`, `dual_additive_clip_fog`, `line_additive_fog`, `fade`, `transition`, `group_mesh_lit`, `trooper_mesh_lit`, `actor_mesh_lit`, `actor_flicker_additive`, `actor_flicker_darkening`, `actor_strip_lit`, `actor_glow`, `actor_glow_nodepth`, `actor_ink_nodepth`, `actor_ink`, `actor_darkening`, `actor_beam_alpha`, `actor_beam_additive`, `blob_shadow`, `crawler_trail`, `crawler_trail_rogue`, `worker_card`, `worker_ground_shadow`, `worker_glow`, `effect_additive`, `effect_additive_nodepth`, `effect_gradient_nodepth`, `laser_bolt`, `route_line`, `overlay_alpha`, `overlay_additive`, `overlay_ink`, `overlay_gradient_alpha`, `cursor_ring`, `cursor_area`, `cursor_dots`, `cursor_ghost`; frame-graph resources `scene_colour`, `scene_depth`, `scene_resolved`, `scene_depth_single`, `final_image`, `captured_frame`, `screenshot_readback`, `terrain_heights`, `back_buffer`; combine modes `modulate`, `replace_then_modulate` |
| Asset pipeline and resources (AST) | `Engine` | identity and mounting | `AssetId`, `VariantId`, `AssetKey`, `ContentKey`, `Sha256Digest`, `AssetError`; `MountOptions`, `MountOverride`, `MountManifest`, `MountSet`, `EntryRef`, `PackEntry` |
| Asset pipeline and resources (AST) | `Engine` | content types and import records | `ContentType`, `TextureKind`, `ColorSpace`, `AlphaMode`, `MipPolicy`, `SamplerId` (`LinearClamp`, `LinearWrap`, `PointClamp`, `PointMagnifyClamp`), `Derivation` (`BlurredShadow`, `StackedCopies`, `Grayscale`, `ToColor`, `ReplaceColor`), `TextureView`, `DerivedVariant`, `TextureImportRecord`, `TintMarking`, `MeshRole` (`Render`, `Collision`), `MeshImportRecord`, `AudioImportRecord`, `PackCodec` (`Stored`, `XpressHuffman`) |
| Asset pipeline and resources (AST) | `Engine` | reading, caching and events | `ReadPriority`, `ContentReader`, `LoadState`, `TextureSize`, `LoadGroup`, `TextureHandle`, `MeshHandle`, `RuntimeTextureId`, `ImageProducer`; `AssetLoaded`, `AssetFailed`, `LoadGroupProgress`, `LoadGroupCompleted`, `AssetReloaded` |
| Asset pipeline and resources (AST) | — | data names | pack ids `startup`, `locale`, `core`, `audio`, `update_<nn>`; the glTF attribute `_SPECIES_TINT`, the material member `species_team_tint`, the socket prefix `socket_`; variant ids `level0`, `point`, `shadow`, `trail` |
| Animation and VFX (VFX) | `Engine` | types | `PresentationClock`, `PresentationTime`; `EffectKind`, `EffectEvent`, `EffectEventId`, `ShatterPayload`; `ParticleKind`, `ParticleBlend`, `TerrainRule`, `ParticleEmission`, `ParticleTypeParams`, `PoolCounts`; `ScreenEffects`, `ShakeOffset`; `ExplainerPlayer`, `ExplainerLayout`; `VfxRuntime`, `ShatterExplosion`, `ProceduralAnimator`, `FootState`, `LegTuning`; GPU structs `ParticleSlot`, `ParticleType`, `Shard`, `Tumbler` |
| Animation and VFX (VFX) | `Engine` | `ParticleKind` enumerators | `RocketTrail`, `ExplosionCore`, `ExplosionDebris`, `MuzzleFlare`, `Fire`, `ControlFlash`, `Spark`, `BlueSpark`, `Casing`, `MissileTrail`, `MissileFire`, `WorkerFire`, `Leaf`, `Sickness`, `SpeedBoostTrail`, `FireballFlame`, `Volcano` |
| Animation and VFX (VFX) | `Engine` | `EffectKind` enumerators | `Explosion`, `RocketBurst`, `StructureDestroyed`, `LaserHitStructure`, `RoundHitStructure`, `RoundHitTerrain`, `ThrownWeaponTrail`, `AirstrikeMarkerSmoke`, `RocketFlame`, `FireballLaunched`, `FirebombBurst`, `LightningStrike`, `SwarmHit`, `TurretCooling`, `TurretShot`, `TrooperShot`, `BarrierSpark`, `ControlGained`, `ControlLost`, `LeaderPromoted`, `LeaderDemoted`, `CrowdOrder`, `LeaderOrder`, `LeaderFormUp`, `OrderMarkerStep`, `WorkerBurning`, `WorkerSpeedBoost`, `WorkerSick`, `PowerReachedWorker`, `WorkerOpeningCrate`, `WorkerAbducted`, `ActorHit`, `Bite`, `TransportExhaust`, `CollectorEngine`, `AbductorEngine`, `MeteorFire`, `OrbBroken`, `TreeBurning`, `TreeLeaf`, `SpitterPlantBurning`, `SmokeColumn`, `VolcanoEruption`, `FuelPumpStroke`, `FuelDepotBoarding`, `VesselBooster`, `VesselWrecked`, `CameraShake`, `Shatter` |
| Animation and VFX (VFX) | — | data names | catalogue entries `orb_ink`, `orb_additive`, `shard_lit`, `particle_darkening`, `particle_additive`, `cloth_cells`, `cloth_cells_fog`, `cloth_pole`, `cloth_pole_fog`, `cloth_pointer`, `cloth_overlay`; the depth-bias variant `bias_flag_pointer` |
| Audio engine (AEN) | `Engine` | types | `SoundLayerDef`, `SoundGroupDef`, `SoundVoiceId`, `SampleBank`, `AudioStream`, `BankId`, `SourceVoiceSlot`, `VoiceBackend`, `AudioBusGraph`, `CommandWriter`, `TriggerCommand`, `StopCommand`, `EmitterUpdate`, `ListenerUpdate`, `MatchState`, `AudioResult`; `EffectBinding` is the data schemas' |
| Audio engine (AEN) | `Engine` | enumerations | `SoundPosition`, `SoundSharing`, `SoundLoop`, `SoundAudience`, `SoundParamKind`, `SoundParamUpdate`, `SoundLink`, `DspFilterKind`, `AudioBus`, `EnvelopeStage`, `VoiceState`, `StopMatch`, `MatchPhase` |
| Audio engine (AEN) | `Engine` | Species XAPOs | `ChorusFx`, `CompressorFx`, `DistortionFx`, `StereoEchoFx`, `FlangerFx`, `StandardGargleFx`, `StudioReverbFx`, `ResonantLowPassFx`, `BitCrusherFx`, `GargleFx`, `EchoFx`, `SimpleReverbFx` |
| UI framework (UI) | `Engine` | core, geometry and input | `PanelId`, `WidgetId`, `PointF`, `SizeF`, `RectF`; `PointerEvent`, `PointerEventKind`, `TextInput`, `EditCommand`, `Callback`; `InputMode` is the input component's |
| UI framework (UI) | `Engine` | widgets and value binding | `UiBorderlessButton`, `UiCloseButton`, `UiInsetPanel`, `UiTextField`, `UiIncrementGlyph`, `UiScrollBar`, `UiCheckBox`, `UiDropDown`, `UiDropDownList`, `UiDropDownOption`, `UiMessageDialog`, `UiChatEntry`, `UiChatField`; `UiValueBinding`, `ValueType`; `PageButton`, `PageBackButton`, `ScreenCloseButton`, `PageTitle`, `PageDropDown`, `PageDropDownList`, `PageDropDownOption`, `PageTextField`, `PageIncrementGlyph`, `PageCheckBox` |
| UI framework (UI) | `Engine` | layout, drawing and text | `PageGrid`, `UiScaleState`, `UiBlend`, `UiVertex`, `FontRole` (`Caption`, `Body`, `Title`), `TextPass`, `TextAlign`, `TextStyle`, `DrawContext`; `TextView`, `MessageArguments`, `LanguageTag`, `StringId`; `AssetId` is the asset pipeline's |
| UI framework (UI) | `Engine` | public fields (R8) | `UiButton::fontSize`, `UiButton::centered`, `UiButton::disabled`; `TextStyle::role`, `TextStyle::size`, `TextStyle::color`, `TextStyle::blend`, `TextStyle::pass` |
| UI framework (UI) | `Engine` | functions | `ComputePageGrid`, `LargeMenuScale`, `ToolWindowScale`, `FinalTextSize`; `RegisterPanel`, `RemovePanel`, `BringToFront`, `SetPanelPosition`, `SetPanelSize`, `PanelAt`, `WidgetAt`, `PointerInPanel`, `PointerInWidget`, `UpdatePointer`, `RegisterPopup`, `ClosePopup`, `Maximize`, `Unmaximize`, `SetFocus`, `IsTextEditing`, `WindowsOnScreen`, `RemoveAllWindows`, `SetTooltipHandler`; `MenuSize`, `SetMenuSize`, `ClientRect`, `AddToNavigation`, `SetCursor`, `SetBackAction`, `SkipNextUpdate`, `ConsumeScroll`, `AddValueControl`, `RemoveValueControl`, `AddColorControl`, `AddMenuControl`, `ShowDialog`; `MeasureWidth`, `Prerasterize`, `Lookup`, `Exists`, `Format` |
| UI framework (UI) | `Engine` | pages and screen recipes | `PageId` (`Title`, `Tutorials`, `HostOrJoin`, `ModeSelect`, `MapSelect`, `MatchSetup`, `AdvancedOptions`, `PlayerOptions`, `SessionBrowser`, `BrowserFilters`, `Password`, `Connecting`, `WaitForStart`, `Error`, `Credits`, `QuickMatch`, `Achievements`, `LeaderboardSelect`, `Leaderboard`), `NavOp` (`Push`, `Pop`, `Replace`, `UnwindTo`, `Rebuild`), `NavRequest`, `TransitionDirection`, `EnterOutcome` (`Built`, `Waiting`, `Redirect`), `EnterResult`, `FillBoxes`, `FrontEndPage`, `PageHost`, `PageContext`, `FrontEndViewModel`, `WidgetBuilder`, `ToolWindow`, `ToolPlacement`, `HudPanel`, `HudContext`; `OpenToolOnce`, `CloseTool`, `ToolString`; the events `PageEntered`, `PageLeft`, `NavigationRejected` |
| UI framework (UI) | `Engine` | front-end widget kinds | `ScrollingText`, `ModeDescription`, `MapDescription`, `OptionDetails`, `StaticImage`, `ModePicture`, `MapThumbnail`, `HelpPicture`, `ColorPickerRow`, `ModeRow`, `MapRow`, `MapListHeader`, `SessionRow`, `SessionListHeader`, `LobbySlotRow`, `KickControl`, `SlotOpenControl`, `PrimaryButton`, `MapNameLabel`, `ChatLog`, `ConnectionStatus`, `TextLabel` |
| UI framework (UI) | — | data names | the action ids `action.menu.left`, `action.menu.right`, `action.menu.up`, `action.menu.down`, `action.menu.activate`, `action.menu.front_end_activate`, `action.menu.close`, `action.menu.dismiss`, `action.menu.scroll_speedup`; the string ids `ui.dialog.close`, `ui.dialog.error`, `ui.dialog.success` |
| Input (INP) | `Engine` | types | `InputEvent`, `ActionValue`, `FrameGates`, `PointerState`, `InputPointer`, `DeviceBackEnd`, `GamepadBackEnd`, `KeyboardMouseBackEnd`, `InputEventQueue`, `DeviceState`, `BindingStateTable`, `PadAssignment`, `LoadDiagnostic`, `LoadReport`, `BindingSources`, `BindingDescription`, `DeviceClass`, `DeviceClassMask`, `DeviceId`, `LocalPlayerId`, `ActionId`, `KeyCode`, `MouseControl`, `PadControl`, `ValueKind`, `Condition`, `InputMode` (also used by the UI framework) |
| Input (INP) | `Engine` | members and aggregate fields (R8) | `InputRouter::onModeChanged`, `InputRouter::onLayoutChanged` (public notification signals); `DeviceBackEnd::Initialize`; `InputEvent::Kind` (`Press`, `Release`, `Axis`, `Connect`, `Disconnect`); the fields of `InputEvent` (`kind`, `osRepeat`, `deviceClass`, `device`, `control`, `timestampNanoseconds`, `x`, `y`), `FrameGates` (`uiWindowOpen`, `textEntryOpen`, `windowFocused`) and `PointerState` (`positionPixels`, `wheelNotches`, `deltaPixels`, `wheelDeltaNotches`, `moved`) |
| Input (INP) | `Engine` | binding nodes and helpers | `BindingNode`, `AllNode`, `ChordNode`, `NotNode`, `AliasNode`, `LatchNode`, `IdleNode`, `SettingNode`, `ValueNode`; `ActionAxis`, `StickAxis`, `PriorityAxis`, `ActionTrigger` |
| Input (INP) | — | data names | the action ids `action.menu.cycle_up`, `action.menu.cycle_down`, `action.menu.select_previous`, `action.menu.select_next`, `action.menu.toggle_period` (with the nine of the UI framework); the control names of the input component's tables 3.2-A to 3.2-C; the layout tags `de-DE`, `en-US`, `fr-FR`, `en-GB`, `en-CA`; the binding-set layer kind `development` (INP-094) |
| Camera and sequences (CAM) | `Engine` | types | `CameraMode` (`Static`, `ScriptedMove`, `FreeMovement`, `Tracking`, `BeamDishAim`, `TurretAim`, `StructureFocus`, `DeveloperFly`), `CameraTransition` (`Move`, `Cut`), `CameraPose`, `CameraView`, `CameraAnimationStep`, `CameraAnimationDef`, `MapCameraData`, `CameraFrameInputs`, `CameraFrameOutput`, `CameraState`, `PickRay`; `TimelineOp` (`Wait`, `WaitCamera`, `WaitFade`, `WaitCondition`, `CameraCut`, `CameraMove`, `CameraApproach`, `CameraFocus`, `CameraAnimation`, `CameraFov`, `CameraRelease`, `Caption`, `Fade`, `Overlay`, `Sound`, `PermitAbort`), `TimelineStatement`, `PresentationTimeline`, `TimelinePlayer`, `StatementArgs`, `SourceLocation`, `TimelineFrameInputs` |
| Camera and sequences (CAM) | `Engine` | `MatchCamera` functions | `SetTargetView`, `SetTargetPose`, `SetTargetAround`, `Cut`, `StartMove`, `FocusStructure`, `EnterMode`, `ReleaseToPlayer`, `SetFieldOfView`, `SetTargetFieldOfView`, `PlayCameraAnimation`, `StopCameraAnimation`, `IsBusy`, `PickRayAt`, `ViewProjectionFor`, `EnterMatch`, `OnLocalTeamKnown`, `LeaveMatch` |
| Camera and sequences (CAM) | — | data names | the action ids `action.debug.camera_fast`, `action.debug.camera_slow`, `action.debug.camera_rotate`, `action.sequence.skip` |
| Data schemas (DAT) | `Engine` | types | `FieldType`, `SimPrecision`, `Presence`, `FieldSpec`, `RecordSchema`, `SchemaRegistry`, `Diagnostic`, `AuthoringParser`, `RecordValidator`, `RecordCooker`, `CanonicalWriter`, `SchemaMigration`, `RecordView`, `RecordLoader`, `MapHeaderView`, `MapDefinitionView`, `MapLoader`, `StructureRecord`, `StructureLink`, `SettingSpec`, `SettingsDefaults`, `SettingsFile`, `StringTableBuilder`, `StringTableView`, `EffectBinding` (also read by the audio engine), `DspParamDesc`, `ImportRecordReader` |
| Data schemas (DAT) | `Engine` | soak and telemetry records | `SoakProgressionKind`, `SoakEntryFailure`, `SoakParticipantRole`, `SoakSequenceItem`, `SoakTestMap`, `SoakConfigurationRecord`, `SoakMatchRow`, `SoakPassSummary`, `TelemetrySession`, `SoakMatchTableWriter` |
| Data schemas (DAT) | — | data names | the schema ids `maps.definition`, `maps.index`, `mode.options`, `settings.defaults`, `input.binding_set`, `input.keyboard_layouts`, `text.string_table`, `actor.stats`, `crate.outcomes`, `sound.sources`, `sound.groups`, `sound.effect_kinds`, `geometry.mesh`, `geometry.collision`, `geometry.sim_record`, `panels.explainer`, `effects.definition`, `camera.timeline`, `interface.page_layout`, `interface.palette`, `interface.text_style`, `interface.font_roles`, `import.texture`, `import.mesh`, `import.audio`, `assets.engine_table`, `edition.manifest`, `test.soak_configuration`, `test.soak_report`, `telemetry.session`; the setting-key groups of table 8-A |
| Platform layer (PLT) | `Engine` | clock and window | `PlatformClock`, `TimeSource`, `TimeSourceKind`, `ManualTimeSource`, `Nanoseconds`; `AppWindow`, `WindowMode`, `Permanence`, `WindowPlacement`, `WindowMessage`, `WindowInputSink`, `WindowEvent` (`Resized`, `DpiChanged`, `FocusChanged`, `Minimized`, `Restored`, `ModeChanged`, `MonitorChanged`, `DisplayConfigurationChanged`, `KeyboardLayoutChanged`, `EndOfSession`); `MonitorInfo`, `DesktopMode`, `ModeSize`; `PointPx`, `SizePx`, `RectPx`, `NativeWindowHandle` |
| Platform layer (PLT) | `Engine` | files, lifecycle, threads and services | `PlatformPaths`, `UserFolder`, `MappedFile`, `FileError`, `PlatformError`, `SettingValue`; `StartupOptions`, `MessageSeverity`, `MessageArgument`, `LifecycleHooks`, `EndWithMessage`, `FailFast`, `ExitAfterTeardown`, `ShowPlatformMessage`; `PlatformThread`, `CrashContext`, `CrashFileKind`, `SystemSummary`; `HttpsClient`, `HttpsRequest`, `HttpsResponse`, `HttpsError`, `AudioEndpointChange`, `ScreenshotImage`, `ScreenshotWriter` |
| Platform layer (PLT) | — | data names | the settings file `settings.txt`; the user-data folders `maps\` and `diagnostics\` |
| Build tooling and distribution (BLD) | `Engine` | types, files and macros | `BuildProduct` (`BuildProduct::Client`, `BuildProduct::Server`, `BuildProduct::ContentBuildTool`), `BuildConfiguration` (`Debug`, `Release`), `SemanticVersion`, `BuildIdentity`, `EditionId`, `ContentBuild`, `ContentBuildCommand` (`Build`, `Validate`, `Loose`, `StringsExport`, `StringsImport`), `ContentBuildStage` (`ResolveEditions`, `ReadInputs`, `CheckSources`, `Cook`, `Derive`, `Placeholders`, `CookRecordsAndGenerate`, `CheckReferences`, `SelectEditions`, `Localization`, `MapIdentities`, `WritePacks`, `WriteManifestsAndReports`, in stage order), `ContentBuildError`, `MapIdentityEntry`, `LanguageFindings`, `RunOptions`, `RunResult`; the generated header `BuildIdentity.h`; the development switch `ENGINE_DEVELOPMENT` (ADR-056) |
| Build tooling and distribution (BLD) | — | data names | `species-contentbuild.exe`, `species_server.exe`; `species_version.json`, `build_identity.json`, `channel_tag.txt`, `language_report.json`, `validation_report.json`, `placeholder_report.json`, `localisation_report.json`, `map_identities.json`; the product values `client`, `server` and `contentbuild` of the build identity; the edition ids `full`, `full_server`, `development`, `soak`; the feature flags `modes`, `crate_classes`, `tutorial`, `play_local_network`, `play_internet`, `map_editor`; the language groups `all`, `efigs`; the start-up switches `--write-build-identity`, `--verify-content` |
| Diagnostics, test and performance (DBG) | `Engine` | types and macros | `LogLevel`, `LogCategory`, `LogStats`, `BlackBox`, `ProfileTree`, `ProfileNodeStats`, `ProfileNodeId`, `CounterProvider`, `CounterSink`, `DevDrawQueue`, `DevDrawItem`, `DevDrawItemId`, `SoakRunner`, `SoakConfig`, `SoakEntry`, `SoakProgression`, `SoakReport`, `TelemetrySessionRecord`, `FatalExit`, `CheckFailed`, `Vec2`; `ENGINE_ASSERT`, `ENGINE_CHECK`, `ENGINE_CHECK_INDEX`, `ENGINE_PROFILE_SCOPE`, `ENGINE_PROFILE_GPU_SCOPE` |
| Diagnostics, test and performance (DBG) | `Species` | debug commands (game concepts, R9) | `DebugCommand`, `DebugCommandKind` (`KillAllEnemies`, `SpawnWorkers`, `SpawnTransport`, `SpawnCrawlers`, `SpawnEssenceOrbs`, `AllowArbitraryPlacement`, `SpawnAtStructurePorts`, `SpawnSupplyCrate`, in button order) |
| Diagnostics, test and performance (DBG) | — | data names | the action ids `action.debug.command_window`, `action.debug.drop_packet`, `action.debug.special_lighting_on`, `action.debug.special_lighting_off`, `action.debug.soak_rendering`, `action.debug.overlay`, `action.debug.profile_view`; the setting keys `debug.command_history`, `debug.capture_frame_rate`, `debug.soak_session_name`; the marker areas of table 8-A; the string ids `ui.dev.*` (among them `ui.dev.command.dump_bindings`) and `ui.match.cheats_used` |

### 8.13 Names used only in Dropped decision entries

These name content that the reference library does not specify: Advisor, Hatchery, Rift, Rift Horror, Rift tentacle, Summoner, Tri-walker, Vehicle Duel.

### 8.14 Generic and component terms

These terms are ordinary words, or names that one owner defines in prose. They need no identifier of their own, and a document uses them in the owner's sense; the glossary defines each one that a reader needs.

| Owner | Terms |
|---|---|
| generic terms of the reference documents | activation delay, advanced outcome, alliance, attackers and defenders, bad outcome, basic outcome, basic outcomes only (the Supply-crate match option; always this name), black box, bot difficulty, burst and pause cycle, capture, cheats, co-op, content build, countdown, crate drop mode, crate drop rate, crowd order, direct control, draw, effect, elimination, excluded outcomes, explosion, formation, front end, fuel, game speed, game time, good outcome, keyboard layout, lobby, main loop, match, match setup, mode second, music channel, network statistics window, no team, one-minute warning, operator, outside height, page, passengers, pause, player command, power-slot message, presentation generator, rank announcement, ready (ready-up), reinforcement count, reinforcement interval, reinforcement wave, replay, results screen, route, score, score mode, server browser, settings file, simulation generator, simulation state, soak test, start position, starting forces, starting powers, starting units, string id, team kind, team slot, tick, time limit, time-up, Transport cap, turret cap, turret frequency, tutorial, ungrouped actors |
| Simulation model (R-SIM) | Airstrike bomb, command frame, command sequencer, dust object (one `DustVortex` effect), extrapolation time, handle group, presentation frame delta, presentation frame time, smooth presentation time, tick-take time, valid-spawn-position search, the Visitor faction's pulse wave (the Core Device pulse wave, `CorePulseWave`) |
| Terrain (TER) | conformance generator; the notation r32, pow32, exp32, log32, sqrt32, sin32, abs32, log2r and ceilLog2, which names arithmetic and is no identifier |
| Renderer (RND) | blight timer, blighted tree, bottom-anchored billboard, burn level, Crawler trail texture, development sphere, empty compass, entity detail, Essence Grove tree, ghost model, high-detail factor, inverted style, look scene, match message, object lighting, ownership lights, poster resolution, quality preset (high, medium, low, custom), route fade record, route preview walkable flag, screen transition, sea detail, sky detail, special lighting, structure detail, tree detail, turning blighted, Worker distance scale factor |
| Asset pipeline and resources (AST) | asset index, collision asset, content manifest, derived variant, engine asset table, import record, load group, loose-content directory, mount manifest, placeholder, residency list, run-time texture, simulation data record, view |
| Audio engine (AEN) | audio clock, effect bus, insert routing, interface bank, match bank, real instance, send routing, silent device, start time, state hold, steal margin, stream head, virtual instance, voice pool |
| UI framework (UI) | back action, back handler, base press, birth cascade, captured widget, close box, draw point, enter rule, fill box, font role, highlight beam, hook page, horizontal scale, horizontal spacing factor, hover-highlight mode, HUD panel, increment glyph, inset panel, leave hook, match-setup sync, menu size, navigation cursor, navigation order, page catalogue, page host, page list, page stack, palette token, pop, screen view-model, session-discovery listener, text row convention, text-type field, tool window |
| Camera and sequences (CAM) | approach, camera clock, camera trigger, developer fly mode, level up vector, presentation timeline, scripted move, structure focus |
| Data schemas (DAT) | absence-meaningful field, canonical writer, caption marker, derived field, limit form, logical field type, record type, schema id, simulation decimal |
| Platform layer (PLT) | crash-context record, fatal path, fixed-step source, manual source, platform clock, platform time, pre-UI message, run id, system summary, user-data root |
| Build tooling and distribution (BLD) | build identity, channel tag, channel wrapper, configuration, edition, edition manifest, edition marked for release, localisation report, map-identity listing, permitted maps, product, reference feature set, release procedure, repair check, symbol store, tools solution, translator pack |
| Diagnostics, test and performance (DBG) | cheats-used marker, counter provider, debug command, design population, development drawing queue, development overlay, profile view, reference test machine, soak entry, stress population |

## 9 Allowed terms

Soft-flagged words that are allowed in a specific context, with the justification. The **Paths** column lists the file globs (relative to the design root) where the gate accepts the term; this file is always among them. Any other soft-flag hit fails the gate.

| Term | Where allowed | Justification | Paths |
|---|---|---|---|
| lockstep | `decisions/deferred/*.md` and decision entries that record what the design phase must decide | names the reference networking model so that the design phase can decide against it; never used as a Species requirement | `decisions/deferred/*.md` `CONVENTIONS.md` |
| Linux | ADRs, decision entries and the "Design-phase inputs" of component specifications, about dedicated-server or CI hosting | community hosts mostly run Linux (vision brief §6 item 10); the client is Windows-only | `ADR/*.md` `decisions/deferred/*.md` `architecture/components/*.md` `CONVENTIONS.md` |
| immediate-mode | developer-tool UI only (`ADR/`, `architecture/components/diagnostics_test_and_performance/`) | the standard name of the debug-overlay UI pattern; the game UI is retained | `ADR/*.md` `architecture/components/diagnostics_test_and_performance/*.md` `CONVENTIONS.md` |
| retail | vendor release-channel names in `ADR/README.md` §"Technology baseline" (for example a retail SDK or Shader Model release) | quotes the vendor's own channel name, with its source URL | `ADR/README.md` `CONVENTIONS.md` |
