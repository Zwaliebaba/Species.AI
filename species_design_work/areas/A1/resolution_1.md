# A1 resolution, round 1

> Proposer: A1-P · Area: A1, world and regions · Answers: `species_design_work/areas/A1/challenge_1.md` (22 findings, 8 "make it better" items) · Documents revised: `Design/gdd/01_world_and_regions/` (split into three files and an index), `Design/gdd/adoption_ledger/08_maps_and_level_design_ledger.md`, `Tools/DesignModels/A1WorldDensity.py` · Date: 2026-10-18

## Summary

| Verdict | Count | Items |
|---|---|---|
| Accepted | 28 | A1-F01 to A1-F22 (A1-F13 in part), A1-M1, A1-M3, A1-M4, A1-M5, A1-M7 (in part), A1-M8 |
| Rejected | 1 | A1-M2 |
| Escalated | 1 | A1-M6 (owner question Q2) |

Open Critical or High findings after this round: none. The five High findings (A1-F01 to A1-F05) are accepted and closed by the changes below.

**What changed overall.** The weakest point, a world designed and measured only at its creation, is answered in the design and in the model. In the design: border areas are split between their two sides so the order of arrival never decides who holds land (WLD-062, WLD-063); a dormant steward holds no place, and returns wait in a queue served before newcomers (WLD-005, WLD-143); a site margin and stand-in AI settlements keep every steward among neighbours as a world fills and empties (WLD-004, WLD-124 to WLD-127); the world records its constants (WLD-009). In the model: a `lifetime` mode runs a world for a year of world days with launch, daily demand, exponential active lifetimes, 21 days on the map under the deputy (O34), dormancy, returns, the return queue and the keeper, over five seeds per scenario; the travel, crossing, bridge, fresh-world and watched figures are checked over seeds 1 to 10; and the battle load handed to A2 is the evening-peak figure with its range in three cases. The document passed 120 KB and is split into `Design/gdd/01_world_and_regions/` (`README.md`, `world_and_sites.md`, `terrain_time_and_views.md`, `skirmish_maps_and_map_tools.md`); every requirement id of round 1 is kept in exactly one file, and ten are new (WLD-009, WLD-010, WLD-085, WLD-086, WLD-124 to WLD-127, WLD-284, WLD-285). `A1WorldDensity.py check` reproduces every figure and every table of figures, in 140 comparisons, with 0 differences.

**The challenge model.** `A1C_WorldDensityChallenge.py` was run against the round-1 model, as the challenge records. It reads the round-1 interface (for example the `pass_` leg of the travel table), so it no longer runs against the revised model; each of its checks is answered by a mode of the revised model: C1 by WLD-101 and `report`, C2 by the whole-valley pattern (exact figures in `check`), C3 by `own_units` and `holdable_units_max`, C4 by `lifetime`, C5 by `battles`, C6 by `watched_union`, C7 by the sea channels (no walkable path), C8 by the reserved extent, C9 by `size_core` and the creation stand-ins, C10 by `founding` and the fresh-world table.

## Findings

### A1-F01 — First to reach holds the border area (High): Accepted

The lens between two valley neighbours is split at its halfway line into two **border areas**, one belonging to each side's site (WLD-062); a crossing's corridor is split the same way at the region border. A border area is held by its own side's settlement from the tick at which that settlement's territory enters it, and the other side enters it only by winning a claim (WLD-063, PIL-121, B5); when a settlement leaves the map, every border area it holds returns to its own side. Contact begins at the halfway line once both territories reach it, at 1,040 wu, and at a crossing's region border once both hold their crossing border areas, at 1,200 wu (WLD-064). The newcomer race of round 1 is deleted, and AC-06 and AC-07 replace round 1's AC-08 to AC-10: a settlement founded on world day 150 beside two settlements at plateau holds its own two border areas from the tick its territory enters each and meets each neighbour on day 5 after its founding. Every site, founded first or in month 6, holds the same 11 resource units by default (WLD-085), against round 1's 8 for a late joiner and 10 to 12 for an incumbent (challenge C3). The border resource site stands at the centre of each half (WLD-080), and the claim object is the other side's half, as the challenger's alternative proposed; the model's lens share by arrival is no longer a figure, because no arrival moves a border area.

### A1-F02 — Places held by dormant stewards (High): Accepted

`wld.world.dormant_place_days` is removed: a settlement that becomes dormant frees its site and its place at once (WLD-005, WLD-043), and only stewards whose settlement is on the map, those away under the deputy for the 21 days of O34 included, count against the capacity. A returning steward takes any free site or a stand-in's site through the founding choice, and when every place is taken waits in a return queue that the server serves before any new steward (WLD-143). In the lifetime model (Table 8 of world and sites), a world of 200 with strong demand refuses 22 of 532 arrivals by day 180 and 25 of 903 by day 365, all of them while every place is taken by a settlement on the map, against the challenger's 147 of 895 with the 42-day place; a 16-steward world with a new friend every 4 days keeps 14 to 15 stewards on the map and refuses 12 of 57 by day 180 because it is full, and the slower friends' world refuses none. The challenger's return margin is met by the site margin of WLD-004, which also holds the stand-ins.

### A1-F03 — The ridges are not barriers (High): Accepted

The challenger's recommended option is adopted: every region is an island valley whose coast falls to a **sea channel** of `wld.region.channel_width` (256 wu) along every side (WLD-021), and two regions are linked only by a **crossing**, a land bridge of 384 wu over the channel (WLD-024). Water already stops every walking actor under the adopted rules (R-UNIT-112, R-SIM-379, R-UNIT-161), so no movement rule changes (WLD-002). The generator checks every sample of every channel strip outside a crossing and every sample of every crossing's centre line (WLD-163 checks 1 and 2), and AC-02 drives Workers, Leaders and a crowd order straight at a site across a channel. In the two-region prototype on the conformance generator, 100 seeds of 100 keep the channel continuous and the crossing dry and gentle. If B4 keeps Transports in the world game, B4 states whether they cross channels (WLD-021).

### A1-F04 — The warning is a forecast (High): Accepted

The warning is a gate, not a forecast: the target is warned when the attack is declared, and the simulation refuses the attacking expedition entry to the target's territory until `pil.conflict.warning_minutes` (10 minutes of world time) have passed since the warning, whatever its route, speed or any effect on it; an expedition that arrives sooner waits at the edge, and the hold is shown to both sides (WLD-102). The travel estimate is shown and decides nothing. AC-09 boosts an expedition to 80 wu/s after its warning and checks its entry time at both paces; Figure 3 of terrain, time and views shows the gate. The challenger's warning radius is not needed, because a warning at the declaration gives the target at least the warning time and usually the whole journey (A1-M7).

### A1-F05 — The battle load is the low end (High): Accepted

WLD-284 hands A2 the number of battles in progress at the evening peak of one time zone with its range (Table 6 of terrain, time and views), from the model's `battles` mode: a compound Poisson process of clustered starts in three cases, **base** (8-minute battles, half of attacks by present stewards in a 5-hour window, single starts), **planning** (12 minutes, 80 % in a 3-hour window, starts in pairs) and **stress** (as planning, starts in fours), each with the 99th percentile and the weekly maximum, at 2, 6 and 10 battles per settlement per week. A2 sizes for the planning case at 6 a week: 4 battles at 16 stewards (6 at the session pace) and 16 at 200, 6,400 figures in battle; the top of the range, the stress case at 10 a week, is 8 at 16 (12 at the session pace) and 28 at 200, 11,200 figures. These match the challenger's C5 (7 to 8 with one change, 16 to 28 with combined changes, at 200). "Upper bound" is gone: the one-zone figures are the planning figures. WLD-285 adds caps with a defined failure behaviour: at most `wld.scale.battles_per_region_max` (2) battles per region and max(4, ⌈0.1 × settlements at full occupancy⌉) per world, 4 at 16 and 26 at 200; an attack beyond a cap waits at its gate in declaration order, shown to both sides. In the planning case the world cap is reached during 4 % of the evening peak at 16, 11 % at the session pace and 0 % at 200. A2 sets the caps at owner gate B (owner question Q4).

### A1-F06 — Density decays (Medium): Accepted

The challenger's density keeper (A1-M3) is adopted as **stand-in AI settlements** (WLD-124 to WLD-127): a world has a site margin of ⌊0.25 × capacity + 0.5⌋ sites (WLD-004); at creation the keeper places a stand-in in every valley that has no settlement; during play, a steward's settlement with fewer than 2 territory contacts for 3 world days gets a stand-in at a free neighbouring site, and when the margin is used up a stand-in without steward neighbours steps back to make room; a stand-in steps back, dormant with its stores, when a steward wants its site or the bounds shrink; it never takes a reserved free site and never raises the settlements on the map above the capacity plus the native AI count, so A2's full-occupancy load is unchanged. Over a year (Table 8 of world and sites, means of five seeds), a world of 200 with weak demand has on day 180 50 stewards on the map, 2.02 settlements per region, a steward's mean of 2.61 contacts, 1 % of stewards below 2 contacts (11 % with the keeper off) and 96 % with a steward neighbour, against the challenger's 1.43 settlements per region and 1.33 contacts; the slow friends' world of 16 has 2.40 settlements per region, 2.39 contacts, none below 2 (14 % with the keeper off) and 81 % with a steward neighbour. WLD-065 now states the band with the model's year: at most 5 % of stewards below 2 contacts after the launch month. The frontier alternative is not adopted, because the founding ranking (A1-F13) already seats newcomers beside settlements and a frontier would conflict with the extension order of WLD-008.

### A1-F07 — Per-site jitter (Medium): Accepted

The valley's 2 × 2 pattern moves as a whole: one seeded offset of up to `wld.site.valley_offset` (64 wu) per axis and one rotation of up to `wld.site.valley_rotation` (3°) per valley (WLD-040), replacing `wld.site.jitter`. Every valley pair is exactly 2,080 wu apart, every lens 720 wu wide, every first valley contact on day 5.0 and every first crossing contact on day 8.5, and every home cell with its own border areas 4.84 million wu², at every site of every world (WLD-061, WLD-062, WLD-064); `check` states these as exact figures.

### A1-F08 — Sites are not equal in passes (Medium): Accepted

The text says 0 or 1 crossing per site, with the model's counts over seeds 1 to 10: 12 to 16 of the 32 sites at 16 and 40 to 44 of the 308 at 200 have none (WLD-024). The crossing factor is set to the border factor, 1 (WLD-081), and a site without a crossing gets a ninth home resource site in its place (WLD-082), so every site holds 11 units (WLD-042, WLD-085). The founding choice shows a crossing as a contact, never as wealth (WLD-140).

### A1-F09 — Claims are not bound (Medium): Accepted

WLD-086: only the settlement on the other side of a border may claim a border area, and only the settlements of a valley may claim its central resource site. A settlement therefore holds at most `wld.resource.holdable_units_max` (17) units, its own 11, the three border areas across its own borders and its valley's central site, 1.55 times what every site is given; this bound is A1's input to the leader guard of PIL-028 (B2, B5). A territory is always one connected area (WLD-060), and AC-08 refuses a claim in another valley.

### A1-F10 — The raise moves the origin (Medium): Accepted

The extent is reserved for `wld.world.capacity_limit` (256) at creation: 14 region cells, 71,680 wu, with the land blob at its centre (WLD-001), so an extension grows inside it and never moves a stored position (WLD-008, AC-22). An extension brings the native AI settlements and the margin of WLD-004 for the new capacity, placed in the new regions by the rounds of WLD-121. All-sea chunks are one value each, so a world of 16 holds 200 land chunks and 4,700 sea chunks (WLD-166). An extension bakes the new regions and, of an existing region, only the coast-band chunks where a new crossing lands.

### A1-F11 — The session-pace warning (Medium): Accepted

The warning is 10 minutes of world time at both paces, because it bounds the movement of expeditions, which runs at the tick rate at both paces (WLD-102, PIL-101, PIL-103). B6 confirms it (cross-area request).

### A1-F12 — Watched settlements and expedition views (Medium): Accepted

An expedition's view never extends the settlement view, and beyond the steward's areas the settlement view draws only the public layer under black fog, so the view covers, for PIL-057, only the steward's own territory with a margin of 800 wu and the battlefields where the steward's settlement is a side (WLD-244). The model computes the union over the stewards present at the evening peak, not the sum, over seeds 1 to 10: 10 to 11 typical and 15 to 16 at most at 16 stewards, 116 to 118 typical and 174 to 177 at most at 200 (Table 5 of terrain, time and views). The largest set per steward is 6, its own, two valley neighbours, the valley's diagonal site and the two sites facing it across the channels, which is why the model's largest union exceeds the challenger's 162.

### A1-F13 — Stewards seated among AI (Medium): Accepted in part

Accepted: the founding score weighs a steward neighbour, valley or crossing, at `wld.founding.steward_weight` (6) against an AI valley neighbour's 2, and adds `wld.founding.room_weight` (1) for each valley neighbour site that is free or a stand-in's, so that the first stewards found where a later one can join them (WLD-140); a steward may name a steward to found beside, and that steward's neighbouring free and stand-in sites come first (WLD-140); a group of up to 4 reserves neighbouring sites together from stage 2 (WLD-141); stand-ins step aside for stewards (WLD-126). Over seeds 1 to 10, in a fresh world of 16, 75 to 100 % of stewards have a steward neighbour at 25 % occupancy, 88 to 100 % at 50 % and 81 to 100 % at full occupancy, and 100 % at 200 (Table 6 of world and sites), against the challenger's 0 to 44 % (C10) and the round-1 rule's 0 to 50 % at 25 %; over a year, 81 to 100 % in every scenario (Table 8). The room weight costs contacts at founding, which the keeper restores: a steward's contacts in a fresh world of 16 are 2.4 to 3.0. Rejected: valleys left free of AI for steward groups in worlds up to 16, because the native settlements stand one per valley first so that a solo steward and a thinly settled world have neighbours from day 1 (PIL-008), and the stand-ins that would otherwise fill such a valley step back as soon as a group chooses it.

### A1-F14 — The travel limits pin B4's speed (Medium): Accepted

WLD-101 states the world rule: an expedition outside a battle travels at `wld.travel.march_speed` (20 wu/s, the Worker's speed), whatever its formation. B4 confirms it or sets another speed, and then the model is run again and the region side and site spacing are scaled together so that the limits of WLD-101 hold (WLD-104). Table 7 of world and sites gives every leg at 20 and at 12 wu/s. Cross-area request to B4.

### A1-F15 — Geometry not protected against patches (Medium): Accepted

WLD-009: a world records at creation its world constants, the generator version, its template's content digest, its seed and every layout-shaping `wld.*` value with its native AI and margin counts, and reads them, never the live tuning, for its whole life; an extension uses the recorded versions, and the server keeps every template and generator version that a running world names (WLD-167). D4 specifies a migration. AC-13 retunes `wld.site.spacing` in a later patch and checks that a running world keeps its value.

### A1-F16 — Passes and ridges not shown to be buildable (Medium): Accepted

A prototype of two neighbouring regions, `species_design_work/scripts/A1P_RegionPrototype.py` on the conformance generator `Tools/DesignModels/TerrainGenerator.py` at a cell of 32 wu, builds the records of WLD-162 and runs checks 1 to 6 of WLD-163. Over 100 seeds, 100 pass every check at the first attempt, with plateau floors of 52 to 111 wu and rim steps of 3 to 5 wu, at 0.5 seconds of Python per region. Instead of a ramp type, the generator uses one new primitive, the **feathered flatten type 6** (WLD-268), a level square blended linearly to the existing height across a band, in two passes: the first generates the tiles and reads the heights at each core's rim, the valley centre and each crossing mouth; the second applies the feathered squares at those heights, the bridge as a chain of feathered squares whose heights rise linearly from mouth to mouth (WLD-162). With the reference Absolute type instead, rim steps are 31 to 52 wu over 20 seeds, above the new limit of 16 wu (WLD-041), and an earlier layout whose hill tiles overlapped the cores failed reachability in every seed tried, which is why hill tiles stay out of the cores' bands. The flatten budget is counted in the model: a region's layout needs at most 24 flatten areas (4 cores, the centre and the bridge squares that reach into it) over seeds 1 to 10, against a budget of 32. The generation budgets (20 and 120 seconds) remain first guesses until the C++ generator is measured on DEC-971's machine, and checks 7 and 8 need B2's weights and the land route graph.

### A1-F17 — The smallest world per stage (Medium): Accepted

WLD-010 and Table 5 of world and sites: stage 1 builds capacities 1 to 16, the native AI settlements, the margin and the keeper, the founding choice with "found beside", the return queue, the unlimited lifetime, no capacity raise, one template, 2 biomes, 2 lighting presets, no set pieces, the checks of WLD-163 for channels, crossings, plateaus, dry sites and reachability, both views and text authoring; stage 2 adds capacity raises, limited lifetimes with the final notice and the archive, group founding, 3 templates, 6 biomes, 6 presets, 8 set pieces, every check and the development map editor; stage 3 the capacities up to 200. The per-stage counts are keys (`wld.template.count_stage1`, `wld.biome.count_stage1`, `wld.light.preset_count_stage1` and their stage-2 keys).

### A1-F18 — AI placement and rounding statements (Low): Accepted

Rounding is half up, written as ⌊*x* + 0.5⌋, in WLD-004 and the model (`round_half_up`). The sites that rounding to whole regions adds go to native AI settlements, and at creation the keeper places a stand-in in every valley that has none, so every world from 1 to 256 stewards has a settlement in every valley on world day 1 (WLD-125); the statement of round 1 that the native count covers every valley is gone.

### A1-F19 — Home resource sites cannot be spread evenly (Low): Accepted

WLD-082 places every site's home resource sites in one fixed pattern measured from its outward diagonal: 3 at 350 wu at −60°, 0° and +60°, 5 at 800 wu at −72°, −36°, 0°, +36° and +72°, and a ninth at 1,050 wu on the diagonal at a site without a crossing, each at least 200 wu from the others, the border areas and the coast band, so every site has the same gathering distances.

### A1-F20 — Small contradictions (Low): Accepted

Table 2 of world and sites now shows the valley's other settlements in the strategic view; Table 4 offers the world pace to integrated worlds only (PIL-104); WLD-220 gives the hosted default, `wld.lifetime.default_hosted` (84 world days); WLD-246 states a world-space error of 0.01 wu at the far corner of the 71,680 wu extent, where a binary32 coordinate has a spacing of 0.0078 wu; WLD-141 holds the one-reservation rule.

### A1-F21 — A lowered AI count breaks WLD-065 (Low): Accepted

WLD-065 states its band at the default native AI count, and the world-creation page shows the host the model's contact figure for a lowered count before the host confirms it; the sites a lowered count frees join the margin, which the keeper may use (WLD-120).

### A1-F22 — No test on a generated world; census verdicts (Low): Accepted

AC-03 founds stewards by WLD-140 on ten generated worlds at 16 and 200 on the server and checks contacts, settlements per region and the holder of every border area against the model's figures for those seeds; AC-16 measures the land route of every edge on 100 generated worlds at 16 and 10 at 200; AC-17 keeps the model check. In the ledger, R-MAP-086 is `adapt` to WLD-180: the 14 ramps, 8 gradients and 4 sea tiles and the seasonal replacement are kept, and the census of their use on the reference maps is dropped (effective verdicts now adopt 6, adopt-skirmish 70, adapt 109).

## Make it better

### A1-M1 — Contested lenses instead of first-come lenses: Accepted

As A1-F01: every border area is split, its resource site is the claim object of the other side's half, and only a claim moves it (WLD-062, WLD-063, WLD-080, WLD-086).

### A1-M2 — Compact AI placement: Rejected

The benefit that M2 measured at 200 stewards and 25 % occupancy, a steward's mean of 2.92 contacts and 6 % of settlements below 2, is reached by the revised design without M2's cost: with one native settlement per valley in rounds, stand-ins in the empty valleys at creation and the founding ranking, a steward's settlement has a mean of 2.8 to 2.9 contacts over seeds 1 to 10 and no steward has fewer than 2 (Table 6 of world and sites), and every valley shows a settlement on world day 1, which M2 gives up. The mean over all settlements, AI included, is lower (1.2 to 1.3 at 25 % occupancy), because an AI settlement alone in its valley waits for neighbours; that mean is not a steward's experience, and it rises to 2.8 at full occupancy.

### A1-M3 — A density keeper over the world's life: Accepted

As A1-F06: stand-in AI settlements kept within the margin by the keeper (WLD-124 to WLD-127), with the lifetime model's year in Table 8 of world and sites. The AI count is the fixed native count plus a variable number of stand-ins within the margin, not a fixed count.

### A1-M4 — Water channels and land bridges: Accepted

As A1-F03 (WLD-021, WLD-024, WLD-163). The black-lit sea between glowing valleys is drawn in the strategic view with the region border as a glowing line along each channel's centre line (Table 4 of terrain, time and views).

### A1-M5 — Founding as a group: Accepted

As A1-F13: "found beside" from stage 1 and the group reservation of up to `wld.founding.group_max` (4) stewards from stage 2 (WLD-140, WLD-141).

### A1-M6 — Eras for unlimited worlds: Escalated (owner question Q2)

The challenger's view: an unlimited world, and every integrated world is one by default and can never become limited (WLD-221), needs a soft reset every `wld.era.weeks` world weeks that settles claims, returns every border area to its side and banks renown, curing a runaway leader's land without erasing investment (Table D of A0). The proposer's view: the revised geometry already bounds what land can add, at most 17 units against every site's 11, 1.55 times (WLD-086), well inside PIL-028's 3 times, and border areas return to their side whenever a holder leaves (WLD-063); the reset itself is a rule of claims and renown, which B5 owns and which comes with renown in stage 2 (PIL-125), and limited lifetimes already exist for hosts who want a reset. Recommendation: no eras in v1; B5 evaluates eras with renown in stage 2, with the lifetime model's year as its input.

### A1-M7 — A warning radius instead of an arrival estimate: Accepted in part

The hold is adopted (WLD-102, A1-F04): no attacking expedition enters the target's territory sooner than 10 minutes after its warning, at either pace, whatever its route or speed. The radius is not, because the warning is sent at the declaration: a radius would warn a distant target later than the declaration does and add a tuning value, and the gate alone already makes the warning immune to route choice and speed effects.

### A1-M8 — Time in the world-density model: Accepted

As the weakest point: the `lifetime` mode (five scenarios, five seeds each, days 30, 90, 180 and 365, with the keeper and without it), whose figures Table 8 of world and sites states and `check` reproduces; the scenarios are placeholders of B6 and C2, named in class `Scenario`.

## Keys retired in this round

None of these keys is reused for another meaning. `wld.world.dormant_place_days` (A1-F02, no replacement); `wld.region.ridge_half_width` and `wld.region.ridge_rise_min` (replaced by `wld.region.coast_band` and `wld.region.channel_width`); `wld.region.passes_per_side`, `wld.pass.width`, `wld.pass.min_normal_y`, `wld.contact.pass_day`, `wld.resource.pass_factor`, `wld.travel.pass_path_factor`, `wld.travel.pass_minutes_max`, `wld.founding.pass_weight` and `wld.scale.pass_battle_reach` (replaced by their `crossing` keys); `wld.site.jitter` (replaced by `wld.site.valley_offset` and `wld.site.valley_rotation`); `wld.contact.degree_min` (replaced by `wld.density.contacts_min`); `wld.resource.inner_band` and `wld.resource.outer_band` (replaced by `wld.resource.inner_radius`, `wld.resource.outer_radius` and `wld.resource.extra_radius`); `wld.generation.ridge_probe_spacing` (every channel sample is checked); `wld.template.count_first`, `wld.template.count_launch`, `wld.setpiece.count_launch` and `wld.light.preset_count_launch` (replaced by the per-stage keys).

## Owner questions (merged, for gate B)

1. **World size, native AI and stand-ins.** Recommended: native AI settlements 6 + 0.25 × capacity, a site margin of 0.25 × capacity for stand-in AI settlements, and a keeper that holds every steward at 2 territory contacts or more. Trade-off: B7 runs stand-ins that step back with their stores, and the native count rises with capacity, while A2's load at full occupancy is unchanged; without the keeper, a weakly settled world of 200 has 11 % of its stewards below 2 contacts on day 180 instead of 1 %. Proposer and challenger agree (A1-M3).
2. **Lifetime defaults and eras.** Recommended: unlimited for integrated worlds, 84 world days for community dedicated and hosted worlds, lengthened by the host at will before the final notice, and no eras in v1. Trade-off: a limited default ends a world that a community wants to keep unless its host lengthens it, and eras would add a soft reset that cures a runaway leader's land but erases standing that stewards built. Proposer and challenger disagree on eras (A1-M6): the challenger recommends them, the proposer leaves them to B5 with renown in stage 2.
3. **Stage-2 capacity.** Recommended: a community dedicated world holds up to 16 stewards, as an integrated one does. Trade-off: one load profile (4 battles in progress in the planning case) and modest host hardware, against larger community worlds, which would need A2's dedicated budget at gate B. No disagreement.
4. **Battle caps.** Recommended: at most 2 battles in progress per region and max(4, 10 % of the settlements at full occupancy) per world, 4 at 16 and 26 at 200, with attacks beyond a cap waiting at their gates in declaration order. Trade-off: attacks wait during 4 % of the evening peak at 16 stewards and 11 % at the session pace in the planning case, and two of a valley war's four battles wait; without caps, A2 sizes for the stress case, 28 battles and 11,200 figures at 200 and 12 battles on the host's PC at the session pace. No disagreement on caps (the challenger asked for them); A2 sets the values.
