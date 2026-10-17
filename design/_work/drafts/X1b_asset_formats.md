# X1b_asset_formats — Data/asset file formats, part B (shapes, bitmaps, ramps, animations, gestures, audio blueprints, archives, shaders, authoring files)

Scope: `data/shapes/*.shp` (directory level), `data/shapes/source/*.max` (directory level), `data/animations/*`, `data/gestures.txt`, `data/gesture_demos/*`, `data/effects.txt`, `data/stats.txt`, `data/sounds.txt`, `data/terrain/*`, `data/shaders/*` (directory level), `data/purity/*`, `data/sepulveda/*`, `data/thumbnails` / `data/icons` / `data/mwhelp` naming, `data/icons/banner_template.psp`, `data/Untitled Gradients.grd`, `tools/sounds.txt`, `tools/menu_sounds.txt`, `tools/build_multiwinia_data.bat`, `tools/ShapeExporter`, `tools/3DGeometry`, `animator.exe`/`animator.dat`, and every parser listed per format. Build baseline: see build_matrix.md (`Release|Win32` OpenGL + `TARGET_PC_FULLGAME` = **[B:FULL]**).

All paths below are relative to `multiwinia/` @ `f909630267a320c8f8de5e30bbb2dbf5f3a68acf`. Helper scripts written for this draft (run with `python3 -I`, path arguments only): `OUT/_work/scripts/X1b_shp_survey.py`, `X1b_bmp_survey.py`, `X1b_anim_survey.py`, `X1b_sounds_grammar.py`, `X1b_ogg_survey.py`. Survey numbers quoted as "[C] survey" come from those scripts.

Format index (anchors used by the coordinator; IDs added beyond the requested list are marked *new*):

| ID | Format | Section |
|---|---|---|
| `fmt-text-common` (*new*) | shared `TextReader` tokeniser rules | 29.B1 |
| `fmt-redshirt` (*new*) | "redshirt2" obfuscated text | 29.B2 |
| `fmt-shp` | `.shp` shape mesh | 29.B3 |
| `fmt-max` | `.max` 3ds Max sources + ShapeExporter/3DGeometry tools | 29.B4 |
| `fmt-bmp` | BMP bitmaps / textures (+ PNG output) | 29.B5 |
| `fmt-colour-ramp` | terrain `landscape_*`, `water_*`, `waves_*` bitmaps | 29.B6 |
| `fmt-anim` | `.anim` animated help panel | 29.B7 |
| `fmt-gestures` | `data/gestures.txt` training set | 29.B8 |
| `fmt-gesture-demo` | `data/gesture_demos/*.txt` | 29.B9 |
| `fmt-effects-txt` | `data/effects.txt` DSP filter blueprints | 29.B10 |
| `fmt-stats-txt` | `data/stats.txt` entity stats | 29.B11 |
| `fmt-sounds-txt` | `data/sounds.txt` sound-event blueprints | 29.B12 |
| `fmt-pack-list` (*new*) | `tools/sounds.txt`, `tools/menu_sounds.txt`, exclusion/inclusion lists | 29.B13 |
| `fmt-ogg` | Ogg Vorbis samples (+ unreachable WAV reader) | 29.B14 |
| `fmt-shader` | HLSL `data/shaders/*` | 29.B15 |
| `fmt-dat-archive` | `.dat` RAR archives | 29.B16 |
| `fmt-psp` | Paint Shop Pro `.psp` | 29.B17 |
| `fmt-grd` | Photoshop gradient `.grd` | 29.B18 |
| `fmt-icons`, `fmt-mwhelp`, `fmt-thumbnail`, `fmt-sepulveda`, `fmt-purity` (*new*) | naming conventions / legacy content | 29.B19 |

---

<a id="fmt-text-common"></a>
## [→ 29_data_formats_reference] 29.B1 Shared text-file conventions (`TextReader`)

Every text format in this draft (`.shp`, `.anim`, `gestures.txt`, gesture demos, `effects.txt`, `stats.txt`, `sounds.txt`) is read through the SystemIV `TextReader` family [C] `contrib/systemiv/lib/filesys/text_stream_readers.cpp:TextReader::CleanLine`, `TextReader::GetNextToken`, `TextFileReader::ReadLine`, `TextDataReader::ReadLine`; `contrib/systemiv/lib/unicode/unicode_text_stream_reader.cpp`. Live in [B:FULL] (all files `vcproj_build=all` in `manifest.csv`).

**Reader selection** [C] `code/lib/resource.cpp:Resource::GetTextReader`: for a resource name `N` the loose file `data/N` is tried first, then the archive entry `data/N` (§29.B16). In both cases a UTF-16 reader is constructed first; if the first two bytes are `FF FE` (UTF-16LE) or `FE FF` (UTF-16BE) the file is Unicode, otherwise it is re-opened as an 8-bit reader. All files in this draft's scope are 8-bit ASCII with CRLF line ends [C] `file(1)` survey.

**Line processing** (`CleanLine`, applied to every physical line):
1. *Obfuscation check* (first line only): if the line starts with `redshirt2` (case-insensitive) the reader strips those 9 characters and decrypts this and every following line (§29.B2).
2. *Merge-conflict guard*: three or more adjacent `<`/`>` characters trigger `AppDebugAssert` ([B:DEBUG] only).
3. *Comment strip*: the first `#` and everything after it is removed (no escape; `#` cannot appear in any value).
4. The first `\n` and the first `\r` are replaced by NUL (so CRLF and LF files both work).

**Line length**: buffer starts at 128 chars and doubles; doubling from 65 536 raises `AppReleaseAssert("Text file contains line with more than 65536 characters")` → maximum line length 65 536 bytes [C] `TextReader::DoubleMaxLineLen`. Longest line in scope: `data/gestures.txt` 2 767 chars [C].

**Tokenisation** (`GetNextToken`): separator set `" \t\n\r:,"` (space, tab, LF, CR, colon, comma) unless a parser calls `SetSeperatorChars` (none in this scope). A token is a maximal run of non-separators; leading separators are skipped; returns `NULL` at end of line. Consequences used by every grammar below: `Positions: 8`, `Positions 8` and `Positions,8` are identical; `0,1,2` is three tokens; **no value may contain a space, tab, colon, comma or `#`** (file/sample/marker/event names included). `TokenAvailable()` is true iff a non-separator remains on the line. `GetRestOfLine()` returns the remainder after skipping separators (not used by the formats here).

**Number parsing**: C `atoi`/`atof` (C locale, `.` decimal point, no error detection — junk parses as 0, `atof(NULL)` crashes) except where noted; `iv_atof(s) = iv_trunc_low_nibble(atof(s))` is used where the value feeds the deterministic simulation (`.shp` geometry, `stats.txt`) [C] `contrib/systemiv/lib/math/math_stubs.cpp:iv_atof`.

**End-of-file quirk** [C] `TextDataReader::ReadLine`: when reading from an archive, the call that copies the *last* line returns `false` if that line is not terminated by `\n`; every `while (ReadLine())` loop in this draft therefore silently drops an unterminated final line in packaged builds, while the loose-file reader (`TextFileReader`) returns it. All shipped files in scope end with CRLF [C] survey, so the quirk is latent. **Rule for authors: always end text files with a newline.**

**Blank lines**: whether a blank or comment-only line is legal depends on the parser loop: loops that start with `if (!TokenAvailable()) continue;` skip them; loops of the form `ReadLine(); tok = GetNextToken(); while (stricmp(tok, "END"))` crash on them (`stricmp(NULL, …)`). Each grammar below states which applies.

---

<a id="fmt-redshirt"></a>
## [→ 29_data_formats_reference] 29.B2 "redshirt2" obfuscated text

Purpose: light obfuscation of text files written into the user profile in non-debug builds (saved `game.txt` / mission files) [C] `code/level_file.cpp:LevelFile::SaveMissionFile`, `code/global_world.cpp` (writer `new TextFileWriter(path, true)` under `#ifndef TARGET_DEBUG`). No file under `data/` is obfuscated [C] grep for `redshirt2` in `data/` = 0 hits. Any text resource *may* be obfuscated because detection is automatic in `TextReader::CleanLine`.

Encoding [C] `contrib/systemiv/lib/filesys/text_file_writer.cpp:TextFileWriter::TextFileWriter`, `TextFileWriter::printf`; decoding `text_stream_readers.cpp:TextReader::CleanLine`:

| Item | Value |
|---|---|
| Magic | ASCII `redshirt2` written at file start **without** a newline (the first plaintext line follows on the same physical line) |
| Key table `s_offsets[16]` | 31, 7, 9, 1, 11, 2, 5, 5, 3, 17, 40, 12, 35, 22, 27, 2 |
| Counter | `k` starts at 0 for the file and persists across lines; it is advanced **before** use and only for encoded characters: `k = (k+1) mod 16` |
| Encoded characters | bytes with signed value > 32 (i.e. 33..127); space, tab, CR, LF and bytes ≥ 128 pass through unchanged and do not advance `k` |
| Encode | `e = p + s_offsets[k]; if (e >= 128) e -= 95` |
| Decode | `p = e - s_offsets[k]; if (p < 33) p += 95` |

Because the first offset used is `s_offsets[1] = 7`, a re-implementation must pre-increment. The scheme is a printable-range Vigenère; it offers no security. A new game should use a real format (e.g. a binary save with a version header) and need not keep this. A Unicode writer variant exists (`code/unicode/unicode_text_file_writer.cpp`) and the UTF-16 reader recognises `L"redshirt2"` [C] `unicode_text_stream_reader.cpp`.

---

<a id="fmt-shp"></a>
## [→ 29_data_formats_reference] 29.B3 `.shp` — hierarchical flat-shaded mesh ("shape")

**Purpose.** All 3-D models (buildings, units, props) are rigid hierarchies of *fragments* (sub-meshes with their own local transform, which code may animate) plus named *markers* (attachment frames). Text format, case-insensitive keywords [C] `code/lib/shape.cpp:Shape::Load`, `ShapeFragment::ShapeFragment(TextReader*,char const*)`, `ShapeFragment::Parse*Block`, `ShapeMarker::ShapeMarker(TextReader*,char const*)`. There is **no binary variant**: the only loader is this text parser and the exporter writes text [C] (no other `Shape` constructor reads data).

**Files.** `data/shapes/*.shp` — 139 files, 441 fragments, 425 markers in total; max 20 fragments and 38 markers per file; max marker depth 7; max 10 632 triangles, 6 245 vertices/positions and 121 colours in one fragment; 39 files use the strip encoding, 98 the triangle-list encoding, none mixes both, and 2 contain markers only (`battlecannonbase.shp` 8 markers, `portal.shp` 5 markers — their markers hang off `SceneRoot`) [C] survey `X1b_shp_survey.py` (per-shape table in `OUT/_work/drafts/P12_shapes.csv`). 138 files are CRLF, `primaryupgradeport.shp` is LF-only. Loaded as `data/shapes/<lower-cased name>` (loose, then archive `main.dat`) [C] `code/lib/resource.cpp:Resource::GetShapeCopy`, `DoesShapeExist`.

### Grammar (EBNF over *lines* after §29.B1 cleaning; `NAME`, `INT`, `F` are single tokens)

```ebnf
shp_file        = { top_line } ;
top_line        = fragment_def | marker_def | other_line ;          (* other_line: any line whose first token is
                                                                       neither FRAGMENT nor MARKER is ignored; blank ok *)
fragment_def    = "Fragment" NAME EOL , { frag_line } , geometry_end ;
frag_line       = "ParentName" NAME EOL
                | "up"    F F F EOL | "front" F F F EOL | "pos" F F F EOL
                | "Positions" INT EOL , { id_line_pos }              (* exactly INT records, see below *)
                | "Normals"   INT EOL , { id_line_vec }
                | "Colours"   INT EOL , { id_line_col }
                | "Vertices"  INT EOL , { id_line_vtx }
                | ignored_line ;                                     (* e.g. "right x y z", blank, unknown keyword *)
geometry_end    = strips_block | triangles_block ;                    (* parsing of the fragment STOPS here *)
id_line_pos     = INT F F F EOL ;          (* id, x, y, z *)
id_line_vec     = INT F F F EOL ;          (* id, nx, ny, nz *)
id_line_col     = INT INT INT INT EOL ;    (* id, r, g, b  (0..255) *)
id_line_vtx     = INT INT INT EOL ;        (* id, positionId, colourId *)
strips_block    = "Strips" INT EOL , strip , { strip } ;              (* INT strips, ids 0..INT-1 in order *)
strip           = "Strip" INT EOL , [ "Material" NAME EOL ] , "Verts" INT EOL , vref_lines ;
vref_lines      = { vref_line } ;          (* lines of tokens "v<INT>"; total token count = Verts *)
triangles_block = "Triangles" INT EOL , { tri_line } ;               (* exactly INT physical lines *)
tri_line        = INT INT INT EOL ;        (* vertex indices a b c, conventionally "a,b,c" *)
marker_def      = "Marker" NAME EOL , { marker_line } , ( "MarkerEnd" EOL | EOF ) ;
marker_line     = "ParentName" NAME | "Depth" INT | "front" F F F | "up" F F F | "pos" F F F | ignored_line ;
```

Record-block rules [C] `ParsePositionBlock`, `ParseNormalBlock`, `ParseColourBlock`, `ParseVertexBlock`:
- Lines whose first token does not start with a decimal digit are skipped inside id blocks (blank/comment lines allowed).
- Ids must be `0, 1, 2, …` consecutively. A wrong id in **Positions** silently abandons the block (positions stay empty; the fragment then has no geometry and a bounding radius from 0 points); in Normals/Colours/Vertices it is only a debug assert (release writes the record at the wrong index).
- EOF inside a block is a debug assert; release continues with garbage.
- **Triangles** are read as exactly T `ReadLine` calls with no skipping: a blank or comment line inside crashes (`atoi(NULL)`). All shipped triangle blocks are dense [C] survey.
- **Strips**: in `ParseAllStripBlocks` lines not starting with `Strip` are skipped; after `Strip k` the *next physical line* must be `Material …` or `Verts n` (the `Material` name is read and discarded); `n > 2` asserted; vertex tokens must start with `v` (debug assert) and `index < Vertices` (debug assert).
- A fragment without a `Strips`/`Triangles` line never terminates: the fragment loop does not recognise `Fragment`/`Marker`, so it swallows the rest of the file. (Not present in shipped data [C] survey: fragments = geometry blocks = 441.)
- `Normals` are parsed but **always discarded** — `GenerateNormals` replaces them with one face normal per triangle. Every shipped file has `Normals: 0` [C] survey.
- `right` lines (15 occurrences in `fieldgun_shell.shp`, `garbagecollector.shp`, `gunturret.shp`) and `# N triangles, M strips, ratio R` comments are ignored legacy exporter output [C] survey + parser.

### Field table

| Field | Type | Units | Default | Range / validation | Semantics | Parser ref |
|---|---|---|---|---|---|---|
| `Fragment` NAME | token | — | — | should be unique (debug assert only for fragments with a non-root parent); ≤100 fragments per file (debug assert, release stack overflow) | fragment name; looked up case-insensitively by code (`ShapeFragment::LookupFragment`) | `Shape::Load` |
| `ParentName` (fragment) | token | — | `"unknown"` | `SceneRoot` (case-insensitive; shipped files write `sceneroot`) or the name of another fragment | parent in the hierarchy; an unknown parent leaves the fragment unattached (debug assert; release: not rendered, leaked) | `Shape::Load` |
| `up`, `front` | 3×F | unit vectors | identity (`up`=(0,1,0), `front`=(0,0,1)) | normalised after parse | local orientation relative to parent; `Matrix34::Normalise`: `f←f/|f|; r←normalise(u×f); u←f×r` (so `up` is re-orthogonalised and `right` is derived) | `ShapeFragment` ctor, `code/lib/matrix34.cpp:Matrix34::Normalise` |
| `pos` | 3×F | world units | (0,0,0) | — | local translation relative to parent | same |
| `Positions` N + records | INT + id,3×F | world units, fragment-local | none | N ≤ 65 535 (indices are `unsigned short`) | unique vertex positions; `iv_atof` | `ParsePositionBlock` |
| `Colours` N + records | INT + id,3×INT | 0..255 | none | values > 255 wrap (stored in `unsigned char`) | palette; **on load every channel is multiplied by 0.85 and truncated** (`(unsigned char)(float(c)*0.85f)`, so 255→216), alpha forced 255 | `ParseColourBlock`, `code/lib/rgb_colour.cpp:RGBAColour::operator*=` |
| `Vertices` N + records | INT + id,posId,colId | indices | none | posId < Positions, colId < Colours (unchecked) | (position, colour) pairs actually referenced by faces | `ParseVertexBlock` |
| `Strips` S / `Strip` k / `Material` / `Verts` n / `v<i>` | INT / INT / NAME / INT / tokens | vertex indices | — | n > 2 | triangle strip; converted on load: for strip index i ≥ 2 with window (a,b,c) = (v[i-2],v[i-1],v[i]) a triangle is emitted only if a,b,c pairwise distinct (degenerates dropped); even i → (a,b,c), odd i → (c,b,a) (winding fix) | `ParseStripBlock` |
| `Triangles` T + records | INT + 3×INT | vertex indices | — | < Vertices (unchecked) | explicit triangle list | `ParseTriangleBlock` |
| `Marker` NAME | token | — | — | ≤100 per file (debug assert) | attachment frame; looked up by `LookupMarker` (case-insensitive, depth-first: own markers first, then children in order) or by substring scan (case-**sensitive** `strstr`) | `Shape::Load`, `ShapeFragment::LookupMarker`, `code/worldobject/building.cpp:Building::SetShapeLights`, `SetShapePorts` |
| `ParentName` (marker) | token | — | `"unknown"` | must name a fragment (debug assert) | fragment the marker is attached to | `Shape::Load` |
| `Depth` | INT | levels | **uninitialised** if absent | must equal (number of ancestors of the parent fragment including SceneRoot) + 1 … i.e. 1 for a marker on `SceneRoot`, 2 for a marker on a root-level fragment (debug assert `depth == -1` after the walk) | size of `m_parents[]`; `m_parents[0]` = SceneRoot … `m_parents[Depth-1]` = parent fragment | `ShapeMarker` ctor, `Shape::Load` |
| `up`/`front`/`pos` (marker) | 3×F | as fragments | identity | normalised | marker frame relative to its parent fragment | `ShapeMarker` ctor |
| `MarkerEnd` | keyword | — | — | optional for the last marker of a file (3 shipped files omit it: `clonelab.shp`, `controlstation.shp`, `portal.shp`) | ends the marker block | `ShapeMarker` ctor |

### Derived data computed at load [C] `ShapeFragment::RegisterPositions`, `GenerateNormals`
- Synthetic root fragment `SceneRoot` (identity transform, no geometry) holds all fragments whose parent is `SceneRoot`; markers are attached to their parent fragment and their `m_parents` chain is filled by walking up to `SceneRoot`.
- Bounding sphere per fragment: centre = AABB centre of its positions; radius = max distance to centre (`iv_sqrt`). A vertical-cylinder alternative is computed but never enabled (`m_useCylinder` stays false).
- Face normals: `n = normalise((b−a) × (c−b))` per triangle (flat shading); rendering emits the same normal at all three corners.
- `m_positionsInWS` scratch array is sized by **Vertices** but indexed by position id in accurate hit tests [C] `ShapeFragment::RayHit`, `SphereHit` → heap overrun when Positions > Vertices; shipped cases: `carryweights.shp` (two fragments, 100 positions / 50 vertices) and `sporegenerator.shp` (43/42) [C] survey. A re-implementation should size it by Positions.

### Transform semantics [C] `ShapeMarker::GetWorldMatrix`, `ShapeFragment::Render`
`Matrix34` stores rows `r,u,f,pos` (row-vector convention). A fragment's world matrix = `frag.T × parent.T × … × root.T × objectTransform`; a marker's = `marker.T × parents[Depth-1].T × … × parents[0].T × objectTransform`. Code may set per-fragment `m_angVel`/`m_vel` for prediction, or overwrite `m_transform` on shapes obtained with `GetShapeCopy(name, animating=true)` [C] `Resource::GetShapeCopy`.

### Marker usage (what markers are for)
Markers carry no data beyond a frame; meaning comes from the name the consuming class looks up [C] grep of `LookupMarker` and marker literals in `code/`:

| Mechanism | Names | Consumer | Purpose |
|---|---|---|---|
| prefix scan (`strstr`, case-sensitive) | `MarkerLight*` | `Building::SetShapeLights` | building signal-light positions |
| prefix scan | `MarkerPort*` | `Building::SetShapePorts` | operator "ports" where Darwinians stand to man a building (position pushed 5 wu out from the building and dropped to the terrain) |
| exact | `MarkerStatus` (+ `MarkerStatus0%d`) | `building.cpp` (control pad), `gunturret.cpp`, `generator.cpp`, `spiritreceiver.cpp` | status-light positions |
| exact | `MarkerEnd` | `code/worldobject/entity_leg.cpp` | foot/tip of a leg segment shape (also appears as a marker *name* in `spider_leg_*.shp`, `tripod_leg_part.shp`) |
| exact / `%d`-numbered | `MarkerBarrel`, `MarkerKills`, `MarkerBarrelEnd0%d`, `MarkerShells0%d` | `gunturret.cpp` | barrel mount, kill-counter, muzzle points, shell ejection |
| exact | `MarkerTurret`, `MarkerEntrance`, `MarkerBarrelEnd` | `tank.cpp` | turret mount, unit entrance, muzzle |
| exact | `MarkerEntrance`, `MarkerFlag` | `armour.cpp` | Darwinian load/unload point, team flag |
| exact | `MarkerFlag` | `officer.cpp` | officer flag |
| exact | `MarkerCarry` | `armyant.cpp` | carried-object point |
| exact | `MarkerLaser`, `MarkerBrass`, `MarkerEye1`, `MarkerEye2` | `insertion_squad.cpp` | muzzle, casing ejection, eye glints |
| exact | `MarkerSpiritLink`, `MarkerDoor` | `spawnpoint.cpp` | link-line origin, door |
| exact | `MarkerSpiritLink`, `MarkerHead`, `MarkerStatus0%d` | `spiritreceiver.cpp` | link origin, head, lights |
| exact | `MarkerDestination1/2`, `MarkerSurface` | `trunkport.cpp` | portal destinations, surface |
| exact | `MarkerFuel`, `MarkerTip`, `MarkerEntrance`, `MarkerBooster`, `MarkerWindow0%d` | `rocket.cpp`; `MarkerBooster` also `weapons.cpp` | escape-rocket fuel inlet, tip, door, exhaust, windows |
| exact | `MarkerEngine%d` | `harvester.cpp`, `spaceship.cpp` | engine exhausts |
| exact | `MarkerEggLay`, `MarkerLegX` (X replaced by leg index) | `spider.cpp`; `MarkerLegX` also `tripod.cpp` | egg spawn point, leg roots |
| exact | `MarkerEggs`, `MarkerTail0%d` | `sporegenerator.cpp` | egg launch, tail segments |
| exact | `MarkerTail` | `souldestroyer.cpp` | tail |
| exact | `MarkerPrimitive0%d`, `MarkerSpike0%d`, `MarkerRay0%d` | `constructionyard.cpp` | build spots, spikes, rays |
| exact | `MarkerLightPos`, `MarkerDishPos`, `MarkerReprogrammer%d`, `MarkerConsole%d` | `controltower.cpp` | |
| exact | `MarkerDishDirection` | `controlstation.cpp` | |
| exact | `MarkerFocus` | `radardish.cpp`, `feedingtube.cpp` | beam focus |
| exact | `MarkerPowerLocation`, `MarkerCounter`, `MarkerGlow0%d`, `MarkerStatus0%d` | `generator.cpp` | |
| exact | `MarkerSpirits`, `MarkerExit`, `MarkerDock`, `MarkerSpiritIncoming0/1/2` | `incubator.cpp` | |
| exact | `MarkerTrack1/2`, `MarkerContents1/2/3`, `MarkerWheel01/02/03`, `MarkerCounter` | `mine.cpp` | |
| exact | others: `MarkerBlueprint` (`blueprintstore.cpp`), `MarkerSignal` (`bridge.cpp`), `MarkerSpawnPoint` (`cave.cpp`), `MarkerSpiritStore` (`factory.cpp`), `MarkerFence01/02` (`laserfence.cpp`), `MarkerResearch%02d` (`library.cpp`), `MarkerGrab1/2` (`researchitem.cpp`), `MarkerConnectionLocation` (`switch.cpp`), `MarkerTeleportEntrance` (`teleport.cpp`), `MarkerLaunchPoint`, `MarkerTriffidStem` (`triffid.cpp`) | as listed | many of these classes are [B:LEGACY] (Darwinia buildings blocked by `BLOCK_OLD_DARWINIA_OBJECTS`; see P06/P07) |

Marker names present in data but not referenced by any code literal (authoring leftovers or Darwinia global-world data): `MarkerIsland*`, `MarkerTrunk<N>to<M>` (`globalworld_*.shp`), `MarkerFloat*`, `MarkerControl*`, `MarkerGun*`, `MarkerRocket*`, `MarkerParticleA/B`, `MarkerPlinth`, `MarkerFlowerCentre`, `MarkerBreach` (in `ark.shp`, `factory.shp`, `fieldgun.shp`, `gunturret.shp`, `help.shp`, `plant.shp`, `primaryupgradeport.shp`, `tank_body.shp`) [C] grep; whether any is built by `sprintf` elsewhere is [U] beyond the literal search.

### Writer (`Shape::WriteToFile`) — defects [C] `ShapeFragment::WriteToFile`, `ShapeMarker::WriteToFile`
The in-engine writer skips the fragment named `SceneRoot`, writes colours **without** undoing the 0.85 factor (round-tripping darkens), and uses broken `printf` formats (`"%5.2 "`, `"%7.3 "`, `"%6.3 "` with no conversion letter) for up/front/positions/normals/markers — it cannot have produced the shipped files (which look like `%5.2f` output). Only the exporter (§29.B4) is a valid writer. Callers of `WriteToFile` are tool-only [I].

### Minimal valid example (written for this design set)
```text
# one 2-triangle quad fragment on the root, one marker on it
Fragment: Plate
	ParentName: SceneRoot
	up:    0 1 0
	front: 0 0 1
	pos:   0 0 0
	Positions: 4
		0: -5 0 -5
		1:  5 0 -5
		2:  5 0  5
		3: -5 0  5
	Normals: 0
	Colours: 1
		0: 200 40 40
	Vertices: 4
		0: 0 0
		1: 1 0
		2: 2 0
		3: 3 0
	Triangles: 2
		0,2,1
		0,3,2

Marker: MarkerPort01
	ParentName: Plate
	Depth: 2
	up:    0 1 0
	front: 1 0 0
	pos:   6 0 0
	MarkerEnd
```

### Discrepancies samples ↔ parser
| Observation | Effect | Ref |
|---|---|---|
| `carrycube.shp` has two fragments named `Box04`, both parented to `sceneroot` | no assert fires (the duplicate-name assert only runs inside the parent search for fragments whose parent is *not* SceneRoot); both render; `LookupFragment("Box04")` returns the first in file order | [C] survey + `Shape::Load` |
| `carryweights.shp`, `sporegenerator.shp`: Positions > Vertices | heap overrun in accurate hit tests | [C] |
| 3 files lack the final `MarkerEnd` | harmless (EOF ends the marker) | [C] |
| `right:` lines, `Material` lines (140 in 35 files), strip format | ignored / supported legacy | [C] |
| Shipped `ParentName: sceneroot` (lower case, from exporter's `"scene root"` with spaces stripped) | matched case-insensitively | [C] `tools/ShapeExporter/ShapeExporter.cpp:DoExport` |

---

<a id="fmt-max"></a>
## [→ 29_data_formats_reference] 29.B4 `.max` sources and the shape tool chain (`tools/ShapeExporter`, `tools/3DGeometry`)

**`.max` files.** `data/shapes/source/*.max` — 133 files, 12 246 016 bytes total; Microsoft OLE2 *Compound File Binary* containers (`D0 CF 11 E0 A1 B1 1A E1`, "Composite Document File V2") = Autodesk 3ds Max scene files [C] magic bytes. Never read by the game; excluded from packaging by the `shapes/source` line of `tools/multiwinia_exclusions.txt` and by the extension filter of `main.dat` [C] `tools/build_multiwinia_data.bat`. 110 have a same-named `.shp` (case-insensitive); 23 `.max` have no `.shp` of that name (e.g. `AnthillNew`, `BattleCannon`, `GlobalWorld`, `LaserTroop`, `SpiderLegLower` — several correspond to renamed shapes such as `spider_leg_lower.shp`, `laser_troop.shp`, `flying_egg.shp`) and 29 `.shp` have no `.max` (e.g. `armyant2/3`, `carry*`, `clonelab`, `portal`, `pulsebomb`, `wallbuster`, `trunkport_mp`) [C] name comparison. Treat as Introversion art sources (IP register).

**ShapeExporter** (3ds Max scene-export plug-in, DLL; `ShapeExporter.dsp`/`ShapeExporterDLL.dsp`, MSVC 6) [C] `tools/ShapeExporter/ShapeExporter.cpp`, `ShapeExporter.h`, `DllEntry.cpp`:
- Registers extension `shp`, description "Futurewar Shape Format v1.0", version 100; class id `0x3288038e, 0x40eff553`.
- `DoExport`: creates root fragment `"scene root"` (spaces are stripped → `sceneroot`), walks the Max node tree from the scene root's children with depth 1; nodes whose name starts with `marker` (case-insensitive) become markers with `Depth` = recursion depth, all other geometry nodes become fragments (non-geometry, target, zero-face nodes skipped; children of a marker are not visited).
- Coordinate conversion (Max Z-up → game Y-up): every point `(x,y,z)` → `(x, z, −y)`. Local transform = `nodeTM × inverse(parentTM)` decomposed into rows `r,f(=Max row 1),u(=Max row 2),pos`; **fragment** `front` is stored **negated** `(−f.x, −f.z, f.y)` (which makes `r = u × f` hold for an un-rotated node), **marker** `front` is stored un-negated `(f.x, f.z, −f.y)` [C]; i.e. a marker's `front` points along the Max helper's local +Y while a fragment's points along −Y [I interpretation]. Both normalised.
- Positions de-duplicated within distance 0.001 (squared 1e-6); colours from Max vertex colours (×255, truncated) de-duplicated exactly, else one colour from the material diffuse ×255, else the node wire colour; vertices = unique (position, colour) pairs; faces written as a triangle list (`Triangles`), normals not exported (`Normals: 0`). `nvtristrip.h` is included but unused in this version → the 39 strip-format shipped shapes came from an older exporter [I].
- **Stale vs engine**: calls a 3-argument `ShapeFragment(name, parent, false)` constructor that no longer exists in `code/lib/shape.h` (2 arguments), and relies on the broken `WriteToFile` formats (§29.B3) → the exporter does not build against this tree as-is [C]. `[B:DEAD]` for the game.

**3DGeometry** [C] `tools/3DGeometry/main.cpp`, `3DGeometry.dsp`: an Allegro/AllegroGL test harness (camera fly-through with WASD/QE, grid, `DrawLine`, `DrawSphere`, `DrawTriPlane`) used to visualise ray–sphere / sphere–triangle intersection maths from `code/lib/math_utils` with hard-coded test vectors. Not a format and not a converter; [B:DEAD].

**Recommended new-engine stance** [I]: replace `.shp` with a modern interchange (glTF 2.0 with node hierarchy, per-vertex colour, no UVs, empties for markers); keep the semantics "fragment = node with local TRS, marker = named empty", the `(x,z,−y)` axis conversion only if importing old art.

---

<a id="fmt-bmp"></a>
## [→ 29_data_formats_reference] 29.B5 BMP bitmaps and textures (+ PNG output)

**Purpose.** Every image the game loads (textures, sprites, icons, help pictures, thumbnails, fonts, terrain ramps) is a Windows BMP decoded by `BitmapRGBA` [C] `code/lib/bitmap.cpp:BitmapRGBA::LoadBmp`, `ReadBMPFileHeader`, `ReadWinBMPInfoHeader`, `ReadOS2BMPInfoHeader`, `ReadBMPPalette`, `Read4BitLine`, `Read8BitLine`, `Read24BitLine`, `Read32BitLine`. Decoding semantics, pixel operations and upload are also described by P12 (§25.1–25.3); this section is the byte-level contract.

**Files** [C] survey `X1b_bmp_survey.py` (506 `.bmp` under `data/`):

| Directory | Count | Bit depth / info-header / compression |
|---|---|---|
| `animations/` | 25 | 24-bit ×6, 32-bit ×19 (all 19 carry real alpha) |
| `icons/` | 121 | 24 ×64, 32 ×21, 8 ×25, 4 ×11 |
| `mwhelp/` | 51 | 24 ×48, 32 ×3 |
| `sepulveda/` | 5 | 8-bit 256×256 |
| `sprites/` | 17 | 24 ×16 (9 use the pink key), 32 ×1 |
| `terrain/` | 36 | 24 ×32, 8 ×4 |
| `textures/` | 54 | 24 ×19, 32 ×5, 8 ×26, 4 ×3, **8-bit RLE8 ×1** (`textures/shaman_background.bmp`) |
| `textures/plusmenu/` | 2 | 24 |
| `textures/unicode/` | 138 | 24 ×90, 4 ×44, 8 ×4 |
| `thumbnails/` | 57 | 400×300; 8 ×53, 24 ×4 |

All use the 40-byte Windows `BITMAPINFOHEADER`; none is OS/2, top-down or `BI_BITFIELDS`; no file has row-padding or palette-offset layouts the loader would mis-read [C] survey hazard check.

### Byte layout accepted by the loader (little-endian)

| Offset | Size | Field | Loader behaviour |
|---|---|---|---|
| 0 | 2 | `bfType` | must be `0x4D42` "BM" (debug assert only) |
| 2 | 4 | `bfSize` | read, ignored |
| 6 | 2+2 | reserved | ignored |
| 10 | 4 | `bfOffBits` | **not used for seeking**; used only to derive the palette size |
| 14 | 4 | `biSize` | 40 → Windows header; 12 → OS/2 header; anything else (e.g. V4 108 / V5 124) → debug assert, release continues with an uninitialised header (garbage/crash) |
| 18.. (Win) | 4,4,2,2,4,4,4,4,4,4 | width, height, planes, bitCount, compression, sizeImage, xppm, yppm, clrUsed, clrImportant | only width, height (as **unsigned** — negative/top-down heights are unsupported), bitCount, compression used; `clrUsed` ignored |
| 18.. (OS/2) | 2,2,2,2 | width, height, planes, bitCount | compression = 0 |
| 54 (Win) / 26 (OS/2) | `ncol`×4 / ×3 | palette, B,G,R(,reserved) | `ncol = (bfOffBits−54)/4` (Win) or `(bfOffBits−26)/3` (OS/2); no clamp to 256 (overflow if larger); palette alpha = 255 |
| immediately after the palette | — | pixel rows, **bottom-up**, file row 0 → `m_lines[0]` (the bottom row; matches GL texture origin) | read sequentially; any gap between palette and `bfOffBits` would be decoded as pixels |

Pixel row decoding:

| bitCount | Per pixel | Row padding consumed? | Notes |
|---|---|---|---|
| 4 | nibble index; **high nibble = left pixel** | **no** | rows must be a multiple of 4 bytes (width multiple of 8) or the image shears; odd widths write one pixel past the row |
| 8 | byte index | **no** | width must be a multiple of 4 |
| 24 | B,G,R; alpha := 255 | yes (`(4 − 3w) & 3` bytes) | |
| 32 | B,G,R,A (alpha used as-is) | n/a | the only way to ship a real alpha channel |
| other (1, 16) | — | — | debug assert; release leaves pixels uninitialised |

`biCompression` must be `BI_RGB` (0) — debug assert only. **RLE4/RLE8 are not decoded**: in release `textures/shaman_background.bmp` (RLE8, 512×512, used by `code/shaman_interface.cpp`) would be read as raw indices, running past the data (`BinaryDataReader::ReadU8` returns 0xFF at EOF) → corrupt image [C] parser + survey. Whether a player can reach the Shaman interface in Multiwinia is owned by P09/P05 ([U] here).

Extension rule: `BitmapRGBA::Initialise(reader, type)` asserts `type == "bmp"` (debug only) and always calls `LoadBmp` — `.jpg`, `.tex`, `.dds` (named in the archive `dir` filters for Xbox/legacy) cannot be decoded on PC [C].

### Texture pipeline rules (what a texture *name* means)
[C] `code/lib/resource.cpp:Resource::GetTextureWithFlags`, `GetTexture`, `GetTextureWithAlpha`, `ProcessBmp`, `GetBitmap`, `DoesTextureExist`; `code/lib/bitmap.cpp:ConvertToTexture`.
1. A texture name is a path **relative to `data/` including the extension** (e.g. `sprites/darwinian.bmp`, `terrain/water_default.bmp`). Code builds most names with literal `.bmp` or `"." TEXTURE_EXTENSION`, where `TEXTURE_EXTENSION` is `#define`d unconditionally as `"bmp"` [C] `code/lib/universal_include.h`; 33 uses (help images, thumbnails, mode images). [I] The macro exists so an Xbox build could switch to `tex` (DXT `.tex` files are produced only by the Xbox branch of the data script via `convert_bmp_to_tex.cmd`, which is absent from the tree).
2. Lookup order: (a) texture cache `m_textures` keyed by the **exact name string** (case-sensitive `hash_map<std::string>`); (b) registered bitmap store `m_bitmaps` (run-time generated images such as `water_lightmap`, case-insensitive); (c) file: name lower-cased → `GetBinaryReader` → loose `data/<name>` then archive (§29.B16). Missing → `AppReleaseAssert("Failed to load texture %s")` (fatal) — callers that may miss guard with `DoesTextureExist` (e.g. thumbnails fall back to `thumbnails/default.bmp`, `code/UI/MapImageButton.h`).
3. **Flags are not part of the cache key**: the first request for a name decides mask/alpha/mipmap processing for all later requests [C].
4. Flags: `GetTexture(name, mipmapping=true, masked=true, compressed=false)` → `WithMipmaps|WithMask`. **Default is masked**, so every BMP requested through the default overload is colour-keyed.
5. `WithMask` → `ConvertPinkToTransparent`: pixels exactly equal to RGBA (255,0,255,**255**) become (128,128,128,0) — the colour key. 24-bit and paletted files always have alpha 255 so the key works; a 32-bit pixel with RGB magenta and alpha ≠ 255 is not keyed.
6. `WithAlpha` (`GetTextureWithAlpha`, 9 call sites) → `ConvertColourToAlpha`: alpha := **green** channel, RGB := white (luminance-style masks authored as greyscale). The header comment says "luminance"; code uses green only (docs-vs-code below).
7. Upload: RGBA8; GL path requires power-of-two only when `GL_ARB_texture_non_power_of_two` is missing or pref `ManuallyScaleTextures` ≠ 0, square when `GL_ARB_texture_rectangle` is missing; D3D path always power-of-two. Resizing rounds **up** to the next power of two (bilinear blit) and the power-of-two rule overrides max-size down-scaling; wrap CLAMP, MAG LINEAR, MIN LINEAR_MIPMAP_LINEAR (even when not mip-mapped) [C] — full algorithm in P12 §25.2. Non-power-of-two assets shipped: 11 in `animations/`, 27 in `icons/`, 49 in `mwhelp/`, 21 in `textures/`, 9 in `terrain/` (`waves_*` 400×10), all 57 `thumbnails/` (400×300) [C] survey.
8. `GetBitmap(name)` (CPU access, used by ramps, animation panel sizes, fonts) caches the decoded bitmap under `name`; a later `GetTexture` on the same name finds it in the bitmap store and **modifies it in place** (pink→transparent) [C].

### PNG
No PNG reader. `WritePng`/`SavePng` (libpng, 8-bit RGB, rows flipped to top-down, `Z_BEST_SPEED`) are used for screenshots `screenshotN.png` [C] `code/lib/poster_maker.cpp` and the Vista game-explorer thumbnail [B:DEAD, `TARGET_OS_VISTA`] `code/app.cpp`. `SavePng` ignores its `_saveAlpha` argument [C]. `WriteBmp` writes 24-bit Windows BMPs (2835 px/m).

### Minimal valid example
A 2×2 24-bit BMP (70 bytes): `42 4D 46 00 00 00 00 00 00 00 36 00 00 00` · `28 00 00 00 02 00 00 00 02 00 00 00 01 00 18 00 00 00 00 00 10 00 00 00 13 0B 00 00 13 0B 00 00 00 00 00 00 00 00 00 00` · bottom row `FF 00 FF  00 00 FF  00 00` (pink key, red, 2 pad bytes) · top row `00 FF 00  FF 00 00  00 00` (green, blue). Requested via `GetTexture`, the bottom-left pixel becomes transparent grey.

### Discrepancies
- `textures/shaman_background.bmp` is RLE8 → undecodable (above).
- `bitmap.h` comment on `ConvertColourToAlpha` ("luminance") vs code (green channel).
- 4-/8-bit row padding not consumed: latent (all shipped widths are multiples of 8) [C] survey.

---
