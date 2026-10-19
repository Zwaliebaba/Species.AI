# Packaging, code signing and crash reporting

> Status: Engine baseline · Prefix: BLD · ADRs: [ADR-051](../../../ADR/ADR-051-windows-toolchain-and-compiler-policy.md) · [ADR-052](../../../ADR/ADR-052-cpp23-language-standard.md) · [ADR-053](../../../ADR/ADR-053-modules-and-import-std.md) · [ADR-054](../../../ADR/ADR-054-build-system-msbuild-solution.md) · [ADR-055](../../../ADR/ADR-055-dependencies-three-nuget-packages.md) · [ADR-056](../../../ADR/ADR-056-products-configurations-and-feature-flags.md) · [ADR-057](../../../ADR/ADR-057-version-identity.md) · [ADR-059](../../../ADR/ADR-059-content-build-and-validators.md) · [ADR-060](../../../ADR/ADR-060-content-identity-by-hash.md) · [ADR-065](../../../ADR/ADR-065-crash-reporting.md) · [ADR-070](../../../ADR/ADR-070-continuous-integration.md) · [ADR-071](../../../ADR/ADR-071-install-layout-runtime-files-and-user-data.md) · [ADR-072](../../../ADR/ADR-072-distribution-channels-and-package-formats.md) · [ADR-073](../../../ADR/ADR-073-code-signing.md) · also [ADR-001](../../../ADR/ADR-001-direct3d-12-renderer-on-the-agility-sdk.md), [ADR-003](../../../ADR/ADR-003-shader-language-compiler-and-material-shaders.md), [ADR-017](../../../ADR/ADR-017-texture-container-and-block-compression.md), [ADR-050](../../../ADR/ADR-050-client-platform-and-minimum-os.md), [ADR-058](../../../ADR/ADR-058-pack-files-and-mount-policy.md), [ADR-061](../../../ADR/ADR-061-data-schemas-and-versioning.md), [ADR-062](../../../ADR/ADR-062-authoring-syntax-and-schema-language.md), [ADR-063](../../../ADR/ADR-063-localisation.md), [ADR-066](../../../ADR/ADR-066-telemetry-with-consent.md), [ADR-067](../../../ADR/ADR-067-logging.md), [ADR-068](../../../ADR/ADR-068-profiling-and-gpu-timing.md), [ADR-069](../../../ADR/ADR-069-test-framework.md) · Related: [Asset pipeline and resources](../asset_pipeline_and_resources.md) · [Data schemas](../data_schemas/README.md) · [Platform layer](../platform_layer.md) · [Diagnostics, test and performance](../diagnostics_test_and_performance/README.md) · [Maps: rules and conventions](../../../reference/08_maps_and_level_design/rules_and_conventions.md) · [Map editor hooks](../../../reference/08_maps_and_level_design/map_editor_hooks.md) · [Options, localisation and hooks](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) · [Game assets](../../../assets/GameAssets.md)

This document holds §4.10 to §4.12 of the build tooling and distribution component ([README](README.md)); section numbers are those of the whole component, and the requirements keep their ids (BLD-096 to BLD-115).

## 1 Responsibilities and boundaries

As [README](README.md) §1 states for the whole component.

## 2 Interfaces

The build definition, the content build command line and the data contracts are in [README](README.md) §2.

## 3 Data model

Products, edition manifests, build identity and the install directory are in [README](README.md) §3.

## 4 Behaviour and algorithms

### 4.10 Client packaging and distribution

**BLD-096** (MUST) Packaging assembles the install directory of table 3.5-A from the outputs of one Release build of the client project and the packs of one client edition, never from files of the repository checkout. Every input it declares must exist; a missing input fails packaging, and no input is skipped.

**BLD-097** (MUST) Packaging then checks the directory, and fails on the first broken rule: every file is listed by table 3.5-A and none is forbidden by BLD-024; the Agility SDK check of BLD-042 passes; the output of `--write-build-identity` of every executable equals `build_identity.json` (BLD-063); the version resource of every PE file matches the identity (BLD-059); and the SHA-256 of the content manifest equals the hash in `build_identity.json`.

**BLD-098** (MUST NOT) No executable is compressed, packed or patched after linking. The file that is signed and whose symbols are published is the file that the linker wrote, so that debuggers and crash dumps match it (ADR-065).

**BLD-099** (MUST) The prerequisites are the GameInput redistributable `GameInputRedist.msi` of the pinned `Microsoft.GameInput` package (`bld.pin.gameinput`), which the installer or channel wrapper runs, never the game, and which never downgrades a newer installed runtime, and the Visual C++ redistributable of the toolset. A channel that cannot run prerequisites receives the Visual C++ runtime DLLs beside the executable instead. No graphics or audio runtime installer is bundled, because Direct3D 12 and XAudio 2.9 are part of every supported Windows (ADR-071, ADR-050).

**BLD-100** (MUST) Packaging produces the packages of table 4.10-A, each a wrapper around the one install directory (ADR-072).

Table 4.10-A Packages.

| Package | Built from | Contents | Signed | Use |
|---|---|---|---|---|
| developer install | either configuration and any client edition | the install directory; in Debug builds the loose-content option is available (AST-105) | no, or self-signed | local testing |
| release-candidate archive | a Release client and an edition marked for release | the install directory, a `prerequisites\` folder with the two prerequisites, and a setup script that runs them | the archive and every PE file (§4.11) | internal, QA and closed-test distribution |
| channel wrapper | one channel that the design phase chooses | generated from the same install directory: a storefront depot definition; an MSIX package, Store-signed, or publisher-signed with an `.appinstaller` update feed; or a signed offline MSI or EXE installer | per channel (§4.11) | players |
| server archive | a Release server and a server edition | a plain archive of the server program, its packs, `build_identity.json` and its licence notices | every PE file | community hosts; how hosts receive it is a design-phase decision (ADR-071) |
| symbol package | every Release build that is packaged | the PE files and PDBs of every product | never distributed | the symbol store (§4.12) |

**BLD-101** (MUST) The install directory stays compatible with MSIX: read-only at run time, with no registry writes, no service and no driver (ADR-071). Before the Microsoft Store MSIX channel is chosen, a test submission confirms that a package holding the Agility SDK runtime and the GameInput prerequisite passes Store certification (ADR-072).

**BLD-102** (MUST) Updates are delivered by the channel: the storefront client, the Store's automatic update or the `.appinstaller` feed. Species has no first-party patcher and no launcher (ADR-072). An update replaces the packs of the install as one edition, so an install always holds the packs of exactly one edition (BLD-052).

**BLD-103** (MUST) The client offers the **repair check** as the start-up switch `--verify-content`. It mounts the packs of the install directory, runs the asset pipeline's check of every entry of every pack (AST-107), writes the result to the log, shows the result through the platform message path with the texts of table 4.10-B, and ends with the normal exit code `plt.lifecycle.exit_code.normal` ([platform layer](../platform_layer.md) PLT-044) when every pack passes and with `bld.repair.exit_failed` otherwise. It never writes to the install directory; restoring a failed pack is the channel's repair or update function (ADR-072). The start-up messages `ui.error.content_missing` and `ui.error.content_damaged` (AST-102) lead the player to this path.

Table 4.10-B Repair-check texts. The texts are new English text in the base language.

| String id | Text |
|---|---|
| `ui.repair.passed` | "Every game file was checked and is intact." |
| `ui.repair.failed` | "Some game files are damaged: {packs}. Use your store's repair function or reinstall the game." |

**BLD-104** (MUST) Uninstalling removes the install directory only; user data stays unless the player chooses to delete it (ADR-071).

**BLD-105** (MUST) Every release-candidate archive is tested on a clean image of a primary test target of ADR-050, in the release procedure or by a CI job that an amendment of ADR-070 adds: the test unpacks the archive, runs the setup script, starts the client as a standard user from a read-only directory, and checks that nothing is written outside the user-data folders (ADR-071, ADR-072). Each channel wrapper, once added, has its own install, update and uninstall test (ADR-072).

### 4.11 Code signing

**BLD-106** (MUST) These files are signed: `Species.exe`, `SpeciesServer.exe`, every first-party DLL, every tool executable that is distributed, every installer file of the release-candidate archive, and every publisher-signed package (ADR-073). The Microsoft Store signs its MSIX packages itself.

**BLD-107** (MUST) Every signature uses SHA-256 with an RFC 3161 timestamp (`SignTool sign /fd SHA256 /tr <timestamp URL> /td SHA256`) and is made in the release procedure, on Release builds only (ADR-073); Debug builds and local builds are unsigned or self-signed. The signing service is Azure Artifact Signing when the publishing entity is eligible, and otherwise an OV certificate held in a hardware-backed or cloud key store. Signing keys never reach a developer machine's disk or the repository: the signing service or the key store holds them, the signing step is the only path to them, and a repository scan rejects certificate and key files (ADR-073).

**BLD-108** (MUST) Before a release candidate is published, a check verifies the signature and the timestamp of every PE file and package in it, and fails on any file that is unsigned, signed by another certificate or not timestamped (ADR-073).

**BLD-109** (MUST) The publisher registers the signing certificate with the Windows Desktop Application Program, so that crash data covers every signed executable (ADR-073, ADR-065).

### 4.12 Crash reporting integration

**BLD-110** (MUST) The client's crash path is that of ADR-065, installed by the [platform layer](../platform_layer.md): no handler swallows a fatal exception, unhandled exceptions on every thread reach Windows Error Reporting, and the platform layer registers the current log, the last settings file, the black box and the excluded memory. The build provides what makes those reports usable: symbols, version resources, signatures and the test-machine settings of this section.

**BLD-111** (MUST) Every Release build that is packaged publishes its PE files and PDBs to the private symbol store (BLD-026), and so does every CI build once the symbol job of table 4.9-B exists. Release builds also upload their symbols to Partner Center. No release is published until a check confirms that every distributed PE file has its PDB in the symbol store (ADR-065).

**BLD-112** (MUST) Every executable carries the version resource of BLD-059, by which the Windows Desktop Application Program groups executables (ADR-057), and is signed (§4.11), which that program requires (ADR-073).

**BLD-113** (MUST) The build provides, with every build output, a registry file that enables Windows Error Reporting local dumps for `Species.exe` and `SpeciesServer.exe` on test machines: custom dumps (`DumpType` `bld.crash.local_dump_type`) with data segments and thread data, at most `bld.crash.local_dump_count` dumps per program (`DumpCount`), written to the default user-data `crashes\` folder of ADR-071 (`DumpFolder`). A `--user-data` root (PLT-024, PLT-025) does not move it: dumps always go to the default user-data `crashes\` folder that the registry file names. Applying the file needs administrator rights, and it is never part of a player package. Dumps from test machines are artefacts of the jobs that run them (ADR-070).

**BLD-114** (MUST NOT) No player package contains a dump-writing helper, a support tool or a crash uploader. The client uploads nothing itself, and crash collection follows the player's Windows diagnostic-data settings (ADR-065). Any later uploader, for example for a dedicated server, is opt-in, uses HTTPS only and carries no credential in its binary or its report (ADR-065).

**BLD-115** (MUST) The server, a Windows build (ADR-051), uses the symbol publication, version resources and signing of the client, and Windows Error Reporting covers its crashes (ADR-065). Where the dedicated server's crash reports go, and the build, symbols and artefact signing of a server for another host operating system, are design-phase decisions (ADR-065, ADR-073; "Hosting" in the [ADR index](../../../ADR/README.md#deferred-to-the-design-phase)).

## 5 Concurrency and performance budget

As [README](README.md) §5 states.

## 6 Failure modes, diagnostics and telemetry

As [README](README.md) §6 states.

## 7 Acceptance tests

The tests of these sections are in [README](README.md) §7.

## 8 Open decisions

The decisions of these sections are in [README](README.md) §8.
