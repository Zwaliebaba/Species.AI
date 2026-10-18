---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead; decided by the owner's AGENTS.md (O21)
---

# Headers only: no modules and no `import std`

## Context and Problem Statement

C++23 standardises `import std` and `import std.compat`, and MSVC implements them; C++20 named modules and header units can shorten build times further. The repository's engineering rules limit source files to `.h` and `.cpp` (`AGENTS.md` R7), give every project one precompiled header (`pch.h`), and make include order load-bearing and hand-grouped, with the standard library included last (`AGENTS.md` §4). One candidate keeps first-party code header-based and adds `import std` behind a build option.

Does Species use C++ modules or `import std`?

## Decision Drivers

* Source files are `.h` and `.cpp` only (`AGENTS.md` R7).
* One way to reach the standard library, in the include order of `AGENTS.md` §4.
* Tooling (IDE, clang-tidy, the debugger) works on every translation unit without module support.

## Considered Options

* `import std` behind a build option; first-party code header-based
* Headers only
* First-party named modules and `import std` everywhere
* Header units

## Decision Outcome

Chosen option: "Headers only". Accepted: the owner's `AGENTS.md` decides it (O21). The option "`import std` behind a build option" is rejected, and so are named modules and header units. Species uses headers only: every translation unit includes the standard headers it needs, after `pch.h`, the Windows and SDK headers and the project headers (`AGENTS.md` §4). No module interface unit (`.ixx`) exists, because `AGENTS.md` R7 allows `.h` and `.cpp` files only, and no translation unit uses `import std`, because a build option for it would add a second way to reach the standard library and a project setting outside the alignment rules of [ADR-054](ADR-054-build-system-msbuild-solution.md). Using modules later needs a new ADR and a change to `AGENTS.md`.

### Consequences

* Good, because every translation unit is an ordinary header-based unit that every tool understands.
* Good, because there is one include order and no second build mode to keep green.
* Bad, because the build-time benefit of `import std` and of named modules is forgone; precompiled headers carry the build-time load instead.

### Confirmation

* `Build\CheckProjectFiles.py` rejects `.ixx`, `.cppm` and other module source files, and project settings that build standard-library modules.
* clang-tidy and review reject `import` declarations.

## Pros and Cons of the Options

### `import std` behind an option, headers for first-party code

* Good, because it would adopt the standard library module where the toolchain supports it.
* Bad, because it adds a second way to reach the standard library and a build option, against `AGENTS.md` R7 and §4.

### Headers only

* Good, because it is the simplest form and the one `AGENTS.md` describes.
* Bad, because it forgoes a supported build-time improvement.

### First-party named modules and `import std` everywhere

* Good, because it gives the largest build-time and isolation benefit.
* Bad, because module interface units are not `.h` or `.cpp` files (`AGENTS.md` R7).

### Header units

* Bad, because they change how headers are compiled and how macros reach a translation unit, which conflicts with the hand-grouped include order of `AGENTS.md` §4.

## More Information

* Rules: `AGENTS.md` R7 (file kinds) and §4 (include order).
* Facts and sources: technology baseline item 2.2 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the MSVC modules overview and `import std` tutorial.
* Related: [ADR-052](ADR-052-cpp23-language-standard.md) (language mode).
* **Amendment 2026-10-17 (AGENTS.md, O21).** Status Rejected: `AGENTS.md` R7 allows `.h` and `.cpp` files only and §4 fixes the standard library as hand-ordered includes, so no module interface unit exists and `import std` is not used. The build option and its CI job are removed.
* **Amendment 2026-10-18 (status).** The decision in force is "Headers only", so the record's status is Accepted and its title names that decision; the rejected `import std` option and the other modular options stay as considered options. The rule itself is unchanged.
