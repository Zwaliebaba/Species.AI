# Phase 4 brief: integration agents (read fully before starting)

You are one integration agent of the Species reference-library conversion run. Phase 3 wrote every reference document and engine component into `design_next/` (NEXT). Phase 4 closes the gaps that Phase 3 found: decisions, ADRs, conflicts between documents, cross-document requests and assets. Your section below names your inputs, the files you may write and your outputs.

## 1 Read first

1. `/home/user/Species.AI/prompts/species-design-conversion.md` §0, §2, **§4 (common rules, non-negotiable, including rule 11 No publishing)**, **§6 (writing standard)** and §7.1–§7.2.
2. `design_next/CONVENTIONS.md` (prefix registry §3, tuning tables §4, units §5, templates and the DEC entry format §7, naming §8, allowed terms §9) and `species_work/rename_map.tsv` (the ledger).
3. `design_next/decisions/README.md` (index of every DEC) and `design_next/adr/README.md` (ADR index and technology baseline).
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

**You own:** `design_next/adr/*.md` and `design_next/adr/README.md` (index and technology baseline).

**Task:** amend each ADR so that it agrees with the component that implements it, or, where the component is wrong, record the component fix in your report for the document fixers. Keep MADR 4 structure and each ADR's number; add an amendment note under "More Information" with the date 2026-10-17. Where the items recommend a direction, follow it (item 70 and item 108: ADR-0063 covers only the technical fallback mechanism; which languages ship is product policy in DEC-099; drop the sign-off exception). Write **ADR-0077: client HTTP stack** (item 116; recommended default: the in-box WinHTTP, HTTPS only, OS certificate validation, system proxy; status Proposed unless the baseline already decides it) and add it to the index. Verify and add the Windows SDK release to the technology baseline (item 108): you may consult Microsoft's own documentation for this, and only for this; cite the source URL in the baseline, as the other baseline lines do.

**Outputs:** the ADR files, `adr/README.md`, and your report with one line per item (item → ADR change, or component fix needed).

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
2. Fix the wrong rows the items name (wrong role, wrong consumer paths, the five ADR-0017 padding rows of item 85, the team-colour authoring of item 83: replace red key colours by the tint attribute or material flag of ADR-0020 in GameAssets, the briefs and the manifest; the look stays the same).
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
