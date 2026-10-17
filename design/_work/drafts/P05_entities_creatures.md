# P05_entities_creatures — Creatures, hazards, legacy entities
Scope: `code/worldobject/{anthill,armyant,centipede,dropship,dustball,egg,lander,lightning,meteor,snow,souldestroyer,spaceship,spam,spider,sporegenerator,tentacle,triffid,tripod,virii,wallbuster}.{cpp,h}` (40 files, 11 302 lines) plus `data/stats.txt`. Build baseline: see build_matrix.md (`Release|Win32`, `TARGET_PC_FULLGAME`, `BLOCK_OLD_DARWINIA_OBJECTS` on, `INCLUDE_CRATES_ADVANCED` on).

<!-- SUMMARY-TABLE-PLACEHOLDER -->

## [→ 02_units_and_entities] Common conventions used by every creature in this partition

These facts are shared engine behaviour that every class below relies on; they are documented in full by the entity-core partition but are restated here so each creature spec is self-contained.

| Item | Value | Ref |
|---|---|---|
| Simulation tick | `SERVER_ADVANCE_PERIOD` = `0.1 * g_gameTimer.m_gameSpeed` seconds (10 Hz at normal speed; the Slow-Down/Speed-Up-Time crates change `m_gameSpeed`) | [C] `code/gametimer.h:SERVER_ADVANCE_PERIOD`, `code/gametimer.cpp:GameTimer::GetServerAdvancePeriod` |
| Stats storage | `Entity::m_stats[3]` is `unsigned char` (0–255). `SetType` copies `EntityBlueprint` stats (doubles from `data/stats.txt`) into it, so blueprint values > 255 wrap/are undefined (Meteor speed 350, SpaceShip speed 500) and fractional values are truncated. Code that needs the true value reads `EntityBlueprint::GetStat` directly (e.g. Meteor). | [C] `code/worldobject/entity.h:Entity::m_stats`, `code/worldobject/entity.cpp:Entity::SetType`; [I] wrap behaviour = MSVC double→unsigned-char conversion |
| Initial reload | `SetType` sets `m_reloading = syncfrand(m_stats[StatRate])` (random phase) | [C] `code/worldobject/entity.cpp:Entity::SetType` |
| Generic death | `Entity::ChangeHealth(amount)`: if `health+amount <= 0` → `m_stats[StatHealth]=100`, `m_dead=true`, sound `"Die"`, **one spirit** spawned at `m_pos` with `vel*0.5` and the entity's team (`Location::SpawnSpirit`). Health clamps at 255. Any negative amount triggers sound `"LoseHealth"`. Classes that override `ChangeHealth` may bypass this. | [C] `code/worldobject/entity.cpp:Entity::ChangeHealth` |
| Dead fade-out | `Entity::AdvanceDead`: health (reset to 100 at death) decreases `40/s` on ground, `20/s` in air; entity removed when it reaches 0 ⇒ 2.5 s (ground) / 5 s (air) corpse/fade time. | [C] `code/worldobject/entity.cpp:Entity::AdvanceDead` |
| Ballistic fall | `Entity::AdvanceInAir`: gravity `-15` m/s²; lands when `y <= ground+0.1`; enters water (`m_inWater=0`) if `y <= 0`. | [C] `code/worldobject/entity.cpp:Entity::AdvanceInAir` |
| Drowning | `Entity::AdvanceInWater`: after `m_inWater > 10.0` s → `ChangeHealth(-500)`; swims at 5 m/s, bobbing `y = -sin(3t) - 4`. | [C] `code/worldobject/entity.cpp:Entity::AdvanceInWater` |
| Map clamp | `Entity::Advance` clamps x,z into `[0, worldSize]`. | [C] `code/worldobject/entity.cpp:Entity::Advance` |
| Spawning | `Location::SpawnEntities(pos, team, unitId, type, n, vel, spread, range)`: if team is the Monster team (id **4**) or Futurewinian team (id **5**) and unused, initialises it first (`InitMonsterTeam`/`InitFuturewinianTeam`); then `SetType`, position = `FindValidSpawnPosition(pos, spread)`, `m_onGround=false`, `m_spawnPoint=pos`, `m_roamRange = range` (or `spread` if `range==-1`), `Begin()`. | [C] `code/location.cpp:Location::SpawnEntities` |
| Monster team | Team 4, `TeamTypeCPU`, colour RGB(10,10,10) near-black, `m_colourId = MULTIWINIA_NUM_TEAMCOLOURS`. Returns -1 if `m_allowMonsterTeam` false. | [C] `code/location.cpp:Location::GetMonsterTeamId`, `Location::InitMonsterTeam` |
| Futurewinian team | Team 5, `TeamTypeCPU`, colour RGB(200,200,200), `m_colourId = MULTIWINIA_NUM_TEAMCOLOURS+1`. Owner of the crate SpaceShip. | [C] `code/location.cpp:Location::GetFuturewinianTeamId`, `Location::InitFuturewinianTeam` |
| Level-file blocking | `Entity::EntityIsBlocked` (true for `TypeAI, TypeArmyAnt, TypeDropShip, TypeEgg, TypeLander, TypeLaserTroop, TypeMeteor, TypeNuke, TypeOfficer, TypeShaman, TypeSpaceInvader, TypeTentacle, TypeTriffidEgg, TypeTripod, TypeInvalid, TypeSpaceShip`) is consulted **only** when loading `InstantUnits` from a level file and by the (editor-only) instant-unit window. It does **not** stop code from creating these types with `SpawnEntities`/`NewEntity`. Likewise `Building::BuildingIsBlocked` only filters `Building::CreateBuilding(char *name)` (level-file/editor by-name creation), not `CreateBuilding(int type)`. | [C] `code/worldobject/entity.cpp:Entity::EntityIsBlocked`, `code/level_file.cpp:LevelFile::ParseInstantUnits` (L684), `code/interface/instant_unit_window.cpp` (L324), `code/worldobject/building.cpp:Building::CreateBuilding(char*)` |
| Random numbers | `syncrand()/syncfrand(x)/syncsfrand(x)` = network-synchronised MT19937 (`syncfrand(x)∈[0,x)`, `syncsfrand(x)∈[-x/2,x/2)`). `FRAND(x)/SFRAND(x)` are wrapper structs that resolve to `_syncfrand/_syncsfrand` (i.e. **synced**) when passed to `Vector3` constructors/`FromRand`. Bare `frand(x)/sfrand(x)` are the **unsynced** LCG (`AppRandom`, `frand∈[0,x]`, `sfrand∈(-x/2,x/2]`) and must only affect rendering. | [C] `contrib/systemiv/lib/math/random_number.cpp:_syncfrand,_syncsfrand,frand,sfrand`, `code/lib/vector3.h:FRAND`, `code/lib/vector3.cpp:FromRand` |

## [→ 02_units_and_entities] AntHill (Building) and ArmyAnt (Entity) — ant colony

### Role and build tag
- **AntHill** — a monster-team nest building that sends ArmyAnts out to fetch Darwinians, spirits and attack enemies. **[B:FULL]** live in Multiwinia as the **Ant Nest** crate outcome (`CrateRewardAntNest`): either as a player-placeable task (`GlobalResearch::TypeAntsNest` → `Task::TargetAntNest` → `Crate::RunAntNest`) when the crate is captured, or immediately at the crate position when an uncaptured crate expires (`Crate::RunUncapturedCrate`). [C] `code/worldobject/crate.cpp:Crate::CaptureCrate`, `Crate::RunUncapturedCrate`, `Crate::RunAntNest`; `code/taskmanager.cpp:Task::TargetAntNest`. Also placed by Darwinia campaign maps (13 instances in `data/levels/map_*.txt`/`mission_*.txt`) → that path is [B:LEGACY]. `TypeAntHill` is **not** in `Building::BuildingIsBlocked`. [C] `code/worldobject/building.cpp:Building::BuildingIsBlocked`.
- **ArmyAnt** — small fast ground creature. **[B:FULL]** live, created only by code: `AntHill::SpawnAnt`, the wake-up ant in `Crate::RunAntNest`, and `TriffidEgg::Spawn` case `SpawnAnts` (Triffids crate). `TypeArmyAnt` **is** in `Entity::EntityIsBlocked`, which only prevents ants being placed as level-file InstantUnits. [C] `code/worldobject/anthill.cpp:AntHill::SpawnAnt`, `code/worldobject/crate.cpp:Crate::RunAntNest`, `code/worldobject/triffid.cpp:TriffidEgg::Spawn`, `code/worldobject/entity.cpp:Entity::EntityIsBlocked`. The Shaman summon path (`code/worldobject/shaman.cpp`) is [B:LEGACY] (Shaman blocked).

### Crate effect mechanics — `Crate::RunAntNest(pos, teamId)` [→ 04_crates_powerups_and_weapons]
[C] `code/worldobject/crate.cpp:Crate::RunAntNest`
1. Creates **2** AntHills, **always owned by the Monster team (4)** regardless of `teamId` (the triggering player only gets the marker/message).
2. Each hill: position = `pos + (syncsfrand(200), 0, syncsfrand(200))` (i.e. ±100 m), y = terrain height; `m_front` = normalised `(syncsfrand(1),0,syncsfrand(1))`; `m_numAntsInside = 75 - syncrand()%25` → **51–75 ants**; `SetDetail(1)`; new building id from `GlobalWorld::GenerateBuildingId`; an explosion effect of the hill shape is played at creation (`g_explosionManager.AddExplosion(shape, mat, 1.0)`).
3. Spawns **one ArmyAnt** for the Monster team at `pos` (comment: wakes up the monster team — `SpawnEntities` initialises team 4 if unused).
4. `m_obstructionGrid->CalculateAll()`.
5. Marker `icons/icon_antnest.bmp` for `teamId`; message `crate_message_run_antnest` (team name substituted for flag `T`) or `multiwinia_exclamation` with the crate name if `teamId` is the monster team; sound `Crate::TriggerSoundEvent(CrateRewardAntNest, pos, teamId)`.
- Crate validity cap: Ant Nest is invalid when `(#AntHills in level) + 2 × (#queued TypeAntsNest tasks across teams) >= 4`. [C] `code/worldobject/crate.cpp:Crate::IsValid`.

### AntHill stats and fields
| Field | Value | Ref |
|---|---|---|
| Shape | `anthill.shp` | [C] `code/worldobject/anthill.cpp:AntHill::AntHill` |
| `m_health` | 100 (own integer health, separate from Building damage) | [C] `AntHill::AntHill` |
| `ANTHILL_SEARCHRANGE` | 400.0 m (`#define`) | [C] `code/worldobject/anthill.h` |
| First spawn delay | `m_spawnTimer = now + 5.0` s after `Initialise` | [C] `AntHill::Initialise` |
| Objective cap | SP: 3; MP: `m_numAntsInside/10` clamped to [1,6] | [C] `AntHill::Advance` |
| Objective search interval | `now + syncrand()%2` s (0 or 1 s) | [C] `AntHill::Advance` |
| Ants per objective | SP: `5 + 5*(difficulty/10)`; MP: `int(5 + syncfrand(5) + m_numAntsInside/20.0)` (5–9 + ants/20) | [C] `AntHill::Advance` |
| Ant release interval | MP: 0.2 s; SP: `0.2 - 0.2*difficulty/10` | [C] `AntHill::Advance` |
| Spirit→ant conversion | each conversion consumes 1 stored spirit: SP +3 ants, next after 5.0 s; MP **+6 ants**, next after 1.0 s | [C] `AntHill::Advance` |
| Empty-hill decay (MP) | if `m_numAntsInside <= 0` for **30 consecutive Advance calls** (3.0 s at 10 Hz) → `m_numAntsInside = 10`, `Damage(-10)` | [C] `AntHill::Advance` |
| Level-file field | one integer `m_numAntsInside` after the generic Building fields; values outside [0,10000] reset to 0 | [C] `AntHill::Read`, `AntHill::Write` |

### AntHill behaviour
- **Only runs when its team is `TeamTypeCPU`** (the Monster team is CPU). [C] `AntHill::Advance`.
- **Population lock** (`AntHill::PopulationLocked`): in multiplayer with a level population cap (`m_populationCap != -1`): locked when `team->m_others.NumUsed() - Virii::s_viriiPopulation[team] >= m_populationCap / m_numPlayers` (team 255 never locked). Otherwise (or no cap), looks once for the nearest `SpawnPopulationLock` building whose `m_searchRadius` contains the hill and is locked when `lock->m_teamCount[team] >= lock->m_maxPopulation`. A locked hill neither creates objectives nor releases ants (spirit conversion continues). [C] `code/worldobject/anthill.cpp:AntHill::PopulationLocked`.
- **Objective selection** (when not locked, timer expired and below cap): MP chooses one search uniformly with `syncrand()%5`: 0,1 → nearest-listed enemy **Darwinian** within 400 m not already `StateCapturedByAnt` and not already targeted; 2 → any enemy entity within 400 m not already targeted; 3 → a spirit within 400 m in `StateBirth`/`StateFloating` not within 20 m of an existing objective; 4 → scout point at radius `200 + syncfrand(200)` m, random angle, accepted only if terrain height > 0. SP tries the four in that order. A found target becomes an `AntObjective {m_pos, m_targetId, m_numToSend}`. [C] `AntHill::SearchForDarwinians`, `SearchForEnemies`, `SearchForSpirits`, `SearchForScoutArea`, `SearchingArea`, `TargettedEntity`.
- **Release** (`AntHill::SpawnAnt`): picks a random objective (`syncrand()%size`), creates/reuses the hill's ant Unit (`Team::NewUnit(TypeArmyAnt, m_numAntsInside, ...)`), spawns 1 ant at the hill (terrain height, spread 10), sets `m_buildingId`, `m_orders = ScoutArea`, `m_wayPoint = objective.pos + random offset radius syncfrand(30)`, pushed from obstructions; decrements `m_numAntsInside` and the objective's `m_numToSend` (objective deleted at 0).
- **Damage** (`AntHill::Damage(d)`, d negative): `m_health += d`; every crossing of a 20-HP band plays a partial explosion (fraction `1 - health/100`) and building sound `"Damage"`. At `m_health <= 0`: full explosion, **releases `m_numAntsInside + m_numSpiritsInside` spirits** (team = hill team) in a 20 m disc with speed `syncfrand(50)`, sound `"Explode"`; `Advance` returns true (building removed). `Destroy()` sets health 0. [C] `AntHill::Damage`, `AntHill::Destroy`.
- **Burning** (`AntHill::Burn`, called by burning Trees within `100 + treeRadius`): 30% chance per call of `Damage(-1)`, and always releases one ant toward a random point 200 m away (fleeing). [C] `AntHill::Burn`; caller `code/worldobject/tree.cpp:Tree::Advance` (L224).
- Visual: damaged flicker using unsynced `frand` (render-only). Sound events listed: building defaults + `"Explode"`.

### ArmyAnt stats
| Stat | Value | Ref |
|---|---|---|
| Health | 20 (`data/stats.txt` ArmyAnt HLTH) | [C] `data/stats.txt` |
| Speed | 25 m/s × `(1 + syncsfrand(0.4))` per ant (20–30 m/s), ×2 while `CollectEntity`/`AttackEnemy` | [C] `ArmyAnt::Begin`, `ArmyAnt::AdvanceToTargetPosition` |
| Rate | 0 (unused) | [C] `data/stats.txt` |
| Scale | `1.0 + syncsfrand(0.9)` (0.55–1.45) | [C] `ArmyAnt::Begin` |
| Self search range | `ARMYANT_SEARCHRANGE` = 10 m (`#define`) for spirits and enemies on the way | [C] `code/worldobject/armyant.h` |
| Bite damage | 1 HP per tick while at target (`ChangeHealth(-1)`) | [C] `ArmyAnt::AdvanceAttackEnemy` |
| Stray return | `SearchForRandomPosition`: return chance `dist(spawnPoint)/400`; else random 100 m step | [C] `ArmyAnt::SearchForRandomPosition` |
| New-base search | friendly AntHill within 500 m | [C] `ArmyAnt::SearchForAntHill` |

### ArmyAnt state machine (`m_orders`)
```mermaid
stateDiagram-v2
    [*] --> NoOrders
    NoOrders --> ReturnToBase: OrderReturnToBase (hill exists or friendly hill <500m found)
    NoOrders --> BaseDestroyed: no hill
    ScoutArea --> CollectEntity: arrived & target is Darwinian
    ScoutArea --> AttackEnemy: arrived & target is other entity
    ScoutArea --> ReturnToBase: arrived & target gone
    ScoutArea --> CollectSpirit: SearchForTargets found spirit within 10m
    ScoutArea --> AttackEnemy: SearchForTargets found non-Darwinian non-Harvester enemy within 10m
    CollectSpirit --> ReturnToBase: arrived (spirit -> StateAttached) or spirit gone/dead/attached/in egg
    CollectEntity --> ReturnToBase: arrived (Darwinian::AntCapture) or target dead/already captured
    AttackEnemy --> ReturnToBase: target dead or gone
    AttackEnemy --> AttackEnemy: arrived: target -1 HP, 3 muzzle-flash particles, sound Attack
    ReturnToBase --> [*]: arrived at hill (ant stored, entity removed)
    ReturnToBase --> BaseDestroyed: arrived & hill gone & no friendly hill
    BaseDestroyed --> BaseDestroyed: arrived -> SearchForRandomPosition
```
[C] `code/worldobject/armyant.cpp:ArmyAnt::Advance` and the `Advance*` methods.
- `SearchForTargets` runs every tick while `!m_targetFound` (spirits first, then enemies); once a target is found it is not re-searched until a failure clears `m_targetFound`. [C] `ArmyAnt::Advance`, `ArmyAnt::SearchForTargets`.
- **Delivery at the hill** (`AdvanceReturnToBase`): hill `m_numAntsInside++`; a carried spirit in `StateAttached` is removed from the world and `m_numSpiritsInside++`; a carried Darwinian in `StateCapturedByAnt` is killed with `ChangeHealth(-2 × health)` (which spawns its spirit next to the hill). The ant entity is then removed (`Advance` returns true). [C] `ArmyAnt::AdvanceReturnToBase`.
- **Carrying**: an attached spirit, and a captured Darwinian (via `Darwinian::AdvanceCapturedByAnt`), are teleported every tick to the ant's `MarkerCarry` shape marker. If the ant dies the Darwinian returns to `StateIdle`. [C] `ArmyAnt::GetCarryMarker`; `code/worldobject/darwinian.cpp:Darwinian::AntCapture`, `Darwinian::AdvanceCapturedByAnt`.
- **Movement**: straight line toward `m_wayPoint` at speed, y snapped to terrain (no obstruction pushing while walking — the call is commented out), heading jittered by `syncsfrand(0.2)` rad per tick; arrival when distance < `|vel|·dt` or < 1 m. Ants fall ballistically when `!m_onGround` (spawned with velocity by triffid eggs). [C] `ArmyAnt::AdvanceToTargetPosition`, `Entity::AdvanceInAir`.
- **Death**: `ChangeHealth` → generic death (spirit spawned) + shape explosion scaled by `m_scale`; drops any attached spirit (`Spirit::CollectorDrops`, inherits ant velocity). [C] `ArmyAnt::ChangeHealth`.
- **Team colouring**: three animation shapes `armyant.shp`, `armyant2.shp`, `armyant3.shp`, each copied and recoloured per team colour/coop group (cached as `<shape>_<colourId>_<groupId>`). Every tick a different one of the three is chosen with `syncrand()%3` (leg animation). [C] `ArmyAnt::SetShape`, `ArmyAnt::Advance`.
- **Sounds**: `"Attack"` (bite), plus generic `"LoseHealth"`, `"Die"`. **Effects**: `Particle::TypeMuzzleFlash` ×3 per bite, shape explosion on death. [C] `ArmyAnt::AdvanceAttackEnemy`.
- **Interaction with Darwinians**: Darwinians treat enemy AntHills and Triffids as attack targets. [C] `code/worldobject/darwinian.cpp:Darwinian::SearchForThreats` (L2827: enemy `TypeAntHill`/`TypeTriffid` buildings are threats).

## [→ 02_units_and_entities] Triffid (Building) and TriffidEgg (Entity) — egg launcher and monster egg

### Role and build tag
- **Triffid** — a rooted plant building that periodically lobs TriffidEggs; each egg bursts after a timer and releases a creature group. **[B:FULL]** live via the **Triffids** crate outcome (`CrateRewardTriffids`, only reachable as an *uncaptured-crate* outcome — `Crate::CaptureCrate` re-rolls it away; `Crate::RunUncapturedCrate` includes it). Darwinia campaign maps place 30 Triffids → [B:LEGACY]. Not in `BuildingIsBlocked`. [C] `code/worldobject/crate.cpp:Crate::CaptureCrate`, `Crate::RunUncapturedCrate`, `Crate::RunTriffids`.
- **TriffidEgg** — **[B:FULL]** live: launched by Triffids (`Triffid::Launch`), spawned directly by the **Eggs** crate (`Crate::RunEggs`, task `GlobalResearch::TypeEggSpawn` or uncaptured — note: `RunUncapturedCrate` has a case for `CrateRewardEggs` but Eggs is not in its acceptable list, so that case is only reached with a fixed `m_rewardId`), and recursively by `SpawnTriffidEggs`. `TypeTriffidEgg` is in `EntityIsBlocked` (level-file only). [C] `code/worldobject/crate.cpp:Crate::RunEggs`, `code/taskmanager.cpp:Task::TargetEggs`, `code/worldobject/triffid.cpp:Triffid::Launch`.

### Crate effect mechanics [→ 04_crates_powerups_and_weapons]
**`Crate::RunTriffids(pos, teamId)`** [C] `code/worldobject/crate.cpp:Crate::RunTriffids`
- Creates `1 + syncrand()%3` (**1–3**) Triffids owned by the **Monster team**, at `pos + (syncsfrand(50),0,syncsfrand(50))` (±25 m), `m_front` pointing away from `pos`.
- Per triffid: `m_pitch = 0.6 + syncfrand(0.6)`, `m_force = 60 + syncfrand(60)`, `m_variance = syncfrand(0.5)`, `m_reloadTime = 40 + syncfrand(60)` s; `m_size` default 1.0; `m_useTrigger` default 0; spawn mask = **all types except `SpawnSpirits` and `SpawnEggs`** (so Virii, Centipede, Spider, TriffidEggs, Ants, SporeGenerator, SoulDestroyer, Darwinians).
- Message `multiwinia_exclamation` with the crate name; marker `icons/icon_triffid.bmp`; sound event for `CrateRewardTriffids` (team 255).

**`Crate::RunEggs(pos, teamId)`** [C] `code/worldobject/crate.cpp:Crate::RunEggs`
- Spawns `2 + syncrand()%3` (**2–4**) TriffidEggs **owned by `teamId`** (the player who used the task — so the hatched creatures fight *for* that player, except Soul Destroyers which are forced to the Monster team in multiplayer, see `TriffidEgg::Spawn`). Start = `pos + (0,20,0)`, velocity direction `(SFRAND(1), 4, SFRAND(1))` with length `50 + syncfrand(50)`.
- Each egg: spawn type drawn with `syncrand()%NumSpawnTypes` until it is **not** Spirits, Eggs, SoulDestroyer, Ants or Darwinians → one of **Virii, Centipede, Spider, TriffidEggs, SporeGenerator** (uniform over these 5); `m_size = 1.0`; burst timer `4 + syncfrand(4)` s; `m_spawnPoint = pos`; `m_roamRange = 300`.
- Marker `icons/icon_egg.bmp`; message `crate_message_auto_eggs` (monster team) or `crate_message_run_eggs` (team name); sound for `CrateRewardEggs`.

### Triffid fields, stats and constants
| Field | Default | Crate value | Level-file order | Ref |
|---|---|---|---|---|
| `m_size` | 1.0 | 1.0 | 1 | [C] `Triffid::Triffid`, `Triffid::Read` |
| `m_pitch` | 0.6 | 0.6–1.2 | 2 | |
| `m_force` | 250.0 | 60–120 | 3 | |
| `m_variance` | π/8 | 0–0.5 | 4 | |
| `m_reloadTime` (s) | 60.0 | 40–100 | 5 | |
| `m_useTrigger` (enemies needed; 0 = always armed) | 0 | 0 | 6 | |
| `m_triggerLocation.x/.z` (offset) | 0,0 | 0,0 | 7,8 | |
| `m_triggerRadius` (m) | 100.0 | 100.0 | 9 | |
| `m_spawn[10]` (0/1 flags in enum order) | all false | see above | 10–19 | |
| `m_damage` ("health") | 50.0 | 50.0 | – | [C] `Triffid::Triffid` |
| `m_numEggs` (MP egg budget) | 3 | 3 | – | |
| Shape / markers | `triffidhead.shp`, `MarkerLaunchPoint`, `MarkerTriffidStem` | | | |

### Triffid behaviour
- **Initial timer**: `m_timerSync = now + (syncrand() % int(m_reloadTime)) + 10` s. Trigger check timer starts at `syncfrand(5)`. [C] `Triffid::Initialise`, `Triffid::Triffid`.
- **Arming**: if `m_useTrigger == 0` or burning → armed. Else every 5 s armed = `EntityGrid::AreEnemiesPresent(pos+triggerLocation, triggerRadius, team)`. On transition unarmed→armed the timer is re-randomised as above. [C] `Triffid::Advance`.
- **Launch** when armed and `now > m_timerSync`: in **multiplayer only while `m_numEggs > 0`**; each launch decrements it and when it reaches 0 calls `Damage(-100)` (sets itself on fire). Single-player launches forever. Next timer `now + reload × (1 + syncsfrand(0.1))`. [C] `Triffid::Advance`.
- **`Triffid::Launch`**: picks uniformly among enabled spawn types (`syncrand()%count`; nothing if none); egg velocity = launch marker forward × `m_force × m_size × (1 + syncsfrand(0.2))`; egg `m_size = 1 + syncsfrand(0.3)`, `m_spawnPoint = pos + triggerLocation`, `m_roamRange = triggerRadius`; MP: burst timer `4 + syncfrand(4)` s. Building sound `"LaunchEgg"`. [C] `Triffid::Launch`.
- **Head pose** (`GetHead`, used for launch direction, rendering and ray hits — computed from network time so it is deterministic): head at `pos + up×30×size`, pitched by `m_pitch×0.5×(1+0.1 sin t)`, bobbing offset `(4 sin(t/1.3), sin(t/1.5), 3 sin(t/1.2))`, recoil `-front×10×((timer-now)/reload)^5`, scale `size×(1+0.1 sin t)` where `t = now + uniqueId×10`. When burning: head thrashes (`front` rotated by `sin(5t)×variance`, larger offsets). [C] `Triffid::GetHead`.
- **Damage/burning**: `Damage(d)` adds `d` to `m_damage`; when it first drops ≤ 0 → sound `"Burn"`. While burning every tick: two `Particle::TypeFire` (size `100 + syncsfrand(100×size)`), 10% (unsynced) debris particle, `m_size -= 0.006`, `m_timerSync -= 0.5`; at `m_size <= 0.3` the head explodes and the building is removed (≈117 ticks ≈ 11.7 s from size 1.0). [C] `Triffid::Damage`, `Triffid::Advance`. Fire sources: flame grenades set Trees and Triffids within `2×power` alight (`code/worldobject/weapons.cpp:FlameGrenade::Advance`, L2050–2065); turret shells can hit Trees, AntHills and Triffids (`code/worldobject/weapons.cpp:TurretShell::Advance`, L1470).
- **Render**: yellow stem lines, head shape, a held egg drawn when within the last 25% of the reload. Editor-only trajectory preview under `LOCATION_EDITOR` [B:EDITOR]. [C] `Triffid::Render`, `Triffid::RenderAlphas`.

### Triffid spawn types (`Triffid::Spawn*` enum) and what `TriffidEgg::Spawn` creates
| # | Enum | Creates (team = egg team unless noted) | SP count | MP count | Spawn args (spread, roam range) | Ref |
|---|---|---|---|---|---|---|
| 0 | `SpawnVirii` | Virii | `5 + rand%5` (5–9) | `20 + rand%20` (20–39), **0 if team already has ≥ `VIRII_POPULATIONLIMIT` (250)** | 0, 100 | [C] `TriffidEgg::Spawn` |
| 1 | `SpawnCentipede` | one Centipede unit of `size` segments (unit created on egg team) | `5 + rand%5` (5–9) | `5 + rand%10` (5–14) | 0, 200 | |
| 2 | `SpawnSpider` | 1 Spider | 1 | 1 | 0, 150 | |
| 3 | `SpawnSpirits` | spirits, horizontal speed `20 + syncfrand(20)` | `5 + rand%5` | same | – | |
| 4 | `SpawnEggs` | Virii `Egg`s, velocity up+random, length `20 + syncfrand(20)` | `5 + rand%5` | same | 0, 0 | |
| 5 | `SpawnTriffidEggs` | TriffidEggs, speed `75 + syncfrand(50)`; each egg type = `syncrand() % 9` (never Darwinians); in MP a SoulDestroyer roll is re-rolled in Assault or when ≥ 2 Soul Destroyers exist | `2 + rand%3` (2–4) | same | 0, 0 | |
| 6 | `SpawnAnts` | ArmyAnts thrown up, speed `10 + syncfrand(20)` (no hill: they roam via `BaseDestroyed`) | `20 + rand%10` (20–29) | same | 0, 0 | |
| 7 | `SpawnSporeGenerator` | 1 SporeGenerator | 1 | 1 | 0, 200 | |
| 8 | `SpawnSoulDestroyer` | 1 SoulDestroyer **on the Monster team in MP**; allowed if (not Assault **and** `SoulDestroyer::s_numSoulDestroyers < 2`) or single-player; otherwise **falls through to `SpawnVirii`** | 1 | 1 | 0, 200 | |
| 9 | `SpawnDarwinians` | Darwinians thrown up, speed `10 + syncfrand(20)` | `10 + rand%10` (10–19) | same | 0, 0 | |
(`rand` = `syncrand()`.) A commented-out `SpawnTripod` case exists. [C] `code/worldobject/triffid.cpp:TriffidEgg::Spawn`.

### TriffidEgg stats and behaviour
| Item | Value | Ref |
|---|---|---|
| Health | 50 (`data/stats.txt` TriffidEgg) | [C] `data/stats.txt` |
| Speed / Rate (blueprint) | 100 / 1 — not used by the egg logic | [C] `data/stats.txt`; `TriffidEgg::Advance` |
| Default burst timer | `m_life = 20 + syncfrand(10)` s (SP); overridden to `4 + syncfrand(4)` s by MP launches and by `RunEggs` | [C] `TriffidEgg::TriffidEgg`, `Triffid::Launch`, `Crate::RunEggs` |
| Gravity | `vel.y -= 9.8 × m_force` **per tick** (≈ 98 m/s² at `m_force=1`, not scaled by tick length) | [C] `TriffidEgg::Advance` |
| Bounce | when `y < ground + 3`: `WorldObject::BounceOffLandscape`, `m_force *= 0.65` (`TRIFFIDEGG_BOUNCEFRICTION`), `vel *= m_force`; sound `"Bounce"` while `m_force > 0.1` | [C] `TriffidEgg::Advance`, `code/worldobject/triffid.h` |
| Tumble | `m_up` rotated about `(up × vel)` by `m_force² × 30 × dt`; self-rights when `up.y < 0.3` and `m_force < 0.4` | [C] `TriffidEgg::Advance` |
| Damage | **immune to `DamageTypeLaser`**; otherwise generic death (spirit) + shape explosion | [C] `TriffidEgg::ChangeHealth` |
| Burst | at `now > m_timerSync`: shape explosion, `Spawn()`, sound `"BurstOpen"`, egg removed | [C] `TriffidEgg::Advance` |
| Visual | `triffidegg.shp` (team-coloured, `SetShape(..., true)`), pulsating size `m_size + |sin 2t|×(1-age)×0.4`, shadow `size×10` | [C] `TriffidEgg::Render` |

```mermaid
stateDiagram-v2
    [*] --> Flying: spawned with velocity
    Flying --> Bouncing: y < ground+3 (force *= 0.65)
    Bouncing --> Flying: still has velocity
    Flying --> Burst: now > timer
    Bouncing --> Burst: now > timer
    Flying --> Dead: health <= 0 (non-laser damage)
    Bouncing --> Dead: health <= 0
    Burst --> [*]: Spawn(type); removed
    Dead --> [*]: spirit + explosion; fade
```

## [→ 02_units_and_entities] Egg (Virii egg) — spirit-fertilised hatchery

### Role and build tag
Small sprite egg that, when a spirit is placed inside it, hatches 4 Virii after 15 s. **[B:FULL]** reachable in Multiwinia only indirectly: laid by **SporeGenerators** and **Spiders**, and thrown by `TriffidEgg` spawn type `SpawnEggs` (which crate Triffids/Eggs exclude — so in Multiwinia only via SporeGenerator/Spider, or a `SpawnTriffidEggs` sub-egg that rolls `SpawnEggs`). `TypeEgg` is in `EntityIsBlocked` (level-file only). [C] `code/worldobject/sporegenerator.cpp:SporeGenerator::AdvanceEggLaying`, `code/worldobject/spider.cpp:Spider::AdvanceEggLaying`, `code/worldobject/triffid.cpp:TriffidEgg::Spawn`.

### Stats
| Item | Value | Ref |
|---|---|---|
| Health | 10 (`data/stats.txt`) | [C] |
| Speed / Rate | 0 / 0 | [C] `data/stats.txt` |
| `EGG_DORMANTLIFE` | 120 s (`#define`) | [C] `code/worldobject/egg.h` |
| Hatch time | 15.0 s after fertilisation | [C] `Egg::Advance` |
| Hatch output | 4 Virii (egg team), spread 0, roam range 200, only if team Virii population < 250 | [C] `Egg::Advance` |

### States (`Egg::StateDormant`, `StateFertilised`, `StateOpen`)
```mermaid
stateDiagram-v2
    [*] --> Dormant
    Dormant --> Dormant: timer += dt; health capped to 10*(1 - timer/120) (decays to 0 at 120 s)
    Dormant --> Fertilised: Egg::Fertilise(spiritId) (spirit -> StateInEgg, health reset 10, timer 0)
    Fertilised --> Dormant: held spirit index no longer valid
    Fertilised --> [*]: timer >= 15 -> spirit removed, 4 Virii spawned (if pop < 250), egg removed
    Dormant --> Dead: health < 0 / falls below y=0 (ChangeHealth -500)
    Fertilised --> Dead: health < 0
    Dead --> [*]: held spirit -> Spirit::EggDestroyed (floating again); fade 2.5 s
```
- `StateOpen` is declared but never entered. [C] `code/worldobject/egg.h`, `Egg::Advance`.
- **Death override**: `Egg::ChangeHealth` kills only when `health + amount < 0` (strictly), does **not** spawn a spirit and plays no sound (unlike `Entity::ChangeHealth`). [C] `Egg::ChangeHealth`.
- While fertilised, the spirit is pinned 3 m above the egg each tick. [C] `Egg::Advance`.
- **Fertilisers**: Virii carrying a spirit (`Virii::AdvanceToEgg`), SporeGenerator (25% of laid eggs pre-fertilised with a fresh spirit), spirits themselves track nearby eggs (`Spirit::SearchForNearbyEggs`, range `VIRII_MAXSEARCHRANGE`). [C] `code/worldobject/virii.cpp:Virii::AdvanceToEgg`, `code/worldobject/sporegenerator.cpp:SporeGenerator::AdvanceEggLaying`, `code/worldobject/spirit.cpp:Spirit::SearchForNearbyEggs`.
- **Physics**: ballistic via `Entity::AdvanceInAir` until grounded; snaps to ground; `y <= 0` (water) → `ChangeHealth(-500)`. [C] `Egg::Advance`.
- **Visual**: camera-facing 8×8 m (`size 4`) additive sprite `sprites/egg.bmp`, alpha = health/10 (min 0.1); fertilised eggs add two throbbing sprites (alpha `|sin t|×8×t`, growing over the 15 s); dead eggs break into 3 drifting quarter-sprite fragments. Team colour is fetched but the sprite is drawn white. [C] `Egg::Render`.

## [→ 02_units_and_entities] Virii, ViriiUnit, ViriiHistory — worm swarm

### Role and build tag
Fast red worm-like creatures leaving a glowing trail; they bite enemies (20 HP), and ferry floating spirits into Eggs to breed. **[B:FULL]** reachable via: TriffidEgg `SpawnVirii` (Eggs/Triffids crates; also the fallback when a Soul Destroyer is disallowed), Egg hatching, and the debug cheat window [B:DEBUG]. **SpamInfection** particles converting spirits (Infection crate — see Spam section), and the debug cheat window [B:DEBUG] (`CHEATMENU_ENABLED`). Cave (`code/worldobject/cave.cpp`, blocked building) and Shaman (blocked) paths are [B:LEGACY]. Darwinia campaign maps place 118 Virii [B:LEGACY]. `TypeVirii` is **not** blocked. [C] `code/worldobject/triffid.cpp:TriffidEgg::Spawn`, `code/worldobject/egg.cpp:Egg::Advance`, `code/worldobject/spam.cpp:SpamInfection::AdvanceAttackingSpirit`, `code/interface/cheat_window.cpp:CheatWindow` (L141, inside `#ifdef CHEATMENU_ENABLED`), `code/worldobject/cave.cpp` (L30).

### Classes
| Class | Base | Purpose | Ref |
|---|---|---|---|
| `ViriiUnit` | `Unit` | Created by `Team::NewUnit` for `TypeVirii`; each Advance sets `m_enemiesFound = AreEnemiesPresent(centre, radius + 60, team)` (cheap pre-check before individual searches); renders all its Virii in one additive `GL_QUADS` batch with `sprites/viriifull.bmp`, choosing detail 1–4 from `RenderEntityDetail` pref and camera distance (detail 2 beyond 1000 m at pref 1; 3 beyond 1000 m / 2 beyond 500 m at pref 2; 4 always at pref 3). `m_cameraClose` is unused. | [C] `code/worldobject/virii.cpp:ViriiUnit::Advance`, `ViriiUnit::Render`; `code/team.cpp:Team::NewUnit` |
| `Virii` | `Entity` | The worm. | [C] |
| `ViriiHistory` | – | One trail sample: `m_pos`, `m_right` (perpendicular on land normal), `m_glowDiff` (10 m step vector), `m_distance` (to previous sample), `m_required` (true = direction change/state sample; false = smoothing for height change). | [C] `code/worldobject/virii.h` |
Virii not in a unit (spawned with unitId −1 by eggs/triffids) live in the team's `m_others` list and are batch-rendered by `Team::RenderVirii` (same detail rules). [C] `code/team.cpp:Team::RenderVirii`.

### Stats and constants
| Item | Value | Ref |
|---|---|---|
| Health | 10 | [C] `data/stats.txt` |
| Speed | 32 m/s; ×slope factor `clamp(1 + Δh/5, 0.1, 2.0)` where Δh = next−current terrain height (uphill slower, downhill faster) | [C] `data/stats.txt`, `Virii::AdvanceToTargetPos` |
| Rate | 0 | [C] |
| Hover height | 0.1 m | [C] `Virii::Virii` |
| `VIRII_MINSEARCHRANGE` / `MAXSEARCHRANGE` | 30 / 60 m (`#define`) | [C] `code/worldobject/virii.h` |
| `VIRII_TAILLENGTH` | 175 m (the code uses a literal 175.0) | [C] `Virii::RecordHistoryPosition` |
| `VIRII_POPULATIONLIMIT` | 250 per team (`#define`) | [C] `code/worldobject/virii.h` |
| Bite | `ChangeHealth(-20)` on arrival at the enemy (unresistable type) | [C] `Virii::AdvanceAttacking` |
| Retarget period (idle) | 1.0 s (first after `syncfrand(2.0)`) | [C] `Virii::AdvanceIdle`, `Virii::Virii` |
| History sample period (non-idle) | 0.5 s, plus extra non-required samples when height differs > 3 m | [C] `Virii::Advance` |
| Fall gravity | −10 m/s², lands at ground + 2 | [C] `Virii::Advance` |
| Population counter | `Virii::s_viriiPopulation[team]` ++ in `Begin`, −− in destructor; excluded from AntHill pop-cap counting | [C] `Virii::Begin`, `Virii::~Virii`, `AntHill::PopulationLocked` |

### State machine (`Virii::StateIdle`, `StateAttacking`, `StateToSpirit`, `StateToEgg`)
```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> ToEgg: retarget & carrying spirit & dormant on-ground friendly egg within 60m (random pick)
    Idle --> Attacking: retarget & unit sees enemies & nearest live on-ground enemy at 30..60m
    Idle --> ToSpirit: retarget & floating/birth spirit within 60m that has nearby eggs
    Idle --> Idle: retarget & SearchForIdleDirection (new waypoint)
    Attacking --> Idle: enemy gone/dead
    Attacking --> Attacking: arrived -> enemy -20 HP, 3 muzzle flashes, sound Attack, re-search (next enemy 30..60m)
    ToSpirit --> Idle: spirit gone/dead/attached/in egg
    ToSpirit --> ToEgg: arrived -> Spirit::CollectorArrives (attached)
    ToEgg --> Idle: no egg found (drops spirit) or spirit invalid
    ToEgg --> Idle: arrived -> Egg::Fertilise(spirit)
```
[C] `code/worldobject/virii.cpp:Virii::AdvanceIdle`, `AdvanceAttacking`, `AdvanceToSpirit`, `AdvanceToEgg`, `SearchForEnemies`, `SearchForSpirits`, `SearchForEggs`, `FindNearbyEgg(Vector3)`.
- **Bite cadence**: after a bite the Virii looks for the nearest enemy at ≥ 30 m; if none is found it keeps the current target and, being on top of it, bites again on the next tick (−20 HP per 0.1 s). [I] from `Virii::AdvanceAttacking` + `EntityGrid::GetBestEnemy` min-range rule (`code/entity_grid.cpp:EntityGrid::GetBestEnemy`).
- **Idle wandering** (`SearchForIdleDirection`): return-to-spawn chance `d/roamRange` (forced 0 below 0.75); returning picks one of 8 compass directions toward spawn; otherwise up to 3 tries of a random compass direction `(int(syncfrand(3))-1, 0, int(syncfrand(3))-1)` whose endpoint (`speed × retargetTimer` ahead) is above sea level. Each new direction records a required history point and plays `"ChangeDirection"`. [C] `Virii::SearchForIdleDirection`.
- **Spirits targeted**: only spirits with `NumNearbyEggs() > 0` in `StateBirth`/`StateFloating`. Carried spirits follow at `pos + (0,2,0)`; dropped (`CollectorDrops`) when the Virii dies. [C] `Virii::SearchForSpirits`, `Virii::Advance`.
- **Trail**: `m_positionHistory` (newest first) is trimmed once its summed length exceeds 175 m. Rendered as a 3 m-half-width textured worm strip (alpha `wormColour.a − cumulative distance`) plus a 12 m-half-width glow strip (skipped at detail 4). Lower detail levels drop non-required samples and render only `1 − 0.25×min(detail−1,2)` of the history. [C] `Virii::RecordHistoryPosition`, `Virii::Render`.
- **Team colouring**: worm colour = team colour × health fraction (alpha 200); glow = RGB(200,100,100) single-player / team colour multiplayer (alpha 150). **Monster-team Virii are rendered white/transparent (`wormColour (255,255,255,0)`, glow `(200,200,200,50)`)**. Dead Virii fade as near-white (alpha ∝ remaining fade health). [C] `Virii::Render`.
- **Death**: generic `Entity::ChangeHealth` (spirit spawned, `"Die"`), velocity zeroed, then 2.5 s fade via `Entity::AdvanceDead`. `Virii::AdvanceDead()` (trail scatter in 8 compass directions at 10 m/s) is **never called** (it does not override the `Entity::AdvanceDead(Unit*)` signature). [C] `Virii::Advance`, `Virii::AdvanceDead`, `Virii::AdvanceDeadPositionVector`; grep shows no caller.
- **Push immunity**: a laser hit does not push or turn a Virii (it still takes damage). [C] `code/worldobject/weapons.cpp:Laser::Advance` (L782–788 skip `TypeVirii`).
- **Map clamp** repeated in `Virii::Advance`. Water damage is commented out (Virii can cross water). [C] `Virii::Advance`.
- **Sounds**: `"ChangeDirection"`, `"Attack"`, generic `"LoseHealth"`, `"Die"`. **Effects**: `Particle::TypeMuzzleFlash` ×3 per bite.

## [→ 02_units_and_entities] SporeGenerator — flying egg layer

### Role and build tag
A hovering creature with four tails that drifts around its spawn point and lays Virii Eggs near floating spirits (and at random). **[B:FULL]** reachable via TriffidEgg `SpawnSporeGenerator` (Eggs and Triffids crates). Shaman path and campaign placements (26) [B:LEGACY]. Not blocked. [C] `code/worldobject/triffid.cpp:TriffidEgg::Spawn`.

### Stats and constants
| Item | Value | Ref |
|---|---|---|
| Health | 200 | [C] `data/stats.txt` |
| Speed | 20 m/s (scaled by `dist/50` within 50 m of target) | [C] `data/stats.txt`, `SporeGenerator::AdvanceToTargetPosition` |
| Rate | 10 (unused) | [C] `data/stats.txt` |
| `SPOREGENERATOR_HOVERHEIGHT` | 100 m (`#define`, cpp) — **not applied on spawn in multiplayer** (spawns at ground level, rises toward targets) | [C] `SporeGenerator::Begin` |
| `SPOREGENERATOR_EGGLAYHEIGHT` | 20 m | [C] |
| `SPOREGENERATOR_SPIRITSEARCHRANGE` | 200 m | [C] |
| `SPOREGENERATOR_NUMTAILS` | 4 (`#define`, h) | [C] |
| Retarget period (idle) | 6.0 s | [C] `SporeGenerator::AdvanceIdle` |
| Egg interval | `2 + syncfrand(2) − difficulty/10` s while within 50 m of the lay target | [C] `SporeGenerator::AdvanceEggLaying` |
| Pre-fertilised eggs | 25% (`syncrand()%4 == 0`) — a new spirit is spawned in the egg | [C] |
| Random lay chance | `10 + 20×difficulty/10` % per random retarget | [C] `SporeGenerator::SearchForRandomPos` |
| Panic | 10 s after any hit ≥ 2 HP; velocity set to `up × speed × 3` | [C] `SporeGenerator::ChangeHealth` |

`g_app->m_difficultyLevel` is 0 in Multiwinia [I]: `App` initialises it to 0 (`code/app.cpp`, L632) and `Location::Init` copies the level-file value only when non-negative (`code/location.cpp`, L348–356).

### State machine (`StateIdle`, `StateEggLaying`, `StatePanic`)
```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> EggLaying: retarget(6s) & floating spirit within 200m with <3 nearby eggs and y>0 (target = spirit +/-30m, 20m up)
    Idle --> EggLaying: random retarget & lay-chance roll
    Idle --> Idle: random retarget (200m box or 200m back toward spawn; 100m*(1+-0.15) up)
    EggLaying --> EggLaying: within 50m: lay egg every 2-3 s
    EggLaying --> EggLaying: arrived(<10m) -> SearchForSpirits / SearchForRandomPos
    EggLaying --> Idle: arrived & random pos chosen without lay roll
    Idle --> Panic: hit (non-laser, amount < -1)
    EggLaying --> Panic: hit
    Panic --> Panic: jitter target +/-50m each tick
    Panic --> Idle: 10 s elapsed
```
- **Steering**: `vel = vel×(1 − dt/2) + targetVel×dt/2`, rotated around the right axis by `sin(3·now)×dt×1.5` (deterministic wobble); arrival < 10 m. [C] `SporeGenerator::AdvanceToTargetPosition`.
- **Return-to-spawn rule**: chance `d/roamRange` (roam range 200 when hatched from an egg). [C] `SporeGenerator::SearchForRandomPos`.
- **Damage**: **immune to lasers**; hits of exactly −1 are ignored; others apply generic damage, play a partial shape explosion (fraction dead) and trigger Panic. Death → generic spirit + `"Die"`. [C] `SporeGenerator::ChangeHealth`.
- **Eggs laid** at shape marker `MarkerEggs`, inheriting the generator's velocity, team = generator team. Sound `"LayEgg"`. [C] `SporeGenerator::AdvanceEggLaying`.
- **Visual**: `sporegenerator.shp` (not team-coloured: `SetShape(..., false)`), four 3-segment quad-strip tails in team colour from markers `MarkerTail01`–`04`, animated with sin offsets and a function-static smoothed velocity (shared between all generators — render-only quirk); damaged flicker; 20 m shadow. [C] `SporeGenerator::Render`, `SporeGenerator::RenderTail`.

## [→ 02_units_and_entities] Centipede — segmented ground predator

### Role and build tag
A chain of linked segments (one Entity per segment, all in one Unit) led by a head that hunts enemies, eats floating spirits and grows. **[B:FULL]** reachable via TriffidEgg `SpawnCentipede` (Eggs and Triffids crates). Campaign InstantUnits (55) and Shaman are [B:LEGACY]. `TypeCentipede` is **not** blocked, and `LevelFile::ParseInstantUnits` has a difficulty-based splitting rule for long centipedes (campaign only). [C] `code/worldobject/triffid.cpp:TriffidEgg::Spawn`, `code/level_file.cpp:LevelFile::ParseInstantUnits`. Must be created in a Unit (`AppReleaseAssert`). [C] `Centipede::Advance`.

### Stats and constants
| Item | Value | Ref |
|---|---|---|
| Base health | 255 (stats) × `2 × m_size`, clamped [0,255] per segment (≥ 255 when size ≥ 0.5; tail size 0.2 → 102) | [C] `data/stats.txt`, `Centipede::Begin` |
| Speed | 80 m/s × slope factor `clamp(1 + Δh/10, 0.6, 1.0)` (only uphill slows) | [C] `data/stats.txt`, `Centipede::AdvanceToTargetPosition` |
| Rate | 1 (unused) | [C] |
| Turn blend | `dt × 3` per tick for the head (`× 1.5` if trailing) | [C] `Centipede::AdvanceToTargetPosition` |
| `CENTIPEDE_MINSEARCHRANGE` / `MAXSEARCHRANGE` | 75 / 150 m (enemy search scaled by head `m_size`; spirit search unscaled) | [C] `code/worldobject/centipede.h` |
| `CENTIPEDE_SPIRITEATRANGE` | 10 m (horizontal) | [C] |
| `CENTIPEDE_NUMSPIRITSTOREGROW` / `_MP` | 4 / 4 | [C] |
| `CENTIPEDE_MAXSIZE` | 20 segments (no eating or spirit seeking above this) | [C] |
| Eat chance | `m_size/2` per candidate spirit per tick, at most one spirit per tick | [C] `Centipede::EatSpirits` |
| Contact damage | every segment, every tick: enemies within `m_radius` (= `10 × m_size` fixed at `Begin`) are pushed out to the radius and take `int((radius − distance) × 10)` damage | [C] `Centipede::Attack` |
| Panic | `2 + syncfrand(2)` s when a hit drops a segment into a lower third of its max health; propagated to every segment toward the head | [C] `Centipede::ChangeHealth`, `Centipede::Panic` |
| Water/building avoidance | a target with y < 0, or a building within 150 m ahead (`Location::GetBuildingId(pos, front, 255, 150)`), counts as "arrived" (forces a retarget) | [C] `Centipede::AdvanceToTargetPosition` |
| Arrival | < 20 m | [C] |

### Segment logic
- **Linking at spawn** (`Centipede::Begin`): when a segment begins without a leader, all segments of its Unit are relinked in unit-array order: entity 0 is the **head** (no `m_next`), each next entity follows the previous one (`m_next` = guy in front, `m_prev` = guy behind). Sizes: head `min(0.2 × 1.1^N, 10)`, each following segment `/1.1`. Segment shapes: per-team recoloured `centipedehead.shp` / `centipede.shp` (cached `<name>_<colourId>_<groupId>`, statics `s_shapeHead[team]`, `s_shapeBody[team]`). [C] `Centipede::Begin`.
- **Size convergence** every tick: tail target size 0.2; others `min(1.1 × size_of_segment_behind, 1.0)`; size moves 10% toward target when off by > 0.01, health rescaled to keep the same fraction. ⇒ steady state tail 0.2, growing ×1.1 per segment toward the head, capped 1.0. [C] `Centipede::Advance`.
- **Following**: a trailing segment copies its leader's interpolated trail point: with the leader's last 3 recorded positions (`RecordHistoryPosition` keeps 3, newest first), `pos = h[k+1] + (h[k] − h[k+1]) × (1 − m_size)`, `vel = (h[k] − h[k+1]) / dt`, where `k = 0` if the leader's entity index is greater than the follower's (leader not yet advanced this tick) else 1. It inherits the leader's `m_panic`. `m_linked` (render gate) requires the leader linked and ≥ 2 own history samples. If the leader dies the segment becomes a new head. [C] `Centipede::Advance`, `Centipede::GetTrailPosition`, `Centipede::RecordHistoryPosition`.
- **Death of a segment**: generic death (spirit), scaled shape explosion, unlinks from both neighbours (`next->m_prev` invalidated) — so a centipede **splits** into two independent centipedes when a middle segment dies. In single-player each health-band drop also plays an explosion. Dead segments run `Entity::AdvanceDead` only. [C] `Centipede::ChangeHealth`, `Centipede::Advance`.
- **Growth** (`EatSpirits`, head only): after eating ≥ 4 spirits, appends `floor(eaten/4)` new segments behind the current tail (new Unit entity, size = tail size ≥ 0.2, placed at `m_spawnPoint`, same roam settings), sound `"Grow"`. [C] `Centipede::EatSpirits`.
- **GetSize** counts segments from this one backwards through `m_prev`. [C] `Centipede::GetSize`.

### Head state machine
```mermaid
stateDiagram-v2
    [*] --> Roam
    Roam --> Hunt: arrived/no target & GetBestEnemy(75*size..150*size) found (sound EnemySighted)
    Roam --> SeekSpirit: no enemy & floating spirit at 75..150m & size<=20
    Roam --> Roam: else SearchForRandomPosition (100m step; return to spawn with chance d/roamRange)
    Hunt --> Hunt: target pos refreshed each tick (ground height)
    Hunt --> Roam: target gone / arrived (re-search)
    SeekSpirit --> Roam: arrived
    Roam --> Panic: m_panic > 0
    Hunt --> Panic: m_panic > 0
    SeekSpirit --> Panic: m_panic > 0
    Panic --> Panic: 50%/tick pick retreat point 50m away from nearest enemy within 150*size (+-90deg jitter)
    Panic --> Roam: m_panic <= 0
```
[C] `Centipede::Advance`, `SearchForTargetEnemy`, `SearchForSpirits`, `SearchForRandomPosition`, `SearchForRetreatPosition`.
- Every tick (head and body) `EatSpirits` (head only) and `Attack(m_pos)` run regardless of state.
- **Sounds**: `"Panic"` (on entering panic), `"EnemySighted"`, `"Grow"`, `"Attack"` (each contact), generic `"LoseHealth"`/`"Die"`. **Effects**: shape explosions; terrain-normal-aligned rendering; a commented-out D3D tearing effect.

## [→ 02_units_and_entities] Spider — eight-legged pouncer

### Role and build tag
Large walker with 8 procedurally animated legs that pounces on enemies and lays Virii eggs near spirits. **[B:FULL]** reachable via TriffidEgg `SpawnSpider` (Eggs/Triffids crates) and **SpamInfection** (5% of spirit conversions, Infection crate). Campaign (45) and Shaman [B:LEGACY]. Not blocked. [C] `code/worldobject/triffid.cpp:TriffidEgg::Spawn`, `code/worldobject/spam.cpp:SpamInfection::AdvanceAttackingSpirit`.

### Stats and constants
| Item | Value | Ref |
|---|---|---|
| Health | 200 | [C] `data/stats.txt` |
| Speed (blueprint 50) | **unused**; walking speed is `min(distance, 30)` m/s | [C] `Spider::AdvanceToTarget` |
| Rate | 1 (unused) | [C] |
| `SPIDER_NUM_LEGS` | 8 | [C] `code/worldobject/spider.h` |
| `HOVER_HEIGHT` | 3.0 m (body follows terrain + 3 with blend `dt` per tick) | [C] `code/worldobject/spider.cpp`, `Spider::Advance` |
| `TURN_RATE` | 0.1 rad/tick (0.05 when dot > 0.9; must face within dot 0.999 before `FaceTarget` succeeds; moves only when dot > 0.5) | [C] `Spider::FaceTarget`, `Spider::AdvanceToTarget` |
| `ATTACK_SEARCH_MIN/MAX_RADIUS` | 100 / 250 m | [C] |
| `SPIRIT_MIN/MAXSEARCHRANGE` | 30 / 100 m (spirits higher than 10 m with < 3 nearby eggs) | [C] `Spider::SearchForSpirits` |
| `MAX_PATH_HEIGHT` | 100 m (random wander target rejected if the path crosses terrain > 100 m or < 0.1 m; 10 tries) | [C] `Spider::IsPathOK`, `Spider::SearchForRandomPos` |
| Pounce trigger distance | ≤ 150 m from pounce target (and facing) | [C] `Spider::AdvanceAttack` |
| Pounce launch | `vel = normalise(dir with y = 0.5) × 24 × sqrt(distance)` | [C] |
| Pounce gravity | `vel.y -= 40` **per tick** | [C] `Spider::AdvancePouncing` |
| Landing squash | enemies within 40 m: `fraction = (40 − d)/40 × (1 + syncfrand(0.3))`; those within **20 m** take `100 × fraction` damage; all are pushed (`front` with y = 5×|front|, length `min(30×fraction, 20)`) and made airborne | [C] `Spider::AdvancePouncing` |
| Retarget | 5 s idle/egg-laying; 3 s set at pounce; 6 s after landing | [C] |
| Spider–spider avoidance | if another spider within 22 m of `pos + vel`, retarget 40 m along the summed escape vector (uses `syncrand()%numFound`, bug: picks a random neighbour, not the spider found) | [C] `Spider::DetectCollisions` |
| `NORMAL_SPEED` 300, `ATTACK_SPEED` 90, `FOOT_MOVE_THRESHOLD` 5, `FOOT_EMERGENCY_THRESHOLD` 12, `FOOT_DAMAGE_RADIUS` 20, `FOOT_DAMAGE_STRENGTH` 10, `ATTACK_PREAMBLE` 5 | `NORMAL_SPEED`, `ATTACK_SPEED`, `ATTACK_PREAMBLE` unused; foot thresholds drive the gait; `StompFoot` (foot damage) is never called | [C] `code/worldobject/spider.cpp` defines, `Spider::UpdateLegs` (call commented out) |

### State machine (`StateIdle`, `StateEggLaying`, `StateAttack`, `StatePouncing`)
```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> Attack: retarget & enemy at 100..250m (pounce target = its position then)
    Idle --> EggLaying: retarget & suitable spirit (target = 45m beyond spirit)
    Idle --> Idle: retarget & random reachable point within roamRange of spawn
    EggLaying --> Attack: retarget(5s) & enemy found
    EggLaying --> Idle: arrived (<5m) -> lay one Egg (sound LayEgg)
    Attack --> Attack: turn to face; if >150m walk toward
    Attack --> Pouncing: facing & <=150m (sounds Attack, Pounce)
    Pouncing --> Idle: lands (y < ground+1): squash, sound PounceLand
```
[C] `code/worldobject/spider.cpp:Spider::Advance`, `AdvanceIdle`, `AdvanceEggLaying`, `AdvanceAttack`, `AdvancePouncing`.
- The pounce target is a **position snapshot** taken at search time; the spider does not track the moving enemy. [C] `Spider::SearchForEnemies`.
- **Gait** (`Spider::UpdateLegs`): up to two feet lifted per tick, chosen by `EntityLeg::CalcFootsDesireToMove` (> 12 = emergency lift, best > 5 lifted, then `m_delayBetweenLifts` lock-out); parameter set chosen by the nearest ideal speed: {10 m/s: swing 0.4 s, lift delay 0.11 s, look-ahead 0.75}, {30: 0.25, 0.06, 0.3}, {50: 0.25, 0.04, 0.2}; leg lift 3.0, ideal slope `2.6 × 2.5`. Sound `"FootFall"` when a foot plants. During a pounce legs follow `EntityLeg::AdvanceSpiderPounce(2 × elapsed)`. Leg geometry/IK lives in `code/worldobject/entity_leg.cpp` (other partition). [C] `Spider::UpdateLegs`, `Spider::CalcWhichFootToMove`, `Spider::Spider`.
- **Damage**: **immune to lasers**; hits of −1 ignored; others generic + partial shape explosion. Death → spirit, `"Die"`. [C] `Spider::ChangeHealth`.
- **Team colouring**: `spider.shp` fully team-coloured (`SetShape(..., true)`); legs `spider_leg_upper.shp`/`spider_leg_lower.shp`; markers `MarkerEggLay`, `MarkerLeg0`–`7`. Damaged flicker scales the body. A D3D-only tearing deform effect follows the pounce [B:D3D]. [C] `Spider::Begin`, `Spider::Render`, `Spider::AdvancePouncing` (`#ifdef USE_DIRECT3D`).
- **Sounds**: `"Pounce"`, `"PounceLand"`, `"FootFall"`, `"LayEgg"`, `"Attack"`.

## [→ 04_crates_powerups_and_weapons] SpamInfection — "Infection" crate swarm (and the legacy Spam building)

### Role and build tag
- **SpamInfection** (`WorldObject`, effect type `EffectSpamInfection`, lives in `Location::m_effects`) — a short-lived homing red/black streak that kills or converts enemies and turns floating spirits into monster Virii/Spiders. **[B:FULL]** live as the crate reward **`CrateRewardSpam`** ("Infection"): captured → task `GlobalResearch::TypeInfection` → `Task::TargetInfection` → `Crate::RunInfection(pos, team)`; uncaptured → `Crate::RunInfection(pos, monsterTeam)`; the AI also uses the task (`AI::RunSpam`). [C] `code/worldobject/crate.cpp:Crate::CaptureCrate`, `Crate::RunUncapturedCrate`, `Crate::RunInfection`; `code/taskmanager.cpp:Task::TargetInfection`; `code/worldobject/ai.cpp:AI::AdvanceCurrentTasks` (L1251).
- **Spam** (`Building`, `TypeSpam`) — the Darwinia "SPAM" research-item-looking pod that periodically emits 20 SpamInfections. **[B:LEGACY]**: `TypeSpam` is in `Building::BuildingIsBlocked`; only Darwinia maps (3) and `GodDish` (also blocked) create it. [C] `code/worldobject/building.cpp:Building::BuildingIsBlocked`, `code/worldobject/goddish.cpp:GodDish::SpawnSpam` (L210).

### `Crate::RunInfection(pos, teamId)` [C] `code/worldobject/crate.cpp:Crate::RunInfection`
- Creates **100** SpamInfections at `pos`, **all owned by the Monster team** (id set to `(monsterTeam, UNIT_EFFECTS, index)`), `m_parentId = -1`, initial velocity `normalise(up + (SFRAND(1), FRAND(2), SFRAND(1))) × 100`.
- Marker `icons/icon_infection.bmp`, message `crate_message_auto_infection`, sound event `CrateRewardSpam`.
- Note: the triggering team gets no protection — the swarm attacks every non-monster team including the user. [I] from team assignment + `SpamInfection::AdvanceAttackingEntity` validity rule.

### SpamInfection constants
| Item | Value | Ref |
|---|---|---|
| `SPAMINFECTION_LIFE` | 10.0 s; **+1.0 s per successful hit/conversion** | [C] `code/worldobject/spam.h`, `SpamInfection::AdvanceAttackingEntity` |
| `SPAMINFECTION_MIN/MAXSEARCHRANGE` | 100 / 200 m (both entities and spirits) | [C] |
| `SPAMINFECTION_TAILLENGTH` | 30 history samples | [C] |
| Cruise | steer toward target at 200 m/s with blend `dt × 0.5` per tick; hit radius 20 m | [C] `SpamInfection::AdvanceToTargetPosition` |
| Retarget | every 1.0 s (first after `syncfrand(1)`); in MP the first retarget skips searching (`m_startCounter = 1`) | [C] `SpamInfection::SpamInfection`, `AdvanceIdle` |
| Search order | 50/50 (`syncfrand(2) < 1`): entities-then-spirits or spirits-then-entities; fallback random point (±100 m cube, not below ground; or back to parent Spam if > 400 m away) | [C] `SpamInfection::AdvanceIdle`, `SearchForRandomPosition` |
| Entity target | `GetBestEnemy(100..200 m)` relative to team 1 (SP) / Monster team (MP); Harvesters are dropped as targets | [C] `SpamInfection::SearchForEntities`, `AdvanceAttackingEntity` |
| Entity hit (MP) | if target team ≠ infection team: if Darwinian research level > 2 (always in MP: set to 4 by `Location::Init`, L377) and `syncfrand(10) < 5` (**50%**) → target replaced by a **Monster-team Darwinian** (target `m_destroyed`, no spirit); else `ChangeHealth(-999)` | [C] `SpamInfection::AdvanceAttackingEntity`; `code/location.cpp:Location::Init` |
| Spirit target | floating spirit 100..200 m away and > 10 m high (SP skips spirits 50% of the time) | [C] `SpamInfection::SearchForSpirits` |
| Spirit hit | spirit removed; spawns 1 Monster-team **Virii** (95%) or **Spider** (5%, `syncfrand(20) < 1`), roam 200; in MP a Virii is skipped if monster Virii ≥ 250 | [C] `SpamInfection::AdvanceAttackingSpirit` |
| Hit FX | `5 + AppRandom()%5` `Particle::TypeFire` bursts (unsynced count — cosmetic) | [C] |

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> AttackingEntity: retarget & enemy at 100..200m
    Idle --> AttackingSpirit: retarget & floating spirit at 100..200m
    Idle --> Idle: random point
    AttackingEntity --> Idle: target dead/gone/harvester, or hit (kill/convert, life+1)
    AttackingSpirit --> Idle: spirit gone/not floating, or hit (spawn Virii/Spider, life+1)
    Idle --> [*]: life <= 0
    AttackingEntity --> [*]: life <= 0
    AttackingSpirit --> [*]: life <= 0
```
- **Visual**: 4 passes of a camera-facing ribbon through the 30-sample history using `textures/laser.bmp`; single-player red additive; **multiplayer black, alpha-blended** (`(0,0,0, 0.5×alpha)`), length shrinking with remaining life. [C] `SpamInfection::Render`.

### Spam building (legacy) — brief
| Item | Value | Ref |
|---|---|---|
| `SPAM_RELOADTIME` | 60 s − 30 s × difficulty/10 (`SpamReloadTime`) between emissions of 20 infections (team = Spam team, sound `"Attack"`) | [C] `code/worldobject/spam.cpp:SpamReloadTime`, `Spam::SpawnInfection` |
| `SPAM_DAMAGE` | 100 "health"; explosion per 10-point band; at 0: the matching `GlobalBuilding` goes `m_online`, sound `"Explode"`, removed | [C] `Spam::Damage`, `Spam::Advance` |
| Movement | hovers 20 m above ground; `SendFromHeaven` drops it (inactive); slides with friction `dt×0.3`; pushes off other Spam | [C] `Spam::Advance`, `Spam::SendFromHeaven` |
| Research mode | `SetAsResearch` → blue glow, inactive, sound `"CreateResearch"` | [C] `Spam::SetAsResearch` |
| Visual | `researchitem.shp` tumbling, 20 `cloudyglow` blobs + 10 starbursts (red; blue if research) | [C] `Spam::Render`, `Spam::RenderAlphas` |
Determinism defects (legacy only): constructor uses unsynced `frand` for `m_front`; `Spam::Render` mutates `m_front/m_up`, which `Advance` uses for `CalculateCentre`. [C] `Spam::Spam`, `Spam::Render`.

## [→ 02_units_and_entities] WallBuster — Assault-mode carryable wall charge

### Role and build tag
A `CarryableBuilding` explosive that Darwinians carry (10–30 lifters) to breach Walls. **[B:FULL]** live in Assault: `data/levels/mp_assault_2P_1.txt` places 3 WallBusters (team 255). Referenced by help hints (`code/multiwiniahelp.cpp`), selection (`code/location_input.cpp`), AI (`code/worldobject/ai.cpp`) and Darwinian carrying (`code/worldobject/darwinian.cpp`). Not blocked. [C] `data/levels/mp_assault_2P_1.txt`; `code/worldobject/carryablebuilding.cpp` (L528 lists `TypeWallBuster` as carryable).

### Parameters
| Item | Value | Ref |
|---|---|---|
| Shape / scale | `wallbuster.shp`, `m_scale = 10` | [C] `WallBuster::WallBuster` |
| Lifters | min 10, max 30; `m_speedScale = 0.6` | [C] |
| Carry radius | 13 m (also `m_radius`) | [C] `WallBuster::Initialise` |
| Wall trigger | any `Wall` building in the obstruction cell whose shape `DoesSphereHit(pos, 20)` | [C] `WallBuster::Advance` |
| Breach | `ExplodeShape(1.0)`; **every Wall within 200 m takes `Damage(-1000)`**; `Location::Bang(pos, 50, 150)`; WallBuster destroyed; global message `dialog_wallbreach` | [C] `WallBuster::Advance`, `WallBuster::BreakWalls` |
| Defender scuttle | if owned by a **non-attacking** team (≠ 255) and carried by ≥ 10 lifters: `m_destructTimer` (10 s) counts down; at 0 → destroyed + `Bang(pos, 50, 500)`; resets to 10 s whenever the condition fails | [C] `WallBuster::Advance` |
| AI target | `m_aiTarget` follows the bomb if > 1.5 × carry radius away (creation code is commented out, so normally NULL) | [C] `WallBuster::Advance` |
| AI routing (`RunAI`) | when waypoint is unset/reached, once per second: route (`RoutingSystem::GenerateRoute`) to the nearest Wall that lies within 500 m of any marker of the AI's current `AIObjective` | [C] `WallBuster::RunAI` |
| Render | uses special lights (`SetupSpecialLights`) then restores | [C] `WallBuster::Render` |
Ownership/capture by carrying, lifter movement and drop rules are in `CarryableBuilding` (other partition). [I] team ownership = team of its lifters.

## [→ 04_crates_powerups_and_weapons] Meteor shower — `Crate::RunMeteorShower` and `Meteor`

### Build tag and acquisition
**[B:FULL]** live as crate reward `CrateRewardMeteor`: on capture the team gets the targetable task `GlobalResearch::TypeMeteorShower` (`Task::TargetMeteorShower` → `Crate::RunMeteorShower(pos, ownerTeam)`); also used by the AI (`AI::RunMeteor`, dispatched from `AI::AdvanceCurrentTasks`). It is **not** in the uncaptured-crate list. Requires `INCLUDE_CRATES_ADVANCED`. `TypeMeteor` is in `EntityIsBlocked` (level-file only). [C] `code/worldobject/crate.cpp:Crate::CaptureCrate`, `Crate::RunMeteorShower`; `code/taskmanager.cpp:Task::TargetMeteorShower`; `code/worldobject/ai.cpp:AI::AdvanceCurrentTasks` (L1257).

### Shower spawn
| Item | Value | Ref |
|---|---|---|
| Count | `5 + syncrand()%5` → **5–9 meteors** | [C] `Crate::RunMeteorShower` |
| Impact area | each target = `pos + (syncsfrand(420), syncsfrand(420))` → uniform in a **420 × 420 m square** (±210 m) centred on the chosen point; `420 = POWERUP_EFFECT_RANGE (70) × 6`; target y = terrain − 10 | [C] `Crate::RunMeteorShower`; `code/taskmanager.h:POWERUP_EFFECT_RANGE` |
| Start | `(FRAND(worldSizeX), 1500 + syncfrand(500), FRAND(worldSizeZ))` — a random point anywhere over the map at 1500–2000 m altitude | [C] |
| Owner | `teamId` passed in (the task owner) → kill credit to that team; damage is **not** team-filtered (friendly fire) | [C] `Crate::RunMeteorShower`; `code/location.cpp:Location::Bang` |
| Feedback | marker `icons/icon_meteorshower.bmp`; message `crate_message_run_meteor` (team name); crate sound `CrateRewardMeteor` | [C] |

### Meteor entity
| Item | Value | Ref |
|---|---|---|
| Speed | **350 m/s** read directly from `EntityBlueprint` (the per-entity byte stat would overflow) | [C] `Meteor::AdvanceToTarget`, `data/stats.txt` |
| Health | 255 (stats); generic damage applies but nothing in the design targets meteors [I] | [C] `data/stats.txt` |
| Radius | 20 m (`Begin`) | [C] `Meteor::Begin` |
| Flight | straight line start → target, capped at `350 × dt` per tick. Flight time = distance / 350 (e.g. 2 km diagonal ≈ 5.7 s at normal speed) | [C] `Meteor::AdvanceToTarget` |
| Impact condition | within 10 m of target **or** below terrain height | [C] |
| Impact | sound `"Land"`; **`Location::Bang(pos, range 50, damage 100, team)`**; meteor removed immediately | [C] `Meteor::Advance` |
| Bang semantics (owned by weapons/location partition) | entities within `2 × range` = 100 m take `100 × clamp(((100 − d)/50)×2, 0, 1)` (= full 100 damage out to 75 m, linear to 0 at 100 m) explosive damage and are pushed/airborne; buildings within `3 × range` = 150 m take `100 × (1 − d/150)` damage; shockwave `range/20`; kills of other teams credited | [C] `code/location.cpp:Location::Bang` |
| **Slow-motion** | the first time a meteor comes within 200 m of its target it calls `g_gameTimer.RequestSpeedChange(0.4, 0.1, 4.0)`: game speed eases toward 0.4× (10% of the gap per tick), holds 4.0 s worth of ticks, then returns to 1.0× | [C] `Meteor::Advance`; `code/gametimer.cpp:GameTimer::Tick`, `GameTimer::RequestSpeedChange` |
| Trail FX | 5 `Particle::TypeFire` per tick, size `300 + syncsfrand(60)` (consumes synced RNG), positions jittered with unsynced `sfrand` | [C] `Meteor::Advance` |
| Render | `meteor.shp` tumbling at 5 rad/s accurate time; 10 orbiting additive `textures/starburst.bmp` sprites RGBA(0.8,0.2,0.1,0.35) of size `4 × radius` = 80 | [C] `Meteor::Render` |
| `m_life` | 5.0, only meaningful after death; dead meteors return immediately (the decrement after `return true` is unreachable) | [C] `Meteor::Advance` |

```mermaid
sequenceDiagram
    participant P as Player (task)
    participant C as Crate::RunMeteorShower
    participant M as Meteor (x5-9)
    participant T as GameTimer
    participant L as Location::Bang
    P->>C: target point
    C->>M: spawn at random map XZ, y 1500-2000, target = point +/-210m
    loop every tick (0.1 s)
        M->>M: move 350*dt toward target, 5 fire particles
        M-->>T: first time within 200m: RequestSpeedChange(0.4, 0.1, 4.0)
    end
    M->>L: hit ground: Bang(pos, 50, 100, team) + sound Land
```

## [→ 02_units_and_entities] Ambient weather objects — Lightning, Snow, DustBall (level effects)

All three are `WorldObject`s stored in `Location::m_effects` with team −1, created by `Location` when the level file's `Effects` line enables them. They are **not crate outcomes**. Creation runs in `Location::Advance` on slice 3 (once per 0.1 s tick). **All three generators share one timer, `Location::m_christmasTimer`** (initial −99.9). [C] `code/location.cpp:Location::Advance` (L1388–1397), `Location::Location` (L120), `code/level_file.cpp:LevelFile::ParseMultiwiniaOptions` (`Effects` keyword, L421–431).

Shipped Multiwinia maps using effects [C] `data/levels/mp_*.txt` `Effects` lines:
| Effect keyword | Maps |
|---|---|
| `snow` | mp_assault_3p_3, mp_blitzkrieg_2P_4, mp_blitzkrieg_4P_2, mp_cts_2p_2, mp_koth_2P_3, mp_koth_2P_3b |
| `lightning` | mp_blitzkrieg_3P_2, mp_cts_4P_2 |
| `dustwind` | mp_cts_3p_1 |
| `SpecialLighting` (not this partition) | mp_koth_2P_3, mp_koth_2P_3b, mp_koth_3P_1 |
| `soulrain` (`AdvanceSoulRain`, not this partition) | none |
Snow is additionally forced on every map when **Christmas mode** is active (`Location::m_christmasMode`, copied from `ClientToServer::m_christmasMode`, which the server/metaserver can switch on; `App::UseChristmasMode`) — also brightens light colours ×(1.3,1.2,1.2). [C] `code/location.cpp:Location::Init` (L212–215), `Location::ChristmasModEnabled`, `Location::SetupLights` (L4750); `code/network/clienttoserver.cpp:ClientToServer::Advance` (L567), `ClientToServer::ProcessServerUpdates` (L3024). **[B:FULL]** all three.

### Lightning (`EffectLightning`) — storm strikes
| Item | Value | Ref |
|---|---|---|
| Generator | `Location::AdvanceLightning`: when shared timer ≤ 0 → timer = `syncfrand(15)` s; spawn one Lightning at random `(FRAND(sizeX), 1200, FRAND(sizeZ))`, re-rolled until terrain height > 1 (over land) | [C] `code/location.cpp:Location::AdvanceLightning` |
| Strike chance | each tick `syncfrand(1) < 0.01` (1%/tick ≈ 10%/s); may strike repeatedly | [C] `Lightning::Advance` |
| Lifetime | none until the first strike; each strike sets `m_life = 1.2` s; removed when it counts down to 0 | [C] |
| Strike effect | **sets on fire (`Darwinian::SetFire`) every Darwinian within 30 m** (any team); every **Tree** in the obstruction cell under the strike takes `Damage(-100)` (catches fire) | [C] `Lightning::Advance` |
| FX | 5 `TypeExplosionDebris` (150–250) + 10 `TypeFire` (160–280) particles at ground (unsynced sizes/velocities) | [C] |
| Render | idle: pulsing 200 m `textures/glow.bmp` cloud glow (alpha `|sin 2t|`); striking: 3 stacked bolt quads `textures/lightning.bmp` from the cloud to the ground, width 30 ×1.2 per layer, alpha halving | [C] `Lightning::Render` |
No sound event is triggered by Lightning code. [C] (no `m_soundSystem` call in `code/worldobject/lightning.cpp`).

### Snow (`EffectSnow`) — purely ambient
| Item | Value | Ref |
|---|---|---|
| Generator | `Location::AdvanceChristmas`: first call (timer < −90) creates **150** flakes at random XZ, y `syncfrand(1000)`; thereafter one flake every 0.1 s at y `700 + syncfrand(300)` | [C] `code/location.cpp:Location::AdvanceChristmas` |
| Motion | `vel *= 0.9`/tick; wobble `hover.xz = sin(offset) × rate`, rates random-walk by `syncsfrand(1)` per tick within [0,2]; fall `hover.y = −(20 + syncfrand(20)) × clamp(heightAboveGround/100, 0.2, 1)` m/s; clamped to the map | [C] `Snow::Advance` |
| Removal | when ≥ 10 m below the terrain surface | [C] |
| Gameplay effect | **none** (no entity/building interaction) | [C] `code/worldobject/snow.cpp` |
| Render | 20 m white `textures/starburst.bmp` 3-D sprite | [C] `Snow::Render` |
| `GetLife` | 0→1 over the first 10 s (fade-in helper; no caller in this partition) | [C] `Snow::GetLife` |
Snow consumes ~4 synced random numbers per flake per tick (deterministic but a cost); `snow.cpp` `#undef`s `TRACK_SYNC_RAND` so these calls are excluded from desync logging. [C] `code/worldobject/snow.cpp` (L36).

### DustBall (`EffectDustBall`) — dust storm and vortex
| Item | Value | Ref |
|---|---|---|
| `DUSTBALL_SPEED` | 500 m/s | [C] `code/worldobject/dustball.cpp` |
| Generator | `Location::AdvanceDustWind`: first call creates **300** balls (y `syncfrand(1000)`, size `20 + syncfrand(20)`); then **2** balls per 0.1 s at random XZ, y `syncfrand(1000)` | [C] `code/location.cpp:Location::AdvanceDustWind` |
| Wind | every `15 + syncfrand(15)` s (`DustBall::s_vortexTimer`): with probability 1/10 (`syncrand()%10 == 0`) and no active vortex → **vortex** at a random land point (height > 1), y `FRAND(500)`; otherwise vortex cleared and a new wind direction = normalise(random point − random point) | [C] |
| Ball motion (wind) | steer toward `pos + wind × 2000` with turn blend `3 × dt`, speed 500 | [C] `DustBall::Advance` |
| Ball motion (vortex) | orbit target `vortex + (cos τ, sin τ) × 300 × y/1000` at own height, τ = now + uniqueId×20 | [C] |
| Removal | leaves the map rectangle | [C] |
| **Vortex gameplay** | each tick every on-ground Darwinian within 50 m of the vortex ground point enters `Darwinian::StateInVortex` and becomes airborne; it then spirals (speed `max(100 × y/1000, 30)`, radius `300 × y/1000`, climbing +50 m/step up to `500 + 5 × (uniqueId % 100)` m) until the vortex clears, then returns to `StateIdle` with speed 50 (falls) | [C] `Location::AdvanceDustWind`; `code/worldobject/darwinian.cpp:Darwinian::AdvanceInVortex` |
| Render | `textures/cloudyglow.bmp` sprite RGBA(0.6,0.46,0.12,0.2), size `m_size` | [C] `DustBall::Render` |
| Reset | `s_vortexTimer = 0`, `s_vortexPos = 0` in `Location::Init` | [C] `code/location.cpp:Location::Init` |

