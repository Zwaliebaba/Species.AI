# Decision register: units, structures, crates and population flow

> Status: Reference decisions · Range: DEC-450 to DEC-549 · Related: [decisions index](README.md), [verdicts and fixes](verdicts_and_fixes.md), [conventions](../CONVENTIONS.md)

This file records the decisions on teams and alliances, match options, population flow, structures and Supply crates that the canonical register does not already hold. It covers the quirks, defects and ambiguous rules of these areas that need an explicit decision of their own: how start positions, slot colours and co-op friendship are assigned; which sources respect the population cap; how objects created during a tick are advanced; and a set of structure and crate corrections. The general decisions for these areas stay in `verdicts_and_fixes.md` and are not repeated here: the tick timers and the mode second (DEC-002, DEC-003), per-match state (DEC-011), bounded algorithms (DEC-014), tie-breaks (DEC-016), the subsystem verdicts (DEC-017 to DEC-021), the Power slots (DEC-033), and the fix-list and uncertainty entries DEC-064 to DEC-117. Each entry states the behaviour the reference documents specify now. A **Resolved** entry states the corrected rule that the documents apply; an **Open** entry states the faithful reference behaviour as the default, with a recommendation; a **Dropped** entry records content the library does not specify.

## Index

| ID | Title | Status | Affects |
|---|---|---|---|
| [DEC-450](#dec-450-start-positions-and-the-colours-of-bot-and-unused-slots) | Start positions and the colours of bot and unused slots | Open | R-MODE, R-META, R-SIM |
| [DEC-451](#dec-451-co-op-friendship-of-slots-outside-the-two-alliances) | Co-op friendship of slots outside the two alliances | Open | R-MODE, R-SIM |
| [DEC-452](#dec-452-comeback-bonus-power-pool-and-the-maps-excluded-outcomes) | Comeback Bonus power pool and the map's excluded outcomes | Open | R-CRATE, R-MODE |
| [DEC-453](#dec-453-population-cap-enforcement-points-and-uncapped-sources) | Population-cap enforcement points and uncapped sources | Open | R-ECON, R-CRATE, R-BLDG, R-UNIT |
| [DEC-454](#dec-454-population-limiter-as-a-fallback-cap) | Population Limiter as a fallback cap | Open | R-BLDG, R-ECON, R-MAP |
| [DEC-455](#dec-455-same-tick-advance-of-objects-created-or-handed-over-during-a-tick) | Same-tick advance of objects created or handed over during a tick | Open | R-SIM, R-ECON, R-CRATE, R-BLDG |
| [DEC-456](#dec-456-spawn-networks-are-trees-and-an-essence-orb-is-on-one-edge-at-a-time) | Spawn networks are trees, and an Essence orb is on one edge at a time | Resolved | R-BLDG, R-MAP, DAT |
| [DEC-457](#dec-457-structure-links-are-type-checked) | Structure links are type-checked | Resolved | R-BLDG, DAT |
| [DEC-458](#dec-458-linking-a-command-console-to-the-core-device-rebuilds-its-ports) | Linking a Command Console to the Core Device rebuilds its ports | Resolved | R-BLDG, R-MODE, R-AUD |
| [DEC-459](#dec-459-core-device-with-no-linked-command-console) | Core Device with no linked Command Console | Resolved | R-BLDG, R-MODE, R-MAP |
| [DEC-460](#dec-460-a-burning-hive-releases-ants-only-while-it-holds-any) | A burning Hive releases ants only while it holds any | Resolved | R-BLDG, R-UNIT |
| [DEC-461](#dec-461-each-turret-type-fires-from-the-sockets-of-its-own-mesh) | Each turret type fires from the sockets of its own mesh | Resolved | R-BLDG, R-CRATE, AST, VFX |
| [DEC-462](#dec-462-turret-facing-from-map-data) | Turret facing from map data | Open | R-BLDG, R-MAP, DAT |
| [DEC-463](#dec-463-one-surge-limit-for-the-fuel-pump) | One surge limit for the fuel pump | Resolved | R-BLDG, RND |
| [DEC-464](#dec-464-the-siege-solar-array-knock-out-uses-one-no-team-value) | The Siege Solar Array knock-out uses one no-team value | Resolved | R-BLDG, R-MODE |
| [DEC-465](#dec-465-score-weighted-supply-crate-drop-when-every-score-is-zero) | Score-weighted Supply crate drop when every score is zero | Resolved | R-CRATE, R-SIM |
| [DEC-466](#dec-466-defects-in-structures-and-fields-that-no-match-uses) | Defects in structures and fields that no match uses | Dropped | R-BLDG |

## 1 Teams, alliances and match options

### DEC-450 Start positions and the colours of bot and unused slots
- Status: Open
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md (R-META), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM)
- Specified behaviour: Outside Siege and co-op, a team's start position is drawn uniformly, with the simulation generator, from the map positions that no team holds yet, at the moment the match first needs that team's position. Match initialisation asks for the positions in team-slot order, so the draws happen in slot order. In a tutorial the position equals the team slot, and the Rogue and Visitor factions use their own slot ids. At match start the host fills every empty slot below the map's player count with a bot team. Each bot team in such a slot receives a random colour from 0 to 3 that no other team holds; DEC-014 makes this a choice from the list of free colours. Each unused slot, at or above the map's player count, draws a colour from 0 to 3 that no player slot holds, so that the Essence Grove and Allegiance Scrambler effects, which can pick a slot that no team plays, have a colour for every slot. A colour drawn for an unused slot is not reserved, so two unused slots can share a colour. Siege assigns sides and positions by its own procedure, and co-op assigns them by alliance (DEC-451).
- Options: A — the reference behaviour; B — assign every start position and every slot colour once, at match start, in slot order, from the match seed, and record the assignment in the match setup; unused slots receive distinct colours while free colours remain.
- Recommendation: B. The distribution of start positions is unchanged, but the assignment no longer depends on which code first asks for a position, and a recorded assignment lets the loading view, replays and bots read it without drawing from the simulation generator. Distinct colours for unused slots keep the random-colour effects distinguishable.
- Decide by: design phase

### DEC-451 Co-op friendship of slots outside the two alliances
- Status: Open
- Affects: reference/01_skirmish_rules_and_modes/README.md (R-MODE), reference/12_simulation_model/06_teams_and_player_commands.md (R-SIM)
- Specified behaviour: In a co-op match two teams are friends exactly when they belong to the same alliance group, group 1 or group 2. Outside co-op a team is a friend only of itself, and the no-team value is nobody's friend. The Rogue and Visitor factions belong to no group, so in a co-op match they are friends of each other, and in any other match they are not. Alliance assignment pairs the player slots by the map's group positions or by colour. Its last two fallback steps, which pair an uncoloured slot with a coloured slot that has no partner and force a random pair, never apply, because their test for an existing partner also matches the candidate slot itself. A player slot that no earlier step pairs therefore stays outside both groups, and it is then a friend of the Rogue and Visitor factions and of every other slot outside the groups.
- Options: A — the reference behaviour; B — alliance assignment always places every player slot in group 1 or group 2 (the fallback steps exclude the candidate itself, and a check at match start rejects an incomplete assignment), and the Rogue and Visitor factions are friends of no other team in any match.
- Recommendation: B. A player allied with the hazard factions has no design purpose, and one friendship rule for both factions matches DEC-082, which treats them as hazards in every mode. It is not known whether real colour choices ever leave a slot unpaired, so the cost of B is only the check.
- Decide by: design phase

### DEC-452 Comeback Bonus power pool and the map's excluded outcomes
- Status: Open
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/01_skirmish_rules_and_modes/README.md (R-MODE)
- Specified behaviour: An eliminated team's comeback power is drawn uniformly from a pool that lists the airstrike twice and the meteor shower, Essence Grove, Blight Grove and Brood once each, so the airstrike has probability 2/6 and each of the others 1/6. When the match allows basic crates only, the pool is the airstrike alone. The grant applies none of the validity rules that a Supply crate outcome passes, and it ignores the map's list of excluded outcomes. A grant needs a free Power slot; an eliminated team has two (DEC-033). When both are occupied, no power is granted and the owning player sees the notice `ui.power_slots.comeback_full`. The grant interval and the per-mode default of the Comeback Bonus are DEC-073.
- Options: A — the reference behaviour; B — the comeback draw is made over the pool members that are valid on the map and not excluded by it, keeping their relative weights, and no power is granted when none remains.
- Recommendation: B. A map designer who excludes an outcome expects it never to appear, whatever grants it; this matches the recommendation for hostile outcomes in DEC-107. On a map that excludes none of the five powers, B changes nothing.
- Decide by: design phase

## 2 Population flow

### DEC-453 Population-cap enforcement points and uncapped sources
- Status: Open
- Affects: reference/05_population_flow.md (R-ECON), reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/02_units/creatures.md (R-UNIT)
- Specified behaviour: The per-team population cap (DEC-079) is checked at four points only: at each production request of a Spawn Node; at each Transit Gate, once per reinforcement wave, so a wave that starts below the cap is released in full; at each ant sortie of a Hive; and at each Collector delivery, where the Collector keeps and waits with an orb that would exceed the cap (DEC-080). An Essence orb already travelling through the spawn network is not checked again when it arrives. These sources add bodies with no cap check: Builder deliveries at a friendly Spawn Node (DEC-085); the Reinforcements outcome (50 to 99 Workers); the 10 to 19 Workers that a Seed Pod releases, and the creatures of the Brood outcome; the passengers that a Launch Vessel releases when it explodes; a map's starting units; and actors that change team through Convert, the Core Device pulse wave or the Allegiance Scrambler. No global limit on the number of actors exists.
- Options: A — the reference behaviour; B — check the cap for every unit that a Transit Gate wave releases and for every Essence orb when it arrives, and keep power outcomes, conversions and starting units as named, uncapped exceptions; C — every source respects the cap.
- Recommendation: B. A power keeps its full value, which is the point of the Supply crate as a wildcard, while the steady sources stop overshooting the cap by a whole wave or by the orbs in transit. Together with the capped Collector delivery of DEC-080, every recurring source is then capped. Confirm the effect on army sizes by playtest across player counts.
- Decide by: playtest

### DEC-454 Population Limiter as a fallback cap
- Status: Open
- Affects: reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/05_population_flow.md (R-ECON), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
- Specified behaviour: A Population Limiter takes effect only on a map that sets no population cap; when the map sets one, every Population Limiter does nothing. On first use, a Spawn Node or a Transit Gate looks for the first Population Limiter in structure order whose radius contains it and keeps the result for the match. It is then locked while that limiter's count for its team is at or above the limiter's maximum. A limiter whose structure id is 0 is never consulted, because the kept result is used only when it is greater than 0. A limiter recounts every 90 to 110 ticks. On every reference map no Population Limiter takes effect: each map either sets a population cap or places no limiter.
- Options: A — the reference behaviour; B — keep the Population Limiter as a local cap that a map may combine with the map cap, with a defined lookup (the nearest limiter whose radius contains the structure, ties broken by stable id) and no id exception; C — remove the Population Limiter and make the population cap a required map value (DEC-079).
- Recommendation: B for the world game, where a local cap is a natural tool for settlements; for skirmish, B and C give the reference result on every reference map. The id exception has no purpose and is dropped under either option.
- Decide by: design phase

### DEC-455 Same-tick advance of objects created or handed over during a tick
- Status: Open
- Affects: reference/12_simulation_model/04_containers_and_update_order.md (R-SIM), reference/05_population_flow.md (R-ECON), reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md (R-CRATE), reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG)
- Specified behaviour: Each container advances its objects in slot order within a tick. An object created during a tick, such as an effect, a laser bolt or a Worker spawned into a team's container, advances in that same tick exactly when its slot lies after the last slot already processed. Freed slots are reused last-freed-first, and new slots are appended in blocks of 100. An object with a countdown per advance therefore starts its life one tick early, and an object timed by match time takes one extra movement step before an unchanged deadline; the rocket and the airstrike target marker are such objects. In the same way, an Essence orb handed from one spawn-network structure to the next advances again in that tick when the receiving structure comes later in structure order, so a transit of h hops takes between 9h + 1 and 10h + 1 ticks. The number of requests that Spawn Surge makes at a Spawn Node, 50 or 51, depends on whether the Supply crate opens before or after that Spawn Node in structure order, and the order of Transit Gate waves against the cap follows structure order. The exact spawn-network transit on a map therefore depends on the order in which its structures load; DEC-002 states its upper bound, 10h + 1 ticks. The fixed Spawn Surge count is DEC-304, the rocket's move count is a derived value of DEC-111, and the draws that initialise a new object happen in its spawn step (DEC-652).
- Options: A — reproduce the reference slot allocation and structure order, so that every same-tick advance matches, with the structure order fixed as the stable structure-index order (DEC-016); B — an object created or handed over during a tick first advances in the next tick, and containers iterate in an order that does not depend on allocation history (DEC-016, DEC-260); C — such an object always advances in the tick in which it is created.
- Recommendation: B. Each of these outcomes becomes a fixed rule that does not depend on the allocation history of a container; B is the rule behind the 10h + 1 transit of DEC-002, and every difference from the reference is a single tick. The design phase confirms it together with the update order of reference/12.
- Decide by: design phase

## 3 Structures

### DEC-456 Spawn networks are trees, and an Essence orb is on one edge at a time
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/data_schemas/ (DAT)
- Specified behaviour: The links between Spawn Nodes, Relays and the Source Node form a tree. The content build rejects a map whose spawn-network links contain a cycle, and the run-time walk over the links keeps a visited set, so it ends on any graph. An Essence orb that enters the spawn network is placed on exactly one edge at a time.
- Options: A — validated trees and one edge per orb; B — the reference behaviour, in which the link walk has no cycle detection and triggering one orb can place it on several edges. Every reference spawn network is a tree, so neither defect occurs on reference maps.
- Recommendation: A. A cyclic network would make the walk recurse without end on every participant, and an orb held by several edges could be delivered more than once. Validation at build time costs nothing on valid maps (DEC-015).

### DEC-457 Structure links are type-checked
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/03_buildings_and_world_objects/README.md (R-BLDG), architecture/components/data_schemas/ (DAT)
- Specified behaviour: A structure link names its target by handle and by the structure kind it expects. An energy-grid structure follows its link only when the target is an energy-grid structure. The content build rejects a map whose link targets a structure of another kind, and at run time a missing or mismatched target counts as no link. The same check applies to every link kind that a map can express.
- Options: A — type-checked links; B — the reference behaviour, in which an energy-grid structure treats its link target as an energy-grid structure without checking, which is undefined for a target of any other kind. Reference maps link only matching kinds.
- Recommendation: A. The unchecked conversion is a crash defect, and a typed link schema catches the mistake in the content build.

### DEC-458 Linking a Command Console to the Core Device rebuilds its ports
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG), reference/01_skirmish_rules_and_modes/siege.md (R-MODE), reference/10_audio_design/ (R-AUD)
- Specified behaviour: When the Core Device links a Command Console at load, the console switches to its three-port Core Device configuration by releasing its old ports and building the new ones explicitly. It does not run its structure initialisation a second time, so it plays no second creation sound and does not register itself again.
- Options: A — an explicit port rebuild; B — the reference behaviour, in which the old ports are discarded without being released and the whole structure initialisation runs again, which plays the creation sound again and repeats the registration.
- Recommendation: A. The reference path leaks the old ports and plays a sound that no player action caused.

### DEC-459 Core Device with no linked Command Console
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG), reference/01_skirmish_rules_and_modes/siege.md (R-MODE), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP)
- Specified behaviour: The Core Device is destroyed when every linked Command Console is held by a team and none is held by a friend of the Core Device's team. A Core Device with no linked Command Console would meet this condition on its first advance and be destroyed at once (R-BLDG-311), but the per-mode map validation of DEC-561 rejects every Siege map whose Core Device links fewer than three Command Consoles (R-BLDG-317), so no match reaches that case. No reference Siege map has such a Core Device.
- Options: A — the reference behaviour, with no validation; B — map validation rejects a Siege map whose Core Device links no Command Console.
- Recommendation: Resolved: B, applied by the Siege rule of DEC-561, which requires at least three linked Command Consoles. A Siege match that is decided on its first tick is never intended, and validation leaves the destruction rule unchanged for every valid map.

### DEC-460 A burning Hive releases ants only while it holds any
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), reference/02_units/creatures.md (R-UNIT)
- Specified behaviour: When a Hive burns, it releases an ant only while its ant count is above zero, and the count never falls below zero. The empty-Hive rule, which refills an empty Hive to 10 ants, applies unchanged.
- Options: A — the guarded release; B — the reference behaviour, in which a burning Hive releases an ant even when it is empty, its count turns negative, and the empty-Hive rule then refills it to 10.
- Recommendation: A. A negative ant count has no meaning, and the guard changes only what a burning, empty Hive releases.

### DEC-461 Each turret type fires from the sockets of its own mesh
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md (R-CRATE), architecture/components/asset_pipeline_and_resources.md (AST), architecture/components/animation_and_vfx/ (VFX)
- Specified behaviour: Each turret type takes its barrel-end and casing-ejection sockets from its own mesh, so every turret type fires from its own barrel positions. Casing ejection is a presentation effect and draws from the presentation generator (DEC-013).
- Options: A — sockets from the turret's own mesh; B — the reference behaviour, in which every turret looks its sockets up once, when it is created, on the gun-turret mesh, so rocket turrets, flame turrets and Convert Turrets fire from gun-turret barrel positions, and every shot of every turret type spends four simulation draws on casing ejection.
- Recommendation: A. Every turret mesh is authored new, so its sockets must match its own geometry, and moving the casing draws off the simulation generator changes nothing a player sees.

### DEC-462 Turret facing from map data
- Status: Open
- Affects: reference/03_buildings_and_world_objects/defences_and_props.md (R-BLDG), reference/08_maps_and_level_design/rules_and_conventions.md (R-MAP), architecture/components/data_schemas/ (DAT)
- Specified behaviour: A turret ignores the facing that the map gives it: its initial facing does not come from map data, and it turns towards its targets as reference/03 specifies.
- Options: A — the reference behaviour, with no facing field in the turret's map schema; B — the facing in the map sets the turret's initial facing.
- Recommendation: A. A turret turns towards every target it engages, so its initial facing affects only its first engagement, and dropping the field keeps the reference timing of that engagement. Choose B if the world game lets players place turrets with a chosen facing.
- Decide by: design phase

### DEC-463 One surge limit for the fuel pump
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/mode_objects.md (R-BLDG), architecture/components/renderer/entity_and_overlay_renderers.md (RND)
- Specified behaviour: A fuel pump tracks at most 20 surges at a time, and the renderer draws every surge that the simulation tracks; both use one constant.
- Options: A — one constant; B — the reference behaviour, in which the simulation tracks up to 20 surges and the renderer draws at most 15.
- Recommendation: A. The simulation value is authoritative, and the presentation shows the state it is given.

### DEC-464 The Siege Solar Array knock-out uses one no-team value
- Status: Resolved
- Affects: reference/03_buildings_and_world_objects/economy_and_spawn_objects.md (R-BLDG), reference/01_skirmish_rules_and_modes/siege.md (R-MODE)
- Specified behaviour: In Siege, a Solar Array that is captured from the team that held it at match start is knocked out: it loses its ports. A Solar Array that starts the match with no team has no starting team for this test, so its first capture does not knock it out. The starting-team field and the current-team field use one no-team value.
- Options: A — one no-team value; B — the reference behaviour, in which a neutral array's starting team is stored with a different no-team value from the one the test compares with, so a neutral-start array is knocked out by its first capture. Every reference Siege Solar Array starts owned by a team, so the difference never shows on reference maps.
- Recommendation: A. The knock-out is defined against a starting owner, and a neutral start has none.

## 4 Supply crates

### DEC-465 Score-weighted Supply crate drop when every score is zero
- Status: Resolved
- Affects: reference/04_supply_crates_powers_and_weapons/crates_and_powers.md (R-CRATE), reference/12_simulation_model/07_randomness_and_integrity.md (R-SIM)
- Specified behaviour: A score-weighted Supply crate drop favours lower-scoring teams. When the scores it weights total zero, every score counts as equal: every team in the weighting has the same weight, and the share p that the drop's first draw tests is max(1 ÷ n, 1 − 1 ÷ n) for n weighted teams, the limit of the rule for equal non-zero scores (R-CRATE-106). The weighting never divides by a zero total and never produces a non-finite value; the fallback to random placement happens only when that first draw exceeds p.
- Options: A — equal weights at a zero total; B — the reference behaviour, in which a zero total can produce a not-a-number weight that then feeds the drop.
- Recommendation: A. Equal weights are the limit of equal non-zero scores, so A agrees with the rule at every other score, and a non-finite value in simulation state is an integrity defect.

## 5 Content outside the reference

### DEC-466 Defects in structures and fields that no match uses
- Status: Dropped
- Affects: reference/03_buildings_and_world_objects/README.md (R-BLDG)
- Specified behaviour: Not specified. The structures concerned are outside the reference (DEC-052), and the fields concerned carry no behaviour. They are: a board-game prototype structure that reads options that do not exist, so it places no pieces, and that writes its data with a malformed format; null dereferences and divisions by zero in three campaign-only structures (a power-station link, a mine-track terminus and a scoring hub); orientation changes made while rendering, and unsynchronised random draws in the constructors, of two campaign-only structures; a Control Zone field for a bot target that nothing assigns; and a bot objective's reference to a wall that nothing reads.
- Options: none.
- Recommendation: Dropped: no match places these structures, and the two fields have no effect. The Species structure set defines its own fields.
