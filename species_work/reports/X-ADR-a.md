# X-ADR-a report — engine-runtime ADRs (0001–0049) and the ADR index

Run date: 2026-10-17. Brief §9.2 X-ADR; package `species_work/packages/X-ADR-a.json`.

## Files written

- `design_next/adr/README.md` — (a) index table for 0001–0099 with my 36 rows, then the marker line `<!-- X-ADR-b rows -->` directly after my last row; (b) §"Technology baseline": the summary table of `standards_baseline.md` (29 rows, with a "Decided in" column linking ADRs in both ranges) plus condensed recommendations by area, every source URL and the check date, and the list of unverified items; (c) §"Deferred to the design phase": nine topics, each with a one-line problem statement. About 42 KB.
- `design_next/adr/0001-…md` to `0036-…md` — 36 MADR 4 records (front matter `status`, `date: 2026-10-17`, `decision-makers: Species technology lead`; all eight MADR sections in order). Largest file 8.7 KB.
- Scripts: `species_work/scripts/X-ADR-a_renumber.py` (shifted ADR references 0013–0049 by one when I inserted ADR-0013), `X-ADR-a_check.py` (front matter, sections, references, links, sizes, index rows), `X-ADR-a_readme.py` (rebuilds my index rows and the "Decided in" links).
- `.gitkeep` left in place. No git commands run. **Nothing was published or uploaded** (no Artifact, Docs, Drive or GitHub tool used).

## ADRs and status

| ADR | Title | Status |
|---|---|---|
| 0001 | Direct3D 12 renderer on the Agility SDK | Accepted |
| 0002 | Minimum GPU capability: FL 12_1, SM 6.6, RB Tier 3, wave ops | Proposed (default: as titled) |
| 0003 | Shader language, compiler and material shader set | Accepted |
| 0004 | Frame graph built from the ordered pass table | Proposed (default: static frame graph) |
| 0005 | Pipeline state catalogue derived from the pass table | Accepted |
| 0006 | Root signature and bindless resource access | Proposed (default: one global SM 6.6 bindless root signature) |
| 0007 | Barrier model: Enhanced Barriers with a resource-state fallback | Proposed (default: not required; both paths) |
| 0008 | GPU memory, uploads and device-removal handling | Accepted |
| 0009 | Instanced crowds, sprite cards and shadows | Accepted |
| 0010 | GPU particles and shatter debris | Accepted |
| 0011 | World-surface rendering: chunked terrain, edge-term overlay, GPU water | Accepted |
| 0012 | Swap chain, presentation and frame pacing | Accepted |
| 0013 | Working colour space and blending | Proposed (default: display-referred, clamping UNORM targets) |
| 0014 | HDR output | Proposed (default: optional scRGB FP16, off) |
| 0015 | Scene anti-aliasing | Proposed (default: 4x MSAA, Off/2x/4x) |
| 0016 | View, projection and depth conventions | Proposed (default: RH +Y up, reversed-Z D32F, Hor+) |
| 0017 | Texture container and block-compression formats | Accepted |
| 0018 | Texture import manifest | Accepted |
| 0019 | Mesh source format and cooked runtime meshes | Accepted |
| 0020 | Flat shading and team tint | Accepted |
| 0021 | Mesh socket schema | Accepted |
| 0022 | Audio engine on XAudio2 2.9 and X3DAudio | Accepted |
| 0023 | Virtual voices, priorities and voice stealing | Accepted |
| 0024 | Audio buses, per-bus effects and the mix | Accepted |
| 0025 | Audio source formats, runtime codecs and residency | Proposed (default: resident 16-bit PCM, streamed Opus) |
| 0026 | Audio engine command queue, control rate and callbacks | Accepted |
| 0027 | Spatialisation and distance attenuation | Accepted |
| 0028 | Input device back-ends | Proposed (default: GameInput for pads, window messages for mouse/keyboard/text) |
| 0029 | Input event model and the binding-graph contract | Accepted |
| 0030 | UI framework: an own retained framework | Proposed (default: own `UiCore`) |
| 0031 | Batched UI rendering | Accepted |
| 0032 | UI focus graph, page stack and input handling | Accepted |
| 0033 | UI layout, scaling and DPI | Accepted |
| 0034 | Text rendering: DirectWrite shaping and a glyph cache | Proposed (default: DirectWrite raster glyph cache) |
| 0035 | Procedural animation runtime | Accepted |
| 0036 | Effect events, presentation clock and effect definitions | Accepted |

Counts: 36 ADRs (24 Accepted, 12 Proposed), **0 flagged `owner-decision: blocking`**. Requirements, tuning keys, diagrams: 0 (not applicable to ADRs). Accepted = 40 decides (D5, D9, R13, R18–R23, T1–T9, T12–T20, T24, T34, G46–G52, G55, F50, F54); Proposed = 40 leaves the choice (T6 feature level, T9 AA/HDR, T7 conventions, T14 codec, T16 glyph path, T19 GameInput or Raw Input, R23 own or middleware) or the choice is new (frame graph, bindless, barriers, working colour space).

Note on ADR-0013 (working colour space): added because keeping the look (O9) requires blending in display-encoded space with per-layer clamping; linear-light blending would change glow halos, darkening depth and saturation. It reconciles the baseline's `BC7_UNORM_SRGB` tagging (kept truthful) with UNORM views.

## F3 ADR requests (17) — coverage

All covered. Mine: graphics API baseline → 0001, 0002, 0006; synchronisation → 0007; shader toolchain → 0003 (X-ADR-b's topic list does not include it, so I wrote it); presentation → 0012, 0014 (plus 0013, 0015); texture pipeline → 0017, 0018; mesh interchange → 0019 (plus 0020, 0021); input → 0028 (plus 0029); audio → 0022, 0025 (plus 0023, 0024, 0026, 0027); text rendering → 0034. Covered by X-ADR-b (verified on disk): platform/OS → 0050; language and compilers → 0051–0053; build and dependencies → 0054, 0055; asset I/O and package format (DirectStorage) → 0058; accessibility → 0064; localisation → 0063; diagnostics and crash reporting → 0065, 0067, 0068; packaging and signing → 0071–0073.

Input rows of my package that belong to X-ADR-b topics and are not written by me: pack files and loose-override rule (25 §2.9, 40 C7, G72, T17 → 0058), data schemas (40 D8 content part, R10, R24, T25, F52 → 0061/0062), content hash (G67, P10 → 0060), localisation (28 §10.5, R28, F51 → 0063), platform clock (G71, T32 → 0074), diagnostics/telemetry/screenshots (G61–G66, T35–T38 → 0065–0068, 0076), toolchain/packaging (R26, R29, T28–T31 → 0050–0057, 0071–0073), window layer (T18 → 0075).

## Version re-checks (brief rule)

Two baseline items marked unverified were re-checked on Microsoft Learn on 2026-10-17 and remain unverified (cited in the README and in ADR-0025 / ADR-0028): the AdpcmEncode distribution (https://learn.microsoft.com/windows/win32/xaudio2/adpcm-overview names the tool, not its source) and a `GameInput::v4` namespace (https://learn.microsoft.com/gaming/gdk/docs/features/common/input/overviews/input-versioning). No version was stated from memory.

## DEC requests (for X-DEC)

1. Cite ADR numbers on F3's DEC requests: Enhanced Barriers → ADR-0007; FL 12_2 → ADR-0002; HDR in v1 → ADR-0014; GameInput pin → ADR-0028; MS-ADPCM → ADR-0025.
2. **Resolved (40 T12):** positional sounds get a maximum distance with a short fade to zero (ADR-0027); new per-event `d_max` and fade-length tuning for R-AUD/AEN. Default for the values: Open, set by the audio design.
3. **Resolved (27 §17 recommendation):** no fixed per-update start quota; triggers keep their spacing (ADR-0023); optional cap off by default.
4. **Open, confirm:** loudness authored in dB; reference values on the 0–10 scale converted with dB = 5 × (v − 10) (ADR-0024; 40 B9, relevant because O13 allows adopting reference values).
5. **Open, confirm:** scene anti-aliasing default 4x MSAA (ADR-0015); Off reproduces reference edges. Default: 4x.
6. **Open, confirm:** display-referred working colour space (ADR-0013). Default: display-referred.
7. **Open, confirm:** Doppler, distance low-pass and distance reverb off by default (per-event factor 0), matching the reference mix (ADR-0027).

## Cross-document requests

- **Coordinator:** add to `CONVENTIONS.md` §9 Allowed terms: `retail` (README only, inside the vendor URL slug `shader-model-6-9-retail-and-more`); `Linux` (README baseline, dedicated-server and CI toolchain only); `immediate-mode` (ADR-0030, developer-tool UI only). These are the only soft hits in my files; hard-fail scan: 0 hits over `design_next/adr/`.
- **Coordinator / X-ADR-b:** my H1s follow X-ADR-b's style (title without number). The README "Decided in" column links 0050–0075 by their current file names; update if any is renumbered. X-ADR-b files already link my 0001, 0004, 0008 and 0012 by file name — do not rename those slugs.
- **C-RND:** define the visual-language requirements in display-encoded values with per-layer clamping (ADR-0013); pass table with catalogue keys (0004/0005); tint function and team palette (0020); orientation modes and card-to-billboard distances (0009); closed combine-mode list (0003); chunk size (0011).
- **C-AST:** socket kinds, naming pattern and per-asset-class schemas (0021); tint attribute name (0020); texture-import record and defaults per kind (0018).
- **C-AEN:** bus graph and Species XAPOs (0024); voice budget, caps, copy decay, steal margin, patience window (0023); control rate and queue capacities (0026); streaming threshold and budgets (0025); `d_max` and fade (0027).
- **C-INP:** bindable-name table, repeat timers, radial dead zone, response-curve data (0029). **C-UI:** font roles, per-language horizontal scales, overflow rules, navigation data; text uses additive and ink states (0031–0034). **C-VFX:** effect event kinds, effect-definition schema, caps/priorities/LOD, maximum clock advance (0010, 0035, 0036). **C-CAM:** FOV, near and far planes for reversed Z (0016).

## Names needed but not in the ledger

No game vocabulary was needed. Engine identifiers used are all from CONVENTIONS §8.12 (`MeshAsset`, `MeshNode`, `MeshSocket`, `ParticlePool`, `ParticleInstance`, `AssetCache`, `SampleCache`, `AudioDecoder`, `AudioEngine`, `SoundVoice`, `SoundParam`, `SoundSourceDef`, `SoundEventDef`, `InputRouter`, `InputSourceNode`, `UiCore`, `GlyphRenderer`, `SpiderGait`, `ProceduralLeg`, `ProceduralFoot`, `TrailPoint`, `ClothFlag`, `ExplainerAnimation`, `FxRng::Seed`, `Mat3x4`). Technical terms introduced without identifiers (propose to the ledger if components need types): presentation clock, effect event, effect definition, socket kind, socket schema, tint weight, orientation mode, copy decay, steal margin, patience window.

## Dropped as legacy (one line per group)

- Graphics stack of 24 §1.3, §2.3, §9 and 40 T1–T9: fixed pipeline, per-vertex submission, precompiled geometry lists, emulation layer, device-reset loop, extension probing and vendor blocklists, matrix stacks, mode switching — replaced positively by 0001–0016; the state table survives as the catalogue defaults of 0005.
- Texture and font mechanisms of 24 §4.22, 25 §2.5, 40 T15–T16: single-format decoding, colour keys, load-time resampling, prebaked per-range atlases — replaced by 0017, 0018, 0034 (coverage and per-language horizontal scale kept).
- Audio back-ends of 27 §2.3, §9, §10, §11.2, §12.5, §17 and 40 T12–T14: polled ring buffers, idle software path, main-thread decode, whole-buffer cache — replaced by 0022–0027 (laws and algorithms kept as requirements).
- Input drivers of 06 §11.6 and 40 T19–T20: message-driven state arrays and the runtime-loaded pad driver — replaced by 0028–0029 (grammar kept for C-INP).
- UI toolkit of 28 §8.7, §11.4, 40 T24: per-frame drawing and name-keyed widgets — replaced by 0030–0033 (page patterns kept).
- Mesh export of 25 §4.4, 40 T34: exporter plug-in and text mesh format; the marker table's file names are not reproduced (socket mechanism in 0021, kinds to C-AST).
