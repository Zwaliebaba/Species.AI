# X-DEC-P4a report: decision triage, gameplay

## Files written

| File | Size (bytes) | What changed |
|---|---|---|
| `design_next/decisions/gameplay_gap_rules.md` | 34,453 | new file, title "Gap rules: gameplay": 22 entries DEC-850 to DEC-871 (19 Open, 2 Resolved, 1 Dropped), index table |
| `design_next/decisions/verdicts_and_fixes_2.md` | 6,211 | new owner-level Open entries DEC-122 (item 50) and DEC-123 (item 72); index rows; stale paths of DEC-120 and DEC-121 |
| `design_next/decisions/verdicts_and_fixes.md` | 119,418 (was 119,482) | 30 amendments, net −64 bytes (see below) |
| `design_next/decisions/open_questions.md` | 44,145 | DEC-303, DEC-304, DEC-311; stale paths |
| `design_next/decisions/units_buildings_crates_economy.md` | 26,556 | DEC-453, DEC-459 (now Resolved, index row too), DEC-465; stale paths |
| `design_next/decisions/controls_bots_maps_meta_audio.md` | 33,250 | DEC-550, DEC-551, DEC-565, DEC-567; stale paths |
| `species_work/dec_map_X-DEC-P4a.csv` | 24 rows | one row per new entry (DEC-122, DEC-123, DEC-850 to DEC-871) |
| `species_work/p4_dec_triage_X-DEC-P4a.csv` | 156 rows | 68 new, 52 amend, 14 merged, 22 none |
| `species_work/p4_dec_handoff_X-DEC-P4a.md` | 4 items | H-1 to H-3 for DEC-652 and DEC-653, H-4 for DEC-229 (all files of X-DEC-P4b) |
| `species_work/scripts/X-DEC-P4a_*.py` | 6 scripts | requirement lookup, id check, path repointing, exact-replacement edits, CSV tables |

Checks: both pattern scans print nothing for every file above. `check_design.py --skip-mermaid`: gate 6 has no citation of a missing DEC; its only failures are Open DECs not yet cited by a requirement (my 21 new Open entries, plus X-DEC-P4b's), which the document fixers add from the `cite_in` column. Gate 3 has one failure: the generated `decisions/README.md` still links the old DEC-085 anchor.

## New entries

| DEC | Title | Status | From |
|---|---|---|---|
| DEC-122 | Bot and deputy actions outside the player command catalogue | Open, design phase | item 50, G07-1 |
| DEC-123 | Tutorial for the first playable | Open, design phase | item 72, G11-1 |
| DEC-850 | Match-rule gap rules | Open | G01-1, G01-2 |
| DEC-851 | Population-flow rule details | Open | G05-8, G05-10 |
| DEC-852 | Game-speed divisors in Worker rules | Open | G02a1-1 |
| DEC-853 | Worker and Trooper rule details | Open | G02a1-2 to 4, G02a2-3 |
| DEC-854 | Transport, Collector and Strike team rule details | Open | G02a2-4, 6, 7, 9, 10 |
| DEC-855 | Creature and hazard rule quirks | Open | G02b-1 to 5, 7 |
| DEC-856 | Creature presentation quirks | Open | G02b-6 |
| DEC-857 | Attempt caps of the bounded searches | Resolved | item 34, G02b caps, G07-5 |
| DEC-858 | Empty outcome lists | Open | G04-1, G05-5, item 5 |
| DEC-859 | Tick counts chosen for reference durations | Open | G04-4, 5, 7, G03b-3b, G01-5 |
| DEC-860 | Structure rule details | Open | G03a-1, 3, 4, 5 |
| DEC-861 | Bot rule quirks | Open | G07-6 to 8 |
| DEC-862 | Bot rule details | Open | G07-2 to 4, 9, 10 (item 51) |
| DEC-863 | Control details of the corrected command set | Resolved | G06-1, 2, 3, 5 |
| DEC-864 | Pointer fallback and camera smoothing | Open | G06-4, 6 |
| DEC-865 | Map data without a reference case | Open | G08a-1, 2, 3, 6, 7, 9 |
| DEC-866 | Population-cap values of a map | Open | G08a-5, 8 |
| DEC-867 | Closed player slots | Open | G09a-1, G08a-4 |
| DEC-868 | Front-end reference quirks | Open | G09a-2 to 4 |
| DEC-869 | Reference sound options without a Species counterpart | Open | G09b-1 |
| DEC-870 | Tutorial director details | Open | G11-4 |
| DEC-871 | Essence orb break operation | Dropped | G02a2-5 |

## Amendments to existing entries

- **Status changes:** DEC-085 Open → Resolved, retitled "Builder Essence-orb delivery to friendly Spawn Nodes" (arbitration A-3, text as A-3 gives it); DEC-116 Open → Resolved (item 12: the start hold means no crate drops in the grace period); DEC-459 Open → Resolved (G03a-7: DEC-561's Siege validation applies option B).
- **verdicts_and_fixes.md:** DEC-002 replayed ranges 21–30, 21–40, 301–500, 91–110 (item 4); DEC-011 Source Node id wording (item 14); DEC-014 cites DEC-857 and DEC-858 (items 5, 34); DEC-017 38 options (item 22); DEC-021 Essence Grove (item 6); DEC-024 building focus and approach as timeline operations, fly mode as developer tool (item 101); DEC-036 grace sentence (item 12); DEC-044 no longer covers separate volumes (item 71); DEC-050 automatic-camera control (G06-7); DEC-064 limited to recounts re-armed from the match clock, bot countdowns named as not covered (A-12, coordinator note); DEC-065 consequence for three Flag Chain maps (item 3); DEC-087 Siege Transport wording, pointers to DEC-862 and DEC-857 (item 51, G07-11); DEC-092 Seed Pod gravity; DEC-097 lost sting (G10-1); DEC-102 percent sign in text fields and one colour-row availability rule (items 93, 98, G09a-5); DEC-103 director placement and tutorial team (items 5, 74); DEC-111 Affects; DEC-112 wording. To stay within the file's size, the Affects lists of DEC-017, 019, 020, 021, 026, 028, 082 and 088 name their folder instead of every file in it.
- **Other files:** DEC-303 Affects; DEC-304 every team (item 1); DEC-311 turret and Hive facts, controller position (G03b-1); DEC-453 Collector delivery capped (DEC-080), Builder delivery uncapped (G02a2-8, A-3); DEC-465 p test (item 47); DEC-550 airstrike wording, Sickness removed from the one-shot list (it is never a Power); DEC-551 Transport case limited to a tutorial (A-9); DEC-565 absorb removed (G10-2); DEC-567 omitted DSP parameter takes its declared default (item 89).
- **Stale paths:** every Affects line of my files now names `creatures.md` or `hazards_and_projectiles.md` and the folder paths of 06, 07, 10, data schemas and VFX (items 32, 36, 55, 56, 79, 90).

## Items of phase4_dec_fixes.md

1 amend DEC-304 · 2 merged into DEC-120 (O19; G05 R-ECON-093 fix) · 3 amend DEC-065 · 4 amend DEC-002 · 5 amend DEC-014, DEC-103, new DEC-858 · 6 amend DEC-021 · 9/16 left to X-DEC-P4b (DEC-016 unchanged) · 12 amend DEC-116, DEC-036 · 14 amend DEC-011 · 19 none (checked) · 20 none (A-1) · 22 amend DEC-017 · 24 hand-off H-4, DEC-123 · 25 amend DEC-085 (A-3) · 27, 31, 35, 43, 58, 63, 69 merged into the report rows · 34 new DEC-857 · 47 amend DEC-465, DEC-550, hand-off H-1 · 50 new DEC-122 · 51 amend DEC-087, new DEC-862 · 52 none (A-12), DEC-064 text narrowed · 59, 60, 67 DEC right, document fixes · 70 none · 71 amend DEC-044, new DEC-869, DEC-800 already correct · 72 new DEC-123 · 73 DEC-050 right, routing fix · 74 amend DEC-103 · 81 amend DEC-097, DEC-565 · 89 amend DEC-567 · 93 amend DEC-102 · 98 amend DEC-102, document fixes · 101 amend DEC-024 (DEC-803 is X-DEC-P4b's) · 48 (A-9, A-10) amend DEC-551, note in DEC-859.

Every report request (G00 to G11) has its own row in the triage table.

## Not done, and notes for the coordinator

- `decisions/README.md` must be regenerated: new file `gameplay_gap_rules.md` (22 entries), `verdicts_and_fixes_2.md` (4 entries), the new title of DEC-085 and three status changes. `dec_map.csv` needs the 24 new rows and the repointed `affects_files`.
- Items 89 and 93 are in X-DEC-P4b's list too; I amended DEC-567 and DEC-102 because they live in my files. Merge any X-DEC-P4b hand-off for them, and for DEC-016 (items 9, 16), onto the current text.
- DEC-085 follows A-3 exactly; no other arbitration entry needs a DEC, and A-19 to A-24 hold no gameplay design choice for me.
- The 21 new Open entries need their citations from the document fixers (the `cite_in` and `document_fix` columns). The readings in G02b and G03b were triaged "none" and left to the fidelity review.
