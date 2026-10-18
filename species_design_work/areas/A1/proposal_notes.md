# A1 proposal notes: world and regions

> Proposer: A1-P · Date: 2026-10-18 · Revision: after challenge round 1 (`challenge_1.md`, answered in `resolution_1.md`) · Documents: `Design/gdd/01_world_and_regions/` (status Draft, prefix WLD: `README.md`, `world_and_sites.md`, `terrain_time_and_views.md`, `skirmish_maps_and_map_tools.md`), `Design/gdd/adoption_ledger/08_maps_and_level_design_ledger.md`, `Tools/DesignModels/A1WorldDensity.py` (with its README entry), `species_design_work/scripts/A1P_RegionPrototype.py` · Prompt §3 steps 1 and 3

These notes are for the coordinator and, at Finalise, for the rationale file `Design/rationale/A1.md`. The challenger does not read them. Upstream: A0 Final (`gdd/00`, `rationale/A0.md`), owner decisions O1 to O37. Round 1's single document (108.6 KB) passed 120 KB in revision and is split into a folder; every requirement id of round 1 is kept.

## 1 Options considered

Every significant choice names the pillar it serves (PIL-016, O36).

| Choice | Options | Chosen | Why (pillar) |
|---|---|---|---|
| Terrain approach (the brief's main question) | (a) procedural from the reference algorithm alone; (b) authored worlds per capacity band, like skirmish maps; (c) hybrid: an authored world template and set pieces, a layout laid out from a seed, the reference generator for the terrain in two passes with a feathered flatten type, validated with regeneration, baked once at creation | (c) hybrid (WLD-160 to WLD-167) | (a) cannot guarantee level sites, open channels, dry crossings, equal home resources or reachability, so pillar 6's equal sites and pillar 3's legible geography would be luck, and it spreads method 2's bias over a continent. (b) is too costly for a small team at 3 to 98 regions, gives every community server the same solved map, and cannot follow the host's capacity. (c) keeps the reference look (pillar 5), makes every world different from a seed, enforces fairness by construction and validation (pillar 6), needs one template for stage 1, and bakes so that no client depends on the generator's floating-point arithmetic. The two-region prototype passes every structural check in 100 seeds of 100 |
| Runtime model of world terrain (D8's DEC-246) | generate at load on every participant; bake at creation into chunks delivered by the server; procedural at run time in integer arithmetic | bake at creation, region chunks delivered by digest (WLD-166) | The terrain is created once and never changes (WLD-160); the server owns it; chunk digests are its identity (DEC-246 B with D) |
| World shape and extent | one continent; an archipelago; a ring; a full square of land; an extent sized to the capacity or reserved for the largest | one compact continent of island valleys at the centre of an extent reserved for 256 stewards, 71,680 wu (WLD-001 to WLD-003) | A sea border is what the terrain mesh needs (R-MAP-198) and what the black-fog look expects (pillar 5); a reserved extent lets a capacity raise grow without moving a stored position (A1-F10); sea chunks cost one value each |
| What a region is | a plain area of the partition; a valley enclosed by ridges and crossed by passes; an island valley with sea channels and land-bridge crossings | an island valley of four sites, sea channels on every side, one crossing per shared side (WLD-020, WLD-021, WLD-024) | The challenger showed that the adopted movement rules let crowds walk over a ridge (A1-F03); water stops every walking actor under those rules with no new movement rule, so the valleys are the units of travel and of the warning; every settlement lives in one region (A2) |
| Sites per region and their pattern | 3; 4 (2 × 2); 6; Poisson-disc; per-site jitter; the whole valley offset and rotated | 4 in a 2 × 2 pattern, the pattern offset by up to 64 wu and rotated by up to 3° as a whole (WLD-040) | Every site has two valley neighbours, the same lens, home cell and contact days exactly (A1-F07; pillar 6); 2,400 bodies per region at a placeholder 600 per settlement |
| Site spacing and largest territory | spacing 1,920 to 2,400 wu × radius 1,300 to 1,500 wu (model sweep) | 2,080 wu and 1,400 wu (WLD-040, WLD-060) | First valley contact on day 5 with square-root growth (battles from week 2, PIL-065), a 720 wu lens, the valley centre 71 wu outside every territory |
| Holding land between neighbours | first to reach the halfway line holds the whole lens; split at the halfway line, each half its own side's; unheld until a claim | split: each side holds its own half once its territory enters it, and the other side enters only by a claim (WLD-062, WLD-063) | Round 1's first-to-reach made arrival order permanent (A1-F01); the split gives a late joiner the same land as the first steward (pillar 6) and makes every change of holder a claim with a cause on record (pillar 3) |
| What a claim may take | any border area or resource site; only across the claimant's own borders and in its own valley | only across own borders and in its own valley; at most 17 resource units, 1.55 times every site's 11 (WLD-086) | A bound that the leader guard of PIL-028 can rely on (A1-F09; pillar 6) |
| Resources per site | a pattern turned away from near neighbours; a fixed pattern from the outward diagonal | a fixed pattern of 8 home resource sites, plus a ninth where a site has no crossing; border and crossing factor 1, central 3 (WLD-080 to WLD-085) | The same gathering distances and the same 11 units at every site (A1-F08, A1-F19; pillar 6) |
| AI settlements | a fixed count; 6 + 0.25 × capacity filled to whole regions; a share of the settlements kept by a keeper | native AI 6 + 0.25 × capacity, the rounding to whole regions going to natives, plus stand-in AI settlements on a margin of 0.25 × capacity, kept by a keeper (WLD-004, WLD-120 to WLD-127) | The natives populate the world from day 1 (PIL-008); the stand-ins keep every steward among neighbours as a world fills and empties (A1-F06; pillar 2) and step aside for stewards, while A2's full-occupancy load never grows |
| AI placement | one per region, then diagonal; diagonal pairs in the centre (A1-M2); random | natives in rounds over the regions (local sites 0, 3, 1, 2), stand-ins first in empty valleys (WLD-121, WLD-125) | Every valley has a settlement on world day 1 at every capacity; M2's steward contacts are reached without leaving valleys empty |
| Founding order | random; ranked by occupied neighbours; ranked with stewards weighted above AI; forced placement | ranked: 6 per steward neighbour (valley or crossing), 2 per AI valley neighbour, 1 per AI crossing neighbour, 1 per valley neighbour site still free or a stand-in's; "found beside" a named steward; groups of up to 4 from stage 2 (WLD-140, WLD-141) | Friends border each other (A1-F13): 75 to 100 % of stewards of a fresh world of 16 at 25 % occupancy have a steward neighbour, against 0 to 50 % without the room weight; the recommendation seats a late joiner among median neighbours without revealing strength (PIL-029, PIL-084) |
| Places of dormant stewards | a place held for 42 world days; no held place, a return queue; a return margin of extra sites | no held place; the return queue is served before newcomers (WLD-005, WLD-143) | A held place locked late joiners out of worlds with empty sites (A1-F02): a world of 200 with strong demand now refuses 22 of 532 arrivals by day 180, all while every place is taken by a settlement on the map |
| The warning of PIL-025 | a sparse world; a forecast at the estimated arrival minus 10 minutes; a warning radius; a warning at the declaration with a gate | warning at the declaration and a gate: no entry to the target's territory until 10 minutes after the warning, at both paces (WLD-102) | A forecast is beaten by a faster route or a speed effect (A1-F04); the gate is immune to both and needs no radius (A1-M7) |
| March speed | B4's formation speed of 12 wu/s; the Worker's 20 wu/s as a world rule; regions sized from B4's speed | 20 wu/s as a world rule for expeditions outside battle, B4 confirms, regions rescaled if B4 sets another (WLD-101, WLD-104) | At 12 wu/s every travel limit fails (A1-F14): the diameter of a world of 200 would be 118 to 127 minutes |
| Battle load handed to A2 | the base case; a range of three cases with the weekly maximum; caps | the evening-peak range in three cases, the planning case for sizing, and caps per region and per world with attacks waiting at the gate (WLD-284, WLD-285) | The base case was the low end of plausible inputs (A1-F05); caps give A2 a lever with a defined failure behaviour (pillar 1's battle budget) |
| World constants | live tuning; a snapshot at creation | a snapshot at creation, read for the world's life (WLD-009) | A patch retunes new worlds only (A1-F15) |
| World lifetime default | unlimited everywhere; 12 weeks everywhere; unlimited integrated, 12 weeks dedicated and hosted; 4 weeks | unlimited integrated, 84 world days dedicated and hosted, stage 1 builds only unlimited (WLD-220) | A friends' world never ends unasked; a world of strangers gains an arc and a fresh start for late joiners; owner question 2 |
| Eras for unlimited worlds (A1-M6) | none; a soft reset every 12 weeks | none in v1; B5 evaluates with renown in stage 2 | The 17-unit bound already caps land at 1.55 times a site's own; the reset is a rule of claims and renown; escalated as owner question 2 |
| End of a lifetime | hard stop; finish battles then archive; reset in place | finish battles, read-only archive, successor world by invitation, no carry-over (WLD-223) | No settlement is removed (PIL-069, PIL-080); a carry-over would bring the runaway leader along (pillar 6) |
| World calendar | a clock; world days of 24 h at both paces; world days scaled by the pace | a world day is 24 h ÷ the pace factor (WLD-200) | Day 14 means the plateau at both paces (PIL-103); no clock time, because there is no time of day |
| Time of day, weather, biomes, lighting | a day and night cycle; weather that acts; rule differences per biome; raw light values | none; look-only weather; biomes of look and shape only; named presets (WLD-180 to WLD-185) | Pillar 5's look, pillar 2's rule-out of set hours, PIL-049's announced losses, equal sites (pillar 6) |
| Strategic view | a separate 2-D map; the same world at a coarse level of detail | the same world and renderer features, coarse facets, marks instead of Worker cards, a public and a seen layer (WLD-240 to WLD-246) | Pillar 5 rules out a second art style; grievances and claims need their places public (pillar 3); strength stays fogged (PIL-084) |
| Settlement view bounds | anywhere; own territory; own areas with expedition views; own territory with a margin and own battles | own territory with 800 wu and own battlefields; expeditions in the strategic view only (WLD-244) | Expedition views made the watched bound fail (A1-F12); the union is 116 to 118 typical and 174 to 177 at most at 200 |
| Map editor (DEC-045) | a new editor with undo and play test; no editor; a player-facing editor in v1 | text authoring first, a development editor in Debug builds after the first playable, no player editor in v1 (WLD-260) | Modding is a vision non-goal (§7); the first playable needs one or two maps |

## 2 Recommendations (improvements beyond the vision)

| Recommendation | Benefit | Cost | Where |
|---|---|---|---|
| Island valleys, sea channels and crossings | Every conflict has a named place; crossings are chokepoints for the defender's advantage; the barrier comes from rules already adopted | Generator work for channels and bridges, and their validation | WLD-021, WLD-024, WLD-163 |
| Split border areas and claims bound to own borders | Arrival order never decides land; a hard bound of 1.55 times on what land and claims add | B1 grows to a line, B5 owns every change of holder | WLD-062, WLD-063, WLD-086 |
| Stand-in AI settlements and the keeper | Every steward among neighbours in month 6 as in week 2; a settlement in every valley on day 1 | B7 runs stand-ins that step back with their stores | WLD-124 to WLD-127 |
| The founding ranking with a room weight, "found beside" and groups | Friends border each other; late joiners sit among median neighbours | A server-side ranking and a C2 page | WLD-140 to WLD-142 |
| No held places, a return queue | No late joiner is locked out of a world with empty sites | A returning steward may wait while a world is full | WLD-005, WLD-143 |
| The warning gate | PIL-025 holds whatever the route or speed | A hold that B5 and B6 show to both sides | WLD-102 |
| Battle caps with waiting attacks | A2 has a lever and a defined failure behaviour | Attacks wait during 4 % of the evening peak at 16 in the planning case | WLD-285 |
| The feathered flatten type | Level plateaus without a step at the rim (3 to 5 wu against 31 to 52) | One enumerator and its tests in the terrain component | WLD-268 |
| Reserved extent and world constants | Capacity raises without moving positions; worlds that survive patches | Sea chunks of one value; constant storage (D4) | WLD-001, WLD-009 |
| Reproducible worlds and regeneration on validation failure | Community events, bug reports and tests can recreate a world; fairness checked, not hoped for | Counter-based draws (DEC-242); a failure path in creation | WLD-161, WLD-163 |

**Not recommended:** a day and night cycle; weather hazards in worlds; per-biome rule differences; a player-facing map editor in v1; carrying strength into a successor world; territories that cross region borders; eras in v1.

## 3 Assumptions

1. Expeditions outside battle travel at 20 wu/s as a world rule (B4 confirms; WLD-104 rescales otherwise).
2. B1's territory grows like the square root of time to the plateau of day 14; with linear growth, first contact slips to day 8.4.
3. A settlement at its plateau holds about 600 Workers and Leaders (placeholder for B1).
4. The battle cases (8 or 12 minutes, 50 or 80 % of attacks by present stewards in a 5- or 3-hour window, starts alone, in pairs or in fours) bracket B4's, B5's and B6's eventual figures.
5. A gathering expedition goes to the valley centre 6 times a world day per settlement and stays 5 minutes (B2).
6. The lifetime scenarios (launch, daily demand, exponential active lifetimes of 43 to 60 days, a quarter of the dormant returning after 7 to 60 days) are plausible for B6 and C2 to replace.
7. The cell of 16 wu is fine enough for crowds and buildings at world scale; D8 may quantise heights to 16 bits.
8. The C++ generator on the reference test machine (DEC-971) meets 20 and 120 seconds; the prototype took 0.5 seconds of Python per region at four times fewer samples.
9. A2 can simulate a battle that reaches up to 448 wu into a second region (a crossing battle), and B4 bounds such a battlefield by the crossing's two mouths.
10. Stewards accept a regular 2 × 2 site pattern when terrain and the valley's offset and rotation break it up on the ground.
11. B5 keeps attacks mostly between settlements in contact, so the gate's hold is the usual case.
12. E2 can give the six biomes distinct looks from ramp files 01 to 06, gradients 01 to 06 and sea tiles 01 to 04.

## 4 Owner questions (for gate B), merged with the challenge

1. **World size, native AI and stand-ins.** Recommended: native AI 6 + 0.25 × capacity, a site margin of 0.25 × capacity for stand-ins, a keeper that holds every steward at 2 contacts. Trade-off: B7 runs stand-ins that step back with their stores, and A2's full-occupancy load is unchanged; without the keeper a weakly settled world of 200 has 11 % of its stewards below 2 contacts on day 180 instead of 1 %. Proposer and challenger agree (A1-M3).
2. **Lifetime defaults and eras.** Recommended: unlimited for integrated worlds, 84 world days for dedicated and hosted ones, lengthened at will before the final notice, and no eras in v1. Trade-off: a limited default ends a world a community wants to keep unless its host lengthens it; eras would cure a runaway leader's land with a soft reset but erase standing. Disagreement: the challenger recommends eras (A1-M6); the proposer leaves them to B5 with renown in stage 2.
3. **Stage-2 capacity.** Recommended: up to 16 stewards, as integrated. Trade-off: one load profile (4 battles in progress in the planning case) against larger community worlds, which need A2's dedicated budget. No disagreement.
4. **Battle caps.** Recommended: 2 per region and max(4, 10 % of settlements) per world (4 at 16, 26 at 200), attacks beyond waiting at their gates. Trade-off: waits during 4 % of the evening peak at 16 (11 % at the session pace) in the planning case, against sizing for the stress case without caps (28 battles, 11,200 figures at 200; 12 on the host's PC at the session pace). No disagreement; A2 sets the values.

## 5 Adoption (reference/08, ledger owned by A1, B9 for the skirmish maps)

All 42 sections and 25 exception rows are filled; none is pending. Effective verdicts: **adopt 6, adopt-skirmish 70, adapt 109, drop 0**.

- **adopt (6):** R-MAP-198 (sea border, every world), R-MAP-356 (a map tool's own generator), R-MAP-388 to R-MAP-390 (the lossless writers), R-MAP-400 (validation on save).
- **adopt-skirmish (70):** the skirmish map model of rules and conventions §2.1 to §2.8, §2.10, §2.11, §2.13 to §2.16 and §2.23, except the exceptions; the world has no map definition, match setup or start positions.
- **adapt (109):** map editor hooks §2.1 to §2.14, §2.16, §2.17 (WLD-260, WLD-261, WLD-183, WLD-265, WLD-268, WLD-262); R-MAP-007 (WLD-263); R-MAP-013, R-MAP-073, R-MAP-080, R-MAP-092, R-MAP-131 and §2.12 (WLD-265); R-MAP-014, R-MAP-060 (WLD-264, WLD-266); R-MAP-062 (WLD-266); R-MAP-081, R-MAP-083 and R-MAP-335 (WLD-268: method 3, flatten types 5 and 6); R-MAP-085 (WLD-162); R-MAP-086 (WLD-180, the census dropped, A1-F22); §2.9 (WLD-183, WLD-267); the census rows and the five other modes (PIL-003).
- **For B9:** every skirmish row is A1's proposal; B9 confirms it or sends a row change.

## 6 Decisions (the eight of the triage), proposed resolutions

| DEC | Proposed | Decision | Implemented by |
|---|---|---|---|
| DEC-045 Map editor | Resolved | Text authoring validated by the content build and previewed in a Debug build first; a development editor in Debug builds after the first playable, with undo, unsaved-change warnings, stable-id selection, lossless writers, validation on save, a play test, link tools with removal, terrain on demand with the variants of WLD-268, and world-template and set-piece authoring; no editor in player builds in v1 | WLD-260, WLD-261, WLD-262 |
| DEC-315 Level-design patterns | Resolved | Option A, the specified default stands; B9 playtests each pattern a Species map uses | WLD-269 |
| DEC-324 Generator semantics and biases | Resolved | Option B: the arithmetic stays exactly; generation method 3 (uniform parent), flatten type 5 (smooth over five) and flatten type 6 (feathered level square) are added; the world generator uses them | WLD-162, WLD-268 |
| DEC-562 Lighting and weather presets | Resolved | Option B: named presets in the key-and-fill pattern and weather presets; play-changing effects stay explicit map flags; in a world weather is presentation only | WLD-183, WLD-184, WLD-267 |
| DEC-802 Maps that match setup does not list | Resolved | Option B: an explicit visibility field, listed or test; world templates listed by the world-creation page | WLD-264 |
| DEC-865 Map data without a reference case | Resolved | Option B with item 5 as stated | WLD-265 |
| DEC-866 Population-cap values | Resolved | Item 1 option A, item 2 option B; B9 sets each map's cap; the world has no map cap | WLD-266, WLD-281 |
| DEC-966 Terrain query edge cases | Resolved | Item 1 option A, item 2 option B | WLD-270 |

None changes a pillar, an owner decision or the stage plan, so none is escalated. All are dated 2026-10-18 at Finalise.

## 7 Cross-area requests

| To | Request |
|---|---|
| Coordinator | Update the GDD index row and CONVENTIONS §3.1 to the folder `gdd/01_world_and_regions/`. Register at Finalise: resource site (`ResourceSite`, `region.resource`), border area (`BorderArea`, `region.border_area`), crossing (`RegionCrossing`, `region.crossing`; replaces round 1's pass), home cell, native AI settlement, stand-in AI settlement, biome (`Biome`, `region.biome`), world template (`WorldTemplate`), set piece (`SetPiece`), world day; region names `region.name.<nnn>`. Refresh the ledger totals |
| A2 | Size the server from Tables 5 and 6 of terrain, time and views (the planning case: 4 battles at 16, 6 at the session pace, 16 at 200; the stress case at 10 a week as the top); set the region body budget (WLD-281) and the battle caps (WLD-285) at gate B; decide which region simulates a crossing battle (WLD-282); provide counter-based draws keyed by seed, purpose, region, attempt and index (DEC-242) |
| B1 | Grow every settlement's territory, founded at any time, to 1,040 wu by day 5, 1,200 wu by day 9 and 1,400 wu at the plateau (WLD-064); never into a border area it does not hold (WLD-063); keep the plateau population within WLD-281; place a returning settlement (WLD-143) |
| B2 | Kinds and yields for the 8 (or 9) home resource sites with equal totals (WLD-081, WLD-082); border and crossing factor 1, central 3; the 17-unit bound in the leader model of PIL-028 (WLD-086); gathering at the valley centre and per journey at both paces (WLD-103) |
| B4 | Confirm 20 wu/s as the world march speed or set another and run the model (WLD-101, WLD-104); state whether Transports cross channels (WLD-021); bound a battlefield at a crossing by its two mouths (WLD-282); say whether an expedition joining a battle takes a slot (WLD-285); decide DEC-101 with WLD-185 |
| B5 | Claims only across a settlement's own borders and in its own valley (WLD-086); contact events as the territory-pressure input (WLD-066); the gate's texts and the waiting attack (WLD-102, WLD-285); no attack declarations in the last world day (WLD-222); evaluate eras with renown in stage 2 (A1-M6) |
| B6 | Confirm the 10-minute warning at both paces (WLD-102); set the evening window and the churn figures that replace the lifetime scenarios (WLD-127, WLD-280); protection while an attack waits (WLD-285) |
| B7 | Run stand-in AI settlements: they step back dormant with their stores, keep their Cladan and are placed again first (WLD-124 to WLD-126); natives placed by WLD-121 and Cladans by WLD-122 |
| B9 | Confirm or change the skirmish rows of the ledger; each skirmish map's population cap (WLD-266); the visibility field (WLD-264); playtest the DEC-315 patterns |
| C1 | The two views and their heights (WLD-240); the settlement view's bounds (WLD-244) |
| C2 | The founding page with "found beside", groups and the recommended site (WLD-140 to WLD-142); the return queue on the join page (WLD-143); the world-creation page with the contact figure for a lowered AI count (WLD-065) and the generation progress (WLD-165); the archive (WLD-223) |
| C3 | The calendar dial (WLD-201); the strategic view's elements, the waiting attacks among them (Table 4 of terrain, time and views) |
| D1 | World creation and extension on the server with the failure path (WLD-008, WLD-163, WLD-165) |
| D4 | Store the world constants and specify their migration (WLD-009); register `maps.world_template` and a set-piece record (WLD-167); the visibility field and the cap's 0 form (WLD-264, WLD-266); baked chunks with digests and archives (WLD-166, WLD-223) |
| D8 | Decide DEC-246 and DEC-247 with WLD-166 as A1's recommendation; amend the terrain component for generation method 3 and flatten types 5 and 6 (TER-032, TER-034, TER-035, TER-100, TER-141, TER-143, TER-250) and the columns × rows scan (TER-076); the two-pass region generation (WLD-162); chunks of 1,024 wu; camera-relative precision of 0.01 wu at 71,680 wu (WLD-246); a far plane for the strategic camera at up to 46,080 wu |
| D9 | The seen layer of the strategic view on the server (WLD-242) |
| D10 | The reference test machine for the generation budgets (DEC-971) |
| E2 | The six biome looks (Table 3 of terrain, time and views); 2 lighting presets in stage 1 and 6 from stage 2; 8 set pieces; 120 region names; the strategic-view glyphs and marks |
| E3 | Schedule per Table 5 of world and sites: the generator and its validation, the keeper, the founding page and the development map editor |

## 8 Model results (for the rationale's "Model results")

`Tools/DesignModels/A1WorldDensity.py` (check: 140 comparisons covering every figure and table, 0 differences); `species_design_work/scripts/A1P_RegionPrototype.py` (100 seeds of 100 pass).

- **Size:** solo 3 regions, 12 sites, 9 native AI, margin 0; 16 stewards 8 regions, 32 sites, 10 native AI, margin 4; 200 stewards 77 regions, 308 sites, 56 native AI, margin 50; the extent is 71,680 wu at every capacity.
- **Travel at 20 wu/s, seeds 1 to 10:** to the halfway line 0.9 min; valley neighbour 2.0; crossing neighbour 2.8 to 3.2; region hop 5.8 to 5.9; diameter 23 to 25 min at 16 and 70 to 76 min at 200.
- **Contact:** first valley contact day 5.0, crossing day 8.5 at every site (linear growth: 8.4 and 10.9); lens 720 wu; every site 11 units, at most 17. Fresh world over seeds 1 to 10: a steward's contacts 2.4 to 3.0 at 16 and 2.8 to 2.9 at 200; stewards with a steward neighbour 75 to 100 % at 16 and 25 % occupancy, 100 % at 200.
- **A year of world days (five seeds):** day 180: 200 with strong demand 167 stewards on the map, 22 of 532 refused, 3.04 settlements per region, contacts 2.75, 1 % below 2; 200 with weak demand 50 stewards, 2.02 per region, 2.61 contacts, 1 % below 2 (11 % without the keeper); 16 with a friend every 20 days 6 stewards, 2.40 per region, 2.39 contacts, 0 % below 2 (14 % without the keeper).
- **Scale for A2:** battles in progress at the evening peak, 6 battles per settlement per week, p99 base / planning / stress: 2 / 4 / 8 at 16, 3 / 6 / 8 at the session pace, 5 / 16 / 20 at 200; at 10 a week the stress case is 8, 12 and 28; caps 4 and 26; expeditions in progress 1.0 and 9.9; present at the peak 4 and 47; watched union 10 to 11 typical and 15 to 16 at most at 16, 116 to 118 and 174 to 177 at 200; 0.82 M and 7.88 M land samples.
- **Terrain prototype:** two regions at 32 wu cells: channel, crossing, plateaus, dry sites and reachability in 100 of 100 seeds; floors 52 to 111 wu; rim steps 3 to 5 wu with the feathered type, 31 to 52 wu with Absolute; 0.5 s of Python per region; a region's layout needs at most 24 flatten areas.

## 9 Gate result

`python3 -I Tools/DesignCheck/CheckDesign.py Design --skip-mermaid --design-phase`: all gates pass; without `--skip-mermaid`, gate 4 passes (284 blocks, 0 failed). Gate 12 reports other areas' pending rows; gate 13 the design-phase decisions until Finalise; gate 15 that `rationale/A1.md` is written at Finalise.
