@AGENTS.md

# CLAUDE.md

`AGENTS.md` (imported above) is the engineering rulebook for this repository: naming, layout, the MSBuild build, the dependency allow-list (R14), ADRs and the checks to run. It applies to every agent and human; this file only adds the entry points.

## Where things are

- **What to build next:** `Design/implementation/mvp_scope.md`, the solo MVP of the world game (owner decisions O50 to O64). It cites the requirements it implements and says where it narrows or defers them; `Design/implementation/mvp_assets.md` lists the assets it needs.
- **The design:** `vision/species-vision-brief.md` (the pitch; owner decisions O1–O30), `Design/decisions/owner_decisions.md` (every owner decision, O1 onwards), `Design/gdd/` (the Species game design: pillars and core loops, world and regions), `Design/architecture/04_solution_strategy.md` (Final) with ADR-079 to ADR-089, then the reference library.
- **The reference library** (`Design/`): `README.md` (reading guide), `CONVENTIONS.md` (ids, units, naming, allowed terms), `REQUIREMENTS_INDEX.md`, `reference/` (reference mechanics), `architecture/components/` (the engine baseline), `ADR/` (architecture decision records, `ADR-NNN-<slug>.md`), `decisions/` (decision register, `DEC-NNN`; an Open entry states the behaviour to build until it is decided), `assets/GameAssets.md` (every asset to create).
- **The paused design run:** `prompts/species-design-phase.md` and its working files in `species_design_work/` (`RUN_LOG.md`, area briefs, challenges).

## Current status (2026-10-20)

- The solution of ADR-090 (`Species.slnx`: six libraries, two programs, six test projects and the tool project `Tools/SpeciesSchemaGen`, each built for x64 and, as the development build of O66 (ADR-094), for ARM64) holds M0's code: the platform basics, diagnostics and transport in NeuronCore; the window, device and swap chain in NeuronClient; the server host, tick loop, command queue and snapshot store in NeuronServer; the world's owners, the world server, the integrated server, the state hash, the world snapshot and the client session in the three Game libraries; and the client's and the headless server's composition roots. `GameData/` is the content root (O61): the owner's first assets (two bitmap fonts, 46 textures in a folder per kind and two CMO meshes; O64, `Design/implementation/mvp_assets.md` §6) and the tuning record `Records/tuning/mvp.json` with its schema in `Schemas/` (ADR-098). `playtests/` holds the format of the MVP's playtest evidence (MVP scope §6.3).
- The design phase ran wave A and is paused (O51): A0 (pillars, loops, vocabulary) and A1 (world and regions) are Final; A2 (solution strategy, ADR-079 to ADR-088, and ADR-089 for MsQuic) is Final, with owner gate B (O41 to O49) applied; its ADRs stay Proposed until the MVP milestones confirm them. Waves B to E are not run.
- The MVP's milestone M0 is in review: `Design/implementation/m0_plan.md`, T1 to T5 merged, T6 to T12 in one pull request (O67) with ADR-095 to ADR-100, in the project layout of ADR-090 (O60). M0 ends when the owner's run checklist passes on that pull request: the tick count rising in the client's title at 10 per second, a tuning change that changes it, and the headless server running and freezing. Code is written here and compiled by CI; the owner runs it on their PC (m0_plan §2).

## Rules that are easy to miss

- Requirement ids (`R-UNIT-014`, `TER-012`), tuning keys, DEC and ADR ids are stable: never renumber or reuse them, and cite them in commits and tests.
- A change under `Design/` passes `python Tools/DesignCheck/CheckDesign.py Design` before it is pushed.
- Design questions go in the decision register; engineering decisions go in `Design/ADR/` (AGENTS.md §6). Ask the owner when neither the design nor a DEC answers a question.
