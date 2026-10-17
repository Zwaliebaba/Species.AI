---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Modules: `import std` behind a build option; first-party code stays header-based

## Context and Problem Statement

C++23 standardises `import std` and `import std.compat`. MSVC implements them fully, and they can shorten build times. Build-system support is not yet stable: CMake 4.4 offers `import std` only behind an experimental gate, with Ninja generators only, and CMake 4.5 (now at release candidate) makes it non-experimental. Shared and server code must stay compilable without modules, so that a non-MSVC server build remains possible ([ADR-0052](0052-cpp23-language-standard.md)).

How does Species use C++ modules?

## Decision Drivers

* Faster builds where modules are reliable.
* No dependence on an experimental build-system feature in shipping builds.
* Shared and server code buildable with and without modules.
* Tooling (IDE, static analysis, the clang-cl analysis build) keeps working.

## Considered Options

* `import std` behind a build option; first-party code header-based
* Headers only
* First-party named modules and `import std` everywhere
* Header units

## Decision Outcome

Chosen option: "`import std` behind a build option; first-party code header-based". This is the default the team proceeds with until the option is reviewed at the CMake 4.5 release.

* **Option:** `SPECIES_IMPORT_STD`, default **OFF** while CMake offers `import std` only experimentally. When CMake 4.5 is released, the team re-checks its documentation and switches the default to **ON** for client-only targets.
* **Rules when ON:** a translation unit uses either `import std;` or standard headers, never both. Macros are not exported by named modules, so `assert`, `errno` and `offsetof` come from `<cassert>`, `<cerrno>` and `<cstddef>`. Third-party headers that include standard headers are wrapped in their own translation units or kept out of importing units.
* **Shared and server code** compiles in both modes; CI builds it with the option OFF on every change.
* **First-party named modules** are not part of this baseline. A later ADR may introduce them library by library once the IDE, the analysis build and the debugger handle them on the pinned toolset.
* **Header units** are not used.

### Consequences

* Good, because client builds gain `import std` without making any shipping build depend on an experimental gate.
* Good, because every target still builds header-only, which keeps a non-MSVC server build possible.
* Bad, because the build-time benefit of first-party modules is deferred.
* Bad, because two build modes must both stay green while the option exists.

### Confirmation

* CI runs one client build with `SPECIES_IMPORT_STD=ON` (nightly) and every per-change build with it OFF, until the default changes.
* A lint step rejects translation units that both `import std` and include a standard header.
* The CMake 4.5 release is a tracked milestone item for re-checking this ADR.

## Pros and Cons of the Options

### `import std` behind an option, headers for first-party code

* Good, because it adopts the standard library module where the toolchain is mature.
* Neutral, because it needs Ninja generators, which [ADR-0054](0054-build-system-cmake-presets.md) uses anyway.

### Headers only

* Good, because it is the simplest and most portable.
* Bad, because it forgoes a supported build-time improvement on the client.

### First-party named modules and `import std` everywhere

* Good, because it gives the largest build-time and isolation benefit.
* Bad, because CMake module support is incomplete (Visual Studio generators cannot export or install module information), and shared code would need module support from every server compiler.

### Header units

* Bad, because CMake does not support them.

## More Information

* Facts and sources: §2.2 and §3.1 of the technology baseline in the [ADR index](README.md) (checked 2026-10-17): the MSVC modules overview and `import std` tutorial, and the CMake 4.4 and development (4.5) `cmake-cxxmodules` manuals.
* A non-Windows server build that turns the option on needs a standard library with working module metadata; the baseline notes that Ubuntu releases before 26.04 ship broken `libstdc++.modules.json` files.
