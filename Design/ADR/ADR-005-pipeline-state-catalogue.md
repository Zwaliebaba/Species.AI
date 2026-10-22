---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Pipeline state catalogue derived from the pass table

## Context and Problem Statement

Every Species draw combines a material shader (ADR-003) with a blend, depth, rasteriser and target configuration. The look relies on a small, fixed set of those combinations: opaque facets, alpha-blended panels, additive glow and text, multiplicative darkening layers and outlines, the brightening blend of placement ghost models, depth-tested but not depth-written effects, screen-facing text drawn without depth, and decals and shadows that need a depth bias. Direct3D 12 bakes all of these into immutable pipeline state objects (PSOs), and creating one can take milliseconds.

Species needs every combination it draws to exist before it is needed, so that no frame stalls on creation and no draw silently falls back to a different state. The frame begins from one documented default state, and every pass is a documented difference from it.

The problem is how the set of pipeline states is defined, created and validated.

## Decision Drivers

- The ordered pass table (ADR-004) is the single source of truth for blend, depth and cull combinations.
- No pipeline state is created during a frame; no draw falls back silently to another state.
- One default state from which every pass's differences are stated explicitly.
- Start-up time: states are created once per run and cached across runs.
- Reviewable: the full list of states is small enough to read.

## Considered Options

1. A **build-time catalogue** derived from the pass table and the material permutations, with every entry created at start-up.
2. **On-demand creation** at first use, with a run-time hash cache.
3. The **full product of all state axes**, created up front.

## Decision Outcome

Chosen option: **1, a build-time catalogue derived from the pass table**, because it guarantees that every state a frame can use exists before the first frame, and nothing else does.

**Default state.** Every catalogue entry starts from these values and lists only its differences:

| Group | Default |
|---|---|
| Rasteriser | solid fill; back faces culled; a triangle whose vertices appear counter-clockwise on the render target is front-facing (`FrontCounterClockwise = TRUE`); depth clip on; no depth bias |
| Depth | test on, write on; comparison equivalent to less-or-equal in the depth convention of ADR-016 |
| Stencil | off (no pass uses stencil) |
| Blend | off |
| Alpha cut-out | off; when a material enables it, the pixel is kept only if its alpha is greater than 0.01 |
| Fog | parameters set per frame; off unless the pass enables it |
| Primitive topology | triangle lists; quads are two triangles with the shared index pattern 0, 1, 2, 0, 2, 3 |

**Blend states** available to catalogue entries:

| Name | Source factor | Destination factor | Used for |
|---|---|---|---|
| opaque | — | — | facets, terrain, opaque scenery |
| alpha | `SRC_ALPHA` | `INV_SRC_ALPHA` | alpha-blended buildings and panels |
| premultiplied | `ONE` | `INV_SRC_ALPHA` | textures whose alpha mode is premultiplied (ADR-018) |
| additive | `SRC_ALPHA` | `ONE` | glow, sprites, particles, normal text (ADR-031) |
| darkening | `ZERO` | `INV_SRC_COLOR` | darkening layers, dark smoke, text outlines and shadows |
| ink | `SRC_ALPHA` | `INV_SRC_COLOR` | UI content whose darkening also depends on source alpha |
| ghost | `ONE` | `SRC_COLOR` | placement ghost models of the world cursor (the destination is brightened in proportion to the source colour) |

Darkening is order-independent (each layer multiplies the destination by one minus the source colour), so darkening passes are never sorted. An ink draw whose source alpha is 0 is arithmetically a darkening draw; the renderer component maps it to the darkening state (ADR-031).

**Depth variants:** test and write; test without write (transparent, additive and darkening passes); neither test nor write (UI and screen-facing text); and depth-biased variants for decals, shadows and overlays, which use the pipeline's `DepthBias` and `SlopeScaledDepthBias` instead of any change to the projection.

**Cull variants:** back-face culling, or none (UI, text, sprite cards, two-sided meshes).

**Catalogue rules.**

- A catalogue entry is keyed by a hash of its full descriptor: shader permutation identifiers, blend, depth and rasteriser state, render-target formats, sample count and topology type. Keys are generated at build time from the pass table and the material permutation list.
- Every entry is created during start-up or the loading screen. Creating a pipeline state at any other time is an error in Debug builds and is logged in Release builds.
- A draw names exactly one catalogue key. A key that is not in the catalogue is a build error, never a fallback.
- Dynamic state is limited to viewport, scissor rectangle, blend factor and primitive topology.
- Created states are stored in a pipeline library under the user's local data folder and reloaded on the next start. The library is discarded when the driver, the Agility SDK version or the shader build changes.
- Configuration changes that alter target formats or sample counts (HDR, anti-aliasing) create the affected entries during the reconfiguration, behind a loading indicator.

### Consequences

- Good, because no frame can stall on pipeline creation, and no draw can run with a state nobody reviewed.
- Good, because the whole set of states is listed in one generated file that a reviewer can read.
- Good, because the default-plus-differences form makes each pass's state self-documenting.
- Bad, because a new combination needs a pass-table change and a rebuild, not just a code change in a sub-renderer.
- Bad, because target-format changes multiply the catalogue (for example SDR and HDR scene formats), which increases start-up work on a cold cache.

### Confirmation

- The build generates the catalogue and fails when a pass references a state that no entry provides, or when an entry is referenced by no pass.
- A development-build assertion fires on any pipeline creation outside start-up, loading or reconfiguration; a CI soak test runs a full match with it enabled.
- Golden-image tests cover each blend state, including a stack of darkening layers drawn in two different orders that must produce the same image.
- A start-up timing test measures cold-cache and warm-cache creation time of the full catalogue on the minimum-capability GPU.

## Pros and Cons of the Options

### 1. Build-time catalogue from the pass table

- Good, because completeness is checked at build time.
- Good, because the catalogue mirrors the documented pass table one to one.
- Bad, because flexibility for experiments is lower: every new state goes through the table.

### 2. On-demand creation with a run-time cache

- Good, because nothing needs to be listed in advance.
- Bad, because the first use of each state stalls a frame, which shows as hitches when new effects appear.
- Bad, because nothing proves that the set of states a player can reach is the set that was tested.

### 3. Full product of all state axes

- Good, because every combination exists.
- Bad, because the product of shaders, blend, depth, cull and target formats is far larger than the set Species uses, which wastes start-up time and memory for states no pass draws.

## More Information

- Related: ADR-003 (material shaders and permutations), ADR-004 (frame graph and pass table), ADR-014 (HDR formats), ADR-015 (sample counts), ADR-016 (depth convention), ADR-018 (alpha modes), ADR-031 (UI blend states).
- The renderer component (`architecture/components/renderer/`) lists the ordered pass table with each pass's catalogue keys.
- **Amendment 2026-10-17 (AGENTS.md, O21).** Pipeline creation outside start-up is an error in Debug builds and is logged in Release builds, the two configurations of ADR-054.
- **Amendment 2026-10-17.** The blend list gains the `ghost` state (`ONE`, `SRC_COLOR`) that the renderer component uses for the world cursor's placement ghost models (RND-058, RND-365). The front-facing rule is stated on the render target, as the renderer's default state gives it (RND-055).
- **Amendment 2026-10-20 (M1 task T13, owner decision O75, [ADR-113](ADR-113-the-frame-at-m1.md)).** M1 creates the catalogue without a pipeline library: every entry is created with the device objects, at start-up and after a rebuild, at the scene's sample count. The entries' alpha factors keep the destination alpha (`ZERO`, `ONE`), because an alpha factor may not name a colour. The M1 form ends when the catalogue's creation is measured over its share of the start-up, which the start-up measurement of a later milestone reports.
- **Amendment 2026-10-21 (M1 task T16c, [ADR-123](ADR-123-weapons-effects-and-particles-at-m1.md)).** An entry's depth-bias variant (RND-219) is its rasteriser's `DepthBias`, in RND-220's units, with no slope and no clamp. The catalogue is created before the client reads its records, so `bias_laser` is compiled in as `DEPTH_BIAS_LASER`, and a test holds it to `rnd.depth_bias.laser`.
- **Amendment 2026-10-22 (M2 task T17, [ADR-153](ADR-153-the-settlement-verbs-at-m2.md)).** The `ghost` state is built as `BlendState::Ghost`, and its entry `cursor_ghost` draws the placement ghost in pass 12 at M2, depth-tested by the hardware with no depth write, until pass 20's world-space part exists (RND-365).
