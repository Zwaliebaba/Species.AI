# 32 — Debug, test and performance

> Part B · Baseline build: Release|Win32 (OpenGL) + TARGET_PC_FULLGAME ([B:FULL]); see _work/build_matrix.md.
> Evidence: [C] confirmed · [I] inferred · [U] unknown. Citations are `path:Symbol` relative to the multiwinia/ source root.
> Sources merged: P17a_ui_framework_editor (D1–D7), P20_sequences_loaders_bench (§8 benchmark, §9 test harness), P14_platform_lib (32.P profiler, 32.D debug output / asserts / black box / leak tracking), P15_network (§8 sync checking, §12 bandwidth / network quality / FTP manager), X3_determinism_sync (sync-RNG tracking), P12_render_core (debug renderers, budgets), P16_sound and [27](27_audio_engine.md) §13 (sound tools), P01_app_core (testbed appendix), routed `_meta_*` notes, `phase2_log.md`; every section re-verified against the code by this synthesis (S32).

## Contents

- [1 Scope, summary and build reality](#1-scope-summary-and-build-reality)
- [2 Build-flag table — every debug and test feature](#2-build-flag-table--every-debug-and-test-feature)
- [3 Debug entry points and the debug menu](#3-debug-entry-points-and-the-debug-menu)
- [4 Cheat window — all 23 cheats](#4-cheat-window--all-23-cheats)
- [5 Network statistics window (F9) and other debug windows](#5-network-statistics-window-f9-and-other-debug-windows)
- [6 Testbed mode](#6-testbed-mode)
- [7 Test harness and script test](#7-test-harness-and-script-test)
- [8 Benchmark mode](#8-benchmark-mode)
- [9 Profiler](#9-profiler)
- [10 Logging, asserts, crash and sync logs](#10-logging-asserts-crash-and-sync-logs)
- [11 Debug renderers and visual debug aids](#11-debug-renderers-and-visual-debug-aids)
- [12 Known performance hotspots and budgets](#12-known-performance-hotspots-and-budgets)
- [13 Desync debugging tooling](#13-desync-debugging-tooling)
- [14 Recommendations for the new game's debug and test infrastructure](#14-recommendations-for-the-new-games-debug-and-test-infrastructure)
- [Open questions and unknowns](#open-questions-and-unknowns)
- [Docs-vs-code discrepancies](#docs-vs-code-discrepancies)
- [IP items](#ip-items)
- [Source map](#source-map)
- [Appendix — dead/legacy code](#appendix--deadlegacy-code)

## 1 Scope, summary and build reality

This document specifies every developer-facing debugging, testing, profiling, logging and benchmarking facility in the tree, states exactly which of them exist in the shipped exe, and collects the performance evidence (budgets, caps, instrumented hotspots and claimed numbers) a new team needs to size its own engine.

Summary of what a player's copy of the shipped exe ([B:FULL]) actually contains [C] unless marked:

| Facility | In [B:FULL]? | How reached | Section |
|---|---|---|---|
| Network statistics window | **yes** | `ToggleNetworkStats` = F9 (opens only) | §5.1 |
| Debug log `debug.txt` | **yes** (`DUMP_DEBUG_LOG`) | always written to the profile directory | §10.1 |
| Release asserts + `blackbox.txt` | **yes** | any failed `AppReleaseAssert` / `AppAssert` / container bounds check | §10.2 |
| SEH crash log `ERRORLOG.TXT` + `CRASH.DMP` + support tool | **yes** (`USE_CRASHREPORTING`) | any unhandled exception on the main thread | §10.3 |
| Desync dump `syncerror-…txt` (world-state dump) | **yes** | first NETSYNCERROR received | §10.4, §13 |
| Debug renderers (`DEBUG_RENDER_ENABLED`) | compiled, **almost never drawn** | editor-only or after an IFrame diff (never in FULL); AI overlay via hidden pref `RenderAIInfo` | §11 |
| CPU profiler (`g_profiler`, 186 instrumented sites) | compiled, **unreachable** (no live opener) | Profiler window only from debug/cheat menus | §9 |
| Debug menu ("Tools Menu") | compiled, **unreachable** in every build | its F1 opener is commented out | §3.3 |
| FPS / entity counter overlay | compiled, **forced off** and unreachable | `FPSButton` only on the unreachable debug menu | §3.5 |
| Hidden debug preferences (`RenderAIInfo`, `AIInfoTeam`, `RecordDemo`, `DemoFrameRate`, `TextLanguageTest`, `RenderPosterResolution`, `RenderLandscapeMode`) | **yes** (edit `preferences.txt`) | preferences file | §5.8 |
| Cheat window (23 cheats) | **no** — empty `Create()` | F11 in [B:DEBUG] | §4 |
| Sound editor / stats / profile tools | no | [B:DEBUG] cheat window | §5.3 |
| Sync-RNG forensic log (`TRACK_SYNC_RAND`) | no | [B:DEBUG] | §13 |
| Full world-snapshot sync hashing (IFrames) | no (hash = next sync-RNG draw) | [B:DEBUG] and beta-group builds | §13 |
| Testbed (automated multi-machine soak test) | no — macro defined nowhere | [B:DEAD] | §6 |
| Campaign test harness, script test | no | [B:DEAD] | §7 |
| Benchmark telemetry recorder + FTP upload | no — macro commented out | [B:DEAD] in this tree; shipped in the hardware-compatibility SKU [I] | §8 |
| Network-quality simulator | no | [B:DEAD] | §13.5 |
| Memory-leak tracker | no | [B:DEAD] | §10.6 |
| AVI capture (Grabber) | no | [B:DEBUG], non-functional | §5.4 |

`docs/cpu cycles.txt`, named by the specification as the performance cross-check, contains **no performance data**: it is an early gameplay design note about "programs" that consume "CPU cycles" (task manager, ALT-TAB task switching, CTRL-BREAK termination, gestures, number keys, "Debug Zeppelins") [C] `docs/cpu cycles.txt`. The performance section (§12) is therefore built from code evidence plus the release notes and change logs in `docs/` (logged in Docs-vs-code discrepancies).

Resolved in this document (drafts or earlier notes disagreed; code re-read):
1. Number of debug entry points — P17a/phase2 log said 17; `code/interface/debugmenu.h:DebugKeyBindings` declares **18** static functions (§3.2).
2. Whether the debug menu, and so the profiler, is reachable in FULL (`_meta_contradictions` item 6, [U] there) — **no**: the only call of `DebugKeyBindings::DebugMenu` is commented out in `code/user_input.cpp:UserInput::AdvanceMenus`, no binding exists, and the profiler's other opener is the cheat window, whose body is compiled out in FULL (§3, §9).
3. Whether the desync dump is debug-only — P15 tagged `syncerror-*.txt` as "[B:DEBUG] plus the sync-rand log"; code: `Location::DumpWorldToSyncLog` runs in **every** build, only the prepended sync-RNG call log needs `TRACK_SYNC_RAND` (§10.4).
4. Units of the benchmark's bandwidth columns (P20 [U]) — **bytes per second, 5-second average** (`code/network/bandwidth.cpp:BandwidthCounter::Count`) (§8.4).
5. Cheat-crate default reward `m_reward = −1` (P17a U3, "[I] random") — confirmed: −1 means "normal random selection", any other value forces that reward (`code/worldobject/crate.cpp` reward selection tests `m_rewardId != -1`) (§4.2).
6. Testbed server "10 times faster" (comment in `code/network/server.cpp:Server::AdvanceIfNecessary`) — every branch adds 0.1 s, so there is **no** speed-up (§6.7).
7. START_PROFILE count — P14 counted 192 lines over all of `code/`; this document counts **186** call sites in compiled files (comments and unbuilt files excluded) with `_work/scripts/S_32_profile_sections.py` (§9.5).
8. Sound-tool reachability — already resolved in [27 §13.2](27_audio_engine.md#132-reachability-of-the-tools-bdebug): only "Sound Editor" and "Sound Stats" are reachable, via the [B:DEBUG] cheat window.

## 2 Build-flag table — every debug and test feature

Columns: *Defined in* = where the macro is set (all in `code/lib/universal_include.h` unless stated); *FULL* = state in the baseline; *Files* = files under `code/`, `contrib/systemiv/lib`, `contrib/eclipse` with an `#if…` on it (grep, this synthesis); *Gates* = everything the macro switches. [C] for every row (macro definitions read line by line in `code/lib/universal_include.h`, call sites read in the listed files).

### 2.1 Product-level debug switches

| Macro | Defined in | FULL | Other builds | Files | Gates | Tag |
|---|---|---|---|---|---|---|
| `TARGET_DEBUG` | commented-out target line | **off** | the "debug" target | 20 | turns on `SOUND_EDITOR`, `CHEATMENU_ENABLED`, `GESTURE_EDITOR`, `NETWORK_STATS_ENABLED`, `AVI_GENERATOR`, `TRACK_SYNC_RAND`, `INCLUDE_CRATE_HELP_WINDOW`, TankBattle / Prologue / Campaign modes, and (in `code/worldobject/crate.h`) `INCLUDE_THEBEAST`; does **not** define `DUMP_DEBUG_LOG`. Directly gates: profile directory = working directory (`code/app.cpp:App::GetProfileDirectory`); duplicate-auth-key rejection skipped (`code/network/server.cpp:Server` client registration); full world-snapshot IFrames + SHA-1 sync hash (`code/network/iframe.cpp:IFrame::Generate`); packet-drop key (`code/network/clienttoserver.cpp:ClientToServer::ReceiveLetter`); lobby network-status overlay (`code/UI/GameMenuWindow.cpp:GameMenuWindow::RenderNetworkStatus`); `inputprefs_debug.txt` (`code/lib/input/input.cpp:InputManager::parseInputPrefs`); editor Save/Publish captions and saving into `levels/` (`code/interface/editor_window.cpp`, `code/level_file.cpp`, `code/global_world.cpp`); "Edit Official Maps" button (`code/interface/multiwinia_window.cpp`); Darwinia-mode button → demo page (`code/UI/DarwiniaModeButton.h`); per-datagram server log (`code/network/server.cpp:Server::AdvanceSender`); `Directory` debug checks (`contrib/systemiv/lib/tosser/directory.cpp` defines `_DEBUG`); attack-objective log (`code/worldobject/aiobjective.cpp`); `ToDouble` helper (`code/main.cpp`) | [B:DEBUG] |
| `CHEATMENU_ENABLED` | `TARGET_DEBUG` | off | debug | 7 | cheat-window body (`code/interface/cheat_window.cpp:CheatWindow::Create`); F11 handler (`code/user_input.cpp:UserInput::Advance`); "Cheat Menu (F4)" debug-menu button; special-lighting keys L/K (`code/location.cpp:Location::RenderBuildings`); ×10 refinery speed with Shift (`code/worldobject/mine.cpp:MineBuilding::RefinerySpeed`, single-player only); ×100 rocket fuel with Shift (`code/worldobject/rocket.cpp:EscapeRocket::ProvideFuel`, single-player only) | [B:DEBUG] |
| `NETWORK_STATS_ENABLED` | FULL, DEBUG, BETATEST_GROUP_ALL | **on** | — | 1 | F9 `ControlToggleNetworkStats` → `DebugKeyBindings::NetworkButton` (`code/user_input.cpp:UserInput::Advance`); the window class itself is always compiled | [B:FULL] |
| `DEBUG_RENDER_ENABLED` | global (all targets) | **on** | all | 20 | `code/lib/debug_render.cpp` primitives, `g_debugRenderer` pass, `SyncDiff::Render`, shape/tree hit-check and marker renderers, editor arrows on buildings (§11) | [B:FULL] (mostly unreachable) |
| `TRACK_SYNC_RAND` | `TARGET_DEBUG` (commented out for BETATEST_GROUP_ALL) | off | debug | 9 (+ `#undef` in `code/worldobject/spirit.cpp`, `code/worldobject/snow.cpp`) | per-call sync-RNG log, entity/effect/building state logging around every `Advance`, entity watch list, per-letter command decoding (§13.3) | [B:DEBUG] |
| `DUMP_DEBUG_LOG` | FULL, MULTIWINIA_DEMOONLY, PC_HARDWARECOMPAT, ASSAULT_STRESSTEST, BETATEST_GROUP_ALL, every TARGET_BETATEST_GROUP | **on** | commented out in DEBUG | 1 compiled (`code/main.cpp:Initialise`) + 2 SDL files not built | `AppDebugOutRedirect("debug.txt")` → all `AppDebugOut` text goes to `<profile>debug.txt` instead of `OutputDebugString` | [B:FULL] |
| `USE_CRASHREPORTING` | global | **on** | all | 5 | SEH `__try/__except(RecordExceptionInfo)` around `RunTheGame` (`code/main.cpp:AppMain`), crash log, minidump, support-tool launch (§10.3) | [B:FULL] |
| `XCRASHREPORT_WRITE_MINIDUMP` | `code/lib/exception_handler.cpp` (=1) | **on** | all | 1 | writes `CRASH.DMP` | [B:FULL] |
| `SOUND_EDITOR` | `TARGET_DEBUG`; `#undef` on Linux and Mac | off | debug | 16 | sound editor, effect editor, sample-group editor, parameter editor, Sound Stats, Sound Profile, Pokey playground, `EffectProcessor` instance ([27 §13](27_audio_engine.md#13-profiles-statistics-debug-and-editor-tooling)) | [B:DEBUG] |
| `GESTURE_EDITOR` | `TARGET_DEBUG` | off | debug | 5 | gesture training window and demo recorder — **also** requires `USE_SEPULVEDA_HELP_TUTORIAL`, defined nowhere | [B:DEAD] |
| `AVI_GENERATOR` | `TARGET_DEBUG`; `#undef` on Linux and Mac | off | debug | 7 | Grabber window, `AVIGenerator` (§5.4) | [B:DEBUG] |
| `INCLUDE_CRATE_HELP_WINDOW` | `TARGET_DEBUG` | off | debug | 1 | crate help list on the Help & Options page (`code/interface/helpandoptions_windows.cpp`) | [B:DEBUG] |
| `INCLUDE_THEBEAST` | `code/worldobject/crate.h` inside `#ifdef TARGET_DEBUG` | off | debug | 3 | crate reward "The Beast" (changes `Crate::NumCrateRewards`, so DEBUG and FULL peers disagree on reward ids) ([04 §5.30](04_crates_powerups_and_weapons.md#530-the-beast-bdebug)) | [B:DEBUG] |
| `TRACK_MEMORY_LEAKS` | commented out inside `TARGET_DEBUG` | off | none | 4 | CRT debug-heap `new`, `leaks.txt` report on quit (§10.6) | [B:DEAD] |
| `SPECTATOR_ONLY` | PC_HARDWARECOMPAT, ASSAULT_STRESSTEST (commented in DEBUG) | off | hwcompat, stress | 3 | `App::m_spectator = true` at start; hides Advanced Options in the lobby; game cursor early-outs | hwcompat/stress SKUs |
| `PERMITTED_MAPS` | PC_PREVIEW, PC_HARDWARECOMPAT (`MAPID_MP_KOTH_2P_1`, `MAPID_MP_BENCHMARK_1`, `MAPID_MP_KOTH_4P_1`), ASSAULT_STRESSTEST (`MAPID_MP_ASSAULT_STRESS`), beta groups | off | as listed | 4 | map whitelist | SKU-specific |

### 2.2 Automated-test and benchmark switches

| Macro | Defined in | FULL | Files | Gates | Tag |
|---|---|---|---|---|---|
| `TESTBED_ENABLED` | commented out inside `TARGET_DEBUG` | off (every target) | 16 | multi-machine automated soak test: menu automation, auto-ready, spectator registration, seed override, sync-error counting, `TestBedLog.txt`, rendering/sound toggle, `GRACE_TIME` 20 s, `multiwinia-testbed.txt` blueprints (§6) | [B:DEAD] |
| `TESTBED` (a different, never-defined macro) | nowhere | off | 2 | lobby spectator-button column (`code/UI/GameMenuWindow.cpp`); client timeout 60 s instead of 20 s (`code/network/server.cpp`) | [B:DEAD] |
| `TEST_HARNESS_ENABLED` | nowhere | off | 4 | campaign mission-graph explorer, `test_log.txt` (§7.1) | [B:DEAD] |
| `SCRIPT_TEST_ENABLED` | nowhere | off | 1 | static script validator `Script::TestScript` and error routing (§7.2) | [B:DEAD] |
| `BENCHMARK_AND_FTP` | commented out inside the `TARGET_BETATEST_GROUP` block | off | 4 | per-second telemetry recorder, dxdiag capture, `ncftpput` upload (§8) | [B:DEAD] in this tree |
| `ATTRACTMODE_ENABLED` | nowhere | off | 2 | attract (demo-playback) mode ([09](09_meta_systems.md)) | [B:DEAD] |
| `NETWORKQUALITY_TESTING` | commented out in `code/network/clienttoserver.cpp` | off | 1 | client-side link simulator 200 ms / 25 % loss / 8000 B/s (§13.5) | [B:DEAD] |
| `CHECK_IFRAME_SERIALISATION` | commented out in `code/network/iframe.cpp` | off | 1 | IFrame write/read round-trip self-check (inside the DEBUG branch) | [B:DEAD] |
| `FAKE_SERVERS` | commented out in `code/UI/GameMenuWindow.cpp` | off | 1 | fills the server browser with `NUM_FAKE_SERVER` = 100 random fake servers (browser UI test) | [B:DEAD] |
| `DEBUG_ENTITY_GRID` | commented out in `code/entity_grid.cpp` | off | 1 | entity-grid error visualiser | [B:DEAD] |
| `SOUNDSYSTEM_VERIFY` | commented out in `code/sound/soundsystem.cpp` | off | 1 | `SoundSystem::RuntimeVerify` | [B:DEAD] |
| `FTP_MANAGER_VERBOSE_DEBUG` | **defined** in `code/network/ftp_manager.cpp` | **on** | 1 | logs every FTP block send / timeout / duplicate / ack to `debug.txt` (only active after a desync, §13) | [B:FULL] |
| `FRAMES_PER_SECOND_COUNTER` | defined in `code/lib/opengl_directx.cpp` | D3D only | 1 | 5-second frame counter whose print is commented out → no effect | [B:D3D], inert |

### 2.3 Profiling, logging and language-QA switches

| Macro | Defined in | FULL | Files | Gates | Tag |
|---|---|---|---|---|---|
| `PROFILER_ENABLED` | nowhere | off | 3 compiled | **not** the main profiler (which is always compiled); only the commented-out F10 hot-key and the software-mixer profiler allocation in `code/sound/sound_library_3d.cpp` / `code/sound/soundsystem.cpp` | [B:DEAD] |
| `NO_PROFILER` | nowhere | off | 1 | would compile `START_PROFILE`/`END_PROFILE` to nothing (`contrib/systemiv/lib/profiler.h`) — so the macros are live in FULL | — |
| `SINGLE_THREADED_PROFILER` | Mac block | off | 1 | ignores profile calls from non-main threads (`contrib/systemiv/lib/profiler.cpp:MAIN_THREAD_ONLY`) | Mac only |
| `NO_DEBUG_OUTPUT`, `NO_DEBUG_ASSERT` | nowhere on PC | off → output and asserts compiled | 2 | `AppDebugOut`, `AppDebugAssert` bodies | — |
| `_DEBUG` (VS Debug configurations only) | compiler | **off** (Release) | 18 | `AppDebugAssert` → `_ASSERT` (no-op in Release); `AppReleaseAssertFailed` uses `_ASSERT(false)` instead of black box + `throw`; `ControlDebugCameraFixUp` (`code/camera.cpp`); client time-out disabled (`code/network/server.cpp`); sound-file validation (`SoundSystem::IsSoundSourceOK`) | dev configs |
| `VERBOSE_DEBUG` | commented out | off | 5 | extra sound and SDL logging | [B:DEAD] |
| `TRACK_LANGUAGEPHRASE_ERRORS` | nowhere | off | 2 | missing-phrase tracking in `contrib/systemiv/lib/language_table.cpp` | [B:DEAD] |
| `FOG_LOGGING`, `FOG_PREFERENCE` | nowhere | off | 2 | pref `RenderFog` (0/1/2) logging/enable; `code/lib/debug_fog_logging.cpp` not in the project | [B:DEAD] |
| `OPENGL_DEBUG_ASSERTS` | Mac block | off | 1 | GL error asserts (`code/renderer.h`) | Mac only |
| `AUDIO_DEBUGGING` | commented out in `code/sound/sound_library_3d_xaudio.cpp` | off | 1 | XAudio logging (Xbox 360 remnant) | [B:DEAD] |
| `RENDER_CURSOR_3D` | commented out in `TARGET_DEBUG` | off | 1 | 3-D cursor (`code/gamecursor.cpp`) | [B:DEAD] |
| `USE_DARWINIA_MOD_SYSTEM` | nowhere | off | 7 | `modsEnabled` — when false (always) hides the editor, gesture-editor and four sound-tool buttons of the debug menu | [B:DEAD] |
| `LOCATION_EDITOR` | commented out everywhere | off | 39+ | in-game level editor ([08](08_levels_and_level_design.md)) | [B:EDITOR] |
| `PURITY_CONTROL`, `GIVE_ALL_LEVELS`, `JAMES_FIX` | nowhere | off | 2 / 1 / 1 | editor-loop removal, unlock-all, a Darwinia global-world fix | [B:DEAD] |

`GRACE_TIME` (the pre-match ready period) is 3.0 s, or 20.0 s under `TESTBED_ENABLED` [C] `code/lib/universal_include.h`.

## 3 Debug entry points and the debug menu

### 3.1 Debug controls and hot-keys [C] `data/input/input_preferences.txt`, call sites as listed

| Control (binding) | Default binding | Handler | Effect | Tag |
|---|---|---|---|---|
| `ToggleNetworkStats` | `key f9 down` | `code/user_input.cpp:UserInput::Advance` (`NETWORK_STATS_ENABLED`) | opens "Network Stats" if not open (never closes it; use its close box) — works in menus, lobby and match because `UserInput::Advance` runs in `MainMenuLoop` and `LocationGameLoop` (`code/main.cpp`) | **[B:FULL]** |
| `ToggleCheatMenu` | `key f11 down` (pad `XInput back` line commented out) | `UserInput::Advance` (`CHEATMENU_ENABLED`), only when **no** Eclipse window is open | opens "Cheat Window" | [B:DEBUG] |
| `ToggleEditor` | `key f9 down` (same key as network stats) | handler commented out in `UserInput::Advance` | — | inert |
| `DebugDropPacket` | `key f pressed` | `code/network/clienttoserver.cpp:ClientToServer::ReceiveLetter` (`TARGET_DEBUG`) | discards the incoming letter while held — manual packet-loss test | [B:DEBUG] |
| `CameraDebugRotate` | `mouse right pressed` | `code/camera.cpp:Camera::AdvanceDebugMode` | yaw −0.005·dx, pitch −0.005·dy rad per pixel while in the debug camera sub-mode (§3.4) | [B:FULL] when the sub-mode is active |
| `DebugCameraFixUp` | `key g pressed` | two `Camera::AdvanceSphereWorld*Mode` advancers (`_DEBUG`) | resets the global-world intro/outro camera | dev configs, [B:LEGACY] |
| `ToggleRendering` | `key r down` | `code/location_input.cpp:LocationInput::Advance` (`TESTBED_ENABLED`, testbed on) | toggles testbed rendering + sound (§6.7) | [B:DEAD] |
| `GameDebugQuit` | binding commented out (`XInput leftbumper down`) | `code/main.cpp:HandleCommonConditions` | would set `m_requestQuit`; no binding → never fires | inert |
| `CreateScreenShot` | `key p down` | `code/renderer.cpp:Renderer::Render` (no window open) | screenshot; `RenderPosterResolution` N > 1 renders an N×N tiled poster via `PosterMaker` ([24](24_rendering.md)) | [B:FULL] |
| `RTLoaderPixelWaveIncrease` / `…Decrease` | `key l pressed` / `key k pressed` | `code/location.cpp:Location::RenderBuildings` (`CHEATMENU_ENABLED`) | sets pref `RenderSpecialLighting` 1 / 0 | [B:DEBUG] |
| `ScrollSpeedup` | `key shift pressed` | `MineBuilding::RefinerySpeed` ×10, `EscapeRocket::ProvideFuel` ×100 (`CHEATMENU_ENABLED`, only when `!App::Multiplayer()`) | campaign speed cheats; never in Multiwinia (every Multiwinia game is `Multiplayer()`) | [B:DEBUG] [B:LEGACY] |

Hot-keys that exist only as commented-out code: F1 debug menu (`UserInput::AdvanceMenus`), F2 debug camera, F5 FPS, F10 profiler (`#ifdef PROFILER_ENABLED`), and the F7/F9/F11 sound-tool keys (`UserInput::Advance`) [C].

### 3.2 `DebugKeyBindings` — the 18 static entry points [C] `code/interface/debugmenu.h:DebugKeyBindings`, `code/interface/debugmenu.cpp`

| # | Function | Action | Compiled under | Live caller in FULL |
|---|---|---|---|---|
| 1 | `DebugMenu()` | toggle `::DebugMenu("dialog_toolsmenu")` | always | none (F1 call commented out); `UserInput::m_removeTopLevelMenu` can still close it |
| 2 | `NetworkButton()` | if no window "Network Stats" → `new NetworkWindow("Network Stats")` | always | **F9** |
| 3 | `DebugCameraButton()` | `Camera::SetNextDebugMode()` (cycles §3.4) | always | none |
| 4 | `FollowCameraButton()` | declared, **no definition** | always | none (would not link) |
| 5 | `FPSButton()` | toggle `Renderer::m_displayFPS` | always | none |
| 6 | `InputLogButton()` | declared, **no definition** | always | none |
| 7 | `ProfileButton()` | toggle `ProfileWindow("Profiler")`, 570 × 600 at (ScreenW − 590, 30) | always | none (debug menu, cheat window) |
| 8 | `EditorButton()` | `App::m_requestToggleEditing = true` | `LOCATION_EDITOR` | — |
| 9 | `GrabberButton()` | open `GrabberWindow("Grabber")`, 200 × 50 at (10, ScreenH − 50) | `AVI_GENERATOR` | none anywhere |
| 10 | `GestureButton()` | open `GestureWindow("Gesture Editor")`, 660 × 660 at (30, 30) | `GESTURE_EDITOR && USE_SEPULVEDA_HELP_TUTORIAL` | — |
| 11 | `CheatButton()` | if not open → `CheatWindow("Cheat Window")`, 200 × 200 at (250, 50) | `CHEATMENU_ENABLED` | — (F11 in DEBUG) |
| 12 | `PokeyButton()` | open `PokeyWindow("Pokey Playground")`, 400 × 480 at (10, 40) — class defined nowhere | `SOUND_EDITOR` | — |
| 13 | `SoundStatsButton()` | toggle Sound Stats window | `SOUND_EDITOR` | — (cheat window in DEBUG) |
| 14 | `SoundVisualiserButton()` | declared, **no definition** | `SOUND_EDITOR` | — |
| 15 | `SoundEditorButton()` | toggle Sound Editor, 500 × 550 at (50, 25) | `SOUND_EDITOR` | — (cheat window in DEBUG) |
| 16 | `SoundProfileButton()` | toggle Sound Profile window | `SOUND_EDITOR` | — |
| 17 | `ReallyQuitButton()` | open `ReallyQuitWindow` if not open | always | none on Win32 |
| 18 | `ToggleFullscreenButton()` | `SetWindowed(!Windowed(), true, …)` | always | none on Win32 (only `code/lib/input_sdl.cpp`, not built) |

### 3.3 The "Tools Menu" (`DebugMenu`, window `dialog_toolsmenu`) — compiled, unreachable in every build [C] `code/interface/debugmenu.cpp:DebugMenu::Create`, `DebugMenu::Render`

Window at (10, 20), 170 × 75. Buttons at x = 10, width `m_w − 20` = 150, y advancing by `pitch` = 18 from 5 (plus half-pitch gaps); captions are literal English:

| Order | Caption | Calls | Condition |
|---|---|---|---|
| 1 | "Profile (F6)" | `ProfileButton` | always |
| 2 | "Network Stats" | `NetworkButton` | always |
| 3 | "Display FPS (F5)" | `FPSButton` | always |
| 4 | "Dbg Cam (F2)" (current sub-mode "Always" / "Auto" / "Never" drawn at its right, size 10) | `DebugCameraButton` | always |
| 5 | "Toggle Editor (F3)" | `EditorButton` | `LOCATION_EDITOR` and `modsEnabled` |
| 6 | "Cheat Menu (F4)" (not added to the pad navigation order) | `CheatButton` | `CHEATMENU_ENABLED` |
| 7 | "Gesture Editor" | `GestureButton` | `GESTURE_EDITOR`, `USE_SEPULVEDA_HELP_TUTORIAL`, `modsEnabled` |
| 8 | "Pokey Playground" | `PokeyButton` | `SOUND_EDITOR` and `modsEnabled` |
| 9 | "Sound Stats (F7)" | `SoundStatsButton` | same |
| 10 | "Sound Editor (F8)" | `SoundEditorButton` | same |
| 11 | "Sound Profile (F9)" (caption string "Sound Profile (F8)") | `SoundProfileButton` | same |

`modsEnabled` is the constant `false` unless `USE_DARWINIA_MOD_SYSTEM` (never defined), so rows 5 and 7–11 never appear. The F-key captions are stale: none of those keys is bound to these actions (F1–F8 are task-manager keys) [C] `data/input/input_preferences.txt`. Three option-window buttons (`PrefsScreenButton`, `PrefsGfxDetailButton`, `PrefsSoundButton`) are defined in the same file but never registered [C].

### 3.4 Debug camera sub-mode [C] `code/camera.cpp:Camera::Advance`, `Camera::AdvanceDebugMode`, `Camera::SetNextDebugMode`; full camera spec in [06 §9.3](06_player_control_and_input.md#93-mode-enumeration-15-modes-and-debug-sub-modes)

`m_debugMode ∈ {DebugModeNever 0, DebugModeAlways 1, DebugModeAuto 2}`, default **Auto**; `SetNextDebugMode` cycles 0 → 1 → 2 → 0. Each frame the debug advancer replaces the normal camera mode when no camera animation is playing and (`Always`) or (`Auto` and at least one Eclipse window is open) or the mode is less than 2 frames old. Debug advancer: fly speed `worldSizeX / 30` units/s, ×10 with `CameraSpeedup`, ×0.1 with `CameraSlowdown`; key movement only when **no** window is open (or editing); RMB rotation as in §3.1; mouse uncaptured.

Consequence in FULL [C]: opening the F9 window during a match (Auto + one window open) switches the camera to the debug advancer, which then cannot translate (a window is open) — the view freezes except RMB rotation — and `LocationInput::Advance` / `teamControls.Advance()` issue no team orders while any window is open (`code/location_input.cpp:LocationInput::Advance`, `code/main.cpp:LocationGameLoop`). The statistics window is therefore an observation tool, not something a player can leave open while playing.

### 3.5 FPS and entity-count overlay [C] `code/renderer.cpp:Renderer::Renderer`, `Renderer::RenderFrame`, `Renderer::FPSMeterAdvance`

`m_displayFPS` defaults to `true` and is forced to `false` under `TARGET_PC_FULLGAME`. When on (and the interface is not hidden) it draws, right-aligned at (ScreenW − 10, ScreenH − 40), "FPS: %d   DWs: %d" (`Renderer::m_fps`, `Entity::s_entityPopulation`) inside a location, "FPS: %d" elsewhere. `m_fps` = frames counted in the last wall-clock second. The only toggle is `FPSButton` (unreachable) → **no FPS display exists in the shipped game**. A dead block in `PrefsGraphicsWindow::Render` (after an unconditional `return`) would have shown "%d FPS" and per-section CPU percentages (`RenderCPUUsage`, `m_lastTotalTime × 100` of named root profile elements) in the graphics options [C] `code/interface/prefs_graphics_window.cpp`.

## 4 Cheat window — all 23 cheats

### 4.1 Build tag, opening and layout [C] `code/interface/cheat_window.cpp:CheatWindow::Create`, `code/interface/debugmenu.cpp:DebugKeyBindings::CheatButton`

- The `CheatWindow` class is compiled in every build; its `Create()` body is inside `#ifdef CHEATMENU_ENABLED`. In FULL `Create()` is empty and nothing opens the window → **[B:DEBUG] only**.
- Opened by F11 (`ToggleCheatMenu`) only when no Eclipse window is open, or by "Cheat Menu (F4)" on the unreachable debug menu. Window "Cheat Window", 200 × 200 at (250, 50); buttons are added below the nominal height.
- Layout: buttons start at y = 25, pitch 20; full-width buttons x = 10, w = `m_w − 20` = 180; paired buttons w = `(m_w − 25)/2` = 87, the second column at x = 10 + 87 + 5 = 102; the crate pair is "GoodCrate" at x = 10, w = `m_w/2 − 20` = 80 and the "Crates" drop-down at x = `m_w/2` = 100, w = `m_w/2 − 10` = 90. All buttons are in `m_buttonOrder` (pad navigable). Captions are literal English (no language table).
- Every spawn cheat aims at the **screen centre**: `Camera::GetClickRay(ScreenW/2, ScreenH/2)` → `Landscape::RayHit` → position P.

### 4.2 Every cheat [C] `code/interface/cheat_window.cpp` (one class per button)

| # | Caption | Class | Effect | Build condition |
|---|---|---|---|---|
| 1 | Kill All Enemies | `KillAllEnemiesButton` | for every team t with `!Location::IsFriend(myTeamId, t)`: every entity of every unit and every entity in `m_others` gets `ChangeHealth(−2 × m_stats[StatHealth])`; then **every** AntHill and Triffid building (any team) gets `Damage(−999)` | location loaded |
| 2 | Spawn Green | `SpawnDarwiniansButton` (`m_teamId` 0) | `Location::SpawnEntities(P, team, unit −1, TypeDarwinian, 20, zero velocity, spread 30)` | location loaded |
| 3 | Spawn Red | same (`m_teamId` 1 — assigned only if a location existed when the window was created; otherwise uninitialised, harmless because the click requires a location) | as 2 | same |
| 4 | Spawn Yellow | same (2) | as 2 | same |
| 5 | Spawn Blue | same (3) | as 2 | same |
| 6 | Spawn Virus | same (`Location::GetMonsterTeamId()` at creation) | as 2, monster team | same |
| 7 | Spawn Future | same (`Location::GetFuturewinianTeamId()` at creation) | as 2, Futurewinian team | same |
| 8 | Spawn Armour | `SpawnTankButton` | `SpawnEntities(P, team 2, −1, TypeArmour, 1, zero, 0)` — always team 2 | same |
| 9 | Spawn Virii | `SpawnViriiButton` | `SpawnEntities(P, monster team, −1, TypeVirii, 20, zero, spread 0, range 1000)` | same |
| 10 | Spawn Spirits | `SpawnSpiritButton` | 10 × `Location::SpawnSpirit(P + (SFRAND(20), 0, SFRAND(20)), zero, team 2, empty id)` — `SFRAND` is the **synced** RNG (±10 units jitter) | same |
| 11 | Allow Arbitrary Placement | `AllowArbitraryPlacementButton` | `GetMyTaskManager()->m_verifyTargetting = false` — `TaskManager::IsValidTargetArea` then skips its placement rules (officer needs a Darwinian within 10, etc.) | same |
| 12 | Enable Generator and Mine | `EnableGeneratorAndMineButton` | global buildings `TypeGenerator` at location "generator" and `TypeRefinery` at "mine" → `m_online = true`; `GlobalWorld::EvaluateEvents()` | always; [B:LEGACY] (Darwinia campaign) |
| 13 | Enable Receiver and Buffer | `EnableReceiverAndBufferButton` | same for `TypeSpiritProcessor` at "receiver" and `TypeBlueprintStore` at "pattern_buffer" | [B:LEGACY] |
| 14 | Open All Locations | `OpenAllLocationsButton` | every `GlobalLocation::m_available = true` | [B:LEGACY] |
| 15 | Give all research | `GiveAllResearchButton` | for every research type: `GlobalResearch::AddResearch(i)` (only acts if level < 1), single-player only `SetCurrentProgress(i, 400)`, `EvaluateLevel(i)` (level +1 when progress ≥ required) | always; [I] a no-op in Multiwinia, where every research level is preset ≥ 1 ([02 §2.3](02_units_and_entities.md#23-research-levels-are-fixed-in-multiwinia)) and progress stays 0 |
| 16 | Clear Resources | `ClearResourcesButton` | `Resource::FlushOpenGlState()` then `RegenerateOpenGlState()` — drops and rebuilds every texture, display list and shape list (exercises the device-loss path) | always |
| 17 | Spawn Ports | `SpawnPortsButton` | for every building and every port: `SpawnEntities(portPos, team 0, −1, TypeDarwinian, 3, zero, 30)` — **no** location null-check (crash if used in a menu) | always |
| 18 | GoodCrate + "Crates" drop-down | `CrateCreateButton` + `DropDownMenu` | drop-down lists `Crate::GetName(p)` for p = 0 … `Crate::NumCrateRewards − 1` (raw names; includes The Beast in DEBUG), bound to `m_reward` (default −1 = normal random reward). Button: create a `Crate` building at (P.x, **300**, P.z) from a template, `SetDetail(1)`, id `(255, UNIT_BUILDINGS, −1, GlobalWorld::GenerateBuildingId())`, `m_rewardId = m_reward`, `MarkerSystem::RegisterMarker_Crate`. `m_good` is set but never read | only added when `App::Multiplayer()` |
| 19 | Profiler | `ProfilerCreateButton` | `DebugKeyBindings::ProfileButton()` (§9) | always |
| 20 | Sound Editor | `SoundEditorCreateButton` | `SoundEditorButton()` ([27 §13.3](27_audio_engine.md#133-tool-windows)) | `SOUND_EDITOR` |
| 21 | Sound Stats | `SoundStatsCreateButton` | `SoundStatsButton()` | `SOUND_EDITOR` |
| 22 | Network Stats | `NetworkStatsCreateButton` | `NetworkButton()` (§5.1) | always |
| 23 | Show Dr S Face | `ShowDrSepulvedaFaceButton` | `Clouds::ShowDrSFace()` → `m_faceTime = now`; from 5 s to 30 s later the sky draws a face texture, fading in over 1 s from 5 s and out over 1 s from 28 s (`Clouds::Render`, `Clouds::RenderFace`). Registered under the button **name "Network Stats"** (caption "Show Dr S Face") → name lookups collide with button 22 | always |

### 4.3 Network safety — which cheats desynchronise [C] mechanism, [I] consequence

None of the 23 buttons sends a network message: every action mutates the **local** copy of the simulation directly (no `ClientToServer::Request…` call in the file) [C]. In a lockstep game ([22](22_determinism_and_networking.md)) every simulation mutation that is not replayed on the other peers is a desync. In TARGET_DEBUG — the only build where cheats exist — the per-frame sync value is the SHA-1 of a full world snapshot (positions, velocities, fronts of every unit, entity, laser, effect and building) [C] `code/network/iframe.cpp:IFrame::Generate`, so any state-changing cheat is detected at the next frame comparison and freezes the match (§13). A game with a single client (one human versus CPU teams; CPU teams are not network clients) has nobody to disagree with and never reports a desync [C] `code/network/server.cpp:Server::HandleSyncMessage` compares only the values of connected clients.

| # | Cheat | Mutates simulation? | With ≥ 2 clients |
|---|---|---|---|
| 1 | Kill All Enemies | yes (health of every enemy entity, ant hills, triffids) | **desync** |
| 2–7 | Spawn Darwinians (6 teams) | yes (+20 entities, entity ids, population counters) | **desync** |
| 8 | Spawn Armour | yes | **desync** |
| 9 | Spawn Virii | yes | **desync** |
| 10 | Spawn Spirits | yes, and draws 20 values from the synced RNG on one peer only | **desync** (also detectable by the FULL-style RNG-position hash) |
| 11 | Allow Arbitrary Placement | changes only the local copy of the player's task manager; `TaskManager::TargetTask` (run on every peer from the networked target command) calls `IsValidTargetArea`, so a placement accepted locally is rejected remotely | **desync on the first otherwise-invalid placement** [I] |
| 12–14 | Global-world (campaign) cheats | global world state only; not read by Multiwinia rules [I] | none expected [I] |
| 15 | Give all research | no change in Multiwinia [I] | none expected [I] |
| 16 | Clear Resources | render resources only; tree geometry (and the tree hit-check radius computed while building it) is regenerated from the same per-tree seed, re-seeding the unsynced `AppRandom` LCG (`code/worldobject/tree.cpp:Tree::Generate`) | none [I] |
| 17 | Spawn Ports | yes | **desync** |
| 18 | GoodCrate | yes (new building, `GenerateBuildingId` advances the global building-id counter) | **desync** |
| 19–22 | Profiler, Sound Editor, Sound Stats, Network Stats | no (tools; audio is outside the determinism boundary, [27 §15](27_audio_engine.md#15-determinism-boundary)) | safe |
| 23 | Show Dr S Face | no (sky render only, wall-clock timed) | safe |

A replacement must route every cheat through the command stream (a "debug command" message sequenced by the server like any other order) so that all peers apply it on the same tick, and the server must refuse debug commands unless the session was created with cheats enabled.

### 4.4 Other cheat hooks under `CHEATMENU_ENABLED` [C]

| Hook | Effect | Ref |
|---|---|---|
| L / K (`RTLoaderPixelWaveIncrease` / `Decrease`) during `Location::RenderBuildings` | pref `RenderSpecialLighting` := 1 / 0 (warm key light on buildings; render only) | `code/location.cpp:Location::RenderBuildings` |
| Shift held, single-player | Refinery speed ×10 | `code/worldobject/mine.cpp:MineBuilding::RefinerySpeed` |
| Shift held, single-player | Escape-rocket fuel per delivery ×100 | `code/worldobject/rocket.cpp:EscapeRocket::ProvideFuel` |

