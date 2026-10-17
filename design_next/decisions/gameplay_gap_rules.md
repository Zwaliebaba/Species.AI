# Gap rules: gameplay

> Status: Reference decisions · Range: DEC-850 to DEC-899 · Related: [decisions index](README.md), [verdicts and fixes](verdicts_and_fixes.md), [conventions](../CONVENTIONS.md)

This file records the gameplay rules that the reference documents had to choose because the reference does not define them, and the small reference quirks of the gameplay documents that need a decision of their own. Related rules of one document or mechanic are grouped into one entry with numbered items. Each entry states the rule that the documents specify now. An **Open** entry states that rule as the default, with options and a recommendation; a **Resolved** entry states a rule that applies a Resolved decision and needs no further choice; a **Dropped** entry records content that the library does not specify. The general decisions for these areas stay in [verdicts and fixes](verdicts_and_fixes.md) and the other register files, and the entries here cite them by number.

## Index

| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-850](#dec-850-match-rule-gap-rules) | Match-rule gap rules | Open | R-MODE |
| [DEC-851](#dec-851-population-flow-rule-details) | Population-flow rule details | Open | R-ECON |
| [DEC-852](#dec-852-game-speed-divisors-in-worker-rules) | Game-speed divisors in Worker rules | Open | R-UNIT |
| [DEC-853](#dec-853-worker-and-trooper-rule-details) | Worker and Trooper rule details | Open | R-UNIT |
| [DEC-854](#dec-854-transport-collector-and-strike-team-rule-details) | Transport, Collector and Strike team rule details | Open | R-UNIT |
| [DEC-855](#dec-855-creature-and-hazard-rule-quirks) | Creature and hazard rule quirks | Open | R-UNIT |
| [DEC-856](#dec-856-creature-presentation-quirks) | Creature presentation quirks | Open | R-UNIT |
| [DEC-857](#dec-857-attempt-caps-of-the-bounded-searches) | Attempt caps of the bounded searches | Resolved | R-UNIT, R-BOT |
| [DEC-858](#dec-858-empty-outcome-lists) | Empty outcome lists | Open | R-CRATE, R-ECON |
| [DEC-859](#dec-859-tick-counts-chosen-for-reference-durations) | Tick counts chosen for reference durations | Open | R-CRATE, R-BLDG, R-MODE |
| [DEC-860](#dec-860-structure-rule-details) | Structure rule details | Open | R-BLDG |
| [DEC-861](#dec-861-bot-rule-quirks) | Bot rule quirks | Open | R-BOT |
| [DEC-862](#dec-862-bot-rule-details) | Bot rule details | Open | R-BOT |
| [DEC-863](#dec-863-control-details-of-the-corrected-command-set) | Control details of the corrected command set | Resolved | R-CTRL |
| [DEC-864](#dec-864-pointer-fallback-and-camera-smoothing) | Pointer fallback and camera smoothing | Open | R-CTRL |
| [DEC-865](#dec-865-map-data-without-a-reference-case) | Map data without a reference case | Open | R-MAP |
| [DEC-866](#dec-866-population-cap-values-of-a-map) | Population-cap values of a map | Open | R-MAP |
| [DEC-867](#dec-867-closed-player-slots) | Closed player slots | Open | R-META, R-MAP |
| [DEC-868](#dec-868-front-end-reference-quirks) | Front-end reference quirks | Open | R-META, R-MAP |
| [DEC-869](#dec-869-reference-sound-options-without-a-species-counterpart) | Reference sound options without a Species counterpart | Open | R-META |
| [DEC-870](#dec-870-tutorial-director-details) | Tutorial director details | Open | R-TUT |
| [DEC-871](#dec-871-essence-orb-break-operation) | Essence orb break operation | Dropped | R-UNIT, VFX |

## 1 Match rules and population flow

### DEC-850 Match-rule gap rules
- Status: Open
- Affects: reference/01_skirmish_rules_and_modes/relic_run.md (R-MODE-306), reference/01_skirmish_rules_and_modes/flag_chain.md (R-MODE-462)
- Specified behaviour: The match rules fix two points that the reference leaves implicit:
  1. **Relic spawn window.** The window 60 × S ÷ Z mode seconds, with S the number of Relics and Z the number of Relic Spawn Zones, and its half, window ÷ 2, are exact real-valued quotients, not integer divisions (R-MODE-306). The two readings differ only when Z does not divide 60 × S or when the window is odd.
  2. **Home post under attack.** The live under-attack status that DEC-078 requires is set for a Home post that its team owns while enemy Workers stand within its radius at the latest recount, which is the condition of the base-under-attack announcement (DEC-071). The statuses of a Home post rank, from highest to lowest: safe, under threat, unoccupied, under attack, enemy occupied (R-MODE-462).
- Options: A — the rules as stated; B — integer division in whole mode seconds, rounded down (item 1), and an under-attack status taken from damage dealt to the Home post since the latest recount (item 2).
- Recommendation: A. Exact division keeps the spawn rhythm even for every Relic and zone count, and the announcement condition gives the status and the announcement one meaning.
- Decide by: design phase

### DEC-851 Population-flow rule details
- Status: Open
- Affects: reference/05_population_flow.md (R-ECON-064, R-ECON-104)
- Specified behaviour:
  1. **Flag Chain airstrike wave.** On a turret wave a team receives the airstrike power instead of the gun turret when it holds at least (flag count + 1) ÷ 2 Flag posts, or, when the map sets an airstrike threshold, more than that many Flag posts; either condition qualifies (R-ECON-104).
  2. **Population Limiter overpopulation.** The summed excess of team slot 0 is halved by integer division (R-ECON-064). The rule is inert on every reference map (DEC-454).
- Options: A — the rules as stated; B — a map's airstrike threshold, when set, replaces the half-flags test (item 1), and the halving keeps the fraction (item 2).
- Recommendation: A. Under B for item 1 a threshold could also raise the requirement, which no reference map needs; check the reading again when the first Flag Chain maps that set a threshold are playtested.
- Decide by: playtest

## 2 Units, creatures and hazards

### DEC-852 Game-speed divisors in Worker rules
- Status: Open
- Affects: reference/02_units/README.md (R-UNIT-007), reference/02_units/worker.md (R-UNIT-079, R-UNIT-082, R-UNIT-103)
- Specified behaviour: Three Worker rules keep the reference formulas that read the game speed s of the tick clock: the fire roll outside a formation, `NextUint` mod ⌊a ÷ s⌋ with a = 10, or 7 with a Shield, or 3 for a Visitor faction Worker (R-UNIT-103); the burning roll, `NextUnit(10)` < 2.0 ÷ s (R-UNIT-079); and the carry lift radius, 20 wu × s × the speed ratio (R-UNIT-082). Game speed is tick scheduling (DEC-039), so every tick is a normal tick and a game second always holds 10 ticks. Keeping the division by s therefore makes a Worker's attack attempts per game second proportional to s and its burning damage per game second proportional to 1 ÷ s: at speed 2 a Worker makes about 2 attempts and loses about 2 health per game second, against 1 attempt and 4 health at speed 1. No division is evaluated at s = 0 (DEC-653).
- Options: A — keep the formulas; B — drop the division by s, so that every speed gives about 1 attempt and 4 health per game second and a lift radius of 20 wu; C — rescale each formula to reproduce the reference rates at every speed: about 1 attempt per game second, and 4 ÷ s² health per game second.
- Recommendation: B. Every other rule sees the same tick arithmetic at every speed (DEC-039, DEC-092); B keeps the reference attack rate, and with A a Time Rush makes Workers fight harder and burn more slowly. Choose C only if the reference burning rate under Time Dilation and Time Rush is wanted exactly.
- Decide by: design phase

### DEC-853 Worker and Trooper rule details
- Status: Open
- Affects: reference/02_units/worker.md (R-UNIT-098, R-UNIT-119), reference/02_units/README.md (R-UNIT-014), reference/02_units/strike_team.md (R-UNIT-341)
- Specified behaviour:
  1. **Fire-damage kills.** Once a Worker's fire damage reaches 100 it is set on fire and the attacking team's kill counter rises by 1, and the counter rises again on every later fire-damage call to the same Worker, so one Worker can count as several kills (R-UNIT-119).
  2. **Hive and Spitter Plant check.** The enemy Hive or Spitter Plant that step 5 of the Worker's threat search attacks is one in the Worker's obstacle cell within the Worker's current threat range (R-UNIT-098).
  3. **Trooper reload.** A Trooper's own reload interval is 0 ticks, so its own reload never blocks a shot, while the Strike team's fire accumulator reads the Trooper's table rate of 0.5 (R-UNIT-014, R-UNIT-341). Both values are kept explicitly under floating-point stats (DEC-090), so the Strike team fires at the reference rate.
- Options: A — the rules as stated; B — a Worker set on fire counts as one kill (item 1), and the table rate also becomes each Trooper's own reload of 0.5 s (item 3).
- Recommendation: A for items 2 and 3, which reproduce the reference fire pattern; B for item 1, because one burning Worker is one kill and a repeated count inflates the kill totals that the statistics and achievement hooks read (DEC-047).
- Decide by: playtest

### DEC-854 Transport, Collector and Strike team rule details
- Status: Open
- Affects: reference/02_units/transport.md (R-UNIT-379, R-UNIT-406), reference/02_units/collector.md (R-UNIT-431, R-UNIT-445), reference/02_units/strike_team.md (R-UNIT-358)
- Specified behaviour:
  1. **Transport braking.** Within 10 wu of its waypoint a Transport's speed falls by 0.5 wu/s every tick, with no floor (R-UNIT-379).
  2. **Collector suck speed.** A harvested Essence orb moves at the level speed of 20 wu/s once its height differs from the collector point's by less than 5 wu, the height test of the collection rule (R-UNIT-431).
  3. **Collector status text.** A Collector shows its idle text while Idle, while Harvesting and while delivering (R-UNIT-445).
  4. **Strike team direct control.** The waypoint moves from the lead Trooper's position by the world-space movement vector, and an attack is aimed along the world-space aiming vector, taken as the point (x, 0, z); the sender converts and scales both vectors before sending (R-UNIT-358, DEC-008).
  5. **Flag Chain bot Transport.** A Transport of a standard or hard bot rushes the last enemy gun turret found within 1.2 × the chosen Flag post's size, which is 200 wu for a Flag post of default size (R-UNIT-406).
- Options: A — the rules as stated; B — the braking stops at a speed of 0 (item 1), and Harvesting and delivering show their own status texts (item 3).
- Recommendation: B for items 1 and 3, which have no play purpose; A for the others. The vector scaling of item 4 is confirmed against the input mapping of reference/06 and the input component.
- Decide by: design phase

### DEC-855 Creature and hazard rule quirks
- Status: Open
- Affects: reference/02_units/creatures.md (R-UNIT-520, R-UNIT-532, R-UNIT-576), reference/02_units/hazards_and_projectiles.md (R-UNIT-705, R-UNIT-723, R-UNIT-726)
- Specified behaviour: These creature and hazard rules are kept as the reference states them (DEC-006):
  1. **Crawler bite.** A Crawler in contact with its target bites for 20 health on every tick while no other enemy lies between 30 wu and its search range, so it deals 200 health per second (R-UNIT-532).
  2. **Spider avoidance.** For each spider found within 22 wu, the escape vector adds the offset from the neighbour at index `NextUint` mod n of the query result, not from the spider found (R-UNIT-576).
  3. **Abductor step.** The Abductor's position advances by speed × dt × dt per tick, so its effective velocity is a tenth of its speed value of 244 (R-UNIT-705, DEC-090).
  4. **Barrage kills.** Every actor that a barrage missile kills, of any team and the launching team's own included, adds to the launching team's kill counter (R-UNIT-726).
  5. **Barrage missile death.** A barrage missile never checks its own dead flag: at 0 health it releases its Essence orb, keeps flying and still explodes (R-UNIT-723).
  6. **Reaper Serpent fallback.** A Seed Pod of kind 8 that cannot release a Reaper Serpent releases kind 0 instead, with the kind-0 roam range of 100 wu (R-UNIT-520).
- Options: A — keep each rule; B — a bite cooldown (item 1); avoidance from the spider found (item 2); a single time step with the speed value re-tuned to the same effective motion (item 3); enemy kills only (item 4); a missile at 0 health is removed (item 5); a roam range of 200 wu (item 6).
- Recommendation: A for items 1, 3 and 6, whose values are the tuned behaviour; the bite cadence is design-critical and is confirmed by playtest. B for items 2, 4 and 5, which have no visible purpose.
- Decide by: playtest

### DEC-856 Creature presentation quirks
- Status: Open
- Affects: reference/02_units/hazards_and_projectiles.md (R-UNIT-718), reference/02_units/creatures.md (R-UNIT-528, R-UNIT-547, R-UNIT-593)
- Specified behaviour: These presentation rules are kept as the reference states them:
  1. the Bomber's shatter fraction is computed against 255 health, whatever its health stat (R-UNIT-718);
  2. every hit on a Reaper Serpent shatters every tail segment (R-UNIT-593);
  3. Crawler eggs are drawn white, whatever their team (R-UNIT-528);
  4. all Brood Drifters share one smoothed tail velocity (R-UNIT-547).
- Options: A — keep the reference look; B — the Bomber's fraction against its own health stat, only the segment hit shatters, eggs in their team colour, and one tail velocity per Brood Drifter.
- Recommendation: A until the new art is reviewed; B for item 4, because a shared tail velocity makes every drifter's tail follow the motion of the others.
- Decide by: design phase

## 3 Supply crates, searches and timing

### DEC-857 Attempt caps of the bounded searches
- Status: Resolved
- Affects: reference/02_units/hazards_and_projectiles.md (R-UNIT-702, R-UNIT-706, R-UNIT-722), reference/07_bots/04_powers_and_unit_policies.md (R-BOT-105, R-BOT-117)
- Specified behaviour: The searches that DEC-014 bounds use these caps, each followed by the deterministic fallback that its rule states:
  1. the Abductor's arrival search and the Missile Barrage sea search: 256 attempts (`ref.unit.abductor.arrival_attempts`, `ref.hazard.missile_barrage.sea_attempts`), then the cell fallback of their rules (R-UNIT-702, R-UNIT-722);
  2. the Abductor's wander search: 2000 attempts (`ref.unit.abductor.wander_attempts`), then the cell fallback (R-UNIT-706);
  3. every placement loop of a bot Power handler: 100 drawn points (`ref.bot.power.placement_attempts`); with no valid point the handler places nothing in that cycle and the Power stays Waiting (R-BOT-105, R-BOT-117).

  The caps of items 1 and 2 are sized from the lowest acceptance rates that DEC-014 states, 6.3 % of landing draws and 0.76 % of wander draws, so that a search falls back less than once in a million searches; the cap of item 3 equals the cap of the reference's own random link selection.
- Options: A — these caps and fallbacks; B — larger caps.
- Recommendation: Resolved: the caps apply DEC-014. They leave the outcome distribution of every valid map unchanged for practical purposes and bound the work of each search.

### DEC-858 Empty outcome lists
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/README.md (R-CRATE-003), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE-121, R-CRATE-124, R-CRATE-126), reference/05_population_flow.md (R-ECON-110)
- Specified behaviour: Map exclusions (DEC-107) and validity rules can leave an outcome draw with no candidate. The draw then grants nothing (R-CRATE-003):
  1. a Supply crate whose beneficial branch finds no valid outcome that the map allows opens and delivers nothing (R-CRATE-121, R-CRATE-124);
  2. a Supply crate whose hostile branch finds every hostile outcome excluded opens and delivers nothing (R-CRATE-126);
  3. a starting-power draw with no qualifying outcome grants no power (R-ECON-110).

  In each case the crate is consumed and every earlier draw of the rule is made as usual.
- Options: A — an empty draw grants nothing; B — an empty hostile branch falls through to the beneficial branch, so a map that excludes every hostile outcome has crates without hazards; C — the content build rejects a map whose exclusions empty a candidate list.
- Recommendation: B for item 2, which gives an exclusion of every hostile outcome the meaning a map author expects; A for items 1 and 3, as the run-time fallback of DEC-014.
- Decide by: design phase

### DEC-859 Tick counts chosen for reference durations
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE-115, R-CRATE-119), reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md (R-CRATE-470), reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG-415), reference/01_skirmish_rules_and_modes/siege.md (R-MODE-362, R-MODE-372)
- Specified behaviour: Where the reference gives a duration without the arithmetic of its countdown, the documents fix these tick counts (DEC-002; the countdown conversion of the numeric model is DEC-241 item 3):
  1. **Supply crate opening.** Progress is an integer number of units, one per counted Worker per tick and at most 50 per tick, and a crate opens on the first tick on which its progress exceeds 2500 units: after ⌊2500 ÷ min(n, 50)⌋ + 1 ticks of an unbroken majority of n Workers, which is 51 ticks at 50 or more. For 4 and 20 Workers this gives 626 and 126 ticks, one tick later than a floating-point accumulation of the same rate (R-CRATE-115).
  2. **Neutral crate life.** A Supply crate on the ground with no owner expires after 600 ticks, its nominal 60 s, the count of a 60 s countdown kept in double precision (R-CRATE-119). A single-precision countdown of 60 s gives 601 ticks, the reference count of a Strike team's life (R-UNIT-338).
  3. **Shockwave ring.** A ring of size s seconds is active for A = ⌈10 × s⌉ − 1 ticks (R-CRATE-470). For a size that is a whole number of tenths, such as the 1.0 s ring of a Transport's death explosion, this gives 9 ticks where a double countdown gives 10.
  4. **Turret hosing.** A turret keeps running its fire step at its last aim point for 10 ticks after the last tick with a target (R-BLDG-415).
  5. **Multi-round Siege.** The fade-out starts at 10 ticks of victory delay left, and the next-round countdown shows below 110 ticks of delay left (R-MODE-362, R-MODE-372); a single-round Siege reaches neither (DEC-066).
- Options: A — the tick counts as stated; B — derive each count again from a replay of the countdown arithmetic of DEC-241 item 3.
- Recommendation: A. Each difference from another reading is at most one tick, and every count is a named tuning key that a playtest can change.
- Decide by: design phase

## 4 Structures

### DEC-860 Structure rule details
- Status: Open
- Affects: reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG-275, R-BLDG-324, R-BLDG-325), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG-149), reference/03_buildings_and_world_objects/README.md (R-BLDG-046), architecture/components/renderer/world_renderers.md (RND-170)
- Specified behaviour:
  1. **Carryable ownership tie.** In the ownership recount of a carryable structure, a later team in slot order replaces the candidate only with a strictly higher Worker count, so a tie goes to the lowest team slot (R-BLDG-275, DEC-016).
  2. **Launch Vessel damage floor.** A Launch Vessel's damage that decays to 0 or below is set to 0, and its attack-warning flag is cleared (R-BLDG-324).
  3. **Launch Vessel attackers.** The k attacker picks among the n enemies found are independent uniform draws, so one actor can be picked more than once (R-BLDG-325).
  4. **Teleport exit jitter.** The single jitter draw, U[−17.5, 17.5) wu, offsets the exit waypoint across the exit direction, in the horizontal plane (R-BLDG-149).
  5. **Turret port look.** The reference anchors a turret's port status at the status sockets `status_01` to `status_04` of its mount (R-BLDG-405) and records no colours or pad use for it. A turret draws its ports with the base port look of R-BLDG-046: the pad while the structure-detail setting is below `ref.bldg.port.pad_detail_limit`, and a status star that is green when the port is occupied and red when it is empty, with the star of port i (1 to 4) at the mount's socket `status_0i` instead of the pad's `status` socket.
- Options: A — the rules as stated; B — distinct picks (item 3), independent draws on both horizontal axes (item 4), and status stars without pads on a turret (item 5).
- Recommendation: A. Each rule follows the stated arithmetic with the fewest draws, and a repeated pick only orders one actor twice. For item 5 the re-anchored base look changes only the anchor that the turret's mount defines, and it matches the green and red status lights of the Solar Array (R-BLDG-183).
- Decide by: design phase

## 5 Bots

### DEC-861 Bot rule quirks
- Status: Open
- Affects: reference/07_bots/05_mode_strategies_and_tutorials.md (R-BOT-129), reference/07_bots/01_navigation_graph.md (R-BOT-036, R-BOT-043)
- Specified behaviour: These bot rules are kept as the reference states them (DEC-006):
  1. **Defence flags.** The single pass over the bot objectives moves a defence flag one step down a chain per objective visited, so a defence chain longer than two is marked to its end only when its objectives come in chain order in structure order (R-BOT-129).
  2. **Tagged zones.** A bot navigation node keeps the Control Zone or Relic Spawn Zone that tagged it after the zone has been captured; no rebuild clears the tag (R-BOT-036).
  3. **Nearest node of a Worker.** Nothing clears a Worker's nearest node, so a Worker can be dispatched only from a node that once was nearest to it (R-BOT-043).
- Options: A — keep each rule; B — propagate defence flags to the end of every chain (item 1), tag nodes again at every rebuild (item 2), and recompute a Worker's nearest node at every recount (item 3).
- Recommendation: decide with the calibration of bot strength (DEC-112): B for item 1, so that every defended chain is defended to its end; A for items 2 and 3 until a playtest shows that stale tags or nearest nodes misdirect bots.
- Decide by: playtest

### DEC-862 Bot rule details
- Status: Open
- Affects: reference/07_bots/README.md (R-BOT-008), reference/07_bots/02_priority_model.md (R-BOT-060, R-BOT-070), reference/07_bots/03_decision_loop_and_leaders.md (R-BOT-088), reference/07_bots/04_powers_and_unit_policies.md (R-BOT-107)
- Specified behaviour:
  1. **Bot controller position.** The bot controller stands at its team's start position, or at the map centre for the Rogue and Visitor factions and for a team whose slot has no start position; its placement makes no search and no draw (R-BOT-008, DEC-087).
  2. **Promotion Power expiry.** A promotion Power that has not promoted a Worker by the end of the bot step that started it ends then and frees its slot (R-BOT-107, DEC-087).
  3. **No usable link.** In the standard priority of a node that the bot's team or an ally owns, a node with no usable link has a neighbour mean of 0, and the Beam Dish, link and modifier bonuses still apply (R-BOT-060, DEC-653 item 6).
  4. **Shared pins.** When several pin writers share one nearest node, the writer that advances last in structure order decides the node's priority and flag (R-BOT-070, DEC-016).
  5. **Stored crossings.** Because crossings are stored whatever a Barrier Emitter's state (DEC-087), the random link selection draws again on a link that crosses an emitter of any other team, switched on or off (R-BOT-088).
- Options: A — the rules as stated; B — a bot controller with no spatial body (DEC-311 option C) for item 1, the node keeps its previous priority (item 3), the highest pin wins (item 4), and the random selection draws again only on a link that crosses a switched-on emitter of another team (item 5).
- Recommendation: A. The rules complete the corrections of DEC-087 with the stable orders of DEC-016; confirm items 1 and 5 when bot strength is calibrated (DEC-112).
- Decide by: playtest

## 6 Controls and camera

### DEC-863 Control details of the corrected command set
- Status: Resolved
- Affects: reference/06_controls_camera_and_commands/actions_and_bindings.md (R-CTRL-044, R-CTRL-045, R-CTRL-050), reference/06_controls_camera_and_commands/selection_and_power_slots.md (R-CTRL-226, R-CTRL-230, R-CTRL-232)
- Specified behaviour: These details complete the corrected controls of DEC-086, DEC-083 and DEC-023:
  1. **Demote and scrap.** `action.order.demote` and `action.order.scrap` share the cancel binding (the C key, or the north face button): with a Leader selected the press demotes it; with nothing selected, over an own ungrouped actor that is neither a Worker nor a Leader, it scraps that actor (R-CTRL-044 rows T4 and T5, R-CTRL-045).
  2. **Clickable power bar.** The bar slot under the pointer takes the focus; `action.power.activate_slot` (primary button up, Enter, or the south face button up) selects its Power; a primary press that starts over the bar acts on the bar and is withheld from the world actions for the rest of the frame; empty slots are drawn with `icon.hud.slot_empty` (R-CTRL-226, R-CTRL-230).
  3. **Secondary-button order.** The meanings of the secondary button are tried in this order: placing error; toggle the unit under the pointer; order the selected Leader or Transport; crowd order; promote. Only the second and the third overlap, and the toggle wins (R-CTRL-050).
  4. **Lock countdown.** Both passes of a slot's lock countdown, the outline pass and the fill pass, draw the countdown text (R-CTRL-232).
- Options: A — these details; B — leave the new actions unbound until rebinding is designed (DEC-050).
- Recommendation: Resolved: each detail applies a Resolved decision with the reference's own bindings, and item 4 corrects a render defect in which the outline pass draws a malformed text.

### DEC-864 Pointer fallback and camera smoothing
- Status: Open
- Affects: reference/06_controls_camera_and_commands/actions_and_bindings.md (R-CTRL-058), reference/06_controls_camera_and_commands/camera.md (R-CTRL-454)
- Specified behaviour:
  1. **Pointer fallback.** When the click ray misses the terrain, the pointer ground point is where the reversed ray meets the inside of a sphere centred on the map centre, with radius `ref.ctrl.pointer.miss_sphere_factor` (40) × the world size (R-CTRL-058).
  2. **Smoothing overshoot.** First-order camera and cursor smoothing, x := x × (1 − k × dt) + target × k × dt, is not clamped where the reference does not clamp it: with the frame-delta cap of 0.25 s the free camera reaches k × dt = 1, and the turret crosshair, with k = 10, overshoots at low frame rates (R-CTRL-454).
- Options: A — the rules as stated; B — a miss falls back to the ray's meeting point with the plane at height 0, clamped to the map bounds (item 1), and k × dt is clamped to at most 1 everywhere (item 2).
- Recommendation: B for item 2, since an overshoot at low frame rates has no design purpose; A for item 1, which affects only rays that miss the terrain.
- Decide by: design phase

## 7 Maps and front end

### DEC-865 Map data without a reference case
- Status: Open
- Affects: reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP-013, R-MAP-048, R-MAP-072, R-MAP-080, R-MAP-092, R-MAP-120, R-MAP-121, R-MAP-131, R-MAP-190)
- Specified behaviour: The map schema accepts these forms, which no reference map uses or whose effect the reference does not define:
  1. a mode named twice in a map's mode list adds the map to that mode's list twice (R-MAP-013);
  2. the Essence-orb rain map effect has no effect in a match (R-MAP-092);
  3. a map may hold routes, but in a match a route id indexes the match's own route table, so a route of the map is never followed (R-MAP-120, R-MAP-121);
  4. a map need not be square, and the camera bounds and the Supply crate cap read the world size x only (R-MAP-048, R-MAP-080, R-MAP-190);
  5. a reinforcement power option names an outcome whose delivery is a Power, and the content build rejects any other value, where the reference accepts any research name (R-MAP-072);
  6. a Trooper starting-forces record whose in-group flag is off creates nothing (R-MAP-131).
- Options: A — the forms as stated; B — the content build also rejects a repeated mode, the Essence-orb rain flag, a routes block, a map that is not square and a Trooper record outside a group; C — specify the missing behaviour: a spawner for Essence-orb rain, routes that starting forces follow, and rules that read each axis of a map that is not square.
- Recommendation: B, with item 5 as stated. Every reference map already satisfies B, and data that has no effect is rejected before it reaches a match; C is a choice for the world game.
- Decide by: design phase

### DEC-866 Population-cap values of a map
- Status: Open
- Affects: reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP-061, R-MAP-062)
- Specified behaviour: The value forms of DEC-120 apply to the population caps of a map, with two consequences:
  1. a Siege defender population cap is absent, 0 or a number, and has no unlimited form, because the attackers' share, map cap − defenders × defender cap, is undefined for an unlimited defender cap (R-MAP-061, R-MAP-062);
  2. a population cap of 0 is allowed and locks every Spawn Node and Transit Gate of every team from the first check (R-MAP-062).
- Options: A — as stated; B — an unlimited defender cap leaves the defenders uncapped while the attackers share the map cap (item 1), and the schema rejects a population cap of 0 (item 2).
- Recommendation: A for item 1; B for item 2, because a match in which no Spawn Node can ever produce is never intended.
- Decide by: design phase

### DEC-867 Closed player slots
- Status: Open
- Affects: reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META-090), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP-043, R-MAP-046)
- Specified behaviour: The host's slot control of DEC-079 follows these rules, which the reference does not have because it has no such control:
  1. every slot that no human holds is open by default and can be closed before the match; a slot that a human holds cannot be closed; at least `ref.map.players.min` (2) slots stay open; a closed slot is never filled with a bot and never offered to a joining player; a session is full once every open slot is held; and the control also exists in single-player matches (R-META-090);
  2. every slot below the map's player count still receives a start position, so the structures at a closed slot's position belong to that unused team, and its starting forces are created for no playing team (R-MAP-043, R-MAP-046).
- Options: A — as stated; B — the control only in multiplayer sessions (item 1), and the structures at a closed slot's position start with no team while its starting forces are not created (item 2).
- Recommendation: A for item 1, which lets a solo player choose the number of opponents; B for item 2, so that a closed slot leaves no forces that no team controls.
- Decide by: design phase

### DEC-868 Front-end reference quirks
- Status: Open
- Affects: reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META-038, R-META-101), reference/09_front_end_and_match_flow/front_end_presentation.md (R-META-217, R-META-219), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP-021)
- Specified behaviour: These front-end rules are kept as the reference states them (DEC-006):
  1. **Chat rename.** The rename command is found anywhere in the chat text, but the new name is always taken from a fixed index (R-META-101).
  2. **Studio-mark fade.** When start-up work finishes before the studio mark has fully faded in, the fade-out starts at full opacity, so the mark jumps to opaque before it fades (R-META-217, R-META-219).
  3. **Play-time format.** The map-select play-time column shows "0:" followed by the minutes in two digits, so 60 minutes reads 0:60 (R-META-038, R-MAP-021).
- Options: A — keep each rule; B — accept the rename command only at the start of the text, fade out from the current opacity, and show true hours and minutes.
- Recommendation: B. Each is a parsing or display defect with no design purpose.
- Decide by: design phase

### DEC-869 Reference sound options without a Species counterpart
- Status: Open
- Affects: reference/09_front_end_and_match_flow/options_localisation_and_hooks.md (R-META-429, R-META-430, R-META-431)
- Specified behaviour: The sound options page keeps three reference options whose subject the Species audio engine fixes: the audio output list, which offers exactly one choice, the platform back end (R-META-430, ADR-022); the mixing rate, with the choices 11 025, 22 050 and 44 100 Hz, while the engine runs at one fixed rate (R-META-431, ADR-025); and the number of sound voices, while the engine has a fixed budget of 64 real voices (R-META-429, DEC-570).
- Options: A — keep the three options as the reference offers them; B — drop them from the Species sound page, which then holds the volume sliders of DEC-570 and the swap of left and right.
- Recommendation: B. An option whose choices the engine ignores, or that offers a single choice, misleads the player; DEC-100 already records that the mixing rate has no Species equivalent.
- Decide by: design phase

## 8 Tutorial

### DEC-870 Tutorial director details
- Status: Open
- Affects: reference/11_tutorial_and_help.md (R-TUT-023 to R-TUT-029, R-TUT-061, R-TUT-065, R-TUT-070 to R-TUT-075)
- Specified behaviour: The tutorial documents fix these details, which the reference leaves implicit:
  1. the deselect hint H3 fades, and the hints H4 to H7 are drawn at full opacity (R-TUT-023 to R-TUT-029);
  2. an instruction tab that would leave the screen at the right edge is clamped by the mirror of the left-edge test (R-TUT-070 to R-TUT-075);
  3. the director's step timer starts unset, and every test against an unset step timer passes, so the first request at tick 0 is accepted (R-TUT-065);
  4. the director is evaluated once per tick of match time, so it first runs on the first tick after the grace period and is not evaluated while the world is held (R-TUT-061).
- Options: A — the details as stated; B — define them anew with the tutorial step graphs (DEC-051, DEC-103).
- Recommendation: A for any reproduction of the reference tutorials; the tutorial redesign of DEC-051 replaces them.
- Decide by: design phase

## 9 Content outside the reference

### DEC-871 Essence orb break operation
- Status: Dropped
- Affects: reference/02_units/essence_orbs.md (R-UNIT-308), architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md (VFX-124)
- Specified behaviour: Not specified. An Essence orb has a broken marker, which Collectors and Builders test and whose presentation is defined (R-UNIT-308, VFX-124), but no rule of a match sets it; the break operation that would set it is outside the reference.
- Options: none.
- Recommendation: Dropped: no match breaks an orb, so the marker stays clear in every match.
