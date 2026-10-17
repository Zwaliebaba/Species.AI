# C-BLD report

Package: `species_work/packages/C-BLD.json` · Target: `design_next/architecture/components/build_tooling_and_distribution.md` · Prefix: BLD (range 001–299)

## 1 Files written

| File | Size | Notes |
|---|---|---|
| `design_next/architecture/components/build_tooling_and_distribution.md` | 105,930 bytes (103 KB) | component template §6.4, headings 1 to 8; the tuning table is §4.13, inside §4, as in `terrain.md` |
| `species_work/reports/C-BLD.md` | this file | |
| `species_work/scripts/C-BLD_check.py` | helper | self-check (ids, keys, DECs, ADRs, cited requirement ids, links, truncation words, headings, size) |
| `species_work/scripts/C-BLD_renumber.py` | helper | numbers requirement definitions in document order and rewrites references |
| `species_work/scripts/C-BLD_relabel.py` | helper | exact, counted string replacements |

No split was needed (under 120 KB). Nothing was published; no git write commands were run.

## 2 Counts

- Requirements: **120** (BLD-001 to BLD-120, ascending in document order, no gaps; 113 MUST, 7 MUST NOT).
- Tuning keys owned: **28** (`bld.pin.*` 15, `bld.version.part_max`, `bld.channel.tag_max_chars`, `bld.cb.exit_*` 4, `bld.l10n.typo_max_edit_distance`, `bld.repair.exit_failed`, `bld.crash.*` 2, `bld.perf.*` 3). Keys cited from other documents: `ast.hash.display_hex_digits`, `ast.hash.digest_bytes`, `ref.meta.lang.base`, `dat.perf.full_build_s`, `plt.lifecycle.exit_code.normal`.
- Diagrams: **5** Mermaid (build overview flowchart, content-build stages, map identity, translation round trip sequence, CI tiers), each with a caption; all 5 parse with mermaid 11.4.1 (headless Chromium, the scratchpad checker).
- Enumerations: 27 tables (23 captioned, plus the failure-mode, acceptance-test, open-decision and tuning tables) with 268 rows; 36 acceptance tests (T1–T36); 13 content-build stages; 12 validator classes; 6 runtime feature flags; 7 configure-preset stems; 15 CI jobs; 5 packages.
- String ids introduced: `ui.repair.passed`, `ui.repair.failed` (new English text, table 4.10-B). Cited: `ui.error.content_missing`, `ui.error.content_damaged` (AST-102).

## 3 Checks

- `scan_patterns.py forbidden_patterns.tsv`: no hits.
- `scan_patterns.py soft_patterns.tsv`: no hits (two "source code/tree" phrasings and one "Linux" were reworded; the dedicated-server host OS is described as "the host operating system that most community hosts run", citing baseline item 2.3).
- `C-BLD_check.py`: OK. Every BLD id in range and unique; every tuning key unique in `design_next/` and cited by a requirement; every cited DEC (25) and ADR (28) exists; every `decs_to_cite` entry is cited and the register statuses match; every cited R-, AST-, DAT-, UI-, RND- id exists; links resolve (to files on disk or planned package targets: `platform_layer.md`, `diagnostics_test_and_performance.md`); no truncation words.
- `accept_package.py`: PASS.
- **Concurrent renumbering.** C-DAT renumbered `data_schemas/README.md` during this run (for example the old DAT-054, DAT-057 and DAT-059 are now DAT-048, DAT-051 and DAT-053). Every DAT citation of this document was remapped by content to the numbering on disk at 19:05 (DAT-019 to DAT-053). Please re-run `C-BLD_check.py` after C-DAT finishes; it reports any DAT id that no longer exists.
- **Other documents already cite BLD ids**: `data_schemas/import_and_build_records.md` (BLD-015, 016, 018, 049, 050, 051, 053, 078) and `diagnostics_test_and_performance.md` (BLD-021, 057, 117, 120). The numbering is final; do not renumber this document.

## 4 Open DECs cited (all from `decs_to_cite`)

| DEC | Cited by |
|---|---|
| DEC-204 | BLD-051, BLD-062, BLD-073, BLD-077; table 4.6-A |
| DEC-220 | BLD-005, BLD-048; table 4.9-A |
| DEC-222 | BLD-041 |
| DEC-229 | BLD-094 |
| DEC-240 | BLD-040 |
| DEC-248 | BLD-053; table 3.2-C |
| DEC-802 | BLD-057 |

Resolved and Dropped entries of the package are cited as fixed rules: DEC-015, DEC-561, DEC-563, DEC-800, DEC-801, DEC-804 (Resolved); DEC-332, DEC-340 (Dropped, §8). Also cited: DEC-041, DEC-099 (Resolved); DEC-045, DEC-047, DEC-228, DEC-243–245, DEC-249, DEC-317, DEC-329 (Open).

## 5 New DEC requests

None. Every open question went to an existing DEC or to the design-phase inputs of §8.

## 6 ADR conflicts and tensions (for X-ADR and the coordinator)

1. **ADR-0054 preset naming vs ADR-0056 one product per configure.** ADR-0054 names presets `<os>-<compiler>-<flavour>`; ADR-0056 requires exactly one product per configure. BLD-006 appends the product: `<os>-<compiler>-<flavour>-<product>` (for example `windows-msvc-development-client`). ADR-0070's "`windows-msvc-development` builds of every product" then reads as that preset family. Request: amend ADR-0054's naming line.
2. **ADR README "Technology baseline" §3** lists the preset set `windows-msvc-{debug,release,profile}`, `windows-clangcl-release` and `linux-*` presets, which differs from ADR-0054 (configurations `Debug`, `Development`, `Profile`, `Shipping`; non-Windows presets only when the design phase decides). BLD follows ADR-0054. Request: align the README line.
3. **ADR-0063 release rule vs DEC-099 and R-META-463.** ADR-0063 offers a language with full coverage "or when the release sign-off accepts the listed fallback keys explicitly"; DEC-099 (Resolved) and R-META-463 offer only complete languages. BLD-081 states the stricter rule and does not specify the sign-off exception. Request: X-ADR to drop the exception from ADR-0063, or X-DEC to amend DEC-099; then BLD-081 follows.
4. **ADR-0057 generation.** ADR-0057 says CMake generates "one header and one data file per build from these fields". BLD-020 puts every field except the content manifest hash in the generated header (so a content change never recompiles code) and has the packaging step of the build definition write `build_identity.json` with every field; programs compose the full identity at start-up from the header and the mounted content manifest. Request: confirm in ADR-0057, or reword it to allow this split.
5. **ADR-0071 install contents.** BLD adds `build_identity.json` and `channel_tag.txt` to the install directory (table 3.5-A); ADR-0071 lists no such files but does not exclude them. Request: add them to ADR-0071's list.
6. **Windows SDK release** is still absent from the technology baseline (X-ADR-b noted it). `bld.pin.windows_sdk` states "the release for Windows 11 version 24H2, or later" and says the exact release is not yet in the baseline. Request: F3 or X-ADR to verify and add it.

## 7 Missing assets

None. The document cites no asset id.

## 8 Cross-document requests

1. **C-DAT (`data_schemas/import_and_build_records.md`).** Already aligned during this run: DAT-358 and table 3.5-A give the field types of exactly the fields of BLD table 3.2-A, and DAT-359 writes the flag record (BLD-016). This answers C-AST request 6: C-BLD owns the meaning of the release flag (BLD-054, BLD-055); C-DAT owns its type. One addition for DAT table 3.5-A's `release` row: an edition with the flag set also lists only complete languages, and is packaged only from an unmodified build (BLD-054).
2. **C-AST.**
   - (a) Answered: the edition release flag (§3.2, BLD-054); the CI jobs of AST-066 (BLD-067, `ci-nightly-reproducibility`) and AST-105 (BLD-046, `ci-change-binaries`); installing the mount and content manifests in `packs\` (BLD-022, table 3.5-A); the repair function (BLD-103, `--verify-content`).
   - (b) AST-057 says the `locale` pack holds "the fonts and string tables of the edition's languages". BLD-081 withholds a listed language that is not complete. Please read "the edition's offered languages" there.
   - (c) Table 1-A of AST names `build_tooling_and_distribution.md` correctly; no change.
3. **C-PLT (`platform_layer.md`).**
   - Aligned already: `--write-build-identity` is PLT-023 (BLD-063 now cites it and `plt.lifecycle.exit_code.normal`); the user-data root and folders are PLT-016 and PLT-017 (BLD-025); the language-test report goes to `diagnostics\` (PLT table 3.2-A row 16, BLD-082).
   - Please add `--verify-content` (BLD-103) to table 3.3-A, owner build tooling, every flavour; PLT-022 otherwise logs and ignores it. Its result is shown through `ShowPlatformMessage` with `ui.repair.passed` or `ui.repair.failed` (table 4.10-B), and it ends with `plt.lifecycle.exit_code.normal` or `bld.repair.exit_failed`.
   - Table 3.2-A row 14 says the distributor tag is "not specified (DEC-249)". Species has the channel tag of BLD-021 (`channel_tag.txt` in the install directory, read at start-up, written in the log header only, which DBG-044 already adopts); reporting it to online services stays DEC-249. Please name BLD-021 in that row and read the file at start-up.
   - At start-up, compose the full identity from `species_build_identity.h` and the mounted content manifest, compare it with `build_identity.json`, and log a mismatch as a modified installation (BLD-020).
   - The test-machine `LocalDumps` registry file writes into the default user-data `crashes\` folder (BLD-113); a `--user-data` root (PLT-024) does not move it.
   - The crash handlers, WER registrations and thread naming of ADR-0065 are yours; BLD-110 cites `platform_layer.md` for them.
4. **C-DBG (`diagnostics_test_and_performance.md`).** Aligned already: DBG-044 puts the channel tag in the log header and keeps it out of the black box (BLD-021, BLD-120); DBG cites BLD-057 and BLD-117. Remaining:
   - The `soak` server edition (table 3.2-D) and the nightly job `ci-nightly-soak` (table 4.9-A) host your soak harness (DEC-804).
   - Please define the "reference class" of developer machine on which `bld.perf.incremental_one_source_s` is measured (table 5-A).
   - `ci-nightly-performance` runs your performance tests and the build budgets of table 5-A.
5. **G09b (`options_localisation_and_hooks.md`) and C-UI-a (`layout_rendering_input_and_localisation.md`).**
   - R-META-473's language test writes its three lists as `language_report.json` in the user-data `diagnostics\` folder, replaced at each language load, in `Debug` and `Development` builds only, never in the working directory (BLD-082, PLT-017). The writer is the string-table code; please cite BLD-082.
   - The options screen's build details show the full identity text of BLD-061 (ADR-0057). Please cite BLD-061 where the build details are listed.
6. **G09a (`screen_flow_and_match_setup.md`) and C-SIM-a (`02_match_lifecycle_and_state_reset.md`).** BLD-053: a match-start command for a map outside the edition's permitted set starts no match and logs the map id. In the reference edition every map is permitted, so no reference rule changes; please cite BLD-053 where match start validates its map.
7. **G08a (`rules_and_conventions.md`).** R-MAP-010 states that the content build computes a map's hash. The map-identity listing (BLD-074) and the rule that a source whose name is not a canonical map id is a build error (BLD-072) may be cited there.
8. **X-AST pass 2.** Add the string ids `ui.repair.passed` and `ui.repair.failed` to the string-table rows.
9. **X-GLO.** Please add these terms, owned by this document: product, flavour, edition, edition manifest, edition marked for release, reference feature set, permitted maps, build identity, channel tag, map-identity listing, localisation report, translator pack, repair check, symbol store, channel wrapper.
10. **Coordinator (CONVENTIONS §9).** No allowed-term entry is needed now. If the design phase writes about a server build for that host operating system in a component, the "Linux" entry of §9 will need "component §8 design-phase inputs about dedicated-server hosting".

## 9 Names not in the ledger (proposals)

These are Species identifiers that this document introduces; none repeats an identifier of the reference. Please add them to the ledger or CONVENTIONS §8 if the coordinator wants them registered.

| Proposed name | Kind | Where |
|---|---|---|
| `species-contentbuild` (`species-contentbuild.exe`) | content-build program | BLD-008 |
| `species_server.exe` | dedicated-server executable | table 3.1-A |
| `species_version.json` | version file at the repository root | table 3.3-A |
| `species_build_identity.h`, `build_identity.json` | generated identity header and data file | BLD-020 |
| `channel_tag.txt` | channel tag file | BLD-021 |
| `language_report.json` | in-game language-test report | BLD-082 |
| `validation_report.json`, `placeholder_report.json`, `localisation_report.json`, `map_identities.json` | content-build reports | BLD-022 |
| `full`, `full_server`, `development`, `soak` | baseline edition ids | table 3.2-D |
| `modes`, `crate_classes`, `tutorial`, `play_local_network`, `play_internet`, `map_editor` | runtime feature flags | table 3.2-B |
| `all`, `efigs` | language groups | table 4.7-A |
| `--write-build-identity`, `--verify-content` | start-up switches | BLD-063, BLD-103 |
| `SPECIES_BUILD_NUMBER`, `SPECIES_SOURCE_REVISION` | cache variables | table 2.1-B |
| `windows-msvc-asan`, `windows-msvc-analyze` (with `-<product>`) | preset stems | table 2.1-A |
| `ci-change-*`, `ci-nightly-*`, `ci-release-package` | workflow presets | table 4.9-A |
| `BuildIdentity`, `ContentBuild`, `MapIdentityEntry`, `LanguageFindings`, `RunOptions`, `RunResult` | C++ types in `species::build` and `species::contentbuild` | §2.3 |

## 10 Dropped as legacy or out of scope (one line per group)

- **Reference map checksum mechanism** (DL-LEGACY-TECH, DEC-063): the 32-bit checksum variant and its trailing byte, the fixed 100 000-byte buffer and its overflow, file-name sanitising, the generated header grammar and its 87 defines, the IDE pre-build step, the first-space command-line split. Replaced by SHA-256 over cooked content (ADR-0060); the lessons became BLD-072 to BLD-076.
- **Demo, preview, hardware-compatibility, stress and beta-group targets** (DL-DEMO, DL-TESTMAPS): their columns of the feature-macro table, their permitted-map lists, the demo map list and demo limits, the "no message" switches, spectator-only and hidden-mode switches, and the zero-terminated list convention. Only the full-game column was kept, as the reference feature set (table 3.2-B, table 3.2-C).
- **Debug-target content** (DL-DEBUG, DL-CUT): the debug-only modes and the debug creature outcome; the developer tools are mapped to DEC-041 or recorded as not specified (table 3.2-C).
- **Online-directory uses** (DL-SERVICES, DD-NET): the advertised map checksum and server-browser resolution, the distributor tag sent to the directory (now DEC-204, DEC-245, DEC-249 design-phase inputs).
- **Data-build scripting** (DL-LEGACY-TECH): the batch, list-file, copy and path-rewrite pipeline; exclusion and inclusion lists per target; the platform and language arguments; console, partner and peripheral variants; the archive tool, its switches and solid archives; the per-archive split (start-up, main, language, menu sounds, sounds, remaining sounds). Replaced by positive edition selection (BLD-049) and the pack set of AST-057.
- **Packaging scripts and installers** (DL-LEGACY-TECH, DL-IP-ONLY, DEC-340): the executable packer, the debug-help library, the support tool and its settings file, the auto-patcher per language, the copy helper, the profile locator, the FTP upload client, the binary patch of a regional executable, the distributor's tag value, the reference manual file, the reference installer version numbers, the earlier-product scripts. Their Species replacements are BLD-096 to BLD-105 and BLD-114; a player manual is a design-phase input.
- **Version artefacts of other platforms** (DL-PLATFORM): the other-platform bundle property list and the OS game-explorer descriptor.
- **Font-page lists** (DL-LEGACY-TECH): per-language bitmap font pages (Latin-1, Cyrillic, CJK, Hangul page ranges) and their packaging gaps. Replaced by the glyph cache (ADR-0034) and the coverage check of AST-088, which also covers the currency-symbol block that the reference never shipped.
- **Translator tool mechanics** (DL-LEGACY-TECH): UCS-2 big-endian inputs, the scripting-language report tool and its batch wrappers, the third-party archiver, the markup emulator in another scripting language, the earlier product's string-subset tool. The behaviour is kept as BLD-083 to BLD-085.
- **Unused-resource script mechanics** (DL-LEGACY-TECH): the folder-name guard, the regular-expression pipeline over code, the base-name comparison and the exceptions file. The behaviour is kept as BLD-069 (reference-graph audit) on top of AST-089 and AST-090.
- **Narrator writer of the translation report** (DL-DEAD).
- **Carry-forward rows owned elsewhere**: floating point (DD-NUMERIC; only the per-target property of BLD-040 is here), window and swap chain, timing, user data and crash handling (C-PLT), archives (C-AST), and the authoring-tool exporter and maths sandbox (replaced by glTF export, BLD-043).
