# Run log — Species reference-library conversion

- Prompt: `prompts/species-design-conversion.md`
- Branch: `claude/fervent-ramanujan-snt7z0`
- BASE_COMMIT: `cc37fbd8d9a6af6885f7902d783e7f94edba682e`
- PR: https://github.com/Zwaliebaba/Species.AI/pull/2

## Phase 0 — Setup (2026-10-17)

- Branch and tree verified clean; `vision/species-vision-brief.md` present.
- Created `species_work/` (reports, scripts, packages) and the `design_next/` skeleton (`.gitkeep` placeholders, removed as files land).
- `scripts/headings.py` → `headings.csv`: 2,362 headings across 27 documents (00–41, 90).
- Drafted `design_next/CONVENTIONS.md` (naming section left for F1).
- **Coordinator adjustment:** drafted `drop_topics.md` with fixed reason-class codes, so the F2 routing agents can run in parallel with F1. F1 refines descriptions and may add classes, but never renames or removes one.
- **Coordinator adjustment:** split routing into five agents to fit context budgets: F2a1 (00–05, 1.3 MB), F2a2 (06–10, 1.5 MB), F2b1 (20–25, 1.8 MB), F2b2 (26–32, 1.6 MB), F2c (40, 41, 90). Six agents run in parallel (F1 plus the five F2 agents); F3 starts when the first finishes.
- **Coordinator adjustment:** pushing after Phase 1 as well (the prompt asks only after Phases 3, 6 and 7), because the cloud container is ephemeral.

## Phase 1 — Foundations
