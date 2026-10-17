---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Texture container and block-compression formats

## Context and Problem Statement

Species uses few textures, and most of the world's colour comes from vertex colours on flat-shaded meshes. The textures it has are of distinct kinds: colour art for sprites, cards, icons and UI; single-channel masks (glow masks, darkening masks, the team-tint mask, heights); a wire-grid edge pattern where a texture is still wanted; sky and water layers; and glyph atlases built at run time. Every texture is newly authored (vision brief §5) and is cooked once by the asset pipeline. The runtime must load textures without decoding or resampling them, with their format, colour space, dimensions and mip chain stated explicitly in the file, and every format must be supported by every GPU that meets the minimum capability (ADR-002).

The problem is which container and which pixel formats cooked textures use, and which tool produces them.

## Decision Drivers

- Load without decoding, converting or resampling: the cooked file maps directly to a GPU resource.
- Format, colour space (sRGB or linear), array size and mip count explicit in every file.
- Compression quality that keeps flat colour art and crisp UI edges clean.
- Hardware support on every Species-capable GPU.
- Reproducible output in CI, independent of the build machine's GPU.
- One maintained, first-party cooking library.

## Considered Options

1. **DDS with the DX10 header**, using **BC7**, **BC5** and **BC4**, with uncompressed formats for exact data, cooked with **DirectXTex**.
2. **KTX2 with Basis Universal** supercompression, transcoded to a GPU format at load.
3. **Uncompressed RGBA8** for every texture.
4. Image files (PNG) loaded and compressed **at run time**.

## Decision Outcome

Chosen option: **1, DDS with the DX10 header and BC7, BC5 and BC4**, because it is a direct GPU upload with explicit metadata, it has hardware support everywhere Species runs, and DirectXTex cooks it reproducibly.

- **Container.** Every cooked texture is a DDS file with the `DX10` header extension, so `dxgiFormat`, sRGB tagging, resource dimension and array size are explicit. The loader rejects any DDS without the DX10 header. Cooked DDS payloads are stored inside the runtime pack files (ADR-058).
- **Format rules** (the cooker applies them from each texture's import manifest, ADR-018):

| Texture kind | Format |
|---|---|
| colour art (sprites, cards, icons, sky and water colour layers) | `BC7_UNORM_SRGB` |
| UI art | `BC7_UNORM_SRGB`; `R8G8B8A8_UNORM_SRGB` when its manifest marks it pixel-exact |
| normal maps (if any) | `BC5_UNORM`, Z reconstructed in the shader |
| single-channel masks (glow, darkening, team tint, height, ambient occlusion) | `BC4_UNORM` |
| data that needs exact values | `R8_UNORM` or `R8G8_UNORM`, uncompressed |
| colour look-up tables read on the CPU (terrain colour ramps and gradients) | `R8G8B8A8_UNORM_SRGB`, uncompressed, one level, never uploaded to the GPU |
| glyph atlases | `R8_UNORM`, built at run time, never block-compressed (ADR-034) |

  BC1 to BC3 are not used. BC6H is used only if HDR sky or light textures are added, by a new revision of this ADR.
- **Sizes.** Textures are authored at the size at which they are used. Dimensions need not be powers of two; block-compressed textures must have top-level dimensions that are multiples of 4, and the content-build validator rejects sources that are not. The cooker never pads or resamples a source to reach a multiple of 4; the source art is authored at a conforming size. Nothing is resampled at load. The maximum dimension is 16384.
- **Mips.** Full mip chains are generated offline, except for textures whose manifest says they are drawn only at 1:1 scale (pixel-exact UI art), which have one level.
- **Tools.** The Species asset cooker, part of the content build tool under `Tools/` ([ADR-059](ADR-059-content-build-and-validators.md)), calls the **DirectXTex** library (`may2026`, the NuGet package restored for that tool only). DirectXTex is a development-tool dependency: it never links into the executable, which may not use it (`AGENTS.md` R14); the runtime loader is Species code that reads the DX10 header itself. texconv is used only for manual inspection. In CI, BC7 is encoded with the software codec, so the cooked output does not depend on the build machine's GPU or driver; artists may use the GPU codec locally.
- **Loading.** The loader creates each texture from its DX10 header. sRGB-tagged textures are created with the typeless format of their family and viewed through a `UNORM` format, as the working colour space requires (ADR-013).
- **Not used.** DirectStorage is not used for textures in v1 (see the pack-file decision, ADR-058).

### Consequences

- Good, because a cooked texture is uploaded as is, with no CPU decode or conversion.
- Good, because BC7 keeps flat colour regions and hard edges clean at a quarter of the RGBA8 size, and BC4 halves single-channel masks again.
- Good, because every format is supported at every feature level from 11_0 up.
- Good, because CI output is reproducible.
- Bad, because BC7 encoding with the software codec is slow, so the content build needs caching of cooked textures by source hash (ADR-059).
- Bad, because other tools may not read DDS files with the DX10 header; only the Species loader and DirectXTex need to.

### Confirmation

- The content-build validator rejects textures without a manifest, block-compressed sources whose dimensions are not multiples of 4, and any format outside the table.
- A loader test feeds DDS files without the DX10 header, with unknown formats and with truncated payloads, and checks that each is rejected with a diagnostic naming the asset id.
- A CI reproducibility test cooks the same sources twice on different machines and compares the outputs byte for byte.

## Pros and Cons of the Options

### 1. DDS with the DX10 header and BC7, BC5, BC4

- Good, because it is a direct upload with explicit metadata.
- Good, because the cooking library is first-party, current and available as a NuGet package for the content build tool.
- Bad, because cooking is slow without caching.

### 2. KTX2 with Basis Universal

- Good, because files are smaller on disk and portable across GPU vendors and APIs.
- Bad, because every load transcodes on the CPU, and transcoded quality for flat colour art and UI edges is below direct BC7.
- Bad, because Species targets one API, so portability brings no benefit.

### 3. Uncompressed RGBA8

- Good, because quality is exact and cooking is trivial.
- Bad, because memory and bandwidth are four times BC7, for no visible gain on most textures.

### 4. Run-time image loading and compression

- Good, because there is no texture cooking step.
- Bad, because every load decodes and compresses on the player's machine, slowly and with results that can differ between machines.

## More Information

- Technology baseline items 5.2, 5.3 and 8.2 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://github.com/microsoft/DirectXTex/releases · https://github.com/microsoft/DirectXTex/wiki/Texconv · https://learn.microsoft.com/windows/win32/direct3d11/texture-block-compression-in-direct3d-11 · https://learn.microsoft.com/windows/win32/direct3d11/bc7-format · https://learn.microsoft.com/windows/win32/direct3ddds/dx-graphics-dds-pguide · https://learn.microsoft.com/windows/win32/direct3ddds/dds-header-dxt10.
- Related: ADR-013 (working colour space), ADR-018 (texture import manifest), ADR-034 (glyph atlases), ADR-058 (pack files), ADR-059 (content build and validators).
- **Amendment 2026-10-17 (AGENTS.md, O21).** DirectXTex is a dependency of the content build tool only, restored by NuGet for that tool; the executable reads DDS with Species code (`AGENTS.md` R14).
- **Amendment 2026-10-17.** The format table names the CPU colour look-up tables of the asset pipeline component (`CpuTable`, AST-072). The size rule states that the cooker never pads a block-compressed source to whole 4 × 4 blocks (AST-068); asset rows whose sizes are not multiples of 4 are corrected in the asset set.
