# Creatures

> Status: Reference · Prefix: R-UNIT · Related: [units](README.md), [hazards and projectiles](hazards_and_projectiles.md), [Worker](worker.md), [Essence orbs](essence_orbs.md), [Collector](collector.md), [Builder](builder.md), [crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md), [defences and props](../03_buildings_and_world_objects/defences_and_props.md), [population flow](../05_population_flow.md), [simulation model](../12_simulation_model/README.md)

## 1 Purpose and player experience

Creatures are the wildcards of a match. Supply crate outcomes release them, and none of them is under a player's direct control: a creature acts on its own rules from the moment it hatches. Together they are a second, hostile economy beside the Workers. Crawlers ferry Essence orbs into Crawler eggs and breed; Brood Drifters and spiders lay eggs near orbs; centipedes eat orbs and grow; Hive ants carry orbs and captured Workers home; and the Reaper Serpent destroys the Essence orb of everything it kills, so those Workers never return to anyone's economy.

This document also holds the rules that every creature and hazard shares (§2.1). The event hazards (the Abductor, the airstrike with its Bombers, the Missile Barrage and the meteor shower) are specified in [hazards and projectiles](hazards_and_projectiles.md). The stats, resistances, death rules, slope factors and tint kind of every kind are in the [units README](README.md); this document gives their behaviour. The Summoner, the Tri-walker and the Rift Horror are not part of the reference (DEC-062).

Table 1-A Creatures.

| Species term | Type | Family | Created in a match by | Owner | Section |
|---|---|---|---|---|---|
| Seed Pod | `SeedPod` | actor | Spitter Plants, the Brood outcome, a bursting Seed Pod | the launching team | §2.2 |
| Crawler egg | `CrawlerEgg` | actor | Brood Drifters, spiders, Seed Pods | the layer's team | §2.3 |
| Crawler, Crawler group, Crawler trail sample | `Crawler`, `CrawlerGroup`, `CrawlerTrailSample` | actor, actor group, record | Seed Pods, Crawler eggs, Swarm streaks; a Crawler group only by map placement | the source's team | §2.4 |
| Brood Drifter | `BroodDrifter` | actor | Seed Pods | the Seed Pod's team | §2.5 |
| centipede | `CentipedeCreature` | one actor per segment, in an actor group | Seed Pods; growth | the Seed Pod's team | §2.6 |
| ant | `AntCreature` | actor | Hives, the Hive outcome, Seed Pods | the Hive's or the Seed Pod's team | §2.7 |
| spider, spider gait | `SpiderCreature`, `SpiderGait` | actor, record | Seed Pods, Swarm streaks | the source's team | §2.8 |
| procedural leg, procedural foot | `ProceduralLeg`, `ProceduralFoot` | presentation helpers | eight per spider | — | §2.9 |
| Reaper Serpent | `ReaperSerpent` | actor | Seed Pods | always the Rogue faction | §2.10 |
| Wisp | `Wisp` | effect | a Worker killed by a Reaper Serpent or by a Blight Grove tree | the victim's team | §2.11 |

How players meet them:

- **Swarm pressure.** Crawlers arrive in tens, bite for 20 health per tick on contact and breed through eggs, up to 250 per team.
- **Big threats.** A spider pounces from 150 wu and crushes a cluster; a centipede of up to 20 segments crushes everything it brushes and splits in two when a middle segment dies; the Reaper Serpent dives through crowds and kills almost anything it touches.
- **Breeders.** Brood Drifters and spiders turn floating Essence orbs into Crawler eggs, so a battlefield left full of orbs breeds Crawlers.
- **Thieves.** Hive ants run out, grab Essence orbs and single Workers, and carry them home.
- **Ghosts.** Workers that a Reaper Serpent kills leave pale Wisps that drift away over ten minutes.

## 2 Rules

### 2.1 Rules shared by every creature and hazard

**R-UNIT-500** (MUST) The values of §4 are the reference tuning baseline (DEC-004), and the rules of this document and of [hazards and projectiles](hazards_and_projectiles.md) are the specification of record for creatures and hazards (DEC-005). Every reference quirk that they keep is an explicit rule with a named key (DEC-006). The roster of Table 1-A and of the hazards is kept with its behaviour (DEC-019).

**R-UNIT-501** (MUST) The stats of each kind are the keys `ref.unit.<kind>.health`, `ref.unit.<kind>.speed` and `ref.unit.<kind>.rate` of the actor stat table in the [units README](README.md) (R-UNIT-011), floating-point values to which every multiplier applies exactly (DEC-090), at difficulty level 0 (R-UNIT-016, DEC-010). Resistances and death rules are table 2.8-A of the README (R-UNIT-027), and every creature contact attack is an unresistable health change (R-UNIT-030). A rule below that replaces or scales a stat says so.

**R-UNIT-502** (MUST) The tick rules of the README (R-UNIT-005, R-UNIT-006) apply: dt in a formula is `ref.sim.tick_period` (0.1 s); a speed of v wu/s moves v × dt per tick; a rule stated per tick applies once per tick and scales with game speed through tick scheduling (DEC-039, DEC-092). A random duration is base + ⌈`SimRng::NextUnit`(spread)⌉ ticks, with base and spread in ticks, drawn at the step that starts it. A fixed duration is the reference tick count of its key (DEC-002).

**R-UNIT-503** (MUST) Every draw written `SimRng::NextUint`, `SimRng::NextUnit(r)` or `SimRng::NextSigned(r)` is a simulation draw with the forms of [reference/12](../12_simulation_model/07_randomness_and_integrity.md) R-SIM-605: `NextUnit(r)` is uniform over [0, r) and `NextSigned(r)` over [−r/2, r/2). A vector draw takes x, then y, then z, and each draw is its own statement. The draws that initialise a new object happen once, in the step that spawns it (DEC-652). Every draw that these documents mark *cosmetic* is a presentation draw from `FxRng` and never reaches simulation state (DEC-013, R-SIM-622).

**R-UNIT-504** (MUST) Every rejection loop of these documents has a capped number of attempts and a deterministic fallback (DEC-014): the Abductor's arrival and wander searches and the barrage launcher's sea search ([hazards and projectiles](hazards_and_projectiles.md)). The Brood kind choice and the nested Seed Pod kind choice (§2.2) draw directly from their candidate lists. A **cell fallback** is the qualifying cell centre of the terrain height field nearest to the last drawn point, with ties going to the lowest row-major cell index. The content validator runs every such search on every map at build time and rejects a map on which a search has no qualifying cell.

**R-UNIT-505** (MUST) Meshes, cards, colours, particles, shatter effects and sounds are presentation. The simulation emits a presentation event at the tick where a rule fires, and no simulation rule reads presentation state (DEC-007). The tint kind of each mesh is table 2.12-A of the README (R-UNIT-046). Sounds name their asset ids; a cue that these documents call silent has no sound (DEC-565).

**R-UNIT-506** (MUST) Which creature kinds a map may place as instant units is R-UNIT-002 of the README. A map-placed centipede is always inside an actor group (§2.6), and a map may place Crawlers inside a Crawler group (§2.4).

**R-UNIT-507** (MUST) A creature that dies through a death rule that releases an Essence orb releases exactly one, of its own team, unless the team is the Rogue faction, whose Essence orb spawns are always refused (R-UNIT-038, DEC-084, [reference/12](../12_simulation_model/03_entity_model_and_identity.md) R-SIM-126). Its corpse then fades by `ref.unit.corpse.fade_ground_per_tick` on the ground and `ref.unit.corpse.fade_air_per_tick` in the air (R-UNIT-034).

**R-UNIT-508** (MUST) The hostile Supply crate outcomes and the Hive creatures belong to the Rogue faction (team slot `ref.sim.team.rogue_slot`, 4), and the Abductor and the Workers it drops to the Visitor faction (`ref.sim.team.visitor_slot`, 5). Neither faction can win a match or block an elimination win (DEC-082), and each runs the full bot controller (DEC-558).

**R-UNIT-509** (MUST) A creature's enemy search uses the actor-grid queries of [reference/12](../12_simulation_model/05_space_routing_and_physics.md) with the ranges and exclusions stated for it, and no other exclusion; in particular it does not exclude a bot controller (DEC-311).

**R-UNIT-510** (MUST) A Seed Pod burst, a Crawler egg hatch and the Brood outcome check no population cap: they add their creatures and Workers whatever the team's count. Only the Crawler limit `ref.unit.crawler.team_limit` (R-UNIT-529) limits Crawlers (DEC-453, [population flow](../05_population_flow.md)).

### 2.2 Seed Pod

A Seed Pod is a timed pod that tumbles, bounces and bursts into a group of creatures of its kind. The Spitter Plant that launches it is a world object ([defences and props](../03_buildings_and_world_objects/defences_and_props.md)).

**R-UNIT-511** (MUST) A Spitter Plant of the Spitter Plants outcome launches Seed Pods ([crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md) gives the outcome, which only an unopened Supply crate yields; [defences and props](../03_buildings_and_world_objects/defences_and_props.md) gives the plant, its pod count and its choice of kind, which excludes kinds 3 and 9 of R-UNIT-519). Each launched pod has size 1 + `SimRng::NextSigned`(`ref.unit.seed_pod.launch_size_spread`) and velocity = launch socket forward × plant force × plant size × (1 + `SimRng::NextSigned`(`ref.unit.seed_pod.launch_speed_spread`)), and its burst timer is replaced by R-UNIT-515.

**R-UNIT-512** (MUST) The Brood outcome, placed at a target point by a player or a bot, creates `ref.unit.seed_pod.group_count_min` + (`SimRng::NextUint` mod `ref.unit.seed_pod.group_count_spread`) Seed Pods (2 to 4), owned by the team that placed the power (DEC-107). Each starts at the target + (0, `ref.unit.seed_pod.brood_start_height`, 0) with velocity normalise(`SimRng::NextSigned(1)`, `ref.unit.seed_pod.brood_lift`, `SimRng::NextSigned(1)`) × (`ref.unit.seed_pod.brood_speed_min` + `SimRng::NextUnit`(`ref.unit.seed_pod.brood_speed_spread`)), takes its kind by R-UNIT-513, and has its burst timer replaced by R-UNIT-515. The released creatures use the fixed roam ranges of R-UNIT-519, not a roam range of the outcome.

**R-UNIT-513** (MUST) The Brood kind is one `SimRng::NextUint` draw modulo 5 over the list (Crawlers, centipede, spider, Seed Pods, Brood Drifter), which are kinds 0, 1, 2, 5 and 7 of R-UNIT-519. This equals the reference distribution, which re-draws `NextUint` mod 10 until the kind is none of Essence orbs, Crawler eggs, Reaper Serpent, ants and Workers, and it needs no loop (DEC-014).

**R-UNIT-514** (MUST) A Seed Pod of kind 5 (nested pods) releases `ref.unit.seed_pod.group_count_min` + (`SimRng::NextUint` mod `ref.unit.seed_pod.group_count_spread`) Seed Pods at speed `ref.unit.seed_pod.nested_speed_min` + `SimRng::NextUnit`(`ref.unit.seed_pod.nested_speed_spread`). Each takes its kind by one `SimRng::NextUint` draw modulo 9 over kinds 0 to 8 (never Workers). When the Reaper Serpent is not allowed at that draw (R-UNIT-520), the kind is one `SimRng::NextUint` draw modulo 8 over kinds 0 to 7 instead, which equals the reference re-draw of the Reaper Serpent result (DEC-014). A nested pod keeps its default burst timer.

**R-UNIT-515** (MUST) The spawn step of every Seed Pod draws its default burst timer, `ref.unit.seed_pod.default_burst_base` + ⌈`SimRng::NextUnit`(`ref.unit.seed_pod.default_burst_spread`)⌉ ticks (200 to 300 ticks, 20 to 30 s). A Spitter Plant launch (R-UNIT-511) and the Brood outcome (R-UNIT-512) then replace it with `ref.unit.seed_pod.launch_burst_base` + ⌈`SimRng::NextUnit`(`ref.unit.seed_pod.launch_burst_spread`)⌉ ticks (40 to 80 ticks, 4 to 8 s).

**R-UNIT-516** (MUST) A Seed Pod has health `ref.unit.seed_pod.health` (50); its speed `ref.unit.seed_pod.speed` and rate `ref.unit.seed_pod.rate` are unused (R-UNIT-017). It is immune to laser bolts (R-UNIT-027). At 0 health it releases its Essence orb (R-UNIT-507) and a shatter effect and is removed on the next tick without a corpse fade.

**R-UNIT-517** (MUST) A Seed Pod starts with force f = `ref.unit.seed_pod.initial_force` (1.0) and moves each tick by:

```
v.y -= ref.unit.seed_pod.gravity_per_tick * f        # 9.8 wu/s per tick at f = 1, about 98 wu/s^2
p += v * dt
if p.y < ground(p) + ref.unit.seed_pod.bounce_clearance:          # 3 wu
    Bounce()                                          # reference/12 R-SIM-385, restitution 0.85
    f *= ref.unit.seed_pod.bounce_friction            # 0.65
    v *= f
    p.y = max(p.y, ground(p) + ref.unit.seed_pod.bounce_clearance)
    if f > ref.unit.seed_pod.bounce_sound_force: emit bounce sound     # 0.1
up = rotate(up, axis = normalise(up x v), angle = f^2 * ref.unit.seed_pod.tumble_rate * dt)   # tumble, 30
if up.y < ref.unit.seed_pod.self_right_up and f < ref.unit.seed_pod.self_right_force:          # 0.3, 0.4
    up = (0, 1, 0)                                    # it rights itself
```

It bounces off the terrain only, never off structures, and bursts wherever it lies.

**R-UNIT-518** (MUST) When its burst timer has elapsed, a Seed Pod emits a shatter effect, releases its kind by R-UNIT-519, emits the burst-open sound and is removed.

**R-UNIT-519** (MUST) The release of a Seed Pod depends on its kind. The kind numbers carry meaning, because the kind draws of R-UNIT-511, R-UNIT-513 and R-UNIT-514 index this list. Every released actor belongs to the pod's team unless the row says otherwise, and spawns at the pod with spawn spread `ref.unit.seed_pod.release_spread` (0 wu):

| Kind | Releases | Count | Velocity | Roam range |
|---|---|---|---|---|
| 0 | Crawlers | `ref.unit.seed_pod.crawlers_min` + (`SimRng::NextUint` mod `ref.unit.seed_pod.crawlers_spread`), 20 to 39; none when the team already has `ref.unit.crawler.team_limit` (250) Crawlers or more | — | `ref.unit.seed_pod.crawlers_roam` (100 wu) |
| 1 | one centipede: an actor group of n segments (§2.6) | n = `ref.unit.seed_pod.centipede_size_min` + (`SimRng::NextUint` mod `ref.unit.seed_pod.centipede_size_spread`), 5 to 14 | — | `ref.unit.seed_pod.centipede_roam` (200 wu) |
| 2 | a spider | 1 | — | `ref.unit.seed_pod.spider_roam` (150 wu) |
| 3 | Essence orbs ([Essence orbs](essence_orbs.md)) | `ref.unit.seed_pod.orbs_min` + (`SimRng::NextUint` mod `ref.unit.seed_pod.orbs_spread`), 5 to 9 | horizontal speed `ref.unit.seed_pod.orb_speed_min` + `SimRng::NextUnit`(`ref.unit.seed_pod.orb_speed_spread`) | — |
| 4 | Crawler eggs | `ref.unit.seed_pod.eggs_min` + (`SimRng::NextUint` mod `ref.unit.seed_pod.eggs_spread`), 5 to 9 | upward plus a random component, length `ref.unit.seed_pod.egg_speed_min` + `SimRng::NextUnit`(`ref.unit.seed_pod.egg_speed_spread`) | `ref.unit.seed_pod.thrown_roam` (0) |
| 5 | Seed Pods | R-UNIT-514, 2 to 4 | R-UNIT-514 | `ref.unit.seed_pod.thrown_roam` (0) |
| 6 | ants, with no Hive (they start in the HiveLost order of §2.7) | `ref.unit.seed_pod.ants_min` + (`SimRng::NextUint` mod `ref.unit.seed_pod.ants_spread`), 20 to 29 | thrown upward at `ref.unit.seed_pod.throw_speed_min` + `SimRng::NextUnit`(`ref.unit.seed_pod.throw_speed_spread`) | `ref.unit.seed_pod.thrown_roam` (0) |
| 7 | a Brood Drifter | 1 | — | `ref.unit.seed_pod.brood_drifter_roam` (200 wu) |
| 8 | a Reaper Serpent of the Rogue faction, whatever the pod's team (R-UNIT-520) | 1 | — | `ref.unit.seed_pod.reaper_serpent_roam` (200 wu) |
| 9 | Workers | `ref.unit.seed_pod.workers_min` + (`SimRng::NextUint` mod `ref.unit.seed_pod.workers_spread`), 10 to 19 | thrown upward at `ref.unit.seed_pod.throw_speed_min` + `SimRng::NextUnit`(`ref.unit.seed_pod.throw_speed_spread`) | `ref.unit.seed_pod.thrown_roam` (0) |

**R-UNIT-520** (MUST) Kind 8 releases a Reaper Serpent only when the mode is not Siege and fewer than `ref.unit.reaper_serpent.alive_limit` (2) Reaper Serpents are alive in the match; otherwise the pod releases kind 0 instead, with the Crawler limit and roam range of kind 0.

**R-UNIT-521** (MUST) Presentation: the Seed Pod uses `mesh.creature.seed_pod`, tinted entirely in its team colour. Its drawn size pulses as size + |sin(`ref.unit.seed_pod.pulse_rate` × t)| × (1 − age) × `ref.unit.seed_pod.pulse_amplitude`, with t the presentation time and age its age fraction, and its shadow is size × `ref.unit.seed_pod.shadow_scale` wu. Sounds: `sfx.creature.seed_pod.spawn` when it is created, `sfx.creature.seed_pod.bounce`, `sfx.creature.seed_pod.burst_open`, and on death the shared group `sfx.creature.reaper_serpent.die`.

### 2.3 Crawler egg

A Crawler egg is a small egg that, once an Essence orb is placed inside it, hatches 4 Crawlers: the creatures' own way of turning orbs into bodies.

**R-UNIT-522** (MUST) Crawler eggs are laid by Brood Drifters (R-UNIT-544) and spiders (R-UNIT-578) and released by Seed Pods of kind 4 (R-UNIT-519). Neither the Brood kind list nor a Spitter Plant's choice includes kind 4, so in a match kind 4 comes only from nested Seed Pods. An egg is fertilised by a Crawler that carries an Essence orb (R-UNIT-533) or, at laying, by a Brood Drifter (R-UNIT-544). Each Essence orb keeps a list of the dormant eggs near it ([Essence orbs](essence_orbs.md)), so Crawlers fetch only orbs that have an egg nearby.

**R-UNIT-523** (MUST) A Crawler egg has health `ref.unit.crawler_egg.health` (10), speed `ref.unit.crawler_egg.speed` (0) and rate `ref.unit.crawler_egg.rate` (0) (R-UNIT-501). Its states are Dormant and Fertilised.

**R-UNIT-524** (MUST) Fertilising an egg sets the orb to InEgg, holds the orb's handle, sets the egg's timer to 0 and the state to Fertilised. Every tick, when the egg's held orb handle is not a valid Essence orb, the egg becomes Dormant and its timer is set to 0. While Fertilised, the timer counts ticks and the orb is held `ref.unit.crawler_egg.orb_hold_height` (3 wu) above the egg. When the timer reaches `ref.unit.crawler_egg.hatch_ticks` (151 ticks, nominal 15.0 s), the orb is removed and, if the egg's team has fewer than `ref.unit.crawler.team_limit` (250) Crawlers, `ref.unit.crawler_egg.hatch_crawlers` (4) Crawlers of the egg's team spawn at the egg with spread 0 and roam range `ref.unit.crawler_egg.hatch_roam` (200 wu); the orb is consumed in either case, and the egg is removed.

**R-UNIT-525** (MUST) A dormant Crawler egg never decays. A dormant egg holds no orb, so R-UNIT-524 resets its timer every tick, and the dormant lifetime `ref.unit.crawler_egg.dormant_lifetime` (1201 ticks, 120 s), with its decay of health to at most 10 × (1 − timer ÷ 120 s), never takes effect (DEC-091).

**R-UNIT-526** (MUST) An egg moves by the airborne step of [reference/12](../12_simulation_model/05_space_routing_and_physics.md) (R-SIM-370) until it is on the ground. On the ground it is held to the terrain height each tick, becomes airborne again when the ground drops away beneath it, and moves by p += v × dt. An egg at or below sea level (`ref.sim.physics.sea_level`) takes `ref.unit.crawler_egg.water_damage` (500).

**R-UNIT-527** (MUST) Any damage kind hurts a Crawler egg. A hit of amount a (negative) kills it only when health + a < 0 (R-UNIT-027), so a hit equal to its remaining health leaves it alive at 0. Its death releases no Essence orb and plays no sound; a held orb returns to Floating ([Essence orbs](essence_orbs.md)); the corpse fades (R-UNIT-034) and is removed. Workers never treat an egg as a threat, so eggs die only incidentally, mostly to explosions.

**R-UNIT-528** (MUST) Presentation: an egg is a camera-facing additive card `sprite.crawler_egg.body` of `ref.unit.crawler_egg.card_size` (8 × 8 wu), with alpha health ÷ 10, at least `ref.unit.crawler_egg.min_alpha` (0.1). A fertilised egg adds two throbbing cards that grow over its hatch time. A dead egg breaks into `ref.unit.crawler_egg.fragments` (3) drifting quarter-card fragments. The egg is drawn white whatever its team colour. It has no sound of its own.

### 2.4 Crawler, Crawler group and Crawler trail sample

Crawlers are fast worm creatures with a glowing trail. They bite enemies on contact and ferry Essence orbs into Crawler eggs to breed more Crawlers: the swarm unit of the creature outcomes.

**R-UNIT-529** (MUST) Crawlers are released by Seed Pods of kind 0, also when a refused Reaper Serpent falls back to kind 0 (R-UNIT-519, R-UNIT-520), by hatching Crawler eggs (R-UNIT-524) and by Swarm streaks, which turn an Essence orb into a Rogue faction Crawler with roam range 200 wu, or into a spider ([crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md)). Every match source spawns Crawlers without an actor group, among the team's ungrouped actors. The match world counts each team's Crawlers from the spawn step until removal, a fading corpse included, and every rule that releases Crawlers checks this count against `ref.unit.crawler.team_limit` (250) as it states. Crawlers are not counted against the population cap ([population flow](../05_population_flow.md) R-ECON-050).

**R-UNIT-530** (MUST) A Crawler has health `ref.unit.crawler.health` (10), speed `ref.unit.crawler.speed` (32 wu/s) and rate `ref.unit.crawler.rate` (0). It moves toward its target position, or along its idle compass direction, at speed × its slope factor (R-UNIT-042), `ref.unit.crawler.hover_height` (0.1 wu) above the terrain. It is never pushed by obstacles and takes no water damage, so Crawlers cross water and pass through structures; the world clamp applies. While above the terrain it falls with `ref.unit.crawler.fall_gravity` (10 wu/s²) and lands at `ref.unit.crawler.land_height` (2 wu) above the ground.

**R-UNIT-531** (MUST) A Crawler's states are Idle, Attacking, ToOrb and ToEgg. Its retarget timer starts at ⌈`SimRng::NextUnit`(`ref.unit.crawler.first_retarget_spread`)⌉ ticks (0 to 20), drawn at spawn, and restarts at `ref.unit.crawler.retarget_ticks` (11 ticks, nominal 1.0 s). When it expires in Idle, the Crawler tries in this order and takes the first that succeeds:

1. if it carries an Essence orb: the dormant, grounded Crawler eggs of its own team within `ref.unit.crawler.egg_search_range` (60 wu); with n found it picks the egg at index ⌊`SimRng::NextUnit`(n)⌋ and enters ToEgg;
2. if enemies are present (for a member of a Crawler group, only when the group's pre-check of R-UNIT-538 found any): the nearest enemy that is alive and on the ground at a distance from `ref.unit.crawler.search_min` (30 wu) to `ref.unit.crawler.search_max` (60 wu); it enters Attacking;
3. the nearest Essence orb of any team in Birth or Floating within `ref.unit.crawler.orb_search_range` (60 wu) whose nearby-egg list is not empty ([Essence orbs](essence_orbs.md) R-UNIT-309); it enters ToOrb;
4. otherwise it picks a new idle direction by R-UNIT-534.

**R-UNIT-532** (MUST) In Attacking, a Crawler whose target is gone or dead returns to Idle. On arrival at its target it bites: the target takes `ref.unit.crawler.bite_damage` (20) as an unresistable health change, three muzzle flashes appear (cosmetic) and the attack sound plays. It then searches for the nearest enemy at least `ref.unit.crawler.search_min` (30 wu) away, up to `ref.unit.crawler.search_max`; if it finds none it keeps its current target and, being on top of it, bites again on the next tick. A Crawler in contact therefore deals 20 per tick, 200 health per second.

**R-UNIT-533** (MUST) In ToOrb, a Crawler whose orb is gone, dying, Attached or InEgg returns to Idle. On arrival it attaches the orb (Attached, [Essence orbs](essence_orbs.md) R-UNIT-313), picks an egg as in step 1 of R-UNIT-531 and enters ToEgg. In ToEgg, when no egg is found, the egg is gone or the orb is no longer valid, it drops the orb and returns to Idle; on arrival at the egg it fertilises it with the orb (R-UNIT-524) and returns to Idle. A carried orb follows the Crawler at its position + (0, `ref.unit.crawler.orb_carry_height`, 0) (2 wu), and a Crawler that dies drops it.

**R-UNIT-534** (MUST) A new idle direction is chosen as follows, with d the Crawler's distance from its spawn point (R-UNIT-020) and R its roam range:

1. the return chance is d ÷ R, or 0 when d ÷ R is below `ref.unit.crawler.return_threshold` (0.75); with that chance, tested against `SimRng::NextUnit(1)`, it takes the one of the 8 compass directions that points most nearly toward its spawn point;
2. otherwise it makes up to `ref.unit.crawler.direction_tries` (3) tries of the compass direction (trunc(`SimRng::NextUnit(3)`) − 1, 0, trunc(`SimRng::NextUnit(3)`) − 1) and takes the first whose end point, speed × retarget time ahead, lies above sea level.

Each new direction records a required trail sample (R-UNIT-535) and plays the change-direction sound.

**R-UNIT-535** (MUST) A Crawler keeps a position history of Crawler trail samples, newest first. It records a required sample at each new direction and state change, a sample every `ref.unit.crawler.sample_ticks` (6 ticks, nominal 0.5 s) while not Idle, and extra non-required samples whenever its height differs by more than `ref.unit.crawler.sample_height_step` (3 wu) from the last sample. The history is trimmed from the oldest end once its summed length exceeds `ref.unit.crawler.tail_length` (175 wu). A Crawler trail sample holds: its world position; the strip's width direction, perpendicular to the segment on the terrain normal; the step vector of `ref.unit.crawler.glow_step` (10 wu) toward the previous sample, for the glow strip; the distance to the previous sample, for the trail length and the alpha falloff; and whether it is required.

**R-UNIT-536** (MUST) A laser bolt that hits a Crawler damages it but never pushes or turns it ([weapons and projectiles](../04_supply_crates_powers_and_weapons/weapons_and_projectiles.md)).

**R-UNIT-537** (MUST) A Crawler dies by the base death rule (R-UNIT-507, its die sound): its velocity becomes 0 and its corpse fades in place. Its trail does not scatter.

**R-UNIT-538** (MUST) A Crawler group exists only when a map places Crawlers as a group (R-UNIT-506); no match rule creates one. In each advance the group first tests whether enemies are present within its radius + `ref.unit.crawler.group_enemy_margin` (60 wu) of its centre, as a pre-check for its members' enemy searches, and then advances as an actor group ([reference/12](../12_simulation_model/06_teams_and_player_commands.md)). It draws its members in one additive batch with the render detail of R-UNIT-539.

**R-UNIT-539** (MUST) Presentation: a Crawler is drawn from `sprite.crawler.segment` as a worm strip of half-width `ref.unit.crawler.worm_half_width` (3 wu) along its trail, with alpha = worm alpha − cumulative distance, plus a glow strip of half-width `ref.unit.crawler.glow_half_width` (12 wu). The worm colour is the team colour × health fraction with alpha `ref.unit.crawler.worm_alpha` (200); the glow is the team colour with alpha `ref.unit.crawler.glow_alpha` (150). A Rogue faction Crawler is drawn with worm colour `ref.unit.crawler.rogue_worm_colour` (255, 255, 255, 0) and glow `ref.unit.crawler.rogue_glow_colour` (200, 200, 200, 50); a dead Crawler fades toward white. Render detail 1 to 4: detail 4 skips the glow strip, and each detail above 1 drops the non-required samples and draws only 1 − `ref.unit.crawler.detail_history_step` (0.25) × min(detail − 1, 2) of the history. A Crawler group takes its detail from the render-detail option and the camera distance: option 1 gives detail 2 beyond `ref.unit.crawler.detail_far` (1000 wu); option 2 gives detail 3 beyond 1000 wu and detail 2 beyond `ref.unit.crawler.detail_near` (500 wu); option 3 gives detail 4; otherwise detail 1. Ungrouped Crawlers are drawn by the team's Crawler crowd pass ([renderer](../../architecture/components/renderer/README.md)). Sounds: `sfx.creature.crawler.turn` (change direction), `sfx.creature.crawler.attack`, `sfx.creature.crawler.hurt`, `sfx.creature.crawler.die`.

### 2.5 Brood Drifter

A Brood Drifter is a hovering creature with four tails. It drifts around its spawn point and lays Crawler eggs near floating Essence orbs and at random: a slow breeding engine that turns orbs into Crawlers.

**R-UNIT-540** (MUST) Brood Drifters are released by Seed Pods of kind 7 with roam range 200 wu (R-UNIT-519). A Brood Drifter has health `ref.unit.brood_drifter.health` (200) and speed `ref.unit.brood_drifter.speed` (20 wu/s); its rate is unused. It spawns at ground level, and its hover height `ref.unit.brood_drifter.hover_height` (100 wu) applies only to the targets it picks, so it rises toward them. It never attacks.

**R-UNIT-541** (MUST) A Brood Drifter flies with no terrain or structure collision. Each tick:

```
desired = normalise(target - p) * speed
if |target - p| < ref.unit.brood_drifter.slow_radius:        # 50 wu
    desired *= |target - p| / ref.unit.brood_drifter.slow_radius
v = v * (1 - dt * ref.unit.brood_drifter.steer_blend) + desired * dt * ref.unit.brood_drifter.steer_blend   # 0.5
v = rotate(v, axis = right, angle = sin(ref.unit.brood_drifter.wobble_rate * t) * dt * ref.unit.brood_drifter.wobble_amplitude)   # 3, 1.5
p += v * dt
```

where t is match time, so the wobble is deterministic. It has arrived when it is within `ref.unit.brood_drifter.arrival` (10 wu) of its target.

**R-UNIT-542** (MUST) Its states are Idle, EggLaying and Panic. In Idle it retargets every `ref.unit.brood_drifter.retarget_ticks` (61 ticks, nominal 6.0 s): it looks for a Floating Essence orb within `ref.unit.brood_drifter.orb_search_range` (200 wu) that is above height 0 and has fewer than `ref.unit.brood_drifter.max_nearby_eggs` (3) eggs in its nearby-egg list; with one it enters EggLaying toward it; otherwise it picks a random target by R-UNIT-543.

**R-UNIT-543** (MUST) A random target is, with the return chance d ÷ R (d the distance from its spawn point, R its roam range, tested against `SimRng::NextUnit(1)`), a point `ref.unit.brood_drifter.random_step` (200 wu) from it toward its spawn point; otherwise a point within a box of side `ref.unit.brood_drifter.random_step` centred on it. The target height is `ref.unit.brood_drifter.hover_height` × (1 ± `ref.unit.brood_drifter.hover_spread`), 100 wu ± 15 %. A random target picked in Idle or on arrival in EggLaying draws the lay roll: when `SimRng::NextUnit(100)` < `ref.unit.brood_drifter.random_lay_chance` (10, at difficulty 0, R-UNIT-016), the Brood Drifter lays eggs at that target (EggLaying); otherwise it flies there in Idle.

**R-UNIT-544** (MUST) The lay target for an orb is the orb's position + (`SimRng::NextSigned`(`ref.unit.brood_drifter.lay_offset`), 0, `SimRng::NextSigned`(`ref.unit.brood_drifter.lay_offset`)), ±30 wu on x and z, at `ref.unit.brood_drifter.egg_lay_height` (20 wu) up. While it is within `ref.unit.brood_drifter.slow_radius` (50 wu) of its lay target, it lays one Crawler egg every `ref.unit.brood_drifter.egg_interval_base` + ⌈`SimRng::NextUnit`(`ref.unit.brood_drifter.egg_interval_spread`)⌉ ticks (20 to 40 ticks, at difficulty 0): the egg appears at the `egg_spawn` socket of its mesh with the Brood Drifter's velocity and team, and the lay-egg sound plays. When `SimRng::NextUint` mod `ref.unit.brood_drifter.prefertilise_one_in` (4) is 0, a new Essence orb of the Brood Drifter's team is spawned into the egg and fertilises it; for the Rogue faction the spawn is refused (R-UNIT-507) and the egg stays dormant. On arrival at the lay target it searches for an orb again (R-UNIT-542), or else picks a random target with the lay roll; a random target without the lay roll returns it to Idle.

**R-UNIT-545** (MUST) A hit that registers (R-UNIT-546) starts Panic for `ref.unit.brood_drifter.panic_ticks` (101 ticks, nominal 10 s) and sets its velocity to up × speed × `ref.unit.brood_drifter.panic_speed_factor` (3). While panicking, its target is moved each tick by (`SimRng::NextSigned`(`ref.unit.brood_drifter.panic_jitter`), 0, `SimRng::NextSigned`(`ref.unit.brood_drifter.panic_jitter`)), ±50 wu; when the panic ends it returns to Idle.

**R-UNIT-546** (MUST) A Brood Drifter is immune to laser bolts and ignores every health change of −1 or above (R-UNIT-027). A larger hit applies the base rule, shatters the fraction of its mesh that matches the health lost, and starts Panic. Its death releases an Essence orb (R-UNIT-507) and plays its die sound.

**R-UNIT-547** (MUST) Presentation: `mesh.creature.brood_drifter_body` with region tint, and `ref.unit.brood_drifter.tail_count` (4) tails, each a strip of `ref.unit.brood_drifter.tail_segments` (3) segments in team colour from the sockets `tail_01` to `tail_04`, animated with sine offsets and with one smoothed velocity that all Brood Drifters share; a damaged flicker; a shadow of `ref.unit.brood_drifter.shadow_size` (20 wu). Sounds: `sfx.creature.brood_drifter.presence_loop` from its creation, `sfx.creature.spider.lay_egg` (shared group) when it lays, `sfx.creature.brood_drifter.hurt`, `sfx.creature.brood_drifter.die`.

### 2.6 centipede

A centipede is a chain of linked segments, one actor per segment, all in one plain actor group. Its head hunts enemies, eats floating Essence orbs and grows; every segment crushes the enemies it touches; killing a middle segment splits it in two.

**R-UNIT-548** (MUST) A centipede is created only inside an actor group. A Seed Pod of kind 1 creates one group of n segments with spread 0 and roam range 200 wu (R-UNIT-519); a map may place one as a group (R-UNIT-506); eating Essence orbs adds segments (R-UNIT-554). Centipede segments live in their actor group and are not counted against the population cap (DEC-117).

**R-UNIT-549** (MUST) When a segment begins without a leader, every segment of its group is relinked in member-slot order: the first member is the **head**, with no segment in front of it, and each next member follows the one before it. The head's size is min(`ref.unit.centipede.tail_size` × `ref.unit.centipede.size_ratio`^N, `ref.unit.centipede.head_size_cap`), with N the number of segments (0.2 × 1.1^N, at most 10), and each following segment's size is the size of the one in front ÷ `ref.unit.centipede.size_ratio` (1.1). A segment's contact radius is `ref.unit.centipede.contact_radius_factor` × its size at that moment (10 × size), and it keeps that radius.

**R-UNIT-550** (MUST) A segment's maximum health is `ref.unit.centipede.health` × `ref.unit.centipede.health_size_factor` × size (255 × 2 × size), clamped to [0, `ref.unit.health_max`]: 255 for a size of 0.5 or more, 102 for the tail size 0.2. Each tick a segment's target size is `ref.unit.centipede.tail_size` (0.2) for the last segment and min(`ref.unit.centipede.size_ratio` × the size of the segment behind it, `ref.unit.centipede.size_cap`) for every other segment (1.1 × behind, at most 1.0). When its size differs from the target by more than `ref.unit.centipede.size_tolerance` (0.01), the size moves `ref.unit.centipede.size_blend` (10 %) of the way toward it, and the health is rescaled to keep the same fraction of the maximum. The steady state is a tail of 0.2 growing by ×1.1 per segment toward the head, capped at 1.0.

**R-UNIT-551** (MUST) Every segment records its position every tick and keeps its last `ref.unit.centipede.history_samples` (3) positions, newest first. A trailing segment does not steer: with its leader's samples h[0], h[1], h[2], and k = 0 when the leader's member index is greater than its own (the leader has not yet advanced this tick) or k = 1 otherwise, it sets

```
p = h[k+1] + (h[k] - h[k+1]) * (1 - size)
v = (h[k] - h[k+1]) / dt
```

and takes its leader's panic timer. It is drawn linked to its leader only when the leader is linked and it has at least 2 samples of its own. When its leader dies, it becomes a head.

**R-UNIT-552** (MUST) A head moves at `ref.unit.centipede.speed` (80 wu/s) × its slope factor (R-UNIT-042), on the terrain. It turns its front toward its target with the blend front = normalise(front × (1 − b) + direction × b), b = dt × `ref.unit.centipede.head_turn` (3); a trailing segment that steers uses b = dt × `ref.unit.centipede.trail_turn` (1.5). It has arrived when it is within `ref.unit.centipede.arrival` (20 wu) of its target. A target below height 0, or a structure within `ref.unit.centipede.structure_lookahead` (150 wu) ahead along its front, counts as arrived and forces a new target, so a centipede refuses water and steers away from structures; it is otherwise never pushed by structures. A centipede gets no world clamp.

**R-UNIT-553** (MUST) The head's states are Roam, Hunt, SeekOrb and Panic. Whenever it has no target or has arrived, outside Panic, it searches in this order:

1. the nearest enemy at a distance from `ref.unit.centipede.search_min` × size to `ref.unit.centipede.search_max` × size (75 × size to 150 × size wu, with the head's size): it enters Hunt, and the enemy-sighted cue is silent;
2. while the centipede has at most `ref.unit.centipede.max_segments` (20) segments, a Floating Essence orb at a distance from `ref.unit.centipede.search_min` to `ref.unit.centipede.search_max` (75 wu to 150 wu, not scaled): it enters SeekOrb;
3. otherwise a random position in Roam: with the return chance d ÷ R (d the distance from its spawn point, R its roam range, tested against `SimRng::NextUnit(1)`) a step of `ref.unit.centipede.wander_step` (100 wu) toward its spawn point, otherwise a step of 100 wu in a random direction.

In Hunt the target position is refreshed every tick to the enemy's position at ground height, and the head searches again when the enemy is gone or reached; SeekOrb ends on arrival. While its panic timer is above 0 the head is in Panic: each tick, when `SimRng::NextUnit(1)` < `ref.unit.centipede.retreat_chance` (0.5), it takes a retreat point `ref.unit.centipede.retreat_distance` (50 wu) directly away from the nearest enemy within `ref.unit.centipede.search_max` × size, turned by `SimRng::NextSigned(π)` (±90°); at 0 it returns to Roam. Every tick, in every state, the head eats (R-UNIT-554) and every segment attacks (R-UNIT-555).

**R-UNIT-554** (MUST) While the centipede has at most `ref.unit.centipede.max_segments` segments, its head eats Floating Essence orbs of any team within `ref.unit.centipede.eat_range` (10 wu, horizontal): each candidate is eaten when `SimRng::NextUnit(1)` < size ÷ `ref.unit.centipede.eat_chance_divisor` (size ÷ 2), at most one per tick, and an eaten orb is removed. When the head has eaten at least `ref.unit.centipede.orbs_per_segment` (4) orbs, ⌊eaten ÷ 4⌋ segments are appended behind the current tail and 4 orbs are used up for each: each new segment is a new member of the group, with the tail's size (at least 0.2), placed at the centipede's spawn point, from where it follows by R-UNIT-551. The grow cue is silent.

**R-UNIT-555** (MUST) Every tick, every live segment pushes each enemy within its contact radius r out to distance r, updating that actor's grid entry itself ([reference/12](../12_simulation_model/03_entity_model_and_identity.md) R-SIM-115), and deals trunc((r − d) × `ref.unit.centipede.contact_damage_factor`) as an unresistable health change (R-UNIT-030), with d the distance before the push; each contact plays the attack sound.

**R-UNIT-556** (MUST) Every damage kind hurts a centipede segment (R-UNIT-027). When a hit drops a segment's health from one third of its maximum into a lower third (`ref.unit.centipede.panic_bands`, 3), the segment panics for `ref.unit.centipede.panic_base` + ⌈`SimRng::NextUnit`(`ref.unit.centipede.panic_spread`)⌉ ticks (20 to 40), and so does every segment in front of it, up to the head. The panic cue is silent.

**R-UNIT-557** (MUST) A segment dies by the base rule: it releases an Essence orb (R-UNIT-507) and a shatter effect scaled by its size, and unlinks from both neighbours. When a middle segment dies, the centipede splits into two independent centipedes: the segment behind the dead one becomes a head. A dead segment runs only its corpse fade. A segment's size count is the number of segments from it backward to the tail.

**R-UNIT-558** (MUST) Presentation: `mesh.creature.centipede_head_segment` for a head and `mesh.creature.centipede_segment` for every other segment, with region tint, aligned to the terrain normal. Sounds: `sfx.creature.centipede.presence_loop` from creation, `sfx.creature.centipede.attack` per contact, `sfx.creature.centipede.hurt`, `sfx.creature.centipede.die`; the panic, enemy-sighted and grow cues are silent (DEC-565).

### 2.7 ant

An ant is a small, fast creature that leaves its Hive on an objective, collects floating Essence orbs and captures Workers, which it carries back and kills at the Hive, and bites any other enemy it meets.

**R-UNIT-559** (MUST) Ants come from three sources:

1. **a Hive** ([defences and props](../03_buildings_and_world_objects/defences_and_props.md)) releases its ants one by one into its shared ant group, each with the ScoutArea order toward an objective that the Hive chooses, one ant per `ref.econ.hive.sortie_interval_ticks` ([population flow](../05_population_flow.md)); the population cap is checked at each sortie (DEC-453), the Hive's own ants are not counted (DEC-084), and a burning Hive releases an ant only while its ant count is above zero (DEC-460);
2. **the Hive outcome** spawns one wake-up ant on the Rogue faction; the outcome's Hives always belong to the Rogue faction, whoever opened the Supply crate ([crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md));
3. **a Seed Pod of kind 6** releases 20 to 29 ants of the pod's team, thrown upward, with no Hive (R-UNIT-519); they start with no order and move to HiveLost by R-UNIT-567.

**R-UNIT-560** (MUST) An ant has health `ref.unit.ant.health` (20). Its spawn step draws its speed, `ref.unit.ant.speed` × (1 + `SimRng::NextSigned`(`ref.unit.ant.speed_spread`)) (20 to 30 wu/s), and its scale, 1 + `SimRng::NextSigned`(`ref.unit.ant.scale_spread`) (0.55 to 1.45), which sizes its mesh and its death shatter. Its speed is × `ref.unit.ant.hurry_factor` (2) while its order is CollectWorker or AttackEnemy.

**R-UNIT-561** (MUST) An ant's orders are NoOrders, ScoutArea, CollectOrb, CollectWorker, AttackEnemy, ReturnToBase and HiveLost, with these transitions:

| Order | Event | Next order |
|---|---|---|
| NoOrders | its Hive exists, or a friendly Hive is found within `ref.unit.ant.hive_search` (500 wu) | ReturnToBase |
| NoOrders | no Hive | HiveLost |
| ScoutArea | arrived, and its objective target is a Worker | CollectWorker |
| ScoutArea | arrived, and its objective target is another actor or a structure | AttackEnemy |
| ScoutArea | arrived, and its objective target is gone | ReturnToBase |
| any order | the search of R-UNIT-562 finds an Essence orb | CollectOrb |
| any order | the search of R-UNIT-562 finds an enemy | AttackEnemy |
| CollectOrb | arrived and the orb is attached, or the orb is gone, dying, Attached to another carrier or InEgg | ReturnToBase |
| CollectWorker | arrived and the Worker is captured, or the Worker is dead or already captured | ReturnToBase |
| AttackEnemy | at the target: it bites (R-UNIT-564) | AttackEnemy |
| AttackEnemy | the target is dead or gone | ReturnToBase |
| ReturnToBase | arrived at its Hive | delivered and removed (R-UNIT-566) |
| ReturnToBase | arrived, its Hive is gone, and a friendly Hive is found within `ref.unit.ant.hive_search` | ReturnToBase, to that Hive |
| ReturnToBase | arrived, its Hive is gone, and no other Hive is found | HiveLost |
| HiveLost | arrived | HiveLost, with a new random position (R-UNIT-567) |

**R-UNIT-562** (MUST) While an ant has no found target, it searches every tick, Essence orbs first: an orb of any team in Birth or Floating within `ref.unit.ant.search_range` (10 wu); then the best enemy within `ref.unit.ant.search_range` that is neither a Worker nor a Collector. A found target is not searched again until a failure clears it. The search can interrupt any order, the trip home included.

**R-UNIT-563** (MUST) An ant walks in a straight line toward its waypoint at its speed, with its height set to the terrain height, and is never pushed by obstacles, so ants walk through structures. Its heading turns by `SimRng::NextSigned`(`ref.unit.ant.heading_jitter`) rad (±0.1) every tick. It has arrived when the remaining distance is below |v| × dt or below `ref.unit.ant.arrival_min` (1 wu). An ant that is not on the ground (a Seed Pod throws its ants) moves by the airborne step of [reference/12](../12_simulation_model/05_space_routing_and_physics.md) (R-SIM-370).

**R-UNIT-564** (MUST) At its target in AttackEnemy, an ant bites every tick: the target takes `ref.unit.ant.bite_damage` (1) as an unresistable health change (R-UNIT-030), about 10 health per second; three muzzle flashes appear (cosmetic); the attack sound plays. A Builder is attacked at the ground point beneath it. An ant's attack on a structure identifies the structure by its handle (DEC-105).

**R-UNIT-565** (MUST) On arrival in CollectOrb the ant attaches the orb ([Essence orbs](essence_orbs.md) R-UNIT-313); on arrival in CollectWorker the Worker enters its captured state ([Worker](worker.md)). Every tick the carried orb and the captured Worker are moved to the ant's `carry` socket. When the ant dies, a captured Worker returns to its idle state and the carried orb is dropped with the ant's velocity.

**R-UNIT-566** (MUST) On arrival at its Hive in ReturnToBase, the Hive's ant count rises by 1; a carried orb that is still Attached is removed and the Hive's orb count rises by 1; a carried Worker that is still captured is killed by a health change of −2 × its health, so its Essence orb is released at the Hive; and the ant is removed.

**R-UNIT-567** (MUST) An ant in HiveLost, on arrival, picks a new random position: with the chance d ÷ `ref.unit.ant.stray_return_range` (d its distance from its spawn point, 400 wu, tested against `SimRng::NextUnit(1)`) a position toward its spawn point, otherwise a random step of `ref.unit.ant.wander_step` (100 wu).

**R-UNIT-568** (MUST) An ant dies by the base rule: it releases an Essence orb (R-UNIT-507), so a Hive's Rogue faction ants release none while ants of a player team from a Brood Seed Pod do, and it shatters at its scale and drops a carried orb (R-UNIT-565).

**R-UNIT-569** (MUST) Presentation: three variants of `mesh.creature.ant_body`, with region tint; every tick an ant shows a variant different from its last one, re-drawn until it differs (a cosmetic draw, DEC-222). Sounds: `sfx.creature.ant.presence_loop` from creation, `sfx.creature.ant.attack` per bite, `sfx.creature.ant.hurt`, `sfx.creature.ant.die`.

