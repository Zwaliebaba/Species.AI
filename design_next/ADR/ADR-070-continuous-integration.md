---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Continuous integration: preset-driven pipelines with content validation, analysis builds and a headless bot soak

## Context and Problem Statement

Every change to Species code or content must be built, validated and tested before it is merged, with the same configurations developers use. Long-running checks (a soak of many matches, fuzzing, GPU validation) must run regularly without slowing every change. A headless soak that plays matches between bots on every mode and map, and reports crashes, timing and peak population as artefacts, replaces manual test passes. The CI service and its hosting are infrastructure choices outside this ADR.

What does Species' continuous integration run, and when?

## Decision Drivers

* CI runs exactly the presets developers run ([ADR-054](ADR-054-build-system-cmake-presets.md)).
* Content errors fail the change ([ADR-059](ADR-059-content-build-and-validators.md)).
* Crashes and performance regressions are found automatically, with artefacts.
* Server and shared code stay buildable by a second front end ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)).

## Considered Options

* Two tiers: required per-change jobs and nightly long-running jobs, all as workflow presets
* One tier: everything on every change
* Manual release testing only

## Decision Outcome

Chosen option: "Two tiers: required per-change jobs and nightly long-running jobs, all as workflow presets".

* **Per change (required):**
  * the `windows-msvc-development-<product>` and `windows-msvc-shipping-<product>` builds of every product, with warnings as errors;
  * the `windows-clangcl-analysis-<product>` builds of the `server` and `tools` products, which cover the shared and server code, with static analysis;
  * the content build of every edition, with all validators;
  * unit, property and golden-hash tests ([ADR-069](ADR-069-test-framework.md));
  * binary checks: mitigation flags, symbols in the symbol store, version resource, no developer or loose-file code in shipping binaries ([ADR-056](ADR-056-editions-build-flavours-and-feature-flags.md), [ADR-057](ADR-057-version-identity.md)).
* **Nightly:**
  * the headless bot soak: the `server` product plays matches between bots on every mode and map of each edition at maximum speed, and reports crashes, assertion failures, simulation update-time percentiles and peak population per match as artefacts;
  * golden-image tests on the pinned reference configuration;
  * a client test pass with the Direct3D 12 debug layer and GPU-based validation;
  * AddressSanitizer and `/analyze` builds with their test runs;
  * fuzzing of loaders;
  * the reproducibility check of the content build;
  * the `SPECIES_IMPORT_STD=ON` client build ([ADR-053](ADR-053-modules-and-import-std.md));
  * performance tests against component budgets.
* **Artefacts:** test reports, soak reports, performance results, crash dumps from test machines ([ADR-065](ADR-065-crash-reporting.md)) and the dependency licence list.
* **Build agents:** Windows agents with the Visual Studio Build Tools at the pinned toolset; at least one agent with a GPU for golden-image and validation jobs. A non-Windows server build would add its own agents.
* **Replays:** if the design phase gives Species reproducible match replays, recorded matches become nightly regression tests that compare end-of-match state hashes; the pipeline reserves a job for them.

### Consequences

* Good, because every change is built, validated and tested with developer configurations.
* Good, because crashes, soak results and timing trends are visible every day.
* Bad, because the nightly tier needs GPU agents and several hours of machine time.

### Confirmation

* Each job is a `workflowPreset`; a check fails when a CI definition runs a command outside the presets.
* The merge rule requires every per-change job to pass.
* The nightly soak report is reviewed in the team's daily triage; a crash opens a defect automatically.

## Pros and Cons of the Options

### Two tiers

* Good, because changes are checked quickly and long checks still run every day.
* Neutral, because a nightly failure is found up to a day after the change that caused it.

### One tier

* Good, because every problem is found before merge.
* Bad, because a soak and fuzzing on every change would make each change take hours.

### Manual release testing

* Bad, because regressions are found late and without reproducible artefacts.

## More Information

* Components: diagnostics, test and performance (`DBG`), build tooling and distribution (`BLD`).
* The CI service, agent hosting and the binary-cache host are infrastructure choices made by the team operating the project.
* **Amendment 2026-10-17.** Job presets carry the product suffix of ADR-054 (`<os>-<compiler>-<flavour>-<product>`, BLD-006); the build tooling component lists the jobs and their workflow presets.
