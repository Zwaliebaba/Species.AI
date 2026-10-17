# Architecture decision records

This folder holds the engine-technology decisions of the Species reference library, one file per decision, in the **MADR 4** format (`NNNN-<slug>.md`). Each record has YAML front matter (`status`, `date`, `decision-makers`) and the sections *Context and Problem Statement*, *Decision Drivers*, *Considered Options*, *Decision Outcome* (with *Consequences* and *Confirmation*), *Pros and Cons of the Options* and *More Information*.

- **Accepted** records state a decision the engine baseline is built on.
- **Proposed** records state a choice that is still open. Each one names the **default** the team proceeds with until the decision is confirmed or changed.
- A record flagged `owner-decision: blocking` in its front matter has no defensible default and waits for the owner. No record numbered 0001–0049 carries that flag.

ADRs cover engine technology only. Game design lives in `reference/` and `decisions/`; the engine components in `architecture/components/` cite these records by number. The topics listed in [Deferred to the design phase](#deferred-to-the-design-phase) have no ADR: the Species design phase decides them (see [the vision brief](../../vision/species-vision-brief.md)).

## Index

Numbers 0001–0049 cover the engine runtime (renderer, textures, meshes, audio, input, UI, animation and VFX). Numbers 0050–0099 cover the toolchain, the content pipeline and operations.

| Number | Title | Status | Date |
|---|---|---|---|
| [ADR-0001](0001-direct3d-12-renderer-on-the-agility-sdk.md) | Direct3D 12 renderer on the Agility SDK | Accepted | 2026-10-17 |
| [ADR-0002](0002-minimum-gpu-capability.md) | Minimum GPU capability: feature level 12_1, Shader Model 6.6, Resource Binding Tier 3 and wave operations | Proposed | 2026-10-17 |
| [ADR-0003](0003-shader-language-compiler-and-material-shaders.md) | Shader language, compiler and material shader set | Accepted | 2026-10-17 |
| [ADR-0004](0004-frame-graph-from-the-ordered-pass-table.md) | Frame graph built from the ordered pass table | Proposed | 2026-10-17 |
| [ADR-0005](0005-pipeline-state-catalogue.md) | Pipeline state catalogue derived from the pass table | Accepted | 2026-10-17 |
| [ADR-0006](0006-root-signature-and-bindless-resource-access.md) | Root signature and bindless resource access | Proposed | 2026-10-17 |
| [ADR-0007](0007-barrier-model.md) | Barrier model: Enhanced Barriers with a resource-state fallback | Proposed | 2026-10-17 |
| [ADR-0008](0008-gpu-memory-uploads-and-device-removal.md) | GPU memory, uploads and device-removal handling | Accepted | 2026-10-17 |
| [ADR-0009](0009-instanced-crowds-sprite-cards-and-shadows.md) | Instanced crowds, sprite cards and shadows | Accepted | 2026-10-17 |
| [ADR-0010](0010-gpu-particles-and-shatter-debris.md) | GPU particles and shatter debris | Accepted | 2026-10-17 |
| [ADR-0011](0011-world-surface-rendering.md) | World-surface rendering: chunked terrain, edge-term wire overlay and GPU water | Accepted | 2026-10-17 |
| [ADR-0012](0012-swap-chain-presentation-and-frame-pacing.md) | Swap chain, presentation and frame pacing | Accepted | 2026-10-17 |
| [ADR-0013](0013-working-colour-space-and-blending.md) | Working colour space and blending | Proposed | 2026-10-17 |
| [ADR-0014](0014-hdr-output.md) | HDR output | Proposed | 2026-10-17 |
| [ADR-0015](0015-scene-anti-aliasing.md) | Scene anti-aliasing | Proposed | 2026-10-17 |
| [ADR-0016](0016-view-projection-and-depth-conventions.md) | View, projection and depth conventions | Proposed | 2026-10-17 |
| [ADR-0017](0017-texture-container-and-block-compression.md) | Texture container and block-compression formats | Accepted | 2026-10-17 |
| [ADR-0018](0018-texture-import-manifest.md) | Texture import manifest: colour space, alpha modes, mips, samplers and tint masks | Accepted | 2026-10-17 |
| [ADR-0019](0019-mesh-source-format-and-cooked-meshes.md) | Mesh source format and cooked runtime meshes | Accepted | 2026-10-17 |
| [ADR-0020](0020-flat-shading-and-team-tint.md) | Flat shading and team tint | Accepted | 2026-10-17 |
| [ADR-0021](0021-mesh-socket-schema.md) | Mesh socket schema | Accepted | 2026-10-17 |
| [ADR-0022](0022-audio-engine-on-xaudio2.md) | Audio engine on XAudio2 2.9 and X3DAudio | Accepted | 2026-10-17 |
| [ADR-0023](0023-virtual-voices-and-voice-stealing.md) | Virtual voices, priorities and voice stealing | Accepted | 2026-10-17 |
| [ADR-0024](0024-audio-buses-and-per-bus-effects.md) | Audio buses, per-bus effects and the mix | Accepted | 2026-10-17 |
| [ADR-0025](0025-audio-formats-codecs-and-residency.md) | Audio source formats, runtime codecs and residency | Proposed | 2026-10-17 |
| [ADR-0026](0026-audio-command-queue-and-callbacks.md) | Audio engine command queue, control rate and callbacks | Accepted | 2026-10-17 |
| [ADR-0027](0027-spatialisation-and-distance-attenuation.md) | Spatialisation and distance attenuation | Accepted | 2026-10-17 |
| [ADR-0028](0028-input-device-back-ends.md) | Input device back-ends | Proposed | 2026-10-17 |
| [ADR-0029](0029-input-event-model-and-binding-contract.md) | Input event model and the binding-graph contract | Accepted | 2026-10-17 |
| [ADR-0030](0030-ui-framework-build-or-buy.md) | UI framework: an own retained framework | Proposed | 2026-10-17 |
| [ADR-0031](0031-batched-ui-rendering.md) | Batched UI rendering | Accepted | 2026-10-17 |
| [ADR-0032](0032-ui-focus-graph-page-stack-and-input.md) | UI focus graph, page stack and input handling | Accepted | 2026-10-17 |
| [ADR-0033](0033-ui-layout-scaling-and-dpi.md) | UI layout, scaling and DPI | Accepted | 2026-10-17 |
| [ADR-0034](0034-text-rendering-and-glyph-cache.md) | Text rendering: DirectWrite shaping and a glyph cache | Proposed | 2026-10-17 |
| [ADR-0035](0035-procedural-animation-runtime.md) | Procedural animation runtime | Accepted | 2026-10-17 |
| [ADR-0036](0036-effect-events-and-presentation-clock.md) | Effect events, presentation clock and effect definitions | Accepted | 2026-10-17 |
| [ADR-0050](0050-client-platform-and-minimum-os.md) | Client platform: Windows 11 x64, minimum version 24H2 (build 26100) | Accepted | 2026-10-17 |
| [ADR-0051](0051-windows-toolchain-and-compiler-policy.md) | Windows toolchain: pinned MSVC Build Tools, Windows SDK and compiler policy | Accepted | 2026-10-17 |
| [ADR-0052](0052-cpp23-language-standard.md) | Language standard: C++23, with a portable subset for shared and server code | Accepted | 2026-10-17 |
| [ADR-0053](0053-modules-and-import-std.md) | Modules: `import std` behind a build option; first-party code stays header-based | Proposed | 2026-10-17 |
| [ADR-0054](0054-build-system-cmake-presets.md) | Build system: CMake presets with the Ninja Multi-Config generator | Proposed | 2026-10-17 |
| [ADR-0055](0055-dependencies-vcpkg-manifest-mode.md) | Third-party dependencies: vcpkg in manifest mode with pinned baseline and overlay triplets | Accepted | 2026-10-17 |
| [ADR-0056](0056-editions-build-flavours-and-feature-flags.md) | Products, build flavours and feature flags: one preset per product and flavour, data-driven runtime flags | Accepted | 2026-10-17 |
| [ADR-0057](0057-version-identity.md) | Version identity: one generated build identity used by every consumer | Accepted | 2026-10-17 |
| [ADR-0058](0058-pack-files-and-mount-policy.md) | Pack files: own indexed, chunked and hashed format; explicit mount order; loose files only in development builds | Accepted | 2026-10-17 |
| [ADR-0059](0059-content-build-and-validators.md) | Content build: one declarative, incremental, validating content build tool | Accepted | 2026-10-17 |
| [ADR-0060](0060-content-identity-by-hash.md) | Content identity: SHA-256 over cooked content, produced by the content build | Accepted | 2026-10-17 |
| [ADR-0061](0061-data-schemas-and-versioning.md) | Data schemas: one schema per record type, text for authoring, versioned binary for runtime | Accepted | 2026-10-17 |
| [ADR-0062](0062-authoring-syntax-and-schema-language.md) | Authoring syntax and schema language: JSON validated by JSON Schema, with a Species code generator | Proposed | 2026-10-17 |
| [ADR-0063](0063-localisation.md) | Localisation: UTF-8 string tables keyed by string id, BCP 47 language tags, ICU formatting and per-key fallback | Accepted | 2026-10-17 |
| [ADR-0064](0064-accessibility-baseline-xag.md) | Accessibility baseline: Xbox Accessibility Guidelines 3.2 as the checklist, with engine features built in from the start | Proposed | 2026-10-17 |
| [ADR-0065](0065-crash-reporting.md) | Crash reporting: Windows Error Reporting for the client, with registered context, a symbol store and no upload of credentials | Proposed | 2026-10-17 |
| [ADR-0066](0066-telemetry-with-consent.md) | Telemetry: opt-in performance telemetry over HTTPS, with a fixed data set and no secrets in the client | Accepted | 2026-10-17 |
| [ADR-0067](0067-logging.md) | Logging: asynchronous, buffered, categorised logging with TraceLogging (ETW) and rotating file sinks | Accepted | 2026-10-17 |
| [ADR-0068](0068-profiling-and-gpu-timing.md) | Profiling: PIX markers through one Species macro layer, GPU timestamp queries and an in-game overlay | Accepted | 2026-10-17 |
| [ADR-0069](0069-test-framework.md) | Test framework: GoogleTest through CTest, plus property, golden and fuzz tests | Proposed | 2026-10-17 |
| [ADR-0070](0070-continuous-integration.md) | Continuous integration: preset-driven pipelines with content validation, analysis builds and a headless bot soak | Accepted | 2026-10-17 |
| [ADR-0071](0071-install-layout-runtime-files-and-user-data.md) | Install layout: one relocatable, read-only install directory with the Agility SDK beside the executable; user data in known folders | Accepted | 2026-10-17 |
| [ADR-0072](0072-distribution-channels-and-package-formats.md) | Distribution channels and package formats: thin wrappers around the install directory; storefronts chosen by the design phase | Proposed | 2026-10-17 |
| [ADR-0073](0073-code-signing.md) | Code signing: every distributed PE file and package signed in CI with SHA-256 and an RFC 3161 timestamp | Proposed | 2026-10-17 |
| [ADR-0074](0074-platform-clock-with-injectable-time.md) | Platform clock: one process-relative monotonic clock behind an injectable time source | Accepted | 2026-10-17 |
| [ADR-0075](0075-application-window-and-dpi-awareness.md) | Application window: a platform-owned Win32 window, per-monitor DPI awareness v2, windowed and borderless full-screen modes | Accepted | 2026-10-17 |
| [ADR-0076](0076-screenshot-encoding.md) | Screenshot encoding: the in-box Windows Imaging Component PNG encoder; video capture by external tools | Proposed | 2026-10-17 |

## Technology baseline

This is a condensed copy of the Species technology standards baseline. Every fact was checked on **2026-10-17** (UTC) against the source named; nothing is stated from memory. Re-check every version at each milestone, and before an ADR that depends on it changes status.

Confidence: **high** means a primary source was read directly and the recommendation follows from it; **medium** means the fact is primary but the recommendation is a judgement, or one part of the fact came from a secondary or summarised source.

### Summary

| # | Item | Baseline for Species | Confidence | Decided in |
|---|---|---|---|---|
| 1.1 | OS (client) | Windows 11 x64, **minimum 24H2, build 10.0.26100**; primary test targets 25H2 (26200) and 26H2 (26300) | high | [ADR-0050](0050-client-platform-and-minimum-os.md) |
| 1.2 | Windows 10 | Not supported (out of support since 2025-10-14; consumer ESU ended 2026-10-13) | high | [ADR-0050](0050-client-platform-and-minimum-os.md) |
| 2.1 | Visual Studio and MSVC | Visual Studio 2026 (18.11 current); **MSVC Build Tools 14.51** pinned, move to 14.52 when it becomes the default | high | [ADR-0051](0051-windows-toolchain-and-compiler-policy.md) |
| 2.2 | Language | **C++23**: `/std:c++23preview` on 14.51, `/std:c++23` from 14.52; no `/std:c++latest`; no C++26 features | high | [ADR-0052](0052-cpp23-language-standard.md) |
| 2.2 | `import std` | Supported by MSVC (since Visual Studio 2022 17.5); allowed behind a build option until CMake 4.5 makes it non-experimental | medium | [ADR-0053](0053-modules-and-import-std.md) |
| 2.3 | Dedicated-server compilers (Linux) | **Clang 23 (LLVM 23.1.3) + libc++** primary, **GCC 16 (16.2.0)** secondary; shared code uses the intersection with MSVC | medium | [ADR-0052](0052-cpp23-language-standard.md), [ADR-0051](0051-windows-toolchain-and-compiler-policy.md) |
| 3.1 | Build | **CMake 4.4 or later** (4.4.4 current), presets schema 10, **Ninja Multi-Config** | high | [ADR-0054](0054-build-system-cmake-presets.md) |
| 3.2 | Dependencies | **vcpkg manifest mode**, pinned `builtin-baseline`, overlay triplets per toolset | high | [ADR-0055](0055-dependencies-vcpkg-manifest-mode.md) |
| 4.1 | Agility SDK | **1.619.6 (D3D12SDKVersion 619)**, `D3D12SDKPath .\D3D12\` | high | [ADR-0001](0001-direct3d-12-renderer-on-the-agility-sdk.md) |
| 4.2 | Feature level | **12_1 minimum** + Shader Model 6.6 + Resource Binding Tier 3 + WaveOps; 12_2 not required (its hardware reach not verified) | medium | [ADR-0002](0002-minimum-gpu-capability.md) |
| 4.3 | Shader Model | **Minimum 6.6** (bindless dynamic resources); current release 6.9, 6.10 in preview | high | [ADR-0002](0002-minimum-gpu-capability.md), [ADR-0006](0006-root-signature-and-bindless-resource-access.md) |
| 4.4 | Enhanced Barriers | Frame graph in Enhanced-Barrier terms; `Barrier()` when `EnhancedBarriersSupported`, the `ResourceBarrier` API otherwise | high | [ADR-0007](0007-barrier-model.md) |
| 4.5 | Shader compiler | **DXC v1.9.2609**, offline, `-HV 2021`, 202x-clean | high | [ADR-0003](0003-shader-language-compiler-and-material-shaders.md) |
| 4.6 | Swap chain, VRR, HDR | `FLIP_DISCARD`, waitable object, `ALLOW_TEARING` when supported, no exclusive full screen; HDR optional through scRGB FP16 | high | [ADR-0012](0012-swap-chain-presentation-and-frame-pacing.md), [ADR-0014](0014-hdr-output.md) |
| 4.7 | Profiling | **PIX 2603.25**; **WinPixEventRuntime 1.0.240308001** (MIT), markers compiled out of shipping builds | high | [ADR-0068](0068-profiling-and-gpu-timing.md) |
| 5.1 | DirectStorage | **Not adopted in v1**; package format chunked (at most 256 KiB, 64 KiB-aligned) so 1.3.0 (stable) or 1.4 (preview now) can be added later | medium | [ADR-0058](0058-pack-files-and-mount-policy.md) |
| 5.2 | Texture tools | **DirectXTex `may2026`** in the cooker; texconv for inspection; CI cooks BC7 with the software codec | high | [ADR-0017](0017-texture-container-and-block-compression.md) |
| 5.3 | Block compression | BC7 sRGB colour, BC5 normals, BC4 single-channel; uncompressed for exact data and glyphs | high | [ADR-0017](0017-texture-container-and-block-compression.md) |
| 6 | Input | **GameInput** for gamepads (pin 3.5.283 / `GameInput::v3` now, 4.0.x after its first point release) + redistributable in the installer; Win32 messages for mouse, keyboard, text and IME | medium | [ADR-0028](0028-input-device-back-ends.md) |
| 7 | Audio API | **In-box XAudio 2.9 + X3DAudio**; no redistributable | high | [ADR-0022](0022-audio-engine-on-xaudio2.md) |
| 8.1 | Mesh source format | **glTF 2.0** (`.glb`), no `NORMAL` for flat shading, `COLOR_0`; extensions `KHR_materials_unlit`, `KHR_materials_emissive_strength`, `KHR_mesh_quantization` only | high | [ADR-0019](0019-mesh-source-format-and-cooked-meshes.md), [ADR-0020](0020-flat-shading-and-team-tint.md) |
| 8.2 | Texture container | **DDS with the DX10 header** for every cooked texture | high | [ADR-0017](0017-texture-container-and-block-compression.md) |
| 8.3 | Audio formats | Sources 48 kHz 24-bit WAV; runtime 16-bit PCM 48 kHz for effects, streamed **Opus 1.5.2** for music; MS-ADPCM only once its encoder's distribution is confirmed; no xWMA | medium | [ADR-0025](0025-audio-formats-codecs-and-residency.md) |
| 9 | Glyph rasterisation | DirectWrite layout → custom renderer → `IDWriteFactory3::CreateGlyphRunAnalysis` (grayscale) → `CreateAlphaTexture` → `R8_UNORM` atlas | medium | [ADR-0034](0034-text-rendering-and-glyph-cache.md) |
| 10.1 | Accessibility | **XAG 3.2** as checklist; text scale, 7:1 high-contrast mode, remapping, photosensitivity check | high | [ADR-0064](0064-accessibility-baseline-xag.md) |
| 10.2 | Localisation | UTF-8 everywhere, **BCP 47** tags, in-box ICU (`icu.dll`) on the client; data level Unicode 17 / CLDR 48 | medium | [ADR-0063](0063-localisation.md) |
| 11.1 | Crash reporting | **WER** + Windows Desktop Application Program + symbol store; the dedicated-server crash path is a design-phase decision | high | [ADR-0065](0065-crash-reporting.md) |
| 11.2 | Packaging | One relocatable install directory, MSIX-compatible; storefront choice is a design-phase decision | medium | [ADR-0071](0071-install-layout-runtime-files-and-user-data.md), [ADR-0072](0072-distribution-channels-and-package-formats.md) |
| 11.3 | Code signing | Sign every PE file and installer, SHA-256 + RFC 3161 timestamp, in CI; **Azure Artifact Signing** if eligible, else an OV certificate | high | [ADR-0073](0073-code-signing.md) |

### Recommendations and sources by area

All sources below were checked on 2026-10-17.

#### 1 Operating system

- **Minimum OS: Windows 11 24H2, OS build 10.0.26100, x64.** Every consumer edition still in service (25H2, 26H2, 26H1) and the LTSC 2024 editions have a build of at least 26100. 24H2, 25H2 and 26H2 share one servicing branch, so one test matrix covers them. Test on 25H2 (26200) and 26H2 (26300) as primary targets, 24H2 LTSC (26100) as the floor, and 26H1 (28000, a separate Windows core for new devices) as a smoke test.
- The application manifest declares Windows 10/11 in `<supportedOS>`; the launcher shows a clear message below build 26100. Hard failures come only from capability checks (for example the Direct3D 12 checks of ADR-0002).
- **Windows 10 is not a supported client platform** (end of support 2025-10-14; the consumer ESU programme ended 2026-10-13). A Windows dedicated server follows the same rule; the server OS is a hosting decision for the design phase.
- 25H2 Home and Pro leave service on 2027-10-12: re-check this item at every milestone.
- Sources: https://learn.microsoft.com/windows/release-health/windows11-release-information · https://learn.microsoft.com/windows/release-health/status-windows-11-24h2 · https://learn.microsoft.com/windows/whats-new/whats-new-windows-11-version-26h2 · https://learn.microsoft.com/windows/whats-new/windows-11-version-26h1#servicing-and-updates · https://learn.microsoft.com/windows/win32/sysinfo/operating-system-version · https://learn.microsoft.com/windows/win32/sysinfo/targeting-your-application-at-windows-8-1 · https://learn.microsoft.com/windows/release-health/release-information · https://learn.microsoft.com/lifecycle/faq/extended-security-updates · https://learn.microsoft.com/windows/whats-new/extended-security-updates · https://learn.microsoft.com/answers/a/12172995 · https://learn.microsoft.com/answers/a/12306470 (the consumer ESU page on support.microsoft.com, which these answers quote, was unreachable).

#### 2 Compilers and language

- **Visual Studio 2026 18.11.0** (2026-10-13) is the current stable release. MSVC Build Tools ship a minor version every six months: 14.50 (long-term, to Nov 2028), 14.51 (default toolset since Visual Studio 18.6, May 2026; end of life Feb 2027), 14.52 (preview; expected to become the default by November 2026).
- **Language baseline: C++23.** Build with `/std:c++23preview` on MSVC 14.51, and with `/std:c++23` once 14.52 is the default toolset. Treat everything built in C++23-preview mode as one ABI unit: rebuild all first-party code and every vcpkg dependency together when the toolset changes. Never use `/std:c++latest` in shipping builds. The MSVC standard library is C++23 feature complete from 14.52 Preview; extended floating-point types (P1467R9) are not implemented.
- Pin one toolset version in CI. Always-on flags: `/permissive-`, `/Zc:__cplusplus`, `/Zc:preprocessor`, `/utf-8`, `/W4`, `/EHsc`.
- **`import std`** is permitted behind a build option (CMake support is experimental until CMake 4.5, and Visual Studio generators cannot build it). Headers that the dedicated server also compiles stay header-includable.
- **Dedicated-server toolchain (Linux): Clang 23 with libc++ primary, GCC 16 secondary**, both `-std=c++23`. Shared client and server code uses only the intersection of MSVC 14.51, Clang 23/libc++ and GCC 16/libstdc++, and CI builds it with all three. It avoids `std::stacktrace`, `std::generator`, `std::move_only_function`, constexpr `<cmath>`, extended floating-point types and every C++26 feature. Linux CI images for `import std` must be Ubuntu 26.04 or newer.
- No C++26 feature is used: MSVC ships none under a stable switch at the run date.
- Sources: https://learn.microsoft.com/visualstudio/releases/2026/release-notes · https://learn.microsoft.com/cpp/overview/compiler-versions?view=msvc-170 · https://learn.microsoft.com/cpp/overview/what-s-new-for-msvc?view=msvc-170 · https://learn.microsoft.com/cpp/build/reference/std-specify-language-standard-version?view=msvc-170 · https://devblogs.microsoft.com/cppblog/c23-support-in-msvc-build-tools-14-51/ · https://learn.microsoft.com/cpp/build/reference/zc-cmath?view=msvc-170 · https://learn.microsoft.com/cpp/overview/visual-cpp-language-conformance?view=msvc-170 · https://learn.microsoft.com/cpp/overview/msvc-conformance-improvements?view=msvc-170 · https://github.com/microsoft/STL/wiki/Changelog · https://learn.microsoft.com/cpp/cpp/modules-cpp?view=msvc-170 · https://learn.microsoft.com/cpp/cpp/tutorial-import-stl-named-module?view=msvc-170 · https://github.com/llvm/llvm-project/releases · https://raw.githubusercontent.com/llvm/llvm-project/main/clang/www/cxx_status.html · https://libcxx.llvm.org/Status/Cxx23.html (read from its status file in the llvm-project repository) · https://github.com/gcc-mirror/gcc/tags · https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-16/libstdc++-v3/doc/xml/manual/status_cxx2023.xml · https://gcc.gnu.org/pipermail/gcc-announce/2026/000190.html and https://gcc.gnu.org/projects/cxx-status.html (both unreachable; summarised from search results, medium confidence).

#### 3 Build and dependencies

- **CMake 4.4 or later** (`cmake_minimum_required(VERSION 4.4)`; 4.4.4 is current, 4.5.0-rc1 a pre-release), presets schema **version 10** unless a later field is needed. **Ninja Multi-Config** for every CI and command-line build on Windows and Linux; the Visual Studio IDE opens the CMake folder directly.
- Preset set: `windows-msvc-{debug,release,profile}`, optional `windows-clangcl-release`, `linux-clang-{debug,release}`, `linux-gcc-release`, each with build and test presets and one workflow preset per CI job. Move `import std` to the stable path when CMake 4.5 is released.
- **vcpkg in manifest mode** (`vcpkg.json` and `vcpkg-configuration.json` at the repository root, a pinned `builtin-baseline`, vcpkg pinned in CI; never classic mode). Overlay triplets `x64-windows-species` (static libraries, dynamic CRT `/MD`) and `x64-linux-species` record the toolset version, so every port is rebuilt on a toolset change. Binary caching in CI keyed by vcpkg's ABI hash; the cache host is an infrastructure decision. Latest registry release tag: 2026.07.29.
- Sources: https://github.com/Kitware/CMake/releases · https://raw.githubusercontent.com/Kitware/CMake/v4.4.4/Help/manual/cmake-presets.7.rst · https://raw.githubusercontent.com/Kitware/CMake/v4.4.4/Help/manual/cmake-cxxmodules.7.rst · https://raw.githubusercontent.com/Kitware/CMake/master/Help/manual/cmake-cxxmodules.7.rst (cmake.org was unreachable; the same documents were read from the Kitware repository) · https://learn.microsoft.com/vcpkg/concepts/manifest-mode · https://learn.microsoft.com/vcpkg/users/versioning · https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration · https://learn.microsoft.com/vcpkg/consume/lock-package-versions · https://github.com/microsoft/vcpkg/tags.

#### 4 Direct3D 12

- **Agility SDK 1.619.6** (NuGet `Microsoft.Direct3D.D3D12`, D3D12SDKVersion 619, released 2026-09-14; 1.619 brought Shader Model 6.9). Export `D3D12SDKVersion` and `D3D12SDKPath = ".\\D3D12\\"` from `species.exe`; never ship a preview SDK (latest preview: 1.721.3-preview) or `D3D12SDKLayers.dll`. A build check asserts that the exported number equals the packaged `D3D12Core.dll`. Every OS allowed by item 1.1 supports the Agility SDK. DirectX-Headers latest release: v1.619.5. The dedicated server does not link Direct3D.
- **Feature level 12_1 minimum**, plus three capability checks: **Shader Model 6.6**, **Resource Binding Tier 3** and `D3D12_FEATURE_DATA_D3D12_OPTIONS1.WaveOps`; fail with a clear message if any is missing. 12_2 is not required: Species' look needs none of raytracing, mesh shaders, sampler feedback or variable-rate shading. Mesh shaders and VRS may be optional fast paths behind capability checks, never the only path. The crowd renderer is designed around compute culling plus `ExecuteIndirect`, which works at 12_1.
- **Shader Model 6.6 minimum**; compile every shader to `*_6_6` profiles. Bindless dynamic-resource access (`ResourceDescriptorHeap[]`, `SamplerDescriptorHeap[]`, root-signature flags `CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED` and `SAMPLER_HEAP_DIRECTLY_INDEXED`) is the only binding model. A Shader Model 6.7+ feature is used only behind a capability check with a 6.6 fallback, and only with an ADR. Query the highest supported model by looping downwards.
- **Enhanced Barriers** have been in the Agility SDK since 1.608.0 but are not a hardware or driver requirement: check `D3D12_FEATURE_DATA_D3D12_OPTIONS12.EnhancedBarriersSupported`. The frame graph expresses synchronisation in Enhanced-Barrier terms (sync, access, layout); the recording layer emits `Barrier()` when supported and translates to the `ResourceBarrier` API otherwise. Fence barriers (preview only) are not used.
- **DXC v1.9.2609** (2026-09-29), offline, invoked by the asset pipeline with `-HV 2021` explicit, `-enable-16bit-types` only where the capability is checked, warnings as errors, PDBs kept for PIX. Ship DXIL only. Shaders also compile under `-HV 202x` (no effects syntax, no `interface`, no `uniform`).
- **Swap chain:** `DXGI_SWAP_EFFECT_FLIP_DISCARD`, 3 back buffers, `FRAME_LATENCY_WAITABLE_OBJECT` with a maximum frame latency of 1 or 2, `ALLOW_TEARING` when `CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING)` reports it. Windowed and borderless full screen only: Direct3D 12 has no fullscreen exclusive mode. Flip-model swap chains cannot be multisampled. VRR uses sync interval 0 with `DXGI_PRESENT_ALLOW_TEARING`; vsync uses sync interval 1; both are user options.
- **HDR: optional, off by default.** The SDR path renders to an 8-bit sRGB back buffer. The HDR path uses `R16G16B16A16_FLOAT` with scRGB (`DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709`), because the additive glow on black depends on blending and on values above 1.0; tone-map to the display from `IDXGIOutput6::GetDesc1`, and map UI to the SDR reference white level. HDR10 is not used.
- **PIX 2603.25 or later**; **WinPixEventRuntime 1.0.240308001** (MIT) as a DLL in development and profile builds, compiled out of shipping builds. Keep the debug layer and GPU-based validation in a CI test configuration.
- Sources: https://devblogs.microsoft.com/directx/directx12agility/ · https://devblogs.microsoft.com/directx/shader-model-6-9-retail-and-more/ · https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/ · https://github.com/microsoft/DirectX-Headers/releases · https://learn.microsoft.com/windows/win32/direct3d12/hardware-feature-levels · https://learn.microsoft.com/windows/win32/direct3d11/overviews-direct3d-11-devices-downlevel-intro · https://raw.githubusercontent.com/microsoft/DirectX-Specs/master/d3d/D3D12_FeatureLevel12_2.md · https://devblogs.microsoft.com/directx/announcing-directx-12-ultimate/ · https://raw.githubusercontent.com/microsoft/DirectX-Specs/master/d3d/HLSL_SM_6_6_DynamicResources.md · https://learn.microsoft.com/windows/win32/api/d3d12/ne-d3d12-d3d_shader_model · https://learn.microsoft.com/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_shader_model · https://learn.microsoft.com/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_d3d12_options12 · https://learn.microsoft.com/windows-hardware/drivers/display/enhanced-barriers · https://learn.microsoft.com/windows-hardware/drivers/ddi/d3d12umddi/nc-d3d12umddi-pfnd3d12ddi_barrier_0094 · https://github.com/microsoft/DirectXShaderCompiler/releases · `ReleaseNotes.md` of https://github.com/microsoft/DirectXShaderCompiler · https://learn.microsoft.com/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model · https://learn.microsoft.com/windows/win32/direct3d12/swap-chains · https://learn.microsoft.com/windows/win32/direct3ddxgi/variable-refresh-rate-displays · https://learn.microsoft.com/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_chain_flag · https://learn.microsoft.com/windows/win32/direct3darticles/high-dynamic-range · https://learn.microsoft.com/windows/win32/api/dxgi1_5/nf-dxgi1_5-idxgiswapchain4-sethdrmetadata · https://devblogs.microsoft.com/pix/download/ · https://www.nuget.org/packages/WinPixEventRuntime · https://github.com/microsoft/PixEvents.

#### 5 Content I/O and textures

- **DirectStorage is not adopted in v1** (latest stable 1.3.0, 2025-06-27; 1.4.0-preview2, 2026-06-16, adds Zstandard). Species' flat-shaded, vertex-coloured assets are few and small, so load time is dominated by parsing and GPU upload, and the dedicated server cannot use DirectStorage anyway. The runtime package format stores assets in independently compressed chunks of at most 256 KiB, aligned to 64 KiB, with a table of contents, so a DirectStorage back-end can be added behind the same I/O interface if terrain streaming at the strategic zoom level proves I/O-bound. No preview DirectStorage or GACL in shipping builds.
- **DirectXTex `may2026`** (2026-05-08; NuGet 2026.5.8, vcpkg, winget) is called from the Species asset cooker; texconv is for manual inspection only. Cooking options are data stored next to each source texture. Always write DDS with the DX10 header, always generate full mip chains, always state the colour space. CI cooks BC7 with the software codec so that output does not depend on the build machine's GPU.
- **Formats:** colour and UI art `BC7_UNORM_SRGB`; normal maps (if any) `BC5_UNORM`; single-channel data (glow masks, darkening masks, height, ambient occlusion) `BC4_UNORM`; data that needs exact values stays uncompressed (`R8_UNORM`, `R8G8_UNORM`). No BC1 to BC3 in new content; BC6H only if HDR sky or light textures are added. Glyph atlases are never block-compressed. BC4, BC5 and BC7 are supported at every feature level from 11_0 up.
- Sources: https://www.nuget.org/packages/Microsoft.Direct3D.DirectStorage · https://devblogs.microsoft.com/directx/directstorage-1-4-release-adds-support-for-zstandard/ · https://learn.microsoft.com/windows/win32/dstorage/dstorage-portal · https://learn.microsoft.com/windows/win32/dstorage/dstorage/ne-dstorage-dstorage_compression_support · https://github.com/microsoft/DirectStorage/releases · https://github.com/microsoft/DirectXTex/releases · https://github.com/microsoft/DirectXTex/wiki/Texconv · https://learn.microsoft.com/windows/win32/direct3d11/texture-block-compression-in-direct3d-11 · https://learn.microsoft.com/windows/win32/direct3d11/bc7-format · https://learn.microsoft.com/windows/win32/direct3d11/overviews-direct3d-11-devices-downlevel-intro.

#### 6 Input

- **Gamepads and other game devices: GameInput**, built against a pinned `Microsoft.GameInput` package and a pinned API namespace. Latest package 4.0.300 (2026-10-15, API version 4); previous line 3.5.283 (2026-09-28). Recommended pin: **3.5.283 with `GameInput::v3`**, moving to 4.0.x after its first point release. Each API version has its own namespace and interface IDs; binary and functional compatibility is kept for apps built against earlier versions.
- PC games must include the GameInput redistributable (`GameInputRedist.msi`, under `redist` in the package) in their installers; it refuses to downgrade a newer runtime.
- **Mouse, keyboard and text: Win32 window messages** (`WM_INPUT` for raw mouse deltas in camera drag, key messages for bindings, `WM_CHAR` plus IME messages for text entry). One input abstraction maps every device to Species actions, so rebinding is done once. The headless server never links GameInput.
- Sources: https://www.nuget.org/packages/Microsoft.GameInput · https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-nuget · https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-versioning · https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-faq.

#### 7 Audio

- **Use the in-box XAudio 2.9 and X3DAudio** (`xaudio2_9.dll`, present on every Windows 10 and 11 device; link `xaudio2.lib` from the Windows SDK). Do not ship the redistributable (`Microsoft.XAudio2.Redist` 1.2.13, for older systems; `CreateHrtfApo` is not implemented there).
- Create the mastering voice with the default device and default format, so device switches work; handle `OnCriticalError` and `XAUDIO2_E_DEVICE_INVALIDATED` by rebuilding the graph, and test device changes while paused. Since Windows 10 1903, XAudio 2.9 output follows the user's system spatial-sound format. The graph processes 32-bit float with a 10 ms quantum.
- Positional sounds use X3DAudio; the HRTF APO is not used. The headless server has no audio.
- Sources: https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-versions · https://learn.microsoft.com/uwp/win32-and-com/win32-apis#apis-from-xaudio2_9dll · https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-redistributable · https://www.nuget.org/packages/Microsoft.XAudio2.Redist · https://learn.microsoft.com/windows/win32/api/xaudio2/nf-xaudio2-ixaudio2-createsourcevoice · https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-audio-graph · https://learn.microsoft.com/windows/win32/xaudio2/adpcm-overview.

#### 8 Asset formats

- **Meshes and animation: glTF 2.0** (`.glb` for checked-in sources; right-handed, +Y up, front faces +Z, metres), cooked into Species' own runtime format; the runtime never parses glTF. A primitive without `NORMAL` has implicit flat normals, which gives the faceted look; `COLOR_0` (unorm8 RGBA) carries per-vertex colour. Allowed extensions: `KHR_materials_unlit`, `KHR_materials_emissive_strength`, `KHR_mesh_quantization`; everything else is rejected unless an ADR adds it. The cooker validates every source against the specification (for example with the Khronos glTF validator).
- **Every cooked texture is a DDS with the DX10 header** (`dxgiFormat`, sRGB-ness and array size explicit); the loader rejects any DDS without it.
- **Audio sources:** uncompressed WAV, **48 kHz**, 24-bit integer (or 32-bit float) PCM; mono for every positional sound, stereo for music and UI. **Runtime:** short, frequently triggered effects as 16-bit PCM at 48 kHz, fully resident; larger effect banks as MS-ADPCM only once the encoder's distribution is confirmed; music and long ambiences as streamed **Opus** (libopus 1.5.2 through vcpkg) decoded on a worker thread into PCM buffers. xWMA is not used. The mastering format is the device default; XAudio2 converts sample rates.
- Sources: https://raw.githubusercontent.com/KhronosGroup/glTF/main/specification/2.0/Specification.adoc · https://github.com/KhronosGroup/glTF/tree/main/extensions · https://learn.microsoft.com/windows/win32/direct3ddds/dx-graphics-dds-pguide · https://learn.microsoft.com/windows/win32/direct3ddds/dds-header-dxt10 · https://learn.microsoft.com/windows/win32/direct3ddds/dds-file-layout-for-textures · https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-redistributable#xaudio-29-api-differences-compared-to-xaudio-27 · https://github.com/xiph/opus/releases (registry.khronos.org was not reached; the Khronos repository was read instead).

#### 9 Text

- **Path:** `IDWriteTextLayout` for shaping, bidirectional text, line breaking and font fallback → a custom `IDWriteTextRenderer` that receives glyph runs → for each glyph not yet cached, `IDWriteFactory3::CreateGlyphRunAnalysis` with a natural-symmetric raster mode and `DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE` → `CreateAlphaTexture` into a staging buffer → one coverage channel copied into an `R8_UNORM` atlas page.
- **Cache key:** font face, glyph index, pixel size (after DPI and the text-scale setting), subpixel x-offset quantised to a quarter pixel; LRU eviction by atlas page. No ClearType in the game UI. A distance-field atlas built from `GetGlyphRunOutline` is an option for world-space labels at the strategic zoom, not the baseline. Fonts ship with the game (licensed for embedding) in a custom font collection; system fonts are only a fallback for user-entered text. The headless server never touches DirectWrite.
- Sources: https://learn.microsoft.com/windows/win32/api/dwrite/nf-dwrite-idwritefactory-createglyphrunanalysis · https://learn.microsoft.com/windows/win32/api/dwrite_2/nf-dwrite_2-idwritefactory2-createglyphrunanalysis · https://learn.microsoft.com/windows/win32/api/dwrite_3/nf-dwrite_3-idwritefactory3-createglyphrunanalysis · https://learn.microsoft.com/windows/win32/api/dwrite/nn-dwrite-idwriteglyphrunanalysis · https://learn.microsoft.com/windows/win32/api/dwrite/nf-dwrite-idwriteglyphrunanalysis-createalphatexture · https://learn.microsoft.com/windows/win32/api/dwrite_1/ne-dwrite_1-dwrite_text_antialias_mode · https://learn.microsoft.com/windows/win32/directwrite/glyphs-and-glyph-runs · https://learn.microsoft.com/windows/win32/api/dwrite_2/nn-dwrite_2-idwritefactory2.

#### 10 Accessibility and localisation

- **XAG 3.2** (published 2023-06-08; best practices, not a compliance checklist) is the accessibility checklist for the PC client, with conformance recorded per guideline in the design phase's acceptance tests. Engine consequences fixed now: a UI text-scale setting that feeds the glyph cache; contrast-checked UI palettes including a 7:1 high-contrast mode; full input remapping and hold/toggle options; colour never the only channel for team identity; a photosensitivity check on glow and flash effects (a luminance flash is a 10 % change); a motion setting for camera shake; an accessible-name field on every widget so screen narration can be added. Measurable criteria include minimum default text of 9 px at 1080p and 19 px at 4K, and gameplay text contrast of at least 4.5:1.
- **Text is UTF-8** in files, data and the network; UTF-16 only at Win32 and DirectWrite boundaries. **Language identifiers are BCP 47 tags** (`en-GB`, `de-DE`, `pt-BR`, `zh-Hans-CN`); the client picks its UI language from `GetUserPreferredUILanguages` and falls back along the tag to `en-GB`. Formatting, plurals and collation use the in-box ICU C API (`icu.dll`, present from Windows 1903); string tables use ICU MessageFormat syntax. Documented data level: Unicode 17 / CLDR 48 (ICU 78.3).
- Sources: https://learn.microsoft.com/xbox/accessibility/guidelines · https://learn.microsoft.com/xbox/accessibility/xag-version-history · https://learn.microsoft.com/gaming/game-publishing/concepts/metadata-accessibility · https://learn.microsoft.com/xbox/accessibility/accessibility-feature-tags · https://github.com/unicode-org/icu/releases · https://github.com/unicode-org/cldr/releases · https://learn.microsoft.com/windows/win32/intl/locale-names · https://learn.microsoft.com/globalization/locale/standard-locale-names · https://learn.microsoft.com/windows/win32/intl/locale-name-constants · https://learn.microsoft.com/windows/win32/intl/international-components-for-unicode--icu- (unicode.org and rfc-editor.org were unreachable).

#### 11 Crash reporting, packaging and code signing

- **Let WER handle crashes** and receive the data through the free **Windows Desktop Application Program** (requires code signing; data appears from 100 installed devices). Do not install a top-level exception filter that swallows crashes. Register the current log and settings files (`WerRegisterFile`), a small black-box ring buffer (`WerRegisterMemoryBlock`), and exclude personal data (`WerRegisterExcludedMemoryBlock`). Every build publishes PDBs to a private symbol store; release builds upload symbols to Partner Center. CI provides a `LocalDumps` registry file for test machines. The device-removed path writes DRED data into the same report. The dedicated-server crash path is a design-phase decision (Sentry Native 0.17.4 with Crashpad is one option).
- **One relocatable install directory** (`species.exe`, `D3D12\`, data packages, the GameInput redistributable as a prerequisite) that writes user data under `%LOCALAPPDATA%\Species` and never beside the executable; every distribution format wraps that directory. The engine stays MSIX-compatible; the storefront choice is a design-phase decision. Whether the Agility SDK passes MSIX certification is not verified.
- **Sign every PE file and the installer** with SHA-256 and an RFC 3161 timestamp, in CI, on release builds only. Preferred: Azure Artifact Signing if the publishing entity is eligible, otherwise an OV certificate in a hardware-backed or cloud key store; EV brings no SmartScreen advantage. Signing keys never live on developer machines or in the repository.
- Sources: https://learn.microsoft.com/windows/win32/wer/using-wer · https://learn.microsoft.com/windows/win32/api/_wer/ · https://learn.microsoft.com/windows/win32/wer/collecting-user-mode-dumps · https://learn.microsoft.com/windows/win32/wer/wer-settings · https://learn.microsoft.com/windows/win32/appxpkg/windows-desktop-application-program · https://learn.microsoft.com/windows/uwp/monetize/get-desktop-application-error-reporting-data · https://learn.microsoft.com/windows/apps/publish/analyze-app-performance/health-report · https://learn.microsoft.com/windows/win32/dxtecharts/crash-dump-analysis · https://github.com/getsentry/sentry-native/releases · https://learn.microsoft.com/windows/apps/publish/get-started#frequently-asked-questions · https://learn.microsoft.com/windows/apps/package-and-deploy/choose-distribution-path · https://learn.microsoft.com/windows/apps/publish/publish-your-app/msix/app-package-requirements · https://learn.microsoft.com/windows/win32/appxpkg/how-to-sign-a-package-using-signtool · https://learn.microsoft.com/windows/apps/package-and-deploy/code-signing-options · https://learn.microsoft.com/windows/apps/package-and-deploy/publish-first-app#step-2-set-up-code-signing · https://learn.microsoft.com/windows/msix/package/signing-package-overview · https://learn.microsoft.com/windows/msix/package/sign-msix-package-guide.

### Items not verified

These baseline facts could not be confirmed from a primary source on 2026-10-17. Each dependent ADR states its default without relying on them; re-check them when the sources can be reached.

- GCC's per-feature C++23 core-language status and the full GCC 16.1 announcement (gcc.gnu.org unreachable).
- Compiler-support tables on cppreference (unreachable; vendor status pages used instead).
- ISO publication status of C++26.
- Hardware reach (market share) of feature level 12_2.
- The Windows 10 consumer ESU support page (end date taken from Microsoft Q&A answers quoting it).
- The Unicode version from unicode.org (taken from the ICU 78 release notes) and the BCP 47 RFC page.
- The current distribution of the xWMA and AdpcmEncode encoder tools. A further check of the XAudio2 ADPCM overview on 2026-10-17 (https://learn.microsoft.com/windows/win32/xaudio2/adpcm-overview) names the AdpcmEncode tool but not where it is distributed.
- Whether the Agility SDK passes MSIX and WACK certification (only a 2021 statement was found).
- The licence terms of the GameInput redistributable (linked from the package page, not read), and whether the 4.0 package adds a `GameInput::v4` namespace. A Microsoft Learn search on 2026-10-17 (https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-versioning) did not settle the namespace question.
- cmake.org and registry.khronos.org (the same documents were read from the Kitware and Khronos repositories).

## Deferred to the design phase

The Species design phase decides these topics; this ADR set records no decision on them. Engine components list them as design-phase inputs where they depend on them.

| Topic | Problem the design phase must solve |
|---|---|
| Networking model | How an authoritative server and its clients exchange commands and state for a region-partitioned world and for skirmish: transport, replication, interest management, and the bandwidth for hundreds of crowd units in view. |
| Simulation numeric model | Which number representation and floating-point rules the simulation uses, so that every process that steps a region computes identical results, and how positions keep their precision far from the origin. |
| Simulation threading and process architecture | How the simulation is stepped (phases, job system, ownership and reset of per-match and per-region state) and kept separate from presentation, both in-process and in a dedicated server. |
| Tick rate | The simulation step period and per-tick budget for the world game and for skirmish, starting from the reference tick rate in `reference/12_simulation_model/`. |
| Persistence | How world state, settings, profiles, statistics and replays are stored, versioned and migrated across patches, with backup and rollback. |
| Sessions | How players join, leave and are placed into worlds and skirmish matches: lobby, discovery, slot assignment and host authority. |
| Online services | Identity, entitlement, the authenticated settlement-status and orders interface, and the services a later companion app would use. |
| Hosting | How community operators, and later official hosting, run dedicated servers: operating system, configuration, moderation, server crash reporting and artefact signing. |
| Terrain runtime model | Whether terrain heights are generated at load or baked into map assets, how terrain streams for a large world, and how terrain content identity is checked. |
