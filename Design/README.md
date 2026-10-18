# Species reference library

> Status: Reference library, input to the Species design phase · Related: [vision brief](../vision/species-vision-brief.md) · [design-phase prompt](../prompts/species-design-phase.md) · [CONVENTIONS](CONVENTIONS.md) · [Requirements index](REQUIREMENTS_INDEX.md)

This library is the **clean reference** that the Species design phase builds on. It is **not the Species game design.** It specifies, faithfully and in Species terms, the mechanics, numbers, states, look and feel of a reference crowd-battle skirmish (`reference/`), and the engine baseline that implements them on Windows and Direct3D 12 (`architecture/components/` and `ADR/`). The design phase, run with `prompts/species-design-phase.md`, adopts, adapts or drops each requirement by id to design Species: the persistent world of settlements described in the [vision brief](../vision/species-vision-brief.md), whose owner decisions O1–O30 bind every document here.

**The Species design so far** sits beside the library. The design phase ran wave A and is paused (owner decision O51): the [game design](gdd/README.md) holds the pillars, core loops and vocabulary and the world and regions, the [solution strategy](architecture/04_solution_strategy.md) and ADR-079 onward hold the architecture, [owner decisions](decisions/owner_decisions.md) records every owner decision from O1, and [implementation](implementation/README.md) holds the MVP scope and its assets, which is what is built next (O50 to O55). The document map below counts the reference library only.

How the code is written and built (naming, the MSBuild solution, the dependency allow-list, ADR rules) is decided by `AGENTS.md` at the repository root; where a baseline ADR and `AGENTS.md` disagree, `AGENTS.md` wins.

## Reading guide

1. **[CONVENTIONS](CONVENTIONS.md)**: normative language, the id scheme and prefix registry, tuning tables, units (world units `wu` and ticks), naming and the allowed terms.
2. **[Reference overview](reference/00_reference_overview.md)**: the core loop and the vocabulary; then the **[simulation model](reference/12_simulation_model/README.md)** that every other reference document relies on (tick, identity, update order, randomness).
3. **The match**: [skirmish rules and modes](reference/01_skirmish_rules_and_modes/README.md), [units](reference/02_units/README.md), [buildings and world objects](reference/03_buildings_and_world_objects/README.md), [supply crates, powers and weapons](reference/04_supply_crates_powers_and_weapons/README.md), [population flow](reference/05_population_flow.md); then control, bots, maps, front end, audio and the tutorial ([reference index](reference/README.md)).
4. **The engine baseline**: the [component index](architecture/components/README.md) and the [ADR index and technology baseline](ADR/README.md).
5. **Open questions**: the [decision register](decisions/README.md). An Open entry states the behaviour every document specifies today, so implementation never blocks on it; the [deferred decisions](decisions/deferred/README.md) are the world-game questions the design phase answers first.
6. **Assets**: [GameAssets.md](assets/GameAssets.md) lists every asset to create, with its Species file name, format, specification and priority; the [manifest](assets/asset_manifest.csv) is its machine-readable form.

## Document map

221 documents, 8650 KB in total. Sizes include every file of a folder.

| Area | Files | Size |
|---|---|---|
| [Reference mechanics](reference/README.md) | 61 | 3202 KB |
| [Engine components](architecture/components/README.md) | 42 | 2002 KB |
| [Architecture decision records](ADR/README.md) | 66 | 531 KB |
| [Decision register](decisions/README.md) | 16 | 598 KB |
| [Assets](assets/README.md) | 4 | 934 KB |
| [Glossary](glossary/README.md) | 3 | 200 KB |
| [Requirements index pages](requirements_index/README.md) | 26 | 1093 KB |

### Reference mechanics

| Document | Files | Size |
|---|---|---|
| [Reference overview](reference/00_reference_overview.md) | 1 | 96 KB |
| [Skirmish rules and modes](reference/01_skirmish_rules_and_modes/README.md) | 7 | 195 KB |
| [Units](reference/02_units/README.md) | 10 | 558 KB |
| [Buildings and world objects](reference/03_buildings_and_world_objects/README.md) | 5 | 383 KB |
| [Supply crates, powers and weapons](reference/04_supply_crates_powers_and_weapons/README.md) | 3 | 234 KB |
| [Population flow](reference/05_population_flow.md) | 1 | 116 KB |
| [Controls, camera and commands](reference/06_controls_camera_and_commands/README.md) | 5 | 217 KB |
| [Bots](reference/07_bots/README.md) | 6 | 174 KB |
| [Maps and level design](reference/08_maps_and_level_design/README.md) | 3 | 205 KB |
| [Front end and match flow](reference/09_front_end_and_match_flow/README.md) | 4 | 263 KB |
| [Audio design](reference/10_audio_design/README.md) | 6 | 205 KB |
| [Tutorial and help](reference/11_tutorial_and_help.md) | 1 | 108 KB |
| [Simulation model](reference/12_simulation_model/README.md) | 8 | 445 KB |

### Engine components

| Component | Files | Size |
|---|---|---|
| [Animation and VFX](architecture/components/animation_and_vfx/README.md) | 3 | 167 KB |
| [Asset pipeline and resources](architecture/components/asset_pipeline_and_resources.md) | 1 | 119 KB |
| [Audio engine](architecture/components/audio_engine/README.md) | 3 | 112 KB |
| [Build tooling and distribution](architecture/components/build_tooling_and_distribution.md) | 1 | 120 KB |
| [Camera and sequences](architecture/components/camera_and_sequences.md) | 1 | 76 KB |
| [Data schemas](architecture/components/data_schemas/README.md) | 7 | 268 KB |
| [Diagnostics, test and performance](architecture/components/diagnostics_test_and_performance/README.md) | 5 | 143 KB |
| [Input](architecture/components/input/README.md) | 4 | 126 KB |
| [Platform layer](architecture/components/platform_layer.md) | 1 | 118 KB |
| [Renderer](architecture/components/renderer/README.md) | 8 | 334 KB |
| [Terrain](architecture/components/terrain/README.md) | 3 | 121 KB |
| [UI framework](architecture/components/ui_framework/README.md) | 4 | 296 KB |

## Ids in brief

| Id | Form | Where | Count |
|---|---|---|---|
| Requirement | `R-UNIT-014` (reference), `TER-012` (component): prefix registry in [CONVENTIONS](CONVENTIONS.md) §3.1 | `**ID** (MUST) …` in the owning document; [index](REQUIREMENTS_INDEX.md) | 4928 |
| Tuning key | `ref.unit.worker.move_speed`, `ter.generator.octaves` | the tuning table of the owning document | 3680 |
| Decision | `DEC-NNN`, Open, Resolved or Dropped | [decisions/](decisions/README.md) | 302 (148 Open, 126 Resolved, 28 Dropped) |
| Architecture decision | `ADR-NNN` (MADR 4) | [ADR/](ADR/README.md) | 65 (47 Accepted, 18 Proposed, 0 Rejected) |
| Asset | `sprite.worker.front`, `sfx.unit.builder.presence_loop` | [assets/asset_manifest.csv](assets/asset_manifest.csv) | 432 ids, 1391 files |
| String id | `ui.lobby.ready`, `unit.worker.name` | the string tables of [assets/](assets/GameAssets.md) | listed in GameAssets §18.1 |

Ids are stable: they are never renumbered or reused, and the design phase cites them when it adopts, adapts or drops a rule.

## Status

The design gates (`python3 Tools/DesignCheck/CheckDesign.py Design` from the repository root) pass on this library:

| Gate | Result | Failures | Summary |
|---|---|---|---|
| 1 Hard-fail patterns | PASS | 0 | 0 hard-fail hits |
| 2 Soft-flag patterns | PASS | 0 | 39 soft hits, 39 allowed by CONVENTIONS §9, 0 not allowed |
| 3 Links and anchors | PASS | 0 | 13568 relative links checked |
| 4 Mermaid | PASS | 0 | blocks=266 failed=0 |
| 5 Requirement ids | PASS | 0 | 4928 requirement ids defined, 29254 citations, 25 prefixes registered |
| 6 DEC and ADR references | PASS | 0 | 302 DECs (148 Open, 148 of them cited by a requirement), 65 ADRs |
| 7 Asset closure | PASS | 0 | 432 asset ids (1391 manifest rows), 2530 asset citations in reference and component documents |
| 8 Tuning keys | PASS | 0 | 3680 tuning keys, 0 cited by no requirement |
| 9 Truncation words | PASS | 0 | 0 truncation words |
| 10 Sizes, READMEs, templates | PASS | 0 | 221 documents, 83 checked for template headings |
| 11 Requirements index | PASS | 0 | index not written (no --write-index) |
