# Species — technology standards baseline

> Brief: F3 (Phase 1). Run date and date checked for every item: **2026-10-17** (UTC), unless an item says otherwise.
> Scope: Windows 11 client, C++23, Direct3D 12; a headless dedicated server that may also be built for Linux.
> Out of scope (design phase): networking, online services, hosting.
> Rule: every version and support status below carries its source URL. Where a source could not be reached, the item says so; nothing is stated from memory.

## Contents

1. Operating system: Windows 11 baseline, minimum build, Windows 10 status
2. Compilers and language: MSVC and Visual Studio, C++23, `import std`, C++26; Clang and GCC for a Linux server
3. Build and dependencies: CMake presets, vcpkg manifest mode
4. Direct3D 12: Agility SDK, feature level, Shader Model, Enhanced Barriers, DXC and HLSL 2021, flip-model swap chain, VRR and HDR, PIX
5. Content I/O: DirectStorage; DirectXTex and texconv; BC7, BC5, BC4
6. Input: GameInput
7. Audio: XAudio2 2.9 and X3DAudio
8. Asset formats: glTF 2.0 and KHR extensions; DDS with the DX10 header; audio source and runtime formats
9. Text: DirectWrite glyph rasterisation for a glyph-cache UI
10. Accessibility and localisation: XAG; Unicode, CLDR, BCP 47
11. Shipping: Windows Error Reporting and crash reporting; MSIX and store packaging; code signing
12. Summary table

## 1 Operating system

### 1.1 Windows 11 servicing baseline

**Current fact (checked 2026-10-17).**
- Windows 11 has an annual feature update in the second half of the calendar year, with 24 months of support for Home and Pro and 36 months for Enterprise and Education.
- Current General Availability Channel versions:

| Version | OS build | Available | End of updates, Home/Pro | End of updates, Enterprise/Education |
|---|---|---|---|---|
| 26H2 | 26300 | 2026-09-29 | 2028-10-10 | 2029-10-09 |
| 26H1 | 28000 | 2026-02-10 | 2028-03-14 | 2029-03-13 |
| 25H2 | 26200 | 2025-09-30 | 2027-10-12 | 2028-10-10 |
| 24H2 | 26100 | 2024-10-01 | ended (2026-10-13) | 2027-10-12 |
| 23H2 | 22631 | 2023-10-31 | ended | 2026-11-10 |

- Windows 11 Enterprise LTSC 2024 is build 26100 and is serviced until 2029-10-09 (IoT Enterprise LTSC 2024 until 2034-10-10).
- 24H2, 25H2 and 26H2 share one servicing branch; 25H2 and 26H2 are delivered to 24H2 devices as small enablement packages.
- 26H1 (build 28000) is scoped to new devices that started shipping in early 2026, is built on a different Windows core from 24H2/25H2, and is not offered as an in-place update to existing devices.

**Sources.**
- https://learn.microsoft.com/windows/release-health/windows11-release-information (checked 2026-10-17)
- https://learn.microsoft.com/windows/release-health/status-windows-11-24h2 ("Current status as of October 13, 2026": 24H2 Home and Pro reached end of updates) (checked 2026-10-17)
- https://learn.microsoft.com/windows/whats-new/whats-new-windows-11-version-26h2 (common servicing branch, enablement package) (checked 2026-10-17)
- https://learn.microsoft.com/windows/whats-new/windows-11-version-26h1#servicing-and-updates (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Minimum OS: Windows 11, version 24H2, OS build 10.0.26100**, x64. Every consumer edition still in service (25H2, 26H2, 26H1) has a build number of at least 26100, and so do the commercial and LTSC 2024 editions that remain in service. Because 24H2, 25H2 and 26H2 share one servicing branch, one test matrix covers them.
- **Test on:** 25H2 (26200) and 26H2 (26300) as primary targets; 24H2 LTSC (26100) as the floor; 26H1 (28000) as a separate core, smoke-tested when hardware is available.
- The executable's application manifest declares Windows 10/11 in `<supportedOS>`; without it, `GetVersion(Ex)` and the version helpers report Windows 8 (6.2). The launcher reads the OS build number and shows a clear message below 26100. Microsoft advises testing for the feature itself rather than the version, so hard failures come only from capability checks (for example the D3D12 checks of §4.2). Source: https://learn.microsoft.com/windows/win32/sysinfo/operating-system-version and https://learn.microsoft.com/windows/win32/sysinfo/targeting-your-application-at-windows-8-1 (checked 2026-10-17).
- Re-check this table at every milestone: 25H2 Home/Pro leaves service on 2027-10-12.

### 1.2 Windows 10 support status

**Current fact (checked 2026-10-17).**
- Windows 10 reached end of support on **2025-10-14**. The October 2025 security update was the final one for Windows 10, version 22H2 (Home, Pro, Enterprise, Education).
- Extended Security Updates (ESU) are paid, security-only, and require version 22H2. Commercial ESU coverage: Year 1 to 2026-10-13, Year 2 to 2027-10-12, Year 3 to 2028-10-10. The consumer ESU programme ended on 2026-10-13 (stated by Microsoft Q&A answers that quote the consumer ESU support page; that page itself, support.microsoft.com, could not be fetched from this environment).
- Windows 10 Enterprise LTSC 2021 is serviced to 2027-01-12; IoT Enterprise LTSC 2021 to 2032-01-13.

**Sources.**
- https://learn.microsoft.com/windows/release-health/release-information (checked 2026-10-17)
- https://learn.microsoft.com/lifecycle/faq/extended-security-updates (ESU end dates table) (checked 2026-10-17)
- https://learn.microsoft.com/windows/whats-new/extended-security-updates (22H2 prerequisite, security-only scope) (checked 2026-10-17)
- https://learn.microsoft.com/answers/a/12172995 and https://learn.microsoft.com/answers/a/12306470 (consumer ESU end 2026-10-13, quoting https://support.microsoft.com/en-us/windows/windows-10-consumer-extended-security-updates-esu-program-33e17de9-36b3-43bb-874d-6c53d2e4bf42, which was unreachable) (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Windows 10 is not a supported client platform.** Consumer Windows 10 no longer receives security updates, and Species ships after the run date. Supporting it would add a second test matrix for an OS that consumers cannot keep patched.
- The client does not block Windows 10 deliberately beyond the build check of §1.1; the installer and the launcher state the requirement.
- The **dedicated server** on Windows follows the same rule (Windows 11 24H2 or Windows Server; the server-OS choice belongs to the design phase, because hosting is out of scope here).

## 2 Compilers and language

### 2.1 Visual Studio and MSVC

**Current fact (checked 2026-10-17).**
- Current Visual Studio stable release: **Visual Studio 2026 version 18.11.0, released 2026-10-13** (the same day 18.10.4 shipped as a security update).
- From Visual Studio 2026 on, MSVC versioning is decoupled from Visual Studio versioning. A new MSVC Build Tools minor version ships every six months. Current toolsets:

| MSVC Build Tools | `_MSC_VER` | Support class | End of life |
|---|---|---|---|
| 14.50 | 1950 | Long-term | Nov 2028 |
| 14.51 | 1951 | Standard (default toolset since VS 18.6, May 2026; 9 months of servicing) | Feb 2027 |
| 14.52 | 1952 | Preview | set at general availability |

- Microsoft expects 14.52 to become the default toolset and 14.53 the preview by November 2026. All toolsets keep binary compatibility with code built by Visual Studio 2015 or later.
- Build-only installs use the Visual Studio Build Tools SKU. Visual Studio 18.8 added `EnableVCToolsVersionDiscovery`, so a pinned `VCToolsVersion` is honoured across installs.

**Sources.**
- https://learn.microsoft.com/visualstudio/releases/2026/release-notes (18.11.0 and 18.10.4 entries) (checked 2026-10-17)
- https://learn.microsoft.com/cpp/overview/compiler-versions?view=msvc-170 (toolset table, cadence, lifecycle) (checked 2026-10-17)
- https://learn.microsoft.com/cpp/overview/what-s-new-for-msvc?view=msvc-170 (14.51 shipped with VS 18.6, May 2026) (checked 2026-10-17)

### 2.2 MSVC C++23 conformance, `import std`, C++26

**Current fact (checked 2026-10-17).**
- **Language switch.** MSVC 14.51 offers `/std:c++20`, `/std:c++23preview` and `/std:c++latest`. `/std:c++23preview` "may change and may not be ABI compatible across releases"; it is to be replaced by `/std:c++23` once C++23 is fully implemented and ABI-stable.
- **Plan for `/std:c++23`.** Microsoft's C++ team blog (2026-04-09) states that two C++23 features remained after 14.51 (P2564R3 consteval propagation and P0533R9 constexpr `<cmath>`), that `/std:c++23` "will be added in upcoming MSVC Build Tools 14.52 Preview releases", and that "full support for `/std:c++23` will begin once MSVC Build Tools 14.52 becomes the default (non-Preview) version". The `/Zc:cmath` reference page (available from 14.52) already documents use "along with `/std:c++23`". The `/std` reference page still lists only `/std:c++23preview` for released toolsets.
- **C++23 compiler features added in 14.51:** P2647R1, P2448R2, P2029R4, P2071R2, P2314R4, P1774R8 (portable assumptions, `[[assume]]`), P2324R2, P2582R1, P2615R1. The Learn conformance table still shows several of these as "no"; the blog states the table update is scheduled before 14.51 reaches full support, so the table lags.
- **C++23 core feature not implemented:** P1467R9 extended floating-point types (optional in C++23; marked "No").
- **Standard library.** The MSVC STL changelog states, for 14.52 Preview: "The STL is **C++23 feature complete**." 14.51 (current default) added `<flat_map>`, `<flat_set>`, P2255R2, P2590R2 and P2674R1. `std::zip` (P2321R2) is listed as partial in the Learn table.
- **`import std`.** "Starting with Visual Studio 2022 version 17.5, importing the Standard Library as a module is both standardized and fully implemented in the Microsoft C++ compiler." `std` and `std.compat` are built from `%VCToolsInstallDir%\modules\std.ixx` with the project's own compiler settings. Named modules do not export macros (`assert`, `errno`, `offsetof`); include the C header for those. Do not mix `#include <vector>` and `import std;` in one translation unit. (The Learn conformance table row for P2465R3 reads "no", which contradicts the modules overview and tutorial pages; the tutorial and overview are taken as authoritative.)
- **C++26.** MSVC's conformance page has no C++26 compiler-feature section. The C++ team blog states that the STL has not started accepting C++26 library features in general; a limited number of C++26 papers are implemented unconditionally as de facto defect reports (14.51 lists P3016R6, P3349R1, P3503R3, P3612R1). The ISO publication status of C++26 could not be verified from a primary source in this environment (isocpp.org and iso.org were unreachable); secondary sources report that WG21 completed technical work in March 2026.

**Sources.**
- https://learn.microsoft.com/cpp/build/reference/std-specify-language-standard-version?view=msvc-170 (checked 2026-10-17)
- https://devblogs.microsoft.com/cppblog/c23-support-in-msvc-build-tools-14-51/ (2026-04-09) (checked 2026-10-17)
- https://learn.microsoft.com/cpp/build/reference/zc-cmath?view=msvc-170 (checked 2026-10-17)
- https://learn.microsoft.com/cpp/overview/visual-cpp-language-conformance?view=msvc-170 (checked 2026-10-17)
- https://learn.microsoft.com/cpp/overview/msvc-conformance-improvements?view=msvc-170 (checked 2026-10-17)
- https://github.com/microsoft/STL/wiki/Changelog (raw: https://raw.githubusercontent.com/wiki/microsoft/STL/Changelog.md) (checked 2026-10-17)
- https://learn.microsoft.com/cpp/cpp/modules-cpp?view=msvc-170 and https://learn.microsoft.com/cpp/cpp/tutorial-import-stl-named-module?view=msvc-170 (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Language baseline: C++23.** Build with `/std:c++23` as soon as the toolset that provides it is the default (expected: MSVC 14.52). Until then, build with `/std:c++23preview` on **MSVC 14.51**, and treat everything built with it as one ABI unit: rebuild all first-party code and every vcpkg dependency together whenever the toolset changes, and never mix prebuilt binaries across toolsets for C++23-mode code.
- **Toolset policy:** pin one toolset version in CI (`VCToolsVersion` or the CMake toolset field), and move only on a deliberate upgrade. Move to 14.52 when it reaches the default toolset; consider 14.50 (long-term, to Nov 2028) only if a C++23 preview ABI break bites, because 14.50 lacks the 14.51 C++23 additions.
- **Always-on flags:** `/permissive-` (implied by `/std:c++20` and later), `/Zc:__cplusplus`, `/Zc:preprocessor`, `/utf-8`, `/W4`, `/EHsc`.
- **Do not use `/std:c++latest`** in shipping builds: it enables in-progress and experimental features.
- **`import std`:** permitted in Species code, but behind a build option, because CMake support is experimental until CMake 4.5 (§3.1) and Visual Studio generators cannot build it. Headers that must also compile on the Linux server stay header-includable.
- **Avoid in shared (client and server) code:** `std::float16_t` and the other extended floating-point types (no MSVC support), `std::generator` and `std::stacktrace` (missing in libc++, §2.3), and any C++26 feature. C++26 features are used only where MSVC ships them under a stable switch; as of the run date there are none.

### 2.3 Clang and GCC for a Linux dedicated server

**Current fact (checked 2026-10-17).**
- **Clang/LLVM.** Latest release: **LLVM 23.1.3** (tag `llvmorg-23.1.3`, marked Latest, 2026-10-06); the status page marks Clang 24 as unreleased. C++23 core rows not complete in Clang: P1949R7 (partial, Clang 14), P0533R9, P1467R9, P2582R1, P2590R2 (all "No"). C++26 rows: pack indexing (Clang 19), `= delete("reason")` (Clang 19), placeholder variables (Clang 18), user-generated `static_assert` messages (Clang 17); `#embed`, contracts, reflection and erroneous behaviour show "No".
- **libc++ C++23 library.** 79 of 114 C++23 papers are Complete. Complete: `<print>` (P2093, libc++ 18), `std`/`std.compat` modules (P2465, libc++ 19), `<mdspan>` (18), `<flat_map>` (20), `<expected>` (16). Not implemented: `<stacktrace>` (P0881), `std::generator` (P2502), `move_only_function` (P0288), `views::chunk`/`slide`, `cartesian_product`, extended floating-point, P2590R2.
- **GCC.** Latest releases: **GCC 16.2.0 (2026-08-07)**, GCC 16.1.0 (2026-04-30), GCC 15.3.0 (2026-06-12). Per the GCC 16.1 announcement (seen via search results; gcc.gnu.org itself was unreachable from this environment), GCC 16 defaults to GNU C++20, C++20 modules remain experimental behind `-fmodules`, and C++26 reflection is experimental behind `-freflection`. Search results quoting the GCC status page state C++23 features are available since GCC 11 (with per-feature versions); the per-feature core-language table could not be fetched.
- **libstdc++ C++23 library (GCC 16 branch status file).** `<print>` 14.1; `std`/`std.compat` modules 15.1; `<flat_map>` 15.1; `<mdspan>` 16.1; `<stacktrace>`, `<expected>`, `move_only_function` 12.1. Not yet implemented: P0533R9 (constexpr `<cmath>`), P2614R2, P2652R2, P2588R3, P2438R2.
- **`import std` with CMake** (see §3.1): Clang 18.1.2 or newer with libc++ or libstdc++; GCC 15 or newer (not macOS); MSVC toolset 14.36 or newer; Ninja generators only. "Ubuntu prior to 26.04 ships broken `libstdc++.modules.json` files."

**Sources.**
- https://github.com/llvm/llvm-project/releases (checked 2026-10-17)
- https://raw.githubusercontent.com/llvm/llvm-project/main/clang/www/cxx_status.html (source of https://clang.llvm.org/cxx_status.html, which was unreachable) (checked 2026-10-17)
- https://libcxx.llvm.org/Status/Cxx23.html (published form; the source read was its `Cxx23Papers.csv` status file in the libc++ documentation folder of the llvm-project repository, branch `release/23.x`, through raw.githubusercontent.com, because libcxx.llvm.org was unreachable) (checked 2026-10-17)
- https://github.com/gcc-mirror/gcc/tags (checked 2026-10-17)
- https://raw.githubusercontent.com/gcc-mirror/gcc/releases/gcc-16/libstdc++-v3/doc/xml/manual/status_cxx2023.xml (checked 2026-10-17)
- https://gcc.gnu.org/pipermail/gcc-announce/2026/000190.html (GCC 16.1 announcement; **not reachable**, content taken from search-result summaries, medium confidence) (checked 2026-10-17)
- https://gcc.gnu.org/projects/cxx-status.html (**not reachable**; C++23 "available since GCC 11" taken from search-result snippets) (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Linux server toolchain: Clang 23 with libc++ as primary, GCC 16 as secondary**, both in C++23 mode (`-std=c++23`). Using Clang on Linux keeps one front-end family close to `clang-cl`, which CMake supports for module scanning on Windows too (clang-cl 19.1 or newer, Ninja).
- **Shared-code rule:** the server and shared simulation code use only the intersection of MSVC 14.51, Clang 23/libc++ and GCC 16/libstdc++ C++23 support. CI builds that code with all three on every change. Concretely, the shared code avoids `std::stacktrace`, `std::generator`, `std::move_only_function` (all missing in libc++), constexpr `<cmath>`, extended floating-point types, and every C++26 feature.
- **`import std` on Linux:** allowed only behind the same build option as on Windows; Linux CI images must be Ubuntu 26.04 or newer (or another distribution with a working `libstdc++.modules.json`) when the option is on.
- Re-check GCC's per-feature C++23 core table when gcc.gnu.org can be reached; X-ADR should verify this item.

## 3 Build and dependencies

### 3.1 CMake and CMake presets

**Current fact (checked 2026-10-17).**
- Latest CMake release: **4.4.4 (Latest, 2026-10-02)**; **4.5.0-rc1** is a pre-release (2026-10-15). Older series still patched: 4.3.5 and 4.2.8 (2026-09-04).
- **Presets.** `CMakePresets.json` (project, checked in) and `CMakeUserPresets.json` (local, not checked in; it implicitly includes `CMakePresets.json`). Highest schema version in CMake 4.4: **12** (added in 4.4; 11 in 4.3; 10 in 3.31; 9 in 3.30). Preset kinds: `configurePresets`, `buildPresets`, `testPresets`, `packagePresets`, `workflowPresets`.
- **C++ modules.** Module scanning works with MSVC toolset 14.34 or newer, Clang 16 or newer, clang-cl 19.1 or newer (not the ClangCl toolset under Visual Studio generators), and GCC 14 or newer, with the Ninja, Ninja Multi-Config, "Visual Studio 17 2022" and "Visual Studio 18 2026" generators; Ninja 1.11 or newer. Header units are not supported. Visual Studio generators cannot export or install module information, and cannot compile BMIs from IMPORTED targets.
- **`import std`.** In CMake 4.4 it is **experimental**, enabled only through the `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` gate, requested per target with the `CXX_MODULE_STD` property, and supported only by Ninja generators. The development documentation for CMake 4.5 describes `import std` support "added in CMake 4.5" with no experimental gate.

**Sources.**
- https://github.com/Kitware/CMake/releases (checked 2026-10-17)
- https://raw.githubusercontent.com/Kitware/CMake/v4.4.4/Help/manual/cmake-presets.7.rst (source of https://cmake.org/cmake/help/v4.4/manual/cmake-presets.7.html; cmake.org was unreachable) (checked 2026-10-17)
- https://raw.githubusercontent.com/Kitware/CMake/v4.4.4/Help/manual/cmake-cxxmodules.7.rst (checked 2026-10-17)
- https://raw.githubusercontent.com/Kitware/CMake/master/Help/manual/cmake-cxxmodules.7.rst (development docs for 4.5) (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Build system: CMake, minimum 4.4** (`cmake_minimum_required(VERSION 4.4)`), presets schema **version 10** unless a version-11 or version-12 field is needed (use the lowest schema version that provides the fields Species uses, so that tools reading the presets with an older bundled CMake still accept them; which CMake version Visual Studio bundles was not checked; raise the schema deliberately).
- **Generator: Ninja Multi-Config** for every CI and command-line build on Windows and Linux. The Visual Studio IDE opens the CMake folder directly; do not generate `.sln` files as the primary path, because Visual Studio generators cannot build `import std`.
- **Preset set:** `windows-msvc-{debug,release,profile}`, `windows-clangcl-release` (optional second compiler), `linux-clang-{debug,release}`, `linux-gcc-release`, each with matching build and test presets, and one `workflowPreset` per CI job.
- Move `import std` from the experimental gate to the stable path when CMake 4.5 is released, and re-check the then-current docs.

### 3.2 vcpkg manifest mode

**Current fact (checked 2026-10-17).**
- vcpkg recommends **manifest mode** for most users: a `vcpkg.json` declares dependencies; versioning, overrides and custom registries require manifest mode.
- Versioning uses minimal version selection. `"builtin-baseline"` (a vcpkg registry commit SHA) sets the version floor; `"version>="` raises a floor; `"overrides"` pins an exact version. Without a baseline or registries, versioning information is ignored (classic algorithm). `vcpkg x-update-baseline` moves the baseline.
- CMake integration is the toolchain file `scripts/buildsystems/vcpkg.cmake`, set as `toolchainFile` in presets (schema 3 or later). In manifest mode the toolchain runs `vcpkg install` during `project()`; installed files go to `${CMAKE_BINARY_DIR}/vcpkg_installed` by default. All vcpkg variables (`VCPKG_TARGET_TRIPLET`, `VCPKG_HOST_TRIPLET`, `VCPKG_MANIFEST_DIR`) must be set before the first `project()`, for example in preset `cacheVariables`.
- `vcpkg-configuration.json` adds registries and overlay ports and triplets.
- Latest registry release tag: **2026.07.29** (published 2026-07-31).

**Sources.**
- https://learn.microsoft.com/vcpkg/concepts/manifest-mode (checked 2026-10-17)
- https://learn.microsoft.com/vcpkg/users/versioning (checked 2026-10-17)
- https://learn.microsoft.com/vcpkg/users/buildsystems/cmake-integration (checked 2026-10-17)
- https://learn.microsoft.com/vcpkg/consume/lock-package-versions (checked 2026-10-17)
- https://github.com/microsoft/vcpkg/tags (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Dependencies: vcpkg in manifest mode**, with `vcpkg.json` and `vcpkg-configuration.json` at the repository root, a pinned `builtin-baseline`, and vcpkg itself as a pinned Git submodule or a pinned checkout in CI. Never classic mode.
- **Custom triplets as overlay triplets** so that every dependency is built with the same toolset, the same C++ mode (§2.2) and the same CRT linkage: `x64-windows-species` (static libraries, dynamic CRT `/MD`) and `x64-linux-species`. The C++23-preview ABI caveat of §2.2 makes rebuilding every port on a toolset change mandatory, so the triplet records the toolset version.
- **Binary caching** in CI (a file or NuGet feed), keyed by vcpkg's ABI hash; the choice of cache host is an infrastructure decision for the design phase.
- The DirectX packages (Agility SDK headers, DirectXTex, DirectStorage, WinPixEventRuntime) are taken from vcpkg where a port exists, or from the vendor NuGet packages otherwise; §4 and §5 name the versions to pin.

## 4 Direct3D 12

### 4.1 Agility SDK

**Current fact (checked 2026-10-17).**
- **Latest retail Agility SDK: 1.619.6** (NuGet `Microsoft.Direct3D.D3D12`), **D3D12SDKVersion 619**, released 2026-09-14. 1.619 (first released 2026-02-26 as 1.619.0) brings Shader Model 6.9, DXR 1.2 (Shader Execution Reordering, Opacity Micromaps), revised resource view creation APIs (byte offsets, `HRESULT`-returning variants), periodic trim notifications, an increased dispatch grid limit, CPU timeline query resolves and Advanced Shader Delivery tooling. Point releases .1 to .6 are fixes (including enhanced-barrier debug-layer fixes in .2 and `D3D12Events.h` in .5).
- **Latest preview: 1.721.3-preview** (SDK version 721, 2026-07-30): Shader Model 6.10 preview features (linear algebra, partial programs, GUID texture layout, UAVs of depth). The 1.719-preview line added fence barriers (an Enhanced Barriers extension).
- **Mechanism.** `D3D12.dll` is a loader; the application ships `D3D12Core.dll` from the SDK and exports `D3D12SDKVersion` and `D3D12SDKPath` from the main `.exe`. If the OS carries a newer `D3D12Core.dll`, the OS copy loads instead. `D3D12CreateDevice` fails if the exported version does not match the shipped `D3D12Core.dll`. Microsoft recommends a subdirectory (default `.\D3D12\`) rather than the `.exe` directory, including Agility SDK headers before Windows SDK headers, and not shipping `D3D12SDKLayers.dll`.
- **OS support:** every Windows 10 1909 or later OS supports the Agility SDK, and all server builds from Windows Server 2022 on; so every OS allowed by §1.1 supports it.
- **DirectX-Headers** (GitHub `microsoft/DirectX-Headers`): latest release v1.619.5 (marked Latest, 31 July); v1.721.2-preview is the latest preview tag.

**Sources.**
- https://devblogs.microsoft.com/directx/directx12agility/ (download table) (checked 2026-10-17)
- https://devblogs.microsoft.com/directx/shader-model-6-9-retail-and-more/ (2026-02-26) (checked 2026-10-17)
- https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/ (checked 2026-10-17)
- https://github.com/microsoft/DirectX-Headers/releases (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Ship Agility SDK 1.619.6** (D3D12SDKVersion 619), exported from `species.exe`, with `D3D12SDKPath = ".\\D3D12\\"`. Do not ship a preview SDK.
- Pin the SDK version in one place (vcpkg port or NuGet package version, plus the exported constant), and add a build check that the exported number equals the packaged `D3D12Core.dll` minor version.
- The dedicated server does not link Direct3D at all.

### 4.2 Feature level: 12_1 versus 12_2

**Current fact (checked 2026-10-17).**
- **12_1** (Learn feature-level table): Shader Model 6.0 at the feature-level floor, Resource Binding Tier 2 or higher (the Direct3D 11 "downlevel" overview table lists Tier 3 for 12_0 and 12_1; the two Learn pages disagree), Tiled Resources Tier 2, Conservative Rasterisation Tier 1, Rasterizer Ordered Views, min/max filters, BC1 to BC7, max texture dimension 16384.
- **12_2 ("DirectX 12 Ultimate")** adds, as requirements: Shader Model 6.5, WDDM 2.0, DirectX Raytracing Tier 1.1, Variable Rate Shading Tier 2, Mesh Shader Tier 1, Sampler Feedback Tier 0.9, Resource Binding Tier 3, Tiled Resources Tier 3, Conservative Rasterisation Tier 3, Root Signature 1.1, depth-bounds test, write-buffer-immediate on direct, compute and bundle queues, 40-bit GPU virtual addresses, and the flags WaveOps, OutputMergerLogicOp, VP/RT array index from any shader without GS emulation, copy-queue timestamp queries, casting fully typed formats, Int64ShaderOps.
- **Hardware reach.** Microsoft's DirectX 12 Ultimate announcement names AMD RDNA 2 and NVIDIA GeForce RTX as the partner hardware lines. No primary source checked gives a market-share figure for 12_2 versus 12_1 hardware; the Steam hardware survey (a secondary source) was not consulted because it cannot be reached reliably from this environment. **Reach: not verified.**

**Sources.**
- https://learn.microsoft.com/windows/win32/direct3d12/hardware-feature-levels (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/direct3d11/overviews-direct3d-11-devices-downlevel-intro (12_2 column) (checked 2026-10-17)
- https://raw.githubusercontent.com/microsoft/DirectX-Specs/master/d3d/D3D12_FeatureLevel12_2.md (checked 2026-10-17)
- https://devblogs.microsoft.com/directx/announcing-directx-12-ultimate/ (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Minimum: feature level 12_1, plus three capability checks that the renderer depends on: Shader Model 6.6, Resource Binding Tier 3, and `D3D12_FEATURE_DATA_D3D12_OPTIONS1.WaveOps`.** Fail with a clear message if any is missing.
- **12_2 is not required.** Species' look (flat-shaded low-poly, additive glow on black, a darkening layer, crowds of small figures) needs none of raytracing, mesh shaders, sampler feedback or VRS. What 12_2 would buy: mesh/amplification shaders for crowd culling, VRS for cheap glow passes, and sampler feedback for texture streaming; what it would cost is every 12_1-only GPU (the reach of that cost is not verified).
- Design the crowd renderer around compute culling plus `ExecuteIndirect`, which works at 12_1. Mesh shaders and VRS are optional fast paths behind capability checks, never the only path.

### 4.3 Shader Model

**Current fact (checked 2026-10-17).**
- **Current retail Shader Model: 6.9** (Agility SDK 1.619, DXC 1.9.2602.16 and later); Shader Model 6.10 is in preview (Agility SDK 1.720/1.721 previews, DXC 1.10.2605.x). SM 6.9 makes 16-bit and 64-bit shader ops and wave ops required. Per Microsoft's SM 6.9 feature-support table, long vectors and 16-bit float specials run on AMD Radeon RX 9000 series, Intel Arc B-series and all NVIDIA RTX hardware with current drivers.
- **SM 6.6 dynamic resources** (`ResourceDescriptorHeap[]`, `SamplerDescriptorHeap[]`) "must be supported on devices that support both `D3D12_RESOURCE_BINDING_TIER_3` and `D3D_SHADER_MODEL_6_6`". They need the root-signature flags `CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED` and `SAMPLER_HEAP_DIRECTLY_INDEXED` (global root signatures only).
- SM 6.6 support was added in Windows 11 and the Agility SDK. Query the highest supported model with `D3D12_FEATURE_SHADER_MODEL`, looping downwards, because `CheckFeatureSupport` returns `E_INVALIDARG` for a model the runtime does not know.

**Sources.**
- https://devblogs.microsoft.com/directx/shader-model-6-9-retail-and-more/ (checked 2026-10-17)
- https://raw.githubusercontent.com/microsoft/DirectX-Specs/master/d3d/HLSL_SM_6_6_DynamicResources.md (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/api/d3d12/ne-d3d12-d3d_shader_model and https://learn.microsoft.com/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_shader_model (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Minimum Shader Model 6.6**; compile every shader to `*_6_6` profiles. Bindless (dynamic-resource) descriptor access is the only binding model: one shader-visible CBV/SRV/UAV heap and one sampler heap, both directly indexed.
- Do not target SM 6.7 or later in the baseline; SM 6.9 hardware support is still limited to the newest GPU generations listed above. An SM 6.7+ feature may be used only behind a capability check with an SM 6.6 fallback, and only with an ADR.

### 4.4 Enhanced Barriers

**Current fact (checked 2026-10-17).**
- Enhanced Barriers shipped in retail Agility SDK 1.608.0 (2022-11-18). "Enhanced Barriers is not currently a hardware or driver requirement": before using `Barrier()` or resource creation with `InitialLayout`, check `D3D12_FEATURE_DATA_D3D12_OPTIONS12.EnhancedBarriersSupported`. The flag needs Agility SDK 1.6 or later; otherwise it is always `FALSE`.
- On the driver side, support is reported through the WDDM 3.0 DDI (Windows 11 22H2 WDK); a newer barrier DDI revision lists Windows 11 24H2. A driver that supports enhanced barriers also receives legacy `ResourceBarrier` calls through the same entry point.
- Fence barriers (an extension) are preview-only (Agility SDK 1.719/1.721 previews).

**Sources.**
- https://learn.microsoft.com/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_d3d12_options12 (checked 2026-10-17)
- https://learn.microsoft.com/windows-hardware/drivers/display/enhanced-barriers (checked 2026-10-17)
- https://learn.microsoft.com/windows-hardware/drivers/ddi/d3d12umddi/nc-d3d12umddi-pfnd3d12ddi_barrier_0094 (checked 2026-10-17)
- https://devblogs.microsoft.com/directx/directx12agility/ (1.608.0 entry) (checked 2026-10-17)

**Implication for Species (recommendation).**
- The frame graph expresses all synchronisation in Enhanced-Barrier terms (sync, access, layout). The command-recording layer emits `Barrier()` when `EnhancedBarriersSupported` is `TRUE`, and translates to legacy `ResourceBarrier` otherwise. Because support is optional, it is not a minimum requirement; whether to require it later is an ADR decision for the design phase.
- Do not use fence barriers (preview).

### 4.5 DXC and HLSL 2021

**Current fact (checked 2026-10-17).**
- Latest DXC release: **v1.9.2609** ("DX Compiler Release for September 2026", Latest, 2026-09-29). Latest preview: v1.10.2605.37 (Shader Model 6.10 preview, patch 2).
- **HLSL 2021 is the default language version since DXC 1.7.2308** (August 2023). "HLSL 202x" is a placeholder for the next language version (`-HV 202x`); recent releases remove legacy effects syntax, `interface`, `shared` and `uniform` under 202x.

**Sources.**
- https://github.com/microsoft/DirectXShaderCompiler/releases (checked 2026-10-17)
- `ReleaseNotes.md` in the documentation folder of https://github.com/microsoft/DirectXShaderCompiler (main branch, read through raw.githubusercontent.com) (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Compile shaders offline with a pinned DXC (v1.9.2609)**, invoked by the asset pipeline, with `-HV 2021` passed explicitly, `-enable-16bit-types` only where the capability is checked, warnings as errors, and PDBs kept for PIX. Ship DXIL only; do not ship `dxcompiler.dll` in the client.
- Write shaders so that they also compile under `-HV 202x` (no effects syntax, no `interface`, no `uniform`), so that the next language version is a small step.

### 4.6 Swap chain, variable refresh rate and HDR

**Current fact (checked 2026-10-17).**
- **Flip model.** Microsoft: if still using the blt swap effects, "it's time to stop". `DXGI_SWAP_EFFECT_FLIP_DISCARD` or `FLIP_SEQUENTIAL` gives better performance and power, and a flip-model borderless window is "effectively equivalent or better" than fullscreen exclusive. **Direct3D 12 does not support fullscreen exclusive mode**; the OS uses fullscreen optimisations. MSAA swap chains are not supported in flip model (resolve to the back buffer). `DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT` lowers latency.
- **VRR.** Variable refresh rate needs tearing enabled: create the swap chain with `DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING` (flip model only; cannot be added by `ResizeBuffers`), present with `Present(0, DXGI_PRESENT_ALLOW_TEARING)` (sync interval 0 only), and check `IDXGIFactory5::CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING)` first.
- **HDR (Advanced Colour).** Option 1, recommended for general apps: `DXGI_FORMAT_R16G16B16A16_FLOAT` with scRGB (`DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709`), the DWM's own composition space. Option 2, a performance option for full-screen HDR without alpha blending of the swap chain: `R10G10B10A2_UNORM` with `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` (HDR10). Read the display's luminance and primaries from `IDXGIOutput6::GetDesc1`; re-query when `IDXGIFactory1::IsCurrent` returns `FALSE`; tone-map to the display instead of relying on `SetHDRMetaData`; map UI to the user's SDR reference white level.

**Sources.**
- https://learn.microsoft.com/windows/win32/direct3ddxgi/for-best-performance--use-dxgi-flip-model (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/direct3d12/swap-chains (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/direct3ddxgi/variable-refresh-rate-displays (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_chain_flag (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/direct3darticles/high-dynamic-range (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/api/dxgi1_5/nf-dxgi1_5-idxgiswapchain4-sethdrmetadata (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Swap chain:** `FLIP_DISCARD`, 3 back buffers, `FRAME_LATENCY_WAITABLE_OBJECT` with a maximum frame latency of 1 or 2, `ALLOW_TEARING` when supported. Window modes: windowed and borderless full screen only (no exclusive mode). The CPU never runs more than (buffer count minus one) frames ahead, enforced with a fence.
- **VRR:** "uncapped/VRR" present mode uses sync interval 0 with `ALLOW_TEARING`; "vsync" uses sync interval 1. Both are user options.
- **HDR: optional, off by default.** The SDR path renders to an 8-bit sRGB back buffer. The HDR path uses scRGB FP16 (option 1), because the additive glow on black depends on blending and on values above 1.0; the tone mapper reads `GetDesc1` and the UI uses the SDR reference white level. HDR10 (option 2) is not used.

### 4.7 PIX and WinPixEventRuntime

**Current fact (checked 2026-10-17).**
- **PIX on Windows, latest main version 2603.25** (x64 and Arm64); latest preview 2606.18-preview (adds DirectX dump files). Microsoft states that PIX supports all retail and preview features of Agility SDK 1.619/1.719. Multiple PIX versions install side by side.
- **WinPixEventRuntime** (NuGet): latest **1.0.240308001** (listed as last updated 2024-03-27). The package combines the PIX event headers and prebuilt `WinPixEventRuntime.dll`; Microsoft recommends the DLL over the static library. The source is on GitHub (`microsoft/PixEvents`), and since March 2024 the NuGet package is under the MIT licence.

**Sources.**
- https://devblogs.microsoft.com/pix/download/ (checked 2026-10-17)
- https://www.nuget.org/packages/WinPixEventRuntime (checked 2026-10-17)
- https://github.com/microsoft/PixEvents (checked 2026-10-17)

**Implication for Species (recommendation).**
- **PIX 2603.25 or later** is the GPU and timing-capture tool. Every frame-graph pass, job-system task group and simulation tick phase carries a PIX event (CPU and GPU) through one Species macro layer.
- **WinPixEventRuntime 1.0.240308001** (MIT) linked as a DLL in development and profile builds; retail builds compile the markers out (the macro layer expands to nothing), so `WinPixEventRuntime.dll` is not shipped. Whether to keep markers in retail for field captures is a design-phase choice.
- Keep the D3D12 debug layer and GPU-based validation in a CI test configuration; never ship `D3D12SDKLayers.dll`.

## 5 Content I/O and textures

### 5.1 DirectStorage

**Current fact (checked 2026-10-17).**
- **Latest stable: DirectStorage 1.3.0** (NuGet `Microsoft.Direct3D.DirectStorage`, 2025-06-27). Previews: **1.4.0-preview1** (2026-03-09) and **1.4.0-preview2** (2026-06-16). The package contains the SDK and the redistributable binaries.
- DirectStorage 1.4 (public preview, announced 2026-03-11) adds Zstandard (Zstd) with CPU and GPU decompression, an open-source Zstd GPU decompression shader "initially optimized for content chunked to 256KB or smaller", and global D3D12 CreatorID support. Its companion, the Game Asset Conditioning Library (GACL, preview), conditions BCn data for better Zstd ratios (post-processing supported for BC1, BC3, BC4 and BC5; BC7 "in a future DirectStorage update"). IHV-optimised Zstd drivers were announced for the second half of 2026.
- DirectStorage 1.3 introduced `EnqueueRequests`. Built-in GPU format: GDeflate (`DSTORAGE_COMPRESSION_FORMAT_GDEFLATE`), with a fallback order of optimised driver path, then built-in GPU fallback shader, then CPU fallback (`IDStorageQueue2::GetCompressionSupport` reports which). Custom formats go through `IDStorageCustomDecompressionQueue`.
- Design goal (Microsoft): many small (for example 64 KB) reads per second from NVMe at minimal CPU cost.

**Sources.**
- https://www.nuget.org/packages/Microsoft.Direct3D.DirectStorage (checked 2026-10-17)
- https://devblogs.microsoft.com/directx/directstorage-1-4-release-adds-support-for-zstandard/ (2026-03-11) (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/dstorage/dstorage-portal (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/dstorage/dstorage/ne-dstorage-dstorage_compression_support (checked 2026-10-17)
- https://github.com/microsoft/DirectStorage/releases ("There aren't any releases here"; versions are published on NuGet only) (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Not adopted for v1** (judgement, medium confidence). Species' art is flat-shaded low-poly with vertex colour and few, small textures. The reference renderer's assets fit in memory, so load time is dominated by parsing and GPU upload, not by I/O throughput, which is where DirectStorage helps. The dedicated server, which may run on Linux, cannot use DirectStorage, so the asset I/O layer must be abstracted anyway.
- **Keep the door open:** the runtime package format stores assets in independently compressed chunks of at most 256 KiB, aligned to 64 KiB, with a table of contents, so a DirectStorage back-end (1.3.0, or 1.4 once retail) can be added behind the same I/O interface. That would be for terrain streaming at the strategic zoom level, if the design phase finds it I/O-bound.
- Do not use the 1.4 preview or GACL in shipping builds.

### 5.2 DirectXTex and texconv

**Current fact (checked 2026-10-17).**
- **Latest DirectXTex release: `may2026`** (Latest, 2026-05-08): an HDR-reader fix for malformed headers and CMake project updates. Distributed as NuGet 2026.5.8, through vcpkg, and as tools installable with winget.
- **texconv** "loads an image and prepares it for runtime use by resizing, format conversion, mip-map generation, block-compression" and writes a runtime format. Relevant options: `-f <DXGI format without the DXGI_FORMAT_ prefix>`; `-bc` flags `u`, `d` (BC1 to BC3), `q` (BC7 mode 6 only) and `x` (BC7 modes 0 and 2 enabled); `-m` (0 means all mips); `-srgb`, `-srgbi`, `-srgbo`; `-pmalpha`; `-dx10` (always write the DDS "DX10" header extension) and `-dx9` (legacy headers). For BC6H and BC7, texconv uses DirectCompute on the GPU adapter given by `-gpu` (default adapter 0) "if available"; `-nogpu` forces the software codec.

**Sources.**
- https://github.com/microsoft/DirectXTex/releases (checked 2026-10-17)
- https://github.com/microsoft/DirectXTex/wiki/Texconv (raw: https://raw.githubusercontent.com/wiki/microsoft/DirectXTex/Texconv.md) (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Texture cooking uses the DirectXTex library (`may2026`, from vcpkg) called from the Species asset cooker**, with texconv used for manual inspection only. Cooking options are data, stored next to each source texture, never typed by hand.
- **Always write DDS with the DX10 header** (§8.2), always generate full mip chains, and always state colour space explicitly (sRGB colour; linear data).
- In CI, cook BC7 with the software codec (`-nogpu` behaviour), so that cooked output does not depend on the build machine's GPU or driver (a reproducibility precaution; the source does not state that GPU and CPU output differ). Local artists may use the GPU path for speed.

### 5.3 BC7, BC5 and BC4

**Current fact (checked 2026-10-17).**
- **BC4** (`DXGI_FORMAT_BC4_UNORM`/`SNORM`): one channel at 8 bits, 8 bytes per 4x4 block, since Direct3D 10.
- **BC5** (`BC5_UNORM`/`SNORM`): two channels at 8:8, 16 bytes per block, since Direct3D 10.
- **BC7** (`BC7_UNORM`, `BC7_UNORM_SRGB`): RGB or RGBA, "high-quality compression", 8 encoding modes chosen per block, 16 bytes per block, since Direct3D 11; hardware decoding is bit-accurate.
- Feature-level support: BC4/BC5 and BC6H/BC7 are "Yes" at every feature level from 11_0 up, so every Species-capable GPU (§4.2) supports all three.

**Sources.**
- https://learn.microsoft.com/windows/win32/direct3d11/texture-block-compression-in-direct3d-11 (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/direct3d11/bc7-format (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/direct3d11/overviews-direct3d-11-devices-downlevel-intro (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Format rules for the cooker:** colour and UI art in `BC7_UNORM_SRGB`; normal maps (if any) in `BC5_UNORM` (two channels, Z reconstructed in the shader); single-channel data (glow masks, darkening masks, height, AO) in `BC4_UNORM`; terrain or masks that need exact values stay uncompressed (`R8_UNORM`, `R8G8_UNORM`). No BC1 to BC3 in new content; BC6H only if HDR sky or light textures are added.
- Glyph atlases for UI text are never block-compressed (§9).

## 6 Input: GameInput

**Current fact (checked 2026-10-17).**
- **Latest package: `Microsoft.GameInput` 4.0.300 (NuGet, 2026-10-15)**: "Increased API version to 4", side-by-side Windows and Xbox binaries, device lighting, touch devices, Nintendo Switch Pro controller. Previous line: 3.5.283 (2026-09-28). The 3.5 notes moved the public header and static library to the MIT licence; the licence for the redistributable binaries is linked from the package page but was not read.
- **API versioning.** From v1 on, each API version is in its own namespace (Microsoft's example is `GameInput::v1`; the NuGet notes give namespace examples up to v3 and do not say whether 4.0 adds a `v4` namespace) with distinct interface IDs, all implemented by the same runtime objects (switch with `QueryInterface`); `GAMEINPUT_API_VERSION` gives the compiled version. Binary and functional compatibility is kept for apps compiled against earlier versions. The v0 API in the Windows SDK is still supported but frozen.
- **Redistribution.** "Although GameInput is supported out of the box on modern versions of Windows, PC games are required to incorporate the GameInput redistributable into their respective installers." The NuGet package places `GameInputRedist.msi` under `redist`; it is not installed automatically; it refuses to downgrade a newer installed runtime. Developers can also run `winget install Microsoft.GameInput`. The GameInput service arbitrates device access.
- **Platforms:** Windows 10 19H1 or later, Windows 11, Xbox (v1+ with GDK 2510 or later). Almost all calls are safe on time-sensitive threads; `GameInputCreate` belongs in initialisation.

**Sources.**
- https://www.nuget.org/packages/Microsoft.GameInput (checked 2026-10-17)
- https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-nuget (checked 2026-10-17)
- https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-versioning (checked 2026-10-17)
- https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-faq (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Gamepads and other game devices: GameInput**, built against a pinned `Microsoft.GameInput` package and a pinned API namespace. Recommended pin: **3.5.283 with `GameInput::v3`** now; move to 4.0.x (and its API-version-4 namespace, once confirmed) after its first point release, since 4.0.300 is two days old at the run date. Either way, the installer runs the `GameInputRedist.msi` from the same package (or newer).
- **Mouse, keyboard and text: Win32 window messages** (`WM_MOUSEMOVE`, the button messages, `WM_INPUT` for raw mouse deltas in camera drag, `WM_KEYDOWN`/`WM_KEYUP` for bindings, `WM_CHAR` plus IME messages for text entry). An RTS is mouse-and-keyboard first, and text entry (chat, names, search) needs the IME path. GameInput mouse and keyboard support is not used in v1 (a design choice; revisit with an ADR if GameInput's keyboard path proves better for bindings).
- One input-abstraction layer maps all devices to Species actions, so rebinding (an accessibility requirement, §10.1) is done once.
- The headless server never links GameInput.

## 7 Audio: XAudio2 2.9 and X3DAudio

**Current fact (checked 2026-10-17).**
- **In-box:** "XAudio2 version 2.9 ships as part of Windows 10, XAUDIO2_9.DLL." `X3DAudioInitialize`, `X3DAudioCalculate`, `CreateFX`, `CreateAudioReverb`, `CreateAudioVolumeMeter` and `XAudio2Create` are exported from `xaudio2_9.dll` since build 10.0.10240 and are listed among the APIs present on all Windows devices. X3DAudio and XAPOFX keep separate headers but link through the single XAudio2 import library.
- **Redistributable:** `Microsoft.XAudio2.Redist` (latest **1.2.13**, last updated 2025-04-10; also a vcpkg port `xaudio2redist`) provides `XAUDIO2_9REDIST.DLL` for Windows 7 SP1 to Windows 11; on Windows 10/11 it delegates to the OS `XAUDIO2_9.DLL` when that is newer. `CreateHrtfApo` is not implemented in the redistributable.
- **Behaviour:** XAudio 2.9 uses a virtual audio endpoint and follows default-device changes, unless the mastering voice was created with a fixed format the new device cannot take (then `XAUDIO2_E_DEVICE_INVALIDATED`). From Windows 10 1903, XAudio 2.9 output goes to the user's system spatial-sound format (for example Windows Sonic) instead of the usual fold-down. The graph processes 32-bit float internally with a 10 ms quantum on Windows (`XAUDIO2_1024_QUANTUM` is an option).
- **Input formats on PC:** linear 16-bit PCM (optimal), 32-bit float PCM (preferred after 16-bit), other integer PCM widths, MS-ADPCM (XAudio2's modified codec, encoded with the AdpcmEncode tool), and xWMA. XMA is Xbox-only.

**Sources.**
- https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-versions (checked 2026-10-17)
- https://learn.microsoft.com/uwp/win32-and-com/win32-apis#apis-from-xaudio2_9dll (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-redistributable (checked 2026-10-17)
- https://www.nuget.org/packages/Microsoft.XAudio2.Redist (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/api/xaudio2/nf-xaudio2-ixaudio2-createsourcevoice (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-audio-graph (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/xaudio2/adpcm-overview (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Use the in-box XAudio 2.9 and X3DAudio** (link `xaudio2.lib` from the Windows SDK). Do not ship the redistributable: every supported OS (§1.1) has `XAUDIO2_9.DLL`.
- Create the mastering voice with the default device and default format (no fixed sample rate), so device switches work; handle `OnCriticalError` and `XAUDIO2_E_DEVICE_INVALIDATED` by rebuilding the graph, and test device changes while paused (Microsoft's testing note).
- Positional crowd and battle sounds use X3DAudio; the HRTF APO is not used.
- The headless server has no audio.

## 8 Asset formats

### 8.1 glTF 2.0 and KHR extensions

**Current fact (checked 2026-10-17).**
- **glTF 2.0** (Khronos; specification source copyright 2013–2021). Coordinate system: right-handed, +Y up, asset front faces +Z; linear units are metres.
- **Flat shading in core glTF:** "When normals are not specified, the mesh primitive has implicit flat normals"; tangents are then ignored and "SHOULD NOT be present".
- **Vertex colours in core glTF:** `COLOR_n` is an RGB or RGBA vertex colour (accessor `VEC3` or `VEC4`; float32, unorm8 or unorm16); a `VEC3` colour has alpha 1.0. `COLOR_0` is used as an extra multiplier for the base colour.
- **Ratified KHR extensions** (24 listed in the Khronos repository): including `KHR_materials_unlit`, `KHR_materials_emissive_strength`, `KHR_mesh_quantization`, `KHR_texture_transform`, `KHR_texture_basisu`, `KHR_draco_mesh_compression`, `KHR_lights_punctual`, `KHR_materials_variants`, `KHR_animation_pointer`, `KHR_node_visibility`, `KHR_xmp_json_ld`. `KHR_materials_diffuse_transmission` is a release candidate. A multi-vendor `EXT_meshopt_compression` is listed; there is no `KHR_meshopt_compression`.

**Sources.**
- https://raw.githubusercontent.com/KhronosGroup/glTF/main/specification/2.0/Specification.adoc (source of https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html; the registry host was not tried separately because other non-GitHub hosts were unreachable) (checked 2026-10-17)
- https://github.com/KhronosGroup/glTF/tree/main/extensions (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Source interchange format for meshes and animation: glTF 2.0** (`.glb` for checked-in sources), with Species' own cooked runtime format produced by the asset cooker. The runtime never parses glTF.
- **Flat-shaded low-poly look:** author meshes *without* `NORMAL` where the faceted look is wanted (the cooker generates face normals, which the specification mandates as the meaning), and with `COLOR_0` (unorm8 RGBA) for per-face or per-vertex colour, multiplied with a white or palette base colour.
- **Allowed extensions in source assets:** `KHR_materials_unlit` (glow and UI meshes), `KHR_materials_emissive_strength` (emissive above 1.0 for the additive glow), `KHR_mesh_quantization` (compact positions). Everything else is rejected by the cooker unless an ADR adds it; compression extensions (`KHR_draco_mesh_compression`, `EXT_meshopt_compression`) are not needed for source files.
- The cooker validates every source file against the specification (for example with the Khronos glTF validator) before cooking.

### 8.2 DDS with the DX10 header

**Current fact (checked 2026-10-17).**
- A DDS file is the magic `'DDS '`, a 124-byte `DDS_HEADER` with a 32-byte `DDS_PIXELFORMAT`, then, when `dwFourCC` is `"DX10"`, a 20-byte `DDS_HEADER_DXT10` (`dxgiFormat`, `resourceDimension`, `miscFlag`, `arraySize`, `miscFlags2`), then the surface data. A reader validates at least 128 bytes (148 with the DX10 header) and the fixed sizes.
- The DX10 header handles texture arrays, cube-map arrays and DXGI formats that the legacy pixel format cannot express (floating point, sRGB, BC6H/BC7). Microsoft "highly recommend[s]" the DX10 extension instead of putting a `D3DFORMAT` or `DXGI_FORMAT` value in `dwFourCC`. Microsoft also notes that legacy pixel formats remain preferable "for maximum compatibility" for uncompressed RGB and DXT1–5, because not all tools read the DX10 extension.

**Sources.**
- https://learn.microsoft.com/windows/win32/direct3ddds/dx-graphics-dds-pguide (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/direct3ddds/dds-header-dxt10 (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/direct3ddds/dds-file-layout-for-textures (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Every cooked texture is a DDS with the DX10 header** (texconv `-dx10`, or the DirectXTex `DDS_FLAGS_FORCE_DX10_EXT` equivalent), so that `dxgiFormat`, sRGB-ness and array size are explicit and unambiguous. The compatibility caveat does not apply: only Species' own loader and DirectXTex read cooked files.
- Cooked DDS payloads go into the runtime package format (§5.1); the loader rejects any DDS without the DX10 header.

### 8.3 Audio source and runtime formats

**Current fact (checked 2026-10-17).**
- **XAudio2 inputs on PC:** 16-bit integer PCM ("optimal format for XAudio2"), 32-bit float PCM ("preferred format after 16-bit integer"), 8-, 20-, 24- and 32-bit integer PCM, MS-ADPCM (XAudio2's modified codec, up to 4:1, custom block sizes, encoded with the AdpcmEncode tool, loop points in `smpl`/`wsmp` chunks), and xWMA. XMA is Xbox-only. All input is decoded to 32-bit float inside the graph.
- The June 2010 SDK that carried XAudio 2.7 "is no longer available on Microsoft Downloads" (XAudio2 versions page); where the current xWMA and AdpcmEncode encoder tools are distributed was **not verified**.
- **Opus** (open codec, decoded in software): latest release on GitHub `xiph/opus` is **1.5.2** (marked Latest; the release list shows day and month only).

**Sources.**
- https://learn.microsoft.com/windows/win32/api/xaudio2/nf-xaudio2-ixaudio2-createsourcevoice (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-redistributable#xaudio-29-api-differences-compared-to-xaudio-27 (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/xaudio2/adpcm-overview (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-versions (checked 2026-10-17)
- https://github.com/xiph/opus/releases (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Source format:** uncompressed WAV, **48 kHz**, 24-bit integer (or 32-bit float) PCM; mono for every positional sound, stereo for music and UI. Sources are checked in; the cooker resamples nothing (sources are authored at 48 kHz).
- **Runtime formats:**
  - short and frequently triggered sound effects (unit barks, impacts, UI clicks): **16-bit PCM, 48 kHz**, fully resident;
  - larger effect banks if memory becomes a concern: **MS-ADPCM** (only once the encoder's distribution is confirmed; otherwise stay with PCM);
  - music and long ambiences: **streamed Opus (libopus 1.5.2 through vcpkg), decoded on a worker thread into PCM buffers submitted to XAudio2**. xWMA is not used, because its encoder's current distribution could not be verified and Opus is an open format with a maintained encoder.
- Fix the mastering format at nothing (device default) and let XAudio2 convert sample rates; all Species audio data is 48 kHz.

## 9 Text: DirectWrite glyph rasterisation for a glyph-cache UI

**Current fact (checked 2026-10-17).**
- `IDWriteFactory::CreateGlyphRunAnalysis` (and the `IDWriteFactory2`/`IDWriteFactory3` overloads with grid-fit and antialias modes; `IDWriteFactory3` from Windows 10) creates an `IDWriteGlyphRunAnalysis`. `GetAlphaTextureBounds` gives the pixel bounds; `CreateAlphaTexture` fills a buffer of type `DWRITE_TEXTURE_BILEVEL_1x1` (one byte per pixel, 0 or 255) or `DWRITE_TEXTURE_CLEARTYPE_3x1` (three bytes per pixel, antialiased glyphs). The rendering mode must be a raster mode (not default, not outline).
- `DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE` "computes one coverage value for each pixel", so that "text can be rendered onto a transparent bitmap, which can then be composited"; ClearType "is not suitable for rendering text onto a transparent intermediate bitmap".
- `IDWriteFontFace::GetGlyphRunOutline` returns glyph outlines through an `IDWriteGeometrySink` (= `ID2D1SimplifiedGeometrySink`), usable to build distance-field glyphs.
- Layout and shaping: `IDWriteTextLayout` with a custom `IDWriteTextRenderer` receives positioned glyph runs in `DrawGlyphRun`; `IDWriteFactory2::GetSystemFontFallback` and `CreateFontFallbackBuilder` give font fallback; `TranslateColorGlyphRun` splits colour-font runs into layers.

**Sources.**
- https://learn.microsoft.com/windows/win32/api/dwrite/nf-dwrite-idwritefactory-createglyphrunanalysis (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/api/dwrite_2/nf-dwrite_2-idwritefactory2-createglyphrunanalysis (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/api/dwrite_3/nf-dwrite_3-idwritefactory3-createglyphrunanalysis (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/api/dwrite/nn-dwrite-idwriteglyphrunanalysis and https://learn.microsoft.com/windows/win32/api/dwrite/nf-dwrite-idwriteglyphrunanalysis-createalphatexture (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/api/dwrite_1/ne-dwrite_1-dwrite_text_antialias_mode (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/directwrite/glyphs-and-glyph-runs and https://learn.microsoft.com/windows/win32/api/dwrite_2/nn-dwrite_2-idwritefactory2 (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Path:** `IDWriteTextLayout` for shaping, bidirectional text, line breaking and font fallback → a custom `IDWriteTextRenderer` that receives glyph runs → for each glyph not yet cached, `IDWriteFactory3::CreateGlyphRunAnalysis` with a natural-symmetric raster mode and `DWRITE_TEXT_ANTIALIAS_MODE_GRAYSCALE` → `CreateAlphaTexture` into a staging buffer → copy one coverage channel into an `R8_UNORM` atlas page on the GPU (premultiplied-alpha blending in the UI shader). A unit test asserts that the three channels returned for grayscale mode are equal before the cooker relies on taking one.
- **Cache key:** font face, glyph index, pixel size (after DPI and the XAG text-scale setting, §10.1), and subpixel x-offset quantised to a quarter pixel. LRU eviction by atlas page.
- **No ClearType** in the game UI (the UI is composited over the 3D scene and the glow). A distance-field atlas from `GetGlyphRunOutline` is an option for world-space labels at the strategic zoom; it is not the baseline.
- Fonts are shipped with the game (licensed for embedding) and loaded through a custom font collection; system fonts are only a fallback for user-entered text in scripts the shipped fonts do not cover.
- The headless server never touches DirectWrite.

## 10 Accessibility and localisation

### 10.1 Xbox Accessibility Guidelines (XAG)

**Current fact (checked 2026-10-17).**
- **Current version: XAG 3.2, published 2023-06-08** (no later entry in the version history). The XAGs are "best practices" for designers, developers and test teams, and are "not intended to act as a checklist to validate any type of compliance or legal requirements".
- Guidelines: 101 Text display, 102 Contrast, 103 Additional channels for visual and audio cues, 104 Subtitles and captions, 105 Audio accessibility, 106 Screen narration, 107 Input, 108 Game difficulty options, 109 Objective (object) clarity, 110 Haptic feedback, 111 Audio description, 112 UI navigation, 113 UI focus handling, 114 UI context, 115 Error messages and destructive actions, 116 Time limits, 117 Visual distractions and motion settings, 118 Photosensitivity, 119 Speech-to-text and text-to-speech chat, 120 Communication experiences, 121 Accessible feature documentation, 122 Accessible customer support, 123 Mental health best practices.
- Examples of measurable criteria (from the version history and the store accessibility-tag criteria): PC/VR minimum default text in icons and glyphs 9 px at 1080p and 19 px at 4K (version 2.0); gameplay-relevant text contrast at least 4.5:1, high-contrast mode at least 7:1, inactive elements 3:1 (version 3.0); a luminance flash is a 10% change in luminance (XAG 118, version 3.0); text left- or right-aligned by language, not centred or justified, or adjustable.
- Store "Accessibility Feature Tags" version 2.0.1 (2025-03-14) align with the Entertainment Software Association's Accessible Games Initiative tags.

**Sources.**
- https://learn.microsoft.com/xbox/accessibility/guidelines (checked 2026-10-17)
- https://learn.microsoft.com/xbox/accessibility/xag-version-history (checked 2026-10-17)
- https://learn.microsoft.com/gaming/game-publishing/concepts/metadata-accessibility (checked 2026-10-17)
- https://learn.microsoft.com/xbox/accessibility/accessibility-feature-tags (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Adopt XAG 3.2 as the accessibility checklist** for the PC client, and record conformance per guideline in the design phase's acceptance tests.
- Engine-level consequences that the baseline fixes now: a UI text-scale setting that feeds the glyph cache (§9); contrast-checked UI palettes, including a high-contrast mode (7:1), which matters for a glow-on-black look; full input remapping and hold/toggle options in the input layer (§6, XAG 107); colour never the only channel for race or team identity (shape or pattern as well, XAG 103); a photosensitivity check on glow and flash effects (XAG 118), which the additive-glow style makes a real risk; and a motion setting for camera shake (XAG 117).
- Screen narration (XAG 106) for menus is a design-phase decision; the UI framework keeps an accessible-name field on every widget so it can be added.

### 10.2 Unicode, CLDR and BCP 47

**Current fact (checked 2026-10-17).**
- **Unicode:** the latest stable ICU (**ICU 78.3**, GitHub `unicode-org/icu`, marked Latest) is based on **Unicode 17** (the ICU 78 release notes say it "updates to Unicode 17"); ICU 79 is at release candidate (79.1rc). unicode.org itself was unreachable from this environment, so the Unicode version is taken from the ICU release notes.
- **CLDR:** ICU 78.3 carries CLDR **48.2** fixes; **CLDR 49** is in beta (`release-49-beta4`, GitHub `unicode-org/cldr`, pre-release).
- **BCP 47:** Windows locale names follow IETF BCP 47 (Windows Vista and later), as `LOCALE_SNAME`, with the pattern `<language>-<REGION>`, `<language>-<Script>-<REGION>` when script matters (for example `uz-Latn-UZ`); `LOCALE_NAME_MAX_LENGTH` is 85 including the terminator. Microsoft's globalisation docs list the BCP 47 subtag structure (ISO 639 language, ISO 15924 script, ISO 3166-1 or UN M49 region, variants, extensions, private use). The RFC Editor's BCP 47 page could not be reached.
- **ICU in Windows:** `icu.dll` (combined common and i18n libraries, C APIs only) ships in Windows from version 1903 and is linked through `icu.lib` from the Windows SDK; "Not all data returned by ICU APIs will align with the Windows OS".

**Sources.**
- https://github.com/unicode-org/icu/releases (checked 2026-10-17)
- https://github.com/unicode-org/cldr/releases (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/intl/locale-names (checked 2026-10-17)
- https://learn.microsoft.com/globalization/locale/standard-locale-names (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/intl/locale-name-constants (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/intl/international-components-for-unicode--icu- (checked 2026-10-17)
- https://www.unicode.org/versions/latest/ and https://www.rfc-editor.org/info/bcp47 (**not reachable**) (checked 2026-10-17)

**Implication for Species (recommendation).**
- **All text is UTF-8 in files, data and the network; UTF-16 only at Win32 and DirectWrite boundaries.** Source code is UTF-8 (`/utf-8`, §2.2).
- **Language identifiers are BCP 47 tags** (`en-GB`, `de-DE`, `pt-BR`, `zh-Hans-CN`) everywhere: string-table file names, settings and the server API. The client picks its UI language from `GetUserPreferredUILanguages`, then falls back along the tag (drop region, then script) to `en-GB`.
- **Formatting and collation:** on the Windows client, use the in-box ICU C API (`icu.dll`, from Windows 1903, so present on every supported OS) for number, date and plural formatting and for sorting; on the Linux server, which formats almost nothing for players, use the distribution's ICU or none. Plural and gender rules come from CLDR via ICU; string tables use ICU MessageFormat syntax.
- Pin "Unicode 17 / CLDR 48" as the documented data level, and accept that the in-box ICU's exact version follows Windows servicing; re-check when the minimum OS moves.

## 11 Shipping: crash reporting, packaging, code signing

### 11.1 Windows Error Reporting and crash-reporting options

**Current fact (checked 2026-10-17).**
- **WER** reports crashes, hangs and kernel faults by default "without requiring changes to your application"; "Because Windows automatically reports unhandled exceptions, application should not handle fatal exceptions." A process is considered unresponsive if it does not handle window messages for five seconds while the user interacts.
- **WER APIs:** `WerRegisterFile`, `WerRegisterMemoryBlock`, `WerRegisterExcludedMemoryBlock`, `WerRegisterRuntimeExceptionModule` (custom runtime exception handler), `WerReportCreate`, `WerReportAddDump`, `WerReportAddFile`, `WerReportSetParameter`, `WerReportHang`.
- **Local dumps:** `HKLM\SOFTWARE\Microsoft\Windows\Windows Error Reporting\LocalDumps[\<app>.exe]` with `DumpFolder` (default `%LOCALAPPDATA%\CrashDumps`), `DumpCount` (default 10), `DumpType` (0 custom, 1 mini (default), 2 full) and `CustomDumpFlags`. Not enabled by default; needs administrator rights; HKCU is not supported; "Applications that do their own custom crash reporting are not supported by this feature."
- **Getting WER data back:** for unpackaged Win32 apps, the free **Windows Desktop Application Program** in Partner Center shows Health reports (crashes, hangs, stack traces, CAB files) for every executable signed with a certificate the publisher registers by signing and uploading a Microsoft-provided file. Data appears once an application is installed on at least 100 devices; symbols can be uploaded; an analytics REST API returns failure hits for the last 30 days. For MSIX apps through the Store, crash data appears in the Partner Center Health report (crash rate and hang rate from devices that opted in to optional diagnostic data).
- **Self-hosted alternative:** `MiniDumpWriteDump` (DbgHelp) writes a minidump from inside the process. An open-source option: the Sentry Native SDK (GitHub `getsentry/sentry-native`, latest **0.17.4**, marked Latest), whose release notes name a Crashpad backend on Windows and Linux.

**Sources.**
- https://learn.microsoft.com/windows/win32/wer/using-wer (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/api/_wer/ (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/wer/collecting-user-mode-dumps and https://learn.microsoft.com/windows/win32/wer/wer-settings (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/appxpkg/windows-desktop-application-program (checked 2026-10-17)
- https://learn.microsoft.com/windows/uwp/monetize/get-desktop-application-error-reporting-data (checked 2026-10-17)
- https://learn.microsoft.com/windows/apps/publish/analyze-app-performance/health-report (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/dxtecharts/crash-dump-analysis (checked 2026-10-17)
- https://github.com/getsentry/sentry-native/releases (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Baseline: let WER handle crashes, and get the data through the Windows Desktop Application Program** (free; requires the code signing of §11.3). Do not install a top-level unhandled-exception filter that swallows crashes. Use `WerRegisterFile` for the current log file and the last settings file, and `WerRegisterMemoryBlock` for a small "black box" ring buffer (last N frame and tick markers, current world region, build ID). `WerRegisterExcludedMemoryBlock` excludes anything that may hold personal data (chat text, account tokens).
- **Every build publishes PDBs to a private symbol store** keyed by build ID, and uploads symbols to Partner Center for release builds. Executables carry accurate `VERSIONINFO` (FileDescription, FileVersion, ProductName, ProductVersion), which the program uses to group executables.
- **Development and QA:** a CI-provided `.reg` file enables `LocalDumps` for `species.exe` (custom dump with data segments and thread data) on test machines.
- **The Linux dedicated server cannot use WER**; a cross-platform minidump path (Crashpad, for example through the Sentry Native SDK, or a self-hosted collector) is a design-phase decision together with server operations, because where reports go is an online-service question.
- The D3D12 device-removed path writes DRED data (from the Agility SDK) into the same report, so GPU hangs are diagnosable.

### 11.2 MSIX and store packaging

**Current fact (checked 2026-10-17).**
- **Microsoft Store, MSIX:** Microsoft re-signs the package after certification ("no CA-trusted code signing certificate is needed"); auto-update is built in.
- **Microsoft Store, MSI/EXE:** allowed since June 2021; the Store "does not re-sign MSI or EXE installers"; the installer and its PE files must be signed with a certificate chaining to a CA in the Microsoft Trusted Root Program (self-signed not accepted); the installer must be offline, must stay unchanged at the submitted URL, and must install only the intended product; updates are the app's own job.
- **MSIX outside the Store:** the publisher signs it; the certificate subject must equal the `Publisher` in the manifest's `Identity` element; updates through an `.appinstaller` file.
- A 2021 Agility SDK guide states "Agility SDKs do not pass WACK at the moment" (the Windows App Certification Kit). No later statement was found in this check; **current status not verified.**

**Sources.**
- https://learn.microsoft.com/windows/apps/publish/get-started#frequently-asked-questions (checked 2026-10-17)
- https://learn.microsoft.com/windows/apps/package-and-deploy/choose-distribution-path (checked 2026-10-17)
- https://learn.microsoft.com/windows/apps/publish/publish-your-app/msix/app-package-requirements (checked 2026-10-17)
- https://learn.microsoft.com/windows/win32/appxpkg/how-to-sign-a-package-using-signtool (checked 2026-10-17)
- https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/ ("Known issues", 2021) (checked 2026-10-17)

**Implication for Species (recommendation).**
- **The build produces one relocatable install directory** (`species.exe`, `D3D12\`, data packages, `GameInputRedist.msi` as a prerequisite) that runs without registry writes and writes all user data under `%LOCALAPPDATA%\Species` and saved worlds under the user's documents or a configured path, never next to the executable. Every distribution format is a thin wrapper around that directory.
- **Packaging targets are a design-phase choice** (which storefronts, which is out of scope here). The engine stays compatible with MSIX (read-only install directory, no writes beside the executable, no services, no drivers) so that the Microsoft Store MSIX path remains open; the GameInput redistributable and the Agility SDK need checking against MSIX certification before that path is chosen (WACK status of the Agility SDK not verified).
- The dedicated server ships as a plain archive (Windows and Linux) and, for Linux, a container image; packaging for community hosts belongs to the design phase.

### 11.3 Code signing

**Current fact (checked 2026-10-17).**
- Microsoft's comparison of signing options:
  - **Store MSIX:** free; the Store signs.
  - **Store MSI/EXE:** a certificate chaining to a Trusted Root Program CA is required.
  - **Azure Artifact Signing (formerly Trusted Signing):** about USD 9.99 a month; organisations in the USA, Canada, the EU and the UK (individuals in the USA and Canada); organisations need a verifiable tax history of three years or more; certificates issued daily, valid about 3 days; reputation tied to the verified publisher identity; CI integration; SignTool needs the Artifact Signing Client Tools plug-in. Microsoft calls it the recommended option for non-Store distribution.
  - **OV certificate:** USD 150–300 a year on one page, 300–500 on another.
  - **EV certificate:** "Same as OV since 2024 — no longer instant bypass" of SmartScreen.
  - **Self-signed:** development only.
- Every newly signed app shows a SmartScreen warning until it builds reputation; unsigned or self-signed apps get a strong block.
- Sign with SHA-256 and an RFC 3161 timestamp (`SignTool sign /fd SHA256 /f <certificate> /tr <timestamp URL> /td SHA256 <file>`).

**Sources.**
- https://learn.microsoft.com/windows/apps/package-and-deploy/code-signing-options (checked 2026-10-17)
- https://learn.microsoft.com/windows/apps/package-and-deploy/publish-first-app#step-2-set-up-code-signing (checked 2026-10-17)
- https://learn.microsoft.com/windows/msix/package/signing-package-overview (checked 2026-10-17)
- https://learn.microsoft.com/windows/msix/package/sign-msix-package-guide (checked 2026-10-17)

**Implication for Species (recommendation).**
- **Sign every shipped PE file** (`species.exe`, the dedicated server's `.exe`, every first-party DLL) and the installer, with SHA-256 and an RFC 3161 timestamp, in CI on release builds only. Signing also enrols the binaries in the WER analytics of §11.1.
- **Preferred signing service: Azure Artifact Signing**, if the publishing entity is eligible (region and three-year tax history); otherwise an OV certificate held in a hardware-backed or cloud key store. EV certificates bring no SmartScreen advantage.
- Signing keys never live on developer machines or in the repository; the CI signing step is the only path.
- Linux server artefacts are published with checksums and a detached signature; the scheme is a design-phase choice.

## 12 Summary

All facts checked on 2026-10-17. Confidence: **high** means a primary source was read directly and the recommendation follows from it; **medium** means the fact is primary but the recommendation is a judgement, or one part of the fact came from a secondary or summarised source; **low** means the item could not be verified from a primary source.

| # | Item | Recommended baseline | Confidence |
|---|---|---|---|
| 1.1 | OS (client) | Windows 11 x64, **minimum 24H2, build 10.0.26100**; primary test targets 25H2 (26200) and 26H2 (26300) | high |
| 1.2 | Windows 10 | Not supported (out of support since 2025-10-14; consumer ESU ended 2026-10-13) | high |
| 2.1 | Visual Studio / MSVC | Visual Studio 2026 (18.11 current); **MSVC Build Tools 14.51** pinned, move to 14.52 when it becomes default | high |
| 2.2 | Language | **C++23**: `/std:c++23preview` on 14.51, `/std:c++23` from 14.52; no `/std:c++latest`; no C++26 features | high |
| 2.2 | `import std` | Supported by MSVC (since VS 2022 17.5); allowed behind a build option until CMake 4.5 makes it non-experimental | medium |
| 2.3 | Linux server compilers | **Clang 23 (LLVM 23.1.3) + libc++** primary, **GCC 16 (16.2.0)** secondary; shared code uses the intersection with MSVC | medium |
| 3.1 | Build | **CMake ≥ 4.4** (4.4.4 current), presets schema 10, **Ninja Multi-Config** | high |
| 3.2 | Dependencies | **vcpkg manifest mode**, pinned `builtin-baseline`, overlay triplets per toolset | high |
| 4.1 | Agility SDK | **1.619.6 (D3D12SDKVersion 619)**, `D3D12SDKPath .\D3D12\` | high |
| 4.2 | Feature level | **12_1 minimum** + SM 6.6 + Resource Binding Tier 3 + WaveOps; 12_2 not required (its hardware reach not verified) | medium |
| 4.3 | Shader Model | **Minimum 6.6** (bindless dynamic resources); current retail 6.9, 6.10 preview | high |
| 4.4 | Enhanced Barriers | Frame graph in Enhanced-Barrier terms; use `Barrier()` when `EnhancedBarriersSupported`, legacy fallback otherwise | high |
| 4.5 | Shader compiler | **DXC v1.9.2609**, offline, `-HV 2021`, 202x-clean | high |
| 4.6 | Swap chain / VRR / HDR | `FLIP_DISCARD`, waitable, `ALLOW_TEARING` when supported, no exclusive full screen; HDR optional via scRGB FP16 | high |
| 4.7 | Profiling | **PIX 2603.25**; **WinPixEventRuntime 1.0.240308001** (MIT), markers compiled out of retail | high |
| 5.1 | DirectStorage | **Not adopted in v1**; package format chunked (≤ 256 KiB, 64 KiB-aligned) so 1.3.0 (stable) or 1.4 (preview now) can be added later | medium |
| 5.2 | Texture tools | **DirectXTex `may2026`** in the cooker; texconv for inspection; CI cooks BC7 with the software codec | high |
| 5.3 | Block compression | BC7 sRGB colour, BC5 normals, BC4 single-channel; uncompressed for exact data and glyphs | high |
| 6 | Input | **GameInput** for gamepads (pin 3.5.283 / `GameInput::v3` now, 4.0.x after its first point release) + redistributable in the installer; Win32 messages for mouse, keyboard, text and IME | medium |
| 7 | Audio API | **In-box XAudio 2.9 + X3DAudio**; no redistributable | high |
| 8.1 | Mesh source format | **glTF 2.0** (`.glb`), no `NORMAL` for flat shading, `COLOR_0`; extensions `KHR_materials_unlit`, `KHR_materials_emissive_strength`, `KHR_mesh_quantization` only | high |
| 8.2 | Texture container | **DDS with the DX10 header** for every cooked texture | high |
| 8.3 | Audio formats | Sources 48 kHz 24-bit WAV; runtime 16-bit PCM 48 kHz for effects, streamed **Opus 1.5.2** for music; MS-ADPCM only once its encoder's distribution is confirmed; no xWMA | medium |
| 9 | Glyph rasterisation | DirectWrite layout → custom renderer → `IDWriteFactory3::CreateGlyphRunAnalysis` (grayscale) → `CreateAlphaTexture` → `R8_UNORM` atlas | medium |
| 10.1 | Accessibility | **XAG 3.2** as checklist; text scale, 7:1 high-contrast mode, remapping, photosensitivity check | high |
| 10.2 | Localisation | UTF-8 everywhere, **BCP 47** tags, in-box ICU (`icu.dll`) on the client; data level Unicode 17 / CLDR 48 | medium |
| 11.1 | Crash reporting | **WER** + Windows Desktop Application Program + symbol store; Linux server crash path is a design-phase decision | high |
| 11.2 | Packaging | One relocatable install directory, MSIX-compatible; storefront choice is a design-phase decision | medium |
| 11.3 | Code signing | Sign every PE and installer, SHA-256 + RFC 3161 timestamp, in CI; **Azure Artifact Signing** if eligible, else OV | high |

### 12.1 Items that could not be verified

- **GCC per-feature C++23 core-language status** and the full **GCC 16.1 announcement**: gcc.gnu.org and its mirrors were unreachable; facts come from search-result summaries and from the GCC GitHub mirror (tags and libstdc++ status file).
- **cppreference compiler-support pages**: unreachable; replaced by the vendors' own status pages (MSVC Learn and STL changelog, Clang `cxx_status`, libc++ and libstdc++ status files).
- **ISO publication status of C++26**: isocpp.org and iso.org were not reached; only secondary reports were seen.
- **Feature level 12_2 hardware reach** (market share): no primary source.
- **Windows 10 consumer ESU page** (support.microsoft.com): unreachable; the end date comes from Microsoft Q&A answers quoting it.
- **Unicode version** (unicode.org unreachable; taken from ICU 78 release notes) and **BCP 47 RFC text** (rfc-editor.org unreachable; Microsoft's documentation of BCP 47 used).
- **Current distribution of the xWMA and AdpcmEncode encoders.**
- **Agility SDK and WACK/MSIX certification**: only a 2021 statement was found.
- **GameInput redistributable licence terms**: linked but not read.
- **cmake.org and registry.khronos.org**: not reached; the same documents were read from the Kitware and Khronos GitHub repositories.
