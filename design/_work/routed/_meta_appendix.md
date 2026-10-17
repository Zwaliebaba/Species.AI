# Collected "appendix" sections from all drafts

<!-- from P01_app_core -->
## Appendix — dead/legacy code

- **[B:DEAD] `TESTBED_ENABLED`**: `TESTBED_MODE {ON, OFF}`, `TESTBED_TYPE {CLIENT, SERVER}`, `TESTBED_STATE {MAINMENU, HOST_GAME, JOIN_GAME, PICK_SERVER, WAIT_FOR_CLIENT_ID, SELECT_GAME, SELECT_MAP, SETGAMEOPTIONSDELAY, SETGAMEOPTIONS, WAITFORGAMEOPTIONS, WAIT_FOR_MAP, WAIT_FOR_CLIENTS, PLAY, IDLE, GAME_OVER_DELAY, MAKE_CLIENT_SPECTATOR, SEQUENCEID_DELAY, BUILD_GAME_ENTRIES}`; 24 App fields and accessors; `TestBedLogWrite` appends CSV `TestBedLog.txt` with Date, NumSyncErrors, GameMode, GameMap, GameIndex, ClientRandomSeed, LastGameIndex, GameTimer, SyncSeqId; `LocationInput` auto-ready, sync-error and termination-sequence handling that returns to the main menu; `ControlToggleRendering`; `Location::Render` skip; `GRACE_TIME` 20 s. [C] `code/app.h`, `code/app.cpp`, `code/location_input.cpp`.
- **[B:DEAD] `USE_SEPULVEDA_HELP_TUTORIAL`**: HelpSystem/Tutorial/Sepulveda/DemoEndSequence members and all `PlayerDoneAction` calls.
- **[B:EDITOR] `LOCATION_EDITOR`**: `LocationEditorLoop` (input/camera/editor advance + render until Esc; re-registers a `.mwm` map with `GameMenu::RemoveMap/AddMap`; tears down location like `ShutdownCurrentGame` without network) and `LocationEditor` creation in `EnterLocation`.
- **[B:LEGACY] `GlobalWorldEditorLoop`**: compiled, but returns immediately (`if( true ) { m_editing = false; return; }`).
- **[B:LEGACY] Darwinia campaign plumbing in FULL**: `GameModePrologue/Campaign`, `IsSinglePlayer()` branches, `LoadProfile/SaveProfile/ResetLevel`, `game.txt`, pref `StartMap`, `MissionComplete/DoMissionCompleteActions`, single-player `IsFriend` table, `RunningProgram`s, quick-run programs, single-player officer follow/none and armour deployment, Engineer bridge, `GlobalWorld::TransferSpirits`, `EnterGlobalWorld` camera (`ModeSphereWorld`, height 50), `UserInput` global-world sphere picking.
- **[B:DEAD] `USE_LOADERS`**: `RunBootloader` (pref `BootLoader` "firsttime" → `StartSequence`, else `Loader::CreateLoader`), camera (1000,500,1000).
- **[B:DEAD] `TARGET_OS_VISTA`**: `DoVistaChecks` (OS ≥ 6 workstation, Parental Controls `IsBlocked` with the GUID), `RICH_GAME_MEDIA_HEADER` (`RM_MAGICNUMBER 'HMGR'`, `RM_MAXLENGTH 1024`, thumbnail 256×192 PNG, `.dsg` file), Media Center relaunch of `ehshell.exe`, `g_saveFile`, `g_mediaCenter`.
- **[B:DEAD]** `BENCHMARK_AND_FTP` (BenchMark Begin/End/Upload, DXDiag), `ATTRACTMODE_ENABLED`, `TEST_HARNESS_ENABLED` (pref `TestHarness`), `USE_DARWINIA_MOD_SYSTEM` (pref `Mod`, `ModSystemEnabled`), `DEMOBUILD`/`DEMO2` (`game_demo*.txt`, open website and exit after a location), `CHRISTMAS_DEMO`, `GIVE_ALL_LEVELS`, `FOG_LOGGING`/`FOG_PREFERENCE` (pref `RenderFog`) — none defined, `JAMES_FIX`, `NETWORKQUALITY_TESTING`.
- **[B:DEBUG]** `SOUND_EDITOR` (`EffectProcessor`), `CHEATMENU_ENABLED` (F11 cheat menu, special-lighting toggles), `TRACK_SYNC_RAND` (`SyncRandLog`, `WatchEntities`, `TrackEntity`, `UntrackEntity`, entity/effect/building state logging around every Advance).
- **[B:DEAD] macOS/Linux**: SDL event handler/drivers, profile paths `~/Library/Application Support/<APP>/<gametype>/`, `~/.<app>/<gametype>/`, x87 control-word asm, sound loading on the loading queue, `Finalise` queue draining.
- **Inert**: `#pragma omp parallel for` in `Location::Advance` (OpenMP off); `GameTimer::DebugRender` (returns first); `Location::RenderParticles` (empty); `Location::WaterReflect` (loop with commented body); `UserInput::m_mousePosHistory`; `App::DoStartServer/DoCheckForPDLC` (no definitions); `m_steamInited`; `TARGET_FRAME_RATE_INCREMENT`; `MINIMUM_RENDER_PERIOD`.

<!-- from P02_game_modes -->
## Appendix — dead/legacy code

- `Multiwinia::AdvanceChess` (score backup only) and `AdvanceShaman` (scores 200, portal scoring commented) — never dispatched [C].
- Commented-out older `AdvanceBlitkrieg` (win by locking all zones, squad-drop retribution at a random high flat spot, BlitzMaster within 300 s) [C] `code/multiwinia.cpp` L1872–2088.
- Commented-out KOTH random zone spawner using non-existent options `NumberOfZones`, `ZoneSize`, `ZoneMoveFrequency` (and `MultiwiniaZone::m_life`) [C].
- Commented-out "retribution: spawn 25–34 Darwinians at a random spot" block [C].
- `Multiwinia::GameOver()` tail after the first `return` (per-mode end checks, 5 s victory timer, rocket check, `m_timeRemaining == 0`) [C]; `m_rocketsFound` only used there.
- Commented-out Assault objectives HUD (`multiwinia_assault_obj*` strings) [C] `Render`.
- TankBattle mode [B:DEBUG] (`INCLUDEGAMEMODE_TANKBATTLE`).
- `data/multiwinia-testbed.txt` [B:DEAD] (`TESTBED_ENABLED`); testbed auto-ready path in `location_input.cpp`.
- `MULTIWINIA_DEMOONLY` bodies-removed variant [B:DEMO-only target].
- `MultiwiniaZone::m_aiTarget` priority code (never assigned).
- `CheckMentorAchievement` stub; `App::CheckMasterAchievement`/`EarnedAchievement` stubs; `AchievementTracker::Load/Save` stubs; `ACHIEVEMENT_RECORD_FILENAME` unused; `m_teamsStatus` write-only; `BLITZKRIEGSCORE_UNDERATTACK` never assigned; `m_numStandardLinks` unused; `PulseBomb::m_active` unused.
- Darwinia branches in `rocket.cpp` [B:LEGACY]: non-MP `Refuel` by pipe count (0–1 pipes → towards 50 at `level×0.005/s`; 2 → towards 100 at 0.01; 3 → 0.1), `SafeToLaunch` (< 2 enemies of team 0 within 200), spectacle (Darwinians watch/cast shadows), Ignition 18 s, Ready 120 s with `launchpad_victory.txt`, scripted attacker spawns by hard-coded building ids 18/51/52/54, SP attacker selection (20%/tick, range 350), `ProvideFuel` ×100 cheat under `CHEATMENU_ENABLED` [B:DEBUG], FuelGenerator GlobalBuilding online at > 8 surges.
- `TrunkPort` destination labels, `m_targetLocationId`, `ReprogramComplete`, open-timer power-up [B:LEGACY].
- `code/worldobject/chess.cpp` reads options `TotalSize`/`SizeVariance` that no blueprint defines (ChessBase/ChessPiece are blocked old objects) [B:LEGACY].

<!-- from P03_entity_core -->
## Appendix — dead/legacy code
- **[B:DEAD]** `Darwinian::AdvanceJumpPadLaunch` (commented out); `StateUsingJumpPad` (commented in enum); Sepulveda hooks under `USE_SEPULVEDA_HELP_TUTORIAL` (`Officer::SetOrders`, `taskmanager.cpp`); `TRACK_SYNC_RAND` tracking (`Entity` ctor/dtor/`NewEntity`, debug only); `DEBUG_ENTITY_GRID` error visualiser; `EntityGrid::Render` / `ObstructionGrid::Render` / `EntityGridCache::Render` are debug renders (`EntityGridCache::Render` call commented out).
- **[B:LEGACY]** (compiled, unreachable or SP-only in Multiwinia): `StateWorshipSpirit` + `BoxKite`, `SearchForSpirits`, `StateCapturedByAnt` (ArmyAnt blocked), `StateFollowingShaman`/`SearchForShamans`, `StateBeingSacraficed`/`Sacrafice`/`OrbitPortal`/Zombie (Shaman blocked), `StateWatchingGodDish`, GodDish spectacle, `BeginVictoryDance`, SP red-team (team 1) special cases ("Red Darwinians don't respond to officers/armour/spirits", always soldier+grenades), SP team-2 grenade safety check, `OfficerOrders` (SP order effect), Officer `SetNextMode/SetPreviousMode` research-level cycling and absorb/shield mechanics (research level 4 only; MP fixes level 3), `Task::Promote`, `EntityLeg` (Tripod only), `Spirit` egg/store/attached/shaman states, `Spirit::AddToGlobalWorld`, `Location::IsFriend` SP friendship table, `Entity::PushFromEachOther` (only LaserTrooper uses it), `Entity::PushFromCliffs` (no live caller), `Officer::SearchForRandomPosition` (no caller), `Darwinian::AdvanceIdle` MP idle search (dead by timer reset), `m_avoidObstruction` (never set).
- `EntityBlueprint` difficulty scaling (`m_difficultyLevel` forced to 0 in Multiwinia unless a level stores one).

<!-- from P04_entities_military -->
## Appendix — dead/legacy code
- [B:LEGACY] `ControllerGrenade` and squad Rocket/AirStrike/Controller secondaries (§1.5, §4.5): no Multiwinia path selects them.
- [B:LEGACY] `LaserTrooper` (§8): only spawned by blocked `Lander`; old `LaserTrooper::Advance` fully commented out (formation, wobble, AI obey timer).
- [B:LEGACY] `Shaman` (§9): `Advance` returns true immediately; `BeginSummoning`, `GrabSouls`, `CanSummonEntity` bodies commented out; `GameTypeShaman` start-up commented out; `Shaman Interface` reachable only with a live Shaman. `Nuke::RenderDeaths` call commented out.
- [B:DEBUG]/unreachable `Tank` (§6): no FULL spawner; `Tank::DropMine`, `LaunchMissile`, `SetMissileTarget` have no callers in the tree (so `LandMine` and `Missile` are never created); passenger (`TANK_CAPACITY`) code and `SetAttackTarget` toggle block commented out.
- [B:LEGACY] Armour single-player paths: damage clamp to −1, `m_awaitingDeployment` → `ConvertToGunTurret`, single-player `ToggleLoading` cycle, commented-out flag/banner rendering and missile launch, commented-out non-Multiplayer conversion block in `Armour::Advance`.
- [B:LEGACY] Engineer bridge building (`BeginBridge` research ≥ 5), single-player marker registration for control towers / research items (`s_buildingIds`, `s_researchIds`).
- [B:LEGACY] Research tables for levels other than the Multiwinia overrides (laser/rocket/grenade/armour/harvester/engineer/squad).
- Commented code: `Laser` old per-type immunity list, `Fireball` ground-impact particles, `LandMine` beacon render, `Rocket` firework only for infected Darwinians, `SpaceInvader` old `ChangeHealth`, `Missile` `m_history` (filled, never read), `SpaceInvader::m_napalmDrops` (unused), `Nuke::m_armed` (never read), `Shaman::MAX_EGGS`.
- `USE_SEPULVEDA_HELP_TUTORIAL` blocks (help-system callbacks in Squad/Task code) [B:DEAD].

<!-- from P05_entities_creatures -->
## Appendix — dead/legacy code
- **[B:LEGACY] Spam building** (`Spam`): blocked; emits SpamInfections every 30–60 s; research-mode variant; render-mutates orientation (desync). `code/worldobject/spam.cpp`.
- **[B:LEGACY] Tripod / TripodNavData**: no creator; hex navigation, two-leg fall physics, crouch-and-fire-lasers cycle; uses render clocks in sim. `code/worldobject/tripod.cpp`.
- **[B:LEGACY] Lander**: no creator; beach craft spawning LaserTroopers; overload/signature bugs. `code/worldobject/lander.cpp`.
- **[B:LEGACY] DropShip**: only reachable through the `DropShip` reinforcement level option used by an `old/` map. `code/worldobject/dropship.cpp`, `code/taskmanager.cpp:Task::TargetDropShip`.
- **[B:DEBUG]/[B:LEGACY] Tentacle**: Portal-only (The Beast crate in debug builds; Shaman maps). `code/worldobject/tentacle.cpp`.
- **[B:LEGACY] Shaman summon paths** for ArmyAnt/Centipede/Spider/SporeGenerator/SoulDestroyer/Tripod/Virii: `Shaman::CanSummonEntity` returns false; the switch is commented out. `code/worldobject/shaman.cpp`.
- **[B:LEGACY] Cave → Virii**: `Cave` is blocked. `code/worldobject/cave.cpp`.
- **[B:LEGACY] Darwinia campaign placements**: `data/levels/map_*.txt`, `mission_*.txt` instant units (AI 6, ArmyAnt 1, Centipede 55, Darwinian 60, SoulDestroyer 8, Spider 45, SporeGenerator 26, Virii 118) and buildings (AntHill 13, GodDish 1, Spam 3, Triffid 30) — campaign mode is not compiled into FULLGAME (`INCLUDEGAMEMODE_CAMPAIGN` off), these files are excluded from the shipped archives (`tools/multiwinia_exclusions.txt` L24–25 `levels/map_*`, `levels/mission_*`), and ArmyAnt instant units are additionally blocked. [C] grep of `data/levels`.
- **[B:LEGACY] Single-player branches** inside live classes: SP-only constants (AntHill objective rules, Triffid unlimited launching, Centipede SP explosions, SpamInfection SP target team 1/2 rules, SporeGenerator hover start) — retained in the sections above as SP vs MP rows.
- **[B:EDITOR] Triffid trajectory/trigger preview** in `Triffid::RenderAlphas` under `LOCATION_EDITOR`.
- **[B:DEBUG] Cheat window Virii spawn** (`CHEATMENU_ENABLED`): 20 Monster Virii, roam 1000. `code/interface/cheat_window.cpp`.
- **Dead code inside live classes**: `Virii::AdvanceDead()`, `Virii::AdvanceDeadPositionVector`, commented `Virii::RenderLowDetail/RenderGlow`, `Spider::StompFoot`, `SpaceShip::FindNewTarget` enemy-seeking block (commented), `WallBuster` AI-target creation (commented), `DustBall` alternative vortex steering (commented), Centipede D3D tearing (commented), Triffid `SpawnTripod` case (commented), `Egg::StateOpen` (never set), `Meteor::m_life` decrement after `return`, `SporeGenerator` constructor shape code (commented).

<!-- from P06_buildings_core -->
## Appendix — dead/legacy code (this partition)

| Item | Status | Note | Ref |
|---|---|---|---|
| Darwinia single-player spawn formula, red-team objective, spirit "accelerate to heaven" | [B:LEGACY] | `!Multiplayer()` branches of SpawnPoint/MasterSpawnPoint | [C] |
| GunTurret SP player-control loss (team ≠ 2), green→yellow remap, SP model `battlecannonbarrel.shp` | [B:LEGACY] | | [C] |
| ControlTower, Incubator | [B:FULL] code, [B:LEGACY] content | only Darwinia and GameTypes-none maps | [C] data |
| Generator, PylonStart, Powerstation, ConstructionYard, DisplayScreen, SpiritReceiver, ReceiverLink, ReceiverSpiritSpawner, SpiritProcessor, UnprocessedSpirit, SpiritStore (via Factory) | [B:LEGACY] | blocked or only used by blocked buildings | [C] |
| `Portal::SpecialSummon`, `SummonStateLoweringPortal`, commented master-portal code | [B:LEGACY] / dead | Shaman blocked | [C] |
| Commented-out code: SpawnBuilding target-type check, SpawnPoint alternate team assignment, RadarDish MP `ValidReceiverDish` block and AI priority block, GunTurret "fire nonstop if enemies present" (disabled: "something wrong with the entity grid"), GunTurret crosshair/manual-input code, JumpPad particle ring and arrow quad, CarryableBuilding debug text | dead | | [C] |
| `LOCATION_EDITOR` blocks: GunTurret mortar arc preview, JumpPad trajectory preview, SpawnPopulationLock radius sphere | [B:EDITOR] | | [C] |
| `DEBUG_RENDER_ENABLED`: `RenderHitCheck`, `RenderLink`, PylonStart requirement arrow, editor arrows | [B:FULL] (debug overlay compiled in) | | [C] |
| `USE_DIRECT3D`: RadarDish vertex-declaration reset, ReceiverBuilding/SpiritProcessor water-reflection skip and batched spirit rendering | [B:D3D] | | [C] |
| `#pragma omp parallel for` in `Location::Advance` | inert (OpenMP off) | | [C] |
| `Building::m_timeOfDeath` | unused in this partition | | [C] |

<!-- from P07_buildings_misc -->
## Appendix — dead/legacy code

All classes below are compiled into `multiwinia.exe` but are **[B:LEGACY]**: their type is in `Building::BuildingIsBlocked` under `BLOCK_OLD_DARWINIA_OBJECTS` [C] `code/worldobject/building.cpp:BuildingIsBlocked`, so level lines naming them are dropped at parse time, and no live MW code creates them by id (exceptions noted). Brief, but every class and every level field is listed for completeness. Census sources: `data/levels/map_*`, `mission_*` (Darwinia campaign) and `data/levels/old/*` (MW prototypes).

### A.1 BlueprintBuilding (abstract), BlueprintStore, BlueprintConsole, BlueprintRelay — `blueprintstore.cpp` [B:LEGACY]
Darwinia "Pattern Buffer" infection puzzle (`map_pattern_buffer.txt`: 1 store, 4 consoles, 12 relays).
- **BlueprintBuilding** fields: [8] buildingLink (int, `%-8d`) — next building in the relay chain. `Initialise` looks up `MarkerBlueprint`; `m_infected` = 100 if team 1 else 0. `Advance`: if `m_infected > 80` → `link->SendBlueprint(segment, true)`; if `< 20` → `SendBlueprint(segment, false)`. Default `SendBlueprint`: store segment id, `m_infected ± 10/s` clamped [0,100]. Render: shape; alpha pass draws an additive `textures/laser.bmp` ribbon (half-width 20) to the link, colour (infected, 0.7−0.7·infected, 0) with alpha `0.1+0.2|sin t|` when both ends agree within 0.01, else `0.5+0.5|sin t|`.
- **BlueprintStore** (`blueprintstore.shp`, `BLUEPRINTSTORE_NUMSEGMENTS` = 4): `m_segments[4]` start 100 (team 1) or 0. `SendBlueprint`: segment `± 1/s`. `Advance`: with f = #segments at exactly 100 and c = 4 − f, each segment < 100 gains `f·0.9/c` per second; if all segments are 0 → GlobalBuilding online. Objective counter "objective_totalinfection N%". Render: a 100×100 billboard of `sprites/darwinian.bmp` with per-corner colours from the four segments plus two scrolling `interface_grey.bmp` overlay passes; display wobbles with `sin/cos(gameTime)`.
- **BlueprintConsole** (`blueprintconsole.shp`): fields [8] buildingLink, [9] segment (int). Ownership: team with > 2 port occupants and plurality, else 255 (same rule as CloneLab). Team 1 → `SendBlueprint(seg, true)`, team 0 → `SendBlueprint(seg, false)` every tick. Port status starbursts as CloneLab.
- **BlueprintRelay** (`blueprintrelay.shp`): fields [8] buildingLink, [9] altitude (float, default 400; `%-2.2f`). Floats at Y = altitude; `Advance` drifts position by `(sin(t/1.5), sin(t/1.3), cos(t/1.7))` units per tick with t = gameTime + uniqueId + index (sim position driven by game time), `m_vel` derived.

### A.2 Bridge — `bridge.cpp` [B:LEGACY] (subclass of `Teleport`, P06)
Darwinia Engineer-operated bridge; created at runtime only by `Engineer` [C] `code/worldobject/engineer.cpp` (≈L1006). No level lines in any file. Fields: [8] nextBridgeId (int), [9] bridgeType (0 = `BridgeTypeEnd` → `bridgeend.shp`, 1 = `BridgeTypeTower` → `bridgetower.shp`; marker `MarkerSignal`). `BRIDGE_TRANSPORTPERIOD` 0.1 s (Teleport send period), `BRIDGE_TRANSPORTSPEED` 50 wu/s. `m_status` = 100 while an engineer operates it (`BeginOperation`/`EndOperation`), else 0; `< 0` → removed. Only End-type bridges with an operated, linked next bridge send entities; entities in transit fly signal-to-signal at 50 wu/s, hop through Tower bridges via `EnterTeleport`, exit at the final End bridge's entrance marker; if the carrier signal is lost mid-transit the entity takes `ChangeHealth(−500)` and is dropped with random velocity (`SFRAND(10), FRAND(10), SFRAND(10)`). Render: 3-px smoothed line between signal markers in team colour with alpha = status/100.

### A.3 Cave — `cave.cpp` [B:LEGACY]
`cave.shp`, marker `MarkerSpawnPoint`; no extra level fields. For a used team: every `syncfrand(0.5)` s spawns one `Entity::TypeVirii` at the spawn marker if fewer than 30 friends within 100 units. `Damage(d)` destroys it (shape explosion) only if `d > 80` — but damage is passed as negative numbers everywhere (e.g. `Location::Bang`), so caves are effectively indestructible [C]/[I].

### A.4 ChessBase, ChessPiece — `chess.cpp` [B:LEGACY] (prototype MW mode; `old/mp_chess1.txt` ×2 ChessBase)
- **ChessBase**: fields [8] width, [9] depth (read with `atoi` → integers; `Write` uses the malformed format `"%2.2 %2.2"` with no conversion letter — a writer bug). Spare points `m_sparePoints = GetGameOption("TotalSize")`; neither "TotalSize" nor "SizeVariance" exists in any game blueprint (grep), so `GetGameOption` returns −1 and **no pieces ever spawn** [C] `code/multiwinia.cpp:Multiwinia::GetGameOption`. Logic (if it worked): every 1 s spawn a piece of scale `1 + syncsfrand(variance/100)` clamped [0.1, spare] while spare > 0.3, pushed 10 units at a time away from overlapping pieces (`50·scaleA + 50·scaleB`); an enemy piece whose 35·scale sphere (offset 35·scale behind it) intersects the width×depth zone triggers `HandleSuccess`. Render: team-coloured line loop of the zone 5 units above ground.
- **ChessPiece** (`CarryableBuilding`, `chesspiece.shp`): `HandleCollision(force)`: damage += force·dt; every 2.0 → scale −0.05 + shatter 0.3; dies when scale < 0.1. `HandleSuccess`: respawn a piece of the same scale at its own base, `Multiwinia::AwardPoints(team, int(10·scale))`, then die. `SetWaypoint` only accepts waypoints on one side of the piece (forward-only movement). The only ChessPiece selection path is `LocationInput` (turret-aim mode branch) [C] `code/location_input.cpp`.

### A.5 Factory — `factory.cpp` [B:LEGACY]
`factory.shp`; field [8] initialCapacity (int, default 10) → embedded `SpiritStore` (capacity, 200 max, at `MarkerSpiritStore` + 11 Y, sizes 5/10/5). `RequestUnit(type, n)`: creation time `n × 0.1` s, spawns one unit per spirit consumed at `pos + (±2.5, 20±2.5, 20)` with velocity `(SFRAND(1), SFRAND(1), 5 + syncsfrand(1))`; then 3 s `StateRecharging`. States `StateUnused → StateCreating → StateRecharging → StateUnused`; `SetTeamId` resets to Unused. `Factory::RequestUnit` has no caller anywhere in `code/` [C] (grep `RequestUnit(`).

### A.6 FeedingTube — `feedingtube.cpp` [B:LEGACY]
`feedingtube.shp`, marker `MarkerFocus`; field [8] receiverId (int). Purely visual link: each tick `m_range` = distance between the two focus markers. Render: 4 nested textured tubes (radii 10, 9, 8, 4; alphas 0.4, 0.2, 0.2, 0.5) between the dishes, 20 radial segments, `distance/200` length steps, units 0/1 = `laserfence.bmp` (REPLACE) / `radarsignal.bmp` (MODULATE), scrolling by `−gameTime/radius` and `−gameTime`, clipped by two user clip planes facing each dish. Sphere hit uses the shape at 1.5× probe radius. Sound events `BeginRotation`, `EndRotation`, `ConnectionEstablished`, `ConnectionLost` (listed, not triggered here).

### A.7 DynamicBase (abstract), DynamicHub, DynamicNode — `generichub.cpp` [B:LEGACY]
Generic objective network (see `docs/new building docs.txt`). `CreateBuilding` forces DynamicHub global.
- **DynamicBase** fields: [8] buildingLink (int), [9] shapeName (token, "none" → debug sphere 40). Sound `TriggerSurge`.
- **DynamicHub** fields: [10] requiredScore, [11] currentScore, [12] minActiveLinks (ints). `m_reprogrammed` true if no ControlTower links to it, or the linking tower has the hub's team; counts linked nodes. Enabled when all nodes active (`ActivateLink`/`DeactivateLink`) or (score mode) `currentScore ≥ requiredScore` and `activeLinks ≥ minActiveLinks`; when `currentScore ≥ requiredScore`, enabled → GlobalBuilding online, else (mission not complete) offline. `ChangeScore` clamps to requiredScore. `PointsPerHub = requiredScore / minActiveLinks` (**integer division by zero if minActiveLinks = 0**). Objective counter text "percent complete: N" (hard-coded English).
- **DynamicNode** fields: [10] scoreValue, [11] scoreSupplied. Operates when all ports are occupied and none by team 1 → `hub->ActivateLink()`; stops when any port empties. While operating with scoreValue > 0: every 1 s add `±scoreValue` (−if any red occupant) while `PointsPerHub() > scoreSupplied`. Portless nodes activate on `ReprogramComplete`. Editor render aligns to terrain normal.

### A.8 GodDish — `goddish.cpp` [B:LEGACY] (`map_temple.txt`, `old/mp_temple.txt`)
`goddish.shp`; no extra fields. Script-driven (`Script::RunCommand_GodDishActivate`, `RunCommand_GodDishDeactivate`, `RunCommand_GodDishSpawnSpam`, `RunCommand_GodDishSpawnResearch`, `RunCommand_SpamTrigger`, `code/script.cpp`). `Activate`: makes all team-0 Darwinians `WatchSpectacle`/`CastShadow`, sound `ConnectToGod`; timer counts up; deactivated timer decays ×(1 − 2·dt). `SpawnSpam(isResearch)`: creates a `Spam` building (by type id) 1125 units up and 675 forward, falling at 80 wu/s. `TriggerSpam`: every Spam → `SpawnInfection()`. Render: additive `cloudyglow.bmp` blobs (50/25/10 by RenderBuildingDetail 1/2/3) and `starburst.bmp` stars (10/5/2), alpha `min(timer·0.1, 1)`, depth test off.

### A.9 Library — `library.cpp` [B:LEGACY]
`library.shp`; no extra fields. Each tick, for every research type the player owns that has not been shown, spawns a `ResearchItem` (in-library) at marker `MarkerResearch%02d` (1-based).

### A.10 MineBuilding (abstract), MineCart (helper), TrackLink, TrackJunction, TrackStart, TrackEnd, Refinery, Mine — `mine.cpp` [B:LEGACY]
Darwinia "Mine"/"Yard" conveyor (`map_mine.txt`, `map_yard.txt`: 111 TrackLinks, 3 junctions, 1 start, 2 ends, 4 mines, 1 refinery).
- **MineBuilding** field [8] trackLink (int). Shared static shapes `wheel.shp`, `minecart.shp` (markers `MarkerTrack1/2`, `MarkerContents1–3`), `minepolygon1.shp`, `mineprimitive1.shp`. `Initialise` seeds carts at progress `0.2 + syncfrand(1.5)` cumulative steps < 1. Speed `RefinerySpeed()` (global, recomputed every 0.1 s): fraction of occupied ports of the first Refinery or Yard found, else 1.0 if the global Refinery on location "mine" is online, else 0; then `× |sin(2·gameTime)| × 0.5` progress/s (`numFuelGenerators` is never incremented → the fuel-generator branch is dead). Carts advance by speed·dt, overflow passes to the link via `TriggerCart`. Sound `CogTurn` when speed rises above 0.1. Render: two `laser.bmp` rails (width 2, or 1 at lower detail) between track markers; carts hang 40 units below the rail midpoint with contents meshes and a `glow.bmp` sprite on primitives (near plane nudged ×1.1).
- **MineCart**: `m_progress` (0–1), `m_polygons[3]`, `m_primitives[3]`.
- **TrackLink**: plain segment (`tracklink.shp`).
- **TrackJunction**: fields [8] trackLink then any number of extra link ids (read while tokens remain, `%-4d`); routes each arriving cart to `syncrand() % n` of its links.
- **TrackStart**: field [9] reqBuildingId; while that GlobalBuilding is online, carts past 50% get one random primitive.
- **TrackEnd** (shape name spelled `trackLink.shp`): field [9] reqBuildingId; online if that building is a TrunkPort with open timer > 0, a Yard, or a FuelGenerator; carts past 50% unload primitives (into `ConstructionYard::AddPrimitive` if Yard) and clear polygons. Dereferences the building without a NULL check.
- **Refinery** (`refinery.shp`, wheels `MarkerWheel01–03`, `MarkerCounter`): a cart with polygons and no primitives becomes one random primitive and increments `GlobalBuilding::m_link` (refined count); at ≥ 20 refined → online. Counter text rendered in 3D.
- **Mine** (`mine.shp`, wheels `MarkerWheel01–02`): an empty arriving cart is filled with polygons with probability = occupied ports / ports.

### A.11 ResearchItem — `researchitem.cpp` [B:LEGACY] (8 campaign lines)
`researchitem.shp`, markers `MarkerGrab1/2`; fields [8] researchType (token, `GlobalResearch::GetType` name), [9] level (int, optional, default 1). Hovers 20 above ground; reprogram progress `m_reprogrammed` 100 → 0 at 3/s while an Engineer works it (`Reprogram`); at 0 → shatter, `AddResearch`, set level, "AquireResearch" sound, task-manager message; removed when the research is already owned at ≥ level. Constructor orients it with unsynced `frand`; `Render` rotates `m_front/m_up` every frame (render-path mutation of orientation that `Advance` uses for `CalculateCentre`) [C]. Non-colliding except rays (sphere of `m_radius`). Visuals: blue (0.1,0.2,0.8) `cloudyglow` blobs, starbursts and a 1050-unit "control line to heaven" (`laser.bmp`) fading with reprogram progress.

### A.12 SafeArea — `safearea.cpp` [B:LEGACY] (4 campaign lines)
Objective circle. Fields [8] size (float, default 50), [9] entitiesRequired (int), [10] entityTypeRequired (int entity type id, default Darwinian). Every 2.0 s counts live friendly entities of that type within `size`; team 1 → online when count ≤ required; other teams → online when count ≥ required. Objective counter "objective_currentcount : N". Editor-only circle render.

### A.13 UpgradePort, PrimaryUpgradePort — `upgrade_port.cpp` [B:LEGACY]
No level lines anywhere. `UpgradePort`: shape `upgrade_port.shp`, no behaviour. `PrimaryUpgradePort` (`primaryupgradeport.shp`, name string "PrimaryUpgrade"): `ReprogramComplete` increments `m_controlTowersOwned`; at exactly 3 → GlobalBuilding online.

### A.14 Dead code inside live classes
- Tree: per-branch wobble animation (commented out in `RenderBranch`); `Render` hit-check call commented out.
- LaserFence: obstruction-grid recalculation calls commented out in `Advance`.
- Wall: sinking/falling animation and damage accumulation commented out (`Advance`, `Damage`); `m_fallSpeed` always 0.
- RestrictionZone/SmokeMarker/EruptionMarker editor visualisation: `LOCATION_EDITOR` only [B:EDITOR].
- ScriptTrigger Sepulveda phrase sequencing: `USE_SEPULVEDA_HELP_TUTORIAL` [B:DEAD].
- `NET_DARWINIA_TOGGLELASERFENCE` handler without sender (`ClientToServer::RequestToggleFence` uncalled).

<!-- from P08_crates_ai -->
## Appendix — dead/legacy code
- **[B:DEAD] commented-out rewards**: `CrateRewardFormationIncrease` (`RunFormtationIncrease` body commented; would set `Team::EvolutionFormationSize = 200`), `CrateRewardExtinguisher` (`RunExtinguisher` still compiled but uncalled; it uses **non-synced** `sfrand/frand` for particles — harmless cosmetics, but a desync trap if reused), `CrateRewardTank` (`Task::TargetTank` body commented). Declared-only `RunShaman`, `RunEngineers` (no definition) [C].
- **[B:DEBUG]** `CrateRewardTheBeast` (`RunTheBeast` → Portal of monster team), crate help window, cheat-window crate spawner, `TRACK_SYNC_RAND` logging (`SyncRandLog` of `s_crates`).
- **[B:EDITOR]** `InvalidCratesWindow`; editor rendering of AITarget link range sphere, AIObjective (red; blue when defence; white flash when active), AIObjectiveMarker (scan-range circle, arrow to bound building).
- **Compiled but unreachable in FULL**: `Multiwinia::AdvanceRetribution` (never called; `RetributionMode` option is inert); `RunUncapturedCrate` forced-id branch and its Airstrike/Darwinians/Forest/Eggs/Meteor/NapalmStrike/SlowDown/SpeedUp cases; `CaptureCrate` Plague case (commented → assert); `AI::RunSpam`, `AI::RunPlague`, `AI::RunTank`, `Tank::RunAI` (no way to obtain those tasks without a forced reward / TankBattle); `AI::HasDarwinianPowerup`; `AI::GetClosestScoreZonePos` (only from commented code); `Location::InitMonsterTeam(capturer)` call in bad crates.
- **[B:LEGACY]** `AI::AdvanceSinglePlayerAI` (Darwinia single-player: well-defended rule with idle limit 30, same sendChance formula, radius 150, `PushFromObstructions`, `RecountTeams` after each send); `AISpawnPoint` (blocked building type; spawns `m_count` entities of `m_entityType` every `m_period ± period/6` s once its activator — BlueprintStore with < `BLUEPRINTSTORE_NUMSEGMENTS` clean and ≤ 2 infected, BlueprintConsole owned by team 0, or online/absent global building — is on, until primary objectives are met or `m_spawnLimit` batches are used; respects SpawnPopulationLock); `ResearchItem` markers; `CloneLab` AITarget; single-player branches in `FindTargetBuilding` (25 % random) and `AITarget::GetRange` (100).

<!-- from P09_control_taskmanager -->
## Appendix — dead/legacy code

- **[B:LEGACY] `TaskManagerInterfaceGestures`, `TaskManagerInterfaceIcons`**, SP branches of `GameCursor::Render`, SP target areas (ControlTower, open TrunkPort), SP auto-select on `RunTask`, SP `Capacity`, research screens, objectives screens, quick-unit d-pad creation (`ControlIconsTaskManagerQuickUnitLeft/Right/Up` → Engineer/Armour/Squad; `…Down`/`Left` officer none/follow) [C].
- **[B:LEGACY]** `RegisterTask` user: saved running programs (`Location` load) [C] (the Teleport/RadarDish user is live, §1.5).
- **[B:DEAD] `GestureDemo`** (`code/gesture_demo.{h,cpp}`, all under `USE_SEPULVEDA_HELP_TUTORIAL`): loads `data/gesture_demos/<name>` (one "x y" pair per line, normalised by screen height, 40 ms per sample = 25 Hz); `GetTotalDuration = count·0.040 s`; `GetCoords(t)`: index = t·25, interpolates between floor/ceil samples — **with the weights swapped** (fractional part weights the *lower* sample), clamps to the last sample, scales by screen height. Played by `Sepulveda::RenderGestureDemo`: 40 px wide GL lines in 0.04 s steps, colour fades with age (2 s), loops 2 s after the end, a mouse cursor drawn at the pen tip [C] `code/sepulveda.cpp` (dead). Recorder `GestureRecordingThread` samples `g_target` every 40 ms while `ControlGestureActive` and writes `gesture_demos/new_gesture_demo.txt` (`%.3f %.3f`).
- **[B:DEAD] `code/gesture_demo_win32.cpp`**: `RecordGestureDemo()` spawns the recorder on a `THREAD_PRIORITY_TIME_CRITICAL` Win32 thread; requires `GESTURE_EDITOR` (TARGET_DEBUG) **and** `USE_SEPULVEDA_HELP_TUTORIAL` (never) [C].
- **[B:DEAD]** Gesture editor window `code/interface/gesture_window.cpp` (same double gate).
- **[B:D3D]** `TaskManagerInterfaceGestures::ReleaseD3dResources()` frees the gesture bitmaps/textures on device loss (`USE_DIRECT3D`) [C].
- Commented-out code: Controller task creation, squad weapon cycling in `LocationInput`, `TargetExtinguisher`, `TargetTank`, Shaman game type start task, MP "special" Darwinian/officer pseudo-slots in the task bar, keyboard F-key shortcuts in the MP interface.

<!-- from P10_input_camera -->
## Appendix — dead/legacy code

- **SDL input** [B:DEAD] (Mac/Linux only; not in vcproj): `code/lib/input/inputdriver_sdl_key.{h,cpp}` (SDL key → Win32 VK mapping: punctuation, arrows, shifts → `KEY_SHIFT/CONTROL/ALT/META`, a–z −32, 0–9/backspace/tab unchanged, F-keys −170, others 0), `inputdriver_sdl_mouse.{h,cpp}` (same `mouse` grammar as W32), `sdl_eventhandler.{h,cpp}` (`SDL_QUIT` → quit; `WindowHasFocus` uses `(SDL_GetAppState() | SDL_APPACTIVE) != 0`, always true — bug), `sdl_eventproc.h`. `inputdriver_sdl_key.cpp` and `inputdriver_sdl_mouse.cpp` were skimmed (key mapping and grammar only); the event-handler files were read fully.
- **Xbox event handler** [B:DEAD]: `code/lib/input/xbox_eventhandler.{h,cpp}` — `.cpp` `ExcludedFromBuild` in all configs (would duplicate `g_windowHasFocus`).
- `code/lib/input/inputfilterspec.cpp` (`newFilterSpecID`) — not in vcproj [B:DEAD].
- Pipe driver, filter manager, direction filter — compiled, never instantiated [B:LEGACY] (§1.6.10, §1.7).
- `code/lib/input/control_types.cpp` — compiled, unused duplicate action table [B:LEGACY].
- `W32EventHandler::BindAltTab/UnbindAltTab/ResetWindowHandle` — compiled, no live caller of `BindAltTab` [B:LEGACY].
- Camera modes `ModeReplay` [B:DEAD]; `ModeFirstPerson`, `ModeEntityFollow`, `ModeBuildingFocus`, `ModeMoveToTarget` (animations/scripts), all `ModeSphereWorld*` [B:LEGACY] (§3.3, §3.8); helpers in §3.9 [B:LEGACY].
- `ControlHelpSystem` rendering, conditions and on-screen aids [B:LEGACY] (§4.1); `code/tutorial_demo2.cpp` aid calls [B:DEAD] (`USE_SEPULVEDA_HELP_TUTORIAL`).
- Under `USE_SEPULVEDA_HELP_TUTORIAL` [B:DEAD] in `code/camera.cpp`: `HelpSystem::PlayerDoneAction(CameraMovement / CameraZoom / CameraHeight / UseRadarDish)` notifications. Under `_DEBUG` [B:DEBUG]: `DebugCameraFixUp` reset of intro/outro.
- `docs/DarwiniaInput.rtf` and `docs/input_preferences_syntax.txt` describe the Darwinia-era system; still broadly accurate (see discrepancies).

<!-- from P11_terrain_world -->
## Appendix — dead/legacy code

| Item | Status | Evidence |
|---|---|---|
| `Landscape(double, int, int)` constructor, `LandscapeRenderer::Initialise`, `Water::RenderReflectiveWater` | declared, never defined/called | [C] grep |
| `Landscape::RenderHitNormals` | defined, no caller | [C] |
| `LIGHTMAP_SCALEFACTOR` | unused | [C] |
| `RenderLandscapeDetail` reads (landscape cell scaling, overlay skip at detail ≥ 4) | commented out → literal 1 | [C] |
| Water D3D `UpdateWaterThread` | never started | [C] |
| `USE_DIRECT3D` paths of landscape/water + `WaterReflectionEffect` | [B:D3D]; water.cpp does not compile under D3D | [C] |
| Mac water update throttling (`TARGET_OS_MACOSX`) | [B:DEAD] on PC | [C] |
| Mission-file parsing, `SaveMissionFile`, `RunningPrograms`, `Difficulty`, `LeaderboardStats`, `GenerateInstantUnits`, `GenerateDynamicBuildings` (called from `App::SaveProfile` for single-player only) | [B:LEGACY] | [C] `code/app.cpp:SaveProfile` |
| GlobalWorld locations/buildings/events/tutorial/level-times, `LoadGame`, `SaveGame`, `LoadLocations`, `SaveLocations`, `TransferSpirits`, `GlobalWorld::Advance` (island picking), `SphereWorld`, `GlobalInternet` rendering | [B:LEGACY] (constructed, never shown) | [C] §8.8, §24.5 |
| `GlobalEventCondition::DebugKey` evaluation | only under `JAMES_FIX` (never defined) → falls to `AppDebugAssert(false)` | [C] |
| `GlobalWorld::WriteTutorial` body | `DEMO2 && USE_SEPULVEDA_HELP_TUTORIAL` → empty | [C] |
| `WriteLevelTimes`, `WriteSquaddiesUsed` | empty bodies | [C] |
| `LevelFile::HasEffect` | always false | [C] |
| Burn patches (`Location::AddBurnPatch`, only from `Portal`) | [B:LEGACY] | [C] grep |
| Editor water grid, `DeleteTile`, `.mwm` save/list, global-world editor | [B:EDITOR] | [C] |
| `Clouds::RenderFace` | [B:DEBUG] (cheat window) | [C] |

<!-- from P12_render_core -->
## Appendix — dead/legacy code
| Item | State | Ref |
|---|---|---|
| Pixel effect (`PreRenderPixelEffect`, `ApplyPixelEffect`, `PaintPixels`, `RasteriseSphere`, `MarkUsedCells`) | compiled, unreachable in Multiwinia (single-player only) [B:LEGACY] | `code/renderer.cpp` |
| `DeformEffect`, `Shader`, `FixedPipeline`, `Texture`, `WaterReflectionEffect` | [B:D3D] only; deform & reflection prerender additionally single-player only | `code/deform.cpp`, `code/lib/{shader,FixedPipeline,texture}.cpp`, `code/water_reflection.cpp` |
| `Renderer::RenderLogo` (`PROMOTIONAL_BUILD`), `RenderFlatTexture` (commented call), `privatedemo.bmp` (missing) | [B:DEAD] | `code/renderer.cpp` |
| Poster `PosterMakerPixelEffect` pre-pass (`#if 0`) | [B:DEAD] | `Renderer::Render` |
| Old `RenderMenuTransition` (`#if 0`) | [B:DEAD] | `code/renderer.cpp` |
| `USE_SEPULVEDA_HELP_TUTORIAL` calls (help system, tutorial, demo end sequence, Sepulveda) in `RenderFrame`; `GestureDemo` resource cache | [B:DEAD] | `code/renderer.cpp`, `code/lib/resource.cpp` |
| `LOCATION_EDITOR` branch (Location + editor render) | [B:EDITOR] | `RenderFrame` |
| `GlobalWorld::Render`, Darwin logo, `GameModePrologue` special lighting | [B:LEGACY] | `RenderFrame`, `Location::RenderBuildings` |
| `TARGET_OS_VISTA` thumbnail; `TESTBED_ENABLED` render gating in particles/explosions | [B:DEAD] | `Renderer::Render`, `ParticleSystem::Render`, `ExplosionManager::Render` |
| `EffectProcessor` (Pokey envelopes) | [B:DEBUG] (`SOUND_EDITOR`), unlinkable `Update_pokey_sound` | `code/effect_processor.cpp` |
| `Sierpinski3D` | compiled; only user `AmigaLoader` under `USE_LOADERS` (off) | `code/3d_sierpinski_gasket.cpp`, `code/loaders/amiga_loader.cpp` |
| `RenderSplitUpQuadTextured/MultiTextured` | compiled, no callers | `code/lib/render_utils.cpp` |
| `ShapeFragment::BuildDisplayList`, cylinder hit volumes, `RenderHitCheck`/`RenderMarkers` callers | compiled, unused | `code/lib/shape.cpp` |
| `Shape/ShapeFragment/ShapeMarker::WriteToFile`, exporter constructors (`EXPORTER_BUILD`) | exporter only; `ShapeMarker::WriteToFile` and fragment `up/front` lines use malformed `%5.2` formats (no conversion char) | `code/lib/shape.cpp` |
| Deform callers in `Centipede` and `EntityLeg` (`AddTearingSpider` does not exist) | commented out | `code/worldobject/centipede.cpp`, `entity_leg.cpp` |
| Legacy font path (`BuildOpenGlState` single bitmap, `m_oldTextureIds`) | compiled, unused (fonts use Unicode atlases) | `code/lib/text_renderer.cpp` |
| Mod system (`LoadMod`, mod lookup paths) | [B:DEAD] (`USE_DARWINIA_MOD_SYSTEM`) | `code/lib/resource.cpp` |
| `Renderer::SetScreenGamma` | [B:D3D], no callers | `code/renderer.cpp` |
| `CHECK_OPENGL_STATE` / `CheckOpenGLState(full)` | `OPENGL_DEBUG_ASSERTS` never defined | `code/renderer.h` |
| SDL MSAA request | non-Win32 | `code/lib/window_manager_sdl.cpp` |

<!-- from P13_gfx_backend_math -->
## Appendix — dead/legacy code

- **SDL window manager** `code/lib/window_manager_sdl.{cpp,h}`, `code/lib/ogl_extensions_sdl.cpp` — not in `Darwinia.vcproj` (Mac/Linux). Skimmed: `SDL_SetVideoMode` with RGB 8/8/8 or 5/5/5, double buffer, depth = zDepth, **antialias via `SDL_GL_MULTISAMPLEBUFFERS 2` / `SDL_GL_MULTISAMPLESAMPLES 4`** (retries without AA, then with 16-bit Z), `SDL_GL_SWAP_CONTROL 1` (vsync); extensions via `SDL_GL_GetProcAddress` + `gluCheckExtension`, no Intel blacklist [B:DEAD].
- **`AviPlayer`** (`EGAMESBUILD`) — VfW AVI playback into a 512×256 RGB buffer for the eGames distributor loader [B:DEAD].
- **`AVIGenerator`** (`AVI_GENERATOR`, `TARGET_DEBUG` only) — frame capture never fed (`AddFrame()` uncalled) [B:DEBUG, effectively non-functional].
- **`TARGET_OS_VISTA` block** in `WindowManagerDirectX` `WinMain` (`-mediacenter`, save-file argument) [B:DEAD].
- **Mac precision control** inline asm in `StandardiseCPUPrecision` [B:DEAD].
- **Unreferenced in live builds**: `code/lib/math_utils.cpp:syncrand` (+ `init_genrand`); `OpenGLRegisterD3DTexture`; `Attributes`/`s_attributeStack` (no `glPushAttrib`); `WindowManagerWin32::m_hRC2` secondary context and the declared-but-undefined `Use/Release/DestroySecondaryContext`; `gglChoosePixelFormatARB`; `Renderer::SetScreenGamma`; `glTraceEnable` callers (`USE_LOADERS` off); `TriTriIntersection` (implementation file excluded); `Matrix34::InverseMultiplyVector`, `Matrix34::Transpose`, `Matrix33::Invert`, `Plane(normal, point)`; `FixedPipeline::m_state.m_environmentDiffuse/Specular` (never assigned); `fogColour` uniform in `FixedFunc2.vs`; triple-buffering block and NVPerfHUD adapter selection in `Direct3DInit` (commented out); `glFinish` back-buffer lock (after `return`).

<!-- from P14_platform_lib -->
## Appendix — dead/legacy code (brief)

* `code/keygen.cpp` — Darwinia registration key generator, compiled but uncalled [B:LEGACY] (§29.B.4).
* `code/lib/user_info.cpp:GetUserInfoData` — uncalled [B:LEGACY].
* `contrib/systemiv/lib/filesys/file_system.cpp:FileSystem` — compiled, never instantiated (§31.5.4).
* `contrib/systemiv/lib/memory_leak.cpp` — only with `TRACK_MEMORY_LEAKS` [B:DEAD].
* RDTSC timer path in `code/lib/hi_res_time.cpp` (`USE_PENTIUM_COUNTER 0`) [B:DEAD]; `code/lib/hi_res_time_gcc.cpp`, `hi_res_time_sdl.cpp`, `system_info_macosx.cpp`, `macosx_hardware_detect.cpp`, `debug_logging.{h,cpp}`, `debug_fog_logging.cpp` — not in the vcproj [B:DEAD].
* `contrib/systemiv/lib/{preferences,language_table,hi_res_time}.*` — shadowed by `code/lib` versions, not compiled [B:DEAD].
* `contrib/systemiv/lib/tosser/hash_map_darwin.h` — GNU `hash_map<std::string>` shim for Mac [B:DEAD].
* `TRACK_SYNC_RAND` logging, `SINGLE_THREADED_PROFILER`, Mac/Linux branches of `debug_utils`, `unicode_string`, `network_stream`, `unicode_text_file_writer` [B:DEBUG]/[B:DEAD].
* `targets/linux/*`, `targets/macosx/Darwinia/*`, Darwinia-era tools (build_demo*, build_final, build_vista, build_tools, Loki, NSIS/GESetup, eGames/Demo Inno scripts, 3DGeometry) [B:LEGACY].
* Xbox remnants in the data pipeline (`platxbox` branches, `convert_bmp_to_tex.cmd`, DXT conversions, `textures/xboxlivearcade.bmp` in `init.list`) [B:DEAD].

<!-- from P15_network -->
## Appendix — dead/legacy code
- `code/network/network_defines_debug.h` [B:DEAD]: human-readable alternative codes ("DARWINIA", "CMD", "ClientJoin", …, incl. obsolete fields X/Y/Z/Flags/DUMoveDX…); not included anywhere.
- `code/network/networkupdate.inc` [B:DEAD]: inline accessors for a removed `NetworkUpdate` class (Darwinia).
- `contrib/systemiv/lib/metaserver/metaserver_defines_old.h` [B:DEAD]: older readable protocol (metaserver port 5008, LAN period 2 s, WAN 10 s, discard 20 s, names "METASERVER"/"Register"/…).
- `contrib/systemiv/lib/metaserver/steamauth_client.*`, `steamauth_server.*` [B:DEAD]: Steam authentication state machine (NotStarted → Started → StartingSteam → StartingSteamAPI → LoggingOnToSteam → GettingSteamId → GettingSteam3Auth → RequestAuthentication → Success/SuccessDemo/Failed; ≤ 10 retries with n×100 ms backoff), STEAM_AUTHENTICATE "sa" carrying the Steam3 ticket; server side wraps Steam GameServer auth (approve ⇒ Accepted, `k_EDenyNoLicense` ⇒ Demo, else Failed). Not in the vcproj; requires Steam SDK headers.
- `contrib/systemiv/lib/netlib/net_lib_linux.h`, `net_lib_apple.h`, `net_mutex_linux.cpp`, `net_thread_linux.cpp` [B:DEAD on PC]: pthread/BSD-socket ports (recursive pthread mutex, cond-var events, POSIX semaphores, detached threads).
- `contrib/systemiv/lib/network_stream_inlines.cpp`: not a compilation unit but `#include`d by `network_stream.h` — live.
- `code/network/network_quality.*` (simulator), `code/network/listener.*`, both `bytestream.h` [compiled, unused].
- [B:DEBUG]/`TARGET_BETATEST_GROUP`: full IFrame snapshots and diffs (§8.2); `TRACK_SYNC_RAND` per-letter logging (`DecodeCmd`, `WriteSummary`, `SyncRandLog`, `DumpSyncRandLog`); `ControlDebugDropPacket`.
- [B:DEBUG] `TESTBED_ENABLED`: rand-seed override, spectator auto-registration (`RegisterSpectator` → SPECTATORASSIGN), SET_TEAM_READY, 50-attempt connect limit, testbed server names.
- [B:EDITOR] `LOCATION_EDITOR`: REQUESTMAP join flag, CUSTOMMAPNAME advertising/browser display.
- Console remnants: XUID, PDLC bit mask, JOIN_VIA_INVITE, ENCRYPTION_PADDING_*, `Disconnect_ClientNeedsPDLC`, `IQNetPlayer` forward declarations (Xbox 360 QNet).

<!-- from P16_sound -->
## Appendix — dead/legacy code

- **[B:DEAD] `SoundLibrary3dXAudio`** (`code/sound/sound_library_3d_xaudio.*`, Xbox 360): whole TU under `HAVE_XAUDIO`; 6-channel (5.1) X3DAudio emitters with positions divided by min distance (`CurveDistanceScaler` 10000), 3 packets per channel sized for an assumed 15 Hz update, music voice at 44100 Hz with 10/15 s packets, `GetMaxChannels` 32, DSP functions empty, pitch via `XAudioSampleRateToPitch`, music channel topped up twice per Advance.
- **[B:DEAD] `SoundLibrary2dSDL`** (`code/sound/sound_library_2d_sdl.*`, not in the vcproj; needs `sound/macosx/ASWSDLRingBuffer.h`): SDL audio at `min(SoundMixFreq, 44100)`, 512-sample device buffer, ring buffer of 1/15 s pre-filled with 1/10 s of silence, filled from `TopupBuffer` by elapsed wall time; with `INVOKE_CALLBACK_FROM_SOUND_THREAD` the mixer runs inside the SDL callback under `SoundSystem::m_mutex`; pref `Sound` (default 1) un-pauses.
- **[B:DEAD] `code/unicode/sound_library_*`** — older copies, not built (build_matrix §6).
- **[B:DEAD] Mac preloading** (`SoundSystem::PreloadSounds`, `BackgroundSoundLoad`, `LoadSound`, `CachedSample::Preload`, `SampleLoadState`): preload every blueprint sample except Music/Gesture/Sepulveda/Interface/MultiwiniaInterface; first group member on the main thread, the rest on a low-priority thread; fall back to the first group member while loading; LRU disabled.
- **[B:DEAD] `INVOKE_CALLBACK_FROM_SOUND_THREAD`** locking variant (Mac).
- **[B:DEAD] Options UI for `USE_SOUND_MEMORY`, `USE_SOUND_HW3D`, `USE_SOUND_DSPEFFECTS`** (§09.1 rows 6–8) and the unreachable tail of `PrefsSoundWindow::Render`.
- **[B:DEAD] Loader/boot-sequence music** (`USE_LOADERS`): LoaderSpeccy, LoaderAmiga, LoaderMatrix, LoaderRaytrace, LoaderGameOfLife, LoaderFodder, LoaderSoul, Credits, StartSequence.
- **[B:DEAD] `SOUNDSYSTEM_VERIFY` / `RuntimeVerify`**, `LoadtimeVerify` (call commented out), `TESTBED_ENABLED` mute.
- **[B:DEBUG]** Sound editor family (§27.13), `ConvertVolume` legacy migration (maps old 0–3+ volume scale to 0–10: ≤1→0, ≤1.25→0.5, ≤1.5→1.8, ≤1.75→3.6, ≤2.0→5.0, ≤2.25→6.1, ≤2.5→7.0, ≤2.75→7.65, ≤3.0→8.5, else 10), Purge Sounds, record-to-WAV (`AVI_GENERATOR`).
- **[B:LEGACY]** Darwinia-only sources/events compiled and loaded but unreachable or blocked in Multiwinia: `OTHER Sepulveda` (Sepulveda code under `USE_SEPULVEDA_HELP_TUTORIAL`), `OTHER Spirit` (only `PickedUp` live), `Ambience EnterGlobalWorld` (global world), Music cutscenes (scripts), and BUILDING blocks for types listed in `Building::BuildingIsBlocked` (`BLOCK_OLD_DARWINIA_OBJECTS`, blocks creation by name from level files) [C] `code/worldobject/building.cpp:BuildingIsBlocked`: BlueprintConsole, BlueprintRelay, BlueprintStore, Generator, GodDish, Library, Mine, Powerstation, Refinery, ResearchItem, SpiritProcessor, SpiritReceiver, TrackLink, Yard are Darwinia-only [I]; Crate, Statue and Spam are also in that list but are spawned at runtime by Multiwinia game modes/crates and are live [I] (owned by P06–P08). EscapeRocket and the Fuel* buildings (Rocket Riot) are not blocked.

<!-- from P17a_ui_framework_editor -->
## Appendix — dead/legacy code

- **[B:DEAD] GestureWindow** (`code/interface/gesture_window.*`): see §D7; needs `USE_SEPULVEDA_HELP_TUTORIAL`.
- **[B:DEBUG], unreachable: GrabberWindow** (§D6) — no caller of `DebugKeyBindings::GrabberButton`.
- **Unreachable everywhere: DebugMenu** (§D3) — opener commented out.
- **[B:EDITOR] unreachable: CameraAnimMainEditWindow/SecondaryEditWindow** (§E4.10).
- **[B:LEGACY] GlobalWorldEditorWindow** (§E4.13) — Darwinia global-world editor; `GlobalWorldEditorLoop` returns immediately.
- **[B:LEGACY] cheats 12–14** (Darwinia campaign globals).
- **Unused framework API**: `EclMaximiseWindow/UnMaximise`, `EclRegisterPopup`, `EclRegisterTooltipCallback`, `EclRegisterClearFunction`, `EclResetDirtyRectangles`, `EclGetDirtyRects`, `EclGenerateUniqueWindowName`, `EclButton::MouseMove`; `DebugMenu::Advance` (empty); `IsGlobalButton`, `PlayMapButton`, `EditMapButton`, `CurrentMapButton`, commented `SelectMapFileDialog`; `DebugKeyBindings::FollowCameraButton/InputLogButton/SoundVisualiserButton` (declared only), `ReallyQuitButton`/`ToggleFullscreenButton` (no Win32 caller).
- **Non-Win32 path**: `EclGetAccurateTime` `gettimeofday` branch (Linux/Mac).
- `MultiwiniaFileButton::SpecialScroll` (commented-out list scrolling for pad navigation of file rows).

<!-- from P17b_menus_options -->
## Appendix — dead/legacy code

| Code | Why dead/legacy | Tag |
|---|---|---|
| `MainMenuWindow` + its buttons (`MainMenuUserProfileButton`, `ModsButton`, `MultiwiniaButton`, `LeaderBoardButton`, `AchievementsButton`, `WebsiteButton`, `MenuGameExitButton`, `AboutDarwiniaButton`) | only opener is `GlobalWorldGameLoop`, which exits on its first iteration | [B:LEGACY] |
| `UserProfileWindow`, `ConfirmResetWindow`, `ResetProfileButton`, `NewUserProfileWindow`, `NewProfileButton`, `LoadUserProfileButton` | reachable only from `MainMenuWindow`; new-profile button commented out | [B:LEGACY] |
| `LeaderboardSelectionWindow`, `LeaderboardWindow`, `LeaderboardLoader` and button classes | inactive button in dead window; loader stub | [B:LEGACY] |
| `ResetLocationWindow`, `ResetLevelButton`, `ResetLocationButton` | `ResetLevelButton` never instantiated | [B:LEGACY] |
| `PrefsKeybindingsWindow`, `KeybindingsOptionsButton` | button creation commented out | [B:LEGACY] |
| `ReallyQuitWindow` | `DebugKeyBindings::ReallyQuitButton` has no caller | [B:LEGACY] |
| `ProfileWindow` | cheat/debug menus only | [B:DEBUG] |
| `CrateHelpWindow`, `CrateHelpWindowButton`, `CrateButton` | `INCLUDE_CRATE_HELP_WINDOW` (TARGET_DEBUG) | [B:DEBUG] |
| `MultiwiniaOptionsWindow` (+ `TextTab`, `AttackersButton`, `CoopGroupButton`) | `LOCATION_EDITOR` | [B:EDITOR] |
| `ModsWindow`, `NewModWindow`, `LoadModButton`, `ToolsButton`, `NewModButton` | `USE_DARWINIA_MOD_SYSTEM` | [B:DEAD] |
| `AboutDarwiniaWindow` | `OBERONBUILD` | [B:DEAD] |
| `DemoEndWindow`, `BuyOnlineButton`, `ExitDemoButton`, `ReturnToGameButton` | only created under `DEMO2` | [B:DEAD] (compiled, never instantiated) |
| `SaveOnQuitWindow` | after unconditional return; `SOUND_EDITOR` | [B:DEAD] |
| `GameMenuOptionsButton` (`code/UI/GameMenuOptionsButton.h`, P18) — opens `OptionsMenuWindow` | never instantiated | [B:LEGACY] |
| `game_menu.cpp` `#ifdef USE_SEPULVEDA_HELP_TUTORIAL` blocks in `CreateMenu` (delete tutorial/demo-end sequence) | macro undefined | [B:DEAD] |
| `game_menu.cpp` `#ifdef _DEBUG` font-spacing tweak block in `GameMenu::Render` (commented) | debug only, body commented | [B:DEBUG] |
| `GameMenu::CreateMapList` `.mwm` custom-map loading | `LOCATION_EDITOR` | [B:EDITOR] |
| `RestartWindowManagerAndRenderer` Linux branch ("Please restart Darwinia for the screen settings to take affect") | `TARGET_OS_LINUX` | [B:DEAD] platform |
| `PrefsKeybindingsWindow` Mac mouse-button drop-down; `GeneralHelpWindow` `_mac` help text; `PrefsOtherWindow` language control omitted on Mac | `TARGET_OS_MACOSX` | [B:DEAD] platform |
| `TARGET_OS_VISTA` branches (`g_mediaCenter` large menus, thumbnail save, extra mods dir) | macro undefined | [B:DEAD] |

<!-- from P18_ui_gamemenu -->
## Appendix — dead/legacy code
- **Testbed** (`TESTBED_ENABLED`, never defined) `[B:DEAD]`: `GameMenuWindow::UpdateTestBed/AdvanceTestBedGame/ReadTestBedConfig`, `StartTestbedButton`. Automates the menus by calling buttons' `MouseUp` (server: Main → Multiplayer → Host → mode → map → register as spectator → wait for >1 clients → auto-ready; client: Main → Multiplayer → Join → pick the server named after the testbed → spectator). Game table = every (mode, map) pair except mode index 6. Progressions `LINEAR` / `RANDOM` / `SEQUENCE`. Config file `testbedconfig.txt` commands: `SetProgression LINEAR|RANDOM|SEQUENCE`, `SetRandSeed <n>`, `SetTestBedName <s>`, `AddSequenceItem <index> <seed>`, `SequenceIDToTerminateOn <n>`. Delays: server game-over 10 s, client game-over 40 s, server-pick poll 3 s, connecting timeout 10 s, option delay 5 sequence ids, spectator/play delays 10 sequence ids. Uses blueprint data file `multiwinia-testbed.txt`.
- `#ifdef TESTBED` (a different, never-defined macro): `CSpectatorButton` column in the lobby `[B:DEAD]`.
- `FAKE_SERVERS` (commented-out `#define`, `NUM_FAKE_SERVER 100`): fabricates random browser entries `[B:DEAD]`.
- `TARGET_DEBUG` `RenderNetworkStatus`: 15 px, α 0.2 overlay on GameSetup/GameConnecting listing server clients "IP:Port [Sent/Known/CaughtUp]" and client connection state (Disconnected / Connecting (attempts) / Handshaking (processed/valid seq) / Connected) `[B:DEBUG]`.
- Xbox Live Arcade remnants: page slots 13, 14, 19, 20; members `m_xbla*`, `m_showingDeviceSelector`; `.vcproj` lists `UI/XBLASelectSessionType.h`, `UI/CustomGameTypeSelection.h`, `UI/InvitePlayersButton.h` which are absent from the tree `[B:DEAD]`.
- PDLC remnants: slots 21, 22; `.vcproj` lists absent `UI/DownloadPDLCButton.h`, `UI/PDLCOptionsButton.h`; comment in `SetupGameSelectPage` about showing PDLC/tank unlocks `[B:DEAD]`.
- Darwinia+ remnants: slots 26–30; `m_darwiniaPlus*`; `DarwiniaPlusSPButton`, `DarwiniaPlusMPButton` `[B:LEGACY]`.
- Darwinia single-player: `PageDarwinia`, `PageDemoSinglePlayer`, `PrologueButton` (`game_demo2.txt`, `GameModePrologue`, location `launchpad`, Sepulveda tutorial only under `USE_SEPULVEDA_HELP_TUTORIAL`), `CampaignButton` (`RequestBootloaderSequence`, `game.txt`, `GameModeCampaign`) `[B:DEBUG]`/`[B:LEGACY]`.
- Research page and `ResearchModeButton` (Darwinia research levels 0–4) `[B:LEGACY]`.
- Old help pages (`PageHelpMenu`, `PageHelp`, `HelpModeButton`, `HelpSelectButton`, `HelpImageButton`, `PrevHelpButton`, `NextHelpButton`) `[B:LEGACY]` — superseded by `HelpAndOptionsWindow`.
- Achievements and leaderboard pages, `AchievementButton` `[B:LEGACY]`.
- Quick match (`PageQuickMatchGame`, `QuickMatchButton`), quick start (`QuickStartButton`), rolling demo (`RollingDemoButton`), `MultiwiniaModeButton`, `RequestAIButton`, `GameMenuOptionsButton`, `MapDetailsButton`, `BuyNowInfoButton`, custom-game browsing (`AnyLevelSelectButton`, `GameTypeAnyButton`, `m_settingUpCustomGame`), `JoinServerAsSpectator`, `ApplyBetterSlowerServerListSort`, overlay error dialog (`RenderErrorDialogue`) `[B:LEGACY]`.
- `HIDE_INVALID_GAMETYPES`, `SPECTATOR_ONLY`, `PERMITTED_MAPS`, `MULTIPLAYER_DISABLED`, `MULTIWINIA_DEMOONLY`, HWCOMPAT/BETATEST/ASSAULT_STRESSTEST layout of the auth page and `QuitAuthkeyButton`: other product targets only.
- macOS: `RunSparkleButton`, pasteboard paste, Cmd-V in the key field.
- `LOCATION_EDITOR`: `MultiwiniaEditorButton`, custom-map name column, custom-map request flag in `GameServerButton::MouseUp` `[B:EDITOR]`.
- Files in `code/UI` outside the P18 list: `ErrorDialog.cpp/.h` (empty `DarwiniaWindow` subclass, not in the `.vcproj`) `[B:DEAD]`; `soundsliderbutton.h` (`VolumeControl` master-volume slider reading pref `SoundMasterVolume`, never included) `[B:DEAD]`.

<!-- from P19_help_tutorial_script -->
## Appendix — dead/legacy code
- [B:DEAD] `code/helpsystem.*`, `code/sepulveda.*`, `code/sepulveda_strings.cpp`, `code/tutorial.*`, `code/tutorial_demo1.cpp`, `code/tutorial_demo2.cpp` — entire files under `USE_SEPULVEDA_HELP_TUTORIAL`; documented in §4–§6 as reference designs.
- [B:DEAD] `Script::TestScript`, `Script::ReportError` (under `SCRIPT_TEST_ENABLED`): offline validator checking mount names, location names, phrase ids, building ids, `CamMove`/`Wait` durations 0.01–20 s.
- [B:DEAD] Attract mode (`code/attract.cpp`): `AttractMode::Advance` only called under `ATTRACTMODE_ENABLED`; would switch to profile `AttractMode`, lock the task manager, run `attractmode.txt` and stop on `ControlStopAttractMode` or script end (`endattract.txt`).
- [B:LEGACY] Script content and Darwinia triggers (ScriptTrigger, FenceSwitch, game.txt events, objective cutscenes, rocket victory) — §1.5–§1.6.
- [B:DEAD-in-effect, compiled] `MultiwiniaHelp` objective tabs rendering (`THIS_IS_FUCKING_SHIT_AND_IT_ALWAYS_HAS_BEEN`), `RenderControlHelp()`, `RenderBasicControlHelp()`, `RenderCurrentTaskHelp()`, `RenderCurrentUnitHelp()`, `HighlightingDarwinian()`, `GetHelpType()`, `GetResearchType()`, `GameTypeHelpWindow::GetLabelPos()`; `GameMenuWindow` `PageHelpMenu`/`PageHelp` (no entry button).
- [B:DEBUG] `CrateHelpWindow` (`INCLUDE_CRATE_HELP_WINDOW`), `mode_prologue.bmp`/`mode_campaign.bmp` buttons (`INCLUDEGAMEMODE_PROLOGUE/_CAMPAIGN`).
- Platform: `multiwinia_help_movetospawnpoint_mac`, `multiwinia_rmbhelp_mac`, `multiwinia_helppage_general%d_mac`, Mac `part_leftclick` defines — `TARGET_OS_MACOSX` only.
