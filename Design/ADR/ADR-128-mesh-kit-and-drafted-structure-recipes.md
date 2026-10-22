---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M2 task T2), owner (O81, O86)
---

# The mesh kit and the drafted structure recipes

## Context and Problem Statement

M2 needs the MVP's structure meshes: the Wellspring, the dwelling, the storehouse, the two resource sites, the turret's mount, head and gun barrel set, the turret round, the shell casing and the port pad, and a collision asset for each collidable one ([MVP assets](../implementation/mvp_assets.md) table 1, MVP-083). The simulation reads the structures' ports from their meshes' sockets (AST-043), so the port positions have to be exact and repeatable.

O81 has a parametric low-poly mesh kit make these meshes from recipes, rather than the owner drawing them in a modelling tool. O86 has the implementing session draft the recipes, and the owner approve or redesign each silhouette before T2 merges.

## Decision Drivers

* The port, entrance, door and muzzle positions are where the simulation's ports and spawn points stand, so they are values of record that a recipe states in numbers (AST-043, R-BLDG-020).
* The kit writes CMO files that the content build checks and cooks like any authored mesh (O64, ADR-019, ADR-108); nothing special-cases its output.
* No third-party code (R14 binds the executable, and the kit needs none).
* A recipe's output is reproducible, and CI shows that the committed meshes are what the committed recipes write.
* Each collision asset comes from its own simpler recipe at its render member's bounds, never from the render triangles (AST-044).

## Considered Options

1. **A C++ tool project of the solution that writes CMO files, with the recipes in code.** It is developer-run, and its output is committed under `GameData/Meshes/`.
2. **The meshes as a build step:** the content build runs the recipes and cooks their output, with no CMO committed.
3. **A Python script** under `Tools/` that writes the CMO files.

## Decision Outcome

Chosen option: 1. The CMO files stay the authored source that every other mesh is, so the content build, its checks and its fixtures treat them alike. The owner can replace any one of them with a hand-made CMO without touching the build. Option 2 would make a recipe change a content change with nothing committed to review, and couple the content build to the kit. Option 3 would give the kit a second copy of the CMO layout in another language, outside the solution's compiler, checkers and naming.

* **The project.** `Tools/SpeciesMeshKit`, a tool project in the solution's `Tools` folder, with no project reference and no package. `SpeciesMeshKit.exe [--root <repository root>] [--out <folder>]` writes every recipe's file into `GameData/Meshes/` of the root, or into the folder `--out` names.
* **The primitives** (`Shapes`):
  - a regular frustum standing up or pointing forward (+z), which is a prism when its radii are equal and a pyramid when its top radius is 0, with either cap;
  - an axis-aligned box, its bottom face optional;
  - a gabled roof.

  Every triangle is wound counter-clockwise seen from outside, which the kit checks against each face's outward direction.
* **Exact circles** (`UnitCircle`). A ring's directions are k ÷ n of a turn for n of 3, 4, 5, 6, 8, 10, 12, 16 or 20, taken from closed forms of square roots, sums and halvings that IEEE 754 rounds alike on every host. No sine or cosine of the C runtime is called, so two hosts write the same bytes. Positions are composed in binary64 and rounded to binary32 once.
* **The CMO it writes** (`CmoWriter`): the layout that the content build's importer reads (ADR-019 as amended by O64):
  - one mesh for each node, named for the node;
  - one material for each surface the node uses: `facet`, `team` (the tint weight of ADR-020) or `unlit`; the white diffuse the cooker requires; a collision asset's one material, `collision`;
  - one index buffer and one vertex buffer, three vertices a triangle, each vertex carrying its triangle's facet normal and colour;
  - the extents;
  - when the asset has more than one node or any socket, a skeleton on the first mesh: a root bone at the identity for each node, then a bone `socket_<name>` for each socket, a child of its node's bone, translated to its position.

  Every node rests at the identity, so a node's vertices are in the asset's own space, and M1's refusal of a node with geometry and another rest transform (ADR-108) is never met.
* **The recipes** (`StructureRecipes`, `TurretRecipes`) are drafts sized on the reference:
  - The turret's structure radius is 30 wu (`ref.bldg.turret.radius`), and its ray sphere 12 wu at 10 wu (R-BLDG-407).
  - Ports are pushed 5 wu clear of their structure (`ref.bldg.port.clearance_wu`).
  - The Wellspring is the largest and brightest shape (MVP-083, the art brief §10).

  | File | Nodes | Sockets | Triangles | Shape |
  |---|---|---|---|---|
  | `Wellspring.cmo` | 2 | 11 | 188 | stepped octagonal plinth of 30 wu radius, a pool, three pylons whose caps are its lights, the arch of its door, and a floating crystal as its second node; its five ports at 38 wu, at 36° + 72° k from +x; its door at +z, its entrance at −z, 33 wu out |
  | `Dwelling.cmo` | 1 | 10 | 84 | a gabled house of 18 × 14 wu with four windows in the team's colour; six construction ports at 18 wu |
  | `Storehouse.cmo` | 1 | 7 | 60 | a slate-roofed hall of 26 × 18 wu with a loading platform, its entrance at 14 wu on +z and two lights over the door; four construction ports at 24 wu on the diagonals |
  | `ResourceSite.cmo` | 1 | 4 | 38 | a hexagonal mound with a cluster of Crystal (O57); four gathering ports at 18 wu |
  | `ResourceSiteCentral.cmo` | 1 | 12 | 106 | a broad mound with three clusters and a spire; twelve gathering ports at 34 wu |
  | `TurretMount.cmo` | 1 | 8 | 0 | a socket carrier: four manning ports at 34 wu on the diagonals and their status lights at 24 wu |
  | `TurretHead.cmo` | 2 | 2 | 96 | a pedestal with a band in the team's colour, and the turning body with its shield and sight; its barrel mount at (0, 10, 7.5) |
  | `TurretBarrelGun.cmo` | 20 | 8 | 336 | four barrels in a square, each of five nodes (breech, tube, jacket, muzzle brake, casing ejector), its muzzle at the brake's end |
  | `TurretRound.cmo`, `ShellCasing.cmo` | 1 | 0 | 22, 28 | a round and a casing along +z |
  | `StructureDefault.cmo` | 3 | 1 | 36 | the port pad: a hexagonal plate on a rim with a marker in the team's colour; its `status` socket 3 wu above it |

  Each collision asset is two to eight boxes or frustums enclosing its render member's nodes, of 16 to 96 triangles.
* **The check.** `Build\CheckMeshKit.py` builds the kit through the solution, runs it twice and fails unless both runs write the same bytes and every file is the committed one. CI runs it before the build.

### Consequences

* Good, because every structure the simulation reads has exact, repeatable ports, and a recipe's change shows as a reviewed change of committed meshes.
* Good, because the content build cooks the kit's files with no special case, and the owner may replace any of them by hand.
* Bad, because the recipes are code: a silhouette is changed by editing numbers, and seen only once it is drawn (M2's T12) or in a preview.
* Bad, because the kit's circles are limited to the step counts with closed forms; a ring of 7 or 9 needs a new closed form or a rounding pinned in data.
* Neutral, because the drafts wait for the owner's approval (O86). A redesign may move ports, which then moves only the simulation data records the content build writes (ADR-129).

### Confirmation

* `Build\CheckMeshKit.py` in CI: two runs alike and alike to the committed files, and its fixture, a changed copy, reported.
* The content build accepts every file within its row's budget (`Build\CheckContentBuild.py`), and refuses a misnumbered port.
* The owner's approval of each silhouette, recorded in the M2 record.

## Pros and Cons of the Options

### 1 — a C++ tool project, output committed

* Good, because it shares the solution's compiler, checkers and conventions, and the CMO stays the one source format.
* Bad, because a developer runs it and commits its output, as for the schema generator.

### 2 — the recipes as a build step

* Good, because nothing generated is committed.
* Bad, because a recipe change would reach the content unreviewed, and the content build would depend on the kit.

### 3 — a Python script

* Good, because it is short.
* Bad, because the CMO layout would be written a second time, outside the C++ checks, and Python's math library is not pinned across hosts.

## More Information

* Related: [ADR-019](ADR-019-mesh-source-format-and-cooked-meshes.md), [ADR-020](ADR-020-flat-shading-and-team-tint.md), [ADR-021](ADR-021-mesh-socket-schema.md), [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md), [ADR-129](ADR-129-simulation-data-records-and-collision-cooker.md).
* Owner decisions O64, O81, O86; MVP-083; AST-038 to AST-040, AST-043, AST-044; R-BLDG-017, R-BLDG-020, R-BLDG-405, R-BLDG-407.
* The code: `Tools/SpeciesMeshKit` (`UnitCircle`, `Shapes`, `MeshModel`, `CmoWriter`, `StructureRecipes`, `TurretRecipes`, `SpeciesMeshKit`) and `Build/CheckMeshKit.py`.
* Measured on 2026-10-21, with a Linux build of the kit on this session's container: the 18 files total 252,174 bytes of CMO. A Linux build of the content build cooks them, with the rest of `GameData/`, in 0.04 to 0.05 s over three runs of an optimised build. The `--out` of two runs compares equal.
