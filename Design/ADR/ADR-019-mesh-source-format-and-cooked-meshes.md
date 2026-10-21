---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Mesh source format and cooked runtime meshes

## Context and Problem Statement

Species meshes are low-poly, flat-shaded and vertex-coloured: buildings, turrets, creatures, crates, scenery and the parts that move (turret heads, barrels, dish arms, legs). Each mesh is a hierarchy of rigid nodes, so code can turn or move one part, and carries named attachment points (sockets, ADR-021) for effects, ports, barrels, lights and carried objects. Colour is per facet, and team-coloured regions must be marked explicitly (ADR-020). Every mesh is newly authored (vision brief §5) in a current digital content-creation tool.

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

- **Sources.** Checked-in mesh sources are binary glTF 2.0 (`.glb`). Each source has an import record (in the schema of ADR-061) that declares its asset id, its unit scale from glTF metres to world units, its socket schema (ADR-021), and its tint mask (ADR-020).
- **Extensions.** Allowed: `KHR_materials_unlit` (glow and UI meshes), `KHR_materials_emissive_strength` (emissive above 1.0 for additive glow), `KHR_mesh_quantization` (compact positions). The cooker rejects every other extension unless an ADR adds it. Compression extensions are not used in sources.
- **Validation.** The content build tool's glTF importer, Species code, validates every source itself; no third-party validator is used (O30). It checks, in this order, and rejects the asset at the first failed check of each element:
  - **container:** the GLB header has the magic `0x46546C67`, version 2 and a length equal to the file size; the first chunk is JSON and the only other chunk, when present, is BIN; every chunk starts and ends on a 4-byte boundary and lies inside the file;
  - **JSON:** the JSON chunk passes the strict authoring parser of [ADR-062](ADR-062-authoring-syntax-and-schema-language.md) (UTF-8, no duplicate keys); `asset.version` is `"2.0"`; the only buffer is the BIN chunk, with no `uri`, and its `byteLength` is at most the chunk's length;
  - **required properties:** every object has the properties the specification requires (an accessor its `componentType`, `count` and `type`; a buffer view its `buffer` and `byteLength`; a mesh its `primitives`; a primitive its `attributes` with `POSITION`), and every index into an array (node, mesh, accessor, buffer view, buffer, material) is in range;
  - **bounds:** a buffer view lies inside its buffer; its `byteStride`, when present, is a multiple of 4 from 4 to 252; an accessor's byte offset, and that offset plus its buffer view's, is a multiple of its component size, and a multiple of 4 for a vertex attribute; offset + (count − 1) × stride + element size is at most the buffer view's length;
  - **component types:** `POSITION` is a `VEC3` of float32, or of an integer type that `KHR_mesh_quantization` adds, with `min` and `max` present; `COLOR_0` is a `VEC3` or `VEC4` of float32, unorm8 or unorm16 with every component in [0, 1]; `TEXCOORD_n` is a `VEC2` of float32, unorm8 or unorm16 (or the quantized types); indices are a `SCALAR` of uint8, uint16 or uint32; every other combination is rejected;
  - **index ranges:** every index is below the vertex count of its primitive and is not the largest value of its component type; the primitive mode is triangles and the index count, or the vertex count without indices, is a multiple of 3;
  - **values and hierarchy:** every float is finite; the node hierarchy is a set of disjoint trees, with no cycle and at most one parent per node;
  - **extensions:** `extensionsUsed` and `extensionsRequired` name only the three allowed extensions, and every value of `extensionsRequired` is also in `extensionsUsed`;
  - **Species rules:** materials that map onto the material shader set of ADR-003; node names unique within the mesh; sockets matching the socket schema.
  The error names the asset id, the JSON pointer of the element (or the byte offset in the container), the rule and the value found.
- **Axes.** glTF is right-handed with +Y up and front faces along +Z; the cooker converts to Species world axes and units once (ADR-016).
- **Cooked format.** The runtime loads a versioned Species mesh (`MeshAsset`): a hierarchy of mesh nodes (`MeshNode`), each with its rest transform, bounding box, draw range and per-triangle data; the socket table (`MeshSocket`, ADR-021); and vertex buffers laid out for the material shaders (position, facet colour, facet normal, texture coordinates where used, barycentric coordinates where the edge mask is used). The runtime never parses glTF.
- **Animation data.** glTF skins, morph targets and animation channels are not imported. Moving parts are animated by procedural code that sets node transforms (ADR-035). Adding skeletal or keyframed animation needs an ADR.
- **Collision and simulation data.** Collision volumes and every other simulation-relevant shape are separate assets with their own ids, cooked into simulation data (the content build's collision asset, ADR-059). Render geometry, including generated tree geometry, never produces simulation data.

### Consequences

- Good, because any current authoring tool that exports glTF 2.0 can produce Species meshes.
- Good, because the cooked format loads directly into GPU buffers.
- Good, because node names and sockets are checked at build time.
- Good, because collision data cannot drift with render-mesh edits.
- Bad, because the cooker and its validation rules are engine work, the specification checks of the importer included, and they cover only the subset of glTF that Species imports.
- Bad, because animators cannot deliver keyframed or skinned animation without a new decision.

### Confirmation

- The content-build validator rejects sources with disallowed extensions, unsupported material features, duplicate node names, non-triangle primitives, or sockets that do not match the schema, and names the asset and node.
- The importer's test suite has one failing fixture per check of the validation list (a bad GLB header, a misaligned chunk, a missing required property, an index out of range, a buffer view past its buffer, a stride of 2, a misaligned accessor, an accessor past its buffer view, a `POSITION` without `min`, a disallowed component type, an index at the restart value, a `COLOR_0` value above 1, a non-finite float, a node cycle, an unknown required extension), each reporting the asset, the JSON pointer or byte offset and the rule; valid exports of the content-creation tools the artists use pass.
- A cooker test round-trips a reference mesh (glTF in, cooked out, read back) and checks the node hierarchy, transforms, socket positions, facet colours and bounding boxes.
- A content-build check fails when a collidable mesh has no collision asset.

## Pros and Cons of the Options

### 1. glTF 2.0

- Good, because it is an open Khronos standard whose specification states every structural rule normatively.
- Good, because implicit flat normals and `COLOR_0` express the look in core glTF.
- Bad, because some authoring tools' glTF exporters differ in details, which the importer's own checks must catch.

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
- The checks of the validation list, checked 2026-10-18 against the specification above: the GLB magic, version, length, chunk order and 4-byte chunk alignment; the `byteStride` range and multiple; the accessor alignment and fit; `min` and `max` on `POSITION`; the attribute and index component types; the forbidden maximum index value; the `COLOR_0` range; `extensionsRequired` as a subset of `extensionsUsed`; and a node hierarchy of disjoint strict trees.
- **Amendment 2026-10-17 (`AGENTS.md` §2; superseded by the O30 amendment below).** The Khronos glTF Validator is pinned at 2.0.0-dev.3.10 with its Apache 2.0 licence as a dependency of the content build tool, and runs as a separate program that never links into an executable.
- **Amendment 2026-10-18 (O26, owner decision O29).** The content build tool is a tool project of the one solution ([ADR-054](ADR-054-build-system-msbuild-solution.md)).
- **Amendment 2026-10-18 (owner decision O30).** A tool project's third-party dependency is restored by NuGet only, the build downloads nothing from another source and no third-party binary is committed. The Khronos glTF Validator has no NuGet package, so it is not a dependency: the content build tool's glTF importer validates what it imports itself, with the checks of the validation list, and rejects the asset with a precise error. The validator's pin, its licence, its archive, the archive's hash and the build-time download are removed, and so are the validator rows of the technology baseline.
- **Amendment 2026-10-19 (owner decision O64).** **Mesh sources are CMO files, in place of glTF 2.0.** The owner authors meshes as binary CMO files of the Visual Studio mesh content pipeline, flat in `GameData/Meshes/`; the cooked runtime format, the import records, the collision assets and the rule that the runtime never parses a source stand. A CMO file holds meshes, each with its name, materials, submeshes, 16-bit index buffers and vertex buffers of 52-byte vertices (position, normal, tangent, an `R8G8B8A8_UNORM` colour with red in the low byte, texture coordinates), skinning buffers, its extents, and, when it has a skeleton, bones (name, parent index, inverse bind pose, bind pose, local transform) and animation clips. The importer, Species code with no third-party reader, maps it as follows. **Nodes** are the bones that are not sockets: a bone's parent index gives the hierarchy and its local transform the rest transform, and a mesh attaches to the bone of its own name; a file with no skeleton is one node. **Sockets** are bones named with the prefix `socket_` and no mesh, children of the node they move with (ADR-021). **Faces** are the triangles of the submeshes; the vertex normals are ignored and facet normals generated, unless the import record declares a submesh smooth-shaded (ADR-020). **Colour** is the vertex colour, sRGB-encoded as authored (ADR-013); a material's diffuse colour must be white. **Materials** map onto the shader set of ADR-003 by name and fields: the material named `team` marks its triangles with tint weight 1 (ADR-020), `unlit` selects the unlit shader, the specular colour and power give the lit shader's specular term, the emissive colour its emissive term, and texture names, empty for a vertex-coloured mesh, name the textures sampled. **Validation** replaces the glTF checks of the list above, in this order, rejecting the asset at the first failed check and naming the asset, the byte offset and the rule: every count and string length leads to data inside the file, and reading ends exactly at its end; every string is UTF-16 of the length stated; every submesh's material, index buffer and vertex buffer exist, and its index range lies inside its index buffer; every index is below its vertex buffer's count, and the index count is a multiple of 3; every float is finite; every bone's parent index is −1 or a bone before it, so the bones form disjoint trees; bone and mesh names are unique, and every mesh's name is a non-socket bone's when the file has a skeleton; skinning buffers and animation clips are absent (ADR-035). Axes: CMO is right-handed with +Y up and counter-clockwise front faces, as both of the owner's files measure (all 238 of their triangles wind counter-clockwise about their stored normals, checked on 2026-10-19), so ADR-016's conversion is the unit scale alone, from source units to world units. The confirmation tests read CMO in place of glTF: one failing fixture per check of this list, and the round trip of a reference CMO mesh.
- **Amendment 2026-10-20 (M1 task T11, [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md); owner decisions O69, O73).** The CMO importer and the mesh cooker are built in the content build tool, with the checks of the O64 amendment in its order: a string is the units its length states ending in its one NUL, and at most one mesh carries the skeleton. The cooked `MeshAsset` is version 1 in the byte layout of ADR-108: materials, nodes with their draws and bounds, sockets in table order and 32-byte vertices. M1 has no import records, so every mesh is cooked at one world unit per source unit and its team regions come from the material named `team`. Three sources are refused until the work that settles them: a node with geometry and a rest transform other than the identity, a simulation-relevant socket, whose simulation data record comes with M2, and a collision asset. The confirmation's failing fixtures, one per check, and the round trip are `Build\CheckContentBuild.py` and `NeuronCoreTests`.
- **Amendment 2026-10-21 (M2 tasks T2 and T3, [ADR-128](ADR-128-mesh-kit-and-drafted-structure-recipes.md), [ADR-129](ADR-129-simulation-data-records-and-collision-cooker.md)).** The MVP's structure meshes and their collision assets are CMO files that the mesh kit, a tool project, writes from recipes, and they are committed like any authored mesh. The content build cooks a collision asset into a `Collision` entry, and checks it against its render member. A collision source carries one material, `collision`, which nothing reads, because the CMO layout needs one. A socket carrier with no geometry, the turret mount, is one node with no triangles, and its row states "no triangles budget; 1 node, depth 1".
- Related: ADR-003 (material shader set), ADR-016 (axes and units), ADR-020 (flat shading and team tint), ADR-021 (socket schema), ADR-035 (procedural animation), ADR-059 (content build), ADR-061 (data schemas).
