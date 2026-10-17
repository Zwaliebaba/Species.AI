# Collected "discrepancies" sections

<!-- from 00_overview.md -->
### From 00_overview.md

Headline discrepancies a reader of the original documentation would trip over; full tables are in each owning document.

| Document | Statement | Code behaviour | Ref | Owner |
|---|---|---|---|---|
| `docs/multiwinia_manual.pdf` (Assault) | "Play then switches around and the attacker becomes the defender … winner is the player who successfully attacks in the quickest time" | single round only; attackers win iff the bomb is destroyed; rotation code unreachable | `code/multiwinia.cpp:Multiwinia::AdvanceAssault` | [01](01_game_modes.md#docs-vs-code-discrepancies) |
| `docs/multiwinia_manual.pdf` (KOTH) | one point per second per zone | true for `ScoreMode` 0 only; three more score modes exist; a "second" is 1.1 game s | `code/multiwinia.cpp:Multiwinia::AdvanceKingOfTheHill` | [01](01_game_modes.md#docs-vs-code-discrepancies) |
| `docs/multiwinia_manual.pdf` | Handicap applies to "Domination, King of the Hill" | also an Assault option, inert there (no spawn points) | `data/multiwinia.txt` | [01](01_game_modes.md#docs-vs-code-discrepancies), [05](05_economy_and_population.md#docs-vs-code-discrepancies) |
| `docs/spirit resource system.txt` | a spirit "gradually floats away over the course of a minute"; "when your creatures die they die for good" | multiplayer spirits live ≈ 126–146 s; harvesters recycle spirits of every team | `code/worldobject/spirit.cpp:Spirit::Advance`, `code/worldobject/harvester.cpp:Harvester::AdvanceHarvesting` | [05](05_economy_and_population.md#docs-vs-code-discrepancies) |
| `docs/Network.txt` | "SERVER: Ticks at 25 FPS" | server ticks every 0.1 s (10 Hz) | `code/network/server.cpp:Server::AdvanceIfNecessary` | [20](20_architecture.md#docs-vs-code-discrepancies) |
| UI "seconds" (time remaining, reinforcement countdown) | one game second per decrement | one decrement per 11 ticks = 1.1 s | `code/multiwinia.cpp:Multiwinia::AdvanceSkirmish`, `Multiwinia::AdvanceTrunkPorts` | [05](05_economy_and_population.md#docs-vs-code-discrepancies) |

<!-- from 01_game_modes.md -->
### From 01_game_modes.md

| Doc statement (`docs/multiwinia_manual.pdf`, mode pages) | Code behaviour | Ref |
|---|---|---|
| Assault: "Play then switches around and the attacker becomes the defender … winner is the player who successfully attacks in the quickest time" | single round only; attackers win iff the bomb is destroyed; rotation code unreachable | `Multiwinia::AdvanceAssault`, §3.6 |
| Assault: "sabotage the three control stations" | 3 or 4 linked stations per map (`mp_assault_4P_1`: 4) | level files |
| Blitzkrieg: "When a player loses their home flag, they are eliminated" | also requires the capturer to hold an unbroken, not-being-recaptured chain of its flags back to its own home flag | `DoesBlitzkriegRouteExist` |
| CTS: a shattered statue "re-appears randomly in one of the Statue Zones" | respawns after a randomised ≥ 10-mode-second timer at the neutral zone farthest from existing statues (random only when none exist) | `Multiwinia::AdvanceCaptureStatue` |
| Rocket Riot: an explosion burns "all the fuel and all the Multiwinians inside, re-setting the rocket to its starting point" | fuel burns down to 50 % of its pre-explosion value; passengers are ejected (50 % on fire, the rest survive); rocket returns to Refueling | `EscapeRocket::AdvanceExploding` |
| Handicap applies to "Domination, King of the Hill" | also an Assault option (where it has no effect on shipped maps) | `data/multiwinia.txt`, §2.8 |
| Co-op "(4 players required) – Domination and King of the Hill" | any map flagged `CoopMode` / `ForceCoop`, incl. 3-player Assault maps, `mp_cts_4P_2`, `mp_tankbattle_4P` | level files |
| KOTH "scoring one point per second for every zone under their control" (also `multiwinia_help_scores`) | true for ScoreMode 0 only; three more score modes exist; a "second" is 1.1 game s | §4.2, B1 |
| Option text "Crate drop … 50% deviation" (`english.txt`) | matches `rate × (1.5 − U(0,1))` | `Location::AdvanceCrates` |
| On-screen "Press ESC to Exit" | correct for FULL (Esc opens the in-game menu); the code's direct ready-key exit exists only in testbed builds | `code/location_input.cpp` |

<!-- from 02_units_and_entities.md -->
### From 02_units_and_entities.md

| Doc | Claim | Code | Ref |
|---|---|---|---|
| `docs/spirit resource system.txt` | spirits "gradually float away over the course of a minute"; garbage-collector units and factories need spirits | MP: 6 s birth + 30–50 s floating + 90 s ascent; SP 6 + 90–150 + 180 s; only Harvesters and Engineers (plus legacy Incubators) consume souls in MP | [C] `code/worldobject/spirit.cpp:Spirit::Advance` |
| `docs/multiwinia_manual.pdf` (Officers) | "Right click on officer: set surrounding Multiwinians into formation" | formation toggles when the order point is < 50 from the officer and no radar-dish entrance is within √5; Darwinians join only if they are FOLLOW-searching within 150 and the formation (max 90) is not full | [C] `Officer::IsFormationToggle`, `Darwinian::SearchForOfficers` |
| manual (tips) | "Multiwinians in Formation are very vulnerable from behind" | consistent: the formation threat search is centred 0.66 × range ahead and members hold position | [C] `Darwinian::SearchForThreats` |
| `docs/changes.txt` | "Un-occupied Battle Cannons will no longer scare Darwinians" | `EffectGunTurretTarget` threats are only considered in single-player; in MP they are ignored entirely | [C] `Darwinian::SearchForThreats` |
| `docs/multiwinia-changes.txt` | "Fixed: Sync error involving Officers sending Darwinians into a radar dish" | current path copies `m_ordersBuildingId` from the officer; no residual issue found | [I] `Darwinian::SearchForOfficers` |
| `contrib/systemiv/lib/math/random_number.h` comments | `sfrand`/`_syncsfrand`: "−range < result < range" | actual range (−range/2, +range/2) | [C] `contrib/systemiv/lib/math/random_number.cpp:_syncsfrand` |
| `data/stats.txt` header | RATE = "secs to reload" | fractional rates truncate per entity (`unsigned char`); speeds scaled by difficulty | [C] `code/worldobject/entity.h`, `EntityBlueprint::GetStat` |
| manual (Armour) | "transport up to 100 Multiwinians … can go across water"; "Armours are vulnerable to attack" | capacity 100 and sea hovering confirmed; but **immune to lasers**, shells ÷ 3, and it cannot unload over water | [C] `Armour::Capacity`, `Armour::ChangeHealth`, `Darwinian::AdvanceInsideArmour` |
| manual (Squads) | "deadly units, but only as long as you control them"; grenade = hold right + left click | autonomous fire is probabilistic `min(1, 4/d)` per tick; `secondaryFire = primaryFireTarget && secondaryFireTarget` confirmed | [C] `Squadie::RunAI`, `code/location.cpp:Location::UpdateTeam` |
| `docs/multiwiniachanges-beta.txt` | "Darwinians surviving armour explosions (Issue 710)" listed as an issue | passengers are now ejected burning | [C] `Darwinian::AdvanceInsideArmour` |
| `docs/multiwinia-changes.txt` | "Napalm Strikes will now set fire to trees much more reliably" | trees/triffids within 100 take a flat −200 with no falloff | [C] `code/worldobject/weapons.cpp:FlameGrenade::Advance` |
| code comment in `Location::FireLaser` | "subversion lasers are half speed" | speed is the caller's: subversion Darwinians fire at 75 (half of 150) but the subversion gun turret fires at shell velocity / 4 | [C] `Darwinian::FireSubversion`, `code/worldobject/gunturret.cpp` |
| `docs/multiwinia_todo.txt` | creatures such as Triffids should belong to "a generic non-player team… reserve team 0" | a dedicated Monster team (4) and Futurewinian team (5), created lazily or at MP init | [C] `code/location.cpp:Location::GetMonsterTeamId`, `InitMonsterTeam` |
| `docs/bad guys.txt` | brainstorm: egg fields, mother wasps, flocking | implemented cycles: SporeGenerator/Spider → Egg → (spirit) → Virii; Triffid → TriffidEgg → creature | [C] §14–§19 |
| `docs/multiwinia-changes.txt` | "Virii no longer count towards your population limit" | consistent: population counts subtract `Virii::s_viriiPopulation` | [C] `code/worldobject/spawnpoint.cpp`, `code/worldobject/anthill.cpp:AntHill::PopulationLocked` |
| `code/worldobject/egg.h` comment on `EGG_DORMANTLIFE` | "total time alive while unfertilised" (120 s) | dormant eggs never decay (timer reset every tick) | [C] `Egg::Advance` |

<!-- from 03_buildings_and_world_objects.md -->
### From 03_buildings_and_world_objects.md

| Doc | Claim | Code reality | Ref |
|---|---|---|---|
| `docs/spirit resource system.txt` (Darwinia design note) | spirits of dead enemies are collected and used by factories to spawn units | Multiwinia spawn points use virtual master-emitted spirits; collected-spirit production exists only via Engineers/Harvesters delivering to spawn points and in legacy Factory/Incubator code | [C] `code/worldobject/spawnpoint.cpp`, `factory.cpp`, `incubator.cpp` |
| `docs/multiwiniachanges-beta.txt` (issue 756) | "Gun turret/Incubators emplacements shared by friendly teams don't fire/spawn" (bug report) | GunTurret ownership explicitly allows allied co-occupation (`IsFriend` check); the turret fires for the winning ally | [C] `GunTurret::RecalculateOwnership` |
| `docs/multiwinia-changes.txt` | "Virii no longer count towards your population limit" | consistent: population = `m_others.NumUsed() − Virii::s_viriiPopulation` | [C] `SpawnPoint::PopulationLocked` |
| `docs/game objects.txt` "Tree / forest" | "acts as an obstacle to anything in the air, but can be walked through" | ground units are pushed out of the trunk and crown spheres — trees block walking | [C] `Darwinian::PushFromObstructions`, `Entity::PushFromObstructions` |
| `docs/game objects.txt` "Tree / forest" | "? spawn seeds which sprout into new trees ?" (speculative) | implemented (`CreateAnotherTree`) but only crate-created trees and their offspring have a spawn budget; level trees never spread | [C] `Tree::CreateAnotherTree`, `Crate::GenerateTrees` |
| `docs/game objects.txt` "Laser Fence" | "can only be shut down by disconnecting / destroying power source" | in Multiwinia a fence keeps its level mode unless it is solar-linked (then it drops 5 s after the last friendly surge); fences are indestructible | [C] `LaserFence::Advance` |
| `docs/game objects.txt` "Wall" | "can be blown up with enough force" | matches: a single hit ≤ −300; in FULL only WallBuster paths reach it | [C] `Wall::Damage`, §7.2 |
| `docs/new building docs.txt` FenceSwitch | mentions "Pylon 1 / Pylon 2" values for passing on power surges | `FenceSwitch` has no pylon fields or surge handling (reads 2 fence links, switchValue, lockable, script, locked) | [C] `code/worldobject/switch.cpp:FenceSwitch::Read` |
| `contrib/systemiv/lib/math/random_number.h` comment | `sfrand`: "−range < result < range" | returns [−range/2, range/2] | [C] (P07) |
| `docs/multiwinia_manual.pdf` (Assault) | "sabotage the three control stations" | 3–4 linked stations per official map | [C] level files |
| `docs/multiwinia_manual.pdf` (Rocket Riot) | an explosion burns "all the fuel and all the Multiwinians inside, re-setting the rocket to its starting point" | MP: fuel burns down to 50% of its pre-explosion value; passengers are ejected (50% set on fire, the rest survive); the rocket returns to Refueling | [C] `EscapeRocket::AdvanceExploding` |
| `docs/multiwinia_manual.pdf` (CTS) | a shattered statue "re-appears randomly in one of the Statue Zones" | respawn after a randomised timer at the neutral zone farthest from existing statues (random only when none exist) | [C] `code/multiwinia.cpp` CTS block ([01](01_game_modes.md)) |
| source comment in `building.h` | "When you add an entry here remember to update building.cpp" | consistent (75 names for 75 values) | [C] |

<!-- from 04_crates_powerups_and_weapons.md -->
### From 04_crates_powerups_and_weapons.md

| Source | Says | Code / data does | Ref |
|---|---|---|---|
| `code/worldobject/crate.h` enum comments | SpawnMania "spawn constantly for 15 seconds" | 5.0 s | `Crate::RunSpawnMania` |
| same | Bitzkreig "all Darwinians will always throw grenades for 10 seconds" | 15 s global rage (`HasRage`), lasers not grenades | `Crate::RunBitzkreig`, `Darwinian::HasRage` |
| same | Forest "spawns 1–3 trees" | 1–2 trees | `Crate::GenerateTrees` |
| same | Eggs "spawns between 1 and 3 eggs" | 2–4 eggs | `Crate::RunEggs` |
| same | Rage "for 60 seconds" | 30 s task | `Task::Start` |
| same | Booster "immune to laser fire" | immune to every resistible damage type (explosives pop it) | `Darwinian::ChangeHealth` |
| same | Spam "spawns a Spam object" | 100 SpamInfections, no Spam building | `Crate::RunInfection` |
| same | Tank "a bitchin tank for 30 seconds" | commented out | `Crate::CaptureCrate` |
| `Crate::RunUncapturedCrate` (name) and `docs/sound changes.txt` "Expired (nobody captured the crate in time)" | suggest a bad effect for uncaptured crates | the function is the 10 % bad outcome of a **captured** crate; an expired crate does nothing (the call is commented out) and its `Expired` sound has no data block | `Crate::AdvanceOnGround`; `data/sounds.txt` |
| `docs/sound changes.txt` CratePowerup list | events Squad … Bitzkreig | data adds FormationBooster, FireExtinguisher, Tank (never fired) and RocketTurret/FlameTurret/NapalmStrike/SlowDown/SpeedUp/Engineer/TheBeast are never fired as such (turrets fire `GunTurret`, napalm fires `Airstrike`, speed crates and Engineer fire none) | `Crate::TriggerSoundEvent` call sites; `data/sounds.txt` |
| `data/language/english.txt` `crate_tooltip_squad` | "follow your every command for 60 seconds" | consistent (60 s, then the squad dies) | `Task::Start`, `Task::Stop` |
| `docs/multiwinia_manual.pdf` (Retribution Mode) | eliminated players periodically receive a power-up | consistent — live in code (contradicting the P08 draft, see §1.2) | `Multiwinia::AdvanceRetribution` |
| `docs/multiwinia_manual.pdf` (Crate Drop Mode) | Domination, KOTH, CTS only | consistent | `data/multiwinia.txt` |
| `docs/multiwinia-changes.txt` | "Napalm Strikes will now set fire to trees much more reliably" | trees/triffids within 100 take a flat −200 (no falloff) | `FlameGrenade::Advance` |
| `docs/multiwiniachanges-beta.txt` Issue 754 | "Permit crate/power-ups placement near trunk ports" (Assault) | only gun turrets are restricted (400 from enemy trunk ports, 200 from enemy spawn points); other power-ups are unrestricted there | `TaskManager::IsValidTargetArea` |

<!-- from 05_economy_and_population.md -->
### From 05_economy_and_population.md

| Doc / comment | Statement | Code behaviour | Ref |
|---|---|---|---|
| `docs/spirit resource system.txt` | a spirit "gradually floats away over the course of a minute" | MP: Birth 6.1 s + Floating 30–50 s (collectable) + ascent 90.1 s ≈ 126–146 s | `code/worldobject/spirit.cpp:Spirit::Advance` |
| `docs/spirit resource system.txt` | "When your creatures die they die for good and can not be respawned" | Harvesters recycle souls of every team, including your own | `code/worldobject/harvester.cpp:Harvester::AdvanceHarvesting` |
| `docs/spirit resource system.txt` | garbage collectors grab "2 or 3 at a time", "if they die they will drop their spirits"; factories need spirits | Harvester capacity 100 and sucks every soul within 75; it dies only by sucking up a grenade, which re-births its souls; spawn points use virtual master spirits; spirit factories are legacy/blocked | `harvester.cpp`, `spawnpoint.cpp`, `code/worldobject/factory.cpp` |
| `docs/multiwiniachanges-beta.txt` Issue 366 | "Magical forests produce red souls only if red around" | tree souls pick a uniform random team 0–3 regardless of surroundings (may be an unused slot) | `code/worldobject/tree.cpp:Tree::Advance` |
| `docs/multiwiniachanges-beta.txt` Issue 304 | "Engineer gets stuck trying to offload souls" | consistent: in MP an engineer can only unload at an incubator; no shipped official map has one, so engineers keep souls | `code/worldobject/engineer.cpp:Engineer::AdvanceIdle` |
| `docs/multiwiniachanges-beta.txt` Issue 612 and the `AdvanceSkirmish` comment ("ScoreMode 0: Final Population, 1: Population Per Second") | Domination scores population | ScoreMode 0 → score = spawn points owned now; 1 → cumulative spawn points per mode-second; population is never scored | `code/multiwinia.cpp:Multiwinia::AdvanceSkirmish` |
| `docs/multiwinia_manual.pdf` (via P02) | Handicap applies to "Domination, King of the Hill" | also an Assault option (inert there: no Assault map has spawn points) | `data/multiwinia.txt` |
| `docs/multiwinia-changes.txt` | "Virii no longer count towards your population limit, but have their own limit" | consistent (subtracted; 250 per team) | `SpawnPoint::PopulationLocked`, `code/worldobject/virii.h` |
| `code/level_file.h` comment on `LevelOptionDefenderReinforcementType` | "replaces the TrunkPortArmour option (which must be active for this option to have effect)" | applied whenever `AdvanceTrunkPorts` runs (`trunkportreinforcements` alone suffices); it suppresses port armour for that side | `Multiwinia::AdvanceTrunkPorts` |
| `code/level_file.h` comment on `LevelOptionNoGunTurrets` | "disables GunTurret tasks from TrunkPorts on RocketRiot" | disables all turret-wave tasks (incl. Blitzkrieg AirStrike upgrades) in any mode (used on `mp_blitzkrieg_2P_4`) | `Multiwinia::AdvanceTrunkPorts` |
| `code/level_file.h` comment on `LevelOptionTrunkPortReinforcements` | "activates the RocketRiot style trunk port reinforcements on a level" | in Rocket Riot/Blitzkrieg its count is ignored (game option wins) but it adds a second call per tick | `Multiwinia::Advance` |
| `SpawnPoint::Advance` comment | instant activation for "our first (or last) spawn point" | triggers whenever the team owns exactly one point at that moment | `code/worldobject/spawnpoint.cpp:SpawnPoint::Advance` |
| UI "seconds" (time remaining, reinforcement countdown) driven by `m_timer = 1.0` / `m_reinforcementTimer = 1.0` resets | one game second per decrement | one decrement per 11 ticks = 1.1 s, so displayed seconds and wave timers run 10 % slow relative to game time | `code/multiwinia.cpp:Multiwinia::AdvanceSkirmish`, `Multiwinia::AdvanceTrunkPorts`; §8.2 |
| `Multiwinia::Render` comment `// Rocket Riot` on the reinforcement countdown | countdown is a Rocket Riot feature | drawn on every map where trunk-port waves run (Assault, Blitzkrieg, `mp_koth_3P_4`, `mp_cts_3p_3`) | `code/multiwinia.cpp:Multiwinia::Render` |

<!-- from 06_player_control_and_input.md -->
### From 06_player_control_and_input.md

| Doc / comment | Claim | Code reality |
|---|---|---|
| `docs/input_preferences_syntax.txt` | grammar lists no `repeat`, `~` icon lines, `alias`, `value`, `->` pipe, `mouse any`, `key any` | `repeat` exists for XInput BOOL controls; `~` sets icons; `alias`/`value` drivers exist; pipe exists but is unregistered; `any` parses but never fires [C] §11.6 |
| same | `XInput <xinput_analog> <analog_cond> [with sensitivity <1-200>]` | sensitivity is also accepted after BOOL conditions on analogue controls and rejected on directional stick buttons [C] `XInputDriver::writeExtraSpecInfo` |
| same | "The user input preferences will override the FIRST binding" | true, and the keyboard-layout file does the same; repeated lines in one override file overwrite each other [C] §11.10 |
| same | unknown/invalid specs implied to fail | unknown condition words are silently accepted as `down` [C] §11.3 |
| `docs/DarwiniaInput.rtf` | action words include `moved` | the word is `move`; `moved` hits the fallback and acts as `down` [C] |
| same | `lib/input/transform.h` holds `Movement2D` | it is in `code/lib/input/movement2d.h` [C] |
| same | `g_controlBindings->ControlEvent(ControlBindings::ControlDeselect)` (old API) | current API is `g_inputManager->controlEvent(ControlUnitDeselect)`; no `ControlDeselect` exists [C] |
| `docs/betachanges.txt` | fixes for "Gestures Mode", "Icons task manager used when use Xbox 360 controller", Tab in Icons mode | these concern the Darwinia(+) interfaces, which exist but are unreachable in Multiwinia (§4.1) [C] |
| `docs/game objects.txt` | power-ups "triggered by a gesture of some kind" | Multiwinia power-ups are placed by click, never by gesture [C] `TaskManagerInterface::CreateTaskManager` |
| `docs/todo.txt` | key-binding window issues | the Controls options window is unreachable in the shipped build [C] `OptionsMenuWindow::Create` |
| DebugMenu captions "(F2)…(F9)" | debug hot-keys | no such bindings; F1–F8 are legacy task-manager keys, F9 = network stats [C] `data/input/input_preferences.txt` ([32](32_debug_test_and_performance.md)) |
| `code/location_input.cpp` comments "Player pressed CTRL-C, so demote this officer" / "so terminate this turret" | demote / terminate | the bound key is plain C; the officer is killed (`ChangeHealth(−999)`), the turret is only deselected (the damage call is commented out) [C] |
| `code/camera.h` comment | "Remember to update the static string table at the end of camera.cpp" | no mode-name table exists [C] |
| `code/camera.cpp` comment on `DistanceToBlockage` / `DirectDistanceToBlockage` | "If there is no blockage FLT_MAX is returned" | returns `_maxDist`; FLT_MAX only when there is no location [C] |
| `code/lib/input/keynames.cpp` comment | `"QUOTE", //192` | QUOTE is index 222; MINUS_PAD/Unknown swapped vs `keydefs.h` [C] |
| `code/lib/input/control_types.inc` comments | `CameraRotate` "rotate the cam in entity track mode"; `WeaponCycle*` "cycle weapons" | no consumer in the FULL build [C] |
| `code/lib/input/control_bindings.cpp` comment | `ControlBindings::replacePrimaryBinding` | commented out; replace is a flag on `bind` [C] |
| `docs/multiwinia_manual.pdf` (Officers) | "Right click on officer: set surrounding Multiwinians into formation" | consistent: RMB on an idle officer → `TOGGLEENTITY` → formation (§5.3); Darwinians join only if FOLLOW-searching within 150 u and the formation (90) is not full [C] |

<!-- from 07_ai.md -->
### From 07_ai.md

| Document / comment | Claim | Code | Ref |
|---|---|---|---|
| `docs/integrating virtual players into network system.txt` | a client may play "with an AI running locally" | the AI is a simulation entity run identically on every peer; CPU teams produce no network commands | `code/location.cpp:Location::InitialiseTeam`, `code/team.cpp:Team::Advance` |
| `docs/multiwinia_todo.txt` (Outstanding issues) | "Creating a CPU player should automatically spawn an AI entity" is outstanding | implemented: `Location::InitialiseTeam` and `Multiwinia::RemoveTeams` spawn one | `code/location.cpp`, `code/multiwinia.cpp` |
| `code/worldobject/aiobjective.h` header comment | several unlinked start objectives → "one will be selected randomly" | all stay active; each AI picks one (random, or a defender waits for the first one attacked) and deactivates its other own start objectives | `AI::AdvanceAssaultAI` |
| `docs/multiwiniachanges-beta.txt` v1.9.7 | "Crash AI::RunForest (Issue 328)" fixed | `RunForest` still dereferences `GetEntity(specials[i])` without a NULL check (07-U11) | `AI::RunForest` |
| `contrib/systemiv/lib/math/random_number.h` | `_syncsfrand`: "−range < result < range" | returns `(U[0,1) − 0.5) × range`, i.e. ±range/2 | `contrib/systemiv/lib/math/random_number.cpp:_syncsfrand` |
| `code/worldobject/ai.h` comment on `AITarget::m_areaControl` | pass / always-update scheme for clustered targets | field never read or written | `code/worldobject/ai.cpp` |
| `code/worldobject/armour.cpp` comment "bit of a cheat" | self-destruct at enemy spawn points is a shortcut | live AI-only behaviour (Standard armour) | `Armour::RunStandardAI` |
| `docs/multiwiniachanges-beta.txt` v1.9.7 "AI places two turrets inside one another (Issue 350)", "KOTH AI abandons hills (Issue 352)", "Multiple officers at the same point (Issue 314)" | fixed | consistent with code: `PushFromTurrets` + turret `DoesSphereHit(10)` validity; KOTH garrison 50/10 and hill override; `SearchForOfficers` checks (no discrepancy — recorded for traceability) | `code/worldobject/ai.cpp`, `code/taskmanager.cpp` |

<!-- from 08_levels_and_level_design.md -->
### From 08_levels_and_level_design.md

| Doc (or code comment) | Says | Code / data | Ref |
|---|---|---|---|
| `docs/changes.txt` ("Editor changes") | moving a land tile moves its buildings; option "in the land tiles menu" | behaviour confirmed; the toggle `editor_movebuildings` is in the main Landscape window shown in land-tile mode, not in the per-tile window | `LocationEditor::AdvanceModeLandTile`, `LandscapeEditWindow::Create` |
| `docs/top level map.txt` | "Each level file specifies a map file at the start"; static vs dynamic buildings per map/level | Darwinia pairing is given by `data/game.txt` `Locations` rows; Multiwinia maps are single files with mission `"null"`; the `m_dynamic` flag exists (`editor_dynamic`) | §2.1, §7.1 |
| `docs/new building docs.txt` (FenceSwitch editor linking order) | fence switches are placed and linked in the editor | `FenceSwitch` is a blocked type in FULL/editor builds and occurs in no level file | §6.3 |
| `docs/multiwinia_manual.pdf` (per P02's reading) | Assault: "sabotage the three control stations" | 3 linked stations, 4 on `mp_assault_4P_1` | §4.5 |
| `docs/multiwinia_manual.pdf` (per P02's reading) | co-op "(4 players required) – Domination and King of the Hill" | `CoopMode`/`ForceCoop` also on CTS `mp_cts_4P_2`, the 3-player Assault maps and `mp_tankbattle_4P` | §5.6 |
| `code/level_file.cpp:ParseLights` comment | mission-file lights take precedence over map lights | mission files cannot contain a `Lights` block | §2.5 |
| `code/level_file.h` comment on `LevelOptionNoHandicap` | "Handicap is overridden to 0" | the handicap function returns a multiplier of 1.0 (no handicap) | §5.2 |
| `code/level_file.h` comment on `LevelOptionFuturewinianTeam` | "the team id specified here" | compared with the team's **start position** | §5.2 |

<!-- from 09_meta_systems.md -->
### From 09_meta_systems.md

| Doc | Doc says | Code does | Ref |
|---|---|---|---|
| `docs/multiwinia-changes.txt` (v1.1.0) | "Server filter settings are now saved" | the four visible checkboxes (plus the dead custom-maps flag) are saved; the **game-type filter** drop-down is not | `code/UI/FinishMultiwiniaButton.h:SaveFiltersButton` |
| `docs/multiwinia-changes.txt` | "Server hosts may now password their servers" | hosting with a password works, but the browser's "show passworded" filter cannot detect passworded servers (`"dy"` vs `"dI"`) | §5.8.6 |
| `docs/multiwinia-changes.txt` (v1.2.0) | demo users may join any LAN game on any map if hosted by a full-game player | not implemented client-side; the client only reacts to the listing's demo-restricted flag [I] | §6.2 |
| `docs/multiwinia-changes.txt` | "Port forwarding configuration in Help and Options \| Settings \| Other Options" | matches (no discrepancy) | `code/interface/prefs_other_window.cpp` |
| `docs/multiwinia-changes.txt` | power-ups in the Advanced Tutorial terminate correctly upon use | consistent only after the T2 end step (`TerminateTask` blocked while `!m_tutorial2AICanSpawn`) [C + I] | [30 §2.2](30_scripting_tutorial_and_cutscenes.md) |
| `docs/multiwiniachanges-beta.txt` issue 264 | Back on map select goes to the lobby; Back on the lobby goes to game mode | MapSelect Back → GameSelect; lobby Back → GameSelect (host), MapSelect (single player), NewOrJoin (client) | §2.5 |
| `docs/multiwiniachanges-beta.txt` issue 412 | "Leaderboards and Achievements appear in the in-game menu" (fixed) | the in-game `LocationWindow` has neither; the front-end pages exist but are unreachable/empty; the only buttons are in the unreachable Darwinia `MainMenuWindow` (inactive / handler-less) | §7.2, §7.5 |
| `docs/todo.txt` | "Leaderboards — DONE (?)" | no leaderboard backend; UI unreachable stub | §7.5 |
| `docs/todo.txt` | water reflection linked to the pixel-effect option | true only under D3D; the pixel-effect control itself is commented out | §11.6 |
| `docs/todo.txt` | key-bindings window issues | the Control Options window is unreachable in the shipped build | §11.9 |
| `docs/help system.txt` | Sepulveda context help with F1, fixed messages, failure help and action items (GestureBasics, ThrowAirstrike, …) | key is `SepulvedaHelp = key h`; the enum has the 20 items of Appendix A.4; no fixed-message or failure-help classes; the whole system is compiled out of Multiwinia | `code/helpsystem.h`, `data/input/input_preferences.txt` L255 |
| `docs/multiwiniareleasenotes-beta.txt` L35 | beta records benchmarks and uploads them via FTP "when you exit" | feature compiled out in every target (`//#define BENCHMARK_AND_FTP`); when enabled the upload happens at the end of each match | `code/lib/universal_include.h`, `code/benchmark.cpp` |
| `docs/changes.txt` L82–84 | "Show First Time Intro" option replays the intro next time the game starts | the intro runs only from `RunBootloader` (Campaign launch, `USE_LOADERS`); the option row is not built in the Multiwinia UI | `code/main.cpp:RunBootloader`, `code/interface/prefs_other_window.cpp` |
| `docs/detailed schedule.txt` L19 | "(OPTIONAL) Intro bootloaders" planned | implemented but compiled out (`USE_LOADERS`) | `code/lib/universal_include.h` |
| `docs/todo.txt` L19, `docs/betachanges.txt` L46/L57, `docs/buglist.txt` L21 | boot-loader issues (Game-of-Life glow frame rate, Spectrum text distortion, ray-tracer performance, widescreen stretching) | loaders not compiled in Multiwinia; widescreen stretching is consistent with the fixed 320 × 256 / 800 × 600 canvases | Appendix A.1 |
| `docs/detailed schedule.txt` ("Translation System → Missed out") | the debug / sound / editor windows are untranslated | consistent — literal English captions | §10.2 |
| `tools/language/README.txt` | translation inputs must be UCS-2 BE | `data/language/japanese.txt` and `russian.txt` are UTF-16 LE; the game accepts both BOMs | §10.2 |
| `docs/RussianFontRequest.txt` | only the Speccy font has Cyrillic | Cyrillic atlases exist for editor, speccy and square (but the "all" language list ships only the speccy ones) | §10.3–10.4 |
| `contrib/systemiv/lib/math/random_number.h` comment | `sfrand(range)` returns values in (−range, range) | implementation returns `(0.5 − U)·range`, i.e. ±range/2 (affects the dead Soul / DemoEnd spawn spreads) | Appendix A.1 |

<!-- from 10_audio_design.md -->
### From 10_audio_design.md

| Doc / comment | Says | Code / data does | Ref |
|---|---|---|---|
| `docs/sound changes.txt` (28 Aug 2007) | new crate event `Expired` | fired by code; no data → silent | `code/worldobject/crate.cpp:Crate::AdvanceOnGround`; `data/sounds.txt` `BUILDING Crate` |
| `docs/sound changes.txt` (19 Sep 2007) | carryable-building events `Lift`, `Carry (loop)`, `Drop` | all three fired; only `Statue Drop` is bound (`statuedrop3`; `StatueDrop`, `StatueDrop2`, `StatueDrop4`, `Drop_Statue`, `Drop_Statue2`, `carry1`–`4` unused) | `code/worldobject/carryablebuilding.cpp:CarryableBuilding::Advance` |
| `docs/sound changes.txt` (28 Aug 2007) | `CratePowerup` event list (Squad … Bitzkreig, incl. PersonalShield, Infection) | data adds FireExtinguisher, FormationBooster, Tank (never fired) and FlameTurret (never passed); lacks Engineer, RocketTurret, NapalmStrike, SlowDown, SpeedUp, TheBeast | `code/worldobject/crate.cpp:Crate::GetName`, `Crate::TriggerSoundEvent` |
| `docs/sound changes.txt` (Requested) | microphone position; "drop all volumes to highlight a single sound event"; Darwinian orders spread over a second; lift frequency by number of carriers | microphone and highlight not implemented; orders not spread; the lift-chant frequency **is** implemented (`soundFrequency = 15 + 20 × (1 − percentLifted)`) | `code/sound/soundsystem.cpp:SoundSystem::Advance`, `code/worldobject/darwinian.cpp:Darwinian::AdvanceCarryingBuilding` |
| `docs/multiwinia audio.docx` | Game Over / You've won / You've lost sounds; last-10-seconds timer; "Age of object" parameter link; Dark Forest and ghost ambiences; KotH warning alarm; CTS cheers / sighs | match-end events fired but unbound; last-10-s ticks implemented; no age link; no Dark Forest / ghost ambience (only `Tree CreateEvil` speech); no zone warning alarm; no cheers / sighs | `code/multiwinia.cpp:Multiwinia::DeclareWinner`, `code/sound/sound_parameter.h:SoundParameter` |
| `docs/ThoughtsOnSoundSystem.txt` | 2D buffer fixed at 2000 samples; linear / log graph toggle wanted; "Sort out SetChannelVolume / SetChannel3dMode for software mixer" | buffer is pref `SoundBufferSize`; the toggle is commented out; both software-mixer functions are implemented | `code/sound/sound_library_2d_win32.cpp`, `code/interface/soundparameter_window.cpp`, `code/sound/sound_library_3d_software.cpp` |
| `docs/adsr.gif` | classic ADSR with a decay stage and a sustain level | no decay stage; SUSTAIN is a hold duration; sustain level = VOLUME | `code/sound/sound_instance.cpp:SoundInstance::UpdateChannelVolume` |
| `docs/MinDistAndVolume.jpg` | flat to the minimum distance, then fall-off | consistent (`min/d`) | `code/sound/sound_instance.cpp:SoundInstance::CalculatePerceivedVolume` |
| `data/effects.txt` header | "Default: the starting value" | defaults are not applied to parameters omitted in `sounds.txt` (they are 0); the editor seeds new effects with the minimum | `code/sound/soundsystem.cpp:SoundSystem::ParseSoundEffect` |
| `code/sound/sample_cache.cpp` comment | "Largest song takes 13 MB" | the largest track decodes to 30 389 760 bytes (29.0 MiB) but no music is shipped; the largest shipped files are `data/sounds/altitude1.ogg` (91.3 s ≈ 8.05 MB decoded, dead data) and, live, `data/sounds/evil.ogg` (53.9 s ≈ 4.75 MB) | `code/sound/sample_cache.cpp:CachedSampleManager::CleanUp` |
| `code/sound/sound_instance.h` comment | VOLUME 0.0 = "silence" | 0.0 = −50 dB on DirectSound | `code/sound/sound_library_3d_dsound.cpp:SoundLibrary3dDirectSound::SetChannelVolume` |
| `code/sound/soundsystem.h` comment on `StopAllSounds` | "Full event name required" | substring match | `code/sound/soundsystem.cpp:SoundSystem::StopAllSounds` |
| `code/worldobject/gunturret.cpp:GunTurret::ListSoundEvents` | lists `RegisteredKill` | the code fires `RegisterKill` (bound in data) | `code/worldobject/gunturret.cpp` |
| `code/sound/soundsystem.cpp:SoundSourceBlueprint::ListSoundEvents` | Music list without `LoaderAmiga`; MultiwiniaInterface list without `UnitHighlight` | both are bound in data and fired by code | `data/sounds.txt` |
| `tools/unused_samples.sh`, `tools/unused_sample_groups.sh` | "run this script from the Darwinia data directory" (helpers for finding unused samples) | they ignore `data/mwsounds/` and orphan-group membership; CRLF in `sounds.txt` breaks the comparison on POSIX shells [I] | `tools/unused_samples.sh` |

<!-- from 20_architecture.md -->
### From 20_architecture.md

| Document | Claim | Code reality | Ref |
|---|---|---|---|
| `docs/Network.txt` | "SERVER: Ticks at 25 FPS"; server loop sleeps until `lastFrameTimeStamp + 1/25` | server ticks every 0.1 s (10 Hz) of real time, at most once per main-loop call, never sleeps | `code/network/server.cpp:Server::AdvanceIfNecessary` |
| `docs/Network.txt` | "CLIENT: Runs heavy weight physics at 25 fps … as a separate thread (at least conceptually)" | 10 ticks per second, each split into 10 slices, executed on the main thread interleaved with rendering | `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters` |
| `docs/NetworkThreads.txt` | dedicated Server Send Thread and Client Send Thread that `Suspend()` when the queue is empty | sends happen synchronously on the main thread (`AdvanceSender` in `ClientToServer::Advance` and `Server::Advance`); only receiving is threaded | `code/network/clienttoserver.cpp:ClientToServer::AdvanceSender`, `code/network/server.cpp:Server::AdvanceSender` |
| `docs/NetworkThreads.txt` | main thread runs "client heavy weight advance" when `time >= nextClientTimeSlice` and "light weight advance … if there is time" | heavy-weight work is metered by the 30 %-of-backlog rule every frame; the "light-weight advance" is the render-time extrapolation with `g_predictionTime` | `code/network/clienttoserver.cpp:GetNumSlicesToAdvance`, `code/gametimer.cpp:GetPredictionTime` |
| `docs/NetworkThreads.txt` | light-weight advance extrapolates from the last received velocity | no state is received; rendering extrapolates the locally simulated state | §10.6 |
| `code/gametimer.h` (comment) | `GameTimer` "Replaces g_gameTime, GetNetworkTime, SERVER_ADVANCE_PERIOD etc" | `g_gameTime` still exists as a separate wall-clock variable; `GetNetworkTime`/`SERVER_ADVANCE_PERIOD` became macros over `GameTimer` | `code/main.cpp`, `code/gametimer.h` |
| `code/globals.h` (comment) | `MINIMUM_RENDER_PERIOD` "Render at least this frequently" | unused | grep |
| `code/app.cpp` (comment) | main-thread affinity `0x02\|0x08\|0x20\|0x80` "Even processors" | zero-based CPUs 1, 3, 5, 7 | `code/app.cpp:App::App` |
| `code/app.cpp`, `code/loading_screen.cpp` (comments) | worker affinity `0x01\|0x04\|0x10\|0x40` "Processor Number % 3 == 2" / "Odd processors" | zero-based CPUs 0, 2, 4, 6 | `SetWorkingThreadProcessor` (both files) |
| `code/network/clienttoserver.cpp` (comment) | catch-up time limit "checked every 500 iterations" | checked every iteration | `ClientToServer::AdvanceAndCatchUp` |
| `code/lib/window_manager_win32.cpp` (comment) | secondary GL context created "for loading thread" | never made current on any thread; workers marshal GL work to the main thread | `WindowManagerWin32::EnableOpenGL` ([24](24_rendering.md)) |

<!-- from 21_simulation_and_entity_model.md -->
### From 21_simulation_and_entity_model.md

| Source | Says | Code | Ref |
|---|---|---|---|
| `docs/Network.txt` | the server sleeps until `lastFrameTimeStamp + 1/25`; the client "runs heavy weight physics at 25 fps" | the server ticks at 10 Hz (0.1 s real time) and each tick is executed as 10 slices | [C] `code/network/server.cpp:Server::AdvanceIfNecessary`, `code/globals.h` (`NUM_SLICES_PER_FRAME`) |
| `docs/Network.txt` | heavy-weight physics "runs as a separate thread (at least conceptually)" | the simulation runs on the main thread interleaved with rendering; only the initial `Location::Init` runs on the loading worker thread | [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters`, `code/main.cpp:DoEnterLocation` |
| `docs/game objects.txt` ("Tree / forest") | "Acts as an obstacle to anything in the air, but can be walked through" | trees are in the obstruction grid; ground units are pushed out of the trunk and crown spheres (`Entity::PushFromObstructions`, `Darwinian::PushFromObstructions`) | [C] `code/worldobject/entity.cpp:Entity::PushFromObstructions` |
| code comment, `code/routing_system.cpp` / `code/location.cpp:IsWalkable` | "Only check the surface normal if the gradient is sloping upwards" | the gradient is measured against the start point, not between samples | [C] §7.2 |
| code comment, `code/obstruction_grid.cpp:CalculateBuildingArea` | fill the 4-neighbours only when a sample skipped diagonally | shadowed `oldCellX/oldCellY` make it fill them for every sample with non-zero cell coordinates | [C] §6.4 |
| code comment, `code/worldobject/worldobject.cpp:BounceOffLandscape` | impact point = midpoint of last and current position | `lastPos = m_pos` (the velocity term is commented out): no midpoint | [C] §8.6 |
| code comment, `code/unit.h:Unit::GetOffset` | "Takes into account formation AND obstructions" | obstructions ignored | [C] §9.6 |
| code comment, `code/team.h:Team::m_specials` | "Officers and tanks for quick lookup" | also armour, harvesters, engineers, shamans | [C] §9.2 |
| `code/lib/slice_darray.h` declaration | constructor `(newstepsize, totalNumSlices)` | defined `(totalNumSlices, newstepsize)` | [C] §5.3 |
| code comment, `code/location.h:ChristmasModEnabled` | "0 = unavailable, 1 = enabled, 2 = disabled" | returns a bool; callers compare `== 1` | [C] `code/location.cpp:Location::ChristmasModEnabled` |

<!-- from 22_determinism_and_networking.md -->
### From 22_determinism_and_networking.md

| Doc | Doc says | Code does |
|---|---|---|
| `docs/Network.txt` | the server ticks at **25 FPS**; the client "runs heavy weight physics at 25 fps"; the server sleeps until the next 1/25 s | server tick every 0.1 s (10 Hz) of real time, no sleep; each tick is executed as 10 slices on the main thread, consumed at ~30 % of the backlog per render frame [C] `Server::AdvanceIfNecessary`, `GetNumSlicesToAdvance` |
| `docs/Network.txt` | each chunk also sends copies of the previous two frames | at most one piggy-backed copy (oldest unacknowledged) + up to 3 letters per tick [C] `UpdateClients` |
| `docs/Network.txt` | watchdog: a client is dead after 1.0 s of silence | 20.0 s [C] |
| `docs/Network.txt` | the client sends a "ResyncMe" message on gaps; I'm-alive only when idle | no resync message — cumulative ack `LASTSEQID` on every letter; gaps are filled by the resend window; a desync is never repaired; ALIVE always every 0.1 s in game [C] `ClientToServer::ReceiveLetter`, `Server::UpdateClients` |
| `docs/NetworkThreads.txt` | separate client/server send threads that suspend when idle | sends happen synchronously on the main thread (`AdvanceSender` at the end of `ClientToServer::Advance` and inside `Server::Advance`); only receive/listen paths are threads [C] |
| `docs/NetworkThreads.txt` | the main thread runs the "client heavy weight advance" only when `time >= nextClientTimeSlice`, and a "light weight advance" extrapolates positions from the last received velocity | heavy-weight work is sliced (10 slices/tick) and interleaved with every rendered frame; there is no state reception — rendering extrapolates locally via `g_predictionTime` [C for no state; I for rendering] |
| `docs/network overview.txt` | lobby over **TCP**, broadcast "Are you a server?" query, LOAD MAP + confirm handshake, then UDP 4000/4001 | everything is UDP in one channel; servers broadcast REGISTER every 5 s (clients never query); START_GAME is just a sequenced command — no confirm; clients catch up after loading [C] |
| `docs/network overview.txt` | ports 4000 server / 4001 client | matches the defaults [C] |
| `docs/integrating virtual players into network system.txt` | `ServerToClient` index = team id; proposes separate team and client arrays; a client may host "an AI running locally" | implemented as proposed (`m_teams` + `m_clients`); the AI is a simulation entity executed identically on all peers, with no input messages [C] |
| `docs/multiwinia_todo.txt` | "(DONE) … fix determinism; ensure floating point accuracy over multiple platforms" | implemented as `_PC_53` precision control plus binary32 rounding of transcendental results [C] |
| `docs/multiwiniareleasenotes-beta.txt` (1.9.20) | beta builds enable debugging code to track rand sync errors | in this tree `TRACK_SYNC_RAND` is commented out for `TARGET_BETATEST_GROUP_ALL`; beta targets instead hash the full IFrame snapshot [C] `code/lib/universal_include.h`, `code/network/iframe.cpp` |
| `docs/multiwinia-changes.txt` | "Fixed: Sync error involving Officers sending Darwinians into a radar dish" | current path `SearchForOfficers` copies `m_ordersBuildingId`; no residual issue identified [I] |
| `contrib/systemiv/lib/math/random_number.h` comments | `AppRandom < APP_RAND_MAX`; `frand`: `0 ≤ r < range`; `sfrand`, `_syncsfrand`: `−range < r < range` | `AppRandom ≤ 32767` inclusive; `frand ∈ [0, range]`; `sfrand ∈ [−range/2, range/2]`; `syncsfrand ∈ [−range/2, range/2)` [C] `random_number.cpp` |
| `code/lib/vector3.h` comment | `operator==` uses FLT_EPSILON | absolute tolerance 1e-6 (`NearlyEquals`) [C] |
| `code/lib/matrix33.h` inline comment | `DecomposeToYDR()` returns "x = dive, y = yaw" | returns (yaw, dive, roll) in (x, y, z) [C] |
| `code/network/clienttoserver.cpp` comment | team requests are sent "once a second" | the gate fires every 0.1 s [C] |

<!-- from 23_terrain_and_landscape.md -->
### From 23_terrain_and_landscape.md

| Document / comment | Says | Code does | Ref |
|---|---|---|---|
| `docs/landscape lightmap.txt` | Landscape has `GenerateLightMap`, `ApplyLightmap`, `DeleteLightMap`: ray-cast shadows from a point light at (700, 200, 100), shade 255/50, optional 5 × 5 smoothing, a lightmap texture | none of these functions or members exist; terrain is only GL vertex-lit by two level lights; the only "lightmap" is the water shore mask; only `LIGHTMAP_SCALEFACTOR` survives | §12 |
| `docs/OpenGLState.txt` | fog start 800, end 2000 | fog derived from world size: end = max(4000, size + 1000), start = end/3 | `code/location.cpp:Location::SetupFog`; [24](24_rendering.md) |
| `code/landscape.h` comment on `m_guideGridPower` | "Log to the base 2 of the resolution of the guide grid" | resolution = 2^p − 1, so p = log₂(resolution + 1) | `code/landscape.cpp:LandscapeTile::GuideGridSetPower` |
| `contrib/systemiv/lib/math/random_number.h` comment on `sfrand` | "−range < result < range" | result is in [−range/2, +range/2] (same for `Landscapesfrand`) | `contrib/systemiv/lib/math/random_number.cpp:sfrand` |
| `code/lib/math_utils.h` comment on `Round` | may round up or down depending on the CPU rounding mode | the active macro is `(int)(a)`: always truncates toward zero (the `fistp` version is commented out) | `code/lib/math_utils.h:Round` |
| `code/landscape.h` declaration `GenerateMidpoints(int _x1, int _x2, int _z1, int _z2)` | parameter order x1, x2, z1, z2 | definition and call use (x1, z1, x2, z2); names only, behaviour consistent | `code/landscape.cpp:LandscapeTile::GenerateMidpoints` |
| Editor label `editor_noiseperiod` for `m_fractalDimension`; file header label "frac dim" | a period / a fractal dimension | the value is the exponent of the per-level amplitude law; larger means smoother (§5.4) | `code/interface/landscape_window.cpp:LandscapeTileEditWindow::Create` |
| `code/water.h` header comment | sea plane twice the land size; depth modulates brightness, wave amplitude and colour | matches; additionally deep water is not drawn at all (flat tiles skipped where all depths ≥ 1, wave vertices where depth > 0.999) | §13 |
| Phase-2 P12 render-pass draft (not a docs file; recorded for traceability) | flat-water texture unit 1 is the "landscape lightmap" | it is the water shore mask `"water_lightmap"` | §12 |

<!-- from 24_rendering.md -->
### From 24_rendering.md

| Doc / comment | Says | Code does | Ref |
|---|---|---|---|
| `docs/OpenGLState.txt` | fog start 800, end 2000 | start `fogEnd/3`, end `max(4000, worldSizeX+1000, worldSizeZ+1000)`; `CheckOpenGLState` asserts end ≥ 4000 | `code/location.cpp:Location::SetupFog`, `code/renderer.cpp:Renderer::CheckOpenGLState` |
| `docs/OpenGLState.txt` | fog colour `g_app->m_world->m_backgroundColour` | `g_app->m_backgroundColour` (constant black, alpha 0); the D3D path ignores fog colour entirely | `Location::SetupFog`, `code/lib/opengl_directx.cpp:glFogfv` |
| `docs/OpenGLState.txt` | lights from `g_app->m_world->m_lights[x]` with ambient/diffuse/position/specular | `Location::SetupLights` (or `GlobalWorld::SetupLights`): direction + colour, ambient (0,0,0,1), LIGHT1 zeroed first, only LIGHT0/1 ever enabled | `code/location.cpp:Location::SetupLights` |
| `docs/OpenGLState.txt` | `glTexEnvf(GL_TEXTURE_ENV_MODE, GL_MODULATE)` | `glTexEnvi(…MODULATE)` plus `glTexEnviv(GL_TEXTURE_ENV_COLOR, {0,0,0,0})` | `Renderer::SetOpenGLState` |
| `docs/OpenGLState.txt` | lists `GL_TEXTURE_MAG_FILTER` twice "undefined" (MIN missing) | neither filter is part of the default state; every textured pass sets its own; uploads default LINEAR / LINEAR_MIPMAP_LINEAR | §3.2 |
| `docs/OpenGLState.txt` | no mention of `GL_NORMALIZE`, `GL_LIGHT_MODEL_AMBIENT`, `GL_SCISSOR_TEST`, lights 2–7 | code disables NORMALIZE, sets light-model ambient (0,0,0,0), disables scissor and all 8 lights | `Renderer::SetOpenGLState` |
| `docs/OpenGLState.txt` | trailing `glMaterialfv(GL_FRONT, …)` and `allegro_gl_set_texture_format(GL_INTENSITY8)` | no Allegro anywhere in `code/`; materials set by `Renderer::SetObjectLighting` (specular 0, diffuse 1, ambient 0, shininess 127) | `Renderer::SetObjectLighting` |
| `docs/OpenGLState.txt` (implied contract) | the documented state is the API contract | under [B:D3D] shade model, line/point smooth, fog colour/density, light-model ambient and material state are not emulated | §1.5.4 |
| `docs/HardwareComplianceIssues.txt` | "Multitexturing ARB not supported" on a Linux test machine | no Win32 fallback: `gglActiveTextureARB` / `gglMultiTexCoord2fARB` are called unguarded (flat water, fences, radar dish) | `code/lib/ogl_extensions.cpp`, `code/water.cpp:Water::RenderFlatWater` |
| `docs/HardwareComplianceIssues.txt` | "Mouse pointer background not transparent" | no explicit fix; Win32 cursors are drawn additively by the game (24-U9) | `code/gamecursor.cpp:MouseCursor::Render` |
| `docs/landscape lightmap.txt` | `Landscape::GenerateLightMap/ApplyLightmap/DeleteLightMap`: ray-cast shadows from a point light at (700,200,100), shade 255/50, optional 5×5 filter | none of these exist; terrain is only GL vertex-lit; the only "lightmap" is the water shore mask; orphan `LIGHTMAP_SCALEFACTOR` remains | `code/landscape.h`, `code/water.cpp:Water::GenerateLightMap` |
| `docs/sphere world.txt`, `docs/top level map.txt` | the sphere world is the hub between islands | present and built at start-up but unreachable in Multiwinia; the menu shows only clouds | `code/main.cpp:GlobalWorldGameLoop`, `code/game_menu.cpp:GameMenu::Render` |
| `code/water.h` header comment | sea plane twice the land size; depth darkens and shrinks waves | matches; additionally deep water is not drawn at all | `code/water.cpp` |
| comment in `code/lib/render_utils.h` | split quads are needed for Radeon fog artefacts | `RenderSplitUpQuad*` have no callers (clouds subdivide inline) | `code/lib/render_utils.cpp`, `code/clouds.cpp:Clouds::RenderQuad` |
| comment in `Renderer::ApplyPixelEffect` | pass 4 "Additive smooth" | uses NEAREST (blocky) | `code/renderer.cpp:Renderer::ApplyPixelEffect` |
| comment on `ParticleType::m_spin` | spin in rad/s; fade-in/out times | `m_spin`, `m_fadeInTime`, `m_fadeOutTime` never read; render fade hard-coded | `code/particle_system.cpp` |
| signature `DeformEffect::AddPunch(Vector3 pos, float range)` | takes a range | argument ignored (`m_range = 1`) | `code/deform.cpp:DeformEffect::AddPunch` |
| comment in `WindowManagerWin32::EnableOpenGL` | secondary GL context "for loading thread" | never made current; no loading-thread GL use | `code/lib/window_manager_win32.cpp` |
| comment in `App::UseChristmasMode` | "Also allow user to disable if he wishes" | the `ChristmasEnabled` preference is not consulted | `code/app.cpp:App::UseChristmasMode`, `code/interface/prefs_other_window.cpp` |
| graphics menu (`dialog_landscapedetail`, `dialog_pixeleffect` strings) | landscape detail and pixel effect are user options | both controls are commented out; `RenderLandscapeDetail` has no effect | `code/interface/prefs_graphics_window.cpp` |
| `Renderer::Initialise` reads `WaitVerticalRetrace` | vsync is configurable | first two `CreateWin` attempts pass literal `true`; the GL window manager ignores the argument entirely | `code/renderer.cpp:Renderer::Initialise`, `code/lib/window_manager_win32.cpp:CreateWin` |

<!-- from 25_textures_and_assets.md -->
### From 25_textures_and_assets.md

| Doc / comment | Says | Code / data does | Ref |
|---|---|---|---|
| `docs/OpenGLState.txt` | MAG/MIN filters "undefined" in the baseline texture state | every texture object is created with MAG LINEAR / MIN LINEAR_MIPMAP_LINEAR and CLAMP wrap | `code/lib/bitmap.cpp:BitmapRGBA::ConvertToTexture` |
| `docs/RussianFontRequest.txt` | only Speccy has Cyrillic glyphs; Speccy was copied to Square and Editor | `data/textures/unicode/` holds distinct Cyrillic atlases for all three fonts, but `tools/language.list` still ships only the Speccy ones | §2.15 |
| `docs/landscape lightmap.txt` | `Landscape::ApplyLightmap` / `GenerateLightMap` with `m_lightMapTextureId` | the lightmap is the registered bitmap `water_lightmap` built by `Water::GenerateLightMap`; `Landscape` only triggers it | `code/water.cpp:Water::GenerateLightMap` |
| `code/lib/bitmap.h:BitmapRGBA::ConvertColourToAlpha` comment | luminance → alpha | green channel → alpha, RGB := white | `code/lib/bitmap.cpp:BitmapRGBA::ConvertColourToAlpha` |
| method name `ApplyDilateFilter` | dilate | 8-neighbour mean (box blur without centre) | `code/lib/bitmap.cpp:BitmapRGBA::ApplyDilateFilter` |
| `tools/FindUnusedResources.sh` | lists unused resources | misses 380 used visual files and counts 21 unused ones as used (§6.4) | `tools/FindUnusedResources.sh` rules emulated over this tree (§6.4) |
| `tools/ShapeExporter` | "Futurewar Shape Format v1.0" exporter | no longer compiles against `code/lib/shape.h`; writes triangle lists only, while 39 shipped files are strips | `tools/ShapeExporter/ShapeExporter.cpp`; shape survey (§1.5) |
| `code/lib/shape.cpp:ShapeFragment::WriteToFile` | writes a loadable `.shp` | broken `%5.2`/`%7.3`/`%6.3` formats, colours not un-scaled | `code/lib/shape.cpp:ShapeFragment::WriteToFile` |
| `code/sound/sample_cache.cpp:CachedSampleManager::CleanUp` comment | "Largest song takes 13 MB" | the largest track decodes to 30 389 760 bytes, but no music ships; the largest live file is `data/sounds/evil.ogg` (53.9 s ≈ 4.75 MB decoded) | §5 |
| `tools/unused_samples.sh`, `tools/unused_sample_groups.sh` | "run from the Darwinia data directory" | ignore `data/mwsounds/` and orphan-group membership; CRLF breaks the comparison on POSIX shells | `tools/unused_samples.sh`, `tools/unused_sample_groups.sh` |

<!-- from 26_animation_and_vfx.md -->
### From 26_animation_and_vfx.md

| Doc / comment | Claim | Code | Ref |
|---|---|---|---|
| Original specification §5 for this document | "effect definitions (`effects.txt`)" under animation/VFX | `effects.txt` holds audio DSP filter blueprints; no VFX reads it | `code/sound/soundsystem.cpp:SoundSystem::LoadEffects` |
| `data/effects.txt` header comment | "Default: the starting value" | defaults are never applied at runtime; editor seeds the minimum | `code/sound/soundsystem.cpp:DspBlueprint::GetParameter`, `code/interface/soundeditor_window.cpp:DspEffectButton::MouseUp` |
| `code/particle_system.h:ParticleType` comments | `m_spin` "rate of spin in radians per second", fade-in/out times | never read; fade is hard-coded 75 %/25 % and particles never spin | `code/particle_system.cpp` |
| `code/gesture_demo.h:GestureDemo::GetCoords` comment | "mouse pos scaled for the current screen resolution" | scales both axes by screen height; interpolation weights swapped | [B:DEAD], Appendix |
| `code/worldobject/entity_leg.h` comment | `m_legLift` "in metres" | world units (the game has no metre scale) | `EntityLeg` |
| `code/worldobject/entity_leg.cpp:EntityLeg::CalcKneePos` FIXME | "cosTheta should never be greater than one, yet sometimes it is" | clamped to 1 — the IK is approximate (§5.1 step 7) | — |
| `docs/game objects.txt` | power-ups "triggered by a gesture of some kind" | Multiwinia power-ups are placed by click; gestures are SP-only | P09 |
| `docs/donelist.txt` | "made it possible to add new gestures from editor" | gesture editor compiled out (`GESTURE_EDITOR` debug-only, window under `USE_SEPULVEDA_HELP_TUTORIAL`) | `code/interface/gesture_window.cpp` |

<!-- from 27_audio_engine.md -->
### From 27_audio_engine.md

| Doc / comment | Says | Code / data does | Ref |
|---|---|---|---|
| `docs/adsr.gif` | Classic ADSR: Attack → **Decay** → Sustain **level** → Release on key-off | No decay stage (`StateDecay` never entered); SUSTAIN is a hold **duration**; peak = sustain level = VOLUME | `code/sound/sound_instance.cpp:SoundInstance::UpdateChannelVolume` |
| `docs/ThoughtsOnSoundSystem.txt` | 2D buffer fixed at 2000 samples "probably needs to be adjustable"; wants a linear/log toggle on the parameter graph; "Sort out SetChannelVolume / SetChannel3dMode for software mixer" | Buffer is pref `SoundBufferSize` (512 generated / 2000 fallback); the graph draws output on a √ scale but the toggle button is commented out; both software-mixer functions are implemented | `code/sound/sound_library_2d_win32.cpp`; `code/interface/soundparameter_window.cpp:SoundParameterGraph::GetPosition`; `code/sound/sound_library_3d_software.cpp` |
| `docs/ImplementationNotes/SoundSystemSamplePath.jpg` | Ogg **and WAV** files reach the sample cache from both the file system and RAR archives | `Resource::GetSoundStreamDecoder` only appends `.ogg`; WAV decoding is unreachable | `code/lib/resource.cpp:Resource::GetSoundStreamDecoder` |
| `docs/ImplementationNotes/SoundSystemData.jpg` | Sound params include min **and max** distance, pan (non-3D), loop points; properties include angular velocity, health, time; source is a WAV file | No max distance, pan or loop points; link inputs are only Nothing, HeightAboveGround, X/Y/Z position, Velocity, CameraDistance; assets are Ogg | `code/sound/sound_parameter.h`, `code/sound/sound_instance.h` |
| `docs/ImplementationNotes/SoundSystemData2.jpg` | `SoundFilterBlueprint` array indexed 0..12 (13 filters); class names `SoundFilterBlueprint` / `SoundFilterParamBlueprint`, `SoundChannel` | 14 filters (`NUM_FILTERS`, 0..13); classes `DspBlueprint` / `DspParameterBlueprint`; channel table is `SoundInstanceId[]` | `code/sound/sound_library_3d.h`, `code/sound/soundsystem.h` |
| `docs/ImplementationNotes/Sounds_DiskToSpeaker.gif` | One `SoundSystem::SoundLibraryCallback()`; instance holds `SoundFilterHandle *m_filters` | Two callbacks (`SoundLibraryMainCallback`, `SoundLibraryMusicCallback`); filters are `DspHandle`s; otherwise the CachedSample → handle → instance → SoundSystem → DirectSound chain matches | `code/sound/soundsystem.h` |
| `docs/ImplementationNotes/SoundEffectBlueprint2.jpg` | Linked parameter graph flat outside the input range, linear between | Consistent | `code/sound/sound_parameter.cpp:SoundParameter::Recalculate` |
| `docs/MinDistAndVolume.jpg` | Flat to min distance, then fall-off | Consistent (inverse distance) | `code/sound/sound_instance.cpp:SoundInstance::CalculatePerceivedVolume` |
| "sound changes.txt" in `docs/` (Requested section) | Microphone positioning; "drop all volumes to highlight a single sound event" | Not implemented (listener = camera; no ducking or buses) | `code/sound/soundsystem.cpp:SoundSystem::Advance` |
| `code/sound/sample_cache.cpp` comment | "Largest song takes 13 MB" | Largest shipped track decodes to 29.0 MiB (`data/sounds/tresk_-_excuses.ogg`, 344.6 s mono 44.1 kHz) and its whole buffer is allocated on open | `code/sound/sample_cache.cpp:CachedSampleManager::CleanUp` |
| `code/sound/sound_instance.h` comment | VOLUME 0.0 = "silence" | 0.0 = −50 dB on DirectSound (−43.4 dB software); only master 0 + volume 0 approaches the −100 dB floor | `code/sound/sound_library_3d_dsound.cpp:SoundLibrary3dDirectSound::SetChannelVolume` |
| `code/sound/sound_library_3d.h` header comment | "Currently two implementations" | Three plus a null device: DirectSound, software, XAudio (dead on PC), NoSound | `code/sound/` |
| `code/sound/soundsystem.h` comment on `StopAllSounds` | "Full event name required" | Substring match (`strstr`) | `code/sound/soundsystem.cpp:SoundSystem::StopAllSounds` |
| `code/sound/sound_library_3d_software.cpp` comment on `m_forceVolumeJump` | "prevents the volume ramping code from being used when a new sound first starts" | Ramping happens **only** when the flag is set (new sound) | `SoundLibrary3dSoftware::Callback` |
| `contrib/systemiv/lib/math/random_number.h` comment | `frand`: 0 ≤ result < range | Can return exactly `range` (`AppRandom` max 32767 / 32767) | `contrib/systemiv/lib/math/random_number.cpp:frand` |
| `code/interface/debugmenu.cpp` captions | "Sound Stats (F7)", "Sound Editor (F8)", "Sound Profile (F9)" (button text "Sound Profile (F8)") | Hot-keys commented out; buttons never created (`modsEnabled` false) | `code/user_input.cpp:UserInput::Advance` |

<!-- from 28_ui_framework.md -->
### From 28_ui_framework.md

| Doc | Says | Code | Ref |
|---|---|---|---|
| `docs/multiwiniachanges-beta.txt` (Issue 650, listed as fixed) | "Game Menu: X360 B button in menus doesn't always go back" | Structural cases remain where B does nothing: pages without a `BackPageButton` (Main, AuthKeyEntry), any frame in which the `BackPageButton` is not rendered (drop-down list open, window unfocused), and `MessageDialog` (no back button) [C mechanism; I that these were the reported cases] | `code/game_menu.cpp:BackPageButton::Render`, `code/interface/message_dialog.cpp` |
| `docs/multiwiniachanges-beta.txt` (Issue 558, listed as fixed) | "Menu navigation in the pre-game with controller" | consistent: the lobby builds `m_buttonOrder` and starts the cursor on Play | `GameMenuWindow::SetupMultiplayerPage` |
| `docs/detailed schedule.txt` "Translation System → Missed out" (Sound Stats/Parameter/Editor/Profile windows, Profile Window, Pokey Window, Network Window, Input Logger, Grabber Window, Gesture Window, Debug Menu, Cheat Window, Camera Anim Window) | these windows are untranslated | consistent: literal English captions (§10.3) | [C] grep of the window captions (§10.3) |
| `docs/RussianFontRequest.txt` | only speccy had Cyrillic glyphs; they were copied to square and editor as a stop-gap | consistent: the three Cyrillic atlas pairs are byte-identical | `data/textures/unicode/*.0410.042f.bmp`, `*.0430.044f.bmp` |
| `docs/sound changes.txt` | "Sound editor will now use a scrollbar if there are too many events to fit" | consistent (`SoundEditorWindow` creates a `ScrollBar`), but the sound editor is [B:DEBUG] only | `code/interface/soundeditor_window.cpp` |

<!-- from 29_data_formats_reference.md -->
### From 29_data_formats_reference.md

| Doc / comment | Says | Code does | Ref |
|---|---|---|---|
| `docs/input_preferences_syntax.txt` | grammar without `~` icon lines, `value`/`alias` drivers, `repeat`, `key any`/`mouse any`, thumbstick directions, `lefttriggerb`/`righttriggerb` | all of these are accepted (§5.2) | `code/lib/input/input.cpp:InputManager::parseInputPrefs`, `code/lib/input/inputdriver_xinput.cpp` |
| `docs/input_preferences_syntax.txt` | "A preference value is always false if there is no corresponding preference manager entry" | `pref X` with a non-existent key **fails to parse** (binding rejected) | `code/lib/input/inputdriver_prefs.cpp:PrefsInputDriver::parseInputSpecification` |
| `docs/input_preferences_syntax.txt` | user prefs override the FIRST binding of the system file | true for each user line, but several user lines for one control overwrite each other (replace semantics) | `code/lib/input/control_bindings.cpp:ControlBindings::bind` |
| `docs/top level map.txt` | each level file specifies a map file at the start; global data has an "X Z position" per map | map/mission pairing comes from `game.txt` `Locations` rows; `locations.txt` stores X Y Z | `code/global_world.cpp:GlobalWorld::ParseLocations`, `LoadLocations` |
| `docs/changes.txt` (Darwinia Steam notes) | default language read from `HKCU\software\valve\steam\language` | `GetLocaleInfo(LOCALE_SENGLANGUAGE)`; no registry read | `code/app.cpp:App::InitLanguage` |
| `docs/changes.txt` (Darwinia) | mods in `mods/<name>/`; "stats.txt reloaded when a mod is selected" | mod look-up compiled out (`USE_DARWINIA_MOD_SYSTEM` undefined) | `code/lib/resource.cpp:Resource::GetTextReader`, `GetBinaryReader` |
| `data/multiwinia.txt` header | `Change: 0 = DropDownMenu, -1 = CheckBox` | consistent | `code/UI/GameMenuWindow.cpp` |
| `data/language/english.txt` comment | `*BETATESTERS` is replaced by all beta-tester names | consistent; the phrase must be exactly `*BETATESTERS` | `code/UI/GameMenuWindow.cpp:ReadCredits` |
| `data/effects.txt` header | "Default: the starting value" | defaults are not applied to parameters omitted in `sounds.txt` (they are fixed 0); the editor seeds with the minimum | `code/sound/soundsystem.cpp:SoundSystem::ParseSoundEffect`, `code/interface/soundeditor_window.cpp` |
| `data/stats.txt` header | `SPED` in m/s, `RATE` "secs to reload", `NAME` = creature name | rows bound by position; speed × (1 + difficulty/10); instance copies truncated to `unsigned char` | `code/worldobject/entity.cpp:EntityBlueprint::Initialise`, `GetStat`, `Entity::SetType` |
| `code/lib/bitmap.h` comment on `ConvertColourToAlpha` | luminance → alpha | **green channel** → alpha, RGB := white | `code/lib/bitmap.cpp:BitmapRGBA::ConvertColourToAlpha` |
| `tools/ShapeExporter` description | "Futurewar Shape Format v1.0" (the exporter writes loadable files) | the exporter no longer compiles against `code/lib/shape.h`; shipped strip-encoded shapes came from an older exporter | `tools/ShapeExporter/ShapeExporter.cpp:ShapeExporter::BuildShapeFragment` |
| `code/lib/shape.cpp:ShapeFragment::WriteToFile` | writes a loadable `.shp` | broken `%5.2`/`%7.3`/`%6.3` formats; colours not un-scaled | `ShapeFragment::WriteToFile`, `ShapeMarker::WriteToFile` |
| `code/gesture_demo.h:GestureDemo::GetCoords` comment | returns a mouse position scaled for the current screen resolution | scales both axes by screen **height**; interpolation weights swapped | `code/gesture_demo.cpp:GestureDemo::GetCoords` |
| `code/level_file.cpp:LevelFile::ParseLights` comment | mission lights override map lights | `ParseMissionFile` rejects `Lights` blocks; only a second `Lights` block in one map file is ignored | `LevelFile::ParseLights`, `LevelFile::ParseMissionFile` |
| `code/sound/sample_cache.cpp` comment | "Largest song takes 13 MB" | the largest shipped track decodes to 29.0 MiB | `CachedSample::CachedSample` |
| `code/team.h:TeamControls` comment | update `NetworkUpdate::GetByteStream` when adding flags | that function no longer exists; packing is `TeamControls::Pack` | `code/team.cpp:TeamControls::Pack` |
| `MultiwiniaSupport.ini`, installers | user files `userprefs.txt`, `authkey` in the install directory | the game writes `user_input_preferences.txt`, reads `authkey.dev`, under `<P>` | `code/lib/input/file_paths.cpp`, `code/app.cpp:App::InitMetaServer`, `App::GetProfileDirectory` |

---

<!-- from 30_scripting_tutorial_and_cutscenes.md -->
### From 30_scripting_tutorial_and_cutscenes.md

| Doc | Claim | Code | Ref |
|---|---|---|---|
| `docs/help system.txt` | A Sepulveda context-help design with F1 to request help, "fixed messages" (BuildingCaptured, BuildingOnline, NewGestureAvailable, objective complete…), "failure help" (SquadCreationOutOfBounds…) and action items GestureBasics, ThrowAirstrike, FireRocket, Persuadatron, EngineerBuildBridge, EngineerHarvestSpirits, EngineerCaptureBuilding, AimRadarDish, PopulateWithDarwinians | Key is `SepulvedaHelp = key h`; the enum has the 20 items listed in [09](09_meta_systems.md); no fixed-message or failure-help classes exist; the whole system is compiled out of Multiwinia | [C] `code/helpsystem.h`; `data/input/input_preferences.txt` L255 |
| `docs/changes.txt` L82–84 | "Show First Time Intro" makes the intro play "the next time you start Darwinia" | The intro runs only from `RunBootloader` when the Campaign is launched (and only with `USE_LOADERS`); the option row is not built in the Multiwinia options UI | [C] `code/main.cpp:RunBootloader`; `code/interface/prefs_other_window.cpp` (see [09](09_meta_systems.md)) |
| `docs/detailed schedule.txt` (CUTSCENES) | "Intro cutscene should always play when a new profile is created" | The trigger is the `BootLoader = firsttime` preference written for a fresh preferences file, not per profile; never reachable in Multiwinia | [C] `code/lib/preferences.cpp` (L377); `RunBootloader` |
| `docs/detailed schedule.txt` L19 | "(OPTIONAL) Intro bootloaders" planned | Implemented, then compiled out (`//#define USE_LOADERS`) | [C] `code/lib/universal_include.h` |
| `docs/multiwinia-changes.txt` L53, `docs/multiwiniachanges-beta.txt` L16 | "Power-ups in the Advanced Tutorial will now terminate correctly upon use" | Consistent only after Tutorial 2's End step: `TaskManager::TerminateTask` is refused while `!m_tutorial2AICanSpawn`, and armour can never be stopped in a tutorial | [C] `code/taskmanager.cpp:TaskManager::TerminateTask`, `Task::Stop` |
| `docs/multiwiniachanges-beta.txt` L111, L330; `docs/multiwiniareleasenotes-beta.txt` L87, L346 | A "quick tutorial" and an "Advanced Concepts tutorial" | Match Tutorial 1 and Tutorial 2 — no discrepancy | [C] §3 |
| `docs/betachanges.txt` L34 | "The final Fence Switch has been made Once Only, preventing constant cutscene triggering" | Darwinia content: `FenceSwitch` (trigger T2) is a blocked building in Multiwinia; the `lockable` field implements "once only" | [C] `code/worldobject/switch.cpp`; `code/worldobject/building.cpp:Building::BuildingIsBlocked` |
| `docs/possible removals.txt` | "Cutscene 5" listed as a possible removal | `music5.txt` and the `Cutscene5` music event still exist (Darwinia campaign) | [C] `data/scripts/music5.txt`; `data/sounds.txt` |
| `contrib/systemiv/lib/math/random_number.h` comment | `sfrand(range)` returns −range < x < range | Implementation returns `(0.5 − U)·range`, i.e. ±range/2 (affects the demo-end sprite spreads in §5.3) | [C] `contrib/systemiv/lib/math/random_number.cpp:sfrand` |
| `docs/multiwiniareleasenotes-beta.txt` L35 | The beta records benchmarks and uploads them by FTP "when you exit" | Compiled out in every target here; when enabled, upload happens at the end of each match | [C] `code/benchmark.cpp`; `code/main.cpp:EnterLocation` (see [32](32_debug_test_and_performance.md)) |

<!-- from 31_platform_build_and_tools.md -->
### From 31_platform_build_and_tools.md

| Doc / artefact | Claim | Code reality |
|---|---|---|
| `README.TXT` | "There are four possible build configurations" — then lists six, including "PC OpenGL (Release) produces darwinia_gl.exe" and "PC OpenGL (ReleaseDirect3D) produces darwinia.exe" | the vcproj has four configurations; the solution adds `PC OpenGL (Debug)`/`(Release)` solution configurations mapped to `Debug`/`Release`; no "PC OpenGL (ReleaseDirect3D)" exists [C] `targets/vs2005/Darwinia.sln` |
| `README.TXT` | "Debug and Release build Darwinia…" | the product built is Multiwinia (`APP_NAME`) |
| `tools/readme-linux.txt` | preferences in `~/.darwinia/demo` or `~/.darwinia/full` | Multiwinia Linux path is `~/.multiwinia/<DARWINIA_GAMETYPE>/` [C] `code/app.cpp:App::GetProfileDirectory` |
| Installers' `[UninstallDelete]`, `MultiwiniaSupport.ini` | user files live in `{app}` | written to `%LOCALAPPDATA%\Introversion\Multiwinia\` on Windows 2000+ unless `--NoProfile` [C] |
| Installers, `MultiwiniaSupport.ini` | files `userprefs.txt`, `game-history.txt`, `stdout.txt`; key file `authkey` | no code in `code/` or `contrib/systemiv/lib` writes the first three; the game reads `authkey.dev` [C grep] |
| Support-tool success message | "This reference is also recorded in errorlog.txt in your Multiwinia directory" | `ERRORLOG.TXT` is written to the profile directory, not the install directory [C] |
| `docs/vista installer check list.txt` | `TARGET_VISTA`, `ReleaseD3D`, `DarwiniaSupport.ini/.exe` | `TARGET_OS_VISTA`, `ReleaseDirect3D`, `MultiwiniaSupport.*` |
| `docs/RussianFontRequest.txt` | only Speccy has Cyrillic glyphs | `data/textures/unicode/` has Cyrillic pages for `editor`, `speccy` and `square`; but the all-languages `tools/language.list` packs only the `speccy` ones (§12.2) |
| `tools/language/README.txt` | translation inputs must be UCS-2 BE | `data/language/japanese.txt` and `russian.txt` are UTF-16 LE; the game accepts both, `report.pl` only BE |
| `tools/FindUnusedResources.sh` | must run from a folder named `Darwinia` | repository folder is `multiwinia` |
| `docs/cpu cycles.txt` | (requested by the specification as performance notes) | a gameplay design note (task manager/gestures); no performance data |
| `docs/Network.txt` | server ticks at 25 FPS | 10 Hz in code ([22](22_determinism_and_networking.md)) |
| `targets/vs2005/GDF/*.gdf.xml` (compiled into the exe) | "Darwinia" 1.5.4.0, `darwinia.exe`, saved games `Darwinia\users` | the product is Multiwinia / `multiwinia.exe`; Game Explorer metadata is stale |
| `targets/vs2005/version.rc` vs installers vs `DARWINIA_VERSION` | 6.0.0.0 vs 1.0.x–1.9.x vs "dev" | no single version source (§9.4) |
| `code/lib/window_manager_win32.cpp` comment | secondary GL context "for loading thread" | never made current; worker GL work is marshalled to the main thread |
| `code/app.cpp` comments | affinity `0x02|0x8|0x20|0x80` "Even processors", `0x01|0x4|0x10|0x40` "Processor Number % 3 == 2" / "Odd processors" | the masks select CPUs 1,3,5,7 and 0,2,4,6 (0-based) respectively |
| `tools/build_multiwinia_data.bat` echoes | sounds2 step prints "Making sounds.dat"; `ambx` platform prints "…hwcompat.txt" | builds `sounds2.dat`; appends the beta-test exclusion list |
| `targets/vs2005/Common.vsprops` | include path `steam/` under `contrib/` | directory absent from the tree |

<!-- from 32_debug_test_and_performance.md -->
### From 32_debug_test_and_performance.md

| Document | Claim | Code / data | Ref |
|---|---|---|---|
| `docs/cpu cycles.txt` | named by the specification as the performance cross-check | a gameplay design note (CPU-cycle program economy, task switching, Debug Zeppelins); no performance data; none of the mechanics exist in Multiwinia | §12.1 |
| `docs/multiwiniareleasenotes-beta.txt` | the beta records benchmarks and uploads them to the FTP server "when you exit" | recorder compiled out in every target here (`//#define BENCHMARK_AND_FTP`); when enabled the upload runs at the **end of each match**, after `LocationGameLoop`; the `benchmarks` folder name matches | `code/main.cpp:EnterLocation`, `code/benchmark.cpp:BenchMark::Upload` |
| `docs/multiwiniareleasenotes-beta.txt` | "debugging code is enabled to track rand sync errors" | `TRACK_SYNC_RAND` is commented out for `TARGET_BETATEST_GROUP_ALL`; beta groups only get snapshot IFrames (`TARGET_BETATEST_GROUP`) | `code/lib/universal_include.h`, `code/network/iframe.cpp` |
| `docs/Network.txt` | the network "Ticks at 25 FPS" and "Runs heavy weight physics at 25 fps" | server tick 10 Hz (`+= 0.1` s); 10 slices per tick | `code/network/server.cpp:Server::AdvanceIfNecessary`, `code/globals.h` |
| `docs/NetworkThreads.txt` | dedicated send threads that sleep when queues are empty | sending is synchronous on the main thread | [22](22_determinism_and_networking.md) |
| `docs/multiwinia_todo.txt` | "Debug tools: Profile for network system, Bandwidth send / received, latency etc, Server logging" | partly implemented: bandwidth/latency in the F9 window, server sections in the CPU profiler, server events in `debug.txt`; no dedicated network profile | §5.1, §9.5 |
| `docs/detailed schedule.txt` | debug windows (Sound Stats/Parameter/Editor/Profile, Profile, Pokey, Network, Input Logger, Grabber, Gesture, Debug Menu, Cheat Window, Camera Anim) "missed out" of translation | consistent: all use literal English captions; the Input Logger has no implementation (`InputLogButton` undefined) | §3.2 |
| Debug-menu captions | "Profile (F6)", "Display FPS (F5)", "Dbg Cam (F2)", "Toggle Editor (F3)", "Cheat Menu (F4)", "Sound Stats (F7)", "Sound Editor (F8)", "Sound Profile (F9)" | none of these keys is bound to these actions (F1–F8 are task-manager keys; F9 = network stats, F11 = cheat menu) | `data/input/input_preferences.txt` |
| Code comment in `Server::AdvanceIfNecessary` | testbed server "Advance time 10 times faster than normal mode" | every branch adds 0.1 s — no speed-up | §6.7 |
| `tools/build_multiwinia_hwcompat.bat` | copies `ncftpput.exe` (the benchmark uploader) into the hwcompat build | `TARGET_PC_HARDWARECOMPAT` does not define `BENCHMARK_AND_FTP` in this tree, and `ncftpput.exe` is absent from the tree | 32-U3 |
| `docs/changes.txt` (Darwinia) | `RenderLandscapeMode` default "set to 2 (Vertex Buffer Objects)" | default −1 = automatic: VBO when the extension is supported, else display lists (same effect, different stored value) | `code/landscape_renderer.cpp:LandscapeRenderer::LandscapeRenderer` |
