---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Code signing: every distributed PE file and package signed in CI with SHA-256 and an RFC 3161 timestamp

## Context and Problem Statement

Windows SmartScreen strongly blocks unsigned or self-signed programs and warns about newly signed ones until they build reputation. The Windows Desktop Application Program, which delivers crash data ([ADR-065](ADR-065-crash-reporting.md)), only covers executables signed with a certificate the publisher has registered. The Microsoft Store signs MSIX packages itself but requires MSI/EXE installers and their PE files to be signed with a certificate from a CA in the Microsoft Trusted Root Program. Signing keys are high-value secrets and must never be exposed on developer machines.

How, where and with what does Species sign its binaries and packages?

## Decision Drivers

* Every distributed PE file and package is signed and timestamped.
* Signing keys are never on developer machines or in the repository.
* Publisher reputation tied to a verified identity.
* Cost and eligibility of the signing service.

## Considered Options

* Azure Artifact Signing from CI, with an OV certificate in a hardware-backed or cloud key store as the fallback
* An OV certificate in a hardware-backed or cloud key store
* An EV certificate
* Store signing only

## Decision Outcome

Chosen option: "Azure Artifact Signing from CI, with an OV certificate in a hardware-backed or cloud key store as the fallback". This is the default the team proceeds with; which branch applies depends on whether the publishing entity meets Artifact Signing's eligibility (supported regions and, for organisations, a verifiable tax history of three years or more).

* **What is signed:** `species.exe`, the dedicated server's Windows executable, every first-party DLL and tool executable distributed with the game, and every installer or publisher-signed package.
* **How:** `SignTool sign /fd SHA256 /tr <timestamp URL> /td SHA256 <certificate or signing-service options> <file>`, so every signature has an RFC 3161 timestamp.
* **Where:** a CI signing step on release-candidate and release builds only; Debug builds and local builds are unsigned or self-signed for local testing.
* **Keys:** held by the signing service or the key store; the CI step is the only path to them.
* **Registration:** the publisher registers the certificate with the Windows Desktop Application Program, so crash data covers every signed executable.
* **MSIX in the Store:** the Store signs the package; no certificate is needed for that channel.
* **Non-Windows server artefacts** (if the design phase adds them) are published with checksums and a detached signature whose scheme the design phase chooses.

### Consequences

* Good, because every distributed binary is attributable and eligible for crash data.
* Good, because keys never leave the signing service or key store.
* Bad, because new signatures still meet SmartScreen warnings until reputation builds; EV certificates no longer bypass this.
* Bad, because Artifact Signing has a monthly cost and eligibility limits; the OV fallback costs more per year and needs a key store.

### Confirmation

* A CI check verifies the signature and timestamp of every PE file in a release candidate before publication.
* A repository scan rejects certificate and key files.

## Pros and Cons of the Options

### Azure Artifact Signing with OV fallback

* Good, because Microsoft names it the recommended option for non-Store distribution; it integrates with CI and ties reputation to the verified publisher (about USD 9.99 a month; short-lived certificates).
* Bad, because eligibility depends on region and, for organisations, tax history.

### OV certificate in a key store

* Good, because it is available to any registered organisation.
* Bad, because it costs about USD 150–500 a year, depending on the CA, and the key store must be operated.

### EV certificate

* Bad, because it costs more than OV and, since 2024, gives no SmartScreen advantage over OV.

### Store signing only

* Bad, because it covers only Store MSIX packages, not storefront depots, installers or the server.

## More Information

* Facts and sources: technology baseline items 11.2 and 11.3 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): Microsoft's code-signing options page, the first-app publishing guide and the MSIX signing guides on learn.microsoft.com.
* Related: [ADR-072](ADR-072-distribution-channels-and-package-formats.md).
* Component: build tooling and distribution (`BLD`).
* **Amendment 2026-10-17 (AGENTS.md, O21).** Signing applies to Release builds only, since CI builds Debug only and the build has no other configuration ([ADR-054](ADR-054-build-system-cmake-presets.md), [ADR-070](ADR-070-continuous-integration.md)); the signing step runs where Release is built.
