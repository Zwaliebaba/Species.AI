---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Telemetry: opt-in performance telemetry over HTTPS, with a fixed data set and no secrets in the client

## Context and Problem Statement

Species needs field data to set performance budgets and minimum specifications: frame-time distributions, simulation update cost, crowd sizes, network traffic and latency, and a summary of the player's hardware. Collecting this data needs the player's informed, revocable consent, must not collect personal data beyond what the data set lists, and must not rely on secrets embedded in the client, which any player could extract. Where the data is sent and how it is stored are online-service decisions for the design phase.

What does the Species client collect for telemetry, and under what rules?

## Decision Drivers

* Opt-in consent, off by default, revocable at any time.
* A fixed, documented data set; nothing outside it.
* Hardware information gathered in-process, without launching external programs.
* No credentials or upload keys in distributed binaries.
* Independence from the collection service the design phase chooses.

## Considered Options

* Opt-in HTTPS telemetry with a fixed data set and no embedded secrets
* No telemetry
* Telemetry through a third-party analytics SDK

## Decision Outcome

Chosen option: "Opt-in HTTPS telemetry with a fixed data set and no embedded secrets".

* **Consent:** off by default. The first-run screen and the options screen ask with plain text that lists the data set; the choice is stored in the settings and can be withdrawn at any time, which deletes unsent data.
* **Data set (per session):** frame-time percentiles (50th, 95th, 99th) rather than a frames-per-second integer; simulation update time percentiles; peak and mean entity counts; network bytes in and out per second and round-trip latency; the build identity ([ADR-057](ADR-057-version-identity.md)); a hardware summary read in-process (DXGI adapter description and driver version, `GetSystemInfo`, `GlobalMemoryStatusEx`, the display mode). No player names, chat, account ids or IP addresses are part of the data set.
* **Transport:** HTTPS only, through the platform layer's HTTPS client, whose stack is chosen in [ADR-077](ADR-077-client-http-stack.md); batched, retried with back-off, and dropped after a bounded local queue size.
* **No secrets:** the client embeds no credential. Submissions carry the build identity and a random per-installation id that the player can reset; the collector treats every submission as untrusted input.
* **Repeatable performance scene:** a recorded, repeatable scene that the client can run headless or rendered, reporting the same data set, serves QA and players who want a performance check.
* **Out of scope:** the collection endpoint, storage and retention (online services, design phase).

### Consequences

* Good, because performance budgets can be based on measured distributions.
* Good, because consent, data minimisation and the absence of client secrets are fixed before any service exists.
* Bad, because opt-in data under-represents players who decline, so budgets must also rely on QA hardware.

### Confirmation

* A test with consent off checks that no telemetry request is made and nothing is queued.
* A schema check rejects any telemetry field not in the documented data set.
* A CI scan of distributed binaries rejects embedded keys or tokens.

## Pros and Cons of the Options

### Opt-in HTTPS telemetry, fixed data set, no secrets

* Good, because it gives the data the team needs with the least personal data.
* Neutral, because a collection service must exist before the client sends anything.

### No telemetry

* Good, because it has no privacy cost.
* Bad, because minimum specifications and budgets would rest on lab hardware only.

### A third-party analytics SDK

* Good, because collection and dashboards come ready-made.
* Bad, because the SDK decides what it collects and where it sends it, which conflicts with a fixed data set and with the design phase's service choices.
* Bad, because `AGENTS.md` R14 excludes third-party code from the executable.

## More Information

* Related: [ADR-065](ADR-065-crash-reporting.md) (crash data uses its own, OS-consented path), [ADR-068](ADR-068-profiling-and-gpu-timing.md) (the timing sources).
* Component: diagnostics, test and performance (`DBG`); the HTTPS client belongs to the platform layer (`PLT`).
* **Amendment 2026-10-17.** The scene is called the repeatable performance scene, as the diagnostics component names it (DBG-183). The HTTPS client is the platform layer's (PLT-099), on the stack of [ADR-077](ADR-077-client-http-stack.md).
* **Amendment 2026-10-17 (AGENTS.md, O21).** Telemetry is compiled into Debug and Release alike ([ADR-056](ADR-056-editions-build-flavours-and-feature-flags.md)); a third-party analytics SDK is excluded by `AGENTS.md` R14.
