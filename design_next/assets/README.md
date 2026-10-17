# Assets

> Status: Asset overview, pass 1 · Related: [GameAssets.md](GameAssets.md) · [asset_manifest.csv](asset_manifest.csv) · [art_and_audio_briefs.md](art_and_audio_briefs.md)

This folder lists what the reference mechanics need as Species content: every sprite, texture, mesh, sound, font, string table and map, newly authored to the technical role its reference counterpart plays. Nothing is traced, re-coloured or edited from the reference game's art, audio or text.

| File | What it is |
|---|---|
| [GameAssets.md](GameAssets.md) | The owner's list: every asset to create, by category, with its Species file name or numbered pattern, role, source and runtime format, spec, variants, priority and placeholder |
| [asset_manifest.csv](asset_manifest.csv) | The same set, one row per file: `asset_id, category, subcategory, consumers, role, target_format, spec, colour_space, alpha_mode, variants, priority, placeholder, brief_ref, species_filename, source_format`. Consumers are documents in pass 1; pass 2 replaces them with requirement IDs. The asset pipeline consumes every row and is not listed per row |
| [art_and_audio_briefs.md](art_and_audio_briefs.md) | The style guide for the look and feel to keep, and a brief per category |

## Counts by category and priority

| Category | Asset ids | Files | first-playable | launch | post-launch |
|---|---:|---:|---:|---:|---:|
| Unit sprites | 12 | 15 | 6 | 8 | 1 |
| VFX textures | 18 | 18 | 13 | 5 | 0 |
| UI panels and title | 13 | 15 | 6 | 9 | 0 |
| HUD, supply-crate and power icons | 32 | 34 | 17 | 15 | 2 |
| Cursors and markers | 22 | 28 | 22 | 6 | 0 |
| Input glyphs | 13 | 13 | 1 | 12 | 0 |
| Terrain ramps, water and waves | 3 | 26 | 3 | 23 | 0 |
| Meshes | 53 | 71 | 16 | 51 | 4 |
| Help images and explainer animations | 22 | 75 | 1 | 74 | 0 |
| Audio by source and event | 167 | 863 | 470 | 393 | 0 |
| UI sounds | 12 | 14 | 14 | 0 | 0 |
| Ambience | 4 | 21 | 11 | 10 | 0 |
| Music | 4 | 6 | 0 | 6 | 0 |
| Fonts | 5 | 5 | 1 | 4 | 0 |
| String table | 9 | 9 | 1 | 8 | 0 |
| Maps and thumbnails | 12 | 98 | 4 | 94 | 0 |
| **Total** | **401** | **1311** | **586** | **718** | **7** |

The first-playable set is one skirmish mode, Spawn Control for 2 players, with the core Supply-crate pool ([GameAssets.md §2.5](GameAssets.md#25-priorities-and-the-first-playable-assumption)). 817 reference files and 281 reference text entries need no Species counterpart ([GameAssets.md §20](GameAssets.md#20-not-to-be-created)).

## Technical constraints per category

| Category | Constraints the new assets keep |
|---|---|
| Unit sprites | Square power-of-two cards, 32-128 px; cut-out alpha (alpha test 0.04 for Workers, 0.02 for other units); white or light-grey bodies for vertex tinting; four hurt variants, a side view and an alternative faction card; full mip chains |
| VFX textures | Additive on black, greyscale intensity, tinted at draw; strips 128x32 and 64x256, flares 128x128; wrap repeat where scrolled or tiled; the particle dot is magnified with NEAREST |
| UI panels and title | Straight alpha or opaque, no mip-maps, linear filtering; authored at 2x display size; panel strips tile along their long axis; new palette with the same token roles |
| HUD, supply-crate and power icons | 128x128 icons (64x64 and 50x50 for small read-outs), white or grey art tinted at draw, darkening halo behind; one icon per power or outcome, reused on crates, slots, cursors and markers |
| Cursors and markers | Cursor and marker art with run-time blurred darkening shadows; markers drawn as tinted alpha masks; banners and flags paint the team area pure red |
| Input glyphs | Platform-neutral shapes and labels; no console trade dress; shadow glyphs are darkening layers |
| Terrain ramps, water and waves | Ramps 64x64 (x slope, y height) and wave gradients 400x10 are CPU tables, never uploaded; sea tiles 128x128 seamless, wrap repeat |
| Meshes | Flat-shaded, per-face colour, no normals in source; team parts mostly red on the root and its direct children; named sockets kept exactly; triangle budgets of the reference as upper limits; eight interchangeable Relics |
| Help images and explainer animations | No baked text; hint pictures 200x200, help pages 150x150, mode cards 300x200 (display sizes); explainer canvases 800-1000 x 250 with fixed loop lengths |
| Audio by source and event | Mono, 48 kHz; variant counts kept per group; loops seamless; behaviour in the sound-event data, not in the files; voice barks are new voice sets |
| UI sounds | 2-D, short one-shots; mono or stereo |
| Ambience | 2-D layered loops; streamed when long |
| Music | New; stereo 48 kHz; streamed; slots proposed, final list set by the design phase |
| Fonts | Licensed families; glyph cache at run time; coverage Latin-1, Cyrillic, currency, Japanese kana and ideographs, Hangul |
| String table | New text only; ICU MessageFormat; one table per language; the reference set of nine languages |
| Maps and thumbnails | New layouts; one slot per reference map by mode and player count; one auto-rendered 4:3 thumbnail per map |

## Placeholder rule

The content build generates a stand-in for every manifest row that has no authored file yet, at the same runtime path and with the same format, size, channel layout, sockets and loop flag as the row specifies. The game therefore always builds and runs, and every missing asset is visible in the build report. An authored file under `content/` replaces its stand-in; nothing else changes. The stand-in kinds:

| Placeholder | What the content build generates |
|---|---|
| generated figure | flat-colour figure or shape that fills the role silhouette, with an alpha outline |
| generated gradient | white-on-black radial or linear gradient of the stated size |
| generated glyph | white geometric glyph (circle, arrow or ring) with the initials of the asset id |
| generated panel | flat panel of the stated size with the asset id printed on it |
| generated ramp | two-axis colour gradient (slope by height) |
| generated noise tile | seamless value-noise tile |
| diagram card | card with the asset id and the page number |
| static mode card | still card that shows the mode name and the explainer id |
| auto-rendered capture | top-down capture rendered by the map tool from the map that exists (a placeholder map until the layout is final) |
| primitive mesh with sockets | boxes, cylinders and cones at the bounding size of the role, with every socket of the row present |
| synthesised tone | sine or noise burst of the stated duration, looping where the row says loop |
| text-to-speech line | synthetic voice line of the stated duration |
| silence | silent file of the stated length, looping where the row says loop |
| fallback font | an open-licence font with the same script coverage |
| pseudo-localised text | text generated from each string id, padded to test layout |
| generated test map | flat map with the objects the mode needs for its player count |

The build also validates the other direction: every asset id that the sound-event data, the maps or the explainer scripts reference must exist in the manifest (logged, never fatal), and unused files are reported.

