# Collected "constants" sections from all drafts

<!-- from P02_game_modes -->
## 9. Constants table (all mode/rule constants in scope)

| Constant | Value | Kind | Where |
|---|---|---|---|
| Server tick | 0.1 s × game speed | code (`GetServerAdvancePeriod`) | `code/gametimer.cpp` |
| Mode second | 11 ticks | emergent (D1) | `code/multiwinia.cpp` |
| GRACE_TIME | 3.0 s (30 ticks) | `#define` | `code/lib/universal_include.h` |
| Grace decrement | 0.1 per tick | literal | `AdvanceGracePeriod` |
| Domination elimination protection | 30 game s after start | literal | `AdvanceSkirmish` |
| Domination elimination countdown | 30 mode s | literal (`m_eliminationTimer` reset 30) | `AdvanceSkirmish`, `Reset` |
| KOTH sudden-death extension | 60 mode s | literal | `AdvanceKingOfTheHill` |
| HailToTheKing margin | 1.5× second place | literal | `AdvanceKingOfTheHill` |
| Hoarder margin | > 2× second place and > 3 | literal | `AdvanceCaptureStatue` |
| CTS Futurewinian/Evilinian fallback win | > 50 entities | literal | `AdvanceCaptureStatue` |
| Statue spawn timer | `max = 60·S/Z`; `max/2 + U(0,max/2)`, min 10 mode s | literal | `AdvanceCaptureStatue` |
| Statue spawn min distance to nearest statue | > 30 units | literal | `AdvanceCaptureStatue` |
| CTS delivery radius / check period | 30 units / 2.0 game s | literal | `MultiwiniaZone::Advance_CaptureTheStatue` |
| Statue lifters min/max | 20 / 50 | ctor | `Statue::Statue` |
| Statue carry radius | 33 | literal | `Statue::Initialise` |
| Statue ownership count radius / period | 50 units / 1.0 game s | literal | `CarryableBuilding::CalculateOwnership` (P06) |
| Statue destroy tilt / water | `up.y < 0.7` / `y < 0` | literal | `Statue::Advance` |
| Statue shapes | 8 | literal | `Statue::Initialise` |
| KOTH zone default size | 50 | ctor | `MultiwiniaZone::MultiwiniaZone` |
| Zone recount period | 1.0 game s | literal | `MultiwiniaZone::Advance_*` |
| KOTH "IsEliminated" Darwinian floor | < 50 Darwinians, 0 spawn points | literal | `Multiwinia::IsEliminated` |
| Blitzkrieg flag count radius | 40 units | literal | `Advance_Blitzkrieg` |
| Blitzkrieg capture rate | `5·top·(top/total)` × 0.1 per recount | literal | `Advance_Blitzkrieg` |
| Blitzkrieg neutral decay | 10 × 0.1 per recount | literal | `Advance_Blitzkrieg` |
| Blitzkrieg ownership range | 0–100 | literal | `Advance_Blitzkrieg` |
| Base-under-attack message throttle | 20 game s | literal | `Advance_Blitzkrieg` |
| Blitzkrieg time-out score | flags + 10 × eliminations | literal | `AdvanceBlitkrieg` |
| BlitzMaster window | 300 (see text) | literal | `AdvanceBlitkrieg` |
| `BLITZKRIEG_FLAGCAPTURE_RANGE` | 20.0 | `#define` | `multiwiniazone.h` (used by `darwinian.cpp`, P03) |
| `BLITZKRIEGSCORE_*` | 10000–10005 | `#define` | `multiwinia.h` |
| Flag raise offset | 50 + (100 − ownership) units | literal | `RenderBlitzkrieg` |
| Rocket win height | world y > 600 | literal | `AdvanceRocketRiot` |
| UncladSkies thresholds | damage ≤ 0.01; others' fuel ≤ 50 | literal | `AdvanceRocketRiot` |
| Rocket refuel→loading / loading→ignition | fuel ≥ 95 / fuel > 95 and passengers ≥ 100 | literal | `EscapeRocket::AdvanceRefueling/Loading` |
| Rocket refuel EMA | 0.99 / 0.01 per tick; rate = pipes × level × 0.4 | literal | `EscapeRocket::Refuel` |
| Fuel cap | 100 | literal | `Refuel` |
| FuelBuilding level EMA | 0.2×dt (×3 when decreasing) | literal | `FuelBuilding::ProvideFuel` |
| Fuel pipe "flowing" threshold | level > 0.1 (counts a pipe), sound at > 0.2 | literal | `EscapeRocket::ProvideFuel`, `FuelPipe::Advance` |
| FuelGenerator max surges | 20 MP / 10 SP (render 15/10) | literal | `FuelGenerator::Advance/Render` |
| Surge decay | 1 per game s | literal | `FuelGenerator::Advance` |
| Attract-attackers threshold / range / period / count | fuel > 33 / 150 units (300 SP) / 1+U(0,2) s / 1 + rand%(1+0.05·n) | literal | `SetupAttackers` |
| Rocket damage multiplier (MP) / decay | ×0.2 / 0.33 per game s | literal | `EscapeRocket::Damage/Advance` |
| Rocket explode threshold | damage > 100 | literal | `Damage` |
| Under-fire sound threshold | damage ≥ 75 → Malfunction | literal | `SetupSounds` |
| Explosion: damage decay / extra / fuel loss / fuel floor | 1/s / +2/s when empty / 5/s / 50% of pre-explosion fuel | literal | `AdvanceExploding` |
| Ignition countdown (SP) / Ready (MP) / Countdown | 18 (forced 0 in MP) / 10 s / 10 at half speed | literal | `AdvanceIgnition/Ready/Countdown` |
| Flight thrust | `max(2√(y−ground), 0.1)` u/s | literal | `AdvanceFlight` |
| Camera shake on ignition | 5.0 decaying 1/s | literal | `AdvanceIgnition`, `Advance` |
| Fuel station recruit radius | 300 units, 1 candidate per tick | literal | `FuelStation::Advance` |
| Rocket passengers required | 100 | literal | `AdvanceLoading`, `Darwinian::AdvanceBoardingRocket` |
| Trunk port spawn offset | up 50, random radius ≤ 40 | literal | `AdvanceTrunkPorts` |
| Trunk port exit speed | 20 + U(0,15) | literal | `AdvanceTrunkPorts` |
| Trunk port armour target | front × 75, ground + 5 | literal | `AdvanceTrunkPorts` |
| Default ReinforcementTimer fallback | 60 | literal | `AdvanceTrunkPorts` |
| Default MaxTurrets fallback | 2 | literal | `AdvanceTrunkPorts` |
| Trunk port surface grid | 16 / detail, 90 units | `#define`/literal | `trunkport.h/.cpp` |
| Pulse bomb scale | 0.75 | ctor | `PulseBomb::PulseBomb` |
| Pulse bomb timer (official maps) | 900.00 s | data | `data/levels/mp_assault_*.txt` |
| Pulse bomb warnings | ≤ 60 (message), ≤ 10 (3D countdown) | literal | `PulseBomb::Advance/RenderCountdown` |
| Pulse wave start radius / growth | 200 / ×1.03 per tick | literal | `PulseBomb::Advance`, `weapons.cpp:PulseWave::Advance` |
| Pulse bomb destruction bang | `Bang(pos, 75, 5)` | literal | `PulseBomb::Destroy` |
| Assault victory delay | 10 s single / 15 s multi | literal | `RecordDefenseTime` |
| Assault next-round text shown | victory timer < 11 | literal | `Render` |
| Control station capture threshold | > 2 occupants (5 ports) / > 1 (3 ports, bomb-linked) | literal + data | `ControlStation::RecalculateOwnership`, shapes |
| Retribution interval | 45 game s (first 5 if never reset) | literal | `AdvanceRetribution`, `Reset` |
| One-minute warning | `0 < remaining ≤ 60` | literal | `Advance` |
| Carnage | ≥ 2000 kills | literal | `Advance` |
| Genocide | ≥ 1,000,000 total kills | literal | `AchievementTracker::AddToTotalKills` |
| Achievement levels | ids ≤ 49; 49 required (41 without Assault) | `#define` | `achievement_tracker.cpp` |
| WrongGame radius | 120 units | literal | `nuke.cpp` |
| HUD reference height | 800 px | literal | `Render` |
| Scoreboard box | 440 (530 RR) × (50 + rows·18.2) at (10,10) | literal | `Render` |
| Rocket panel | w = 0.2·ScreenH, h = 1.5w, at (ScreenW − w − 10, 10) | literal | `RenderRocketStatusPanel` |

---

<!-- from P03_entity_core -->
## Constants table (P03 scope)
| Constant | Value | Kind | Ref |
|---|---|---|---|
| Server tick | 0.1 s × gameSpeed | code | `gametimer.cpp:GetServerAdvancePeriod` |
| `COEF_OF_RESTITUTION` | 0.85 | #define | `worldobject.cpp` |
| `ID_MAXTEAMS / ID_MAXUNITS / ID_MAXTROOPS` | 256 / 65536 / 65536 | #define (debug asserts) | `worldobject.cpp` |
| `UNIT_BUILDINGS/SPIRITS/LASERS/EFFECTS` | −100/−101/−102/−103 | #define | `worldobject.h` |
| Gravity (Entity in air) | 15 u/s² | literal | `Entity::AdvanceInAir` |
| Landing epsilon | ground + 0.1 (+0.1) | literal | `Entity::AdvanceInAir` |
| Drown time (Entity) | 10 s (Officer effectively 5 s) | literal | `Entity::AdvanceInWater`, `Officer::Advance` |
| Swim speed | 5 u/s; bob `−sin(3t) − 4` | literal | `Entity::AdvanceInWater` |
| Death fade | −40·dt on ground, −20·dt in air from 100 | literal | `Entity::AdvanceDead` |
| Laser speed / aim jitter / muzzle height | 150 / ±7.5 / +3 | literal | `Entity::Attack` |
| Teleport entry range | RadarDish 15 (to entrance), Bridge 25 (to pos) | literal | `Entity::EnterTeleports` |
| Cliff gradient limit | 1.0 (search 0..50 step 0.2) | literal | `Entity::PushFromCliffs` |
| Water push spiral | Entity 0..50 step 1; Darwinian 40..95 step 5; dry if height > 1 | literal | `PushFromObstructions` |
| Building push | Entity 4.0 steps while sphere(2.0); Darwinian: other 30 trigger / 4 step / 2 test; wall 4 trigger / 5 step / 1 test; fence `5 + uid%10` / 20 step | literal | `PushFromObstructions` |
| Separation | radius 2.0, force 0.1/d | literal | `Entity::PushFromEachOther` |
| Ray-pick sphere (sprites) | 5.0 | literal | `Entity::RayHit` |
| Shape colour scale | ×0.7; darwinian.shp colourId 0 ×0.5; "red" test r > 2g & r > 2b | literal | `Entity::ConvertFragmentColours` |
| `m_routeTriggerDistance` | 10.0 | member default | `Entity::Entity` |
| Entity grid cell | 32 × 32 | ctor arg | `Location::Init` |
| Obstruction grid cell | 64 (SP) / 32 (MP) | ctor arg | `Location::Init` |
| `END_OF_LIST` | −100000 | #define | `entity_grid.cpp` |
| `INITIAL_OBJECT_ID_ARRAY_SIZE` | 0 (then 2, ×2) | #define | `entity_grid.cpp` |
| `ENTITYGRIDCACHE_MAXAGE` | 1.0 s | #define | `entity_grid_cache.h` |
| Cache container | size 20, step 20 | literal | `EntityGridCache::EntityGridCache` |
| Route graph edge max | 600 | literal | `LandRoutingSystem::Initialise` |
| Corridor half-width | 40 | literal | `RoutingSystem::GenerateRoute` |
| Smoothing step / merge distance | 50 / 50 | literal | `GenerateRoute` |
| Max route waypoints | 100 | literal | `GenerateRoute` |
| Route fade | 0.25 /s | literal | `RoutingSystem::Render` |
| IsWalkable step / water / slope / normal | 20 / 40 / 2.4 / 0.8 | literal | `Location::IsWalkable` (P01) |
| GetWalkingDistance step / uphill factor | 40 / 1.5 | literal | `Location::GetWalkingDistance` (P01) |
| Darwinian ranges | see `#define` table | #define | `darwinian.h` |
| Darwinian speed factors | Idle 0.2, UnderControl 1.75, slope [0.3, 2.0], arrival < 10 | literal | `Darwinian::AdvanceToTargetPosition` |
| Darwinian turn blend | 0.4 per tick (`dt × 4`) | literal | same |
| Retarget interval | 1 + U[0,1) s | literal | `Darwinian::Advance` |
| Attack-rate divisor | 10 / 7 booster / 3 futurewinian | literal | `AdvanceCombat` |
| Grenade timer | 12 + U[0,8) s (first U[0,5)) | literal | `AdvanceCombat`, ctor |
| Grenade conditions | dist > 75, normals > 0.7, heightDiff > −50, target < 40 above ground, enemies > 5 & friends < 2 within 50 | literal | `AdvanceCombat` |
| Combat keep-distance | 20 + U[0,20); retreat inside 100 unless 25 below; flee step 30 ±45° | literal | `AdvanceCombat` |
| Fear | scared if > 5 enemies & no friends; forget beyond 200 | literal | `SearchForThreats`, `AdvanceCombat` |
| Burn damage | −2 hp with p = 0.2/gameSpeed per tick | literal | `AdvanceOnFire` |
| Fire-damage ignition | 100 accumulated | literal | `FireDamage` |
| Explosive damage in formation | ×0.6; rage ×0.75; futurewinian ×2 | literal | `ChangeHealth` |
| Power-up speed caps | hot feet ×2 if ratio < 3.5; rage ×1.5 if < 3; plague ×0.75, /2 | literal | `GiveHotFeet`, `GiveRage`, `Infect`, `AdvanceInfection` |
| Plague | interval 5 + U[0,5); spread 30, 50 %; outcomes 1/20 each (0–4), 2/20 immune | literal | `AdvanceInfection` |
| Paralysis | 5 + U[0,10) s | literal | `AdvanceInfection` |
| Port operator limit per team | SP 20 / MP 1 | literal | `SearchForPorts` |
| Carry lifting tolerance | 20 × gameSpeed × speedRatio | literal | `AdvanceCarryingBuilding` |
| Armour boarding | range 150, board < 20; eject `20 + U[0,20)`; unload `dir×10`, `vy 5 + U[0,10)` | literal | `AdvanceApproachingArmour`, `AdvanceInsideArmour` |
| Spectacle distance | 200 SP / 100 MP (+ U[0, min/2)) | literal | `AdvanceWatchingSpectacle` |
| Rocket passengers max | 100 | literal | `AdvanceBoardingRocket` |
| Vortex | speed max(100·y/1000, 30), radius 300·y/1000, max height 500 + 5·(uid%100), fling 50 | literal | `AdvanceInVortex` |
| Abduction | toward-ship 2×speed, tangent 50, blend 0.25·dt + (uid%10)·0.1·dt, capture < 20 | literal | `AdvanceBeingAbducted` |
| Officer orders effect cadence | MP 2.0 s; SP 5 %/tick | literal | `Officer::Advance` |
| Formation toggle radius | 50 | literal | `Officer::IsFormationToggle` |
| Formation slot keep-alive | 5.0 s initial, +dt per register | literal | `GetFormationPosition`, `RegisterWithFormation` |
| Formation spacing / first rank | 6 × 6 / 12 ahead | literal | `GetFormationPositionFromIndex` |
| Formation max size | 90 (`Team::m_evolutions[EvolutionFormationSize]`) | team default | `code/team.cpp:Team::Team` |
| Officer speed factors | Idle 0.2, formation 0.4, slope [0.1, 2.0] | literal | `Officer::AdvanceToTargetPosition` |
| Officer route rebuild | > 50 from route start | literal | `Officer::Advance` |
| Officer AI | think 1 s / 10 s (Goto); re-order if > 60; threat 270, > 10 enemies | literal | `Officer::RunAI` |
| Spirit timings | Birth 6 s; Floating 120 ± 30 s (MP ÷3); Death 180 s (MP ÷2) | literal | `Spirit::Begin/Advance` |
| Spirit motion | drag (1 − dt)/tick; hover rates [0,2]; floor ground+2; building push sphere 5, speed ≤ 10 | literal | `Spirit::Advance`, `PushFromBuildings` |
| `SPIRIT_MAXNEARBYEGGS` | 8 | #define | `spirit.h` |
| Spirit render | inner 1.0/255, outer 3.0/100 (MP halved) | literal | `Spirit::RenderWithoutGlBegin` |
| `OFFICER_ATTACKRANGE` / `OFFICER_ABSORBRANGE` | 10 / 10 | #define | `officer.h` |
| MP research levels | all 3; Squad 1; Laser 2; Grenade 2; Rocket 2; AirStrike 2; Darwinian 4 | code | `Location::Init` (P01) |
| Laser life by level | 0.2/0.4/0.6/0.8/1.0 s (×2 subversion) | code | `Location::FireLaser` (P01) |

<!-- from P08_crates_ai -->
## 18. Constants table

| Name | Value | Kind | Ref |
|---|---|---|---|
| `CRATE_START_HEIGHT` | 1300.0 wu (+ U[0,200)) | `#define` | `crate.h`; `location.cpp:CreateRandomCrate` |
| `CRATE_SCAN_RANGE` | 35.0 wu | `#define` | `crate.h` |
| `CRATE_CAPTURE_TIME` | 2.5 (progress units) | `#define` | `crate.cpp` |
| capture step cap | 0.05 per tick | literal | `Crate::AdvanceOnGround` |
| capture rate | `num/100` per game-s | literal | same |
| capture decay | 1.0 per game-s | literal | same |
| crate life (neutral, landed) | 60.0 s | literal | `Crate::Crate`, `AdvanceOnGround` |
| landing height | ground + 20 | literal | `AdvanceFloating` |
| fall speed | 40 wu/s | literal | same |
| sway amplitude | 300 × y/1300 wu (X only) | literal | same |
| crate spacing (random drop) | 350 wu (= 35×10) | expr | `CreateRandomCrate` |
| weighted drop offset | ±100 wu (`syncsfrand(200)`) | literal | same |
| random-drop min ground height | 20 wu | literal | same |
| random-drop attempts | 101 (fail after 100) | literal | same |
| weighted-drop attempts | 1000 | literal | same |
| max crates | `int(worldSizeX/500)` | expr | `AdvanceCrates` |
| drop interval | `CrateDropRate × (1.5 − U[0,1))` s | data+literal | same; `data/multiwinia.txt` |
| `CrateDropRate` default/range | 40 s / 0–300 step 10 | data | `data/multiwinia.txt` |
| good/bad split | 0.9 / 0.1 | literal | `AdvanceOnGround` |
| CrateMania invalid at | `s_crates ≥ 4` | literal | `IsValid` |
| AntNest limit | 4 (nests + 2×pending tasks) | literal | `IsValid` |
| `POWERUP_EFFECT_RANGE` | 70.0 wu | `#define` | `taskmanager.h` |
| Darwinian-powerup task life | 30 s | literal | `Task::Start` |
| Squad task life | 60 s | literal | `Task::Start` |
| StartingPowerups lock | 30 s | literal | `Location::InitialiseTeam` |
| MP task capacity | 6 (2 eliminated) | literal | `TaskManager::Capacity` |
| SpawnMania | 5.0 s | literal | `RunSpawnMania` |
| Mega Rage (Bitzkreig) | 15.0 s | literal | `RunBitzkreig` |
| SlowDown target / rate / hold | 0.2–0.4 / 0.05 / 15.0 | literal | `RunSlowDownTime` |
| SpeedUp target | 1.3–2.0 | literal | `RunSpeedUpTime` |
| Airstrike markers | 3–7 (10 tutorial), ±50 | literal | `RunAirStrike` |
| Nukes | 10, spread ±150 | literal | `RunNuke` |
| Darwinians reward | 50–99 | literal | `RunDarwinians` |
| Meteors | 5–9, ±210, y 1500–2000 | literal | `RunMeteorShower` |
| Eggs | 2–4, speed 50–100, timer 4–8 s, roam 300 | literal | `RunEggs` |
| Magical trees | 1–2, ±50 | literal | `GenerateTrees` |
| Evil trees | 2–4, ±50 | literal | `GenerateEvilTrees` |
| Ant hills | 2, ±100, 51–75 ants | literal | `RunAntNest` |
| Triffids | 1–3, ±25, reload 40–100 s | literal | `RunTriffids` |
| Infection particles | 100, speed 100 | literal | `RunInfection` |
| SpaceShip spawn y | 5000 | literal | `RunSpaceShip` |
| CrateMania crates | 5–9 | literal | `RunCrateMania` |
| `DARWINIAN_SEARCHRANGE_CRATES` | 150 wu | `#define` | `darwinian.h` |
| `AITARGET_LINKRANGE` | 600 wu | `#define` | `ai.h` |
| link cull factor | 0.8 | literal | `Location::RecalculateAITargets` |
| AITarget recount period | 2.0 s | literal | `AITarget::Advance` |
| AITarget range | SP 100 / MP 200 / zone size | literal | `AITarget::GetRange` |
| important-building radius | 100 wu | literal | `AITarget::IsImportant` |
| NeedsDefending enemy threshold / officer radius | 30 / 300 wu | literal | `AITarget::NeedsDefending` |
| Standard priorities | 0.95 grab, 0.8 enemy, 1.0 attack/defend, own = mean/2, +0.35 radar, +0.01/neighbour | literal | `RecalculatePriorityStandard` |
| KOTH zone override | 0.95 if > 10 non-own in zone | literal | same |
| CTS statue override | 1.0 if not held / lifters < max / > 10 enemies | literal | `RecalculatePriorityCTS` |
| crate AITarget modifier | 0.5 | literal | `Crate::AdvanceOnGround` |
| statue / fuel-station AITarget modifier | 1.0 | literal | `statue.cpp`, `rocket.cpp` |
| stray recall fraction | 2 % of `m_others` per tick (≥ 1) | literal | `AI::Advance` |
| stray recall distance | 70 wu; offset radius 20 + U[0,20) (Blitz 0 + U[0,20)) | literal | same |
| well-defended idle limit | 10 (×3 for Assault attacker radar target) | literal | `AdvanceMultiwiniaAI` |
| noAttack ratio | enemy < 0.33 × friend | literal | same |
| sendChance | clamp(1.5 × enemy/idle, 0.6, 1.0); 1.1 overrides | literal | same |
| gather radius around target | 150 wu | literal | same |
| spawn-point association radius | 200 wu | literal | same |
| score-zone garrison | 10 (neutral important dest) / 50 | literal | same |
| officer creation | friend ≥ 70 (CTS idle ≥ 150), no officer within 300, promote within 100 | literal | `AdvanceOfficerAI` |
| Goto-officer spawn waypoint | spawn front × 60 | literal | `AdvanceMultiwiniaAI` |
| radar retarget period | 10 s; transit limit 20 | literal | `AdvanceRadarDishAI` |
| AIObjective period | 5.0 s | literal | `AIObjective::Advance` |
| AIObjectiveMarker period | 2.0 s; default scanRange 100; bind radius 20 | literal | `AIObjectiveMarker` |
| marker capture rule | > 4× leader while leader < 10 (attack); 2× / 15 defenders (defence) | literal | `AdvanceStandard/AdvanceDefensive` |
| Assault commit radius (defender) | 450 wu | literal | `AdvanceAssaultAI` |
| AI difficulty periods | 20/10/1 s (Assault 3/3/1) | literal | §14 |
| Marker sizes | icon H/20, shadow +10, compass +H/30; offset 50 px | literal | `MarkerSystem::Render` |
| Marker fades | seen 1 s; fade 6→15 s; non-persistent 6→8 s; fixed removed at 8 s on-screen | literal | same |
| Marker flash | 3 s, 0.4 s period | literal | same |
| Message duration | 3.0 s real time | literal | `Location::SetCurrentMessage` |

<!-- from P11_terrain_world -->
## Constants table (this partition)

| Constant | Value | Kind | Where |
|---|---|---|---|
| Tile defaults: outsideHeight, size, guideGridPower, method, desiredHeight, fractalDimension, heightScale, lowlandSmoothing, seed | 0.0, 256, 0, 1, 200, 1.2, 1.0, 1.0, 1 | ctor | `code/landscape.cpp:LandscapeTile::LandscapeTile` |
| `LandscapeDef` defaults: cellSize, worldSizeX/Z, outsideHeight, maxHeight | 12.0, 2000, 2000, −10, 0.0 | ctor | `code/level_file.h:LandscapeDef` |
| `Landscape::m_outsideHeight` before Init | −20 | ctor | `code/landscape.cpp:Landscape::Landscape` |
| Flatten defaults: size, type, threshold | 40.0, Absolute, 40.0 | ctor | `code/level_file.h:LandscapeFlattenArea` |
| LCG multiplier / increment / output mask | 214013, 2531011, `(s>>16)&0x7FFF` | literal | `LandscapeTile::LandscapeAppRandom`, `AppRandom` |
| `APP_RAND_MAX` | 32767 | `#define` | `contrib/systemiv/lib/math/random_number.h` |
| Noise: length scale, length mult, envelope floor, envelope gain | 256.0, 10.0, 0.1, 0.15 | literal | `GenerateNoise` |
| Compensation: 30.7·e^(−6.5·fd), 15.353·e^(−3.1·ls) | 30.7, −6.5, 15.353, −3.1 | literal | `Generate` |
| Merge range epsilon | 0.001 | literal | `MergeTileIntoLandscape` |
| Smooth divisor / passes | 6.0 / 2 | literal | `FlattenArea` |
| `LIGHTMAP_SCALEFACTOR` | 1 (unused) | `#define` | `code/landscape.h` |
| Render: near-sea sink threshold / depth | 0.3 → −10.0 | literal | `BuildVertArrayAndTriStrip` |
| Colour: slope exponent, jitter numerator, jitter offset | 0.4, 0.45, +2.0 | literal | `GetLandscapeColour` |
| Burn patch: outer radius factor, inner factor, colour | 1.5·r, r/2, (20,20,20) | literal | `BuildColourArray` |
| Main material spec/diffuse/shininess | (0,0,0,0)/(1,1,1,0)/100 | literal | `RenderMainSlow` |
| Overlay material spec/diffuse/shininess | (0.5,0.5,0.5,1)/(1.2,1.2,1.2,0)/40 | literal | `RenderOverlaySlow` |
| Display list names | `"LandscapeMain"`, `"LandscapeOverlay"`, `"GlobalInternetLinks"`, `"GlobalInternetNodes"` | `#define` | renderer, global_internet |
| Water: brightness scale, shore brightening, shore noise | 4.0, 250.0, 0.25 | `float const` | `code/water.cpp` |
| Water mask size / blur taps / gain | 128 / 11 `{0.2,0.3,0.4,0.5,0.8,1.0,0.8,0.5,0.4,0.3,0.2}/5.4` / 855 | `#define`/literal | `GenerateLightMap` |
| Flat water: tile ratio, height, texture repeats, scroll period | 4, −9.0, 30, 30 s | literal | `GenerateLightMap`, `RenderFlatWater` |
| Water cell | `RenderWaterDetail × max(sizeX,sizeZ)/100` | pref-driven | `Water::Water` |
| IsVertNeeded thresholds | land > 4.0, depth > 0.999 | literal | `IsVertNeeded` |
| Wave amplitude scale | 7.0 | literal | `UpdateDynamicWater` |
| Wave X terms (freq, speed, amp) | (0.01, 0.65, 1.2), (0.03, 1.5, 0.9) | literal | same |
| Wave Z terms | (0.02, 0.75, 0.9), (0.03, 1.85, 0.65) | literal | same |
| Wave colour mix | 0.7 / 0.3 | literal | same |
| Waves table row / width | y = 1 / bitmap width (400 shipped) | literal | `Water::Water` |
| D3D water buffers / thread rate | 10 / 120 Hz (thread never started) | `static const` | `water.h`, `UpdateWaterThread` |
| Reflection RT size / clear grey / clip plane | ½ screen / 0.3 / `GL_CLIP_PLANE2 (0,1,0,0)` | literal | `water_reflection.cpp` |
| Reflection shader: ripple amplitude, seam offset, blend | `0.001·1024/ScreenW`, 0.004, 0.7 | literal / shader | `WaterReflectionEffect::Start`, `water-meshwaves.ps*` |
| Clouds initial / advance velocity | (0.04,0,0) / (0.03,0,−0.01) per second of `SERVER_ADVANCE_PERIOD` | literal | `Clouds::Clouds`, `Advance` |
| Cloud layer heights / extents / detail | 1200, 1000, 800 / [−8000, 9000] / 9, 4.5, 1.8 | literal | `RenderBlobby`, `RenderFlat` |
| Cloud colours | (0.7,0.7,0.9, 0.6 blobby / 0.3 flat / 0.5 flat at detail 3) | literal | same |
| Sky grid: extent, spacing, half width, height, colour | [−4000, 6000], 80, 8, 1200, (0.5,0.5,1.0,0.3) | literal | `RenderSky` |
| Sky/cloud fog | black; 2000–4000 sky, 2000–5000 clouds | literal | `RenderSky`, `RenderBlobby`, `RenderFlat` |
| `RenderQuad` subdivision | 4 × 4 | literal | `Clouds::RenderQuad` |
| Face: height, half size, delay, fade, end, frame time | 800, 800, 5 s, 1 s, 28/30 s, 0.4 s | literal | `RenderFace` [B:DEBUG] |
| Location fog | start = end/3, end = max(4000, size+1000) | literal | `Location::SetupFog` |
| Global internet: iterations, max nodes, max links, centre | 7, 13701, 13700, (−797,1949,−1135) | `#define`/literal | `global_internet.h/.cpp` |
| Global internet render: scale, fog end, node size, packet size, burst, spawn prob | 1000, 5 810 000, 10, 30, 4.0 s, 11 % per frame | literal | `GlobalInternet::Render`, `RenderPackets` |
| Global world: location scale, hit radius, attract radius | 120, 5000, 10000 | literal | `GetLocationPosition`, `LocationHit`, `Advance` |
| Research thresholds / time per point / control-tower points | {1,50,100,200} / 10 s / 22 | literal / `#define` | `GlobalResearch`, `global_world.h` |
| SP level-time default | 3 599 999 | literal | `GlobalWorld::GlobalWorld`, `WriteLeaderboardStats` |
| Level file limits | mount/anim name 63, filename 512, route id < 10000, anim duration < 60 s, running program positions 10 | `#define`/literal | `level_file.h/.cpp` |
| `MAGIC_MOUNT_NAME_START_POS` | `"CamPosBefore"` | `#define` | `level_file.h` |
| InstantUnit default spread | 200.0 | ctor | `level_file.h:InstantUnit` |
| Difficulty factor | `1 + difficulty/5` (difficulty forced 0) | literal | `ParseBuildings`, `ParseInstantUnits` |
| Map CRC buffer | 100 000 bytes (+1 overflow) | `#define MAXSIZE` | `filecrc.cpp` |
| CRC-32 polynomial / init / xorout | 0xEDB88320 / 0 / none | literal | `contrib/unrar/crc.cpp` |
| MapFile keys | `MapData`, `MapLandscapeTexture`, `MapWavesTexture`, `MapWaterTexture`, `MapThumbnail`, `TextureWidth<n>`, `TextureHeight<n>`, `<key>Filename`; zlib level 9 | `#define`/literal | `mapfile.h/.cpp` |

<!-- from P12_render_core -->
## Constants table (this partition)

| Constant | Value | Kind | Ref |
|---|---|---|---|
| `PIXEL_EFFECT_GRID_RES` | 16 | `#define` | `code/renderer.h` |
| `USE_PIXEL_EFFECT_GRID_OPTIMISATION` | 1 (GL and D3D) | `#define` | `code/renderer.cpp` |
| `m_pixelSize` | 256 px | member init | `Renderer::Renderer` |
| pixel-effect cutoff | 1000 world units | literal | `PreRenderPixelEffect` |
| pixel scale | `(0.017 + 0.000037*fov)*fov` | literal | `PaintPixels` |
| near/far in location | 5 / 15000 | literal | `RenderFrame` |
| near/far outside location | 50 / 10 000 000 | literal | `RenderFrame` |
| near/far ctor default | 5 / 150000 | member init | `Renderer::Renderer` |
| laser near multiplier | 1.2 | literal | `Location::RenderWeapons` |
| Virii near multiplier | 1.05 | literal | `Team::RenderVirii` |
| poster FOV reference | 78° | literal | `SetupProjMatrixFor3D` |
| no-location clear colour | (0.05, 0, 0.05, 0.1) | literal | `RenderFrame` |
| default alpha ref | GREATER 0.01 | literal | `SetOpenGLState` |
| unit alpha ref / Darwinian alpha ref | 0.02 / 0.04 | literal | `Team::Render`, `RenderDarwinians` |
| fog end | max(4000, worldX+1000, worldZ+1000); start = end/3 | literal | `Location::SetupFog` |
| sky fog | 2000–4000; clouds fog 2000–5000 | literal | `code/clouds.cpp` (P11) |
| fade clamp | dt ≤ 0.05 s, rate ±1/s | literal | `RenderFadeOut` |
| lost-connection banner delay | 5.0 s | literal | `RenderFrame` |
| menu transition speed | `f*1.2*3`, shrink 0.2, slide `fraction^6` | literal | `RenderMenuTransition` |
| Darwin logo timings | 3 s in, hold to 8 s, out by 10 s | literal | `RenderFrame` |
| `m_gamma` | 1.2 (D3D ramp `pow(i/255, 2-gamma)`) | member init | `Renderer::SetScreenGamma` |
| particle initial array / growth | 1500 / 200 | literal | `ParticleSystem::ParticleSystem` |
| particle alpha | 90/255, fade over last 25 % of life | literal | `Particle::Render` |
| particle billboard half-size | size/16 | literal | `Particle::Render` |
| bounce restitution term | 0.7 | literal | `Particle::Advance` |
| debris smoke spawn prob/tick | `0.5*dt*10` | literal | `Particle::Advance` |
| explosion constants | see Explosions table | `#define` | `code/explosion.cpp` |
| `DEFORM_PRECISION` | 10 | HLSL `#define` | `data/shaders/globals.h` |
| deform lifetimes | punch 1 s, tear 1 s | table | `deformParams` |
| deform sprite size cap | 0.4 (screen fraction) | literal | `DeformEffect::RenderSprite` |
| water ripple amplitude | `0.001*1024/ScreenW` | literal | `WaterReflectionEffect::Start` |
| water blend | `0.7*col + 0.7*(r+g+b)*refl`, a 0.7; seam +0.004 | HLSL | `water-meshwaves.ps*` |
| fixed-function specular power | 16 | HLSL | `FixedFunc0.vs` |
| shape colour scale | 0.85 | literal | `ShapeFragment::ParseColourBlock` |
| team recolour scale | 0.7 (armour 0.1; green Darwinian ×0.5) | literal | `code/worldobject/entity.cpp`, `armour.cpp` |
| "mostly red" test | `r > 2g && r > 2b` | literal | `Entity::ConvertFragmentColours` |
| max fragments / markers per shape | 100 / 100 | literal (debug assert) | `Shape::Load` |
| text margins | `TEX_MARGIN 0.003`, `TEX_STRETCH 0.922`, `TEX_WIDTH ≈0.05187` | `#define` | `code/lib/text_renderer.cpp` |
| text scale factors | 0.6 (default), 1.0 (CJK langs); spacing editor 0.91, title 1.03 | literal | `code/app.cpp:App::SetLanguage` |
| `DEF_FONT_SIZE` | 12.0 | `#define` | `code/lib/text_renderer.h` |
| `STARTING_FONT_ARRAY_SIZE` | 8 (doubles) | `#define` | `code/lib/text_renderer.cpp` |
| text 2D ortho offset | −0.325 px | literal | `BeginText2D` |
| blur kernel | `[2,4,7,4,2]*scale*0.0526`, vertical ×2 | literal | `BitmapRGBA::ApplyBlurFilter` |
| pink key | (255,0,255) → (128,128,128,0) | literal | `ConvertPinkToTransparent` |
| colour match radius | 10 | literal | `ConvertColour` |
| debug sphere | icosahedron ×1 subdivision = 80 tris | literal | `Sphere` |
| persisting debug item label | 64 chars, initial 100 items | literal | `PersistingDebugRenderer` |
| Sierpinski | 5-point pyramid size 20, point size 3 | literal | `Sierpinski3D` |
| Pokey registers | 0xD200 + 2ch (freq), 0xD201 + 2ch (ctrl<<5 \| vol>>4), gain 4 | literal | `EffectProcessor::Advance` |

<!-- from P13_gfx_backend_math -->
## Constants table (this partition)

| Constant | Value | Kind | Ref |
|---|---|---|---|
| `MAX_ACTIVE_TEXTURES` | 2 | `const int` | `opengl_directx.cpp` |
| Initial immediate-mode vertex capacity | 1024 vertices (×2 growth) | literal | `InitialiseData`, `reallocateVertexList` |
| `CustomVertex` stride | 44 bytes | struct | `opengl_directx_internals.h` |
| Mip worker CPU affinity mask | `0x01|0x04|0x10|0x40` (CPUs 0,2,4,6) | literal | `SetWorkingThreadProcessor` |
| Lost-device poll interval | 100 ms | literal `Sleep(100)` | `WaitAndResetDevice` |
| FPS counter window | 5000 ms | literal | `Direct3DSwapBuffers` |
| `gluPerspective` z-translation factor | `_43 *= 2.0f` | literal | `gluPerspective` |
| `gluOrtho2D` depth range | −1 … +1 | literal | `gluOrtho2D` |
| Clear depth / stencil | 1.0 / 0 | literal | `glClear` |
| Default clear colour | RGBA(0,0,0,255) | literal | `InitialiseData` |
| Alpha-ref quantisation | ×255, truncated | literal | `glAlphaFunc` |
| Colour float→byte | ×255 truncation, clamp [0,1] | macro `CLAMPF2B` | `opengl_directx_inline.h` |
| Fog range when fog disabled | (100000, 1000) | literal | `FixedPipeline::Bind` |
| Specular exponent | 16 | HLSL literal | `FixedFunc0.vs` |
| Normal epsilon | (0, 0.0001, 0) added before normalise | HLSL literal | `FixedFunc0.vs` |
| Shared VS constants | c0–c3 `matCameraView`, c4–c7 `matCameraViewIT` | HLSL `register` | `FixedFunc*.vs` |
| `DEFORM_PRECISION` | 10 | `#define` | `data/shaders/globals.h` |
| GL trace max lines | 50000 | `const unsigned MAX_LINES` | `opengl_trace.cpp:glTrace` |
| Trace rotating string slots | 32 | `const int MAX_STRINGS` | `rotatingStaticString` |
| Extension-string buffer | 4096 bytes (vendor/renderer/version 256 each) | static arrays | `ogl_extensions.cpp` |
| Display-mode filter | bpp ≥ 15, w ≥ 640, h ≥ 480 | literals | `ListAllDisplayModes` |
| Temporary window for fullscreen res change | 640×480 windowed | literals | `RestartWindowManagerAndRenderer` |
| Safety window mode | 800×600, windowed, 16-bit colour, 16/24-bit z, 60 Hz | literals | `Renderer::Initialise` |
| D3D depth formats | 16→D16, 24→D24X8, 32→D32 | switch | `Direct3DInit` |
| D3D colour formats | 24/32→X8R8G8B8 (fallback A8R8G8B8); 15/16→X1R5G5B5 (fallback R5G6B5, A1R5G5B5) | switch | `Direct3DInit` |
| Gamma exponent | `2.0 − gamma`; ramp scale 65535 | literals | `Renderer::SetScreenGamma` |
| AVI defaults | DemoFrameRate 25, AVIMotionBlurFactor 1, 24 bpp, 1000 ppm, `"Untitled.avi"`, VfW ≥ 0x010a | prefs/literals | `AVIGenerator::AVIGenerator`, `InitEngine` |
| AviPlayer frame buffer | 512×256×3 bytes; streaming speed 2000 | literals | `AviPlayer::OpenAvi` |
| `M_PI` | 3.1415926535897932384626 | `#define` | `math_utils.h` |
| `NearlyEquals` tolerance | 1e-6 | macro | `math_utils.h` |
| `Vector3::Normalise` threshold | lenSqrd > 1e-10 else (0,0,1) | literal | `vector3.h` |
| `Vector3::RotateAround` threshold | |axis|² < 1e-8 | literal | `vector3.cpp` |
| `Matrix33/34::RotateAround` threshold | |axis|² < (1e-5)² | literal | `matrix33.cpp`, `matrix34.cpp` |
| `Matrix34::IsNormalised` tolerances | length² ∈ [0.999, 1.001], dot ≤ 0.001 | literals | `matrix34.cpp` |
| `RampUpAndDown` overrun value | 1.0001 | literal | `math_utils.cpp` |
| `IsPointInTriangle` tolerance | ×1.0001 | literal | `math_utils.cpp` |
| `RaySegDist` direction threshold / miss multiplier | 0.0001 / ×100000 | literals | `math_utils.cpp` |
| `RayTriIntersection` det epsilon / default ray length | 1e-7 / 1e10 | literals | `math_utils.cpp` |
| `RayPlaneIntersection` epsilons | 1e-7 | literals | `math_utils.cpp` |
| `nearestPowerOfTwo` cap | 2³¹ | literal | `math_utils.cpp` |
| MT19937 (dead copy) | N 624, M 397, MATRIX_A 0x9908b0df, default seed 5489 | `#define` | `math_utils.cpp` |
| `Vector2::Compare` tolerance | `FLT_EPSILON` (float `fabsf`) | `<float.h>` | `vector2.cpp` |
| `code/globals.h` | NUM_TEAMS 6, NUM_SLICES_PER_FRAME 10, IAMALIVE_PERIOD 0.1 s, MINIMUM_RENDER_PERIOD 1.0 s, GRAVITY 10.0 | `#define` | `code/globals.h` |

---

<!-- from P14_platform_lib -->
## Constants table (this partition)

| Constant | Value | Kind | Evidence |
|---|---|---|---|
| `APP_NAME` | "Multiwinia" | `#define` | `code/lib/universal_include.h` |
| `DARWINIA_VERSION` / `APP_VERSION` | "dev" | `#define` | same |
| `DARWINIA_EXE_VERSION` | 6,0,0,0 | `#define` | same |
| `DARWINIA_GAMETYPE` (FULL) | "multiwinia" | `#define` | same |
| `GRACE_TIME` | 3.0 s (20.0 s testbed) | `#define` | same |
| `USE_PENTIUM_COUNTER` | 0 | `#define` | `code/lib/hi_res_time.cpp` |
| `APP_RAND_MAX` | 32767 | `#define` | `contrib/systemiv/lib/math/random_number.h` |
| LCG multiplier / increment | 214013 / 2531011 | literal | `random_number.cpp:AppRandom` |
| MT19937 N, M, MATRIX_A, UPPER/LOWER masks | 624, 397, 0x9908B0DF, 0x80000000, 0x7FFFFFFF | `#define` | `random_number.cpp` |
| MT init multiplier; default seed for −1 | 1812433253; 5489 | literal | `random_number.cpp:syncrandseed` |
| MT tempering masks | 0x9D2C5680, 0xEFC60000 | literal | `random_number.cpp:_syncrand` |
| syncfrand divisor | 4294967296.0 | literal | `random_number.cpp:_syncfrand` |
| TRACK_SYNC_RAND log cap | 100000 entries | `static int` | `random_number.cpp` |
| `RandomNormalNumber` samples | 12 | literal | `random_number.cpp` |
| `FileCRC` MAXSIZE | 100000 bytes | `#define` | `contrib/systemiv/lib/math/filecrc.cpp` |
| CRC32 polynomial / init | 0xEDB88320 / 0 (no xorout) | literal | `contrib/unrar/crc.cpp:InitCRC`, `filecrc.cpp` |
| Text separators | `" \t\n\r:,"`; UTF-16 `L" \f\t\n\r:,"` | `#define DEFAULT_SEPERATOR_CHARS` | text/unicode stream readers |
| Initial line buffer / max line | 128 / 65536 chars | `#define INITIAL_LINE_LEN`, assert | same |
| Encryption marker / offsets | "redshirt2"; {31,7,9,1,11,2,5,5,3,17,40,12,35,22,27,2} | literal / `static unsigned int[]` | `text_file_writer.cpp`, `text_stream_readers.cpp` |
| UTF-16 BOMs | FF FE (LE), FE FF (BE) | literal | `unicode_text_stream_reader.cpp` |
| Language limits | 1024 phrase chars; 64 marker key; 96 final key | `#define` | `code/lib/language_table.cpp` |
| Language mode suffixes | `_kbd`, `_xin`; prefix `part_` | literal | same |
| Directory limits | 1024 data items; 10240 string length | `#define DIRECTORY_MAXDIRSIZE/MAXSTRINGLENGTH` | `contrib/systemiv/lib/tosser/directory.cpp` |
| Directory markers | '<' / '>' (1 byte) | `#define` | same |
| Directory type ids | 0 UNKNOWN,1 INT,2 FLOAT,3 CHAR,4 STRING,5 BOOL,6 VOID,7 FIXED,8 INTS,9 ULLONG,10 UNICODE | `#define` | `directory.h` |
| Directory safe values | −1, −1.0f, '?', "[SAFESTRING]", L"[SAFESTRING", false, (ull)−1 | `#define` | `directory.h` |
| PackedInt escape | 255 | literal | `contrib/systemiv/lib/network_stream.cpp:WritePackedInt` |
| HashTable initial size / max | 4 / 65536 | literal/assert | `hash_table.cpp` |
| HashTable hash case mask | 0xDF | literal | `hash_table.cpp:HashFunc` |
| Profiler frame-time history | 200 frames | literal | `profiler.cpp:Advance` |
| `PROFILE_HISTORY_LENGTH` | 10 (unused) | `#define` | `profiler.cpp` |
| Crash handler stack dump / code bytes | 3072 DWORDs / 16 bytes / 4 columns | `const int` | `exception_handler.cpp` |
| hprintf buffer | 8192 chars | `#define` | same |
| Crash file names | CRASH.DMP, ERRORLOG.TXT, REGISTRY.TXT, MultiwiniaSupport.exe | `#define` | `contrib/XCrashReport/CrashFileNames.h` |
| App magic passed to support tool | "0B9432AF" | literal | `code/main.cpp:AppMain` |
| Thread affinity masks | main 0xAA; workers 0x55 | literal | `code/app.cpp`, `code/loading_screen.cpp`, `code/lib/opengl_directx.cpp` |
| Loading-screen job frame budget | 1/60 s | literal | `code/loading_screen.cpp` |
| Language-table deferred delete | 5 frames | literal | `code/app.cpp:SetLanguage` |
| Archive max files | 16384 | `#define MAX_FILES` | `contrib/unrar/unrar.h` |
| keygen | seed 28477717, 1710 rounds, sysinfo 184–500 B, userinfo 28–200 B | literal | `code/keygen.cpp` |
| Auth key length | 31 chars (+NUL = 32) | `#define AUTHENTICATION_KEYLEN` | `contrib/systemiv/lib/metaserver/authentication.h` |
| Prefs float precision on save | 2 decimals | format | `code/lib/preferences.cpp:SaveItem` |
| ShapeExporter position merge threshold | (0.001)² | literal | `tools/ShapeExporter/ShapeExporter.cpp:LookupPosition` |

---

<!-- from P17a_ui_framework_editor -->
## Constants table

| Constant | Value | Kind | Ref |
|---|---|---|---|
| `SIZE_ECLWINDOW_NAME`, `SIZE_ECLWINDOW_TITLE`, `SIZE_ECLBUTTON_NAME` | 256 | `#define` | `contrib/eclipse/eclwindow.h`, `eclbutton.h` |
| Tooltip delay | 1000 ms | literal | `eclipse.cpp:EclUpdateMouse` |
| Resize grab band | ±4 px (exclusive) around bottom/right edge | literal | `EclUpdateMouse` |
| Minimum window size on drag-resize | 60 × 40 px | literal | `EclUpdateMouse` |
| Auto-grow margin | 10 px below lowest button | literal | `EclWindow::RegisterButton` |
| Default button font size | 11.0 | ctor | `DarwiniaButton::DarwiniaButton` |
| Auto button width | `chars × 7 + 9` px; height 15 | literal | `DarwiniaButton::SetShortProperties` |
| Title-bar height | `GetMenuSize(15)` px | literal | `DarwiniaWindow::GetClientRectY1` |
| Close box | 10 × 10 at (w − 12, 2) | literal | `DarwiniaWindow::Create` |
| Large-menu scale | `0.96 × screenH / 460` | literal | `DarwiniaWindow::GetMenuSize` |
| Large-menu preference | `LargeMenus` == 2 | pref | `code/app.cpp`, `prefs_other_window.h:OTHER_LARGEMENUS` |
| Window body tint alpha | 0.96 | literal | `DarwiniaWindow::Render` |
| Body texture repeat | s × 8 (assumes 512×64 texture) | literal | `DarwiniaWindow::Render` |
| Value-control field width | `w − 37` (numeric); scrollers 15×15 at +5 and +22 | literal | `CreateValueControl` |
| `PIXELS_PER_CHAR` | 7 | `#define` | `input_field.cpp` |
| Input box width | `len × 7 + 7` | literal | `InputField::Keypress/Render` |
| Edit well | right 40 % of field | literal | `InputField::Render` |
| Caret blink | 1 Hz, 50 % duty | literal | `InputField::Render` |
| `INTEGER_INCREMENT_PERIOD` | 0.1 s (10 steps/s) | `#define` | `input_field.cpp` |
| Float scroller rate | `change × elapsed × 0.2` per frame | literal | `InputScroller::CheckInput` |
| Scroll speed-up | × 5 (Shift) | literal | `InputScroller::CheckInput` |
| Default numeric bounds | [0, 1.0e4] | default args | `InputField::Register*` |
| `DEF_FONT_SIZE` | 12.0 | `#define` | `code/lib/text_renderer.h` |
| `GetTextWidth` default size | 13.0; horiz scale 0.6 (CJK 1.0) | default arg / ctor | `text_renderer.h/.cpp` |
| Scrollbar arrow height | 18 px | literal | `ScrollBar::Create` |
| Drop-down column threshold | 0.8 × screen height | literal | `DropDownMenu::CreateMenu` |
| MessageDialog | ≤ 20 lines; w ≥ 100; h = 65 + 12·lines ≥ 85; button 40×18 at h − 30 | literal | `message_dialog.cpp` |
| FileDialog row | 13 px pitch, 12 px high; rows = (h − 60)/13 | literal | `FileDialog::Create` |
| Double-click window | 0.2 s | literal | `FileButton::MouseUp`, `MultiwiniaFileButton::MouseUp` |
| MultiwiniaFileDialog rows | 10 | literal | `MultiwiniaFileDialog::Create` |
| Front-end fonts | large 0.095 H, med 0.04 H, small 0.022 H | literal | `code/game_menu.cpp:GetFontSizes` |
| Front-end buttons | w = 0.9 × left box w, h = 1.3 × med font, gap = 0.1 × med font | literal | `game_menu.cpp:GetButtonSizes` |
| Editor magic location id | 999 | literal | `multiwinia_window.cpp` |
| Default research level for editor test games | 3 | literal | `MultiwiniaWindow::MultiwiniaWindow` |
| `INSTANT_UNIT_SIZE_FACTOR` | 6.0 (pick/sphere radius = √n × 6) | `#define` | `location_editor.cpp` |
| Instant-unit dot pitch / half-size | 10 / 5 world units | literal | `LocationEditor::RenderUnit` |
| New instant unit | 40 units, team 0, spread 200.0 | literal / ctor | `CreateButton::MouseUp`, `level_file.h` |
| Camera-mount gizmo cull distance | 20.0 | literal | `LocationEditor::Render` |
| Building rotate rate | 0.01 rad / pixel | literal | `AdvanceModeBuilding` |
| Light rotate rate | 0.05 rad / pixel | literal | `AdvanceModeLight` |
| New tile size | 384 | literal | `NewTileButton::MouseUp` |
| New flatten-area half-size | 40.0 | literal | `NewFlattenAreaButton::MouseUp`, `LandscapeFlattenArea` ctor |
| Landscape scale factors | 0.95 / 1.05 | literal | `LandscapeEditWindow::Create` |
| Light brightness factors | 0.9 / 1.1 | literal | `LightsEditWindow::Create` |
| Guide-grid flatten pivot | 20 (of 255) | literal | `GuideGrid::HandleMouseInput` |
| Guide-grid resolution power | [0, 5] | literal | `LandscapeGuideGridWindow::Create` |
| Tree clone height jitter | × (1 ± 0.15) (`sfrand(0.3)`) | literal | `TreeButton::MouseUp` |
| Crosshair half-length | 30 px | literal | `LocationEditor::Render` |
| New GlobalLocation position | (−96.25, −274.02, 75.16) | literal | `NewLocationButton::MouseUp` |
| Cheat spawn counts | Darwinians 20 (spread 30); Virii 20 (range 1000); spirits 10 (x/z jitter ±10 via synced `SFRAND(20)` = `(syncfrand − 0.5) × 20`); ports 3 per port | literal | `cheat_window.cpp` |
| Cheat crate drop height | y = 300 | literal | `CrateCreateButton::MouseUp` |
| Network window client slots | 6 | literal | `NetworkWindow::Create/Render` |
| Lost-connection display threshold | 2.0 s | literal | `NetworkWindow::Render` |
| Assumed ms per sequence step | 100 | literal | `NetworkWindow::Render` |
| Monster / Futurewinian team ids | 4 / 5 (−1 if `m_allowMonsterTeam` false) | literal | `code/location.cpp:GetMonsterTeamId`, `GetFuturewinianTeamId` |

<!-- from P17b_menus_options -->
## Constants table

| Constant | Value | Kind | Where |
|---|---|---|---|
| `SERVERS_ON_SCREEN` | 13 | `#define` | `code/game_menu.h:SERVERS_ON_SCREEN` |
| `MAX_GAME_TYPES` | 7 | `#define` | `code/game_menu.h:MAX_GAME_TYPES` |
| `GAMETYPE_PROLOGUE`, `GAMETYPE_CAMPAIGN` | 254, 255 | `#define` | `code/game_menu.h:GAMETYPE_PROLOGUE` |
| `PlayerNameString` max length | 16 chars | ctor default | `code/game_menu.h:PlayerNameString` |
| Chat rename max | 15 chars (buffer 16) | literal | `ChatInputField::Keypress` |
| Chat message buffer | 255 chars | literal | `ChatInputWindow::Create` |
| `PIXELS_PER_CHAR` | 25 | `#define` | `code/game_menu.cpp:PIXELS_PER_CHAR` |
| `INTEGER_INCREMENT_PERIOD` | 0.1 s | `#define` | `code/game_menu.cpp:INTEGER_INCREMENT_PERIOD` |
| Menu fade-in on create | 0.25 s | literal | `GameMenu::CreateMenu` |
| Menu transition | duration arg 1.0, direction ±1 | literal | `GameMenuButton::MouseUp` etc. |
| Button cascade | 0.6 × y/H s slide; (5/numButtons) × y/H s flash; 0.1 s caption flash; slide distance 0.1 W per s | literals | `GameMenuButton::Render` |
| Caret blink | 1 s period, on for 0.5 s | literal | input fields |
| Layout fractions | 0.26 H, 0.6 W, 0.05, 1.4 aspect, 0.08 W | literals | `GetPositions` |
| Font fractions | 0.095 H, 0.04 H, 0.022 H | literals | `GetFontSizes` |
| Button sizes | 0.9 leftW, 1.3 × med, 0.1 × med | literals | `GetButtonSizes` |
| Options row shrink | 0.65 | literal | prefs windows |
| Logo aspect | 277/194 | literal | title bar |
| Front-end camera | eye (0,600,0); yaw −1.899 rad, roll −1.079 rad; FOV 90; sphere-world target (−900,3000,3970) front (0,0.5,−1) | literals | `GameMenu::Render`, `CreateMenu` |
| Large-menu scale | 0.96 × H/460 | literal | `DarwiniaWindow::GetMenuSize` |
| General help pages | `MAX_PAGES` 5 | `#define` | `helpandoptions_windows.cpp` |
| Crate help slide speed | 750 px/s | literal | `CrateButton::Render` |
| Leaderboard caption anim | alpha ±0.1 per 0.025 s | members | `LeaderBoardListButton` |
| Demo-end exit delay | 4 s; fade alpha cap 0.9 | literal | `DemoEndWindow` |
| Screen fallback mode | 800×600 windowed, 16-bit, z16→z24, 60 Hz | literal | `Renderer::Initialise` |
| Mode-switch hack temp mode | 640×480 windowed | literal | `RestartWindowManagerAndRenderer` |
| Resolution filter | bpp ≥ 15, W ≥ 640, H ≥ 480 | literal | `WindowManagerWin32::ListAllDisplayModes` |
| Port bounds | 0 … 65535.1 | literal | `PrefsOtherWindow::Create` |
| Default ports | server 4000, client 4001 | data + literal | `data/default_preferences.txt` |
| Water cell size | detail × max(worldX,worldZ)/100 | formula | `Water::Water` |
| Darwinian high-detail radius | 99999.9 / 1000 / 500 | literal | `Team::RenderDarwinians` |
| Virii detail distance thresholds | 300 / 500 / 600 / 1000 | literal | `Team::RenderVirii`, `Virii::Render` |
| Darwinian scale formula | `1 + k(d−100)/1000` (units, officers), `/1500` (flags, blitz zones); k = `DarwinianScaleFactor` = 2.0 | data | §4.1 |
| Cloud layer heights | 1200 / 1000 / 800 | literal | `Clouds::RenderBlobby/RenderFlat` |
| Chat box | font 0.015 H; width 0.25 W; reference widths 1280/1024, scale 0.75 at ≤640 | literal | `ChatInputWindow` |
| Error dialogue | W/3 × H/3 | literal | `GameOptionsWindow::RenderErrorDialogue` |
| Profiler window | 570×600; history 200 frames | literal | `DebugKeyBindings::ProfileButton`, `ProfileWindow::RenderFrameTimes` |

---
