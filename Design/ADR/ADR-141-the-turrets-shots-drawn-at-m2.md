---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T12)
---

# The turret's shots drawn at M2

## Context and Problem Statement

M2 task T12's second step draws the gun turret's fire, which [ADR-133](ADR-133-the-gun-turret-at-m2.md) simulates and [ADR-139](ADR-139-replicating-the-settlement-at-m2.md) replicates:
- its rounds, a lit mesh with a white tracer (R-CRATE-450, R-CRATE-453);
- each shot's muzzle flash (R-CRATE-405) and the casing its barrel ejects (R-BLDG-418 step 3, R-CRATE-455, R-CRATE-456);
- the sparks and dust of a round's hit (R-CRATE-451 steps 4 and 5), and the spark of a laser bolt meeting a structure (R-CRATE-441 step 6, VFX-219).

The step adds no simulation and no message. These decisions are open:
- where a round's direction comes from, since its replicated state holds a position, a yaw and a timer;
- how a casing, which is presentation alone (DEC-461), is placed and moved on a client that holds no turret's barrel transform;
- what M2 draws of the shatter that R-CRATE-451 and R-BLDG-423 name, which [ADR-010](ADR-010-gpu-particles-and-shatter-debris.md)'s shards provide and M1 did not build.

## Decision Drivers

* The client draws what the server sends and its events say, and never computes a rule of the simulation (MVP-090, DEC-007).
* The effects' existing paths serve: [ADR-123](ADR-123-weapons-effects-and-particles-at-m1.md)'s events played at their tick, its CPU particle pool, the mesh batches of pass 10a and the segments of pass 10b.
* Every value is a key of the presentation record, from the reference's tables (VFX-207, VFX-219, R-CRATE-405, R-CRATE-453, R-CRATE-456).

## Considered Options

For a casing's place:
1. **The barrel set's sockets** (`muzzle_0i`, `casing_eject_0i`, ADR-128), placed in the barrels' axes rebuilt from the shot's velocity.
2. **The turret's interpolated aim** from the replica, with the head's barrel mount, as ADR-140 draws the barrels.
3. **The muzzle alone**, with no casing socket.

For the shatter:
4. **None until ADR-010's shards**, as at M1 (ADR-010's amendment of O77).
5. **A burst of explosion debris** standing for the shatter.

## Decision Outcome

Chosen options: 1 and 4.
- Option 2 places the casing at the aim interpolated for the frame, not the aim of the tick that fired, and needs the structure's state in the effects.
- Option 3 ejects every casing from the muzzle, 14 wu ahead of the ejectors.
- Option 5 draws a look the reference does not have, which the shards would replace.

* **The rounds** (R-CRATE-453), drawn from the replica's effects of kind `TurretRound`:
  - **Direction.** The `TurretFired` event's subject is the round's id and its vector the round's velocity (ADR-133). The look keeps each shot's velocity for the round's life and a tick either side. A round whose event was lost flies along its latest tick's motion, or else its yaw.
  - **Mesh.** `mesh.projectile.turret_round` along that velocity, lit, in pass 10a (`effect_mesh_lit`), as the grenade is.
  - **Tracer.** A white segment behind the round of its speed × `ref.weapon.turret_round.tracer_factor`, 25 wu at 500 wu/s, at alpha `tracer_alpha`.
    - It is one layer of pass 10b's laser segment (`laser_bolt`, `fx.beam_short`, RND-338) at `rnd.laser.half_width`, since the reference gives the line no texture or width of its own.
    - The doubled tracer of a turret the local player aims is not built: the MVP has no turret aiming (R-BLDG-425).
* **The muzzle flash** (R-CRATE-405): each `TurretFired` adds a flash of `ref.weapon.muzzle_flash.round_size` (60) and `round_life` (2) of no team, white, at the muzzle, which is where the round's flight passes half a tick after its start. It is drawn by M1's flash, with the depth test at its size (VFX-222).
* **The casings** (R-CRATE-455, R-CRATE-456), presentation alone, made as the shot's event plays:
  - **Barrel axes.** Front along the round's velocity, right level, and up = front × right, which are ADR-140's barrel axes at the tick of the shot.
  - **Start.** The casing starts at the muzzle plus the offset from barrel i's `muzzle_0i` socket to its `casing_eject_0i` socket. It is moved `ref.weapon.shell_casing.socket_offset` along the barrels' up: downwards for barrels 0 and 1, upwards for 2 and 3.
  - **Eject direction.** ADR-128's sockets carry no rotation, so the eject direction is the barrel's outer side, where its ejector stands, and the casing's front is its negation.
  - **Launch.** Its velocity is Normalize(eject + (0, 1, 0) + `FxRng::NextSigned(0.5)` per axis) × (`speed_min` + `FxRng::NextUnit(speed_spread)`). These are four draws of the event's generator (VFX-011), as R-BLDG-418 counts.
  - **Spin.** It turns about front + (frac(0.1 n), frac(0.3 n), frac(0.7 n)) at its force in radians a second, with n the look's casing serial.
  - **Motion.** It falls by `ref.sim.physics.thrown_gravity` on the scaled clock, in steps of `vfx.particle.sub_step`.
  - **Bounce and rest.** At the terrain its vertical velocity turns over, its force is multiplied by `bounce_factor` and its velocity by its force. It goes once its force is at or below `rest_force`, at its third bounce, or when it leaves the region.
  - **Draw.** It is `mesh.projectile.shell_casing`, lit, in pass 10a.
  - **Meshes.** The effects look takes the scene meshes when the client loads them (`UseMeshes`); without them a shot ejects no casing.
* **The hits** (VFX-219):
  - `TurretRoundHit` on a structure shows `vfx.emit.round_hit.structure_count` (3) sparks; on the terrain, `terrain_count` (1) missile trail stands for the dust (DEC-962).
  - `LaserHitStructure` shows `vfx.emit.laser_hit.count` (1) spark. The bolt it names ends where its flight meets the event's position, as a ricochet's second flight begins at its contact (ADR-123).
  - **The spark** is VFX-207's type (`vfx.particle.spark.*`) in the pool's glow batch, class 1. Its terrain rule, a bounce, is not built: M1's pool has no terrain, so a spark falls through the ground, where the depth test hides it.
* **The shatter is not drawn at M2** (option 4): neither the round's shatter of R-CRATE-451 nor the head and barrels' of R-BLDG-423. `TurretDestroyed` draws nothing more; the explosion that destroyed the turret shows, and the turret becomes a sunken construction site (ADR-140). `TurretTargetSighted` is a sound, for M3's audio.

### Consequences

* Good, because every shot shows where it leaves, where it flies and where it ends, with no new pass, entry or message.
* Good, because a casing leaves its own barrel's ejector at the tick of its shot, from the barrel set the mesh kit drew.
* Bad, because a casing's eject direction is fixed by the barrel's side rather than authored in its socket. A socket with a rotation would need its axes read.
* Bad, because sparks sink into the ground rather than bounce, and a destroyed turret does not shatter, until the particle pool gains its terrain rule and its shards.
* It forecloses: a casing placed from the replicated aim; a debris burst standing for the shatter.

### Confirmation

* `GameClientTests`:
  - a turret round is its lit mesh along its shot's velocity, with one white tracer segment of the tracer factor × its speed behind it at the tracer's alpha, and its shot's flash; a round whose event was not seen flies along its motion;
  - a casing is ejected only with the barrel set's sockets, starts at barrel 1's casing socket below its up axis, faces away from the barrel's outer side, flies outwards, and rests and goes after its bounces;
  - a round on a structure shows three sparks, one on the ground a puff of dust, and a bolt on a structure one spark, and that bolt ends at the structure;
  - the spark is VFX-207's type.
* The owner's run: the turret fires at enemies only, and M2 has none until M3's battles, so its shots are not seen on the owner's runs of phase B. The tests above stand for them until M3.

## Pros and Cons of the Options

### 1 — the barrel set's sockets

* Good, because the casing leaves the mesh's own ejector (DEC-461).
* Bad, because the eject direction comes from the barrel's side.

### 2 — the interpolated aim

* Good, because the casing would leave the barrels as drawn.
* Bad, because the frame's aim is not the shot's.

### 3 — the muzzle alone

* Good, because it needs no socket.
* Bad, because the casing leaves the wrong end of the barrel.

### 4 — no shatter until the shards

* Good, because nothing is drawn that ADR-010's shards will replace.
* Bad, because a destroyed turret does not break apart at M2.

### 5 — a debris burst

* Good, because a turret's destruction would show on its own.
* Bad, because it is a look of no reference rule.

## More Information

* Related: [ADR-010](ADR-010-gpu-particles-and-shatter-debris.md), [ADR-123](ADR-123-weapons-effects-and-particles-at-m1.md), [ADR-128](ADR-128-mesh-kit-and-drafted-structure-recipes.md), [ADR-133](ADR-133-the-gun-turret-at-m2.md), [ADR-139](ADR-139-replicating-the-settlement-at-m2.md), [ADR-140](ADR-140-structures-drawn-and-the-stand-ins-at-m2.md).
* The rules: R-BLDG-418, R-BLDG-423, R-BLDG-425; R-CRATE-405, R-CRATE-441, R-CRATE-450, R-CRATE-451, R-CRATE-453, R-CRATE-455, R-CRATE-456; RND-331, RND-338; VFX-011, VFX-207, VFX-219, VFX-222; DEC-461, DEC-962.
* The code: GameClient (`EffectsLook`, `WorldPresentation`), and the presentation record's new keys.
* ADR-010 and ADR-140 are amended with this record.
