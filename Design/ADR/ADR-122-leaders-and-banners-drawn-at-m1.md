---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M1 task T16), owner (O75, O77)
---

# The Leaders and their banners drawn at M1

## Context and Problem Statement

M1 task T16's second step, T16b, draws the Leaders (M1 plan table 2 row T16; MVP-082, ADR-121).
- **The figure.** R-UNIT-190 draws a Leader as `mesh.unit.leader_figure` under the whole-model team tint, scaled and turned towards the camera as a Worker card is, with the damage flicker.
- **The banner.** R-UNIT-191 and VFX-139 to VFX-142 give it a banner of cloth at the figure's flag socket.
- **Where they are drawn.** RND-324 draws both in pass 7b, except the local team's banners, which the world markers of pass 16 draw (T17's).

Five things in the design do not hold as written at M1:
1. **No mesh or cloth path.** No mesh, cloth or two-texture path exists in the renderer.
2. **No palette.** The team palette of RND-016, which ADR-020's per-instance palette index reads, does not exist.
3. **No pass 16.** The local team's banners have no pass until T17.
4. **Unknown target.** A Waypoint banner's pointer aims at the order route's first waypoint, which is not replicated.
5. **A loose phrase.** "Turns towards the camera by d ÷ 200" names no quantity.

O77 deferred the death shatter of R-UNIT-189.

## Decision Drivers

* The reference look as written, wherever M1 reaches it (MVP-082), with each deviation named.
* A fixed number of draw calls per batch, whatever the number of Leaders (RND-455).
* Presentation state kept by the renderer, keyed by the object, and dropped with it (RND-301, RND-307).
* Every rule that runs once per reference frame runs in fixed 60 Hz steps on the unscaled clock (VFX-018).

## Considered Options

1. **Instanced meshes, CPU cloth drawn as triangles.** One instanced draw per entry for the figures and one triangle list per banner variant; the cloth simulated on the CPU.
2. **One draw per Leader.** The figure and its banner drawn with per-draw constants.
3. **The cloth on the GPU.** VFX-139 in a compute pass, as ADR-035 allows.

## Decision Outcome

Chosen option: 1, because it keeps RND-455's fixed call count and needs no compute pass. Option 2 grows with the number of Leaders. Option 3 adds a compute pass for at most a few dozen 6 × 6 grids.

* **The scene's meshes** (`NeuronClient/MeshRenderer`).
  - Each cooked mesh's 32-byte vertices are one vertex buffer as they stand (AST-076), uploaded once and again after a rebuild (RND-035).
  - A `MeshBatch` names its entry, its mesh and its view test's radius. Its `SceneMeshInstance`s each hold an origin, the model's three axes, whose lengths are its scale, and a tint. The CPU tests each instance's sphere (ADR-121's form of ADR-009) and writes the survivors as `MeshInstance`s, the camera-relative form of `DrawConstants::world`, for one instanced draw.
  - A geometry node's rest transform is the identity (ADR-019's T11 amendment), so the vertices take the instance's transform alone.
* **`MeshLit`** (RND-263, RND-324).
  - **Normals.** The facet normal goes through the instance's axes and is renormalised.
  - **Tint.** RND-271 clamp(c + w × (T × f − c), 0, 1), where w is the triangle's tint weight times the instance's, T the team colour and f `rnd.tint.factor` (0.7, R-UNIT-044).
  - **Lighting.** RND-228's object lighting: diffuse from the key and fill lights, with no specular, no ambient and no fog (RND-323).
  - **No palette at M1.** The tint is the instance's colour rather than a palette index; ADR-020 carries the amendment.
* **The Leader** (`GameClient/LeaderLook`, R-UNIT-190):
  - **Scale** 1 + 2.0 × (d − 100) ÷ 1,000 beyond 100 wu.
  - **Facing.** Its front blends from its heading towards the camera between 300 and 1,300 wu, as a Worker card turns.
  - **Death.** A dead Leader's figure is drawn while its corpse sinks, as the server replicates it, with no banner and no shatter (O77).
* **The damage flicker** (VFX-126, VFX-127, R-UNIT-051).
  - **The choice.** Once every `vfx.flicker.step` (0.1 s) of scaled time, a generator seeded from (the step, the Leader's id) chooses whether the Leader flickers: NextUnit(0.75) × (1 − |s| × 0.8) above its health fraction, with s = sin(t + 10 × the unit id's counter). It also chooses the axis, by the shares (front 0.3, up 0.3, right 0.4). A Leader at full health never flickers.
  - **The draw.** A flickering Leader is drawn by `actor_flicker_additive` in place of its body, its chosen axis scaled by 1 − s × 0.5 (front and right) or 1 − s × 0.2 (up).
* **The banner** (R-UNIT-191, VFX-139, VFX-141, VFX-142):
  - **Placement.** At the figure's flag socket, of size 20 × (1 + 2.0 × (d − 100) ÷ 1,500) beyond 100 wu. It streams behind the Leader.
  - **Turning to the camera.** "Turns towards the camera by d ÷ 200" is read as a blend of min(d ÷ 200, 1) of its direction towards the camera's right or left, whichever is nearer. Its texture flips where the turn shows it from the far side.
  - **Wobble.** While the Leader's order is not None, its up axis wobbles about its front by 0.4 rad × sin(2 × (t + 10 × the unit id's counter)).
  - **The cloth** is VFX-139's 6 × 6 grid, kept for each Leader by its id and stepped at `vfx.emit.step_rate` (60) on the unscaled clock, the remainder carried, at most 30 steps a frame. Each vertex relaxes towards its target by k:
    - 1 on the pole;
    - Δ × 10 × (6 − x) ÷ 6 elsewhere, or 0.7 a step when the banner is larger than 40 wu or its Leader is selected.
    A link never grows beyond half the size. A new Leader's cloth starts at its targets.
  - **The draw.** A white pole of half-width 0.2 wu and 1.5 sizes high (`cloth_pole`, an upright card with the solid shader). Then the 5 × 5 cells as triangles (`cloth_cells`), in one batch per variant: idle, follow or formation.
  - **The tint mask.** It gives the team area the team colour × 0.5, or the whole team colour while the Leader is selected.
* **Pass 7b** (`pass_07b_other_actors`) draws the figures, then the poles, then the cloth, before the Workers' pass 7d.
  - Until T17's world markers (pass 16) exist, pass 7b draws every Leader's banner, the local team's too (RND-324 amendment).
  - A Waypoint banner's pointer `marker.flag_pointer` waits for T17, which brings the order route to the client.
* **The death shatter** (R-UNIT-189, pass 14) is deferred (O77, ADR-010 amendment).

### Consequences

* Good, because the T16 run sees each side's Leaders as tinted figures under waving banners, flickering when wounded.
* Good, because the figures, poles and cloth take at most six draws a frame, whatever the number of Leaders.
* Bad, because the cloth costs CPU time per Leader per step: 36 vertices, at most 30 steps a frame.
* Bad, because a dead Leader sinks whole instead of shattering, until the particle pool's milestone.
* Neutral, because the local team's banners move to pass 16 with T17's world markers.

### Confirmation

* NeuronClientTests: the mesh instance's transpose form (`SceneQuadsTests`).
* GameClientTests (`LeaderLookTests`):
  - the figure's tint and facing;
  - the pole and banner of each variant, with the selected tint;
  - the cloth's pole column and link bound over two seconds of steps;
  - the flicker of a wounded Leader and the steadiness of a whole one;
  - the dropping of a Leader's state.
* The render check of ADR-117 on WARP.
* The owner's T16 run, with its checklist at T16c.

## Pros and Cons of the Options

### 1 — instanced meshes, CPU cloth drawn as triangles

* Good, because the call count is fixed and the cloth's arithmetic is tested in CI.
* Bad, because the cloth's vertices are written into the upload ring every frame.

### 2 — one draw per Leader

* Good, because it is the simplest renderer code.
* Bad, because its call count grows with the Leaders, which RND-455 forbids.

### 3 — the cloth on the GPU

* Good, because the CPU cost of the cloth vanishes.
* Bad, because a compute pass and its barriers serve at most a few dozen flags.

## More Information

* Related: [ADR-010](ADR-010-gpu-particles-and-shatter-debris.md), [ADR-019](ADR-019-mesh-source-format-and-cooked-meshes.md), [ADR-020](ADR-020-flat-shading-and-team-tint.md), [ADR-035](ADR-035-procedural-animation-runtime.md), [ADR-121](ADR-121-workers-and-orbs-drawn-at-m1.md).
* Owner decisions O75 and O77; R-UNIT-044, R-UNIT-051, R-UNIT-189 to R-UNIT-191; RND-228, RND-271, RND-324; VFX-018, VFX-126, VFX-127, VFX-139 to VFX-142.
* The code: `NeuronClient/MeshRenderer`, `QuadRenderer::RecordTriangles`, the `MeshLit` and `Cloth` shaders, `UnlitTexturedSolidPS` and `FrameRenderer`; `GameClient/LeaderLook`, `CrowdLook` and `WorldPresentation`.
* **Amendment 2026-10-21 (M1 task T17b, owner decision O78, [ADR-125](ADR-125-battle-marks-route-preview-and-world-markers-at-m1.md)).** A Waypoint banner with a replicated target carries the flag pointer: a camera-facing `marker.flag_pointer` at the cloth's centre, `vfx.cloth.pointer_size_factor` (0.25, provisional) × the banner's size, drawn white with `cloth_pointer` and the bias variant `bias_flag_pointer` at the end of pass 7b, after the cloth, and turned to point at the target on the screen. Pass 7b keeps drawing the local team's banners at M1, since the world markers of pass 16 draw no 3-D banner (RND-358 amendment).
