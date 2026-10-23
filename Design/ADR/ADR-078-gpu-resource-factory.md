---
status: proposed
date: 2026-10-18
decision-makers: Species technology lead
---

# GPU resource factory: one creation seam for every client GPU resource

## Context and Problem Statement

The renderer owns the Direct3D 12 device, the swap chain, the direct and copy queues, the descriptor heaps and the frame graph ([ADR-001](ADR-001-direct3d-12-renderer-on-the-agility-sdk.md), [ADR-004](ADR-004-frame-graph-from-the-ordered-pass-table.md), [ADR-006](ADR-006-root-signature-and-bindless-resource-access.md)). Several other client components hold GPU data of their own: the asset cache creates the textures and meshes of the packs and uploads them, and registers the run-time textures that other components produce; the UI framework's glyph cache keeps its glyph pages ([ADR-034](ADR-034-text-rendering-and-glyph-cache.md)); the animation and VFX runtime keeps the particle pool, its parameter buffer and the shard buffers ([ADR-010](ADR-010-gpu-particles-and-shatter-debris.md)); and the terrain, sea and tree renderers build static buffers at map load ([ADR-011](ADR-011-world-surface-rendering.md)).

[ADR-008](ADR-008-gpu-memory-uploads-and-device-removal.md) fixes the memory model: static resources in default heaps, filled through the copy queue with a fence per upload batch; release only after every frame in flight that may use a resource; and a full rebuild after device removal, with every resource recreated from its source and no CPU shadow copy. It names no interface through which a component other than the renderer creates or fills a resource. The start-up order adds a constraint: the packs are mounted and the asset cache starts its CPU loads (platform start-up step 11) before the renderer creates the device (step 13), so the asset cache exists before any device does.

Through which interface do client components create and fill GPU resources, who owns the copy queue, and how do those resources survive a device rebuild?

## Decision Drivers

* One owner of the device, its queues and its descriptor heaps.
* Loaders create and fill resources from their own threads and never wait for the frame thread.
* A resource is released only after every frame in flight that may use it has completed.
* A device rebuild recreates every client resource without a CPU shadow copy, and keeps the bindless heap index of each resource, so that handles and instance data stay valid.
* CPU work, the asset cache's mounting and loads included, can start before the device exists.
* The components that create resources can be tested without a GPU.

## Considered Options

* A renderer-owned factory interface (`GpuResourceFactory`) through which every client resource is created, filled, released and rebuilt
* The device and a copy queue handed to every component that needs GPU resources
* Every creation and upload sent as a request to the frame thread, which serves it between frames

## Decision Outcome

Proposed option: "A renderer-owned factory interface". **Default while Proposed:** every client GPU resource is created through `Neuron::GpuResourceFactory`, declared in the [renderer](../architecture/components/renderer/README.md) §2 (RND-001, RND-009), because it keeps the device, the copy queue and the descriptor heaps in one owner and gives the rebuild of ADR-008 one list of resources to recreate. **Confirmed when:** the first textured draw of the first playable (O20) loads its textures through the asset cache attached to the factory, and the forced device removal of ADR-008 during a match recreates every factory resource at its heap index with the golden image unchanged afterwards; both are checked when the asset cache is first connected to the renderer, because the seam is on the path of every texture.

* **Scope.** The factory creates every GPU resource that a component outside the renderer holds: the asset cache's textures, meshes and run-time textures, the glyph pages, the particle and shard buffers, and the terrain, sea and tree buffers. The renderer's own resources, the swap chain, the frame-graph targets, the upload ring and the pipeline states, are not factory resources. Per-frame dynamic data stays in the upload ring of ADR-008, which a pass reaches through its pass context.
* **Creation.** A resource is created from a description: a texture from the size, level count and `DXGI_FORMAT` of its cooked DDS header ([ADR-017](ADR-017-texture-container-and-block-compression.md)) or of a run-time texture; a buffer from its size, its element stride and whether the GPU writes it. The factory creates it in a default heap with its shader-visible descriptor in the persistent region of the heap of ADR-006, and returns an id (a table slot and a generation). The heap index belongs to the resource for its whole life, device rebuilds included.
* **Uploads.** A copy-queue upload copies the caller's bytes into the factory's staging memory on the calling thread and queues the copy; the frame thread submits the queued copies to the copy queue once per frame, signals the copy fence and gives each upload the fence value of its batch. A resource holds its data, and may be drawn, only once that value has completed (ADR-008); the handover between the copy queue and the direct queue is at that fence, in the common layout or state ([ADR-007](ADR-007-barrier-model.md)). When the staging memory is full, the upload is refused at once, never waited for, and the caller repeats it on a later frame.
* **Same-frame writes.** A resource whose content changes within a frame, a glyph page that receives new glyphs or a buffer that compute work writes, is created by the factory and written by a pass of the frame graph that imports it by name, so its barriers are derived by the graph like those of any other frame-graph resource (ADR-004, ADR-007).
* **Release.** A release may be made from any thread. The factory destroys the resource, and frees its descriptor slot for reuse, only after every frame in flight that may use it has completed ([ADR-012](ADR-012-swap-chain-presentation-and-frame-pacing.md)) and after its last queued upload has completed.
* **Device rebuild.** During the rebuild of ADR-008 the factory releases every device object, then recreates every live resource from its stored description on the new device, under the same id and at the same heap index, holding no data. The renderer then raises `DeviceRebuilt`, and each owner fills its resources again from its own source: the asset cache from the pack entries and the producers of its run-time textures, the glyph cache by rasterising again (ADR-034), the VFX runtime by starting its pool and shard buffers empty, and the world renderers from the terrain field and the tree parameters. A call made while the rebuild runs does not wait: it is refused, and its caller repeats it after `DeviceRebuilt`.
* **Threading.** Creation, upload and release may be called from any thread. Direct3D 12 command lists are not free-threaded, and any thread may submit to a command queue, which serialises the submissions; the factory therefore records the copies of each thread into its own command list, and only the frame thread submits them, on the copy queue that belongs to the renderer.
* **Lifetime.** `FrameRenderer::Create` creates the factory right after the device and before the swap chain. The asset cache, created before the renderer, attaches to it once it exists and detaches before the renderer is destroyed; every other client is created after the renderer and destroyed before it.
* **Server.** The dedicated server links no renderer and no factory ([ADR-056](ADR-056-products-configurations-and-feature-flags.md)); its asset system loads only CPU data.
* **Tests.** The factory is an abstract interface. The renderer implements it on the device, and the unit tests of its clients use an in-memory implementation that records every call and lets a test refuse an upload or start a rebuild.

### Consequences

* Good, because the device, the copy queue and the descriptor heaps have one owner, and no component outside the renderer creates a Direct3D 12 object.
* Good, because a device rebuild walks one table of resources and keeps every heap index, so handles and instance data survive it unchanged.
* Good, because the asset cache can mount packs and read entries before the device exists, which resolves the start-up order.
* Good, because the clients of the factory can be tested without a GPU.
* Bad, because every copy-queue upload pays one copy into the factory's staging memory and waits at least one frame for its submission.
* Bad, because every client must handle a refused upload and fill its resources again after `DeviceRebuilt`.
* Neutral, because the factory keeps a description of every live resource, a few dozen bytes each, and never its contents.
* It forecloses components that create Direct3D 12 resources themselves, a second device, a copy queue owned by a client, CPU shadow copies held for the rebuild, and a resource that changes its size in place: a new size is a new resource.

### Confirmation

* A boundary check fails when a source outside the renderer project includes `d3d12.h`; the clients see only the factory's interface and `dxgiformat.h`.
* Unit tests of the asset cache with the in-memory factory: CPU loads before the cache is attached and uploads after it; a refused upload is repeated on a later frame; a call refused during a rebuild is repeated after `DeviceRebuilt`.
* The device-removal test of ADR-008 recreates every factory resource at its heap index, the asset cache, the glyph cache and the VFX runtime fill theirs again, and a golden image taken after the rebuild equals the one taken before it.
* A fence test, with the debug layer and GPU-based validation on, releases resources every frame and finds no use of a released resource and no reuse of a descriptor slot within the frames in flight.
* A stress test creates, uploads and releases resources from eight threads under the debug layer and produces no message.

## Pros and Cons of the Options

### A renderer-owned factory interface

* Good, because creation, upload, release and rebuild follow one set of rules for every client.
* Good, because the heap indices stay stable across a rebuild.
* Bad, because the factory's staging memory is shared by every client and needs a budget.

### The device and a copy queue handed to every component

* Good, because each component uploads without an intermediary.
* Bad, because every component then manages its own fences, descriptors, deferred release and rebuild, and a mistake in any of them is a use-after-free on the GPU.
* Bad, because the rebuild of ADR-008 would have to know every component's resources, and the asset cache still could not exist before the device.

### Every creation and upload served by the frame thread

* Good, because only one thread ever touches the device.
* Bad, because loaders would wait for the frame thread, which the asset pipeline forbids, and creation would cost frame time.

## More Information

* Related: [ADR-001](ADR-001-direct3d-12-renderer-on-the-agility-sdk.md) (the device), [ADR-006](ADR-006-root-signature-and-bindless-resource-access.md) (the persistent descriptor region), [ADR-007](ADR-007-barrier-model.md) (queue handover), [ADR-008](ADR-008-gpu-memory-uploads-and-device-removal.md) (memory model and rebuild), [ADR-010](ADR-010-gpu-particles-and-shatter-debris.md) (particle and shard buffers), [ADR-011](ADR-011-world-surface-rendering.md) (terrain and sea buffers), [ADR-034](ADR-034-text-rendering-and-glyph-cache.md) (glyph pages).
* Components: the [renderer](../architecture/components/renderer/README.md) declares the interface (§2, RND-001, RND-009, RND-035); the [asset pipeline](../architecture/components/asset_pipeline_and_resources.md) attaches its cache to it (AST-002, AST-116, AST-119, AST-122); the platform layer's start-up order places the attachment after the device (PLT-038 steps 11 and 13); the UI framework creates its glyph pages and the [animation and VFX](../architecture/components/animation_and_vfx/README.md) runtime its buffers through it (VFX-003).
* Source: Direct3D 12 command lists are not free-threaded, several can be recorded at once, and any thread may submit to any command queue, whose runtime serialises the submissions in order (https://learn.microsoft.com/windows/win32/direct3d12/design-philosophy-of-command-queues-and-command-lists, checked 2026-10-18).
* **Amendment 2026-10-18 (owner decision O55).** Since O55 the first playable of the "Confirmed when" line is the MVP's M3 build ([MVP scope](../implementation/mvp_scope.md) §3, §5.3), not the skirmish of O20, and the evidence is gathered on that build: the first textured draw is that build's, and the forced device removal of ADR-008 runs during a session in a world in place of a match.
* **Amendment 2026-10-19 (owner decision O60, ADR-090).** The programs are `Species.exe` and `SpeciesServer.exe` and the content build tool `SpeciesContentBuild.exe`, and engine code is in namespace `Neuron`; this record's names follow.
* **Amendment 2026-10-23 (M3 task T16, owner decision O104, [ADR-174](ADR-174-the-first-playables-confirmations-at-m3.md)).** M3's evidence:
  - `Build\CheckProjectFiles.py` rule 14 refuses `d3d12.h` outside the renderer project, with a fixture;
  - `GpuResourceFactoryTests` check the descriptions, uploads from several threads, the release after the frames in flight, and a rebuild that keeps ids and heap indices;
  - every texture the client draws is created through the factory (RND-009).

  The forced removal of ADR-008 moves to the owner's T16 run: Ctrl+Shift+F11 once in a battle of the `--gpu-validation` session, after which the picture comes back and the session continues. Until the golden images exist (O104), the owner's eye stands in for the golden image before and after. That run confirms the record.
