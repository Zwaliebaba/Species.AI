---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead; decided by the owner's AGENTS.md (O21)
---

# Test framework: the Microsoft C++ Unit Test Framework run by `vstest.console.exe`, with property, golden and fuzz tests in Species code

## Context and Problem Statement

The component specifications require unit, property, golden-image, golden-hash and performance tests, each citing requirement ids. Tests must build as projects of the solution ([ADR-054](ADR-054-build-system-msbuild-solution.md)), run on developer machines and in CI, report results per test case, and never pass by being empty. The executable and its test suites may use no third-party library (`AGENTS.md` R14), and the repository's engineering rules name the runner: `vstest.console.exe`, over every suite the build produced, with a `SuiteSmoke` placeholder in every test project (`AGENTS.md` §3). Content loaders, which read files that players can tamper with, also need fuzz testing.

Which test framework and test kinds does Species use?

## Decision Drivers

* A framework that ships with Visual Studio and needs no third-party code (`AGENTS.md` R14).
* Test projects built through the solution and run by `vstest.console.exe` (`AGENTS.md` §3).
* Per-test-case results, with the requirement id each test verifies.
* No empty suite reported as a pass.

## Considered Options

* The Microsoft C++ Unit Test Framework (CppUnitTest), run by `vstest.console.exe`
* GoogleTest with GoogleMock
* Catch2 or doctest

## Decision Outcome

Chosen option: "The Microsoft C++ Unit Test Framework, run by `vstest.console.exe`". Accepted: the owner's `AGENTS.md` decides it (O21).

* **Projects:** one test project `<Project>Tests` per library, a DLL project of the solution that links the library under test and the framework's header and library, which ship with Visual Studio. Test code lives in `namespace <Project>Tests` (`AGENTS.md` R9). Every test project ships a placeholder test class `SuiteSmoke` until its first real test lands; the placeholder is deleted then, never before, because `vstest.console.exe` reports a suite with no tests as a pass.
* **Tests:** `TEST_CLASS` and `TEST_METHOD` with PascalCase names; each test method carries the requirement id it verifies, both in its name (for example `Bld014RefusesTwoProducts`) and as a test-method attribute (`Requirement` with the value `BLD-014`), so results can be filtered and traced per requirement.
* **Running:** `vstest.console.exe` runs every `<Project>Tests.dll` the build produced, `/Platform:x64`, with a `.trx` log ([ADR-070](ADR-070-continuous-integration.md)). A test project that produced no DLL fails the run.
* **Property tests:** parameterised loops over a Species seeded generator inside test methods; a failing seed is logged and becomes a permanent regression case.
* **Golden-hash tests:** the content build ([ADR-059](ADR-059-content-build-and-validators.md)) and deterministic engine subsystems compare output hashes with checked-in expectations.
* **Golden-image tests:** the renderer renders fixed scenes on a pinned reference configuration and compares images with per-test tolerances; reference images are reviewed on change. They need a GPU agent, so they enter CI with the job that provides one ([ADR-070](ADR-070-continuous-integration.md)).
* **Fuzz tests:** harnesses for the pack-file reader, every binary record loader and the authoring-file parser live in the library's `<Project>Tests` project. Each harness is a function that takes one byte buffer; its test methods run it over a committed seed corpus and over every input that has ever failed, in the ordinary Debug test run, so they build with the settings of every other project. The baseline has no separate fuzz project (O27). A coverage-guided fuzzing build is an executable of its own, compiled with `/fsanitize=address` and `/fsanitize=fuzzer`, and it needs `BasicRuntimeChecks` off and `LinkIncremental` false in both configurations, because AddressSanitizer is incompatible with the `/RTC` checks and with incremental linking; such a project differs from the settings every other project shares ([ADR-054](ADR-054-build-system-msbuild-solution.md)), so it needs an owner decision, an `AGENTS.md` change and an amendment of this record that names its settings, its checker exception and its CI job.
* **Performance tests:** performance-test executables with repeatable scenes and budgets from the component specifications; results are recorded with the build identity.
* **Modules:** none; test files include standard headers ([ADR-053](ADR-053-modules-and-import-std.md)).

### Consequences

* Good, because the framework, its runner and the IDE's test window ship with Visual Studio and need no package.
* Good, because the `SuiteSmoke` rule and the missing-DLL check prevent a green result over an untested library.
* Bad, because the framework has no built-in mocking or generators; fakes and property generators are Species test code.
* Bad, because the baseline fuzz tests replay a fixed corpus and do not search for new inputs; coverage-guided fuzzing needs MSVC's experimental LibFuzzer support in a project of its own, which the baseline does not have.

### Confirmation

* CI fails when a `<Project>Tests` project produced no DLL, and when `vstest.console.exe` reports a failure ([ADR-070](ADR-070-continuous-integration.md)).
* `Build\CheckProjectFiles.py` checks that every library has a `<Project>Tests` project and that test projects use the `<Project>Tests` name and namespace.
* `Build\CheckProjectFiles.py` checks that every test method carries an existing requirement id ([build tooling](../architecture/components/build_tooling_and_distribution.md) table 4.1-B row 14, DBG-140).

## Pros and Cons of the Options

### Microsoft C++ Unit Test Framework

* Good, because it ships with Visual Studio, integrates with the IDE's test window and runs under `vstest.console.exe`.
* Neutral, because its assertion and attribute macros are plainer than other frameworks'.

### GoogleTest with GoogleMock

* Good, because it has mocks and parameterised tests built in.
* Bad, because it is third-party code, which `AGENTS.md` R14 excludes.

### Catch2 or doctest

* Good, because they are expressive and light.
* Bad, because they are third-party code, which `AGENTS.md` R14 excludes.

## More Information

* Rules: `AGENTS.md` §3 (run the tests through `vstest.console.exe`; `SuiteSmoke`), R9 (test namespaces) and R14.
* Facts and sources (checked 2026-10-17): the framework's header and library are under the Visual Studio installation folder (`VC\Auxiliary\VS\UnitTest`), with test-method attributes (https://learn.microsoft.com/visualstudio/test/microsoft-visualstudio-testtools-cppunittestframework-api-reference); `vstest.console.exe` returns 0 when no tests are discovered (https://learn.microsoft.com/visualstudio/test/vstest-console-options); `/fsanitize=fuzzer` is experimental LibFuzzer support from Visual Studio 2022 version 17.0 (https://learn.microsoft.com/cpp/build/reference/fsanitize · https://learn.microsoft.com/cpp/sanitizers/asan-building); AddressSanitizer is incompatible with the `/RTC` options and does not support incremental linking (https://learn.microsoft.com/cpp/sanitizers/asan-known-issues, checked 2026-10-18).
* Related: [ADR-070](ADR-070-continuous-integration.md).
* Component: diagnostics, test and performance (`DBG`).
* **Amendment 2026-10-17.** Performance tests use repeatable scenes, in the wording of the diagnostics component (DBG-183, DBG-208).
* **Amendment 2026-10-17 (AGENTS.md, O21).** The framework is the Microsoft C++ Unit Test Framework that ships with Visual Studio, run by `vstest.console.exe`, with one `<Project>Tests` project per library and a `SuiteSmoke` placeholder until the first real test; property, golden and fuzz tests use no third-party library. The third-party test framework, CTest and the test presets are removed. Status Accepted, decided by the owner's `AGENTS.md`.
* **Amendment 2026-10-18 (O27).** The fuzz harnesses are test methods of the `<Project>Tests` projects, run over a committed seed corpus in the Debug test run; the `<Project>Fuzz` projects are removed, because O27 adds no fuzz project to `AGENTS.md`. The settings that a coverage-guided fuzzing project would need are recorded in the decision for that later owner decision.
* **Amendment 2026-10-18.** The requirement-id check of every test method is a rule of the project-file checker `Build\CheckProjectFiles.py` (build tooling table 4.1-B row 14, DBG-140), not a review or lint step.
