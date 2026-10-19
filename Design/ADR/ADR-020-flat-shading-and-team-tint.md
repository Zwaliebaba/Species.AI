---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Flat shading and team tint

## Context and Problem Statement

Every Species mesh is drawn flat-shaded: each triangle has one colour and one normal, so the low-poly facets read as crisp planes under the two directional lights. Team identity comes from a fixed team palette: parts of buildings, units and effects take their team's colour, and the rest of the mesh keeps its authored colours. Up to the full number of team slots of a match must be distinguishable at once, and colour must never be the only channel that identifies a team (ADR-064).

A rasteriser produces flat shading only if one vertex's values are used for the whole triangle, and which vertex that is depends on API conventions. Team colouring that works by recognising "colours that look like the team colour" in the art breaks as soon as an artist uses a similar colour for something else.

The problem is how the facet look is produced reliably, and how the regions that take the team colour are marked and tinted.

## Decision Drivers

- Exactly one colour and one normal per facet, independent of any provoking-vertex convention.
- Team-coloured regions marked explicitly by the artist, never inferred from colour values.
- One mesh asset serves every team; no per-team copies.
- The tint function and the palette values are visual-language data of the renderer component, not code.
- The team index is available to the shader, so a second identity channel (pattern or marker) can use it.

## Considered Options

1. **Bake one colour and one normal per triangle at cook time**, mark team regions with an **explicit tint weight**, and tint in the shader from a **per-instance team palette index**.
2. Use **`nointerpolation` attributes** with a fixed provoking-vertex convention, without splitting vertices.
3. Produce **per-team copies** of each mesh with the team colours baked in.
4. **Recolour** source colours that fall in a reserved colour range, at cook or run time.

## Decision Outcome

Chosen option: **1, baked facets, explicit tint weight, per-instance palette index**, because it makes the facet look independent of rasteriser conventions and makes team regions an explicit, validated part of the asset.

- **Facets.** Sources omit `NORMAL` for faceted primitives, which glTF defines as implicit flat normals. The cooker splits vertices per triangle and writes, for each triangle, its face normal and its facet colour to all three of its vertices. The facet colour is the colour of the triangle's corners (`COLOR_0`, converted to the working colour space, ADR-013); the validator rejects a faceted triangle whose corners have different colours. No shader depends on which vertex the rasteriser treats as provoking.
- **Smooth exceptions.** A primitive with `NORMAL` is accepted only if its import record declares smooth shading, as a documented exception of the visual-language requirements.
- **Tint weight.** The artist marks team regions in one of two ways, declared in the import record: a per-vertex tint weight in an application-specific glTF vertex attribute (glTF allows these with a leading underscore; the asset pipeline component names it), or a tint flag on a material, which marks every triangle of its primitives with weight 1. The cooker bakes one tint weight per triangle. Textured art uses a single-channel tint mask texture instead (ADR-018).
- **Palette.** Each instance carries a team palette index (ADR-009). The team palette is a constant buffer with one entry per team slot, set per match. The material shader blends the facet colour towards the palette colour by the tint weight, using the tint function that the renderer component defines.
- **Second channel.** The same palette index selects any per-team pattern or marker that the accessibility requirements add (ADR-064), so identity never rests on colour alone.

### Consequences

- Good, because facets are exact on every GPU and in every pass, including debris (ADR-010).
- Good, because one mesh serves every team, and the tint regions are reviewed with the art.
- Good, because changing the palette or the tint function is a data change.
- Bad, because splitting vertices per triangle triples vertex counts compared with indexed meshes; at Species' polygon counts this costs little memory.
- Bad, because artists must paint tint weights or flags deliberately; the validator can check presence but not intent.

### Confirmation

- The content-build validator rejects faceted triangles with differing corner colours, primitives with `NORMAL` and no smooth-shading declaration, and team-tinted assets with no tint weight anywhere.
- A golden-image test renders a reference mesh for every team slot and checks the tinted and untinted regions against the reference images.
- A shader test renders a triangle whose corners carry different values in unused attributes and checks that the facet colour is unaffected.

## Pros and Cons of the Options

### 1. Baked facets, explicit tint weight, palette index

- Good, because the look and the team regions are both explicit data.
- Bad, because vertex counts are higher than with indexed meshes.

### 2. `nointerpolation` with a provoking-vertex convention

- Good, because vertices need not be split.
- Bad, because the facet colour depends on which vertex the convention picks, so every exporter and every mesh edit must respect it, and the edge-mask barycentrics (ADR-003) need split vertices anyway.

### 3. Per-team mesh copies

- Good, because the shader needs no tint logic.
- Bad, because memory and asset counts multiply by the number of team slots, and palette changes need a rebuild.

### 4. Recolouring a reserved colour range

- Good, because artists paint team regions in a recognisable colour.
- Bad, because any other use of a similar colour is tinted by mistake, and nothing can validate the artist's intent.

## More Information

- Technology baseline item 8.1 (flat shading through omitted `NORMAL`; see [the index](README.md#technology-baseline)).
- Related: ADR-003 (material shaders), ADR-009 (per-instance data), ADR-010 (shatter debris), ADR-013 (working colour space), ADR-018 (tint mask textures), ADR-019 (mesh source format), ADR-064 (accessibility).
- The renderer component (`architecture/components/renderer/`) defines the tint function and the team palette as visual-language requirements; the asset pipeline component names the tint attribute.
- **Amendment 2026-10-17.** The asset pipeline component names the vertex attribute `_SPECIES_TINT` and the material flag `species_team_tint` (AST-034, AST-035). An authoring helper in the content tools may propose explicit weights for art made in a key-colour style; the helper writes them into the source, the artist reviews them, and the cooker still never infers a weight from colour values (AST-034). Textured team art (cloth banners and flags) uses a companion tint mask and the shader tint (AST-022).
- **Amendment 2026-10-19 (owner decision O64).** Mesh sources are CMO files (ADR-019 amendment). A CMO vertex always carries a normal, so faceting no longer comes from an omitted `NORMAL`: the cooker ignores the vertex normals and generates facet normals, unless the import record declares a submesh smooth-shaded. Team regions are marked by the material named `team`, which marks every triangle that uses it with weight 1; CMO has no application-specific vertex attribute, so the per-vertex tint weight is not available.
