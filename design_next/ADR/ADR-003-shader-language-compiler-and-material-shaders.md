---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Shader language, compiler and material shader set

## Context and Problem Statement

The Species look is produced by a small set of shading techniques: unlit vertex colour; one texture modulated by vertex colour; two textures with an explicit combine mode; lighting by two directional lights with no ambient term (light colours may exceed 1.0, up to 2.0 per channel, which gives the over-bright facets), with a specular highlight where a material declares one; a lit, textured variant with a wire-grid term; per-pixel linear fog towards the level's fog colour; additive glow; and multiplicative darkening. Terrain, water, sprite cards, particles, UI, glyphs and full-screen passes add their own shaders.

Species needs these shaders written once, compiled as part of every build, debuggable in PIX, and used without a compiler in the client. The repository's engineering rules fix the mechanism: HLSL compiled at build time by an `FXCompile` item of the `.vcxproj` with the Windows SDK's compiler into byte arrays that are compiled into the executable; no shader file beside the executable, no shader loaded from disk at run time, no run-time compilation and no redistributable shader compiler (`AGENTS.md` §2, R14). The set must be small enough that every combination the renderer uses is a catalogued pipeline state (ADR-005), and every shader must run on the minimum capability of ADR-002.

The problem is which shader language and compiler Species uses, when shaders are compiled, and how material variety is expressed.

## Decision Drivers

- One shader source per technique, with every variant known at build time (no hidden permutations).
- Shader Model 6.6 profiles (ADR-002) and directly indexed descriptor heaps (ADR-006).
- Shaders compiled by the build itself, with the compiler of the pinned Windows SDK ([ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)), and embedded in the executable (`AGENTS.md` §2).
- PIX debugging with source-level information, without shipping that information.
- No shader compiler in the client; nothing compiled or loaded from disk at run time.
- A small, closed material set that reproduces the look exactly.
- An easy step to the next HLSL language version.

## Considered Options

1. **HLSL 2021 compiled at build time by `FXCompile` with the Windows SDK's `dxc` into byte arrays embedded in the executable**, with a fixed material shader set and one source file per declared permutation.
2. HLSL compiled **at run time** in the client.
3. A **node-based material system** that generates a shader per asset.
4. One **über-shader** that branches on material flags at run time.

## Decision Outcome

Chosen option: **1, HLSL 2021 compiled by `FXCompile` and embedded in the executable, fixed material set**, because it is the mechanism `AGENTS.md` fixes and it keeps every shader variant known and compiled before it reaches a player, at no run-time cost.

- **Compiler.** The `dxc` of the pinned Windows SDK (10.0.26100.9457, [ADR-051](ADR-051-windows-toolchain-and-compiler-policy.md)), driven by `FXCompile` items in the project that owns the shaders; Visual Studio uses `dxc` when a Shader Model 6 profile is selected. Every compile passes `-HV 2021` explicitly, targets a `*_6_6` profile and treats warnings as errors. `-enable-16bit-types` is passed only for optional fast paths behind a capability check (ADR-002), and a profile above 6.6 is used only where the SDK's `dxc` supports it. The `FXCompile` settings are identical in Debug and Release (`AGENTS.md` §3).
- **Files.** Sources live in `<Lib>/Shader/`, one file per shader and stage, named `<Shader><Stage>.hlsl` (`VS` vertex, `PS` pixel, `CS` compute). Each compiles to `<Lib>/CompiledShader/<Shader><Stage>.h`, a header declaring the byte array `g_<Shader><Stage>`. The compiled headers are build output, never edited or committed; the one `.cpp` that binds the pipeline states includes them and nothing else does.
- **Output.** The bytecode reaches the GPU from those byte arrays and nothing else: no shader in a pack ([ADR-058](ADR-058-pack-files-and-mount-policy.md)), no `.cso` beside the executable, no run-time compilation, no `dxcompiler.dll` and no `d3dcompiler_47.dll` dependency. A shader change is a rebuild. Debug information goes to shader PDB files that each build keeps for PIX; the embedded bytecode carries no debug information.
- **Language hygiene.** Shaders are written to compile under `-HV 202x` as well: no effects syntax, no `interface`, no `uniform`.
- **Shared layouts.** Constant-buffer and root-constant layouts are declared once in a header that both C++ and HLSL include, so the CPU and GPU views of every structure cannot drift.
- **Material shader set.** The world uses exactly these material shaders:
  1. unlit, vertex colour;
  2. one texture, modulated by vertex colour;
  3. two textures with an explicit combine mode, chosen per material from a closed list that the renderer component defines;
  4. lit by two directional lights with no ambient term, light colours up to 2.0 per channel, and a specular term with a per-material specular colour and shininess;
  5. lit, one texture, with a wire-grid term (the terrain's grid texture or a barycentric edge mask).
  The other shaders are: terrain, water, sky, sprite cards and particles, shatter debris, UI, glyphs, and the full-screen passes (fade, screen transition, resolve, tone mapping).
- **Lighting and fog inputs.** Light directions are given in world space in the per-frame constants, independent of the camera. The specular term uses the half vector between the light direction and the view direction, with the viewer at an infinite distance along the camera's view axis; its specular colour and shininess are material parameters, and a material without a highlight has specular colour 0. The terrain grid material and the special lighting rig use it with the values of the renderer component. Fog is linear, evaluated per pixel on view-space distance, towards the level's fog colour.
- **Flat shading.** Facet colour and facet normal are baked per triangle by the mesh cooker (ADR-020), so no shader depends on a provoking-vertex convention.
- **Wire-grid term and edge mask.** The terrain shader's grid term comes from the stylised grid texture (the default) or from a barycentric edge term, selected by a declared permutation (ADR-011, DEC-326). Barycentric edge terms, the terrain option and the shatter-debris edge mask (ADR-010), are computed in the pixel shader from barycentric coordinates that the cooker writes as a vertex attribute (meshes are already split per triangle for flat colour). `SV_Barycentrics` may be used only as an optional path behind `D3D12_FEATURE_DATA_D3D12_OPTIONS3.BarycentricsSupported`.
- **Permutations.** Each shader declares its permutation axes (for example fog on or off, alpha cut-out on or off). Because one `FXCompile` item compiles one file, each permutation is its own short source file, `<Shader><Permutation><Stage>.hlsl`, which sets the permutation's defines and includes the shader's shared source; it compiles to its own byte array. A permutation that has no file does not exist. Each compiled permutation is referenced by at least one pipeline state in the catalogue (ADR-005); a check rejects unreferenced ones.

### Consequences

- Good, because every shader variant that can run on a player's machine has been compiled, validated and tested in CI.
- Good, because the client needs no shader compiler, loads no shader file and spends no time compiling at run time.
- Bad, because embedded bytecode enlarges the executable and stays in memory after the pipeline states are created; at Species' small shader set this costs little.
- Good, because the closed material set makes the pipeline state catalogue small and reviewable.
- Good, because the edge mask does not depend on an optional GPU capability.
- Bad, because a new material technique needs a shader change, a catalogue entry and a build, not just data.
- Bad, because barycentric attributes add bytes per vertex on the meshes that use the edge mask.

### Confirmation

- The CI build compiles every shader through `FXCompile` with warnings as errors ([ADR-070](ADR-070-continuous-integration.md)); a shader that the SDK's `dxc` cannot compile to its `*_6_6` profile fails the build.
- `Build\CheckProjectFiles.py` checks the shader file names and folders, and that the `FXCompile` settings are identical in Debug and Release.
- A check fails when a compiled permutation is not referenced by the pipeline state catalogue, or a catalogue entry names a permutation that does not exist.
- Before a release, the shipping builder checks that no `.cso`, `dxcompiler.dll` or `d3dcompiler_47.dll` is in the install directory.
- Golden-image tests of the renderer component cover each material shader, including over-bright lighting (light colour 2.0) and the fog ramp.

## Pros and Cons of the Options

### 1. HLSL 2021 by `FXCompile`, embedded, fixed material set

- Good, because shaders are compiled by the build that compiles the code, with no separate tool step and no file to install.
- Good, because PIX gets full source-level debugging from the kept PDBs.
- Bad, because adding a technique needs an engineering change, and the shader compiler version follows the Windows SDK.

### 2. Run-time compilation in the client

- Good, because new variants can appear without a build.
- Bad, because the client ships a compiler, start-up or first use stalls on compilation, a variant can fail on a player's machine that CI never compiled, and `AGENTS.md` §2 excludes it.

### 3. Node-based material system

- Good, because artists can create new looks without engineers.
- Bad, because the look is a fixed technique set; open-ended materials invite drift from it and multiply pipeline states.
- Bad, because generated shaders are harder to review and to keep within the catalogue.

### 4. Über-shader with run-time branching

- Good, because there is only one shader to maintain and few pipeline states.
- Bad, because every pixel pays for the most expensive branch's register use, and divergent branches in crowds of mixed materials waste throughput.
- Bad, because blend and depth state still differ per pass, so pipeline states do not shrink as much as the shader count does.

## More Information

- Rules: `AGENTS.md` §2 (shader folders, `FXCompile`, shaders compiled into the executable) and R14 (no redistributable shader compiler).
- Technology baseline item 4.5 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: Visual Studio compiles HLSL into byte arrays in header files and uses `dxc` when Shader Model 6 is selected (https://learn.microsoft.com/windows/win32/direct3dhlsl/dx-graphics-hlsl-part1 · https://learn.microsoft.com/cpp/build/reference/hlsl-property-pages); Shader Model 6.6 was released in April 2021 (https://devblogs.microsoft.com/directx/hlsl-shader-model-6-6/).
- Related: ADR-002 (Shader Model 6.6 profiles), ADR-005 (pipeline state catalogue), ADR-006 (root signature), ADR-020 (flat shading and team tint).
- The renderer component (`architecture/components/renderer/`) specifies each shader's inputs, outputs and permutation axes, and the closed list of combine modes.
- **Amendment 2026-10-17 (AGENTS.md, O21).** Shaders are compiled at build time by `FXCompile` with the Windows SDK's `dxc` (Shader Model 6.6) into `<Lib>/CompiledShader/<Shader><Stage>.h` byte arrays embedded in the executable, from sources in `<Lib>/Shader/<Shader><Stage>.hlsl`; one source file per permutation. Shader blobs in packs, `dxcompiler.dll` and the pinned redistributable compiler are removed. The `CS` stage suffix for compute shaders and the shared include files of permutations are proposed as `AGENTS.md` extensions.
- **Amendment 2026-10-17.** The lit material has a specular term (RND-233, RND-270), used by the terrain grid material and the special lighting rig (RND-229, RND-266); the terrain grid term has two sources, the grid texture by default and the barycentric edge term as a permutation (RND-135, RND-264, RND-266), matching ADR-011 and DEC-326.
