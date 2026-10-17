# P06_buildings_core — Core buildings: base class, spawn economy, turrets, transport

Scope: `code/worldobject/{building,spawnpoint,generator,powerstation,controltower,gunturret,radardish,portal,teleport,jumppad,carryablebuilding,constructionyard,spiritreceiver,spiritstore,incubator}.{h,cpp}` (30 files, all read in full). Build baseline: see `build_matrix.md` (`Release|Win32` + `TARGET_PC_FULLGAME`, `BLOCK_OLD_DARWINIA_OBJECTS` on, `LOCATION_EDITOR` off).

Supporting evidence read outside scope (listed in the coverage CSV): `code/level_file.cpp:LevelFile::ParseBuildings/ParseMultiwiniaOptions`, `code/location.cpp:Location::Init/InitialiseBuildings-loop/AdvanceBuildings/Advance/InitialiseTeam/Bang/IsFriend/FireTurretShell/FireRocket/ThrowWeapon/LaunchFireball/FireLaser`, `code/worldobject/darwinian.cpp:SearchForPorts/AdvanceApproachingPort/AdvanceOperatingPort`, `code/worldobject/entity.cpp:EnterTeleports/EntityIsBlocked`, `code/worldobject/weapons.cpp:TurretShell::Advance`, `code/worldobject/crate.cpp:RunGunTurret/RunTheBeast/RunSpawnMania`, `code/worldobject/controlstation.cpp:RecalculateOwnership`, `code/gametimer.cpp:GetServerAdvancePeriod`, `contrib/systemiv/lib/math/random_number.cpp`, `code/lib/vector3.{h,cpp}` (FRAND/SFRAND), `data/levels/*.txt`, `data/shapes/*.shp`, `data/multiwinia.txt`.

Helper scripts (run with `python3 -I`): `OUT/_work/scripts/P06_level_building_usage.py` → `drafts/P06_level_building_usage.csv` (building type × level counts); `OUT/_work/scripts/P06_spawn_graph.py` → `drafts/P06_spawn_graph.csv` (spawn-link graph analysis per level); `OUT/_work/scripts/P06_shape_markers.py` (port/light marker counts per shape).

### Conventions used throughout this draft

| Term | Meaning | Evidence |
|---|---|---|
| tick | One server advance. Every building's `Advance()` runs once per tick (buildings are advanced in `NUM_SLICES_PER_FRAME` slices, each building in exactly one slice) | [C] `code/location.cpp:Location::AdvanceBuildings` (`GetNextSliceBounds`); [I] once-per-tick follows from the slice partition (core-loop agent to confirm) |
| `SERVER_ADVANCE_PERIOD` (dt) | `0.1 * m_gameSpeed` seconds. Normal speed → **0.1 s**. All "per tick" constants below assume dt = 0.1 s; the speed-up/slow-down crates change `m_gameSpeed` | [C] `code/gametimer.h` macro; `code/gametimer.cpp:GameTimer::GetServerAdvancePeriod` |
| Team id | `unsigned char`; 0..5 = teams (`NUM_TEAMS` = 6), **255 = neutral/none**. `SetTeamId(-1)` therefore stores 255 | [C] `code/worldobject/worldobject.h:WorldObjectId::SetTeamId`; `code/globals.h:NUM_TEAMS` |
| `syncfrand(x)` | synced Mersenne-Twister, uniform in **[0, x)** | [C] `contrib/systemiv/lib/math/random_number.cpp:_syncfrand` |
| `syncsfrand(x)` | synced, uniform in **[−x/2, +x/2)** (NOT ±x) | [C] `random_number.cpp:_syncsfrand` = `(_syncfrand(1)-0.5)*x` |
| `FRAND(x)` / `SFRAND(x)` (upper case) | wrappers that call `_syncfrand` / `_syncsfrand` → **synced** | [C] `code/lib/vector3.cpp:FromRand` |
| `frand`/`sfrand`/`AppRandom`/`rand` (lower case) | **unsynced** LCG / CRT — only legal for cosmetic effects | [C] `random_number.cpp:frand/sfrand/AppRandom` |
| `Multiplayer()` | `m_gameMode == GameModeMultiwinia` — true for every Multiwinia game including vs-CPU and tutorials. `IsSinglePlayer()` = Campaign or Prologue (Darwinia) | [C] `code/app.cpp:App::Multiplayer/IsSinglePlayer` |
| Official MP levels | the 49 `mp_*.txt` files listed in `data/levels/level_enum.txt` (+10 PDLC names whose files are absent). Dev maps `mp_sandbox`, `mp_sandbox_all`, `mp_benchmark_1`, `mp_assault_stress` declare `GameTypes none`; `mp_tankbattle_*` use the compiled-out TankBattle mode | [C] `data/levels/level_enum.txt`; `GameTypes` lines; [I] GameTypes-none maps are not offered by `GameMenu::CreateMapList`/`AddMap` (not read in full) |

## [→ 03_buildings_and_world_objects] Building base class (`Building`, `BuildingPort`)

Source: `code/worldobject/building.h`, `code/worldobject/building.cpp`. Build tag: **[B:FULL]** (whole class), with debug-only render helpers under `DEBUG_RENDER_ENABLED` (globally on → also [B:FULL]).

### 1. Role and object model

`Building` derives from `WorldObject` (position `m_pos`, velocity `m_vel`, id `m_id`, type `m_type`) and is the base of every static or semi-static world object that the level file places in a `Buildings_StartDefinition` block, plus several runtime-created objects (crates, statues, turrets from crates, portals, AI targets). [C] `building.h:class Building`.

```mermaid
classDiagram
    WorldObject <|-- Building
    Building <|-- SpawnBuilding
    SpawnBuilding <|-- SpawnLink
    SpawnBuilding <|-- MasterSpawnPoint
    SpawnBuilding <|-- SpawnPoint
    Building <|-- SpawnPopulationLock
    Building <|-- GunTurret
    Building <|-- Teleport
    Teleport <|-- RadarDish
    Building <|-- JumpPad
    Building <|-- CarryableBuilding
    Building <|-- PowerBuilding
    PowerBuilding <|-- Generator
    PowerBuilding <|-- Pylon
    PowerBuilding <|-- PylonStart
    PowerBuilding <|-- PylonEnd
    PowerBuilding <|-- SolarPanel
    PowerBuilding <|-- PowerSplitter
    Building <|-- Powerstation
    Building <|-- ControlTower
    Building <|-- Incubator
    Building <|-- Portal
    Building <|-- ConstructionYard
    Building <|-- DisplayScreen
    Building <|-- ReceiverBuilding
    ReceiverBuilding <|-- SpiritProcessor
    ReceiverBuilding <|-- ReceiverLink
    ReceiverBuilding <|-- ReceiverSpiritSpawner
    ReceiverBuilding <|-- SpiritReceiver
    WorldObject <|-- GunTurretTarget
    WorldObject <|-- UnprocessedSpirit
    Building "1" *-- "0..n" BuildingPort : m_ports
```
(Only classes in this partition are shown; `Bridge` also derives from `Teleport` [C] `code/worldobject/entity.cpp:Entity::EnterTeleports` casts; `Statue`, `WallBuster`, `ChessPiece` derive from `CarryableBuilding` [C] `carryablebuilding.cpp:IsCarryableBuilding`.)

### 2. Data members (every member)

| Member | Type | Default (ctor) | Meaning | Ref |
|---|---|---|---|---|
| `m_front` | Vector3 | (1,0,0) | facing; level file stores x,z only, then normalised | [C] `Building::Building`, `Building::Read` |
| `m_up` | Vector3 | `g_upVector` (0,1,0) | up vector (SolarPanel/SpiritReceiver use terrain normal) | [C] |
| `m_timeOfDeath` | double | −1.0 | unused by this partition's classes [I] (no reads in scope) | [C] ctor |
| `m_dynamic` | bool | false | "only appears on this level, not all levels for this map" (Darwinia map/mission split); set from `Read(_in,_dynamic)` | [C] `building.h` comment, `Read` |
| `m_isGlobal` | bool | false (forced true for 6 types, §9) | building is mirrored in `GlobalWorld` (Darwinia campaign persistence) | [C] `CreateBuilding(int)` |
| `m_centrePos` | Vector3 | — | bounding-sphere centre: `Shape::CalculateCentre(mat)` or `m_pos` if no shape | [C] `Initialise`, `SetDetail` |
| `m_radius` | double | **13.0** | bounding-sphere radius: `Shape::CalculateRadius` or 13.0 if no shape | [C] ctor, `Initialise` |
| `m_destroyed` | bool | false | when true, next `Advance()` returns true → location deletes the building | [C] `Building::Advance`, `Location::AdvanceBuildings` |
| `m_shape` | Shape* | NULL | render/collision shape (set by subclass ctor via `SetShape`) | [C] |
| `m_lights` | LList<ShapeMarker*> | empty | every shape marker whose name **contains** `MarkerLight` (recursive) | [C] `SetShapeLights` (`strstr`) |
| `m_ports` | LList<BuildingPort*> | empty | every marker whose name **contains** `MarkerPort` | [C] `SetShapePorts` |
| `s_controlPad` (static) | Shape* | `controlpad.shp` | port pad shape, loaded by first Building ctor | [C] ctor |
| `s_controlPadStatus` (static) | ShapeMarker* | marker `MarkerStatus` of controlpad.shp | where the port status light is drawn | [C] ctor |
| `s_neighbours` (static, protected) | `std::vector<WorldObjectId>` | — | shared scratch buffer for entity-grid queries (not re-entrant) | [C] |
| team (in `m_id`) | uchar | **255 in Multiwinia**, 1 in single-player (Darwinia) | owner | [C] ctor `IsSinglePlayer()` branch |

`BuildingPort` members: `m_marker` (ShapeMarker*), `m_occupant` (WorldObjectId of the Darwinian operating it, invalid if empty), `m_mat` (world matrix of the port), `m_counter[NUM_TEAMS]` (per-team reservation counter). [C] `building.h:class BuildingPort`.

### 3. Lifecycle (template → instance)

1. **Parse.** `LevelFile::ParseBuildings` reads each line; first token = type name → `Building::CreateBuilding(char*)` (honours `BuildingIsBlocked`, returns NULL for blocked or unknown names, and the line is then silently skipped) → `building->Read(_in,_dynamic)`. Duplicate unique ids are a fatal `AppReleaseAssert`. Types TrunkPort, ControlTower, RadarDish, Incubator, FenceSwitch must have `isGlobal = 1` or loading asserts. Team-1-owned SpawnPopulationLocks get `m_maxPopulation *= 1 + difficulty/5` when the level difficulty is < 0 (Darwinia). [C] `code/level_file.cpp:LevelFile::ParseBuildings`.
2. **Instantiate.** For each parsed template, `Location` calls `CreateBuilding(type)` (int overload: **no** block check), maps the template's team **position** to a real team with `GetTeamFromPosition` (if team ≠ 255/−1), then `newBuilding->Initialise(template)` and `newBuilding->SetDetail(1)`. [C] `code/location.cpp` building-instantiation loop (≈L465–495).
3. **`Initialise(template)`** (base): copies `m_id, m_pos, m_front, m_up, m_dynamic, m_isGlobal, m_destroyed`; snaps `m_pos.y` to the heightmap; computes `m_centrePos/m_radius` from the shape; collects lights and ports; if a `GlobalBuilding` record exists for (uniqueId, requested location) its team overrides; triggers sound event `Create`. [C] `Building::Initialise`.
4. **`SetDetail(detail)`**: re-snaps y, recomputes centre/radius, **deletes and rebuilds all ports** (so port occupancy resets). `_detail` is ignored by the base class. [C] `Building::SetDetail`.
5. **`Advance()`** each tick: if `m_destroyed` return true (delete me); else `EvaluatePorts()`; return false. Subclasses call it last. [C] `Building::Advance`.
6. **Removal**: `Location::AdvanceBuildings` deletes the building when `Advance()` returns true, then recalculates the whole obstruction grid (and AI targets when an AITarget or Wall was removed). [C] `code/location.cpp:Location::AdvanceBuildings`.

Runtime-created buildings (crate turrets, statues, crates, portals, AI targets) follow the same pattern with a stack-allocated template: `CreateBuilding(int)` → `m_buildings.PutData` → `Initialise(&template)` → `m_id.SetUniqueId(GlobalWorld::GenerateBuildingId())`. [C] `code/worldobject/crate.cpp:Crate::RunGunTurret`, `gunturret.cpp:GunTurret::Advance` (AITarget).

### 4. Virtual / member function catalogue (every member)

| Function | Base behaviour | Overridden in this partition by | Ref |
|---|---|---|---|
| `Building()` ctor | defaults above; loads `controlpad.shp` once | all | [C] |
| `~Building()` | `m_ports.EmptyAndDelete()` | SpawnBuilding (links), PowerBuilding (surges), RadarDish (shape copy + AITarget), Incubator (empty) | [C] |
| `Initialise(Building*)` | §3 | SpawnBuilding, SpawnPoint, SpawnPopulationLock, ControlTower, PowerBuilding, PylonStart, SolarPanel, PowerSplitter, GunTurret, RadarDish, JumpPad, CarryableBuilding, Incubator, ReceiverBuilding, SpiritProcessor, SpiritReceiver | [C] |
| `Advance()` → bool | §3 step 5 | every class (JumpPad does **not** call the base → no port evaluation, `m_destroyed` ignored) | [C] `jumppad.cpp:JumpPad::Advance` |
| `SetShape(Shape*)` | stores pointer | Teleport (looks up `MarkerTeleportEntrance`), CarryableBuilding (pass-through) | [C] |
| `SetShapeLights(frag)` | recursive collect `*MarkerLight*` markers | — | [C] |
| `SetShapePorts(frag)` | recursive; for each `*MarkerPort*` marker: world matrix, position pushed out of the building by `PushFromBuilding(pos, 5.0)`, y snapped to terrain, counters zeroed | — | [C] |
| `SetDetail(int)` | §3 step 4 | GunTurret (re-forces centre = pos, radius = 30), RadarDish (caches entrance pos/front) | [C] |
| `IsInView()` | `camera->SphereInViewFrustum(m_centrePos, m_radius)` | SpawnBuilding, ControlTower, PowerBuilding, GunTurret, Teleport, CarryableBuilding (always true), ReceiverBuilding, SpiritProcessor (always true) | [C] |
| `Render(t)` | debug arrow when editing; `m_shape->Render(t, mat(front,up,pos))` | many (see per-class) | [C] |
| `RenderAlphas(t)` | `RenderLights(); RenderPorts();` | many | [C] |
| `RenderLights()` | if team ≠ 255 and lights exist: flash star sprites (size 6, 10 nested quads, team colour) only when `(lastValidSequenceIdFromServer % 10) / 2 == teamId` (or editing) — i.e. teams 0–4 each flash for 2 of every 10 server ticks; team 5 never flashes | — | [C] `Building::RenderLights` |
| `RenderPorts()` | per port: control-pad shape (if `RenderBuildingDetail` < 3) and status star: green (0.3,1,0.3) occupied / red (1,0.3,0.3) empty | SpawnPoint, GunTurret (grey/team colour), SolarPanel, SpiritReceiver | [C] |
| `RenderHitCheck()` | `DEBUG_RENDER_ENABLED`: shape hit-check or sphere | — | [C] |
| `RenderLink()` | `DEBUG_RENDER_ENABLED`: magenta arrow to `GetBuildingLink()` target | — | [C] |
| `PerformDepthSort(Vector3&)` | false | SpawnPoint (true, centre) | [C] |
| `SetTeamId(int)` | if changed: set team, mirror into `GlobalBuilding.m_teamId` (if any), sound `ChangeTeam` | — (subclasses call it) | [C] |
| `Reprogram(double)` | no-op | — | [C] |
| `ReprogramComplete()` | sound `ReprogramComplete`; toggles `GlobalBuilding.m_online`; `GlobalWorld::EvaluateEvents()` | Generator (enable) | [C] |
| `Damage(double)` | sound `Damage` only — **base buildings are indestructible** | GunTurret | [C] |
| `Destroy(double intensity)` | `m_destroyed = true`; 3× `g_explosionManager.AddExplosion(shape)`; `Location::Bang(m_pos, intensity, intensity/4)`; sound `Explode`; `(int)(intensity/4)` particles `TypeExplosionCore` size 100 with velocity `syncsfrand(100)` per axis (∈[−50,50)) | — | [C] |
| `PushFromBuilding(pos, r)` | if a sphere (pos,r) hits the building, step it by 2.0 units away from `m_pos` until clear | — | [C] |
| `EvaluatePorts()` | §5 | — | [C] |
| `GetNumPorts()` | `m_ports.Size()` | — | [C] |
| `GetNumPortsOccupied()` | count of ports with valid occupant | — | [C] |
| `GetPortOccupant(i)` | occupant id or invalid | — | [C] |
| `GetPortPosition(i,&pos,&front)` | port matrix pos/front; false if bad index | — | [C] |
| `OperatePort(i, team)` | `counter[team] = min(counter[team]+1, 50)` | — | [C] |
| `GetPortOperatorCount(i, team)` | `counter[team]`, or −1 for a bad index | — | [C] |
| `GetObjectiveCounter(UnicodeString&)` | empty string | MasterSpawnPoint, Generator, SpiritProcessor (Darwinia objectives) | [C] |
| `DoesSphereHit(pos,r)` | shape sphere test, else `|pos−m_pos| ≤ r + m_radius` | SpawnPopulationLock, JumpPad, Portal, CarryableBuilding (all false); RadarDish (radius ×1.5) | [C] |
| `DoesShapeHit(shape, mat)` | shape-vs-shape, else sphere | SpawnPopulationLock, JumpPad, Portal, CarryableBuilding (false) | [C] |
| `DoesRayHit(start,dir,len,pos,norm)` | shape ray test (`pos/norm` not filled), else ray-sphere | GunTurret, RadarDish, JumpPad, Portal, CarryableBuilding | [C] |
| `ListSoundEvents(list)` | `Create`, `Reprogramming` (comment "Remove me"), `ReprogramComplete`, `ChangeTeam`, `Damage` | most | [C] |
| `Read(TextReader*, bool dynamic)` / `Write(TextWriter*)` | §8 | per-class | [C] |
| `GetBuildingLink()` / `SetBuildingLink(id)` | −1 / no-op | SpawnBuilding (adds link), ControlTower, PowerBuilding, PowerSplitter (adds), Powerstation, RadarDish (adds valid link), ReceiverBuilding | [C] |
| `GetTypeName(int)` (static) | name table (§9); asserts and returns NULL out of range | — | [C] |
| `GetTypeId(name)` (static) | case-insensitive lookup, **ignores** the block list; −1 if not found | — | [C] |
| `CreateBuilding(int)` / `CreateBuilding(char*)` (static) | §9 | — | [C] |
| `GetTypeNameTranslated(type,&dest)` | language phrase `buildingname_<TypeName>` if present, else the raw type name | — | [C] |
| `LogState(msg)` | sync-log line hashing pos, vel, front, centre, radius (used by `SyncRandLogText` around every building advance) | — | [C] `Location::AdvanceBuildings` |
| `BuildingIsBlocked(int)` (static) | §10 | — | [C] |

### 5. Ports, occupancy and reservation

* **Creation**: one `BuildingPort` per shape marker containing `MarkerPort`; position pushed 5.0 units clear of the building, y on terrain. [C] `Building::SetShapePorts`.
* **Occupancy (every tick, `EvaluatePorts`)**: for each port, `m_occupant` is cleared, then the entity grid is queried within **5.0** units of the port; the first entity that is a `Darwinian` in `StateOperatingPort` becomes the occupant (any team; no check that the Darwinian is operating *this* port). [C] `Building::EvaluatePorts`.
* **Reservation counters**: if the port is occupied, all `m_counter[t]` = 0; otherwise each `m_counter[t] -= 4`, floored at 0. A Darwinian walking to a port calls `OperatePort(port, team)` every tick (`+1`, cap 50). [C] `Building::EvaluatePorts`, `code/worldobject/darwinian.cpp:Darwinian::AdvanceApproachingPort`.
* **Darwinian side (cross-partition)**: an idle (or worshipping/watching-spectacle/following-orders/following-officer/approaching-armour) Darwinian calls `SearchForPorts()`: candidate buildings have ≥1 port, `|building.pos − pos| − building.radius < 100` (`DARWINIAN_SEARCHRANGE_PORTS`), at least one free port, and a walkable path; it picks the **nearest** candidate (the random pick `syncrand() % n` is overwritten by the nearest loop but still consumes a synced random number), then picks uniformly (`syncrand()`) among ports with no occupant and `GetPortOperatorCount(p, team) < limit` where **limit = 1 in Multiwinia** (20 in Darwinia). Arrival → `StateOperatingPort`. The Darwinian leaves the port when `GetPortOccupant(port) != me` (e.g. displaced) or the building disappears. [C] `darwinian.cpp:Darwinian::SearchForPorts/AdvanceOperatingPort`; `darwinian.h:DARWINIAN_SEARCHRANGE_PORTS`.
* **Net effect in Multiwinia** [I]: a counter value ≥ 1 means "a team-mate is already walking to this port", so at most one Darwinian per team heads for each free port; counters decay by 4/tick once nobody is approaching. Different teams may race for the same port; whoever is first in `StateOperatingPort` within 5 units wins it.

### 6. Team ownership and capture — summary across the partition

Every capture rule in this partition is computed from **port occupants** (Darwinians standing on the building's pads) except where noted.

| Class | Ports (shape) | Recalc period | Rule | Contested result | Ref |
|---|---|---|---|---|---|
| SpawnPoint | 5 (`spawnpointmultiwinia.shp`) | 2.0 + syncfrand(1.0) s (first: syncfrand(2.0)) | owner = a team with **≥ 3** occupants (a later team index displaces the leader only if it has > 2 and strictly more); all other occupants must be friends of the owner | neutral (255) | [C] `SpawnPoint::RecalculateOwnership` |
| GunTurret | 4 (`battlecannonbase.shp`) | 1.0 s (`GUNTURRET_OWNERSHIPTIMER`) | owner = team with most occupants (≥1; ties → lowest team index); all occupants must be friends | neutral | [C] `GunTurret::RecalculateOwnership` |
| SolarPanel | 5 (`solarpanel.shp`) | every tick (Multiwinia only) | owner = team with most occupants (strictly greater wins; ties → lowest index); **enemies may co-occupy** | majority team | [C] `SolarPanel::RecalculateOwnership` |
| CarryableBuilding | 0 (proximity) | 1.0 s | team with most live Darwinians within 50 units, if ≥ `m_minLifters`; only switches if new team is not a friend of the current | 255 if nobody qualifies | [C] `CarryableBuilding::CalculateOwnership` |
| RadarDish | 0 | — | no own rule: team is set by a linked `ControlStation` (other partition) | — | [C] `code/worldobject/controlstation.cpp:ControlStation::RecalculateOwnership` (`b->SetTeamId(winningTeam)`) |
| ControlTower | 0 (3 reprogrammer slots) | per Engineer call | Engineer reprogramming moves `m_ownership` ±0.1 per call | — | [C] `ControlTower::Reprogram` |
| Incubator, JumpPad, Portal, SpawnLink, MasterSpawnPoint, SpawnPopulationLock, Pylon/PylonEnd/PowerSplitter | — | — | team fixed by level file / creator | — | [C] (no ownership code) |

`Building::SetTeamId` is the single mutation point (fires sound `ChangeTeam`, mirrors to GlobalBuilding). `RecalculateOwnership` variants call it every evaluation, but it is a no-op when unchanged. [C].

### 7. Damage, health, destruction

* Base `Damage()` only plays a sound: buildings without an override cannot be destroyed by weapons. [C] `Building::Damage`.
* `Location::Bang(pos, range, damage, team, fromBuilding)` calls `building->Damage(−damage × clamp(1 − dist/(3·range), 0, 1))` on **every** building within `3 × range` — this is the main source of building damage (grenades, rockets, air strikes, nukes, portal explosion). [C] `code/location.cpp:Location::Bang`.
* Other sources: `TurretShell` hits deal `Damage(−2)` but only to Tree/AntHill/Triffid buildings [C] `code/worldobject/weapons.cpp:TurretShell::Advance`; tasks call `Damage(−1000)` to self-destruct turrets [C] `code/taskmanager.cpp:Task::Advance/Task::Stop`; `WallBuster` deals −1000 [C] `code/worldobject/wallbuster.cpp`.
* In this partition only **GunTurret** has health (60). Sign convention: `Damage()` receives a **negative** number for harm (`m_health += _damage`). [C] `GunTurret::Damage`.
* `Destroy(intensity)` is the scripted-destruction path (§4). [C].

### 8. Level-file line (base fields, in order)

Header comment in every level: `# Type id x z tm rx rz isGlobal` [C] `data/levels/mp_koth_2P_1.txt`.

| # | Token | Parsed as | Meaning | Write format |
|---|---|---|---|---|
| 0 | type name | `CreateBuilding(char*)` (case-insensitive) | §9 names | `\t%-20s` |
| 1 | id | int | unique building id (must be unique per level) | `%-8d` |
| 2 | x | `iv_atof` | world x | `%-8.2f` |
| 3 | z | `iv_atof` | world z (y always from heightmap) | `%-8.2f` |
| 4 | team | int | team **position** in the level (0..5) or 255 neutral; remapped by `GetTeamFromPosition` at instantiation | `%-8d` |
| 5 | front.x | `iv_atof` | facing x | `%-8.2f` |
| 6 | front.z | `iv_atof` | facing z (then `m_front.Normalise()`, y = 0) | `%-8.2f` |
| 7 | isGlobal | `(bool)atoi` | §2 | `%-8d` |
| 8.. | subclass fields | per class `Read` | see each class | per class |

`m_id.Set(teamId, UNIT_BUILDINGS, −1, buildingId)`; `m_dynamic = _dynamic` (true for the second `Buildings_StartDefinition` block, the "mission" half). [C] `Building::Read/Write`; [I] which block is `_dynamic` is decided in `LevelFile` (other agent).

### 9. Type enum, name strings and factory

`CreateBuilding(int)` is a `switch` over the enum that `new`s the class, then for **RadarDish, ControlTower, TrunkPort, Incubator, DynamicHub, FenceSwitch** forces `m_isGlobal = true`, and sets `m_type`. `CreateBuilding(char*)` loops over all types, **skips blocked types**, and compares names case-insensitively; unknown/blocked → NULL. [C] `Building::CreateBuilding` (both overloads). The name table must stay in enum order (comment in source). `GetTypeId` does not check the block list. [C].

"Official MP / dev / Darwinia maps" = number of level files containing at least one line of that type (49 official MP levels; 9 dev `mp_*` files not in `level_enum.txt`; 30 Darwinia `map_*`/`mission_*` files). [C] `drafts/P06_level_building_usage.csv` via `scripts/P06_level_building_usage.py`.

| Value | Enum | Level-file name | Class (factory) | Header | `BuildingIsBlocked` | Forced global | Official MP / dev / Darwinia maps | Partition owner |
|---|---|---|---|---|---|---|---|---|
| 0 | `TypeInvalid` | `Invalid` | — (NULL) | | no | | 0/0/0 | — |
| 1 | `TypeFactory` | `Factory` | `Factory` | `factory.h` | **BLOCKED** | | 0/0/0 | other |
| 2 | `TypeCave` | `Cave` | `Cave` | `cave.h` | **BLOCKED** | | 0/0/0 | other |
| 3 | `TypeRadarDish` | `RadarDish` | `RadarDish` | `radardish.h` | no | yes | 7/0/11 | **P06** |
| 4 | `TypeLaserFence` | `LaserFence` | `LaserFence` | `laserfence.h` | no | | 14/1/1 | other |
| 5 | `TypeControlTower` | `ControlTower` | `ControlTower` | `controltower.h` | no | yes | 0/2/12 | **P06** |
| 6 | `TypeGunTurret` | `GunTurret` | `GunTurret` | `gunturret.h` | no | | 13/1/2 | **P06** |
| 7 | `TypeBridge` | `Bridge` | `Bridge` | `bridge.h` | **BLOCKED** | | 0/0/0 | other |
| 8 | `TypePowerstation` | `Powerstation` | `Powerstation` | `powerstation.h` | **BLOCKED** | | 0/0/0 | **P06** |
| 9 | `TypeTree` | `Tree` | `Tree` | `tree.h` | no | | 39/6/8 | other |
| 10 | `TypeWall` | `Wall` | `Wall` | `wall.h` | no | | 9/2/0 | other |
| 11 | `TypeTrunkPort` | `TrunkPort` | `TrunkPort` | `trunkport.h` | no | yes | 28/3/12 | other |
| 12 | `TypeResearchItem` | `ResearchItem` | `ResearchItem` | `researchitem.h` | **BLOCKED** | | 0/0/6 | other |
| 13 | `TypeLibrary` | `Library` | `Library` | `library.h` | **BLOCKED** | | 0/0/0 | other |
| 14 | `TypeGenerator` | `Generator` | `Generator` | `generator.h` | **BLOCKED** | | 0/0/1 | **P06** |
| 15 | `TypePylon` | `Pylon` | `Pylon` | `generator.h` | no | | 16/1/3 | **P06** |
| 16 | `TypePylonStart` | `PylonStart` | `PylonStart` | `generator.h` | **BLOCKED** | | 0/0/1 | **P06** |
| 17 | `TypePylonEnd` | `PylonEnd` | `PylonEnd` | `generator.h` | no | | 16/1/2 | **P06** |
| 18 | `TypeSolarPanel` | `SolarPanel` | `SolarPanel` | `generator.h` | no | | 16/1/2 | **P06** |
| 19 | `TypePowerSplitter` | `PowerSplitter` | `PowerSplitter` | `generator.h` | no | | 7/0/0 | **P06** |
| 20 | `TypeTrackLink` | `TrackLink` | `TrackLink` | `mine.h` | **BLOCKED** | | 0/0/2 | other |
| 21 | `TypeTrackJunction` | `TrackJunction` | `TrackJunction` | `mine.h` | **BLOCKED** | | 0/0/1 | other |
| 22 | `TypeTrackStart` | `TrackStart` | `TrackStart` | `mine.h` | **BLOCKED** | | 0/0/1 | other |
| 23 | `TypeTrackEnd` | `TrackEnd` | `TrackEnd` | `mine.h` | **BLOCKED** | | 0/0/2 | other |
| 24 | `TypeRefinery` | `Refinery` | `Refinery` | `mine.h` | **BLOCKED** | | 0/0/1 | other |
| 25 | `TypeMine` | `Mine` | `Mine` | `mine.h` | **BLOCKED** | | 0/0/1 | other |
| 26 | `TypeYard` | `Yard` | `ConstructionYard` | `constructionyard.h` | **BLOCKED** | | 0/0/1 | **P06** |
| 27 | `TypeDisplayScreen` | `DisplayScreen` | `DisplayScreen` | `constructionyard.h` | **BLOCKED** | | 0/0/0 | **P06** |
| 28 | `TypeUpgradePort` | `UpgradePort` | `UpgradePort` | `upgrade_port.h` | **BLOCKED** | | 0/0/0 | other |
| 29 | `TypePrimaryUpgradePort` | `PrimaryUpgrade` | `PrimaryUpgradePort` | `upgrade_port.h` | **BLOCKED** | | 0/0/0 | other |
| 30 | `TypeIncubator` | `Incubator` | `Incubator` | `incubator.h` | no | yes | 0/2/14 | **P06** |
| 31 | `TypeAntHill` | `AntHill` | `AntHill` | `anthill.h` | no | | 0/0/2 | other |
| 32 | `TypeSafeArea` | `SafeArea` | `SafeArea` | `safearea.h` | **BLOCKED** | | 0/0/3 | other |
| 33 | `TypeTriffid` | `Triffid` | `Triffid` | `triffid.h` | no | | 0/0/9 | other |
| 34 | `TypeSpiritReceiver` | `SpiritReceiver` | `SpiritReceiver` | `spiritreceiver.h` | **BLOCKED** | | 0/0/1 | **P06** |
| 35 | `TypeReceiverLink` | `ReceiverLink` | `ReceiverLink` | `spiritreceiver.h` | **BLOCKED** | | 0/0/2 | **P06** |
| 36 | `TypeReceiverSpiritSpawner` | `SpiritSpawner` | `ReceiverSpiritSpawner` | `spiritreceiver.h` | **BLOCKED** | | 0/0/1 | **P06** |
| 37 | `TypeSpiritProcessor` | `SpiritProcessor` | `SpiritProcessor` | `spiritreceiver.h` | **BLOCKED** | | 0/0/1 | **P06** |
| 38 | `TypeSpawnPoint` | `SpawnPoint` | `SpawnPoint` | `spawnpoint.h` | no | | 32/2/3 | **P06** |
| 39 | `TypeSpawnPopulationLock` | `SpawnPopulationLock` | `SpawnPopulationLock` | `spawnpoint.h` | no | | 21/1/6 | **P06** |
| 40 | `TypeSpawnPointMaster` | `SpawnPointMaster` | `MasterSpawnPoint` | `spawnpoint.h` | no | | 32/2/3 | **P06** |
| 41 | `TypeSpawnLink` | `SpawnLink` | `SpawnLink` | `spawnpoint.h` | no | | 21/2/3 | **P06** |
| 42 | `TypeAITarget` | `AITarget` | `AITarget` | `ai.h` | no | | 49/4/4 | other |
| 43 | `TypeAISpawnPoint` | `AISpawnPoint` | `AISpawnPoint` | `ai.h` | **BLOCKED** | | 0/0/1 | other |
| 44 | `TypeBlueprintStore` | `BlueprintStore` | `BlueprintStore` | `blueprintstore.h` | **BLOCKED** | | 0/0/1 | other |
| 45 | `TypeBlueprintConsole` | `BlueprintConsole` | `BlueprintConsole` | `blueprintstore.h` | **BLOCKED** | | 0/0/1 | other |
| 46 | `TypeBlueprintRelay` | `BlueprintRelay` | `BlueprintRelay` | `blueprintstore.h` | **BLOCKED** | | 0/0/1 | other |
| 47 | `TypeScriptTrigger` | `ScriptTrigger` | `ScriptTrigger` | `scripttrigger.h` | **BLOCKED** | | 0/0/11 | other |
| 48 | `TypeSpam` | `Spam` | `Spam` | `spam.h` | **BLOCKED** | | 0/0/1 | other |
| 49 | `TypeGodDish` | `GodDish` | `GodDish` | `goddish.h` | **BLOCKED** | | 0/0/1 | other |
| 50 | `TypeStaticShape` | `StaticShape` | `StaticShape` | `staticshape.h` | no | | 19/0/2 | other |
| 51 | `TypeFuelGenerator` | `FuelGenerator` | `FuelGenerator` | `rocket.h` | no | | 7/0/1 | other |
| 52 | `TypeFuelPipe` | `FuelPipe` | `FuelPipe` | `rocket.h` | no | | 7/0/1 | other |
| 53 | `TypeFuelStation` | `FuelStation` | `FuelStation` | `rocket.h` | no | | 7/0/1 | other |
| 54 | `TypeEscapeRocket` | `EscapeRocket` | `EscapeRocket` | `rocket.h` | no | | 7/0/1 | other |
| 55 | `TypeFenceSwitch` | `FenceSwitch` | `FenceSwitch` | `switch.h` | **BLOCKED** | yes | 0/0/0 | other |
| 56 | `TypeDynamicHub` | `DynamicHub` | `DynamicHub` | `generichub.h` | **BLOCKED** | yes | 0/0/0 | other |
| 57 | `TypeDynamicNode` | `DynamicNode` | `DynamicNode` | `generichub.h` | **BLOCKED** | | 0/0/0 | other |
| 58 | `TypeFeedingTube` | `FeedingTube` | `FeedingTube` | `feedingtube.h` | **BLOCKED** | | 0/0/0 | other |
| 59 | `TypeMultiwiniaZone` | `MultiwiniaZone` | `MultiwiniaZone` | `multiwiniazone.h` | no | | 25/2/1 | other |
| 60 | `TypeChessPiece` | `ChessPiece` | `ChessPiece` | `chess.h` | **BLOCKED** | | 0/0/0 | other |
| 61 | `TypeChessBase` | `ChessBase` | `ChessBase` | `chess.h` | **BLOCKED** | | 0/0/0 | other |
| 62 | `TypeCloneLab` | `CloneLab` | `CloneLab` | `clonelab.h` | no | | 0/0/0 | other |
| 63 | `TypeControlStation` | `ControlStation` | `ControlStation` | `controlstation.h` | no | | 9/1/0 | other |
| 64 | `TypePortal` | `Portal` | `Portal` | `portal.h` | **BLOCKED** | | 0/0/0 | **P06** |
| 65 | `TypeCrate` | `Crate` | `Crate` | `crate.h` | **BLOCKED** | | 0/0/0 | other |
| 66 | `TypeStatue` | `Statue` | `Statue` | `statue.h` | **BLOCKED** | | 0/1/0 | other |
| 67 | `TypeWallBuster` | `WallBuster` | `WallBuster` | `wallbuster.h` | no | | 1/0/0 | other |
| 68 | `TypePulseBomb` | `PulseBomb` | `PulseBomb` | `pulsebomb.h` | no | | 8/1/0 | other |
| 69 | `TypeRestrictionZone` | `RestrictionZone` | `RestrictionZone` | `restrictionzone.h` | no | | 14/0/0 | other |
| 70 | `TypeJumpPad` | `JumpPad` | `JumpPad` | `jumppad.h` | no | | 3/0/0 | **P06** |
| 71 | `TypeAIObjective` | `AIObjective` | `AIObjective` | `aiobjective.h` | no | | 8/1/0 | other |
| 72 | `TypeAIObjectiveMarker` | `AIObjectiveMarker` | `AIObjectiveMarker` | `aiobjective.h` | no | | 15/1/0 | other |
| 73 | `TypeEruptionMarker` | `EruptionMarker` | `EruptionMarker` | `eruptionmarker.h` | no | | 1/0/0 | other |
| 74 | `TypeSmokeMarker` | `SmokeMarker` | `SmokeMarker` | `smokemarker.h` | no | | 1/0/0 | other |
| 75 | `NumBuildingTypes` | — | — | | | | | — |

Notes:
* Name ≠ enum suffix for three types: `TypePrimaryUpgradePort` ↔ `"PrimaryUpgrade"`, `TypeReceiverSpiritSpawner` ↔ `"SpiritSpawner"`, `TypeYard` ↔ class `ConstructionYard`. [C] `GetTypeName` table.
* **Blocked ≠ impossible.** The block list only stops *level-file* creation by name and hides types in the editor (`interface/buildings_window.cpp`). Blocked types are still created at runtime through `CreateBuilding(int)`: `Crate` (`Location::AdvanceCrates`, cheat/help code), `Statue` (CTS mode, `code/multiwinia.cpp`), `Portal` ("The Beast" crate, `crate.cpp:Crate::RunTheBeast`), `ChessPiece` (`chess.cpp`), `Bridge` (Engineer), `Spam` (GodDish). [C] grep of `CreateBuilding(` callers.
* A Darwinia level line naming an unknown type (`CarryableBuilding` in `data/levels/map_sandbox.txt`) is silently dropped. [C].

### 10. `BuildingIsBlocked` — verbatim list as a table

Under `#ifdef BLOCK_OLD_DARWINIA_OBJECTS` (on in FULLGAME) the function returns true for exactly these 39 types (source order); otherwise false. [C] `Building::BuildingIsBlocked`.

| # | Blocked type | # | Blocked type | # | Blocked type |
|---|---|---|---|---|---|
| 1 | `TypeBlueprintConsole` | 14 | `TypeGenerator` | 27 | `TypeSpam` |
| 2 | `TypeBlueprintRelay` | 15 | `TypeGodDish` | 28 | `TypeSpiritProcessor` |
| 3 | `TypeBlueprintStore` | 16 | `TypeLibrary` | 29 | `TypeSpiritReceiver` |
| 4 | `TypeBridge` | 17 | `TypeMine` | 30 | `TypeStatue` |
| 5 | `TypeCave` | 18 | `TypePortal` | 31 | `TypeTrackEnd` |
| 6 | `TypeChessBase` | 19 | `TypePowerstation` | 32 | `TypeTrackJunction` |
| 7 | `TypeChessPiece` | 20 | `TypePrimaryUpgradePort` | 33 | `TypeTrackLink` |
| 8 | `TypeCrate` | 21 | `TypeReceiverLink` | 34 | `TypeTrackStart` |
| 9 | `TypeDisplayScreen` | 22 | `TypeReceiverSpiritSpawner` | 35 | `TypeUpgradePort` |
| 10 | `TypeDynamicHub` | 23 | `TypeRefinery` | 36 | `TypeYard` |
| 11 | `TypeDynamicNode` | 24 | `TypeResearchItem` | 37 | `TypePylonStart` |
| 12 | `TypeFactory` | 25 | `TypeSafeArea` | 38 | `TypeFeedingTube` |
| 13 | `TypeFenceSwitch` | 26 | `TypeScriptTrigger` | 39 | `TypeAISpawnPoint` |

Exact source order (single list, for unambiguous re-implementation): BlueprintConsole, BlueprintRelay, BlueprintStore, Bridge, Cave, ChessBase, ChessPiece, Crate, DisplayScreen, DynamicHub, DynamicNode, Factory, FenceSwitch, Generator, GodDish, Library, Mine, Portal, Powerstation, PrimaryUpgradePort, ReceiverLink, ReceiverSpiritSpawner, Refinery, ResearchItem, SafeArea, ScriptTrigger, Spam, SpiritProcessor, SpiritReceiver, Statue, TrackEnd, TrackJunction, TrackLink, TrackStart, UpgradePort, Yard, PylonStart, FeedingTube, AISpawnPoint. [C].

Not blocked although Darwinia-era: AntHill, Triffid, ControlTower, Incubator, TrunkPort, LaserFence, Pylon/PylonEnd/SolarPanel/PowerSplitter, FuelGenerator/FuelPipe/FuelStation/EscapeRocket, StaticShape, CloneLab. [C].

### 11. Global vs local buildings

`m_isGlobal` marks a building that is mirrored as a `GlobalBuilding` record (team, online flag, link) in `g_app->m_globalWorld` — the Darwinia campaign's cross-level persistence. Touch points in this partition: `Initialise` (team from GlobalBuilding), `SetTeamId` (writes team), `ReprogramComplete` (toggles online), ControlTower/Generator/SpiritProcessor/ConstructionYard/MasterSpawnPoint (set `m_online`), RadarDish (stores `m_link`). In Multiwinia `GlobalBuilding` lookups are harmless no-ops or campaign leftovers [I] (MP levels still set `isGlobal=1` on RadarDish/TrunkPort because `ParseBuildings` asserts it). [C] `building.cpp`, `level_file.cpp:ParseBuildings`.

### 12. Shapes, markers and hit tests used by this partition

| Shape | `MarkerPort*` | `MarkerLight*` | Other markers used | Ref |
|---|---|---|---|---|
| `spawnpointmultiwinia.shp` (MP) / `spawnpoint.shp` (SP) | 5 | 3 | MarkerDoor, MarkerSpiritLink | [C] `data/shapes/*.shp` via `scripts/P06_shape_markers.py` |
| `spawnlink.shp`, `masterspawnpoint.shp` | 0 | 0 | MarkerSpiritLink | [C] |
| `battlecannonbase.shp` (turret base) | 4 | 0 | MarkerStatus01–04 (code asks for 5; #5 NULL, harmless because only 4 ports) | [C] |
| `battlecannonturret.shp` | 0 | 0 | MarkerBarrel, MarkerKills | [C] |
| `cannon_bullets.shp`, `cannon_flame.shp`, `cannon_rocket.shp`, `battlecannonbarrel.shp` | 0 | 0 | MarkerBarrelEnd01–04; MarkerShells01–04 only on cannon_bullets | [C] |
| `radardish.shp` (per-instance copy) | 0 | 2 | MarkerTeleportEntrance, MarkerFocus; fragments `Dish`, `UpperMount` | [C] |
| `controltower.shp` | 0 | 1 (`MarkerLightPos` matches the `strstr` test) | MarkerReprogrammer0–2, MarkerConsole0–2, MarkerDishPos | [C] |
| `solarpanel.shp` | 5 | 0 | MarkerStatus01–05, MarkerGlow01–04, MarkerPowerLocation | [C] |
| `pylon.shp` | 0 | 1 | MarkerPowerLocation | [C] |
| `generator.shp` | 0 | 0 | MarkerPowerLocation, MarkerCounter | [C] |
| `powerstation.shp` | 4 | 0 | — | [C] |
| `constructionyard.shp` | 12 | 0 | MarkerPrimitive01–09 | [C] |
| `incubator.shp` | 0 | 4 | MarkerExit, MarkerSpirits, MarkerDock, MarkerSpiritIncoming0–2 | [C] |
| `spiritreceiver.shp` | 5 | 0 | MarkerHead, MarkerStatus01–05 | [C] |
| `controlpad.shp` | 0 | 0 | MarkerStatus | [C] |

Hit-test overrides: buildings that must never block movement or projectiles return false from `DoesSphereHit/DoesShapeHit` (SpawnPopulationLock, JumpPad, Portal, CarryableBuilding); RadarDish inflates its sphere test ×1.5 so its door registers in obstruction-grid cells; GunTurret ray test = turret shape ∪ barrel shape ∪ sphere r=12 at pos+(0,10,0) (exact picking for selection). [C] respective `DoesSphereHit/DoesRayHit`.

