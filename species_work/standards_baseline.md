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

