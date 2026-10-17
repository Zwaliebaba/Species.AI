# Defences and props

> Status: Reference · Prefix: R-BLDG · Related: [Buildings and world objects](README.md) · [Economy and spawn objects](economy_and_spawn_objects.md) · [Mode objects](mode_objects.md) · [02 Worker](../02_units/worker.md) · [02 Creatures](../02_units/creatures.md) · [04 Crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md) · [04 Weapons and projectiles](../04_supply_crates_powers_and_weapons/weapons_and_projectiles.md) · [05 Population flow](../05_population_flow.md) · [07 Bots](../07_bots.md) · [08 Maps and level design](../08_maps_and_level_design/README.md) · [12 Simulation model](../12_simulation_model/README.md)

## 1 Purpose and player experience

This document specifies the defences, the scenery and the crate-spawned structures of the reference game: the four turret types and the Convert Turret, Barrier Emitters, walls, trees, scenery meshes, exclusion zones, smoke columns, volcano vents, jump pads, Hives, Spitter Plants and the Allegiance Scrambler effect. The rules that every structure shares (ports, capture model, lifecycle, damage, hit tests, presentation and sound events) are in the [folder README](README.md); this document gives each kind's own behaviour.

Table 1-A The objects of this document.

| Object | Identifier | Kind of object | Placed by | Owner | Section |
|---|---|---|---|---|---|
| turret (gun, rocket, mortar, flame) and Convert Turret | `Turret`, `TurretType::Gun`, `TurretType::Rocket`, `TurretType::Mortar`, `TurretType::Flame`, `TurretType::Convert` | structure | maps; turret powers from Supply crates and Transit Gate waves | the team that mans its ports | §2.2 |
| turret aim marker | — | effect | each turret | the turret's team, gun turrets only | §2.2.11 |
| Barrier Emitter | `BarrierEmitter` | structure | maps | fixed by the map | §2.3 |
| wall | `WallObject` | structure | maps | no team | §2.4 |
| tree | `TreeObject` | structure | maps; the Essence Grove and Blight Grove outcomes; other trees (offspring) | no team | §2.5 |
| scenery mesh | `SceneryMesh` | structure | maps | no team | §2.6 |
| exclusion zone | `ExclusionZone` | structure | maps | no team | §2.7 |
| smoke column | `SmokeColumn` | structure | maps | no team | §2.8 |
| volcano vent | `VolcanoVent` | structure | maps | no team | §2.9 |
| jump pad | `JumpPadObject` | structure | maps | no team; works for every team | §2.10 |
| Hive | `Hive` | structure | the Hive outcome; a map may place one | the Rogue faction | §2.11 |
| Spitter Plant | `SpitterPlant` | structure | the Spitter Plants outcome; a map may place one | the Rogue faction | §2.12 |
| Allegiance Scrambler effect | `AllegianceScramblerEffect` | effect | the Allegiance Scrambler outcome | — | §2.13 |

**Player experience.**

- **Turrets** are the strongest static weapons on a map. A turret belongs to whoever stands on its four ports; a single Worker is enough. It finds the nearest enemy on its own, sprays it with fire while the target wobbles in its sights, and keeps hosing for a moment after the target is gone. Its owner can also take direct control and aim it with the pointer. Siege maps ring the defenders' base with neutral gun turrets that either side can man; Supply crates and Transit Gate waves hand out gun, rocket and flame turrets that the player places where they are needed.
- **Barrier Emitters** are posts that raise lethal energy curtains between them. An enemy that walks into a raised curtain dies (a Worker catches fire), and every raised curtain blocks walking routes. On the reference maps the curtains stay up only while friendly Solar Arrays keep sending energy surges down the energy grid, so knocking out the Solar Arrays brings the barriers down.
- **Walls** block movement and fire and shrug off ordinary weapons; only the Breach Charge or the defenders' own scuttle explosion brings one down. They make Siege a battle for a breach.
- **Trees** are scenery that matters: they block movement and shots, catch fire, set nearby units alight, spread fire to their neighbours, burn down to a stump and regrow. Essence trees drop Essence orbs; blight trees kill Workers that come close and slowly corrupt the trees around them. Trees that a Supply crate plants also seed new trees.
- **Scenery meshes, exclusion zones, smoke columns and volcano vents** are props: solid rocks and ruins, invisible zones where no Supply crate lands, rising smoke and an erupting vent.
- **Jump pads** fling every Worker that stands on them through the air along a fixed arc, to whichever side they serve.
- **Hives and Spitter Plants** are hostile Supply crate outcomes owned by the Rogue faction: a Hive sends ants out to steal Essence orbs and carry off Workers, and a Spitter Plant lobs Seed Pods that burst into creatures, three times, before it bursts into flame and withers.
- **The Allegiance Scrambler** hands every Worker near it to a random team, again and again, for 15 seconds.

## 2 Rules

### 2.1 Rules shared by these objects

**R-BLDG-400** (MUST) The values of this document are the reference tuning baseline that Species may adopt (DEC-004), and this document is the specification of record for the objects of table 1-A (DEC-005). Every reference behaviour that looks accidental and that no Resolved decision corrects is stated as an explicit rule with a named key (DEC-006). The function, capture rules, ports and links of these kinds are kept, with the structure corrections applied (DEC-020, DEC-088).

**R-BLDG-401** (MUST) The shared structure rules of the [README](README.md) apply to every structure of this document: one advance per tick (R-BLDG-001, R-BLDG-002); integer tick timers, with every recount written with a one-second period running every `ref.mode.recount.one_second` (11 ticks) of reference/01 (R-BLDG-003); game speed as tick scheduling (R-BLDG-004); teams and friendship (R-BLDG-005 to R-BLDG-008); the damage sign convention (R-BLDG-009); ports, occupancy and reservation (R-BLDG-019 to R-BLDG-025); the lifecycle (R-BLDG-028 to R-BLDG-032); damage and destruction (R-BLDG-034 to R-BLDG-037); ownership changes (R-BLDG-038, R-BLDG-039); hit tests and meshes (R-BLDG-041 to R-BLDG-044); presentation (R-BLDG-045 to R-BLDG-048); sound events (R-BLDG-049); and map records (R-BLDG-050). This document states only what each kind adds or changes.

**R-BLDG-402** (MUST) The notation of the README applies: `U[a, b)` is a uniform real draw and `U{a..b}` a uniform integer draw, both from the simulation generator unless a rule names the presentation generator. Every draw is its own statement, made in the step that the rule names, and the draws of one step are made in the order in which the rule lists them (DEC-013, DEC-652). Cosmetic draws (particles, sparks, smoke, flicker, sound choice) come from the presentation generator and never reach simulation state (DEC-013, DEC-222; reference/12 R-SIM-622 and R-SIM-640). The simulation draws of these objects are listed in reference/12 R-SIM-630 Table 1.

**R-BLDG-403** (MUST) "Match time" is the tick count since match start × `ref.sim.tick_period` (R-BLDG-004), in seconds; "presentation time" is the presentation clock of the renderer. A rule that compares match time with a deadline is written here in ticks: a deadline of T seconds is 10 × T ticks at `ref.sim.tick_period` (0.1 s), and a random duration base + U[0, s) seconds is drawn as base × 10 + ⌈U[0, s × 10)⌉ ticks, which keeps the reference distribution.

**R-BLDG-404** (MUST) No step of these objects stores a non-finite value (DEC-653). Where the reference divides by a length or takes an arccosine, the rule states the guard: the turret aim cosine is clamped to [−1, 1] before its arccosine (DEC-653 item 8); a direction normalised from a zero vector is (0, 0, 1) (the tree generator's rule, R-BLDG-492); and a line-of-sight test over less than one step needs no division (R-BLDG-413).

### 2.2 Turret

#### 2.2.1 Structure and types

**R-BLDG-405** (MUST) A turret (`Turret`) is a manned weapon emplacement. It consists of a fixed mount, `mesh.structure.turret_mount`, which carries `ref.bldg.turret.ports` (4) port sockets `port_01` to `port_04` and the status sockets `status_01` to `status_04`; a head, `mesh.structure.turret_head`, which turns towards the aim point and carries the sockets `barrel_mount` and `kill_counter`; and a barrel set chosen by the turret's type, one variant of `mesh.structure.turret_barrel`, with `ref.bldg.turret.barrels` (4) barrels. Barrel i belongs to port i. The turret is owned by the team that mans its ports (R-BLDG-408), engages enemies on its own (§2.2.4) and can be aimed by its owner (§2.2.7).

**R-BLDG-406** (MUST) A turret has one of five types (`TurretType`), listed in this order, which is also the order of the type field of its map record (R-BLDG-407). The display name of each type is new text under the string id given.

Table 2.2-A Turret types.

| Type | Name string id | Barrel mesh variant | Weapon of each fired barrel | Reload | Minimum range | Maximum range | Barrels examined per fire step |
|---|---|---|---|---|---|---|---|
| gun turret (`TurretType::Gun`) | `building.turret.gun.name` | gun barrel (`casing_eject_01` to `casing_eject_04`, `muzzle_01` to `muzzle_04`) | a turret round (R-BLDG-420), or a Convert laser bolt from a tagged operator (R-BLDG-421) | `ref.bldg.turret.gun.reload_ticks` (1 tick) | `ref.bldg.turret.gun.min_range` (50 wu) | `ref.bldg.turret.gun.max_range` (225 wu) | `ref.bldg.turret.barrels_per_fire_step` (2) |
| rocket turret (`TurretType::Rocket`) | `building.turret.rocket.name` | rocket barrel | a rocket | `ref.bldg.turret.rocket.reload_ticks` (15 ticks) | `ref.bldg.turret.rocket.min_range` (100 wu) | `ref.bldg.turret.rocket.max_range` (200 wu) | `ref.bldg.turret.rocket.barrels_per_fire_step` (1) |
| mortar turret (`TurretType::Mortar`) | `building.turret.mortar.name` | gun barrel | a grenade thrown into the mortar target area | `ref.bldg.turret.mortar.reload_ticks` (40 ticks) | area-targeted; `ref.bldg.turret.mortar.min_range` (100 wu) is unused by its targeting | area-targeted; `ref.bldg.turret.mortar.max_range` (300 wu) is unused by its targeting | `ref.bldg.turret.barrels_per_fire_step` (2) |
| flame turret (`TurretType::Flame`) | `building.turret.flame.name` | flame barrel | a fireball | `ref.bldg.turret.flame.reload_ticks` (1 tick) | `ref.bldg.turret.flame.min_range` (10 wu) | `ref.bldg.turret.flame.max_range` (150 wu) | `ref.bldg.turret.barrels_per_fire_step` (2) |
| Convert Turret (`TurretType::Convert`) | `building.turret.convert.name` | cannon barrel | a Convert laser bolt | `ref.bldg.turret.convert.reload_ticks` (6 ticks) | `ref.bldg.turret.convert.min_range` (10 wu) | `ref.bldg.turret.convert.max_range` (150 wu) | `ref.bldg.turret.barrels_per_fire_step` (2) |

The reload tick counts are those of DEC-002 (rocket turret 1.5 s = 15 ticks, Convert Turret 0.5 s = 6 ticks, mortar turret 4.0 s = 40 ticks; gun and flame turrets 0.1 s = 1 tick). The gun turret's ranges are its base ranges of 100 wu and 300 wu scaled by 0.5 and 0.75 in matches.

**R-BLDG-407** (MUST) A turret's map record holds the base fields (R-BLDG-050) and then, each optional with its default:

| Field | Default | Meaning |
|---|---|---|
| type | gun turret | one of table 2.2-A |
| mortar target x | `ref.bldg.turret.default_mortar_value` (0) | x offset of the mortar target point from the turret, in wu |
| mortar target z | `ref.bldg.turret.default_mortar_value` (0) | z offset of the mortar target point |
| mortar target radius | `ref.bldg.turret.default_mortar_value` (0) | the mortar's search radius and scatter width, in wu |
| mortar force | `ref.bldg.turret.default_mortar_value` (0) | the mortar's throw force |

The four mortar fields are given together or not at all. A turret ignores the facing of its map record: initialisation sets its up to (0, 1, 0) and its front to (0, 0, 1), whatever the map says (see DEC-462). Its centre is its position and its radius is `ref.bldg.turret.radius` (30 wu).

**R-BLDG-408** (MUST) A turret is a structure of the README's capture model (table 2.7-A). Its ownership evaluation runs on its first advance and then every `ref.mode.recount.one_second` (11 ticks): it counts the operators of its four ports per team; the candidate is the first team in slot order with at least one operator, replaced by any later team with strictly more operators; if any operator is not a friend of the candidate, the result is no team; with no operator at all the result is no team. A single Worker therefore owns a turret, and co-op allies may man it together. The result is applied through the team-assignment operation (R-BLDG-038).

**R-BLDG-409** (MUST) A Convert Turret owned by a team that is not the Visitor faction, and a turret of any other type owned by the Visitor faction, damages itself by −`ref.bldg.turret.self_destruct_damage` (1000) at that ownership evaluation and is destroyed (R-BLDG-423).

#### 2.2.2 The turret step

**R-BLDG-410** (MUST) Each tick a turret's advance runs these steps in order:

1. On its first advance, it creates its bot navigation node (R-BLDG-426).
2. Its ownership timer counts down; at 0 it runs the ownership evaluation (R-BLDG-408) and restarts at `ref.mode.recount.one_second`.
3. Its fire timer decreases by 1 tick.
4. While it has a team and is not selected by its owner: targeting, aiming and the automatic fire decision (§2.2.3 and §2.2.4).
5. While it is selected by its owner: player control (§2.2.7).
6. Aim clamping and turning (R-BLDG-416).
7. The flame turret's friendly-fire guard (R-BLDG-417).
8. The overheat presentation (R-BLDG-430).
9. The base advance (R-BLDG-031), which evaluates its ports and requests its removal once it is destroyed.

A turret with no team only evaluates ownership and its ports; it neither searches nor fires.

#### 2.2.3 Targeting

**R-BLDG-411** (MUST) A turret with a team searches for a target on its first eligible advance and then every `ref.mode.recount.one_second` (11 ticks):

- **Mortar turret.** The target point is the turret's position + (mortar target x, 0, mortar target z). The target is `NearestEnemy(target point, 0, mortar target radius, team)` of the actor grid (reference/12 R-SIM-311): the nearest living enemy that is not a bot controller, the first in result order on equal distance. While it has a target, its aim point is the target point.
- **Every other type.** When `EnemiesInCells(position, maximum range, team)` reports no enemy (reference/12 R-SIM-313), there is no target. Otherwise it takes the exact enemy list `Enemies(position, maximum range, team)` in result order, skips every bot controller, every Collector and every dead actor, and keeps the nearest candidate whose distance is at least the type's minimum range and to which the line of sight is clear (R-BLDG-413). A candidate replaces the best so far only when it is strictly nearer, so on equal distance the first in result order wins.

When the chosen target differs from the previous one, the turret raises `turret.target_sighted`. Because both searches skip bot controllers, a turret never locks onto one (see DEC-311).

**R-BLDG-412** (MUST) When a search of a turret with a team finds no target, the turret sweeps idly. Its first idle aim point is drawn once, at its initialisation (DEC-652; reference/12 R-SIM-626): an angle U[0, 2π), a distance `ref.bldg.turret.sweep_distance_base` + U[0, `ref.bldg.turret.sweep_distance_spread`) (200 to 400 wu) and a height U[0, `ref.bldg.turret.sweep_height_first`) (0 to 100 wu), drawn in this order. Every later search without a target rotates the current aim direction about the vertical by U[−`ref.bldg.turret.sweep_turn` ÷ 2, `ref.bldg.turret.sweep_turn` ÷ 2) (±π/4) and moves the aim height by U[−`ref.bldg.turret.sweep_height_change` ÷ 2, `ref.bldg.turret.sweep_height_change` ÷ 2) (±25 wu).

**R-BLDG-413** (MUST) The line-of-sight test from a turret at p to a target at t samples the line from the turret towards the target (DEC-088): with d = |t − p| and n = ⌊d ÷ `ref.bldg.turret.los_step`⌋ (30 wu), the sample points are p + (t − p) × k ÷ n for k = 0 to n, or p and t when n = 0. For every structure other than the turret itself that is registered in the obstacle-grid cell of any sample point, the structure's ray test along the unit direction from p to t with length d is evaluated (R-BLDG-041); a hit blocks the line of sight. Walls, Barrier Emitters, trees, scenery meshes, smoke columns and other turrets therefore block a turret's shots.

#### 2.2.4 Aiming and the automatic fire decision

**R-BLDG-414** (MUST) While a turret has a target that exists and is alive, its aim point is the target's position + (`ref.bldg.turret.wobble_xz` × sin(`ref.bldg.turret.wobble_rate_xz` × t), |`ref.bldg.turret.wobble_y` × sin(`ref.bldg.turret.wobble_rate_y` × t)|, `ref.bldg.turret.wobble_xz` × cos(`ref.bldg.turret.wobble_rate_xz` × t)), with t the match time in seconds (R-BLDG-403): a wobble of ±20 wu in x and z and 0 to 10 wu up, at 3 and 2 rad per second. The mortar turret's aim point is its target point (R-BLDG-411).

**R-BLDG-415** (MUST) The automatic fire decision runs the fire step (R-BLDG-418) when the turret has a target, the horizontal angle between its front and the direction to its aim point is below `ref.bldg.turret.fire_cone` (0.3 rad), and its friendly-fire flag is clear. The angle is the arccosine of the dot product of the two horizontal unit vectors, clamped to [−1, 1] before the arccosine (DEC-653 item 8). Every tick on which the turret has a target records that tick; for `ref.bldg.turret.hose_window_ticks` (10 ticks) after the last such tick the turret keeps running the fire step at its last aim point although it has no target ("hosing").

**R-BLDG-416** (MUST) Every tick, an aim point closer than `ref.bldg.turret.min_aim_distance` (75 wu) to the turret is moved outward along the direction from the turret to `ref.bldg.turret.min_aim_distance`. The head then turns towards the aim point by `ref.bldg.turret.yaw_gain` (0.2) × the horizontal angle between its front and the aim direction, and the barrel's up vector blends towards the aim direction as up = up × (1 − `ref.bldg.turret.pitch_blend`) + aim direction × `ref.bldg.turret.pitch_blend` (0.2 per tick, 2 per second × the tick period). The muzzle sockets follow the head and the barrels.

**R-BLDG-417** (MUST) A flame turret sets its friendly-fire flag every tick when `CountFriendsInCells(position + front × ref.bldg.turret.flame.guard_offset, ref.bldg.turret.flame.guard_radius, team)` exceeds `ref.bldg.turret.flame.guard_friends` (more than 10 friends within 20 wu of the point 60 wu ahead), and clears it otherwise. The count is the cell-granular count of reference/12 R-SIM-312, so it includes friends up to about 45 wu beyond the radius. While the flag is set, the automatic fire decision does not fire and the kill counter shows the warning text (R-BLDG-427). The flag is evaluated after the fire decision of the same tick (R-BLDG-410), so the decision reads the flag of the previous tick. Other turret types have no friendly-fire guard.

#### 2.2.5 The fire step

**R-BLDG-418** (MUST) The fire step does nothing while the fire timer is above 0. Otherwise it examines barrels from its barrel cursor: two barrels, the cursor's barrel and the next one, or one barrel for the rocket turret (table 2.2-A). The cursor advances by one, modulo 4, past every examined barrel, whether or not its port is occupied. For each examined barrel whose port has an occupant (R-BLDG-021), in order:

1. three simulation draws (`ref.sim.rng.turret_sim_draws_per_barrel`, 3; reference/12 R-SIM-630): dx = U[−r ÷ 2, r ÷ 2), dz = U[−r ÷ 2, r ÷ 2) and df = U[−`ref.bldg.turret.mortar.force_spread` ÷ 2, `ref.bldg.turret.mortar.force_spread` ÷ 2), with r the mortar target radius. Every turret type makes these draws; only the mortar turret uses them;
2. the barrel fires its type's weapon from its own muzzle socket (`muzzle_0i` of the turret's own barrel mesh, DEC-461), carrying the turret's handle for kill credit (R-BLDG-420);
3. the casing ejection, which is presentation and draws from the presentation generator (`ref.sim.rng.turret_casing_draws_per_barrel`, 4; DEC-461): a gun turret ejects a shell casing (`mesh.projectile.shell_casing`) from the barrel's `casing_eject_0i` socket; the other types eject none;
4. the fire timer is set to the type's reload.

The second examined barrel fires in the same step even though the first has set the fire timer. An examined barrel whose port is unoccupied draws nothing and fires nothing.

**R-BLDG-419** (MUST) These cadences follow from R-BLDG-418, and the acceptance criteria test them: a gun or flame turret fires each manned barrel on every second tick, `ref.bldg.turret.gun.reload_ticks` = 1, so it fires 5 shots per second per manned port, up to 20 with all four ports manned; a mortar turret fires up to two grenades every 40 ticks; a Convert Turret fires up to two laser bolts every 6 ticks; a rocket turret fires one rocket every 15 ticks, and each examined unmanned barrel delays the next rocket by one more tick.

**R-BLDG-420** (MUST) Each fired barrel launches its type's weapon. The weapons' flight, damage, team filters, lifetimes and kill credit are specified in [04 Weapons and projectiles](../04_supply_crates_powers_and_weapons/weapons_and_projectiles.md); the turret supplies these launch values:

| Type | Weapon | Launch |
|---|---|---|
| gun turret | turret round (`TurretRound`, `mesh.projectile.turret_round`) of the turret's team | velocity v = barrel front × `ref.bldg.turret.gun.round_speed` (500 wu/s); start = muzzle − v × `ref.sim.tick_period` × `ref.bldg.turret.gun.start_back_fraction` (half a tick of travel behind the muzzle) |
| rocket turret | rocket (`RocketProjectile`) of the turret's team | from the muzzle towards the point muzzle + barrel front × `ref.bldg.turret.rocket.aim_distance` (500 wu), with a life of `ref.bldg.turret.rocket.life_ticks` (10 ticks) |
| mortar turret | grenade (`GrenadeProjectile`) of the turret's team | thrown from the muzzle at the point turret position + (mortar target x + dx, 0, mortar target z + dz) with force = mortar force + df (R-BLDG-418 step 1), a fuse of `ref.bldg.turret.mortar.fuse_ticks` (10,000 ticks, so it never times out in practice), exploding on impact |
| flame turret | fireball (`FireballProjectile`) with no team, so it harms every team | velocity barrel front × `ref.bldg.turret.flame.fireball_speed` (100 wu/s), with a life of `ref.bldg.turret.flame.fireball_life_ticks` (10 ticks) |
| Convert Turret | Convert laser bolt (`LaserBolt`, converting) of the turret's team | velocity barrel front × `ref.bldg.turret.convert.bolt_speed` (125 wu/s, a quarter of the turret-round speed); its life is the laser life of reference/04 at the fixed match laser level, 12 ticks, about 150 wu of range (see DEC-115) |

A turret round, a rocket and a grenade also create the shot's muzzle flash (reference/12 R-SIM-228).

**R-BLDG-421** (MUST) A gun turret fires a Convert laser bolt, launched as the Convert Turret's weapon of R-BLDG-420, instead of a turret round from every fired barrel whose port occupant carries a convert tag. The tag is set only by the Convert power and cleared by the Worker once that power has ended (DEC-331; [02 Worker](../02_units/worker.md) R-UNIT-126). The test is made per shot.

#### 2.2.6 Health, damage and destruction

**R-BLDG-422** (MUST) A turret has health `ref.bldg.turret.health` (60). Its damage operation adds the signed amount to its health (R-BLDG-009). Its damage comes from explosions (R-BLDG-035), from its own Visitor faction rule (R-BLDG-409) and from the turret-cap rule (R-BLDG-424).

**R-BLDG-423** (MUST) When a turret's health is at or below 0, it shatters its head and barrel meshes (shatter effects, presentation), sets its destroyed flag, destroys its bot navigation node (R-BLDG-426), and its advance requests its removal. Its weapons already in flight keep flying and keep their kill credit handle.

#### 2.2.7 Turrets placed by a power

**R-BLDG-424** (MUST) A turret placed by a turret power (the gun turret, rocket turret and flame turret outcomes of [04 Crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md), and the turret powers of Transit Gate waves, [05 Population flow](../05_population_flow.md) R-ECON-092 and R-ECON-104) is created at the power's target point with the power's turret type and **no team**, so it must be manned. The creation inserts it into the structure pool and updates the obstacle grid incrementally (R-BLDG-030), shatters its meshes once as a build-in effect (presentation), and shows the map marker `icon.power.gun_turret`; the outcome message is reference/04's. Then, by mode (R-MODE-077):

- **In a mode without the turret-cap option** (Spawn Control, Zone Control, Relic Run, Siege), the power ends at once. The turret still has no team, so ending the power leaves it standing, and it is an ordinary turret from then on.
- **In a mode with the turret-cap option** (Launch Race, Flag Chain), the power keeps running while the turret exists, and counts towards the cap ([01 README](../01_skirmish_rules_and_modes/README.md) R-MODE-077). Every tick of the running power, when the turret has a team and that team is not the power's team, the power damages the turret by −`ref.bldg.turret.self_destruct_damage` (1000), which destroys it. Ending the power destroys the turret the same way when the power's team owns it at that moment; a request to end the power is refused while another team owns the turret. Whether cancelling a power frees its Power slot when its turret stays is DEC-551.

A turret power's target point is invalid inside an exclusion zone that blocks powers (R-BLDG-517).

#### 2.2.8 Player control

**R-BLDG-425** (MUST) The owner of a turret may select it; the selection is local input of the owning team only, and the `Select` command sets the turret's fire timer to `ref.sim.team.turret_select_delay` (6 ticks) and puts the owner's camera into turret-aim mode (reference/12 R-SIM-405). While the turret is selected by its team, it makes no automatic search, aim or fire decision. Each tick its aim point is the `pointer_position` of its team's command state, and the fire step runs when that state carries `move_target`, `primary_fire` or `primary_held` (reference/12 R-SIM-439). Player-controlled fire is not blocked by the friendly-fire flag. The aiming cursor is `cursor.turret_aim` ([06 Controls, camera and commands](../06_controls_camera_and_commands.md)). Deselecting returns the turret to automatic control.

#### 2.2.9 Bot navigation node and kill counter

**R-BLDG-426** (MUST) On its first advance a turret creates a bot navigation node (`BotNavNode`) at its own position, marked as a turret node, with a new serial, and the bot navigation links are recomputed ([07 Bots](../07_bots.md)), so that bots contest the turret. The node is destroyed with the turret.

**R-BLDG-427** (MUST) A turret keeps a kill count per team slot. A kill made by a weapon that carries the turret's handle adds 1 to the count of the turret's team when the victim is an enemy and subtracts 1 when the victim is a friend, and raises `turret.kill_tick` (presentation); the kill also counts for the team (reference/12 R-SIM-221). The head's `kill_counter` socket shows the count of the turret's team at text size `ref.bldg.turret.kill_text_size` (2), or `ref.bldg.turret.kill_text_size_large` (1.5) when the count exceeds `ref.bldg.turret.kill_text_threshold` (999); while the friendly-fire flag is set it shows the new text `building.turret.friendly_fire_warning` ("Allies in the line of fire") instead.

#### 2.2.10 Presentation

**R-BLDG-428** (MUST) A turret with no team draws its head and barrels untinted. An owned turret draws its head tinted with its owner's team colour, scaled by `ref.bldg.turret.tint_weight` (0.2), with the co-op group shade variant of the colour (ADR-0020), and its barrels untinted. The tinted variants of every team colour and group shade are prepared at match setup (reference/12 02, initialisation step 6).

**R-BLDG-429** (MUST) A turret's sound events, besides the base events of README table 2.14-A, are: `turret.target_sighted` when a search finds a new target (silent, see DEC-565); `turret.fire_gun` for each turret round or gun-turret Convert bolt; `turret.fire_rocket` for each rocket (silent, see DEC-565); `turret.fire_grenade` for each mortar grenade; `turret.fire_flame` for each fireball; and `turret.kill_tick` for each credited kill (§2.14).

**R-BLDG-430** (MUST) Overheating is presentation state of the turret, never read by a rule. Each fire step that passes the fire-timer check raises the heat by `ref.bldg.turret.heat_per_fire` (1), up to `ref.bldg.turret.heat_max` (40), and restarts the cooling pause. When more than `ref.bldg.turret.cool_pause_ticks` (2 ticks) have passed since the pause restarted, the heat falls by `ref.bldg.turret.cool_step` (0.3), not below 0, and the pause restarts: the heat falls by 1.5 per second while the turret is silent. Each cooling step releases, at each barrel's muzzle, a number of smoke particles drawn from the presentation generator as ⌊U[0, ⌊1 + heat ÷ `ref.bldg.turret.smoke_heat_divisor`⌋)⌋ (heat ÷ 10).

#### 2.2.11 Turret aim marker

**R-BLDG-431** (MUST) Every turret owns one turret aim marker, an effect in the match world's effect pool that takes a simulated-object handle and therefore shifts the serials of later effects (reference/12 R-SIM-138). Every tick the marker moves to its turret's aim point while any port of the turret is occupied, and to the world origin otherwise; it takes its turret's team when the turret is a gun turret; and it removes itself once its turret no longer exists. No rule of a match reads it: Workers have no threat reaction to it in a match (DEC-052).

### 2.3 Barrier Emitter

**R-BLDG-432** (MUST) A Barrier Emitter (`BarrierEmitter`) is a post, `mesh.structure.barrier_emitter` (sockets `field_anchor_01`, `field_anchor_02` and `light_01`), that while raised projects an energy field to the **next** emitter named by its link; chains of linked emitters form barrier lines. An enemy actor that touches a raised field dies or, if it is a Worker, catches fire (R-BLDG-442); every raised field, of any team, blocks walking routes (R-BLDG-443). Its team comes from its map record and never changes; it has no ports and is never captured; it is indestructible (R-BLDG-034); it draws no lights (R-BLDG-045).

**R-BLDG-433** (MUST) A Barrier Emitter's map record holds the base fields and then:

| Field | Default | Meaning |
|---|---|---|
| link | none (required field) | the structure id of the next emitter; none ends the chain, and this post then draws and tests no field |
| scale | `ref.bldg.barrier_emitter.default_scale` (1.0) | uniform scale of the post, and so of the field height |
| initial mode | Lowered | one of the modes of R-BLDG-435 |

The content build rejects a link that names no structure or a structure that is not a Barrier Emitter (DEC-015). If such a link reached the simulation, every rule would treat it as no link; presentation never changes the link (DEC-007, DEC-222; reference/12 R-SIM-640 Table 4).

**R-BLDG-434** (MUST) Initialisation copies the three fields, sets the field status to 1 when the mode is Raised or AlwaysRaised and to 0 otherwise, and draws the first spark delay ⌈U[0, `ref.bldg.barrier_emitter.first_spark_spread_ticks`)⌉ ticks (0 to 100 ticks) from the simulation generator (reference/12 R-SIM-630 Table 1).

**R-BLDG-435** (MUST) A Barrier Emitter is in one of these modes (`BarrierMode`):

| Mode | Field status | Meaning |
|---|---|---|
| Lowered | 0 | down; a raise request starts Raising |
| Raising | rising | the field rises at `ref.bldg.barrier_emitter.raise_rate` per tick |
| Raised | 1 | up |
| Lowering | falling | the field falls at `ref.bldg.barrier_emitter.raise_rate` per tick |
| RelayOnly | stays 0 | never raises; forwards every raise and lower request to its linked emitter at once; draws and tests no field; still sparks |
| AlwaysRaised | 1 | permanently up; raise, lower and toggle requests and the energy timeout have no effect |

An emitter is **raised** when its mode is Raising, Raised or AlwaysRaised.

**R-BLDG-436** (MUST) The requests on an emitter are:

- **raise:** RelayOnly forwards the request to the linked emitter; Raised and AlwaysRaised stay as they are; every other mode becomes Raising;
- **lower:** RelayOnly forwards the request to the linked emitter; Lowered and AlwaysRaised stay as they are; every other mode becomes Lowering;
- **toggle:** Lowered and Lowering raise; Raised and Raising lower; RelayOnly and AlwaysRaised stay as they are.

Every request clears the emitter's hand-off flag (R-BLDG-437).

**R-BLDG-437** (MUST) Each tick a Barrier Emitter's advance runs these steps in order:

1. On its first advance, when its link resolves, its centre becomes the midpoint of the two posts and its radius half the distance between them plus its own radius, so that the whole line is covered by the visibility test and the obstacle grid.
2. While its mode is not Lowered, the spark delay counts down; at 0 the emitter sparks (R-BLDG-447) and the next delay is `ref.bldg.barrier_emitter.spark_base_ticks` + ⌈U[0, `ref.bldg.barrier_emitter.spark_spread_ticks`)⌉ ticks (81 to 120 ticks), drawn from the presentation generator.
3. The energy control (R-BLDG-438).
4. The mode step: in Raising the status rises by `ref.bldg.barrier_emitter.raise_rate` (0.03 per tick, 0.3 per second); when the status is at least `ref.bldg.barrier_emitter.handoff_status` (0.5), the emitter has a link and its hand-off flag is clear, it sends a raise request to the linked emitter and sets the flag; at a status of at least 1 the status becomes 1 and the mode Raised. In Lowering, when the status is at or below `ref.bldg.barrier_emitter.handoff_status`, tested before this tick's decrease, and the emitter has a link and a clear hand-off flag, it sends a lower request to the linked emitter and sets the flag; the status then falls by `ref.bldg.barrier_emitter.raise_rate`; at or below 0 it becomes 0 and the mode Lowered. Raised and AlwaysRaised hold the status at 1, Lowered holds it at 0, and RelayOnly leaves it unchanged.
5. The base advance (R-BLDG-031); the emitter has no ports.

A full raise takes 34 ticks and passes the request on at its 17th tick; a full lowering takes 34 ticks and passes the request on at its 18th tick, so a line of n emitters rises from one end to the other with a delay of 17 ticks per post.

**R-BLDG-438** (MUST) An emitter that has received energy at least once (R-BLDG-439) is in energy mode. Each tick in energy mode: when more than `ref.bldg.barrier_emitter.energy_timeout_ticks` (50 ticks, 5.0 s) have passed since its last energy delivery, the emitter sends itself a lower request if it is raised or RelayOnly; otherwise it sends itself a raise request if it is not raised. An emitter that has never received energy keeps its map mode for the whole match.

**R-BLDG-439** (MUST) The energy grid is the only active switch of Barrier Emitters (DEC-020). A grid terminal delivers an energy surge to its linked emitter only when the surge's team and the emitter's team are friends ([economy and spawn objects](economy_and_spawn_objects.md) R-BLDG-186), so an emitter with no team is never powered. A delivery puts the emitter into energy mode and records the tick; the first delivery also marks the emitter and every emitter downstream along the links as energy-linked, stopping at an AlwaysRaised emitter or at one already marked, so the marking ends on every chain. Only the directly powered emitter runs the timeout of R-BLDG-438; the emitters downstream follow it through the chain hand-off of R-BLDG-437. The command catalogue's toggle command for a Barrier Emitter toggles it (reference/12 06, `PlayerCommand::ToggleBarrierEmitter`); no player or bot action sends that command in the reference.

**R-BLDG-440** (MUST) The field height of an emitter is h_full = the height of socket `field_anchor_02` above socket `field_anchor_01` of the scaled mesh, `ref.bldg.barrier_emitter.field_height` (48.28 wu × scale). The field of an emitter with a link and a status s > 0 is the quad between its post and the linked post: from b − (0, h ÷ 3, 0) to b + (0, h, 0) at its own base b, with h = its own h_full × s, and from b′ − (0, h′ ÷ 3, 0) to b′ + (0, h′, 0) at the linked base b′, with h′ = the linked emitter's h_full × s. The skirt below each base is `ref.bldg.barrier_emitter.skirt_fraction` (1/3) of the height. The collision field equals the drawn field (DEC-088).

**R-BLDG-441** (MUST) The hit tests of a Barrier Emitter override the defaults (README table 2.12-A):

- **Lowered, or in the map editor:** every test is against the scaled post mesh only.
- **Ray test otherwise:** when the emitter has a link and s > 0, the ray is tested against the two field triangles (p1, p2, p4) and (p4, p3, p1), with p1 = b − (0, h ÷ 3, 0), p2 = b + (0, h, 0), p3 = b′ − (0, h′ ÷ 3, 0) and p4 = b′ + (0, h′, 0) of R-BLDG-440, and the hit normal is normalise((p1 − p3) × (p1 − p2)); when the field is not hit, the ray is tested against the scaled post mesh.
- **Sphere test otherwise:** the sphere (c, r) is approximated by two rays of length `ref.bldg.barrier_emitter.sphere_ray_factor` × r (2r) through c, one along +x and one along +z, each tested as above; then the post mesh.
- **Mesh test:** the sphere test with the other mesh's root sphere.

The obstacle grid registers a linked emitter along its whole line, as reference/12 R-SIM-322 specifies; the extra cells of that band only select candidates for these exact tests (DEC-088).

**R-BLDG-442** (MUST) A raised field acts on the actors that touch it, whatever its team, through the actor rules: an actor of a team that is not a friend of the emitter's team that touches a raised field with the kill flag set takes −`ref.sim.push.emitter_kill_damage` (9999) and is never pushed by the field (reference/12 R-SIM-381); a Worker is set on fire or pushed clear instead ([02 Worker](../02_units/worker.md) R-UNIT-113); a Transport is destroyed ([02 Transport](../02_units/transport.md) R-UNIT-377). Each kill raises `barrier_emitter.electrocute` at the actor's position. A friendly or lowered field is passed through.

**R-BLDG-443** (MUST) A raised field of any team blocks walking: the walkability test reports it, the walking cost treats it as an obstacle and the nearest-node scan does not take a node behind it (reference/12 R-SIM-335, R-SIM-338, R-SIM-346); the bot navigation graph skips links that cross a raised field of a team that is not a friend (DEC-087; [07 Bots](../07_bots.md)); and the Transport's sea-route test refuses a crossing of a raised field of a team that is not a friend (R-UNIT-384). Raising or lowering a field rebuilds no route table (reference/12 R-SIM-343).

**R-BLDG-444** (MUST) A Barrier Emitter is in view during the grace period whatever the camera sees, and its translucent pass is depth-sorted at its centre (README table 2.9-A). Its drawing reads simulation state only (DEC-007).

**R-BLDG-445** (MUST) The field is drawn in the translucent pass only while the mode is neither Lowered nor RelayOnly and the emitter has a link, double-sided, alpha-blended, without depth writes. Its colour is (`ref.bldg.barrier_emitter.neutral_rgb`, (100, 100, 100)) for an emitter with no team; for an energy-linked emitter, a pulse from `ref.bldg.barrier_emitter.energy_dark_rgb` (20, 20, 20) to `ref.bldg.barrier_emitter.energy_team_weight` (0.9) × the team colour with weight |sin(`ref.bldg.barrier_emitter.energy_pulse_rate` × t)| (4 rad/s over the presentation time t); and the team colour otherwise; the alpha is always `ref.bldg.barrier_emitter.field_alpha` (150 of 255). The drawn status is the status extrapolated by `ref.bldg.barrier_emitter.raise_rate` per tick over the presentation's extrapolation time, upward while Raising and downward while Lowering, and each post's own field height is used at its end of the quad (R-BLDG-440).

**R-BLDG-446** (MUST) While the structure-detail setting is below `ref.bldg.barrier_emitter.detail_layer_limit` (3), the field draws a base layer of `fx.barrier_field` combined with a multiplied layer of `fx.barrier_field_wide`, scrolling along the field by t ÷ `ref.bldg.barrier_emitter.scroll_divisor` (15), or by t ÷ `ref.bldg.barrier_emitter.energy_scroll_divisor` (1.5) with the texture repeated `ref.bldg.barrier_emitter.energy_tiling` (7) times for an energy-linked emitter. At every detail level it draws an additive layer of `fx.barrier_field_wide` and a smoothed line along the top edge, `ref.bldg.barrier_emitter.edge_width` (2.0) wide, or `ref.bldg.barrier_emitter.energy_edge_width` (0.5) for an energy-linked emitter. The visual change of each detail branch is decided with the renderer's quality levels (see DEC-327).

**R-BLDG-447** (MUST) A spark is presentation: it releases U{`ref.bldg.barrier_emitter.spark_count_min`..`ref.bldg.barrier_emitter.spark_count_max`} (5 to 10) spark particles of size U[25, 50) (`ref.bldg.barrier_emitter.spark_size_min`, `ref.bldg.barrier_emitter.spark_size_max`), flung perpendicular to the field at U[40, 60) wu/s (`ref.bldg.barrier_emitter.spark_speed_min`, `ref.bldg.barrier_emitter.spark_speed_max`) from a height U[0, `ref.bldg.barrier_emitter.spark_height` × scale) (50 wu × scale) above the post, all drawn from the presentation generator, and raises `barrier_emitter.spark`.

**R-BLDG-448** (MUST) A Barrier Emitter's sound events, besides the base events of README table 2.14-A, are `barrier_emitter.spark` (R-BLDG-447) and `barrier_emitter.electrocute` (R-BLDG-442; silent, see DEC-565). Its `barrier_emitter.spawn` event starts its presence loop, and its `barrier_emitter.damaged` event is raised by every damage, which changes nothing else (§2.14).

### 2.4 Wall

**R-BLDG-449** (MUST) A wall (`WallObject`) is a straight wall section, `mesh.structure.wall_section`, that blocks movement and the line of fire. It has no team that any rule reads (every reference wall has no team), no ports, and is never captured. Its main use is Siege, where the attackers breach the defenders' walls with the Breach Charge ([mode objects](mode_objects.md)).

**R-BLDG-450** (MUST) A wall's map record holds the base fields only. The bot-objective link field that reference map records carry has no effect and is not specified (DEC-466). A wall has scale 1.

**R-BLDG-451** (MUST) A wall's collision volume is a rectangle `ref.bldg.wall.length` (100 wu) long along right = normalise(front × up) and `ref.bldg.wall.depth` (20 wu) deep along front, centred on its position, with a ray height of `ref.bldg.wall.height` × `ref.bldg.wall.ray_height_fraction` (75 wu × 0.5 = 37.5 wu) above its base. This footprint is larger than the drawn section, so actors stop short of the visible wall (§4.13).

**R-BLDG-452** (MUST) The hit tests of a wall override the defaults (README table 2.12-A):

- **Sphere test:** the four corners of the rectangle and the sphere centre are all moved to y = 0, and the sphere is tested against the two triangles (t1, t2, t3) and (t2, t3, t4) of the rectangle with an exact sphere-to-triangle test; the wall is therefore an infinitely tall vertical prism for spheres.
- **Ray test:** a vertical quad along right, `ref.bldg.wall.length` long, centred on the position, whose bottom corners lie on the terrain under each end and whose top edge lies at the position's height + 37.5 wu.
- **Mesh test:** exact against the wall mesh's collision data (DEC-012).

**R-BLDG-453** (MUST) A wall's advance requests its removal when its destroyed flag is set; it never runs the base advance (R-BLDG-031). The removal recomputes the bot navigation links at once and updates the obstacle grid and the affected route-graph edges (R-BLDG-032).

**R-BLDG-454** (MUST) A wall's damage operation destroys the wall when a single damage amount is at or below −`ref.bldg.wall.destroy_threshold` (300): it plays a shatter effect of its mesh with strength `ref.bldg.wall.shatter_strength` (3.0) and sets its destroyed flag. A smaller hit does nothing: damage never accumulates, and the wall raises no `wall.damaged` event, because it does not run the base damage operation (see DEC-043).

**R-BLDG-455** (MUST) Exactly two damage sources reach the wall threshold: the Breach Charge's breach, which deals a lethal hit to every wall within its break radius, and the Breach Charge's scuttle explosion, which destroys every wall whose base point lies within 60 wu of it, because 500 × (1 − d ÷ 150) ≥ 300 exactly when d ≤ 60 ([mode objects](mode_objects.md); R-BLDG-035). No other explosion of the reference deals 300 or more to a structure; barrage missiles and meteors deal at most 100 (DEC-020, DEC-043).

**R-BLDG-456** (MUST) A wall acts on actors and routes through their rules: a Worker that hits it is pushed along the wall's front and its wall-hit count rises ([02 Worker](../02_units/worker.md) R-UNIT-114); the walkability test and the walking cost treat a wall as impassable (reference/12 R-SIM-335, R-SIM-338); and no bot objective binds to a wall ([07 Bots](../07_bots.md)).

**R-BLDG-457** (MUST) A wall is drawn as its mesh at (front, up, position) until it is destroyed, and its destruction is the shatter effect of R-BLDG-454 ([animation and VFX](../../architecture/components/animation_and_vfx.md)).

### 2.5 Tree

#### 2.5.1 Role, map record and state

**R-BLDG-458** (MUST) A tree (`TreeObject`) is scenery with gameplay: it blocks movement and shots with two hit spheres, catches fire, burns down to a stump and regrows, sets nearby actors on fire, spreads fire to nearby trees, and in its variants drops Essence orbs (an **essence tree**) or kills Workers and corrupts nearby trees (a **blight tree**). Trees are placed by maps, created by the Essence Grove and Blight Grove outcomes ([04 Crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md)) and created by other trees as offspring (R-BLDG-479). A tree has no ports and is never captured; its map record may give it a team, which no tree rule reads, and a tree created during a match has no team.

**R-BLDG-459** (MUST) A tree's map record holds the base fields and then these fields, each with its default; the ranges are the map editor's input ranges, which the content build also enforces (DEC-015):

| Field | Default | Range | Meaning |
|---|---|---|---|
| height | `ref.bldg.tree.default_height` (50 wu) | 1 to 1000, step 1 | uniform scale of the whole tree; the geometry is generated at unit height |
| leaf width | `ref.bldg.tree.default_leaf_width` (1.0) | 0 to 50, step 0.05 | width multiplier of the leaf quads only |
| push out | `ref.bldg.tree.default_push` (1.0) | 0 to 5, step 0.01 | sideways reach of child branches |
| push up | `ref.bldg.tree.default_push` (1.0) | 0 to 5, step 0.01 | upward reach of child branches |
| iterations | `ref.bldg.tree.default_iterations` (6) | 1 to 10 | recursion depth N: the trunk is level 1 and the leaves level N |
| seed | 0 | 0 to 99999 | seed of the tree generator |
| branch colour | (255, 255, 255, 255) | RGBA8 | colour of the branches |
| leaf colour | (255, 255, 255, 255) | RGBA8 | colour of the leaves |
| leaf drop rate | 0 | 0 to 50 | 0 = off; the rate of cosmetic falling leaves (R-BLDG-477) |
| essence drop rate | 0 | 0 to 50 | above 0 makes an essence tree (R-BLDG-478) |
| blight | false | false or true | a blight tree (R-BLDG-482) |

**R-BLDG-460** (MUST) Besides the structure state, a tree holds:

| Field | Initial value | Meaning |
|---|---|---|
| burn phase | 0, or −1 for an offspring | 0 is fully grown and not burning; above 0 is burning, up to 1; below 0 is regrowing, from −1 (a stump) up to 0 |
| fire damage | 0 | the ignition meter, 0 to `ref.bldg.tree.fire_damage_max` (100) |
| offspring budget | 0 for a map tree; the creator's value otherwise | the number of offspring it may still create |
| offspring deadline | unset | the tick of its next offspring |
| removable | false for a map tree; true for crate trees and their offspring | whether it is removed at the end of a burn |
| corruption timer | `ref.bldg.tree.corruption_first_ticks` (51 ticks, 5.0 s) | R-BLDG-483, R-BLDG-484 |
| corrupted | false | R-BLDG-483 |
| crown centre and crown radius | from collision data (R-BLDG-461) | the crown hit sphere at unit height |

#### 2.5.2 Generator and hit volumes

**R-BLDG-461** (MUST) A tree's geometry and its crown hit volume come from one deterministic generator that works in unit space, with the trunk from (0, 0, 0) to (0, 1, 0); the height is only a uniform scale applied at drawing and in the hit tests. The geometry is presentation. The crown centre and crown radius are collision data (DEC-012): for a tree that a map places they are baked by the content build, each centre stated explicitly; for a tree created during a match the simulation computes them once, at its creation, with the same generator and a generator state local to that tree, never a shared generator (reference/12 R-SIM-651). Every participant therefore holds the same crown volume for every tree.

**R-BLDG-462** (MUST) The tree generator uses its own 32-bit linear congruential generator: seeding sets the state s to the tree's seed; each draw sets s = (s × `ref.bldg.tree.rng_multiplier` + `ref.bldg.tree.rng_increment`) mod 2³² (214013 and 2531011) and returns (s >> 16) & `ref.bldg.tree.rng_mask` (0x7FFF), a value from 0 to 32767; and frand(r) = r × draw ÷ 32767, from 0 to r inclusive. The generator is re-seeded with the tree's seed at the start of every pass, so every pass reproduces the same branching.

**R-BLDG-463** (MUST) The generator runs three passes of this recursion over the same tree, written here in Species notation:

```
Branch(from, to, n, pass):
    if n == 0: return
    n = n - 1
    if n == 0:                                     # this segment is a leaf
        if pass == RADIUS: crownR = max(crownR, |to - crownC|)
        if pass == LEAF:   crownC = crownC + to; leafCount = leafCount + 1
    d = to - from
    A = normalise(d x to); B = normalise(A x d)    # normalise(v) = (0, 0, 1) when |v|^2 <= 1e-10
    len = |d|
    w = ref.bldg.tree.branch_width * (leaf_width if n == 0 else 1)      # 0.1
    if (n == 0 and pass == LEAF) or (n != 0 and pass == BRANCH):
        emit quad (from - A*len*w, from + A*len*w, to + A*len*w, to - A*len*w), uv (0,0) (0,1) (1,1) (1,0)
        emit the same quad with B in place of A    # two crossed planes per segment
    for dir in (A, -A, B, -B):                     # always four children, even below a leaf
        t = ref.bldg.tree.child_start_min + frand(ref.bldg.tree.child_start_spread)   # 0.3 + frand(0.6)
        childFrom = from + d * t
        childTo = childFrom + dir * len * ref.bldg.tree.child_reach * push_out + d * (1 - t) * push_up   # 0.4
        Branch(childFrom, childTo, n, pass)        # returns at once when n == 0

Generate():
    crownC = (0, 0, 0); crownR = 0; leafCount = 0
    seed(seed); Branch((0,0,0), (0,1,0), iterations, BRANCH)    # branch geometry
    seed(seed); Branch((0,0,0), (0,1,0), iterations, LEAF)      # leaf geometry; sums the leaf tips
    crownC = crownC / leafCount
    seed(seed); Branch((0,0,0), (0,1,0), iterations, RADIUS)    # no geometry
    crownR = crownR * ref.sim.integrity.tree_crown_radius_factor   # 0.8
```

The threshold 1e−10 is `ref.bldg.tree.normalise_epsilon`. For the trunk, d × to = 0, so A = (0, 0, 1) and B = (−1, 0, 0). At depth N there are 4^k segments at level k (k = 0 for the trunk to N − 1 for the leaves), 4^(N−1) leaf segments (1024 at N = 6) and (4^(N−1) − 1) ÷ 3 branch segments (341 at N = 6), two quads per segment, and each pass makes 4 draws per segment. The vector arithmetic is the binary64 arithmetic of reference/12, so the baked and the computed crown volumes are bit-identical on every participant.

**R-BLDG-464** (MUST) After generation a tree's structure centre is its position + crown centre × height, and its structure radius is crown radius × height × `ref.bldg.tree.radius_factor` (1.5). The in-view test is the sphere (position, height). Drawing may reduce the iteration count by (structure-detail level − 1), to at least `ref.bldg.tree.min_draw_iterations` (3), for its geometry only; the crown volume is always that of the full iteration count (R-BLDG-047).

**R-BLDG-465** (MUST) A tree's actual height is height × (1 − |burn phase|), and at least `ref.bldg.tree.min_height_fraction` (0.01) × height. Its hit tests use the actual height hA (README table 2.12-A): a sphere, mesh or ray query hits the tree when it hits the trunk sphere, centre the position and radius `ref.bldg.tree.trunk_radius` (10 wu, whatever the height), or the crown sphere, centre position + crown centre × hA and radius crown radius × hA. The crown centre is not rotated by the tree's facing (DEC-012); the drawn crown is, so the crown sphere sits slightly off the drawn crown for a tree that does not face the default direction.

#### 2.5.3 Fire, burn-out and regrowth

**R-BLDG-466** (MUST) A tree's damage operation raises `tree.damaged` (R-BLDG-034) and then, while its fire damage is below `ref.bldg.tree.ignite_threshold` (50), adds −amount to its fire damage. When the fire damage then exceeds the threshold: a fully grown tree (phase 0) starts burning at phase `ref.bldg.tree.ignite_phase` (0.01); a regrowing tree (phase below 0) resumes burning at the same height, its phase becoming −phase; a burning tree is unaffected; and the `tree.burning` loop starts if it is not already playing. A fire damage of exactly 50 neither ignites the tree nor accumulates further until it has decayed below 50.

**R-BLDG-467** (MUST) Every tick a tree's fire damage falls by `ref.bldg.tree.fire_decay_per_tick` (0.1, 1.0 per second) and is clamped to [0, `ref.bldg.tree.fire_damage_max`].

**R-BLDG-468** (MUST) Every tick that a tree burns (phase above 0), with hA its actual height and R its crown radius:

1. its phase rises by `ref.bldg.tree.burn_rate_per_tick` (0.002, 0.02 per second);
2. every other tree whose distance from it is at most hA × R × `ref.bldg.tree.radius_factor` + its own hA′ × R′ × `ref.bldg.tree.radius_factor` takes −`ref.bldg.tree.spread_damage` (1) damage;
3. every Hive whose distance from it is at most hA × R × `ref.bldg.tree.radius_factor` + `ref.bldg.tree.hive_reach` (100 wu) is burnt (R-BLDG-526);
4. every actor of every team within its burn range, hA × `ref.bldg.tree.burn_range_factor` (half its actual height), is affected: a Worker is set on fire ([02 Worker](../02_units/worker.md) R-UNIT-121), and any other actor takes −`ref.bldg.tree.actor_fire_damage` (1) fire damage;
5. it releases fire particles (R-BLDG-490).

With one burning neighbour adding 1 per tick against the decay of 0.1, a tree ignites after 56 ticks of contact.

**R-BLDG-469** (MUST) When a burning tree's phase exceeds 1.0, the burn ends: the phase becomes `ref.bldg.tree.stump_phase` (−1.0), a stump at 1 % of the height; a tree that is both corrupted and blight loses both marks, so fire cleanses it; and a removable tree stops its `tree.burning` loop and requests its removal. A tree that is not removable regrows. From ignition at 0.01 the burn lasts 495 ticks, about 49.5 s, with the phase accumulated in binary64 as in the reference.

**R-BLDG-470** (MUST) Every tick that a tree regrows (phase below 0), its phase rises by `ref.bldg.tree.regrow_rate_per_tick` (0.005, 0.05 per second), up to 0, where it is fully grown again. From the stump the tree regrows in 200 ticks, about 20 s, and is healthy (R-BLDG-471) from its 140th tick.

**R-BLDG-471** (MUST) A tree is **healthy** while its phase lies in `ref.econ.essence_grove.healthy_burn_range` ((−0.3, 0], reference/05): fully grown and not burning, or at least 70 % regrown. Only a healthy tree drops leaves (R-BLDG-477), drops Essence orbs (R-BLDG-478) and creates offspring (R-BLDG-479).

**R-BLDG-472** (MUST) The damage sources that ignite trees are, each with its values in its owning document: explosions (R-BLDG-035); turret rounds, fireballs and firebomb canisters ([04 Weapons and projectiles](../04_supply_crates_powers_and_weapons/weapons_and_projectiles.md)); lightning strikes, which damage the trees of their obstacle-grid cell ([04 Crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md)); Workers under the Speed Boost power that touch a tree ([02 Worker](../02_units/worker.md) R-UNIT-123); and burning neighbours (R-BLDG-468). Workers flee a burning tree within twice its burn range ([02 Worker](../02_units/worker.md), threat search step 4).

#### 2.5.4 Healthy-tree behaviours

**R-BLDG-473** (MUST) Each tick a healthy tree with a leaf drop rate L above 0 drops a cosmetic leaf when a presentation-generator integer draw modulo (`ref.bldg.tree.leaf_drop_base` − L) (51 − L) is 0: one leaf particle in the leaf colour at position + (0, h, 0) + (U[−h ÷ 2, h ÷ 2], U[−h ÷ 8, h ÷ 8], U[−h ÷ 2, h ÷ 2]), all drawn from the presentation generator, with h the actual height. Leaves never reach simulation state.

**R-BLDG-474** (MUST) Each tick a healthy tree that is not blight and whose essence drop rate E is above 0 makes one simulation draw U{0..2³² − 1} and, when it is divisible by (`ref.econ.essence_grove.drop_base` − E) (51 − E, reference/05), drops one Essence orb: it draws a distance U[0, h) and a yaw U[0, 2π), in this order, and spawns the orb at position + the point (distance, h, 0) rotated about the vertical by the yaw, with zero velocity, for a team slot drawn uniformly from `ref.econ.essence_grove.team_slots` (0 to 3, reference/05) by one draw U{0..3} ([02 Essence orbs](../02_units/essence_orbs.md)). The reference re-draws a slot from 0 to 5 until it is neither the Rogue slot nor the Visitor slot, which gives the same distribution; one draw needs no loop (DEC-014). The slot may belong to no team in play (see DEC-305). The per-tick probability is 1 ÷ (51 − E), so a tree of rate 15 drops about 0.28 orbs per second at game speed 1; the probability is per tick and scales with game speed (DEC-092). A blight tree never drops orbs.

**R-BLDG-475** (MUST) Each tick a healthy tree whose offspring budget is above 0 and whose offspring deadline is unset sets the deadline to now + `ref.bldg.tree.offspring_delay_base_ticks` + ⌈U[0, `ref.bldg.tree.offspring_delay_spread_ticks`)⌉ ticks (50 to 100 ticks). When the tick count reaches the deadline, the tree lowers its budget by 1, unsets the deadline and creates one offspring (R-BLDG-476, R-BLDG-479).

**R-BLDG-476** (MUST) Creating an offspring runs these steps in order:

1. It takes the centroid of the positions of every other tree within `ref.bldg.tree.offspring_neighbour_radius` (100 wu).
2. With H its actual height, it draws range = H + U[0, H); the draw is made whether or not it is used.
3. With at least one neighbour, the offset is (position − centroid) scaled to length H and rotated about the vertical by U[−`ref.bldg.tree.offspring_turn` ÷ 2, `ref.bldg.tree.offspring_turn` ÷ 2) (±π/8); with none, the offset is (range, 0, 0) rotated about the vertical by U[0, 2π). The candidate point is position + offset.
4. For each structure in the obstacle-grid cell of the candidate, while that structure's sphere test with radius `ref.bldg.tree.offspring_clearance` (1 wu) hits at the candidate, the candidate moves by the structure push of README table 2.9-A (`ref.bldg.structure.push_step_wu`, 2 wu, directly away from that structure's position). The push moves a fixed step away from a finite volume, so it ends; an implementation bounds it with a deterministic fallback that keeps the last candidate (DEC-014).
5. The candidate's height is set to the terrain height there; when that height is below 0, the candidate lies under the sea and no offspring is created.
6. Otherwise a tree is created with no team and a new serial, with every parameter of the parent copied (height, leaf width, push out, push up, iterations, seed, both colours, leaf drop rate, essence drop rate, blight flag, removable flag and the remaining offspring budget), and then, each its own draw in this order: its seed = ⌊U[0, `ref.bldg.tree.offspring_seed_max`)⌋ (99999) and its height = parent height × `ref.bldg.tree.offspring_height_factor` × (1 + U[−`ref.bldg.tree.offspring_height_spread` ÷ 2, `ref.bldg.tree.offspring_height_spread` ÷ 2)), that is 0.675 to 0.825 × the parent height. Its burn phase is set to −1, so it starts as a stump and regrows; its crown volume is computed (R-BLDG-461); it is initialised and the obstacle grid is updated incrementally (R-BLDG-030).

**R-BLDG-477** (MUST) The offspring budget of a tree is set only by its creator: map trees have none, and the grove outcomes give their trees a budget ([04 Crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md)). An offspring inherits the parent's remaining budget after the parent's decrement, so every tree of a grove spreads the grove further until the budgets run out.

#### 2.5.5 Blight trees and corruption

**R-BLDG-478** (MUST) Each tick a blight tree makes a draw U[0, `ref.bldg.tree.blight_roll_range`) (10) and, when it is below `ref.bldg.tree.blight_roll_below` (0.2), a 2 % chance per tick, takes `NearestEnemy(position, 0, hA × ref.bldg.tree.blight_reach_factor, Rogue faction)` (half its actual height; reference/12 R-SIM-311). When that actor is a Worker that is not operating a port, it takes −`ref.bldg.tree.blight_kill_damage` (999). When this kills it, the Essence orb that its death releases is removed at once, so it feeds no economy, and a Wisp is created at its position ([02 Creatures](../02_units/creatures.md) §2.11) with vertical speed `ref.bldg.tree.wisp_lift_base` + U[0, `ref.bldg.tree.wisp_lift_spread`) (20 to 45 wu/s) and its up vector tilted about its front by U[−`ref.bldg.tree.wisp_tilt` ÷ 2, `ref.bldg.tree.wisp_tilt` ÷ 2) rad, drawn in this order.

**R-BLDG-479** (MUST) While a blight tree is fully grown (phase 0), its corruption timer counts down by 1 tick per tick. At 0 the timer restarts at `ref.bldg.tree.corruption_check_base_ticks` + ⌈U[0, `ref.bldg.tree.corruption_check_spread_ticks`)⌉ ticks (100 to 150 ticks), and the first tree in structure order that is not blight, not corrupted and within the blight tree's actual height becomes corrupted: its essence drop rate becomes 0, its corrupted mark is set and its blight countdown starts at `ref.bldg.tree.blight_delay_ticks` (101 ticks). One tree at most is corrupted per check.

**R-BLDG-480** (MUST) Each tick a corrupted tree that is not yet blight counts its blight countdown down by 1; when it ends, the tree becomes a blight tree, and its corruption timer restarts at `ref.bldg.tree.corruption_first_ticks` (51 ticks), after which it corrupts trees itself (R-BLDG-479).

**R-BLDG-481** (MUST) The corruption marks form a cycle: normal, corrupted (101 ticks), blight, and, when a corrupted blight tree burns out, normal again (R-BLDG-469). A tree that a map places as blight, and a tree of the Blight Grove outcome, is blight without the corrupted mark, so burning never cleanses it.

**R-BLDG-482** (MUST) The blight flag of a map record or a creator makes a blight tree from its creation. A blight tree draws no leaves and plays the blight presence loop (R-BLDG-493).

#### 2.5.6 Tree presentation and sound events

**R-BLDG-483** (MUST) A tree draws nothing in the opaque pass; it is drawn in the translucent pass, depth-sorted at position + crown centre × height. The pass samples `fx.tree_canopy` with linear, mipmapped filtering, blends additively (source alpha, one), draws both faces and writes no depth. The transform is (front, up, position) with a uniform scale of the drawn height (R-BLDG-484). The branch geometry is drawn in the branch colour and then, unless the seasonal theme is active or the tree is blight, the leaf geometry in the leaf colour. The drawing reads simulation state only (DEC-007).

**R-BLDG-484** (MUST) The drawn height is the actual height × (1 + sin((t + serial × `ref.bldg.tree.sway_phase_per_serial`) × `ref.bldg.tree.sway_rate`) × `ref.bldg.tree.sway_amplitude`), with t the presentation time (a phase of serial × 2, a rate of 2 rad/s and an amplitude of 0.003, or `ref.bldg.tree.blight_sway_amplitude` (0.01) for a blight tree): the tree breathes as a whole, without per-branch motion.

**R-BLDG-485** (MUST) The drawing varies by the tree's state:

1. **Essence tree:** the leaf colour's blue channel pulses as B × f + 255 × (1 − f), with f = |sin(t + serial × `ref.bldg.tree.essence_pulse_phase_per_serial`)| (10).
2. **Blight tree:** the branches in `ref.bldg.tree.blight_branch_rgba` (0, 0, 0, 200) with ordinary alpha blending, and no leaves: a dark silhouette.
3. **Corrupted, not yet blight:** the tree is drawn twice. With f the fraction of the blight countdown already run (0 at corruption, 1 at blight, extrapolated over the presentation's extrapolation time), the first drawing darkens the branch colour and alpha by f and multiplies the leaf alpha by 1 − f, and the second draws the blight silhouette with branch alpha `ref.bldg.tree.blight_branch_rgba` alpha × f.
4. **Burning:** ordinary alpha blending; the branch colour is scaled towards black by min(phase × `ref.bldg.tree.burn_darken_factor`, 1) (10) at branch alpha 255; the leaves are black with alpha × (1 − phase). Over it, `ref.bldg.tree.burn_flare_count` (4) camera-facing `fx.starburst` quads i = 0 to 3 are drawn without depth test in colour `ref.bldg.tree.burn_flare_rgb` (0.8, 0.2, 0.1) with alpha `ref.bldg.tree.burn_flare_alpha` (0.5) × hA ÷ height, centred at position + (sin(2g + i) × i × `ref.bldg.tree.burn_flare_spread` (1.7), `ref.bldg.tree.burn_flare_height` (0.8) × hA + cos(2g + i), cos(2g + i) × i × 1.7), with g the match time, and half-size max(`ref.bldg.tree.burn_flare_size_factor` (2) × hA, `ref.bldg.tree.burn_flare_min_size` (5 wu)).
5. **Regrowing:** the leaves are invisible while |phase| is above `ref.bldg.tree.regrow_leaf_threshold` (0.5), and have alpha × (1 − 2 × |phase|) after that.
6. **Seasonal theme** (`SeasonalTheme`): the branches in `ref.bldg.tree.seasonal_branch_rgb` (180, 100, 50) and no leaves.

**R-BLDG-486** (MUST) A tree-detail preference above 1, which the options screen does not expose (default 1), multiplies the leaf alpha by `ref.bldg.tree.leaf_alpha_boost`^detail (1.3^detail), capped at 255, when the geometry is built. It changes presentation only.

**R-BLDG-487** (MUST) Each tick a burning tree releases ⌊⌊hA ÷ `ref.bldg.tree.fire_particle_height_step`⌋ ÷ structure-detail level⌋ fire particles (hA ÷ 12), each from the presentation generator: at a horizontal distance U[0, `ref.bldg.tree.fire_particle_radius_factor` × hA] (0.6 hA) in a random direction, at a height `ref.bldg.tree.fire_particle_base_factor` × hA + `ref.bldg.tree.fire_particle_rise_factor` × U[0, hA] (0.7 hA + 0.3 × U[0, hA]), with size `ref.bldg.tree.fire_particle_size_factor` × hA × (1 + U[−0.25, 0.25]) (2 hA ± 25 %). Particles never reach simulation state.

**R-BLDG-488** (MUST) A tree's sound events, besides the base events of README table 2.14-A, are: one presence loop chosen at initialisation, `tree.ambient_blight` for a blight tree, otherwise `tree.ambient_essence` for an essence tree, otherwise `tree.ambient_normal`; and the loop `tree.burning`, started once at ignition (R-BLDG-466) and stopped when a removable tree is removed at the end of its burn (R-BLDG-469). `tree.spawn` is the base spawn event (§2.14).

### 2.6 Scenery mesh

**R-BLDG-489** (MUST) A scenery mesh (`SceneryMesh`) places one scenery mesh as a solid, non-interactive obstacle: rocks, ruins and other landmarks. It has no team that any rule reads (every reference scenery mesh has no team), no ports, is never captured and is indestructible (R-BLDG-034). It has no lights, because its mesh is assigned after the structure's lights are collected (R-BLDG-016).

**R-BLDG-490** (MUST) A scenery mesh's map record holds the base fields and then its scale (default `ref.bldg.scenery_mesh.default_scale`, 1.0) and its mesh, one of the pieces of `mesh.scenery.prop` or none (the default). The content build rejects a record that names a mesh that does not exist (DEC-015).

**R-BLDG-491** (MUST) Initialisation copies the scale, assigns the mesh, and computes the centre and radius from the mesh at the scaled transform. Its advance is the base advance. Its hit tests are exact against the scaled mesh's collision data (DEC-012); with no mesh they are the default tests with radius `ref.sim.structure.default_radius` (13 wu; R-BLDG-042). It is drawn as its scaled mesh with renormalised normals; with no mesh nothing is drawn in a match.

### 2.7 Exclusion zone

**R-BLDG-492** (MUST) An exclusion zone (`ExclusionZone`) is an invisible circle that keeps randomly placed Supply crates out of its area and, optionally, blocks the powers whose target area is the whole map inside it, for example on starting islands. It has no team, no ports and no collision, is never captured and is indestructible.

**R-BLDG-493** (MUST) An exclusion zone's map record holds the base fields and then its size, the horizontal radius (default `ref.bldg.exclusion_zone.default_size`, 300 wu), and its power flag (default false).

**R-BLDG-494** (MUST) Every exclusion zone joins the match world's exclusion-zone list when it is created and leaves it when it is destroyed (reference/12 02, per-match state). The exclusion test IsExcluded(p, forPowers) is true when some zone of the list lies at a horizontal distance below its size from p (the zone's height replaces p's) and either forPowers is false or the zone's power flag is set.

**R-BLDG-495** (MUST) The exclusion test has exactly two callers: the random placement of a Supply crate, which rejects an excluded point with forPowers false ([04 Crates and powers](../04_supply_crates_powers_and_weapons/crates_and_powers.md)); and the target validity of a power whose target area is the whole map, which rejects a target point with forPowers true. The powers with a whole-map target area are the Shield, Speed Boost, the gun turret, rocket turret and flame turret powers, Swarm, Essence Grove, Blight Grove, Hive, Sickness, Brood, meteor shower and Fury (reference/04).

**R-BLDG-496** (MUST) An exclusion zone's advance does nothing. Its sphere and mesh tests never hit and its ray test hits only in the map editor (README table 2.12-A); it is drawn only in the map editor.

### 2.8 Smoke column

**R-BLDG-497** (MUST) A smoke column (`SmokeColumn`) emits a continuous rising column of grey smoke. Its map record holds the base fields only; it has no team that any rule reads, no ports, is never captured and is indestructible. Its advance never runs the base advance (R-BLDG-031).

**R-BLDG-498** (MUST) A smoke column has no hit-test override, so the default tests of a structure without a mesh apply (R-BLDG-042): it is an invisible obstacle, a sphere of radius `ref.sim.structure.default_radius` (13 wu) at its position, registered in the obstacle grid like every structure. It pushes actors out and blocks rays, turret lines of sight included (reference/12 R-SIM-328; see DEC-089).

**R-BLDG-499** (MUST) Each tick a smoke column releases `ref.bldg.smoke_column.particles_per_tick` (3) missile-trail particles at its position, each with velocity (U[−0.2, 0.2], 1, U[−0.2, 0.2]) scaled to the length `ref.bldg.smoke_column.particle_speed` (50 wu/s) and size `ref.bldg.smoke_column.particle_size_base` + U[0, `ref.bldg.smoke_column.particle_size_spread`] (200 to 300), all from the presentation generator. The missile-trail particle kind is specified in [animation and VFX](../../architecture/components/animation_and_vfx.md) (§4.16).

### 2.9 Volcano vent

**R-BLDG-500** (MUST) A volcano vent (`VolcanoVent`) is an atmospheric vent that erupts from time to time. It is visual only and deals no damage. Its map record holds the base fields only; it has no team that any rule reads, no ports, is never captured, is indestructible and never collides (README table 2.12-A). Its advance never runs the base advance (R-BLDG-031).

**R-BLDG-501** (MUST) A volcano vent's eruption state is presentation state. Each tick of the world step while it is idle, it starts an eruption when a presentation-generator integer draw modulo `ref.bldg.volcano_vent.eruption_odds` (1000) is 0 (a chance of 0.001 per tick, a mean wait of 1,000 ticks, 100 s at game speed 1); the eruption lasts `ref.bldg.volcano_vent.eruption_base_ticks` + ⌈U[0, `ref.bldg.volcano_vent.eruption_spread_ticks`)⌉ ticks (30 to 70 ticks), drawn from the presentation generator. The roll is made once per tick and therefore scales with game speed (DEC-092). Its draws are presentation draws (DEC-013, DEC-222; reference/12 R-SIM-622).

**R-BLDG-502** (MUST) Each tick of an eruption the vent releases `ref.bldg.volcano_vent.particles_per_tick` (10) volcano particles at its position, each with velocity (U[−0.5, 0.5], 1, U[−0.5, 0.5]) scaled to `ref.bldg.volcano_vent.particle_speed` (100 wu/s) and size `ref.bldg.volcano_vent.particle_size_base` + U[0, `ref.bldg.volcano_vent.particle_size_spread`] (300 to 500), all from the presentation generator. The volcano particle kind is specified in [animation and VFX](../../architecture/components/animation_and_vfx.md) (§4.16).

### 2.10 Jump pad

**R-BLDG-503** (MUST) A jump pad (`JumpPadObject`) launches the Workers that stand on it into a ballistic jump. It has no mesh, no team and no ports; it works for every team, is never captured and is indestructible. Its advance never runs the base advance (R-BLDG-031), so its destroyed flag is never honoured.

**R-BLDG-504** (MUST) A jump pad's map record holds the base fields and then its force, the launch speed in wu/s, and its angle, the elevation of the launch above its front in radians, both defaulting to `ref.bldg.jump_pad.default_value` (0).

**R-BLDG-505** (MUST) Each tick a jump pad's launch timer counts down; at 0 it restarts at `ref.bldg.jump_pad.launch_interval_ticks` (3 ticks, a 0.3 s countdown) and the pad launches. The launch vector is the pad's front rotated about the axis front × up by the angle, which raises it by the angle above the horizontal, scaled to the length force. Every Worker of every team within `ref.bldg.jump_pad.radius` (40 wu) of the pad (`InRange`, reference/12 R-SIM-311) that is on the ground and not inside a Transport gets: velocity = the launch vector, airborne, the Idle state, no waypoint and no orders. It then flies by the airborne step (reference/12 R-SIM-373).

**R-BLDG-506** (MUST) A jump pad's sphere and mesh tests never hit, and its ray test is against a sphere of radius `ref.sim.structure.default_radius` (13 wu; README table 2.12-A). It is drawn as an animated arrow strip of `marker.jump_pad`, `ref.bldg.jump_pad.arrow_length` (80 wu) long and ±`ref.bldg.jump_pad.arrow_half_width` (20 wu) wide along its launch direction, with brightness `ref.bldg.jump_pad.brightness_base` + `ref.bldg.jump_pad.brightness_pulse` × |sin t| (0.3 + 0.7 × |sin t|) over the presentation time t.

