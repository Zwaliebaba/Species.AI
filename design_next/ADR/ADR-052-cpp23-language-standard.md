---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead; switch decided by the owner's AGENTS.md (O21)
---

# Language standard: `/std:c++latest`, code written to C++23, with a portable subset as guidance for a later server

## Context and Problem Statement

Species is written in C++ (vision brief §5). The language mode decides which standard-library facilities the engine can rely on (`std::expected`, `std::print`, `std::mdspan`, `std::flat_map`, ranges and `[[assume]]`). The repository's engineering rules fix the switch: `/std:c++latest` in every project and both configurations (`AGENTS.md` §3). That switch also exposes facilities beyond C++23 and gives no ABI stability across toolset updates, so the code needs a rule for what it may use. A dedicated server for another operating system is not part of the baseline ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)), but code it might share with the client should not drift far from what other standard libraries provide.

Which C++ language mode does Species use, and what may the code rely on?

## Decision Drivers

* The switch of `AGENTS.md` §3, identical in Debug and Release.
* A language level that MSVC implements completely, so code does not depend on in-progress features.
* No ABI mixing: everything is built together with one toolset.
* Shared code that a later non-Windows server could compile with little change.

## Considered Options

* `/std:c++latest`, with code written to C++23
* `/std:c++23preview` (and `/std:c++23` when it becomes available)
* C++20

## Decision Outcome

Chosen option: "`/std:c++latest`, with code written to C++23". Accepted: the switch is decided by the owner's `AGENTS.md` (O21).

* **Switch:** `/std:c++latest` in every project, Debug and Release alike. It gives the full C++23 language and standard library of the `v145` toolset ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)).
* **Language level:** code is written to C++23. A facility newer than C++23 that `/std:c++latest` exposes is used only when MSVC documents it as complete, and its first use amends this record with the facility and the reason. The extended floating-point types (`std::float16_t` and the others) are not available, because MSVC does not implement them.
* **One ABI unit:** the whole tree is built from source in one solution with one toolset ([ADR-054](ADR-054-build-system-cmake-presets.md)); no prebuilt C++ library is linked. The allow-listed packages ([ADR-055](ADR-055-dependencies-vcpkg-manifest-mode.md)) expose C and COM interfaces.
* **Portable subset (guidance):** code that a dedicated server would share with the client avoids `std::stacktrace`, `std::generator` and `std::move_only_function` (missing in libc++) and constexpr `<cmath>` (missing in libstdc++). This is guidance for the later ADR that decides a non-Windows server, not a rule the build enforces now. Client-only code (renderer, audio, UI, input, crash reporting) may use the full MSVC C++23 library.
* **Error handling at module boundaries:** recoverable errors are returned as `std::expected`; the component specifications set the rest of the error-handling policy.

### Consequences

* Good, because the switch is the one `AGENTS.md` fixes, and the C++23 rule keeps code off in-progress features.
* Good, because building everything together removes the ABI risk of `/std:c++latest`.
* Bad, because a toolset update can change what `/std:c++latest` exposes or how it behaves; the C++23 rule and the CI toolchain record ([ADR-070](ADR-070-continuous-integration.md)) limit and trace the effect.
* Bad, because the portable subset is not enforced, so code shared with a later non-Windows server may need rework.

### Confirmation

* `Build\CheckProjectFiles.py` checks that every project uses `/std:c++latest` (`LanguageStandard` `stdcpplatest`) in both configurations.
* Review checks every use of a facility newer than C++23 against this record.

## Pros and Cons of the Options

### `/std:c++latest`, code written to C++23

* Good, because it is the switch `AGENTS.md` fixes and it exposes the complete C++23 library.
* Neutral, because the C++23 rule is a review rule.

### `/std:c++23preview` or `/std:c++23`

* Good, because it limits the compiler to C++23 by itself.
* Bad, because `AGENTS.md` §3 fixes `/std:c++latest`.

### C++20

* Bad, because it lacks `std::expected`, `std::print`, `std::mdspan` and `std::flat_map`, which the engine would otherwise re-implement, and `AGENTS.md` fixes a later mode.

## More Information

* Rules: `AGENTS.md` §3 and R7.
* Facts and sources: technology baseline item 2.2 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the MSVC `/std` reference page, the C++ team blog on C++23 in MSVC Build Tools 14.51 and the MSVC STL changelog; "to access all the new language features, build with `/std:c++latest`" (https://learn.microsoft.com/cpp/overview/what-s-new-for-msvc).
* Related: [ADR-053](ADR-053-modules-and-import-std.md) (modules).
* **Amendment 2026-10-17 (AGENTS.md, O21).** The switch is `/std:c++latest` in every project and both configurations; code is written to C++23; the portable subset for shared code is guidance for a later non-Windows server only. Status Accepted, with the switch decided by the owner's `AGENTS.md`.
