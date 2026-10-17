---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Install layout: one relocatable, read-only install directory with the Agility SDK beside the executable; user data in known folders

## Context and Problem Statement

The Species client must install and run the same way whatever distribution channel the design phase chooses ([ADR-072](ADR-072-distribution-channels-and-package-formats.md)). Direct3D 12 and XAudio 2.9 are part of every supported Windows version ([ADR-050](ADR-050-client-platform-and-minimum-os.md)); the Agility SDK runtime must travel with the game; GameInput requires its redistributable in PC installers; and the dynamic CRT ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)) needs the Visual C++ runtime. Player data (settings, logs, crash data, screenshots) must survive updates, never live inside the install directory, and be easy for support to find.

What does a Species client installation contain, and where does the client write data?

## Decision Drivers

* One install layout that every channel wraps without changes.
* Compatibility with MSIX: a read-only install directory, no registry writes, no services or drivers.
* Only the runtime files Species needs; nothing the OS already provides.
* User data in per-user known folders under the studio and game name.

## Considered Options

* A relocatable, read-only install directory with user data in known folders
* An installer that writes beside the executable and into the registry
* A per-channel layout

## Decision Outcome

Chosen option: "A relocatable, read-only install directory with user data in known folders".

* **Install directory:**
  * `species.exe` (the client) and first-party DLLs;
  * `D3D12\` with `D3D12Core.dll` from **Agility SDK 1.619.6** (chosen in [ADR-001](ADR-001-direct3d-12-renderer-on-the-agility-sdk.md)); `species.exe` exports `D3D12SDKVersion = 619` and `D3D12SDKPath = ".\\D3D12\\"`. `D3D12SDKLayers.dll` and preview SDKs are never distributed;
  * `packs\` with the pack files, the mount manifest and the content manifest of exactly one edition ([ADR-058](ADR-058-pack-files-and-mount-policy.md), [ADR-060](ADR-060-content-identity-by-hash.md));
  * `build_identity.json`, the full build identity written by packaging ([ADR-057](ADR-057-version-identity.md));
  * `channel_tag.txt`, the tag of the channel that delivered the install, written by the channel wrapper (channel packages only);
  * `licences\` with the third-party licence notices.
* **Prerequisites** run by the installer or channel wrapper: `GameInputRedist.msi` from the pinned GameInput package (it never downgrades a newer installed runtime), and the Visual C++ redistributable for the pinned toolset. A channel that cannot run prerequisites receives the Visual C++ runtime DLLs beside the executable instead. No graphics or audio runtime installer is bundled: Direct3D 12 and XAudio 2.9 are in-box.
* **No writes** to the install directory or the registry at run time. The client runs as a standard user, installs no service and no driver.
* **User data** (from `SHGetKnownFolderPath`): `%LOCALAPPDATA%\<Studio>\Species\` with `settings\`, `logs\` ([ADR-067](ADR-067-logging.md)), `crashes\` (local dumps, [ADR-065](ADR-065-crash-reporting.md)), `maps\` (the player's map folder) and `diagnostics\` (development outputs, in `Debug` and `Development` builds only); screenshots in the user's Pictures folder under `Species\` ([ADR-076](ADR-076-screenshot-encoding.md)). Saved worlds and replays follow the persistence design (design phase), under the same rule: never in the install directory.
* **Uninstall** removes the install directory only; user data stays unless the player chooses to delete it.
* **Dedicated server:** a plain archive of the `server` product and its packs; its packaging for community hosts is a design-phase decision.

### Consequences

* Good, because every distribution channel is a thin wrapper around one tested directory.
* Good, because the MSIX path stays open and updates never touch player data.
* Good, because a missing or mismatched Agility SDK runtime is caught by a build check rather than by players.
* Bad, because two prerequisites (GameInput, Visual C++ runtime) must be handled by every channel wrapper.

### Confirmation

* A build check compares the exported `D3D12SDKVersion` with the minor version of the packaged `D3D12Core.dll`.
* An install test runs the client from a read-only directory as a standard user and checks that nothing is written outside the user-data folders.
* The GameInput redistributable licence terms are read and recorded before the first external build (they were linked but not read when the baseline was checked).

## Pros and Cons of the Options

### Relocatable, read-only install directory

* Good, because it suits every channel, including MSIX.
* Neutral, because each channel still needs its own small wrapper.

### Installer writing beside the executable and into the registry

* Bad, because it breaks MSIX compatibility, needs elevation and loses player data on reinstall.

### Per-channel layout

* Bad, because each layout needs its own test pass and support documentation.

## More Information

* Facts and sources: technology baseline items 4.1, 6, 7 and 11.2 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): the Agility SDK getting-started page, the GameInput NuGet and redistribution pages, and the XAudio2 versions page.
* `<Studio>` is the publisher-name parameter, the studio-name placeholder of CONVENTIONS §8.11; the same form names the user-data folder in the [ADR index](README.md#technology-baseline) and in the decision register.
* Component: build tooling and distribution (`BLD`); runtime paths belong to the platform layer (`PLT`).
* **Amendment 2026-10-17.** The install list gains `build_identity.json` and `channel_tag.txt` and names the licence folder (BLD-020, BLD-021, BLD-023); the user-data root gains `maps\` (R-MAP-391) and `diagnostics\` (development builds only), as the platform layer specifies (PLT-017).
