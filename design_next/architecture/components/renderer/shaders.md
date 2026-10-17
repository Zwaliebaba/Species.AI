# Shaders

> Status: Engine baseline · Prefix: RND · ADRs: [ADR-0002](../../../adr/0002-minimum-gpu-capability.md) · [ADR-0003](../../../adr/0003-shader-language-compiler-and-material-shaders.md) · [ADR-0005](../../../adr/0005-pipeline-state-catalogue.md) · [ADR-0006](../../../adr/0006-root-signature-and-bindless-resource-access.md) · [ADR-0009](../../../adr/0009-instanced-crowds-sprite-cards-and-shadows.md) · [ADR-0010](../../../adr/0010-gpu-particles-and-shatter-debris.md) · [ADR-0011](../../../adr/0011-world-surface-rendering.md) · [ADR-0013](../../../adr/0013-working-colour-space-and-blending.md) · [ADR-0014](../../../adr/0014-hdr-output.md) · [ADR-0015](../../../adr/0015-scene-anti-aliasing.md) · [ADR-0018](../../../adr/0018-texture-import-manifest.md) · [ADR-0020](../../../adr/0020-flat-shading-and-team-tint.md) · Related: [Renderer](README.md) · [Frame graph and passes](frame_graph_and_passes.md) · [World renderers](world_renderers.md) · [View, lighting and fog](view_lighting_and_fog.md)

This document lists the HLSL shaders of the renderer, derived from the ordered pass table ([frame_graph_and_passes.md](frame_graph_and_passes.md) §4.3) and its catalogue entries (§3.4), with their inputs, outputs and permutation axes, the closed list of two-texture combine modes, and the team-tint function (ADR-0003, ADR-0020).

## 1 Responsibilities and boundaries

**RND-260** (MUST) The renderer uses exactly the shaders of §4.1. Each is written in HLSL 2021, compiled offline by the pinned DXC of ADR-0003 to a `*_6_6` profile with warnings as errors, and also compiles under `-HV 202x`; the client loads DXIL only, and no shader is compiled at run time (ADR-0003). Shaders reach resources only through the directly indexed descriptor heaps of ADR-0006, wrapping any index that can differ within a wave in `NonUniformResourceIndex`.

**RND-261** (MUST) Each shader declares its permutation axes in the list below; a permutation that is not declared does not exist, and every compiled permutation is used by at least one catalogue entry (ADR-0003, ADR-0005). Optional fast paths behind capability checks (ADR-0002) are not part of the baseline.

## 2 Interfaces

**RND-262** (MUST) One shared header, included by C++ and by HLSL, declares every constant and structured-buffer layout (ADR-0003): the per-frame constants of README RND-014 (`FrameConstants`), the per-draw root constants of RND-015 (`DrawConstants`), the instance records (`MeshInstance`, `QuadInstance`, `SkyLayerInstance`, `WaveVertexPair`), the material parameters (`LitMaterial`: specular colour, shininess; `CombineParams`: the combine mode) and the fixed sampler indices of the closed sampler set of ADR-0018: `kSamplerLinearClamp` = 0, `kSamplerLinearWrap` = 1, `kSamplerPointClamp` = 2.

```hlsl
// Shared layout sketch (HLSL side); the C++ side includes the same declarations.
struct FrameConstants {
    row_major float4x4 view, projection, viewProjection;
    float3 cameraPosition;  float  fogStart;
    float3 cameraFront;     float  fogEnd;
    float3 fogColour;       float  extrapolationTime;
    float3 light0Towards;   float  realClock;
    float3 light0Colour;    float  presentationFrameTime;
    float3 light1Towards;   float  smoothPresentationTime;
    float3 light1Colour;    float  padding0;
    float2 renderSize;      float2 padding1;
};

struct DrawConstants {               // root constants
    row_major float3x4 world;
    float4 tint;
    uint   paletteIndex;
    uint   instanceBufferIndex;
    uint   textureIndex0, textureIndex1;
    uint   materialIndex;
};
```

## 3 Data model

| Vertex or instance stream | Attributes | Used by |
|---|---|---|
| mesh vertex | position, facet colour (sRGB-encoded, ADR-0013), facet normal, tint weight, texture coordinates where used, barycentric coordinates where the edge mask is used | `mesh_lit` |
| terrain vertex | position, facet colour, facet normal, grid coordinates, barycentric coordinates | `terrain` |
| sea vertex | position | `sea_flat` |
| wave vertex | position and facet colour, from the buffer that `sea_waves_update` writes | `sea_waves` |
| quad instance | centre or end points, size, colour, orientation mode (ADR-0009), texture index | `unlit_textured`, `unlit_dual_texture`, `unlit_vertex_colour`, `screen_line` |
| sky layer instance | height, extent, colour, texture view, sampler index, coordinate transform, fog range, kind (grid line or cloud layer) | `sky` |

## 4 Behaviour and algorithms

### 4.1 The shader list

**RND-263** (MUST) The renderer's shaders, their stages, permutation axes and users are:

| Shader | Stages | Permutation axes | Catalogue entries | Passes |
|---|---|---|---|---|
| `sky` | VS, PS | none | `sky` | 3, main-menu sky, loading view |
| `terrain` | VS, PS | `INVERTED` (off, on); `GRID_SOURCE` (`grid_texture`, `barycentric`) | `terrain`, `terrain_inverted` | 4 |
| `sea_flat` | VS, PS | `INVERTED` (off, on) | `sea_flat`, `sea_flat_inverted` | 5a |
| `sea_waves_update` | CS | none | — (compute) | 5b |
| `sea_waves` | VS, PS | none | `sea_waves` | 5b |
| `mesh_lit` | VS, PS | `FOG` (off, on); `EDGE_MASK` (off, on) | `mesh_lit_fog`, `port_pad`, the lit-mesh entries of [entity_and_overlay_renderers.md](entity_and_overlay_renderers.md), the shatter-debris entry of [animation_and_vfx.md](../animation_and_vfx.md) | 6, 7b, 8, 14 |
| `unlit_textured` | VS, PS | `FOG` (off, on); `CUTOUT` (off, on); `ALPHA_ONE` (off, on) | `glow_additive_fog`, `glow_additive_fog_nodepth`, `alpha_fog` and the textured sprite entries of the owner documents | 7a to 7d, 8, 10a, 16, 20, 23 |
| `unlit_dual_texture` | VS, PS | `COMBINE` (`modulate`, `replace_then_modulate`); `CLIP_PLANES` (0, 2); `FOG` (off, on) | `dual_alpha_fog`, `dual_additive_clip_fog` | 8 |
| `unlit_vertex_colour` | VS, PS | `FOG` (off, on) | the untextured quad entries of the owner documents; the map-editor sea fill | 11, map editor |
| `screen_line` | VS, PS | `FOG` (off, on) | `line_additive_fog` and the line entries of the owner documents | 8, 12, 20 |
| `depth_copy` | CS | `SAMPLES` (1, 2, 4) | — (compute) | after 15 |
| `final` | VS, PS | `HDR` (off, on) | the final-pass entry | resolve to `final_image` |
| `fullscreen_fade` | VS, PS | none | `fade` | 27 |
| `screen_transition` | VS, PS | `EDGE_STRIP` (off, on) | `transition` | 34 |

The remaining shaders of the frame belong to their owner documents and follow the same rules: the sprite-card and billboard shaders and the instance-culling compute shader ([entity_and_overlay_renderers.md](entity_and_overlay_renderers.md), ADR-0009); the particle emit, simulate, compact and draw shaders and the shard update and draw shaders ([animation_and_vfx.md](../animation_and_vfx.md), ADR-0010); the UI batch and glyph shaders ([ui_framework/README.md](../ui_framework/README.md), ADR-0031, ADR-0034).

**RND-264** (MUST) The material shaders of ADR-0003 map onto this list as follows: unlit vertex colour is `unlit_vertex_colour`; one texture modulated by the vertex colour is `unlit_textured`; two textures with an explicit combine mode is `unlit_dual_texture` (and `sea_flat`); lit by two directional lights with no ambient term is `mesh_lit`; lit with one texture and the wire-grid edge mask is `mesh_lit` with `EDGE_MASK` on and the `terrain` shader's grid term.

### 4.2 Per-shader rules

**RND-265** (MUST) `sky`: the vertex shader expands each instance into its layer quad (a grid line or a cloud layer, RND-151, RND-153), facing downwards; the pixel shader samples the instance's texture with the instance's sampler (the flat cloud layers wrap their coordinates in the shader and sample with `kSamplerPointClamp`; cloud layers read a view of mip level 0 only), multiplies by the instance colour, and applies black linear fog with the instance's fog range on the view-space depth. Its output alpha is the colour's alpha × the texel, which the additive blend uses as the weight.

**RND-266** (MUST) `terrain`: with the facet normal *n*, the facet colour *D*, the lit colour of RND-233 for the surface material (specular 0) *L*ₛ and for the grid material (specular `rnd.terrain.overlay_specular`, shininess `rnd.terrain.overlay_shininess`) *L*_g, the grid value *g* (the single channel of `fx.terrain_grid` at the grid coordinates, or the barycentric edge term under `GRID_SOURCE = barycentric`, smoothed with screen-space derivatives, ADR-0015), the fog factor *f* and the fog colour *F* of RND-222:

- surface fragment *s* = lerp(*F*, *L*ₛ, *f*); grid fragment *q* = lerp(*F*, *L*_g × *g*, *f*); grid weight *α* = *D*.a;
- `INVERTED` off: output colour = clamp(*s* + *q* × *α*, 0, 1), written with the `Opaque` blend; this equals an opaque surface pass followed by an additive grid pass at equal depth;
- `INVERTED` on: output colour = 1 − (1 − *s*) × (1 − *q*), written with the `Darkening` blend; this equals two successive darkening passes.

`GRID_SOURCE = grid_texture` is the default (see DEC-326); `barycentric` is the alternative of DEC-326 option B and ADR-0011.

**RND-267** (MUST) `sea_flat`: the vertex shader derives both coordinate sets from the world position over the sea plane (RND-142): texture 0 with `rnd.sea.tile_repeats` repeats plus the scroll, texture 1 from 0 to 1 across the plane; the pixel shader applies the `modulate` combine mode (RND-268) with a white vertex colour (alpha 0 under `INVERTED`) and level fog.

**RND-268** (MUST) The two-texture combine modes are a closed list:

| Mode | Colour | Alpha | Used by |
|---|---|---|---|
| `modulate` | vertex colour × texture 0 × texture 1 | vertex alpha × texture-0 alpha × texture-1 alpha | the flat sea layer, the Beam Dish beam tubes |
| `replace_then_modulate` | texture 0 × texture 1 (the vertex colour does not affect the colour) | vertex alpha × texture-0 alpha × texture-1 alpha | the Barrier Emitter curtain at structure detail below 3 |

**RND-269** (MUST) `sea_waves_update` runs one thread per wave-mesh vertex pair: it evaluates the pair's heights and its predecessor's heights from the wave formulae of [terrain.md](../terrain.md) at the smooth presentation time, looks up both vertex colours in the colour-table buffer, and writes the positions and the per-triangle facet colours of RND-145 into the current frame's wave buffer. `sea_waves` draws that buffer unlit with level fog.

**RND-270** (MUST) `mesh_lit` evaluates the lighting of RND-233 with the per-frame light rig (the map rig or the special rig, chosen per pass, RND-228, RND-229) and the material's specular colour and shininess; it applies the team tint of RND-271 to the facet colour before lighting and level fog under `FOG`. Under `EDGE_MASK` it multiplies by the edge mask computed from the barycentric coordinates, as the shatter-debris owner specifies (ADR-0010).

**RND-271** (MUST) The team-tint function of ADR-0020 is: with the facet colour *c*, the tint weight *w* of the facet, the instance's palette colour *T* (for the Worker figure, entry 0 multiplied by `rnd.tint.worker_figure_entry0_factor`) and the mesh's tint flags, tinted = *T* × `rnd.tint.factor` for a convert-all mesh, *T* × `rnd.tint.transport_factor` for the Transport hull, and otherwise *T* × (*c.r* + *c.g* + *c.b*) ÷ (*T.r* + *T.g* + *T.b*) × `rnd.tint.factor` (0 when the palette colour is black); the result is clamp(*c* + *w* × (tinted − *c*), 0, 1). The weight is 0 on nodes below the direct children of the root and on instances of no team (RND-161, RND-162).

**RND-272** (MUST) `unlit_textured` multiplies the texture by the vertex or instance colour; under `CUTOUT` it discards a pixel whose alpha is at or below the catalogue entry's threshold, or uses alpha-to-coverage when multisampling is on (ADR-0015); under `ALPHA_ONE` it writes alpha 1, which turns the `Additive` blend into the pure addition of the reference "one, one" combination (RND-058).

**RND-273** (MUST) `unlit_dual_texture` under `CLIP_PLANES = 2` writes two clip distances (`SV_ClipDistance0` and `SV_ClipDistance1`) from two planes in the per-draw material parameters, so beam tubes end exactly at the planes of their dishes (RND-171).

**RND-274** (MUST) `screen_line` expands each segment into a quad whose width is given in pixels of the target, with a one-pixel coverage ramp on each side that writes the smoothed edge into the alpha; it serves the reference's smoothed lines (RND-201).

**RND-275** (MUST) `depth_copy` writes, for each pixel, the nearest sample of `scene_depth` (the largest reversed-Z value) into `scene_depth_single`; `final` scales `scene_resolved` to the client area, applies the brightness constant (ADR-0012) and, under `HDR`, the conversion of ADR-0014; `fullscreen_fade` writes black with the fade amount as alpha; `screen_transition` draws the edge strip under `EDGE_STRIP` and otherwise the captured frame with the quad and alpha of RND-096.

**RND-276** (MUST) No shader depends on a provoking-vertex convention: every facet attribute is either baked per triangle by the cookers (ADR-0020) or written per triangle by `sea_waves_update`. Fog, where enabled, always uses the view-space depth as its coordinate (RND-222).

## 5 Concurrency and performance budget

**RND-280** (MUST) Shader permutations are compiled at build time only; the catalogue creates every pipeline state at start-up or reconfiguration (ADR-0005). `sea_waves_update` is the only per-frame compute work of the world renderers; its cost is bounded by the wave-mesh size, which the sea-detail setting sets.

## 6 Failure modes, diagnostics and telemetry

| Failure | Behaviour | Requirement |
|---|---|---|
| a permutation is compiled but no catalogue entry uses it | a build error | RND-261 |
| a catalogue entry names a permutation that does not exist | a build error | RND-261 |
| a shared-header structure differs in size or member offset between C++ and HLSL | a build error | RND-262 |
| a distributed blob carries debug or reflection data | a build error (ADR-0003) | RND-260 |

PDB files of every shader build are kept for PIX source-level debugging and are never distributed (ADR-0003).

## 7 Acceptance tests

| Test | Kind | Checks | Requirements |
|---|---|---|---|
| T1 compile | build | every shader compiles in both language modes with no warning, to `*_6_6` profiles | RND-260 |
| T2 permutations | build | the compiled permutation set equals the set the catalogue references | RND-261 |
| T3 shared layouts | build | sizes and offsets of every shared structure match in C++ and HLSL | RND-262 |
| T4 terrain equivalence | golden-image | the single-pass terrain equals a reference two-pass rendering (opaque surface, then additive grid at equal depth) within one code value, in both styles | RND-266 |
| T5 combine modes | golden-image | both combine modes with known textures and vertex colours | RND-268 |
| T6 tint | unit | the tint function for known colours, weights, flags and a black palette colour | RND-271 |
| T7 cut-out and alpha one | golden-image | cut-out thresholds and the pure-addition path | RND-272 |
| T8 provoking vertex | golden-image | triangles whose corners carry different values in unused attributes draw with their facet colour | RND-276 |
| T9 clip planes | golden-image | a beam tube ends exactly at both dish planes | RND-273 |

## 8 Open decisions

| Reference | Status | Where it applies |
|---|---|---|
| DEC-326 | Open | the terrain shader's grid source (RND-266) |
| DEC-040 | Open | the `INVERTED` permutations (RND-263, RND-266, RND-267) |
| ADR-0003 | Accepted | the language, compiler and material set |
| ADR-0011 | Accepted | the single-pass terrain and the GPU sea |

**Design-phase inputs.** Whether the world game needs further material techniques (each a shader change, a catalogue entry and a build, ADR-0003); whether a level-of-detail scheme for the strategic zoom needs its own terrain or sea permutations (DEC-247).

## 9 Tuning table

This document defines no tuning keys of its own; the shaders read the keys of [world_renderers.md](world_renderers.md), [view_lighting_and_fog.md](view_lighting_and_fog.md) and [frame_graph_and_passes.md](frame_graph_and_passes.md) through the shared constants.
