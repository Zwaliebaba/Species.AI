---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Mesh socket schema

## Context and Problem Statement

Code attaches behaviour and effects to named points on meshes: the ports where Workers man a building, the ends of barrels where shots and muzzle flashes start, lights and glow points, status indicators, the points where carried objects sit, the feet of legged creatures, engine and exhaust points, entrances and exits, flag mounts, connection points of the energy grid and fuel lines, particle emitters and more. Some of these points only place effects; others (port and entrance positions, for example) decide where units stand and therefore feed the simulation. Points on moving parts must follow the part, such as a barrel end on a turning turret head. Where several points of one kind exist, their order can carry meaning (port order, for example).

If code finds these points by searching for names at run time, a renamed or missing point is discovered only when the game runs, and nothing guarantees that a mesh has the points its code needs.

The problem is how attachment points are authored, validated, stored and referenced, and how the simulation-relevant ones reach the simulation.

## Decision Drivers

- Code references attachment points by a stable id, never by a string search at run time.
- Every mesh is checked at build time against the points its asset class requires.
- Points on moving parts follow their parent node.
- Order is explicit where it carries meaning.
- Simulation-relevant points reach the simulation as simulation data, never through render meshes.

## Considered Options

1. **Socket nodes authored in the glTF source**, validated against a **socket schema per asset class**, and cooked into a **socket table** that code indexes by (kind, index).
2. Sockets in a **separate sidecar file** per mesh, edited by hand or by a tool.
3. **Run-time lookup** of glTF node names.

## Decision Outcome

Chosen option: **1, socket nodes, schema per asset class, cooked socket table**, because it keeps sockets in the artist's scene where they can be placed visually, and moves every check to the build.

- **Authoring.** A socket is a glTF node without a mesh, placed as a child of the node it moves with. Its name has a reserved socket prefix, a socket kind and an index; the asset pipeline component defines the exact naming pattern.
- **Socket kinds.** Socket kinds form a closed Species enumeration (the `MeshSocket` kind) defined in the asset pipeline component. Each kind states whether its order carries meaning and whether it is simulation-relevant.
- **Schema per asset class.** Each asset class (for example a building type, a turret type, a creature) has a socket schema that lists the kinds it requires, with an exact count or a range. The mesh's import record names its asset class (ADR-019).
- **Validation.** The cooker rejects a mesh whose sockets do not match its schema: a missing or surplus socket, an unknown kind, a duplicate, or a gap in the indices of a kind (indices start at 0 and are contiguous). Errors name the asset, the kind and the index.
- **Cooked table.** The cooked mesh (`MeshAsset`) carries a socket table: for each kind, the sockets in index order, each with its parent node and its transform relative to that node. Code asks for (kind, index) and receives an array entry; the world transform is the parent node's current transform times the socket's.
- **Simulation-relevant sockets.** For kinds marked simulation-relevant, the cooker also writes the socket positions, in the asset's rest pose, into the asset's simulation data record, which the content identity covers (ADR-060). The simulation reads only that record, never the render mesh.

### Consequences

- Good, because a missing or misnamed socket is a build error, not a run-time fault.
- Good, because code has no string handling for sockets and no search cost.
- Good, because sockets on moving parts follow their parts automatically.
- Good, because the simulation never depends on render data, and a change to a simulation-relevant socket changes the content identity.
- Bad, because every new asset class needs a schema, and every new socket kind is a change to the enumeration.

### Confirmation

- The content-build validator runs the schema check on every mesh and fails the build on any mismatch.
- A cooker test checks a reference mesh's socket table: kinds, indices, parent nodes and transforms, including a socket on a rotated child node.
- A content-build check confirms that every simulation-relevant socket kind appears in the simulation data record of each asset that has it, and that changing such a socket changes the asset's content hash.

## Pros and Cons of the Options

### 1. Socket nodes, schema, cooked table

- Good, because sockets are placed visually and validated strictly.
- Bad, because schemas and the kind enumeration must be maintained.

### 2. Sidecar socket files

- Good, because sockets can be edited without opening the mesh.
- Bad, because positions typed outside the scene drift when the mesh is edited, and the two files can disagree.

### 3. Run-time name lookup

- Good, because nothing needs to be declared in advance.
- Bad, because missing sockets are found only in play, and string searches cost time on every lookup.

## More Information

- Related: ADR-019 (mesh source format and asset classes), ADR-035 (procedural animation of nodes), ADR-036 (effects attached to sockets), ADR-059 (content build), ADR-060 (content identity).
- The asset pipeline component (`architecture/components/asset_pipeline_and_resources.md`) defines the socket kinds, the naming pattern and the schema of every asset class.
- **Amendment 2026-10-19 (owner decision O64).** Mesh sources are CMO files (ADR-019 amendment). A socket is a bone of the CMO skeleton named with the socket prefix and carrying no mesh, a child of the bone it moves with, as `socket_flag` of `GameData/Meshes/LeaderFigure.cmo` is; the naming pattern, the schema and the cooked socket table stand.
