# P02_game_modes — Multiwinia game rules and mode objects

Scope: `code/multiwinia.{h,cpp}`, `code/achievement_tracker.{h,cpp}`, `code/rocket_status_panel.{h,cpp}`, `code/worldobject/{multiwiniazone,statue,flag,rocket,trunkport,pulsebomb,controlstation}.{h,cpp}`; data `data/multiwinia.txt` (+ variants `data/multiwinia-testbed.txt`, `data/multiwinia_betatesters.txt`), `data/levels/level_enum.txt`, MultiwiniaOptions/Buildings sections of `data/levels/mp_*.txt` (confirmation only; X4 owns the level catalogue). Build baseline: see build_matrix.md (`Release|Win32` + `TARGET_PC_FULLGAME`).

Conventions used below:
- "tick" = one server advance = one call of `Multiwinia::Advance` / `Building::Advance`, nominally 0.1 s of game time. `SERVER_ADVANCE_PERIOD` ≡ `0.1 * g_gameTimer.m_gameSpeed` [C] `code/gametimer.cpp:GameTimer::GetServerAdvancePeriod`, `code/gametimer.h`.
- "game second" = `GetNetworkTime()` ≡ `g_gameTimer.GetGameTime()` units [C] `code/gametimer.h`.
- "mode second" = one period of the `m_timer`/`m_reinforcementTimer`/`m_positionTimer` 1.0-second countdowns. **These last 11 ticks (1.1 game seconds), not 10** — see §8 Determinism D1 [C by arithmetic on the code path].
- Team ids: `NUM_TEAMS = 6` [C] `code/globals.h`; player teams 0–3; team 4 = "monster" team (Evilinians), team 5 = Futurewinians, both only when `Location::m_allowMonsterTeam` [C] `code/location.cpp:Location::GetMonsterTeamId`, `GetFuturewinianTeamId`. Team id 255 = neutral/unowned.

---

## [→ 01_game_modes] 1. Game-type catalogue and build gating

### 1.1 Enumerations (verbatim values)

`Multiwinia::GameType*` [C] `code/multiwinia.h:Multiwinia` (anonymous enum):

| Value | Enum | Blueprint name in `data/multiwinia.txt` (matched by `GameTypes` in level files) | UI string key → English | Complexity key (`MultiwiniaGameBlueprint::GetDifficulty`) → English | Advance function | Build macro | Status in baseline |
|---|---|---|---|---|---|---|---|
| 0 | `GameTypeSkirmish` | `Domination` | `multiwinia_gametype_domination` → "Domination" | `multiwinia_complexity_basic` → "Simple" | `AdvanceSkirmish` | `INCLUDEGAMEMODE_DOMINATION` (+ body also inside `#ifndef MULTIWINIA_DEMOONLY`) | **[B:FULL]** |
| 1 | `GameTypeKingOfTheHill` | `KingOfTheHill` | `multiwinia_gametype_kingofthehill` → "King Of The Hill" | basic → "Simple" | `AdvanceKingOfTheHill` | `INCLUDEGAMEMODE_KOTH` | **[B:FULL]** (also DEMOONLY, PC_PREVIEW, HWCOMPAT) |
| 2 | `GameTypeCaptureTheStatue` | `CaptureTheStatue` | `multiwinia_gametype_capturethestatue` → "Capture The Statue" | intermediate → "Medium" | `AdvanceCaptureStatue` (+ `EnsureStartingStatue` in grace) | `INCLUDEGAMEMODE_CTS` | **[B:FULL]** |
| 3 | `GameTypeAssault` | `Assault` | `multiwinia_gametype_assault` → "Assault" | intermediate → "Medium" | `AdvanceAssault` | `INCLUDEGAMEMODE_ASSAULT` + `#ifndef MULTIWINIA_DEMOONLY` | **[B:FULL]** |
| 4 | `GameTypeRocketRiot` | `RocketRiot` | `multiwinia_gametype_rocketriot` → "Rocket Riot" | advanced → "Hard" | `AdvanceRocketRiot` (+ `AdvanceTrunkPorts`) | `INCLUDEGAMEMODE_ROCKET` + `#ifndef MULTIWINIA_DEMOONLY` | **[B:FULL]** |
| 5 | `GameTypeBlitzkreig` (sic) | `Blitzkrieg` | `multiwinia_gametype_blitzkrieg` → "Blitzkrieg" | advanced → "Hard" | `AdvanceBlitkrieg` (sic) (+ `AdvanceTrunkPorts`) | `INCLUDEGAMEMODE_BLITZ` + `#ifndef MULTIWINIA_DEMOONLY` | **[B:FULL]** |
| 6 | `GameTypeTankBattle` | `TankBattle` | none in english.txt (falls back to raw name "TankBattle") | intermediate → "Medium" | `AdvanceTankBattle` | `INCLUDEGAMEMODE_TANKBATTLE` + `#ifndef MULTIWINIA_DEMOONLY` | **[B:DEBUG]** only (macro commented out in FULLGAME) |
| 7 | `NumGameTypes` | — | — | — | — | — | — |

Evidence: names [C] `data/multiwinia.txt`; string ids built as `"multiwinia_gametype_%s"` lower-cased [C] `code/multiwinia.cpp:MultiwiniaGameBlueprint::MultiwiniaGameBlueprint`; English text [C] `data/language/english.txt`; complexity [C] `code/multiwinia.cpp:MultiwiniaGameBlueprint::GetDifficulty`; dispatch [C] `code/multiwinia.cpp:Multiwinia::Advance`; gating [C] `#ifdef`s in each `Advance*` and `code/app.cpp:App::IsGameModePermitted`.

The blueprint index is the order of `GAMETYPE` blocks in `data/multiwinia.txt` (`s_gameBlueprints.PutData` in file order) and is used directly as the game-type id, so the data file order **must** match the enum order [C] `code/multiwinia.cpp:Multiwinia::LoadBlueprints`, `Multiwinia::SetGameOptions` (indexes `s_gameBlueprints[m_gameType]`).

`Multiwinia::AITypeEasy=0, AITypeStandard=1, AITypeHard=2, NumAITypes=3` [C] `code/multiwinia.h`. UI labels: `dialog_easy_difficulty`, `multiwinia_menu_normal`, `multiwinia_menu_hard` [C] `code/UI/GameMenuWindow.cpp` (main/lobby page AI-mode drop-down).

### 1.2 Runtime permission of a game type

`App::IsGameModePermitted(gameMode)` [C] `code/app.cpp`:
1. If not full version (`!IsFullVersion()`, i.e. demo auth key) → consult `DemoLimitations::GameModePermitted` from the metaserver (P15/P17b) → false if not permitted. [B:DEMO runtime gate]
2. `switch(gameMode)`: returns true only for the cases whose `INCLUDEGAMEMODE_*` macro is defined. In FULLGAME: 0,1,2,3,4,5 → true; 6 (TankBattle) → false.

Consumers: game-type select page skips non-permitted types (`continue`) [C] `code/UI/GameMenuWindow.cpp` (~L3761, L3781); server browser filters rows [C] `code/UI/GameMenuWindow.cpp` (~L1844); `App::IsMapPermitted` [C] `code/app.cpp`. Therefore TankBattle is unreachable in the shipped exe even though its blueprint, three maps (`mp_tankbattle_2P/3P/4P.txt`) and the `switch` case exist; its `AdvanceTankBattle` body is empty in FULLGAME [C].

Other gates inside the mode code:
- `#ifndef MULTIWINIA_DEMOONLY` wraps the bodies of `AdvanceSkirmish`, `AdvanceRocketRiot`, `AdvanceTrunkPorts`, `AdvanceTankBattle`, `AdvanceAssault`, `AdvanceBlitkrieg` [C] — in the demo-only target only KOTH and CTS can run (matches build_matrix §3). [B:DEMO = TARGET_MULTIWINIA_DEMOONLY]
- Multiwinia::Advance only runs when `g_app->Multiplayer()` (≡ `m_gameMode == GameModeMultiwinia`), once per server advance on slice 8 of 10 [C] `code/network/clienttoserver.cpp:ClientToServer::AdvanceAndCatchUp` (L~1980: `if( g_app->Multiplayer() && g_sliceNum == 8 ) g_app->m_multiwinia->Advance();`). Note: this call happens even when the game is paused; `Multiwinia::Advance` itself returns after the grace-period branch if `GamePaused()` [C].
- Every Multiwinia game, including vs-CPU, is `Multiplayer()==true` and `IsSinglePlayer()==false` (IsSinglePlayer ≡ Campaign/Prologue) [C] `code/app.cpp:App::Multiplayer`, `App::IsSinglePlayer`. All "if Multiplayer()" branches in the world objects below are therefore the live Multiwinia behaviour; the `else` branches are Darwinia [B:LEGACY].

### 1.3 Level files per game type (confirmation only; X4 owns the catalogue)

Counted from the `GameTypes` line of `MultiwiniaOptions_StartDefinition` in `data/levels/mp_*.txt` [C]:

| Game type | Level files (count) | Mode-specific buildings present (from Buildings section) |
|---|---|---|
| Domination | mp_domination_2P_1, _2P_2, _3P_1, _4P_1, _4P_2, mp_koth_2P_2, mp_koth_2P_3, mp_skirmish_2p_4, mp_skirmish_3P_1 (9) | SpawnPoint/SpawnLink/SpawnPointMaster (no MultiwiniaZone) |
| KingOfTheHill | mp_koth_2P_1, _2P_3b, _3P_1, _3P_2, _3P_3, _3P_4, _4P_1, _4P_3, _4P_4 (9) | MultiwiniaZone (1–15 per map); mp_koth_3P_1 also 6 ControlStation linked to RadarDish; mp_koth_3P_4 + trunk ports |
| CaptureTheStatue | mp_cts_2p_1, _2p_2, _2p_3, _2p_4, _3P_2, _3p_1, _3p_3, _4P_1, _4P_2, _4p_3 (10) | MultiwiniaZone (team-owned = capture zones, team 255 = statue spawns) |
| Assault | mp_assault_2P_1, _2P_2, _2p_3, _3P_2, _3p_1, _3p_3, _4P_1, _4P_2 (8) | exactly 1 PulseBomb, 3–5 ControlStation, TrunkPort, AIObjective(+Marker), LaserFence, GunTurret |
| RocketRiot | mp_rocketriot_2P_2, _2P_4, _2p_3, _3P_2, _3P_3, _3p_1, _4P_1 (7) | 1 EscapeRocket/team, 3 FuelStation/team, FuelGenerator, FuelPipe, PylonEnd/Pylon/PowerSplitter, SolarPanel, 1 TrunkPort/team |
| Blitzkrieg | mp_blitzkrieg_2P_3, _2P_4, _2p_1, _2p_2, _3P_1, _3P_2, _4P_2 (7) | MultiwiniaZone (flags; 3–9 per map) with link lists, TrunkPort |
| TankBattle [B:DEBUG] | mp_tankbattle_2P, _3P, _4P (3) | trees only (tanks come from InstantUnits) |
| `none` | mp_assault_stress, mp_benchmark_1, mp_sandbox, mp_sandbox_all | — |

`data/levels/level_enum.txt` lists IDs 1–49 (the 49 shipped official maps above **except `mp_cts_3p_3.txt`**, which has no id) and IDs 50–59 for PDLC maps (`mp_PDLC1_*`, `mp_PDLC2_*`) whose files are **not present** in this tree [C] (`ls data/levels`).

---

## [→ 01_game_modes] 2. The game-option (parameter) system

### 2.1 Data format and loader

File: `data/multiwinia.txt` in FULLGAME; `data/multiwinia-testbed.txt` only under `TESTBED_ENABLED` [B:DEAD in all shipped targets] [C] `code/multiwinia.cpp:Multiwinia::LoadBlueprints`. (`data/multiwinia_betatesters.txt` is unrelated — a credits name list read by `code/UI/GameMenuWindow.cpp` [C].) Missing file → `AppReleaseAssert` "Couldn't load multiwinia.txt" (fatal) [C].

Grammar (line-based `TextReader`, `#` comments and blank lines skipped) [C] `Multiwinia::LoadBlueprints`:
```
GAMETYPE <BlueprintName>
    <ParamName> <min:float> <max:float> <default:float> <change:int>
    ...
END
```
- `change` semantics (header comment in the data file + UI construction in `code/UI/GameMenuWindow.cpp:GameMenuWindow::SetupAdvancedOptionsPage`): `-1` = check box; `0` = drop-down whose entries are every integer `p` in `[min,max]` labelled by string `multiwinia_option_<gameType>_<paramIndex>_<p>`; `>0` = integer spinner/input with step `change`, bounded to `[min,max]` (`NetworkGameOptionInt::SetBounds`).
- Values are parsed with `iv_atof`/`atoi` into `MultiwiniaGameParameter::{m_min,m_max,m_default,m_change}` but stored per game in `int Multiwinia::m_params[MULTIWINIA_MAXPARAMS=64]` [C] `code/multiwinia.h`.
- Parameter string ids: `"multiwinia_param_<name>"` lower-cased; `GetNameTranslated` falls back to the raw name [C] `code/multiwinia.cpp:MultiwiniaGameParameter::*`.
- Testbed variant differs only by adding `TimeLimit 0 60 10 1` as the last RocketRiot parameter [C] (`diff data/multiwinia.txt data/multiwinia-testbed.txt`).

### 2.2 Complete option tables (baseline `data/multiwinia.txt`) [C]

Index = position inside the block (= index into `m_params[]` and the network `GAMEOPTION_GAME_PARAM` array element).

**Domination (type 0)**

| Idx | Name | Min | Max | Default | Change | Control | Labels / meaning |
|---|---|---|---|---|---|---|---|
| 0 | ScoreMode | 0 | 1 | 0 | 0 | drop-down | 0 "Total Spawn Points", 1 "Spawn Points per Second" |
| 1 | TimeLimit | 0 | 60 | 10 | 1 | spinner (minutes) | 0 = unlimited |
| 2 | Handicap | 0 | 90 | 90 | 10 | spinner (%) | max spawn-rate bonus |
| 3 | CrateDropRate | 0 | 300 | 40 | 10 | spinner (s) | 0 = no crates |
| 4 | StartingPowerups | 0 | 3 | 0 | 1 | spinner | random powerups per team at start |
| 5 | RetributionMode | 0 | 1 | 1 | -1 | check box | |
| 6 | SuddenDeath | 0 | 1 | 1 | -1 | check box | |
| 7 | CrateDropMode | 0 | 1 | 1 | 0 | drop-down | 0 "Random", 1 "Weighted" |

**KingOfTheHill (type 1)**

| Idx | Name | Min | Max | Default | Change | Labels |
|---|---|---|---|---|---|---|
| 0 | ScoreMode | 0 | 3 | 0 | 0 | 0 "Zones controlled per second", 1 "Zone Population per second", 2 "Final Zone population", 3 "Total zone control" |
| 1 | TimeLimit | 1 | 60 | 5 | 1 | minutes (cannot be unlimited) |
| 2 | Handicap | 0 | 90 | 90 | 10 | |
| 3 | CrateDropRate | 0 | 300 | 40 | 10 | |
| 4 | StartingPowerups | 0 | 3 | 0 | 1 | |
| 5 | RetributionMode | 0 | 1 | 1 | -1 | |
| 6 | SuddenDeath | 0 | 1 | 1 | -1 | |
| 7 | CrateDropMode | 0 | 1 | 1 | 0 | 0 "Random", 1 "Weighted" |

**CaptureTheStatue (type 2)**

| Idx | Name | Min | Max | Default | Change | Labels |
|---|---|---|---|---|---|---|
| 0 | TimeLimit | 1 | 60 | 10 | 1 | minutes |
| 1 | CrateDropRate | 0 | 300 | 40 | 10 | |
| 2 | RetributionMode | 0 | 1 | 1 | -1 | |
| 3 | SuddenDeath | 0 | 1 | 1 | -1 | |
| 4 | CrateDropMode | 0 | 1 | 1 | 0 | 0 "Random", 1 "Weighted" |

**Assault (type 3)**

| Idx | Name | Min | Max | Default | Change |
|---|---|---|---|---|---|
| 0 | Handicap | 0 | 90 | 90 | 10 |
| 1 | CrateDropRate | 0 | 300 | 40 | 10 |

**RocketRiot (type 4)**

| Idx | Name | Min | Max | Default | Change | Meaning |
|---|---|---|---|---|---|---|
| 0 | CrateDropRate | 0 | 300 | 40 | 10 | |
| 1 | ReinforcementTimer | 10 | 300 | 60 | 10 | mode-seconds between trunk-port waves |
| 2 | ReinforcementCount | 1 | 100 | 40 | 1 | Darwinians per trunk port per wave |
| 3 | MaxArmour | 1 | 10 | 2 | 1 | max simultaneous Armour tasks |
| 4 | TurretFrequency | 0 | 10 | 7 | 1 | a turret every Nth wave (0 = never) |
| 5 | MaxTurrets | 0 | 5 | 3 | 1 | max simultaneous turret tasks |

**Blitzkrieg (type 5)**

| Idx | Name | Min | Max | Default | Change | Meaning |
|---|---|---|---|---|---|---|
| 0 | TimeLimit | 0 | 60 | 15 | 1 | minutes, 0 = unlimited |
| 1 | CrateDropRate | 0 | 300 | 40 | 10 | |
| 2 | ReinforcementTimer | 10 | 300 | 60 | 10 | |
| 3 | ReinforcementCount | 1 | 100 | 40 | 1 | |
| 4 | MaxArmour | 1 | 10 | 2 | 1 | |
| 5 | TurretFrequency | 0 | 10 | 2 | 1 | |
| 6 | RetributionMode | 0 | 1 | 1 | -1 | |
| 7 | SuddenDeath | 0 | 1 | 1 | -1 | |
| 8 | MaxTurrets | 0 | 5 | 2 | 1 | |

**TankBattle (type 6) [B:DEBUG]**: idx 0 `NumTanks` 3/10/3/1.

Language-file drift: `english.txt` also contains `multiwinia_option_0_5_*`, `0_6_*`, `1_5_*`, `1_6_*`, `2_2_*`, `2_3_*`, `5_6_*`, `5_7_*`, `5_8_*` (No/Yes, Random/Weighted). Those indices are check boxes or spinners in the current data, so the strings are never used; `5_8_*` "Random/Weighted" belongs to a CrateDropMode Blitzkrieg option that no longer exists [C] (compare data vs `data/language/english.txt`).

### 2.3 Reading an option: `GetGameOption(name)` and the "-1 = absent" rule

`int Multiwinia::GetGameOption(char*)` does a case-insensitive linear search of the current blueprint's parameter names and returns `m_params[i]`, or **-1 if the current game type has no parameter of that name** [C] `code/multiwinia.cpp:Multiwinia::GetGameOption`. All consumers must therefore treat -1 as "not applicable". Consequences confirmed in code:

| Read site | Expression | Effect when the option is absent (-1) |
|---|---|---|
| `Multiwinia::GameOptionsChanged` | `timeLimit > 0 ? m_timeRemaining = timeLimit*60 : -1` | -1 → `m_timeRemaining = -1` → unlimited time (RocketRiot, Assault until `PulseBomb::Initialise` overwrites it) [C] |
| `Multiwinia::Advance` | `bool retributionMode = GetGameOption("RetributionMode")` | **-1 converts to `true`** → `AdvanceRetribution` runs in Assault and RocketRiot too; harmless because `IsEliminated` is always false in those modes [C]; the HUD retribution counter is likewise gated by `eliminated` [C] `Multiwinia::Render` |
| sudden-death checks | `GetGameOption("SuddenDeath") == 1` | -1 → no sudden death [C] |
| `AdvanceTrunkPorts` | `ReinforcementTimer` | `< 0` → 60.0 [C] |
| `AdvanceTrunkPorts` | `ReinforcementCount` | -1 → `LevelOptionTrunkPortReinforcements` (and for Assault the attacker/defender level options) [C] |
| `AdvanceTrunkPorts` | `MaxArmour` | -1 → `LevelOptionTrunkPortArmour` (itself -1 when unset → no armour ever, since `CountNumTasks < -1` is false and `-1 != 0`) [C] |
| `AdvanceTrunkPorts` | `TurretFrequency` | -1 → no turrets (`turretTimer > 0` false) [C] |
| `AdvanceTrunkPorts` | `MaxTurrets` | -1 → 2 [C] |
| `code/location.cpp:Location::AdvanceCrates` | `CrateDropRate` | 0 or -1 → no crates [C] |
| `code/location.cpp:Location::CreateRandomCrate` | `CrateDropMode` | 0 or -1 → random placement; 1 → weighted [C] |
| `code/worldobject/spawnpoint.cpp:SpawnPoint::CalculateHandicap` | `Handicap` | 0 or -1 → multiplier 1.0 [C] |
| `code/taskmanager.cpp:Task::TargetGunTurret`, `TaskManager` (L~1329) | `MaxTurrets` | -1 → turret task terminates after placement (no cap tracking); otherwise task persists while the turret exists and counts toward the cap [C] |
| `code/location.cpp:Location::InitialiseTeam` (L~3307) | `StartingPowerups` | -1 → loop does not run [C] |
| `code/UI/mapDetailsButton.h`, `code/UI/GameMenuWindow.cpp:UpdateAdvancedOptionsPage` | `TimeLimit` | UI: when `TimeLimit == 0` the SuddenDeath control is forced off (`Set(0)`/`SelectOption(0)`) and made inactive [C] |

### 2.4 Setting options (lobby → network → Multiwinia)

- Defaults: on choosing a game type every `m_params[i] = blueprint->m_params[i]->m_default` [C] `code/UI/GameTypeButton.h`, `QuickStartButton.h`, `RollingDemoButton.h`, `GameMenuWindow::SetupTutorial*`.
- Per-map time default: on map select, the `TimeLimit` param is set to the level's `PlayTime` (minutes) if non-zero, else to the blueprint default [C] `code/UI/LevelSelectButton.h` (`GetTimeOption()`, `mapData->m_playTime`). Level `PlayTime` values: Assault 15 (4P_2: 20), KOTH 5–10, Domination 10–15 [C] level files.
- Map coop: selecting a map without coop support clears coop; `ForceCoop` maps force coop on [C] `code/UI/LevelSelectButton.h`.
- Network: options are `NetworkGameOptionInt/Bool` values replicated by `NET_DARWINIA_SETGAMEOPTIONINT` / `...ARRAYELEMENTINT` / `...ARRAYINT` letters with a per-option version number (newer version wins) [C] `code/network/clienttoserver.cpp` (L~2410–2560). Game-option ids [C] `code/network/network_defines.h`: `GAMEOPTION_GAME_TYPE=0`, `GAME_PARAM=1` (array), `GAME_RESEARCH_LEVEL=2` (array), `MAPNAME=3`, `AIMODE=4`, `COOPMODE=5`, `SINGLEASSAULT=6`, `BASICCRATES=7`, `TEAMSSTATUS_LBOUND=8 … UBOUND=13`, `NUMGAMEOPTIONS=14`.
- On `GAMEOPTION_GAME_TYPE` receipt the client zeroes all params and research then calls `SetGameOptions(type, zeros)` (params are re-sent afterwards) [C].
- AI mode range check is `value < 0 || value > NumAITypes` — off-by-one: value 3 would be accepted [C] (`clienttoserver.cpp` L~2492).
- `SetGameOptions(type, params)` copies `numParams` ints and calls `GameOptionsChanged()`; `SetGameOption(idx, v)` asserts `ValidGameOption(idx)` [C] `code/multiwinia.cpp`.
- `SetGameResearch(int*)` copies `GlobalResearch::NumResearchItems` research levels into `g_app->m_globalWorld->m_research->m_researchLevel` and zeroes progress [C] (research = crate/task weapon levels; owned by P08/P09).

### 2.5 Lobby options outside the blueprint

| Option | Storage | Network id | UI | Default | Effect |
|---|---|---|---|---|---|
| AI difficulty | `Multiwinia::m_aiType` | `GAMEOPTION_AIMODE` | lobby drop-down Easy/Normal/Hard (server only; disabled for demo keys) | `AITypeStandard` (ctor, Reset, `App::StartSinglePlayerServer`/`StartMultiPlayerServer` unless tutorial); tutorials force Easy | read by AI, Armour, MultiwiniaZone (see P08) [C] |
| Co-op | `Multiwinia::m_coopMode` | `GAMEOPTION_COOPMODE` | Advanced Options check box "multiwinia_menu_coop", only for maps with `m_coop` and not demo | false | 2-group alliance scoring; enabling resets all team colours to -1 [C] `clienttoserver.cpp` |
| Basic crates only | `m_basicCratesOnly` | `GAMEOPTION_BASICCRATES` | check box "multiwinia_param_basiccrates" (only `#ifdef INCLUDE_CRATES_ADVANCED`, on in FULL) | false | restricts crate rewards (P08) and retribution list [C] |
| Single assault round | `m_assaultSingleRound` | `GAMEOPTION_SINGLEASSAULT` | **no UI control found** (`m_singleAssaultRound` bound but never placed on a page) | **true** (ctor only; `Reset()` does not touch it) | multi-round Assault rotation is therefore unreachable [I: grep of `code/UI` shows no control] |
| Team status ×6 | `m_teamsStatus[6]` | `TEAMSSTATUS_LBOUND+i` | — | 0 | written on receipt, **never read** [C] |
| Research levels | `GlobalResearch::m_researchLevel[]` | `GAME_RESEARCH_LEVEL` | (P17b) | — | see P08 |

