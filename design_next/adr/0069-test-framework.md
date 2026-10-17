---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Test framework: GoogleTest through CTest, plus property, golden and fuzz tests

## Context and Problem Statement

The component specifications require unit, property, golden-image, golden-hash and performance tests, each citing requirement ids. Tests must run from CMake test presets ([ADR-0054](0054-build-system-cmake-presets.md)) on developer machines and CI, report results per test case, and run in parallel. Shared and server code must stay testable with any compiler a server build may use. Content loaders, which read files that players can tamper with, also need fuzz testing.

Which test framework and test kinds does Species use?

## Decision Drivers

* Native integration with CTest and test presets, with per-test-case discovery.
* Fixtures, parameterised tests and mocks for engine subsystems.
* Portable across MSVC, clang-cl and other server compilers.
* Available through vcpkg ([ADR-0055](0055-dependencies-vcpkg-manifest-mode.md)).

## Considered Options

* GoogleTest (with GoogleMock), discovered by CTest
* Catch2
* doctest
* The Microsoft C++ unit test framework for Visual Studio

## Decision Outcome

Chosen option: "GoogleTest (with GoogleMock), discovered by CTest". This is the default the team proceeds with.

* **Unit and integration tests:** GoogleTest test executables per library; CMake's `gtest_discover_tests` registers every test case with CTest, so test presets, CI reporting and parallel runs work per case. Test names start with the requirement id they verify (for example `BLD_014_RefusesTwoProducts`).
* **Property tests:** parameterised tests with seeded generators; a failing seed is printed and becomes a permanent regression case.
* **Golden-hash tests:** the content build ([ADR-0059](0059-content-build-and-validators.md)) and deterministic engine subsystems compare output hashes with checked-in expectations.
* **Golden-image tests:** the renderer renders fixed scenes on a pinned reference configuration and compares images with per-test tolerances; reference images are reviewed on change.
* **Fuzz tests:** libFuzzer-compatible harnesses for the pack-file reader, every binary record loader and the authoring-file parser, run nightly in a sanitiser-enabled analysis configuration ([ADR-0051](0051-windows-toolchain-and-compiler-policy.md)); the fuzzing engine is pinned with the other dependencies.
* **Performance tests:** benchmark executables with fixed scenes and budgets from the component specifications; results are CI artefacts.
* **Modules:** test translation units include standard headers and never `import std` ([ADR-0053](0053-modules-and-import-std.md)), because the framework headers include standard headers.

### Consequences

* Good, because CTest knows every test case, so presets and CI need no custom runners.
* Good, because the same framework builds with any mainstream compiler.
* Bad, because GoogleTest's macro style is verbose compared with Catch2 or doctest.

### Confirmation

* CI fails if a test executable registers no test cases with CTest.
* A lint step checks that every test name starts with an existing requirement id.

## Pros and Cons of the Options

### GoogleTest with GoogleMock

* Good, because CMake ships a module that discovers its test cases, and mocks are included.
* Neutral, because compile times of large test files are moderate.

### Catch2

* Good, because expressive assertions and built-in generators suit property tests.
* Bad, because CTest discovery relies on a script distributed with Catch2 rather than on CMake itself, and mocks need another library.

### doctest

* Good, because it compiles quickly and is light.
* Bad, because it has no mocking and a smaller ecosystem.

### Microsoft C++ unit test framework

* Bad, because it is tied to Visual Studio's test runner and does not build for a non-Windows server.

## More Information

* The standards baseline does not list test-framework versions; the version is pinned through vcpkg and recorded by the diagnostics component.
* Related: [ADR-0070](0070-continuous-integration.md).
* Component: diagnostics, test and performance (`DBG`).
