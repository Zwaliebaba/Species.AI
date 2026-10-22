---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M1 task T16), owner (O75, O77)
---

# The weapons' effects, the screen effects and the particles at M1

## Context and Problem Statement

M1 task T16's third step, T16c, draws what a battle throws about (M1 plan table 2 row T16; MVP-082; ADR-121, ADR-122):
- **Laser bolts and muzzle flashes** (R-CRATE-405, R-CRATE-445, R-CRATE-475, RND-338 to RND-340, VFX-222).
- **Grenades** with their tumble, fuse blink and trail (R-CRATE-410, R-CRATE-414).
- **Explosions and shockwave rings**, with the full-screen flash and the camera shake (R-CRATE-406, R-CRATE-473, RND-333, RND-334, VFX-232 to VFX-235).
- **The particles** of the M1 effects (VFX-205 to VFX-219), the order markers' flare (VFX-143) and the firing recoil of O75.

O77 chose the flash and the shake at T16, with DEC-964's limit on flash onsets.

Six things in the design do not hold as written at M1:
1. **No bolt state.** Laser bolts are not replicated (ADR-111), so the client has no bolt to draw.
2. **No GPU pool.** ADR-010's particle pool is compute work that does not exist.
3. **No shooter.** `LaserFired` names the bolt, not its shooter, so the recoil has no figure.
4. **No grenade entry.** Pass 10a is additive (RND-331), and the grenade is a lit mesh.
5. **No compile-time record.** The catalogue is created before the client reads its records, so `bias_laser` (RND-219) cannot come from the record.
6. **Gaps in the design:**
   - the grenade's tumble rate, the fuse blink's length and its inner flash's size are not given;
   - whether a muzzle flash's "size" is its half-size is not stated;
   - the ring's fan has no segment count.

## Decision Drivers

* The reference look as written, wherever M1 reaches it (MVP-082), with each deviation named.
* The presentation never writes simulation state, and its randomness never reaches the simulation's generator (DEC-007, DEC-013).
* A fixed number of draw calls per batch, whatever the number of effects (RND-455).
* Every value in data, in the client's own record (CONVENTIONS §4, ADR-116).

## Considered Options

1. **Rebuild from events and the replica, a CPU particle pool.** The client rebuilds bolts and muzzle flashes from the presentation events, draws grenades and rings from the replica, and keeps its particles on the CPU, drawn as two quad batches.
2. **Replicate the bolts.** The server sends each bolt as a state record.
3. **ADR-010 as written.** The particle pool in compute passes, with indirect draws.

## Decision Outcome

Chosen option: 1, because it needs no change to the wire. Option 2 adds tens of records a tick for objects that live six ticks. Option 3 adds compute passes before any measurement asks for them.

* **The bolts** (`GameClient/EffectsLook`; R-CRATE-440, R-CRATE-441):
  - **Playing an event.** An event of tick t is played as the display time reaches t − 1 (ADR-084 item 5).
  - **The first flight.** A `LaserFired` bolt flies from the muzzle at its velocity from that moment.
  - **The ricochet.** A `LaserRicochet` starts the second flight at the contact, from the moment the first flight reaches it.
  - **Removal.** A bolt is removed after `ref.weapon.laser.life_ticks` (6) ticks, when it leaves the region, or when its second flight meets the terrain.
  - **The draw** (RND-338, RND-339). Pass 10b draws `rnd.laser.layers` (5) identical `Segment` quads of `fx.beam_short`, `ref.weapon.laser.billboard_length` (10 wu) long, half-width `rnd.laser.half_width` (0.8 wu), in the source team's colour. The `laser_bolt` entry has the bias `bias_laser` (RND-220).
  - **The beam's mip level.** `UnlitTexturedLevelZeroPS` reads the beam at mip level 0 with `SampleLevel`, which stands in for AST-026's level-0 view.
  - **The shake** (RND-340). A bolt within √`ref.weapon.laser.shake_range_sq` of the camera requests a shake of `ref.weapon.laser.shake` (0.5).
* **The muzzle flashes** (R-CRATE-405, R-CRATE-475, VFX-222):
  - **Sources.** A laser shot's flash has size 25 × the bolt's life in seconds (15) and life 1.5; a throw's has 20 and 3.
  - **Motion.** It moves along its front at 10 wu/s, and its life falls by 10 a second.
  - **The draw.** Five nested camera-facing quads of `fx.muzzle_flash`, each × 0.8, at alpha clamp(life × 0.75, 0, 1). Below size 50 they use `effect_additive_nodepth`.
  - **Size.** A billboard's "size" is its half-size, as R-UNIT-330's orbs and RND-334's blast are (ADR-121).
* **The grenades** (R-CRATE-410, R-CRATE-414):
  - **The mesh.** `mesh.projectile.thrown_weapon`, drawn by `effect_mesh_lit` (MeshLit, Opaque, TestWrite, Back) in pass 10a before its additive draws. It tumbles about its right axis at `vfx.thrown.tumble_rate` (10 rad/s of scaled time, provisional, for the owner to tune).
  - **The fuse blink** shows during the last `ref.weapon.thrown.inner_flash_fraction` (20 %) of each second of its age. It is an `fx.starburst` quad in `ref.weapon.thrown.color` at alpha 200 of 255, of half-size 1000 ÷ √(camera distance), and a white inner quad at `vfx.thrown.inner_flash_scale` (0.5, provisional) of it.
  - **The trail.** Each tick in flight, with chance 1 − 0.1, it leaves a missile trail of size 60 at its previous position, at a quarter of its velocity plus a jitter of ± 2.5 wu/s per axis.
* **The explosions** (R-CRATE-406, VFX-219). Each `Explosion` event emits ⌊r × D × 0.01⌋ + ⌊U[0, n)⌋ cores and ⌊clamp(r × D × 0.01, 2, 30)⌋ debris in a ring, with VFX-219's sizes and velocities.
* **The shockwave rings** (R-CRATE-473, RND-333, RND-334):
  - **The copy.** A ring met in the replica is copied, its birth one tick before its active tick 0, so its blast outlives the replicated ring.
  - **The disc.** A fan of `rnd.shockwave.disc_segments` (32, provisional) triangles, drawn by `effect_gradient_nodepth` (UnlitVertexColor) while its life lasts. It lies 5 wu above the centre, of radius 35 + 40 × age, its rim at alpha 0.6 × the life left.
  - **The blast.** Five starbursts and five glows over its first second.
  - **The flash.** During the ring's first 0.1 s, while its centre passes RND-234's point test, a flash of alpha (1 − age ÷ 0.1) × max(0, 1 − distance ÷ 500), and a shake of that alpha.
* **The screen effects** (VFX-232 to VFX-235):
  - **The flash.** The frame's largest flash is one white quad of `effect_gradient_nodepth`, given in clip space (`TRIANGLE_SCREEN_SPACE`), last in pass 10a. At most `vfx.screen.flash_onsets` (3) onsets reaching `vfx.screen.flash_count_threshold` (0.1) are shown in a `vfx.screen.flash_window` (1 s) of real time (DEC-964), and a refused onset's flash is not drawn.
  - **The shake.** It decays 1 a second of unscaled time. At each 60 Hz step a yaw of `FxRng::NextSigned`(0.05 × shake) and a roll of 0.3 × `FxRng::NextSigned`(shake) are drawn and held.
  - **The offset.** The pair is added to the frame's camera pose alone, never the camera's: `CameraPose::rollRadians` turns the view's up about its front.
  - **Settings.** No settings store exists at M1, so the reduced-motion and reduced-flash scales are not applied.
* **The particles** (`NeuronClient/ParticlePool`, VFX-202 to VFX-217; ADR-010's M1 form):
  - **The pool.** It is CPU code with VFX-212's capacity, type cap and class fill limits, and VFX-213's distance thinning of bursts.
  - **Integration.** VFX-210's sub-steps of 1/60 s of scaled time, at most 30 a frame. Debris puffs missile trails.
  - **The types.** M1's six of table 4.2-A: explosion core, explosion debris, muzzle flare, control flash, missile trail and Worker fire. None has a terrain rule.
  - **The draw.** Pass 15: the smoke batch (`particle_darkening`), then the glow batch (`particle_additive`). Each particle is a camera-facing diamond of `fx.particle_dot`, read through the point-magnifying sampler (AST-027), with VFX-215's alpha and VFX-217's shade.
* **The emitters** (VFX-219):
  - **From events.** `WorkerPromoted` emits its control flashes, and `WorkersOrderedByPlayer` and `LeaderOrderWaypoint` their two flares. The client sees these events for its own team only (ADR-106).
  - **Per tick, from the replica.** Burning Workers, grenade trails, a Worker's change of crowd control (its `underControl` flag), and travelling order markers' step flares.
  - **Not at M1.** `LeaderFormUp` waits for the formation slots, which the client does not hold.
* **The order markers** (VFX-143). A travelling marker is an `fx.starburst` card of half-size `vfx.motion.order_marker_flare` (15 wu) in its team colour, fading over its last `ref.unit.leader.marker_fade_ticks` once it has arrived. Only the commanding team's client receives markers (ADR-106).
* **The recoil** (O75). A shot's shooter is the nearest living Worker of its team whose foot lies within `vfx.worker.recoil_match_range` (3 wu) of the muzzle's.
  - It is drawn pushed back from the shot by `vfx.worker.recoil_distance` (0.5 wu), falling linearly to 0 over `vfx.worker.recoil_time` (0.15 s of scaled time).
  - The three values are provisional, for the owner to tune.
* **The generators** (VFX-011). An event's generator is seeded from its tick and its order within the tick, and a per-tick emitter's from the tick, the object's id and its purpose. The pool and the screen effects have their own.
* **The catalogue** gains seven entries:
  - `effect_additive`, `effect_additive_nodepth`, `effect_gradient_nodepth` and `effect_mesh_lit` for pass 10a;
  - `laser_bolt` for pass 10b;
  - `particle_darkening` and `particle_additive` for pass 15.
* **The bias.** `PipelineEntry::depthBias` is the rasteriser's `DepthBias`. `DEPTH_BIAS_LASER` is RND-220's value of rnd.depth_bias.laser, compiled in, and a test holds it to the record.
* **The frame's order.** The passes `pass_10a_effects`, `pass_10b_laser_bolts` and `pass_15_particles` run in RND-070's order: 7b, 7d, 10a, 10b, 11, 15.

### Consequences

* Good, because the T16 run sees the battle: bolts, flashes, grenades, explosions, smoke, rings, the flash and the shake.
* Good, because the call count is fixed per pass, whatever the battle: two particle batches, one bolt batch, and one batch for each of 10a's entries and textures.
* Good, because every rule above is CPU code that CI tests.
* Bad, because a lost state datagram loses its events, so a bolt or a flash may be missing (ADR-084 item 5). A bolt whose ricochet is lost flies on until its life ends, under the terrain.
* Bad, because the CPU writes every particle each frame. ADR-010's GPU pool replaces the pool if T18's measurement exceeds `rnd.budget.effects_ms`.
* Neutral, because the recoil's match by position, not by shooter, may push a neighbour within 3 wu.

### Confirmation

* NeuronClientTests:
  - the pool's caps, distance thinning, sub-steps, fade, diamond and puffs (`ParticlePoolTests`);
  - the roll of the view (`WorldFrameTests`).
* GameClientTests (`EffectsLookTests`):
  - the compiled-in bias against the record;
  - the record's particle types;
  - a bolt's flight, life and ricochet;
  - the muzzle flash's motion and fade;
  - an explosion's cores and debris (VFX-219's test T9);
  - a ring's disc, blast, flash and shake (RND-333's test T12);
  - the flash onsets' limit, and the shake's decay;
  - a grenade's tumble and blink;
  - the recoil, the burning emitter and the control flash.
* The render check of ADR-117 on WARP, which creates every entry.
* The owner's T16 run, with its checklist.

## Pros and Cons of the Options

### 1 — rebuild from events and the replica, a CPU particle pool

* Good, because the wire is unchanged and the pool is tested in CI.
* Bad, because a lost datagram loses its effects.

### 2 — replicate the bolts

* Good, because a bolt is drawn where the server holds it.
* Bad, because each bolt costs a record for each of its ticks, and the reference's bolts are presentation.

### 3 — ADR-010 as written

* Good, because the CPU cost of the particles is fixed.
* Bad, because it adds three compute passes and indirect draws before M1 has measured the need.

* **Amendment 2026-10-22 (M2 task T15, [ADR-150](ADR-150-the-settings-store-and-user-data-root-at-m2.md)).** DEC-964's settings are applied. With `interface.reduced_flash`, the full-screen flash's alpha is multiplied by `vfx.screen.reduced_flash_scale` before its onsets are counted (VFX-235). With `interface.reduced_motion`, the shake's yaw and roll offsets are multiplied by `vfx.screen.reduced_motion_scale`, while the shake's value, decay and draws are unchanged (VFX-232).

## More Information

* Related: [ADR-005](ADR-005-pipeline-state-catalogue.md), [ADR-010](ADR-010-gpu-particles-and-shatter-debris.md), [ADR-036](ADR-036-effect-events-and-presentation-clock.md), [ADR-106](ADR-106-leaders-formations-and-battle-commands.md), [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md), [ADR-116](ADR-116-presentation-tuning-record.md), [ADR-121](ADR-121-workers-and-orbs-drawn-at-m1.md), [ADR-122](ADR-122-leaders-and-banners-drawn-at-m1.md).
* Owner decisions O75 and O77; DEC-964; R-CRATE-405, R-CRATE-406, R-CRATE-410, R-CRATE-414, R-CRATE-440, R-CRATE-441, R-CRATE-445, R-CRATE-473, R-CRATE-475; RND-219, RND-220, RND-331 to RND-340; VFX-143, VFX-202 to VFX-219, VFX-222, VFX-232 to VFX-235.
* The code: `NeuronClient/ParticlePool`, `DepthBias.h`, `QuadRenderer::RecordTriangles`, the `UnlitTexturedLevelZeroPS`, `UnlitTexturedParticleGlowPS`, `UnlitTexturedParticleSmokePS` and `UnlitVertexColor` shaders, `FrameView` and `FrameRenderer`; `GameClient/EffectsLook`, `ReplicaTiming` and `WorldPresentation`.
