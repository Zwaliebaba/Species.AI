# C-TER report: terrain component

## Files written

| File | Size (bytes) | Content |
|---|---|---|
| `design_next/architecture/components/terrain.md` | 117 200 | the component specification (template §6.4), one file, under 120 KB, so no folder split |
| `species_work/scripts/C-TER_generator.py` | helper | conformance generator: an independent implementation of terrain.md §4, used to check every vector |
| `species_work/scripts/C-TER_check.py` | helper | self-check: ids, keys, citations, headings, truncation words, links, Mermaid captions, size |

The file is close to the 120 KB limit (117.2 KB). A further addition of more than about 2.5 KB should split it into `architecture/components/terrain/` with a `README.md` (natural split: generator §4.1–§4.11 and tests; presentation derivations §4.12–§4.14).

## Counts

- Requirements: **116**, TER-001 to TER-254, unique and ascending, all in the range 001–399.
- Tuning keys: **55** `ter.*` keys in one table (§4.17), each used by a requirement; no `ref.*` or `rnd.*` key is defined here; every cited foreign key exists in `design_next/`.
- Diagrams: **2** Mermaid flowcharts (pipeline; generation of one tile), each with a caption; both parse with mermaid 11.4.1 (the parser rejects a known-bad sample).
- Enumerations: 24 numbered tables (fields, guide sizes, rounded functions, pass plans, methods, edge cases, sampler cases, flatten operations, grid sizes, edge semantics, normals comparison, data products, lifecycle, conversions, tuning, tests, vector sets A–D) and 6 numbered step lists or pseudocode blocks.
- Acceptance tests: 26 (T1–T26: unit, property, golden-hash, performance) and 4 anonymous vector sets.

## Checks run

- `scan_patterns.py forbidden_patterns.tsv` and `soft_patterns.tsv` on terrain.md: no output.
- `C-TER_check.py`: 0 problems (ids, keys, DEC/ADR/asset ids exist, every Open entry of `decs_to_cite` cited by a requirement, every `assets_for_you` id cited, every cited R-*/RND-* id defined elsewhere, template headings and "Design-phase inputs" present, no truncation words, every relative link resolves to a file or a package target).
- **Vector verification.** `C-TER_generator.py` was written from my text and reproduces every value of the input's worked example and unit vectors exactly: the derived values of all three tiles of set A (including posZi = 62), the ten-event method-0 trace, the method-1 and method-2 traces, the world range and the four probe samples, the 259 negative-fraction merge reads, the event counts, the smooth-flatten sample and its neighbours-first variant, and the ceilLog2 property (0 mismatches below 70 000). For set B the input gave only the highest sample and height factor of a tile whose position y and desired height it did not list; the generator shows they are exactly position y 0.0 and desired height 391, which set B now states. Set B's cell size (20.0), world size and position are my choices; the tile values do not depend on them (stated in §7.2). Set D (five flatten operations on a flat field) was computed with the generator and hand-checked for two samples.

## Decisions cited

- **Open, from `decs_to_cite`:** DEC-226 (TER-222, TER-242), DEC-240 (TER-061, TER-065, TER-221), DEC-242 (TER-080), DEC-246 (TER-006, TER-009, TER-061, TER-062, TER-153, TER-187, TER-221, TER-252), DEC-247 (TER-009, TER-155, TER-240, TER-241), DEC-324 (TER-032, TER-101, TER-106, TER-144), DEC-326 (TER-043).
- **Resolved:** DEC-025 stated as the fixed rule (TER-003, TER-009). **Dropped:** DEC-063 (TER-009, §7.2: no reference terrain, shape, name or digest is reproduced).
- Further DECs cited where they apply: DEC-004, DEC-007, DEC-010, DEC-011, DEC-013, DEC-014, DEC-015, DEC-040, DEC-045, DEC-113, DEC-118, DEC-204, DEC-224, DEC-241, DEC-251, DEC-653, DEC-800.

## New DEC requests

1. **Grid walk of an axis-parallel ray** (TER-017). The input says a ray with a zero x or z component "falls into the last (south-east) branch" of the quadrant variants, tagged inferred; which quadrant that is, and whether the walk then misses cells, is not established. Stated default: the walk visits the column or row the ray lies in. Options: A — that rule; B — reproduce the reference branch once confirmed. §8.1 mentions the point without a DEC id.
2. **Highest-sample scan on a non-square grid** (TER-076). The reference bounds both loops by the row count; tiles and every reference terrain are square, so only a non-square Species terrain (its colour normaliser) is affected. Default: the reference scan. Options: A — keep; B — scan columns × rows. Recommend B.
3. **Ray entering from outside with direction.x = 0** (TER-016). The reference divides by zero for the entry height. I applied the DEC-653 rule (no non-finite value; keep the reference result where defined) by computing λ from the z components. Please add this site to DEC-653's list.
4. **Grid-size limit** (TER-072, TER-250). The index semantics hold for grids of at most 32 768 samples per side, so the content build rejects larger grids (`ter.grid.max_samples_per_side`); reference grids are at most 334. Record under DEC-015 or DEC-247 (a large world uses chunks).
5. **Colour-variation and shore-noise generator** (TER-185, TER-186). Baseline: a private presentation-generator instance per renderer build, seeded with `ref.sim.match.presentation_seed` at the start of each build, keeping the reference's per-triangle re-seeding chain; each sea-detail level's shore noise continues from a copy of the end state. This makes rebuilds identical (the reference perturbs colours through the shared generator and through mid-match regeneration). Please confirm under DEC-013, or record it with DEC-246 item 11 (stateless alternative, TER-187).
6. **No regeneration on an option change** (TER-221). Species never regenerates the field mid-match (RND-031 already rebuilds nothing); simulation-observable behaviour is unchanged because the reference regeneration gives identical heights. DEC-246 item 8 records it only as a lesson of an Open DEC; a Resolved entry (or a sentence in DEC-007/DEC-010) would make it a fixed rule.
7. **Build-time rejections** (TER-112, TER-124, TER-250): tiles below 2 cell sizes, methods outside 0–2, wrong guide-grid cell counts, partly overlapping tiles with an index of −1 or below, non-finite values. These are undefined in the reference and absent from its content; please confirm them as part of DEC-015's scope.

## ADR conflicts

None. Note only: ADR-0011 speaks of "a per-vertex colour field"; the terrain supplies one colour per triangle (RND-134, flat shading per ADR-0020), which the barycentric chunk layout carries per vertex. No change requested.

## Missing assets

None. Asset ids cited: `terrain.ramp`, `terrain.sea_tile`, `terrain.shore_wave_gradient`, `fx.terrain_grid` (all of `assets_for_you`). For X-AST: the manifest spec of `terrain.ramp` says "x = slope, y = height" but not the row orientation; TER-040 fixes it (look-up row 0 = the **bottom** row of the image as viewed = the highest terrain; top row = sea level). The shore-wave gradient's colour row is row 1 counted from the bottom as viewed (TER-041). Please carry both into the spec column and `art_and_audio_briefs.md` §9, and add the 64 × 64 size validation of the ramps (TER-250).

## Cross-document requests

1. **G08a, `reference/08_maps_and_level_design/rules_and_conventions.md` R-MAP-081 and `ref.map.terrain.guide_grid_levels` (16).** A guide cell holds one byte, 0 to 255 (256 levels), as map_editor_hooks R-MAP-334 (`ref.map.editor.guide_grid.sample_max` 255) and TER-036 state. In the reference text each cell is written as **two** letters from A to P, high nibble then low nibble (value = 16 × (hi − A) + (lo − A)); "16 levels, written A to P" is wrong. Please correct R-MAP-081 and the key (or drop the key and cite TER-036).
2. **C-DAT, `data_schemas.md` (map schema).** Please take from my input, which I do not reproduce: the field order and types of TER-030, TER-032 and TER-034; one correctly rounded decimal parser with a single binary32 rounding (TER-066, DEC-241 item 12); the guide-grid authoring encoding (row-major, z outer, x inner; the reference text form of request 1, 2r² letters after the power and three spaces; the reference decoder does no validation, keeps value mod 256, reads past the end of a short string and truncates a long one); the flatten record's optional fields (the threshold is read only when the type is present); and the validation list of TER-250. The reference block keywords, tokeniser separators, comment character, the case-insensitive keys, the crash on a blank line inside the terrain-definition block and the fixed 512-character file-name buffers are format matters for your file.
3. **C-RND-a, `renderer/world_renderers.md`.** (a) RND-144 says "row 1 of the map's shore-wave gradient": please add "counted from the bottom row as viewed" (TER-041). (b) RND-145 uses the smooth presentation time for the waves; my input gives the game timer's accurate time read on the render path; please confirm the mapping. (c) TER-181 sums each triangle's centre in strip order (p1, p2, q1 and p2, q1, q2), while RND-134 lists the second triangle as (q1, p2, q2); only the facet-normal order is yours, which matches. (d) RND-131 lists what it reads from terrain.md; all of it is now specified (TER-040 to TER-043, TER-180 to TER-206).
4. **G08b, `map_editor_hooks.md` R-MAP-387.** My input gives the reference tile writer fixed-width fields (lowland smoothing `%6.2f`, power `%6d`), so the two values are separated by padding unless a value fills its width; please check the "no separator" statement. §8.2 of terrain.md cites R-MAP-385 to R-MAP-387 for the writer and states the precision loss.
5. **C-SIM-b, `05_space_routing_and_physics.md` R-SIM-387.** The terrain ray is also used by the Transport and by weapon terrain hits, not only by the line of sight and the pointer (TER-018).
6. **C-DBG, `diagnostics_test_and_performance.md`.** Terrain provides the canonical grid digest (TER-252), the per-tile trace (TER-253) and its generation time (TER-254, part of load timing); the determinism corpus starts with terrain heights (R-SIM-708); tests T22, T23 and T26 need your harness.
7. **Coordinator, DEC-246 item 6.** "Changing either changes the heights of about one terrain in six": per my input, changing the bilinear order changes every terrain's digest, and the smooth order changes 11 of the 57 reference map files. TER-062 states this.

## Names not in the ledger (proposals)

- `TileMethod` with `Average` (0), `RandomPair` (1), `CopyOne` (2): the tile generation method (the reference has a bare integer).
- `FlattenOp` with `Absolute`, `Add`, `Subtract`, `Smooth`, `Subtract2`: the flatten type (named so as not to repeat the reference enum's identifier; display names follow R-MAP-083).
- `SeaMasks` (introduced by C-RND-a), `WaveVertex`, `WaveMeshSource`; functions `TerrainField::Generate`, `Height`, `HeightAt`, `Normal`, `NormalAt`, `IsInside`, `CastRay`, `LineOfSight`, `BuildSeaMasks`, `SelectWaveVertexPairs`, `FacetColour`; `SurfaceGrid` members `At`, `Set`, `Sample`, `Nearest`, `IndexX`, `IndexZ`, `PositionX`, `PositionZ`, `Highest`; the private tile generator's `Seed`, `Next`, `Signed`.
- Notation (not identifiers): *r32*, pow32, exp32, log32, sqrt32, sin32, abs32, log2r, ceilLog2; "conformance generator" for the independent reference implementation kept with the tests.

## Dropped as legacy (one line per group)

- Provenance: the build-tag table, evidence tags, call-site counts, the recovery scripts, the source map and every citation.
- D3D-only material: the normal negation and triangle-list conversion, the D3D water buffers, the unnormalised wave normals and the out-of-bounds table read that only those normals make (the routing's dropped second-renderer group).
- Burn patches and their colour darkening (DL-CAMPAIGN; only a blocked campaign structure creates them).
- Legacy technology: per-thread floating-point precision control, compiler and runtime-library flags, the statically linked C runtime, the second-platform project, BMP header and palettised-format details, the text tokeniser, file obfuscation and fixed-size file-name buffers; replaced by TER-061, DEC-240/241, ADR-0017/0018 and the map schema.
- Dead or no-effect code: the unused unsigned-fraction draw of the tile generator, two unused locals of the merge, the commented-out rescale of the smooth operation, the detail scale factor that has no effect, an unused lightmap constant, the sphere query with no caller, the debug drawing of normals, a debug assertion in the guide-grid power, the commented-out water-detail shortcut, and the strip-start magenta colour on vertices that never colour a triangle.
- Implementation details without observable effect: when the height and normal grids are reallocated on regeneration, the tile grid reallocated only when *n* changes.
- Reference-map specifics: map names, the per-file usage tables of ramps, gradients and sea tiles (their counts are R-MAP-086), the two golden digests, map checksums, the map-specific building counts; the worked example and unit vectors are restated as anonymous sets A to C (DEC-063).
- History: the corrections of earlier drafts noted in the input (bilinear order, smooth order, rounding of the wave colour index) appear only as the rules they fixed.
