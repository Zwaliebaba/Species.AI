# X-ADR-b report — toolchain, content-pipeline and operations ADRs

Run date: 2026-10-17. Package: `species_work/packages/X-ADR-b.json` (ADR-0050 to ADR-0099).

Nothing was published, shared or uploaded. No Artifact, Gamma, Drive, Docs or GitHub write tool was used. No git commands were run.

## 1 Files written

- `design_next/adr/0050-client-platform-and-minimum-os.md` to `design_next/adr/0076-screenshot-encoding.md`: 27 MADR 4 records (about 130 KB in total; the largest is 6.5 KB).
- `species_work/adr_index_X-ADR-b.md`: 27 index rows in the README's format (`| [ADR-NNNN](file) | Title | Status | Date |`), for the coordinator to merge at the `<!-- X-ADR-b rows -->` marker.
- `species_work/scripts/X-ADR-b_check.py`: checks front matter, the order of the MADR sections, the "Chosen option" line, the default statement in Proposed records, relative links and cited ADR numbers. It also writes the index rows.
- `species_work/scripts/X-ADR-b_fixrefs.py`: rewrites citations of the baseline's sections into citations of the README's "Technology baseline" item numbers, matching what X-ADR-a does.
- This report.

I did not edit `design_next/adr/README.md`.

## 2 Status rule applied

- **Accepted:** 40 (or an owner decision in the vision brief) settles the core choice. Version pins from the verified baseline count as parameters, not as open choices.
- **Proposed:** 40 offers alternatives ("A or B") or says nothing on the core choice. Every Proposed record contains the sentence "This is the default the team proceeds with".
- No record carries `owner-decision: blocking`. Every one has a defensible default.

## 3 ADRs and status (18 Accepted, 9 Proposed)

| ADR | Topic | Status | Basis |
|---|---|---|---|
| 0050 | Client platform: Windows 11 x64, minimum 24H2 (build 26100) | Accepted | 40 T28 (Windows x64); baseline 1.1 and 1.2 |
| 0051 | Windows toolchain: MSVC 14.51 pinned (then 14.52), Windows SDK, compiler and linker policy, clang-cl for analysis only | Accepted | 40 T28 |
| 0052 | C++23, with a portable subset for shared and server code | Accepted | 40 T28 |
| 0053 | Modules: `import std` behind `SPECIES_IMPORT_STD` (OFF until CMake 4.5) | Proposed | 40 silent |
| 0054 | Build system: CMake 4.4+ presets (schema 10), Ninja Multi-Config | Proposed | 40 T28 and R26 say "CMake presets or MSBuild" |
| 0055 | vcpkg manifest mode, pinned baseline, overlay triplet `x64-windows-species` | Accepted | 40 T28 |
| 0056 | Products, build flavours (Debug, Development, Profile, Shipping) and data-driven feature flags; configure refuses zero or two products | Accepted | 40 R26; 31 §15 |
| 0057 | One generated build identity (semantic version, build number, revision, content hash, product, flavour) | Accepted | 40 T28, R26 |
| 0058 | Pack files: own indexed, chunked, SHA-256-hashed format (Zstandard by default, chunks of at most 256 KiB, 64 KiB-aligned); explicit mount order; loose files only in Debug and Development builds; no DirectStorage in v1 | Accepted | 40 T17, R21, G72, C7 |
| 0059 | Content build tool: declarative edition manifests, dependency graph, validators, reproducible output (one Mermaid diagram) | Accepted | 40 T29 |
| 0060 | Content identity: SHA-256 over cooked entries; map identity = the hash of the cooked map; a simulation content identity | Accepted | 40 T17, T29, G67, P10 |
| 0061 | Data schemas: one schema per record type, text authoring, versioned binary, logical numeric types, map header, one defaults table | Accepted | 40 T25, R24, R10, F52 |
| 0062 | Authoring syntax: JSON plus JSON Schema, with a Species code generator | Proposed | 40 T25 lists JSON, TOML, YAML or an own IDL |
| 0063 | Localisation: UTF-8, BCP 47, ICU MessageFormat via the in-box ICU, per-key fallback, a release rule per language | Accepted | 40 R28, F51 |
| 0064 | Accessibility: XAG 3.2 as the checklist; engine features from the start | Proposed | 40 silent |
| 0065 | Crash reporting: WER, registered context, symbol store, Windows Desktop Application Program | Proposed | 40 R29 and T31 say "WER or Crashpad" |
| 0066 | Telemetry: opt-in, HTTPS, a fixed data set, no secrets in the client | Accepted | 40 G64, T35 |
| 0067 | Logging: asynchronous, buffered, categories and levels; TraceLogging (ETW) and file sinks | Accepted | 40 T37 |
| 0068 | Profiling: PIX 2603.25, WinPixEventRuntime 1.0.240308001, GPU timestamps, overlay | Accepted | 40 T36, G62 |
| 0069 | Test framework: GoogleTest through CTest; property, golden, fuzz and performance tests | Proposed | 40 silent on the framework |
| 0070 | Continuous integration: per-change and nightly tiers, headless bot soak | Accepted | 40 R27, G63 |
| 0071 | Install layout: relocatable read-only directory, Agility SDK in `D3D12\`, prerequisites, user data in known folders | Accepted | 40 T30 |
| 0072 | Distribution: a signed archive now; channel wrappers once the design phase picks channels | Proposed | 40 T30 says "Steam depots and/or MSIX" |
| 0073 | Code signing: SHA-256 with an RFC 3161 timestamp in CI; Azure Artifact Signing if eligible, otherwise OV | Proposed | 40 silent |
| 0074 | Platform clock: process-relative monotonic clock behind an injectable time source | Accepted | 40 T32, G71 |
| 0075 | Application window: platform-owned Win32 window, per-monitor DPI v2, windowed and borderless | Accepted | 40 T18 |
| 0076 | Screenshot encoding: WIC PNG; video by external tools | Proposed | 40 T38 says "WIC or libpng/stb" |

Counts: 27 ADRs, 1 Mermaid diagram (ADR-0059, with a caption), 0 requirement IDs (ADRs carry none), 0 tuning keys.

## 4 Coverage of the package's input rows

- **Covered here:** 08 §9 items 1, 2, 7 and 10–11 (0059–0061). 29 §4.6 and 31 §10.3 (0060). 29 §10.1 and 31 §8.8 (0058). 29 §12 items 1–6 and 9 (0058, 0061; the network-container part of item 8 is deferred). 31 §3.3 (window part: 0075). 31 §6.2 (0065). 31 §7.3 (0051, 0056). 31 §7.6 (0056). 31 §8.3 (0059). 31 §9.2 (0071, 0072). 31 §15, every row except floating point (0050–0076). 32 §8.6 (0066). 32 §9.1 (0068). 32 §10.1 (0067). 32 §14 items 4–9 (0065–0070). 40 D8 content part, C7, R10, R24, R26–R29, G61–G67 (diagnostics parts), G71, G72, T17, T25, T28–T32, T35–T38, the §3.3 node N10 and the N9 parts for pack files and schemas, P10, F51, F52. 41 R18–R21 (validators, separate collision assets, localisation fallback; IME via the input ADR).
- **Left to X-ADR-a** (engine-runtime topics in shared rows): 29 §8.1 and 31 §11.1 (mesh pipeline; X-ADR-a's README names ADR-0019 and ADR-0020); 40 D5, D9, R13, R18–R23, R7, G14, G46–G52, G55, T1–T9, T12–T16, T19, T20, T24, T34 and F50, F54 (renderer, audio, input, UI, textures, meshes). 31 §3.3 swap-chain part: X-ADR-a's ADR-0012, which already cites ADR-0075 for the window.
- **Not made into ADRs (deferred to the design phase):**
  - 32 §14 items 1–3: sequenced debug commands, sync hashing and deterministic replays depend on the networking and numeric models. ADR-0070 reserves a replay regression job, and ADR-0056 compiles the debug console out of profile and shipping builds.
  - The floating-point row of 31 §15. ADR-0051 deliberately fixes no `/fp` option for simulation code.
  - The exchange of map hashes in a lobby (08 §9 item 2, P10 exchange part).
  - The 29 §12 item 8 network container.
  - The toolchain for a Linux dedicated server (baseline 2.3). The brief says to keep the option open without deciding it: ADR-0051 and ADR-0052 keep a portable subset and a clang-cl portability build, and name no server compilers.

## 5 The 17 ADR requests in F3

| F3 request | Covered by |
|---|---|
| Platform and OS minimum | ADR-0050 |
| Language and compilers | ADR-0051, ADR-0052, ADR-0053. The Linux server compilers are not decided (see §4); the design phase decides them with hosting. |
| Build system and dependency management | ADR-0054, ADR-0055 |
| Graphics API baseline; synchronisation model; shader toolchain; presentation | X-ADR-a (ADR-0001, 0002, 0006, 0007, 0003, 0012). The window part is ADR-0075. |
| Asset I/O and package format | ADR-0058 |
| Texture pipeline; mesh interchange; input; audio; text rendering | X-ADR-a (README names ADR-0017, 0019, 0028, 0022 and 0025, 0034) |
| Accessibility baseline | ADR-0064 |
| Localisation | ADR-0063 |
| Diagnostics and crash reporting | ADR-0065, with ADR-0066, 0067 and 0068 |
| Packaging and code signing | ADR-0071, ADR-0072, ADR-0073 |

None of the 17 is uncovered. The only part left undecided is the Linux server toolchain, which is deferred on purpose.

## 6 Version re-checks and versions the baseline does not cover

- **Re-checked** (marked unverified in baseline §12.1). Microsoft Learn, https://learn.microsoft.com/windows/msix/package/packaging-uwp-apps#validate-your-app-package (checked 2026-10-17): "Windows App Certification Kit (WACK) is deprecated and no longer maintained" and "Official certification is performed automatically when you submit your package to Partner Center". It is cited in ADR-0072. Whether a package that contains the Agility SDK passes Store certification is still unverified, so ADR-0072 requires a test submission first.
- **Not re-checked:** the GCC C++23 table, C++26 ISO status and the 12_2 reach (none affects my decisions), the Windows 10 consumer ESU page (support.microsoft.com is not on Learn), and the Unicode and BCP 47 primary pages (not Microsoft or vendor documentation). The GameInput redistributable licence was not re-checked: ADR-0071's Confirmation requires reading it before the first external build.
- **Versions absent from the baseline, so no number appears in any ADR:**
  - the Windows SDK release (ADR-0051 says "at least the release for Windows 11 24H2");
  - the JSON Schema draft and the JSON parser and validator libraries (ADR-0062);
  - the GoogleTest version (ADR-0069);
  - the Zstandard library;
  - the fuzzing engine;
  - the Visual C++ redistributable;
  - TraceLogging and WIC, which are part of the Windows SDK.

  Request to the coordinator and F3: verify and add the Windows SDK release to the baseline. The components (C-BLD, C-DAT, C-DBG) pin the rest through vcpkg.

## 7 DEC requests (for X-DEC)

1. **PIX markers in shipping builds** (F3 DEC 5). Default: compiled out (ADR-0056, ADR-0068). Decide by: design phase.
2. **DirectStorage in v1** (F3 DEC 7). Default: no; the pack format stays DirectStorage-ready (ADR-0058).
3. **Distribution channels** (F3 DEC 9). Default: a signed archive plus the install directory; channel wrappers come later (ADR-0072). Decide by: design phase.
4. **Dedicated-server toolchain, crash path and artefact signing on non-Windows hosts** (F3 DEC 8). This belongs to the deferred "Hosting" topic in the README. ADR-0051, 0052, 0065 and 0073 keep it open.
5. **Screen narration of menus in v1** (XAG 106). Default: not in v1; every widget carries an accessible-name string id (ADR-0064). Decide by: design phase.
6. **Release rule for partially translated languages** (40 F51 "complete or drop"). Applied in ADR-0063: a language ships only with full key coverage, or with an explicit sign-off of the fallback keys. X-DEC should cite ADR-0063 in the F51 entry.
7. **One defaults table** (40 F52). Applied in ADR-0061 (settings defaults are compiled in, so they are available before any pack mounts). The settings file format stays DD-PERSIST.
8. **Modding through loose files.** Shipping builds have no loose-file mount (ADR-0058, 40 G72). Modding is a vision §7 later topic. The design phase needs a DEC on how mods would be packaged and identified.
9. **Developer console and debug commands in shipping builds** (32 §14 item 6, "optionally behind a launch flag"). Default: compiled out of profile and shipping builds (ADR-0056). Decide by: design phase, together with sequenced debug commands (G61, which depends on DD-NET).
10. **Code-signing route.** Not a DEC, but an owner fact. Is the publishing entity eligible for Azure Artifact Signing (region, three-year tax history)? ADR-0073 defaults to the OV fallback otherwise.

## 8 Cross-document requests

1. **README (X-ADR-a, coordinator) — user-data path.** "Technology baseline" area 11 says user data lives under `%LOCALAPPDATA%\Species`. ADR-0071 uses `%LOCALAPPDATA%\<STUDIO>\Species\`, following the rename ledger's profile folder `<STUDIO>\Species\`. Please align the README line.
2. **README "Items not verified" — WACK.** Update the Agility SDK and WACK item with the WACK deprecation finding of §6. Store certification of an Agility SDK package remains unverified.
3. **README summary "Decided in" column.** Replace "toolchain ADRs (0050–0099)" with:
   - 1.1 and 1.2 → 0050;
   - 2.1 → 0051;
   - 2.2 → 0052 and 0053;
   - 2.3 → the "Hosting" deferral (0051 and 0052 keep only a portable subset);
   - 3.1 → 0054;
   - 3.2 → 0055;
   - 4.7 → 0068;
   - 5.1 → 0058;
   - 10.1 → 0064;
   - 10.2 → 0063;
   - 11.1 → 0065;
   - 11.2 → 0071 and 0072;
   - 11.3 → 0073.

   Row 2.3 states server compilers as a baseline. The brief asks that this not be decided, so the coordinator may want to mark the row as a design-phase input.
4. **Links to future X-ADR-a records.** ADR-0063 (text rendering, IME) and ADR-0075 refer to the UI, text and input ADRs by topic, because ADR-0028 and ADR-0034 did not exist yet. At G2, links can be added once those files exist.
5. **Components to cite these ADRs:**
   - C-BLD: 0051–0057, 0059, 0071–0073;
   - C-PLT: 0050, 0071, 0074, 0075, 0076;
   - C-AST: 0058–0060;
   - C-DAT: 0061, 0062, 0056 (edition manifests);
   - C-DBG: 0065–0070;
   - C-UI: 0063, 0064;
   - C-INP: 0064 (remapping, hold and toggle).
6. **CONVENTIONS §9 "Allowed terms".** `Linux` is allowed in ADRs for dedicated-server hosting (ADR-0051 line 11, ADR-0052 line 11).

## 9 Names needed but not in the ledger (proposals)

- **Pack-file extension:** `.pack` (ADR-0058).
- **Product names:** `client`, `server`, `tools`. CMake variable `SPECIES_PRODUCT` (ADR-0056).
- **Build flavours:** `Debug`, `Development`, `Profile`, `Shipping`. Preset pattern `<os>-<compiler>-<flavour>` (ADR-0054, ADR-0056).
- **Build option** `SPECIES_IMPORT_STD` (ADR-0053). **Marker macro** `SPECIES_PROFILE_SCOPE` (ADR-0068).
- **vcpkg overlay triplet:** `x64-windows-species` (ADR-0055).
- **Schema annotation prefix:** `x-species-`; the author-comment field `notes` (ADR-0062).
- **Screenshot file name:** `species_<UTC date and time>.png` under `Pictures\Species\` (ADR-0076).
- **User-data folders:** `settings\`, `logs\` and `crashes\` under `%LOCALAPPDATA%\<STUDIO>\Species\` (ADR-0071).

Ledger names used: `TickClock`, `<STUDIO>`, the map-id pattern `map_<mode>_<players>p_<nn>`, and `species`/`Species`.

## 10 Gate results

- `scan_patterns.py` with `forbidden_patterns.tsv` over the 27 ADRs and the index file: **0 hits**.
- `scan_patterns.py` with `soft_patterns.tsv`: **2 hits**, both `Linux`, in a dedicated-server hosting context (ADR-0051 line 11, ADR-0052 line 11). They are allowed by the soft-pattern rule; I request the allowed-terms entry in §8 item 6. All earlier hits (`shipped`) were reworded.
- Truncation words: none. The one "and others" and the two ellipses were rewritten.
- `X-ADR-b_check.py`: 0 errors. All sections are present and in order, every relative link resolves, and every cited ADR exists (ADR-0001, 0002, 0004, 0008 and 0012 from X-ADR-a are cited because they exist).

## 11 Dropped as legacy (one line per group)

- **Old toolchain, configuration and product-macro specifics** (31 §7.3, §7.6): replaced by ADR-0051 and ADR-0056. The specific flags and macros are not reproduced.
- **Solid-archive format, its build switches, its archive names and its load order** (29 §10.1, 31 §8.3, §8.8): replaced by ADR-0058 and ADR-0059. Only the behaviours that 40 keeps survive (ordered mounts, last mount wins, development-only override).
- **The map checksum algorithm and the generated identity header** (29 §4.6, 31 §10.3): replaced by SHA-256 content identity (ADR-0060). No bit pattern needs preserving (40 §3.1).
- **Installer scripts, the auto-patcher and the uninstall lists** (31 §9.2): replaced by ADR-0071 and ADR-0072.
- **The crash-report tool, its log layout and its upload** (31 §6.2): replaced by ADR-0065. Never attaching credentials is kept as a rule.
- **Profiler and debug-log internals** (32 §9.1, §10.1): replaced by ADR-0067 and ADR-0068.
- **The mesh exporter and the text mesh format** (29 §8.1, 31 §11.1): left to X-ADR-a's mesh ADRs; nothing reproduced here.
