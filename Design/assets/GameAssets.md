# Species game assets

> Status: Asset list (consumers named by requirement id) · Related: [assets README](README.md) · [asset_manifest.csv](asset_manifest.csv) · [art_and_audio_briefs.md](art_and_audio_briefs.md) · [CONVENTIONS](../CONVENTIONS.md)

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

This document lists every asset Species must create so that the mechanics, look and feel of the reference library run with Species content. It covers what a skirmish match of the reference game reaches: units, creatures and hazards, structures and mode objects, Supply crates and powers, the HUD and the front end, help and explainer animations, sound effects, ambience, fonts, player-facing text and maps. It adds music, which the reference does not have. The persistent world adds further needs (world-scale terrain, settlements, a strategic zoom level): the rows that the MVP needs (O55) are in the manifest with the priority `mvp` (§2.5, [MVP assets](../implementation/mvp_assets.md)), and the design phase adds the rest.

**Every asset is newly authored.** Each file is made from scratch to the same technical role as its reference counterpart: size class, alpha mode, channel layout, loop behaviour, socket names and variant count. Nothing is traced, re-coloured, resampled or edited from the reference game's art, audio or text, and no reference file serves as a starting point, a paint-over base or a placeholder. Character silhouettes, textures, UI layout, logo and wordmark are new; the style stays the same (vision O9, [art_and_audio_briefs.md](art_and_audio_briefs.md)).

How to read the tables: one row per asset id. A row names its file, or an explicit numbered pattern with its count, for example `GameData/Audio/sfx/worker/worker_attack_{01..24}.wav` (24 files). [asset_manifest.csv](asset_manifest.csv) expands every row to one line per file, with the full format strings and its consumers: the requirement ids that use the asset, and the path of a document that cites it only outside its requirements.

## 2 Filename convention and formats

### 2.1 Content root and folders

All assets live under the content root `GameData/`. The content build rejects any file outside it.

**Amendment 2026-10-19 (owner decision O61).** The content root is `GameData/` at the repository root, in place of `content/`. Its top-level folders are written in PascalCase, as the owner named the first two, `Fonts/` and `Textures/`, and every texture sits directly in `GameData/Textures/`, as the owner's do; the folders below `Audio/`, `Meshes/` and `Records/` keep their lower-case names. Keys are lower-cased (AST-014, O61), so the case of a folder or a file name never reaches the game. The files that the owner has made keep the owner's names, and their rows name them; the other rows give proposed names, which the owner may change when making the file, with the row changed in the same change. A file that the owner keeps without a role is a parked file (§2.7).

**Amendment 2026-10-19 (owner decision O64).** Textures sit in one PascalCase folder per kind under `GameData/Textures/`, named from the asset id, the sprite cards among them in `Sprite/`; meshes and their collision assets sit in `GameData/Meshes/` itself, with no group folders. This replaces O61's single texture folder and the mesh group folders.

| Folder | Holds |
|---|---|
| `GameData/Textures/<Kind>/` | every texture, in one folder per kind (O64): `Cursor/` cursors and their shadows; `Fx/` VFX textures (glows, beams, flares, sky clouds, wire grid, canopy mask); `Glyph/` input glyphs; `Help/` hint pictures, help pages, mode cards and the explainer pictures; `Icon/Hud/` HUD icons; `Icon/Power/` power and Supply-crate outcome icons; `Marker/` markers, route arrows, banners and flags with their tint masks; `Sprite/` unit, creature and effect cards (camera-facing billboards); `Terrain/` terrain colour ramps, sea tiles and shore-wave gradients; `Uitex/` UI panels, title wordmark, logo emblem, front-end decoration, small UI icons and the application icon |
| `GameData/Ui/explainers/` | explainer animation scripts, one per mode, each found by its `mode` field, never by its member order (DAT-284) |
| `GameData/Meshes/` | flat-shaded meshes, CMO files in one folder (O64): units, creatures, structures, projectiles, relics, scenery and the map editor's meshes, and the collision assets, the hit geometry of every collidable mesh, each authored as its own source (AST-044) |
| `GameData/Audio/sfx/<source>/` | sound effects, one folder per sound source; `crate_outcome/` holds the outcome stings |
| `GameData/Audio/ui/` | interface sounds |
| `GameData/Audio/ambience/` | ambience beds |
| `GameData/Audio/music/` | music (new) |
| `GameData/Fonts/` | the glyph sheets and glyph tables of the faces drawn for Species (O56), and the font files of the Japanese and Korean companions for the DirectWrite upgrade of ADR-034 |
| `GameData/Strings/` | string tables, one per language, and the development string table (development editions only) |
| `GameData/Maps/` | map definitions, one per map slot |
| `GameData/Thumbnails/` | map thumbnails, one per map |
| `GameData/Records/modes/` | the match-option schema of each mode (`mode.options`) |
| `GameData/Records/input/` | the default binding set and the keyboard-layout layers (`input.binding_set`, `input.keyboard_layouts`) |
| `GameData/Records/actors/` | the actor stat table (`actor.stats`) |
| `GameData/Records/crates/` | the Supply-crate outcome catalogue (`crate.outcomes`) |
| `GameData/Records/audio/` | the sound sources with their events and layers (`sound.sources`), and the member weights of the sample groups (`sound.groups`, DAT-275) |
| `GameData/Records/vfx/` | the effect definitions (`effects.definition`) |
| `GameData/Records/timelines/` | the presentation timelines (`camera.timeline`) |
| `GameData/Records/ui/` | page layouts, palettes, text styles and the font-role map (`interface.page_layout`, `interface.palette`, `interface.text_style`, `interface.font_roles`) |
| `GameData/Records/editions/` | the edition manifests (`edition.manifest`) |
| `GameData/Records/test/` | the soak configurations, selected only into the development and soak editions (`test.soak_configuration`) |

The `GameData/Records/<family>/` folders hold the authored JSON data records of the record families of [data schemas](../architecture/components/data_schemas/README.md) table 1-A (DAT-006). They are content but not assets to create, so the manifest has no rows for them. A sample group's members are the members of its asset in the order of its `variants` column; a group whose members are not equally weighted lists its weights in `sound.groups` (DAT-275).

### 2.2 File names

- **The owner's names stand** (O61): a file the owner has made keeps its name, and its row names it; the rules below are for the names that the rows propose. Stems are unique within their folder, since keys must be.
- Textures and meshes are named in PascalCase by **role**, with no category prefix, because the kind folder carries it (O64): `Fx/BeamLong.dds`, `Icon/Power/Workers.dds`, `Help/HintCameraControls.dds`. Sprites and meshes start with the unit or structure name (`Sprite/WorkerFront.dds`, `LeaderFigure.cmo`). Every other file is lower-case snake_case, named by role. Audio files start with their source (`worker_die_01.wav`), UI sounds with `ui_`, ambience with `amb_`, outcome stings with `outcome_`.
- Numbered variants end in two digits, `NN` in a PascalCase name (`Ramp01`) and `_NN` in a snake_case one, counting from 01 without gaps. A row with a numbered pattern is one asset id with interchangeable variants, unless its spec gives each number a meaning (states, pages, themes).
- No stem repeats a reference file stem, and no name contains an IP word. Ids and names come from [CONVENTIONS §8](../CONVENTIONS.md#8-naming).
- String tables are named by their BCP 47 language tag in lower case with `_` for `-` (`en_gb.json`); the tag itself is stored in the file.
- Maps follow `map_<mode>_<players>p_<nn>`; a thumbnail adds `_thumb`.
- A **collision asset** (AST-044) is named after its render member with the suffix `Collision`, beside it in `GameData/Meshes/` (`TurretHeadCollision.cmo`); a **tint mask** (AST-022) is named after its parent member with `Tint`, which the owner's banner masks place before the state (`Marker/LeaderBannerTintIdle.dds`).
- A drawn face (§17) has one **glyph sheet**, named after the face, and a **glyph table** named after the sheet with the suffix `Glyphs`, or `_glyphs` for a snake_case name (`GameFont.dds` and `GameFontGlyphs.json`; `ui_compact.png` and `ui_compact_glyphs.json`), so that the two members have different keys (O63; DAT-219 amendment).
- A **derived variant** (AST-028) has no authored file: the cooker produces it offline from its parent's source. Its row names its cooked entry, `<parent file without extension>@<variant id>` (`GameData/Textures/Marker/CrateLanding@shadow`).

### 2.3 Asset ids

Asset ids are dotted and lower case. Numbered variants share one id; documents cite ids, never file names.

| Id root | Used for | Example |
|---|---|---|
| `sprite.` | sprite cards | `sprite.worker.front` |
| `fx.` | VFX textures | `fx.glow_round` |
| `uitex.` | UI panels, title art and the application icon | `uitex.title_wordmark` |
| `icon.power.`, `icon.hud.` | power and HUD icons | `icon.power.airstrike` |
| `cursor.`, `marker.` | cursors and markers | `marker.edge_compass` |
| `glyph.` | input glyphs | `glyph.pad.face_south` |
| `terrain.` | ramps, sea tiles, wave gradients | `terrain.ramp` |
| `mesh.<group>.` | meshes; the group `collision` holds the collision assets, `dev` the editor meshes | `mesh.structure.spawn_node`, `mesh.collision.spawn_node` |
| `help.`, `anim.explainer` | help pictures and explainer scripts | `help.hint.camera_controls` |
| `sfx.<kind>.<source>.<event>` | sound-effect sample groups; kind is `unit`, `creature`, `weapon`, `building` or `crate` | `sfx.unit.worker.attack` |
| `sfx.crate.outcome.` | Supply-crate outcome stings | `sfx.crate.outcome.shield` |
| `sfx.ui.` | UI sounds | `sfx.ui.match_ui.menu_select` |
| `amb.` | ambience beds | `amb.match_bed` |
| `music.` | music | `music.front_end.theme` |
| `font.` | fonts | `font.ui.primary` |
| `strings.` | string tables | `strings.en_gb` |
| `mapfile.`, `thumb.` | maps and thumbnails, one id per mode | `mapfile.spawn_control` |
| `<parent id>.<variant id>` | derived texture variants (AST-028): `shadow`, `trail` | `cursor.world.shadow` |
| `<parent id>_tint` | companion tint masks of team-tinted cloth (AST-022) | `marker.leader_banner_tint` |

### 2.4 Source and runtime formats

The artist delivers the source file under `GameData/`; the content build generates the runtime file in the pack. Every cooked texture is a DDS with the DX10 header. A block-compressed texture is a whole number of 4x4 blocks: its authored size (twice the display size for UI art) is a multiple of 4 in both dimensions, and the content build rejects any other size; the cooker never pads or resamples a source (ADR-017, AST-068). UI and HUD art is authored at twice its display size; the cooked top level keeps the authored size, never resampled, and the UI draws it at its display size (AST-024); world textures are authored at the stated size. Audio sources are 48 kHz. Samplers come from the closed set of ADR-018: linear clamp-to-edge (the default), linear wrap, point clamp-to-edge, and point-magnify clamp-to-edge (point under magnification, linear under minification and between mips), which the particle dot and the front-end Worker figure use. Icons, markers and cursors are straight-alpha art that their passes may also draw with the additive blend (RND-393, RND-423). Nothing is recoloured at load, and derived variants are produced offline by the cooker (ADR-018).

| Profile | Source (artist delivers) | Runtime (content build generates) | Colour space | Alpha mode |
|---|---|---|---|---|
| cut-out sprite | PNG RGBA8, sRGB, cut-out alpha painted by the artist | BC7_UNORM_SRGB, full mips | sRGB | cut-out: alpha test at the consumer threshold (0.04 Workers, 0.02 other units) |
| alpha sprite or picture | PNG RGBA8, sRGB, straight alpha | BC7_UNORM_SRGB, full mips (world) or no mips (UI) | sRGB | straight alpha blend |
| additive VFX | PNG 8-bit greyscale, intensity on black | BC4_UNORM, full mips | linear | additive: black adds nothing, colour from the draw tint |
| alpha mask | PNG 8-bit greyscale, white = opaque | BC4_UNORM, full mips | linear | mask as alpha over white, tinted at draw |
| darkening layer | PNG 8-bit greyscale, white = full darkening | BC4_UNORM, full mips | linear | destination multiplied by (1 - texel) |
| tinted icon | PNG RGBA8, sRGB, white or grey art | BC7_UNORM_SRGB, full mips | sRGB | straight alpha, tinted by vertex colour |
| team cloth | PNG RGBA8, sRGB, team area painted as grey levels (its intensity), no key colour | BC7_UNORM_SRGB, full mips; no per-team copy | sRGB | straight alpha; the shader tints the team area by the weight of its companion tint mask (AST-022) |
| tint mask | PNG 8-bit greyscale, white = full team tint, the size of its parent | BC4_UNORM, full mips | linear | team-tint weight read by the shader, never drawn on its own |
| derived variant | none: the cooker derives it offline from its parent's source (`BlurredShadow`, `StackedCopies`) | its family's runtime format; shadows BC4_UNORM, full mips | as its family | as its family; shadows are darkening layers |
| application icon | PNG RGBA8, sRGB, one file per icon size | Windows icon resource (.ico) built into the executable; not in the pack | sRGB | straight alpha |
| UI picture | PNG RGB8 or RGBA8, sRGB | BC7_UNORM_SRGB, no mips (tiled strips keep mips and repeat) | sRGB | opaque or straight alpha |
| CPU look-up table | PNG RGB8, sRGB | uncompressed R8G8B8A8_UNORM_SRGB, no mips, read on the CPU | sRGB | none |
| sea tile | PNG RGB8, sRGB, seamless | BC7_UNORM_SRGB, full mips, wrap repeat | sRGB | opaque, multiplied by the sea light map |
| thumbnail | PNG RGB8, sRGB, rendered by the map tool | BC7_UNORM_SRGB, no mips | sRGB | opaque |
| mesh | binary CMO of the Visual Studio mesh content pipeline (O64): right-handed, +Y up, counter-clockwise front faces, the colour per vertex, nodes and sockets as the bones of its skeleton | cooked Species mesh: face normals generated, colours attenuated once at cook (RND-160) | sRGB vertex colour | opaque; team regions marked by the material named `team` (AST-034, ADR-020) |
| collision mesh | binary CMO: triangles only, materials ignored, right-handed, +Y up; authored as its own source | cooked `Collision` entry, simulation-relevant (DAT-278) | n/a | n/a |
| sound effect | WAV, 48 kHz, 24-bit PCM, mono (2-D sounds may be stereo) | 16-bit PCM, 48 kHz, resident | n/a | n/a |
| long audio and music | WAV, 48 kHz, 24-bit PCM, stereo for music | MS-ADPCM stream at 48 kHz, decoded by XAudio2, with no codec in the executable (ADR-025, AST-080): every member of a group whose longest member is over 20 s (`aen.residency.stream_threshold`), and every music group (AEN-072) | n/a | n/a |
| drawn face | one glyph sheet on the sheet grid, a PNG of 8-bit greyscale coverage (white = full coverage) or a 32-bit DDS whose alpha channel holds the coverage, and a UTF-8 JSON glyph table (ADR-062) | one R8_UNORM coverage atlas page, never block-compressed, and the cooked glyph table, written by the font packer under `Tools/` (ADR-034 as amended by O56 and O63) | n/a | coverage |
| font (DirectWrite upgrade) | OpenType font licensed for embedding | font file in the pack; glyphs rasterised at run time into R8_UNORM cache pages | n/a | coverage |
| string table | UTF-8 JSON: string id to ICU MessageFormat text | cooked string table | n/a | n/a |
| map | map definition from the map tool (text, Species map schema) | cooked map definition | n/a | n/a |
| explainer script | JSON key-frame script that animates pictures by asset id | cooked explainer script | n/a | n/a |

**Amendment 2026-10-19 (owner decision O61).** A source may also be a DDS file that already has its row's runtime format, colour space, size and mip chain, as the owner's textures are: the content build validates its header and size against the row and packs it unchanged, without cooking it again (ADR-017 amendment). A DDS that differs from its row in any of the four is refused with each difference named, and is neither converted nor completed.

**Amendment 2026-10-19 (owner decision O64).** Every texture is authored as such a DDS, so every texture row proposes a `.dds`; the PNG forms of the Source column remain the art notes of each profile, and what the cooker accepts when a texture is delivered as a PNG instead, its row then naming the PNG. Meshes and collision meshes are CMO files in place of glTF 2.0 (ADR-019 amendment).

### 2.5 Priorities and the first-playable assumption

Each row has one priority: `mvp`, `skirmish-first-playable`, `launch` or `post-launch`, in that order of need.

- `mvp`: needed by the MVP's milestones M1 to M4 (O55); comes before `skirmish-first-playable`. The rows are chosen by MVP-080 to MVP-087, MVP-102 and MVP-103 of the [MVP scope](../implementation/mvp_scope.md), at the minimum that still meets the look of O52; [MVP assets](../implementation/mvp_assets.md) gives each one the milestone that first needs it and how it is produced.

**First-playable assumption** (vision O8: one skirmish mode first). The first playable is **Spawn Control** for 2 players on the map slots `map_spawn_control_2p_01` and `map_spawn_control_2p_02`, with bots, and with Supply crates drawing from a core outcome pool: Reinforcements, Strike team (with its grenade weapon), Transport, airstrike, gun turret, Shield, Speed Boost and Fury (DEC-229: the two first-playable maps exclude every other outcome). It needs:

- units: Worker (all cards), Leader with its banners, Trooper, Transport, Bomber, Essence orbs;
- structures: Spawn Node, Relay, Transit Gate, gun turret (mount, head, gun barrel), Barrier Emitter, wall, tree, jump pad, Supply crate, the port pad, and the collision assets of the collidable ones;
- UI: title wordmark, panels, menu highlight, check box, every cursor, the markers, the power-slot HUD for the core pool, the keyboard Tab glyph, the Spawn Control mode card, the route arrow, the primary font (Latin and Cyrillic), the English string table, the development string table and the application icon;
- world: sky clouds, terrain ramp 01, sea tile 01, shore-wave gradient 01, the VFX of those units;
- audio: every sample group of those sources and weapons, with the laser-bolt fire, structure-hit and kill groups and the grenade throw, the match interface sounds with the game-over announcement, and the match ambience bed.

Everything else that a match reaches is `launch`. `post-launch` holds the seasonal theme, alternative mesh bodies with their collision assets, the map editor's camera-view marker and its bot navigation node, the land colour ramp and the shore-wave gradient that no match map names (R-MAP-086), the front-end theme and the in-match music beds, which are authored only if DEC-049 selects option A (R-AUD-407), and the six Supply-crate outcome stings that play only if DEC-565 binds them (R-AUD-139). The objective cards have no picture assets: they draw nothing in a match (DEC-050). The design phase may move rows between priorities; the placeholder rule keeps the game building and running whatever is missing.

**Amendment 2026-10-19 (owner decision O55).** The world MVP replaces the skirmish first playable of O20 as the first playable of Species, and skirmish follows the MVP. `skirmish-first-playable` therefore means the first skirmish build after the MVP, and the assumption above describes what that build needs. Of its rows, the 87 files that the MVP needs as well now carry `mvp`, which comes first, and the others keep `skirmish-first-playable`; four `launch` rows that the MVP needs (the long beam strip and the three files of the body face, O56) carry `mvp` too. The MVP's new rows (the Wellspring, the dwelling, the storehouse and the resource sites with their collision assets, the Worker's carried-load overlay, the HUD icons and panels of MVP-102 and the three new sample groups of MVP-087) are `mvp` rows of their own; the owner confirmed the names dwelling and storehouse, and Crystal for the one resource (O57).

**Amendment 2026-10-19 (owner decision O56).** The caption face `font.ui.stylised` returns to `launch`: it is an optional later third face, and until it exists the Caption role uses the body face. The title and body faces are drawn bitmap faces of three files each (§17), so the MVP needs 91 files of the rows that the list already had: 87 from the skirmish first playable, the title face's three files among them, and 4 from `launch`, the long beam strip and the body face's three files. The primary font of the first-playable assumption is the drawn title face, which covers Latin only; Cyrillic waits for the DirectWrite upgrade of ADR-034.

**Amendment 2026-10-19 (owner decisions O62 and O63).** The owner's two faces replace the title and body faces above. The game face GameFont is `font.ui.primary`, now one sheet and its glyph table (2 files, `mvp`), and draws every role; the development face EditorFont is the new id `font.dev.editor` (2 files, `mvp`); `font.ui.compact` returns to `launch` beside `font.ui.stylised`, each an optional later face of two files. The MVP therefore needs 123 files in 90 asset ids: 87 files of rows that the list already had (86 from the skirmish first playable, the game face's two files among them, and 1 from `launch`, the long beam strip) and 36 files of new rows.

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
| generated block font | a block-letter face that a tool draws from a glyph table written in the tool's own source, newly authored for Species: its glyph sheet at the face's design size and its glyph table, with the face's coverage, so a missing face never blocks a build |
| fallback font | an open-licence font with the same script coverage (the companion font files of the DirectWrite upgrade only) |
| pseudo-localised text | text generated from each string id, padded to test layout |
| generated test map | flat map with the objects the mode needs for its player count |
| full-weight mask | tint mask of weight 1 over the opaque area of its parent art |
| derived by the cooker | the variant derived from its parent's stand-in or authored file |
| primitive collision | collision triangles at the bounding shape of its render member |

An edition that its edition manifest marks for release contains no placeholder: the content build fails such an edition and lists the rows that have no authored file (AST-085). Every other edition may contain placeholders.

### 2.7 Parked files

A **parked file** is a file that the owner keeps under the content root before it has a role (O61). It has no manifest row, the content build skips it, packs nothing of it and names it in its report, and an edition marked for release fails while one remains (AST-013, O61). When the owner gives it a role, it moves into the manifest as a row's file, or it is deleted; either way it leaves this list.

No file is parked: `GameData/Textures/Particle.dds`, parked on 2026-10-19 (O61), was removed by the owner on the same day (O64).

## 3 Summary

| Category | Asset ids | Files | mvp | first-playable | launch | post-launch | Reference files replaced |
|---|---:|---:|---:|---:|---:|---:|---:|
| [Unit sprites](#4-unit-sprites) | 14 | 17 | 7 | 0 | 9 | 1 | 15 |
| [VFX textures](#5-vfx-textures) | 19 | 19 | 9 | 5 | 5 | 0 | 18 |
| [UI panels and title](#6-ui-panels-and-title) | 13 | 20 | 5 | 7 | 8 | 0 | 14 |
| [HUD, supply-crate and power icons](#7-hud-supply-crate-and-power-icons) | 46 | 50 | 24 | 11 | 15 | 0 | 30 |
| [Cursors and markers](#8-cursors-and-markers) | 29 | 41 | 25 | 5 | 11 | 0 | 25 |
| [Input glyphs](#9-input-glyphs) | 19 | 19 | 0 | 1 | 18 | 0 | 13 |
| [Terrain ramps, water and waves](#10-terrain-ramps-water-and-waves) | 3 | 26 | 3 | 0 | 21 | 2 | 26 |
| [Meshes](#11-meshes) | 85 | 114 | 20 | 17 | 71 | 6 | 71 |
| [Help images and explainer animations](#12-help-images-and-explainer-animations) | 6 | 59 | 0 | 1 | 58 | 0 | 60 |
| [Audio by source and event](#13-audio-by-source-and-event) | 178 | 902 | 23 | 480 | 393 | 6 | 891 |
| [UI sounds](#14-ui-sounds) | 15 | 18 | 2 | 16 | 0 | 0 | 16 |
| [Ambience](#15-ambience) | 4 | 21 | 0 | 11 | 10 | 0 | 21 |
| [Music](#16-music) | 4 | 6 | 0 | 0 | 2 | 4 | 0 |
| [Fonts](#17-fonts) | 6 | 10 | 4 | 0 | 6 | 0 | 138 |
| [String table](#18-string-table) | 10 | 10 | 1 | 1 | 8 | 0 | 0 |
| [Maps and thumbnails](#19-maps-and-thumbnails) | 12 | 98 | 0 | 4 | 94 | 0 | 98 |
| **Total** | **463** | **1430** | **123** | **559** | **729** | **19** | **1436** |

"Reference files replaced" counts the in-scope reference files whose role each category takes over; one Species file may replace several (138 bitmap font pages become the glyph sheets and glyph tables of four drawn faces, two of them the owner's, and two companion font files, 10 files). The `mvp` column counts the files that the MVP needs (§2.5): 87 files of rows that the list already had (86 of them from the skirmish first playable, 1 from `launch`; amendments of O55, O56 and O62) and the MVP's 36 new rows, the development face's two files among them. Music, string tables, some help pictures and every row with no reference counterpart (the application icon, the MVP's new rows, the new glyphs, markers and the Abductor beam ring, the tint masks, the collision assets, the derived variants, the Trooper grenade throw and the six outcome stings that wait for DEC-565) have no one-to-one counterpart. "Files" counts manifest rows: the 7 derived-variant rows have no authored file, because the cooker produces them from their parents.

## 4 Unit sprites

Camera-facing cards for the crowd and for small creatures and effects. Worker cards are white or light grey so that the team colour comes from the vertex tint; the renderer scales them with camera distance and turns them into billboards far away. Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#3-unit-sprites).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `GameData/Textures/Sprite/WorkerFront.dds` | `sprite.worker.front` | Worker card, front view: the crowd figure; also drawn large by the loading view, the Launch Race status panel and the front-end backdrop | BC7 sRGB DDS with its mip chain, made by the owner and packed as authored (O64) | 128x128 px; white or light grey body for team tint; alpha-test threshold 0.04; magnified on the front-end backdrop through the point-magnify clamp-to-edge sampler (ADR-018) | single | mvp | generated figure |
| `GameData/Textures/Sprite/WorkerHurt{01..04}.dds` (4 files) | `sprite.worker.hurt` | Worker hurt pose; variants 01-03 are chosen by unit id while a Worker is airborne or dying, variant 04 is the fixed hurt pose | BC7 sRGB DDS with its mip chain, made by the owner and packed as authored (O64) | 128x128 px; same canvas and pivot as the front card; alpha-test threshold 0.04 | 4 | mvp | generated figure |
| `GameData/Textures/Sprite/WorkerSide.dds` | `sprite.worker.side` | Worker card seen edge-on (side view), used when the card turns away from the camera | BC7 sRGB DDS with its mip chain, made by the owner and packed as authored (O64) | 32x64 px; alpha-test threshold 0.04 | single | mvp | generated figure |
| `GameData/Textures/Sprite/WorkerCarryLoad.dds` | `sprite.worker.carry_load` | Carried-load overlay card drawn over a Worker while it carries a load of the gathered resource to a drop-off point (MVP-031, MVP-082) | BC7 sRGB DDS, mips (cut-out), packed as authored (O64) | 64x64 px; drawn over the Worker card on its pivot and baseline, scaled and turned with the card; white or light grey art tinted at draw; alpha-test threshold 0.04 | single | mvp | generated figure |
| `GameData/Textures/Sprite/WorkerStateOverlay.dds` | `sprite.worker.state_overlay` | Overlay card drawn over a Worker while it has a Convert link (R-UNIT-147) | BC7 sRGB DDS, mips (cut-out), packed as authored (O64) | 64x64 px; drawn over the Worker card | single | launch | generated figure |
| `GameData/Textures/Sprite/VisitorFront.dds` | `sprite.visitor.front` | Visitor faction figure card | BC7 sRGB DDS, mips (cut-out), packed as authored (O64) | 128x128 px; drawn as a Worker card: alpha-test threshold 0.04 (RND-060, RND-316) | single | launch | generated figure |
| `GameData/Textures/Sprite/WorkerSeasonalHat.dds` | `sprite.worker.seasonal_hat` | Seasonal-theme hat drawn 0.95 wu above every third Worker | BC7 sRGB DDS, mips (cut-out), packed as authored (O64) | 32x32 px; small accessory card; drawn only while the seasonal theme is on | single | post-launch | generated figure |
| `GameData/Textures/Sprite/CrawlerSegment.dds` | `sprite.crawler.segment` | Crawler body card; the renderer builds each Crawler's trail strip from it | BC4 DDS, mips (additive), packed as authored (O64) | 32x32 px; hostile (Rogue faction) Crawlers draw it as a darkening layer | single | launch | generated gradient |
| `GameData/Textures/Sprite/CrawlerSegment@trail` | `sprite.crawler.segment.trail` | Crawler trail texture sampled by the worm strip, a derived variant of `sprite.crawler.segment` produced offline by the cooker (`StackedCopies`, AST-029) | derived from `sprite.crawler.segment` → BC4, mips | 32x512 px column cleared to black; copies of the segment art stacked from the top, copy i (from 0) (32 - floor(i / 2)) px square: 18 copies, 504 px in all (RND-320) | single | launch | derived by the cooker |
| `GameData/Textures/Sprite/CrawlerEgg.dds` | `sprite.crawler_egg.body` | Crawler egg card | BC7 sRGB DDS, mips, packed as authored (O64) | 32x32 px | single | launch | generated figure |
| `GameData/Textures/Sprite/Wisp.dds` | `sprite.wisp.body` | Wisp card | BC4 DDS, mips (additive), packed as authored (O64) | 32x32 px | single | launch | generated gradient |
| `GameData/Textures/Sprite/BarrageMissile.dds` | `sprite.missile_barrage.missile` | Missile Barrage missile card (new presentation of the mechanic) | BC7 sRGB DDS, mips, packed as authored (O64) | 128x128 px | single | launch | generated figure |
| `GameData/Textures/Sprite/BarrageLauncher.dds` | `sprite.missile_barrage.launcher` | Missile Barrage launcher card that surfaces before the volley (new presentation) | BC7 sRGB DDS, mips, packed as authored (O64) | 128x128 px | single | launch | generated figure |
| `GameData/Textures/Sprite/BarrageLauncherShadow.dds` | `sprite.missile_barrage.launcher_shadow` | Shadow under the Missile Barrage launcher | BC4 DDS, mips (darkening), packed as authored (O64) | 128x128 px | single | launch | generated gradient |

## 5 VFX textures

Additive textures painted on black: overlapping layers saturate toward white and draw order does not matter. They are greyscale; colour comes from the draw tint (team colour, effect colour). Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#4-vfx-textures).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `GameData/Textures/Fx/BubbleSphere.dds` | `fx.bubble_sphere` | Translucent bubble around a falling Supply crate, a shielded Worker and a Collector | BC4 DDS, mips (additive), packed as authored (O64) | 128x128 px | single | skirmish-first-playable | generated gradient |
| `GameData/Textures/Fx/SkyCloud.dds` | `fx.sky_cloud` | Sky cloud layer (also the front-end background clouds) | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 16x16 px; read through its `level0` view (mip level 0 only, RND-153); the blobby layers wrap with the linear-wrap sampler, the flat layers wrap in the shader and sample through the point clamp-to-edge sampler (RND-154); small tile magnified on purpose | single | mvp | generated gradient |
| `GameData/Textures/Fx/GlowCloudy.dds` | `fx.glow_cloudy` | Soft cloudy glow: front-end fill boxes, Supply crate glow, dust vortex, Spawn Node glow | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 128x128 px; wrap repeat in front-end fill boxes | single | mvp | generated gradient |
| `GameData/Textures/Fx/FuelFlow.dds` | `fx.fuel_flow` | Scrolling fuel-flow strip on fuel lines | BC4 DDS, mips (additive), packed as authored (O64) | 128x128 px; wrap repeat, scrolled along the line | single | launch | generated gradient |
| `GameData/Textures/Fx/GlowRound.dds` | `fx.glow_round` | Round soft glow: Worker glow, blob shadows, lights, Essence orb glow, Core Device, Solar Array | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 128x128 px; also drawn as a darkening blob shadow | single | mvp | generated gradient |
| `GameData/Textures/Fx/BeamLong.dds` | `fx.beam_long` | Long beam strip: Launch Race status panel, Control Zone edge, Core Device pulse wave | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 128x32 px | single | mvp | generated gradient |
| `GameData/Textures/Fx/BeamShort.dds` | `fx.beam_short` | Beam strip: laser bolts, the sky grid lines and front-end decoration | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 128x32 px; the sky grid samples it with the linear clamp-to-edge sampler through all mip levels (RND-151); the laser-bolt pass reads its `level0` view (RND-339) | single | mvp | generated gradient |
| `GameData/Textures/Fx/BarrierField.dds` | `fx.barrier_field` | Barrier Emitter field and the base layer of the Beam Dish signal | BC4 DDS, mips (additive), packed as authored (O64) | 128x128 px; wrap repeat | single | skirmish-first-playable | generated gradient |
| `GameData/Textures/Fx/BarrierFieldWide.dds` | `fx.barrier_field_wide` | Wide Barrier Emitter field strip | BC4 DDS, mips (additive), packed as authored (O64) | 256x64 px; wrap repeat | single | skirmish-first-playable | generated gradient |
| `GameData/Textures/Fx/LightningStrip.dds` | `fx.lightning_strip` | Lightning-strike strip (map effect) | BC4 DDS, mips (additive), packed as authored (O64) | 64x256 px | single | launch | generated gradient |
| `GameData/Textures/Fx/MuzzleFlash.dds` | `fx.muzzle_flash` | Muzzle flash for turrets and units; also the Launch Race status panel | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 64x64 px | single | mvp | generated gradient |
| `GameData/Textures/Fx/ParticleDot.dds` | `fx.particle_dot` | Particle dot: particle pool, cursor formation preview, off-screen arrows, route lines | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 16x16 px; magnified with crisp pixels in the particle batches through the point-magnify clamp-to-edge sampler (ADR-018) | single | mvp | generated gradient |
| `GameData/Textures/Fx/DishSignal.dds` | `fx.dish_signal` | Beam Dish signal beam, combined with the barrier field texture | BC4 DDS, mips (additive), packed as authored (O64) | 128x128 px; wrap repeat; two-texture beam | single | launch | generated gradient |
| `GameData/Textures/Fx/ShardEdges.dds` | `fx.shard_edges` | Edge-line art reference for the shatter shards: not sampled at run time, because the shard edge lines come from a barycentric mask (VFX-229, ADR-010); it fixes the look the mask reproduces | BC7 sRGB DDS, mips (cut-out), packed as authored (O64) | 128x128 px; decal over the shard colour where alpha is set | single | skirmish-first-playable | generated figure |
| `GameData/Textures/Fx/Starburst.dds` | `fx.starburst` | Star flare on structure lights and manning ports | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 128x128 px | single | mvp | generated gradient |
| `GameData/Textures/Fx/ConvertBeam.dds` | `fx.convert_beam` | Convert beam strip | BC4 DDS, mips (additive), packed as authored (O64) | 128x32 px | single | launch | generated gradient |
| `GameData/Textures/Fx/TreeCanopy.dds` | `fx.tree_canopy` | Tree canopy mask, tinted per tree | BC4 DDS, mips (mask), packed as authored (O64) | 128x32 px | single | skirmish-first-playable | generated gradient |
| `GameData/Textures/Fx/TerrainGrid.dds` | `fx.terrain_grid` | Triangle wire-grid overlay drawn once per terrain cell over the terrain and the sea | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 128x128 px; wrap repeat, one repeat per terrain cell | single | mvp | generated gradient |
| `GameData/Textures/Fx/AbductorBeamRing.dds` | `fx.abductor_beam_ring` | Beam ring of the Abductor's tractor beam: stacked rings in the Visitor faction colour, scrolling up while Abducting and down while Dropping (R-UNIT-710); drawn with `actor_beam_alpha` | BC4 DDS, mips (mask), packed as authored (O64) | 128x32 px; one soft ring band across v; wrap repeat around the ring (u) | single | launch | generated gradient |

## 6 UI panels and title

Front-end and window art. The palette is new; it keeps the token roles of the reference (an accent panel, a neutral panel, a highlight). UI pictures keep exact pixels: no mip-maps, linear filtering. Authored sizes are multiples of 4 (ADR-017). Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#5-ui-panels-title-and-logo).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `GameData/Textures/Uitex/WorkerBadge.dds` | `uitex.worker_badge` | Worker badge on map-select buttons | BC7 sRGB DDS, no mips, packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | launch | generated panel |
| `GameData/Textures/Uitex/FillBlur.dds` | `uitex.fill_blur` | Blurred fill for front-end boxes | BC7 sRGB DDS, mips, repeat (tileable), packed as authored (O64) | 64x512 px display size; author at 2x (128x1024) | single | mvp | generated panel |
| `GameData/Textures/Uitex/PanelNeutral.dds` | `uitex.panel_neutral` | Neutral panel strip: power-slot panel, Core Device countdown, fuel depot read-out | BC7 sRGB DDS made by the owner, one level, repeat; packed as authored (O64) | 64x512 px display size; author at 2x (128x1024); one level, as authored (O64) | single | mvp | generated panel |
| `GameData/Textures/Uitex/PanelAccent.dds` | `uitex.panel_accent` | Accent panel strip for windows and the power-slot panel (new palette, same token role) | BC7 sRGB DDS made by the owner, one level, repeat; packed as authored (O64) | 64x510 px display size; author at 2x (128x1020); one level, as authored (O64) | single | mvp | generated panel |
| `GameData/Textures/Uitex/StudioSplash.dds` | `uitex.studio_splash` | Studio mark on the loading view (placeholder until the studio mark exists) | BC7 sRGB DDS, no mips, packed as authored (O64) | 600x600 px display size; author at 2x (1200x1200) | single | launch | generated panel |
| `GameData/Textures/Uitex/MenuHighlight.dds` | `uitex.menu_highlight` | Highlight beam behind the focused menu item | BC4 DDS made by the owner at the display size, one level, packed as authored (O61) | 256x64 px display size, drawn at 1x with one level (amended 2026-10-19, O61); wrap repeat | single | mvp | generated gradient |
| `GameData/Textures/Uitex/TitleWordmark.dds` | `uitex.title_wordmark` | Title wordmark (new logo) | BC7 sRGB DDS, no mips, packed as authored (O64) | 660x122 px display size; author at 2x (1320x244) | single | skirmish-first-playable | generated panel |
| `GameData/Textures/Uitex/LogoEmblem.dds` | `uitex.logo_emblem` | Logo emblem on the main menu, drop-down menus and the options screen | BC7 sRGB DDS, no mips, packed as authored (O64) | 278x194 px display size; author at 2x (556x388); wrap repeat where tiled | single | launch | generated panel |
| `GameData/Textures/Uitex/IconLocked.dds` | `uitex.icon_locked` | Lock icon on password-protected match entries | BC7 sRGB DDS, no mips, packed as authored (O64) | 20x20 px display size; author at 2x (40x40) | single | launch | generated panel |
| `GameData/Textures/Uitex/IconCheck.dds` | `uitex.icon_check` | Check-box tick | BC7 sRGB DDS, no mips, packed as authored (O64) | 50x50 px display size; author at 2x (100x100) | single | mvp | generated panel |
| `GameData/Textures/Uitex/ComplexitySimple.dds` | `uitex.difficulty_badge` | Mode complexity badge (simple, medium, hard) on the mode buttons | BC7 sRGB DDS, no mips, packed as authored (O64) | 64x32 px display size; author at 2x (128x64) | 1 of 3 | launch | generated panel |
| `GameData/Textures/Uitex/ComplexityMedium.dds` | `uitex.difficulty_badge` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 64x32 px display size; author at 2x (128x64) | 2 of 3 | launch | generated panel |
| `GameData/Textures/Uitex/ComplexityHard.dds` | `uitex.difficulty_badge` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 64x32 px display size; author at 2x (128x64) | 3 of 3 | launch | generated panel |
| `GameData/Textures/Uitex/BackdropWheel.dds` | `uitex.backdrop_wheel` | Large decorative wheel behind the front-end backdrop figure (new presentation) | BC7 sRGB DDS, no mips, packed as authored (O64) | 426x406 px display size; author at 2x (852x812) | single | launch | generated panel |
| `GameData/Textures/Uitex/AppIcon01.png` | `uitex.app_icon` | Application icon: the executable's icon resource, also the window icon (PLT-057); one member per icon size | PNG RGBA8 per size → icon resource in the executable | 16x16 px, icon size 1 of 6 (member order 16, 24, 32, 48, 64, 256 px); drawn for its own size | 1 of 6 | skirmish-first-playable | generated glyph |
| `GameData/Textures/Uitex/AppIcon02.png` | `uitex.app_icon` | ″ | PNG RGBA8 per size → icon resource in the executable | 24x24 px, icon size 2 of 6 (member order 16, 24, 32, 48, 64, 256 px); drawn for its own size | 2 of 6 | skirmish-first-playable | generated glyph |
| `GameData/Textures/Uitex/AppIcon03.png` | `uitex.app_icon` | ″ | PNG RGBA8 per size → icon resource in the executable | 32x32 px, icon size 3 of 6 (member order 16, 24, 32, 48, 64, 256 px); drawn for its own size | 3 of 6 | skirmish-first-playable | generated glyph |
| `GameData/Textures/Uitex/AppIcon04.png` | `uitex.app_icon` | ″ | PNG RGBA8 per size → icon resource in the executable | 48x48 px, icon size 4 of 6 (member order 16, 24, 32, 48, 64, 256 px); drawn for its own size | 4 of 6 | skirmish-first-playable | generated glyph |
| `GameData/Textures/Uitex/AppIcon05.png` | `uitex.app_icon` | ″ | PNG RGBA8 per size → icon resource in the executable | 64x64 px, icon size 5 of 6 (member order 16, 24, 32, 48, 64, 256 px); drawn for its own size | 5 of 6 | skirmish-first-playable | generated glyph |
| `GameData/Textures/Uitex/AppIcon06.png` | `uitex.app_icon` | ″ | PNG RGBA8 per size → icon resource in the executable | 256x256 px, icon size 6 of 6 (member order 16, 24, 32, 48, 64, 256 px); drawn for its own size | 6 of 6 | skirmish-first-playable | generated glyph |

## 7 HUD, supply-crate and power icons

Power and Supply-crate outcome icons, the power-slot panel read-outs, and the MVP's HUD read-outs, command icons and panels (MVP-102; the last 20 rows). Icons are white or grey art tinted at draw, drawn over a darkening halo; the same icon shows on the crate, in the power slot, on the cursor and on off-screen markers. Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#6-hud-supply-crate-and-power-icons).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `GameData/Textures/Icon/Power/Workers.dds` | `icon.power.workers` | Workers icon: selected-Workers cursor badge, power slot, Reinforcements outcome | BC4 DDS made by the owner at the display size, one level, packed as authored (O64) | 128x128 px display size, drawn at 1x with one level; a single-channel mask, tinted at draw (O64) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Power/Leader.dds` | `icon.power.leader` | Leader icon: selected-unit cursor badge and the promotion power | BC4 DDS made by the owner at the display size, one level, packed as authored (O64) | 128x128 px display size, drawn at 1x with one level; a single-channel mask, tinted at draw (O64) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Power/StrikeTeam.dds` | `icon.power.strike_team` | Strike team power | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Power/Grenade.dds` | `icon.power.grenade` | Strike team grenade weapon | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Power/Airstrike.dds` | `icon.power.airstrike` | airstrike outcome and power | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Power/Transport.dds` | `icon.power.transport` | Transport outcome and power | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Power/Builder.dds` | `icon.power.builder` | Builder outcome and power; also drawn over the Builder | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/Collector.dds` | `icon.power.collector` | Collector outcome and power | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/MissileBarrage.dds` | `icon.power.missile_barrage` | Missile Barrage outcome and power | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/Convert.dds` | `icon.power.convert` | Convert outcome and power | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/Shield.dds` | `icon.power.shield` | Shield outcome and power | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Power/SpeedBoost.dds` | `icon.power.speed_boost` | Speed Boost outcome and power | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Power/GunTurret.dds` | `icon.power.gun_turret` | gun turret outcome and power | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Power/Swarm.dds` | `icon.power.swarm` | Swarm outcome | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/EssenceGrove.dds` | `icon.power.essence_grove` | Essence Grove outcome | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/BlightGrove.dds` | `icon.power.blight_grove` | Blight Grove outcome | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/Hive.dds` | `icon.power.hive` | Hive outcome | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/Sickness.dds` | `icon.power.sickness` | Sickness outcome | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/Brood.dds` | `icon.power.brood` | Brood outcome | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/MeteorShower.dds` | `icon.power.meteor_shower` | meteor shower outcome | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/Fury.dds` | `icon.power.fury` | Fury outcome | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Power/AllegianceScrambler.dds` | `icon.power.allegiance_scrambler` | Allegiance Scrambler outcome (shown on the crate) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Power/SpitterPlants.dds` | `icon.power.spitter_plants` | Spitter Plants outcome (shown on the crate) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | launch | generated glyph |
| `GameData/Textures/Icon/Hud/SlotEmpty.dds` | `icon.hud.slot_empty` | Empty power slot | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Hud/IconHalo.dds` | `icon.hud.icon_halo` | Darkening halo behind HUD icons | BC4 DDS made by the owner at the display size, one level, packed as authored (O61) | 128x128 px display size, drawn at 1x with one level (amended 2026-10-19, O61) | single | mvp | generated gradient |
| `GameData/Textures/Icon/Hud/TransportBoarding.dds` | `icon.hud.transport_state` | Transport state read-out on its power slot: boarding, travelling, unloading | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | 1 of 3 | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Hud/TransportTravelling.dds` | `icon.hud.transport_state` | ″ | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | 2 of 3 | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Hud/TransportUnloading.dds` | `icon.hud.transport_state` | ″ | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | 3 of 3 | skirmish-first-playable | generated glyph |
| `GameData/Textures/Icon/Hud/LaunchPanel.dds` | `icon.hud.launch_panel` | Launch Race status panel frame | BC7 sRGB DDS, no mips, packed as authored (O64) | 200x300 px display size; author at 2x (400x600) | single | launch | generated panel |
| `GameData/Textures/Icon/Hud/LaunchPanelDamaged.dds` | `icon.hud.launch_panel_damaged` | Launch Race status panel overlay for a damaged Launch Vessel | BC7 sRGB DDS, no mips, packed as authored (O64) | 250x300 px display size; author at 2x (500x600) | single | launch | generated panel |
| `GameData/Textures/Icon/Hud/Resource.dds` | `icon.hud.resource` | Crystal icon (the gathered resource, O57): the stores and their capacity, structure costs on the construction bar and the stores moved in a battle result (MVP-030, MVP-034, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Growth.dds` | `icon.hud.growth` | Growth read-out: an Essence orb becoming a Worker, drawn dimmed beside its cause while growth stops (MVP-024, MVP-035, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/PriorityGather.dds` | `icon.hud.priority` | Settlement priority read-out and control, in member order: gathering, construction, guard duty (MVP-036, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | 1 of 3 | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/PriorityConstruct.dds` | `icon.hud.priority` | ″ | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | 2 of 3 | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/PriorityGuard.dds` | `icon.hud.priority` | ″ | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | 3 of 3 | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Dwelling.dds` | `icon.hud.dwelling` | Dwelling on the construction bar, in the construction queue and in its completion notification (MVP-034, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Storehouse.dds` | `icon.hud.storehouse` | Storehouse on the construction bar, in the construction queue and in its completion notification (MVP-034, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Contact.dds` | `icon.hud.contact` | Contact: the contact notification and the territory-pressure cause of a grievance (MVP-027, MVP-051, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Claim.dds` | `icon.hud.claim` | Claim: the make-claim command, claim notifications, the claim cause of a grievance and the world marker of a claimed area (MVP-051, MVP-053, MVP-084, MVP-101) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Attack.dds` | `icon.hud.attack` | Attack: the declare-attack command with its cost, the attack warning with its countdown, the attack cause of a grievance and the world marker of an incoming expedition (MVP-042, MVP-051, MVP-101, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Battle.dds` | `icon.hud.battle` | Battle: the battle start and end notifications and the battle result (MVP-043, MVP-045, MVP-048, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Tribute.dds` | `icon.hud.tribute` | Tribute: the send-tribute command and an eased grievance in the relations panel (MVP-054, MVP-101, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Withdraw.dds` | `icon.hud.withdraw` | Withdraw: the withdraw-expedition and withdraw-claim commands (MVP-045, MVP-053, MVP-101) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Respite.dds` | `icon.hud.respite` | Respite: a settlement under a respite in the relations panel, where it also disables the declare-attack command (MVP-047, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Renown.dds` | `icon.hud.renown` | Renown: the renown standing, its sources and the final standing of the epilogue (MVP-061 to MVP-063, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/RunClock.dds` | `icon.hud.run_clock` | Run clock: the time left in the run and its quiet period before the end (MVP-060, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Deputy.dds` | `icon.hud.deputy` | Deputy: each deputy action in the away report, beside the standing order that permitted it (MVP-073, MVP-074, MVP-102) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Icon/Hud/Meter.dds` | `icon.hud.meter` | Meter strip: the track and the fill of every HUD meter: construction progress, the priority shares, a Cladan's anger, the attack warning's countdown and the renown standing (MVP-033, MVP-036, MVP-050, MVP-062, MVP-102) | BC7 sRGB DDS, mips, repeat (tileable), packed as authored (O64) | 256x16 px display size; author at 2x (512x32); tiles along its long axis; the fill is the same strip, tinted and clipped to its fraction | single | mvp | generated panel |
| `GameData/Textures/Icon/Hud/NotificationCard.dds` | `icon.hud.notification_card` | Notification card: the frame behind each notification line: contact, claims, the attack warning with its countdown, battle start and end, and structures completed (MVP-102) | BC7 sRGB DDS, no mips, packed as authored (O64) | 320x48 px display size; author at 2x (640x96) | single | mvp | generated panel |
| `GameData/Textures/Icon/Hud/ResultCard.dds` | `icon.hud.result_card` | Battle-result card: the frame of a battle result: the sides, the Workers lost, the stores and Essence moved, the claimed area, the grievance, the respite and the renown (MVP-048, MVP-102) | BC7 sRGB DDS, no mips, packed as authored (O64) | 400x300 px display size; author at 2x (800x600) | single | mvp | generated panel |

## 8 Cursors and markers

World and menu cursors, off-screen and ground markers, Leader banners and Flag Chain flags. The owner authors the darkening shadows of the shadowed cursors (O64), and the cooker derives those of the crate landing and barrage markers offline, as derived variants; banners and flags carry the team area as grey levels, with a companion tint mask each. Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#7-cursors-and-markers).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `GameData/Textures/Cursor/World.dds` | `cursor.world` | Default world cursor | BC4 DDS made by the owner at the display size, one level, packed as authored (O64) | 128x128 px display size, drawn at 1x with one level; a single-channel mask, tinted at draw (O64); its darkening shadow `cursor.world.shadow` is authored with it (O64) | single | mvp | generated glyph |
| `GameData/Textures/Cursor/Highlight.dds` | `cursor.highlight` | Cursor over a selectable unit; also the power-slot screen-zone highlight | BC4 DDS made by the owner at the display size, one level, packed as authored (O64) | 128x128 px display size, drawn at 1x with one level; a single-channel mask, tinted at draw (O64) | single | mvp | generated glyph |
| `GameData/Textures/Cursor/Place.dds` | `cursor.place` | Placement cursor for powers that place a structure; also the entering marker drawn at the entrance of a Transport or a Beam Dish (R-CTRL-371) | BC4 DDS at the display size, one level, as the owner made the other cursors; packed as authored (O64) | 128x128 px display size, drawn at 1x with one level; a single-channel mask, tinted at draw (O64) | single | mvp | generated glyph |
| `GameData/Textures/Cursor/Unavailable.dds` | `cursor.unavailable` | Unavailable cursor; also the kick-player button and invalid power targets | BC4 DDS made by the owner at the display size, one level, packed as authored (O64) | 128x128 px display size, drawn at 1x with one level; a single-channel mask, tinted at draw (O64) | single | mvp | generated glyph |
| `GameData/Textures/Cursor/TurretAim.dds` | `cursor.turret_aim` | Turret aiming cursor | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Cursor/Pointer.dds` | `cursor.pointer` | Pointer-arrow image of the scroll-bar arrows and the selection arrows; the world cursor `cursor.world` draws the pointer itself (UI-465) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | mvp | generated glyph |
| `GameData/Textures/Marker/EdgeCompass.dds` | `marker.edge_compass` | Off-screen compass marker and power-slot compass | BC4 DDS made by the owner at the display size, one level, packed as authored (O64) | 256x256 px display size, drawn at 1x with one level; a single-channel mask, tinted at draw (O64) | single | mvp | generated glyph |
| `GameData/Textures/Marker/EdgeCompassEmpty.dds` | `marker.edge_compass_empty` | Empty compass: the compass ring that the compass layer of an on-screen world marker draws instead of the arrow (RND-357) | BC4 DDS made by the owner at the display size, one level, packed as authored (O64) | 256x256 px display size, drawn at 1x with one level; a single-channel mask, tinted at draw (O64); the compass ring with no arrow | single | mvp | generated glyph |
| `GameData/Textures/Marker/EdgeCompassShadow.dds` | `marker.edge_compass_shadow` | Shadow for the off-screen compass marker | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 256x256 px | single | mvp | generated gradient |
| `GameData/Textures/Marker/TeamDisc.dds` | `marker.team_disc` | Team-coloured disc under off-screen markers and power slots | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 128x128 px | single | mvp | generated gradient |
| `GameData/Textures/Marker/TeamDiscShadow.dds` | `marker.team_disc_shadow` | Shadow for the team disc | BC4 DDS with its mip chain, made by the owner and packed as authored (O61) | 128x128 px | single | mvp | generated gradient |
| `GameData/Textures/Marker/SupplyCrate.dds` | `marker.supply_crate` | Off-screen marker and recent-power badge for a Supply crate | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Marker/Relic.dds` | `marker.relic` | Off-screen marker for a Relic | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px | single | launch | generated glyph |
| `GameData/Textures/Marker/CrateLanding.dds` | `marker.crate_landing` | Ground marker where a Supply crate will land | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Marker/BarrageTarget.dds` | `marker.barrage_target` | Ground symbol under a Missile Barrage target (new presentation) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px | single | launch | generated glyph |
| `GameData/Textures/Marker/FlagPointer.dds` | `marker.flag_pointer` | Pointer drawn on a cloth flag | BC4 DDS with its mip chain, made by the owner and packed as authored (O64) | 64x64 px; a single-channel mask, tinted at draw (O64) | single | mvp | generated figure |
| `GameData/Textures/Marker/JumpPad.dds` | `marker.jump_pad` | Direction arrow on a jump pad | BC7 sRGB DDS, mips, packed as authored (O64) | 64x64 px; wrap repeat, scrolled | single | skirmish-first-playable | generated figure |
| `GameData/Textures/Marker/OrderTrail.dds` | `marker.order_trail` | Order trail mark left by a Leader's order | BC4 DDS with its mip chain, made by the owner and packed as authored (O64) | 64x64 px; a single-channel mask, tinted at draw (O64) | single | mvp | generated figure |
| `GameData/Textures/Marker/LeaderBannerIdle.dds` | `marker.leader_banner` | Leader banner per order state: idle, follow, formation, gather (no in-scope rule draws gather; it keeps the state order complete) | BC7 sRGB DDS with its mip chain, made by the owner and packed as authored (O64); tint mask | 64x64 px; cloth texture; team area as grey levels, with its companion tint mask `marker.leader_banner_tint` | 1 of 4 | mvp | generated glyph |
| `GameData/Textures/Marker/LeaderBannerFollow.dds` | `marker.leader_banner` | ″ | BC7 sRGB DDS with its mip chain, made by the owner and packed as authored (O64); tint mask | 64x64 px; cloth texture; team area as grey levels, with its companion tint mask `marker.leader_banner_tint` | 2 of 4 | mvp | generated glyph |
| `GameData/Textures/Marker/LeaderBannerFormation.dds` | `marker.leader_banner` | ″ | BC7 sRGB DDS with its mip chain, made by the owner and packed as authored (O64); tint mask | 64x64 px; cloth texture; team area as grey levels, with its companion tint mask `marker.leader_banner_tint` | 3 of 4 | mvp | generated glyph |
| `GameData/Textures/Marker/LeaderBannerGather.dds` | `marker.leader_banner` | ″ | BC7 sRGB DDS with its mip chain, made by the owner and packed as authored (O64); tint mask | 64x64 px; cloth texture; team area as grey levels, with its companion tint mask `marker.leader_banner_tint` | 4 of 4 | mvp | generated glyph |
| `GameData/Textures/Marker/LeaderBannerTintIdle.dds` | `marker.leader_banner_tint` | Companion tint mask of `marker.leader_banner`, one per member in the parent's order: the team-tint weight of the cloth, 1 where the team colour applies fully (AST-022, ADR-020) | BC4 DDS with its mip chain, made by the owner and packed as authored (O64) | 64x64 px, the size of its parent member | 1 of 4 | mvp | full-weight mask |
| `GameData/Textures/Marker/LeaderBannerTintFollow.dds` | `marker.leader_banner_tint` | ″ | BC4 DDS with its mip chain, made by the owner and packed as authored (O64) | 64x64 px, the size of its parent member | 2 of 4 | mvp | full-weight mask |
| `GameData/Textures/Marker/LeaderBannerTintFormation.dds` | `marker.leader_banner_tint` | ″ | BC4 DDS with its mip chain, made by the owner and packed as authored (O64) | 64x64 px, the size of its parent member | 3 of 4 | mvp | full-weight mask |
| `GameData/Textures/Marker/LeaderBannerTintGather.dds` | `marker.leader_banner_tint` | ″ | BC4 DDS with its mip chain, made by the owner and packed as authored (O64) | 64x64 px, the size of its parent member | 4 of 4 | mvp | full-weight mask |
| `GameData/Textures/Marker/FlagPostNeutral.dds` | `marker.flag_post_state` | Flag Chain flag per state: neutral, lowered, locked, raised | BC7 sRGB DDS, mips; tint mask (team area grey), packed as authored (O64) | 64x64 px; cloth texture; team area as grey levels, with its companion tint mask `marker.flag_post_state_tint` | 1 of 4 | launch | generated glyph |
| `GameData/Textures/Marker/FlagPostLowered.dds` | `marker.flag_post_state` | ″ | BC7 sRGB DDS, mips; tint mask (team area grey), packed as authored (O64) | 64x64 px; cloth texture; team area as grey levels, with its companion tint mask `marker.flag_post_state_tint` | 2 of 4 | launch | generated glyph |
| `GameData/Textures/Marker/FlagPostLocked.dds` | `marker.flag_post_state` | ″ | BC7 sRGB DDS, mips; tint mask (team area grey), packed as authored (O64) | 64x64 px; cloth texture; team area as grey levels, with its companion tint mask `marker.flag_post_state_tint` | 3 of 4 | launch | generated glyph |
| `GameData/Textures/Marker/FlagPostRaised.dds` | `marker.flag_post_state` | ″ | BC7 sRGB DDS, mips; tint mask (team area grey), packed as authored (O64) | 64x64 px; cloth texture; team area as grey levels, with its companion tint mask `marker.flag_post_state_tint` | 4 of 4 | launch | generated glyph |
| `GameData/Textures/Marker/FlagPostNeutralTint.dds` | `marker.flag_post_state_tint` | Companion tint mask of `marker.flag_post_state`, one per member in the parent's order: the team-tint weight of the cloth, 1 where the team colour applies fully (AST-022, ADR-020) | BC4 DDS, mips (mask), packed as authored (O64) | 64x64 px, the size of its parent member | 1 of 4 | launch | full-weight mask |
| `GameData/Textures/Marker/FlagPostLoweredTint.dds` | `marker.flag_post_state_tint` | ″ | BC4 DDS, mips (mask), packed as authored (O64) | 64x64 px, the size of its parent member | 2 of 4 | launch | full-weight mask |
| `GameData/Textures/Marker/FlagPostLockedTint.dds` | `marker.flag_post_state_tint` | ″ | BC4 DDS, mips (mask), packed as authored (O64) | 64x64 px, the size of its parent member | 3 of 4 | launch | full-weight mask |
| `GameData/Textures/Marker/FlagPostRaisedTint.dds` | `marker.flag_post_state_tint` | ″ | BC4 DDS, mips (mask), packed as authored (O64) | 64x64 px, the size of its parent member | 4 of 4 | launch | full-weight mask |
| `GameData/Textures/Marker/RouteArrow.dds` | `marker.route_arrow` | Arrow head drawn along each leg of a route line, pointing in the route's direction, in the colour of the route's spots (RND-353) | BC4 DDS with its mip chain, made by the owner and packed as authored (O64) | 32x32 px; a single-channel mask, tinted at draw (O64); white arrow head pointing along +u | single | mvp | generated glyph |
| `GameData/Textures/Cursor/WorldShadow.dds` | `cursor.world.shadow` | Darkening shadow of `cursor.world`, drawn under it: authored darkening art, not a derived variant (O64, RND-422) | BC4 DDS made by the owner at the display size, one level, packed as authored (O64) | 128x128 px display size, drawn at 1x with one level, the size of its parent; darkening art authored by the owner, not derived by the cooker (O64, RND-422) | single | mvp | derived by the cooker |
| `GameData/Textures/Cursor/PlaceShadow.dds` | `cursor.place.shadow` | Darkening shadow of `cursor.place`, drawn under it: authored darkening art, not a derived variant (O64, RND-422) | BC4 DDS at the display size, one level, as the owner made the other cursor shadows; packed as authored (O64) | 128x128 px display size, drawn at 1x with one level, the size of its parent; darkening art authored by the owner, not derived by the cooker (O64, RND-422) | single | mvp | derived by the cooker |
| `GameData/Textures/Cursor/UnavailableShadow.dds` | `cursor.unavailable.shadow` | Darkening shadow of `cursor.unavailable`, drawn under it: authored darkening art, not a derived variant (O64, RND-422) | BC4 DDS made by the owner at the display size, one level, packed as authored (O64) | 128x128 px display size, drawn at 1x with one level, the size of its parent; darkening art authored by the owner, not derived by the cooker (O64, RND-422) | single | mvp | derived by the cooker |
| `GameData/Textures/Cursor/HighlightShadow.dds` | `cursor.highlight.shadow` | Darkening shadow of `cursor.highlight`, drawn under it: authored darkening art, not a derived variant (O64, RND-422) | BC4 DDS made by the owner at the display size, one level, packed as authored (O64) | 128x128 px display size, drawn at 1x with one level, the size of its parent; darkening art authored by the owner, not derived by the cooker (O64, RND-422) | single | mvp | derived by the cooker |
| `GameData/Textures/Marker/CrateLanding@shadow` | `marker.crate_landing.shadow` | Blurred darkening shadow of `marker.crate_landing`, a derived variant produced offline by the cooker (`BlurredShadow`, AST-029, RND-243) | derived from `marker.crate_landing` → BC4, mips | 128x128 px, the size of its parent; separable five-tap blur (2, 4, 7, 4, 2) scaled by `rnd.shadow.icon_blur_radius` (RND-422) | single | skirmish-first-playable | derived by the cooker |
| `GameData/Textures/Marker/BarrageTarget@shadow` | `marker.barrage_target.shadow` | Blurred darkening shadow of `marker.barrage_target`, a derived variant produced offline by the cooker (`BlurredShadow`, AST-029, RND-243) | derived from `marker.barrage_target` → BC4, mips | 128x128 px, the size of its parent; separable five-tap blur (2, 4, 7, 4, 2) scaled by `rnd.shadow.icon_blur_radius` (RND-422) | single | launch | derived by the cooker |

## 9 Input glyphs

Platform-neutral input glyphs: shapes and labels describe the position of a control, never a console brand or trade dress. Brief: [art_and_audio_briefs.md](art_and_audio_briefs.md#8-input-glyphs).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `GameData/Textures/Glyph/PadFaceSouth.dds` | `glyph.pad.face_south` | Gamepad face button, south position | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128); platform-neutral shape and label; no console trade dress | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadFaceEast.dds` | `glyph.pad.face_east` | Gamepad face button, east position | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadFaceWest.dds` | `glyph.pad.face_west` | Gamepad face button, west position | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadFaceNorth.dds` | `glyph.pad.face_north` | Gamepad face button, north position | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadFaceShadow.dds` | `glyph.pad.face_shadow` | Darkening shadow under face-button glyphs | BC4 DDS, mips (darkening), packed as authored (O64) | 50x50 px display size; author at 2x (100x100) | single | launch | generated gradient |
| `GameData/Textures/Glyph/PadShoulderLeft.dds` | `glyph.pad.shoulder_left` | Gamepad left shoulder button (also on the power-slot panel) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 50x50 px display size; author at 2x (100x100) | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadShoulderLeftShadow.dds` | `glyph.pad.shoulder_left_shadow` | Darkening shadow under the left shoulder glyph | BC4 DDS, mips (darkening), packed as authored (O64) | 50x50 px display size; author at 2x (100x100) | single | launch | generated gradient |
| `GameData/Textures/Glyph/PadShoulderRight.dds` | `glyph.pad.shoulder_right` | Gamepad right shoulder button | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 50x50 px display size; author at 2x (100x100) | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadShoulderRightShadow.dds` | `glyph.pad.shoulder_right_shadow` | Darkening shadow under the right shoulder glyph | BC4 DDS, mips (darkening), packed as authored (O64) | 50x50 px display size; author at 2x (100x100) | single | launch | generated gradient |
| `GameData/Textures/Glyph/PadTriggerLeft.dds` | `glyph.pad.trigger_left` | Gamepad left trigger | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 50x50 px display size; author at 2x (100x100) | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadTriggerRight.dds` | `glyph.pad.trigger_right` | Gamepad right trigger | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 50x50 px display size; author at 2x (100x100) | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadTriggerShadow.dds` | `glyph.pad.trigger_shadow` | Darkening shadow under trigger glyphs | BC4 DDS, mips (darkening), packed as authored (O64) | 50x50 px display size; author at 2x (100x100) | single | launch | generated gradient |
| `GameData/Textures/Glyph/KeyTab.dds` | `glyph.key.tab` | Keyboard Tab key on the power-slot panel | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 128x128 px display size; author at 2x (256x256) | single | skirmish-first-playable | generated glyph |
| `GameData/Textures/Glyph/PadStickLeft.dds` | `glyph.pad.stick_left` | Gamepad left stick glyph (INP-081) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128); platform-neutral shape and label; no console trade dress | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadStickLeftShadow.dds` | `glyph.pad.stick_left_shadow` | Darkening shadow under the left stick glyph | BC4 DDS, mips (darkening), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | launch | generated gradient |
| `GameData/Textures/Glyph/PadStickRight.dds` | `glyph.pad.stick_right` | Gamepad right stick glyph (INP-081) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128); platform-neutral shape and label; no console trade dress | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadStickRightShadow.dds` | `glyph.pad.stick_right_shadow` | Darkening shadow under the right stick glyph | BC4 DDS, mips (darkening), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | launch | generated gradient |
| `GameData/Textures/Glyph/PadDpadHorizontal.dds` | `glyph.pad.dpad_horizontal` | Gamepad d-pad, left and right glyph (INP-081) | BC7 sRGB DDS, mips (white art), packed as authored (O64) | 64x64 px display size; author at 2x (128x128); platform-neutral shape and label; no console trade dress | single | launch | generated glyph |
| `GameData/Textures/Glyph/PadDpadHorizontalShadow.dds` | `glyph.pad.dpad_horizontal_shadow` | Darkening shadow under the d-pad, left and right glyph | BC4 DDS, mips (darkening), packed as authored (O64) | 64x64 px display size; author at 2x (128x128) | single | launch | generated gradient |

## 10 Terrain ramps, water and waves

Ramps and wave gradients are CPU look-up tables, not GPU textures: the terrain builder reads a ramp by slope and height to colour each terrain triangle, and the sea renderer reads one wave-gradient row. Sea tiles are GPU textures, repeated and multiplied by the sea light map. Each map names one ramp, one sea tile and one wave gradient; number 01 of each is the map default. The seasonal theme forces ramp 06, sea tile 02 and wave gradient 04. No match map names ramp 14 or wave gradient 08; the map editor lists them with the others, so they are post-launch (R-MAP-086, R-MAP-325). Theme numbers carry no fixed biome; the environment brief assigns one look per number.

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `GameData/Textures/Terrain/Ramp01.dds`, `GameData/Textures/Terrain/Ramp{02..14}.dds` (14 files) | `terrain.ramp` | Terrain colour ramp: colour by slope (x) and height (y), read on the CPU when the terrain mesh is built | file 01: RGBA8 sRGB DDS made by the owner, packed as authored (O61); 02 to 14: RGBA8 sRGB DDS table; read on the CPU; packed as authored (O64) | 64x64 px; x = slope (column 0 flat ground, column 63 vertical); y = height, the look-up row counted from the bottom row of the image as viewed: the bottom row is the highest terrain, the top row sea level and below (TER-040); the content build rejects another size (TER-250) | 14 | mvp: 1 (file 01)<br>launch: 12<br>post-launch: 1 (file 14) | generated ramp |
| `GameData/Textures/Terrain/SeaTile01.dds`, `GameData/Textures/Terrain/SeaTile{02..04}.dds` (4 files) | `terrain.sea_tile` | Sea surface tile (repeated, multiplied by the sea light map) | file 01: BC7 sRGB DDS with its mip chain, made by the owner and packed as authored (O61); 02 to 04: BC7 sRGB DDS, mips, repeat (seamless); packed as authored (O64) | 128x128 px; wrap repeat | 4 | mvp: 1 (file 01)<br>launch: 3 | generated noise tile |
| `GameData/Textures/Terrain/ShoreWaveGradient01.dds`, `GameData/Textures/Terrain/ShoreWaveGradient{02..08}.dds` (8 files) | `terrain.shore_wave_gradient` | Shore-wave colour gradient: one row read on the CPU by the sea renderer | file 01: RGBA8 sRGB DDS made by the owner, packed as authored (O61); 02 to 08: RGBA8 sRGB DDS table; read on the CPU; packed as authored (O64) | 400x10 px; the wave colours are pixel row 1 counted from the bottom row of the image as viewed, one colour per column (TER-041) | 8 | mvp: 1 (file 01)<br>launch: 6<br>post-launch: 1 (file 08) | generated ramp |

## 11 Meshes

Flat-shaded low-poly meshes with per-face colours. Parts that take the team colour use the material named `team` on the root node and its direct children (AST-034, ADR-020). The renderer tints them in the shader and nothing is recoloured at load; an authoring helper may propose weights for art made in the "mostly red" key-colour style (red more than twice green and twice blue), which the artist reviews. Code fetches sockets by name, so every socket of a row must exist with exactly that name, as a bone with no mesh in the `.cmo` (O64). Triangle budgets are upper limits, each one of the budget classes 100, 300, 1000, 3000, 10000 and 25000 triangles. Formats: `.cmo` (binary CMO of the Visual Studio mesh content pipeline, O64) → cooked Species mesh, colours attenuated once at cook (RND-160); placeholder: primitive mesh with sockets. Indexed sockets are numbered from 01 without gaps (AST-038).

**Socket vocabulary.** `port_NN` manning slot where a Worker stands to operate a structure; `light_NN` lamp or flare; `status_NN` status light per port; `muzzle_NN` barrel end where shots start; `casing_eject_NN` where spent casings leave; `barrel_mount` where a barrel attaches; `kill_counter` kill read-out; `carry` carry point; `leg_NN` leg root; `segment_end` end of a leg part; `engine_NN` engine glow; `essence_link` end point of an essence link; `tail_NN` tail attachment; `gate_exit_NN` destination point of a gate; `gate_surface` gate surface centre; `gate_entrance` where units enter; `door` exit for spawned units; `fuel_point` fuel connection; `glow_NN` panel glow; `grab_NN` lift points; `egg_spawn` and `egg_lay` egg release points; `entrance` boarding point; `flag` flag pole tip; `grid_link` energy-grid connection; `beam_focus` beam origin; `window_NN` lit window; `thruster` engine exhaust; `field_anchor_NN` field end points; `eye_NN` eye glints; `launch_point` pod launch point; `stem` stem attachment; `arm_tip` pump-arm tip; `dish_direction` dish aiming point. Indexed sockets count from 01 without gaps, legs included. A kind that a mesh carries exactly once may be written without an index, as its consumer rule names it: the Trooper's `muzzle` and `casing_eject` (R-UNIT-342), the Reaper Serpent head's `tail` (R-UNIT-594) and the port pad's `status` (R-BLDG-017).

| Species file | Asset id | Role | Spec (budget, nodes, sockets) | Variants | Priority |
|---|---|---|---|---|---|
| `GameData/Meshes/WorkerFigure.cmo` | `mesh.unit.worker_figure` | Worker 3-D figure (team-tinted mesh with a flag socket) | <= 100 triangles budget; 1 node, depth 1; sockets: flag | single | launch |
| `GameData/Meshes/LeaderFigure.cmo` | `mesh.unit.leader_figure` | Leader figure carrying its banner | <= 100 triangles budget; 1 node, depth 1; sockets: flag | single | mvp |
| `GameData/Meshes/TrooperFigure.cmo` | `mesh.unit.trooper_figure` | Trooper figure | <= 3000 triangles budget; 2 nodes, depth 1; sockets: casing_eject, eye_01, eye_02, muzzle | single | skirmish-first-playable |
| `GameData/Meshes/BuilderBody.cmo` | `mesh.unit.builder_body` | Builder body | <= 100 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `GameData/Meshes/TransportHull.cmo` | `mesh.unit.transport_hull` | Transport hull | <= 100 triangles budget; 3 nodes, depth 2; sockets: entrance, flag | single | skirmish-first-playable |
| `GameData/Meshes/CollectorBody.cmo` | `mesh.unit.collector_body` | Collector body | <= 3000 triangles budget; 1 node, depth 1; sockets: engine_01, engine_02, engine_03, engine_04 | single | launch |
| `GameData/Meshes/BomberBody.cmo` | `mesh.unit.bomber_body` | Bomber flying the airstrike | <= 100 triangles budget; 1 node, depth 1; sockets: none | single | skirmish-first-playable |
| `GameData/Meshes/ThrownWeapon.cmo` | `mesh.projectile.thrown_weapon` | Thrown weapon: grenade, the Bomber's bombs and rockets | <= 300 triangles budget; 1 node, depth 1; sockets: none | single | mvp |
| `GameData/Meshes/TurretRound.cmo` | `mesh.projectile.turret_round` | Turret round | <= 100 triangles budget; 1 node, depth 1; sockets: none | single | mvp |
| `GameData/Meshes/ShellCasing.cmo` | `mesh.projectile.shell_casing` | Ejected shell casing | <= 300 triangles budget; 1 node, depth 1; sockets: none | single | mvp |
| `GameData/Meshes/MeteorRock.cmo` | `mesh.projectile.meteor_rock` | Meteor of the meteor shower | <= 300 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `GameData/Meshes/AntBody01.cmo` | `mesh.creature.ant_body` | ant body, three variants chosen per ant | <= 100 triangles budget; 1 node, depth 1; sockets: carry | 1 of 3 | launch |
| `GameData/Meshes/AntBody02.cmo` | `mesh.creature.ant_body` | ″ | <= 100 triangles budget; 1 node, depth 1; sockets: carry | 2 of 3 | launch |
| `GameData/Meshes/AntBody03.cmo` | `mesh.creature.ant_body` | ″ | <= 100 triangles budget; 1 node, depth 1; sockets: carry | 3 of 3 | launch |
| `GameData/Meshes/CentipedeSegment.cmo` | `mesh.creature.centipede_segment` | centipede body segment | <= 100 triangles budget; 2 nodes, depth 1; sockets: none | single | launch |
| `GameData/Meshes/CentipedeHeadSegment.cmo` | `mesh.creature.centipede_head_segment` | centipede head segment | <= 100 triangles budget; 4 nodes, depth 1; sockets: none | single | launch |
| `GameData/Meshes/SpiderBody.cmo` | `mesh.creature.spider_body` | spider body with eight leg sockets | <= 300 triangles budget; 1 node, depth 1; sockets: egg_lay, leg_01, leg_02, leg_03, leg_04, leg_05, leg_06, leg_07, leg_08 | single | launch |
| `GameData/Meshes/SpiderThigh.cmo` | `mesh.creature.spider_thigh` | spider upper leg part (procedural leg) | <= 100 triangles budget; 1 node, depth 1; sockets: segment_end | single | launch |
| `GameData/Meshes/SpiderShin.cmo` | `mesh.creature.spider_shin` | spider lower leg part (procedural leg) | <= 100 triangles budget; 1 node, depth 1; sockets: segment_end | single | launch |
| `GameData/Meshes/ReaperSerpentHead.cmo` | `mesh.creature.reaper_serpent_head` | Reaper Serpent head | <= 100 triangles budget; 1 node, depth 1; sockets: tail | single | launch |
| `GameData/Meshes/ReaperSerpentTail.cmo` | `mesh.creature.reaper_serpent_tail` | Reaper Serpent tail segment | <= 100 triangles budget; 2 nodes, depth 1; sockets: none | single | launch |
| `GameData/Meshes/BroodDrifterBody.cmo` | `mesh.creature.brood_drifter_body` | Brood Drifter body | <= 100 triangles budget; 1 node, depth 1; sockets: egg_spawn, tail_01, tail_02, tail_03, tail_04 | single | launch |
| `GameData/Meshes/AbductorHull.cmo` | `mesh.creature.abductor_hull` | Abductor hull (also placed by maps as scenery) | <= 10000 triangles budget; 2 nodes, depth 1; sockets: engine_01, engine_02, engine_03, engine_04 | single | launch |
| `GameData/Meshes/SeedPod.cmo` | `mesh.creature.seed_pod` | Seed Pod (also the Spitter Plant's pod) | <= 300 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `GameData/Meshes/SpitterPlantHead.cmo` | `mesh.structure.spitter_plant_head` | Spitter Plant head | <= 1000 triangles budget; 1 node, depth 1; sockets: launch_point, stem | single | launch |
| `GameData/Meshes/HiveMound.cmo` | `mesh.structure.hive_mound` | Hive mound (also the placement-cursor preview) | <= 1000 triangles budget; 2 nodes, depth 1; sockets: none | single | launch |
| `GameData/Meshes/SpawnNode.cmo` | `mesh.structure.spawn_node` | Spawn Node (the match variant) | <= 300 triangles budget; 1 node, depth 1; sockets: door, essence_link, light_01, light_02, light_03, port_01, port_02, port_03, port_04, port_05 | single | skirmish-first-playable |
| `GameData/Meshes/SpawnNodeAlt.cmo` | `mesh.structure.spawn_node_alt` | Spawn Node alternative body (second constructor branch) | <= 300 triangles budget; 1 node, depth 1; sockets: door, essence_link, light_01, light_02, light_03, port_01, port_02, port_03, port_04, port_05 | single | post-launch |
| `GameData/Meshes/SourceNode.cmo` | `mesh.structure.source_node` | Source Node | <= 300 triangles budget; 1 node, depth 1; sockets: essence_link | single | launch |
| `GameData/Meshes/RelayMast.cmo` | `mesh.structure.relay_mast` | Relay | <= 300 triangles budget; 2 nodes, depth 1; sockets: essence_link | single | skirmish-first-playable |
| `GameData/Meshes/TransitGate.cmo` | `mesh.structure.transit_gate` | Transit Gate (the match variant) | <= 300 triangles budget; 1 node, depth 1; sockets: gate_exit_01, gate_exit_02, gate_surface | single | skirmish-first-playable |
| `GameData/Meshes/TransitGateAlt.cmo` | `mesh.structure.transit_gate_alt` | Transit Gate alternative body (second constructor branch) | <= 300 triangles budget; 6 nodes, depth 1; sockets: gate_exit_01, gate_exit_02, gate_surface, light_01, light_02 | single | post-launch |
| `GameData/Meshes/CommandConsole.cmo` | `mesh.structure.command_console` | Command Console | <= 1000 triangles budget; 1 node, depth 1; sockets: port_01, port_02, port_03, port_04, port_05 | single | launch |
| `GameData/Meshes/CommandConsoleLinked.cmo` | `mesh.structure.command_console_linked` | Command Console linked to a Core Device (with dish-direction socket) | <= 1000 triangles budget; 2 nodes, depth 1; sockets: dish_direction, port_01, port_02, port_03 | single | launch |
| `GameData/Meshes/CoreDevice.cmo` | `mesh.structure.core_device` | Core Device | <= 3000 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `GameData/Meshes/BreachCharge.cmo` | `mesh.structure.breach_charge` | Breach Charge | <= 300 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `GameData/Meshes/LaunchVessel.cmo` | `mesh.structure.launch_vessel` | Launch Vessel | <= 10000 triangles budget; 1 node, depth 1; sockets: fuel_point, thruster, window_01, window_02, window_03 | single | launch |
| `GameData/Meshes/FuelPumpHousing.cmo` | `mesh.structure.fuel_pump_housing` | fuel pump housing | <= 300 triangles budget; 1 node, depth 1; sockets: fuel_point | single | launch |
| `GameData/Meshes/FuelPumpArm.cmo` | `mesh.structure.fuel_pump_arm` | fuel pump moving arm | <= 300 triangles budget; 1 node, depth 1; sockets: arm_tip | single | launch |
| `GameData/Meshes/FuelLineFoot.cmo` | `mesh.structure.fuel_line_foot` | fuel line foot | <= 100 triangles budget; 2 nodes, depth 1; sockets: fuel_point | single | launch |
| `GameData/Meshes/FuelLineSpan.cmo` | `mesh.structure.fuel_line_span` | fuel line span between feet | <= 100 triangles budget; 1 node, depth 1; sockets: none | single | launch |
| `GameData/Meshes/FuelDepot.cmo` | `mesh.structure.fuel_depot` | fuel depot | <= 1000 triangles budget; 4 nodes, depth 1; sockets: entrance, fuel_point | single | launch |
| `GameData/Meshes/SolarArray.cmo` | `mesh.structure.solar_array` | Solar Array | <= 1000 triangles budget; 5 nodes, depth 1; sockets: glow_01, glow_02, glow_03, glow_04, grid_link, port_01, port_02, port_03, port_04, port_05, status_01, status_02, status_03, status_04, status_05 | single | launch |
| `GameData/Meshes/GridPylon.cmo` | `mesh.structure.grid_pylon` | grid pylon (also the grid terminal and the grid splitter) | <= 100 triangles budget; 3 nodes, depth 1; sockets: grid_link, light_01 | single | launch |
| `GameData/Meshes/BeamDish.cmo` | `mesh.structure.beam_dish` | Beam Dish with a rotating dish node | <= 300 triangles budget; 6 nodes, depth 3; sockets: beam_focus (on node dish), gate_entrance (on node door), light_01 (on node upper_mount), light_02 (on node column) | single | launch |
| `GameData/Meshes/BarrierEmitter.cmo` | `mesh.structure.barrier_emitter` | Barrier Emitter post | <= 100 triangles budget; 2 nodes, depth 1; sockets: field_anchor_01, field_anchor_02, light_01 | single | skirmish-first-playable |
| `GameData/Meshes/TurretMount.cmo` | `mesh.structure.turret_mount` | turret mount: socket carrier only (manning ports and status lights), no geometry | no triangles budget; 0 nodes, depth 0; sockets: port_01, port_02, port_03, port_04, status_01, status_02, status_03, status_04 | single | mvp |
| `GameData/Meshes/TurretHead.cmo` | `mesh.structure.turret_head` | turret head (also the placement-cursor preview) | <= 300 triangles budget; 2 nodes, depth 1; sockets: barrel_mount, kill_counter | single | mvp |
| `GameData/Meshes/TurretBarrelGun.cmo` | `mesh.structure.turret_barrel` | turret barrel set per turret type, in member order: gun barrel (gun and mortar turrets), rocket barrel (rocket turret), flame barrel (flame turret), cannon barrel (Convert Turret) | <= 1000 triangles budget; 20 nodes, depth 1; sockets: casing_eject_01, casing_eject_02, casing_eject_03, casing_eject_04, muzzle_01, muzzle_02, muzzle_03, muzzle_04 | 1 of 4 | mvp |
| `GameData/Meshes/TurretBarrelRocket.cmo` | `mesh.structure.turret_barrel` | ″ | <= 3000 triangles budget; 8 nodes, depth 1; sockets: muzzle_01, muzzle_02, muzzle_03, muzzle_04 | 2 of 4 | launch |
| `GameData/Meshes/TurretBarrelFlame.cmo` | `mesh.structure.turret_barrel` | ″ | <= 3000 triangles budget; 20 nodes, depth 1; sockets: muzzle_01, muzzle_02, muzzle_03, muzzle_04 | 3 of 4 | launch |
| `GameData/Meshes/TurretBarrelCannon.cmo` | `mesh.structure.turret_barrel` | ″ | <= 100 triangles budget; 4 nodes, depth 1; sockets: muzzle_01, muzzle_02, muzzle_03, muzzle_04 | 4 of 4 | launch |
| `GameData/Meshes/WallSection.cmo` | `mesh.structure.wall_section` | wall section | <= 1000 triangles budget; 3 nodes, depth 1; sockets: none | single | skirmish-first-playable |
| `GameData/Meshes/SupplyCrate.cmo` | `mesh.structure.supply_crate` | Supply crate body | <= 1000 triangles budget; 1 node, depth 1; sockets: grab_01, grab_02 | single | skirmish-first-playable |
| `GameData/Meshes/BotNavNode.cmo` | `mesh.structure.bot_nav_node` | bot navigation node marker, drawn and picked in the map editor only | <= 100 triangles budget; 2 nodes, depth 1; sockets: light_01 | single | post-launch |
| `GameData/Meshes/StructureDefault.cmo` | `mesh.structure.structure_default` | Port pad: the pad mesh drawn under every port, whose `status` socket places the port's status star (R-BLDG-017, RND-168); also the fallback body drawn when a structure's mesh is missing (RND-163) | <= 300 triangles budget; 3 nodes, depth 1; sockets: status | single | mvp |
| `GameData/Meshes/Wellspring.cmo` | `mesh.structure.wellspring` | Wellspring: the settlement's centre and first drop-off point, from which new Workers emerge; glow accents at its lights in its Cladan's colour (MVP-020, MVP-021, MVP-083) | <= 1000 triangles budget; 2 nodes, depth 1; sockets: door, entrance, essence_link, light_01, light_02, light_03, port_01, port_02, port_03, port_04, port_05 | single | mvp |
| `GameData/Meshes/Dwelling.cmo` | `mesh.structure.dwelling` | Dwelling: raises the population limit; its windows glow in its Cladan's colour while it is active and go dark while it is inactive; it rises from the ground while it is built (MVP-022, MVP-033 to MVP-035, MVP-083) | <= 300 triangles budget; 1 node, depth 1; sockets: port_01, port_02, port_03, port_04, port_05, port_06, window_01, window_02, window_03, window_04; the ports are its construction ports (MVP-033) | single | mvp |
| `GameData/Meshes/Storehouse.cmo` | `mesh.structure.storehouse` | Storehouse: holds more stores and is a drop-off point for carried loads; it rises from the ground while it is built and goes dark while it is inactive (MVP-031, MVP-033 to MVP-035, MVP-083) | <= 300 triangles budget; 1 node, depth 1; sockets: entrance, light_01, light_02, port_01, port_02, port_03, port_04; the ports are its construction ports (MVP-033) and the entrance its drop-off point (MVP-031) | single | mvp |
| `GameData/Meshes/ResourceSite.cmo` | `mesh.structure.resource_site` | Resource site, in member order: a home or border site of one resource unit and the valley's central site of three units; Workers gather at its ports, whose port pads show which are manned (MVP-030, MVP-031, MVP-083) | <= 300 triangles budget; 1 node, depth 1; sockets: port_01, port_02, port_03, port_04; the gathering ports of its one unit (`mvp.resource.ports_per_unit`) | 1 of 2 | mvp |
| `GameData/Meshes/ResourceSiteCentral.cmo` | `mesh.structure.resource_site` | ″ | <= 1000 triangles budget; 1 node, depth 1; sockets: port_01, port_02, port_03, port_04, port_05, port_06, port_07, port_08, port_09, port_10, port_11, port_12; the gathering ports of its three units | 2 of 2 | mvp |
| `GameData/Meshes/Relic01.cmo` | `mesh.relic.variant` | Relic, eight interchangeable variants | <= 10000 triangles budget; 3 nodes, depth 1; sockets: none | 1 of 8 | launch |
| `GameData/Meshes/Relic02.cmo` | `mesh.relic.variant` | ″ | <= 10000 triangles budget; 6 nodes, depth 1; sockets: none | 2 of 8 | launch |
| `GameData/Meshes/Relic03.cmo` | `mesh.relic.variant` | ″ | <= 25000 triangles budget; 5 nodes, depth 1; sockets: none | 3 of 8 | launch |
| `GameData/Meshes/Relic04.cmo` | `mesh.relic.variant` | ″ | <= 10000 triangles budget; 2 nodes, depth 1; sockets: none | 4 of 8 | launch |
| `GameData/Meshes/Relic05.cmo` | `mesh.relic.variant` | ″ | <= 10000 triangles budget; 2 nodes, depth 1; sockets: none | 5 of 8 | launch |
| `GameData/Meshes/Relic06.cmo` | `mesh.relic.variant` | ″ | <= 10000 triangles budget; 5 nodes, depth 1; sockets: none | 6 of 8 | launch |
| `GameData/Meshes/Relic07.cmo` | `mesh.relic.variant` | ″ | <= 3000 triangles budget; 7 nodes, depth 1; sockets: none | 7 of 8 | launch |
| `GameData/Meshes/Relic08.cmo` | `mesh.relic.variant` | ″ | <= 10000 triangles budget; 4 nodes, depth 1; sockets: none | 8 of 8 | launch |
| `GameData/Meshes/SceneryProp01.cmo` | `mesh.scenery.prop` | map scenery mesh, seven pieces placed by maps | <= 100 triangles budget; 3 nodes, depth 1; sockets: none | 1 of 7 | launch |
| `GameData/Meshes/SceneryProp02.cmo` | `mesh.scenery.prop` | ″ | <= 3000 triangles budget; 19 nodes, depth 1; sockets: none | 2 of 7 | launch |
| `GameData/Meshes/SceneryProp03.cmo` | `mesh.scenery.prop` | ″ | <= 1000 triangles budget; 1 node, depth 1; sockets: none | 3 of 7 | launch |
| `GameData/Meshes/SceneryProp04.cmo` | `mesh.scenery.prop` | ″ | <= 3000 triangles budget; 2 nodes, depth 1; sockets: none | 4 of 7 | launch |
| `GameData/Meshes/SceneryProp05.cmo` | `mesh.scenery.prop` | ″ | <= 300 triangles budget; 1 node, depth 1; sockets: none | 5 of 7 | launch |
| `GameData/Meshes/SceneryProp06.cmo` | `mesh.scenery.prop` | ″ | <= 300 triangles budget; 2 nodes, depth 1; sockets: none | 6 of 7 | launch |
| `GameData/Meshes/SceneryProp07.cmo` | `mesh.scenery.prop` | ″ | <= 300 triangles budget; 1 node, depth 1; sockets: none | 7 of 7 | launch |
| `GameData/Meshes/CameraView.cmo` | `mesh.dev.camera_view` | Camera-view marker that the map editor draws at each camera view and uses for picking (R-MAP-322, R-MAP-380); editor builds only | small camera-frustum marker; 1 node; no sockets; few triangles | single | post-launch |

### 11.1 Collision assets

Every collidable mesh, that is every mesh that a structure's sphere, mesh or ray test reads ([buildings and world objects](../reference/03_buildings_and_world_objects/README.md) R-BLDG-041 to R-BLDG-043), has a collision asset: a separate CMO file of triangles whose materials are ignored (O64), authored as its own source and never generated from render geometry, cooked into a simulation-relevant `Collision` entry ([asset pipeline](../architecture/components/asset_pipeline_and_resources.md) AST-044). A collision asset has one member per member of its render mesh, in the same order, and the render mesh's import record names it. The structures that the hit tests never read (Launch Vessel, Control Zone, carryable objects, bot navigation nodes and the other kinds of table 2.12-A that are never hit), the jump pad, trees and the turret mount have none: the mount is a socket carrier with no geometry (AST-045), and the turret's sphere, mesh and ray tests read its head and barrel assets (table 2.12-A). Placeholder: primitive collision.

| Species file | Asset id | Role | Spec | Variants | Priority |
|---|---|---|---|---|---|
| `GameData/Meshes/SpawnNodeCollision.cmo` | `mesh.collision.spawn_node` | Collision asset of `mesh.structure.spawn_node`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | skirmish-first-playable |
| `GameData/Meshes/SpawnNodeAltCollision.cmo` | `mesh.collision.spawn_node_alt` | Collision asset of `mesh.structure.spawn_node_alt`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | post-launch |
| `GameData/Meshes/SourceNodeCollision.cmo` | `mesh.collision.source_node` | Collision asset of `mesh.structure.source_node`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | launch |
| `GameData/Meshes/RelayMastCollision.cmo` | `mesh.collision.relay_mast` | Collision asset of `mesh.structure.relay_mast`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | skirmish-first-playable |
| `GameData/Meshes/TransitGateCollision.cmo` | `mesh.collision.transit_gate` | Collision asset of `mesh.structure.transit_gate`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | skirmish-first-playable |
| `GameData/Meshes/TransitGateAltCollision.cmo` | `mesh.collision.transit_gate_alt` | Collision asset of `mesh.structure.transit_gate_alt`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | post-launch |
| `GameData/Meshes/CommandConsoleCollision.cmo` | `mesh.collision.command_console` | Collision asset of `mesh.structure.command_console`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | single | launch |
| `GameData/Meshes/CommandConsoleLinkedCollision.cmo` | `mesh.collision.command_console_linked` | Collision asset of `mesh.structure.command_console_linked`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | single | launch |
| `GameData/Meshes/CoreDeviceCollision.cmo` | `mesh.collision.core_device` | Collision asset of `mesh.structure.core_device`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 3000 triangles budget | single | launch |
| `GameData/Meshes/FuelPumpHousingCollision.cmo` | `mesh.collision.fuel_pump_housing` | Collision asset of `mesh.structure.fuel_pump_housing`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | launch |
| `GameData/Meshes/FuelLineFootCollision.cmo` | `mesh.collision.fuel_line_foot` | Collision asset of `mesh.structure.fuel_line_foot`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 100 triangles budget | single | launch |
| `GameData/Meshes/FuelDepotCollision.cmo` | `mesh.collision.fuel_depot` | Collision asset of `mesh.structure.fuel_depot`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | single | launch |
| `GameData/Meshes/SolarArrayCollision.cmo` | `mesh.collision.solar_array` | Collision asset of `mesh.structure.solar_array`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | single | launch |
| `GameData/Meshes/GridPylonCollision.cmo` | `mesh.collision.grid_pylon` | Collision asset of `mesh.structure.grid_pylon`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 100 triangles budget | single | launch |
| `GameData/Meshes/BeamDishCollision.cmo` | `mesh.collision.beam_dish` | Collision asset of `mesh.structure.beam_dish`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | launch |
| `GameData/Meshes/BarrierEmitterCollision.cmo` | `mesh.collision.barrier_emitter` | Collision asset of `mesh.structure.barrier_emitter`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 100 triangles budget | single | skirmish-first-playable |
| `GameData/Meshes/TurretHeadCollision.cmo` | `mesh.collision.turret_head` | Collision asset of `mesh.structure.turret_head`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | mvp |
| `GameData/Meshes/TurretBarrelGunCollision.cmo` | `mesh.collision.turret_barrel` | Collision asset of `mesh.structure.turret_barrel`, one per member in its order: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | 1 of 4 | mvp |
| `GameData/Meshes/TurretBarrelRocketCollision.cmo` | `mesh.collision.turret_barrel` | ″ | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 3000 triangles budget | 2 of 4 | skirmish-first-playable |
| `GameData/Meshes/TurretBarrelFlameCollision.cmo` | `mesh.collision.turret_barrel` | ″ | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 3000 triangles budget | 3 of 4 | skirmish-first-playable |
| `GameData/Meshes/TurretBarrelCannonCollision.cmo` | `mesh.collision.turret_barrel` | ″ | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 100 triangles budget | 4 of 4 | skirmish-first-playable |
| `GameData/Meshes/WallSectionCollision.cmo` | `mesh.collision.wall_section` | Collision asset of `mesh.structure.wall_section`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | single | skirmish-first-playable |
| `GameData/Meshes/HiveMoundCollision.cmo` | `mesh.collision.hive_mound` | Collision asset of `mesh.structure.hive_mound`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | single | launch |
| `GameData/Meshes/SpitterPlantHeadCollision.cmo` | `mesh.collision.spitter_plant_head` | Collision asset of `mesh.structure.spitter_plant_head`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | single | launch |
| `GameData/Meshes/SceneryProp01Collision.cmo` | `mesh.collision.scenery_prop` | Collision asset of `mesh.scenery.prop`, one per member in its order: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 100 triangles budget | 1 of 7 | launch |
| `GameData/Meshes/SceneryProp02Collision.cmo` | `mesh.collision.scenery_prop` | ″ | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 3000 triangles budget | 2 of 7 | launch |
| `GameData/Meshes/SceneryProp03Collision.cmo` | `mesh.collision.scenery_prop` | ″ | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | 3 of 7 | launch |
| `GameData/Meshes/SceneryProp04Collision.cmo` | `mesh.collision.scenery_prop` | ″ | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 3000 triangles budget | 4 of 7 | launch |
| `GameData/Meshes/SceneryProp05Collision.cmo` | `mesh.collision.scenery_prop` | ″ | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | 5 of 7 | launch |
| `GameData/Meshes/SceneryProp06Collision.cmo` | `mesh.collision.scenery_prop` | ″ | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | 6 of 7 | launch |
| `GameData/Meshes/SceneryProp07Collision.cmo` | `mesh.collision.scenery_prop` | ″ | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | 7 of 7 | launch |
| `GameData/Meshes/AbductorHullCollision.cmo` | `mesh.collision.abductor_hull` | Collision asset of `mesh.creature.abductor_hull` where a map places it as a scenery mesh: the hit geometry that the sphere, mesh and ray tests read (R-BLDG-041, R-BLDG-042, R-BLDG-459, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 10000 triangles budget | single | launch |
| `GameData/Meshes/WellspringCollision.cmo` | `mesh.collision.wellspring` | Collision asset of `mesh.structure.wellspring`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | single | mvp |
| `GameData/Meshes/DwellingCollision.cmo` | `mesh.collision.dwelling` | Collision asset of `mesh.structure.dwelling`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | mvp |
| `GameData/Meshes/StorehouseCollision.cmo` | `mesh.collision.storehouse` | Collision asset of `mesh.structure.storehouse`: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | single | mvp |
| `GameData/Meshes/ResourceSiteCollision.cmo` | `mesh.collision.resource_site` | Collision asset of `mesh.structure.resource_site`, one per member in its order: the hit geometry that the structure's sphere, mesh and ray tests read (R-BLDG-041 to R-BLDG-043, AST-044) | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 300 triangles budget | 1 of 2 | mvp |
| `GameData/Meshes/ResourceSiteCentralCollision.cmo` | `mesh.collision.resource_site` | ″ | triangle primitives only, no materials, no sockets; the nodes of its render member that the hit tests read, at that member's bounds; <= 1000 triangles budget | 2 of 2 | mvp |

## 12 Help images and explainer animations

Help pictures carry no baked text: labels come from the string table, so one picture serves every language. Explainer animations are one script per mode that moves pictures on a canvas for a fixed loop; the scripts are re-authored in the Species explainer format and reference their pictures by asset id. Choreography (what moves when) is new; canvas size and loop length follow the reference. The explainer panel samples its pictures linearly with mip-maps (R-META-231), so they carry full mip chains. The objective cards have no picture assets: they draw nothing in a match, and their pictures are created only if the design phase adopts objective cards (DEC-050, R-TUT-013).

| Species file | Asset id | Role | Formats (source → runtime) | Spec | Variants | Priority | Placeholder |
|---|---|---|---|---|---|---|---|
| `GameData/Textures/Help/HintCameraControls.dds` | `help.hint.camera_controls` | Hint: camera controls, shown during the grace period; text labels come from the string table | BC7 sRGB DDS, no mips, packed as authored (O64) | 304x128 px display size; author at 2x (608x256) | single | launch | diagram card |
| `GameData/Textures/Help/ModeCardSpawnControl.dds` | `help.mode_card` | Mode card on the mode-select buttons, one per mode | BC7 sRGB DDS, no mips, packed as authored (O64) | 300x200 px display size; author at 2x (600x400); Spawn Control | 1 of 6 | skirmish-first-playable | diagram card |
| `GameData/Textures/Help/ModeCardZoneControl.dds` | `help.mode_card` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 300x200 px display size; author at 2x (600x400); Zone Control | 2 of 6 | launch | diagram card |
| `GameData/Textures/Help/ModeCardRelicRun.dds` | `help.mode_card` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 300x200 px display size; author at 2x (600x400); Relic Run | 3 of 6 | launch | diagram card |
| `GameData/Textures/Help/ModeCardSiege.dds` | `help.mode_card` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 300x200 px display size; author at 2x (600x400); Siege | 4 of 6 | launch | diagram card |
| `GameData/Textures/Help/ModeCardLaunchRace.dds` | `help.mode_card` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 300x200 px display size; author at 2x (600x400); Launch Race | 5 of 6 | launch | diagram card |
| `GameData/Textures/Help/ModeCardFlagChain.dds` | `help.mode_card` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 300x200 px display size; author at 2x (600x400); Flag Chain | 6 of 6 | launch | diagram card |
| `GameData/Textures/Help/GeneralPage01.dds` | `help.general_page` | General help page picture, pages 1-5 | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 1 of 5 | launch | diagram card |
| `GameData/Textures/Help/GeneralPage02.dds` | `help.general_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 2 of 5 | launch | diagram card |
| `GameData/Textures/Help/GeneralPage03.dds` | `help.general_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 3 of 5 | launch | diagram card |
| `GameData/Textures/Help/GeneralPage04.dds` | `help.general_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 4 of 5 | launch | diagram card |
| `GameData/Textures/Help/GeneralPage05.dds` | `help.general_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 5 of 5 | launch | diagram card |
| `GameData/Textures/Help/SpawnControlPage01.dds` | `help.mode_page` | Per-mode help page picture (pages per mode: Spawn Control 1, Zone Control 2, Relic Run 3, Siege 3, Launch Race 4, Flag Chain 3) | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 1 of 16 | launch | diagram card |
| `GameData/Textures/Help/ZoneControlPage01.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 2 of 16 | launch | diagram card |
| `GameData/Textures/Help/ZoneControlPage02.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 3 of 16 | launch | diagram card |
| `GameData/Textures/Help/RelicRunPage01.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 4 of 16 | launch | diagram card |
| `GameData/Textures/Help/RelicRunPage02.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 5 of 16 | launch | diagram card |
| `GameData/Textures/Help/RelicRunPage03.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 6 of 16 | launch | diagram card |
| `GameData/Textures/Help/SiegePage01.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 7 of 16 | launch | diagram card |
| `GameData/Textures/Help/SiegePage02.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 8 of 16 | launch | diagram card |
| `GameData/Textures/Help/SiegePage03.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 9 of 16 | launch | diagram card |
| `GameData/Textures/Help/LaunchRacePage01.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 10 of 16 | launch | diagram card |
| `GameData/Textures/Help/LaunchRacePage02.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 11 of 16 | launch | diagram card |
| `GameData/Textures/Help/LaunchRacePage03.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 12 of 16 | launch | diagram card |
| `GameData/Textures/Help/LaunchRacePage04.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 13 of 16 | launch | diagram card |
| `GameData/Textures/Help/FlagChainPage01.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 14 of 16 | launch | diagram card |
| `GameData/Textures/Help/FlagChainPage02.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 15 of 16 | launch | diagram card |
| `GameData/Textures/Help/FlagChainPage03.dds` | `help.mode_page` | ″ | BC7 sRGB DDS, no mips, packed as authored (O64) | 150x150 px display size; author at 2x (300x300) | 16 of 16 | launch | diagram card |
| `GameData/Textures/Help/ExplainerTransport.dds` | `help.explainer_picture` | Picture animated by the explainer scripts | BC7 sRGB DDS, mips, packed as authored (O64) | Transport; 128x80 px; used by Launch Race | 1 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerGlow.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | soft glow; 128x128 px; used by Flag Chain, Launch Race, Relic Run, Siege, Spawn Control, Zone Control | 2 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerWorker.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | Worker figure; 128x128 px; used by Flag Chain, Launch Race, Relic Run, Siege, Spawn Control, Zone Control | 3 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerWorkerHurt.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | hurt Worker; 128x128 px; used by Flag Chain, Relic Run, Siege, Spawn Control, Zone Control | 4 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerFlagNeutral.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | neutral flag; 64x64 px; used by Flag Chain | 5 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerFlagLowered.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | lowered flag; 64x64 px; used by Flag Chain | 6 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerFlagLocked.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | locked flag; 64x64 px; used by Flag Chain | 7 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerFlagRaised.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | raised flag; 64x64 px; used by Flag Chain | 8 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerFuelPump.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | fuel pump; 184x256 px; used by Launch Race | 9 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerFuelPumpArm.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | fuel pump arm; 52x256 px; used by Flag Chain, Launch Race | 10 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerIslandShape.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | island outline; 256x128 px; used by Launch Race | 11 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerBeam.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | laser beam; 128x32 px; used by Flag Chain, Launch Race, Relic Run, Spawn Control, Zone Control | 12 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerMuzzleFlash.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | muzzle flash; 64x64 px; used by Flag Chain, Launch Race, Relic Run, Siege, Spawn Control, Zone Control | 13 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerScorePlusOne.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | +1 score pop-up; 128x128 px; used by Relic Run, Zone Control | 14 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerLaunchVessel.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | Launch Vessel; 240x256 px; used by Launch Race | 15 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerSolarArray.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | Solar Array; 160x256 px; used by Launch Race | 16 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerSpawnNode.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | Spawn Node; 260x256 px; used by Relic Run, Spawn Control, Zone Control | 17 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerRelic.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | Relic; 256x256 px; used by Relic Run | 18 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerTransitGate.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | Transit Gate; 256x256 px; used by Flag Chain, Launch Race, Siege | 19 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerTurretBarrel.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | turret barrel; 140x92 px; used by Siege | 20 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerTurretBase.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | turret base; 140x112 px; used by Siege | 21 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerWater.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | water band; 256x128 px; used by Launch Race | 22 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerCoreDevice.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | Core Device; 208x256 px; used by Siege | 23 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerZoneBack.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | Control Zone, back layer; 512x312 px; used by Relic Run, Zone Control | 24 of 25 | launch | generated panel |
| `GameData/Textures/Help/ExplainerZoneFront.dds` | `help.explainer_picture` | ″ | BC7 sRGB DDS, mips, packed as authored (O64) | Control Zone, front layer; 512x312 px; used by Relic Run, Zone Control | 25 of 25 | launch | generated panel |
| `GameData/Ui/explainers/explainer_siege.json` | `anim.explainer` | Explainer animation: a timed key-frame script that animates pictures on a canvas, one per mode | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 900x250 px, loop 12.02 s; Siege | 1 of 6 | launch | static mode card |
| `GameData/Ui/explainers/explainer_flag_chain.json` | `anim.explainer` | ″ | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 900x250 px, loop 24.01 s; Flag Chain | 2 of 6 | launch | static mode card |
| `GameData/Ui/explainers/explainer_relic_run.json` | `anim.explainer` | ″ | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 924x250 px, loop 20.01 s; Relic Run | 3 of 6 | launch | static mode card |
| `GameData/Ui/explainers/explainer_spawn_control.json` | `anim.explainer` | ″ | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 800x250 px, loop 15.01 s; Spawn Control | 4 of 6 | launch | static mode card |
| `GameData/Ui/explainers/explainer_zone_control.json` | `anim.explainer` | ″ | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 800x250 px, loop 15.66 s; Zone Control | 5 of 6 | launch | static mode card |
| `GameData/Ui/explainers/explainer_launch_race.json` | `anim.explainer` | ″ | JSON key-frame script in the Species explainer format (pictures referenced by asset id) → cooked explainer script in the pack | canvas 1000x250 px, loop 30.01 s; Launch Race | 6 of 6 | launch | static mode card |

## 13 Audio by source and event

Sound effects are sample groups named by **source and event**: a group holds the variants one event picks from, and its file count matches the reference group, so the mix keeps the same variety. Behaviour (volume, pitch spread, distance, looping, priority) lives in the sound-event data of [10](../reference/10_audio_design/README.md), not in the files. Where one reference group serves several events, the Species group is named after its main binding and the other bindings are listed; two files are shared by two groups and listed once. Formats: WAV 48 kHz 24-bit → 16-bit PCM resident, or an MS-ADPCM stream, decoded by XAudio2, for every member of a group whose longest member is over 20 s (`aen.residency.stream_threshold`, AEN-072, ADR-025); the resident groups fit the bank budgets of AEN-243, which the content build measures on the authored, cooked set at every build (AEN-304, ADR-025). A spec's target duration, rounded to a tenth of a second, is a timing target, not a limit. Placeholder: synthesised tone (voice lines: text-to-speech line).

182 (source, event) bindings of the reference fire in a match and play these groups. Of the events that fire with no bound sound in the reference, four gain new groups here by Resolved decisions (the laser-bolt fire, structure-hit and kill groups of DEC-564 and the Trooper grenade throw of DEC-097), the game-over announcement is a UI sound (§14), and the victory and defeat events play the music stings of §16; every other one stays silent and needs no file ([event catalogue](../reference/10_audio_design/event_catalogue.md) R-AUD-141 to R-AUD-143). The MVP's first sound set (MVP-087) plays four of these groups: the laser-bolt fire and the Worker death at their own events, the orb collected when an orb becomes a Worker at its Wellspring, and the war horn as the attack warning. With them it plays the structure-complete group of §13.41 and the battle start and end sounds of §14.

### 13.1 Builder (15 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/builder/builder_presence_loop.wav` | `sfx.unit.builder.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 0.8 s | 1 variant | launch |
| `GameData/Audio/sfx/builder/builder_hurt_{01..09}.wav` (9 files) | `sfx.unit.builder.hurt` | takes damage | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 1.3-2.8 s | 9 variants | launch |
| `GameData/Audio/sfx/builder/builder_die_{01..05}.wav` (5 files) | `sfx.unit.builder.die` | dies | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 2.3-3.5 s | 5 variants | launch |

### 13.2 Crawler (47 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/crawler/crawler_attack_{01..08}.wav` (8 files) | `sfx.creature.crawler.attack` | attacks | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.3-0.8 s | 8 variants | launch |
| `GameData/Audio/sfx/crawler/crawler_hurt_{01..03}.wav` (3 files) | `sfx.creature.crawler.hurt` | takes damage | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 0.2-0.8 s | 3 variants | launch |
| `GameData/Audio/sfx/crawler/crawler_turn_{01..29}.wav` (29 files) | `sfx.creature.crawler.turn` | changes direction | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.1 s | 29 variants | launch |
| `GameData/Audio/sfx/crawler/crawler_die_{01..07}.wav` (7 files) | `sfx.creature.crawler.die` | dies | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.2-0.8 s | 7 variants | launch |

### 13.3 Trooper (52 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/trooper/trooper_presence_loop_{01..20}.wav` (20 files) | `sfx.unit.trooper.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 0.3-1.0 s | 20 variants | skirmish-first-playable |
| `GameData/Audio/sfx/trooper/trooper_hurt_{01..04}.wav` (4 files) | `sfx.unit.trooper.hurt` | takes damage | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.3-0.9 s | 4 variants | skirmish-first-playable |
| `GameData/Audio/sfx/trooper/trooper_weapon_ready.wav` | `sfx.unit.trooper.weapon_ready` | weapon is ready again | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.5 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/trooper/trooper_fire_laser_{01..06}.wav` (6 files) | `sfx.unit.trooper.fire_laser` | fires its laser | `turret.fire_gun` | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.9-1.1 s | 6 variants | skirmish-first-playable |
| `GameData/Audio/sfx/trooper/trooper_die_{01..05}.wav` (5 files) | `sfx.unit.trooper.die` | dies | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 1.8-2.4 s | 5 variants | skirmish-first-playable |
| `GameData/Audio/sfx/trooper/trooper_enter_gate_{01..06}.wav` (6 files) | `sfx.unit.trooper.enter_gate` | enters a gate | `leader.enter_gate` | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 0.6-0.7 s | 6 variants | skirmish-first-playable |
| `GameData/Audio/sfx/trooper/trooper_exit_gate_{01..06}.wav` (6 files) | `sfx.unit.trooper.exit_gate` | leaves a gate | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.6-0.7 s | 6 variants | skirmish-first-playable |
| `GameData/Audio/sfx/trooper/trooper_throw_grenade_{01..04}.wav` (4 files) | `sfx.unit.trooper.throw_grenade` | the Trooper throws the Strike team grenade (DEC-097) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.5-1.0 s; variant count proposed (DEC-097) | 4 variants | skirmish-first-playable |

### 13.4 Brood Drifter (15 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/brood_drifter/brood_drifter_presence_loop_{01..03}.wav` (3 files) | `sfx.creature.brood_drifter.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 4.6-5.7 s | 3 variants | launch |
| `GameData/Audio/sfx/brood_drifter/brood_drifter_hurt_{01..06}.wav` (6 files) | `sfx.creature.brood_drifter.hurt` | takes damage | `spitter_plant.damaged` | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.3-4.7 s | 6 variants | launch |
| `GameData/Audio/sfx/brood_drifter/brood_drifter_die_{01..06}.wav` (6 files) | `sfx.creature.brood_drifter.die` | dies | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 2.5-5.2 s | 6 variants | launch |

### 13.5 spider (53 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/spider/spider_lay_egg_{01..10}.wav` (10 files) | `sfx.creature.spider.lay_egg` | lays an egg | `brood_drifter.lay_egg` | mono, 48 kHz; positional, follows the source; one-shot; target duration about 1.2-2.7 s | 10 variants | launch |
| `GameData/Audio/sfx/spider/spider_presence_loop_{01..17}.wav` (17 files) | `sfx.creature.spider.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 0.5-2.3 s | 17 variants | launch |
| `GameData/Audio/sfx/spider/spider_die_{01..05}.wav` (5 files) | `sfx.creature.spider.die` | dies | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.8-3.2 s | 5 variants + 2 shared | launch |
| `GameData/Audio/sfx/spider/spider_leap_{01..06}.wav` (6 files) | `sfx.creature.spider.pounce` | pounces | `spider.attack` | mono, 48 kHz; positional, follows the source; one-shot; target duration about 3.4-4.4 s | 6 variants | launch |
| `GameData/Audio/sfx/spider/spider_hurt_{01..02}.wav` (2 files) | `sfx.creature.spider.hurt` | takes damage | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.8-3.2 s | 2 variants | launch |
| `GameData/Audio/sfx/spider/spider_footstep_{01..13}.wav` (13 files) | `sfx.creature.spider.footstep` | footstep | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.1-0.4 s | 13 variants | launch |

### 13.6 centipede (21 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/centipede/centipede_bite_{01..08}.wav` (8 files) | `sfx.creature.centipede.attack` | attacks | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.5-1.4 s | 8 variants | launch |
| `GameData/Audio/sfx/centipede/centipede_presence_loop_{01..04}.wav` (4 files) | `sfx.creature.centipede.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 2.9-4.3 s | 4 variants | launch |
| `GameData/Audio/sfx/centipede/centipede_die_{01..04}.wav` (4 files) | `sfx.creature.centipede.die` | dies | `launch_vessel.explode (layer 2)` | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 4.2-5.5 s | 4 variants | launch |
| `GameData/Audio/sfx/centipede/centipede_hurt_{01..05}.wav` (5 files) | `sfx.creature.centipede.hurt` | takes damage | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.9-1.7 s | 5 variants | launch |

### 13.7 Bomber (2 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/bomber/bomber_drop_bomb.wav` | `sfx.unit.bomber.drop_bomb` | drops a bomb | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 4.0 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/bomber/bomber_presence_loop.wav` | `sfx.unit.bomber.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 3.9 s | 1 variant | skirmish-first-playable |

### 13.8 Worker (194 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/worker/worker_die_{01..08}.wav` (8 files) | `sfx.unit.worker.die` | dies | `leader.die` | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 0.4-1.3 s | 8 variants | mvp |
| `GameData/Audio/sfx/worker/worker_die_b_{01..12}.wav` (12 files) | `sfx.unit.worker.die_b` | dies (additional layer) | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 0.2-1.2 s | 12 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_hurt_{01..18}.wav` (18 files) | `sfx.unit.worker.hurt` | takes damage | `leader.hurt` | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.2-0.9 s | 18 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_spawn_{01..15}.wav` (15 files) | `sfx.unit.worker.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.1-0.5 s | 15 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_flee_bark_{01..10}.wav` (10 files) | `sfx.unit.worker.flee_bark` | flees from a threat (voice) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.4-1.1 s | 10 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_flee_bark_b_{01..26}.wav` (26 files) | `sfx.unit.worker.flee_bark_b` | flees from a threat (voice) (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.3-1.7 s | 26 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_attack_{01..24}.wav` (24 files) | `sfx.unit.worker.attack` | attacks | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.5-1.0 s | 24 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_attack_b_{01..05}.wav` (5 files) | `sfx.unit.worker.attack_b` | attacks (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.7-1.4 s | 5 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_victory_jump_{01..09}.wav` (9 files) | `sfx.unit.worker.victory_jump` | jumps in victory | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.4-3.2 s | 9 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_burning_{01..13}.wav` (13 files) | `sfx.unit.worker.burning` | burns | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 0.2-4.1 s; members 07 to 13 are silent samples, part of the group's design (R-AUD-066) | 13 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_selected_{01..10}.wav` (10 files) | `sfx.unit.worker.selected` | is selected (voice) | `worker.ordered_by_player` | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.1-0.4 s | 10 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_lift_relic_{01..04}.wav` (4 files) | `sfx.unit.worker.lift_relic` | lifts a Relic | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 3.0 s | 4 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_attack_bark_{01..37}.wav` (37 files) | `sfx.unit.worker.attack_bark` | charges a threat (voice) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.4-1.5 s | 37 variants | skirmish-first-playable |
| `GameData/Audio/sfx/worker/worker_attack_horn_{01..03}.wav` (3 files) | `sfx.unit.worker.attack_horn` | sounds the attack | the attack warning of the MVP, two-dimensional (MVP-087) | mono, 48 kHz; positional, follows the source; one-shot; target duration about 3.0-4.4 s | 3 variants | mvp |

### 13.9 Leader (23 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/leader/leader_spawn_{01..06}.wav` (6 files) | `sfx.unit.leader.spawn` | appears | `worker.ordered`, `leader.order_waypoint (layer 2)`, `leader.selected`, `leader.order_formation (layer 2)` | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.2-0.6 s | 6 variants | skirmish-first-playable |
| `GameData/Audio/sfx/leader/leader_order_waypoint_{01..17}.wav` (17 files) | `sfx.unit.leader.order_waypoint` | is given a waypoint order | `leader.order_formation` | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 3.0 s | 17 variants | skirmish-first-playable |

### 13.10 ant (27 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/ant/ant_bite_{01..06}.wav` (6 files) | `sfx.creature.ant.attack` | attacks | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.1-0.2 s | 6 variants | launch |
| `GameData/Audio/sfx/ant/ant_presence_loop_{01..08}.wav` (8 files) | `sfx.creature.ant.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 0.1-0.2 s | 8 variants | launch |
| `GameData/Audio/sfx/ant/ant_hurt_{01..07}.wav` (7 files) | `sfx.creature.ant.hurt` | takes damage | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.1-0.4 s | 7 variants | launch |
| `GameData/Audio/sfx/ant/ant_die_{01..06}.wav` (6 files) | `sfx.creature.ant.die` | dies | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.4-0.8 s | 6 variants | launch |

### 13.11 Transport (30 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/transport/transport_presence_loop.wav` | `sfx.unit.transport.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 17.8 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/transport/transport_spawn_{01..06}.wav` (6 files) | `sfx.unit.transport.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 3.0-3.3 s | 6 variants | skirmish-first-playable |
| `GameData/Audio/sfx/transport/transport_hurt_{01..10}.wav` (10 files) | `sfx.unit.transport.hurt` | takes damage | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 3.1-5.4 s | 10 variants | skirmish-first-playable |
| `GameData/Audio/sfx/transport/transport_die_{01..09}.wav` (9 files) | `sfx.unit.transport.die` | dies | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 3.5-5.4 s | 9 variants | skirmish-first-playable |
| `GameData/Audio/sfx/transport/transport_ordered_{01..04}.wav` (4 files) | `sfx.unit.transport.ordered` | receives an order | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 4.5-4.9 s | 4 variants | skirmish-first-playable |

### 13.12 Reaper Serpent (47 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/reaper_serpent/reaper_serpent_presence_loop_{01..11}.wav` (11 files) | `sfx.creature.reaper_serpent.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 0.8-2.1 s | 11 variants | launch |
| `GameData/Audio/sfx/reaper_serpent/reaper_serpent_hurt_{01..10}.wav` (10 files) | `sfx.creature.reaper_serpent.hurt` | takes damage | `reaper_serpent.spawn (layer 2)` | mono, 48 kHz; positional, follows the source; one-shot; target duration about 1.6-3.4 s | 10 variants | launch |
| `GameData/Audio/sfx/reaper_serpent/reaper_serpent_die_{01..10}.wav` (10 files) | `sfx.creature.reaper_serpent.die` | dies | `seed_pod.die` | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.2-3.5 s | 10 variants | launch |
| `GameData/Audio/sfx/reaper_serpent/reaper_serpent_attack_{01..07}.wav` (7 files) | `sfx.creature.reaper_serpent.attack` | attacks | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 1.4-2.2 s | 7 variants | launch |
| `GameData/Audio/sfx/reaper_serpent/reaper_serpent_enemy_sighted_{01..09}.wav` (9 files) | `sfx.creature.reaper_serpent.enemy_sighted` | sights an enemy | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 1.2-3.1 s | 9 variants | launch |

### 13.13 Seed Pod (20 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/seed_pod/seed_pod_spawn_{01..05}.wav` (5 files) | `sfx.creature.seed_pod.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.9-1.6 s | 5 variants | launch |
| `GameData/Audio/sfx/seed_pod/seed_pod_bounce_{01..05}.wav` (5 files) | `sfx.creature.seed_pod.bounce` | bounces | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 1.2-1.3 s | 5 variants | launch |
| `GameData/Audio/sfx/seed_pod/seed_pod_burst_open_{01..10}.wav` (10 files) | `sfx.creature.seed_pod.burst_open` | bursts open | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 2.0-2.6 s | 10 variants | launch |

### 13.14 Abductor (12 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/abductor/abductor_presence_loop_b.wav` | `sfx.creature.abductor.presence_loop_b` | loop while it exists (additional layer) | `collector.spawn` | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 1.4 s | 1 variant | launch |
| `GameData/Audio/sfx/abductor/abductor_presence_loop_c.wav` | `sfx.creature.abductor.presence_loop_c` | loop while it exists (additional layer) | `collector.spawn (layer 2)` | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 6.6 s | 1 variant | launch |
| `GameData/Audio/sfx/abductor/abductor_presence_loop_{01..09}.wav` (9 files) | `sfx.creature.abductor.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 15.1-23.5 s | 9 variants | launch |
| `GameData/Audio/sfx/abductor/abductor_tractor_beam.wav` | `sfx.creature.abductor.tractor_beam` | tractor beam loop | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 4.0 s | 1 variant | launch |

### 13.15 Collector (7 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/collector/collector_presence_loop.wav` | `sfx.unit.collector.presence_loop` | loop while it exists | `meteor.spawn (layer 2)`, `abductor.spawn (layer 4)` | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 1.4 s | 1 variant | launch |
| `GameData/Audio/sfx/collector/collector_drop_essence_{01..06}.wav` (6 files) | `sfx.unit.collector.drop_essence` | drops Essence orbs | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 1.1-1.6 s | 6 variants | launch |

### 13.16 Missile Barrage missile (3 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/barrage_missile/barrage_missile_presence_loop.wav` | `sfx.weapon.barrage_missile.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 3.2 s | 1 variant | launch |
| `GameData/Audio/sfx/barrage_missile/barrage_missile_explode.wav` | `sfx.weapon.barrage_missile.explode` | explodes | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 7.3 s | 1 variant | launch |
| `GameData/Audio/sfx/barrage_missile/barrage_missile_explode_b.wav` | `sfx.weapon.barrage_missile.explode_b` | explodes (additional layer) | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 10.1 s | 1 variant | launch |

### 13.17 meteor (3 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/meteor/meteor_land.wav` | `sfx.weapon.meteor.land` | lands | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 7.3 s | 1 variant | launch |
| `GameData/Audio/sfx/meteor/meteor_land_b.wav` | `sfx.weapon.meteor.land_b` | lands (additional layer) | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 5.8 s | 1 variant | launch |
| `GameData/Audio/sfx/meteor/meteor_presence_loop.wav` | `sfx.weapon.meteor.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 3.2 s | 1 variant | launch |

### 13.18 Beam Dish (11 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/beam_dish/beam_dish_rotate_start.wav` | `sfx.building.beam_dish.rotate_start` | starts turning | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 1.5 s | 1 variant | launch |
| `GameData/Audio/sfx/beam_dish/beam_dish_rotate_start_b.wav` | `sfx.building.beam_dish.rotate_start_b` | starts turning (additional layer) | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 0.7 s | 1 variant | launch |
| `GameData/Audio/sfx/beam_dish/beam_dish_rotate_stop.wav` | `sfx.building.beam_dish.rotate_stop` | stops turning | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 3.2 s | 1 variant | launch |
| `GameData/Audio/sfx/beam_dish/beam_dish_link_up_{01..04}.wav` (4 files) | `sfx.building.beam_dish.link_up` | link established | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 0.6-1.8 s | 4 variants | launch |
| `GameData/Audio/sfx/beam_dish/beam_dish_link_lost_{01..04}.wav` (4 files) | `sfx.building.beam_dish.link_lost` | link lost | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 1.1-1.8 s | 4 variants | launch |

### 13.19 Barrier Emitter (23 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/barrier_emitter/barrier_emitter_presence_loop.wav` | `sfx.building.barrier_emitter.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 0.1 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/barrier_emitter/barrier_emitter_spark_{01..06}.wav` (6 files) | `sfx.building.barrier_emitter.spark` | sparks | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.8-2.0 s | 6 variants | skirmish-first-playable |
| `GameData/Audio/sfx/barrier_emitter/barrier_emitter_damaged_{01..16}.wav` (16 files) | `sfx.building.barrier_emitter.damaged` | is damaged | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.7-2.5 s | 16 variants | skirmish-first-playable |

### 13.20 turret (16 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/turret/turret_spawn_{01..06}.wav` (6 files) | `sfx.building.turret.spawn` | appears | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 5.1-6.9 s | 6 variants | skirmish-first-playable |
| `GameData/Audio/sfx/turret/turret_fire_flame_{01..10}.wav` (10 files) | `sfx.building.turret.fire_flame` | fires flame | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 0.4-1.0 s | 10 variants | skirmish-first-playable |

### 13.21 tree (41 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/tree/tree_ambient_normal_{01..08}.wav` (8 files) | `sfx.building.tree.ambient_normal` | ambient loop | `tree.ambient_essence` | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 8.0 s | 8 variants | skirmish-first-playable |
| `GameData/Audio/sfx/tree/tree_burning_{01..15}.wav` (15 files) | `sfx.building.tree.burning` | burns | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 0.4-13.2 s | 15 variants | skirmish-first-playable |
| `GameData/Audio/sfx/tree/tree_spawn.wav` | `sfx.building.tree.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 12.1 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/tree/tree_ambient_blight_{01..17}.wav` (17 files) | `sfx.building.tree.ambient_blight` | Blight Grove ambient loop | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 2.1-6.7 s | 17 variants | skirmish-first-playable |

### 13.22 grid pylon (22 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/grid_pylon/grid_pylon_surge_{01..22}.wav` (22 files) | `sfx.building.grid_pylon.surge` | energy surge passes | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.2-0.6 s | 22 variants | launch |

### 13.23 Solar Array (6 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/solar_array/solar_array_surge_{01..05}.wav` (5 files) | `sfx.building.solar_array.surge` | energy surge passes | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.1-1.4 s | 5 variants | launch |
| `GameData/Audio/sfx/solar_array/solar_array_charging.wav` | `sfx.building.solar_array.charging` | charging loop | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 3.7 s | 1 variant | launch |

### 13.24 Hive (10 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/hive/hive_damaged_{01..10}.wav` (10 files) | `sfx.building.hive.damaged` | is damaged | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.7-4.5 s | 10 variants | launch |

### 13.25 grenade (17 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/grenade/grenade_explode_{01..05}.wav` (5 files) | `sfx.weapon.grenade.explode` | explodes | `hive.explode` | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 3.7-4.1 s | 5 variants | skirmish-first-playable |
| `GameData/Audio/sfx/grenade/grenade_spawn_{01..05}.wav` (5 files) | `sfx.weapon.grenade.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 3.3 s | 5 variants | skirmish-first-playable |
| `GameData/Audio/sfx/grenade/grenade_fuse_blink.wav` | `sfx.weapon.grenade.fuse_blink` | fuse blink | `airstrike_bomb.fuse_blink` | mono, 48 kHz; positional, follows the source; one-shot; target duration about 1.0 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/grenade/grenade_bounce_{01..06}.wav` (6 files) | `sfx.weapon.grenade.bounce` | bounces | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.3-0.8 s | 6 variants | skirmish-first-playable |

### 13.26 Spitter Plant (3 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/spitter_plant/spitter_plant_burning_{01..03}.wav` (3 files) | `sfx.building.spitter_plant.burning` | burns | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 17.8-18.0 s | 3 variants | launch |

### 13.27 Spawn Node (2 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/spawn_node/spawn_node_activate.wav` | `sfx.building.spawn_node.activate` | activates | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 9.4 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/spawn_node/spawn_node_deactivate.wav` | `sfx.building.spawn_node.deactivate` | deactivates | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 7.9 s | 1 variant | skirmish-first-playable |

### 13.28 fuel pump (10 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/fuel_pump/fuel_pump_pump_up.wav` | `sfx.building.fuel_pump.pump_up` | pump stroke up | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 5.6 s | 1 variant | launch |
| `GameData/Audio/sfx/fuel_pump/fuel_pump_pump_up_b_{01..05}.wav` (5 files) | `sfx.building.fuel_pump.pump_up_b` | pump stroke up (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.1-5.1 s | 5 variants | launch |
| `GameData/Audio/sfx/fuel_pump/fuel_pump_pump_down.wav` | `sfx.building.fuel_pump.pump_down` | pump stroke down | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.9 s | 1 variant | launch |
| `GameData/Audio/sfx/fuel_pump/fuel_pump_pump_down_b_{01..03}.wav` (3 files) | `sfx.building.fuel_pump.pump_down_b` | pump stroke down (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.0-3.0 s | 3 variants | launch |

### 13.29 fuel line (1 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/fuel_line/fuel_line_flow.wav` | `sfx.building.fuel_line.flow` | fuel flow loop | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 4.2 s | 1 variant | launch |

### 13.30 fuel depot (5 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/fuel_depot/fuel_depot_load_passenger_{01..05}.wav` (5 files) | `sfx.building.fuel_depot.load_passenger` | loads a passenger | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 0.5-0.9 s | 5 variants | launch |

### 13.31 Launch Vessel (25 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/launch_vessel/launch_vessel_fuelled_{01..05}.wav` (5 files) | `sfx.building.launch_vessel.fuelled` | fuelled loop | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 3.1-3.6 s | 5 variants | launch |
| `GameData/Audio/sfx/launch_vessel/launch_vessel_fuelled_b.wav` | `sfx.building.launch_vessel.fuelled_b` | fuelled loop (additional layer) | `match_ui.lobby_bed (layer 2)` | mono, 48 kHz; positional, following the source, as the fuelled layer; 2-D as a layer of the lobby bed; seamless loop; target duration about 1.4 s | 1 variant | launch |
| `GameData/Audio/sfx/launch_vessel/launch_vessel_malfunction.wav` | `sfx.building.launch_vessel.malfunction` | malfunction loop | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 1.2 s | 1 variant | launch |
| `GameData/Audio/sfx/launch_vessel/launch_vessel_explode.wav` | `sfx.building.launch_vessel.explode` | explodes | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 5.3 s | 1 variant | launch |
| `GameData/Audio/sfx/launch_vessel/launch_vessel_damaged_{01..02}.wav` (2 files) | `sfx.building.launch_vessel.damaged` | is damaged | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 3.7-4.0 s | 2 variants | launch |
| `GameData/Audio/sfx/launch_vessel/launch_vessel_engine_burn.wav` | `sfx.building.launch_vessel.engine_burn` | engine burn | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 3.3 s | 1 variant | launch |
| `GameData/Audio/sfx/launch_vessel/launch_vessel_engine_burn_b.wav` | `sfx.building.launch_vessel.engine_burn_b` | engine burn (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.6 s | 1 variant | launch |
| `GameData/Audio/sfx/launch_vessel/launch_vessel_hungry_{01..07}.wav` (7 files) | `sfx.building.launch_vessel.hungry` | needs fuel loop | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 4.5-7.3 s | 7 variants | launch |
| `GameData/Audio/sfx/launch_vessel/launch_vessel_flight_{01..06}.wav` (6 files) | `sfx.building.launch_vessel.flight` | launch flight loop | `core_device.spawn` | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 5.8-6.3 s | 6 variants | launch |

### 13.32 Control Zone (6 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/control_zone/control_zone_relic_scored.wav` | `sfx.building.control_zone.relic_scored` | a Relic is scored | `relic.owner_changed` | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 1.2 s | 1 variant | launch |
| `GameData/Audio/sfx/control_zone/control_zone_relic_scored_b.wav` | `sfx.building.control_zone.relic_scored_b` | a Relic is scored (additional layer) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 4.6 s | 1 variant | launch |
| `GameData/Audio/sfx/control_zone/control_zone_relic_scored_c.wav` | `sfx.building.control_zone.relic_scored_c` | a Relic is scored (additional layer) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 3.0 s | 1 variant | launch |
| `GameData/Audio/sfx/control_zone/control_zone_owner_changed.wav` | `sfx.building.control_zone.owner_changed` | changes owner | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.0 s | 1 variant | launch |
| `GameData/Audio/sfx/control_zone/control_zone_owner_changed_b.wav` | `sfx.building.control_zone.owner_changed_b` | changes owner (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.8 s | 1 variant | launch |
| `GameData/Audio/sfx/control_zone/control_zone_presence_loop.wav` | `sfx.building.control_zone.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 5.4 s | 1 variant | launch |

### 13.33 Command Console (4 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/command_console/command_console_presence_loop_{01..04}.wav` (4 files) | `sfx.building.command_console.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 7.1-7.9 s | 4 variants | launch |

### 13.34 Supply crate (18 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/supply_crate/supply_crate_land.wav` | `sfx.crate.supply_crate.land` | lands | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 4.0 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_capture_begin.wav` | `sfx.crate.supply_crate.capture_begin` | capture starts | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 4.6 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_capture_begin_b.wav` | `sfx.crate.supply_crate.capture_begin_b` | capture starts (additional layer) | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 8.0 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_capture_begin_c.wav` | `sfx.crate.supply_crate.capture_begin_c` | capture starts (additional layer) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 5.3 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_capture_lost.wav` | `sfx.crate.supply_crate.capture_lost` | capture lost | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.5 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_capture_progress_00.wav` | `sfx.crate.supply_crate.capture_progress_00` | capture under way, 0-20 % | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 16.5 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_capture_progress_20.wav` | `sfx.crate.supply_crate.capture_progress_20` | capture under way, 20-40 % | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 11.9 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_capture_progress_40.wav` | `sfx.crate.supply_crate.capture_progress_40` | capture under way, 40-60 % | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 15.4 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_capture_progress_60.wav` | `sfx.crate.supply_crate.capture_progress_60` | capture under way, 60-80 % | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 12.4 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_capture_progress_80.wav` | `sfx.crate.supply_crate.capture_progress_80` | capture under way, 80-100 % | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 14.8 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_captured.wav` | `sfx.crate.supply_crate.captured` | is captured | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 3.6 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/supply_crate/supply_crate_falling_{01..07}.wav` (7 files) | `sfx.crate.supply_crate.falling` | falls from the sky | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 12.4-13.3 s | 7 variants | skirmish-first-playable |

### 13.35 Relic (3 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/relic/relic_owner_changed.wav` | `sfx.building.relic.owner_changed` | changes owner | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.8 s | 1 variant | launch |
| `GameData/Audio/sfx/relic/relic_drop.wav` | `sfx.building.relic.drop` | is dropped | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 5.9 s | 1 variant | launch |
| `GameData/Audio/sfx/relic/relic_spawn.wav` | `sfx.building.relic.spawn` | appears | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 1.3 s | 1 variant | launch |

### 13.36 laser bolt (64 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/laser_bolt/laser_bolt_hit_bark_{01..36}.wav` (36 files) | `sfx.weapon.laser_bolt.hit_bark` | a unit is hit by a laser bolt (voice of the hit unit) | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.2-0.8 s | 36 variants | skirmish-first-playable |
| `GameData/Audio/sfx/laser_bolt/laser_bolt_spawn_{01..10}.wav` (10 files) | `sfx.weapon.laser_bolt.spawn` | every laser bolt fired (DEC-564) | — | mono, 48 kHz; positional; one-shot; target duration about 0.2-0.7 s; variant count proposed (DEC-564) | 10 variants | mvp |
| `GameData/Audio/sfx/laser_bolt/laser_bolt_hit_structure_{01..10}.wav` (10 files) | `sfx.weapon.laser_bolt.hit_structure` | a bolt hits a structure (DEC-564) | — | mono, 48 kHz; positional; one-shot; target duration about 0.3-1.1 s; variant count proposed (DEC-564) | 10 variants | skirmish-first-playable |
| `GameData/Audio/sfx/laser_bolt/laser_bolt_kill_{01..08}.wav` (8 files) | `sfx.weapon.laser_bolt.kill` | a laser bolt or a turret round kills an actor (DEC-564) | — | mono, 48 kHz; positional; one-shot; target duration about 0.2-0.8 s; variant count proposed (DEC-564) | 8 variants | skirmish-first-playable |

### 13.37 rocket (6 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/rocket/rocket_explode_{01..04}.wav` (4 files) | `sfx.weapon.rocket.explode` | explodes | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 4.3-5.7 s | 4 variants | skirmish-first-playable |
| `GameData/Audio/sfx/rocket/rocket_presence_loop.wav` | `sfx.weapon.rocket.presence_loop` | loop while it exists | — | mono, 48 kHz; positional, follows the source; seamless loop; target duration about 0.5 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/rocket/rocket_spawn.wav` | `sfx.weapon.rocket.spawn` | appears | — | mono, 48 kHz; positional, follows the source; one-shot; target duration about 0.8 s | 1 variant | skirmish-first-playable |

### 13.38 airstrike bomb (5 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/airstrike_bomb/airstrike_bomb_explode_{01..05}.wav` (5 files) | `sfx.weapon.airstrike_bomb.explode` | explodes | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 5.8-6.1 s | 5 variants | skirmish-first-playable |

### 13.39 Essence orb (1 files)

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/essence_orb/essence_orb_collected.wav` | `sfx.unit.essence_orb.collected` | is collected | an orb becoming a Worker at its Wellspring (MVP-087) | mono, 48 kHz; positional, follows the source; one-shot; target duration about 2.0 s | 1 variant | mvp |

### 13.40 Supply-crate outcome stings (31 files)

The 25 stings that a match raises are bound now; the gun-turret sting also plays for the rocket turret and flame turret outcomes, and the airstrike sting for the Firebomb Run. The last six rows are stings of their own for the rocket turret, flame turret, Firebomb Run, Builder, Time Dilation and Time Rush outcomes, which play only if DEC-565 binds them (R-AUD-139).

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/crate_outcome/outcome_strike_team.wav` | `sfx.crate.outcome.strike_team` | Supply crate yields Strike team | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.0 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/crate_outcome/outcome_collector.wav` | `sfx.crate.outcome.collector` | Supply crate yields Collector | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 6.0 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_transport.wav` | `sfx.crate.outcome.transport` | Supply crate yields Transport | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 3.5 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/crate_outcome/outcome_reinforcements.wav` | `sfx.crate.outcome.reinforcements` | Supply crate yields Reinforcements | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 6.6 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/crate_outcome/outcome_hive.wav` | `sfx.crate.outcome.hive` | Supply crate yields Hive | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 5.2 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_brood.wav` | `sfx.crate.outcome.brood` | Supply crate yields Brood | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 3.1 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_airstrike.wav` | `sfx.crate.outcome.airstrike` | Supply crate yields airstrike; the Firebomb Run outcome plays it too until DEC-565 binds a sting of its own (R-AUD-139) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 4.8 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/crate_outcome/outcome_essence_grove.wav` | `sfx.crate.outcome.essence_grove` | Supply crate yields Essence Grove | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 4.0 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_blight_grove.wav` | `sfx.crate.outcome.blight_grove` | Supply crate yields Blight Grove | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 5.2 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_supply_storm.wav` | `sfx.crate.outcome.supply_storm` | Supply crate yields Supply Storm | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 4.2 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_gun_turret.wav` | `sfx.crate.outcome.gun_turret` | Supply crate yields gun turret; the rocket turret and flame turret outcomes play it too until DEC-565 binds a sting of their own (R-AUD-139) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 5.0 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/crate_outcome/outcome_meteor_shower.wav` | `sfx.crate.outcome.meteor_shower` | Supply crate yields meteor shower | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 4.6 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_speed_boost.wav` | `sfx.crate.outcome.speed_boost` | Supply crate yields Speed Boost | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.9 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/crate_outcome/outcome_missile_barrage.wav` | `sfx.crate.outcome.missile_barrage` | Supply crate yields Missile Barrage | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 8.1 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_sickness.wav` | `sfx.crate.outcome.sickness` | Supply crate yields Sickness | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 6.9 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_allegiance_scrambler.wav` | `sfx.crate.outcome.allegiance_scrambler` | Supply crate yields Allegiance Scrambler | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 3.2 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_shield.wav` | `sfx.crate.outcome.shield` | Supply crate yields Shield | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 3.1 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/crate_outcome/outcome_convert.wav` | `sfx.crate.outcome.convert` | Supply crate yields Convert | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 8.2 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_spawn_surge.wav` | `sfx.crate.outcome.spawn_surge` | Supply crate yields Spawn Surge | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 8.1 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_frenzy.wav` | `sfx.crate.outcome.frenzy` | Supply crate yields Frenzy | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 5.7 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_rocket_troops.wav` | `sfx.crate.outcome.rocket_troops` | Supply crate yields Rocket Troops | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 3.6 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_swarm.wav` | `sfx.crate.outcome.swarm` | Supply crate yields Swarm | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 4.0 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_fury.wav` | `sfx.crate.outcome.fury` | Supply crate yields Fury | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 4.3 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/sfx/crate_outcome/outcome_spitter_plants.wav` | `sfx.crate.outcome.spitter_plants` | Supply crate yields Spitter Plants | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 3.3 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_abductor.wav` | `sfx.crate.outcome.abductor` | Supply crate yields Abductor | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 6.3 s | 1 variant | launch |
| `GameData/Audio/sfx/crate_outcome/outcome_rocket_turret.wav` | `sfx.crate.outcome.rocket_turret` | Supply crate yields rocket turret; bound only if DEC-565 binds it (R-AUD-139) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.0-8.2 s; variant count proposed (DEC-565) | 1 variant | post-launch |
| `GameData/Audio/sfx/crate_outcome/outcome_flame_turret.wav` | `sfx.crate.outcome.flame_turret` | Supply crate yields flame turret; bound only if DEC-565 binds it (R-AUD-139) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.0-8.2 s; variant count proposed (DEC-565) | 1 variant | post-launch |
| `GameData/Audio/sfx/crate_outcome/outcome_firebomb_run.wav` | `sfx.crate.outcome.firebomb_run` | Supply crate yields Firebomb Run; bound only if DEC-565 binds it (R-AUD-139) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.0-8.2 s; variant count proposed (DEC-565) | 1 variant | post-launch |
| `GameData/Audio/sfx/crate_outcome/outcome_builder.wav` | `sfx.crate.outcome.builder` | Supply crate yields Builder; bound only if DEC-565 binds it (R-AUD-139) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.0-8.2 s; variant count proposed (DEC-565) | 1 variant | post-launch |
| `GameData/Audio/sfx/crate_outcome/outcome_time_dilation.wav` | `sfx.crate.outcome.time_dilation` | Supply crate yields Time Dilation; bound only if DEC-565 binds it (R-AUD-139) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.0-8.2 s; variant count proposed (DEC-565) | 1 variant | post-launch |
| `GameData/Audio/sfx/crate_outcome/outcome_time_rush.wav` | `sfx.crate.outcome.time_rush` | Supply crate yields Time Rush; bound only if DEC-565 binds it (R-AUD-139) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.0-8.2 s; variant count proposed (DEC-565) | 1 variant | post-launch |

### 13.41 dwelling (1 files)

The MVP's structure-complete group (MVP-087): it plays when a settlement completes a dwelling, a storehouse or a turret, together with the completion flash of `fx.starburst` (MVP-083).

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/sfx/dwelling/dwelling_complete.wav` | `sfx.building.dwelling.complete` | is completed (MVP-033, MVP-087) | the storehouse and the turret when they are completed | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 1.5-2.5 s; variant count proposed (MVP-087) | 1 variant | mvp |

## 14 UI sounds

Interface sounds: menus, selection, countdown, chat, the power-slot panel, the game-over announcement, and the MVP's battle start and end (MVP-087). They are 2-D (not positional) except the unit highlight. Formats: WAV 48 kHz 24-bit → 16-bit PCM resident. Placeholder: synthesised tone.

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/ui/ui_countdown_tick.wav` | `sfx.ui.match_ui.countdown_tick` | countdown tick | `turret.kill_tick`, `match_ui.countdown_zero` | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 0.1 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ui/ui_power_input_begin.wav` | `sfx.ui.power_input.begin` | power input starts | `match_ui.selection_circle` | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 3.5 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ui/ui_power_input_confirm.wav` | `sfx.ui.power_input.confirm` | power input succeeds | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.2 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ui/ui_power_slot_discard.wav` | `sfx.ui.power_slots.discard` | a power is discarded | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.5 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ui/ui_power_slot_icon_hover_{01..02}.wav` (2 files) | `sfx.ui.power_slots.icon_hover` | pointer over an icon | `match_ui.menu_hover` | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 0.1 s | 2 variants | skirmish-first-playable |
| `GameData/Audio/ui/ui_selection_circle.wav` | `sfx.ui.match_ui.selection_circle` | selection circle drawn | — | mono or stereo, 48 kHz; 2-D (not positional); seamless loop; target duration about 6.0 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ui/ui_menu_select.wav` | `sfx.ui.match_ui.menu_select` | menu item chosen | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 1.9 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ui/ui_menu_back.wav` | `sfx.ui.match_ui.menu_back` | menu back | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.2 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ui/ui_unit_highlight_{01..02}.wav` (2 files) | `sfx.ui.match_ui.unit_highlight` | unit highlighted | — | mono, 48 kHz; positional, fixed where it started; one-shot; target duration about 0.1 s | 2 variants | skirmish-first-playable |
| `GameData/Audio/ui/ui_unit_select.wav` | `sfx.ui.match_ui.unit_select` | unit selected | `match_ui.toggle_state` | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 0.1 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ui/ui_error.wav` | `sfx.ui.match_ui.error` | invalid action | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 0.8 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ui/ui_chat_message.wav` | `sfx.ui.match_ui.chat_message` | chat message arrives | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 0.2 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ui/ui_game_over_{01..02}.wav` (2 files) | `sfx.ui.match_ui.game_over` | a result is declared: the game-over announcement (DEC-097, R-AUD-307) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; announcer bus; target duration about 4.3-4.4 s; variant count proposed (DEC-097) | 2 variants | skirmish-first-playable |
| `GameData/Audio/ui/ui_battle_start.wav` | `sfx.ui.battle.start` | a battle starts (MVP-043, MVP-087) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.0-3.0 s; variant count proposed (MVP-087) | 1 variant | mvp |
| `GameData/Audio/ui/ui_battle_end.wav` | `sfx.ui.battle.end` | a battle ends in its battle result (MVP-045, MVP-048, MVP-087) | — | mono or stereo, 48 kHz; 2-D (not positional); one-shot; target duration about 2.0-3.0 s; variant count proposed (MVP-087) | 1 variant | mvp |

## 15 Ambience

Ambience beds: 2-D loops under the match and under the lobby. Each bed has layers that play together. Formats: WAV 48 kHz 24-bit → 16-bit PCM resident, or an MS-ADPCM stream, decoded by XAudio2, for every member of a group whose longest member is over 20 s (AEN-072, ADR-025). Placeholder: silence.

| Files | Asset id | Event | Also bound to | Spec | Variants | Priority |
|---|---|---|---|---|---|---|
| `GameData/Audio/ambience/amb_match_bed_{01..03}.wav` (3 files) | `amb.match_bed` | match bed | `match_ui.lobby_bed` | mono or stereo, 48 kHz; 2-D (not positional); seamless loop; target duration about 22.1-53.9 s | 3 variants | skirmish-first-playable |
| `GameData/Audio/ambience/amb_match_bed_b.wav` | `amb.match_bed_b` | match bed (additional layer) | — | mono or stereo, 48 kHz; 2-D (not positional); seamless loop; target duration about 29.0 s | 1 variant | skirmish-first-playable |
| `GameData/Audio/ambience/amb_match_bed_c_{01..07}.wav` (7 files) | `amb.match_bed_c` | match bed (additional layer) | — | mono or stereo, 48 kHz; 2-D (not positional); seamless loop; target duration about 15.3-39.6 s | 7 variants | skirmish-first-playable |
| `GameData/Audio/ambience/amb_lobby_bed_{01..10}.wav` (10 files) | `amb.lobby_bed` | lobby ambience bed | — | mono or stereo, 48 kHz; 2-D (not positional); seamless loop; target duration about 19.4-36.1 s | 10 variants | launch |

## 16 Music

No rule raises a music event, so no music plays at the front end, in the lobby or in a match (DEC-049). Species commissions new music. The victory and defeat stings are bound to the match end now (R-AUD-307, DEC-097). The front-end theme and the in-match beds are the proposed minimum of option A of DEC-049 and are authored only if DEC-049 selects it, so their rows are `post-launch`; the design phase then sets the final list, its triggers and the music bus (R-AUD-407). The style follows the look: sparse, electronic, cool timbres that leave room for the crowd and the glow; no pastiche of any existing score.

| File | Asset id | Role | Spec | Priority | Placeholder |
|---|---|---|---|---|---|
| `GameData/Audio/music/front_end_theme.wav` | `music.front_end.theme` | Front-end theme, loops under the menus and the lobby; only if DEC-049 selects option A | stereo, 48 kHz; seamless loop; 2-4 min | post-launch | silence |
| `GameData/Audio/music/match_bed_01.wav` | `music.match.bed` | In-match music bed, low intensity, sits under the crowd and the glow; only if DEC-049 selects option A | stereo, 48 kHz; seamless loop; 3-5 min; mixed below the effects bus | post-launch | silence |
| `GameData/Audio/music/match_bed_02.wav` | `music.match.bed` | In-match music bed, low intensity, sits under the crowd and the glow | stereo, 48 kHz; seamless loop; 3-5 min | post-launch | silence |
| `GameData/Audio/music/match_bed_03.wav` | `music.match.bed` | In-match music bed, low intensity, sits under the crowd and the glow | stereo, 48 kHz; seamless loop; 3-5 min | post-launch | silence |
| `GameData/Audio/music/sting_victory.wav` | `music.match.sting_victory` | Victory sting at the end of a match | stereo, 48 kHz; one-shot; 5-10 s | launch | silence |
| `GameData/Audio/music/sting_defeat.wav` | `music.match.sting_defeat` | Defeat sting at the end of a match | stereo, 48 kHz; one-shot; 5-10 s | launch | silence |

## 17 Fonts

Species draws its text from **bitmap faces drawn for Species** (O56), and since O62 they are the owner's two faces: **GameFont**, the game face, draws every text that a player sees, in the Title, Body and Caption roles; **EditorFont**, the development face, draws the development overlay, the development windows and the text of the development drawing queue, in Debug builds only ([ui_framework](../architecture/components/ui_framework/layout_rendering_input_and_localisation.md) UI-195 amendment of O62). Both are newly authored like every other asset (§1); no licensed typeface is needed, and the reference game's fonts are not used (O9). Each face is one **glyph sheet** and a **glyph table**; the font packer under `Tools/` turns them into one `R8_UNORM` atlas page and a cooked glyph table, and the UI draws text at whole-number scales of the sheet with the point sampler, so the drawn pixels stay crisp (O63; [ADR-034](../ADR/ADR-034-text-rendering-and-glyph-cache.md) as amended; UI-198). `font.ui.compact` and `font.ui.stylised` are optional later faces for the Body and Caption roles. The Japanese and Korean companions are font files for the DirectWrite upgrade of ADR-034, which also brings Cyrillic and the currency signs. Placeholder: a generated block font for the drawn faces, the fallback font for the companions (§2.6).

| Files | Asset id | Role | Coverage | Priority |
|---|---|---|---|---|
| `GameData/Fonts/GameFont.dds`, `GameData/Fonts/GameFontGlyphs.json` | `font.ui.primary` | Game face GameFont: the Title, Body and Caption roles (UI-195), every player-facing text | U+0020-007E, U+00BF-00FF; U+00A0 as the blank cell | mvp |
| `GameData/Fonts/EditorFont.dds`, `GameData/Fonts/EditorFontGlyphs.json` | `font.dev.editor` | Development face EditorFont: the Development role, in Debug builds only (DBG-095 amendment) | U+0020-007E, U+00BF-00FF; U+00A0 as the blank cell | mvp |
| `GameData/Fonts/ui_compact.png`, `GameData/Fonts/ui_compact_glyphs.json` | `font.ui.compact` | An optional later body face (`FontRole::Body`); until it exists the role uses `font.ui.primary` | at least that of `font.ui.primary` | launch |
| `GameData/Fonts/ui_stylised.png`, `GameData/Fonts/ui_stylised_glyphs.json` | `font.ui.stylised` | An optional later caption face (`FontRole::Caption`: tool-window titles, the session-browser header, player names in rows, overlays); until it exists the role uses `font.ui.primary` | at least that of `font.ui.primary` | launch |
| `GameData/Fonts/ui_primary_ja.otf` | `font.ui.primary_ja` | Japanese companion of the primary face (kana and CJK ideographs), for the DirectWrite upgrade | U+3000-30FF, U+4E00-9EFF | launch |
| `GameData/Fonts/ui_primary_ko.otf` | `font.ui.primary_ko` | Korean companion of the primary face (Hangul syllables), for the DirectWrite upgrade | U+AC00-D7FF | launch |

**The glyph sheet.** The owner's sheets are 32-bit DDS files of 256 x 224 px: white, with the coverage in the alpha channel (0 or 255), on a grid of 16 x 16 px cells, 16 to a row and 14 rows, the top-left cell U+0020 and each next cell the next code point, up to U+00FF. The capitals of both faces stand on rows 1 to 12 of a cell and the descenders reach row 14, so the baseline is 13 px below the cell's top; both faces are monospaced on the cell, as drawn. Most of GameFont's glyphs are drawn in blocks of 2 x 2 px, and EditorFont's in single pixels. EditorFont's cells 0x80 to 0xBF hold accented letters in a second order, which duplicates its letters at U+00C0 to U+00FF, so its glyph table leaves them unmapped and takes only U+00BF from them (cell 0xA8). A later face may be drawn the same way or as an 8-bit greyscale PNG.

**The glyph table** is UTF-8 JSON validated by its schema (ADR-062, DAT-219): the design size, line height, baseline and cell size, and per code point its cell, ink width, bearing and advance, in pixels at the scale 1, with an optional kerning-pair table. A cell may hold any code point that the table names, so a glyph drawn into a free cell can be mapped without moving the others. The tables of the owner's faces are generated from the sheets' grid ([MVP assets](../implementation/mvp_assets.md) §6): every glyph has the cell width of 16 px as its advance and its ink width, and a bearing of 0, so the whole cell is drawn.

**The coverage** is what the glyph tables map, and a content-build check walks the en-GB string table against the game face and the development string table against the development face, failing on a missing glyph ([ui_framework](../architecture/components/ui_framework/layout_rendering_input_and_localisation.md) UI-199 amendment of O62). Neither face has the eight general-punctuation marks (U+2013, U+2014, U+2018, U+2019, U+201C, U+201D, U+2022, U+2026) or U+00A1 to U+00BE, among them £ and °; until the owner draws any of them into free cells (0x7F to 0x9F of GameFont are empty), the en-GB strings use ASCII punctuation. Cyrillic and the currency block wait for the DirectWrite upgrade.

**No licence is needed for the drawn faces**: they are Species' own content (O56, O62), and the likeness review covers them as it covers other drawn art (PIL-027). The Japanese and Korean companions, when the upgrade brings them, and the open-licence family of the fallback-font placeholder are third-party content under a licence: each needs the owner's approval before its files land under `GameData/Fonts/`, and its licence text travels with the font files (`AGENTS.md` R14). The MVP needs the game face and the development face (`mvp`); the other faces are optional (`launch`), and the companions wait for those languages and the upgrade.

**Sizes.** A text is drawn at the whole-number scale k nearest its final size ÷ 16 px, rounded half up, at least 1, and raised until it meets the accessibility minimum: 16 px covers the minimum of 9 px at 1080p, and at 2160p, whose minimum is 19 px, text draws at 32 px or more (ADR-064, UI-171). The darkening outline pass draws behind the glyphs at every scale.

**Amendment 2026-10-19 (owner decisions O62 and O63).** This section replaces the two faces that O56 asked the owner to draw, a 24 px title face and a 12 px body face of a 1x and a 2x sheet each, with the owner's GameFont and EditorFont, one 16 px sheet each at whole-number scales; the coverage of 199 glyphs becomes what the owner's sheets hold.

## 18 String table

All player-facing text is new, written in plain functional English and keyed by Species string ids (`ui.lobby.ready`, `mode.flag_chain.objective`). No reference text is copied. The first table sizes the work from the reference text categories, without the six display-option and six sound-option entries of settings that Species does not offer (R-META-414, R-META-429); §18.1 lists the string ids that the documents cite.

| Category | String-id prefix | Reference entries |
|---|---|---:|
| mode messages, objectives and HUD lines | `mode.*` | 74 |
| team colour and faction names | `team.colour.*`, `faction.*` | 11 |
| front-end menus | `ui.menu.*` | 4 |
| main and pause menus | `ui.menu.*` | 31 |
| help and options screens | `ui.help.*` | 11 |
| leaderboard and statistics pages | `ui.leaderboard.*` | 26 |
| graphics options | `ui.options.*` | 14 |
| input options | `ui.options.*` | 7 |
| general options | `ui.options.*` | 5 |
| display options | `ui.options.*` | 7 |
| chat | `ui.chat.*` | 1 |
| controller-disconnected dialog | `ui.dialog.*` | 1 |
| file dialog | `ui.dialog.*` | 3 |
| message dialog | `ui.dialog.*` | 1 |
| window chrome | `ui.window.*` | 8 |
| text and number fields | `ui.input.*` | 5 |
| **Total** | | **209** |

| File | Asset id | Language | Priority | Placeholder |
|---|---|---|---|---|
| `GameData/Strings/en_gb.json` | `strings.en_gb` | English (United Kingdom), the source language (`en-GB`) | mvp | pseudo-localised text |
| `GameData/Strings/fr_fr.json` | `strings.fr_fr` | French (`fr-FR`) | launch | pseudo-localised text |
| `GameData/Strings/de_de.json` | `strings.de_de` | German (`de-DE`) | launch | pseudo-localised text |
| `GameData/Strings/it_it.json` | `strings.it_it` | Italian (`it-IT`) | launch | pseudo-localised text |
| `GameData/Strings/es_es.json` | `strings.es_es` | Spanish (`es-ES`) | launch | pseudo-localised text |
| `GameData/Strings/pt_br.json` | `strings.pt_br` | Portuguese (region to be confirmed) (`pt-BR`) | launch | pseudo-localised text |
| `GameData/Strings/ru_ru.json` | `strings.ru_ru` | Russian (`ru-RU`) | launch | pseudo-localised text |
| `GameData/Strings/ja_jp.json` | `strings.ja_jp` | Japanese (`ja-JP`) | launch | pseudo-localised text |
| `GameData/Strings/ko_kr.json` | `strings.ko_kr` | Korean (`ko-KR`) | launch | pseudo-localised text |
| `GameData/Strings/dev_en_gb.json` | `strings.dev_en_gb` | English (United Kingdom), development captions `ui.dev.*` only; in development editions only, never in a player edition | skirmish-first-playable | pseudo-localised text |

The language set is the reference set of nine; the design phase confirms it. Japanese and Korean need the companion fonts of §17, and every language beyond what the drawn faces cover, Russian among them, needs the DirectWrite upgrade of ADR-034 (O56).

### 18.1 String ids cited by the documents

The reference documents and engine components cite 590 string ids and id patterns (such as `ui.input.key.<name>`). Each needs new en-GB text in `strings.en_gb` and a translation in every other table, except the development captions `ui.dev.*`, which live only in `strings.dev_en_gb`. The owning document defines each id and its meaning; this table sizes the writing by family (counted mechanically from the documents).

| Family | Ids | Main owner |
|---|---:|---|
| `achievement.*` | 25 | [09_front_end_and_match_flow/options_localisation_and_hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| `bot.*` | 1 | [12_simulation_model/06_teams_and_player_commands](../reference/12_simulation_model/06_teams_and_player_commands.md) |
| `building.*` | 15 | [03_buildings_and_world_objects/defences_and_props](../reference/03_buildings_and_world_objects/defences_and_props.md) and 4 more |
| `crate.*` | 59 | [04_supply_crates_powers_and_weapons/README](../reference/04_supply_crates_powers_and_weapons/README.md) and 4 more |
| `creature.*` | 2 | [02_units/README](../reference/02_units/README.md) |
| `faction.*` | 2 | [12_simulation_model/06_teams_and_player_commands](../reference/12_simulation_model/06_teams_and_player_commands.md) |
| `map.*` | 3 | [08_maps_and_level_design/rules_and_conventions](../reference/08_maps_and_level_design/rules_and_conventions.md) |
| `match.*` | 3 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 3 more |
| `match.comeback_bonus.*` | 1 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) |
| `match.eliminated.*` | 2 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) |
| `match.game_over.*` | 5 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 2 more |
| `match.grace_period.*` | 5 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 3 more |
| `match.hud.*` | 5 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 2 more |
| `match.options.*` | 25 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 5 more |
| `match.rank.*` | 9 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 1 more |
| `match.sudden_death.*` | 1 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 3 more |
| `mode.*` | 3 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) and 4 more |
| `mode.complexity.*` | 3 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 2 more |
| `mode.flag_chain.*` | 9 | [01_skirmish_rules_and_modes/flag_chain](../reference/01_skirmish_rules_and_modes/flag_chain.md) and 2 more |
| `mode.launch_race.*` | 12 | [01_skirmish_rules_and_modes/launch_race](../reference/01_skirmish_rules_and_modes/launch_race.md) and 3 more |
| `mode.relic_run.*` | 4 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 2 more |
| `mode.siege.*` | 13 | [01_skirmish_rules_and_modes/siege](../reference/01_skirmish_rules_and_modes/siege.md) and 2 more |
| `mode.spawn_control.*` | 10 | [01_skirmish_rules_and_modes/spawn_control](../reference/01_skirmish_rules_and_modes/spawn_control.md) and 1 more |
| `mode.zone_control.*` | 6 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 1 more |
| `power.*` | 2 | [06_controls_camera_and_commands/selection_and_power_slots](../reference/06_controls_camera_and_commands/selection_and_power_slots.md) |
| `team.*` | 8 | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 2 more |
| `tutorial.tab.*` | 60 | [11_tutorial_and_help](../reference/11_tutorial_and_help.md) |
| `ui.*` | 1 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| `ui.achievements.*` | 2 | [09_front_end_and_match_flow/options_localisation_and_hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| `ui.advanced_options.*` | 19 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) and 2 more |
| `ui.browser.*` | 2 | [09_front_end_and_match_flow/front_end_presentation](../reference/09_front_end_and_match_flow/front_end_presentation.md) |
| `ui.chat.*` | 1 | [ui_framework/core_and_widgets](../architecture/components/ui_framework/core_and_widgets.md) and 1 more |
| `ui.common.*` | 1 | [ui_framework/core_and_widgets](../architecture/components/ui_framework/core_and_widgets.md) and 2 more |
| `ui.confirm_leave.*` | 3 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) and 1 more |
| `ui.connecting.*` | 11 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| `ui.credits.*` | 4 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| `ui.dev.*` | 23 | [diagnostics_test_and_performance/developer_tools](../architecture/components/diagnostics_test_and_performance/developer_tools.md) and 1 more |
| `ui.dialog.*` | 5 | [ui_framework/core_and_widgets](../architecture/components/ui_framework/core_and_widgets.md) and 4 more |
| `ui.error.*` | 18 | [platform_layer](../architecture/components/platform_layer.md) and 6 more |
| `ui.help.*` | 52 | [11_tutorial_and_help](../reference/11_tutorial_and_help.md) and 2 more |
| `ui.help_and_options.*` | 3 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| `ui.help_select.*` | 2 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) and 1 more |
| `ui.host_or_join.*` | 3 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| `ui.input.*` | 5 | [input/controls_and_default_bindings](../architecture/components/input/controls_and_default_bindings.md) and 1 more |
| `ui.language.*` | 9 | [09_front_end_and_match_flow/options_localisation_and_hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| `ui.leaderboard.*` | 14 | [09_front_end_and_match_flow/options_localisation_and_hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| `ui.loading.*` | 1 | [09_front_end_and_match_flow/front_end_presentation](../reference/09_front_end_and_match_flow/front_end_presentation.md) |
| `ui.lobby.*` | 17 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) and 3 more |
| `ui.main.*` | 7 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| `ui.map_editor.*` | 16 | [08_maps_and_level_design/map_editor_hooks](../reference/08_maps_and_level_design/map_editor_hooks.md) |
| `ui.map_select.*` | 5 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| `ui.match.*` | 3 | [diagnostics_test_and_performance/developer_tools](../architecture/components/diagnostics_test_and_performance/developer_tools.md) and 5 more |
| `ui.match_menu.*` | 4 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) and 1 more |
| `ui.mode_select.*` | 1 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| `ui.options.*` | 30 | [09_front_end_and_match_flow/options_localisation_and_hooks](../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) |
| `ui.player.*` | 1 | [data_schemas/config_input_and_strings](../architecture/components/data_schemas/config_input_and_strings.md) and 2 more |
| `ui.player_options.*` | 5 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) |
| `ui.power_slots.*` | 10 | [06_controls_camera_and_commands/selection_and_power_slots](../reference/06_controls_camera_and_commands/selection_and_power_slots.md) and 6 more |
| `ui.repair.*` | 2 | [build_tooling_and_distribution](../architecture/components/build_tooling_and_distribution/README.md) and 1 more |
| `ui.settings.*` | 1 | [data_schemas/config_input_and_strings](../architecture/components/data_schemas/config_input_and_strings.md) and 2 more |
| `ui.team.*` | 1 | [12_simulation_model/06_teams_and_player_commands](../reference/12_simulation_model/06_teams_and_player_commands.md) |
| `ui.title.*` | 2 | [ui_framework/core_and_widgets](../architecture/components/ui_framework/core_and_widgets.md) and 3 more |
| `ui.tutorials.*` | 3 | [09_front_end_and_match_flow/screen_flow_and_match_setup](../reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md) and 1 more |
| `unit.*` | 10 | [02_units/builder](../reference/02_units/builder.md) and 3 more |

String ids added for the assets, with the documents that cite them:

| String ids | Text | Main owner |
|---|---|---|
| `ui.repair.passed`, `ui.repair.failed` | the result of the installation repair check | [build_tooling_and_distribution](../architecture/components/build_tooling_and_distribution/README.md) and 1 more |
| `ui.error.os_unsupported`, `ui.error.user_data_unavailable`, `ui.error.user_data_inside_install`, `ui.error.language_missing`, `ui.error.internal` | start-up errors of the platform layer, also compiled into the executable | [platform_layer](../architecture/components/platform_layer.md) and 1 more |
| `ui.error.content_missing`, `ui.error.content_damaged` | a game file is missing or damaged | [asset_pipeline_and_resources](../architecture/components/asset_pipeline_and_resources.md) and 2 more |
| `ui.error.gpu_unsupported`, `ui.error.display_mode`, `ui.error.resolution_rejected` | graphics start-up and display-mode errors | [platform_layer](../architecture/components/platform_layer.md) and 2 more |
| `ui.match.cheats_used` | the marker shown and recorded when development commands changed a match | [diagnostics_test_and_performance/developer_tools](../architecture/components/diagnostics_test_and_performance/developer_tools.md) and 2 more |
| `ui.match.paused` | the match-paused message | [renderer/frame_graph_and_passes](../architecture/components/renderer/frame_graph_and_passes.md) |
| `ui.dialog.close`, `ui.dialog.error`, `ui.dialog.success` | message-dialog captions and button | [ui_framework/core_and_widgets](../architecture/components/ui_framework/core_and_widgets.md) |
| `ui.input.key.<name>`, `ui.input.mouse.<name>`, `ui.input.pad.<name>`, `ui.input.condition.<condition>`, `ui.input.not` | display names of the 113 keys, 6 mouse controls, 25 gamepad controls and 9 binding conditions, and the negation word | [input/controls_and_default_bindings](../architecture/components/input/controls_and_default_bindings.md) and 1 more |
| `bot.label`, `match.team_left`, `match.player_left`, `ui.lobby.player_joined`, `ui.team.invalid_name`, `faction.rogue.name`, `faction.visitor.name` | team, player and faction names and messages | [12_simulation_model/06_teams_and_player_commands](../reference/12_simulation_model/06_teams_and_player_commands.md) and 3 more |
| `team.colour.green`, `team.colour.red`, `team.colour.yellow`, `team.colour.blue`, `team.colour.orange`, `team.colour.purple`, `team.colour.cyan`, `team.colour.pink` | the eight team colour names | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) and 2 more |
| `match.kill_milestone` | the kill-milestone message of the local team, with the milestone value as its parameter (the kill-milestone event of R-SIM-221) | [01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md) |
| `crate.strike_team.tooltip`, `crate.collector.tooltip`, `crate.builder.tooltip`, `crate.transport.tooltip`, `crate.airstrike.tooltip`, `crate.missile_barrage.tooltip`, `crate.shield.tooltip`, `crate.convert.tooltip`, `crate.speed_boost.tooltip`, `crate.turret_gun.tooltip`, `crate.swarm.tooltip`, `crate.essence_grove.tooltip`, `crate.hive.tooltip`, `crate.brood.tooltip`, `crate.meteor_shower.tooltip`, `crate.blight_grove.tooltip`, `crate.fury.tooltip`, `crate.turret_rocket.tooltip`, `crate.turret_flame.tooltip`, `crate.firebomb_run.tooltip` | the Power help text of each of the 20 outcomes that can be delivered as a Power, shown while the Power is Waiting (R-CRATE-011, R-CTRL-236) | [04_supply_crates_powers_and_weapons/README](../reference/04_supply_crates_powers_and_weapons/README.md) |
| `ui.dev.command.dump_bindings` | the caption of the development command that writes the bindings to the log | [diagnostics_test_and_performance/developer_tools](../architecture/components/diagnostics_test_and_performance/developer_tools.md) |

## 19 Maps and thumbnails

Maps are new layouts. One map slot exists per reference map, following the reference distribution per mode and player count, so every mode keeps its range of 2-, 3- and 4-player maps. Each map has one thumbnail, rendered automatically by the map tool. One 2-player and one 3-player Zone Control slot also host the two tutorials. Map formats: map definition (text, Species map schema) → cooked map definition; placeholder: generated test map. Thumbnail formats: PNG RGB8 rendered by the map tool → BC7 sRGB, no mips; 800x600 px (4:3, display size 400x300).

| Mode | Players | Map files | Thumbnail files | Priority |
|---|---|---|---|---|
| Spawn Control (`mapfile.spawn_control`, `thumb.spawn_control`) | 2 | `GameData/Maps/map_spawn_control_2p_{01..05}.json` (5 files) | `GameData/Thumbnails/map_spawn_control_2p_{01..05}_thumb.png` (5 files) | skirmish-first-playable: 2<br>launch: 3 |
| Spawn Control (`mapfile.spawn_control`, `thumb.spawn_control`) | 3 | `GameData/Maps/map_spawn_control_3p_{01..02}.json` (2 files) | `GameData/Thumbnails/map_spawn_control_3p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Spawn Control (`mapfile.spawn_control`, `thumb.spawn_control`) | 4 | `GameData/Maps/map_spawn_control_4p_{01..02}.json` (2 files) | `GameData/Thumbnails/map_spawn_control_4p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Zone Control (`mapfile.zone_control`, `thumb.zone_control`) | 2 | `GameData/Maps/map_zone_control_2p_{01..02}.json` (2 files) | `GameData/Thumbnails/map_zone_control_2p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Zone Control (`mapfile.zone_control`, `thumb.zone_control`) | 3 | `GameData/Maps/map_zone_control_3p_{01..04}.json` (4 files) | `GameData/Thumbnails/map_zone_control_3p_{01..04}_thumb.png` (4 files) | launch: 4 |
| Zone Control (`mapfile.zone_control`, `thumb.zone_control`) | 4 | `GameData/Maps/map_zone_control_4p_{01..03}.json` (3 files) | `GameData/Thumbnails/map_zone_control_4p_{01..03}_thumb.png` (3 files) | launch: 3 |
| Relic Run (`mapfile.relic_run`, `thumb.relic_run`) | 2 | `GameData/Maps/map_relic_run_2p_{01..04}.json` (4 files) | `GameData/Thumbnails/map_relic_run_2p_{01..04}_thumb.png` (4 files) | launch: 4 |
| Relic Run (`mapfile.relic_run`, `thumb.relic_run`) | 3 | `GameData/Maps/map_relic_run_3p_{01..02}.json` (2 files) | `GameData/Thumbnails/map_relic_run_3p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Relic Run (`mapfile.relic_run`, `thumb.relic_run`) | 4 | `GameData/Maps/map_relic_run_4p_{01..03}.json` (3 files) | `GameData/Thumbnails/map_relic_run_4p_{01..03}_thumb.png` (3 files) | launch: 3 |
| Siege (`mapfile.siege`, `thumb.siege`) | 2 | `GameData/Maps/map_siege_2p_{01..04}.json` (4 files) | `GameData/Thumbnails/map_siege_2p_{01..04}_thumb.png` (4 files) | launch: 4 |
| Siege (`mapfile.siege`, `thumb.siege`) | 3 | `GameData/Maps/map_siege_3p_{01..02}.json` (2 files) | `GameData/Thumbnails/map_siege_3p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Siege (`mapfile.siege`, `thumb.siege`) | 4 | `GameData/Maps/map_siege_4p_{01..02}.json` (2 files) | `GameData/Thumbnails/map_siege_4p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Launch Race (`mapfile.launch_race`, `thumb.launch_race`) | 2 | `GameData/Maps/map_launch_race_2p_{01..03}.json` (3 files) | `GameData/Thumbnails/map_launch_race_2p_{01..03}_thumb.png` (3 files) | launch: 3 |
| Launch Race (`mapfile.launch_race`, `thumb.launch_race`) | 3 | `GameData/Maps/map_launch_race_3p_{01..03}.json` (3 files) | `GameData/Thumbnails/map_launch_race_3p_{01..03}_thumb.png` (3 files) | launch: 3 |
| Launch Race (`mapfile.launch_race`, `thumb.launch_race`) | 4 | `GameData/Maps/map_launch_race_4p_01.json` | `GameData/Thumbnails/map_launch_race_4p_01_thumb.png` | launch: 1 |
| Flag Chain (`mapfile.flag_chain`, `thumb.flag_chain`) | 2 | `GameData/Maps/map_flag_chain_2p_{01..04}.json` (4 files) | `GameData/Thumbnails/map_flag_chain_2p_{01..04}_thumb.png` (4 files) | launch: 4 |
| Flag Chain (`mapfile.flag_chain`, `thumb.flag_chain`) | 3 | `GameData/Maps/map_flag_chain_3p_{01..02}.json` (2 files) | `GameData/Thumbnails/map_flag_chain_3p_{01..02}_thumb.png` (2 files) | launch: 2 |
| Flag Chain (`mapfile.flag_chain`, `thumb.flag_chain`) | 4 | `GameData/Maps/map_flag_chain_4p_01.json` | `GameData/Thumbnails/map_flag_chain_4p_01_thumb.png` | launch: 1 |
| **Total** | | **49 maps** | **49 thumbnails** | |

## 20 Not to be created

These reference files and text entries need no Species counterpart. Counts only; nothing here is named.

| Class | Textures | Meshes | Audio files | Map files | Text entries | Total |
|---|---:|---:|---:|---:|---:|---:|
| bound only to sound events that never fire |  |  | 231 |  |  | 231 |
| editor only (map editor windows) |  |  |  |  | 164 | 164 |
| unused (bound to no sound event) |  |  | 162 |  |  | 162 |
| authoring source | 1 | 133 |  |  |  | 134 |
| campaign, single-player or unreachable code path | 19 | 4 |  | 29 | 14 | 66 |
| unused (never named) | 35 | 19 |  |  |  | 54 |
| debug only | 10 |  |  |  | 43 | 53 |
| dropped feature (campaign or dropped unit, structure or mode) |  | 42 |  |  |  | 42 |
| unused (never listed: superseded map files) |  |  |  | 30 |  | 30 |
| drawn by no rule (among them the objective-card pictures, DEC-050) | 24 |  |  |  |  | 24 |
| editor only (map editor match options) |  |  |  |  | 23 | 23 |
| unused (named only in dead code or data) | 14 | 2 |  |  | 3 | 19 |
| second graphics back end of the reference (not built) | 17 |  |  |  |  | 17 |
| dropped feature (gesture input) | 9 |  |  |  | 2 | 11 |
| music and loader tracks never triggered and never packed |  |  | 11 |  |  | 11 |
| cut or test map | 1 |  |  | 8 |  | 9 |
| dropped feature (user profiles) |  |  |  |  | 9 | 9 |
| edition and store features (not built) | 1 |  |  |  | 8 | 9 |
| dropped feature (mods) |  |  |  |  | 8 | 8 |
| dropped feature (dropped mode) | 1 |  |  |  | 2 | 3 |
| deferred to the design phase (networking options) |  |  |  |  | 3 | 3 |
| dropped feature (dropped unit) | 2 |  |  |  |  | 2 |
| dropped feature (user profiles, mods) |  |  |  |  | 2 | 2 |
| dropped feature (dropped crate outcome and its structure) | 1 |  |  |  |  | 1 |
| dropped feature (dropped weapon) | 1 |  |  |  |  | 1 |
| dropped feature (disabled crate outcome) | 1 |  |  |  |  | 1 |
| dropped feature (dropped mode and unit) | 1 |  |  |  |  | 1 |
| unreachable fallback | 1 |  |  |  |  | 1 |
| editor only |  | 1 |  |  |  | 1 |
| **Total** | **139** | **201** | **404** | **67** | **281** | **1092** |

Bitmap font pages are not in this table: their role passes to the faces of §17, which are drawn new for Species (O56).

