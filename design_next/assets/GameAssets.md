# Species game assets

> Status: Asset list, pass 1 (consumers named by document; pass 2 replaces them with requirement IDs) · Related: [assets README](README.md) · [asset_manifest.csv](asset_manifest.csv) · [art_and_audio_briefs.md](art_and_audio_briefs.md) · [CONVENTIONS](../CONVENTIONS.md)

## Contents

- [1 Purpose and the newly-authored rule](#1-purpose-and-the-newly-authored-rule)
- [2 Filename convention and formats](#2-filename-convention-and-formats)
- [3 Summary](#3-summary)
- [4 Unit sprites](#4-unit-sprites)
- [5 VFX textures](#5-vfx-textures)
- [6 UI panels and title](#6-ui-panels-and-title)
- [7 HUD, supply-crate and power icons](#7-hud-supply-crate-and-power-icons)
- [8 Cursors and markers](#8-cursors-and-markers)
- [9 Input glyphs](#9-input-glyphs)
- [10 Terrain ramps, water and waves](#10-terrain-ramps-water-and-waves)
- [11 Meshes](#11-meshes)
- [12 Help images and explainer animations](#12-help-images-and-explainer-animations)
- [13 Audio by source and event](#13-audio-by-source-and-event)
- [14 UI sounds](#14-ui-sounds)
- [15 Ambience](#15-ambience)
- [16 Music](#16-music)
- [17 Fonts](#17-fonts)
- [18 String table](#18-string-table)
- [19 Maps and thumbnails](#19-maps-and-thumbnails)
- [20 Not to be created](#20-not-to-be-created)

## 1 Purpose and the newly-authored rule

This document lists every asset Species must create so that the mechanics, look and feel of the reference library run with Species content. It covers what a skirmish match of the reference game reaches: units, creatures and hazards, structures and mode objects, Supply crates and powers, the HUD and the front end, help and explainer animations, sound effects, ambience, fonts, player-facing text and maps. It adds music, which the reference does not have. The persistent world adds further needs (world-scale terrain, settlements, a strategic zoom level); the design phase adds those to the manifest.

**Every asset is newly authored.** Each file is made from scratch to the same technical role as its reference counterpart: size class, alpha mode, channel layout, loop behaviour, socket names and variant count. Nothing is traced, re-coloured, resampled or edited from the reference game's art, audio or text, and no reference file serves as a starting point, a paint-over base or a placeholder. Character silhouettes, textures, UI layout, logo and wordmark are new; the style stays the same (vision O9, [art_and_audio_briefs.md](art_and_audio_briefs.md)).

How to read the tables: one row per asset id. A row names its file, or an explicit numbered pattern with its count, for example `content/audio/sfx/worker/worker_attack_{01..24}.wav` (24 files). [asset_manifest.csv](asset_manifest.csv) expands every row to one line per file, with the full format strings and the consuming documents.

## 2 Filename convention and formats

### 2.1 Content root and folders

All assets live under the content root `content/`. The content build rejects any file outside it.

| Folder | Holds |
|---|---|
| `content/sprites/` | unit, creature and effect cards (camera-facing billboards) |
| `content/textures/fx/` | VFX textures: glows, beams, flares, sky clouds, wire grid, canopy mask |
| `content/textures/ui/` | UI panels, title wordmark, logos, front-end decoration, small UI icons |
| `content/textures/icons/power/` | power and Supply-crate outcome icons |
| `content/textures/icons/hud/` | power-slot panel and status read-outs |
| `content/textures/icons/cursors/` | world and menu cursors |
| `content/textures/icons/markers/` | off-screen markers, ground markers, banners and flags |
| `content/textures/icons/input/` | platform-neutral input glyphs |
| `content/textures/terrain/` | terrain colour ramps, sea tiles, shore-wave gradients |
| `content/textures/help/` | hint pictures, help pages, mode cards |
| `content/textures/help/explainer/` | pictures animated by the explainer scripts |
| `content/ui/explainers/` | explainer animation scripts, one per mode |
| `content/meshes/<group>/` | flat-shaded meshes; groups `units`, `creatures`, `structures`, `projectiles`, `relics`, `scenery` |
| `content/audio/sfx/<source>/` | sound effects, one folder per sound source; `crate_outcome/` holds the outcome stings |
| `content/audio/ui/` | interface sounds |
| `content/audio/ambience/` | ambience beds |
| `content/audio/music/` | music (new) |
| `content/fonts/` | licensed font files |
| `content/strings/` | string tables, one per language |
| `content/maps/` | map definitions, one per map slot |
| `content/thumbnails/` | map thumbnails, one per map |

### 2.2 File names

- Lower-case snake_case, named by **role**. A category prefix makes each stem unique across the pack: `fx_`, `ui_`, `power_`, `hud_`, `cursor_`, `marker_`, `glyph_`, `terrain_`, `sea_`, `shore_`, `hint_`, `help_`, `mode_card_`, `explainer_`. Sprites and meshes start with the unit or structure name. Audio files start with their source (`worker_die_01.wav`), UI sounds with `ui_`, ambience with `amb_`, outcome stings with `outcome_`.
- Numbered variants end in `_NN`, two digits, counting from 01 without gaps. A row with a numbered pattern is one asset id with interchangeable variants, unless its spec gives each number a meaning (states, pages, themes).
- No stem repeats a reference file stem, and no name contains an IP word. Ids and names come from [CONVENTIONS §8](../CONVENTIONS.md#8-naming).
- String tables are named by their BCP 47 language tag in lower case with `_` for `-` (`en_gb.json`); the tag itself is stored in the file.
- Maps follow `map_<mode>_<players>p_<nn>`; a thumbnail adds `_thumb`.

### 2.3 Asset ids

Asset ids are dotted and lower case. Numbered variants share one id; documents cite ids, never file names.

| Id root | Used for | Example |
|---|---|---|
| `sprite.` | sprite cards | `sprite.worker.front` |
| `fx.` | VFX textures | `fx.glow_round` |
| `ui.` | UI panels and title art | `ui.title_wordmark` |
| `icon.power.`, `icon.hud.` | power and HUD icons | `icon.power.airstrike` |
| `cursor.`, `marker.` | cursors and markers | `marker.edge_compass` |
| `glyph.` | input glyphs | `glyph.pad.face_south` |
| `terrain.` | ramps, sea tiles, wave gradients | `terrain.ramp` |
| `mesh.<group>.` | meshes | `mesh.structure.spawn_node` |
| `help.`, `anim.explainer` | help pictures and explainer scripts | `help.hint.supply_crates` |
| `sfx.<kind>.<source>.<event>` | sound-effect sample groups; kind is `unit`, `creature`, `weapon`, `building` or `crate` | `sfx.unit.worker.attack` |
| `sfx.crate.outcome.` | Supply-crate outcome stings | `sfx.crate.outcome.shield` |
| `sfx.ui.` | UI sounds | `sfx.ui.match_ui.menu_select` |
| `amb.` | ambience beds | `amb.match_bed` |
| `music.` | music | `music.front_end.theme` |
| `font.` | fonts | `font.ui.primary` |
| `strings.` | string tables | `strings.en_gb` |
| `map.`, `thumb.` | maps and thumbnails, one id per mode | `map.spawn_control` |

### 2.4 Source and runtime formats

The artist delivers the source file under `content/`; the content build generates the runtime file in the pack. Every cooked texture is a DDS with the DX10 header; block-compressed textures are padded to whole 4x4 blocks by the cooker. UI and HUD art is authored at twice its display size and reduced by the cooker; world textures are authored at the stated size. Audio sources are 48 kHz.

| Profile | Source (artist delivers) | Runtime (content build generates) | Colour space | Alpha mode |
|---|---|---|---|---|
| cut-out sprite | PNG RGBA8, sRGB, cut-out alpha painted by the artist | BC7_UNORM_SRGB, full mips | sRGB | cut-out: alpha test at the consumer threshold (0.04 Workers, 0.02 other units) |
| alpha sprite or picture | PNG RGBA8, sRGB, straight alpha | BC7_UNORM_SRGB, full mips (world) or no mips (UI) | sRGB | straight alpha blend |
| additive VFX | PNG 8-bit greyscale, intensity on black | BC4_UNORM, full mips | linear | additive: black adds nothing, colour from the draw tint |
| alpha mask | PNG 8-bit greyscale, white = opaque | BC4_UNORM, full mips | linear | mask as alpha over white, tinted at draw |
| darkening layer | PNG 8-bit greyscale, white = full darkening | BC4_UNORM, full mips | linear | destination multiplied by (1 - texel) |
| tinted icon | PNG RGBA8, sRGB, white or grey art | BC7_UNORM_SRGB, full mips | sRGB | straight alpha, tinted by vertex colour |
| team cloth | PNG RGBA8, sRGB, team area painted pure red (255,0,0) | BC7_UNORM_SRGB, full mips, one recolour per team at load | sRGB | straight alpha; pure-red texels take the team colour |
| UI picture | PNG RGB8 or RGBA8, sRGB | BC7_UNORM_SRGB, no mips (tiled strips keep mips and repeat) | sRGB | opaque or straight alpha |
| CPU look-up table | PNG RGB8, sRGB | uncompressed R8G8B8A8_UNORM_SRGB, no mips, read on the CPU | sRGB | none |
| sea tile | PNG RGB8, sRGB, seamless | BC7_UNORM_SRGB, full mips, wrap repeat | sRGB | opaque, multiplied by the sea light map |
| thumbnail | PNG RGB8, sRGB, rendered by the map tool | BC7_UNORM_SRGB, no mips | sRGB | opaque |
| mesh | glTF 2.0 binary `.glb`: metres, +Y up, no `NORMAL` (flat faces), `COLOR_0` per-face colour, named socket nodes | cooked Species mesh: face normals generated, colours attenuated at load | sRGB vertex colour | opaque; team parts painted mostly red |
| sound effect | WAV, 48 kHz, 24-bit PCM, mono (2-D sounds may be stereo) | 16-bit PCM, 48 kHz, resident | n/a | n/a |
| long audio and music | WAV, 48 kHz, 24-bit PCM, stereo for music | Opus stream, decoded on a worker thread | n/a | n/a |
| font | OpenType font licensed for embedding | font file in the pack; glyphs rasterised at run time into R8_UNORM cache pages | n/a | coverage |
| string table | UTF-8 JSON: string id to ICU MessageFormat text | cooked string table | n/a | n/a |
| map | map definition from the map tool (text, Species map schema) | cooked map definition | n/a | n/a |
| explainer script | JSON key-frame script that animates pictures by asset id | cooked explainer script | n/a | n/a |

### 2.5 Priorities and the first-playable assumption

Each row has one priority: `skirmish-first-playable`, `launch` or `post-launch`.

**First-playable assumption** (vision O8: one skirmish mode first). The first playable is **Spawn Control** for 2 players on the map slots `map_spawn_control_2p_01` and `map_spawn_control_2p_02`, with bots, and with Supply crates drawing from a core outcome pool: Reinforcements, Strike team (with its grenade and rocket weapons), Transport, airstrike, gun turret, Shield, Speed Boost and Fury. It needs:

- units: Worker (all cards), Leader with its banners, Trooper, Transport, Bomber, Essence orbs;
- structures: Spawn Node, Relay, Transit Gate, gun turret (mount, head, gun barrel), Barrier Emitter, wall, tree, jump pad, Supply crate;
- UI: title wordmark, panels, menu highlight, check box, every cursor, the markers, the power-slot HUD for the core pool, the keyboard Tab glyph, the Spawn Control mode card, the primary font (Latin and Cyrillic) and the English string table;
- world: sky clouds, terrain ramp 01, sea tile 01, shore-wave gradient 01, the VFX of those units;
- audio: every sample group of those sources and weapons, the match interface sounds and the match ambience bed.

Everything else that a match reaches is `launch`. `post-launch` holds the seasonal theme, alternative mesh bodies and the few rows whose role still needs confirmation. The design phase may move rows between priorities; the placeholder rule keeps the game building and running whatever is missing.

### 2.6 Placeholders

The content build generates a stand-in for every manifest row that has no authored file yet, at the same runtime path, so the game always builds and runs. Authored files replace stand-ins one by one.

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

## 3 Summary

| Category | Asset ids | Files | first-playable | launch | post-launch | Reference files replaced |
|---|---:|---:|---:|---:|---:|---:|
| [Unit sprites](#4-unit-sprites) | 12 | 15 | 6 | 8 | 1 | 15 |
| [VFX textures](#5-vfx-textures) | 18 | 18 | 13 | 5 | 0 | 18 |
| [UI panels and title](#6-ui-panels-and-title) | 13 | 15 | 6 | 9 | 0 | 15 |
| [HUD, supply-crate and power icons](#7-hud-supply-crate-and-power-icons) | 32 | 34 | 17 | 15 | 2 | 34 |
| [Cursors and markers](#8-cursors-and-markers) | 22 | 28 | 22 | 6 | 0 | 28 |
| [Input glyphs](#9-input-glyphs) | 13 | 13 | 1 | 12 | 0 | 13 |
| [Terrain ramps, water and waves](#10-terrain-ramps-water-and-waves) | 3 | 26 | 3 | 23 | 0 | 26 |
| [Meshes](#11-meshes) | 53 | 71 | 16 | 51 | 4 | 71 |
| [Help images and explainer animations](#12-help-images-and-explainer-animations) | 22 | 75 | 1 | 74 | 0 | 76 |
| [Audio by source and event](#13-audio-by-source-and-event) | 167 | 863 | 470 | 393 | 0 | 863 |
| [UI sounds](#14-ui-sounds) | 12 | 14 | 14 | 0 | 0 | 14 |
| [Ambience](#15-ambience) | 4 | 21 | 11 | 10 | 0 | 21 |
| [Music](#16-music) | 4 | 6 | 0 | 6 | 0 | 0 |
| [Fonts](#17-fonts) | 5 | 5 | 1 | 4 | 0 | 138 |
| [String table](#18-string-table) | 9 | 9 | 1 | 8 | 0 | 0 |
| [Maps and thumbnails](#19-maps-and-thumbnails) | 12 | 98 | 4 | 94 | 0 | 98 |
| **Total** | **401** | **1311** | **586** | **718** | **7** | **1430** |

"Reference files replaced" counts the in-scope reference files whose role each category takes over; one Species file may replace several (138 bitmap font pages become 5 font files). Music, string tables and some help pictures have no one-to-one counterpart.

## 4 Unit sprites

Camera-facing cards for the crowd and for small creatures and effects. Worker cards are white or light grey so that the team colour comes from the vertex tint; the renderer scales them with camera distance and turns them into billboards far away. Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#3-unit-sprites).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `content/sprites/worker_front.png` | `sprite.worker.front` | Worker card, front view: the crowd figure; also drawn large by the loading view, the Launch Race status panel and the front-end backdrop | PNG RGBA8 (cut-out) → BC7 sRGB, mips | 128x128 px; white or light grey body for team tint; alpha-test threshold 0.04; magnified with NEAREST on the front-end backdrop | single | skirmish-first-playable | generated figure |
| `content/sprites/worker_hurt_{01..04}.png` (4 files) | `sprite.worker.hurt` | Worker hurt pose; variants 01-03 are chosen by unit id while a Worker is airborne or dying, variant 04 is the fixed hurt pose | PNG RGBA8 (cut-out) → BC7 sRGB, mips | 128x128 px; same canvas and pivot as the front card; alpha-test threshold 0.04 | 4 | skirmish-first-playable | generated figure |
| `content/sprites/worker_side.png` | `sprite.worker.side` | Worker card seen edge-on (side view), used when the card turns away from the camera | PNG RGBA8 (cut-out) → BC7 sRGB, mips | 32x64 px; alpha-test threshold 0.04 | single | skirmish-first-playable | generated figure |
| `content/sprites/worker_state_overlay.png` | `sprite.worker.state_overlay` | Overlay sprite drawn above a Worker in an altered control state | PNG RGBA8 (cut-out) → BC7 sRGB, mips | 64x64 px; drawn over the Worker card; role to be confirmed against reference/02_units/worker.md | single | launch | generated figure |
| `content/sprites/visitor_front.png` | `sprite.visitor.front` | Visitor faction figure card | PNG RGBA8 → BC7 sRGB, mips | 128x128 px; alpha-test threshold 0.02 | single | launch | generated figure |
| `content/sprites/worker_seasonal_hat.png` | `sprite.worker.seasonal_hat` | Seasonal-theme hat drawn 0.95 wu above every third Worker | PNG RGBA8 (cut-out) → BC7 sRGB, mips | 32x32 px; small accessory card; drawn only while the seasonal theme is on | single | post-launch | generated figure |
| `content/sprites/crawler_segment.png` | `sprite.crawler.segment` | Crawler body card; the renderer builds each Crawler's trail strip from it | PNG grey (additive) → BC4, mips | 32x32 px; hostile (Rogue faction) Crawlers draw it as a darkening layer | single | launch | generated gradient |
| `content/sprites/crawler_egg.png` | `sprite.crawler_egg.body` | Crawler egg card | PNG RGBA8 → BC7 sRGB, mips | 32x32 px | single | launch | generated figure |
| `content/sprites/wisp.png` | `sprite.wisp.body` | Wisp card | PNG grey (additive) → BC4, mips | 32x32 px | single | launch | generated gradient |
| `content/sprites/barrage_missile.png` | `sprite.missile_barrage.missile` | Missile Barrage missile card (new presentation of the mechanic) | PNG RGBA8 → BC7 sRGB, mips | 128x128 px | single | launch | generated figure |
| `content/sprites/barrage_launcher.png` | `sprite.missile_barrage.launcher` | Missile Barrage launcher card that surfaces before the volley (new presentation) | PNG RGBA8 → BC7 sRGB, mips | 128x128 px | single | launch | generated figure |
| `content/sprites/barrage_launcher_shadow.png` | `sprite.missile_barrage.launcher_shadow` | Shadow under the Missile Barrage launcher | PNG grey (darkening) → BC4, mips | 128x128 px | single | launch | generated gradient |

## 5 VFX textures

Additive textures painted on black: overlapping layers saturate toward white and draw order does not matter. They are greyscale; colour comes from the draw tint (team colour, effect colour). Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#4-vfx-textures).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `content/textures/fx/fx_bubble_sphere.png` | `fx.bubble_sphere` | Translucent bubble around a falling Supply crate, a shielded Worker and a Collector | PNG grey (additive) → BC4, mips | 128x128 px | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_sky_cloud.png` | `fx.sky_cloud` | Sky cloud layer (also the front-end background clouds) | PNG grey (additive) → BC4, mips | 16x16 px; blobby layer MAG LINEAR / MIN NEAREST, flat layer NEAREST; wrap repeat; small tile magnified on purpose | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_glow_cloudy.png` | `fx.glow_cloudy` | Soft cloudy glow: front-end fill boxes, Supply crate glow, dust vortex, Spawn Node glow | PNG grey (additive) → BC4, mips | 128x128 px; wrap repeat in front-end fill boxes | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_fuel_flow.png` | `fx.fuel_flow` | Scrolling fuel-flow strip on fuel lines | PNG grey (additive) → BC4, mips | 128x128 px; wrap repeat, scrolled along the line | single | launch | generated gradient |
| `content/textures/fx/fx_glow_round.png` | `fx.glow_round` | Round soft glow: Worker glow, blob shadows, lights, Essence orb glow, Core Device, Solar Array | PNG grey (additive) → BC4, mips | 128x128 px; also drawn as a darkening blob shadow | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_beam_long.png` | `fx.beam_long` | Long beam strip: Launch Race status panel, Control Zone edge, Core Device pulse wave | PNG grey (additive) → BC4, mips | 128x32 px | single | launch | generated gradient |
| `content/textures/fx/fx_beam_short.png` | `fx.beam_short` | Beam strip: laser bolts, the sky grid lines and front-end decoration | PNG grey (additive) → BC4, mips | 128x32 px; wrap repeat for the sky grid; drawn without mips by the sky and laser-bolt passes | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_barrier_field.png` | `fx.barrier_field` | Barrier Emitter field and the base layer of the Beam Dish signal | PNG grey (additive) → BC4, mips | 128x128 px; wrap repeat | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_barrier_field_wide.png` | `fx.barrier_field_wide` | Wide Barrier Emitter field strip | PNG grey (additive) → BC4, mips | 256x64 px; wrap repeat | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_lightning_strip.png` | `fx.lightning_strip` | Lightning-strike strip (map effect) | PNG grey (additive) → BC4, mips | 64x256 px | single | launch | generated gradient |
| `content/textures/fx/fx_muzzle_flash.png` | `fx.muzzle_flash` | Muzzle flash for turrets and units; also the Launch Race status panel | PNG grey (additive) → BC4, mips | 64x64 px | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_particle_dot.png` | `fx.particle_dot` | Particle dot: particle pool, cursor formation preview, off-screen arrows, route lines | PNG grey (additive) → BC4, mips | 16x16 px; MAG NEAREST (crisp pixels) in particle batches | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_dish_signal.png` | `fx.dish_signal` | Beam Dish signal beam, combined with the barrier field texture | PNG grey (additive) → BC4, mips | 128x128 px; wrap repeat; two-texture beam | single | launch | generated gradient |
| `content/textures/fx/fx_shard_edges.png` | `fx.shard_edges` | Edge-line pattern decaled on shatter shards | PNG RGBA8 (cut-out) → BC7 sRGB, mips | 128x128 px; decal over the shard colour where alpha is set | single | skirmish-first-playable | generated figure |
| `content/textures/fx/fx_starburst.png` | `fx.starburst` | Star flare on structure lights and manning ports | PNG grey (additive) → BC4, mips | 128x128 px | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_convert_beam.png` | `fx.convert_beam` | Convert beam strip | PNG grey (additive) → BC4, mips | 128x32 px | single | launch | generated gradient |
| `content/textures/fx/fx_tree_canopy.png` | `fx.tree_canopy` | Tree canopy mask, tinted per tree | PNG grey (mask) → BC4, mips | 128x32 px | single | skirmish-first-playable | generated gradient |
| `content/textures/fx/fx_terrain_grid.png` | `fx.terrain_grid` | Triangle wire-grid overlay drawn once per terrain cell over the terrain and the sea | PNG grey (additive) → BC4, mips | 128x128 px; wrap repeat, one repeat per terrain cell | single | skirmish-first-playable | generated gradient |

## 6 UI panels and title

Front-end and window art. The palette is new; it keeps the token roles of the reference (an accent panel, a neutral panel, a highlight). UI pictures keep exact pixels: no mip-maps, linear filtering. Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#5-ui-panels-title-and-logo).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `content/textures/ui/ui_worker_badge.png` | `ui.worker_badge` | Worker badge on map-select buttons | PNG RGBA8 → BC7 sRGB, no mips | 64x64 px display size; author at 2x (128x128) | single | launch | generated panel |
| `content/textures/ui/ui_fill_blur.png` | `ui.fill_blur` | Blurred fill for front-end boxes | PNG RGB8 (tileable) → BC7 sRGB, mips, repeat | 64x512 px display size; author at 2x (128x1024) | single | skirmish-first-playable | generated panel |
| `content/textures/ui/ui_panel_neutral.png` | `ui.panel_neutral` | Neutral panel strip: power-slot panel, Core Device countdown, fuel depot read-out | PNG RGB8 (tileable) → BC7 sRGB, mips, repeat | 64x512 px display size; author at 2x (128x1024) | single | skirmish-first-playable | generated panel |
| `content/textures/ui/ui_panel_accent.png` | `ui.panel_accent` | Accent panel strip for windows and the power-slot panel (new palette, same token role) | PNG RGB8 (tileable) → BC7 sRGB, mips, repeat | 64x510 px display size; author at 2x (128x1020) | single | skirmish-first-playable | generated panel |
| `content/textures/ui/ui_studio_splash.png` | `ui.studio_splash` | Studio mark on the loading view (placeholder until the studio mark exists) | PNG RGB8 → BC7 sRGB, no mips | 600x600 px display size; author at 2x (1200x1200) | single | launch | generated panel |
| `content/textures/ui/ui_menu_highlight.png` | `ui.menu_highlight` | Highlight beam behind the focused menu item | PNG grey (additive) → BC4, mips | 256x64 px display size; author at 2x (512x128); wrap repeat | single | skirmish-first-playable | generated gradient |
| `content/textures/ui/ui_logo_banner.png` | `ui.logo_banner` | Large title logo on the main menu | PNG RGB8 → BC7 sRGB, no mips | 394x149 px display size; author at 2x (788x298) | single | launch | generated panel |
| `content/textures/ui/ui_title_wordmark.png` | `ui.title_wordmark` | Title wordmark (new logo) | PNG RGBA8 → BC7 sRGB, no mips | 659x122 px display size; author at 2x (1318x244) | single | skirmish-first-playable | generated panel |
| `content/textures/ui/ui_logo_emblem.png` | `ui.logo_emblem` | Logo emblem on the main menu, drop-down menus and the options screen | PNG RGBA8 → BC7 sRGB, no mips | 277x194 px display size; author at 2x (554x388); wrap repeat where tiled | single | launch | generated panel |
| `content/textures/ui/ui_icon_locked.png` | `ui.icon_locked` | Lock icon on password-protected match entries | PNG RGBA8 → BC7 sRGB, no mips | 20x20 px display size; author at 2x (40x40) | single | launch | generated panel |
| `content/textures/ui/ui_icon_check.png` | `ui.icon_check` | Check-box tick | PNG RGBA8 → BC7 sRGB, no mips | 50x50 px display size; author at 2x (100x100) | single | skirmish-first-playable | generated panel |
| `content/textures/ui/ui_difficulty_basic.png` | `ui.difficulty_badge` | Map difficulty badge (basic, intermediate, advanced) on mode and map buttons | PNG RGBA8 → BC7 sRGB, no mips | 64x32 px display size; author at 2x (128x64) | 1 of 3 | launch | generated panel |
| `content/textures/ui/ui_difficulty_intermediate.png` | `ui.difficulty_badge` | ″ | PNG RGBA8 → BC7 sRGB, no mips | 64x32 px display size; author at 2x (128x64) | 2 of 3 | launch | generated panel |
| `content/textures/ui/ui_difficulty_advanced.png` | `ui.difficulty_badge` | ″ | PNG RGBA8 → BC7 sRGB, no mips | 64x32 px display size; author at 2x (128x64) | 3 of 3 | launch | generated panel |
| `content/textures/ui/ui_backdrop_wheel.png` | `ui.backdrop_wheel` | Large decorative wheel behind the front-end backdrop figure (new presentation) | PNG RGB8 → BC7 sRGB, no mips | 426x406 px display size; author at 2x (852x812) | single | launch | generated panel |

## 7 HUD, supply-crate and power icons

Power and Supply-crate outcome icons and the power-slot panel read-outs. Icons are white or grey art tinted at draw, drawn over a darkening halo; the same icon shows on the crate, in the power slot, on the cursor and on off-screen markers. Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#6-hud-supply-crate-and-power-icons).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `content/textures/icons/power/power_workers.png` | `icon.power.workers` | Workers icon: selected-Workers cursor badge, power slot, Reinforcements outcome | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_leader.png` | `icon.power.leader` | Leader icon: selected-unit cursor badge and the promotion power | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_strike_team.png` | `icon.power.strike_team` | Strike team power | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_grenade.png` | `icon.power.grenade` | Strike team grenade weapon | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_rocket.png` | `icon.power.rocket` | Strike team rocket weapon | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_laser.png` | `icon.power.laser` | Laser upgrade power level | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256); role to be confirmed against reference/04 | single | post-launch | generated glyph |
| `content/textures/icons/power/power_airstrike.png` | `icon.power.airstrike` | airstrike outcome and power | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_transport.png` | `icon.power.transport` | Transport outcome and power | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_builder.png` | `icon.power.builder` | Builder outcome and power; also drawn over the Builder | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_collector.png` | `icon.power.collector` | Collector outcome and power | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_missile_barrage.png` | `icon.power.missile_barrage` | Missile Barrage outcome and power | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_convert.png` | `icon.power.convert` | Convert outcome and power | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_shield.png` | `icon.power.shield` | Shield outcome and power | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_speed_boost.png` | `icon.power.speed_boost` | Speed Boost outcome and power | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_gun_turret.png` | `icon.power.gun_turret` | gun turret outcome and power | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_swarm.png` | `icon.power.swarm` | Swarm outcome | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_essence_grove.png` | `icon.power.essence_grove` | Essence Grove outcome | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_blight_grove.png` | `icon.power.blight_grove` | Blight Grove outcome | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_hive.png` | `icon.power.hive` | Hive outcome | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_sickness.png` | `icon.power.sickness` | Sickness outcome | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_brood.png` | `icon.power.brood` | Brood outcome | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_meteor_shower.png` | `icon.power.meteor_shower` | meteor shower outcome | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_fury.png` | `icon.power.fury` | Fury outcome | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/power/power_allegiance_scrambler.png` | `icon.power.allegiance_scrambler` | Allegiance Scrambler outcome (shown on the crate) | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_spitter_plants.png` | `icon.power.spitter_plants` | Spitter Plants outcome (shown on the crate) | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `content/textures/icons/power/power_slots_upgrade.png` | `icon.power.slots_upgrade` | Power-slots power level | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256); role to be confirmed against reference/04 | single | post-launch | generated glyph |
| `content/textures/icons/hud/hud_slot_empty.png` | `icon.hud.slot_empty` | Empty power slot | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/hud/hud_slot_discard.png` | `icon.hud.slot_discard` | Discard-power button on a power slot | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/hud/hud_icon_halo.png` | `icon.hud.icon_halo` | Darkening halo behind HUD icons | PNG grey (darkening) → BC4, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated gradient |
| `content/textures/icons/hud/hud_transport_boarding.png` | `icon.hud.transport_state` | Transport state read-out on its power slot: boarding, travelling, unloading | PNG RGBA8 (white art) → BC7 sRGB, mips | 64x64 px display size; author at 2x (128x128) | 1 of 3 | skirmish-first-playable | generated glyph |
| `content/textures/icons/hud/hud_transport_travelling.png` | `icon.hud.transport_state` | ″ | PNG RGBA8 (white art) → BC7 sRGB, mips | 64x64 px display size; author at 2x (128x128) | 2 of 3 | skirmish-first-playable | generated glyph |
| `content/textures/icons/hud/hud_transport_unloading.png` | `icon.hud.transport_state` | ″ | PNG RGBA8 (white art) → BC7 sRGB, mips | 64x64 px display size; author at 2x (128x128) | 3 of 3 | skirmish-first-playable | generated glyph |
| `content/textures/icons/hud/hud_launch_panel.png` | `icon.hud.launch_panel` | Launch Race status panel frame | PNG RGBA8 → BC7 sRGB, no mips | 200x300 px display size; author at 2x (400x600) | single | launch | generated panel |
| `content/textures/icons/hud/hud_launch_panel_damaged.png` | `icon.hud.launch_panel_damaged` | Launch Race status panel overlay for a damaged Launch Vessel | PNG RGB8 → BC7 sRGB, no mips | 250x300 px display size; author at 2x (500x600) | single | launch | generated panel |

## 8 Cursors and markers

World and menu cursors, off-screen and ground markers, Leader banners and Flag Chain flags. The runtime blurs cursors and markers into darkening shadows; banners and flags paint the team area pure red. Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#7-cursors-and-markers).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `content/textures/icons/cursors/cursor_world.png` | `cursor.world` | Default world cursor | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256); blurred darkening shadow generated at run time | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/cursors/cursor_highlight.png` | `cursor.highlight` | Cursor over a selectable unit; also the power-slot screen-zone highlight | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/cursors/cursor_select.png` | `cursor.select` | Selection cursor; also the power-slot screen-zone highlight | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/cursors/cursor_move_target.png` | `cursor.move_target` | Move-here cursor | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/cursors/cursor_place.png` | `cursor.place` | Placement cursor for powers that place a structure | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/cursors/cursor_enter.png` | `cursor.enter` | Enter cursor over a Transport or a gate | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/cursors/cursor_unavailable.png` | `cursor.unavailable` | Unavailable cursor; also the kick-player button and invalid power targets | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/cursors/cursor_turret_aim.png` | `cursor.turret_aim` | Turret aiming cursor | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/cursors/cursor_pointer.png` | `cursor.pointer` | Pointer arrow for menus; also the scroll-bar arrow | PNG RGBA8 (white art) → BC7 sRGB, mips | 64x64 px display size; author at 2x (128x128) | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/markers/marker_edge_compass.png` | `marker.edge_compass` | Off-screen compass marker and power-slot compass | PNG RGBA8 (white art) → BC7 sRGB, mips | 256x256 px | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/markers/marker_edge_compass_shadow.png` | `marker.edge_compass_shadow` | Shadow for the off-screen compass marker | PNG grey (mask) → BC4, mips | 256x256 px | single | skirmish-first-playable | generated gradient |
| `content/textures/icons/markers/marker_team_disc.png` | `marker.team_disc` | Team-coloured disc under off-screen markers and power slots | PNG grey (mask) → BC4, mips | 128x128 px | single | skirmish-first-playable | generated gradient |
| `content/textures/icons/markers/marker_team_disc_shadow.png` | `marker.team_disc_shadow` | Shadow for the team disc | PNG grey (mask) → BC4, mips | 128x128 px | single | skirmish-first-playable | generated gradient |
| `content/textures/icons/markers/marker_supply_crate.png` | `marker.supply_crate` | Off-screen marker and recent-power badge for a Supply crate | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/markers/marker_relic.png` | `marker.relic` | Off-screen marker for a Relic | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px | single | launch | generated glyph |
| `content/textures/icons/markers/marker_crate_landing.png` | `marker.crate_landing` | Ground marker where a Supply crate will land | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px | single | skirmish-first-playable | generated glyph |
| `content/textures/icons/markers/marker_barrage_target.png` | `marker.barrage_target` | Ground symbol under a Missile Barrage target (new presentation) | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px | single | launch | generated glyph |
| `content/textures/icons/markers/marker_flag_pointer.png` | `marker.flag_pointer` | Pointer drawn on a cloth flag | PNG RGBA8 → BC7 sRGB, mips | 64x64 px | single | skirmish-first-playable | generated figure |
| `content/textures/icons/markers/marker_jump_pad.png` | `marker.jump_pad` | Direction arrow on a jump pad | PNG RGBA8 → BC7 sRGB, mips | 64x64 px; wrap repeat, scrolled | single | skirmish-first-playable | generated figure |
| `content/textures/icons/markers/marker_order_trail.png` | `marker.order_trail` | Order trail mark left by a Leader's order | PNG RGBA8 → BC7 sRGB, mips | 64x64 px | single | skirmish-first-playable | generated figure |
| `content/textures/icons/markers/leader_banner_idle.png` | `marker.leader_banner` | Leader banner per order state: idle, follow, formation, gather | PNG RGBA8 (team red) → BC7 sRGB, mips, per-team | 64x64 px; cloth texture; team area pure red | 1 of 4 | skirmish-first-playable | generated glyph |
| `content/textures/icons/markers/leader_banner_follow.png` | `marker.leader_banner` | ″ | PNG RGBA8 (team red) → BC7 sRGB, mips, per-team | 64x64 px; cloth texture; team area pure red | 2 of 4 | skirmish-first-playable | generated glyph |
| `content/textures/icons/markers/leader_banner_formation.png` | `marker.leader_banner` | ″ | PNG RGBA8 (team red) → BC7 sRGB, mips, per-team | 64x64 px; cloth texture; team area pure red | 3 of 4 | skirmish-first-playable | generated glyph |
| `content/textures/icons/markers/leader_banner_gather.png` | `marker.leader_banner` | ″ | PNG RGBA8 (team red) → BC7 sRGB, mips, per-team | 64x64 px; cloth texture; team area pure red | 4 of 4 | skirmish-first-playable | generated glyph |
| `content/textures/icons/markers/flag_post_neutral.png` | `marker.flag_post_state` | Flag Chain flag per state: neutral, lowered, locked, raised | PNG RGBA8 (team red) → BC7 sRGB, mips, per-team | 64x64 px; cloth texture; team area pure red | 1 of 4 | launch | generated glyph |
| `content/textures/icons/markers/flag_post_lowered.png` | `marker.flag_post_state` | ″ | PNG RGBA8 (team red) → BC7 sRGB, mips, per-team | 64x64 px; cloth texture; team area pure red | 2 of 4 | launch | generated glyph |
| `content/textures/icons/markers/flag_post_locked.png` | `marker.flag_post_state` | ″ | PNG RGBA8 (team red) → BC7 sRGB, mips, per-team | 64x64 px; cloth texture; team area pure red | 3 of 4 | launch | generated glyph |
| `content/textures/icons/markers/flag_post_raised.png` | `marker.flag_post_state` | ″ | PNG RGBA8 (team red) → BC7 sRGB, mips, per-team | 64x64 px; cloth texture; team area pure red | 4 of 4 | launch | generated glyph |

## 9 Input glyphs

Platform-neutral input glyphs: shapes and labels describe the position of a control, never a console brand or trade dress. Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#8-input-glyphs).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `content/textures/icons/input/glyph_pad_face_south.png` | `glyph.pad.face_south` | Gamepad face button, south position | PNG RGBA8 (white art) → BC7 sRGB, mips | 64x64 px display size; author at 2x (128x128); platform-neutral shape and label; no console trade dress | single | launch | generated glyph |
| `content/textures/icons/input/glyph_pad_face_east.png` | `glyph.pad.face_east` | Gamepad face button, east position | PNG RGBA8 (white art) → BC7 sRGB, mips | 64x64 px display size; author at 2x (128x128) | single | launch | generated glyph |
| `content/textures/icons/input/glyph_pad_face_west.png` | `glyph.pad.face_west` | Gamepad face button, west position | PNG RGBA8 (white art) → BC7 sRGB, mips | 64x64 px display size; author at 2x (128x128) | single | launch | generated glyph |
| `content/textures/icons/input/glyph_pad_face_north.png` | `glyph.pad.face_north` | Gamepad face button, north position | PNG RGBA8 (white art) → BC7 sRGB, mips | 64x64 px display size; author at 2x (128x128) | single | launch | generated glyph |
| `content/textures/icons/input/glyph_pad_face_shadow.png` | `glyph.pad.face_shadow` | Darkening shadow under face-button glyphs | PNG grey (darkening) → BC4, mips | 50x50 px display size; author at 2x (100x100) | single | launch | generated gradient |
| `content/textures/icons/input/glyph_pad_shoulder_left.png` | `glyph.pad.shoulder_left` | Gamepad left shoulder button (also on the power-slot panel) | PNG RGBA8 (white art) → BC7 sRGB, mips | 50x50 px display size; author at 2x (100x100) | single | launch | generated glyph |
| `content/textures/icons/input/glyph_pad_shoulder_left_shadow.png` | `glyph.pad.shoulder_left_shadow` | Darkening shadow under the left shoulder glyph | PNG grey (darkening) → BC4, mips | 50x50 px display size; author at 2x (100x100) | single | launch | generated gradient |
| `content/textures/icons/input/glyph_pad_shoulder_right.png` | `glyph.pad.shoulder_right` | Gamepad right shoulder button | PNG RGBA8 (white art) → BC7 sRGB, mips | 50x50 px display size; author at 2x (100x100) | single | launch | generated glyph |
| `content/textures/icons/input/glyph_pad_shoulder_right_shadow.png` | `glyph.pad.shoulder_right_shadow` | Darkening shadow under the right shoulder glyph | PNG grey (darkening) → BC4, mips | 50x50 px display size; author at 2x (100x100) | single | launch | generated gradient |
| `content/textures/icons/input/glyph_pad_trigger_left.png` | `glyph.pad.trigger_left` | Gamepad left trigger | PNG RGBA8 (white art) → BC7 sRGB, mips | 50x50 px display size; author at 2x (100x100) | single | launch | generated glyph |
| `content/textures/icons/input/glyph_pad_trigger_right.png` | `glyph.pad.trigger_right` | Gamepad right trigger | PNG RGBA8 (white art) → BC7 sRGB, mips | 50x50 px display size; author at 2x (100x100) | single | launch | generated glyph |
| `content/textures/icons/input/glyph_pad_trigger_shadow.png` | `glyph.pad.trigger_shadow` | Darkening shadow under trigger glyphs | PNG grey (darkening) → BC4, mips | 50x50 px display size; author at 2x (100x100) | single | launch | generated gradient |
| `content/textures/icons/input/glyph_key_tab.png` | `glyph.key.tab` | Keyboard Tab key on the power-slot panel | PNG RGBA8 (white art) → BC7 sRGB, mips | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |

## 10 Terrain ramps, water and waves

Ramps and wave gradients are CPU look-up tables, not GPU textures: the terrain builder reads a ramp by slope and height to colour each terrain triangle, and the sea renderer reads one wave-gradient row. Sea tiles are GPU textures, repeated and multiplied by the sea light map. Each map names one ramp, one sea tile and one wave gradient; number 01 of each is the map default. The seasonal theme forces ramp 06, sea tile 02 and wave gradient 04. Theme numbers carry no fixed biome; the environment brief assigns one look per number.

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `content/textures/terrain/terrain_ramp_{01..14}.png` (14 files) | `terrain.ramp` | Terrain colour ramp: colour by slope (x) and height (y), read on the CPU when the terrain mesh is built | PNG RGB8 (table) → RGBA8 sRGB table, read on the CPU | 64x64 px; x = slope, y = height | 14 | skirmish-first-playable: 1<br>launch: 13 | generated ramp |
| `content/textures/terrain/sea_tile_{01..04}.png` (4 files) | `terrain.sea_tile` | Sea surface tile (repeated, multiplied by the sea light map) | PNG RGB8 (seamless) → BC7 sRGB, mips, repeat | 128x128 px; wrap repeat | 4 | skirmish-first-playable: 1<br>launch: 3 | generated noise tile |
| `content/textures/terrain/shore_wave_gradient_{01..08}.png` (8 files) | `terrain.shore_wave_gradient` | Shore-wave colour gradient: one row read on the CPU by the sea renderer | PNG RGB8 (table) → RGBA8 sRGB table, read on the CPU | 400x10 px; the wave colour runs along x | 8 | skirmish-first-playable: 1<br>launch: 7 | generated ramp |

## 11 Meshes

Flat-shaded low-poly meshes with per-face colours. Parts that take the team colour are painted "mostly red" (red more than twice green and twice blue) on the root node and its direct children; the renderer recolours them. Code fetches sockets by name, so every socket of a row must exist with exactly that name, as an empty node in the `.glb`. Budgets are the triangle counts of the reference meshes and are upper limits. Formats: `.glb` (glTF 2.0 binary) → cooked Species mesh; placeholder: primitive mesh with sockets.

**Socket vocabulary.** `port_NN` manning slot where a Worker stands to operate a structure; `light_NN` lamp or flare; `status_NN` status light per port; `muzzle_NN` barrel end where shots start; `casing_eject_NN` where spent casings leave; `barrel_mount` where a barrel attaches; `kill_counter` kill read-out; `carry` carry point; `leg_NN` leg root; `segment_end` end of a leg part; `engine_NN` engine glow; `essence_link` end point of an essence link; `tail_NN` tail attachment; `gate_exit_NN` destination point of a gate; `gate_surface` gate surface centre; `gate_entrance` where units enter; `door` exit for spawned units; `fuel_point` fuel connection; `glow_NN` panel glow; `grab_NN` lift points; `egg_spawn` and `egg_lay` egg release points; `entrance` boarding point; `flag` flag pole tip; `grid_link` energy-grid connection; `beam_focus` beam origin; `window_NN` lit window; `thruster` engine exhaust; `field_anchor_NN` field end points; `eye_NN` eye glints; `launch_point` pod launch point; `stem` stem attachment; `arm_tip` pump-arm tip; `dish_direction` dish aiming point. Numbers keep their reference base: leg sockets count from 00, the others from 01.

| Species file | Asset id | Role | Spec (budget, nodes, sockets) | Variants | Priority |
|---|---|---|---|---|---|
| `content/meshes/units/worker_figure.glb` | `mesh.unit.worker_figure` | Worker 3-D figure (team-recoloured mesh with a flag socket) | <= 76 triangles budget; 1 node, depth 1; sockets: flag | single | launch |
| `content/meshes/units/leader_figure.glb` | `mesh.unit.leader_figure` | Leader figure carrying its banner | <= 76 triangles budget; 1 node, depth 1; sockets: flag | single | skirmish-first-playable |
| `content/meshes/units/trooper_figure.glb` | `mesh.unit.trooper_figure` | Trooper figure | <= 1036 triangles budget; 2 nodes, depth 1; sockets: casing_eject, eye_01, eye_02, muzzle | single | skirmish-first-playable |
| `content/meshes/units/builder_body.glb` | `mesh.unit.builder_body` | Builder body | <= 72 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `content/meshes/units/transport_hull.glb` | `mesh.unit.transport_hull` | Transport hull | <= 96 triangles budget; 3 nodes, depth 2; sockets: entrance, flag | single | skirmish-first-playable |
| `content/meshes/units/collector_body.glb` | `mesh.unit.collector_body` | Collector body | <= 2064 triangles budget; 1 node, depth 1; sockets: engine_01, engine_02, engine_03, engine_04 | single | launch |
| `content/meshes/units/bomber_body.glb` | `mesh.unit.bomber_body` | Bomber flying the airstrike | <= 92 triangles budget; 1 node, depth 1; sockets: none | single | skirmish-first-playable |
| `content/meshes/projectiles/thrown_weapon.glb` | `mesh.projectile.thrown_weapon` | Thrown weapon: grenade, the Bomber's bombs and rockets | <= 286 triangles budget; 1 node, depth 1; sockets: none | single | skirmish-first-playable |
| `content/meshes/projectiles/turret_round.glb` | `mesh.projectile.turret_round` | Turret round | <= 8 triangles budget; 1 node, depth 1; sockets: none | single | skirmish-first-playable |
| `content/meshes/projectiles/shell_casing.glb` | `mesh.projectile.shell_casing` | Ejected shell casing | <= 140 triangles budget; 1 node, depth 1; sockets: none | single | skirmish-first-playable |
| `content/meshes/projectiles/meteor_rock.glb` | `mesh.projectile.meteor_rock` | Meteor of the meteor shower | <= 180 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `content/meshes/creatures/ant_body_01.glb` | `mesh.creature.ant_body` | ant body, three variants chosen per ant | <= 56 triangles budget; 1 node, depth 1; sockets: carry | 1 of 3 | launch |
| `content/meshes/creatures/ant_body_02.glb` | `mesh.creature.ant_body` | ″ | <= 56 triangles budget; 1 node, depth 1; sockets: carry | 2 of 3 | launch |
| `content/meshes/creatures/ant_body_03.glb` | `mesh.creature.ant_body` | ″ | <= 56 triangles budget; 1 node, depth 1; sockets: carry | 3 of 3 | launch |
| `content/meshes/creatures/centipede_segment.glb` | `mesh.creature.centipede_segment` | centipede body segment | <= 40 triangles budget; 2 nodes, depth 1; sockets: none | single | launch |
| `content/meshes/creatures/centipede_head_segment.glb` | `mesh.creature.centipede_head_segment` | centipede head segment | <= 56 triangles budget; 4 nodes, depth 1; sockets: none | single | launch |
| `content/meshes/creatures/spider_body.glb` | `mesh.creature.spider_body` | spider body with eight leg sockets | <= 132 triangles budget; 1 node, depth 1; sockets: egg_lay, leg_00, leg_01, leg_02, leg_03, leg_04, leg_05, leg_06, leg_07 | single | launch |
| `content/meshes/creatures/spider_thigh.glb` | `mesh.creature.spider_thigh` | spider upper leg part (procedural leg) | <= 84 triangles budget; 1 node, depth 1; sockets: segment_end | single | launch |
| `content/meshes/creatures/spider_shin.glb` | `mesh.creature.spider_shin` | spider lower leg part (procedural leg) | <= 66 triangles budget; 1 node, depth 1; sockets: segment_end | single | launch |
| `content/meshes/creatures/reaper_serpent_head.glb` | `mesh.creature.reaper_serpent_head` | Reaper Serpent head | <= 38 triangles budget; 1 node, depth 1; sockets: tail | single | launch |
| `content/meshes/creatures/reaper_serpent_tail.glb` | `mesh.creature.reaper_serpent_tail` | Reaper Serpent tail segment | <= 42 triangles budget; 2 nodes, depth 1; sockets: none | single | launch |
| `content/meshes/creatures/brood_drifter_body.glb` | `mesh.creature.brood_drifter_body` | Brood Drifter body | <= 55 triangles budget; 1 node, depth 1; sockets: egg_spawn, tail_01, tail_02, tail_03, tail_04 | single | launch |
| `content/meshes/creatures/abductor_hull.glb` | `mesh.creature.abductor_hull` | Abductor hull (also placed by maps as scenery) | <= 9028 triangles budget; 2 nodes, depth 1; sockets: engine_01, engine_02, engine_03, engine_04 | single | launch |
| `content/meshes/creatures/seed_pod.glb` | `mesh.creature.seed_pod` | Seed Pod (also the Spitter Plant's pod) | <= 130 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `content/meshes/structures/spitter_plant_head.glb` | `mesh.structure.spitter_plant_head` | Spitter Plant head | <= 336 triangles budget; 1 node, depth 1; sockets: launch_point, stem | single | launch |
| `content/meshes/structures/hive_mound.glb` | `mesh.structure.hive_mound` | Hive mound (also the placement-cursor preview) | <= 396 triangles budget; 2 nodes, depth 1; sockets: none | single | launch |
| `content/meshes/structures/spawn_node.glb` | `mesh.structure.spawn_node` | Spawn Node (the match variant) | <= 196 triangles budget; 1 node, depth 1; sockets: door, essence_link, light_01, light_02, light_03, port_01, port_02, port_03, port_04, port_05 | single | skirmish-first-playable |
| `content/meshes/structures/spawn_node_alt.glb` | `mesh.structure.spawn_node_alt` | Spawn Node alternative body (second constructor branch) | <= 152 triangles budget; 1 node, depth 1; sockets: door, essence_link, light_01, light_02, light_03, port_01, port_02, port_03, port_04, port_05 | single | post-launch |
| `content/meshes/structures/source_node.glb` | `mesh.structure.source_node` | Source Node | <= 180 triangles budget; 1 node, depth 1; sockets: essence_link | single | launch |
| `content/meshes/structures/relay_mast.glb` | `mesh.structure.relay_mast` | Relay | <= 152 triangles budget; 2 nodes, depth 1; sockets: essence_link | single | skirmish-first-playable |
| `content/meshes/structures/transit_gate.glb` | `mesh.structure.transit_gate` | Transit Gate (the match variant) | <= 192 triangles budget; 1 node, depth 1; sockets: gate_exit_01, gate_exit_02, gate_surface | single | skirmish-first-playable |
| `content/meshes/structures/transit_gate_alt.glb` | `mesh.structure.transit_gate_alt` | Transit Gate alternative body (second constructor branch) | <= 292 triangles budget; 6 nodes, depth 1; sockets: gate_exit_01, gate_exit_02, gate_surface, light_01, light_02 | single | post-launch |
| `content/meshes/structures/command_console.glb` | `mesh.structure.command_console` | Command Console | <= 308 triangles budget; 1 node, depth 1; sockets: port_01, port_02, port_03, port_04, port_05 | single | launch |
| `content/meshes/structures/command_console_linked.glb` | `mesh.structure.command_console_linked` | Command Console linked to a Core Device (with dish-direction socket) | <= 628 triangles budget; 2 nodes, depth 1; sockets: dish_direction, port_01, port_02, port_03 | single | launch |
| `content/meshes/structures/core_device.glb` | `mesh.structure.core_device` | Core Device | <= 1340 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `content/meshes/structures/breach_charge.glb` | `mesh.structure.breach_charge` | Breach Charge | <= 286 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `content/meshes/structures/launch_vessel.glb` | `mesh.structure.launch_vessel` | Launch Vessel | <= 4992 triangles budget; 1 node, depth 1; sockets: fuel_point, thruster, window_01, window_02, window_03 | single | launch |
| `content/meshes/structures/fuel_pump_housing.glb` | `mesh.structure.fuel_pump_housing` | fuel pump housing | <= 252 triangles budget; 1 node, depth 1; sockets: fuel_point | single | launch |
| `content/meshes/structures/fuel_pump_arm.glb` | `mesh.structure.fuel_pump_arm` | fuel pump moving arm | <= 126 triangles budget; 1 node, depth 1; sockets: arm_tip | single | launch |
| `content/meshes/structures/fuel_line_foot.glb` | `mesh.structure.fuel_line_foot` | fuel line foot | <= 96 triangles budget; 2 nodes, depth 1; sockets: fuel_point | single | launch |
| `content/meshes/structures/fuel_line_span.glb` | `mesh.structure.fuel_line_span` | fuel line span between feet | <= 72 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `content/meshes/structures/fuel_depot.glb` | `mesh.structure.fuel_depot` | fuel depot | <= 344 triangles budget; 4 nodes, depth 1; sockets: entrance, fuel_point | single | launch |
| `content/meshes/structures/solar_array.glb` | `mesh.structure.solar_array` | Solar Array | <= 648 triangles budget; 5 nodes, depth 1; sockets: glow_01, glow_02, glow_03, glow_04, grid_link, port_01, port_02, port_03, port_04, port_05, status_01, status_02, status_03, status_04, status_05 | single | launch |
| `content/meshes/structures/grid_pylon.glb` | `mesh.structure.grid_pylon` | grid pylon (also the grid terminal and the grid splitter) | <= 96 triangles budget; 3 nodes, depth 1; sockets: grid_link, light_01 | single | launch |
| `content/meshes/structures/beam_dish.glb` | `mesh.structure.beam_dish` | Beam Dish with a rotating dish node | <= 248 triangles budget; 6 nodes, depth 3; sockets: beam_focus (on node dish), gate_entrance (on node door), light_01 (on node upper_mount), light_02 (on node column) | single | launch |
| `content/meshes/structures/barrier_emitter.glb` | `mesh.structure.barrier_emitter` | Barrier Emitter post | <= 54 triangles budget; 2 nodes, depth 1; sockets: field_anchor_01, field_anchor_02, light_01 | single | skirmish-first-playable |
| `content/meshes/structures/turret_mount.glb` | `mesh.structure.turret_mount` | turret mount: socket carrier only (manning ports and status lights), no geometry | no triangles budget; 0 nodes, depth 0; sockets: port_01, port_02, port_03, port_04, status_01, status_02, status_03, status_04 | single | skirmish-first-playable |
| `content/meshes/structures/turret_head.glb` | `mesh.structure.turret_head` | turret head (also the placement-cursor preview) | <= 114 triangles budget; 2 nodes, depth 1; sockets: barrel_mount, kill_counter | single | skirmish-first-playable |
| `content/meshes/structures/turret_barrel_gun.glb` | `mesh.structure.turret_barrel` | turret barrel set per turret type: gun, rocket, flame, and the shared cannon barrel | <= 752 triangles budget; 20 nodes, depth 1; sockets: casing_eject_01, casing_eject_02, casing_eject_03, casing_eject_04, muzzle_01, muzzle_02, muzzle_03, muzzle_04 | 1 of 4 | skirmish-first-playable |
| `content/meshes/structures/turret_barrel_rocket.glb` | `mesh.structure.turret_barrel` | ″ | <= 1528 triangles budget; 8 nodes, depth 1; sockets: muzzle_01, muzzle_02, muzzle_03, muzzle_04 | 2 of 4 | launch |
| `content/meshes/structures/turret_barrel_flame.glb` | `mesh.structure.turret_barrel` | ″ | <= 1344 triangles budget; 20 nodes, depth 1; sockets: muzzle_01, muzzle_02, muzzle_03, muzzle_04 | 3 of 4 | launch |
| `content/meshes/structures/turret_barrel_cannon.glb` | `mesh.structure.turret_barrel` | ″ | <= 80 triangles budget; 4 nodes, depth 1; sockets: muzzle_01, muzzle_02, muzzle_03, muzzle_04 | 4 of 4 | launch |
| `content/meshes/structures/wall_section.glb` | `mesh.structure.wall_section` | wall section | <= 332 triangles budget; 3 nodes, depth 1; sockets: none | single | skirmish-first-playable |
| `content/meshes/structures/supply_crate.glb` | `mesh.structure.supply_crate` | Supply crate body | <= 784 triangles budget; 1 node, depth 1; sockets: grab_01, grab_02 | single | skirmish-first-playable |
| `content/meshes/structures/bot_nav_node.glb` | `mesh.structure.bot_nav_node` | bot navigation node marker | <= 84 triangles budget; 2 nodes, depth 1; sockets: light_01 | single | post-launch |
| `content/meshes/structures/structure_default.glb` | `mesh.structure.structure_default` | default structure body assigned before a structure sets its own mesh | <= 264 triangles budget; 3 nodes, depth 1; sockets: status | single | post-launch |
| `content/meshes/relics/relic_01.glb` | `mesh.relic.variant` | Relic, eight interchangeable variants | <= 4244 triangles budget; 3 nodes, depth 1; sockets: none | 1 of 8 | launch |
| `content/meshes/relics/relic_02.glb` | `mesh.relic.variant` | ″ | <= 9768 triangles budget; 6 nodes, depth 1; sockets: none | 2 of 8 | launch |
| `content/meshes/relics/relic_03.glb` | `mesh.relic.variant` | ″ | <= 24366 triangles budget; 5 nodes, depth 1; sockets: none | 3 of 8 | launch |
| `content/meshes/relics/relic_04.glb` | `mesh.relic.variant` | ″ | <= 3872 triangles budget; 2 nodes, depth 1; sockets: none | 4 of 8 | launch |
| `content/meshes/relics/relic_05.glb` | `mesh.relic.variant` | ″ | <= 3252 triangles budget; 2 nodes, depth 1; sockets: none | 5 of 8 | launch |
| `content/meshes/relics/relic_06.glb` | `mesh.relic.variant` | ″ | <= 4740 triangles budget; 5 nodes, depth 1; sockets: none | 6 of 8 | launch |
| `content/meshes/relics/relic_07.glb` | `mesh.relic.variant` | ″ | <= 2136 triangles budget; 7 nodes, depth 1; sockets: none | 7 of 8 | launch |
| `content/meshes/relics/relic_08.glb` | `mesh.relic.variant` | ″ | <= 3816 triangles budget; 4 nodes, depth 1; sockets: none | 8 of 8 | launch |
| `content/meshes/scenery/scenery_prop_01.glb` | `mesh.scenery.prop` | map scenery mesh, seven pieces placed by maps | <= 36 triangles budget; 3 nodes, depth 1; sockets: none | 1 of 7 | launch |
| `content/meshes/scenery/scenery_prop_02.glb` | `mesh.scenery.prop` | ″ | <= 1912 triangles budget; 19 nodes, depth 1; sockets: none | 2 of 7 | launch |
| `content/meshes/scenery/scenery_prop_03.glb` | `mesh.scenery.prop` | ″ | <= 540 triangles budget; 1 node, depth 1; sockets: none | 3 of 7 | launch |
| `content/meshes/scenery/scenery_prop_04.glb` | `mesh.scenery.prop` | ″ | <= 1036 triangles budget; 2 nodes, depth 1; sockets: none | 4 of 7 | launch |
| `content/meshes/scenery/scenery_prop_05.glb` | `mesh.scenery.prop` | ″ | <= 128 triangles budget; 1 node, depth 1; sockets: none | 5 of 7 | launch |
| `content/meshes/scenery/scenery_prop_06.glb` | `mesh.scenery.prop` | ″ | <= 236 triangles budget; 2 nodes, depth 1; sockets: none | 6 of 7 | launch |
| `content/meshes/scenery/scenery_prop_07.glb` | `mesh.scenery.prop` | ″ | <= 128 triangles budget; 1 node, depth 1; sockets: none | 7 of 7 | launch |

## 12 Help images and explainer animations

Help pictures carry no baked text: labels come from the string table, so one picture serves every language. Explainer animations are one script per mode that moves pictures on a canvas for a fixed loop; the scripts are re-authored in the Species explainer format and reference their pictures by asset id. Choreography (what moves when) is new; canvas size and loop length follow the reference.

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `content/textures/help/hint_start_here.png` | `help.hint.start_here` | Hint: where the player starts (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_spawn_nodes.png` | `help.hint.spawn_nodes` | Hint: capturing Spawn Nodes (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_reinforcements.png` | `help.hint.reinforcements` | Hint: reinforcements (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_supply_crates.png` | `help.hint.supply_crates` | Hint: Supply crates (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_zone_control.png` | `help.hint.zone_control` | Hint: holding Control Zones (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_relic_run_capture.png` | `help.hint.relic_run_capture` | Hint: bringing a Relic home (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_relic_run_lift.png` | `help.hint.relic_run_lift` | Hint: lifting a Relic (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_siege_core_device.png` | `help.hint.siege_core_device` | Hint: the Core Device (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_siege_attack.png` | `help.hint.siege_attack` | Hint: attacking in Siege (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_siege_defend.png` | `help.hint.siege_defend` | Hint: defending in Siege (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_siege_spawn_nodes.png` | `help.hint.siege_spawn_nodes` | Hint: Spawn Nodes in Siege (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_siege_breach_charge.png` | `help.hint.siege_breach_charge` | Hint: the Breach Charge (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_launch_race_01.png` | `help.hint.launch_race_01` | Hint: fuelling a Launch Vessel (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_launch_race_02.png` | `help.hint.launch_race_02` | Hint: launching (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_flag_chain_01.png` | `help.hint.flag_chain_01` | Hint: the flag chain (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_flag_chain_02.png` | `help.hint.flag_chain_02` | Hint: attacking a flag (hint overlay picture) | PNG RGB8 → BC7 sRGB, no mips | 200x200 px display size; author at 2x (400x400) | single | launch | diagram card |
| `content/textures/help/hint_camera_controls.png` | `help.hint.camera_controls` | Hint: camera controls, shown during the grace period; text labels come from the string table | PNG RGBA8 → BC7 sRGB, no mips | 512x128 px display size; author at 2x (1024x256) | single | launch | diagram card |
| `content/textures/help/mode_card_spawn_control.png` | `help.mode_card` | Mode card on the mode-select buttons, one per mode | PNG RGB8 → BC7 sRGB, no mips | 300x200 px display size; author at 2x (600x400); Spawn Control | 1 of 6 | skirmish-first-playable | diagram card |
| `content/textures/help/mode_card_zone_control.png` | `help.mode_card` | ″ | PNG RGB8 → BC7 sRGB, no mips | 300x200 px display size; author at 2x (600x400); Zone Control | 2 of 6 | launch | diagram card |
| `content/textures/help/mode_card_relic_run.png` | `help.mode_card` | ″ | PNG RGB8 → BC7 sRGB, no mips | 300x200 px display size; author at 2x (600x400); Relic Run | 3 of 6 | launch | diagram card |
| `content/textures/help/mode_card_siege.png` | `help.mode_card` | ″ | PNG RGB8 → BC7 sRGB, no mips | 300x200 px display size; author at 2x (600x400); Siege | 4 of 6 | launch | diagram card |
| `content/textures/help/mode_card_launch_race.png` | `help.mode_card` | ″ | PNG RGB8 → BC7 sRGB, no mips | 300x200 px display size; author at 2x (600x400); Launch Race | 5 of 6 | launch | diagram card |
| `content/textures/help/mode_card_flag_chain.png` | `help.mode_card` | ″ | PNG RGB8 → BC7 sRGB, no mips | 300x200 px display size; author at 2x (600x400); Flag Chain | 6 of 6 | launch | diagram card |
| `content/textures/help/help_general_page_01.png` | `help.general_page` | General help page picture, pages 1-5 | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 1 of 5 | launch | diagram card |
| `content/textures/help/help_general_page_02.png` | `help.general_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 2 of 5 | launch | diagram card |
| `content/textures/help/help_general_page_03.png` | `help.general_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x149 px display size; author at 2x (300x298) | 3 of 5 | launch | diagram card |
| `content/textures/help/help_general_page_04.png` | `help.general_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 4 of 5 | launch | diagram card |
| `content/textures/help/help_general_page_05.png` | `help.general_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 5 of 5 | launch | diagram card |
| `content/textures/help/help_spawn_control_page_01.png` | `help.mode_page` | Per-mode help page picture (pages per mode: Spawn Control 1, Zone Control 2, Relic Run 3, Siege 3, Launch Race 4, Flag Chain 3) | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 1 of 16 | launch | diagram card |
| `content/textures/help/help_zone_control_page_01.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 2 of 16 | launch | diagram card |
| `content/textures/help/help_zone_control_page_02.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 3 of 16 | launch | diagram card |
| `content/textures/help/help_relic_run_page_01.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 4 of 16 | launch | diagram card |
| `content/textures/help/help_relic_run_page_02.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 5 of 16 | launch | diagram card |
| `content/textures/help/help_relic_run_page_03.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 6 of 16 | launch | diagram card |
| `content/textures/help/help_siege_page_01.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 7 of 16 | launch | diagram card |
| `content/textures/help/help_siege_page_02.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 8 of 16 | launch | diagram card |
| `content/textures/help/help_siege_page_03.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 9 of 16 | launch | diagram card |
| `content/textures/help/help_launch_race_page_01.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 10 of 16 | launch | diagram card |
| `content/textures/help/help_launch_race_page_02.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 11 of 16 | launch | diagram card |
| `content/textures/help/help_launch_race_page_03.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 12 of 16 | launch | diagram card |
| `content/textures/help/help_launch_race_page_04.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 13 of 16 | launch | diagram card |
| `content/textures/help/help_flag_chain_page_01.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 14 of 16 | launch | diagram card |
| `content/textures/help/help_flag_chain_page_02.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 15 of 16 | launch | diagram card |
| `content/textures/help/help_flag_chain_page_03.png` | `help.mode_page` | ″ | PNG RGB8 → BC7 sRGB, no mips | 150x150 px display size; author at 2x (300x300) | 16 of 16 | launch | diagram card |
| `content/textures/help/explainer/explainer_transport.png` | `help.explainer_picture` | Picture animated by the explainer scripts | PNG RGBA8 → BC7 sRGB, no mips | Transport; 128x80 px; used by Launch Race | 1 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_glow.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | soft glow; 128x128 px; used by Flag Chain, Relic Run, Siege, Spawn Control | 2 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_worker.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | Worker figure; 128x128 px; used by Flag Chain, Relic Run, Siege, Spawn Control | 3 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_worker_hurt.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | hurt Worker; 128x128 px; used by Flag Chain, Relic Run, Siege, Spawn Control | 4 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_flag_neutral.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | neutral flag; 64x64 px; used by Flag Chain | 5 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_flag_lowered.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | lowered flag; 64x64 px; used by Flag Chain | 6 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_flag_locked.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | locked flag; 64x64 px; used by Flag Chain | 7 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_flag_raised.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | raised flag; 64x64 px; used by Flag Chain | 8 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_fuel_pump.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | fuel pump; 181x256 px; used by Launch Race | 9 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_fuel_pump_arm.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | fuel pump arm; 50x256 px; used by Flag Chain, Launch Race | 10 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_island_shape.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | island outline; 256x128 px; used by Launch Race | 11 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_beam.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | laser beam; 128x32 px; used by Flag Chain, Relic Run, Spawn Control, Zone Control | 12 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_muzzle_flash.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | muzzle flash; 64x64 px; used by Flag Chain, Relic Run, Siege, Spawn Control | 13 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_score_plus_one.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | +1 score pop-up; 128x128 px; used by Relic Run, Zone Control | 14 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_launch_vessel.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | Launch Vessel; 238x256 px; used by Launch Race | 15 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_solar_array.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | Solar Array; 158x256 px; used by Launch Race | 16 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_spawn_node.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | Spawn Node; 260x256 px; used by Relic Run, Spawn Control, Zone Control | 17 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_relic.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | Relic; 256x256 px; used by Relic Run | 18 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_transit_gate.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | Transit Gate; 256x256 px; used by Flag Chain, Launch Race, Siege | 19 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_turret_barrel.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | turret barrel; 140x90 px; used by Siege | 20 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_turret_base.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | turret base; 140x110 px; used by Siege | 21 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_water.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | water band; 256x128 px; used by Launch Race | 22 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_core_device.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | Core Device; 207x256 px; used by Siege | 23 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_zone_back.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | Control Zone, back layer; 512x310 px; used by Relic Run, Zone Control | 24 of 25 | launch | generated panel |
| `content/textures/help/explainer/explainer_zone_front.png` | `help.explainer_picture` | ″ | PNG RGBA8 → BC7 sRGB, no mips | Control Zone, front layer; 512x310 px; used by Relic Run, Zone Control | 25 of 25 | launch | generated panel |
| `content/ui/explainers/explainer_siege.json` | `anim.explainer` | Explainer animation: a timed key-frame script that animates pictures on a canvas, one per mode | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 900x250 px, loop 12.02 s; Siege | 1 of 6 | launch | static mode card |
| `content/ui/explainers/explainer_flag_chain.json` | `anim.explainer` | ″ | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 900x250 px, loop 24.01 s; Flag Chain | 2 of 6 | launch | static mode card |
| `content/ui/explainers/explainer_relic_run.json` | `anim.explainer` | ″ | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 924x250 px, loop 20.01 s; Relic Run | 3 of 6 | launch | static mode card |
| `content/ui/explainers/explainer_spawn_control.json` | `anim.explainer` | ″ | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 800x250 px, loop 15.01 s; Spawn Control | 4 of 6 | launch | static mode card |
| `content/ui/explainers/explainer_zone_control.json` | `anim.explainer` | ″ | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 800x250 px, loop 15.66 s; Zone Control | 5 of 6 | launch | static mode card |
| `content/ui/explainers/explainer_launch_race.json` | `anim.explainer` | ″ | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 1000x250 px, loop 30.01 s; Launch Race | 6 of 6 | launch | static mode card |

## 13 Audio by source and event

Sound effects are sample groups named by **source and event**: a group holds the variants one event picks from, and its file count matches the reference group, so the mix keeps the same variety. Behaviour (volume, pitch spread, distance, looping, priority) lives in the sound-event data of [10](../reference/10_audio_design.md), not in the files. Where one reference group serves several events, the Species group is named after its main binding and the other bindings are listed; two files are shared by two groups and listed once. Formats: WAV 48 kHz 24-bit → 16-bit PCM resident (Opus stream for files over 20 s). Placeholder: synthesised tone (voice lines: text-to-speech line).

182 (source, event) bindings fire in a match and play these groups. A further 98 events fire with no bound sound; they stay silent in the reference, so the event table lists them and no file is needed.

### 13.1 Builder (15 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/builder/builder_presence_loop.wav` | `sfx.unit.builder.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 0.78 s | 1 variant | launch |
| `content/audio/sfx/builder/builder_hurt_{01..09}.wav` (9 files) | `sfx.unit.builder.hurt` | takes damage | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 1.36-2.72 s | 9 variants | launch |
| `content/audio/sfx/builder/builder_die_{01..05}.wav` (5 files) | `sfx.unit.builder.die` | dies | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 2.35-3.48 s | 5 variants | launch |

### 13.2 Crawler (47 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/crawler/crawler_attack_{01..08}.wav` (8 files) | `sfx.creature.crawler.attack` | attacks | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.39-0.71 s | 8 variants | launch |
| `content/audio/sfx/crawler/crawler_hurt_{01..03}.wav` (3 files) | `sfx.creature.crawler.hurt` | takes damage | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 0.29-0.72 s | 3 variants | launch |
| `content/audio/sfx/crawler/crawler_turn_{01..29}.wav` (29 files) | `sfx.creature.crawler.turn` | changes direction | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.06-0.10 s | 29 variants | launch |
| `content/audio/sfx/crawler/crawler_die_{01..07}.wav` (7 files) | `sfx.creature.crawler.die` | dies | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.24-0.80 s | 7 variants | launch |

### 13.3 Trooper (48 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/trooper/trooper_presence_loop_{01..20}.wav` (20 files) | `sfx.unit.trooper.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 0.30-0.95 s | 20 variants | skirmish-first-playable |
| `content/audio/sfx/trooper/trooper_hurt_{01..04}.wav` (4 files) | `sfx.unit.trooper.hurt` | takes damage | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.31-0.83 s | 4 variants | skirmish-first-playable |
| `content/audio/sfx/trooper/trooper_weapon_ready.wav` | `sfx.unit.trooper.weapon_ready` | weapon is ready again | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.52 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/trooper/trooper_fire_laser_{01..06}.wav` (6 files) | `sfx.unit.trooper.fire_laser` | fires its laser | `turret.fire_gun` | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.93-1.04 s | 6 variants | skirmish-first-playable |
| `content/audio/sfx/trooper/trooper_die_{01..05}.wav` (5 files) | `sfx.unit.trooper.die` | dies | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 1.85-2.36 s | 5 variants | skirmish-first-playable |
| `content/audio/sfx/trooper/trooper_enter_gate_{01..06}.wav` (6 files) | `sfx.unit.trooper.enter_gate` | enters a gate | `leader.enter_gate` | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 0.63-0.68 s | 6 variants | skirmish-first-playable |
| `content/audio/sfx/trooper/trooper_exit_gate_{01..06}.wav` (6 files) | `sfx.unit.trooper.exit_gate` | leaves a gate | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.63-0.68 s | 6 variants | skirmish-first-playable |

### 13.4 Brood Drifter (15 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/brood_drifter/brood_drifter_presence_loop_{01..03}.wav` (3 files) | `sfx.creature.brood_drifter.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 4.68-5.63 s | 3 variants | launch |
| `content/audio/sfx/brood_drifter/brood_drifter_hurt_{01..06}.wav` (6 files) | `sfx.creature.brood_drifter.hurt` | takes damage | `spitter_plant.damaged` | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.36-4.70 s | 6 variants | launch |
| `content/audio/sfx/brood_drifter/brood_drifter_die_{01..06}.wav` (6 files) | `sfx.creature.brood_drifter.die` | dies | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 2.50-5.12 s | 6 variants | launch |

### 13.5 spider (53 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/spider/spider_lay_egg_{01..10}.wav` (10 files) | `sfx.creature.spider.lay_egg` | lays an egg | `brood_drifter.lay_egg` | mono, 48 kHz; positional, follows the source; one-shot; reference duration 1.20-2.64 s | 10 variants | launch |
| `content/audio/sfx/spider/spider_presence_loop_{01..17}.wav` (17 files) | `sfx.creature.spider.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 0.51-2.25 s | 17 variants | launch |
| `content/audio/sfx/spider/spider_die_{01..05}.wav` (5 files) | `sfx.creature.spider.die` | dies | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.81-3.16 s | 5 variants + 2 shared | launch |
| `content/audio/sfx/spider/spider_leap_{01..06}.wav` (6 files) | `sfx.creature.spider.pounce` | pounces | `spider.attack` | mono, 48 kHz; positional, follows the source; one-shot; reference duration 3.42-4.39 s | 6 variants | launch |
| `content/audio/sfx/spider/spider_hurt_{01..02}.wav` (2 files) | `sfx.creature.spider.hurt` | takes damage | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.81-3.15 s | 2 variants | launch |
| `content/audio/sfx/spider/spider_footstep_{01..13}.wav` (13 files) | `sfx.creature.spider.footstep` | footstep | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.11-0.36 s | 13 variants | launch |

### 13.6 centipede (21 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/centipede/centipede_bite_{01..08}.wav` (8 files) | `sfx.creature.centipede.attack` | attacks | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.59-1.37 s | 8 variants | launch |
| `content/audio/sfx/centipede/centipede_presence_loop_{01..04}.wav` (4 files) | `sfx.creature.centipede.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 2.90-4.27 s | 4 variants | launch |
| `content/audio/sfx/centipede/centipede_die_{01..04}.wav` (4 files) | `sfx.creature.centipede.die` | dies | `launch_vessel.explode (layer 2)` | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 4.27-5.41 s | 4 variants | launch |
| `content/audio/sfx/centipede/centipede_hurt_{01..05}.wav` (5 files) | `sfx.creature.centipede.hurt` | takes damage | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.95-1.66 s | 5 variants | launch |

### 13.7 Bomber (2 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/bomber/bomber_drop_bomb.wav` | `sfx.unit.bomber.drop_bomb` | drops a bomb | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 4.00 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/bomber/bomber_presence_loop.wav` | `sfx.unit.bomber.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 3.88 s | 1 variant | skirmish-first-playable |

### 13.8 Worker (194 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/worker/worker_die_{01..08}.wav` (8 files) | `sfx.unit.worker.die` | dies | `leader.die` | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 0.48-1.23 s | 8 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_die_b_{01..12}.wav` (12 files) | `sfx.unit.worker.die_b` | dies (additional layer) | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 0.26-1.12 s | 12 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_hurt_{01..18}.wav` (18 files) | `sfx.unit.worker.hurt` | takes damage | `leader.hurt` | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.21-0.83 s | 18 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_spawn_{01..15}.wav` (15 files) | `sfx.unit.worker.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.08-0.48 s | 15 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_flee_bark_{01..10}.wav` (10 files) | `sfx.unit.worker.flee_bark` | flees from a threat (voice) | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.41-1.02 s | 10 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_flee_bark_b_{01..26}.wav` (26 files) | `sfx.unit.worker.flee_bark_b` | flees from a threat (voice) (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.30-1.68 s | 26 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_attack_{01..24}.wav` (24 files) | `sfx.unit.worker.attack` | attacks | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.50-1.00 s | 24 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_attack_b_{01..05}.wav` (5 files) | `sfx.unit.worker.attack_b` | attacks (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.73-1.40 s | 5 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_victory_jump_{01..09}.wav` (9 files) | `sfx.unit.worker.victory_jump` | jumps in victory | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.45-3.11 s | 9 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_burning_{01..13}.wav` (13 files) | `sfx.unit.worker.burning` | burns | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 0.25-4.06 s | 13 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_selected_{01..10}.wav` (10 files) | `sfx.unit.worker.selected` | is selected (voice) | `worker.ordered_by_player` | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.15-0.36 s | 10 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_lift_relic_{01..04}.wav` (4 files) | `sfx.unit.worker.lift_relic` | lifts a Relic | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 3.01-3.01 s | 4 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_attack_bark_{01..37}.wav` (37 files) | `sfx.unit.worker.attack_bark` | charges a threat (voice) | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.40-1.42 s | 37 variants | skirmish-first-playable |
| `content/audio/sfx/worker/worker_attack_horn_{01..03}.wav` (3 files) | `sfx.unit.worker.attack_horn` | sounds the attack | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 3.07-4.37 s | 3 variants | skirmish-first-playable |

### 13.9 Leader (23 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/leader/leader_spawn_{01..06}.wav` (6 files) | `sfx.unit.leader.spawn` | appears | `worker.ordered`, `leader.order_waypoint (layer 2)`, `leader.selected`, `leader.order_formation (layer 2)` | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.27-0.53 s | 6 variants | skirmish-first-playable |
| `content/audio/sfx/leader/leader_order_waypoint_{01..17}.wav` (17 files) | `sfx.unit.leader.order_waypoint` | is given a waypoint order | `leader.order_formation` | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 3.00-3.00 s | 17 variants | skirmish-first-playable |

### 13.10 ant (27 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/ant/ant_bite_{01..06}.wav` (6 files) | `sfx.creature.ant.attack` | attacks | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.09-0.17 s | 6 variants | launch |
| `content/audio/sfx/ant/ant_presence_loop_{01..08}.wav` (8 files) | `sfx.creature.ant.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 0.08-0.13 s | 8 variants | launch |
| `content/audio/sfx/ant/ant_hurt_{01..07}.wav` (7 files) | `sfx.creature.ant.hurt` | takes damage | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.17-0.34 s | 7 variants | launch |
| `content/audio/sfx/ant/ant_die_{01..06}.wav` (6 files) | `sfx.creature.ant.die` | dies | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.45-0.80 s | 6 variants | launch |

### 13.11 Transport (30 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/transport/transport_presence_loop.wav` | `sfx.unit.transport.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 17.75 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/transport/transport_spawn_{01..06}.wav` (6 files) | `sfx.unit.transport.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 3.01-3.29 s | 6 variants | skirmish-first-playable |
| `content/audio/sfx/transport/transport_hurt_{01..10}.wav` (10 files) | `sfx.unit.transport.hurt` | takes damage | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 3.11-5.33 s | 10 variants | skirmish-first-playable |
| `content/audio/sfx/transport/transport_die_{01..09}.wav` (9 files) | `sfx.unit.transport.die` | dies | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 3.54-5.33 s | 9 variants | skirmish-first-playable |
| `content/audio/sfx/transport/transport_ordered_{01..04}.wav` (4 files) | `sfx.unit.transport.ordered` | receives an order | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 4.54-4.88 s | 4 variants | skirmish-first-playable |

### 13.12 Reaper Serpent (47 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/reaper_serpent/reaper_serpent_presence_loop_{01..11}.wav` (11 files) | `sfx.creature.reaper_serpent.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 0.81-2.01 s | 11 variants | launch |
| `content/audio/sfx/reaper_serpent/reaper_serpent_hurt_{01..10}.wav` (10 files) | `sfx.creature.reaper_serpent.hurt` | takes damage | `reaper_serpent.spawn (layer 2)` | mono, 48 kHz; positional, follows the source; one-shot; reference duration 1.61-3.35 s | 10 variants | launch |
| `content/audio/sfx/reaper_serpent/reaper_serpent_die_{01..10}.wav` (10 files) | `sfx.creature.reaper_serpent.die` | dies | `seed_pod.die` | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.29-3.42 s | 10 variants | launch |
| `content/audio/sfx/reaper_serpent/reaper_serpent_attack_{01..07}.wav` (7 files) | `sfx.creature.reaper_serpent.attack` | attacks | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 1.44-2.11 s | 7 variants | launch |
| `content/audio/sfx/reaper_serpent/reaper_serpent_enemy_sighted_{01..09}.wav` (9 files) | `sfx.creature.reaper_serpent.enemy_sighted` | sights an enemy | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 1.23-3.04 s | 9 variants | launch |

### 13.13 Seed Pod (20 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/seed_pod/seed_pod_spawn_{01..05}.wav` (5 files) | `sfx.creature.seed_pod.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.99-1.54 s | 5 variants | launch |
| `content/audio/sfx/seed_pod/seed_pod_bounce_{01..05}.wav` (5 files) | `sfx.creature.seed_pod.bounce` | bounces | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 1.25-1.28 s | 5 variants | launch |
| `content/audio/sfx/seed_pod/seed_pod_burst_open_{01..10}.wav` (10 files) | `sfx.creature.seed_pod.burst_open` | bursts open | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 2.01-2.58 s | 10 variants | launch |

### 13.14 Abductor (12 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/abductor/abductor_presence_loop_b.wav` | `sfx.creature.abductor.presence_loop_b` | loop while it exists (additional layer) | `collector.presence_loop` | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 1.39 s | 1 variant | launch |
| `content/audio/sfx/abductor/abductor_presence_loop_c.wav` | `sfx.creature.abductor.presence_loop_c` | loop while it exists (additional layer) | `collector.presence_loop (layer 2)` | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 6.63 s | 1 variant | launch |
| `content/audio/sfx/abductor/abductor_presence_loop_{01..09}.wav` (9 files) | `sfx.creature.abductor.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 15.14-23.43 s | 9 variants | launch |
| `content/audio/sfx/abductor/abductor_tractor_beam.wav` | `sfx.creature.abductor.tractor_beam` | tractor beam loop | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 3.99 s | 1 variant | launch |

### 13.15 Collector (7 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/collector/collector_presence_loop.wav` | `sfx.unit.collector.presence_loop` | loop while it exists | `meteor.presence_loop (layer 2)`, `abductor.presence_loop (layer 4)` | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 1.40 s | 1 variant | launch |
| `content/audio/sfx/collector/collector_drop_essence_{01..06}.wav` (6 files) | `sfx.unit.collector.drop_essence` | drops Essence orbs | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 1.12-1.59 s | 6 variants | launch |

### 13.16 Missile Barrage missile (3 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/barrage_missile/barrage_missile_presence_loop.wav` | `sfx.weapon.barrage_missile.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 3.23 s | 1 variant | launch |
| `content/audio/sfx/barrage_missile/barrage_missile_explode.wav` | `sfx.weapon.barrage_missile.explode` | explodes | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 7.25 s | 1 variant | launch |
| `content/audio/sfx/barrage_missile/barrage_missile_explode_b.wav` | `sfx.weapon.barrage_missile.explode_b` | explodes (additional layer) | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 10.08 s | 1 variant | launch |

### 13.17 meteor (3 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/meteor/meteor_land.wav` | `sfx.weapon.meteor.land` | lands | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 7.25 s | 1 variant | launch |
| `content/audio/sfx/meteor/meteor_land_b.wav` | `sfx.weapon.meteor.land_b` | lands (additional layer) | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 5.79 s | 1 variant | launch |
| `content/audio/sfx/meteor/meteor_presence_loop.wav` | `sfx.weapon.meteor.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 3.22 s | 1 variant | launch |

### 13.18 Beam Dish (11 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/beam_dish/beam_dish_rotate_start.wav` | `sfx.building.beam_dish.rotate_start` | starts turning | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 1.50 s | 1 variant | launch |
| `content/audio/sfx/beam_dish/beam_dish_rotate_start_b.wav` | `sfx.building.beam_dish.rotate_start_b` | starts turning (additional layer) | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 0.68 s | 1 variant | launch |
| `content/audio/sfx/beam_dish/beam_dish_rotate_stop.wav` | `sfx.building.beam_dish.rotate_stop` | stops turning | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 3.22 s | 1 variant | launch |
| `content/audio/sfx/beam_dish/beam_dish_link_up_{01..04}.wav` (4 files) | `sfx.building.beam_dish.link_up` | link established | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 0.66-1.75 s | 4 variants | launch |
| `content/audio/sfx/beam_dish/beam_dish_link_lost_{01..04}.wav` (4 files) | `sfx.building.beam_dish.link_lost` | link lost | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 1.17-1.79 s | 4 variants | launch |

### 13.19 Barrier Emitter (23 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/barrier_emitter/barrier_emitter_presence_loop.wav` | `sfx.building.barrier_emitter.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 0.02 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/barrier_emitter/barrier_emitter_spark_{01..06}.wav` (6 files) | `sfx.building.barrier_emitter.spark` | sparks | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.88-1.96 s | 6 variants | skirmish-first-playable |
| `content/audio/sfx/barrier_emitter/barrier_emitter_damaged_{01..16}.wav` (16 files) | `sfx.building.barrier_emitter.damaged` | is damaged | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.79-2.46 s | 16 variants | skirmish-first-playable |

### 13.20 turret (16 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/turret/turret_spawn_{01..06}.wav` (6 files) | `sfx.building.turret.spawn` | appears | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 5.13-6.83 s | 6 variants | skirmish-first-playable |
| `content/audio/sfx/turret/turret_fire_flame_{01..10}.wav` (10 files) | `sfx.building.turret.fire_flame` | fires flame | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 0.47-0.99 s | 10 variants | skirmish-first-playable |

### 13.21 tree (41 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/tree/tree_ambient_normal_{01..08}.wav` (8 files) | `sfx.building.tree.ambient_normal` | ambient loop | `tree.ambient_essence` | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 8.00-8.00 s | 8 variants | skirmish-first-playable |
| `content/audio/sfx/tree/tree_burning_{01..15}.wav` (15 files) | `sfx.building.tree.burning` | burns | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 0.44-13.12 s | 15 variants | skirmish-first-playable |
| `content/audio/sfx/tree/tree_spawn.wav` | `sfx.building.tree.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 12.05 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/tree/tree_ambient_blight_{01..17}.wav` (17 files) | `sfx.building.tree.ambient_blight` | Blight Grove ambient loop | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 2.17-6.69 s | 17 variants | skirmish-first-playable |

### 13.22 grid pylon (22 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/grid_pylon/grid_pylon_surge_{01..22}.wav` (22 files) | `sfx.building.grid_pylon.surge` | energy surge passes | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.28-0.57 s | 22 variants | launch |

### 13.23 Solar Array (6 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/solar_array/solar_array_surge_{01..05}.wav` (5 files) | `sfx.building.solar_array.surge` | energy surge passes | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.17-1.38 s | 5 variants | launch |
| `content/audio/sfx/solar_array/solar_array_charging.wav` | `sfx.building.solar_array.charging` | charging loop | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 3.71 s | 1 variant | launch |

### 13.24 Hive (10 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/hive/hive_damaged_{01..10}.wav` (10 files) | `sfx.building.hive.damaged` | is damaged | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.77-4.48 s | 10 variants | launch |

### 13.25 grenade (17 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/grenade/grenade_explode_{01..05}.wav` (5 files) | `sfx.weapon.grenade.explode` | explodes | `hive.explode` | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 3.70-4.08 s | 5 variants | skirmish-first-playable |
| `content/audio/sfx/grenade/grenade_spawn_{01..05}.wav` (5 files) | `sfx.weapon.grenade.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 3.25-3.25 s | 5 variants | skirmish-first-playable |
| `content/audio/sfx/grenade/grenade_fuse_blink.wav` | `sfx.weapon.grenade.fuse_blink` | fuse blink | `airstrike_bomb.fuse_blink` | mono, 48 kHz; positional, follows the source; one-shot; reference duration 1.00 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/grenade/grenade_bounce_{01..06}.wav` (6 files) | `sfx.weapon.grenade.bounce` | bounces | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.37-0.73 s | 6 variants | skirmish-first-playable |

### 13.26 Spitter Plant (3 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/spitter_plant/spitter_plant_burning_{01..03}.wav` (3 files) | `sfx.building.spitter_plant.burning` | burns | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 17.89-17.99 s | 3 variants | launch |

### 13.27 Spawn Node (2 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/spawn_node/spawn_node_activate.wav` | `sfx.building.spawn_node.activate` | activates | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 9.39 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/spawn_node/spawn_node_deactivate.wav` | `sfx.building.spawn_node.deactivate` | deactivates | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 7.88 s | 1 variant | skirmish-first-playable |

### 13.28 fuel pump (10 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/fuel_pump/fuel_pump_pump_up.wav` | `sfx.building.fuel_pump.pump_up` | pump stroke up | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 5.59 s | 1 variant | launch |
| `content/audio/sfx/fuel_pump/fuel_pump_pump_up_b_{01..05}.wav` (5 files) | `sfx.building.fuel_pump.pump_up_b` | pump stroke up (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.19-5.08 s | 5 variants | launch |
| `content/audio/sfx/fuel_pump/fuel_pump_pump_down.wav` | `sfx.building.fuel_pump.pump_down` | pump stroke down | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.89 s | 1 variant | launch |
| `content/audio/sfx/fuel_pump/fuel_pump_pump_down_b_{01..03}.wav` (3 files) | `sfx.building.fuel_pump.pump_down_b` | pump stroke down (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.04-2.97 s | 3 variants | launch |

### 13.29 fuel line (1 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/fuel_line/fuel_line_flow.wav` | `sfx.building.fuel_line.flow` | fuel flow loop | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 4.23 s | 1 variant | launch |

### 13.30 fuel depot (5 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/fuel_depot/fuel_depot_load_passenger_{01..05}.wav` (5 files) | `sfx.building.fuel_depot.load_passenger` | loads a passenger | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 0.55-0.83 s | 5 variants | launch |

### 13.31 Launch Vessel (25 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/launch_vessel/launch_vessel_fuelled_{01..05}.wav` (5 files) | `sfx.building.launch_vessel.fuelled` | fuelled loop | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 3.13-3.60 s | 5 variants | launch |
| `content/audio/sfx/launch_vessel/launch_vessel_fuelled_b.wav` | `sfx.building.launch_vessel.fuelled_b` | fuelled loop (additional layer) | `match_ui.lobby_bed (layer 2)` | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 1.44 s | 1 variant | launch |
| `content/audio/sfx/launch_vessel/launch_vessel_malfunction.wav` | `sfx.building.launch_vessel.malfunction` | malfunction loop | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 1.22 s | 1 variant | launch |
| `content/audio/sfx/launch_vessel/launch_vessel_explode.wav` | `sfx.building.launch_vessel.explode` | explodes | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 5.25 s | 1 variant | launch |
| `content/audio/sfx/launch_vessel/launch_vessel_damaged_{01..02}.wav` (2 files) | `sfx.building.launch_vessel.damaged` | is damaged | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 3.78-3.93 s | 2 variants | launch |
| `content/audio/sfx/launch_vessel/launch_vessel_engine_burn.wav` | `sfx.building.launch_vessel.engine_burn` | engine burn | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 3.27 s | 1 variant | launch |
| `content/audio/sfx/launch_vessel/launch_vessel_engine_burn_b.wav` | `sfx.building.launch_vessel.engine_burn_b` | engine burn (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.61 s | 1 variant | launch |
| `content/audio/sfx/launch_vessel/launch_vessel_hungry_{01..07}.wav` (7 files) | `sfx.building.launch_vessel.hungry` | needs fuel loop | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 4.50-7.21 s | 7 variants | launch |
| `content/audio/sfx/launch_vessel/launch_vessel_flight_{01..06}.wav` (6 files) | `sfx.building.launch_vessel.flight` | launch flight loop | `core_device.presence_loop` | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 5.80-6.25 s | 6 variants | launch |

### 13.32 Control Zone (6 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/control_zone/control_zone_relic_scored.wav` | `sfx.building.control_zone.relic_scored` | a Relic is scored | `relic.owner_changed` | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 1.16 s | 1 variant | launch |
| `content/audio/sfx/control_zone/control_zone_relic_scored_b.wav` | `sfx.building.control_zone.relic_scored_b` | a Relic is scored (additional layer) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 4.55 s | 1 variant | launch |
| `content/audio/sfx/control_zone/control_zone_relic_scored_c.wav` | `sfx.building.control_zone.relic_scored_c` | a Relic is scored (additional layer) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 2.98 s | 1 variant | launch |
| `content/audio/sfx/control_zone/control_zone_owner_changed.wav` | `sfx.building.control_zone.owner_changed` | changes owner | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 1.95 s | 1 variant | launch |
| `content/audio/sfx/control_zone/control_zone_owner_changed_b.wav` | `sfx.building.control_zone.owner_changed_b` | changes owner (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.79 s | 1 variant | launch |
| `content/audio/sfx/control_zone/control_zone_presence_loop.wav` | `sfx.building.control_zone.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 5.35 s | 1 variant | launch |

### 13.33 Command Console (4 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/command_console/command_console_presence_loop_{01..04}.wav` (4 files) | `sfx.building.command_console.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 7.15-7.89 s | 4 variants | launch |

### 13.34 Supply crate (18 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/supply_crate/supply_crate_land.wav` | `sfx.crate.supply_crate.land` | lands | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 4.03 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_capture_begin.wav` | `sfx.crate.supply_crate.capture_begin` | capture starts | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 4.57 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_capture_begin_b.wav` | `sfx.crate.supply_crate.capture_begin_b` | capture starts (additional layer) | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 7.99 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_capture_begin_c.wav` | `sfx.crate.supply_crate.capture_begin_c` | capture starts (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 5.30 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_capture_lost.wav` | `sfx.crate.supply_crate.capture_lost` | capture lost | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 2.52 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_capture_progress_00.wav` | `sfx.crate.supply_crate.capture_progress_00` | capture under way, 0-20 % | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 16.48 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_capture_progress_20.wav` | `sfx.crate.supply_crate.capture_progress_20` | capture under way, 20-40 % | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 11.94 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_capture_progress_40.wav` | `sfx.crate.supply_crate.capture_progress_40` | capture under way, 40-60 % | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 15.41 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_capture_progress_60.wav` | `sfx.crate.supply_crate.capture_progress_60` | capture under way, 60-80 % | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 12.36 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_capture_progress_80.wav` | `sfx.crate.supply_crate.capture_progress_80` | capture under way, 80-100 % | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 14.77 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_captured.wav` | `sfx.crate.supply_crate.captured` | is captured | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 3.56 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/supply_crate/supply_crate_falling_{01..07}.wav` (7 files) | `sfx.crate.supply_crate.falling` | falls from the sky | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 12.47-13.26 s | 7 variants | skirmish-first-playable |

### 13.35 Relic (3 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/relic/relic_owner_changed.wav` | `sfx.building.relic.owner_changed` | changes owner | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 2.81 s | 1 variant | launch |
| `content/audio/sfx/relic/relic_drop.wav` | `sfx.building.relic.drop` | is dropped | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 5.91 s | 1 variant | launch |
| `content/audio/sfx/relic/relic_spawn.wav` | `sfx.building.relic.spawn` | appears | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 1.28 s | 1 variant | launch |

### 13.36 laser bolt (36 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/laser_bolt/laser_bolt_hit_bark_{01..36}.wav` (36 files) | `sfx.weapon.laser_bolt.hit_bark` | a unit is hit by a laser bolt (voice of the hit unit) | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.20-0.80 s | 36 variants | skirmish-first-playable |

### 13.37 rocket (6 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/rocket/rocket_explode_{01..04}.wav` (4 files) | `sfx.weapon.rocket.explode` | explodes | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 4.33-5.69 s | 4 variants | skirmish-first-playable |
| `content/audio/sfx/rocket/rocket_presence_loop.wav` | `sfx.weapon.rocket.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; reference duration 0.46 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/rocket/rocket_spawn.wav` | `sfx.weapon.rocket.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 0.83 s | 1 variant | skirmish-first-playable |

### 13.38 airstrike bomb (5 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/airstrike_bomb/airstrike_bomb_explode_{01..05}.wav` (5 files) | `sfx.weapon.airstrike_bomb.explode` | explodes | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 5.88-6.01 s | 5 variants | skirmish-first-playable |

### 13.39 Essence orb (1 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/essence_orb/essence_orb_collected.wav` | `sfx.unit.essence_orb.collected` | is collected | — | mono, 48 kHz; positional, follows the source; one-shot; reference duration 1.97 s | 1 variant | skirmish-first-playable |

### 13.40 Supply-crate outcome stings (25 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/sfx/crate_outcome/outcome_strike_team.wav` | `sfx.crate.outcome.strike_team` | Supply crate yields strike team | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 2.03 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/crate_outcome/outcome_collector.wav` | `sfx.crate.outcome.collector` | Supply crate yields collector | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 5.95 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_transport.wav` | `sfx.crate.outcome.transport` | Supply crate yields transport | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 3.51 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/crate_outcome/outcome_reinforcements.wav` | `sfx.crate.outcome.reinforcements` | Supply crate yields reinforcements | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 6.62 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/crate_outcome/outcome_hive.wav` | `sfx.crate.outcome.hive` | Supply crate yields hive | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 5.21 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_brood.wav` | `sfx.crate.outcome.brood` | Supply crate yields brood | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 3.06 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_airstrike.wav` | `sfx.crate.outcome.airstrike` | Supply crate yields airstrike | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 4.83 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/crate_outcome/outcome_essence_grove.wav` | `sfx.crate.outcome.essence_grove` | Supply crate yields essence grove | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 4.00 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_blight_grove.wav` | `sfx.crate.outcome.blight_grove` | Supply crate yields blight grove | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 5.22 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_supply_storm.wav` | `sfx.crate.outcome.supply_storm` | Supply crate yields supply storm | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 4.16 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_gun_turret.wav` | `sfx.crate.outcome.gun_turret` | Supply crate yields gun turret | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 5.02 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/crate_outcome/outcome_meteor_shower.wav` | `sfx.crate.outcome.meteor_shower` | Supply crate yields meteor shower | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 4.58 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_speed_boost.wav` | `sfx.crate.outcome.speed_boost` | Supply crate yields speed boost | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 2.91 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/crate_outcome/outcome_missile_barrage.wav` | `sfx.crate.outcome.missile_barrage` | Supply crate yields missile barrage | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 8.12 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_sickness.wav` | `sfx.crate.outcome.sickness` | Supply crate yields sickness | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 6.88 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_allegiance_scrambler.wav` | `sfx.crate.outcome.allegiance_scrambler` | Supply crate yields allegiance scrambler | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 3.24 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_shield.wav` | `sfx.crate.outcome.shield` | Supply crate yields shield | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 3.09 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/crate_outcome/outcome_convert.wav` | `sfx.crate.outcome.convert` | Supply crate yields convert | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 8.19 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_spawn_surge.wav` | `sfx.crate.outcome.spawn_surge` | Supply crate yields spawn surge | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 8.08 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_frenzy.wav` | `sfx.crate.outcome.frenzy` | Supply crate yields frenzy | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 5.73 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_rocket_troops.wav` | `sfx.crate.outcome.rocket_troops` | Supply crate yields rocket troops | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 3.63 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_swarm.wav` | `sfx.crate.outcome.swarm` | Supply crate yields swarm | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 4.00 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_fury.wav` | `sfx.crate.outcome.fury` | Supply crate yields fury | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 4.31 s | 1 variant | skirmish-first-playable |
| `content/audio/sfx/crate_outcome/outcome_spitter_plants.wav` | `sfx.crate.outcome.spitter_plants` | Supply crate yields spitter plants | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 3.27 s | 1 variant | launch |
| `content/audio/sfx/crate_outcome/outcome_abductor.wav` | `sfx.crate.outcome.abductor` | Supply crate yields abductor | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 6.28 s | 1 variant | launch |

## 14 UI sounds

Interface sounds: menus, selection, countdown, chat and the power-slot panel. They are 2-D (not positional) except the unit highlight. Formats: WAV 48 kHz 24-bit → 16-bit PCM resident. Placeholder: synthesised tone.

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/ui/ui_countdown_tick.wav` | `sfx.ui.match_ui.countdown_tick` | countdown tick | `turret.kill_tick`, `match_ui.countdown_zero` | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 0.09 s | 1 variant | skirmish-first-playable |
| `content/audio/ui/ui_power_input_begin.wav` | `sfx.ui.power_input.begin` | power input starts | `match_ui.selection_circle` | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 3.54 s | 1 variant | skirmish-first-playable |
| `content/audio/ui/ui_power_input_confirm.wav` | `sfx.ui.power_input.confirm` | power input succeeds | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 2.21 s | 1 variant | skirmish-first-playable |
| `content/audio/ui/ui_power_slot_discard.wav` | `sfx.ui.power_slots.discard` | a power is discarded | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 2.51 s | 1 variant | skirmish-first-playable |
| `content/audio/ui/ui_power_slot_icon_hover_{01..02}.wav` (2 files) | `sfx.ui.power_slots.icon_hover` | pointer over an icon | `match_ui.menu_hover` | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 0.05-0.06 s | 2 variants | skirmish-first-playable |
| `content/audio/ui/ui_selection_circle.wav` | `sfx.ui.match_ui.selection_circle` | selection circle drawn | — | mono or stereo, 48 kHz; 2-D (not positional); seamless loop; reference duration 5.96 s | 1 variant | skirmish-first-playable |
| `content/audio/ui/ui_menu_select.wav` | `sfx.ui.match_ui.menu_select` | menu item chosen | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 1.86 s | 1 variant | skirmish-first-playable |
| `content/audio/ui/ui_menu_back.wav` | `sfx.ui.match_ui.menu_back` | menu back | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 2.17 s | 1 variant | skirmish-first-playable |
| `content/audio/ui/ui_unit_highlight_{01..02}.wav` (2 files) | `sfx.ui.match_ui.unit_highlight` | unit highlighted | — | mono, 48 kHz; positional, fixed where it started; one-shot; reference duration 0.02 s | 2 variants | skirmish-first-playable |
| `content/audio/ui/ui_unit_select.wav` | `sfx.ui.match_ui.unit_select` | unit selected | `match_ui.toggle_state` | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 0.11 s | 1 variant | skirmish-first-playable |
| `content/audio/ui/ui_error.wav` | `sfx.ui.match_ui.error` | invalid action | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 0.84 s | 1 variant | skirmish-first-playable |
| `content/audio/ui/ui_chat_message.wav` | `sfx.ui.match_ui.chat_message` | chat message arrives | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; reference duration 0.22 s | 1 variant | skirmish-first-playable |

## 15 Ambience

Ambience beds: 2-D loops under the match and under the lobby. Each bed has layers that play together; long beds stream. Formats: WAV 48 kHz 24-bit → 16-bit PCM resident or Opus stream (over 20 s). Placeholder: silence.

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `content/audio/ambience/amb_match_bed_{01..03}.wav` (3 files) | `amb.match_bed` | match bed | `match_ui.lobby_bed` | mono or stereo, 48 kHz; 2-D (not positional); seamless loop; reference duration 22.11-53.87 s | 3 variants | skirmish-first-playable |
| `content/audio/ambience/amb_match_bed_b.wav` | `amb.match_bed_b` | match bed (additional layer) | — | mono or stereo, 48 kHz; 2-D (not positional); seamless loop; reference duration 28.98 s | 1 variant | skirmish-first-playable |
| `content/audio/ambience/amb_match_bed_c_{01..07}.wav` (7 files) | `amb.match_bed_c` | match bed (additional layer) | — | mono or stereo, 48 kHz; 2-D (not positional); seamless loop; reference duration 15.31-39.56 s | 7 variants | skirmish-first-playable |
| `content/audio/ambience/amb_lobby_bed_{01..10}.wav` (10 files) | `amb.lobby_bed` | lobby ambience bed | — | mono or stereo, 48 kHz; 2-D (not positional); seamless loop; reference duration 19.42-36.02 s | 10 variants | launch |

## 16 Music

The reference plays no music in a match: its music events are never triggered. Species commissions new music. This is a brief entry: the slots below are the proposed minimum, and the design phase sets the final list, its triggers and the music bus. The style follows the look: sparse, electronic, cool timbres that leave room for the crowd and the glow; no pastiche of any existing score.

| File | Asset id | Role | Spec | Priority | Placeholder |
|---|---|---|---|---|---|
| `content/audio/music/front_end_theme.wav` | `music.front_end.theme` | Front-end theme, loops under the menus and the lobby | stereo, 48 kHz; seamless loop; 2-4 min | launch | silence |
| `content/audio/music/match_bed_01.wav` | `music.match.bed` | In-match music bed, low intensity, sits under the crowd and the glow | stereo, 48 kHz; seamless loop; 3-5 min; mixed below the effects bus | launch | silence |
| `content/audio/music/match_bed_02.wav` | `music.match.bed` | In-match music bed, low intensity, sits under the crowd and the glow | stereo, 48 kHz; seamless loop; 3-5 min | launch | silence |
| `content/audio/music/match_bed_03.wav` | `music.match.bed` | In-match music bed, low intensity, sits under the crowd and the glow | stereo, 48 kHz; seamless loop; 3-5 min | launch | silence |
| `content/audio/music/sting_victory.wav` | `music.match.sting_victory` | Victory sting at the end of a match | stereo, 48 kHz; one-shot; 5-10 s | launch | silence |
| `content/audio/music/sting_defeat.wav` | `music.match.sting_defeat` | Defeat sting at the end of a match | stereo, 48 kHz; one-shot; 5-10 s | launch | silence |

## 17 Fonts

Fonts are licensed families (open licence or commercial, licensed for embedding), delivered as font files and rasterised at run time into a glyph cache; there are no bitmap font atlases. The reference uses three text faces; the primary face also carries Japanese and Korean, which Species splits into companion files. The glyph renderer assigns faces to text roles ([ui_framework](../architecture/components/ui_framework/layout_rendering_input_and_localisation.md)). Coverage is the minimum; a family with wider coverage is welcome. Placeholder: fallback font.

| File | Asset id | Role | Coverage | Priority |
|---|---|---|---|---|
| `content/fonts/ui_primary.otf` | `font.ui.primary` | Primary UI face, Latin and Cyrillic scripts (the reference face that also carries Japanese and Korean) | U+0020-00FF, U+0410-044F, U+20A0-20AF | skirmish-first-playable |
| `content/fonts/ui_primary_ja.otf` | `font.ui.primary_ja` | Japanese companion of the primary face (kana and CJK ideographs) | U+3000-30FF, U+4E00-9EFF | launch |
| `content/fonts/ui_primary_ko.otf` | `font.ui.primary_ko` | Korean companion of the primary face (Hangul syllables) | U+AC00-D7FF | launch |
| `content/fonts/ui_stylised.otf` | `font.ui.stylised` | Stylised display face (the second reference face) | U+0020-00FF, U+0410-044F, U+20A0-20AF | launch |
| `content/fonts/ui_compact.otf` | `font.ui.compact` | Compact face for dense panels (the third reference face) | U+0020-00FF, U+0410-044F, U+20A0-20AF | launch |

Each Latin face needs a regular weight at least; a bold weight is optional. Hinting and outlines must stay legible at the smallest HUD size (9 px at 1080p) and in the darkening text-outline pass.

## 18 String table

All player-facing text is new, written in plain functional English and keyed by Species string ids (`ui.lobby.ready`, `mode.flag_chain.objective`). No reference text is copied. Pass 2 collects every string id the documents use; the counts below size the work from the reference text categories.

| Category | String-id prefix | Reference entries |
|---|---|---:|
| mode messages, objectives and HUD lines | `mode.*` | 74 |
| team and faction names | `match.team.*` | 11 |
| front-end menus | `ui.menu.*` | 4 |
| main and pause menus | `ui.menu.*` | 31 |
| help and options screens | `ui.help.*` | 11 |
| leaderboard and statistics pages | `ui.leaderboard.*` | 26 |
| graphics options | `ui.options.*` | 14 |
| input options | `ui.options.*` | 7 |
| general options | `ui.options.*` | 11 |
| display options | `ui.options.*` | 13 |
| chat | `ui.chat.*` | 1 |
| controller-disconnected dialog | `ui.dialog.*` | 1 |
| file dialog | `ui.dialog.*` | 3 |
| message dialog | `ui.dialog.*` | 1 |
| window chrome | `ui.window.*` | 8 |
| text and number fields | `ui.input.*` | 5 |
| **Total** | | **221** |

| File | Asset id | Language | Priority | Placeholder |
|---|---|---|---|---|
| `content/strings/en_gb.json` | `strings.en_gb` | English (United Kingdom), the source language (`en-GB`) | skirmish-first-playable | pseudo-localised text |
| `content/strings/fr_fr.json` | `strings.fr_fr` | French (`fr-FR`) | launch | pseudo-localised text |
| `content/strings/de_de.json` | `strings.de_de` | German (`de-DE`) | launch | pseudo-localised text |
| `content/strings/it_it.json` | `strings.it_it` | Italian (`it-IT`) | launch | pseudo-localised text |
| `content/strings/es_es.json` | `strings.es_es` | Spanish (`es-ES`) | launch | pseudo-localised text |
| `content/strings/pt_br.json` | `strings.pt_br` | Portuguese (region to be confirmed) (`pt-BR`) | launch | pseudo-localised text |
| `content/strings/ru_ru.json` | `strings.ru_ru` | Russian (`ru-RU`) | launch | pseudo-localised text |
| `content/strings/ja_jp.json` | `strings.ja_jp` | Japanese (`ja-JP`) | launch | pseudo-localised text |
| `content/strings/ko_kr.json` | `strings.ko_kr` | Korean (`ko-KR`) | launch | pseudo-localised text |

The language set is the reference set of nine; the design phase confirms it. Japanese and Korean need the companion fonts of §17.

## 19 Maps and thumbnails

Maps are new layouts. One map slot exists per reference map, following the reference distribution per mode and player count, so every mode keeps its range of 2-, 3- and 4-player maps. Each map has one thumbnail, rendered automatically by the map tool. One 2-player and one 3-player Zone Control slot also host the two tutorials. Map formats: map definition (text, Species map schema) → cooked map definition; placeholder: generated test map. Thumbnail formats: PNG RGB8 rendered by the map tool → BC7 sRGB, no mips; 800x600 px (4:3, display size 400x300).

| Mode | Players | Map files | Thumbnail files | Priority |
|---|---|---|---|---|
| Spawn Control (`map.spawn_control`, `thumb.spawn_control`) | 2 | `content/maps/map_spawn_control_2p_{01..05}.json` (5 files) | `content/thumbnails/map_spawn_control_2p_{01..05}_thumb.png` (5 files) | skirmish-first-playable: 2<br>launch: 3 |
| Spawn Control (`map.spawn_control`, `thumb.spawn_control`) | 3 | `content/maps/map_spawn_control_3p_{01..02}.json` (2 files) | `content/thumbnails/map_spawn_control_3p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Spawn Control (`map.spawn_control`, `thumb.spawn_control`) | 4 | `content/maps/map_spawn_control_4p_{01..02}.json` (2 files) | `content/thumbnails/map_spawn_control_4p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Zone Control (`map.zone_control`, `thumb.zone_control`) | 2 | `content/maps/map_zone_control_2p_{01..02}.json` (2 files) | `content/thumbnails/map_zone_control_2p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Zone Control (`map.zone_control`, `thumb.zone_control`) | 3 | `content/maps/map_zone_control_3p_{01..04}.json` (4 files) | `content/thumbnails/map_zone_control_3p_{01..04}_thumb.png` (4 files) | launch: 4 |
| Zone Control (`map.zone_control`, `thumb.zone_control`) | 4 | `content/maps/map_zone_control_4p_{01..03}.json` (3 files) | `content/thumbnails/map_zone_control_4p_{01..03}_thumb.png` (3 files) | launch: 3 |
| Relic Run (`map.relic_run`, `thumb.relic_run`) | 2 | `content/maps/map_relic_run_2p_{01..04}.json` (4 files) | `content/thumbnails/map_relic_run_2p_{01..04}_thumb.png` (4 files) | launch: 4 |
| Relic Run (`map.relic_run`, `thumb.relic_run`) | 3 | `content/maps/map_relic_run_3p_{01..02}.json` (2 files) | `content/thumbnails/map_relic_run_3p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Relic Run (`map.relic_run`, `thumb.relic_run`) | 4 | `content/maps/map_relic_run_4p_{01..03}.json` (3 files) | `content/thumbnails/map_relic_run_4p_{01..03}_thumb.png` (3 files) | launch: 3 |
| Siege (`map.siege`, `thumb.siege`) | 2 | `content/maps/map_siege_2p_{01..04}.json` (4 files) | `content/thumbnails/map_siege_2p_{01..04}_thumb.png` (4 files) | launch: 4 |
| Siege (`map.siege`, `thumb.siege`) | 3 | `content/maps/map_siege_3p_{01..02}.json` (2 files) | `content/thumbnails/map_siege_3p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Siege (`map.siege`, `thumb.siege`) | 4 | `content/maps/map_siege_4p_{01..02}.json` (2 files) | `content/thumbnails/map_siege_4p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Launch Race (`map.launch_race`, `thumb.launch_race`) | 2 | `content/maps/map_launch_race_2p_{01..03}.json` (3 files) | `content/thumbnails/map_launch_race_2p_{01..03}_thumb.png` (3 files) | launch: 3 |
| Launch Race (`map.launch_race`, `thumb.launch_race`) | 3 | `content/maps/map_launch_race_3p_{01..03}.json` (3 files) | `content/thumbnails/map_launch_race_3p_{01..03}_thumb.png` (3 files) | launch: 3 |
| Launch Race (`map.launch_race`, `thumb.launch_race`) | 4 | `content/maps/map_launch_race_4p_01.json` | `content/thumbnails/map_launch_race_4p_01_thumb.png` | launch: 1 |
| Flag Chain (`map.flag_chain`, `thumb.flag_chain`) | 2 | `content/maps/map_flag_chain_2p_{01..04}.json` (4 files) | `content/thumbnails/map_flag_chain_2p_{01..04}_thumb.png` (4 files) | launch: 4 |
| Flag Chain (`map.flag_chain`, `thumb.flag_chain`) | 3 | `content/maps/map_flag_chain_3p_{01..02}.json` (2 files) | `content/thumbnails/map_flag_chain_3p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Flag Chain (`map.flag_chain`, `thumb.flag_chain`) | 4 | `content/maps/map_flag_chain_4p_01.json` | `content/thumbnails/map_flag_chain_4p_01_thumb.png` | launch: 1 |
| **Total** | | **49 maps** | **49 thumbnails** | |

## 20 Not to be created

These reference files and text entries need no Species counterpart. Counts only; nothing here is named.

| Class | Textures | Meshes | Audio files | Map files | Text entries | Total |
|---|---:|---:|---:|---:|---:|---:|
| bound only to sound events that never fire |  |  | 231 |  |  | 231 |
| unused (bound to no sound event) |  |  | 192 |  |  | 192 |
| editor only (map editor windows) |  |  |  |  | 164 | 164 |
| authoring source | 1 | 133 |  |  |  | 134 |
| campaign, single-player or unreachable code path | 19 | 4 |  | 29 | 14 | 66 |
| unused (never named) | 35 | 19 |  |  |  | 54 |
| debug only | 10 |  |  |  | 43 | 53 |
| dropped feature (campaign or dropped unit, structure or mode) |  | 42 |  |  |  | 42 |
| unused (never listed: superseded map files) |  |  |  | 30 |  | 30 |
| editor only (map editor match options) |  |  |  |  | 23 | 23 |
| unused (named only in dead code or data) | 14 | 2 |  |  | 3 | 19 |
| previous graphics back-end only | 17 |  |  |  |  | 17 |
| dropped feature (gesture input) | 9 |  |  |  | 2 | 11 |
| music and loader tracks never triggered and never packed |  |  | 11 |  |  | 11 |
| cut or test map | 1 |  |  | 8 |  | 9 |
| dropped feature (user profiles) |  |  |  |  | 9 | 9 |
| trial-version and store prompts |  |  |  |  | 8 | 8 |
| dropped feature (mods) |  |  |  |  | 8 | 8 |
| dropped feature (dropped mode) | 1 |  |  |  | 2 | 3 |
| deferred to the design phase (networking options) |  |  |  |  | 3 | 3 |
| dropped feature (dropped unit) | 2 |  |  |  |  | 2 |
| dropped feature (user profiles, mods) |  |  |  |  | 2 | 2 |
| dropped feature (dropped crate outcome and its structure) | 1 |  |  |  |  | 1 |
| store and branding art | 1 |  |  |  |  | 1 |
| dropped feature (dropped weapon) | 1 |  |  |  |  | 1 |
| dropped feature (disabled crate outcome) | 1 |  |  |  |  | 1 |
| dropped feature (dropped mode and unit) | 1 |  |  |  |  | 1 |
| unreachable fallback | 1 |  |  |  |  | 1 |
| editor only |  | 1 |  |  |  | 1 |
| **Total** | **115** | **201** | **434** | **67** | **281** | **1098** |

Bitmap font pages are not in this table: their role passes to the font files of §17.

