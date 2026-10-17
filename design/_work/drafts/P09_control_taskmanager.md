# P09_control_taskmanager — Task manager, unit-control UI, gestures, game cursor
Scope: `code/taskmanager.{h,cpp}`, `code/taskmanager_interface.{h,cpp}`, `code/taskmanager_interface_gestures.{h,cpp}`, `code/taskmanager_interface_icons.{h,cpp}`, `code/taskmanager_interface_multiwinia.{h,cpp}`, `code/gesture.{h,cpp}`, `code/gesture_demo.{h,cpp}`, `code/gesture_demo_win32.cpp`, `code/gamecursor.{h,cpp}`; content read: `data/gestures.txt`, `data/gesture_demos/*.txt`. Build baseline: see build_matrix.md (`Release|Win32` + `TARGET_PC_FULLGAME`).

Conventions in this draft: "MP" = `App::Multiplayer()` ≡ `m_gameMode == GameModeMultiwinia` (every Multiwinia match, including the tutorial); "SP" = `App::IsSinglePlayer()` ≡ `m_gameMode ∈ {GameModeCampaign, GameModePrologue}` [C] `code/app.cpp:App::IsSinglePlayer`, `App::Multiplayer`. Because the Prologue/Campaign menu entries are not compiled in FULLGAME (build_matrix §5), **every SP-only branch below is [B:LEGACY]**. "Tick" = one server advance of `SERVER_ADVANCE_PERIOD` = `0.1 * gameSpeed` s [C] `code/gametimer.cpp:GameTimer::GetServerAdvancePeriod`.

---

## [→ 06_player_control_and_input] 1. TaskManager — the Task ("program") model

### 1.1 Overview

Each `Team` owns one `TaskManager` (`new TaskManager(m_teamId)` [C] `code/team.cpp:Team::Team`). A **Task** (called a "program" on the wire and in UI strings — Darwinia's fiction) is a slot-limited, player-owned power: a spawned unit (squad, engineer, armour, harvester, officer-promotion, shaman, tank) or a targetable one-shot/duration power-up (air strike, nuke, booster, …). In Multiwinia tasks are **never** created by the player directly; they are granted by the simulation (crates, reinforcements, retribution, starting power-ups, instant units, AI). The player only **selects**, **targets (places)** and **terminates** them. All three player actions travel as network commands and are executed identically on every peer in lockstep (§1.9).

Lifetime in one sentence: *created (`RunTask`) → `StateStarted` (awaiting a target click, icon pulses) → `Target(pos)` spawns/fires the effect → `StateRunning` (tracked object alive / timer running) → removed when `Advance` returns true or `TerminateTask` is called; one-shot powers terminate themselves immediately inside `Target`.*

### 1.2 Data model

`Task` fields [C] `code/taskmanager.h:Task`, initial values [C] `code/taskmanager.cpp:Task::Task`:

| Field | Type | Init | Meaning |
|---|---|---|---|
| `m_teamId` | int | ctor arg | Owning team |
| `m_id` | int | -1 | Assigned by `TaskManager::RunTask/RegisterTask` from `m_nextTaskId++` (monotonic per team, never reused) |
| `m_type` | int | `GlobalResearch::TypeSquad` | Task type = `GlobalResearch` enum value (§1.3) |
| `m_state` | int | `StateIdle` | §1.4 |
| `m_objId` | WorldObjectId | invalid | The spawned unit/entity/building/effect being tracked |
| `m_pos` | Vector3 | (0,0,0) | Click position (air strike, turret) or running centroid of affected Darwinians (booster family); used by `GetPosition()` |
| `m_route` | Route* | NULL | Only for the (disabled) Controller task; deleted in dtor |
| `m_lifeTimer` | float (s) | 0 | Remaining life for time-limited tasks; `-1` = infinite (instant units) |
| `m_startTimer` | float (s) | 0 | Lock-out: while > 0 the task cannot be targeted or selected (UI greys it and shows the countdown) |
| `m_option` | int | -1 | Sub-variant: air strike `1` = napalm; gun turret = `GunTurret::GunTurretType*` (0 Standard, 1 Rocket, 2 Mortar, 3 Flame, 4 Subversion); reinforcement tasks get `LevelOptionDefend/AttackReinforcementMode` |
| `m_screenX`, `m_screenY`, `m_screenH` | float | 0 | **UI-only**: last on-screen position/size of this task's icon in the Multiwinia task bar (written by the renderer, read by the "recent task" fly-in animation) |

`TaskSource` [C] `code/taskmanager.h:TaskSource` — UI record for the "power-up received" pop-up: `m_taskId` (-1), `m_timer` (double, wall clock; positive = showing, negative = minimising since `-m_timer`), `m_source` (0 = crate, 1 = reinforcement, 2 = retribution), `m_rotation` (float, crate-icon spin angle).

`TaskTargetArea` [C] `code/taskmanager.h:TaskTargetArea`: `m_centre` (Vector3), `m_radius` (float), `m_stationary` (bool; true for building-anchored areas).

`TaskManager` fields [C] `code/taskmanager.h:TaskManager`, init [C] `code/taskmanager.cpp:TaskManager::TaskManager`:

| Field | Init | Meaning |
|---|---|---|
| `m_tasks` (LList<Task*>) | empty | Ordered slot list; **order matters** (UI slot index, keyboard 1–9) |
| `m_teamId` | ctor | |
| `m_nextTaskId` | 0 | Next id |
| `m_currentTaskId` | -1 | Currently selected task (the one the cursor will place) |
| `m_verifyTargetting` | true | If false, `IsValidTargetArea` accepts any in-landscape point (instant units; cheat window) |
| `m_mostRecentTaskId` | -1 | Id of last task created (read by sim code right after `RunTask`, and by help UI) |
| `m_mostRecentTaskIdTimer` | 0.0 | Wall-clock time of creation (UI) |
| `m_blockMostRecentPopup` | false | Set by reinforcement code; **no reader** (the guarding `if` in `RunTask` is commented out) [C] |
| `m_recentTasks` (DArray<TaskSource*>) | empty | Pop-up queue |
| `m_previousTaskId` | -1 | Last task explicitly selected (Tab-cycling anchor when nothing selected) |
| `m_mostRecentEntity` | invalid | Id of the most recently promoted Officer (read by AI) |

`#define POWERUP_EFFECT_RANGE 70.0` (world units) [C] `code/taskmanager.h`.

### 1.3 Task types — complete enumeration

Task type ids are the `GlobalResearch` enum [C] `code/global_world.h:GlobalResearch`; names from `GlobalResearch::GetTypeName` [C] `code/global_world.cpp` (used for icon file names `icons/icon_<Name>.bmp`, `icons/gesture_<Name>.bmp`, string ids `researchname_<Name>`, `crate_tooltip_<name>`, `newcontrols_select_<Name>`, `newcontrols_place_<Name>`, `newcontrols_delete_<Name>`).

| Id | Enum | Name string | Created by `RunTask(int)`? | `Start()` effect | `Target(pos)` effect | Terminates when (`Advance`) | `Stop()` (on terminate) | MP source |
|---|---|---|---|---|---|---|---|---|
| 0 | TypeDarwinian | Darwinian | no (returns false) | – | – | – | – | – (research level only) |
| 1 | TypeOfficer | Officer | yes | – | Promote nearest own Darwinian within 10 wu to Officer, then **terminate own task** (§1.6) | never (state forced back to Started each tick) | – | AI only (`ai.cpp` RunTask Officer); player promotion is a direct order, see §2.6 |
| 2 | TypeSquad | Squad | yes | `m_lifeTimer = 60` | Spawn squad unit of `2 + CurrentLevel(TypeSquad)` InsertionSquadies (MP level 1 → **3**) at pos, spread 10; select it; optional camera track | MP: life timer reaches 0 → `Stop()`; any: unit gone or `NumAliveEntities()==0` | all squaddies `ChangeHealth(-1000)` | Crate "Squad" |
| 3 | TypeLaser | Laser | no | – | – | – | – | – (squad weapon research) |
| 4 | TypeGrenade | Grenade | special: sets current squad's weapon | – | – | – | – | – |
| 5 | TypeRocket | Rocket | special: sets current squad's weapon | – | – | – | – | – |
| 6 | TypeController | Controller | special: sets current squad's weapon (Controller-task creation is commented out) | – | – | unit gone/empty (shares Squad branch) | – | – |
| 7 | TypeAirStrike | AirStrike | MP: normal task. SP: squad weapon (falls through to Grenade case) | – | `Crate::RunAirStrike(pos, team, m_option==1)` → `m_objId` = effect id; `m_pos=pos`; **terminate own task (no message)** | (effect gone) | – | Crate "Airstrike", "Napalm Strike" (`m_option=1`); Blitzkrieg reinforcement; retribution |
| 8 | TypeArmour | Armour | yes (`#ifndef DEMOBUILD`) | – | Spawn Armour at pos; select it | entity gone/dead | `ChangeHealth(-1000)`; **refused during Multiwinia tutorial** (`stop=false`) | Crate "Armour"; trunk-port reinforcement (Rocket Riot/Blitzkrieg) |
| 9 | TypeTaskManager | TaskManager | no | – | – | – | – | – (capacity research, SP only) |
| 10 | TypeEngineer | Engineer | yes | – | Spawn Engineer at pos + (0,10,0); select it | entity gone/dead | `ChangeHealth(-1000)` | Crate "Engineer" |
| 11 | TypeHarvester | Harvester | yes | – | Spawn Harvester at pos + (0,50,0); select it | entity gone/dead | set `m_vunerable=true`, `ChangeHealth(-1000)` | Crate "Harvester" |
| 12 | TypeNuke | Nuke | yes | – | `Crate::RunNuke(pos, team)`; terminate own task (no message) | (entity gone) | – | Crate "Nuke" |
| 13 | TypeSubversion | Subversion | yes | `m_lifeTimer = 30` | `Crate::RunSubversion(pos, team, m_id)`; Running; `SwitchTo()` | life timer ≤ 0, or no Darwinian of team still tagged `m_subversionTaskId==m_id` | – | Crate "Subversion" |
| 14 | TypeBooster | PersonalShield | yes | `m_lifeTimer = 30` | `Crate::RunBooster(...)`; Running; `SwitchTo()` | as Subversion (`m_boosterTaskId`) | – | Crate "Booster" (personal shield) |
| 15 | TypeHotFeet | HotFeet | yes | `m_lifeTimer = 30` | `Crate::RunHotFeet(...)`; Running; `SwitchTo()` | as Subversion (`m_hotFeetTaskId`) | – | Crate "Hot Feet" |
| 16 | TypeGunTurret | GunTurret | yes | – | `Crate::RunGunTurret(pos, team, m_option or Standard)`; `m_pos=pos`; if game option `MaxTurrets == -1` terminate own task, else stay Running and clear `m_currentTaskId` | building gone → done; building captured by another real team (team ≠ own and ≠ 255) → `Damage(-1000)` and done | if building still own: `Damage(-1000)`; else **refuse** (`stop=false`) | Crates "Gun Turret"/"Rocket Turret"/"Flame Turret"; reinforcement turret (Rocket Riot/Blitzkrieg) |
| 17 | TypeShaman | Shaman | yes | – | Spawn Shaman at pos; select it | (no branch — never auto-ends) | – | Instant units (`m_runAsTask`); **cannot be terminated** |
| 18 | TypeInfection | Infection | yes | – | `Crate::RunInfection`; terminate own | – | – | Crate "Spam"(Infection) |
| 19 | TypeMagicalForest | MagicalForest | yes | – | `Crate::RunMagicalForest`; terminate own | – | – | `Crate::CaptureCrate(id,team)` path (retribution "Forest") |
| 20 | TypeDarkForest | DarkForest | yes | – | `Crate::RunDarkForest`; terminate own | – | – | Crate "Dark Forest"; retribution |
| 21 | TypeAntsNest | AntNest | yes | – | `Crate::RunAntNest`; terminate own | – | – | Crate "Ant Nest" |
| 22 | TypePlague | Plague | yes | – | `Crate::RunPlague`; terminate own | – | – | none (crate mapping commented out) |
| 23 | TypeEggSpawn | Egg | yes | – | `Crate::RunEggs`; terminate own | – | – | Crate "Eggs"; retribution |
| 24 | TypeMeteorShower | MeteorShower | yes | – | `Crate::RunMeteorShower`; terminate own | – | – | Crate "Meteor"; retribution |
| 25 | TypeExtinguisher | FireExtinguisher | yes | – | **body commented out — no effect, task stays Started forever** | – | – | none (crate mapping commented out) |
| 26 | TypeRage | Rage | yes | `m_lifeTimer = 30` | `Crate::RunRage(...)`; Running; `SwitchTo()` | as Subversion (`m_rageTaskId`) | – | Crate "Rage" |
| 27 | TypeTank | Tank | yes | `m_lifeTimer = 30` unless game type TankBattle | **body commented out** | MP non-TankBattle: life timer → `Stop()`; entity gone/dead | `ChangeHealth(-1000)` | none in FULLGAME (TankBattle compiled out; crate mapping commented) |
| 28 | TypeDropShip | DropShip | no (not in `RunTask(int)` switch) | – | Spawn DropShip at the map corner nearest pos (corners inset 100 wu, height 500 wu), `SetWaypoint(pos)` | entity gone/dead | – | none found [I] (only reachable via `RunTask(Task*)`) |

Evidence: [C] `code/taskmanager.cpp:Task::Start`, `Task::Target` and every `Task::Target*`, `Task::Advance`, `Task::Stop`, `TaskManager::RunTask(int)`; crate mapping [C] `code/worldobject/crate.cpp:Crate::CaptureCrate()`, `Crate::CaptureCrate(int,int)` (P08 owns crate semantics); reinforcement [C] `code/multiwinia.cpp` reinforcement block and `Multiwinia::AdvanceRetribution`; shaman/instant units [C] `code/location.cpp:Location::InitialiseTeam`.

Notes:
- `TypeController`, `TypeGrenade`, `TypeRocket` (and `TypeAirStrike` in SP) are not slot tasks: `RunTask(type)` looks up the *current* task; if it is a Squad whose unit is an `InsertionSquad`, it calls `squad->SetWeaponType(type)` and shows `MessageSuccess`; otherwise `MessageFailure` and returns false [C] `TaskManager::RunTask(int)`. This is a squad **weapon selection**, not a new task.
- Unknown types return false [C].
- `MapGestureToTask(gestureId)`: 0→Squad, 1→Engineer, 2→Grenade, 3→Rocket, 4→AirStrike, 5→Controller, 6→Officer, 7→Armour, else -1 [C] `TaskManager::MapGestureToTask`. Gesture ids are the order of symbols in `data/gestures.txt` (§3.6).

### 1.4 Task states

`enum { StateIdle, StateStarted, StateRunning, StateStopping }` [C] `code/taskmanager.h:Task`.

```mermaid
stateDiagram-v2
    [*] --> Idle : new Task
    Idle --> Started : TaskManager::RunTask → Task::Start()
    Started --> Running : Target(pos) succeeds (spawn/effect)\n[blocked while m_startTimer > 0]
    Started --> [*] : Target() of one-shot types calls TerminateTask(self)\n(AirStrike, Nuke, Infection, MagicalForest, DarkForest,\nAntNest, Plague, Eggs, MeteorShower, GunTurret if MaxTurrets==-1,\nOfficer after promotion)
    Running --> Started : Officer only (Advance resets every tick)
    Running --> Stopping : Stop() (life timer expiry for Squad/Tank)
    Running --> [*] : Advance() returns true (object dead/gone, timer expired,\nno affected Darwinians left, turret lost)
    Stopping --> [*] : Advance() returns true (m_state == StateStopping)
    Started --> [*] : TerminateTask (player "delete") — Stop() then delete
    Running --> [*] : TerminateTask — Stop() then delete
```

- `Target()` is ignored unless `m_state == StateStarted` and `m_startTimer <= 0` [C] `Task::Target`.
- `Advance()` return value = "remove me"; it returns `m_state == StateStopping` at the end, after decrementing `m_startTimer` by one tick if > 0 [C] `Task::Advance`. `m_startTimer` therefore counts down in **all** states.
- When a Squad/Controller/Engineer/Armour/Harvester/Tank task dies while it is the current task and belongs to the local player, `MessageShutdown` is shown for 3.0 s [C].

### 1.5 Slots / capacity

`Capacity()` [C] `code/taskmanager.cpp:TaskManager::Capacity`:
- SP: `1 + research level(TypeTaskManager)` [B:LEGACY].
- MP: **6**, or **2** if `Multiwinia::IsEliminated(team)`.

`CapacityUsed()` = `m_tasks.Size()` — **every** task occupies a slot, including Started (unplaced) ones and running turrets [C].

`RunTask(Task*)` [C] `TaskManager::RunTask(Task*)`:
1. If `CapacityUsed() >= Capacity()`: local player in MP gets `dialog_powerup_limit` location message; in SP `MessageFailure` 2.5 s; the task is deleted; return false.
2. Else assign `m_id = m_nextTaskId++`.
3. **Insertion order**: MP — insert before the first task whose state is `StateStarted` (so placed/running tasks stay left of unplaced ones); if none, append. SP — append.
4. `Start()`.
5. SP only: auto-select it (`m_currentTaskId = id`). **MP never auto-selects a newly granted task** [C].
6. `m_mostRecentTaskId = id`, `m_mostRecentTaskIdTimer = GetHighResTime()`, push `TaskSource{id, GetHighResTime(), source 0}` into `m_recentTasks`.
7. MP and now full and local team → `dialog_powerup_limit` message.

`RegisterTask(Task*)`: capacity-checked append (at the end, no MP re-ordering) without `Start()` [C] `TaskManager::RegisterTask`. Users:
- **[B:FULL] Radar-dish squad migration**: when the first squaddie of an `InsertionSquad` enters a RadarDish/Teleport and a new unit is created at the destination, the old Squad task is removed (no `Stop()`), a new `Task{type=Squad, state=Running, objId=new unit}` is registered with the old `m_lifeTimer` carried over in MP, and — if registration succeeds — `SelectTask(newId)` is called (in the sim, on all peers). The new task gets a **new id** and moves to the **end of the slot list**; weapon type and controller id are copied to the new unit [C] `code/worldobject/teleport.cpp:Teleport::EnterTeleport`.
- [B:LEGACY] restoring saved Darwinia running programs (Engineer, InsertionSquad) [C] `code/location.cpp:Location::InitialiseTeam` (running-program block).

`TaskManager::Advance()` (sim; once per tick from `Team::Advance` slice 1) [C] `TaskManager::Advance`, `code/team.cpp:Team::Advance`:
1. MP: one bubble pass from the end: if task *i* is Running and *i-1* is Started, swap them (Running tasks drift left one position per tick).
2. `AdvanceTasks()`: if the current task is Started and the team is local, refresh `MessageSuccess(type)` for 2.5 s (keeps the "place your X" caption alive); then for each task call `Advance()`; if it returns true: if it was current → `m_currentTaskId=-1` and `Team::SelectUnit(-1,-1,-1)`; remove and delete.
3. `GlobalResearch::AdvanceResearch()` (note: called once per team per tick).
4. SP: clear `m_mostRecentTaskId` 1.0 s after creation. MP: after 3.0 s flip `m_mostRecentTaskIdTimer` negative (`-now`) = "minimising".
5. MP: for each `TaskSource`: after 3.0 s flip `m_timer` to `-now`; 1.0 s after that remove it.

`StopAllTasks()` empties/deletes all tasks without `Stop()`, `m_currentTaskId=-1` (team elimination) [C] `code/team.cpp` elimination block.

### 1.6 Targeting rules (`IsValidTargetArea`, `GetTargetArea`, `TargetTask`)

`TargetTask(id,pos)` [C] `TaskManager::TargetTask`: if `IsValidTargetArea(id,pos)` → `task->Target(pos)`; re-lookup the task (it may have deleted itself); if it still exists with a valid `m_objId` and type ≠ GunTurret → `MarkerSystem::RegisterMarker_Task(objId)` (on-screen off-screen marker, P08). Returns validity.

`IsValidTargetArea(id,pos)` [C] `TaskManager::IsValidTargetArea`:
1. False if task missing or `pos` outside the landscape.
2. If `m_verifyTargetting == false` → true.
3. Officer: true iff `EntityGrid::GetFriends(pos, r=10.0)` contains a Darwinian of the team.
4. GunTurret: false if `RestrictionZone::IsRestricted(pos, true)`; false if any building's `DoesSphereHit(pos, 10.0)`; false if within **400 wu** of a non-friendly owned (team ≠ 255) TrunkPort or within **200 wu** of a non-friendly owned SpawnPoint; else true.
5. Booster/HotFeet/Subversion/Rage: true iff at least one entity of the own team is within `POWERUP_EFFECT_RANGE` = 70 wu (`EntityGrid::GetNumNeighbours`).
6. Otherwise: true iff `pos` lies within any area from `GetTargetArea` (distance ≤ radius); for the "anywhere" area (centre (0,0,0)) a `RestrictionZone::IsRestricted(pos,true)` point is skipped.

`GetTargetArea(id)` [C] `TaskManager::GetTargetArea` (note the deliberate fall-throughs):

| Task type | Areas produced (MP) | Areas (SP, [B:LEGACY]) |
|---|---|---|
| Engineer, Harvester | circle r=**100** around each own unit whose troop type is InsertionSquadie or Engineer (non-stationary), **then falls through to Armour/Tank → (MP) falls through to Squad** | + own ControlTower r=75; + open TrunkPorts r=120 (break) |
| Armour, Tank | (MP) falls through to Squad list | open TrunkPorts (`m_openTimer>0`) r=120 |
| Squad | r=**75** (stationary) around every friendly (`IsFriend`) SpawnPoint, GunTurret, MultiwiniaZone, TrunkPort | friendly ControlTowers r=75 |
| Officer, AirStrike, Nuke, Subversion, Booster, HotFeet, GunTurret, Shaman, Infection, MagicalForest, DarkForest, AntsNest, Plague, EggSpawn, MeteorShower, Extinguisher, Rage, DropShip | one area centre (0,0,0), r=99999.9 ("anywhere") | same |

(For Officer/GunTurret/booster-family the special checks in `IsValidTargetArea` pre-empt the area list.)

`Promote(id)` [C] `Task::Promote`: spawn an Officer at the Darwinian's position with its velocity; create `5 + AppRandom()%5` `Particle::TypeControlFlash` particles with velocity (sfrand(5), frand(15), sfrand(5)); set `darwinian->m_promoted = true` (the Darwinian removes itself — P03). `Demote(id)` (static; unused in this scope) spawns a team-0 Darwinian at an entity's position with the same flash.

`TargetOfficer(pos)` [C] `Task::TargetOfficer`: refused if the team currently has a unit, entity or building selected; else `FindDarwinian(pos, team)` = nearest (squared distance) Darwinian of the team (MP: exactly the team; SP: any friendly team) within 10 wu; promote; `TerminateTask(self)`; select the new officer; `m_mostRecentEntity = officer id`; local: `MessageSuccess(Officer)` 2.5 s + sound `GestureSuccess`.

Spawn side effects for local team: Squad → if pref `OTHER_AUTOMATICCAM` (`"AutomaticCamera"`, P17b) is 2, or 0 while in gamepad mode, request camera entity-track of the squad [C] `Task::TargetSquad`. Sounds: MP Squad/Armour/Harvester use `Crate::TriggerSoundEvent(CrateReward*, pos, team)`; others `GestureSuccess` (TypeGesture).

### 1.7 Selection (`SelectTask`, `SwitchTo`, cycling)

- `SelectTask(int id)` [C]: `m_currentTaskId = id`; if ≠ -1: `m_previousTaskId = id`; find task (return silently if missing); `task->SwitchTo()`; `GameCursor::BoostSelectionArrows(2.0)` (UI-only effect called from sim — see §6 D5).
- `SelectTask(WorldObjectId)`: selects the task whose `m_objId` matches team/unit/index; else `m_currentTaskId=-1`. Called by the network `SelectUnit` command so that clicking a unit in the world also selects its task [C] `code/network/clienttoserver.cpp` `NET_DARWINIA_SELECTUNIT` handler.
- `SwitchTo()` [C] `Task::SwitchTo`: if local team and camera in RadarAim/TurretAim → camera FreeMovement. Then by type: Squad → `Team::SelectUnit(unitId,-1,-1)`; Engineer/Armour/Harvester/Shaman/Tank → `SelectUnit(-1, entityIndex, -1)`; Controller → select the Squad task with the same `m_objId`; Officer → deselect all; HotFeet/Booster/Subversion/Rage → for non-CPU teams set `m_underPlayerControl = true` on every Darwinian tagged with this task id, then deselect all (this is how the power-up's affected Darwinians become a directly controlled group); default → deselect all.
- `GetNextTaskId()` returns the id after the current one (wrapping; -1 current ⇒ first) [C]; used by the legacy interfaces.
- `TaskManagerInterface::AdvanceTab()` (all variants, client side) [C] `code/taskmanager_interface.cpp:TaskManagerInterface::AdvanceTab` — runs when the TM overlay is hidden or the input mode is keyboard:
  - **Cycle right** on `ControlUnitCycleRight` (keys `]`, **Tab**; pad right bumper; SpaceNav left button) or on `ControlGesturesSwitchUnit` (Tab) when pref `ControlMethod == 0`: anchor = current task, or `m_previousTaskId` if none; next index (wrap to 0).
  - **Cycle left** on `ControlUnitCycleLeft` (`[`; pad left bumper): previous index from the current task (wrap to last).
  - If no anchor found, index 0. Plays `MouseOverIcon` (TypeInterface). If the chosen task is a Squad → camera `RequestEntityTrackMode(objId)`, else camera `ModeFreeMovement`. Then SP: `SelectTask` directly; **MP: `ClientToServer::RequestSelectProgram(myTeam, taskId)`**.
- Deselect (`ControlUnitDeselect`, Space / pad B) is handled in `LocationInput` (P01): sends `RequestSelectUnit(-1,-1,-1)`, camera free, `m_previousTaskId = -1` [C] `code/location_input.cpp:LocationInput::Advance`.

### 1.8 Termination (`TerminateTask`)

`TerminateTask(id, showMessage=true)` [C] `TaskManager::TerminateTask`:
1. Not found → false.
2. Shaman → false (cannot be terminated).
3. Squad: `ShutdownController()` first — every Darwinian of the team with `m_controllerId == task id` calls `LoseControl()`; if any were found, show `MessageShutdown(Controller)` (3 s) and **return true without removing the squad** (first press releases controlled Darwinians, second press kills the squad).
4. GunTurret when `MaxTurrets != -1`: if the turret building now belongs to another team → false.
5. Multiwinia tutorial: if tutorial 1, or tutorial 2 before `m_tutorial2AICanSpawn` → false (cannot terminate) [C] (`g_app->m_multiwiniaTutorial`, P19).
6. If `showMessage` and local team → `MessageShutdown(type)` 3.0 s.
7. Remove from list, `Stop()`, delete; clear `m_currentTaskId` if it was current; return true.

Note `Stop()` may itself decline (`stop=false`: Armour in tutorial; turret not owned) but the task object is deleted regardless in step 7 [C] — i.e. the entity survives while its task slot is freed.

### 1.9 Mapping to network commands

All player task actions are sent as `ClientToServer` letters and executed by every peer when the server echoes them in the ordered update stream (P15) [C] `code/network/clienttoserver.cpp:ClientToServer::RequestRunProgram`, `RequestTargetProgram`, `RequestSelectProgram`, `RequestTerminateProgram` and the matching branches of the update processor:

| Player action | Sent by (client) | Command string (`NET_DARWINIA_COMMAND`) | Payload | Executed on every peer |
|---|---|---|---|---|
| Create task (SP/legacy only; MP UI never creates) | Icons/Gestures/Multiwinia `RunScreenZone("NewTask"/"SelectWeapon")`; `LocationInput` quick-unit d-pad (SP) (the squad weapon-cycling sender in `location_input.cpp` is commented out) | `RunProgram` | `TEAMID` (uchar), `PROGRAM` (uchar task type) | `team->m_taskManager->RunTask(program)` |
| Place / fire current task | `LocationInput::Advance` on `ControlUnitCreate` (mouse left down / pad A) while current task is Started, TM overlay not visible, and (MP) not grace period/game over/paused | `TargetProgram` | `TEAMID`, `PROGRAM` (uchar **task id**), `POSITION` (Vector3 = `UserInput::GetMousePos3d()`) | `TaskManager::TargetTask(id,pos)` |
| Select task | `AdvanceTab` (MP), Multiwinia/Icons `RunScreenZone("SelectTask")` | `SelectProgram` | `TEAMID`, `PROGRAM` (uchar task id) | `TaskManager::SelectTask(id)` |
| Terminate current task | Multiwinia `AdvanceTerminate` on `ControlIconsTaskManagerEndTask` (**C** key / pad **Y**) when no Eclipse window open; Icons equivalent | `Terminate` (`NET_DARWINIA_TERMINATE`) | `TEAMID`, `PROGRAM` (uchar task id), `ENTITYID` = -1 | if `PROGRAM != 255`: `TerminateTask(program)`; else kill `team->m_others[ENTITYID]` (`RequestTerminateEntity`) |
| Select unit in world | `LocationInput` (P01) | `SelectUnit` | team, unit, entity, building ids | `Team::SelectUnit(...)` then `TaskManager::SelectTask(WorldObjectId)` |

Wire-format consequences [C] (from the `unsigned char` parameter types):
- Task **ids** are truncated to 8 bits on the wire while `m_nextTaskId` is an unbounded int. A team's 256th and later tasks (id ≥ 256) can no longer be selected/targeted/terminated correctly (the receiver looks up `id mod 256`). [I] Rarely reached in a match (needs ≥ 256 tasks granted to one team); a new design should use ≥ 16-bit ids.
- Terminating with no current task sends `PROGRAM = (uchar)-1 = 255` → interpreted as "terminate entity" with `ENTITYID = -1` → no-op. Task id 255 itself can never be terminated over the network.

```mermaid
sequenceDiagram
    participant Sim as Sim (all peers)
    participant UI as Local client UI
    participant Net as ClientToServer
    Sim->>Sim: Crate captured → TaskManager::RunTask(type)\n(state Started, not selected in MP)
    Sim-->>UI: m_recentTasks pop-up (wall-clock timers)
    UI->>Net: Tab / [ / ] / bumpers / key 1-9 → RequestSelectProgram(team, taskId)
    Net->>Sim: SelectProgram → SelectTask(id) → SwitchTo()
    UI->>Net: Left click (ControlUnitCreate) → RequestTargetProgram(team, currentTaskId, mousePos3d)
    Net->>Sim: TargetProgram → TargetTask → IsValidTargetArea → Task::Target(pos)
    Sim->>Sim: spawn/effect; one-shots terminate themselves
    UI->>Net: C / pad Y → RequestTerminateProgram(team, currentTaskId)
    Net->>Sim: Terminate → TerminateTask(id) → Stop()
```

### 1.10 Edge cases and defects (task core)

| # | Behaviour [C] | Ref |
|---|---|---|
| E1 | Multiwinia tutorial 1 (and tutorial 2 until `MultiwiniaHelp::m_tutorial2AICanSpawn`): `TerminateTask` refuses, so one-shot tasks (Nuke, Infection, forests, AntNest, Plague, Eggs, Meteor) whose `Target` relies on self-termination remain `StateStarted` and **can be fired again**; AirStrike sets Running first and is then removed by `Advance` when the effect object disappears. | `Task::TargetNuke` etc., `TaskManager::TerminateTask` |
| E2 | `Task::Stop()` refusing (`stop=false`) does not stop `TerminateTask` from deleting the task — the armour/turret survives without a slot. | `TaskManager::TerminateTask` |
| E3 | Extinguisher and Tank `Target*` bodies are commented out: such a task would stay Started forever (not reachable in FULLGAME). | `Task::TargetExtinguisher`, `Task::TargetTank` |
| E4 | `Capacity()` in MP ignores research; an eliminated team gets 2 slots (for retribution powers). | `TaskManager::Capacity`; `Multiwinia::AdvanceRetribution` checks `CapacityUsed() < Capacity()` before granting |
| E5 | Squad life timer only counts down in MP and only while `> 0`; instant-unit squads get `m_lifeTimer = -1` (permanent). | `Task::Advance`, `Location::InitialiseTeam` |
| E6 | `StartingPowerups` game option: each team gets N random crate rewards at team init (excluding Randomiser, CrateMania, Darwinians, Bitzkreig, SpawnMania, Plague, RocketDarwinians, SpaceShip, Spam, Triffids, SpeedUpTime, SlowDownTime, Forest, TheBeast and invalid ones); then every non-Shaman task gets `m_startTimer = 30.0` s (locked, greyed, countdown shown). | `code/location.cpp:Location::InitialiseTeam` |
| E7 | `TaskManager::Advance` calls `GlobalResearch::AdvanceResearch()` once per team per tick (research is a single global object). | `TaskManager::Advance` |
| E8 | A squad passing through a RadarDish gets a **new task id** and moves to the last slot (keyboard 1–9 mapping changes); if registration fails (e.g. capacity dropped to 2 on elimination) the squad continues **without a task** (no life timer, no slot, cannot be terminated via the task bar). | `Teleport::EnterTeleport` |
| E9 | Tab is bound to both `ControlUnitCycleRight` and `ControlIconsTaskManagerDisplay`: with a Shaman selected, one Tab press both requests the next task and opens the Shaman UI (the select request applies a round-trip later) [I]. | `data/input/input_preferences.txt`; `TaskManagerInterfaceMultiwinia::Advance` |

---

## [→ 06_player_control_and_input] 2. Task-manager user interfaces

### 2.1 Variant selection (build/runtime)

All three variants are compiled into FULLGAME. `TaskManagerInterface::CreateTaskManager()` [C] `code/taskmanager_interface.cpp`:

```
delete old g_app->m_taskManagerInterface
if inputMode != GAMEPAD and IsSinglePlayer() and pref "ControlMethod" == 0 → TaskManagerInterfaceGestures
else if Multiplayer()                                                      → TaskManagerInterfaceMultiwinia
else                                                                       → TaskManagerInterfaceIcons
```

Called at app init (`App::App`, game mode None → Icons), at every location entry (`code/main.cpp:DoEnterLocation`), when the keybinding/control preferences window changes the control method (`code/interface/prefs_keybindings_window.cpp`), and by `SwitchTaskManagerForX360Controller()` each client frame when the input mode switches to/from gamepad while the overlay is hidden (`code/main.cpp`). `NET_DARWINIA_START_GAME` sets `m_gameMode = GameModeMultiwinia` before the location loads [C] `code/network/clienttoserver.cpp`, so **every Multiwinia match (including the tutorial) uses `TaskManagerInterfaceMultiwinia`** [B:FULL]. Gestures and Icons are reachable only in Prologue/Campaign → [B:LEGACY]. Default prefs: `ControlMethod = 1`, `MultiwiniaControlMethod = 1` [C] `data/default_preferences.txt`.

Per client frame, `TaskManagerInterface::Advance()` runs after `LocationInput::Advance()` and before the script/sound/control-help/shaman/marker/help advances [C] `code/main.cpp` client loop; `Render()` is the last overlay in `Renderer` after markers, help, shaman UI, user input, game cursor and chat [C] `code/renderer.cpp`. Attract mode sets `m_lockTaskManager = true`, which makes `Advance()` return immediately (all variants) [C] `code/attract.cpp` [B:DEAD: `ATTRACTMODE_ENABLED` off].

### 2.2 Base class `TaskManagerInterface`

Fields [C] `code/taskmanager_interface.h`: `m_visible` (overlay shown), `m_highlightedTaskId` (task under the cursor/pad focus, read by `GameCursor::GetHighlightedObject`), `m_verifyTargetting` (unused here), `m_currentMessageType`, `m_currentTaskType`, `m_messageTimer` (wall-clock expiry), `m_viewingDefaultObjective`, `m_lockTaskManager`, `m_quickUnitVisible` (no reader), `m_showingRecentTask` (set by the MP pop-up renderer).

Message types (`enum`): `MessageSuccess`=0, `MessageFailure`=1, `MessageShutdown`=2, `MessageResearch`=3, `MessageResearchUpgrade`=4, `MessageObjectivesComplete`=5, `NumMessages`. String id = `taskmanager_msg<N>` (English: RUN PROGRAM, SYNTAX ERROR, PROGRAM TERMINATED, ACQUIRED, UPGRADED, OBJECTIVES COMPLETED) [C] `data/language/english.txt`. Special task-type values 999 → `taskmanager_mapeditor`, 998 → `taskmanager_accessallareas` [C] `RenderMessages`. `SetCurrentMessage(type, taskType, seconds)` sets expiry = now + seconds.

`enum TMControl { TMTerminate, TMDisplay }`; pure virtuals `ControlEvent(TMControl)`, `Advance`, `Render`, and four "control help" advisories used by `ControlHelpSystem` (P10): `AdviseCreateControlHelpBlue`, `AdviseCreateControlHelpGreen`, `AdviseCloseControlHelp`, `AdviseOverSelectableZone`.

`IsVisible()` = `m_visible || ShamanInterface::IsVisible()` [C] — so the Shaman summon UI counts as "task manager visible" for input gating.

`ScreenZone` (clickable rectangle in the 600-high virtual canvas): `m_name` (action string, < 256 chars), `m_toolTip` (string id, < 1024), `m_x,m_y,m_w,m_h`, `m_data` (int argument), `m_scrollZone` (gamepad navigation group; -1 = none), `m_subZones` (has child weapon zones) [C] `ScreenZone::ScreenZone`. Zones are regenerated every frame by the renderer into `m_newScreenZones` and swapped into `m_screenZones` at the start of the next `Advance` (one-frame latency).

`KeyboardShortcut` = `ControlEventFunctor(controlType)` + action name + data; `operator()` = `g_inputManager->controlEvent(type)`; `noun()` = bound key description (shown in tooltips as `help_keyboardshortcut` + " : " + noun) [C] `code/lib/input/transform.cpp:ControlEventFunctor`.

`RunDefaultObjective(cond)`: for BuildingOnline/Offline/ResearchOwned objectives sets `m_viewingDefaultObjective` (Sepulveda/camera parts are `USE_SEPULVEDA_HELP_TUTORIAL` → dead) [B:LEGACY].

### 2.3 `TaskManagerInterfaceMultiwinia` [B:FULL] — the live interface

#### 2.3.1 What the player sees

The Multiwinia interface has **no pop-up task-manager screen in practice**: `SetVisible()` is only reached inside `if (g_app->IsSinglePlayer())` [C] `TaskManagerInterfaceMultiwinia::Advance`, so `m_visible` stays false for the whole match. The player-facing elements are always-on overlays:

| # | Element | When drawn | Layout (virtual canvas H=600, W=600·aspect) | Rendering | Ref |
|---|---|---|---|---|---|
| U1 | **Placement-area sparkles** (world) | current task is Started and its target-area list is not the single "anywhere" area | for each `TaskTargetArea`: two `Particle::TypeMuzzleFlash` particles (size 60) per frame at centre ± (r·sin a, 0, r·cos a), a = `g_gameTime`·3 rad/s, y = ground + 5 | particle system | `RenderTargetAreas` |
| U2 | **Power-up received pop-up** ("recent task") | for each `TaskSource` in `m_recentTasks` whose task still exists; not after game over | stacked at (W/2, 600/4.1 + i·boxH·1.1); `overallScale=0.4`; iconSize = min(t·3,1)·(600/5)·0.4; boxH = min(t·3,1)·(600/4.2)·0.4; box width = max(text widths)+1.5·iconSize | see §2.3.4 | `RenderRecentRunTask` |
| U3 | **Centre status message** | `m_currentMessageType != -1` and not expired | centred at (W/2, 370); size 40, growing by 15·√(2−remaining) in the last 2 s | `g_titleFont`, outline pass alpha³ then fill alpha = clamp(remaining·0.5,0,1) | `RenderMessages` |
| U4 | **Crate help tooltip** | current task is Started | box width 0.35·600 = 210, top y=420, centred on W/2; text wrapped at 1.75·210, font 11 (+20 % gap); black 0.75 fill, white 0.5 outline; text outline+fill (`g_editorFont`) | caption `crate_tooltip_<name>` (lower-cased); napalm → `crate_tooltip_napalmstrike`; turret option 0/1/3 → gun/rocket/flame tooltip, other → gun; missing → "Missing String:\n<id>" | `RenderCurrentCrateHelp` |
| U5 | **Running-task bar** | whenever ≥1 task (drawn in the `!m_visible` branch every frame) | centred horizontally at y≈550; slot pitch = iconSize + 10; iconSize eases from 60 to 55; selected slot ×1.1, others ÷1.1 | see §2.3.3 | `RenderRunningTasks` |
| U6 | **Tab / LB hint** | ≥1 task | left of first slot: centre (x₀ − 1.2·size, y + size/2 − 0.2·size), half-size 0.4·size | `icons/button_tab.bmp` (keyboard mode) or `icons/button_lb.bmp` (other modes); black 0.75 pass then white 0.75 | `RenderRunningTasks` |
| U7 | **Focus highlight** | `m_currentScreenZone` valid — **never in MP** (see note below); the same highlight is drawn explicitly around the current task icon while it is unplaced (§2.3.3) | zone grown by 10 % of its height on every side | MP: `icons/mouse_highlight.bmp`, additive, a **square** of side h at (x,y) | `RenderScreenZones`, `RenderScreenZoneHighlight` |
| U8 | Standard cursor redraw | only if `m_visible` (never in MP) | – | `GameCursor::RenderStandardCursor` | `Render` |

`Render()` draws nothing when: editing, no location, any Eclipse window open, `m_hideInterface` (Alt+X toggles), Multiwinia grace period, game over, or the local team currently controls a GunTurret building [C] `TaskManagerInterfaceMultiwinia::Render`.

#### 2.3.2 Controls handled (Multiwinia variant)

`Advance()` order [C]: return if locked → `AdvanceTab()` (§1.7) → display/hide logic → `AdvanceScreenZones()` → `AdvanceKeyboardShortcuts()` → `AdvanceTerminate()` → default-objective cleanup → recent-task timer cleanup.

| Control (binding name) | Default binding (`data/input/input_preferences.txt`) | Effect in MP | Ref |
|---|---|---|---|
| `ControlIconsTaskManagerDisplay` / `ControlIconsTaskManagerDisplayDown` | Tab down / pad Back down | If the selected entity is a Shaman → `ShamanInterface::ShowInterface` (P04). Otherwise nothing (SP would show the overlay; Back-down also records `m_taskManagerDownTime`). | `Advance` |
| `ControlIconsTaskManagerHide` | Tab up; pad Back down; pad B down | Hides the Shaman interface if visible (overlay hide path unreachable in MP). | `Advance` |
| `ControlUnitCycleRight` | `]`, **Tab** down; pad right bumper; SpaceNav left button | Select next task (wraps) via `RequestSelectProgram`; camera tracks squads, else free camera | `TaskManagerInterface::AdvanceTab` |
| `ControlUnitCycleLeft` | `[` down; pad left bumper | Select previous task | same |
| `ControlGesturesSwitchUnit` (only if pref `ControlMethod == 0`) | Tab down | Same as cycle right | same |
| `ControlTaskManagerSelectTask1`…`9` | keys 1…9 down | `RunScreenZone("SelectTask", 0…8)`: if slot index valid and its `m_startTimer <= 0` → `RequestSelectProgram(myTeam, task id)` + sound `SelectTask` (Interface); pad mode also calls `HideTaskManager()` | `AdvanceKeyboardShortcuts`, `RunScreenZone` |
| `ControlTaskManagerEndTask` | **unbound** in all shipped input files | would call `TerminateTask(current)` **locally** (non-networked) — dormant | `RunScreenZone("DeleteTask")` |
| `ControlUnitCycleLeft`/`Right`, `ControlCameraForwards`/`Backwards` as shortcuts "ScreenUp"/"ScreenDown" | as above / W, Up / S, Down | **no handler** in the MP `RunScreenZone` → no-op | constructor, `RunScreenZone` |
| `ControlIconsTaskManagerEndTask` | **C** down; pad **Y** down | If no Eclipse window: `RequestTerminateProgram(myTeam, m_currentTaskId)` (no-op if none selected, see §1.9) | `AdvanceTerminate` |
| `ControlActivateTMButton` | mouse left up; pad A up; Enter down | Activates `m_currentScreenZone` — **never fires in MP** because the focus index is forced to −1 while hidden | `AdvanceScreenZones` |
| `ControlIconsTaskManagerDisplayUp` (after Back held > 0.5 s) | pad Back up | Activates focused zone ("hold Back, release on a button") | `ButtonHeld`, `ButtonHeldAndReleased` |
| `ControlMenuUp` / `ControlMenuDown` | pad d-pad/sticks up/down (repeat) | Only while visible (never in MP): move focus within current scroll zone, wrapping | `AdvanceScreenZones` |
| `ControlMenuLeft` / `ControlMenuRight` | pad d-pad/sticks left/right (repeat) | Only while visible: switch scroll zone 1↔2, 2→3 when the zone has sub-zones, step through zone 3 | `AdvanceScreenZones` |
| `ControlCloseTaskHelp` | read but body commented out | no effect | `Advance` |

`RunScreenZone(name, data)` actions in this variant [C]: `NewTask` (refused with `MessageFailure` 2.5 s while another non-Officer task is Started; else if research owned → `RequestRunProgram` + sound `GestureBegin`; pad → hide), `SelectTask` (above), `DeleteTask` (local `TerminateTask(data or current)` + sound `DeleteTask` — only reachable via zones rendered while visible or the unbound key), `SelectWeapon` (research owned → `RequestRunProgram(weapon)` + `GestureBegin` + `GestureSuccess`), `Research` (`SetCurrentResearch`), `Objective` (data = ±(index+10); runs cut-scene script or default objective; hides). In MP only `SelectTask` zones are ever generated (by the bar, scroll zone 2) [C] `RenderRunningTasks`.

**The MP task bar is display-only for the mouse** [C] `AdvanceScreenZones`: mouse hover is evaluated only `if (m_visible)`, and when hidden `m_currentScreenZone` is reset to −1 every frame (the initial auto-focus only picks zones whose `m_scrollZone` equals `m_currentScrollZone` = 1, while bar zones use 2). Clicking a task icon therefore does nothing; tasks are selected with keys 1–9, Tab/`[`/`]`/bumpers, or by selecting the task's unit in the world (`SelectUnit` → `SelectTask(WorldObjectId)`).

**World-hover echo**: when hidden, if the object under the mouse (`LocationInput::GetObjectUnderMouse`) is the `m_objId` of one of my tasks, the matching `SelectTask` zone is flagged "highlight only" (would suppress activation) [C] `AdvanceScreenZones` — the line that would focus it is commented out, so in MP this has no visible effect. `m_highlightedTaskId` = task of the focused `SelectTask` zone, hence always −1 in MP.

Advisories for `ControlHelpSystem` [C]: Blue = focus valid ∧ Back held > 0.5 s ∧ Back pressed ∧ free slot; Green = focus valid ∧ free slot ∧ focused zone is `NewTask`; OverSelectableZone = focused zone exists and is not `NewTask`; CloseControlHelp = visible ∧ ¬(focus valid ∧ held ∧ pressed).

#### 2.3.3 Running-task bar (U5) — exact rendering

Per frame [C] `TaskManagerInterfaceMultiwinia::RenderRunningTasks`:
1. `numTasks = m_tasks.Size()` (capacity is computed but **not** drawn — empty slots are invisible). Static, eased layout: `factor = min(g_advanceTime, 0.1)·10`; `s_iconSize → 55`, `s_iconX → W/2 − (s_iconSize+10)·numTasks/2 + s_iconSize/2 + 5`, `s_iconY → 550`, each `s = s·(1−factor) + target·factor` (statics: initial 60/…/550).
2. For each slot i (x advancing by iconSize+10): selected index = index of `m_currentTaskId`.
   - **Shadow**: `icons/marker_shadow.bmp` (alpha texture), colour (0,0,0,0.81), quad half-extent (iconSize+10)·k/2.2 with k = 1.1 if selected else 1/1.1; blend SRC_ALPHA/ONE_MINUS_SRC_ALPHA.
   - **Colour disc**: `icons/marker_colour.bmp`, additive (SRC_ALPHA, ONE), half-extent (iconSize+10)·k/2.4; colour = team colour; for a GunTurret task the *current owner's* colour of the turret building, and skipped if the turret is neutral (team 255); skipped if `m_startTimer > 0`; RGB ×0.1 if state ≠ Running; alpha × max(0.5, |sin(4·gameTime)|) while Started.
   - **Icon**: `icons/icon_<TypeName>.bmp`, additive, size iconSize·k; colour (0.5,0.5,0.5,1) if `m_startTimer > 0`, (1,1,1,0.6) if not Running, else white. Creates `ScreenZone("SelectTask", "newcontrols_select_<Name>" or "newcontrols_place_<Name>" if Started, icon rect, data=i, scrollZone=2)` and stores `m_screenX/Y/H` in the task.
   - **Current-and-unplaced highlight**: if the task is current and not Running → `RenderScreenZoneHighlight` around the icon (+5 px each side).
   - **Extra info** `RenderExtraTaskInfo(task, centre, size, alpha=1, targetOrders=false, taskManager=true)` (also used by `MarkerSystem` for world markers, P08):

| Condition | Text / graphic | Position (relative to icon centre) | Font size |
|---|---|---|---|
| `m_startTimer > 0` | countdown `%2.0f` s (the outline pass uses the malformed format "%2.0", a code defect) | (+3, +2) | 12 |
| Engineer | current action (`Engineer::GetCurrentAction()` string id) + spirit count | action (+3, +25) size 7; count (+3, +2) | 7 / 12 |
| Harvester | spirit count | (+3, +19) | 12 |
| Squad | "x<alive>"; plus life timer `%1.0f` if > 0, red (1,0,0) when ≤ 10 s | count (+11, +5) size 10; timer (+3, +0.4·size) | 10 / 12 |
| Booster, HotFeet, Subversion, Rage, Tank (non-TankBattle), while Running with life > 0 | life timer `%1.0f`, red when ≤ 10 s | (+3, +0.4·size) | 12 |
| Armour | passenger count (unless targetOrders); `icons/arrows_load.bmp` when state (or `m_conversionOrder` for targetOrders) = Loading, `icons/arrows_unload.bmp` when Unloading; `icons/arrows_moving.bmp` when in the task bar and > 30 wu from its waypoint | count (+3, +0.33·size), font 0.2·size; arrows: quad half-extent size/2.2, subtractive-ish shadow pass (SRC_ALPHA, ONE_MINUS_SRC_COLOR) then additive 0.3 alpha | – |

   - **Compass** (`RenderCompass`) if `task->GetPosition() != 0`: a direction pointer around the icon toward the task's world position. On-screen target (in front of camera and inside viewport): vector from icon to the projected point. Off-screen: cast the click-ray through the icon, take the point 1000 wu along it, step 100 wu toward the target, project, and use that direction. Quad of half-size `compassSize` (= 0.77·iconSize, ×k) oriented along the vector; `icons/compass_shadow.bmp` black 0.8 alpha-blend then `icons/compass.bmp` additive, alpha 1.0 if current else 0.5.
   - **Delete icon** (only if visible → never in MP): `icons/icon_delete.bmp` at (x, y+0.6·size), size 0.4·size, zone `DeleteTask` with data = task id.

#### 2.3.4 "Power-up received" pop-up (U2) — exact rendering

Data comes from `TaskManager::m_recentTasks` (§1.5). For entry i, t = now − |m_timer| [C] `RenderRecentRunTask`:
- Spin: while t < 3, `m_rotation += g_advanceTime · 20 · (3 − t)^4`.
- scale = min(3t, 1); alpha = min(2t, 1).
- Not minimising (m_timer > 0, first 3 s): black 0.9 box, a vertical gradient overlay (0.3 black → 0.3 white), white 0.4 outline; caption line 1 = source text (`multiwinia_cratepowerupfound` / `multiwinia_reinforcementsgiven` / `multiwinia_retributiongiven`) at 0.7·fontH in grey 0.7; line 2 = upper-cased power name (napalm → `cratename_napalmstrike`; turrets → `GunTurret::GetTurretTypeName(option)`), size fontH (×0.8 if `TextLanguage` ≠ english); fontH = 0.3·boxH. Crate source (0) also draws a faint (alpha 0.15) additive `icons/icon_crate.bmp` rotated by −0.02·m_rotation, clipped to the box.
- Minimising (negative timer, 1 s): icon flies from the pop-up toward its slot in the bar (`m_screenX/Y`), lerp factor min(2t,1); size lerps from 600/6 to `m_screenH`; alpha × (1 − min(t,1)).
- Always: shadow `icons/icon_shadow.bmp` (1.25×, SRC_ALPHA/ONE_MINUS_SRC_COLOR, grey 0.9·alpha), team-colour ×0.6 disc `icons/marker_colour.bmp` additive, then the task icon additive with alpha.

#### 2.3.5 Selection model — what the task manager contributes

- **Selectable via task manager**: any task in `m_tasks` (slots 1–9 by key; all by Tab/bumper cycling). Selecting a task routes through `Task::SwitchTo` (§1.7): it selects the task's unit/entity, or for booster-family powers makes the affected Darwinians `m_underPlayerControl` (a temporary group selection), or deselects for one-shot powers so the next left click places them.
- **Single selection**: one current task (`m_currentTaskId`) and one current unit/entity/building per team (`Team::SelectUnit`) [C] `code/team.cpp:Team::SelectUnit`. Selecting an Officer entity clears the current task (`SelectTask(-1)`); `Team::m_blockUnitChange` suppresses selection changes while reinforcements are spawned.
- **Task locked**: `m_startTimer > 0` blocks select (zone/key) and target.
- Darwinian group selection (selection circle), officer/armour orders and promotion are handled in `LocationInput` (P01) — summarised in §2.6 for completeness.

### 2.4 `TaskManagerInterfaceIcons` [B:LEGACY] (Prologue/Campaign with ControlMethod ≠ 0 or gamepad)

Three vertically-sliding screens over a virtual 600-high canvas: `ScreenOverlay`=0, `ScreenTaskManager`=1, `ScreenObjectives`=2, `ScreenResearch`=3; `m_screenY` eases to `m_desiredScreenY` ∈ {−1 (research), 0, +1 (objectives)} with `diff = g_advanceTime·10`, snapping when within 0.001 [C] `AdvanceScrolling`. (Despite the "Y" naming the projection offsets `left` horizontally by `m_screenY·600`, [C] `SetupRenderMatrices`.)

Shortcuts [C] constructor: `NewTask` Squad/Engineer/Officer/Armour ← `ControlIconsTaskManagerNewSquad/NewEngineer/NewOfficer/NewArmour` (F1–F4); `SelectWeapon` Grenade/Rocket/AirStrike/Controller ← `ControlIconsTaskManagerSelectGrenade/Rocket/AirStrike/Controller` (F5–F8); `SelectTask` 0–8 ← `ControlTaskManagerSelectTask1–9` (1–9); `DeleteTask` ← `ControlTaskManagerEndTask` (unbound); `ScreenUp` ← `ControlTaskManagerScrollUp` (W) and `ControlXBoxMenuCycleUp` (pad LB repeat); `ScreenDown` ← `ControlTaskManagerScrollDown` (S) and `ControlXBoxMenuCycleDown` (pad RB repeat).

Controls [C] `Advance`, `AdvanceScreenEdges`, `AdvanceTerminate`: Display (Tab down / pad Back down) → Shaman UI if a Shaman is selected, else SP shows overlay (sound `Show`); Hide (Tab up / Back / B) → `HideTaskManager` (mouse warped to screen centre, re-select current task, `Gesture::EndGesture`, sound `Hide`); screen edges: cursor y < 10 px → research, y > H−10 → objectives (only when not scrolling; sound `Slide`); `ControlIconsTaskManagerEndTask` (C / pad Y) → `RequestTerminateProgram`; `ControlActivateTMButton` activates zone; pad navigation as §2.3.2 but `ButtonHeld()` hard-wired to false. Extra screen-zone actions: `ShowScreen` (data = screen id), `ScreenUp`, `ScreenDown` (only while visible). `NewTask` for Squad also increments `GlobalWorld::m_spSquaddiesUsed[location index]` (location 12 → index 0, else id−1; sentinel 3599999 → 1) [C].

Elements [C] `Render` and `Render*`: centre message (same formula as U3 but `g_gameFont`, and the first `MessageObjectivesComplete` adds the mission time to the leaderboard time `m_spLevelTimes`); title bar (grey `textures/interface_grey.bmp`, tint (0.5,0.5,1,0.7), 80 px high, plus a line); bottom divider `textures/interface_divider.bmp` 26 px; tooltip with typewriter reveal (50 chars/s) at (20, H−12) size 12 plus shortcut noun at (W−250, H−12); running-task column at x≈40→80 (visible), y from 130, icon 50→70 px, gap 10, slot shadows for **every capacity slot**, Started icons blink (invisible when `fmod(gameTime,1) < 0.4`), current task alpha 0.9 else 0.27, booster life timer, compass (0.75·size, yellow 0.8 when current), squad weapon sub-icons (Grenade/Rocket/AirStrike/Controller if researched; current weapon bright; zones `SelectWeapon`, scroll zone 3; parent zone `m_subZones=true`), delete icons (zone `DeleteTask`); "Create new task" menu (`newcontrols_createnewtask`) listing Squad, Engineer, Officer, Armour, Harvester if researched (rows 46 px, zones `NewTask`, scroll zone 1, tooltips `newcontrols_create_<Name>`); objectives screen (primary/secondary lists with complete/incomplete colours, objective counter, statistics box: time hh:mm:ss, kills = team 2 kills, survivors = team 0 Darwinian count, squaddies used); research screen (one row per researched item, 3 progress bars with `RequiredProgress(level)`·0.85 px, version "v<level>.0", current research pulses |sin(2·gameTime)|); navigation arrows or LB/RB button icons (pad + `ControlHelpEnabled`), blinking alpha 155/255 every second; "launchpad" Prologue progress bar with 5 hard-coded objectives (building ids 66, global buildings 1/2/3, research item 130, spawn points 18/51/52/54) and a 256-particle starburst celebration (`textures/starburst.bmp`, alpha decay 0.015–0.03 per frame, uses non-synced `frand`) [C] `EvaluteTutorialObjectives`, `AdvanceProgressBar`, `RenderProgressBar`. `RenderGestures` and `RenderOverview` exist but are never called in this variant [C].

### 2.5 `TaskManagerInterfaceGestures` [B:LEGACY] (Prologue/Campaign, keyboard/mouse, ControlMethod = 0)

Overlay is **held**: visible while `ControlGesturesTaskManagerDisplay` (Alt pressed / pad X pressed) is true; released → `HideTaskManager` [C] `Advance`. Screens slide horizontally (`m_screenX` eases at `g_advanceTime·5`, ±1). Screen-edge or `ControlGestureLeft` (A, Left, pad LB, pad d-pad left — note the typo `donw` in that binding line) / `ControlGestureRight` (D, Right, pad RB, d-pad right) switch screens, 10 px border, only when not drawing [C] `AdvanceScreenEdges`.

Gesture capture [C] `AdvanceGestures` (all while the overlay is held and the central screen is shown and no screen zone is focused):
1. If `ControlGestureActive` (left mouse pressed / pad A pressed) or a recording is in progress and the current task is Started (not Officer) → `MessageFailure` 2.5 s, abort (must place first).
2. Begin: not recording ∧ GestureActive → `Gesture::BeginGesture`, sound `GestureBegin` (TypeGesture).
3. While held: `AddSample(g_target->X(), g_target->Y())` every client frame (screen pixels).
4. Release: `EndGesture`, sound `GestureEnd`; if `GestureSuccess()` → `taskType = MapGestureToTask(GetSymbolID())`; if researched → `RequestRunProgram(taskType)` + sound `GestureSuccess`, else `MessageFailure`; if not success → `MessageFailure` + sound `GestureFail`.

Terminate: `ControlGesturesTaskManagerEndTask` (Ctrl+C chord / pad Y) → **local** `TerminateTask(current)` and `m_currentTaskId = -1` (SP only, not networked) [C] `AdvanceTerminate`. Shortcuts: `SelectTask` 0–8 (keys 1–9), which here set `m_currentTaskId` and call `SelectTask` locally.

Elements: title "taskmanager_taskmanager" size 50; grey title bar 100 px high spanning 3 screens; silver dividers; central red panel `textures/interface_red.bmp` multiplied by the alpha mask `icons/gesture_alpha.bmp` (red channel → alpha, two texture units, GL_COMBINE); **gesture guide** `icons/gestureguide.bmp` 320×320 centred at (W/2, H/2 − 10), additive 0.7; running-task row at y=490 (icon 70, shadow 78, gap 18, all capacity slots; Started blink; current alpha 1.0, visible 0.5, hidden 0.3; engineer/squad info; compass size 53); overview tabs (research / task manager / objectives) at y=95, 100×16 boxes; objectives and research screens as in Icons (research rows also show `icons/gesture_<Name>.bmp` for every type except Darwinian, Laser, TaskManager); live gesture trail (§3.9) [C] `Render*`.

### 2.6 Multiwinia player-action reference (cross-partition summary; primary owner P01 `code/location_input.cpp`, P10 input)

Listed so 06 has one table; evidence [C] `code/location_input.cpp:LocationInput::AdvanceTeamControl`, `AdvanceNoSelection`, `SelectObjectUnderMouse`, `GetObjectUnderMouse`, bindings `data/input/input_preferences.txt`. "MP" gates: most world actions are suppressed during the grace period, game over and pause.

| Action | Mouse/keyboard | Pad | Condition | Network command | Notes |
|---|---|---|---|---|---|
| Select object under cursor | left down (`ControlUnitSelect`) | A down | overlay hidden; pick priority: marker icon under mouse, else nearest of entity/unit/building along the click ray; buildings only RadarDish, GunTurret, FenceSwitch (own or neutral) | `SelectUnit` | RadarDish (unless level option NoRadarControl) → camera radar-aim; own GunTurret/ChessPiece → camera turret-aim; FenceSwitch → `Switch()` called **locally** (blocked type in MP). If pref `ControlRemap == 0` nothing new can be selected while something is selected (the key is absent from default prefs → `GetInt` default −1 → restriction inactive). Clicking empty ground with something selected (MultiwiniaControlMethod 1) → "MouseError" sound + Space-to-deselect help |
| Deselect | Space down (`ControlUnitDeselect`) | B down | always | `SelectUnit(-1,-1,-1)` | camera free; `m_previousTaskId = -1` |
| Place / fire current task | left down (`ControlUnitCreate`) | A down | current task Started, overlay hidden, not grace/game over/pause | `TargetProgram` | right-click instead → "MouseError" + help hint |
| Select / cycle task | 1–9; Tab, `]`, `[` | RB / LB | §2.3.2 | `SelectProgram` | |
| Terminate current task | C | Y | §2.3.2 | `Terminate` | |
| Terminate entity under cursor | C (`TMTerminate`) | Y | nothing selected; entity is not a Darwinian | `RequestTerminateEntity` + `SelectUnit(-1)` | same key press also sends task terminate |
| Demote (kill) selected officer | C | Y | officer selected | `RequestTerminateEntity` | |
| Release a controlled turret | C | Y | own GunTurret selected | `SelectUnit(-1)` | also auto-released if the turret changes owner |
| Promote Darwinian to Officer | right down (`ControlDarwinianPromote`) | X down | nothing selected, no Darwinians selected, no task being placed; nearest own selectable Darwinian within 10 wu of cursor | `RequestPromoteDarwinian` | the AI uses the Officer *task* instead (§1.3) |
| Toggle armour load/unload / conversion order | right down on armour | X | nothing else selected or it is the selected armour; on the order marker → toggles the conversion order and deselects | `RequestToggleEntity(team, idx, secondary)` | |
| Toggle officer orders | right down on officer | X | as above | `RequestToggleEntity(team, idx, false)` | on every peer: if officer is in formation mode or has `OrderGoto` → `CancelOrders()`; else `SetOrders(pos + front)` and each peer issues `RequestSelectUnit` for that officer [C] `code/network/clienttoserver.cpp` TOGGLEENTITY handler (officer semantics P03) |
| Select Darwinians (selection circle) | left down starts at cursor, left held grows radius by 100·dt up to **100 wu**, left up ends | A down/held/up | nothing selected, no task Started, not over an own non-Darwinian object/building, not grace period | `RequestSelectDarwinians(team, centre, radius)` sent whenever network time advances | particles drawn by GameCursor (§4.6) |
| Order selected Darwinians | right down (`ControlIssueDarwinianOrders`) | X down; RT (`UnitSecondaryFireDirected`, unbound in data) | Darwinians selected; ≥ 1.0 s since last order | `RequestIssueDarwinanOrders(team, pos, directRoute)` | Ctrl (`OrderDirectRoute`) = direct route; forced direct if ≥ 25 % carry a WallBuster; target snaps to RadarDish entrance, crate, non-full armour or officer under cursor |
| Officer goto / special move | right down (`ControlIssueSpecialOrders`); Shift = `OrderSpecialMove`; Ctrl = direct route | pad A (special = X toggles) | officer selected | `RequestOfficerOrders(team, pos, direct, specialMove)` | MultiwiniaControlMethod 0: left (`UnitSetTarget`) = force-move, right in formation first sends `RequestOfficerNone` |
| Armour move / deploy | right down; Shift = deploy (inverted when MultiwiniaControlMethod 0) | A / X | armour selected | `RequestArmourOrders(team, pos, deploy)`; deploy also deselects | MultiwiniaControlMethod 0: right-button-held `UnitSetTarget` = move |
| Radar dish aim | left down while dish selected | A | dish selected | `RequestAimBuilding(team, dish, pos or other dish start point)` | |
| Toggle ready (grace period) | Space | A / B | grace period | `RequestToggleReady` | after game over the same key returns to menu |
| Shaman summon UI | Tab | Back | Shaman selected | (P04) | |


---

## [→ 06_player_control_and_input] 3. Gesture recognition (`Gesture`)

### 3.1 Liveness

`App::App` always constructs `m_gesture = new Gesture("gestures.txt")`, which loads and trains on `data/gestures.txt` at start-up [C] `code/app.cpp`. The recogniser is **only driven** by `TaskManagerInterfaceGestures` (SP, [B:LEGACY]) and the gesture editor window (`GESTURE_EDITOR` && `USE_SEPULVEDA_HELP_TUTORIAL` → [B:DEAD]) [C] `code/interface/gesture_window.cpp`. In Multiwinia the only calls are `EndGesture()` from `HideTaskManager()` (which recomputes features of a stale/empty buffer and prints "Sorry, not enough mouse samples" when < 3 samples) [C]. Gesture symbols are therefore **not a Multiwinia mechanic**; they are documented in full because they are a self-contained, reusable recogniser.

### 3.2 Algorithm overview

A **Rubine-style linear statistical classifier** (13 global stroke features, per-class mean vectors, a pooled ("common") covariance matrix, linear discriminant evaluation, soft-max confidence, Mahalanobis rejection) [C] `code/gesture.cpp`; [I] the feature list and training maths match D. Rubine, "Specifying Gestures by Example" (SIGGRAPH 1991), a public-domain algorithm — safe to re-implement.

Constants [C] `code/gesture.h`:

| Name | Value | Meaning |
|---|---|---|
| `GESTURE_MAX_MOUSE_SAMPLES` | 1024 | sample buffer (debug assert on overflow) |
| `GESTURE_MAX_FEATURES` | 13 | features f1..f13 (array index 0 unused, always 0) |
| `GESTURE_MAX_TRAINERS` | 32 | max training examples per symbol (assert `< 32`) |
| `GESTURE_MIN_SAMPLE_DISTANCE` | 6.0 px | a new sample closer than this to the previous accepted one is dropped |
| `GESTURE_CONFIDENCETHRESHOLD` | 0.5 | min confidence for success |
| `GESTURE_MAHALANOBISTHRESHOLD` | 3000.0 | max Mahalanobis distance for success |
| confidence temperature | 500.0 | divisor in the soft-max (literal in `RunComparison`) |

### 3.3 Sampling

- `BeginGesture()`: clear samples and results, `recording = true`.
- `AddSample(x, y)` — called once per client frame while the button is held with **integer screen pixel** coordinates of the target cursor (`g_target->X/Y`, origin top-left, y down). Drops the sample if its Euclidean distance to the last accepted sample is < 6.0 px; else stores `{x, y, time = GetHighResTime()·1000}` (milliseconds, double). No resampling, no smoothing, no normalisation of position/scale/rotation — the features themselves are scale-dependent (lengths in px, speeds in px²/ms², durations in ms) [C] `Gesture::AddSample`.
- `EndGesture()`: `recording = false`; `CalculateFeatureSet()`; `RunComparison()`.

### 3.4 Features (computed in `CalculateFeatureSet`, n = number of samples; requires n ≥ 3, otherwise the previous feature set is kept)

Let sᵢ = (xᵢ, yᵢ, tᵢ).

| f | Definition as implemented [C] | Note |
|---|---|---|
| f1 | (x₂−x₀)/‖s₂−s₀‖ | initial direction cosine (uses 3rd point to reduce noise) |
| f2 | (y₂−y₀)/‖s₂−s₀‖ | initial direction sine |
| f3 | √((R−L)² + (B−T)²) where L = min x, T = min y, B = max y, and **R is updated as "if xᵢ > R then R = yᵢ"** (initial R = 0, L = T = 999999, B = 0) | bounding-box diagonal — **defect**: R takes a y value |
| f4 | atan((B−T)/(R−L)) (`iv_atan`) | bounding-box diagonal angle, same defect |
| f5 | ‖s_{n−1} − s₀‖ | start–end distance |
| f6 | (x_{n−1}−x₀)/f5 | start–end cosine (NaN if f5 = 0) |
| f7 | (y_{n−1}−y₀)/f5 | start–end sine |
| f8 | Σ_{j=0}^{n−3} ‖s_{j+1}−s_j‖ | total length — **defect**: last segment omitted |
| f9 | Σ_{k=1}^{n−2} θₖ, θₖ = atan( (aₓ·b_y − bₓ·a_y) / (aₓ·bₓ + a_y·b_y) ), a = s_{k+1}−s_k, b = s_k−s_{k−1} | total turning; CRT `atan` (not atan2) so each θ ∈ (−π/2, π/2) |
| f10 | Σ |θₖ| | |
| f11 | Σ θₖ² | |
| f12 | max over consecutive pairs of (Δx²+Δy²)/Δt² (pairs with Δt = 0 skipped) | max speed², px²/ms² |
| f13 | Σ Δt over pairs with Δt ≠ 0 | duration, ms |

### 3.5 Training (`TrainSystem`, run after loading and after every added sample)

For C symbols with nᶜ trainers each (training vectors include index 0, always 0):
1. Mean μᶜᵢ = (1/nᶜ) Σₑ fᵢ⁽ᵉ⁾ for i = 0..13.
2. Per-class scatter Sᶜᵢⱼ = Σₑ (fᵢ⁽ᵉ⁾−μᶜᵢ)(fⱼ⁽ᵉ⁾−μᶜⱼ), i, j = 0..13.
3. Pooled covariance **as implemented**: Σᵢⱼ = [ Σ_c ( Sᶜᵢⱼ / nᶜ − 1.0 ) ] / ( Σ_c nᶜ − C ). (The source writes `S / (double) n - 1.0`; operator precedence makes it "divide by n, then subtract 1", not "divide by n−1". Because f0 ≡ 0, row/column 0 of Σ is the constant −C/(N−C), which keeps the 14×14 matrix invertible.) **A re-implementation must reproduce this exact formula to reproduce the shipped classification with `data/gestures.txt`.**
4. Σ⁻¹ by Gaussian elimination with scaled partial pivoting (`code/lib/invert_matrix.cpp:inv`, 1-based).
5. Weights: wᶜⱼ = Σ_{i=1}^{13} Σ⁻¹ᵢⱼ μᶜᵢ for j = 1..13; wᶜ₀ = −½ Σ_{i=1}^{13} wᶜᵢ μᶜᵢ.

### 3.6 Classification and thresholds (`RunComparison`, `GetSymbolID`, `GestureSuccess`)

- Score rᶜ = wᶜ₀ + Σ_{i=1}^{13} wᶜᵢ fᵢ (0 for classes with no trainers).
- Symbol = first index with the maximum score.
- Confidence of class i = 1 / Σⱼ exp((rⱼ − rᵢ)/500).
- Mahalanobis of class i = Σ_{j=1}^{12} Σ_{k=1}^{12} Σ⁻¹ⱼₖ (fⱼ−μⁱⱼ)(fₖ−μⁱₖ) — **defect**: loops stop at `< GESTURE_MAX_FEATURES`, so f13 (duration) is excluded.
- Success ⇔ confidence(best) ≥ 0.5 ∧ Mahalanobis(best) ≤ 3000.0.
- Accessors return −1.0 for out-of-range symbol ids.

Training-file format (`data/gestures.txt`, grammar owned by X1b) [C] `Gesture::LoadTrainingData`/`SaveTrainingData`: line 1 = number of symbols; then per symbol a line with the trainer count and a line with trainerCount × 14 floats (f0..f13 per trainer, f0 written as 0). Shipped file: **8 symbols × 20 trainers** [C] (parsed by `_work/scripts/P09_gestures.py`).

Validation [I] (simulation, not original code): a faithful Python port including all defects (`_work/scripts/P09_gesture_sim.py`) accepts 160/160 training vectors and classifies all eight `data/gesture_demos/*.txt` strokes (replayed at 40 ms/sample, screen height 768 and 600) as their intended symbol with confidence 0.57–1.00 and Mahalanobis 72–887 (< 3000). These are usable as regression test vectors.

### 3.7 Symbol set (complete)

Symbol id = order in `data/gestures.txt`; mapping by `TaskManager::MapGestureToTask` [C]; shapes [I] from the stroke coordinates in `data/gesture_demos/<name>.txt` (screen space, y down) and confirmed by the trained means [C] (`_work/scripts/P09_gestures.py` output).

| Id | Task | Stroke (from demo file) | Mean f5 start–end px | Mean f13 duration ms | Mean f9 turning rad | Icon |
|---|---|---|---|---|---|---|
| 0 | Squad | closed triangle: from apex down-right, left along the base, back up to apex | 24.2 | 955 | −0.22 | `icons/gesture_squad.bmp` |
| 1 | Engineer | square arch "⊓": up, right, down | 234.7 | 739 | −1.29 | `icons/gesture_engineer.bmp` |
| 2 | Grenade | rounded arch "∩": up, curving over to the right, down | 351.8 | 482 | −3.14 | `icons/gesture_grenade.bmp` |
| 3 | Rocket | right-pointing arrow: shaft left→right, then up-left, down, back up-right to the tip (arrowhead) | 449.4 | 902 | −2.84 | `icons/gesture_rocket.bmp` |
| 4 | AirStrike | three-spoke star drawn as out-and-back strokes from a centre (up, down-right, down-left) | 25.8 | 1845 | 0.15 | `icons/gesture_airstrike.bmp` |
| 5 | Controller | zig-zag with three peaks "/\/\/\" | 344.1 | 1133 | −0.04 | `icons/gesture_controller.bmp` |
| 6 | Officer | "V" / tick: down-right then up-right | 364.6 | 407 | −0.82 | `icons/gesture_officer.bmp` |
| 7 | Armour | closed shield/pentagon: right along top, down, down-left to a point, up-left, up to start | 30.3 | 1358 | −0.57 | `icons/gesture_armour.bmp` |

The gesture guide image `icons/gestureguide.bmp` (all symbols) and `icons/gesture_alpha.bmp` (panel mask) are used only by the Gestures interface [C].

### 3.8 Gesture → action

Successful symbol → task type (above) → if `GlobalResearch::HasResearch(type)` → `RequestRunProgram(team, type)`; Squad/Engineer/Officer/Armour create tasks; Grenade/Rocket/AirStrike(SP)/Controller switch the current squad's weapon (§1.3) [C] `TaskManagerInterfaceGestures::AdvanceGestures`, `TaskManager::RunTask(int)`.

## [→ 26_animation_and_vfx] 3.9 Gesture trail rendering [B:LEGACY]

[C] `TaskManagerInterfaceGestures::RenderGestures` (an identical copy exists, uncalled, in `TaskManagerInterfaceIcons::RenderGestures`):
1. Lazily create a 64×64 `BitmapRGBA` and texture.
2. Each frame clear to black; map every sample to the 64×48 region (x·64/screenW, y·48/screenH); draw line segments between consecutive samples with grey level α = 255·(1 − age/3 s) (0 if age ≥ 3 s), where age = now − sample time in **whole seconds** (the int accessor truncates ms/1000), so the trail fades in 1-s steps.
3. `ApplyBlurFilter(1.4)`; upload with `glTexImage2D` (RGBA).
4. Draw a full-screen quad with texcoords (0,0)–(1,0.75) twice, additive (SRC_ALPHA, ONE): pass 1 `GL_NEAREST`, colour (0.2,0.2,0.9,1) (blocky blue); pass 2 `GL_LINEAR`, colour (0.7,0.7,1.0,1) (soft glow). Result: a low-resolution, blurred, glowing blue trail covering the screen.

Gesture-demo playback (Sepulveda tutorial) is [B:DEAD] — see Appendix.

---

## [→ 06_player_control_and_input] 4. GameCursor — cursors, highlights, placement previews

### 4.1 Cursor objects

`MouseCursor` = a textured screen- or world-space quad with optional blurred shadow [C] `code/gamecursor.cpp:MouseCursor`. Constructed from a BMP: the bitmap is registered as a texture under its file name, then a copy blurred with `ApplyBlurFilter(10.0)` is registered as `shadow_<file>`. Defaults: size 20, hotspot (0,0) (fraction of size; (0,0) = top-left, (1,1) = bottom-right), not animating, shadowed, additive, colour white.

| Member | Texture | Hotspot | Size (px or wu) | Animated | Colour / blend | Used in MP? |
|---|---|---|---|---|---|---|
| `m_cursorStandard` | `icons/mouse_game.bmp` | (0.055, 0.070) | 35 | no | white, additive, shadowed | **yes** — default pointer |
| `m_cursorPlacement` | `icons/mouse_placement.bmp` | (0.5, 0.5) | 40 (varies) | per call | white, additive | **yes** — "trying to enter" marker; SP placement, orders markers |
| `m_cursorDisabled` | `icons/mouse_disabled.bmp` | (0.5, 0.5) | 60 | no | red (255,0,0), additive | **yes** — invalid enter; (texture also drawn as a red sprite for invalid placement / no route) |
| `m_cursorMoveHere` | `icons/mouse_movehere.bmp` | (0.5, 0.5) | 30 | yes | (255,255,150) additive | SP only |
| `m_cursorHighlight` | `icons/mouse_highlight.bmp` | (0.5, 0.5) | 30 (set by caller) | yes (off when used) | white additive | SP only (the same texture is the MP task-bar focus highlight) |
| `m_cursorTurretTarget` | `icons/mouse_turrettarget.bmp` | (0.5, 0.5) | caller | no | white, **alpha-blend**, **no shadow** | **yes** — crosshair |
| `m_cursorSelection` | `icons/mouse_selection.bmp` | (0.5, 0.5) | caller | no | team colour | SP only |
| `m_cursorEnter` | `icons/mouse_enter.bmp` | (0.5, 1.0) | 50 | yes | white additive | SP only |
| `m_cursorMissile` | – (NULL, never created) | – | – | – | – | no |

Animation: `GetSize() = size + |sin(4·g_gameTime)|·size·0.6` (pulses between 1.0× and 1.6× at 4 rad/s) [C] `MouseCursor::GetSize`.

2D render (`MouseCursor::Render(x, y, angle)`): top-left = (x − s·hotX, y − s·hotY); translate to quad centre, rotate by `angle` degrees about Z; shadow pass with `shadow_` texture, blend (SRC_ALPHA, ONE_MINUS_SRC_COLOR), colour (1,1,1,0) (darkens what is beneath by the shadow's colour); main pass with colour, blend additive or alpha; depth writes off, culling off; texcoords V flipped (BMP bottom-up) [C].

3D render (`Render3D(pos, front, up, cameraScale)`): quad lying in the plane spanned by `right = front × up` and `front`, size s = GetSize() (+ (camDist − 300)/20 when camera farther than 300 wu and cameraScale), offset by hotspot; same two passes (main pass always additive) [C]. (`Render3D(pos)` derives `up` from `normalMap(pos.x, pos.y)` — **defect**: should be pos.z; unused.) `RENDER_CURSOR_3D` is never defined, so all in-game cursors use the projected-2D path [C].

### 4.2 Frame flow in Multiwinia

`Renderer` calls, in order, `GameCursor::Render()` then `GameCursor::RenderBasicCursorAlwaysOnTop()` [C] `code/renderer.cpp`.

1. `Render()`: return if the interface is hidden (Alt+X) and no window is open; return if at a location without a local team (unless `SPECTATOR_ONLY`, defined only for the hardware-compat and assault-stress targets); in MP → `RenderMultiwinia()` and return [C].
2. `RenderMultiwinia()` (3D matrices; returns if no location or any Eclipse window is open) [C]:
   a. `RenderSelectionCircle()` (§4.6).
   b. Team colour for effects: team colour (or (255,255,100) if none) + (100,100,100) clamped, alpha 150.
   c. **Highlighted object** (§4.3): if any → `RenderUnitHighlightEffect(pos, radius, teamCol)`; if it is an Armour → route line from armour to its waypoint (`RoutingSystem::RenderRouteLine`, intensity 0.5); if it is an Officer in formation mode, not my selected entity, and in `StateToWaypoint` → route line to its waypoint + formation preview at the waypoint facing the travel direction. Store `m_objUnderMouse`.
   d. **Selected object**: unless it is a GunTurret building, or a squad/squaddie currently in transit inside any RadarDish (`m_inTransit`), draw `RenderUnitHighlightEffect` with teamCol + (50,50,50), alpha 255.
   e. **Promotable Darwinian**: if nothing highlighted or selected, no Darwinians selected and no selection circle → `Task::FindDarwinian(mouse3d, myTeam)`; if found and selectable → highlight ring at its predicted position, radius 2× entity radius.
   f. **Officer formation preview**: if my selected entity is an Officer and the cursor is not on it: if `Officer::IsFormationToggle(mouse3d)` → preview formation at the officer facing the cursor; else if in formation mode → preview at the cursor facing away from the officer.
3. `RenderBasicCursorAlwaysOnTop()` (MP only; skipped when interface hidden and no window): 2D; if no window open → `RenderSelectedObjectNearCursor()` (§4.5) and `RenderTryingToEnterEffect()` (§4.7); then `RenderCrosshair()`; if it returned false → `RenderStandardCursor(x, y)`.

The standard pointer is therefore drawn in MP whenever no crosshair applies; with Eclipse windows open only the icon/enter effects are suppressed and `RenderCrosshair` returns false, so the pointer is still drawn [C] `RenderBasicCursorAlwaysOnTop`.

### 4.3 Highlight / selection resolution

`GetHighlightedObject(id, pos, radius)` [C]:
- Source: if the TM overlay is hidden, `LocationInput::GetObjectUnderMouse` (marker icon under mouse first; then nearest of entity/unit/building along the click ray; buildings limited to RadarDish/GunTurret/FenceSwitch; none while controlling a turret; while placing an Officer task a nearby Darwinian counts). If visible, the task under the TM focus (`m_highlightedTaskId`).
- Accepted kinds: building = RadarDish, Bridge, own GunTurret, FenceSwitch (radius ×0.8; turret pos raised by 0.5·radius); unit (if `IsSelectable`) at centre + vel·`g_predictionTime` + first entity's centre offset; entity (if `IsSelectable`) at predicted position, Darwinian radius ×2.
- Predicted position = pos + centrePos + vel·(`g_predictionTime` [+ `SERVER_ADVANCE_PERIOD` if the entity index is beyond the team's last-updated index this tick]) [C] `GetPredictedPosition` (client-side interpolation, P15).
- Side effect: when the highlighted id changes, sound `UnitHighlight` (TypeMultiwiniaInterface) at most every 0.2 s.

`GetSelectedObject`: selected entity (predicted pos) → else current building (centrePos) → else selected unit (centre + vel·prediction + first entity's centre) [C].

### 4.4 Unit highlight effect (rings + glow cylinder)

[C] `RenderUnitHighlightEffect(pos, radius, colour)`:
- `numSteps = max(25, 2·radius)`; main ring width = 0.2·radius; outline width = 0.075·radius.
- Projection near plane pushed to 1.03× to reduce z-fighting; alpha blend; depth writes off.
- Ring 1 (team colour): inner radius−0.2r, outer r. Ring 2 (black): inner r−0.2r−0.075r, outer r−0.2r. Ring 3 (black): r → r+0.075r.
- `RenderRing`: quad strip in the plane perpendicular to the ground normal at the centre; vertices blended toward terrain height by `amountToFollowLand = min(1, 75 / distance(camera, centre))` (only applied if > 0.1) — close objects drape on the terrain, far ones stay planar.
- Glow cylinder: radius r + 0.075r, height r·max(1, camDist/300), `numSteps` sides, colour alpha at the base fading to 0 at the top (smooth shading), "up" = normalised average of world up and ground normal.

### 4.5 Icon near the cursor (`RenderSelectedObjectNearCursor`) — MP context rules

Not drawn after game over. Anchor: cursor + 5 px along direction n = normalise(0.85, 1) (down-right); icon size = screenH/24; drawn through `MarkerSystem::RenderShadow`, `RenderColourCircle` (team colour) and `RenderIcon` (P08) [C] `RenderSelectedIcon`. Priority:

| Priority | Context | What is drawn | Ref |
|---|---|---|---|
| 1 | A current task exists | task icon `icons/icon_<Name>.bmp`; while Started it blinks (shown when `fmod(now, 0.4) < 0.25`) | `RenderSelectedTaskIcon` |
| 1a | …and Started and `IsValidTargetArea(mouse3d)` is false | red sprite `icons/mouse_disabled.bmp` (×1.2 of half icon) at the icon centre (anchor + 50 px along n) | same |
| 1b | …Started GunTurret / AntsNest | ghost model at the cursor: `battlecannonturret.shp` / `anthill.shp`, identity orientation (front (0,0,1)), blend (ONE, SRC_COLOR), depth writes off | same |
| 1c | …Started HotFeet, Booster, Subversion, Plague, Extinguisher, MeteorShower, Rage, Nuke | glowing area cylinder at the cursor (`RenderCyliner`, 100 sides, attached to ground, up = world up), team colour, alpha halved during the "off" phase of the 0.4 s blink. Radius/height: default 70 / 42 wu; MeteorShower 280 / 70; Nuke 210 / 70 | same (radii = `POWERUP_EFFECT_RANGE` ×1, ×4, ×3; height ×0.6 or ×1) |
| 2 | No task, selected entity is an Officer | `icons/icon_officer.bmp` | `RenderSelectedUnitIcon` |
| 3 | Darwinians selected | `icons/icon_darwinian.bmp` + count (font 15, outline then 0.9 white) at anchor+50 px+(2, 0.4·size); if `RoutingSystem::m_walkableRoute` is false, the red disabled sprite replaces the count ("no route") | `RenderSelectedDGsIcon` |

### 4.6 Selection circle (Darwinian group selection) visual

[C] `RenderSelectionCircle`: while `Team::m_selectionCircleCentre ≠ 0`, every frame create 30 `Particle::TypeControlFlash` particles, size = current radius, evenly spaced on the circle of `m_selectionCircleRadius` (max 100 wu, grown by `LocationInput`), rotating at 0.5 rad/s (angle offset = wall time·0.5), 10 wu above the terrain.

### 4.7 "Trying to enter" effect

[C] `RenderTryingToEnterEffect` when `m_objUnderMouse` is: an Armour while Darwinians are selected (valid if passengers < capacity), or a RadarDish while Darwinians or an Officer are selected (entrance from `RadarDish::GetEntrance`, valid if `Connected()`). Draw `m_cursorPlacement` at the projected entrance, animated only if valid; if invalid also draw `m_cursorDisabled` (red, 60 px) on top.

### 4.8 Crosshair (`RenderCrosshair`)

Returns false (→ standard pointer) if any window is open, no team, or nothing building/unit selected. Rules [C]:

| Selected | Crosshair | Size |
|---|---|---|
| Own GunTurret building | `m_cursorTurretTarget` at the projected `GunTurret::m_actualTargetPos`, smoothed (exp. lerp, factor 10·dt) — only when a target pos exists; the standard pointer is suppressed either way | 1.5·screenH/10 |
| Any other selected building (in MP: RadarDish) | turret-target sprite at the projected 3D mouse position | screenH/10 |
| InsertionSquad unit in gamepad mode with a tracked entity | nothing drawn (early `return true` — comment: "doesn't work") | – |
| InsertionSquad unit in gamepad mode without tracking | turret-target at projected mouse | screenW/25.6 |
| Any other unit (keyboard) | none (standard pointer) | – |

**Defect**: the RadarDish test is written `building->m_type = Building::TypeRadarDish` (assignment) [C] — any selected non-turret building has its **type overwritten to RadarDish on the local client** while the crosshair renders. In Multiwinia only GunTurret, RadarDish and ChessPiece can be selected, and ChessPiece is blocked (`BLOCK_OLD_DARWINIA_OBJECTS`), so the write is a no-op in practice — but it is a render→sim write (see §6).

### 4.9 Formation preview (`RenderFormation`)

For i in 0..`Team::m_evolutions[EvolutionFormationSize]`−1: position = `Officer::GetFormationPositionFromIndex(pos, i, front, size)` (P03); a camera-facing diamond (vertices ±2·camUp, ±2·camRight) textured `textures/particle.bmp`, team colour, alpha blend; near plane ×1.03 [C].

### 4.10 Selection arrows, off-screen arrow, markers [B:LEGACY] in MP

- `BoostSelectionArrows(s)`: `m_selectionArrowBoost = max(current, s)`; called with 2.0 by `TaskManager::SelectTask` and `Team::SelectUnit` for the local team [C] — but arrows are drawn only by the SP render path, so in MP the call has no visible effect.
- `RenderSelectionArrows(id, pos)` (SP): on-screen → four `icons/selectionarrow.bmp` triangles (size 40) at distance 1000/√camDist around the object, alpha = max(0.1, min(boost, 0.9), clamp((camDist−200)/200, ·, 0.9)), boost decays 0.4/s; team colour in MP, (255,255,75) in SP; off-screen → a 60 px arrow at the screen edge (`FindScreenEdge`) pointing toward the object; also `RenderOffscreenArrow` (dotted particle path from object to mouse, darkened ×0.2 if not walkable; disabled in MP).
- `CreateMarker(pos)` (move-order confirmation) is called by `Location` for the local team when orders are given, but `RenderMarkers` (which shrinks a 3D placement marker from 20 to 0 over 0.5 s and deletes it) is only called by the SP path → in MP the marker list **grows without bound** (leak) [C] `code/location.cpp:Location::IssueDarwinianOrders`, `GameCursor::RenderMarkers`.
- `RenderWeaponMarker` (animated, shadowed 3D placement marker, size 40): no callers found [I].

### 4.11 SP cursor state machine [B:LEGACY] (for completeness)

`Render()` non-MP branch [C]: editing/window → none; chat log → none; non-interactive camera (cut-scene) → none; global world → placement cursor (animated if the highlighted location is available, else + disabled); at a location: TM visible → selection arrows + selection sprite; placing a non-Officer task → placement cursor (animated if valid, else + disabled; sets `m_validPlacementOpportunity`) plus area sparkles (`Particle::TypeSelectionMarker`, 10 or 30 around radius 70/140/420) or ghost turret/anthill; entity-track camera → turret target when firing; otherwise highlight sprite, armour-enter cursor, move-here arrow rotated toward the selected unit, building crosshair (200 px), Darwinian-promote highlight, landscape highlight cursor. Advisories `AdviseHighlightingSomething`, `AdvisePlacementOpportunity`, `AdviseMoveableEntitySelected` are only updated in this branch (so in MP they keep their initial `false`) — consumed by `ControlHelpSystem` (P10).


---

## [→ 24_rendering] 4R. Task-manager / cursor overlay render states (summary)

All drawing is immediate-mode OpenGL (emulated by the D3D9 layer under `USE_DIRECT3D`), no shaders [C]. Render order inside a frame: … `MarkerSystem::Render` → `MultiwiniaHelp::Render` → `ShamanInterface::Render` → `UserInput::Render` → `GameCursor::Render` (3D world-space highlights) → `GameCursor::RenderBasicCursorAlwaysOnTop` (2D icon, enter marker, crosshair/pointer) → chat → `Camera::Render` → `TaskManagerInterface::Render` (target-area particles, then 2D bar/pop-ups/messages) → debug renderer [C] `code/renderer.cpp`.

| Pass | Space / projection | Blend (src, dst) | Depth | Texture (filter) | Ref |
|---|---|---|---|---|---|
| Cursor shadow | 2D screen px (or world quad) | SRC_ALPHA, ONE_MINUS_SRC_COLOR with colour (1,1,1,0) | write off | `shadow_<cursor>` = cursor BMP blurred r=10 | `MouseCursor::Render` |
| Cursor main | same | SRC_ALPHA, ONE (additive) or SRC_ALPHA, ONE_MINUS_SRC_ALPHA (`m_additive=false`: turret crosshair) | write off | cursor BMP | same |
| Highlight rings | 3D, near plane ×1.03 | SRC_ALPHA, ONE_MINUS_SRC_ALPHA | write off | none | `RenderUnitHighlightEffect` |
| Glow / area cylinder | 3D | alpha blend, smooth shading, top alpha 0 | write off (highlight) | none | `RenderCyliner` |
| Ghost turret/anthill | 3D model at cursor | ONE, SRC_COLOR | write off | model textures | `RenderSelectedTaskIcon` |
| Formation dots | 3D billboards | alpha blend | – | `textures/particle.bmp` | `RenderFormation` |
| Task bar shadow | 2D ortho 600-high | alpha blend | write off | `icons/marker_shadow.bmp` (alpha) | `RenderRunningTasks` |
| Task bar colour disc / icon | same | additive | write off | `icons/marker_colour.bmp` (alpha), `icons/icon_*.bmp` | same |
| Compass | same | shadow alpha-blend, arrow additive | – | `icons/compass_shadow.bmp`, `icons/compass.bmp` (no mipmap) | `RenderCompass` |
| Pop-up box | same | alpha blend; gradient with smooth shading; crate icon additive, clipped to the box via `Renderer::Clip` ([I] scissor) | – | `icons/icon_crate.bmp` | `RenderRecentRunTask` |
| Text | same | fonts' own (outline pass with colour alpha 0, then fill) | – | `g_titleFont`, `g_gameFont`, `g_editorFont` | `RenderMessages`, `RenderCurrentCrateHelp` |
| Gesture trail [B:LEGACY] | full-screen quad | additive ×2 (nearest then linear) | – | dynamic 64×64 RGBA, blurred 1.4 | §3.9 |

Notes: [I] icon BMPs carry no alpha (24-bit BMP; `TEXTURE_EXTENSION` = "bmp"; only `marker_*` and `compass_shadow` are loaded with `GetTextureWithAlpha`), so black reads as transparent through additive blending, and "shadow" passes darken via ONE_MINUS_SRC_COLOR [C for the blend modes]; a new renderer must either keep additive icon art or author RGBA art. UI coordinates are resolution-independent via the 600-unit-high orthographic canvas (`gluOrtho2D(0, 600·aspect, 600, 0)`), cursors are in physical pixels (sizes in px, e.g. 35 px pointer regardless of resolution) [C].

## [→ 06_player_control_and_input] 5. Constants

| Constant | Value | Kind | Ref |
|---|---|---|---|
| `POWERUP_EFFECT_RANGE` | 70.0 wu | `#define` | `code/taskmanager.h` |
| Squad task life | 60.0 s (MP only counts down) | literal | `Task::Start` |
| Booster / HotFeet / Subversion / Rage life | 30.0 s | literal | `Task::Start` |
| Tank life (non-TankBattle) | 30.0 s | literal | `Task::Start` |
| Squad size | 2 + research level(Squad); MP level = 1 → 3 | formula / data | `Task::TargetSquad`; `code/location.cpp:Location::Init` |
| MP research levels | all 3, except Squad 1, Laser 2, Grenade 2, Rocket 2, AirStrike 2, Darwinian 4 | literal | `Location::Init` |
| Squad spawn spread | 10 wu | literal | `Task::TargetSquad` |
| Engineer / Harvester spawn height offset | +10 / +50 wu | literal | `TargetEngineer`, `TargetHarvester` |
| DropShip entry | map corners inset 100 wu, height 500 wu | literal | `TargetDropShip` |
| Promote/demote flash particles | 5 + rand%5, vel (±5, 0..15, ±5) | literal | `Task::Promote`, `Demote` |
| FindDarwinian radius | 10.0 wu | literal | `Task::FindDarwinian`, `IsValidTargetArea` |
| Capacity MP | 6 (2 when eliminated) | literal | `TaskManager::Capacity` |
| Capacity SP | 1 + research level(TaskManager) | formula | same |
| Turret placement exclusion | building sphere 10 wu; enemy TrunkPort 400 wu; enemy SpawnPoint 200 wu; restriction zones | literal | `IsValidTargetArea` |
| Target-area radii | squad-anchored 100 wu; building-anchored 75 wu; open trunk port 120 wu (SP); "anywhere" 99999.9 | literal | `GetTargetArea` |
| Turret limit | game option `MaxTurrets` (Rocket Riot default 3, range 0–5; Blitzkrieg default 2, range 0–5; other modes −1 = no limit) | data | `data/multiwinia.txt` |
| Starting power-up lock | 30.0 s (`m_startTimer`) | literal | `Location::InitialiseTeam` |
| Retribution grant interval | 45 s (eliminated teams) | literal | `Multiwinia::AdvanceRetribution` (P02) |
| Message durations | Success 2.5 s, Failure 2.5 s, Shutdown 3.0 s | literal | `taskmanager.cpp`, interfaces |
| Recent-task pop-up | show 3.0 s, minimise 1.0 s; SP clear after 1.0 s | literal | `TaskManager::Advance`, `RenderRecentRunTask` |
| Pad "hold Back" threshold | 0.5 s | literal | `TaskManagerInterfaceMultiwinia::ButtonHeld` |
| Virtual UI canvas | height 600, width 600·aspect | literal | `SetupRenderMatrices` |
| MP task bar | icon 60→55, gap 10, y 550, selected ×1.1 / others ÷1.1, ease factor min(dt,0.1)·10 | literal | `RenderRunningTasks` |
| Compass size | 0.77·icon (MP), 0.75·icon (Icons), 53 (Gestures) | literal | `RenderCompass` callers |
| Tooltip typewriter | 50 chars/s | literal | `RenderTooltip` |
| Centre message | y = 370, size 40 (+15·√(2−t) last 2 s) | literal | `RenderMessages` |
| Crate help box | width 210, y 420, font 11 | literal | `RenderCurrentCrateHelp` |
| Screen-edge scroll border | 10 px | literal | `AdvanceScreenEdges` |
| Gesture constants | §3.2 | `#define` | `code/gesture.h` |
| Cursor sizes / hotspots | §4.1 | literal | `GameCursor::GameCursor` |
| Cursor pulse | +60 % at |sin(4·gameTime)| | literal | `MouseCursor::GetSize` |
| Shadow blur radius | 10.0 (cursors, selection arrow) | literal | `MouseCursor::MouseCursor` |
| Highlight ring widths | 0.2·r, 0.075·r; min 25 steps | literal | `RenderUnitHighlightEffect` |
| Land-follow factor | min(1, 75 / camDist), threshold 0.1 | literal | `RenderRing`, `RenderCyliner` |
| Placement preview radii | 70/42 (default), 280/70 (meteor), 210/70 (nuke) wu | formula | `RenderSelectedTaskIcon` |
| Selection-circle visual | 30 particles, 0.5 rad/s, +10 wu | literal | `RenderSelectionCircle` |
| Selection circle growth | +100 wu/s up to 100 wu | literal | `code/location_input.cpp` (P01) |
| Darwinian order rate limit | 1.0 s | literal | same |
| Highlight sound rate limit | 0.2 s | literal | `GetHighlightedObject` |
| Order marker life (SP) | 0.5 s, size 20→0 | literal | `RenderMarkers` |

---

## 6. Determinism notes for X3 (local UI ↔ simulation)

Tasks are simulation state replicated by lockstep command execution; everything below is where local, per-client UI state touches or could touch it.

| # | Finding | Severity | Ref |
|---|---|---|---|
| D1 | **Wall-clock in sim objects**: `TaskManager::RunTask` stores `GetHighResTime()` in `m_mostRecentTaskIdTimer` and `TaskSource::m_timer`; `TaskManager::Advance` (sim tick) edits/removes `m_recentTasks` by wall time. Only UI reads these, so harmless, but they live inside the replicated object. New design: keep pop-up state in a client-side view model. | low | `TaskManager::RunTask`, `Advance` |
| D2 | **UI writes a sim field**: `TaskManagerInterfaceMultiwinia::Advance` sets `tm->m_mostRecentTaskId = -1` on the owning client only. Sim code reads `m_mostRecentTaskId` right after `RunTask` (crate turret/napalm option, instant units, AI officer) — safe only because those reads follow a *successful* `RunTask` in the same tick (AI teams are never "my team" [I]). Any future read after a failed `RunTask` desyncs. | medium (latent) | `taskmanager_interface_multiwinia.cpp:Advance`; `crate.cpp:CaptureCrate`; `ai.cpp` |
| D3 | **Local (non-networked) terminations**: `RunScreenZone("DeleteTask")` calls `TerminateTask` directly. In MP it is reachable only via `ControlTaskManagerEndTask` (unbound) or delete zones (only drawn when the overlay is visible, which never happens in MP). Gestures interface `AdvanceTerminate` is also local (SP only). If either were reachable in MP it would desync immediately. | medium (latent) | `RunScreenZone`, `TaskManagerInterfaceGestures::AdvanceTerminate` |
| D4 | **Render writes sim state**: `GameCursor::RenderCrosshair` assigns `building->m_type = TypeRadarDish` for any selected non-turret building (typo for `==`). No-op in MP today (only RadarDish/GunTurret selectable). | medium (latent) | `GameCursor::RenderCrosshair` |
| D5 | **Sim calls UI singletons**: `TaskManager::SelectTask` → `GameCursor::BoostSelectionArrows`; `Task::*` → `TaskManagerInterface::SetCurrentMessage`, sound events, camera requests (`RequestEntityTrackMode`, `RequestMode`) and `Location::SetCurrentMessage` — all guarded by `m_teamId == m_myTeamId` or purely cosmetic; deterministic sim must route these through an event queue. | low | `taskmanager.cpp` throughout |
| D6 | **Non-synced RNG in sim**: `Task::Promote`/`Demote` use `AppRandom`/`frand`/`sfrand` (non-synchronised) for particle count/velocity. Safe only if the particle system never feeds back into simulation (P12/P03 to confirm). | low | `Task::Promote` |
| D7 | **8-bit task ids on the wire** while ids are unbounded ints — after 256 tasks per team, Select/Target/Terminate address the wrong task (identically on all peers, so no desync, but wrong behaviour); id 255 is untargetable for terminate (sentinel). | medium | `ClientToServer::Request*Program`; update handler |
| D8 | Selection affects sim: `Team::SelectUnit` / `m_currentTaskId` are replicated via commands (`SelectUnit`, `SelectProgram`), and `Task::SwitchTo` sets `m_underPlayerControl` on Darwinians (sim state) — correct because it runs from the command stream on all peers. The client must never call `SelectTask` locally in MP (SP code paths do). | info | `Task::SwitchTo` |
| D9 | `NET_DARWINIA_TOGGLEENTITY` handler (officer branch) calls `RequestSelectUnit` from inside command execution on **every** peer → N duplicate SelectUnit commands for the officer's team (idempotent). | low | `code/network/clienttoserver.cpp` (P15) |
| D10 | `LocationInput` writes `Team::m_selectionCircleCentre/Radius` and `m_lastOrdersSet` on the local client only, and `FenceSwitch::Switch()` is called locally on click (blocked type in MP). Whether any sim code reads the selection-circle fields must be checked by P01/X3 [U]. | unknown | `code/location_input.cpp` |
| D11 | `GameCursor::CreateMarker` is invoked from sim order processing for the local team (UI side effect from sim; in MP also an unbounded list). | low | `Location::IssueDarwinianOrders` |
| D12 | Floating point: gesture features use CRT `atan` and `iv_sqrt/iv_atan` (nibble-truncated); irrelevant to sync (local UI only). | info | `Gesture::CalculateFeatureSet` |

---

## Cross-partition notes

- **P08 (crates/markers)**: crate rewards → `TaskManager::RunTask(type)` and `m_option` assignment (napalm = 1; turret types) [C] `code/worldobject/crate.cpp:CaptureCrate()`, `CaptureCrate(int,int)`; effect bodies `Crate::Run*` (AirStrike, Nuke, Subversion, Booster, HotFeet, GunTurret, Infection, MagicalForest, DarkForest, AntNest, Plague, Eggs, MeteorShower, Rage) are called by `Task::Target*`. `MarkerSystem::RegisterMarker_Task(objId)` is called on successful targeting (not for turrets); `MarkerSystem` calls back `TaskManagerInterfaceMultiwinia::RenderExtraTaskInfo` for world markers.
- **P02 (game modes)**: reinforcement armour/turret/air-strike tasks and retribution grants with `NotifyRecentTaskSource(1|2)`; `m_blockMostRecentPopup` is written but has no reader; `MaxTurrets`, `MaxArmour`, `StartingPowerups` options; `IsEliminated` drives capacity 2.
- **P01 (app/input/team)**: `LocationInput` owns Darwinian selection circle, promotion, officer/armour orders, placement click (`RequestTargetProgram`), entity terminate; `Team::SelectUnit` (selection state, `m_blockUnitChange`); `Team::Advance` slice 1 advances the task manager; `Team` elimination calls `StopAllTasks`.
- **P03 (entities)**: `Darwinian` fields `m_promoted`, `m_controllerId`, `m_hotFeetTaskId`, `m_boosterTaskId`, `m_subversionTaskId`, `m_rageTaskId`, `m_underPlayerControl`, `LoseControl()`; `Officer::IsFormationToggle`, `IsInFormationMode`, `GetFormationPositionFromIndex`, `CancelOrders`, `SetOrders`; `InsertionSquad::SetWeaponType`; `Engineer::GetCurrentAction/GetNumSpirits`; `Harvester::m_vunerable`; `Armour` states/`m_conversionOrder`/passengers.
- **P04**: `ShamanInterface::ShowInterface/HideInterface/IsVisible` (Tab with a Shaman selected; Shaman tasks can't be terminated).
- **P10 (input/camera/control help)**: binding names in §2.3.2/§2.6; `ControlHelpSystem` consumes the four `Advise*` methods and `GameCursor::Advise*`; camera modes `ModeEntityTrack`, `ModeFreeMovement`, `ModeRadarAim`, `ModeTurretAim`.
- **P15 (network)**: command strings `RunProgram`, `TargetProgram`, `SelectProgram`, `Terminate`, `SelectUnit`, `ToggleEntity`, `Promote`; uchar program field width (§1.9).
- **P12/P13 (rendering)**: all TM/cursor drawing is immediate-mode GL in a 600-high ortho canvas; blend modes listed per element; `MarkerSystem::RenderShadow/RenderColourCircle/RenderIcon` used by the cursor icon.
- **P19 (tutorial)**: `g_app->m_multiwiniaTutorial`, `m_multiwiniaTutorialType`, `MultiwiniaHelp::m_tutorial2AICanSpawn` block termination; MultiwiniaHelp reads `m_mostRecentTaskId` and `GameCursor::GetHighlightedObject/m_objUnderMouse`.

## Contradictions / open questions

1. `GameCursor::RenderSelectionArrows` and `BoostSelectionArrows` are wired from sim code but never drawn in MP — dead feedback path in the shipped game [C].
2. The MP task bar draws only occupied slots although `Capacity()` is computed; the Icons/Gestures variants draw empty slot shadows for every capacity slot [C]. Design intent for MP slot visibility is unclear [I].
3. `TaskManagerInterfaceMultiwinia` contains a full overlay implementation (delete icons, pad zone navigation, scroll zones 1/3) that can never become visible in MP [C] — remnants of the SP Icons interface.
4. `Task::m_route` and the Controller task creation are commented out; `TypeController` survives only as a squad weapon and as a fall-through branch in `Advance` [C].

## [U] Unknowns

| # | Unknown | What would resolve it |
|---|---|---|
| U1 | Whether `Team::m_selectionCircleCentre/Radius` (written locally by UI) are read by simulation code | grep/read of P01/P03 Darwinian selection handling (`NET_DARWINIA_SELECTDARWINIANS` handler) |
| U2 | Whether particles created from sim code (`Task::Promote`) can affect simulation | P12 particle-system review |
| U3 | Callers of `GameCursor::RenderWeaponMarker`, `Task::Demote`, `TaskManager::GetNextTaskId` in live code (none found by grep in `code/`) | full-tree reference search by X-agents |
| U4 | Exact semantics of `RestrictionZone::IsRestricted(pos, true)` used by turret/anywhere placement | P07/P11 restriction-zone documentation |
| U5 | Visual appearance of icon/cursor BMPs (only file names known) | asset review (art IP — replace anyway) |

## Docs-vs-code discrepancies

- `docs/betachanges.txt` lists fixes for "Gestures Mode", "Icons task manager used when use Xbox 360 controller" and Tab in Icons mode — these concern the Darwinia(+) SP interfaces; code confirms both SP variants still exist but are unreachable in Multiwinia (FULLGAME has no Prologue/Campaign entry) [C]. No Multiwinia-specific task-manager documentation found in `docs/` (grep for "task manager", "gesture").
- `docs/game objects.txt` suggests power-ups "triggered by a gesture of some kind"; in Multiwinia power-ups are placed by click, never by gesture [C] `TaskManagerInterface::CreateTaskManager`.

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

## Appendix — dead/legacy code

- **[B:LEGACY] `TaskManagerInterfaceGestures`, `TaskManagerInterfaceIcons`**, SP branches of `GameCursor::Render`, SP target areas (ControlTower, open TrunkPort), SP auto-select on `RunTask`, SP `Capacity`, research screens, objectives screens, quick-unit d-pad creation (`ControlIconsTaskManagerQuickUnitLeft/Right/Up` → Engineer/Armour/Squad; `…Down`/`Left` officer none/follow) [C].
- **[B:LEGACY]** `RegisterTask` user: saved running programs (`Location` load) [C] (the Teleport/RadarDish user is live, §1.5).
- **[B:DEAD] `GestureDemo`** (`code/gesture_demo.{h,cpp}`, all under `USE_SEPULVEDA_HELP_TUTORIAL`): loads `data/gesture_demos/<name>` (one "x y" pair per line, normalised by screen height, 40 ms per sample = 25 Hz); `GetTotalDuration = count·0.040 s`; `GetCoords(t)`: index = t·25, interpolates between floor/ceil samples — **with the weights swapped** (fractional part weights the *lower* sample), clamps to the last sample, scales by screen height. Played by `Sepulveda::RenderGestureDemo`: 40 px wide GL lines in 0.04 s steps, colour fades with age (2 s), loops 2 s after the end, a mouse cursor drawn at the pen tip [C] `code/sepulveda.cpp` (dead). Recorder `GestureRecordingThread` samples `g_target` every 40 ms while `ControlGestureActive` and writes `gesture_demos/new_gesture_demo.txt` (`%.3f %.3f`).
- **[B:DEAD] `code/gesture_demo_win32.cpp`**: `RecordGestureDemo()` spawns the recorder on a `THREAD_PRIORITY_TIME_CRITICAL` Win32 thread; requires `GESTURE_EDITOR` (TARGET_DEBUG) **and** `USE_SEPULVEDA_HELP_TUTORIAL` (never) [C].
- **[B:DEAD]** Gesture editor window `code/interface/gesture_window.cpp` (same double gate).
- **[B:D3D]** `TaskManagerInterfaceGestures::ReleaseD3dResources()` frees the gesture bitmaps/textures on device loss (`USE_DIRECT3D`) [C].
- Commented-out code: Controller task creation, squad weapon cycling in `LocationInput`, `TargetExtinguisher`, `TargetTank`, Shaman game type start task, MP "special" Darwinian/officer pseudo-slots in the task bar, keyboard F-key shortcuts in the MP interface.
