# A1 proposal notes: world and regions

> Proposer: A1-P · Date: 2026-10-18 · Documents: `Design/gdd/01_world_and_regions.md` (status Draft, prefix WLD), `Design/gdd/adoption_ledger/08_maps_and_level_design_ledger.md`, `Tools/DesignModels/A1WorldDensity.py` (with its README entry) · Prompt §3 step 1

These notes are for the coordinator and, at Finalise, for the rationale file `Design/rationale/A1.md`. The challenger does not read them. Upstream: A0 Final (`gdd/00`, `rationale/A0.md`), owner decisions O1 to O37.

## 1 Options considered

Every significant choice names the pillar it serves (PIL-016, O36).

| Choice | Options | Chosen | Why (pillar) |
|---|---|---|---|
| Terrain approach (the brief's main question) | (a) procedural from the reference algorithm alone: the world as a random set of reference tiles and flatten areas; (b) authored worlds: a fixed set of hand-made worlds per capacity band, like skirmish maps; (c) hybrid: an authored world template and set pieces, a procedural layout from a seed, the reference generator for the terrain, baked once at creation | (c) hybrid (WLD-160 to WLD-167) | (a) cannot guarantee level sites, impassable ridges, passes, equal home resources or reachability, so pillar 6's equal sites and pillar 3's legible geography would be luck; it also inherits method 2's directional streaks across a whole continent. (b) is too costly for a small team at 7 to 65 regions, gives every community server the same solved map, and cannot follow the host's capacity setting. (c) keeps the reference look (DEC-025, pillar 5), makes every world different from a seed, enforces fairness by construction and by validation with re-rolls (pillar 6), costs one template for the first milestone, and bakes so that no participant depends on floating-point reproducibility of the generator (DEC-246 lesson 5) |
| Runtime model of world terrain (D8's DEC-246, A1's recommendation) | generate at load on every participant; bake at creation into chunks delivered by the server; procedural at run time in integer arithmetic | bake at world creation, deliver region chunks by digest (WLD-166) | A world's terrain is created once, never changes (WLD-160), and the server owns it; baking moves 160 M generator calls at 200 out of every client and makes chunk digests the identity (DEC-246 B with D) |
| World shape | one continent (a blob of land regions ringed by sea); an archipelago; a ring; a full square of land | one compact continent (WLD-001 to WLD-003) | Workers die in deep water (R-SIM-377), so islands would need sea travel the world game does not have; a sea border is what the terrain mesh needs (R-MAP-198) and what the black-fog look expects (pillar 5); a blob is legible at strategic zoom |
| Region shape and partition | square regions on a grid aligned with chunks; Voronoi regions of chunks; hexagonal regions | squares of 5,120 wu, 5 × 5 chunks (WLD-020) | Squares align with terrain chunks (DEC-247 B) and the server partition (vision §5); ridges along the borders hide the grid on the ground; hexagons do not align with square chunks |
| What a region is | a plain area of the partition with sites anywhere; a valley enclosed by ridges and crossed by passes | a valley of four sites with border ridges and passes (WLD-021, WLD-024) | Territories then never cross a region border, so a settlement lives in one region and only pass battles touch two (WLD-282), which A2 needs; passes give chokepoints for the defender's advantage (O2) and named places for conflict (pillar 3) |
| Sites per region | 3 (a triangle); 4 (2 × 2); 6 (2 × 3); Poisson-disc placement | 4 in a 2 × 2 pattern with ±128 wu jitter (WLD-040) | Four gives every site exactly two valley neighbours and a diagonal, a central resource site all four reach at the same distance, and equal sites by symmetry (pillar 6); 2,400 bodies per region at a placeholder 600 per settlement is near the reference match's crowd scale. Six does not fit with 2,080 wu spacing in a 5,120 wu valley; Poisson-disc sites are unequal |
| Site spacing and largest territory | spacing 1,920 to 2,400 wu × largest radius 1,300 to 1,500 wu (model sweep) | 2,080 wu spacing, 1,400 wu radius (WLD-040, WLD-060) | First valley contact on day 5 with a square-root growth (battles from week 2, PIL-065), a 720 wu border area at plateau, the valley centre 71 wu outside every territory; 2,400 wu delays contact to day 8.5, 1,920 wu puts the centre inside the territories |
| Contested areas (PIL-120) | a grid of claimable tracts; named border areas only; any territory outside the core | named static border areas (valley lenses, pass corridors) and contested resource sites; home cells never claimable (WLD-061, WLD-062) | Few, named, visible places are legible (pillar 3) and cheap for the server; a home cell that no claim can take keeps a lost claim a setback (pillar 6) |
| Holding a border area | split at the bisector; first to reach the halfway line holds the whole lens; shared | first to reach the halfway line, never while the other site is free, never against a newcomer under protection (WLD-063) | One holder per area makes claims discrete; the free-site and newcomer rules stop an incumbent from pre-claiming a late joiner's land (pillar 6) |
| AI settlement count | a fixed share of sites; a constant; 6 + 0.25 × capacity, filled to whole regions | 6 + 0.25 × S, filled (WLD-004) | Solo needs neighbours (9 AI), 16 needs one per valley (10), 200 needs fewer per steward (58, 22 % of sites) to keep A2's AI load bounded |
| AI placement | one per region, then diagonal; diagonal pairs from the start; random | one per region in ring order, then the diagonal (WLD-121) | Every valley has a settlement from day 1 when A ≥ R; diagonal pairs only help at 200 and 25 % occupancy (model `ai`), and would leave valleys empty at 16 |
| Founding order | any free site, random; ranked by occupied neighbours; forced placement | ranked list, three highlighted, one recommended by median-strength neighbours; any site allowed (WLD-140 to WLD-142) | Random founding leaves 12 % of stewards with no contact at 200 and 25 % occupancy, ranked founding none (model `ai`); forced placement takes the choice away; the recommendation seats a late joiner among median neighbours without revealing strength (PIL-029, PIL-084) |
| Free sites (PIL-068) | a reserve that grows with capacity; exactly capacity + 2 by construction | by construction: sites = capacity + AI + 2 (WLD-004, WLD-123) | The invariant cannot break; dormant stewards keep a place for 42 world days so a return is guaranteed (WLD-005) |
| Warning of PIL-025 against travel | make the world sparse enough that travel takes 10 minutes; hold the expedition | hold (WLD-102) | Ten minutes at 20 wu/s is 12,000 wu; settlements that far apart never touch, which kills territory pressure (pillar 3). Settlements in contact are 52 s apart, so the hold of A0's Figure 5 provides the warning; distant attacks warn at the estimated arrival minus 10 minutes |
| World lifetime default | unlimited everywhere; 12 weeks everywhere; unlimited integrated and 12 weeks dedicated; 4 weeks | unlimited integrated, 84 world days dedicated, stage 1 builds only unlimited (WLD-220) | A friends' world should not end unasked; a world of strangers gains an arc, a fresh start for late joiners and a clean patch boundary (seed challenge 4); A0's Table 10 puts the lifetime in stage 2. Owner question Q2 |
| End of a lifetime | hard stop; finish battles then archive; reset in place | finish battles, read-only archive for 30 real days, successor world by invitation, no carry-over (WLD-223) | No settlement is removed (PIL-069, PIL-080); a carry-over of strength would bring the runaway leader into the next world (pillar 6) |
| Last days | nothing special; no attacks in the last world day; protection for everyone in the last week | final notice 7 world days, no attack declarations in the last world day (WLD-222) | Stops last-minute raids on the standing; one day keeps the last week alive |
| World calendar | show world time as a clock; world days of 24 h at both paces; world days scaled by the pace | world day = 24 h ÷ pace factor (WLD-200) | Day 14 then means the plateau at both paces, so every timer of the economy, relations and protection reads the same in a session-paced world (PIL-103); no clock time, because there is no time of day |
| Time of day | a day and night lighting cycle; none | none (WLD-185) | It would change the reference look (pillar 5) and reward play at set hours (pillar 2's rule-out) |
| Weather in a world | the reference map effects in the simulation; look only | look only; lightning and dust vortices never act in a world in v1 (WLD-184) | An unannounced loss while away breaks PIL-049; per-region weather objects would cost server CPU for nothing |
| Biomes | rule differences per biome (richer, harsher); look and terrain shape only | look and shape only, six biomes on the existing ramp files (WLD-180) | Symmetric worlds (O7) and equal sites (pillar 6); the 14 ramps, 8 gradients and 4 sea tiles already exist in the manifest |
| Lighting (DEC-562) | per-map raw values; named presets | named presets in the key-and-fill pattern (WLD-183) | Consistent look across new maps and biomes (pillar 5) |
| Strategic view | a separate 2-D map; the same 3-D world at a coarse level of detail | the same world, same renderer features, coarse facets, marks instead of Worker cards (WLD-240 to WLD-246) | Pillar 5 rules out a second art style; one camera with two height ranges is cheaper and keeps orientation |
| Information in the strategic view | everything public; everything fogged; a public layer of geography and record plus a seen layer | two layers (WLD-242) | Grievances and claims need their places public (pillar 3); expeditions, structures and strength stay fogged (PIL-084) |
| Settlement view bounds | anywhere; own territory only; own areas (territory + margin, own battles, own expeditions' sight) | own areas (WLD-244) | Keeps the watched set, and so A2's full-fidelity cost, bounded at 6 settlements per present steward |
| Map editor (DEC-045) | A: new editor with undo, round trip, play test; B: no editor, external authoring and the content build; a player-facing editor in v1 | B for the first playable, A as a Debug-build tool after it, no player editor in v1 (WLD-260) | Modding is a vision non-goal (§7); the first playable needs one or two maps; the world templates and set pieces need a tool later |
| Custom maps | keep the container and the player map folder; drop them | no container, no player folder; Debug builds read a development folder (WLD-261) | Nothing in v1 uses them; less attack surface (no player-supplied files) |

## 2 Recommendations (improvements beyond the vision)

| Recommendation | Benefit | Cost | Where |
|---|---|---|---|
| Valleys and passes as the geography of conflict | Every conflict has a named place on the map; chokepoints give the defender's advantage a shape; the partition is clean for A2 | Generator work for ridges and passes, and their validation | WLD-021, WLD-024, WLD-062 |
| Founding ranking with a recommended site | No steward founds alone (12 % do with random founding at 200, 25 %); late joiners sit among median neighbours | A server-side ranking and one strength comparison per founding | WLD-140 to WLD-142 |
| Reproducible worlds from a seed | Community events, bug reports and tests can recreate a world exactly; extension without regenerating | Counter-based draws in the generator (A2's DEC-242) | WLD-161 |
| Re-roll on validation failure | Fairness checked, not hoped for | Generation time; a failure path in world creation | WLD-163 |
| A world calendar that contains the pace | One number (world day) means the same stage at both paces; areas write timers once | A conversion rule for every timer owner | WLD-200 to WLD-202 |
| Final-day attack close | No last-minute raids on the standing | One rule for B5 | WLD-222 |
| Dormant stewards keep a place for 42 world days | A returning steward always gets back into a full world | Capacity can look full while places are held | WLD-005 |
| Capacity raise by extension (stage 2) | Integrated worlds that move to a dedicated server can grow | Extension logic in the generator and chunk delivery | WLD-008 |
| Unbiased generator variants | Worlds without method 2's directional streaks; smoothing that does not sink terrain | Two enumerators and their tests in the terrain component | WLD-268 |
| The session-pace battle concurrency finding for A2 | An integrated world at the session pace has 3 to 4 battles in progress at the 99th percentile, more than a dedicated world of 16 (2) | A2 sizes the integrated host for it | Table 9 |

**Not recommended:** a day and night cycle; weather hazards in worlds; per-biome rule differences; a player-facing map editor in v1; carrying strength into a successor world; making territories cross region borders.

## 3 Assumptions

1. Expeditions travel at about the Worker's speed, 20 wu/s (B4 sets it; WLD-104 scales Table 6).
2. B1's territory grows like the square root of time to the plateau of day 14; with linear growth, first contact slips to day 8.4 (WLD-064 sets the target, not the curve).
3. A settlement at its plateau holds about 600 Workers and Leaders (placeholder for B1; Table 9's bodies per region scale with it).
4. Half of all attacks are declared by present stewards, the rest by AI settlements uniformly in time (B5); present play falls in a 5-hour evening window of one time zone (worst case for the peak).
5. A gathering expedition goes to a contested resource site 6 times a world day per settlement and stays 5 minutes (B2).
6. The cell size of 16 wu (the reference median) is fine enough for crowds and buildings at world scale; D8 may quantise heights to 16 bits.
7. The reference test machine (DEC-971) generates and bakes a region in well under a second (about 2.5 million generator calls a region); the budgets of WLD-165 are first guesses until measured.
8. A2 can simulate a battle that reaches 320 wu into a second region (a pass battle).
9. Stewards accept a regular 2 × 2 site pattern if terrain variation and jitter hide it on the ground; the strategic view shows it plainly.
10. B5 keeps attacks mostly between settlements in contact, so the attack hold is the usual case and distant attacks are rare.
11. The world-creation page (C2) can show the progress of a generation that takes up to 2 minutes at 200.
12. E2 can give the six biomes distinct looks from ramp files 01 to 06, gradients 01 to 06 and sea tiles 01 to 04.

## 4 Owner questions (for gate B)

| # | Question | Options (recommended first) | Recommendation and trade-off |
|---|---|---|---|
| Q1 | World size per player count (gate B's scale budgets): accept the sizes of Table 3, with AI settlements 6 + 0.25 × capacity filled to whole valleys (9 AI for a solo world, 10 at 16, 58 at 200) and four sites per valley? | (a) as proposed; (b) more AI per steward at 200 (one per valley at every size, 65 at 200); (c) fewer AI everywhere (a fixed 6) | (a): solo and small worlds get neighbours, every valley has a settlement from day 1, and the AI load at 200 stays at 22 % of settlements. (b) adds 7 AI settlements of server load at 200 for little contact gain once stewards arrive; (c) leaves a solo steward with one or two neighbours |
| Q2 | Default lifetime of a dedicated world from stage 2 | (a) 84 world days (12 weeks), unlimited for integrated worlds; (b) unlimited everywhere; (c) 28 world days | (a) gives a world of strangers an arc and a fresh start for late joiners while a friends' world never ends unasked; (b) relies only on upkeep and claims against runaway leaders and makes patch migration open-ended; (c) is shorter than the 14-day plateau plus a month of the world loop |
| Q3 | Capacity of a community dedicated world in stage 2 | (a) up to 16, as O1's starting tier; (b) up to 50 | (a) keeps stage 2 inside the budgets A2 proves at 16; (b) serves larger communities sooner but needs A2's 50-steward budgets before stage 3 |

## 5 Adoption (reference/08, ledger owned by A1, B9 for the skirmish maps)

All 42 sections and 25 exception rows are filled; none is pending. Effective verdicts: **adopt 7, adopt-skirmish 70, adapt 108, drop 0**.

- **adopt (7):** R-MAP-086 (terrain colour assets, used by the biomes), R-MAP-198 (sea border, used by every world), R-MAP-356 (a map tool's own generator), R-MAP-388 to R-MAP-390 (the lossless writers), R-MAP-400 (validation on save). They bind the world generator's writer and every map tool (WLD-262).
- **adopt-skirmish (70):** the skirmish map model of rules and conventions §2.1 to §2.8, §2.10, §2.11, §2.13 to §2.16 and §2.23, except the exceptions below; the world has no map definition, match setup or start positions.
- **adapt (108):** map editor hooks §2.1 to §2.14, §2.16, §2.17 (to WLD-260, WLD-261, WLD-183, WLD-265, WLD-268, WLD-262), as the design input of the development editor; R-MAP-007 (WLD-263); R-MAP-013, R-MAP-073, R-MAP-080, R-MAP-092, R-MAP-131 and §2.12 (WLD-265, DEC-865); R-MAP-014 and R-MAP-060 (WLD-264, WLD-266, DEC-802); R-MAP-062 (WLD-266, DEC-866); R-MAP-081 and R-MAP-083 (WLD-268, DEC-324); R-MAP-085 (WLD-162); §2.9 lights (WLD-183, WLD-267, DEC-562); the census rows R-MAP-110, R-MAP-151, R-MAP-152, R-MAP-191, R-MAP-192, R-MAP-200 and the sections §2.17 to §2.22, §2.24, §2.25 (PIL-003, as in the reference/00 ledger: B9 sets the map set; the five other modes wait until scheduled).
- **drop (0).**
- **For B9:** every skirmish row is A1's proposal; B9 confirms it or sends a row change (cross-area request below).

## 6 Decisions (the eight of the triage), proposed resolutions

| DEC | Proposed | Decision | Implemented by |
|---|---|---|---|
| DEC-045 Map editor | Resolved | Neither option alone: B (no editor, text authoring validated by the content build, previewed in a Debug build) for the first playable, then A as a development editor in Debug builds after the first playable, with undo, unsaved-change warnings, stable-id selection, lossless writers, validation on save, a play test of the real simulation, link tools with removal, terrain on demand with the unbiased variants, and world-template and set-piece authoring. No editor in player builds in v1; a player-facing editor waits for modding (vision §7) | WLD-260, WLD-261, WLD-262 |
| DEC-315 Level-design patterns | Resolved | Option A, the specified default stands: every pattern is a supported tool; B9 playtests the effect of each pattern a Species map uses | WLD-269 |
| DEC-324 Generator semantics and biases | Resolved | Option B: keep the arithmetic exactly (conformance vectors hold), add generation method 3 (uniform parent, ⌊Next() ÷ 8,192⌋) and flatten type 5 (smooth over five); the world generator uses them; the inferred parameter semantics are documented as TER-106 states | WLD-162, WLD-268 |
| DEC-562 Lighting and weather presets | Resolved | Option B: named lighting presets (key and fill) and weather presets, selected by skirmish maps and biomes; play-changing effects stay explicit map flags; in a world, weather is presentation only and lightning and dust vortices never act | WLD-183, WLD-184, WLD-267 |
| DEC-802 Maps that match setup does not list | Resolved | Option B: an explicit visibility field (listed, test); test maps never in player editions; world templates listed by the world-creation page | WLD-264 |
| DEC-865 Map data without a reference case | Resolved | Option B with item 5 as stated: repeated mode, Essence-orb rain, routes block, non-square map and a Trooper record outside a group are build errors; the world game uses none of the six forms | WLD-265 |
| DEC-866 Population-cap values | Resolved | Item 1 option A (no unlimited defender cap), item 2 option B (cap 0 rejected); B9 sets each map's cap; the world has no map cap, replaced by B1's limit within A2's region budget | WLD-266, WLD-281 |
| DEC-966 Terrain query edge cases | Resolved | Item 1 option A (an axis-parallel ray visits its column or row), item 2 option B (scan columns × rows) | WLD-270 |

None changes a pillar, an owner decision or the stage plan, so none is escalated. All are dated 2026-10-18 at Finalise.

## 7 Cross-area requests

| To | Request |
|---|---|
| Coordinator | Register the proposed terms in CONVENTIONS §8.15 and the glossary at Finalise: resource site (`ResourceSite`, string-id prefix proposed `region.resource`), border area (`BorderArea`, `region.border_area`), pass (`RegionPass`, `region.pass`), home cell (no identifier), biome (`Biome`, `region.biome`), world template (`WorldTemplate`), set piece (`SetPiece`), world day (no identifier). Region names use string ids `region.name.<nnn>` under the registered root `region`. Refresh the ledger README totals (`scripts/LedgerTotals.py`) and the GDD index row for A1 |
| A2 | Use Table 9 and Table 3 for the server budgets; set the region body budget of WLD-281; decide which region simulates a pass battle (WLD-282); provide counter-based draws keyed by (seed, purpose, region, index) under DEC-242 for world generation (WLD-161); note that an integrated world at the session pace has 3 to 4 battles in progress at the 99th percentile |
| B1 | Grow territory to 1,040 wu by world day 5 and 1,200 wu by day 9, and to 1,400 wu at the plateau of day 14 (WLD-064); size the core with A1 (400 wu); decide whether a late joiner's growth reaches the halfway line within newcomer protection (WLD-064); keep the plateau population within WLD-281; place a returning dormant settlement's structures at a new site (WLD-143); the Wellspring at the site centre (WLD-041) |
| B2 | Fill the 8 home resource slots per site with kinds and equal yields (WLD-081, WLD-082); set the border, pass and central factors' value against claims; specify gathering at central resource sites (wilderness); size gathering per journey at both paces (WLD-103) |
| B4 | Set the expedition speed and run the model (WLD-104); keep lightning and dust vortices out of worlds (WLD-184); decide DEC-101 with the world seasonal state of WLD-185; specify pass battles with A2 (WLD-282) |
| B5 | Claims on border areas and contested resource sites only (WLD-061, WLD-062); contact events as the territory-pressure input (WLD-066); the attack hold and warning timing (WLD-102); no attack declarations in the last world day (WLD-222); AI Cladan assignment (WLD-122) or its replacement; limit the reach of an attack, recommended to at most two region hops (about 12 minutes), so distant attacks stay rare |
| B6 | Set newcomer protection knowing that it holds a late joiner's border areas (WLD-063); decide whether the warning converts at the session pace (50 seconds, PIL-103) or stays 10 minutes (WLD-102); confirm the 42 world days for which a dormant steward keeps a place (WLD-005) |
| B7 | AI settlements are placed by WLD-121 and play under the seen layer of WLD-242 |
| B9 | Confirm or change the skirmish rows of the reference/08 ledger; set each skirmish map's population cap (WLD-266); author the first-playable maps as text with the visibility field (WLD-264); playtest the DEC-315 patterns that Species maps use (WLD-269) |
| C1 | The two views and their heights (WLD-240); the settlement view's bounds (WLD-244); camera controls across the switch at 5,000 to 6,000 wu |
| C2 | The founding page (WLD-140 to WLD-142), the world-creation page with Table 4 and generation progress (WLD-165), the join page with lifetime changes (WLD-221), the ended-world archive (WLD-223) |
| C3 | The calendar dial and durations in world days (WLD-201); the strategic view's elements of Table 8 |
| D1 | World creation on the server with its failure path (WLD-163, WLD-165); capacity raises and extension (WLD-008) |
| D4 | Register the record types `maps.world_template` and a set-piece record in the map schema family (WLD-167); add the visibility field and remove the population cap's 0 form in the map schema (WLD-264, WLD-266); store baked chunks with digests (WLD-166) and ended-world archives for 30 real days (WLD-223) |
| D8 | Decide DEC-246 and DEC-247 with WLD-166 as A1's recommendation; amend the terrain component for generation method 3 and flatten type 5 (TER-032, TER-035, TER-100, TER-112, TER-143, TER-250) and for the highest-sample scan over columns × rows (TER-076, DEC-966); chunks of 1,024 wu; camera-relative precision of 0.01 wu at 66,560 wu (WLD-246); the strategic level of detail (WLD-245); a far plane for the strategic view (the reference far plane is 15,000 wu, the strategic camera rises to 46,080 wu) |
| D9 | Server-side computation of the seen layer of the strategic view (WLD-242) |
| D10 | Pin the reference test machine for the generation budgets (DEC-971, WLD-165) |
| E2 | Give ramp files 01 to 06, gradients 01 to 06 and sea tiles 01 to 04 the six biome looks of Table 7; 6 lighting presets; 8 set pieces; the region name list of 120 names; new strategic-view glyphs and marks for Table 8; 1 world template at the first world-game milestone and 3 at launch |
| E3 | Schedule the development map editor after the first playable (WLD-260), the world generator and its validation, and the templates (WLD-160) |

## 8 Model results (for the rationale's "Model results")

`Tools/DesignModels/A1WorldDensity.py` (check: 64 figures, 0 differences).

- **Size:** 16 stewards: 7 regions, 28 sites, 10 AI, 25,600 wu extent, 0.72 M land samples. 200 stewards: 65 regions, 260 sites, 58 AI, 61,440 wu, 6.66 M samples. Solo: 3 regions, 12 sites, 9 AI.
- **Density:** settlements per region at 25 / 50 / 100 % occupancy: 2.0 / 2.6 / 3.7 at 16, 1.7 / 2.4 / 4.0 at 200; 2,400 bodies per full region at 600 per settlement.
- **Travel at 20 wu/s:** to a contact 0.9 min; valley neighbour 2.0; pass neighbour 3.0; region hop 5.9; diameter 23 min at 16, 66 at 200 (formation speed × 1.67). The warning needs a 9.1 min hold at the always-on pace and none at the session pace if B6 converts it.
- **Contact:** first valley contact day 5.0, first pass contact day 8.5 (square-root growth; linear growth 8.4 and 10.9); lens 720 wu; mean territory contacts at full occupancy 2.5 (16) and 2.8 (200); a steward's settlement 2.5 to 3.0. Random founding: 12 % of stewards with no contact at 200 and 25 %; ranked founding: 0 %.
- **Scale for A2:** battles in progress at the 99th percentile of the evening peak, at 2 / 6 / 10 battles per settlement per week: 1 / 2 / 2 at 16, 3 / 5 / 8 at 200, 2 / 3 / 4 at 16 at the session pace; per region at most 1; expeditions in progress 1.0 (16) and 9.9 (200); stewards present at the peak 3.7 and 46.7; watched settlements 11 to 22 (16) and 140 to 258 (200); terrain 5.7 MB of binary64 heights and 17.2 MB of normals at 16, 53.2 MB and 159.7 MB at 200 (13.3 MB at 16 bits).

## 9 Gate result

`python3 -I Tools/DesignCheck/CheckDesign.py Design --skip-mermaid --design-phase`: all gates pass; without `--skip-mermaid`, gate 4 passes (280 blocks, 0 failed). Gate 12 reports other areas' pending rows; gate 13 reports the design-phase decisions, the eight of A1 among them until Finalise; gate 15 reports that `rationale/A1.md` is written at Finalise.
