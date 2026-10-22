---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T5)
---

# The gun turret at M2

## Context and Problem Statement

The MVP's turret is the reference gun turret (R-BLDG-405 to R-BLDG-423), whose ports Workers on guard duty man. It costs `mvp.cost.turret_ru`, it is the only MVP structure that takes damage, and a destroyed turret leaves a construction site that its settlement may rebuild at half its cost (MVP-034). MVP-033 makes its construction ports its manning ports. The M2 plan's task T5 is done when the turret fires on the sandbox's side headless with a stable hash.

The third part of T5 builds the turret on ADR-131's structures and ports and ADR-132's construction. The requirements leave open:
- which team a turret fires for, when the settlement owns it and the reference turret belongs to whoever mans it;
- what its hit volume is, while its head turns;
- where its barrels aim from, since R-BLDG-416 blends their up vector towards "the aim direction" without saying from where;
- what becomes of the reference's draws and parts that the gun turret of the MVP does not use;
- what a destroyed turret leaves, and how it is rebuilt.

## Decision Drivers

* The reference turret is kept as its rules state it, except where a rule needs a choice it does not make (MVP-034).
* A run reproduces exactly: integer health and timers, keyed draws (ADR-097), no wall clock (MVP-093).
* Every number is a key (MVP-099).
* The hit volume reads the collision assets, never the drawn geometry (R-SIM-123, ADR-131).

## Considered Options

1. **The turret is a structure kind with its own state, firing for the team that mans it**, as the reference's capture model does; its rounds are effects of the effect pool.
2. **The turret always fires for its settlement's team**, and manning only enables its barrels.
3. **The turret's rounds are laser bolts** of the laser-bolt pool, with the round's speed, life and damage.

## Decision Outcome

Chosen option: 1. Option 2 drops R-BLDG-408 and the reference's capture, which MVP-034 adopts with the rest of the turret. Option 3 would give the round a bolt's single hit and ricochet, where R-CRATE-451 has it pierce and end on the terrain.

* **The kind and its records.**
  - `StructureKind::Turret` (5) builds its ports from the turret mount's record. Its collision asset is its head's, as the mount has no triangles (AST-045), and its barrels sit at the head's barrel mount.
  - Its centre is its position and its radius `ref.bldg.turret.radius` (30 wu), as R-BLDG-407 sets.
  - The server starts only when the mount has `mvp.construction.ports_turret` port sockets, the head a barrel mount, and the barrels a muzzle for each port (R-BLDG-405), as it checks the other records (ADR-130, ADR-132).
* **Its state** (`TurretState`, part of the structure in state version 9): the team that mans it, its health, its ownership, search and fire countdowns, the fire steps it still hoses, its barrel cursor, its target, its aim point, its head's front, its barrels' up vector, and whether it has drawn its first idle aim point. A turret starts at full health, with its head and barrels along its front, no team and its countdowns at 0.
* **Its advance.** A built turret runs R-BLDG-410's steps 2 to 6 before the base advance evaluates its ports, which R-BLDG-410 puts last. Its ownership evaluation and its fire step therefore read the occupants of the tick before.
  - Its initialisation, which draws its first idle aim point (R-BLDG-412), is its first advance as a built turret.
  - The ownership evaluation follows R-BLDG-408. The MVP's teams are friends only of themselves, so operators of two teams leave it with no team. A change of team clears its target and its hosing.
  - **The firing team is the team that mans it; the settlement stays its owner.** The settlement pays its upkeep and counts it, and an enemy that mans the steward's turret turns it, as the reference's capture does. M3's battles may revisit this.
  - The target search, the idle sweep, the wobble over match time, the fire cone and hosing follow R-BLDG-411 to R-BLDG-415 with the gun turret's ranges.
    - The line of sight samples every `ref.bldg.turret.los_step` against the obstacle grid's shapes, the turret's own excepted.
    - The MVP has no bot controller or Collector to skip. `EnemiesInCells`' early answer changes no result, so the search does not make it.
* **Where the barrels aim from.** The head turns by the yaw gain (R-BLDG-416). The barrels' up vector blends towards the direction from the head's barrel mount to the aim point, and is kept of unit length. The barrels' front is the head's front raised to that vector's elevation, and each round leaves its muzzle along it.
  - From the turret's base, the rounds pass about 10 wu above a Worker's 5 wu hit sphere.
  - In the sandbox scene of the confirmation, aiming from the base gave 364 rounds and no kill in 200 ticks; aiming from the mount kills 176 of the AI's 200 Workers.
* **The fire step** follows R-BLDG-418 to R-BLDG-420: barrel i belongs to port i, and a round starts half a tick of travel behind its muzzle.
  - Step 1's three draws serve only the mortar turret. Under keyed draws an unmade draw changes no other (ADR-097), so the gun turret does not make them.
  - The casing is presentation, carried by the event `TurretFired`.
* **The turret round** is an effect of kind `TurretRound` (4) whose fuse is its life. It records the turret that fired it (`source`, state version 9).
  - Each advance follows R-CRATE-451: it pierces every enemy its step's ray hits. A Worker is a sphere of `ref.sim.ray.sprite_radius` (R-SIM-387), and a Leader a sphere of its radius when that is larger.
  - A Worker that was already burning counts no kill. A Leader takes no knock-back, as from an explosion.
  - The round ends on a structure or on the terrain, with `TurretRoundHit`.
  - Its structure test passes its own turret: its start lies inside its own turret's ray sphere.
  - A round is not replicated. The client draws it from `TurretFired`, as it draws laser bolts (ADR-123).
* **The hit volume** in the obstacle grid (ADR-131):
  - the turret's 30 wu sphere about its position, which the pushes measure from;
  - its head's triangles and its barrels' at rest, along the turret's front;
  - its ray sphere of `ref.bldg.turret.ray_sphere_radius` (12 wu), `ref.bldg.turret.ray_sphere_height` (10 wu) above its position, which its ray test hits besides the triangles (R-BLDG-407).

  The head's turn is not followed. Its collision is an eight-sided frustum about the vertical, and only the four slim barrels would move.
* **Damage.** The explosion's step 2 is built (R-BLDG-035, R-SIM-388): every structure within `ref.sim.explosion.structure_reach` × r takes −D × (1 − d ÷ 3r), with no team filter, truncated to an integer as the actors' share is. Only a built turret applies it; a construction site takes none.
* **Destruction and the rebuild.** At health 0 or below a turret raises `TurretDestroyed` (R-BLDG-423). It becomes a construction site of its settlement that is not queued, costing `mvp.cost.turret_rebuild_fraction` (0.5) of `mvp.cost.turret_ru`.
  - `RebuildTurret` pays that cost and queues the site, with ADR-132's refusals.
  - A site gains work only while it is queued (amending ADR-132). Guard duty at a ruin's ports therefore does not rebuild it unpaid.
  - The site stands as an obstacle, and its ports stay mannable. Its completion makes a fresh turret, which has no team until its next evaluation.
* **Placement.** The steward places a turret as a dwelling or a storehouse is placed (ADR-132), for `mvp.cost.turret_ru`.
* **Events and draws.** The presentation events gain `TurretTargetSighted` (20), `TurretFired` (21), `TurretRoundHit` (22) and `TurretDestroyed` (23); a round's kill raises `LaserKill`, whose sound it shares (R-CRATE-451). The draw purposes gain `TurretSweep` (35) and `RoundPush` (36).
* **Not built at M2:**
  - player control (R-BLDG-425), which the MVP's verbs do not include;
  - the bot navigation node (R-BLDG-426), as the MVP's AI contests no turret;
  - the Visitor and turret-cap rules (R-BLDG-409, R-BLDG-424), which the MVP has no use for;
  - the other types' parts, the flame guard and the overheat (R-BLDG-417, R-BLDG-430);
  - the kill count, tint and status lights (R-BLDG-427, R-BLDG-428), which are presentation for M2's phase B.
* **Keys.** `ref.bldg.turret.*` (the reference's values), `ref.weapon.turret_round.life_ticks`, `damage`, `push` and `push_jitter`, `ref.sim.explosion.structure_reach`, `ref.sim.ray.sprite_radius` and `mvp.cost.turret_rebuild_fraction`.

### Consequences

* Good, because the turret is the reference's: its cadence of 5 rounds a second for each manned port, its line of sight, its capture and its piercing rounds.
* Good, because a destroyed turret's site keeps its identity, its ports and its footprint, and its rebuild is a payment and a queue entry.
* Bad, because an enemy that reaches a steward's turret and mans it turns it on the steward's settlement while the settlement still pays its upkeep.
* Bad, because the barrels are tested at rest, so a round or a bolt can pass where a turned barrel stands, or meet one where it no longer is.
* Bad, because the aim from the barrel mount is a choice the reference does not state; a round from a raised muzzle at a near target still passes above it.

### Confirmation

GameLogicTests (`TurretTests`):
- ownership by the team that mans it from the second evaluation, and no team with an enemy operator;
- two rounds a tick in barrel order with four ports manned, and 5 a second for each manned port;
- a target hidden behind a storehouse passed over for one in the open;
- a round that pierces three enemies and spares a friend, and ends after its six moves;
- explosion damage of 26 at 10 wu of a range-10, damage-40 explosion, destruction into an unqueued site of half the cost, no work while unqueued, and a paid rebuild to full health;
- the turret firing on the sandbox's AI side with the same hash on every tick at 0, 1 and 8 worker threads.

## Pros and Cons of the Options

### 1 — its own state, firing for its manning team

* Good, because R-BLDG-408 and the capture model hold as written.
* Bad, because the firing team and the settlement can differ.

### 2 — always its settlement's team

* Good, because a settlement's turret never fires on it.
* Bad, because it drops a reference rule that MVP-034 adopts.

### 3 — rounds as laser bolts

* Good, because no new effect kind is needed.
* Bad, because a bolt hits once and ricochets, where a round pierces and ends on the terrain.
