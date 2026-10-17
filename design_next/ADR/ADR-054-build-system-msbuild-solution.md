---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead; decided by the owner's AGENTS.md (O21)
---

# Build system: one MSBuild solution (`.slnx`) of `.vcxproj` projects, with the x64 configurations Debug and Release only

## Context and Problem Statement

Species builds several programs from one codebase: the game client, the headless dedicated server and the test projects in the game's solution, and the content build tool in a separate tools solution under `Tools/` ([ADR-056](ADR-056-products-configurations-and-feature-flags.md), `AGENTS.md` §2), for Windows x64 only, with MSVC ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)). The build definition must be checked in, used identically by the Visual Studio IDE and by CI, compile HLSL into the executable as part of the build ([ADR-003](ADR-003-shader-language-compiler-and-material-shaders.md)), restore the three allow-listed NuGet packages ([ADR-055](ADR-055-dependencies-three-nuget-packages.md)), and keep its two configurations aligned by rule, because CI builds only one of them. The repository's engineering rules (`AGENTS.md` §2, §3) fix the build system.

Which build system does Species use, and how are its projects and configurations shaped?

## Decision Drivers

* One checked-in build definition, used the same way by the IDE, the command line and CI.
* Debug and Release aligned by rule, with the differences listed and checkable without building Release.
* Shaders compiled by the build itself, with no separate shader tool step.
* No build system or package manager beyond MSBuild and NuGet restore (`AGENTS.md` §2, R14).
* Output and include paths that resolve the same way for every project.

## Considered Options

* One MSBuild solution (`.slnx`) of `.vcxproj` projects
* CMake with presets (Ninja or Visual Studio generators)
* Another meta-build system (Meson, Bazel)

## Decision Outcome

Chosen option: "One MSBuild solution (`.slnx`) of `.vcxproj` projects". Accepted: the owner's `AGENTS.md` decides it (O21).

* **Solution:** exactly one `.slnx` solution of the game at the repository root; MSBuild builds the `.slnx` format from version 17.12. CI finds the solution at the root and fails when there are two.
* **Tools solution:** the C++ development tools that never ship, the content build tool and its importers ([ADR-059](ADR-059-content-build-and-validators.md)), are projects of a separate solution under `Tools/`. It follows every rule of this record except the allow-list of R14: it may use pinned third-party code, each dependency recorded in an ADR with its licence ([ADR-055](ADR-055-dependencies-three-nuget-packages.md)). Nothing from it links into the executable, and the game's solution never references a project of it (`AGENTS.md` §2).
* **Projects:** one `.vcxproj` per library or program, each with its `.filters` file. Filters are functional (`Rendering`, `Audio`, `Input`, `Shader`); the default `Source Files`, `Header Files` and `Resource Files` filters are deleted when a project is created, and a `.h` sits in the same filter as its `.cpp`. Adding, removing or moving a file edits the owning `.vcxproj` and its `.filters`.
* **Directories:** C++ source lives directly in its project's folder, with no `src/` or `include/` split. The two sanctioned subfolders are `<Lib>/Shader/` (hand-written HLSL) and `<Lib>/CompiledShader/` (headers written by `FXCompile`, build output) ([ADR-003](ADR-003-shader-language-compiler-and-material-shaders.md)).
* **Toolset and platform:** `PlatformToolset` `v145` in every project, x64 only; no Win32 platform in any project or solution. The compiler and linker settings are those of [ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md).
* **Configurations:** `Debug|x64` and `Release|x64`, and no other. They differ in exactly these settings: `Optimization`, `_DEBUG` against `NDEBUG`, `FunctionLevelLinking` and `IntrinsicFunctions`, and the linker's folding and whole-program switches, spelled in MSBuild as `UseDebugLibraries`, `RuntimeLibrary` (debug or release CRT), `LinkIncremental`, `WholeProgramOptimization`, `EnableCOMDATFolding` and `OptimizeReferences`. Every other setting (language standard, conformance, warning level, include directories, precompiled header, floating-point model, instruction set, shader compilation) reads identically in both. What Debug compiles in beyond Release follows from `NDEBUG` ([ADR-056](ADR-056-products-configurations-and-feature-flags.md)).
* **Building:** always through the solution, never a `.vcxproj` directly, because output paths and cross-project include directories are anchored on `$(SolutionDir)`, which MSBuild defines only for a solution build. One project is built with `/t:<ProjectName>` on the solution. Output goes to `x64\<Configuration>\`. A project never lists its own folder on its include path; the folders of other projects are listed as `$(SolutionDir)<Project>`.
* **Restore:** `msbuild <Solution>.slnx /t:Restore /p:RestorePackagesConfig=true` restores the packages of [ADR-055](ADR-055-dependencies-three-nuget-packages.md) into `packages\` before the build; restored packages are build output.
* **Never committed:** `x64\`, `.vs\`, `*.user`, `packages\`, `CompiledShader\` and every other file a build step generates.
* **Layout of the first projects:** the solution name, the projects and the edges between them are settled when the first project is created and recorded then, in a new ADR (`AGENTS.md` §2). Authoring content for the content build lives under `content/` ([ADR-059](ADR-059-content-build-and-validators.md)); development tools live in the tools solution under `Tools/` and the build checkers under `Build/`.
* **Checkers:** `Build\CheckProjectFiles.py` checks the build shape (one solution, every file registered in its project and filters, functional filters, flat directories, `v145` and x64 only, and Debug and Release differing only in the settings listed above); `Build\CheckFormat.py` and `Build\RunClangTidy.py` check the code ([ADR-070](ADR-070-continuous-integration.md)).

### Consequences

* Good, because the build definition is native to Visual Studio and MSBuild, with `FXCompile` and NuGet restore built in, and no second build system.
* Good, because the static check of the two configurations stands in for the Release build that CI does not run.
* Bad, because the build is Windows-only: a dedicated server for another operating system needs a later ADR and a build definition of its own.
* Bad, because project files are verbose XML, and every file change edits two of them.
* Bad, because two configurations leave no optimised build with development tools ([ADR-056](ADR-056-products-configurations-and-feature-flags.md)), and a sanitiser or fuzzing build needs projects of its own ([ADR-069](ADR-069-test-framework.md)).

### Confirmation

* `Build\CheckProjectFiles.py` runs first in CI and fails on any rule of the decision above, including a setting that differs between Debug and Release outside the listed ones.
* CI builds `Debug|x64` through the solution with MSBuild warnings as errors ([ADR-070](ADR-070-continuous-integration.md)).
* Before a release, whoever ships builds `Release|x64` through the solution and says so.

## Pros and Cons of the Options

### One MSBuild solution of `.vcxproj` projects

* Good, because Visual Studio, MSBuild, `FXCompile`, NuGet restore and the Microsoft C++ Unit Test Framework work together without glue.
* Neutral, because the static configuration check must be written early ([ADR-070](ADR-070-continuous-integration.md)).
* Bad, because it builds only on Windows.

### CMake with presets

* Good, because presets give named, reproducible configure, build and test steps on several operating systems.
* Bad, because it is a second build system next to the IDE's own, and `AGENTS.md` §3 excludes it ("There is no CMake").

### Meson or Bazel

* Good, because both are capable cross-platform build systems.
* Bad, because Visual Studio integration, shader compilation and NuGet restore would all need custom rules, and `AGENTS.md` excludes any build system beyond MSBuild.

## More Information

* Rules: `AGENTS.md` §2 (repository shape, filters, shader folders, NuGet restore) and §3 (build settings, configurations, building through the solution).
* Facts and sources (checked 2026-10-17): MSBuild builds `.slnx` solutions from version 17.12 (https://learn.microsoft.com/visualstudio/msbuild/msbuild-command-line-reference); Visual Studio 2026 ships the `v145` platform toolset (https://learn.microsoft.com/cpp/overview/what-s-new-for-msvc); `msbuild -t:restore -p:RestorePackagesConfig=true` restores `packages.config` projects (https://learn.microsoft.com/nuget/consume-packages/package-restore).
* Component: build tooling and distribution (`BLD`).
* **Amendment 2026-10-17 (AGENTS.md, O21).** The build system is one MSBuild solution of `.vcxproj` projects with functional filters, toolset `v145`, and the configurations `Debug|x64` and `Release|x64`, which differ only in the optimisation settings of `AGENTS.md` §3; it is built through the solution. CMake, Ninja, presets and the configurations `Development`, `Profile` and `Shipping` are not used. Status Accepted, decided by the owner's `AGENTS.md`.
* **Amendment 2026-10-17 (AGENTS.md §2, O26).** The content build tool is not a project of the game's solution: it and its importers are projects of a separate tools solution under `Tools/`, which follows this record except R14 and which the game's solution never references. The context, the solution rule and the layout rule say so.
