@AGENTS.md

# CLAUDE.md

`AGENTS.md` (imported above) is the engineering rulebook for this repository: naming, layout, the MSBuild build, the dependency allow-list (R14), ADRs and the checks to run. It applies to every agent and human; this file only adds the entry points.

## Where things are

- **What to build:** `vision/species-vision-brief.md` (the pitch and the owner's decisions O1–O30), then the Species game design once the design phase has written it, then the reference library in `Design/`.
- **The reference library** (`Design/`): `README.md` (reading guide), `CONVENTIONS.md` (ids, units, naming, allowed terms), `REQUIREMENTS_INDEX.md`, `reference/` (reference mechanics), `architecture/components/` (the engine baseline), `ADR/` (architecture decision records, `ADR-NNN-<slug>.md`), `decisions/` (decision register, `DEC-NNN`; an Open entry states the behaviour to build until it is decided), `assets/GameAssets.md` (every asset to create).
- **The next run:** `prompts/species-design-phase.md` turns the library and the vision into the Species game design.

## Current status (2026-10-17)

- There is no code yet: no solution, no projects, no source.
- The reference library is still being built by the conversion run (Phase 5, review; every automated gate passes). Until its cutover it lives in `design_next/`, beside the source corpus `design/` and the working folder `species_work/`; read `design_next/` wherever `AGENTS.md` or this file says `Design/`. The cutover renames it to `Design/` and removes the other two.

## Rules that are easy to miss

- Requirement ids (`R-UNIT-014`, `TER-012`), tuning keys, DEC and ADR ids are stable: never renumber or reuse them, and cite them in commits and tests.
- A change under `Design/` passes `python Tools/DesignCheck/CheckDesign.py Design` before it is pushed.
- Design questions go in the decision register; engineering decisions go in `Design/ADR/` (AGENTS.md §6). Ask the owner when neither the design nor a DEC answers a question.
