---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Windows toolchain: pinned MSVC Build Tools, Windows SDK and compiler policy

## Context and Problem Statement

Species builds a Windows x64 client, content tools and a headless dedicated server from one codebase (vision O5, O10). Every developer machine and every CI agent must produce the same binaries, with debugging symbols for every build and with the exploit mitigations Windows offers. The simulation and server code must stay compilable by a second compiler front end, because the design phase may add a dedicated-server build for Linux hosts; this ADR keeps that option open without deciding it.

Which compiler, toolset version, SDKs and compiler and linker policy does Species use on Windows?

## Decision Drivers

* Reproducible builds: one toolset version across all machines, changed only deliberately.
* C++23 support (see [ADR-052](ADR-052-cpp23-language-standard.md)) and Direct3D 12 through the Agility SDK.
* Symbols for every build, including shipping builds, for crash analysis ([ADR-065](ADR-065-crash-reporting.md)).
* Exploit mitigations on every distributed binary.
* Portability of shared and server code to a second compiler front end.

## Considered Options

* MSVC Build Tools, one pinned version, with clang-cl as a second front end for analysis builds only
* MSVC for the client and clang-cl for the server and simulation code
* clang-cl for every target

## Decision Outcome

Chosen option: "MSVC Build Tools, one pinned version, with clang-cl as a second front end for analysis builds only", because MSVC is the reference compiler for the Windows SDK, the Agility SDK and PIX, and building all simulation binaries with one compiler avoids two code generators for the same rules.

* **IDE and build tools:** Visual Studio 2026 (18.11.0 is current) for development; the Visual Studio Build Tools edition on CI agents.
* **Toolset pin:** MSVC Build Tools **14.51** now. Move to **14.52** when it becomes the default toolset (Microsoft expects November 2026), because `/std:c++23` arrives with it. Toolset 14.50 (long-term support to November 2028) is the fallback only if a C++23-preview ABI change breaks the build. The pin lives in one place, the toolchain preset of [ADR-054](ADR-054-build-system-cmake-presets.md), and CI honours it through `VCToolsVersion`.
* **Windows SDK:** one pinned Windows SDK release, at least the release for Windows 11 version 24H2 (the OS floor of [ADR-050](ADR-050-client-platform-and-minimum-os.md)). The pin is **10.0.26100.9457** (September 2026), the latest release of the 10.0.26100 series, which is the series of the OS floor and of the primary test targets 25H2 and 26H2. The 10.0.28000 series (Windows 11 26H1) is not needed, because no Species component needs an API that only that series provides; the pin moves to a later release at each milestone re-check. Agility SDK headers are included before Windows SDK headers.
* **Always-on compiler options:** `/permissive-`, `/Zc:__cplusplus`, `/Zc:preprocessor`, `/utf-8`, `/W4`, `/EHsc`, `/Zi`; `/WX` on CI; `/guard:cf`; `/GS`; `/MD` (dynamic CRT, one CRT instance across all modules).
* **Always-on linker options:** `/DEBUG:FULL` (a PDB for every configuration), `/DYNAMICBASE`, `/HIGHENTROPYVA`, `/NXCOMPAT`, `/CETCOMPAT`, `/guard:cf`; `/OPT:REF` and `/OPT:ICF` in optimised configurations.
* **Assertions:** `NDEBUG` is defined in the profile and shipping configurations; development checks use Species assertion macros controlled by the build flavour ([ADR-056](ADR-056-editions-build-flavours-and-feature-flags.md)), never by `NDEBUG` alone.
* **Floating point:** this ADR fixes no floating-point option for simulation code. The numeric model is a design-phase decision; the build exposes the floating-point mode as a per-target property so that the decision applies to the simulation targets alone.
* **Analysis builds:** a clang-cl configuration (Ninja, clang-cl 19.1 or newer for module scanning) runs static analysis and compiles the shared and server code as a portability check. Its binaries are never distributed. `/analyze` and AddressSanitizer (`/fsanitize=address`) run in CI test configurations ([ADR-070](ADR-070-continuous-integration.md)).

### Consequences

* Good, because one compiler builds every distributed binary, and the pin makes builds repeatable.
* Good, because every crash can be symbolised and every binary carries the standard mitigations.
* Good, because the clang-cl portability build keeps a non-MSVC server build possible.
* Bad, because the dynamic CRT requires the Visual C++ runtime on the player's machine (handled by [ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md)).
* Bad, because 14.51 support ends in February 2027, so the move to 14.52 is planned work, not optional.

### Confirmation

* Configure fails when `CMAKE_CXX_COMPILER_VERSION` does not match the pinned toolset.
* A CI step checks every distributed PE file for the CFG, CET, ASLR and NX flags and for a matching PDB in the symbol store.
* The clang-cl analysis job is a required check on every change.

## Pros and Cons of the Options

### MSVC pinned, clang-cl for analysis only

* Good, because it matches the vendor toolchain of every Windows and DirectX dependency.
* Good, because a second front end still catches non-portable code.
* Neutral, because analysis builds take CI time but no release effort.

### MSVC for the client, clang-cl for server and simulation code

* Good, because the server code would be built by the same front-end family a portable server build might use.
* Bad, because an integrated server runs in the client process (vision O10), so the client would mix two compilers' simulation code.

### clang-cl for every target

* Good, because one front end would serve Windows and a possible non-Windows server.
* Bad, because PIX, the Windows SDK samples and Microsoft's C++23 conformance work target MSVC first, and clang-cl cannot build `import std` under Visual Studio generators.

## More Information

* Facts and sources: technology baseline items 2.1, 2.2, 2.3, 2.4 and 4.1 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the Visual Studio 2026 release notes, the MSVC compiler-versions page, the Windows SDK overview, downloads and release-notes pages, and the Agility SDK getting-started page.
* The Windows SDK pin is recorded in the toolchain preset by the build tooling component (`bld.pin.windows_sdk`).
* Component: build tooling and distribution (`BLD`).
* **Amendment 2026-10-17.** The Windows SDK release is verified and added to the technology baseline (item 2.4): 10.0.26100.9457.
