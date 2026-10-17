---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# ADR-0015 Scene anti-aliasing

## Context and Problem Statement

Species draws hard-edged, flat-shaded facets, thin glowing lines (lasers, trails, the wire-grid overlay), and crowds of small sprite cards that move quickly across the screen. Hard polygon edges show stair-stepping at common resolutions, and small moving cards shimmer. The flip-model swap chain cannot be multisampled (ADR-0012), so any anti-aliasing happens in the scene targets before the final pass. The working colour space clamps blends per layer in `UNORM` targets (ADR-0013), and any anti-aliasing method must keep glow and darkening layers as defined.

The problem is which anti-aliasing method the renderer uses for the 3-D scene, and what the default is.

## Decision Drivers

- Smooth polygon edges on flat-shaded geometry without blurring the facets themselves.
- No ghosting or smearing of fast-moving crowds, glow lines and trails.
- Glow and darkening layers keep their per-layer clamped blending (ADR-0013).
- Cost suitable for the minimum-capability GPU at 1080p, with a lower setting for weaker GPUs.
- The UI and text are never anti-aliased by a scene method (they are drawn at native resolution after it).

## Considered Options

1. **Multisample anti-aliasing (MSAA)** on the scene colour and depth targets, resolved before the full-screen passes, with Off, 2x and 4x settings.
2. **Temporal anti-aliasing (TAA)** with a jittered projection and history reprojection.
3. **Post-process anti-aliasing** (an edge-detecting full-screen filter of the FXAA or SMAA kind).
4. **No anti-aliasing.**

## Decision Outcome

Proposed option: **1, MSAA with Off, 2x and 4x settings**. **Default while Proposed:** 4x MSAA where the GPU supports it for the scene target format, because MSAA smooths exactly the geometric edges that flat shading exposes, does not blur facets, and has no temporal artefacts.

- The scene colour target and the depth target are multisampled at the chosen sample count; support is checked with `D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS` for each scene format (SDR and HDR, ADR-0014). If 4x is not supported, the default falls back to 2x, then Off.
- Sprite cards and other alpha-cut-out materials use alpha-to-coverage when MSAA is on, so their edges are smoothed too; the pipeline-state catalogue (ADR-0005) carries the sample count and the alpha-to-coverage flag in its keys.
- The frame graph (ADR-0004) resolves the scene colour target before the first full-screen pass that samples it (screen transition, tone mapping, brightness).
- The wire-grid edge term (ADR-0003) is computed with screen-space derivative smoothing, so wire lines are smooth at every sample count, including Off.
- **Off** draws the scene with one sample, which is the reference appearance of polygon edges.
- Temporal anti-aliasing and post-process anti-aliasing are not part of the baseline.

### Consequences

- Good, because polygon edges are smoothed without any blur of flat facets or text.
- Good, because MSAA has no history, so fast crowds and glow trails show no ghosting.
- Good, because the per-layer clamped blending of glow and darkening works per sample, unchanged.
- Bad, because 4x MSAA multiplies scene colour and depth memory and bandwidth by four.
- Bad, because shader-computed effects inside triangles (other than the smoothed edge term) are not anti-aliased by MSAA.
- Bad, because the sample count is part of the catalogue keys, so changing it creates pipeline states during reconfiguration.

### Confirmation

- Golden-image tests render the reference scenes at Off and 4x; the Off images must equal the single-sample reference images, and the 4x images must stay within a tolerance of them away from edges.
- A performance test measures scene GPU time at each setting on the minimum-capability GPU at 1080p and records the result in the renderer component's budget.
- A capability test checks the fallback order for formats that do not support 4x.

## Pros and Cons of the Options

### 1. MSAA

- Good, because it targets geometric edges, the main aliasing source in a flat-shaded scene.
- Good, because it is stable over time.
- Bad, because of its memory and bandwidth cost.

### 2. TAA

- Good, because it smooths every kind of aliasing, including shading inside triangles, at low cost per sample.
- Bad, because it ghosts and smears fast-moving small figures and thin glow lines, which dominate Species battles.
- Bad, because the jitter and history interact with additive glow accumulation and make golden-image testing unstable.

### 3. Post-process anti-aliasing

- Good, because it is cheap and needs no multisampled targets.
- Bad, because it blurs thin lines and small sprite cards, which are part of the look, and it misses sub-pixel edges.

### 4. No anti-aliasing

- Good, because it costs nothing and is the reference appearance.
- Bad, because polygon edges stair-step and small moving cards shimmer at common resolutions.

## More Information

- Related: ADR-0003 (edge term), ADR-0004 (resolve pass), ADR-0005 (sample count in catalogue keys), ADR-0012 (no multisampled back buffer), ADR-0013 (per-layer clamping), ADR-0014 (HDR scene format).
