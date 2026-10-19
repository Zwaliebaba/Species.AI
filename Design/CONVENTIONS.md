# Conventions

> Status: Reference library conventions. They bind every document of this library and every later change to it.

These conventions apply to every document in this design set and to every later change to it. The design checker in `Tools/DesignCheck/` enforces them (`AGENTS.md` §6).

## 1 What this set is

This set is the **Species reference library**. It is a clean, faithful specification of the reference game's mechanics, look and feel, and engine behaviour, with the engine retargeted to Windows, C++23 and Direct3D 12. It is **not** the Species game design. The Species design phase adopts, adapts or drops each reference requirement by ID. See `vision/species-vision-brief.md`.

## 2 Normative language

Requirement keywords follow RFC 2119 and RFC 8174: **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT**, **MAY**. They are written in capitals only when normative.

Documents are written in the present tense and describe what a mechanic or component does and must do. They never describe history, earlier versions or how something was found out.

## 3 Requirement IDs

Every normative statement is a requirement with a stable ID, written as `**<PREFIX>-<NNN>** (MUST) <statement>`. Numbers start at 001 within each prefix and are never reused.

- **New ids.** A new requirement takes the next free number of its prefix; no id is renumbered, and an id that has been used is never given to another statement, even after the statement it named has gone.
- **Removed requirements.** A requirement that is removed while anything still cites it keeps its id and its place, with the text "(removed: <reason>)" in place of its statement, so that every citation stays valid and says why the rule is gone. A removed requirement that nothing cites disappears, and its id stays retired.
- **Restated rules.** One rule has one owner. A requirement that restates a rule which another requirement owns names that owner ("restates R-…"); the owner's text and values govern where the two differ, and the design phase gives a restating id the verdict of its owner.

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
| `BLD` | Build tooling and distribution | `architecture/components/build_tooling_and_distribution/` |
| `DBG` | Diagnostics, test and performance | `architecture/components/diagnostics_test_and_performance/` |
| `PIL` | Species: vision, pillars and core loops | `gdd/00_vision_pillars_and_core_loops.md` |
| `WLD` | Species: world and regions | `gdd/01_world_and_regions/` |
| `SET` | Species: settlements and population | `gdd/02_settlements_and_population.md` |
| `RES` | Species: resources and economy | `gdd/03_resources_and_economy.md` |
| `CON` | Species: building and construction | `gdd/04_building_and_construction.md` |
| `CMB` | Species: units, combat and battles | `gdd/05_units_combat_and_battles.md` |
| `REL` | Species: Cladans, relations and conflict | `gdd/06_cladans_relations_and_conflict.md` |
| `DEP` | Species: deputy, standing orders and AFK play | `gdd/07_deputy_standing_orders_and_afk_play.md` |
| `AIS` | Species: AI settlements | `gdd/08_ai_settlements.md` |
| `PRG` | Species: progression hooks | `gdd/09_progression_hooks.md` |
| `SKM` | Species: skirmish | `gdd/10_skirmish.md` |
| `CTL` | Species: controls, camera and commands | `gdd/11_controls_camera_and_commands.md` |
| `FE` | Species: front end and onboarding | `gdd/12_front_end_and_onboarding.md` |
| `HUD` | Species: HUD and information | `gdd/13_hud_and_information.md` |
| `SND` | Species: audio design | `gdd/14_audio_design.md` |
| `HLP` | Species: tutorial and help | `gdd/15_tutorial_and_help.md` |
| `QG` | Species architecture: quality goals | `architecture/01_introduction_and_goals.md` |
| `QS` | Species architecture: quality scenarios | `architecture/10_quality_requirements.md` |
| `SRV` | Species component: server and hosting | `architecture/components/server_and_hosting.md` |
| `WSIM` | Species component: world simulation | `architecture/components/world_simulation.md` |
| `REP` | Species component: replication | `architecture/components/replication.md` |
| `PER` | Species component: persistence | `architecture/components/persistence.md` |
| `IDN` | Species component: identity and services | `architecture/components/identity_and_services.md` |
| `API` | Species component: server API | `architecture/components/server_api.md` |
| `AIR` | Species component: AI runtime | `architecture/components/ai_runtime.md` |
| `LWD` | Species component: large world | `architecture/components/large_world.md` |
| `SEC` | Species component: security | `architecture/components/security.md` |
| `TEL` | Species component: telemetry and privacy | `architecture/components/telemetry_and_privacy.md` |
| `MVP` | Species implementation: the solo MVP scope | `implementation/mvp_scope.md` |

Decisions use `DEC-NNN` (index: `decisions/README.md`), and architecture decisions use `ADR-NNN` (`ADR/ADR-NNN-<slug>.md`, AGENTS.md §6). Both are stable in the same way as requirement ids: never renumbered, never reused, and a decision that no longer applies becomes Dropped and keeps its id.

- **DEC ids.** The decisions of this reference library have three-digit ids, each numbered within the block of its register file. The block DEC-900 to DEC-949 is reserved for the decisions that the review of this library leaves open; no register file numbers into it. The Species design phase numbers its own decisions with four-digit ids, DEC-NNNN, from one thousand upward, so that a design decision never collides with a reference-library decision.
- **ADR ids.** A new architecture decision record takes the next id after the highest existing one (`ADR/README.md`).

## 4 Tuning tables

Every reference document has a **Tuning table** with the columns `key | value | unit | range or notes | requirement | DEC`. Keys are stable and dotted, for example `ref.unit.worker.move_speed`. Every number a rule uses appears exactly once in some tuning table; rules refer to the key and may repeat the value for readability.

The Species design documents (the prefixes from `PIL` on) use the lower-case prefix of their document as the key root (`wld.region.size_max`, `dep.order.limit`, `srv.tick.budget_ms`, and `mvp.` for the MVP scope) and add two columns to the tuning table: `status` (`final` or `provisional`) and a playtest note, required for a provisional value. A reference requirement that the Species design adopts keeps its `ref.` keys; one it adapts moves its numbers to Species keys (`gdd/adoption_ledger/`).

## 5 Units

- Distance is in world units (`wu`).
- Every simulation duration is in **ticks**. The reference tick rate is defined once, in `reference/12_simulation_model/`: one tick is `ref.sim.tick_period` (0.1 s) of game time, so ticks per second = 1 ÷ `ref.sim.tick_period`. Seconds appear only as a derived display value next to a tick count.
- A formula or rule that uses the tick rate writes it through `ref.sim.tick_period` (ticks = s ÷ `ref.sim.tick_period`, s = ticks × `ref.sim.tick_period`), never as a bare 10, so that a change of the tick period reaches it (DEC-227).
- A seconds value written beside a tick count is one of two kinds, and says which. A **nominal** duration is the reference duration in seconds whose countdown gives the tick count (DEC-241 item 3, DEC-859); it is written "nominal X s", as in "601 ticks, nominal 60 s". Any other seconds value beside a tick count is the derived value, ticks × `ref.sim.tick_period`, as in "601 ticks, 60.1 s".
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

A document uses only the names below. A new name is added to this section, with its glossary row, in the same change that first uses it; a document never coins a name on its own.

- **Terms.** Proper Species terms are capitalised as listed (Worker, Spawn Node, Supply crate). Generic words (tree, wall, spider, turret) stay lower case in prose. §8.14 lists the generic and component terms that the documents use without a row of their own.
- **Identifiers.** A code identifier (a type, enumerator, function, member, parameter, constant, macro, namespace or C++ source file), wherever a document writes it, in prose, a table, a diagram or an interface sketch, follows the naming convention of `AGENTS.md` §1, rules R1 to R11. The Identifier columns below are written to it. The design adds only these rules:
  - *Namespaces.* One namespace per layer (R9): `Engine` for engine code, which the components of `architecture/components/` specify, and `Species` for game code, which the reference documents and the simulation model specify. A test project's namespace is `<Project>Tests`. A type, enumeration or free function that has to know a game concept by name to do its job is game code in `Species`, whichever document describes it: its name, an enumerator or a type in its declaration names a game concept. The game concepts are the Species terms of §8.1 to §8.9 (factions and bots, units, creatures, the world objects other than the generic tree and wall, modes, match rules, Supply crates and their outcomes, Powers, achievements), the Power slots of §8.10 and the match HUD, teams, and the game types of table 8.12-A. The engine's own vocabulary names none of them: match, map, front end, page, session, tick, frame, camera, sound, effect, particle, mesh, terrain, sea, sky and tree. Game code builds on engine types and never the reverse: no `Engine` declaration names a `Species` type, and an engine type that carries a game value holds it as a generic engine value (an id, a handle, a palette index or an interface) that the game layer's presentation code fills, as the snapshot interface `SceneSnapshot`, the emitter id `EmitterId`, the audience group `AudienceGroup` and the particle-type id `ParticleTypeId` do. An engine setting, record field or enumerator whose value has a game meaning is named for its engine role, and the `Species` code that reads it gives it that meaning: the soak entry's automated-participant count `SoakEntry::automatedParticipants`, which the game fills with bot teams, the renderer's decoration-detail level `RenderSettings::decorationDetail` and crowd-card distance scale `RenderSettings::crowdCardDistanceScale`, which the game's renderers map onto its structures and its Worker cards, and the frame kinds `FrameKind::Loading` and `FrameKind::Editor`, which the game selects for its loading view and its map editor. The identifiers of §8.1 to §8.10 are game code, except the UI core (`UiCore`), which is the UI framework's. Engine macros are `ENGINE_*` (`ENGINE_ASSERT`, `ENGINE_PROFILE_SCOPE`). Because a layer has one namespace, a name is unique within its layer; when two documents need the same name, this section settles which one owns it (§8.12).
  - *Registry.* This section registers types, enumerations with their enumerators, public free functions, macros and constants. Member functions and fields of a type are not registered: the interface sketch of the type's owner is their record. An enumeration whose entry lists no enumerators has them in its owner's sketch. Table 8.12-A maps a term to its code form, which for a term of a member names that member (`FxRng::NextUnit`).
  - *Files.* A C++ source file is named for its primary type (R7, `BuildIdentity.h`). A shader name is PascalCase, because its source and compiled files carry it (`AGENTS.md` §2: `<Shader>VS.hlsl`, `<Shader>PS.hlsl`, `<Shader>CS.hlsl`, and `CompiledShader/<Shader>VS.h` with the byte array `g_<Shader>VS`; the permutation files of §8.11).
  - *Words of a term.* A generic word used as a type name takes its kind as a suffix (`TreeObject`, `SpiderCreature`). The one exception is `Turret`, the structure that every turret type shares: the type names a family of structures whose members are the enumerators of `TurretType`, so the suffix would only repeat what `TurretType::Gun` already says. A term whose last word is one of the affixes AGENTS.md R2 bans (`Base`, `Impl`, `Abstract`) takes a different identifier, so the repository checker never needs an exception: the Relic Base is `RelicReceptacle`.
  - *Spelling.* An identifier spells the words of its term in American English (R11), while the term itself and all prose stay British: Iron Defence is `Achievement::IronDefense`, an RGBA colour is `ColorRgba`, a centre is a `Center`. Identifiers of an SDK keep the SDK's spelling (R4).
  - *Constants.* A compile-time constant is `UPPER_CASE` with its unit in the name where it has one (R3, R6): `MAX_TEAMS`, `UPDATE_SLICES_PER_TICK`, `TICK_PERIOD_SECONDS`, `PRODUCT_NAME`. An enumerator is PascalCase (`MapEffect::Snow`).
  - *Originality.* No Species identifier repeats an identifier of the reference implementation.
  - *Data names.* Asset ids, string ids, tuning keys, setting keys, schema ids, action ids, profiling-marker names, the names that a data table holds (pipeline catalogue entries, frame-graph resources, pack and variant ids, and the enumerator strings of a schema, which are in American spelling where they generate an identifier), mesh material names (`team`) and the names of data files and folders are not code identifiers: they keep their spelling and are lower case, except the PascalCase file and folder names of the content root, whose keys are lower-cased (O61, O64, AST-014). The game name is `Species` in namespaces and types and `species` in data names. A schema field whose name generates a C++ field (ADR-062) is spelt in American English, in snake case in JSON (`color_role`) and in camelCase in the generated code (`colorRole`). So is an enumerator string of a schema, which generates an enumerator (`color`, `grayscale`, `parametric_equalizer`); the generator only converts case and rejects a key or string that `AGENTS.md` R11 spells otherwise or whose identifier is a C++ keyword ([data schemas](architecture/components/data_schemas/README.md) DAT-054).
  - *One declaration per type.* A type, enumeration or record is declared once. Where a schema carries it, the declaration that the code generator produces from the schema is the declaration, in the data-schemas document that lists its fields, and every other document cites it with the comment `// generated from <schema id>: <document> §2`; otherwise the owner that table 8.12-B registers declares it, and the documents that use it cite that owner. Table 8.12-B registers each name under the document that declares it.
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
| string id | `<prefix>.<item>` | `achievement`, `aide`, `battle`, `bot`, `building`, `camera`, `cladan`, `crate`, `creature`, `deputy`, `expedition`, `faction`, `game`, `map`, `match`, `mode`, `power`, `presence`, `protection`, `region`, `relation`, `report`, `settlement`, `skirmish`, `team`, `tutorial`, `ui`, `unit`, `view`, `world` | the owning documents; the String-id prefix columns of §8.1 to §8.12 and §8.15; GameAssets §18.1 |
| tuning key | `ref.<prefix>.<parameter>`; `<component prefix>.<topic>.<parameter>` | `ref`; `ter`, `rnd`, `ast`, `vfx`, `aen`, `ui`, `inp`, `cam`, `dat`, `plt`, `bld`, `dbg` | the tuning tables (§4) |
| setting key | `<group>.<setting>` | `audio`, `binding`, `browser`, `camera` (only `camera.automatic`), `debug`, `input`, `interface`, `network`, `player`, `render`, `telemetry`, `window` | the settings table 3.1-A of the data schemas |
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
| flame turret (outcome) | crate outcome | `CrateOutcome::TurretFlame` | `crate.turret_flame` |
| Frenzy | crate outcome | `CrateOutcome::Frenzy` | `crate.frenzy` |
| Fury | crate outcome | `CrateOutcome::Fury` | `crate.fury` |
| gun turret (outcome) | crate outcome | `CrateOutcome::TurretGun` | `crate.turret_gun` |
| Hive (outcome) | crate outcome | `CrateOutcome::Hive` | `crate.hive` |
| meteor shower | crate outcome | `CrateOutcome::MeteorShower` | `crate.meteor_shower` |
| Missile Barrage | crate outcome | `CrateOutcome::MissileBarrage` | `crate.missile_barrage` |
| Reinforcements | crate outcome | `CrateOutcome::Reinforcements` | `crate.reinforcements` |
| Rocket Troops | crate outcome | `CrateOutcome::RocketTroops` | `crate.rocket_troops` |
| rocket turret (outcome) | crate outcome | `CrateOutcome::TurretRocket` | `crate.turret_rocket` |
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
| map definition | UI or text | `MapDefinition` | `map` |
| map id | UI or text | `MapId` | `map` |
| match UI sound source | UI or text | `MatchUi` | — |
| Not available | UI or text | — | `ui.power_slots.not_available` |
| Power slot | UI or text | `PowerSlot` | `ui.power_slots` |
| Power slots | UI or text | `PowerSlots` | `ui.power_slots` |
| Ready | UI or text | — | `ui.power_slots.ready`, `ui.lobby.ready` |
| Skill rating | UI or text | `SkillRating` | `ui.leaderboard.skill_rating` |
| UI core | UI or text | `UiCore` | `ui` |

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

Table 8.12-A maps the terms of the engine and the simulation to their identifiers. The Kind column names the layer (R9): an engine type is declared in `Engine`, a game type in `Species`. A name that an owner's group of table 8.12-B registers is not repeated here: its term follows it there, in brackets.

Table 8.12-A Engine and game types.

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| achievement events | game type | `AchievementEvents` | — |
| action bindings | engine type | `ActionBindings` | — |
| actor | game type | `Actor` | — |
| actor grid | game type | `ActorGrid` | — |
| actor grid cache | game type | `ActorGridCache` | — |
| actor group | game type | `ActorGroup` | — |
| actor kind | game type | `ActorKind` | — |
| actor stat table | game type | `ActorStatTable` | — |
| Airstrike bomb | game type | `ThrownWeaponKind::AirstrikeBomb` | — |
| airstrike flight | game type | `AirstrikeFlight` | — |
| airstrike target marker | game type | `AirstrikeTargetMarker` | — |
| airstrike threshold (map option) | game type | `MapOption::AirstrikeThreshold` | — |
| Allegiance Scrambler effect | game type | `AllegianceScramblerEffect` | — |
| application | game type | `Application` | — |
| asset cache | engine type | `AssetCache` | — |
| attacker reinforcement count (map option) | game type | `MapOption::AttackerReinforcementCount` | — |
| attacker reinforcement power (map option) | game type | `MapOption::AttackerReinforcementPower` | — |
| attacker reinforcement power option (map option) | game type | `MapOption::AttackerReinforcementPowerOption` | — |
| audience group | engine type | `AudienceGroup` | — |
| audio engine | engine type | `AudioEngine` | — |
| barrage missile | game type | `BarrageMissile` | — |
| Barrier Emitter mode | game type | `BarrierMode` | — |
| bot navigation node | game type | `BotNavNode` | — |
| bot objective | game type | `BotObjective` | — |
| bot objective marker | game type | `BotObjectiveMarker` | — |
| bot team | game type | `TeamKind::Bot` | — |
| Builder state | game type | `BuilderState` | — |
| carryable structure | game type | `CarryableStructure` | — |
| cloth flag | game type | `ClothFlag` | — |
| Collector state | game type | `CollectorState` | — |
| control-hint overlay | game type | `ControlHintOverlay` | — |
| Core Device pulse wave | game type | `CorePulseWave` | — |
| crash reporter | engine type | `CrashReporter` | — |
| Crawler group | game type | `CrawlerGroup` | — |
| Crawler trail sample | game type | `CrawlerTrailSample` | — |
| defender reinforcement count (map option) | game type | `MapOption::DefenderReinforcementCount` | — |
| defender reinforcement power (map option) | game type | `MapOption::DefenderReinforcementPower` | — |
| defender reinforcement power option (map option) | game type | `MapOption::DefenderReinforcementPowerOption` | — |
| dust object | game type | `DustVortex` | — |
| dust vortex | game type | — (the vortex that the dust-storm spawner holds; no type of its own) | — |
| dust wind (map effect) | game type | `MapEffect::DustVortices` | — |
| easy bot | game type | `BotDifficulty::Easy` | — |
| emitter id | engine type | `EmitterId` | — |
| energy-grid structure | game type | `GridStructure` | — |
| Essence-orb rain (map effect) | game type | `MapEffect::EssenceOrbRain` | — |
| Essence orb state | game type | `EssenceOrbState` | — |
| fireball | game type | `FireballProjectile` | — |
| firebomb canister | game type | `FirebombCanister` | — |
| formation slot | game type | `FormationSlot` | — |
| frame renderer | engine type | `FrameRenderer` | — |
| front-end controller | game type | `FrontEndController` | — |
| front-end screen | game type | `FrontEndScreen` | — |
| fuel-chain structure | game type | `FuelStructure` | — |
| glyph renderer | engine type | `GlyphRenderer` | — |
| grace period | game type | `GracePeriod` | — |
| grenade | game type | `GrenadeProjectile` | — |
| hard bot | game type | `BotDifficulty::Hard` | — |
| hint overlay | game type | `HintOverlay` | — |
| input router | engine type | `InputRouter` | — |
| input source node | engine type | `InputSourceNode` | — |
| instant Spawn Node capture (map option) | game type | `MapOption::InstantSpawnNodeCapture` | — |
| job queue | engine type | `JobQueue` | — |
| land route graph | game type | `LandRouteGraph` | — |
| laser bolt | game type | `LaserBolt` | — |
| Launch Vessel state | game type | `LaunchVesselState` | — |
| lightning (map effect) | game type | `MapEffect::Lightning` | — |
| lightning strike | game type | `LightningStrike` | — |
| loading view | game type | `LoadingView` | — |
| lobby slot | game type | `LobbySlot` | — |
| local player team | game type | `TeamKind::LocalPlayer` | — |
| map editor | game type | `MapEditor` | — |
| map editor hub page | game type | `MapEditorHubPage` | — |
| map editor window | game type | `MapEditorWindow` | — |
| map effect | game type | `MapEffect` | — |
| map file picker | game type | `MapFilePicker` | — |
| map light | game type | `MapLight` | — |
| map option | game type | `MapOption` | — |
| match camera | game type | `MatchCamera` | — |
| match input pump | game type | `MatchInputPump` | — |
| match statistics | game type | `MatchStatistics` | — |
| match world | game type | `MatchWorld` | — |
| mesh | engine type | `MeshAsset` | — |
| mesh batch | engine type | `MeshRenderer` | — |
| mesh node | engine type | `MeshNode` | — |
| mesh socket | engine type | `MeshSocket` | — |
| meteor | game type | `MeteorRock` | — |
| muzzle flash | game type | `MuzzleFlashEffect` | — |
| no Beam Dish aiming (map option) | game type | `MapOption::NoBeamDishAiming` | — |
| no Catch-up bonus (map option) | game type | `MapOption::NoCatchUpBonus` | — |
| no gun turrets (map option) | game type | `MapOption::NoGunTurrets` | — |
| no-team value | game type | `TeamId::None` | — |
| obstacle grid | game type | `ObstacleGrid` | — |
| options screen | engine type | `OptionsScreen` | — |
| order trail mark | game type | `OrderTrailMark` | — |
| pack file | engine type | `PackFile` | — |
| particle | engine type | `ParticleInstance` | — |
| particle pool | engine type | `ParticlePool` | — |
| particle-type id | engine type | `ParticleTypeId` | — |
| player command | game type | `PlayerCommand` | — |
| power and research levels | game type | `PowerLevels` | — |
| power slots panel | game type | `PowerSlotsPanel` | — |
| presentation generator | engine type | `FxRng` | — |
| presentation generator seed | engine type | `FxRng::Seed` | — |
| presentation random fraction | engine type | `FxRng::NextUnit` | — |
| presentation random integer | engine type | `FxRng::NextUint` | — |
| presentation random signed fraction | engine type | `FxRng::NextSigned` | — |
| procedural foot | engine type | `ProceduralFoot` | — |
| procedural leg | engine type | `ProceduralLeg` | — |
| product name | engine type | `PRODUCT_NAME` | — |
| remote player team | game type | `TeamKind::RemotePlayer` | — |
| render backdrop | engine type | `RenderBackdrop` | — |
| RGBA8 image | engine type | `ImageRgba8` | — |
| rocket | game type | `RocketProjectile` | — |
| route graph | game type | `RouteGraph` | — |
| route graph node | game type | `RouteGraphNode` | — |
| route point | game type | `RoutePoint` | — |
| same-team Beam Dish links (map option) | game type | `MapOption::SameTeamBeamDishLinks` | — |
| sample cache | engine type | `SampleCache` | — |
| scenery mesh | game type | `SceneryMesh` | — |
| sea renderer | engine type | `SeaRenderer` | — |
| sea route graph | game type | `SeaRouteGraph` | — |
| settings store | engine type | `SettingsStore` | — |
| shatter effects | engine type | `ShatterEffects` | — |
| shell casing | game type | `ShellCasing` | — |
| shockwave ring | game type | `ShockwaveRing` | — |
| simulated object | game type | `SimObject` | — |
| simulated-object handle | game type | `SimObjectId` | — |
| simulation generator | game type | `SimRng` | — |
| simulation random fraction | game type | `SimRng::NextUnit` | — |
| simulation random integer | game type | `SimRng::NextUint` | — |
| simulation random signed fraction | game type | `SimRng::NextSigned` | — |
| sky renderer | engine type | `SkyRenderer` | — |
| sliced pool | game type | `SlicedPool` | — |
| smoke column | game type | `SmokeColumn` | — |
| snow (map effect) | game type | `MapEffect::Snow` | — |
| snowflake | game type | `Snowflake` | — |
| sound voice | engine type | `SoundVoice` | — |
| spawn-network edge | game type | `SpawnEdge` | — |
| spawn-network structure | game type | `SpawnStructure` | — |
| special lighting (map effect) | game type | `MapEffect::SpecialLightRig` | — |
| spectator | game type | `TeamKind::Spectator` | — |
| spider gait | game type | `SpiderGait` | — |
| standard bot | game type | `BotDifficulty::Standard` | — |
| starting forces | game type | `StartingForces` | `map.starting_forces` |
| string table | engine type | `StringTable` | — |
| structure | game type | `Structure` | — |
| structure kind | game type | `StructureKind` | — |
| structure port | game type | `StructurePort` | — |
| supply-crate outcome | game type | `CrateOutcome` | — |
| team | game type | `MatchTeam` | — |
| team command state | game type | `TeamCommandState` | — |
| team kind | game type | `TeamKind` | — |
| team slots per match | game type | `MAX_TEAMS` | — |
| teleport | game type | `TeleportStructure` | — |
| terrain height field | engine type | `TerrainField` | — |
| terrain renderer | engine type | `TerrainRenderer` | — |
| thrown weapon | game type | `ThrownWeapon` | — |
| tick clock | game type | `TickClock` | — |
| tick period | game type | `TICK_PERIOD_SECONDS` | — |
| trail point | game type | `TrailPoint` | — |
| Transit Gate reinforcement count (map option) | game type | `MapOption::GateReinforcementCount` | — |
| Transit Gate Transport cap (map option) | game type | `MapOption::GateTransportCap` | — |
| transit orb | game type | `TransitOrb` | — |
| Transport state | game type | `TransportState` | — |
| turret | game type | `Turret` | `building.turret` |
| turret round | game type | `TurretRound` | — |
| turret type | game type | `TurretType` | — |
| UI button | engine type | `UiButton` | — |
| UI button base | engine type | `UiWidget` | — |
| UI input pump | engine type | `UiInputPump` | — |
| UI panel base | engine type | `UiPanel` | — |
| unused slot | game type | `TeamKind::Unused` | — |
| update slices per tick | game type | `UPDATE_SLICES_PER_TICK` | — |
| Visitor-faction position (map option) | game type | `MapOption::VisitorFactionPosition` | — |
| volcano vent | game type | `VolcanoVent` | — |
| waypoint order | game type | `LeaderOrder::Waypoint` | — |
| world cursor | game type | `WorldCursor` | — |
| world markers | game type | `WorldMarkers` | — |
| world surface grid | engine type | `SurfaceGrid` | — |

Table 8.12-B registers the further names of the simulation model and the engine components, grouped by owner: the document that declares them (§8, *One declaration per type*). A group marked "generated" holds the types that the code generator produces from a data schema; the data-schemas document that lists their fields holds their only declaration, and the component that reads them cites it, as a pointer in its own row ("… are the data schemas'"). A name is written once, under its owner; another document that uses it cites the owner. The Namespace column names the layer (R9): a name that knows a game concept is in `Species`, whichever document owns it; the other names of a component are in `Engine`, and those of the simulation model in `Species`. Names in the data names group of an owner are data names, not code identifiers.

Table 8.12-B Names by owner.

| Owner | Namespace | Group | Names |
|---|---|---|---|
| Simulation model (R-SIM) | `Species` | handles and kinds | `SimObjectId::Group` (`Ungrouped`, `StructurePool`, `EssenceOrbPool`, `LaserBoltPool`, `EffectPool`); `TeamId` (`TeamId::None`); `ThrownWeaponKind` (`AirstrikeBomb`) |
| Simulation model (R-SIM) | `Species` | player commands | `PlayerCommand` with `ControlState`, `Select`, `AimBeamDish`, `ToggleBarrierEmitter`, `TargetPower`, `SelectPower`, `CancelPower`, `ScrapActor`, `SelectWorkers`, `OrderWorkers`, `PromoteWorker`, `DemoteLeader`, `OrderUnit`, `ToggleUnit`, `ToggleReady`, `Pause`, `Chat`, `StartMatch`, `SetTeamName`, `SetColor`, `SetAchievementMask`, `SetSlotOpen` (team slot, open flag), `ReportScores`, `SetMatchOption`, `SetMatchOptionElement`, `SetMatchOptionText`, `SetSeasonalTheme`, `SetMatchOptionArray`, `NextMode`, `PreviousMode` |
| Simulation model (R-SIM) | `Species` | routes, formations, space and physics | `RoutePath`, `RouteHop`, `RoutePointKind` (`Position`, `GroundPosition`, `Structure`); `GroupFormation` (`Rectangle`, `Airstrike`); the friendship function `IsFriend`; `IsPathClear`, `WalkCost`, `PushFromObstacles`, `TryEnterTeleport`, `Bounce`, `Explode` |
| Simulation model (R-SIM) | — | data names | the sound source name `airstrike_bomb` |
| Terrain (TER) | `Engine` | types | `SeaMasks` (also read by the renderer), `WaveVertex`, `WaveMeshSource`; the terrain records of a map, `TileMethod` and `FlattenOp` among them, are the data schemas' |
| Terrain (TER) | `Engine` | free functions | `BuildSeaMasks`, `SelectWaveVertexPairs`, `FacetColor` |
| Renderer (RND) | `Engine` | frame, configuration and view | `FrameKind` (`Match`, `MainMenu`, `Loading`, `Editor`), `PosterState`, `TransitionState`, `PresentMode`, `RenderConfig`, `ViewParams`, `FrameInputs`, `StartupError` (`NoCapableAdapter`, `DeviceCreationFailed`, `SwapChainFailed`, `PipelineCreationFailed`, `FrameGraphInvalid`), `ColorRgba` (RGBA colour), `SceneSnapshot` (snapshot interface), `ScissorRect`, `FrameView`, `DirectionalLight`, `LightRig`; the event `DeviceRebuilt` |
| Renderer (RND) | `Engine` | GPU resources (ADR-078) | `GpuResourceFactory`, `GpuResourceId`, `GpuResourceError` (`InvalidDescription`, `OutOfVideoMemory`, `StagingFull`, `DeviceRebuilding`, `UnknownResource`), `GpuTextureDesc`, `GpuBufferDesc`, `GpuSubresourceData` |
| Renderer (RND) | `Engine` | pipeline and frame graph | `BlendState` (`Opaque`, `Alpha`, `Premultiplied`, `Additive`, `Darkening`, `Ink`, `Ghost`), `DepthState`, `CullState`, `SortRule`, `DepthBiasVariant`, `PipelineDescriptor`, `PassTableEntry`, `FrameGraph`, `CompiledFrameGraph`, `FrameGraphError`, `ShaderPermutationId`, `RenderTargetFormats`, `CatalogKey`, `ResourceAccess`, `SettingGate`, `RecordFunction`, `PassContext`, `UploadAllocation` |
| Renderer (RND) | `Engine` | world renderers and batches | `TreeGeometry`, `TreeQuad`, `TreeParameters`, `TerrainColorRamp`, `SeaLook`; `CrowdRenderer`, `DevSphereMesh`; `OrientationMode` (`CameraFacing`, `UprightCard`, `WorldOriented`, `Segment`), `QuadAnchor` (`Center`, `BottomCenter`), `InstanceType`, `InstanceTypeId` |
| Renderer (RND) | `Engine` | shader layouts and options | `FrameConstants`, `DrawConstants`, `MeshInstance`, `QuadInstance`, `SkyLayerInstance`, `WaveVertexPair`, `LitMaterial`, `CombineParams`; the sampler-index constants of the shared header `SAMPLER_LINEAR_CLAMP`, `SAMPLER_LINEAR_WRAP`, `SAMPLER_POINT_CLAMP`, `SAMPLER_POINT_MAGNIFY_CLAMP`; the type aliases of the shared header `Float2`, `Float3`, `Float4`, `Float3x3`, `Float3x4`, `Float4x4`, `Uint32`; `RenderSettings`, `DetailLevel` (`None`, `High`, `Medium`, `Low`), `AntiAliasing`, `QualityPreset`; the shared header `FrameConstants.h` (RND-262) |
| Renderer (RND) | `Engine` | free functions | `PointInView`, `SphereInView`, `GenerateTreeGeometry`, `BuildDevSphere`, `PresetOf`, `ApplyPreset`, `Validated` |
| Renderer (RND) | `Engine` | shaders and permutation macros | `Sky`, `Terrain`, `SeaFlat`, `SeaWavesUpdate`, `SeaWaves`, `MeshLit`, `UnlitTextured`, `UnlitDualTexture`, `UnlitVertexColor`, `ScreenLine`, `DepthCopy`, `Final`, `FullscreenFade`, `ScreenTransition`, `InstanceCull`, `WorldOverlay`; the compute shaders `ParticleEmit`, `ParticleSimulate`, `ParticleCompact` and `ShardUpdate` of the animation and VFX runtime (RND-263); the permutation axes `INVERTED`, `GRID_SOURCE` (`GRID_TEXTURE`, `BARYCENTRIC`), `FOG`, `EDGE_MASK`, `CUTOUT`, `ALPHA_ONE`, `COMBINE` (`PRODUCT`, `TEXTURE_COLOR`), `CLIP_PLANES`, `SAMPLES`, `HDR`, `EDGE_STRIP` and `SOURCE` (`VERTEX_COLOR`, `TEXTURED`, `MESH`); file names as §8.11 |
| Renderer (RND) | `Species` | game-object renderers, overlays and look checks (game concepts, R9) | `PresentationSnapshot` (the game's presentation snapshot of DEC-222, which implements `SceneSnapshot`), `MatchSetupView`; `StructureRenderer`, `ActorRenderer`, `EffectRenderer`, `RouteLineRenderer`, `WorldMarkerRenderer`, `WorldCursorRenderer`, `PowerSlotsPanelRenderer`, `MatchHudRenderer`, `MatchHudView`, `RoutePreview`, `RouteHandle`; `LookScene` (`GlowStack`, `DarkeningStack`, `InkText`, `TeamPalette`, `FogRamp`, `OverBrightFacet`, `CrowdAtDistance`, `CardToBillboard`, `InvertedStyle`, `SeasonalTheme`), `LookCheck`, `LookError`; the function `RenderLookScene` |
| Renderer (RND) | — | data names | catalogue entries `sky`, `terrain`, `terrain_inverted`, `sea_flat`, `sea_flat_inverted`, `sea_waves`, `mesh_lit_fog`, `port_pad`, `glow_additive_fog`, `glow_additive_fog_nodepth`, `alpha_fog`, `dual_alpha_fog`, `dual_additive_clip_fog`, `line_additive_fog`, `fade`, `transition`, `group_mesh_lit`, `trooper_mesh_lit`, `actor_mesh_lit`, `actor_flicker_additive`, `actor_flicker_darkening`, `actor_strip_lit`, `actor_glow`, `actor_glow_nodepth`, `actor_ink_nodepth`, `actor_ink`, `actor_darkening`, `actor_beam_alpha`, `actor_beam_additive`, `blob_shadow`, `crawler_trail`, `crawler_trail_rogue`, `worker_card`, `worker_ground_shadow`, `worker_glow`, `effect_additive`, `effect_additive_nodepth`, `effect_gradient_nodepth`, `laser_bolt`, `route_line`, `overlay_alpha`, `overlay_additive`, `overlay_ink`, `overlay_gradient_alpha`, `cursor_ring`, `cursor_area`, `cursor_dots`, `cursor_ghost`; frame-graph resources `scene_colour`, `scene_depth`, `scene_resolved`, `scene_depth_single`, `final_image`, `captured_frame`, `screenshot_readback`, `terrain_heights`, `back_buffer`; combine modes `product`, `texture_color` |
| Asset pipeline and resources (AST) | `Engine` | identity and mounting | `AssetId`, `VariantId`, `AssetKey`, `ContentKey`, `Sha256Digest`, `AssetError`; `MountOptions`, `MountOverride`, `MountManifest`, `MountSet`, `EntryRef`, `PackEntry` |
| Asset pipeline and resources (AST) | `Engine` | content types | `ContentType`, `PackCodec` (`Stored`, `XpressHuffman`); the import records are the data schemas' |
| Asset pipeline and resources (AST) | `Engine` | reading, caching and events | `ReadPriority`, `ContentReader`, `LoadState`, `TextureSize`, `LoadGroup`, `TextureHandle`, `MeshHandle`, `RuntimeTextureId`, `ImageProducer`; `AssetLoaded`, `AssetFailed`, `LoadGroupProgress`, `LoadGroupCompleted`, `AssetReloaded` |
| Asset pipeline and resources (AST) | — | data names | pack ids `startup`, `locale`, `core`, `audio`, `update_<nn>`; the mesh material name `team`, the socket prefix `socket_`; variant ids `level0`, `point`, `shadow`, `trail` |
| Animation and VFX (VFX) | `Engine` | types | `PresentationClock` (presentation clock), `PresentationTime`; `ShatterPayload`; `ParticleBlend`, `TerrainRule`, `ParticleEmission`, `ParticleTypeParams`, `PoolCounts`; `ScreenEffects`, `ShakeOffset`; `ShatterExplosion`, `FootState`, `LegTuning`; GPU structs `ParticleSlot`, `ParticleType`, `Shard`, `Tumbler`, in the shared header `ParticleSlot.h` |
| Animation and VFX (VFX) | `Species` | types (game concepts, R9) | `EffectKind`, `EffectEvent`, `EffectEventId`; `ParticleKind`; `ExplainerPlayer`, `ExplainerLayout`, `ExplainerTeamColors`; `VfxRuntime`, `ProceduralAnimator` |
| Animation and VFX (VFX) | `Species` | `ParticleKind` enumerators | `RocketTrail`, `ExplosionCore`, `ExplosionDebris`, `MuzzleFlare`, `Fire`, `ControlFlash`, `Spark`, `BlueSpark`, `Casing`, `MissileTrail`, `MissileFire`, `WorkerFire`, `Leaf`, `Sickness`, `SpeedBoostTrail`, `FireballFlame`, `Volcano` |
| Animation and VFX (VFX) | `Species` | `EffectKind` enumerators | `Explosion`, `RocketBurst`, `StructureDestroyed`, `LaserHitStructure`, `RoundHitStructure`, `RoundHitTerrain`, `ThrownWeaponTrail`, `AirstrikeMarkerSmoke`, `RocketFlame`, `FireballLaunched`, `FirebombBurst`, `LightningStrike`, `SwarmHit`, `TurretCooling`, `TurretShot`, `TrooperShot`, `BarrierSpark`, `ControlGained`, `ControlLost`, `LeaderPromoted`, `LeaderDemoted`, `CrowdOrder`, `LeaderOrder`, `LeaderFormUp`, `OrderMarkerStep`, `WorkerBurning`, `WorkerSpeedBoost`, `WorkerSick`, `PowerReachedWorker`, `WorkerOpeningCrate`, `WorkerAbducted`, `ActorHit`, `Bite`, `TransportExhaust`, `CollectorEngine`, `AbductorEngine`, `MeteorFire`, `TreeBurning`, `TreeLeaf`, `SpitterPlantBurning`, `SmokeColumn`, `VolcanoEruption`, `FuelPumpStroke`, `FuelDepotBoarding`, `VesselBooster`, `VesselWrecked`, `CameraShake`, `Shatter` |
| Animation and VFX (VFX) | — | data names | catalogue entries `orb_ink`, `orb_additive`, `shard_lit`, `particle_darkening`, `particle_additive`, `cloth_cells`, `cloth_cells_fog`, `cloth_pole`, `cloth_pole_fog`, `cloth_pointer`, `cloth_overlay`; the depth-bias variant `bias_flag_pointer` |
| Audio engine (AEN) | `Engine` | types | `AudioSeconds` (the alias of audio time in s), `SourceId`, `EventId`, `BankId`, `RequestId`, `OperationSetId`, `SoundVoiceId`, `AudioSettings`, `SoundDataSet`, `BusGraphDef`, `SoundGroupDef`, `SampleBank`, `AudioStream`, `SourceVoiceSlot`, `VoiceFormat`, `SendLevel`, `VoiceBackend`, `AudioBusGraph`, `CommandWriter`, `TriggerCommand`, `StopCommand`, `EmitterUpdate`, `ListenerUpdate`, `MatchState`, `AudioResult`; the constant `AUDIO_BUS_COUNT`; `EffectBinding`, `SoundParam`, `SoundParamKind`, `SoundParamUpdate`, `SoundLink`, `DspFilterKind`, `SoundLayerDef`, `SoundEventDef` and `SoundSourceDef` are the data schemas' |
| Audio engine (AEN) | `Engine` | enumerations | `SoundPosition` (`Screen2D`, `WorldFixed`, `Attached`), `SoundSharing`, `SoundLoop`, `SoundAudience` (`All`, `OwnGroup`, `OtherGroup`, `NoGroup`), `AudioBus`, `EnvelopeStage`, `VoiceState`, `StopMatch`, `MatchPhase` (`NoMatch`, `Countdown`, `Running`, `Ended`), `ResultKind` (`Count`, `Playing`, `DeviceReady`, `InterfaceReady`, `MatchReady`, `EmitterTracked`, `EmitterUntracked`) |
| Audio engine (AEN) | `Engine` | Species XAPOs | `StereoEchoFx`, `StudioReverbFx`, `ResonantLowPassFx`, `BitCrusherFx`, `GargleFx`, `EchoFx`, `SimpleReverbFx` |
| UI framework (UI) | `Engine` | core, geometry and input | `PanelId`, `WidgetId`, `PointF`, `SizeF`, `RectF`; `PointerEvent`, `PointerEventKind` (`PrimaryDown`, `PrimaryUp`, `AltDown`), `TextInput`, `EditCommand`, `Callback`; `UiWindow` (UI window); `InputMode` is the input component's |
| UI framework (UI) | `Engine` | widgets and value binding | `UiBorderlessButton`, `UiCloseButton`, `UiInsetPanel`, `UiTextField`, `UiIncrementGlyph`, `UiScrollBar`, `UiCheckBox`, `UiDropDown`, `UiDropDownList`, `UiDropDownOption`, `UiMessageDialog`, `UiChatEntry`, `UiChatField`; `UiValueBinding`, `ValueType`; `PageButton`, `PageBackButton`, `ScreenCloseButton`, `PageTitle`, `PageDropDown`, `PageDropDownList`, `PageDropDownOption`, `PageTextField`, `PageIncrementGlyph`, `PageCheckBox` |
| UI framework (UI) | `Engine` | layout, drawing and text | `PageGrid`, `UiScaleState`, `UiBlend`, `UiVertex`, `FontRole` (`Caption`, `Body`, `Title`), `TextPass`, `TextAlign`, `TextStyle`, `DrawContext`; `TextView`, `MessageArguments`, `LanguageTag`, `StringId`; `AssetId` is the asset pipeline's |
| UI framework (UI) | `Engine` | free functions | `ComputePageGrid`, `LargeMenuScale`, `ToolWindowScale`, `FinalTextSize` |
| UI framework (UI) | `Engine` | page navigation and tool windows | `NavOp` (`Push`, `Pop`, `Replace`, `UnwindTo`, `Rebuild`), `TransitionDirection`, `EnterOutcome` (`Built`, `Waiting`, `Redirect`), `FillBoxes`, `WidgetBuilder`, `ToolWindow`, `ToolPlacement`; `OpenToolOnce`, `CloseTool`, `ToolString` |
| UI framework (UI) | `Species` | pages and HUD panels (game concepts, R9) | `PageId` (`Title`, `Tutorials`, `HostOrJoin`, `ModeSelect`, `MapSelect`, `MatchSetup`, `AdvancedOptions`, `PlayerOptions`, `SessionBrowser`, `BrowserFilters`, `Password`, `Connecting`, `WaitForStart`, `Error`, `Credits`, `QuickMatch`, `Achievements`, `LeaderboardSelect`, `Leaderboard`), `NavRequest`, `EnterResult`, `FrontEndPage`, `PageHost`, `PageContext`, `FrontEndViewModel`, `HudPanel`, `HudContext`; the events `PageEntered`, `PageLeft`, `NavigationRejected` |
| UI framework (UI) | `Engine` | front-end widget kinds | `ScrollingText`, `MapDescription`, `OptionDetails`, `StaticImage`, `MapThumbnail`, `HelpPicture`, `MapRow`, `MapListHeader`, `SessionRow`, `SessionListHeader`, `KickControl`, `PrimaryButton`, `MapNameLabel`, `ChatLog`, `ConnectionStatus`, `TextLabel` |
| UI framework (UI) | `Species` | front-end widget kinds (game concepts, R9) | `ModeDescription`, `ModePicture`, `ColorPickerRow`, `ModeRow`, `LobbySlotRow`, `SlotOpenControl` |
| UI framework (UI) | — | data names | the action ids `action.menu.left`, `action.menu.right`, `action.menu.up`, `action.menu.down`, `action.menu.activate`, `action.menu.front_end_activate`, `action.menu.close`, `action.menu.dismiss`, `action.menu.scroll_speedup`; the string ids `ui.dialog.close`, `ui.dialog.error`, `ui.dialog.success` |
| Input (INP) | `Engine` | types | `InputEvent` with its nested `InputEvent::Kind` (`Press`, `Release`, `Axis`, `Connect`, `Disconnect`), `ActionValue`, `FrameGates`, `PointerState`, `InputPointer`, `DeviceBackEnd`, `GamepadBackEnd`, `KeyboardMouseBackEnd`, `InputEventQueue`, `DeviceState`, `BindingStateTable`, `PadAssignment`, `LoadDiagnostic`, `LoadReport`, `BindingSources`, `BindingDescription`, `DeviceClass`, `DeviceClassMask`, `DeviceId`, `LocalPlayerId`, `ActionId`, `KeyCode`, `MouseControl`, `PadControl`, `ValueKind`, `Condition`, `InputMode` (also used by the UI framework) |
| Input (INP) | `Engine` | binding nodes and helpers | `BindingNode`, `AllNode`, `ChordNode`, `NotNode`, `AliasNode`, `LatchNode`, `IdleNode`, `SettingNode`, `ValueNode`; `ActionAxis`, `StickAxis`, `PriorityAxis`, `ActionTrigger` |
| Input (INP) | — | data names | the value-node tokens `always_true` and `feature` (INP-143); the control names of the input component's tables 3.2-A to 3.2-C; the layout tags `de-DE`, `en-US`, `fr-FR`, `en-GB`, `en-CA`; the binding-set layer kind `development` (INP-094); its menu actions are the nine of the UI framework's data names |
| Camera and sequences (CAM) | `Engine` | types | `CameraPose`, `MapCameraData`, `CameraFrameOutput`; `TimelineOp` (`Wait`, `WaitCamera`, `WaitFade`, `WaitCondition`, `CameraCut`, `CameraMove`, `CameraApproach`, `CameraFocus`, `CameraAnimation`, `CameraFov`, `CameraRelease`, `Caption`, `Fade`, `Overlay`, `Sound`, `PermitAbort`), `TimelineStatement`, `PresentationTimeline`, `StatementArgs`; the camera records of a map and `SourceLocation` are the data schemas' |
| Camera and sequences (CAM) | `Species` | match camera and timeline player (game concepts, R9) | `CameraMode` (`Static`, `ScriptedMove`, `FreeMovement`, `Tracking`, `BeamDishAim`, `TurretAim`, `StructureFocus`, `DeveloperFly`), `CameraFrameInputs`, `CameraState`, `TimelinePlayer`, `TimelineFrameInputs`; `MatchCamera` is in table 8.12-A |
| Camera and sequences (CAM) | — | data names | the action ids `action.debug.camera_fast`, `action.debug.camera_slow`, `action.debug.camera_rotate`, `action.sequence.skip` |
| Data schemas (DAT) | `Engine` | types | `FieldType`, `SimPrecision`, `Presence`, `SchemaVersion`, `FieldSpec`, `RecordSchema`, `SchemaRegistry`, `SourceLocation`, `Severity`, `DiagnosticClass` (the 21 classes of DAT table 6-A, in its order: `Syntax`, `Encoding`, `Depth`, `DuplicateKey`, `DuplicateId`, `UnknownField`, `UnknownValue`, `MissingField`, `DerivedField`, `WrongType`, `OutOfRange`, `TooLong`, `ZeroDirection`, `PositionalTable`, `MissingAsset`, `MissingRecord`, `MissingString`, `ReferenceCycle`, `FamilyRule`, `Version`, `ContentWarning`), `Diagnostic`, `JsonDocument`, `ReferenceIndex`, `RecordValue`, `CookedRecord`, `AuthoringParser`, `RecordValidator`, `RecordCooker`, `CanonicalWriter`, `SchemaMigration`, `RecordView`, `LoadError` (`WrongSchema`, `WrongVersion`, `HashMismatch`, `Truncated`, `Malformed`; also returned by the map loader and the explainer player), `RecordLoader`; `SettingType`, `ApplyScope` (`Immediate`, `ApplyScope::Renderer`, `None`), `DefaultValue`, `ValueMap`, `SettingSpec`, `SettingsDefaults`, `SettingsFileContent`, `SettingsFile`, `CookedStringTable`, `StringTableBuilder`; `Limit` with `Limit::Form` (`Absent`, `None`, `Value`, `Unlimited`); `EngineAssetTable`, `EditionManifest`, `ImportRecordReader` |
| Data schemas (DAT) | `Engine` | import records (generated) | `TextureKind`, `ColorSpace`, `AlphaMode`, `MipPolicy`, `SamplerId` (`LinearClamp`, `LinearWrap`, `PointClamp`, `PointMagnifyClamp`), `Derivation` (`BlurredShadow`, `StackedCopies`, `Grayscale`, `ToColor`, `ReplaceColor`), `DerivationParameters`, `DerivedVariant`, `TextureView`, `TextureImportRecord`, `MeshRole` (`Render`, `Collision`; read by the asset pipeline's cookers), `TintMarking`, `SmoothPrimitive`, `MeshImportRecord`, `AudioImportRecord` |
| Data schemas (DAT) | `Engine` | sound records (generated) | `DspFilterKind` (`StereoEcho`, `RoomReverb`, `ParametricEqualizer`, `StudioReverb`, `ResonantLowPass`, `BitCrusher`, `Gargle`, `Echo`, `SimpleReverb`), `DspParamDesc`, `DspFilterDef` (DSP filter definition); `SoundParamKind` (`Fixed`, `Random`, `Linked`), `SoundParamUpdate` (`Continuous`, `PerNote`), `SoundLink` (`None`, `Height`, `PosX`, `Altitude`, `PosZ`, `Speed`, `CameraDistance`, `ObjectAge`, `GroupsNearby`, `TimeRemaining`, `ObjectsNearby`), `SoundParam` (sound parameter), `EffectBinding` (also read by the audio engine), `SoundLayerDef`, `SoundEventDef` (sound event definition), `SoundSourceDef` (sound source definition) |
| Data schemas (DAT) | `Engine` | terrain and camera records of a map (generated) | `TileMethod` (`Average`, `RandomPair`, `CopyOne`), `FlattenOp` (`Absolute`, `Add`, `Subtract`, `Smooth`, `Subtract2`), `TerrainDefinition` (terrain definition), `TerrainTile` (terrain tile), `TerrainFlattenArea` (terrain flatten area); `CameraTransition` (`Move`, `Cut`), `CameraView`, `CameraAnimationStep`, `CameraAnimationDef` |
| Data schemas (DAT) | `Species` | map, structure and game records (game concepts, R9) | `MapHeaderView`, `MapDefinitionView`, `MapLoader`, `StructureRecord`, `StructureLink`, `KindFields`; `MapDifficulty` (`Basic`, `Intermediate`, `Advanced`), `SiegeSide` (`Defending`, `Attacking`), `CoopGroup` (`None`, `One`, `Two`), `StartPosition`, `StartingForcesRecord`, `RouteRecord`; `ActorStats`, `OutcomeClass` (`Basic`, `Advanced`), `Delivery` (`Power`, `AtCrate`, `Global`, `OpeningTeam`, `None`), `CrateOutcomeEntry`; `ExplainerKey`, `ExplainerCanvas`, `ExplainerLoop`, `ExplainerObject`. `MapOption`, `MapEffect` and `MapLight` are in table 8.12-A, `MatchOptions` in §8.6 and `ExplainerAnimation` in §8.10 |
| Data schemas (DAT) | `Engine` | soak and telemetry records (generated) | `SoakProgression` (`Linear`, `Random`, `Sequence`), `SoakEntryFailure`, `SoakParticipantRole`, `SoakSequenceItem`, `SoakTestMap`, `SoakConfigurationRecord`, `SoakMatchRow`, `SoakPassSummary`, `TelemetrySessionRecord`, `SoakMatchTableWriter` |
| Data schemas (DAT) | `Engine` | free functions | `WriteTelemetrySession`, `ReadQueuedTelemetrySession` |
| Data schemas (DAT) | — | data names | the schema ids `maps.definition`, `maps.index`, `mode.options`, `settings.defaults`, `input.binding_set`, `input.keyboard_layouts`, `text.string_table`, `actor.stats`, `crate.outcomes`, `sound.sources`, `sound.groups`, `sound.effect_kinds`, `geometry.mesh`, `geometry.collision`, `geometry.sim_record`, `panels.explainer`, `effects.definition`, `camera.timeline`, `interface.page_layout`, `interface.palette`, `interface.text_style`, `interface.font_roles`, `import.texture`, `import.mesh`, `import.audio`, `assets.engine_table`, `assets.mount_manifest`, `assets.content_manifest`, `edition.manifest`, `test.soak_configuration`, `test.soak_report`, `telemetry.session`; the setting-key groups of table 8-A |
| Platform layer (PLT) | `Engine` | clock and window | `PlatformClock`, `TimeSource`, `TimeSourceKind`, `ManualTimeSource`, `Nanoseconds`; `AppWindow` (application window; the renderer draws into it), `WindowMode` (window mode; the renderer reads it), `Permanence`, `WindowPlacement`, `WindowMessage`, `WindowInputSink`; `WindowEvent`, the variant of the event structs `Resized`, `DpiChanged`, `FocusChanged`, `Minimized`, `Restored`, `ModeChanged`, `MonitorChanged`, `DisplayConfigurationChanged`, `KeyboardLayoutChanged` and `EndOfSession`; `MonitorInfo`, `DesktopMode`; `PointPx`, `SizePx`, `RectPx`, `NativeWindowHandle` |
| Platform layer (PLT) | `Engine` | files, lifecycle, threads and services | `PlatformPaths`, `UserFolder`, `MappedFile`, `FileError`, `PlatformError`, `SettingValue`; `StartupOptions`, `MessageSeverity`, `MessageArgument`, `LifecycleHooks`; `PlatformLogSink`, `PlatformLogLevel` (`Debug`, `Info`, `Warning`, `Error`); `PlatformThread`, `CrashContext`, `CrashFileKind`, `SystemSummary`; `HttpsClient`, `HttpsRequest`, `HttpsResponse`, `HttpsError`, `AudioEndpointChange`, `ScreenshotImage`, `ScreenshotWriter` |
| Platform layer (PLT) | `Engine` | free functions | `EnumerateMonitors`, `ReadWholeFile`, `MapReadOnly`, `ReplaceFileAtomically`, `ToUtf16`, `ToUtf8`, `InstallFatalHandlers`, `CheckOperatingSystem`, `ParseCommandLine`, `ShowPlatformMessage`, `ExitAfterTeardown`, `EndWithMessage`, `FailFast`, `SetPlatformLogSink`, `IsDefaultFloatEnvironment`, `ApplyDefaultFloatEnvironment`, `System`, `ClipboardText`, `SetClipboardText`, `OpenWebAddress`, `SetAudioEndpointListener` |
| Platform layer (PLT) | `Engine` | maths library (§4.17) | `Vec2`, `Vec3` (3-vector), `Mat3x4` (3x4 matrix), `Mat4`, `Ray`, `RayTriangleHit`, `Grid2D` (2-D grid; its index rules are terrain TER-039 and TER-072); the free functions `Normalize`, `NormalizeHorizontal`, `WithLength`, `NearlyEqual`, `TransformPoint`, `TransformDirection`, `Inverse`, `IntersectTriangle` |
| Platform layer (PLT) | — | files | the header `WindowsHeaders.h`, the one header that includes `<windows.h>` and defines the Windows macro family (PLT-132) |
| Platform layer (PLT) | — | data names | the settings file `settings.txt`; the user-data folders `maps\` and `diagnostics\` |
| Build tooling and distribution (BLD) | `Engine` | types, files and macros | `BuildProduct` (`BuildProduct::Client`, `BuildProduct::Server`, `BuildProduct::ContentBuildTool`), `BuildConfiguration` (`Debug`, `Release`), `SemanticVersion`, `BuildIdentity`, `EditionId`, `ContentBuild`, `ContentBuildCommand` (`Build`, `Validate`, `Loose`, `StringsExport`, `StringsImport`), `ContentBuildStage` (`ResolveEditions`, `ReadInputs`, `CheckSources`, `Cook`, `Derive`, `Placeholders`, `CookRecordsAndGenerate`, `CheckReferences`, `SelectEditions`, `Localization`, `MapIdentities`, `WritePacks`, `WriteManifestsAndReports`, in stage order), `ContentBuildError`, `MapIdentityEntry`, `LanguageFindings`, `RunOptions`, `RunResult`; the generated header `BuildIdentityFields.h` (the build-identity constants, BLD-020; `BuildIdentity.h` is the hand-written header of `BuildIdentity`); the development switch `ENGINE_DEVELOPMENT` (ADR-056) |
| Build tooling and distribution (BLD) | `Engine` | free functions | `CurrentBuildIdentity`, `DisplayVersion`, `FullIdentityText`, `VersionResourceQuad`, `ChannelTag` |
| Build tooling and distribution (BLD) | — | data names | `Species.exe`, `SpeciesServer.exe`, `SpeciesContentBuild.exe`; `species_version.json`, `build_identity.json`, `channel_tag.txt`, `language_report.json`, `validation_report.json`, `placeholder_report.json`, `localisation_report.json`, `map_identities.json`; the product values `client`, `server` and `contentbuild` of the build identity; the edition ids `full`, `full_server`, `development`, `soak`; the feature flags `modes`, `crate_classes`, `tutorial`, `play_local_network`, `play_internet`, `map_editor`; the language groups `all`, `efigs`; the start-up switches `--write-build-identity`, `--verify-content` |
| Diagnostics, test and performance (DBG) | `Engine` | types and macros | `LogLevel`, `LogCategory`, `LogStats`, `BlackBox`, `ProfileTree`, `ProfileNodeStats`, `ProfileNodeId`, `CounterProvider`, `CounterSink`, `DevDrawQueue`, `DevDrawItem`, `DevDrawItemId`, `SoakRunner`, `SoakEntry`, `SoakHost`, `SoakReport`; `ENGINE_ASSERT`, `ENGINE_CHECK`, `ENGINE_CHECK_INDEX`, `ENGINE_PROFILE_SCOPE`, `ENGINE_PROFILE_GPU_SCOPE`; the soak configuration records and `TelemetrySessionRecord` are the data schemas' |
| Diagnostics, test and performance (DBG) | `Engine` | free functions | `Log`, `LogHexDump`, `LogValueDump`, `SetCategoryLevel`, `GetLogStats`, `FlushLogSynchronously`, `CheckFailed`, `FatalExit`, `RecentFrameTimesMs`, `RegisterCounterProvider`, `UnregisterCounterProvider`, `SubmitTelemetry` |
| Diagnostics, test and performance (DBG) | `Species` | debug commands (game concepts, R9) | `DebugCommand`, `DebugCommandKind` (`KillAllEnemies`, `SpawnWorkers`, `SpawnTransport`, `SpawnCrawlers`, `SpawnEssenceOrbs`, `AllowArbitraryPlacement`, `SpawnAtStructurePorts`, `SpawnSupplyCrate`, in button order) |
| Diagnostics, test and performance (DBG) | — | data names | the action ids `action.debug.command_window`, `action.debug.drop_packet`, `action.debug.special_lighting_on`, `action.debug.special_lighting_off`, `action.debug.soak_rendering`, `action.debug.overlay`, `action.debug.profile_view`; the setting keys `debug.command_history`, `debug.capture_frame_rate`, `debug.soak_session_name`; the marker areas of table 8-A; the string ids `ui.dev.*` (among them `ui.dev.command.dump_bindings`) and `ui.match.cheats_used` |

### 8.13 Names used only in Dropped decision entries

These name content that the reference library does not specify: Advisor, Hatchery, Rift, Rift Horror, Rift tentacle, Summoner, Tri-walker, Vehicle Duel.

### 8.14 Generic and component terms

These terms are ordinary words, or names that one owner defines in prose. They need no identifier of their own, and a document uses them in the owner's sense; the glossary defines each one that a reader needs.

| Owner | Terms |
|---|---|
| generic terms of the reference documents | activation delay, advanced outcome, alliance, attackers and defenders, bad outcome, basic outcome, basic outcomes only (the Supply-crate match option; always this name), black box, bot difficulty, burst and pause cycle, capture, cheats, co-op, content build, countdown, crate drop period, crate placement, crowd order, direct control, draw, draw value, effect, elimination, excluded outcomes, explosion, formation, friend (friendship test), front end, fuel, game speed, game time, good outcome, host, keyboard layout, lobby, local team, main loop, match, match setup, mode second, music channel, network statistics window, no team, one-minute warning, operator, outside height, page, participant, passengers, pause, player command, population-locked, power-slot message, presentation generator, rank announcement, ready (ready-up), reinforcement count, reinforcement interval, reinforcement wave, replay, results screen, route, score, score mode, server browser, session (private local session, hosted session), settings file, simulation generator, simulation state, soak test, start position, starting forces, starting powers, starting units, string id, structure push, structure-detail level, team kind, team slot, tick, time limit, time-up, Transport cap, turret cap, turret frequency, tutorial, ungrouped actors, used slot (used team) |
| Simulation model (R-SIM) | Airstrike bomb, base actor advance, command frame, command sequencer (sequencer), dust object (one `DustVortex` effect), extrapolation time, handle group, held (world held), object serial, obstacle push, presentation frame delta, presentation frame time, smooth presentation time, tick-take time, valid-spawn-position search, the Visitor faction's pulse wave (the Core Device pulse wave, `CorePulseWave`) |
| Terrain (TER) | conformance generator; the notation r32, pow32, exp32, log32, sqrt32, sin32, abs32, log2r and ceilLog2, which names arithmetic and is no identifier |
| Renderer (RND) | blight timer, blight tree, bottom-anchored billboard, burn level, corrupted tree, Crawler trail texture, development sphere, empty compass, entity detail, essence tree, ghost model, high-detail factor, inverted style, look scene, match message, object lighting, ownership lights, poster resolution, quality preset (high, medium, low, custom), route fade record, route preview walkable flag, screen transition, sea detail, sky detail, special lighting, structure detail (the setting that `RenderSettings::decorationDetail` holds), tree detail, Worker distance scale factor (the setting that `RenderSettings::crowdCardDistanceScale` holds) |
| Asset pipeline and resources (AST) | asset index, collision asset, content manifest, derived variant, engine asset table, import record, load group, loose-content directory, mount manifest, placeholder, residency list, run-time texture, simulation data record, view |
| Audio engine (AEN) | audio clock, effect bus, insert routing, interface bank, match bank, real instance, send routing, silent device, start time, state hold, steal margin, stream head, virtual instance, voice pool |
| UI framework (UI) | back action, back handler, base press, birth cascade, captured widget, close box, draw point, enter rule, fill box, font role, highlight beam, hook page, horizontal scale, horizontal spacing factor, hover-highlight mode, HUD panel, increment glyph, inset panel, leave hook, match-setup sync, menu size, navigation cursor, navigation order, page catalogue, page host, page list, page stack, palette token, pop, screen view-model, session-discovery listener, text row convention, text-type field, tool window |
| Camera and sequences (CAM) | approach, camera clock, camera trigger, developer fly mode, level up vector, presentation timeline, scripted move, structure focus |
| Data schemas (DAT) | absence-meaningful field, canonical writer, caption marker, derived field, limit form, logical field type, record type, schema id, simulation decimal |
| Platform layer (PLT) | composition root, crash-context record, fatal path, fixed-step source, manual source, platform clock, platform time, log sink, pre-UI message, run id, system summary, user-data root |
| Build tooling and distribution (BLD) | build identity, channel tag, channel wrapper, configuration, edition, edition manifest, edition marked for release, localisation report, map-identity listing, permitted maps, product, reference feature set, release procedure, repair check, symbol store, tool project, translator pack |
| Diagnostics, test and performance (DBG) | cheats-used marker, counter provider, debug command, design population, development drawing queue, development overlay, profile view, reference test machine, single-process pass, soak entry, soak host, stress population |

### 8.15 World game

The terms of the Species world game, which [the Species design](gdd/00_vision_pillars_and_core_loops.md) §2.12 defines (Table 3; PIL-090, PIL-091) and the owner confirmed at gate A (O37, with O32 for Cladan). The rows of area A1 (biome, border area, crossing, home cell, native AI settlement, region name, resource site, set piece, stand-in AI settlement, world day and world template) are defined in [world and regions](gdd/01_world_and_regions/README.md) and were registered when A1 was finalised. The Kind column gives the kind of that table; the glossary defines each term. The world game reuses some reference terms with a changed use (a settlement's player is its steward, never its owner; the server's runner is the world host, never an operator; the protection period is a respite, never a shield; a world's span is its lifetime, never a season; construction is done by Workers, never by builders; a people is a Cladan): table 4 of the Species design lists them.

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| abstract fidelity | simulation | `Fidelity::Modeled` | — |
| AI settlement | world game | `SettlementControl::Ai`, beside `SettlementControl::Steward` | `settlement.ai` |
| aide | delegation | `Aide` | `aide` |
| anger | relations | `Anger` | `relation.anger` |
| attack | world game | `AttackDeclaration` | `battle.attack` |
| attack cost | protection | `AttackCost` | `protection.attack_cost` |
| away | session | `StewardPresence::Away` | `presence.away` |
| away report | delegation | `AwayReport` | `report.away` |
| battle | battle | `Battle` | `battle` |
| battle result | battle | `BattleResult` | `battle.result` |
| battlefield | battle | `Battlefield` | `battle.field` |
| biome | world game | `Biome` | `region.biome` |
| border area | world game | `BorderArea` | `region.border_area` |
| check-in | session | — | `presence.check_in` |
| Cladan | relations | `Cladan` | `cladan` |
| claim | relations | `Claim` | `relation.claim` |
| core | world game | `SettlementCore` | `settlement.core` |
| crossing | world object | `RegionCrossing` | `region.crossing` |
| Crystal | world game | `ResourceKind::Crystal` | `resource.crystal` |
| dedicated server | server | `ServerMode::Dedicated` | — |
| defender's advantage | protection | `DefenderAdvantage` | `protection.defender_advantage` |
| deputy | delegation | `Deputy` | `deputy` |
| dormant | world game | `SettlementState::Dormant` | `settlement.dormant` |
| dwelling | world object | `Dwelling` | `building.dwelling` |
| expedition | world game | `Expedition` | `expedition` |
| founding | world game | — (a player command; C1 names it) | `settlement.founding` |
| frozen | world game | `WorldState::Frozen` | `world.frozen` |
| full fidelity | simulation | `Fidelity::Full` | — |
| grievance | relations | `Grievance` | `relation.grievance` |
| home cell | world game | — | — |
| integrated server | server | `ServerMode::Integrated` | — |
| loss cap | protection | `LossCap` | `protection.loss_cap` |
| native AI settlement | world game | `AiSettlementRole::Native`, beside `AiSettlementRole::StandIn` | `settlement.ai.native` |
| newcomer protection | protection | `NewcomerProtection` | `protection.newcomer` |
| presence | session | `StewardPresence` (not `Presence`, the field presence of the data schemas) | `presence` |
| present | session | `StewardPresence::Present` | `presence.present` |
| protection rules | protection | — | `protection` |
| region | world game | `Region` | `region` |
| region name | world game | — (string ids `region.name.<nnn>`) | `region.name` |
| relations | relations | `Relation` | `relation` |
| renown | relations | `Renown` | `world.renown` |
| resource site | world object | `ResourceSite` | `region.resource` |
| respite | protection | `Respite` | `protection.respite` |
| set piece | world object | `SetPiece` | — |
| settlement | world game | `Settlement` | `settlement` |
| settlement site | world object | `SettlementSite` | `settlement.site` |
| settlement view | view | `SettlementView` | `view.settlement` |
| skirmish | form of play | `Skirmish` | `skirmish` |
| stand-in AI settlement | world game | `AiSettlementRole::StandIn` | `settlement.ai.stand_in` |
| standing order | delegation | `StandingOrder` | `deputy.order` |
| steward | role | `Steward` | `settlement.steward` |
| storehouse | world object | `Storehouse` | `building.storehouse` |
| strategic view | view | `StrategicView` | `view.strategic` |
| strength | world game | `SettlementStrength` | — |
| temperament | relations | `Temperament` | `cladan.temperament` |
| territory | world game | `Territory` | `settlement.territory` |
| territory pressure | relations | `GrievanceCause::TerritoryPressure` | — |
| tribute (reserved) | relations | `Tribute` | `relation.tribute` |
| truce (reserved) | relations | `Truce` | `relation.truce` |
| unprovoked attack | relations | — | `battle.unprovoked` |
| watched | world game | — | — |
| Wellspring | world object | `Wellspring` | `building.wellspring` |
| world | world game | `World` (not `MatchWorld`, the world of a skirmish match) | `world` |
| world day | world game | — (a unit of the world calendar) | `world.day` |
| world event | battle | `WorldEvent` | `world.event` |
| world game | form of play | — | — |
| world host | role | — | `world.host` |
| world lifetime | world game | — (a setting; A1 names its key) | `world.lifetime` |
| world pace | world game | `WorldPace` (`WorldPace::AlwaysOn`, `WorldPace::Session`) | `world.pace` |
| world template | world game | `WorldTemplate` (schema `maps.world_template`) | `world.template` |
| world time | world game | `WorldTime` | — |

## 9 Allowed terms

Soft-flagged words that are allowed in a specific context, with the justification. The **Paths** column lists the file globs (relative to the design root) where the gate accepts the term; this file is always among them. Any other soft-flag hit fails the gate.

| Term | Where allowed | Justification | Paths |
|---|---|---|---|
| lockstep | `decisions/deferred/*.md` and decision entries that record what the design phase must decide | names the reference networking model so that the design phase can decide against it; never used as a Species requirement | `decisions/deferred/*.md` `CONVENTIONS.md` |
| Linux | ADRs, decision entries and the "Design-phase inputs" of component specifications, about dedicated-server or CI hosting | community hosts mostly run Linux (vision brief §6 item 10); the client is Windows-only | `ADR/*.md` `decisions/deferred/*.md` `architecture/components/*.md` `CONVENTIONS.md` |
| immediate-mode | developer-tool UI only (`ADR/`, `architecture/components/diagnostics_test_and_performance/`) | the standard name of the debug-overlay UI pattern; the game UI is retained | `ADR/*.md` `architecture/components/diagnostics_test_and_performance/*.md` `CONVENTIONS.md` |
| retail | vendor release-channel names in `ADR/README.md` §"Technology baseline" (for example a retail SDK or Shader Model release) | quotes the vendor's own channel name, with its source URL | `ADR/README.md` `CONVENTIONS.md` |
