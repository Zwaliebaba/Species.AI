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
- F2b2 done: 600 rows (522 distinct ranges, 26–32); coverage complete and its own column check gives 0 errors. No package over 150 KB, but C-UI sits at about 152 KB with hooks → split C-UI-a (28 §1–5, §7–10, §14, about 100 KB) / C-UI-b (28 §6, §11, §13, about 52 KB). C-DAT at 126 KB → `data_schemas/` folder likely. C-VFX 119 KB, C-AEN 104 KB. To confirm at G1: 32 §7.1 "test harness" is a campaign mission explorer → DL-CAMPAIGN (accepted by the coordinator: a campaign-only tool); the tick-rate row is tagged DD-THREADS for lack of a code → **add `DD-TICK`** with DD-ROADMAP after F1.
- F3 done: `standards_baseline.md` (12 sections, sourced, checked 2026-10-17); 9 DEC and 17 ADR requests in `reports/F3.md`. Recommended: Windows 11 24H2 (build 10.0.26100 minimum; Windows 10 unsupported); MSVC Build Tools 14.51 (VS 2026 18.11) with `/std:c++23preview`, then `/std:c++23` from 14.52, and no C++26; Agility SDK 1.619.6; FL 12_1 plus SM 6.6, Resource Binding Tier 3 and WaveOps checks (12_2 not required); SM minimum 6.6 (current retail 6.9); DXC v1.9.2609. Unverified items are listed in the report. **G1 action:** the `docs/` provenance pattern matches URLs (three GameInput URLs) → add a look-behind so path patterns never match inside a URL.
- F1 done: 687-row ledger (all 151 rows of 90 §3 and 161 IP rows of §2; no Species identifier equals one of the 2,588 original identifiers); 1,321 hard and 52 soft patterns; self-tests pass; CONVENTIONS §8 (311 rows); 8 codes added to drop_topics (incl. DD-ROADMAP); 546 IP tokens. Collisions: H1 Uplink Console→Command Console, H2 Subversion turret→Convert Turret, M1 Warp Gate→Transit Gate, M2 Relay Dish→Beam Dish, M3 Energy Collector→Solar Array, M4 power network→energy grid, M5 Spirit Grove→Essence Grove, M6 Relic Spawn Zone split from Relic Base; L1–L13 information only (L13: the title "Species" overlaps a 1995 film franchise and a 2018 game, so it is owner information).

## Gate G1 (2026-10-17)

- Ledger spot-check: 30 seeded rows read, all correct (role-preserving, IP word dropped, generic words kept with kind suffixes).
- Pattern fixes:
  - the path pattern gets a look-behind (it skips URLs, which fixes F3's three GameInput links, and longer paths);
  - `tools/` allows `tools/design-models/`;
  - `_work/` no longer matches inside `species_work/` or `species_design_work/`.
  Verified on samples: provenance is still caught.
- Packages that must never name `data/`, `code/` or `docs/` directories: C-AST, C-BLD, C-DAT (note in their packages).
- CONVENTIONS §6: two soft hits reworded.
- drop_topics: `DD-TICK` added; 32 L879 re-tagged DD-TICK; 40 §7 re-tagged DD-ROADMAP.
- Routing check after the fixes: 1,990 rows, 0 errors, every line of 27 documents covered.
- Coordinator accepted: 32 §7.1 as DL-CAMPAIGN; 24 §2.3 as KEEP + TO-ADR; about 23 KB of doc 20 to C-SIM.
- `scripts/build_packages.py` → 49 packages (`packages/INDEX.md`), 0 unclaimed content rows, all ≤ 150 KB except two exempt ones:
  - X-DEC-2a (DD-NET, read for lessons only);
  - X-AST (script-driven from the CSVs).
- Splits:
  - G02a→a1/a2; G08→a/b (editor hooks); C-SIM→a/b/c; C-RND→a/b; C-UI→a/b;
  - X-DEC→1, 2a, 2b, 2c, 3, 4a, 4b, 5a, 5b, 5c (two waves; wave 2b deduplicates against 2a); X-ADR→a/b; X-GLO→1/2.
- New folder outputs:
  - in `reference/`: 04, 08, 09, 12;
  - in the components: `ui_framework/` and `data_schemas/`;
  - also `decisions/` (an index plus per-package files and `deferred/`) and `glossary/`.
  - The prompt §5 is updated; package JSONs are authoritative (§9.3 note).
- DEC and ADR ids: unique within the assigned ranges; gaps allowed; never renumbered (prompt G2 updated).
- Surviving files (`vision/`, `prompts/species-design-phase.md`, `design_next/CONVENTIONS.md`): 0 hard-fail hits with the full F1 pattern set.
- Owner questions batch 1: pending.
- **Owner batch 1 answered (2026-10-17):** all eight renames accepted (Command Console, Convert Turret, Transit Gate, Essence Grove, energy grid, Beam Dish, Solar Array, Relic Spawn Zone); 40-U14 reference tuning values may be adopted as a baseline, with a legal check before commercial release. Recorded in `owner_answers.md`, in the ledger (25 rows marked confirmed), in `name_collisions.md`, and as **O12 and O13** in the vision brief. The design-phase prompt now starts from O1–O13; new owner answers there start at O14. Phase 2 packages now read `owner_answers.md`.
- **Gate G1 passed.** Phase 2 ready: wave 2a = X-DEC-1, X-DEC-2a, X-DEC-2b, X-DEC-2c, X-ADR-a, X-ADR-b; wave 2b = X-DEC-3, X-DEC-4a, X-DEC-4b, X-DEC-5a, X-DEC-5b, X-DEC-5c.

## Phase 2 — Decisions
- Wave 2a started (2026-10-17): X-DEC-1, X-DEC-2a, X-DEC-2b, X-DEC-2c, X-ADR-a, X-ADR-b. ADR index ownership: X-ADR-a writes `adr/README.md` with a marker row; X-ADR-b writes `adr_index_X-ADR-b.md` for the coordinator to merge (avoids a concurrent-write collision).
- **Owner request (during Phase 2):** a document `GameAssets.md` listing every asset to create, with filenames. Owner chose Species filenames in the repo plus a private original-to-Species map. X-AST pass 1 was pulled forward and started now as a seventh parallel agent (owner-requested deliverable): `design_next/assets/GameAssets.md` plus the manifest, README and briefs, and the private `species_work/asset_origin_map.csv` (→ OUTBOX). Prompt §5 and §9.3, and design-phase E2, updated to include GameAssets.md.
- X-DEC-2a done: DEC-200…208 (9 Open networking decisions, 88 numbered constraints) in `decisions/deferred/networking.md` (50.5 KB); forbidden scan 0; 17 soft hits, all "lockstep" in DEC entries (allowed; add to CONVENTIONS §9 at G2). **Incident:** the agent used the Artifact tool to apply edits, which published the file as a private artifact (https://claude.ai/artifact/PDkJgi6qCyVwQkiTkbtoEv; clean Species content). Not deleted: deletion is the owner's call. All six running agents were told never to use publishing tools; the "No publishing" rule was added to §4 of both prompts.
- X-DEC-2b done: DEC-220…229 (10 Open DECs, 94 constraints) in `decisions/deferred/process_tick_and_roadmap.md` (55.8 KB); forbidden scan 0; 1 soft hit (`Linux` in DEC-220, dedicated-server hosting, allowed → CONVENTIONS §9 at G2). Published nothing. Cross-document requests queued for Phase 3 (C-SIM-a/c, C-RND-b, C-AEN, C-AST, C-DBG, C-PLT, X-DEC-2a/2c, X-ADR). **G2 check:** confirm that font aliasing on a language switch (C-UI), achievement gating (G09b) and input-driver order (C-INP) are in their owners' packages.
- X-DEC-2c done: DEC-240…251 (12 Open) in `decisions/deferred/numerics_lobby_terrain_services.md`: numeric model 240–242, lobby and sessions 243–245, terrain runtime 246–247, services 248–249, persistence 250–251 (DEC-251 constraints from the vision brief, because no input covered persistence). Forbidden scan 0; 4 soft hits (`Linux` in the dedicated-server build context → CONVENTIONS §9 at G2). Published nothing. Ledger proposal: `Mat3x3` for the 3×3 matrix type (apply at G2). The report lists which DECs each rewrite package must cite.
- X-ADR-b done: ADR-0050…0076 (27 MADR 4: 18 Accepted, 9 Proposed, none blocking) incl. product editions and feature flags, version identity, the platform clock and the application window; index rows in `adr_index_X-ADR-b.md` (merge at G2). Forbidden 0; 2 soft hits (`Linux`, server hosting). All 17 F3 requests covered by the a and b ADRs. WACK deprecation re-checked on Microsoft Learn (ADR-0072). **G2 items:** reconcile the user-data path (X-ADR-a README says `%LOCALAPPDATA%\Species`, the ledger says `<STUDIO>\Species\`); fill the README "Decided in" column with 0050+ numbers; 10 DEC requests and 6 cross-document requests in `reports/X-ADR-b.md`. Published nothing.
- X-DEC-1 done: DEC-001…117 (71 Resolved, 34 Open, 12 Dropped) in `decisions/verdicts_and_fixes.md` (117.9 KB, under the limit); forbidden 0, soft 0; every F1–F59 and D1–D10 row mapped (F16, F30, F31, F59 → X-DEC-2a). Interpretations accepted by the coordinator and listed for owner review at G2: 40's "Decide" rows → Open (reference default; 40's default as the recommendation); F35 Open (two fixes offered); F20 read as Flight both invulnerable and terminal; **2-second recounts = 21 ticks** (same one-tick-late rule as the 11-tick mode second); 41 rank 7 covered by 40's fixes, ranks 12 and 14 each split into a Resolved fix and an Open decision.
- Wave 2b started: X-DEC-3, X-DEC-4a, X-DEC-4b, X-DEC-5a now; X-DEC-5b and 5c when slots free (cap 6, with X-ADR-a and X-AST running).
- **Wave 2b ownership rule (coordinator):** X-DEC-3 owns every per-document open question, via the consolidated 41 §3 register. X-DEC-4a/4b/5a/5b/5c skip input ranges whose anchor starts with `open-questions` (about 20 % of each package) and decide only body rows, deduplicating against the wave-2a files. Overlaps with X-DEC-3 are listed as "covered by X-DEC-3". Running: X-DEC-3, 4a, 4b, 5a (plus X-ADR-a and X-AST); 5b and 5c are queued.
- X-ADR-a done: ADR-0001…0036 (24 Accepted, 12 Proposed, none blocking), including the added ADR-0013 (working colour space, display-referred default) and ADR-0003 (shader toolchain); `adr/README.md` with an index, the `<!-- X-ADR-b rows -->` marker, the condensed technology baseline with sources, and the nine deferred topics. Forbidden 0; 6 soft hits (`retail` in a vendor URL, `Linux` server/CI, `immediate-mode` dev tools → CONVENTIONS §9 at G2). Unverified: the ADPCM encoder source and the `GameInput::v4` namespace (defaults stand). Published nothing.
- X-DEC-5b started (slot freed by X-ADR-a); X-DEC-5c queued.
- X-AST pass 1 done: `assets/GameAssets.md` (149 KB, kept as one document at the owner's request), `asset_manifest.csv` (764 KB, 1,311 rows, 401 asset ids), README, briefs; private `asset_origin_map.csv`. All 1,430 in-scope originals map to exactly one Species file; 817 excluded with counts per class; no Species stem equals an original stem; both scans 0. Priorities: 586 first-playable (Spawn Control 2p, core crate pool), 718 launch, 7 post-launch. **Coordinator decisions:** exempt `assets/GameAssets.md` (owner-requested single document) and `assets/*.csv` (machine data) from the 120 KB gate. **Pass 2 items:** reconcile 1,311 GameAssets files vs 1,327 origin-map rows (string-table counting); ten assets with unconfirmed roles; requests for G10, G04, G03, C-UI, C-DAT, C-AST; check "NEAREST" sampler wording (use "point filtering"); map file extension `.json` against the data-schema ADR.
- Owner hand-over sent: `asset_recreation_overview.md` and `.csv` (scratchpad, not in the repo), plus GameAssets.md.
- X-DEC-5a done: DEC-650…654 (4 Resolved: actor-grid footprint, draws at object set-up, no NaN or infinity, gameplay on the CPU only; 1 Open: route lifetime); 9 body rows covered by existing DECs; 6 open-questions rows skipped; scans 0. **G2 items:** DEC-011 wording ("filled on demand") conflicts with DEC-652 (filled at match setup) → fix DEC-011; confirm H7 (obstacle-grid band around Barrier Emitter lines) → DEC-088; check that X-DEC-3 covers 06-U9 (orders given with no Workers under control).
- X-DEC-4a done: DEC-450…466 (8 Resolved, 8 Open, 1 Dropped); about 130 body items already covered by wave 2a; open-questions ranges skipped; scans 0. **G2 items:** DEC-455 may overlap X-DEC-3 (04-U11, 05-U2, 05-U11) and X-DEC-5a's update order (DEC-651/652) → merge or cross-reference; **the Collector/Builder loop that can spin on an invalid entry is a possible hang → must stay Resolved (DEC-014)**, and any X-DEC-3 entry for it must be Resolved; extra facts listed for 16 existing DECs (DEC-002, 007, 011, 014, 015, 016, 018, 028, 033, 073, 079, 081, 084, 092, 117, 241) → apply at G2.
- X-DEC-4b done: DEC-550…570 (10 Resolved, 10 Open, 1 Dropped); about 135 of about 185 mapped rows already covered (every 09 row covered by DEC-026, 044, 067, 099, 100, 243–250); scans 0; its build script validates ids, field order, statuses, references and Affects. Facts it took from out-of-range lines were removed. **G2 items:** DEC-087 lists only some of the bot fixes, and the B11 fix depends on Siege bot Transport behaviour that never runs in the reference, so DEC-112 bot calibration must cover Siege; **"simulation code never issues player commands" exists only in Open DEC-202 → it is an integrity rule, so record it as Resolved (prompt: integrity defects are always Resolved)**; cross-cite DEC-569↔DEC-751 and DEC-555↔DEC-653.
- X-DEC-5b done: DEC-750 (Open: explainer-animation loop and visibility), DEC-751 (Resolved: game-speed pitch state starts each match at normal speed), DEC-752 (Resolved: audio query crash and effects left on reused voices); 15 of 19 body rows already covered by DECs and ADRs; scans 0. Told X-DEC-5c that its two shared rows are covered by ADR-0061 and DEC-015. **G2 items:** DEC-101 (seasonal theme, Open) still lists a per-client date check, which is an integrity defect → split out a Resolved part; DEC-102 must also cover the window quirks Q1–Q10; if DEC-567 covers stale effects on toggle, reduce DEC-752 to the crash.
- X-DEC-3 done: DEC-300…342 (31 Open, 1 Resolved, 11 Dropped) in `decisions/open_questions.md` (about 44 KB); all 208 question rows and 19 merge pointers mapped (94 to new entries, 114 covered by existing DECs or ADRs); scans 0. **G2 items:** 15 extra facts for existing entries (for example the DEC-016 port-occupant tie key, DEC-111 Collector throughput, DEC-002's 10h + 1 being only the upper bound); 10 readings it could not confirm ("booster" taken as the Shield; the meaning of "twin terrains"; the page-transition progress variable; DEC-321–323 possibly overlapping DEC-097). Its report has a table of which rewrite package must cite each Open DEC.
- X-DEC-5c done: DEC-800…804 (3 Resolved: lossless, typed, atomic writers; duplicate keys, repeated fields and over-long values are build errors, which fills an ADR-0059 gap; soak-test rules; 2 Open: unlisted maps, camera animations without content); 13 rows covered; scans 0.

## Gate G2 (2026-10-17)

- **Phase 2 totals:** 242 DECs in ten files (118 Open, 99 Resolved, 25 Dropped); 63 ADRs (0001–0036, 0050–0076).
- `scripts/g2_merge.py`:
  - ids unique and in range;
  - `decisions/README.md` index generated;
  - `dec_map.csv` merged (242 rows);
  - X-ADR-b's 27 rows inserted at the ADR README marker;
  - every DEC and ADR reference in `design_next/` resolves (range statements excluded).
  Result: 0 errors.
- Forbidden scan over all of `design_next/`: **0 hits**.
- Soft hits: Linux (11), lockstep (17), immediate-mode (1), retail (1), all in their allowed contexts → added to CONVENTIONS §9 "Allowed terms".
- **No ADR is `owner-decision: blocking`**, so owner batch 2 is not needed.
- Reconciliation agent X-DEC-R started with `g2_amendments.md`: 13 specific fixes, the extra facts from every report, and the owner-review list.
- Owner instruction: delete the stray artifact → deleted (https://claude.ai/artifact/PDkJgi6qCyVwQkiTkbtoEv no longer resolves). The content remains in `design_next/decisions/deferred/networking.md`.
- X-DEC-R done: 109 amendments (A 28, B 49, report cross-fixes 32); new Resolved DEC-118 (seasonal state decided once per match) and DEC-119 (simulation code never issues player commands); DEC-306 superseded by DEC-455; DEC-323 Open→Resolved. The verdicts index table was replaced by a section index (file 119,005 B). Coordinator: added `dec_map_X-DEC-R.csv`, updated DEC-306/323 in the X-DEC-3 map, `<STUDIO>`→`<Studio>` in CONVENTIONS and the ledger. **Unresolved conflicts, routed to Phase 3:** DEC-011 (whether the stale Source Node id has any effect) → C-SIM-a and G03a decide from ORIG and report; ADR-0063 (signed-off fallback keys) vs DEC-099 (complete languages only) → G09b and C-UI-a report the conflict, and it is reconciled in Phase 4; DEC-021's ambiguous "Forest" in the starting-power exclusions → G04 resolves it from ORIG (Essence Grove or Blight Grove). ADR agents' new-DEC requests are listed in `reports/X-DEC-R.md` for Phase 4.
- **Gate G2 passed (2026-10-17):** 244 DECs (116 Open, 102 Resolved, 26 Dropped) = 244 map rows; 63 ADRs; `g2_merge.py` 0 errors; forbidden scan over `design_next/` 0 hits. `enrich_packages.py` added `decs_to_cite` and `assets_for_you` to every Phase 3 package; **every Open DEC is claimed by at least one package**.

## Phase 3 — Rewrite

- Common brief: `species_work/phase3_brief.md`. Rolling queue with at most 6 running, in this order: G00, G05, C-SIM-a, C-SIM-b, C-SIM-c, X-GLO-1 | X-GLO-2, G01, G02a1, G02a2, G02b, C-RND-a | G03a, G03b, G04, G06, G07, C-RND-b | G08a, G08b, G09a, G09b, G10, G11 | C-TER, C-AST, C-VFX, C-AEN, C-UI-a, C-UI-b | C-INP, C-CAM, C-DAT, C-PLT, C-BLD, C-DBG.
