# Report A0-P: vision, pillars and core loops (proposal)

> Area A0, wave A · Proposer · Date: 2026-10-18 · Status of the GDD: Draft

## Files written

| File | What |
|---|---|
| `Design/gdd/00_vision_pillars_and_core_loops.md` | The GDD, prefix `PIL`, in the nine-heading template; 89 KB |
| `Design/gdd/adoption_ledger/00_reference_overview_ledger.md` | Every verdict of reference/00: 18 section rows and 11 exception rows, no `pending` row |
| `species_design_work/areas/A0/proposal_notes.md` | Options, recommendations, assumptions, owner questions, adoption, DEC-001, cross-area requests, the comparable titles |
| `species_design_work/reports/A0-P.md` | This report |

No other file was changed. No publishing or upload tool was used.

## Counts

- **Requirements:** 74 (`PIL-001` to `PIL-091`, with gaps between groups): scope 9, pillars 7, pillar guards 10, audience 3, battle loop 9, settlement loop 7, world loop 10, presence and sessions 10, safety floor and information 7, vocabulary 2.
- **Tuning keys:** 29 `pil.*` keys, 5 final and 24 provisional, each provisional row with a playtest note naming the model or test that confirms it.
- **Diagrams:** 8 Mermaid diagrams (the coupled loops, the battle loop, the settlement loop, the world loop, the attack lifecycle, presence, world state, a return to the world); all parse.
- **Vocabulary:** 49 rows in Table 3 (new terms, with identifier, string-id prefix, mechanics owner and trademark screen) and 13 rows in Table 4 (reference terms whose use changes in the world game).
- **Acceptance criteria:** 36. **Edge cases:** 28.

## Pillars as proposed

1 Living crowds (confirmed, extended to settlement work) · 2 A world that runs while you are away (renamed: the vision's name contradicts O4 and Stage 1 when read literally) · 3 Conflict with a cause (confirmed; legibility, easing, attack cost) · 4 Delegation you can trust (confirmed; audit trail, information rule) · 5 The reference style, a distinct game (renamed: both halves of O9) · 6 Always a way back (new; owner question). Each pillar has rules in, rules out, a rejected design and a measurable guard.

## Adoption verdicts (reference/00, 112 requirements)

adopt 2 (R-VIS-007, R-VIS-009) · adopt-skirmish 93 · adapt 17 (§2.2 and §2.3 except five ids, R-VIS-003, R-VIS-008, R-VIS-170, R-VIS-174) · drop 0 · pending 0. The overview's restatements bind the skirmish summary; the world game takes the owning documents' verdicts, and owners send A0 a row change if theirs differ.

## DEC and ADR changes

- **DEC-001** (the only A0 decision of the triage): proposed **Resolved, option B** (skirmish keeps the reference loop; the world game adds settlements, resources and construction on top of essence-driven growth), implemented by PIL-002, PIL-003, PIL-004 and PIL-050 to PIL-056. Not escalated. The register is not edited until Finalise.
- No new DEC, no ADR written or changed.

## Owner questions for gate A (4)

Q1 add pillar 6 (recommended: yes) · Q2 clade instead of race (recommended: clade) · Q3 may a steward attack any settlement outside protection (recommended: yes, with attack cost and grievance) · Q4 can a settlement be removed from a world (recommended: never by a battle; the Wellspring survives).

## Cross-area requests

Listed in full in `proposal_notes.md` §7. The main ones: the coordinator registers the vocabulary in CONVENTIONS §8 and the glossary after gate A and updates the ledger README row of reference/00; B2 builds the models for the guards PIL-022, PIL-023, PIL-028 and PIL-029; B6 owns the protection levers, the idle threshold, the default orders and PIL-086; B1 provides a source of Essence that needs no death (PIL-056); B9 aligns its R-MODE verdicts with the R-VIS rows; D1 considers the keep-running mode of the integrated client (recommendation R3).

## Names proposed

world, world game, skirmish, region, settlement site, world time, frozen, world lifetime, world host, integrated server, dedicated server, settlement, Wellspring, territory, founding, steward, AI settlement, strength, expedition, clade, temperament, grievance, territory pressure, anger, relations, truce (reserved), tribute (reserved), attack, battle, battlefield, battle result, protection rules, loss cap, respite, attack cost, defender's advantage, newcomer protection, deputy, standing order, away report, aide, presence, present, away, check-in, full fidelity, abstract fidelity, strategic view, settlement view. Identifiers proposed in Table 3, among them `World`, `Region`, `Settlement`, `Wellspring`, `Steward`, `Clade`, `Grievance`, `Deputy`, `StandingOrder`, `AwayReport`, `Respite`, `LossCap`, `StewardPresence` and `Fidelity`. Renamed from the vision: race → clade (owner question), shield → respite (collision with the Shield outcome), owner → steward, operator → world host, assistant → aide.

## Gate result

`python3 -I Tools/DesignCheck/CheckDesign.py Design --skip-mermaid --design-phase` and the same without `--skip-mermaid`: gates 1 to 14 and 16 to 19 **pass** (gate 4: 274 blocks, 0 failed; gate 12: no pending row in the A0 ledger; gates 12 and 13 report the other areas' pending rows and design-phase decisions, as expected). **Gate 15 fails** with one message: the GDD's status line names `../rationale/A0.md`, which does not exist yet. The brief asks for that path in backticks because the rationale is written at Finalise, and creating it now is outside A0's files; the coordinator either adds `Design/rationale/README.md` with a stub `A0.md`, or makes gate 15 report rather than fail for a Draft document.

## What I verified and what I assumed

Verified: the gates above; that every cited requirement, DEC and owner decision exists; that no term of Table 3 collides with a registered identifier, string-id root, tuning-key root, setting group, schema family or asset stem; the soft-flag word list. Assumed: the twelve assumptions of `proposal_notes.md` §3, chiefly the first audience, stewards attacking stewards (Q3) and Workers as the world's inhabitants. The trademark screen is from general knowledge, not a registry search (recommendation R11).
