---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Products, build flavours and feature flags: one preset per product and flavour, data-driven runtime flags

## Context and Problem Statement

Species produces several programs from one codebase: the client (which can also host the integrated server, vision O10), the headless dedicated server (vision O5), and the content tools. Each is built in flavours with different diagnostics. Some content or features differ between products or test builds (for example a test build that enables only some match modes). Product variants must be explicit, must never be selected by editing a source file, and a build must never be ambiguous about which product it is.

How are products, build flavours and feature variants expressed?

## Decision Drivers

* Exactly one product per build, chosen by the build definition, never by editing source.
* Diagnostics that cannot reach shipping binaries stay out of them by construction.
* Feature variation without recompiling where possible, so that test builds reuse shipping binaries.
* Simulation-relevant flags must be part of the content identity ([ADR-060](ADR-060-content-identity-by-hash.md)).

## Considered Options

* Build presets per product and flavour, plus data-driven runtime feature flags
* Compile-time feature macros selected in a shared header
* Runtime flags only, one binary for everything

## Decision Outcome

Chosen option: "Build presets per product and flavour, plus data-driven runtime feature flags".

* **Products:** `client`, `server` (headless; links no graphics, audio, input or text-rendering library) and `tools`. Each is a separate CMake target set; the variable `SPECIES_PRODUCT` names it. Configure fails unless it is exactly one of the known values, so a build with no product or two products cannot be produced.
* **Build flavours** (the configurations of [ADR-054](ADR-054-build-system-cmake-presets.md)):

| Flavour | Optimised | Assertions | Developer console and debug commands | PIX markers | Loose content files ([ADR-058](ADR-058-pack-files-and-mount-policy.md)) |
|---|---|---|---|---|---|
| `Debug` | no | all | compiled in | compiled in | allowed (opt-in) |
| `Development` | yes | all | compiled in | compiled in | allowed (opt-in) |
| `Profile` | yes | release checks only | compiled out | compiled in | not compiled in |
| `Shipping` | yes | release checks only | compiled out | compiled out | not compiled in |

* **Compile-time switches** exist only for code that must not be present in a binary: the developer console and debug commands, PIX markers ([ADR-068](ADR-068-profiling-and-gpu-timing.md)), loose-file mounting and test hooks. Each is a CMake option derived from the flavour, never a hand-edited macro.
* **Runtime feature flags** are data: an edition manifest in the content build ([ADR-059](ADR-059-content-build-and-validators.md)) lists the enabled features (for example which match modes, maps and supply-crate sets a test build offers). Flags are read once at start-up and are immutable while a session runs. Flags that change simulation behaviour are included in the simulation content identity.
* **Unknown flag names** in an edition manifest are a content-build error.

### Consequences

* Good, because each binary states its product and flavour unambiguously, and the build refuses ambiguous selections.
* Good, because test editions differ only in data, so they run the same binaries as shipping.
* Good, because diagnostics excluded from shipping builds are absent from the binary, not merely disabled.
* Bad, because every runtime flag needs a test with the flag on and off.

### Confirmation

* A configure test runs CMake with no product, with an unknown product and with two products, and expects each to fail.
* A CI check scans shipping binaries for the developer-console and loose-file symbols and fails if any is present.
* The content build rejects edition manifests with unknown flags.

## Pros and Cons of the Options

### Presets per product and flavour plus runtime flags

* Good, because compile-time selection is limited to what must not ship.
* Neutral, because the flag set must be designed with the content identity in mind.

### Compile-time feature macros in a shared header

* Bad, because changing a variant means editing source, and a header can define zero or several variants by mistake.
* Bad, because every variant needs its own binary and its own test pass.

### Runtime flags only

* Good, because there is a single binary.
* Bad, because developer cheats, loose-file mounting and markers would be present in shipping binaries.

## More Information

* Related: [ADR-057](ADR-057-version-identity.md) (each build records product and flavour in its version identity).
* Component: build tooling and distribution (`BLD`); the edition manifest schema belongs to the data schemas component (`DAT`).
* Whether PIX markers stay in shipping builds for field captures is a design-phase choice; the default here is compiled out.
