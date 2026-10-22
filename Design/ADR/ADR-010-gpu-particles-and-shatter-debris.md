---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# GPU particles and shatter debris

## Context and Problem Statement

Battles in Species fill the screen with effects: sparks, fire, smoke, glow motes, muzzle flashes, explosions that shatter meshes into tumbling, lit facets with white edges, and debris that bounces on the terrain or dies when it touches it. Each particle type has its own parameter set (lifetime, speed, size, colour ramp, gravity, terrain behaviour), and two blend classes carry the look: additive glow, and darkening smoke that multiplies the destination by one minus the source colour, which needs no sorting.

A large battle can request more particles than any budget allows, and fast particles show visible stepping if they move only once per simulation tick. Effects are presentation only: they must never feed back into the simulation.

The problem is where particles and shatter debris are simulated, how their number is bounded, and how they are drawn.

## Decision Drivers

- Bounded cost: a hard cap on live particles and on GPU time, whatever the battle size.
- Smooth motion at any frame rate, independent of the simulation tick.
- The two blend classes and every particle type's parameters reproduced exactly.
- Terrain interaction (bounce, die on contact) without CPU read-back.
- Effects never write simulation state and never draw from the simulation's random generator.
- Minimum capability of ADR-002 only.

## Considered Options

1. A **GPU-simulated particle pool** (compute), with hard caps, distance LOD, a fixed presentation-clock sub-step, and instanced indirect draws; shatter debris as an instanced shard buffer.
2. A **CPU-simulated pool** whose live particles are uploaded and drawn instanced each frame.
3. **Third-party VFX middleware.**

## Decision Outcome

Chosen option: **1, a GPU-simulated particle pool with shard buffers for debris**, because it keeps particle cost off the CPU, bounds it by construction, and decouples particle motion from the simulation tick.

- **Pool.** One GPU particle pool (`ParticlePool`) holds every live particle in a structured buffer, with a free list and a live list managed by compute passes (append and consume buffers, or an equivalent ring). Each particle (`ParticleInstance`) records its type index, so per-type parameters come from one parameter buffer built from the effect definitions (ADR-036).
- **Emission.** Each frame the VFX runtime turns the effect events it has received into emission requests in an upload buffer; an emit pass allocates from the free list.
- **Caps and LOD.** The pool has a hard global capacity, and each type has its own cap. An emission that would exceed a cap is dropped, lowest-priority types first. Emission is scaled down with camera distance, and nothing is emitted beyond a type's LOD distance. The capacities, priorities and distances are VFX component tuning values.
- **Integration.** A simulate pass advances every live particle at a **fixed sub-step of the presentation clock** (ADR-036), running as many sub-steps as the frame needs, up to a maximum after which the clock is clamped. Particle motion therefore does not depend on the simulation tick or on the frame rate.
- **Terrain interaction.** The simulate pass samples a terrain height-field texture to apply each type's bounce and die-on-contact rules.
- **Drawing.** A compact pass writes the indirect arguments from the live count; particles are drawn as instanced quads (ADR-009) in two passes per frame: an additive pass for glow types and a darkening pass for smoke types, both unsorted.
- **Shatter debris.** A shatter event carries the mesh, its transform, the fraction of triangles to release and the facet colours. The VFX runtime fills a per-explosion shard buffer (one entry per triangle: centre, velocity, tumbler index, colour, death time) and a small set of tumbler rotations that a compute pass advances. Each explosion is one instanced draw of its shards, and all live explosions are issued together by one `ExecuteIndirect` call whose argument buffer holds one draw record per explosion, so the number of calls does not grow with the number of explosions; the lit-facet-with-white-edges look comes from the barycentric edge mask of ADR-003, with no edge texture.
- **Isolation from the simulation.** Particles and shards use the presentation random generator (`FxRng`), seeded per effect. Nothing in the simulation reads particle or shard state.

### Consequences

- Good, because particle cost is bounded by the pool capacity and is independent of CPU load.
- Good, because motion is smooth at every frame rate.
- Good, because darkening smoke stays order-independent, so no particle sorting is needed.
- Bad, because debugging GPU-side particle state needs development tools (a read-back of pool counts and a frozen-frame inspector).
- Bad, because dropped emissions under load change how a very large battle looks; the priorities must be tuned so that gameplay-relevant effects survive.

### Confirmation

- A stress test emits far beyond the pool capacity and checks that the live count never exceeds the cap and that GPU particle time stays within the VFX component's budget on the minimum-capability GPU.
- A frame-rate test renders the same effect at 30, 60 and 144 frames per second and checks that particle positions at matching presentation times agree within a tolerance.
- A code check fails when VFX code includes the simulation random generator's header.
- Golden-image tests cover each blend class and a shatter event.

## Pros and Cons of the Options

### 1. GPU-simulated pool and shard buffers

- Good, because it scales with GPU throughput and keeps the CPU free.
- Good, because terrain interaction needs no read-back.
- Bad, because effect logic is split between CPU emission and GPU simulation.

### 2. CPU-simulated pool

- Good, because effect logic is in one place and easy to debug.
- Bad, because simulating and uploading every live particle each frame costs CPU time and bandwidth that grow with battle size.

### 3. Third-party VFX middleware

- Good, because authoring tools come ready-made.
- Bad, because the look is a small, fixed set of particle types and blend classes that needs no general authoring system.
- Bad, because a middleware renderer would sit outside the frame graph and the pipeline state catalogue.
- Bad, because `AGENTS.md` R14 excludes third-party code from the executable.

## More Information

- Related: ADR-003 (edge mask), ADR-005 (additive and darkening blend states), ADR-009 (quad expansion and indirect draws), ADR-011 (terrain height-field texture), ADR-036 (effect events, presentation clock and effect definitions).
- The animation and VFX component (`architecture/components/animation_and_vfx/`) specifies the particle types and their parameter sets, the caps, priorities and LOD distances, and the shatter parameters.
- **Amendment 2026-10-17 (AGENTS.md, O21).** Third-party VFX middleware is excluded by `AGENTS.md` R14.
- **Amendment 2026-10-17.** "One instanced draw per explosion" is realised as one `ExecuteIndirect` call with one argument record per live explosion (VFX-237, RND-455).
- **Amendment 2026-10-21 (M1 task T16c, owner decision O75, [ADR-123](ADR-123-weapons-effects-and-particles-at-m1.md)).** M1's particle pool is CPU code (`NeuronClient/ParticlePool`): VFX-212's capacity, caps and class fill limits, VFX-213's distance thinning and VFX-210's sub-steps, drawn in pass 15 as two quad batches with one draw each. It holds the six types M1 emits, none with a terrain rule. No emit, simulate or compact dispatch exists at M1. The M1 form ends when a measured frame's effect passes exceed `rnd.budget.effects_ms` on the reference test machine (RND-454), whose task builds this ADR's GPU pool.
- **Amendment 2026-10-21 (M1 task T16, owner decision O77, [ADR-122](ADR-122-leaders-and-banners-drawn-at-m1.md)).** M1 builds no shatter: a dead Leader's figure is drawn while its corpse sinks, as the server replicates it. The shards of pass 14 come with this ADR's GPU particle pool, whose milestone builds them.
- **Amendment 2026-10-22 (M2 task T12, [ADR-141](ADR-141-the-turrets-shots-drawn-at-m2.md)).** M2 builds no shatter either: a turret round's shatter (R-CRATE-451) and a destroyed turret's head and barrels (R-BLDG-423) are not drawn, and the CPU pool's spark has no terrain bounce. Both come with this ADR's GPU particle pool.
