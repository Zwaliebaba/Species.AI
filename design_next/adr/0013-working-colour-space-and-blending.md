---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Working colour space and blending

## Context and Problem Statement

The Species look is a set of blending techniques: additive glow on a black world, multiplicative darkening layers, over-bright facets lit by light colours up to 2.0 per channel, black linear fog, and sprite cards whose glow accumulates where crowds overlap. The renderer component's visual-language requirements define these techniques on **display-encoded** colour values: every colour constant (the team palette, fog colours, light colours, UI colours) and every colour texture is a display-encoded (sRGB-encoded) value, blends combine display-encoded values, and each blend result is clamped to the displayable range before the next layer is drawn. The intensity of a glow halo, the depth of a darkening layer and the point at which overlapping glow saturates to white all follow from that.

A renderer that converts colours to linear light before blending produces different halos, darker darkening and later saturation, which would change the look that the vision brief keeps unchanged (O9).

The problem is in which colour space the renderer computes and blends, what format its colour targets have, and how colour metadata in assets is interpreted.

## Decision Drivers

- The look is kept exactly (vision brief O9; the renderer component's visual-language requirements).
- Blend results clamp per layer, as the techniques require.
- Asset metadata stays truthful (a colour texture is tagged as sRGB-encoded), so tools and a later change of working space read it correctly.
- glTF defines vertex colours and colour factors in linear terms; the cooker must convert them exactly once.
- HDR output, where offered, is derived from the same working values (ADR-0014).

## Considered Options

1. A **display-referred working space**: every colour is an sRGB-encoded value; shaders and blends operate on encoded values; SDR colour targets are `UNORM` and clamp per layer; no sRGB decode on sampling and no encode on write.
2. A **linear-light working space**: colour textures are decoded on sampling, blending happens in linear light in floating-point targets, and the final pass encodes to sRGB.
3. A linear-light working space with the **art and colour constants retuned** to approximate the look.

## Decision Outcome

Proposed option: **1, a display-referred working space**. **Default while Proposed:** the renderer computes and blends sRGB-encoded values in clamping `UNORM` targets, because that is how the visual-language requirements define the look, and it reproduces the look with no retuning.

- **Targets.** SDR scene colour targets are `R8G8B8A8_UNORM`. Every blend result clamps to [0, 1] per channel as it is written, so additive layers saturate and darkening layers multiply the clamped result, as the techniques require. The back buffer (ADR-0012) is written with a `UNORM` (not `_SRGB`) view, because its contents are already encoded.
- **Textures.** The cooker tags colour textures as sRGB-encoded (for example `BC7_UNORM_SRGB`, ADR-0017) and data textures as linear. The loader creates sRGB-tagged textures with the typeless format of their family and gives shaders a `UNORM` view, so samples are the encoded values. Mip levels of sRGB-tagged textures are filtered with sRGB-aware filtering at cook time.
- **Mesh colours.** The mesh cooker converts glTF vertex colours (`COLOR_0`) and colour factors from their linear definition to sRGB-encoded values with the exact sRGB encoding curve, once, at cook time (ADR-0019, ADR-0020).
- **Constants.** Palette colours, fog colours, light colours and UI colours are stored and used as encoded values, exactly as the tuning tables give them.
- **Lighting.** Lighting multiplies encoded colours by the sum of the two lights' encoded colours weighted by their facing terms, and clamps on write, as the shader set of ADR-0003 specifies.
- **HDR.** When HDR output is enabled, the scene targets become `R16G16B16A16_FLOAT` holding encoded values without per-layer clamping, and the final pass converts them to the output colour space (ADR-0014). The HDR image intentionally differs from the SDR image above the SDR white level.
- Changing to a linear-light working space later (option 2) would be a deliberate change of the look and needs a new ADR; because texture metadata is truthful, it would change shader views and constants, not assets.

### Consequences

- Good, because the glow, darkening, saturation and fog behave exactly as the visual-language requirements describe.
- Good, because colour constants from the tuning tables are used directly, with no conversion.
- Good, because asset metadata remains correct for every tool that reads it.
- Bad, because blending in encoded space is not physically based; lighting and filtering are approximate in the photometric sense (irrelevant to this look, but a constraint on any future physically based feature).
- Bad, because 8-bit targets can band in dark fog gradients, as the look's own definition implies; a 10-bit `UNORM` target is a possible later refinement that keeps per-layer clamping.

### Confirmation

- Golden-image tests of the renderer component render the reference test scenes (overlapping glow, stacked darkening layers, over-bright facets, fog ramp) and compare them with reference images computed from the visual-language formulas in encoded space.
- A loader test checks that every sRGB-tagged texture is viewed through a `UNORM` format and that data textures are unaffected.
- A cooker test converts known linear glTF colours and compares the encoded results with the sRGB encoding formula.

## Pros and Cons of the Options

### 1. Display-referred working space

- Good, because it matches the look's definition and needs no retuning.
- Good, because the per-layer clamp comes for free from `UNORM` targets.
- Bad, because it rules out physically based lighting without a further ADR.

### 2. Linear-light working space

- Good, because filtering, blending and lighting are photometrically correct, and HDR output follows naturally.
- Bad, because glow halos, darkening depth and saturation all change, which breaks the kept look.

### 3. Linear-light working space with retuned art and constants

- Good, because it keeps linear-light rendering while approaching the look.
- Bad, because no retuning can reproduce per-layer clamping and encoded-space blending exactly; every colour constant would differ from its tuning table, and the result would need open-ended visual comparison.

## More Information

- Related: ADR-0003 (shader set and lighting), ADR-0005 (blend states), ADR-0012 (back-buffer format), ADR-0014 (HDR output), ADR-0017 (texture formats and sRGB tags), ADR-0018 (texture import manifest), ADR-0020 (flat shading and team tint).
- The renderer component (`architecture/components/renderer/`) states the visual-language requirements that define the look in encoded values.
