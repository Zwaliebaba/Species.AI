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

