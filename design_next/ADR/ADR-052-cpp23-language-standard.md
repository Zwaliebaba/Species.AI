---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Language standard: C++23, with a portable subset for shared and server code

## Context and Problem Statement

Species is written in C++ (vision brief §5). The language mode decides which standard-library facilities the engine can rely on (`std::expected`, `std::print`, `std::mdspan`, `std::flat_map`, ranges and `[[assume]]`), and whether compiled code stays ABI-compatible across toolset updates. Code that the integrated and dedicated servers share with the client must also stay compilable by other standard libraries, so that a dedicated-server build for Linux hosts remains possible if the design phase chooses one.

Which C++ language mode does Species use, and what may shared code rely on?

## Decision Drivers

* A stable, supported language switch for every distributed binary.
* No experimental or in-progress features in shipping builds.
* One ABI unit: no mixing of objects built by different toolsets in C++23 preview mode.
* Shared and server code limited to features that MSVC, libc++ and libstdc++ all provide.

## Considered Options

* C++23
* C++20
* `/std:c++latest` (C++26 working draft)

## Decision Outcome

Chosen option: "C++23", because the MSVC standard library is C++23 feature-complete in 14.52, the compiler gains `/std:c++23` with that toolset, and C++23 adds the error-handling, formatting and container facilities the engine needs.

* **Switch:** `/std:c++23preview` on MSVC 14.51; `/std:c++23` from the moment 14.52 becomes the pinned toolset ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)).
* **ABI unit:** while the preview switch is in use, all first-party code and every vcpkg dependency ([ADR-055](ADR-055-dependencies-vcpkg-manifest-mode.md)) are rebuilt together on every toolset change; prebuilt C++23-mode binaries are never mixed across toolsets.
* **Never in shipping builds:** `/std:c++latest` and every C++26 feature. A C++26 facility is adopted only when MSVC offers it under a stable switch, through a new ADR.
* **Not available in any code:** the extended floating-point types (`std::float16_t` and the others), which MSVC does not implement.
* **Shared and server code** (everything the dedicated server links) avoids, in addition: `std::stacktrace`, `std::generator` and `std::move_only_function` (missing in libc++), and constexpr `<cmath>` (missing in libstdc++). Client-only code (renderer, audio, UI, input, crash reporting) may use the full MSVC C++23 library, including `std::stacktrace` for diagnostics.
* **Error handling at module boundaries:** recoverable errors are returned as `std::expected`; the component specifications set the rest of the error-handling policy.

### Consequences

* Good, because the language mode is supported by Microsoft and stabilises when 14.52 becomes default.
* Good, because the portable subset keeps the server code compilable by Clang and GCC standard libraries, without deciding a server platform.
* Bad, because the preview switch forces full rebuilds of dependencies on every toolset change until `/std:c++23` is in use.
* Bad, because shared code gives up coroutine generators and the standard stack trace.

### Confirmation

* The CMake toolchain sets the language switch in one place; configure fails on `/std:c++latest` in the profile and shipping flavours.
* The clang-cl analysis build ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)) compiles the shared and server targets; a lint rule rejects the excluded library facilities in those targets.
* The vcpkg triplet records the toolset version, so a toolset change invalidates every cached dependency binary.

## Pros and Cons of the Options

### C++23

* Good, because `std::expected`, `std::print`, `std::mdspan`, `std::flat_map` and `[[assume]]` are available.
* Neutral, because the stable switch arrives with 14.52; until then the preview switch applies.

### C++20

* Good, because `/std:c++20` is stable today.
* Bad, because it lacks `std::expected`, `std::print`, `std::mdspan` and `std::flat_map`, which the engine would otherwise re-implement.

### `/std:c++latest`

* Good, because it exposes the newest features.
* Bad, because it enables in-progress and experimental features and offers no ABI stability.

## More Information

* Facts and sources: technology baseline items 2.2 and 2.3 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the MSVC `/std` reference page, the C++ team blog on C++23 in MSVC Build Tools 14.51, the MSVC STL changelog, the Clang C++ status page and the libc++ and libstdc++ C++23 status files.
* The ISO publication status of C++26 was not verified (see [Items not verified](README.md#items-not-verified)); it does not affect this decision.
* Related: [ADR-053](ADR-053-modules-and-import-std.md) (modules).
