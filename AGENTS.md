# AGENTS.md — Engineering Rules

Operating instructions for every agent (and human) writing code in this repository. **Read this before generating a single line.**

This repository is a greenfield C++23 game and a hobby project with one developer: a Direct3D 12 game built on Windows with MSVC. This file is about **how code is written here** — naming, layout, build settings and the standing rules of the codebase. It is not the design: what the game *is* belongs in `Design/` and `vision/` (see "What is authoritative" below).

**There is no code yet.** This repository holds this file, the root configuration files, `.gitignore`, `.github/`, the design material and the owner's first content in `GameData/` — no solution, no projects, no source. Nothing below is a target to migrate towards; it describes the code as it must be written from the first line. There is no legacy here and nothing is grandfathered, so a whole-tree run of any code checker comes back clean — trivially today, and by conformance from then on.

**What the tree holds today (2026-10-18):**

| Path | What it is |
|---|---|
| `CLAUDE.md` | The entry point for Claude Code: it imports this file and adds where things are and the current status. |
| `vision/species-vision-brief.md` | The Species pitch and the owner's decisions O1–O30; every later owner decision is in `Design/decisions/owner_decisions.md`. |
| `Design/` | The **Species reference library**: `reference/` (the reference mechanics of a skirmish match, engine-agnostic), `architecture/components/` (the engine baseline, one specification per component), `ADR/` (the baseline architecture decision records), `decisions/` (the decision register, `DEC-NNN`), `assets/` (every asset to create, `GameAssets.md`), `glossary/`, `CONVENTIONS.md` and `REQUIREMENTS_INDEX.md`. Beside it, the **Species design** of wave A: `gdd/` (pillars and core loops, world and regions), `architecture/04_solution_strategy.md` with ADR-079 onward, and `implementation/` (the MVP scope and its assets, what is built next). |
| `prompts/species-design-phase.md`, `species_design_work/` | The design-phase run and its working files; the run is paused after wave A (owner decision O51) and resumes from the MVP's playtest evidence. |
| `GameData/` | The **content root** (owner decision O61): the authored assets that the content build reads, in PascalCase top-level folders (`Fonts/`, `Textures/`). Today the owner's two bitmap fonts with their generated glyph tables, 17 textures and one parked texture. Content, not code: no checker of §1 to §4 applies to it, and `Design/assets/` is its manifest. |
| `Tools/DesignCheck/`, `Tools/DesignModels/` | The design gate checker (run on every change under `Design/`) and the design phase's quantitative models. They never ship and never link (R14). |

**Where these rules come from.** They are carried over from two sibling repositories: `Outpost.Warzone`, where the formatter and linter settings were measured against roughly 223,000 lines, and `Nomad-Commander`. That lineage is why `.clang-format` and `.clang-tidy` are what they are, and it is why code can move between the trees without a rename or a reflow pass. **What did not come across is the other trees' design, their decisions or their plan.** A decision taken there binds nothing here.

**What is authoritative for how code is written, in order:**

1. **This file** — conformance: naming, style, build settings, and how to work here.
2. **`Design/ADR/`** — engineering decisions, one file per decision (§6). The baseline holds **ADR-001 to ADR-078**, written when the reference library was built (the index and the pinned technology baseline are in `Design/ADR/README.md`); new records continue from the highest id. Where a baseline ADR and this file disagree, this file wins and the ADR is amended.
3. **The surrounding code** — for anything neither of the above covers, match the file you are editing.

**What is authoritative for what is built:** the owner's decisions in `vision/species-vision-brief.md`, then the Species game design when the design phase has written it, then `Design/`: the engine component specifications are the engine baseline, and the reference mechanics are adopted, adapted or dropped by the design phase requirement by requirement. Open questions are entries in the decision register (`Design/decisions/`); an Open entry states the behaviour to build until it is decided.

The design sits alongside this file rather than above it: the design says what is built and this file says how. A task that needs a design answer the design does not give asks the owner, and the answer is written down (in the design or the decision register) before the code is.

If a rule here conflicts with a habit from another codebase, this file wins. If you think a rule is wrong or your task cannot be done without deviating, **say so in your report — never deviate silently.**

---

## 1. Naming convention (normative — no exceptions)

| Kind | Convention | Example |
|---|---|---|
| Type (class, struct, enum, concept, alias) | `PascalCase` | `SwapChainTarget` |
| Function, method | `PascalCase` | `PresentFrame()` |
| Member variable | `m_camelCase` | `m_deviceRemoved` |
| Static member (mutable) | `sm_camelCase` | `sm_activeDevice` |
| Global | `g_camelCase` | `g_instance`, `g_frameCount` |
| Parameter | `_camelCase` | `_fileName`, `_entityId` |
| Local | `camelCase` | `shadedColor` |
| Compile-time constant | `UPPER_CASE` | `WIDTH_PIXELS`, `CELL_PIXELS` |
| Enumerator | `PascalCase` | `DeviceLost`, `OutOfVideoMemory` |
| Macro | `UPPER_CASE` | `ENGINE_ASSERT` |
| Namespace | `PascalCase` | `Engine` |
| File | `PascalCase.cpp` / `.h` | `SwapChainTarget.cpp` |

**Note the split that catches people out: a `constexpr` is `UPPER_CASE`, an enumerator is `PascalCase`.** They are both compile-time and they are spelled differently on purpose — an enumerator is a *value of a type* and reads as one at the use site (`PageFault::OutOfVideoMemory`), while a constant is a number with a name and is meant to look like one. [`.clang-tidy`](.clang-tidy) enforces both, and it is the single source of truth for the option values; this document states the rules in prose and does not repeat the settings, so there is nothing to drift.

### The rules behind the table

**R1 — The leading underscore on parameters is deliberate.** It is legal C++: the reserved forms are `_Uppercase`, anything containing `__`, and `_lowercase` **at global scope**. A parameter is never at global scope, so `_fileName` is safe. Never introduce a reserved form — no `_Impl`, no `__helper`, no file-scope `_cache` (use `g_cache` in an anonymous namespace).

**R2 — A type name carries no prefix or affix, and that includes abstract ones.** An interface is `Transport`, not `ITransport`. A base class is not `BaseTransport` or `AbstractTransport`. PascalCase means the name and nothing else. This bans `CFoo`, `SFoo`, `EFoo`, `IFoo`, `FooBase`, `FooAbstract`, `FooImpl` and `_t` suffixes. Name the concept and let the concrete types say what they are:

```
Transport             ← the concept
├── UdpTransport      ← a socket-backed one
└── LoopbackTransport ← in-process, for tests
```

That tree is an illustration of the rule, not a description of anything. A base class for one derived class is ceremony: name the concept, and add the layer when a second thing needs it.

clang-tidy can require an *absent* prefix but cannot see a *present* suffix, so the repository checker carries the other half (§6).

**R3 — Compile-time constants are `UPPER_CASE`.** `constexpr`, `inline constexpr` and `static constexpr` members: `WIDTH_PIXELS`, `TICKS_PER_SECOND`, `CELL_PIXELS`. `sm_` is reserved for *mutable* statics, which are rare and must document their thread-safety.

**R4 — Acronyms capitalize as words**: `HlslSource`, `DxgiFactory`, `UdpTransport` — never `HLSLSource`. Identifiers from an external SDK keep that SDK's spelling (`ID3D12Device`, `DXGI_FORMAT`, `HRESULT`, `IDXGISwapChain4`) and are never renamed to fit.

**R5 — Template parameters are PascalCase**: `T`, `Fn`, `BlockBytes`, `Ts...`.

**R6 — Units belong in names; types do not.** `durationTicks`, `speedUnitsPerSecond`, `radiusMeters`, `volumePercent` are encouraged — a game measured in ticks, seconds, pixels and distances makes unit ambiguity a real defect class, and it is one the compiler cannot catch for you. Never encode the type: no `iCount`, `pEntity`, `strName`.

**R7 — A file is named for its primary type**, PascalCase, `.h` / `.cpp` only. `.hpp`, `.cc` and `.inl` are not used; template implementations live in the header. Exceptions, because MSBuild and the Visual Studio wizards spell them this way: `pch.h`, `pch.cpp`, `framework.h`, `targetver.h`, `Resource.h`.

**R8 — `m_` marks encapsulated state, not every field.** A `class` with invariants prefixes private members `m_`. A public aggregate — a `Desc` config struct, a wire record, a POD handed to the renderer — uses plain `camelCase` fields so brace initialization reads naturally.

**R9 — One namespace per layer, and the engine does not know the game.** Reusable engine code gets its own namespace, `Neuron` (the NeuronCore, NeuronClient and NeuronServer libraries); game code gets another, `Species` (GameShared, GameLogic, GameClient and the two programs; owner decision O60, ADR-090). The split is a rule rather than a filing preference: if an engine type has to know a game concept by name in order to do its job, it is in the wrong layer. Test suites use `namespace <Project>Tests`.

**R10 — No `using namespace` at file scope in a header.** It leaks into every translation unit that includes it, and the failure it causes appears somewhere else. In a `.cpp` it is allowed for the unit-test framework and nothing else; otherwise qualify the name or write a local alias.

**R11 — One spelling per family, and it is the SDK's.** `color`, `initialize`, `serialize`, `normalize`, `quantize`, `synchronize`, `behavior`, `neighbor`, `center`, `gray`, `canceled`. Neither spelling is wrong English; the defect is a tree where a reader has to know which half they are in and a grep for one finds half the uses. `D3D12_CLEAR_VALUE::Color` settles which half wins. Prose is not checked — a design document may spell `flavour` and `harbour`; an identifier spells `flavor` and `harbor`.

### Worked example — this is the target style

```cpp
// NeuronClient/SceneTarget.h
#pragma once

#include <cstdint>

namespace Neuron
{

// R3: constant → UPPER_CASE. R6: the unit is in the name.
inline constexpr std::uint32_t SCREEN_WIDTH_PIXELS = 1920;
inline constexpr std::uint32_t SCREEN_HEIGHT_PIXELS = 1080;

// Enumerator → PascalCase, unlike the constants above.
enum class TargetFault : std::uint8_t
{
  DeviceRemoved,
  BadFormat,
  OutOfVideoMemory
};

/// The colour framebuffer the game draws into, and the depth buffer that goes with it.
/// R2: no prefix on the type. R8: private state carries m_.
class SceneTarget
{
public:
  struct Desc                                            // R8: aggregate → plain fields
  {
    std::uint32_t widthPixels;                           // R6: unit in the name
    std::uint32_t heightPixels;
    DXGI_FORMAT colorFormat;                             // R4: SDK spelling kept as-is
  };

  [[nodiscard]] static bool Create(ID3D12Device* _device,        // R1: _ on parameters
                                   const Desc& _desc,
                                   SceneTarget& _outTarget) noexcept;

  [[nodiscard]] std::uint32_t WidthPixels() const noexcept { return m_widthPixels; }

private:
  ID3D12Resource* m_depthTarget = nullptr;
  std::uint32_t m_widthPixels = 0;
  bool m_deviceRemoved = false;
};

} // namespace Neuron
```

### Enforcement

| Rule | Enforced by |
|---|---|
| The naming table, R1, R3, R5, R8 | [`.clang-tidy`](.clang-tidy), gated in CI over the whole tree |
| R2 affixes, R7 file names and project registration, R11 spellings, §2 flat directories, shader names and functional filters | `Build/CheckProjectFiles.py`, gated in CI |
| R4, R6, R9, R10 | Review. Check your own diff against the table before handing it back. |

**Neither checker exists yet** (§6). `.clang-tidy` is configured and gates the moment there is a translation unit to run it over; `Build/CheckProjectFiles.py` has to be written, and until it is, the four rules in its row are review's problem and nothing else. A rule nobody can run is a rule that rots, so writing that checker is early work rather than housekeeping.

---

## 2. Repository shape

The concrete layout — the solution, the projects and the edges between them — is ADR-090 (owner decision O60): `Species.slnx` at the repository root; the static libraries NeuronCore (engine code that client and server share), NeuronClient (the engine's client side), NeuronServer (the engine's server side), GameShared (the game's types, messages, records and pure functions), GameLogic (the game simulation, server side only) and GameClient (the game's client side); the thin programs `Species.exe` and `SpeciesServer.exe`; a `<Project>Tests` project per library; and tool projects under `Tools/`. Edges run engine to game and never back, and the headless server never links NeuronClient or GameClient. Every layout change is an amendment of ADR-090, and it satisfies these standing constraints:

**Project directories are flat, with exactly two sanctioned subdirectories.** C++ source — headers and `.cpp` alike — lives directly in its project's folder. **There is no `src/`, no `include/`**, and no other split of a project by file kind. This is not taste: `.clang-tidy`'s `HeaderFilterRegex` matches headers exactly one level in, so **a header in a subdirectory is silently unchecked** — no findings, no warning, and nobody notices for months. The two exceptions are the shader pipeline:

- **`<Lib>/Shader/`** holds the HLSL, hand-written, named for the shader and its stage: `<Shader>VS.hlsl` for a vertex shader, `<Shader>PS.hlsl` for a pixel shader and `<Shader>CS.hlsl` for a compute shader. A permutation is its own file, `<Shader><Permutation><Stage>.hlsl` (for example `TerrainWireVS.hlsl`), and the code a shader's stages and permutations share lives in one include file, `<Shader>.hlsli`, in the same folder. Every `.hlsl` is an `FXCompile` item; an `.hlsli` is never compiled on its own.
- **`<Lib>/CompiledShader/`** holds what the compiler wrote: one header per `.hlsl`, named like its source (`<Shader>VS.h`, `<Shader>PS.h`, `<Shader>CS.h`, `<Shader><Permutation><Stage>.h`), each declaring a byte array of the same name with a `g_` prefix (`g_<Shader>VS`, `g_<Shader><Permutation>PS`). It is **build output** — produced by an `FXCompile` item in the `.vcxproj` on every build, listed in `.gitignore`, skipped by every checker, and never edited or committed. The `.cpp` that binds the pipeline state includes it and nothing else does.

**Shaders are compiled into the executable.** The bytecode reaches the GPU from those generated byte arrays, and nothing else: no `.cso` beside the `.exe`, no shader loaded from disk at runtime, no `D3DCompile` and no `d3dcompiler_47.dll` dependency. A shader change is a rebuild.

**The edges run one way, and a layer never reaches sideways.** Engine code is built on by game code and never the reverse (R9), and two libraries at the same level share what is below them rather than each other. An edge that only exists "for now" is an edge, and it is the one that will be impossible to remove later.

**The project files are part of the source.** Adding, removing or moving a file means editing the owning `.vcxproj` **and** its `.filters`. A file that compiles locally but is missing from the project fails only in CI — or worse, links a stale object nobody notices.

**Filters are functional.** A `.filters` file groups a project by what the code *does* — `Rendering`, `Audio`, `Input`, `Shader` — never by what kind of file it is. The Visual Studio defaults `Source Files`, `Header Files` and `Resource Files` are deleted when a project is created and never come back, and a `.h` sits in the same filter as its `.cpp`.

**There are no vendored SDKs and no package manager beyond NuGet restore.** The game's projects depend on the Windows SDK, the MSVC standard library and the four Microsoft packages R14 allows, which NuGet restores into `packages/` at build time; restored packages are build output and are never committed. See R14.

**Development tools are projects of the one solution, under `Tools/`.** C++ development tools that never ship — the content build tool and its importers — are projects in `Tools/<Project>/`, in a `Tools` solution folder of the one solution at the repository root, so they build with the same `$(SolutionDir)` paths as everything else and may reference the engine projects they share code with. They follow every rule in this file except R14: a tool project may use pinned third-party code (for example a BC7 texture encoder), each dependency recorded in an ADR with its licence and restored by NuGet for that project only. The edge runs one way: no game or engine project references a tool project or a tool-only package, and nothing from a tool project links into the executable. `Build/CheckProjectFiles.py` checks both.

**Build and IDE output is never committed** — `x64/`, `.vs/`, `*.user`, and anything a build step generates.

**Generated source is committed when a developer-run tool produces it.** The schema code generator, a tool project (ADR-062), is not a build step: a developer runs it when a schema changes, and it writes ordinary `.h` files in this file's style directly into the owning project folder. Each starts with a comment naming the schema it came from and saying it is generated; it is never edited by hand, and it is formatted, linted and registered in the `.vcxproj` like any other header. CI regenerates the files and fails on any difference (`Build/CheckGeneratedSchemas.py`), so no game or engine project needs a tool project to build.

---

## 3. Build and verify

**x64 is the only platform.** No Win32/x86 configuration in any project or solution; do not add one, and do not write code that only works at 32 bits.

**The compiler settings are the settings.** Toolset `v145` (Visual Studio 2026), `/std:c++latest`, `/permissive-`, `/W4` with **warnings as errors**, `/fp:precise`, `/arch:AVX2` (R16). There is no CMake. If a build error tempts you to change the toolset, lower the language standard, turn off `/permissive-` or silence a warning — **stop and report instead.**

**Debug and Release are aligned by rule, not by luck.** Every setting that is not *about* optimisation reads identically in both configurations: language standard, conformance, warning level, include directories, precompiled header, floating-point model, instruction set. The two differ in exactly four things — `Optimization`, `_DEBUG` vs `NDEBUG`, `FunctionLevelLinking`/`IntrinsicFunctions`, and the linker's folding and LTCG switches. (MSBuild spells those four through a few more properties — `UseDebugLibraries`, `RuntimeLibrary` as the debug or release CRT, `LinkIncremental`, `WholeProgramOptimization`, `EnableCOMDATFolding`, `OptimizeReferences` — and that list is the whole of what may differ.)

That alignment matters more than it looks, because **CI builds Debug only** (§6). Release is compiled by whoever ships, and a Release that quietly lost an include directory or sat on an older language standard would not be discovered until then. A static check of the two configurations is what stands in for the build nobody runs.

**Build through the solution, never a `.vcxproj` directly.** Output paths and cross-project include directories are anchored on `$(SolutionDir)`, and MSBuild defines `SolutionDir` only for a solution build. Building a project file directly resolves every one of those paths against the *project* folder instead of the repository root. **It does not fail — that is the problem.** Output lands in the wrong folder, so the next solution build links against whichever copy is staler, and every cross-project include path becomes a directory that does not exist. The breakage is latent: it bites the first time a file reaches across projects, which may be weeks after someone got into the habit. To build one project, use `/t:<ProjectName>` on the solution.

```powershell
# Restore the NuGet packages R14 allows (once a project references one).
msbuild <Solution>.slnx /t:Restore /p:RestorePackagesConfig=true /nologo

# Everything, from the repository root, naming the solution.
msbuild <Solution>.slnx /p:Configuration=Debug /p:Platform=x64 /m /v:minimal /nologo

# One project, still through the solution.
msbuild <Solution>.slnx /t:<ProjectName> /p:Configuration=Debug /p:Platform=x64 /m /nologo

# Release, before you claim anything about it.
msbuild <Solution>.slnx /p:Configuration=Release /p:Platform=x64 /m /v:minimal /nologo
```

**A project does not put its own directory on the include path.** `cl.exe` already searches the directory of the including file first for a quoted include, so `#include "FileSys.h"` from a `.cpp` in the same folder resolves without help. Only the directories of *other* projects are listed, as `$(SolutionDir)<Project>`.

**Run the tests**, through `vstest.console.exe`, over every suite the build produced.

**vstest reports "no tests found" as a pass.** An empty suite is therefore worse than no suite: it is a green check mark over a library nobody exercised. Every test project ships a placeholder `SuiteSmoke` for exactly this reason; delete it when the first real test lands, never before.

**Run the checkers before you push.** They are seconds of Python and they are what CI runs:

```powershell
python Build\CheckFormat.py           # clang-format, whole tree. --fix rewrites the offenders
python Build\CheckProjectFiles.py     # build shape, project registration, R2/R7/R11
python Build\RunClangTidy.py          # needs a Developer PowerShell (INCLUDE must be set)
```

**A green build says nothing about whether the game draws.** For anything touching rendering, input, audio or presentation, launch the executable and look at it.

**Report what you actually did.** "Builds clean, not run" and "builds and runs" are different claims. Never imply the second when you only did the first, and say which configurations you built.

---

## 4. Layout and formatting

[`.clang-format`](.clang-format) is the authority for C++ layout: 2-space indent, 140 columns, Allman braces, pointer and reference bound left, includes never reordered. [`.editorconfig`](.editorconfig) covers everything clang-format does not — CRLF, UTF-8, final newline, trailing whitespace, and the non-C++ formats — and repeats the two numbers an editor needs before the first save.

**This tree is formatted, and CI keeps it that way.** A whole-tree format check here is a no-op. Format what you write; if the check fires, run `--fix` and commit the result rather than arguing with it.

- **Do not reformat what your task did not touch.** The check being green tree-wide means a drive-by reformat produces pure churn and buries your actual change.
- **Include order is load-bearing and grouped by hand**, which is why `SortIncludes` is `Never`: `pch.h`, then `<windows.h>` before any D3D12/DXGI/XAudio2 header, then the rest of the SDK, then project headers, then the standard library. A formatter reordering these behind a change's back is a correctness risk, not a style preference.
- **One header owns the Windows macro family, and nothing else defines any of it.** `NOMINMAX`, `WIN32_LEAN_AND_MEAN`, `NOMCX`, `NOSERVICE`, `NOHELP` are set in that one header, before `<windows.h>`, and the project files deliberately define none of them. Two owners of one macro is C4005, and `/WX` makes that fatal — `/D` spells a bare macro as `1` where a `#define` spells it as nothing, so the collision is guaranteed rather than possible. If you need `<windows.h>`, include that header; do not add the macros yourself.
- Do not silence a diagnostic with `#pragma warning(disable: ...)` to make a build pass. Fix the cause, or report it.

---

## 5. Rules for this codebase

**R12 — Graphics is Direct3D 12**, through the DirectX Agility SDK (R14). COM lifetimes are RAII from the first line — a raw `AddRef`/`Release` pair in new code is a defect, not a style. How the renderer is shaped — passes, render targets, resolution, scaling, window style, multisampling, text — is decided in the baseline ADRs (`Design/ADR/`, ADR-001 to ADR-016) and specified in `Design/architecture/components/renderer/`; a change to it is a new or amended ADR (§6).

**R14 — No third-party dependencies and no package manager, except four Microsoft packages.** The executable is built from the Windows SDK, the MSVC standard library and this allow-list, and nothing else:

| Package (NuGet) | What it provides | Why it is allowed |
|---|---|---|
| `Microsoft.Direct3D.D3D12` (DirectX Agility SDK) | the current Direct3D 12 runtime (`D3D12Core.dll`, installed beside the executable) and its headers | one, current Direct3D 12 runtime on every supported Windows build (ADR-001) |
| `Microsoft.GameInput` | the GameInput header, library and the `GameInputRedist.msi` the installer runs | the supported controller API and its updated runtime (ADR-028) |
| `WinPixEventRuntime` | `pix3.h` and `WinPixEventRuntime.dll` | PIX markers and captures (ADR-068) |
| `Microsoft.Native.Quic.MsQuic.Schannel` (MsQuic) | the MsQuic API header and `msquic.dll`, installed beside the executable, on the operating system's Schannel TLS 1.3; MIT, its licence notice shipped with it | the client–server transport: reliable streams, unreliable datagrams and TLS 1.3, with no protocol of Species' own (owner decision O44; the package ADR-089, the transport ADR-088) |

The pinned versions are in the technology baseline of `Design/ADR/README.md`. If you believe anything else is unavoidable, propose it in your report with what it buys and what it costs — do not add it. This is a closed list, not a high bar; adding to it is an owner decision recorded as an ADR.

**It binds what the executable is built from, not what a development tool needs.** Scripts under `Build/` and `Tools/`, and the tool projects under `Tools/` (§2), never ship and never link into the executable, so a baker that needs Pillow, or a cooker that needs a texture encoder, does not reopen this rule. **Third-party *content* is a different question and it is the owner's**: art, fonts and sound are allowed, and anything under a licence needs the owner's approval before it lands, with the licence text travelling with the bytes.

For Direct3D the list means what the Windows SDK and the Agility SDK install: `d3d12.h`, `dxgi1_6.h`, `DirectXMath.h`, `wrl/client.h` (`Microsoft::WRL::ComPtr` is the COM smart pointer R12 asks for) and the `fxc`/`dxc` compilers of the Windows SDK that `FXCompile` drives. It excludes what a D3D12 sample reaches for by reflex: the `d3dx12.h` helpers (even where a package carries them), DirectX-Headers, DirectXTK12, DirectXTex, and the DirectX Shader Compiler as a redistributable. Resource barriers and heap descriptions are written by hand.

**R15 — Memory is plain C++.** `new`/`delete` where it must be, RAII everywhere, standard containers by default. No pool, slab or free-list allocator without a decision recorded in `Design/ADR/`.

**R16 — The floating-point model and instruction set are stated, not inherited.** Every project compiles `/fp:precise` and **`/arch:AVX2`**, stated explicitly in the project file rather than inherited from an MSVC default — a default is not a decision, and the symptom of losing one is two builds of the same code disagreeing about the same sum with no line to blame. Both settings are identical in Debug and Release.

**What `/arch:AVX2` costs is named rather than waved at.** It sets an AVX2 floor — Intel Haswell (2013) and AMD Excavator (2015); an older CPU meets an illegal instruction, not a message. And it lets MSVC contract `a*b+c` into an FMA even under `/fp:precise`, which changes float results, and may contract differently at different optimisation levels. Float code that must produce bit-identical results across builds cannot rely on it.

**If the game needs a deterministic core** — a simulation that replays, lockstep networking, a result that must reproduce from a seed — that is a decision recorded as an ADR, and inside that core: no `float` where a fixed-point or integer quantity will do (hold a fraction as integer hundredths and say so in the name, R6), no iteration over an unordered container whose order reaches the outcome, no wall-clock time (a tick is the clock, and wall time maps to ticks at one seam), and randomness from a pinned PRNG with a recorded seed — never `std::random_device`, never a hash of an address.

**R17 — A string you do not write is `const`.** `/permissive-` turns on `/Zc:strictStrings`: a literal is `const char[N]` and will not bind to `char*`. The fix is `const` on the signature, never a cast at the call site — a `const_cast` here is a lie about a literal that lives in a read-only section, and writing through it is a real crash rather than a theoretical one.

**R18 and up are reserved.** A design document does not only say what to build; some of what it says constrains how the code is *shaped* — which state a decision routine may read, what an emitted event has to carry with it, where tuning values live. Those are conformance rules with a design source, and they are written here as R18 onward when there is a design to cite, without renumbering anything above. Until then, do not invent one and do not import one from another tree: a rule with no source behind it is a rule nobody can settle an argument with.


The reference library already states rules of this kind, and the design phase decides which become R18 onward when it writes the Species design. The candidates are: the presentation layer never writes simulation state; the simulation core is deterministic (the tick is the clock, randomness comes from named streams with recorded seeds, iteration order never depends on allocation history: `Design/reference/12_simulation_model/` and the numeric-model decisions DEC-240 and DEC-241); and every tuning value lives in data with a stable key, never as a literal in code (`Design/CONVENTIONS.md` §4). Until the design phase writes them here, treat them as the specification of the component that states them, not as conformance rules.
---

## 6. Working rules

**Stay in scope.** Do what the task asks. Adjacent code that offends you is not part of the task — note it in your report and move on. Unrequested "while I was in there" changes are the main way a young tree acquires regressions it cannot bisect.

**Record decisions as ADRs.** An engineering decision — a file format, a wire protocol, a subsystem's shape, an exception to a rule here — goes in `Design/ADR/` as one file per decision, `ADR-NNN-<slug>.md`, numbered in order from the highest existing id (the baseline ends at ADR-078), in the MADR 4 form the baseline uses (context, options, decision, consequences: what it forecloses), in the same commit as the change that implements it. A change to a baseline ADR is an amendment note in that ADR, never a silent edit. Figures in an ADR are measured, not estimated — if you quote one, say how you measured it. A decision nobody wrote down gets re-litigated every few months by whoever forgot it. Design questions (what the game does, as opposed to how it is built) go in the decision register, `Design/decisions/`, as `DEC-NNN` entries.

**Run the design checks on every change under `Design/`.** `python Tools/DesignCheck/CheckDesign.py Design` checks links, requirement ids, decision and ADR references, asset ids, tuning keys, sizes, templates and banned terms; it must pass before a design change is pushed.

**Write the checkers early.** `Build/CheckFormat.py`, `Build/CheckProjectFiles.py` and `Build/RunClangTidy.py` are what §1, §2 and §3 lean on, and **none of them exists yet.** Until each one lands, the rules it would enforce are review's problem — which is exactly why they are early work rather than housekeeping.

**What CI runs.** [`.github/workflows/build.yml`](.github/workflows/build.yml) has three jobs: a Windows job that checks the build shape, restores the NuGet packages R14 allows, builds **Debug|x64**, runs the test suites and then clang-tidy; a Linux job that checks formatting on a pinned clang-format; and a Linux job that runs the design checks over `Design/` (§6, "Run the design checks"). **Every step that has something to run blocks; a step whose input does not exist yet is skipped, not faked.** Each gate is guarded on the file it needs — the checker script, the solution, the built test DLLs — so the workflow is honest about today's empty tree and starts gating the moment that file lands. The guards are the only concession: nothing is `continue-on-error`, and a script that exists and fails still fails the build. Remove a guard once its input is permanently there, not before, and never add one to get past a red build.

**CI does not build Release.** The Windows build is the slow half of the pipeline and a second configuration roughly doubles it for a tree where the two differ only in optimisation. What stands in for it is the static alignment check on the two configurations (§3) — and, before a release, an actual `Configuration=Release` build by whoever is shipping. If you change something that could plausibly break only under optimisation, build Release yourself and say so.

**Commits and PRs.** Branch off `main`; small, focused commits with an imperative subject describing the change, not the process. One change per PR. CI must be green. Never commit build output, `.vs/` or `.user` files.

---

## 7. Before you hand work back

- [ ] Naming conforms to §1 — `_` on parameters, `m_` on class state, `UPPER_CASE` constants, `PascalCase` enumerators, no `I`/`C`/`Base` affixes.
- [ ] Only the lines the task required were changed; no reformatting, no drive-by fixes.
- [ ] New, removed or moved files are in the `.vcxproj` **and** the `.filters` of every project involved.
- [ ] No project's `ConformanceMode`, `LanguageStandard`, `WarningLevel` or `TreatWarningAsError` was changed, and no warning was silenced with a pragma.
- [ ] Debug and Release still agree on everything §3 says they must.
- [ ] No new third-party dependency (R14).
- [ ] The checkers pass — or, for one not yet written, the report says which and why.
- [ ] It builds Debug|x64, and every test suite runs and passes.
- [ ] If it touches rendering, input, audio or presentation: it was **run**, not just built.
- [ ] `Design/ADR/` has a new file if the change *was* a decision.
- [ ] If it touches `Design/`: `python Tools/DesignCheck/CheckDesign.py Design` passes.
- [ ] Your report states plainly what you verified, what you assumed, and any rule here you had to bend.
