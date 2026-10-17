---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Build system: CMake presets with the Ninja Multi-Config generator

## Context and Problem Statement

Species builds several products (client, dedicated server, content tools) in several build flavours ([ADR-056](ADR-056-editions-build-flavours-and-feature-flags.md)), on developer machines and on CI. The build definition must be declarative, checked in, shared by the IDE and CI, able to drive `import std` ([ADR-053](ADR-053-modules-and-import-std.md)) and the vcpkg toolchain ([ADR-055](ADR-055-dependencies-vcpkg-manifest-mode.md)), and able to grow a non-Windows server build if the design phase chooses one.

Which build system and generator does Species use, and how are configurations named?

## Decision Drivers

* One checked-in definition of every configuration, used identically by developers and CI.
* Support for C++ module scanning and `import std`.
* vcpkg manifest-mode integration.
* A route to a non-Windows server build without a second build system.

## Considered Options

* CMake with presets and the Ninja Multi-Config generator
* CMake with the Visual Studio generator
* MSBuild projects with property sheets per product
* Another meta-build system (Meson, Bazel)

## Decision Outcome

Chosen option: "CMake with presets and the Ninja Multi-Config generator". This is the default the team proceeds with.

* **CMake:** `cmake_minimum_required(VERSION 4.4)`; 4.4.4 is current. CMake 4.5 is adopted after its release, when `import std` is no longer experimental.
* **Presets:** `CMakePresets.json` is checked in; `CMakeUserPresets.json` is local and ignored by source control. The presets file uses **schema version 10** unless a newer field is needed; the schema version is raised deliberately and recorded here.
* **Generator:** Ninja Multi-Config (Ninja 1.11 or newer) for every command-line and CI build. Developers open the CMake folder directly in Visual Studio; generated solution files are not the primary build path, because Visual Studio generators cannot build `import std`.
* **Configurations:** `Debug`, `Development`, `Profile` and `Shipping` (meanings in [ADR-056](ADR-056-editions-build-flavours-and-feature-flags.md)).
* **Preset naming:** each configure preset selects exactly one operating system, compiler, flavour and product ([ADR-056](ADR-056-editions-build-flavours-and-feature-flags.md)), and is named `<os>-<compiler>-<flavour>-<product>`, for example `windows-msvc-development-client` and `windows-clangcl-analysis-server`; `<os>-<compiler>-<flavour>` is the preset stem, and the build tooling component lists the stems of the baseline and the products each one has. Each configure preset has matching build and test presets of the same name; each CI job is one `workflowPreset` named `ci-<tier>-<job>` ([ADR-070](ADR-070-continuous-integration.md)). Presets for a non-Windows server build are added only if the design phase decides to build one.
* **Repository layout:** `src/` (engine, game and server code), `content/` (authoring sources for the content build), `tools/` (content build and other tools), `documentation/`.

### Consequences

* Good, because one file defines every configuration for the IDE, the command line and CI.
* Good, because Ninja Multi-Config supports module scanning and `import std`.
* Good, because the same CMake project can configure a non-Windows server build later.
* Bad, because developers who prefer Visual Studio solution files use the IDE's CMake mode instead.
* Bad, because CMake 4.4 is new enough that the CMake version bundled with an IDE must be checked before raising the presets schema.

### Confirmation

* CI configures only through presets; a job fails when it passes ad-hoc `-D` options not present in a preset.
* A check validates `CMakePresets.json` against the declared schema version.

## Pros and Cons of the Options

### CMake presets with Ninja Multi-Config

* Good, because it is the only CMake generator family that builds `import std`.
* Good, because presets give named, reproducible configure, build, test and workflow steps.

### CMake with the Visual Studio generator

* Good, because it produces familiar solution files.
* Bad, because it cannot build `import std`, export module information or compile module interfaces from imported targets.

### MSBuild projects with property sheets

* Good, because it is native to Visual Studio.
* Bad, because a non-Windows server build would need a second build system.
* Bad, because vcpkg manifest mode and module support would be configured per project file rather than once.

### Meson or Bazel

* Good, because both are capable cross-platform build systems.
* Bad, because vcpkg integration, Visual Studio support and module support are weaker than with CMake, and the team would maintain more custom rules.

## More Information

* Facts and sources: technology baseline item 3.1 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): CMake releases on the Kitware GitHub repository and the CMake 4.4.4 `cmake-presets` and `cmake-cxxmodules` manuals (cmake.org itself was not reachable; the same documents were read from the repository).
* Which CMake version Visual Studio bundles was not checked.
* Component: build tooling and distribution (`BLD`).
* **Amendment 2026-10-17.** Preset names end with the product, because ADR-056 allows exactly one product per configure; the baseline preset stems are those of the build tooling component (BLD-006, its table of configure presets), and workflow presets are named `ci-<tier>-<job>`.
