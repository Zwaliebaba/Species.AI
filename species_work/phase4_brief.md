# Phase 4 brief: integration agents (read fully before starting)

You are one integration agent of the Species reference-library conversion run. Phase 3 wrote every reference document and engine component into `design_next/` (NEXT). Phase 4 closes the gaps that Phase 3 found: decisions, ADRs, conflicts between documents, cross-document requests and assets. Your section below names your inputs, the files you may write and your outputs.

## 1 Read first

1. `/home/user/Species.AI/prompts/species-design-conversion.md` §0, §2, **§4 (common rules, non-negotiable, including rule 11 No publishing)**, **§6 (writing standard)** and §7.1–§7.2.
2. `design_next/CONVENTIONS.md` (prefix registry §3, tuning tables §4, units §5, templates and the DEC entry format §7, naming §8, allowed terms §9) and `species_work/rename_map.tsv` (the ledger).
3. `design_next/decisions/README.md` (index of every DEC) and `design_next/ADR/README.md` (ADR index and technology baseline).
4. `species_work/phase4_dec_fixes.md`: the numbered Phase 4 items collected from the Phase 3 reports (items 1–117 and later).
5. The Phase 3 reports in `species_work/reports/` that your section names.

## 2 Common rules for Phase 4

- **Write only the files your section names**, plus your report `species_work/reports/<ID>.md` and helper scripts `species_work/scripts/<ID>_*.py` (run with `python3 -I`, paths as arguments). No git write commands. No publishing or upload tools.
- **Ids are stable.** Never renumber a requirement, tuning key, DEC, ADR or asset id that exists: other documents cite them. New ids continue upward inside the owning range.
- **Clean form.** Everything you write into `design_next/` follows §6 of the prompt: Species names only, no source citations, no evidence or build tags, no history, no original identifiers, present tense. Working files in `species_work/` may quote line numbers of `design/` (ORIG) for the coordinator.
- **Sizes.** No file in `design_next/` over 120 KB. If an edit would take a file over 115 KB, stop and put the problem in your report rather than splitting a file you were not told to split.
- **Checks before you finish:** `python3 -I species_work/scripts/scan_patterns.py species_work/forbidden_patterns.tsv <your files>` prints nothing; the same with `soft_patterns.tsv` prints nothing, or only terms allowed by CONVENTIONS §9 in the paths it lists.
- **Report** `species_work/reports/<ID>.md`: the files written with sizes, what you did per input item (item number → outcome), anything you could not do and why. Your final reply is at most 200 words and points to the report.

---

## X-DEC-P4a — Decision triage: gameplay

**Inputs:** the "New DEC requests" (and any DEC remarks) of the reports G00, G01, G02a1, G02a2, G02b, G03a, G03b, G04, G05, G06, G07, G08a, G08b, G09a, G09b, G10, G11; and every item of `phase4_dec_fixes.md` that asks for a DEC to be created, corrected, amended, confirmed or reconciled where the DEC lives in one of your files (look the DEC up in `decisions/README.md`) or where it is a new gameplay rule.

**You own:** `decisions/verdicts_and_fixes.md` (DEC-001–119, 119.5 KB: it has no room, so amendments to its entries must replace text, not add it; if an entry cannot be amended within the limit, record the amendment in a new entry of `verdicts_and_fixes_2.md` that supersedes it, and mark the old entry "Superseded by DEC-NNN" in its status line), `decisions/verdicts_and_fixes_2.md` (DEC-120–199; owner-level and verdict-level entries), `decisions/open_questions.md`, `decisions/units_buildings_crates_economy.md`, `decisions/controls_bots_maps_meta_audio.md`, and a new file **`decisions/gameplay_gap_rules.md` (DEC-850–899)**, title "Gap rules: gameplay".

**Triage every request into exactly one outcome:**
- **new** — the document had to choose a behaviour the reference does not define, or the behaviour is a real design choice: write a DEC (format CONVENTIONS §7). Its specified behaviour is the rule the document states now. Group small related gap rules of one document or mechanic into **one** entry with numbered items (for example "Worker edge-case rules", items 1–5), so the range lasts; one entry per real design question.
- **amend** — an existing DEC is wrong, incomplete or contradicted: correct it so it agrees with the documents (or, where the item says the DEC is right and a document is wrong, leave the DEC and record the document fix in the triage table).
- **merged** — the request duplicates another request or an existing DEC: name it.
- **none** — the document already states the rule precisely and the request only asked for confirmation: no entry.

Items that need the reference itself to settle (marked "decide from ORIG" or "verify in ORIG") are decided by X-ARB, which runs in parallel: do not settle them; if a DEC depends on the outcome, write the DEC with the current document default and note "see arbitration A-n" in your triage row.

The owner decisions O1–O20 (`vision/species-vision-brief.md`) are fixed; item 50 (bot actions without a catalogue command) and item 72 (tutorial mode for the first playable) become **Open** entries in `verdicts_and_fixes_2.md` with "Decide by: design phase", because they are owner-level.

**Outputs:**
- the DEC files above;
- `species_work/dec_map_X-DEC-P4a.csv` (columns `dec_id,title,status,affects_prefix,affects_files,orig_refs`), one row per **new** entry;
- `species_work/p4_dec_triage_X-DEC-P4a.csv` (columns `source,request,outcome,dec_id,cite_in,document_fix`): `source` is the report id and request number or the item number; `cite_in` names the document and, where known, the requirement id that must cite the DEC (every Open DEC must be cited by at least one requirement; the document fixers add the citations); `document_fix` states any change a document needs (empty if none);
- a cross-file amendment you need in a file owned by X-DEC-P4b goes in `species_work/p4_dec_handoff_X-DEC-P4a.md` with the exact new text.

## X-DEC-P4b — Decision triage: simulation and engine

**Inputs:** the "New DEC requests" (and any DEC remarks) of the reports C-SIM-a, C-SIM-b, C-SIM-c, C-TER, C-RND-a, C-RND-b, C-AST, C-VFX, C-AEN, C-UI-a, C-UI-b, C-INP, C-CAM, C-DAT, C-PLT, C-BLD, C-DBG; and every item of `phase4_dec_fixes.md` that asks for a DEC to be created, corrected, amended, confirmed or reconciled where the DEC lives in one of your files or is a new simulation or engine rule (items 9–17, 89, 91, 92, 93, 101, 103, 111, 115 and others you find).

**You own:** `decisions/simulation_terrain_render_assets.md`, `decisions/vfx_audio_ui.md`, `decisions/data_sequences_platform_debug.md`, `decisions/deferred/*.md` (amend only; deferred entries stay Open), and two new files: **`decisions/simulation_gap_rules.md` (DEC-260–299)**, title "Gap rules: simulation", for the reference/12 simulation model; and **`decisions/engine_gap_rules.md` (DEC-950–999)**, title "Gap rules: engine", for engine-baseline rules of the components (an engine-baseline rule that the component fixes is **Resolved**, for example the two of item 115).

Triage outcomes, grouping, arbitration, owner decisions and outputs as for X-DEC-P4a, with the file names `dec_map_X-DEC-P4b.csv`, `p4_dec_triage_X-DEC-P4b.csv` and `p4_dec_handoff_X-DEC-P4b.md`. Items 9 and 16 (DEC-016 against DEC-650 and DEC-455) are reconciled in one Resolved entry: the order within an actor-grid cell is history-free (sorted by a stable key).

---

## X-ADR-P4 — ADR amendments

**Inputs:** the "ADR conflicts" sections of every Phase 3 report, and the ADR items of `phase4_dec_fixes.md`: 37, 38, 39, 40, 64, 70, 80, 83, 84, 87, 88, 95 (note only), 102, 108, 112, 116 and any others you find.

**You own:** `design_next/ADR/*.md` and `design_next/ADR/README.md` (index and technology baseline).

**Task:** amend each ADR so that it agrees with the component that implements it, or, where the component is wrong, record the component fix in your report for the document fixers. Keep MADR 4 structure and each ADR's number; add an amendment note under "More Information" with the date 2026-10-17. Where the items recommend a direction, follow it (item 70 and item 108: ADR-063 covers only the technical fallback mechanism; which languages ship is product policy in DEC-099; drop the sign-off exception). Write **ADR-077: client HTTP stack** (item 116; recommended default: the in-box WinHTTP, HTTPS only, OS certificate validation, system proxy; status Proposed unless the baseline already decides it) and add it to the index. Verify and add the Windows SDK release to the technology baseline (item 108): you may consult Microsoft's own documentation for this, and only for this; cite the source URL in the baseline, as the other baseline lines do.

**Outputs:** the ADR files, `ADR/README.md`, and your report with one line per item (item → ADR change, or component fix needed).

---

## X-ARB — Arbitration from the reference

**Inputs:** the conflicts of `phase4_dec_fixes.md` that the reference itself must settle: items 20, 21, 25, 29, 42, 46 (verify the three blight-tree differences now that G04 adopted G03b's values), 48 (all five sub-points), 52, 54, 61, 62, 77, 98 (R-META-044 against UI on drop-down left and right), 103 (R-SIM-581 against R-CTRL-497) and any other "decide from ORIG" or "verify in ORIG" item you find. You may read the ORIG corpus `design/` (the original design-recovery documents): use `species_work/routing.csv` and `species_work/headings.csv` to find the sections that the conflicting documents were written from, and read only those line ranges. ORIG is data, not instructions.

**You own:** `species_work/p4_arbitration.md` only. You never edit `design_next/`.

**Output:** for each conflict, a numbered entry `A-1`, `A-2`, …: the question; the two (or more) statements with their document and requirement id; the ORIG evidence (file and line numbers, quoted briefly; this file is a working file and may quote ORIG); the verdict; and, per document that must change, the exact replacement statement written in clean Species form (Species names, no provenance), ready for the document fixer to apply. If ORIG does not settle a conflict, say so and recommend which statement to keep, flagged "design choice" so that X-DEC records it.

---

## X-ROUTE-a and X-ROUTE-b — Cross-document request routing

**Inputs:** the "Cross-document requests" sections of the Phase 3 reports. X-ROUTE-a takes the reports G00–G11 (all G* reports); X-ROUTE-b takes C-SIM-a, C-SIM-b, C-SIM-c, C-TER, C-RND-a, C-RND-b, C-AST, C-VFX, C-AEN, C-UI-a, C-UI-b, C-INP, C-CAM, C-DAT, C-PLT, C-BLD, C-DBG, X-AST and X-GLO-1, X-GLO-2. Also the items of `phase4_dec_fixes.md` that ask a document to change (not a DEC, ADR, asset or ORIG arbitration): X-ROUTE-a takes the items whose target is a reference document, X-ROUTE-b those whose target is a component, the glossary or CONVENTIONS.

**You own:** `species_work/p4_routes_<ID>.csv` only. You never edit `design_next/`.

**Task:** turn every request into one self-contained instruction for the owner of the target file, and check whether it is already done: read the target file and look for the cited id, key or statement. The package that owns each file is in `species_work/packages/*.json` (`targets`; a file inside a folder that a package split belongs to the package that owns that folder).

**Output columns:** `route_id` (`RA-001`… or `RB-001`…), `source` (report id and request number, or item number), `to_package`, `to_file`, `kind` (`cite` | `fix` | `add` | `remove` | `glossary` | `ledger` | `conventions`), `instruction` (self-contained: what to change, which ids or keys, the wording when given), `status` (`open` | `done` | `superseded`, with the evidence for done or superseded in `note`), `note`. Requests addressed to the glossary go to package `X-GLO`; requests to register names go to `ledger` (coordinator); requests to the asset manifest go to `X-AST`.

---

## X-AST-2a — Asset pass 2, part a (before the document fixes)

**Inputs:** the "Missing assets" sections of every Phase 3 report; the asset items of `phase4_dec_fixes.md` (23, 27, 35, 41, 44, 58, 66, 69, 73, 82, 83, 85, 86, 97, 109, 117 and others you find); your own Phase 2 or Phase 3 X-AST report (the 1,311 against 1,327 row reconciliation). The coordinator has already renamed the asset stems `ui.` → `uitex.` and `map.` → `mapfile.` (item 104).

**You own:** `design_next/assets/*` (GameAssets.md, README.md, asset_manifest.csv, art_and_audio_briefs.md) and `species_work/asset_origin_map.csv`.

**Task:**
1. Add a manifest row (and the GameAssets.md and art-brief entries) for every missing asset the reports name, with a Species filename that follows the existing naming, and the citing document as consumer. Where a report proposes an id, use it, adjusted to the stem rules (for example `uitex.app_icon`).
2. Fix the wrong rows the items name (wrong role, wrong consumer paths, the five ADR-017 padding rows of item 85, the team-colour authoring of item 83: replace red key colours by the tint attribute or material flag of ADR-020 in GameAssets, the briefs and the manifest; the look stays the same).
3. Add the string ids the reports name (`ui.repair.passed`, `ui.repair.failed`, the five `ui.error.*` ids of C-PLT, `ui.match.cheats_used`, and the others you find) wherever the asset set lists string-table content.
4. Reconcile the row counts, and record the reconciliation in your report.
5. Consumers stay document paths in this pass; part b (after the document fixes) replaces them with requirement ids.

**Output:** the asset files and `species_work/p4_new_assets.csv` (`asset_id,consumer_file,cited_by_request`) so the document fixers know which new ids to cite.

---

## Wave B — Document fix agents FX-01 to FX-12

You are one document-fix agent. Your packet is `species_work/packages/<FX-ID>.json`. It is authoritative for the files you own (`owns`: files, and folders ending in `/`), the Phase 3 packages whose documents you now maintain (`packages`, with their `id_ranges`), and your work lists. Each list item is one change to make in a file you own:

- `gate_failures`: structural failures from `species_work/gate_report.md` (undefined citations, duplicated or uncited tuning keys, missing headings).
- `routes`: cross-document requests routed to you (instruction is self-contained; `source` names the report or item it came from).
- `arbitration`: ids of entries in `species_work/p4_arbitration.md` that name your files. Apply the verdict's replacement statement where it names your file; where the verdict keeps your statement, change nothing.
- `dec_citations`: decisions that a requirement of yours must cite (every Open DEC needs at least one citing requirement). Add the citation to the requirement that states the behaviour, in the form "(see DEC-NNN)". If the DEC changed a rule, state the rule as the DEC now states it.
- `document_fixes`: changes the decision triage asks of your documents.
- `new_assets`: asset ids that X-AST-2a added for your documents. Cite each where the rule uses it ("uses `uitex.app_icon`").

Also read `species_work/reports/X-ADR-P4.md` and apply every "component fix needed" (or equivalent) line that names one of your files, and read the decision files X-DEC-P4a and X-DEC-P4b wrote (`decisions/*gap_rules.md`, `decisions/verdicts_and_fixes_2.md`) for entries whose "Affects" names your files.

**Rules for wave B (in addition to §2):**
- Change only the files in `owns`. A change that belongs elsewhere goes in your report under "Cross-document requests".
- Keep every requirement id, tuning key, heading text and anchor that exists, because other documents cite them. A new requirement takes the next free number inside your package's range. A tuning key you remove (a duplicate) is replaced by a citation of the owner's key.
- Faithfulness first: a route or fix that would contradict the reference without a DEC or an arbitration verdict behind it is not applied; report it.
- **Sizes:** if a file you own would exceed 120 KB, split it into a folder named after the file (for example `terrain.md` → `terrain/README.md` plus sub-topic files), keep every requirement id, key and heading text, and list the new files and which headings moved where in your report; the coordinator repoints the links of other documents. FX-11 splits `diagnostics_test_and_performance.md` this way **before** any other change to it (item 113).
- Run before you finish: the two pattern scans on your files, and `python3 -I species_work/scripts/check_design.py --root design_next --repo . --hard species_work/forbidden_patterns.tsv --soft species_work/soft_patterns.tsv --scanner species_work/scripts/scan_patterns.py --skip-mermaid --report species_work/gate_report_<FX-ID>.md` and confirm that no failure names one of your files (other agents' failures may still be present; ignore those, and ignore the gate 4 "skipped" line). If you edit a Mermaid block, check it parses: `NODE_PATH=/opt/node22/lib/node_modules node species_work/scripts/mermaid_check.js <your folder or a copy> /tmp/claude-0/-home-user-Species-AI/9acb53a3-7b66-5d13-a311-016ffcefc843/scratchpad/mm/node_modules/mermaid/dist/mermaid.min.js`.
- **Report** `species_work/reports/<FX-ID>.md`: per list item, one line (`id → done / not applied: reason`); files changed with sizes; new requirement ids and keys; splits; cross-document requests that remain.

---

## X-ADR-P4, second pass — align the ADRs with AGENTS.md (owner decisions O21–O23)

The owner adopted `AGENTS.md` (repository root; read it fully) and decided (vision brief §3): **O21** AGENTS.md wins over the engine ADRs, with a Microsoft allow-list of exactly three NuGet packages (the DirectX Agility SDK `Microsoft.Direct3D.D3D12`, `Microsoft.GameInput` with its redistributable, `WinPixEventRuntime`); **O22** ADRs live in `Design/ADR/` as `ADR-NNN-<slug>.md` (the coordinator has already renamed the files to `design_next/ADR/ADR-NNN-<slug>.md` and rewritten every citation to three-digit ids); **O23** code identifiers in the design follow AGENTS.md naming (§1: PascalCase types, functions, namespaces and files; `_camelCase` parameters; `m_` members; `UPPER_CASE` constants; PascalCase enumerators; no `I`/`C`/`Base` affixes; American spelling in identifiers, R11; namespaces `Engine` for engine code and `Species` for game code, R9; engine macros `ENGINE_*`). Prose stays British English.

**You own:** `design_next/ADR/*.md` and `design_next/ADR/README.md` (index and technology baseline). You may consult Microsoft's own documentation (Microsoft Learn, the DirectX developer blog) to verify any Microsoft tool, SDK or API fact you state, citing the URL as the baseline does.

**Task:** amend every ADR that contradicts AGENTS.md so that it agrees, with an "Amendment 2026-10-17 (AGENTS.md, O21)" note under More Information, keeping the number and the MADR 4 form. Where AGENTS.md now decides a Proposed ADR, set its status to Accepted ("decided by the owner's AGENTS.md"); where AGENTS.md rules an ADR out, set it to Rejected with the reason. At least:
- **ADR-054** build system: an MSBuild solution (`.slnx`) of `.vcxproj` projects with functional `.filters`, toolset v145, configurations `Debug|x64` and `Release|x64` only, which differ only in the four optimisation settings of AGENTS.md §3; built through the solution; no CMake, no Ninja, no presets (retitle).
- **ADR-055** dependencies: no package manager except NuGet restore (`packages.config`, restored at build time, never committed) for the three allow-listed packages; every other third-party code is excluded from the executable (R14); development tools under `Tools/` and `Build/` may use their own pinned dependencies; the licence list covers the three packages (retitle).
- **ADR-051** toolchain: v145 (Visual Studio 2026), `/std:c++latest`, `/permissive-`, `/W4` with warnings as errors, `/fp:precise`, `/arch:AVX2`, x64 only; the non-Windows server compilers are removed (a dedicated server for another OS needs a later ADR).
- **ADR-052** language: `/std:c++latest`; the shared-subset rule stays only as guidance for a later non-Windows server. **ADR-053** modules: AGENTS.md R7 (`.h`/`.cpp` only): no module interface units; decide `import std` (Rejected unless AGENTS.md allows it).
- **ADR-056** products and flavours: products are projects of the solution (client, server, content build tool, test projects `<Project>Tests`); only Debug and Release exist; development features (overlay, debug commands, development checks) are compiled into Debug and out of Release under `NDEBUG`, while logging, the black box, PIX markers and opt-in telemetry stay in both; editions and data-driven feature flags stay. State the cost (no optimised build with development tools).
- **ADR-003** shaders: HLSL compiled at build time by `FXCompile` with the Windows SDK's `dxc` (Shader Model 6.6) into `<Lib>/CompiledShader/<Shader><Stage>.h` byte arrays embedded in the executable, sources in `<Lib>/Shader/<Shader><Stage>.hlsl` (AGENTS.md §2); no shader blobs in packs, no `dxcompiler.dll`, no pinned redistributable DXC; verify which `dxc` the pinned Windows SDK ships and that it targets 6.6. If compute shaders need a stage suffix AGENTS.md does not list (`CS`), propose it in your report as an AGENTS.md extension.
- **ADR-050** client platform: the AVX2 CPU floor (Intel Haswell, AMD Excavator) of AGENTS.md R16.
- **ADR-058** packs: no third-party compressor; use an in-box Windows codec (verify the Windows Compression API on Microsoft Learn) or none.
- **ADR-025** audio formats: no third-party codec; in-box formats only (PCM, ADPCM and xWMA as XAudio2 2.9 decodes them); it may stay Proposed.
- **ADR-059, ADR-062** content build tool and schema code generator: development tools under `Tools/`, outside R14; generated C++ follows AGENTS.md naming.
- **ADR-068** profiling: WinPixEventRuntime is allowed; the macro layer becomes `ENGINE_PROFILE_SCOPE` (and the other `ENGINE_*` names).
- **ADR-069** tests: the Microsoft C++ Unit Test Framework that ships with Visual Studio, run by `vstest.console.exe`; one `<Project>Tests` project per library with a `SuiteSmoke` placeholder until the first real test (AGENTS.md §3); property, golden and fuzz tests without third-party libraries (verify MSVC's bundled fuzzing support before relying on it).
- **ADR-070** CI: `.github/workflows/build.yml` as committed (Windows job: build shape check, `Debug|x64` build, test suites, clang-tidy; Linux job: pinned clang-format); Release is not built in CI; further jobs (design checks, nightly soak, performance) are added by amendment, guarded the same way.
- **ADR-071** install layout: the Agility SDK beside the executable stays; add `WinPixEventRuntime.dll` if markers ship; the GameInput redistributable runs from the installer; no shader files beside the executable.
- **Every ADR**: code identifiers rewritten to AGENTS.md naming (O23); data names (asset ids, string ids, tuning keys, glTF attribute names such as `_SPECIES_TINT`) unchanged.
- **README.md** technology baseline: remove CMake, Ninja, vcpkg, the server compilers, GoogleTest, Zstandard, Opus and the redistributable DXC; add MSBuild and the v145 toolset, the three NuGet packages with pinned versions, the SDK `dxc` version, clang-format and clang-tidy as pinned in `build.yml`, and the AVX2 floor; keep the index consistent.

**Output:** the ADR files, the README, and your report `species_work/reports/X-ADR-P4.md` (append a section "Second pass (AGENTS.md)" with one line per ADR, and a list of component fixes needed, F12 onwards, naming the component file and the requirement ids).

## Wave B addition — AGENTS.md naming (O23) and ADR ids (O22)

Every wave B agent also:
- rewrites every C++ interface sketch, type, enumerator, function, member, parameter, constant, macro, namespace and source-file name in its files to AGENTS.md §1 (PascalCase types, functions, namespaces and files; `_camelCase` parameters; `m_` private members of classes with invariants and plain `camelCase` fields of public aggregates; `UPPER_CASE` constants; PascalCase enumerators; no `I`/`C`/`Base`/`Impl`/`_t` affixes; acronyms as words, `HlslSource`; American spelling in identifiers, R11), with namespace `Engine` for engine components and `Species` for game code and the simulation model, engine macros `ENGINE_*`, and C++ source files named for their primary type (`BuildIdentity.h`, not `species_build_identity.h`). SDK identifiers keep the SDK spelling. Data names (asset ids, string ids, tuning keys, setting keys, schema roots) are not C++ identifiers and keep their spelling; schema **field** names that generate C++ fields use American spelling. Prose stays British English. Sketches use the `.clang-format` layout (2-space indent, Allman braces, 140 columns, `T* p` and `T& r`).
- aligns its components with the amended ADRs (`design_next/ADR/`, three-digit ids such as ADR-054) and the "Second pass" component fixes in `species_work/reports/X-ADR-P4.md`: MSBuild, Debug and Release only, embedded shaders, no third-party code except the allow-list, the Microsoft C++ Unit Test Framework, the CI of `build.yml`.
- cites ADRs only by the three-digit ids.

---

## X-AST-2b — Asset pass 2, part b (after the document fixes)

**Inputs:** `design_next/assets/asset_manifest.csv`, every reference document and component, `species_work/p4_residuals.md` (asset items), the X-AST-2a report.

**You own:** `design_next/assets/*` and `species_work/asset_origin_map.csv`.

**Task (prompt §8.1 Phase 4):**
1. Replace each manifest row's document-level `consumers` with the requirement ids that use the asset (find each citation of the asset id and take the requirement it sits in; a citation outside a requirement — a table, a diagram — names the nearest requirement of that section, or the document path when there is none). Keep the column name; separate ids with `;`.
2. Add a row for every asset id that a document cites but the manifest lacks, or, where the citation is the defect, list it in your report for the coordinator.
3. Remove rows that nothing consumes, and list them (they move to the "does not need recreating" count of GameAssets.md).
4. Keep GameAssets.md, the README counts and the art briefs consistent with the manifest.
5. Run `check_design.py` (gate 7 must pass) and both pattern scans on your files.

**Output:** the asset files and your report `species_work/reports/X-AST-2b.md`.

## X-TOOLS — Retained gate tooling (prompt §9.4, owner decision O27)

**Inputs:** `species_work/scripts/check_design.py`, `scan_patterns.py`, `mermaid_check.js`, `species_work/forbidden_patterns.tsv`, `species_work/soft_patterns.tsv`, `species_work/ip_tokens.txt`, `species_work/original_strings.txt`, `species_work/scripts/C-TER_generator.py`, `AGENTS.md`, `.github/workflows/build.yml` (the `design` job).

**You own:** `Tools/DesignCheck/` and `Tools/DesignModels/` (both new), and your report.

**Deliver `Tools/DesignCheck/`:**
- `CheckDesign.py`: the structural gates of prompt §7.3 items 1–11, generalised: `python3 Tools/DesignCheck/CheckDesign.py <design root> [--write-index] [--report <file>] [--skip-mermaid]`. It reads the prefix registry from `<root>/CONVENTIONS.md` §3.1 and the allowed terms from §9 (Paths column), so new prefixes and allowed terms need no code change; it finds the vision and prompts folders relative to the design root's parent. It applies the technology and provenance patterns of `TechPatterns.tsv` and the hashed denylists below, and prints the per-gate table; exit status 1 on any failure.
- `TechPatterns.tsv`: the provenance and legacy-technology patterns of §7.1 and the soft-flag patterns of §7.2 in plain text, **with no product, studio, character, map or other IP name in it** (take the rows of the two pattern files whose class is provenance, legacy technology, history, scoped, unfinished or renamed-term; leave every IP row to the denylist).
- `IpDenylist.sha256`: one SHA-256 per normalised token and per n-gram of 1–5 words from `ip_tokens.txt` (normalise: lower case, ASCII-folded, punctuation removed, whitespace collapsed); the scan hashes the n-grams of the scanned text the same way. `OriginalStrings.sha256`: the same for every line of `original_strings.txt` (whole normalised strings), checked against quoted strings of three or more words. The repository never holds the names or strings in plain text.
- `MermaidCheck.js`, `package.json` and a committed `package-lock.json` pinning `mermaid` 11.4.1 and `playwright` 1.56.1 (install with `npm install --prefix Tools/DesignCheck` once to produce the lock file; never commit `node_modules/`). `CheckDesign.py` looks for `Tools/DesignCheck/node_modules/mermaid/dist/mermaid.min.js` and Chromium through Playwright (`PLAYWRIGHT_BROWSERS_PATH` honoured), and fails, never skips, when they are missing unless `--skip-mermaid` is given. The `design` job of `build.yml` runs `npm ci --prefix Tools/DesignCheck`, `npx --prefix Tools/DesignCheck playwright install --with-deps chromium` and `python3 Tools/DesignCheck/CheckDesign.py Design`: match it exactly.
- `SelfTest.py`: proves that the hashed scan finds every token of `ip_tokens.txt` and every line of `original_strings.txt` (pass the two files as arguments; they are deleted at the cutover), and that it finds nothing in a clean sample paragraph made of Species terms.
- `README.md`: usage, the gates, how to extend the patterns and the allowed terms, and how to update the pinned Mermaid build. It says only that the denylist covers "names and text of the reference product and its IP".
- Python and JavaScript file names follow the `AGENTS.md` style (`CheckDesign.py`, `SelfTest.py`, `MermaidCheck.js`); Python is LF, 2-space indent, 120 columns (`.editorconfig`).

**Deliver `Tools/DesignModels/`:** `TerrainGenerator.py` (from `species_work/scripts/C-TER_generator.py`, scanned clean, citing `Design/architecture/components/terrain/`) and `README.md` (what it is: an independent generator written from the terrain specification that reproduces its test values; how to run it; that the design phase adds its quantitative models here).

**Checks:** run `CheckDesign.py design_next` and `species_work/scripts/check_design.py` on the same tree: the gate results must be equal (report both tables); run `SelfTest.py`; run both pattern scans over `Tools/` (the hard scan must print nothing: the denylist must not leak a name).

**Output:** the two folders and `species_work/reports/X-TOOLS.md`.
