# X-DEC-P4a hand-off to X-DEC-P4b

Cross-file amendments that X-DEC-P4a needs in decision files owned by X-DEC-P4b. Each item gives the file, the entry, the exact old text and the exact new text. They follow from requests that X-DEC-P4a triaged (`p4_dec_triage_X-DEC-P4a.csv`); the new entries they cite are in `design_next/decisions/gameplay_gap_rules.md` and `verdicts_and_fixes_2.md`.

## H-1 DEC-653 item 7: zero-score weighted crate drop (item 47, G04 request 2)

File `decisions/simulation_terrain_render_assets.md`, entry DEC-653, specified behaviour, item 7. DEC-465 (equal weights at a zero total) is the rule the documents apply (R-CRATE-106 step 2); item 7 reads as if the drop fell back to random placement.

Old text:

> 7. A score-weighted Supply crate drop whose total score is 0 takes the unweighted choice directly; the reference reaches the same fallback after extra draws, which DEC-013 does not reproduce. The drop position is DEC-114.

New text:

> 7. A score-weighted Supply crate drop whose total score is 0 counts every score as equal: every weighted team has weight 1, and the share p that its first draw tests is max(1 ÷ n, 1 − 1 ÷ n) for n weighted teams (DEC-465, R-CRATE-106); no step divides by the zero total. The drop position is DEC-114.

## H-2 DEC-652: a forced crate outcome makes no selection draw (G04 request 6)

File `decisions/simulation_terrain_render_assets.md`, entry DEC-652, specified behaviour. R-CRATE-121 skips both the 0.9 roll and the draw of a beneficial outcome when a tutorial forces the outcome.

Old text:

> for example, the Supply crate capture roll against 0.9 is drawn only when no outcome is forced.

New text:

> for example, the Supply crate capture roll against 0.9 and the draw of a beneficial outcome are made only when no outcome is forced (R-CRATE-121).

## H-3 DEC-653 items 5 and 6: cross-references to the new gap rules (G02a1 request 1, G07 request 4)

File `decisions/simulation_terrain_render_assets.md`, entry DEC-653, specified behaviour.

Item 5, old text:

> 5. The Worker fire and combat rules and the thrown-weapon rule that divide by the game speed are never evaluated at a game speed of 0, because no tick runs while the match is paused (DEC-038, DEC-039); each of these divisions carries an explicit guard.

Item 5, new text:

> 5. The Worker fire and combat rules and the thrown-weapon rule that divide by the game speed are never evaluated at a game speed of 0, because no tick runs while the match is paused (DEC-038, DEC-039); each of these divisions carries an explicit guard. Whether the Worker rules keep these divisions is DEC-852.

Item 6, old text:

> 6. A bot priority with no usable neighbours yields no NaN (DEC-087).

Item 6, new text:

> 6. A bot priority with no usable neighbours yields no NaN: its neighbour mean is 0 (DEC-087, DEC-862).

## H-4 DEC-229: the owner decision on the first playable (item 24)

File `decisions/deferred/process_tick_and_roadmap.md`, entry DEC-229. O20 has no register line; G01 cites DEC-229 for the build order. Insert one line after `- Status: Open` (the entry stays Open):

> - Owner decision: O20 (2026-10-17): the first playable is Spawn Control with two players and the core Supply crate pool; whether it has a tutorial is DEC-123.

## Not needed

- DEC-800 option B (G09b request 3, item 71): the entry already says that the reference settings writer re-emits the blank and comment lines of the loaded file, which serves as its template. No change.
- DEC-016 (items 9 and 16) lives in `verdicts_and_fixes.md`, which X-DEC-P4a owns, but the brief gives the reconciliation to X-DEC-P4b. X-DEC-P4a left DEC-016 unchanged; X-DEC-P4b's hand-off text for it (the order within an actor-grid cell sorted by a stable key, in place of "then cell slot") can be applied to the entry as it stands now.
