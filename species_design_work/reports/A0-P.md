# Report A0-P: vision, pillars and core loops (proposal, revised after challenge round 1)

> Area A0, wave A · Proposer · Date: 2026-10-18 · Status of the GDD: Draft · Round 1 answered in `species_design_work/areas/A0/resolution_1.md`

## Files written

| File | What |
|---|---|
| `Design/gdd/00_vision_pillars_and_core_loops.md` | The GDD, prefix `PIL`, in the nine-heading template; 116.6 KB of the 120 KB limit (a later growth splits it into a folder) |
| `Design/gdd/adoption_ledger/00_reference_overview_ledger.md` | Every verdict of reference/00: 18 section rows and 24 exception rows, no `pending` row |
| `species_design_work/areas/A0/proposal_notes.md` | Options, recommendations, assumptions, the merged owner questions, adoption, DEC-001, cross-area requests, the comparable titles |
| `species_design_work/areas/A0/resolution_1.md` | The answer to every finding and "make it better" item of challenge round 1 |
| `species_design_work/reports/A0-P.md` | This report |

No other file was changed. No publishing or upload tool was used.

## Counts (after round 1)

- **Requirements:** 98, `PIL-001` to `PIL-141` in groups (two SHOULD: PIL-125 renown, PIL-140 replays). New in round 1: world pace PIL-100 to PIL-104; the integrated host PIL-130 to PIL-133; claims and conflict end states PIL-120 to PIL-125; stages PIL-110; world events PIL-049; watched and strength PIL-057, PIL-058; respite and re-founding PIL-087, PIL-088; the reference baseline PIL-092; replays and the check-in entry PIL-140, PIL-141.
- **Tuning keys:** 47 `pil.*` keys, 5 final and 42 provisional, each provisional row with a playtest note.
- **Diagrams:** 8 Mermaid diagrams; all parse (figures 4 to 8 revised).
- **Vocabulary:** 57 rows in Table 3 and 16 in Table 4. New: world pace, core, watched, dormant, claim, unprovoked attack, renown, world event.
- **Acceptance criteria:** 49. **Edge cases:** 36. **Tables:** 10 numbered plus 4 lettered (Table 10: the smallest world game per stage).

## Challenge round 1

- **Findings (23):** 22 Accepted (5 with one part changed or left out, each stated), 0 Rejected, 1 Escalated.
- **Make it better (6):** 6 Accepted, 3 of them in part.
- **Open Critical or High:** A0-F01 (Critical, world time in stage 1), escalated as Q5; the design meets it for every option through the two world paces. All five High findings are resolved (A0-F02 to A0-F06).

## Adoption verdicts (reference/00, 112 requirements)

adopt 1 (R-VIS-009) · adopt-skirmish 60 · adapt 51 · drop 0 · pending 0. Round 1 moved the restatements that a PIL requirement commits the world game to (verbs, Leaders, combat, the tick) to `adapt`, and the six-mode content to `adapt` with PIL-003's clause rule.

## DEC and ADR changes

- **DEC-001**: proposed Resolved, option B, unchanged by round 1 (PIL-002 to PIL-004, PIL-050 to PIL-058). Not escalated; the register is edited at Finalise.
- No new DEC, no ADR written or changed.

## Owner questions for gate A (5, merged)

Q1 add pillar 6 with claims (agree) · Q2 clade (agree) · Q3 attacks outside protection, an unprovoked attack costs double, attacks between stewards off by default in integrated worlds (agree) · Q4 never removed by battle, dormancy after 21 days (agree) · Q5 stage-1 always-on worlds: in the open client with presentation suspended (proposer) or as a background headless process (challenger), or none in stage 1 (**disagree**).

## Cross-area requests

In full in `proposal_notes.md` §7. Main ones: the coordinator registers the vocabulary after gate A, updates the ledger README row, considers a gate-computed owner verdict for restatements, and renames B5's file if "clade" is confirmed; B2 runs its models at both paces, including the joint leader model with B4, B5 and A1; B5 owns claims, end states and renown; B6 owns the levers of PIL-086, PIL-087's minimum loss and dormancy; D4 and D9 own the save record of PIL-133; D5 makes second accounts recognisable.

## Names proposed

world, world game, skirmish, region, settlement site, world time, frozen, world pace (always-on, session), world lifetime, world host, integrated server, dedicated server, settlement, Wellspring, territory, core, founding, steward, AI settlement, strength, watched, dormant, expedition, clade, temperament, grievance, territory pressure, anger, relations, truce (reserved), tribute (reserved), claim, unprovoked attack, renown, attack, world event, battle, battlefield, battle result, protection rules, loss cap, respite, attack cost, defender's advantage, newcomer protection, deputy, standing order, away report, aide, presence, present, away, check-in, full fidelity, abstract fidelity, strategic view, settlement view. Round 1 identifier changes: `AttackDeclaration`, `Fidelity::Modeled`, string roots `aide` and `presence.check_in`.

## Gate result

`python3 -I Tools/DesignCheck/CheckDesign.py Design --skip-mermaid --design-phase` and the same without `--skip-mermaid`: **all 19 gates pass** (gate 4: 274 blocks, 0 failed). Gate 15 now reports the rationale file that Finalise writes; gates 12 and 13 report the other areas' pending rows and design-phase decisions, as expected.

## What I verified and what I assumed

Verified: the gates; the challenger's model (`species_design_work/scripts/A0C_SessionPacing.py`, inputs and arithmetic); that every cited requirement, DEC and owner decision exists; that the new terms collide with no registered identifier, string-id root, tuning-key root, setting group, schema family or asset stem. Assumed: the twelve assumptions of `proposal_notes.md` §3, and a session factor of 12 sized for a host who plays 2 hours a day. The trademark screen is from general knowledge, not a registry search; the product name is now on the registry-search list.
