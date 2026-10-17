# Synthesis brief — Phase 3 (read fully before starting)

You are a Phase-3 synthesis agent. Phase-2 agents have produced draft sections for the Multiwinia design-recovery set. Your job is to turn the drafts routed to your document(s) into the **final** document(s), resolving contradictions by **re-reading the source code** — never by picking one draft over another.

## Inputs

- `SRC=/home/user/gene9/darwinia-and-multiwinia-source-code/multiwinia` (read-only source tree; commit `f909630`).
- `OUT=/home/user/Species.AI/design`.
- Rules that still apply in full: `OUT/_work/AGENT_BRIEF.md` (evidence tagging [C]/[I]/[U], `path:Symbol` citations, build tags, INTERNAL_STUDY excerpt rule ≤15 lines with the exact "Verbatim excerpt" marker, no silent truncation, IP register, security hygiene, incremental writing). Read it.
- Build reality: `OUT/_work/build_matrix.md` (note: `INCLUDE_THEBEAST` is **debug-only**).
- Routed draft sections: `OUT/_work/routed/<NN>.md` — every draft section whose heading was routed to document NN (each chunk starts with `<!-- from AGENT -->`). Also `OUT/_work/routed/_meta_*.md` (cross-partition notes, contradictions, unknowns, docs-vs-code discrepancies, IP registers, appendices, constants, determinism notes from every agent) — grep these for anything relevant to your document.
- Full drafts (if you need context around a routed section): `OUT/_work/drafts/<AGENT>.md`; generated tables `OUT/_work/drafts/*.csv`; helper scripts `OUT/_work/scripts/`.
- Coordinator's summary of every agent's key findings: `OUT/_work/phase2_log.md`.
- The original specification (§5 defines what each document must contain; §6 acceptance criteria): `/tmp/claude-0/-home-user-Species-AI/f64da416-5c15-5870-ada9-978a3bb49e15/scratchpad/original_prompt.md`.

## Output — the final document `OUT/<NN>_<name>.md`

Required skeleton:
```
# NN — <Title>

> Part A/B/C · Baseline build: Release|Win32 (OpenGL) + TARGET_PC_FULLGAME ([B:FULL]); see _work/build_matrix.md.
> Evidence: [C] confirmed · [I] inferred · [U] unknown. Citations are `path:Symbol` relative to the multiwinia/ source root.
> Sources merged: <list of drafts>.

## Contents
## 1 … (body — follow the §5 template for this document exactly, every bullet of it)
…
## Open questions and unknowns   (every [U] and every design-critical [I] in this document, numbered NN-U1, NN-U2 …, each with what would resolve it)
## Docs-vs-code discrepancies   (table; may be empty with the sentence "None found.")
## IP items                     (table: item, kind, ref, neutral placeholder)
## Source map                   (table: every source/data file that implements or feeds this document's subject, with its role — one row per file)
## Appendix — dead/legacy code  (brief, build-tagged)
```

Rules for the final text:
1. **Completeness over brevity.** The audience is a team building a new game (Windows, C++23, DirectX 12, multiplayer) who will not read the original source. Keep every table, constant, formula, state machine and enumeration the drafts contain unless you prove it wrong. Do not summarise enumerations. Never write "etc.", "and others", "various", "similar to above".
2. **Contradictions.** Where drafts disagree (with each other, with `phase2_log.md`, or with the build matrix), open the code, determine the truth, write it, and note the resolution briefly in place ("Resolved: drafts disagreed on X; code shows …"). If you cannot resolve it, it becomes an [U] item.
3. **Spot-verify.** Before writing a [C] claim that came from a draft, verify a sample of them against the code yourself (at least the numeric constants and state transitions in each major section). Downgrade to [I] anything you cannot confirm.
4. **Citations** stay in `path:Symbol` form with paths starting `code/`, `data/`, `contrib/`, `tools/`, `targets/`, `docs/` (the coverage checker parses them). Every file in your Source map must exist in `OUT/_work/manifest.csv`.
5. **Cross-references** to other final documents use relative links, e.g. `[21 §3](21_simulation_and_entity_model.md#3-location)`. The full document set: 00_overview, 01_game_modes, 02_units_and_entities, 03_buildings_and_world_objects, 04_crates_powerups_and_weapons, 05_economy_and_population, 06_player_control_and_input, 07_ai, 08_levels_and_level_design, 09_meta_systems, 10_audio_design, 20_architecture, 21_simulation_and_entity_model, 22_determinism_and_networking, 23_terrain_and_landscape, 24_rendering, 25_textures_and_assets, 26_animation_and_vfx, 27_audio_engine, 28_ui_framework, 29_data_formats_reference, 30_scripting_tutorial_and_cutscenes, 31_platform_build_and_tools, 32_debug_test_and_performance, 40_carry_forward_assessment, 41_risks_and_open_questions, 90_glossary, 91_cross_reference_index, 99_coverage_matrix.
6. **Mermaid** for class hierarchies, state machines, sequence diagrams and pipelines (keep diagrams syntactically valid: quote labels with spaces/brackets, no parentheses in node ids). Tables for any enumerable set.
7. **Large catalogues** (asset tables, event tables, binding tables) may be generated from the `_work/drafts/*.csv` files with a script in `OUT/_work/scripts/S_<NN>_*.py` (run with `python3 -I`) and pasted/inserted into the document; keep the script.
8. **Excerpts:** INTERNAL_STUDY — keep verbatim excerpts rare, ≤15 lines, with the exact marker line `> **Verbatim excerpt** — \`path:Symbol\` (Introversion licence, ≤15 lines)`.
9. Write incrementally (skeleton first, then fill sections), because your context may be compacted.
10. Do not modify drafts, other agents' documents, or anything in `SRC`. Do not git commit/push.


## Mandatory pre-finish checks
1. **Mermaid syntax** — run and fix until your document reports 0 failed blocks:
   `NODE_PATH=/opt/node-tools/node_modules node /home/user/Species.AI/design/_work/scripts/mermaid_check.js /home/user/Species.AI/design /tmp/claude-0/-home-user-Species-AI/f64da416-5c15-5870-ada9-978a3bb49e15/scratchpad/mermaidcheck/node_modules/mermaid/dist/mermaid.min.js 2>&1 | grep -E "YOUR_DOC|blocks="`
   Common causes: `:` / `(` / `"` / `→` / `<` inside state-diagram transition labels or state names (use plain words, or `state "Label with: chars" as Id`), unquoted special characters in sequence-diagram messages, `;` and `#` in labels, node ids containing spaces or punctuation.
2. **Citations** — `python3 -I /home/user/Species.AI/design/_work/scripts/coverage_check.py /home/user/gene9/darwinia-and-multiwinia-source-code/multiwinia /home/user/Species.AI/design` then read `_work/coverage_report.md` section "Cited paths that do not exist" and fix any entries attributed to your document (other sections of the report concern the whole set; ignore them).

## Final reply (≤ 200 words)
Document(s) written with size; number of contradictions resolved; [U] count; any §5 bullet you could not satisfy and why.
