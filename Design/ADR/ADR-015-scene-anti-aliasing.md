---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Scene anti-aliasing

## Context and Problem Statement

Species draws hard-edged, flat-shaded facets, thin glowing lines (lasers, trails, the wire-grid overlay), and crowds of small sprite cards that move quickly across the screen. Hard polygon edges show stair-stepping at common resolutions, and small moving cards shimmer. The flip-model swap chain cannot be multisampled (ADR-012), so any anti-aliasing happens in the scene targets before the final pass. The working colour space clamps blends per layer in `UNORM` targets (ADR-013), and any anti-aliasing method must keep glow and darkening layers as defined.

The problem is which anti-aliasing method the renderer uses for the 3-D scene, and what the default is.

## Decision Drivers

- Smooth polygon edges on flat-shaded geometry without blurring the facets themselves.
- No ghosting or smearing of fast-moving crowds, glow lines and trails.
- Glow and darkening layers keep their per-layer clamped blending (ADR-013).
- Cost suitable for the minimum-capability GPU at 1080p, with a lower setting for weaker GPUs.
- The UI and text are never anti-aliased by a scene method (they are drawn at native resolution after it).

## Considered Options

1. **Multisample anti-aliasing (MSAA)** on the scene colour and depth targets, resolved before the full-screen passes, with Off, 2x and 4x settings.
2. **Temporal anti-aliasing (TAA)** with a jittered projection and history reprojection.
3. **Post-process anti-aliasing** (an edge-detecting full-screen filter of the FXAA or SMAA kind).
4. **No anti-aliasing.**

## Decision Outcome

Proposed option: **1, MSAA with Off, 2x and 4x settings**. **Default while Proposed:** 4x MSAA where the GPU supports it for the scene target format, because MSAA smooths exactly the geometric edges that flat shading exposes, does not blur facets, and has no temporal artefacts. **Confirmed when:** the golden-image tests at Off and 4x pass and the measured scene GPU time at 4x on the minimum-capability GPU at 1080p fits the renderer budget, at the first playable (O20); a measurement over budget changes the default setting by a revision of this record, and edge artefacts that MSAA cannot remove in the visual test reopen option 3.

- The scene colour target and the depth target are multisampled at the chosen sample count; support is checked with `D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS` for each scene format (SDR and HDR, ADR-014). If 4x is not supported, the default falls back to 2x, then Off.
- Sprite cards and other alpha-cut-out materials use alpha-to-coverage when MSAA is on, so their edges are smoothed too; the pipeline-state catalogue (ADR-005) carries the sample count and the alpha-to-coverage flag in its keys.
- The frame graph (ADR-004) resolves the scene colour target before the first full-screen pass that samples it (screen transition, tone mapping, brightness).
- The wire-grid edge term (ADR-003) is computed with screen-space derivative smoothing, so wire lines are smooth at every sample count, including Off.
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

- Related: ADR-003 (edge term), ADR-004 (resolve pass), ADR-005 (sample count in catalogue keys), ADR-012 (no multisampled back buffer), ADR-013 (per-layer clamping), ADR-014 (HDR scene format).
- **Amendment 2026-10-18 (owner decision O55).** Since O55 the first playable of the "Confirmed when" line is the MVP's M3 build ([MVP scope](../implementation/mvp_scope.md) §3, §5.3), not the skirmish of O20, and the evidence is gathered on that build: the scene GPU time at 4x is measured in a battle of that build at the figure count of MVP-085 (1,500 figures on screen), on the minimum-capability GPU at 1080p.
- **Amendment 2026-10-22 (owner decision O98).** For the MVP the confirmation's GPU time at 4x is measured on the reference test machine, the owner's PC (O58), not on the minimum-capability GPU, which the MVP does not measure on. The measurement is repeated on the minimum-specification tier before the first external build; until then the default of 4x stands on the reference test machine's evidence.
- **Amendment 2026-10-23 (M3 task T16, owner decision O104, [ADR-174](ADR-174-the-first-playables-confirmations-at-m3.md)).** M3's evidence:
  - the scene's samples per pixel in use are recorded with each rendered scene (`FrameRenderer::SceneSamples`), since RND-442's fallback may take fewer than 4;
  - the measured figure is the frame's GPU time (ADR-148) in the frame bench's `figures-max-drawn`, a battle of `mvp.client.figures_max` figures at 1,920 × 1,080. It is the whole frame's GPU time, and so at least the scene's.

  The owner's T16 performance run records it. The record is confirmed when it fits the frame of `rnd.budget.target_frame_rate` (16.7 ms) at 4x; a miss changes the default by a revision. The pass groups' provisional budgets (DEC-106) are not measured pass by pass in the MVP. The golden-image tests at Off and 4x move to before the first external build (O104), with the measurement on the minimum-specification tier (O98).
