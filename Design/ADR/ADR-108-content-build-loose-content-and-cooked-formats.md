---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T11), with owner decision O73
---

# The content build at M1: SpeciesContentBuild, the cooked formats and the loose-content mount

## Context and Problem Statement

Task T11 of the [M1 plan](../implementation/m1_plan.md) builds the content build tool. It covers:
- the record cooker, whose output the server reads in place of ADR-098's interim parse of the authoring file (DAT-010, DAT-050);
- the check that packs each texture as authored (O61, O64);
- the font packer for both faces (O56, O63);
- the CMO importer and mesh cooker for `LeaderFigure.cmo` and `ThrownWeapon.cmo` (O64, O69);
- the Debug loose-content mount of the cooked output (AST-105);
- a CI step that runs it.

The asset pipeline component specifies the whole content build: import records, edition manifests, placeholders, packs with their mount and content manifests, residency lists and the reference checks. The M0 plan puts packs, editions and content identity "before the first build that leaves the owner's PC", so M1 builds a first part of it.

The specifications leave open:
- what M1's cooked output is and where the programs find it, given that AST-006 keeps loose files out of Release builds while T10 and T18 measure `Release|ARM64`;
- the byte layouts of a cooked record, a cooked mesh, a cooked face and the folder that holds them;
- what the schema generator needs for the glyph table, a record with a list and defaulted fields;
- how the content build reads the asset manifest's rows, which are prose;
- which of the pipeline's rules wait for later milestones.

The owner answered four of these (O73):
- until packs exist, every configuration mounts the loose-content folder;
- the solution build cooks the content;
- M1 has no import records, and a mesh is cooked at one world unit per source unit;
- T11 lands on M1's pull request ahead of the phase-A tasks still to come.

## Decision Drivers

* No program parses content numbers; every process reads the bits the content build wrote (DAT-010, DAT-050).
* Cooked output is identical for identical inputs (AST-066), and every fault names its file and the rule (AST §6).
* The runtime path is the one later content takes, so packs replace the folder without changing a loader (MVP-086).
* A tool never links into a program, and no program references a tool (`AGENTS.md` §2).
* What M1 leaves out is stated, not silently skipped.

## Considered Options

* 1 — a tool project whose output is a loose-content folder of cooked entries with an index, mounted by every configuration until packs exist; cooked formats that pack entries can carry unchanged
* 2 — the pack writer and reader of AST-128 now, with the loose folder as the Debug override only
* 3 — the cooked tuning record alone, and the other content left for the milestones that draw it

## Decision Outcome

Chosen option: "1".
- Option 2 brings forward the packs that the M0 plan places before the first external build, with their codec, chunks and manifests, for a build that never leaves the owner's PC.
- Option 3 leaves the textures, faces and meshes unchecked until they are drawn, and T11's done-when names a texture, a mesh and a glyph table.

* **The tool.** `Tools/SpeciesContentBuild/SpeciesContentBuild.exe`, a tool project of the solution in the `Tools` folder (ADR-090 item 6).
  - It references NeuronCore and GameShared, whose generated `TuningMvp.h` holds the tuning record's reader and cooker. It holds the font packer, which ADR-090 item 6 listed as a tool of its own.
  - It has no third-party code. `SpeciesContentBuild.exe --root <repository> --out <folder>` reads `Design/assets/asset_manifest.csv` (AST-012) and every file under `GameData/` in byte-wise path order. It writes the folder only when no source has a fault.
  - It skips `GameData/Schemas/`, which the generator reads (ADR-098). It cooks a file with a manifest row by the row's category:
    - a `.dds` of a texture category is checked against its row and packed as authored;
    - the two members of a face, its sheet and its glyph table, are packed by the font packer;
    - a `.cmo` of the category Meshes goes to the mesh cooker;
    - a row of another category is refused, its cooker arriving after M1.
  - A `.json` under `GameData/Records/` with no row is a data record. Any other file is a stray (AST-013).
  - It reports, every run, how many rows have no authored file yet.
  - The exit code is 0 for a written folder, 1 for content faults, each printed as `<file>: <fault>`, and 2 for a wrong command line.
* **The solution build cooks (O73).** The project's post-build step runs `"$(TargetPath)" --root "$(SolutionDir)." --out "$(OutDir)content"`. `DisableFastUpToDateCheck` makes Visual Studio run it on every build, so a content change cooks without a code change.
  - A fault fails the build, as ADR-059 asks of a validation error.
  - `Species` and `SpeciesServer` carry a build dependency on the tool in `Species.slnx`, which orders the build and links nothing. So building either program, which is what Visual Studio does on a run, cooks the content too. Neither references the tool project (ADR-090 item 6).
  - Every configuration and platform cooks into its own `content` folder.
  - Building ARM64 on an x64 machine cannot run the ARM64 tool. The owner builds ARM64 natively (ADR-094), and CI builds x64.
* **Textures, packed as authored (ADR-017 as amended by O61, O64 and O69).** The row's `target_format` names the format and whether the chain is full, one level or read on the CPU (`CpuTable`, AST-072). The `spec`'s first `<w>x<h> px` is the display size, doubled for "author at 2x" (AST-024). Its `colour_space` must agree with the format (AST-020).
  - The DDS must have the DX10 header and be one 2-D texture. Its format, size and level count must be the row's.
  - A block-compressed size must be a multiple of 4, and no side may pass 16384 px (AST-068). Its payload must be exactly the bytes its levels hold.
  - Every difference is reported. The entry is the file's bytes, unchanged.
* **Faces (DAT-219 as amended by O61 and O63).** The sheet is a DDS without a DX10 header, 32-bit A8R8G8B8 with one level, of the size its row states. It is white wherever its coverage, the alpha, is not 0, and the coverage is 0 or 255.
  - The table is read by the reader generated from `GameData/Schemas/text.glyph_table.schema.json`.
  - The packer checks what the schema cannot: the `face` names the table's own asset; the cell, design size and baseline are the row's; sizes fit 16 bits; no code point repeats; every cell lies on the sheet; no glyph is wider than its cell; the missing glyph is held.
  - The face's one `Font` entry takes the sheet's key. The table folds into it and has no entry of its own.
* **The cooked face** (`FontAsset`, version 1, every value little-endian):
  - the version (32 bits);
  - the design size, line height, baseline and cell width and height (16 bits each);
  - the missing code point (32 bits);
  - the page's width and height (16 bits each);
  - the glyph count (32 bits);
  - the glyphs sorted by code point, each its code point (32 bits), the top-left texel of its cell, its ink width (16 bits each), its bearing (signed 16), its advance (16) and its vertical offset (signed 16);
  - the page, one R8 coverage byte a texel, rows from the top.
* **Meshes (ADR-019 as amended by O64).** The importer reads the CMO file whole and applies the amendment's checks in its order. It rejects the asset at the first failure and names the byte offset and the check.
  - A string is the units its length states, ending in its one NUL, as the Visual Studio pipeline writes it.
  - At most one mesh may carry the skeleton.
  - The cooker then applies the Species rules:
    - nodes are the bones that are not sockets, and a file with no skeleton is one node named after its mesh;
    - a socket (`socket_<kind>[_NN]`) is a bone with no mesh, the child of a node, of a kind of AST table 3.8-A, numbered from 01 without gaps and not written both with and without a number;
    - a material's diffuse colour is white, and it names no pixel shader and no texture; `unlit` selects the unlit shader and `team` marks its triangles with tint weight 1;
    - every triangle is faceted, one colour at its three corners and with an area;
    - the triangle count is within the row's budget, and the node count, the depth and the sockets (with any "on node" constraint) are the row's;
    - a `team` material marks at least one triangle (AST-035).
  - M1 refuses three things that it has no source for yet, each with its reason: a node with geometry whose rest transform is not the identity, because whether a bone's vertices are in its own space or the model's is settled by the first mesh that has one; a simulation-relevant socket, whose simulation data record (AST-043) comes with M2's structures; and a collision row.
* **The cooked mesh** (`MeshAsset`, version 1, every value little-endian, floats as IEEE 754 binary32):
  - the version, then the material, node, socket and vertex counts (32 bits each);
  - each material: its name (length in 8 bits, then UTF-8), its shading (8 bits: lit, unlit), its specular colour (3 floats), specular power and emissive colour (3 floats);
  - each node: its name, its parent (32 bits, 0xFFFFFFFF for a root, which comes before its children), its rest transform (16 floats, row-major with row vectors, as the CMO file writes it), its bounds' minimum and maximum (3 floats each; their y range is its vertical extent), its bounding-sphere radius about the bounds' centre, and its draws (a count, then each draw's material, first vertex and vertex count, 32 bits each);
  - each socket, sorted by kind in table order and then by index: its kind (8 bits), its index and its node (32 bits each) and its transform relative to the node (16 floats);
  - each vertex, 32 bytes: position and facet normal (3 floats each, the normal computed in binary64 and rounded once), facet colour (sRGB-encoded red, green, blue and alpha, each red, green and blue × 0.85 rounded half up in integer arithmetic, RND-160), tint weight (0 to 255), barycentric corner (0, 1 or 2) and 16 reserved zero bits.
  - Positions and translations are scaled by the unit scale, 1 at M1 (O73). The CMO axes are already Species' (ADR-016).
* **Records (ADR-098).** The record cooker parses an authoring file with the strict parser, picks the record type its `schema` names, checks the type's folder (DAT-016) and reads it with the generated reader.
  - It writes the cooked entry with the generated cooker, reads it back with the generated loader and compares the bytes (AST-091).
  - The registry holds `tuning.mvp`, which is simulation-relevant (AST-098).
  - **The cooked record** (DAT-014): the schema id after its length in 8 bits, the version (two 16-bit values), the payload's length and the record count (32 bits each), then the payload.
  - A record of fixed-size fields has as its payload every field in schema order, an integer as 64 bits and a decimal as its binary64 bits, at the precision the content build rounded it to (DAT-010). `tuning.mvp`'s payload is 2,984 bytes, 373 fields.
  - `ReadCookedTuningMvp` checks the header and the length and copies the fields. A record with text or lists gets its cooked form, with payload offsets, from the first record type that needs one.
* **The generator's subset grows (ADR-062, ADR-098 amendment).** It adds:
  - a `defaulted` presence with JSON Schema's `default`, for integers and for lists, whose default is `[]`;
  - an integer's `maximum`;
  - `asset_ref`, a text with its `x-species-ref` and `x-species-max-chars`;
  - `list`, with `items`, `minItems`, `maxItems` and `x-species-order`, whose element is an object, read by a reader of its own named `<Key>Item`, or an integer;
  - the cooked writer and reader of a schema whose fields all have a fixed size;
  - a generated file in `Tools/Species<Name>`, which takes the namespace `<Name>`.

  `RecordReader` gains the reads behind these. `text.glyph_table` 1.0 has no kerning, which no face has: a table that adds one is refused as an unknown field, and kerning joins as version 1.1 with the first face that kerns (DAT-017).
* **The loose-content folder (AST-105).** It holds each entry's bytes in the file at its key (AST-014) and the index `content-index.bin`, whose name holds a `-`, which no key can.
  - The index is the magic `SPCLOOSE`, a version (16 bits, 1), flags (16 bits, 0) and the entry count (32 bits), then the entries in byte-wise key order, then the SHA-256 of every byte before it.
  - Each entry is AST-128's entry record without its chunk: the key after its length in 16 bits, the content type and the flags (simulation-relevant, placeholder) in 8 bits each, the schema id after its length in 8 bits, its version (two 16-bit values), the size (64 bits) and the SHA-256 of the bytes.
  - The writer empties the folder first, but only one that holds an index or nothing. It writes the entries, then the index, so a folder with an index is whole.
* **The mount** (`LooseContent`, NeuronCore).
  - It reads and checks the index: missing when the folder or its index is not there, damaged when the index is not one.
  - It finds an entry by its canonicalised key (AST-015).
  - It reads an entry whole and checks its size, and its SHA-256 when it is simulation-relevant and, in Debug builds, always (AST-107).
  - It computes the simulation content identity: the SHA-256 over the simulation-relevant entries in key order, each as its key's length (16 bits), its key and its hash. M1 has no runtime feature flag to add to it (AST-100).
  - A program logs the identity and reports it as modified, because the content is loose.
* **The programs.** The client and the headless server parse their start-up options (`StartupOptions`, PLT-022). They mount `ContentDirectory`, which is the `content` folder beside the executable or, in a Debug build, the folder that `--loose-content` names (PLT-026).
  - They load the tuning record with `LoadTuningMvp`, from the mount. The integrated server takes the mount and not a content root.
  - A missing folder or entry ends the client with `ui.error.content_missing`, and a damaged one with `ui.error.content_damaged` (AST-102). The server prints the reason and exits.
  - `FindInterimContentRoot` is gone. `ReadTuningMvpSource` keeps the authoring read for the tests, and no program calls it.
* **What M1 departs from or defers.** Each item states when it ends.
  - The loose mount is in Release until packs replace it (O73; AST-006, ADR-058).
  - There are no import records until T13's samplers and views (AST-018, O73).
  - The content build generates no placeholders, packs, mount or content manifests, editions or residency lists; these come with packs (AST-082, AST-094 to AST-096, AST-129, AST-067).
  - There is no reference or unreferenced-asset check (AST-089, AST-090), no collision cooker and no simulation data record (AST-044, AST-046), and these come with M2's structures.
  - The face sheets are DDS only, and PNG sheets wait for a decoder (ADR-034 as amended by O63).
  - There is no texture cooker (O69).
  - Packs are first-party code and carry the formats above unchanged as entry payloads.
* **The checks.**
  - `Build\CheckContentBuild.py`, a CI step after the build, runs the tool twice and fails unless the two folders are byte-identical. It then runs 23 fixtures, each a copy of `GameData/` and the manifest with one fault, which must each fail with a message naming the file and the fault: four textures off their rows, one per CMO check, four Species mesh rules, four glyph-table faults and a sheet fault, a stray file and a mistyped record value.
  - `Build\CheckProjectFiles.py` gains DAT-277's source rule: outside the tool projects no code names a socket by its text, so code addresses a socket by kind and index.

### Consequences

* Good, because the server reads the tuning record's bits with no parse, and every process reads the same bits (DAT-010).
* Good, because the owner's 52 sources are checked against their rows on every build. A texture off its row, a damaged mesh or glyph table, a stray file and a broken record each stop the build with the file and the fault.
* Good, because the cooked formats and the index are what pack entries will carry, so packs add a container and change no loader.
* Bad, because a Release build reads loose files until packs exist, against AST-006. No M1 build leaves the owner's PC, and the change that adds packs removes it from Release.
* Bad, because the manifest's prose is the build's input: a row reworded so that its format, size or mip policy no longer parses is refused until it is rephrased or import records arrive.
* Bad, because every build cooks again. 51 entries from 1.4 MB of sources took 0.38 s to 0.44 s in three runs of a Linux build of the tool on a 4-core cloud container, not on the reference machine.
* It forecloses: a program that parses an authoring file; a texture re-encoded on its way into the content; a loose folder that a content build did not write being emptied; a socket looked up by its name.

### Confirmation

* `NeuronCoreTests` checks:
  - keys and canonical requests;
  - the cooked record's values and header, and a schema, version, count or length other than the reader's;
  - the DDS headers, the format names and a file that is no DDS;
  - a cooked mesh and a cooked face read back as written, and each broken form refused;
  - the loose folder's mount, its look-ups, its identity, a damaged entry, damaged in Debug only when it is not simulation-relevant, a damaged or missing index, and a writer that empties only its own folder;
  - the start-up option in Debug and Release;
  - the record reader's defaults, lists, texts and bounds.
* `GameSharedTests` checks:
  - the cooked tuning record reads as the authored one;
  - a changed value cooked and mounted is the value the server's load reads;
  - a missing and a damaged record are refused.
* `GameLogicTests` starts the integrated server on cooked content and refuses a mount with no record.
* `Build\CheckContentBuild.py` and `Build\CheckProjectFiles.py` pass on CI.
* The owner runs the Debug build on the reference PC and sees the server start from the cooked record, `content` beside the executable.

## Pros and Cons of the Options

### 1 — a loose-content folder now, packs later

* Good, because it is the smallest thing that ends the interim parse and checks every source.
* Bad, because Release reads loose files until packs exist.

### 2 — packs now

* Good, because AST-006 holds from M1.
* Bad, because it builds the codec, chunking, TOC, mount order and manifests ahead of their milestone.

### 3 — the tuning record alone

* Good, because it is the least code.
* Bad, because faults in the owner's textures, faces and meshes surface only when they are drawn.

## More Information

* Related: [ADR-017](ADR-017-texture-container-and-block-compression.md), [ADR-019](ADR-019-mesh-source-format-and-cooked-meshes.md), [ADR-034](ADR-034-text-rendering-and-glyph-cache.md), [ADR-058](ADR-058-pack-files-and-mount-policy.md), [ADR-059](ADR-059-content-build-and-validators.md), [ADR-062](ADR-062-authoring-syntax-and-schema-language.md), [ADR-070](ADR-070-continuous-integration.md), [ADR-090](ADR-090-solution-and-project-layout.md), [ADR-098](ADR-098-tuning-in-data-and-the-schema-generator.md).
* The code:
  - `Tools/SpeciesContentBuild` holds `AssetManifest`, `RowSpec`, `TextureCheck`, `FontPacker`, `CmoFile`, `MeshCooker`, `RecordCooker`, `ContentBuild` and the generated `TextGlyphTable.h`;
  - NeuronCore holds `ContentKey`, `AssetError`, `CookedRecord`, `DdsHeader`, `MeshAsset`, `FontAsset`, `LooseContent` and `StartupOptions`;
  - GameShared holds `Tuning`;
  - the CI step is `Build\CheckContentBuild.py`.
* Measured on 2026-10-20, with a Linux build of the tool on this session's cloud container:
  - a clean cook of 0.38 to 0.40 s over three runs, by an unoptimised build with debug information on four cores;
  - the output of 889,451 bytes, of which the index is 3,926;
  - each face 59,946 bytes, the Leader figure 9,661 bytes (294 vertices) and the thrown weapon 13,621 bytes (420 vertices);
  - the tuning record 3,007 bytes, against the authoring file's 13,452.
* **Amendment 2026-10-20 (M1 task T13, [ADR-114](ADR-114-gpu-resources-at-m1.md)).** T13 adds no import records: each pass names its sampler by the rule that states it, `fx.sky_cloud`'s `level0` view is the shader's read of level 0, and a BC4 mask is read through its red channel. The import records come with the asset cache. The record cooker's registry gains `tuning.presentation` ([ADR-116](ADR-116-presentation-tuning-record.md)).
* **Amendment 2026-10-21 (M2 tasks T2 and T3, [ADR-128](ADR-128-mesh-kit-and-drafted-structure-recipes.md), [ADR-129](ADR-129-simulation-data-records-and-collision-cooker.md)).** Two of M1's three refusals end, and the deferred checks begin:
  - The mesh cooker accepts a simulation-relevant socket, and the content build sends a collision row to the collision cooker.
  - The pairing pass checks each collidable mesh against its collision asset (AST-044, AST-090) and writes each mesh's simulation data record, keyed `<mesh key>@sim`.
  - The refusal of a node with geometry and another rest transform stays: every mesh of M2 rests at the identity.
  - A row's "no triangles budget" is a budget of 0, for a socket carrier.
  - NeuronCore gains `CollisionAsset` and `SimulationRecord`, and GameShared `StructureRecords`, which the server loads when it starts.
  - `Build\CheckContentBuild.py` gains five fixtures, and `Build\CheckMeshKit.py` checks the mesh kit's committed output.
* **Amendment 2026-10-22 (M2 task T12, owner decision O88, [ADR-140](ADR-140-structures-drawn-and-the-stand-ins-at-m2.md)).** The content build generates stand-ins, which this record left to packs: a texture row of the MVP's priority with no authored file, in BC7 sRGB or BC4, whose `placeholder` column names a figure, a glyph, a panel, a gradient or a cooker's variant, gets one at its key, in its format, size and levels. It passes the row's texture check, takes part in the key check and carries the placeholder flag in the index (AST-082, AST-083). The manifest reader gains the `priority` and `placeholder` columns. Every other row still has no stand-in, and the placeholder report waits for packs.
* **Amendment 2026-10-22 (M2 task T14, owner decision O92, [ADR-147](ADR-147-the-directwrite-text-system-at-m2.md)).** The content build cooks no faces. With the bitmap faces go:
  - the font packer and the cooked font format (`FontAsset` version 1);
  - the glyph tables and their schema;
  - `GameData/Fonts/`.

  The content type `font` stays in the cooked index's list, unused, so the list keeps its numbering. The string tables' coverage is checked against the system fonts of their roles.
