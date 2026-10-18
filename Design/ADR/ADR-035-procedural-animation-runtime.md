---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Procedural animation runtime

## Context and Problem Statement

Species has no skeletal animation. Its motion is procedural: spiders and other legged creatures plant and swing their feet with a gait planner (a home position per foot, a step triggered by distance with a threshold and a lock-out, an eased swing with a trapezoidal lift, look-ahead with yaw prediction); segmented creatures (the centipede, the Reaper Serpent) and Crawlers follow trails of their own past positions; cloth flags ripple; turret heads, barrels and dish arms turn; Workers are flat cards whose motion comes from their positions; and the help screens play 2-D explainer animations. All of this is presentation: it must follow the authoritative state of the simulation, must never change it, and must look the same at any frame rate and game speed.

The problem is where and how procedural animation is evaluated, and on which clock.

## Decision Drivers

- Animation reads authoritative simulation state and never writes it.
- Smooth motion at any frame rate, pausable and aware of game speed.
- The gait planner's behaviour kept, with leg segments that keep their lengths.
- Many trail-following and cloth elements at low CPU cost.
- No dependence on authored animation clips.

## Considered Options

1. **Client-side procedural animation** driven by the authoritative transforms of the simulation, on an explicit **presentation clock**, with **two-bone IK** for legs and **GPU evaluation** of strips, trails and cloth.
2. **Skeletal animation** with authored clips and blend trees.
3. Procedural animation **evaluated inside the simulation step**, as part of simulation state.

## Decision Outcome

Chosen option: **1, client-side procedural animation on the presentation clock**, because it keeps every procedural technique while separating it completely from the simulation.

- **Inputs.** Each frame, the animation runtime reads the presentation view of the simulation (body positions, facings and velocities, interpolated between ticks as `reference/12_simulation_model/` specifies). It writes only presentation data: mesh node transforms, instance data and GPU buffers.
- **Clock.** Every procedural motion advances on the **presentation clock** (ADR-036), which pauses with the game and scales with game speed where the visual language says so. Nothing reads wall-clock time or a raw frame delta.
- **Legs.** The gait planner (`SpiderGait`, `ProceduralLeg`, `ProceduralFoot`) runs on the client from the authoritative body transform. Each leg is solved with **two-bone inverse kinematics** by the law of cosines, so the upper and lower segments keep their lengths; the planner's parameters are VFX component tuning values.
- **Trails, strips and cloth.** Segmented creatures, Crawler strips and trails keep a history buffer of past positions (`TrailPoint`); cloth flags (`ClothFlag`) keep their simulation state per flag. Segment transforms, strip vertices and cloth vertices are built in a vertex or compute shader from these buffers.
- **Rigid parts.** Turning parts are mesh nodes (ADR-019) whose transforms are set from presentation state, with sockets following them (ADR-021).
- **Explainer animations.** The 2-D explainer animations of the help screens are a UI timeline component (`ExplainerAnimation`) played by the UI framework (ADR-030); their colour roles are resolved against the local player's team colour at draw time.
- **Not used.** Skeletal animation, skins and morph targets are not part of the runtime (ADR-019).

### Consequences

- Good, because animation can never change the simulation, and the simulation never waits for animation.
- Good, because legs look correct at every stride, and motion is smooth at every frame rate.
- Good, because trail, strip and cloth evaluation costs little CPU time.
- Neutral, because legs look different from the reference, in which the shin keeps its length and is turned outward by θ = −acos(min(1, 0.4·L ÷ thigh length)), the ratio clamped to 1, and the thigh is drawn from knee to hip at whatever length results, so it stretches and compresses; in Species both segments keep their lengths.
- Bad, because procedural motion is code, so new creature motion needs engineering work rather than authored clips.
- Bad, because the presentation view must provide velocities and history for every animated body, which the simulation-to-presentation interface must carry.

### Confirmation

- A unit test of the two-bone solver checks segment lengths and foot placement over reachable and unreachable targets.
- A frame-rate test replays the same match at 30, 60 and 144 frames per second and checks that leg, trail and cloth poses at matching presentation times agree within a tolerance.
- A code check fails when animation code takes write access to simulation state.

## Pros and Cons of the Options

### 1. Client-side procedural animation

- Good, because it keeps the techniques and the separation from the simulation.
- Bad, because every motion is code.

### 2. Skeletal animation with authored clips

- Good, because animators can author motion directly.
- Bad, because it changes the motion of the look, adds a skinning pipeline the art does not need, and still needs procedural foot placement on terrain.

### 3. Animation inside the simulation step

- Good, because animation state would be exact and identical everywhere.
- Bad, because presentation work would cost simulation time on every process that runs the simulation, including the dedicated server, for no gameplay effect.

## More Information

- Related: ADR-019 (mesh nodes), ADR-021 (sockets on moving parts), ADR-030 (UI timelines), ADR-036 (presentation clock and effect events).
- The animation and VFX component (`architecture/components/animation_and_vfx/`) specifies the gait planner, the trail and cloth models and their tuning values.
