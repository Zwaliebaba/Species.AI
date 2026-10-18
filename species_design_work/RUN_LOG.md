# Species design phase: run log

Coordinator log of the design phase (`prompts/species-design-phase.md`). Newest entries last.

## Phase 0 — Setup (2026-10-18)

- Branch `claude/fervent-ramanujan-snt7z0` restarted from `main` at `b3e8bc4` (PR #3 merged: the prompt revised on the finished library). Precondition holds: `Design/README.md` describes the library, `Tools/DesignCheck/` passes on `Design/` (all 11 gates), no source corpus in the tree.
- Library facts recounted: 2,969 `R-` requirements in 657 sections of 61 reference files; 1,959 component requirements; 302 decisions, 148 Open (117 design phase, 30 playtest, 1 prototype); 65 ADRs, 18 Proposed; 432 asset ids in 1,391 files. Unchanged from the prompt.
- `species_design_work/` created (`areas/`, `reports/`, `scripts/`).
- X-CHECK started: `CheckDesign.py --design-phase` (ledger, decisions, templates, rationale, owner decisions, tuning status, models, repository scan with `ScanAllow.tsv`) and `--final`, with self-tests.
- CONVENTIONS §3.1: 28 prefixes registered (GDD `PIL` … `HLP`; architecture `QG`, `QS`, `SRV` … `TEL`); §4: Species key roots (lower-case prefix) and the `status` and playtest-note columns.
- Ledger skeleton (`scripts/LedgerSkeleton.py`): `Design/gdd/adoption_ledger/`, 13 files named `<document>_ledger.md` (without the suffix, the ledger of reference document 03 would have the file name of a source-corpus document, which gate 1 rejects), 657 section rows, 2,969 requirements, every verdict `pending`; README with totals. Prompt §5.3 file names updated to match.
- Decision triage (`scripts/DecTriage.py` → `dec_triage.md`): 148 Open decisions, every one mapped (deferred topics through the ADR index, the rest through the Affects line): A0 1, A1 8, A2 20, B1 3, B3 7, B4 26, B7 9, B9 8, C1 7, C2 12, C4 12, C5 4, D1 1, D10 3, D2 12, D4 3, D5 2, D8 8, E2 2. None assigned by hand.
- `Design/decisions/owner_decisions.md`: O1–O30 copied from the vision brief §3; the register README points to it. `Design/gdd/README.md`: the GDD index (16 documents, not started).
- Gates on `Design/` after these changes: all 11 PASS (Mermaid skipped; no diagram changed).
- X-CHECK **done**: `CheckDesign.py --design-phase` adds gates 12 (adoption ledger), 13 (design-phase decisions), 14 (GDD and component templates), 15 (rationale files), 16 (owner decisions), 17 (tuning status), 18 (models), 19 (repository scan with `ScanAllow.tsv`, digests only, three stock Visual Studio template lines); `--final` fails on pending verdicts, remaining design-phase decisions and missing models. `SelfTestDesignPhase.py`: 28 of 28. Gates 1–11 unchanged. Coordinator fixed its own two hits (an O-number cited before it exists in owner_decisions.md; a corpus file name in this log). Run on `Design/` with `--design-phase`: all 19 gates PASS (gate 12: 2,969 ids pending; gate 13: 117 design-phase decisions).
- CI design job: runs `SelfTestDesignPhase.py` and `CheckDesign.py Design --design-phase` (guarded; `--final` stays the run's closing check).
- **Phase 0 complete.**

## Phase 1 — Wave A
- A0-P **done**: `gdd/00_vision_pillars_and_core_loops.md` (Draft, 89 KB): 74 requirements (PIL-001 to PIL-091), 29 tuning keys (5 final, 24 provisional), 8 Mermaid diagrams, 49 vocabulary terms and 13 changed-use rows (for example shield → respite, owner → steward, race → clade); six pillars (two of the vision's five renamed, new pillar 6 "Always a way back"); ledger for reference/00: adopt 2, adopt-skirmish 93, adapt 17, drop 0; DEC-001 proposed Resolved (option B: skirmish keeps the reference loop; the world game adds resources and building on essence-driven growth); four owner questions (pillar 6, "clade" for "race", free attacks between stewards, a settlement never removed by a battle).
- Coordinator: gate 15 now reports, and does not fail, a not-yet-written rationale under `rationale/` while the document is not Final (`--final` and a Final status still fail; a path outside `rationale/` always fails); self-test 29 of 29. `scripts/LedgerTotals.py` refreshes the ledger README totals from gate 12's effective verdicts. All 19 gates PASS. → A0-C started.
