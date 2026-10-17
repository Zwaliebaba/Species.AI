# 21 — Simulation and entity model

> Part B · Baseline build: Release|Win32 (OpenGL) + TARGET_PC_FULLGAME ([B:FULL]); see _work/build_matrix.md.
> Evidence: [C] confirmed · [I] inferred · [U] unknown. Citations are `path:Symbol` relative to the multiwinia/ source root.
> Sources merged: P03_entity_core (identity, WorldObject/Entity base, spatial partitioning, routing, class hierarchy of all 159 `code/worldobject` classes), P01_app_core (Location containers, level-load order, per-slice update order, Team/LobbyTeam/Unit, TeamControls), P14_platform_lib (container types, slot reuse, iteration order), X3_determinism_sync (simulated vs cosmetic classification, render-writes-sim scan, local-state leaks, id allocation), cross-partition notes in `_work/routed/_meta_*.md`, coordinator log `_work/phase2_log.md`; re-verified against the code by S21.

## Contents

- [1 Scope and the model at a glance](#1-scope-and-the-model-at-a-glance)
- [2 Class hierarchy (every class)](#2-class-hierarchy-every-class)
- [3 Object identity and handles](#3-object-identity-and-handles)
- [4 Base-class contracts](#4-base-class-contracts)
- [5 Location: containers, creation, removal and update ordering](#5-location-containers-creation-removal-and-update-ordering)
- [6 Spatial partitioning](#6-spatial-partitioning)
- [7 Pathfinding and routing](#7-pathfinding-and-routing)
- [8 Collision and physics approximations](#8-collision-and-physics-approximations)
- [9 Team and ownership model](#9-team-and-ownership-model)
- [10 Simulated state versus render state](#10-simulated-state-versus-render-state)
- [11 Guidance for the new engine's entity model](#11-guidance-for-the-new-engines-entity-model)
- [12 Constants](#12-constants)
- [Open questions and unknowns](#open-questions-and-unknowns)
- [Docs-vs-code discrepancies](#docs-vs-code-discrepancies)
- [IP items](#ip-items)
- [Source map](#source-map)
- [Appendix — dead/legacy code](#appendix--deadlegacy-code)

## 1 Scope and the model at a glance

This document specifies the engine-level object model of a match: what kinds of simulated objects exist, how they are identified, stored, created, advanced, removed and found in space, how they move, collide and find paths, who owns them, and how simulated state is kept apart from what is drawn. Per-type behaviour (state machines, stats, weapons) lives in [02](02_units_and_entities.md) (entities), [03](03_buildings_and_world_objects.md) (buildings) and [04](04_crates_powerups_and_weapons.md) (crates, weapons, effects); the main loop, tick clock and threads in [20](20_architecture.md); lockstep networking, RNG and float determinism in [22](22_determinism_and_networking.md); terrain queries in [23](23_terrain_and_landscape.md); render passes in [24](24_rendering.md).

Summary [C] unless tagged:

- One `Location` object (`g_app->m_location`) owns everything simulated in a match: the landscape, six `Team` slots, the building/effect/laser/spirit containers, an entity grid, its query cache, an obstruction grid and two routing systems [C] `code/location.h:Location`.
- Every simulated object derives from `WorldObject` (position, velocity, type, `WorldObjectId`). Three families: `Entity` (units, owned by a team), `Building` (level-placed or runtime-created static objects) and "effects" (`WorldObject` subclasses in `Location::m_effects`, plus `Laser` and `Spirit` stored by value) [C] `code/worldobject/worldobject.h`.
- Objects are addressed by a weak four-field handle `WorldObjectId(team, unit, index, uniqueId)` that is re-resolved on every use [C] `code/location.cpp:Location::GetWorldObject`.
- The simulation is lockstep: one server tick (one `NET_DARWINIA_UPDATE` letter, 0.1 s of real time, `0.1 × gameSpeed` of game time) is executed as 10 *slices*; every slice-partitioned container advances one tenth of its slots per slice, so every object advances exactly once per tick with `dt = SERVER_ADVANCE_PERIOD` [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters`, `code/lib/slice_darray.cpp:SliceDArray::GetNextSliceBounds` ([20 §10.3](20_architecture.md#103-client-tick-and-slices)).
- `Advance()` returning **true** means "remove and delete me" for every object family; the container owner performs the removal and its side effects (grid removal, obstruction-grid rebuild, AI-link rebuild) [C] `code/team.cpp:Team::Advance`, `code/location.cpp:Location::AdvanceBuildings`.
- Rendering never stores interpolated state: renderers receive a prediction time and extrapolate `pos + vel × t` [C] ([§10.2](#102-prediction-time-extrapolation-not-interpolation)).

```mermaid
flowchart TD
    L["NET_DARWINIA_UPDATE letter, tick N"] --> C["ProcessServerUpdates: apply every command of tick N"]
    C --> T["GameTimer Tick, GenerateSyncValue, SendSyncronisation"]
    T --> S["Slice loop s = 0..9, possibly spread over several render frames"]
    S --> A0["Location Advance s: teams 0..5, weapons, buildings, spirits, clouds at s=3"]
    A0 --> W["s=3 weather spawners, s=7 entity-grid cache eviction, s=8 crates and crate timers"]
    W --> P["ParticleSystem Advance s, cosmetic"]
    P --> M["s=8 and Multiplayer: Multiwinia Advance, also while paused"]
    M --> S
    S --> R["Render frames in between: extrapolate pos + vel x predictionTime"]
```

## 2 Class hierarchy (every class)

### 2.1 Inventory and method

- The class list is every `class X [: public Y]` declaration in the 85 headers of `code/worldobject/` (all compiled into the baseline) — **159 classes**: 142 derive from a base, 17 have none [C] `_work/drafts/P03_worldobject_classes.csv` (generated by `_work/scripts/P03_classdiag.py`), cross-checked by `_work/scripts/S_21_classdiagram.py verify`, which re-extracts the list with the coverage checker's regex (159 = 159, no difference).
- `Unit` (base of `AirstrikeUnit`, `InsertionSquad`, `ViriiUnit`) is declared in `code/unit.h`, not in `code/worldobject/`; it is drawn in the diagrams but not counted [C] `code/unit.h:Unit`.
- No `code/worldobject` class uses non-public or multiple inheritance, and no class outside `code/worldobject` derives from `WorldObject`, `Entity` or `Building` [C] grep of `: public WorldObject`, `: public Entity`, `: public Building` over `code/`.
- Counts by family: `Entity` and its 26 subclasses (27), `Building` and its 83 subclasses (84), 31 other `WorldObject`-derived or `Unit`-derived classes, `WorldObject` itself, and 16 helper classes with no `WorldObject` ancestry [C] script output.
- Diagrams 2.3–2.7 are generated by `_work/scripts/S_21_classdiagram.py gen` from `_work/class_assignment.csv`; classes tagged `<<legacy>>` are compiled but unreachable in Multiwinia ([B:LEGACY]), `<<debug>>` only reachable under `TARGET_DEBUG`. The table in §2.8 gives every class's base, header, build tag and the heading of its behaviour specification.

### 2.2 Framework types (members that the rest of this document relies on)

```mermaid
classDiagram
    class Location {
      +Landscape m_landscape
      +Team* m_teams_x6
      +SliceDArray_Building m_buildings
      +SliceDArray_Spirit m_spirits
      +SliceDArray_Laser m_lasers
      +SliceDArray_WorldObject m_effects
      +EntityGrid* m_entityGrid
      +EntityGridCache* m_entityGridCache
      +ObstructionGrid* m_obstructionGrid
      +LandRoutingSystem m_routingSystem
      +SeaRoutingSystem m_seaRoutingSystem
      +Advance(int slice)
      +GetWorldObject(WorldObjectId) WorldObject*
      +SpawnEntities() WorldObjectId
    }
    class Team {
      +int m_teamId
      +int m_teamType
      +FastDArray_Unit m_units
      +SliceDArray_Entity m_others
      +LList_WorldObjectId m_specials
      +TaskManager* m_taskManager
      +Advance(int slice)
      +NewEntity(int type, int unitId, int* index) Entity*
    }
    class Unit {
      +int m_teamId
      +int m_unitId
      +int m_troopType
      +SliceDArray_Entity m_entities
      +Vector3 m_centrePos
      +AdvanceEntities(int slice)
      +Advance(int unitIndex) bool
    }
    class WorldObjectId {
      -uchar m_teamId
      -int m_unitId
      -int m_index
      -int m_uniqueId
      -static int s_nextUniqueId
      +IsValid() bool
      +GenerateUniqueId()
      +ResetUniqueIdSeed()$
    }
    class WorldObject {
      +WorldObjectId m_id
      +int m_type
      +Vector3 m_pos
      +Vector3 m_vel
      +bool m_onGround
      +bool m_enabled
      +Advance() bool
      +Render(double t)
      +RenderPixelEffect(double t) bool
      +LogState(char* msg) char*
      +BounceOffLandscape()
    }
    class Entity {
      +uchar m_stats_x3
      +bool m_dead
      +bool m_destroyed
      +double m_reloading
      +double m_inWater
      +Vector3 m_front
      +Shape* m_shape
      +double m_radius
      +int m_routeId
      +Begin()
      +Advance(Unit* unit) bool
      +ChangeHealth(int amount, int type) bool
      +PushFromObstructions(Vector3 pos, bool killem) Vector3
      +FollowRoute()
      +NewEntity(int type)$ Entity*
    }
    class Building {
      +Vector3 m_front
      +Vector3 m_up
      +Vector3 m_centrePos
      +double m_radius
      +bool m_destroyed
      +Shape* m_shape
      +LList_BuildingPort m_ports
      +Initialise(Building* template)
      +Advance() bool
      +SetTeamId(int team)
      +DoesSphereHit(Vector3 pos, double r) bool
      +DoesShapeHit(Shape* s, Matrix34 m) bool
      +DoesRayHit() bool
      +CreateBuilding(int type)$ Building*
    }
    Location *-- "6" Team
    Location *-- "0..*" Building : m_buildings
    Location *-- "0..*" WorldObject : m_effects, m_lasers, m_spirits
    Team *-- "0..*" Unit : m_units
    Team *-- "0..*" Entity : m_others
    Unit *-- "0..*" Entity : m_entities
    WorldObject *-- WorldObjectId
    WorldObject <|-- Entity
    WorldObject <|-- Building
```

Member types are written with `_` in place of template brackets for diagram compatibility (`SliceDArray_Building` = `SliceDArray<Building*>`; `m_teams_x6` = `Team *m_teams[NUM_TEAMS]`). Full member tables: `WorldObjectId`/`WorldObject`/`Entity` in [02 §3](02_units_and_entities.md#3-framework-classes), `Building` in [03 §3.2](03_buildings_and_world_objects.md#32-data-members), `Location` in [§5.1](#51-location-members), `Team`/`Unit` in [§9](#9-team-and-ownership-model).

#### 2.3 Entity subclasses — 27 classes (Entity and every class derived from it)

```mermaid
classDiagram
    WorldObject <|-- Entity
    Entity <|-- AI
    Entity <|-- Armour
    Entity <|-- ArmyAnt
    Entity <|-- Centipede
    Entity <|-- Darwinian
    Entity <|-- DropShip
    Entity <|-- Egg
    Entity <|-- Engineer
    Entity <|-- Harvester
    Entity <|-- Lander
    Entity <|-- LaserTrooper
    Entity <|-- Meteor
    Entity <|-- Nuke
    Entity <|-- Officer
    Entity <|-- Shaman
    Entity <|-- SoulDestroyer
    Entity <|-- SpaceInvader
    Entity <|-- SpaceShip
    Entity <|-- Spider
    Entity <|-- SporeGenerator
    Entity <|-- Squadie
    Entity <|-- Tank
    Entity <|-- Tentacle
    Entity <|-- TriffidEgg
    Entity <|-- Tripod
    Entity <|-- Virii
    <<legacy>> DropShip
    <<legacy>> Lander
    <<legacy>> LaserTrooper
    <<legacy>> Shaman
    <<debug>> Tank
    <<debug>> Tentacle
    <<legacy>> Tripod
```

#### 2.4 Building intermediate bases and their subclasses — 45 classes

```mermaid
classDiagram
    WorldObject <|-- Building
    Building <|-- BlueprintBuilding
    Building <|-- CarryableBuilding
    Building <|-- DynamicBase
    Building <|-- FuelBuilding
    Building <|-- Incubator
    Building <|-- MineBuilding
    Building <|-- PowerBuilding
    Building <|-- ReceiverBuilding
    Building <|-- SpawnBuilding
    Building <|-- Teleport
    BlueprintBuilding <|-- BlueprintConsole
    BlueprintBuilding <|-- BlueprintRelay
    BlueprintBuilding <|-- BlueprintStore
    CarryableBuilding <|-- ChessPiece
    CarryableBuilding <|-- Statue
    CarryableBuilding <|-- WallBuster
    DynamicBase <|-- DynamicHub
    DynamicBase <|-- DynamicNode
    FuelBuilding <|-- EscapeRocket
    FuelBuilding <|-- FuelGenerator
    FuelBuilding <|-- FuelPipe
    FuelBuilding <|-- FuelStation
    Incubator <|-- CloneLab
    MineBuilding <|-- Mine
    MineBuilding <|-- Refinery
    MineBuilding <|-- TrackEnd
    MineBuilding <|-- TrackJunction
    MineBuilding <|-- TrackLink
    MineBuilding <|-- TrackStart
    PowerBuilding <|-- Generator
    PowerBuilding <|-- PowerSplitter
    PowerBuilding <|-- Pylon
    PowerBuilding <|-- PylonEnd
    PowerBuilding <|-- PylonStart
    PowerBuilding <|-- SolarPanel
    ReceiverBuilding <|-- ReceiverLink
    ReceiverBuilding <|-- ReceiverSpiritSpawner
    ReceiverBuilding <|-- SpiritProcessor
    ReceiverBuilding <|-- SpiritReceiver
    SpawnBuilding <|-- MasterSpawnPoint
    SpawnBuilding <|-- SpawnLink
    SpawnBuilding <|-- SpawnPoint
    Teleport <|-- Bridge
    Teleport <|-- RadarDish
    <<legacy>> BlueprintBuilding
    <<legacy>> BlueprintConsole
    <<legacy>> BlueprintRelay
    <<legacy>> BlueprintStore
    <<legacy>> Bridge
    <<legacy>> ChessPiece
    <<legacy>> DynamicBase
    <<legacy>> DynamicHub
    <<legacy>> DynamicNode
    <<legacy>> Generator
    <<legacy>> Mine
    <<legacy>> MineBuilding
    <<legacy>> PylonStart
    <<legacy>> ReceiverBuilding
    <<legacy>> ReceiverLink
    <<legacy>> ReceiverSpiritSpawner
    <<legacy>> Refinery
    <<legacy>> SpiritProcessor
    <<legacy>> SpiritReceiver
    <<legacy>> TrackEnd
    <<legacy>> TrackJunction
    <<legacy>> TrackLink
    <<legacy>> TrackStart
```

#### 2.5 Building direct leaf subclasses — 39 classes

```mermaid
classDiagram
    Building <|-- AIObjective
    Building <|-- AIObjectiveMarker
    Building <|-- AISpawnPoint
    Building <|-- AITarget
    Building <|-- AntHill
    Building <|-- Cave
    Building <|-- ChessBase
    Building <|-- ConstructionYard
    Building <|-- ControlStation
    Building <|-- ControlTower
    Building <|-- Crate
    Building <|-- DisplayScreen
    Building <|-- EruptionMarker
    Building <|-- Factory
    Building <|-- FeedingTube
    Building <|-- FenceSwitch
    Building <|-- GodDish
    Building <|-- GunTurret
    Building <|-- JumpPad
    Building <|-- LaserFence
    Building <|-- Library
    Building <|-- MultiwiniaZone
    Building <|-- Portal
    Building <|-- Powerstation
    Building <|-- PrimaryUpgradePort
    Building <|-- PulseBomb
    Building <|-- ResearchItem
    Building <|-- RestrictionZone
    Building <|-- SafeArea
    Building <|-- ScriptTrigger
    Building <|-- SmokeMarker
    Building <|-- Spam
    Building <|-- SpawnPopulationLock
    Building <|-- StaticShape
    Building <|-- Tree
    Building <|-- Triffid
    Building <|-- TrunkPort
    Building <|-- UpgradePort
    Building <|-- Wall
    <<legacy>> AISpawnPoint
    <<legacy>> Cave
    <<legacy>> ChessBase
    <<legacy>> ConstructionYard
    <<legacy>> DisplayScreen
    <<legacy>> Factory
    <<legacy>> FeedingTube
    <<legacy>> FenceSwitch
    <<legacy>> GodDish
    <<legacy>> Library
    <<debug>> Portal
    <<legacy>> Powerstation
    <<legacy>> PrimaryUpgradePort
    <<legacy>> ResearchItem
    <<legacy>> SafeArea
    <<legacy>> ScriptTrigger
    <<legacy>> Spam
    <<legacy>> UpgradePort
```

#### 2.6 Other WorldObject-derived classes (effects, weapons, spirits, order markers) and Unit subclasses — 32 classes

```mermaid
classDiagram
    Grenade <|-- FlameGrenade
    ThrowableWeapon <|-- AirStrikeMarker
    ThrowableWeapon <|-- ControllerGrenade
    ThrowableWeapon <|-- Grenade
    Unit <|-- AirstrikeUnit
    Unit <|-- InsertionSquad
    Unit <|-- ViriiUnit
    WorldObject <|-- BoxKite
    WorldObject <|-- DustBall
    WorldObject <|-- Fireball
    WorldObject <|-- GunTurretTarget
    WorldObject <|-- LandMine
    WorldObject <|-- Laser
    WorldObject <|-- Lightning
    WorldObject <|-- Missile
    WorldObject <|-- MultiwiniaOfficerOrders
    WorldObject <|-- MuzzleFlash
    WorldObject <|-- OfficerOrderTrail
    WorldObject <|-- OfficerOrders
    WorldObject <|-- PulseWave
    WorldObject <|-- Randomiser
    WorldObject <|-- Rocket
    WorldObject <|-- Shockwave
    WorldObject <|-- Snow
    WorldObject <|-- SpamInfection
    WorldObject <|-- Spirit
    WorldObject <|-- ThrowableWeapon
    WorldObject <|-- TurretEmptyShell
    WorldObject <|-- TurretShell
    WorldObject <|-- UnprocessedSpirit
    WorldObject <|-- Zombie
    <<legacy>> BoxKite
    <<legacy>> ControllerGrenade
    <<legacy>> LandMine
    <<legacy>> Missile
    <<legacy>> OfficerOrders
```

#### 2.7 Helper classes with no WorldObject ancestry — 16 classes, with their owners

```mermaid
classDiagram
    WorldObject *-- WorldObjectId : m_id
    Entity ..> EntityBlueprint : SetType stats
    Location o-- Light : m_lights
    Building *-- BuildingPort : m_ports
    SpawnBuilding *-- SpawnBuildingLink : m_links
    SpawnBuildingLink *-- SpawnBuildingSpirit : m_spirits
    Officer *-- OfficerFormation : m_formationEntities
    Officer *-- Flag : m_flag
    Armour *-- Flag : m_flag m_deployFlag
    MultiwiniaZone o-- Flag : m_blitzkriegFlag
    InsertionSquad *-- HistoricWayPoint : m_positionHistory
    Virii *-- ViriiHistory : m_positionHistory
    Spider *-- SpiderParameters : m_parameters
    Spider *-- EntityLeg : m_legs
    Tripod *-- EntityLeg : m_legs
    Tripod ..> TripodNavData : navigation
    EntityLeg *-- EntityFoot : m_foot
    Factory *-- SpiritStore : m_spiritStore
    MineBuilding *-- MineCart : m_carts
    <<legacy>> Factory
    <<legacy>> MineBuilding
    <<legacy>> MineCart
    <<legacy>> SpiritStore
    <<legacy>> Tripod
    <<legacy>> TripodNavData
```

#### 2.8 Every class: base, header, build tag, behaviour specification

Generated by `_work/scripts/S_21_classdiagram.py` from `_work/class_assignment.csv` (the class list is the one the coverage checker extracts from `code/worldobject/*.h`). "Spec" links to the heading in the final document that carries the class's behaviour specification. Build tags: `_work/build_matrix.md` §7.

| # | Class | Base | Header | Build tag | Spec |
|---|---|---|---|---|---|
| 1 | `AI` | `Entity` | `code/worldobject/ai.h` | [B:FULL] | [02 — 24 AI](02_units_and_entities.md#24-ai) |
| 2 | `AIObjective` | `Building` | `code/worldobject/aiobjective.h` | [B:FULL] | [03 — 10.2 AIObjective](03_buildings_and_world_objects.md#102-aiobjective) |
| 3 | `AIObjectiveMarker` | `Building` | `code/worldobject/aiobjective.h` | [B:FULL] | [03 — 10.3 AIObjectiveMarker](03_buildings_and_world_objects.md#103-aiobjectivemarker) |
| 4 | `AirStrikeMarker` | `ThrowableWeapon` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.5 AirStrikeMarker](04_crates_powerups_and_weapons.md#65-airstrikemarker) |
| 5 | `AirstrikeUnit` | `Unit` | `code/worldobject/airstrike.h` | [B:FULL] | [02 — 11.1 AirstrikeUnit and SpaceInvader — role and acquisition](02_units_and_entities.md#111-airstrikeunit-and-spaceinvader--role-and-acquisition) |
| 6 | `AISpawnPoint` | `Building` | `code/worldobject/ai.h` | [B:LEGACY] | [03 — 10.4 AISpawnPoint](03_buildings_and_world_objects.md#104-aispawnpoint) |
| 7 | `AITarget` | `Building` | `code/worldobject/ai.h` | [B:FULL] | [03 — 10.1 AITarget](03_buildings_and_world_objects.md#101-aitarget) |
| 8 | `AntHill` | `Building` | `code/worldobject/anthill.h` | [B:FULL] | [03 — 9.1 AntHill](03_buildings_and_world_objects.md#91-anthill) |
| 9 | `Armour` | `Entity` | `code/worldobject/armour.h` | [B:FULL] | [02 — 8 Armour](02_units_and_entities.md#8-armour) |
| 10 | `ArmyAnt` | `Entity` | `code/worldobject/armyant.h` | [B:FULL] | [02 — 13 ArmyAnt](02_units_and_entities.md#13-armyant) |
| 11 | `BlueprintBuilding` | `Building` | `code/worldobject/blueprintstore.h` | [B:LEGACY] | [03 — 12.11 BlueprintBuilding](03_buildings_and_world_objects.md#1211-blueprintbuilding) |
| 12 | `BlueprintConsole` | `BlueprintBuilding` | `code/worldobject/blueprintstore.h` | [B:LEGACY] | [03 — 12.13 BlueprintConsole](03_buildings_and_world_objects.md#1213-blueprintconsole) |
| 13 | `BlueprintRelay` | `BlueprintBuilding` | `code/worldobject/blueprintstore.h` | [B:LEGACY] | [03 — 12.14 BlueprintRelay](03_buildings_and_world_objects.md#1214-blueprintrelay) |
| 14 | `BlueprintStore` | `BlueprintBuilding` | `code/worldobject/blueprintstore.h` | [B:LEGACY] | [03 — 12.12 BlueprintStore](03_buildings_and_world_objects.md#1212-blueprintstore) |
| 15 | `BoxKite` | `WorldObject` | `code/worldobject/darwinian.h` | [B:LEGACY] | [04 — 6.22 BoxKite](04_crates_powerups_and_weapons.md#622-boxkite) |
| 16 | `Bridge` | `Teleport` | `code/worldobject/bridge.h` | [B:LEGACY] | [03 — 12.15 Bridge](03_buildings_and_world_objects.md#1215-bridge) |
| 17 | `Building` | `WorldObject` | `code/worldobject/building.h` | [B:FULL] | [03 — 3.1 Building](03_buildings_and_world_objects.md#31-building) |
| 18 | `BuildingPort` | — | `code/worldobject/building.h` | [B:FULL] | [03 — 3.3 BuildingPort](03_buildings_and_world_objects.md#33-buildingport) |
| 19 | `CarryableBuilding` | `Building` | `code/worldobject/carryablebuilding.h` | [B:FULL] | [03 — 8.3 CarryableBuilding](03_buildings_and_world_objects.md#83-carryablebuilding) |
| 20 | `Cave` | `Building` | `code/worldobject/cave.h` | [B:LEGACY] | [03 — 12.16 Cave](03_buildings_and_world_objects.md#1216-cave) |
| 21 | `Centipede` | `Entity` | `code/worldobject/centipede.h` | [B:FULL] | [02 — 18 Centipede](02_units_and_entities.md#18-centipede) |
| 22 | `ChessBase` | `Building` | `code/worldobject/chess.h` | [B:LEGACY] | [03 — 12.17 ChessBase](03_buildings_and_world_objects.md#1217-chessbase) |
| 23 | `ChessPiece` | `CarryableBuilding` | `code/worldobject/chess.h` | [B:LEGACY] | [03 — 12.18 ChessPiece](03_buildings_and_world_objects.md#1218-chesspiece) |
| 24 | `CloneLab` | `Incubator` | `code/worldobject/clonelab.h` | [B:FULL] (loadable; no shipped map) | [03 — 11.3 CloneLab](03_buildings_and_world_objects.md#113-clonelab) |
| 25 | `ConstructionYard` | `Building` | `code/worldobject/constructionyard.h` | [B:LEGACY] | [03 — 12.1 ConstructionYard](03_buildings_and_world_objects.md#121-constructionyard) |
| 26 | `ControllerGrenade` | `ThrowableWeapon` | `code/worldobject/weapons.h` | [B:LEGACY] | [04 — 6.17 ControllerGrenade](04_crates_powerups_and_weapons.md#617-controllergrenade) |
| 27 | `ControlStation` | `Building` | `code/worldobject/controlstation.h` | [B:FULL] | [03 — 5.4 ControlStation](03_buildings_and_world_objects.md#54-controlstation) |
| 28 | `ControlTower` | `Building` | `code/worldobject/controltower.h` | [B:FULL] (loadable; no shipped map) | [03 — 11.1 ControlTower](03_buildings_and_world_objects.md#111-controltower) |
| 29 | `Crate` | `Building` | `code/worldobject/crate.h` | [B:FULL] | [04 — 1 Crate lifecycle](04_crates_powerups_and_weapons.md#1-crate-lifecycle) |
| 30 | `Darwinian` | `Entity` | `code/worldobject/darwinian.h` | [B:FULL] | [02 — 4 Darwinian](02_units_and_entities.md#4-darwinian) |
| 31 | `DisplayScreen` | `Building` | `code/worldobject/constructionyard.h` | [B:LEGACY] | [03 — 12.2 DisplayScreen](03_buildings_and_world_objects.md#122-displayscreen) |
| 32 | `DropShip` | `Entity` | `code/worldobject/dropship.h` | [B:LEGACY] | [02 — 28 DropShip](02_units_and_entities.md#28-dropship) |
| 33 | `DustBall` | `WorldObject` | `code/worldobject/dustball.h` | [B:FULL] | [04 — 6.20 DustBall](04_crates_powerups_and_weapons.md#620-dustball) |
| 34 | `DynamicBase` | `Building` | `code/worldobject/generichub.h` | [B:LEGACY] | [03 — 12.20 DynamicBase](03_buildings_and_world_objects.md#1220-dynamicbase) |
| 35 | `DynamicHub` | `DynamicBase` | `code/worldobject/generichub.h` | [B:LEGACY] | [03 — 12.21 DynamicHub](03_buildings_and_world_objects.md#1221-dynamichub) |
| 36 | `DynamicNode` | `DynamicBase` | `code/worldobject/generichub.h` | [B:LEGACY] | [03 — 12.22 DynamicNode](03_buildings_and_world_objects.md#1222-dynamicnode) |
| 37 | `Egg` | `Entity` | `code/worldobject/egg.h` | [B:FULL] | [02 — 15 Egg](02_units_and_entities.md#15-egg) |
| 38 | `Engineer` | `Entity` | `code/worldobject/engineer.h` | [B:FULL] | [02 — 10 Engineer](02_units_and_entities.md#10-engineer) |
| 39 | `Entity` | `WorldObject` | `code/worldobject/entity.h` | [B:FULL] | [02 — 3.4 Entity — base class of all units](02_units_and_entities.md#34-entity--base-class-of-all-units) |
| 40 | `EntityBlueprint` | — | `code/worldobject/entity.h` | [B:FULL] | [02 — 3.5 EntityBlueprint — the stats.txt table](02_units_and_entities.md#35-entityblueprint--the-statstxt-table) |
| 41 | `EntityFoot` | — | `code/worldobject/entity_leg.h` | [B:FULL] | [02 — 20.1 EntityFoot — data](02_units_and_entities.md#201-entityfoot--data) |
| 42 | `EntityLeg` | — | `code/worldobject/entity_leg.h` | [B:FULL] | [02 — 20 EntityLeg](02_units_and_entities.md#20-entityleg) |
| 43 | `EruptionMarker` | `Building` | `code/worldobject/eruptionmarker.h` | [B:FULL] | [03 — 7.7 EruptionMarker](03_buildings_and_world_objects.md#77-eruptionmarker) |
| 44 | `EscapeRocket` | `FuelBuilding` | `code/worldobject/rocket.h` | [B:FULL] | [03 — 8.12 EscapeRocket](03_buildings_and_world_objects.md#812-escaperocket) |
| 45 | `Factory` | `Building` | `code/worldobject/factory.h` | [B:LEGACY] | [03 — 12.10 Factory](03_buildings_and_world_objects.md#1210-factory) |
| 46 | `FeedingTube` | `Building` | `code/worldobject/feedingtube.h` | [B:LEGACY] | [03 — 12.19 FeedingTube](03_buildings_and_world_objects.md#1219-feedingtube) |
| 47 | `FenceSwitch` | `Building` | `code/worldobject/switch.h` | [B:LEGACY] | [03 — 12.37 FenceSwitch](03_buildings_and_world_objects.md#1237-fenceswitch) |
| 48 | `Fireball` | `WorldObject` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.10 Fireball](04_crates_powerups_and_weapons.md#610-fireball) |
| 49 | `Flag` | — | `code/worldobject/flag.h` | [B:FULL] | [03 — 8.2 Flag](03_buildings_and_world_objects.md#82-flag) |
| 50 | `FlameGrenade` | `Grenade` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.4 FlameGrenade](04_crates_powerups_and_weapons.md#64-flamegrenade) |
| 51 | `FuelBuilding` | `Building` | `code/worldobject/rocket.h` | [B:FULL] | [03 — 8.8 FuelBuilding](03_buildings_and_world_objects.md#88-fuelbuilding) |
| 52 | `FuelGenerator` | `FuelBuilding` | `code/worldobject/rocket.h` | [B:FULL] | [03 — 8.9 FuelGenerator](03_buildings_and_world_objects.md#89-fuelgenerator) |
| 53 | `FuelPipe` | `FuelBuilding` | `code/worldobject/rocket.h` | [B:FULL] | [03 — 8.10 FuelPipe](03_buildings_and_world_objects.md#810-fuelpipe) |
| 54 | `FuelStation` | `FuelBuilding` | `code/worldobject/rocket.h` | [B:FULL] | [03 — 8.11 FuelStation](03_buildings_and_world_objects.md#811-fuelstation) |
| 55 | `Generator` | `PowerBuilding` | `code/worldobject/generator.h` | [B:LEGACY] | [03 — 6.6 Generator](03_buildings_and_world_objects.md#66-generator) |
| 56 | `GodDish` | `Building` | `code/worldobject/goddish.h` | [B:LEGACY] | [03 — 12.23 GodDish](03_buildings_and_world_objects.md#1223-goddish) |
| 57 | `Grenade` | `ThrowableWeapon` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.3 Grenade](04_crates_powerups_and_weapons.md#63-grenade) |
| 58 | `GunTurret` | `Building` | `code/worldobject/gunturret.h` | [B:FULL] | [03 — 5.1 GunTurret](03_buildings_and_world_objects.md#51-gunturret) |
| 59 | `GunTurretTarget` | `WorldObject` | `code/worldobject/gunturret.h` | [B:FULL] | [04 — 6.21 GunTurretTarget](04_crates_powerups_and_weapons.md#621-gunturrettarget) |
| 60 | `Harvester` | `Entity` | `code/worldobject/harvester.h` | [B:FULL] | [02 — 9 Harvester](02_units_and_entities.md#9-harvester) |
| 61 | `HistoricWayPoint` | — | `code/worldobject/insertion_squad.h` | [B:FULL] | [02 — 7.7 HistoricWayPoint — squad breadcrumb](02_units_and_entities.md#77-historicwaypoint--squad-breadcrumb) |
| 62 | `Incubator` | `Building` | `code/worldobject/incubator.h` | [B:FULL] (loadable; no shipped map) | [03 — 11.2 Incubator](03_buildings_and_world_objects.md#112-incubator) |
| 63 | `InsertionSquad` | `Unit` | `code/worldobject/insertion_squad.h` | [B:FULL] | [02 — 7.1 InsertionSquad and Squadie — role and acquisition](02_units_and_entities.md#71-insertionsquad-and-squadie--role-and-acquisition) |
| 64 | `JumpPad` | `Building` | `code/worldobject/jumppad.h` | [B:FULL] | [03 — 5.5 JumpPad](03_buildings_and_world_objects.md#55-jumppad) |
| 65 | `Lander` | `Entity` | `code/worldobject/lander.h` | [B:LEGACY] | [02 — 31 Lander](02_units_and_entities.md#31-lander) |
| 66 | `LandMine` | `WorldObject` | `code/worldobject/weapons.h` | [B:LEGACY] (compiled, no caller) | [04 — 6.14 LandMine](04_crates_powerups_and_weapons.md#614-landmine) |
| 67 | `Laser` | `WorldObject` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.7 Laser](04_crates_powerups_and_weapons.md#67-laser) |
| 68 | `LaserFence` | `Building` | `code/worldobject/laserfence.h` | [B:FULL] | [03 — 7.1 LaserFence](03_buildings_and_world_objects.md#71-laserfence) |
| 69 | `LaserTrooper` | `Entity` | `code/worldobject/lasertrooper.h` | [B:LEGACY] | [02 — 26 LaserTrooper](02_units_and_entities.md#26-lasertrooper) |
| 70 | `Library` | `Building` | `code/worldobject/library.h` | [B:LEGACY] | [03 — 12.25 Library](03_buildings_and_world_objects.md#1225-library) |
| 71 | `Light` | — | `code/worldobject/worldobject.h` | [B:FULL] | [02 — 3.3 Light — level light record](02_units_and_entities.md#33-light--level-light-record) |
| 72 | `Lightning` | `WorldObject` | `code/worldobject/lightning.h` | [B:FULL] | [04 — 6.18 Lightning](04_crates_powerups_and_weapons.md#618-lightning) |
| 73 | `MasterSpawnPoint` | `SpawnBuilding` | `code/worldobject/spawnpoint.h` | [B:FULL] | [03 — 4.4 MasterSpawnPoint](03_buildings_and_world_objects.md#44-masterspawnpoint) |
| 74 | `Meteor` | `Entity` | `code/worldobject/meteor.h` | [B:FULL] | [02 — 22 Meteor](02_units_and_entities.md#22-meteor) |
| 75 | `Mine` | `MineBuilding` | `code/worldobject/mine.h` | [B:LEGACY] | [03 — 12.34 Mine](03_buildings_and_world_objects.md#1234-mine) |
| 76 | `MineBuilding` | `Building` | `code/worldobject/mine.h` | [B:LEGACY] | [03 — 12.27 MineBuilding](03_buildings_and_world_objects.md#1227-minebuilding) |
| 77 | `MineCart` | — | `code/worldobject/mine.h` | [B:LEGACY] | [03 — 12.28 MineCart](03_buildings_and_world_objects.md#1228-minecart) |
| 78 | `Missile` | `WorldObject` | `code/worldobject/weapons.h` | [B:LEGACY] (compiled, no caller) | [04 — 6.13 Missile](04_crates_powerups_and_weapons.md#613-missile) |
| 79 | `MultiwiniaOfficerOrders` | `WorldObject` | `code/worldobject/officer.h` | [B:FULL] | [02 — 5.8 OfficerOrders, MultiwiniaOfficerOrders, OfficerOrderTrail — order effects](02_units_and_entities.md#58-officerorders-multiwiniaofficerorders-officerordertrail--order-effects) |
| 80 | `MultiwiniaZone` | `Building` | `code/worldobject/multiwiniazone.h` | [B:FULL] | [03 — 8.1 MultiwiniaZone](03_buildings_and_world_objects.md#81-multiwiniazone) |
| 81 | `MuzzleFlash` | `WorldObject` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.12 MuzzleFlash](04_crates_powerups_and_weapons.md#612-muzzleflash) |
| 82 | `Nuke` | `Entity` | `code/worldobject/nuke.h` | [B:FULL] | [02 — 12 Nuke](02_units_and_entities.md#12-nuke) |
| 83 | `Officer` | `Entity` | `code/worldobject/officer.h` | [B:FULL] | [02 — 5 Officer](02_units_and_entities.md#5-officer) |
| 84 | `OfficerFormation` | — | `code/worldobject/officer.h` | [B:FULL] | [02 — 5.6 OfficerFormation — formation slot](02_units_and_entities.md#56-officerformation--formation-slot) |
| 85 | `OfficerOrders` | `WorldObject` | `code/worldobject/officer.h` | [B:LEGACY] | [02 — 5.8 OfficerOrders, MultiwiniaOfficerOrders, OfficerOrderTrail — order effects](02_units_and_entities.md#58-officerorders-multiwiniaofficerorders-officerordertrail--order-effects) |
| 86 | `OfficerOrderTrail` | `WorldObject` | `code/worldobject/officer.h` | [B:FULL] | [02 — 5.8 OfficerOrders, MultiwiniaOfficerOrders, OfficerOrderTrail — order effects](02_units_and_entities.md#58-officerorders-multiwiniaofficerorders-officerordertrail--order-effects) |
| 87 | `Portal` | `Building` | `code/worldobject/portal.h` | [B:DEBUG] | [03 — 9.3 Portal](03_buildings_and_world_objects.md#93-portal) |
| 88 | `PowerBuilding` | `Building` | `code/worldobject/generator.h` | [B:FULL] | [03 — 6.1 PowerBuilding](03_buildings_and_world_objects.md#61-powerbuilding) |
| 89 | `PowerSplitter` | `PowerBuilding` | `code/worldobject/generator.h` | [B:FULL] | [03 — 6.5 PowerSplitter](03_buildings_and_world_objects.md#65-powersplitter) |
| 90 | `Powerstation` | `Building` | `code/worldobject/powerstation.h` | [B:LEGACY] | [03 — 6.8 Powerstation](03_buildings_and_world_objects.md#68-powerstation) |
| 91 | `PrimaryUpgradePort` | `Building` | `code/worldobject/upgrade_port.h` | [B:LEGACY] | [03 — 12.39 PrimaryUpgradePort](03_buildings_and_world_objects.md#1239-primaryupgradeport) |
| 92 | `PulseBomb` | `Building` | `code/worldobject/pulsebomb.h` | [B:FULL] | [03 — 8.7 PulseBomb](03_buildings_and_world_objects.md#87-pulsebomb) |
| 93 | `PulseWave` | `WorldObject` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.15 PulseWave](04_crates_powerups_and_weapons.md#615-pulsewave) |
| 94 | `Pylon` | `PowerBuilding` | `code/worldobject/generator.h` | [B:FULL] | [03 — 6.3 Pylon](03_buildings_and_world_objects.md#63-pylon) |
| 95 | `PylonEnd` | `PowerBuilding` | `code/worldobject/generator.h` | [B:FULL] | [03 — 6.4 PylonEnd](03_buildings_and_world_objects.md#64-pylonend) |
| 96 | `PylonStart` | `PowerBuilding` | `code/worldobject/generator.h` | [B:LEGACY] | [03 — 6.7 PylonStart](03_buildings_and_world_objects.md#67-pylonstart) |
| 97 | `RadarDish` | `Teleport` | `code/worldobject/radardish.h` | [B:FULL] | [03 — 5.3 RadarDish](03_buildings_and_world_objects.md#53-radardish) |
| 98 | `Randomiser` | `WorldObject` | `code/worldobject/randomiser.h` | [B:FULL] | [03 — 9.4 Randomiser](03_buildings_and_world_objects.md#94-randomiser) |
| 99 | `ReceiverBuilding` | `Building` | `code/worldobject/spiritreceiver.h` | [B:LEGACY] | [03 — 12.3 ReceiverBuilding](03_buildings_and_world_objects.md#123-receiverbuilding) |
| 100 | `ReceiverLink` | `ReceiverBuilding` | `code/worldobject/spiritreceiver.h` | [B:LEGACY] | [03 — 12.5 ReceiverLink](03_buildings_and_world_objects.md#125-receiverlink) |
| 101 | `ReceiverSpiritSpawner` | `ReceiverBuilding` | `code/worldobject/spiritreceiver.h` | [B:LEGACY] | [03 — 12.6 ReceiverSpiritSpawner](03_buildings_and_world_objects.md#126-receiverspiritspawner) |
| 102 | `Refinery` | `MineBuilding` | `code/worldobject/mine.h` | [B:LEGACY] | [03 — 12.33 Refinery](03_buildings_and_world_objects.md#1233-refinery) |
| 103 | `ResearchItem` | `Building` | `code/worldobject/researchitem.h` | [B:LEGACY] | [03 — 12.26 ResearchItem](03_buildings_and_world_objects.md#1226-researchitem) |
| 104 | `RestrictionZone` | `Building` | `code/worldobject/restrictionzone.h` | [B:FULL] | [03 — 7.5 RestrictionZone](03_buildings_and_world_objects.md#75-restrictionzone) |
| 105 | `Rocket` | `WorldObject` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.6 Rocket](04_crates_powerups_and_weapons.md#66-rocket) |
| 106 | `SafeArea` | `Building` | `code/worldobject/safearea.h` | [B:LEGACY] | [03 — 12.35 SafeArea](03_buildings_and_world_objects.md#1235-safearea) |
| 107 | `ScriptTrigger` | `Building` | `code/worldobject/scripttrigger.h` | [B:LEGACY] | [03 — 12.36 ScriptTrigger](03_buildings_and_world_objects.md#1236-scripttrigger) |
| 108 | `Shaman` | `Entity` | `code/worldobject/shaman.h` | [B:LEGACY] | [02 — 27 Shaman](02_units_and_entities.md#27-shaman) |
| 109 | `Shockwave` | `WorldObject` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.11 Shockwave](04_crates_powerups_and_weapons.md#611-shockwave) |
| 110 | `SmokeMarker` | `Building` | `code/worldobject/smokemarker.h` | [B:FULL] | [03 — 7.6 SmokeMarker](03_buildings_and_world_objects.md#76-smokemarker) |
| 111 | `Snow` | `WorldObject` | `code/worldobject/snow.h` | [B:FULL] | [04 — 6.19 Snow](04_crates_powerups_and_weapons.md#619-snow) |
| 112 | `SolarPanel` | `PowerBuilding` | `code/worldobject/generator.h` | [B:FULL] | [03 — 6.2 SolarPanel](03_buildings_and_world_objects.md#62-solarpanel) |
| 113 | `SoulDestroyer` | `Entity` | `code/worldobject/souldestroyer.h` | [B:FULL] | [02 — 21 SoulDestroyer](02_units_and_entities.md#21-souldestroyer) |
| 114 | `SpaceInvader` | `Entity` | `code/worldobject/airstrike.h` | [B:FULL] | [02 — 11.4 SpaceInvader — behaviour](02_units_and_entities.md#114-spaceinvader--behaviour) |
| 115 | `SpaceShip` | `Entity` | `code/worldobject/spaceship.h` | [B:FULL] | [02 — 23 SpaceShip](02_units_and_entities.md#23-spaceship) |
| 116 | `Spam` | `Building` | `code/worldobject/spam.h` | [B:LEGACY] | [03 — 12.24 Spam](03_buildings_and_world_objects.md#1224-spam) |
| 117 | `SpamInfection` | `WorldObject` | `code/worldobject/spam.h` | [B:FULL] | [04 — 6.16 SpamInfection](04_crates_powerups_and_weapons.md#616-spaminfection) |
| 118 | `SpawnBuilding` | `Building` | `code/worldobject/spawnpoint.h` | [B:FULL] | [03 — 4.2 SpawnBuilding](03_buildings_and_world_objects.md#42-spawnbuilding) |
| 119 | `SpawnBuildingLink` | — | `code/worldobject/spawnpoint.h` | [B:FULL] | [03 — SpawnBuildingLink](03_buildings_and_world_objects.md#spawnbuildinglink) |
| 120 | `SpawnBuildingSpirit` | — | `code/worldobject/spawnpoint.h` | [B:FULL] | [03 — SpawnBuildingSpirit](03_buildings_and_world_objects.md#spawnbuildingspirit) |
| 121 | `SpawnLink` | `SpawnBuilding` | `code/worldobject/spawnpoint.h` | [B:FULL] | [03 — 4.3 SpawnLink](03_buildings_and_world_objects.md#43-spawnlink) |
| 122 | `SpawnPoint` | `SpawnBuilding` | `code/worldobject/spawnpoint.h` | [B:FULL] | [03 — 4.5 SpawnPoint](03_buildings_and_world_objects.md#45-spawnpoint) |
| 123 | `SpawnPopulationLock` | `Building` | `code/worldobject/spawnpoint.h` | [B:FULL] | [03 — 4.6 SpawnPopulationLock](03_buildings_and_world_objects.md#46-spawnpopulationlock) |
| 124 | `Spider` | `Entity` | `code/worldobject/spider.h` | [B:FULL] | [02 — 19 Spider](02_units_and_entities.md#19-spider) |
| 125 | `SpiderParameters` | — | `code/worldobject/spider.h` | [B:FULL] | [02 — 19.6 SpiderParameters — gait parameter set](02_units_and_entities.md#196-spiderparameters--gait-parameter-set) |
| 126 | `Spirit` | `WorldObject` | `code/worldobject/spirit.h` | [B:FULL] | [02 — 6 Spirit](02_units_and_entities.md#6-spirit) |
| 127 | `SpiritProcessor` | `ReceiverBuilding` | `code/worldobject/spiritreceiver.h` | [B:LEGACY] | [03 — 12.4 SpiritProcessor](03_buildings_and_world_objects.md#124-spiritprocessor) |
| 128 | `SpiritReceiver` | `ReceiverBuilding` | `code/worldobject/spiritreceiver.h` | [B:LEGACY] | [03 — 12.7 SpiritReceiver](03_buildings_and_world_objects.md#127-spiritreceiver) |
| 129 | `SpiritStore` | — | `code/worldobject/spiritstore.h` | [B:LEGACY] | [03 — 12.9 SpiritStore](03_buildings_and_world_objects.md#129-spiritstore) |
| 130 | `SporeGenerator` | `Entity` | `code/worldobject/sporegenerator.h` | [B:FULL] | [02 — 17 SporeGenerator](02_units_and_entities.md#17-sporegenerator) |
| 131 | `Squadie` | `Entity` | `code/worldobject/insertion_squad.h` | [B:FULL] | [02 — 7.2 Squadie — stats and constants](02_units_and_entities.md#72-squadie--stats-and-constants) |
| 132 | `StaticShape` | `Building` | `code/worldobject/staticshape.h` | [B:FULL] | [03 — 7.4 StaticShape](03_buildings_and_world_objects.md#74-staticshape) |
| 133 | `Statue` | `CarryableBuilding` | `code/worldobject/statue.h` | [B:FULL] | [03 — 8.4 Statue](03_buildings_and_world_objects.md#84-statue) |
| 134 | `Tank` | `Entity` | `code/worldobject/tank.h` | [B:DEBUG] (TankBattle); unreachable in FULL | [02 — 25 Tank](02_units_and_entities.md#25-tank) |
| 135 | `Teleport` | `Building` | `code/worldobject/teleport.h` | [B:FULL] | [03 — 5.2 Teleport](03_buildings_and_world_objects.md#52-teleport) |
| 136 | `Tentacle` | `Entity` | `code/worldobject/tentacle.h` | [B:DEBUG] (via Portal / The Beast) | [02 — 30 Tentacle](02_units_and_entities.md#30-tentacle) |
| 137 | `ThrowableWeapon` | `WorldObject` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.2 ThrowableWeapon](04_crates_powerups_and_weapons.md#62-throwableweapon) |
| 138 | `TrackEnd` | `MineBuilding` | `code/worldobject/mine.h` | [B:LEGACY] | [03 — 12.32 TrackEnd](03_buildings_and_world_objects.md#1232-trackend) |
| 139 | `TrackJunction` | `MineBuilding` | `code/worldobject/mine.h` | [B:LEGACY] | [03 — 12.30 TrackJunction](03_buildings_and_world_objects.md#1230-trackjunction) |
| 140 | `TrackLink` | `MineBuilding` | `code/worldobject/mine.h` | [B:LEGACY] | [03 — 12.29 TrackLink](03_buildings_and_world_objects.md#1229-tracklink) |
| 141 | `TrackStart` | `MineBuilding` | `code/worldobject/mine.h` | [B:LEGACY] | [03 — 12.31 TrackStart](03_buildings_and_world_objects.md#1231-trackstart) |
| 142 | `Tree` | `Building` | `code/worldobject/tree.h` | [B:FULL] | [03 — 7.3 Tree](03_buildings_and_world_objects.md#73-tree) |
| 143 | `Triffid` | `Building` | `code/worldobject/triffid.h` | [B:FULL] | [03 — 9.2 Triffid](03_buildings_and_world_objects.md#92-triffid) |
| 144 | `TriffidEgg` | `Entity` | `code/worldobject/triffid.h` | [B:FULL] | [02 — 14 TriffidEgg](02_units_and_entities.md#14-triffidegg) |
| 145 | `Tripod` | `Entity` | `code/worldobject/tripod.h` | [B:LEGACY] | [02 — 29 Tripod](02_units_and_entities.md#29-tripod) |
| 146 | `TripodNavData` | — | `code/worldobject/tripod.h` | [B:LEGACY] | [02 — 29.2 TripodNavData — hex-direction navigator](02_units_and_entities.md#292-tripodnavdata--hex-direction-navigator) |
| 147 | `TrunkPort` | `Building` | `code/worldobject/trunkport.h` | [B:FULL] | [03 — 8.6 TrunkPort](03_buildings_and_world_objects.md#86-trunkport) |
| 148 | `TurretEmptyShell` | `WorldObject` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.9 TurretEmptyShell](04_crates_powerups_and_weapons.md#69-turretemptyshell) |
| 149 | `TurretShell` | `WorldObject` | `code/worldobject/weapons.h` | [B:FULL] | [04 — 6.8 TurretShell](04_crates_powerups_and_weapons.md#68-turretshell) |
| 150 | `UnprocessedSpirit` | `WorldObject` | `code/worldobject/spiritreceiver.h` | [B:FULL] code path (soulrain level effect; no shipped map) | [03 — 12.8 UnprocessedSpirit](03_buildings_and_world_objects.md#128-unprocessedspirit) |
| 151 | `UpgradePort` | `Building` | `code/worldobject/upgrade_port.h` | [B:LEGACY] | [03 — 12.38 UpgradePort](03_buildings_and_world_objects.md#1238-upgradeport) |
| 152 | `Virii` | `Entity` | `code/worldobject/virii.h` | [B:FULL] | [02 — 16 Virii](02_units_and_entities.md#16-virii) |
| 153 | `ViriiHistory` | — | `code/worldobject/virii.h` | [B:FULL] | [02 — 16.7 ViriiHistory — trail sample](02_units_and_entities.md#167-viriihistory--trail-sample) |
| 154 | `ViriiUnit` | `Unit` | `code/worldobject/virii.h` | [B:FULL] | [02 — 16.6 ViriiUnit — Virii group container](02_units_and_entities.md#166-viriiunit--virii-group-container) |
| 155 | `Wall` | `Building` | `code/worldobject/wall.h` | [B:FULL] | [03 — 7.2 Wall](03_buildings_and_world_objects.md#72-wall) |
| 156 | `WallBuster` | `CarryableBuilding` | `code/worldobject/wallbuster.h` | [B:FULL] | [03 — 8.5 WallBuster](03_buildings_and_world_objects.md#85-wallbuster) |
| 157 | `WorldObject` | — | `code/worldobject/worldobject.h` | [B:FULL] | [02 — 3.2 WorldObject — base of all simulated objects](02_units_and_entities.md#32-worldobject--base-of-all-simulated-objects) |
| 158 | `WorldObjectId` | — | `code/worldobject/worldobject.h` | [B:FULL] | [02 — 3.1 WorldObjectId — object handle](02_units_and_entities.md#31-worldobjectid--object-handle) |
| 159 | `Zombie` | `WorldObject` | `code/worldobject/souldestroyer.h` | [B:FULL] | [02 — 21.6 Zombie — ghost effect](02_units_and_entities.md#216-zombie--ghost-effect) |

Totals: [B:DEBUG] 3, [B:FULL] 103, [B:LEGACY] 53; 159 classes.


## 3 Object identity and handles

### 3.1 `WorldObjectId` — fields

[C] `code/worldobject/worldobject.h:WorldObjectId`, `code/worldobject/worldobject.cpp`. All fields are `protected`; access is through `Get*`/`Set*`.

| Field | Type | Invalid default (default ctor, `SetInvalid`) | Meaning |
|---|---|---|---|
| `m_teamId` | `unsigned char` | 255 | owning team slot 0–5 (`NUM_TEAMS` = 6, `code/globals.h`); 255 = no team |
| `m_unitId` | `int` | −1 | index in the team's `m_units`; −1 = the team's loose list `m_others`; or one of the pseudo-unit codes of §3.2 |
| `m_index` | `int` | −1 | slot index inside the container selected by (team, unit) |
| `m_uniqueId` | `int` | −1 | serial number that disambiguates re-used slots (a generation counter); for buildings it is the building id from the level file or `GlobalWorld::GenerateBuildingId` |
| `s_nextUniqueId` (static, protected) | `int` | 0 | last serial handed out |

Debug-only limits asserted by `Set()`: `teamId < ID_MAXTEAMS (256)`, `unitId < ID_MAXUNITS (65536)`, `index < ID_MAXTROOPS (65536)` (`#define`s in `worldobject.cpp`; `AppDebugAssert`, compiled out in Release) [C] `code/worldobject/worldobject.cpp:WorldObjectId::Set`.

### 3.2 Pseudo-unit codes and the container each id resolves into

| `m_unitId` | `#define` | Container | Resolver | Slot check | uniqueId check |
|---|---|---|---|---|---|
| ≥ 0 | — | `Team::m_units[unitId]->m_entities` | `Location::GetEntity` | `ValidIndex(unitId)`, `ValidIndex(index)` | **yes** |
| −1 | — | `Team::m_others` | `Location::GetEntity` | `ValidIndex(index)` | **yes** |
| −100 | `UNIT_BUILDINGS` | `Location::m_buildings` | `Location::GetBuilding(uniqueId)` — linear scan over all slots, first building whose `m_id.GetUniqueId()` matches; in editing mode reads `LevelFile::GetBuilding` [B:EDITOR] | none (index is −1) | the only key |
| −101 | `UNIT_SPIRITS` | `Location::m_spirits` (by value) | `Location::GetSpirit(int index)` | `ValidIndex(index)` | **no** — spirits are created with uniqueId −1 |
| −102 | `UNIT_LASERS` | `Location::m_lasers` (by value) | none: `GetWorldObject` falls to `GetEntity`, which returns NULL | — | — |
| −103 | `UNIT_EFFECTS` | `Location::m_effects` | `Location::GetEffect` | `ValidIndex(index)` | **yes** |

[C] `code/location.cpp:Location::GetWorldObject`, `GetEntity`, `GetBuilding(int)`, `GetSpirit(int)`, `GetEffect`; `code/worldobject/worldobject.h` (`#define UNIT_*`). Lasers are created by `Location::FireLaser` with no id at all (value objects in a pooled array) [C] `code/location.cpp:Location::FireLaser`.

`Location::GetEntity(id)` exactly [C]: return NULL if `teamId ≥ NUM_TEAMS` or the team slot is `TeamTypeUnused`; if `m_units.ValidIndex(unitId)` and `unit->m_entities.ValidIndex(index)` and the occupant's uniqueId equals `id.uniqueId` → that entity; else if `unitId == −1` and `m_others.ValidIndex(index)` and uniqueId matches → that entity; else NULL. `GetEntitySafe(id, type)` additionally requires `m_type == type`; `GetUnit(id)` resolves only (team, unit). `Location::GetSpirit(WorldObjectId)` is a different function: a linear search for the spirit whose `m_worldObjectId` (the entity that released it) equals the argument, returning the slot or −1 [C] `code/location.cpp:Location::GetSpirit(WorldObjectId)`.

### 3.3 Validity and comparison

- `IsValid()` ⇔ `teamId != 255 || unitId != −1 || index != −1`; the uniqueId is ignored, so `(255, UNIT_BUILDINGS, −1, 42)` is valid and `(255, −1, −1, 42)` is not [C] `worldobject.h:WorldObjectId::IsValid`.
- `operator==` / `operator!=` compare all four fields; the entity grid stores and de-duplicates ids with `==` [C] `worldobject.cpp:operator==`, `code/entity_grid.cpp:EntityGrid::GetNeighbours`.
- A valid-looking id can be stale: validity says nothing about whether the object still exists. Only a resolver answers that.

### 3.4 Allocation — every kind of object

| Object kind | team | unit | index | uniqueId | Set by | Ref |
|---|---|---|---|---|---|---|
| Entity in `m_others` | owner | −1 | slot returned by `m_others.PutData` (LIFO free list, §5.3) | `GenerateUniqueId()` in `Entity::NewEntity` | `Team::NewEntity` → `Entity::NewEntity`; `Location::SpawnEntities` sets team/unit/index | [C] `code/worldobject/entity.cpp:Entity::NewEntity`, `code/team.cpp:Team::NewEntity`, `code/location.cpp:Location::SpawnEntities` |
| Entity in a `Unit` | owner | unit slot | slot in `unit->m_entities` | as above | `Unit::NewEntity`; `SpawnEntities` | [C] `code/unit.cpp:Unit::NewEntity` |
| Building from the level file | owner (level-file *position* remapped to a team by `GetTeamFromPosition`, unless 255/−1) | `UNIT_BUILDINGS` | −1 | level-file id (duplicate id = fatal assert) | `LevelFile::ParseBuildings`, `Location::InitBuildings` | [C] `code/location.cpp:Location::InitBuildings` |
| Building created at run time (crate, crate turret, tree offspring, AI targets of turrets/statues/dishes, statues, chess pieces) | creator | `UNIT_BUILDINGS` | −1 (trees pass a slot) | `GlobalWorld::GenerateBuildingId()` = `s_nextUniqueBuildingId++` | each creator | [C] `code/global_world.cpp:GlobalWorld::GenerateBuildingId`; `code/location.cpp:Location::CreateRandomCrate`; `code/worldobject/tree.cpp`, `gunturret.cpp`, `statue.cpp`, `radardish.cpp`, `chess.cpp`; `code/multiwinia.cpp` |
| Effect in `m_effects` | firing team, or −1/255 | `UNIT_EFFECTS` | slot | `GenerateUniqueId()` | `Location::ThrowWeapon`, `FireRocket`, `FireTurretShell`, `LaunchFireball`, `FireLaser` (muzzle flash), `CreateShockwave`, `AdvanceChristmas` (snow), `AdvanceDustWind`, `AdvanceLightning`; `Tree` (zombie), `Spam` (infection), `GunTurret` (shell, target), `Officer` (order markers), `Tank` (missile, mine) | [C] grep of `GenerateUniqueId` in `code/location.cpp`, `code/worldobject/*.cpp` |
| Effect spawned without an id | — | — | — | — | `Location::AdvanceSoulRain` puts an `UnprocessedSpirit` into `m_effects` **without setting `m_id`** (id stays invalid) | [C] `code/location.cpp:Location::AdvanceSoulRain` |
| Spirit | releasing team | `UNIT_SPIRITS` | slot from `m_spirits.GetNextFree()` | −1 | `Location::SpawnSpirit` (refused, returns −1, for the monster team 4) | [C] `code/location.cpp:Location::SpawnSpirit` |
| Laser | — (team in `Laser::m_fromTeamId`) | — | pooled slot | — | `Location::FireLaser` | [C] `code/location.cpp:Location::FireLaser` |
| Unit | — | `Team::m_units` slot via `GetNextFree` | — | — | `Team::NewUnit` | [C] `code/team.cpp:Team::NewUnit` |
| Route | — | `RoutingSystem::m_routes` slot (DArray: **lowest** free index) | — | — | `RoutingSystem::GenerateRoute` | [C] `code/routing_system.cpp:RoutingSystem::GenerateRoute`; `contrib/systemiv/lib/tosser/darray.cpp:DArray::PutData` |

Counters and their reset points [C]:

| Counter | Reset | Ref |
|---|---|---|
| `WorldObjectId::s_nextUniqueId` (shared by entities **and** effects; pre-incremented, first id 1) | `ResetUniqueIdSeed()` → 0 in `DoEnterLocation`, immediately before `new Location()`; **not** reset by the Assault multi-round `Multiwinia::ResetLocation` (unreachable in the shipped game, [22](22_determinism_and_networking.md)) | `code/main.cpp:DoEnterLocation`, `code/worldobject/worldobject.cpp:WorldObjectId::ResetUniqueIdSeed` |
| `GlobalWorld::s_nextUniqueBuildingId` | 0 at the start of `Location::Init`; set to highest level id + 1 at the end of `InitBuildings` | `code/location.cpp:Location::Init`, `InitBuildings` |

Determinism consequence: ids are identical on all peers only if every peer creates the same objects in the same order. Unique ids are not just handles — they feed gameplay (`uid % 20` worship distance, `uid % 10` fence clearance, abduction blend `(uid % 10) × 0.1`, vortex height `5 × (uid % 100)`, render-damage phase `uid × 10`) [C] `code/worldobject/darwinian.cpp` ([02 §4](02_units_and_entities.md#4-darwinian)). Every effect creation therefore shifts later entity ids, including cosmetic effects such as muzzle flashes and snow [C].

### 3.5 Handle rules

1. **Handles are weak.** Holding a `WorldObjectId` never keeps an object alive; every use must call a resolver and handle NULL [C] `code/location.cpp:Location::GetEntity`.
2. **Entities and effects are generation-checked** (uniqueId must match), so a handle to a deleted object whose slot was re-used resolves to NULL, not to the new occupant [C].
3. **Spirits are not generation-checked**: `GetSpirit(index)` returns whatever spirit occupies the slot. A stored spirit index can silently designate a newer spirit after the original was removed and the slot re-used (LIFO re-use makes this likely within a tick or two) [C] `code/location.cpp:Location::GetSpirit(int)`, `SpawnSpirit`; impact on spirit collectors [I] — see [02 §6](02_units_and_entities.md#6-spirit) and 02-U items.
4. **Buildings are found by uniqueId only**, by a linear scan of `m_buildings` (O(n) per lookup; the obstruction grid, port logic and every building link resolve this way) [C] `Location::GetBuilding(int)`.
5. **Lasers have no handle** [C].
6. **Network commands address entities by slot index** inside the sender's team (TERMINATE, PROMOTE, TOGGLEENTITY, SELECTUNIT entity id) and buildings by uniqueId (AIMBUILDING, SELECTUNIT building id, TOGGLELASERFENCE). Slot allocation history must therefore be identical on all peers; it is, because all creation and removal happens inside the lockstep slices [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessServerUpdates` ([22](22_determinism_and_networking.md)).
7. **Team selection is stored as raw ints** (`Team::m_currentUnitId`, `m_currentEntityId`, `m_currentBuildingId`), not as `WorldObjectId`s; they are changed only through network commands ("Do not set these directly") and re-validated on use by `Team::GetMyUnit/GetMyEntity` [C] `code/team.h:Team`, `code/team.cpp:Team::GetMyEntity`. Because the entity id is an index without a uniqueId, a selection survives the death of the selected entity and then designates whatever entity re-uses the slot [I] from the type of the field; `Unit::~Unit` clears `m_currentUnitId` when the selected unit is deleted [C] `code/unit.cpp:Unit::~Unit`.
8. **Grid membership is keyed by the full id**, including uniqueId; the id used for removal is rebuilt from (team, unit, slot, uniqueId) by the container owner, so it always matches the inserted one [C] `code/team.cpp:Team::Advance`, `code/unit.cpp:Unit::AdvanceEntities`.

## 4 Base-class contracts

### 4.1 `WorldObject`

[C] `code/worldobject/worldobject.h:WorldObject`, `code/worldobject/worldobject.cpp`. Member table and `Effect*` enum: [02 §3.2](02_units_and_entities.md#32-worldobject--base-of-all-simulated-objects).

| Hook | Default | Contract |
|---|---|---|
| ctor | `m_onGround = false`, `m_enabled = true`, `m_type = 0` | id invalid until the creator sets it |
| `virtual bool Advance()` | returns false | parameterless form used by effects, lasers, spirits and buildings; **true = remove and delete me** (lasers and spirits are removed from their pooled arrays without `delete`) [C] `code/location.cpp:Location::AdvanceWeapons`, `AdvanceSpirits` |
| `virtual void Render(double t)` | nothing | `t` = prediction time to extrapolate by; must not write simulated fields ([§10.3](#103-render-functions-that-write-fields-of-simulated-objects)) |
| `virtual bool RenderPixelEffect(double t)` | false | "return true if you did anything"; the pixel effect never runs in Multiwinia ([24](24_rendering.md)) |
| `virtual char *LogState(char *msg)` | static buffer `"WOBJ Type[%d] Id[%d] pos[x,y,z], vel[sum]"` with `HashDouble` | sync-log fingerprint ([22](22_determinism_and_networking.md)) |
| `void BounceOffLandscape()` | §8.5 | non-virtual helper |

### 4.2 `Entity`

[C] `code/worldobject/entity.h:Entity`, `code/worldobject/entity.cpp`. Full member and virtual tables: [02 §3.4](02_units_and_entities.md#34-entity--base-class-of-all-units). The engine-level contract:

| Phase | Rule | Ref |
|---|---|---|
| Construction | only through `Entity::NewEntity(type)` (class factory over the 27-value `Type*` enum; asserts on `TypeInvalid`), which also calls `m_id.GenerateUniqueId()`; constructors must not depend on the team (id not yet set) | [C] `Entity::NewEntity` |
| Initialisation | `SpawnEntities` sets type (`SetType` → stats, `m_reloading = syncfrand(Rate)`), position (`FindValidSpawnPosition`), `m_onGround = false`, velocity, front, id, spawn point, roam range, route; then `Begin()`; then inserts into the entity grid with `m_radius`. `Begin()` is where team-coloured shapes are set and where `m_centrePos`/`m_radius` are computed from the shape | [C] `code/location.cpp:Location::SpawnEntities`, `Entity::Begin` |
| Per tick | `bool Advance(Unit *unit)` (`unit` = owning unit or NULL for `m_others`); called only while `m_enabled`; **true = I am dead, remove and delete me**; false = keep. After a false return the owner moves the entity in the grid from its pre-Advance position | [C] `code/team.cpp:Team::Advance`, `code/unit.cpp:Unit::AdvanceEntities` |
| Base `Advance` sequence | (1) `!m_enabled` → false; `m_destroyed` → true. (2) `m_dead` → `AdvanceDead`, true → true. (3) `m_reloading −= dt` (≥ 0). (4) cosmetic `m_renderDamaged` (unsynced `frand`, wall clock). (5) `m_inWater ≠ −1` → `AdvanceInWater`. (6) clamp x, z into [0, worldSize]. (7) in a unit and alive → `unit->UpdateEntityPosition(pos, radius)`. (8) `m_routeId ≠ −1` → `FollowRoute()`. (9) false. Which subclasses call it (first, last, never): [02 §3.4](02_units_and_entities.md#34-entity--base-class-of-all-units) | [C] `Entity::Advance` |
| Dying | `ChangeHealth` sets `m_dead = true`, health := 100 (re-used as fade alpha), spawns one spirit; the entity stays in its container and in the grid while `AdvanceDead` fades it (−4 per tick on ground, −2 in the air); removal happens when `AdvanceDead` returns true | [C] `Entity::ChangeHealth`, `Entity::AdvanceDead` |
| Instant removal | set `m_destroyed = true`; the next `Advance` returns true | [C] `Entity::Advance` |
| Freezing | `m_enabled = false`: the owner skips `Advance` entirely; `Team::Advance` removes a just-disabled entity from the grid (`Unit::AdvanceEntities` does not). Used by teleports while an entity is in transit; the teleport re-enables it and re-inserts it into the grid on exit | [C] `code/team.cpp:Team::Advance`, `code/worldobject/radardish.cpp`, `code/worldobject/teleport.cpp` |
| Grid invariant | an entity must not change `m_radius` between grid updates (removal uses the current radius at the old position); code that teleports an entity outside `Advance` must update the grid itself (RadarDish, Bridge, Centipede, SoulDestroyer and Shaman do) | [C] `code/entity_grid.cpp:EntityGrid::UpdateObject`; grep of `m_entityGrid->AddObject` |
| Render hooks | `Render(double t)` (empty in base), `IsInView()` (true in base), `RenderShadow` static helpers; Darwinians and Virii are batch-rendered by `Team::RenderDarwinians` / `RenderVirii` instead of per-entity `Render` | [C] `code/team.cpp:Team::Render` ([24](24_rendering.md)) |
| Population counter | `static s_entityPopulation`: ++ in ctor, −− in dtor (the AI entity subtracts itself); reset to 0 in `~Location` | [C] `Entity::Entity`, `code/location.cpp:Location::~Location` |

### 4.3 `Building`

[C] `code/worldobject/building.h:Building`, `code/worldobject/building.cpp`. Full specification: [03 §3](03_buildings_and_world_objects.md#3-building-base-class). The engine-level contract:

| Phase | Rule | Ref |
|---|---|---|
| Construction | `Building::CreateBuilding(int type)` (by id; ignores the block list) or `CreateBuilding(char *name)` (level parsing; refuses blocked or unknown names). Team defaults to 255 in Multiwinia | [C] `Building::CreateBuilding`, `Building::Building` |
| Initialisation | template pattern: a template object of the concrete class is filled by `Read()` (level file) or by the creator on the stack; the live object copies it in `Initialise(template)` (snaps y to terrain, computes `m_centrePos`/`m_radius` from the shape or 13.0, collects lights and ports); `SetDetail(1)` rebuilds ports | [C] `Building::Initialise`, `Building::SetDetail` |
| Insertion | `m_buildings.PutData(building)`; runtime creators then usually call `m_obstructionGrid->CalculateAll()` themselves (tree.cpp, crate.cpp ×4, engineer.cpp, armour.cpp) | [C] grep of `CalculateAll` |
| Per tick | `bool Advance()`: base = `if (m_destroyed) return true; EvaluatePorts(); return false;` | [C] `Building::Advance` |
| Removal | true → `Location::AdvanceBuildings` removes the slot, deletes the building, calls `RecalculateAITargets()` at once if it was an `AITarget` or `Wall`, and after the slice loop calls `ObstructionGrid::CalculateAll()` once. The two routing systems are **not** rebuilt (their node tables are built once in `Location::Init`) | [C] `code/location.cpp:Location::AdvanceBuildings`; grep of `m_routingSystem.Initialise` (one call) |
| Ownership | `SetTeamId(int)` is the normal mutation point (sound `ChangeTeam`, GlobalBuilding mirror); some paths write `m_id.SetTeamId` directly ([03 §3.9](03_buildings_and_world_objects.md#39-ownership-change-paths)) | [C] |
| Collision | `DoesSphereHit`, `DoesShapeHit`, `DoesRayHit` — precise tests delegated to the shape; base indestructible (`Damage` only plays a sound) | [C] ([03 §3.14](03_buildings_and_world_objects.md#314-hit-test-overrides-and-shapes)) |
| Render hooks | `Render(t)` (opaque shape), `RenderAlphas(t)` (lights, ports, transparent parts), `IsInView()`, `PerformDepthSort(Vector3&)` (true → drawn in the far-to-near alpha sort) | [C] `code/location.cpp:Location::RenderBuildings`, `RenderBuildingAlphas` |

### 4.4 Effects, lasers and spirits

- **Effects** (`WorldObject` subclasses in `m_effects`): created with `new`, given `(team, UNIT_EFFECTS, slot, uniqueId)`, advanced with `Advance()`; true → `RemoveData` + `delete` [C] `code/location.cpp:Location::AdvanceWeapons`. Type enum `WorldObject::Effect*` (23 values) — [02 §3.2](02_units_and_entities.md#32-worldobject--base-of-all-simulated-objects); per-class behaviour [04 §6](04_crates_powerups_and_weapons.md#6-weapons-projectiles-and-effect-objects).
- **Lasers** (`Laser` by value in `m_lasers`): `FireLaser` takes a slot with `GetPointer()` (allocate) and calls `Laser::Initialise(lifetime)`; `Advance()` true → `RemoveData` (no destructor call on the pooled element) [C] `Location::FireLaser`, `AdvanceWeapons`.
- **Spirits** (`Spirit` by value in `m_spirits`, presized 100): `SpawnSpirit` fills the slot, sets `m_id = (team, UNIT_SPIRITS, slot, −1)`, calls `Begin()`; `Advance()` true → `RemoveData` [C] `Location::SpawnSpirit`, `AdvanceSpirits`.
- **Pointer stability**: by-value pools copy their elements when they grow (`FastDArray::SetSize` copies into a new array), so a `Spirit*` or `Laser*` held across any creation in the same pool may dangle; all code uses indices or ids across ticks [C] `contrib/systemiv/lib/tosser/fast_darray.cpp:FastDArray::SetSize`; [I] no held-pointer bug was found in the P03 files.

### 4.5 `Unit` (group container)

`Unit` (`code/unit.h:Unit`) is not a `WorldObject`: it is a team-owned container of same-type entities moving as a group. Its own `Advance(unitIndex)` (slice 0 only) recomputes the group centroid, velocity and radius from the members' `UpdateEntityPosition` calls of the previous tick, follows a level route and returns **true when it has no used slot left** (the team then deletes it) [C] `code/unit.cpp:Unit::Advance`. Members, formation offsets and subclasses: [§9.6](#96-unit--groups-of-entities-in-formation).

### 4.6 Render hooks — where each family is drawn

| Family | Draw call | Prediction time used | Ref |
|---|---|---|---|
| Buildings | `Location::RenderBuildings` → `Building::Render(t)` for in-view buildings; `RenderBuildingAlphas` → `RenderAlphas(t)`, depth-sorted when `PerformDepthSort` | `t` = `g_predictionTime`, + `SERVER_ADVANCE_PERIOD` for slots above `m_buildings.GetLastUpdated()` | [C] `code/location.cpp:Location::RenderBuildings` |
| Entities in `m_others` | `Team::RenderOthers`, `RenderDarwinians`, `RenderVirii` | as above against `m_others.GetLastUpdated()` | [C] `code/team.cpp:Team::RenderOthers`, `RenderDarwinians` |
| Unit members | `Unit::Render(t)` for in-view units | as above against `m_entities.GetLastUpdated()` | [C] `code/unit.cpp:Unit::Render` |
| Effects, lasers | `Location::RenderWeapons` | as above | [C] `Location::RenderWeapons` |
| Spirits | `Location::RenderSpirits` (monster spirits subtractive first, others additive) | as above | [C] `Location::RenderSpirits` |
| Route trails | `RoutingSystem::Render` | wall clock (pulse) and `g_advanceTime` (fade) | [C] `code/routing_system.cpp:RoutingSystem::Render` |

Render order of the whole frame: [24](24_rendering.md).

## 5 Location: containers, creation, removal and update ordering

`Location` (`code/location.h:Location`) is the container and service façade for everything simulated in one match. Exactly one exists at a time (`g_app->m_location`); it is created on the loading worker thread in `DoEnterLocation` and destroyed by `App::ShutdownCurrentGame` ([20 §8](20_architecture.md#8-lifetimes-process-session-match)). Simulation code reaches it through the global, never as a parameter [C] `code/main.cpp:DoEnterLocation`, `code/app.cpp:App::ShutdownCurrentGame`.

### 5.1 Location members

[C] `code/location.h`, `code/location.cpp:Location::Location`, `Location::Init`.

| Member | Type | Initial value | Meaning |
|---|---|---|---|
| `m_lastSliceProcessed` (protected) | int | 0 | last slice passed to `Advance`; read by the cloud render offset |
| `m_missionComplete` (protected) | bool | false | Darwinia objective flag [B:LEGACY] |
| `m_coopGroupColourTaken[2][2]` (private) | bool | false | per co-op group: member-0 / member-1 shade already handed out by `GetGroupColour` |
| `m_showCrateDropMessage` (private) | bool | true | show the "crate_drop" message only for the first crate |
| `m_landscape` | `Landscape` (by value) | — | height map, normal map, render data ([23](23_terrain_and_landscape.md)) |
| `m_entityGrid` | `EntityGrid*` | NULL; `new EntityGrid(32.0, 32.0)` in `Init` | spatial hash of entities (§6.2) |
| `m_entityGridCache` | `EntityGridCache*` | NULL; created in `Init`; deleted in `~Location` and `Empty` | memo of enemy queries (§6.3) |
| `m_obstructionGrid` | `ObstructionGrid*` | NULL; `new ObstructionGrid(s, s)`, s = **32.0 in Multiplayer, 64.0 otherwise** | building footprint grid (§6.4) |
| `m_levelFile` | `LevelFile*` | NULL; `LoadLevel` | parsed map + mission ([08](08_levels_and_level_design.md), [29](29_data_formats_reference.md)) |
| `m_clouds` | `Clouds*` | NULL; `Init` | sky clouds; advanced at slice 3 (cosmetic) |
| `m_water` | `Water*` | NULL; `Init` | advanced per render frame by the main loop (cosmetic) |
| `m_routingSystem` | `LandRoutingSystem` | — | land pathfinding (§7) |
| `m_seaRoutingSystem` | `SeaRoutingSystem` | — | sea pathfinding for Armour (§7) |
| `m_teams[NUM_TEAMS]` | `Team*[6]` | NULL; `InitTeams` creates **all six** (even unused) | §9 |
| `m_teamPosition[NUM_TEAMS]` | int | −1 | team id → map start position (0..numPlayers−1) |
| `m_christmasTimer` | double | −99.9 | shared timer of the snow, dust and lightning spawners (< −90 = first-run burst) |
| `m_spawnSync` | double | 0.0 | soul-rain spawner timer |
| `m_crateTimer` | double | 0.0 | crate-drop countdown ([04 §1.3](04_crates_powerups_and_weapons.md#13-drop-timer--locationadvancecrates)) |
| `m_spawnManiaTimer`, `m_bitzkreigTimer` | double | 0.0 | crate effect timers, decremented at slice 8 |
| `m_allowMonsterTeam` | bool | true | gates `GetMonsterTeamId`/`GetFuturewinianTeamId` (→ −1 when false) |
| `m_teamsInitialised` | bool | false | set at the end of `InitTeams` |
| `m_messageTimer`, `m_message`, `m_messageTeam`, `m_messageNoOverwrite` | double, UnicodeString, int, bool | 0.0, empty, −1, false | centre-screen message (real-time expiry 3.0 s; team 255 = white) — presentation |
| `m_lights` | `FastDArray<Light*>` | step 1 | level lights (render data) |
| `m_buildings` | `SliceDArray<Building*>` | step 10, 10 slices | all buildings, crates included |
| `m_spirits` | `SliceDArray<Spirit>` (by value) | size 100, step 100, 10 slices | released souls |
| `m_lasers` | `SliceDArray<Laser>` (by value) | step 100, 10 slices | laser bolts |
| `m_effects` | `SliceDArray<WorldObject*>` | step 100, 10 slices | grenades, rockets, shells, fireballs, shockwaves, muzzle flashes, snow, dust balls, lightning, pulse waves, spam infections, zombies, order markers, unprocessed spirits and the other effect classes of [04 §6](04_crates_powerups_and_weapons.md#6-weapons-projectiles-and-effect-objects) |
| `m_chatMessages` | `LList<ChatMessage*>` | — | newest first; presentation only |
| `m_windDirection` | Vector3 | zero | set by the dust-wind spawner |
| `m_coopGroups[NUM_TEAMS]` | int | 0 | 0 = none, 1 / 2 = alliance group (§9.5) |
| `m_groupColourId[2]` | int | −1 | colour id chosen for each co-op group |
| `m_burnPatches` | `LList<BurnPatch>` | — | scorched terrain patches; adding one rebuilds landscape GL state [B:LEGACY] in practice ([23 §11.5](23_terrain_and_landscape.md#115-burn-patches-blegacy)) |
| `m_christmasMode` | bool | false | copied from `ClientToServer::m_christmasMode` at `Init` |
| `m_sortedBuildingsAlphas`, `m_sizeSortedBuildingsAlphas` (private) | `DepthSortedBuilding*`, int | NULL, 0 | growable scratch buffer (step 128) for the alpha depth sort — render only |
| `s_neighbours` (static) | `std::vector<WorldObjectId>` | — | shared scratch result buffer for grid queries |
| `s_neighboursInUse` (static) | bool | false | re-entrancy guard used only by `Bang` (falls back to a local vector when set) |

Structs: `ChatMessage {m_msg, m_clientId, m_recievedTime (real time), m_colour, m_senderName}`; `BurnPatch {m_pos, m_radius}`; `DepthSortedBuilding {m_buildingIndex, m_distance (float, squared distance)}` [C] `code/location.h`.

### 5.2 Every simulation container

| Container | Owner | Type | Element identity | Added by | Removed when | Sliced? |
|---|---|---|---|---|---|---|
| `m_others` | `Team` | `SliceDArray<Entity*>` step 100 | (team, −1, slot, uid) | `Location::SpawnEntities` → `Team::NewEntity(type, −1)` → `Entity::NewEntity` | `Advance(NULL)` true → grid remove at the pre-Advance position, `RemoveData`, `delete` | yes |
| `m_entities` | `Unit` | `SliceDArray<Entity*>` step 100, presized to the requested count | (team, unit, slot, uid) | `SpawnEntities` with `unitId ≠ −1` → `Unit::NewEntity` | `Advance(unit)` true → `Unit::RemoveEntity` (grid remove + delete) | yes |
| `m_units` | `Team` | `FastDArray<Unit*>` step 5 | unit slot | `Team::NewUnit` (class by troop type: `InsertionSquad`, `AirstrikeUnit`, `ViriiUnit`, else `Unit`) | `Unit::Advance` true (no used slot) at slice 0 → `RemoveData`, `delete` | no (all units every slice) |
| `m_specials` | `Team` | `LList<WorldObjectId>` | id copy | `Team::RegisterSpecial` (Officer, Armour, Harvester, Engineer (MP), Tank, Shaman in their `Begin`) | `UnRegisterSpecial` | no (lookup list) |
| `m_buildings` | `Location` | `SliceDArray<Building*>` step 10 | (team, `UNIT_BUILDINGS`, −1, uid) | `InitBuildings`, `CreateRandomCrate`, runtime creators (§3.4) | `Advance()` true → `RemoveData`, `delete`; `RecalculateAITargets()` if AITarget/Wall; `ObstructionGrid::CalculateAll()` after the slice | yes |
| `m_spirits` | `Location` | `SliceDArray<Spirit>` by value, presized 100, step 100 | (team, `UNIT_SPIRITS`, slot, −1) | `SpawnSpirit` (−1 for the monster team) | `Spirit::Advance()` true → `RemoveData` | yes |
| `m_lasers` | `Location` | `SliceDArray<Laser>` by value, step 100 | none | `FireLaser` | `Laser::Advance()` true → `RemoveData` | yes |
| `m_effects` | `Location` | `SliceDArray<WorldObject*>` step 100 | (team or 255, `UNIT_EFFECTS`, slot, uid) | §3.4 | `Advance()` true → `RemoveData`, `delete` | yes |
| `m_lights` | `Location` | `FastDArray<Light*>` | — | `InitLights` | `Empty` | render data |
| `m_chatMessages`, `m_burnPatches` | `Location` | `LList` | — | chat / `AddBurnPatch` | after 60 s / never | presentation |
| `m_cells[team][cell]` | `EntityGrid` | growable id arrays per team per cell | `WorldObjectId` | `AddObject` | `RemoveObject` | — (§6.2) |
| `m_cache` | `EntityGridCache` | `FastDArray<EntityGridCacheElement*>` size 20, step 20 | slot | cache miss in `GetEnemies` | older than 1.0 s game time at slice 7 | — (§6.3) |
| `m_cells` | `ObstructionGrid` | `SurfaceMap2D<ObstructionGridCell>`; each cell `LList<int>` of building ids | building uid | `CalculateAll` | `CalculateAll` clears | — (§6.4) |
| `m_nodes`, `m_routes` | `RoutingSystem` | `DArray<RouteNode*>`, `DArray<Route*>` | slot | `Initialise`, `GenerateRoute` | never / `DeleteRoute` | — (§7) |
| `m_ports` | `Building` | `LList<BuildingPort*>` | port index | `SetShapePorts` | `SetDetail` rebuilds | — ([03 §3.6](03_buildings_and_world_objects.md#36-ports-occupancy-and-reservation)) |
| `m_particles` | `ParticleSystem` | `SliceDArray<Particle>` | none | `CreateParticle` (returns void) | life over | cosmetic; all particles advance at slice 0 only ([26](26_animation_and_vfx.md)) |

[C] `code/location.h`, `code/team.h`, `code/unit.h`, `code/particle_system.h`, `code/location.cpp:Location::Location`, `code/team.cpp:Team::Team`, `code/unit.cpp:Unit::Unit`.

### 5.3 Container semantics: iteration order and slot re-use

All are header templates whose bodies live in the `.cpp` the header includes [C] `contrib/systemiv/lib/tosser/*.h`, `code/lib/*.h`. The simulation uses `LList`, `DArray`, `FastDArray`, `SliceDArray`, `Array2D`/`SurfaceMap2D` and `std::vector`; the others are listed because they define the ordering of data the simulation reads (language tables, archives).

| Container | Storage | Insert | Remove | Iteration order | Notes |
|---|---|---|---|---|---|
| `LList<T>` | doubly linked list + cached cursor (`m_previous`, `m_previousIndex`) | `PutData` = `PutDataAtEnd`, `PutDataAtStart`, `PutDataAtIndex` (O(n)) | `RemoveData(i)` | index order 0…n−1 (insertion order unless inserted at an index) | `GetData(i)` walks from first/last/cursor, whichever is nearest; sequential access O(1) amortised; **const reads move the cursor** (not thread-safe); `GetData` on an invalid index returns `(T)0` |
| `DArray<T>` | array + `shadow` used-flags | `PutData(x)` takes the **lowest free index** (linear scan), else grows; `PutData(x, i)` grows until i fits | `RemoveData(i)` clears the flag (no compaction; indices stable) | slot order 0…Size()−1 skipping unused (`ValidIndex`) | grow step default 1 (`SetStepDouble` → doubling); `Size()` = capacity, `NumUsed()` counts flags; accessors bounds-check with **release asserts** |
| `FastDArray<T>` : DArray | + free list | `PutData(x)` / `GetNextFree()` pop the **head of the free list = most recently freed slot (LIFO)**; when full, grows by the step (or doubles for step −1): new slots are linked in ascending order and the last new slot links to the previous head | `RemoveData(i)` pushes i on the free-list head | slot order | `PutData(x, i)` / `MarkUsed` rebuild the free list ascending; growth **copies by-value elements** (pointers into the array dangle) |
| `SliceDArray<T>` : FastDArray | + slice bookkeeping (`totalNumSlices`, `lastSlice`, `lastUpdated`) | as FastDArray | as FastDArray | slot order; `GetNextSliceBounds` (§5.4) | the constructor is declared `(newstepsize, totalNumSlices)` but defined `(totalNumSlices, newstepsize)` — the first argument is the slice count; every simulation container sets both explicitly (`SetTotalNumSlices`, `SetStepSize`) so the mismatch is inert |
| `BoundedArray<T>` | fixed array | — | — | index | release-assert bounds check |
| `PresizedArray<T,N>` | `T m_data[N]` | — | — | index | debug-only bounds assert |
| `BTree<T>` | unbalanced binary search tree keyed by `char*`, `stricmp` | `PutData` (equal keys go left; duplicates allowed) | `RemoveData` (replaces the node with its left child and appends the right subtree; leaks nodes) | `ConvertToDArray` / `ConvertIndexToDArray` give **pre-order** (node, left, right) — not sorted; depends on insertion order | `GetData` returns 0 if absent; language tables, archive maps (`Resource::m_resourceFiles`) |
| `HashTable<T>` | open addressing, linear probing, `char*` keys, `stricmp` | `PutData` grows ×2 when `slotsFree × 2 ≤ size` (load ≤ 50 %); **inserting an existing key aborts** (`AppAbort("Error with HashTable")`) | `RemoveData` marks + full rehash | slot order (hash-dependent) | hash: 16-bit sum over character pairs `(c0 & 0xDF) + ~(c1 & 0xDF)` (+ last odd character), masked by `size − 1`; initial size 4; max 65536 (release assert) |
| `SortingHashTable<T>` : HashTable | + `short` linked list in alphabetical (`stricmp`) order | O(n) ordered insert | buggy when removing the first element (walk runs off the list) | `StartOrderedWalk` / `GetNextOrderedIndex` alphabetical | profiler only |
| `Array2D<T>` | row-major `T[cols × rows]` (`unsigned short` dimensions) | `PutData(x, y)` ignores out-of-range | — | — | out-of-range `GetData`/`GetPointer` return the single shared `m_outsideValue` |
| `SurfaceMap2D<T>` : Array2D | + world transform (`m_x0`, `m_y0`, cell sizes) | — | — | — | `GetValue` interpolates bilinearly; `GetValueNearest`/`GetPointerNearest` use `floor(x / cell)` **without subtracting `m_x0`** (only correct for origin 0, which every user has); height map, normal map, water depth, obstruction grid ([23 §7.3](23_terrain_and_landscape.md#73-bilinear-resampling-and-the-unsigned-short-conversion-edge-cases)) |
| `auto_vector<T>` | `std::vector<T*>` owning pointers | `push_back(auto_ptr)`, `insert` | `erase` deletes | vector order | input system lists |
| `Directory` | DArrays of data and subdirectories | `CreateData` (replace by name) | `RemoveData` | slot order | network letters ([29](29_data_formats_reference.md)) |

[C] `contrib/systemiv/lib/tosser/llist.cpp`, `darray.cpp`, `fast_darray.cpp`, `bounded_array.h`, `btree.cpp`, `hash_table.cpp`, `sorting_hash_table.cpp`, `code/lib/slice_darray.cpp`, `code/lib/2d_array.cpp`, `code/lib/2d_surface_map.cpp`, `code/lib/auto_vector.h`.

Guarantees a re-implementation must reproduce for bit-compatibility, or replace by one documented policy [C]/[I]:
- Iteration of every simulation array is by **ascending slot index**, including the slots created during the iteration (§5.4).
- `FastDArray` re-use is **LIFO** (most recently freed first); `DArray` re-use is **lowest index first**. The same sequence of operations therefore yields different slots in the two container kinds [C]; routes (`DArray`) and entities/effects (`FastDArray`) differ.
- Slot indices never move; removal never compacts.
- No simulation code iterates a hash map, sorts by pointer, or depends on allocation addresses (`qsort` is used only for the alpha depth sort; `std::map<Entity*, …>` only in the `TRACK_SYNC_RAND` watch list) [C] grep over `code/worldobject`, `location`, `team`, `unit`, `taskmanager`, `multiwinia`, `entity_grid`, `routing_system`, `obstruction_grid`, `level_file`, `global_world` (X3).

### 5.4 Slice partitioning

`SliceDArray::GetNextSliceBounds(slice, &lower, &upper)` [C] `code/lib/slice_darray.cpp`:

```
lower = (slice == 0) ? 0 : lastUpdated + 1
upper = (slice == totalNumSlices - 1) ? Size() - 1
                                      : lower + int(Size() / (float)totalNumSlices)
lastUpdated = upper; lastSlice = slice
```

- `Size()` is the **capacity** (empty slots included), read at the moment each slice starts. Slices 0–8 each cover `int(Size()/10) + 1` slots (inclusive bounds) and slice 9 the remainder, which can be empty or negative-length [C].
- An object created during a tick into a slot **above** the current `lastUpdated` is advanced later in the same tick; one created into a re-used slot **at or below** it waits until the next tick. If the container grows during a tick, later slice bounds are computed from the new capacity [C]/[I] arithmetic.
- Every object present for the whole tick is advanced exactly once per tick [C] ([20 §10.3](20_architecture.md#103-client-tick-and-slices)).
- Within a tick, objects in earlier slices see the previous-tick state of objects in later slices; ordering between objects is by slot index, which is history-dependent but identical on all peers [C]/[I].
- `GetLastUpdated()` (the last upper bound) is what renderers compare slot indices against to pick the extrapolation offset (§4.6).
- A debug assert enforces that slices are requested in order 0, 1, …, 9, 0 [C].

### 5.5 Per-slice update order (exact)

The only caller of `Location::Advance` is `ClientToServer::ProcessAllServerLetters` on the main thread, once per slice while `!App::GamePaused()` [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters`. Within one tick, before slice 0, the server letter's commands are applied (`ProcessServerUpdates`: ALIVE → `Location::UpdateTeam`, SELECTUNIT, RUNPROGRAM, …), the game timer ticks and the sync byte is generated ([20 §10.3](20_architecture.md#103-client-tick-and-slices), [22](22_determinism_and_networking.md)).

`Location::Advance(slice)` [C] `code/location.cpp:Location::Advance`:

1. `m_lastSliceProcessed = slice`.
2. Sub-steps 0–4, serially in this order (written as `#pragma omp parallel for schedule(dynamic)` over `step = 0..4`, but OpenMP is off in every configuration, so the loop is serial — see [20 §10.3](20_architecture.md#103-client-tick-and-slices)):
   1. **Teams** — `AdvanceTeams(slice)`: teams 0…5 in index order, skipping `TeamTypeUnused`; each `Team::Advance(slice)` (§5.6).
   2. **Weapons** — `AdvanceWeapons(slice)`: this slice of `m_lasers` (`Laser::Advance`, remove), then this slice of `m_effects` (`WorldObject::Advance`, remove + delete).
   3. **Buildings** — `AdvanceBuildings(slice)`: this slice of `m_buildings`, each bracketed by `SyncRandLogText(LogState)`; removals as in §5.2; then, if anything was removed, `ObstructionGrid::CalculateAll()`.
   4. **Spirits** — `AdvanceSpirits(slice)`: this slice of `m_spirits`.
   5. **Clouds** — `AdvanceClouds(slice)`: `m_clouds->Advance()` only when slice == 3 (cosmetic offset, no RNG).
3. Darwinia objectives: `!m_missionComplete && MissionComplete()` → `DoMissionCompleteActions()` (Multiwinia maps carry only the automatic never-true objective) [B:LEGACY].
4. slice == 3: level effects, in this order — `LevelEffectSnow` → `AdvanceChristmas()`; `LevelEffectSoulRain` → `AdvanceSoulRain()`; `LevelEffectDustStorm` → `AdvanceDustWind()`; `LevelEffectLightning` → `AdvanceLightning()`; then `ChristmasModEnabled()` → `AdvanceChristmas()` again (§5.10).
5. slice == 7: `m_entityGridCache->Advance()` (evict entries older than 1.0 s).
6. `Multiplayer()` and slice == 8: `AdvanceCrates()`; `m_spawnManiaTimer −= SERVER_ADVANCE_PERIOD` if > 0; `m_bitzkreigTimer −= SERVER_ADVANCE_PERIOD` if > 0.

After `Location::Advance(slice)` returns: `ParticleSystem::Advance(slice)` (cosmetic); and at slice 8 in Multiplayer `Multiwinia::Advance()` (mode rules, scoring, victory) — which runs **even while paused** [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters` ([01](01_game_modes.md)).

Which work runs on which slice:

| Slice | Work |
|---|---|
| every slice 0–9 | one tenth of every team's `m_others` and of every unit's `m_entities`; one tenth of `m_lasers`, `m_effects`, `m_buildings`, `m_spirits` |
| 0 | every `Unit::Advance` (centroid/radius/velocity, route following, empty-unit removal); per-team selected-Darwinian count and centre latched; all particles |
| 1 | every team's `TaskManager::Advance()`; team kill-milestone messages (local team only) |
| 3 | clouds; weather and level-effect spawners |
| 7 | entity-grid cache eviction |
| 8 | crates and crate timers (inside `Location::Advance`), then `Multiwinia::Advance()` |

### 5.6 `Team::Advance(slice)` and `Unit::AdvanceEntities(slice)`

`Team::Advance(slice)` runs only when `m_teamType > TeamTypeUnused` [C] `code/team.cpp:Team::Advance`:

1. slice 1: `m_taskManager->Advance()`; if `m_teamKills` changed and this is the local team, the first crossed milestone of 500 / 1000 / 2000 / 3000 / 4000 / 5000 shows `multiwinia_kills_N` (else-if chain: one per tick); `m_oldKills = m_teamKills`.
2. Every slice: for each valid unit in slot order, `Unit::AdvanceEntities(slice)`.
3. slice 0: for each valid unit in slot order, `Unit::Advance(unitIndex)`; true → `RemoveData` + `delete`.
4. slice 0: `m_numDarwiniansSelected = m_newNumDarwiniansSelected`; `m_selectedDarwinianCentre = m_newSelectedDarwinianCentre / count` (**division by zero when count is 0** → NaN/∞ centre, consumed only by the UI route preview and order code that first checks the count [I]); accumulators reset.
5. This slice of `m_others`: for each valid **enabled** entity: remember `oldPos` and `myId = (team, −1, slot, uid)`; `dead = Advance(NULL)`; a live, not-dead Darwinian with `m_underPlayerControl` adds to the selection accumulators; then dead → grid `RemoveObject(myId, oldPos, radius)`, `RemoveData`, `delete`; else if now disabled → grid `RemoveObject`; else grid `UpdateObject(myId, oldPos → m_pos, radius)`.

`Unit::AdvanceEntities(slice)` [C] `code/unit.cpp:Unit::AdvanceEntities`: for this slice of `m_entities`, each enabled entity: `oldPos`; `dead = Advance(this)`; dead → `RemoveEntity(slot, oldPos)` (grid remove + delete); else `UpdateObject((team, unit, slot, uid), oldPos → m_pos, radius)`. Unlike `Team::Advance` it does not remove an entity that disabled itself during its `Advance` from the grid [C].

Ordering consequence: all units' members of team 0 advance before team 0's loose entities, which advance before any entity of team 1 [C].

### 5.7 Creation rules

| Kind | Rule | Ref |
|---|---|---|
| Entities | only through `Location::SpawnEntities(pos, team, unitId, type, n, vel, spread, range = −1, routeId = −1, routeWaypointId = −1)`. If `team` is the monster (4) or Futurewinian (5) slot and it is unused, the slot is initialised first. For each of `n`: `Team::NewEntity` → `SetType` → position `FindValidSpawnPosition(pos, spread)` (up to 10 tries of `r = syncfrand(spread)`, `θ = syncfrand(2π)`, accepting on-map points with height > 0, else `pos` clamped to [20, size − 20]) → `m_onGround = false` → velocity, front (normalised velocity; else (1,0,0), Darwinians a random horizontal front from two synced `SFRAND(1)`) → id → `m_spawnPoint = pos` → `m_roamRange` (`range`, or `spread` when `range == −1`) → route ids → `Begin()` → `EntityGrid::AddObject(id, x, z, m_radius)`. Then `unit->RecalculateOffsets()` if spawned into a unit. The population-cap check is commented out (caps are enforced by spawning buildings, [05](05_economy_and_population.md)) | [C] `code/location.cpp:Location::SpawnEntities`, `FindValidSpawnPosition` |
| Units | `Team::NewUnit(troopType, n, &unitId, pos)`: slot `m_units.GetNextFree()`; class by type (`TypeInsertionSquadie` → `InsertionSquad`, `TypeSpaceInvader` → `AirstrikeUnit`, `TypeVirii` → `ViriiUnit`, else `Unit`); `PutData(unit, slot)`; `Begin()` | [C] `code/team.cpp:Team::NewUnit` |
| Spirits | `Location::SpawnSpirit(pos, vel, team, ownerId)`: refused (−1) for the monster team; slot `GetNextFree()`; `pos + up`, `vel`, team, owner id; `Begin()` | [C] `Location::SpawnSpirit` |
| Effects | `new` + `m_effects.PutData` + id (§3.4); the creating helper usually also creates a `MuzzleFlash` effect (`ThrowWeapon`, `FireRocket`, `FireTurretShell`, `FireLaser`) | [C] `code/location.cpp` |
| Buildings | §4.3; level buildings in `InitBuildings` (owner position → team; `Initialise(template)`, `SetDetail(1)`); runtime creators then `CalculateAll()` | [C] `Location::InitBuildings` |
| Creation inside a tick | allowed from any `Advance`; visibility to the rest of the same tick follows §5.4 | [C]/[I] |
| Creation outside the tick | must not happen; the known exceptions are listed in §10.4 (tutorial director, local preview routes) | [C] |

### 5.8 Removal rules

| Kind | Who removes | Side effects | Ref |
|---|---|---|---|
| Entity | container owner after `Advance` returned true (or the entity was `m_destroyed`) | entity-grid removal at the pre-Advance position with the current radius; `delete` (dtor: `s_entityPopulation−−`; subclass dtors unregister specials) | [C] `Team::Advance`, `Unit::RemoveEntity` |
| Dead entity | stays in its container and the grid while fading (`m_dead`), still visible to grid queries; most queries skip `m_dead` explicitly (e.g. `GetBestEnemy`) | — | [C] `EntityGrid::GetBestEnemy` |
| Whole team | `Team::Eliminate()` (game rules) calls `ChangeHealth(−2 × StatHealth)` on every member and loose entity, stops all tasks and neutralises owned trunk ports; removal then follows the normal death path | [C] `code/team.cpp:Team::Eliminate` |
| Unit | `Team::Advance` slice 0 when `Unit::Advance` reports no used slot | `~Unit` clears the team's selection if it pointed here | [C] |
| Building | `AdvanceBuildings` | `RecalculateAITargets` (AITarget, Wall); `ObstructionGrid::CalculateAll` once per slice; routing node tables untouched | [C] |
| Effect / laser / spirit | `AdvanceWeapons` / `AdvanceSpirits` | none | [C] |
| Route | only `RoutingSystem::DeleteRoute`; simulation routes are never deleted (§7.6) | — | [C] |
| Everything | `Location::Empty()` (and `~Location`): delete water, landscape, lights, buildings, effects; empty spirits, lasers; delete `Unit::s_offsets`; delete all teams; delete level file, entity grid, cache, obstruction grid, clouds; reset AIObjective/Armour/Engineer statics; `~Location` also zeroes `Entity::s_entityPopulation` | — | [C] `Location::Empty`, `Location::~Location` |

### 5.9 Level-load order (`Location::Init(mission, map)`)

Exact order [C] `code/location.cpp:Location::Init` (runs on the loading worker thread for a normal start, [20 §9.3](20_architecture.md#93-entering-a-match-enterlocation--doenterlocation)):

1. `LoadLevel(mission, map)` → `m_levelFile = new LevelFile(mission, map, ext(map) == "txt")`; if `MissionComplete()` already, mark the global location complete (false for Multiwinia's automatic objective).
2. `AppSeedRandom(1)` — reseeds the **non-synchronised** generator ([22](22_determinism_and_networking.md)).
3. Reset statics: `GlobalWorld::s_nextUniqueBuildingId = 0`; `Crate::s_crates = 0`; `Crate::s_spaceShipUsed = false`; `AIObjective::s_objectivesInitialised = false`; `Armour::s_armourObjectives.Empty()`; `Engineer::s_buildingIds` / `s_researchIds.EmptyAndDelete()`; `SpawnPopulationLock::Reset()`; `SoulDestroyer::s_numSoulDestroyers = 0`; `DustBall::s_vortexTimer = 0`, `s_vortexPos = 0`; delete `Unit::s_offsets`; for each team i: `AI::s_ai[i] = NULL`, `SpawnPoint::s_numSpawnPoints[i] = 0`, `GetTeamPosition(i)` (assigns start positions — consumes `syncrand`), `PulseBomb::s_pulseBombs[i] = −1`, `MultiwiniaZone::s_blitzkriegBaseZone[i] = −1`.
4. Christmas: `m_clientToServer->m_christmasMode` → `m_christmasMode`.
5. `InitLights()`, `InitLandscape()` ([23](23_terrain_and_landscape.md)), `m_water = new Water()`.
6. Team colours / alliances (§9.5): Assault → `SetupAssault()`; else co-op → `SetupCoop(i)` per player position not yet grouped; else every CPU team gets `Multiwinia::GetCPUColour()` (tutorial: colour = team index).
7. Not editing: `InitBuildings()` → `RecalculateAITargets()` (**before** the obstruction grid exists, so AI-target links ignore walls — [07 §4](07_ai.md#4-the-aitarget-node-graph--the-ais-map)) → `m_entityGrid = new EntityGrid(32, 32)` → `m_entityGridCache` → `m_obstructionGrid(32 or 64)` (its constructor runs `CalculateAll`) → `m_clouds` → `m_routingSystem.Initialise()` → `m_seaRoutingSystem.Initialise()` (both **after** the obstruction grid, so routing edges see walls). Editing [B:EDITOR]: only snap level buildings to the terrain.
8. `InitTeams()`: for i in 0..5: a lobby colour of −1 → `GetNextAvailableColour()`; `m_teams[i] = new Team(lobbyTeam)`; type ≠ Unused → `InitialiseTeam(i)`; `m_teamsInitialised = true`.
9. Fewer than 4 players: each team slot i ≥ numPlayers (except 4 and 5) gets a random unused colour `(syncrand() % 8) / 2` (0–3), colour only.
10. Difficulty: level difficulty −1 → store `g_app->m_difficultyLevel` (0 in Multiwinia) in it; else copy it into `m_difficultyLevel`.
11. `m_allowMonsterTeam = true`; Multiplayer: `InitMonsterTeam(4)`, `InitFuturewinianTeam(5)`; fixed research levels (every item 3, then Squad 1, Laser 2, Grenade 2, Rocket 2, AirStrike 2, Darwinian 4).

`InitBuildings()` and `InitialiseTeam(team)` (AI entity, starting power-ups, one Darwinian per port of owned spawn points and solar panels, instant units, terrain snap) are specified in [03 §3.4](03_buildings_and_world_objects.md#34-lifecycle) and [05 §6.7](05_economy_and_population.md#67-starting-units) [C] `code/location.cpp:Location::InitBuildings`, `Location::InitialiseTeam`.

### 5.10 Weather and ambient spawners (slice 3, dt = `SERVER_ADVANCE_PERIOD`)

| Function | Behaviour [C] `code/location.cpp` |
|---|---|
| `AdvanceChristmas` | if `m_christmasTimer < −90`: 150 `Snow` effects at (FRAND(sizeX), syncfrand(1000), FRAND(sizeZ)). Timer −= dt; when ≤ 0: timer = 0.1 s and one `Snow` at y = 700 + syncfrand(300). Ids (255, `UNIT_EFFECTS`, slot, generated uid) |
| `AdvanceSoulRain` | `m_spawnSync −= dt`; when ≤ 0: 1.0 s; one `UnprocessedSpirit` at (FRAND(sizeX), 700 + syncfrand(300), FRAND(sizeZ)) into `m_effects` **without an id**. No shipped `mp_` map sets `soulrain` [C] scan of `data/levels/mp_*.txt` `Effects` lines |
| `AdvanceDustWind` | `DustBall::s_vortexTimer −= dt`; when ≤ 0: timer = 15 + syncfrand(15); if `syncrand() % 10 == 0` and no vortex: vortex at a random point (FRAND(sizeX), FRAND(500), FRAND(sizeZ)) re-rolled while terrain ≤ 1.0; else clear the vortex and set `m_windDirection = normalise(random − random)`. First run (`m_christmasTimer < −90`): 300 `DustBall`s at y = syncfrand(1000), size += syncfrand(20). Shared timer −= dt; when ≤ 0: two dust balls (timer 0.1). Active vortex: every grid neighbour within 50 of the vortex ground point that is an on-ground Darwinian → `StateInVortex`, airborne |
| `AdvanceLightning` | shared timer −= dt; when ≤ 0: timer = syncfrand(15); one `Lightning` effect at y = 1200 over a random land point (height > 1.0, re-rolled) |

`AdvanceChristmas`, `AdvanceDustWind` and `AdvanceLightning` share `m_christmasTimer`, so a level with several of these effects (or Christmas mode plus dust) makes them interfere; in Christmas mode on a snow level `AdvanceChristmas` runs twice per tick [C]. Crate drops (`AdvanceCrates`, `CreateRandomCrate`, slice 8): [04 §1.3–1.4](04_crates_powerups_and_weapons.md#13-drop-timer--locationadvancecrates).

### 5.11 Location query and service API

[C] `code/location.cpp`. Functions specified elsewhere are linked.

| Function | Semantics |
|---|---|
| `GetWorldObject(id)`, `GetEntity(id)`, `GetEntitySafe(id, type)`, `GetUnit(id)`, `GetEffect(id)`, `GetBuilding(uid)`, `GetSpirit(index)`, `GetSpirit(ownerId)` | §3.2 |
| `GetEntity(rayStart, dir)` / `GetBuilding(rayStart, dir)` | first entity (teams 0–5, `Team::RayHitEntity`) / building (`DoesRayHit`) hit; `GetBuilding(ray)` returns `false` cast to NULL when nothing is hit |
| `GetBuildingId(rayStart, dir, team, maxDist, *range)` | buildings of `team` or 255 except ControlTower; RadarDish/GunTurret use exact `DoesRayHit` and win immediately (range 0); others ray–sphere at 0.75 × radius; best = smallest screen-space distance between building centre and hit point (local picking) |
| `GetUnitId(rayStart, dir, team, *range)` | ray vs unit sphere (1.5 × radius) then per-member sphere (1.5 × radius), screen-distance best (local picking) |
| `GetEntityId(rayStart, dir, team, *range, ignoreDarwinians)` | over `m_others`: Officer → `CalculateBoundingSphere` (camera-scaled, local only); Armour → `RayHit` (wins with range 0.001); others sphere 1.0 × radius (local picking) |
| `IsWalkable`, `GetWalkingDistance` | §7.2 |
| `IsVisible(from, to)` | landscape ray from `from + 20·dir`; visible if no hit or the hit lies beyond `|to − from| + 20` |
| `SpawnEntities`, `FindValidSpawnPosition`, `SpawnSpirit` | §5.7 |
| `ThrowWeapon`, `FireRocket`, `FireTurretShell`, `LaunchFireball`, `FireLaser`, `CreateShockwave`, `Bang` | [04 §6.0–6.1](04_crates_powerups_and_weapons.md#60-conventions-class-inventory-and-fire-helpers) |
| `SelectDarwinians`, `IssueDarwinianOrders`, `PromoteDarwinian`, `DemoteOfficer`, `UpdateTeam` | command application ([06](06_player_control_and_input.md)) |
| `IsFriend`, `GetMonsterTeamId`, `GetFuturewinianTeamId`, `InitMonsterTeam`, `InitFuturewinianTeam`, `GetTeamPosition`, `GetTeamFromPosition`, `RotatePositions`, `IsAttacking`, `IsDefending`, `SetupCoop`, `SetupAssault`, `SetCoopTeam`, `SetSoloTeam`, `GetGroupColour`, `SetGroupColourId` | §9 |
| `RecalculateAITargets()` | every live AITarget recomputes its neighbours, then links culled ([07 §4.3](07_ai.md#43-link-culling--locationrecalculateaitargets-c)) |
| `ViablePosition(pos)` | true if `IsWalkable(pos, aiTarget, false, true)` to any AITarget |
| `GetMyTeam()` / `GetMyTaskManager()` | via `GlobalWorld::m_myTeamId` (255 → NULL) — local |
| `DumpWorldToSyncLog(file)` | appends `LogState()` of all unit members, loose entities, effects (except snow) and buildings ([22](22_determinism_and_networking.md)) |
| `Render`, `RenderChatMessages`, `SetupFog`, `SetupLights`, `SetupSpecialLights`, `FlushOpenGlState`, `RegenerateOpenGlState`, `SetCurrentMessage`, `NewChatMessage`, `AddBurnPatch` | presentation ([24](24_rendering.md)) |
| `MissionComplete`, `DoMissionCompleteActions` | [B:LEGACY] |
| `GetLastProcessedSlice` / `SetLastProcessedSlice` | accessors |
