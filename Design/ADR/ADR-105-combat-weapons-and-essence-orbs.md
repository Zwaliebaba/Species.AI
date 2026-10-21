---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T6)
---

# Combat, weapons and Essence orbs

## Context and Problem Statement

Task T6 of the [M1 plan](../implementation/m1_plan.md) makes the crowds fight (MVP-040). It covers:
- the Worker's threat search, its fighting, its base attack and its grenades (R-UNIT-098 to R-UNIT-104);
- burning and death (R-UNIT-079, R-UNIT-134, R-UNIT-034 to R-UNIT-039);
- the laser bolt, the grenade, the explosion and its shockwave ring (R-CRATE-405 to R-CRATE-473);
- the Essence orbs that deaths release, which at M1 lie outside every territory and ascend (MVP-023, R-UNIT-301 to R-UNIT-329);
- the effect events the client will draw (ADR-036, ADR-084 item 5).

The reference keeps the weapons and the effects in pools of its match world, draws from one generator, and lets its renderer read live objects. These decisions are open:
- where the new objects live, and in which order they step;
- what is an effect object and what is a presentation event;
- what subject keys the draws of objects that are not figures;
- how a Worker holds a threat that may be an actor or a grenade;
- the readings that the reference's prose leaves to the implementation;
- the tuning keys that cannot generate an identifier as the reference spells them.

## Decision Drivers

* The crowd rules are tuned gameplay and stay as the reference states them (MVP-040), including the quirks that DEC-094 and DEC-111 keep.
* A region reads no other owner's state (ADR-085 item 2), and a restored world gives the hashes of a world never stopped (ADR-099).
* Presentation never writes simulation state, and no gameplay information travels only in an effect (ADR-036, DEC-222).
* Every draw is keyed by a purpose and a subject (ADR-082, ADR-097), and every number is a tuning value (MVP-099).

## Considered Options

* 1 — a laser-bolt pool, an effect pool of grenades and rings, and an Essence-orb pool in each region, stepped in the reference's slice order; the presentation of the weapons as effect events beside the state
* 2 — the reference's effect pool whole, its muzzle flashes and particles included, as simulation state
* 3 — the weapons as fields of the figures that fire them, with no pools

## Decision Outcome

Chosen option: "1".
- Option 2 puts presentation in the state, and so in every hash and every snapshot, against ADR-036.
- Option 3 cannot express an object that outlives its shooter, and loses the slice order of R-SIM-218.

* **The pools.** Each region holds a laser-bolt pool, an effect pool and an Essence-orb pool, sliced like the teams' figures (R-SIM-204 to R-SIM-216).
  - Growth steps: `ref.sim.container.laser_bolt_step`, `effect_step` and `orb_step`. The orb pool starts with `orb_presize` slots.
  - In each slice the region steps its teams, then the slice's laser bolts, then its effects, then its orbs (R-SIM-218 steps 2, 3 and 5). The structures of step 4 arrive with M2.
  - An object created during a tick advances again in that tick exactly when its slot lies after the last slot already advanced (DEC-455).
  - Each object takes a stable id from the region's counter (ADR-083 item 5). That id is the subject of its draws: a bolt's damage and push (`LaserHit`), a ring's ignition rolls (`RingIgnite`), an orb's start and per-tick draws (`OrbStart`, `OrbStep`).
* **Effect events.** The effect pool holds only what a rule reads: grenades and shockwave rings.
  - Everything else is an effect event (`EffectEvent`, GameShared): muzzle flashes, explosion particles, the flashes and the shake, the barks and the sounds. A region lists the events of each step in the order its rules emitted them.
  - The kinds: a bolt fired, a ricochet, a laser kill, a grenade thrown, a bounce, an explosion, a figure hurt, a figure dead, a Worker set on fire, and a threat bark.
  - An event is never state: neither the hash nor the snapshot holds one, and the stable identity of ADR-036 is the tick, the region and the event's order.
  - Because the reference's muzzle flashes are not pool objects here, a grenade or a ring can take another slot than the reference's. Only the same-tick advance of DEC-455 reads a slot, and its outcome stays within the reference's stated alternatives.
* **Threats.** A Worker's threat is a kind (none, an actor, a grenade), a team, a slot and an id.
  - It resolves only while the object still holds that slot with that id, so a slot reused by another object is never mistaken for the threat.
  - Its threat range, its scared flag, its threat-sound latch and its grenade timer are figure state.
* **Damage and death.** One region operation applies a health change to the figure an entry names (R-UNIT-026).
  - It emits the hurt and death events and, on a kill, releases the one Essence orb of R-UNIT-038. The orb starts at the corpse, raised by `ref.sim.spawn.orb_lift`, with half the corpse's velocity.
  - Kill credit is the caller's: the laser bolt (R-CRATE-443), the explosion (R-CRATE-406 step 4) and the ring (R-CRATE-471 step 3) each count the enemy Workers they killed into their team's kill counter, which is team state (R-UNIT-032).
* **Readings of the reference's prose**, kept as rules here:
  - The threat search's grenade threats are measured in 3-D. The nearest wins, and the first in slot order on a tie.
  - The combat step turns the 3-D direction to or from the threat about the vertical by its draw, and scales it. The waypoint is then pushed and snapped to the ground, so the stand-off of R-UNIT-102 lies along the slope to a higher target, as AC-UNIT-W08 measures.
  - A Worker throws grenades only at an actor threat, never at a grenade it flees.
  - The burning step runs while the Worker is on the ground: its wander, its walk at full speed to that waypoint, and its damage roll, an unresistable change. A Burning Worker never reaches the state behaviour of R-UNIT-065 step 3, so the step itself moves it.
  - A bolt meets the ground when its new height is at or below the terrain's. The terrain ray cast from its previous position along its step finds the contact (TER-016, TER-017), its velocity reflects about the terrain normal there, and it flies the rest of its step's length from the contact. Its hit test then runs on that last straight leg.
  - Reflections use the terrain normal as TER-013 gives it, not renormalised.
  - A ring's candidates lie strictly within its radius horizontally, so its band lies just inside the ring (R-CRATE-471, AC-W-10).
* **Not built in the MVP:**
  - the draws that start nothing in a match: the Hive and Spitter Plant check of R-UNIT-098 step 5 and the victory celebration of R-UNIT-088. With keyed draws a draw that no rule reads changes nothing;
  - structures, which arrive with M2: a bolt never stops on one, and an explosion damages none;
  - the Shield, Fury, the Visitor faction, Convert, Sickness, insanity, rockets, Transports and the heavy kinds;
  - the bot census shortcut of R-UNIT-098 step 6, which arrives with the bot's node census (T8);
  - formations, their volleys and FollowingLeader, which arrive with T7.
  - Every Worker is a soldier with grenades (R-UNIT-062). The fire roll's game speed s is 1, because the world has none.
* **Values in data.**
  - The record holds the `ref.*` keys these rules read. A formula key is an object of its parts: the grenade timers' base and spread, the orb's floating time's centre and spread, and the burn roll's threshold and range.
  - `ref.weapon.throw.*` cannot generate a field, because `throw` is a C++ keyword (DAT-054). Its three keys are renamed `ref.weapon.throwing.*` and the old ones retired (CONVENTIONS table 4.1-A).
  - The schema generator now breaks a `ReadFields` signature or a field assignment that passes column 140 as clang-format lays it out (ADR-098 amendment).
* **State.** `WORLD_STATE_VERSION` becomes 6: the Worker's combat fields, each team's kills, and the three pools after the routes. A region of an older version reads with no bolts, effects or orbs, and its Workers with no threat. The golden run of DBG-147 changes by the empty pools, and an independent model of the layout computes the same values.

### Consequences

* Good, because the combat rules, their draws and their order are the reference's, and every object they create survives a restore.
* Good, because the client gets every effect it draws with a stable identity, and the state carries no presentation.
* Bad, because the effect pool's slots differ from the reference's wherever a muzzle flash would have taken one.
* Bad, because two crowds left to the threat search alone do not fight to an end. Level soldiers hover about 100 wu from their threat (R-UNIT-102), beyond the laser's reach of about 90 wu (DEC-111). The threat range shrinks to the last threat's distance (R-UNIT-098 step 9) and relaxes only slowly. So the survivors of a clash lose their threats, wander and stand off.
  - From a mixed start of 200 a side at the centre of seed 1's valley, 3,000 ticks leave about 110 and 75.
  - After 100,000 ticks, 56 and 13 remain, still trading a shot every few hundred ticks.
  - Battles that end need orders, Leaders and the AI (T7, T8), and whether level fights should resolve is a question for the owner's playtests (DEC-111).
* It forecloses: presentation in the effect pool; weapons owned by their shooters; a draw context shared between a bolt and its shooter.

### Confirmation

* `NeuronCoreTests` checks:
  - the ray–triangle test of PLT-138: a hit, a miss, a parallel ray, a hit behind the start, and the range bound;
  - the terrain ray cast: straight down, slanting across cells, entering from outside the grid, parallel above it, and up from above it.
* `GameLogicTests` checks:
  - the threat search's nearest enemy and its grenade threat (AC-UNIT-W05);
  - the combat distances of AC-UNIT-W08 over 40 ticks' draws;
  - the fire rate of AC-UNIT-W09 over 10,000 ticks;
  - the crowd rule of AC-UNIT-W11;
  - burning to death in about 125 ticks with one orb each (AC-UNIT-W15), and drowning on the next tick with one orb (AC-UNIT-W16);
  - the explosion vector of AC-W-01 with its kills and orbs;
  - the nine bounces of AC-W-03, the fuse of AC-W-04 counted in advances, and the bolt's reach of AC-W-06 with its damage and push;
  - the ricochet;
  - the ring of AC-W-10;
  - an orb's 61, 301 to 500 and 901 ticks;
  - a region with every new object reading back to the same state and stepping on alike;
  - the two crowds' clash, the same at every 50th tick on a second run, and the golden run.
* The stand-off was measured with a probe program over `RegionSimulation`, built with g++ 13 at -O1 in a 4-core 2.1 GHz Xeon container. 100,000 ticks of the battle took 3.0 s there.
* T10 measures the costs of the pools and of the threat search on the reference test machine.

## Pros and Cons of the Options

### 1 — region pools, effect events beside them

* Good, because state and presentation stay apart, as ADR-036 and DEC-222 require.
* Bad, because every presented effect needs an event kind.

### 2 — the reference's effect pool whole

* Good, because its slots are the reference's.
* Bad, because presentation enters the hash and the snapshot.

### 3 — weapons in the figures

* Good, because there are no pools.
* Bad, because a grenade dies with its thrower.

## More Information

* Related: [ADR-036](ADR-036-effect-events-and-presentation-clock.md), [ADR-081](ADR-081-numeric-model-and-determinism-scope.md), [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-097](ADR-097-keyed-draws-and-state-hash.md), [ADR-098](ADR-098-tuning-in-data-and-the-schema-generator.md), [ADR-099](ADR-099-world-snapshots.md), [ADR-103](ADR-103-region-grids-draw-contexts-and-figure-movement.md), [ADR-104](ADR-104-navigation-nodes-land-route-graph-and-routes.md).
* The rules: R-UNIT-034 to R-UNIT-039 and R-UNIT-052 ([units](../reference/02_units/README.md)), R-UNIT-079 to R-UNIT-104 and R-UNIT-134 ([Worker](../reference/02_units/worker.md)), R-UNIT-301 to R-UNIT-329 ([Essence orbs](../reference/02_units/essence_orbs.md)), R-CRATE-405 to R-CRATE-473 ([weapons and projectiles](../reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md)), R-SIM-374, R-SIM-385 and R-SIM-388 ([space, routing and physics](../reference/12_simulation_model/05_space_routing_and_physics.md)), TER-016 to TER-018 ([terrain](../architecture/components/terrain/README.md)), PLT-138 ([platform layer](../architecture/components/platform_layer.md)).
* The code is in NeuronCore (`Geometry`, `TerrainField::CastRay`), GameShared (`EffectEvent`) and GameLogic (`Combat`, `Weapons`, `EssenceOrb`, `FigureAdvance`, `RegionSimulation`, `StateValues`).
* **Amendment 2026-10-20 (M1 task T7, [ADR-106](ADR-106-leaders-formations-and-battle-commands.md)).** Formations, their volleys and FollowingLeader are built: a Worker returns to the Leader that still leads it when its fight ends (R-UNIT-101), a formation member holds its slot, searches ahead along its Leader's front and fires by the volley rhythm, and a Worker's formation membership is read from its Leader rather than held as a figure field.
* **Amendment 2026-10-20 (M1 task T8, [ADR-107](ADR-107-simple-ai-bot-navigation-and-node-census.md)).** The bot census shortcut of R-UNIT-098 step 6 is built: outside a formation, a Worker within the range of a navigation node whose last recount found no enemy of its team finds no threat.
* **Amendment 2026-10-21 (M2 task T5, [ADR-131](ADR-131-structure-step-collision-shapes-and-ports-at-m2.md)).** A bolt whose step meets a structure of its cell ends there with the presentation event `LaserHitStructure` and deals no damage (R-CRATE-441 step 6); an orb is pushed out of a structure by R-UNIT-321's push, which now finds the structures in the grid. Explosions damage the turret from its own task step (MVP-034).
