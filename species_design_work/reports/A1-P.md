# Report A1-P: world and regions

> Area A1, wave A · Proposer · Date: 2026-10-18 · Status of the GDD: Draft, revised after challenge round 1 (Step 3, Revise)

## Files written

| File | What |
|---|---|
| `Design/gdd/01_world_and_regions/README.md` | The index of the split document: files, requirement ranges, acceptance-criterion ranges, the model |
| `Design/gdd/01_world_and_regions/world_and_sites.md` | Extent, size and capacity, world constants, the smallest world per stage, regions, channels and crossings, sites, territory and border areas, resources, travel and the attack gate, native and stand-in AI settlements, founding and returns (WLD-001 to WLD-144); 74 KB |
| `Design/gdd/01_world_and_regions/terrain_time_and_views.md` | The hybrid generator in two passes and its validation, biomes, lighting, weather, calendar, lifetime, the two views, the scale figures, the battle load and the caps (WLD-160 to WLD-246, WLD-280 to WLD-285); 65 KB |
| `Design/gdd/01_world_and_regions/skirmish_maps_and_map_tools.md` | Skirmish map and map tool rules, the generator variants with the feathered flatten type, the ledger mirror, the eight decisions (WLD-260 to WLD-270); 22 KB |
| `Design/gdd/01_world_and_regions.md` | Deleted: replaced by the folder (every requirement id kept, each defined once) |
| `Design/gdd/adoption_ledger/08_maps_and_level_design_ledger.md` | Link to the folder; R-MAP-086 to `adapt` WLD-180; R-MAP-083, R-MAP-085 and R-MAP-335 notes for the feathered type; totals |
| `Tools/DesignModels/A1WorldDensity.py` | Revised model: a clock (`lifetime`), seeds 1 to 10 for layout figures, five seeds per lifetime scenario, three battle cases with p99 and weekly maximum, caps, the watched union, bridges; `check` compares 140 figures and tables, 0 differences |
| `Tools/DesignModels/README.md` | The model's entry, revised |
| `species_design_work/scripts/A1P_RegionPrototype.py` | Two-region terrain prototype on the conformance generator (A1-F16) |
| `species_design_work/areas/A1/resolution_1.md` | Answers to A1-F01 to A1-F22 and A1-M1 to A1-M8, retired keys, merged owner questions |
| `species_design_work/areas/A1/proposal_notes.md` | Options, recommendations, assumptions, owner questions, adoption, decisions, cross-area requests, model results, revised |
| `species_design_work/reports/A1-P.md` | This report |

No other file was changed. No publishing, upload or Artifact tool was used. No git write command was run.

## Counts

- **Requirements:** 99 (98 MUST, 1 SHOULD: WLD-008), every round-1 id kept; new: WLD-009, WLD-010, WLD-085, WLD-086, WLD-124 to WLD-127, WLD-284, WLD-285.
- **Tuning keys:** 118 `wld.*` keys, 6 final and 112 provisional, each provisional row with a playtest note; 21 round-1 keys retired and not reused (listed in `resolution_1.md`).
- **Diagrams:** 10 Mermaid diagrams (4, 4 and 2 per file); gate 4: 284 blocks, 0 failed.
- **Edge cases:** 59; **acceptance criteria:** AC-01 to AC-46.

## Resolution of round 1

Accepted 28 (A1-F01 to A1-F22, A1-F13 in part; A1-M1, M3, M4, M5, M7 in part, M8), Rejected 1 (A1-M2), Escalated 1 (A1-M6, owner question 2). No Critical or High finding remains open.

## Model results

At **16 stewards**: 8 regions, 32 sites, 10 native AI, margin 4; valley neighbour 2.0 min, world diameter 23 to 25 min (seeds 1 to 10); first contact day 5 at every site; a steward's contacts in a fresh world 2.4 to 3.0, 75 to 100 % with a steward neighbour at 25 % occupancy; day 180 of the slow friends' world: 2.40 settlements per region, 0 % below 2 contacts; battles in progress at the evening peak, planning case at 6 a week, 4 (6 at the session pace), range 1 to 8 (2 to 12), cap 4. At **200**: 77 regions, 308 sites, 56 native AI, margin 50; diameter 70 to 76 min; contacts 2.8 to 2.9; day 180: 167 stewards (strong demand, 22 of 532 refused) or 50 (weak demand, 2.02 per region), 1 % below 2; battles 16 in the planning case (6,400 figures), range 3 to 28, cap 26; watched union 116 to 118 typical, 174 to 177 at most.

## Adoption verdicts (reference/08, 185 requirements)

adopt 6 · adopt-skirmish 70 · adapt 109 · drop 0 · pending 0.

## DEC and ADR changes

- Proposed Resolved (register edited at Finalise): DEC-045, DEC-315, DEC-324 (option B: method 3 and flatten types 5 and 6), DEC-562, DEC-802, DEC-865, DEC-866, DEC-966.
- Recommended to D8: DEC-246 (B with D) and DEC-247 (B, inside the reserved extent).
- No ADR written or changed; no new DEC.

## Owner questions (for gate B)

Q1 world size, native AI and stand-ins (recommend as proposed); Q2 lifetime defaults, 84 world days for dedicated and hosted, and no eras in v1 (the challenger recommends eras); Q3 stage-2 capacity 16; Q4 battle caps 2 per region and max(4, 10 %) per world with attacks waiting at the gate.

## Cross-area requests

In full in `proposal_notes.md` §7. Main ones: the coordinator updates the GDD index row and CONVENTIONS §3.1 to the folder; A2 sizes for the planning case and sets the region budget and the caps; B1 meets the contact days for every founding; B4 confirms the 20 wu/s march speed and bounds crossing battles; B5 owns claims across own borders and the waiting attack; B6 confirms the 10-minute warning at both paces and replaces the lifetime scenarios; B7 runs stand-ins; D8 amends the terrain component (method 3, flatten types 5 and 6, the columns × rows scan) and decides DEC-246 and DEC-247.

## Names proposed

resource site (`ResourceSite`), border area (`BorderArea`), crossing (`RegionCrossing`, replaces round 1's pass), home cell, native AI settlement, stand-in AI settlement, biome (`Biome`), world template (`WorldTemplate`, schema `maps.world_template`), set piece (`SetPiece`), world day. String ids `region.name.<nnn>`; proposed prefixes `region.resource`, `region.border_area`, `region.crossing`, `region.biome`. Biome ids `temperate`, `highland`, `arid`, `marsh`, `ember`, `frost`.

## Gate result

`python3 -I Tools/DesignCheck/CheckDesign.py Design --skip-mermaid --design-phase`: all gates pass; without `--skip-mermaid` gate 4 passes too (284 blocks). Gates 12, 13 and 15 report what is expected.

## What I verified and what I assumed

Verified: every figure that the three files state is reproduced by `A1WorldDensity.py check` (0 differences); every requirement, DEC, ADR and owner decision cited exists (gates 5, 6, 16); every key is defined once and cited (gate 8); the prototype's structural checks pass in 100 seeds of 100. Assumed: the twelve assumptions of `proposal_notes.md` §3, chiefly the 20 wu/s march speed, square-root territory growth, a plateau population of 600, and the battle cases and lifetime scenarios as placeholders of B4, B5, B6 and C2. Not verified: generation time of the C++ generator on DEC-971's machine; checks 7 and 8 of WLD-163, which need B2's weights and the land route graph. The challenger's script reads the round-1 model's interface and does not run against the revised model; its ten checks are answered by the revised model's modes.
