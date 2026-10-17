---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# ADR-0003 Shader language, compiler and material shader set

## Context and Problem Statement

The Species look is produced by a small set of shading techniques: unlit vertex colour; one texture modulated by vertex colour; two textures with an explicit combine mode; lighting by two directional lights with no ambient term (light colours may exceed 1.0, up to 2.0 per channel, which gives the over-bright facets); a lit, textured variant with a wire-grid edge mask; per-pixel linear fog towards the level's fog colour; additive glow; and multiplicative darkening. Terrain, water, sprite cards, particles, UI, glyphs and full-screen passes add their own shaders.

Species needs these shaders written once, compiled reproducibly, validated in CI, debuggable in PIX, and loaded without a compiler in the client. The set must be small enough that every combination the renderer uses is a catalogued pipeline state (ADR-0005), and every shader must run on the minimum capability of ADR-0002.

The problem is which shader language and compiler Species uses, when shaders are compiled, and how material variety is expressed.

## Decision Drivers

- One shader source per technique, with every variant known at build time (no hidden permutations).
- Shader Model 6.6 profiles (ADR-0002) and directly indexed descriptor heaps (ADR-0006).
- Reproducible output: the same compiler version in development, CI and release builds.
- PIX debugging with source-level information, without shipping that information.
- No shader compiler in the client; nothing compiled at run time.
- A small, closed material set that reproduces the look exactly.
- An easy step to the next HLSL language version.

## Considered Options

1. **HLSL 2021 compiled offline with a pinned DXC to DXIL**, with a fixed material shader set and a declared permutation list.
2. HLSL compiled **at run time** in the client with DXC.
3. A **node-based material system** that generates a shader per asset.
4. One **über-shader** that branches on material flags at run time.

## Decision Outcome

Chosen option: **1, HLSL 2021, offline DXC, fixed material set**, because it is the only option that keeps every shader variant known, reproducible and validated before it reaches a player, at no run-time cost.

- **Compiler.** DXC **v1.9.2609**, pinned, invoked by the asset pipeline. Every compile passes `-HV 2021` explicitly, targets a `*_6_6` profile, and treats warnings as errors. `-enable-16bit-types` is passed only for optional fast paths behind a capability check (ADR-0002).
- **Output.** The client loads DXIL blobs only. Debug information and reflection data go to PDB files that each build keeps for PIX; distributed blobs carry neither. `dxcompiler.dll` is never distributed.
- **Language hygiene.** Shaders also compile under `-HV 202x`: no effects syntax, no `interface`, no `uniform`. CI compiles every shader in both modes.
- **Shared layouts.** Constant-buffer and root-constant layouts are declared once in a header that both C++ and HLSL include, so the CPU and GPU views of every structure cannot drift.
- **Material shader set.** The world uses exactly these material shaders:
  1. unlit, vertex colour;
  2. one texture, modulated by vertex colour;
  3. two textures with an explicit combine mode, chosen per material from a closed list that the renderer component defines;
  4. lit by two directional lights with no ambient term, light colours up to 2.0 per channel;
  5. lit, one texture, with a wire-grid edge mask.
  The other shaders are: terrain, water, sky, sprite cards and particles, shatter debris, UI, glyphs, and the full-screen passes (fade, screen transition, resolve, tone mapping).
- **Lighting and fog inputs.** Light directions are given in world space in the per-frame constants, never derived from the previous frame's camera. Fog is linear, evaluated per pixel on view-space distance, towards the level's fog colour.
- **Flat shading.** Facet colour and facet normal are baked per triangle by the mesh cooker (ADR-0020), so no shader depends on a provoking-vertex convention.
- **Edge mask.** The wire-grid edge term is computed in the pixel shader from barycentric coordinates that the cooker writes as a vertex attribute (meshes are already split per triangle for flat colour). `SV_Barycentrics` may be used only as an optional path behind `D3D12_FEATURE_DATA_D3D12_OPTIONS3.BarycentricsSupported`.
- **Permutations.** Each shader declares its permutation axes (for example fog on or off, alpha cut-out on or off) in a list that the build expands. A permutation that is not declared does not exist. Each compiled permutation is referenced by at least one pipeline state in the catalogue (ADR-0005); the build rejects unreferenced ones.

### Consequences

- Good, because every shader variant that can run on a player's machine has been compiled, validated and tested in CI.
- Good, because the client needs no shader compiler and spends no time compiling at run time.
- Good, because the closed material set makes the pipeline state catalogue small and reviewable.
- Good, because the edge mask does not depend on an optional GPU capability.
- Bad, because a new material technique needs a shader change, a catalogue entry and a build, not just data.
- Bad, because barycentric attributes add bytes per vertex on the meshes that use the edge mask.

### Confirmation

- CI compiles every shader in both `-HV 2021` and `-HV 202x` modes and fails on any warning.
- A build check fails when a compiled permutation is not referenced by the pipeline state catalogue, or a catalogue entry names a permutation that does not exist.
- A build check fails when a distributed blob contains debug or reflection parts, or when `dxcompiler.dll` appears in the install directory.
- Golden-image tests of the renderer component cover each material shader, including over-bright lighting (light colour 2.0) and the fog ramp.

## Pros and Cons of the Options

### 1. HLSL 2021, offline DXC, fixed material set

- Good, because output is reproducible from a pinned compiler and a declared variant list.
- Good, because PIX gets full source-level debugging from the kept PDBs.
- Bad, because adding a technique needs an engineering change.

### 2. Run-time compilation in the client

- Good, because new variants can appear without a build.
- Bad, because the client ships a compiler, start-up or first use stalls on compilation, and a variant can fail on a player's machine that CI never compiled.

### 3. Node-based material system

- Good, because artists can create new looks without engineers.
- Bad, because the look is a fixed technique set; open-ended materials invite drift from it and multiply pipeline states.
- Bad, because generated shaders are harder to review and to keep within the catalogue.

### 4. Über-shader with run-time branching

- Good, because there is only one shader to maintain and few pipeline states.
- Bad, because every pixel pays for the most expensive branch's register use, and divergent branches in crowds of mixed materials waste throughput.
- Bad, because blend and depth state still differ per pass, so pipeline states do not shrink as much as the shader count does.

## More Information

- Technology baseline item 4.5 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://github.com/microsoft/DirectXShaderCompiler/releases · `ReleaseNotes.md` of https://github.com/microsoft/DirectXShaderCompiler.
- Related: ADR-0002 (Shader Model 6.6 profiles), ADR-0005 (pipeline state catalogue), ADR-0006 (root signature), ADR-0020 (flat shading and team tint).
- The renderer component (`architecture/components/renderer/`) specifies each shader's inputs, outputs and permutation axes, and the closed list of combine modes.
