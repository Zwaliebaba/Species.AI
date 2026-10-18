# Report A1-P: world and regions

> Area A1, wave A · Proposer · Date: 2026-10-18 · Status of the GDD: Draft (proposal, before challenge round 1)

## Files written

| File | What |
|---|---|
| `Design/gdd/01_world_and_regions.md` | The GDD, prefix `WLD`, in the nine-heading template, status Draft, rationale path `../rationale/A1.md` as text; 108.6 KB of the 120 KB limit (a large revision would split §2.13 and §2.14 into a second file of a folder) |
| `Design/gdd/adoption_ledger/08_maps_and_level_design_ledger.md` | Every verdict of reference/08: 42 section rows and 25 exception rows, no `pending` row |
| `Tools/DesignModels/A1WorldDensity.py` | The world-density model (standard library only, `python3 -I`; modes `check`, `report`, `sweep`, `ai`, `layout`); `check` reproduces 64 figures of the GDD with 0 differences |
| `Tools/DesignModels/README.md` | The model's entry (added before "Adding a model") |
| `species_design_work/areas/A1/proposal_notes.md` | Options, recommendations, assumptions, owner questions, adoption, the eight decisions, cross-area requests, model results, gate result |
| `species_design_work/reports/A1-P.md` | This report |

No other file was changed. No publishing, upload or Artifact tool was used.

## Counts

- **Requirements:** 89, `WLD-001` to `WLD-283` in groups (88 MUST, 1 SHOULD: WLD-008, capacity raises by extension from stage 2).
- **Tuning keys:** 93 `wld.*` keys, 6 final and 87 provisional, each provisional row with a playtest note.
- **Diagrams:** 6 Mermaid diagrams (the life of a world, world creation, a region and its links, site states, border-area states, the two views); all parse (gate 4: 280 blocks, 0 failed).
- **Tables:** 13 numbered; **edge cases:** 35; **acceptance criteria:** 33.

## The design in brief

A world is one continent of square regions (5,120 wu) ringed by sea; each region is a valley of four sites (2 × 2, 2,080 wu apart) enclosed by a ridge that passes cross. Valley neighbours share a border area (the 720 wu lens of their largest territories), each pass corridor is a border area, and every valley centre holds a wilderness resource site. Size follows capacity: AI settlements 6 + 0.25 × capacity, filled to whole valleys, sites = capacity + AI + 2 (PIL-068 by construction). Terrain is **hybrid**: an authored world template and set pieces, a procedural layout from a seed, the reference generator for the terrain, validated with re-rolls, baked once into 1,024 wu chunks, never changed in play. The world calendar's day is 24 h ÷ the pace factor. Lifetime: unlimited for integrated worlds, 84 world days for dedicated ones (stage 2). The strategic view uses the renderer's visual language only, with a public layer and a seen layer.

## Model results

At **16 stewards**: 7 regions, 28 sites, 10 AI, 25,600 wu extent; valley neighbour 2.0 min, pass neighbour 3.0, world diameter 23 min at 20 wu/s; first valley contact day 5.0; 2.5 territory contacts per settlement at full occupancy; battles in progress at the 99th percentile of the evening peak 1, 2, 2 at 2, 6, 10 battles a week, and 2, 3, 4 at the session pace. At **200 stewards**: 65 regions, 260 sites, 58 AI, 61,440 wu; diameter 66 min; 2.8 contacts; battles 3, 5, 8; 46.7 stewards present at the peak; 6.66 M land samples (53.2 MB binary64 heights). Bodies per full region 2,400 at a placeholder 600 per settlement. Random founding leaves 12 % of stewards with no contact at 200 and 25 % occupancy; the ranked founding of WLD-140 leaves none.

## Adoption verdicts (reference/08, 185 requirements)

adopt 7 · adopt-skirmish 70 · adapt 108 · drop 0 · pending 0. Skirmish map rules as written; A1's decisions adapt the rules they change (WLD-260 to WLD-270); the map editor hooks become the development editor's design input; the census and the five other modes are `adapt` to PIL-003 as in the reference/00 ledger. B9 confirms the skirmish rows.

## DEC and ADR changes

- Proposed Resolved (register edited at Finalise): **DEC-045** (no editor in player builds; text authoring first, a development editor after the first playable), **DEC-315** (option A, the default stands), **DEC-324** (option B: generation method 3 and flatten type 5 added), **DEC-562** (option B: named presets; world weather is presentation only), **DEC-802** (option B: visibility field), **DEC-865** (option B with item 5 as stated), **DEC-866** (item 1 A, item 2 B), **DEC-966** (item 1 A, item 2 B).
- Recommended to D8: DEC-246 (B with D: baked region chunks by digest) and DEC-247 (B: one continuous field in region-aligned chunks).
- No ADR written or changed; no new DEC.

## Owner questions (for gate B)

Q1 world size per player count and the AI share (recommend as proposed); Q2 the default lifetime of a dedicated world (recommend 84 world days, unlimited for integrated); Q3 the stage-2 capacity of a community dedicated world (recommend 16).

## Cross-area requests

In full in `proposal_notes.md` §7. Main ones: A2 sizes the server from Table 9 and sets the region body budget (WLD-281) and pass battles (WLD-282); B1 meets the contact days (1,040 wu by day 5, 1,200 by day 9); B5 owns claims on the named areas and the attack hold (WLD-102); B6 decides the warning at the session pace and the newcomer protection that holds a late joiner's border areas; D4 registers `maps.world_template` and the visibility field; D8 amends the terrain component (method 3, flatten type 5, the columns × rows scan) and decides DEC-246 and DEC-247; E2 gives the six biomes their looks; B9 confirms the skirmish ledger rows.

## Names proposed

resource site (`ResourceSite`; already used by PIL-120), border area (`BorderArea`; PIL-120), pass (`RegionPass`), home cell, biome (`Biome`), world template (`WorldTemplate`, schema `maps.world_template`), set piece (`SetPiece`), world day. Proposed string ids `region.name.<nnn>`; proposed string-id prefixes `region.resource`, `region.border_area`, `region.pass`, `region.biome`. Biome ids `temperate`, `highland`, `arid`, `marsh`, `ember`, `frost`. World states Creating and Ended beside A0's Running and Frozen.

## Gate result

`python3 -I Tools/DesignCheck/CheckDesign.py Design --skip-mermaid --design-phase`: all 19 gates pass; without `--skip-mermaid`, gate 4 passes too. Gates 12, 13 and 15 report what is expected (other areas' pending rows, the design-phase decisions until Finalise, the rationale written at Finalise).

## What I verified and what I assumed

Verified: every cited requirement, DEC, ADR and owner decision exists (gates 5, 6, 16); every tuning key is in the table and cited (gate 8); the model's `check` matches every figure stated; the ledger covers all 185 ids with no pending row. Assumed: the twelve assumptions of `proposal_notes.md` §3, chiefly B4's expedition speed of 20 wu/s, B1's square-root territory growth and a plateau population of 600. Not verified: generation cost on real hardware (WLD-165 budgets are first guesses until the generator exists), and that the 2 × 2 site pattern reads well in the strategic view (a playtest note).
