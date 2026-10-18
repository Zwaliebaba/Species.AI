---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead; settings decided by the owner's AGENTS.md (O21)
---

# Windows toolchain: Visual Studio 2026 toolset v145, the pinned Windows SDK and the AGENTS.md compiler settings

## Context and Problem Statement

Species builds Windows x64 programs only: the client, the dedicated server, the content build tool and the test suites ([ADR-056](ADR-056-products-configurations-and-feature-flags.md)), with MSBuild ([ADR-054](ADR-054-build-system-msbuild-solution.md)). Every machine must compile with the same settings, with debugging symbols for every build and with the exploit mitigations Windows offers. The repository's engineering rules fix the toolset, the language mode, the conformance and warning settings, the floating-point model and the instruction set, and require them to be identical in Debug and Release (`AGENTS.md` §3, R16). A dedicated server for another operating system is not part of this baseline; it needs a later ADR.

Which compiler, toolset, SDK and compiler and linker settings does Species use?

## Decision Drivers

* The settings of `AGENTS.md` §3 and R16, stated explicitly in every project and identical in both configurations.
* Symbols for every build, including the Release builds players run, for crash analysis ([ADR-065](ADR-065-crash-reporting.md)).
* Exploit mitigations on every distributed binary.
* A floating-point model and instruction set that are stated, never inherited from a default.

## Considered Options

* MSVC with the `v145` toolset and the `AGENTS.md` settings
* clang-cl (the LLVM compiler with the MSVC driver) for some or all projects

## Decision Outcome

Chosen option: "MSVC with the `v145` toolset and the `AGENTS.md` settings", because it is the compiler `AGENTS.md` fixes and the reference compiler of the Windows SDK, the Agility SDK and PIX.

* **IDE and build tools:** Visual Studio 2026 (18.11.0 is current) for development; the Visual Studio of the CI runner image on CI agents.
* **Toolset:** `PlatformToolset` `v145` in every project. Visual Studio 2026 ships it with the MSVC Build Tools 14.50 and later; 14.51 is the default toolset since May 2026 and 14.52 is expected to follow in November 2026. The MSVC minor version is not pinned: CI prints the toolchain it got, and a pin is taken from that evidence when one is needed ([ADR-070](ADR-070-continuous-integration.md)). A build error is never solved by changing the toolset, lowering the language standard, turning off `/permissive-` or silencing a warning.
* **Windows SDK:** one Windows SDK, at least the release for Windows 11 version 24H2 (the OS floor of [ADR-050](ADR-050-client-platform-and-minimum-os.md)). The pin is **10.0.26100.9457** (September 2026), the latest release of the 10.0.26100 series, which is the series of the OS floor and of the primary test targets 25H2 and 26H2; `WindowsTargetPlatformVersion` names that series in every project. The 10.0.28000 series (Windows 11 26H1) is not needed, because no Species component needs an API that only that series provides; the pin moves to a later release at each milestone re-check. The SDK also provides the shader compiler ([ADR-003](ADR-003-shader-language-compiler-and-material-shaders.md)). Agility SDK headers are included before Windows SDK headers.
* **Compiler settings, identical in Debug and Release:** `/std:c++latest` ([ADR-052](ADR-052-cpp23-language-standard.md)); `/permissive-` (which also gives `/Zc:strictStrings`, `AGENTS.md` R17); `/W4` with warnings as errors (`/WX`) in every build; `/fp:precise`; `/arch:AVX2`; and `/Zc:__cplusplus`, `/Zc:preprocessor`, `/utf-8`, `/EHsc`, `/Zi` (a program database; never `/ZI`, which `/guard:cf` excludes), `/guard:cf`, `/GS`, one precompiled header `pch.h` per project, and the dynamic C runtime (`/MDd` in Debug and `/MD` in Release, the runtime-library difference that `AGENTS.md` §3 allows). No warning is silenced with a `#pragma warning(disable)` directive.
* **Linker settings:** in both configurations `/DEBUG:FULL` (a PDB for every build), `/DYNAMICBASE`, `/HIGHENTROPYVA`, `/NXCOMPAT`, `/CETCOMPAT` and `/GUARD:CF`; in Release only `/OPT:REF`, `/OPT:ICF` and link-time code generation, and in Debug only incremental linking, which are the folding, whole-program and incremental differences that `AGENTS.md` §3 allows.
* **Floating point and instruction set:** `/fp:precise` and `/arch:AVX2` are stated in every project file (`AGENTS.md` R16). `/arch:AVX2` sets the CPU floor of [ADR-050](ADR-050-client-platform-and-minimum-os.md), and it lets MSVC contract `a*b+c` into a fused multiply-add even under `/fp:precise`, which changes float results and may contract differently at different optimisation levels. Float code that must give bit-identical results across builds cannot rely on it; a deterministic simulation core is a decision recorded as an ADR with the numeric model (DEC-240, DEC-241).
* **Assertions:** `NDEBUG` is defined in Release; development checks use `ENGINE_ASSERT`, which [ADR-056](ADR-056-products-configurations-and-feature-flags.md) compiles into Debug only; release checks stay in both.
* **Static analysis:** clang-tidy, at the version pinned in the CI workflow, runs over the whole tree, and clang-format checks the layout ([ADR-070](ADR-070-continuous-integration.md)). There is no analysis configuration and no clang-cl build; a sanitiser or fuzzing build is a project of its own ([ADR-069](ADR-069-test-framework.md)).
* **Other compilers:** none in this baseline. A dedicated server for another operating system needs a later ADR, which chooses its compiler; the portable subset of [ADR-052](ADR-052-cpp23-language-standard.md) is guidance for it.

### Consequences

* Good, because one compiler builds every program, with settings that are the same in both configurations and checkable from the project files.
* Good, because every crash can be symbolised and every binary carries the standard mitigations.
* Bad, because the dynamic CRT requires the Visual C++ runtime on the player's machine (handled by [ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md)).
* Bad, because `/arch:AVX2` excludes older CPUs ([ADR-050](ADR-050-client-platform-and-minimum-os.md)), and its contraction of multiply-adds constrains any bit-exact float code.
* Bad, because the toolset minor version follows the CI image, so a build can change without a code change; printing the toolchain in every CI run makes such a change traceable.

### Confirmation

* `Build\CheckProjectFiles.py` checks that every project uses `v145`, x64 only, and the settings above, identical in Debug and Release except where `AGENTS.md` §3 allows a difference.
* CI builds `Debug|x64` with warnings as errors ([ADR-070](ADR-070-continuous-integration.md)).
* Before a release, every distributed PE file is checked for the CFG, CET, ASLR and NX flags and for a matching PDB in the symbol store ([ADR-065](ADR-065-crash-reporting.md)).

## Pros and Cons of the Options

### MSVC `v145` with the `AGENTS.md` settings

* Good, because it matches the vendor toolchain of every Windows and DirectX dependency and the compiler `AGENTS.md` fixes.
* Neutral, because portability is not checked by a second front end; clang-tidy parses the tree with Clang, which catches some non-portable code.

### clang-cl for some or all projects

* Good, because one front end could serve Windows and a possible non-Windows server.
* Bad, because `AGENTS.md` fixes MSVC and its settings, and a second compiler doubles the build matrix.

## More Information

* Rules: `AGENTS.md` §3 (toolset and settings, Debug and Release alignment), §4 (no silenced warnings), R16 (floating point and instruction set), R17 (strict strings).
* Facts and sources: technology baseline items 2.1, 2.2, 2.4 and 4.1 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17); Visual Studio 2026 ships the `v145` platform toolset (https://learn.microsoft.com/cpp/overview/what-s-new-for-msvc · https://learn.microsoft.com/cpp/overview/compiler-versions); `/guard:cf` is incompatible with `/ZI` (https://learn.microsoft.com/cpp/build/reference/guard-enable-control-flow-guard).
* The Windows SDK pin is recorded by the build tooling component (`bld.pin.windows_sdk`).
* Component: build tooling and distribution (`BLD`).
* **Amendment 2026-10-17.** The Windows SDK release is verified and added to the technology baseline (item 2.4): 10.0.26100.9457.
* **Amendment 2026-10-17 (AGENTS.md, O21).** The toolset is `v145`, with `/std:c++latest`, `/permissive-`, `/W4` and warnings as errors in every build, `/fp:precise` and `/arch:AVX2`, all identical in Debug and Release; x64 only. The MSVC minor-version pin, the clang-cl analysis build and the non-Windows server compilers are removed; a dedicated server for another operating system needs a later ADR.
