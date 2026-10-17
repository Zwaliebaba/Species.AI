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

## A-25 Cadence of the damage flicker (coordinator request, VFX-126 against R-UNIT-051)

**Question.** Is a unit's damaged-flicker flag evaluated every render frame (R-UNIT-051) or once per tick period (VFX-126)?

**Statements.**
- `reference/02_units/README.md` **R-UNIT-051**: "each render frame it is drawn damaged when FxRng `NextUnit(ref.unit.flicker.chance_scale)` × (1 − |sin(presentation time + …)| × …) > its health fraction".
- `architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md` **VFX-126**: the flag is "evaluated once per `vfx.flicker.step` (one `ref.sim.tick_period`, 0.1 s of scaled presentation time), the cadence of the reference", with the sine of the unscaled presentation time.

**ORIG evidence.**
- `26_animation_and_vfx.md` l.505 (§4.6, routed to C-VFX): "`Entity::Advance` evaluates, every tick and **inside the sim step**, `m_renderDamaged = frand(0.75)·(1 − |sin(g_gameTime + uid·10)|·0.8) > health/maxHealth` … A full-health entity never flickers; at low health it flickers on most ticks … When the flag is set, the classes below distort their render matrix each frame".
- `02_units_and_entities.md` l.412 (§2, routed to G02a1): "`Entity::Advance` sets `m_renderDamaged` = …" (the per-tick advance, not the render); `21_simulation_and_entity_model.md` l.688: step (4) of the base per-tick advance.
- `26_animation_and_vfx.md` l.50: the time in the sine is the wall time, set every frame and not advanced while paused (unscaled by game speed).

**Verdict.** ORIG settles it: the flag is **evaluated once per tick**, and each render frame only reads it (and applies its own per-frame distortion). VFX-126 is right; R-UNIT-051 wrongly evaluates the expression every render frame and leaves the clock of the sine unstated.

**Replacements.**
- `reference/02_units/README.md`, R-UNIT-051:
  > **R-UNIT-051** (MUST) A unit flickers to show damage. Its damaged flag is evaluated once per tick period of scaled presentation time, the reference cadence of one evaluation per simulation tick, and not every render frame ([animation and VFX](../../architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md) VFX-126): the flag is set when FxRng `NextUnit(ref.unit.flicker.chance_scale)` × (1 − |sin(t + `ref.unit.flicker.phase_per_id` × unit id)| × `ref.unit.flicker.sine_weight`) > its health fraction, with t the unscaled presentation time, chance scale 0.75, phase 10 per unit id and sine weight 0.8. Every render frame draws the unit by its current flag. The flicker is presentation state: it draws only from the presentation generator, reads only the presentation clock, and no simulation rule reads it (DEC-007, DEC-013).
- No change to VFX-126.

## A-26 Does the Frenzy timer give the Fury damage factor? (route RA-066)

**Question.** While the Frenzy outcome's match-wide timer runs, does every Worker take 0.75 of its damage, as with a Fury link (worker.md), or only get the Fury combat behaviours (reference/04)?

**Statements.**
- `reference/02_units/worker.md` **R-UNIT-125**: "the Frenzy timer gives every other effect of Fury but no speed change. A Worker with Fury takes 0.75 of its damage (R-UNIT-116) …".
- `reference/04_supply_crates_powers_and_weapons/crates_and_powers.md` **R-CRATE-213**: under the Frenzy timer every Worker fires every tick with scatter, fires volleys four times as often and ignores projectile threats; "it does not get the Fury damage factor or the Fury speed bonus, which only a Fury link gives".

**ORIG evidence.**
- `04_crates_powerups_and_weapons.md` l.865 (Frenzy effect): while the timer is above 0 the raging test is true for every Worker of every team (fire every tick at a target jittered ±30, formation fire 4×, no flight from threats); "It does **not** give the Rage damage reduction or speed bonus (those depend on `m_rageTaskId`)", citing the Worker's health-change function.
- l.886 (Fury link): speed × 1.5, "damage taken × 0.75", and the raging behaviours.
- `02_units_and_entities.md` l.872 ("Rage → × 0.75") and l.881 (a summary row that lists the Frenzy timer under the Fury row with every Fury effect) do not separate the two paths.

**Verdict.** ORIG settles it: the damage factor follows the **Fury link only**, not the Frenzy timer (reference/04 is right). ORIG does not say whether the Frenzy timer also drives the Fury glow and size pulse, so R-UNIT-125 keeps its presentation clause as it stands.

**Replacements.**
- `reference/02_units/worker.md`, R-UNIT-125:
  > **R-UNIT-125** (MUST) **Fury.** A Fury Power links the Workers it reaches, and every Worker has the Fury behaviours while the Frenzy outcome's match-wide timer runs ([reference/04](../04_supply_crates_powers_and_weapons/crates_and_powers.md) R-CRATE-213). On linking a Fury Power, a Worker's speed stat is multiplied by `ref.unit.worker.fury_speed_factor` (1.5) when its current speed ÷ its stat-table speed is below `ref.unit.worker.fury_cap_ratio` (3), giving 30, 45 and then 67.5 wu/s, and the Worker takes 0.75 of its damage (R-UNIT-116); the Frenzy timer gives neither the speed change nor the damage factor. A Worker with Fury, from a link or from the Frenzy timer, ignores projectile threats (R-UNIT-098 step 3), fires every tick with scatter and no reload (R-UNIT-103), fires denser formation volleys (R-UNIT-103), and is drawn pulsing in size (R-UNIT-147). When the link is cleared, the speed returns to the stat-table speed.
- Same file, R-UNIT-116 step 4: "With a Fury link: amount × `ref.unit.worker.fury_damage_factor` (0.75); …" (the step now says "With Fury", which reads as including the Frenzy timer).
- Closes: RA-066.

## A-27 The Worker's stand distance at a Flag post (route RA-073)

**Question.** R-UNIT-095 lets a Worker stay put when it is within "the Flag post capture range (20 wu)", while reference/03 gives the Flag post's capture radius as 40 wu. Is the 20 wu value the capture radius?

**Statements.**
- `reference/02_units/worker.md` **R-UNIT-095** (Flag search): "If the Worker is within the Flag post capture range (20 wu, [reference/01](…flag_chain.md)), its waypoint is its own position".
- `reference/03_buildings_and_world_objects/mode_objects.md` (Flag post recount, step 2): counts the Workers within `ref.bldg.flag_post.capture_radius_wu` (40 wu, not its size).

**ORIG evidence.**
- `02_units_and_entities.md` l.627 (Worker constants): "`BLITZKRIEG_FLAGCAPTURE_RANGE` (external) | 20.0", defined with the mode object; l.796 (flag search): "first non-own, unlocked `MultiwiniaZone` with distance < `zone->m_size`; waypoint = stay if < 20, else zone ± `SFRAND(20)` (±10)".
- `03_buildings_and_world_objects.md` l.1507 (the Flag post recount): "count Darwinians within **40** units (not `m_size`)", a literal; no capture rule uses the 20.0 constant.

**Verdict.** ORIG settles it: the two values are different. **40 wu** is the Flag post's capture radius (reference/03 is right). **20 wu** is a separate constant that only the Worker's flag search reads, as its stand distance; ORIG names it a capture range, but no capture rule uses it. R-UNIT-095 must stop calling it the capture range and key it.

**Replacements.**
- `reference/02_units/worker.md`, R-UNIT-095, second sentence:
  > If the Worker is within `ref.unit.worker.flag_stand_range` (20 wu) of the Flag post, its waypoint is its own position; otherwise the Flag post's position + (`NextSigned(20)`, 0, `NextSigned(20)`), each offset in (−10, 10) wu. This stand distance is not the Flag post's capture radius (40 wu, [reference/03](../03_buildings_and_world_objects/mode_objects.md)).
- Same file, new tuning row: `| ref.unit.worker.flag_stand_range | 20 | wu | flag search: closer than this, the Worker stays where it is | R-UNIT-095 | — |`
- No change to reference/03 or flag_chain.md.
- Closes: RA-073.

## A-28 Number of match option definitions (route RA-324)

**Question.** The overview gives 39 match option definitions; reference/01 gives 38. Which is the reference count?

**Statements.**
- `reference/00_reference_overview.md` **R-VIS-019** and tuning row `ref.vis.scope.option_definitions` (39).
- `reference/01_skirmish_rules_and_modes/README.md` **R-MODE-073**: "The six mode schemas hold 38 options."

**ORIG evidence.**
- `01_game_modes.md` l.535 (heading): "Every option definition (39 = 38 shipped + 1 debug)"; l.537: "8 + 8 + 5 + 2 + 6 + 9 = **38** parameters in the six shipped blueprints plus 1 in [the debug-only mode] = **39**".

**Verdict.** ORIG settles it: **38** options in the six in-scope modes; the 39th belongs to the debug-only mode, which the library drops (item 22). R-VIS-019 must use 38 (DEC-017's own count is X-DEC-P4a's item 22).

**Replacements.**
- `reference/00_reference_overview.md`, R-VIS-019: "The match options comprise `ref.vis.scope.option_definitions` (38) option definitions, those of the six mode schemas ([01](01_skirmish_rules_and_modes/README.md) R-MODE-073), plus the lobby settings and the per-map overrides for reinforcements, Transports, turrets, instant capture, Beam Dish rules, the Visitor faction, the airstrike threshold, excluded Supply crate outcomes and population caps (DEC-017, DEC-073)."
- Same file, tuning row: `| ref.vis.scope.option_definitions | 38 | match option definitions | the six mode schemas (R-MODE-073); plus the lobby settings and the per-map overrides | R-VIS-019 | DEC-017, DEC-073 |`
- Closes: RA-324.

## A-29 Hostile outcomes and the map's excluded outcomes (route RA-117, owner decision O18)

**Question.** R-VIS-124 says hostile outcomes ignore a map's excluded-outcome list; Resolved DEC-107 (owner decision O18) says every outcome, hostile ones included, respects it. Which statement stands?

**Statements.**
- `reference/00_reference_overview.md` **R-VIS-124**: "A map can exclude outcomes, and hostile outcomes ignore a map's exclusions (DEC-107)"; §8 row: "DEC-107 | Hostile outcomes: ownership and exclusion filtering".
- `decisions/verdicts_and_fixes.md` **DEC-107** (Resolved, O18): "Every Supply crate outcome, hostile ones included, respects the map's list of excluded outcomes"; `reference/01` R-MODE-084 already states this rule.

**ORIG evidence.**
- `04_crates_powerups_and_weapons.md` l.337 (bad pool): "**`IsValid`/`InvalidCrates` are not consulted** — the AntNest limit and per-level exclusions do not apply to bad outcomes". This is the reference behaviour that R-VIS-124 states.

**Verdict.** ORIG confirms that R-VIS-124 describes the reference behaviour, but the question is not one ORIG decides: owner decision O18 is fixed and DEC-107 applies it, so the documents state the decided rule. **DEC-107 stands**; R-VIS-124 must change. The overview makes no statement about Brood ownership, so DEC-121 needs no citation there.

**Replacements.**
- `reference/00_reference_overview.md`, R-VIS-124, first sentence: "A map can exclude outcomes, and every outcome, hostile ones included, respects the map's exclusions (DEC-107)." The rest stays.
- Same file, §8 row: `| DEC-107 | Hostile outcomes: team-blind hazards; every outcome respects the map's exclusions | R-VIS-124 |`
- Closes: RA-117.

## A-30 When the second tutorial switches basic outcomes on (route RA-030)

**Question.** Does the second tutorial switch "basic outcomes only" on "when its bot phase starts" (reference/04) or at its end step (reference/01, reference/11)? Are these different moments?

**Statements.**
- `reference/04_supply_crates_powers_and_weapons/README.md` **R-CRATE-031**: "the second tutorial switches it on when its bot phase starts".
- `reference/01_skirmish_rules_and_modes/README.md` **R-MODE-083** and `reference/11_tutorial_and_help.md` **R-TUT-117** / AC-TUT-08: at the end step basic crates only is switched on, crate placement becomes random, the bot difficulty becomes Standard and the bots are released.

**ORIG evidence.**
- `30_scripting_tutorial_and_cutscenes.md` l.631 (second tutorial, end row): "`HelpTutorial2End` …; `m_basicCratesOnly = true`; `SetGameOption(7, 0)` …; `m_aiType = AITypeStandard`; `AICanSpawn = true`", all in the one end step.
- l.470 (overrides table): random crate drops are off "before End" and on "after End (and `m_basicCratesOnly = true`, CrateDropMode 0)", where l.466 defines "T2 before End" as "`!m_tutorial2AICanSpawn`", that is, before the bots may spawn.

**Verdict.** ORIG settles it: the switch happens **at the end step**, the same step that releases the bots, so "when its bot phase starts" means that moment. R-CRATE-031 is reworded to name it.

**Replacements.**
- `reference/04_supply_crates_powers_and_weapons/README.md`, R-CRATE-031, last clause: "…; the second tutorial switches it on at its end step, the same step that releases its bots ([reference/11](../11_tutorial_and_help.md) R-TUT-117)."
- Closes: RA-030.

## A-31 When a Transport frees its sea routes (route RA-093)

**Question.** R-SIM-357 says that only a Transport frees routes, its own sea routes, but no document says when (on a new route, on arrival or on removal). Does ORIG give the trigger?

**Statements.**
- `reference/12_simulation_model/05_space_routing_and_physics.md` **R-SIM-357**: simulation routes stay until the match world is destroyed; "only a Transport frees routes, its own sea routes"; the default of the Open DEC-651.
- `reference/02_units/transport.md`: no trigger stated.

**ORIG evidence.**
- `21_simulation_and_entity_model.md` l.1203: "`DeleteRoute(id)` frees a slot; it is called only by [the local route preview] … and by `Armour` for sea routes [C] grep of `DeleteRoute`"; l.923: simulation routes are never deleted.
- `02_units_and_entities.md` l.1364 and `07_ai.md` l.974: a Transport creates a route only when a bot order sets its conversion point, which routes directly when the line is clear and otherwise through the sea routing system; player orders never create a route.
- No ORIG line says at which moment the Transport calls the delete.

**Verdict.** ORIG does **not** settle the trigger: it records only that a Transport deletes its own sea routes. **Design choice** for X-DEC-P4b (fold into DEC-651). Recommendation: the Transport frees its previous sea route when it creates a new route for itself, and frees its current sea route when it is removed. That keeps at most one live sea route per Transport, and it is the only reading under which the delete has any effect, since a bot order is the only place a Transport creates a route.

**Replacements.**
- `reference/12_simulation_model/05_space_routing_and_physics.md`, R-SIM-357, append: "The reference does not define the moment at which a Transport frees one of its sea routes; the default is that a Transport frees its previous sea route when it creates a new route for itself, and its current sea route when it is removed (DEC-651)."
- Design-choice note for X-DEC-P4b (DEC-651): "Trigger of a Transport's sea-route freeing: unspecified in the reference. Options: A — free the previous sea route on each new route and the current one on removal (default); B — never free. Recommendation: A."
- Closes: RA-093 (once X-DEC-P4b records the default).

## A-32 Initialisation draws of a bot controller (route RA-219, the draw-count point)

**Question.** R-SIM-626 counts 4 initialisation draws for a bot controller; reference/07 R-BOT-009 makes exactly 2 (the Beam Dish phase, then the main-cycle phase). Which count is right?

**Statements.**
- `reference/12_simulation_model/07_randomness_and_integrity.md` **R-SIM-626**: "the initial values of a bot controller (4)", listed apart from "the rate that an actor draws when its kind is set, which a Worker draws once per spawn"; the reference note: "a Worker draws its rate twice per spawn".
- `reference/07_bots/README.md` **R-BOT-009**: "exactly two draws in this order: the Beam Dish phase … and then the main-cycle phase"; **R-BOT-008**: no placement search and no draw.
- `reference/02_units/README.md` **R-UNIT-015**: setting an actor's kind draws its initial reload timer once per spawn.

**ORIG evidence.**
- `07_ai.md` l.189 (bot controller constructor body): "`SetType(TypeAI)`; … `m_radarReposTimer = syncfrand(10)`; `m_timer = syncfrand(20)` (Easy) / `syncfrand(10)` (Standard) / `syncfrand(1)` (Hard …)". These are the two own-value draws, Beam Dish (radar) phase first, plus the rate draw of setting the kind.
- `02_units_and_entities.md` l.231: setting the kind "also sets `m_reloading = syncfrand(Rate)`"; a constructor that sets the kind is followed by the spawn routine setting it again, so such a spawn draws the rate twice; l.2401, l.2403: the two phase draws.
- `22_determinism_and_networking.md` l.1055: "`AI::AI` (4 draws)". This total is 1 rate draw in the constructor, the 2 phase draws, and 1 rate draw when the spawn sets the kind again; it does not include the spawn-position search.

**Verdict.** ORIG settles it: the bot controller's own initial values take **2 draws** (Beam Dish phase, then main-cycle phase), as R-BOT-009 says. The reference's 4 also counts the rate draw of setting its kind twice, which R-SIM-626 already lists separately as the per-kind rate draw. R-SIM-626 must give 2 for the bot controller and extend its reference note to the bot controller. R-BOT-009 gains the order relative to the rate draw.

**Replacements.**
- `reference/12_simulation_model/07_randomness_and_integrity.md`, R-SIM-626: "the initial values of a bot controller (4)" becomes "the initial values of a bot controller (2: the Beam Dish phase, then the main-cycle phase; [../07_bots/README.md](../07_bots/README.md) R-BOT-009)"; and the last sentence ends "…, so each map-placed bot objective marker draws twice at load, and a Worker and a bot controller each draw their rate twice per spawn."
- `reference/07_bots/README.md`, R-BOT-009, first sentence: "In the initialisation step that creates the bot controller (DEC-652), after the rate draw that setting its kind makes ([02 Units](../02_units/README.md) R-UNIT-015), its state takes the initial values of table 2.3-A, with exactly two draws in this order: …". The rest stays.
- Not arbitrated (no ORIG question; routing keeps them): RA-219's other points, R-SIM-690 "issues no command", R-SIM-628 (the reference search that DEC-087 replaces) and R-SIM-663.
- Closes: the draw-count point of RA-219.

## A-33 Three Power-rule readings between reference/04 and reference/06 (route RA-166)

**Question.** (1) Under the tutorial stop block, does a one-shot Power stay Running or Waiting after it fires? (2) Which grant sources does the received-Power pop-up record? (3) Does a Power's fixed map marker vanish 8 s after it is first seen, or after 8 s of on-screen time?

**Statements.**
- `reference/04_supply_crates_powers_and_weapons/README.md` **R-CRATE-021** ("A one-shot Power then stays Running after it fires and can be fired again"), **R-CRATE-023** (source "Supply crate, starting powers or Comeback Bonus"), **R-CRATE-028** item 2 ("disappears `ref.power.marker.lifetime_s` (8 s of real time) after the local player first sees it") and the tuning note of `ref.power.marker.lifetime_s` ("from the first time the local player sees the marker").
- `reference/06_controls_camera_and_commands/selection_and_power_slots.md` **R-CTRL-210** step 3 (every cancel is refused, including the self-ending of one-shot Powers, which can therefore be fired again), **R-CTRL-186** step 6 (sources "a Supply crate, a reinforcement or the Comeback Bonus"); `cursor_and_markers.md` **R-CTRL-381**, fixed row ("removed once its on-screen time reaches `ref.power.marker.lifetime_s` after it was seen").

**ORIG evidence.**
- (1) `06_player_control_and_input.md` l.524: in the tutorials the delete is refused, "so one-shot tasks (Nuke, Infection, forests, AntNest, Plague, Eggs, Meteor) whose `Target` relies on self-termination remain `StateStarted` and **can be fired again**; AirStrike sets Running first and is removed by `Advance` when its effect object disappears"; l.392: a one-shot type goes from Started straight to removal. Started is the Species Waiting state (R-CRATE-025).
- (2) `06_player_control_and_input.md` l.318: the pop-up source record has "`m_source` (0 crate, 1 reinforcement, 2 retribution)"; l.418: every grant pushes source 0; `05_economy_and_population.md` l.600 tags Transit Gate reinforcement grants with source 1; `04_crates_powerups_and_weapons.md` l.508 and l.519 tag Comeback Bonus grants with source 2. Starting powers are granted through the same grant path and keep source 0.
- (3) `06_player_control_and_input.md` l.1062 (fixed marker): "removed when `m_onscreenTimer ≥ 8 s` once seen"; l.1075: "on-screen accumulates `m_onscreenTimer`; seen after 1 s". `04_crates_powerups_and_weapons.md` l.200 ("disappears 8 s after first being seen") is a summary of the same rule.

**Verdict.** ORIG settles all three for **reference/06**: (1) the one-shot Power stays **Waiting**; the airstrike (both bomb kinds) becomes Running and ends when its effect is gone. (2) The sources are **Supply crate, reinforcement and Comeback Bonus**; a starting power records the Supply crate source. (3) The marker goes when its **accumulated on-screen time** reaches 8 s; it is tested only once it has been seen, and time off screen does not count. (The Sickness entry of the ORIG list is never a Power, A-7, so it is left out of the Species list.)

**Replacements.**
- `reference/04_supply_crates_powers_and_weapons/README.md`, R-CRATE-021:
  > **R-CRATE-021** (MUST) During the first tutorial, and during the second tutorial until its bot is allowed to spawn, no Power can be stopped. A one-shot Power whose effect ends it at once (Missile Barrage, Swarm, Essence Grove, Blight Grove, Hive, Brood and meteor shower) therefore stays Waiting after it fires and can be fired again; the airstrike Power (both bomb kinds) is unaffected by the block, because it becomes Running when it fires and ends when its effect is gone ([reference/06](../06_controls_camera_and_commands/selection_and_power_slots.md) R-CTRL-210). This is the reference default of DEC-550.
- Same file, R-CRATE-023, the parenthesis: "(a Supply crate, a reinforcement or the Comeback Bonus; a starting power records the Supply crate source)", and add "([reference/06](../06_controls_camera_and_commands/selection_and_power_slots.md) R-CTRL-186)".
- Same file, R-CRATE-028 item 2: "**Map marker**: a fixed marker at the effect's position with the outcome's icon, in the colour of the team the effect belongs to; it is removed when its accumulated on-screen time for the local player reaches `ref.power.marker.lifetime_s` (8 s of real time), tested once the player has seen it, so time off screen does not count ([reference/06](../06_controls_camera_and_commands/cursor_and_markers.md) R-CTRL-381)."
- Same file, tuning row `ref.power.marker.lifetime_s`, notes: "presentation; accumulated on-screen time, tested once the local player has seen the marker".
- No change to reference/06.
- Closes: RA-166.

## A-34 Which commands boost the selection arrows (route RA-200, point b)

**Question.** R-SIM-405 step 6 has selecting a unit or structure boost the cursor's selection arrows; reference/06 has only `SelectPower` do it (R-CTRL-200, R-CTRL-375). Which commands boost?

**Statements.**
- `reference/12_simulation_model/06_teams_and_player_commands.md` **R-SIM-405** step 6: selecting emits presentation events for the team's own participant, including "the cursor arrows grow (×2.0 …)".
- `reference/06_controls_camera_and_commands/selection_and_power_slots.md` **R-CTRL-200** and `cursor_and_markers.md` **R-CTRL-375**: "Executing `SelectPower` emits a presentation event that boosts the selection arrows by `ref.ctrl.cursor.selection_boost`"; R-CTRL-375 adds that a match draws no selection arrows, so the boost has no visible effect.

**ORIG evidence.**
- `06_player_control_and_input.md` l.1046: "`BoostSelectionArrows(s)` …; called with 2.0 by `TaskManager::SelectTask` **and** `Team::SelectUnit` for the local team [C] — arrows are drawn only by the [single-player] render path, so in [a match] the call has no visible effect"; l.467: the Power selection calls it.

**Verdict.** ORIG settles it: **both** commands boost, selecting a unit or structure (R-SIM-405) and selecting a Power (R-CTRL-200). Both documents are partly right; R-CTRL-375 must name both. R-SIM-405 stays.

**Replacements.**
- `reference/06_controls_camera_and_commands/cursor_and_markers.md`, R-CTRL-375, first sentence: "Executing `Select` ([Teams and player commands](../12_simulation_model/06_teams_and_player_commands.md) R-SIM-405) and executing `SelectPower` (R-CTRL-200) each emit a presentation event that boosts the selection arrows by `ref.ctrl.cursor.selection_boost`." The rest stays.
- No change to R-SIM-405 or R-CTRL-200.
- Closes: point (b) of RA-200 (the other points of RA-200 are alignment fixes with no ORIG question).

## A-35 Maximum length of a camera view name: 62 or 63 characters (route RB-204)

**Question.** R-MAP-125 limits a camera view name to `ref.map.camera.view_name_max_chars` (62) characters. C-CAM reports that the reference holds 63 characters in a 64-byte buffer. Which is the limit?

**Statements.**
- `reference/08_maps_and_level_design/rules_and_conventions.md` **R-MAP-125** and tuning row `ref.map.camera.view_name_max_chars` (62); R-MAP-127 applies the same limit to camera animation names.
- `reference/08_maps_and_level_design/map_editor_hooks.md`: the editor's name field accepts `ref.map.editor.camera_view.name_field_chars` (63), "one more than" the 62 limit; under DEC-801 a longer name is refused, never truncated (R-MAP-370).
- C-CAM report: "the data model gives camera-view and animation names at most 63 characters in a 64-byte buffer".

**ORIG evidence.**
- `30_scripting_tutorial_and_cutscenes.md` l.830: "`m_name` | ≤ 63 characters (`CAMERA_MOUNT_MAX_NAME_LEN`), `strncpy` into `char[64]`"; `08_levels_and_level_design.md` l.1856: the editor field edits "`m_name` (char[64], `CAMERA_MOUNT_MAX_NAME_LEN` 63)".
- `08_levels_and_level_design.md` l.1125 (the map reader): "name copied with `strncpy` into `char[64]` (keep ≤ 62 characters, a 63+ name is not NUL-terminated)".

**Verdict.** ORIG settles it: the declared length is 63, but the map reader copies at most 63 characters without adding a terminator, so the longest name the reference reads back correctly is **62**. R-MAP-125's 62 is the reference's working limit and stands. The editor field's 63, already keyed separately, is the declared length, and under DEC-801 that 63rd character is refused. The coordinator's alternative (raise the key to 63) is not taken. Only the tuning note changes, so that a reader sees why 62 and 63 both appear.

**Replacements.**
- `reference/08_maps_and_level_design/rules_and_conventions.md`, tuning row: `| ref.map.camera.view_name_max_chars | 62 | characters | the longest name the reference reads back intact; its name field holds 63 (map editor hooks) | R-MAP-125 | DEC-801 |`
- No change to R-MAP-125, R-MAP-127 or map_editor_hooks.md.
- Closes: RB-204.

## A-36 The reference map census recounted over the 49 match maps (route RA-221)

**Question.** reference/08 states several field and team distributions over all 57 reference map files (the 49 match maps plus 8 development, test and cut files). What are the figures over the 49 match maps only?

**Statements.**
- `reference/08_maps_and_level_design/rules_and_conventions.md`: **R-MAP-110** with table 2.11-A (owner field per structure kind, 57 files); **R-MAP-200** with table 2.11-B (57-file field distributions of the bot structures); **R-MAP-111** item 3 (Barrier Emitter modes and turret types "over the reference map files"); **R-MAP-128** (camera views "over the 57 reference map files"); **R-MAP-086** (terrain asset use per map file); **R-MAP-109** (Population Limiter radius and maximum "on the reference map files").

**ORIG evidence and method.** The 49-map figure of each kind is the 57-file figure minus the eight non-match files. `03_buildings_and_world_objects.md` l.80–155 (type table, columns "Official MP maps (lines)" and "Dev mp_ maps") gives the 49-map record and map counts of every kind. The owner field of the eight files' records comes from:
- `08_levels_and_level_design.md` l.358–366 (contents of each non-match file);
- l.228–276 (master table);
- l.567 (the cut map's zones with owners);
- l.598 (the stress file's Core Device and consoles);
- `05_economy_and_population.md` l.393, l.394, l.408, l.436 (per-file Transit Gate and Spawn Node owners);
- `07_ai.md` l.346 and l.1548–1590 (navigation nodes per file).

The camera views and terrain assets were recounted file by file from ORIG 08's per-file camera-mount table (l.1136–1194) and landscape table (l.833–903), with the helper script X-ARB + `_census.py`, which first reproduces the 57-file figures of R-MAP-128 and R-MAP-086 exactly. Where ORIG gives no owner or field value for a non-match file's records, the 49-map split cannot be derived, and the entry says so.

**Verdict.** ORIG settles the figures below. Table 2.11-A, the R-MAP-200 field column and the parts of R-MAP-111 item 3, R-MAP-109 and R-MAP-111 item 5 that cannot be split stay at their stated 57-file scope, which is correct as written. Every derivable 49-map figure is given here, so that reference/08 can state the census of record over the 49 match maps.

Table A-36-1. Owner field over the 49 match maps (records, maps, owner split). "= 57" means that the eight non-match files hold no record of the kind, so the 57-file row of table 2.11-A is also the 49-map row.

| Structure kind | 49 maps | Owner field | Non-match records removed |
|---|---|---|---|
| bot objective | 92, 8 maps | not derivable (3 records of unknown owner) | 3 (stress file) |
| bot objective marker | 330, 15 maps (Siege 240, Launch Race 90) | Launch Race 90: position 0: 32, 1: 32, 2: 22, 3: 4; Siege 240: not derivable (10 records of unknown owner) | 10 (stress file) |
| bot navigation node | 1,029, 49 maps | not derivable (the 11 records owned by position 1 are not located) | 83 (stress 30, benchmark 13, cut map 35, sandbox 5) |
| Command Console | 33, 9 maps | no team 30; position 0: 3 | 3, no team (stress file) |
| volcano vent, Launch Vessel, fuel pump, fuel line, fuel depot, jump pad, grid splitter, Beam Dish, exclusion zone, smoke column, scenery mesh, Breach Charge | = 57 | = 57 | none |
| turret | 200, 13 maps | no team 200 | 7, no team (stress file) |
| Barrier Emitter | 677, 14 maps | not derivable (26 records of unknown owner) | 26 (stress file) |
| Control Zone (all its kinds) | 152, 25 maps | no team 105; position 0: 17, 1: 17, 2: 9, 3: 4 | 24 (benchmark 15 no team; cut map 6 no team and one each at positions 0, 1, 2) |
| Core Device | 8, 8 maps | position 0: 5, 1: 2, 2: 1 | 1, position 1 (stress file) |
| grid pylon | 296, 16 maps | not derivable (5 records of unknown owner) | 5 (stress file) |
| grid terminal | 86, 16 maps | no team 86 | 2, no team (stress file) |
| Solar Array | 225, 16 maps | not derivable (6 records of unknown owner) | 6 (stress file) |
| Relay | 261, 21 maps | not derivable (the 11 records owned by position 1 are not located) | 15 (sandbox 11, benchmark 4) |
| Spawn Node | 258, 32 maps | no team 120; position 0: 48, 1: 48, 2: 28, 3: 14 (A-14) | 28 |
| Source Node | 32, 32 maps | not derivable (the record owned by position 1 is not located) | 2 (sandbox, benchmark) |
| Population Limiter | 86, 21 maps | no team 86 | 1, no team (benchmark) |
| tree | 661, 39 maps | not derivable (the 51 records owned by position 1 are not located) | 94 (benchmark 18, sandbox 2, cut map 15, the three arena files 18, 18, 23) |
| Transit Gate | 109, 28 maps | no team 10; position 0: 27, 1: 43, 2: 22, 3: 7 | 14 (stress position 0: 5, 1: 2; benchmark 4 no team; cut map one each at positions 0, 1, 2) |
| wall | 323, 9 maps | no team 323 | 33, no team (stress 30, sandbox 3) |

Other 49-map figures:
- **Turret types** (R-MAP-111 item 3): gun type 189 and flame type 11, on the 49 maps (ORIG 03 l.898: every flame turret is on a match map, and the stress file's 7 are of the gun type). Barrier Emitter modes: not derivable (the stress file's 26 records).
- **Camera views** (R-MAP-128): over the 49 match maps `spawn_view_0` is defined in 48 maps, `spawn_view_1` in 48, `spawn_view_2` in 29, `spawn_view_3` in 12 and `default_view` in 5.
- **Terrain assets** (R-MAP-086): over the 49 match maps the land colour ramps are used by 6 maps (the map default), 10, 8, 5, 5, 4, 3, 3, 1, 1, 1, 1 and 1, and one ramp by no match map. The shore-wave gradients are used by 11 (the map default), 10, 10, 8, 4, 3 and 3, and one gradient by no match map. The sea tiles are used by 24 (the map default), 18, 5 and 2. (Asset consequence for X-AST: one land colour ramp and one shore-wave gradient of the manifest are used only by non-match files.)
- **Bot structure fields** (table 2.11-B, 57-file column): not derivable for the 83 navigation nodes, 3 objectives and 10 markers of the non-match files, so that column keeps its 57-file scope.
- **Population Limiters** (R-MAP-109): the upper ends of the stated ranges, 1,169.84 wu and 1,200, are those of the benchmark file's limiter (ORIG 08 l.361). The 49-map maxima are not stated in ORIG, so R-MAP-109 must name its scope.

**Replacements.**
- `reference/08_maps_and_level_design/rules_and_conventions.md`, R-MAP-110, second sentence: "The records, maps and owner field over the 49 match maps are table 2.11-C, and the per-mode counts are [reference/03](../03_buildings_and_world_objects/README.md) table 2.4-A; table 2.11-A covers the 57 reference map files of R-MAP-085, without the records of kinds that only campaign content uses."
- Same file, new table after table 2.11-A, "Table 2.11-C Owner field of the structure records of the 49 match maps.", with the columns `Structure kind | Records | Maps | Owner field: no team, then per start position`. It has one row per kind of table A-36-1, using the 49-map values above. Where a split is "not derivable", the row reads "not recorded for the match maps alone; see table 2.11-A". Where a row is "= 57", it repeats the table 2.11-A row.
- Same file, R-MAP-111 item 3: "… turrets, every one with no team, of the gun type on 189 records and the flame type on 11 on the match maps (196 and 11 over the reference map files) …"; the Barrier Emitter mode figures keep "over the reference map files".
- Same file, R-MAP-128, last sentence: "On the 49 match maps `spawn_view_0` is defined in 48 maps, `spawn_view_1` in 48, `spawn_view_2` in 29, `spawn_view_3` in 12 and `default_view` in 5; over the 57 reference map files the counts are 51, 51, 31, 13 and 7."
- Same file, R-MAP-086, the counts sentence: "On the 49 match maps the number of maps that use each asset is: land colour ramps 6 (the map default), 10, 8, 5, 5, 4, 3, 3, 1, 1, 1, 1, 1 and 0; shore-wave gradients 11 (the map default), 10, 10, 8, 4, 3, 3 and 0; sea tiles 24 (the map default), 18, 5 and 2. Over the 57 reference map files they are 10, 10, 8, 5, 5, 5, 3, 3, 2, 2, 1, 1, 1 and 1; 17, 11, 10, 8, 4, 3, 3 and 1; and 30, 20, 5 and 2." (The list order follows the manifest's file order as R-MAP-086 does now; the fixer keeps each count with its asset.)
- Same file, R-MAP-109, the range sentence: "Over the 57 reference map files their radius lies between 272.77 and 1,169.84 wu and their maximum population between 100 and 1,200; the largest of both belongs to a development file."
- Closes: RA-221, for every figure that ORIG allows to be derived; the rest stays at 57-file scope, as stated.

## A-37 Cliff push and separation push between actors (route RB-004, items 1 and 2)

**Question.** The C-SIM-a report lists two movement constants for reference/12: a cliff push (slope above 1.0, spiral search 0 to 50 wu in 0.2 wu steps) and a separation push between actors (radius 2.0 wu, force 0.1 ÷ distance). reference/02 R-UNIT-041 says that actors never push each other apart and never push themselves away from cliffs. Which holds in the reference?

**Statements.**
- `reference/02_units/README.md` **R-UNIT-041**: "Actors never push each other apart and never push themselves away from cliffs; no separation step exists (… R-SIM-360)".
- C-SIM-a report (request to C-SIM-b): the cliff push and the separation push as constants whose rules belong in reference/12 05.

**ORIG evidence.**
- `21_simulation_and_entity_model.md` l.1287: "`Entity::PushFromCliffs(pos, oldPos)` | slope … > 1.0 → spiral `d = 0..50` step 0.2 … | none in Multiwinia [B:LEGACY]"; l.1288: "`Entity::PushFromEachOther(pos)` | grid neighbours within 2.0 … × 0.1 / |pos − other| … | only `LaserTrooper` [B:LEGACY]; commented out in `Entity::Advance`"; l.1933: "`Entity::PushFromCliffs` (no caller) and `PushFromEachOther` (LaserTrooper only)" are legacy.
- `02_units_and_entities.md` l.361: both "exist but have no live caller in Multiwinia".
- The constants table at l.1703–1704 of ORIG 21, the source of C-SIM-a's list, records the literals without their reachability.

**Verdict.** ORIG settles it: **R-UNIT-041 holds**. Both pushes are code with no live caller (the only user of the separation push is a campaign unit), so they are not reference rules, and reference/12 gets no rule or key for them. RB-004 items 1 and 2 are closed with no change. Items 3 to 6 of RB-004 are not ORIG questions and stay with the routing pass.

**Replacements.** None. R-UNIT-041 and R-SIM-360 stand.
- Closes: RB-004 items 1 and 2.

## A-38 How a turret draws its ports (route RA-145)

**Question.** R-BLDG-046 says that the turret draws its ports in its own way, with status sockets `status_01` to `status_04` on its mount, but no document states that way. What does the reference draw?

**Statements.**
- `reference/03_buildings_and_world_objects/README.md` **R-BLDG-046**: the base port drawing (a pad mesh below structure detail 3 and a 6 wu status star at the pad's `status` socket, green when occupied, red when empty). "The Spawn Node …, the Solar Array (R-BLDG-183), the Command Console … and the turret, whose mount carries the status sockets `status_01` to `status_04` …, draw their ports in their own way."
- `reference/03_buildings_and_world_objects/defences_and_props.md`: no turret port-drawing rule.

**ORIG evidence.**
- `03_buildings_and_world_objects.md` l.372: the base port drawing is overridden by "SpawnPoint, GunTurret, SolarPanel, ControlStation" (and two campaign classes); l.536: the turret base mesh carries "MarkerStatus01–04 (code requests 5; #5 NULL, harmless)"; l.865: "Ports / status markers | 4 / 5 (4 present in the shape)".
- `25_textures_and_assets.md` l.1355: the turret base mesh's markers are four port markers and four status markers.
- `24_rendering.md` l.1090: the turret's alpha drawing is "lights, ports"; no line of ORIG states the colours, sizes or pad use of the turret's own port drawing. The nearest analogue, the Solar Array, draws "status lights green/red" at its own status markers (ORIG 03 l.1065).

**Verdict.** ORIG settles only the anchor: the turret draws its port status at the four status sockets of its own mount, not at the pads' status sockets. It does not record the look, so that part is a **design choice** for X-DEC-P4a (the gameplay-gap file, or DEC-959 "renderer values the reference does not record"). Recommendation: the base look of R-BLDG-046 (pad mesh below structure detail 3, a 6 wu `fx.starburst` star, green when the port is occupied and red when empty), with star i drawn at the mount's socket `status_0i` instead of the pad's socket. This is the reading that changes only what ORIG shows the turret changing, and it matches the Solar Array's green and red status lights.

**Replacements.**
- `reference/03_buildings_and_world_objects/defences_and_props.md`, new rule in the turret presentation section (next free id of the file's block):
  > **R-BLDG-4NN** (MUST) A turret draws its four ports with the base port look of [README](README.md) R-BLDG-046, the pad below structure detail `ref.bldg.port.pad_detail_limit` and a status star green when the port is occupied and red when it is empty, except that the status star of port i (1 to 4) sits at the turret mount's socket `status_0i` instead of the pad's `status` socket (see DEC-NNN).
- `reference/03_buildings_and_world_objects/README.md`, R-BLDG-046, the turret clause: "… and the turret, whose mount carries the status sockets `status_01` to `status_04` ([defences and props](defences_and_props.md) R-BLDG-4NN), draw their ports in their own way."
- Design-choice note for X-DEC-P4a: "Turret port look: the reference anchors the status lights at the mount's four status sockets but records no colours or pad use. Options: A — the base port look re-anchored (default); B — status stars only, without pads. Recommendation: A."
- Closes: RA-145 (with the DEC id filled in).

## A-39 Width of an actor's stats (R-SIM-145 against R-SIM-707 and DEC-090)

**Question.** R-SIM-145 says actor stats are unsigned 8-bit values; R-SIM-707 and Resolved DEC-090 say floating point. What does the reference store, and which statement belongs in the library?

**Statements.**
- `reference/12_simulation_model/03_entity_model_and_identity.md` **R-SIM-145**: "Actor stats are unsigned 8-bit values (DEC-241)."
- `reference/12_simulation_model/07_randomness_and_integrity.md` **R-SIM-707**: "Actor stats are floating-point values, so no stat is truncated to a byte and no speed overflows (DEC-090)."
- `reference/02_units/README.md` **R-UNIT-011**: stats are floating-point values, speed multipliers apply exactly (DEC-090); R-UNIT-013: the Abductor's speed is the explicit 244.
- `decisions/verdicts_and_fixes.md` **DEC-090** (Resolved): floating-point stats; option B is the reference behaviour of 8-bit stats.

**ORIG evidence.**
- `02_units_and_entities.md` l.228–231 and l.484: each entity copies its three stats into "`unsigned char m_stats[3]`"; values are "truncated to integers and limited to 0–255"; speed multipliers applied to the byte truncate; the Abductor's listed speed of 500 becomes 244.
- l.237: "New-game recommendation: store stats as floats per instance", which DEC-090 applies.

**Verdict.** ORIG settles the reference: it stores **unsigned 8-bit** stats per actor. But ORIG also recommends the fix, and DEC-090 is Resolved, so under the library's rules the documents state the fixed rule: **floating-point stats** (R-SIM-707, R-UNIT-011). R-SIM-145 states the superseded reference behaviour, and its citation of DEC-241 is wrong (DEC-241 is the Open entry on numeric semantics, not on stat width). R-SIM-145 changes.

**Replacements.**
- `reference/12_simulation_model/03_entity_model_and_identity.md`, R-SIM-145, first sentence: "Actor stats are floating-point values ([randomness and integrity](07_randomness_and_integrity.md) R-SIM-707; DEC-090)." The second and third sentences stay.
- No change to R-SIM-707, R-UNIT-011 or DEC-090.

## A-40 The Crawler trail texture: 32 × 512 or 160 × 512 (route RB-044)

**Question.** RND-320, AST-029 and the manifest row `sprite.crawler.segment.trail` give a 32 × 512 px column holding 18 shrinking copies of the segment art. C-SIM-c's input gives "160 × 512, 32 copies, glow 128". What does the reference use?

**Statements.**
- `architecture/components/renderer/entity_and_overlay_renderers.md` **RND-320**: a column `rnd.crawler.trail_column_width` (32 px) by `rnd.crawler.trail_column_height` (512 px), copy i being (32 − ⌊i ÷ 2⌋) px square "while the next copy fits", "that is 18 copies, 504 px in all. The glow strip samples `fx.glow_round`."
- C-SIM-c input row (via FX-08 §6 item 1): "160 × 512, 32 copies, glow 128".

**ORIG evidence.**
- `24_rendering.md` l.1176 and `26_animation_and_vfx.md` l.627: the reference builds one runtime atlas, "a 160×512 bitmap cleared black, the left 32-px column filled with 32 successively smaller copies of [the segment sprite] (copy i is `32 − i/2` px square, stacked downward while they fit in 512), and [the round glow texture] blitted 128×128 at (32, 0)".
- X-ARB replay: with copy sizes 32 − ⌊i ÷ 2⌋, 18 copies fit (504 px); the 19th would end at 534 px. The loop bound is 32, and the fit test stops it at 18.
- `species_work/asset_origin_map.csv` row 21: `fx.glow_round` recreates the same round glow texture.

**Verdict.** ORIG settles it, and the library is right. The reference's 160 × 512 image is two parts in one atlas: the 32 px trail column (RND-320's texture) and, to its right, the round glow copied at 128 × 128 px. "32 copies" is the loop bound, of which 18 fit. "Glow 128" is the size at which the glow is copied into the atlas, not a required size of `fx.glow_round`. Species keeps the two parts as separate assets, which changes no sampled texel of the trail and only the source resolution of the glow strip. RB-044 is closed with no change. One clarifying sentence may be added to RND-320 so that a later reader does not raise the question again.

**Replacements.**
- `architecture/components/renderer/entity_and_overlay_renderers.md`, RND-320, append: "The reference builds the column and a 128 × 128 px copy of the round glow side by side in one 160 × 512 px image; Species keeps them as the two assets named here."
- No change to AST-029 or the manifest.
- Closes: RB-044.

## A-41 Ownership lights: which clock, and what size (FX-08 §6 item 3)

**Question.** RND-167 and R-BLDG-045 give the same ownership-light values under two key sets, but they count different clocks: RND-167 the command-frame number (R-SIM-531), R-BLDG-045 "the number of the last completed tick". They also describe the quad size differently. Which are the reference values?

**Statements.**
- `architecture/components/renderer/world_renderers.md` **RND-167**: lights shown in "the command frames whose number n (R-SIM-531) satisfies ⌊(n mod `rnd.structure.light.blink_cycle` (10)) ÷ `rnd.structure.light.blink_frames` (2)⌋ = the team's index"; 10 nested quads of "half-size `rnd.structure.light.max_half_size` (6 wu) × i ÷ 10 for i from 0 to 9". Keys `rnd.structure.light.*` (DEC-958 item 1 cites them).
- `reference/03_buildings_and_world_objects/README.md` **R-BLDG-045**: shown "on ticks where ⌊(t mod `ref.bldg.structure.light_cycle_ticks`) ÷ `ref.bldg.structure.light_ticks_per_team`⌋ equals the team slot, with t the number of the last completed tick"; 10 nested quads "up to `ref.bldg.structure.light_size_wu` (6 wu) across".

**ORIG evidence.**
- `24_rendering.md` l.1083: "shown only when `(m_lastValidSequenceIdFromServer % 10) / 2 == teamId` (or editing) … each team's lights are on for 2 of every 10 server sequence ids … **team id 5 never shows ownership lights** …; 10 nested camera-facing quads of half-size `6·i/10` (i = 0..9)"; `03_buildings_and_world_objects.md` l.371 and l.2473 give the same rule.
- `22_determinism_and_networking.md` l.101 and l.115: `m_lastValidSequenceIdFromServer` is the latest frame received, against `g_lastProcessedSequenceId`, the last frame processed. The pacing controller (R-SIM-531) runs on the difference between the two.

**Verdict.** ORIG settles both points.
- (1) The clock is the number of the **latest command frame received** from the host (R-SIM-531's "latest frame received"). It runs ahead of the last completed tick by the pending frames. Neither document says "received": RND-167 is closer, and R-BLDG-045 is wrong.
- (2) The quads have **half-size 6 wu × i ÷ 10** (i = 0 to 9), so the largest is 10.8 wu across. RND-167 is right, and R-BLDG-045's "up to 6 wu across" is wrong.

The values 10 and 2 agree. On the duplicate keys: the reference value belongs to the reference document under the one-key rule, so the recommendation is that RND-167 and DEC-958 item 1 cite the `ref.bldg.structure.light_*` keys and the four `rnd.structure.light.*` rows are removed. That is a routing choice for the coordinator, not an ORIG question.

**Replacements.**
- `reference/03_buildings_and_world_objects/README.md`, R-BLDG-045:
  > **R-BLDG-045** (MUST) When a structure has a team and light sockets, the translucent pass draws at each light `ref.bldg.structure.light_quads` (10) nested additive camera-facing quads of `fx.starburst` in the team colour, quad i (0 to 9) of half-size `ref.bldg.structure.light_size_wu` (6 wu) × i ÷ 10. They are drawn only while ⌊(n mod `ref.bldg.structure.light_cycle_ticks`) ÷ `ref.bldg.structure.light_ticks_per_team`⌋ equals the team slot, with n the number of the latest command frame the participant has received ([reference/12](../12_simulation_model/01_tick_timing_and_main_loop.md) R-SIM-531), which can run ahead of the last completed tick; with the keys 10 and 2, teams 0 to 4 flash in 2 of every 10 command frames, and team slot 5 never flashes. In the map editor they are always drawn. The Barrier Emitter draws no lights.
- Same file, tuning rows: `ref.bldg.structure.light_size_wu` note "largest half-size; quad i has 6 × i ÷ 10"; `ref.bldg.structure.light_cycle_ticks` and `ref.bldg.structure.light_ticks_per_team` unit "command frames".
- `architecture/components/renderer/world_renderers.md`, RND-167, the clock clause: "… only in the command frames whose number n, the latest command frame received (R-SIM-531), satisfies …". If the coordinator takes the key recommendation: RND-167 cites `ref.bldg.structure.light_cycle_ticks`, `ref.bldg.structure.light_ticks_per_team`, `ref.bldg.structure.light_quads` and `ref.bldg.structure.light_size_wu`; the four `rnd.structure.light.*` rows are removed; and DEC-958 item 1 cites the same `ref.bldg` keys.
- Closes: FX-08 §6 item 3.

## A-42 Spawn Node glow: opacity, timer and start colour (FX-08 §6 item 4)

**Question.** RND-169 and R-BLDG-131 (with R-ECON-025) describe the Spawn Node's glow blobs with different values: opacity (occupied ports ÷ ports, against occupied ÷ 5), timer (from `rnd.structure.spawn.glow_timer_start` (30), against "300 to 0") and start colour (two keys with the same value). Which are the reference values?

**Statements.**
- `architecture/components/renderer/world_renderers.md` **RND-169** item 4: "alpha = occupied ports ÷ ports, and a colour that moves linearly from `rnd.structure.spawn.glow_start_colour` (100, 100, 100, alpha 200 of 255) to the team colour as the Spawn Node's activation timer runs from `rnd.structure.spawn.glow_timer_start` (30) down to 0".
- `reference/03_buildings_and_world_objects/economy_and_spawn_objects.md` **R-BLDG-131** and `reference/05_population_flow.md` **R-ECON-025**: opacity "(occupied ports ÷ 5)", colour from `ref.econ.spawn_node.glow_start_rgba` (100, 100, 100, 200) to the owner's colour "as the activation delay runs from 300 to 0" (R-ECON-025: "as (remaining delay / 300) goes from 1 to 0").

**ORIG evidence.**
- `03_buildings_and_world_objects.md` l.743 (Spawn Node visuals): "20 / 10 / 3 glow blobs … alpha = occupied/total, colour lerps from (100,100,100,200) to the team colour as `m_activationTimer` goes 30 → 0".
- The activation delay is 30.0 s, which is 300 ticks under DEC-002 (`ref.econ.spawn_node.activation_ticks`, reference/05); a Spawn Node has 5 ports (`ref.econ.spawn_node.port_count`).

**Verdict.** ORIG settles it, and the values **agree**. "Occupied ÷ total" is "occupied ÷ 5" for a Spawn Node. The timer's 30 is seconds of the activation delay, that is 300 ticks, and the start colour is the same RGBA. RND-169's formulation is the less correct one: its timer key holds a seconds value, which the integer-tick model (DEC-002) does not allow, and its colour key duplicates the reference key. RND-169 therefore changes to cite the reference keys, and its two duplicate keys are removed. R-BLDG-131 and R-ECON-025 stand.

**Replacements.**
- `architecture/components/renderer/world_renderers.md`, RND-169 item 4:
  > 4. glow blobs above the structure with `fx.glow_cloudy`: `ref.bldg.spawn_node.glow_blobs_by_detail` (20, 10 and 3 at structure detail 1, 2 and 3; [economy_and_spawn_objects.md](../../../reference/03_buildings_and_world_objects/economy_and_spawn_objects.md) R-BLDG-131), with alpha = occupied ports ÷ `ref.econ.spawn_node.port_count` (5), and a colour that moves linearly from `ref.econ.spawn_node.glow_start_rgba` (100, 100, 100, 200) to the team colour as the remaining activation delay runs from `ref.econ.spawn_node.activation_ticks` (300 ticks) down to 0 ([reference/05](../../../reference/05_population_flow.md) R-ECON-025).
- Same file, tuning table: remove the rows `rnd.structure.spawn.glow_start_colour` and `rnd.structure.spawn.glow_timer_start`, and list the three reference keys among the keys cited.
- Closes: FX-08 §6 item 4.

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
| A-25 | coordinator | damage-flicker cadence | once per tick period; frames only read the flag | yes | reference/02 README (R-UNIT-051) |
| A-26 | RA-066 | Frenzy and the Fury damage factor | only a Fury link gives the factor | yes | worker.md (R-UNIT-125, R-UNIT-116 step 4) |
| A-27 | RA-073 | Worker stand distance at a Flag post | 20 wu stand distance, separate from the 40 wu capture radius | yes | worker.md (R-UNIT-095, new key) |
| A-28 | RA-324 | match option definitions | 38 in scope (39 with the debug-only mode) | yes | reference/00 (R-VIS-019, tuning row) |
| A-29 | RA-117 | hostile outcomes and map exclusions | owner decision O18 (DEC-107) stands; ORIG gives the superseded default | owner decision | reference/00 (R-VIS-124, §8 row) |
| A-30 | RA-030 | second tutorial switches basic outcomes on | at the end step, which also releases the bots | yes | reference/04 README (R-CRATE-031) |
| A-31 | RA-093 | trigger of a Transport's sea-route freeing | not stated in ORIG | **design choice** | reference/12 05 (R-SIM-357 sentence); note for X-DEC-P4b (DEC-651) |
| A-32 | RA-219 | bot controller initialisation draws | 2 own draws after the kind's rate draw; the reference total of 4 counts the rate twice | yes | reference/12 07 (R-SIM-626), reference/07 README (R-BOT-009) |
| A-33 | RA-166 | three Power rules | stays Waiting; sources crate, reinforcement, Comeback Bonus; accumulated on-screen time | yes | reference/04 README (R-CRATE-021, R-CRATE-023, R-CRATE-028, tuning note) |
| A-34 | RA-200 (b) | commands that boost the selection arrows | both `Select` and `SelectPower` | yes | reference/06 cursor_and_markers.md (R-CTRL-375) |
| A-35 | RB-204 | camera view name limit | 62 (the declared 63 is read without a terminator) | yes | reference/08 rules_and_conventions.md (tuning note only) |
| A-36 | RA-221 | census over the 49 match maps | derivable figures given; the rest stay at 57-file scope | yes, partly | reference/08 rules_and_conventions.md (R-MAP-110 and a new table 2.11-C, R-MAP-111 item 3, R-MAP-128, R-MAP-086, R-MAP-109) |
| A-37 | RB-004 (1, 2) | cliff push and separation push | no live caller; R-UNIT-041 holds | yes | none |
| A-38 | RA-145 | turret port drawing | anchored at the mount's four status sockets; look not recorded | anchor yes; look **design choice** | defences_and_props.md (new rule), reference/03 README (R-BLDG-046); note for X-DEC-P4a |
| A-39 | coordinator | actor stat width | 8-bit stats in the reference; Resolved DEC-090 applies floating point, which the documents state | yes (fixed rule) | reference/12 03 (R-SIM-145) |
| A-40 | RB-044 | Crawler trail texture | the 32 px column of a 160 px atlas whose other part is the glow at 128 px; library right | yes | renderer entity_and_overlay_renderers.md (RND-320, one sentence) |
| A-41 | FX-08 §6.3 | ownership lights | clock: latest command frame received; quads of half-size 6 × i ÷ 10 | yes | reference/03 README (R-BLDG-045, tuning notes), renderer world_renderers.md (RND-167); key duplication for the coordinator |
| A-42 | FX-08 §6.4 | Spawn Node glow | values agree; RND-169 cites the reference keys in ticks | yes | renderer world_renderers.md (RND-169 item 4, two rows removed) |

Design choices for X-DEC-P4b: A-19 and A-20 (both item 118) and A-31 (DEC-651); for X-DEC-P4a: A-38 (turret port look). DEC text for X-DEC-P4a: A-3 (DEC-085). No other entry needs a DEC change; A-12 means the proposed action of item 52 ("G07 states 21 with DEC-064") is not taken.
