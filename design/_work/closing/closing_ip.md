# Collected "ip" sections

<!-- from 00_overview.md -->
### From 00_overview.md

Overview-level items only; every owning document carries its own complete register, and [90](90_glossary.md) / [40](40_carry_forward_assessment.md) consolidate them.

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Multiwinia", "Darwinia", subtitle "Survival of the flattest", title art | brand / text / art | `code/lib/universal_include.h` (`APP_NAME`), `data/language/english.txt` (`multiwinia_mainmenu_title`, `multiwinia_mainmenu_subtitle`) | `<GAME_TITLE>`, own tagline and logo |
| "Darwinian(s)", "Multiwinian(s)" and the Darwinian sprite and shape | name / art | `code/worldobject/darwinian.cpp`, `data/sprites/darwinian.bmp`, `data/shapes/darwinian.shp` | Worker; new character art |
| "Officer", "Squadie", "Harvester", "Engineer", "Armour" as unit names | name | `code/worldobject/entity.cpp:Entity::GetTypeName`, `data/language/english.txt` | Leader, Strike team, Collector, Builder, Transport |
| Spirit / soul concept and orb visual | concept / art | `code/worldobject/spirit.cpp` | Essence orb |
| "Evilinians", "Futurewinians" | name | `data/language/english.txt` (`multiwinia_team_virus`, `multiwinia_team_future`) | Rogue faction, Visitor faction |
| Mode names Domination, King of the Hill, Capture the Statue, Assault, Rocket Riot, Blitzkrieg, Tank Battle | name | `data/multiwinia.txt`, `data/language/english.txt` (`multiwinia_gametype_*`) | Spawn Control, Zone Control, Relic Run, Siege, Launch Race, Flag Chain, Armour Duel |
| Building names (Spawn Point, Trunk Port, Pulse Bomb / "WMD", Control Station, Escape Rocket, Solar Panel, Radar Dish, Laser Fence, Wall Buster, Statue) | name | `data/language/english.txt` (`buildingname_*`) | Spawn Node, Warp Gate, Core Device, Uplink Console, Launch Vessel, Energy Collector, Relay Dish, Barrier Emitter, Breach Charge, Relic |
| All 31 crate outcome names (`cratename_*`, enumerated in §8.4 — distinctive ones include Mega Rage, Spawn Mania, Crate Mania, Rocketwinians, Hot Feet, Magical Forest, Dark Forest and Triffids, a third-party literary name) and every `crate_message_*` text, including the Napalm film quote | name / text | `data/language/english.txt` (`cratename_*`, `crate_message_*`) | §8.4 placeholders |
| "Retribution", "Task Manager", "program" fiction | name / text | `data/language/english.txt` | Comeback Bonus, Power slots, Power |
| Achievement names (12) | name | `code/app.h` | per [01 IP items](01_game_modes.md#ip-items) |
| Map names and layouts of the 49 retail maps | name / level design | `data/levels/level_enum.txt` and the `mp_*` level files | new maps and names |
| Dr Sepulveda (Darwinia character, dead in Multiwinia) | character | `data/sepulveda/sepulveda1.bmp`, `docs/sepulveda_interview.txt` | drop |
| All audio samples and the licensed music tracks (never shipped in Multiwinia) | audio | `data/sounds.txt`, `tools/multiwinia_exclusions.txt` | new audio; never reuse the licensed tracks |
| "Introversion Software", store and metaserver URLs | brand / service | `code/app.cpp:App::InitMetaServer`, `code/app.cpp:App::GetBuyNowURL` | own publisher name and endpoints |

<!-- from 01_game_modes.md -->
### From 01_game_modes.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Multiwinia", "Multiwinians", "Darwinian(s)" | brand / name | strings, class names | "the game", "units" |
| Mode names Domination, King of the Hill, Capture the Statue, Assault, Rocket Riot, Blitzkrieg, Tank Battle (and "Skirmish") | name | `data/multiwinia.txt`, `multiwinia_gametype_*` | Spawn Control, Zone Control, Relic Run, Siege, Launch Race, Flag Chain, Armour Duel |
| "Evilinians", "Futurewinians" | names / characters | `multiwinia_team_virus`, `multiwinia_team_future` | Rogue faction, Future faction |
| "WMD", "Pulse Bomb", "Trunk Port", "Retribution", "MEGA RAGE", "SPAWN MANIA", "Hot Feet", "Sudden Death" (generic) | names | strings §8, level options | Core Device, Warp Gate, Comeback Bonus, Frenzy, Spawn Surge, Speed Boost ("Sudden Death" is a generic term) |
| Achievement names Dominator, Hoarder, Hail To The King, Unclad Skies, Aggravated Assault, Master Player, Master of Multiwinia, Explorer, Carnage, BlitzMaster, Wrong Game, Genocide | names | `code/app.h` | Total Control, Collector, Crowned, Clean Launch, Iron Defence, Giant Slayer, Completionist, Tourist, Massacre, Lightning War, Friendly Fire, Annihilator |
| All 87 mode UI strings (§8) and option labels/descriptions (`multiwinia_param_*`, `multiwinia_option_*`, `multiwinia_help_*`) | text | `data/language/*.txt` | rewrite all copy |
| Map display names (49 retail + 8 unshipped, §4 map tables) and file names `mp_<mode>_<n>P_<k>` | names | the `mp_*.txt` level files (Source map), `data/language/english.txt` and the other eight language files | new map names |
| Mode pictures `mwhelp/mode_*.bmp`, complexity icons `icons/menu_basic/intermediate/advanced.bmp`, flag textures `icons/flag*.bmp`, rocket panel art `icons/rocketstatuspanel.bmp`, `icons/rocketcracked.bmp` | art | §1.1, §2.12, §4.5–§4.6 | new art |
| Statue shapes `carrystatue`, `carrysquaddie`, `carryglobe`, `carrytelescope`, `carryweights`, `carrycube`, `carryteapot`, `carrybillboard` (.shp); rocket, fuel, pulse-bomb and control-station shapes | art | [03 §8](03_buildings_and_world_objects.md#8-mode-objective-objects) | new models |
| Interface sounds `CountdownTick`, `CountdownZeroTick`, `GameOver`, `YouWon`, `YouLost`, `CaptureStatue` and their samples | audio | `data/sounds.txt`, [10](10_audio_design.md) | new audio |
| `data/multiwinia_betatesters.txt` (real people's names) | personal data | data file | do not ship |
| Team colour palette and colour names | generic | §2.5.3 | free to reuse |

<!-- from 02_units_and_entities.md -->
### From 02_units_and_entities.md

Introversion IP that a new game must not reuse, with neutral placeholders.

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Darwinian(s)", "Multiwinian(s)" (unit names, sound-event prefixes, text "Creating Darwinians", "Darwinia Hit, %d Dead") | name/text | `code/worldobject/darwinian.h`, `code/worldobject/shaman.cpp`, `code/worldobject/nuke.cpp` | "Citizen(s)" |
| "Futurewinian(s)" | name/character | `Darwinian::m_futurewinian`, `code/location.cpp:Location::GetFuturewinianTeamId`, `sprites/futurewinian.bmp` | "Visitors" |
| "Multiwinia" / "Darwinia" | brand | throughout (`multiwinia_exclamation`, achievement `DarwiniaAchievementWrongGame`) | project codename |
| "Officer" banners and order visuals | art | `icons/banner_none.bmp`, `banner_follow.bmp`, `banner_formation.bmp`, `banner_absorb.bmp`, `icons/widearrow.bmp` | new banner art |
| "Insertion Squad" / "Squadie" | name | `code/worldobject/insertion_squad.h` | "Strike Team" / "Trooper" |
| "SpaceInvader" bomber (echoes a third-party trademark) | name/brand | `code/worldobject/airstrike.h`, `spaceinvader.shp` | "Bomber" |
| "Virii", "Soul Destroyer", "Spore Generator", "Triffid" (third-party literary term), "Army Ant", "Centipede", "Spider", "Tripod", "Tentacle", "Lander", "Shaman", "Engineer", "Harvester", "Armour", "Egg" (creature/unit designs) | name/design | `Entity::GetTypeName`, `data/stats.txt`, class headers | "Crawler", "Reaper Serpent", "Brood Drifter", "Pod Launcher"/"Seed Pod", "Ant", generic role names; redesign creatures |
| "Zombie" ghost, "The Beast" crate | name | `code/worldobject/souldestroyer.h`, `code/worldobject/crate.cpp:Crate::RunTheBeast` | "Wisp", "The Maw" |
| Submarine-launched multi-warhead nuke with ground "nuke symbol" | design expression | §12 | keep the mechanic, new presentation |
| UFO abduction/drop event | design expression | §23 | new presentation |
| UI/text strings: `crate_message_run_airstrike`, `crate_message_auto_airstrike`, `crate_message_run_napalm`, `crate_message_auto_napalm`, `crate_message_run_nuke`, `crate_message_auto_nuke`, `crate_message_run_meteor`, `crate_message_spaceship`, `crate_message_run_eggs`, `crate_message_auto_eggs`, `crate_message_run_antnest`, `dialog_wronggame`, `engineer_idle`, `engineer_towaypoint`, `engineer_tospirit`, `engineer_toincubator`, `engineer_reprogramming`, `engineer_researching`, `engineer_bridge`, `entityname_<Type>`, `spawnname_*` | text | `data/language/*` via `LANGUAGEPHRASE` | rewrite all strings |
| Shapes: `officer.shp`, `squad.shp`, `armour.shp`, `harvester.shp`, `engineer.shp`, `spaceinvader.shp`, `throwable.shp`, `armyant.shp`, `armyant2.shp`, `armyant3.shp`, `triffidegg.shp`, `centipede.shp`, `centipedehead.shp`, `spider.shp`, `spider_leg_upper.shp`, `spider_leg_lower.shp`, `sporegenerator.shp`, `souldestroyerhead.shp`, `souldestroyertail.shp`, `meteor.shp`, `spaceship.shp`, `tank_body.shp`, `tank_turret.shp`, `missile.shp`, `flying_egg.shp`, `tripod.shp`, `tripod_leg_part.shp`, `lander.shp`, `boxkite.shp`, `data/shapes/darwinian.shp` | art | `data/shapes/` | new models |
| Sprites/textures/icons: `sprites/darwinian.bmp`, `darwinian_ouch1.bmp`–`darwinian_ouch4.bmp`, `darwinian_side.bmp`, `santahat.bmp`, `brainwaves.bmp`, `futurewinian.bmp`, `egg.bmp`, `viriifull.bmp`, `ghost.bmp`, `sub.bmp`, `subshadow.bmp`, `nuke.bmp`; `textures/glow.bmp`, `bubble.bmp`, `laser.bmp`, `starburst.bmp`, `shaman_background.bmp`; `icons/nukesymbol.bmp`, `icon_nuke.bmp`, `icon_airstrike.bmp`, `icon_meteorshower.bmp`, `shaman_*.bmp`, `mouse_selection.bmp` | art | per-class `Render` | new art |
| Soul visual (team-coloured double-quad orb) and BoxKite concept | art/design | `Spirit::Render`, `BoxKite` | "essence orb" |
| Christmas santa-hat cosmetic | design/art | `Darwinian::Render`, `Location::ChristmasModEnabled` | optional seasonal cosmetic |
| Audio sample groups bound in `data/sounds.txt` (§33.4: darwiniandie, laserhitdarwinian, darwinianlosehealth, darwiniancreate, officercreate, darwinrunaway, combatvoxretreat, 0lasers, laserreload, victoryjum, darwinnianburncry, dselect, carrystatue, combatvoxattack, darwinseethreatattack, officerleftright, enterteleport, exitteleport, newsamplegroup1/5/9/11/13/25/26/27/28, grenade_return3, airstrike_alarm, turretfire, squaddiedeath, tank_hover, tankcreate, tankloseheath, tankdeath, tankenginefar, officerordergoto, ufo_engine1, ufo_engine2, rumble2_loop, incubatoraddspirit, engineer_hover, engineer_reprog_radar_loop, engineer_repog_radar_loop_end, engineerinjury, engineerdie, airstrike_drop, airstrike_hover2, rocketthrustloop, nukedetonatenear, nukedetonatefar, antatack, antcreat, antlosehealth, antdeath, triffidegglaunc, triffideggbounce, triffeggbursts, souldestdeath, eggsplat, virusattack, viruspain, sporegengroan, sporegenlosehealt, sporegeneratordie, spideregglay, spidercreate, spiderpounce, spiderdeath, spiderfeet, souldestroycreate, souldestpain, soulattack, soulenemysighted, meteorhitclose1, meteorhitfar1, rocketthrust2loop, theramins, tractorbeam2, tripodcreate, tprdattac, trpodlosehealth, ttripoddie, sonar) | audio | `data/sounds.txt` | new sound set; event names may stay as neutral ids |
| Sound event names ("SeenThreatAttackHorn", "VictoryJump", "LiftBuilding", "SetOrderAbsorb", …) | audio/text | `ListSoundEvents` | neutral ids |
| "Dr Sepulveda" line `officer_notwalkable` | text/character | `Officer::SetOrders` (dead under `USE_SEPULVEDA_HELP_TUTORIAL`) | omit |

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

<!-- from 08_levels_and_level_design.md -->
### From 08_levels_and_level_design.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| Map file names `mp_<mode>_<N>P_<k>`, `MAPID_*` macro names, PDLC map names | name | `data/levels/`, `code/mapcrc.h`, `data/levels/level_enum.txt` | `map_<mode>_<players>p_<nn>` |
| 49 English map names and descriptions ("Beneath the Baleful Eye" … "Threads of Fate"), `mp_*_name`/`_description` phrases | text | `data/language/english.txt` | new names written for the new maps |
| Map layouts, terrain parameters, building placements of all shipped maps | level design content | `data/levels/mp_*.txt` | new maps |
| Thumbnails (56 `mp_*` + `default.bmp`) | art | `data/thumbnails/` | engine-generated thumbnails |
| Terrain/wave/water colour-ramp names used by maps (landscape_icecaps, launchpad, mine, containmentfield, stronghold, jailbreak, firetop, shrine, trippy, marble, silver, dark, desert, earth, dull, generator, yellow, default) | art + names | `data/terrain/*.bmp` via `landColourFile` etc. | procedurally generated `ramp_<theme>` |
| Static shapes referenced by maps (`beachheadobstruction.shp`, `temple2.shp`, `temple3.shp`, `temple4.shp`, `rockhead.shp`, `spaceship.shp`) and the editor gizmo `camera.shp` | art | `data/shapes/` | new meshes |
| "Multiwinia" in `MultiwiniaOptions_StartDefinition`, `MultiwiniaZone`, `MultiwiniaWindow` | brand | `code/level_file.cpp`, `code/worldobject/multiwiniazone.cpp` | `MatchOptions`, `Zone` |
| "Darwinian", "Futurewinian" (`futurewinianteam`), "Multiwinians" in map descriptions | names | `code/level_file.h`, `data/language/english.txt` | "unit", "alien faction" |
| Darwinia location names (garden, containment, generator, mine, yard, escort, pattern_buffer, biosphere, receiver, temple, launchpad, sandbox) | names | `data/levels/map_*`, `data/game.txt` | drop |
| Camera-mount conventions `player0`–`player3`, `start`, `CamPosBefore`; editor defaults `NewLevel`, `mp_blank.txt`, `mp_newmap.txt`, `blah<n>`, `CamAnim<n>` | names/text | `code/level_file.h`, `code/main.cpp`, `code/interface/*` | `spawn_view_<n>`, `default_view`, `__start_pose`, neutral defaults |
| Editor UI strings (`editor_*`, `dialog_otheroptions`, `dialog_invalidcrates`, `taskmanager_mapeditor` "Map Editor", `multiwinia_editor_*`) | text | `data/language/english.txt`, `code/interface/*` | new string table |
| Achievement "Master of Multiwinia" tied to the level index | name | `code/achievement_tracker.cpp` | new achievement names |

<!-- from 09_meta_systems.md -->
### From 09_meta_systems.md

All English strings behind the phrase ids are Introversion text (`data/language/*.txt`); the ids may be reused as neutral keys but texts, names and art may not. Complete phrase-id inventories for this area: `_work/drafts/P17b_strings.csv` (194 ids) and `_work/drafts/P02_game_modes_strings.csv` (87 ids).

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "MULTIWINIA" title, subtitle "Survival of the flattest", `textures/mwtitle.bmp`, logo `icons/mwlogo.bmp` | brand / text / art | `multiwinia_mainmenu_title`, `multiwinia_mainmenu_subtitle`, `GameMenuWindow::Render` | "<GAME TITLE>", new tagline, new logotype |
| Menu mascot backdrop (giant unit sprite `sprites/darwinian.bmp`), gear `mwhelp/gear.bmp`, player icons `textures/darwinian.bmp` | character art | `GameMenuWindow::RenderBigDarwinian`, `code/UI/LevelSelectButton.h` | generic unit silhouette, abstract cog |
| Unit / world names "Darwinia", "Darwinian", "Multiwinian(s)", "Futurewinian", "Evilinians" in strings and help pages | name | `multiwinia_helppage_general*`, `multiwinia_help_*`, rocket panel captions | "<world>", "<unit>" |
| Game-mode names Domination, King of the Hill, Capture the Statue, Assault, Rocket Riot, Blitzkrieg, Tank Battle; short forms DOM / KOTH / CTS / ASLT / ROCKET / BLITZ | names / text | `multiwinia_gametype_*`, `multiwinia_gametypeshort_*` | descriptive generic names (Control Points, Hill, Relic Carry, Siege, Launch Race, Flag Assault) |
| Mode blurbs, animated-panel captions `multiwinia_animdesc_*`, help pages `multiwinia_helppage_*`, `multiwinia_help_gametypetitle_*`, tutorial tab texts `multiwinia_help_*`, hint texts (`multiwinia_rmbhelp*`, `multiwinia_deselect*`, `multiwinia_shiftspeedhelp`, `multiwinia_officerterminate*`, `multiwinia_armourtoggle*`), `crate_drop`, "Wrong Game!" | text | §8, §9.3 | rewrite |
| Tutorial titles "Part 1: The Basics", "Part 2: Advanced Concepts" | text | `multiwinia_menu_tutorial1/2` | own titles |
| Help illustrations `mwhelp/general1–5.bmp`, `help_*_*.bmp`, mode cards `mode_*.bmp`, `control_camera*.bmp`, tab icons, `360_controller.bmp`, `background.bmp` | art | `data/mwhelp/` | new art |
| Animated-panel sprites (`data/animations/*.bmp`) and the six `.anim` choreographies | art / animation | `data/animations/` | new sprite set and choreographies |
| Map names / descriptions `<mapfile>_name`, `_description`, thumbnails `thumbnails/*.bmp` | level names / text / art | `code/UI/LevelSelectButton.h`, `code/UI/MapInfoButton.h`, `code/UI/MapImageButton.h` | new maps |
| Credits `multiwinia_credits_1..66`, `bootloader_credits_1..44`; beta-tester lists `data/multiwinia_betatesters.txt` (69) and `data/betatesters.txt` (38) — real people's names | text / personal data | `GameMenuWindow::ReadCredits`, `CreditsLoader::GenerateBetaTesterList` | own credits (with consent) |
| Publisher logo `textures/ivlogo.bmp`, "Introversion Software", `http://www.introversion.co.uk`, `http://store.introversion.co.uk`, `http://www.multiwinia.co.uk/`, `http://www.savedarwinia.com` | brand / URL | `LoadingScreen::RenderIVLogo`, `App::GetBuyNowURL`, `WebsiteButton`, `AboutDarwiniaWindow` | own publisher splash and URLs from config |
| Metaserver host `metaserver-mwdev.introversion.co.uk`, product id `METASERVER_GAMETYPE_MULTIWINIA = 4` | brand / service | `App::InitMetaServer`, `metaserver_defines.h` | own service endpoint / product id |
| Fixed demo key and blacklisted key strings, key prefixes `DEMO`, `AMBR` (distributor), hash prefix `hsh`, `authkey` / `authkey.dev` files | key strings / partner brand | `authentication.cpp`, §6 | platform entitlements |
| Box art `textures/multiwinia.bmp`, price art `textures/prices.bmp` (+ `prices_ambrosia.bmp`, `prices_steam.bmp`), buy-now copy `launchpad_buyme_*`, `multiwinia_buy_prices`, `multiwinia_buynow*` | art / brand / text | `GameMenuWindow::SetupBuyMePage` | own store art and copy |
| Rating notice `esrb_warning` | rating-board wording | `LoadingScreen::RenderIVLogo` | regenerate from the new game's actual rating |
| Profile / cheat names `AccessAllAreas`, `AttractMode`, `game_unlockall.txt` | names | `App::LoadProfile` | `__unlockall__`, `__attract__` |
| Darwinia location names (Garden, Containment, Mine, Generator, Yard, Escort, Pattern Buffer, Biosphere, Receiver, Temple, Prologue, launchpad) | names | leaderboard boards, attract scripts | generic sector names |
| "TrueSkill" | third-party brand (Microsoft) | `multiwinia_leaderboards_tslb`, `_tsr` | "Skill rating" |
| Controller glyphs `icons/button_*.bmp` and Xbox terminology (`ControlXBoxMenuCycle*`, XBLA pages) | third-party glyph art / brand | `GameMenuWindow::PreloadTextures`, `code/game_menu.cpp:BackPageButton::Render` | platform-neutral glyphs |
| Interface sounds `LobbyAmbience`, `MenuSelect`, `MenuCancel`, `MenuRollOver`, `ChatMessage`, `MouseError`, `CountdownTick`, `CountdownZeroTick`, `GameOver`, `YouWon`, `YouLost` (sample assets) | audio | §2.7, §3.23 | new SFX |
| Loader / intro / credits music (`speccyloader`, `matrix`, `fodder`, `pang`, `twoataris`, `dma_sc-visitors_from_dreams`, `tresk_-weeklies-_schroeders-failure`, `trash80_-_pain-fade-down`) — third-party chiptune artists | audio (third-party licence) | `data/sounds.txt` Music events | licensed / commissioned tracks |
| Intro captions `intro_1..10` (publisher credit, title, fictional server log, scientist character), loader texts `bootloader_*`, Matrix block-letter product-name mask, Spectrum / demo-group art (`program_darwinia.bmp`, `speccy_screen.bmp`, `dmacrew.bmp`) | text / brand / art | Appendix A.1, [30 §6](30_scripting_tutorial_and_cutscenes.md) | new texts and art |
| Distributor splash / video (`msn_oberon_combosplash.bmp`, `cinemaware.avi`), distributor origin tag `buka` | third-party brand | `OberonLoader`, `EgamesLoader`, `tools/build_multiwinia_retail_russian.bat` | none (drop) |
| Front-end UI strings ("Help and Options", "How To Play", "Settings", "Screen / Graphics / Sound / Other Options", "Press Enter To Chat", "Your control pad has been unplugged…", "Quit This Game?", "Location Menu", "Exit Game", "Open/CPU", "(DEMO)", error and auth status phrases) | text | `P17b_strings.csv` | generic equivalents; do not copy the string table verbatim |
| Defaults "NewPlayer", "NewServer" | text | §11.10 | keep generic or rename |

<!-- from 10_audio_design.md -->
### From 10_audio_design.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| All 1332 audio samples (954 `data/sounds/`, 378 `data/mwsounds/`; 898 live) and the archives carrying them (`sounds.dat`, `sounds2.dat`, `menusounds.dat`) | audio | `data/sounds/`, `data/mwsounds/`, `tools/sounds.txt`, `tools/menu_sounds.txt` | commission a new set from §12 / §14.2; name by function (`sfx_unit_die_01`) |
| Third-party music `tresk_-_excuses`, `tresk_-_faces-of-a-fashion`, `tresk_-_impact-of-silence`, `tresk_-weeklies-_schroeders-failure`, `trash80_-_pain-fade-down`, `dma_sc-visitors_from_dreams` | audio (third-party artists; not shipped with Multiwinia) | `data/sounds/`, `OTHER Music` blocks | `music_*` (new licence) |
| Retro-homage loader tunes `fodder`, `speccyloader`, `pang`, `matrix`, `twoataris` | audio (possibly derived from third-party game music) | `data/sounds/` | drop |
| Artist / track list `docs/retro music.txt` | text (third-party) | `docs/retro music.txt` | do not reuse |
| Signature ambience: lobby pads `MultiLobbyAmb1`–`10`, altitude bed `High1`–`7`, `evil`, `alien_lands3`, `rumble1`, `white_noise`, `shipheartbeat1` | audio | `data/mwsounds/`, `data/sounds/` | `amb_lobby_pad_01…`, `amb_altitude_01…`, `amb_ground_01…` |
| Voice / bark sets (`darwin_*`, `DSelect*`, `Attack*`, `Retreat*`, `Hit*`, `LaserHitDarwinian*`, `officer_*`, `squaddie_chatter*`, `GDSpeech*`, `YesSir*`, `Aye*`, `Attention*`, `Marching*`, `Formation*`, `DStrain*`) | audio (character voices) | `data/sounds/`, `data/mwsounds/` | `vo_unit_select_01`, `vo_unit_attack_01`, `vo_commander_ack_01` |
| Announcer stings `CrateReward_*` (incl. `CrateReward_Futurewinians`, `CrateReward_Rocketwinians`), `GameOver*`, `YouWon`, `YouLost2`, `Player_Capture_Statue`, `Enemy_Capture_Statue`, `KOH_Zone_*`, `Crate_Capture*` | audio + embedded IP names | `data/mwsounds/` | `vo_reward_<function>`, `vo_match_win`, `sting_zone_captured` |
| Source / event names `Darwinian`, `Officer`, `Squadie`, `SoulDestroyer`, `Virii`, `Sepulveda`, `MultiwiniaInterface`, `MultiwiniaZone`, reward names `Bitzkreig`, `Rocketwinians`, `Darwinians`, `Infection`, `Plague` | name | `data/sounds.txt`, `code/sound/soundsystem.cpp:SoundSourceBlueprint::GetSoundSourceName`, `code/worldobject/crate.cpp:Crate::GetName` | `Unit`, `Commander`, `Trooper`, `Narrator`, `MatchUI`, `CaptureZone`, `Reward_<function>` |
| Sample-group and file names embedding IP names (`darwiniandie`, `darwiniancreate`, `darwinnianonfire`, `darwinnianburncry`, `officercreate`, `CrateRewrad_Darwinians*`, `sepulveda_appear`, `koh_zone*`, `gdspeechgroup`, `souldestroyer*`, `redvirus_*`, `multilobbyambiences`) | name | `data/sounds.txt` | functional names (`unit_die`, `unit_spawn`, `zone_active`) |
| UI strings of the sound options (`dialog_soundoptions`, `dialog_soundlibrary`, `dialog_directsound`, `dialog_softwaresound`, `dialog_mixfrequency`, `dialog_11khz`, `dialog_22khz`, `dialog_44khz`, `dialog_numchannels`, `dialog_8channels`, `dialog_16channels`, `dialog_32channels`, `dialog_64channels`, `dialog_memoryusage`, `dialog_high`, `dialog_medium`, `dialog_low`, `dialog_swapstereo`, `dialog_hw3dsound`, `dialog_unavailable`, `dialog_realtimeeffects`, `dialog_apply`, `dialog_close`, `multiwinia_menu_back`), hard-coded "Master Volume", "Failed to set requested sound settings" | text | `code/interface/prefs_sound_window.cpp`, `code/UI/soundsliderbutton.h` | new string table ("Audio", "Output", "Mix rate", "Voices", "Swap L/R", "Master volume", "Apply", "Back") |
| Planning notes (`docs/multiwinia audio.docx`, `docs/sound changes.txt`) | text | `docs/` | do not reuse |
| `docs/MinDistAndVolume.jpg` (apparently from DirectX documentation) | art (third-party) | `docs/MinDistAndVolume.jpg` | redraw |

<!-- from 20_architecture.md -->
### From 20_architecture.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Multiwinia" (`APP_NAME`, window title, `DARWINIA_GAMETYPE "multiwinia"`, profile folder `…\Introversion\Multiwinia\`) | brand/name | `code/lib/universal_include.h`, `code/app.cpp:App::GetProfileDirectory`, `code/renderer.cpp` | `<GameName>` |
| "Darwinia" (`DARWINIA_VERSION`, `Darwinia*` achievement enum prefix, `darwinia_gl.exe`, `darwinia_vistaedition` string) | brand/name | `code/main.cpp:AppMain`, `code/app.h`, `targets/vs2005/Darwinia.vcproj` | `<GameName>` |
| "Introversion" (company folder, `http://store.introversion.co.uk`, metaserver host `metaserver-mwdev.introversion.co.uk`, `http://www.multiwinia.co.uk/`) | brand/endpoint | `code/app.cpp:App::GetBuyNowURL`, `App::InitMetaServer`, `code/main.cpp` | `<Studio>`, own endpoints |
| crash-report module id `0B9432AF`; Vista Game Explorer GUID | identifiers | `code/main.cpp:AppMain`, `DoVistaChecks` | own identifiers |
| archive names `init.dat`, `language.dat`, `main.dat`, `patch.dat`, `menusounds.dat`, `branding.dat`, `sounds.dat`, `sounds2.dat`; data files `game.txt`, `game_unlockall.txt`, `gestures.txt`, `origin`, `authkey.dev` | packaging | `code/app.cpp` | own packaging names |
| fonts `speccy`, `editor`, `square` | art | `code/app.cpp:App::SetLanguage` | licensed/open fonts |
| loading-screen art `textures/ivlogo.bmp` (publisher logo), spinner `sprites/darwinian.bmp`; rating text `esrb_warning` | art/text | `code/loading_screen.cpp`, `code/lib/resource.cpp` | own splash and spinner |
| sound events `EnterLocation`, `ExitLocation`, `EnterGlobalWorld` (Ambience), `LobbyAmbience`, `CountdownZeroTick`, `MenuSelect` (MultiwiniaInterface) and their samples | audio | `code/main.cpp`, `code/multiwinia.cpp`, `code/interface/mainmenus.cpp` | own audio events |
| achievement names Dominator, Hoarder, Hail To The King, Unclad Skies, Aggravated Assault, Master Player, Master Of Multiwinia, Explorer, Carnage, Blitz Master, Wrong Game, Genocide | names | `code/app.h` | own achievement names |
| special profile names `AccessAllAreas`, `AttractMode` | text | `code/app.cpp:App::LoadProfile`, `SetProfileName` | drop |
| team/unit names surfacing in this document's constants: "Darwinian", "Futurewinian", "Virii" | names | `code/location.cpp`, `code/team.cpp` | "Drone", "Neutral Hostile", "Swarm" |
| `autopatch.exe` (Introversion patcher) | tool | `code/UI/update_page_buttons.h:RunAutoPatcherButton` | own updater |

<!-- from 21_simulation_and_entity_model.md -->
### From 21_simulation_and_entity_model.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Darwinian" (class and entity type name, sound-event prefixes, `sprites/darwinian.bmp`, `data/shapes/darwinian.shp`) | name / art | `code/worldobject/darwinian.h`, `code/team.cpp:Team::RenderDarwinians` | "Citizen" / "Worker"; new sprite and model |
| "Multiwinia" (string keys `multiwinia_team_*`, `multiwinia_kills_*`, class `MultiwiniaZone`, `MultiwiniaOfficerOrders`) | name / text | `code/team.cpp:Team::GetTeamName`, `Team::Advance` | `team_name_*`, `kills_milestone_*`, `ControlZone`, `OrderMarker` |
| "Futurewinian" team and its colour role | name | `code/location.cpp:Location::InitFuturewinianTeam`, `code/team.cpp:Team::IsFuturewinianTeam` | "Elite faction" |
| "Virii" and `sprites/virii.bmp`, generated `sprites/viriifull.bmp` | name / art | `code/team.cpp:Team::Initialise`, `Team::RenderVirii` | "Swarm" sprite |
| Monster team "virus" name key `multiwinia_team_virus`, future team `multiwinia_team_future` | text | `Team::GetTeamName` | `team_name_hostile`, `team_name_elite` |
| Team colour names green, red, yellow, blue, orange, purple, cyan, pink and the RGB tables | design data (generic; reusable) | `code/multiwinia.cpp` (`teamColours`, `groupColours1/2`) | may be reused or re-tuned |
| Entity, building and effect class/type names of §2 (Squadie, SoulDestroyer, TriffidEgg, SpaceInvader, Officer, Harvester, Engineer, Armour, Nuke, Meteor, SpaceShip, DropShip, Tentacle, Tripod, Centipede, Spider, ArmyAnt, Egg, SporeGenerator, Lander, LaserTrooper, Shaman, AI, Tank; RadarDish, TrunkPort, SpawnPoint, PulseBomb, EscapeRocket, Statue, WallBuster, GodDish …) | names | §2.8 | generic role names (per-type placeholders in [02](02_units_and_entities.md), [03](03_buildings_and_world_objects.md), [04](04_crates_powerups_and_weapons.md)) |
| `textures/glow.bmp`, `textures/particle.bmp`, `textures/laser.bmp` used by shadows, route trails, lasers | art | `code/worldobject/entity.cpp:Entity::RenderShadow`, `code/routing_system.cpp:RoutingSystem::Render`, `code/location.cpp:Location::RenderWeapons` | new VFX textures |
| Spirit/soul concept and its rendering | design / art | `code/location.cpp:Location::RenderSpirits` | "essence orb" |

<!-- from 22_determinism_and_networking.md -->
### From 22_determinism_and_networking.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| `metaserver-mwdev.introversion.co.uk` | brand / service hostname | `code/app.cpp:App::InitMetaServer` | `directory.example.net` |
| `80.175.29.66` (route-probe address) | Introversion-owned IP [I] | `contrib/systemiv/lib/netlib/net_lib.cpp:GetLocalHostIP` | use OS interface enumeration |
| `ftp.introversion.co.uk`, benchmark upload account | brand / credentials | `code/benchmark.cpp` (credentials deliberately not reproduced) | none — do not reuse |
| `http://store.introversion.co.uk` | brand URL | `code/app.cpp:App::GetBuyNowURL` | the new title's store URL |
| `"Multiwinia"` (`APP_NAME`, GAMENAME on the wire, fallback server name) | name | `code/lib/universal_include.h`, `Server::Advertise` | `<NewTitle>` |
| `METASERVER_GAMETYPE_MULTIWINIA` = 4, `_DEFCON` = 2, "defcon-steam3" | product ids / names | `contrib/systemiv/lib/metaserver/metaserver_defines.h`, `steamauth_server.cpp` | new product id |
| Fixed demo key and two blacklisted keys | key strings | `contrib/systemiv/lib/metaserver/authentication.cpp` | none (do not copy) |
| Key prefixes `"DEMO"`, `"AMBR"` (Ambrosia distributor), hash prefix `"hsh"` | text / partner brand | `authentication.cpp` | platform entitlements |
| `authkey.dev` / `authkey` file names | text | `code/app.cpp` | — |
| Steam app id 1520 | partner id | `contrib/systemiv/lib/metaserver/steamauth_client.cpp` (dead) | — |
| Ambrosia Software credit | third-party name | `contrib/systemiv/lib/math/math_stubs.cpp` | — (do not copy) |
| UI phrases `multiwinia_error_serverfull`, `_kickedbyserver`, `_badpass`, `_connection`, `_syncerror`, `_lostconnection`, `_incompatible`, `_missingmap`, `_unknownmap`, `multiwinia_player_left`, `multiwinia_menu_connected`, `multiwinia_menu_attempts`, `multiwinia_update_available`, `dialog_duplicate_key`, `dialog_auth_*`, `dialog_steam_progress_*`, `authentication_demorestricted`, chat tokens `PLAYERJOIN`/`PLAYERLEAVE` | UI string ids (text in data/language) | `code/network/clienttoserver.cpp`, `code/renderer.cpp`, `code/UI/GameServerButton.h`, `code/multiwinia.cpp`, `authentication.cpp` | `net_error_server_full`, `net_error_kicked`, `net_error_bad_password`, `net_error_connection`, `net_error_desync`, `net_error_connection_lost`, `net_error_incompatible`, `net_error_missing_map`, `net_error_unknown_map`, `net_player_left`, … |
| Default server name `"NewServer"` | text | `Server::Advertise` | `"New Server"` |
| Map ids `MAPID_MP_KOTH_2P_1`, `_KOTH_3P_1`, `_KOTH_4P_1`, `_CTS_2P_1`, `_ROCKETRIOT_3P_1` (demo map list) | level names | `code/network/demo_limitations.cpp`, `code/mapcrc.h` | new map ids |
| Game-mode names (Domination/Skirmish, King of the Hill, Capture the Statue, Assault, Rocket Riot, Blitzkrieg, Tank Battle) carried in `GAMEMODE` | names | `code/multiwinia.h` | generic mode names |
| Identifier prefix `NET_DARWINIA_*` and readable codes "DARWINIA"/"CMD" | brand in identifiers | `code/network/network_defines.h`, `network_defines_debug.h` | `NET_GAME_*` |
| "Darwinian" in debug state lines (`DARWINIAN Id[…]`) and entity type names | name | `code/worldobject/darwinian.cpp:Darwinian::LogState` | "Worker" |
| Sync error report file names `syncerror-…`, `syncdiff-…` | generic text (not IP) | `ClientToServer::GetSyncFileName` | may be reused |
| MT19937 reference implementation (BSD-style licence, Matsumoto & Nishimura) | third-party code | `contrib/systemiv/lib/math/random_number.cpp`, `code/lib/math_utils.cpp` | `std::mt19937` or own PCG/xoshiro; attribution if reference code is copied |
| SHA-1 from the bundled unrar sources (seed, IFrame hash) | third-party code (unrar licence) | `contrib/unrar/sha1.cpp`, `code/network/server.cpp:GenSyncRandSeed`, `code/network/iframe.cpp` | any public-domain hash (xxHash, BLAKE3) |

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

<!-- from 25_textures_and_assets.md -->
### From 25_textures_and_assets.md

All shipped art, meshes, animation scripts, fonts, shaders, sounds and file names in scope are Introversion Software IP or third-party brands; nothing may be reused.

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| Darwinian / Multiwinian / Futurewinian sprites (`data/sprites/darwinian*.bmp`, `futurewinian.bmp`, `ghost.bmp`, `santahat.bmp`, `lasertrooper.bmp`, `egg.bmp`, `virii.bmp`, `brainwaves.bmp`, `nuke.bmp`, `sub*.bmp`, `sound.bmp`; `data/animations/darwinian*.bmp`; `data/textures/darwinian.bmp`) | art + character names | §3.3, §3.4 | `unit_basic`, `unit_hurt_1..4`, `unit_side`, `unit_alt`, `ghost_fx`, `hat_seasonal`, `creature_tail` |
| Dr Sepulveda portrait frames (`data/sepulveda/sepulveda{1,2,3,4,5}.bmp`) | character art | §3.9 | `advisor_face_idle_1..3`, `_blink`, `_talk` (or drop the feature) |
| Studio and game logos/titles (`data/textures/ivlogo.bmp`, `data/icons/mwlogo.bmp`, `data/textures/mwtitle.bmp`, `data/textures/multiwinia.bmp`, `data/textures/plusmenu/*`, `data/icons/darwin_research_associates.bmp` — in-fiction "Darwin Research Associates", `data/textures/program_darwinia.bmp`) | brand | §3.1, §3.6 | `studio_logo_placeholder`, `game_title_wordmark`, `in_fiction_corp_logo` |
| Third-party brands: `data/textures/xboxlivearcade.bmp` (Microsoft), `data/mwhelp/360_controller.bmp` and `data/icons/button_*.bmp` (Xbox 360 trade dress), `data/textures/ambrosia_logo.bmp`, `prices_ambrosia.bmp` (Ambrosia Software), `prices_steam.bmp` (Valve), `msn_oberon_combosplash.bmp` (MSN/Oberon), `data/shapes/carrybillboard_{pcg,tpcg,xfire,zone}.shp` and their `.max` sources (magazine/service names [I] from suffixes), `data/textures/dmacrew.bmp`, `speccy_screen.bmp` (demo-scene / ZX Spectrum homage) | third-party brand | §3, §4 | do not recreate; `input_glyph_<button>`, no store or magazine branding |
| All 139 meshes and 133 `.max` sources (names such as `souldestroyer*`, `darwinian.shp`, `officer.shp`, `trunkport*`, `spiritreceiver*`, `masterspawnpoint.shp`) | 3-D art + names | §4 | role-named meshes (§7.3) |
| Map thumbnails and map names (`data/thumbnails/mp_*.bmp`) | level art + names | §3.7 | `map_<mode>_<players>p_<n>_thumb` |
| Help illustrations, mode pictures and panel animations (`data/mwhelp/*.bmp`, `data/animations/*`) | art + layout | §3.4, §3.8 | `help_<mode>_<page>`, `mode_<mode>` |
| Terrain ramps and their names (`landscape_icecaps`, `launchpad`, `containment`, `stronghold`, `jailbreak`) | art (palette) + names | §3.5 | `terrain_ramp_<theme>`, `water_<theme>`, `waves_<theme>` |
| VFX and UI textures (`glow`, `starburst`, `cloudyglow`, `laser*`, `beam`, `bubble`, `lightning`, `particle`, `triangleoutline`, `shapewireframe`, `interface_*`, `menuhighlight`, cursor/marker/crate/task icons, `gesture_*`) | art | §3.1, §3.6 | `fx_*`, `ui_*`, `cursor_<state>`, `icon_<ability>` |
| Bitmap fonts `speccy`, `editor`, `square` (all atlases and legacy font sheets) | typeface art | §2.15 | open-licence fonts rasterised at run time |
| HLSL shaders (`data/shaders/*`) | code | §3.10 | re-implement from [24](24_rendering.md) |
| All 1332 sound samples (898 live), including third-party music (`tresk_*`, `trash80_*`, `dma_sc*`) and retro loader tunes (`fodder`, `speccyloader`, `pang`, `matrix`, `twoataris`) | audio | §5 | `sfx_<event>`, `vo_<role>_<line>`, `amb_<bed>`, `music_*` commissioned |
| Voice/bark and announcer sets (`darwin_*`, `DSelect*`, `Attack*`, `Retreat*`, `YesSir*`, `GDSpeech*`, `CrateReward_*` incl. `CrateReward_Futurewinians`, `CrateReward_Rocketwinians`, `GameOver*`, `YouWon`, `YouLost2`, `KOH_Zone_*`) | audio + embedded names | §5 | `vo_unit_select_01`, `vo_reward_<function>`, `vo_match_win`, `sfx_zone_captured` |
| File stems embedding IP names: "darwinian", "multiwinia", "sepulveda", "futurewinian", "darwinia", "souldestroyer", "virii", "multilobbyamb" | names | throughout | role-based names |
| Encoder vendor strings (`libVorbis`, `aoTuV`) | metadata (BSD) | §5.2 | none needed |

<!-- from 26_animation_and_vfx.md -->
### From 26_animation_and_vfx.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| Six `.anim` choreographies (domination, koth, cts, assault_defend, rocket, blitz) | animation/art | `data/animations/*.anim` | new explainer animations per mode |
| 25 panel sprites (`darwinian.bmp`, `darwinian_ouch4.bmp`, `flag*.bmp`, `wmd.bmp`, `statue.bmp`, `rocket.bmp`, `island.bmp`, …) | art | `data/animations/*.bmp` | new 2-D icons |
| "Darwinian" sprite family incl. ouch frames, side view, Futurewinian, santa hat, brainwaves, ghost | art / character | `data/sprites/darwinian*.bmp`, `futurewinian.bmp`, `santahat.bmp`, `brainwaves.bmp`, `ghost.bmp` | "Unit" sprite set (`unit_*.png`) |
| Particle, glow, starburst, laser, lightning, muzzle-flash, shape-wireframe textures | art | `data/textures/particle.bmp`, `glow.bmp`, `starburst.bmp`, `laser.bmp`, `lightning.bmp`, `muzzleflash.bmp`, `shapewireframe.bmp`, `cloudyglow.bmp`, `bubble.bmp`, `subversion.bmp`, `laser-long.bmp` | procedurally generated sprites |
| Officer banners and goto arrow | art | `data/icons/banner_none.bmp`, `banner_follow.bmp`, `banner_formation.bmp`, `banner_absorb.bmp`, `arrow.bmp` | new order-state icons |
| Gesture guide and gesture icons | art | `data/icons/gestureguide.bmp`, `gesture_*.bmp`, `gesture_alpha.bmp` | drop (feature unused) |
| Creature names used in effect descriptions (Virii, SoulDestroyer, SporeGenerator, Triffid, Centipede, Spider, Tripod, Futurewinian, Zombie) | name | [02](02_units_and_entities.md) | Swarm, Reaper, Spawner, Launcher-plant, Segment-crawler, Strider, Tri-walker, Abductee, Ghost |
| Mode panel captions `multiwinia_animdesc_*`, mode titles | text | `data/language/english.txt` | new copy |
| `animator.exe` / `animator.dat` | software/tool | repository root | own panel editor |

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

<!-- from 29_data_formats_reference.md -->
### From 29_data_formats_reference.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Multiwinia", "Darwinia", "Darwinian(s)", "Futurewinian", "Rocketwinians" (in `multiwinia.txt`, `stats.txt`, `sounds.txt` source names such as `MultiwiniaInterface`, file names `darwinian*.bmp`) | name / brand | `data/multiwinia.txt`, `data/stats.txt`, `data/sounds.txt`, `code/worldobject/crate.cpp:Crate::GetName` | "the game", "Sprite", "Future Sprite", "Rocket Sprites" |
| Game-type names `Domination`, `KingOfTheHill`, `CaptureTheStatue`, `Assault`, `RocketRiot`, `Blitzkrieg`, `TankBattle` (as this specific set) | name | `data/multiwinia.txt` | `ZoneControl`, `HoldTheCentre`, `CarryTheIdol`, `AttackDefend`, `RocketRace`, `ZoneChain`, `ArmourMatch` |
| Map file names `mp_*` and location names (`garden`, `containment`, `mine`, `generator`, `yard`, `escort`, `pattern_buffer`, `biosphere`, `receiver`, `temple`, `launchpad`, `sandbox`) | name | `data/levels/`, `data/game.txt` | `map_<mode>_<players>p_<n>`, `region_<n>` |
| Every phrase text of `data/language/*.txt` (UI strings, credits, help) | text | `data/language/` | new text; keep only the id scheme |
| Beta-tester and credit names | text (personal names) | `data/betatesters.txt`, `data/multiwinia_betatesters.txt`, `multiwinia_credits_*` | never reuse; own credits |
| Dr Sepulveda portraits, `OTHER Sepulveda` sound source, `puritycontrol_*` dialogue, `buyme` cut-scene | character / art / text | `data/sepulveda/*`, `data/sounds.txt`, `data/purity/buyme.txt` | "Mentor portrait 1–5"; do not implement purity control |
| "Purity Control" data set | text / name | `data/purity/*` | omit |
| Building / entity / research type names used as file keywords (`SpawnPointMaster`, `TrunkPort`, `SoulDestroyer`, `Virii`, `Triffid`, …) | name | §4.1.10, §4.1.13, §4.4 | new neutral ids (`SpawnHub`, `Portal`, …) |
| 139 `.shp` models and 133 `.max` sources | art | `data/shapes/`, `data/shapes/source/` | new models (glTF) |
| 506 BMP images (icons, help pictures, thumbnails, sprites, textures, font pages, terrain ramps, animation sprites) | art | `data/**/*.bmp` | new art; terrain ramps may be regenerated procedurally |
| 6 `.anim` choreographies | art / animation | `data/animations/*.anim` | new explainer animations |
| 1 332 Ogg samples incl. licensed retro tracks by "Tresk", "Trash80", "DMA-SC" (file names) | audio (incl. third-party music) | `data/sounds/`, `data/mwsounds/` | new audio library |
| `sounds.txt` / `effects.txt` authored mix data | data (creative tuning) | `data/sounds.txt`, `data/effects.txt` | re-author; effect *types* (chorus, echo …) are generic |
| Gesture training vectors and demo strokes | data | `data/gestures.txt`, `data/gesture_demos/*` | retrain (the Rubine algorithm is public) |
| Value-driver names `Pecan`, `Hummingbird`; test-bed name `TESTBED_GodOfCode`; profile names `AccessAllAreas`, `AttractMode` | name | `code/lib/input/inputdriver_values.inc`, `testbedconfig.txt`, `code/app.cpp` | `AlwaysTrue`, `FeatureFlagA`, `DevProfile` |
| Obfuscation magic `redshirt2` and its key table; exporter name "Futurewar"; author string "Introversion Software" | text / brand | `contrib/systemiv/lib/filesys/text_stream_readers.cpp`, `contrib/systemiv/lib/filesys/text_file_writer.cpp`, `tools/ShapeExporter/ShapeExporter.cpp` | none (no obfuscation; own tool names) |
| Folder / bundle names `Introversion\Multiwinia`, `uk.co.introversion.*` | brand | `code/app.cpp:App::GetProfileDirectory`, `App::GetPreferencesPath` | `<Company>\<Game>` |
| Auth-key format, demo-key literal, `DEMO`/`AMBR` prefixes | brand / credential | `contrib/systemiv/lib/metaserver/authentication.cpp` | own licensing scheme |
| Crash tool name `MultiwiniaSupport` | brand | `MultiwiniaSupport.ini`, `contrib/XCrashReport` | `<Game>Support` |
| `animator.exe`, `animator.dat` (tool binary and its UI data) | software | repository root | own tooling |
| `banner_template.psp` | art source | `data/icons/banner_template.psp` | omit |
| Third-party formats and names: RAR/UnRAR (format licence), DirectSound effect names `DSound_*` (Microsoft), Paint Shop Pro, Photoshop `.grd`, amBX (`ambx.dat`) | third-party | as listed | avoid RAR; generic effect names |

---

<!-- from 30_scripting_tutorial_and_cutscenes.md -->
### From 30_scripting_tutorial_and_cutscenes.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| Dr Sepulveda (narrator name, persona, `sepulveda_*` phrase ids, `SepulvedaHelp` control) | name / character | `code/sepulveda.*`; `data/input/input_preferences.txt` L255 | "Advisor"; `AdvisorHelp` |
| Sepulveda portraits `sepulveda1..5.bmp` (256 × 256, 8-bit) | art | `data/sepulveda/` | placeholder advisor portrait set (3 neutral, eyes shut, mouth open) |
| All narrator/cutscene dialogue: 267 script `Say` lines, `tutorial_*`, `launchpad_tutorial_*`, `help_*`, `research_explanation_*`, `outro_*`, `yardend_*`, `temple_*`, `containment*`, `pixelmines_*`, `launchpad_success_*`, `launchpad_middemo_*`, `tutorial_skipped`, `dialog_skiptutorial`, `sepulveda_pressreturn*`, `help_pressh` | text | `data/language/english.txt`; `data/scripts/*` | rewrite all narration |
| The 42 cutscene scripts and their camera choreography | text / data | `data/scripts/*` | author new scripts |
| Darwinia location names used by scripts: biosphere, containment, generator, mine, yard, receiver, pattern_buffer, temple, launchpad, garden, escort, labs, "heaven" | name | `data/scripts/*`; `data/game*.txt` | generic sector names |
| Darwinia camera mount names (`gamestart`, `tutorial1..7`, `incubator1..3`, `attack6`, `attack7`, `attack7b`, `wherenext1`, `booster1`, `booster2`, `launch1`, `launch2`, `lastshot`, `lastshot2`, `attract1..6`) and the conventions `player0..3`, `start`, `CamPosBefore` | level data / names | `data/levels/*.txt`; `code/level_file.h` | `spawn_view_<n>`, `default_view`, `previous_view` |
| Multiwinia tutorial and help text: `multiwinia_help_*`, `multiwinia_help_title_*`, `crate_drop`, `multiwinia_menu_tutorial_short`, `multiwinia_menu_tutorial1/2` ("Part 1: The Basics", "Part 2: Advanced Concepts") | text | `data/language/english.txt` | rewrite; own titles |
| "Darwinian", "Multiwinian", "Multiwinia", "Darwinia" inside help strings | brand / name | `multiwinia_help_*` phrases | "unit" / product placeholder |
| Tutorial camera-controls picture `control_camera.bmp`, `control_camera_french.bmp` | art | `data/mwhelp/` | new control diagram |
| Tutorial map layouts `mp_koth_2P_1`, `mp_koth_3P_1` and their hard-coded building ids | level / name | `data/levels/`; `code/multiwiniahelp.cpp` | new tutorial maps with tagged objects |
| Intro captions `intro_1..10` (publisher credit, title, fictional server log, scientist character name) | text / character | `data/language/english.txt` L1412–1421 | new intro text |
| Logo splash texture `icons/darwin_research_associates.bmp` and sound `ShowLogo` | brand / art / audio | `code/renderer.cpp`; `code/script.cpp` | product logo placeholder |
| Demo-end prompts `multiwinia_spacetoexit`, `multiwinia_atoexit`; ghost and creature sprites `sprites/ghost.bmp`, `sprites/darwinian.bmp` | text / art | `code/demoendsequence.cpp` | new text and sprites |
| Music cues `Cutscene1..5`, `EndSequence`, `Demo2Intro`, `Demo2Mid`, `StartSequence` and their samples (third-party chiptune artists for `StartSequence`) | audio | `data/sounds.txt`; `data/sounds/*.ogg` | licensed/commissioned score; events `cue_<n>`, `intro` |
| Narrator UI sounds `Appear`, `Disappear`, `TextAppear` | audio | `code/sepulveda.cpp` | new SFX |
| God-ray beam texture `textures/godray.bmp` | art | `SepulvedaHighlight::Render` | generic beam |
| Gesture demos `squad.txt`, `engineer.txt`, `officer.txt`, `armour.txt`, `airstrike.txt`, `controller.txt`, `grenade.txt`, `rocket.txt` | data | `data/gesture_demos/` | not needed |
| Profile / cheat names `AccessAllAreas`, `AttractMode`, `game_unlockall.txt`; `PurityControl` kill switch and buy-now URL | name / behaviour | `code/script.cpp`; `code/app.cpp` | drop |

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
| Map names in `MAPID_*` (87, e.g. `MAPID_MP_KOTH_2P_1`) and in inclusion lists | name (level names) | `code/mapcrc.h`, the six inclusion lists in `tools/` (§8.6) | new map names |
| Third-party code: crash-report sample code (XCrashReport, public domain), `GetWinVer`/`MiniVersion` (public domain), unrar (RARLAB licence), `Rar.exe`, UPX, GnuWin32 grep/sed, 7-Zip, PuTTY `pscp`, NSIS SDK samples, Loki Setup, DirectX redistributable cabinets, AVIGenerator sample | third-party | `contrib/XCrashReport/*`, `code/lib/GetWinVer.cpp`, `contrib/unrar/*`, `tools/*.exe`, `tools/NSIS Installer/*`, `tools/loki/*`, `code/lib/avi_generator.cpp` | use the platform/standard equivalents named in §15; respect their licences |

<!-- from 32_debug_test_and_performance.md -->
### From 32_debug_test_and_performance.md

| Item | Kind | Ref | Neutral placeholder |
|---|---|---|---|
| "Multiwinia" (`APP_NAME`) in the debug-log banner, black-box header and `DARWINIA_VERSION_STRING` in benchmark files | brand | `code/lib/universal_include.h`, `contrib/systemiv/lib/debug_utils.cpp:AppGenerateBlackBox`, `code/benchmark.cpp` | product-name constant of the new game |
| Profile folder `Introversion\Multiwinia\`, registry key `HKCU\Software\Introversion\Multiwinia` | brand | `code/app.cpp:App::GetProfileDirectory`, `contrib/XCrashReport/RegistryDefines.h` | own company / product folder and key |
| Introversion FTP host and hard-coded FTP account | brand / credential | `code/benchmark.cpp:BenchMark::Upload` | none — token-based HTTPS telemetry endpoint; never embed credentials |
| Support tool name `MultiwiniaSupport.exe`, crash-support e-mail address and crash-report host | brand / contact | `contrib/XCrashReport/CrashFileNames.h`, `contrib/XCrashReport/EmailDefines.h`, `tools/multiwiniasupport/MultiwiniaSupport.exe` | own crash reporter and endpoint |
| Dr Sepulveda face cheat and its images | character / art | `code/clouds.cpp:Clouds::RenderFace`, `data/sepulveda/sepulveda1.bmp` (and siblings) | drop |
| Cheat captions "Spawn Green/Red/Yellow/Blue/Virus/Future", "Spawn Virii", "Spawn Spirits", "Show Dr S Face"; campaign names "generator", "mine", "receiver", "pattern_buffer"; "Enable Generator and Mine", "Enable Receiver and Buffer", "Open All Locations" | text / names | `code/interface/cheat_window.cpp` | "Spawn Team 1–4", "Spawn Hostile", "Spawn Souls"; drop the campaign cheats |
| All literal debug captions ("Kill All Enemies", "Allow Arbitrary Placement", "Give all research", "Clear Resources", "Spawn Ports", "GoodCrate", "Profile (F6)", "Dbg Cam (F2)", "Toggle glFinish", "Reset History", "Pokey Playground", the network-window labels) | text | `code/interface/cheat_window.cpp`, `code/interface/debugmenu.cpp`, `code/interface/profilewindow.cpp`, `code/interface/network_window.cpp` | rewrite |
| "DWs" (Darwinians) in the FPS overlay | name | `code/renderer.cpp:Renderer::RenderFrame` | "Units" |
| Testbed captions `start_testbed_server` "Testbed Server", `start_testbed_client` "Testbed Client"; English mode names written to `TestBedLog.txt` ("Domination", "King Of The Hill", "Capture the Statue", "Assault", "Rocket Riot", "Blitzkrieg") | text | `data/language/english.txt`, `code/UI/GameMenuWindow.cpp:GameMenuWindow::UpdateTestBed` | own mode names ([01](01_game_modes.md) register) |
| Stress maps "Stress Test" (+ description) and "Assault Stress Test" | level / text | `data/levels/mp_benchmark_1.txt`, `data/levels/mp_assault_stress.txt`, `data/language/english.txt` | `stress_test_01`, `stress_test_assault_01` |
| Third-party code: exception handler (Bruce Dawson, modified by Hans Dietrich) and XCrashReport; `AVIGenerator` (Jonathan de Halleux); NcFTP's `ncftpput`; MT19937 (Matsumoto–Nishimura) | third-party licence | `code/lib/exception_handler.cpp`, `contrib/XCrashReport/XCrashReport.cpp`, `code/lib/avi_generator.cpp`, `tools/build_multiwinia_hwcompat.bat`, `contrib/systemiv/lib/math/random_number.cpp` | platform crash APIs / own implementations |
