---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# ADR-0001 Direct3D 12 renderer on the Agility SDK

## Context and Problem Statement

Species is a Windows 11 client that draws crowds of hundreds to thousands of small autonomous figures, low-poly flat-shaded buildings and creatures, additive glow on a black world, multiplicative darkening layers, and sprite cards that scale with distance. The vision brief fixes the look (O9) and fixes the client platform as Windows, C++23 and Direct3D 12 (§5).

At crowd scale, the cost of rendering is dominated by how many separate submissions the CPU makes per frame and by how much hidden state the driver has to validate. Species therefore needs an explicit renderer: one that owns every piece of GPU state, submits work in large batches, and makes the cost of each pass visible. The renderer must also use Direct3D 12 features (Shader Model 6.6 dynamic resources, Enhanced Barriers) that are delivered faster than the operating system is serviced, while the dedicated server must build and run with no graphics at all.

The problem is which Direct3D 12 runtime the client is built against and how that runtime reaches players' machines.

## Decision Drivers

- The client platform is Windows 11 x64, minimum 24H2 (build 10.0.26100), and the graphics API is Direct3D 12 (vision brief §5).
- Crowd-scale submission: instancing and large batches, not one submission per figure.
- Every pipeline state is explicit and immutable; nothing depends on driver defaults.
- Access to current runtime features (Shader Model 6.6 dynamic resources, Enhanced Barriers, current debug-layer fixes) on every supported OS build.
- Reproducible behaviour: the same runtime version in development, CI and players' installs.
- First-class tooling: PIX, the debug layer, GPU-based validation, DRED.
- The headless dedicated server links no graphics library.

## Considered Options

1. Direct3D 12 through the **Agility SDK**, with `D3D12Core.dll` installed beside the executable.
2. Direct3D 12 through the **runtime that the operating system provides**, with no runtime in the install directory.
3. Direct3D 11.
4. Vulkan.

## Decision Outcome

Chosen option: **1, Direct3D 12 through the Agility SDK**, because it is the only option that gives every supported OS build the same, current Direct3D 12 runtime while staying inside the Direct3D 12 constraint of the vision brief.

- The client ships **Agility SDK 1.619.6** (NuGet `Microsoft.Direct3D.D3D12`, D3D12SDKVersion **619**). `species.exe` exports `D3D12SDKVersion = 619` and `D3D12SDKPath = ".\\D3D12\\"`, and the install directory carries `D3D12\D3D12Core.dll`. Agility SDK headers are included before Windows SDK headers.
- No preview Agility SDK is ever distributed, and `D3D12SDKLayers.dll` is never distributed. The debug layer and GPU-based validation run in development and in a CI test configuration only.
- The SDK version is pinned in exactly one place (the package version), and the exported constant is generated from it.
- The renderer is an explicit Direct3D 12 renderer: it owns all GPU state, builds each frame from a frame graph (ADR-0004), draws only through catalogued pipeline state objects (ADR-0005), binds resources through one bindless root signature (ADR-0006), and expresses synchronisation through the barrier model of ADR-0007. Resource upload, residency and device-removal handling follow ADR-0008.
- The renderer is a client-only module. The dedicated server target does not link `d3d12.lib`, `dxgi.lib` or any renderer code.

### Consequences

- Good, because every supported OS build runs the same runtime version that development and CI test against.
- Good, because Shader Model 6.6 dynamic resources, Enhanced Barriers and the current debug-layer fixes are available without waiting for OS servicing.
- Good, because an explicit renderer makes the per-pass cost visible and lets crowds be drawn as a few instanced batches.
- Good, because the renderer's absence from the server is enforced by the build graph, not by convention.
- Bad, because the install directory carries an extra runtime DLL, and every Agility SDK upgrade is a deliberate change with its own test pass.
- Bad, because the explicit API puts resource lifetime, synchronisation and memory management on the engine, which ADR-0004, ADR-0007 and ADR-0008 have to cover.
- Neutral, because if the OS carries a newer `D3D12Core.dll`, the OS copy loads instead; the renderer must therefore treat capabilities as queried facts, never as assumptions tied to the bundled version.

### Confirmation

- A build check fails when the exported `D3D12SDKVersion` differs from the minor version of the packaged `D3D12Core.dll`.
- A start-up test on the minimum OS build (24H2, 26100) creates the device through the Agility runtime and logs the loaded runtime version.
- The CI test configuration runs the golden-image and frame-graph tests of the renderer component with the debug layer and GPU-based validation enabled, and fails on any validation message.
- The dedicated-server link map is checked in CI for the absence of `d3d12`, `dxgi` and renderer symbols.

## Pros and Cons of the Options

### 1. Direct3D 12 through the Agility SDK

- Good, because the runtime version is chosen by Species, not by the player's OS servicing state.
- Good, because the newest released features (Shader Model 6.9, revised view-creation APIs, current Enhanced-Barrier fixes) are available to every supported build.
- Good, because every OS allowed by the baseline (Windows 10 1909 or later, Windows Server 2022 or later) supports the Agility SDK.
- Bad, because `D3D12CreateDevice` fails if the exported version does not match the bundled runtime, so packaging errors surface as start-up failures (mitigated by the build check).
- Bad, because whether the Agility SDK passes MSIX certification is not verified (it matters only if the MSIX store path is chosen).

### 2. Direct3D 12 through the OS runtime only

- Good, because nothing extra is installed.
- Bad, because the available feature set varies with each player's OS servicing state, so the engine would need more fallback paths.
- Bad, because debug-layer and runtime fixes arrive only with OS updates.

### 3. Direct3D 11

- Good, because the driver manages residency and hazards, so the renderer is simpler to write.
- Bad, because it conflicts with the vision brief's Direct3D 12 constraint.
- Bad, because there are no Shader Model 6.6 dynamic resources and no explicit barriers, and per-draw driver overhead stays high at crowd scale.

### 4. Vulkan

- Good, because it is explicit and portable.
- Bad, because it conflicts with the vision brief's Direct3D 12 constraint, and the client is Windows-only, so portability brings no benefit.
- Bad, because the baseline's tooling (PIX, DRED, the Agility SDK debug layer) and the Windows HDR and flip-model presentation path are Direct3D-native.

## More Information

- Technology baseline items 4.1 and 4.7 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://devblogs.microsoft.com/directx/directx12agility/ · https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/ · https://github.com/microsoft/DirectX-Headers/releases.
- Related: ADR-0002 (minimum GPU capability), ADR-0004 (frame graph), ADR-0005 (pipeline state catalogue), ADR-0006 (root signature), ADR-0007 (barriers), ADR-0008 (memory, uploads, device removal), ADR-0012 (swap chain).
- The renderer component (`architecture/components/renderer/`) specifies the frame, the pass table and each sub-renderer on top of this decision.
- Re-check the Agility SDK version at every milestone; an upgrade is a new revision of this ADR.
