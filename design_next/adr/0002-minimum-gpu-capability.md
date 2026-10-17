---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Minimum GPU capability: feature level 12_1, Shader Model 6.6, Resource Binding Tier 3 and wave operations

## Context and Problem Statement

The Species look (flat-shaded low-poly meshes, vertex colour, additive glow on black, multiplicative darkening layers, distance-scaled sprite cards) is cheap per pixel. What it needs from the GPU is throughput for crowds: instanced draws, compute passes for culling and particles, and indirect submission. The renderer's binding model (ADR-0006) indexes descriptor heaps directly from shaders, and the particle and culling passes use wave intrinsics for compaction.

Species wants the widest hardware reach that still supports those techniques, a single well-defined floor that the test matrix covers, and a clear message, at start-up, for a player whose GPU falls below it. The hardware reach of feature level 12_2 ("DirectX 12 Ultimate") could not be verified from a primary source.

The problem is which device feature level, Shader Model and capability checks the client requires.

## Decision Drivers

- Bindless resource access through Shader Model 6.6 dynamic resources (ADR-0006), which requires Resource Binding Tier 3 together with Shader Model 6.6.
- Wave intrinsics for stream compaction in culling and particle passes.
- Crowd culling with compute and `ExecuteIndirect`, which works at feature level 12_1.
- No dependence on raytracing, mesh shaders, sampler feedback or variable-rate shading: the look needs none of them.
- Widest reach among GPUs that run the supported OS (Windows 11 24H2 or later).
- One floor that development, CI and QA hardware all meet, and that a start-up check can test exactly.

## Considered Options

1. **Feature level 12_1** plus capability checks for **Shader Model 6.6**, **Resource Binding Tier 3** and **WaveOps**.
2. **Feature level 12_0** plus the same three capability checks.
3. **Feature level 12_2** (DirectX 12 Ultimate).
4. Feature level 12_1 with **Shader Model 6.9** (the current released Shader Model) as the minimum.

## Decision Outcome

Proposed option: **1**. **Default while Proposed:** the team builds and tests against feature level 12_1 with the three capability checks, because it is the lowest floor that gives the binding model and the crowd techniques everything they need, and it matches the technology baseline.

- The device is created at `D3D_FEATURE_LEVEL_12_1` on the adapter that `IDXGIFactory6::EnumAdapterByGpuPreference` returns first for `DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE`; an adapter that fails any check below is skipped in favour of the next one.
- Start-up checks, all of which must pass:
  - the highest supported Shader Model is at least **6.6**, queried with `D3D12_FEATURE_SHADER_MODEL` by looping downwards from the highest model the runtime knows (the query rejects unknown models);
  - `D3D12_FEATURE_DATA_D3D12_OPTIONS.ResourceBindingTier` is **Tier 3**;
  - `D3D12_FEATURE_DATA_D3D12_OPTIONS1.WaveOps` is `TRUE`.
- If no adapter passes, the client shows a localised message that names the missing capability and the adapter, writes it to the log, and exits without creating a swap chain.
- Every shader compiles to `*_6_6` profiles (ADR-0003). A feature of Shader Model 6.7 or later, 16-bit shader types, mesh or amplification shaders, or variable-rate shading may be used only as an optional fast path behind its own capability check, with a 6.6 path that produces the same image, and only with an ADR.
- The renderer does not depend, in any required path, on a capability that feature level 12_1 adds over 12_0 (conservative rasterisation, rasteriser-ordered views). This keeps option 2 open as a compatible relaxation if hardware data later shows players blocked by the floor.
- Optional capabilities the renderer queries at start-up for other decisions: `EnhancedBarriersSupported` (ADR-0007), tearing support (ADR-0012), HDR output (ADR-0014).

### Consequences

- Good, because the binding model, the compute culling and the GPU particle system all rest on guaranteed capabilities, with no fallback binding path to maintain.
- Good, because GPUs without raytracing or mesh shaders remain supported, which suits a look that uses neither.
- Good, because the start-up check is exact, so an unsupported GPU fails with a message instead of a rendering fault.
- Bad, because GPUs that support feature level 12_0 but not 12_1, and still meet the three checks, are excluded while the default stands.
- Bad, because optional fast paths (mesh shaders, VRS) cost a second implementation each; the default is not to build them.

### Confirmation

- A unit test of the capability probe feeds it recorded `CheckFeatureSupport` results (pass, each single failure, unknown Shader Model) and checks the adapter choice and the message.
- The CI shader build rejects any profile other than `*_6_6`, unless the shader is registered as an optional fast path with its ADR number.
- QA's hardware matrix includes at least one GPU at exactly feature level 12_1 that meets the checks, and the golden-image tests run on it.

## Pros and Cons of the Options

### 1. Feature level 12_1 plus Shader Model 6.6, Resource Binding Tier 3 and WaveOps

- Good, because it covers every technique Species needs and nothing it does not.
- Good, because it is the floor of the technology baseline, so the test matrix and the documentation agree.
- Neutral, because 12_1's own additions (conservative rasterisation, rasteriser-ordered views) are not needed by any required path.
- Bad, because it excludes 12_0-only GPUs that would otherwise qualify.

### 2. Feature level 12_0 plus the same checks

- Good, because it admits the largest set of GPUs that can run the binding model.
- Neutral, because the three capability checks, not the feature level, carry the real requirements.
- Bad, because the number of 12_0-only GPUs that pass the checks on Windows 11 24H2 drivers is unknown, so the extra reach cannot be weighed against the extra test hardware.

### 3. Feature level 12_2

- Good, because mesh shaders, VRS and sampler feedback would be available everywhere.
- Bad, because the look needs none of them, and every GPU below 12_2 would be excluded; the size of that cost is not verified.

### 4. Shader Model 6.9 minimum

- Good, because 16-bit and 64-bit shader operations and wave operations become guaranteed.
- Bad, because Shader Model 6.9 support is limited to the newest GPU generations, which would exclude most of the installed base for no visual gain.

## More Information

- Technology baseline items 4.2 and 4.3 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://learn.microsoft.com/windows/win32/direct3d12/hardware-feature-levels · https://raw.githubusercontent.com/microsoft/DirectX-Specs/master/d3d/HLSL_SM_6_6_DynamicResources.md · https://learn.microsoft.com/windows/win32/api/d3d12/ns-d3d12-d3d12_feature_data_shader_model.
- The hardware reach of feature level 12_2, and of 12_0-only GPUs that pass the checks, is not verified. Revisit this ADR when hardware data is available.
- Related: ADR-0003 (shader profiles), ADR-0006 (bindless binding model), ADR-0009 (crowd culling with `ExecuteIndirect`), ADR-0010 (GPU particles).
