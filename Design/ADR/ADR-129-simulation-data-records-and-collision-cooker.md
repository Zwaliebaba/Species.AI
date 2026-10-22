---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M2 task T3)
---

# The simulation data records and the collision cooker

## Context and Problem Statement

The simulation reads a structure's ports, door, entrance, muzzles and hit volumes from its mesh's simulation data record, never from the render mesh (AST-043, AST-046). Its sphere, mesh and ray tests read the structure's collision asset, which is its own authored source (AST-044, R-BLDG-041). M1 had no structure, so its content build refused all three things M2's meshes bring (ADR-108):
- a simulation-relevant socket;
- a collision row;
- a node with geometry and a rest transform other than the identity.

M2 task T3 builds the records and the collision cooker, checks the references they make (AST-089, AST-090), and has the server start from the cooked records. With no import records yet (O73), four things need deciding:
- how a render mesh knows its collision asset;
- the records' key and layout;
- how a socket carrier with no geometry is spelt;
- what the server loads.

## Decision Drivers

* The record is the simulation's only view of a mesh, and the content identity covers it (AST-098, AST-100). A changed socket changes the identity, as ADR-021's confirmation asks.
* A collidable mesh without its collision asset, and a collision asset without its render member, are build errors with the file and the fault (AST-044, AST-093).
* No new source format or authored file where the manifest already states the relation.
* The cooked formats are versioned, read whole, and refuse anything outside their layout (AST-091).

## Considered Options

1. **Pair by the manifest's asset ids.** `mesh.collision.<name>` is the collision asset of the one mesh asset `mesh.<class>.<name>`, member by member. Each render mesh with a simulation-relevant socket or a collision asset gets a record keyed `<mesh key>@sim`.
2. **Import records** naming each mesh's collision asset (AST-033), written as authored JSON beside the meshes.
3. **One record for all structures**, a table cooked from the manifest.

## Decision Outcome

Chosen option: 1. The manifest already names every collision asset as `mesh.collision.<name>` of the render asset it serves, with members in the same order (AST-016), so the pairing needs no new file. The import records of option 2 come with the asset cache, which M2 does not build, and would restate what the manifest says. Option 3 would make one record change with every structure, and break AST-046's one record per asset.

* **The pairing pass.** It runs after every source is cooked (`SimulationRecordCooker`).
  - A render mesh is collidable when the manifest has a collision asset of its name with a member at its member's place. Its collision source must then be authored: if it is not, the render file is refused (AST-044).
  - A collision source whose render member has no authored file is refused (AST-090).
  - So is a collision node whose name is no node of its render member (AST-077).
  - The render asset of `mesh.collision.<name>` is the one mesh asset, not a collision asset, whose id ends in `.<name>`. Two such assets are refused as ambiguous.
* **The collision cooker** (`CollisionCooker`, AST-077). It imports the CMO with ADR-019's checks and reads every triangle, whatever its material. A collision source carries one material, `collision`, which nothing reads, because the CMO layout needs one.
  - Its nodes are its skeleton's bones, each a root at the identity, or one node for a file with no skeleton.
  - It refuses a socket bone, a node that is not a root at the identity, and more triangles than its row's budget.
  - It writes `Neuron::CollisionAsset`, version 1, into a `Collision` entry flagged simulation-relevant: each node's name, its corners in world units in the asset's space, and its bounding sphere about the centre of its bounds, and the asset's sphere. The sphere is made in binary64 and rounded once.
* **The simulation data record** (`Neuron::SimulationRecord`, version 1, AST-046, DAT-279), keyed `<mesh key>@sim` (`meshes/wellspring@sim`), in a `SimRecord` entry flagged simulation-relevant. It holds:
  - the mesh's node hierarchy at rest (name, parent, rest transform);
  - its simulation-relevant sockets, in AST table 3.8-A's order and then by index, each with its node and its transform relative to that node (AST-043);
  - the collision asset's key, its bounding sphere and each of its nodes' spheres by the render node they stand for.

  `RestPositionOf` carries a socket through its node chain, and `SocketsOf` gives a kind's sockets in index order (ADR-021: a socket is addressed by kind and index). A mesh with neither a simulation-relevant socket nor a collision asset gets no record.
* **The refusals lifted, and the one kept.**
  - The mesh cooker no longer refuses a simulation-relevant socket, and the content build sends a collision row to the collision cooker.
  - The refusal of a node with geometry and another rest transform stays: every M2 mesh rests at the identity (ADR-128), so no mesh has settled whether such a node's vertices are in its own space.
* **A socket carrier.** The turret mount's row states "no triangles budget; 1 node, depth 1": a node with no geometry carries its sockets (AST-045). The row read "0 nodes, depth 0", which no CMO file can spell: the importer needs a mesh named for a bone, and a socket needs a node. "No triangles budget" is read as a budget of 0.
* **The server.**
  - GameShared's `LoadStructureRecords` loads the records of the MVP's eight structure meshes by their keys. `WorldServer::Desc` gains `structures`.
  - `SpeciesServer` and the integrated server start only when every record loads, as they do with the tuning record (MVP-099), and report a missing or damaged one by its key.
* **No schema id.** A record's bytes carry their own version, as a cooked mesh's do, so its index entry has no schema id, as no geometry entry does.
* **The checks.** `Build\CheckContentBuild.py` gains five fixtures:
  - a misnumbered port;
  - a collidable mesh without its collision asset;
  - a collision asset without its render member;
  - a collision node of no render node;
  - a glyph table naming another face (AST-089).

### Consequences

* Good, because the simulation reads ports, entrances and spheres from records that the content identity covers, and never from a mesh.
* Good, because a missing or orphaned collision asset is a build error with its file, and no new authored file states a relation the manifest already holds.
* Bad, because the pairing rests on a naming convention of asset ids. A collision asset for a mesh of another name needs the import records of AST-033, which supersede this rule when they come.
* Bad, because the server now starts only with the eight records, so a test that starts a server cooks records of its own.
* Neutral, because AST-089's reference check covers the references M2's content makes: the collision pairs and the glyph tables' faces. A record field that names an asset is checked when the first one exists.

### Confirmation

* NeuronCoreTests: the collision asset and the simulation data record read back as written, a socket placed through its node chain, and their refusals.
* GameLogicTests: a server whose content lacks the records does not start (`Ast046AServerWithoutItsStructureRecordsDoesNotStart`).
* `Build\CheckContentBuild.py`: the five fixtures above, and two runs alike.
* T4 and T5 read the ports and spheres of the records.

## Pros and Cons of the Options

### 1 — pair by asset ids, a record a mesh

* Good, because the manifest is the one statement of the relation.
* Bad, because the convention admits one render asset per name.

### 2 — import records

* Good, because each relation is explicit and per member.
* Bad, because they need an authored file per mesh and the import pipeline that reads them, neither of which M2 has.

### 3 — one table of all structures

* Good, because the server loads one entry.
* Bad, because every structure's change changes one record, and AST-046 has one record per asset.

## More Information

* Related: [ADR-019](ADR-019-mesh-source-format-and-cooked-meshes.md), [ADR-021](ADR-021-mesh-socket-schema.md), [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md), [ADR-128](ADR-128-mesh-kit-and-drafted-structure-recipes.md).
* AST-033, AST-043 to AST-046, AST-077, AST-089 to AST-091, AST-098, AST-100; DAT-279; R-BLDG-041 to R-BLDG-044; MVP-099.
* The code:
  - NeuronCore `CollisionAsset` and `SimulationRecord`;
  - the content build's `CollisionCooker`, `SimulationRecordCooker` and the pass in `ContentBuild`;
  - GameShared `StructureRecords`;
  - `WorldServer`, `IntegratedServer` and `SpeciesServer`;
  - `Build/CheckContentBuild.py`.
* Measured on 2026-10-21, with a Linux build of the content build on this session's container, over the repository with the kit's meshes:
  - 78 entries: 13 meshes, 7 collision assets and 8 simulation data records;
  - the records are 337 to 2,209 bytes each (the Wellspring's 774), and the collision assets 629 to 3,768 bytes;
  - the whole output is 1,019,081 bytes, of which the index is 5,884;
  - a cook takes 0.04 to 0.05 s over three runs of an optimised build.
