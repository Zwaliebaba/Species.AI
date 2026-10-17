---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead; decided by the owner's AGENTS.md (O21)
---

# Products, configurations and feature flags: one project per product, Debug and Release only, data-driven runtime flags

## Context and Problem Statement

Species produces several programs from one codebase: the game client (which can also host the integrated server, vision O10), the headless dedicated server (vision O5), the content build tool and the test suites. The solution has exactly two configurations, `Debug|x64` and `Release|x64`, which differ only in their optimisation settings ([ADR-054](ADR-054-build-system-cmake-presets.md), `AGENTS.md` §3). Development features (the developer console, debug commands, the development overlay, development checks) must exist where the team works and must be absent from what players run. Some content or features differ between test builds and releases (for example a test build that enables only some match modes). A build must never be ambiguous about which program it is.

How are products, development features and feature variants expressed?

## Decision Drivers

* Each program is one project of the solution, chosen by building that project, never by editing source.
* Debug and Release differ only in the settings `AGENTS.md` §3 lists.
* Development features cannot reach a Release binary by construction.
* Logging, crash context, profiling markers and opt-in telemetry exist in every binary that players or testers run.
* Feature variation without recompiling, so test editions reuse release binaries.
* Simulation-relevant flags are part of the content identity ([ADR-060](ADR-060-content-identity-by-hash.md)).

## Considered Options

* One project per product, development features keyed on `NDEBUG`, data-driven runtime feature flags
* Further configurations for development and profiling builds
* Runtime flags only, with development features present in every binary

## Decision Outcome

Chosen option: "One project per product, development features keyed on `NDEBUG`, data-driven runtime feature flags". Accepted: the owner's `AGENTS.md` decides it (O21).

* **Products** are projects of the solution: the client (the game executable, `species.exe`); the server (headless; links no graphics, audio, input or text-rendering library); the content build tool, a development tool under `Tools/` ([ADR-059](ADR-059-content-build-and-validators.md)); and one test project `<Project>Tests` per library ([ADR-069](ADR-069-test-framework.md)). The libraries they link are projects too. A product is built by building its project through the solution (`/t:<ProjectName>`); no macro or variable selects a product.
* **Configurations:** `Debug` and `Release` only. What each compiles in:

| Feature | `Debug` | `Release` |
|---|---|---|
| optimisation | off | on |
| development checks (`ENGINE_ASSERT`) | compiled in | compiled out |
| release checks (container bounds checks, the fatal path) | compiled in | compiled in |
| developer console, debug commands, development overlay and windows, development drawing, test hooks, hot reload of definitions | compiled in | compiled out |
| loose content files ([ADR-058](ADR-058-pack-files-and-mount-policy.md)) | allowed, by an explicit start-up option | compiled out |
| logging at info level and above, the black box, crash context ([ADR-065](ADR-065-crash-reporting.md), [ADR-067](ADR-067-logging.md)) | compiled in | compiled in |
| debug-level and trace log records | compiled in | compiled out |
| PIX markers and GPU timestamps ([ADR-068](ADR-068-profiling-and-gpu-timing.md)) | compiled in | compiled in |
| opt-in telemetry ([ADR-066](ADR-066-telemetry-with-consent.md)) | compiled in | compiled in |

* **Mechanism:** one engine header derives the development switch `ENGINE_DEVELOPMENT` from `NDEBUG` (1 when `NDEBUG` is not defined, 0 otherwise), and development code is compiled under it. No project setting other than those of `AGENTS.md` §3 differs between the configurations, and no macro is set by hand.
* **Runtime feature flags** are data: an edition manifest in the content build ([ADR-059](ADR-059-content-build-and-validators.md)) lists the enabled features (for example which match modes, maps and supply-crate sets a test build offers). Flags are read once at start-up and are immutable while a session runs. Flags that change simulation behaviour are included in the simulation content identity.
* **Unknown flag names** in an edition manifest are a content-build error.

### Consequences

* Good, because each program is one project, and its configuration is one of two names that every tool understands.
* Good, because development features are absent from Release binaries, not merely disabled.
* Good, because test editions differ only in data, so they run release binaries.
* Bad, because there is no optimised build with development tools: the development overlay, debug commands and development checks exist only in the unoptimised Debug build. Performance work on an optimised build uses PIX captures (the markers stay in Release), the telemetry data set and the repeatable performance scene ([ADR-066](ADR-066-telemetry-with-consent.md)), and Debug timings are not representative.
* Bad, because every runtime flag needs a test with the flag on and off.

### Confirmation

* `Build\CheckProjectFiles.py` confirms that Debug and Release differ only in the settings of `AGENTS.md` §3 ([ADR-054](ADR-054-build-system-cmake-presets.md)).
* Before a release, the shipping builder checks the Release binaries for the developer-console and loose-file symbols and fails if any is present.
* The content build rejects edition manifests with unknown flags.

## Pros and Cons of the Options

### One project per product, `NDEBUG`-keyed development features, runtime flags

* Good, because compile-time selection is limited to what must not ship, and it rides on the one switch the two configurations already differ in.
* Neutral, because the flag set must be designed with the content identity in mind.

### Further configurations for development and profiling

* Good, because an optimised build with development tools would exist.
* Bad, because `AGENTS.md` §3 allows exactly `Debug|x64` and `Release|x64`.

### Runtime flags only

* Good, because there is a single binary.
* Bad, because developer cheats and loose-file mounting would be present in the binaries players run.

## More Information

* Rules: `AGENTS.md` §3 (two configurations, aligned by rule) and §6 (CI builds Debug only).
* Related: [ADR-057](ADR-057-version-identity.md) (each build records its product and configuration in its version identity).
* Component: build tooling and distribution (`BLD`); the edition manifest schema belongs to the data schemas component (`DAT`); the facilities per configuration are listed by the diagnostics component (`DBG`).
* **Amendment 2026-10-17 (AGENTS.md, O21).** Products are projects of the solution, the build has the configurations `Debug` and `Release` only, and development features are compiled into Debug and out of Release under `NDEBUG`, while logging, the black box, PIX markers and opt-in telemetry stay in both. The flavours `Development`, `Profile` and `Shipping` and the product variable are removed; editions and data-driven feature flags are unchanged. Status Accepted, decided by the owner's `AGENTS.md`.
