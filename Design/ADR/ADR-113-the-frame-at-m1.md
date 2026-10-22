---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T13)
---

# The frame at M1: the pass table, the catalogue, the barriers and the scene targets

## Context and Problem Statement

Task T13 of the [M1 plan](../implementation/m1_plan.md) draws the first world frame. It needs the frame infrastructure that ADR-004, ADR-005, ADR-007 and ADR-015 decide in outline:
- the ordered pass table and the barriers derived from it;
- the pipeline-state catalogue;
- the scene targets with multisampling and reversed depth;
- the recording of one frame.

These are open:
- which passes the world frame has at T13;
- how much of each baseline ADR M1 builds. Owner decision O75 accepts simpler M1 forms for ADR-004 (no transient aliasing) and ADR-005 (no pipeline library).

## Decision Drivers

* The frame must be valid on the owner's Adreno with the debug layer silent (M1 plan T13), and CI has no GPU.
* Game code describes nothing of the frame at M1, but the pass table must not name a Direct3D type, so that game passes can be added later (ADR-078).
* The frame must stay correct under resize, a mode switch and a device rebuild (RND-030, ADR-008).

## Considered Options

* 1 — a static world frame table in engine terms, validated at start-up; the resource-state barrier path only; committed scene targets
* 2 — the same, with both barrier executors (enhanced and resource-state) picked at start-up, as ADR-007 states
* 3 — hand-ordered recording with hand-written barriers

## Decision Outcome

Chosen option: "1", because it gives the frame graph's validation and derived barriers with half of the barrier code that no CI job can execute. Option 2's enhanced executor would be confirmed only on the owner's machine. Option 3 is ADR-004's rejected option.

* **The world frame** (RND-067 amendment) has these passes at T13, in order:
  - `pass_02_clear`: scene colour to `rnd.frame.clear_color`, scene depth to 0;
  - `pass_03_sky`, `pass_04_terrain`, `pass_05a_flat_sea`, `pass_05b_coastline_waves` (ADR-115);
  - `resolve`, only with more than one sample;
  - `final`: a full-screen triangle that reads the resolved scene into the back buffer with the brightness of RND-275;
  - `pass_34_present`.

  Pass 0, the frame constants, and pass 1, the camera, are CPU work at the frame's start. A frame without a world, or one whose surfaces are still uploading, clears the back buffer to `rnd.frame.clear_color_no_match` (RND-070).
* **The pass table** is `NeuronClient/PassTable` in engine terms (`TargetAccess`).
  - `PlanBarriers` validates it once per sample count when the device objects are created: unknown target, read before write, conflicting uses, written and never read.
  - Every catalogue key the table names must exist; a failure is `StartupError::FrameGraphInvalid`.
  - It derives the transitions before each pass and at the frame's end. Each target begins and ends the frame in one access: scene colour as a resolve source with multisampling and as a shader resource without it, scene depth as a depth write, the back buffer presented. So no barrier depends on the last frame's work.
* **Barriers** use the resource-state path (`ResourceBarrier`) only at M1. `StateOf` translates each `TargetAccess` to its resource state, and no pass issues a barrier of its own. The enhanced executor of ADR-007 waits for the milestone that needs it (ADR-007 amendment). The device's support is logged at start-up, as M0 does.
* **The catalogue** (`NeuronClient/PipelineCatalog`):
  - each entry is built from RND-055's default and its pass's differences, with every enumeration named in its initialiser;
  - it is created with the device objects, so at start-up and after a rebuild, and holds an entry per pass key at the scene's sample count;
  - there is no pipeline library (O75; ADR-005 amendment);
  - the blend states' alpha factors keep the destination alpha (`ZERO`, `ONE`), because an alpha factor may not name a colour and nothing reads the scene's alpha.

  The entries at T13 are `sky`, `terrain`, `sea_flat`, `sea_waves` and `final`.
* **The scene targets** (`NeuronClient/SceneTargets`, RND-063):
  - scene colour is `R8G8B8A8_UNORM` and scene depth `D32_FLOAT` with no stencil, at the client area's size (RND-025, until a stored render size exists);
  - the sample count is the first of 4, 2 and 1 that the device supports for both formats (ADR-015);
  - with more than one sample, a single-sample scene_resolved is the final pass's input;
  - each is a committed resource, with no aliasing (O75; ADR-004 amendment), and has an optimized clear value: black and depth 0 (reversed Z, ADR-016);
  - they are created again after the GPU is idle on every resize, and with the device objects on a rebuild;
  - the final pass's view of the scene is a persistent descriptor whose slot is kept across resizes.
* **Recording** uses one direct command list per frame. The heaps are set before the root signature. Every root parameter holds a value before the first draw. Each pass is bracketed by a CPU profile marker (DBG-045 amendment). The GPU markers and the timestamps per pass of RND-041 wait for the frame page of T15b.

### Consequences

* Good, because the frame's barriers are derived and validated in code that CI tests, and a malformed table stops the start-up with the pass named.
* Good, because the frame is identical at every sample count, and multisampling off is the reference's edge look (RND-433).
* Bad, because ADR-007's two-path confirmation cannot run until the enhanced executor exists. Until then the owner's run confirms one path only.
* Bad, because committed targets cost their full memory: at 2,880 × 1,920 with 4 samples, scene colour and depth take about 177 MB (computed from the formats, not measured).
* It forecloses nothing: the table, the accesses and the recorders are those an enhanced executor and an aliasing allocator take.

### Confirmation

* CI: the frame-graph tests validate the table's errors and derived barriers (`NeuronClientTests/FrameGraphTests`), and the build compiles every shader with the settings of ADR-112.
* The owner's T13 run: the valley is drawn with the debug layer silent at the device's sample count, through a resize, Alt+Enter and Ctrl+Shift+F11.

## Pros and Cons of the Options

### 1 — static table, resource-state barriers, committed targets

* Good, because every barrier comes from one table that CI validates.
* Bad, because the enhanced path, ADR-007's default, is not built.

### 2 — both executors

* Good, because it is ADR-007 as written.
* Bad, because the enhanced executor would be code that no automated check runs, and the Adreno's driver would be its only test.

### 3 — hand-ordered recording

* Good, because it is the least code.
* Bad, because every pass added later repeats the barrier reasoning by hand, which is ADR-004's rejected option.

## More Information

* Related: [ADR-004](ADR-004-frame-graph-from-the-ordered-pass-table.md), [ADR-005](ADR-005-pipeline-state-catalogue.md), [ADR-007](ADR-007-barrier-model.md), [ADR-012](ADR-012-swap-chain-presentation-and-frame-pacing.md), [ADR-015](ADR-015-scene-anti-aliasing.md), [ADR-016](ADR-016-view-projection-and-depth-conventions.md), [ADR-112](ADR-112-shader-build-and-shared-layouts.md), [ADR-114](ADR-114-gpu-resources-at-m1.md), [ADR-115](ADR-115-the-world-frame-at-m1.md).
* The rules: RND-050 to RND-070, RND-433, RND-442; owner decision O75.
* The code is in NeuronClient (`FrameRenderer`, `PassTable`, `PipelineCatalog`, `SceneTargets`).
* **Amendment 2026-10-22 (M2 task T12, [ADR-140](ADR-140-structures-drawn-and-the-stand-ins-at-m2.md)).** The world frame gains pass 6 `pass_06_structures_solid` after the coastline waves and pass 8 `pass_08_structures_alpha` after the Workers. The catalogue gains `mesh_lit_fog`, `port_pad` and `glow_additive_fog`, from the new pixel shaders `MeshLitFogPS` and `UnlitTexturedGlowFogPS`, and the mesh and quad vertex shaders pass the view depth for the level fog.
