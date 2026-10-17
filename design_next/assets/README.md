# Assets

> Status: Asset overview, pass 2b · Related: [GameAssets.md](GameAssets.md) · [asset_manifest.csv](asset_manifest.csv) · [art_and_audio_briefs.md](art_and_audio_briefs.md)

This folder lists what the reference mechanics need as Species content: every sprite, texture, mesh, sound, font, string table and map, newly authored to the technical role its reference counterpart plays. Nothing is traced, re-coloured or edited from the reference game's art, audio or text.

| File | What it is |
|---|---|
| [GameAssets.md](GameAssets.md) | The owner's list: every asset to create, by category, with its Species file name or numbered pattern, role, source and runtime format, spec, variants, priority and placeholder |
| [asset_manifest.csv](asset_manifest.csv) | The same set, one row per file: `asset_id, category, subcategory, consumers, role, target_format, spec, colour_space, alpha_mode, variants, priority, placeholder, brief_ref, species_filename, source_format`. Consumers are the requirement ids that use each asset, separated by `;`; where a document cites the asset only outside its requirements (a dependency list, a data-model table), its path stands in. The asset pipeline consumes every row and the audio engine loads every sample group (AEN-074); neither is listed per row except where one of its requirements names the asset |
| [art_and_audio_briefs.md](art_and_audio_briefs.md) | The style guide for the look and feel to keep, and a brief per category |

## Counts by category and priority

| Category | Asset ids | Files | first-playable | launch | post-launch |
|---|---:|---:|---:|---:|---:|
| Unit sprites | 13 | 16 | 6 | 9 | 1 |
| VFX textures | 19 | 19 | 13 | 6 | 0 |
| UI panels and title | 13 | 20 | 12 | 8 | 0 |
| HUD, supply-crate and power icons | 32 | 34 | 17 | 15 | 2 |
| Cursors and markers | 35 | 47 | 36 | 11 | 0 |
| Input glyphs | 19 | 19 | 1 | 18 | 0 |
| Terrain ramps, water and waves | 3 | 26 | 3 | 21 | 2 |
| Meshes | 77 | 104 | 27 | 71 | 6 |
| Help images and explainer animations | 22 | 75 | 1 | 58 | 16 |
| Audio by source and event | 171 | 895 | 502 | 393 | 0 |
| UI sounds | 13 | 16 | 16 | 0 | 0 |
| Ambience | 4 | 21 | 11 | 10 | 0 |
| Music | 4 | 6 | 0 | 6 | 0 |
| Fonts | 5 | 5 | 1 | 4 | 0 |
| String table | 10 | 10 | 2 | 8 | 0 |
| Maps and thumbnails | 12 | 98 | 4 | 94 | 0 |
| **Total** | **452** | **1411** | **652** | **732** | **27** |

The first-playable set is one skirmish mode, Spawn Control for 2 players, with the core Supply-crate pool ([GameAssets.md §2.5](GameAssets.md#25-priorities-and-the-first-playable-assumption)). 788 reference files and 281 reference text entries need no Species counterpart ([GameAssets.md §20](GameAssets.md#20-not-to-be-created)).

## Technical constraints per category

| Category | Constraints the new assets keep |
|---|---|
| Unit sprites | Power-of-two cards, 32-128 px (square except the 32x64 side view); cut-out alpha (alpha test 0.04 for Workers, 0.02 for other units); white or light-grey bodies for vertex tinting; four hurt variants, a side view and an alternative faction card; full mip chains |
| VFX textures | Additive on black, greyscale intensity, tinted at draw; strips 128x32 and 64x256, flares 128x128; wrap repeat where scrolled or tiled; the particle dot is magnified through the point-magnify clamp-to-edge sampler (ADR-018) |
| UI panels and title | Straight alpha or opaque, no mip-maps, linear filtering; authored at 2x display size, a multiple of 4 in both dimensions (ADR-017); the application icon is a Windows icon resource with six sizes; panel strips tile along their long axis; new palette with the same token roles |
| HUD, supply-crate and power icons | 128x128 icons (64x64 and 50x50 for small read-outs), white or grey art tinted at draw, darkening halo behind; one icon per power or outcome, reused on crates, slots, cursors and markers |
| Cursors and markers | Straight-alpha art, drawn additively or alpha-blended as the pass states (RND-393); blurred darkening shadows are derived variants that the cooker produces offline; compass and disc shadows are tinted alpha masks; banners and flags carry the team area as grey levels with a companion tint mask (AST-022) |
| Input glyphs | Platform-neutral shapes and labels; no console trade dress; shadow glyphs are darkening layers |
| Terrain ramps, water and waves | Ramps 64x64 (x slope, y height) and wave gradients 400x10 are CPU tables, never uploaded; sea tiles 128x128 seamless, wrap repeat |
| Meshes | Flat-shaded, per-face colour, no normals in source; team regions marked by explicit tint weights on the root and its direct children (ADR-020); a collision asset for every collidable mesh (AST-044); named sockets kept exactly; triangle budgets of the reference as upper limits; eight interchangeable Relics |
| Help images and explainer animations | No baked text; hint pictures 200x200, help pages 150x150, mode cards 300x200 (display sizes); explainer pictures with full mip chains; explainer canvases 800-1000 x 250 with fixed loop lengths |
| Audio by source and event | Mono, 48 kHz; variant counts kept per group; loops seamless; behaviour in the sound-event data, not in the files; voice barks are new voice sets |
| UI sounds | 2-D, short one-shots; mono or stereo |
| Ambience | 2-D layered loops; a group streams, as MS-ADPCM decoded by XAudio2, when its longest member is over 20 s (AEN-072, ADR-025) |
| Music | New; stereo 48 kHz; streamed as MS-ADPCM, decoded by XAudio2; slots proposed, final list set by the design phase |
| Fonts | Licensed families; glyph cache at run time; coverage Latin-1, Cyrillic, currency, Japanese kana and ideographs, Hangul |
| String table | New text only; ICU MessageFormat; one table per language; the reference set of nine languages; a development string table in development editions only |
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
| full-weight mask | tint mask of weight 1 over the opaque area of its parent art |
| derived by the cooker | the variant derived from its parent's stand-in or authored file |
| primitive collision | collision triangles at the bounding shape of its render member |

An edition that its edition manifest marks for release contains no placeholder: the content build fails such an edition and lists the rows that have no authored file (AST-085). The build also validates the other direction: an asset id that the sound-event data, the maps or the explainer scripts reference and the manifest lacks is a build error (AST-049, DEC-015), while a file missing at run time is a diagnostic, never fatal; unused files are reported.

