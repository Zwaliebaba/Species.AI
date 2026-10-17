# Owner answers recorded during the conversion run

## Batch 1 (gate G1, 2026-10-17)

| # | Question | Answer | Applies to |
|---|---|---|---|
| G1-1 | Control station name | **Command Console** | ledger, every package |
| G1-2 | Renames A | **Convert Turret, Transit Gate, Essence Grove, energy grid** (all accepted) | ledger, every package |
| G1-3 | Renames B | **Beam Dish, Solar Array, Relic Spawn Zone** (all accepted) | ledger, every package |
| G1-4 | Reuse of reference tuning values (40-U14) | **Yes, as a baseline.** Species may adopt the reference values as its starting values; a legal check is required before commercial release. | X-DEC: record 40-U14 as **Resolved (owner decision)**. Reference documents keep every value; tuning tables carry no "provisional" flag for this reason. |

Owner information only (no decision asked): the title "Species" overlaps the *Species* film franchise (1995) and the game *Species: Artificial Life, Real Evolution* (2018). The coordinator recommends a trademark search before any public use of the title.

## Batch 3: reference gameplay rules (during Phase 3, 2026-10-17)

| O | Question | Answer | Applied |
|---|---|---|---|
| O14 | Per-team population cap (DEC-079) | **Fill the missing player slots with AI**; follow-up: **fill by default; the host may close slots** (the cap is then divided by the open slots); leavers are taken over by bots | DEC-079 Resolved |
| O15 | Collector any-team orbs (DEC-080) | **Any team, cap on delivery** | DEC-080 Resolved |
| O16 | Catch-up bonus (DEC-081) | **Proportional curve** (Rogue/Visitor excluded) | DEC-081 Resolved |
| O17 | Mode second (DEC-003) | **11 ticks for the first playable; A/B playtest of 10 vs 11 before launch** | DEC-003 stays Open with an owner note |
| O18 | Hostile crate outcomes (DEC-107) | **Hazards team-blind; every outcome respects map exclusions; Brood ownership in playtest** | DEC-107 Resolved; DEC-121 (Open) split out |
| O19 | Map value 0 (new) | **Explicit values: absent = lobby default, 0 = none, explicit unlimited** | DEC-120 Resolved |
| O20 | First playable mode | **Spawn Control** (2 players), confirming the GameAssets.md assumption | GameAssets priorities stand; design phase B9 |

## Batch 4: code standards (Phase 4, 2026-10-17)

The owner added `AGENTS.md`, `.clang-format`, `.clang-tidy`, `.editorconfig`, `.github/pull_request_template.md` and `.github/workflows/build.yml` (commit 1b54ae7, merged into the branch). `AGENTS.md` contradicted nine engine ADRs, the ADR home and the design's code naming.

| O | Question | Answer | Applied |
|---|---|---|---|
| O21 | AGENTS.md against the engine ADRs (build system, dependencies, shaders, flavours) | **AGENTS.md wins, with a Microsoft allow-list**: the Agility SDK, the GameInput redistributable and WinPixEventRuntime (NuGet); everything else as AGENTS.md says | AGENTS.md R14 amended; X-ADR-P4 follow-up amends ADR-003, 0050, 0051, 0052, 0053, 0054, 0055, 0056, 0058, 0059, 0062, 0068, 0069, 0070, 0071 and the baseline; wave B aligns the components |
| O22 | ADR home | **`Design/ADR/ADR-NNN-<slug>.md`**, three-digit ids keeping the numbers; library root `Design/` at the cutover | coordinator renames and rewrites every citation; prompt §8.2 cutover target `Design/` |
| O23 | Code naming in the design | **Convert now** to AGENTS.md naming (`Engine` and `Species` namespaces; American spelling in identifiers; prose British) | wave B fixers; FX-12 rewrites CONVENTIONS §8 |
| O24 | AGENTS.md scope | **Status and authority**; R18+ reserved for the design phase, with candidates listed | AGENTS.md updated by the coordinator |

| O25 | Shader files (X-ADR-P4 proposals 1, 2) | **Add `CS` and permutation files** (`<Shader><Permutation><Stage>.hlsl`, shared `<Shader>.hlsli`) | AGENTS.md §2, .editorconfig |
| O26 | Content build tool's BC7 encoder (proposal 4) | **Separate tools solution under `Tools/`**, may use pinned third-party code with an ADR and licence | AGENTS.md §2 and R14 |
| O27 | Housekeeping (proposals 3, 5, 6 and a design-check job) | **CI NuGet restore step and CI design-check job** only; no fuzz-project rule, no `ENGINE_DEVELOPMENT` rule | build.yml; X-TOOLS brief |
| O28 | How the game gets the schema code generator's output (RES-1 question) | **Commit generated source**; CI regenerates and fails on a difference | AGENTS.md §2, ADR-062 amendment, DAT README, build.yml guarded step |
