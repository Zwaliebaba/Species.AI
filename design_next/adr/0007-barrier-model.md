---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Barrier model: Enhanced Barriers with a resource-state fallback

## Context and Problem Statement

Every Species frame moves resources between uses: the scene colour target is written by world passes and then sampled by the screen transition; depth is written and then only tested; particle buffers are written by compute and read as instance data; uploads arrive through the copy queue; transient targets share memory (ADR-0004). On Direct3D 12 the engine must declare each of these hazards with a barrier.

Direct3D 12 offers two barrier APIs. **Enhanced Barriers** (`Barrier()`, with separate synchronisation scope, access and texture layout) express exactly what a transition needs and allow resources to be created in an initial layout. They have been in the Agility SDK since 1.608.0, but they are **not** a hardware or driver requirement: support must be checked through `D3D12_FEATURE_DATA_D3D12_OPTIONS12.EnhancedBarriersSupported`. The `ResourceBarrier` API, based on combined resource states, works everywhere.

Species needs one way of thinking about synchronisation in the frame graph, the best barriers on hardware that supports them, and correct rendering on hardware that does not. The problem is which barrier model the frame graph uses and whether Enhanced Barriers are required.

## Decision Drivers

- One synchronisation vocabulary in the frame graph, independent of the API that executes it.
- Precise barriers (sync, access and layout separate) where the driver supports them.
- No increase of the minimum capability of ADR-0002 for a feature whose reach is unknown.
- Correctness verified by the debug layer and GPU-based validation in CI on both paths.
- Aliasing of transient resources (ADR-0004) is expressed safely.

## Considered Options

1. **Enhanced-Barrier model in the frame graph**, executed with `Barrier()` where supported and **translated to the `ResourceBarrier` API** otherwise.
2. **Require Enhanced Barriers**, and add `EnhancedBarriersSupported` to the start-up checks.
3. **`ResourceBarrier` API only.**

## Decision Outcome

Proposed option: **1, Enhanced-Barrier model with a resource-state fallback**. **Default while Proposed:** Enhanced Barriers are not a minimum requirement, and the frame graph runs on both paths, because their hardware reach is not verified and the translation keeps one model in the engine.

- The frame graph describes every transition as a triple of synchronisation scope, access and (for textures) layout, before and after. Buffers have no layout.
- At start-up the renderer reads `EnhancedBarriersSupported`. When it is `TRUE`, the executor emits `Barrier()` groups and creates textures with an initial layout. When it is `FALSE`, the executor translates each triple to resource states and emits `ResourceBarrier` calls; textures are created in the matching initial state.
- The translation table is part of the renderer component, covers every triple the pass table uses, and is unit-tested.
- Aliased transient resources receive an aliasing barrier on first use in each frame (on the fallback path, the `ResourceBarrier` aliasing barrier; on the enhanced path, a barrier from the no-access state with the discard flag).
- Resources shared between the copy queue and the direct queue are handed over at a fence, in the common layout or state, as both APIs require.
- Split barriers are not used in the baseline; they are an optimisation that needs profiling evidence first.
- Fence barriers, which are preview-only in the Agility SDK, are not used.
- Requiring Enhanced Barriers (option 2) remains open; the decision is revisited when hardware data shows their reach among Species-capable GPUs.

### Consequences

- Good, because the frame graph has one synchronisation model, and every pass is described the same way on every GPU.
- Good, because supporting hardware gets precise barriers and initial-layout creation.
- Good, because no player is excluded for lacking a driver feature that the look does not need.
- Bad, because there are two execution paths, and both must be tested on real hardware.
- Bad, because the translation to resource states is coarser, so the fallback path may synchronise more than needed.

### Confirmation

- Unit tests run the translation table over every triple used by the pass table and compare the result with the expected resource states.
- The CI test configuration renders the reference frame twice, once on each path (the enhanced path forced off by a development setting), with the debug layer and GPU-based validation enabled, and compares both images with the golden image.
- QA's hardware matrix includes at least one GPU without Enhanced-Barrier support.

## Pros and Cons of the Options

### 1. Enhanced-Barrier model with a fallback

- Good, because it uses the better API where available without excluding anyone.
- Bad, because two paths double the barrier test surface.

### 2. Require Enhanced Barriers

- Good, because there is only one execution path.
- Bad, because GPUs whose drivers do not report support would be excluded, and how many there are is not verified.

### 3. `ResourceBarrier` API only

- Good, because it works everywhere with one path.
- Bad, because combined resource states over-synchronise and cannot express initial layouts or some read-only combinations precisely.
- Bad, because the engine would later need a rewrite of its synchronisation model to adopt Enhanced Barriers.

## More Information

- Technology baseline item 4.4 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://learn.microsoft.com/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_d3d12_options12 · https://learn.microsoft.com/windows-hardware/drivers/display/enhanced-barriers · https://learn.microsoft.com/windows-hardware/drivers/ddi/d3d12umddi/nc-d3d12umddi-pfnd3d12ddi_barrier_0094 · https://devblogs.microsoft.com/directx/directx12agility/.
- Related: ADR-0002 (minimum capability), ADR-0004 (frame graph), ADR-0008 (copy-queue uploads).
