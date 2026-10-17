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
| `TARGET_DEBUG` | commented-out target line | **off** | the "debug" target | 20 | turns on `SOUND_EDITOR`, `CHEATMENU_ENABLED`, `GESTURE_EDITOR`, `NETWORK_STATS_ENABLED`, `AVI_GENERATOR`, `TRACK_SYNC_RAND`, `INCLUDE_CRATE_HELP_WINDOW`, TankBattle / Prologue / Campaign modes, and (in `code/worldobject/crate.h`) `INCLUDE_THEBEAST`; does **not** define `DUMP_DEBUG_LOG`. Directly gates: profile directory = working directory (`code/app.cpp:App::GetProfileDirectory`); duplicate-auth-key rejection skipped (`code/network/server.cpp:Server::AuthenticateClient`); full world-snapshot IFrames + SHA-1 sync hash (`code/network/iframe.cpp:IFrame::Generate`); packet-drop key (`code/network/clienttoserver.cpp:ClientToServer::ReceiveLetter`); lobby network-status overlay (`code/UI/GameMenuWindow.cpp:GameMenuWindow::RenderNetworkStatus`); `inputprefs_debug.txt` (`code/lib/input/input.cpp:InputManager::parseInputPrefs`); editor Save/Publish captions and saving into `levels/` (`code/interface/editor_window.cpp`, `code/level_file.cpp`, `code/global_world.cpp`); "Edit Official Maps" button (`code/interface/multiwinia_window.cpp`); Darwinia-mode button → demo page (`code/UI/DarwiniaModeButton.h`); per-datagram server log (`code/network/server.cpp:Server::AdvanceSender`); `Directory` debug checks (`contrib/systemiv/lib/tosser/directory.cpp` defines `_DEBUG`); attack-objective log (`code/worldobject/aiobjective.cpp`); `ToDouble` helper (`code/main.cpp`) | [B:DEBUG] |
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
| 23 | Show Dr S Face | `ShowDrSepulvedaFaceButton` | `Clouds::ShowDrSFace()` → `m_faceTime = now`; from 5 s to 30 s later the sky draws an 800-unit face texture 800 units up at the map centre, alpha `min(t − 5, 1)`; from 28 s the alpha is recomputed as `min(t − 28, 1)`, i.e. it drops to 0 and ramps up again (an inverted fade-out), and every frame writes "alpha = %f" / "alpha2 = %f" to the debug log (`Clouds::Render`, `Clouds::RenderFace`). Registered under the button **name "Network Stats"** (caption "Show Dr S Face") → name lookups collide with button 22 | always |

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

## 5 Network statistics window (F9) and other debug windows

### 5.1 "Network Stats" window — ships in FULL [C] `code/interface/network_window.cpp:NetworkWindow::Create`, `NetworkWindow::Render`

Size 600 × 200; on a host (`App::m_server` non-NULL) the height becomes `170 + (6 × 20 + 15)` = 305 and an `InvertedBox` panel is registered at (10, 110), w = `m_w − 20`, h = 135 for the client table. Positioned bottom-left at (10, ScreenH − h). Font `g_editorFont`; every caption literal English. Text lines are 15 px apart starting at `m_y + 15`.

**Server block (host only)**, size-12 text at x + 10:

| Line | Value | Source |
|---|---|---|
| "Server SeqID       %d" | last sequence id issued | `Server::m_sequenceId` |
| "Server Send        %2.1 Kb/sec" | send rate | `(int)Server::m_sendRate / 1024.0` |
| "Server Receive     %2.1 Kb/sec" | receive rate | `(int)Server::m_receiveRate / 1024.0` |

The format `"%2.1 Kb/sec"` has no conversion letter and receives a `double` → the printed number is undefined (bug; see 32-U6). A horizontal rule follows; then a header row (size 14) "ID", "IP:Port", "SeqID", "Name", "Status" at column offsets 0, 60, 200, 260, 410 from `m_x + 20`, the sync flag at +500.

Up to **6** client rows (`maxClients = 6` hard-coded; slots ≥ 6 are never shown), 20 px apart. For each valid `ServerToClient`:

| Column | Content |
|---|---|
| ID | `m_clientId` (colour (200,200,255,200)) |
| IP:Port | `"%s:%d"` of `m_ip`, `m_port` |
| SeqID | `m_lastKnownSequenceId` |
| Name | translated team-type name (`LANGUAGEPHRASE(Team::GetTeamType(teamType))`) of the lobby team whose `m_clientId` matches, in that team's colour |
| Status | `timeBehind = now − m_lastMessageReceived`; if > **2.0 s** → red "Lost Con...%ds"; else if not `m_caughtUp` → (200,200,30) "Synching...%d" with `100 × lastKnownSeq / serverSeq`; else "Ping %dms" with `latencyMs = (serverSeq − lastKnownSeq) × 100` (one sequence = 100 ms, the 10 Hz server tick) and colour red = `min(255, 20 × lagSteps)`, green = 255 − red |
| sync flag | red "out of sync" when `m_syncErrorSeqId ≠ −1` |

Empty slots print "Empty" at alpha 50.

**Client block** (whenever `App::m_clientToServer` exists):
- If `g_lastProcessedSequenceId ≥ 0`: caption (size 13) "Sequence messages in queue. Ideal is 0 or 1 filled, no gaps." at `m_y + m_h − 50`, then a strip at `m_y + m_h − 25`: label "SeqID %d" (last processed id), then 10 boxes for ids last+1 … last+10, each `(m_w − 120)/10` = 48 px wide minus a 20 % gap, 20 px high, background (20,20,50), filled green (0,255,0) when `ClientToServer::IsSequenceIdInQueue(id)`, frame (255,255,255,100).
- Non-host clients add "Server Known     SeqID   %d" (`m_serverSequenceId`), "Server Estimated SeqID   %d" (`GetEstimatedServerSeqId()`) and a centred size-15 "Estimated Latency : %2.1 secs" (`GetEstimatedLatency()`, same broken format).
- Right column at x + 250 from the top: "Client SEQID       %d" (`m_lastValidSequenceIdFromServer`), "Client Send … Kb/sec", "Client Receive … Kb/sec" (`m_sendRate`, `m_receiveRate`, same broken format).

Diagnostic value: the window exposes exactly the lockstep health indicators — per-client sequence lag (≈ ping in 100 ms steps), catch-up state, last-message age with a 2.0 s "lost" display threshold, the per-client desync flag and the client's inbox gaps. Rates are 5-second averages in bytes/s from `BandwidthCounter` (`code/network/bandwidth.cpp:BandwidthCounter::Count`: when more than 5.0 s have passed since `m_since`, `m_rate = m_count / period`, then the count restarts); there is no bandwidth limiting anywhere, only measurement ([22](22_determinism_and_networking.md)).

### 5.2 Profiler window → §9.

### 5.3 Sound tools [B:DEBUG]

Sound Editor, Effect Editor, Sample-group editor, Parameter editor, Purge Sounds, Sound Stats, Sound Profile and the Pokey playground are specified in [27 §13](27_audio_engine.md#13-profiles-statistics-debug-and-editor-tooling). Reachability: only "Sound Editor" and "Sound Stats", from cheat-window buttons 20–21; the debug-menu entries need `modsEnabled` (never true) and the hot-keys are commented out [C] `code/interface/debugmenu.cpp`, `code/user_input.cpp`. `PokeyWindow` is referenced but defined nowhere, so a `SOUND_EDITOR` build that compiled `DebugKeyBindings::PokeyButton` would need that class [C grep].

### 5.4 Grabber window (AVI capture) [B:DEBUG, non-functional] [C] `code/interface/grabber_window.cpp`, `code/lib/avi_generator.cpp`, `code/lib/hi_res_time.cpp`

Buttons "Start" (10, 30, 70 × 15) and "Stop" (90, 30, 70 × 15). Start (if no generator): `SetFakeTimeMode()` (freezes `GetHighResTime` at the current value so the game advances only by explicit `IncrementFakeTime` steps), create `App::m_aviGenerator` with a `BITMAPINFOHEADER` of ScreenW × ScreenH, 24 bpp, `BI_RGB`, `InitEngine()`. Stop: `SetRealTimeMode()` (re-bases real time by the frozen offset), `ReleaseEngine()`, delete. `AVIGenerator` reads prefs `AVIMotionBlurFactor` (default 1) and `DemoFrameRate` (default 25). **No code calls `AVIGenerator::AddFrame()`** and `GrabberButton` has no caller, so capture never records a frame [C grep]. The fixed-step clock it relies on is advanced only when pref `RecordDemo` is 1 or 2 (`code/main.cpp:UpdateAdvanceTime`: `g_advanceTime = 1/DemoFrameRate`, `IncrementFakeTime(1/DemoFrameRate)` per frame) — see [31 §4.2](31_platform_build_and_tools.md#42-fake-time-offline-capture-c-codelibhi_res_timecpp-codemaincppupdateadvancetime-codeinterfacegrabber_windowcpp).

### 5.5 Gesture editor [B:DEAD] [C] `code/interface/gesture_window.cpp`, `code/gesture_demo_win32.cpp`

Training tool for the gesture recogniser ([06](06_player_control_and_input.md)): a 4:3 drawing box (height `m_h − 180`) where mouse-down/up record a sample; `(m_w − 100)/110` status tiles of 90 × 120 each showing gesture *i*'s task name (`TaskManager::MapGestureToTask`), icon `icons/gesture_<taskname>.bmp` (64 × 64, additive), number of training samples, confidence % and Mahalanobis deviation, the recognised symbol framed yellow; buttons "Record Demo" (`RecordGestureDemo()` — a recorder thread at `THREAD_PRIORITY_TIME_CRITICAL`), "New Gesture" (caption "New Demo", `NewSymbol()`), "Load Data" / "Save Data" (`gestures.txt`). Requires `GESTURE_EDITOR` **and** `USE_SEPULVEDA_HELP_TUTORIAL` → compiled out everywhere.

### 5.6 Lobby network-status overlay [B:DEBUG] [C] `code/UI/GameMenuWindow.cpp:GameMenuWindow::RenderNetworkStatus`

Drawn on two front-end pages only under `TARGET_DEBUG`, `g_gameFont` size 15, line height 18, from (10, 10), colour (1,1,1,0.2): on a host the header "IP:Port [Sent/Known/CaughtUp]" and one line per client `"%s [%d/%d/%d]"` (`m_lastSentSequenceId`, `m_lastKnownSequenceId`, `m_caughtUp`); then the client connection state: "Disconnected", "Connecting (%d)" (attempts), "Handshaking (%d/%d)" (`g_lastProcessedSequenceId` / `m_lastValidSequenceIdFromServer`) or "Connected".

### 5.7 Crate help window [B:DEBUG] [C] `code/interface/helpandoptions_windows.cpp`

`INCLUDE_CRATE_HELP_WINDOW` adds a "multiwinia_menu_cratehelp" button to the Help & Options page that opens a list of every crate reward with its tooltip/hint strings ([04](04_crates_powerups_and_weapons.md)).

### 5.8 Hidden preferences that act as debug switches [B:FULL] [C] (pref reads as listed; full key list in [29](29_data_formats_reference.md))

| Preference | Default | Effect | Ref |
|---|---|---|---|
| `RenderAIInfo` | 0 | 1 → every AITarget prints the AI priority `m_priority[t]` ("%2.2f", outlined, team colour) for one team at 60 + 15·t above the node; ≠ 0 → arrows along every AITarget link (half length, width 3, grey for neutral, team colour otherwise). The AIObjective / marker overlays it also tests are inside `#ifdef LOCATION_EDITOR` → nothing in FULL ([07 §18](07_ai.md#18-debug-visualisation)) | `code/worldobject/ai.cpp:AITarget::Render`, `AITarget::RenderAlphas`, `code/worldobject/aiobjective.cpp:AIObjective::RenderAlphas` |
| `AIInfoTeam` | 1 | lobby **position** whose priorities `RenderAIInfo` shows (`Location::GetTeamFromPosition`) | `code/worldobject/ai.cpp:AITarget::Render` |
| `RecordDemo` | absent (0) | 1 → fixed time step `1/DemoFrameRate` and `Server::SaveHistory("serverHistory.dat")` when the game shuts down; 2 → fixed step and `Server::LoadHistory("ServerHistory.dat")` after the level loads (command-stream replay, [22](22_determinism_and_networking.md)) | `code/main.cpp:UpdateAdvanceTime`, `code/app.cpp:App::ShutdownCurrentGame` |
| `DemoFrameRate` | 25 | frames per simulated second for `RecordDemo` and the AVI generator | `code/main.cpp:UpdateAdvanceTime`, `code/lib/avi_generator.cpp` |
| `TextLanguageTest` | 0 | 1 → `LangTable::TestAgainstEnglish` writes `language_errors.txt` (strings missing versus English) in the working directory ([31 §12.5](31_platform_build_and_tools.md#125-in-game-qa-c-codeliblanguage_tablecpplangtabletestagainstenglish)) | `code/app.cpp:App::SetLanguage`, `code/lib/language_table.cpp` |
| `RenderPosterResolution` | 1 | N > 1 → the screenshot key renders an N × N tiled high-resolution poster | `code/renderer.cpp:Renderer::Render`, `code/lib/poster_maker.cpp` |
| `RenderLandscapeMode` | −1 (auto) | forces vertex arrays / display lists / VBOs for the terrain mesh (§12.3) | `code/landscape_renderer.cpp:LandscapeRenderer::LandscapeRenderer` |
| `RenderSpecialLighting` | 0 | warm key light on buildings | `code/location.cpp:Location::RenderBuildings` |
| `TestHarness` | absent | 1 → runs the campaign test harness, only with `TEST_HARNESS_ENABLED` (§7) | `code/main.cpp:RunTheGame` |
| `TestbedServerName` | — | server name the testbed hosts / looks for, only with `TESTBED_ENABLED` (§6) | `code/app.cpp:App::App` |

Command-line switches with a diagnostic role: `--write-version` (write `DARWINIA_VERSION` to a file `version` and exit) and `--NoProfile` (use the working directory instead of the profile directory for every log) [C] `code/main.cpp:AppMain`, `code/main.cpp:Initialise` ([31 §5.4](31_platform_build_and_tools.md#54-command-line-switches-c)).

## 6 Testbed mode

### 6.1 Purpose, build tag and activation [B:DEAD] [C] `code/UI/TestBedButtons.h:StartTestbedButton`, `code/UI/GameMenuWindow.cpp:GameMenuWindow::SetupMainPage`, `code/app.h`

The testbed is an **unattended multi-machine soak test of the lockstep network game**: one machine runs as testbed *server*, any number of others as testbed *clients*; the server walks through every (game mode, map) pair, every machine joins as a spectator so the CPU teams play the whole match, and sync errors are counted and logged per game. It exists only with `TESTBED_ENABLED`, which is commented out inside the `TARGET_DEBUG` block and defined in no target → dead at this commit.

Activation: two extra main-menu buttons after "Help and options" — "start_testbed_server" (`StartTestbedButton(TESTBED_SERVER)`, text `start_testbed_server` = "Testbed Server") and a second button whose caption is `start_testbed_client` ("Testbed Client") but whose **name is also "start_testbed_server"** (name collision). `MouseUp` sets: mode `TESTBED_ON`, state `TESTBED_BUILD_GAME_ENTRIES`, type server/client, game mode −1, map −1, game count 0, sync-error count 0, rendering **off**, `m_iGameIndex = 4`, `m_iLastGameIndex = 0`. The driver `GameMenuWindow::UpdateTestBed()` runs every front-end frame (`GameMenuWindow::Update`).

Enumerations [C] `code/app.h`: `TESTBED_MODE {TESTBED_ON, TESTBED_OFF}`; `TESTBED_TYPE {TESTBED_CLIENT, TESTBED_SERVER}`; `TESTBED_STATE {TESTBED_MAINMENU, TESTBED_HOST_GAME, TESTBED_JOIN_GAME, TESTBED_PICK_SERVER, TESTBED_WAIT_FOR_CLIENT_ID, TESTBED_SELECT_GAME, TESTBED_SELECT_MAP, TESTBED_SETGAMEOPTIONSDELAY, TESTBED_SETGAMEOPTIONS, TESTBED_WAITFORGAMEOPTIONS, TESTBED_WAIT_FOR_MAP, TESTBED_WAIT_FOR_CLIENTS, TESTBED_PLAY, TESTBED_IDLE, TESTBED_GAME_OVER_DELAY, TESTBED_MAKE_CLIENT_SPECTATOR, TESTBED_SEQUENCEID_DELAY, TESTBED_BUILD_GAME_ENTRIES}` (18 values; `TESTBED_WAITFORGAMEOPTIONS` is never entered); `TESTBED_PROGRESSION {TESTBED_LINEAR, TESTBED_RANDOM, TESTBED_SEQUENCE}` (`code/UI/GameMenuWindow.cpp`).

App state (24 fields) [C] `code/app.h`: `m_eTestBedMode`, `m_eTestBedType`, `m_eTestBedState`, `m_fTestBedDelay`, `m_fTestBedLastTime`, `m_fTestBedCurrentTime`, `m_iDesiredSequenceID`, `m_eNextState`, `m_iClientCount`, `m_iGameMode`, `m_iGameMap`, `m_iGameIndex`, `m_iLastGameIndex`, `m_iGameCount`, `m_iNumSyncErrors`, `m_cGameMode[100]`, `m_cGameMap[100]`, `m_iClientRandomSeed`, `m_bRenderOn`, `m_iLastGame`, `m_iLastMap`, `m_testbedServerName` (pref `TestbedServerName`), `m_iTerminationSequenceID`; accessors `Get/SetTestBedMode`, `Get/SetTestBedType`, `Get/SetTestBedState`, `GetTestBedServerName`, `Get/SetTestBedGameIndex`, `Get/SetTestBedGameMode`, `Get/SetTestBedGameMap`, `GetTestBedGameCount`, `IncrementGameCount`, `ResetGameCount`, `GetTestBedNumSyncErrors`, `IncremementNumSyncErrors`, `ResetNumSyncErrors`, `Set/GetTestBedClientRandomSeed`, `SetSequenceIDDelay(seq, next)` (store target id and next state, enter `TESTBED_SEQUENCEID_DELAY`), `ToggleTestBedRendering`, `GetTestBedRendering`, `SetTestBedRenderOn`, `SetTestBedRenderOff`, `TestBedLogWrite`.

### 6.2 The game table and progressions [C] `code/UI/GameMenuWindow.cpp:BuildGameEntries`, `GameMenuWindow::AdvanceTestBedGame`, `FindGameEntry`

- `BuildGameEntries`: one `GAME_ENTRY {gameNumber, mapNumber, mapCRC}` for every valid map of every game-type list `GameMenu::m_maps[i]` **except list index 6** (the code comment: the menu finds 7 game types but buttons exist for only 6). Defect: `m_iMapCRC` is read from `maps.GetData(i)` (the game-type index) instead of `x` — wrong or out-of-range [C].
- `AdvanceTestBedGame` (server only; clients learn the game from the server they join):
  - `LINEAR`: index + 1 while index < number of entries (it is called right after the table is built, so **entry 0 is skipped**; once index reaches the table size it stays there and the next `g_vGameEntries[index]` read is out of bounds) [C].
  - `RANDOM`: `rand() % entries` (CRT `rand`, seeded `srand(time(NULL))` on table build).
  - `SEQUENCE`: walk the `AddSequenceItem` list cyclically; each item sets the entry index and a forced sync seed `g_iSelectedRandSeed`.

### 6.3 `testbedconfig.txt` [C] `code/UI/GameMenuWindow.cpp:GameMenuWindow::ReadTestBedConfig`

Read from the **working directory** (`TextFileReader("testbedconfig.txt")`, not the resource archives) at `TESTBED_BUILD_GAME_ENTRIES`; missing file → defaults (LINEAR, seed 0, no termination). One command per line, tokens separated by whitespace, `#` starts a comment (TextReader rules, [29](29_data_formats_reference.md)). No such file ships in the tree [C].

```
SetProgression LINEAR | RANDOM | SEQUENCE
SetRandSeed <int>                 # 0 = use the server's normal seed
SetTestBedName <word>             # sscanf "%s": a single token, overrides pref TestbedServerName
AddSequenceItem <gameIndex> <seed> # appended to the SEQUENCE list (list cleared on each read)
SequenceIDToTerminateOn <int>     # end each game when g_lastProcessedSequenceId reaches it (0 = off)
```

### 6.4 Server automation [C] `code/UI/GameMenuWindow.cpp:GameMenuWindow::UpdateTestBed`

Every frame, before the switch: if a server exists, `Server::m_iRandSeed = g_iSelectedRandSeed` and `m_bRandSeedOverride = (seed ≠ 0)`; if a client exists, `ReliableSetTeamName(testbed name)`.

```mermaid
stateDiagram-v2
    [*] --> BUILD_GAME_ENTRIES
    BUILD_GAME_ENTRIES --> MAINMENU: build table, read config, advance game
    MAINMENU --> HOST_GAME: on main page press Multiplayer
    HOST_GAME --> WAIT_FOR_CLIENT_ID: press Host
    WAIT_FOR_CLIENT_ID --> SELECT_GAME: own team id assigned
    SELECT_GAME --> SELECT_MAP: press game type button of entry
    SELECT_MAP --> WAIT_FOR_MAP: press level button of entry
    WAIT_FOR_MAP --> SETGAMEOPTIONSDELAY: requested map confirmed
    SETGAMEOPTIONSDELAY --> SETGAMEOPTIONS: 5 sequence ids later
    SETGAMEOPTIONS --> SEQUENCEID_DELAY: register as spectator
    SEQUENCEID_DELAY --> WAIT_FOR_CLIENTS: 10 ids later
    WAIT_FOR_CLIENTS --> SEQUENCEID_DELAY: more than 1 client connected
    SEQUENCEID_DELAY --> PLAY: 10 ids later
    PLAY --> IDLE: lobby page left, game count plus 1
    IDLE --> GAME_OVER_DELAY: game over or sync error or termination id
    GAME_OVER_DELAY --> MAINMENU: 10 s, advance game, remove own teams
```

Details per state:
- `TESTBED_MAINMENU`: delay and client count reset; on `PageMain` press `NewOrJoinButton` "multiwinia_menu_multiplayergame".
- `TESTBED_HOST_GAME`: on `PageNewOrJoin` press "multiwinia_menu_hostgame".
- `TESTBED_WAIT_FOR_CLIENT_ID`: wait until `GlobalWorld::m_myTeamId ≠ 255`.
- `TESTBED_SELECT_GAME`: on `PageGameSelect` press the game-type button for entry `gameNumber` 0–5 ("multiwinia_gametype_domination", "…_kingofthehill", "…_capturethestatue", "…_assault", "…_rocketriot", "…_blitzkrieg") and copy the English mode name into `m_cGameMode` ("Domination", "King Of The Hill", "Capture the Statue", "Assault", "Rocket Riot", "Blitzkrieg").
- `TESTBED_SELECT_MAP`: on `PageMapSelect` press the `LevelSelectButton` named by the map's level name (or file name); `m_cGameMap` = translated map name.
- `TESTBED_WAIT_FOR_MAP`: when `m_localSelectedLevel == m_requestedMapId` and the map index is valid, remember the current sequence id.
- `TESTBED_SETGAMEOPTIONSDELAY`: wait 5 sequence ids (0.5 s).
- `TESTBED_SETGAMEOPTIONS`: `RequestToRegisterAsSpectator(myTeamId)` (time-limit option code is commented out), then `SetSequenceIDDelay(+10, WAIT_FOR_CLIENTS)`.
- `TESTBED_WAIT_FOR_CLIENTS`: on `PageGameSetup`, when `Server::m_clients.NumUsed() > 1` → `SetSequenceIDDelay(+10, PLAY)`.
- `TESTBED_PLAY`: the lobby's "all ready" test is replaced by `notReady = (state ≠ TESTBED_PLAY)` (`GameMenuWindow::UpdateMultiplayerPage`), so the game starts; when the page changes away from `PageGameSetup` → game count + 1, `TESTBED_IDLE`.
- `TESTBED_GAME_OVER_DELAY`: accumulate wall time; after **10 s** advance to the next entry, remove own teams, clear spectator, back to `TESTBED_MAINMENU`.

### 6.5 Client automation [C] same function, client branch

```mermaid
stateDiagram-v2
    [*] --> BUILD_GAME_ENTRIES
    BUILD_GAME_ENTRIES --> MAINMENU: build table, read config
    MAINMENU --> JOIN_GAME: press Multiplayer
    JOIN_GAME --> PICK_SERVER: press Join
    PICK_SERVER --> MAKE_CLIENT_SPECTATOR: every 3 s scan browser for server named like testbed
    MAKE_CLIENT_SPECTATOR --> IDLE: game type and map known, game index found
    PICK_SERVER --> PICK_SERVER: connecting page times out after 10 s
    IDLE --> GAME_OVER_DELAY: game over or sync error or termination id
    GAME_OVER_DELAY --> MAINMENU: 40 s, remove own teams
```

- `TESTBED_PICK_SERVER`: on `PageJoinGame`, every **3 s** walk `m_serverButtons`; pick the first `GameServerButton` whose `m_hostName` equals the testbed name and whose map name is not "Unknown Map"; copy game type and map into the log fields, `MouseUp` (the "server full" refusal is bypassed in testbed mode, `code/UI/GameServerButton.h`), game count + 1.
- On `PageGameConnecting` the testbed accumulates time and after **10 s** without connecting presses "multiwinia_menu_back" and returns to `TESTBED_PICK_SERVER` (`GameMenuWindow::UpdateGameConnectingPage`; the timer is reset when `GameMenuWindow::JoinServer` starts the connection).
- `TESTBED_MAKE_CLIENT_SPECTATOR`: when `m_gameType` and `m_requestedMapId` are known, `m_iGameIndex = FindGameEntry(type, map)` → `TESTBED_IDLE`. The server registers every testbed client as a spectator on join (§6.7).
- `TESTBED_GAME_OVER_DELAY`: **40 s**, then remove own teams, clear spectator, `TESTBED_MAINMENU`.
- Disconnection by the server (disconnect letter) → `TESTBED_MAINMENU` [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessServerLetters`.

### 6.6 In-match automation and `TestBedLog.txt` [C] `code/location_input.cpp:LocationInput::Advance`, `code/app.cpp:App::TestBedLogWrite`

In a match with testbed on:
1. During the grace period the ready toggle is replaced by an automatic `RequestSetTeamReady(myTeamId)` every frame (no window open).
2. If `ClientToServer::m_outOfSyncClients` is non-empty: leave the location (`m_requestedLocationId = −1`, `m_atMainMenu = true`), and unless already in `GAME_OVER_DELAY` increment `m_iNumSyncErrors` and write a log row; state → `GAME_OVER_DELAY`.
3. If `m_iTerminationSequenceID ≠ 0` and reached: leave, log a row, `GAME_OVER_DELAY`.
4. On `Multiwinia::GameOver()` with no window open: leave, log a row (unless already delaying), `GAME_OVER_DELAY`.

`TestBedLogWrite` appends to `TestBedLog.txt` in the **working directory** (Mac: profile directory); header row written when the file does not yet exist; fields in this order, joined with ", ":

| Column | Value |
|---|---|
| Date | `asctime(localtime(now))` without the trailing newline |
| NumSyncErrors | `m_iNumSyncErrors` (cumulative for the session) |
| GameMode | `m_cGameMode` |
| GameMap | `m_cGameMap` |
| GameIndex | `m_iGameIndex` |
| ClientRandomSeed | `m_iClientRandomSeed` (the sync seed received in CLIENTID, `code/network/clienttoserver.cpp`) |
| LastGameIndex | `m_iLastGameIndex` |
| GameTimer | `GameTimer::GetGameTime()` |
| SyncSeqId | `ClientToServer::m_syncErrorSeqId` |

The seed column plus `SetRandSeed`/`AddSequenceItem` let a failing game be replayed with the same seed (§13).

### 6.7 Engine changes compiled under `TESTBED_ENABLED` [C]

| Change | Ref |
|---|---|
| `GRACE_TIME` 20.0 s instead of 3.0 s | `code/lib/universal_include.h` |
| Game-option blueprints from `data/multiwinia-testbed.txt` instead of `data/multiwinia.txt` — identical except one added `TimeLimit 0 60 10 1` row for `GAMETYPE RocketRiot` ("TimeLimit added to Rocket Riot by John for testbed"), so unattended Rocket Riot games end | `code/multiwinia.cpp:Multiwinia::LoadBlueprints`, `data/multiwinia-testbed.txt` |
| CLIENTID carries `Server::m_iRandSeed` instead of `m_syncRandSeed` when the override is set and non-zero | `code/network/server.cpp:Server::SendClientId` |
| Server registers every joining client as a spectator (client-type testbed: all; server-type: every client except the host's own, counting `m_iClientCount`) | `code/network/server.cpp:Server::AuthenticateClient` |
| Duplicate-auth-key rejection skipped (also skipped in `TARGET_DEBUG`) | `code/network/server.cpp:Server::AuthenticateClient` |
| Client connection-attempt limit (`CTOS_MAX_CONNECTION_ATTEMPTS` = 50) not applied while testbed is on | `code/network/clienttoserver.cpp` connect loop, `code/network/clienttoserver.h` |
| Testbed server: `m_nextServerAdvanceTime += 0.1` in every branch — the "advance 10 times faster" comment is not implemented; the "server more than 30 ids ahead" branch is also +0.1 | `code/network/server.cpp:Server::AdvanceIfNecessary` |
| `Location::Render`, `ParticleSystem::Render`, `ExplosionManager::Render` return early and sound is muted (`SoundSystem::NotReadyForGameSoundsYet` returns true) while `m_bRenderOn` is false; `ToggleRendering` (R) toggles it | `code/location.cpp`, `code/particle_system.cpp`, `code/explosion.cpp`, `code/sound/soundsystem.cpp` |
| team-slot debug asserts disabled | `code/multiwinia.cpp:Multiwinia::InitialiseTeam` |
| Server-full dialog bypassed when joining | `code/UI/GameServerButton.h` |
| Client stores the received seed in `m_iClientRandomSeed` | `code/network/clienttoserver.cpp` |

### 6.8 Defects to avoid [C]

Wrong CRC index in `BuildGameEntries`; entry 0 skipped and out-of-range read at the end of LINEAR; duplicate button names; delays measured with `GetHighResTime` deltas stored in `float`; the testbed keeps a global `GAME_SEQUENCE` and statics in a UI translation unit. The design idea (scripted menu driving + spectator-only peers + per-game sync log keyed by seed) is sound and worth keeping as a headless CI job (§14).

## 7 Test harness and script test

### 7.1 Campaign test harness (`TestHarness`) [B:DEAD] [C] `code/testharness.cpp`, `code/main.cpp:RunTheGame`, `code/global_world.cpp:GlobalEventAction::Execute`

Activation: with `TEST_HARNESS_ENABLED`, after `Initialise()`, pref `TestHarness == 1` → `m_testHarness = new TestHarness(); TestHarnessLoop()` (= `RunTest()`); `Finalise(); exit(0)`. Never renders. The whole TU is inside `#ifdef TEST_HARNESS_ENABLED`; `App::m_testHarness` is compiled in FULL and always NULL.

What it automates: an offline, exhaustive **mission-graph explorer** for the Darwinia campaign's global world — it "completes" missions in every possible order (depth first over all available missions), applies each mission's objectives to the global state, re-evaluates global events, and reports every reachable end state and data error. It runs neither the simulation nor the AI. Irrelevant to Multiwinia gameplay ([B:LEGACY] content).

Algorithm:
- Output `test_log.txt` in the working directory, unbuffered (`setvbuf(_IONBF)`); `m_indent` points into a 255-space buffer, starting at its end (empty string); indenting moves the pointer back 3 characters.
- `ExploreStates(world)`: (1) `PrintGlobalState`: per location `"%-16s %-37s %-8s"` name, mission file, available/unavailable. (2) `FindAvailableMissions`: locations with `m_available` and mission ≠ "null"; missions whose file name starts with "cutscene" → `PrintError("Cutscene mission file left hanging around in <loc>")` and skipped. (3) none → end state: "Game Over. Missions done: N" plus the list; a `CompletedGame` stores each mission name minus its first 8 characters ("mission_"). (4) otherwise for each available mission: deep-copy the `GlobalWorld`, make it `g_app->m_globalWorld`, `DoMission`, recurse, pop, "End of mission in <loc>", delete the copy, restore.
- `DoMission`: log "Doing mission in location %s: %s"; push name; indent; load `LevelFile(missionFile, mapFile)`; `AchieveObjective` for every primary then every secondary objective; set the location's mission to "null"; `while (world->EvaluateEvents());` (with the harness on, `GlobalEventAction::Execute` logs `SetMission`, `RunScript`, `MakeAvailable`).
- `AchieveObjective` per `GlobalEventCondition`: `AlwaysTrue`, `DebugKey`, `NotInLocation` → nothing; `BuildingOffline` → global building offline (error if bad id); `BuildingOnline` → online, and for a `TrunkPort` the destination location becomes available (error if invalid); `ResearchOwned` → `AddResearch(id)`, "Player collects research %d"; `NeverTrue` → not handled.
- `RunTest`: explore from the live world, then a summary of each completed game's mission count and list, and "Number of errors: N".
- Defects: the destructor `delete[] m_indent` frees the moved pointer (undefined behaviour); the indent underflows after 85 levels; the search is factorial in the number of simultaneously available missions (no memoisation).

### 7.2 Script test (`SCRIPT_TEST_ENABLED`) [B:DEAD] [C] `code/script.cpp:Script::TestScript`, `Script::ReportError`

When a script is started, `TestScript(file)` first parses it statically and reports through `ReportError` (to `TestHarness::PrintError` if a harness exists, else `AppDebugAssert(false)`), appending " (map mission line: N)" or " (no map, no mission, line: N)". Checks by opcode:

| Opcode | Errors reported |
|---|---|
| `CamMove` | "called with no mount specified"; "called when not in a location"; "called with invalid mount name: %s"; "called with no duration parameter"; "called with invalid duration parameter: %f" |
| `CamCut` | no mount; not in a location; invalid mount name |
| `CamAnim` | no anim name; bad anim name |
| `EnterLocation` | no location name; invalid location name |
| `ExitLocation`, `WaitSay`, `WaitCam`, `ClearHighlights` | none |
| `Say` | no language symbol; invalid language symbol |
| `Wait` | invalid duration |
| `Highlight` | invalid building id |
| unknown word | "Invalid script command: %s" |
| (file) | "Script file not found" |

`Script::IsRunningScript()` returns false whenever a harness exists [C]. Scripts cannot run in Multiwinia at all ([30](30_scripting_tutorial_and_cutscenes.md)).

## 8 Benchmark mode

### 8.1 What "benchmark mode" is in this code base

There is **no benchmark scene, no scripted camera fly-through and no score** anywhere in the tree [C grep for benchmark/score paths]. "Benchmark" consists of three pieces:

| Piece | What it is | Tag |
|---|---|---|
| `BenchMark` recorder (`code/benchmark.cpp`) | per-second telemetry logger active during every Multiwinia match, with a hardware header from DirectX Diagnostic and an FTP upload at match end | [B:DEAD] here (`BENCHMARK_AND_FTP` commented out) |
| Stress-test maps `data/levels/mp_benchmark_1.txt` ("Stress Test") and `data/levels/mp_assault_stress.txt` ("Assault Stress Test") | content that loads the simulation heavily | data present; not selectable in FULL (§8.5) |
| Hardware-compatibility SKU (`TARGET_PC_HARDWARECOMPAT`, `tools/build_multiwinia_hwcompat.bat`) | spectator-only, single-player-only build whitelisting `MAPID_MP_KOTH_2P_1`, `MAPID_MP_BENCHMARK_1`, `MAPID_MP_KOTH_4P_1`; its packaging copies `ncftpput.exe` (the benchmark uploader) next to the exe | SKU build [C]; that it shipped with `BENCHMARK_AND_FTP` enabled is [I] (32-U3) |

So the intended "benchmark run" was [I]: a tester installs the hwcompat build, watches CPU teams play (spectator only) on two normal maps and the stress map, and the recorder uploads fps / bandwidth / latency / entity-count traces plus a hardware summary. `docs/multiwiniareleasenotes-beta.txt` describes the same recorder as active in a public beta (Docs-vs-code).

### 8.2 Build tag and life cycle [C] `code/benchmark.cpp:BenchMark`, `code/app.cpp:App::App`, `code/main.cpp:EnterLocation`, `code/main.cpp:LocationGameLoop`

`benchmark.cpp` is entirely inside `#ifdef BENCHMARK_AND_FTP`; the macro's only mention in `code/lib/universal_include.h` is the commented line inside the `TARGET_BETATEST_GROUP` block, so it is off in every target. `App::m_benchMark` and all call sites are gated.

```mermaid
sequenceDiagram
    participant App as App constructor
    participant DX as dxdiag.exe
    participant Loop as Location loop
    participant BM as BenchMark
    participant FTP as ncftpput.exe
    App->>BM: new BenchMark, RequestDXDiag
    BM->>DX: ShellExecute open dxdiag.exe /t profile dxdiag.txt
    Loop->>BM: Begin before LocationGameLoop
    BM->>BM: create benchmarks folder, parse dxdiag.txt, Multiwinia only, write header
    loop every frame of the match
        Loop->>BM: Advance
        BM->>BM: retry Begin if not recording, else one sample per second
    end
    Loop->>BM: End after LocationGameLoop, append marker line
    Loop->>BM: Upload
    BM->>FTP: ShellExecute hidden, 60 s timeout, upload file
```

1. **Construction** (`App::App`): `m_benchMark = new BenchMark(); RequestDXDiag()` → `ShellExecute("open", "dxdiag.exe", "/t <profile>dxdiag.txt", SW_SHOWNORMAL)` inside SEH `__try/__except` (failures swallowed); dxdiag writes its report asynchronously.
2. **`Begin()`** — at every non-editor location entry (before `LocationGameLoop`) and retried from every `Advance()` while not recording: `CreateDirectory(<profile>benchmarks)`; `BuildDxDiag()` — return if `dxdiag.txt` cannot be opened yet; return unless `App::m_gameMode == GameModeMultiwinia`; `authKey = Authentication_GetKey`; `m_filename = "<profile>benchmarks/game <authKey> <timeIndex>.txt"` (newlines in the key are not stripped although a comment says they should be); return if the file exists; `m_recording = true`; write the header (§8.3).
3. **`Advance()`** — every location-loop frame: if recording and `now ≥ m_timer` → `m_timer = now + 1.0`, append one sample line (file opened and closed per write).
4. **`End()`** — after `LocationGameLoop` returns: append a line "@"; log "Benchmark stopped".
5. **`Upload()`** — immediately after `End()`: if the file exists, `ShellExecute("open", "ncftpput.exe", "-t 60 -u <user> -p <password> <Introversion FTP host> . \"<file>\"", SW_HIDE)` and log "Uploading benchmark...result code %d". The account credentials are hard-coded in the source; they are deliberately **not reproduced** here and a new game must never embed credentials.

### 8.3 Output file format [C] `code/benchmark.cpp:BenchMark::Begin`, `BenchMark::BuildBenchmark`, `BenchMark::BuildTimeIndex`, `BenchMark::BuildDxDiag`

Header (comma-separated, label column padded):

| Line | Content |
|---|---|
| `USR AuthKey             , <key>` | the player's authentication key |
| `USR Build               , <DARWINIA_VERSION_STRING>` | `"<platform>-<gametype>-<version>"` |
| `USR Game                , <name>` | current game blueprint name |
| `USR Map                 , <file>` | `App::m_requestedMap` |
| `USR Screen              , <W> x <H> <windowed\|fullscreen>, <bpp> bit Colour, <hz> hz, <z> zDepth` | prefs `ScreenWidth`, `ScreenHeight`, `ScreenWindowed`, `ScreenColourDepth`, `ScreenRefresh`, `ScreenZDepth` |
| `<TAG> <identifier padded to 20>,<value>` | filtered dxdiag lines (below) |
| (blank line) | |

dxdiag filter: `dxdiag.txt` is read with `TextFileReader` (strips line terminators, truncates at `#`, and returns no rest-of-line for an empty line); the loop **stops at the first blank line** [C], so with dxdiag's usual layout only the first section is captured [I] (32-U4). Section headers switch the permitted fields; each field is accepted once per header; commas in values become spaces:

| Header substring | Tag | Permitted identifiers (case-insensitive, text before the first `:`) |
|---|---|---|
| `System Information` | `SYS` | Operating System, Language, System Manufacturer, System Model, BIOS, Processor, Memory, DirectX Version |
| `Display Devices` | `GFX` | Card name, Chip type, Display Memory, Driver Version, Driver Date/Size |
| `Sound Devices` | `SND` | Description, Driver Version, Date and Size |

Sample line, once per wall-clock second, format `"%s,%5d,%6d,%6d,%5d,%5d"`:

| # | Field | Source | Unit |
|---|---|---|---|
| 1 | time index | `BuildTimeIndex()` = local time `YYYY-MM-DD hh.mm.ss` | wall clock |
| 2 | frames rendered | `Renderer::m_fps` | frames in the last second |
| 3 | bandwidth in | `Server::m_receiveRate` when hosting, else `ClientToServer::m_receiveRate` | bytes/s, 5-s average (resolved, §1) |
| 4 | bandwidth out | `…m_sendRate` likewise | bytes/s, 5-s average |
| 5 | latency | `ClientToServer::GetEstimatedLatency() × 1000` | ms |
| 6 | entity count | `Entity::s_entityPopulation` (all live entities; AI entities excluded) | count |

Final line "@" marks a cleanly ended match.

### 8.4 Metrics and scoring

Metrics: exactly the six sample columns plus the header. **Scoring: none** — no aggregate, threshold, pass/fail or comparison is computed client-side; analysis was presumably done offline on the uploaded files [I].

### 8.5 The stress-test maps [C] `data/levels/mp_benchmark_1.txt`, `data/levels/mp_assault_stress.txt`, `data/language/english.txt`, `code/mapcrc.h`

| Map | Options | Content | Purpose |
|---|---|---|---|
| `mp_benchmark_1` ("Stress Test"; description: designed to spawn units indefinitely "until your computer falls over") | `GameTypes none`, `PopulationCap -1` (no cap), `NumPlayers 4`; `MAPID_MP_BENCHMARK_1` = `0x49bda3c0` | 4 × 150 Darwinians at start; 24 SpawnPoint, 4 SpawnLink, 4 TrunkPort, 15 MultiwiniaZone, 13 AITarget, 18 Tree, 1 SpawnPopulationLock | unbounded population growth → finds the entity count at which a machine collapses |
| `mp_assault_stress` ("Assault Stress Test") | `GameTypes none`, `PopulationCap 2000`, `NumPlayers 2`, `trunkportreinforcements 250`, defender reinforcement Armour, `AttackingTeams 0`, `OfficialLevel` | 90 Darwinians at start; 30 Wall, 30 AITarget, 26 LaserFence, 10 AIObjectiveMarker, 7 TrunkPort, 7 GunTurret, 6 SolarPanel, 5 Pylon | Assault-specific load (fences, turrets, large reinforcement waves) for the `TARGET_ASSAULT_STRESSTEST` SKU |

`GameTypes none` matches no blueprint, so `GameMenu::AddMap` discards both maps and they cannot be chosen even in the SKUs that whitelist them (X4; [08](08_levels_and_level_design.md)); how those SKUs launched them is 32-U3.

### 8.6 Re-implementation guidance

Keep the idea (opt-in per-second telemetry: frame time, net in/out, latency, entity count, plus a hardware summary) but: query hardware in-process (DXGI adapter description, `GetSystemInfo`, `GlobalMemoryStatusEx`) instead of shelling out; record frame-time percentiles rather than an fps integer; upload over HTTPS with per-build tokens and explicit consent; and add a real deterministic benchmark scene (a recorded command stream replayed headless and rendered, §14).

## 9 Profiler

### 9.1 Mechanism [C] `contrib/systemiv/lib/profiler.h`, `contrib/systemiv/lib/profiler.cpp:Profiler`, `ProfiledElement`

- Global `Profiler *g_profiler = NULL`. `Profiler::Start()` creates it if absent; `Profiler::Stop()` deletes it. The only callers are `ProfileWindow::ProfileWindow` / `~ProfileWindow` (and a final `Profiler::Stop()` in `code/main.cpp:Finalise`) — profiling runs **only while the "Profiler" window exists**.
- `START_PROFILE(name)` / `END_PROFILE(name)` expand to `if (g_profiler) g_profiler->StartProfile/EndProfile(name)`; they compile to nothing only under `NO_PROFILER` (never defined) → the instrumentation is in the shipped exe at the cost of one pointer test per site.
- Data model: a tree of `ProfiledElement` rooted at "Root" (always expanded). Each element: `m_currentTotalTime`, `m_currentNumCalls` (accumulating this second), `m_lastTotalTime`, `m_lastNumCalls` (previous second), `m_historyTotalTime`, `m_historyNumSeconds`, `m_historyNumCalls` (since last reset), `m_shortest` (init `DBL_MAX`), `m_longest` (init `DBL_MIN`), `m_callStartTime`, `m_isExpanded` (default false), `m_wasExpanded`, `m_name` (strdup), `m_children` (`SortingHashTable<ProfiledElement*>`, keyed by name, alphabetical walk), `m_parent`.
- `StartProfile(name)`: find or create the child `name` of the current element; `AppReleaseAssert` that Root is still expanded; **only if the current element is expanded** start timing (`m_callStartTime = GetHighResTime()`, preceded by `glFinish()` when `m_doGlFinish` and inside a render section); descend. `EndProfile`: if the parent is expanded, optional `glFinish()`, then `End()` (calls + 1, add duration, update longest/shortest); ascend. Nesting is purely by call order; the name passed to `EndProfile` is only checked by debug asserts. Consequence: collapsed subtrees cost a hash lookup per call but are not timed; the tree must be expanded by clicking to measure deeper levels.
- `RenderStarted()` / `RenderEnded()` bracket `Renderer::Render` (`code/renderer.cpp`) so that "Toggle glFinish" attributes GPU time to the enclosing section.
- `Profiler::Advance()` — called once per frame from the location, editor and global-world loops (`code/main.cpp`) and the start sequence (`code/startsequence.cpp`), **not** from `MainMenuLoop`: when wall time passes `m_endOfSecond`, `m_lengthOfLastSecond = now − (m_endOfSecond − 1)`, `m_endOfSecond = now + 1`, and every element rolls current → last and adds last to history (`m_historyNumSeconds += 1`); the last frame time in ms is pushed to the front of `m_frameTimes` (kept to 200 entries).
- `ResetHistory()` clears history, longest and shortest recursively. `GetMaxChildTime()` = largest child history total ÷ the first child's history seconds. `ExpandAll()` and `LookupElement(name)` exist. `PROFILE_HISTORY_LENGTH` (10) is unused.
- Threading: no locking. On PC a `START_PROFILE` from a worker thread (for example the sound callback sections or loading-thread code) would corrupt the current-element pointer; Mac builds guard with `SINGLE_THREADED_PROFILER` [C]/[I].
- Leak: neither destructor frees children or names (each Start/Stop cycle leaks the tree) [C].

### 9.2 Output — the "Profiler" window [C] `code/interface/profilewindow.cpp:ProfileWindow`

- Opened by `DebugKeyBindings::ProfileButton()`: 570 × 600 at (ScreenW − 590, 30). `Create()` registers "Toggle glFinish" (10, 18), "Avg" (330, 18) and "Reset History" (190, 18), and **turns `m_doGlFinish` on** by default; "Yes"/"No" is drawn at (130, 28).
- Header "calls x avrg = total" right-aligned at x + 330. `RenderElementProfile` walks children alphabetically, skipping elements whose history average is 0; each row `"%*s%-*s:%5d x%4.1 = %4.0"` with indent icon '+'/'−' (expandable) or ' ', name padded to `24 − indent`, last-second calls, per-call ms and total ms — the two float conversions lack their `f` letter, so per-call and total print as undefined text (bug). Row brightness `105 + 150 × time / largest` (clamped 105–255). A bar starting at x + 340, length `√(ms) × 10` px, height 11, colour (150,150,250,brightness): outline when expanded, filled when collapsed. Clicking a row (`ControlEclipseLMousePressed` inside the row's 12-px band) toggles its expansion. The window grows by 12 px per row past its height.
- The "Avg" button cycles its caption Min → Avg → Max → Min and the code computes the matching column value, but it is never printed (unused variable) [C].
- Frame-time graph in the bottom 100 px: guide lines at 200 ms ("200ms per frame (5 fps)"), 100 ms ("(10 fps)") and 50 ms ("(20 fps)"); one red vertical line per frame for the last 200 frames, height `0.5 × (ms / 100) × 100` px (200 ms = full height), newest at the right.
- No file output, no capture, no network or remote view [C].

### 9.3 Overhead

| Situation | Cost per instrumented site | Evidence |
|---|---|---|
| Window closed (always in FULL) | one global pointer test | [C] macro |
| Window open, element under a collapsed parent | `SortingHashTable::GetData(name)` string-hash lookup on Start, pointer walk on End | [C] `Profiler::StartProfile` |
| Window open, element timed | lookup + two `GetHighResTime()` (QueryPerformanceCounter) | [C] |
| "Toggle glFinish" on (default when opened) | a full GPU pipeline drain (`glFinish`) at every Start and End inside `Renderer::Render` — frame rate collapses, numbers become per-section GPU+CPU time | [C] |
| Window drawing | the "render profile" section itself, one text line per visible element | [C] |

### 9.4 Other profilers

`SoundSystem` owns two private `Profiler` instances (event triggers, per-event PCM-fill cost measured inside the sound callback) and the software mixer a third (allocated only under `PROFILER_ENABLED`); their window is Sound Profile ([27 §13.1](27_audio_engine.md#131-always-compiled-profiling-bfull)). A memory line "Available Memory: %4u MB Physical, %4u MB Virtual. Sample Cache = %d MB (%d%% in use)" is written to the debug log every **60 s** in every build (`code/main.cpp:PrintAvailableMemory`, `AdvanceAvailableMemoryCheck`).

### 9.5 Every instrumented section

The developers' own instrumentation is the best available map of what they considered expensive. 186 `START_PROFILE` sites in compiled files [C] (generated by `_work/scripts/S_32_profile_sections.py`, run with `python3 -I` over the manifest's compiled set; "run-time name" = the section name is computed, giving one node per building or entity type). Nesting at run time follows call order: `Client Main Loop` (the whole network-advance step, which runs `Location::Advance`) and `Render` are the two big roots inside a match.

| File | Function → section names |
|---|---|
| `code/camera.cpp` | `Camera::Advance`: "Advance Camera" |
| `code/clouds.cpp` | `Clouds::Render`: "RenderSky", "RenderBlobby", "RenderFlat" |
| `code/deform.cpp` | `DeformEffect::Stop`: "Deform Pre-render", "Deform Apply" |
| `code/explosion.cpp` | `ExplosionManager::Advance`: "Advance Explosions"; `ExplosionManager::Render`: "Render Explosions" |
| `code/gamecursor.cpp` | `GameCursor::Render`: "Render GameCursor" |
| `code/global_internet.cpp` | `GlobalInternet::Render`: "Internet" |
| `code/global_world.cpp` | `SphereWorld::RenderSpirits`: "Spirits"; `SphereWorld::RenderWorldShape`: "Shape"; `SphereWorld::RenderHeaven`: "Heaven"; `SphereWorld::RenderIslands`: "Islands"; `GlobalWorld::Render`: "Render Global World" |
| `code/helpsystem.cpp` | `HelpSystem::Render`: "Render Helpsys" |
| `code/interface/profilewindow.cpp` | `ProfileWindow::Render`: "render profile" |
| `code/landscape_renderer.cpp` | `LandscapeRenderer::Render`: "Render Landscape Main", "Render Landscape Overlay" |
| `code/lib/bitmap.cpp` | `BitmapRGBA::ApplyBlurFilter`: "ApplyBlur" |
| `code/location.cpp` | `Location::AdvanceWeapons`: "Advance Lasers", "Advance Effects"; `Location::AdvanceBuildings`: "Advance Buildings", run-time name `Building::GetTypeName( building->m_type )`; `Location::AdvanceSpirits`: "Advance Spirits"; `Location::AdvanceClouds`: "Advance Clouds"; `Location::RenderSpirits`: "Render Spirits"; `Location::Render`: "Render Routing System", "RenderCurrentMessage"; `Location::RenderBuildings`: "Render Buildings", run-time name `Building::GetTypeName( building->m_type )`; `Location::RenderBuildingAlphas`: "Render Building Alphas", run-time name `Building::GetTypeName( building->m_type )`, "Depth Sort"; `Location::RenderClouds`: "Render Clouds"; `Location::RenderWeapons`: "Render Weapons" |
| `code/main.cpp` | `LocationGameLoop`: "Client Main Loop" |
| `code/markersystem.cpp` | `MarkerSystem::Render`: "RenderMarkerSystem" |
| `code/multiwinia.cpp` | `Multiwinia::Render`: "Multiawinia::Render" |
| `code/multiwiniahelp.cpp` | `MultiwiniaHelp::Render`: "Render MultiwiniaHelp" |
| `code/network/server.cpp` | `Server::Advance`: "Advance Server", "UpdateClients", "Authentication", "SEND", "Advertise" |
| `code/particle_system.cpp` | `ParticleSystem::Advance`: "Advance Particles"; `ParticleSystem::Render`: "Render Particles" |
| `code/renderer.cpp` | `Renderer::Render`: "Render"; `Renderer::RenderFrame`: "Render Clear", "GL Flip"; `Renderer::PreRenderPixelEffect`: "Pixel Pre-render", "blend old", "Draw pixelated", "Gen new texture"; `Renderer::ApplyPixelEffect`: "Pixel Apply", "pass 1", "pass 2", "pass 3", "pass 4"; `Renderer::MarkUsedCells`: "MarkUsedCells" |
| `code/sepulveda.cpp` | `Sepulveda::RenderHighlights`: "Highlights"; `Sepulveda::Render`: "Render Sepulveda" |
| `code/sound/sound_filter.cpp` | `DspResLowPass::Process`: "DspResLowPass"; `DspGargle::Process`: "DspGargle"; `DspEcho::Process`: "DspEcho" |
| `code/sound/sound_instance.cpp` | `SoundInstance::StartPlaying`: "StartPlaying", "OpenStream", "Set Parameters", "Set freq/vol/pos", "Setup DSP Effects" |
| `code/sound/sound_library_3d_dsound.cpp` | `SoundLibrary3dDirectSound::PopulateBuffer`: "LockBuf", "FillBuf", "UnlockBuf"; `SoundLibrary3dDirectSound::AdvanceChannel`: "GetPlayCursor", "PopulateBuf"; `SoundLibrary3dDirectSound::Advance`: "Commit" |
| `code/sound/soundsystem.cpp` | `SoundSystem::Advance`: "Advance SoundSystem", "Shutdown Music", "Propagate Blueprints", "Allocate Sorted Array", "Perceived Volumes", "Sort Samples", "Recalculate Priorities", "Find best Channel", "Stop Old Sound", "Start New Sound", "Reset Channel", "Advance All Channels", "Shutdown Sound", "UpdateListener", "SoundLibrary3d Advance" |
| `code/taskmanager_interface_gestures.cpp` | `TaskManagerInterfaceGestures::Render`: "Render Taskman"; `TaskManagerInterfaceGestures::RenderGestures`: "Render Gestures", "Create Bitmap", "ConvertToTexture", "Clear", "Draw lines", "Draw texture" |
| `code/taskmanager_interface_icons.cpp` | `TaskManagerInterfaceIcons::Render`: "Render Taskman"; `TaskManagerInterfaceIcons::RenderGestures`: "Render Gestures", "Create Bitmap", "ConvertToTexture", "Clear", "Draw lines", "Draw texture" |
| `code/taskmanager_interface_multiwinia.cpp` | `TaskManagerInterfaceMultiwinia::Render`: "Render Taskman" |
| `code/team.cpp` | `Team::Advance`: "Advance Unit Entities", "Advance Units", "Advance Others", run-time name `entityName`; `Team::Render`: "Render Units", run-time name `Entity::GetTypeName( unit->m_troopType )`, "Render Others", "Render Virii", "Render Darwinians", "Render Sync Errors"; `Team::RenderOthers`: run-time name `Entity::GetTypeName( entity->m_type )` |
| `code/unit.cpp` | `Unit::AdvanceEntities`: run-time name `Entity::GetTypeName( s->m_type )` |
| `code/user_input.cpp` | `UserInput::Advance`: "Advance UserInput"; `UserInput::Render`: "Render UserInput" |
| `code/water.cpp` | `Water::UpdateDynamicWater`: "LookupTables", "VertexHeights", "VertexBuffer"; `Water::Render`: "Render Water"; `Water::Advance`: "Advance Water" |
| `code/water_reflection.cpp` | `WaterReflectionEffect::PreRenderWaterReflection`: "Water Pre-render", "Render Clear" |
| `code/worldobject/airstrike.cpp` | `SpaceInvader::Advance`: "Update Pos" |
| `code/worldobject/building.cpp` | `Building::PushFromBuilding`: "PushFromBuilding"; `Building::RenderPorts`: "RenderPorts" |
| `code/worldobject/centipede.cpp` | `Centipede::SearchForSpirits`: "SearchSpirits" |
| `code/worldobject/darwinian.cpp` | `Darwinian::SearchForNewTask`: "SearchingForTask"; `Darwinian::AdvanceCombat`: "AdvanceCombat", "ThrowGrenade"; `Darwinian::AdvanceWorshipSpirit`: "AdvanceWorship"; `Darwinian::SearchForRandomPosition`: "SearchRandomPos"; `Darwinian::SearchForCrates`: "SearchCrate"; `Darwinian::SearchForFlags`: "SearchFlags"; `Darwinian::SearchForArmour`: "SearchArmour"; `Darwinian::SearchForOfficers`: "SearchOfficers"; `Darwinian::SearchForShamans`: "SearchShaman"; `Darwinian::SearchForSpirits`: "SearchSpirits"; `Darwinian::SearchForThreats`: "SearchThreats", "FindThreatObjects", "Trees", "AntsNests", "AITargets", "GetEnemies"; `Darwinian::SearchForPorts`: "SearchPorts"; `Darwinian::SearchForCarryableBuilding`: "SearchCarryable"; `Darwinian::AdvanceToTargetPosition`: "AdvanceToTargetPos"; `Darwinian::PushFromObstructions`: "PushFromWater", "PushFromBuildings" |
| `code/worldobject/feedingtube.cpp` | `FeedingTube::RenderSignal`: "Signal" |
| `code/worldobject/flag.cpp` | `Flag::Render`: "RenderFlag" |
| `code/worldobject/radardish.cpp` | `RadarDish::RenderSignal`: "Signal" |
| `code/worldobject/spawnpoint.cpp` | `SpawnPoint::Advance`: "Evaluate", "SpawnDarwinians" |
| `code/worldobject/spider.cpp` | `Spider::SearchForSpirits`: "SearchSpirits" |
| `code/worldobject/spirit.cpp` | `Spirit::PushFromBuildings`: "PushFromBuildings" |
| `code/worldobject/spiritstore.cpp` | `SpiritStore::Render`: "Spirit Store" |
| `code/worldobject/triffid.cpp` | `Triffid::Advance`: "CheckTrigger" |
| `code/worldobject/tripod.cpp` | `Tripod::FindEntityToAttack`: "FindEntityToA"; `Tripod::ChooseDestination`: "ChooseDest"; `Tripod::DoNavigation`: "DoNav"; `Tripod::AdvanceWalk`: "AdvanceWalk"; `Tripod::AdvancePreAttack`: "AdvancePreAttack"; `Tripod::AdvanceAttack`: "AdvanceAttack" |
| `code/worldobject/trunkport.cpp` | `TrunkPort::Render`: "RenderDestination"; `TrunkPort::RenderAlphas`: "Advance Heightmap", "Render Heightmap" |
| `code/worldobject/virii.cpp` | `Virii::RecordHistoryPosition`: "RecordHistory"; `Virii::AdvanceToTargetPos`: "AdvanceToTargetPos"; `Virii::AdvanceIdle`: "AdvanceIdle"; `Virii::AdvanceAttacking`: "AdvanceAttacking"; `Virii::AdvanceToSpirit`: "AdvanceToSpirit"; `Virii::AdvanceToEgg`: "AdvanceToEgg"; `Virii::SearchForEnemies`: "SearchForEnemies"; `Virii::SearchForSpirits`: "SearchForSpirits"; `Virii::SearchForEggs`: "SearchForEggs"; `Virii::SearchForIdleDirection`: "SearchForIdleDir" |

Tags: sections in `code/deform.cpp` and `code/water_reflection.cpp` exist only in [B:D3D]; `code/global_world.cpp`, `code/global_internet.cpp`, `code/sepulveda.cpp`, `code/helpsystem.cpp`, `code/taskmanager_interface_gestures.cpp`, `code/taskmanager_interface_icons.cpp`, `Darwinian::AdvanceWorshipSpirit`, `code/worldobject/spiritstore.cpp`, `code/worldobject/tripod.cpp` are [B:LEGACY] or unreachable in Multiwinia; the pixel-effect sections never run in Multiwinia ([24](24_rendering.md)); the three `code/water.cpp:Water::UpdateDynamicWater` sections use the GL-only wrapper `START_PROFILE_NO_DIRECT3D`.

## 10 Logging, asserts, crash and sync logs

### 10.1 Debug output (`AppDebugOut`) and `debug.txt` [C] `contrib/systemiv/lib/debug_utils.cpp`, `code/main.cpp:Initialise`

| Function | Behaviour |
|---|---|
| `AppDebugLogPathPrefix(dir)` | stores the prefix used by `AppDebugLogPath`; `Initialise` sets it to the profile directory (`%LOCALAPPDATA%\Introversion\Multiwinia\` on Windows 2000+, creating it), or "" with `--NoProfile`; in TARGET_DEBUG the profile directory is "" (working directory) |
| `AppDebugLogPath(file)` | Win32: prefix + file; Mac `~/Library/Logs/<bundle name>/file`; Linux `/tmp/file` |
| `AppDebugOutRedirect(file)` | if different from the current target: set the target to `AppDebugLogPath(file)` and truncate it; on failure drop the redirect. With `DUMP_DEBUG_LOG` (FULL) `Initialise` redirects to `debug.txt` before anything else and immediately writes `"<APP_NAME> <APP_VERSION> built <__DATE__>"` |
| `AppDebugOut(fmt, …)` | `vsprintf` into a 10240-byte stack buffer (no bound check); with a redirect: lock a global `NetMutex`, open the file in append mode, write, **close** (one open/close per call); without: `OutputDebugString`. On open failure the redirect is dropped and the message goes to `OutputDebugString` (the follow-up "Failed to open %s" prints a NULL name). Compiled out only with `NO_DEBUG_OUTPUT` (not on PC) |
| `AppDebugOutData(ptr, len)` | hex dump 16 bytes per line (`int*` overload: decimal) |

Volume: 656 `AppDebugOut(` call sites across `code/`, `contrib/systemiv/lib` and `contrib/eclipse` [C grep]. Recurring content in FULL: start-up banner, the 60-s memory line (§9.4), network connection, sync and disconnect events ("CLIENT: Server informed us that client %d is out of sync at seq id %d", …), every FTP block event (`FTP_MANAGER_VERBOSE_DEBUG` is defined), sound and resource loading notes, and — while the Dr S face cheat runs — one line per frame. Because every call re-opens the file, high-frequency logging is itself a performance hazard (§12.3).

Input-preference parse errors go to `inputprefs_debug.txt` in the profile directory only on Mac and TARGET_DEBUG; in FULL they go to `cout`, which a Windows GUI process discards [C] `code/lib/input/input.cpp:InputManager::parseInputPrefs`.

### 10.2 Asserts and the black box [C] `contrib/systemiv/lib/debug_utils.h`, `contrib/systemiv/lib/debug_utils.cpp`; summary also in [31 §6.5](31_platform_build_and_tools.md#65-asserts-and-the-black-box-c-contribsystemivlibdebug_utilsh-contribsystemivlibdebug_utilscpp)

| Macro / function | Release (FULL) behaviour |
|---|---|
| `AppDebugAssert(cond)` | `ShowCursor(true); _ASSERT(cond)` — **no effect** without `_DEBUG` |
| `AppReleaseAssert(cond, fmt, …)` | if false → `AppReleaseAssertFailed`: format into 512 bytes, `MessageBox(NULL, msg, "Fatal Error")`, `AppGenerateBlackBox("blackbox.txt", msg)`, then `throw;` with no active exception (process termination through the SEH / terminate path; whether the crash handler of §10.3 also runs is 32-U1) |
| `AppAssert(x)` | `AppReleaseAssert(x, "Assertion failed : '%s'\n\n%s\nline number %d", #x, __FILE__, __LINE__)` — **active in release** |
| `AppAssertArgs(x, fmt, …)` | as above plus formatted text; used by every `DArray` / `FastDArray` / `BoundedArray` bounds check, so an out-of-range container access is a fatal, logged error in the shipped game |
| `AppAbort(msg)` | `AppReleaseAssert(false, "Abort : '%s'\n\n%s\nline number %d", …)` |
| `AppReleaseExit(fmt, …)` | log, `MessageBox(…, "Exit")`, black box, `exit(-1)` |
| `AppGenerateBlackBox(file, msg)` | writes `<profile>blackbox.txt`: a banner "BLACK BOX REPORT", `"<APP_NAME> <APP_VERSION> built <date>"`, `"Date H:M, D/M/YYYY"`, `ERROR : '<msg>'`, then "STACKTRACE" by walking saved-EBP frames (`retAddress = %p` per frame) until a frame pointer is misaligned, not increasing or unreadable |

The black-box trace relies on EBP frame chains; `Release|Win32` is built with `/O2`, which on x86 includes frame-pointer omission, so the trace is probably short or wrong [I] (32-U5); the minidump of §10.3 is the reliable artefact.

### 10.3 Crash logs [C] `code/main.cpp:AppMain`, `code/lib/exception_handler.cpp:RecordExceptionInfo`, `contrib/XCrashReport/CrashFileNames.h`; full specification in [31 §6](31_platform_build_and_tools.md#6-crash-handling-and-system-information)

`AppMain` wraps `RunTheGame` in `__try / __except(RecordExceptionInfo(GetExceptionInformation(), profileDir, "AppMain", "0B9432AF", DARWINIA_VERSION))` (main thread only). On the first exception (re-entry returns `EXCEPTION_CONTINUE_SEARCH`):
1. `<profile>ERRORLOG.TXT` (created with `CREATE_ALWAYS`, so overwritten each crash): "<exe> caused <exception description> (0x%08x) in module <module> at <CS>:<EIP>", "Exception handler called in AppMain.", "CRASH REFERENCE CODE: <hex of the low 32 bits of the current FILETIME>", time, user, OS, processor count/type, memory load and sizes, access-violation read/write address, registers, 16 code bytes at EIP, up to 3072 stack DWORDs in 4 columns, and the module list with version information.
2. `<profile>CRASH.DMP` — `MiniDumpWriteDump` with `MiniDumpWithIndirectlyReferencedMemory`.
3. If no debugger is attached: launch `MultiwiniaSupport.exe` from the exe directory with the exe name, crash id, magic "0B9432AF", version and the profile directory; it gathers `dxdiag_info.txt`, zips the listed files into `multiwinia.zip` and uploads only when the user presses Send ([31 §6.4](31_platform_build_and_tools.md#64-support-tool--multiwiniasupportexe-c-contribxcrashreportxcrashreportcppcappinitinstance-contribxcrashreportdlgcpp-contribxcrashreportsendemailcppsendemail-contribxcrashreportsysteminfogateringcpp-multiwiniasupportini)). If the tool starts, the standard Windows crash dialog is suppressed.

### 10.4 Sync logs [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessServerLetters`, `ClientToServer::GetSyncFileName`, `code/location.cpp:Location::DumpWorldToSyncLog`, `code/network/iframe.cpp`

| File | Written when | Content | Build |
|---|---|---|---|
| `<profile>syncerror-<YYYY-MM-DD--HH-MM-SS>-client<id>-team<team>.txt` (date = **server start time**, `m_serverStartTime`) | the first NETSYNCERROR this client receives | [B:DEBUG] only: first the sync-RNG call log (`DumpSyncRandLog`, §13.3, truncating the file); in **every** build: "Begin Final Object Positions" and one `LogState()` line per unit entity, other entity, effect (except snow) and building (appended) | **FULL** (+ DEBUG extras) |
| `<profile>syncdiff-<same pattern>.txt` | after a remote IFrame arrives and differs | `SyncDiff::Print` lines `syncdiff: id: (team: T, unit: U, index: I, uniqueId: N) (x, y, z)  - name - description` | effectively DEBUG / beta only (no snapshot exists in FULL) |

`LogState` formats (positions as `HashDouble` hex — 16 lowercase nibbles, least significant first; velocities and fronts as the hash of x + y + z) [C] `code/worldobject/worldobject.cpp:WorldObject::LogState` and the overrides:

| Class | Line |
|---|---|
| `WorldObject` | `WOBJ Type[<int>] Id[<uid>] pos[x,y,z], vel[Σ]` |
| `Entity` | `ENT Type[<name>] Id[<uid>] pos[x,y,z], vel[Σ] front[Σ]` |
| `Darwinian` | `DARWINIAN Id[] state[] pos[x,y,z] vel[Σ] front[Σ] waypoint[x,y,z] boxId[]` |
| `Officer` | `ENT OFFICER Id[] state[] pos[x,y,z] vel[Σ] front[Σ] waypoint[Σ]` |
| `BoxKite` | `BOXKITE Id[] state[] pos[Σ], vel[Σ]` |
| `Building` | `BUILDING Type[<name>] Id[] pos[x,y,z], vel[Σ] front[Σ] centre[Σ], radius[]` |
| `Crate` | `CRATE Id[] pos[x,y,z], vel[Σ] radius[]` |
| `Tree` | `TREE Id[] pos[x,y,z], vel[Σ] front[Σ] centre[Σ] radius[] height[] hitcheckCentre[Σ] hitcheckRadius[]` |

Each `LogState` returns a 1024-byte static buffer (not re-entrant).

### 10.5 Every diagnostic file the game writes

`<P>` = profile directory (`%LOCALAPPDATA%\Introversion\Multiwinia\`; working directory under `--NoProfile` or TARGET_DEBUG); `./` = working directory.

| File | Writer | Content | Build |
|---|---|---|---|
| `<P>debug.txt` | `AppDebugOut` | debug log (§10.1), truncated at start | [B:FULL] |
| `<P>blackbox.txt` | `AppGenerateBlackBox` | fatal assert report (§10.2) | [B:FULL] |
| `<P>ERRORLOG.TXT`, `<P>CRASH.DMP` | `RecordExceptionInfo` | crash report and minidump (§10.3) | [B:FULL] |
| `<P>dxdiag_info.txt`, `<P>multiwinia.zip` | `MultiwiniaSupport.exe` | support bundle | [B:FULL] (separate exe) |
| `<P>syncerror-…txt` | §10.4 | desync world dump (+ RNG log in DEBUG) | [B:FULL] |
| `<P>syncdiff-…txt` | `IFrame` diff dump | object-level desync differences | [B:DEBUG]/beta |
| `./language_errors.txt` | `LangTable::TestAgainstEnglish` | localisation QA (pref `TextLanguageTest`) | [B:FULL], pref-gated |
| `./serverHistory.dat` | `Server::SaveHistory` | command-stream recording (pref `RecordDemo` 1) | [B:FULL], pref-gated |
| `./version` | `AppMain` with `--write-version` | version string | [B:FULL] |
| `<P>inputprefs_debug.txt` | `InputManager::parseInputPrefs` | key-binding parse errors | [B:DEBUG] / Mac |
| `./TestBedLog.txt` | `App::TestBedLogWrite` | one CSV row per testbed game (§6.6) | [B:DEAD] |
| `./test_log.txt` | `TestHarness` | campaign exploration (§7.1) | [B:DEAD] |
| `<P>benchmarks/game <key> <time>.txt`, `<P>dxdiag.txt` | `BenchMark` | telemetry (§8) | [B:DEAD] here |
| `leaks.txt` (+ `leaks.txt.tmp`) | `AppPrintMemoryLeaks` on quit | leak summary (§10.6) | [B:DEAD] |

### 10.6 Memory-leak tracking (`TRACK_MEMORY_LEAKS`) [B:DEAD] [C] `contrib/systemiv/lib/memory_leak.h`, `contrib/systemiv/lib/memory_leak.cpp`, `contrib/systemiv/lib/string_utils.h`, `code/main.cpp:HandleCommonConditions`

With the macro, `memory_leak.h` is included from `universal_include.h` and redefines `new` as `new(_NORMAL_BLOCK, __FILE__, __LINE__)` (CRT debug heap); `newStr` gains file/line tracking. On quit `AppPrintMemoryLeaks("leaks.txt")` sends `_CrtDumpMemoryLeaks` output to `leaks.txt.tmp`, parses it (`ParseMemoryLeakFile`) and accumulates bytes and counts per lower-cased source location in `BTree<int>`s, writing "Total recognised memory leaks   : %d Kbytes", "Total unrecognised memory leaks : %d Kbytes", then one line per location sorted by size, `"%-95s (%d Kbytes in %d leaks)"` above 2048 bytes or `"%-95s (%d  bytes in %d leaks)"`. `memory_leak.cpp` is compiled in the baseline but inert without the macro. `docs/todo.txt` lists "memory leaks" and "D3D resource leaks" as open issues [C].

## 11 Debug renderers and visual debug aids

### 11.1 Primitives (`DEBUG_RENDER_ENABLED`, defined globally → compiled into FULL) [C] `code/lib/debug_render.cpp`, `code/lib/persisting_debug_render.cpp`, `code/lib/sphere_renderer.cpp`; also [24 §4.23](24_rendering.md#423-debug-renderers-and-utilities-c)

| Function | Draws |
|---|---|
| `RenderSquare2d` | 2-D square in its own ortho projection, depth test off |
| `RenderCube` | line-loop cube, line smoothing, width 2 |
| `RenderSphereRings` | three orthogonal 20-segment circles |
| `RenderSphere` / `RenderSphereFilled` | wireframe / filled icosphere (80 triangles, one display list) |
| `RenderVerticalCylinder` | 16-edge cylinder, lighting off then on |
| `RenderArrow` | line of width `width / distToCamera × arrowLength`, two 20 % arrow-head lines |
| `RenderPointMarker` | arrow from (+20, +20, +20) to the point plus 3-D text size 3 |
| `PrintMatrix` / `PrintMatrices` | matrix to the debug log, at most 10 times |
| `g_debugRenderer` (`PersistingDebugRenderer`) | queue of `Square2d`, `PointMarker`, `Sphere`, `Vector` items (`FastDArray`, initial 100), keyed by a formatted label ≤ 63 characters (case-insensitive prefix match), `life` in **frames** (0 = forever; decremented per render, removed at 1); `Sphere`/`Vector` never store their label, so find-by-label can match stale items; rendered every frame at pass 24 of [24 §3.2](24_rendering.md#32-ordered-pass-table--one-multiwinia-match-frame-bfull) and normally empty (no code queues items) |

### 11.2 Where they are drawn, and whether a FULL player can see it [C] call sites as listed

| Visual | Condition | Visible in FULL? | Ref |
|---|---|---|---|
| Sync-difference markers: 500-unit arrow (width 20) down to each differing object + two lines of 3-D text (name, description), size 5 | a received IFrame differs from the local one | **no** (no IFrame snapshots in FULL) | `code/network/syncdiff.cpp:SyncDiff::Render`, `code/team.cpp:Team::RenderSyncErrors` |
| AI priority text and link arrows | pref `RenderAIInfo` | **yes, if the pref is set** (§5.8) | `code/worldobject/ai.cpp:AITarget::Render`, `AITarget::RenderAlphas` |
| AISpawnPoint sphere + "Spawn %d %s's / Every %d seconds / Next spawn in %d seconds" (its editing check is commented out) | always when the building exists | no — `AISpawnPoint` is a blocked Darwinia object and occurs only in `data/levels/old/` and campaign maps | `code/worldobject/ai.cpp:AISpawnPoint::RenderAlphas` |
| Building front arrows (Wall, PulseBomb, base `Building`), PylonStart / TrackStart requirement arrows, power-splitter links, radar-dish links, SafeArea / MultiwiniaZone / RestrictionZone / JumpPad / EruptionMarker / SmokeMarker / ScriptTrigger / StaticShape / GenericHub / Portal / EscapeRocket / SpawnPopulationLock spheres | `App::m_editing` | no (editor builds only) | the respective `Render` / `RenderAlphas` |
| Shape hit-check spheres/cylinders, shape markers, tree hit-check spheres | callers commented out (only `code/location_editor.cpp` calls `RenderHitCheck`) | no | `code/lib/shape.cpp`, `code/worldobject/tree.cpp:Tree::RenderHitCheck`, `code/worldobject/building.cpp:Building::RenderHitCheck` |
| Fence-switch and track-junction link arrows (`RenderLink`) | called from the editor only | no | `code/worldobject/switch.cpp:FenceSwitch::RenderLink`, `code/worldobject/mine.cpp:TrackJunction::RenderLink` |
| Route-node spheres and labels (`RoutingSystem::RenderNodes`) | call commented out | no | `code/routing_system.cpp` |
| Entity-grid / obstruction-grid / grid-cache renders | debug functions, `EntityGridCache::Render` call commented out; `DEBUG_ENTITY_GRID` off | no | `code/entity_grid.cpp` |
| FPS / entity counter | `m_displayFPS` (forced off) | no | §3.5 |
| Red "sync error" banner, "lost connection" banner | any client out of sync / > 5 s without server letters | **yes** (player-facing, not a debug aid) | `code/renderer.cpp:Renderer::RenderFrame` ([24](24_rendering.md)) |

Commented-out debug draws worth knowing because they show what the developers inspected: tank and armour waypoint/attack-target arrows, air-strike approach/exit arrows, ant-hill health and occupancy text with objective spheres (`code/worldobject/tank.cpp`, `code/worldobject/armour.cpp`, `code/worldobject/airstrike.cpp`, `code/worldobject/anthill.cpp`) [C].

## 12 Known performance hotspots and budgets

### 12.1 Cross-check of `docs/cpu cycles.txt`

The file is 4932 bytes of gameplay design with sections "CPU CYLES", "TASK MANAGER", "TERMINATION", "GESTURES", "NUMBER KEYS", "ZEPELLINS": programs consume a pool of "processor cycles" that capturing Processor buildings increases; ALT-TAB task switching; CTRL-BREAK termination freeing cycles at "1 every 2 seconds"; air-strike strength proportional to cycles; gesture launching; "Debug Zeppelins" that display information [C] `docs/cpu cycles.txt`. It contains **no measurement, budget or profiling note**, and none of these mechanics exists in Multiwinia's task manager ([06](06_player_control_and_input.md)). Every hotspot below is therefore established from code (instrumentation, caps, loop structure) and from the change logs / release notes in `docs/`, each cited.

No measured frame-time or per-system cost exists anywhere in the tree (32-U2): the numbers below are budgets and limits **as coded**, and claims **as written** by the developers.

### 12.2 Frame and tick structure — the time budget as coded [C]

| Quantity | Value | Ref |
|---|---|---|
| Server tick | 10 Hz: `m_nextServerAdvanceTime += 0.1` per `Server::Advance` | `code/network/server.cpp:Server::AdvanceIfNecessary` |
| Client "I am alive" (team controls) send period | `IAMALIVE_PERIOD` 0.1 s | `code/globals.h`, `code/main.cpp:LocationGameLoop` |
| Slices per simulation frame | `NUM_SLICES_PER_FRAME` 10 — each tick's heavy work is split into 10 slices so a render frame runs only part of a tick | `code/globals.h` |
| Slices advanced per render frame (normal) | `clamp(int(0.3 × pending), 0, 20)` with `pending = (lastValidSeq − lastProcessedSeq) × 10 + (10 − g_sliceNum)`; the intended time-based branch for `pending ≤ 10` declares a shadowing local and is discarded, so 1–3 pending slices advance **0** per frame | `code/network/clienttoserver.cpp:ClientToServer::GetNumSlicesToAdvance` ([22 §2.3](22_determinism_and_networking.md#23-slices-per-render-frame-client-pacing-controller)) |
| Catch-up | when more than 20 sequence ids behind: 20 slices (2 ticks) per iteration, repeated until caught up or **0.3 s** of wall time has been spent in this render frame | `code/network/clienttoserver.cpp:ClientToServer::AdvanceAndCatchUp` |
| Render-side delta clamp | `g_advanceTime ≤ 0.25 s` (camera, UI, sound, particles' render-side use) | `code/main.cpp:UpdateAdvanceTime` |
| Slow-frame reaction | if `Renderer::m_fps < 15` the sound system is advanced a second time after rendering (keeps the 20 Hz audio tick fed) | `code/main.cpp:LocationGameLoop` |
| Per-frame legacy work | `GlobalWorld::EvaluateEvents()` every render frame in a Multiwinia match ("DELETEME: for debug purposes only") | `code/main.cpp:LocationGameLoop` |
| Main-thread GL jobs | `g_loadingScreen->RunJobs()` each frame compiles queued display lists | `code/main.cpp:LocationGameLoop` |
| Memory report | every 60 s (`GlobalMemoryStatus` + sample-cache usage) to the debug log | `code/main.cpp:AdvanceAvailableMemoryCheck` |
| Thread placement | main thread affinity mask `0x02 \| 0x08 \| 0x20 \| 0x80` (CPUs 1, 3, 5, 7); all simulation, rendering and network sending on that thread; level loading on a `WorkQueue` worker | `code/app.cpp:App::App`, [20](20_architecture.md) |
| Profiler graph reference lines | 50 / 100 / 200 ms per frame (20 / 10 / 5 fps) — the range the developers expected to inspect | `code/interface/profilewindow.cpp:ProfileWindow::RenderFrameTimes` |
| Sound CPU warning (dead code) | "Advance SoundSystem" share drawn red above 15 % | `code/interface/prefs_sound_window.cpp` ([27 §13.1](27_audio_engine.md#131-always-compiled-profiling-bfull)) |

Design consequence: the simulation cost of one tick must fit, amortised over 10 slices, inside the render frames of 0.1 s; when it does not, the client falls behind, the catch-up loop spends up to 0.3 s per frame simulating, and the frame rate drops further (a spiral bounded only by the 0.3 s cap) [I from the code above].

### 12.3 Hotspot catalogue

Evidence column: *instrumented* = has its own `START_PROFILE` node (§9.5); *claim* = a developer note in `docs/`.

| # | Hotspot | Mechanism and numbers | Evidence |
|---|---|---|---|
| H1 | **Entity count** (Darwinians dominate) | No global cap: the generic cap in `Location::SpawnEntities` is commented out. Shipped maps cap population per map (`PopulationCap`): 1400 (1 map), 1500 (9), 1600 (2), 1800 (15), 2000 (21); per team = cap / level `NumPlayers` = 350–1000; two co-op maps and the stress map are **uncapped**, `mp_sandbox` 2500 ([05 §4.6](05_economy_and_population.md#46-per-map-table-every-mp_-level-file-in-datalevels)). Reinforcements and crates can overshoot ([05 §4.4](05_economy_and_population.md#44-overshoot)). `Entity::s_entityPopulation` counts live entities. | [C] `code/location.cpp:Location::SpawnEntities`; [C] data; instrumented ("Advance Unit Entities", "Advance Others", per-type nodes); claim: Darwinia `docs/changes.txt` "Optimisations made to Darwinian combat code. Should help during big battles involving lots of Darwinians"; Multiwinia beta `docs/multiwiniachanges-beta.txt` "ASLT: Sudden Frame Rate Drop (Problem with lots of _stuck_ Darwinians?)"; `docs/betachanges.txt` "The population limit on Doomsday has been lowered to help with performance issues" |
| H2 | **Darwinian AI searches** | Each Darwinian re-searches every `1.0 + syncfrand(1.0)` s (1–2 s, staggered): crates, flags, armour, officers, shamans, spirits, ports, carryables, random positions, and `SearchForThreats` (threat objects, trees, ant nests, AITargets, `GetEnemies`); per-tick movement (`AdvanceToTargetPosition`) and `PushFromObstructions` (water, buildings). At 2000 Darwinians ≈ 1000–2000 full searches per second | [C] `code/worldobject/darwinian.cpp:Darwinian::SearchForNewTask` and the `Search*` functions; instrumented (16 nodes) |
| H3 | **Spatial grid queries** | `EntityGrid(32.0, 32.0)` cells; `ObstructionGrid` 32 (multiplayer) / 64 (single-player) cells; `EntityGridCache` re-uses recent query results (staleness and range shortcuts change results — [21](21_simulation_and_entity_model.md), X3 U6); neighbour results go to shared static arrays (`s_neighbours`) and `std::vector`s | [C] `code/location.cpp:Location::Init`, `code/entity_grid.cpp`; instrumented ("GetEnemies", "FindThreatObjects") |
| H4 | **Spirits (souls)** | `Location::m_spirits` is a `FastDArray<Spirit>` stored by value, initial 100, step 100, sliced; every death adds one; harvesting is the only sink | [C] `code/location.cpp:Location::Location`; instrumented ("Advance Spirits", "Render Spirits", `Spirit::PushFromBuildings`); claim: `docs/changes.txt` "Spirit code heavily optimised. Was causing serious slowdown after large battles due to large numbers of spirits floating around" |
| H5 | **Lasers and effects** | `m_lasers`, `m_effects` step 100; every Darwinian shot is a laser object | [C] `code/location.cpp:Location::Location`; instrumented ("Advance Lasers", "Advance Effects", "Render Weapons"); claim: `docs/changes.txt` "Laser code optimised" |
| H6 | **Particles** | array 1500 initial, +200 growth, **no cap**; advanced all at once in slice 0 of every tick (the per-slice loop is commented out); each `CreateParticle` is refused only while the game is paused; tree fire particle count `/= RenderBuildingDetail` | [C] `code/particle_system.cpp:ParticleSystem::ParticleSystem`, `ParticleSystem::Advance`, `ParticleSystem::CreateParticle`; instrumented; claim: `docs/newdemo.txt` "Lag in final fade to white due to particle density" |
| H7 | **Explosions (shape shattering)** | up to about 501 source triangles scanned per fragment, perimeter < 6 skipped ([26](26_animation_and_vfx.md)) | [C] `code/explosion.cpp`; instrumented |
| H8 | **Draw calls / immediate mode** | every Darwinian is drawn by its own `Render` call (immediate mode), with a high-detail factor `1 − d²/D²` where D = 99999.9 / 1000 / 500 for `RenderEntityDetail` 1 / 2 / 3; buildings per object (`Shape::Render` → display list or recursive fragments); alpha buildings depth-sorted with `qsort` every frame | [C] `code/team.cpp:Team::RenderDarwinians`, `code/lib/shape.cpp:Shape::Render`, `code/location.cpp:Location::RenderBuildingAlphas`; instrumented ("Render Darwinians", "Depth Sort", per-type nodes) |
| H9 | **Display lists** | non-animating shapes, trees, sphere renderer and the landscape (display-list mode) compile GL display lists; compilation is marshalled to the main thread through the loading-screen job queue; `Resource::FlushOpenGlState` / `RegenerateOpenGlState` rebuild everything (resolution change, cheat 16); tree geometry and the tree hit-check radius are generated inside the display-list build | [C] `code/lib/resource.cpp`, `code/worldobject/tree.cpp:Tree::Generate`; P07 note; [24 §4.5](24_rendering.md#45-shape-rendering-shp-meshes-c-codelibshapeh-codelibshapecpp) |
| H10 | **Landscape** | full triangle-strip mesh, **no LOD**, two passes (main + wireframe overlay); `RenderLandscapeMode` −1 → VBO when `GL_ARB_vertex_buffer_object` is supported, else display list (D3D: always VBO); `RenderLandscapeDetail` is read but replaced by the literal 1 | [C] `code/landscape_renderer.cpp:LandscapeRenderer::LandscapeRenderer`; instrumented ("Render Landscape Main", "Render Landscape Overlay"); claim: `docs/changes.txt` VBOs "should be faster in most cases than Display Lists" ([23](23_terrain_and_landscape.md)) |
| H11 | **Dynamic water** | wave mesh rebuilt every frame on the CPU (lookup tables, vertex heights, vertex buffer); `RenderWaterDetail` 1–3 sets mesh resolution | [C] `code/water.cpp:Water::UpdateDynamicWater`; instrumented (3 GL-only nodes, "Render Water" ×3, "Advance Water") |
| H12 | **Buildings** | per-type profile node around each building's `Advance` and `Render`; decorative blob/star counts by `RenderBuildingDetail` (e.g. spawn point 20/10/3 blobs) | [C] `code/location.cpp:Location::AdvanceBuildings`, `Location::RenderBuildings`; [24](24_rendering.md) |
| H13 | **Virii and other creatures** | history-position recording per Virii, per-state search nodes | [C] `code/worldobject/virii.cpp`; instrumented (10 nodes) |
| H14 | **AI placement loops** | several AI task handlers loop `while (!IsValidTargetArea(task, pos))` drawing new positions with `syncsfrand(area radius)` around an area centre, with no iteration limit — deterministic, but a frame hitch (or a hang on every peer) when an area has few or no valid points | [C] `code/worldobject/ai.cpp` (task-target helpers) ([07](07_ai.md)) |
| H15 | **Sound** | 20 Hz tick with sort/priority passes; doubled when fps < 15 | [C] [27 §3](27_audio_engine.md#3-update-rate-the-20-hz-tick); instrumented (15 nodes) |
| H16 | **Debug-log I/O** | `AppDebugOut` opens and closes `debug.txt` on every call under a mutex; FTP verbose logging is on; the Dr S face writes one line per frame | [C] `contrib/systemiv/lib/debug_utils.cpp:AppDebugOut`, `code/network/ftp_manager.cpp`, `code/clouds.cpp:Clouds::RenderFace` |
| H17 | **Network** | no bandwidth limiting or adaptation [C]; letters larger than 1500 bytes are probably dropped on receive and resent indefinitely [I] ([22](22_determinism_and_networking.md)); server sections "Advance Server", "UpdateClients", "Authentication", "SEND", "Advertise" | [C] `code/network/bandwidth.cpp`; instrumented |
| H18 | **Legacy per-frame work in Multiwinia** | `GlobalWorld::EvaluateEvents()` each frame; task-manager gesture/icon interfaces would build a bitmap and texture per frame ("Create Bitmap", "ConvertToTexture") but are unreachable in Multiwinia | [C] `code/main.cpp:LocationGameLoop`, `code/taskmanager_interface_icons.cpp` |
| H19 | **Profiler with glFinish** | opening the profiler turns on `glFinish` at every section boundary inside rendering | [C] §9.3 |

Hardware target claimed by the publisher's EULA: Windows XP / Vista, 2.0 GHz CPU, GeForce 6200 or Radeon 9600, 512 MB RAM, 120 MB disk [C] `docs/IntroversionEULA.txt`.

### 12.4 Budget guidance for the new game [I]

| Area | Recommendation |
|---|---|
| Simulation tick | Keep a fixed simulation tick (10 Hz was the original; 20–30 Hz recommended), measured in **ticks** not wall time, with a hard per-tick CPU budget (e.g. ≤ 25 % of a 16.7 ms frame at the population cap on minimum-spec hardware) and per-system budgets taken from H1–H6 |
| Population | Design for the shipped peak (2000 units on a map plus spirits and lasers, H1, H4, H5) and test at 2× with an uncapped stress map like `mp_benchmark_1` (4 × 150 start units, 24 spawn points); make the cap a hard engine limit, not only a map option |
| Searches and grids | Keep the staggered 1–2 s re-search (H2) but make spatial queries exact (no cache shortcuts that change results) and data-oriented (SoA, SIMD over cell lists); budget queries per tick |
| Particles and VFX | Fixed-capacity pools with a hard cap and graceful LOD; keep VFX entirely outside the deterministic simulation |
| Rendering | Instanced rendering for units (one draw per unit type per pass), GPU-driven terrain LOD, compute-shader water; no immediate mode, no display lists; a draw-call budget per frame |
| Catch-up | Bound catch-up by ticks per frame and show a "catching up" state instead of letting the frame rate collapse (12.2) |
| Logging | Asynchronous, buffered logging thread; per-category levels; never open/close a file per line |

## 13 Desync debugging tooling

Full network and determinism specification: [22](22_determinism_and_networking.md) (IFrames: [22 §7](22_determinism_and_networking.md#7-iframes)). This section lists the **tools**.

### 13.1 Detection in each build [C] `code/network/iframe.cpp:IFrame::Generate`, `code/network/server.cpp:Server::HandleSyncMessage`

| Build | Per-frame sync value | What it detects |
|---|---|---|
| [B:FULL] | `(unsigned char)(255 × syncfrand(1))` — the next value of the synced RNG; no snapshot is kept (`m_data = NULL`) | only divergence in the **number or order of synced-RNG draws**; world-state drift that does not change RNG consumption goes unnoticed |
| [B:DEBUG], `TARGET_BETATEST_GROUP` | low 8 bits of word 0 of SHA-1 over a serialised world snapshot (teams → units → entities pos/vel/front/uniqueId, others, lasers, effects, buildings incl. radius/front/up/centre, network time, a final `syncfrand(255)`); newest 50 IFrames kept | any position/velocity/orientation divergence of listed objects |

Both variants draw exactly one synced-RNG value per frame, so the RNG stream stays aligned within a build type; DEBUG and FULL peers cannot be mixed anyway (`INCLUDE_THEBEAST`, tracked-`syncsfrand` arithmetic, §13.3) [C].

The server compares all clients' values for a sequence id once every client has reported; a client whose value no other client shares is flagged and NETSYNCERROR is broadcast (in a two-client game both are flagged); the "fixed" notification is commented out, so **a desync is permanent** and every client that hears of it pauses its simulation (`App::GamePaused`) and shows the red banner [C].

### 13.2 Forensics produced on desync [C] `code/network/clienttoserver.cpp:ClientToServer::ProcessServerLetters`, `ClientToServer::SendIFrame`, `code/network/iframe.cpp:IFrame::CalculateDiff`, `code/network/syncdiff.h`

1. First NETSYNCERROR: `SendIFrame(seq)` (upload "IFrame <myClientId> <seq>" through the FTP manager and keep a local copy), store `m_syncErrorSeqId`, write `syncerror-…txt` (§10.4).
2. While desynced, each `ClientToServer::Advance` retrieves completed remote IFrame files; for frames whose hashes differ, `IFrame::CalculateDiff` does an index-aligned comparison of units, entities, others, lasers, effects and buildings and produces `SyncDiff {WorldObjectId id, Vector3 pos, char *name, char *description, RGBAColour colour}` records with descriptions "Local only", "Remote only", "[POS][VEL][FRONT][UP][CENTRE][RADIUS]" flags, "[CENTREPOS]", "[n ENTITY DIFFS]", "Network Time Different", "Sync Rand Different"; `DumpSyncDiffs` writes `syncdiff-…txt` and `Team::RenderSyncErrors` draws the markers (§11.2).
3. The server relays each uploaded IFrame to every other client's FTP manager (blocks of 1024 bytes, window 10, acks ×3).

In FULL steps 2–3 are inert: no snapshot exists and frames are matched by `SequenceId()`, which stays 0 [C]. The only FULL forensic artefact is the world-state dump of step 1.

### 13.3 Sync-RNG call tracking (`TRACK_SYNC_RAND`) [B:DEBUG] [C] `contrib/systemiv/lib/math/random_number.h`, `contrib/systemiv/lib/math/random_number.cpp`, `code/lib/vector3.h`, `code/lib/vector3.cpp`

- `syncrand()` → `DebugSyncRand(__FILE__, __LINE__)`; `syncfrand(x)` → `DebugSyncFrand(file, line, 0, x)`; `syncsfrand(x)` → `DebugSyncFrand(file, line, −x/2, x/2)` = `min + (_syncrand() × range) × 2⁻³²` — **not bit-identical** to the release formula `(r × 2⁻³² − 0.5) × x`, so tracked and untracked peers diverge numerically; `FRAND`/`SFRAND` (the `Vector3` helpers) carry file/line through `RandomFloat` / `SymmetricRandomFloat` wrappers.
- Each call appends `"%8d | <HashDouble(value)> | <file basename> line %d"` (running call counter) to an `LList<char*>` capped at **100000** entries (oldest dropped). `syncrandseed` logs "SyncRandSeed called". `SyncRandLog(fmt, …)` (1024-character entries) and `SyncRandLogText` annotate the stream.
- `DumpSyncRandLog(file)` writes "SyncRandSeed <seed> (as signed int: <seed>)" then every entry (Unix line endings, truncating the file); `FlushSyncRandLog()` empties the log when a client connects.
- Annotations around every update: "EFFECT BEFORE/AFTER: " + `LogState` around each effect's `Advance` (except snow) (`code/location.cpp:Location::AdvanceWeapons`); building `LogState` around each building's `Advance` (`Location::AdvanceBuildings`); "ENTITY BEFORE/AFTER: " around each other-entity `Advance` (`code/team.cpp:Team::Advance`); "Seq ID: %d", per-letter `WriteSummary` with the `DecodeCmd` code → name table, "Handled as Server Letter: %d", "Handled as update: %d", "g_sliceNum = %d" for the first 1000 sequence ids (`code/network/clienttoserver.cpp:ClientToServer::ProcessAllServerLetters`); crate list logging ([04](04_crates_powerups_and_weapons.md)).
- Entity watch list (`code/location.cpp:TrackEntity`, `UntrackEntity`, `WatchEntities`): a `std::map<Entity*, std::string>` of last `LogState` per watched entity; `TrackEntity` (called from `Entity::NewEntity`) has its selection test commented out (`uniqueId == 3045`), so a developer edits the id to watch one object; `WatchEntities` (called after every entity/effect/building advance) logs "Watch Changed From/To" when the state string changes; deletion logs "Watch Deleted".
- `code/worldobject/spirit.cpp` and `code/worldobject/snow.cpp` `#undef TRACK_SYNC_RAND`, so spirit and snow draws are never logged.
- `docs/multiwiniareleasenotes-beta.txt` says a beta shipped with "debugging code … to track rand sync errors", which "may slow down Multiwinia"; in this tree `TRACK_SYNC_RAND` is commented out for the beta targets (Docs-vs-code).

### 13.4 Reproduction aids [C]

| Aid | Use | Ref |
|---|---|---|
| Command-stream record/replay | pref `RecordDemo` 1 saves `serverHistory.dat`, 2 replays `ServerHistory.dat`; the lockstep stream plus seed reproduces the match | `code/network/server.cpp:Server::SaveHistory`, `Server::LoadHistory` |
| Fixed time step | `RecordDemo` forces `g_advanceTime = 1/DemoFrameRate` (default 25) | `code/main.cpp:UpdateAdvanceTime` |
| Forced seed | testbed `SetRandSeed` / `AddSequenceItem`; `TestBedLog.txt` records the seed and the sync-error sequence id per game | §6 |
| Packet drop | `DebugDropPacket` (F held) [B:DEBUG] | §3.1 |
| Network window | per-client lag, catch-up, "out of sync" flag [B:FULL] | §5.1 |

### 13.5 Network-quality simulator [B:DEAD] [C] `code/network/network_quality.cpp:NetworkQuality`, `code/network/clienttoserver.cpp`

`NetworkQuality(latencyMs, lossProbability, bandwidthBytesPerSec)`; `Write`: run `Advance`, report the full length as written, silently drop if more than the byte budget was queued in the last second, else keep the packet with probability `1 − loss` (test `AppRandom() % 1000 > loss × 1000`, the shared unsynced LCG) and queue it with a timestamp. `Advance`: forget packets older than 1 s from the bandwidth window; send every queued packet older than the latency. Because `Advance` runs only inside `Write`, queued packets leave only when the next packet is written. Used only when `NETWORKQUALITY_TESTING` (commented out) wraps the client's socket writes with `NetworkQuality(200, 0.25, 8000)`.

## 14 Recommendations for the new game's debug and test infrastructure

1. **Cheats as network commands.** A sequenced `DebugCommand` message, accepted by the server only for sessions created with a cheats flag, applied by every peer on the same tick (fixes every "desync" row of §4.3). Show a session-wide "cheats used" marker.
2. **Real sync hashing in every build.** A cheap rolling checksum of authoritative state each tick (and a full snapshot hash every N ticks) instead of hashing the RNG position; keep the last N snapshots; on mismatch automatically exchange snapshots, diff them at object/field level and write a desync report that names the first differing object and field. Support resync or rejoin.
3. **Deterministic replay as the primary test tool.** Record seed + command stream for every match (local ring buffer), replay headless at maximum speed, and use replays as regression tests (state hash at the end must match) and as the benchmark scene.
4. **Headless soak test (the testbed, done properly).** Command-line driven host/clients that join as spectators, play every mode × map with AI teams, and report sync errors, crashes, frame-time percentiles and peak population per game as CI artefacts; no UI button-pressing.
5. **Always-available, low-overhead profiling.** Scoped CPU markers compatible with an external profiler (e.g. PIX / ETW markers) plus an in-game overlay with frame-time graph and per-system budgets; GPU timestamps instead of `glFinish`; thread-safe.
6. **Console and debug UI** reachable in development builds (and optionally behind a launch flag in release): FPS/frame time, entity counts per type, network stats (rates with correct formatting, per-client lag in ticks, inbox gaps), AI overlays, grid/obstruction visualisers.
7. **Logging and crash reporting.** Asynchronous buffered log with categories and levels; crash handler on every thread (vectored exception handler + `std::set_terminate`); full-thread minidumps; consented HTTPS upload; no credentials in the binary; symbol server for release builds (do not rely on frame-pointer walks).
8. **Telemetry with consent** replacing the FTP benchmark (§8.6).
9. **Data validators** in the content pipeline instead of run-time-only checks (the script test of §7.2 and the language test of §5.8 belong in the build).

