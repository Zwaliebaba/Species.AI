# Shaders

> Status: Engine baseline · Prefix: RND · ADRs: [ADR-002](../../../ADR/ADR-002-minimum-gpu-capability.md) · [ADR-003](../../../ADR/ADR-003-shader-language-compiler-and-material-shaders.md) · [ADR-005](../../../ADR/ADR-005-pipeline-state-catalogue.md) · [ADR-006](../../../ADR/ADR-006-root-signature-and-bindless-resource-access.md) · [ADR-009](../../../ADR/ADR-009-instanced-crowds-sprite-cards-and-shadows.md) · [ADR-010](../../../ADR/ADR-010-gpu-particles-and-shatter-debris.md) · [ADR-011](../../../ADR/ADR-011-world-surface-rendering.md) · [ADR-013](../../../ADR/ADR-013-working-colour-space-and-blending.md) · [ADR-014](../../../ADR/ADR-014-hdr-output.md) · [ADR-015](../../../ADR/ADR-015-scene-anti-aliasing.md) · [ADR-018](../../../ADR/ADR-018-texture-import-manifest.md) · [ADR-020](../../../ADR/ADR-020-flat-shading-and-team-tint.md) · Related: [Renderer](README.md) · [Frame graph and passes](frame_graph_and_passes.md) · [World renderers](world_renderers.md) · [View, lighting and fog](view_lighting_and_fog.md)

This document lists the HLSL shaders of the renderer, derived from the ordered pass table ([frame_graph_and_passes.md](frame_graph_and_passes.md) §4.3) and its catalogue entries (§3.4), with their inputs, outputs and permutation axes, the closed list of two-texture combine modes, and the team-tint function (ADR-003, ADR-020).

## 1 Responsibilities and boundaries

**RND-260** (MUST) The renderer uses exactly the shaders of §4.1. Each is written in HLSL 2021 and compiled at build time by an `FXCompile` item of the renderer project, with the `dxc` of the pinned Windows SDK (ADR-003, ADR-051), to a `*_6_6` profile with `-HV 2021`, row-major matrix packing (`-Zpr`) and warnings as errors, with the same settings in Debug and Release; every source also compiles under `-HV 202x`, which a check step of the build confirms (T1). The bytecode reaches the GPU only from the byte arrays that the build compiles into the executable: no shader file is installed or loaded from disk, and no shader is compiled at run time (`AGENTS.md` §2, ADR-003). Shaders reach resources only through the directly indexed descriptor heaps of ADR-006, wrapping any index that can differ within a wave in `NonUniformResourceIndex`.

**RND-261** (MUST) Each shader declares its permutation axes in the list below; a permutation that is not declared does not exist, and every compiled permutation is used by at least one catalogue entry (ADR-003, ADR-005). The sources live in the `Shader/` folder of the project that owns them (`AGENTS.md` §2): the renderer project for every shader of RND-263 except the four compute shaders of the animation and VFX runtime, which live with that runtime; one file per shader, stage and permutation, with the stages `VS`, `PS` and `CS`:

- the default permutation, every axis at its first listed value, is `<Shader><Stage>.hlsl` (`SkyVS.hlsl`, `SeaWavesUpdateCS.hlsl`);
- any other permutation is `<Shader><Permutation><Stage>.hlsl`, where `<Permutation>` joins, in the order the axes are listed, one part for each axis that is not at its first value: the axis name for an on-off axis (`FOG` on gives `Fog`, so `MeshLitFogPS.hlsl`), the axis name and the number for a numeric axis (`SAMPLES` = 4 gives `Samples4`), and the value for a named axis (`GRID_SOURCE` = `BARYCENTRIC` gives `Barycentric`, so `TerrainInvertedBarycentricPS.hlsl`), each in PascalCase;
- a permutation has a file only for the stages that its axes change (the stages named with each axis in RND-263); its other stages use the files of the default permutation;
- a permutation file defines the axis macros and includes the shader's shared source `<Shader>.hlsli`, which is never compiled on its own.

Each `.hlsl` compiles to the generated header `CompiledShader/<Shader><Permutation><Stage>.h`, which declares the byte array `g_<Shader><Permutation><Stage>` (`g_SkyVS`, `g_TerrainInvertedPS`, `g_MeshLitFogPS`, `g_SeaWavesUpdateCS`). The headers are build output, never edited or committed; the one `.cpp` that creates the pipeline-state catalogue includes them and nothing else does. Optional fast paths behind capability checks (ADR-002) are not part of the baseline.

## 2 Interfaces

**RND-262** (MUST) One shared header of the renderer project, `FrameConstants.h`, named for its primary type (`AGENTS.md` R7), which C++ includes and the shaders include, declares every constant and structured-buffer layout of the renderer (ADR-003), in `namespace Neuron`: the per-frame constants of README RND-014 (`FrameConstants`), the per-draw root constants of RND-015 (`DrawConstants`), the instance records (`MeshInstance`, `QuadInstance`, `SkyLayerInstance`, `WaveVertexPair`), the material parameters (`LitMaterial`: specular colour, shininess; `CombineParams`: the combine mode) and the fixed sampler indices of the closed sampler set of ADR-018: `SAMPLER_LINEAR_CLAMP` = 0, `SAMPLER_LINEAR_WRAP` = 1, `SAMPLER_POINT_CLAMP` = 2 and `SAMPLER_POINT_MAGNIFY_CLAMP` = 3 (point filtering under magnification, linear filtering under minification and between mip levels, clamp-to-edge). `QuadInstance` has the fields of [entity_and_overlay_renderers.md](entity_and_overlay_renderers.md) §2, in this order: the position, the end point, the right and up axes, the half-width and half-height, the billboard blend start and length of `WorldOriented` (RND-306), the rotation, the colour and the end colour (the per-end colours of `Segment`, RND-319), the texture index, the UV-rectangle index, the orientation mode and the anchor.

The header is written in the common subset of C++ and HLSL. Its type names are those of the sketch below: under HLSL (`__HLSL_VERSION` defined) they are the HLSL types, and under C++ the DirectXMath and standard types of the same size and layout. A record holds only these types: an 8-bit colour of an instance or material record is a `Uint32` holding red, green, blue and alpha with red in the lowest byte (the byte order of `R8G8B8A8_UNORM`); the draw tint of `DrawConstants` and the fog and light colours of `FrameConstants` are `Float4` and `Float3` values, because a light colour can exceed 1 (README RND-013); and an enumerator is stored as its `Uint32` value. Matrices are packed row-major by `-Zpr` (RND-260), so the header needs no packing keyword. Every world transform in these layouts, `DrawConstants::world` and the per-instance and per-node transforms of the instance buffers (RND-015), is a `Float3x4` that holds the transpose of the maths library's row-vector `Mat3x4` ([platform layer](../platform_layer.md) PLT-136): presentation code writes the x components of `axisX`, `axisY`, `axisZ` and `translation` as row 0, their y components as row 1 and their z components as row 2, each rounded once to the nearest binary32 value (PLT-135), and a shader applies it as `mul(world, float4(p, 1))` to a point and `mul(world, float4(d, 0))` to a direction. It is the one transposed matrix of the renderer, because three rows of four fill three 16-byte registers in C++ and in HLSL alike, where a row-major `float4x3` starts each of its four rows on a register of its own and no longer matches `XMFLOAT4X3`; every other matrix keeps the row-vector form `mul(v, M)` of ADR-016. The GPU records of the animation and VFX runtime ([particles_shatter_and_screen_effects.md](../animation_and_vfx/particles_shatter_and_screen_effects.md) §2) are declared the same way in that runtime's shared header, `ParticleSlot.h`, which includes `FrameConstants.h` for these type names.

```cpp
// FrameConstants.h: the shared layout header of the renderer project, included by C++ and by every shader (RND-262).
#pragma once

#ifndef __HLSL_VERSION
#   include <DirectXMath.h>

#   include <cstdint>
#endif

namespace Neuron
{

#ifdef __HLSL_VERSION
typedef float2 Float2;
typedef float3 Float3;
typedef float4 Float4;
typedef float3x3 Float3x3;
typedef float3x4 Float3x4;
typedef float4x4 Float4x4;
typedef uint Uint32;
#else
using Float2 = DirectX::XMFLOAT2;
using Float3 = DirectX::XMFLOAT3;
using Float4 = DirectX::XMFLOAT4;
using Float3x3 = DirectX::XMFLOAT3X3;
using Float3x4 = DirectX::XMFLOAT3X4;
using Float4x4 = DirectX::XMFLOAT4X4;
using Uint32 = std::uint32_t;
#endif

static const Uint32 SAMPLER_LINEAR_CLAMP = 0;
static const Uint32 SAMPLER_LINEAR_WRAP = 1;
static const Uint32 SAMPLER_POINT_CLAMP = 2;
static const Uint32 SAMPLER_POINT_MAGNIFY_CLAMP = 3;

struct FrameConstants // matrices are row-major (-Zpr, RND-260)
{
  Float4x4 view;
  Float4x4 projection;
  Float4x4 viewProjection;
  Float3 cameraPosition;
  float fogStart;
  Float3 cameraFront;
  float fogEnd;
  Float3 fogColor;
  float extrapolationTime;
  Float3 light0Towards;
  float realClock;
  Float3 light0Color;
  float presentationFrameTime;
  Float3 light1Towards;
  float smoothPresentationTime;
  Float3 light1Color;
  float padding0;
  Float2 renderSize;
  Float2 padding1;
};

struct DrawConstants // root constants
{
  Float3x4 world; // the transpose of the object's Mat3x4, applied as mul(world, float4(p, 1))
  Float4 tint;
  Uint32 paletteIndex;
  Uint32 instanceBufferIndex;
  Uint32 textureIndex0;
  Uint32 textureIndex1;
  Uint32 materialIndex;
};

} // namespace Neuron
```

## 3 Data model

| Vertex or instance stream | Attributes | Used by |
|---|---|---|
| mesh vertex | position, facet colour (sRGB-encoded, ADR-013), facet normal, tint weight, texture coordinates where used, barycentric coordinates where the edge mask is used | `MeshLit`, `WorldOverlay` with `SOURCE` = `MESH` |
| terrain vertex | position, facet colour, facet normal, grid coordinates, barycentric coordinates | `Terrain` |
| sea vertex | position | `SeaFlat` |
| wave vertex | position and facet colour, from the buffer that `SeaWavesUpdate` writes | `SeaWaves` |
| quad instance | the `QuadInstance` fields of RND-262 (ADR-009) | `UnlitTextured`, `UnlitDualTexture`, `UnlitVertexColor`, `ScreenLine`, `WorldOverlay` |
| sky layer instance | height, extent, colour, texture view, sampler index, coordinate transform, fog range, kind (grid line or cloud layer) | `Sky` |

## 4 Behaviour and algorithms

### 4.1 The shader list

**RND-263** (MUST) The renderer's shaders, their stages, permutation axes and users are:

| Shader | Stages | Permutation axes (values, first is the default; stages each axis changes) | Catalogue entries | Passes |
|---|---|---|---|---|
| `Sky` | VS, PS | none | `sky` | 3, main-menu sky, loading view |
| `Terrain` | VS, PS | `INVERTED` (off, on; PS); `GRID_SOURCE` (`GRID_TEXTURE`, `BARYCENTRIC`; PS) | `terrain`, `terrain_inverted` | 4 |
| `SeaFlat` | VS, PS | `INVERTED` (off, on; PS) | `sea_flat`, `sea_flat_inverted` | 5a |
| `SeaWavesUpdate` | CS | none | — (compute) | 5b |
| `SeaWaves` | VS, PS | none | `sea_waves` | 5b |
| `MeshLit` | VS, PS | `FOG` (off, on; PS); `EDGE_MASK` (off, on; VS, PS) | `mesh_lit_fog`, `port_pad`, the lit-mesh entries of [entity_and_overlay_renderers.md](entity_and_overlay_renderers.md), the shatter-debris entry of [README.md](../animation_and_vfx/README.md) | 6, 7a, 7b, 8, 14 |
| `UnlitTextured` | VS, PS | `FOG` (off, on; PS); `CUTOUT` (off, on; PS); `ALPHA_ONE` (off, on; PS) | `glow_additive_fog`, `glow_additive_fog_nodepth`, `alpha_fog` and the textured sprite entries of the owner documents | 7a to 7d, 8, 10a, 11, 15, 16, 20, 23 |
| `UnlitDualTexture` | VS, PS | `COMBINE` (`PRODUCT`, `TEXTURE_COLOR`: the combine modes `product` and `texture_color` of RND-268; PS); `CLIP_PLANES` (0, 2; VS); `FOG` (off, on; PS) | `dual_alpha_fog`, `dual_additive_clip_fog` | 8 |
| `UnlitVertexColor` | VS, PS | `FOG` (off, on; PS) | the untextured quad entries of the owner documents; the map-editor sea fill | 7b, 8, 10a, 23, map editor |
| `ScreenLine` | VS, PS | `FOG` (off, on; PS) | `line_additive_fog` and the line entries of the owner documents | 8, 12, 20 |
| `DepthCopy` | CS | `SAMPLES` (1, 2, 4; CS) | — (compute) | after 15 |
| `Final` | VS, PS | `HDR` (off, on; PS) | the final-pass entry | resolve to `final_image` |
| `FullscreenFade` | VS, PS | none | `fade` | 27 |
| `ScreenTransition` | VS, PS | `EDGE_STRIP` (off, on; VS, PS) | `transition` | 34 |
| `InstanceCull` | CS | none | — (compute) | before the draws of 7a to 7d, 10a, 10b and 11 ([entity_and_overlay_renderers.md](entity_and_overlay_renderers.md) RND-310) |
| `WorldOverlay` | VS, PS | `SOURCE` (`VERTEX_COLOR`, `TEXTURED`, `MESH`; VS, PS) | `cursor_ring`, `cursor_area`, `cursor_dots`, `cursor_ghost` ([entity_and_overlay_renderers.md](entity_and_overlay_renderers.md) RND-310) and the overlay entries of the owner documents | 16, 20 |
| `ParticleEmit` | CS | none | — (compute) | before 14 and 15: allocates the frame's emissions ([particles](../animation_and_vfx/particles_shatter_and_screen_effects.md) VFX-206) |
| `ParticleSimulate` | CS | none | — (compute) | before 14 and 15: one dispatch per sub-step (VFX-209, VFX-210) |
| `ParticleCompact` | CS | none | — (compute) | before 15: the live list and the indirect arguments of the two batches (VFX-205, VFX-217) |
| `ShardUpdate` | CS | none | — (compute) | before 14: tumblers and shards (VFX-228) |

The files follow RND-261: for example `Sky.hlsli`, `SkyVS.hlsl` and `SkyPS.hlsl`; `TerrainVS.hlsl`, `TerrainPS.hlsl` and `TerrainInvertedPS.hlsl` for the `terrain` and `terrain_inverted` entries, and `TerrainBarycentricPS.hlsl` and `TerrainInvertedBarycentricPS.hlsl` only where the catalogue uses option B of DEC-326; `MeshLitFogPS.hlsl` for `mesh_lit_fog`; `DepthCopySamples2CS.hlsl` and `DepthCopySamples4CS.hlsl` for the multisampled targets; `UnlitDualTextureTextureColorFogPS.hlsl` for `dual_alpha_fog`; and `ParticleEmitCS.hlsl`, `ParticleSimulateCS.hlsl`, `ParticleCompactCS.hlsl` and `ShardUpdateCS.hlsl`, with their byte arrays `g_ParticleEmitCS`, `g_ParticleSimulateCS`, `g_ParticleCompactCS` and `g_ShardUpdateCS`, in the `Shader/` folder of the animation and VFX runtime.

The sprite cards and billboards draw on `UnlitTextured` and `UnlitVertexColor` with the `QuadInstance` stream ([entity_and_overlay_renderers.md](entity_and_overlay_renderers.md), ADR-009); the particle batches draw on `UnlitTextured` and the shards on `MeshLit` with `EDGE_MASK`, from the buffers that the four compute shaders above write ([README.md](../animation_and_vfx/README.md) VFX-014, ADR-010). The UI batch and glyph shaders belong to the UI framework ([ui_framework/README.md](../ui_framework/README.md), ADR-031, ADR-034) and follow the same rules.

**RND-264** (MUST) The material shaders of ADR-003 map onto this list as follows: unlit vertex colour is `UnlitVertexColor`; one texture modulated by the vertex colour is `UnlitTextured`; two textures with an explicit combine mode is `UnlitDualTexture` (and `SeaFlat`); lit by two directional lights with no ambient term is `MeshLit`; lit with one texture and the wire-grid term is the `Terrain` shader's grid term, and the shatter-debris edge mask is `MeshLit` with `EDGE_MASK` on.

### 4.2 Per-shader rules

**RND-265** (MUST) `Sky`: the vertex shader expands each instance into its layer quad (a grid line or a cloud layer, RND-151, RND-153), facing downwards; the pixel shader samples the instance's texture with the instance's sampler (the flat cloud layers wrap their coordinates in the shader and sample with `SAMPLER_POINT_CLAMP`; cloud layers read the view `level0` of mip level 0 only, AST-026), multiplies by the instance colour, and applies black linear fog with the instance's fog range on the view-space depth. Its output alpha is the colour's alpha × the texel, which the additive blend uses as the weight.

**RND-266** (MUST) `Terrain`: with the facet normal *n*, the facet colour *D*, the lit colour of RND-233 for the surface material (specular 0) *L*ₛ and for the grid material (specular `rnd.terrain.overlay_specular`, shininess `rnd.terrain.overlay_shininess`) *L*_g, the grid value *g* (the single channel of `fx.terrain_grid` at the grid coordinates, or the barycentric edge term under `GRID_SOURCE` = `BARYCENTRIC`, smoothed with screen-space derivatives, ADR-015), the fog factor *f* and the fog colour *F* of RND-222:

- surface fragment *s* = lerp(*F*, *L*ₛ, *f*); grid fragment *q* = lerp(*F*, *L*_g × *g*, *f*); grid weight *α* = *D*.a;
- `INVERTED` off: output colour = clamp(*s* + *q* × *α*, 0, 1), written with the `Opaque` blend; this equals an opaque surface pass followed by an additive grid pass at equal depth;
- `INVERTED` on: output colour = 1 − (1 − *s*) × (1 − *q*), written with the `Darkening` blend; this equals two successive darkening passes.

`GRID_SOURCE` = `GRID_TEXTURE` is the default (see DEC-326); `BARYCENTRIC` is the alternative of DEC-326 option B and ADR-011.

**RND-267** (MUST) `SeaFlat`: the vertex shader derives both coordinate sets from the world position over the sea plane (RND-142): texture 0 with `rnd.sea.tile_repeats` repeats plus the scroll, texture 1 from 0 to 1 across the plane; the pixel shader applies the `product` combine mode (RND-268) with a white vertex colour (alpha 0 under `INVERTED`) and level fog.

**RND-268** (MUST) The two-texture combine modes are a closed list, each named for what it computes:

| Mode | Colour | Alpha | Used by |
|---|---|---|---|
| `product` | vertex colour × texture 0 × texture 1 | vertex alpha × texture-0 alpha × texture-1 alpha | the flat sea layer, the Beam Dish beam tubes |
| `texture_color` | texture 0 × texture 1: the colour of the textures alone, which the vertex colour does not affect | vertex alpha × texture-0 alpha × texture-1 alpha | the Barrier Emitter curtain at structure detail below 3 |

**RND-269** (MUST) `SeaWavesUpdate` runs one thread per wave-mesh vertex pair: it evaluates the pair's heights and its predecessor's heights from the wave formulae of [terrain](../terrain/edges_normals_colours_and_sea.md#414-sea-derivations) (TER-204, TER-205) at the smooth presentation time, looks up both vertex colours in the colour-table buffer, and writes the positions and the per-triangle facet colours of RND-145 into the current frame's wave buffer. `SeaWaves` draws that buffer unlit with level fog.

**RND-270** (MUST) `MeshLit` evaluates the lighting of RND-233 with the per-frame light rig (the map rig or the special rig, chosen per pass, RND-228, RND-229) and the material's specular colour and shininess; it applies the team tint of RND-271 to the facet colour before lighting and level fog under `FOG`. Under `EDGE_MASK` it lerps the lit facet colour to white by the edge mask computed from the barycentric coordinates, so the edge lines are unlit, as the shatter-debris owner specifies ([particles_shatter_and_screen_effects.md](../animation_and_vfx/particles_shatter_and_screen_effects.md) VFX-229, ADR-010).

**RND-271** (MUST) The team-tint function of ADR-020 is: with the facet colour *c*, the tint weight *w* of the facet, the instance's palette colour *T* (for the Worker figure, entry 0 multiplied by `rnd.tint.worker_figure_entry0_factor`) the mesh's tint flags and the mesh's tint scale *f* (RND-161: `rnd.tint.factor` unless the mesh names another, such as the Transport hull's `ref.unit.transport.hull_tint`), tinted = *T* × *f* for a convert-all mesh, and otherwise *T* × (*c.r* + *c.g* + *c.b*) ÷ (*T.r* + *T.g* + *T.b*) × *f* (0 when the palette colour is black); the result is clamp(*c* + *w* × (tinted − *c*), 0, 1). The weight is 0 on nodes below the direct children of the root and on instances of no team (RND-161, RND-162).

**RND-272** (MUST) `UnlitTextured` multiplies the texture by the vertex or instance colour; under `CUTOUT` it discards a pixel whose alpha is at or below the catalogue entry's threshold, or uses alpha-to-coverage when multisampling is on (ADR-015); under `ALPHA_ONE` it writes alpha 1, which turns the `Additive` blend into the pure addition of the reference "one, one" combination (RND-058).

**RND-273** (MUST) `UnlitDualTexture` under `CLIP_PLANES` = 2 writes two clip distances (`SV_ClipDistance0` and `SV_ClipDistance1`) from two planes in the per-draw material parameters, so beam tubes end exactly at the planes of their dishes (RND-171).

**RND-274** (MUST) `ScreenLine` expands each segment into a quad whose width is given in pixels of the target, with a one-pixel coverage ramp on each side that writes the smoothed edge into the alpha; it serves the reference's smoothed lines (RND-201).

**RND-275** (MUST) `DepthCopy` writes, for each pixel, the nearest sample of `scene_depth` (the largest reversed-Z value) into `scene_depth_single`; `Final` scales `scene_resolved` to the client area, applies the brightness constant (ADR-012) and, under `HDR`, the conversion of ADR-014; `FullscreenFade` writes black with the fade amount as alpha; `ScreenTransition` draws the edge strip under `EDGE_STRIP` and otherwise the captured frame with the quad and alpha of RND-096.

**RND-276** (MUST) No shader depends on a provoking-vertex convention: every facet attribute is either baked per triangle by the cookers (ADR-020) or written per triangle by `SeaWavesUpdate`. Fog, where enabled, always uses the view-space depth as its coordinate (RND-222).

## 5 Concurrency and performance budget

**RND-280** (MUST) Shader permutations are compiled at build time only; the catalogue creates every pipeline state at start-up or reconfiguration (ADR-005). `SeaWavesUpdate` is the only per-frame compute work of the world renderers; its cost is bounded by the wave-mesh size, which the sea-detail setting sets.

## 6 Failure modes, diagnostics and telemetry

| Failure | Behaviour | Requirement |
|---|---|---|
| a permutation is compiled but no catalogue entry uses it | a build error | RND-261 |
| a catalogue entry names a permutation that does not exist | a build error | RND-261 |
| a shared-header structure differs in size or member offset between C++ and HLSL | a build error | RND-262 |
| the embedded bytecode carries debug or reflection data | a build error (ADR-003) | RND-260 |
| a shader source is named or placed against RND-261, or a `.cso` or shader compiler library is in the install directory | a build or release check failure (`Build\CheckProjectFiles.py`, ADR-003) | RND-260, RND-261 |

PDB files of every shader build are kept for PIX source-level debugging and are never distributed (ADR-003).

## 7 Acceptance tests

The golden-image tests compare with the default tolerance and on the pinned reference configuration of the test harness ([diagnostics, test and performance](../diagnostics_test_and_performance/README.md) §4.8), unless a test states its own tolerance, as T4 does.

| Test | Kind | Checks | Requirements |
|---|---|---|---|
| T1 compile | build | every shader compiles through `FXCompile` with `-HV 2021` and no warning, to `*_6_6` profiles, with identical settings in Debug and Release; a check step of the build, the script `Build/CheckShaders.py`, compiles every `.hlsl` once more with the same `dxc` and `-HV 202x` to a discarded output and fails on any error or warning | RND-260 |
| T2 permutations | build | the compiled permutation set equals the set the catalogue references, and every source and byte-array name follows RND-261 | RND-261 |
| T3 shared layouts | build | the same script writes the assembly listing of each shader (`dxc -Fc`), reads from its buffer definitions the offset and size of every member of every constant buffer and structured buffer that uses a shared record, and compares them with the `offsetof` and `sizeof` values that the renderer's test project writes from the C++ declarations; any difference fails the build | RND-262 |
| T4 terrain equivalence | golden-image | the single-pass terrain equals a reference two-pass rendering (opaque surface, then additive grid at equal depth) within one code value, in both styles | RND-266 |
| T5 combine modes | golden-image | both combine modes with known textures and vertex colours | RND-268 |
| T6 tint | unit | the tint function for known colours, weights, flags and a black palette colour | RND-271 |
| T7 cut-out and alpha one | golden-image | cut-out thresholds and the pure-addition path | RND-272 |
| T8 provoking vertex | golden-image | triangles whose corners carry different values in unused attributes draw with their facet colour | RND-276 |
| T9 clip planes | golden-image | a beam tube ends exactly at both dish planes | RND-273 |

## 8 Open decisions

| Reference | Status | Where it applies |
|---|---|---|
| DEC-326 | Resolved (O83) | the terrain shader's grid source (RND-266) |
| DEC-040 | Open | the `INVERTED` permutations (RND-263, RND-266, RND-267) |
| ADR-003 | Accepted | the language, compiler and material set |
| ADR-011 | Accepted | the single-pass terrain and the GPU sea |

**Design-phase inputs.** Whether the world game needs further material techniques (each a shader change, a catalogue entry and a build, ADR-003); whether a level-of-detail scheme for the strategic zoom needs its own terrain or sea permutations (DEC-247).

## 9 Tuning table

This document defines no tuning keys of its own; the shaders read the keys of [world_renderers.md](world_renderers.md), [view_lighting_and_fog.md](view_lighting_and_fog.md) and [frame_graph_and_passes.md](frame_graph_and_passes.md) through the shared constants.
