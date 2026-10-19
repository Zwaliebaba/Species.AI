# Species art and audio briefs

> Status: Style guide and category briefs · Related: [GameAssets.md](GameAssets.md) · [assets README](README.md) · [asset_manifest.csv](asset_manifest.csv) · [vision brief](../../vision/species-vision-brief.md) (O9)

## Contents

- [1 Purpose](#1-purpose)
- [2 Style guide: the look and feel to keep](#2-style-guide-the-look-and-feel-to-keep)
- [3 Unit sprites](#3-unit-sprites)
- [4 VFX textures](#4-vfx-textures)
- [5 UI panels, title and logo](#5-ui-panels-title-and-logo)
- [6 HUD, supply-crate and power icons](#6-hud-supply-crate-and-power-icons)
- [7 Cursors and markers](#7-cursors-and-markers)
- [8 Input glyphs](#8-input-glyphs)
- [9 Terrain ramps, water and waves](#9-terrain-ramps-water-and-waves)
- [10 Meshes](#10-meshes)
- [11 Help images and explainer animations](#11-help-images-and-explainer-animations)
- [12 Sound effects by source and event](#12-sound-effects-by-source-and-event)
- [13 UI sounds](#13-ui-sounds)
- [14 Ambience](#14-ambience)
- [15 Music](#15-music)
- [16 Fonts](#16-fonts)
- [17 Player-facing text](#17-player-facing-text)
- [18 Maps and thumbnails](#18-maps-and-thumbnails)

## 1 Purpose

This guide tells artists, sound designers and writers what Species content must look and sound like. The **style** of the reference library stays as it is (vision O9). The **content** is new: every silhouette, texture, icon, panel, logo, wordmark, mesh, sound, voice line, music cue, font choice and line of text is designed and made for Species. No reference file, screenshot or recording is traced, re-coloured, resampled, paint-overed or used as a template. Reviewers may look at the reference library's specifications (sizes, sockets, timings, counts); they never hand reference art or audio to a creator as a model.

Every category below names its counts and constraints; [GameAssets.md](GameAssets.md) gives each file, and [asset_manifest.csv](asset_manifest.csv) gives one row per file.

## 2 Style guide: the look and feel to keep

### 2.1 Flat-shaded low-poly

- Geometry is faceted on purpose: every triangle is lit as one flat face, with one face normal and one colour. Meshes carry no smoothing and no normal maps; the cooker generates face normals.
- Terrain follows the same rule: one colour and one normal per terrain triangle, coloured from a ramp by slope and height.
- Colour comes from per-face colour, not from textures. Meshes are built from a few large shapes: cylinders, boxes, cones and their combinations, read at a distance.
- The mesh cooker attenuates every mesh colour once, at cook (x 0.85, RND-160); the renderer scales team-tinted parts further (x 0.7), the Transport hull strongly (x 0.1) and the green team's Worker figure by half (x 0.5). Artists author final colours and preview them through the engine, not in the modelling tool alone.

### 2.2 Additive glow on black

- Almost every non-solid element glows: the sky grid, clouds, the terrain wire grid, the sea surface, lasers, Essence orbs, particle glows, building lights and manning ports, trees, cursors and normal text. Each is painted on black and added to the frame; overlapping layers saturate toward white, and their draw order does not matter.
- Glow art is greyscale intensity. The draw tint gives it colour: team colour for team-owned glows, effect colour otherwise. Painting colour into a glow texture is the exception, and the row says so.
- Glows have soft falloff to pure black at the texture edge, so no tile edge ever shows.
- Thin bright lines carry the "wire" look: a triangle wire grid laid over the terrain once per cell, and edge lines on shatter shards.

### 2.3 The darkening layer

- A second family of effects darkens instead of brightening: smoke, the hostile Crawlers and their orbs, text outlines and shadows, cursor and icon shadows, logo halos and blob shadows under units.
- Darkening art is greyscale too: white means full darkening. The result is the destination multiplied by one minus the texel, so darkening needs no sorting either.
- Shadows of cursors, of the crate landing marker and of the barrage marker are blurred copies of their art: derived variants that the cooker produces offline, never at load or at run time (RND-422, ADR-018), or greyscale darkening art that an artist authors instead. The compass and team-disc shadows and the glyph shadows are authored art.

### 2.4 Crowds of small figures

- The core of the look is hundreds of tiny figures at once. A Worker is a flat camera-facing card, a few pixels tall at play distance; cards grow with camera distance and turn into pure billboards beyond 300 wu.
- Workers read by silhouette and motion, not by detail: the shape must stay readable at 8-16 px tall and stay distinct from Leaders, Troopers and creatures at the same size.
- Cards use a hard cut-out edge (alpha test), not soft alpha, so crowds stay crisp and need no sorting.
- Larger units (Leaders, Troopers, Transports, Collectors, Builders, creatures) are flat-shaded meshes that sit in the crowd at a small scale; they keep the same simple-shape language.

### 2.5 Team tint and palette

- Team identity is applied by tint, never painted into art. Worker cards, icons and markers are white or light grey and take the team colour from the vertex tint; lasers, glows and lights take it the same way.
- Meshes mark team parts with explicit tint weights on the root node and its direct children: the `_SPECIES_TINT` vertex attribute or the `species_team_tint` material flag (AST-034, ADR-020). The renderer tints them in the shader while keeping their brightness; the cooker never infers a weight from a colour. An artist who works in the "mostly red" key-colour style (red more than twice green and twice blue) runs the authoring helper, which writes explicit weights for review.
- Cloth banners and flags carry the team area as grey levels (its intensity) and a companion tint mask whose white marks full team tint (AST-022). Nothing is recoloured at load and no per-team copy exists; the look is unchanged.
- A match has eight team colours (green, red, yellow, blue, orange, purple, cyan, pink). Art must read correctly under all eight, including next to each other.
- Colour is never the only team cue (XAG 103): banners, flags and markers also differ in shape or pattern where the design phase asks for it.

### 2.6 Black world, fog and light

- The world sits on black: the clear colour and the fog colour are black, the deep sea is not drawn, and linear fog fades terrain and structures to black between a third of the fog distance and the fog distance (the larger of 4000 wu and the world size plus 1000 wu).
- Map lights are over-bright (up to 2.0 per channel) with no ambient term, so faces turned away from the lights fall to near black.
- The seasonal theme swaps in frost terrain and sea, warm white lights, leafless trees and a small hat on every third Worker; it is a palette and accessory swap, not new geometry.

### 2.7 Deliberate crispness

- Some textures are magnified with point filtering on purpose, for a blocky pixel look (RND-432): the particle dot and the front-end backdrop figure through the point-magnify clamp-to-edge sampler (linear under minification), the flat cloud layers, and the captured frame of the screen transition. Their art is designed as clean pixel shapes at the stated size.
- Front-end pictures are authored at twice their display size and drawn with linear filtering. Menu art has no mip-maps; the explainer pictures carry full mip chains, because the explainer panel samples them with mip-maps (R-META-231).
- Block-compressed art is a whole number of 4x4 blocks: its authored size is a multiple of 4 in both dimensions, and the content build rejects any other size (ADR-017).

### 2.8 What must be new

- **New silhouettes.** Workers, Leaders, Troopers, every creature, every structure and every Relic get a new shape language, sketched from a Species character and structure sheet. The same roles, sizes and sockets; different shapes.
- **New textures and icons.** Every glow, beam, icon, cursor, marker and glyph is drawn fresh for its role.
- **New UI layout and palette.** Panels, windows and menus keep their roles (an accent panel, a neutral panel, a highlight beam, positive and negative captions, glowing boxes) with a new palette and new layout.
- **New logo and wordmark**, and a studio mark placeholder until the studio's own mark exists.
- **New presentations** where the reference has a distinctive one: the Missile Barrage launcher and its ground symbol, the Abductor and its abduction effect, the front-end backdrop with its large figure.
- An art lead signs off each asset as a new Species design before it replaces its placeholder.

### 2.9 Accessibility limits on the look

- Gameplay-relevant text and icons keep a contrast of at least 4.5:1 against their background, and a high-contrast mode reaches 7:1 (XAG 102). A glow-on-black look meets this easily for bright elements and fails easily for dim ones; check dim captions and inactive states (3:1).
- Glow and flash effects pass a photosensitivity check (XAG 118): no full-screen flashes above the luminance-change limit, and an option to reduce flashes and camera shake (XAG 117).
- Minimum text and glyph size is 9 px at 1080p (XAG 101).

## 3 Unit sprites

**17 rows, 14 asset ids.** Worker cards (front, four hurt poses, side view, a state overlay, the MVP's carried-load overlay, a seasonal hat), the Visitor faction card, Crawler, Crawler egg and Wisp cards, and the three Missile Barrage cards.

- Canvas 128x128 for Worker and Visitor cards, 32x64 for the side view, 32x32 for small creatures and the hat, 64x64 for the overlay; full mip chains.
- Worker cards: white or light-grey body, hard cut-out alpha (threshold 0.04), the same pivot and baseline on every card so that swapping poses never pops. Hurt poses 01-03 are interchangeable; 04 is the fixed hurt pose that the explainers also use.
- The Visitor card is a distinct faction figure, drawn as a Worker card with a hard cut-out edge (threshold 0.04).
- The Crawler trail is a derived variant: the cooker stacks 18 shrinking copies of the Crawler card into a 32x512 column (RND-320); nothing is authored for it.
- The carried-load overlay (MVP-082) is drawn over the Worker card on the same pivot and baseline while a Worker carries a load of the gathered resource; it reads at 8-16 px as a load held by the figure, not as a second figure, and it stays white or light grey for the tint.
- Crawler and Wisp cards are additive glows; hostile Crawlers draw the same card as a darkening layer, so the shape must read both ways.
- The Missile Barrage launcher, its shadow and the missile are a new presentation of the barrage: a launcher appears, fires a volley, and marks its targets.

## 4 VFX textures

**19 files, 19 asset ids**, all additive greyscale on black except the tree canopy mask and the Abductor beam ring (alpha masks) and the shatter-shard edge pattern, an art reference for the edge lines that the shard shader draws from a barycentric mask (VFX-229); it is not sampled at run time.

- Flares and glows 128x128 (round glow, cloudy glow, starburst, bubble sphere, barrier field, dish signal, terrain grid, fuel flow); beam strips 128x32 (short and long beams, Convert beam); lightning strip 64x256; muzzle flash 64x64; particle dot 16x16; sky cloud tile 16x16; wide barrier field 256x64; canopy mask 128x32; shard edge pattern 128x128; Abductor beam ring 128x32.
- Tiled and scrolled textures (barrier fields, fuel flow, terrain grid, cloud tile, the Abductor beam ring) tile seamlessly along their repeat axis. The sky grid draws the short beam once per line, clamped (RND-151).
- The terrain grid draws one triangle outline per terrain cell: a clean thin bright line on black, centred on the cell edges.
- Keep falloffs smooth to black; avoid baked colour. Effects read through intensity and shape.

## 5 UI panels, title and logo

**20 files, 13 asset ids**: accent and neutral panel strips (about 64x512), a blurred fill strip, the menu highlight beam, the title wordmark, the logo emblem, a studio splash placeholder, the Worker badge for map buttons, three difficulty badges, a check-box tick, a lock icon, the front-end backdrop wheel and the application icon.

- Panel strips tile along their long axis and must work at any panel height.
- The wordmark and the logo emblem are new identity design for Species; the wordmark works on black, at 1x and 2x, and as a halo-darkened mark over busy backgrounds.
- The front-end backdrop (a large figure in front of a decorative wheel, over drifting clouds and glow) is a new composition with the same layering.
- All UI art is authored at 2x display size, opaque or straight alpha, no mip-maps.
- The application icon is the executable's and the window's icon: six PNG files, one per size (16, 24, 32, 48, 64 and 256 px), each drawn for its size, which the build combines into one Windows icon resource.

## 6 HUD, supply-crate and power icons

**50 files, 46 asset ids**: 23 power and outcome icons, the empty-slot read-out, the darkening halo behind icons, three Transport state read-outs, the two Launch Race status-panel layers, and the MVP's HUD set of 17 read-out and command icons in 15 asset ids with three panels (a meter strip, a notification card and a battle-result card).

- Power icons are 128x128 white or grey art on transparent, tinted at draw and drawn over the darkening halo. The same icon appears on a Supply crate, in a power slot, beside the cursor and on off-screen markers, so it must read at 128 px and at 24 px.
- One icon per power or outcome, each with a distinct silhouette; a shared frame or badge is allowed, but the icon inside must differ in shape, not only in colour.
- The Launch Race status panel is a frame (200x300, alpha) and a damage overlay (250x300); both hold live read-outs drawn by the HUD.
- The MVP's HUD icons are 64x64 read-outs (authored at 2x), white art tinted at draw, one silhouette per concept: the resource, growth, the three priorities, the dwelling and the storehouse, contact, claim, attack, battle, tribute, withdraw, respite, renown, the run clock and the deputy. The claim and attack icons are also the icons of world markers, so they must read at 64 px and at 24 px. The meter strip, the notification card and the battle-result card are panel art in the new palette; the HUD layout they form is one of the features reviewed for likeness (PIL-027).

## 7 Cursors and markers

**41 rows, 29 asset ids**: six cursors, thirteen markers (off-screen compass, its empty variant and its shadow, team disc and its shadow, Supply crate and Relic markers, crate landing and barrage target ground markers, flag pointer, jump-pad arrow, order trail, route arrow), four Leader banners and four Flag Chain flag states with a tint mask each, and six blurred shadows that the cooker derives.

- Cursors are 128x128 (the pointer 64x64), white art; the cooker derives the blurred darkening shadow of every cursor of the set except the turret-aim crosshair, offline. Each cursor state must be distinguishable by shape alone.
- The route arrow marks the direction along route lines (RND-353); the empty compass is the compass ring without its arrow, which on-screen markers draw (RND-357).
- The team disc and compass shadows are alpha masks tinted at draw.
- Leader banners show the Leader's order state (idle, follow, formation, gather); Flag Chain flags show neutral, lowered, locked and raised. Both are cloth textures that carry the team area as grey levels with a companion tint mask (white = full team tint), read on a moving cloth at small size.

## 8 Input glyphs

**19 files, 19 asset ids**: four face buttons by position (south, east, west, north), two shoulder buttons, two triggers, two sticks, the horizontal d-pad, six shadow layers and the keyboard Tab key.

- Platform-neutral: shapes and labels describe a control's position, never a console maker's colours, letters or trade dress.
- 64x64 for face buttons, sticks, the d-pad and their shadows, 128x128 for the Tab key, 50x50 for shoulders, triggers and shadows; shadows are darkening layers.

## 9 Terrain ramps, water and waves

**26 files, 3 asset ids**: 14 terrain colour ramps, 4 sea tiles and 8 shore-wave gradients, numbered by theme.

- A ramp is a 64x64 table: x is slope (column 0 flat ground, column 63 vertical), y is height, read with row 0 at the bottom of the image as viewed: the bottom row is the highest terrain and the top row sea level (TER-040). Any other size is rejected. The terrain builder reads one colour per terrain triangle from it, so a ramp is a palette design, not a picture: bands, transitions and the snow line live in it.
- A shore-wave gradient is a 400x10 table; the sea renderer reads pixel row 1 counted from the bottom row (the second row from the bottom), one colour per column (TER-041).
- A sea tile is 128x128, seamless, multiplied by the sea light map.
- Number 01 of each is the map default; ramp 06, sea tile 02 and wave gradient 04 form the frost set that the seasonal theme forces. The environment artist gives each number one biome look; maps pick by number. No match map picks ramp 14 or wave gradient 08; the map editor offers them for new maps, so they come last (post-launch).

## 10 Meshes

**114 files, 85 asset ids**: 76 render meshes in 57 asset ids (unit figures and bodies 7, projectiles 4, creatures 13, structures 37, eight Relics and seven scenery pieces), 37 collision assets in 27 asset ids, and the map editor's camera-view marker.

- Flat-shaded low-poly, per-face colour in `COLOR_0`, no normals; metres, +Y up; triangle budgets are upper limits in the classes 100, 300, 1000, 3000, 10000 and 25000 (the low-poly look usually needs far fewer).
- Team parts carry explicit tint weights (the `_SPECIES_TINT` attribute or the `species_team_tint` material flag) on the root node and its direct children; see §2.5.
- Indexed sockets are numbered from 01 without gaps (`port_01`, `leg_01` to `leg_08`).
- Every collidable mesh has a collision asset: triangle-only geometry with no materials, authored as its own source at the render member's bounds. The hit tests read it, never the render mesh.
- Sockets are empty named nodes; their names come from the socket vocabulary in [GameAssets.md §11](GameAssets.md#11-meshes) and must match exactly. Manning ports stand where a Worker operates the structure; muzzles sit at barrel ends; status lights pair with ports.
- The Beam Dish keeps its node hierarchy (column, upper mount, rotating dish, door), because sockets hang on those nodes.
- The eight Relics are interchangeable carried objects of similar size and weight class, each with a base node; they must look carryable by a crowd.
- The MVP's structures (MVP-083) are new silhouettes: the Wellspring, the centre of a settlement, is its largest and brightest shape, with glow accents at its three lights; the dwelling shows lit windows; the storehouse a drop-off entrance; a resource site reads as a neutral deposit ringed by its ports, the central one as a site of three units. Each structure rises from the ground while it is built and goes dark while it is inactive, so its silhouette must read at every height of the rise and without its glow.
- Scenery pieces are free-standing props that maps place for silhouette and cover; they keep the flat-shaded language and stay below their budgets.

## 11 Help images and explainer animations

**59 files, 6 asset ids**: the camera-controls hint picture, 6 mode cards, 5 general help pages, 16 per-mode help pages, 6 explainer scripts and 25 explainer pictures.

- No baked text in any picture: captions come from the string table, so one picture serves nine languages.
- Help pages 150x150, mode cards 300x200, the camera-controls hint 304x128 (display sizes; author at 2x).
- The objective cards have no pictures: they draw nothing in a match, and their art is created only if the design phase adopts objective cards (DEC-050).
- Explainers are one looping script per mode (canvas 800-1000 x 250; loops of 12.0-30.0 s). The choreography is new: what moves, when and where is designed for Species. The scripts reference their pictures by asset id; the pictures are cut-outs with soft alpha and full mip chains.

## 12 Sound effects by source and event

**902 files in 178 sample groups** from 40 sound sources (units, creatures, weapons, structures, the Supply crate), including the 31 Supply-crate outcome stings, six of which play only if DEC-565 binds them.

- Four groups give sound to moments the reference leaves silent, by Resolved decisions: the laser-bolt fire, structure-hit and kill groups (DEC-564) and the Trooper grenade throw (DEC-097). Variant counts are proposed (DEC-564, DEC-097).
- Each group keeps the reference variant count, so the mix keeps its variety: the Worker attack group has 24 variants, the laser-hit bark 36, the Worker charge bark 37.
- The MVP adds one group of its own, the structure-complete group (MVP-087): a short, bright, positional one-shot that plays with the completion flash whenever a settlement completes a structure.
- Mono, 48 kHz, 24-bit sources. Loops are seamless (no click at the wrap); one-shots start on the transient. The target durations in each row, rounded to a tenth of a second, are targets for timing, not limits.
- The behaviour (volume, pitch spread, distance roll-off, looping, priority, which variant plays) lives in the sound-event data, not in the files. Files are recorded clean and full; the data shapes them.
- **Crowd voice.** Worker, Leader and Trooper barks (selection, orders, fleeing, charging, being hit) are new voice sets for the small figures. Their character is the audio director's choice; whether they use words is an open question (words need one recording per language).
- **Outcome stings** announce a Supply crate's result to every player; one per outcome, 2-8 s.
- The placeholder plan: the sound-event table is written first with every event present (silent placeholders allowed), so a missing binding shows in the data validator, not as silence; placeholders are synthesised tones or text-to-speech lines named by function; the final set is commissioned per source.

## 13 UI sounds

**18 files in 15 groups**: menu hover, select and back, unit select and highlight, state toggle, invalid action, countdown tick, chat message, selection circle, the power-slot panel sounds, the game-over announcement (two takes, played before the victory or defeat sting), and the MVP's battle start and battle end (MVP-087).

- 2-D, short, dry; they sit above the world mix without masking it. Mono or stereo, 48 kHz.

## 14 Ambience

**21 files in 4 groups**: the match ambience bed with two extra layers, and the lobby bed.

- 2-D loops, layered: the layers of one bed play together and must stay in phase-free harmony when they start at random offsets. A bed group streams when its longest member is over 20 s (AEN-072).
- The match bed sits under the crowd: low, wide, no rhythm that competes with the action.

## 15 Music

**6 files proposed, 4 asset ids**: a front-end theme, three in-match beds and victory and defeat stings. The victory and defeat stings are bound to the match end now (R-AUD-307); the theme and the beds are authored only if DEC-049 selects option A. Species commissions new music, and the design phase sets the final list and triggers.

- Stereo, 48 kHz, streamed; beds loop seamlessly and mix below the effects.
- Character: sparse, electronic, cool timbres that leave room for crowd sound and match the glow-on-black look; newly composed, no pastiche of any existing score.

## 16 Fonts

**5 font files**: the title face (Latin, Cyrillic, currency) with its Japanese and Korean companions, the caption face and the body face, one per font role of the UI framework (UI-195).

- Licensed families (open licence or commercial) whose licence allows embedding in a game; rasterised at run time into a glyph cache. A font is third-party content: the owner approves each family, the placeholder's fallback family included, before it lands, and its licence text travels with its files (`AGENTS.md` R14).
- Legible at 9 px at 1080p, and with the darkening outline pass behind the glyphs; numerals clear and fixed-width where the HUD counts.
- Coverage at least U+0020-00FF, U+0410-044F and U+20A0-20AF for the Latin faces, U+3000-30FF and U+4E00-9EFF for Japanese, U+AC00-D7FF for Korean.

## 17 Player-facing text

**9 string tables** (en-GB first, then French, German, Italian, Spanish, Portuguese, Russian, Japanese, Korean), and a development string table (en-GB only, development editions only). About 209 reference text entries in scope size the work: mode messages, objectives and HUD lines (74), team and faction names (11), menus (35), help (11), leaderboard pages (26), options (33), dialogs, windows and fields (19).

- All text is new, in plain functional English, keyed by Species string ids; no reference line is copied or paraphrased closely.
- Messages are short, read at a glance during a battle, and avoid jokes that need translation notes.
- ICU MessageFormat for counts, plurals and names.
- [GameAssets §18.1](GameAssets.md#181-string-ids-cited-by-the-documents) lists the string ids that the documents cite, by family.

## 18 Maps and thumbnails

**49 map slots and 49 thumbnails**: Spawn Control 9 (2-player 5, 3-player 2, 4-player 2), Zone Control 9 (2, 4, 3), Relic Run 9 (4, 2, 3), Siege 8 (4, 2, 2), Launch Race 7 (3, 3, 1), Flag Chain 7 (4, 2, 1).

- Every map is a new layout made with the map tool to the rules of [reference/08](../reference/08_maps_and_level_design/rules_and_conventions.md); no reference layout is copied or reconstructed.
- Each map names its terrain ramp, sea tile and wave gradient by number, and its scenery from the scenery set.
- One 2-player and one 3-player Zone Control map also host the two tutorials.
- Thumbnails are rendered by the map tool: a top-down 4:3 capture (800x600, shown at 400x300), regenerated whenever the map changes.
