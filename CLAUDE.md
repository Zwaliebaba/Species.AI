@AGENTS.md

# CLAUDE.md

`AGENTS.md` (imported above) is the engineering rulebook for this repository: naming, layout, the MSBuild build, the dependency allow-list (R14), ADRs and the checks to run. It applies to every agent and human; this file only adds the entry points.

## Where things are

- **What to build next:** `Design/implementation/mvp_scope.md`, the solo MVP of the world game (owner decisions O50 to O56). It cites the requirements it implements and says where it narrows or defers them; `Design/implementation/mvp_assets.md` lists the assets it needs.
- **The design:** `vision/species-vision-brief.md` (the pitch; owner decisions O1–O30), `Design/decisions/owner_decisions.md` (every owner decision, O1 onwards), `Design/gdd/` (the Species game design: pillars and core loops, world and regions), `Design/architecture/04_solution_strategy.md` (Final) with ADR-079 to ADR-089, then the reference library.
- **The reference library** (`Design/`): `README.md` (reading guide), `CONVENTIONS.md` (ids, units, naming, allowed terms), `REQUIREMENTS_INDEX.md`, `reference/` (reference mechanics), `architecture/components/` (the engine baseline), `ADR/` (architecture decision records, `ADR-NNN-<slug>.md`), `decisions/` (decision register, `DEC-NNN`; an Open entry states the behaviour to build until it is decided), `assets/GameAssets.md` (every asset to create).
- **The paused design run:** `prompts/species-design-phase.md` and its working files in `species_design_work/` (`RUN_LOG.md`, area briefs, challenges).

## Current status (2026-10-18)

- There is no code yet: no solution, no projects, no source.
- The design phase ran wave A and is paused (O51): A0 (pillars, loops, vocabulary) and A1 (world and regions) are Final; A2 (solution strategy, ADR-079 to ADR-088, and ADR-089 for MsQuic) is Final, with owner gate B (O41 to O49) applied; its ADRs stay Proposed until the MVP milestones confirm them. Waves B to E are not run.
- The next step is the MVP: start at milestone M0 of `Design/implementation/mvp_scope.md`.

## Rules that are easy to miss

- Requirement ids (`R-UNIT-014`, `TER-012`), tuning keys, DEC and ADR ids are stable: never renumber or reuse them, and cite them in commits and tests.
- A change under `Design/` passes `python Tools/DesignCheck/CheckDesign.py Design` before it is pushed.
- Design questions go in the decision register; engineering decisions go in `Design/ADR/` (AGENTS.md §6). Ask the owner when neither the design nor a DEC answers a question.
