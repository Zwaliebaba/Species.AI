# X-DEC-3 report: decision register from the open-question register

Phase 2, wave 2b. Brief §9.2 X-DEC. Input: `ORIG/41` §3 (package lines 209–603 with the gaps the package leaves to X-DEC-2a/2b/2c). ID range DEC-300 to DEC-449; DEC-300 to DEC-342 used, DEC-343 to DEC-449 free.

This is a WORK file: it names original question ids on purpose. The design file names none.

## Files written

| File | What |
|---|---|
| `design_next/decisions/open_questions.md` | heading, scope paragraph, generated index table, 43 entries in the §6.5 format, about 44 KB (no split needed) |
| `species_work/dec_map_X-DEC-3.csv` | one row per entry: `dec_id, title, status, affects_prefix, affects_files, orig_refs, orig_ids` (csv module, `lineterminator='\n'`); `orig_refs` are `41_risks_and_open_questions.md:start-end`, merge-pointer lines included |
| `species_work/scripts/X-DEC-3_build.py` | validates the register (sequential ids in range, field order per status, "Decide by" only on Open, every Affects path is a package target, every DEC and ADR reference exists), regenerates the index, writes the CSV and renders this report from its template; holds the question-to-DEC mapping |
| `species_work/reports/X-DEC-3.md` | this report |

Nothing was published or uploaded: no Artifact, Docs, Gamma, Drive or GitHub tool was used. No git write command was run.

## Counts

| Status | Entries |
|---|---|
| Open | 31 |
| Resolved | 1 |
| Dropped | 11 |
| **Total** | **43** |

Package question rows: 208 (280 question ids including merged ids on the same row); 94 rows map to an entry of this register, 114 rows are covered by an existing DEC or ADR. Merge-pointer lines: 19.

Requirements, tuning keys, diagrams: 0 (a register carries none). Enumerations: 0.

Every Open entry has "Decide by" (design phase or playtest). The Resolved entry and the Dropped entries have none. No entry is Open without a stated default: the specified behaviour of each Open entry is the reference behaviour, written as the most conservative reading the corpus supports; where the corpus leaves a behaviour unestablished, the entry says so and the default adds no special case.

## Entries and their source questions

| DEC | Title | Status | Question ids |
|---|---|---|---|
| DEC-300 | Return of teams wiped out in sudden death | Open | 01-U12 |
| DEC-301 | Solar Array knock-out in Siege | Open | 03-U16 |
| DEC-302 | Transport unload order over sea | Open | 02-U15 |
| DEC-303 | Missile Barrage arrival spread | Open | 02-U16 |
| DEC-304 | Spawn Surge request count | Open | 05-U11 |
| DEC-305 | Essence Grove orbs of team slots not in play | Open | 04-U5 / 03-U10; 03-U10 (merged into 04-U5) |
| DEC-306 | Spawn-network transit time and update order | Open | 05-U2 |
| DEC-307 | Essence orbs held by a removed Collector | Open | 05-U3 / 02-U9; 02-U9 (merged into 05-U3) |
| DEC-308 | Orders to Workers that operate a port | Open | 05-U9 |
| DEC-309 | Global flag on Source Nodes | Open | 03-U17 |
| DEC-310 | Camera tracking after selecting a Leader or a Transport | Open | 06-U10 |
| DEC-311 | Bot controller as a target | Open | 07-U7 / 02-U13; 02-U13 (merged into 07-U7) |
| DEC-312 | Bot decisions and node recounts within a tick | Open | 07-U12 |
| DEC-313 | Dead airborne actors | Open | 21-U11 |
| DEC-314 | Map list order under ties | Open | 08-U4 |
| DEC-315 | Level-design patterns of the reference maps | Open | 08-U12 |
| DEC-316 | Front-end text blend and page transition | Open | 09-U6; 28-U1; 28-U2 |
| DEC-317 | Voice set language | Open | 10-U1 / 25-U9; 25-U9 (merged into 10-U1) |
| DEC-318 | Sound triggers tied to the frame rate | Open | 10-U14 |
| DEC-319 | Ambience during loading views | Open | 10-U15 |
| DEC-320 | Match-end pitch drop | Open | 10-U16 |
| DEC-321 | Attached sounds when their emitter is removed | Open | 27-U5 / 10-U6; 10-U6 (merged into 27-U5) |
| DEC-322 | Start value of smoothed sound parameters | Open | 27-U10 / 10-U13; 10-U13 (merged into 27-U10) |
| DEC-323 | Loop keep-alive polling | Open | 27-U14 / 10-U11; 10-U11 (merged into 27-U14) |
| DEC-324 | Terrain generator parameter semantics and biases | Open | 23-I3; 23-I4 |
| DEC-325 | Light direction and the camera transform | Open | 24-U14 |
| DEC-326 | Terrain grid overlay | Open | 24-U15 |
| DEC-327 | Structure render-detail variants | Open | 09-U10 |
| DEC-328 | Saturated team colours on effect bubbles | Open | 26-U8 |
| DEC-329 | Assets of objects spawned only at run time | Open | 25-U3 |
| DEC-330 | Mesh sockets referenced only by data | Open | 29-U4 / 25-U10; 25-U10 (merged into 29-U4) |
| DEC-331 | Confirmed facts: the convert tag and the victory celebration | Resolved | 03-U5; 10-U8 |
| DEC-332 | Other editions, packaged archives and evidence questions | Dropped | 06-U11; 08-U1 / 29-U1; 08-U3; 08-U11; 10-U2 / 25-U8; 25-U1 / 00-U8 / 29-U9 / 31-U7; 25-U2; 31-U10; 32-U8; 00-U8 (merged into 25-U1); 25-U8 (merged into 10-U2); 29-U1 (merged into 08-U1); 29-U9 (merged into 25-U1); 31-U7 (merged into 25-U1) |
| DEC-333 | Trial-edition gating and credential files | Dropped | 08-U6; 09-U2 / 22-U16 / 31-I4 / 31-U5; 09-U17; 28-U9; 31-U5 (merged into 09-U2); 31-I4 (merged into 09-U2) |
| DEC-334 | Development, test and benchmark maps | Dropped | 03-U3 / 02-U17 / 04-U9; 08-U5; 32-U3 / 08-U2; 02-U17 (merged into 03-U3); 04-U9 (merged into 03-U3); 08-U2 (merged into 32-U3) |
| DEC-335 | Online-service listing fields | Dropped | 09-U5; 09-U7 |
| DEC-336 | Second graphics back end | Dropped | 24-U1 / 31-U1 / 31-U2; 24-U2; 24-U3; 24-U4; 24-U5; 24-U12; 24-U13 / 31-U3; 26-U6; 31-U1 (merged into 24-U1); 31-U2 (merged into 24-U1); 31-U3 (merged into 24-U13) |
| DEC-337 | Graphics-driver, cursor and glyph-atlas behaviour | Dropped | 24-U9; 24-U11; 25-U4 |
| DEC-338 | Input-stack implementation details | Dropped | 06-U1; 06-U2; 06-U3; 06-U6 |
| DEC-339 | Audio driver, software mixer and effect-processing details | Dropped | 26-U9; 27-U1 / 10-U7; 27-U3; 27-U4; 27-U6; 27-U7; 27-U9; 27-U11; 27-U12; 10-U7 (merged into 27-U1) |
| DEC-340 | Data formats, archives and production tools | Dropped | 29-U5 / 25-U7; 29-U7; 29-U8 / 26-U4; 29-U13; 31-U6; 31-U9; 31-U12; 25-U7 (merged into 29-U5); 26-U4 (merged into 29-U8) |
| DEC-341 | Runtime-library, compiler and debug-build details | Dropped | 29-U10; 32-U1 / 31-U4; 32-U4; 32-U5; 32-U6; 32-U7; 31-U4 (merged into 32-U1) |
| DEC-342 | Provenance of reference art and audio | Dropped | 08-U7; 09-U8; 10-U3 |

## Mapping: every question of the package to a DEC

"covered by" names the existing entry that already decides the question (wave 2a registers or ADRs); no entry was added for it. Merge-pointer lines resolve to the owner row's mapping.

| 41 line | Question ids | Mapped to |
|---|---|---|
| 212 | 00-U1 | merged into 09-U1: covered by DEC-047 |
| 212 | 00-U2 | merged into 01-U2: covered by DEC-002, DEC-003 |
| 212 | 00-U3 | merged into 05-U5: covered by DEC-081 |
| 212 | 00-U4 | merged into 05-U6: covered by DEC-079 |
| 212 | 00-U5 | merged into 01-U10: covered by DEC-082 |
| 212 | 00-U6 | merged into 01-U11: covered by DEC-066 |
| 212 | 00-U7 | merged into 22-U1: owner row outside this package (see below) |
| 212 | 00-U8 | merged into 25-U1: Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 212 | 00-U9 | merged into 09-U4: covered by DEC-100 |
| 212 | 00-U10 | merged into 10-U12: covered by DEC-049 |
| 218 | 01-U1 / 09-U3 | covered by DEC-067 |
| 219 | 01-U2 / 00-U2 | covered by DEC-002, DEC-003 |
| 220 | 01-U5 | covered by DEC-068 |
| 221 | 01-U6 | covered by DEC-075 |
| 222 | 01-U7 | covered by DEC-076 |
| 223 | 01-U8 / 03-U15 | covered by DEC-064 |
| 224 | 01-U9 | covered by DEC-070 |
| 225 | 01-U10 / 00-U5 | covered by DEC-082 |
| 226 | 01-U11 / 00-U6 | covered by DEC-066 |
| 227 | 01-U12 | DEC-300 Return of teams wiped out in sudden death (Open) |
| 229 | 01-U3 | merged into 03-U13: covered by DEC-108 |
| 229 | 01-U4 | merged into 09-U1: covered by DEC-047 |
| 235 | 02-U1 | covered by DEC-090 |
| 236 | 02-U3 | covered by DEC-009 |
| 237 | 02-U5 | covered by DEC-113 |
| 238 | 02-U6 / 06-U8 | covered by DEC-117 |
| 239 | 02-U7 / 03-U4 | covered by DEC-115 |
| 240 | 02-U12 | covered by DEC-014 |
| 241 | 02-U15 | DEC-302 Transport unload order over sea (Open) |
| 242 | 02-U16 | DEC-303 Missile Barrage arrival spread (Open) |
| 244 | 02-U2 | merged into 04-U4: covered by DEC-107 |
| 244 | 02-U4 | merged into 04-U4: covered by DEC-107 |
| 244 | 02-U8 | merged into 21-U4: covered by DEC-095, DEC-200 (item 5) |
| 244 | 02-U9 | merged into 05-U3: DEC-307 Essence orbs held by a removed Collector (Open) |
| 244 | 02-U10 | merged into 05-U10: covered by DEC-011 |
| 244 | 02-U11 | merged into 04-U7: covered by DEC-111 |
| 244 | 02-U13 | merged into 07-U7: DEC-311 Bot controller as a target (Open) |
| 244 | 02-U14 | merged into 21-U6: covered by DEC-028 |
| 244 | 02-U17 | merged into 03-U3: Dropped: DEC-334 Development, test and benchmark maps |
| 244 | 02-U18 | merged into 26-U5: covered by DEC-007, DEC-222 |
| 250 | 03-U1 | covered by DEC-117, DEC-079 |
| 251 | 03-U2 | covered by DEC-117 |
| 252 | 03-U3 / 02-U17 / 04-U9 | Dropped: DEC-334 Development, test and benchmark maps |
| 253 | 03-U5 | DEC-331 Confirmed facts: the convert tag and the victory celebration (Resolved) |
| 254 | 03-U6 | covered by DEC-052 |
| 255 | 03-U7 / 30-U4 | covered by DEC-015 |
| 256 | 03-U8 | covered by DEC-052 |
| 257 | 03-U9 | covered by DEC-089 |
| 258 | 03-U12 | covered by DEC-012 |
| 259 | 03-U13 / 01-U3 | covered by DEC-108 |
| 260 | 03-U14 | covered by DEC-117 |
| 261 | 03-U16 | DEC-301 Solar Array knock-out in Siege (Open) |
| 262 | 03-U17 | DEC-309 Global flag on Source Nodes (Open) |
| 264 | 03-U4 | merged into 02-U7: covered by DEC-115 |
| 264 | 03-U10 | merged into 04-U5: DEC-305 Essence Grove orbs of team slots not in play (Open) |
| 264 | 03-U11 | merged into 05-U6: covered by DEC-079 |
| 264 | 03-U15 | merged into 01-U8: covered by DEC-064 |
| 270 | 04-U1 / 07-U4 | covered by DEC-014 |
| 271 | 04-U2 | covered by DEC-114 |
| 272 | 04-U3 | covered by DEC-014 |
| 273 | 04-U4 / 02-U2 / 02-U4 | covered by DEC-107 |
| 274 | 04-U5 / 03-U10 | DEC-305 Essence Grove orbs of team slots not in play (Open) |
| 275 | 04-U6 | covered by DEC-022 |
| 276 | 04-U7 / 02-U11 | covered by DEC-111 |
| 277 | 04-U10 | covered by DEC-116 |
| 278 | 04-U11 | covered by DEC-111 |
| 279 | 04-U12 | covered by DEC-002 |
| 281 | 04-U8 | merged into 20-U6: covered by DEC-011 |
| 281 | 04-U9 | merged into 03-U3: Dropped: DEC-334 Development, test and benchmark maps |
| 287 | 05-U1 | covered by DEC-094, DEC-016 |
| 288 | 05-U2 | DEC-306 Spawn-network transit time and update order (Open) |
| 289 | 05-U3 / 02-U9 | DEC-307 Essence orbs held by a removed Collector (Open) |
| 290 | 05-U5 / 00-U3 | covered by DEC-081 |
| 291 | 05-U6 / 00-U4 / 03-U11 | covered by DEC-079 |
| 292 | 05-U7 | covered by DEC-065 |
| 293 | 05-U9 | DEC-308 Orders to Workers that operate a port (Open) |
| 294 | 05-U10 / 02-U10 | covered by DEC-011 |
| 295 | 05-U11 | DEC-304 Spawn Surge request count (Open) |
| 296 | 05-U12 | covered by DEC-111 |
| 298 | 05-U4 | merged into 22-U17: covered by DEC-011, DEC-223 (item 2) |
| 298 | 05-U8 | merged into 21-U6: covered by DEC-028 |
| 304 | 06-U1 | Dropped: DEC-338 Input-stack implementation details |
| 305 | 06-U2 | Dropped: DEC-338 Input-stack implementation details |
| 306 | 06-U3 | Dropped: DEC-338 Input-stack implementation details |
| 307 | 06-U5 | covered by DEC-008 |
| 308 | 06-U6 | Dropped: DEC-338 Input-stack implementation details |
| 309 | 06-U9 | covered by DEC-202 (item 7) |
| 310 | 06-U10 | DEC-310 Camera tracking after selecting a Leader or a Transport (Open) |
| 311 | 06-U11 | Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 312 | 06-U12 | covered by DEC-015 |
| 314 | 06-U4 | merged into 21-U4: covered by DEC-095, DEC-200 (item 5) |
| 314 | 06-U7 | merged into 22-U8: owner row outside this package (see below) |
| 314 | 06-U8 | merged into 02-U6: covered by DEC-117 |
| 320 | 07-U1 | covered by DEC-087, DEC-112 |
| 321 | 07-U2 | covered by DEC-087, DEC-112 |
| 322 | 07-U5 | covered by DEC-087 |
| 323 | 07-U6 | covered by DEC-031 |
| 324 | 07-U7 / 02-U13 | DEC-311 Bot controller as a target (Open) |
| 325 | 07-U8 | covered by DEC-087 |
| 326 | 07-U9 | covered by DEC-073 |
| 327 | 07-U10 | covered by DEC-087, DEC-112 |
| 328 | 07-U11 | covered by DEC-117 |
| 329 | 07-U12 | DEC-312 Bot decisions and node recounts within a tick (Open) |
| 331 | 07-U3 | merged into 31-U14: covered by DEC-005 |
| 331 | 07-U4 | merged into 04-U1: covered by DEC-014 |
| 337 | 08-U1 / 29-U1 | Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 338 | 08-U3 | Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 339 | 08-U4 | DEC-314 Map list order under ties (Open) |
| 340 | 08-U5 | Dropped: DEC-334 Development, test and benchmark maps |
| 341 | 08-U6 | Dropped: DEC-333 Trial-edition gating and credential files |
| 342 | 08-U7 | Dropped: DEC-342 Provenance of reference art and audio |
| 343 | 08-U8 | covered by DEC-045 |
| 344 | 08-U9 | covered by DEC-045, DEC-015 |
| 345 | 08-U10 | covered by DEC-045, DEC-015 |
| 346 | 08-U11 | Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 347 | 08-U12 | DEC-315 Level-design patterns of the reference maps (Open) |
| 348 | 08-U13 | covered by DEC-204, ADR-0060 |
| 350 | 08-U2 | merged into 32-U3: Dropped: DEC-334 Development, test and benchmark maps |
| 356 | 09-U1 / 00-U1 / 01-U4 | covered by DEC-047 |
| 357 | 09-U2 / 22-U16 / 31-I4 / 31-U5 | Dropped: DEC-333 Trial-edition gating and credential files |
| 358 | 09-U4 / 00-U9 / 29-U2 / 31-I5 / 31-U8 | covered by DEC-100 |
| 359 | 09-U5 | Dropped: DEC-335 Online-service listing fields |
| 360 | 09-U6 | DEC-316 Front-end text blend and page transition (Open) |
| 361 | 09-U7 | Dropped: DEC-335 Online-service listing fields |
| 362 | 09-U8 | Dropped: DEC-342 Provenance of reference art and audio |
| 363 | 09-U9 / 30-U8 | covered by DEC-053 |
| 364 | 09-U10 | DEC-327 Structure render-detail variants (Open) |
| 365 | 09-U13 | covered by DEC-245 (item 10) |
| 366 | 09-U16 | covered by DEC-224 (item 7) |
| 367 | 09-U17 | Dropped: DEC-333 Trial-edition gating and credential files |
| 369 | 09-U3 | merged into 01-U1: covered by DEC-067 |
| 369 | 09-U11 | merged into 22-U1: owner row outside this package (see below) |
| 369 | 09-U12 | merged into 22-U2: owner row outside this package (see below) |
| 369 | 09-U14 | merged into 28-U3: covered by DEC-099 |
| 369 | 09-U15 | merged into 30-U1: covered by DEC-110 |
| 375 | 10-U1 / 25-U9 | DEC-317 Voice set language (Open) |
| 376 | 10-U2 / 25-U8 | Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 377 | 10-U3 | Dropped: DEC-342 Provenance of reference art and audio |
| 378 | 10-U4 | covered by DEC-097 |
| 379 | 10-U8 | DEC-331 Confirmed facts: the convert tag and the victory celebration (Resolved) |
| 380 | 10-U9 | covered by DEC-097 |
| 381 | 10-U10 | covered by DEC-097 |
| 382 | 10-U12 / 00-U10 | covered by DEC-049 |
| 383 | 10-U14 | DEC-318 Sound triggers tied to the frame rate (Open) |
| 384 | 10-U15 | DEC-319 Ambience during loading views (Open) |
| 385 | 10-U16 | DEC-320 Match-end pitch drop (Open) |
| 387 | 10-U5 | merged into 27-U8: covered by DEC-109 |
| 387 | 10-U6 | merged into 27-U5: DEC-321 Attached sounds when their emitter is removed (Open) |
| 387 | 10-U7 | merged into 27-U1: Dropped: DEC-339 Audio driver, software mixer and effect-processing details |
| 387 | 10-U11 | merged into 27-U14: DEC-323 Loop keep-alive polling (Open) |
| 387 | 10-U13 | merged into 27-U10: DEC-322 Start value of smoothed sound parameters (Open) |
| 396 | 20-U4 | covered by DEC-066, DEC-011 |
| 397 | 20-U6 / 04-U8 | covered by DEC-011 |
| 402 | 20-U13 | covered by DEC-011, DEC-223 (item 2) |
| 411 | 21-U1 / 22-U9 | covered by DEC-012, DEC-013 |
| 412 | 21-U2 / 22-U14 | covered by DEC-094 |
| 413 | 21-U3 | covered by DEC-095 |
| 414 | 21-U4 / 02-U8 / 06-U4 | covered by DEC-095, DEC-200 (item 5) |
| 415 | 21-U6 / 02-U14 / 05-U8 | covered by DEC-028 |
| 416 | 21-U7 | covered by DEC-028 |
| 417 | 21-U8 | covered by DEC-014 |
| 418 | 21-U9 | covered by DEC-094 |
| 419 | 21-U10 | covered by DEC-028 |
| 420 | 21-U11 | DEC-313 Dead airborne actors (Open) |
| 422 | 21-U5 | merged into 22-U8: owner row outside this package (see below) |
| 441 | 22-U17 / 05-U4 / 20-U5 | covered by DEC-011, DEC-223 (item 2) |
| 454 | 23-U3 | covered by DEC-013, DEC-242 (item 2) |
| 455 | 23-U4 | covered by DEC-246 (item 7), DEC-113 |
| 457 | 23-I3 | DEC-324 Terrain generator parameter semantics and biases (Open) |
| 458 | 23-I4 | DEC-324 Terrain generator parameter semantics and biases (Open) |
| 459 | 23-I5 | covered by DEC-246 (item 7) |
| 461 | 23-U2 | merged into 22-U12: owner row outside this package (see below) |
| 461 | 23-U5 | merged into 22-U13: owner row outside this package (see below) |
| 461 | 23-I2 | merged into 29-U14: covered by DEC-204 |
| 467 | 24-U1 / 31-U1 / 31-U2 | Dropped: DEC-336 Second graphics back end |
| 468 | 24-U2 | Dropped: DEC-336 Second graphics back end |
| 469 | 24-U3 | Dropped: DEC-336 Second graphics back end |
| 470 | 24-U4 | Dropped: DEC-336 Second graphics back end |
| 471 | 24-U5 | Dropped: DEC-336 Second graphics back end |
| 472 | 24-U8 | covered by DEC-098 |
| 473 | 24-U9 | Dropped: DEC-337 Graphics-driver, cursor and glyph-atlas behaviour |
| 474 | 24-U10 | covered by DEC-029, ADR-0020 |
| 475 | 24-U11 | Dropped: DEC-337 Graphics-driver, cursor and glyph-atlas behaviour |
| 476 | 24-U12 | Dropped: DEC-336 Second graphics back end |
| 477 | 24-U13 / 31-U3 | Dropped: DEC-336 Second graphics back end |
| 478 | 24-U14 | DEC-325 Light direction and the camera transform (Open) |
| 479 | 24-U15 | DEC-326 Terrain grid overlay (Open) |
| 481 | 24-U6 | merged into 32-U2: covered by DEC-106, DEC-228 |
| 481 | 24-U7 | merged into 20-U1: owner row outside this package (see below) |
| 487 | 25-U1 / 00-U8 / 29-U9 / 31-U7 | Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 488 | 25-U2 | Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 489 | 25-U3 | DEC-329 Assets of objects spawned only at run time (Open) |
| 490 | 25-U4 | Dropped: DEC-337 Graphics-driver, cursor and glyph-atlas behaviour |
| 491 | 25-U5 | covered by DEC-012 |
| 492 | 25-U6 | covered by DEC-098 |
| 494 | 25-U7 | merged into 29-U5: Dropped: DEC-340 Data formats, archives and production tools |
| 494 | 25-U8 | merged into 10-U2: Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 494 | 25-U9 | merged into 10-U1: DEC-317 Voice set language (Open) |
| 494 | 25-U10 | merged into 29-U4: DEC-330 Mesh sockets referenced only by data (Open) |
| 500 | 26-U1 | covered by DEC-037 |
| 501 | 26-U2 | covered by DEC-101 |
| 502 | 26-U5 / 02-U18 / 26-I4 | covered by DEC-007, DEC-222 |
| 503 | 26-U6 | Dropped: DEC-336 Second graphics back end |
| 504 | 26-U7 | covered by DEC-222 |
| 505 | 26-U8 | DEC-328 Saturated team colours on effect bubbles (Open) |
| 506 | 26-U9 | Dropped: DEC-339 Audio driver, software mixer and effect-processing details |
| 507 | 26-I1 | covered by DEC-012 |
| 508 | 26-I2 | covered by DEC-013 |
| 509 | 26-I3 | covered by DEC-013 |
| 511 | 26-U3 | merged into 32-U2: covered by DEC-106, DEC-228 |
| 511 | 26-U4 | merged into 29-U8: Dropped: DEC-340 Data formats, archives and production tools |
| 511 | 26-I4 | merged into 26-U5: covered by DEC-007, DEC-222 |
| 517 | 27-U1 / 10-U7 | Dropped: DEC-339 Audio driver, software mixer and effect-processing details |
| 518 | 27-U2 | covered by DEC-049 |
| 519 | 27-U3 | Dropped: DEC-339 Audio driver, software mixer and effect-processing details |
| 520 | 27-U4 | Dropped: DEC-339 Audio driver, software mixer and effect-processing details |
| 521 | 27-U5 / 10-U6 | DEC-321 Attached sounds when their emitter is removed (Open) |
| 522 | 27-U6 | Dropped: DEC-339 Audio driver, software mixer and effect-processing details |
| 523 | 27-U7 | Dropped: DEC-339 Audio driver, software mixer and effect-processing details |
| 524 | 27-U8 / 10-U5 | covered by DEC-109 |
| 525 | 27-U9 | Dropped: DEC-339 Audio driver, software mixer and effect-processing details |
| 526 | 27-U10 / 10-U13 | DEC-322 Start value of smoothed sound parameters (Open) |
| 527 | 27-U11 | Dropped: DEC-339 Audio driver, software mixer and effect-processing details |
| 528 | 27-U12 | Dropped: DEC-339 Audio driver, software mixer and effect-processing details |
| 529 | 27-U13 / 20-U7 | covered by DEC-224 (item 7) |
| 530 | 27-U14 / 10-U11 | DEC-323 Loop keep-alive polling (Open) |
| 536 | 28-U1 | DEC-316 Front-end text blend and page transition (Open) |
| 537 | 28-U2 | DEC-316 Front-end text blend and page transition (Open) |
| 538 | 28-U3 / 09-U14 / 29-U3 | covered by DEC-099 |
| 539 | 28-U4 | covered by ADR-0063, ADR-0032 |
| 540 | 28-U5 | covered by DEC-243 (item 1), DEC-202 |
| 541 | 28-U6 | covered by DEC-102 |
| 542 | 28-U7 | covered by DEC-102 |
| 543 | 28-U8 | covered by DEC-102 |
| 544 | 28-U9 | Dropped: DEC-333 Trial-edition gating and credential files |
| 550 | 29-U4 / 25-U10 | DEC-330 Mesh sockets referenced only by data (Open) |
| 551 | 29-U5 / 25-U7 | Dropped: DEC-340 Data formats, archives and production tools |
| 552 | 29-U6 | covered by DEC-015 |
| 553 | 29-U7 | Dropped: DEC-340 Data formats, archives and production tools |
| 554 | 29-U8 / 26-U4 | Dropped: DEC-340 Data formats, archives and production tools |
| 555 | 29-U10 | Dropped: DEC-341 Runtime-library, compiler and debug-build details |
| 556 | 29-U11 | covered by DEC-015 |
| 557 | 29-U13 | Dropped: DEC-340 Data formats, archives and production tools |
| 558 | 29-U14 / 23-I2 / 31-I2 | covered by DEC-204 |
| 560 | 29-U1 | merged into 08-U1: Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 560 | 29-U2 | merged into 09-U4: covered by DEC-100 |
| 560 | 29-U3 | merged into 28-U3: covered by DEC-099 |
| 560 | 29-U9 | merged into 25-U1: Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 560 | 29-U12 | merged into 22-U3: owner row outside this package (see below) |
| 560 | 29-U15 | merged into 22-U20: owner row outside this package (see below) |
| 566 | 30-U1 / 09-U15 | covered by DEC-110 |
| 567 | 30-U2 | covered by DEC-110 |
| 568 | 30-U5 | covered by DEC-103 |
| 569 | 30-U6 | covered by DEC-053 |
| 571 | 30-U3 | merged into 22-U20: owner row outside this package (see below) |
| 571 | 30-U4 | merged into 03-U7: covered by DEC-015 |
| 571 | 30-U7 | merged into 22-U21: owner row outside this package (see below) |
| 571 | 30-U8 | merged into 09-U9: covered by DEC-053 |
| 577 | 31-U6 | Dropped: DEC-340 Data formats, archives and production tools |
| 578 | 31-U9 | Dropped: DEC-340 Data formats, archives and production tools |
| 579 | 31-U10 | Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 580 | 31-U11 | covered by DEC-041 |
| 581 | 31-U12 | Dropped: DEC-340 Data formats, archives and production tools |
| 582 | 31-U13 | covered by ADR-0074, DEC-224 (item 5) |
| 583 | 31-U14 / 07-U3 | covered by DEC-005 |
| 584 | 31-I3 | covered by ADR-0065 |
| 586 | 31-U1 | merged into 24-U1: Dropped: DEC-336 Second graphics back end |
| 586 | 31-U2 | merged into 24-U1: Dropped: DEC-336 Second graphics back end |
| 586 | 31-U3 | merged into 24-U13: Dropped: DEC-336 Second graphics back end |
| 586 | 31-U4 | merged into 32-U1: Dropped: DEC-341 Runtime-library, compiler and debug-build details |
| 586 | 31-U5 | merged into 09-U2: Dropped: DEC-333 Trial-edition gating and credential files |
| 586 | 31-U7 | merged into 25-U1: Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 586 | 31-U8 | merged into 09-U4: covered by DEC-100 |
| 586 | 31-U15 | merged into 22-U13: owner row outside this package (see below) |
| 586 | 31-I1 | merged into 22-U15: owner row outside this package (see below) |
| 586 | 31-I2 | merged into 29-U14: covered by DEC-204 |
| 586 | 31-I4 | merged into 09-U2: Dropped: DEC-333 Trial-edition gating and credential files |
| 586 | 31-I5 | merged into 09-U4: covered by DEC-100 |
| 592 | 32-U1 / 31-U4 | Dropped: DEC-341 Runtime-library, compiler and debug-build details |
| 593 | 32-U2 / 24-U6 / 26-U3 | covered by DEC-106, DEC-228 |
| 594 | 32-U3 / 08-U2 | Dropped: DEC-334 Development, test and benchmark maps |
| 595 | 32-U4 | Dropped: DEC-341 Runtime-library, compiler and debug-build details |
| 596 | 32-U5 | Dropped: DEC-341 Runtime-library, compiler and debug-build details |
| 597 | 32-U6 | Dropped: DEC-341 Runtime-library, compiler and debug-build details |
| 598 | 32-U7 | Dropped: DEC-341 Runtime-library, compiler and debug-build details |
| 599 | 32-U8 | Dropped: DEC-332 Other editions, packaged archives and evidence questions |
| 600 | 32-I1 | covered by DEC-202 (item 8) |
| 601 | 32-I2 | covered by DEC-225 (item 3) |
| 602 | 32-I3 | covered by DEC-041, DEC-115 |

### Rows of 41 §3 outside this package

These lines are not in my package. Their owners, from `routing.csv` and the wave-2a DEC maps:

| 41 lines | Question ids | Owner and DEC |
|---|---|---|
| 167-170 | §3 heading and column notes | no DEC (routed DP-METHOD) |
| 171-208 | §3.0 questions resolved while the register was built | no DEC (routed DP-METHOD; each resolution lives in its owning document) |
| 389-395 | 20-U1, 20-U2, 20-U3 (also 24-U7) | X-DEC-2a: DEC-203 |
| 398-401 | 20-U8, 20-U10, 20-U11; 20-U12 | X-DEC-2b: DEC-224 (items 6-8, 12); 20-U12 dropped there as dead content |
| 403-406 | 20-U14 | X-DEC-2b: DEC-224 (item 10), DEC-223, DEC-226 |
| 424-429 | 22-U1, 22-U2 (also 00-U7, 09-U11, 09-U12) | X-DEC-2c: DEC-248, DEC-249 |
| 430-433 | 22-U3, 22-U4, 22-U5, 22-U6 (also 29-U12) | X-DEC-2a: DEC-203, DEC-205, DEC-207 |
| 434 | 22-U7 | X-DEC-2c: DEC-240 |
| 435 | 22-U8 (also 06-U7, 21-U5) | X-DEC-2a: DEC-200 (item 4) |
| 436-440 | 22-U10, 22-U11, 22-U12, 22-U13, 22-U15 (also 23-U2, 23-U5, 31-U15, 31-I1) | X-DEC-2c: DEC-240, DEC-246 |
| 442-448 | 22-U18, 22-U19, 22-U20, 22-U21 (also 29-U15, 30-U3, 30-U7) | X-DEC-2a: DEC-205, DEC-206 |
| 449-453 | 23-U1 | X-DEC-2c: DEC-246 |
| 456 | 23-I1 | X-DEC-2c: DEC-240 |

## Additions for the coordinator (facts for existing entries; I did not edit those files)

1. **DEC-016 / DEC-094 (05-U1).** Port occupancy order: the "first operating Worker" is decided by grid result order, which is cell x, then cell z, then team 0 to 5, then cell slot. 41 recommends an explicit tie rule; DEC-016 could list the port-occupant tie with this order as its stable key.
2. **DEC-111 (05-U12, 04-U11).** Add to its list of derived values: the Collector throughput figure of about 2.5 Workers per second for a 500 wu round trip is an estimate (the movement law has turn rate 9.5 × dt, velocity filter 0.92 / 0.08 and arrival radius 5 wu); and the rocket's 8-or-9-move outcome also depends on its effect slot.
3. **DEC-073 (07-U9).** The Comeback Bonus is a check box on the host's advanced-options page for Spawn Control, Zone Control, Relic Run and Flag Chain; an absent value counts as on.
4. **DEC-087 / DEC-112 (07-U2, 07-U10).** I mapped the stale power id and the Siege defence fall-back to DEC-087 because DEC-112 lists both among the behaviours DEC-087 corrects. Suggested addition: a map-validation rule that every Siege defence marker with a scan range is bound to a structure, because an unbound marker defaults to team 0 and its objective falls back at once.
5. **DEC-012 (03-U12).** The reference tree crown hit centre ignores the tree's rotation; the baked collision data should state the centre explicitly.
6. **DEC-052 (03-U6).** In a match Workers have no threat reaction to turret aiming markers; that threat type is evaluated only outside matches.
7. **DEC-097 (10-U4, 10-U9, 10-U10).** The Core Device loop (10-U4) and the no-team sting (10-U10) are named in DEC-097. I assumed that the capture-progress tone whose event name never resolves (10-U9) is the "Supply crate decay tone" correction; G10 should confirm.
8. **DEC-098 (24-U8).** The start-up preload first loads the shared cloud and laser textures mip-mapped, which is why the first requester fixes their processing (fact for C-AST).
9. **DEC-002 versus DEC-306 (05-U2).** DEC-002 states Relay transit as 10h + 1 ticks; 41 gives 9h + 1 to 10h + 1 depending on structure order. DEC-306 records the choice; DEC-002 could read "at most 10h + 1 (DEC-306)".
10. **DEC-045 (08-U8, 08-U9, 08-U10).** Editor lessons: a load, save and load round trip changes terrain and map identity through writer quantisation; opening a plain-text map under the compressed-map extension always fails; a compressed map with trailing bytes after the stream is read with an unchecked decompression call. All three argue for the lossless round-trip and fuzz-tested loaders that DEC-045 and DEC-015 already require.
11. **DEC-246 item 7 (23-I5, 23-U4).** On the reference maps the world-edge wrap artefacts affect only heights below sea level, so play never sees phantom land at the edge. 23-U4 (the compiler's conversion of negative heights to an unsigned index) is the truncation that item 7 already names, and it underlies DEC-113.
12. **DEC-041 / DEC-115 (32-I3).** The "give all research" cheat has no effect in a match, because match start fixes the research levels; the rebuilt cheat set should set the fixed levels or omit the cheat.
13. **DEC-037 (26-U1).** What each explainer animation depicts is inferred from its track data only; X-AST should treat the depiction column as a brief, not as a fact.
14. **DEC-222 (26-U7).** The render extrapolation semantics are settled in the owning documents (24 §2.4, 20 §10.6); DEC-222's reference behaviour already states position plus velocity times the time since the tick.
15. **DEC-007 / DEC-222 (26-U5, 02-U18, 26-I4).** Spider leg and foot state, including its wall-clock swing timing, is presentation only; no simulation rule reads it. C-VFX and G02b should state it as a boundary rule.

## Interpretations to confirm (could not be decided from my inputs)

- **DEC-328.** The question names "booster" bubbles. I mapped the booster to the **Shield** power, because the four Worker-targeted powers are booster, convert, speed and fury and the ledger maps the personal shield to Shield. C-VFX should confirm.
- **DEC-315.** "Twin terrains" is read as two maps built on one terrain. G08a should confirm from 08.
- **DEC-316.** 41 gives the transition as "fraction = 3.6 f, slide fraction⁶ × W, alpha 1 − 0.2 × fraction"; I wrote f as the transition's progress value. C-UI-a should state the variable exactly from 28 §6.7.
- **DEC-303.** 41 says only "launch compensation (/100)". I wrote it as a compensation term of distance ÷ 100; G04 states the exact rule.
- **DEC-309.** That the global flag of a Source Node only changes drawing is an inference (41 [I]); G03a should state what the drawing difference is, if its inputs say.
- **DEC-311.** DEC-087 moves the bot controller inside the map; that does not settle whether target searches exclude it, so DEC-311 stays separate. If G07's inputs show the correction also excludes it from target searches, DEC-311 can be closed as covered.
- **DEC-321, DEC-322, DEC-323.** Possible overlap with the 24 audio corrections of DEC-097 (F48). If reference/10 lists any of them as a correction, the coordinator can mark that entry Resolved by DEC-097.
- **28-U6, 28-U7, 28-U8.** Mapped to DEC-102 on the assumption that they are among the 33 framework defects avoided by construction; the package note had them as Dropped.
- **29-U6 and 29-U11.** The package note had 29-U6 as Dropped; DEC-015 already decides both (no load path passes a missing token on, an unterminated bracket never hangs the string-table parser), so I mapped them as covered.
- **23-U4.** The package note had it as Dropped; DEC-246 item 7 already records the truncating conversion, so I mapped it as covered.

## Cross-document requests (gate §7.3 item 6: every Open DEC is cited by at least one requirement)

Each rewrite package should cite these entries in its "Open decisions" section (the Resolved DEC-331 is a fact list for the same documents):

| DEC | Status | Packages that cite it (target files) |
|---|---|---|
| DEC-300 | Open | G01 (`reference/01_skirmish_rules_and_modes/README.md`) |
| DEC-301 | Open | G01 (`reference/01_skirmish_rules_and_modes/siege.md`); G03a (`reference/03_buildings_and_world_objects/economy_and_spawn_objects.md`) |
| DEC-302 | Open | G02a2 (`reference/02_units/transport.md`); G06 (`reference/06_controls_camera_and_commands.md`) |
| DEC-303 | Open | G04 (`reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md`); G04 (`reference/04_supply_crates_powers_and_weapons/crates_and_powers.md`) |
| DEC-304 | Open | G04 (`reference/04_supply_crates_powers_and_weapons/crates_and_powers.md`); G05 (`reference/05_population_flow.md`) |
| DEC-305 | Open | G04 (`reference/04_supply_crates_powers_and_weapons/crates_and_powers.md`); G02a2 (`reference/02_units/essence_orbs.md`); G05 (`reference/05_population_flow.md`) |
| DEC-306 | Open | G05 (`reference/05_population_flow.md`); G03a (`reference/03_buildings_and_world_objects/economy_and_spawn_objects.md`); C-SIM-a (`reference/12_simulation_model/04_containers_and_update_order.md`) |
| DEC-307 | Open | G02a2 (`reference/02_units/collector.md`); G02a2 (`reference/02_units/essence_orbs.md`); G05 (`reference/05_population_flow.md`) |
| DEC-308 | Open | G06 (`reference/06_controls_camera_and_commands.md`); G03a (`reference/03_buildings_and_world_objects/README.md`); G05 (`reference/05_population_flow.md`) |
| DEC-309 | Open | G03a (`reference/03_buildings_and_world_objects/economy_and_spawn_objects.md`); G08a (`reference/08_maps_and_level_design/rules_and_conventions.md`); C-DAT (`architecture/components/data_schemas.md`) |
| DEC-310 | Open | G06 (`reference/06_controls_camera_and_commands.md`); C-CAM (`architecture/components/camera_and_sequences.md`) |
| DEC-311 | Open | G07 (`reference/07_bots.md`); G02a1 (`reference/02_units/worker.md`); G02b (`reference/02_units/creatures_and_hazards.md`); G03b (`reference/03_buildings_and_world_objects/defences_and_props.md`) |
| DEC-312 | Open | G07 (`reference/07_bots.md`); C-SIM-a (`reference/12_simulation_model/04_containers_and_update_order.md`) |
| DEC-313 | Open | C-SIM-b (`reference/12_simulation_model/05_space_routing_and_physics.md`); G02a1 (`reference/02_units/README.md`) |
| DEC-314 | Open | G09a (`reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md`); G08a (`reference/08_maps_and_level_design/README.md`) |
| DEC-315 | Open | G08a (`reference/08_maps_and_level_design/rules_and_conventions.md`); G01 (`reference/01_skirmish_rules_and_modes/zone_control.md`); G01 (`reference/01_skirmish_rules_and_modes/relic_run.md`) |
| DEC-316 | Open | C-UI-a (`architecture/components/ui_framework/layout_rendering_input_and_localisation.md`); C-UI-b (`architecture/components/ui_framework/pages_and_screen_recipes.md`); C-RND-b (`architecture/components/renderer/visual_language.md`) |
| DEC-317 | Open | G10 (`reference/10_audio_design.md`); G09b (`reference/09_front_end_and_match_flow/options_localisation_and_hooks.md`); X-AST (`assets/art_and_audio_briefs.md`) |
| DEC-318 | Open | G10 (`reference/10_audio_design.md`); C-AEN (`architecture/components/audio_engine.md`) |
| DEC-319 | Open | G10 (`reference/10_audio_design.md`); G09a (`reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md`) |
| DEC-320 | Open | G10 (`reference/10_audio_design.md`); C-AEN (`architecture/components/audio_engine.md`) |
| DEC-321 | Open | G10 (`reference/10_audio_design.md`); C-AEN (`architecture/components/audio_engine.md`) |
| DEC-322 | Open | G10 (`reference/10_audio_design.md`); C-AEN (`architecture/components/audio_engine.md`) |
| DEC-323 | Open | G10 (`reference/10_audio_design.md`); C-AEN (`architecture/components/audio_engine.md`); G01 (`reference/01_skirmish_rules_and_modes/launch_race.md`) |
| DEC-324 | Open | C-TER (`architecture/components/terrain.md`); G08a (`reference/08_maps_and_level_design/rules_and_conventions.md`); G08b (`reference/08_maps_and_level_design/map_editor_hooks.md`) |
| DEC-325 | Open | C-RND-a (`architecture/components/renderer/view_lighting_and_fog.md`) |
| DEC-326 | Open | C-RND-a (`architecture/components/renderer/world_renderers.md`); C-TER (`architecture/components/terrain.md`) |
| DEC-327 | Open | C-RND-b (`architecture/components/renderer/options_and_budgets.md`); C-RND-a (`architecture/components/renderer/world_renderers.md`); G03a (`reference/03_buildings_and_world_objects/README.md`) |
| DEC-328 | Open | C-VFX (`architecture/components/animation_and_vfx.md`); C-RND-b (`architecture/components/renderer/visual_language.md`) |
| DEC-329 | Open | X-AST (`assets/asset_manifest.csv`); X-AST (`assets/README.md`); C-AST (`architecture/components/asset_pipeline_and_resources.md`) |
| DEC-330 | Open | C-AST (`architecture/components/asset_pipeline_and_resources.md`); X-AST (`assets/asset_manifest.csv`) |
| DEC-331 | Resolved | G04 (`reference/04_supply_crates_powers_and_weapons/crates_and_powers.md`); G03b (`reference/03_buildings_and_world_objects/defences_and_props.md`); G02a1 (`reference/02_units/worker.md`); G08a (`reference/08_maps_and_level_design/rules_and_conventions.md`); G10 (`reference/10_audio_design.md`) |

Specific requests:

- **G05 and C-SIM-a**: DEC-306 needs the spawn-network transit time stated with its range and its dependence on structure order.
- **G04**: DEC-304 and DEC-305 need the Spawn Surge count rule and the Essence Grove team draw stated as rules with tuning keys.
- **G10 and C-AEN**: DEC-318 and DEC-323 need the triggering point of the flash and clock-tick sounds and the loop keep-alive mechanism stated as reference behaviour.
- **X-AST**: DEC-317 (voice set language), DEC-329 (run-time-only assets: keep all 51 until the spawn analysis), DEC-330 (socket inventory) and DEC-342 (thumbnails generated by the engine).
- **Coordinator (CONVENTIONS.md §3)**: like X-DEC-1, I point to `decisions/README.md` for the index; `CONVENTIONS.md` still names `decisions/decision_register.md`.

## DEC and ADR requests

- None outside my range. DEC-343 to DEC-449 are free.
- ADR: none. 28-U4 (IME input) is covered by ADR-0063 and ADR-0032, 31-U13 (one monotonic clock) by ADR-0074, 31-I3 (crashes on every thread) by ADR-0065.

## Names needed but not in the ledger

No new proper names. Descriptive phrases used, proposed for the glossary: **convert tag** (the mark that the Convert power puts on Workers), **convert laser**, **victory celebration** (the idle Workers' celebration when every objective is complete), **ink text** (a front-end text style), **Shield bubble** and **Visitor-faction bubble**, **trial edition**, **harvested and collected states** (Essence orb states). "Transit orb" is the ledger's engine-type term.

## Dropped, by group (one line each)

- DEC-332, other editions, packaged archives and evidence questions: Species sets no parity target (DEC-005) and authors all content new.
- DEC-333, trial-edition gating and credential files: replaced by platform entitlements (DEC-248).
- DEC-334, development, test and benchmark maps: no match can select them.
- DEC-335, online-service listing fields: the reference's own services are not part of the library.
- DEC-336, second graphics back end: no match runs on it (DEC-059).
- DEC-337, graphics-driver, cursor and glyph-atlas behaviour: properties of superseded drivers and font atlases.
- DEC-338, input-stack implementation details: Species defines its own input processing (ADR-0029, DEC-096).
- DEC-339, audio driver, software mixer and effect-processing details: the normative laws are in reference/10; the engine is new.
- DEC-340, data formats, archives and production tools: no reference data or tool output is imported.
- DEC-341, runtime-library, compiler and debug-build details: toolchain and debug-build properties only.
- DEC-342, provenance of reference art and audio: every asset is authored new.

## Gate results

- `scan_patterns.py forbidden_patterns.tsv design_next/decisions/open_questions.md`: **0 hits**.
- `scan_patterns.py soft_patterns.tsv design_next/decisions/open_questions.md`: **0 hits** (one earlier hit, "Recovery" in a title, was reworded; nothing to justify).
- `X-DEC-3_build.py` validation: pass (43 entries, ids 300 to 342 sequential, field order correct, every Affects path a package target, every DEC and ADR reference resolves).
- Own check: the design file contains no question id, evidence tag or source path.
