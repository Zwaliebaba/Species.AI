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
- The launcher checks `RtlGetVersion` (not the manifest-dependent `GetVersionEx`) for build at least 26100 and shows a clear message below it. Do not hard-fail on build alone where a capability query (for example, a D3D12 feature check) answers the real question.
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
- https://raw.githubusercontent.com/llvm/llvm-project/release/23.x/libcxx/docs/Status/Cxx23Papers.csv (checked 2026-10-17)
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
- **Build system: CMake, minimum 4.4** (`cmake_minimum_required(VERSION 4.4)`), presets schema **version 10** unless a version-11 or version-12 field is needed (version 10 keeps the presets readable by CMake 3.31 tooling such as older IDE integrations; raise it deliberately).
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

