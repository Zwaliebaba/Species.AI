---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead; decided by the owner's AGENTS.md (O21)
---

# Third-party dependencies: none in the executable except three Microsoft NuGet packages, restored at build time

## Context and Problem Statement

The Species executable is built from the Windows SDK, the MSVC standard library and a closed list of three Microsoft packages, and nothing else (`AGENTS.md` R14): the DirectX Agility SDK, GameInput and the PIX event runtime. Everything else an engine usually takes from a library is met by an in-box Windows component or by Species code: compression ([ADR-058](ADR-058-pack-files-and-mount-policy.md)), audio decoding ([ADR-025](ADR-025-audio-formats-codecs-and-residency.md)), HTTPS ([ADR-077](ADR-077-client-http-stack.md)), PNG encoding ([ADR-076](ADR-076-screenshot-encoding.md)), the authoring parser ([ADR-062](ADR-062-authoring-syntax-and-schema-language.md)), tests ([ADR-069](ADR-069-test-framework.md)) and shader compilation ([ADR-003](ADR-003-shader-language-compiler-and-material-shaders.md)). Development tools that never ship have different needs. Every developer and CI agent must get exactly the same package versions, and restored packages must never be committed.

How does Species obtain, pin and limit its third-party dependencies?

## Decision Drivers

* A closed allow-list for what the executable is built from; adding to it is an owner decision recorded as an ADR.
* Exact versions checked in with the projects that use them.
* No package manager beyond NuGet restore; no vendored SDKs.
* Restored packages are build output, never source.
* A licence list for everything that ships.

## Considered Options

* NuGet restore of the three allow-listed packages through `packages.config`
* A package manager (vcpkg in manifest mode) for a wider set of libraries
* Vendored sources or Git submodules

## Decision Outcome

Chosen option: "NuGet restore of the three allow-listed packages through `packages.config`". Accepted: the owner's `AGENTS.md` decides it (O21, R14).

* **The allow-list** (versions pinned in the [technology baseline](README.md#technology-baseline)):

| Package (NuGet) | What it provides | Pinned version |
|---|---|---|
| `Microsoft.Direct3D.D3D12` (DirectX Agility SDK) | the Direct3D 12 runtime `D3D12Core.dll`, installed beside the executable, and its headers ([ADR-001](ADR-001-direct3d-12-renderer-on-the-agility-sdk.md)) | 1.619.6 |
| `Microsoft.GameInput` | the GameInput header and library, and `GameInputRedist.msi`, which the installer runs ([ADR-028](ADR-028-input-device-back-ends.md)) | 3.5.283 |
| `WinPixEventRuntime` | `pix3.h` and `WinPixEventRuntime.dll`, the PIX markers ([ADR-068](ADR-068-profiling-and-gpu-timing.md)) | 1.0.240308001 |

* **Restore:** each project that uses a package lists it, at its exact version, in a `packages.config` file in the project folder. `msbuild <Solution>.slnx /t:Restore /p:RestorePackagesConfig=true` restores the packages into `packages\` before the build ([ADR-054](ADR-054-build-system-cmake-presets.md)); `packages\` is build output and is never committed.
* **Excluded from the executable:** every other third-party library, including the `d3dx12.h` helpers (even where a package carries them), DirectX-Headers, DirectXTK12, DirectXTex, the DirectX Shader Compiler as a redistributable, and any third-party compression library, audio codec, JSON library, test framework, HTTP or TLS library and profiler. Resource barriers and heap descriptions are written by hand.
* **Development tools:** programs and scripts under `Tools/` and `Build/` never ship and never link into the executable, so they may use dependencies of their own, pinned and listed with their licences in the tool's folder. The content build tool's texture encoder ([ADR-017](ADR-017-texture-container-and-block-compression.md)) is one; Python packages that a checker or baker needs are others.
* **Content:** third-party content (art, fonts, sound) is the owner's decision; anything under a licence needs the owner's approval before it lands, and its licence text travels with the bytes.
* **Licences:** the install directory carries the licence notices of the three packages and of every licensed content file ([ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md)).

### Consequences

* Good, because the executable has a small, closed and reviewable dependency surface, and security fixes for everything else arrive through Windows servicing.
* Good, because there is no package-manager configuration, cache host or binary cache to maintain.
* Bad, because work that a library would do (the authoring parser, compression framing, test helpers, audio and texture encoders in the tools) is Species code with its own tests.
* Bad, because a dedicated server for another operating system would need replacements for the in-box Windows components the engine uses.

### Confirmation

* `Build\CheckProjectFiles.py` rejects a `packages.config` entry that is not on the allow-list or whose version differs from the technology baseline, a committed `packages\` folder, and an include directory outside the Windows SDK, the toolset, `packages\` and the solution's own projects.
* Before a release, the shipping builder checks that the install's licence list names exactly the three packages and the licensed content.

## Pros and Cons of the Options

### NuGet restore of the allow-list

* Good, because MSBuild restores `packages.config` projects itself, and the three packages are published on NuGet by Microsoft.
* Neutral, because every package version moves only by an edit that the configuration check verifies against the baseline.

### A package manager for a wider set

* Good, because versioning, overrides and binary caching are built in.
* Bad, because `AGENTS.md` R14 excludes every third-party library from the executable, and a second package source has nothing to deliver.

### Vendored sources or submodules

* Bad, because `AGENTS.md` §2 excludes vendored SDKs, and Species would maintain each dependency's build by hand.

## More Information

* Rules: `AGENTS.md` R14 and §2.
* Facts and sources (checked 2026-10-17): `msbuild -t:restore -p:RestorePackagesConfig=true` restores `packages.config` projects from MSBuild 16.5 (https://learn.microsoft.com/nuget/consume-packages/package-restore · https://learn.microsoft.com/nuget/reference/msbuild-targets#restore-target). Package versions: technology baseline items 4.1, 4.7 and 6 in the [ADR index](README.md#technology-baseline).
* Component: build tooling and distribution (`BLD`).
* **Amendment 2026-10-17 (AGENTS.md, O21).** The executable depends only on the Windows SDK, the MSVC standard library and the three allow-listed Microsoft packages, restored by NuGet from `packages.config` at build time and never committed. The package manager, its manifest, triplets and binary cache, and every other third-party library in the executable are removed; development tools under `Tools/` and `Build/` keep their own pinned dependencies. Status Accepted, decided by the owner's `AGENTS.md`.
