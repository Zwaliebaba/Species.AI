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
| 1 | TypeOfficer | Officer | yes | – | Promote nearest own Darwinian within 10 wu to Officer, then **terminate own task** (§1.6) | never (state forced back to Started each tick) | – | AI only (`ai.cpp` RunTask Officer); player promotion is a direct order, see §2.7 |
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

`RegisterTask(Task*)`: capacity-checked append without `Start()` (used for restoring saved Darwinia programs and teleport transfers — [B:LEGACY]) [C] `TaskManager::RegisterTask`; `code/location.cpp`, `code/worldobject/teleport.cpp`.

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

- `SelectTask(int id)` [C]: `m_currentTaskId = id`; if ≠ -1: `m_previousTaskId = id`; find task (return silently if missing); `task->SwitchTo()`; `GameCursor::BoostSelectionArrows(2.0)` (UI-only effect called from sim — see §5.3).
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
| Create task (SP/legacy only; MP UI never creates) | Icons/Gestures/Multiwinia `RunScreenZone("NewTask"/"SelectWeapon")`; `LocationInput` quick-unit d-pad (SP); squad weapon cycling (`location_input.cpp`) | `RunProgram` | `TEAMID` (uchar), `PROGRAM` (uchar task type) | `team->m_taskManager->RunTask(program)` |
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
    UI->>Net: Tab / key 1-9 / click icon → RequestSelectProgram(team, taskId)
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
| E1 | Multiwinia tutorial: `TerminateTask` refuses, so one-shot tasks (Nuke, Infection, forests, AntNest, Plague, Eggs, Meteor) whose `Target` relies on self-termination remain `StateStarted` and **can be fired again**; AirStrike sets Running first and is then removed by `Advance` when the effect object disappears. | `Task::TargetNuke` etc., `TaskManager::TerminateTask` |
| E2 | `Task::Stop()` refusing (`stop=false`) does not stop `TerminateTask` from deleting the task — the armour/turret survives without a slot. | `TaskManager::TerminateTask` |
| E3 | Extinguisher and Tank `Target*` bodies are commented out: such a task would stay Started forever (not reachable in FULLGAME). | `Task::TargetExtinguisher`, `Task::TargetTank` |
| E4 | `Capacity()` in MP ignores research; an eliminated team gets 2 slots (for retribution powers). | `TaskManager::Capacity`; `Multiwinia::AdvanceRetribution` checks `CapacityUsed() < Capacity()` before granting |
| E5 | Squad life timer only counts down in MP and only while `> 0`; instant-unit squads get `m_lifeTimer = -1` (permanent). | `Task::Advance`, `Location::InitialiseTeam` |
| E6 | `StartingPowerups` game option: each team gets N random crate rewards at team init (excluding Randomiser, CrateMania, Darwinians, Bitzkreig, SpawnMania, Plague, RocketDarwinians, SpaceShip, Spam, Triffids, SpeedUpTime, SlowDownTime, Forest, TheBeast and invalid ones); then every non-Shaman task gets `m_startTimer = 30.0` s (locked, greyed, countdown shown). | `code/location.cpp:Location::InitialiseTeam` |
| E7 | `TaskManager::Advance` calls `GlobalResearch::AdvanceResearch()` once per team per tick (research is a single global object). | `TaskManager::Advance` |

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
| U7 | **Focus highlight** | `m_currentScreenZone` valid (pad navigation; mouse hover only while visible) | zone grown by 10 % of its height on every side | MP: `icons/mouse_highlight.bmp`, additive, a **square** of side h at (x,y) | `RenderScreenZones`, `RenderScreenZoneHighlight` |
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
| `ControlActivateTMButton` | mouse left up; pad A up; Enter down | Activates `m_currentScreenZone` (not when the focus is only a world-hover echo) | `AdvanceScreenZones` |
| `ControlIconsTaskManagerDisplayUp` (after Back held > 0.5 s) | pad Back up | Activates focused zone ("hold Back, release on a button") | `ButtonHeld`, `ButtonHeldAndReleased` |
| `ControlMenuUp` / `ControlMenuDown` | pad d-pad/sticks up/down (repeat) | Only while visible (never in MP): move focus within current scroll zone, wrapping | `AdvanceScreenZones` |
| `ControlMenuLeft` / `ControlMenuRight` | pad d-pad/sticks left/right (repeat) | Only while visible: switch scroll zone 1↔2, 2→3 when the zone has sub-zones, step through zone 3 | `AdvanceScreenZones` |
| `ControlCloseTaskHelp` | read but body commented out | no effect | `Advance` |

`RunScreenZone(name, data)` actions in this variant [C]: `NewTask` (refused with `MessageFailure` 2.5 s while another non-Officer task is Started; else if research owned → `RequestRunProgram` + sound `GestureBegin`; pad → hide), `SelectTask` (above), `DeleteTask` (local `TerminateTask(data or current)` + sound `DeleteTask` — only reachable via zones rendered while visible or the unbound key), `SelectWeapon` (research owned → `RequestRunProgram(weapon)` + `GestureBegin` + `GestureSuccess`), `Research` (`SetCurrentResearch`), `Objective` (data = ±(index+10); runs cut-scene script or default objective; hides). In MP only `SelectTask` zones are ever generated (by the bar, scroll zone 2) [C] `RenderRunningTasks`.

**World-hover echo**: when hidden, if the object under the mouse (`LocationInput::GetObjectUnderMouse`) is the `m_objId` of one of my tasks, the matching `SelectTask` zone is marked "highlight only" (clicking it does nothing; it prevents a world click from also activating the zone) [C] `AdvanceScreenZones`. `m_highlightedTaskId` = task of the focused `SelectTask` zone.

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

Elements: title "taskmanager_taskmanager" size 50; grey title bar 100 px high spanning 3 screens; silver dividers; central red panel `textures/interface_red.bmp` multiplied by the alpha mask `icons/gesture_alpha.bmp` (red channel → alpha, two texture units, GL_COMBINE); **gesture guide** `icons/gestureguide.bmp` 320×320 centred at (W/2, H/2 − 10), additive 0.7; running-task row at y=490 (icon 70, shadow 78, gap 18, all capacity slots; Started blink; current alpha 1.0, visible 0.5, hidden 0.3; engineer/squad info; compass size 53); overview tabs (research / task manager / objectives) at y=95, 100×16 boxes; objectives and research screens as in Icons (research rows also show `icons/gesture_<Name>.bmp` for every type except Darwinian, Laser, TaskManager); live gesture trail (§3.7) [C] `Render*`.

### 2.6 Multiwinia player-action reference (cross-partition summary; primary owner P01 `code/location_input.cpp`, P10 input)

Listed so 06 has one table; evidence [C] `code/location_input.cpp:LocationInput::AdvanceTeamControl`, `AdvanceNoSelection`, `SelectObjectUnderMouse`, `GetObjectUnderMouse`, bindings `data/input/input_preferences.txt`. "MP" gates: most world actions are suppressed during the grace period, game over and pause.

| Action | Mouse/keyboard | Pad | Condition | Network command | Notes |
|---|---|---|---|---|---|
| Select object under cursor | left down (`ControlUnitSelect`) | A down | overlay hidden; pick priority: marker icon under mouse, else nearest of entity/unit/building along the click ray; buildings only RadarDish, GunTurret, FenceSwitch (own or neutral) | `SelectUnit` | RadarDish (unless level option NoRadarControl) → camera radar-aim; own GunTurret/ChessPiece → camera turret-aim; FenceSwitch → `Switch()` called **locally** (blocked type in MP). With pref `ControlRemap == 0` nothing new can be selected while something is selected. Clicking empty ground with something selected (MultiwiniaControlMethod 1) → "MouseError" sound + Space-to-deselect help |
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

