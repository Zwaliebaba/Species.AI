Species names come from the rename ledger of the conversion run. A document uses only the names below; a name that is missing is proposed in the agent report, never invented in a document.

- **Terms.** Proper Species terms are capitalised as listed (Worker, Spawn Node, Supply crate). Generic words (tree, wall, spider, turret) stay lower case in prose.
- **Identifiers.** Types and enumerators are PascalCase (`SpawnNode`, `CrateOutcome::Shield`); files and ids are lower case (`species`, `map_siege_3p_01`). The game name is `Species` in types and namespaces and `species` in files and ids. A generic word used as a type name takes its kind as a suffix (`TreeObject`, `SpiderCreature`), and no Species identifier repeats an identifier of the reference implementation.
- **String ids** are dotted, `<prefix>.<item>`, using the prefix listed (`unit.worker.name`, `mode.flag_chain.objective`, `ui.lobby.ready`). Player-facing text is always new text.
- **Tuning keys** are `ref.` followed by the same prefix and a parameter (`ref.unit.worker.move_speed`, `ref.mode.spawn_control.match_length`).
- **Asset ids** come from `assets/asset_manifest.csv`; the patterns below give only the stems that documents may cite.

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
| Relic Base | world object | `RelicBase` | `building.relic_base` |
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
| match options | match rule | `MatchOptions` | `match.options` |
| match rules | match rule | `MatchRules` | `match` |
| sudden death | match rule | — | `match.sudden_death` |

### 8.7 Supply crates and their outcomes

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| Abductor (outcome) | crate outcome | `CrateOutcome::Abductor` | `crate.abductor` |
| airstrike | crate outcome | `CrateOutcome::Airstrike` | `crate.airstrike` |
| Allegiance Scrambler | crate outcome | `CrateOutcome::AllegianceScrambler` | `crate.allegiance_scrambler` |
| Allegiance Scrambler (effect) | crate outcome | `AllegianceScrambler` | `crate.allegiance_scrambler` |
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
| Iron Defence | achievement | `Achievement::IronDefence` | `achievement.iron_defence` |
| Massacre | achievement | `Achievement::Massacre` | `achievement.massacre` |
| Stockpiler | achievement | `Achievement::Stockpiler` | `achievement.stockpiler` |
| Total Control | achievement | `Achievement::TotalControl` | `achievement.total_control` |
| Tourist | achievement | `Achievement::Tourist` | `achievement.tourist` |

### 8.10 User interface, text and camera

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
| All power slots full | UI or text | — | `ui.power_slots.full` |
| Cancelled | UI or text | — | `ui.power_slots.cancelled` |
| Comeback powers: slots full | UI or text | — | `ui.power_slots.comeback_full` |
| control scheme | UI or text | `ControlScheme` | `ui.options.control_scheme` |
| explainer animation | UI or text | `ExplainerAnimation` | `ui.help.explainer` |
| Improved | UI or text | — | `ui.power_slots.improved` |
| map definition | UI or text | `MapDefinition` | `map` |
| map id | UI or text | `MapId` | `map` |
| match UI sound source | UI or text | `MatchUI` | `sfx.match_ui` |
| Not available | UI or text | — | `ui.power_slots.not_available` |
| Objectives complete | UI or text | — | `ui.match.objectives_complete` |
| Power slot | UI or text | `PowerSlot` | `ui.power_slots` |
| Power slots | UI or text | `PowerSlots` | `ui.power_slots` |
| Ready | UI or text | — | `ui.power_slots.ready`, `ui.lobby.ready` |
| Skill rating | UI or text | `SkillRating` | `ui.leaderboard.skill_rating` |
| UI core | UI or text | `UiCore` | `ui` |
| Unlocked | UI or text | — | `ui.power_slots.unlocked` |

### 8.11 Naming patterns

| Species term | Kind | Identifier | String-id prefix |
|---|---|---|---|
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
| player command ids | naming pattern | `NET_GAME_*` | — |
| previous camera view | naming pattern | `previous_view` | — |
| studio name (placeholder) | naming pattern | `<STUDIO>` | — |

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
| airstrike flight | engine type | `AirstrikeFlight` | — |
| airstrike target marker | engine type | `AirstrikeTargetMarker` | — |
| Allegiance Scrambler effect | engine type | `AllegianceScramblerEffect` | — |
| always-on feature flag | engine type | `AlwaysTrue` | — |
| application | engine type | `Application` | — |
| asset cache | engine type | `AssetCache` | — |
| audio decoder | engine type | `AudioDecoder` | — |
| audio engine | engine type | `AudioEngine` | — |
| barrage missile | engine type | `BarrageMissile` | — |
| bot navigation node | engine type | `BotNavNode` | — |
| bot objective | engine type | `BotObjective` | — |
| bot objective marker | engine type | `BotObjectiveMarker` | — |
| bot team | engine type | `TeamKind::Bot` | — |
| carryable structure | engine type | `CarryableStructure` | — |
| cloth flag | engine type | `ClothFlag` | — |
| Core Device pulse wave | engine type | `CorePulseWave` | — |
| crash reporter | engine type | `CrashReporter` | — |
| Crawler group | engine type | `CrawlerGroup` | — |
| Crawler trail sample | engine type | `CrawlerTrailSample` | — |
| DSP filter definition | engine type | `DspFilterDef` | — |
| dust vortex | engine type | `DustVortex` | — |
| easy bot | engine type | `BotDifficulty::Easy` | — |
| energy-grid structure | engine type | `GridStructure` | — |
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
| job queue | engine type | `JobQueue` | — |
| land route graph | engine type | `LandRouteGraph` | — |
| laser bolt | engine type | `LaserBolt` | — |
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
| obstacle grid | engine type | `ObstacleGrid` | — |
| options screen | engine type | `OptionsScreen` | — |
| order trail mark | engine type | `OrderTrailMark` | — |
| pack file | engine type | `PackFile` | — |
| pad menu cycle down | engine type | `PadMenuCycleDown` | — |
| pad menu cycle up | engine type | `PadMenuCycleUp` | — |
| particle | engine type | `ParticleInstance` | — |
| particle pool | engine type | `ParticlePool` | — |
| power and research levels | engine type | `PowerLevels` | — |
| power slots panel | engine type | `PowerSlotsPanel` | — |
| presentation generator seed | engine type | `FxRng::Seed` | — |
| presentation random fraction | engine type | `FxRng::NextUnit` | — |
| presentation random integer | engine type | `FxRng::NextUint` | — |
| presentation random signed fraction | engine type | `FxRng::NextSigned` | — |
| procedural foot | engine type | `ProceduralFoot` | — |
| procedural leg | engine type | `ProceduralLeg` | — |
| product name | engine type | `kProductName` | — |
| remote player team | engine type | `TeamKind::RemotePlayer` | — |
| render backdrop | engine type | `RenderBackdrop` | — |
| RGBA colour | engine type | `ColourRgba` | — |
| RGBA8 image | engine type | `ImageRgba8` | — |
| rocket | engine type | `RocketProjectile` | — |
| route graph | engine type | `RouteGraph` | — |
| route graph node | engine type | `RouteGraphNode` | — |
| route point | engine type | `RoutePoint` | — |
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
| simulation random fraction | engine type | `SimRng::NextUnit` | — |
| simulation random integer | engine type | `SimRng::NextUint` | — |
| simulation random signed fraction | engine type | `SimRng::NextSigned` | — |
| sky renderer | engine type | `SkyRenderer` | — |
| sliced pool | engine type | `SlicedPool` | — |
| smoke column | engine type | `SmokeColumn` | — |
| snowflake | engine type | `SnowFlake` | — |
| sound event definition | engine type | `SoundEventDef` | — |
| sound parameter | engine type | `SoundParam` | — |
| sound source definition | engine type | `SoundSourceDef` | — |
| sound voice | engine type | `SoundVoice` | — |
| spawn-network edge | engine type | `SpawnEdge` | — |
| spawn-network structure | engine type | `SpawnStructure` | — |
| spectator | engine type | `TeamKind::Spectator` | — |
| spider gait | engine type | `SpiderGait` | — |
| standard bot | engine type | `BotDifficulty::Standard` | — |
| string table | engine type | `StringTable` | — |
| structure | engine type | `Structure` | — |
| structure kind | engine type | `StructureKind` | — |
| structure port | engine type | `StructurePort` | — |
| supply-crate outcome | engine type | `CrateOutcome` | — |
| team | engine type | `MatchTeam` | — |
| team command state | engine type | `TeamCommandState` | — |
| team slots per match | engine type | `kMaxTeams` | — |
| teleport | engine type | `TeleportStructure` | — |
| terrain definition | engine type | `TerrainDefinition` | — |
| terrain flatten area | engine type | `TerrainFlattenArea` | — |
| terrain height field | engine type | `TerrainField` | — |
| terrain renderer | engine type | `TerrainRenderer` | — |
| terrain tile | engine type | `TerrainTile` | — |
| thrown weapon | engine type | `ThrownWeapon` | — |
| tick clock | engine type | `TickClock` | — |
| tick period | engine type | `kTickPeriod` | — |
| trail point | engine type | `TrailPoint` | — |
| transit orb | engine type | `TransitOrb` | — |
| turret | engine type | `Turret` | — |
| turret round | engine type | `TurretRound` | — |
| UI alt pointer down | engine type | `UiAltPointerDown` | — |
| UI button | engine type | `UiButton` | — |
| UI button base | engine type | `UiButtonBase` | — |
| UI input pump | engine type | `UiInputPump` | — |
| UI panel base | engine type | `UiPanelBase` | — |
| UI pointer down | engine type | `UiPointerDown` | — |
| UI pointer held | engine type | `UiPointerHeld` | — |
| unused slot | engine type | `TeamKind::Unused` | — |
| update slices per tick | engine type | `kUpdateSlicesPerTick` | — |
| volcano vent | engine type | `VolcanoVent` | — |
| waypoint order | engine type | `LeaderOrder::Waypoint` | — |
| world cursor | engine type | `WorldCursor` | — |
| world markers | engine type | `WorldMarkers` | — |
| world surface grid | engine type | `SurfaceGrid` | — |

### 8.13 Names used only in Dropped decision entries

These name content that the reference library does not specify: Advisor, Hatchery, Rift, Rift Horror, Rift tentacle, Summoner, Tri-walker, Vehicle Duel.
