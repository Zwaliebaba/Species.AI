# X-ROUTE-a report: cross-document request routing (reference reports G00–G11)

## Files written

| File | Size | Content |
|---|---|---|
| `species_work/p4_routes_X-ROUTE-a.csv` | 172 KB | 325 routes RA-001 to RA-325, columns `route_id, source, to_package, to_file, kind, instruction, status, note` |
| `species_work/scripts/X-ROUTE-a_append.py` | 2.2 KB | appends JSON rows with the next RA id; refuses a `done` or `superseded` row without evidence |
| `species_work/scripts/X-ROUTE-a_edit.py` | 1.7 KB | edits fields of existing rows (whole value or one exact substring) |
| `species_work/scripts/X-ROUTE-a_find.py` | 2.4 KB | finds where a requirement id or tuning key is defined and which files use it |
| `species_work/scripts/X-ROUTE-a_glo.py` | 1.5 KB | tells whether a term has a glossary entry, and which glossary file it belongs in |
| `species_work/reports/X-ROUTE-a.md` | this file | |

No file under `design_next/` was edited. No git command and no publishing tool was used.

## Method

1. I read the "Cross-document requests" section of all 17 G reports (G00, G01, G02a1, G02a2, G02b, G03a, G03b, G04, G05, G06, G07, G08a, G08b, G09a, G09b, G10, G11). Each one became one row or more, one row per target file and per separate instruction. Bundled requests were split.
2. I took the items of `phase4_dec_fixes.md` that ask a **reference document** to change. I counted `reference/12_simulation_model/` as a reference document (prefix R-SIM), so items 13, 17, 68 and 74 are mine. Where an item has several targets, I routed only the parts aimed at reference documents and left the component, glossary and CONVENTIONS parts to X-ROUTE-b.
3. For every row I opened the target file and searched for the cited id, key or statement (`X-ROUTE-a_find.py`, `X-ROUTE-a_glo.py`, grep). Rows marked `done` give the evidence (file, line or id) in `note`. Rows marked `superseded` name the X-ARB entry (A-n) or the route that replaces them.
4. **Ownership.** `to_package` is taken from `species_work/packages/*.json`. Glossary rows go to `X-GLO`, asset-manifest rows to `X-AST`, the one name registration to `ledger`, and CONVENTIONS, package-index and dec_map rows to `coordinator`. Rows about decision files name the Phase 3 package (X-DEC-1, X-DEC-2a, X-DEC-5a, X-DEC-5b) and say in the note which Phase 4 agent now owns the file.
5. **Re-checks at the end.** X-DEC-P4a/b, X-AST pass 2 and X-ARB were editing files while I worked. At the end I re-checked every row that targets decisions or assets, and every row that touches an X-ARB question. The rows those agents had already closed are now `done` or `superseded`, and their notes say so. Reference documents, components and the glossary did not change while I routed, apart from two component files that X-AST touched.

## Counts

- **325 routes:** 195 open, 113 done, 17 superseded.
- **By kind:** fix 112, cite 90, add 88, glossary 24, remove 7, conventions 3, ledger 1.
- **Open routes by package:** G05 20, G03a 15, G00 11, G01 11, C-SIM-b 10, G06 9, G02a1 8, G02a2 8, G02b 8, G03b 8, G04 8, G09a 8, C-SIM-c 8, C-SIM-a 6, G07 5, G09b 5, G08a 4, X-AST 4, coordinator 4, C-RND-a 3, C-DAT 2, G11 2, C-RND-b 1, G08b 1, G10 1, ledger 1, X-GLO 24 (missing glossary terms and five entry corrections).

## Per report: request → routes

Requests are numbered in the order of each report's "Cross-document requests" section. (d) = done, (s) = superseded, unmarked = open.

| Report | Requests and routes |
|---|---|
| G00 | #1: RA-001, RA-003, RA-004, RA-005, RA-324; #2: RA-006 (d); #3: RA-007 (d); #4: RA-008; #5: RA-009 (d); #6: RA-010, RA-011 (d); #7: RA-012, RA-013 |
| G01 | #1: RA-007 (d), RA-010, RA-011 (d); #2: RA-014 (d), RA-015, RA-033; #3: RA-016, RA-017, RA-018; #4: RA-017; #5: RA-019 (s), RA-020 (s); #6: RA-021, RA-022; #7: RA-023 (d), RA-024; #8: RA-025 (d), RA-230; #9: RA-026 (d); #10: RA-027; #11: RA-028; #12: RA-029; #13: RA-031, RA-032 |
| G02a1 | #1: RA-058, RA-059 (d); #2: RA-060 (d); #3: RA-034, RA-050 (s); #4: RA-061, RA-062, RA-063; #5: RA-002; #6: RA-064, RA-065 (d), RA-066; #7: RA-067; #8: RA-068, RA-070 (d); #9: RA-071 (d); #10: RA-072 (d); #11: RA-073; #12: RA-074 (d); #13: RA-075 (d); #14: RA-076 (d); #15: RA-077, RA-078 |
| G02a2 | #1: RA-058, RA-079, RA-080, RA-081 (d); #2: RA-034, RA-044, RA-045, RA-046, RA-051 (s); #3: RA-051 (s), RA-083; #4: RA-084 (d); #5: RA-085; #6: RA-086; #7: RA-087 (d); #8: RA-090; #9: RA-091, RA-092 (d); #10: RA-093; the "base keys" section: RA-082 |
| G02b | #1: RA-094 (d), RA-095 (d); #2: RA-096 (d), RA-097; #3: RA-098; #4: RA-097; #5: RA-099, RA-325; #6: RA-100, RA-101 (d); #7: RA-102; #8: RA-103 (d); #9: RA-104 (d), RA-105; #10: RA-035; #11: RA-106 (d); #12: RA-107 (d), RA-108 (d), RA-109, RA-110 (d) |
| G03a | #1: RA-122, RA-123; #2: RA-036, RA-039, RA-040; #3: RA-124 (s), RA-125; #4: RA-126 (d), RA-127; #5: RA-128 (d); #6: RA-129 (d); #7: RA-130 (d), RA-131; #8: RA-132, RA-133 (s); #9: RA-134; #10: RA-135 (d); #11: RA-136 (d); #12: RA-138; #13: RA-140 (d); #14: RA-141; #15: RA-142, RA-143 |
| G03b | #1: RA-144, RA-145; #2: shared-key list, no request; #3: RA-036, RA-054; #4: RA-147 (d), RA-148 (d), RA-149 (s); #5: RA-116; #6: RA-150 (s); #7: RA-122; #8: RA-151; #9: RA-152 (d); #10: RA-153 (d), RA-154; #11: RA-155; #12: RA-157 (d); #13: RA-158; #14: RA-146 (d); #15: RA-159, RA-160 |
| G04 | #1: RA-008; #2: RA-037, RA-047, RA-056, RA-057; #3: RA-023 (d), RA-024, RA-030; #4: RA-161 (s); #5: RA-112 (s), RA-113, RA-114, RA-115; #6: RA-149 (s), RA-162 (s), RA-163; #7: RA-064; #8: RA-164 (d); #9: RA-165 (d); #10: RA-169, RA-171 (d); #11: RA-172 (d), RA-173; #12: RA-174, RA-175 (d); #13: RA-176 (d); #14: RA-177 (d); #15: RA-178, RA-179 |
| G05 | #1: RA-180 (d); #2: RA-181 (d); #3: RA-182 (d); #4: RA-183 (d); #5: RA-184 (d); #6: RA-185 (d); #7: RA-186 (d), RA-033; #8: RA-187 (d); #9: RA-188; #10: RA-189 (d); #11: RA-190 (d); #12 (its own mirror keys, item 8): RA-033 to RA-038 |
| G06 | #1: RA-191 (d), RA-192 (d), RA-193, RA-194 (d), RA-197, RA-198; #2: RA-199 (d); #3: RA-200; #4: RA-201 (d); #5: RA-166, RA-167, RA-168 (d); #6: RA-202, RA-203 (d); #7: RA-009 (d); #8: RA-067; #9: RA-086; #10: RA-204 (d); #11: RA-205; #12: RA-206 (d); #13: RA-207 (d); #14: RA-208 (d); #15: RA-209, RA-210 (d); #16: RA-211 (d); #17: RA-212; #18: RA-213, RA-214 |
| G07 | #1: RA-194 (d), RA-195 (d), RA-197, RA-215 (d); #2: RA-133 (s); #3: RA-132; #4: RA-018; #5: RA-158; #6: RA-169, RA-170; #7: RA-088, RA-089 (s), RA-216; #8: RA-068, RA-069; #9: RA-217, RA-218; #10: RA-219; #11: RA-041; #12: RA-027; #13: RA-221; #14: RA-222 (d); #15: RA-223 (d); #16: RA-225 (d); #17: RA-226, RA-227 |
| G08a | #1: RA-025 (d), RA-228, RA-229, RA-230; #2: RA-042, RA-231 (d); #3: RA-135 (d), RA-232 (s); #4: RA-233 (s); #5: RA-234 (d); #6: RA-235 (d); #7: RA-236 (d); #8: RA-237 (d); #9: RA-239 (d); #10: RA-240; #11: RA-241 (d); #12: RA-242, RA-243; #13: RA-244 (d) |
| G08b | #1: RA-048; #2: RA-247; #3: RA-248, RA-249; #4: RA-250 (d); #5: RA-251; #6: RA-245, RA-252 (d); #7: RA-253; #8: RA-254 (d); #9: RA-255 (d) |
| G09a | #1: RA-256 (d); #2: RA-257; #3: RA-258, RA-259; #4: RA-260 (d), RA-261; #5: RA-262 (d); #6: RA-263; #7: RA-264 (d); #8: RA-265 (d); #9: RA-001, RA-010; #10: RA-238; #11: RA-209; #12: RA-266 |
| G09b | #1: RA-267; #2: RA-268; #3: RA-269 (d), RA-270; #4: RA-272, RA-273 (d); #5: RA-274; #6: RA-275 (d), RA-276; #7: RA-277 (d); #8: RA-270, RA-278 (d); #9: RA-279; #10: RA-280; #11: RA-281; #12: RA-282 |
| G10 | #1: RA-194 (d), RA-195 (d), RA-196 (d), RA-197, RA-283 (d), RA-284 (d); #2: RA-285; #3: RA-174; #4: RA-270, RA-271; #5: RA-028; #6: RA-286 (d); #7: RA-090; #8: RA-109; #9: RA-137, RA-287 (d); #10: RA-155, RA-156; #11: RA-204 (d); #12: RA-288 (d); #13: RA-289 (d); #14: RA-290, RA-291 |
| G11 | #1: RA-292, RA-293, RA-294; #2: RA-138, RA-139; #3: RA-295; #4: RA-029; #5: RA-111; #6: RA-049, RA-036; #7: RA-205; #8: RA-224; #9: RA-296, RA-297; #10: RA-279, RA-298 (d), RA-299, RA-300 (d); #11: RA-301 (d); #12: RA-302 (d); #13: RA-303; #14: RA-304, RA-305 |

Rows RA-053 to RA-057 are follow-ups that the reference/05 mirror removal forces on the documents that cite the mirrors: reference/03 README, trees_and_crate_structures.md, creatures.md, reference/09 screen_flow and reference/11.

## Items of `phase4_dec_fixes.md`

**Routed (the target is a reference document):**
- item 4 (the drift in the overview): RA-003
- item 6: RA-183 (d)
- item 7: RA-001
- item 8: RA-033 to RA-038
- item 13: RA-021
- item 17: RA-220
- item 18: RA-015, RA-042, RA-043, RA-052
- item 19: RA-115, RA-117 to RA-121
- item 22 (the overview count): RA-324
- item 26: RA-079, RA-080
- items 28 and 105: RA-058
- item 30: RA-002
- items 32 and 36: RA-094 (d), RA-095 (d)
- item 33: RA-079
- item 45: RA-144
- items 46 and 49: RA-147 (d; X-ARB A-6 closes item 46 with no change)
- item 52: RA-133 (s)
- item 53: RA-169
- item 55: RA-215 (d)
- item 56: RA-192 (d), RA-193
- item 57: RA-202
- item 58 (the three G04 conflicts): RA-166, RA-167, RA-168 (d)
- item 59: RA-042, RA-242
- item 60: RA-229
- item 67: RA-257
- item 68: RA-258, RA-259
- item 69: RA-260 (d), RA-262 (d)
- item 73: RA-303, RA-323 (coordinator decision)
- item 74: RA-292
- item 79: RA-283 (d), RA-284 (d)
- item 81: RA-174, RA-271
- item 94 (the G09b part): RA-274
- item 98: RA-306, RA-307 (s, X-ARB A-22), RA-308
- item 101: RA-245, RA-246
- item 106: RA-309 to RA-314
- item 109 (the reference-document parts): RA-315 to RA-318
- item 114: RA-212, RA-319, RA-320
- item 117 (the G09b part): RA-272
- item 118: RA-321 (s, A-19), RA-322 (s, A-20)

**Not routed here (not a request for a reference-document change):**
- **DEC entries** (X-DEC-P4a/b): 1, 2, 3, 5, 9 to 12, 14 to 16, 24, 27, 31, 34, 35, 41, 43, 47, 50, 51, 63, 66, 71, 72, 78, 91 to 93, 97, 99, 111, 115, and the DEC parts of 4, 22, 89, 101, 103 and 118.
- **ADRs** (X-ADR-P4): 37 to 40, 64, 70, 80, 84, 87, 88, 95, 102, 108, 112, 116, 119.
- **Assets** (X-AST pass 2): 23, 44, 82, 83, 85, and the asset parts of 27, 35, 41, 58, 66, 69, 86, 97, 109 and 117.
- **X-ARB:** 20, 21, 25, 29, 42, 46, 48, 54, 61, 62, 77, 103, and the R-META-044 point of 98. Where a G report raised the same question, the route is `superseded` and names the A-n entry: RA-019, RA-020, RA-050, RA-051, RA-089, RA-112, RA-124, RA-133, RA-149, RA-150, RA-161, RA-162, RA-232, RA-233.
- **Component, glossary or CONVENTIONS targets** (X-ROUTE-b): 65, 76, 90, 96, 100, 113, 120, and the component parts of 86, 89, 94, 109, 114, 117 and 119.
- **Already closed:** 49, 75, 104, 107, 110.

## Points for the coordinator

1. **Items 28 and 105 point in opposite directions** for the eight per-kind stat keys that are defined twice in reference/02. RA-058 follows item 28, item 33 and G02a1's request: the unit files cite the README keys. The README stat table is the one that reference/12 R-SIM-360 delegates to, and it also holds the G02b kinds. Please confirm this direction before G02a2 applies the route. The same row lists four hidden duplicates found while routing: `ref.unit.transport.speed_max`, `ref.unit.transport.tint_scale`, `ref.unit.trooper.fire_rate_s` and `ref.unit.trooper.reload_ticks`.
2. **reference/05 has the most open work (20 rows) and is 115.4 KB.** RA-015 (DEC-081), RA-042 (DEC-079) and RA-043 (DEC-080) are rewrites. They must replace text rather than add it, or the file needs a split.
3. **Owner-resolved DECs leave gaps that the document fixers cannot fill:**
   - DEC-081 gives no formula for the proportional curve (RA-015).
   - DEC-080 does not say what happens to orbs that are not converted at the cap (RA-043).
   - DEC-079's option B says "pooled within alliances" while its specified behaviour says "Alliances are not pooled".

   These are for X-DEC-P4a.
4. **Conflicts found while routing** (not on the item list). Each is routed to an owner, with "refer to X-ARB if the inputs do not settle it":
   - Frenzy and the Fury damage factor: R-UNIT-125 against R-CRATE-213 (RA-066).
   - The Flag post stand distance: 20 wu in R-UNIT-095 against 40 wu in reference/03 (RA-073).
   - The overview's 39 option definitions against reference/01's 38 (RA-324).
   - R-VIS-124 contradicts Resolved DEC-107, owner decision O18 (RA-117).
   - When the second tutorial switches basic crates on (RA-030).
   - What triggers the Transport's route freeing in R-SIM-357 (RA-093).
   - The bot controller's initialisation draws (RA-219).
   - Three Power-rule readings that differ between reference/06 and reference/04 (RA-166).
5. **Overlap with X-ROUTE-b.** Items 17, 98 and 109 repeat requests from the C-SIM-a, C-UI-b and C-BLD reports, which X-ROUTE-b routes from those reports. The notes say "deduplicate" (RA-220, RA-306, RA-308, RA-315 to RA-318).
6. **The overview's mirror keys (RA-001)** list the owner keys that the owners named. Five value drifts are routed on their own: RA-002 to RA-005 and RA-324.
7. **Smaller coordinator rows:**
   - CONVENTIONS §8.11 needs the action-id pattern `action.<group>.<name>` (RA-198).
   - `species_work/packages/INDEX.md` still lists Phase 3 single-file targets (RA-266).
   - 17 `10_audio_design.md` paths remain in `dec_map.csv` (RA-197).
   - The objective-card gap needs a decision (RA-323, item 73).
   - The slot open/close command name needs registering in the ledger (RA-259).
   - RA-290: check whether "KH-MUSIC" is a routing label before the glossary defines it.

## What I could not do

- I did not decide any conflict that needs the reference itself to settle. Such conflicts are either superseded by an X-ARB entry or flagged for X-ARB in the note.
- I did not write a complete mapping of every `ref.vis.*` overview key to its owner key (about 140 keys). RA-001 gives the owner keys that the owners named and the rule to follow; the G00 fixer looks up the rest in the owner documents named by each §4 group heading.
- Routes that depend on concurrent Phase 4 work stay open with that dependency in the note, because the outcome is not yet written:
  - X-DEC-P4b for DEC-024 and DEC-803 (RA-245, RA-246);
  - X-DEC-P4a for G09a's DEC request 4 (RA-238);
  - the G09a decision on the map-editor label (RA-249).

## Checks

`python3 -I species_work/scripts/scan_patterns.py species_work/forbidden_patterns.tsv` and the same command with `soft_patterns.tsv`, run on `species_work/p4_routes_X-ROUTE-a.csv` and on this report: both print nothing.
