# Phase 4 arbitration from the reference (X-ARB)

Working file of agent X-ARB. Each entry settles one conflict between documents of `design_next/` (NEXT) from the original design-recovery corpus `design/` (ORIG). ORIG line numbers are quoted for the coordinator only; the replacement statements are written in clean Species form, ready for the document fixers to apply. Where ORIG does not settle a conflict, the entry says so and is flagged **design choice** for X-DEC.

Entry layout: question · statements (document, requirement id) · ORIG evidence · verdict · replacements (per document that must change).

The summary table of all entries is at the end of this file.

---

## A-1 Colour of a co-op group without a colour (item 20, DEC-078)

**Question.** What colour does a co-op group (alliance) that has no colour get: white, or the opaque grey of DEC-078?

**Statements.**
- `reference/12_simulation_model/06_teams_and_player_commands.md` **R-SIM-421**: the shade of a co-op group is handed out on request (member-1 shade first, member-2 shade afterwards); "a group without a colour returns `ref.sim.team_colour.no_group_shade` (white)".
- `reference/01_skirmish_rules_and_modes/README.md` **R-MODE-017**: "The colour of a co-op group that has no colour id is `ref.match.colour.no_group`, an opaque grey (DEC-078)."

**ORIG evidence.** ORIG describes two different lookups.
- `21_simulation_and_entity_model.md` l.1484 (§9.5, the group-level shade request): "`GetGroupColour(group)` returns the member-0 shade the first time, member-1 afterwards; white if the group has no colour." l.747: the per-group "shade already handed out" flags belong to this request.
- `01_game_modes.md` l.225 (§2.5.3, the colour-id-level member-shade lookup): "`GetGroupColour(member, −1)` returns `RGBAColour(0.6f, 0.6f, 0.6f, 1.0f)` … the floats convert to bytes 0, 0, 0, 1 — black at alpha 1/255, not the evidently intended 60 % grey"; l.1250 defect B22. `40_carry_forward_assessment.md` l.558 (F22) names this value "No-team colour black (alpha 1/255) instead of grey" and gives the verdict **Fix**, which is DEC-078.

**Verdict.** Both values exist and neither document is wrong about its own value; R-MODE-017 attaches the grey to the wrong lookup. The **shade request for a co-op group without a colour returns white** (R-SIM-421 stands, ORIG 21). The **opaque grey of DEC-078 is the member shade looked up for colour id none**, that is, the no-team colour (ORIG 01 l.225, ORIG 40 F22). DEC-078, R-BLDG-048 and RND-405 already state the no-team colour correctly and need no change.

**Replacements.**
- `reference/01_skirmish_rules_and_modes/README.md`, R-MODE-017 (replace the whole statement):
  > **R-MODE-017** (MUST) The member shade of colour id none, which is the no-team colour, is `ref.match.colour.no_group`, an opaque grey (DEC-078). A shade request for a co-op group that has no colour returns white instead (R-SIM-421).
- Same file, tuning-table row `ref.match.colour.no_group`, column "range or notes": `60 % grey, opaque; the no-team colour (member shade of colour id none)`.
- `reference/12_simulation_model/06_teams_and_player_commands.md`: no change.

## A-2 Damage dealt to the actors of an eliminated team (item 21)

**Question.** When a team is eliminated (the world wipe), is the damage to each actor 2 × its health stat (the per-kind maximum) or 2 × its current health?

**Statements.**
- `reference/12_simulation_model/06_teams_and_player_commands.md` **R-SIM-407**: "−`ref.sim.team.eliminate_damage_factor` × that actor's health stat (2 × health)"; tuning row `ref.sim.team.eliminate_damage_factor` unit "× health stat"; acceptance criterion AC-SIM-06-14 "−2 × its health stat".
- `reference/01_skirmish_rules_and_modes/README.md`, elimination table row "wiping a team" (cites R-SIM-407): "unresistable damage of 2 × its current health".
- `reference/05_population_flow.md` **R-ECON-010**: "unresistable damage equal to `ref.econ.elimination.damage_factor` (2) × its current health".

**ORIG evidence.**
- `01_game_modes.md` l.296 and `05_economy_and_population.md` l.96 (world wipe): "takes damage 2 × its **current** health", unresistable (the damage type is the default argument of the health change).
- `21_simulation_and_entity_model.md` l.919 and l.1432: "`ChangeHealth(−2 × StatHealth)` on every unit member and loose entity". This names the stat slot, which is not the per-kind maximum: `02_units_and_entities.md` l.228–231 and l.484 show that each entity copies its three stats into its own byte array `m_stats[3]` = {Health, Speed, Rate} and that this per-entity health slot is the value the health change modifies ("Health can never exceed 255 (every `ChangeHealth` clamps)"); the entity has no other health member (member table l.478–498). The per-kind maximum is read separately from the blueprint (l.234: "maximum-health fractions").

**Verdict.** ORIG settles it: **2 × current health**. The "health stat" of ORIG 21 is the entity's own health slot, which holds its current health. R-SIM-407 (and its tuning note and AC-SIM-06-14) must change; G01 and R-ECON-010 stand. The two values are the same number, so a full-health actor is unaffected; a damaged actor takes less than twice its maximum, and every actor still dies because twice its current health always exceeds it.

**Replacements.**
- `reference/12_simulation_model/06_teams_and_player_commands.md`, R-SIM-407:
  > **R-SIM-407** (MUST) Eliminating a team changes the health of every member of its actor groups and of every ungrouped actor by −`ref.sim.team.eliminate_damage_factor` × that actor's current health (2 × current health), as unresistable damage, stops all of its Powers, and sets every Transit Gate it owns to the no-team value.
- Same file, tuning row `ref.sim.team.eliminate_damage_factor`, unit column: `× current health`.
- Same file, AC-SIM-06-14: replace "−2 × its health stat" by "−2 × its current health".
- No change to `reference/01_skirmish_rules_and_modes/README.md`.
- Observation for the coordinator (not part of the conflict): the factor 2 is defined twice, as `ref.sim.team.eliminate_damage_factor` (R-SIM-407) and `ref.econ.elimination.damage_factor` (R-ECON-010). Under the one-key rule, R-ECON-010 should cite `ref.sim.team.eliminate_damage_factor` and its own row should be removed (route to X-ROUTE / G05 owner).

## A-3 Where a Builder's collected Essence orbs go (item 25, DEC-085)

**Question.** In a match, does a Builder deliver the Essence orbs it collects to the nearest friendly Spawn Node, or do they reach no destination?

**Statements.**
- `reference/02_units/builder.md` **R-UNIT-447**, **R-UNIT-454** item 2, **R-UNIT-457**: a carrying Builder that takes no further orb flies to the nearest Spawn Node (plain distance) whose owner is a friend of its team, to a delivery point at the node's position + front × 50 wu, and there delivers one orb per tick, each spawning one Worker of the node's current owner, with no cap or activation check.
- `reference/05_population_flow.md` **R-ECON-082**: "No structure in a match receives Builder-collected orbs, so a Builder keeps its orbs in the Attached state … until it dies"; also the population-event table ("Essence orb hoarding"), Figure 1 ("no destination"), the Figure 3 caption and the formula reference ("keeps them; no destination in a match").
- `reference/00_reference_overview.md` glossary row "Builder": "the orbs it collects reach no destination (DEC-085)".
- `decisions/verdicts_and_fixes.md` **DEC-085** (Open): "No reference map places the structure that receives Builder-collected orbs, so the orbs a Builder collects reach no destination."

**ORIG evidence.** ORIG contradicts itself; the code-level descriptions win.
- For delivery: `02_units_and_entities.md` l.1580 (§10, delivery): "target = nearest friendly Incubator (distance + 10 × its spirits) **or**, in Multiwinia, nearest friendly SpawnPoint (plain distance) [C] `Engineer::SearchForIncubator`. At a SpawnPoint: one spirit per tick removed and `SpawnPoint::SpawnDarwinian()`"; l.1525 (role: "returns each to a friendly Spawn Point as a new Darwinian"); l.1543 (drop position "spawnPoint.pos + front·50 (pushed out of obstructions)"); l.1560 (state machine: "Idle --> ToIncubator : nothing found, carrying spirits, friendly Incubator or MP SpawnPoint exists"). Independently, `10_audio_design.md` l.904 (sound inventory, written from the delivery function): "Engineer delivers a spirit to a spawn point (`Engineer::AdvanceToIncubator`)".
- Against: `05_economy_and_population.md` l.548 ("a friendly **Incubator** only … engineers **hoard** souls"), l.704, l.892, l.910; `00_overview.md` l.339; `40_carry_forward_assessment.md` l.164 (G24) and l.571 (F35, "give engineer souls a destination or remove the behaviour"); `41_risks_and_open_questions.md` l.681 (D067). These cite the search for orbs and the idle step, not the delivery-target search, and the 40 verdict rests on them.

**Verdict.** ORIG settles it for **delivery**: in a match a Builder delivers to the nearest friendly Spawn Node (G02a2's builder.md is right). R-ECON-082 and the related rows of reference/05, the G00 glossary row and DEC-085 must change. Because the reference already gives the orbs a destination, the aim of the 40 verdict ("give collected souls a destination") is met by the reference rule, so DEC-085 becomes **Resolved** with the reference rule as its specified behaviour. (X-DEC-P4a owns the entry; if it prefers to keep the Builder's economic role open for the design phase, the specified behaviour below still applies with status Open.)

**Replacements.**
- `reference/05_population_flow.md`, R-ECON-082:
  > **R-ECON-082** (MUST) A Builder takes Essence orbs in Birth or Floating that are not marked broken, of any team: while idle it searches every `ref.econ.builder.idle_retarget_ticks` (11 ticks) for the nearest within `ref.econ.builder.search_range_wu` (200 wu), and it carries up to `ref.econ.builder.capacity` (25 orbs at research level 3; 0, 10, 15, 25 and 30 for levels 0 to 4). A Builder that carries orbs and takes no further orb, because it is full or finds none, flies to the nearest Spawn Node, by plain distance, whose owner is a friend of its team, and there delivers one orb per tick; each delivered orb becomes one Worker of the node's current owner, with no population-cap check and no activation check (reference/02 builder.md R-UNIT-454, R-UNIT-457; DEC-085). Carried orbs are Attached and never expire; when a Builder dies, the orbs it still carries drop to Floating.
- Same file, population-event table row: `| Builder outcome | Essence orb recycling at friendly Spawn Nodes (§2.19) | no |`
- Same file, Figure 1 node: `B["Builder (crate unit)<br/>capacity 25, range 200 wu"]`, and after the edge `S -->|"Birth or Floating, any team"| B` add the edge `B -->|"1 orb per tick at the nearest friendly Spawn Node<br/>no cap check, no activation check"| D`.
- Same file, Figure 3 (§3.3): replace the transition `Attached --> [*] : delivered to a Hive` by `Attached --> [*] : delivered to a Hive, or by a Builder at a friendly Spawn Node (1 Worker)`; caption: "*Figure 3 — Essence orb states in a match. An actor that the Reaper Serpent kills leaves no orb, and a Builder's orbs stay Attached until it delivers them at a friendly Spawn Node or dies.*"
- Same file, formula reference, Builder line: `Builder:   takes Birth or Floating orbs of ANY team within 200 wu (capacity 25); delivers 1 per tick at the nearest friendly Spawn Node → 1 Worker each, no cap check, no activation check`
- Same file, §8 line: `- DEC-085 — Builder Essence-orb delivery to friendly Spawn Nodes (R-ECON-082).`
- `reference/00_reference_overview.md`, glossary row: `| Builder | A hovering Essence-orb collector that carries the orbs it collects to the nearest friendly Spawn Node, where each becomes a Worker (DEC-085) | [02](02_units/builder.md) |`
- `reference/02_units/builder.md` §8 row: title "Builder Essence-orb delivery to friendly Spawn Nodes"; text unchanged.
- `decisions/verdicts_and_fixes.md`, DEC-085 (replace the whole entry; same length class), and the title in `decisions/README.md`:
  > ### DEC-085 Builder Essence-orb delivery to friendly Spawn Nodes
  > - Status: Resolved
  > - Affects: reference/02_units/builder.md (R-UNIT-447, R-UNIT-454, R-UNIT-457), reference/05_population_flow.md (R-ECON-082), reference/00_reference_overview.md
  > - Specified behaviour: A Builder collects Essence orbs of any team in Birth or Floating, up to its capacity, and delivers them to the nearest Spawn Node, by plain distance, whose owner is a friend of its team: one orb per tick, each becoming one Worker of the node's current owner, with no population-cap or activation check. Carried orbs never expire; a Builder that dies drops them to Floating.
  > - Options: A — the delivery rule as specified; B — remove the Builder's orb collection.
  > - Recommendation: A. The collected orbs have a destination on every map with Spawn Nodes, and the Builder outcome is valid only on such maps, so the behaviour has a clear economic effect and needs no change.

## A-4 Is a Worker from a Spawn Node door airborne or on the ground? (item 29)

**Question.** Does a Worker that a Spawn Node produces at its door start airborne (G05) or on the ground (G02a1)?

**Statements.**
- `reference/05_population_flow.md` **R-ECON-043**: the door Worker appears "with no position spread, airborne, and with velocity (door front + j) × `ref.econ.spawn_node.door_speed`".
- `reference/02_units/worker.md` **R-UNIT-061**: "The Worker's spawn hook … sets the Worker on the ground … A creator that wants an airborne Worker clears the on-ground flag after the spawn step (reference/05 states which creators do)." G02a1's report asked G05 to confirm that the Spawn Node and the Transit Gate do so.

**ORIG evidence.**
- `02_units_and_entities.md` l.588: the Worker's start hook: "`Begin`: `Entity::Begin`; `m_onGround = true`; `m_wayPoint = m_pos` …". `21_simulation_and_entity_model.md` l.686: the spawn routine sets the flag false, sets velocity and front, "then `Begin()`".
- `03_buildings_and_world_objects.md` l.737 (Spawn Node outputs): "spawned at the door marker; MP velocity `(door.front + (syncsfrand(1), syncsfrand(1), syncsfrand(1))) × 10` … `m_onGround = false`"; `05_economy_and_population.md` l.270: the door spawn passes the velocity, "`m_onGround = false`", after the spawn call.
- `05_economy_and_population.md` l.598 (Transit Gate wave): "airborne (`m_onGround = false`)".

**Verdict.** ORIG settles it, and the two documents do not really disagree: the spawn hook sets the Worker on the ground, and the Spawn Node door then clears the flag, so **the door Worker starts airborne** (R-ECON-043 is right; R-UNIT-061 is right as a hook rule). The Transit Gate wave does the same, and R-ECON's gate rule already says so. The only fix is to make the order explicit in R-ECON-043 so that the two rules read as one sequence.

**Replacements.**
- `reference/05_population_flow.md`, R-ECON-043:
  > **R-ECON-043** (MUST) A Worker produced by a Spawn Node appears at the node's door socket as an ungrouped actor of the node's owner, with no position spread and with velocity (door front + j) × `ref.econ.spawn_node.door_speed` (10), in wu/s, where j has three components, each drawn from `ref.econ.spawn_node.door_jitter` (U[−0.5, 0.5)). After the Worker's spawn step, which sets it on the ground (reference/02 worker.md R-UNIT-061), the Spawn Node clears its on-ground flag, so the Worker starts airborne.
- Same file, §2.21 step 2, last sentence: "The Workers start airborne: the gate clears each Worker's on-ground flag after its spawn step (reference/02 worker.md R-UNIT-061)."
- `reference/02_units/worker.md`: no change.

## A-5 Core Device countdown on the reference maps (item 42)

**Question.** Is the Core Device countdown 900 mode seconds on every reference Siege map (G03a), or 900 on most and 1,200 on one (siege.md)?

**Statements.**
- `reference/03_buildings_and_world_objects/mode_objects.md` **R-BLDG-317**: "Every reference Siege map places one Core Device … with 3 or 4 linked consoles and a countdown of 900 mode seconds".
- `reference/01_skirmish_rules_and_modes/siege.md` **R-MODE-351**: "The reference maps use 900, and 1,200 on one map", with the derived ticks 9,889 and 13,189; AC-01 tests both.
- `reference/08_maps_and_level_design/rules_and_conventions.md` **R-MAP-167**: 900 on 7 maps (play time 15 minutes) and 1,200 on the 4-player map with play time 20 minutes.

**ORIG evidence.**
- `01_game_modes.md` l.914: the bomb timer "counts **mode seconds**: 900 on seven maps, 1200 on `mp_assault_4P_2` (= PlayTime × 60)".
- `08_levels_and_level_design.md` l.590–597 (per-map table): seven maps `t=900s`, one 4-player map `t=1200s` with PlayTime 20; l.790: "every shipped Assault map has 15 (20 on `mp_assault_4P_2`), matching its `PulseBomb` `startTime` / 60".
- `03_buildings_and_world_objects.md` l.1701 (the source of R-BLDG-317): "official maps use `900.00` with 3–4 station ids", a census summary that the per-map table contradicts.

**Verdict.** ORIG settles it: **900 on seven maps, 1,200 on one 4-player map** (siege.md and R-MAP-167 are right). R-BLDG-317 must change.

**Replacements.**
- `reference/03_buildings_and_world_objects/mode_objects.md`, R-BLDG-317, second sentence:
  > Every reference Siege map places one Core Device, owned by a defending position, with 3 or 4 linked consoles and a countdown equal to the map's play time in mode seconds: 900 on seven maps and 1,200 on one 4-player map ([01 Siege](../01_skirmish_rules_and_modes/siege.md) R-MODE-351, [08 rules and conventions](../08_maps_and_level_design/rules_and_conventions.md) R-MAP-167).
  The first and third sentences stay as they are.
- No other line of mode_objects.md repeats the value (checked: the only other citation of R-BLDG-317 is edge case 8, which is unaffected). No change to siege.md or R-MAP-167.

## A-6 Blight-tree rules: the three differences between G03b and G04 (items 46 and 49)

**Question.** Now that G04 adopted G03b's values, are G03b's three readings right? (1) the kill reach is half the tree's **actual** height (not its full height), and the victim is the nearest enemy of any kind, killed only when it is a Worker not operating a port; (2) the corrupted tree is the **first tree in structure order** that is not blight, not corrupted and within the actual height (not "the nearest healthy tree"); (3) a corrupted tree turns blight after **101** ticks (not 100). Also: does R-CRATE-181 now state the corrected turret line of sight?

**Statements (current).**
- `reference/03_buildings_and_world_objects/trees_and_crate_structures.md` **R-BLDG-620** (reach hA × 0.5, nearest enemy of the Rogue faction, kill only a Worker not operating a port), **R-BLDG-621** (first tree in structure order; blight countdown `ref.bldg.tree.blight_delay_ticks` 101 ticks), **R-BLDG-622**, **R-BLDG-623**.
- `reference/04_supply_crates_powers_and_weapons/crates_and_powers.md` **R-CRATE-206**: now cites R-BLDG-620 to R-BLDG-623 and the `ref.bldg.tree.*` keys and states the same three readings; **R-CRATE-181**: now states the corrected line of sight of R-BLDG-413 (DEC-088).

**ORIG evidence.**
- `03_buildings_and_world_objects.md` l.1360 (tree code, kill aura): "each tick with probability `syncfrand(10) < 0.2` (2%): `GetBestEnemy(pos, actualHeight/2, monster team)`; if that is a Darwinian not in `StateOperatingPort` → `ChangeHealth(−999)`". l.1325: `GetActualHeight` shrinks with the burn phase.
- l.1361 (corrupting): "at ≤ 0 it resets to `10 + syncfrand(5)` and the **first** tree (building-array order) that is not evil, not corrupted and within `actualHeight` becomes corrupted … `m_corruptCheckTimer = −10` … (one victim per check)".
- l.1362 (becoming evil): "a non-evil tree with `m_corruptCheckTimer < 0` adds dt each tick; when it passes 0 the timer resets to 5 and `m_evil = true`". With dt = the binary64 tick period 0.1, the timer after 100 additions is −1.9 × 10⁻¹⁴, still below 0, and it passes 0 on the **101st** tick (replayed by X-ARB: 101 ticks under both "> 0" and "≥ 0").
- `04_crates_powerups_and_weapons.md` l.835 (the crate-outcome summary that G04 first followed): "kills the nearest non-Monster Darwinian within `height/2` … corrupts the nearest normal tree within its height (that tree turns evil 10 s later)". This is a summary of the same tree code and is less precise on all three points.

**Verdict.** ORIG settles all three points in favour of **G03b**: actual height, first tree in structure order, 101 ticks. The current R-CRATE-206 agrees with R-BLDG-620 to R-BLDG-623 and cites them, and R-CRATE-181 states the corrected rule with DEC-088. **No document change is needed**; item 46 is closed.

**Replacements.** None.

## A-7 Which Powers test the exclusion zones (item 48, sub-point 1)

**Question.** Which Powers have a whole-map target area and so reject a target point inside an exclusion zone that blocks Powers?

**Statements.**
- `reference/03_buildings_and_world_objects/defences_and_props.md` **R-BLDG-464**: "The powers with a whole-map target area are the Shield, Speed Boost, the gun turret, rocket turret and flame turret powers, Swarm, Essence Grove, Blight Grove, Hive, Sickness, Brood, meteor shower and Fury (reference/04)."
- `reference/04_supply_crates_powers_and_weapons/README.md` table 2.5-A and **R-CRATE-034**: the zone test applies to the turret Power (with its own clearance tests) and to "airstrike (both bomb kinds), Missile Barrage, Swarm, Essence Grove, Blight Grove, Hive, Brood, meteor shower"; the Shield, Speed Boost, Convert and Fury need an own actor within 70 wu and ignore zones; R-CRATE-035: no Sickness Power exists.

**ORIG evidence.**
- `04_crates_powerups_and_weapons.md` l.588–595 (targeting rules): "Booster, HotFeet, Subversion, Rage | at least one entity of the owner's team within 70"; "GunTurret (all three variants) | not inside a `blockPowerups` restriction zone; no building whose `DoesSphereHit(point, 10)`; not within 400 … nor within 200 …"; "AirStrike (incl. Napalm), Nuke, Infection, MagicalForest, DarkForest, AntsNest, Plague, EggSpawn, MeteorShower | anywhere inside the landscape except inside a `blockPowerups` restriction zone".
- l.798 (Plague = Sickness): "**Bad pool** only (immediate at the crate). A forced Plague reward hits `AppReleaseAssert` in `CaptureCrate` (its task case is commented out)": the Sickness never becomes a Power, so its row in the targeting table is never reached.
- Deliveries as Powers: l.643 (airstrike), l.674 (Missile Barrage), l.756 (Swarm, forced outcome only), l.766 (Essence Grove, Comeback grant only), l.788 (Hive), l.807 (Brood), l.817 (meteor shower), l.831 (Blight Grove), l.744 (turrets).

**Verdict.** ORIG settles it: **G04 is right**. R-BLDG-464 wrongly includes the Shield, Speed Boost, Sickness and Fury and omits the airstrike, Firebomb Run and Missile Barrage. The turret Powers test the zone as one of their own conditions.

**Replacements.**
- `reference/03_buildings_and_world_objects/defences_and_props.md`, R-BLDG-464 (replace the whole statement):
  > **R-BLDG-464** (MUST) The exclusion test has exactly two callers: the random placement of a Supply crate, which rejects an excluded point with forPowers false ([04 Crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md)); and power targeting, which rejects a target point with forPowers true for the turret powers (gun turret, rocket turret and flame turret), as one of their placement conditions, and for the powers whose target area is the whole map: the airstrike, Firebomb Run, Missile Barrage, Swarm, Essence Grove, Blight Grove, Hive, Brood and meteor shower powers. The Shield, Speed Boost, Convert and Fury powers need an own actor near the target point and ignore exclusion zones ([04 README](../04_supply_crates_powers_and_weapons/README.md) table 2.5-A, R-CRATE-034).
- No change to reference/04.

## A-8 Life of a rocket-turret rocket: 10 or 11 ticks (item 48, sub-point 2)

**Question.** A rocket fired by a rocket turret has a nominal life of 1.0 s. Is that 10 ticks (G03b) or 11 ticks (G04)?

**Statements.**
- `reference/03_buildings_and_world_objects/defences_and_props.md` **R-BLDG-420** (launch-values table, rocket turret row): "with a life of `ref.bldg.turret.rocket.life_ticks` (10 ticks)"; tuning row `ref.bldg.turret.rocket.life_ticks | 10 | ticks | 1.0 s`.
- `reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md` **R-CRATE-431**: a rocket's life is "the first tick on which its age exceeds" its life in seconds: 3, 6, 8, **11** and 16 ticks for 0.25, 0.5, 0.75, **1.0** and 1.5 s; "A life given by the caller replaces it: the rocket turret's `ref.bldg.turret.rocket.life_ticks`".

**ORIG evidence.**
- `03_buildings_and_world_objects.md` l.800 (rocket turret barrel): `FireRocket(barrelEnd, barrelEnd + barrelFront·500, team, life 1.0, turretId)`.
- `04_crates_powerups_and_weapons.md` l.1171: "a non-zero `m_lifeTime` overrides (Rocket turret 1.0, Tank 2.0)"; l.1172: "Explodes when `now > m_timer + maxLife` (after moving)"; l.1176: "turret life 1.0: **11 or 12 moves**"; l.1178: the expiry test "is first true on the 8th tick after the creation tick N (0.8 > 0.75); the move precedes the test, so ticks N+1 … N+8 each move the rocket", with one extra move only when the rocket is also advanced in its creation tick ("9 moves (turret 12); otherwise 8 (turret 11)").

**Verdict.** ORIG settles it: the turret rocket expires on the **11th** tick after the tick it was fired in, the same conversion that R-CRATE-431 uses for every rocket life (1.0 s → 11). G04 is right; `ref.bldg.turret.rocket.life_ticks` must be 11. (The creation-tick move, which makes 12 moves possible in the reference, is the general rocket rule of R-CRATE-431 and is not changed here.)

**Related finding (same table, same conversion; found by X-ARB, not raised by a report).** The flame turret's fireball has the same nominal 1.0 s life and the same off-by-one. ORIG evidence: `04_crates_powerups_and_weapons.md` l.1238: "Per tick `life −= dt`; … `radius = 20 × min(1, abs(1 − life))` (grows 2 → 20 as life goes 0.9 → 0)"; l.1243: "Removal | `life ≤ 0` or touching the ground"; l.1485 (resolved open question on these timers): "Tick counts of `x −= 0.1` timers: computed … with IEEE-754 doubles — 1.0 → 11 ticks". X-ARB replay: after 10 subtractions the life is 1.4 × 10⁻¹⁶, still above 0, so the fireball is removed on its 11th tick. Verdict: `ref.bldg.turret.flame.fireball_life_ticks` is 11, not 10. Consequence: R-CRATE-461 derives the radius growth by dividing by this key, which with 11 would no longer give the reference growth of 2 wu per tick reaching 20 wu after 10 ticks; it must state the growth directly. Whether the fireball still damages on its 11th tick (removal before or after the damage pass) is not stated in ORIG and stays as reference/04 now orders it.

**Replacements.**
- `reference/03_buildings_and_world_objects/defences_and_props.md`, R-BLDG-420 table, rocket turret row, last cell: "from the muzzle towards the point muzzle + barrel front × `ref.bldg.turret.rocket.aim_distance` (500 wu), with a life of `ref.bldg.turret.rocket.life_ticks` (11 ticks: the first tick on which the rocket's age exceeds 1.0 s, as in R-CRATE-431)".
- Same file, tuning row: `| ref.bldg.turret.rocket.life_ticks | 11 | ticks | nominal 1.0 s; the first tick on which the age exceeds 1.0 s | R-BLDG-420 | DEC-002 |`
- No acceptance criterion of defences_and_props.md uses either key (checked).
- Related finding, same file: R-BLDG-420 table, flame turret row: "with a life of `ref.bldg.turret.flame.fireball_life_ticks` (11 ticks)"; tuning row `| ref.bldg.turret.flame.fireball_life_ticks | 11 | ticks | nominal 1.0 s; removed on the 11th tick | R-BLDG-420 | DEC-002 |`.
- Related finding, `reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md`: R-CRATE-460 "(10 ticks, 1.0 s)" becomes "(11 ticks, nominal 1.0 s)"; the summary table row of the fireball "10 ticks" becomes "11 ticks"; R-CRATE-461, the radius clause, becomes "its radius becomes min(`ref.weapon.fireball.max_radius` (20 wu), `ref.weapon.fireball.growth_per_tick` × k) after k ticks, growing by 2 wu per tick from 2 to 20 wu", with the new tuning row `| ref.weapon.fireball.growth_per_tick | 2 | wu per tick | the radius reaches 20 wu after 10 ticks | R-CRATE-461 | — |`.

## A-9 What cancelling a Transport's Power does (item 48, sub-point 3)

**Question.** When a team cancels (stops) the Power of a Transport it placed, does the Transport stay in the world (G02a2) or is it destroyed (G04)?

**Statements.**
- `reference/02_units/transport.md` **R-UNIT-370**: "Cancelling a Transport's Power frees its Power slot and leaves the Transport in the world, where it stays with no slot (see DEC-551)"; also the command table row "cancel the Power … the Transport stays (R-UNIT-370)", edge case 8 ("Cancelled Power") and the §8 row for DEC-551.
- `reference/04_supply_crates_powers_and_weapons/README.md` **R-CRATE-020**, stop-effect table: "Transport | the Transport takes a health change of −`ref.power.stop.damage` (1000), except in a tutorial".

**ORIG evidence.**
- `04_crates_powerups_and_weapons.md` l.583 (task manager rules, "Player termination (`Task::Stop`)"): "Armour (except in tutorials), Engineer, Tank: the entity `ChangeHealth(−1000)`".
- `06_player_control_and_input.md` l.354 (task table, the Transport task): stop effect "`ChangeHealth(−1000)`; **refused during the Multiwinia tutorial** (`stop = false`)".
- `02_units_and_entities.md` §8 (l.1302–1429), the source of transport.md, does not describe the stop.

**Verdict.** ORIG settles it: **G04 is right**. Outside a tutorial, stopping a Transport's Power destroys the Transport; only in a tutorial does the Transport refuse the stop and stay in the world with no slot, which is the case DEC-551 records. R-UNIT-370 and its dependent rows must change. (Whether a refused stop still frees the slot is DEC-551's rule as R-CRATE-020 states it; this entry does not change it.)

**Replacements.**
- `reference/02_units/transport.md`, R-UNIT-370:
  > **R-UNIT-370** (MUST) A Transport's Power ends when the Transport is dead or no longer exists. Stopping a Transport's Power, by its owner's cancel command or by a stop of all of the team's Powers, gives the Transport a health change of −`ref.power.stop.damage` (1000), which destroys it ([reference/04 README](../04_supply_crates_powers_and_weapons/README.md) R-CRATE-020). In a tutorial the Transport refuses the stop: the Power's slot is freed and the Transport stays in the world with no slot (see DEC-551).
- Same file, command table row: `| cancel the Power | PlayerCommand::CancelPower | the Transport is destroyed; in a tutorial the slot is freed and the Transport stays (R-UNIT-370) |`
- Same file, edge case 8: "**Cancelled Power.** A cancelled Transport Power destroys the Transport, except in a tutorial, where the Transport stays in the world outside the Power-slot limit (R-UNIT-370, DEC-551)."
- Same file, §8 row DEC-551: "in a tutorial a cancelled Transport Power frees its slot and the Transport stays (R-UNIT-370)".

## A-10 Life of a Strike team's Power: 600 or 601 ticks (item 48, sub-point 4)

**Question.** Is the nominal 60 s life of a Strike team's Power 600 ticks (G02a2) or 601 ticks (G04)?

**Statements.**
- `reference/02_units/strike_team.md` **R-UNIT-338**: "starts with a life of `ref.unit.strike_team.life_ticks` (600 ticks, nominal 60 s)"; tuning row 600; edge case 2 ("during the 600-tick life"); AC-ST-01 ("after 600 ticks every Trooper is dead").
- G04 (report, and R-CRATE-152, which cites the key): ORIG gives a float counter of 601 ticks.

**ORIG evidence.**
- `04_crates_powerups_and_weapons.md` l.582: "Lifetimes set in `Task::Start` | Squad 60 s (float counter: 601 ticks); Booster, HotFeet, Subversion, Rage 30 s (300 ticks)"; l.606: "60 s (`m_lifeTimer`, multiplayer; 601 ticks)"; l.1485 (04-U12, resolved): "float `Task::m_lifeTimer` 30 → 300, 60 → 601", whereas a double 60.0 gives 600.
- `06_player_control_and_input.md` l.313: "`m_lifeTimer` | float (s)".
- X-ARB replay: a binary32 counter starting at 60 and decreased by the 0.1 s tick period reaches ≤ 0 on the **601st** tick (300th for 30); a binary64 counter would reach it on the 600th.
- `02_units_and_entities.md` l.1209 (the source of strike_team.md): "the squad dies after **60 s**" (nominal only).

**Verdict.** ORIG settles it: the reference count is **601 ticks**. Under DEC-002 every timer is an integer tick count equal to the reference count, so `ref.unit.strike_team.life_ticks` is 601 (nominal 60 s). The buff Powers' 300 ticks (`ref.power.lifetime.buff_ticks`) are right and unchanged.

**Replacements.**
- `reference/02_units/strike_team.md`, R-UNIT-338, first sentence: "A Strike team's Power starts with a life of `ref.unit.strike_team.life_ticks` (601 ticks, nominal 60 s)." The rest of the statement stays.
- Same file, tuning row: `| ref.unit.strike_team.life_ticks | 601 | ticks | nominal 60 s; counts down only while above 0 | R-UNIT-338 | DEC-002, DEC-551 |`
- Same file, edge case 2: "during the 601-tick life".
- Same file, AC-ST-01: "… after 601 ticks every Trooper is dead; with no other damage, every Trooper is still alive after 600 ticks."

## A-11 An airstrike from the bad branch of a Supply crate (item 48, sub-point 5)

**Question.** Can the bad branch of a Supply crate yield an airstrike or Firebomb Run owned by the Rogue faction at the crate's position, as hazards_and_projectiles.md states?

**Statements.**
- `reference/02_units/hazards_and_projectiles.md` §1 object table, row "airstrike flight, Bomber": source "the airstrike and Firebomb Run powers and outcomes", owner "the power's owner, or the Rogue faction"; **R-UNIT-711**, source table row 3: "the airstrike or Firebomb Run outcome of an unopened Supply crate, at the crate's own position | as the outcome | the Rogue faction"; §5 edge case "**Hostile airstrike.**".
- `reference/04_supply_crates_powers_and_weapons/crates_and_powers.md` **R-CRATE-122** (cited by G04): the bad pool holds no airstrike.

**ORIG evidence.**
- `04_crates_powerups_and_weapons.md` l.337 (bad pool): the list is "{Spam, Randomiser, Plague, DarkForest, Triffids, [TheBeast — DEBUG], SpaceShip (…), AntNest}" (Swarm, Randomiser, Sickness, Blight Grove, Spitter Plants, the debug-only outcome, the Abductor, Hive), and "The `m_rewardId != −1` branch and the switch cases Airstrike, Darwinians, Forest, Eggs, Meteor, NapalmStrike, SlowDown, SpeedUp inside `RunUncapturedCrate` are **unreachable** (a forced reward always goes to `CaptureCrate`, and the list never contains those types)".
- l.643: the airstrike is delivered only as a Power targeted by its owner.

**Verdict.** ORIG settles it: the path is **unreachable**. The bad branch never yields an airstrike or a Firebomb Run, so row 3 of R-UNIT-711 and the edge case describe nothing that can happen; they are removed. (An airstrike Power opened by a Rogue-faction majority at a crate is the ordinary "opening team" row; whether that occurs is outside this conflict. The `crate.airstrike.announce_rogue` and `crate.firebomb_run.announce_rogue` strings of reference/04 are G04's and are not changed by this entry.)

**Replacements.**
- `reference/02_units/hazards_and_projectiles.md`, R-UNIT-711: delete table row 3 ("the airstrike or Firebomb Run outcome of an unopened Supply crate, at the crate's own position | as the outcome | the Rogue faction"). The other three rows stay, and the statement becomes "An airstrike runs only from these sources:".
- Same file, §1 object table, row "airstrike flight, Bomber": source cell "the airstrike and Firebomb Run Powers", owner cell "the Power's owner, or the Transit Gate's owner for a reinforcement airstrike".
- Same file, §5: delete the edge case "**Hostile airstrike.** …" and, if a statement is wanted in its place: "- **No hostile airstrike.** The bad branch of an opened Supply crate never yields an airstrike or a Firebomb Run; every airstrike belongs to the team that targets its Power (R-UNIT-711; [crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md) R-CRATE-122)."

## A-12 Recount period of bot navigation nodes and bot objective markers (item 52, DEC-064)

**Question.** Do bot navigation nodes recount, and bot objective markers evaluate, every 21 ticks (reference/03, applying DEC-064) or every 20 ticks (reference/07)?

**Statements.**
- `reference/03_buildings_and_world_objects/README.md` table 2.7-A, rows "bot navigation node" and "bot objective marker": period "21 ticks"; **R-BLDG-003**: "a recount that a structure performs with a period written as … two seconds [runs] every `ref.mode.recount.two_second` (21 ticks) … (DEC-064)".
- `reference/07_bots/01_navigation_graph.md` **R-BOT-041** (`ref.bot.node.recount_ticks`, 20 ticks), **R-BOT-046**, AC-BOT-12; `reference/07_bots/05_mode_strategies_and_tutorials.md` **R-BOT-133** (`ref.bot.marker.period_ticks`, 20 ticks).
- `decisions/verdicts_and_fixes.md` **DEC-064** (Resolved): affects mode_objects.md and the zone_control, flag_chain and relic_run rules only: "The recounts of Control Zones, Flag posts and Relics run on one fixed integer period".

**ORIG evidence.**
- `07_ai.md` l.427–431 (node advance): "Every tick: `m_teamCountTimer -= SERVER_ADVANCE_PERIOD`; when ≤ 0 (every 20 ticks = 2.0 game-s, random initial phase): 1. `RecalculatePriority()` … 2. `m_teamCountTimer = 2.0`; 3. `RecountTeams()`; 4. `RecalculateOwnership()`".
- l.603 and l.605 (timer table): "Node recount + priority | `AITarget::m_teamCountTimer` | 2.0 s | **20**"; "Marker evaluation | `AIObjectiveMarker::m_timer` | 2.0 s | **20**". l.612–614 list, by contrast, the timers re-armed from the absolute game time ("`GetNetworkTime() + 1.0` | 10 or 11"), which is the irregular cadence that DEC-064 replaces.
- X-ARB replay: a countdown reset to 2.0 and decreased by the binary64 tick period reaches ≤ 0 on the 20th tick every time (residual −6.4 × 10⁻¹⁶, not carried, because the timer is reset, not incremented).

**Verdict.** ORIG settles it: **20 ticks** for both (reference/07 is right). These are countdown timers with a fixed reset, so their period never varies, and DEC-064 does not apply to them; DEC-064 covers only the recounts re-armed from the match clock. The coordinator's proposed action for item 52 ("G07 states 21 with DEC-064") is therefore not taken: reference/03 changes instead, and R-BLDG-003 is narrowed so that it no longer reads as covering every two-second structure timer.

**Replacements.**
- `reference/03_buildings_and_world_objects/README.md`, R-BLDG-003:
  > **R-BLDG-003** (MUST) Every structure timer is an integer number of ticks, read from the tick counter (DEC-002, DEC-009). A recount that a structure re-arms from the match clock with a period written as one second runs every `ref.mode.recount.one_second` (11 ticks), and one written as two seconds every `ref.mode.recount.two_second` (21 ticks), both keys of reference/01 (DEC-064). Every other structure timer, including the countdowns of the bot navigation node and the bot objective marker (20 ticks, reference/07), states its tick count with its own key in the file that owns the structure.
- Same file, table 2.7-A, row "bot navigation node", Period cell: "`ref.bot.node.recount_ticks` (20 ticks)"; row "bot objective marker", Period cell: "`ref.bot.marker.period_ticks` (20 ticks)".
- No change to reference/07 or DEC-064.

## A-13 Two bot selection rules for the Transport and the Strike team (item 54)

**Question.** (1) In Launch Race, while the vessel boards, from which bot navigation node does a bot Transport collect idle Workers: the node with the **most** idle Workers (transport.md) or the **first** node in structure order with more than 10 (G07)? (2) Which enemy does a bot Strike team head for: the nearest enemy that has a cluster (strike_team.md, "nearest first") or the first enemy, in query result order, that is nearer than all earlier ones and has a cluster (G07)?

**Statements.**
- `reference/02_units/transport.md` **R-UNIT-407** item 2: "it collects from the bot navigation node that has the most idle Workers of its team, provided they number more than … (10), the node is more than … (400 wu) from the vessel and no other Transport … within … (100 wu) of it; or else at its team's Transit Gate."
- `reference/02_units/strike_team.md` **R-UNIT-360**: "scans the enemies within … (500 wu) of its centre, nearest first, and picks the first enemy that has at least … (20) enemies within … (75 wu)".
- G07 report, request 7 to G02a2: first node in structure order; running-nearest scan in result order.

**ORIG evidence.**
- (1) `07_ai.md` l.979: "`FindIdleDarwinians()` (RR ferry) | the **first** AITarget (building order) with `idleCount[team] > 10`, > 400 from the own rocket and `RRValidPickupPoint` → conversion point there; else the own `TrunkPort` front × 75 (if no trunk port: the world origin). Resolved: P04 said "the AITarget with the most idle"; the loop `break`s at the first node with > 10". `02_units_and_entities.md` l.1421 ("collect from the AITarget with the most idle own Darwinians") is the earlier reading that l.979 corrects; l.1422 marks it "P04 reading".
- (2) `07_ai.md` l.1012–1015 (pseudocode): "`enemies = GetEnemies(centre, 500)`; scan in grid order (§6.4) keeping a running nearest distance; the first enemy e that improves it AND has `GetNumEnemies(e.pos, 75) ≥ 20` → goal = e.pos; stop". `02_units_and_entities.md` l.1292 ("iterate nearer-first") is a summary of the same routine.

**Verdict.** ORIG settles both in favour of **G07**. (1) The ferry pickup is the first qualifying node in structure order. (2) The scan runs in query result order with a running nearest distance; an enemy is taken only if it is nearer than every enemy before it in that order and has the cluster, and the scan stops at the first such enemy. The two rules differ when a nearer enemy without a cluster comes earlier in the result order than a farther enemy with one: the running-nearest scan then skips the farther enemy.

**Replacements.**
- `reference/02_units/transport.md`, R-UNIT-407 item 2:
  > 2. While its team's Launch Vessel is in its loading state, the Transport ferries: it collects from the first bot navigation node, in structure order, at which its team has more than `ref.unit.transport.bot_ferry_min_idle` (10) idle Workers, that is more than `ref.unit.transport.bot_ferry_min_range` (400 wu) from the vessel and that has no other Transport of its team loading, or heading for an arrival point, within `ref.unit.transport.bot_ferry_exclusion` (100 wu) of it; when no node qualifies, it collects at its team's Transit Gate. It drops at the vessel, on a ring of `ref.unit.transport.bot_ring_radius` (100 wu) round it for the standard and hard bots.
- `reference/02_units/strike_team.md`, R-UNIT-360, first sentence:
  > Every `ref.unit.strike_team.bot_scan_ticks` (11 ticks), the bot Strike team takes the enemies within `ref.unit.strike_team.bot_scan_range` (500 wu) of its centre in the result order of the actor-grid query ([space, routing and physics](../12_simulation_model/05_space_routing_and_physics.md)), keeping a running nearest distance, and picks the first enemy that is nearer than every enemy before it and has at least `ref.unit.strike_team.bot_cluster_min` (20) enemies within `ref.unit.strike_team.bot_cluster_radius` (75 wu), counted with the cell-granular count `CountEnemiesInCells` (R-SIM-313, DEC-094); the scan stops at that enemy.
  The rest of R-UNIT-360 (stand-off move and fallback to the navigation node) stays.
- Not arbitrated here (wording, no ORIG question): G07's request to align R-UNIT-359 "acts directly and sends no commands" with R-BOT-014 belongs to the routing pass and to the Open DEC of item 50.

## A-14 Spawn Node census in R-BLDG-132 (item 61)

**Question.** R-BLDG-132 gives 286 Spawn Nodes (162 owned, 124 with no team), while reference/03 table 2.4-A gives 258 on 32 of the 49 match maps. Which count belongs in R-BLDG-132, and what is the owner split over the 49 match maps?

**Statements.**
- `reference/03_buildings_and_world_objects/economy_and_spawn_objects.md` **R-BLDG-132**: "Across the reference maps, 162 Spawn Nodes start owned by a map position (position 0: 53, position 1: 57, position 2: 33, position 3: 19) and 124 start with no team".
- `reference/03_buildings_and_world_objects/README.md` table 2.4-A, row Spawn Node: "258 (32)" (records on the reference maps, maps with at least one), with "reference maps" defined by the mode counts 9 + 9 + 9 + 8 + 7 + 7 = 49.
- `reference/08_maps_and_level_design/rules_and_conventions.md` **R-MAP-110**, table 2.11-A: 286 records in 34 files, "the 57 reference map files of R-MAP-085" (this table states its scope and is correct).

**ORIG evidence.**
- `03_buildings_and_world_objects.md` l.119 (type table): Spawn Node "32 (258)" on official maps, "2" development maps; l.746: "official maps … 1–15 per map".
- `08_levels_and_level_design.md` l.989 (type totals "over the 57 top-level files", l.970): "SpawnPoint | 286 | 34 | neutral 124, pos0 53, pos1 57, pos2 33, pos3 19"; `03_buildings_and_world_objects.md` l.745 gives the same split "across all `mp_*` files".
- `08_levels_and_level_design.md` l.361 (benchmark file): "5 SpawnPoint per position + 4 neutral" on a 4-position map (24 records); l.363 (sandbox file): "4 SpawnPoint (position 1)". 286 − 24 − 4 = 258, which matches the official count exactly, so the other non-match files hold none.
- X-ARB derivation for the 49 match maps: no team 124 − 4 = **120**; position 0: 53 − 5 = **48**; position 1: 57 − 5 − 4 = **48**; position 2: 33 − 5 = **28**; position 3: 19 − 5 = **14**; owned 138; total 258.

**Verdict.** ORIG settles it: R-BLDG-132 mixes the 57-file owner split into a rule about the reference maps, whose count is 258 on 32 maps. R-BLDG-132 takes the 49-map figures derived above; table 2.11-A of reference/08 keeps its 57-file figures, because it says so.

**Replacements.**
- `reference/03_buildings_and_world_objects/economy_and_spawn_objects.md`, R-BLDG-132:
  > **R-BLDG-132** (MUST) A Spawn Node's map record holds the base fields and its links (R-BLDG-108). The 49 reference match maps place 258 Spawn Nodes on 32 maps: 138 start owned by a map position (position 0: 48, position 1: 48, position 2: 28, position 3: 14) and 120 start with no team; none has its global flag set. A reference map that places Spawn Nodes places 1 to 15. The hint overlay explains Spawn Node capture with the picture `help.hint.spawn_nodes` ([11 Tutorial and help](../11_tutorial_and_help.md)).
- No change to reference/03 README or reference/08.

## A-15 Which reference maps use the special-lighting map effect (item 62)

**Question.** Is the special-lighting map effect used by three Zone Control maps (renderer) or by two Zone Control maps and one Spawn Control map (reference/08)?

**Statements.**
- `architecture/components/renderer/view_lighting_and_fog.md` **RND-229**: "… on maps whose map effects include special lighting (…; three reference Zone Control maps use it)".
- `reference/08_maps_and_level_design/rules_and_conventions.md` §5 summary table: "special lighting on 3 (Spawn Control 1, Zone Control 2), two of them together with snow"; map-effects table: special lighting 3.

**ORIG evidence.**
- `08_levels_and_level_design.md` l.711: the special-lighting effect is set on three files (the three file names are original map names and are not reproduced in NEXT).
- l.760–762 (per-map table): the first of the three has game type `Domination` (Spawn Control), with `snow SpecialLighting`; the second has `KingOfTheHill` (Zone Control), with `snow SpecialLighting`; the third has `KingOfTheHill` (Zone Control), with `SpecialLighting` only. (The first file's name suggests Zone Control, but its declared game type is Spawn Control, which is likely where the renderer statement came from.)

**Verdict.** ORIG settles it: **two Zone Control maps and one Spawn Control map** (reference/08 is right). RND-229 must change.

**Replacements.**
- `architecture/components/renderer/view_lighting_and_fog.md`, RND-229, the parenthesis: "([reference/08_maps_and_level_design/rules_and_conventions.md](../../../reference/08_maps_and_level_design/rules_and_conventions.md); three reference maps use it: two Zone Control maps and one Spawn Control map)". The rest of RND-229 stays.

## A-16 Number of levels of a guide-grid cell (item 77)

**Question.** Does a cell of a terrain tile's guide grid take 16 levels (one letter A to P) or 256 levels (one byte, written as two letters)?

**Statements.**
- `reference/08_maps_and_level_design/rules_and_conventions.md` **R-MAP-081**: "a guide grid of 2^p − 1 cells per side whose cells take one of `ref.map.terrain.guide_grid_levels` (16) levels, written A to P"; tuning row `ref.map.terrain.guide_grid_levels | 16 | levels | written A to P`.
- `architecture/components/terrain.md` **TER-003** ("a coarse guide grid of 8-bit heights") and the tile-record table, row 12: "*r* × *r* unsigned 8-bit values".

**ORIG evidence.**
- `23_terrain_and_landscape.md` l.182: the guide string has "2 letters per cell", "2·r² letters"; l.241: the pinned value is "the raw byte 0 – 255 used directly as a tile-space height"; l.360: all 402 guide strings in the map files are exactly 2r² letters long; l.364–368: "Each cell is two letters `hi lo`: value = 16·(hi − 'A') + (lo − 'A'). Legal letters are `A`–`P`, i.e. one hexadecimal nibble each … `AA` = 0, `AP` = 15, `BA` = 16, `PP` = 255".
- `08_levels_and_level_design.md` l.807 ("letters `A`–`P`") is the source of the 16-level reading: it names the alphabet, not the number of levels.

**Verdict.** ORIG settles it: a cell holds **256 levels, 0 to 255** (terrain.md is right). The letters A to P are the two hexadecimal digits of one byte in the reference's text form of a map; the Species map schema stores the value as an unsigned 8-bit number (terrain.md table row 12), so the letter form is not a rule of the reference map data and is dropped from R-MAP-081.

**Replacements.**
- `reference/08_maps_and_level_design/rules_and_conventions.md`, R-MAP-081, the guide-grid clause: "… guide-grid power p; and, when p > 0, a guide grid of 2^p − 1 cells per side whose cells hold one of `ref.map.terrain.guide_grid_levels` (256) levels, the values 0 to 255, each an absolute height in tile space ([terrain](../../architecture/components/terrain.md) TER-003)." The rest of R-MAP-081 stays.
- Same file, tuning row: `| ref.map.terrain.guide_grid_levels | 256 | levels | values 0 to 255, one unsigned byte per cell | R-MAP-081 | DEC-025 |`
- No change to terrain.md.

## A-17 Left and right on a cycle-mode drop-down (item 98, R-META-044)

**Question.** On a drop-down in cycle mode (the bot-difficulty control of the match setup page), do the menu actions left and right step through the values (R-META-044), or does the control change only when activated (UI framework)?

**Statements.**
- `reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md` **R-META-044**, widget row 3 (bot difficulty): "cycle mode, in which left and right step through the values".
- `architecture/components/ui_framework/layout_rendering_input_and_localisation.md` **UI-210**: "Left and right change only a numeric page text field that is the cursor (UI-120). A drop-down in cycle mode changes only on activate, to its next value (UI-116)"; binding table: `action.menu.left`, `action.menu.right` act on "numeric page text fields (UI-120)". `core_and_widgets.md` **UI-116**: in cycle mode a click selects the next option, wrapping.
- `ui_framework/pages_and_screen_recipes.md` UI-381 (C-UI-b) defers to C-UI-a.

**ORIG evidence.**
- `28_ui_framework.md` l.1186 (navigation rules): "**Left/Right** only change numeric `GameMenuInputField`s that are the cursor; drop-downs in cycle mode change only on Activate (next value) [C]"; l.1165 (bindings): "`MenuLeft` / `MenuRight` | … | `GameMenuInputField` number adjust"; l.720: the left and right handling is in the numeric field's update; l.711: the cycle-mode drop-down changes in its click handler ("in cycle mode select the next option (wrap to 0)").
- `09_meta_systems.md` l.537 (page widget table, the source of R-META-044): "cycle mode (left/right cycles values)", a screen-level description that the input code does not support.

**Verdict.** ORIG settles it: **the UI framework is right**. Left and right never change a cycle-mode drop-down; activating it (click or the menu activate action) steps to the next value, wrapping. R-META-044 must change.

**Replacements.**
- `reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md`, R-META-044, widget row 3, third cell: "0.66 × bh high; no background; cycle mode, in which each activation steps to the next value, wrapping from the last to the first; left and right do not change it ([UI framework](../../architecture/components/ui_framework/layout_rendering_input_and_localisation.md) UI-210)". The other cells stay.
- No other line of reference/09 says that left and right step a cycle-mode control (checked). No change to the UI framework.

## A-18 Is the cut to the player's start view made once, or again after a world reset? (item 103)

**Question.** The in-match loop cuts the camera to the local team's start view (`spawn_view_<n>`) when the local team first exists. Does this happen once per match (R-SIM-581) or also again after every reset that rebuilds the match world (R-CTRL-497, CAM-061)?

**Statements.**
- `reference/12_simulation_model/01_tick_timing_and_main_loop.md` **R-SIM-581** step 2: "In the first iteration in which the local team exists …, cut the camera to that team's camera view `spawn_view_<n>` … This happens once." Step 11.3: "if the match rules have requested a multi-round reset, reset the match world in place".
- `reference/06_controls_camera_and_commands/camera.md` **R-CTRL-497**: "As soon as the local team exists …, and again after every reset that rebuilds the match world, the camera cuts to `spawn_view_<n>`"; `architecture/components/camera_and_sequences.md` **CAM-061** follows R-CTRL-497.

**ORIG evidence.**
- `20_architecture.md` l.1118 (loop step 2): "once `m_location->GetMyTeam()` exists (first frame after team initialisation) → camera target mount `"player<N>"` … `CutToTarget()`, once"; l.1130 (loop step 11.3): "if `m_multiwinia->m_resetLevel` → `Multiwinia::ResetLocation()` (§8.4) **and re-arm step 2**".
- `06_player_control_and_input.md` l.1227: "as soon as the local team exists, `SetTarget("player<N>")` … and `CutToTarget()` (repeated after a level reset)".
- `01_game_modes.md` l.468: the reset rotates the start positions before the world is rebuilt, so after a reset n is the team's new start position.

**Verdict.** ORIG settles it, and the two sources agree: the cut happens **once per build of the match world**: once after the first team initialisation, and again after each in-place reset, which re-arms it. R-CTRL-497 and CAM-061 are right; R-SIM-581 step 2 ("This happens once") is incomplete and must say that step 11.3 re-arms it.

**Replacements.**
- `reference/12_simulation_model/01_tick_timing_and_main_loop.md`, R-SIM-581 step 2:
  > 2. **Player camera.** In the first iteration in which the local team exists (the first frame after team initialisation), cut the camera to that team's camera view `spawn_view_<n>`, where *n* is the team's start position. This happens once per build of the match world: the in-place reset of step 11.3 re-arms it, so the camera cuts again, to the view of the team's new start position, in the first iteration after the reset ([reference/06 camera](../06_controls_camera_and_commands/camera.md) R-CTRL-497).
- Same requirement, step 11.3: append "; this re-arms step 2".
- No change to camera.md or CAM-061.

## A-19 Order and size of a map's co-op group position lists (item 118, first conflict)

**Question.** Alliance assignment reads the map's group-1 and group-2 position lists by index (G1[0], G1[1], G2[0], G2[1]). The data schema derives these lists from the start-position records in ascending position order and limits each group to at most 2 positions; the reference reads them in the authored order. Does the derived order change any reference behaviour, and which statement stands?

**Statements.**
- `reference/01_skirmish_rules_and_modes/README.md` **R-MODE-020**: "`G1` and `G2` are the map's group-1 and group-2 position lists"; `AssignAlliance` moves a team to `G2[0]` or `G2[1]` (mirror `G1[0]`, `G1[1]`) when its group is full. It states neither the order nor a size limit.
- `architecture/components/data_schemas/map_schema.md` **DAT-088**: the group-1 and group-2 positions are derived "from the start positions in group `one` and group `two`, each list in ascending position order"; **DAT-087** with C-DAT's report: groups hold at most 2 positions.

**ORIG evidence.**
- `01_game_modes.md` l.251–262 (the alliance pseudocode, source of R-MODE-020): it indexes the parsed lists `CoopGroup1Positions` and `CoopGroup2Positions` as read from the map file, so the reference order is the authored order.
- `08_levels_and_level_design.md` l.721 and l.724–790 (per-map options, "values exactly as interpreted by `ParseMultiwiniaOptions`"): the reference maps that define groups list them as "0 / 1 2" (two 3-player Siege maps), "0 1 / 2 3" (two 4-player Siege maps) and "0 2 / 1 3" (one Spawn Control and one Zone Control 4-player map). **Every authored list is already in ascending order**, and none holds more than 2 positions.
- l.1879 (map editor, co-op group swatches): "a group holds ≤ 2 positions; a position cannot be in both".
- R-SIM-419 / `21_simulation_and_entity_model.md` l.1482: in Siege the group is chosen by membership only ("1 if the position is in `m_coopGroup1Positions`, else 2"), so order matters only outside Siege, on the two "0 2 / 1 3" maps.

**Verdict.** ORIG settles the reference content: on every reference map the authored order equals ascending order, so DAT-088's derivation reproduces every reference outcome of R-MODE-020 exactly, and the 2-position limit is the reference editor's own rule. Only a custom map authored in the reference text form with a non-ascending list would behave differently; Species' schema has no authored list order to preserve, so that case is a **design choice** (minor), for X-DEC-P4b to record with item 118. Recommendation: keep DAT-088 and DAT-087 as written, and make R-MODE-020 state the order and the limit so that both documents read the same.

**Replacements.**
- `reference/01_skirmish_rules_and_modes/README.md`, R-MODE-020, last sentence before the pseudocode:
  > `G1` and `G2` are the map's group-1 and group-2 position lists, each holding at most two start positions in ascending position order ([data schemas](../../architecture/components/data_schemas/map_schema.md) DAT-087, DAT-088); a position belongs to at most one group:
- Design-choice note for X-DEC-P4b (fold into the item-118 entry): "Co-op group lists are derived in ascending position order; the reference reads them in authored order, which is ascending on every reference map. Options: A — ascending order (default); B — keep an authored order field. Recommendation: A."

## A-20 Hive ants-inside value out of range: build error or run-time replacement (item 118, second conflict)

**Question.** A Hive's map record carries its starting ants inside. R-BLDG-633 replaces a value outside [0, 10,000] by 0 when the map loads; DAT-159 makes such a value a build error. Which rule stands?

**Statements.**
- `reference/03_buildings_and_world_objects/trees_and_crate_structures.md` **R-BLDG-633**: "a value outside [0, `ref.bldg.hive.map_ants_max`] (10,000) becomes 0, and the default is 0. A map may place a Hive; no reference map does."
- `architecture/components/data_schemas/map_structure_records.md` **DAT-159**: "A Hive's ants-inside value outside [0, `ref.bldg.hive.map_ants_max`] is a build error. The run-time replacement … that R-BLDG-633 states therefore never meets validated content; it remains the rule for any other creator."

**ORIG evidence.**
- `03_buildings_and_world_objects.md` l.1906 (Hive record): "numAntsInside | int, mandatory | 0 | starting ants; values outside [0, 10000] reset to 0".
- `08_levels_and_level_design.md` l.1111: the Hive is "absent from every shipped (retail) map"; l.1752: the map editor's field range is [0, 1000].
- ORIG has no content build step, so it says nothing about rejecting a value before load.

**Verdict.** ORIG settles the **reference rule**: the run-time replacement by 0, with the bound 10,000, as R-BLDG-633 states. It does not settle whether Species' content build rejects such a value: DAT-159 is a Species validation rule layered in front of the reference rule, and the two statements are compatible as DAT-159 words them. This part is a **design choice** for X-DEC-P4b (item 118). Recommendation: keep both; R-BLDG-633 gains a cross-reference so that a reader of the reference rule finds the build check.

**Replacements.**
- `reference/03_buildings_and_world_objects/trees_and_crate_structures.md`, R-BLDG-633:
  > **R-BLDG-633** (MUST) A Hive's map record holds the base fields and its ants inside, an integer; a value outside [0, `ref.bldg.hive.map_ants_max`] (10,000) becomes 0, and the default is 0. The content build rejects such a value in an authored map ([data schemas](../../architecture/components/data_schemas/map_structure_records.md) DAT-159), so the replacement applies only to records that bypass validation. A map may place a Hive; no reference map does.
- Design-choice note for X-DEC-P4b (fold into the item-118 entry): "An out-of-range Hive ants-inside value is a build error; the reference replaces it by 0 at load. Options: A — build error, run-time replacement kept for other creators (default); B — run-time replacement only. Recommendation: A."
- No change to DAT-159.

## A-21 Number of neutral gun turrets on a reference Siege map (found in report G03b, request to G01)

Not on the item list; G03b asked G01 to reconcile, and only ORIG can settle the number.

**Question.** How many neutral gun turrets does a reference Siege map place: 7 to 40 (siege.md) or 12 to 40 (G03b)?

**Statements.**
- `reference/01_skirmish_rules_and_modes/siege.md` **R-MODE-354**: "… Barrier Emitters, 7 to 40 neutral gun turrets per reference map, …".
- G03b report: "12 to 40 per Siege map (200 records on 13 maps)" (the 200 records are all match maps with turrets, not only Siege).

**ORIG evidence.**
- `08_levels_and_level_design.md` l.220–227 (per-map table of the eight Siege maps): turret counts 20, 12, 19, 18, 13, 40, 18 and 20 (minimum 12, maximum 40, 160 in total); l.228: the hidden stress-test file, which no player can select, has 7.
- `08_levels_and_level_design.md` l.604 and `01_game_modes.md` l.917 ("7–40 neutral `GunTurret`s") include that stress-test file; l.989 (owner census): every gun turret record has no team.

**Verdict.** ORIG settles it: **12 to 40** on the eight reference Siege maps; the 7 belongs to a file outside the reference map set. R-MODE-354 must change.

**Replacements.**
- `reference/01_skirmish_rules_and_modes/siege.md`, R-MODE-354: replace "7 to 40 neutral gun turrets per reference map" by "12 to 40 neutral gun turrets per reference map (160 on the eight maps)". The rest stays.

## A-22 Does the colour-row press save the settings file? (item 98, last point; report C-UI-b request 3)

Item 98 lists this point for the document fixers; it is a factual question that only ORIG settles, so it is arbitrated here.

**Question.** When the player presses a free colour row of the player options page outside Siege and co-op, the preferred colour is stored. Is the settings file also saved at that moment?

**Statements.**
- `reference/09_front_end_and_match_flow/options_localisation_and_hooks.md` **R-META-406**: "The store saves the settings file on each of these events, and on no other:" followed by six events, none of them the colour row.
- `reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md` **R-META-081**: the press "stores it in the preferred-colour setting" (silent on saving).
- `architecture/components/ui_framework/pages_and_screen_recipes.md` **UI-419**: "outside Siege and co-op it also stores that colour in the preferred-colour setting and saves the settings file".

**ORIG evidence.**
- `28_ui_framework.md` l.899 (colour-row button, click handler): "if not taken → `parent->m_currentRequestedColour = index`; outside Assault/co-op also save pref `PlayerPreferedColour` (+ `Save()`)".
- `09_meta_systems.md` l.1194 and l.2304 (screen-level description): "the pref `PlayerPreferedColour` is stored", written by the same click handler; it does not list the save, but does not exclude it.

**Verdict.** ORIG settles it: the press **stores and saves**. UI-419 is right; R-META-406's closed list misses the event, and R-META-081 should say so too.

**Replacements.**
- `reference/09_front_end_and_match_flow/options_localisation_and_hooks.md`, R-META-406: add item 7:
  > 7. a press of a free colour row of the player options page outside Siege and co-op, which stores the preferred colour ([screen_flow_and_match_setup.md](screen_flow_and_match_setup.md) R-META-081).
- `reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md`, R-META-081:
  > **R-META-081** (MUST) Pressing a free colour or side row sets the requested colour to its colour id and, outside Siege and co-op, stores it in the preferred-colour setting and saves the settings file ([options_localisation_and_hooks.md](options_localisation_and_hooks.md) R-META-406); it then plays a backwards transition and returns to the match setup page.

## A-23 Per-tick draws of the Wisp wobble (found in report G02b, request to C-SIM-c)

Not on the item list; G02b asked C-SIM-c to check a draw count that only ORIG settles.

**Question.** Does the Wisp's per-tick wobble make three draws `NextSigned(1)` (creatures.md), or three draws plus one `NextUnit(20)` (reference/12 table 1)?

**Statements.**
- `reference/02_units/creatures.md` **R-UNIT-596**: "each axis rate takes a cosmetic random walk of `FxRng::NextSigned`(`ref.unit.wisp.rate_walk`) (1) per tick" (three draws).
- `reference/12_simulation_model/07_randomness_and_integrity.md` table rows "snowflake and Wisp wobble | three `NextSigned(1)` and one `NextUnit(20)` per object per tick" (l.71 and l.138 of that file).

**ORIG evidence.**
- `22_determinism_and_networking.md` l.1193 (synced-draw catalogue, by function): "`snow.cpp:Snow::Advance`, `souldestroyer.cpp:Zombie::Advance` | `syncsfrand(1)` × 3 + `syncfrand(20)` wobble (no gameplay effect; snow unlogged) | every object every tick"; l.1051 repeats it ("`Snow`/`Zombie` wobble `syncsfrand(1)` × 3 + `syncfrand(20)` per object per tick").
- `02_units_and_entities.md` l.2273 (the Wisp's own table, source of creatures.md): "wobble like a spirit (rates random-walk `syncsfrand(1)` per tick in [0, 2] …)": it names the random walk and omits the fourth draw; ORIG does not say what the fourth draw feeds.

**Verdict.** ORIG settles the count: **three `NextSigned(1)` and one `NextUnit(20)` per Wisp per tick**, as for the snowflake. Reference/12 is right; creatures.md omits the fourth draw. Under DEC-013 these draws come from the cosmetic stream in Species, so the simulation is unaffected, but the reference draw count belongs in the rule.

**Replacements.**
- `reference/02_units/creatures.md`, R-UNIT-596, append: "Each tick the wobble also makes one further cosmetic draw `FxRng::NextUnit`(20), which has no gameplay effect, so the Wisp makes four cosmetic draws per tick, as the snowflake does ([randomness and integrity](../12_simulation_model/07_randomness_and_integrity.md))." The value 20 needs a key in creatures.md's tuning table (proposed `ref.unit.wisp.extra_draw_range`, `20`, note "fourth per-tick wobble draw; no gameplay effect").
- No change to reference/12.

## A-24 The tile writer's missing separator (found in report C-TER, request 4 to G08b)

Not on the item list; C-TER asked G08b to check a statement that only ORIG settles.

**Question.** R-MAP-387 says that the reference text writer puts no separator between a tile's lowland smoothing factor and its guide-grid power, "so that they can be read back only through fixed field widths". Is that right?

**Statements.**
- `reference/08_maps_and_level_design/map_editor_hooks.md` **R-MAP-387**: "it writes a tile's lowland smoothing factor and guide-grid power with no separator between them, so that they can be read back only through fixed field widths".
- C-TER (report): the writer uses fixed-width fields (lowland `%6.2f`, power `%6d`), so the two values are separated by padding unless a value fills its width.

**ORIG evidence.**
- `08_levels_and_level_design.md` l.1944 (tile writer formats): "… `%6.2f` `%6d` [`   %s` guide] — … `lowlandSmoothing` and `guidePower` are concatenated without a separator (works only thanks to the field widths)".
- `23_terrain_and_landscape.md` l.131: the reader splits tokens on spaces, tabs, line ends, colons and commas (it does not read fixed widths); l.181: the guide-grid power is 0 to 5, so its 6-character field always starts with at least 5 spaces.

**Verdict.** ORIG settles it, and C-TER is right in substance: the format string has no separator, but the power's right-aligned field width always pads it with spaces, and the reader splits on those spaces. The two values therefore read back as separate tokens; the reader does not use fixed widths. R-MAP-387's clause is misleading and is reworded; the data-loss list it belongs to is unchanged.

**Replacements.**
- `reference/08_maps_and_level_design/map_editor_hooks.md`, R-MAP-387, the clause: replace "it writes a tile's lowland smoothing factor and guide-grid power with no separator between them, so that they can be read back only through fixed field widths;" by "it writes a tile's lowland smoothing factor and guide-grid power with no separator between them, so that they read back as two values only because the power's field is 6 characters wide and its value (0 to 5) leaves leading spaces;". The rest of R-MAP-387 stays.

---

## Summary

| Entry | Item | Topic | Verdict | Settled by ORIG | Documents to change |
|---|---|---|---|---|---|
| A-1 | 20 | colour of a co-op group without a colour | white for the group shade request; the DEC-078 grey is the no-team colour (colour id none) | yes | reference/01 README (R-MODE-017, tuning note) |
| A-2 | 21 | elimination damage | 2 × current health | yes | reference/12 06 (R-SIM-407, tuning unit, AC-SIM-06-14) |
| A-3 | 25 | Builder orb destination | delivery to the nearest friendly Spawn Node | yes | reference/05 (R-ECON-082, event table, Figures 1 and 3, formula reference, §8), reference/00 glossary row, builder.md §8 title, DEC-085 (rewrite, Resolved) and the decisions index title |
| A-4 | 29 | door Worker airborne or grounded | airborne: the spawn hook grounds it, the door then clears the flag | yes | reference/05 (R-ECON-043, §2.21 step 2 sentence) |
| A-5 | 42 | Core Device countdown | 900 on seven maps, 1,200 on one 4-player map | yes | mode_objects.md (R-BLDG-317) |
| A-6 | 46, 49 | blight-tree differences | G03b right on all three (actual height, first tree in structure order, 101 ticks); R-CRATE-181 already corrected | yes | none |
| A-7 | 48.1 | Powers that test exclusion zones | G04's list | yes | defences_and_props.md (R-BLDG-464) |
| A-8 | 48.2 | rocket-turret rocket life | 11 ticks; related finding: the flame-turret fireball life is also 11 | yes | defences_and_props.md (R-BLDG-420 row, tuning rows); for the fireball also weapons_and_projectiles.md (R-CRATE-460, R-CRATE-461 with one new key, summary row) |
| A-9 | 48.3 | cancelled Transport | destroyed (−1000), except in a tutorial | yes | transport.md (R-UNIT-370, command row, edge case 8, §8 row) |
| A-10 | 48.4 | Strike team life | 601 ticks | yes | strike_team.md (R-UNIT-338, tuning row, edge case 2, AC-ST-01) |
| A-11 | 48.5 | bad-branch airstrike | unreachable; removed | yes | hazards_and_projectiles.md (R-UNIT-711 row 3, §1 row, §5 edge case) |
| A-12 | 52 | bot node and marker recount period | 20 ticks; DEC-064 does not apply | yes | reference/03 README (R-BLDG-003, table 2.7-A two rows) |
| A-13 | 54 | Transport ferry pickup; Strike team scan | first qualifying node in structure order; running-nearest scan in result order | yes | transport.md (R-UNIT-407 item 2), strike_team.md (R-UNIT-360 first sentence) |
| A-14 | 61 | Spawn Node census | 258 on 32 of the 49 match maps; owner split 120 / 48 / 48 / 28 / 14 | yes (derived) | economy_and_spawn_objects.md (R-BLDG-132) |
| A-15 | 62 | maps with special lighting | two Zone Control, one Spawn Control | yes | renderer view_lighting_and_fog.md (RND-229) |
| A-16 | 77 | guide-grid cell levels | 256 (0 to 255) | yes | reference/08 rules_and_conventions.md (R-MAP-081, tuning row) |
| A-17 | 98 | left and right on a cycle-mode drop-down | no effect; activation steps the value | yes | reference/09 screen_flow (R-META-044 row 3) |
| A-18 | 103 | start-view cut once or after resets | once per build of the match world; the reset re-arms it | yes | reference/12 01 (R-SIM-581 steps 2 and 11.3) |
| A-19 | 118.1 | co-op group list order and size | identical on every reference map; ascending order and at most 2 kept | reference content yes; custom maps **design choice** | reference/01 README (R-MODE-020 sentence); note for X-DEC-P4b |
| A-20 | 118.2 | Hive ants-inside out of range | reference rule is the run-time replacement; the build error is a Species check | reference rule yes; build check **design choice** | trees_and_crate_structures.md (R-BLDG-633 sentence); note for X-DEC-P4b |
| A-21 | report G03b | neutral gun turrets per Siege map | 12 to 40 (160 in total) | yes | siege.md (R-MODE-354) |
| A-22 | 98 | colour-row press saves the settings | stores and saves | yes | reference/09 options (R-META-406 item 7), screen_flow (R-META-081) |
| A-23 | report G02b | Wisp wobble draws per tick | three `NextSigned(1)` and one `NextUnit(20)` | yes | creatures.md (R-UNIT-596, one tuning key) |
| A-24 | report C-TER | tile writer separator | values separate through the power field's padding | yes | reference/08 map_editor_hooks.md (R-MAP-387 clause) |

Design choices for X-DEC-P4b: A-19 and A-20 (both item 118). DEC text for X-DEC-P4a: A-3 (DEC-085). No other entry needs a DEC change; A-12 means the proposed action of item 52 ("G07 states 21 with DEC-064") is not taken.
