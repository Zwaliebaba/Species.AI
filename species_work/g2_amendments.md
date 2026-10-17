# Gate G2 amendments (coordinator list for the reconciliation agent X-DEC-R)

Source: RUN_LOG Phase 2 entries plus the "extra facts" and "cross-document requests" sections of `reports/X-DEC-1.md`, `X-DEC-2a.md`, `X-DEC-2b.md`, `X-DEC-2c.md`, `X-DEC-3.md`, `X-DEC-4a.md`, `X-DEC-4b.md`, `X-DEC-5a.md`, `X-DEC-5b.md`, `X-DEC-5c.md`, `X-ADR-a.md`, `X-ADR-b.md`.

## A. Specific fixes (do all)

1. **DEC-011 vs DEC-652.** DEC-011 says the actor-group offset table is filled on demand; DEC-652 (Resolved) fills it during match setup. Align DEC-011's wording with DEC-652 and cross-cite.
2. **DEC-101 (seasonal theme, Open)** still contains a per-client date check, an integrity defect. Split it: add a new **Resolved** entry in the verdicts range (the next free id from DEC-118) stating that the seasonal state is decided once per match and identical for all participants. DEC-101 keeps the open design choice and cites the new entry.
3. **DEC-102** covers the defects F1–F33 but not the window quirks Q1–Q10 (see `reports/X-DEC-5b.md`). Extend it, or add a Resolved companion entry from DEC-118.
4. **"Simulation code never issues player commands"** exists only in Open DEC-202. It is an integrity rule, and integrity rules are always Resolved. Add a **Resolved** entry (next free id from DEC-118) and make DEC-202 cite it.
5. **DEC-455** may overlap X-DEC-3 entries (from 04-U11, 05-U2, 05-U11: find them through `species_work/dec_map_X-DEC-3.csv`, column `orig_ids`) and the update-order entries DEC-651/652. Where two entries decide the same thing, keep the more complete one. Turn the other into `Status: Dropped` with "Superseded by DEC-nnn", and keep its id (ids are never deleted or renumbered). Where they complement each other, cross-cite.
6. **DEC-752 vs DEC-567.** If DEC-567 already covers stale effects on toggle, reduce DEC-752 to the crash, and cross-cite.
7. Cross-cite **DEC-569 ↔ DEC-751** and **DEC-555 ↔ DEC-653**.
8. **DEC-087 and DEC-112.** DEC-087 names only some of the bot fixes, and the B11 fix depends on Siege bot Transport behaviour that never runs in the reference, so DEC-112 bot calibration must explicitly cover Siege.
9. **DEC-321–323 vs DEC-097**: check for overlap, then cross-cite or supersede as in item 5.
10. **06-U9** (orders given with no Workers under control): confirm an entry covers it (`dec_map_X-DEC-3.csv`). If none does, add an Open entry in the open-questions range from DEC-343.
11. **The Collector/Builder loop that can spin on an invalid entry** (a possible hang) must be **Resolved** wherever it appears (DEC-014 and any X-DEC-3 entry).
12. **H7** (the obstacle-grid band around Barrier Emitter lines): confirm DEC-088 covers it. If it does not, extend DEC-088.
13. **ADR index (`design_next/ADR/README.md`):**
    - Reconcile the user-data path. The README says `%LOCALAPPDATA%\Species`; the ledger and ADR-071 may say `<STUDIO>\Species\`. Use one form everywhere (`%LOCALAPPDATA%\<Studio>\Species\`, where `<Studio>` is the publisher-name parameter), and update ADR-071 and the README.
    - Fill the "Decided in" column of the baseline table with the 0050+ ADR numbers.

## B. Extra facts (apply each where it is consistent)

For every "extra fact for DEC-nnn" listed in the reports above, add it to that entry's "Specified behaviour", or to its "Constraints and lessons" field for deferred entries. Do so only when it is consistent with the entry. When it conflicts with the entry, do not change the entry; list the conflict in your report. Examples: the DEC-016 port-occupant tie key; DEC-111 Collector throughput; DEC-002's 10h + 1 being only the upper bound; the X-DEC-4a list for DEC-002, 007, 011, 014, 015, 016, 018, 028, 033, 073, 079, 081, 084, 092, 117 and 241; the X-DEC-5b and X-DEC-5c lists.

## C. Owner-review list (no change; just collect)

Collect, in `species_work/reports/X-DEC-R.md` §"For the owner", the interpretations that agents made and the coordinator accepted:
- X-DEC-1's list: the "Decide" rows as Open, F35, F20, the 21-tick recounts, and 41 ranks 7, 12 and 14;
- X-DEC-3's 10 unconfirmed readings.
Give one line each.
