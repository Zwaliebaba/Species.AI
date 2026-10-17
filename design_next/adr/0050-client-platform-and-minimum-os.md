---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Client platform: Windows 11 x64, minimum version 24H2 (build 26100)

## Context and Problem Statement

Species is a Windows client game built with C++23 and Direct3D 12 (vision brief §5). The engine needs one supported operating-system floor, so that the team tests against a known matrix, uses Direct3D 12 features through the Agility SDK, relies on in-box components (XAudio 2.9, ICU, DirectWrite, GameInput support) and tells players clearly when their system is too old. The dedicated server is a separate build target; its operating system is decided by the design phase together with hosting (vision O5, challenge 10).

Which operating system, version and processor architecture does the Species client support?

## Decision Drivers

* Every supported OS must still receive security updates from Microsoft for consumers.
* One servicing branch should cover as many supported versions as possible, to keep the test matrix small.
* The OS must support the Agility SDK, in-box XAudio 2.9 and in-box ICU (`icu.dll`).
* The decision must not constrain the dedicated-server operating system.

## Considered Options

* Windows 11 x64, minimum version 24H2 (OS build 10.0.26100)
* Windows 11 x64, minimum version 25H2 (OS build 10.0.26200)
* Windows 10 and Windows 11 x64
* Windows 11 on x64 and Arm64

## Decision Outcome

Chosen option: "Windows 11 x64, minimum version 24H2 (OS build 10.0.26100)", because every Windows 11 version still in consumer service (25H2, 26H2 and 26H1) has a build number of 26100 or higher, 24H2, 25H2 and 26H2 share one servicing branch, and the commercial and LTSC 2024 editions that remain in service are also build 26100.

* **Test matrix:** 25H2 (26200) and 26H2 (26300) are the primary targets; 24H2 LTSC (26100) is the floor; 26H1 (28000), which is built on a different Windows core, is smoke-tested when hardware is available.
* **Application manifest:** the executable declares Windows 10 and 11 in `<supportedOS>` so that version queries report the real version.
* **Start-up check:** Species has no launcher. The platform layer's start-up check reads the OS build number and, below 26100, shows a clear notice with the build found and the minimum, logs it and continues. Hard failures come only from capability checks (for example the Direct3D 12 checks of [ADR-0002](0002-minimum-gpu-capability.md)), following Microsoft's advice to test for features rather than versions.
* **Architecture:** x64 only. Arm64 is not a target of this baseline.

### Consequences

* Good, because one servicing branch and one build-number floor cover all supported consumer versions.
* Good, because the Agility SDK, XAudio 2.9, ICU and GameInput are present on every supported system, so the installer carries no runtime installers beyond those named in [ADR-0071](0071-install-layout-runtime-files-and-user-data.md).
* Bad, because players still on Windows 10 cannot run Species.
* Bad, because the floor moves: 25H2 Home and Pro leave service on 2027-10-12, so the table must be re-checked at every milestone.

### Confirmation

* CI runs the client test suite on 25H2 and 26H2 images and a smoke test on a 24H2 LTSC image.
* A unit test of the start-up check feeds build numbers 26099, 26100 and 28000 and asserts the message and pass results.
* The milestone checklist includes a re-check of the Windows release-information page.

## Pros and Cons of the Options

### Windows 11 x64, minimum 24H2 (build 26100)

* Good, because it includes every in-service consumer and LTSC 2024 build.
* Good, because one servicing branch (24H2, 25H2, 26H2) simplifies testing.
* Neutral, because 24H2 Home and Pro are out of service, so the floor exists only for LTSC and commercial editions.

### Windows 11 x64, minimum 25H2 (build 26200)

* Good, because every consumer edition at that floor is in service.
* Bad, because it excludes Enterprise LTSC 2024 (build 26100, serviced to 2029-10-09) for no technical gain.

### Windows 10 and Windows 11 x64

* Good, because it reaches more machines.
* Bad, because Windows 10 left support on 2025-10-14 and the consumer extended-security programme ended on 2026-10-13, so players could not keep it patched.
* Bad, because it doubles the test matrix.

### Windows 11 on x64 and Arm64

* Good, because it reaches Arm64 devices.
* Bad, because every dependency, the shader toolchain and the GPU test matrix would need a second architecture before the first is stable.

## More Information

* Facts and sources: technology baseline items 1.1 and 1.2 in the [ADR index](README.md#technology-baseline) (checked 2026-10-17): Windows 11 release information, the 24H2 status page and the Windows 10 life-cycle pages on learn.microsoft.com.
* Related: [ADR-0051](0051-windows-toolchain-and-compiler-policy.md) (toolchain and Windows SDK), [ADR-0075](0075-application-window-and-dpi-awareness.md) (application manifest settings).
* Component: platform layer (`PLT`).
* Re-check trigger: any Windows 11 version leaving consumer service.
* **Amendment 2026-10-17.** The below-minimum message is a notice of the platform layer's start-up check, which continues afterwards (PLT-040, `plt.os.minimum_build`); the install directory has no launcher ([ADR-0071](0071-install-layout-runtime-files-and-user-data.md)).
