# Build matrix — what is actually live in the Multiwinia PC build

Phase 1 output. Source root for every path below: `multiwinia/` in `gene9/Darwinia-and-Multiwinia-Source-Code` @ `f909630267a320c8f8de5e30bbb2dbf5f3a68acf`.

## 1. Baseline configuration documented by this design set

| Axis | Baseline value | Evidence |
|---|---|---|
| Product target | `TARGET_PC_FULLGAME` (the only uncommented target; the header `#error`s unless exactly one is set) | [C] `code/lib/universal_include.h` "PICK ONE OF THESE TARGETS" block |
| VS configuration | `Release|Win32` → `darwinia_gl.exe`, OpenGL renderer (no `USE_DIRECT3D`) | [C] `targets/vs2005/Darwinia.vcproj` config `Release|Win32`, `InheritedPropertySheets=Common.vsprops;OpenGL.vsprops` |
| Shipped binary | `darwinia_gl.exe` copied to `build\multiwinia.exe` by every Multiwinia packaging script | [C] `tools/build_multiwinia_retail.bat`, `build_multiwinia_gold.bat`, `build_multiwinia.bat`, `build_multiwinia_crashreporter.bat`, `build_multiwinia_hwcompat*.bat`, `build_multiwinia_pc_preview.bat` |
| Secondary PC configuration (documented in full in 24_rendering) | `ReleaseDirect3D|Win32` → `darwinia.exe`, `USE_DIRECT3D` defined: D3D9 implementing the OpenGL API via `lib/opengl_directx*` | [C] `targets/vs2005/Direct3D.vsprops` (`PreprocessorDefinitions="USE_DIRECT3D"`, links `d3d9 d3dx9 d3dxof`). Only Darwinia-era scripts (`tools/build_final.bat`, `tools/build_vista.bat`) copy `darwinia.exe` |
| Platform | Win32 (`WIN32;TARGET_MSVC;_CRT_SECURE_NO_DEPRECATE`) | [C] `targets/vs2005/Common.vsprops` |
| Audio | DirectSound (`HAVE_DSOUND`), linked `dsound.lib`; XInput delay-loaded (`xinput1_3.dll`) | [C] `universal_include.h` TARGET_MSVC block; `Common.vsprops` linker |
| Data packaging | `data/` packed to `.dat` RAR archives by `tools/build_multiwinia_data.bat`; unrar linked in | [C] `Darwinia.sln` sub-project `contrib/unrar/Unrar.vcproj`; [I] archive details resolved in 29/31 |

Linked sub-projects of `Darwinia.sln` (all configs): `darwinia`, `unrar` (contrib/unrar), `eclipse` (contrib/eclipse — Introversion's in-house window framework, **live**), `vorbis_static`, `vorbisfile_static`, `ogg_static`, `libpng`, `zlib`. [C] `targets/vs2005/Darwinia.sln`.

## 2. The four Visual Studio configurations

| Config | Property sheets | Extra defines | Output | Status |
|---|---|---|---|---|
| `Debug|Win32` | Common + OpenGL | `_DEBUG` (MIDL/RC) | `darwinia_d_gl.exe` | dev only |
| `Release|Win32` | Common + OpenGL | `NDEBUG` | `darwinia_gl.exe` | **baseline (ships as multiwinia.exe)** |
| `DebugDirect3D|Win32` | Common + Direct3D | `USE_DIRECT3D`, `_CRT_NON_CONFORMING_SWPRINTFS` | `darwinia_d.exe` | dev only |
| `ReleaseDirect3D|Win32` | Common + Direct3D | `USE_DIRECT3D`, `_CRT_NON_CONFORMING_SWPRINTFS` | `darwinia.exe` | secondary |

`README.TXT` lists two additional "PC OpenGL (Release / ReleaseDirect3D)" configurations that do not exist in the `.vcproj` [C] — docs-vs-build discrepancy logged in 41.

## 3. Product targets (`code/lib/universal_include.h`) and the feature macros each sets

`TARGET_PC_FULLGAME` is active. All other targets are compiled out but documented here because they gate code.

| Macro | FULLGAME (baseline) | DEBUG | PC_PREVIEW | MULTIWINIA_DEMOONLY | PC_HARDWARECOMPAT | ASSAULT_STRESSTEST | BETATEST_GROUP_ALL | BETATEST_GROUP_A/B/C |
|---|---|---|---|---|---|---|---|---|
| `DARWINIA_GAMETYPE` | "multiwinia" | "debug" | "pc-preview" | "multiwinia-demo" | "hwcompat" | "assault-stress" | "beta-group-all" | "beta-group-a/b/c" |
| `LAN_PLAY_ENABLED` | Y | Y | – | – | – | – | Y | – |
| `WAN_PLAY_ENABLED` | Y | Y | – | – | – | – | Y | Y |
| `MULTIPLAYER_DISABLED` | – | – | Y | Y | Y | Y | – | – |
| `NETWORK_STATS_ENABLED` | Y | Y | – | – | – | – | Y | – |
| `INCLUDEGAMEMODE_DOMINATION` | Y | Y | – | – | – | – | Y | – |
| `INCLUDEGAMEMODE_KOTH` | Y | Y | Y | Y | Y | – | Y | – |
| `INCLUDEGAMEMODE_CTS` | Y | Y | Y | Y | – | – | Y | A |
| `INCLUDEGAMEMODE_ROCKET` | Y | Y | – | – | – | – | Y | C |
| `INCLUDEGAMEMODE_ASSAULT` | Y | Y | – | – | – | Y | Y | – |
| `INCLUDEGAMEMODE_BLITZ` | Y | Y | – | – | – | – | Y | B |
| `INCLUDEGAMEMODE_TANKBATTLE` | – (commented out) | Y | – | – | – | – | – | – |
| `INCLUDEGAMEMODE_PROLOGUE` / `_CAMPAIGN` | – | Y | – | – | – | – | – | – |
| `INCLUDE_CRATES_BASIC` | Y | Y | Y | Y | Y | Y | Y | via `TARGET_BETATEST_GROUP` |
| `INCLUDE_CRATES_ADVANCED` | Y | Y | – | – | – | – | Y | A only |
| `INCLUDE_TUTORIAL` | Y | Y | Y | – | – | – | Y | – |
| `AUTHENTICATION_LEVEL` | 1 | 1 | – | – | – | – | 1 | 1 (group) |
| `BLOCK_OLD_DARWINIA_OBJECTS` | Y | Y | – | – | – | – | Y | – |
| `DUMP_DEBUG_LOG` | Y | – | – | Y | Y | Y | Y | Y (group) |
| `PERMITTED_MAPS` | – (all maps) | – | KOTH_2P_1, KOTH_4P_1, CTS_2P_1 | – | KOTH_2P_1, BENCHMARK_1, KOTH_4P_1 | ASSAULT_STRESS | – | per-group list |
| `SPECTATOR_ONLY`, `HIDE_INVALID_GAMETYPES` | – | – | – | – | Y | Y | – | HIDE only |
| `CHEATMENU_ENABLED`, `SOUND_EDITOR`, `GESTURE_EDITOR`, `AVI_GENERATOR`, `TRACK_SYNC_RAND`, `INCLUDE_CRATE_HELP_WINDOW` | – | Y | – | – | – | – | – | – |
| `LOCATION_EDITOR` | – (commented out) | – (commented out) | – | – | – | – | – | – |

Global (all targets): `DEBUG_RENDER_ENABLED`, `FLOAT_NUMERICS`, `USE_CRASHREPORTING` defined; `USE_SOUND_DSPEFFECTS`, `USE_SOUND_HW3D`, `USE_SOUND_MEMORY`, `USE_LOADERS` commented out. `GRACE_TIME` = 3.0 s (20.0 s under `TESTBED_ENABLED`). `TEXTURE_EXTENSION` = "bmp". [C] `universal_include.h`.

Determinism-relevant global: all `iv_sin/cos/tan/sqrt/pow/exp/log/asin/acos/atan` wrap the CRT call in `iv_trunc_low_nibble(double)` — see 22. [C] `universal_include.h`.

## 4. Other conditional-compilation families (state in baseline)

File counts = number of files under `code/`, `contrib/systemiv/lib`, `contrib/eclipse` with an `#if/#ifdef/#ifndef/#elif` on the macro.

| Macro | Defined where | Baseline state | Files | Meaning / consequence |
|---|---|---|---|---|
| `USE_SEPULVEDA_HELP_TUTORIAL` | nowhere | **off** | 49 | Darwinia-era Sepulveda help/tutorial paths — dead in every Multiwinia build |
| `LOCATION_EDITOR` | commented out in every target | **off** | 39 | In-game level editor and all editor windows compiled out of the shipped exe. Documented in 08 as a design capability (code present), tagged *editor build only* |
| `TESTBED_ENABLED` | nowhere active | off | 16 | Automated test-bed mode |
| `TRACK_SYNC_RAND` | `TARGET_DEBUG` | off | 9 | Per-call logging of synced random numbers for desync forensics |
| `SOUND_EDITOR` | `TARGET_DEBUG` | off | 16 | Sound editor windows |
| `TARGET_OS_VISTA` | nowhere | off | 10 | Vista Game Explorer / parental controls |
| `TARGET_OS_MACOSX`, `TARGET_OS_LINUX` | Mac/Linux toolchains | off | many | Platform appendix only |
| `USE_DIRECT3D` | `Direct3D.vsprops` | off in baseline, **on in secondary** | ~50 | Selects D3D9 GL-emulation |
| `USE_DIRECT3D_TRILIST` | `landscape_renderer.cpp`, `water.cpp` (inside D3D path) | secondary only | 2 | Tristrip→trilist for flat shading under D3D |
| `USE_DISPLAY_LISTS` | `global_internet.cpp`, `worldobject/tree.cpp`, `lib/shape.cpp` | on | 3+ | GL display lists (emulated under D3D: `opengl_directx_dlist*`) |
| `INVOKE_CALLBACK_FROM_SOUND_THREAD` | `TARGET_OS_MACOSX` | off | 7 | Sound threading variant |
| `DEBUG_RENDER_ENABLED` | global | **on** | 17 | Debug overlay renderers compiled into shipping exe |
| `USE_LOADERS` | commented out | **off** | 17 | Loading-screen mini-programs gated (verify per-file in P20) |
| `INCLUDE_THEBEAST` | `worldobject/crate.h` | on | 3 | Crate outcome "The Beast" |
| `USE_FIXED`, `FIXED64_NUMERICS` | nowhere | off | 3 / – | Fixed-point numerics alternative |
| `DEMOBUILD`, `DEMO2`, `EGAMESBUILD`, `OBERONBUILD`, `AMBROSIA_REGISTRATION` | nowhere / Darwinia-era | off | 7/7/8/4/2 | Darwinia demo and distributor variants |
| `BENCHMARK_AND_FTP` | commented out | off | 4 | Hardware-compat benchmark upload |
| `USE_DARWINIA_MOD_SYSTEM` | nowhere | off | 7 | Darwinia mod loading |
| `HAVE_XAUDIO`, `_XBOX` | nowhere on PC | off | 3 / 2 | Xbox 360 remnants (`code/lib/input/xbox_eventhandler.cpp` is `ExcludedFromBuild` in all four configs) |
| `USE_SOUND_DSPEFFECTS`, `USE_SOUND_HW3D`, `USE_SOUND_MEMORY` | commented out | off | 2 each | Sound-system variants |
| `TEST_HARNESS_ENABLED`, `ATTRACTMODE_ENABLED`, `PURITY_CONTROL`, `SCRIPT_TEST_ENABLED`, `PROFILER_ENABLED` | nowhere | off | 4/2/2/3/3 | Dev features |
| `HAVE_REFRESH_RATES` | `interface/prefs_screen_window.cpp` | on | 1 | Refresh-rate option |
| `USE_PIXEL_EFFECT_GRID_OPTIMISATION` | `renderer.cpp` | on | 1 | Pixel-effect optimisation |
| `STRETCH_RECT` | `deform.cpp` | on | 1 | Deform effect via StretchRect (D3D) |
| `USE_PENTIUM_COUNTER` | `lib/hi_res_time.cpp` (=0) | off | 1 | RDTSC timer |
| `MULTIWINIA_DEMOONLY` | `TARGET_MULTIWINIA_DEMOONLY` | off | 4 | Demo-only build (distinct from the *runtime* demo gate below) |

## 5. Runtime gating inside the baseline binary

Compiled-in does not mean reachable. These runtime gates further restrict what a player experiences:

| Gate | Mechanism | Evidence |
|---|---|---|
| Game mode | `App::m_gameMode ∈ {GameModeNone, GameModePrologue, GameModeCampaign, GameModeMultiwinia}`; Prologue/Campaign menu buttons are compiled only under `INCLUDEGAMEMODE_PROLOGUE/_CAMPAIGN` (not in FULLGAME) | [C] `code/app.h` enum; `code/UI/GameMenuWindow.cpp` main-menu construction |
| Multiwinia game types | `Multiwinia::GameTypeSkirmish(=Domination), KingOfTheHill, CaptureTheStatue, Assault, RocketRiot, Blitzkreig, TankBattle` (TankBattle compiled out by `INCLUDEGAMEMODE_TANKBATTLE`) | [C] `code/multiwinia.h`; `code/multiwinia.cpp`, `code/app.cpp` |
| Legacy objects | `Building::BuildingIsBlocked` / `Entity::EntityIsBlocked` return true for Darwinia-only types under `BLOCK_OLD_DARWINIA_OBJECTS` | [C] `code/worldobject/building.cpp:BuildingIsBlocked`, `code/worldobject/entity.cpp:EntityIsBlocked` |
| Demo vs full | `IS_DEMO` ≡ `!g_app->IsFullVersion()` — runtime authentication-key gate on top of FULLGAME | [C] `code/game_menu.h` |

## 6. Files excluded from the live set

- Source files under `code/` not referenced by `Darwinia.vcproj` (47 files: SDL/Mac/Linux/GCC variants, `buynow_window`, `updateavailable_window`, `ErrorDialog`, `tri_tri`, `input_sdl`, `code/unicode/sound_library_*`, debug-logging stubs, etc.). Full list: `manifest.csv` rows with `area=code`, `type=source/*`, empty `vcproj_build`.
- `code/lib/input/xbox_eventhandler.cpp`: listed but `ExcludedFromBuild` in all four configurations.
- 14 `.vcproj` entries refer to files absent from the tree (Xbox 360 `x360_qnet`, `multiwinia.spa.h`, XBLA UI buttons, `preferences.txt`, cursor/resource headers) — Xbox Live Arcade remnants.
- `contrib/systemiv`: only `lib/{debug_utils,memory_leak,network_stream,profiler,string_utils}`, `lib/filesys`, `lib/math` (8 of 30 files), `lib/metaserver` (6 of 12), `lib/netlib` (16 of 20), `lib/tosser` (9 of 17), `lib/unicode` are compiled. `contrib/systemiv/interface`, `lib/eclipse`, `lib/gucci`, `lib/render`, `lib/resource`, `lib/sound`, `template`, `tools`, `contrib/*` are **not** compiled (Multiwinia uses its own `code/lib`, `code/sound`, `code/interface` and `contrib/eclipse` instead).
- `targets/linux/Makefile` and `targets/macosx/*` still reference Darwinia (`TARGET_DEMOGAME`/`TARGET_FULLGAME`, `Darwinia.xcode`) and do not build Multiwinia [I] (not maintained for this tree). Platform appendix only.

## 7. Tagging convention used across all documents

Every feature is tagged with the builds in which it exists:

- **[B:FULL]** baseline `Release|Win32` + `TARGET_PC_FULLGAME`.
- **[B:D3D]** additionally/only under `USE_DIRECT3D` (`ReleaseDirect3D`).
- **[B:DEBUG]** only under `TARGET_DEBUG`.
- **[B:EDITOR]** only under `LOCATION_EDITOR` (no shipped target defines it).
- **[B:DEMO]** runtime demo gate (`IS_DEMO`) or `TARGET_MULTIWINIA_DEMOONLY` — state which.
- **[B:LEGACY]** compiled into FULL but unreachable in Multiwinia (Darwinia campaign / `BLOCK_OLD_DARWINIA_OBJECTS`).
- **[B:DEAD]** not compiled in any PC configuration (appendix only).
