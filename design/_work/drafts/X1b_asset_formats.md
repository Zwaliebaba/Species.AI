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
| `up`, `front` | 3×F | unit vectors | identity (`up`=(0,1,0), `front`=(0,0,1)) | normalised after parse | local orientation relative to parent; `Matrix34::Normalise`: `f←normalise(f); r←normalise(u×f); u←f×r` (so `up` is re-orthogonalised and `right` is derived) | `ShapeFragment` ctor, `code/lib/matrix34.cpp:Matrix34::Normalise` |
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
<a id="fmt-colour-ramp"></a>
## [→ 29_data_formats_reference] 29.B6 Terrain colour ramps (`data/terrain/landscape_*`, `water_*`, `waves_*`)

**Purpose.** Three per-level BMP look-up images colour the landscape, the flat sea texture and the animated wave mesh. A level names them in its `Landscape_StartDefinition` block with `landColourFile`, `wavesColourFile`, `waterColourFile` (single token each, file name only, resolved as `terrain/<name>`); defaults `landscape_default.bmp`, `waves_default.bmp`, `water_default.bmp` [C] `code/level_file.cpp:LevelFile::ParseLandscapeData` (keywords at L789–L799), `LevelFile` constructors (L1409–L1411). When the Christmas mod is active the three are forced to `landscape_icecaps.bmp`, `waves_earth.bmp`, `water_icecaps.bmp` [C] `code/landscape_renderer.cpp:LandscapeRenderer::LandscapeRenderer`, `code/water.cpp:Water::Water`, `Water::Render` (`Location::ChristmasModEnabled`).

**Files and pixel formats** [C] survey: 23 `landscape_*.bmp` 64×64 24-bit; 4 `water_*.bmp` 128×128 8-bit paletted; 9 `waves_*.bmp` 400×10 24-bit. No ramp uses the pink key. The code scales look-ups by the bitmap's actual width/height, so other sizes are legal.

### Landscape ramp — 2-D look-up (slope × height)
[C] `code/landscape_renderer.cpp:LandscapeRenderer::GetLandscapeColour`, `BuildColourArray`; loaded with `Resource::GetBitmap` (CPU only, no texture).

| Axis | Bitmap coordinate | Driven by | Mapping |
|---|---|---|---|
| x (column) | `x = int(u · width)`, clamped 0..width−1 | triangle slope: `gradient = normal.y` (1 flat … 0 vertical) | `u = (1 − gradient)^0.4` → **left column = flat, right column = vertical**; the 0.4 exponent devotes most columns to gentle slopes |
| y (row of `m_lines`, row 0 = first row stored in the file = **bottom row of the picture as viewed**) | `y = int(v · height)`, clamped | triangle centre height `h` (sea level 0) | `v = 1 − h/highest + jitter`, `highest` = max height-map value; jitter = `sfrand(0.45/(max(h,0)+2))` (uniform ±0.1125 at sea level, shrinking with height; RNG reseeded per triangle from its x,z) → **top row of the picture = sea level, bottom row = highest peak of this map** |

So an artist paints the ramp with the sea-level beach at the top, peaks at the bottom, flat ground on the left and cliffs on the right. Colour is per triangle (flat-shaded, written to the provoking vertex) and alpha is forced 0 in "negative" render mode. Burn-patch darkening and the full algorithm are in P11 §23.11.

### Water texture — tiled RGB(A) texture
[C] `code/water.cpp:Water::Render`, `Water::RenderFlatWaterTiles`. Requested as `GetTexture("terrain/<waterColourFile>", mipmaps=true, masked=true)`, bound on texture unit 0 with `GL_REPEAT` wrap, LINEAR / LINEAR_MIPMAP_LINEAR filters, `GL_MODULATE`. It is mapped onto the flat sea quad grid at y = −9.0 covering `[−sizeX/2, 1.5·sizeX] × [−sizeZ/2, 1.5·sizeZ]` (twice the land extent) with texture coordinates running from `t/30` to `t/30 + 30` on both axes, `t = g_gameTimer.GetAccurateTime()` seconds → the image **repeats 30 times across the sea plane and scrolls diagonally by 1/30 of a tile per second** (one full tile every 30 s). Unit 1 multiplies by the generated shore mask `water_lightmap`. The pixel content is therefore an ordinary seamlessly-tiling colour texture (no axis semantics).

### Waves ramp — 1-D colour table
[C] `code/water.cpp:Water::Water` (table build), `Water::UpdateDynamicWater`, `code/water.h:Water::GetColour`.
- Only **row y = 1** of the bitmap is read (the second row from the bottom of the picture as viewed): `table[x] = pixel(x, 1)`, alpha := 255 (0 in negative mode), `numColours = width` (400 for all shipped files). The other 9 rows are ignored.
- Per animated-water vertex: `brightness = (h_prev1 + h_prev2 + h_this) · 4.0 · (1 − depth) + shoreNoise`, where `h` are wave heights, `depth` ∈ [0,1] the shore-mask depth (1 = open sea), `shoreNoise = ((1−depth) + sfrand(1−depth)·0.25)·250`; the two vertices of a strip pair mix 0.7/0.3; `index = Round(brightness)` clamped to [0, width−1] → `colour = table[index]`.
- Hence **x = 0 is calm open water; increasing x is brighter, wave-crest and shoreline foam** (open sea at rest maps to index 0; the shoreline adds up to ~250) [I: interpretation of the formula; arithmetic C].

### Minimal valid examples
- `landscape_example.bmp`: 64×64 24-bit; columns 0–40 green→brown, columns 41–63 grey rock; top rows sand, bottom rows snow.
- `waves_example.bmp`: 400×2 24-bit; row 1 (top row as viewed in a 2-row image) gradient from dark blue (x=0) to white (x=399); row 0 unused.
- `water_example.bmp`: any 128×128 seamlessly tiling texture.

### Discrepancies
- 9 landscape ramps are not referenced by any level (`beach`, `containment`, `field`, `icecaps_peaked`, `marble2`, `mine2`, `ridges`, `volcanic`, `volcanicplain`) [C] P11 grep; `waves_*` usage is P11's to tabulate.
- The D3D build path in `Water::Water` writes `bmp.GetPixel(x,1)` on a `const BitmapRGBA *` → would not compile under `USE_DIRECT3D` [C] `code/water.cpp` L119 (supports P13's doubt about D3D buildability).

---

<a id="fmt-anim"></a>
## [→ 29_data_formats_reference] 29.B7 `.anim` — animated help panel (2-D key-framed sprites)

**Purpose.** A short looping 2-D cartoon shown at the bottom of the screen during a Multiwinia game's grace period explaining the game mode [C] `code/multiwiniahelp.cpp:MultiwiniaHelp::InitialiseAnimatedPanel` (loaded from `MultiwiniaHelp::Advance` when `Multiwinia::GameInGracePeriod()` and no panel exists; freed by `MultiwiniaHelp::Reset`), `RenderAnimatedPanel` (drawn only while in the grace period, not in the tutorial, and `m_gracePeriodTimer >= GRACE_TIME`; background black at alpha 0.9, title = localised game-type name upper-cased in the title font at size 30, caption = phrase `multiwinia_animdesc_<GAMETYPE name>` word-wrapped at size 18 in the editor font). Parser [C] `code/animatedpanel.cpp:AnimatedPanel::Read`, `AnimatedPanelObject::Read`, `AnimatedPanelKeyframe::Read`; interpolation `AnimatedPanel::CalculateObjectProperties`, `CalculateRampUpAndDown`; renderer `code/animatedpanel_renderer.cpp:AnimatedPanelRenderer::RenderObjects`, `Blit`. Writer `AnimatedPanel::Write` exists (no caller in the game) — the files were authored with the external **`animator.exe`** at the repository root (no source in tree; its data archive `animator.dat` is a RAR containing `data/animations/*.bmp`, `data/fonts/verdana-bold.bmp`, `data/dialogs/animator-*.txt`) [C] strings in `animator.exe`, RAR listing of `animator.dat` via `OUT/_work/scripts/P14_rar_list.py`. [B:FULL].

**Files** (game type → file, [C] `InitialiseAnimatedPanel`):

| Game type (`Multiwinia::m_gameType`) | File | WIDTH×HEIGHT | START–STOP s | Objects | Keyframes |
|---|---|---|---|---|---|
| `GameTypeSkirmish` (Domination) | `animations/domination.anim` | 800×250 | 0.00–15.01 | 28 | 169 |
| `GameTypeKingOfTheHill` | `animations/koth.anim` | 800×250 | 0.00–15.66 | 17 | 108 |
| `GameTypeCaptureTheStatue` | `animations/cts.anim` | 924×250 | 0.00–20.01 | 23 | 111 |
| `GameTypeAssault` | `animations/assault_defend.anim` | 900×250 | 0.00–12.02 | 14 | 135 |
| `GameTypeRocketRiot` | `animations/rocket.anim` | 1000×250 | 0.00–30.01 | 21 | 173 |
| `GameTypeBlitzkreig` | `animations/blitz.anim` | 900×250 | 0.00–24.01 | 28 | 180 |
| `GameTypeTankBattle` | none → no panel | | | | |

[C] survey `X1b_anim_survey.py`. All 25 bitmaps in `data/animations/` are referenced by at least one `.anim`; no `.anim` references a missing image.

### Grammar
```ebnf
anim_file   = { header_line | object | ignored_line } ;          (* blank lines skipped *)
header_line = ( "WIDTH" INT | "HEIGHT" INT | "START" F | "STOP" F ) EOL ;
object      = "BEGIN" "Object" EOL , { keyframe | ignored_line } , "END" EOL ;
keyframe    = "BEGIN" "Keyframe" EOL , { kf_field | ignored_line } , "END" EOL ;
kf_field    = ( "TIMEINDEX" F | "IMAGENAME" NAME | "XPOS" F | "YPOS" F | "COLOUR" INT
              | "SCALE" F | "ANGLE" F | "FLIPHORIZ" INT | "ALPHA" INT ) EOL ;
```
Keywords case-insensitive (`stricmp`); unknown keywords ignored at every level; header lines may appear anywhere outside objects (last wins). Hazards: a `BEGIN` line without a second token, or a keyframe field without a value, crashes (`stricmp/atof(NULL)`); `IMAGENAME` is copied into a 256-byte buffer unchecked. An `END` at top level is ignored.

### Field table
| Field | Type | Units | Default | Range in shipped data | Semantics |
|---|---|---|---|---|---|
| `WIDTH`, `HEIGHT` | int | panel units | 600, 300 | 800–1000, 250 | logical canvas; on screen `scale = ScreenH/1300`, panel `screenW = WIDTH·scale`, `screenH = HEIGHT·scale`, centred horizontally, `screenY = ScreenH − HEIGHT·1.15·scale`; border 5 px, title band 25 px, caption band 15 px [C] `InitialiseAnimatedPanel`, `Resource::RegenerateOpenGlState` |
| `START`, `STOP` | float | s | 0, 30 | 0 / 12.02–30.01 | loop window: playback clock starts at **0** (not START) on first render, advances by wall-clock seconds, and when it exceeds STOP is set to START (overshoot discarded) [C] `RenderObjects` |
| `TIMEINDEX` | float | s | 0.0 | 0–30.66 | key time; keys after STOP are never reached (e.g. `cts.anim`, `domination.anim`, `koth.anim` have keys at 30.00 with STOP < 30) |
| `IMAGENAME` | token | file | `darwinian.bmp` | 25 names | sprite `animations/<name>`; loaded with `GetBitmap` (size) and `GetTexture` (default flags → mip-mapped, **pink-keyed**) |
| `XPOS`, `YPOS` | float | panel units | 100, 100 | x 28–940, y −75–233 | **centre** of the sprite; screen = `screenX + XPOS·scale'`, `screenY + YPOS·scale'` with `scale' = screenW/WIDTH`; y grows downward |
| `SCALE` | float | × source pixels | 0.3 | 0.0–2.5 | sprite size = bitmap width/height × SCALE × scale' |
| `ANGLE` | float | degrees | 0 | −1000–1500 | rotation about the sprite centre (`Vector3::RotateAroundZ(2π·ANGLE/360)` applied to the quad corners in screen space); values beyond ±360 make spins when interpolated |
| `COLOUR` | int enum | — | 0 | 0–3 | 0 `ColourNone` = white; 1 `ColourFriendly` = local team colour (`Location::GetMyTeam()->m_colour`); 2 `ColourEnemy` = colour of the first team (index order) whose type ≠ unused and which is not a friend of the local team (white if none); 3 `ColourInvisible` = alpha 0 |
| `FLIPHORIZ` | int bool | — | 0 | 0/1 | mirror horizontally (texture u from 1 to 0) |
| `ALPHA` | int | 0–255 | 255 | 0–255 | vertex alpha (ignored when COLOUR = 3) |

### Playback semantics (route also to 26) [C] `CalculateObjectProperties`, `CalculateRampUpAndDown`
For each object (drawn in file order, so later objects are on top) at time `t`:
1. `left` = keyframe with the greatest time ≤ t (first in file order on ties); `right` = keyframe with the smallest time ≥ t.
2. Neither → not drawn. Only `right` → **not drawn** (the object does not exist before its first key). Only `left` → hold `left`. Same keyframe, or `|right.t − left.t| < 0.01` → `left`.
3. Otherwise `lin = (t − left.t)/(right.t − left.t)`; `ramp = ease(lin)` with `ease(f) = 2f²` for f < 0.5, `1 − 2(1−f)²` otherwise (smooth in/out; documented in `docs/ImplementationNotes/RampUpAndDown.gif` [I]). **XPOS, YPOS, SCALE use `ramp`; ANGLE and ALPHA use `lin`** (ALPHA truncated to int); IMAGENAME, COLOUR, FLIPHORIZ step (taken from `left`).
4. Draw a textured quad, blend state left from `RenderBackground` (`SRC_ALPHA, ONE_MINUS_SRC_ALPHA`), clipped to the panel rectangle (`Renderer::Clip`).

### Minimal valid example
```text
WIDTH      800
HEIGHT     250
START      0.00
STOP       4.00

BEGIN Object
	BEGIN Keyframe
		TIMEINDEX       0.00
		IMAGENAME       darwinian.bmp
		XPOS            100.00
		YPOS            150.00
		COLOUR          1
		SCALE           0.50
		ANGLE           0.00
		FLIPHORIZ       0
		ALPHA           255
	END
	BEGIN Keyframe
		TIMEINDEX       4.00
		IMAGENAME       darwinian.bmp
		XPOS            700.00
		YPOS            150.00
		COLOUR          1
		SCALE           0.50
		ANGLE           360.00
		FLIPHORIZ       0
		ALPHA           0
	END
END
```

### Discrepancies
- Keys beyond `STOP` (above) are dead data [C].
- The sprite bitmaps in `data/animations/` duplicate several icons/sprites under other paths (e.g. `flag*.bmp` also in `icons/`, `darwinian.bmp` also in `sprites/`, `textures/`); they are separate files and may differ — asset agents compare contents [U].

---

<a id="fmt-gestures"></a>
## [→ 29_data_formats_reference] 29.B8 `data/gestures.txt` — gesture-classifier training set

**Purpose.** Training vectors for the Rubine-style mouse-gesture recogniser (algorithm, symbol meaning and liveness: P09 §3). Loaded at start-up by `App::App` → `new Gesture("gestures.txt")` → `Gesture::LoadTrainingData` → `TrainSystem` [C] `code/app.cpp` L390, `code/gesture.cpp:Gesture::LoadTrainingData`. Written only by the gesture editor (`GESTURE_EDITOR`, [B:DEBUG]) via `Gesture::SaveTrainingData` [C] `code/interface/gesture_window.cpp`. Gestures are not used by Multiwinia play (P09) — the file is still parsed and is required (`AppReleaseAssert("Couldn't open Gesture data file")`).

### Grammar (strictly line-positional — no blank or comment lines allowed between records)
```ebnf
gestures_file = count_line , { symbol } ;            (* exactly numSymbols symbol records *)
count_line    = INT EOL ;                            (* numSymbols *)
symbol        = trainer_count_line , [ vector_line ] ;   (* vector_line present iff trainerCount > 0 *)
trainer_count_line = INT EOL ;                       (* trainerCount, must be ≤ 32 *)
vector_line   = { F } EOL ;                          (* trainerCount × 14 floats on ONE line: f0..f13 per trainer *)
```
Each `ReadLine()` is followed by unguarded `GetNextToken()` calls: a blank line, a comment-only line or a vector split over several lines makes `atoi/atof(NULL)` crash [C]. `trainerCount > 32` (`GESTURE_MAX_TRAINERS`) overflows `Classification::m_trainers[32]` (no check) [C] `code/gesture.h`.

| Field | Type | Range | Semantics |
|---|---|---|---|
| numSymbols | int | shipped 8 | number of symbol classes; symbol id = record order (0 Squad … 7 Armour, P09 §3.7) |
| trainerCount | int | 0–32; shipped 20 each | number of training examples |
| f0 | float | always 0.000000 | unused slot (features are 1-based) |
| f1..f13 | float | see P09 §3.4 | Rubine features of one recorded stroke (pixels, radians, px²/ms², ms) |

Writer asymmetry [C] `SaveTrainingData`: writes `"%d\n"` then the vectors as `"%f "` and always a trailing `"\n"` — for a symbol with 0 trainers this produces an **empty line that the loader will read as the next trainer count** (crash). Shipped file: 17 lines (1 + 8 × 2), each vector line 280 values + trailing space [C].

Minimal valid example (1 symbol, 1 trainer; values illustrative):
```text
1
1
0.000000 0.500000 0.866025 120.000000 0.785398 80.000000 0.707107 0.707107 150.000000 0.100000 0.300000 0.050000 2.500000 400.000000 
```

---

<a id="fmt-gesture-demo"></a>
## [→ 29_data_formats_reference] 29.B9 `data/gesture_demos/*.txt` — recorded gesture strokes

**Purpose.** Mouse-path recordings replayed by Dr Sepulveda's tutorial to demonstrate each gesture. Parser [C] `code/gesture_demo.cpp:GestureDemo::GestureDemo`, `GetCoords`, `GetTotalDuration`; recorder `GestureRecordingThread` writes `gesture_demos/new_gesture_demo.txt`; loader `code/lib/resource.cpp:Resource::GetGestureDemo` (`gesture_demos/<name>`). **All of it is inside `#ifdef USE_SEPULVEDA_HELP_TUTORIAL`, which no target defines → [B:DEAD]**; the files are also excluded from packaging (`gesture_demos/*` in `tools/multiwinia_exclusions.txt`) [C].

Files: `airstrike.txt` (59 samples), `armour.txt` (72), `controller.txt` (47), `engineer.txt` (39), `grenade.txt` (16), `officer.txt` (27), `rocket.txt` (45), `squad.txt` (36) — names = lower-case gesture symbol / task names [C]. Coordinates range x 0.271–0.971, y 0.091–0.645 [C] survey.

### Grammar
```ebnf
demo_file = { sample_line } ;
sample_line = F F EOL ;        (* x y, both divided by screen HEIGHT at record time; "%.3f %.3f" *)
```
Every physical line becomes a sample (blank line → `atof(NULL)` crash). Fixed sample period **40 ms** (`GESTURE_DEMO_SAMPLE_PERIOD`, 25 Hz); duration = samples × 0.040 s. Playback: `index = t·25`, interpolate between `floor` and `ceil` samples, then multiply both coordinates by the current `ScreenH()` (so the stroke keeps its shape and is scaled by screen height; x origin left, y origin top). **Defect**: the interpolation weights are swapped (`factor1 = frac` multiplies the *lower* sample), so motion between samples runs backwards within each 40 ms step [C].

Minimal example: three lines `0.500 0.200`, `0.550 0.300`, `0.600 0.200` (a 120 ms "V").

---

<a id="fmt-effects-txt"></a>
## [→ 29_data_formats_reference] 29.B10 `data/effects.txt` — DSP filter blueprints (audio, not visual)

**Purpose.** Declares the 14 real-time audio filter types and their parameters (min, max, default, data type) used by `EFFECT` blocks of `sounds.txt` and by the sound editor [C] `code/sound/soundsystem.cpp:SoundSystem::LoadEffects`, `DspBlueprint::GetParameter`; parameter consumption `code/sound/sound_instance.cpp:DspHandle::Advance`. Despite the original spec listing "effect definitions (effects.txt)" under animation/VFX, the file is **audio-only** (route 10/27). Required at start-up (`AppReleaseAssert("Couldn't load effects.txt")`).

### Grammar
```ebnf
effects_file = { blank_or_comment | effect } ;
effect       = "EFFECT" NAME EOL , { param_line } , "END" EOL ;
param_line   = NAME F F F INT EOL ;      (* name min max default dataType *)
```
Top level skips blank lines; any other first token is debug-asserted to be `EFFECT` and treated as one. Inside an effect, **the line after `EFFECT` and every following line must be non-blank** (`stricmp(NULL,"END")` crash) [C]. **Binding is positional**: effect index = order of `EFFECT` blocks = `SoundLibrary3d` filter enum (`code/sound/sound_library_3d.h`), parameter index = line order within the block; the effect *name* matters only for `sounds.txt` look-up (case-insensitive) and the editor; parameter names are labels only. A file with a different number/order of effects silently re-binds DSP types.

| Index | EFFECT name (file) | Enum | Params (name min..max default, type) |
|---|---|---|---|
| 0 | DSound_Chorus | `DSP_DSOUND_CHORUS` | WetDryMix 0..100 50 f; Depth 0..100 10 f; Feedback −99..99 25 f; Frequency 0..10 1.1 f; Waveform 0..1 1 L; Delay 0..20 16 f; Phase 0..4 3 L |
| 1 | DSound_Compressor | `DSP_DSOUND_COMPRESSOR` | Gain −60..60 0 f; Attack 0.01..500 10 f; Release 50..3000 200 f; Threshold −60..0 −20 f; Ratio 1..100 3 f; PreDelay 0..4 4 f |
| 2 | DSound_Distortion | `DSP_DSOUND_DISTORTION` | Gain −60..0 −18 f; Edge 0..100 15 f; PostEQCentreFreq 100..8000 2400 f; PostEQBandwidth 100..8000 2400 f; PreLowpassCutoff 100..8000 8000 f |
| 3 | DSound_Echo | `DSP_DSOUND_ECHO` | WetDryMix 0..100 50 f; Feedback 0..100 50 f; LeftDelay 1..2000 500 f; RightDelay 1..2000 500 f; PanDelay 0..1 0 L |
| 4 | DSound_Flanger | `DSP_DSOUND_FLANGER` | WetDryMix 0..100 50 f; Depth 0..100 100 f; Feedback −99..99 −50 f; Frequency 0..10 0.25 f; Waveform 0..1 1 L; Delay 0..4 2 f; Phase 0..4 2 L |
| 5 | DSound_Gargle | `DSP_DSOUND_GARGLE` | RateHz 1..100 20 L; WaveShape 0..1 0 L |
| 6 | DSound_I3DL2Reverb | `DSP_DSOUND_I3DL2REVERB` | Room −10000..0 −1000 L; RoomHF −10000..0 −100 L; RoomRolloffFactor 0..10 0 f; DecayTime 0.1..20 1.49 f; DecayHFRatio 0.1..2 0.83 f; Reflections −10000..1000 −2602 L; ReflectionsDelay 0..0.3 0.01 f; Reverb −10000..2000 200 L; ReverbDelay 0..0.1 0.01 f; Diffusion 0..100 100 f; Density 0..100 100 f; HFReference 20..20000 5000 f; Quality 0..3 2 L |
| 7 | DSound_ParamEQ | `DSP_DSOUND_PARAMEQ` | Centre 80..16000 8000 f; Bandwidth 1..36 12 f; Gain −15..15 0 f |
| 8 | DSound_WavesReverb | `DSP_DSOUND_WAVESREVERB` | InGain −96..0 0 f; ReverbMix −96..0 0 f; ReverbTime 0.01..3000 1000 f; HighFreqRTRatio 0.01..0.99 0.01 f |
| 9 | ResonantLowPass | `DSP_RESONANTLOWPASS` | CutoffFrequency 100..20000 2500 f; Resonance 0.1..20 7 f; Gain 0..10 1 f |
| 10 | BitCrusher | `DSP_BITCRUSHER` | BitRate 1..16 16 f |
| 11 | Gargle | `DSP_GARGLE` | WetDryMix 0..100 50 f; RateHz 0.01..100 20 f; WaveShape 0..1 0 L |
| 12 | Echo | `DSP_ECHO` | WetDryMix 0..100 50 f; Delay 1..3000 200 f; Attenuation 0..100 50 f |
| 13 | SimpleReverb | `DSP_SIMPLE_REVERB` | WetDryMix 0..100 50 f |

(f = dataType 0 float, L = dataType 1 `long`.) [C] `data/effects.txt` + enum. Filter maths: P16 §10.7.

Field semantics [C]:
| Field | Type | Use |
|---|---|---|
| NAME | token ≤ 255 chars | label shown in editor; also used by `sounds.txt` `EFFECT` lookup |
| min, max | float | editor slider limits only (not enforced at runtime) |
| default | float | **not applied at runtime**: parameters omitted in a `sounds.txt` EFFECT block stay `SoundParameter()` = fixed 0.0. The sound editor's "add effect" seeds new effects through `GetParameter(i, &defaultVal)` which fills the **min** slot (argument-order bug), so editor-created effects start at their *minimum* [C] `code/interface/soundeditor_window.cpp` L551 |
| dataType | int 0/1 | at DSP update, value is passed as `float` (0) or `long` reinterpreted into the float array (1); any other value → `AppReleaseAssert("Unknown datatype")` [C] `DspHandle::Advance` |

Hazard [C]: `DspHandle::Advance` loops over all `MAX_PARAMS = 13` slots and calls `GetParameter(i, …, &dataType)` even for slots beyond the blueprint's parameter count, leaving `dataType` uninitialised → possible spurious "Unknown datatype" assert. Only reached when pref `SoundDSP` ≠ 0 (default 0, P16).

Minimal valid example:
```text
EFFECT MyEcho
    WetDryMix   0   100   50   0
    Delay       1   3000  200  0
END
```
(as the 13th block it would become `DSP_ECHO` regardless of its name).

---

<a id="fmt-stats-txt"></a>
## [→ 29_data_formats_reference] 29.B11 `data/stats.txt` — per-entity-type base stats

**Purpose.** Health, speed and reload rate for every `Entity` type [C] `code/worldobject/entity.cpp:EntityBlueprint::Initialise`, `EntityBlueprint::GetStat`, `Entity::SetType` (copies into the instance, L168). Required (`AppReleaseAssert("Couldn't open stats.txt")`).

### Grammar
```ebnf
stats_file = { blank_or_comment | stat_row } ;
stat_row   = NAME F F F EOL ;        (* name, health, speed, rate *)
```
**Positional**: the k-th non-blank row is entity type k (`Entity` enum order); the NAME column is stored (`EntityBlueprint::m_names`) but never used to bind. More than `NumEntityTypes` (27) rows → `AppReleaseAssert("Too many entity blueprints defined")`; fewer rows leave the rest 0. Values parsed with `iv_atof` (deterministic).

| Row | Enum | NAME in file | HLTH | SPED | RATE |
|---|---|---|---|---|---|
| 0 | `TypeInvalid` | Invalid | 0 | 0 | 0 |
| 1 | `TypeLaserTroop` | LaserTroop | 20 | 15 | 1 |
| 2 | `TypeEngineer` | Engineer | 255 | 28 | 0 |
| 3 | `TypeVirii` | Virii | 10 | 32 | 0 |
| 4 | `TypeSquadie` | **InsertionSquadie** (label differs from `Entity::GetTypeName` "Squadie") | 75 | 30 | 0.5 |
| 5 | `TypeEgg` | Egg | 10 | 0 | 0 |
| 6 | `TypeSporeGenerator` | SporeGenerator | 200 | 20 | 10 |
| 7 | `TypeLander` | Lander | 255 | 40 | 1 |
| 8 | `TypeTripod` | Tripod | 155 | 20 | 1 |
| 9 | `TypeCentipede` | Centipede | 255 | 80 | 1 |
| 10 | `TypeSpaceInvader` | SpaceInvader | 10 | 40 | 1 |
| 11 | `TypeSpider` | Spider | 200 | 50 | 1 |
| 12 | `TypeDarwinian` | Darwinian | 50 | 20 | 3 |
| 13 | `TypeOfficer` | Officer | 255 | 30 | 3 |
| 14 | `TypeArmyAnt` | ArmyAnt | 20 | 25 | 0 |
| 15 | `TypeArmour` | Armour | 200 | 60 | 5 |
| 16 | `TypeSoulDestroyer` | SoulDestroyer | 255 | 150 | 1 |
| 17 | `TypeTriffidEgg` | TriffidEgg | 50 | 100 | 1 |
| 18 | `TypeAI` | AI | 255 | 255 | 255 |
| 19 | `TypeShaman` | Shaman | 200 | 25 | 2 |
| 20 | `TypeHarvester` | Harvester | 255 | 28 | 0 |
| 21 | `TypeNuke` | Nuke | 255 | 100 | 0 |
| 22 | `TypeTentacle` | Tentacle | 255 | 100 | 0 |
| 23 | `TypeMeteor` | Meteor | 255 | 350 | 0 |
| 24 | `TypeTank` | Tank | 150 | 50 | 2 |
| 25 | `TypeSpaceShip` | SpaceShip | 255 | 500 | 0 |
| 26 | `TypeDropShip` | DropShip | 255 | 255 | 0 |

[C] `data/stats.txt`; enum names from `code/worldobject/entity.cpp:Entity::GetTypeName` order (enum identifiers per P03).

| Field | Type | Units (file header) | Semantics / quirks |
|---|---|---|---|
| HLTH | float | hit points | initial/maximum health |
| SPED | float | "m/s" = world units per second | `GetStat(type, StatSpeed)` returns `SPED × (1 + m_difficultyLevel/10)` for every type except `SpaceInvader`; `App::m_difficultyLevel` is forced to 0 by `App::UpdateDifficultyFromPreferences` (preference read commented out); `Location::Init` copies a level file's explicit difficulty into it, or stores the current value (0) into a level that has none (−1) [C] `code/app.cpp` L632, `code/location.cpp` L347–L356 → in Multiwinia the factor is 1.0 unless a level file carries `CreatedAsDifficulty n` (`code/level_file.cpp:LevelFile::ParseDifficulty`; no `mp_*` level does, only 3 Darwinia-era files) [C] grep |
| RATE | float | seconds between shots | reload time |

**Storage truncation** [C] `code/worldobject/entity.h` (`unsigned char m_stats[NumStats]`), `entity.cpp` L168: per-instance copies are `unsigned char` → fractional values truncate (Squadie RATE 0.5 → 0) and values > 255 wrap; code that reads `EntityBlueprint::GetStat` directly (e.g. `code/worldobject/insertion_squad.cpp` L347 reload) sees the double. A re-implementation must decide which to honour (the shipped balance was tuned against this behaviour — owned by P03/P04).

Minimal valid example: 27 rows `Name h s r` in enum order, e.g. first two `Invalid 0 0 0` / `LaserTroop 20 15 1`.

---

<a id="fmt-sounds-txt"></a>
## [→ 29_data_formats_reference] 29.B12 `data/sounds.txt` — sound-event blueprints (full grammar)

**Purpose.** Binds every (sound source, event name) to zero or more sound instances with playback parameters, and defines sample groups. Parser [C] `code/sound/soundsystem.cpp:SoundSystem::LoadBlueprints`, `ParseSoundEvent`, `ParseSoundEffect`, `ParseSampleGroup`, `NewSampleGroup`, `GetSampleGroup`; `code/sound/sound_parameter.cpp:SoundParameter::Read`, `GetParameterType`, `GetLinkType`, `GetUpdateType`; writer `SaveBlueprints`, `WriteSoundEvent`, `WriteSampleGroup`, `SoundParameter::Write` ([B:DEBUG] sound editor). Runtime semantics of every field: P16 §10.1–10.8 (route 10). Validated against the shipped file by `OUT/_work/scripts/X1b_sounds_grammar.py` (this agent's independent state-machine; P16's `P16_parse_sounds.py` produced `P16_sound_events.csv`).

### Grammar
```ebnf
sounds_file    = { blank_or_comment | source_block | samplegroup | ignored_top } ;
source_block   = source_kind TYPENAME EOL , { blank_or_comment | event_block } , "END" EOL ;
source_kind    = "ENTITY" | "BUILDING" | "OTHER" ;
event_block    = "EVENT" EVENTNAME EOL , { event_field } , "END" EOL ;      (* NO blank/comment lines inside *)
event_field    = "SOUNDNAME"     NAME EOL
               | "SOURCETYPE"    INT EOL          (* 0 Sample, 1 SampleGroupRandom *)
               | "POSITIONTYPE"  INT EOL          (* 0 Type2D, 1 Type3DStationary, 2 Type3DAttachedToObject, 3 TypeInEditor *)
               | "INSTANCETYPE"  INT EOL          (* 0 Polyphonic, 1 MonophonicRandom, 2 MonophonicNearest *)
               | "LOOPTYPE"      INT EOL          (* 0 SinglePlay, 1 Looped, 2 LoopedADSR *)
               | "TEAMMATCHTYPE" INT EOL          (* 0 AllTeams, 1 MyTeamOnly, 2 EnemyTeamOnly *)
               | "PRIORITY"      INT EOL          (* parsed and discarded *)
               | "MINDISTANCE"   F EOL            (* world units *)
               | param_field_name param_spec EOL
               | effect_block ;
param_field_name = "VOLUME" | "FREQUENCY" | "ATTACK" | "SUSTAIN" | "RELEASE" | "LOOPDELAY" ;
effect_block   = "EFFECT" EFFECTNAME EOL , { PARAMLABEL param_spec EOL } , "END" EOL ;   (* ≤ 13 params, positional *)
param_spec     = "PARAMETER" ( fixed | ranged | linked ) [ update_type ] ;
fixed          = "TypeFixedValue"   F ;                     (* value *)
ranged         = "TypeRangedRandom" F F F ;                 (* lower upper smooth *)
linked         = "TypeLinked"       F F F F F LINKNAME ;    (* inLower outLower inUpper outUpper smooth link *)
LINKNAME       = "Nothing" | "HeightAboveGround" | "Xpos" | "Ypos" | "Zpos" | "Velocity" | "CameraDistance" ;
update_type    = "UpdateConstantly" | "UpdateOncePerLoop" ;  (* absent → UpdateConstantly *)
samplegroup    = "SAMPLEGROUP" GROUPNAME EOL , { ANYWORD SAMPLENAME EOL } , "END" EOL ;  (* NO blank lines inside *)
ignored_top    = any other first token EOL ;                 (* e.g. a stray "END" at top level *)
```
Lexical rules: §29.B1 (all keywords and enumerator names case-insensitive via `stricmp`; `#` comments allowed only where blank lines are allowed).

Parser-structure facts [C]:
- **Blank/comment lines**: allowed at top level and between `EVENT` blocks inside a source block (that loop tests `TokenAvailable`); **forbidden inside `EVENT`, `EFFECT` and `SAMPLEGROUP` bodies** (`ReadLine(); GetNextToken(); stricmp(...)` → NULL crash). The shipped file obeys this (0 hazards) [C] survey.
- Inside a source block any non-`END` line is treated as an `EVENT` header (debug-assert only) and its second token becomes the event name.
- Source `TYPENAME` resolution (case-insensitive): `ENTITY` → `Entity::GetTypeId` over the 27 names `InvalidType, LaserTroop, Engineer, Virii, Squadie, Egg, SporeGenerator, Lander, Tripod, Centipede, SpaceInvader, Spider, Darwinian, Officer, ArmyAnt, Armour, SoulDestroyer, TriffidEgg, AI, Shaman, Harvester, Nuke, Tentacle, Meteor, Tank, SpaceShip, DropShip`; `BUILDING` → `Building::GetTypeId` over the 75 names of `Building::GetTypeName` (`Invalid` … `SmokeMarker`, P06); `OTHER` → `SoundSourceBlueprint::GetSoundSoundType` over `Laser, Grenade, Rocket, AirStrikeBomb, Spirit, Sepulveda, Gesture, Ambience, Music, Interface, CratePowerup, MultiwiniaInterface`. An unknown name returns −1 and is stored at index −1 (debug assert only → memory corruption in release). A **duplicate** source block for the same type replaces the earlier one (debug assert only) — not present in shipped data.
- Object name for the instance's event identifier = the TYPENAME token as written (≤127 chars; `"sample group"` for groups); identifier = `"<TYPENAME> <EVENTNAME>"` [C] `SoundInstance::SetEventName`.
- `SOUNDNAME` is lower-cased in place and its **last `.extension` stripped** (`RemoveExtension`) before storing (≤255 chars). With `SOURCETYPE 0` it names a sample file stem; with `SOURCETYPE 1` a `SAMPLEGROUP` name (looked up case-insensitively at play time; first match wins).
- Unknown event field → debug assert, then the line is skipped in release.
- `SoundParameter::Read`: if the type name is unknown, `m_type = −1` and no values are read; after the type-specific values, *if any token remains* it is taken as the update type (unknown → −1). Extra tokens beyond that are ignored. `Recalculate()` is called immediately.
- `EFFECT EFFECTNAME`: matched case-insensitively against `effects.txt` names (unknown → debug assert, type −1 → crash when used). Each following line: first token = label (ignored), rest = `param_spec`; parameter *k* binds to blueprint parameter *k* by position (shipped: every block lists exactly the blueprint's parameter count in blueprint order) [C] survey.
- `SAMPLEGROUP`: first token of each member line is ignored (shipped files always write `SAMPLE`); member name lower-cased and extension-stripped. Group names are stored as written (≤255). **Duplicate group names are allowed** and both kept; look-ups return the first: shipped `officercreate` is defined twice (identical) [C] survey + `GetSampleGroup`.

### Field table (event fields) — see P16 §10.4 for runtime semantics
| Field | Type | Default (`SoundInstance` ctor) | Valid values | Shipped usage (296 EVENT blocks) |
|---|---|---|---|---|
| SOUNDNAME | token | `"[???]"` | sample stem or group name | 296 (one is literally `[???]`: `OTHER Sepulveda EVENT TextAppear`, a dead source) |
| SOURCETYPE | int | 0 | 0,1 | 0:143, 1:153 |
| POSITIONTYPE | int | 2 | 0–3 | 0:83, 1:42, 2:169, 3:2 |
| INSTANCETYPE | int | 0 | 0–2 | 0:290, 1:3, 2:3 |
| LOOPTYPE | int | 0 | 0–2 | 0:210, 1:70, 2:16 |
| TEAMMATCHTYPE | int | 0 | 0–2 | 0:284, 1:8, 2:4 |
| PRIORITY | int | — | ignored | 0 |
| MINDISTANCE | float | 100.0 | > 0 | 296 |
| VOLUME | param | fixed 7.0 | 0–10 vol units | 296 |
| FREQUENCY | param | fixed 1.0 | multiplier | 119 |
| ATTACK / SUSTAIN / RELEASE / LOOPDELAY | param | fixed 0.0 | seconds | 27 / 10 / 64 / 18 |
| EFFECT … END | block | none | 0..n | 22 (Echo 12, DSound_Echo 5, Gargle 3, DSound_I3DL2Reverb 1, DSound_WavesReverb 1) |

Parameter tokens in shipped data: `TypeFixedValue` 512, `TypeRangedRandom` 79, `TypeLinked` 30; links Velocity 16, HeightAboveGround 7, CameraDistance 4, Ypos 3; update types `UpdateConstantly` 509, `UpdateOncePerLoop` 112 (none omitted) [C] survey. Shipped top level: 21 ENTITY, 35 BUILDING, 12 OTHER blocks, 131 SAMPLEGROUP blocks (130 distinct names, 1 126 member lines) [C] survey.

### Sample resolution
At play time a sample stem `s` resolves to `sounds/s.ogg`, else `mwsounds/s.ogg` (loose, then archives); if neither exists `CachedSample` raises `AppReleaseAssert("Failed to open sound stream decoder")` (fatal) [C] `code/sound/sample_cache.cpp:CachedSample::CachedSample`, `code/lib/resource.cpp:Resource::GetSoundStreamDecoder`. All 1 228 sample files referenced by shipped events/groups exist [C] survey (the only unresolved name is `[???]`).

### Writer format (editor output) [C] `WriteSoundEvent`, `SoundParameter::Write`
`\tEVENT %-20s` then fixed-order fields `SOUNDNAME, SOURCETYPE, POSITIONTYPE, INSTANCETYPE, LOOPTYPE, TEAMMATCHTYPE, MINDISTANCE %2.2f`, `VOLUME` always, `FREQUENCY` only if not fixed 1.0, `ATTACK/SUSTAIN/RELEASE/LOOPDELAY` only if not fixed 0.0, each as `%-18s PARAMETER %-18s` + `%8.2f` values (+ link name) + two spaces + update-type name; effects as `\t\tEFFECT %s` + one line per blueprint parameter (`\t\t\t`) + `\t\tEND`; `\tEND`. Sources are written in enum order preceded by a `# ====…` comment line and followed by `END` and two blank lines; sample groups as `\tSAMPLE  <name>`. Values are written with 2 decimals → **round-tripping through the editor quantises every parameter to 0.01** [C].

### Minimal valid example
```text
SAMPLEGROUP footsteps
	SAMPLE  step1
	SAMPLE  step2
END

ENTITY Darwinian
	EVENT Create
		SOUNDNAME          footsteps
		SOURCETYPE         1
		POSITIONTYPE       2
		INSTANCETYPE       0
		LOOPTYPE           0
		TEAMMATCHTYPE      0
		MINDISTANCE        100.00
		VOLUME             PARAMETER TypeRangedRandom       6.00     8.00     0.00  UpdateOncePerLoop
		FREQUENCY          PARAMETER TypeLinked             0.00     1.00    30.00     1.50     0.50  Velocity  UpdateConstantly
		EFFECT             Echo
			WetDryMix          PARAMETER TypeFixedValue        30.00  UpdateConstantly
			Delay              PARAMETER TypeFixedValue       200.00  UpdateConstantly
			Attenuation        PARAMETER TypeFixedValue        50.00  UpdateConstantly
		END
	END
END
```

### Discrepancies samples ↔ parser
- Duplicate `SAMPLEGROUP officercreate` (harmless) [C].
- `SOUNDNAME [???]` in a dead source [C].
- 16 sample files referenced by `data/sounds.txt` are listed in neither `tools/sounds.txt` nor `tools/menu_sounds.txt`. Five of them (`mwsounds/rollover1a`, `rollover1c`, `mouseclick1`, `chat`, `beep_harsh_malfunction`) are packed into `sounds2.dat` by the catch-all rule (§29.B13). The other **11 are excluded from every archive** by `tools/multiwinia_exclusions.txt` (`sounds/tresk*`, `trash80*`, `dma_sc*`, `twoataris*`, `pang*`, `speccyloader.ogg`, `matrix.ogg`, `fodder.ogg`) although `OTHER Music` events `Cutscene1–5`, `LoaderRaytrace`, `LoaderFodder`, `LoaderSoul`, `LoaderSpeccy`, `LoaderAmiga`, `LoaderMatrix`, `LoaderGameOfLife`, `EndSequence`, `Credits`, `StartSequence` reference them → triggering any of these in a packaged retail build would hit the fatal `CachedSample` assert. They are Darwinia cut-scene / loader-mini-program music ([B:LEGACY]; loaders are behind the undefined `USE_LOADERS`) [C] packaging rules + `data/sounds.txt`; [I] unreachable in Multiwinia (P16/P20 own the event reachability).

<a id="fmt-pack-list"></a>
## [→ 29_data_formats_reference] 29.B13 Packaging lists (`tools/sounds.txt`, `tools/menu_sounds.txt`, exclusion/inclusion lists)

**Purpose.** Build-time file lists that decide which archive each data file goes into [C] `tools/build_multiwinia_data.bat` (`:makeMenuSounds`, `:makeSounds`, `:makeSounds2`, `:makeMain`), `tools/listfiles.pl`, `tools/copyfiles.pl`. Not read by the game.

### Grammar
```ebnf
pack_list = { path_line } ;
path_line = RELPATH EOL ;            (* relative to data/, forward slashes, e.g. "mwsounds/attack1.ogg"; CRLF tolerated *)
```
`tools/sounds.txt` and `tools/menu_sounds.txt` are used twice: (1) `sed` prefixes `data/` (sounds.txt: every line; menu_sounds.txt: lines starting `sounds`/`mwsounds`) and the result is a RAR `@listfile` for `sounds.dat` / `menusounds.dat`; (2) both are concatenated with the platform exclusion list into `completeExclusions`, whose lines `listfiles.pl` treats as **`bsd_glob` patterns relative to `data/`** (a line beginning `**/` instead prunes any path component with that exact basename); matching is case-insensitive against the expanded paths [C] `listfiles.pl:wanted`. Exclusion/inclusion list grammar and per-target contents are tabulated by P14 §31.6.

Shipped contents [C] survey: `tools/sounds.txt` 1 297 entries (935 `sounds/`, 362 `mwsounds/`), `tools/menu_sounds.txt` 18 entries (10 `mwsounds/multilobbyamb1–10.ogg`; `sounds/shipheartbeat1`, `mouse2`, `mouse3`, `menuscroll2`, `menuscroll1`, `alien_lands3`, `evil`, `rumble1`), no overlap, no duplicates, all exist. Of the 1 332 Oggs on disk, 1 315 are listed; `sounds2.dat` receives the remaining non-excluded six (`mwsounds/rollover1a/1b/1c`, `mouseclick1`, `chat`, `beep_harsh_malfunction`); the other 11 are excluded (§29.B12 discrepancies).

Minimal example (`menu_sounds.txt`): `mwsounds/lobby1.ogg` ⏎ `sounds/click.ogg` ⏎.

---

<a id="fmt-ogg"></a>
## [→ 29_data_formats_reference] 29.B14 Ogg Vorbis samples (`data/sounds/*.ogg`, `data/mwsounds/*.ogg`)

**Container.** Ogg Vorbis I, decoded with libvorbisfile through custom callbacks over `BinaryReader` (`ov_open_callbacks`; read/seek/tell map to the loose-file or in-memory archive reader; seek is supported for both) [C] `code/sound/sound_stream_decoder.cpp:SoundStreamDecoder::ReadOggHeader`, `ReadOggData`, `ReadFunc`, `SeekFunc`, `TellFunc`. Output always 16-bit signed native-endian PCM (`ov_read(…, 2, 1, …)`). Validity checks (fatal `AppReleaseAssert`): `ov_open_callbacks == 0`, `ov_pcm_total > 0`, no `OV_HOLE`/`OV_EBADLINK` while reading; cached samples additionally require 1–8 channels, 1–268 435 456 frames, 1–192 000 Hz and ≤ 1 GiB of PCM [C] `code/sound/sample_cache.cpp:CachedSample::CachedSample`.

**WAV.** A RIFF/WAVE PCM reader (8/16-bit, mono/stereo, `fmt `/`data` chunks) exists [C] `SoundStreamDecoder::ReadWavHeader`, `ReadWavData`, selected by the reader's file extension `wav` — but `Resource::GetSoundStreamDecoder` always appends `.ogg` to the requested name, so **WAV files are unreachable** in Multiwinia [C] `code/lib/resource.cpp:Resource::GetSoundStreamDecoder`.

**Naming and placement.**
- A sample is addressed by its **stem** (no directory, no extension), lower-cased by the `sounds.txt` parser; resolution `sounds/<stem>.ogg` first, then `mwsounds/<stem>.ogg` [C] `CachedSample::CachedSample`, `Resource::SoundExists`. No stem exists in both directories [C] survey.
- Variants are numbered suffixes (`attack1` … `attack44`, `mine_cart_creak1` … ) and grouped by `SAMPLEGROUP` blocks [C] data.
- `sounds/` = 954 files inherited from Darwinia, all lower-case; `mwsounds/` = 378 Multiwinia files, 359 with upper-case letters (e.g. `Attack44.ogg`, `CrateReward_Tank.ogg`) — works because Windows file lookup and the archive hash table are case-insensitive (§29.B16) [C] survey.
- Debug-only authoring checks: sample names must not contain spaces and samples should be **mono** (`IsSoundSourceOK`, `_DEBUG`) [C] `code/sound/soundsystem.cpp:SoundSystem::IsSoundSourceOK`.

**Shipped properties** [C] survey `X1b_ogg_survey.py`: all 1 332 files mono; 1 330 at 44 100 Hz, 2 at 22 050 Hz; nominal bit-rate `sounds/` 64 kbps ×862, 128 ×89, 32/60/80 ×1 each; `mwsounds/` 128 ×369, 96 ×8, 40 ×1. Encoder/decoder tools in tree: `tools/oggenc.exe`, `tools/oggdec.exe` (binaries, not read).

---

<a id="fmt-shader"></a>
## [→ 29_data_formats_reference] 29.B15 HLSL shaders (`data/shaders/*`) — file-level conventions [B:D3D]

[C] `code/lib/shader.cpp:Shader::Shader`, `Shader::Create`, `MemFile`, `MyD3DXInclude::Open`; users `code/lib/FixedPipeline.cpp` (L25–27), `code/deform.cpp` (L63–65), `code/water_reflection.cpp:WaterReflectionEffect::WaterReflectionEffect`. The whole of `shader.cpp` is inside `#ifdef USE_DIRECT3D` → the files are **unused in the baseline OpenGL build**, although they are packed into both `init.dat` (all 14 via `tools/init.list`) and `main.dat` (all except the six `FixedFunc*` files, which `multiwinia_exclusions.txt` removes) [C]. Semantics belong to P12/P13.

| Convention | Rule |
|---|---|
| Source form | plain-text HLSL compiled **at run time** with `D3DXCompileShader` (no precompiled blobs, flags 0, no macros) |
| Entry points | `vs_main` in every vertex shader, `ps_main` in every pixel shader |
| Profiles | the device's best: `D3DXGetVertexShaderProfile(device)` / `D3DXGetPixelShaderProfile(device)` |
| Extensions | `.vs` vertex, `.ps` pixel, `.ps3` pixel variant requiring a `ps_3_x` profile (chosen when the profile string's 4th char > '2'; falls back to `.ps` if creation fails) [C] `water_reflection.cpp` |
| Includes | `#include "x"` resolved by `MyD3DXInclude` as resource `shaders/x` (loose or archive); only `globals.h` is included (by the three `deform*.ps`) and defines `DEFORM_PRECISION 10` |
| Pairing | a `Shader` object = one `.vs` + one `.ps` path; failure of either compile is debug-asserted; `Shader::Create` raises `AppReleaseAssert("Shader create error…")` if either object could not be created |
| Constants | set by name through the D3DX constant tables of both stages (`SetFloat`, `SetUniform`, `SetMatrix`, `SetBool`, `SetInt`) |

Pairs created [C]: `FixedFunc0.vs`+`FixedFunc0Maps.ps`, `FixedFunc1.vs`+`FixedFunc1Maps.ps`, `FixedFunc2.vs`+`FixedFunc2Maps.ps` (fixed-function emulation); `deform.vs` + `deformFullscreen.ps` / `deformSphere.ps` / `deformDiamond.ps`; `water-meshwaves.vs` + `water-meshwaves.ps3` or `.ps`. Line counts: FixedFunc0.vs 88, FixedFunc0Maps.ps 6, FixedFunc1.vs 28, FixedFunc1Maps.ps 9, FixedFunc2.vs 33, FixedFunc2Maps.ps 11, deform.vs 17, deformDiamond.ps 27, deformFullscreen.ps 21, deformSphere.ps 25, globals.h 4, water-meshwaves.ps 16, water-meshwaves.ps3 21, water-meshwaves.vs 23 [C].

---

<a id="fmt-dat-archive"></a>
## [→ 29_data_formats_reference] 29.B16 `.dat` archives (RAR)

**Container.** A `.dat` file is a standard **RAR 4.x archive** (signature `52 61 72 21 1A 07 00` "Rar!"), produced by `tools/Rar.exe` and read by the bundled UnRAR 3.30 beta 1 source (`contrib/unrar/version.h`: 3.30 β1, 14 Nov 2003; built as the `unrar` sub-project; supports unpack methods 1.5, 2.0/2.6 and 2.9) [C] `contrib/unrar/unrar.cpp:UncompressedArchive::UncompressedArchive`, `contrib/unrar/extract.cpp:CmdExtract::ExtractArchive` (modified to unpack into memory via `AllocNewFile` + `DataIO.SetUnpackToMemory`), `contrib/unrar/unpack.cpp:Unpack::DoUnpack`. The repository's `animator.dat` is an example: archive flags 0x0008 (solid), file headers unpack version 29, method 0x35 (best), names `data\animations\…` [C] RAR header walk. No runtime `.dat` is committed to the tree.

**Encryption / obfuscation.** None: every `ParseArchive` call passes `password = NULL`, and the build script never uses `-p`/`-hp` [C] `code/app.cpp`, `tools/build_multiwinia_data.bat`. (UnRAR's password support is compiled in but unused.) Text files inside may in principle be "redshirt2"-obfuscated (§29.B2) — none are.

**How archives are built** [C] `tools/build_multiwinia_data.bat`, `tools/build_language_dat.bat` (full step list in P14 §31.6):

| Archive | RAR command | Entry names | Content rule |
|---|---|---|---|
| `init.dat` | `rar a -m5 -s -idp init.dat @list` | `data\…` | `tools/init.list` (14 shaders, `sprites/darwinian.bmp`, `textures/ivlogo.bmp`) |
| `language.dat` | `rar a -m5 -s -idp` | `data\…` | `*.bmp *.dds *.tex *.txt` from the selected `*language.list` |
| `main.dat` | `rar a -m5 -s -idp` | `data\…` | all `data/` minus `completeExclusions`, minus any path containing `sounds/`, filtered to `*.tex *.bmp *.jpg *.shp *.txt *.vs *.ps *.ps3 *.h *.anim` (Xbox adds `*.wmv`) — so `.max`, `.psp`, `.grd`, `.ogg` never enter it |
| `menusounds.dat` | `rar a -m2 -s -idp` | `data/…` | `tools/menu_sounds.txt` |
| `sounds.dat` | `rar a -m2 -s -idp` | `data/…` | `tools/sounds.txt` |
| `sounds2.dat` | `rar a -m2 -s -idp` | `data/…` | every `.ogg` of `listAll` (i.e. not in `completeExclusions`) |
| `patch.dat`, `branding.dat` | not built in tree | | loaded if present |
| `ambx.dat` | `build_ambx_dat` (script absent) | | shipped by `tools/InstallerScript_retail_crashreporter_euro.iss` but **never parsed** by the code [C] grep |

Switches [I, RAR CLI]: `a` add, `-m5` best / `-m2` fast (Ogg already compressed), `-s` solid, `-idp` no percentage display, `@file` list file.

**How the runtime opens them** [C] `code/lib/resource.cpp:Resource::ParseArchive`, `GetUncompressedFile`, `GetTextReader`, `GetBinaryReader`, `GetShapeCopy`, `ListResources`; `contrib/unrar/unrar.cpp:MemMappedFile::MemMappedFile`; `contrib/systemiv/lib/tosser/hash_table.cpp:HashTable::HashFunc`, `GetIndex`.
1. Load order: `init.dat` (in `App::App`, before preferences), then `language.dat`, `main.dat`, `patch.dat`, `menusounds.dat`, `branding.dat` (`App::Initialise`), then `sounds.dat`, `sounds2.dat` (`App::LoadSounds`) [C] `code/app.cpp` L217, L334–L340, L502–L503. A missing file is skipped silently; an exception during extraction drops that archive.
2. The **entire archive is decompressed into RAM** (`UncompressedArchive`, at most `MAX_FILES = 16384` entries; excess entries make `AllocNewFile` return NULL → crash in the modified extractor). Each entry becomes a `MemMappedFile {name, data, size}`; `\` → `/`; the name is `strlwr`-ed; **zero-size entries (and directory entries) are dropped** (and leaked).
3. Entries go into one global `HashTable<MemMappedFile*>`; an entry whose name already exists replaces the old one → **later archives override earlier ones** (effective precedence sounds2 > sounds > branding > menusounds > patch > main > language > init).
4. The table's hash masks the ASCII case bit (`& 0xDF`) and compares with `stricmp` → **archive look-ups are case-insensitive** regardless of the caller's spelling.
5. Every resource request for `N` (relative to `data/`) tries, in order: the loose file `data/N` (relative to the working directory; on Win32 `fopen` is case-insensitive; on other OSes `FindCaseInsensitive` walks the directory tree), then the archive key `data/N`. **Loose files always win over archives** — patching/modding by dropping files into `data/` works; the Darwinia mod-directory step (`<profile>/mods/<mod>/N`) is compiled out (`USE_DARWINIA_MOD_SYSTEM` undefined) [C].
6. `ListResources(dir, filter)` merges the loose directory listing with archive entries whose directory part equals `data/<dir>` (case-insensitive) and whose file part matches the wildcard filter (`*`, `?`; `Resource::WildCmp` is **case-sensitive**, archive names are lower case) [C]. Entries are inserted with `OrderedInsert` (`contrib/systemiv/lib/filesys/file_system.cpp`), which keeps case-sensitive `strcmp` order and skips exact duplicates **except against the last element** → a loose file and its archive twin can both appear (always when their case differs, e.g. loose `mp_koth_2P_1.txt` vs archive `mp_koth_2p_1.txt`) [C].

**Minimal valid archive (for tests).** `rar a -m5 -s test.dat data\stats.txt` → loaded by `ParseArchive("test.dat", NULL)` it serves resource `stats.txt` unless a loose `data/stats.txt` exists.

**New-engine note [I].** RAR is a proprietary format (creating RAR archives needs the licensed `rar` tool; UnRAR source has a licence forbidding RAR-compatible archivers). Replace with a zip/PAK-style archive or a custom indexed pack, keeping the semantics: ordered mount list, last-mounted wins, loose-file override, case-insensitive keys, forward-slash names relative to `data/`.

---

<a id="fmt-psp"></a>
## [→ 29_data_formats_reference] 29.B17 `.psp` — Paint Shop Pro image (`data/icons/banner_template.psp`)

One file, 32 020 bytes. Header [C] byte dump: ASCII `"Paint Shop Pro Image File\n\x1a"` + 5 NUL bytes (32-byte signature), major version 3, minor 0, then a `~BK\0` general-image-attributes block (block length 38) giving width 64 and height 64. It is the layered authoring template for the seven 64×64 24-bit `icons/banner_*.bmp` task banners (`banner_absorb`, `banner_deploy`, `banner_follow`, `banner_formation`, `banner_goto`, `banner_none`, `banner_unload`) [I name + matching size]. Not referenced by any code or build script and not packaged (`*.psp` is not in the `main.dat` filter) [C]. Authoring-only; IP (art).

<a id="fmt-grd"></a>
## [→ 29_data_formats_reference] 29.B18 `.grd` — Photoshop gradient (`data/Untitled Gradients.grd`)

One file, 244 bytes. Header [C] byte dump: `"8BGR"` signature, version 3, gradient count 1, Pascal-string name (length 25) **"Foreground to Transparent"**, followed by colour/transparency stop records. This is Adobe Photoshop's (v3–5) gradient preset file. Not referenced by code or build scripts, not packaged (`.grd` not in any archive filter; the space in the name would also break the `dir`/`sed` list pipeline only cosmetically) [C] grep + `tools/build_multiwinia_data.bat`. **Confirmed unused** authoring residue; not IP-relevant beyond being a stock Photoshop preset name.

---

## [→ 29_data_formats_reference] 29.B19 Naming conventions and legacy content

<a id="fmt-icons"></a>
### `data/icons/` naming [C] `code/taskmanager_interface_icons.cpp`, `code/multiwiniahelp.cpp`, `code/interface/helpandoptions_windows.cpp`, `code/UI/mapDetailsButton.h`, grep of `"icons/` literals
| Pattern | Built from | Example |
|---|---|---|
| `icons/icon_<name>.bmp` | `GlobalResearch::GetTypeName(i)`, `Task::GetTaskName(type)`, or crate power-up name (`Crate::GetName`) | `icon_squad.bmp`, `icon_nuke.bmp`, `icon_hotfeet.bmp` |
| `icons/gesture_<name>.bmp` | research type name (gesture symbol picture) | `gesture_officer.bmp` |
| `icons/banner_<order>.bmp` | officer order / task banner | `banner_goto.bmp` |
| `icons/button_<pad button>[_shadow].bmp` | controller glyphs (Xbox 360 pad names: a, b, x, y, lb, rb, lt, rt, la, ra, dpad, tab) | `button_lb_shadow.bmp` |
| `icons/mouse_<cursor state>.bmp` | cursor images (`MouseCursor`) | `mouse_movehere.bmp` |
| `icons/menu_<level>.bmp`, `icons/menu_%dplayer.bmp` | menu badges | `menu_basic.bmp` |
| `icons/<free name>.bmp` | `MultiwiniaHelp` icon tabs (`"icons/%s.bmp"`), misc HUD | `compass.bmp`, `nukesymbol.bmp` |

Formats: 64×64 or 128×128 mostly; 4/8/24/32-bit (§29.B5). Referenced names absent from `data/icons/` (e.g. `menu_campaign.bmp`, `menu_prologue.bmp`, `menu_coop.bmp`, `menu_forcecoop.bmp`, `menu_<n>player.bmp`, `compass_empty.bmp`, `icon_enter.bmp`, `shaman_shadow.bmp`) would be fatal if requested without a `DoesTextureExist` guard — reachability is for the asset-catalogue agents [C] existence check.

<a id="fmt-mwhelp"></a>
### `data/mwhelp/` naming [C] `code/multiwiniahelp.cpp` (L72–89, L2590–2599, L2757–2759, L3392–3398), `code/interface/helpandoptions_windows.cpp` (L708, L849), `code/UI/GameTypeImageButton.h`, `code/UI/GameMenuWindow.cpp`, `code/UI/HelpImageButton.h`
| Pattern | Meaning |
|---|---|
| `mwhelp/general<page>.bmp` | general help pages (shipped 1–5) |
| `mwhelp/help_<gameType>_<page>.bmp` | per-mode help picture; `gameType` = `Multiwinia` game-type index 0 Domination … 5 Blitzkrieg; pages 1-based (shipped `help_0_1`, `help_1_1..2`, `help_2_1..3`, `help_3_1..3`, `help_4_1..4`, `help_5_1..3`); the text id is `multiwinia_helppage_<gameType+1>_<page>` |
| `mwhelp/mode_<blueprint m_name>.bmp` | game-mode tile; `m_name` is the `GAMETYPE` name from `data/multiwinia.txt` (`Domination`, `KingOfTheHill`, `CaptureTheStatue`, `Assault`, `RocketRiot`, `Blitzkrieg`, `TankBattle`) matched case-insensitively against the lower-case files `mode_domination.bmp` …; plus fixed `mode_prologue.bmp`, `mode_campaign.bmp` |
| `mwhelp/<topic>.bmp` | in-game help tab icons (`starthere`, `koth`, `spawnpoint`, `spawnpoint_assault`, `crates`, `liftstatues`, `capturestatues`, `wallbuster`, `pulsebomb`, `pulsebomb_attack`, `pulsebomb_defend`, `rocketriot1/2`, `reinforcements`, `blitzkrieg1/2`, `gear`, `control_camera[_french]`) |
| `mwhelp/helppage_<page>_<image>.bmp`, `basic_select[_xbox]`, `basic_promote[_xbox]`, `darwinian`, `officercontrol`, `darwiniancontrol` | referenced by code but **absent** from `data/mwhelp/` (Xbox/legacy help flows) [C] |
All names are built with `"." TEXTURE_EXTENSION` (= `.bmp`). Sizes: 150×150, 200×200, 300×200, 400×300, 512×128, 600×389, 426×406, 150×149 [C] survey.

<a id="fmt-thumbnail"></a>
### `data/thumbnails/` naming [C] `code/UI/MapImageButton.h:MapImageButton::Render`, `code/UI/GameMenuWindow.cpp` L4327
`thumbnails/<level file name without ".txt">.bmp`, **lower-cased** before look-up (`strlwr`); if `DoesTextureExist` fails → `thumbnails/default.bmp`. Loaded with `GetTexture(name, mipmaps=false, masked=false)` and drawn with LINEAR min/mag. Shipped: 57 files, 400×300 (53 8-bit paletted, 4 24-bit), names keep mixed case on disk (e.g. `mp_koth_2P_1.bmp`) which works only because file and archive look-ups are case-insensitive [C].

<a id="fmt-sepulveda"></a>
### `data/sepulveda/sepulveda1–5.bmp` [C] `code/sepulveda.cpp:Sepulveda::Sepulveda`, `Sepulveda::RenderFace`; `code/clouds.cpp:Clouds::RenderFace`
Five 256×256 8-bit portraits of the Darwinia mentor character (index 1–3 neutral, 4 "eyes shut", 5 "mouth open" per the code comments) cycled every 0.4 s with random zoom/offset. `sepulveda.cpp` is entirely inside `USE_SEPULVEDA_HELP_TUTORIAL` → [B:DEAD]; `Clouds::RenderFace` is compiled but only runs while `Clouds::m_faceTime > 0`, which nothing ever sets (initialised −1) → unreachable [C] grep. The files are still packed into `main.dat` (not excluded). IP: character art.

<a id="fmt-purity"></a>
### `data/purity/*` ("Purity Control" Darwinia promotional build) [C] file contents, `tools/multiwinia_exclusions.txt`, `code/script.cpp:Script::RunCommand_PurityControl`
| File | Format (owner of grammar) | Content |
|---|---|---|
| `purity/buyme.txt` | Darwinia cut-scene script (`Script` command list: `EnterLocation`, `CamMove`, `WaitFade`, `TriggerSound`, `Say`, `WaitCam`, `WaitSay`, `StopSound`, `PurityControl`) — P19 | "buy me" outro: says `puritycontrol_1..6`, then `PurityControl` (deletes the user's save directory `users/<profile>/`) |
| `purity/game.txt` | GlobalWorld game file (`Locations_`, `Buildings_`, `Research_`, `Events_` blocks) — P01/P11 | 11 Darwinia locations, research levels, events |
| `purity/map_mine.txt` | level map file (Landscape/tiles/buildings) — P11 | a "mine" map variant using `landscape_mine.bmp`, `waves_containmentfield.bmp`, `water_default.bmp` |
| `purity/mission_mine_enable.txt` | level mission file — P11 | camera mounts `start`, `buyme1` |
`PURITY_CONTROL` is never defined and the directory is excluded from every archive (`purity` line) → [B:LEGACY]/[B:DEAD] content; the `PurityControl` script opcode itself is compiled into [B:FULL] (reachable only from a script that uses it — none in Multiwinia data, P19).

---

## [→ 25_textures_and_assets] 25.X1b Asset-format rules the catalogue relies on (summary)

- Texture name = `data/`-relative path with extension; `.bmp` only (4/8/24/32-bit uncompressed, bottom-up, 40- or 12-byte info header); RLE unsupported (`textures/shaman_background.bmp` is RLE8) — §29.B5.
- Alpha sources: 32-bit BMP alpha; pink (255,0,255) colour key when requested `masked` (the default); green-channel-as-alpha when requested `WithAlpha` — §29.B5. The first request for a name fixes its processing (flags not part of the cache key).
- Power-of-two: only enforced at upload (rounding **up**); D3D always POT; GL only without `GL_ARB_texture_non_power_of_two` — §29.B5 / P12 §25.2.
- Archive packing, override order and case-insensitive look-up — §29.B16.
- Terrain ramps' axis semantics — §29.B6. Animation sprites are pink-keyed + mip-mapped — §29.B7.
- Authoring-only files never packed: 133 `.max`, 1 `.psp`, 1 `.grd`; dead/legacy images still packed: `sepulveda/*` — §29.B4, B17–B19.

## [→ 26_animation_and_vfx] 26.X1b `.anim` playback model (summary of §29.B7)

Per-object key-frame tracks over a looping wall-clock timeline (START..STOP s, first pass starts at 0), objects invisible before their first key and frozen after their last, position/scale eased with the quadratic in-out `ease(f)` (2f² / 1−2(1−f)²), angle/alpha linear, image/colour/flip stepped; colour roles *none / friendly-team / first-enemy-team / invisible* resolved at render time; sprites centred at (XPOS,YPOS) in a WIDTH×HEIGHT canvas scaled by `ScreenH/1300`. Only used for the grace-period mode explainer panel; no other animation format exists in the game (skeletal/legged animation is procedural code — `entity_leg`, owned by P04/P05).

## [→ 10_audio_design] 10.X1b Grammar cross-reference

The complete `sounds.txt` grammar (every keyword, enumerator name and value domain), the `effects.txt` blueprint table, packaging lists and Ogg conventions are §29.B10–B14 of this draft; P16 §10.1–10.8 documents runtime meaning. Points P16 should reflect: effect parameter defaults are not applied at runtime; editor seeds new effects with the parameter *minimum*; blank lines inside EVENT/EFFECT/SAMPLEGROUP crash the loader; 11 referenced Darwinia music/loader samples are excluded from all retail archives.

---

## Cross-partition notes
- **P14 (31.5)**: archive look-ups are case-insensitive (`HashTable::HashFunc` masks bit 0x20, `GetIndex` uses `stricmp`), so P14's "callers must pass lower-case names" is not required [C] `contrib/systemiv/lib/tosser/hash_table.cpp`. Also: zero-size entries are leaked, `MAX_FILES` overflow crashes rather than truncating.
- **P12 (24.5/25.x)**: shipped shape count is 139 `.shp` (P12 text says 140; `P12_shapes.csv` has 139 data rows). Add: two marker-only shapes (`battlecannonbase.shp`, `portal.shp`); `m_positionsInWS` overrun cases (`carryweights.shp`, `sporegenerator.shp`); `carrycube.shp` duplicate fragment name.
- **P11 (23.11/23.13)**: waves ramp uses only bitmap row y = 1 as a 1-D table indexed by wave brightness; water texture repeats 30× across the sea plane and scrolls 1/30 tile/s; D3D compile error in `Water::Water` (`bmp.GetPixel` on a pointer).
- **P16 (10.x/27.x)**: see 10.X1b above.
- **P09 (3.x)**: `SaveTrainingData` writes an empty line for zero-trainer symbols that `LoadTrainingData` cannot read; `GestureDemo::GetCoords` interpolation weights swapped (dead code).
- **P03/P04**: `stats.txt` is positional; per-instance stats are `unsigned char` (Squadie RATE 0.5 → 0 in `m_stats`, 0.5 via `GetStat`); speed scaled by `1 + difficulty/10` except SpaceInvader.
- **P13**: shader file conventions (§29.B15) and the D3D-only status of `data/shaders`.
- **P19**: `data/purity/buyme.txt` uses the cut-scene script grammar and the `PurityControl` opcode (deletes the profile's save files).
- **Asset catalogue agents**: the BMP survey script `X1b_bmp_survey.py --detail <subdir>` prints per-file size/depth/pink/alpha usage; referenced-but-missing icons/mwhelp names listed in §29.B19.

## Contradictions / open questions
1. P14 states archive keys must be requested in lower case; code shows case-insensitive look-up (above) — resolved in favour of code [C].
2. Original spec §5 places `effects.txt` under 26_animation_and_vfx; the file is audio DSP configuration → route to 10/27 [C].
3. `ShapeExporter` source vs shipped `.shp`: the in-tree exporter writes triangle lists and no `right:` lines, yet 39 shipped files are strip-encoded and 3 contain `right:` → files predate the current exporter [I].
4. Marker `front` sign convention differs from fragments in the exporter (§29.B4) — interpretation [I]; a test export would confirm.

## [U] Unknowns
1. Exact version of `tools/Rar.exe` (strings are packed; no version resource found) and whether retail `.dat` files were produced with RAR 2.9 or 3.x method numbering — resolve by running `Rar.exe` without arguments on Windows or by inspecting a retail `.dat` header (`animator.dat` shows unpack version 29).
2. Whether the Shaman interface (which loads the RLE8 `textures/shaman_background.bmp`) is reachable in Multiwinia play — resolve with P05/P09 reachability analysis or a run.
3. Whether data-only marker names (`MarkerIsland*`, `MarkerTrunk*to*`, `MarkerFloat*`, …) are looked up via a `sprintf` pattern not caught by the literal grep — resolve with a full call-graph search of `LookupMarker` arguments.
4. `animator.exe` behaviour (key editing, any fields it writes beyond `AnimatedPanel::Write`) — no source in tree; resolve by running the tool.
5. `.psp` / `.grd` internal block contents beyond the header (irrelevant to the game).
6. Content equality of duplicated images (`animations/flag*.bmp` vs `icons/flag*.bmp`, `animations/darwinian.bmp` vs `sprites/darwinian.bmp`) — resolve by hashing.

## Docs-vs-code discrepancies
| Doc / comment | Says | Code does | Ref |
|---|---|---|---|
| `code/lib/bitmap.h` comment on `ConvertColourToAlpha` | luminance → alpha | **green channel** → alpha, RGB := white | `code/lib/bitmap.cpp:BitmapRGBA::ConvertColourToAlpha` |
| `data/effects.txt` header | "Default: the starting value" | defaults are not applied to parameters omitted in `sounds.txt` (they are fixed 0); editor seeds with min | `soundsystem.cpp:ParseSoundEffect`, `soundeditor_window.cpp` |
| `data/stats.txt` header | `SPED` in m/s, `NAME` = creature name | rows bound by position; speed × (1 + difficulty/10); instance copy truncated to `unsigned char` | `entity.cpp:EntityBlueprint::Initialise`, `GetStat`, `Entity::SetType` |
| `docs/changes.txt` (Darwinia) | mods in `mods/<name>/`, "stats.txt reloaded when a mod is selected" | mod look-up compiled out in Multiwinia (`USE_DARWINIA_MOD_SYSTEM` undefined) | `code/lib/resource.cpp` |
| `tools/ShapeExporter` description | "Futurewar Shape Format v1.0" | game-side parser has diverged (exporter no longer compiles against `shape.h`) | `ShapeExporter.cpp` vs `code/lib/shape.h` |
| `code/lib/shape.cpp` `WriteToFile` | writes a loadable `.shp` | broken `%5.2`/`%7.3`/`%6.3` formats, colours not un-scaled | `ShapeFragment::WriteToFile`, `ShapeMarker::WriteToFile` |
| `code/gesture_demo.h` comment | "Returns a mouse pos scaled for the current screen resolution" | scales both axes by screen **height** only; interpolation weights swapped | `GestureDemo::GetCoords` |

## IP register
| Item | Kind | Where (ref) | Neutral placeholder |
|---|---|---|---|
| "Multiwinia", "Darwinia", "Darwinian" (names in `stats.txt`, `sounds.txt` source names `MultiwiniaInterface`, file names `darwinian*.bmp`) | name/brand | `data/stats.txt`, `data/sounds.txt`, `data/animations/darwinian*.bmp` | "Game", "Sprite", "Unit" |
| Dr Sepulveda portraits `sepulveda1–5.bmp`; `OTHER Sepulveda` sound source; `sepulveda_appear/disappear.ogg` | art/name/audio | `data/sepulveda/*`, `data/sounds.txt`, `tools/sounds.txt` | "Mentor portrait 1–5", "Mentor" |
| "Purity Control" promo content (`buyme.txt` phrases `puritycontrol_1..6`, `game.txt`, map/mission) | text/name | `data/purity/*` | omit |
| 139 `.shp` models + 133 `.max` sources | art | `data/shapes/`, `data/shapes/source/` | new models (glTF) |
| 506 BMP images (icons, help pictures, thumbnails, sprites, textures, fonts, terrain ramps, animation sprites) | art | `data/**.bmp` | new art; terrain ramps may be regenerated procedurally |
| `.anim` choreographies (6) | art/animation | `data/animations/*.anim` | new explainer animations |
| 1 332 Ogg samples incl. licensed retro tracks by "Tresk", "Trash80", "DMA-SC" (file names) | audio (incl. third-party music) | `data/sounds/`, `data/mwsounds/` | new audio library |
| `sounds.txt` / `effects.txt` authored mix data (event bindings, parameter values) | data (creative tuning) | `data/sounds.txt`, `data/effects.txt` | re-author; effect *types* (chorus, echo …) are generic |
| Gesture training vectors / demo strokes | data | `data/gestures.txt`, `data/gesture_demos/*` | retrain (Rubine algorithm is public) |
| "redshirt2" magic, "Futurewar" exporter name, "Introversion Software" author string | text/brand | `text_file_writer.cpp`, `ShapeExporter.cpp` | none needed / own magic |
| `animator.exe`, `animator.dat` (tool binary + UI data) | software | repo root | own tooling |
| `banner_template.psp` | art source | `data/icons/` | omit |
| Third-party brands in formats: RAR/UnRAR (format licence), DirectSound effect names `DSound_*` (Microsoft), Paint Shop Pro, Photoshop `.grd`, amBX (`ambx.dat`) | third-party | as listed | avoid RAR; use generic effect names |

## Appendix — dead/legacy code
- `code/gesture_demo.cpp`, `code/gesture_demo_win32.cpp` (`USE_SEPULVEDA_HELP_TUTORIAL`, `GESTURE_EDITOR`), `Resource::GetGestureDemo` — [B:DEAD]; `code/gesture_demo_sdl.cpp` not in the project.
- `code/sepulveda.cpp` image use — [B:DEAD]; `Clouds::RenderFace` — compiled, unreachable.
- `code/lib/shader.cpp` and all `data/shaders/*` — [B:D3D] only.
- WAV decoding path in `SoundStreamDecoder` — compiled, unreachable (`.ogg` always appended).
- Mod-directory look-ups in `Resource` (`USE_DARWINIA_MOD_SYSTEM`, `TARGET_OS_VISTA`) — [B:DEAD].
- Xbox data branch of `build_multiwinia_data.bat` (`.tex` DXT conversion via absent `convert_bmp_to_tex.cmd`, `*.wmv`) — not a PC path.
- `tools/ShapeExporter` (stale vs engine headers) and `tools/3DGeometry` (maths test viewer) — tools, not compiled into the game.
- `data/purity/*`, `data/gesture_demos/*`, `data/shapes/source/*.max`, `data/icons/banner_template.psp`, `data/Untitled Gradients.grd` — never packaged.
- Writers used only by debug editors: `AnimatedPanel::Write` (no caller), `Gesture::SaveTrainingData` ([B:DEBUG] gesture editor), `SoundSystem::SaveBlueprints` ([B:DEBUG] sound editor), `Shape::WriteToFile` (broken).
