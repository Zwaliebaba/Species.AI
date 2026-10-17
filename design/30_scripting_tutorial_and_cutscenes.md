# 30 — Scripting, tutorials and cutscenes

> Part B · Baseline build: Release|Win32 (OpenGL) + TARGET_PC_FULLGAME ([B:FULL]); see _work/build_matrix.md.
> Evidence: [C] confirmed · [I] inferred · [U] unknown. Citations are `path:Symbol` relative to the multiwinia/ source root.
> Sources merged: P19_help_tutorial_script (§1, §2, §4, §5, §7 and closing sections), P20_sequences_loaders_bench (§6, §7, §8 benchmark, closing sections), P10_input_camera (§3 camera modes, §3.8 scripted camera, constants), P11_terrain_world (level-file `CameraMounts`/`CameraAnimations` blocks, map CRC script), X1a_text_formats (§2.10–2.11 camera blocks, tokeniser), P15_network (§12.6 command-history record/replay, late join), routed/_meta_* (appendix, contradictions, unknowns, IP, discrepancies). Every numeric constant and state transition in §2–§7 was re-read in the source for this document; corrections are marked "Resolved:".

## Contents

- [1 Reachability summary](#1-reachability-summary)
- [2 Script language (`Script`)](#2-script-language-script)
  - [2.1 Ownership, lifetime and tick](#21-ownership-lifetime-and-tick)
  - [2.2 Lexical and syntactic grammar](#22-lexical-and-syntactic-grammar)
  - [2.3 Command reference — all 43 opcodes](#23-command-reference--all-43-opcodes)
  - [2.4 Execution model](#24-execution-model)
  - [2.5 Start paths (triggers)](#25-start-paths-triggers)
  - [2.6 Script file register — one row per `data/scripts/*` file](#26-script-file-register--one-row-per-datascripts-file)
  - [2.7 Script-engine constants](#27-script-engine-constants)
  - [2.8 Determinism and safety notes](#28-determinism-and-safety-notes)
- [3 Live Multiwinia tutorials [B:FULL]](#3-live-multiwinia-tutorials-bfull)
  - [3.1 Overview and setup](#31-overview-and-setup)
  - [3.2 Start and end flow](#32-start-and-end-flow)
  - [3.3 Director mechanics (shared)](#33-director-mechanics-shared)
  - [3.4 Gameplay overrides while a tutorial runs](#34-gameplay-overrides-while-a-tutorial-runs)
  - [3.5 Tutorial 1 — "The Basics" (`mp_koth_2P_1`)](#35-tutorial-1--the-basics-mp_koth_2p_1)
  - [3.6 Tutorial 2 — "Advanced Concepts" (`mp_koth_3P_1`)](#36-tutorial-2--advanced-concepts-mp_koth_3p_1)
  - [3.7 Instruction tabs and phrase ids](#37-instruction-tabs-and-phrase-ids)
  - [3.8 Failure, skip and restart paths](#38-failure-skip-and-restart-paths)
  - [3.9 Uncertainties and their resolution status](#39-uncertainties-and-their-resolution-status)
  - [3.10 Determinism caveat](#310-determinism-caveat)
  - [3.11 Tutorial constants](#311-tutorial-constants)
- [4 Camera animation and camera mounts](#4-camera-animation-and-camera-mounts)
- [5 Start sequence, logo splash and demo end sequence](#5-start-sequence-logo-splash-and-demo-end-sequence)
- [6 Dr Sepulveda dialogue system [B:DEAD]](#6-dr-sepulveda-dialogue-system-bdead)
- [7 Demo recording, playback and replay](#7-demo-recording-playback-and-replay)
- [8 What the new game needs](#8-what-the-new-game-needs)
- [9 Darwinia demo tutorials (`tutorial*.cpp`) [B:DEAD] — reference tables](#9-darwinia-demo-tutorials-tutorialcpp-bdead--reference-tables)
- [Open questions and unknowns](#open-questions-and-unknowns)
- [Docs-vs-code discrepancies](#docs-vs-code-discrepancies)
- [IP items](#ip-items)
- [Source map](#source-map)
- [Appendix — dead/legacy code](#appendix--deadlegacy-code)

---

## 1 Reachability summary

Multiwinia inherited Darwinia's whole cinematic stack — a line-oriented cutscene script interpreter, a scripted camera with mounts and animations, a narrator character ("Dr Sepulveda"), chapter-based demo tutorials, a first-time intro and a demo-end sequence. In the shipped PC build almost none of it is reachable. The only live "tutorial / cutscene" content in [B:FULL] is the **pair of Multiwinia tutorials**, which are not scripts at all but a per-frame *help-tab director* inside `MultiwiniaHelp` running on top of ordinary King-of-the-Hill matches.

| # | Component | Files | Body compiled in [B:FULL]? | Reachable by a player in [B:FULL]? | Tag | Evidence |
|---|---|---|---|---|---|---|
| R1 | Script interpreter `Script` | `code/script.{h,cpp}` | Yes, except bodies of `Say`, `ShutUp`, `Highlight`, `ClearHighlights`, `DemoGesture`, `SetCutsceneMode`, the busy/speech wait checks and `ShowDemoEndSequence`, which are hollowed out by `USE_SEPULVEDA_HELP_TUTORIAL` | Ticked every frame (`Script::Advance` in the location loop), but **no Multiwinia path ever opens a script** (§2.5) and `scripts/*` is excluded from the PC data archive | **[B:FULL] engine / [B:LEGACY] content** | [C] `code/main.cpp` location loop; `code/script.cpp:Script::RunScript`; `tools/multiwinia_exclusions.txt` L36 |
| R2 | 42 cutscene scripts | `data/scripts/*.txt` | data | No (37 only via Darwinia triggers T1–T5, 5 via dead attract mode or nothing) | [B:LEGACY] ×37, [B:DEAD] ×5 | §2.6 |
| R3 | Script triggers T1–T6 | `code/worldobject/scripttrigger.cpp`, `code/worldobject/switch.cpp`, `code/global_world.cpp`, `code/taskmanager_interface_*.cpp`, `code/worldobject/rocket.cpp`, `code/attract.cpp` | Yes | No (blocked buildings, no global events, no cutscene objectives, multiplayer guard, attract mode never constructed) | [B:LEGACY] (T1–T5), [B:DEAD] (T6) | §2.5 |
| R4 | Multiwinia tutorials 1 and 2 (help-tab director) | `code/multiwiniahelp.{h,cpp}` (`AttachTutorialTo3d`, `AttachTutorial2To3d`, `RunTutorialHelp`, `RenderTab3d`), `code/UI/GameMenuWindow.cpp`, `code/UI/TutorialButton.h` | Yes (`INCLUDE_TUTORIAL` is defined for `TARGET_PC_FULLGAME`) | **Yes** — Main menu → Tutorials | **[B:FULL]** | [C] `code/lib/universal_include.h` FULLGAME block L97; `code/multiwiniahelp.cpp:MultiwiniaHelp::Advance` |
| R5 | Scripted camera: mounts | `code/camera.cpp:Camera::SetTarget(char const*)`, `code/level_file.cpp:LevelFile::ParseCameraMounts` | Yes | **Yes** — every Multiwinia level start cuts to mount `start`, then to `player<N>` | **[B:FULL]** | [C] `code/main.cpp:DoEnterLocation`, location loop |
| R6 | Scripted camera: animations, MoveToTarget, BuildingFocus, sphere-world scripted/intro/outro/focus modes | `code/camera.cpp` | Yes | No — every shipped `CameraAnimations` block is empty and only scripts/editor/intro request these modes | [B:LEGACY] | §4 |
| R7 | First-time intro `StartSequence` | `code/startsequence.{h,cpp}` | Yes | No — constructed only in `RunBootloader`, which is `#ifdef USE_LOADERS` (off) | [B:LEGACY] (compiled, never constructed) | [C] `code/main.cpp:RunBootloader` |
| R8 | "Darwin logo" splash (`ShowDarwinLogo`) | `code/script.cpp`, `code/renderer.cpp:Renderer::Render` | Yes | No (script-only) | [B:LEGACY] | §5.2 |
| R9 | `DemoEndSequence` | `code/demoendsequence.{h,cpp}` | No (whole file under `USE_SEPULVEDA_HELP_TUTORIAL`) | No | [B:DEAD] | [C] `code/demoendsequence.cpp` L2 |
| R10 | Dr Sepulveda narrator | `code/sepulveda.{h,cpp}`, `code/sepulveda_strings.cpp` | No (whole files under `USE_SEPULVEDA_HELP_TUTORIAL`) | No | [B:DEAD] | [C] `code/sepulveda.cpp` L3/L1494; `code/app.h` member gated |
| R11 | Darwinia demo tutorials | `code/tutorial.{h,cpp}`, `code/tutorial_demo1.cpp`, `code/tutorial_demo2.cpp` | No (whole files under `USE_SEPULVEDA_HELP_TUTORIAL`) | No | [B:DEAD] | [C] `code/tutorial.h` L5–L88 |
| R12 | Darwinia context help `HelpSystem` | `code/helpsystem.{h,cpp}` | No | No | [B:DEAD] (owned by [09](09_meta_systems.md)) | [C] `code/helpsystem.cpp` |
| R13 | Attract mode (idle fly-through) | `code/attract.{h,cpp}` | Yes (no file gate) | No — object constructed and advanced only under `ATTRACTMODE_ENABLED` (defined nowhere) | [B:DEAD] (feature) | [C] `code/main.cpp` L583–588, L752–754 |
| R14 | Gesture demos (recorded mouse paths replayed by the narrator) | `code/gesture_demo.{h,cpp}`, `data/gesture_demos/*` | No | No | [B:DEAD] | [C] `code/gesture_demo.cpp`; `tools/multiwinia_exclusions.txt` L34 |
| R15 | Network command-history record/replay (`RecordDemo` preference) | `code/main.cpp:UpdateAdvanceTime`, `code/app.cpp:App::ShutdownCurrentGame`, `code/network/server.cpp:Server::SaveHistory`/`LoadHistory` | Yes | Hidden preference only (not in `data/default_preferences.txt`, no UI) | [B:FULL] (developer feature) | §7 |
| R16 | Late-join replay of the lockstep stream from sequence 0 | `code/network/clienttoserver.cpp`, `code/network/server.cpp` | Yes | Yes (implicit in joining) | [B:FULL] (owned by [22](22_determinism_and_networking.md)) | §7.3 |
| R17 | AVI capture with fake time (`GrabberWindow`, `AVIGenerator`) | `code/interface/grabber_window.cpp`, `code/lib/avi_generator.cpp` | Only under `AVI_GENERATOR` (`TARGET_DEBUG`) | Debug menu only | [B:DEBUG] | [C] `code/lib/universal_include.h` L125 |
| R18 | Benchmark telemetry recorder + FTP upload | `code/benchmark.{h,cpp}` | No (`BENCHMARK_AND_FTP` commented out) | No | [B:DEAD] (owned by [32](32_debug_test_and_performance.md)) | §7.4 |

```mermaid
flowchart TD
    subgraph LIVE["Reachable in Release|Win32 + TARGET_PC_FULLGAME"]
        MENU["Main menu: Tutorials page"] --> TB["TutorialButton (1 or 2)"]
        TB --> KOTH["Local single-player KOTH match vs Easy CPU"]
        KOTH --> DIR["MultiwiniaHelp director: AttachTutorialTo3d / AttachTutorial2To3d"]
        DIR --> TABS["RenderTab3d instruction tabs"]
        LOAD["Every level load"] --> MOUNT["Camera cut to mount start then player N"]
        TICK["Location loop each frame"] --> SADV["Script::Advance (idle: no reader open)"]
    end
    subgraph LEGACY["Compiled, unreachable in Multiwinia"]
        T1["T1 ScriptTrigger (blocked building)"] --> RS["Script::RunScript"]
        T2["T2 FenceSwitch (blocked building)"] --> RS
        T3["T3 game.txt event action (no events loaded)"] --> RS
        T4["T4 objective cutscene click (no cutscene objectives)"] --> RS
        T5["T5 EscapeRocket victory (single-player only)"] --> RS
        RS --> SCR["42 data/scripts files (not shipped)"]
        SCR --> CAM["CamAnim / CamMove / BuildingFocus camera modes"]
        SS["StartSequence (only from RunBootloader)"]
    end
    subgraph DEAD["Not compiled / never constructed"]
        SEP["Sepulveda narrator"]
        TUT["Demo1Tutorial / Demo2Tutorial"]
        HS["HelpSystem"]
        DES["DemoEndSequence"]
        AM["AttractMode -> T6"]
        RB["RunBootloader (USE_LOADERS)"] --> SS
    end
```

Design consequences:

1. **The Multiwinia tutorial has no narrator, no voice, no script file and no cutscene.** It is a context-sensitive director that re-evaluates world state every frame and shows at most one floating instruction tab (§3). [C] `code/multiwiniahelp.cpp:MultiwiniaHelp::Advance` (L1232–1244).
2. **The script engine is a reference design only.** The 43-command language (§2.3), its six trigger types (§2.5) and the camera animation player (§4) are complete and precise enough to re-implement, but they ship unused. [C]
3. **The Darwinia campaign that would use scripts is only reachable from a `TARGET_DEBUG` build** (main-menu entries `PageDarwinia` → Prologue / Campaign exist only behind the debug single-player page) and even then the level and script data are excluded from the PC archives (`levels/map_*`, `levels/mission_*`, `scripts/*`). [C] `code/UI/GameMenuWindow.cpp` (`PageDarwinia` entered only from `PageDemoSinglePlayer`, see [09](09_meta_systems.md)); `tools/multiwinia_exclusions.txt` L24–25, L36. Trigger T1 (`ScriptTrigger`) is dead even in debug builds because `BLOCK_OLD_DARWINIA_OBJECTS` (defined in FULL and DEBUG) makes `Building::CreateBuilding(char*)` skip blocked types. [C] `code/worldobject/building.cpp:Building::CreateBuilding`, `Building::BuildingIsBlocked`.

---

## 2 Script language (`Script`)

### 2.1 Ownership, lifetime and tick

- One global instance `g_app->m_script`, created in `App::App` (`m_script = new Script()`), deleted in `App::~App`. [C] `code/app.cpp:App::App`, `App::~App`.
- Constructor state: `m_in = NULL`, `m_waitUntil = −1.0`, all wait flags false, `m_requestedLocationId = −1`, `m_permitEscape = false`. `m_darwinianResearchLevel`, `m_rocketId`, `m_rocketState`, `m_rocketData` are **not initialised** (set before use by the commands that read them). [C] `code/script.cpp:Script::Script`.
- `Script::Advance()` is called **once per rendered frame**:
  - in the location game loop, after `TaskManagerInterface::Advance` and before the (dead) `Sepulveda::Advance`; the camera has already been advanced earlier in the same frame, so a camera command issued by a script takes effect on the next frame's `Camera::Advance`. [C] `code/main.cpp` location loop (L553 camera, L568 script).
  - in `GlobalWorldGameLoop` (Darwinia world map) after `ClientToServer::AdvanceAndCatchUp`. [C] `code/main.cpp` (L742). That loop exits immediately when `m_atMainMenu` is set, which is always the case in Multiwinia. [I] (Multiwinia returns to `MainMenuLoop`.)
- The interpreter is **client-local and not networked**: it never sends network messages; its timers use wall-clock time. [C] `code/script.cpp` (no network calls). Several commands mutate simulation state directly (§2.8).
- `IsRunningScript()` = `m_in != NULL`, forced `false` whenever `g_app->m_testHarness` exists (always `NULL` in FULL). [C] `Script::IsRunningScript`. Consumers: ESC routing in the location loop, `ControlHelpSystem` hint suppression, `ScriptTrigger` (no new trigger while a script runs), `GlobalWorld::EvaluateEvents` (suspended while a script runs), `EnterGlobalWorld` (Prologue), `Tutorial` (dead), `AttractMode` (dead). [C] `code/main.cpp` L469, L1412; `code/control_help.cpp` L943; `code/worldobject/scripttrigger.cpp:ScriptTrigger::Advance`; `code/global_world.cpp` L2275.

### 2.2 Lexical and syntactic grammar

Location and loading [C] `code/script.cpp:Script::RunScript`:
- File `data/scripts/<name>` opened with `g_app->m_resource->GetTextReader("scripts/" + name)` (archive or loose file).
- `RunScript(name)` only takes the `.txt` branch when `strstr(name, ".txt")` is non-NULL. A name **without** `.txt` is the "phrase-dump" form: push language phrases `<name>_1`, `<name>_2`, … (until the first missing id) into the narrator queue — compiled only under `USE_SEPULVEDA_HELP_TUTORIAL`, so in FULL a non-`.txt` name is silently ignored. [C]
- Demo gate: when `!g_app->IsFullVersion()` and the name starts with `launchpad_purpose` (17-character `strnicmp`), `RunScript` returns without running. [C] [B:DEMO runtime gate on legacy content].

Lexical rules — the shared `TextReader` tokeniser (full rules: [29 — text-file tokeniser](29_data_formats_reference.md)) [C] `contrib/systemiv/lib/filesys/text_stream_readers.cpp:TextReader::CleanLine`, `TextReader::GetNextToken`, `TextReader::TokenAvailable`:
- One physical line = one statement. `\n`/`\r` are stripped.
- `#` starts a comment to end of line, also mid-line (e.g. `GodDishSpawnResearch  # Research comes through`).
- Three or more adjacent `<`/`>` characters trigger `AppDebugAssert` (merge-conflict guard; no effect in release).
- Optional "redshirt2" line-obfuscation header is supported by the reader; no script uses it.
- Token separators: any run of the characters space, TAB, `\n`, `\r`, `:`, `,` (`DEFAULT_SEPERATOR_CHARS " \t\n\r:,"`). Leading separators are skipped (indented lines are fine). Tokens cannot contain separators, so mount names, phrase ids and file names are single words.
- `GetNextToken()` returns `NULL` when the line has no further token.

Syntax [C] `code/script.cpp:Script::AdvanceScript`, `Script::GetOpCode`:
- The first token is the command keyword, matched **case-insensitively** (`stricmp`) against the 43 names of `g_opCodeNames` (so `CamFOV` in `launchpad_purpose.txt` resolves to `CamFov`). Unknown keyword → opcode −1 → `AppDebugAssert(false)` (no-op in release; the line is skipped).
- The second token (if present) is read once as a string (`nextWord`) and as a number (`nextFloat = iv_atof(nextWord)`, 0.0 if absent). Commands that need more arguments pull further tokens with `GetNextToken()` **without** checking availability: a missing mandatory 2nd–4th argument passes `NULL` to `iv_atof`/`atoi`/`stricmp` → undefined behaviour (likely crash). [I] (`iv_atof(NULL)` is not guarded in `contrib/systemiv/lib/math/math_stubs.cpp:iv_atof`.) `Wait` with no argument is `Wait 0` (uses `nextFloat`). Extra trailing tokens are ignored (`CamCut booster1 1`).
- Building ids are parsed as `(int)nextFloat`.

EBNF (over physical lines after cleaning):
```
script      = { line } ;
line        = [ sep ] [ statement ] EOL ;            (* comments already removed *)
statement   = keyword { sep arg } [ sep ] ;
sep         = sepchar { sepchar } ;
sepchar     = " " | TAB | ":" | "," ;
keyword     = (* one of the 43 opcode names of §2.3, case-insensitive *) ;
arg         = token ;                                (* typed per command, §2.3 *)
token       = tokchar { tokchar } ;
tokchar     = (* any character except sepchar, CR, LF, NUL and "#" *) ;
```

Minimal example (written for this document, not from the game):
```
# fly to two mounts, then hand the camera back
PermitEscape
CamMove      overview   4
WaitCam
CamFov       40  0
Wait         2
CamReset
```

### 2.3 Command reference — all 43 opcodes

`g_opCodeNames` has 43 entries and `GetOpCode` asserts `sizeof(g_opCodeNames)/sizeof(char*) == OpNumOps`. [C] `code/script.cpp:Script::GetOpCode`; `code/script.h:Script` enum. "Blocking" = the command sets a condition that stops `Advance` from reading further lines until it holds (§2.4); every other command executes and the interpreter proceeds to the next line on the **next frame**. "Sim" = mutates simulation state on the local peer only. "Build" = where the command has an effect.

| # | Keyword | Arguments (types) | Semantics | Blocking | Effects / side effects | Build | Ref |
|---|---|---|---|---|---|---|---|
| 0 | `CamCut` | `mount` (word) | Requires a location (else no-op). `Camera::SetTarget(mount)` — level-file camera mount by name, case-insensitive; magic name `CamPosBefore` = pose recorded before the last animation/`RecordCameraPosition` — then `CutToTarget()` (instant pose change, `m_height = target.y`). `AppDebugAssert` if the mount is missing (release: camera cuts to the previous target). | No | camera pose | FULL (legacy content) | [C] `Script::RunCommand_CamCut`; `code/camera.cpp:Camera::SetTarget(char const*)`, `Camera::CutToTarget`; `code/level_file.h` `MAGIC_MOUNT_NAME_START_POS` |
| 1 | `CamMove` | `mount` (word), `duration` (float, s) | Requires a location. If the mount exists: `SetTarget(mount)`, `RequestMode(ModeMoveToTarget)` (records start pose/time), `SetMoveDuration(duration)`; silently ignored if the mount is missing. Motion profile §4.4. `CamMove m 0` followed by `CamCut m` is the scripts' idiom for a hard cut that also resets orientation. | No (pair with `WaitCam`) | camera mode | FULL | [C] `Script::RunCommand_CamMove`; `Camera::AdvanceMoveToTargetMode` |
| 2 | `CamAnim` | `animName` (word) | Requires a location. `LevelFile::GetCameraAnimId(name)` (case-insensitive); `AppReleaseAssert` (fatal) if missing; `Camera::PlayAnimation(anim)` (§4.3). | No (pair with `WaitCam`) | camera mode | FULL (no script uses it) | [C] `Script::RunCommand_CamAnim`; `Camera::PlayAnimation` |
| 3 | `CamFov` | `fov` (float, degrees, vertical), optional `immediate` (int, default **true**) | `immediate ≠ 0` → `Camera::SetFOV(fov)` (fov and target); `0` → `SetTargetFOV(fov)` (eased, §4.4). Works in or out of a location. | No | camera FOV | FULL | [C] `Script::AdvanceScript` case `OpCamFov`; `Script::RunCommand_CamFov` |
| 4 | `CamBuildingFocus` | `buildingId` (int), `range` (float, wu), `height` (float, wu) | Requires a location. `Camera::RequestBuildingFocusMode(b, range, height)` (orbit, §4.4). Building missing → `AppDebugOut("SCRIPT ERROR : Tried to target non-existent building %d")`. | No (never completes on its own) | camera mode; target FOV reset to 60° | FULL | [C] `Script::RunCommand_CamBuildingFocus`; `Camera::RequestBuildingFocusMode`, `Camera::AdvanceBuildingFocusMode` |
| 5 | `CamBuildingApproach` | `buildingId` (int), `range`, `height` (float, wu), `duration` (float, s) | Requires a location. `Camera::SetTarget(b->m_centrePos, range, height)` (point at `range` from the building along the current camera→building bearing, raised `height` above the building centre, looking at it), `SetMoveDuration(duration)`, `RequestMode(ModeMoveToTarget)`. | No (pair with `WaitCam`) | camera mode | FULL | [C] `Script::RunCommand_CamBuildingApproach`; `Camera::SetTarget(Vector3 const&, float, float)` |
| 6 | `CamLocationFocus` | `locationName` (word; `heaven` = world origin), `fov` (float) | **Only outside a location** (Darwinia world map). Enter `ModeSphereWorldScripted` if not already; `SetTargetFOV(fov)`; `SetTarget(locationPos, (0,0,1), up)`. Unknown location → no-op. | No | camera | FULL code, world map unreachable in MW | [C] `Script::RunCommand_LocationFocus` |
| 7 | `CamGlobalWorldFocus` | — | `Camera::RequestSphereFocusMode()`. | No | camera | FULL (legacy) | [C] `Script::RunCommand_CamGlobalWorldFocus` |
| 8 | `CamReset` | — | Stop any camera animation (`StopAnimation`); in a location → `ModeFreeMovement` and `g_app->m_hideInterface = false`; else `ModeSphereWorld`. | No | camera; interface shown | FULL | [C] `Script::RunCommand_CamReset` |
| 9 | `EnterLocation` | `locationName` (word) | `g_app->m_requestedLocationId = GlobalWorld::GetLocationId(name)`; record it in `m_requestedLocationId`; `m_loadingNewProfile = false`; **`App::LoadProfile()`** (rebuilds `GlobalWorld` from the profile's `game.txt`); copy that location's mission/map file names into `m_requestedMission` / `m_requestedMap`. The main loop then loads the level. | **Yes** — until `g_app->m_locationId == m_requestedLocationId` | level load; profile reload | FULL code; MW never has global locations | [C] `Script::RunCommand_EnterLocation`; `Script::Advance` |
| 10 | `ExitLocation` | — | Clear requested location/mission/map (the main loop leaves the location); `m_requestedLocationId = −1`. | No (scripts follow it with `WaitFade`) | level unload | FULL (legacy) | [C] `Script::RunCommand_ExitLocation` |
| 11 | `Say` | `phraseId` (word) | `Sepulveda::Say(id)` — queue a narrator line (§6.3). | No (pair with `WaitSay`) | — | **no-op in FULL** → [B:DEAD] | [C] `Script::RunCommand_Say` |
| 12 | `ShutUp` | — | `Sepulveda::ShutUp()` — clear caption, queue, highlights, gesture demo. | No | — | no-op in FULL → [B:DEAD] | [C] `Script::RunCommand_ShutUp` |
| 13 | `Wait` | `seconds` (float) | `m_waitUntil = max(m_waitUntil, GetHighResTime() + seconds)` — consecutive waits take the max, they do not add. Compared against `g_gameTime` in `Advance`. | **Yes** — until `g_gameTime ≥ m_waitUntil` | — | FULL | [C] `Script::RunCommand_Wait` |
| 14 | `WaitSay` | — | Set `m_waitForSpeech`; `Advance` would block while `Sepulveda::IsTalking()`. | Flag set, but the check is compiled out → **does not block** in FULL | — | [B:DEAD] effect | [C] `Script::Advance` (`#ifdef USE_SEPULVEDA_HELP_TUTORIAL`) |
| 15 | `WaitCam` | — | Set `m_waitForCamera`; block while `Camera::IsAnimPlaying()` (= an animation is active **or** mode is `ModeMoveToTarget`). Does **not** wait for `ModeBuildingFocus` (which never ends). | **Yes** | — | FULL | [C] `Script::Advance`; `Camera::IsAnimPlaying` |
| 16 | `WaitFade` | — | Set `m_waitForFade`; block while `!Renderer::IsFadeComplete()`. | **Yes** | — | FULL | [C] `Script::Advance` |
| 17 | `WaitRocket` | `buildingId` (int), `state` (word: `Refueling`, `Loading`, `Ignition`, `Ready`, `Countdown`, `Exploding`, `Flight`, case-insensitive), `data` (int) | Block until the EscapeRocket's `m_state ≥ state`; when `m_state == state == Countdown`, additionally until `(int)m_countdown ≤ data`. Building missing or not an EscapeRocket → the wait is cancelled (that frame returns, next frame proceeds). Unknown state word → `GetStateId` −1 → passes at once. | **Yes** | — | FULL | [C] `Script::RunCommand_WaitRocket`, `Script::Advance`; `code/worldobject/rocket.cpp:EscapeRocket::GetStateId` |
| 18 | `WaitPlayerNotBusy` | — | Would call `Demo2Tutorial::BeginPlayerBusyCheck()` and block while `Demo2Tutorial::IsPlayerBusy()` (§9.3). | Flag set but both calls compiled out → **does not block** in FULL | — | [B:DEAD] effect | [C] `Script::RunCommand_WaitPlayerNotBusy`; `Script::Advance` |
| 19 | `Highlight` | `buildingId` (int) | `Sepulveda::HighlightBuilding(id, "ScriptHighlight")` — god-ray beam (§6.5). | No | — | no-op in FULL → [B:DEAD] | [C] `Script::RunCommand_Highlight` |
| 20 | `ClearHighlights` | — | `Sepulveda::ClearHighlights("ScriptHighlight")`. | No | — | no-op in FULL → [B:DEAD] | [C] `Script::RunCommand_ClearHighlights` |
| 21 | `TriggerSound` | `event` (word) | **Music start.** If no instance of `"Music <event>"` is playing, `SoundSystem::TriggerOtherEvent(NULL, event, SoundSourceBlueprint::TypeMusic)` (idempotent start). Event names match case-insensitively. | No | audio | FULL | [C] `Script::RunCommand_TriggerSound`; `code/sound/soundsystem.cpp:SoundSystem::TriggerOtherEvent` (`stricmp`) |
| 22 | `StopSound` | `event` (word) | **Music stop.** `SoundSystem::StopAllSounds(WorldObjectId(), "Music <event>")`. | No | audio | FULL | [C] `Script::RunCommand_StopSound` |
| 23 | `DemoGesture` | `gestureFile` (word, e.g. `officer.txt`) | `Sepulveda::DemoGesture(name, 2.0)` — loop-replay a recorded mouse gesture starting 2.0 s later (§6.5). | No | — | no-op in FULL → [B:DEAD] | [C] `Script::RunCommand_DemoGesture` |
| 24 | `GiveResearch` | `name` (word) | `modsystem` → set pref `ModSystemEnabled = 1` (only under `USE_DARWINIA_MOD_SYSTEM`), save prefs, task-manager research message id 999 for 4.0 s; `accessallareas` → create `<profileDir>users/` and `<profileDir>users/AccessAllAreas/`, message id 998 for 4.0 s; otherwise `GlobalResearch::GetType(name)` → `AddResearch(type)` + message for 4.0 s (unknown name → nothing). | No | **Sim** (research), prefs, file system | FULL (legacy) | [C] `Script::RunCommand_GiveResearch` |
| 25 | `SetMission` | `locationName`, `missionFile` (words) | Set a global location's mission file; `m_missionCompleted = false`. `AppDebugAssert` if the location is missing (release: NULL dereference) [I]. | No | global world | FULL (legacy) | [C] `Script::RunCommand_SetMission` |
| 26 | `GameOver` | — | Camera → `ModeSphereWorldOutro` (after 70 s it runs the credits loader, see [09](09_meta_systems.md)); stop `"Ambience EnterGlobalWorld"`. | No | camera, audio | FULL (legacy) | [C] `Script::RunCommand_GameOver`; `Camera::AdvanceSphereWorldOutroMode` |
| 27 | `ResetResearch` | — | Save the Darwinian research level into `m_darwinianResearchLevel`, set it to 1. | No | **Sim** | FULL (legacy) | [C] `Script::RunCommand_ResetResearch` |
| 28 | `RestoreResearch` | — | Restore the saved Darwinian research level (garbage if `ResetResearch` never ran). | No | **Sim** | FULL (legacy) | [C] `Script::RunCommand_RestoreResearch` |
| 29 | `RunCredits` | — | `Loader::GetLoaderIndex("credits")` (in FULL the only loader, index 0) → `CreateLoader` → synchronous `Run()` (own render loop) → delete. | Synchronous (blocks the whole application until the credits finish or are skipped) | full-screen takeover | FULL (no script uses it) | [C] `Script::RunCommand_RunCredits`; `code/loaders/loader.cpp:Loader::GetLoaderIndex` |
| 30 | `SetCutsceneMode` | `mode` (int) | `Sepulveda::SetCutsceneMode(mode)` (big-face layout). | No | — | no-op in FULL → [B:DEAD] | [C] `Script::AdvanceScript` case `OpSetCutsceneMode` |
| 31 | `GodDishActivate` | — | First `GodDish` building in the location → `Activate()`. | No | **Sim** | [B:LEGACY] (GodDish blocked) | [C] `Script::RunCommand_GodDishActivate`, `GetGodDish` |
| 32 | `GodDishDeactivate` | — | `GodDish::DeActivate()`. | No | **Sim** | [B:LEGACY] | [C] `Script::RunCommand_GodDishDeactivate` |
| 33 | `GodDishSpawnSpam` | — | `GodDish::SpawnSpam(false)`. | No | **Sim** | [B:LEGACY] | [C] `Script::RunCommand_GodDishSpawnSpam` |
| 34 | `GodDishSpawnResearch` | — | `GodDish::SpawnSpam(true)`. | No | **Sim** | [B:LEGACY] | [C] `Script::RunCommand_GodDishSpawnResearch` |
| 35 | `TriggerSpam` | — | `GodDish::TriggerSpam()`. | No | **Sim** | [B:LEGACY] | [C] `Script::RunCommand_SpamTrigger` |
| 36 | `PurityControl` | — | Delete every file in `users/<profile>/`, open the buy-now URL, `exit(0)` (anti-piracy / demo kill switch). | Terminates the process | file system, process | FULL code (no script uses it) | [C] `Script::RunCommand_PurityControl` |
| 37 | `ShowDarwinLogo` | — | `Renderer::m_renderDarwinLogo = GetHighResTime()`; interface sound `ShowLogo` (§5.2). | No | overlay, audio | FULL (legacy) | [C] `Script::RunCommand_ShowDarwinLogo`; `code/renderer.cpp:Renderer::Render` |
| 38 | `ShowDemoEndSequence` | — | `g_app->m_demoEndSequence = new DemoEndSequence()` (§5.3). | No | — | empty body in FULL → [B:DEAD] | [C] `Script::RunCommand_ShowDemoEndSequence` |
| 39 | `PermitEscape` | — | `m_permitEscape = true`: from now until the script ends, `SkipCutscene` aborts the whole script (§2.4). | No | — | FULL | [C] `Script::RunCommand_PermitEscape` |
| 40 | `DestroyBuilding` | `buildingId` (int), `intensity` (float) | `Building::Destroy(intensity)` if the building exists. | No | **Sim** | FULL (no script uses it) | [C] `Script::RunCommand_DestroyBuilding` |
| 41 | `ActivateTrunkPort` | `buildingId` (int) | If a TrunkPort: its `GlobalBuilding::m_online = true` (partial activation, no reprogram effect). | No | **Sim** (global) | FULL (no script uses it) | [C] `Script::RunCommand_ActivateTrunkPort` |
| 42 | `ActivateTrunkPortFull` | `buildingId` (int) | If a TrunkPort: `ReprogramComplete()` (opens the port). | No | **Sim** | FULL (no script uses it) | [C] `Script::RunCommand_ActivateTrunkPort` (shared, `_fullActivation = true`) |

### 2.4 Execution model

State (public members of `Script`) [C] `code/script.h:Script`: `m_in` (open `TextReader` or NULL), `m_waitUntil` (double), `m_waitForSpeech`, `m_waitForCamera`, `m_waitForFade`, `m_waitForPlayerNotBusy`, `m_waitForRocket` (+ `m_rocketId`, `m_rocketState`, `m_rocketData`), `m_requestedLocationId`, `m_darwinianResearchLevel`, `m_permitEscape`.

`RunScript(name)` only opens the reader and returns; the first line runs on the next `Advance`. It **replaces** any running script's reader pointer without deleting the old reader (leak) and without resetting wait flags or `m_permitEscape`. [C] `Script::RunScript`.

Per-frame algorithm [C] `code/script.cpp:Script::Advance`:
```
if controlEvent(SkipCutscene):                       # key Enter, XInput B
    m_waitUntil = g_gameTime
    clear waitForCamera, waitForRocket, waitForPlayerNotBusy
    renderer.m_renderDarwinLogo = -1                 # hide logo splash
    if m_permitEscape:                               # abort the whole script
        delete m_in; m_in = NULL; (Sepulveda.ShutUp — dead)
        StopAllSounds("Music"); m_permitEscape = false
        camera.RequestMode(FreeMovement if in location else SphereWorld)
        return
if waitForFade and not renderer.IsFadeComplete(): return
if m_waitUntil > g_gameTime: return
(if waitForSpeech and sepulveda.IsTalking(): return)                 # compiled out in FULL
if waitForCamera and camera.IsAnimPlaying(): return
(if waitForPlayerNotBusy and Demo2Tutorial.IsPlayerBusy(): return)   # compiled out in FULL
if waitForRocket:
    rocket = building(m_rocketId)
    if rocket missing or not EscapeRocket: waitForRocket = false; return
    if rocket.m_state < m_rocketState: return
    if rocket.m_state == m_rocketState == Countdown and int(rocket.m_countdown) > m_rocketData: return
if m_requestedLocationId != -1:
    if app.m_locationId != m_requestedLocationId: return
    m_requestedLocationId = -1
clear waitForSpeech, waitForCamera, waitForFade, waitForRocket, waitForPlayerNotBusy
if m_in:
    if m_in.ReadLine(): AdvanceScript()              # execute exactly ONE physical line
    else: delete m_in; m_in = NULL; m_permitEscape = false
```

Consequences (all [C] from the code above):
- **One physical line per frame.** Blank and comment-only lines also consume a frame (`AdvanceScript` returns when no token is available). A run of non-blocking commands therefore executes over consecutive frames, never atomically; at 60 fps a 109-line script costs at least 109 frames (1.8 s) on top of its waits. §2.6 gives each file's physical line count.
- Wait conditions are evaluated in the fixed order fade → timer → speech → camera → player-busy → rocket → location, and all flags are cleared together once every condition passes. Several waits can therefore be armed by consecutive lines and are satisfied jointly.
- `SkipCutscene` without `PermitEscape` fast-forwards only the current `Wait`, `WaitCam`, `WaitRocket` and `WaitPlayerNotBusy` — not `WaitFade`, `WaitSay` or `EnterLocation`. It does not stop camera motion: after a skipped `WaitCam` the camera keeps flying while the next lines execute. Bindings `SkipCutscene = key enter down`, `XInput b down`. [C] `data/input/input_preferences.txt` L39–40.
- The skip check runs every frame even when no script is open (it is harmless then: it only rewrites `m_waitUntil` and the logo timer). [C]
- `ESC` (`ControlMenuEscape`) while a script runs is routed to `Sepulveda::PlayerSkipsMessage` (dead in FULL) instead of opening the in-game menu — in FULL, ESC does nothing during a script. [C] `code/main.cpp` location loop (L467–474).
- `Wait` stores `GetHighResTime() + t` but `Advance` compares with `g_gameTime`. In normal play `g_gameTime` is the wall clock sampled at `UpdateAdvanceTime` (frozen while `App::GamePaused()`); in `RecordDemo` mode it is still the real clock (§7.2). So a pause holds a pending `Wait` (the comparison value stops) and the wait ends as soon as the game resumes if real time has passed. [C] `code/main.cpp:UpdateAdvanceTime`; [I] behavioural reading.

```mermaid
stateDiagram-v2
    [*] --> Idle
    Idle --> Loaded: "RunScript(name.txt) opens reader"
    Loaded --> Loaded: "next frame: ReadLine + execute one line"
    Loaded --> Waiting: "line arms a wait (Wait, WaitCam, WaitFade, WaitRocket, EnterLocation)"
    Waiting --> Loaded: "all armed conditions satisfied, flags cleared"
    Waiting --> Loaded: "SkipCutscene clears timer, camera, rocket, busy waits"
    Loaded --> Idle: "end of file (PermitEscape cleared)"
    Waiting --> Idle: "SkipCutscene and PermitEscape: abort, stop Music, camera reset"
    Loaded --> Idle: "SkipCutscene and PermitEscape"
```

### 2.5 Start paths (triggers)

Every call site of `Script::RunScript` [C] (grep of `code/`):

| # | Trigger | Call site | Script source | Reachable in MW FULL? | Evidence |
|---|---|---|---|---|---|
| T1 | `ScriptTrigger` building (one-shot volume trigger placed in a level) | `code/worldobject/scripttrigger.cpp:ScriptTrigger::Trigger` | building field `m_scriptFilename` | No — `TypeScriptTrigger` is in `Building::BuildingIsBlocked`, so `CreateBuilding(char*)` returns NULL for it in FULL **and** DEBUG; no `mp_*` map contains one | [C] `code/worldobject/building.cpp:Building::CreateBuilding`, `Building::BuildingIsBlocked`; grep `data/levels/mp_*.txt` |
| T2 | `FenceSwitch` toggled, `m_script` containing `.txt` | `code/worldobject/switch.cpp` (L227–229; a second call site at L415–418 is commented out) | per-building `m_script` | No — `TypeFenceSwitch` blocked | [C] same |
| T3 | Global-world event action `RunScript <file>` | `code/global_world.cpp:GlobalEventAction::Execute` (L361–363) | `data/game.txt`, `data/game_unlockall.txt`, `data/game_demo2.txt`, `data/purity/game.txt` lines `Action RunScript …` | No — Multiwinia creates a bare `new GlobalWorld` at start-up (`// bool profileLoaded = LoadProfile();` is commented out), so no `game.txt` events are loaded; only the debug Campaign/Prologue entries load a profile | [C] `code/app.cpp:App::App` (L452–453) |
| T4 | Player clicks an objective in the task manager whose `GlobalEventCondition::m_cutScene` is set (third column of a `PrimaryObjectives` line) | `code/taskmanager_interface_multiwinia.cpp` (L929), `code/taskmanager_interface_icons.cpp` (L1415), `code/taskmanager_interface_gestures.cpp` (L771) | level `PrimaryObjectives` block | No — of 57 `mp_*.txt` maps, 45 contain only `NeverTrue`, 2 (`mp_sandbox`, `mp_sandbox_all`) contain two `BuildingOnline :sandbox,2` lines, 10 have no objectives block; none has a cutscene column | [C] census of `data/levels/mp_*.txt` |
| T5 | EscapeRocket ends `StateIgnition` (countdown ≤ 0) in **single-player** with `m_spawnCompleted` → `launchpad_victory.txt` | `code/worldobject/rocket.cpp` (L1196–1198) | hard-coded | No — the branch is the `else` of `if (g_app->Multiplayer())`; Multiwinia is always "multiplayer" | [C] `code/worldobject/rocket.cpp` (`EscapeRocket` ignition advance) |
| T6 | Attract mode start/stop → `attractmode.txt`, `endattract.txt` | `code/attract.cpp:AttractMode::StartAttractMode` (L45), `AttractMode::EndAttractMode` (L61) | hard-coded | No — `AttractMode` is only constructed and advanced under `#ifdef ATTRACTMODE_ENABLED` (never defined); its start binding is commented out too | [C] `code/main.cpp` L583–588, L752–754; `data/input/input_preferences.txt` L280–287 |

Conclusion [C]: in [B:FULL] the interpreter is ticked but has **no reachable entry point**. T3, T4 and T5 become reachable only in a `TARGET_DEBUG` build through the debug-only Prologue/Campaign menu entries, and only with loose Darwinia data, because `levels/map_*`, `levels/mission_*` and `scripts/*` are excluded from the PC archives. [C] `tools/multiwinia_exclusions.txt` L24–25, L36.

**`ScriptTrigger` as the reference trigger design** [C] `code/worldobject/scripttrigger.cpp:ScriptTrigger::Read`, `ScriptTrigger::Advance`, `ScriptTrigger::Trigger`; `code/worldobject/scripttrigger.h`:
- Level line: standard building columns, then `linkId` (int), `range` (float, default 100.0), `scriptFilename` (word), `entityType` (word). Full column grammar: [29](29_data_formats_reference.md).
- `entityType` keywords: `always` (`SCRIPTRIGGER_RUNALWAYS` 999), `never` (998), `camenter` (997), `camview` (996); any other word is an entity type name (`Entity::GetTypeId`, e.g. `Darwinian`, `Squadie`, `Engineer`).
- Polling: a countdown `m_timer` is decreased by `SERVER_ADVANCE_PERIOD` (0.1 s) per building advance and re-armed to 1.0 s, so the condition is tested once per second of simulation time, and only while **no script is running and the camera is interactive**.
- Conditions: `always` → fire; `camenter` → camera within `range`, camera speed < 5 wu/s, camera interactive; `camview` → (within `range + 300`, speed < 5, interactive, view ray hits the sphere of radius `range`) or the `camenter` condition; entity type → at least one entity of that type **owned by the trigger's team** within `range` (entity grid).
- On fire: a `.txt` name runs the script and the trigger deletes itself next advance (`m_triggered = −1`); any other name enters the dead phrase-sequencing mode (with `USE_SEPULVEDA_HELP_TUTORIAL`: say `<name>_1`, `<name>_2`, … one per silence, highlighting `linkId`; without it the trigger simply deletes itself). `never` triggers never fire.
- Determinism hazard: the trigger runs inside the simulation tick but tests the **local camera** (position, velocity, interactivity) and the local script state. [C] — a new game must evaluate camera triggers client-side only and never let them mutate shared state.

### 2.6 Script file register — one row per `data/scripts/*` file

Generated by `OUT/_work/scripts/S_30_script_census.py` (run `python3 -I S_30_script_census.py <multiwinia root>`). *Lines phys/exec* = physical lines (= minimum frames consumed) / executable lines; *Say ids* = narrator phrase ids (all dead in FULL); *ΣWait / ΣCamMove* = sum of explicit `Wait` arguments and of `CamMove` durations (a lower bound on run time ignoring `WaitSay`, fades and level loads); *Live references* = uncommented references with the trigger class of §2.5; tag rule: [B:LEGACY] = started only by triggers T1–T5 (compiled into FULL but unreachable in Multiwinia), [B:DEAD] = started only by the dead attract mode (T6) or by nothing (orphan). The PC data build excludes all 42 files (`tools/multiwinia_exclusions.txt` L36). [C]

Resolved: P19 tagged `launchpad_purpose.txt` [B:LEGACY] but the orphan `gamestart.txt` [B:DEAD], and listed `music4.txt` as "still referenced by `game_unlockall.txt`". The data show both `music4.txt` references (`data/game.txt` L107, `data/game_unlockall.txt` L85) are commented out, so `music4.txt` is an orphan too; all three orphans are tagged [B:DEAD] here. Totals: **42 files = 37 [B:LEGACY] + 5 [B:DEAD]** (`attractmode.txt`, `endattract.txt`, `gamestart.txt`, `launchpad_purpose.txt`, `music4.txt`), superseding the "39 + 3" of the Phase-2 log.

| Script | Lines phys/exec | Commands (opcode×count) | Say ids (n: first..last) | ΣWait / ΣCamMove s | Live references | Commented-out references | Tag |
|---|---|---|---|---|---|---|---|
| `attractmode.txt` | 53/44 | CamMove×17, CamReset×1, EnterLocation×3, ExitLocation×3, SetMission×3, Wait×3, WaitCam×14 | 0 | 21 / 180 | T6 attract mode `code/attract.cpp:L45` | none | [B:DEAD] |
| `biosphere_intro.txt` | 43/25 | CamBuildingFocus×2, CamMove×3, CamReset×1, Say×10, StopSound×1, TriggerSound×1, Wait×1, WaitCam×1, WaitFade×1, WaitSay×4 | 10: `biosphere_primary_1`..`biosphere_primary_10` | 2 / 58 | T4 objective cutscene `data/levels/mission_biosphere_attract.txt:L52`; T1 ScriptTrigger `data/levels/mission_biosphere_recapture.txt:L18`; T4 objective cutscene `data/levels/mission_biosphere_recapture.txt:L47` | none | [B:LEGACY] |
| `biospherealpha_primary.txt` | 26/17 | CamBuildingFocus×3, CamMove×2, CamReset×1, Say×5, WaitCam×1, WaitFade×1, WaitSay×4 | 5: `biospherealpha_primary1`..`biospherealpha_primary5` | 0 / 25 | T1 ScriptTrigger `data/levels/mission_escort_escort.txt:L15`; T4 objective cutscene `data/levels/mission_escort_escort.txt:L45` | none | [B:LEGACY] |
| `containment_end.txt` | 108/71 | CamBuildingFocus×3, CamLocationFocus×5, CamMove×2, CamReset×1, ClearHighlights×4, EnterLocation×2, ExitLocation×2, Highlight×7, Say×26, StopSound×1, TriggerSound×1, WaitFade×5, WaitSay×12 | 26: `containmentend_1`..`containmentend_8` | 0 / 50 | T3 global event `data/game.txt:L76`; T3 global event `data/game_unlockall.txt:L58` | none | [B:LEGACY] |
| `containment_intro.txt` | 40/27 | CamBuildingFocus×3, CamMove×1, CamReset×1, ClearHighlights×1, Highlight×3, Say×11, StopSound×1, TriggerSound×1, WaitFade×1, WaitSay×4 | 11: `containment_intro1`..`containment_rescuedw4` | 0 / 5 | T1 ScriptTrigger `data/levels/mission_containment_enable.txt:L17` | none | [B:LEGACY] |
| `containment_officer.txt` | 13/8 | DemoGesture×2, Highlight×1, Say×3, WaitSay×2 | 3: `containment_officer_1`..`containment_officer_3` | 0 / 0 | T3 global event `data/game.txt:L62`; T3 global event `data/purity/game.txt:L60` | none | [B:LEGACY] |
| `containment_primary1.txt` | 24/16 | CamBuildingFocus×2, CamMove×1, CamReset×1, ClearHighlights×1, Highlight×3, Say×5, WaitSay×3 | 5: `containment_primary_1`..`containment_primary_5` | 0 / 13 | T4 objective cutscene `data/levels/mission_containment_enable.txt:L51` | none | [B:LEGACY] |
| `endattract.txt` | 3/2 | CamReset×1, ExitLocation×1 | 0 | 0 / 0 | T6 attract mode `code/attract.cpp:L61` | none | [B:DEAD] |
| `gameover.txt` | 108/55 | CamCut×3, CamMove×3, EnterLocation×3, ExitLocation×4, GameOver×1, GiveResearch×2, Say×19, TriggerSound×1, Wait×9, WaitCam×2, WaitFade×1, WaitSay×7 | 19: `outro_biosphere_1`..`outro_4` | 46 / 180 | T3 global event `data/game.txt:L155`; T3 global event `data/game_unlockall.txt:L129` | none | [B:LEGACY] |
| `gamestart.txt` | 2/1 | EnterLocation×1 | 0 | 0 / 0 | orphan (no live reference) | none | [B:DEAD] |
| `garden_intro.txt` | 16/11 | Say×5, Wait×2, WaitFade×1, WaitSay×3 | 5: `garden_intro_1`..`garden_intro_5` | 7 / 0 | T1 ScriptTrigger `data/levels/mission_garden_liberate.txt:L15` | none | [B:LEGACY] |
| `garden_opentrunkport.txt` | 14/10 | ClearHighlights×1, Highlight×2, Say×4, WaitSay×3 | 4: `garden_souls_1`..`garden_opentrunkport_2` | 0 / 0 | T1 ScriptTrigger `data/levels/mission_garden_liberate.txt:L14` | none | [B:LEGACY] |
| `generator_primary1.txt` | 44/26 | CamBuildingFocus×5, CamReset×1, ClearHighlights×5, Highlight×1, Say×9, WaitSay×5 | 9: `generator_primary_1`..`generator_panels_7` | 0 / 0 | T4 objective cutscene `data/levels/mission_generator_enable.txt:L44` | none | [B:LEGACY] |
| `launchpad_finalisland.txt` | 39/29 | CamBuildingFocus×4, CamCut×1, CamFov×3, CamMove×2, CamReset×1, PermitEscape×1, Say×9, ShutUp×1, Wait×1, WaitPlayerNotBusy×1, WaitSay×5 | 9: `launchpad_finalisland_1`..`launchpad_finalisland_9` | 1 / 8 | T3 global event `data/game_demo2.txt:L46` | none | [B:LEGACY] |
| `launchpad_intro.txt` | 109/69 | CamBuildingFocus×1, CamCut×8, CamFov×11, CamMove×11, CamReset×1, PermitEscape×1, Say×15, ShowDarwinLogo×1, ShutUp×1, StopSound×1, TriggerSound×1, Wait×13, WaitCam×1, WaitFade×1, WaitRocket×1, WaitSay×1 | 15: `launchpad_intro_1`..`launchpad_intro_15` | 127 / 163 | T1 ScriptTrigger `data/levels/mission_launchpad.txt:L54` | none | [B:LEGACY] |
| `launchpad_primary1.txt` | 22/14 | CamBuildingFocus×3, CamReset×1, PermitEscape×1, Say×6, WaitSay×3 | 6: `launchpad_primary_1`..`launchpad_primary_7` | 0 / 0 | T4 objective cutscene `data/levels/mission_launchpad.txt:L88` | none | [B:LEGACY] |
| `launchpad_primary2.txt` | 22/14 | CamBuildingFocus×3, CamReset×1, PermitEscape×1, Say×6, WaitSay×3 | 6: `launchpad_primary_1`..`launchpad_primary_7` | 0 / 0 | T4 objective cutscene `data/levels/mission_launchpad.txt:L89` | none | [B:LEGACY] |
| `launchpad_primary3.txt` | 22/14 | CamBuildingFocus×3, CamReset×1, PermitEscape×1, Say×6, WaitSay×3 | 6: `launchpad_primary_1`..`launchpad_primary_7` | 0 / 0 | T4 objective cutscene `data/levels/mission_launchpad.txt:L90` | none | [B:LEGACY] |
| `launchpad_primary_rocket.txt` | 18/13 | CamBuildingFocus×1, CamReset×1, ClearHighlights×1, Highlight×3, PermitEscape×1, Say×5, WaitSay×1 | 5: `launchpad_primary_rocket_1`..`launchpad_primary_rocket_5` | 0 / 0 | T4 objective cutscene `data/levels/mission_launchpad.txt:L91` | none | [B:LEGACY] |
| `launchpad_purpose.txt` | 56/36 | CamCut×1, CamFOV×1, CamGlobalWorldFocus×1, CamLocationFocus×2, CamMove×2, CamReset×1, EnterLocation×1, ExitLocation×1, PermitEscape×1, Say×13, ShutUp×1, StopSound×1, TriggerSound×1, Wait×3, WaitFade×2, WaitPlayerNotBusy×1, WaitSay×3 | 13: `launchpad_middemo_1`..`launchpad_middemo_14` | 22 / 60 | orphan (no live reference) | `data/levels/mission_launchpad.txt:L55` | [B:DEAD] |
| `launchpad_victory.txt` | 42/33 | CamBuildingApproach×1, CamBuildingFocus×1, CamCut×4, CamFov×5, CamMove×8, Say×4, ShowDemoEndSequence×1, ShutUp×1, Wait×7, WaitRocket×1 | 4: `launchpad_success_1`..`launchpad_success_4` | 49 / 50 | T5 EscapeRocket victory `code/worldobject/rocket.cpp:L1198` | none | [B:LEGACY] |
| `mine_demo_intro.txt` | 30/21 | CamBuildingFocus×1, CamMove×3, CamReset×1, Say×10, Wait×1, WaitCam×1, WaitFade×1, WaitSay×3 | 10: `tutorial_intro_1`..`tutorial_intro_10` | 6 / 65 | T1 ScriptTrigger `data/levels/mission_mine_demo.txt:L31` | none | [B:LEGACY] |
| `mine_minehelp.txt` | 15/11 | ClearHighlights×1, Highlight×4, Say×4, WaitSay×2 | 4: `pixelmines_progress_1`..`pixelmines_mines_3` | 0 / 0 | T1 ScriptTrigger `data/levels/mission_mine_demo.txt:L27`; T1 ScriptTrigger `data/levels/mission_mine_enable.txt:L19` | none | [B:LEGACY] |
| `mine_primary1.txt` | 31/21 | CamBuildingFocus×1, CamMove×2, CamReset×1, ClearHighlights×1, Highlight×5, Say×8, WaitCam×1, WaitSay×2 | 8: `pixelmines_intro_1`..`pixelmines_mines_3` | 0 / 43 | T4 objective cutscene `data/levels/mission_mine_attract.txt:L58`; T4 objective cutscene `data/levels/mission_mine_demo.txt:L57`; T4 objective cutscene `data/levels/mission_mine_enable.txt:L52` | none | [B:LEGACY] |
| `mine_primary2.txt` | 10/7 | CamBuildingFocus×1, CamReset×1, ClearHighlights×1, Highlight×1, Say×2, WaitSay×1 | 2: `pixelmines_yardport_1`..`pixelmines_yardport_2` | 0 / 0 | T4 objective cutscene `data/levels/mission_mine_attract.txt:L59`; T4 objective cutscene `data/levels/mission_mine_demo.txt:L58`; T4 objective cutscene `data/levels/mission_mine_enable.txt:L53` | none | [B:LEGACY] |
| `music1.txt` | 1/1 | TriggerSound×1 | 0 | 0 / 0 | T3 global event `data/game.txt:L48`; T3 global event `data/purity/game.txt:L46` | none | [B:LEGACY] |
| `music2.txt` | 1/1 | TriggerSound×1 | 0 | 0 / 0 | T3 global event `data/game.txt:L70`; T3 global event `data/game_unlockall.txt:L52`; T3 global event `data/purity/game.txt:L68` | none | [B:LEGACY] |
| `music3.txt` | 1/1 | TriggerSound×1 | 0 | 0 / 0 | T3 global event `data/game.txt:L85`; T3 global event `data/game_unlockall.txt:L67` | none | [B:LEGACY] |
| `music4.txt` | 1/1 | TriggerSound×1 | 0 | 0 / 0 | orphan (no live reference) | `data/game.txt:L107`; `data/game_unlockall.txt:L85` | [B:DEAD] |
| `music5.txt` | 1/1 | TriggerSound×1 | 0 | 0 / 0 | T3 global event `data/game.txt:L123` | none | [B:LEGACY] |
| `patternbuffer_primary1.txt` | 32/20 | CamBuildingFocus×1, CamMove×2, CamReset×1, ClearHighlights×2, Highlight×4, Say×7, WaitCam×1, WaitSay×2 | 7: `yardend_11`..`patternbuffer_primary_5` | 0 / 33 | T4 objective cutscene `data/levels/mission_pattern_buffer_attract.txt:L66`; T4 objective cutscene `data/levels/mission_pattern_buffer_recapture.txt:L58` | none | [B:LEGACY] |
| `patternbuffer_primary2.txt` | 10/7 | CamBuildingFocus×1, CamReset×1, ClearHighlights×1, Highlight×1, Say×2, WaitSay×1 | 2: `patternbuffer_trunk_1`..`patternbuffer_trunk_2` | 0 / 0 | T4 objective cutscene `data/levels/mission_pattern_buffer_attract.txt:L67`; T4 objective cutscene `data/levels/mission_pattern_buffer_recapture.txt:L59` | none | [B:LEGACY] |
| `receiver_primary1.txt` | 16/10 | CamBuildingFocus×2, CamReset×1, Say×5, WaitSay×2 | 5: `yardend_5`..`receiver_primary_3` | 0 / 0 | T4 objective cutscene `data/levels/mission_receiver_recapture.txt:L69` | none | [B:LEGACY] |
| `receiver_primary2.txt` | 10/7 | CamBuildingFocus×1, CamReset×1, ClearHighlights×1, Highlight×1, Say×2, WaitSay×1 | 2: `receiver_trunk_1`..`receiver_trunk_2` | 0 / 0 | T4 objective cutscene `data/levels/mission_receiver_recapture.txt:L70` | none | [B:LEGACY] |
| `temple_intro.txt` | 127/89 | CamBuildingFocus×3, CamCut×3, CamMove×7, CamReset×1, ClearHighlights×2, EnterLocation×1, ExitLocation×1, GodDishActivate×1, GodDishDeactivate×1, GodDishSpawnResearch×5, GodDishSpawnSpam×3, Highlight×3, ResetResearch×1, RestoreResearch×1, Say×21, SetMission×1, TriggerSpam×4, Wait×13, WaitCam×4, WaitFade×3, WaitSay×10 | 21: `temple_intro_1`..`temple_primary_3` | 27 / 145 | T1 ScriptTrigger `data/levels/mission_temple_cutscene.txt:L21` | none | [B:LEGACY] |
| `temple_primary.txt` | 18/13 | CamMove×2, CamReset×1, ClearHighlights×1, Highlight×3, Say×4, WaitCam×1, WaitSay×1 | 4: `temple_primary_1`..`temple_primary_3` | 0 / 33 | T4 objective cutscene `data/levels/mission_temple_recapture.txt:L51`; T4 objective cutscene `data/levels/mission_temple_recapture.txt:L52`; T4 objective cutscene `data/levels/mission_temple_recapture.txt:L53` | none | [B:LEGACY] |
| `yard_armour.txt` | 10/7 | DemoGesture×2, GiveResearch×1, Say×2, WaitSay×2 | 2: `yard_finished_1`..`yard_finished_2` | 0 / 0 | T3 global event `data/game.txt:L108`; T3 global event `data/game_unlockall.txt:L86` | none | [B:LEGACY] |
| `yard_end.txt` | 86/53 | CamBuildingFocus×1, CamLocationFocus×8, CamMove×1, CamReset×1, EnterLocation×2, ExitLocation×2, Say×21, StopSound×1, TriggerSound×1, WaitFade×5, WaitSay×10 | 21: `yardend_1`..`yardend_17` | 0 / 60 | T3 global event `data/game.txt:L114`; T3 global event `data/game_unlockall.txt:L92` | none | [B:LEGACY] |
| `yard_intro.txt` | 49/30 | CamBuildingFocus×3, CamMove×2, CamReset×1, ClearHighlights×2, Highlight×1, Say×11, StopSound×1, TriggerSound×1, Wait×1, WaitCam×1, WaitFade×1, WaitSay×5 | 11: `yard_intro_1`..`yard_intro_7` | 2 / 40 | T1 ScriptTrigger `data/levels/mission_yard_enable.txt:L14` | none | [B:LEGACY] |
| `yard_primary1.txt` | 9/7 | CamBuildingFocus×1, CamReset×1, ClearHighlights×1, Highlight×1, Say×2, WaitSay×1 | 2: `yard_genport_1`..`yard_genport_2` | 0 / 0 | T4 objective cutscene `data/levels/mission_yard_enable.txt:L69` | none | [B:LEGACY] |
| `yard_primary2.txt` | 10/6 | CamBuildingFocus×1, CamReset×1, Say×3, WaitSay×1 | 3: `yard_intro_1`..`yard_intro_5` | 0 / 0 | T4 objective cutscene `data/levels/mission_yard_enable.txt:L70` | none | [B:LEGACY] |
| `yard_primary3.txt` | 8/5 | CamBuildingFocus×1, CamReset×1, Say×2, WaitSay×1 | 2: `yard_safearea_1`..`yard_safearea_2` | 0 / 0 | T4 objective cutscene `data/levels/mission_yard_enable.txt:L71` | none | [B:LEGACY] |

Notes on the register:
- `ScriptTrigger` entries whose script name has no `.txt` (`launchpad_tutorial_defend`, `launchpad_finalintro`, `launchpad_dgupgrade`, `launchpad_reprogram`, `launchpad_reprogram2` in `data/levels/mission_launchpad.txt`; `pixelmines_yardport`, `pixelmines_refinery` in `data/levels/mission_mine_*.txt`) and `Action RunScript` entries without `.txt` in `data/game*.txt` (`gardenfinished`, `containment_finished`, `yard_available`, `yard_progress`, `yard_progressarmour`, `prebiosphere`, `biosphere_reddead`, `biosphere_end`, `gameover`) use the phrase-dump form of `RunScript` — dead twice over in FULL. [C] P19 census; `data/game.txt` L53–151.
- Music events used by scripts: `cutscene1`…`cutscene5`, `endsequence`, `Demo2Intro`, `Demo2Mid`, all defined in the `OTHER Music` block of `data/sounds.txt` (`Cutscene1`–`Cutscene5`, `EndSequence`, `Demo2Intro`, `Demo2Mid`; matching is case-insensitive). The narrator sounds `Appear`/`Disappear`/`TextAppear` are in `OTHER Sepulveda`. [C] `data/sounds.txt` (L2758–2790, L2951–3141); `code/sound/soundsystem.cpp:SoundSystem::TriggerOtherEvent`. Music in Multiwinia is therefore only reachable through scripts and loaders — see [10](10_audio_design.md) / [27](27_audio_engine.md).
- No shipped script uses `CamAnim`, `RunCredits`, `PurityControl`, `DestroyBuilding`, `ActivateTrunkPort`, `ActivateTrunkPortFull`, `SetCutsceneMode`. [C] census.

### 2.7 Script-engine constants

| Constant | Value | Kind | Ref |
|---|---|---|---|
| Script folder | `scripts/` | literal | [C] `Script::RunScript` |
| Token separators | `" \t\n\r:,"` | `#define DEFAULT_SEPERATOR_CHARS` | [C] `contrib/systemiv/lib/filesys/text_stream_readers.cpp` |
| Comment character | `#` | literal | [C] `TextReader::CleanLine` |
| Opcode count | 43 (`OpNumOps`) | enum | [C] `code/script.h` |
| Lines executed per frame | 1 | algorithm | [C] `Script::Advance` |
| `CamFov` default `immediate` | true (1) | literal | [C] `Script::AdvanceScript` |
| `DemoGesture` start delay | 2.0 s | literal | [C] `Script::RunCommand_DemoGesture` |
| `GiveResearch` message duration | 4.0 s | literal | [C] `Script::RunCommand_GiveResearch` |
| `GiveResearch` special message ids | 999 (`modsystem`), 998 (`accessallareas`) | literal | [C] same |
| Demo-gated script prefix | `launchpad_purpose` (17 characters) | literal | [C] `Script::RunScript` |
| `ScriptTrigger` default range / poll period | 100.0 wu / 1.0 s of sim time | literal | [C] `ScriptTrigger::ScriptTrigger`, `ScriptTrigger::Advance` |
| `ScriptTrigger` camera conditions | speed < 5.0 wu/s; `camview` extra distance +300 wu | literal | [C] `ScriptTrigger::Advance` |
| `ScriptTrigger` mode codes | always 999, never 998, camenter 997, camview 996 | `#define` | [C] `code/worldobject/scripttrigger.h` |
| `SCRIPT_TEST_ENABLED` validation ranges (dead) | `CamMove` duration 0.01–20.0 s; `Wait` 0.01–20.0 s | literal | [C] `Script::TestScript` |
| Building-focus orbit | −0.15 rad/s; range ±30 % and height ±50 % at ω = 0.3 rad/s; lift-in 2.0 s, factor 0.75, cap y ≤ 1000; target FOV 60° | literal | [C] `Camera::AdvanceBuildingFocusMode`, `Camera::RequestBuildingFocusMode` |
| Move-to-target minimum time fraction | 0.2 | literal | [C] `Camera::AdvanceMoveToTargetMode` |
| Animation node duration range | 0 ≤ d < 60 s (release-assert) | literal | [C] `LevelFile::ParseCameraAnims` |

### 2.8 Determinism and safety notes

- The interpreter is per-peer and wall-clock driven; eleven commands mutate simulation state on the local peer only (`GiveResearch`, `ResetResearch`, `RestoreResearch`, `GodDishActivate`, `GodDishDeactivate`, `GodDishSpawnSpam`, `GodDishSpawnResearch`, `TriggerSpam`, `DestroyBuilding`, `ActivateTrunkPort`, `ActivateTrunkPortFull`), and three more change global-world or level state (`EnterLocation`, `ExitLocation`, `SetMission`). In a lockstep multiplayer game any of these would desync. [C] §2.3; [I] consequence.
- `ScriptTrigger` decides inside the simulation tick from local camera state (§2.5). [C]
- `PurityControl` deletes user files and exits; `GiveResearch accessallareas` creates a profile folder that unlocks content. Neither belongs in a new game. [C]
- Re-implementation rule for the new game: scripts may drive **presentation** (camera, captions, music, UI focus) locally, but every gameplay effect must be issued as a sequenced, authoritative simulation command. [I]

---
