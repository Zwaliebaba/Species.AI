---
status: accepted
date: 2026-10-19
decision-makers: the owner (O60, 2026-10-19)
---

# Solution and project layout: six Neuron and Game libraries, two thin programs, one MSBuild solution

## Context and Problem Statement

`AGENTS.md` §2 leaves the concrete layout (the solution, its projects and the edges between them) to be settled when the first project is created, and recorded there and in an ADR. The constraints are fixed already:
- one MSBuild solution at the repository root (ADR-054);
- flat project folders;
- the engine never knows the game (R9);
- the headless server never links presentation code (ADR-083 item 1, PLT-005);
- the client reaches the server only through the transport, and includes no server header except the integrated server's facade (ADR-084);
- dependencies come from the four NuGet packages of R14 (ADR-055, ADR-089);
- test projects per library (ADR-069).

The MVP hosts the server in the client's process until M4 and starts it as a process of its own from M4 (MVP-070, MVP-071). The owner proposed the library split at milestone M0 and decided the open points (O60).

Which projects does the solution hold, what does each own, and which may reference which?

## Decision Drivers

- The engine is reusable and knows no game concept (R9); the game is built on it.
- The headless server builds without the renderer, audio, input, UI or text code and without the Agility SDK and GameInput packages (ADR-056, BLD-013).
- The client and the server share the game's types, messages and pure functions without the client reaching simulation state (ADR-084 item 2).
- Every library can be unit-tested; a program cannot be linked by a test project.
- One toolset, MSBuild only, packages by NuGet restore, nothing vendored (`AGENTS.md` §2, §3; ADR-054, ADR-055).

## Considered Options

* 1 — six static libraries (NeuronCore, NeuronClient, NeuronServer, GameShared, GameLogic, GameClient) and two thin programs (`Species.exe`, `SpeciesServer.exe`)
* 2 — the owner's first proposal: four libraries (NeuronCore, NeuronClient, NeuronServer, GameLogic), with the client linking GameLogic for the shared game types
* 3 — four libraries, with the game's client code in the client program
* 4 — one engine library and one game library, the split being by folder only

## Decision Outcome

Chosen option: "1", decided by the owner (O60). Accepted: the layout binds from the first project of M0.

1. **The solution.** `Species.slnx` at the repository root, built by MSBuild only (`AGENTS.md` §3). Its solution folders are `Neuron`, `Game`, `Programs`, `Tests` and `Tools`. Every project is a `.vcxproj` with its `.filters`, in a flat folder of its own at the repository root, named as the project, with the settings of `AGENTS.md` §3 and R16 stated in the project file. The shader subfolders `Shader/` and `CompiledShader/` exist in NeuronClient only.

2. **The projects** of table 1. Every library is a static library (`.lib`), so no DLL boundary exists and one toolset builds every binary (BLD-032). The two programs are thin entry points: each holds its composition root and `main`, and nothing that a test needs.

   | Project | Kind | Namespace | Owns | References | NuGet packages |
   |---|---|---|---|---|---|
   | `NeuronCore` | library | `Neuron` | the platform layer without a window (files, threads, the clock of ADR-074, the crash path, `WindowsHeaders.h` of PLT-132); logging, checks and PIX markers (ADR-067, ADR-068); maths; the record and schema runtime (ADR-061, ADR-062); keyed draws (ADR-082); terrain generation from the seed (TER, MVP-011); the `Transport` interface with `InProcessTransport` and `QuicTransport` (ADR-088, ADR-089) | none | MsQuic, WinPixEventRuntime |
   | `NeuronClient` | library | `Neuron` | the window (ADR-075); the renderer and its shaders (RND); terrain rendering; animation and VFX; input for keyboard and mouse (ADR-028, ADR-029); audio (AEN); the UI framework and text (UI, ADR-034); the camera (CAM); the client's asset loading (AST) | NeuronCore | Agility SDK; GameInput from the milestone that adds gamepads, because keyboard and mouse use window messages (ADR-028) |
   | `NeuronServer` | library | `Neuron` | the deadline-paced tick loop, region jobs and consistent cuts (ADR-079, ADR-083); the validated command path (ADR-087); the seen-layer and replication framework (ADR-086); snapshot persistence (MVP-075) | NeuronCore | none beyond NeuronCore's |
   | `GameShared` | library | `Species` | the game's ids and types; the message schemas and codecs of commands, state and reports; the record types and tuning keys (MVP-099); the simulation's pure functions that the client may call (movement, terrain queries and the tuning they read; ADR-084 item 2) | NeuronCore | none |
   | `GameLogic` | library | `Species` | the game simulation: `WorldSimulation`, `RegionSimulation`, settlements, Workers, Leaders, battles, relations and the run (MVP §2); the AI settlements and the deputy (MVP-058, MVP-073); `IntegratedServer.h`, the one header through which the client program composes the in-process server (ADR-083, ADR-084) | GameShared, NeuronServer, NeuronCore | none |
   | `GameClient` | library | `Species` | the client session and the game side of the replica (ADR-084); the settlement view, the HUD and the front-end page (MVP-100 to MVP-103); the presentation of Workers, structures, orbs and battles | GameShared, NeuronClient, NeuronCore | none |
   | `Species` | program, `Species.exe` | `Species` | the client's composition root: it creates the client session and, until M4 and for session-paced worlds, the integrated server through `IntegratedServer.h` with an `InProcessTransport` pair; from M4 it starts `SpeciesServer.exe` for an always-on world and connects over `QuicTransport` (MVP-070, MVP-071) | GameClient, GameLogic (through `IntegratedServer.h` only), and what they reference | through the libraries |
   | `SpeciesServer` | program, `SpeciesServer.exe` | `Species` | the headless server's composition root: one world, the transport listener, the snapshots | GameLogic and what it references | through the libraries |

   *Table 1 — The projects of the solution.*

3. **Edges run one way.** No `Neuron` project references a `Game` project (R9). NeuronServer and GameLogic never reference NeuronClient or GameClient, so `SpeciesServer.exe` links no presentation code and none of the Agility SDK and GameInput packages (ADR-056). NeuronClient and NeuronServer never reference each other, and neither do GameClient and GameLogic: libraries at one level share what is below them (BLD-041). A project includes another project's headers only through `$(SolutionDir)<Project>` on its include path (`AGENTS.md` §3).

4. **The client boundary is checked by the build.** GameClient includes no header of GameLogic or NeuronServer. In the `Species` program, only the composition root includes `IntegratedServer.h`, and no other GameLogic or NeuronServer header (ADR-084 item 2, MVP-090). `Build\CheckProjectFiles.py` fails a change that breaks a rule of items 3 and 4.

5. **Tests.** Each library has a test project `<Project>Tests` in a folder of its own beside the library's, building `<Project>Tests.dll` with the Microsoft C++ Unit Test Framework (ADR-069). Its namespace is `<Project>Tests` (R9), and it starts with the placeholder `SuiteSmoke` (`AGENTS.md` §3).

6. **Tools.** Development tools are tool projects in `Tools/<Project>/`, in the `Tools` solution folder (O29). Each is created when a milestone first needs it:
   - the schema code generator (ADR-062) at M0;
   - the content build tool `SpeciesContentBuild.exe` (ADR-059) at M1;
   - the font packer (O56) and the parametric mesh kit (MVP-086) when their assets are first cooked.

   A tool project may reference the Neuron and Game libraries it shares code with. No library or program references a tool project or a tool-only package (`AGENTS.md` §2).

7. **Packages.** Each project that uses a package lists it in its `packages.config` at the pin of the technology baseline. NuGet restores it into `packages\`, and it is never committed (BLD-038, ADR-055, ADR-089). MsQuic is referenced by NeuronCore alone, because the transport is its only caller (ADR-089).

8. **The linter.** The first project created adds the projects of table 1 and their test projects to `.clang-tidy`'s `HeaderFilterRegex`. The form is `(NeuronCore|NeuronClient|NeuronServer|GameShared|GameLogic|GameClient|Species|SpeciesServer|[A-Za-z0-9]+Tests)[/\\][A-Za-z0-9]+\.h$`, so that every hand-written header is checked (`AGENTS.md` §2).

### Consequences

* Good, because the headless server is a program whose link line holds no presentation library, so ADR-056's rule is a property of the references and not of discipline.
* Good, because every line of the client's game code sits in a library that a test links.
* Good, because the integrated server of M0 to M3 and the separate server of M4 are the same GameLogic, composed by two roots behind one transport interface (O10, ADR-083).
* Good, because the client reaches the game's types and pure functions through GameShared without linking the simulation's owners (ADR-084 item 2).
* Bad, because eight projects and six test projects are more project files to keep registered than a two-library split; `Build\CheckProjectFiles.py` carries that cost.
* Bad, because a type that both sides need and that is not engine-generic must move to GameShared, which takes judgement at review.
* It forecloses dynamic libraries and plug-ins: a later modding layer (vision §7) would need its own ADR.

### Confirmation

The layout is confirmed by the first build of M0:
- `Species.slnx` builds `Debug|x64` and `Release|x64` through MSBuild in CI;
- `Build\CheckProjectFiles.py` passes, and it fails on a fixture that makes GameClient include a GameLogic header and on one that makes NeuronServer reference NeuronClient;
- `dumpbin /dependents` of `SpeciesServer.exe` lists neither `d3d12.dll` nor `D3D12Core.dll`, and it shows no GameInput library either.

## Pros and Cons of the Options

### 1 — six libraries and two thin programs

* Good, because the shared game code has a home that neither side owns.
* Good, because every library is testable.
* Bad, because it has the most projects.

### 2 — four libraries, the client linking GameLogic

* Good, because it has the fewest libraries.
* Bad, because the client links the simulation's owners and could reach server state, which ADR-084 forbids, and only review would stop it.

### 3 — four libraries, the game's client code in the program

* Good, because it keeps the owner's four libraries.
* Bad, because the client's game code cannot be linked by a test project, and the shared game types still need a home.

### 4 — one engine and one game library

* Good, because there are few projects.
* Bad, because the headless server would link the client's engine code unless folders were policed by hand, which the build cannot check.

## More Information

* Owner decision O60 records the choice and its options. ADR-056 names the programs, and the build tooling component's table 3.1-A lists the products; both carry an amendment note to this record.
* `AGENTS.md` R9 names the engine namespace `Neuron`, and §2 points to this record. The design's code sketches that were written with `Engine::` read `Neuron::` (the component index's amendment note).
* The MVP's milestone M0 plan ([m0_plan.md](../implementation/m0_plan.md)) builds this layout first.
* **Amendment 2026-10-19 (M0 task T3).** Item 7 is widened: a package whose import library or DLL a library uses is also listed by every project that links that library, the programs and the test projects, because a static library passes neither its link inputs nor its DLL on. WinPixEventRuntime is therefore restored by NeuronCore, which includes `pix3.h`, and by `Species`, `SpeciesServer` and the six test projects, whose builds link `WinPixEventRuntime.lib` and copy `WinPixEventRuntime.dll` beside their output through the package's `.targets` file. The test projects need the DLL at run time, since `vstest.console.exe` loads them from the shared output folder.
* **Amendment 2026-10-20 (M0 task T4).** Under the same rule, `Species` also restores the Agility SDK package (`Microsoft.Direct3D.D3D12`): its build copies `D3D12Core.dll` and `D3D12SDKLayers.dll` into `x64\<Configuration>\D3D12\` (BLD-042), and its linker exports the SDK constants ([ADR-092](ADR-092-first-window-device-and-presentation.md)). A library names the system import libraries it calls (`d3d12.lib`, `dxgi.lib`, `shcore.lib`, `version.lib`, `bcrypt.lib`) with `#pragma comment(lib, ...)` in the source that calls them, so a program or test links each only when it links that source; no test project restores the Agility SDK, because no test links a Direct3D object. `Build/CheckProjectFiles.py` rule 14 rejects a Direct3D or DXGI library named that way outside NeuronClient and GameClient (BLD-045), and an include of `d3d12.h` or a DXGI header other than `dxgiformat.h` outside NeuronClient (ADR-078).
* **Amendment 2026-10-20 (M1 task T11, [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md)).** Item 6's content build tool is created: `Tools/SpeciesContentBuild`, which references NeuronCore and GameShared, whose generated `TuningMvp.h` holds the tuning record's reader and cooker. It holds the font packer, which item 6 listed as a tool of its own, because the content build runs it on every face. `.clang-tidy`'s `HeaderFilterRegex` names it. `Species` and `SpeciesServer` carry a solution build dependency on it (`BuildDependency` in `Species.slnx`): it orders the build so that building a program cooks the content, and it is no reference, so nothing of the tool links into a program.
* **Amendment 2026-10-20 (M1 task T10, [ADR-110](ADR-110-cost-measurement-marker-timing-and-performance-script.md)).** The tool project `Tools/SpeciesPerformance` is created: the performance tool, which references GameLogic, GameShared, NeuronServer and NeuronCore and links into no game program. `.clang-tidy`'s `HeaderFilterRegex` names it, and it carries a solution build dependency on the content build, whose cooked tuning record it reads.
