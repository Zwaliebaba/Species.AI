# Conventions

> Status: Draft (Phase 0). The naming section is completed by the rename-ledger pass, and the allowed-terms list grows during integration.

These conventions apply to every document in this design set and to every later change to it. `tools/design-check/` enforces them.

## 1 What this set is

This set is the **Species reference library**. It is a clean, faithful specification of the reference game's mechanics, look and feel, and engine behaviour, with the engine retargeted to Windows, C++23 and Direct3D 12. It is **not** the Species game design. The Species design phase adopts, adapts or drops each reference requirement by ID. See `vision/species-vision-brief.md`.

## 2 Normative language

Requirement keywords follow RFC 2119 and RFC 8174: **MUST**, **MUST NOT**, **SHOULD**, **SHOULD NOT**, **MAY**. They are written in capitals only when normative.

Documents are written in the present tense and describe what a mechanic or component does and must do. They never describe history, earlier versions or how something was found out.

## 3 Requirement IDs

Every normative statement is a requirement with a stable ID, written as `**<PREFIX>-<NNN>** (MUST) <statement>`. Numbers start at 001 within each prefix and are never reused.

### 3.1 Prefix registry

| Prefix | Scope | Document |
|---|---|---|
| `R-VIS` | Reference overview and core loop | `reference/00_reference_overview.md` |
| `R-MODE` | Skirmish match rules and modes | `reference/01_skirmish_rules_and_modes/` |
| `R-UNIT` | Units, creatures, hazards, projectiles | `reference/02_units/` |
| `R-BLDG` | Buildings and world objects | `reference/03_buildings_and_world_objects/` |
| `R-CRATE` | Supply crates, powers and weapons | `reference/04_supply_crates_powers_and_weapons.md` |
| `R-ECON` | Population flow | `reference/05_population_flow.md` |
| `R-CTRL` | Controls, camera and commands | `reference/06_controls_camera_and_commands.md` |
| `R-BOT` | Bots | `reference/07_bots.md` |
| `R-MAP` | Maps and level design | `reference/08_maps_and_level_design.md` |
| `R-META` | Front end and match flow | `reference/09_front_end_and_match_flow.md` |
| `R-AUD` | Audio design | `reference/10_audio_design.md` |
| `R-TUT` | Tutorial and help | `reference/11_tutorial_and_help.md` |
| `R-SIM` | Simulation model | `reference/12_simulation_model.md` |
| `TER` | Terrain | `architecture/components/terrain.md` |
| `RND` | Renderer | `architecture/components/renderer/` |
| `AST` | Asset pipeline and resources | `architecture/components/asset_pipeline_and_resources.md` |
| `VFX` | Animation and VFX | `architecture/components/animation_and_vfx.md` |
| `AEN` | Audio engine | `architecture/components/audio_engine.md` |
| `UI` | UI framework | `architecture/components/ui_framework.md` |
| `INP` | Input | `architecture/components/input.md` |
| `CAM` | Camera and sequences | `architecture/components/camera_and_sequences.md` |
| `DAT` | Data schemas | `architecture/components/data_schemas.md` |
| `PLT` | Platform layer | `architecture/components/platform_layer.md` |
| `BLD` | Build tooling and distribution | `architecture/components/build_tooling_and_distribution.md` |
| `DBG` | Diagnostics, test and performance | `architecture/components/diagnostics_test_and_performance.md` |

Decisions use `DEC-NNN` (`decisions/decision_register.md`), and architecture decisions use `ADR-NNNN` (`adr/`).

## 4 Tuning tables

Every reference document has a **Tuning table** with the columns `key | value | unit | range or notes | requirement | DEC`. Keys are stable and dotted, for example `ref.unit.worker.move_speed`. Every number a rule uses appears exactly once in some tuning table; rules refer to the key and may repeat the value for readability.

## 5 Units

- Distance is in world units (`wu`).
- Every simulation duration is in **ticks**. The reference tick rate is defined once, in `reference/12_simulation_model.md`. Seconds appear only as a derived display value next to a tick count.
- Rates are given per tick, or per second derived from ticks, and say which.

## 6 Content rules

A document in this set never contains:

- names of the reference product, its studio, characters, places, maps, user-interface text or code identifiers. Use the Species vocabulary in §8;
- references to source files, line numbers, evidence or build tags, verbatim excerpts, source maps, coverage figures or review statistics;
- history or recovery narrative;
- names of superseded technology, even as a contrast. Specify the Species technology positively; the rationale belongs in the ADR;
- original player-facing text. Write new text, keyed by Species string ids such as `ui.lobby.ready`;
- the reference's numeric type ids, message ids, magic numbers or wire layouts. Species defines its own enumerations, and keeps the order of enumerations whose order carries meaning;
- truncation words: "etc.", "and so on", "and others", "various", "similar to the above", or "…" standing in for list members.

Documents may say "the reference game" or "the reference" where the context needs it.

## 7 Templates and diagrams

- Reference documents use the reference template: Purpose and player experience · Rules · States and flows · Tuning table · Edge cases and interactions · Acceptance criteria · Dependencies · Open decisions.
- Engine components use the component template: Responsibilities and boundaries · Interfaces · Data model · Behaviour and algorithms · Concurrency and performance budget · Failure modes, diagnostics and telemetry · Acceptance tests · Open decisions (including "Design-phase inputs").
- ADRs use MADR 4. Decision-register entries use the format in `decisions/decision_register.md`.
- Diagrams are Mermaid only, validated, with a caption line, using Species names only. Component diagrams may use `C4Component`.
- No file is larger than 120 KB. A larger topic becomes a folder with a `README.md` index.
- The language is British English (en-GB).

## 8 Naming

*(Completed by the rename-ledger pass: Species term · kind · identifier · string-id prefix.)*

## 9 Allowed terms

Soft-flagged words that are allowed in a specific context, with the justification.

| Term | Where allowed | Justification |
|---|---|---|
| *(none yet)* | | |
