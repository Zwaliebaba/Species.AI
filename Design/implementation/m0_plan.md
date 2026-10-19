# M0 plan

> Status: Draft · Related: [MVP scope](mvp_scope.md) §3 (M0) · [ADR-090](../ADR/ADR-090-solution-and-project-layout.md) (the project layout) · [MVP assets](mvp_assets.md) · [owner decisions](../decisions/owner_decisions.md) (O50 to O63) · [ADR index and technology baseline](../ADR/README.md)

## 1 Purpose

This plan turns milestone M0 of the [MVP scope](mvp_scope.md) into work an implementing session can start cold. It covers:
- how the work flows between the cloud session and the owner's PC (§2);
- what the owner's PC needs (§3);
- the order in which M0 builds the engine baseline (§4);
- the tasks of M0, each a pull request (§5);
- what the owner prepares in parallel for M1 (§6).

M0 ends when the exit criteria of the MVP scope's §3 hold:
- Debug and Release build;
- CI is green, with the three checkers gating;
- the client shows the server's tick count through the transport;
- the unit tests pass.

## 2 How the work flows

The implementing sessions run in a cloud container without a Windows toolchain: they write code, run the Python checkers and push to a branch, but they cannot compile with MSVC and cannot run the game. The owner's Windows PC runs it. So:

1. **CI is the compiler.** Every push builds `Debug|x64` through `Species.slnx` on CI's Windows image, which carries VS 2026 with MSVC 14.51 and the v145 toolset. It runs every test suite and clang-tidy, and the other jobs check formatting and the design (`AGENTS.md` §6). A session pushes small commits, so that a red build points at one change.
2. **The owner runs it.** A pull request that touches rendering, input, audio or presentation states a **run checklist**: what to launch, what to look at, what must happen. The owner pulls the branch and builds `Debug|x64`, and `Release|x64` before any claim about Release (`AGENTS.md` §6). The owner then runs `Species.exe` and answers on the pull request or in the session with what happened. Such a pull request is not merged before that answer.
3. **Nothing is claimed that was not done.** A session reports "builds in CI, not run" until the owner has run it (`AGENTS.md` §3).
4. **Requirement ids in every change and test** (CLAUDE.md), so that the playtest evidence later traces to the rules.

## 3 The owner's PC

| What | Needed | Source |
|---|---|---|
| Windows | Windows 11, version 24H2 (build 26100) or later | ADR-050 |
| Visual Studio 2026 | version 18.11 or later, with the workload "Desktop development with C++": the MSVC v145 build tools (14.50 or later), the C++ unit-test components and MSBuild | ADR-051, `bld.pin.visual_studio` |
| Windows SDK | 10.0.26100.9457, from the Visual Studio installer's individual components | `bld.pin.windows_sdk` |
| Graphics Tools | the Windows optional feature "Graphics Tools", which installs the Direct3D 12 debug layer that Debug builds enable | [renderer](../architecture/components/renderer/README.md) |
| Graphics adapter | feature level 12_1, Shader Model 6.6, resource binding tier 3 and wave operations, with a current driver; a DirectX 12 Ultimate adapter normally meets them, and M0's capability probe confirms it | ADR-002 |
| Git, Python | Git; Python 3.12 for the checkers and the design tools | `AGENTS.md` §3 |
| PIX on Windows | optional, for GPU captures and markers | ADR-068 |

This PC becomes the reference test machine at M1, and its configuration is recorded then (O58). CI's image carries Visual Studio 18.10.1 while `bld.pin.visual_studio` names 18.11 as the developer minimum. Task T12 settles which number is the pin.

## 4 The order of the engine baseline

The engine components specify the whole engine. The MVP builds them in the order its milestones need, and the specifications stay the target. What M0 and M1 build of each pipeline:

| Part | M0 | Later |
|---|---|---|
| Content loading | none | the Debug loose-content mount (AST-105) of `GameData/`, the content root (O61), from M1; packs (ADR-058), editions (ADR-056 manifests) and content identity by hash (ADR-060) before the first build that leaves the owner's PC |
| Content build tool | none | `SpeciesContentBuild.exe` at M1, with the texture cooker of ADR-017 (DirectXTex, a tool-only package that ADR-017 records), the check that packs a DDS in its row's format as authored (O61) and the font packer of O56 and O63; the glTF importer and mesh cooker of ADR-019 at M2; the audio cooker at M3 |
| Schema code generator | the tool project of ADR-062, for the schema subset that the first tuning record needs | the further record types as each milestone adds them |
| Input | none | keyboard and mouse through window messages at M1 (ADR-028); GameInput when gamepads arrive, after the MVP |
| Text | none | the owner's two bitmap faces at M1, GameFont for the game and EditorFont for the development overlay, at whole-number scales (ADR-034 as amended, O62, O63); the generated placeholder face only for a face that is missing |
| Audio | none | the first sound set at M3 (MVP-087) |
| Transport | `InProcessTransport` | `QuicTransport` over MsQuic at M4 (ADR-089) |
| Crash reporting, signing, installer | none | before the first external build (ADR-065, ADR-071, ADR-072, ADR-073) |

## 5 Tasks

Each task is one pull request on a branch off `main`, in this order. Each one cites the requirements it implements.

| Task | Builds | Done when | Ids |
|---|---|---|---|
| **T1 Solution skeleton** | `Species.slnx`; the six libraries, the two programs and the six test projects of ADR-090, each with its `.vcxproj` and functional `.filters`; the settings of `AGENTS.md` §3 and R16 stated in every project, Debug and Release aligned; `pch.h` per project; `WindowsHeaders.h` in NeuronCore; `SuiteSmoke` in every test project; `packages.config` at the pins for the Agility SDK and WinPixEventRuntime (BLD-038); the `HeaderFilterRegex` of ADR-090 item 8 | CI builds `Debug|x64` and vstest runs six suites; the owner builds `Release|x64` | ADR-090, BLD-013, BLD-036, BLD-038 |
| **T2 Checkers** | `Build\CheckFormat.py`; `Build\CheckProjectFiles.py` with its fixtures; `Build\RunClangTidy.py`. CheckProjectFiles covers R2 affixes, R7 names and registration, R11 spellings, the flat directories and shader names, functional filters, Debug and Release alignment, the edges and include boundary of ADR-090 items 3 and 4, and the package pins per project | the checkers pass on the tree and fail on their fixtures; CI's guards for them are removed | `AGENTS.md` §1, §2, §6; BLD-037, BLD-041 |
| **T3 NeuronCore basics** | logging (ADR-067); the development switch and `ENGINE_ASSERT` (BLD-014); the PIX scope macro (ADR-068); the clock (ADR-074); files with an atomic replace; threads; the crash path's first form (PLT) | unit tests of each | PLT, DBG-001 to DBG-049 |
| **T4 Window and device** | the Win32 window with per-monitor DPI v2 (ADR-075); the Direct3D 12 device on the pinned Agility SDK with its SDK exports (BLD-042); the capability probe (ADR-002); the swap chain and frame pacing (ADR-012); device removal (ADR-008); the GPU resource factory's first form (ADR-078); a frame cleared to black | the owner runs `Species.exe`: a black window that resizes, moves between monitors, minimises and closes cleanly, with the debug layer silent | RND-001 to RND-045, ADR-001, ADR-002, ADR-012 |
| **T5 Transport** | the `Transport` interface and `InProcessTransport` (ADR-088 item 1); message framing with the codec skeleton (ADR-088 item 5) | unit tests send, receive and close in both directions | ADR-088, MVP-090 |
| **T6 Server host** | NeuronServer's composition root; the deadline-paced loop at 10 ticks per second (ADR-079, ADR-083); the command path that executes at the next tick in keyed order (ADR-087) | a headless test runs 600 ticks on the deadline | MVP-092, MVP-094 |
| **T7 Game skeletons** | GameShared's first message (the tick report); GameLogic's `WorldSimulation` with one `RegionSimulation`; `IntegratedServer.h`; GameClient's session, which shows the server's tick count in the window title until the UI exists | the owner runs `Species.exe` and sees the tick count rise at 10 per second | MVP-090, MVP-091, MVP-096, ADR-084 |
| **T8 Draws and hashes** | keyed draws (ADR-082); the per-tick state hash and its golden test | the hash of 600 ticks from one seed is stable across runs | MVP-093, DBG-147 |
| **T9 Tuning in data** | the schema code generator tool project (ADR-062) for the subset that one tuning record needs; the record of the MVP's `mvp.*` keys, loaded at server start; `Build\CheckGeneratedSchemas.py` | a tuning value changed in data changes the server's behaviour with no rebuild of code; CI regenerates and finds no difference | MVP-003, MVP-099, `AGENTS.md` §2 |
| **T10 Snapshots** | the versioned snapshot of an empty world: written to a temporary file, flushed and renamed atomically, and loaded and validated; a snapshot of an unknown version is refused | unit tests of a round trip, a torn write and a refused version | MVP-075, MVP-095 |
| **T11 Headless server** | `SpeciesServer.exe`, running the same world with no client | `dumpbin /dependents SpeciesServer.exe` shows no Direct3D, DXGI or GameInput library (ADR-090 confirmation) | ADR-083, ADR-090 |
| **T12 Housekeeping** | `playtests/` with its README (MVP §6.3); the Visual Studio pin settled between 18.10 (CI) and 18.11; every CI guard whose input now exists removed (`AGENTS.md` §6) | CI green with nothing skipped that has input | MVP-002 |

*Table 1 — The tasks of M0, in order.*

T3, T5 and T8 have no dependency on T4 and can run beside it; everything else follows the table's order.

## 6 What the owner prepares for M1

M1 starts on generated placeholders for whatever is missing, so nothing here blocks M0. The owner's 17 textures and two fonts are in `GameData/` already ([MVP assets](mvp_assets.md) §6). Its look exit criterion, a battle that is fun to watch, needs the owner's other hand-made assets of [MVP assets](mvp_assets.md) §4, drawn while M0 is built:
- the Worker cards: front, side and the four hurt poses;
- the 14 vector files that M1 uses: cursors, markers, the Leader banner states and the first icons.

The reference test machine is pinned at the start of M1 (O58).
