---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Root signature and bindless resource access

## Context and Problem Statement

Species draws few distinct materials but very many instances: crowds of Workers, sprite cards, particles, debris shards and glyphs. Per frame, every draw needs the same per-frame data (view and projection, camera position, fog colour and range, two directional lights, the presentation time), a small amount of per-draw data (a world transform, a team tint, the indices of its instance buffer and textures), and access to textures and buffers whose number grows with the content. Instanced draws read per-instance data from buffers; compute passes for culling and particles read and write buffers that later draws consume.

Species needs a binding model in which a draw's cost does not depend on how many resources exist, where adding a texture does not change any pipeline state, and where CPU and GPU agree on every layout by construction.

The problem is how shaders reach their constants, buffers, textures and samplers.

## Decision Drivers

- One root signature for the whole renderer keeps the pipeline state catalogue (ADR-005) independent of resource counts.
- Per-draw CPU cost is a handful of root constants, not descriptor-table updates.
- Shader Model 6.6 dynamic resources are guaranteed by the minimum capability (ADR-002).
- Per-frame constants are written once per frame and bound once.
- Descriptor lifetime is safe with several frames in flight.
- Layouts are shared between C++ and HLSL (ADR-003).

## Considered Options

1. **One global bindless root signature** using Shader Model 6.6 directly indexed heaps, with a root constant-buffer view for per-frame data and root constants for per-draw data.
2. **Bindless through unbounded descriptor-table ranges** declared in the root signature.
3. **Per-material root signatures** with descriptor tables bound per draw.

## Decision Outcome

Proposed option: **1, one global bindless root signature**. **Default while Proposed:** the renderer is built on this binding model, because the minimum capability guarantees it and it makes per-draw cost independent of content size.

- **Root signature.** Version 1.1, one for graphics and one for compute, each with the flags `CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED` and `SAMPLER_HEAP_DIRECTLY_INDEXED`. Every pipeline state in the catalogue uses one of these two.
- **Root parameters (graphics).**
  - a root constant-buffer view for **per-frame constants**: view, projection and view-projection matrices, camera position, fog colour, fog start and end distances, the two directional lights (world-space direction and colour each), the presentation time, and the viewport size;
  - a root constant-buffer view for **per-pass constants**, used by passes that need them;
  - **per-draw root constants**: the draw's world transform as a 3x4 matrix, its tint, and the heap indices of its instance buffer, textures and material parameters. Instanced draws put per-instance transforms and colours in a structured buffer and pass only its index.
- **Heaps.** One shader-visible CBV/SRV/UAV heap and one shader-visible sampler heap, each bound once per command list. The CBV/SRV/UAV heap has a persistent region (allocated when an asset loads, released when it unloads, and reused only after every frame in flight that could reference it has completed) and a transient region used as a ring per frame in flight. Index 0 of each resource kind holds a valid default resource, so an unset index never reads an undefined descriptor.
- **Samplers.** The closed sampler set of ADR-018 lives in the sampler heap at fixed indices declared in the shared header.
- **Vertex input.** Mesh vertex streams come through the input assembler from vertex and index buffers; per-instance and per-draw data come from structured buffers through heap indices.
- **Shader rules.** Shaders index `ResourceDescriptorHeap[]` and `SamplerDescriptorHeap[]` with indices from root constants or buffers, and wrap any index that can differ within a wave in `NonUniformResourceIndex`.
- **Layouts.** Every constant and structured-buffer layout is declared once in the shared C++/HLSL header (ADR-003).

### Consequences

- Good, because a draw's CPU cost is setting a few root constants, whatever the number of textures and buffers in the scene.
- Good, because new content never changes the root signature or the pipeline state catalogue.
- Good, because instanced crowds, particles and glyphs share one access pattern for their per-instance data.
- Bad, because descriptor indices are plain integers: a stale or wrong index is not caught by the API and must be caught by the engine's debug checks and by GPU-based validation.
- Bad, because the persistent-region allocator must defer reuse across frames in flight, which is engine code to write and test.

### Confirmation

- GPU-based validation runs in the CI test configuration over a full match and fails on any descriptor error.
- Unit tests of the descriptor allocator check that a freed index is not reused before the last frame that could reference it has completed.
- A build check compiles the shared header in both C++ and HLSL and compares structure sizes and member offsets.
- A code check fails when a root signature other than the two global ones is created.

## Pros and Cons of the Options

### 1. One global bindless root signature

- Good, because it is the simplest model that scales with content.
- Good, because root constants carry per-draw data with no extra buffer writes.
- Bad, because errors move from the API to the engine's own index bookkeeping.

### 2. Unbounded descriptor-table ranges

- Good, because it is also bindless, and descriptor-table ranges are well understood.
- Bad, because the root signature must declare a range per resource type and space, which grows with every new kind of resource.
- Bad, because it gains nothing over directly indexed heaps on hardware that the minimum capability already requires.

### 3. Per-material root signatures with descriptor tables

- Good, because each shader declares exactly what it binds, and the API catches mismatches.
- Bad, because every draw writes or copies descriptors and binds tables, which costs CPU time at crowd scale.
- Bad, because root signatures multiply with materials and fragment the pipeline state catalogue.

## More Information

- Technology baseline item 4.3 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://raw.githubusercontent.com/microsoft/DirectX-Specs/master/d3d/HLSL_SM_6_6_DynamicResources.md · https://learn.microsoft.com/windows/win32/api/d3d12/ne-d3d12-d3d_shader_model.
- Related: ADR-002 (Shader Model 6.6 and Resource Binding Tier 3), ADR-003 (shared layouts), ADR-005 (catalogue), ADR-008 (frames in flight and memory), ADR-009 (instance buffers), ADR-018 (sampler set).
