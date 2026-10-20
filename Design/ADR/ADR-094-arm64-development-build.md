---
status: accepted
date: 2026-10-20
decision-makers: owner (O66); Species technology lead
---

# ARM64 development build: a second platform for the owner's PC, not a shipping target

## Context and Problem Statement

The owner's development PC is Windows 11 on Arm, with a Qualcomm Snapdragon X processor and an Adreno X1-85 GPU. The x64 build runs there only under emulation: `Species.exe`, the test host and the compiler itself. `AGENTS.md` §3, [ADR-050](ADR-050-client-platform-and-minimum-os.md), [ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md) and [ADR-054](ADR-054-build-system-msbuild-solution.md) all say x64 only.

On 2026-10-20 the owner added `Debug|ARM64` and `Release|ARM64` to the solution and to every project. The owner asked for `/arch:armv8.7` in place of `/arch:AVX2` on ARM64, and decided that ARM64 is a development build and not a shipping target (O66).

The first ARM64 build showed what else this needs:
- **Output folders.** Every project wrote both platforms to `x64\<Configuration>\`, so an x64 build after an ARM64 build loaded the ARM64 precompiled header and failed with C3859.
- **CET.** The linker refuses `/CETCOMPAT` for ARM64 (LNK1246), because CET shadow stacks are an x64 feature.
- **Floating-point environment.** The code of PLT-087 used the SSE intrinsics of `<immintrin.h>`, which do not compile for ARM64 (C1189).
- **Checkers.** `Build/CheckProjectFiles.py` and `Build/SolutionModel.py` evaluated x64 conditions only, and reported every ARM64 group as a condition they do not evaluate.

## Decision Drivers

* The owner builds, runs and debugs natively on their own PC.
* x64 stays the only platform that players get; nothing about ADR-050's client changes.
* Both platforms follow the same rules, and differ only where the hardware forces it.
* The checkers see every configuration, so neither platform can drift unseen.
* CI time does not grow.

## Considered Options

* 1 — ARM64 as a development build beside x64
* 2 — x64 only, run under emulation on the owner's PC
* 3 — ARM64EC, which mixes ARM64 and x64 code in one process
* 4 — ARM64 as a second shipping platform

## Decision Outcome

Chosen option: "1", by the owner (O66). Option 2 keeps the compiler, the tests and the game under emulation on the only PC that runs the game. Option 3 exists to load x64 binaries into an Arm process, and Species has none to load. Option 4 adds a player platform, with its own CPU floor and test matrix, which nothing in the design asks for.

* **Configurations.** The solution lists the platforms `x64` and `ARM64`. Every project has exactly `Debug|x64`, `Release|x64`, `Debug|ARM64` and `Release|ARM64`.
* **Instruction set (R16).**
  - The shared compile settings keep `AdvancedVectorExtensions2` (`/arch:AVX2`).
  - Each ARM64 configuration states `CPUExtensionRequirementsARMv87` (`/arch:armv8.7`), which overrides it.
  - `/fp:precise` and every other compile setting are the same on both platforms.
* **CET.** ARM64 links with `CETCompat` false; x64 keeps `/CETCOMPAT`. Every other link setting is the same on both platforms.
* **Output folders.** Every project writes to `$(SolutionDir)$(Platform)\$(Configuration)\`, with its intermediate files beneath. The x64 paths are unchanged, so CI and every script that reads `x64\Debug\` are unaffected.
* **Packages (R14).** The same four packages serve both platforms. Their build files copy the ARM64 binaries of `D3D12Core.dll`, `D3D12SDKLayers.dll`, `msquic.dll` and `WinPixEventRuntime.dll` beside an ARM64 build.
* **Source.** The source has no architecture-specific code.
  - The floating-point environment of PLT-087 is read and set through the CRT's `_controlfp_s`, over the rounding and denormal fields (`_MCW_RC`, `_MCW_DN`) and the masks of the five IEEE exceptions, in place of the SSE intrinsics. The CRT maps these fields to MXCSR on x64 and to FPCR on ARM64.
  - The mask of the denormal-operand exception, an x86 extension, is no longer part of the check: `_controlfp_s` does not set it, the CRT does not map it on ARM64, and nothing in the tree unmasks it. The CRT function that would set it is a superseded technology that the design check rejects.
  - Code that needs an architecture `#if` amends this record.
* **Checkers.**
  - `Build/SolutionModel.py` reads settings per configuration and platform; a query reads x64 unless it names a platform.
  - `Build/CheckProjectFiles.py` rule 1 requires the four configurations.
  - Rule 2 holds Debug and Release together on each platform, and x64 and ARM64 together in each configuration, except the instruction set and `CETCompat`. A fixture puts an ARM64 configuration on AVX2 and must be reported.
  - Rules 8, 9 and 11 check every configuration of both platforms.
  - The checkers skip `ARM64\` as they skip `x64\`.
  - `Build/RunClangTidy.py` lints the x64 configuration, whose sources and settings are the ARM64 configuration's except the instruction set.
* **CI.** CI builds and tests `Debug|x64` only, as before. The project-file checker is CI's static check of the ARM64 configurations, as it is for Release, and the owner builds and runs ARM64 on their PC.

### Consequences

* Good, because the owner builds, tests, runs and debugs natively. The x64 build stays available on the same PC under emulation.
* Good, because the floating-point environment code is portable, and its two PLT-087 tests pass on both platforms.
* Good, because a setting that drifts between the platforms fails the checker in CI, although CI builds no ARM64.
* Bad, because CI does not compile ARM64. A break that only ARM64 shows, such as an x86 intrinsic or an x64-only switch, surfaces first on the owner's PC.
* Bad, because `/arch:armv8.7` sets a floor for the development build. An Arm CPU that implements less than Armv8.7-A may meet an illegal instruction, as an older x64 CPU does under `/arch:AVX2`.
* Neutral, because the determinism levels and golden hashes of [ADR-081](ADR-081-numeric-model-and-determinism-scope.md) belong to the x64 build. ARM64 is not one of its builds, and agreement between the two is not required.
* It forecloses: shipping an ARM64 build without a new owner decision and an amendment of ADR-050; a platform-specific setting outside the two that rule 2 allows; architecture-specific source without an amendment of this record.

### Confirmation

Measured on the owner's PC on 2026-10-20, with Visual Studio 2026 18.11 and MSVC 14.51:
* `Debug|ARM64`, `Debug|x64`, `Release|ARM64` and `Release|x64` build through `Species.slnx` with no warning.
* All 76 tests pass on both platforms:
  - ARM64 natively, through `vstest.console.arm64.exe`;
  - x64 under emulation, through `vstest.console.exe`.
* `dumpbin /headers` reports `AA64 machine (ARM64)` for both programs, the six test suites and the four runtime DLLs copied beside them.
* The ARM64 `Species.exe` creates its window, its device on the Adreno X1-85 and its swap chain, runs until it is closed, and exits with code 0, with no debug-layer message in its log.
* `Build/CheckProjectFiles.py` passes with 0 findings, and each of its 33 fixtures is reported under its rule. `Build/RunClangTidy.py` reports no finding in 69 translation units.

## Pros and Cons of the Options

### 1 — a development build

* Good, because the owner works natively, and x64 remains the only shipping build.
* Bad, because the second platform is checked statically in CI but not compiled there.

### 2 — x64 only, under emulation

* Good, because there is one platform and nothing to keep aligned.
* Bad, because the compiler, the tests and the game all run emulated on the only machine that runs the game.

### 3 — ARM64EC

* Good, because ARM64EC code can load x64 DLLs in one process.
* Bad, because Species loads no x64-only binary, so it adds an ABI and buys nothing.

### 4 — a second shipping platform

* Good, because players on Windows on Arm would get a native build.
* Bad, because it adds a CPU floor, a test matrix and installer work that no design asks for.

## More Information

* Related: [ADR-050](ADR-050-client-platform-and-minimum-os.md), [ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md), [ADR-054](ADR-054-build-system-msbuild-solution.md), [ADR-055](ADR-055-dependencies-three-nuget-packages.md), [ADR-070](ADR-070-continuous-integration.md), [ADR-081](ADR-081-numeric-model-and-determinism-scope.md); owner decision O66.
* Component: build tooling and distribution (`BLD`), whose table 2.1-B, BLD-030 and BLD-040 carry an amendment note to this record.
* **Accepted 2026-10-20, at the end of M0.** The Confirmation holds on `b8c4d2e`: the four configurations build with no warning, and 137 tests pass in Debug and 136 in Release on both platforms (`playtests/M0.md`).
