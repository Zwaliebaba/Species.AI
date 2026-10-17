---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Third-party dependencies: vcpkg in manifest mode with pinned baseline and overlay triplets

## Context and Problem Statement

Species depends on third-party libraries. The baseline set is: the DirectX headers, DirectXTex, the PIX event runtime, GameInput, a Zstandard codec, a JSON parser and schema validator, Opus and a test framework; the component specifications add to it through this ADR. Each must be built with the pinned toolset, the same C++ mode and the same C runtime as Species itself ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md), [ADR-052](ADR-052-cpp23-language-standard.md)), and every developer and CI agent must get exactly the same versions.

How does Species declare, build and pin third-party dependencies?

## Decision Drivers

* Exact, reviewable version pins checked in with the code.
* Every dependency rebuilt with the Species toolset and CRT settings.
* Fast CI through binary caching.
* Integration with CMake presets ([ADR-054](ADR-054-build-system-cmake-presets.md)).
* A route to a non-Windows server build.

## Considered Options

* vcpkg in manifest mode
* vcpkg in classic mode
* Conan
* Vendored sources or Git submodules built by the Species CMake project

## Decision Outcome

Chosen option: "vcpkg in manifest mode", because it is the vendor-recommended mode for versioned dependencies, integrates with CMake through a toolchain file, and builds every dependency from source with Species' own triplets.

* **Manifest:** `vcpkg.json` and `vcpkg-configuration.json` at the repository root. `builtin-baseline` pins the registry revision; `overrides` pin exact versions where a library must not move with the baseline; `vcpkg x-update-baseline` moves the baseline in a reviewed change. Classic mode is never used.
* **vcpkg itself:** pinned as a Git submodule or a pinned checkout on CI agents. The latest registry release tag on the run date is 2026.07.29.
* **Toolchain integration:** the presets set `toolchainFile` to vcpkg's `scripts/buildsystems/vcpkg.cmake`, and set `VCPKG_TARGET_TRIPLET`, `VCPKG_HOST_TRIPLET` and `VCPKG_MANIFEST_DIR` in `cacheVariables`, before the first `project()`.
* **Overlay triplet** `x64-windows-species`: static libraries, dynamic CRT (`/MD`), the pinned toolset version recorded in the triplet so that a toolset change invalidates every cached binary. A non-Windows server triplet follows the same rule if the design phase adds such a build.
* **Binary caching:** CI uses vcpkg binary caching keyed by vcpkg's ABI hash. The cache host is an infrastructure choice outside this ADR.
* **Vendor packages:** DirectX packages come from vcpkg where a port exists (for example `directx-headers`), otherwise from the vendor NuGet package at a pinned version: Agility SDK `Microsoft.Direct3D.D3D12` 1.619.6, WinPixEventRuntime 1.0.240308001, and the GameInput package pinned by the input ADR.
* **Licences:** the build produces a list of every dependency with its licence; adding a dependency requires a licence review.

### Consequences

* Good, because versions are pinned in the repository and reviewed like code.
* Good, because every dependency is built with Species' toolset, C++ mode and CRT, which the C++23 preview ABI requires.
* Bad, because a cold build compiles every dependency; binary caching is required for acceptable CI times.
* Bad, because NuGet-only vendor packages are a second package source with their own pinning.

### Confirmation

* A CI check fails when `vcpkg.json` lacks `builtin-baseline` or when classic-mode installs are detected.
* A CI check compares the toolset version in the triplet with the toolchain pin.
* The dependency licence list is an artefact of every release build.

## Pros and Cons of the Options

### vcpkg manifest mode

* Good, because versioning, overrides, registries and overlay triplets require manifest mode and are all needed.
* Neutral, because the vcpkg tool itself must be pinned separately.

### vcpkg classic mode

* Bad, because it ignores versioning information and installs into a shared tree outside the repository.

### Conan

* Good, because it is a mature, cross-platform package manager with binary repositories.
* Bad, because the DirectX family of libraries is published for vcpkg and NuGet, not as Conan recipes maintained by the vendor.

### Vendored sources or submodules

* Good, because there is no external tool.
* Bad, because Species would maintain every dependency's build script and upgrade path by hand.

## More Information

* Facts and sources: technology baseline item 3.2 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the vcpkg manifest-mode, versioning, CMake-integration and lock-package-versions pages on learn.microsoft.com, and the vcpkg tags on GitHub. The `directx-headers` vcpkg port is named on https://learn.microsoft.com/windows/win32/direct3d12/helper-structures-and-functions-for-d3d12 (checked 2026-10-17).
* Component: build tooling and distribution (`BLD`).
