---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T13)
---

# GPU resources at M1: the device factory, the heaps, the upload ring and textures without import records

## Context and Problem Statement

M1 task T13 is the first user of the GPU resource factory on the device. ADR-092 left the device implementation for the first textured draw, and ADR-078 and RND-009 fix its interface. The world frame (ADR-115) uploads:
- its terrain chunks, the sea's flat layer and shore mask;
- four textures, built once per world;
- a wave mesh and the sky's instances every frame.

These are open:
- the memory model at M1: committed resources, or ADR-008's placed allocator;
- the heap and ring sizes;
- what index 0 of the one resource heap holds (ADR-006);
- how a texture reaches the factory without the asset cache, its import records or its sampler choices (ADR-018, ADR-108).

## Decision Drivers

* A device rebuild keeps every resource's id and heap index (RND-035), and the world's owner uploads the same surfaces again.
* An upload never blocks the frame thread on the copy queue (RND-009).
* No d3dx12.h (AGENTS.md R14).

## Considered Options

* 1 — committed resources, a copy queue with per-upload staging buffers, fixed heap and ring sizes, and textures uploaded from their DDS bytes
* 2 — placed resources in pooled heaps (ADR-008) now
* 3 — uploads on the direct queue at the frame's start

## Decision Outcome

Chosen option: "1", the smallest form that keeps RND-009's interface and RND-035's rebuild contract. Pooled heaps (option 2) pay off with streaming, which M1 does not have. Direct-queue uploads (option 3) would serialise the copies with the frame.

* **The factory** (`NeuronClient/DeviceGpuResourceFactory`):
  - Every resource is committed in the default heap in the common state, with a view in the persistent region. An sRGB texture is created typeless and viewed through its UNORM format (ADR-013).
  - An upload is copied into its own staging buffer at once, within the 96 MiB of `rnd.resources.staging_budget_mib`, and is refused with `StagingFull` beyond it. A refused upload is repeated on a later frame by its owner.
  - The frame thread submits the copies on the copy queue at each frame's start. A resource is drawn only once its last copy's fence has completed.
  - A release is carried out once no frame in flight reads it and no copy writes it.
  - A rebuild creates every live resource again under its id and heap index, and the owner queues its uploads again (`WorldRenderer::UploadAgain`).
  - Static resources stay in the common state for their life: a buffer or a texture read by a shader is promoted implicitly by the direct queue and decays at the end of each submission, so no static resource needs a barrier.
* **The shader-visible heaps** (`NeuronClient/DescriptorHeaps`): 16,384 persistent descriptors and 2 × 2,048 transient ones, well inside resource binding tier 3. Index 0 is a null texture view, which reads zero (ADR-006 amendment). The sampler heap holds ADR-018's four samplers at their indices.
* **The upload ring** (`NeuronClient/UploadRing`, ADR-008) has a segment of 8 MiB per frame in flight. A frame of the world uses about 0.4 MB of it:
  - the frame and pass constants;
  - 255 sky instances of 80 bytes, about 20 KB;
  - the wave mesh, 8,056 triangles of 48 bytes, about 387 KB on seed 1's valley.

  The figures are counted from the built surfaces by the T13 probe and the record sizes.
* **Textures without import records** (ADR-108 amendment):
  - `NeuronClient/DdsTexture` describes a cooked DDS file's levels for the factory, a row of 4 × 4 blocks for a block-compressed format.
  - Each pass names its sampler by the rule that states it.
  - `fx.sky_cloud`'s `level0` view (AST-026) is the shader's read of level 0, with no second descriptor.
  - A BC4 mask's intensity is its red channel, and its alpha is 1 (ADR-115).
  - The import records come with the asset cache.
* **The ramp and the shore-wave gradient** are read on the CPU from their DDS bytes (AST-072) and never uploaded.

### Consequences

* Good, because every surface of the world is uploaded once, from the copy queue, and drawn only when it is complete.
* Good, because a rebuild needs nothing from the world's owner but the uploads it made the first time.
* Bad, because a staging buffer per upload costs one allocation per upload: 31 for a world at T13, made once.
* Bad, because committed resources cannot alias and are not budgeted by the allocation record of ADR-008, which comes with the placed allocator.
* It forecloses nothing: the factory's interface is the one placed resources and the asset cache use.

### Confirmation

* CI: `NeuronClientTests/WorldFrameTests` describes a BC4 chain and an sRGB texture, and refuses a short payload. `GameClientTests/WorldPresentationTests` builds seed 1's world from every texture of the content, under its key.
* The owner's T13 run: the textures are drawn, and Ctrl+Shift+F11 brings the same valley back.

## Pros and Cons of the Options

### 1 — committed resources and per-upload staging

* Good, because it is RND-009's contract with the least code.
* Bad, because it leaves ADR-008's allocator for a later milestone.

### 2 — placed resources now

* Good, because it is ADR-008 as written.
* Bad, because its pools and their budgets have nothing to manage at M1.

### 3 — direct-queue uploads

* Good, because no second queue or fence is needed.
* Bad, because every upload delays the frame that records it.

## More Information

* Related: [ADR-006](ADR-006-root-signature-and-bindless-resource-access.md), [ADR-008](ADR-008-gpu-memory-uploads-and-device-removal.md), [ADR-013](ADR-013-working-colour-space-and-blending.md), [ADR-017](ADR-017-texture-container-and-block-compression.md), [ADR-018](ADR-018-texture-import-manifest.md), [ADR-078](ADR-078-gpu-resource-factory.md), [ADR-092](ADR-092-first-window-device-and-presentation.md), [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md), [ADR-113](ADR-113-the-frame-at-m1.md).
* The rules: RND-009, RND-017, RND-035, AST-026, AST-072.
* The code is in NeuronClient (`DeviceGpuResourceFactory`, `DescriptorHeaps`, `UploadRing`, `DdsTexture`, `GpuDescriptions`).
