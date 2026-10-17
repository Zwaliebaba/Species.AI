# Collected "ip" sections from all drafts

<!-- from P01_app_core -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Multiwinia" (product name, profile folder `Introversion\Multiwinia\`, `DARWINIA_GAMETYPE "multiwinia"`) | brand/name | `code/app.cpp:GetProfileDirectory`, `code/lib/universal_include.h` | "<GameName>" |
| "Darwinia" (`DARWINIA_VERSION`, Vista rich header game name, achievement enum prefix) | brand/name | `code/main.cpp:AppMain`, `code/app.cpp:SaveRichHeader`, `code/app.h` | "<GameName>" |
| "Introversion" (company folder, store URL `http://store.introversion.co.uk`, metaserver host `metaserver-mwdev.introversion.co.uk`, websites `www.multiwinia.co.uk`, `www.darwinia.co.uk`) | brand | `code/app.cpp:GetBuyNowURL/InitMetaServer`, `code/main.cpp` | "<Studio>", own endpoints |
| "Darwinian" (unit name; sprite `sprites/darwinian.bmp`) | name/art | `code/team.cpp:RenderDarwinians` | "Drone" |
| "Virii" / virus team (`sprites/virii.bmp`, `sprites/viriifull.bmp`, `multiwinia_team_virus`) | name/art/text | `code/team.cpp` | "Swarm" |
| "Futurewinian" team (`multiwinia_team_future`) | name/text | `code/team.cpp:GetTeamName`, `code/location.cpp` | "Neutral Hostile" |
| Team colour names `multiwinia_team_{green,red,yellow,blue,orange,purple,cyan,pink}`, `teamtype_2` ("CPU"), `teamtype_left`, `teamtype_demo` | UI text | `code/team.cpp:GetTeamName` | own strings |
| Kill-milestone strings `multiwinia_kills_500…5000`, `crate_drop`, `multiwinia_presstotalk`, `error_incompatible`, `error_parental`, `darwinia_vistaedition` | UI text | `code/team.cpp`, `code/location.cpp`, `code/main.cpp` | own strings |
| Achievement names Dominator, Hoarder, Hail To The King, Unclad Skies, Aggravated Assault, Master Player, Master Of Multiwinia, Explorer, Carnage, Blitz Master, Wrong Game, Genocide | names | `code/app.h` enum | own achievement names |
| Game-mode names Domination ("Skirmish"), King of the Hill, Capture the Statue, Assault, Rocket Riot, Blitzkrieg ("Blitzkreig"), Tank Battle | names | `code/app.cpp:IsGameModePermitted` | descriptive neutral names |
| Fonts `speccy`, `editor`, `square` (bitmap font textures) | art | `code/app.cpp:SetLanguage` | own fonts |
| Textures `textures/glow.bmp`, `textures/laser.bmp` | art | `code/team.cpp`, `code/location.cpp` | own art |
| Data archives `init.dat`, `language.dat`, `main.dat`, `patch.dat`, `menusounds.dat`, `branding.dat`, `sounds.dat`, `sounds2.dat`, `game.txt`, `game_unlockall.txt`, `gestures.txt` | asset packaging | `code/app.cpp` | own packaging |
| Sound event names (`EnterLocation`, `LobbyAmbience`, `GivenOrdersFromPlayer`, …) and the samples bound to them | audio | §Cross-partition P16 | own audio |
| Special profile names `AccessAllAreas`, `AttractMode` | text | `code/app.cpp:LoadProfile` | – (drop) |
| Dr Sepulveda references (`m_sepulveda`, `PlayerSkipsMessage`, `Say("PrimaryObjectivesComplete")`) | character | `code/main.cpp`, `code/location.cpp` (dead) | – (drop) |
| Vista Game Explorer GUID `F58175C7-E99C-4151-800D-7BB5E89E4960`; crash-report id `0B9432AF` | identifiers | `code/main.cpp:DoVistaChecks`, `AppMain` | own ids |
| Unit/building names used in control code: Officer, Armour, Engineer, Shaman, Harvester, Tank, InsertionSquad/Squaddie, RadarDish, GunTurret, TrunkPort, SpawnPoint, SolarPanel, PulseBomb, ControlStation, WallBuster, FenceSwitch, ChessPiece | names | throughout | generic descriptive names (e.g. "Commander", "Transport", "Teleporter") |

<!-- from P02_game_modes -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Multiwinia", "Multiwinian(s)", "Darwinian" | brand/name | strings (`multiwinia_rr_status_b`, …), class names | "the game", "units"/"Sprites" |
| Mode names Domination, King of the Hill, Capture the Statue, Assault, Rocket Riot, Blitzkrieg, Tank Battle | name | `data/multiwinia.txt`, `multiwinia_gametype_*` | Spawn Control, Zone Control, Relic Run, Siege, Launch Race, Flag Chain, Armour Duel |
| "Evilinians", "Futurewinians" | name/characters | `multiwinia_team_virus`, `multiwinia_team_future` | Rogue faction, Future faction |
| "WMD", "WMD Control Panel", "Pulse Bomb", "Trunk Port", "Retribution", "MEGA RAGE", "SPAWN MANIA", "Hot Feet" | name | strings csv, level options | Core Device, Uplink Console, Warp Gate, Comeback Bonus, Frenzy, Spawn Surge, Speed Boost |
| Achievement names Dominator, Hoarder, Hail To The King, Unclad Skies, Aggravated Assault, Master Player, Master of Multiwinia, Explorer, Carnage, BlitzMaster, Wrong Game, Genocide | name | `code/app.h` enum | Generic: "Total Control", "Collector", "Crowned", "Clean Launch", "Iron Defence", "Giant Slayer", "Completionist", "Tourist", "Massacre", "Lightning War", "Friendly Fire", "Annihilator" |
| All 87 UI strings used by these files (e.g. "Defend the WMD!", "Capture solar panels to begin refueling rocket", "It's a DRAW!") | text | `OUT/_work/drafts/P02_game_modes_strings.csv`, `data/language/*.txt` | rewrite all copy |
| Level ids/file names `mp_<mode>_<n>P_<k>` and their display names; PDLC1/PDLC2 map names | name | `data/levels/level_enum.txt`, X4 | new map names |
| Shapes `rocket.shp`, `fuelgenerator.shp`, `fuelgeneratorpump.shp`, `fuelpipe.shp`, `fuelpipebase.shp`, `fuelstation.shp`, `trunkport.shp`, `trunkport_mp.shp`, `pulsebomb.shp`, `controlstation.shp`, `pulsebombcontrolstation.shp`, `carrystatue/squaddie/globe/telescope/weights/cube/teapot/billboard.shp` (+ unused `carrydollar`, `carryspider`, `carrygun`, billboard variants `_pcg`, `_tpcg`, `_xfire`, `_zone` — the latter reference third-party brands) | art | `data/shapes/` | new models |
| Textures `icons/rocketstatuspanel.bmp`, `icons/rocketcracked.bmp`, `icons/flag.bmp`, `flag_up.bmp`, `flag_down.bmp`, `flag_locked.bmp`, `icons/arrow.bmp`, `sprites/darwinian.bmp`, `textures/laser.bmp`, `laser-long.bmp`, `fuel.bmp`, `starburst.bmp`, `glow.bmp`, `laserfence.bmp`, `interface_grey.bmp`, `muzzleflash.bmp` | art | cited render code | new textures |
| Sound events listed in Cross-partition P16 and their samples | audio | `data/sounds.txt` (P16) | new audio |
| Darwinia script `launchpad_victory.txt` | text/script | `EscapeRocket::AdvanceIgnition` [B:LEGACY] | — |
| `data/multiwinia_betatesters.txt` (real people's names/handles) | text/personal data | data file | do not ship |
| Colour palette and names (Green, Red, Yellow, Blue, Orange, Purple, Cyan, Pink) | generic | §3.8 | free to reuse (generic) |

<!-- from P03_entity_core -->
## IP register
| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Darwinian" (unit/class name, sounds "Darwinian …") | name | `code/worldobject/darwinian.*`, sound event prefixes | "Sprite" / "Citizen" |
| "Multiwinian" (manual term) | name | `docs/multiwinia_manual.pdf` | "Citizen" |
| "Futurewinian" | name | `Darwinian::m_futurewinian`, `sprites/futurewinian.bmp` | "Elite Citizen" |
| "Officer" banners `icons/banner_none/follow/formation/absorb.bmp` | art | `Officer::RenderFlag` | new banner art |
| `officer.shp`, `boxkite.shp`, `tripod_leg_part.shp`, `data/shapes/darwinian.shp` | art (3D) | `Officer::Begin`, `BoxKite::BoxKite`, `tripod.cpp`, `Entity::ConvertShapeColoursToTeam` | new models |
| `sprites/darwinian.bmp`, `darwinian_ouch1..4.bmp`, `darwinian_side.bmp`, `santahat.bmp`, `brainwaves.bmp`, `futurewinian.bmp` | art (sprites) | `Darwinian::Render` | new sprites |
| `textures/glow.bmp`, `bubble.bmp`, `laser.bmp`, `starburst.bmp`, `particle.bmp`, `icons/widearrow.bmp` | art | Darwinian/Officer/Spirit/Routing render | new VFX textures |
| Spirit/soul visual (green/team-coloured double-quad soul) and BoxKite (spirit-guiding kite) concepts | art/design | `Spirit::Render`, `BoxKite` | "essence orb" |
| Sound event names ("SeenThreatAttackHorn", "VictoryJump", "LiftBuilding", "SetOrderAbsorb", …) and their samples | audio | `ListSoundEvents` (samples bound in `data/sounds.txt`, X2b) | new event names/samples |
| Entity type names "SoulDestroyer", "Virii", "Squadie", "Tripod", "Centipede", "SpaceInvader", "TriffidEgg", "Shaman", "Harvester", "Tank", "Nuke", "Meteor", "SpaceShip", "DropShip", "Engineer", "LaserTroop", "ArmyAnt", "Spider", "Egg", "SporeGenerator", "Lander", "Tentacle", "AI" | name | `Entity::GetTypeName`, `data/stats.txt` | generic role names |
| Language phrase keys `entityname_<Type>`, `multiwinia_kills_*` | text | `Entity::GetTypeNameTranslated`, `team.cpp` | new string table |
| Christmas santa-hat mod | design/art | `Darwinian::Render`, `Location::ChristmasModEnabled` | optional seasonal cosmetic |
| "Dr Sepulveda" lines (`officer_notwalkable`) | text/character | `Officer::SetOrders` (dead under `USE_SEPULVEDA_HELP_TUTORIAL`) | omit |

<!-- from P04_entities_military -->
## IP register
| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Darwinian" / "Multiwinian" (unit name, text "Creating Darwinians", "Darwinia Hit, %d Dead") | name/text | `shaman.cpp:Shaman::Render`, `nuke.cpp:Nuke::RenderDeaths` | "Sprite-folk" / "Citizen" |
| "Multiwinia" | brand | throughout | project codename |
| "Insertion Squad" / "Squadie" | name | `insertion_squad.h` | "Strike Team" / "Trooper" |
| "SpaceInvader" (bomber class name; echoes Taito's *Space Invaders* trademark) | name/brand | `airstrike.h`, `spaceinvader.shp` | "Bomber" |
| "Shaman", summonable "Soul Destroyer", "Virii", "Spore Generator", "Triffid" (Wyndham), "Centipede", "Army Ant" | names | `shaman.cpp`, `shaman_interface.cpp` | "Mystic", "Wraith", "Swarmers", "Spore Pod", "Carnivore Plant", "Crawler", "Ant" |
| "Futurewinian" team | name | `Location::GetFuturewinianTeamId` (referenced) | "Neutral future team" |
| Achievement "Wrong Game" (`DarwiniaAchievementWrongGame`, `dialog_wronggame`) | text | `Nuke::AdvanceToTarget` | own achievement set |
| String ids `crate_message_run_airstrike`, `crate_message_auto_airstrike`, `crate_message_run_napalm`, `crate_message_auto_napalm`, `crate_message_run_nuke`, `crate_message_auto_nuke`, `engineer_idle`, `engineer_towaypoint`, `engineer_tospirit`, `engineer_toincubator`, `engineer_reprogramming`, `engineer_researching`, `engineer_bridge`, `gc_setturret`, `gc_move`, `gc_cancel`, `gc_deselect` (text content in language files) | text | `crate.cpp`, `engineer.cpp`, `harvester.cpp`, `control_help.cpp` | rewrite all strings |
| Shapes `throwable.shp`, `missile.shp`, `turretshell.shp`, `emptyshell.shp`, `flying_egg.shp`, `spaceinvader.shp`, `armour.shp`, `squad.shp`, `tank_body.shp`, `tank_turret.shp`, `harvester.shp`, `engineer.shp`, `officer.shp` | art | §13.3 | new meshes |
| Textures/sprites/icons listed in §13.3 (`starburst`, `glow`, `laser`, `subversion`, `muzzleflash`, `laser-long`, `bubble`, `shaman_background`, `nuke`, `sub`, `subshadow`, `nukesymbol`, `icon_nuke`, `icon_airstrike`, `icon_engineer`, `shaman_*`, `mouse_selection`, banners) | art | §13.3 | new art |
| Sound events and their sample banks (§13.1, bound in `data/sounds.txt`) | audio | §13.1 | new audio; event names may be kept as neutral ids |
| Submarine-launched multi-warhead nuke with ground "nuke symbol" | design expression | §3 | acceptable mechanic; new presentation |

<!-- from P05_entities_creatures -->
## IP register
| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Virii" (creature name, `TypeVirii`, `stats.txt` row) | name | `code/worldobject/virii.h`, `data/stats.txt` | "Crawler" |
| "Soul Destroyer" | name | `code/worldobject/souldestroyer.h` | "Reaper Serpent" |
| "Triffid" / "Triffid egg" (also a third-party literary term) | name | `code/worldobject/triffid.h` | "Pod Launcher" / "Seed Pod" |
| "SporeGenerator" | name | `code/worldobject/sporegenerator.h` | "Brood Drifter" |
| "Centipede", "Spider", "ArmyAnt", "AntHill", "Tripod", "Tentacle", "Lander", "Egg" (generic words; the specific designs are IP) | name/design | respective headers | keep generic nouns, redesign creatures |
| "Futurewinian(s)" | name/character | `code/location.cpp:Location::GetFuturewinianTeamId`, `code/worldobject/spaceship.h` comments | "Visitors" |
| "Darwinian(s)" (abducted/dropped, captured by ants) | name/character | throughout | "Sprites" / "Units" |
| "SPAM" / "Spam" infection, `researchitem.shp` pod | name/brand-joke | `code/worldobject/spam.h` | "Contagion" / "Blight" |
| "Zombie" ghost | name | `code/worldobject/souldestroyer.h` | "Wisp" |
| "The Beast" crate | name | `code/worldobject/crate.cpp:Crate::RunTheBeast` | "The Maw" |
| "WallBuster" | name | `code/worldobject/wallbuster.h` | "Breach Charge" |
| "Multiwinia" (team/crate strings, `multiwinia_exclamation`) | brand/text | `code/worldobject/crate.cpp` | product name TBD |
| UI strings: `crate_message_run_antnest`, `crate_message_auto_eggs`, `crate_message_run_eggs`, `crate_message_run_meteor`, `crate_message_auto_infection`, `crate_message_auto_plague`, `crate_message_spaceship`, `crate_message_thebeast`, `crate_message_run_darkforest`, `dialog_wallbreach`, `spawnname_*`, `cratename_*` | text | `data/language/*` via `LANGUAGEPHRASE` | rewrite all strings |
| Shapes: `anthill.shp`, `armyant.shp`, `armyant2.shp`, `armyant3.shp`, `centipede.shp`, `centipedehead.shp`, `spider.shp`, `spider_leg_upper.shp`, `spider_leg_lower.shp`, `sporegenerator.shp`, `souldestroyerhead.shp`, `souldestroyertail.shp`, `triffidhead.shp`, `triffidegg.shp`, `meteor.shp`, `spaceship.shp`, `tripod.shp`, `tripod_leg_part.shp`, `lander.shp`, `wallbuster.shp`, `researchitem.shp` | art | `data/shapes/` (loaded by name in each ctor/`Begin`) | new models |
| Textures/sprites: `sprites/egg.bmp`, `sprites/viriifull.bmp`, `sprites/ghost.bmp`, `textures/lightning.bmp`, `textures/glow.bmp`, `textures/starburst.bmp`, `textures/cloudyglow.bmp`, `textures/laser.bmp`, icons `icons/icon_antnest.bmp`, `icon_egg.bmp`, `icon_triffid.bmp`, `icon_meteorshower.bmp`, `icon_infection.bmp`, `icon_plague.bmp`, `icon_darkforest.bmp` | art | per-class Render; `Crate::Run*` markers | new art |
| Audio samples: virusattack, viruspain, eggsplat, sporegengroan, sporegenlosehealt, sporegeneratordie, spideregglay, tripodcreate, tprdattac, trpodlosehealth, ttripoddie, spidercreate, spiderpounce, spiderdeath, spiderfeet, antatack, antcreat, antlosehealth, antdeath, souldestroycreate, souldestpain, souldestdeath, soulattack, soulenemysighted, triffidegglaunc, triffideggbounce, triffeggbursts, meteorhitclose1, meteorhitfar1, rocketthrust2loop, rumble2_loop, theramins, ufo_engine1, ufo_engine2, tractorbeam2, 1anthilldamage, triffidburn, samcreate, emaildamage, spamattack, spamboom, researchcreate, cratereward_meteor4, newsamplegroup3/9/11/13/25/26/27/28 | audio | `data/sounds.txt` | new sound set |

<!-- from P06_buildings_core -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Darwinian(s)" | name | throughout (`Entity::TypeDarwinian`, comments, UI) | "Sprite" / "worker unit" |
| "Multiwinia", "Darwinia" | brand | `Multiwinia::*`, `App::GameModeMultiwinia` | "<Game>" / "legacy campaign" |
| "Futurewinian" team | name | `GunTurret::RecalculateOwnership`, `Location::GetFuturewinianTeamId` | "Vanguard faction" |
| "The Beast" (Portal crate), Tentacle | name | `Crate::RunTheBeast`, `portal.cpp` | "Rift Horror" |
| "Spawn Mania" | name | `Crate::RunSpawnMania` | "Spawn Frenzy" |
| Shaman | name | `Portal::SpecialSummon` | "Summoner" |
| "Gq/s" output unit | text | `Generator::GetObjectiveCounter` | "units/s" |
| UI/language keys `buildingname_*`, `buildingname_gunturret/rocketturret/grenadeturret/flameturret/subversionturret`, `objective_redpopulation`, `objective_output`, `objective_throughput`, `crate_message_run_gunturret`, `crate_message_thebeast`; on-turret text "FF!" | text | `GetTurretTypeName`, `GetTypeNameTranslated`, crate.cpp | own string table |
| Shapes: `spawnpointmultiwinia.shp`, `spawnpoint.shp`, `spawnlink.shp`, `masterspawnpoint.shp`, `battlecannonbase.shp`, `battlecannonturret.shp`, `battlecannonbarrel.shp`, `cannon_bullets.shp`, `cannon_flame.shp`, `cannon_rocket.shp`, `radardish.shp`, `controltower.shp`, `controltowerdish.shp`, `generator.shp`, `pylon.shp`, `solarpanel.shp`, `powerstation.shp`, `constructionyard.shp`, `constructionyardrung.shp`, `mineprimitive1.shp`, `displayscreen.shp`, `armour.shp`, `incubator.shp`, `spiritreceiver.shp`, `spiritreceiverhead.shp`, `receiverlink.shp`, `spiritprocessor.shp`, `controlpad.shp`, `throwable.shp` | art | ctors in this partition | new models |
| Textures: `textures/starburst.bmp`, `laser.bmp`, `cloudyglow.bmp`, `laserfence.bmp`, `radarsignal.bmp`, `glow.bmp`, `triangleOutline.bmp`, `beam.bmp`, `icons/thickarrow.bmp`, `icons/icon_gunturret.bmp` | art | render code | new textures |
| Sound event bindings (names listed in Cross-partition notes; actual samples via `data/sounds.txt`) | audio | `ListSoundEvents` | new audio |
| Level file names `mp_*` and their layouts | content | `data/levels` | new maps |

<!-- from P07_buildings_misc -->
## IP register
| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Darwinian" (unit name used in tree/fence/randomiser logic) | name | `code/worldobject/darwinian.h` | "Sprite" / "Unit" |
| "Multiwinia" | brand | build/product | (project codename) |
| "Futurewinian" team | name | `Location::GetFuturewinianTeamId` (used by `Tree::Advance`) | "Neutral-AI team" |
| "Dark Forest", "Magical Forest" crate outcomes | name | `code/worldobject/crate.cpp`, `taskmanager.cpp` | "Blight Grove", "Spirit Grove" |
| "Randomiser" outcome / `icons/icon_randomizer.bmp` | name/art | `crate.cpp:Crate::RunRandomiser` | "Allegiance Scrambler" / new icon |
| "Soul Destroyer" (Zombie effect class home) | name | `code/worldobject/souldestroyer.h` | "Wraith effect" |
| "Spam", "GodDish", "Sepulveda", "Pattern Buffer", "Blueprint Store/Console/Relay" | name | blocked Darwinia objects | do not reuse |
| Darwinia campaign map names `biosphere`, `containment`, `escort`, `garden`, `generator`, `launchpad`, `mine`, `pattern_buffer`, `receiver`, `sandbox`, `temple`, `yard` | name | `data/levels/map_*`, `mission_*` | do not reuse |
| Map titles in docs: "The Last Stand", "Stairway to Heaven", "The Holy Tree" | name | `docs/multiwinia-changes.txt`, `docs/multiwiniareleasenotes-beta.txt` | new map names |
| Shapes: `laserfence.shp`, `wall.shp`, `clonelab.shp`, `fenceswitch.shp`, `beachheadobstruction.shp`, `temple2.shp`, `temple3.shp`, `temple4.shp`, `rockhead.shp`, `statue.shp`, `spaceship.shp`, `carryspider.shp`, `incubator.shp`, legacy (`blueprint*.shp`, `bridgeend/bridgetower.shp`, `cave.shp`, `chesspiece.shp`, `factory.shp`, `feedingtube.shp`, `goddish.shp`, `library.shp`, `mine.shp`, `minecart.shp`, `minepolygon1.shp`, `mineprimitive1.shp`, `refinery.shp`, `researchitem.shp`, `tracklink.shp`, `wheel.shp`, `upgrade_port.shp`, `primaryupgradeport.shp`) | art | `data/shapes/` | new meshes |
| Textures: `textures/tree.bmp`, `starburst.bmp`, `laserfence.bmp`, `laserfence2.bmp`, `laser.bmp`, `cloudyglow.bmp`, `glow.bmp`, `interface_grey.bmp`, `radarsignal.bmp`, `sprites/darwinian.bmp` | art | as cited | new textures |
| Sound event names: `CreateNormal`, `CreateEvil`, `CreateMagic`, `Burn`/"Tree Burn", `Spark`, `Electrocute`, `ConnectToGod`, `DisconnectFromGod`, `CogTurn`, `AquireResearch`, `TriggerSurge`, `Enable`, `Operate`, `BeginRotation`, `EndRotation`, `ConnectionEstablished`, `ConnectionLost` | audio binding | `ListSoundEvents` implementations | new event ids |
| UI strings: `dialog_wallbreach`, `objective_totalinfection`, `objective_currentcount`, `objective_refined`, editor labels `editor_*` | text | `wallbuster.cpp`, `blueprintstore.cpp`, `safearea.cpp`, `mine.cpp`, `tree_window.cpp` | new string ids |
| Christmas mod tree styling | feature/art | `Location::ChristmasModEnabled` in `Tree::RenderAlphas` | optional seasonal theme |

<!-- from P08_crates_ai -->
## IP register
| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Multiwinians", "Darwinians" (crate reward and messages) | name/text | `data/language/english.txt` `cratename_darwinians`, `crate_message_darwinians`, `crate_tooltip_*`; `Crate::RunDarwinians` "Evil Darwinians!" | "Reinforcements", "Hostile units!" |
| "Evilinians" (monster team), "Futurewinians" (SpaceShip team) | name | `multiwinia_team_virus`, `multiwinia_team_future`, `crate_message_spaceship` | "Corrupted", "Visitors" |
| "Rocketwinians" | name | `cratename_rocketwinians`, `crate_message_rocketwinians` | "Rocket Troops" |
| "Mega Rage" / "Bitzkreig" | name | `cratename_bitzkreig`, `multiwinia_blitzkrieg_timer` | "Frenzy" |
| "Crate Mania", "Spawn Mania", "Hot Feet", "Personal Shield", "Subversion", "Infection", "Magical Forest", "Dark Forest", "Ants Nest", "Plague", "Randomiser", "Napalm Strike", "Slow Down", "Speed Up", "The Beast", "Space Ship" | names | `cratename_*` | generic descriptive names (e.g. "Supply Storm", "Rapid Spawn", "Haste", "Shield", "Convert", "Contagion", "Grove", "Blight Grove", "Hive", "Sickness", "Scrambler", "Firebomb Run", "Time Dilation", "Time Rush", "Summoned Horror", "Abductor") |
| "Triffids" | name (third-party literary IP, John Wyndham) | `cratename_triffids`, `Triffid` class | "Spitter Plants" |
| "I love the smell of Napalm in the morning..." | text (third-party film quote) | `crate_message_run_napalm` | "Firebombs inbound!" |
| "Sloooow Dooown...", "Speed up!!", "The Beast Cometh!", "A familiar presence returns", "All Multiwinians just go nuts with their lasers!" and all `crate_tooltip_*` / `crate_message_*` / `crate_drop` / `crate_enemycapture` / `dialog_powerup_limit` / `multiwinia_spawnmania_timer` / `multiwinia_param_basiccrates*` strings | text | `data/language/*.txt` | rewrite all UI text |
| "(CPU)", "(LEFT)" team prefixes; difficulty labels | text | `teamtype_2`, `teamtype_left`, `dialog_easy_difficulty`, `multiwinia_menu_normal`, `multiwinia_menu_hard` | own strings |
| Crate model `researchitem.shp`; AI target model `aitarget.shp` | art | `Crate::Crate`, `AITarget::AITarget` | new meshes |
| `textures/bubble.bmp`, `textures/starburst.bmp`, `textures/cloudyglow.bmp`, `icons/target.bmp` | art | `Crate::Render*` | new VFX textures |
| `icons/icon_crate.bmp`, `icon_statue.bmp`, `icon_airstrike`, `icon_nuke`, `icon_subversion`, `icon_personalshield`, `icon_hotfeet`, `icon_gunturret`, `icon_infection`, `icon_magicalforest`, `icon_darkforest`, `icon_antnest`, `icon_plague`, `icon_egg`, `icon_meteorshower`, `icon_darwinian`, `icon_randomizer`, `icon_rage`, `icon_triffid` (+ other `icon_<task>` icons), `icons/marker_shadow.bmp`, `icons/compass.bmp`, `icons/compass_shadow.bmp`, `icons/compass_empty.bmp`, `icons/marker_colour.bmp` | art | `Crate::Run*`, `MarkerSystem::Render*`, `data/icons/` | new icon set |
| Crate sound events (`Falling`, `Land`, `BeginCapture`, `Capturing_*pc`, `Captured`, `LostCapture`, `Expired`) and power-up sounds named by crate key | audio (assets bound via `sounds.txt`) | `Crate::ListSoundEvents`, `Crate::TriggerSoundEvent` | new audio; event names may be kept as neutral identifiers |
| Level ids/names `mp_assault_2P_1` etc. (as used in InvalidCrates table) | name | `data/levels/` | new map names |
| Developer names in code comments ("Chris", "Gary") | name | crate.cpp, location.cpp, aiobjective.h, restrictionzone.cpp | do not carry over |

<!-- from P09_control_taskmanager -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Task Manager", "program", "RUN PROGRAM", "SYNTAX ERROR", "PROGRAM TERMINATED", "ACQUIRED", "UPGRADED", "OBJECTIVES COMPLETED" | text / fiction | `data/language/english.txt` `taskmanager_msg0..5`; `taskmanager_taskmanager` | "Power slots"; "Ready", "Not available", "Cancelled", "Unlocked", "Improved", "Objectives complete" |
| "*P TERMINATED", "Powerup Limit Reached!", "Retribution : Two powerups max" | text | `dialog_terminated`, `dialog_powerup_limit`, `multiwinia_retribution_maxtasks` | "<power> cancelled", "All power slots full", "Comeback powers: slots full" |
| "crate opened", "reinforcements", "retribution" | text | `multiwinia_cratepowerupfound`, `multiwinia_reinforcementsgiven`, `multiwinia_retributiongiven` | "supply drop", "reinforcements", "comeback bonus" |
| Power-up names ("Napalm Strike", "Hot Feet", "Shields"/"PersonalShield", "Subversion", "Rage", "Magical Forest", "Dark Forest", "Ant Nest", "Plague", "Eggs", "Meteor Shower", "Fire Extinguisher", "Space Ship", "Gun Turret", "Rocket/Flame Turret", "Infection", "Shaman") and all `crate_tooltip_*` texts (e.g. "High-powered cannon operated by Multiwinians") | names / text | `researchname_*`, `cratename_*`, `crate_tooltip_*` | functional names: "Air strike", "Incendiary strike", "Speed boost", "Shield", "Convert", "Frenzy", "Grove", "Blight grove", "Swarm nest", … |
| "Darwinian(s)", "Multiwinian(s)", "Officer", "Engineer", "Squad", "Armour", "Harvester" as unit names | names | `GlobalResearch::GetTypeName`, language file | "Worker", "Leader", "Builder", "Strike team", "Transport", "Collector" |
| Gesture symbol designs (triangle, arch, arrow, star, zig-zag, V, shield) as authored with the icons | art / design | `icons/gesture_*.bmp`, `data/gestures.txt`, `data/gesture_demos/*` | new symbol set + newly recorded training data |
| All TM / cursor bitmaps: `icons/icon_*.bmp`, `icons/gesture_*.bmp`, `icons/gestureguide.bmp`, `icons/gesture_alpha.bmp`, `icons/mouse_*.bmp`, `icons/compass*.bmp`, `icons/marker_*.bmp`, `icons/arrows_*.bmp`, `icons/button_*.bmp`, `icons/selectionarrow.bmp`, `icons/icon_crate.bmp`, `icons/icon_delete.bmp`, `icons/tick.bmp`, `textures/interface_*.bmp`, `textures/starburst.bmp`, `textures/particle.bmp` | art | `data/icons`, `data/textures` | new UI icon set |
| Ghost models `battlecannonturret.shp`, `anthill.shp` | art | `GameCursor::RenderSelectedTaskIcon` | new placement-preview meshes |
| Sound event names `GestureBegin/End/Success/Fail`, `SelectTask`, `DeleteTask`, `MouseOverIcon`, `UnitHighlight`, `Show`, `Hide`, `Slide`, `MouseError`, `SelectUnit`, `ToggleUnitState` and their audio | audio | sound system calls in scope files | new UI sound set |
| Prologue objectives text `prologue_objective_1..5`, "launchpad" location | text / level name | `TaskManagerInterfaceIcons::InitProgressBarList` | n/a (legacy) |
| Dr Sepulveda lines (`task_manager_full`, `task_manager_targetfirst`, `research_notyetavailable`, …) | character / text | `#ifdef USE_SEPULVEDA_HELP_TUTORIAL` blocks | n/a (dead) |

<!-- from P10_input_camera -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Darwinian" in control names `DarwinianPromote`, `IssueDarwinianOrders` | name | `code/lib/input/control_types.inc`; `data/input/input_preferences*.txt` | `PromoteUnit`, `IssueCrowdOrders` |
| "Sepulveda" in `SepulvedaHelp`; Dr Sepulveda help system | name/character | `control_types.inc` | `AdvisorHelp` |
| "Multiwinia" / "Darwinia" in preference names (`MultiwiniaControlMethod`), file names (`input_preferences_darwinia.txt`), string ids `multiwinia_control_*`, `darwinia_tutorial_*` | brand | `code/lib/input/file_paths.cpp`; `code/control_help.cpp`; `code/tutorial_demo2.cpp` | `ControlScheme`, `bindings_classic.txt`, `ui_control_*` |
| "Eclipse" (Introversion UI framework) in `EclipseLMouseDown`, `EclipseLMousePressed`, `EclipseRMousePressed` | name | `control_types.inc` | `UiPointerDown`, `UiPointerHeld`, `UiAltPointerDown` |
| Internal codenames `Pecan`, `Hummingbird` | name | `code/lib/input/inputdriver_values.inc` | `AlwaysTrue`, `FeatureFlagX` |
| Xbox / XInput / "360" naming (`XBoxMenuCycleUp/Down`, "360 Control Help", gamepad button art) and SpaceNav | third-party brand | `control_types.inc`; `data/language/english.txt`; `data/input/input_preferences.txt` | `PadMenuCycleUp/Down`; generic pad glyphs |
| Button/trigger icon art `icons/button_a.bmp`, `button_b.bmp`, `button_x.bmp`, `button_y.bmp`, `button_abxy_shadow.bmp`, `button_dpad.bmp`, `button_dpad_lr.bmp`, `button_control_shadow.bmp`, `button_lb.bmp`, `button_lb_shadow.bmp`, `button_rb.bmp`, `button_rb_shadow.bmp`, `button_la.bmp`, `button_ra.bmp`, `button_lt.bmp`, `button_rt.bmp`, `button_trigger_shadow.bmp`, `button_lt_ra.bmp`, `button_rt_ra.bmp`, `mouse_lmb.bmp`, `banner_goto.bmp`, `banner_follow.bmp`, `banner_none.bmp` | art | `code/control_help.cpp`; `data/input/input_preferences.txt` | new glyph set (e.g. `ui/glyph_pad_south.png`) |
| Camera-help images `mwhelp/control_camera.bmp`, `mwhelp/control_camera_french.bmp` | art | `code/multiwiniahelp.cpp` | `help/camera_controls_<layout>.png` |
| UI strings `gc_*` (Select, Destroy, Create, Close, Deselect, Place, Promote, Move, Look, Fire, Grenade, Rocket, Airstrike, Change Weapon, Change Orders, Up, Down, Set Goto, Set Turret, Prev, Next, Zoom, Zoom In, Zoom Out, Cancel), `control_help_*`, `control_event_*`, `control_<key>` | text | `data/language/english.txt` (and other languages) | write new strings |
| Camera mount naming convention `player0..3`, `start`, `CamPosBefore` and Darwinia mount names (`intro*`, `outro*`, `attract*`, `tutorial*`, …) | level data | `data/levels/*.txt`; `code/level_file.h` | `spawn_view_<n>`, `default_view`, `previous_view` |
| Weapon/unit names in hints (Squad, Engineer, Armour, Officer, grenade, rocket, airstrike) | names (generic but Introversion phrasing) | `code/control_help.cpp` | reuse own unit names |

<!-- from P11_terrain_world -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Multiwinia" in file-format keywords (`MultiwiniaOptions_StartDefinition`), class names | brand | `code/level_file.cpp:ParseMultiwiniaOptions` | `MatchOptions` block |
| "Darwinian" (research type 0, level/instant-unit type names) | name | `code/global_world.cpp:GlobalResearch::GetTypeName` | "Unit" |
| "Futurewinian" (`futurewinianteam` option) | name | `code/level_file.h` LevelOption 12 | "alien faction" |
| Dr Sepulveda face images and cheat (`ShowDrSFace`) | character / art | `data/sepulveda/sepulveda1..5.bmp`, `code/clouds.cpp:RenderFace` | drop |
| Map file names and IDs (`mp_koth_2P_1` …, `MAPID_*`, `level_enum.txt`) | name | `data/levels/*`, `code/mapcrc.h` | `map_<mode>_<n>` (X4 catalogue) |
| Darwinia location names: sandbox, garden, containment, generator, mine, yard, escort, pattern_buffer, biosphere, receiver, temple, launchpad | name | `data/game*.txt`, `data/locations.txt`, `map_*`/`mission_*` | `region_01` … `region_12` |
| Terrain colour ramps (23): landscape_beach, containment, dark, default, desert, earth, field, firetop, icecaps, icecaps_peaked, jailbreak, launchpad, marble, marble2, mine, mine2, ridges, shrine, silver, stronghold, trippy, volcanic, volcanicplain | art | `data/terrain/landscape_*.bmp` | new 64×64 ramps `ramp_<theme>` |
| Water textures (4): water_default, icecaps, launchpad, yellow | art | `data/terrain/water_*.bmp` | new tileable textures |
| Wave colour tables (9): waves_containmentfield, default, desert, dull, earth, generator, icecaps, launchpad, yellow | art | `data/terrain/waves_*.bmp` | new 1-D gradients |
| Sky/cloud/grid textures: clouds.bmp, laser.bmp, triangleoutline.bmp, glow.bmp, starburst.bmp, laserfence2.bmp | art | `data/textures/` | new textures |
| Global-world shapes: globalworld_outer/middle/inner.shp, trunkport.shp | art | `data/shapes/` | drop (legacy) |
| "redshirt2" text-file cipher marker | text | `contrib/systemiv/lib/filesys/text_stream_readers.cpp:CleanLine` | new or none |
| Research/crate type names that are distinctive: "TaskManager", "PersonalShield", "HotFeet", "MagicalForest", "DarkForest", "AntNest", "Rage", "Shaman", "Subversion" | name | `GlobalResearch::GetTypeName`, `Crate::GetName` | generic ability names |
| Game type names "KingOfTheHill", "CaptureTheStatue", "RocketRiot", "Blitzkrieg", "Domination", "Assault", "TankBattle" | name (low risk, partly generic) | `data/multiwinia.txt` | `mode_hill`, `mode_relic`, `mode_race`, … |
| Darwinia lore concepts used by global world: spirits, "heaven", digital islands, trunk ports | concept / text | `code/global_world.cpp`, `code/global_internet.cpp` | neutral meta-map or drop |
| Language keys `location_%s`, `researchname_%s`, `research_%s_v%d`, `objective_research`, `objective_capture_trunk` | text ids | `code/global_world.cpp`, `code/level_file.cpp` | new string table |

<!-- from P12_render_core -->
## IP register
| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Multiwinia" window title | brand/name | `code/lib/universal_include.h` `APP_NAME_W`, `Renderer::Initialise` | `<GAME_TITLE>` |
| "Darwinian" (sprite `sprites/darwinian.bmp`, shape `data/shapes/darwinian.shp`) | name/art | `Team::RenderDarwinians`, `Entity::ConvertShapeColoursToTeam` | "Unit sprite" / `unit_basic` |
| Virii, Spirit, Soul Destroyer, Centipede, Spider, Squadie/Insertion Squad, Shaman, Officer, Armour, Triffid, Harvester (names in pixel-effect/deform/colour code) | names | 24.3, Deform, 24.5 | generic role names (swarm, soul orb, devourer, segmented crawler, leaper, squad, summoner, commander, transport, spore pod, collector) |
| `textures/particle.bmp`, `shapewireframe.bmp`, `triangleoutline.bmp`, `laser.bmp`, `clouds.bmp`, `starburst.bmp`, `glow.bmp`, `cloudyglow.bmp`, `deform1c.bmp`, `deform3.bmp`, `deform36c.bmp`, `sprites/virii.bmp` | art | tables in 25.3 | regenerate procedurally / new art with same roles |
| 140 `.shp` meshes (`data/shapes/*.shp`) | art | `OUT/_work/drafts/P12_shapes.csv` | new meshes |
| Font atlases `speccy`, `editor`, `square` (`data/textures/unicode/*.bmp`, incl. CJK/Hangul glyph art) and legacy `editor_font*.bmp`, `speccy_font*.bmp` | art (font) | 24.7 | licensed open fonts (e.g. an OFL pixel font + Noto CJK) rendered to a glyph cache |
| "Darwin Research Associates" logo (`icons/darwin_research_associates.bmp`) and caption `launchpad_logo_1` | brand/art/text | `RenderFrame` logo block | omit / studio placeholder |
| `privatedemo1/2` strings and `textures/privatedemo.bmp` | text/art | `Renderer::RenderLogo` [B:DEAD] | omit |
| `multiwinia_error_syncerror`, `multiwinia_error_lostconnection`, `multiwinia_presstotalk` | UI text keys | `RenderFrame`, `Location::RenderChatMessages` | `err_desync`, `err_connection_lost`, `hint_chat` |
| Team colour names green/red/yellow/blue/orange/purple/cyan/pink (`multiwinia_team_*`) | text | `Team::GetTeamName` | keep as generic colour names, new keys |
| Dr Sepulveda face in clouds (`Clouds::RenderFace`, `ShowDrSFace`) | character | P11 (noted) | none |
| Shader source files `data/shaders/*` | code (licence) | 24 Shaders | re-author in HLSL 6 |
| "Pokey" sound chip emulation hooks | third-party trademark (Atari POKEY) | `code/effect_processor.cpp` [B:DEBUG] | drop |
| "PAUSED" | UI text (generic, but hard-coded English) | `Renderer::RenderPaused` | localised `ui_paused` |

<!-- from P13_gfx_backend_math -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Multiwinia" (`APP_NAME_W`, used as window title **and** window-class name) | brand/name | `code/lib/universal_include.h`, `code/renderer.cpp:Renderer::Initialise` → `CreateWin` | `"<GameTitle>"` |
| `http://www.multiwinia.co.uk/` (opened via `OpenWebsite`) | brand/URL | `code/main.cpp` | studio/product URL from config |
| "Please restart Darwinia for the screen settings to take affect" (Linux-only dialog) | UI text / brand | `code/interface/prefs_screen_window.cpp:RestartWindowManagerAndRenderer` | "Please restart the game to apply display settings" |
| Application icon resource 101 (D3D window class) | art | `code/lib/window_manager_directx.cpp:CreateWin` | new icon |
| `iv_` prefix ("Introversion") on numeric wrappers | naming | `universal_include.h`, `math_stubs.cpp` | `det_` / `dmath::` |
| Shader file names `FixedFunc0/1/2(.vs, Maps.ps)` and `globals.h` content | asset (code-like) | `data/shaders/` | rewrite; names are generic |
| Third-party (not Introversion, but licence-relevant): MT19937 (Matsumoto/Nishimura BSD notice) in `math_utils.cpp`; "AVIGenerator" by Jonathan de Halleux (CodeProject) in `avi_generator.*` incl. its error strings ("…Come on, join the 21th century!"); `math_stubs.cpp` "Copyright 2007 Ambrosia Software"; comp.graphics FAQ 1.03 segment/ray algorithm; Möller–Trumbore ray/triangle | third-party code/text | as listed | use standard libraries / own implementations |

<!-- from P14_platform_lib -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Multiwinia", "Darwinia" product names, `APP_NAME`, `DARWINIA_*` macro names, `darwinia*.exe` | brand | `code/lib/universal_include.h`, `targets/vs2005/*`, `tools/*.bat` | `GAME_NAME`, `game.exe` |
| "Introversion Software", web/support URLs (`introversion.co.uk`, `multiwinia.co.uk`, `darwinia.co.uk`, `crashreports.introversion.co.uk`, metaserver host) | brand | `targets/vs2005/version.rc`, installers, `MultiwiniaSupport.exe`, `code/app.cpp:InitMetaServer` | "Studio", `example.com` |
| Ambrosia Software, Buka, eGames/Cinemaware Marquee (distributors), Logitech G15, amBX | brand (third-party) | targets/macosx, installers, exclusion lists | omit |
| Dr Sepulveda, Protologic 68000, "Future War"/"Futurewar Shape Format", story documents | name/text | `docs/sepulveda_interview.txt`, `docs/story material.txt`, `tools/ShapeExporter/ShapeExporter.cpp:LongDesc` | omit / "Shape format v1" |
| `"redshirt2"` encryption marker | text | `text_file_writer.cpp` | own marker (or drop obfuscation) |
| UI/language strings (`data/language/*`, `LANGUAGEPHRASE` keys such as `multiwinia_enterauthkey`, `error_parental`) | text | `data/language/*.txt` | new string table |
| Fonts `speccy`, `square`, `editor` font pages; `verdana-bold.bmp` in animator.dat | art | `data/textures/unicode/*`, `animator.dat` | licensed/open fonts |
| Icons/box art: `darwinia.ico`, `DarwiniaVista.ico`, `GDF/*/boxart_*.png`, `gameicon_*.ico`, `Multiwinia.icns`, `darwinia.png`, NSIS `darwinia.bmp`/`header.bmp`, Loki `splash.xpm` | art | targets/, tools/, root | new art |
| Sound lists naming assets (`mwsounds/*`, `sounds/*`, menu ambience) | audio | `tools/sounds.txt`, `tools/menu_sounds.txt` | new audio set |
| Licensed retro music (Trash80, Tresk, DMA SC, Two Ataris…) | audio (third-party) | `docs/retro music.txt`, exclusion lists | do not reuse |
| EULAs (`EULA.txt`, `docs/IntroversionEULA.txt`, Buka EULA, NSIS `licence.txt`) | text | root/docs/tools | own licence |
| Beta tester handles/e-mails | personal data | `docs/betatesters.txt` | never reproduce |
| Developer names in credits/comments (Mac port credits, "Hack by Chris", Bruce Dawson/Hans Dietrich headers) | name | targets/macosx rtfs, code comments | omit |
| Third-party code: MT19937 (BSD-style notice), XCrashReport/ExceptionHandler (public domain/Dawson), GetWinVer/MiniVersion (Dietrich, public domain), auto_vector (Reliable Software), unrar (RARLAB licence), NSIS samples, Loki Setup | third-party | files listed | use standard libraries (`std::mt19937`, `minidumpwriter`, `std::vector<std::unique_ptr>`) |
| Map ID names (all 87 `MAPID_*` in `code/mapcrc.h`, e.g. `MAPID_MP_KOTH_2P_1`) | name (level names) | `code/mapcrc.h`, inclusion lists | new map names |

<!-- from P15_network -->
## IP register
| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| `metaserver-mwdev.introversion.co.uk` | brand / service hostname | `code/app.cpp:App::InitMetaServer` | `directory.example.net` |
| `80.175.29.66` (route-probe address) | Introversion-owned IP [I] | `contrib/systemiv/lib/netlib/net_lib.cpp:GetLocalHostIP` | use OS interface enumeration |
| `ftp.introversion.co.uk`, user/password `multiwinia_stats` | brand / credentials | `code/benchmark.cpp` (P20) | none — do not reuse |
| `http://store.introversion.co.uk` | brand URL | `code/app.cpp:App::GetBuyNowURL` | store URL of new title |
| `"Multiwinia"` (`APP_NAME`, GAMENAME on the wire) | name | `code/lib/universal_include.h` | `<NewTitle>` |
| `METASERVER_GAMETYPE_MULTIWINIA` = 4, `_DEFCON` = 2, "defcon-steam3" | product ids / names | `metaserver_defines.h`, `steamauth_server.cpp` | new product id |
| Fixed demo key `DEMOTP-AXYVLW-SGIZLU-DNLXPK-OJS`; blacklisted keys `ATHETA-DKFOPQ-OGPUSS-CCKOPL-TNF`, `OWRGIO-CLMQEE-URWUDL-SUVUCC-WYQ` | key strings | `authentication.cpp` | none |
| Key prefixes `"DEMO"`, `"AMBR"` (Ambrosia distributor), hash prefix `"hsh"` | text / partner brand | `authentication.cpp` | platform entitlements |
| `authkey.dev` / `authkey` file names | text | `code/app.cpp`, `steamauth_client.cpp` | — |
| Steam app id 1520 | partner id | `steamauth_client.cpp` (dead) | — |
| UI phrases `multiwinia_error_serverfull`, `_kickedbyserver`, `_badpass`, `_connection`, `_syncerror`, `_lostconnection`, `_incompatible`, `_missingmap`, `_unknownmap`, `multiwinia_player_left`, `multiwinia_menu_connected`, `multiwinia_menu_attempts`, `multiwinia_update_available`, `dialog_duplicate_key`, `dialog_auth_*`, `dialog_steam_progress_*`, `authentication_demorestricted`, chat tokens `PLAYERJOIN`/`PLAYERLEAVE` | UI string ids (text in data/language) | `clienttoserver.cpp`, `renderer.cpp`, `GameServerButton.h`, `multiwinia.cpp`, `authentication.cpp` | neutral ids, new text |
| Default server name `"NewServer"`, fallback `"Multiwinia"` | text | `Server::Advertise` | `"New Server"` |
| Map ids `MAPID_MP_KOTH_2P_1`, `_KOTH_3P_1`, `_KOTH_4P_1`, `_CTS_2P_1`, `_ROCKETRIOT_3P_1` (demo map list) | level names | `demo_limitations.cpp`, `code/mapcrc.h` | new map ids |
| Game-mode names (Domination/Skirmish, King of the Hill, Capture the Statue, Assault, Rocket Riot, Blitzkrieg, Tank Battle) in `GAMEMODE` | names | `code/multiwinia.h` | generic mode names |
| Identifier prefix `NET_DARWINIA_*` and the readable codes "DARWINIA"/"CMD" in `network_defines_debug.h` | brand in identifiers | `network_defines*.h` | `NET_GAME_*` |

<!-- from P16_sound -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| All 1332 audio samples (954 in `data/sounds/`, 378 in `data/mwsounds/`, e.g. `darwiniandie`, `CrateFall1`, `MultiLobbyAmb1..10`, `countdownbeep3`, `koh_zone_captured`) | audio | `data/sounds/`, `data/mwsounds/`, `sounds.dat`, `sounds2.dat`, `menusounds.dat` | Commission/record new assets; name by function (`unit_die_01`, `crate_fall_01`, `lobby_pad_01`) |
| Licensed music: `tresk_-_excuses`, `tresk_-_faces-of-a-fashion`, `tresk_-_impact-of-silence`, `tresk_-weeklies-_schroeders-failure`, `trash80_-_pain-fade-down`, `dma_sc-visitors_from_dreams` | audio (third-party artists) | `data/sounds/*.ogg`; `OTHER Music` blocks | New licensed/commissioned tracks `music_cutscene_01..` |
| Retro-homage loader tunes `fodder`, `speccyloader`, `pang`, `matrix`, `twoataris` | audio (possibly third-party game music) | `data/sounds/*.ogg` | Drop or replace with original chiptunes |
| Artist/track list in `docs/retro music.txt` (trash80 catalogue: "impact of silence", "schroeders failure", …) | text (third-party) | `docs/retro music.txt` | Do not reuse |
| Sound-source/type names "Darwinian", "Officer", "Squadie", "Sepulveda", "MultiwiniaInterface", "MultiwiniaZone", "SoulDestroyer", "Virii", reward names "Rocketwinians", "Futurewinians" (sample `CrateReward_Futurewinians`), "Bitzkreig" | name | `data/sounds.txt`, `soundsystem.cpp:GetSoundSourceName` | Generic: `Unit`, `Commander`, `Trooper`, `Narrator`, `MatchUI`, `CaptureZone`, `reward_rocket_units` |
| Sample-group names embedding IP names (`darwiniandie`, `darwiniancreate`, `officercreate`, `darwinnianonfire`, …) | name | `data/sounds.txt` SAMPLEGROUPs | Functional names |
| UI strings: "Master Volume" (hard-coded English), "Failed to set requested sound settings", language keys `dialog_soundoptions`, `dialog_soundlibrary`, `dialog_directsound`, `dialog_softwaresound`, `dialog_mixfrequency`, `dialog_11khz`, `dialog_22khz`, `dialog_44khz`, `dialog_numchannels`, `dialog_8channels`, `dialog_16channels`, `dialog_32channels`, `dialog_64channels`, `dialog_memoryusage`, `dialog_high`, `dialog_medium`, `dialog_low`, `dialog_swapstereo`, `dialog_hw3dsound`, `dialog_unavailable`, `dialog_realtimeeffects`, `dialog_apply`, `dialog_close`, `multiwinia_menu_back`, `multiwinia_mainmenu_title`, `dialog_cpuusage`, `dialog_cpuusageunknown` | text | `code/interface/prefs_sound_window.cpp`, `code/UI/soundsliderbutton.h`, `soundsystem.cpp` | New string table: "Audio", "Output device", "Mix rate", "Voices", "Swap L/R", "Master volume", "Apply", "Back" |
| Editor debug labels "Pokey Playground", window titles "Sound Editor", "Sound Stats", "Sound Profile", "Purge Sounds", "Effect Editor" | text | `code/interface/debugmenu.cpp`, `soundeditor_window.*` | Generic tool names |
| Editor billboard `sprites/sound.bmp` | art | `soundeditor_window.cpp:SoundEditorWindow::Render` | Any speaker icon |
| `docs/MinDistAndVolume.jpg` (airplane/bee/waterfall chart, apparently from DirectX documentation) | art (third-party) | `docs/` | Redraw own chart |
| Planning notes `docs/multiwinia audio.docx` (crate reward commentary, "voice of the devil" etc.) | text | `docs/` | Do not reuse |

<!-- from P17a_ui_framework_editor -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "MULTIWINIA" window title (`multiwinia_mainmenu_title`) | brand/text | `code/interface/darwinia_window.cpp:DarwiniaWindow::Render`, `multiwinia_window.cpp`, `multiwinia_filedialog.cpp` | product title token `app.title` |
| Class/brand names `DarwiniaWindow`, `DarwiniaButton`, `MultiwiniaWindow`, `MultiwiniaFileDialog`, `MultiwiniaOptionsWindow`, `MultiwiniaEditorButton`, `dialog_multiwinia` | name | `code/interface/*` | `UiWindow`, `UiButton`, `MapEditorHubPage`, `MapFilePicker`, `MapOptionsPanel` |
| "Eclipse" library name (Introversion in-house, author credit "Christopher Delay") | name | `contrib/eclipse/*` | `ui-core` |
| "Darwinian(s)", "Spawn Green/Red/Yellow/Blue/Virus/Future", "Spawn Virii", "Spawn Spirits", "Futurewinian" | name/text | `cheat_window.cpp`, `multiwiniaoptions_window.cpp` | "unit", "Spawn Team 1–4", "Spawn Hostile", "Spawn Souls", "alt-faction" |
| "Show Dr S Face" / `Clouds::ShowDrSFace` (Dr Sepulveda) | character/text | `cheat_window.cpp` | drop |
| Darwinia campaign location names "generator", "mine", "receiver", "pattern_buffer"; buttons "Enable Generator and Mine", "Enable Receiver and Buffer", "Open All Locations" | name/text | `cheat_window.cpp` | drop (no campaign) |
| Every building type name exposed by the editor building list and per-type windows (`Building::GetTypeName` for all non-blocked types, plus `buildingname_gunturret/rocketturret/grenadeturret/flameturret/subversionturret`) — enumerated in the 03 IP register | name | `code/interface/buildings_window.cpp` | generic type ids (see 03 register) |
| Entity type names in editor lists (`Entity::GetTypeName`) | name | `instant_unit_window.cpp`, `buildings_window.cpp` | generic ids (see 02 register) |
| Crate reward names (`Crate::GetName`, `cratename_*`) | name/text | `cheat_window.cpp`, `invalidcrates_window.cpp`, `multiwiniaoptions_window.cpp` | see 04 register |
| Editor UI strings (`editor_*`, `dialog_*`, `multiwinia_editor_*`, `taskmanager_mapeditor`) — full list with English text in `drafts/P17a_ui_framework_editor_ui_strings.csv` (82 phrase keys + 80 value-control keys) | text | `data/language/english.txt` + scope files | rewrite all UI copy; keep only generic verbs (Save, Cancel, OK) |
| All 59 literal (untranslated) English captions in scope — e.g. "Kill All Enemies", "Allow Arbitrary Placement", "Give all research", "Clear Resources", "Spawn Ports", "GoodCrate", "Profile (F6)", "Dbg Cam (F2)", "CameStart", "Create New Anim", "Record Demo"; complete list = rows with kind `literal` in `drafts/P17a_ui_framework_editor_ui_strings.csv` | text | `cheat_window.cpp`, `debugmenu.cpp`, `camera_anim_window.cpp`, `gesture_window.cpp`, `grabber_window.cpp`, `darwinia_window.cpp`, `input_field.cpp`, `multiwinia_window.cpp` | rewrite |
| "Pokey Playground" (sound toy name) | name | `debugmenu.cpp` | "Synth sandbox" |
| Magic camera mount name "CamPosBefore" | name | `code/level_file.h:MAGIC_MOUNT_NAME_START_POS` | `__start_pose` |
| Default names "NewLevel", "mp_blank.txt", "mp_newmap.txt", "blah<n>", "CamAnim<n>", "map_<name>.txt"/"mission_<name>.txt", "mp_*" filter | text | `globalworldeditor_window.cpp`, `multiwinia_window.cpp`, `camera_*_window.cpp` | neutral defaults |
| Textures `textures/interface_red.bmp`, `icons/selectionarrow.bmp`, `icons/tick.bmp`, `icons/button_b.bmp`, `icons/gesture_*.bmp`; shape `camera.shp` | art | `darwinia_window.cpp`, `scrollbar.cpp`, `checkbox.cpp`, `mainmenus.h`, `gesture_window.cpp`, `location_editor.cpp` | new art |
| Fonts "speccy", "editor", "square" (`data/textures/*font*`) | art | `code/app.cpp` | licensed/open fonts |
| UI palette (red glass panel + steel-blue highlight, §5) | art direction | `darwinia_window.cpp` | new palette (keep structure) |
| Sound events "MenuSelect", "MenuCancel" (`TypeMultiwiniaInterface`) | audio | `multiwinia_window.cpp`, `mainmenus.h` | `ui.select`, `ui.back` |
| URLs `http://www.multiwinia.co.uk/` (and `http://www.savedarwinia.com` under `EGAMESBUILD`) in `MainMenuWindow` (P17b) | brand | `code/interface/mainmenus.cpp` | drop |

<!-- from P17b_menus_options -->
## IP register

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "MULTIWINIA" title and "Survival of the flattest" subtitle | brand/text | `multiwinia_mainmenu_title`, `multiwinia_mainmenu_subtitle`; `GameOptionsWindow::Render` | "<GAME TITLE>", "<tagline>" |
| `icons/mwlogo.bmp` (title-bar logo, drawn twice mirrored) | art/brand | `GameOptionsWindow::Render`, `GameMenuDropDownWindow::Render` | own logo |
| `GameMenuWindow::RenderBigDarwinian` backdrop (big Darwinian sprite) | art/character | called by every `GameOptionsWindow::Render` | own mascot/backdrop |
| `textures/interface_blur.bmp`, `textures/cloudyglow.bmp`, `textures/menuhighlight.bmp`, `textures/interface_red.bmp`, `icons/tick.bmp`, `icons/button_*.bmp` (360 pad glyphs a/b/x/y, lb/rb/lt/rt, shadows), `sprites/virii.bmp` (used as arrow), `sprites/darwinian.bmp`, `icons/icon_<crate>.bmp`, `icons/icon_template.bmp`, `icons/icon_shadow.bmp` | art | §11, §5.4, §5.9 | new UI texture set; controller glyphs from platform guidelines |
| `mwhelp/general1-5.bmp`, `mwhelp/help_<t>_<p>.bmp` | art | §5.4 | new help illustrations |
| "Darwinia", "Darwinian", "Multiwinian(s)", "Futurewinian" in strings and help pages | name | `multiwinia_helppage_general*`, `about_darwinia`, `dialog_leavedarwinia`, `multiwinia_leaderboards_sp_title` "Darwinia" | "<world name>", "<unit>" |
| Darwinia location names Garden, Containment, Mine, Generator, Yard, Escort, Pattern Buffer, Biosphere, Receiver, Temple, Prologue | name | `LeaderboardWindow::SetUpBoardButton` | campaign level placeholders |
| Game-mode names Domination, King Of The Hill, Capture The Statue, Assault, Rocket Riot, Blitzkrieg, Tank Battle; short forms DOM/KOTH/CTS/ASLT/ROCKET/BLITZ | name/text | blueprints; `multiwinia_gametypeshort_*` | generic: Control Points, Hill, Relic Carry, Siege, Launch Race, Flag Assault (or new names) |
| Help texts `multiwinia_helppage_*`, `multiwinia_help_gametypetitle_*`, crate names/tooltips/hints | text | §5.4 | rewrite |
| Demo-end copy "WANT MORE?", "Buy Darwinia online and DOWNLOAD THE GAME IMMEDIATELY!", "Just a 30Mb download stands between you and the full game.", "BUY NOW", "EXIT DEMO" | text | `launchpad_buyme_*`, `launchpad_button_*` | own store copy |
| URLs `http://www.multiwinia.co.uk/`, `http://www.savedarwinia.com`, `http://www.introversion.co.uk`, `http://store.introversion.co.uk` | brand | `WebsiteButton`, `AboutDarwiniaWindow`, `App::GetBuyNowURL` | own URLs from config |
| "Introversion Software" / "Produced by" credit, `uk.co.introversion.<APP>` Mac prefs id, `%LOCALAPPDATA%\Introversion\Multiwinia\` path | brand | `AboutDarwiniaWindow`, `App::GetPreferencesPath`, `App::GetProfileDirectory` | own company / bundle id / folder |
| "Access All Areas" cheat profile name and `game_unlockall.txt` | name | `UserProfileWindow`, `App::LoadProfile` | "UnlockAll" |
| "NewPlayer", "NewServer", "NewUser", "MyNewMod" defaults | text | §4.2, §5.10, §5.11 | keep generic or rename |
| UI strings ("Help and Options", "How To Play", "Settings", "Screen/Graphics/Sound/Other Options", "Press Enter To Chat", "Your control pad has been unplugged…", "Quit This Game?", "Location Menu", "Resume Game", "Exit Game", "Show weekly stats", "TrueSkill rankings") | text | `P17b_strings.csv` (all 166 ids) | generic equivalents (short UI labels are not distinctive, but do not copy the string table verbatim) |
| "TrueSkill" | third-party brand (Microsoft) | `multiwinia_leaderboards_tslb/tsr` | "Skill rating" |
| Xbox 360 controller glyph art and Xbox terminology (`ControlXBoxMenuCycle*`, "XBLA" pages) | third-party brand | §5.9, `s_pageNames` | platform-neutral glyphs |

<!-- from P18_ui_gamemenu -->
## IP register
All English strings behind the phrase ids below are Introversion text (`data/language/*.txt`); the ids themselves may be reused as neutral keys but the texts, names and art may not.

| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "MULTIWINIA" title, `multiwinia_mainmenu_title`, `textures/mwtitle.bmp` | brand/text/art | `GameMenuWindow::Render` | "GAME TITLE" + new logotype |
| Subtitle "Survival of the flattest" (`multiwinia_mainmenu_subtitle`) | brand text | `Render` | new tagline |
| Logo `icons/mwlogo.bmp`, box art `textures/multiwinia.bmp`, price art `textures/prices.bmp` | art/brand | `Render`, `SetupBuyMePage` | new logo, store art |
| Giant Darwinian sprite `sprites/darwinian.bmp`, player icons `textures/darwinian.bmp`, gear `mwhelp/gear.bmp` | character art | `RenderBigDarwinian`, `LevelSelectButton.h` | generic unit silhouette, abstract cog |
| Darwinia / Darwinia+ / Prologue / Campaign / "launchpad" location, `game.txt`, `game_demo2.txt` | brand/level names | `PrologueButton.h`, `CampaignButton.h`, `DarwiniaPlus*Button.h` | omit |
| Game-mode names Domination, King of the Hill, Capture the Statue, Assault, Rocket Riot, Blitzkrieg, Tank Battle (+ short forms DOM/KOTH/CTS/ASLT/ROCKET/BLITZ) | names/text | `multiwinia_gametype_*`, `multiwinia_gametypeshort_*` | Mode A…G / descriptive generic names |
| Mode blurbs `multiwinia_help_<mode>`, `multiwinia_help_prologue`, `multiwinia_help_campaign` | text | `GameTypeInfoButton.h` | new copy |
| Mode pictures `mwhelp/mode_*.bmp`, help images `mwhelp/helppage_*`, complexity icons `icons/menu_basic/intermediate/advanced.bmp`, padlock `icons/locked.bmp`, kick icon `icons/mouse_disabled.bmp` (+shadow) | art | §11.2 | new icon set |
| Map names/descriptions `<mapfile>_name`, `<mapfile>_description`, thumbnails `thumbnails/*.bmp` | level names/text/art | `LevelSelectButton.h`, `MapInfoButton.h`, `MapImageButton.h` | new maps |
| Credits `multiwinia_credits_1..N`, beta-tester list `multiwinia_betatesters.txt` (real people's names) | text/personal names | `ReadCredits` | own credits |
| Store URL `http://store.introversion.co.uk`, UpdateURL, autopatch.exe | brand/URL/tool | `code/app.cpp:GetBuyNowURL`, `update_page_buttons.h` | own store/updater |
| Controller glyphs `icons/button_a.bmp`, `button_b.bmp`, `button_lb/rb(_shadow).bmp` | third-party (console) glyph art | `PreloadTextures`, `BackPageButton::Render` | platform-neutral glyphs |
| Interface sounds `LobbyAmbience`, `MenuSelect`, `MenuCancel`, `MenuRollOver`, `ChatMessage` (sample assets) | audio | §11.3 | new SFX |
| Front-end UI phrase ids — titles: `multiwinia_menu_mainmenutitle`, `multiwinia_neworjoin_title`, `multiwinia_gameselect_title`, `multiwinia_join_game_title`, `multiwinia_menu_joining`, `multiwinia_setup_quick_match`, `multiwinia_singleplayer_title`, `multiwinia_menu_advanceoptions`, `multiwinia_playeroptions`, `multiwinia_menu_filters`, `multiwinia_enterpassword`, `multiwinia_enterauthkey`, `multiwinia_update_available`, `dialog_credits`, `multiwinia_menu_tutorial_short`, `multiwinia_menu_achievements`, `launchpad_buyme_1`, `error` | UI text | setup functions | own strings |
| Buttons: `multiwinia_menu_singleplayergame`, `_multiplayergame`, `_helpandoptions`, `_buyfullgame`, `_quit`, `_hostgame`, `_joingame`, `_quickmatch`, `_quickstart`, `_back`, `_ready`, `_play`, `_aimode`, `_coop`, `_serverpassword`, `_connect`, `_paste`, `_paste_tooltip`, `_play_demo`, `_done`, `_tutorial1`, `_tutorial2`, `_versuscpu`, `_prologue`, `_campaign`, `_autopatch`, `_searching`, `_connecting`, `_connected`, `_attempts`, `_resyncing`, `_showdemogames`, `_showincompatible`, `_showfull`, `_showpasswords`; `dialog_visitwebsite`, `dialog_next`, `launchpad_button_buynow`, `taskmanager_mapeditor`, `start_testbed_server`, `start_testbed_client` | UI text | §4 | own strings |
| Lobby/browser/status: `multiwinia_join_host`, `_join_type`, `_join_map`, `_join_players`, `_join_address`, `multiwinia_gametype`, `_gametype_any`, `multiwinia_mapname`, `_playtime`, `_players`, `_gamemode`, `_difficulty`, `_coop`, `multiwinia_ready`, `multiwinia_open_or_cpu`, `multiwinia_clicktokick`, `teamtype_0`, `teamtype_1`, `teamtype_2`, `teamtype_demo`, `Spectator`, `multiwinia_player_join`, `multiwinia_player_leave`, `multiwinia_lobbypresstotalk`, `multiwinia_player_name`, `multiwina_playername_default`, `multiwinia_attackers`, `multiwinia_defenders`, `multiwinia_colour_green/red/yellow/blue/orange/cyan/purple/pink`, `multiwinia_passwordrequired`, `multiwinia_buy_prices`, `multiwinia_buynow`, `multiwinia_buynow_1..5`, `dialog_fullonly`, `dialog_easy_difficulty`, `multiwinia_menu_normal`, `multiwinia_menu_hard`, `multiwinia_param_*`, `multiwinia_param_*_description[_<mode>]`, `multiwinia_menu_coop_description`, `multiwinia_menu_aimode_description`, `multiwinia_menu_serverpassword_description`, `multiwinia_option_<g>_<i>_<p>`, `multiwinia_complexity_basic/intermediate/advanced`, `multiwinia_level_any` | UI text | §4, §8 | own strings |
| Errors/auth: `multiwinia_error_incompatible`, `_serverfull`, `_missingmap`, `_unknownmap`, `_requestinglist`, `_noserversfound`, `_loadingachievement`, `_kickedbyserver`, `_badpass`, `_connection`, `dialog_duplicate_key`, `authentication_demorestricted`, `authentication_result`, `dialog_auth_status`, `dialog_auth_status_demo_user`, `dialog_auth_*` status phrases (12) | UI text | §4.14, §4.15 | own strings |

<!-- from P19_help_tutorial_script -->
## IP register
| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| Dr Sepulveda (character name, advisor persona) | name/character | `code/sepulveda.*`, phrase ids `sepulveda_*` | "Advisor" |
| Sepulveda portraits `sepulveda1..5.bmp` (256×256 8-bit) | art | `data/sepulveda/` | placeholder advisor portrait set (5 frames: 3 neutral, eyes-shut, mouth-open) |
| All Sepulveda/cutscene dialogue lines (267 script `Say` lines, `tutorial_*`, `launchpad_tutorial_*`, `help_*`, `research_explanation_*`, `outro_*`, `yardend_*`, `temple_*`, `containment*`, `pixelmines_*`, …) | text | `data/language/english.txt`, `data/scripts/*` | rewrite all narration |
| Script files themselves (42 Darwinia cutscene scripts, camera choreography) | text/data | `data/scripts/*` | author new scripts |
| Darwinia location names: biosphere, biosphere_gamma, containment, generator, mine, yard, receiver, pattern_buffer, temple, launchpad, garden, escort, labs, "heaven" | name | scripts, `data/game*.txt` | generic sector names |
| "Darwinian", "Multiwinian", "Multiwinia", "Darwinia" in help strings | brand/name | `multiwinia_help_*`, `multiwinia_helppage_*` | "unit"/"drone", product placeholder |
| Multiwinia help/tutorial text (`multiwinia_help_*`, `multiwinia_help_title_*`, `multiwinia_helppage_*`, `multiwinia_animdesc_*`, `multiwinia_rmbhelp*`, `multiwinia_deselect*`, `multiwinia_shiftspeedhelp`, `multiwinia_officerterminate*`, `multiwinia_armourtoggle*`, `crate_drop`) | text | `data/language/english.txt` | rewrite |
| Tutorial titles "Part 1: The Basics", "Part 2: Advanced Concepts" | text | `multiwinia_menu_tutorial1/2` | own titles |
| Help-page illustrations `general1..5.bmp`, `help_*_*.bmp`, mode cards `mode_*.bmp`, `gear.bmp`, `control_camera*.bmp`, tab icons (`starthere`, `koth`, `spawnpoint*`, `crates`, `liftstatues`, `capturestatues`, `wallbuster`, `pulsebomb*`, `rocketriot1/2`, `reinforcements`, `blitzkrieg1/2`), `360_controller.bmp`, `background.bmp` | art | `data/mwhelp/` | new art |
| Game-mode names "King of the Hill", "Capture the Statue", "Rocket Riot", "Blitzkrieg", "Assault", "Domination" (generic terms, but the specific set/branding) | name | blueprints, phrases | review; generic names acceptable |
| Music cue names `cutscene1..5`, `endsequence`, `Demo2Intro`, `Demo2Mid` and their audio | audio | scripts / `data/sounds` | new score |
| Sepulveda UI sounds `Appear`, `Disappear`, `TextAppear` | audio | `Sepulveda::Advance`/render | new SFX |
| "Darwin logo" splash (`ShowDarwinLogo`, renderer `m_renderDarwinLogo`, sound `ShowLogo`) | brand/art | `code/script.cpp` | product logo placeholder |
| Gesture demos `squad.txt`, `engineer.txt`, `officer.txt`, `armour.txt` | data | `data/gesture_demos/` | not needed |
| `textures/godray.bmp` beam texture | art | Sepulveda highlights | generic beam |
| Cheat/profile names `AccessAllAreas`, `AttractMode`; `PurityControl` kill-switch + buy-now URL | name/behaviour | `code/script.cpp`, `code/app.cpp` | drop |

<!-- from P20_sequences_loaders_bench -->
## IP register
| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| Publisher logo bitmap | art/brand | `data/textures/ivlogo.bmp` (`LoadingScreen::RenderIVLogo`, `OberonLoader`) | `textures/publisher_logo.png` |
| "Darwinian" creature sprite used as spinner, matrix glyph, life cell, carrier | art/name | `data/sprites/darwinian.bmp` | generic "unit" sprite / "Sprite" |
| `esrb_warning` rating notice text | text (rating-board wording) | `data/language/english.txt` L2851 | regenerate from the new game's actual rating |
| Game-mode panel art (`armour, cloudyglow, darwinian, darwinian_ouch4, flag, flag_down, flag_locked, flag_up, fuelpump, fuelpumper, island, laser, muzzleflash, plusone, rocket, solarpanel, spawnpoint, statue, trunkport, turretbarrel, turretbase, water, wmd, zone-back, zone-front`.bmp) and the 6 `.anim` choreographies | art/animation | `data/animations/` | new sprite set + new choreographies (`mode_<id>.anim`) |
| Panel captions `multiwinia_animdesc_*` and mode names | text | `data/language/english.txt` L2356–2361 | rewrite |
| Intro captions `intro_1..10` (publisher credit, title, fictional server log, scientist character name) | text/character | `data/language/english.txt` L1412–1421 | new intro script text; no "Dr Sepulveda", no "Darwin Research Associates" |
| Product name as block-letter mask | brand | `code/loaders/matrix_loader.cpp:MatrixLoader::s_highlights` | new game logo mask |
| Credits text `bootloader_credits_1..44`, `multiwinia_credits_*` (real staff names, roles, third parties) | text/personal data | `data/language/english.txt` | new credits |
| Beta-tester names (38) | personal data | `data/betatesters.txt` | own tester list (with consent) |
| Loader texts `bootloader_amiga`, `bootloader_fodder_1..3` (names a third-party developer), `bootloader_raytrace_1..23`, `bootloader_life*` (20), `bootloader_soul_0..9` (incl. company name and slogan) | text/brand | `data/language/english.txt` | new texts |
| Spectrum banner / loading picture / font | art (and third-party platform homage) | `data/textures/program_darwinia.bmp`, `speccy_screen.bmp`, `speccy_font_*.bmp`, `textures/unicode/speccy.*.bmp` | new retro art |
| Demo-group style logo | art | `data/textures/dmacrew.bmp` | new logo |
| Glow sprite, ghost sprite | art | `data/textures/glow.bmp`, `data/sprites/ghost.bmp` | new FX sprites |
| Music: `speccyloader`, `matrix`, `fodder`, `pang`, `twoataris`, `dma_sc-visitors_from_dreams`, `tresk_-weeklies-_schroeders-failure`, `trash80_-_pain-fade-down`, `tresk_-_faces-of-a-fashion` (names indicate third-party chiptune artists) | audio (third-party licence) | `data/sounds/*.ogg`, `data/sounds.txt` Music events | licensed/commissioned tracks, events `LoaderA..`, `Credits`, `Intro` |
| Distributor splash and video (`msn_oberon_combosplash.bmp`, `cinemaware.avi`) | brand (third party) | `code/loaders/oberon_loader.cpp`, `egames_loader.cpp` | none (drop) |
| Attract levels and scripts (`mine`, `pattern_buffer`, `biosphere`, `mission_*_attract.txt`, camera mounts `attract1..6`) | names/levels | `data/scripts/attractmode.txt`, `data/levels/` | new level ids |
| "Stress Test" map name/description, `mp_benchmark_1` | text/level | `data/language/english.txt` L2989–2990, `data/levels/mp_benchmark_1.txt` | `stress_test_01` |
| Introversion FTP host and hard-coded account | brand/credential | `code/benchmark.cpp:BenchMark::Upload` | none — use a token-based telemetry endpoint |
| `APP_NAME` shown by the ray tracer, `DARWINIA_VERSION_STRING` in benchmark files | brand | `code/loaders/raytrace_loader.cpp:RenderMessage`, `code/benchmark.cpp` | new product constant |
| Profile names `AttractMode`, `AccessAllAreas`, `game_unlockall.txt` | names | `code/app.cpp:App::LoadProfile` | `__attract__`, `__unlockall__` |
