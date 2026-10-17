---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Effect events, presentation clock and effect definitions

## Context and Problem Statement

Gameplay produces effects: explosions with a position and radius, units catching fire, meshes shattering into debris, muzzle flashes, impacts, camera shake, full-screen flashes. Effects are presentation only. They must never write simulation state, never draw from the simulation's random generator, and never depend on the frame rate: an effect that emits particles every frame, or shakes the camera once per frame, looks different at 30 and 144 frames per second. Effects that involve randomness should look the same when the same moment is replayed. Particle types and emitter bursts are many, and tuning them should not need code changes. Bright flashes and camera shake are accessibility concerns (photosensitivity and motion settings, ADR-0064).

How simulation state reaches presentation in the world game (in-process or from a server) is part of the networking and threading models, which are deferred to the design phase. This decision covers the contract between gameplay and the VFX runtime, the clock that presentation runs on, and how effects are defined.

The problem is how the simulation asks for effects, which clock effects and procedural animation use, and how effects are described.

## Decision Drivers

- Effects never write simulation state or use the simulation's random generator.
- Effect behaviour independent of frame rate, and pausable with the game.
- Effects reproducible on replay on the same client.
- Particle types and bursts described as data, reloadable in development builds.
- Flash intensity and camera motion bounded by the accessibility settings.
- Independence from the deferred transport of simulation state.

## Considered Options

1. The simulation **emits effect events** into a per-tick stream that a **client-side VFX runtime** consumes on an explicit **presentation clock**, with effects described in **effect definitions** (data).
2. Simulation code **calls the VFX system directly** to spawn particles and effects.
3. Presentation **infers effects** by comparing simulation state between ticks.

## Decision Outcome

Chosen option: **1, effect events, a presentation clock and data-defined effects**, because it gives the simulation a narrow, one-way interface to presentation and makes every effect independent of the frame rate.

- **Effect events.** The simulation emits effect events, each with a kind from a closed enumeration defined in the VFX component (for example explosion with position and radius, burning with the unit's handle, shatter with the mesh, transform, fraction and facet colours, camera shake with an intensity), the tick it belongs to, and a stable event identity. The simulation knows nothing about how an effect looks. Events are presentation data: presentation may drop or simplify them (for distance, level of detail or caps, ADR-0010) without any effect on the game.
- **Stream.** Events are delivered to presentation in tick order. Whether the stream is produced in-process or arrives from a server is decided with the networking and threading models; the VFX runtime depends only on receiving events in tick order with their tick.
- **Randomness.** The VFX runtime seeds the presentation random generator (`FxRng::Seed`) per effect from the event identity, so the same event looks the same on replay on the same client. The simulation random generator is never visible to VFX code.
- **Presentation clock.** One presentation clock drives VFX, particles (ADR-0010) and procedural animation (ADR-0035). It reads the platform clock (ADR-0074) once per frame, clamps a single frame's step to a maximum after a stall, and gives three readings of that one time source: the **scaled** reading advances by the step times the game speed, the **unscaled** reading advances by the step, and both stop while the game is paused (the world is held); the **real** reading always follows the platform clock and drives UI animation and explainers. Each motion and effect names the reading it uses. The camera keeps its own clock from the same platform source, unscaled and running while the game is paused, so the camera stays live during a pause. Because the platform clock accepts injected time, captures and replays can drive presentation at a fixed rate.
- **Screen effects.** Camera shake is an additive rotation offset applied to each frame's view and never stored in the camera's pose, with linear decay over unscaled presentation time; flashes are a full-screen additive pass with an intensity curve over presentation time. Requests are accumulated per event, not per frame, so their strength never depends on the frame rate. Flash intensity is capped by the photosensitivity rule and shake is scaled by the player's motion setting (ADR-0064).
- **Effect definitions.** Each effect kind maps to an **effect definition** in data (in the schema of ADR-0061): the particle types it uses, with their parameter sets (lifetime, speed, size, colour ramp, gravity, terrain behaviour, blend class, priority, cap, LOD distance), its emitter bursts (counts, spread, timing), its attachments to sockets (ADR-0021) or positions, and its screen effects. Definitions are validated by the content build and reload while the game runs in development builds.

### Consequences

- Good, because the simulation has a one-way, read-only relationship with presentation, which the deferred networking model can carry without changes to the VFX runtime.
- Good, because every effect is frame-rate independent and pauses with the game.
- Good, because effects look the same on replay.
- Good, because tuning effects is a data change.
- Bad, because every effect the game needs must have an event kind and a definition, which adds to the content and to the event enumeration.
- Bad, because presentation can drop events, so no gameplay information may be carried only by an effect.

### Confirmation

- A code check fails when VFX code includes the simulation random generator's header or takes write access to simulation state.
- A replay test plays the same recorded match twice and checks that every effect seeded from the same event produces the same particle emission sequence.
- A frame-rate test checks that camera-shake displacement and flash intensity over presentation time are equal at 30 and 144 frames per second.
- The content-build validator rejects effect definitions with unknown particle types, sockets or parameters, and event kinds without a definition.

## Pros and Cons of the Options

### 1. Effect events, presentation clock, effect definitions

- Good, because the boundary between simulation and presentation is narrow and one-way.
- Bad, because event kinds and definitions must be maintained.

### 2. Direct calls from simulation code

- Good, because it is the quickest way to add an effect.
- Bad, because simulation code then depends on presentation, can draw from the wrong random generator, and runs presentation work on every process that steps the simulation.

### 3. Inferring effects from state changes

- Good, because the simulation needs no event interface.
- Bad, because many effects (impacts, flashes, shake) have no lasting state to compare, and inference rules duplicate gameplay logic in presentation.

## More Information

- Related: ADR-0010 (particle pool and caps), ADR-0021 (sockets), ADR-0035 (procedural animation), ADR-0061 (data schemas), ADR-0064 (photosensitivity and motion settings), ADR-0074 (platform clock with injectable time).
- The animation and VFX component (`architecture/components/animation_and_vfx/`) specifies the effect event kinds, the effect-definition schema and the particle parameter sets; the networking and threading models are deferred to the design phase (see [the index](README.md#deferred-to-the-design-phase)).
- **Amendment 2026-10-17.** The presentation clock has three readings of one platform time source, scaled, unscaled and real (VFX-009, VFX-016); the camera clock is separate, unscaled and keeps running while the world is held (CAM-030, R-SIM-571); camera shake is a view offset that is never stored (VFX-232, CAM-012).
