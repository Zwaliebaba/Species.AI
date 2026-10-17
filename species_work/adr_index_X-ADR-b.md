# ADR index rows from X-ADR-b (ADR-050 to ADR-099)

Rows for `design_next/ADR/README.md`; links are relative to `design_next/ADR/`.

| Number | Title | Status | Date |
|---|---|---|---|
| [ADR-050](ADR-050-client-platform-and-minimum-os.md) | Client platform: Windows 11 x64, minimum version 24H2 (build 26100) | Accepted | 2026-10-17 |
| [ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md) | Windows toolchain: pinned MSVC Build Tools, Windows SDK and compiler policy | Accepted | 2026-10-17 |
| [ADR-052](ADR-052-cpp23-language-standard.md) | Language standard: C++23, with a portable subset for shared and server code | Accepted | 2026-10-17 |
| [ADR-053](ADR-053-modules-and-import-std.md) | Modules: `import std` behind a build option; first-party code stays header-based | Proposed | 2026-10-17 |
| [ADR-054](ADR-054-build-system-msbuild-solution.md) | Build system: CMake presets with the Ninja Multi-Config generator | Proposed | 2026-10-17 |
| [ADR-055](ADR-055-dependencies-three-nuget-packages.md) | Third-party dependencies: vcpkg in manifest mode with pinned baseline and overlay triplets | Accepted | 2026-10-17 |
| [ADR-056](ADR-056-products-configurations-and-feature-flags.md) | Products, build flavours and feature flags: one preset per product and flavour, data-driven runtime flags | Accepted | 2026-10-17 |
| [ADR-057](ADR-057-version-identity.md) | Version identity: one generated build identity used by every consumer | Accepted | 2026-10-17 |
| [ADR-058](ADR-058-pack-files-and-mount-policy.md) | Pack files: own indexed, chunked and hashed format; explicit mount order; loose files only in development builds | Accepted | 2026-10-17 |
| [ADR-059](ADR-059-content-build-and-validators.md) | Content build: one declarative, incremental, validating content build tool | Accepted | 2026-10-17 |
| [ADR-060](ADR-060-content-identity-by-hash.md) | Content identity: SHA-256 over cooked content, produced by the content build | Accepted | 2026-10-17 |
| [ADR-061](ADR-061-data-schemas-and-versioning.md) | Data schemas: one schema per record type, text for authoring, versioned binary for runtime | Accepted | 2026-10-17 |
| [ADR-062](ADR-062-authoring-syntax-and-schema-language.md) | Authoring syntax and schema language: JSON validated by JSON Schema, with a Species code generator | Proposed | 2026-10-17 |
| [ADR-063](ADR-063-localisation.md) | Localisation: UTF-8 string tables keyed by string id, BCP 47 language tags, ICU formatting and per-key fallback | Accepted | 2026-10-17 |
| [ADR-064](ADR-064-accessibility-baseline-xag.md) | Accessibility baseline: Xbox Accessibility Guidelines 3.2 as the checklist, with engine features built in from the start | Proposed | 2026-10-17 |
| [ADR-065](ADR-065-crash-reporting.md) | Crash reporting: Windows Error Reporting for the client, with registered context, a symbol store and no upload of credentials | Proposed | 2026-10-17 |
| [ADR-066](ADR-066-telemetry-with-consent.md) | Telemetry: opt-in performance telemetry over HTTPS, with a fixed data set and no secrets in the client | Accepted | 2026-10-17 |
| [ADR-067](ADR-067-logging.md) | Logging: asynchronous, buffered, categorised logging with TraceLogging (ETW) and rotating file sinks | Accepted | 2026-10-17 |
| [ADR-068](ADR-068-profiling-and-gpu-timing.md) | Profiling: PIX markers through one Species macro layer, GPU timestamp queries and an in-game overlay | Accepted | 2026-10-17 |
| [ADR-069](ADR-069-test-framework.md) | Test framework: GoogleTest through CTest, plus property, golden and fuzz tests | Proposed | 2026-10-17 |
| [ADR-070](ADR-070-continuous-integration.md) | Continuous integration: preset-driven pipelines with content validation, analysis builds and a headless bot soak | Accepted | 2026-10-17 |
| [ADR-071](ADR-071-install-layout-runtime-files-and-user-data.md) | Install layout: one relocatable, read-only install directory with the Agility SDK beside the executable; user data in known folders | Accepted | 2026-10-17 |
| [ADR-072](ADR-072-distribution-channels-and-package-formats.md) | Distribution channels and package formats: thin wrappers around the install directory; storefronts chosen by the design phase | Proposed | 2026-10-17 |
| [ADR-073](ADR-073-code-signing.md) | Code signing: every distributed PE file and package signed in CI with SHA-256 and an RFC 3161 timestamp | Proposed | 2026-10-17 |
| [ADR-074](ADR-074-platform-clock-with-injectable-time.md) | Platform clock: one process-relative monotonic clock behind an injectable time source | Accepted | 2026-10-17 |
| [ADR-075](ADR-075-application-window-and-dpi-awareness.md) | Application window: a platform-owned Win32 window, per-monitor DPI awareness v2, windowed and borderless full-screen modes | Accepted | 2026-10-17 |
| [ADR-076](ADR-076-screenshot-encoding.md) | Screenshot encoding: the in-box Windows Imaging Component PNG encoder; video capture by external tools | Proposed | 2026-10-17 |
