---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Crash reporting: Windows Error Reporting for the client, with registered context, a symbol store and no upload of credentials

## Context and Problem Statement

Species must learn about every client crash and hang, on every thread, with enough context to fix it: a dump with all threads, symbols, the build identity ([ADR-057](ADR-057-version-identity.md)), the current log and a short history of what the engine was doing. Crash data must be collected only with the player's consent and must never contain credentials or private messages. The engine should not ship its own uploader if the operating system already provides a consented path. A dedicated server, possibly on a non-Windows host, needs its own crash path, whose destination is an online-service question for the design phase.

How does the Species client capture and report crashes?

## Decision Drivers

* Coverage of every thread and of hangs, not only the main thread.
* Consent and privacy: no personal data or credentials in reports.
* Symbolised stacks for every distributed build.
* No first-party crash-upload service required for the client in v1.
* Developers and QA get full local dumps.

## Considered Options

* Windows Error Reporting (WER) with registered context and the Windows Desktop Application Program
* An in-process handler with a self-hosted collector (a third-party crash SDK)
* An own exception filter, dump writer and uploader

## Decision Outcome

Proposed option: "Windows Error Reporting (WER) with registered context and the Windows Desktop Application Program". **Default while Proposed:** the platform layer builds the crash path this way. **Confirmed when:** the crash commands of the Confirmation section each produce a dump with the registered files and the black box, and the dump scan finds no seeded secret, before the first external build; the program's crash data needs the code signing of [ADR-073](ADR-073-code-signing.md).

* **No crash swallowing:** the client installs no top-level unhandled-exception filter that handles fatal exceptions; unhandled exceptions on any thread reach WER. The `std::set_terminate` handler, the CRT invalid-parameter handler and the pure-call handler record a reason in the black box and then end the process with a fail-fast exception, so WER captures it.
* **Registered context:** `WerRegisterFile` for the current log ([ADR-067](ADR-067-logging.md)) and the last settings file; `WerRegisterMemoryBlock` for a small black-box ring buffer (the last frame and update markers, the current map or world region, the build identity); `WerRegisterExcludedMemoryBlock` for memory that may hold personal data (chat text, account tokens).
* **Threads:** every thread is created through the platform layer, which names it, so dumps show thread names.
* **GPU faults:** the device-removal path of [ADR-008](ADR-008-gpu-memory-uploads-and-device-removal.md) adds its DRED data to the same report.
* **Collection:** reports reach the publisher through the Windows Desktop Application Program in Partner Center (free; requires the code signing of [ADR-073](ADR-073-code-signing.md)), or through the Partner Center health report for a Store package. Collection follows the player's Windows diagnostic-data settings; the client uploads nothing itself.
* **Symbols:** every Release build that the release procedure packages publishes its PE files and PDBs to a private symbol store keyed by the build identity and uploads its symbols to Partner Center. CI builds publish symbols only once the symbol job that an amendment of [ADR-070](ADR-070-continuous-integration.md) adds exists; until then the workflow uploads the build log and the test results only. Executables carry accurate `VERSIONINFO`.
* **Development and QA:** a registry file that the build provides with every build output enables WER `LocalDumps` for the Species executables on test machines (custom dumps with data segments and thread data).
* **Rule for any later uploader** (for example for a dedicated server): explicit opt-in, HTTPS only, no credentials in the binary or in the report.
* **Dedicated server:** the dedicated server is a Windows build ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)), so WER with the same registered context covers it; where its reports go is a design-phase decision together with server operations. A crash path for another server operating system needs a later ADR.

### Consequences

* Good, because WER covers every thread and hangs without first-party upload code or a collection service.
* Good, because consent is handled by the operating system's diagnostic-data settings.
* Bad, because Partner Center shows data only after an application is installed on at least 100 devices, and keeps failure hits for 30 days through its analytics API.
* Bad, because a dedicated server for another operating system would need a second solution, decided with that server.

### Confirmation

* A test build exposes debug commands that crash on the main thread, on a worker thread, through `std::terminate` and through a hang; QA checks that each produces a dump with the registered files and black box.
* The release procedure checks that every distributed PE file has symbols in the symbol store before a release is published.
* A test scans a captured dump for the strings of a seeded chat message and account token and expects none.

## Pros and Cons of the Options

### WER with registered context

* Good, because it is in-box, consented by the OS and free to receive.
* Neutral, because analysis happens in Partner Center or with the dumps from `LocalDumps`.

### In-process handler with a self-hosted collector

* Good, because data arrives immediately and the same path could serve a non-Windows server.
* Bad, because a third-party crash SDK is excluded from the executable by `AGENTS.md` R14.
* Bad, because Species would run a collection service, which is an online-service decision for the design phase, and would need its own consent prompt.

### Own filter, dump writer and uploader

* Bad, because it duplicates WER, tends to cover only some threads, and needs a first-party service and consent flow.

## More Information

* Facts and sources: technology baseline item 11.1 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the WER guide and API pages, the `LocalDumps` settings, the Windows Desktop Application Program, the Partner Center health report.
* Components: the platform layer (`PLT`) owns the crash path (the terminate, invalid-parameter and pure-call handlers, the WER registrations, the exclusions and thread naming); build tooling and distribution (`BLD`) owns the symbols, signatures, version resources and the test-machine `LocalDumps` settings; diagnostics, test and performance (`DBG`) owns the black box and the crash debug commands.
* **Amendment 2026-10-17 (AGENTS.md, O21).** The dedicated server is a Windows build, so WER covers it too; a third-party crash SDK is excluded by `AGENTS.md` R14.
* **Amendment 2026-10-17.** The crash path is the platform layer's (PLT-118 to PLT-125), as the build tooling (BLD-004) and diagnostics components state; the system summary and the crash context hold no Windows user name and write paths relative to placeholders (PLT-035, PLT-125).
* **Amendment 2026-10-17 (`build.yml`, `AGENTS.md` §6).** CI builds `Debug|x64` only and uploads the build log and the test results only, so publishing symbols is a step of the release procedure of the build tooling (BLD-111, BLD-122): every Release build that it packages publishes its PE files and PDBs to the symbol store and to Partner Center, and CI builds join once the symbol job of an ADR-070 amendment exists (BLD table 4.9-B). The symbol check before a release is a release-procedure check, and the test-machine `LocalDumps` registry file comes with every build output (BLD-113).
