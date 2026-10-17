---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# HDR output

## Context and Problem Statement

Additive glow on a black world is the kind of image that high-dynamic-range displays show well: glow could exceed the SDR white level instead of saturating, and the black world stays black. Many players' displays support HDR, and Windows 11 composes HDR and SDR content together. The look, however, is defined and tuned in SDR (ADR-013): glow saturates at the display maximum, and every colour constant is an SDR value. HDR output also makes bright flashes brighter, which matters for photosensitivity.

The problem is whether Species offers HDR output, in which format, how it is derived from the SDR-defined look, and whether it is on by default.

## Decision Drivers

- The SDR image is the reference image of the look; HDR must not change it below the SDR white level.
- The additive glow depends on blending and on values above 1.0, which an HDR path can keep instead of clamping.
- The UI keeps its SDR appearance at the player's SDR reference white level.
- Photosensitivity safety applies to HDR highlights too (ADR-064).
- Implementation and test cost for v1.

## Considered Options

1. **Optional HDR, off by default**, through an **scRGB FP16** swap chain (`R16G16B16A16_FLOAT`, `DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709`).
2. Optional HDR through **HDR10** (`R10G10B10A2_UNORM` with `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020`).
3. **No HDR output** in v1.
4. HDR **on by default** whenever the display supports it.

## Decision Outcome

Proposed option: **1, optional scRGB FP16 HDR, off by default**. **Default while Proposed:** the team ships SDR as the default and builds the HDR path behind a player option, because scRGB is the composition space of the Windows desktop compositor and keeps the over-range glow without changing the SDR-defined look.

- **Availability.** The option is offered only when the window's output reports an HDR colour space in `IDXGIOutput6::GetDesc1` and the swap chain reports support for the scRGB colour space through `CheckColorSpaceSupport`. The renderer re-queries both when `IDXGIFactory1::IsCurrent` returns `FALSE` or the window moves to another output, and falls back to SDR when HDR is no longer available.
- **Targets.** With HDR on, scene colour targets are `R16G16B16A16_FLOAT` holding encoded working values without per-layer clamping (ADR-013), and the swap chain is `R16G16B16A16_FLOAT` with `DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709` set through `SetColorSpace1`.
- **Final pass.** Each scene value is converted to linear light with the sRGB decoding curve, extended smoothly above 1.0; it is scaled so that encoded 1.0 maps to the player's SDR reference white level; and values above that level are tone-mapped towards the display's maximum luminance from `GetDesc1`. Values at or below 1.0 therefore reproduce the SDR image at SDR white; only over-range glow gains brightness. `SetHDRMetaData` is not relied on.
- **UI.** The UI and text are composed after tone mapping at the SDR reference white level, so menus look the same in SDR and HDR.
- **Limits.** A maximum HDR highlight level is a player setting, capped by the photosensitivity rule of ADR-064 for flashes and full-screen effects.
- **Not used.** HDR10 output (option 2) is not used.

### Consequences

- Good, because players with HDR displays can see glow beyond the SDR white level, while the SDR look stays the default and the reference.
- Good, because scRGB is the compositor's own space, so windowed HDR and alpha-blended overlays compose correctly.
- Good, because HDR off costs nothing.
- Bad, because the HDR path doubles scene-target bandwidth and adds pipeline states for the float target format (ADR-005).
- Bad, because HDR images cannot be fully checked by golden images on SDR CI machines; the HDR path needs tests on HDR hardware.

### Confirmation

- A unit test of the final-pass conversion checks that encoded values from 0 to 1.0 map to the SDR-white-scaled sRGB decoding curve within a tolerance, and that the tone mapper never exceeds the display maximum.
- A golden-image test renders the reference scenes with HDR on into a float target and checks that, after clamping to SDR white and re-encoding, they equal the SDR golden images.
- A manual QA check on HDR hardware covers windowed and borderless modes, moving the window between an HDR and an SDR display, and toggling HDR in Windows while the game runs.

## Pros and Cons of the Options

### 1. Optional scRGB FP16, off by default

- Good, because it adds HDR without changing the default look.
- Good, because scRGB composes correctly with the desktop and with alpha.
- Bad, because FP16 targets cost more bandwidth than SDR targets.

### 2. Optional HDR10

- Good, because 32-bit targets cost less bandwidth than FP16.
- Bad, because the swap chain must not be alpha-blended by the compositor, and the PQ encoding makes the conversion from the SDR-defined look harder to keep exact.

### 3. No HDR in v1

- Good, because it costs nothing now.
- Bad, because it forgoes a visible improvement that suits the glow-on-black look, and adding it later touches the frame graph and the catalogue anyway.

### 4. HDR on by default

- Good, because more players would see it.
- Bad, because the default image would then differ from the reference image of the look, and HDR calibration varies widely between displays.

## More Information

- Technology baseline item 4.6 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://learn.microsoft.com/windows/win32/direct3darticles/high-dynamic-range · https://learn.microsoft.com/windows/win32/api/dxgi1_5/nf-dxgi1_5-idxgiswapchain4-sethdrmetadata.
- Related: ADR-005 (catalogue entries per target format), ADR-012 (swap chain), ADR-013 (working colour space), ADR-064 (accessibility, photosensitivity).
