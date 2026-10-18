# P6-ADR report

## Summary

- Findings: 79 in the packet. **70 fixed**, **8 fixed differently**, **1 rejected**, **0 left as request only** (counts after round 2; four fixed findings also need a matching change in another packet's file; see Requests). Round 2 (O30 and late requests) is at the end of this report.
- Relayed requests from other packets: 3 applied (table below).
- Files edited: `design_next/ADR/README.md`; ADR-001, 002, 003, 004, 006, 007, 008, 011, 012, 013, 014, 015, 016, 017, 019, 020, 022, 023, 024, 025, 028, 029, 030, 034, 053, 054, 055, 056, 057, 059, 062, 064, 065, 069, 070, 072, 073, 076, 077; `design_next/architecture/components/build_tooling_and_distribution.md`. Deleted: `design_next/ADR/.gitkeep`. ADR-078 not created and its index row not added.
- Gates: `python3 -I Tools/DesignCheck/CheckDesign.py design_next` with Mermaid: all 11 gates pass (266 Mermaid blocks, 0 failed). The last run, without Mermaid and after the last edit, also passes.
- Ruling 1 (O29): applied through ADR-054 (amendment note; the O26 note is marked superseded), ADR-017, ADR-019, ADR-055, ADR-056, ADR-057, ADR-059, ADR-062, ADR-069, ADR-070, the ADR README and BLD. No "tools solution" is left in my files. I checked the result against `AGENTS.md` §2, §3 and R14 as they stand now. One point differs; see Requests item 7.
- Ruling 11: every Proposed record (17) now has a **Confirmed when** line after its "Default while Proposed" sentence: the evidence (the tests of its Confirmation section) and the milestone (the first playable, O20, for the records on that path; "before the first external build", "before the options pages are final" or the design phase's storefront choice for the others). The README states the rule. ADR-076 became Accepted (R3b-44) and ADR-053 Accepted (R4b-75).
- ADR file pattern: the README states `ADR-NNN-<slug>.md` with three-digit ids (O22, `AGENTS.md` §6). No four-digit id remains. New records take the next id after the highest one. The README does not name ADR-078, so its text holds before and after that record lands.
- Facts that are new in ADRs were checked on 2026-10-18 and are cited in the records. Microsoft Learn: the MSBuild solution-folder target syntax and the `.slnx` solution folders; the D3D12 swap effects; the AddressSanitizer incompatibilities; the MSBuild `DownloadFile`, `VerifyFileHash` and `Unzip` tasks; MSVC binary compatibility; the float depth-bias formula. **Outside Microsoft Learn**, beyond what the brief names: nuget.org and the DirectXTex repository's `LICENSE`, for the DirectXTex package id, version and licence; and the glTF Validator release page. I also downloaded two files into the scratchpad to measure them; neither was executed. The validator archive gave the size and SHA-256 that ADR-019 records (computed with `sha256sum`). I opened the DirectXTex `.nupkg`, a zip file, to read its `.nuspec` and file list. Say if this oversteps the brief.

## Findings

| Id | Status | Note |
|---|---|---|
| R2b-33 | fixed | ADR-022 context: the engine "is a voice graph on XAudio2 under the data-driven (sound source, event) binding of the audio design". |
| R2b-44 | fixed | ADR-059: the reference mesh counts are removed; the rule cites DEC-012 and the mesh fails the build. |
| R4b-80 | fixed | README: `ADR-NNN-<slug>.md`, three-digit ids, O22 and `AGENTS.md` §6; no four-digit number remains. |
| R3b-1 | fixed-differently | O29 resolves it: there is one solution, so every `$(SolutionDir)` path is the repository root for the tool projects too; neither option A nor option B is needed. ADR-054 has the amendment, `/t:Tools\<ProjectName>` and `$(SolutionDir)Tools\<Project>`; BLD-005, BLD-006, BLD-041, BLD-043, BLD-044 and table 3.1-A follow. |
| R2b-23 | fixed | ADR-001: the OS reach cites the floor of ADR-050 (Windows 11 24H2). |
| R3b-29 | fixed | ADR-002: Confirmed when (probe test and golden images on a GPU at exactly 12_1, first playable; hardware data relaxes to option 2). |
| R3b-30 | fixed | ADR-004: Confirmed when, with the PIX measurement that would revise it. |
| R3b-31 | fixed | ADR-006: Confirmed when (first material shader and GPU-based validation). It is not set to Accepted, because ADR-002, which guarantees the binding model, is itself Proposed. |
| R3b-32 | fixed | ADR-007: Confirmed when, on both barrier paths. |
| R2b-28 | fixed | ADR-012: the blit-model option and its pros and cons are deleted (Direct3D 12 supports only the two flip swap effects; Microsoft Learn cited); amendment note. |
| R3b-33 | fixed | ADR-013: Confirmed when (golden images match the formulas before the first golden image is approved). |
| R3b-34 | fixed | ADR-015: Confirmed when (golden images and the measured 4x cost against the budget). |
| R2b-29 | rejected | The conversion is not a 24-bit trick. On a float depth target a bias unit is 2^(e−23) (Microsoft Learn, Depth Bias), so round((k−1)×2²⁴) units give between (k−1) and 2(k−1) times the primitive's depth, as RND-220 and `rnd.depth_bias.unit_scale` state for `D32_FLOAT`. Deleting it would contradict the renderer. ADR-016 now gives this derivation and its source. |
| R3b-35 | fixed | ADR-016: Confirmed when, before the first cooked mesh. |
| R4b-83 | fixed | ADR-017: MIT licence. The package declares the licence expression and has no licence file, so the tool project keeps the repository's `LICENSE` text beside its `packages.config`; More Information gives the sources. |
| R3b-11 | fixed | ADR-017: NuGet `directxtex_desktop_win10` 2026.5.8.1 (the may2026 release, x64 static libraries), licence, where the text is kept. The package is deprecated on nuget.org; that is recorded as a consequence. README items 5.2 and §5 follow. |
| R3b-13 | fixed-differently | Round 2, after O30: the Khronos validator is no longer a dependency. ADR-019's glTF importer validates the sources itself, with nine concrete check groups (container; JSON; required properties and indices; buffer-view and accessor bounds; component types; index ranges; values and hierarchy; extensions; Species rules) and one failing fixture per check. The download, hash and licence are gone. AST-076 and AST-091 are requests (Requests 1). |
| R2b-34 | fixed | ADR-022: "as CONVENTIONS §8 names it". |
| R2b-38 | fixed | ADR-025: the 173 MiB estimate is removed; the budgets cite their keys (`aen.memory.*`), and the content build checks the authored set against them. |
| R2b-39 | fixed | ADR-025: the sentence about the reference engine's sample cache is deleted. |
| R3b-36 | fixed | ADR-025: Confirmed when. Not Accepted: R14 rules out only option 5, and options 2 to 4 remain. |
| R2b-43 | fixed | ADR-030: the amendment cites "(O21, `AGENTS.md` R14)". |
| R3b-37 | fixed | ADR-034: Confirmed when. |
| R3b-4 | fixed | ADR-062 has a new bullet, "Generator build and drift check". The generator is a hand-written tool project of its own that references no project holding generated code. `CheckGeneratedSchemas.py` builds it through the solution and compares a regenerated temporary tree with the committed headers. Each schema names its owning project. BLD table 4.9-A adds the step, and ADR-070 follows. The DAT wording is a request (Requests 5). |
| R3b-6 | fixed | P6-DAT had already rewritten the Code generation bullet as committed source (O28). The bullet is now mine; it is aligned with DAT-054 and the amendment. |
| R2b-45 | fixed | As R3b-6: the bullet states committed source, regenerated by a developer and checked by CI. |
| R3b-7 | fixed | ADR-062: Default while Proposed and Confirmed when (before the first content record is committed). The decision states that the committed-code handling is the owner's O28 and is not open; the decision-makers line names the owner for it. |
| R2b-47 | fixed | ADR-069: fuzz harnesses live in `<Project>Tests`; a separate fuzz project needs an owner decision and an `AGENTS.md` change (O27). |
| R3b-8 | fixed-differently | Under O27 the baseline has no `<Project>Fuzz` project, so there is no checker exception and no fuzz CI job. The harnesses are `<Project>Tests` methods that replay a seed corpus in the Debug test run. ADR-069 records the settings a coverage-guided fuzzing project would need for the later owner decision: `/fsanitize=address` and `fuzzer`, `BasicRuntimeChecks` off, `LinkIncremental` false (Microsoft Learn cited). BLD table 3.1-A, table 4.9-B, figure 4.9-A and ADR-070 drop the fuzz projects. |
| R4b-81 | fixed | README: the baseline ranges are described as history, not policy; new records take the next id after the highest. |
| R2b-20 | fixed | README: "ADR-001 to ADR-036 … ADR-050 to ADR-077; ADR-037 to ADR-049 are unassigned and stay unused". |
| R2b-18 | fixed | README line 3. |
| R3b-5 | fixed | BLD-001 owns the drift check; table 4.9-A row between restore and build; §6 failure row. |
| R3b-3 | fixed-differently | BLD-006 now states what the project table must hold (name, namespace, kind, components, references) and its constraints: acyclic, R9, BLD-041 and BLD-045, the server- and tool-linked parts apart from the client-only parts, the client as composition root (DEC-221). The table itself is added by the first-project ADR (`AGENTS.md` §2). Reason for not writing it now: the splits it would encode (R3b-14 to R3b-17) are being fixed in parallel by P6-ENG2, P6-RND and P6-DAT, and BLD has under 2 KB of its 120 KB left. See Requests 6. |
| R2b-213 | fixed | BLD-017 and table 3.2-C deleted (cited nowhere else); the §8 rows that pointed at the table are repointed. |
| R4a-30 | fixed | The generated header is `BuildIdentityFields.h` (constants, no type); `BuildIdentity.h` is the hand-written header of the type. Changed in BLD-020, BLD-058, T8 and the §2.3 sketch comment, with an amendment of ADR-057. CONVENTIONS and PLT are requests (Requests 2, 3). |
| R3b-12 | fixed | BLD-037 and BLD-038 are scoped: game and engine projects use the allow-list; tool projects use the ADR record with its licence; game and engine projects may not restore a tool-only package. T4 has fixtures for both; ADR-055 Confirmation follows. |
| R2b-214 | fixed | BLD-094: "The first build milestone is the design phase's (DEC-229)." |
| R4b-85 | fixed | As R3b-5 (table 4.9-A row). |
| R2b-265 | fixed | `ADR/.gitkeep` deleted. |
| R2b-24 | fixed | ADR-003 amendment: `AGENTS.md` §2 states `CS`, permutation files and `.hlsli` (O25). |
| R2b-25 | fixed | ADR-006: "the Debug test run of the GPU job in CI". |
| R2b-26 | fixed | ADR-008: the content-pipeline ADRs are named as ADR-058 to ADR-062. |
| R2b-27 | fixed | ADR-011: `architecture/components/terrain/README.md`. |
| R3b-38 | fixed | ADR-014: Confirmed when, before the options pages are final. |
| R2b-30 | fixed | ADR-017: "every block-compressed asset row states a size that is a multiple of 4 (GameAssets §2.4)". |
| R2b-31 | fixed | ADR-019: the amendment states the present rule only. |
| R2b-32 | fixed | ADR-020: the work-note sentence is deleted. |
| R2b-35 | fixed | ADR-022: "the executable has no decoder of its own". |
| R2b-36 | fixed | ADR-023 path. |
| R2b-37 | fixed | ADR-024 path. |
| R2b-40 | fixed | ADR-025: "No codec is compiled into the executable." |
| R2b-41 | fixed | ADR-028 path. |
| R3b-39 | fixed-differently | ADR-028 stays Proposed, with a Confirmed when line, because options 2 and 3 are not excluded by `AGENTS.md` and the device tests have not run. The decision names only the pin in force (3.5.283, `GameInput::v3`); the move to the 4.0 line is recorded under More Information as a future amendment. README items 6 and §6 follow. |
| R2b-42 | fixed | ADR-029 path. |
| R4b-75 | fixed | ADR-053: status accepted, title "Headers only: no modules and no `import std`", chosen option "Headers only", the rejected option kept, amendment note; README index, item 2.2 and BLD-034/§8 follow. |
| R4b-84 | fixed | ADR-055: each tool dependency is recorded in an ADR with its licence (O26, O29, `AGENTS.md` §2); the licence text is kept with the pinned dependency. |
| R2b-46 | fixed | ADR-064: "team and faction identity". |
| R3b-40 | fixed | ADR-064: Default while Proposed and Confirmed when. |
| R3b-41 | fixed-differently | ADR-065 gets a Confirmed when line (before the first external build) and stays Proposed, not Accepted: option 3, an own handler, is not excluded by R14, and the crash tests have not run. |
| R2b-48 | fixed | ADR-070: `<os>-<compiler>-<configuration>-<product>`, configuration Debug or Release. |
| R3b-42 | fixed | ADR-072: Confirmed when, after the design phase's storefront choice. |
| R3b-43 | fixed | ADR-073: Confirmed when, before the first external build. |
| R3b-44 | fixed | ADR-076 Accepted: R14 decides the encoder; external video capture is the decision, and an engine recorder would be an amendment. Also the README index. PLT and DAT §8 rows are requests (Requests 3, 5). |
| R2b-49 | fixed | ADR-077: "in either configuration (Debug or Release)". |
| R3b-45 | fixed | ADR-077: Confirmed when, before telemetry is first on in a build that leaves the team. |
| R2b-22 | fixed | README: the link now cites the current "Compiling shaders" page (`dx-graphics-hlsl-part1`). |
| R2b-19 | fixed | README line 7: "No baseline record carries that flag." |
| R2b-21 | fixed | README: "This table is the Species technology baseline; the ADRs and `AGENTS.md` R14 cite it." |
| R3b-61 | fixed | As R4b-80 and R4b-81. |
| R2b-211 | fixed-differently | The explicit anchor is deleted rather than renamed: the heading already gives `31-products-configurations-and-compile-time-switches`, and two links use it; no link used the old anchor. |
| R2b-212 | fixed-differently | As R2b-211, for `42-products-configurations-and-binary-checks`, which five links use. |
| R2b-215 | fixed | BLD: `species_contentbuild.exe` (and the job command). CONVENTIONS is a request (Requests 2). |
| R3b-62 | fixed | New BLD-123 and table 4.1-B: the 13 rules of `Build\CheckProjectFiles.py`, each with its source. T39 has one failing fixture per rule. Table 4.9-A cites the table. |
| R3b-46 | fixed | `x64\<Configuration>\Generated\<Project>\`, on that project's include path only, neither registered nor checked by format or lint (BLD-020); ADR-054 has a "Generated build output" rule. |
| R3b-65 | fixed | BLD-029: include-path order, not `#include` order (`AGENTS.md` §4). |
| R3b-64 | fixed | BLD-042: the client build copies `D3D12Core.dll` and `D3D12SDKLayers.dll` into `x64\<Configuration>\D3D12\`; packaging copies only `D3D12Core.dll`. |
| R4a-64 | fixed | BLD-121: `<Shader><Stage>.hlsl` for the default permutation, `<Shader><Permutation><Stage>.hlsl` otherwise (RND-261; `SkyVS.hlsl`, `TerrainInvertedPS.hlsl`), `CompiledShader/<Shader><Permutation><Stage>.h`, `g_<Shader><Permutation><Stage>`; T38 follows. |
| R2b-216 | fixed | BLD §8: "DEC-332, DEC-340, Dropped: Species defines its own editions (ADR-056), packs (ADR-058), content build, packages and crash path". |

## Requests from other packets applied

| From | Request | Applied |
|---|---|---|
| P6-RE request 7 | BLD-053: delete "The reference edition permits every map it holds." | yes |
| P6-DAT request 7 | ADR-062: amendment for O29 and DAT-054, and the Code generation bullet made to agree | Merged into one amendment, "2026-10-18 (O28, owner decision O29, DAT-054)", with my R3b-4 change. The amendment and the bullet make the generator a tool project of its own beside the content build tool, not part of it, so that it builds from a tree with stale generated headers (R3b-4). Otherwise as requested. |
| Coordinator | clear the gate 1 and 2 hits in ADR-017 and ADR-019 | Done: the `docs/` file names in ADR-019 are described in words; "legacy" and a `porting` URL in ADR-017 are replaced (the binary-compatibility fact now cites "What's new for MSVC Build Tools"). |

## Requests

1. **P6-RND, `architecture/components/asset_pipeline_and_resources.md`.** (a) Changed after O30. AST-076: replace "validates each source against the glTF 2.0 specification (for example with the Khronos glTF validator)" with "validates each source against the glTF 2.0 specification with the importer's own checks of ADR-019 (no third-party validator, O30)". AST-091: replace "every glTF source passes the glTF validator" with "every glTF source passes the importer's checks of ADR-019". (b) AST-001: replace "a development tool in the tools solution under `Tools/`" with "a development tool, a tool project under `Tools/`" (ruling 1).
2. **P6-GLO, `CONVENTIONS.md`.** (a) Row "Build tooling and distribution (BLD) | — | data names" (line 617): `species-contentbuild.exe` → `species_contentbuild.exe` (R2b-215). (b) Table 8.12-B, BLD row (line 616): replace "the generated header `BuildIdentity.h`" with "the generated header `BuildIdentityFields.h` (the build-identity constants, BLD-020; `BuildIdentity.h` is the hand-written header of `BuildIdentity`)" (R4a-30). (c) Line 642, the BLD term list: replace "tools solution" with "tool project" (ruling 1; BLD now defines "tool project" in its notation).
3. **P6-ENG2, `architecture/components/platform_layer.md`.** (a) PLT-104: "the generated header `BuildIdentity.h`" → "the generated header `BuildIdentityFields.h`" (R4a-30, BLD-020). (b) §8 row "ADR-065, ADR-076, ADR-028, ADR-077 | Proposed": remove ADR-076 and add a row "ADR-076 | Accepted | screenshot encoding with the in-box WIC PNG encoder; video by external tools" (R3b-44).
4. **P6-ENG2, `architecture/components/diagnostics_test_and_performance/test_harness_soak_and_telemetry.md` line 33, the "fuzz" row.** Replace the text with: "harnesses for the pack-file reader, every binary record loader and the authoring parser, test methods of the library's `<Project>Tests` project that replay a committed seed corpus and every input that has ever failed, in the Debug test run; a coverage-guided fuzzing project needs an owner decision and an `AGENTS.md` change (O27, ADR-069)" (R2b-47, R3b-8).
5. **Coordinator (P6-DAT has finished), `architecture/components/data_schemas/`.** (a) README DAT-019: replace "The parser and the validator are Species code generated with the schemas (ADR-062)" with "The parser is hand-written Species code in a project that holds no generated code, and the validator is generated with the schemas (ADR-062)" (R3b-4: the generator reads the schemas with the parser). (b) README §2 preamble: "by the Species code generator of the content build tool, a tool project of the one solution under `Tools/`" → "by the Species schema code generator, a tool project of the one solution under `Tools/` beside the content build tool". (c) Let each schema name the project folder that owns its generated header (R3b-4). Either add `x-species-project` to the closed annotation list of DAT-015, or add an "Output project" column to DAT table 1-A; the coordinator chooses. (d) `entity_audio_geometry_and_panel_schemas.md` line 459: "ADR-076 \| Proposed" → "ADR-076 \| Accepted" (R3b-44).
6. **Coordinator, R3b-3.** After wave 2, once R3b-14 to R3b-17 are settled, decide whether a default project table goes into BLD §2.1 now. BLD is at about 121 KB of its 122,880-byte limit, so §2.1 or another section would have to move to a file of its own first. The alternative is to leave it to the first-project ADR, which BLD-006 now requires.
7. **Resolved by O30 (round 2); kept for the record.** `AGENTS.md` §2 says a tool project's dependency is "restored by NuGet for that project only". The Khronos glTF Validator is not on NuGet, so ADR-019 has the tool project fetch the pinned archive itself. It uses MSBuild's built-in `DownloadFile`, `VerifyFileHash` (SHA-256) and `Unzip` tasks. ADR-054, ADR-055 and the README say "restored for that project only", without "by NuGet". This is the one place where my files differ from `AGENTS.md` as written. The owner either accepts it, and `AGENTS.md` §2 reads "restored by NuGet, or fetched at a pinned URL and SHA-256 by MSBuild's own tasks, for that project only", or chooses another route, such as a local NuGet package that wraps the archive. Also for the owner: nuget.org marks the DirectXTex NuGet package deprecated in favour of vcpkg, which `AGENTS.md` excludes. The pinned version restores, but a later DirectXTex release may need a decision (ADR-017 consequence).
8. **Coordinator.** The ADR index row for ADR-078 (P6-RND) is yours as planned. The README's baseline sentence ("ADR-001 to ADR-036 … ADR-050 to ADR-077") and the rule "a new record takes the next id after the highest existing one" stay correct after it lands.

## Round 2: O30 and late requests (2026-10-18)

**O30**: a tool project's third-party dependency is restored by NuGet only; the build downloads nothing from another source, and no third-party binary is committed.

| Change | Files |
|---|---|
| The Khronos glTF Validator is no longer a dependency. The importer validates what it imports itself: the GLB container; strict JSON and `asset.version`; required properties and index ranges into arrays; buffer-view and accessor bounds and alignment (stride a multiple of 4 from 4 to 252); the component types of `POSITION`, `COLOR_0`, `TEXCOORD_n` and indices, with `POSITION` `min` and `max` present; index ranges, with no restart value and triangles only; finite values and an acyclic hierarchy; the extensions; and the Species rules. It rejects the asset with its id, the JSON pointer or byte offset, the rule and the value found. A Confirmation line adds one failing fixture per check. The pin, licence, archive, SHA-256 and the MSBuild `DownloadFile`/`VerifyFileHash`/`Unzip` mechanism are removed. The 2026-10-17 validator amendment is marked superseded, and the O29 amendment is reduced to the solution change. The checks were read from the glTF 2.0 specification on 2026-10-18. | ADR-019 |
| The validator is removed from technology baseline item 8.1, from §8 and from the sources of §8. | ADR README |
| "restored by NuGet for that project only", with "nothing downloaded from another source, no third-party binary committed"; amendment notes for O30 | ADR-054, ADR-055, ADR README §3, ADR-017 (DirectXTex bullet), BLD-043 (validator text removed), BLD §8 row |

| Late request | Applied |
|---|---|
| P6-ENG1 request 5: ADR-024 amendment on the nine DSP filter kinds (DEC-339) | yes, as written |
| P6-ENG2 request 1 (R3b-9, R3b-60): BLD table 4.1-B row 13 replaced and row 14 added; BLD-076 names `Build\CheckProjectFiles.py` (row 14); the §6 row and T24 become checker items | yes, as written; BLD is now 122,040 bytes, under the 122,880-byte limit |
| P6-X request 6: ADR-062 Schemas bullet names the project folder that owns the generated header (DAT-015, `x-species-project`) | yes |

Gates after round 2: all 11 pass, run without Mermaid; no diagram changed in this round.
