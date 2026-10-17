# P08_crates_ai — Crates/power-ups, CPU AI, objective markers
Scope: `code/worldobject/crate.{h,cpp}`, `code/worldobject/ai.{h,cpp}`, `code/worldobject/aiobjective.{h,cpp}`, `code/markersystem.{h,cpp}`; plus `code/interface/invalidcrates_window.*` and every crate/InvalidCrates/crate-option reference (`code/location.cpp`, `code/level_file.cpp`, `code/multiwinia.cpp`, `code/taskmanager.cpp`, `data/multiwinia.txt`, `data/levels/mp_*.txt`, `data/language/english.txt`), and the AI-facing `RunAI` methods of other entities (read to describe CPU behaviour; their non-AI mechanics belong to P04/P06/P07). Build baseline: see build_matrix.md.

Conventions in this draft: "tick" = one simulation step; `SERVER_ADVANCE_PERIOD` = `0.1 × gameSpeed` game-seconds (`code/gametimer.cpp:GameTimer::GetServerAdvancePeriod`) [C]; all distances are world units (wu). `syncfrand(x)` ∈ [0,x), `syncsfrand(x)` ∈ [−x/2, +x/2), `syncrand()` = MT19937 32-bit (`contrib/systemiv/lib/math/random_number.cpp:_syncrand/_syncfrand/_syncsfrand`) [C]. `FRAND(x)`/`SFRAND(x)` are the *synced* order-safe wrappers (`code/lib/vector3.h`, `code/lib/vector3.cpp:FromRand`) [C]. Monster team = team index 4, Futurewinian team = 5 (`code/location.cpp:Location::GetMonsterTeamId/GetFuturewinianTeamId`) [C].

---

## [→ 04_crates_powerups_and_weapons] 1. Build gating of the crate system

| Macro | Where defined | Baseline FULL | Effect | Evidence |
|---|---|---|---|---|
| `INCLUDE_CRATES_BASIC` | `code/lib/universal_include.h` (every target) | **on** | Compiles crate spawning (`Location::CreateRandomCrate`, `Location::AdvanceCrates`). Without it no crates ever drop. | [C] `code/location.cpp:Location::CreateRandomCrate`, `Location::AdvanceCrates` |
| `INCLUDE_CRATES_ADVANCED` | FULL, DEBUG, BETATEST_GROUP_ALL, group A | **on** | Bodies of all advanced `Run*` effects; "bad" crate pool; lobby "Basic Crates Only" checkbox. Without it `IsValid` rejects every advanced type and `RunUncapturedCrate` returns immediately. | [C] `code/worldobject/crate.cpp:Crate::IsValid`, `Crate::RunUncapturedCrate`, `Crate::RunNuke`…; `code/UI/GameMenuWindow.cpp` (checkbox block) |
| `INCLUDE_THEBEAST` | `code/worldobject/crate.h`, **only `#ifdef TARGET_DEBUG`** | **off** | Adds enum value `CrateRewardTheBeast` (index 31) → `NumCrateRewards` = 32 in DEBUG, **31 in FULL**. | [C] `code/worldobject/crate.h` lines 12–14, 66–68 |
| `INCLUDE_CRATE_HELP_WINDOW` | `TARGET_DEBUG` only | off | Front-end "crate help" list window (`CrateHelpWindow`, `CrateButton`) [B:DEBUG]. | [C] `code/lib/universal_include.h` TARGET_DEBUG block; `code/interface/helpandoptions_windows.cpp` |
| `CHEATMENU_ENABLED` | `TARGET_DEBUG` only | off | Cheat window "Crates" drop-down that spawns a crate with a forced `m_rewardId` [B:DEBUG]. | [C] `code/interface/cheat_window.cpp` |
| `LOCATION_EDITOR` | nowhere | off | `InvalidCratesWindow` editor panel [B:EDITOR]. | [C] `code/location_editor.cpp`, `code/interface/invalidcrates_window.cpp` |

> Note: build_matrix.md §4 lists `INCLUDE_THEBEAST` as "on" in baseline. Code says otherwise: it is defined only inside `#ifdef TARGET_DEBUG` in `crate.h` [C]. Logged under Contradictions.

## [→ 04_crates_powerups_and_weapons] 2. The crate reward enum (complete)

`Crate::CrateReward*` anonymous enum in `code/worldobject/crate.h` [C]. The level-file/sound key is `Crate::GetName(type)` (`code/worldobject/crate.cpp:Crate::GetName`, "no spaces allowed"); the English display name is `cratename_<GetName>` in `data/language/english.txt` (looked up case-insensitively by `Crate::GetNameTranslated`, falling back to the raw key) [C].

| Idx | Enum | GetName key (used in `InvalidCrates`, sounds, icons) | English display (`cratename_*`) | Class (`IsBasicCrate`) | Build |
|---|---|---|---|---|---|
| 0 | `CrateRewardSquad` | `Squad` | Squad | **Basic** | FULL |
| 1 | `CrateRewardHarvester` | `Harvester` | Harvester | Advanced | FULL |
| 2 | `CrateRewardEngineer` | `Engineer` | Engineer | **Basic** | FULL |
| 3 | `CrateRewardArmour` | `Armour` | Armour | **Basic** | FULL |
| 4 | `CrateRewardAirstrike` | `Airstrike` | AirStrike | **Basic** | FULL |
| 5 | `CrateRewardNuke` | `Nuke` | Nuke | Advanced | FULL |
| 6 | `CrateRewardDarwinians` | `Darwinians` | Multiwinians | **Basic** | FULL |
| 7 | `CrateRewardBooster` | `PersonalShield` | Personal Shield | Advanced | FULL |
| 8 | `CrateRewardSubversion` | `Subversion` | Subversion | Advanced | FULL |
| 9 | `CrateRewardHotFeet` | `HotFeet` | Hot Feet | Advanced | FULL |
| 10 | `CrateRewardGunTurret` | `GunTurret` | Gun Turret | **Basic** | FULL |
| 11 | `CrateRewardSpam` | `Infection` | Infection | Advanced | FULL |
| 12 | `CrateRewardForest` | `MagicalForest` | Magical Forest | **Basic** | FULL |
| 13 | `CrateRewardRandomiser` | `Randomiser` | Randomiser | Advanced | FULL |
| 14 | `CrateRewardAntNest` | `AntNest` | Ants Nest | Advanced | FULL |
| 15 | `CrateRewardPlague` | `Plague` | Plague | Advanced | FULL |
| 16 | `CrateRewardEggs` | `Eggs` | Eggs | Advanced | FULL |
| 17 | `CrateRewardMeteor` | `Meteor` | Meteor | Advanced | FULL |
| 18 | `CrateRewardDarkForest` | `DarkForest` | Dark Forest | Advanced | FULL |
| 19 | `CrateRewardCrateMania` | `CrateMania` | Crate Mania | Advanced | FULL |
| 20 | `CrateRewardSpawnMania` | `SpawnMania` | Spawn Mania | Advanced | FULL |
| 21 | `CrateRewardBitzkreig` | `Bitzkreig` | **Mega Rage** | Advanced | FULL |
| 22 | `CrateRewardRocketDarwinians` | `Rocketwinians` | Rocketwinians | Advanced | FULL |
| 23 | `CrateRewardRage` | `Rage` | Rage | Advanced | FULL |
| 24 | `CrateRewardRocketTurret` | `RocketTurret` | Rocket Turret | Advanced | FULL |
| 25 | `CrateRewardFlameTurret` | `FlameTurret` | Flame Turret | Advanced | FULL |
| 26 | `CrateRewardSpaceShip` | `SpaceShip` | Space Ship | Advanced | FULL |
| 27 | `CrateRewardTriffids` | `Triffids` | Triffids | Advanced | FULL |
| 28 | `CrateRewardNapalmStrike` | `NapalmStrike` | Napalm Strike | Advanced | FULL |
| 29 | `CrateRewardSlowDownTime` | `SlowDown` | Slow Down | Advanced | FULL |
| 30 | `CrateRewardSpeedUpTime` | `SpeedUp` | Speed Up | Advanced | FULL |
| 31 | `CrateRewardTheBeast` | `TheBeast` | The Beast | Advanced | **[B:DEBUG]** (`INCLUDE_THEBEAST`) |
| — | `NumCrateRewards` | (returns `"Crate"` for out-of-range) | — | — | = 31 FULL / 32 DEBUG |

Commented-out enum members (not compiled, appendix): `CrateRewardFormationIncrease`, `CrateRewardExtinguisher`, `CrateRewardTank` — their strings `cratename_formationbooster`, `cratename_fireextinguisher`, `cratename_tank` still exist in `english.txt` [C].

`IsBasicCrate(t)` = t ∈ {Squad, Engineer, Armour, Airstrike, Darwinians, GunTurret, Forest} (7 types); `IsAdvancedCrate(t)` = ¬Basic (24 types in FULL) [C] `code/worldobject/crate.cpp:Crate::IsBasicCrate/IsAdvancedCrate`. Note Harvester is *advanced* although it is a unit task.

## [→ 04_crates_powerups_and_weapons] 3. Crate options (lobby / data / level)

| Option | Source | Values | Modes that define it | Effect |
|---|---|---|---|---|
| `CrateDropRate` | `data/multiwinia.txt` (min 0, max 300, **default 40**, step 10) | seconds (mean interval) | Domination, KingOfTheHill, CaptureTheStatue, Assault, RocketRiot, Blitzkrieg (not TankBattle) | Base drop interval; 0 = no crates; `GetGameOption` returns −1 if undefined → no crates. [C] `code/location.cpp:Location::AdvanceCrates`, `code/multiwinia.cpp:Multiwinia::GetGameOption` |
| `CrateDropMode` | `data/multiwinia.txt` (0–1, **default 1**, change 0 = drop-down) | 0 = random, 1 = score-weighted | Domination, KingOfTheHill, CaptureTheStatue only (others → −1 = random) | Placement algorithm, §4.2. [C] |
| `StartingPowerups` | `data/multiwinia.txt` (0–3, default 0, step 1) | count | Domination, KingOfTheHill only | Free tasks at team init, §8. [C] `code/location.cpp:Location::InitialiseTeam` |
| `RetributionMode` | `data/multiwinia.txt` (0–1, default 1, checkbox) | bool | Dom, KOTH, CTS, Blitzkrieg | **No effect**: the only consumer (`Multiwinia::AdvanceRetribution`) is never called (call commented out) — see §8 / appendix. [C] grep: only definition in `code/multiwinia.cpp` |
| Basic Crates Only | lobby checkbox `multiwinia_param_basiccrates` → `Multiwinia::m_basicCratesOnly`; network game option `GAMEOPTION_BASICCRATES = 7` | bool, default false | all | Restricts rewards to the 7 basic types and disables the "bad" pool. Checkbox compiled only under `INCLUDE_CRATES_ADVANCED`. Reset to false by `FinishMultiwiniaButton` and `Multiwinia::Reset`; forced true by tutorial 2. [C] `code/UI/GameMenuWindow.cpp`, `code/network/network_defines.h`, `code/network/clienttoserver.cpp` (case `GAMEOPTION_BASICCRATES`), `code/UI/FinishMultiwiniaButton.h`, `code/multiwiniahelp.cpp` |
| `InvalidCrates` | level file `MultiwiniaOptions` block | list of GetName keys | per level | Per-level exclusion, §7. [C] `code/level_file.cpp:LevelFile::ParseMultiwiniaOptions` |
| `LevelOptions trunkportarmour N` | level file | int | per level | N>0 makes `Armour` invalid. [C] `Crate::IsValid` |
| `RestrictionZone` building | level file building `RestrictionZone id x z team rx rz isGlobal size [blockPowerups]` (default size 300) | — | per level | No random crate may drop inside (`RestrictionZone::IsRestricted`). `m_blockPowerups` is read but not used by crate code [C] `code/worldobject/restrictionzone.cpp`. |

## [→ 04_crates_powerups_and_weapons] 4. Crate spawn rules

### 4.1 Drop timer — `Location::AdvanceCrates` [C] `code/location.cpp`
Called from `Location::Advance(slice)` only when `g_app->Multiplayer()` and `slice == 8` → **once per tick**. Same block decrements `m_spawnManiaTimer` and `m_bitzkreigTimer` by `SERVER_ADVANCE_PERIOD` [C].

```
AdvanceCrates():
  if tutorial and (tutorialType == Tutorial1 or !help.m_tutorial2AICanSpawn): return   # no crates
  maxCrates = int(worldSizeX / 500.0)                    # e.g. 2000 wu → 4, 5000 wu → 10
  if Crate::s_crates >= maxCrates: return                # timer is frozen while at cap
  crateTime = GetGameOption("CrateDropRate")
  if crateTime == 0 or crateTime == -1: return
  if m_crateTimer == 0.0: m_crateTimer = crateTime * (1.5 - syncfrand(1.0))   # (0.5..1.5]×rate
  m_crateTimer -= SERVER_ADVANCE_PERIOD
  if m_crateTimer <= 0.0:
      m_crateTimer = crateTime * (1.5 - syncfrand(1.0))
      CreateRandomCrate()
```
- `Crate::s_crates` counts every live `Crate` object (incremented in ctor, decremented in dtor) — including falling crates and template objects briefly created on the stack (`Crate crateTemplate;` in `CreateRandomCrate` increments and decrements around the call) [C] `Crate::Crate/~Crate`. Reset to 0 at `Location` construction together with `s_spaceShipUsed = false` [C] `code/location.cpp` ctor.
- Cap uses world **X** size only [C].
- Interval distribution: uniform on (0.5×, 1.5×] `CrateDropRate` game-seconds; game-speed crate effects stretch/compress it in real time because `SERVER_ADVANCE_PERIOD` scales [I].

### 4.2 Placement — `Location::CreateRandomCrate` [C] `code/location.cpp`
1. `dropMode = GetGameOption("CrateDropMode")`; random if 0 or −1.
2. **Weighted mode (1)** — a catch-up mechanic:
   - For each team that is used, not monster (4), not Futurewinian (5): `score_i = Multiwinia::m_teams[i].m_score`; `total = Σscore`, `high = max`, `low = min` (low initialised to `FLT_MAX`, high to 0).
   - `p = max(high/total, 1 − low/total)`; if `syncfrand(1) > p` → fall back to random.
   - Else choose a team with probability `w_i / Σw`, `w_i = total/score_i` (or `total` if `score_i == 0`) using one `syncfrand(1)` and cumulative odds — i.e. **lower-scoring teams are favoured**.
   - Build a list of *all* buildings owned by that team (any type, including AI targets). Up to 1000 tries: pick `syncrand() % n`, `pos = b.pos + (syncsfrand(200), 0, syncsfrand(200))` (±100 wu). Height test is buggy: `GetValue(pos.x, pos.z > 20.0)` passes a bool as the z coordinate, so it samples the terrain at z∈{0,1} and accepts whenever that height is non-zero — effectively always accepts the first try [C code, I consequence]. Crate start = `(pos.x, 1300 + syncfrand(200), pos.z)`.
   - If no team chosen, no buildings, or no position → random. (With `total == 0` the NaN arithmetic yields `dropTeam = −1` → random [I].)
3. **Random mode**: loop while `height(pos) < 20` or `RestrictionZone::IsRestricted(pos)`; each iteration (max 101; after 100 failures *no crate is created*): `pos = (FRAND(worldX), 1300, FRAND(worldZ))`, `pos.y += syncfrand(200)`; if any existing crate is within `CRATE_SCAN_RANGE × 10 = 350` wu horizontally, pos is zeroed to force another try. Weighted-mode drops skip the restriction-zone and spacing checks [C].
4. Create: team 255 (neutral), `Building::CreateBuilding(TypeCrate)`, `Initialise`, `SetDetail(1)`, `MarkerSystem::RegisterMarker_Crate(id)`; if `m_showCrateDropMessage` (true once per Location) show `crate_drop` ("Crate Drop!") and clear the flag [C].
- Start height constant `CRATE_START_HEIGHT = 1300.0` (`#define`, `crate.h`) [C].
- Crates **cannot be placed by level files**: `TypeCrate` is in `Building::BuildingIsBlocked` (`BLOCK_OLD_DARWINIA_OBJECTS`), which blocks name-based creation only; runtime `CreateBuilding(int)` still works [C] `code/worldobject/building.cpp:Building::BuildingIsBlocked/CreateBuilding(char*)`. `Crate` has no `Read/Write` override, so `m_rewardId` is never level-settable [C].
- Other crate sources: `RunCrateMania` (§6), tutorials (`code/multiwiniahelp.cpp` — fixed `m_rewardId` Airstrike at spawn-point−(100,0,200) +800 y; Armour at +100 x, +300 y), cheat window [B:DEBUG] (`m_rewardId` from drop-down, y=300) [C].

### 4.3 Falling — `Crate::AdvanceFloating` [C] `code/worldobject/crate.cpp`
Per tick while `!m_onGround`:
- Landed when `pos.y ≤ height(x,z) + 20` → `vel = 0`, `m_onGround = true`, stop sound "Crate Falling", trigger building sound "Land".
- `t = GetNetworkTime() + uniqueId × 20`; `sway = 300 × (pos.y / 1300)`; `target = (pos.x + cos(t)·sway, height(target.x,z), pos.z)` (sway is on X only); `target = PushFromObstructions(target)`; step = unit vector toward target × 40, push again, then clamp the displacement to `40 × SERVER_ADVANCE_PERIOD` → **crate moves at 40 wu per game-second** along a swaying path. `vel = Δpos / SERVER_ADVANCE_PERIOD` (used for render prediction and iframes).
- Derived fall time from 1300–1500 wu ≈ 33–38 game-seconds [I: path length / 40 wu·s⁻¹, vertical component ≈ 0.97].
- `PushFromObstructions(p)`: for each building in the obstruction-grid cell whose `DoesSphereHit(p, m_radius)`, repeatedly step p away from the building centre (XZ) by 2 wu until clear; if the resulting terrain height ≤ 10 the push is discarded (prevents pushing into water) [C].
- On spawn (`Initialise`) sound event "Falling" [C].

### 4.4 Lifetime / expiry [C] `Crate::AdvanceOnGround`
- `m_life` starts at 60.0 s and only counts down while the crate is on the ground **and neutral (team 255)** and not in tutorial 1 / tutorial 2 pre-spawn. It is reset to 60 every tick the owner holds the majority.
- When `m_life ≤ 0` and `m_captureTimer ≤ 0` → destroyed, sound "Expired", no effect (an `RunUncapturedCrate(monster)` call here is commented out), AI target destroyed. Explosion effect `g_explosionManager.AddExplosion(shape)` on any destruction [C].

## [→ 04_crates_powerups_and_weapons] 5. Pickup (capture) rules — `Crate::AdvanceOnGround`, `Crate::RecalculateOwnership` [C]

Constants: `CRATE_SCAN_RANGE = 35.0` (`#define`, crate.h), `CRATE_CAPTURE_TIME = 2.5` (`#define`, crate.cpp), max progress per tick 0.05 (literal).

On first ground tick the crate creates an **AITarget** at its ground position: `m_priorityModifier = 0.5`, `m_crateTarget = true`, unique building id, then `Location::RecalculateAITargets()` (see §AI) [C].

Each tick:
1. `RecalculateOwnership(num)`: for every used team t, count entities from `EntityGrid::GetNeighbours(pos, 35, includes={t})` that are `TypeDarwinian`, `!m_dead`, `state != StateInsideArmour`. `majority` = team with the strictly highest count (ties keep the earlier team index); 255 if all zero; `num` = that count. Officers, squads, armour, monsters do not count [C].
2. If `owner == majority && owner != 255`:
   - `Δ = min(0.05, SERVER_ADVANCE_PERIOD × num / 100)`; `m_captureTimer = min(2.5, m_captureTimer + Δ)`; `m_life = 60`.
   - Progress rate = `num/100` per game-second, capped at 0.05 per tick → **minimum capture time 50 ticks (5 s at speed 1) with ≥ 50 Darwinians**; 10 Darwinians → 25 s; 1 Darwinian → 250 s [C formula, I examples].
   - Sound when the 20 % bucket `int(5·timer/2.5)` changes: stop "Crate Capturing_*", trigger `Capturing_<bucket×20>pc` (0,20,40,60,80).
   - When `m_captureTimer ≥ 2.5` → **capture** (§6), `m_destroyed = true`, stop capture sounds and "Crate BeginCapture", trigger "Captured", destroy AI target + `RecalculateAITargets`, and if the capturer is the local team call `MultiwiniaHelp::NotifyCapturedCrate()`.
3. Else (contested, owner lost majority, or neutral): `m_captureTimer = max(0, timer − SERVER_ADVANCE_PERIOD)` (drains at 1.0 per game-second). When it reaches 0 and `owner != majority` → `SetTeamId(majority)`; if new owner is 255 → sound "LostCapture"; else "BeginCapture" + "Capturing_0pc". A neutral crate with a majority team changes owner on the same tick. A challenger must first drain the previous owner's progress.
   - Bug: the decay branch builds the sound name with `"Capturing_%pc"` (missing `d`), producing a pointer-formatted, non-existent event name [C].
4. `GetPercentCaptured()` = `m_captureTimer / 2.5` when owned, else 0 [C].

Darwinian side (P04 owns the state machine): idle Darwinians run `Darwinian::SearchForCrates` as the 4th idle search (after threats, ports, carryable buildings; before armour, Blitzkrieg flags, officers, spirits): any on-ground crate within `DARWINIAN_SEARCHRANGE_CRATES = 150` and walkable → `StateOpeningCrate`; waypoint = own position if already within 35, else a point `35/3 + syncfrand(70/3)` from the crate toward the Darwinian, plus an offset of `syncfrand(35/3)` rotated by `syncfrand(2π)` [C] `code/worldobject/darwinian.cpp:Darwinian::SearchForCrates`.

Capture resolution:
```
on capture (owner = capturing team):
  noAdvanced = m_basicCratesOnly || !INCLUDE_CRATES_ADVANCED
  if m_rewardId != -1 || syncfrand(1.0) < 0.9 || noAdvanced:   # syncfrand consumed only when m_rewardId == -1
       CaptureCrate()            # "good" outcome (90 %)
  else RunUncapturedCrate(owner) # "bad" outcome (10 %)
```

## [→ 04_crates_powerups_and_weapons] 6. Outcome selection and per-outcome specification

### 6.1 "Good" pool — `Crate::CaptureCrate()` [C]
`rewardId = syncrand() % NumCrateRewards`, repeated (rejection sampling, unbounded loop) until `rewardId ∉ {Spam, Randomiser, Plague, SpaceShip, Triffids, [TheBeast]}` **and** `IsValid(rewardId)`. Then `if (m_rewardId != -1) rewardId = m_rewardId` (the random draw still consumed RNG). Hence each valid good type has **equal probability** `0.9 / |valid good set|`.

`Crate::IsValid(t)` (in order) [C]:
1. `!INCLUDE_CRATES_ADVANCED` and advanced → false.
2. `m_basicCratesOnly` and advanced → false.
3. t ∈ {Harvester, Engineer, SpawnMania} and `Σ SpawnPoint::s_numSpawnPoints[0..NUM_TEAMS]` == 0 → false (no spawn points on level).
4. t == Armour and (game type RocketRiot or `LevelOptionTrunkPortArmour > 0`) → false.
5. Assault and t ∈ {RocketTurret, SpaceShip} → false.
6. t == SpaceShip and `s_spaceShipUsed` → false.
7. t == CrateMania and `s_crates ≥ 4` → false (the crate being captured is still counted).
8. t == AntNest and (#AntHill buildings + 2 × Σ_teams pending `TypeAntsNest` tasks) ≥ 4 → false.
9. return `!levelFile.m_invalidCrates[t]`.

### 6.2 "Bad" pool — `Crate::RunUncapturedCrate(team)` [C]
Returns immediately if `!INCLUDE_CRATES_ADVANCED` or `m_basicCratesOnly`. Builds list {Spam, Randomiser, Plague, DarkForest, Triffids, [TheBeast DEBUG], SpaceShip (only if `!s_spaceShipUsed` and not Assault), AntNest} and picks `syncrand() % size` uniformly. **`IsValid`/`InvalidCrates` are not consulted** (e.g. AntNest limit and per-level exclusions do not apply to bad outcomes). Calls `Location::InitMonsterTeam(capturingTeam)` — a no-op because the capturing team is in use (the intended `monsterTeam == team` guard is commented out) [C code; I intent]. Effects are created for the monster team (4), Futurewinians (5) for SpaceShip.
- The `m_rewardId` branch and the switch cases Airstrike, Darwinians, Forest, Eggs, Meteor, NapalmStrike, SlowDown, SpeedUp inside `RunUncapturedCrate` are **unreachable** (a forced reward always goes to `CaptureCrate`; the list never contains those types) [C].

### 6.3 Probability tables
Per-level computation in `OUT/_work/drafts/P08_crate_pools.csv` (script `OUT/_work/scripts/P08_crate_pools.py`; start-of-game state: no dynamic limits active, SpaceShip unused) [C inputs from `data/levels/mp_*.txt`; arithmetic I].

| Context (FULL, Basic-only OFF) | Valid good types | Each good type | Bad pool | Each bad type | Notes |
|---|---|---|---|---|---|
| Dom / KOTH / CTS / Blitz with spawn points, no `InvalidCrates` | 26 | 0.9/26 = **3.462 %** | 7 | 0.1/7 = **1.429 %** | DarkForest and AntNest are in both pools → 4.890 % each |
| Same, after SpaceShip used | 26 | 3.462 % | 6 | 1.667 % | |
| Map with no SpawnPoints (e.g. `mp_koth_3P_4`, `mp_cts_3p_3`, `mp_blitzkrieg_3P_1/3P_2/4P_2`) | 23 (−Harvester, Engineer, SpawnMania) | 3.913 % | 7 | 1.429 % | |
| RocketRiot with spawn points (`mp_rocketriot_2P_2`, `4P_1`) | 25 (−Armour) | 3.600 % | 7 | 1.429 % | |
| RocketRiot without spawn points (2P_4, 2p_3, 3P_2, 3P_3, 3p_1) | 22 | 4.091 % | 7 | 1.429 % | |
| Assault, no InvalidCrates (`mp_assault_2P_2`, `3p_3`) | 21–22 (−RocketTurret, −Harvester/Engineer/SpawnMania; −Armour if trunkportarmour) | 4.09–4.29 % | 6 (no SpaceShip) | 1.667 % | |
| `mp_assault_2P_1` (`InvalidCrates Squad Airstrike Nuke Meteor`) | 18 | 5.000 % | 6 | 1.667 % | |
| `mp_assault_2p_3` (same list + trunkportarmour 3) | 17 | 5.294 % | 6 | 1.667 % | |
| Basic-only ON, spawn points present | 7 basic | **14.286 %** (100 %/7) | none | 0 % | bad pool disabled |
| Basic-only ON, no spawn points | 6 (−Engineer) | 16.667 % | none | | |

Good-pool membership (26 in the standard case): Squad, Harvester, Engineer, Armour, Airstrike, Nuke, Darwinians, PersonalShield, Subversion, HotFeet, GunTurret, MagicalForest, AntNest, Eggs, Meteor, DarkForest, CrateMania, SpawnMania, Bitzkreig, Rocketwinians, Rage, RocketTurret, FlameTurret, NapalmStrike, SlowDown, SpeedUp. Never in the good pool by random draw: Infection(Spam), Randomiser, Plague, SpaceShip, Triffids, TheBeast.

### 6.4 Delivery modes
- **Task (deferred)**: `team->m_taskManager->RunTask(GlobalResearch::Type…)` adds a task the owner targets later (human via task UI; CPU via `AI::AdvanceCurrentTasks`). If the task manager is full the reward is **lost**; local player sees `dialog_powerup_limit` "Powerup Limit Reached!". Multiplayer capacity = 6 tasks (2 if the team is eliminated) [C] `code/taskmanager.cpp:TaskManager::RunTask(Task*)`, `TaskManager::Capacity`. New tasks are inserted before the first not-yet-targeted task (multiplayer) [C].
- **Immediate**: effect fires at the crate's position on capture.
- For tasks, message: if capturer ≠ local team, `crate_enemycapture` "*T Opened a Crate!" (T = team name) is shown to everyone else; the local capturer gets the task popup instead (message removed "by Chris") [C] `Crate::CaptureCrate`.

### 6.5 Per-outcome specification
Effect functions are all `static` in `Crate` and are reused by tasks (`code/taskmanager.cpp:Task::Target*`). `POWERUP_EFFECT_RANGE = 70.0` (`#define`, `code/taskmanager.h`). "Marker" = `MarkerSystem::RegisterMarker_Fixed(team, pos, icon)` (§MarkerSystem). "Sound" = `Crate::TriggerSoundEvent(type,pos,team)` → `SoundSystem::TriggerOtherEvent(pos, GetName(type), SoundSourceBlueprint::TypeCratePowerup, team)`. Message strings are in `data/language/english.txt`; `*T` = team name, `*G` = turret name.

| Outcome | Pools | Delivery / task type | Effect (counts, placement, duration, recipient) | Limits | Message key → English | Marker icon | Code |
|---|---|---|---|---|---|---|---|
| Squad | good, start | Task `TypeSquad` (`m_lifeTimer` 60 s) | On target: spawn `2 + research(TypeSquad)` InsertionSquadie (research is forced to 3 in MP → 5 squaddies [I: `Location::Init` sets all research levels to 3]) at target, unit selected. Mechanics P04. | — | task UI | task marker `icon_squad` | `Task::TargetSquad` |
| Harvester | good, start | Task `TypeHarvester` | Spawn Harvester at target +50 y. | needs spawn points | task UI | — | `Task::TargetHarvester` |
| Engineer | good, start | Task `TypeEngineer` | Spawn Engineer (P04/P09). | needs spawn points | task UI | — | `Task::TargetEngineer` |
| Armour | good, start | Task `TypeArmour` | Spawn Armour at target; sound `Armour`. | invalid in RocketRiot or `trunkportarmour > 0` | task UI | — | `Task::TargetArmour` |
| Airstrike | good, start | Task `TypeAirStrike` (MP) | `RunAirStrike(pos, team, false)`: throw `3 + syncrand()%5` (3–7; **10 in tutorial**) `EffectThrowableAirstrikeMarker` weapons from pos to pos + (SFRAND(100),0,SFRAND(100)) (±50); bombers are P04/P05. Task terminates on use. | — | `crate_message_run_airstrike` "*T launches Airstrike!" (monster: `crate_message_auto_airstrike` "Incoming Airstrike!") | `icon_airstrike` | `Crate::RunAirStrike` |
| NapalmStrike | good, start | Task `TypeAirStrike` with `m_option = 1` | `RunAirStrike(pos, team, true)` → markers with `m_naplamBombers = true`. | — | `crate_message_run_napalm` "I love the smell of Napalm in the morning..." (no *T substitution text in English though `ReplaceStringFlag('T')` is applied); monster: `crate_message_auto_napalm` | `icon_airstrike` | same |
| Nuke | good, start | Task `TypeNuke` | 10 Nuke entities of `team` spawned at random sea positions (`FRAND(worldX−100)`, `FRAND(worldZ−100)`, re-rolled until height ≤ 0 — **infinite loop on a map without water**); nuke 0 targets pos exactly and renders a marker; nukes 1–9 target pos + `syncsfrand(300)` (±150); target y = ground − 5. | — | `crate_message_run_nuke` "*T launches a Nuke Barrage!" / auto: "Incoming Nuke Barrage!" | `icon_nuke` | `Crate::RunNuke` |
| Darwinians | good | **Immediate** | Spawn `50 + syncrand()%50` (50–99) Darwinians of the capturer at the crate, vel = up+(SFRAND(1),0,SFRAND(1)) scaled to `10 + syncfrand(20)`, airborne. | — | `crate_message_darwinians` "*T receives Reinforcements!" (monster: hard-coded "Evil Darwinians!") | `icon_darwinian` | `Crate::RunDarwinians` |
| PersonalShield (Booster) | good, start | Task `TypeBooster`, life 30 s | All capturer Darwinians within 70 of target: `Darwinian::SetBooster(taskId)`. | — | `crate_message_run_personalshield` "*T runs Personal Shield!" | `icon_personalshield` | `Crate::RunBooster` |
| Subversion | good, start | Task `TypeSubversion`, life 30 s | Own Darwinians within 70: `GiveMindControl(taskId)`. | — | "*T runs Subversion!" | `icon_subversion` | `Crate::RunSubversion` |
| HotFeet | good, start | Task `TypeHotFeet`, life 30 s | Own Darwinians within 70: `GiveHotFeet(taskId)`. | — | "*T runs Hot Feet!" | `icon_hotfeet` | `Crate::RunHotFeet` |
| Rage | good, start | Task `TypeRage`, life 30 s | Own Darwinians within 70: `GiveRage(taskId)`. | — | `crate_message_rage` "*T runs Rage!" | `icon_rage` | `Crate::RunRage` |
| GunTurret | good, start | Task `TypeGunTurret`, `m_option = GunTurretTypeStandard` | `RunGunTurret`: create GunTurret building (state = turret type) at target, `ExplodeBody()` (build effect), obstruction grid recalculated. Task terminated unless the mode has `MaxTurrets` (RocketRiot/Blitzkrieg) in which case the task stays Running. | — | `crate_message_run_gunturret` "*T places a *G" | `icon_gunturret` | `Crate::RunGunTurret`, `Task::TargetGunTurret` |
| RocketTurret | good, start | as GunTurret, `GunTurretTypeRocket` | as above | invalid in Assault | same | same | same |
| FlameTurret | good, start | as GunTurret, `GunTurretTypeFlame` | as above | — | same | same | same |
| MagicalForest | good | **Immediate** (`RunMagicalForest(pos, monsterTeam)`; team id used only for message/marker colour) | `GenerateTrees`: `1 + syncrand()%2` (1–2) trees within ±50; clone of the first non-evil level tree, else random template (budsize/pushOut/pushUp `1.2 − syncfrand(0.4)`, random colours `100 + rand%155` per channel, height 80, 6 iterations); each: height capped 100 then ×`(1.2 − syncfrand(0.4))`, `SetFireAmount(−1)` (min height, not burning), `m_spawnsRemaining = 2 + syncfrand(3)`, `m_spiritDropRate = 15 + syncfrand(15)`, destroyable, team 255. | — | `multiwinia_exclamation` "*T!" with "Magical Forest" | `icon_magicalforest` | `Crate::RunMagicalForest/GenerateTrees` |
| DarkForest | good, start, **bad** | good: Task `TypeDarkForest`; bad: immediate for monster team | `GenerateEvilTrees`: `2 + syncrand()%3` (2–4) evil trees (black branches, leaves alpha 200, 5 iterations, `m_evil = true`) within ±50 (tree behaviour P07). | — | team: "*T plants a Dark Forest!" / monster: "Dark Forest!" | `icon_darkforest` | `Crate::RunDarkForest/GenerateEvilTrees` |
| AntNest | good, start, **bad** | good: Task `TypeAntsNest`; bad: immediate | 2 AntHill buildings of the monster team within ±100, random facing, `m_numAntsInside = 75 − syncrand()%25` (51–75); plus 1 ArmyAnt spawned at pos to wake the monster team. | good pool: ≤ 4 nests counting pending tasks ×2 | team: "*T creates an Ants Nest!" / monster: "Ants Nest!" | `icon_antnest` | `Crate::RunAntNest` |
| Eggs | good, start | Task `TypeEggSpawn` | `2 + syncrand()%3` (2–4) TriffidEgg entities **of the capturer's team** from pos+20 y, vel (SFRAND(1),4,SFRAND(1)) scaled to `50 + syncfrand(50)`; spawn type uniform over {Virii, Centipede, Spider, TriffidEggs, SporeGenerator} (rejects Spirits, Eggs, SoulDestroyer, Ants, Darwinians); timer `4 + syncfrand(4)` s; roam range 300. | — | "*T creates some Eggs!" / auto: "Egg Spawn!" | `icon_egg` | `Crate::RunEggs` |
| Meteor | good, start | Task `TypeMeteorShower` | `5 + syncrand()%5` (5–9) Meteor entities of `team`, spawned at random world (FRAND) x,z, y = 1500 + `syncfrand(500)`, each targeting pos ± `syncsfrand(420)` (±210; area = 70 × 6) at ground − 10. | — | "*T causes a Meteor Shower!" | `icon_meteorshower` | `Crate::RunMeteorShower` |
| CrateMania | good | **Immediate**, global | `int(5 + syncfrand(5))` (5–9) calls of `Location::CreateRandomCrate()` (normal placement rules; ignores maxCrates). | invalid when `s_crates ≥ 4` | "Crate Mania!" (name + '!') | — | `Crate::RunCrateMania` |
| SpawnMania | good | **Immediate**, global (all teams) | `Location::m_spawnManiaTimer = 5.0` s: while > 0, every owned, un-locked SpawnPoint requests a spirit every tick (`SpawnPoint::Advance`). HUD "SPAWN MANIA: *T" top-centre, red when < 2 s. | needs spawn points | "Spawn Mania!" | — | `Crate::RunSpawnMania`; `code/worldobject/spawnpoint.cpp`; `code/multiwinia.cpp` render |
| Bitzkreig ("Mega Rage") | good | **Immediate**, global (all teams) | `Location::m_bitzkreigTimer = 15.0` s: `Darwinian::HasRage()` true for every Darwinian of every team. HUD "MEGA RAGE: *T". | — | "Mega Rage!" | — | `Crate::RunBitzkreig`; `code/worldobject/darwinian.cpp:Darwinian::HasRage` |
| Rocketwinians | good | **Immediate**, capturer, **permanent** | `team->m_evolutions[Team::EvolutionRockets] = 1` (Darwinians throw rockets instead of grenades; P04). | — | "*T gets Rocketwinians!" | — | `Crate::RunRocketDarwinians` |
| SlowDown | good | **Immediate**, global | If not game over: `g_gameTimer.RequestSpeedChange(0.2 + syncfrand(0.2), 0.05, 15.0)`: speed approaches target by 5 % of the gap per tick, holds for 15.0 (decremented 0.1 per `GameTimer::Tick` once reached), then requests 1.0. | ignored if `LockSpeedChange` | `crate_message_slowdown` "Sloooow Dooown..." | — | `Crate::RunSlowDownTime`; `code/gametimer.cpp` |
| SpeedUp | good | **Immediate**, global | As above with target `1.3 + syncfrand(0.7)` (1.3–2.0). | | "Speed up!!" | — | `Crate::RunSpeedUpTime` |
| Infection (Spam) | **bad** (good only via forced id → Task `TypeInfection`) | Immediate, monster team | 100 `SpamInfection` effects of the monster team at pos, vel = up + (SFRAND(1), FRAND(2), SFRAND(1)) scaled to 100. | — | `crate_message_auto_infection` "Infection Outbreak!" | `icon_infection` | `Crate::RunInfection` |
| Randomiser | **bad** (good only via forced id) | Immediate | `Randomiser` effect placed at pos (behaviour P05: continually converts Darwinians in range to random teams). | — | "Randomiser" | `icon_randomizer` | `Crate::RunRandomiser` |
| Plague | **bad** | Immediate | **Every** Darwinian (all teams, including the capturer) within 70: `Darwinian::Infect()`. Forced-id Plague in `CaptureCrate` hits the default `AppReleaseAssert` (case commented out). | — | `crate_message_auto_plague` "Plague Outbreak!" | `icon_plague` | `Crate::RunPlague` |
| Triffids | **bad** (good only via forced id) | Immediate, monster team | `1 + syncrand()%3` (1–3) Triffid buildings within ±25, front = away from pos, pitch `0.6 + syncfrand(0.6)`, force `60 + syncfrand(60)`, variance `syncfrand(0.5)`, reload `40 + syncfrand(60)` s, may spawn every type except Spirits and Eggs. | — | "Triffids!" | `icon_triffid` | `Crate::RunTriffids` |
| SpaceShip | **bad** (good only via forced id) | Immediate, Futurewinian team (5) | One SpaceShip entity spawned at a random land position (height ≥ 10, re-rolled), y = 5000; abducts Darwinians (P05). `s_spaceShipUsed = true` → once per game. | once per game; never in Assault | `crate_message_spaceship` "Futurewinians!" | — | `Crate::RunSpaceShip` |
| TheBeast | bad [B:DEBUG] | Immediate, monster team | Portal building of the monster team at pos. | — | "The Beast Cometh!" | — | `Crate::RunTheBeast` |

Global/untargeted immediate effects pass team 255 to the sound and no team to the message [C].

## [→ 04_crates_powerups_and_weapons] 7. Per-level exclusion — `InvalidCrates`

- **Syntax** (inside `MultiwiniaOptions_StartDefinition … MultiwiniaOptions_EndDefinition`): `InvalidCrates <Key> <Key> …` where each key is matched case-insensitively (`stricmp`) against `Crate::GetName(i)`; unknown keys are ignored silently [C] `code/level_file.cpp:LevelFile::ParseMultiwiniaOptions`.
- Storage: `bool LevelFile::m_invalidCrates[Crate::NumCrateRewards]`, zeroed in both `LevelFile` constructors; written back by `LevelFile::WriteMultiwiniaOptions` as `\tInvalidCrates\t<Key>\t…` (after `LevelOptions`, before `PlayTime`) only if any are set [C] `code/level_file.cpp:LevelFile::WriteMultiwiniaOptions`.
- Consumer: last step of `Crate::IsValid` (good pool, StartingPowerups). Not consulted by the bad pool [C].
- Editor: `InvalidCratesWindow` — one checkbox per reward (`crate_<i>`, caption `GetNameTranslated`), bound directly to `m_invalidCrates[i]` [B:EDITOR] [C] `code/interface/invalidcrates_window.cpp`.
- Because the array is sized by `NumCrateRewards`, level files are binary-compatible across DEBUG/FULL only because TheBeast is last [I].

Shipped usage [C] grep of `data/levels`:

| Level | InvalidCrates |
|---|---|
| `mp_assault_2P_1.txt` | Squad Airstrike Nuke Meteor |
| `mp_assault_2p_3.txt` | Squad Airstrike Nuke Meteor |
| `mp_assault_3P_2.txt` | Armour |
| `mp_assault_3p_1.txt` | Armour |
| `mp_assault_4P_1.txt` | Armour |
| `mp_assault_4P_2.txt` | Armour |
| `mp_rocketriot_2P_4.txt` | Armour (redundant: Armour is already invalid in RocketRiot) |
| `old/mp_assault_3P_1.txt` (unused) | SpaceShip |

## [→ 04_crates_powerups_and_weapons] 8. Non-crate grants that reuse crate rewards

- **StartingPowerups** (`Location::InitialiseTeam`, multiplayer only, Domination/KOTH option 0–3): for each of N, draw `syncrand() % NumCrateRewards` until not in {Randomiser, CrateMania, Darwinians, Bitzkreig, SpawnMania, Plague, Rocketwinians, SpaceShip, Infection, Triffids, SpeedUp, SlowDown, MagicalForest, [TheBeast]} and `IsValid` → `Crate::CaptureCrate(type, team)` (static overload: always a task; MagicalForest/RocketDarwinians cases exist but excluded here). Pool = 18 types (Squad, Harvester, Engineer, Armour, Airstrike, Nuke, PersonalShield, Subversion, HotFeet, GunTurret, AntNest, Eggs, Meteor, DarkForest, Rage, RocketTurret, FlameTurret, NapalmStrike) minus `IsValid` exclusions; basic-only → {Squad, Engineer, Armour, Airstrike, GunTurret}. Then every task of that team (except Shaman) gets `m_startTimer = 30.0` (unusable for 30 s). Skipped for monster/Futurewinian teams [C].
- **Retribution** (`Multiwinia::AdvanceRetribution`): every 45 s an eliminated team with spare task capacity would receive a random task from {Airstrike, Airstrike, Meteor, MagicalForest, DarkForest, Eggs} (only Airstrike if basic-only), source tagged 2. **Never called** → dead in FULL [C]. The static `Crate::CaptureCrate(int,int)` asserts for types without a case (Darwinians, Infection, Randomiser, Plague, CrateMania, SpawnMania, Bitzkreig, SpaceShip, Triffids, SlowDown, SpeedUp, TheBeast) [C].
- Reinforcements (`Multiwinia::AdvanceReinforcements`, trunk-port armour etc.) use `TaskManager::RunTask` directly — owned by P02.

## [→ 04_crates_powerups_and_weapons] 9. Crate presentation (render, audio, UI)

Rendering [C] `code/worldobject/crate.cpp:Crate::Render*` (render-side only, not simulated):
- Shape `researchitem.shp`; tumbles each frame: rotation axis = up rotated by `g_gameTime×1.0` about X and `×0.7` about Z; position predicted `pos + vel × predictionTime`.
- Falling: `RenderBubble` (camera-facing quad `textures/bubble.bmp`, white α150, size = 2×radius), `RenderGroundMarker` (rotating `icons/target.bmp` decal on the terrain under the crate at −0.3 rad/s, with a blurred shadow texture generated once by `BitmapRGBA::ApplyBlurFilter(10)` and cached as `shadow_icons/target.bmp`), `RenderGlow`.
- On ground: bubble "pop" — three quarter-quads drifting apart while `m_bubbleTimer` (starts 100) decreases by `40 × SERVER_ADVANCE_PERIOD` **per rendered frame** (frame-rate dependent; render code mutates object state) [C]; then glow only.
- `RenderGlow`: while falling, `numStars` (10 / 5 / 2 for pref `RenderBuildingDetail` 1/2/3) red-orange starbursts `textures/starburst.bmp` (colour 1.0,0.4,0.2, α = 0.6 × y/1300, size i×15); always: `maxBlobs` (20 / 10 / 0) `textures/cloudyglow.bmp` blobs around the centre, colour = capturing team colour α155 or (255,155,100,55) neutral.
- `IsInView` also true if the point above/below the crate at camera height or ground is in the frustum (keeps falling crates rendered) [C].
- Network iframes write zero vectors for a crate's front/up/centre (orientation is cosmetic) [C] `code/network/iframe.cpp`.

Audio [C] `Crate::ListSoundEvents`: building events `Falling`, `Land`, `BeginCapture`, `Capturing_0pc`, `Capturing_20pc`, `Capturing_40pc`, `Capturing_60pc`, `Capturing_80pc`, `Captured`, `LostCapture`, `Expired`; power-up events named by `GetName(type)` in sound-source type `TypeCratePowerup` (enumerated by `code/sound/soundsystem.cpp` over all `NumCrateRewards`).

UI: crate off-screen/on-screen marker (§MarkerSystem); `MultiwiniaHelp` "crates" help tab appropriate during grace period when `Crate::s_crates > 0` [C] `code/multiwiniahelp.cpp:MultiwiniaHelp::CratesOnLevel`; `NotifyCapturedCrate` increments the tab's accomplished count. Help-window tooltips `crate_tooltip_<Key>` and optional `crate_hint_<Key>` [B:DEBUG]. Messages go through `Location::SetCurrentMessage(msg, team, noOverwrite)` (3.0 s real time, team used for colour) [C] `code/location.cpp`.

