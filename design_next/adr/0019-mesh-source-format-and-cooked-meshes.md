---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# ADR-0019 Mesh source format and cooked runtime meshes

## Context and Problem Statement

Species meshes are low-poly, flat-shaded and vertex-coloured: buildings, turrets, creatures, crates, scenery and the parts that move (turret heads, barrels, dish arms, legs). Each mesh is a hierarchy of rigid nodes, so code can turn or move one part, and carries named attachment points (sockets, ADR-0021) for effects, ports, barrels, lights and carried objects. Colour is per facet, and team-coloured regions must be marked explicitly (ADR-0020). Every mesh is newly authored (vision brief §5) in a current digital content-creation tool.

Meshes also feed the simulation indirectly: buildings and trees need collision volumes. That data must never be a by-product of how a mesh is drawn, because presentation never produces simulation state.

The problem is which source format artists deliver, what the runtime loads, and how render geometry and simulation-relevant data are kept apart.

## Decision Drivers

- An open, current interchange format that every mainstream authoring tool exports.
- Flat shading and per-facet colour expressible in the source without workarounds.
- A runtime format that loads without parsing or conversion.
- Node hierarchy and sockets preserved by name and validated.
- Strict validation at build time, with errors that name the asset.
- Collision and other simulation-relevant data are separate assets.

## Considered Options

1. **glTF 2.0** (`.glb`) sources, validated and cooked into a **Species runtime mesh format**.
2. **FBX** sources cooked into a Species runtime format.
3. A **Species text mesh format** authored through an exporter plug-in.
4. **USD** sources cooked into a Species runtime format.

## Decision Outcome

Chosen option: **1, glTF 2.0 sources cooked into a Species runtime format**, because glTF is an open standard whose core already defines implicit flat normals and vertex colours, and a cooked format keeps all parsing and validation out of the runtime.

- **Sources.** Checked-in mesh sources are binary glTF 2.0 (`.glb`). Each source has an import record (in the schema of ADR-0061) that declares its asset id, its unit scale from glTF metres to world units, its socket schema (ADR-0021), and its tint mask (ADR-0020).
- **Extensions.** Allowed: `KHR_materials_unlit` (glow and UI meshes), `KHR_materials_emissive_strength` (emissive above 1.0 for additive glow), `KHR_mesh_quantization` (compact positions). The cooker rejects every other extension unless an ADR adds it. Compression extensions are not used in sources.
- **Validation.** The cooker validates every source against the glTF 2.0 specification (for example with the Khronos glTF validator) and then against Species rules: allowed extensions only; materials that map onto the material shader set of ADR-0003; node names unique within the mesh; sockets matching the socket schema; triangle primitives only.
- **Axes.** glTF is right-handed with +Y up and front faces along +Z; the cooker converts to Species world axes and units once (ADR-0016).
- **Cooked format.** The runtime loads a versioned Species mesh (`MeshAsset`): a hierarchy of mesh nodes (`MeshNode`), each with its rest transform, bounding box, draw range and per-triangle data; the socket table (`MeshSocket`, ADR-0021); and vertex buffers laid out for the material shaders (position, facet colour, facet normal, texture coordinates where used, barycentric coordinates where the edge mask is used). The runtime never parses glTF.
- **Animation data.** glTF skins, morph targets and animation channels are not imported. Moving parts are animated by procedural code that sets node transforms (ADR-0035). Adding skeletal or keyframed animation needs an ADR.
- **Collision and simulation data.** Collision volumes and every other simulation-relevant shape are separate assets with their own ids, cooked into simulation data (the content build's collision asset, ADR-0059). Render geometry, including generated tree geometry, never produces simulation data.

### Consequences

- Good, because any current authoring tool that exports glTF 2.0 can produce Species meshes.
- Good, because the cooked format loads directly into GPU buffers.
- Good, because node names and sockets are checked at build time.
- Good, because collision data cannot drift with render-mesh edits.
- Bad, because the cooker and its validation rules are engine work.
- Bad, because animators cannot deliver keyframed or skinned animation without a new decision.

### Confirmation

- The content-build validator rejects sources with disallowed extensions, unsupported material features, duplicate node names, non-triangle primitives, or sockets that do not match the schema, and names the asset and node.
- A cooker test round-trips a reference mesh (glTF in, cooked out, read back) and checks the node hierarchy, transforms, socket positions, facet colours and bounding boxes.
- A content-build check fails when a collidable mesh has no collision asset.

## Pros and Cons of the Options

### 1. glTF 2.0

- Good, because it is an open Khronos standard with a reference validator.
- Good, because implicit flat normals and `COLOR_0` express the look in core glTF.
- Bad, because some authoring tools' glTF exporters differ in details, which the validator must catch.

### 2. FBX

- Good, because many authoring tools export it.
- Bad, because it is a proprietary format whose reading needs a vendor SDK, and its flat-shading and vertex-colour handling varies by exporter.

### 3. Species text mesh format with an exporter plug-in

- Good, because it can express exactly what Species needs.
- Bad, because the plug-in ties authoring to one tool and must be maintained for every version of that tool.

### 4. USD

- Good, because it is open and suited to large scene assembly.
- Bad, because it is far larger than Species' single-mesh needs, and its tooling and validation add weight for no gain at this scale.

## More Information

- Technology baseline item 8.1 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://raw.githubusercontent.com/KhronosGroup/glTF/main/specification/2.0/Specification.adoc · https://github.com/KhronosGroup/glTF/tree/main/extensions.
- Related: ADR-0003 (material shader set), ADR-0016 (axes and units), ADR-0020 (flat shading and team tint), ADR-0021 (socket schema), ADR-0035 (procedural animation), ADR-0059 (content build), ADR-0061 (data schemas).
