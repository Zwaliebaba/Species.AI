---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Texture import manifest: colour space, alpha modes, mips, samplers and tint masks

## Context and Problem Statement

A texture's correct processing depends on what it is: colour art is sRGB-encoded and data is linear; some art blends with straight alpha, some is premultiplied, some is drawn additively (where black means "no contribution" and alpha is irrelevant), and some is cut out at an alpha threshold; UI art drawn at 1:1 needs no mips and must not be filtered; terrain and water layers repeat, while sprites must not bleed at their edges; and team-coloured art needs a mask that says where the team tint applies. Some art also exists in derived forms, such as a team-tinted variant or a blurred shadow.

If these choices are made where a texture is used, two users of the same texture can ask for different processing, and whichever loads it first decides for both. Species needs every processing choice made once per asset, in data, before the game runs.

The problem is where texture processing is declared, which choices it covers, and how the runtime identifies a processed texture.

## Decision Drivers

- One declaration per texture asset; no processing decided at a call site or by load order.
- Colour space and alpha mode explicit, because the working colour space (ADR-0013) and the blend states (ADR-0005) depend on them.
- Real alpha channels; no colour value or colour channel stands in for transparency.
- Derived variants are explicit assets, cooked offline.
- Validated at build time with errors that name the asset.

## Considered Options

1. A **per-asset import manifest** that declares every processing choice, with the runtime texture cache keyed by asset id and processing variant, and derived variants cooked offline.
2. **Processing flags passed by each call site** when the texture is requested.
3. **File-name conventions** (suffixes) that encode processing.

## Decision Outcome

Chosen option: **1, a per-asset import manifest**, because it is the only option in which a texture's processing is decided once, reviewed, and validated before the game runs.

- **Manifest.** Each texture source has an import record stored next to it, in the texture-import schema of the data-schema decisions (ADR-0061). It declares:
  - the **kind**, which selects the format (ADR-0017);
  - the **colour space**: `srgb` for colour art, `linear` for masks, ramps and data;
  - the **alpha mode**: `opaque`, `straight`, `premultiplied`, `additive` (colour is added, alpha is ignored, and black contributes nothing), or `cutout` with a threshold;
  - the **mip policy**: `full`, or `none` for art drawn only at 1:1;
  - the **sampler** from the closed set below;
  - the **tint mask**: none, or the companion mask texture whose single channel gives the team-tint weight (ADR-0020);
  - the **pixel-exact** flag for UI art that must not be compressed or filtered;
  - the **derived variants** the cooker must produce, each with its own asset id.
- **Alpha.** Sources carry real alpha channels where their alpha mode needs one; the validator rejects a source whose alpha mode needs alpha and which has none. Premultiplied textures are premultiplied by the cooker before mips are generated. Cut-out textures generate mips that preserve alpha-test coverage at the declared threshold. The additive art style comes from the additive blend state, never from black backgrounds standing in for transparency.
- **Blend pairing.** The material that uses a texture takes its blend state from the alpha mode: `straight` with the alpha blend, `premultiplied` with the premultiplied blend, `additive` with the additive blend, and `cutout` with the opaque state and alpha cut-out (ADR-0005). A material that pairs them differently is a build error.
- **Samplers.** The closed sampler set, held in the sampler heap at fixed indices (ADR-0006), is: **linear clamp-to-edge** (the default), **linear wrap** (repeating terrain, water and sky layers), and **point clamp-to-edge** (pixel-exact UI art). Clamp-to-edge never blends a border colour into edge texels.
- **Cache key.** The runtime texture cache (`AssetCache`) keys every texture by **(asset id, processing variant)**. A request can name a variant only if the manifest declares it, so no requester can change another requester's texture.
- **Derived variants.** Team-tinted variants (where a tint cannot be applied in the shader), blurred-shadow variants and any other derivative are produced offline by the cooker as separate assets, never at load.

### Consequences

- Good, because every texture's processing is visible in one reviewed file and cannot depend on load order.
- Good, because the alpha mode, blend state and colour space always agree.
- Good, because derived variants cost nothing at load.
- Bad, because every texture needs a manifest record, which adds authoring work (mitigated by defaults per kind).
- Bad, because adding a sampler means a revision of this ADR and of the shared sampler indices.

### Confirmation

- The content-build validator rejects: a texture without a manifest; an unknown kind, alpha mode, sampler or variant; an alpha mode that needs alpha on a source without alpha; and a material whose blend state does not match its texture's alpha mode.
- A unit test of the texture cache checks that requests for the same asset with different declared variants return different resources, and that a request for an undeclared variant fails.
- Golden-image tests cover one texture of each alpha mode, including cut-out coverage at the smallest mip.

## Pros and Cons of the Options

### 1. Per-asset import manifest

- Good, because processing is data, reviewed and validated.
- Good, because it removes every load-order dependency.
- Bad, because it adds a record per texture.

### 2. Call-site processing flags

- Good, because no extra file is needed.
- Bad, because two call sites can request the same texture differently, and the result depends on which loads first.
- Bad, because processing happens at load on the player's machine.

### 3. File-name conventions

- Good, because the processing is visible in the file list.
- Bad, because a name can carry only a few choices, is easy to get wrong, and cannot be validated as strictly as a schema.

## More Information

- Related: ADR-0005 (blend states), ADR-0006 (sampler heap), ADR-0013 (working colour space), ADR-0017 (formats), ADR-0020 (team tint), ADR-0059 (content build and validators), ADR-0061 (data schemas).
- The asset pipeline component (`architecture/components/asset_pipeline_and_resources.md`) specifies the texture-import record and the defaults per kind.
