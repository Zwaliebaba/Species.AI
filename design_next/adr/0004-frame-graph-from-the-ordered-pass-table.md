---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# ADR-0004 Frame graph built from the ordered pass table

## Context and Problem Statement

A Species frame is a fixed, ordered sequence of passes: sky, terrain and its wire overlay, water, buildings and scenery, crowds and their shadows, effects and particles, additive glow, multiplicative darkening layers, world-space text, the 2-D UI, glyphs, and full-screen effects such as fades and the screen transition that samples a captured image of the previous screen. The order is part of the look: additive and darkening layers depend on what has been drawn before them, and only some passes need sorting (alpha-blended buildings, back to front).

On an explicit API, the engine itself must order the passes, move each resource between the states its passes need, allocate the intermediate targets (scene colour, depth, the captured image for transitions, the read-back buffer for screenshots), and keep per-frame resources separate for every frame in flight. Doing that by hand in each sub-renderer spreads synchronisation and lifetime rules across the code base.

The renderer keeps one **ordered pass table** as the single source of truth for pass order and for the blend, depth and cull combinations each pass uses (the latter feed the pipeline state catalogue, ADR-0005). The problem is how the frame is assembled and synchronised from that table.

## Decision Drivers

- The ordered pass table is the single source of truth for pass order and pass state.
- Synchronisation and resource lifetime are derived in one place, not written by hand per pass (ADR-0007).
- Transient targets are allocated, and aliased where lifetimes allow, without per-feature code.
- Predictable cost: the frame topology changes only when the render configuration changes (resolution, settings), never per frame.
- Passes can be recorded in parallel once the threading model is decided.
- Every pass is visible to profiling (one marker and one GPU timestamp pair per pass).

## Considered Options

1. A **static, declarative frame graph** compiled from the ordered pass table: each pass declares the resources it reads and writes; the graph is compiled when the render configuration changes.
2. A **dynamic render graph** rebuilt every frame, with automatic removal of passes whose outputs nobody reads.
3. **Hand-ordered recording**: each sub-renderer records its own commands and barriers in a fixed call order.

## Decision Outcome

Proposed option: **1, a static frame graph compiled from the pass table**. **Default while Proposed:** the team builds the renderer this way, because it derives synchronisation and lifetimes automatically while keeping the frame as predictable as hand-ordered code.

- **Pass table as data.** Each entry names the pass, the pipeline-state catalogue keys it may use, the resources it reads and writes (with the access each needs), its sort rule (unsorted, front to back, or back to front), and the setting, if any, that enables it. The renderer component owns the table.
- **Resources.** Every target and buffer the frame uses is declared by name with its format and size class (fixed, or relative to the back buffer). Transient resources whose lifetimes do not overlap share memory through placed resources in one heap per size class.
- **Compilation.** The graph is compiled at start-up and again on any change of render configuration (resize, resolution scale, anti-aliasing setting, HDR toggle). Compilation validates the table: no resource is read before it is written, every written resource is read or presented, and every pass's catalogue keys exist. A failed validation stops the renderer with a diagnostic naming the pass.
- **Synchronisation.** The compiled graph derives every barrier from the declared accesses, in the terms of ADR-0007. No sub-renderer issues a barrier itself.
- **Frames in flight.** Per-frame resources (upload ring segments, command allocators, per-frame constants, read-back buffers) exist once per frame in flight. The number of frames in flight follows ADR-0012.
- **Queues.** The frame runs on one direct queue; uploads run on a copy queue (ADR-0008). An asynchronous compute queue for particle and culling passes is an optional change that needs measurements first.
- **Recording.** Each pass, or each group of adjacent passes, records into its own command list with its own allocator per frame in flight, so passes can be recorded in parallel. Which threads record them is decided with the threading model, which is deferred to the design phase.
- **Captured images and read-back.** The screen transition and any effect that samples an earlier image read a declared frame-graph target that an earlier pass rendered or copied, never the back buffer in place. Screenshots are a read-back pass that copies the final image into a read-back buffer, which the CPU maps after the frame's fence completes; image encoding belongs to the diagnostics decisions.
- **Profiling.** Every pass emits one CPU and GPU marker and one pair of GPU timestamps through the engine's profiling layer.

### Consequences

- Good, because barrier placement, transient allocation and aliasing are computed and validated in one place.
- Good, because the frame topology is fixed per configuration, so per-frame CPU cost is a walk over a precompiled list.
- Good, because adding a pass is a table entry plus its recording function, with no synchronisation code.
- Bad, because a static graph cannot drop unused passes automatically; passes that are off are disabled through their setting, and the graph is recompiled.
- Bad, because the graph compiler is engine code that must be written and tested before the first frame renders.

### Confirmation

- Unit tests compile sample tables and check the derived barrier list, the aliasing plan and the validation errors (read before write, unread write, unknown catalogue key).
- A CI test renders the reference frame with the debug layer and GPU-based validation enabled and fails on any message.
- A code check fails when a barrier call appears outside the frame-graph executor.
- The profiling capture of a test frame shows one marker per pass, in table order.

## Pros and Cons of the Options

### 1. Static frame graph compiled from the pass table

- Good, because it keeps the pass table authoritative and makes the frame reproducible.
- Good, because it compiles rarely and costs almost nothing per frame.
- Neutral, because conditional passes are handled through settings and recompilation rather than per-frame culling.
- Bad, because the graph compiler is up-front work.

### 2. Dynamic render graph rebuilt every frame

- Good, because passes can appear and disappear per frame and unused work is culled automatically.
- Bad, because rebuilding and re-validating the graph every frame costs CPU time that a fixed frame does not need.
- Bad, because the frame's shape can change from frame to frame, which makes performance and golden-image tests less stable.

### 3. Hand-ordered recording

- Good, because there is no graph machinery to build.
- Bad, because each sub-renderer must know the state every resource is in when it starts, so synchronisation rules spread across the code and break silently when a pass is added or reordered.
- Bad, because transient memory cannot be aliased without global knowledge.

## More Information

- Related: ADR-0001 (explicit renderer), ADR-0005 (pipeline state catalogue), ADR-0007 (barrier model), ADR-0008 (uploads and memory), ADR-0012 (frames in flight and presentation), ADR-0014 (anti-aliasing targets).
- The renderer component (`architecture/components/renderer/`) holds the ordered pass table and the recording function of every pass.
- The threading model that decides which threads record passes is deferred to the design phase (see [the index](README.md#deferred-to-the-design-phase)).
