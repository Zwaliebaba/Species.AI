# A1 challenge, round 1

> Challenger: A1-C · Area: A1, world and regions · Documents challenged: `Design/gdd/01_world_and_regions.md` (Draft), `Design/gdd/adoption_ledger/08_maps_and_level_design_ledger.md` (Draft), `Tools/DesignModels/A1WorldDensity.py` · Challenge model: `species_design_work/scripts/A1C_WorldDensityChallenge.py` · Date: 2026-10-18

Inputs read: the A1 document and its ledger, the vision brief, `owner_decisions.md` O1 to O37, prompt §1 to §6, the Final A0 document and `rationale/A0.md`, reference/08 (rules and conventions §2.8 to §2.11, map editor hooks), reference/02 (Worker movement and slope factors, the Leader's formation speed), reference/12 §2.8 to §2.10 (walkability, the route graph, path queries) and the terrain component (TER-002 to TER-009, TER-140 to TER-145, TER-250, TER-252). The proposer's notes and report were not read.

## Weakest point

**A1 designs and measures the world at the moment of its creation, never over its life.** Every density, contact and equality figure of §1.3, §2.4 and §2.15 is computed for a fresh world that stewards fill in ranked order up to a share of its capacity, and nothing in the model has a clock. A persistent world does not stay in that state, and A1's own rules decide what it becomes. A border area goes to the first settlement whose territory reaches its line (WLD-063), so arrival order becomes permanent: AI settlements, which stand from world creation, hold 89 to 100 % of the lenses they share with stewards, and 42 to 100 % of the stewards who join after week 4 hold no valley lens at all (challenge model C4). A dormant steward keeps a place for 42 world days after 21 days on the map under the deputy (WLD-005, PIL-078), so at saturation 56 % of a 200-world's places belong to stewards who have stopped playing, and over a year 147 of 895 joiners are refused while 87 sites stand empty; a 16-steward friends' world settles at about 7 active stewards and refuses 44 of 104 joiners. Where demand is weak, the AI count and the site set are fixed for ever (WLD-120, WLD-084), so a 200-world falls to 1.4 settlements per region with 63 % AI settlements, and a 16-world to 71 % AI with no steward bordering another. Late joiners in a months-old world (seed challenge 3), snowballing by early arrival (seed 4) and the 16-to-200 path (seed 12) all fail through this one gap: the geometry is sound, but the allocation of that geometry over time was never designed or modelled.

## Model checks

The proposer's model was run in all its modes. `check` reproduces the document's 64 figures with no difference, and `report`, `sweep` and `ai` run cleanly. Its code was read in full: it implements the geometry as the document states it (blob growth, the 2 × 2 valley, the pass parity rule, the travel network, the √t growth curve), but every figure comes from seed 1, the contact days, lens width and travel table use the geometry before jitter, the contact count treats AI and steward neighbours alike, `watched()` multiplies the stewards present by 3 or 6 without removing overlaps and uses the capacity rather than the stewards present, `battle_load` is a Poisson count of independent battle starts at the low end of every input, and the model has no time: no arrivals after founding, no churn, no dormancy, no held places and no border-area holders.

The challenge model (`python3 -I species_design_work/scripts/A1C_WorldDensityChallenge.py Tools/DesignModels/A1WorldDensity.py all`) loads the proposer's model by path and adds ten checks. Their results are cited by the findings below.

| Check | What it tests | What it shows |
|---|---|---|
| C1 speed | WLD-101's limits at 12, 15, 20 and 25 wu/s; the diameter over seeds 1 to 20 | Every limit needs at least 15.2 to 18.8 wu/s; the formation speed of 12 wu/s and 15 wu/s fail all five limits; the diameter over 20 seeds is 18.6 to 23.8 min at 16 and 64.5 to 67.5 min at 200, inside the limits at 20 wu/s |
| C2 jitter | The per-site jitter of WLD-040 over 10 worlds of 200 | Valley spacing 1,834 to 2,336 wu; first valley contact world day 3.0 to 7.7 (14 % of pairs before day 4, 20 % after day 6); lens 464 to 966 wu; first pass contact day 5.7 to 11.8; home cell 3.35 to 4.35 M wu² (1.30 ×); 23 % of valley pairs put a held lens inside the neighbour's core clearance |
| C3 late joiner | Territory and resource sites beside settlements at plateau | Late joiner 3.89 M wu² and 8 home-site units; incumbent with both lenses 5.69 M wu² and 10 units; with its pass 5.93 M wu² and 12 units (1.5 ×); with a central site 15 units (1.9 ×) |
| C4 churn | A year of world days: launch, demand, exponential active lifetime (43 or 120 days), 21 days on the map, then dormancy with or without the 42-day place; ranked founding; border areas held by the first to reach | Refused joiners while sites stand empty; ghost settlements; AI share and contact decay under weak demand; AI and launch stewards holding the lenses (figures in the findings) |
| C5 battles | Battles in progress against battle length, steward share, evening window and clustered starts | At 200 the model's p99 of 5 becomes 7 to 8 with any one plausible change and 16 to 28 with combined ones, inside PIL-065's band; at the session pace at 16, 3 becomes 4 to 8 |
| C6 watched | The union of watched settlements at the peak, with expedition views | At 200, full occupancy: union 118 typical and 162 at most (model 140 and 258); one expedition view per present steward raises the most to 178, three to 203 |
| C7 ridge | A Worker's straight walk over a border ridge, with the slope factor and no slope block | A ridge band costs a Worker 4 to 29 s; the target of an attack across a ridge gets 5.9 to 8.4 min of its 10-minute warning, and 2.5 or 5 min after a Speed Boost |
| C8 extension | A raise from 16 to 200 by WLD-008 | The land grows 3 cells below the original on both axes, so every stored position moves by 15,360 wu; the AI count stays 10, so 55 of 65 regions have no AI settlement |
| C9 AI and passes | AI against regions at capacities 1 to 256; a host who lowers the AI count; passes per site | AI settlements are fewer than regions at 168 capacities, the first being 74; at 16 with 2 AI the mean contacts at full occupancy are 2.33, below WLD-065's band; no site has 2 passes, and 43 % of the sites at 16 and 16 % at 200 have none |
| C10 adjacency | Stewards with a steward neighbour in a fresh world | At 16 with 4 or 8 stewards, 100 % of the stewards have no steward neighbour; with 16, 44 %; at 200 with 50, 100 % |

**What the model gets right, and should keep.** The region-based geometry scales without change from 1 to 256 stewards; the travel diameter over 20 seeds stays inside the limits at 20 wu/s; the counter-based layout keys make each region independent; and the geometry bounds what land can add to a settlement (8 home-site units at the least, 12 at the most from land, 15 with a central site), which is the kind of hard anti-snowball cap pillar 6 needs, provided claims are bound to it (A1-F09).

## Findings

### A1-F01 — First to reach holds the border area, so arrival order becomes permanent

- **Severity:** High
- **Category:** economy and balance
- **Failure scenario:** A world of 16 opens; the AI settlements stand from world creation (WLD-121) and grow from day 0. A steward founds on day 1 beside an AI settlement: the AI's territory reaches the halfway line on day 5.0, the steward's on day 6.0, and the AI holds the lens with its border resource site for good unless a claim moves it (WLD-063). In C4 the AI holds 89 to 100 % of the lenses it shares with stewards, in every scenario. In week 6 a late joiner founds at a site vacated by dormancy, between two settlements at plateau whose territories have sat at the halfway lines for weeks. WLD-063 gives each border area to "the first settlement whose territory reaches that halfway line", which is the neighbour, from the tick newcomer protection ends; the late joiner keeps its home cell, 3.89 M wu² and 8 home-site units, beside neighbours with 5.69 M wu² and 10 units, 12 with a pass (C3). From day 90 on, 42 to 86 % of the stewards who joined a 200-world after day 28 hold no valley lens, and 77 to 100 % in a 16-world; the launch cohort averages 9.3 to 10.1 units against 8.3 to 9.6 for later joiners (C4). The document's own escape clause cannot happen: WLD-064 and AC-10 say the late joiner gets the area if it reaches the halfway line first within its protection, but the neighbour reached it before the late joiner existed, so under WLD-063 as written AC-10's "unless" branch can never pass. This is the pattern that Table 1 of PIL-016 rules out for pillar 6 (first sites richer than later ones) and that WLD-042 promises away ("no first site is richer than a later one").
- **Recommendation:** Make holding independent of arrival order. Recommended: a valley lens is split at the halfway line by default (each side's territory takes its half once both sites are occupied), its border resource site stands on the line and is the claim object (PIL-120), and the whole lens changes hands only by a claim; a pass border area likewise splits at the ridge line. Alternative: a border area stays unheld until a claim settles it, and contact begins when both territories reach its edge. Either way, delete the newcomer-protection race of WLD-063 and WLD-064, rewrite AC-10, and add to the model the share of lenses held by AI, by the launch cohort and by later joiners.

### A1-F02 — Places held by dormant stewards lock late joiners out of a world with empty sites

- **Severity:** High
- **Category:** player experience
- **Failure scenario:** A community world of 200 with steady interest (2 newcomers a world day) and the churn that A0's challenger assumed (half the stewards gone a month). A steward who stops playing stays on the map under the deputy for 21 days (PIL-078), then keeps a place for 42 more world days (WLD-005), so each quitter takes a place for 63 days. At saturation in C4 the world holds 82 active stewards, 34 settlements on the map whose stewards have quit and 78 places held by dormant stewards: 56 % of the capacity belongs to people who no longer play. Over a year, 147 of 895 newcomers are refused while 87 sites (45 of them beside a settlement) stand empty; without the 42-day place, 23 are refused, all during the launch rush. In an engaged world (active lifetime 120 days) the place costs 40 active stewards (128 against 168) and 157 extra refusals. A 16-steward friends' world with a new friend every 4 days settles at 6 to 7 active stewards and refuses 44 of 104 joiners while 8 sites are free; without the place, about 10 active and 19 refused. Pillar 6 promises that "a steward who joins late finds a place" (PIL-015); here the place exists as a site but not as a place.
- **Recommendation:** Release a dormant steward's place at dormancy, and let a return use any free site, which the spare sites of PIL-068 and the sites freed by dormancy keep available; if a return finds the world full, it waits in a return queue served before new joiners. Count against the capacity only the stewards whose settlement is on the map. If the owner wants a guaranteed return, size the world with a return margin (`wld.world.return_margin`, for example 10 % of the capacity in extra sites) instead of holding places. Add the arrival-and-churn simulation to the model (Make it better M8) so that B6 and C2 tune dormancy against refusals.

### A1-F03 — The border ridges are not barriers under the adopted movement rules

- **Severity:** High
- **Category:** technical feasibility
- **Failure scenario:** WLD-021 makes each region a valley "so the walkability test of R-SIM-335 fails and no land route crosses the ridge". But the walkability test only steers route building, and only when its cliff flag is set; movement itself never stops on a slope. A Worker's movement step (R-UNIT-105) scales speed by the slope factor of R-UNIT-042, clamped at 0.3 for Workers and 0.1 for Leaders, and has no slope block (only the Trooper stops, below a normal y of 0.6). WalkCost (R-SIM-338) blocks only on water, walls and Barrier Emitters, so the direct-route override of R-SIM-351, with `_permitClimbing` true by default for crowd and Leader orders (R-SIM-353), replaces a node chain with a straight line over the ridge whenever that is cheaper. In C7 a Worker crosses the 640 wu ridge band in 36 s at a rise of 160 wu, 53 s at 300 and 61 s at 500, against 32 s on flat ground; the ridge's WalkCost is 984 to 2,579 wu against a pass detour of up to 5,400 wu more. A steward sends a crowd at the site behind the ridge that no pass links, and it walks straight over. The geometry that depends on the ridge then fails: passes are no longer the only links between regions (WLD-024, WLD-100), the travel network overstates distances, two hostile expeditions can meet and fight on a ridge where four regions meet (against WLD-282), and the attack warning is sent late (A1-F04).
- **Recommendation:** Make the barrier real in the simulation, and record it. Recommended: border the regions with water channels and make each pass a land bridge or ford: water already blocks `IsPathClear` (two wet samples) and `WalkCost` (one wet sample, so no direct-route override), and a channel of black sea suits the visual language (Make it better M4). Alternative: a world rule that a walking actor never moves uphill onto a sample whose normal y is below `ref.sim.walk.min_normal_y`, extending the Trooper's block of table 2.11-A to every walking kind in a world, with exception rows for R-UNIT-042 and R-UNIT-105 in the ledger of reference/02 (B4) and for R-SIM-351 in the ledger of reference/12 (D2), and a ridge that is steep along the whole of every probe line, not at one sample. Either way, add an acceptance criterion that drives Workers, Leaders and a crowd order straight at a ridge and checks that none crosses.

### A1-F04 — The attack warning is a forecast, so a faster route or a speed effect beats it

- **Severity:** High
- **Category:** griefing or exploit
- **Failure scenario:** WLD-102 sends the warning "at the later of the declaration and the estimated arrival minus the warning time", with the estimate taken over the travel network at the expedition's speed. An attacker declares on a settlement in the next region that no pass links to its own site. The server estimates 7.0 min over the network; the crowd walks straight over the ridge (A1-F03) and arrives in 2.9 min, so the target gets 5.9 min of its 10 (8.4 min for the other two cross-border pairs, C7). An attacker who triggers a Speed Boost after the warning goes out (R-UNIT-123: 40 wu/s, then 80 wu/s) cuts the 10 minutes to 5 or 2.5. PIL-025 is a rule of pillar 3 and binds every attack. The invariant that WLD-102 states ("the expedition enters the target's territory no sooner than the warning time after the warning went out") is right; the mechanism that is meant to deliver it cannot.
- **Recommendation:** Enforce the warning as a gate, not a forecast. The server sends the warning when the attacking expedition first comes within `wld.conflict.warning_radius` of the target's territory, set to the highest speed any effect can give an expedition × the warning time (or at the declaration for a settlement in contact), and the simulation refuses entry to the target's territory, holding the expedition at the gate, until the warning time has run since the warning. The estimate may stay for display only. B5 and B6 own the hold; A1 owns the radius and the acceptance criterion (an AC that boosts an expedition after its warning and checks its entry time).

### A1-F05 — The battles in progress handed to A2 are the low end of the plausible range

- **Severity:** High
- **Category:** scale and cost
- **Failure scenario:** Table 9 gives A2 a 99th percentile of 5 battles in progress at the evening peak of a 200-world, at 6 battles per settlement per week, with 8-minute battles, half of all attacks declared by present stewards in a 5-hour window, and independent battle starts; Table 10 calls the one-zone figure "the upper bound". Each input is the favourable end. In C5, at 6 battles a week, 12-minute battles give 7, a steward share of 0.8 gives 7, a 3-hour window gives 7, and attacks that start in pairs (a Cladan offensive) give 8; together they give 16, and 20 when four start together (a valley war). At 10 battles a week, still inside PIL-065's band, the range is 8 to 28: 3,200 to 11,200 figures in full-fidelity battle against Table 9's 2,000. The 16-steward integrated world at the session pace runs every battle on the host's PC: 3 at the p99 in Table 9, 4 to 8 under the same changes. The document sets no limit on battles in one region or one world, so A2 has no lever when the figure is wrong.
- **Recommendation:** Hand A2 the range with its drivers (battle length, steward share, evening window, cluster size, battles a week), not one value, and drop "upper bound" from Table 10. Add a cap with a defined failure behaviour: `wld.scale.battles_per_region_max` and `wld.scale.battles_per_world_max`, with an attack that would exceed a cap held at its gate (A1-F04) until a slot frees and the hold shown to both sides; A2 sizes the caps at owner gate B. Model the clustered starts in `battle_load` and report the weekly maximum beside the p99.

### A1-F06 — Density decays in a months-old world, and nothing restores it

- **Severity:** Medium
- **Category:** player experience
- **Failure scenario:** WLD-065's 2.5 to 3.0 contacts per settlement and Table 1's 4 settlements per region hold at full occupancy, and Table 5 shows lower occupancy only for a fresh world filled in ranked order. Over a year (C4), a 200-world with weak demand (120 at launch, then 1 newcomer every 2 days, half the stewards leaving a month) falls by day 180 to 21 active stewards, 1.43 settlements per region, 63 % AI settlements and 1.33 contacts per settlement. A 16-steward world whose friends join every 20 days falls to 3 or 4 active stewards, 71 % AI, 1.34 to 1.41 contacts, and by day 180 no steward borders another. Dormancy frees sites across the whole map, so the survivors are scattered; the AI count never changes (WLD-120) and no rule moves settlements together. The world the document designs exists for its first weeks.
- **Recommendation:** Add a density rule over the world's life. Recommended: a target of `wld.density.settlements_per_region_min` (for example 2) per occupied valley, kept by AI settlements that found at free sites of valleys below the target and step back (go dormant, keeping their stores) when a steward needs the site in a full world; AI settlements stay a share of the world's settlements (`wld.ai.share_max`) rather than a fixed count. Alternative or addition: open regions to founding progressively from the centre (a frontier), so new joiners concentrate where settlements already are. State WLD-065's band at the occupancies the churn model gives at months 1, 3 and 6.

### A1-F07 — The per-site jitter breaks the equality of sites and the day-5 contact

- **Severity:** Medium
- **Category:** economy and balance
- **Failure scenario:** WLD-040 moves each site independently by up to 128 wu on each axis, so the spacing of a valley pair varies by up to 256 wu. Over 10 worlds of 200 (C2): spacing 1,834 to 2,336 wu, first valley contact world day 3.0 to 7.7 (14 % of the pairs before day 4, 20 % after day 6, against WLD-064's day 5), lens 464 to 966 wu (WLD-062 says 720), first pass contact day 5.7 to 11.8 (WLD-064 targets 9), and home-cell area 3.35 to 4.35 M wu² (1.30 ×). In 23 % of the valley pairs the spacing is below 2,000 wu, so a held lens reaches inside the neighbour's core clearance and WLD-060's stop at 200 wu outside the core cuts it: the two rules disagree on what the border area is. Under WLD-082 the home resource sites must turn away from a near neighbour's lens: at the nearest jitter the free arc of the inner band is 159° for three sites spread 120° apart. WLD-042 says sites differ only in position, biome and passes, yet one site's home cell is 30 % larger than another's, and a world host who knows the seed and template (WLD-161) can find the largest home cells before founding.
- **Recommendation:** Jitter each valley's 2 × 2 pattern as a whole (one seeded offset of up to 128 wu and a rotation of up to `wld.site.rotation_max`, for example 4°, for all four sites), so every spacing, lens, home cell and contact day stays exact and the pattern still looks irregular. If per-site jitter stays, lower it to 32 wu and state contact days and lens widths as ranges that the model checks over seeds.

### A1-F08 — Sites are not equal in passes, and pass sites are worth more

- **Severity:** Medium
- **Category:** economy and balance
- **Failure scenario:** WLD-024's parity rule gives every interior site exactly one pass, and no site ever two: in C9, 0 of 260 sites at 200 have two passes, while 42 (16 %) have none; at 16, 12 of 28 sites (43 %) have none. WLD-024, WLD-042 and Table 10 say 0, 1 or 2. A pass-less site has 2 contacts instead of 3 and can hold at most 10 home-site units of resource sites against 12 for a pass site, because a pass resource site yields `wld.resource.pass_factor` (2) times a home one (WLD-081). The founding choice shows the pass count (WLD-140), so informed founders, and the world host, take the pass sites first, and the 43 % of pass-less sites in a 16-world go to the late and the uninformed.
- **Recommendation:** Correct the text to 0 or 1 pass per site. Equalise the potential: either set the pass factor to the border factor (1), so a pass-less site loses a contact but not a yield, or give every pass-less rim site a shore resource site of equal value that its valley neighbour can claim. Show the founding page's pass count as a contact, not as wealth.

### A1-F09 — Claims are not bound to the geometry, so the land cap does not hold

- **Severity:** Medium
- **Category:** economy and balance
- **Failure scenario:** WLD-063 lets a border area change holder "only by a claim", and WLD-081 lets B2 and B5 say how a settlement claims a central resource site; neither says who may claim. A strong settlement claims the lens between two weaker settlements of another valley, or the central resource sites of three valleys (3 home-site units each), and holds them through claims it wins (PIL-121). WLD-060 bounds a territory by the areas it holds, not by contiguity, so the territory becomes disjoint and the land that a settlement can hold has no limit. Bound, the geometry would cap land at 12 home-site units (15 with the valley's central site) against a late joiner's 8, a factor of 1.5 to 1.9, which is the kind of cap PIL-028's leader guard (3 ×) needs.
- **Recommendation:** Add the rule: only the two sides of a border area may hold or claim it, and only the settlements of a valley may claim its central resource site; state the resulting cap (`wld.resource.holdable_units_max`) as a requirement that serves pillar 6, and hand it to B2 and B5 for PIL-028.

### A1-F10 — The 16-to-200 raise moves the origin and leaves the AI behind

- **Severity:** Medium
- **Category:** technical feasibility
- **Failure scenario:** WLD-001 fixes the extent's corner at (0, 0) with one sea cell on every side, and WLD-008 grows a raised world "onto a sea cell that touches the blob in the order of WLD-003", which orders cells by their distance from the grid's centre. Raising a 16-steward world (7 regions) to 200 (65 regions) in C8 grows the land from cells 0 to 2 out to cells −3 to 5 on both axes, so every stored position, territory, route and chunk index must move by 15,360 wu on each axis to keep the corner at (0, 0), or the grid cannot grow downwards and the blob is lopsided. WLD-120 keeps the AI count at 10, so 55 of 65 regions start without an AI settlement and the AI share is 5 % against 22 % in a world created at 200, and WLD-122's Cladan balance is fixed at the old count. (Contact is not the problem: the raised world fills outwards from its core and gives 2.67 contacts at 25 % occupancy, more than the 1.85 of a world created at 200.)
- **Recommendation:** Put the world's origin at the centre of its first blob with signed coordinates (D8's camera-relative precision of WLD-246 covers the range), or reserve the extent of `wld.world.capacity_limit` at creation, since an all-sea chunk is stored as one value (WLD-166). State whether extension regions get AI settlements, and if they do, at what share.

### A1-F11 — The session-pace warning is read as 50 seconds, against PIL-101

- **Severity:** Medium
- **Category:** consistency
- **Failure scenario:** WLD-102 says that at the session pace "a warning that B6 converts by PIL-103 (50 seconds) is shorter than the 52 seconds of travel, and the attack needs no hold". PIL-101 runs travel at the tick rate at both paces and asks A1 to "size travel so that a session-paced world still meets PIL-025", which only makes sense if the 10-minute warning stays 10 minutes. A solo steward, whose integrated world runs at the session pace by default (Table B of A0), would get 50 seconds between an AI attack's warning and its arrival, at the very pace in which the steward is always present.
- **Recommendation:** Keep the warning at 10 minutes of world time at both paces (a battle-scale target under PIL-103, like the battle length), with the 9.1-minute hold at the session pace too, or ask A0 and B6 for a ruling before Finalise and state both outcomes in WLD-102 until then.

### A1-F12 — The watched-settlement bound ignores expedition views

- **Severity:** Medium
- **Category:** scale and cost
- **Failure scenario:** WLD-244 lets the settlement view show "the area that the settlement's own expeditions see" and then states that a present steward makes watched "at most 6" settlements outside battles. A steward who sends small expeditions next to rival territories (gathering trips, reinforcements) makes each of them watched, so the bound does not hold, and a steward can force full fidelity on settlements across the world. PIL-057 removes the gain but not the CPU cost. In C6 the union at 200 at full occupancy is 118 typical and 162 at most (the model's 140 and 258 add overlaps and are not a union), and one expedition view per present steward raises it to 178, three to 203.
- **Recommendation:** Decide that an expedition's view never makes a settlement watched (it shows the strategic summary and battles only), or count it within a cap of `wld.scale.watched_max_per_present`; compute the union in the model and hand A2 the union, not the sum.

### A1-F13 — Stewards are seated among AI and rarely border each other

- **Severity:** Medium
- **Category:** player experience
- **Failure scenario:** The first audience is a group of 2 to 16 friends (Table B of A0), who want "battles they can fight together or against each other". WLD-121 puts an AI settlement in every valley first, and WLD-140's score counts an AI neighbour like a steward one, so each newcomer takes the site between AI settlements. In C10, with 4 or 8 stewards in a world of 16, 100 % of the stewards have no steward neighbour; with all 16, 44 %; in a world of 200 with 50 stewards, 100 %. Even weighting a steward valley neighbour at 5 against an AI's 2 leaves 75 % alone at 8 of 16, because the AI-first placement has already taken the sites between stewards. Friends never share a border area, so they never meet in territory.
- **Recommendation:** Add founding beside a chosen steward: the founding choice lists the free sites next to a named steward or a Cladan member first, and a group can reserve neighbouring sites together (C2 specifies the page). Leave some valleys AI-free for steward groups in worlds of up to 16, and rank a steward neighbour above an AI neighbour by default.

### A1-F14 — The travel limits pin B4's expedition speed to at least 18.8 wu/s

- **Severity:** Medium
- **Category:** consistency
- **Failure scenario:** WLD-101's limits hold at 20 wu/s, the Worker's speed. The formation speed of the Leader is 12 wu/s (R-UNIT-156), and pillar 1 steers crowds through Leaders and formations (PIL-010). In C1 each limit needs at least 15.9 (valley), 15.2 (pass), 18.2 (hop), 18.3 (diameter at 16) and 18.8 wu/s (diameter at 200); at 12 or 15 wu/s every limit fails, and the attack hold, the gathering trip of WLD-103 and the expedition load of Table 9 all change. WLD-104 leaves B4 free to choose, but the geometry silently decides for it.
- **Recommendation:** State the world's travel speed as a world rule now (expeditions outside battle move at `wld.travel.march_speed`, 20 wu/s, whatever their formation), or derive the region side from B4's speed so that the limits hold by construction, and record the dependency as a cross-area request to B4.

### A1-F15 — A world's geometry is not protected against patches

- **Severity:** Medium
- **Category:** operability
- **Failure scenario:** Seed challenge 11: a world must survive game updates. Most of A1's values are provisional and will be retuned (site spacing, radii, clearances, path factors), and a world template is a versioned asset (WLD-167). A patch that retunes `wld.territory.max_radius` changes the lens of every running world if the value is read live; a patch that changes the generator or the template changes the regions that a later raise adds (WLD-008), and WLD-161's "the same world on any server" no longer holds across versions. WLD-166 keeps the baked chunks, but not the layout values or the generator version that produced them.
- **Recommendation:** A world records at creation its generator version, its template digest and a snapshot of every `wld.*` value that shapes its layout (the world constants), and reads them, never the live tuning, for the life of the world; an extension uses the recorded versions, so the server keeps every template version a running world names. D4 specifies the migration of a world whose constants a later patch must change.

### A1-F16 — Passes and ridges are not shown to be buildable with the reference terrain primitives

- **Severity:** Medium
- **Category:** technical feasibility
- **Failure scenario:** A flatten area is an axis-aligned, hard-edged square with no falloff (TER-140), and a tile can only raise terrain (TER-003). A pass floor must keep a normal y of at least 0.9 along its whole centre line (WLD-024), which allows a rise of at most about 7.7 wu per 16 wu cell, between valleys whose site plateaus sit anywhere from 20 to 120 wu (WLD-041) and whose base tiles are fractal. An Absolute flatten square at one height leaves a step at each pass mouth unless the floor heights already match; no reference operation builds a ramp. The 8 regenerations of WLD-163 change the random draws, not the structure, so a structural mismatch fails 8 times and stops the world's creation. The generation budgets (20 s and 120 s) and the failure rate are first guesses, although the conformance generator `Tools/DesignModels/TerrainGenerator.py` exists and could measure them now.
- **Recommendation:** Prototype one region with `TerrainGenerator.py` before Finalise and report its failure rate over 100 seeds; give the generator a ramp primitive (a flatten type 6 that interpolates between two heights across its square, added beside the unbiased types of WLD-268) or one shared valley-floor height for every region; and count the flatten budget per region (4 cores, 1 centre and each region's half of its passes: 9 of the 12 allowed).

### A1-F17 — The smallest world per stage is not stated

- **Severity:** Medium
- **Category:** scope
- **Failure scenario:** PIL-110 and Table 10 of A0 ask each stage for its smallest world game, and stage 1 for "the regions that A1 sizes for up to 16 stewards". The document lists launch counts (3 templates, 6 biomes, 6 lighting presets, 8 set pieces) and one first-milestone count (1 template), but not what stage 1 builds of the generator's validation, biomes, presets, set pieces, the strategic view's level of detail, capacity raises or the archive. A small team builds the full list for the first world-game milestone, or cuts it without a rule.
- **Recommendation:** Add a table of the smallest world per stage: stage 1, one template, 2 biomes, 2 lighting presets, no set pieces, no capacity raise, the unlimited lifetime, the validation checks of WLD-163 that guard reachability, plateaus and passes; stage 2, raises, lifetimes, the archive and 3 templates; stage 3, worlds of 200 from a hosted template set.

### A1-F18 — Statements about AI placement and rounding that the model contradicts

- **Severity:** Low
- **Category:** consistency
- **Failure scenario:** WLD-121 says that with at least as many AI settlements as regions "every valley has a settlement from day 1 and every free site has an AI settlement in its valley"; Table 1 and §1.2 read as if this held at 200, where 58 AI settlements meet 65 regions and 7 valleys start empty; it fails at 168 of the 256 capacities, the first being 74 (C9). WLD-004 says "round" without a rule for halves; Python's ties-to-even and rounding half up give different region counts at 16 capacities (10, 26, 42 and every 16th to 250), which breaks WLD-161's same world on every server.
- **Recommendation:** Say from which capacity some valleys start empty, and specify rounding half up (or floor of x + 0.5) in WLD-004 and the model.

### A1-F19 — The home resource sites cannot be spread evenly as WLD-082 states

- **Severity:** Low
- **Category:** consistency
- **Failure scenario:** WLD-082 spreads 3 inner sites 120° apart and 5 outer sites 72° apart, at least 200 wu from the border areas. At the nominal spacing the outer band (600 to 1,100 wu) runs into both lenses, leaving a free arc of 171 to 207°, so the 5 outer directions (spanning 288°) cannot all fit; at the nearest jitter the inner band's free arc is 159° for three directions spanning 240°. The turning rule then packs them toward the outward corner, so gathering distances differ by site, and "spread evenly" describes no real site.
- **Recommendation:** Place the home resource sites directly in the outward sector of the home cell, at fixed angles and radii relative to the site's corner, so that every site gets the same pattern and the same gathering distances.

### A1-F20 — Small contradictions in the player-facing promises and settings

- **Severity:** Low
- **Category:** consistency
- **Failure scenario:** Table 2 promises on days 1 to 5 "the valley's other settlements visible across open ground", but WLD-244 bounds the settlement view to 800 wu beyond the territory: 1,541 wu on day 1 and 1,838 wu on day 5, short of the neighbour's core at 1,680 wu until about day 3 and of its Wellspring at 2,080 wu at any time unless the steward holds the lens. Table 4 offers the session pace to every world, while PIL-104 makes a dedicated server's world always-on. WLD-220 gives no default lifetime for a hosted world of stage 3. WLD-246 states a screen-space error in world units. The rule that a steward holds one reservation at a time lives only in Table 10, not in WLD-141.
- **Recommendation:** Reword Table 2 to the strategic view, restrict Table 4's pace to integrated worlds, give the hosted default, state WLD-246's precision as a world-space error at the far corner, and move the one-reservation rule into WLD-141.

### A1-F21 — A host who lowers the AI count breaks WLD-065's band

- **Severity:** Low
- **Category:** consistency
- **Failure scenario:** WLD-120 lets the world host lower the AI count to 2. At 16 with 2 AI settlements, the mean contacts at full occupancy are 2.33 (C9), below WLD-065's "between 2.5 and 3.0", which is a MUST with no condition on the AI count; with 6 AI settlements, 2.45.
- **Recommendation:** State WLD-065's band at the default AI count, and show the host the contact figure for a lowered count on the world-creation page.

### A1-F22 — The density claims have no test on a generated world, and the ledger treats censuses two ways

- **Severity:** Low
- **Category:** testability
- **Failure scenario:** AC-11 checks that the model reproduces the document, which the model's own `check` guarantees, so it tests neither the game nor the model's assumptions; no acceptance criterion founds stewards on a generated world in the server and counts contacts, border-area holders or first contact days. In the ledger, R-MAP-086 is `adopt` although it carries a census of reference-map usage (how many of the 49 match maps use each colour asset), while the censuses of R-MAP-110, R-MAP-191, R-MAP-192 and R-MAP-200 are `adapt` to PIL-003.
- **Recommendation:** Add a server-side test that founds 16 and 200 stewards by WLD-140 on generated worlds of 10 seeds and checks contacts, lens holders and first contact days against the model's ranges; make R-MAP-086 `adapt` to WLD-180, keeping its asset counts and the seasonal replacement and dropping the census.

## Make it better

### A1-M1 — Contested lenses instead of first-come lenses

Split every border area at its halfway line (or ridge line) by default and make its resource site the claim object. **Benefit:** arrival order stops mattering, so AI settlements, the launch cohort and late joiners start equal (A1-F01); every gain of land is a claim with a cause on record (pillar 3) that the loser can reverse (pillar 6); WLD-063, WLD-064 and AC-10 become simpler. **Cost:** B1 grows territory to a line instead of racing to it, B5 owns every change of holder; one rule and one acceptance criterion change in A1.

### A1-M2 — Compact AI placement for worlds where AI settlements are fewer than twice the regions

Place AI settlements in diagonal pairs in the central regions, leaving outer valleys empty for later stewards. **Benefit:** the proposer's own `ai` mode shows, at 200 and 25 % occupancy, 2.70 mean contacts against 1.85, 6 % of settlements below 2 contacts against 13 %, and a steward mean of 2.92 against 2.00; at 16 the two rules are equal. **Cost:** the strategic view no longer shows a settlement in every valley at day 1, and stewards found further from the centre once the AI valleys fill.

### A1-M3 — A density keeper over the world's life

AI settlements found at free sites in valleys below a target and step back when a full world needs the site; founding opens regions from the centre outwards. **Benefit:** the contact band of WLD-065 holds in month 6 as in week 2 (A1-F06), and a late joiner always finds neighbours. **Cost:** B7's AI budget grows with empty valleys, and AI settlements gain a dormancy rule; the AI share becomes a tuning value instead of a fixed count.

### A1-M4 — Water channels as the region borders, fords as passes

Separate the valleys with narrow channels of sea and cross them with land bridges. **Benefit:** the barrier comes free from the adopted path tests (water blocks `IsPathClear` and `WalkCost`, so no route and no direct-route override crosses it), fixing A1-F03 without a new movement rule; the black-lit sea between glowing valleys reads clearly in the strategic view; sea chunks cost one value each. **Cost:** Workers die in deep water (R-SIM-377), so every order path must keep the existing water test; if B4 keeps Transports in the world, they cross channels, and the channels' width must exceed their reach or the ford rule must cover them.

### A1-M5 — Founding as a group

A group of stewards reserves neighbouring free sites together, or a steward founds beside a named steward. **Benefit:** the first audience (2 to 16 friends) shares border areas and passes instead of being spread among AI (A1-F13), and reinforcing a friend is a 2-minute march. **Cost:** a C2 page and a group reservation of up to `wld.founding.group_max` sites under WLD-141's timer.

### A1-M6 — Eras for unlimited worlds

Every `wld.era.weeks` world weeks of an unlimited world (for example 12), claims are settled as they stand, every border area returns to its split, renown is banked, and settlements keep their Workers, stores and structures. **Benefit:** a soft reset that cures a runaway leader's land without erasing investment, the lesson of Table D of A0, available to the integrated worlds whose lifetime is unlimited by default and can never become limited (WLD-221). **Cost:** B5 and B2 define the era's end; stage 2 with renown.

### A1-M7 — A warning radius instead of an arrival estimate

The server warns when an attacking expedition first enters a radius of (highest possible speed × warning time) around the target's territory, and holds it at the territory edge until the warning time has run (A1-F04). **Benefit:** one geometric test, immune to route choice, speed effects and the travel network's errors, and cheaper than re-estimating every expedition every tick. **Cost:** a distant fast expedition may wait longer at the edge; the radius is one tuning value.

### A1-M8 — Put time into the world-density model

Add a `lifetime` mode to `A1WorldDensity.py`: arrivals, an active-lifetime distribution, dormancy, held places and border-area holders, as the challenge model's C4 does. **Benefit:** B6 (dormancy, newcomer protection), C2 (founding and returns) and A2 (load in month 6) tune against a world that ages; the figures of §1.3 gain columns for months 1, 3 and 6. **Cost:** about 200 lines of the standard library; seeds and scenario inputs become documented placeholders.

## Summary

22 findings: 0 Critical, 5 High (A1-F01 to A1-F05), 12 Medium (A1-F06 to A1-F17), 5 Low (A1-F18 to A1-F22); 8 "make it better" items (A1-M1 to A1-M8).
