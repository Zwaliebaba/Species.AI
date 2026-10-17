# Collected "ip" sections

<!-- from 02_units_and_entities.md -->
### From 02_units_and_entities.md

_Pending — being written._

<!-- from 03_buildings_and_world_objects.md -->
### From 03_buildings_and_world_objects.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Darwinian(s)", "Multiwinian(s)" | name | `Entity::TypeDarwinian`, UI strings | "Sprite" / "worker unit" |
| "Multiwinia", "Darwinia" | brand | `Multiwinia::*`, `App::GameModeMultiwinia` | "<Game>" / "legacy campaign" |
| "Futurewinian" team, "Evilinian" | name | `Location::GetFuturewinianTeamId`, `GunTurret::RecalculateOwnership`, `Tree::Advance` | "Vanguard faction" / "Rogue faction" |
| "The Beast" (Portal crate), Tentacle | name | `Crate::RunTheBeast`, `portal.cpp` | "Rift Horror" |
| "Spawn Mania", "Hot Feet" | name | `Location::m_spawnManiaTimer`, Hot Feet task | "Spawn Surge", "Speed Boost" |
| "Dark Forest", "Magical Forest", "Randomiser", "Ant Nest", "Triffids", "Eggs" crate outcomes | name | `code/worldobject/crate.cpp`, `taskmanager.cpp` | "Blight Grove", "Spirit Grove", "Allegiance Scrambler", "Hive", "Spore Pods", "Brood" |
| "Trunk Port", "Pulse Bomb", "WMD", "WMD Control Panel", "Escape Rocket", "Solar Panel", "Laser Fence", "Radar Dish", "Spawn Point", "Wall Buster", "Statue" (as mode object), "Triffid", "Shaman", "Soul Destroyer", "Spam", "GodDish", "Sepulveda", "Pattern Buffer", "Blueprint Store/Console/Relay", "Control Tower", "Incubator", "Clone Lab" | name | class names and language keys `buildingname_*` | "Warp Gate", "Core Device", "Uplink Console", "Launch Vessel", "Energy Collector", "Barrier Emitter", "Relay Dish", "Spawn Node", "Breach Charge", "Relic", "Pod Plant", "Summoner", "Wraith", — (do not reuse legacy names) |
| "Gq/s" output unit | text | `Generator::GetObjectiveCounter` | "units/s" |
| UI/language keys: `buildingname_<Type>`, `buildingname_gunturret/rocketturret/grenadeturret/flameturret/subversionturret`, `crate_message_run_gunturret`, `crate_message_thebeast`, `crate_message_run_antnest`, `multiwinia_exclamation`, `multiwinia_cts_statuecapture`, `multiwinia_rr_refueled`, `multiwinia_rr_otherrefueled`, `multiwinia_rr_launchprep`, `multiwinia_rr_launchprepother`, `dialog_rocketunderattack`, `dialog_rocketdestroyed`, `dialog_wallbreach`, `dialog_oneminutewarning`, `dialog_pulsebombdestroyed`, `dialog_baseunderattack`, `objective_redpopulation`, `objective_output`, `objective_throughput`, `objective_totalinfection`, `objective_currentcount`, `objective_refined`, `objective_fuel`, `objective_passengers`, on-turret text "FF!" | text | per-class code | own string table |
| Shapes (art): `spawnpointmultiwinia.shp`, `spawnpoint.shp`, `spawnlink.shp`, `masterspawnpoint.shp`, `controlpad.shp`, `battlecannonbase.shp`, `battlecannonturret.shp`, `battlecannonbarrel.shp`, `cannon_bullets.shp`, `cannon_flame.shp`, `cannon_rocket.shp`, `radardish.shp`, `controlstation.shp`, `pulsebombcontrolstation.shp`, `solarpanel.shp`, `pylon.shp`, `generator.shp`, `powerstation.shp`, `laserfence.shp`, `wall.shp`, `beachheadobstruction.shp`, `temple2.shp`, `temple3.shp`, `temple4.shp`, `rockhead.shp`, `statue.shp`, `spaceship.shp`, `carryspider.shp`, `carrystatue.shp`, `carrysquaddie.shp`, `carryglobe.shp`, `carrytelescope.shp`, `carryweights.shp`, `carrycube.shp`, `carryteapot.shp`, `carrybillboard.shp`, `wallbuster.shp`, `trunkport.shp`, `trunkport_mp.shp`, `pulsebomb.shp`, `rocket.shp`, `fuelgenerator.shp`, `fuelgeneratorpump.shp`, `fuelpipe.shp`, `fuelpipebase.shp`, `fuelstation.shp`, `anthill.shp`, `triffidhead.shp`, `aitarget.shp`, `controltower.shp`, `controltowerdish.shp`, `incubator.shp`, `clonelab.shp`, legacy `constructionyard.shp`, `constructionyardrung.shp`, `mineprimitive1.shp`, `displayscreen.shp`, `armour.shp`, `spiritreceiver.shp`, `spiritreceiverhead.shp`, `receiverlink.shp`, `spiritprocessor.shp`, `factory.shp`, `blueprintstore.shp`, `blueprintconsole.shp`, `blueprintrelay.shp`, `bridgeend.shp`, `bridgetower.shp`, `cave.shp`, `chesspiece.shp`, `feedingtube.shp`, `goddish.shp`, `library.shp`, `researchitem.shp`, `mine.shp`, `minecart.shp`, `minepolygon1.shp`, `wheel.shp`, `refinery.shp`, `tracklink.shp`, `fenceswitch.shp`, `upgrade_port.shp`, `primaryupgradeport.shp` | art | `data/shapes/` | new meshes |
| Textures/icons: `textures/starburst.bmp`, `laser.bmp`, `cloudyglow.bmp`, `laserfence.bmp`, `laserfence2.bmp`, `radarsignal.bmp`, `glow.bmp`, `fuel.bmp`, `tree.bmp`, `triangleOutline.bmp`, `interface_grey.bmp`, `sprites/darwinian.bmp`, `icons/thickarrow.bmp`, `icons/arrow.bmp`, `icons/icon_gunturret.bmp`, `icons/icon_antnest.bmp`, `icons/icon_triffid.bmp`, `icons/icon_randomizer.bmp` | art | render code | new textures |
| Sound event names (bindings in `data/sounds.txt`): `Create`, `Reprogramming`, `ReprogramComplete`, `ChangeTeam`, `Damage`, `Explode`, `TurnOn`, `TurnOff`, `TargetSighted`, `FireShell`, `FireFlame`, `FireGrenade`, `FireRocket`, `RegisterKill`, `BeginRotation`, `EndRotation`, `ConnectionEstablished`, `ConnectionLost`, `ExitTeleport`, `TriggerSurge`, `Operate`, `Enable`, `Spark`, `Electrocute`, `CreateNormal`, `CreateEvil`, `CreateMagic`, `Burn`, `Lift`, `Carry`, `Drop`, `CaptureStatue`, `PowerUp`, `PumpUp`, `PumpDown`, `PumpFuel`, `LoadPassenger`, `Refueling`, `Happy`, `Unhappy`, `Flight`, `Malfunction`, `EngineBurn`, `LaunchEgg`, `AddSpirit`, `SpawnEntity`, `ConnectToGod`, `DisconnectFromGod`, `CogTurn`, `AquireResearch`, `CreateResearch`, `Attack` | audio binding | `ListSoundEvents` implementations | new event ids |
| Map file names `mp_<mode>_<n>P_<k>` and layouts; Darwinia campaign map names (`biosphere`, `containment`, `escort`, `garden`, `generator`, `launchpad`, `mine`, `pattern_buffer`, `receiver`, `sandbox`, `temple`, `yard`); prototype map names under `data/levels/old` | content / name | `data/levels/` | new maps and names |
| Darwinia script `launchpad_victory.txt` | text/script | `EscapeRocket::AdvanceIgnition` [B:LEGACY] | — |
| Christmas-mod tree styling | feature/art | `Location::ChristmasModEnabled` in `Tree::RenderAlphas` | optional seasonal theme |

<!-- from 04_crates_powerups_and_weapons.md -->
### From 04_crates_powerups_and_weapons.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Multiwinians" / "Darwinians" (crate reward name and messages), "Evil Darwinians!" | name / text | `cratename_darwinians`, `crate_message_darwinians`, `Crate::RunDarwinians` | "Reinforcements", "Hostile reinforcements!" |
| "Evilinians" (Monster team), "Futurewinians" (Space Ship team) | name | `crate_message_spaceship`, `crate_tooltip_infection` | "Corrupted", "Visitors" |
| "Rocketwinians" | name | `cratename_rocketwinians`, `crate_message_rocketwinians` | "Rocket Troops" |
| "Mega Rage" / key `Bitzkreig` | name | `cratename_bitzkreig`, `multiwinia_blitzkrieg_timer` | "Frenzy" |
| Reward names: Crate Mania, Spawn Mania, Hot Feet, Personal Shield, Subversion, Infection, Magical Forest, Dark Forest, Ants Nest, Plague, Randomiser, Napalm Strike, Slow Down, Speed Up, The Beast, Space Ship, Gun/Rocket/Flame Turret, Airstrike, Nuke, Meteor, Eggs, Squad, Armour, Harvester, Engineer, Rage | names | `cratename_*` | descriptive generic names chosen by the new team (e.g. "Supply Storm", "Rapid Spawn", "Haste", "Shield", "Convert", "Swarm", "Grove", "Blight Grove", "Hive", "Sickness", "Scrambler", "Firebomb Run", "Time Dilation", "Time Rush", "Summoned Horror", "Abductor") |
| "Triffids" | name (third-party literary IP, John Wyndham) | `cratename_triffids`, `Triffid` class | "Spitter Plants" |
| "I love the smell of Napalm in the morning..." | text (third-party film quote) | `crate_message_run_napalm` | "Firebombs inbound!" |
| All crate/power-up UI strings: `crate_drop`, `crate_enemycapture`, `crate_capture`, every `crate_message_*`, `crate_tooltip_*`, `crate_hint_*`, `dialog_powerup_limit`, `multiwinia_spawnmania_timer`, `multiwinia_blitzkrieg_timer`, `multiwinia_retribution_*`, `multiwinia_exclamation`, `multiwinia_param_basiccrates` | text | `data/language/english.txt` | rewrite all text |
| Crate model `researchitem.shp`; weapon/effect models `throwable.shp`, `missile.shp`, `turretshell.shp`, `emptyshell.shp`, `flying_egg.shp`, `boxkite.shp`, `meteor.shp` | art (3-D) | `data/shapes/` | new meshes |
| Textures `bubble.bmp`, `starburst.bmp`, `cloudyglow.bmp`, `laser.bmp`, `laser-long.bmp`, `muzzleflash.bmp`, `subversion.bmp`, `glow.bmp`, `lightning.bmp` | art | `data/textures/` | new VFX textures |
| Icons `target.bmp`, `icon_crate.bmp`, `icon_squad.bmp`, `icon_airstrike.bmp`, `icon_nuke.bmp`, `icon_subversion.bmp`, `icon_personalshield.bmp`, `icon_hotfeet.bmp`, `icon_gunturret.bmp`, `icon_infection.bmp`, `icon_magicalforest.bmp`, `icon_darkforest.bmp`, `icon_antnest.bmp`, `icon_plague.bmp`, `icon_egg.bmp`, `icon_meteorshower.bmp`, `icon_darwinian.bmp`, `icon_randomizer.bmp`, `icon_rage.bmp`, `icon_triffid.bmp`, `nukesymbol.bmp` | art | `data/icons/` | new icon set |
| Crate and power-up sound assets (`cratereward_*`, `crate_capture*`, `cratecapture_*`, `crateland1`, `cratefalling`, …) bound through `data/sounds.txt` | audio | `data/sounds.txt` | new audio; event names may be kept as neutral identifiers |
| "SPAM" (legacy building / internal name of Infection) | name | `code/worldobject/spam.h` | "Swarm" |
| Developer names in comments ("Chris", "Gary") | name | `crate.cpp`, `location.cpp`, `armour.cpp` | do not carry over |
| Level names in the per-level tables (`mp_assault_2P_1`, …) | name | `data/levels/` | new map names |

<!-- from 05_economy_and_population.md -->
### From 05_economy_and_population.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Darwinian(s)", "Multiwinian(s)" (unit name; crate name `cratename_darwinians` "Multiwinians"; "Evil Darwinians!" hard-coded) | name / text | `code/worldobject/crate.cpp:Crate::RunDarwinians`, `data/language/english.txt` | "worker(s)", "hostile workers" |
| "Futurewinian(s)" (team 5) | name | `code/location.cpp:Location::InitFuturewinianTeam` | "alt-faction" |
| "Spirit" / "soul" concept with green orb visual and ascent "to heaven" | concept / art | `code/worldobject/spirit.cpp`, `code/worldobject/spawnpoint.cpp:MasterSpawnPoint::Advance` | "essence orb" |
| "SpawnPoint", "SpawnPointMaster", "SpawnLink", "SpawnPopulationLock" (`buildingname_spawnpoint`, `buildingname_spawnpointmaster`) | name | `data/language/english.txt` | "Gateway", "Source Node", "Relay", "Population Limiter" |
| "TrunkPort" (`buildingname_trunkport`) | name | `data/language/english.txt` | "Warp Gate" |
| "Incubator" (`buildingname_incubator`) | name | `data/language/english.txt` | "Hatchery" |
| "Harvester", "Engineer" (`cratename_harvester`) | name | `data/language/english.txt` | "Collector", "Builder" |
| "Spawn Mania" (`cratename_spawnmania`), "Retribution" (`multiwinia_retribution_maxtasks`) | name / text | `data/language/english.txt` | "Spawn Surge", "Comeback Bonus" |
| UI strings `crate_message_darwinians` ("*T receives Reinforcements!"), `multiwinia_reinforcements` (wave countdown), `multiwinia_reinforcementsgiven`, `multiwinia_skirmish_elimination_30`, `multiwinia_player_left`, `objective_redpopulation` | text | `data/language/english.txt` | own string table |
| Shapes `spawnpointmultiwinia.shp`, `spawnpoint.shp`, `masterspawnpoint.shp`, `spawnlink.shp`, `trunkport_mp.shp`, `trunkport.shp`, `harvester.shp`, `incubator.shp`, `solarpanel.shp` | art | data/shapes directory (listed in the Source map) | new meshes |
| Textures used by the spawn network and ports: `textures/laser.bmp` (link beams), `textures/cloudyglow.bmp` (activation glow), `textures/starburst.bmp` (port status stars) | art | `SpawnBuilding::RenderAlphas`, `SpawnPoint::RenderAlphas`, `SpawnPoint::RenderPorts` | new VFX textures |
| Sound events `TurnOn`, `TurnOff` (spawn point), `DropSpirit` (harvester), `PickedUp` (spirit), `SpawnEntity` (incubator), `PowerUp` (trunk port) — the event names are generic, the samples bound to them in `data/sounds.txt` are IP | audio | [10](10_audio_design.md) | new samples |
| Level names and layouts of the `mp_` level files | name / level design | §4.6 | new maps |
| Darwinia lore of a "central repository of spirits" | text / concept | `docs/sepulveda_interview.txt` | drop |

<!-- from 06_player_control_and_input.md -->
### From 06_player_control_and_input.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Task Manager", "program", "RUN PROGRAM", "SYNTAX ERROR", "PROGRAM TERMINATED", "ACQUIRED", "UPGRADED", "OBJECTIVES COMPLETED" | text / fiction | `data/language/english.txt` (`taskmanager_msg0..5`, `taskmanager_taskmanager`) | "Power slots"; "Ready", "Not available", "Cancelled", "Unlocked", "Improved", "Objectives complete" |
| "*P TERMINATED", "Powerup Limit Reached!", "Retribution : Two powerups max" | text | `dialog_terminated`, `dialog_powerup_limit`, `multiwinia_retribution_maxtasks` | "<power> cancelled", "All power slots full", "Comeback powers: slots full" |
| "crate opened", "reinforcements", "retribution" | text | `multiwinia_cratepowerupfound`, `multiwinia_reinforcementsgiven`, `multiwinia_retributiongiven` | "supply drop", "reinforcements", "comeback bonus" |
| Power-up names ("Napalm Strike", "Hot Feet", "PersonalShield", "Subversion", "Rage", "Magical Forest", "Dark Forest", "Ant Nest", "Plague", "Eggs", "Meteor Shower", "Fire Extinguisher", "Gun Turret", "Rocket Turret", "Flame Turret", "Infection", "Shaman") and all `crate_tooltip_*` texts | names / text | `researchname_*`, `cratename_*`, `crate_tooltip_*` | functional names: "Air strike", "Incendiary strike", "Speed boost", "Shield", "Convert", "Frenzy", "Grove", "Blight grove", "Swarm nest", "Blight", "Brood", "Meteor storm", "Extinguisher", "Turret", "Missile turret", "Flame turret", "Contagion", "Mystic" |
| "Darwinian(s)", "Multiwinian(s)", "Officer", "Engineer", "Squad", "Armour", "Harvester" as unit names; control names `DarwinianPromote`, `IssueDarwinianOrders` | names | `code/global_world.cpp:GlobalResearch::GetTypeName`, `code/lib/input/control_types.inc`, language file | "Worker", "Leader", "Builder", "Strike team", "Transport", "Collector"; `PromoteUnit`, `IssueCrowdOrders` |
| "Sepulveda" (`SepulvedaHelp`); Dr Sepulveda lines (`task_manager_full`, `task_manager_targetfirst`, `research_notyetavailable`) | character / text | `control_types.inc`; `#ifdef USE_SEPULVEDA_HELP_TUTORIAL` blocks | `AdvisorHelp`; n/a (dead) |
| "Multiwinia" / "Darwinia" in preference and file names (`MultiwiniaControlMethod`, `input_preferences_darwinia.txt`), string ids `multiwinia_control_*`, `multiwinia_*help`, `darwinia_tutorial_*` | brand | `code/lib/input/file_paths.cpp`, `code/control_help.cpp`, `code/multiwiniahelp.cpp` | `ControlScheme`, `bindings_classic.txt`, `ui_control_*` |
| "Eclipse" (Introversion UI framework) in `EclipseLMouseDown`, `EclipseLMousePressed`, `EclipseRMousePressed` | name | `control_types.inc` | `UiPointerDown`, `UiPointerHeld`, `UiAltPointerDown` |
| Internal codenames `Pecan`, `Hummingbird` | name | `code/lib/input/inputdriver_values.inc` | `AlwaysTrue`, `FeatureFlagX` |
| Xbox / XInput / "360" naming (`XBoxMenuCycleUp/Down`, "360 Control Help", `_xbox` phrase suffixes, pad button art) and SpaceNav | third-party brand | `control_types.inc`; `data/language/english.txt`; `data/input/input_preferences.txt` | `PadMenuCycleUp/Down`; generic pad glyphs |
| Gesture symbol designs (triangle, arch, arrow, star, zig-zag, V, shield) with their training data | art / design | `data/gestures.txt`, `data/gesture_demos/*`, `icons/gesture_*.bmp` | new symbol set and newly recorded training data |
| All task-bar, cursor, marker and pad-glyph bitmaps listed in the Source map (`icons/icon_*.bmp`, `icons/gesture_*.bmp`, `icons/gestureguide.bmp`, `icons/gesture_alpha.bmp`, `icons/mouse_*.bmp`, `icons/compass*.bmp`, `icons/marker_*.bmp`, `icons/arrows_*.bmp`, `icons/button_*.bmp`, `icons/banner_*.bmp`, `icons/selectionarrow.bmp`, `icons/icon_crate.bmp`, `icons/icon_delete.bmp`, `icons/widearrow.bmp`, `textures/interface_*.bmp`, `textures/starburst.bmp`, `textures/particle.bmp`, `mwhelp/control_camera*.bmp`) | art | `data/icons`, `data/textures`, `data/mwhelp` | new UI icon set (e.g. `ui/glyph_pad_south.png`) |
| Ghost models `battlecannonturret.shp`, `anthill.shp` | art | `GameCursor::RenderSelectedTaskIcon` | new placement-preview meshes |
| UI sound event names `GestureBegin/End/Success/Fail`, `SelectTask`, `DeleteTask`, `MouseOverIcon`, `UnitHighlight`, `Show`, `Hide`, `Slide`, `MouseError`, `SelectUnit`, `SelectionCircle`, `ToggleUnitState`, `SetOrderGoto`, `SetOrders` and their audio | audio | sound calls in `code/location_input.cpp`, `code/taskmanager*.cpp`, `code/gamecursor.cpp` | new UI sound set |
| UI strings `gc_*` (Select, Destroy, Create, Close, Deselect, Place, Promote, Move, Look, Fire, Grenade, Rocket, Airstrike, Change Weapon, Change Orders, Up, Down, Set Goto, Set Turret, Prev, Next, Zoom, Zoom In, Zoom Out, Cancel), `control_help_*`, `control_<key>`, hint texts ("Right mouse button to move / press SPACE to deselect", "Hold SHIFT to move faster", "press c to terminate a unit", "Right Click to toggle orders") | text | `data/language/english.txt` (and the other language files) | write new strings |
| Camera-mount naming convention `player0..3`, `start`, `CamPosBefore` | level data | `data/levels/*.txt`; `code/level_file.h` | `spawn_view_<n>`, `default_view`, `previous_view` |
| Prologue objectives text `prologue_objective_1..5`, "launchpad" location | text / level name | `TaskManagerInterfaceIcons::InitProgressBarList` | n/a (legacy) |

<!-- from 07_ai.md -->
### From 07_ai.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Darwinian(s)", "Multiwinian(s)" as the name of the AI-controlled unit and in Darwinian state names (`StateWorshipSpirit`, `StateWatchingSpectacle`) | name | `code/worldobject/darwinian.h`; throughout | "Trooper(s)"; states "Idle/Rallying/Spectating" |
| "Multiwinia" (game/mode class name, `Multiwinia::m_aiType`) | brand | `code/multiwinia.h` | "Match", "MatchRules" |
| "Evilinians" (`multiwinia_team_virus`), "Futurewinians" (`multiwinia_team_future`) — the two AI-driven neutral-hostile teams | name/text | `data/language/english.txt`, `Team::GetTeamName` | "Corrupted", "Visitors" |
| "(CPU)" `teamtype_2`, "(LEFT)" `teamtype_left`, "Open/CPU" `multiwinia_open_or_cpu` | UI text | `data/language/english.txt`, `Team::GetTeamName` | own strings ("Bot", "Disconnected", "Open slot") |
| "AI Type" `multiwinia_menu_aimode`, "Controls the difficulty of any CPU Players on this level." `multiwinia_menu_aimode_description`; "Easy" `dialog_easy_difficulty`, "Normal" `multiwinia_menu_normal`, "Hard" `multiwinia_menu_hard` | UI text | `code/UI/GameMenuWindow.cpp`, `data/language/english.txt` | own strings |
| "*T left the game!" `multiwinia_player_left`; "Powerup Limit Reached!" `dialog_powerup_limit` | UI text | `Multiwinia::RemoveTeams`, `TaskManager::RunTask` | own strings |
| AI target model `aitarget.shp` | art | `AITarget::AITarget`, `data/shapes/aitarget.shp` | editor gizmo of the new engine |
| Developer names in comments ("Chris", "Gary", "Leander") and "Rocket demo" remark | text | `code/worldobject/ai.cpp:AI::FindTargetBuilding`, `code/worldobject/aiobjective.h`, `code/location.cpp:Location::IsFriend` | do not carry over |
| Map-specific tutorial ids (`mp_koth_2P_1`, `mp_koth_3P_1`, node 41, node 92, zone 9, dishes 7/9/10/12/13/15) | level data / names | `code/worldobject/ai.cpp`, `code/multiwiniahelp.cpp` | new tutorial maps with data-driven AI scripting instead of hard-coded ids |
| Technical class names `AITarget`, `AIObjective`, `AIObjectiveMarker`, `AISpawnPoint` | generic technical names | `code/worldobject/ai.h`, `code/worldobject/aiobjective.h` | may be kept (not distinctive IP) |

<!-- from 20_architecture.md -->
### From 20_architecture.md

<!-- SIP -->

<!-- from 23_terrain_and_landscape.md -->
### From 23_terrain_and_landscape.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| 23 terrain colour ramps: `landscape_beach`, `landscape_containment`, `landscape_dark`, `landscape_default`, `landscape_desert`, `landscape_earth`, `landscape_field`, `landscape_firetop`, `landscape_icecaps`, `landscape_icecaps_peaked`, `landscape_jailbreak`, `landscape_launchpad`, `landscape_marble`, `landscape_marble2`, `landscape_mine`, `landscape_mine2`, `landscape_ridges`, `landscape_shrine`, `landscape_silver`, `landscape_stronghold`, `landscape_trippy`, `landscape_volcanic`, `landscape_volcanicplain` | art | `data/terrain/landscape_*.bmp` | new 64 × 64 slope/height ramps `ramp_<theme>` |
| 9 wave colour tables: `waves_containmentfield`, `waves_default`, `waves_desert`, `waves_dull`, `waves_earth`, `waves_generator`, `waves_icecaps`, `waves_launchpad`, `waves_yellow` | art | `data/terrain/waves_*.bmp` | new 1-D gradients `wavegrad_<theme>` |
| 4 flat-water textures: `water_default`, `water_icecaps`, `water_launchpad`, `water_yellow` | art | `data/terrain/water_*.bmp` | new tileable textures `seabed_<theme>` |
| Landscape overlay grid texture `triangleoutline.bmp` | art | `data/textures/triangleoutline.bmp` | new grid texture |
| Shipped landscape designs (the tile/flatten parameter sets, seeds and guide grids of all 69 landscape-bearing level files) | level design | `data/levels/mp_*.txt`, `data/levels/map_*.txt` | author new maps; do not bake the shipped terrains into the new game |
| Map file names and IDs (`mp_koth_2P_1` …, `map_*` location names such as launchpad, containment, generator, mine, receiver, temple, yard, biosphere, escort, garden, pattern_buffer, sandbox) | name | `data/levels/`, `code/mapcrc.h` | `map_<mode>_<n>`, `region_<nn>` |
| Theme words embedded in asset names (icecaps, launchpad, jailbreak, stronghold, shrine, firetop, trippy, containmentfield) | name | `data/terrain/*` | neutral theme names |
| `docs/landscape lightmap.txt` (Introversion source text) | text / code | `docs/landscape lightmap.txt` | do not reuse; §12 describes the idea in prose |
| Editor UI string ids: `editor_noiseperiod`, `editor_noisescale`, `editor_height`, `editor_method`, `editor_lowlandsmooth`, `editor_posX`, `editor_posY`, `editor_posZ`, `editor_size`, `editor_seed`, `editor_generate`, `editor_randomise`, `editor_delete`, `editor_clone`, `editor_guidegrid`, `editor_flattentype`, `editor_flatten_absolute`, `editor_flatten_add`, `editor_flatten_subtract`, `editor_flatten_subtract2`, `editor_flatten_smooth`, `editor_threshold`, `editor_newtile`, `editor_newflattenarea`, `editor_scaledown`, `editor_scaleup`, `editor_outsideheight`, `editor_cellsize`, `editor_worldsizex`, `editor_worldsizez`, `editor_maxheight`, `editor_movebuildings`, `editor_selectterrain`, `editor_selectwaves`, `editor_selectwater`, `editor_resolution`, `editor_toolsize` | text | `code/interface/landscape_window.cpp` | new string table |
| `redshirt2` text-file cipher marker | text | `contrib/systemiv/lib/filesys/text_stream_readers.cpp:TextReader::CleanLine` | none (use signed/hashed assets) |

<!-- from 24_rendering.md -->
### From 24_rendering.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Multiwinia" window title and window-class name (`APP_NAME_W`) | brand | `code/lib/universal_include.h`, `code/renderer.cpp:Renderer::Initialise` | `<GAME_TITLE>` |
| "Darwinian" sprite and shape (`data/sprites/darwinian.bmp`, `data/shapes/darwinian.shp`), `sprites/darwinian_ouch*.bmp`, `sprites/futurewinian.bmp`, loading-screen spinning Darwinian | name / art | `code/team.cpp:Team::RenderDarwinians`, `code/worldobject/darwinian.cpp`, `code/loading_screen.cpp` | `unit_basic` sprite set |
| Virii, Spirit, Soul Destroyer, Centipede, Spider, Squadie / Insertion Squad, Shaman, Officer, Armour, Triffid, Harvester (names in render code) | names | §4.9, §4.13, §4.14 | role names: swarm, soul orb, devourer, segmented crawler, leaper, squad, summoner, commander, transport, spore pod, collector |
| Sprite/texture art: `data/textures/particle.bmp`, `shapewireframe.bmp`, `triangleoutline.bmp`, `laser.bmp`, `clouds.bmp`, `starburst.bmp`, `glow.bmp`, `cloudyglow.bmp`, `tree.bmp`, `laserfence.bmp`, `laserfence2.bmp`, `lightning.bmp`, `subversion.bmp`, `radarsignal.bmp`, `godray.bmp`, `deform1c.bmp`, `deform3.bmp`, `deform36c.bmp`, `interface_red.bmp`; `data/sprites/virii.bmp`, `santahat.bmp`, `subshadow.bmp` | art | catalogue in [25](25_textures_and_assets.md) | regenerate procedurally / new art with the same roles |
| Terrain ramps, water textures and wave tables (`data/terrain/landscape_*.bmp`, `water_*.bmp`, `waves_*.bmp`) | art | §4.1, §4.2 | new ramps `ramp_<theme>`, tileable water, 1-D gradients |
| 139 `.shp` meshes (`data/shapes/*.shp`) | art | `_work/drafts/P12_shapes.csv` | new meshes |
| Font atlases `speccy`, `editor`, `square` (`data/textures/unicode/*.bmp`, 138 files incl. CJK/Hangul glyph art) and legacy `editor_font*.bmp`, `speccy_font*.bmp` | art (font) | §4.22 | licensed open fonts (an OFL pixel font + Noto CJK) in a glyph cache |
| Cursor and HUD icons (`data/icons/mouse_*.bmp`, `marker_*.bmp`, `compass*.bmp`, `icon_*.bmp`, `widearrow.bmp`, `thickarrow.bmp`, `target.bmp`, `selectionarrow.bmp`) | art | §4.18–§4.20 | new icon set |
| Dr Sepulveda face in the clouds (`data/sepulveda/sepulveda1..5.bmp`, `Clouds::RenderFace`, `ShowDrSFace`) | character / art | §4.4 | drop |
| "Darwin Research Associates" logo (`data/icons/darwin_research_associates.bmp`) and caption `launchpad_logo_1`; publisher logo on the loading screen (`RenderIVLogo`) | brand / art / text | §2.6, §4.25 | omit / studio placeholder |
| `privatedemo1/2` strings and `textures/privatedemo.bmp` | text / art | `Renderer::RenderLogo` [B:DEAD] | omit |
| UI text keys `multiwinia_error_syncerror`, `multiwinia_error_lostconnection`, `multiwinia_presstotalk`; hard-coded "PAUSED" | text | §3.2 | `err_desync`, `err_connection_lost`, `hint_chat`, localised `ui_paused` |
| Team colour names green/red/yellow/blue/orange/purple/cyan/pink (`multiwinia_team_*`) | text | `code/team.cpp:Team::GetTeamName` | generic colour names, new keys |
| Christmas theme (Santa hats, ice ramps) | concept / art | §5.5 | optional seasonal theme with new art |
| Shader sources `data/shaders/*` (14 files) | code (licence) | §6 | re-author in HLSL SM6 |
| Global-world shapes `globalworld_outer/middle/inner.shp`, global-internet visuals | art / concept | §4.24 | drop (legacy) |
| Third-party note: none of the renderer code is third-party (AVIGenerator, MT19937 are listed in [31](31_platform_build_and_tools.md)/[22](22_determinism_and_networking.md)) | — | — | — |

<!-- from 27_audio_engine.md -->
### From 27_audio_engine.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| All 1332 audio samples (954 in `data/sounds/`, 378 in `data/mwsounds/`, e.g. `darwiniandie`, `CrateFall1`, `MultiLobbyAmb1`..`MultiLobbyAmb10`, `countdownbeep3`, `koh_zone_captured`) and the archives that carry them (`sounds.dat`, `sounds2.dat`, `menusounds.dat`) | audio | `data/sounds/`, `data/mwsounds/`, `tools/sounds.txt`, `tools/menu_sounds.txt` | Commission new assets named by function (`unit_die_01`, `crate_fall_01`, `lobby_pad_01`) |
| Licensed third-party music `tresk_-_excuses`, `tresk_-_faces-of-a-fashion`, `tresk_-_impact-of-silence`, `tresk_-weeklies-_schroeders-failure`, `trash80_-_pain-fade-down`, `dma_sc-visitors_from_dreams` | audio (third party) | `data/sounds/*.ogg`, `OTHER Music` blocks in `data/sounds.txt` | `music_track_01`… (new licence) |
| Retro-homage loader tunes `fodder`, `speccyloader`, `pang`, `matrix`, `twoataris` | audio (possibly third-party game music) | `data/sounds/*.ogg` | Drop or original chiptunes |
| Sound-source names hard-coded in the engine: "Sepulveda", "MultiwiniaInterface" (others in the 12-name table are generic) | name / brand | `code/sound/soundsystem.cpp:SoundSourceBlueprint::GetSoundSourceName` | `Narrator`, `MatchUI` |
| Entity/building source and event names in `data/sounds.txt` ("Darwinian", "Officer", "Squadie", "SoulDestroyer", "Virii", …) and sample-group names embedding them | name | `data/sounds.txt` (catalogue in [10](10_audio_design.md)) | Functional names |
| Application name in the DirectX check message (`APP_NAME " requires DirectX9 or Greater"`) | brand / text | `code/sound/sound_library_3d_dsound.cpp:SoundLibrary3dDirectSound::Initialise` | Product name placeholder |
| UI strings in the engine path: "Master Volume" (hard-coded English), "Error" / "Failed to set requested sound settings", "Couldn't start the sound library", "Too few channels" | text | `code/interface/prefs_sound_window.cpp`, `code/sound/soundsystem.cpp:SoundSystem::RestartSoundLibraryInternal` | New string table entries |
| Tool names "Sound Editor", "Sound Stats", "Sound Profile", "Purge Sounds", "Effect Editor", "Pokey Playground" | text | `code/interface/debugmenu.cpp`, `code/interface/soundeditor_window.cpp` | Generic tool names ("Audio editor", "Synth sandbox") |
| "Pokey" (Atari POKEY sound chip) | third-party trademark | `code/effect_processor.cpp` [B:DEBUG] | Drop |
| Editor billboard `sprites/sound.bmp` | art | `code/interface/soundeditor_window.cpp:SoundEditorWindow::Render` | Any speaker icon |
| `docs/MinDistAndVolume.jpg` (airplane / bee / waterfall chart, apparently from DirectX documentation) | art (third party) | `docs/MinDistAndVolume.jpg` | Redraw |

<!-- from 28_ui_framework.md -->
### From 28_ui_framework.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Eclipse" library name and author credit ("Christopher Delay") | name | `contrib/eclipse/*`; control names `EclipseLMouseDown`, `EclipseLMousePressed`, `EclipseRMousePressed` (`data/input/input_preferences.txt`) | `ui-core`; `UiPointerDown`, `UiPointerHeld`, `UiAltPointerDown` |
| Class/brand names `DarwiniaWindow`, `DarwiniaButton`, `GameMenuWindow::RenderBigDarwinian`, `MultiwiniaWindow`, `MultiwiniaFileDialog`, `MultiwiniaEditorButton`, `FinishMultiwiniaButton`, `dialog_multiwinia` | name | `code/interface/*`, `code/UI/*` | `UiWindow`, `UiButton`, `RenderBackdrop`, `MapEditorHubPage`, `MapFilePicker` |
| Title "MULTIWINIA" (`multiwinia_mainmenu_title`), subtitle "Survival of the flattest" (`multiwinia_mainmenu_subtitle`), `textures/mwtitle.bmp`, `icons/mwlogo.bmp` | brand / text / art | `GameOptionsWindow::Render`, `GameMenuWindow::Render`, `DarwiniaWindow::Render` | `app.title`, `app.tagline`, new logotype and logo |
| Backdrop art `sprites/darwinian.bmp` (giant Darwinian), `mwhelp/gear.bmp` | character art | `GameMenuWindow::RenderBigDarwinian` | own mascot / abstract backdrop |
| UI textures `textures/interface_red.bmp`, `textures/interface_blur.bmp`, `textures/cloudyglow.bmp`, `textures/menuhighlight.bmp`, `icons/selectionarrow.bmp`, `icons/tick.bmp` | art | §8.4 | new UI texture set |
| Controller glyphs `icons/button_a.bmp`, `icons/button_b.bmp`, `icons/button_lb.bmp`, `icons/button_rb.bmp`, `icons/button_lb_shadow.bmp`, `icons/button_rb_shadow.bmp` (Xbox 360 pad art) | third-party glyph art | `BackPageButton::Render`, `MenuCloseButton::Render`, `GameMenuWindow::PreloadTextures` | platform-guideline glyphs |
| Fonts "speccy", "editor", "square" (`data/textures/unicode/*`, incl. CJK/Hangul glyph art) | art (font) | `code/app.cpp:App::SetLanguage` | licensed open fonts (e.g. an OFL UI font + Noto CJK) via a glyph cache |
| Palettes (red glass panel + steel-blue highlight; green/red front-end captions on blue-glow boxes) | art direction | §8.3 | new palette (keep the token structure) |
| UI sound events `MenuSelect`, `MenuCancel`, `MenuRollOver`, `LobbyAmbience`, `ChatMessage` (`SoundSourceBlueprint::TypeMultiwiniaInterface`) | audio | `code/game_menu.cpp`, `code/interface/mainmenus.h`, `code/UI/GameMenuWindow.cpp` | `ui.select`, `ui.back`, `ui.hover`, `ui.lobbyLoop`, `ui.chat` |
| Framework phrase ids and their English text: `dialog_close`, `dialog_ok`, `dialog_cancel`, `dialog_back`, `dialog_multiplefiles`, `editor_coloureditor`, `editor_red`, `editor_green`, `editor_blue`, `editor_alpha`, `error`, `multiwinia_editor_success`, `multiwinia_editor_selectmap`, `multiwinia_menu_back`, `multiwinia_presstotalk`, `authentication_result`, `dialog_auth_status_demo_user` | text | `data/language/*.txt` | own string table (ids may be reused as neutral keys, texts not) |
| Literal English captions in framework code: "Close", "Close this window", "Value left", "Value right", "New Window", "New Button", "New Scrollbar", "Failed to set requested screen resolution of …", "Failed to set requested sound settings", "Restart Required" / "Please restart Darwinia …" (Linux) | text | `code/interface/darwinia_window.cpp`, `code/interface/input_field.cpp`, `code/game_menu.cpp`, `contrib/eclipse/*`, `code/interface/scrollbar.cpp`, `code/renderer.cpp`, `code/sound/soundsystem.cpp`, `code/interface/prefs_screen_window.cpp` | rewrite |
| Store URL `http://store.introversion.co.uk` (`App::GetBuyNowURL`), update URL / `autopatch.exe` launch | brand / URL | `code/app.cpp`, `code/UI/UnlockFullGameButton.h`, `code/UI/update_page_buttons.h` | own store/updater URLs from configuration |
| Menu page and button strings (every `multiwinia_*` id used by `code/UI`) and window names | text | enumerated in the IP registers of [09](09_meta_systems.md) and `_work/drafts/P17b_strings.csv`, `_work/drafts/P17a_ui_framework_editor_ui_strings.csv` | own strings |

<!-- from 31_platform_build_and_tools.md -->
### From 31_platform_build_and_tools.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Multiwinia", "Darwinia" product names; `APP_NAME`/`APP_NAME_W` window title and class name; `DARWINIA_*` macro names; `darwinia*.exe`, `multiwinia.exe` | brand | `code/lib/universal_include.h`, `targets/vs2005/*`, `tools/*.bat` | `GAME_NAME`, `game.exe` |
| "Introversion Software" (version resource, installers' `AppPublisher`), profile folder `Introversion\Multiwinia\`, Mac bundle id `uk.co.introversion.multiwinia` | brand | `targets/vs2005/version.rc`, `tools/InstallerScript*.iss`, `code/app.cpp:App::GetProfileDirectory`, `targets/macosx/Multiwinia/Info.plist` | own studio name / bundle id / folder |
| Host names and endpoints: `metaserver-mwdev.introversion.co.uk`, `crashreports.introversion.co.uk:4537`, `multiwinia.co.uk`, `forums.introversion.co.uk`, `darwinia.co.uk`, fallback crash e-mail address | brand / service | `code/app.cpp:App::InitMetaServer`, `contrib/XCrashReport/SendEmail.cpp`, `contrib/XCrashReport/EmailDefines.h`, installers | `example.com` services of the new studio |
| Crash-report magic `0B9432AF`; Vista Game Explorer GUID `F58175C7-E99C-4151-800D-7BB5E89E4960` | identifiers | `code/main.cpp:AppMain`, `code/main.cpp:DoVistaChecks` | new ids |
| Distributors and partners: Ambrosia Software, Buka (`origin` tag `buka`), eGames/Cinemaware Marquee, Logitech G15, amBX, Sparkle | brand (third party) | `targets/macosx/Multiwinia/*`, `tools/build_multiwinia_retail_russian.bat`, `tools/InstallerScriptEGames.iss`, exclusion lists | omit |
| "Futurewar Shape Format" exporter description; Dr Sepulveda / Protologic story documents | name / text | `tools/ShapeExporter/ShapeExporter.cpp`, `docs/sepulveda_interview.txt`, `docs/story material.txt` | "Shape format v1"; omit |
| Icons and box art: `targets/vs2005/darwinia.ico`, `targets/vs2005/DarwiniaVista.ico`, `targets/vs2005/GDF/*/boxart_*.png`, `targets/vs2005/GDF/*/gameicon_*.ico`, `targets/macosx/Multiwinia/Multiwinia.icns`, root `darwinia.png`, `tools/NSIS Installer/darwinia.bmp`, `tools/NSIS Installer/header.bmp`, `tools/loki/common/setup.data/splash.xpm` | art | listed | new art |
| Fonts packed into `language.dat` (`editor`, `speccy`, `square` pages); `verdana-bold.bmp` inside root `animator.dat` | art | `data/textures/unicode/*`, `animator.dat` | licensed/open fonts |
| Sound lists naming every audio asset; licensed retro music excluded from retail (`tresk*`, `trash80*`, `dma_sc*`, `twoataris*`, `pang*`) | audio (own and third-party) | `tools/sounds.txt`, `tools/menu_sounds.txt`, `tools/multiwinia_exclusions.txt` | new audio set; never reuse the licensed tracks |
| Language/UI text tables | text | `data/language/*.txt` | new string table |
| EULAs | text | `EULA.txt`, `docs/IntroversionEULA.txt`, `docs/Multiwinia-Buka-Russian-EULA.txt`, `tools/NSIS Installer/licence.txt` | own licence |
| Personal data: beta-tester credit lists packed into retail `main.dat` (`data/betatesters.txt`, `data/multiwinia_betatesters.txt`), `docs/betatesters.txt` (handles and e-mail addresses), developer machine path in `targets/macosx/Multiwinia/data`, developer names in Mac credits and code headers | personal data / names | listed | never reproduce |
| Map names in `MAPID_*` (87, e.g. `MAPID_MP_KOTH_2P_1`) and in inclusion lists | name (level names) | `code/mapcrc.h`, `tools/multiwinia_inclusions_*.txt` | new map names |
| Third-party code: crash-report sample code (XCrashReport, public domain), `GetWinVer`/`MiniVersion` (public domain), unrar (RARLAB licence), `Rar.exe`, UPX, GnuWin32 grep/sed, 7-Zip, PuTTY `pscp`, NSIS SDK samples, Loki Setup, DirectX redistributable cabinets, AVIGenerator sample | third-party | `contrib/XCrashReport/*`, `code/lib/GetWinVer.cpp`, `contrib/unrar/*`, `tools/*.exe`, `tools/NSIS Installer/*`, `tools/loki/*`, `code/lib/avi_generator.cpp` | use the platform/standard equivalents named in §15; respect their licences |
