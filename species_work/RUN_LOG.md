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
- F2a1 done: 235 rows (00–05); its own check finds 0 errors. G02a is 185 KB → split G02a1 (units README, worker, leader and formations) / G02a2 (essence orbs, strike team, transport, collector, builder). G04 at 140.5 KB: output likely > 120 KB → make 04 a folder (crates and powers / weapons and projectiles). G03a at 145 KB: optional split. G08 receives 45.5 KB of per-map data from 01, 04 and 05. Mode file names provisional, to align with the F1 ledger.
- F3 started (slot freed by F2a1).
- F2a2 done: 400 rows (06–10); coverage passes for all five files. G09 is 207.6 KB → split in the CSV into a folder `reference/09_front_end_and_match_flow/`: G09a (123.8 KB, 09 §1–§5 and §9, plus README) / G09b (83.8 KB, 09 §7 and §10–§12, plus the 08 §7.3 hooks). **Update the §5 map at packaging.** G06 at 138.5 KB from 06 alone → split off G06b (camera, 06 §9) if F2b2 adds more than about 11 KB of ORIG 30. Per-map tables are cut out as TO-ASSETS (no map list reaches G08). **G10 package must include `ORIG/_work/drafts/X2b_sound_events.csv` as a shared read** (the event catalogue is TO-ASSETS). Four MERGE rows feed C-SIM.
- F2b1 done: 383 rows (20–25); its own check plus `check_routing.py` on a scratch copy give 0 errors for its files. DEFER-DESIGN 72 rows. **C-SIM is 351.5 KB** → split into three packages, and make `reference/12_simulation_model/` a folder: C-SIM-a (object model and match lifecycle, 139 KB) / C-SIM-b (space, teams and commands, 85 KB) / C-SIM-c (timing, randomness and integrity, 128 KB). **Update the §5 map.** C-RND is 125.8 KB from 24 alone, with 26 and 40 D5 still to come → C-RND-a (frame and world, 90 KB) / C-RND-b (crowds, overlays and visual language, 36 KB plus the 26 render parts). Judgement calls to confirm at G1: about 23 KB of doc 20 (per-match reset and integrity rules) goes to C-SIM; 24 §2.3 baseline state table is KEEP + TO-ADR (the pass table depends on it); **23 §16 test vectors name a reference map → C-TER must restate their inputs without the name.**
- F2c done: 372 rows (40, 41, 90); its read-only gate check passes (rows with shared ranges are split into "part k/N" pieces). Pending for G1:
  - **Add the code `DD-ROADMAP`** to `drop_topics.md` once F1 has finished editing it, and re-tag the 40 §7 rows (currently DD-NET plus four other DD codes).
  - **The fix-list is 59 rows (F1–F59), not 61**; the prompt X-DEC brief is corrected.
  - **X-DEC is about 300 KB of input from 40/41 alone** → three packages (117 / 86 / 125 KiB; the middle one, the design-phase topics, may need a further split once the 20–25 DEFER rows are added).
  - **X-GLO is 174.7 KiB** → split A–L / M–Z.
  - **40-U14 (may the reference tuning values be reused verbatim?)** is an IP and scope question → escalate to the owner in batch 1.
  - Six facts resolved in 41 §1.4 are MERGE to G02a, G02b, G04 and G10.
