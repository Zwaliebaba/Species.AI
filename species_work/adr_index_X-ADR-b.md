# ADR index rows from X-ADR-b (ADR-0050 to ADR-0099)

Rows for `design_next/adr/README.md`; links are relative to `design_next/adr/`.

| Number | Title | Status | Date |
|---|---|---|---|
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
