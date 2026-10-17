---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# ADR-0008 GPU memory, uploads and device-removal handling

## Context and Problem Statement

Species has two kinds of GPU data. **Static** data (meshes, terrain buffers, textures, glyph atlases once rasterised) is created when a map or the front end loads and does not change. **Dynamic** data (per-frame constants, per-instance transforms and colours written by the CPU, UI vertices, sprite cards) is rewritten every frame. Assets are few and small (flat-shaded, vertex-coloured meshes; few textures), so everything a match needs fits in video memory.

On Direct3D 12 the engine chooses where every resource lives, how data reaches it, and when memory may be reused. The GPU can also be lost while the game runs (a driver update, a timeout, a removed adapter). Species needs loading that never stalls the frame, per-frame data with no allocation cost, and a way to survive device removal without ending a long session: the simulation never depends on presentation state, so presentation can be rebuilt while the game continues.

The problem is how GPU memory is organised, how data is uploaded, and what happens when the device is removed.

## Decision Drivers

- No frame stalls on an upload or an allocation.
- Static data is read from the fastest GPU memory.
- Per-frame data costs a pointer bump, with no per-draw allocation.
- Safe reuse of memory with several frames in flight (ADR-0012).
- Device removal is survivable without losing simulation state, and is diagnosable afterwards.
- No duplicate CPU copies of GPU resources only for rebuilding.

## Considered Options

1. **Static resources in default heaps uploaded through the copy queue, a per-frame upload ring for dynamic data, and full device recreation from the asset system on removal.**
2. The same memory model, but **device removal ends the session** with a crash report.
3. The same memory model, with **CPU shadow copies** of every GPU resource kept for rebuilding.
4. **Dynamic and static data drawn directly from upload heaps.**

## Decision Outcome

Chosen option: **1**, because it gives the fastest memory for static data, the cheapest path for per-frame data, and a rebuild that reuses the asset system instead of duplicating memory.

- **Static resources** live in default heaps. The asset system writes their contents into staging upload buffers, and the **copy queue** copies them into place. Each upload batch signals a fence; an asset becomes visible to the renderer only after its fence has completed. Uploads never block the direct queue.
- **Dynamic data** is written into a **persistently mapped upload ring buffer**, one segment per frame in flight. Allocation is a linear bump with 256-byte alignment for constant-buffer views. A segment is reused only after the fence of the frame that last used it has completed. If a frame overflows its segment, the frame takes a one-off overflow buffer, the overflow is logged, and the ring is enlarged at the next reconfiguration.
- **Transient render targets** are placed resources in heaps that the frame graph aliases (ADR-0004).
- **Long-lived allocations** go through one engine allocator that pools placed resources by heap type and size class and records every allocation by asset id for diagnostics. Video-memory use is read from `IDXGIAdapter3::QueryVideoMemoryInfo` and shown in the development overlay; the baseline does not page resources in and out, because a match's assets fit in memory.
- **No CPU shadow copies.** Every GPU resource can be recreated from its source: static resources from the asset system's packages, dynamic resources by the next frame, and rasterised glyphs by the glyph cache (ADR-0034).
- **Device removal.** `DXGI_ERROR_DEVICE_REMOVED` and `DXGI_ERROR_DEVICE_RESET`, whether returned by `Present`, by command-list execution or found by a fence that will never complete, start a rebuild of presentation:
  1. read the removal reason and the DRED auto-breadcrumbs and page-fault data, and write them to the log and the crash-report attachments;
  2. release every device object;
  3. create a new device (applying the checks of ADR-0002), swap chain, root signatures and pipeline-state catalogue (from the pipeline library where valid);
  4. reload static GPU resources from the asset system, showing a loading indicator;
  5. resume presentation.
  The simulation keeps running during the rebuild, because presentation never owns simulation state. If recreation fails, or the device is removed again during the rebuild, the client reports the failure and exits.
- DRED auto-breadcrumbs and page-fault reporting are enabled in every build.
- Bundles and pre-recorded command lists are not used in the baseline; static geometry is drawn through instanced and indirect draws (ADR-0009).

### Consequences

- Good, because loading never stalls rendering, and static data is read at full GPU bandwidth.
- Good, because per-frame data needs no allocator calls and no locks.
- Good, because a driver reset during a long session costs a reload, not the session.
- Good, because every removal leaves a diagnosable record.
- Bad, because the asset system must be able to reload any GPU resource at any time, which constrains how it caches packages.
- Bad, because the rebuild is a rarely exercised path and needs its own tests.

### Confirmation

- A development command forces device removal (`ID3D12Device5::RemoveDevice`); a CI test triggers it during a running match and checks that presentation resumes, that the match continues, and that the log contains the DRED report.
- A fence-timing test checks that no upload-ring segment is written while the GPU may still read it.
- A soak test loads and unloads maps repeatedly and checks that the allocator returns to its baseline footprint.

## Pros and Cons of the Options

### 1. Default heaps, copy-queue uploads, upload ring, recreation from assets

- Good, because it uses each memory type for what it is best at.
- Good, because the rebuild needs no extra memory.
- Bad, because the rebuild depends on the asset system being reloadable.

### 2. End the session on device removal

- Good, because there is no rebuild code.
- Bad, because a driver update or timeout during a long session forces the player out, which a world that keeps running makes costly.

### 3. CPU shadow copies for rebuilding

- Good, because the rebuild does not touch the asset system.
- Bad, because every static resource is held twice, for an event that rarely happens.

### 4. Draw from upload heaps

- Good, because there are no copies and no copy queue.
- Bad, because GPU reads from upload memory are slower, which matters for terrain and large instance buffers read every frame.

## More Information

- Technology baseline items 4.1 and 11.1 (DRED in crash reports; see [the index](README.md#technology-baseline)).
- Related: ADR-0001 (explicit renderer), ADR-0004 (frame graph and transient aliasing), ADR-0005 (pipeline library), ADR-0012 (frames in flight and swap-chain resize), ADR-0034 (glyph cache).
- The asset pipeline component (`architecture/components/asset_pipeline_and_resources.md`) specifies how packages are read and reloaded; the pack-file decisions are in the content-pipeline ADRs (0050–0099).
