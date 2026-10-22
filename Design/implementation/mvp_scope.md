# MVP scope

> Status: Draft · Prefix: MVP · Related: [owner decisions](../decisions/owner_decisions.md) (O50 to O55) · [vision, pillars and core loops](../gdd/00_vision_pillars_and_core_loops.md) · [world and regions](../gdd/01_world_and_regions/README.md) · [solution strategy](../architecture/04_solution_strategy.md) · [ADR index](../ADR/README.md) · [engine components](../architecture/components/README.md) · [reference library](../reference/README.md) · [assets](../assets/GameAssets.md)

## 1 Purpose

### 1.1 What the MVP is

The MVP is the smallest Species world game that one player enjoys for a whole run. A valley of four settlements, the steward's and three AI settlements, each grows from its Wellspring: its Workers gather, carry and build on their own, Essence becomes new Workers, and the settlements' territories grow until they meet. Contact creates grievances, grievances and claims lead to crowd battles of hundreds of glowing figures, and a run of eight world days ends in a renown standing and an epilogue. Later in the MVP the world keeps running while the steward is away, the deputy runs the settlement on default orders, and the steward comes back to an away report.

It is the first playable of Species (O55), it is built in the real codebase under `AGENTS.md` with nothing thrown away (O52), and it is judged by solo play first (O50). The design phase is paused after wave A (O51), so this document also states, as its own provisional rules, the parts of the unwritten areas (B1 to B7, C1, C3) that the MVP needs. Those rules are first guesses: the MVP's playtests (§6) produce the evidence from which those areas are designed later.

### 1.2 The three questions

The MVP exists to answer three questions, in this order:

1. **Is an hour of play fun?** Growing a settlement and fighting neighbours who attack for a reason (Playtest 1, at the end of M3).
2. **Does a run have an arc?** Whether there is still something to want after the settlement plateaus, and whether the steward wants to start another run (Playtest 1).
3. **Is coming back fun?** Whether returning to the deputy's away report, in a world that ran without the steward, is something to look forward to (Playtest 2, at the end of M4).

A no to the first question stops the MVP at M3 for a redesign; M4 is built only on a yes.

### 1.3 How to read this document

- **MVP requirements** (`MVP-nnn`) are what is built. Each cites the requirements it implements as written (`PIL`, `WLD`, `R-`, engine component ids and ADRs), the ones it narrows (and how), or the unwritten area it stands in for.
- **What is not named here is not built.** §5 lists the notable omissions and every Final requirement that the MVP narrows or defers.
- **Pace.** The MVP world runs at the session pace until M4 and at the always-on pace from M4 (O54). The tuning values of §4 are stated at the session pace, because Playtest 1 tunes them first; the pace column of §4 says how each one converts at the always-on pace. This is the one place where this document departs from the statement rule of PIL-103, and it changes no rule.
- **Vocabulary.** The terms of [CONVENTIONS](../CONVENTIONS.md) §8.15 apply. The MVP's one gathered resource is Crystal, counted in resource units (ru), and its two new structures are the dwelling and the storehouse (O57).

## 2 Rules

### 2.1 Form of play and scope

**MVP-001** (MUST) The MVP is the world game of PIL-001 for one steward: one world of one region, the steward's settlement and `mvp.world.ai_count` (3) AI settlements. Skirmish is not built (O55); its reference requirements keep their `adopt-skirmish` verdicts for later.

**MVP-002** (MUST) The MVP is built in four steps whose content and exit criteria §3 states: M0 foundations, M1 battle sandbox, M2 settlement, M3 neighbours and the run (Playtest 1), and M4 away play (Playtest 2). Each milestone ends with a build the steward can run and play, and no milestone starts before the previous one meets its exit criteria.

> **Amendment 2026-10-21 (O86).** M2's phase A, the headless settlement (tasks T2 to T10 of the [M2 plan](m2_plan.md)), starts before M1 meets its exit criteria, on a pull request of its own stacked on M1's last. Its phase B, which makes the settlement visible and playable, still waits for M1's exit, and the M2 plan stays a Draft until then.

> **Amendment 2026-10-22 (O88).** M2's phase B, which makes the settlement visible and playable (tasks T11 to T19 of the [M2 plan](m2_plan.md)), also starts before M1 meets its exit criteria. M1's criterion of a client at 60 frames per second, which the owner's first rendered run missed at the 99th percentile, stays open and is taken up at M2's T19; the M2 plan stays a Draft until M1 meets its exit criteria.

**MVP-003** (MUST) Every number of an MVP rule is a tuning value with a stable key in data (MVP-099); the values of §4 are first guesses that the milestones and playtests retune, and a retuned value changes the data and the table, never code.

**MVP-004** (MUST) The MVP follows the Final design wherever this document does not narrow it: the pillars (PIL-010 to PIL-016), the vocabulary (PIL-090, PIL-091), the world rules of A1 for one region, and the reference mechanics it names. Where it narrows a Final requirement, §5.2 says how and why.

**MVP-005** (MUST) A world is created from a seed that the steward may enter or that the game draws, and the seed is shown in the world's information panel, so that a run can be played again on the same terrain and layout (WLD-160).

### 2.2 World

**MVP-010** (MUST) A world is exactly one region of A1 (WLD-020 to WLD-022): a valley of side `wld.region.side` (5,120 wu) holding four settlement sites in the two-by-two pattern, its central resource site and its eight valley border areas, ringed by black-lit sea on every side. The sizing formula of WLD-004 is not used. The region has no crossing (WLD-024 needs two regions), so every site has the ninth home resource site of WLD-042 and WLD-082, and holds 11 resource units of its own (WLD-085) and at most 16 with claims (WLD-086 without the crossing border area). Where that ninth home resource site stands on the MVP's terrain is DEC-1000.

> **Amendment 2026-10-21 (M2 task T4; DEC-1000, Open).** M1's terrain puts the ninth home resource site's place on WLD-082's diagonal in the sea or on a slope on 93 of seeds 1 to 100; until the owner decides DEC-1000, it stands at the same distance at +84° from the diagonal, and every site keeps its 11 units.

> **Amendment 2026-10-22 (O87; DEC-1000 resolved).** The owner chose the place at +84° from the diagonal: every site keeps its 11 units at the same distances, and every seed's valley stays as M1 measured it.

**MVP-011** (MUST) Terrain is generated at world creation by the hybrid generator of WLD-160 with one world template, one biome (WLD-180) and one lighting preset, with no set pieces and no bridges, and is immutable for the life of the world. Of the region checks of WLD-163, the checks that apply to one region without a crossing are run, and a failed check re-rolls the seed. The server and the client generate the terrain from the seed with the same terrain library (TER-009) and compare a terrain hash at connection; a client whose hash differs refuses the world with a message.

**MVP-012** (MUST) Settlement sites follow WLD-040 to WLD-044 as written: each is a levelled plateau with its core of radius `wld.site.core_radius` (400 wu) around its centre (WLD-041) and its home cell (WLD-061). The steward's settlement takes the site the steward picks at world creation, and the AI settlements take the other three.

**MVP-013** (MUST) Each settlement belongs to its own Cladan, so a world has four Cladans with four distinct colours (PIL-007, PIL-014). An AI settlement's Cladan carries one of the three temperaments of MVP-057, and no two AI settlements of a world share a temperament.

**MVP-014** (MUST) A world has no free settlement site, no founding after creation, no stand-in AI settlements and no keeper; PIL-068, WLD-120 to WLD-127 and `pil.world.free_sites_min` are waived for the MVP, because one steward in one region has no late joiner.

**MVP-015** (MUST) At most `wld.scale.battles_per_region_max` (2) battles are in progress at once (WLD-285, O46); an attack that would exceed the cap waits at the gate of WLD-102 in the order of declaration.

**MVP-016** (MUST) A settlement sees its own territory and the area within `mvp.vision.unit_radius` (300 wu) of each of its Workers, Leaders and structures; this is its seen layer (WLD-242). Every settlement sees the public layer of WLD-242: terrain, sites, settlements and their Cladans, territories, cores, border areas and their holders, claims and battle results. AI settlements and the deputy act on these two layers only (PIL-084), and the server sends a client only what its settlement may see (MVP-097).

**MVP-017** (MUST) Navigation nodes are generated with the terrain, in place of the nodes that a reference map places by hand (R-SIM-331, R-BOT-027; O70): a point of a square lattice of spacing `mvp.nav.node_spacing` (320 wu) over the region is a node where the terrain is dry and walkable by the path test of R-SIM-335 to R-SIM-337, and one node more stands at each settlement site's centre, at the central resource site and at the middle of each valley border area. The nodes are numbered in a stable order from the terrain alone, with no draw. The land route graph, its next-hop tables and route building (R-SIM-330 to R-SIM-359), the threat search (R-UNIT-098) and the AI (MVP-058) use them as the reference uses a map's nodes, and they are the routes that WLD-100 and check 8 of WLD-163 measure.

### 2.3 Settlement, Essence and growth (stands in for B1)

**MVP-020** (MUST) A settlement starts with its Wellspring at its site's centre, `mvp.settlement.start_workers` (20) Workers, empty stores and its founding territory of radius `wld.territory.founding_radius` (500 wu) (WLD-060).

**MVP-021** (MUST) The Wellspring (`Wellspring`) can be neither destroyed nor captured (PIL-080). It holds the settlement's stores up to `mvp.store.wellspring_capacity_ru` (1,500 ru), it is a drop-off point for gathered resources, and new Workers emerge from its ports, as Workers leave a Spawn Node (R-BLDG-122 to R-BLDG-132, without capture, activation and bursts).

**MVP-022** (MUST) A settlement's population limit is `mvp.settlement.population_base` (40) plus `mvp.dwelling.population` (30) for each active dwelling, never above `mvp.settlement.population_max` (300), which is inside the range of O43.

**MVP-023** (MUST) Essence orbs are created by deaths as the reference states (R-UNIT-034 to R-UNIT-039, R-UNIT-301 to R-UNIT-329, R-ECON-066 to R-ECON-071). An orb inside a settlement's territory drifts to that settlement's Wellspring at `mvp.essence.drift_speed` (30 wu/s); an orb outside every territory follows the reference states and ascends. No Collector, Builder or Spawn Node delivery is involved (R-ECON-072 to R-ECON-081 are not used).

**MVP-024** (MUST) Growth that needs no death (PIL-056): while the settlement is below its population limit and its stores hold at least `mvp.essence.orb_cost_ru` (27 ru), its Wellspring turns that cost into one orb every `mvp.essence.wellspring_interval_s` (12 s).

**MVP-025** (MUST) An orb that reaches its Wellspring becomes a Worker at the Wellspring if the settlement is below its population limit; otherwise it waits there, glowing, up to `mvp.essence.wait_max` (20) waiting orbs, and an orb beyond that ascends. A waiting orb becomes a Worker as soon as there is room.

**MVP-026** (MUST) The territory radius is `wld.territory.founding_radius` plus `mvp.territory.radius_per_worker` (4 wu) for each Worker above the starting count, up to `wld.territory.max_radius` (1,400 wu), and grows inside the bounds of WLD-060: it stops at the coast, at the halfway line of each valley border area (WLD-062, WLD-063) and at the edge of the home cell. Territory never shrinks in the MVP, except that a border area lost by a claim leaves it (WLD-086).

> **Amendment 2026-10-22 (M2 task T6; [ADR-134](../ADR/ADR-134-territory-essence-growth-contact-and-the-seen-layer-at-m2.md)).** The territory is held as a raster of cells of `mvp.territory.cell_size` (16 wu): the cells within its radius of the Wellspring and inside the bounds, joined to the Wellspring's cell. The bounds are the coast bands, the halfway lines of the valley border areas, the edge of the home cell, `wld.territory.site_clearance` outside every other site's core, `wld.resource.wild_clearance` outside the central resource site and dry ground. A structure lies inside it when the cells its footprint covers are held (MVP-032), so it may overhang the edge by less than a cell.

**MVP-027** (MUST) Two settlements are in contact while their territories touch at a valley border area's halfway line; the server records each contact event as WLD-066 states, and contact is the territory-pressure cause of MVP-051.

### 2.4 Gathering, stores, construction and upkeep (stands in for B2 and B3)

**MVP-030** (MUST) The MVP has one resource kind, Crystal (O57), held in the settlement's stores in resource units (ru) as integer thousandths, so that every amount is exact (R16). Each resource unit of WLD-085 is one resource site's yield; a home or border resource site of one unit has `mvp.resource.ports_per_unit` (4) gathering ports, and the central resource site of three units has three times as many.

**MVP-031** (MUST) Gathering is a Worker task of its own task search (R-UNIT-087 to R-UNIT-097, extended): a Worker whose settlement priority is gathering looks for a free gathering port at a resource site inside its territory, or at the central resource site while its settlement holds it, within `ref.unit.worker.port_search_range`; it walks there, mans the port for `mvp.gather.port_ticks` (100 ticks), picks up a load of `mvp.gather.load_ru` (1 ru), carries it visibly to the nearest drop-off point (its Wellspring or a storehouse) and returns. A full store refuses the load and the Worker waits at the drop-off point.

> **Amendment 2026-10-21 (O84).** The gathering search is not bounded by `ref.unit.worker.port_search_range`: a gathering Worker takes the nearest free gathering port anywhere in its territory, or at the central resource site while its settlement holds it, because the nearest resource sites lie 350 wu from the Wellspring (WLD-082). The 100 wu stays the range of the idle Worker's port search (R-UNIT-090).

> **Amendment 2026-10-22 (M2 task T7; [ADR-135](../ADR/ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md)).** A gatherer walks to its port and to its drop-off point along routes of the land route graph. It puts its load down at the Wellspring's door or a built, active storehouse's entrance, whichever is nearest. The central resource site is gathered only while its settlement holds it by a claim, so not at M2.

**MVP-032** (MUST) The steward places a structure in its territory, outside the core's centre ring that keeps the Wellspring's ports clear, on ground whose slope the walkability rules allow (R-SIM-335 to R-SIM-338), not overlapping another structure, a resource site or a port. Placing pays its cost from the stores at once and queues a construction site; the queue holds at most `mvp.construction.queue_max` (8) sites, which are built in order.

> **Amendment 2026-10-21 (M2 task T5; [ADR-132](../ADR/ADR-132-placement-and-construction-at-m2.md)).** The core's centre ring has the radius `mvp.construction.center_ring_radius` (60 wu) about the Wellspring: a structure lies wholly outside it. Until T6 builds the territory's bounds (WLD-060), a structure lies wholly inside the disc of the territory's radius about the Wellspring; ground is walkable where the terrain is above the shore height and its normal's y component is at least `ref.sim.walk.min_normal_y` at the structure's position and at every one of its ports; and two structures overlap when their bounding spheres' footprints do, or when a port lies within its 5 wu radius of a footprint.

**MVP-033** (MUST) Construction is a Worker task of the task search: a Worker whose priority is construction mans one of the ports of the first queued construction site (`mvp.construction.ports_dwelling` (6) on a dwelling, `mvp.construction.ports_storehouse` (4) on a storehouse, `mvp.construction.ports_turret` (4) on a turret), and the site completes when its Workers have given `mvp.construction.work_per_ru` (1 Worker-second per resource unit of its cost). The structure rises visibly as it progresses (MVP-083).

**MVP-034** (MUST) The MVP has three structures besides the Wellspring:
- **Dwelling**: raises the population limit (MVP-022). It costs `mvp.cost.dwelling_ru` (300 ru), multiplied by `mvp.cost.dwelling_escalation` (1.10) for every dwelling the settlement already has, so that growth has diminishing returns (PIL-053).
- **Storehouse**: holds `mvp.store.storehouse_capacity_ru` (1,500 ru) more and is a drop-off point; it costs `mvp.cost.storehouse_ru` (200 ru).
- **Turret**: the reference turret (R-BLDG-405 to R-BLDG-423), whose ports Workers on guard duty man; it costs `mvp.cost.turret_ru` (400 ru). It is the only MVP structure that takes damage, and a destroyed turret leaves a construction site that its settlement may rebuild at half its cost.

> **Amendment 2026-10-22 (M2 task T5; [ADR-133](../ADR/ADR-133-the-gun-turret-at-m2.md)).** The turret is the reference gun turret. It fires for the team that mans it (R-BLDG-408) while it remains its settlement's structure, so an enemy that mans it turns it. A turret under construction takes no damage. A destroyed turret's site is not queued and gains no work until its settlement rebuilds it for `mvp.cost.turret_rebuild_fraction` (0.5) of its cost, which queues it.

**MVP-035** (MUST) Upkeep is paid from the stores continuously: `mvp.upkeep.worker_ru_per_hour` (6 ru) per Worker and `mvp.upkeep.structure_ru_per_hour` (30 ru) per structure other than the Wellspring. While the stores are empty, growth stops, and structures become inactive one at a time, the most recently completed first, every `mvp.upkeep.inactive_step_s` (30 s) until upkeep balances; an inactive structure has no effect and is shown as dark. Workers never die of an empty store.

> **Amendment 2026-10-22 (M2 task T7; [ADR-135](../ADR/ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md)).** Upkeep accrues in integer thousandths of a resource unit, exact over an hour. A Leader pays as the Worker it was, and a construction site pays none. Stores that fall short pay all they hold. While they are empty, a structure becomes inactive every `mvp.upkeep.inactive_step_s`, the most recently completed first; while they hold anything, the inactive ones become active again in reverse order. An inactive turret does not fire, and an inactive storehouse is no drop-off point.

**MVP-036** (MUST) The steward's settlement priorities are three shares that sum to 100 %: gathering, construction and guard duty, by default `mvp.priority.default_gather` (50 %), `mvp.priority.default_construct` (30 %) and `mvp.priority.default_guard` (20 %) (PIL-052). The task search assigns tasks so that the shares of Workers that are not following a Leader approach the set shares; a task with no work (no free port, an empty queue) releases its share to the others. Guard duty mans turret ports and otherwise gathers Workers near the Wellspring, from where the threat search of R-UNIT-098 and R-UNIT-099 sends them to fight.

> **Amendment 2026-10-22 (M2 task T7; [ADR-135](../ADR/ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md)).** Each Worker holds its task. An idle Worker that follows no Leader takes, at its own task search, the task with work furthest below its share of the settlement's Workers, once the tasks with no work have released their shares in proportion. Guard duty always has work: with no free turret port, a guard stands within `mvp.guard.gather_radius` (100 wu) of the Wellspring.

**MVP-037** (MUST) A Worker with no task wanders as the reference Worker does (R-UNIT-087 to R-UNIT-097), so that a settlement never stands still on screen. The settlement panel shows, for every Worker that is idle, why: no free port, the queue is empty, the store is full, or its share is met (PIL-054, PIL-020).

> **Amendment 2026-10-22 (M2 task T7; [ADR-135](../ADR/ADR-135-settlement-tasks-gathering-and-upkeep-at-m2.md)).** As guard duty always has work, a Worker has no task only when guard duty has no share and every task with a share has no work. It then gives that task's reason: the queue is empty for construction, otherwise no free port. A carrier at a full store gives "the store is full". "Its share is met" cannot arise from MVP-036's rule as built.

**MVP-038** (MUST) A settlement's strength (PIL-058) for the MVP's statistics is its Workers at `mvp.strength.worker_ru` (20 ru) each, plus its stores, plus each structure at its cost.

### 2.5 Battles (stands in for B4)

**MVP-040** (MUST) Battles use the reference crowd rules for Workers and Leaders as written: movement and routes (R-UNIT-105 to R-UNIT-115, R-UNIT-040 to R-UNIT-042, R-SIM-300 to R-SIM-389, as they apply to Workers and Leaders), combat (R-UNIT-101 to R-UNIT-104, R-UNIT-011 to R-UNIT-033, with the laser bolt and grenade of R-CRATE-415 to R-CRATE-418 and R-CRATE-440 to R-CRATE-445, the grenade drawn as `mesh.projectile.thrown_weapon`), death (R-UNIT-134), and Leaders with formations (R-UNIT-152 to R-UNIT-189). Special units, Supply crates, powers, creatures and hazards are not built. Every battle runs at full fidelity (O3), as everything in the MVP does (MVP-096).

**MVP-041** (MUST) An expedition is a Leader, the Workers following it (R-UNIT-096) and up to `mvp.expedition.size_max` (200) Workers in all. The steward forms one by promoting a Worker (R-UNIT-152) and drawing Workers from guard duty to it, and sends it by declaring an attack on a target: a raid on a settlement, or a claim battle at a claimed area (MVP-053).

**MVP-042** (MUST) Declaring an attack pays `mvp.conflict.attack_cost_ru` (300 ru), or `pil.conflict.unprovoked_cost_factor` (2) times as much for an unprovoked attack (PIL-063), warns the target at once, and holds the expedition at the target's territory edge until `mvp.conflict.warning_minutes` (5 minutes of world time) have passed since the warning (WLD-102, with the duration narrowed). No attack may be declared on a settlement under a respite (MVP-047).

**MVP-043** (MUST) A battle starts when a hostile expedition, released by the warning hold, enters the target's territory or the claimed area (PIL-041). The battlefield is the target's territory and `wld.view.settlement_margin` (800 wu) around it, or the claimed area and the same margin.

**MVP-044** (MUST) Defender's advantage (O2): inside its own territory, a defending settlement's Workers and Leaders take `mvp.defense.damage_taken_factor` (0.75) of the damage the reference rules give; the defender's turrets fight; and the Essence of every death inside the defender's territory drifts to the defender's Wellspring (MVP-023), within the bound of MVP-046.

**MVP-045** (MUST) A battle ends at the first of:
- **Objective held**: the attacker has Workers within `mvp.battle.objective_radius` (150 wu) of the objective, and no defender there, for `mvp.battle.objective_hold_s` (30 s). The objective of a raid is the target's Wellspring; of a claim battle, the claimed area's resource site. The attacker wins.
- **Loss cap reached**: the defender's losses reach its loss cap (MVP-046). The attacker wins, and its gain is the capped amount.
- **Rout**: the attacker has fewer than `mvp.battle.rout_share` (25 %) of the Workers it sent, or it withdraws by command. The defender wins.
- **Time**: `pil.battle.max_minutes` (20 minutes) pass (PIL-047). The defender wins.

**MVP-046** (MUST) Loss cap (stands in for B6): in one battle a defender loses at most `mvp.protection.loss_cap_worker_share` (30 %) of the Workers it had at the battle's start and `mvp.protection.loss_cap_store_share` (25 %) of its stores, and never falls below `mvp.protection.worker_floor` (20) Workers (PIL-080). A raid that wins moves the capped stores to the attacker, carried home by its surviving Workers; a claim battle that wins moves the claimed area. The net transfer of a battle to either side, stores, Workers and Essence together, is bounded by the losing side's cap, and Essence beyond the bound ascends (PIL-045).

**MVP-047** (MUST) Respite (stands in for B6): a defender that lost at least `mvp.protection.respite_min_loss_share` (10 %) of its strength in a battle starts a respite of `mvp.protection.respite_minutes` (20 minutes at the session pace) during which no attack may be declared on it, except that no respite starts after a battle that ended within `pil.battle.min_respite_minutes` (2 minutes) of its start or after an attack withdrawn before its battle (PIL-087).

**MVP-048** (MUST) Every battle ends in a battle result (PIL-046) that records the sides, the Workers lost, the stores and Essence moved, the claimed area if any, the grievance it created, the respite it started and the renown it gave. The steward sees the result of every battle in which its settlement is a side, and the public line of every other battle.

**MVP-049** (MUST) An AI settlement commands its side of a battle by the AI of MVP-058; the steward commands its side with the verbs of MVP-101; Workers that no Leader leads fight by the reference threat search (R-UNIT-098, R-UNIT-099).

### 2.6 Relations, claims and the AI settlements (stands in for B5 and B7)

**MVP-050** (MUST) Each Cladan holds grievances against settlements, and its anger against a settlement is the sum of the weights of those grievances (PIL-064). Every grievance shows its cause, its weight and how it fades, in the relations panel (MVP-102).

**MVP-051** (MUST) Grievances arise only from settlement-level actions (PIL-124), from three causes:
- **Territory pressure**: while two settlements are in contact (MVP-027), each one's Cladan gains `mvp.grievance.pressure_per_hour` (30) of weight per hour against the other.
- **Claim**: a claim made on an area a settlement holds gives its Cladan `mvp.grievance.claim_weight` (30) against the claimant.
- **Attack**: an attack declared on a settlement gives its Cladan `mvp.grievance.attack_weight` (40) against the attacker, `mvp.grievance.unprovoked_factor` (2) times as much for an unprovoked attack.

**MVP-052** (MUST) Every grievance loses `mvp.grievance.fade_share_per_hour` (10 %) of its weight per hour, `pil.conflict.settled_fade_factor` (4) times faster once its cause is removed (contact ended, claim withdrawn or settled; PIL-122). A battle result removes `mvp.grievance.battle_relief_won` (50 %) of the attacker's grievances against the target when the attacker wins, and `mvp.grievance.battle_relief_lost` (25 %) when it loses.

**MVP-053** (MUST) Claims (PIL-120, PIL-121): a settlement may claim the half of a valley border area that a neighbour holds (WLD-062, WLD-086), or the valley's central resource site. A claim on a held area is settled by a claim battle, which moves the area if the claimant wins, or by the claimant's withdrawal. A claim on the central resource site while nobody holds it is held after `mvp.claim.uncontested_minutes` (30 minutes) unless another settlement claims it in that time; then the claim is contested and settled by a claim battle at the site, outside every territory and so with no defender's advantage. A settlement's core and home cell are never claimable (WLD-061).

**MVP-054** (MUST) Easing (PIL-064): a steward eases a grievance that a Cladan holds against its settlement by tribute, sending stores that reduce the grievance's weight by one for every `mvp.relation.tribute_ru_per_weight` (10 ru), or by withdrawing a claim, which removes the claim's cause. Tribute is the reserved term of CONVENTIONS §8.15 used for this one rule; truce and alliance are not built.

**MVP-055** (MUST) An AI settlement declares an attack only on a settlement against which its Cladan's anger is at least its temperament's threshold (MVP-057), or on an area it has claimed (PIL-025), and only when it can field an expedition of at least `mvp.ai.attack_min_share` (60 %) of the target's Workers that it can see. It chooses a raid or a claim battle by its temperament. Its choice of target never reads whether the target's steward is present (PIL-086 holds by construction).

**MVP-056** (MUST) AI settlements run the settlement rules of this document as the steward does, through the same command path (MVP-094): priorities, construction placed from a layout that keeps its ports and paths clear, claims, attacks, tribute received and withdrawals. They hold grievances against each other and fight each other, so the valley lives beyond the steward's own settlement.

**MVP-057** (MUST) The three temperaments of the MVP set an AI settlement's anger threshold, its eagerness to claim and its response to tribute:

| Temperament | Anger threshold | Claims | Attack it prefers | Tribute |
|---|---|---|---|---|
| Bold | `mvp.temperament.bold_threshold` (40) | the central resource site early | raid | eases at the standard rate |
| Territorial | `mvp.temperament.territorial_threshold` (70) | every neighbour's half of its border areas | claim battle | eases at half the rate |
| Wary | `mvp.temperament.wary_threshold` (100) | only after being attacked | claim battle | eases at twice the rate |

*Table 1 — The temperaments of the MVP's AI settlements (MVP-013, MVP-055).*

**MVP-058** (MUST) The AI of AI settlements adapts the reference bot (R-BOT-001 to R-BOT-099) to the seen layer: its navigation graph and priority field (R-BOT-025 to R-BOT-071) are fed only from what its settlement sees (MVP-016), its dispatch and its formation Leaders (R-BOT-078 to R-BOT-094) command its Workers and Leaders, and it acts only through the command path, executed at the next tick (ADR-087). It decides every `mvp.ai.decision_ticks` (50 ticks) within the budget of ADR-080 item 2. The power handlers and mode logic of the reference bot are not built.

### 2.7 The run and renown (O53)

**MVP-060** (MUST) A world is a run of `mvp.run.length_hours` (8 hours of play at the session pace; 8 world days at the always-on pace). The world shows the time left, and no attack may be declared in its last `mvp.run.quiet_minutes` (15 minutes); battles in progress finish (WLD-221 narrowed to a fixed lifetime).

**MVP-061** (MUST) Settlements earn renown (PIL-125), never by stores taken: `mvp.renown.defense_held` (3) for each battle won as a defender; `mvp.renown.claim_won` (3) for each claim battle won as a claimant; `mvp.renown.grievance_eased` (1) for each 20 weight of grievance eased by tribute or by a withdrawn claim; and `mvp.renown.structure_per_100ru` (1) for each 100 ru of structures completed.

**MVP-062** (MUST) The standing of the four settlements by renown is visible throughout the run, with each settlement's renown broken down by source.

**MVP-063** (MUST) When the run ends the world becomes read-only and the epilogue opens: the final standing, each settlement's history (battles, claims, structures, its strength over the run) and the steward's statistics (PIL-125, WLD-224). The steward may reopen the epilogue of any ended world and starts a new run from the front end.

**MVP-064** (MUST) The server records, for every settlement, its strength (MVP-038) and its Worker count every `mvp.stats.sample_minutes` (5 minutes), and every growth step, battle, claim and grievance, so that the epilogue and the playtest measures of §6 come from the run itself.

> **Amendment 2026-10-22 (M2 task T8; [ADR-136](../ADR/ADR-136-settlement-commands-status-and-statistics-at-m2.md)).** The statistics are part of the world's state, saved and replayed with it. A growth step is a structure of the settlement built, its population reaching a new multiple of `mvp.stats.population_step` (10 Workers), or its territory taking in a resource site (the [M2 plan](m2_plan.md) §9, question 1). Each entry records the tick, the Worker count and the strength. Samples fall on the world ticks that are multiples of the sample period.

### 2.8 World clock, presence and the deputy (O54)

**MVP-070** (MUST) In M0 to M3 the world runs at the session pace (PIL-100, PIL-104) while the client runs it, on the integrated server in the client's process behind the in-process transport (O10, ADR-084, ADR-088 item 1); closing the client saves the world (MVP-075) and freezes it (O4). The session factor is `mvp.pace.session_factor` (24), so one hour of play is one world day of the economy, relations and protection timers (PIL-101 and WLD-200 narrowed from 12).

**MVP-071** (MUST) From M4 a new world may be created at the always-on pace (PIL-104: a world's pace is fixed for its life, so an M3 world stays session-paced). An always-on world runs in a headless server process (`SpeciesServer.exe`) that the client starts if it is not running and that keeps running after the client closes (O54, amending O35); the client connects to it over MsQuic on the loopback interface (O44; ADR-088's `QuicTransport`, ADR-089). The world freezes whenever that process is not running or the computer sleeps (O4), and each freeze is recorded with its cause and its world time (PIL-131).

**MVP-072** (MUST) Presence follows PIL-070: the steward is present while a client is connected and has given input within `mvp.presence.idle_minutes` (5 minutes), and away otherwise. A world at the session pace has no away state.

**MVP-073** (MUST) While the steward is away, the deputy runs the settlement on the default orders of PIL-079 and the priorities the steward last set: defend the settlement, keep gathering, finish the queued construction, never attack. In a battle at its settlement the deputy keeps the turrets manned and sends each Leader's formation to the objective; Workers without a Leader fight by the threat search. The deputy never declares an attack, makes or withdraws a claim, pays tribute, demolishes, queues construction or changes the priorities (PIL-083). While the steward is present the deputy does nothing (PIL-071).

**MVP-074** (MUST) A steward who returns sees the away report first (PIL-073): its summary (battles and their results, Workers and stores lost and gained, construction finished, grievances recorded, claims moved, freezes) on one page, readable in `pil.session.report_read_minutes` (2 minutes), and the list of every deputy action with the standing order that permitted it (PIL-026). The away report and the settlement status are server messages, not client logic (PIL-075, O6).

**MVP-075** (MUST) The server saves the world as a versioned snapshot every `mvp.persist.snapshot_seconds` (60 seconds of world time) and when it stops cleanly: it writes a temporary file, flushes it and replaces the previous snapshot by an atomic rename through the platform layer, keeping the last `mvp.persist.snapshots_kept` (3). At start it loads the newest snapshot that validates and records the time since that snapshot as a freeze of cause "unclean stop" when the previous stop was not clean (PIL-131). Saving starts at M2, because a session-paced world is continued across sessions. The journaled record store of O42 is deferred (§5.2).

**MVP-076** (SHOULD) The server SHOULD write the commands it executes since the last snapshot to a command journal beside it, so that a defect is reproduced from the snapshot and the journal on the same build (ADR-081 Level 1, ADR-088's journal).

> **Amendment 2026-10-22 (M2 task T9; [ADR-137](../ADR/ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md)).** The journal beside a snapshot is written when the next snapshot is saved, so after a crash the last snapshot has none. A stop is clean when the server left a marker naming the snapshot it opens. Freezes are kept in a world record that the snapshot holds and the state hash does not cover. Version 9 is the first saved; until M2's exit build, a version raise may refuse its predecessor instead of migrating it (MVP-095).

### 2.9 Look (O52)

**MVP-080** (MUST) The MVP looks good from M1 on: it is drawn in the renderer's visual language (O9; PIL-014) with flat-shaded low-poly facets (RND-426, RND-427, ADR-020), additive glow on a black world with no post-process glow (RND-416 to RND-420), the darkening layer and black fog (RND-421, RND-424), and the terrain wire grid (RND-428). Placeholder dots, untextured cubes and debug colours appear only in development views.

**MVP-081** (MUST) The terrain is the generated valley with its facet colours from one biome's ramp (`terrain.ramp`), its wire grid (`fx.terrain_grid`), the sea ring with its shore waves (`terrain.sea_tile`, `terrain.shore_wave_gradient`) and a sky layer (`fx.sky_cloud`), lit by one lighting preset.

**MVP-082** (MUST) Workers are camera-facing sprite cards drawn as instanced crowds (ADR-009; `sprite.worker.front`, `sprite.worker.side`, `sprite.worker.hurt`) with their glow and blob shadows (`fx.glow_round`, RND-424), animated procedurally (ADR-035) so that a viewer sees them walk, carry a load, gather, build, fight and fall; a carried load is visible on the card (`sprite.worker.carry_load`). Leaders are drawn with their figure and banner (`mesh.unit.leader_figure`, `marker.leader_banner`, `marker.leader_banner_tint`). Laser bolts, muzzle flashes, deaths and corpse fades follow the reference look (`fx.beam_short`, `fx.muzzle_flash`, `fx.particle_dot`), and Essence orbs glow and drift (`fx.glow_round`, RND-418).

> **Amendment 2026-10-21 (O75, O77; M1 task T16, [ADR-121](../ADR/ADR-121-workers-and-orbs-drawn-at-m1.md)).** A Worker is drawn with the reference's layers (VFX-118, R-UNIT-147): its own card laid flat on the terrain as its ground shadow, not a blob shadow, and the glow of stacked cards while it is under the local team's control, with a provisional walk bob and firing recoil held in data (O75). The Essence orbs glow with `fx.glow_round` alone (R-UNIT-330, O77); `fx.glow_cloudy` waits for M2's orbs drifting home (MVP-023).

**MVP-083** (MUST) The Wellspring (`mesh.structure.wellspring`), the dwelling (`mesh.structure.dwelling`), the storehouse (`mesh.structure.storehouse`) and the turret (`mesh.structure.turret_mount`, `mesh.structure.turret_head`, `mesh.structure.turret_barrel`, firing `mesh.projectile.turret_round`) are low-poly flat-shaded meshes with their collision assets, standing on the port pad `mesh.structure.structure_default`, with glow accents tinted in their Cladan's colour (ADR-020); a construction site shows the structure rising with its progress, an inactive structure is dark, and a completed structure flashes once (`fx.starburst`). Resource sites have a mesh of their own (`mesh.structure.resource_site`) that shows how many of their ports are manned. Every new asset is newly authored (O9) and carries its authorship record (PIL-027). How far a site is sunk, how dark an inactive structure is and how the flash and the glows look is DEC-1001.

> **Amendment 2026-10-22 (O89; M2 task T12, [ADR-140](../ADR/ADR-140-structures-drawn-and-the-stand-ins-at-m2.md); DEC-1001, Open).** A dwelling's and a storehouse's ports, which serve their construction alone at M2, show their pads and status stars only while the structure is a construction site; a resource site's and a turret's ports are drawn always. How far a site is sunk, how dark an inactive structure is and how the completion flash and the Wellspring's glow look are the provisional presentation keys of table 3, which the owner tunes after T12's run (DEC-1001).

**MVP-084** (MUST) A settlement's territory is drawn as a glowing line on the terrain in its Cladan's colour (`fx.beam_long`), border areas and claims are marked on it, and the area outside the steward's areas lies under black fog with the public layer drawn on it (WLD-244).

**MVP-085** (MUST) The client draws at `mvp.client.fps_target` (60 frames per second) at 1,920 × 1,080 with up to `mvp.client.figures_max` (1,500) figures on screen on the reference test machine (DBG-209), which is the owner's PC, pinned at M1 (O58), and draws the replica one tick behind with interpolation (ADR-084).

**MVP-086** (MUST) The MVP's meshes and textures are produced by development tools under `Tools/` (procedural shape kits and texture generators, or a digital content creation tool with an exporter) and written in the cooked formats the runtime loads (ADR-017, ADR-019), so that the runtime path is the one later content takes. The tools may use pinned third-party code restored by NuGet (O29, O30); nothing of them ships (R14).

**MVP-087** (SHOULD) From M3 the MVP SHOULD play a first sound set through the audio engine (ADR-022 to ADR-027): laser shots (`sfx.weapon.laser_bolt.spawn`), deaths (`sfx.unit.worker.die`), an orb becoming a Worker (`sfx.unit.essence_orb.collected`), a structure completed (`sfx.building.dwelling.complete`), the attack warning (`sfx.unit.worker.attack_horn`), and a battle starting and ending (`sfx.ui.battle.start`, `sfx.ui.battle.end`).

### 2.10 Seams kept for growth (O50)

The MVP builds only one region and one steward, but it keeps the seams that are cheap now and expensive to add later, so that friends and larger worlds (O1) need no rewrite of what it builds.

**MVP-090** (MUST) The client never reads or writes simulation state: it sends commands and receives the state its settlement may see through the `Transport` interface (ADR-084, O10), the in-process transport until M4 and MsQuic from M4 (MVP-071). The build enforces the include boundary of ADR-084 in the layout of ADR-090: GameClient includes no GameLogic or NeuronServer header, and only the client program's composition root includes `IntegratedServer.h`.

**MVP-091** (MUST) The simulation is the headless library GameLogic on NeuronServer (ADR-090), which reaches the operating system only through the platform layer and includes no graphics, audio or window code (ADR-083, O41, PLT-005); the server and the client's integrated server link the same library.

**MVP-092** (MUST) The simulation advances in whole ticks of `ref.sim.tick_period` (0.1 s, 10 ticks per second) with the slices in order (ADR-079, R-SIM-500 to R-SIM-516).

**MVP-093** (MUST) Randomness comes from keyed draws (ADR-082), and one build reproduces a run from a snapshot and its commands bit for bit (ADR-081 Level 1, with fixed-width types and total orders); a per-tick state hash is tested in the unit tests (DBG-147). The own deterministic maths functions and the cross-build Level 2 of ADR-081 are deferred.

**MVP-094** (MUST) Every command, from the steward, the deputy or an AI settlement, takes the one validated command path of ADR-087: checked on arrival and on execution and executed at the next tick in a keyed order.

**MVP-095** (MUST) The world state is serialised in a versioned format whose loader migrates every older version it accepts; a snapshot of a version it cannot migrate is refused with a message, never loaded in part.

**MVP-096** (MUST) The world steps its region as a region job (ADR-083, ADR-085) that owns its settlements, its battles and its Workers, so that more regions add jobs rather than a rewrite. Every settlement runs at full fidelity; the abstract model of O3, fidelity switching and the overload lever (O47) are deferred, and the fidelity state of PIL-057 is a field that is always full.

**MVP-097** (MUST) The server computes each settlement's seen layer (MVP-016) once per update, and the AI, the deputy and the replication to the client all read that one seen layer (ADR-086 item 1); the client receives no state outside it (PIL-084).

**MVP-098** (MUST) The away report, the settlement status and the default orders are server data, read and written through server messages (O6; PIL-075).

> **Amendment 2026-10-22 (M2 task T8; [ADR-136](../ADR/ADR-136-settlement-commands-status-and-statistics-at-m2.md)).** The settlement status is read by a request that the server answers itself, for a steward who commands the settlement. Its outcome comes at once, and the status of the next tick follows. The status holds the stores and their capacity, the population and its limit, the growth state and its cause, the priorities and the task counts, the idle Workers by reason, the inactive structures, the upkeep, the strength and the construction queue with its progress. A placement or rebuild that a check refuses reports the refusal as its outcome's reason (MVP-032, MVP-101).

**MVP-099** (MUST) Every tuning value lives in data with its stable key and is loaded at server start, never written as a literal in code (CONVENTIONS §4); the records follow the authoring syntax and schemas of ADR-062. The data model of a Cladan has a place for the modifiers that later stages add (PIL-007, O7, O11), empty in the MVP.

### 2.11 Engine minimum, controls and HUD (stands in for C1 and C3)

**MVP-100** (MUST) The engine of the MVP is the engine baseline's components at the size the rules above need: the platform layer (window per ADR-075, files, clock per ADR-074, threads, crash path), the renderer (ADR-001 to ADR-013, ADR-078 as their defaults state), the terrain component, animation and VFX (ADR-035, ADR-036), input for keyboard and mouse (ADR-028, ADR-029; no gamepad), the camera for the settlement view, the UI framework (ADR-030 to ADR-034) with its text drawn from the owner's two bitmap fonts, GameFont for every player-facing text and EditorFont for the development views, at whole-number scales (ADR-034 as amended by O56, O62 and O63), the data schemas (ADR-061, ADR-062), and diagnostics (logging ADR-067, PIX markers ADR-068, unit tests ADR-069, CI ADR-070). Where a component assumes an in-process snapshot, a match that every participant simulates, or a skirmish match frame, ADR-084 and this document govern (§5.3).

**MVP-101** (MUST) The steward's controls are those of the reference where the reference has them: the player camera (R-CTRL-450 to R-CTRL-497) for the settlement view, selection, and Leader orders and formations (R-UNIT-169 to R-UNIT-187). The MVP adds: placing a structure, setting the three priorities, promoting a Worker and forming an expedition, declaring an attack (with its cost and whether it is unprovoked shown before it is confirmed), withdrawing an expedition, making and withdrawing a claim, and sending tribute. Every one of them is a command of MVP-094.

> **Amendment 2026-10-21 (O82; M1 task T17c, [ADR-127](../ADR/ADR-127-left-button-verbs-and-order-feedback-at-m1.md)).** The buttons are Species' own, not the reference's: the primary button selects and gives the orders of the crowd and of the selected Leader, and the secondary button releases a selection, toggles a Leader and promotes a Worker (R-CTRL-041, R-CTRL-042 and R-CTRL-050 as amended). The selection circle takes the pointer's drag as well as its growth (R-CTRL-156), and the badge near the pointer counts the selected Workers (R-CTRL-366).

**MVP-102** (MUST) The HUD shows: the stores and their capacity, the Workers and the population limit, growth and the cause when it stops (MVP-035, MVP-037), the priorities, the construction bar and queue, a relations panel (each Cladan's anger toward the steward's settlement, every grievance with its cause, weight and fade, every claim), notifications (contact, claims, the warning with its countdown, battle start and end, structures completed), the battle result, the renown standing (MVP-062), the time left in the run, the away report (MVP-074) and the epilogue (MVP-063). Its icons, panels, fonts and strings are the UI rows of the [MVP assets](mvp_assets.md).

**MVP-103** (MUST) The front end of the MVP is one page: continue a world, start a new run (pace, seed, site), open the epilogue of an ended world, options for display and volume, and quit.

## 3 Milestones

| Milestone | Builds | Exit criteria |
|---|---|---|
| **M0 Foundations** | The solution `Species.slnx` with the libraries NeuronCore, NeuronClient, NeuronServer, GameShared, GameLogic and GameClient, the programs `Species.exe` and `SpeciesServer.exe` and their test projects (ADR-090, O60; AGENTS.md §2, §3; tasks in the [M0 plan](m0_plan.md)); `Build/CheckFormat.py`, `Build/CheckProjectFiles.py` and `Build/RunClangTidy.py` (AGENTS.md §6); a window with a D3D12 device and swap chain clearing to black; the server loop at 10 ticks per second behind the in-process transport; a snapshot of an empty world written and read | Debug and Release build; CI green with the three checkers gating; the client shows the server's tick count through the transport; the unit tests pass (MVP-090, MVP-091, MVP-092) |
| **M1 Battle sandbox** | The valley terrain generated and drawn (MVP-011, MVP-081); Workers and Leaders with movement, the threat search, combat, death and orbs (MVP-040); two crowds of 200 under the steward's and a simple AI's command; the camera and the battle verbs; the first look pass (MVP-080, MVP-082) | A battle of 400 figures runs at 10 ticks per second and the client at 60 frames per second (MVP-085); the reference test machine is pinned and recorded (O58), and the per-figure costs are measured on it and ADR-080's budgets confirmed or corrected (O43's first task); the steward finds a battle fun to watch and to steer, recorded in the session log (§6.3) |
| **M2 Settlement** | The Wellspring, Essence drift and growth (MVP-020 to MVP-027); gathering, stores, the three structures, construction, upkeep and the priorities (MVP-030 to MVP-038); the HUD for the settlement; snapshot saving and continuing (MVP-075); the structure and resource-site look (MVP-083, MVP-084) | One settlement with no neighbours grows from 20 Workers to its plateau with a growth step at least every `pil.settlement.growth_step_minutes` (20 minutes) of play in its first 5 hours, and plateaus after 4 to 6 hours of play; the steward finds it nice to watch for 10 minutes |
| **M3 Neighbours and the run** | Three AI settlements on the same rules under the seen layer (MVP-056 to MVP-058); contact, grievances, claims, attacks with the warning, battles at settlements and claimed areas, the loss cap and respite (MVP-041 to MVP-055); the relations panel; renown, the run, the epilogue and the statistics (MVP-060 to MVP-064); the first sound set (MVP-087) | Playtest 1 (§6.1) is run and recorded; the Proposed ADRs whose "Confirmed when" names the first playable are confirmed or amended on this milestone's build (§5.3) |
| **M4 Away play** | The always-on pace, the headless server process and the MsQuic connection (MVP-071), presence, the deputy and the away report (MVP-072 to MVP-074), freeze records; the ADR that records the MsQuic package and the amendments of ADR-088 and PLT-100 that it needs (§5.3) | Playtest 2 (§6.2) is run and recorded |

*Table 2 — The milestones of the MVP, their content and their exit criteria (MVP-002).*

> **Amendment 2026-10-21 (O80).** The front end of MVP-103 grows with the milestones: M2 builds its page with continue, a new world (seed and site), the display and volume options and quit; M3 adds the run and the epilogue of an ended world; M4 adds the choice of pace. M2 also builds the server's statistics of MVP-064 for one settlement (its strength, its Worker count and every growth step), because the evidence of M2's exit is measured with them; M3 adds the battles, claims and grievances. The plans of the two milestones are the [M2 plan](m2_plan.md) and the [M3 plan](m3_plan.md).

> **Amendment 2026-10-21 (O81).** M2 also builds the seen layer of MVP-016 on the server, computed once per update (MVP-097), and draws MVP-084's fog from it. Its readers, the AI settlements (MVP-058) and the replication narrowed to it, stay M3's.

> **Amendment 2026-10-21 (O84).** M2's exit is judged on the growth scene's own measurement (the [M2 plan](m2_plan.md) §5). Where that measurement cannot hold all three of a growth step at least every 20 minutes of play, a plateau after 4 to 6 hours and M3's first contact in hour 2 to 3, the plateau window yields first: the step and the contact hold, and the plateau may fall outside hours 4 to 6, though within the run. A plateau at `mvp.settlement.population_max` meets the exit.

Each milestone is a series of pull requests that cite the MVP requirements they implement, as AGENTS.md §6 asks for requirement ids; a milestone's exit is recorded in its milestone record in `playtests/` (§6.3).

## 4 Tuning table

All values are provisional first guesses, stated at the session pace (§1.3). **Pace**: `tick` means the value is the same at both paces (movement, battles, tick-rate events and real-time settings); `scaled` means that at the always-on pace a duration is multiplied by `mvp.pace.session_factor` and an amount per tick-rate event or per hour is divided by it; `—` means the value is not a time or a rate.

| key | value | unit | pace | status | requirement | playtest note |
|---|---|---|---|---|---|---|
| `mvp.pace.session_factor` | 24 | times | — | provisional | MVP-070 | One hour of play is one world day; Playtest 1 judges whether a run of 8 hours feels like a week of a settlement's life |
| `mvp.world.ai_count` | 3 | AI settlements | — | provisional | MVP-001 | Fixed by the four sites of one region |
| `mvp.world.region_cell` | 6 | grid cells on each axis | — | provisional | MVP-010 | The region's cell of the extent of WLD-001, aligned with the chunks (WLD-020); 30,720 wu from the origin (ADR-102) |
| `mvp.terrain.base_method` | 1 | `TileMethod` | — | provisional | MVP-011 | WLD-162's temperate set: the base tile averages a random opposite pair |
| `mvp.terrain.base_fractal_dimension` | 2.0 | fractal dimension | — | provisional | MVP-011 | A larger value gives a smoother valley floor (WLD-268) |
| `mvp.terrain.base_height_scale` | 0.25 | height scale | — | provisional | MVP-011 | — |
| `mvp.terrain.base_desired_height` | 150 | wu | — | provisional | MVP-011 | The base tile's highest sample before the cores are levelled |
| `mvp.terrain.base_lowland_smoothing` | 2.0 | lowland smoothing | — | provisional | MVP-011 | — |
| `mvp.terrain.guide_rim` | 70 | guide height (of 255) | — | provisional | MVP-011 | The guide grid's rim, edge and centre values raise the floor towards the centre |
| `mvp.terrain.guide_edge` | 80 | guide height (of 255) | — | provisional | MVP-011 | — |
| `mvp.terrain.guide_center` | 90 | guide height (of 255) | — | provisional | MVP-011 | — |
| `mvp.terrain.hill_method` | 0 | `TileMethod` | — | provisional | MVP-011 | The hill tiles average four parents |
| `mvp.terrain.hill_fractal_dimension` | 1.2 | fractal dimension | — | provisional | MVP-011 | — |
| `mvp.terrain.hill_height_scale` | 1.0 | height scale | — | provisional | MVP-011 | — |
| `mvp.terrain.hill_lowland_smoothing` | 1.0 | lowland smoothing | — | provisional | MVP-011 | — |
| `mvp.terrain.hill_desired_min` | 260 | wu | — | provisional | MVP-011 | A hill's desired height is drawn from this to the maximum (WLD-162) |
| `mvp.terrain.hill_desired_max` | 340 | wu | — | provisional | MVP-011 | Below `wld.terrain.peak_height_max` (400 wu) |
| `mvp.terrain.hill_inset` | 300 | wu | — | provisional | MVP-011 | WLD-162's distance of a hill tile inside the region border; see the WLD-162 amendment |
| `mvp.unit.leader_mesh_radius` | 6.7252 | wu | — | provisional | MVP-040 | The bounding radius `LeaderFigure.cmo` records; the actor grid uses twice it (R-UNIT-159) until the cooked mesh gives it (M1 plan T11, ADR-103) |
| `mvp.vision.unit_radius` | 300 | wu | — | provisional | MVP-016 | Raise if AI settlements miss obvious threats; lower if the fog never hides anything |
| `mvp.vision.cell_size` | 64 | wu | — | provisional | MVP-016, MVP-097 | The seen layer's raster: fine enough for the fog's edge, coarse enough to make every update |
| `mvp.nav.node_spacing` | 320 | wu | — | provisional | MVP-017 | About 200 lattice nodes on the valley's land; lower it if routes miss narrow passes, raise it if the next-hop tables cost too much |
| `mvp.sandbox.side_workers` | 200 | Workers | — | provisional | MVP-005 | Each side of M1's battle sandbox at its start (M1 plan §6, ADR-109); a side of 200 needs at least three formations |
| `mvp.sandbox.side_spread` | 60 | wu | — | provisional | MVP-005 | The radius each sandbox side spawns in around its site's centre |
| `mvp.sandbox.steward_site` | 0 | site index | — | provisional | MVP-005 | The local site order of WLD-040: 0 and 3 are diagonally opposite (O70) |
| `mvp.sandbox.ai_site` | 3 | site index | — | provisional | MVP-005 | The AI's side, opposite the steward's |
| `mvp.perf.synthetic_figures` | 2000 | figures | — | provisional | MVP-093 | ADR-080 item 2's synthetic battle, in two halves with an AI on each; M1's determinism tests and T10 run it |
| `mvp.perf.synthetic_gap` | 200 | wu | — | provisional | MVP-093 | The distance between the synthetic battle's halves, across the valley's centre, so that it is fought from its first ticks (ADR-110) |
| `mvp.settlement.start_workers` | 20 | Workers | — | provisional | MVP-020 | The first minutes must show a busy settlement |
| `mvp.settlement.population_base` | 40 | Workers | — | provisional | MVP-022 | — |
| `mvp.settlement.population_max` | 300 | Workers | — | provisional | MVP-022 | Inside O43's range; M1's cost measurement may raise it |
| `mvp.dwelling.population` | 30 | Workers | — | provisional | MVP-022 | Tuned to M2's growth-step target |
| `mvp.essence.orb_cost_ru` | 27 | ru | — | provisional | MVP-024 | M2 task T10 (ADR-138): 27 from 20, which the growth scene's plateau came before hour 4 with (3.81 to 3.85 hours on seeds 1 to 8); at 27, with the escalation at 1.10, the plateau comes at 4.27 to 4.32 hours, the largest gap is 14.9 to 15.4 minutes and the halfway line falls at 2.65 to 2.67 hours |
| `mvp.essence.wellspring_interval_s` | 12 | seconds | scaled | provisional | MVP-024 | Caps peaceful growth at 300 Workers per hour of play |
| `mvp.essence.drift_speed` | 30 | wu/s | tick | provisional | MVP-023 | Orbs drifting home should be seen, not missed |
| `mvp.essence.wait_max` | 20 | orbs | — | provisional | MVP-025 | — |
| `mvp.territory.radius_per_worker` | 4 | wu per Worker | — | provisional | MVP-026 | Contact between neighbours at about 150 Workers, in hour 2 to 3 of a run |
| `mvp.territory.cell_size` | 16 | wu | — | provisional | MVP-026 | The territory's raster: a height-field cell's side (O83), fine enough that a structure's footprint test reads its edge |
| `mvp.resource.ports_per_unit` | 4 | ports | — | provisional | MVP-030 | 44 gatherers at full territory; sets the plateau with the upkeep |
| `mvp.gather.port_ticks` | 100 | ticks | tick | provisional | MVP-031 | — |
| `mvp.gather.load_ru` | 1 | ru per load | scaled | provisional | MVP-031 | About 60 ru per gatherer per hour of play |
| `mvp.store.wellspring_capacity_ru` | 1500 | ru | — | provisional | MVP-021 | — |
| `mvp.store.storehouse_capacity_ru` | 1500 | ru | — | provisional | MVP-034 | — |
| `mvp.construction.queue_max` | 8 | construction sites | — | provisional | MVP-032 | — |
| `mvp.construction.center_ring_radius` | 60 | wu from the Wellspring | — | provisional | MVP-032 | Clears the Wellspring's ports at 38 wu and their 5 wu radius, with room to stand |
| `mvp.construction.ports_dwelling` | 6 | ports | — | provisional | MVP-033 | Matches the dwelling mesh |
| `mvp.construction.ports_storehouse` | 4 | ports | — | provisional | MVP-033 | Matches the storehouse mesh |
| `mvp.construction.ports_turret` | 4 | ports | — | provisional | MVP-033 | The turret's four manning ports (R-BLDG-405 to R-BLDG-423) |
| `mvp.construction.work_per_ru` | 1 | Worker-seconds per ru | scaled | provisional | MVP-033 | A dwelling takes 6 Workers under a minute early on |
| `mvp.cost.dwelling_ru` | 300 | ru | — | provisional | MVP-034 | Tuned to M2's growth-step target |
| `mvp.cost.dwelling_escalation` | 1.10 | times per dwelling built | — | provisional | MVP-034 | M2 task T10 (ADR-138): 1.10 from 1.15, whose last dwellings' waits made the largest gap (17.3 to 17.8 minutes at an orb cost of 26); at 1.10 the largest gap is the first step's wait, about 15 minutes |
| `mvp.cost.storehouse_ru` | 200 | ru | — | provisional | MVP-034 | — |
| `mvp.cost.turret_ru` | 400 | ru | — | provisional | MVP-034 | — |
| `mvp.cost.turret_rebuild_fraction` | 0.5 | of the turret's cost | — | provisional | MVP-034 | A destroyed turret's site is rebuilt at half its cost |
| `mvp.upkeep.worker_ru_per_hour` | 6 | ru per Worker per hour | scaled | provisional | MVP-035 | With the ports, puts the plateau near 280 Workers on 11 units |
| `mvp.upkeep.structure_ru_per_hour` | 30 | ru per structure per hour | scaled | provisional | MVP-035 | — |
| `mvp.upkeep.inactive_step_s` | 30 | seconds | scaled | provisional | MVP-035 | Decline must be visible before it hurts (PIL-054) |
| `mvp.priority.default_gather` | 50 | % | — | provisional | MVP-036 | — |
| `mvp.priority.default_construct` | 30 | % | — | provisional | MVP-036 | — |
| `mvp.priority.default_guard` | 20 | % | — | provisional | MVP-036 | — |
| `mvp.guard.gather_radius` | 100 | wu from the Wellspring | — | provisional | MVP-036 | Guard duty with no turret port gathers here, near enough that the threat search of R-UNIT-098 covers the core |
| `mvp.steward.think_s` | 2 | seconds | tick | provisional | MVP-056 | How often the scripted steward looks at its settlement (the [M2 plan](m2_plan.md) §5, ADR-138) |
| `mvp.steward.dwelling_margin` | 5 | Workers | — | provisional | MVP-056 | A dwelling is placed once the population comes within this of its limit, as in the M2 economy model |
| `mvp.steward.ring_first` | 140 | wu from the Wellspring | — | provisional | MVP-056 | The dwellings' first ring, outside the centre ring |
| `mvp.steward.ring_step` | 70 | wu | — | provisional | MVP-056 | — |
| `mvp.steward.rings_max` | 5 | rings | — | provisional | MVP-056 | Inside the founding territory |
| `mvp.steward.slot_spacing` | 70 | wu | — | provisional | MVP-056 | Between neighbouring places on a ring, clear of a dwelling's ports |
| `mvp.steward.lane_half_width` | 40 | wu | — | provisional | MVP-056 | No dwelling within this of the line from the Wellspring to a home resource site |
| `mvp.steward.storehouse_search_step` | 20 | wu | — | provisional | MVP-056 | — |
| `mvp.steward.storehouse_search_rings` | 5 | rings | — | provisional | MVP-056 | Around the midpoint of two neighbouring outer resource sites |
| `mvp.strength.worker_ru` | 20 | ru per Worker | — | provisional | MVP-038 | The orb cost's first guess; T10 leaves it, as it is no lever of the M2 plan's §9, question 2 |
| `mvp.expedition.size_max` | 200 | Workers | — | provisional | MVP-041 | With the defenders, a battle of 300 to 450 figures (PIL-021) |
| `mvp.conflict.attack_cost_ru` | 300 | ru | — | provisional | MVP-042 | An attack must cost a dwelling's worth |
| `mvp.conflict.warning_minutes` | 5 | minutes of world time | tick | provisional | MVP-042 | Narrows WLD-102's 10; Playtest 2 checks it is enough when the deputy defends |
| `mvp.defense.damage_taken_factor` | 0.75 | of reference damage | tick | provisional | MVP-044 | Defending should win most even fights |
| `mvp.battle.objective_radius` | 150 | wu | — | provisional | MVP-045 | — |
| `mvp.battle.objective_hold_s` | 30 | seconds | tick | provisional | MVP-045 | — |
| `mvp.battle.rout_share` | 0.25 | of Workers sent | — | provisional | MVP-045 | Battles of 4 to 12 minutes (PIL-047) |
| `mvp.protection.loss_cap_worker_share` | 0.30 | of Workers at battle start | — | provisional | MVP-046 | Playtest 2: back to strength within a day after a night of attacks (PIL-022) |
| `mvp.protection.loss_cap_store_share` | 0.25 | of stores | — | provisional | MVP-046 | — |
| `mvp.protection.worker_floor` | 20 | Workers | — | provisional | MVP-046 | Equal to the starting count |
| `mvp.protection.respite_minutes` | 20 | minutes | scaled | provisional | MVP-047 | 8 world hours at the always-on pace |
| `mvp.protection.respite_min_loss_share` | 0.10 | of strength | — | provisional | MVP-047 | — |
| `mvp.grievance.pressure_per_hour` | 30 | weight per hour of contact | scaled | provisional | MVP-051 | The first AI attack about 1.5 hours after contact for a Bold neighbour |
| `mvp.grievance.claim_weight` | 30 | weight | — | provisional | MVP-051 | — |
| `mvp.grievance.attack_weight` | 40 | weight | — | provisional | MVP-051 | — |
| `mvp.grievance.unprovoked_factor` | 2 | times | — | provisional | MVP-051 | — |
| `mvp.grievance.fade_share_per_hour` | 10 | % of weight per hour | scaled | provisional | MVP-052 | — |
| `mvp.grievance.battle_relief_won` | 50 | % of the attacker's grievances | — | provisional | MVP-052 | Without relief an AI would attack again at every respite's end |
| `mvp.grievance.battle_relief_lost` | 25 | % of the attacker's grievances | — | provisional | MVP-052 | — |
| `mvp.claim.uncontested_minutes` | 30 | minutes | scaled | provisional | MVP-053 | — |
| `mvp.relation.tribute_ru_per_weight` | 10 | ru per weight | — | provisional | MVP-054 | Easing must be affordable but not cheaper than defending |
| `mvp.temperament.bold_threshold` | 40 | anger | — | provisional | MVP-057 | — |
| `mvp.temperament.territorial_threshold` | 70 | anger | — | provisional | MVP-057 | — |
| `mvp.temperament.wary_threshold` | 100 | anger | — | provisional | MVP-057 | — |
| `mvp.ai.attack_min_share` | 0.60 | of the target's seen Workers | — | provisional | MVP-055 | — |
| `mvp.ai.decision_ticks` | 50 | ticks | tick | provisional | MVP-058 | ADR-080 item 2's cadence |
| `mvp.run.length_hours` | 8 | hours of play | scaled | provisional | MVP-060 | 8 world days at the always-on pace; Playtest 1 judges the arc |
| `mvp.run.quiet_minutes` | 15 | minutes | scaled | provisional | MVP-060 | — |
| `mvp.renown.defense_held` | 3 | renown | — | provisional | MVP-061 | — |
| `mvp.renown.claim_won` | 3 | renown | — | provisional | MVP-061 | — |
| `mvp.renown.grievance_eased` | 1 | renown per 20 weight | — | provisional | MVP-061 | Watch for tribute bought only for renown |
| `mvp.renown.structure_per_100ru` | 1 | renown per 100 ru | — | provisional | MVP-061 | — |
| `mvp.stats.sample_minutes` | 5 | minutes | scaled | provisional | MVP-064 | — |
| `mvp.stats.population_step` | 10 | Workers | — | provisional | MVP-064 | A growth step at each new multiple (M2 plan §9, question 1); fixed before T10 and not one of its levers |
| `mvp.presence.idle_minutes` | 5 | minutes | tick | provisional | MVP-072 | — |
| `mvp.persist.snapshot_seconds` | 60 | seconds of world time | tick | provisional | MVP-075 | At most a minute lost on a crash |
| `mvp.persist.snapshots_kept` | 3 | snapshots | — | provisional | MVP-075 | — |
| `mvp.client.fps_target` | 60 | frames per second | tick | provisional | MVP-085 | — |
| `mvp.client.figures_max` | 1500 | figures on screen | — | provisional | MVP-085 | Four settlements at plateau plus a battle |
| `mvp.light.key.towards` | −0.04, 0.60, 0.80 | direction towards the light, normalised at load | — | provisional | MVP-081 | The key light of the one lighting preset (O75; DEC-562 stays Open): warm, from behind the development view; the owner tunes it after M1 task T13's run. Raised from 0.43 after the T17 run (ADR-115 amendment): the low key lit the terrain's facets at a grazing angle in stair-step blocks |
| `mvp.light.key.color` | 1.2, 1.0, 0.65 | RGB multiplier | — | provisional | MVP-081 | Softened from 2.0, 1.0, 0.4 after the T17 run, still warm (WLD-183): the strong key turned the ramp's pale shore rows orange and the valley olive |
| `mvp.light.fill.towards` | 0.60, 0.40, −0.60 | direction towards the light, normalised at load | — | provisional | MVP-081 | The fill light (O75), a neutral grey in the pattern of WLD-183, from the side opposite the key, where 0.07, 0.35, 0.93 doubled the key |
| `mvp.light.fill.color` | 0.25, 0.25, 0.25 | RGB multiplier | — | provisional | MVP-081 | Dimmed from 1.0, 1.0, 1.0, so the ground sits darker than the crowds |
| `mvp.cursor.world_size` | 48 | physical px | — | provisional | MVP-101 | The pointer `cursor.world` (O76, ADR-119), in place of R-CTRL-350's 35 px for the owner's 128 px art; the owner tunes it at M1 task T14's run |
| `mvp.cursor.highlight_size` | 48 | physical px | — | provisional | MVP-101 | The pointer over a selectable Leader, `cursor.highlight` (O76) |
| `mvp.cursor.unavailable_size` | 60 | physical px | — | provisional | MVP-101 | `cursor.unavailable` (O76), R-CTRL-350's size |
| `mvp.cursor.world_hotspot` | 0.113, 0.082 | fraction of the size (x, y) | — | provisional | MVP-101 | The tip of the owner's arrow, in place of `ref.ctrl.cursor.set.world_hotspot` (0.055, 0.070), which misses it by 7 texels (ADR-119) |
| `mvp.selection.disc_size_factor` | 0.8 | × the Worker card's half-size | — | provisional | MVP-101 | The disc on the ground under a selected Worker (O82, ADR-127) |
| `mvp.selection.disc_alpha` | 0.35 | alpha, 0 to 1 | — | provisional | MVP-101 | The disc's strength in the control glow's colour |
| `mvp.selection.circle_fade_seconds` | 0.4 | s of real time | — | provisional | MVP-101 | The released selection circle's fade |
| `mvp.structure.rise_depth_factor` | 1.0 | × the structure's height | — | provisional | MVP-083 | A construction site is sunk by this share of its height times the work still to come, so it rises out of the ground as it is built (O89, DEC-1001, ADR-140); the owner tunes the structures' looks after M2 task T12's run |
| `mvp.structure.inactive_brightness` | 0.35 | × the lit colour | — | provisional | MVP-083 | An inactive structure is dark (MVP-035) |
| `mvp.structure.flash_size` | 40 | wu, half-size | — | provisional | MVP-083 | The completion flash of `fx.starburst` in the team colour |
| `mvp.structure.flash_seconds` | 0.5 | s | — | provisional | MVP-083 | The flash fades over this time from its event |
| `mvp.wellspring.glow_blobs` | 5 | quads | — | provisional | MVP-083 | The Wellspring's cloudy glow, `fx.glow_cloudy` blobs circling its crystal in the team colour (O77) |
| `mvp.wellspring.glow_radius` | 12 | wu | — | provisional | MVP-083 | The circle the blobs ride |
| `mvp.wellspring.glow_size` | 22 | wu, half-size | — | provisional | MVP-083 | — |
| `mvp.wellspring.glow_alpha` | 0.45 | alpha, 0 to 1 | — | provisional | MVP-083 | — |
| `mvp.wellspring.glow_turn_seconds` | 24 | s of real time per turn | — | provisional | MVP-083 | — |
| `mvp.orb.cloudy_size` | 16 | wu, half-size | — | provisional | MVP-082 | The cloudy glow of `fx.glow_cloudy` about an orb drifting home or waiting at its Wellspring (MVP-023, MVP-025) |
| `mvp.orb.cloudy_alpha` | 0.5 | alpha, 0 to 1 | — | provisional | MVP-082 | — |
| `mvp.worker.load_color` | 0.6, 0.9, 1.0 | linear RGB | — | provisional | MVP-082 | The carried load's card `sprite.worker.carry_load` over the Worker's body (MVP-031) |

*Table 3 — The tuning values of the MVP.*

The first guesses were set against two targets, which M2 and Playtest 1 check. One is a plateau near 280 Workers on a settlement's own 11 units: about 44 gatherers bring about 2,640 ru per hour of play, which pays the upkeep of about 280 Workers and 14 structures. The other is a first contact in hour 2 to 3, with 4 to 10 battles involving the steward's settlement in a run.

> **Amendment 2026-10-21 (O84).** The economy model of the [M2 plan](m2_plan.md) §3 (`Tools/DesignModels/M2Economy.py`) finds the first target unmet as stated: with storehouses near the outer resource sites, 44 gatherers bring about 4,000 ru per hour, twice the upkeep, so the plateau is `mvp.settlement.population_max` and not where income and upkeep meet. The owner accepts a plateau at the cap for the MVP; an upkeep that brings the plateau below it is B2's.

## 5 What the MVP does not build, narrows or defers

### 5.1 Not built in the MVP

- Skirmish and its maps, bots, Supply crates and modes (O55).
- More than one region, crossings, world templates beyond one, set pieces, the strategic view as a separate view (the settlement view's zoom covers the valley, WLD-244), and a separate strategic zoom level (O9's strategic zoom waits for a world of more than one region).
- More than one steward, joining a world, founding after creation, dormancy (PIL-078), newcomer protection (PIL-088), stand-in AI settlements and the keeper (O38), and attacks between stewards (O33).
- The abstract model, fidelity switching and the overload lever (O3, O47; PIL-057 holds trivially at full fidelity).
- Special units, Supply crates, powers, creatures, hazards and world events (PIL-049).
- Truce and alliance; grievance causes beyond the three of MVP-051.
- The aide, standing orders beyond the default orders and the three priorities, and the check-in entry without the settlement view (PIL-141).
- Replays (PIL-140) and the restored-save record of PIL-133.
- The server API for a companion beyond the server messages of MVP-098 (O6), rate limits, and identity, port mapping, listing and relay (O45).
- The journaled record store (O42), the own deterministic maths functions and cross-build determinism (ADR-081 Level 2 and 3), interest tiers and bandwidth caps (ADR-086), and region processes (ADR-085).
- Research, training and asymmetric Cladans (O7, O11), achievements, localisation beyond British English, tutorials (the steward is the designer), gamepad input, music, and a dedicated server for another operating system (O41).

### 5.2 Final requirements the MVP narrows

| Requirement | MVP | Why |
|---|---|---|
| PIL-001, PIL-002, PIL-003 | Skirmish not built | O55 |
| PIL-031, PIL-030 | The first audience is one steward; PIL-030's solo case is the only case | O50 |
| PIL-025, WLD-102 | The warning hold is `mvp.conflict.warning_minutes` (5 minutes) instead of 10 | A present steward needs less; Playtest 2 checks the away case |
| PIL-057 | Every settlement at full fidelity | O3's abstract model deferred (MVP-096) |
| PIL-067, WLD-220, WLD-221 | A fixed run of 8 world days in an integrated world | O53 amends O39 for the MVP |
| PIL-077, PIL-100 | The always-on world runs in a background headless process | O54 amends O35 |
| PIL-101, WLD-200 | Session factor 24 instead of 12 | A run of 8 hours of play |
| PIL-125 | Renown and the epilogue in the MVP | O53 |
| PIL-068, WLD-004, WLD-120 to WLD-127 | One region, three AI settlements, no free site | One steward, no late joiner |
| PIL-110 (Table 10, stage 1) | The MVP is stage 1's smallest world game in one region, with renown and a lifetime added | O50, O53 |
| O42 | Versioned snapshots instead of the journaled store | A solo world loses at most a minute; the store follows when worlds are shared |

*Table 4 — The Final requirements that the MVP narrows.*

### 5.3 Engineering consequences to carry

- **Client boundary.** The renderer, camera, input, diagnostics, data-schema, terrain, VFX, UI, asset-pipeline and build components carry dated amendment notes where the reference's in-process snapshot, extrapolation or match-only structure would contradict ADR-084 (for example RND-002, RND-018, CAM-004, INP-004, TER-009, DBG-160): the client keeps a replica drawn one tick behind with interpolation, and the world frame stands beside the match frame. Three points stay open: the fog end beyond the far plane at world extent (RND-211, RND-222, DEC-247), the light-cycle slot of a world settlement (RND-167), and the reference random stream named in diagnostics records against ADR-082's keyed draws (DBG-117).
- **Transport.** ADR-089 records the MsQuic package (O44), pinned at 2.6.3, which the pin check re-confirms before M4; ADR-088 builds `QuicTransport` on it beside `InProcessTransport`, and the platform layer stays socket-free (PLT-100). The per-datagram overhead of 89 bytes on IPv6 is computed from the RFCs and is measured at M4.
- **Proposed ADRs confirmed at the first playable.** ADR-002, ADR-004, ADR-006, ADR-007, ADR-013, ADR-014, ADR-015, ADR-025, ADR-028, ADR-034, ADR-062, ADR-064, ADR-077 and ADR-078 name the first playable as their confirmation point; each carries a note that since O55 this is the MVP's M3 build, where it is confirmed or amended on its own "Confirmed when" evidence. ADR-028's gamepad tests wait for a build with gamepad input.
- **A2 is Final.** Gate B is applied to the solution strategy (Final) and to ADR-079 to ADR-088, which stay Proposed with their confirmations moved to the MVP's milestones: budgets and per-figure costs at M1 (O43), the first-playable evidence at M3, the transport at M4, and the multi-steward evidence (ADR-079's latency playtest of six players) after the MVP. Where M1's measurements change a budget (ADR-080), the ADR is amended with the measured figures.
- **Assets.** [GameAssets.md](../assets/GameAssets.md) has a priority `mvp` before `skirmish-first-playable` (O55), carried by every asset the MVP needs, existing rows and the new rows for the Wellspring, dwelling, storehouse and resource-site meshes, the Worker's carried-load card, the HUD icons and panels and three sound groups. The [MVP assets](mvp_assets.md) list them by category and milestone, with how each is produced and what the owner makes or approves by hand.

## 6 Playtests and acceptance

### 6.1 Playtest 1: an hour of play and a run (end of M3)

The steward plays two full runs at the session pace over two to three weeks, with the session log of §6.3. The server's statistics (MVP-064) supply the measured lines.

| Question | Measure | Pass line | Source |
|---|---|---|---|
| Does the settlement grow in steps a player notices? | Minutes of play between growth steps in the first 5 hours | At most 20 minutes | PIL-055 |
| Are battles crowd battles? | Figures in the steward's battles | Typical 300 to 450, never below 200 | PIL-021 |
| Are battles bounded? | Battle length | 90 % within 4 to 12 minutes, none above 20 | PIL-047 |
| Do attacks have readable causes? | For each attack on the steward, the steward writes the cause before opening the relations panel | At least 80 % right | PIL-024 |
| Do conflicts settle? | Conflicts between neighbours that reach an end state within 7 world hours | At least 60 % | PIL-123 at the session pace |
| Does anyone run away with the valley? | The strongest settlement's strength at the end against the median | At most 3 times | PIL-028 |
| Is there conflict without permanent war? | Battles involving the steward's settlement in a run | 4 to 10 | PIL-065 scaled to a run |
| Is an hour of play fun? | Session log | The steward wants to continue after most sessions | O50 |
| Is there something to want after the plateau? | Session log | The renown standing is still open when the plateau comes | O53 |
| Does the steward want another run? | Session log | Yes, after each of the two runs | O53 |

*Table 5 — Playtest 1.*

### 6.2 Playtest 2: coming back (end of M4)

The steward plays one always-on run of 8 world days (8 real days, with the computer left running), at the typical pattern of PIL-102: about an hour of play and two check-ins a day.

| Question | Measure | Pass line | Source |
|---|---|---|---|
| Is the away report quick to read? | Time to read its summary | At most 2 minutes | PIL-073 |
| Can the steward predict the deputy? | Five scripted situations (an attack declared on the settlement; a claim battle at a border area; stores below the next queued cost; population at the limit with a dwelling queued; a respite ending while a neighbour's anger is above its threshold): the steward writes the expected deputy action, then compares it with the report | At least 4 of 5 right; every deputy action in the report with its order | PIL-026 |
| Is absence safe? | After a night or a day away with attacks, world days back to the strength before the absence | At most 2 world days | PIL-022 |
| Does presence pay, and absence not? | The steward's growth per world day against the AI settlements' at the same stage | Present days grow more than check-in days | PIL-023 |
| Is coming back fun? | Session log | The steward looks forward to the check-ins | O54 |

*Table 6 — Playtest 2.*

### 6.3 Session log

After every session of a playtest the steward writes three lines in the playtest log: what was fun, what was dull or frustrating, and what was confusing, with the world time and the build's identity (ADR-057). The log lives in `playtests/` at the repository root, outside `Design/`, one Markdown file per playtest (`Playtest1.md`, `Playtest2.md`), with the milestone records beside it; M0 creates the folder with a README that states this format. The log, the server's statistics and the epilogues are the evidence that the paused design areas resume from (O51), starting with B1, B2, B5 and B6.

## 7 Open items

| Item | Decided by |
|---|---|
| Whether the AI settlements need a difficulty setting beyond their temperaments (R-BOT-022 to R-BOT-024) | Playtest 1 |
| Whether the MVP needs the strategic view's second layer to follow battles between AI settlements | Playtest 1 |

*Table 7 — Open items of the MVP scope. Decided on 2026-10-19: the names of the resource and the structures (O57), the reference test machine (O58), and where the playtest log lives (§6.3).*

## 8 Dependencies

- **Owner decisions:** O1 to O55; this document applies O50 to O55.
- **Game design:** [vision, pillars and core loops](../gdd/00_vision_pillars_and_core_loops.md) (Final), [world and regions](../gdd/01_world_and_regions/README.md) (Final).
- **Architecture:** [solution strategy](../architecture/04_solution_strategy.md) and ADR-079 to ADR-088 (Draft and Proposed); the baseline ADRs of the [ADR index](../ADR/README.md).
- **Reference:** units, Leaders and Essence orbs (reference/02), turrets and the Spawn Node's ports (reference/03), the laser bolt and grenade (reference/04), the population flow's orb rules (reference/05), the player camera (reference/06), the bot (reference/07) and the simulation model (reference/12), at the ids cited above.
- **Engine components:** platform layer, renderer, terrain, animation and VFX, input, camera, UI framework, data schemas, asset pipeline, build tooling, diagnostics, and from M3 the audio engine ([component index](../architecture/components/README.md)).
- **Assets:** the `mvp` rows of the manifest, listed in the [MVP assets](mvp_assets.md).
