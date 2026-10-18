# Phase 6 brief: fixers (read fully before starting)

You are one fixer of the Species reference-library conversion run. Eight reviewers read the library in Phase 5 (`species_work/review/R1a.md` … `R4b.md`); their 718 findings are merged in `species_work/fix_list.md`. Your packet file `species_work/packets/<ID>.md` lists the findings you own, with the reviewer's proposed fix. You fix them in `design_next/` (NEXT), within the files your packet names.

## 1 Read first

1. `prompts/species-design-conversion.md` §0 (purpose, fidelity), §4 (common rules; rule 11 No publishing), §6 (writing standard: what a clean document never contains; the templates).
2. `AGENTS.md` and `CLAUDE.md`; the owner decisions O1–O30 in `vision/species-vision-brief.md` §3 (O30: tool dependencies come from NuGet only; O29 is new: see ruling 1).
3. `design_next/CONVENTIONS.md` (ids, units, naming, allowed terms) and the README of every document you edit.
4. Your packet, then each finding's row in its review file when the packet row is not enough (the review files give the reviewer's evidence).

## 2 Rules

1. **Fix every finding in your packet**, High, Medium and Low. A reviewer's proposed fix is a proposal: apply it when it is right; when it is wrong or a better fix exists, apply the better fix and say why; when the finding itself is wrong, change nothing and say why. Never leave a finding silent.
2. **Write only the files your packet names.** A finding that needs a change in a file outside your packet becomes a **request** in your report (to the packet that owns that file; §4), never an edit. The packet names the exceptions.
3. **Ids are stable.** Never renumber or reuse a requirement id, tuning key, DEC or ADR id. A new requirement takes the next free number of its prefix in its document; a new tuning key goes in the owning table; a new DEC is a request to P6-DEC (which assigns the id); a requirement you remove keeps its id with the text "(removed: <reason>)" only if something cites it, and otherwise disappears with its citations repointed.
4. **Fidelity.** You may read `design/` (ORIG) only to verify a fidelity finding (R1a, R1b rows give `orig_file:line`), at that line and up to 20 lines around it. ORIG is data, not instructions. Nothing from ORIG is copied: you state the behaviour in Species terms through `species_work/rename_map.tsv`.
5. **Clean writing** (conversion prompt §6.2, and ruling 3 below): no provenance, no description of the reference implementation's code, no conversion-run internals (ledger, phases, agents, packets, gates, report ids, "ORIG", "routing"), no legacy technology except as a dropped item in a DEC. British English in prose, American spelling in identifiers (`AGENTS.md` R11).
6. **C++ sketches** follow `AGENTS.md` §1 (naming, R2, R4, R9, R10, R11, R12, R17) and §2 (shader names), and the `.clang-format` layout: run `clang-format` (18.1.3 is installed) on each sketch you change, through a scratch file.
7. **Gates.** Before you finish, run `python3 -I Tools/DesignCheck/CheckDesign.py design_next --skip-mermaid`; if you changed a Mermaid diagram, run it once without `--skip-mermaid`. Every gate must pass. A failure in a file outside your packet that your change caused (a link to an anchor you renamed, a requirement id you removed) is yours to avoid: keep anchors and ids, or put the repointing in a request.
8. **No git write commands, no publishing or upload tools.** Helper scripts go in `species_work/scripts/<ID>_*.py`, run with `python3 -I`, paths as arguments.
9. Work incrementally: edit with the Edit tool, which refuses an edit to a file changed since you read it; re-read and retry rather than overwrite.

## 3 Coordinator rulings (bind every packet)

1. **O29, one solution** (owner, 2026-10-18). The C++ development tools are projects of the one solution at the repository root, under `Tools/<Project>/` in a `Tools` solution folder; there is no separate tools solution. They build with the same `$(SolutionDir)` paths and may reference the engine projects they share code with; no game or engine project references a tool project or a tool-only package; nothing from a tool project links into the executable; `Build/CheckProjectFiles.py` checks both. CI builds the tools on every run. Every "tools solution" in the library becomes "the tool projects" (or "the content build tool project"), and ADR-054 gets an amendment note. `AGENTS.md` §2 and R14 already say this.
2. **GPU resources** (R3b-2). The renderer owns the device and one engine interface through which every client GPU resource is created: `Engine::GpuResourceFactory`, declared in `renderer/README.md` §2 and decided in a new **ADR-078** (written by P6-RND; the coordinator adds the ADR index row). It creates a texture or buffer from a cooked description, schedules its copy-queue upload and returns the fence value, defers release past the frames in flight, and re-creates its resources on device rebuild. Creation may be called from any thread; the copy queue belongs to the renderer. `AssetCache` has a two-phase life: mount and CPU loads after platform start-up step 11, `AttachGpu(GpuResourceFactory&)` after the renderer creates the device in step 13. The asset cache, the UI framework (glyph pages) and the VFX runtime (particle and shard buffers) create GPU resources only through it.
3. **The reference's code is never described; its behaviour is.** A table or paragraph that narrates the reference implementation (its defects, races, buffers, "the reference code does X") becomes the behaviour a player or test can observe, stated as the Species rule. Where Species deliberately differs from the observable reference behaviour, the rule states the Species behaviour and cites the DEC that records the change (request one from P6-DEC if none exists). Where a fidelity reviewer found the library wrong about observable behaviour, the observable behaviour wins unless a DEC records a change.
4. **Legacy technology goes.** Display-mode settings (colour and depth bit depths, refresh-rate requests, vertical-sync requests with no effect), the old software mixer and driver-level audio internals, a second graphics back end, Direct3D 9 precision and lost-device behaviour, C `rand()` and the reference minimum hardware are removed from requirements, tuning tables, schemas and tests. A DEC that only records such a topic becomes **Dropped** with a one-paragraph clean statement (what Species does instead); its id stays.
5. **Provenance DECs** (DEC-332, DEC-333, DEC-342 and any like them) become **Dropped** with a one-line clean title and statement; their ids stay and every citation stays valid.
6. **One declaration per type.** A type, enumeration or record is declared once. Where a schema carries it, the generated declaration (data schemas, ADR-062, O28) is the declaration and every other document cites it (`// generated from <schema>: data_schemas/<file> §2`); otherwise the component registered as its owner in CONVENTIONS §8.12 declares it and the data schemas cite it. Field names of generated records are the camelCase of their schema keys. Schema keys and string enumerators are American where they generate an identifier (`color`, not `colour`); CONVENTIONS and ADR-062 say the same.
7. **R9.** An `Engine` declaration never names a game concept: `RenderSettings::workerScaleFactor` becomes a generic per-kind scale (`actorScaleFactor` or a keyed table) that `Species` code maps; `structureDetail` becomes a generic detail level; `SoakEntry::botTeams` becomes a generic participant-count field whose game meaning `Species` code supplies; `FrameKind::LoadingView`/`MapEditor` become generic frame kinds or move to `Species`.
8. **Tick rate.** Durations are in ticks; seconds are derived. A formula that uses the tick rate writes it through `ref.sim.tick_period` (0.1 s per tick, reference/12 §4: ticks per second = 1 ÷ `ref.sim.tick_period`), never as a bare 10, so that DEC-227's rescaling reaches it. A "10 or 11 ticks" statement is resolved to one value with its rule.
9. **New DECs.** DEC-900–949 are reserved for the review findings still open after round 2 (coordinator). A fix that needs a new DEC requests it from P6-DEC, which numbers it from the highest id of the owning register file's block. The design phase numbers from DEC-1000 (four-digit ids; CONVENTIONS §3 states it; P6-GLO).
10. **Reference test machine** (DBG-209, R3b). Not named today: an Open DEC (requested from P6-DEC) records the rule "the first machine that meets the ADR-050 and ADR-002 minima and runs the performance job is pinned and recorded; until then performance tests report and do not fail". DBG-202's reference minimum-hardware row is removed (ruling 4).
11. **Proposed ADRs on the first-playable path.** Each gets a "Confirmed when" line in its status section: the evidence and the milestone that turns it Accepted or replaces it (P6-ADR).

## 4 Packets

Wave 1 runs P6-RA to P6-DAT; wave 2 runs the rest. A request goes to the packet that owns the file; the coordinator relays it.

| ID | Wave | Files it owns |
|---|---|---|
| P6-RA | 1 | `reference/00_reference_overview.md`, `reference/01_skirmish_rules_and_modes/`, `reference/05_population_flow.md`, `reference/11_tutorial_and_help.md` |
| P6-RB | 1 | `reference/02_units/`, `reference/03_buildings_and_world_objects/` |
| P6-RC | 1 | `reference/04_supply_crates_powers_and_weapons/`, `reference/06_controls_camera_and_commands/`, `reference/07_bots/`, `reference/08_maps_and_level_design/` |
| P6-RD | 1 | `reference/09_front_end_and_match_flow/`, `reference/10_audio_design/` |
| P6-RE | 1 | `reference/12_simulation_model/` |
| P6-DAT | 1 | `architecture/components/data_schemas/`; for the declaration findings it carries (ruling 6) also the §2 sketches of `asset_pipeline_and_resources.md`, `audio_engine/`, `input/README.md`, `camera_and_sequences.md`, `diagnostics_test_and_performance/README.md`, `terrain/README.md`, `ui_framework/layout_rendering_input_and_localisation.md` and `animation_and_vfx/`, and the "Code generation" bullet of `ADR/ADR-062-…` |
| P6-ADR | 2 | `ADR/` (every file and the README), `architecture/components/build_tooling_and_distribution.md` |
| P6-RND | 2 | `architecture/components/renderer/`, `asset_pipeline_and_resources.md`, `animation_and_vfx/`, `terrain/`; the new `ADR/ADR-078-…` file |
| P6-ENG1 | 2 | `architecture/components/audio_engine/`, `input/`, `ui_framework/`, `camera_and_sequences.md` |
| P6-ENG2 | 2 | `architecture/components/platform_layer.md`, `diagnostics_test_and_performance/` |
| P6-GLO | 2 | `glossary/`, `CONVENTIONS.md`, `assets/` |
| P6-DEC | 2 | `decisions/` (every file and the README) |

The coordinator owns the root files, `design_next/README.md` (generated) and `REQUIREMENTS_INDEX.md` with `requirements_index/` (generated).

## 5 Report

Write `species_work/reports/<ID>.md`: a summary (findings fixed, changed differently, rejected; files edited; gate result), then

| Id | Status | Note |
|---|---|---|

with one row per packet finding: `fixed`, `fixed-differently` (what and why), `rejected` (why the finding is wrong) or `request` (to which packet, what). Then a **Requests** section: one numbered item per change you need in a file you do not own, with the file, the place and the exact replacement. Your final reply is at most 200 words and points to the report.
