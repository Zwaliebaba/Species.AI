---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Distribution channels and package formats: thin wrappers around the install directory; storefronts chosen by the design phase

## Context and Problem Statement

Species can reach players through storefront depots (for example Steam or the Epic Games Store), the Microsoft Store as an MSIX or MSI/EXE package, direct download of a signed MSIX with an `.appinstaller` update feed, or a signed installer. Which storefronts Species uses is a business and online-services choice for the design phase. The engine needs a default now, so that the release procedure produces installable builds from the first playable onwards, and so that no engine choice closes a channel.

What does the build produce for distribution, and how does each channel get its package?

## Decision Drivers

* Installable, signed builds for the team and testers from the first playable.
* No engine decision that excludes a storefront or the Microsoft Store.
* Updates delivered by the channel, not by a first-party patcher.
* Every package is a wrapper around the one install layout of [ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md).

## Considered Options

* Install directory plus a signed archive now; channel wrappers added when channels are chosen
* MSIX as the single package format for every channel
* An MSI or EXE installer for every channel
* A first-party launcher with its own updater

## Decision Outcome

Chosen option: "Install directory plus a signed archive now; channel wrappers added when channels are chosen". This is the default the team proceeds with.

* **Release-candidate output:** for every release candidate, the release procedure (a `Release|x64` build by whoever ships, because CI builds `Debug|x64` only: `AGENTS.md` §6, [ADR-070](ADR-070-continuous-integration.md)) produces the install directory and a signed archive of it, with a small setup script that runs the two prerequisites of [ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md). This serves internal, QA and closed-test distribution.
* **Channel wrappers** are added per chosen channel, each generated from the same directory: a storefront depot definition; an MSIX package (Store-signed for the Microsoft Store, or publisher-signed with an `.appinstaller` feed for direct download); or a signed offline MSI/EXE installer for the Store's MSI/EXE path.
* **Updates:** delivered by the channel (storefront client, Store auto-update, `.appinstaller` feed). Species has no first-party patcher in v1.
* **Before choosing the Microsoft Store MSIX channel:** a test submission confirms that a package containing the Agility SDK runtime and the GameInput prerequisite passes Store certification. The Windows App Certification Kit is deprecated, and certification runs at submission in Partner Center.
* **Package version fields** are generated from the build identity ([ADR-057](ADR-057-version-identity.md)).

### Consequences

* Good, because builds are installable from the start without committing to a storefront.
* Good, because each later channel is a wrapper, not a change to the game.
* Bad, because a channel's specific requirements (Store certification, storefront overlays, entitlement checks) are found only when its wrapper is built; entitlement and platform services are design-phase topics.

### Confirmation

* An install test unpacks the archive on a clean Windows 11 image, runs the setup script as a standard user and starts the client.
* Each channel wrapper, once added, has an install, update and uninstall test, run in the release procedure or by a CI job that an amendment of [ADR-070](ADR-070-continuous-integration.md) adds.

## Pros and Cons of the Options

### Install directory plus signed archive, wrappers later

* Good, because it decouples engine work from the channel decision.
* Neutral, because the archive is not a consumer distribution format.

### MSIX for every channel

* Good, because it gives clean install, update and uninstall, and Store signing is free.
* Bad, because storefront depots do not use MSIX, and Store certification of the Agility SDK runtime is not yet confirmed.

### MSI or EXE installer for every channel

* Good, because it works everywhere outside storefront depots.
* Bad, because it needs a trusted code-signing certificate and leaves updates to Species.

### First-party launcher with its own updater

* Bad, because it duplicates what every channel provides and adds an online service to operate.

## More Information

* Facts and sources: technology baseline item 11.2 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): Store publishing FAQ, distribution-path choice and MSIX package requirements on learn.microsoft.com. The Windows App Certification Kit status was re-checked for this ADR because the baseline marked it unverified: "Windows App Certification Kit (WACK) is deprecated and no longer maintained" and "Official certification is performed automatically when you submit your package to Partner Center", https://learn.microsoft.com/windows/msix/package/packaging-uwp-apps#validate-your-app-package (checked 2026-10-17). Whether a package with the Agility SDK passes Store certification remains unverified.
* Related: [ADR-073](ADR-073-code-signing.md).
* Component: build tooling and distribution (`BLD`).
* **Amendment 2026-10-17 (AGENTS.md §6, `build.yml`).** CI builds `Debug|x64` only and produces no release candidate. The install directory, the signed archive and its setup script come from the release procedure that whoever ships runs before a release (build tooling BLD-122), and the install tests run there or in a CI job added by amendment of ADR-070 (BLD-105). The context, the release-candidate output and the confirmation say so.
