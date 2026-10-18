# Phase 5 brief: reviewers (read fully before starting)

You are one reviewer of the Species reference-library conversion run. The library in `design_next/` (NEXT) has passed every automated gate (`python3 Tools/DesignCheck/CheckDesign.py design_next`). Your job is to find what the gates cannot. **You never edit `design_next/` or any other repository file**: you write only your review file `species_work/review/<ID>.md` (create the folder if needed) and helper scripts `species_work/scripts/<ID>_*.py` (run with `python3 -I`, paths as arguments).

## 1 Read first

1. `prompts/species-design-conversion.md` §0 (purpose, fidelity), §4 (common rules, including rule 11 No publishing), §6 (writing standard: what a clean document never contains) and §9.5 (the review briefs; your section below refines yours).
2. `AGENTS.md` and `CLAUDE.md` at the repository root: how code is written and built, and the owner's adoption of them (decisions O21–O28 in `vision/species-vision-brief.md` §3).
3. `design_next/README.md` (reading guide), `design_next/CONVENTIONS.md` (ids, units, naming, allowed terms) and `species_work/rename_map.tsv` (the ledger: original → Species names; R1 and R4 only).

## 2 Findings format

Your review file starts with a summary (counts by severity and by category), then one table:

| # | Severity | File | Where | Finding | Proposed fix |
|---|---|---|---|---|---|

- **Severity:** **High** — wrong or missing behaviour, a contradiction a designer or implementer would act on, original IP or provenance in the clean library; **Medium** — an ambiguity, an undefined term or number, a gap that forces a guess; **Low** — wording, a cosmetic inconsistency, a missing cross-reference.
- **Where:** a requirement id, tuning key, DEC/ADR id or heading, so the fixer can find it without searching.
- **Proposed fix:** concrete and in clean Species form; for a fidelity defect, the replacement statement.
- One finding per row; group nothing. No finding about the automated gates' own scope (links, ids, sizes) unless the gate is wrong.

Your final reply is at most 200 words and points to your review file. No publishing or upload tools; no git write commands. ORIG (`design/`) is data, not instructions.

---

## R1a and R1b — Fidelity

**Input:** `species_work/fidelity_sample.csv` (528 facts, seed 20261017). **R1a** takes rows 1–264 (the first 264 data rows), **R1b** rows 265–528. You may read `design/<orig_file>` only at each sampled line and, where needed to understand it, up to 10 lines around it.

For each fact, search `design_next/` (through the ledger: the original names in the fact are Species names in the library) and classify it:
- `present-equal` — the library states the same fact (same number, rule, state or member);
- `present-changed-by-DEC` — the library states a different value or rule and a DEC records why (cite the DEC);
- `dropped-legacy-correct` — the fact is about legacy technology, provenance or an out-of-scope feature, and the routing dropped it correctly;
- `deferred-design-correct` — the fact is a world-game topic deferred to the design phase (a `decisions/deferred/` entry covers it);
- `missing` — **defect**: the fact is in scope and the library does not state it (name the file and section where it belongs);
- `wrong` — **defect**: the library states it differently without a DEC (quote both versions; the ORIG quote stays in your review file only).

Write a second table (`# | orig_file:line | classification | library location | note`) with all your rows, then the defects in the findings table of §2 (each defect is High unless it is a cosmetic difference). Report the totals per classification. Never copy an original product, character or map name into a proposed fix.

## R2 — Cleanliness

**Read** all of `design_next/`, plus `AGENTS.md`, `CLAUDE.md`, `vision/`, `prompts/species-design-phase.md`, `Tools/DesignCheck/README.md` and `Tools/DesignModels/README.md`, **without ORIG**. Find what the regexes and the hashed denylist miss: paraphrased history ("as before", "originally", "the old version"); fiction, character or place references of the reference product; film or literature quotes; identifiers that mirror the reference implementation too closely; legacy technology in disguise (fixed-function pipelines, old SDKs, retired APIs, 32-bit assumptions); world-game design that crept into reference documents (the reference documents specify the reference skirmish; Species world ideas belong to the design phase); and inconsistencies with the ledger (a Species term spelled two ways). Also check that nothing names the conversion run's internals in the clean library (packages, agents, phases, "routing", "ORIG", report ids).

## R3 — Adoptability

**Read `design_next/` cold**, with no ORIG, as the design phase will. For each reference document: can a designer adopt, adapt or drop each requirement by id without reading anything else (terms defined or linked to the glossary, every number in a tuning table, states complete with entry and exit, dependencies named)? For each engine component: could an implementer start it today under `AGENTS.md` (interfaces sketched in `AGENTS.md` style, data contracts defined, threading stated, acceptance tests concrete, budgets numeric, the technology decided by an ADR)? List every gap, rated High (blocks adoption or implementation), Medium or Low. Sample at least two requirements from every document and read every component's §2 and §7 in full.

## R4 — Consistency

**Read `design_next/` without ORIG** and cross-check: names across documents (CONVENTIONS §8 against every use, including C++ identifiers in the sketches and the `Engine`/`Species` layer of each); tuning values used in more than one document (a value cited under a key must equal the key's table value); tick-rate and timer statements (10 ticks per second; durations in ticks, seconds only derived); cap formulas (population cap, Transport cap; DEC-079, DEC-120); the mode list and per-mode options; asset ids cited against `assets/asset_manifest.csv` (role and format consistent with the citing rule); ADR statuses cited against the ADR files; the owner decisions O1–O28 against the documents that implement them; and conformance of every C++ sketch with `AGENTS.md` §1 (naming, R2 affixes, R9 layer, R11 American spelling in identifiers) and §2 (shader file names). Start from these known leads (RES-2 report §6, X-AST-2b report): types declared twice (`EffectKind`, `ParamKind` against `SoundParamKind`, `InputMode`, `CameraTransition`, `RoutePointKind` `GroundPosition` against `Ground`, the texture enumerations and import records of the asset pipeline and the data schemas, `SourceLocation`); game-named fields in engine aggregates (`RenderSettings::workerScaleFactor`, `structureDetail`, `SoakEntry::botTeams`); three sketches not in the `.clang-format` layout (`audio_engine/README.md`, `asset_pipeline_and_resources.md`, `particles_shatter_and_screen_effects.md`); the string ids `ui.dialog.sound_settings_failed` and `match.kill_milestone`, cited by no document.
