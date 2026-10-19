# MVP assets

> Status: Draft · Related: [MVP scope](mvp_scope.md) (MVP-080 to MVP-087, MVP-102, MVP-103) · [GameAssets.md](../assets/GameAssets.md) · [asset manifest](../assets/asset_manifest.csv) · [art and audio briefs](../assets/art_and_audio_briefs.md) · [owner decisions](../decisions/owner_decisions.md) (O52, O55)

## 1 Purpose

This page is the owner's working list of the assets that the MVP's milestones M1 to M4 need (O55), at the minimum that still meets the look of O52: generated low-poly terrain, a few structures, and Workers that are seen walking, carrying and fighting, with no placeholder dots outside the development views (MVP-080). It holds **91 asset ids in 122 files**: 88 files that the asset list already had for the skirmish first playable or for launch, and 34 new files in 30 asset ids. Each is a manifest row with the priority `mvp`, which comes before `skirmish-first-playable` ([GameAssets.md §2.5](../assets/GameAssets.md#25-priorities-and-the-first-playable-assumption)). The manifest stays the record of every file's format, spec, sockets and consumers; this page adds the milestone that first needs each asset and how it is made. M0 needs no asset: from the first content build, the content build generates a stand-in for every row that has no authored file ([GameAssets.md §2.6](../assets/GameAssets.md#26-placeholders)), and a milestone's exit needs the authored files of the assets it first needs.

**Every asset is newly authored** (O9). Each file is made from scratch for its role in the Species style of the [art and audio briefs](../assets/art_and_audio_briefs.md#2-style-guide-the-look-and-feel-to-keep): nothing is traced, re-coloured, resampled or edited from the reference game's art, audio or text, and no reference file is a starting point, a model, an input to a generator or a placeholder. A generator's recipe (its falloff curves, colour stops, mesh shapes and synthesiser patches) is written for Species. **Each asset carries its authorship record** (PIL-027): who made it, by hand or with which tool and recipe, and when, kept beside the asset's import record (ADR-018, ADR-019) until the design phase fixes the record's form. At each art milestone, people allowed to see the reference game (the owner or counsel, outside the clean room) review the features that must differ, and the result is recorded in the asset manifest (PIL-027). Fonts are the one kind that is not made: they are licensed third-party families, which the owner approves (§4).

How to read the tables of §3: one row per asset id; **First needed** is the milestone whose build first draws or plays the asset (MVP scope §3); **Produced by** is one of the methods of §2.

## 2 How the assets are made

The generators are development tools under `Tools/`, projects of the one solution that never ship (MVP-086, `AGENTS.md` §2). They write their output in the source formats of the content root, which the content build cooks into the formats the runtime loads (ADR-017, ADR-019), so the runtime path is the one that later content takes. A tool may use pinned third-party code restored by NuGet (O29, O30), for example the texture compressor of ADR-017, each package recorded in an ADR with its licence; nothing of it reaches the executable (R14). The tools' project names are set when the projects are created.

| Method | Tool | What it makes | The owner's part | Ids | Files |
|---|---|---|---|---:|---:|
| glow generator | texture generator under `Tools/` | greyscale glows, beams, flares, the wire grid, the cloud tile, the front-end highlight beam, the icon halo and the marker masks: falloff curves on black, written as data | sets the curves; approves the look | 14 | 14 |
| ramp generator | the same texture generator | the terrain ramp and the shore-wave gradient, from colour stops by slope and height | chooses the colour stops of the one biome | 2 | 2 |
| noise-tile generator | the same texture generator | the seamless sea tile | approves it | 1 | 1 |
| panel generator | the same texture generator | panel strips, the front-end fill, the meter strip and the notification and battle-result cards, in the new palette (RND-403) | chooses the palette; approves the HUD layout | 6 | 6 |
| mesh kit | parametric low-poly mesh kit under `Tools/`, writing glTF 2.0 (ADR-019) | figures, structures and projectiles from recipes of boxes, cylinders, cones and prisms, with named sockets (ADR-021) and tint weights (ADR-020); each collision asset from its own simpler recipe at its render member's bounds, never from the render triangles (AST-044) | designs each silhouette's recipe | 18 | 20 |
| icon tool, from the banner | the icon rasteriser under `Tools/` | the four banner tint masks, from the team-area layer of the banners' vector sources (AST-022) | nothing beyond the banners | 1 | 4 |
| derived by the cooker | the content build | the blurred cursor shadows (`BlurredShadow`, RND-422) | nothing | 4 | 4 |
| synthesised | sound synthesiser under `Tools/`, rendering patches to 48 kHz WAV | laser shots, the orb, the war horn, the structure-complete sound, battle start and end | designs each patch; approves the set | 6 | 17 |
| hand-drawn card | an image editor | the Worker cards and the carried-load overlay, cut-out art at their exact sizes | draws them | 4 | 7 |
| hand-drawn vector art | a vector editor; the icon rasteriser renders the vectors at 2x | cursors, markers, Leader banners, icons and the check-box tick | draws them | 30 | 35 |
| recorded | a microphone | the Worker death group: eight short voices of a small crowd figure | records them | 1 | 8 |
| written by hand | a text editor | the en-GB string table | writes it | 1 | 1 |
| licensed font (owner approves) | none: third-party families | the body, caption and title faces | chooses the families and approves their licences | 3 | 3 |

*Table 1 — The production methods of the MVP's assets.*

## 3 Assets by category

### 3.1 Terrain

MVP-081: the generated valley in one biome's ramp, its wire grid, the sea ring with its shore waves, and the sky. Only number 01 of each ramp, sea tile and wave gradient is `mvp`; the others keep their priorities.

| Asset id | What it is | Format and budget | New or existing | First needed | Produced by |
|---|---|---|---|---|---|
| `terrain.ramp` | Terrain colour ramp 01: facet colour by slope and height for the valley's one biome | 64x64 px look-up table read on the CPU | existing | M1 | ramp generator |
| `terrain.sea_tile` | Sea tile 01: the sea ring around the valley | 128x128 px seamless, BC7 sRGB, wrap | existing | M1 | noise-tile generator |
| `terrain.shore_wave_gradient` | Shore-wave gradient 01: the colours of the waves on the coast | 400x10 px look-up table read on the CPU | existing | M1 | ramp generator |
| `fx.terrain_grid` | Terrain wire grid: one triangle outline per terrain cell | 128x128 px, BC4 additive, wrap | existing | M1 | glow generator |
| `fx.sky_cloud` | Sky cloud layer over the valley | 16x16 px tile, BC4 additive | existing | M1 | glow generator |

*Table 2 — The MVP's terrain assets.*

### 3.2 Units

MVP-082 and MVP-040: Worker cards drawn as instanced crowds and animated procedurally (ADR-009, ADR-035), so walking, gathering, building, fighting and falling need no further cards; the carried-load overlay is the one new card. Leaders carry their banner, and Workers throw grenades.

| Asset id | What it is | Format and budget | New or existing | First needed | Produced by |
|---|---|---|---|---|---|
| `sprite.worker.front` | Worker card, front view: the crowd figure | 128x128 px, BC7 sRGB, cut-out at 0.04 | existing | M1 | hand-drawn card |
| `sprite.worker.side` | Worker card seen edge-on | 32x64 px, BC7 sRGB, cut-out at 0.04 | existing | M1 | hand-drawn card |
| `sprite.worker.hurt` | Worker hurt and falling poses | 4 files of 128x128 px, cut-out at 0.04 | existing | M1 | hand-drawn card |
| `sprite.worker.carry_load` | Carried-load overlay on a Worker's card | 64x64 px, BC7 sRGB, cut-out at 0.04 | new | M2 | hand-drawn card |
| `mesh.unit.leader_figure` | Leader figure carrying its banner | <= 100 triangles; socket `flag` | existing | M1 | mesh kit |
| `marker.leader_banner` | Leader banner per order state: idle, follow, formation, gather | 4 files of 64x64 px cloth, team area as grey levels | existing | M1 | hand-drawn vector art |
| `marker.leader_banner_tint` | Tint masks of the four banner states | 4 files of 64x64 px, BC4 | existing | M1 | icon tool, from the banner |
| `mesh.projectile.thrown_weapon` | Grenade that Workers throw in battle | <= 300 triangles | existing | M1 | mesh kit |

*Table 3 — The MVP's unit assets.*

### 3.3 Structures and world objects

MVP-034 and MVP-083: the Wellspring, the dwelling, the storehouse and the turret, the resource sites, and the port pad drawn under every port, whose status star shows a manned port (R-BLDG-017). Every structure rises from the ground while it is built and is dark while it is inactive; both are drawing rules, not assets. Every collidable structure has a collision asset, as the existing structures have (AST-044). Of the turret's barrel sets only the gun barrel is `mvp`, because the MVP has the reference gun turret only.

| Asset id | What it is | Format and budget | New or existing | First needed | Produced by |
|---|---|---|---|---|---|
| `mesh.structure.wellspring` | Wellspring: the settlement's centre, drop-off point and source of new Workers | <= 1000 triangles; 11 sockets | new | M2 | mesh kit |
| `mesh.structure.dwelling` | Dwelling (working name): lit windows, construction ports | <= 300 triangles; 10 sockets | new | M2 | mesh kit |
| `mesh.structure.storehouse` | Storehouse (working name): drop-off entrance, construction ports | <= 300 triangles; 7 sockets | new | M2 | mesh kit |
| `mesh.structure.resource_site` | Resource sites: one-unit site and the three-unit central site | <= 300 and <= 1000 triangles; 4 and 12 ports | new | M2 | mesh kit |
| `mesh.structure.turret_mount` | Turret mount: carrier of the four manning ports and status lights | no geometry; 8 sockets | existing | M2 | mesh kit |
| `mesh.structure.turret_head` | Turret head, also the placement preview | <= 300 triangles; 2 sockets | existing | M2 | mesh kit |
| `mesh.structure.turret_barrel` | Gun barrel set: member 1 of 4 (the other members stay `launch`) | <= 1000 triangles; 4 muzzles, 4 casing ejects | existing | M2 | mesh kit |
| `mesh.projectile.turret_round` | Turret round that the gun turret fires | <= 100 triangles | existing | M2 | mesh kit |
| `mesh.projectile.shell_casing` | Shell casing that the gun turret ejects | <= 300 triangles | existing | M2 | mesh kit |
| `mesh.structure.structure_default` | Port pad under every port, whose status star shows a manned port | <= 300 triangles; socket `status` | existing | M2 | mesh kit |
| `mesh.collision.wellspring` | Collision asset of the Wellspring | <= 1000 triangles, no materials | new | M2 | mesh kit |
| `mesh.collision.dwelling` | Collision asset of the dwelling | <= 300 triangles, no materials | new | M2 | mesh kit |
| `mesh.collision.storehouse` | Collision asset of the storehouse | <= 300 triangles, no materials | new | M2 | mesh kit |
| `mesh.collision.resource_site` | Collision assets of the two resource sites | <= 300 and <= 1000 triangles, no materials | new | M2 | mesh kit |
| `mesh.collision.turret_head` | Collision asset of the turret head | <= 300 triangles, no materials | existing | M2 | mesh kit |
| `mesh.collision.turret_barrel` | Collision asset of the gun barrel set: member 1 of 4 | <= 1000 triangles, no materials | existing | M2 | mesh kit |

*Table 4 — The MVP's structures and world objects.*

### 3.4 Effects

MVP-082 to MVP-084: laser bolts, muzzle flashes, deaths, Essence orbs, port stars, the completion flash and the territory line. Additive glow on black needs no post-process glow (RND-418). The border areas' halfway lines and the claims on a territory line need no texture of their own: they are drawn with `fx.particle_dot` spots and with world markers that carry `icon.hud.claim`.

| Asset id | What it is | Format and budget | New or existing | First needed | Produced by |
|---|---|---|---|---|---|
| `fx.glow_round` | Round glow: Worker glow, blob shadows, Essence orbs, shockwave halos | 128x128 px, BC4 additive | existing | M1 | glow generator |
| `fx.glow_cloudy` | Cloudy glow: Essence orbs drifting home (MVP-082) and the Wellspring's glow | 128x128 px, BC4 additive | existing | M1 | glow generator |
| `fx.beam_short` | Beam strip: laser bolts and the sky grid lines | 128x32 px, BC4 additive | existing | M1 | glow generator |
| `fx.muzzle_flash` | Muzzle flash of Workers and turrets | 64x64 px, BC4 additive | existing | M1 | glow generator |
| `fx.particle_dot` | Particle dot: deaths, smoke, the formation preview and route lines | 16x16 px, BC4, point-magnify sampler | existing | M1 | glow generator |
| `fx.starburst` | Star flare: Leader orders, port stars, structure lights and the completion flash | 128x128 px, BC4 additive | existing | M1 | glow generator |
| `fx.beam_long` | Long beam strip: the territory line in its Cladan's colour | 128x32 px, BC4 additive | existing | M2 | glow generator |

*Table 5 — The MVP's effect textures.*

### 3.5 UI and text

MVP-102 and MVP-103: the HUD's panels are bodies of the two panel strips (UI-460) with three new frames, its read-outs and commands carry 64x64 icons, and the front end is one page of fill boxes, a highlight beam and check boxes. The three Latin faces serve the three font roles (UI-195); the Japanese and Korean companions wait for those languages, and the string table is en-GB only (MVP scope §5.1).

| Asset id | What it is | Format and budget | New or existing | First needed | Produced by |
|---|---|---|---|---|---|
| `uitex.panel_accent` | Accent panel strip: window and HUD panel bodies | 64x510 px display, authored at 2x | existing | M1 | panel generator |
| `uitex.panel_neutral` | Neutral panel strip: HUD panel bodies | 64x512 px display, authored at 2x | existing | M1 | panel generator |
| `icon.hud.meter` | Meter strip of every HUD meter | 256x16 px display, authored at 2x | new | M2 | panel generator |
| `icon.hud.notification_card` | Frame behind each notification line | 320x48 px display, authored at 2x | new | M2 | panel generator |
| `icon.hud.result_card` | Frame of the battle result | 400x300 px display, authored at 2x | new | M3 | panel generator |
| `uitex.fill_blur` | Blurred fill of the front-end boxes | 64x512 px display, authored at 2x | existing | M2 | panel generator |
| `uitex.menu_highlight` | Highlight beam behind the focused front-end item | 256x64 px display, BC4 | existing | M2 | glow generator |
| `uitex.icon_check` | Check-box tick of the display and volume options | 50x50 px display, authored at 2x | existing | M2 | hand-drawn vector art |
| `icon.hud.icon_halo` | Darkening halo behind HUD icons | 128x128 px display, BC4 darkening | existing | M2 | glow generator |
| `icon.power.workers` | Workers icon: the Worker count and the selected-Workers cursor badge | 128x128 px display, white art | existing | M1 | hand-drawn vector art |
| `icon.power.leader` | Leader icon: promoting a Worker and the selected-Leader badge | 128x128 px display, white art | existing | M1 | hand-drawn vector art |
| `icon.power.gun_turret` | Turret on the construction bar and in the queue | 128x128 px display, white art | existing | M2 | hand-drawn vector art |
| `icon.hud.resource` | Gathered resource: stores, capacity and costs | 64x64 px display, white art | new | M2 | hand-drawn vector art |
| `icon.hud.growth` | Growth, and its cause when it stops | 64x64 px display, white art | new | M2 | hand-drawn vector art |
| `icon.hud.priority` | The three priorities: gathering, construction, guard duty | 3 files of 64x64 px display, white art | new | M2 | hand-drawn vector art |
| `icon.hud.dwelling` | Dwelling on the construction bar and queue | 64x64 px display, white art | new | M2 | hand-drawn vector art |
| `icon.hud.storehouse` | Storehouse on the construction bar and queue | 64x64 px display, white art | new | M2 | hand-drawn vector art |
| `icon.hud.contact` | Contact and the territory-pressure cause | 64x64 px display, white art | new | M3 | hand-drawn vector art |
| `icon.hud.claim` | Claims: command, notification, cause, world marker | 64x64 px display, white art | new | M3 | hand-drawn vector art |
| `icon.hud.attack` | Attacks: command, warning, cause, world marker | 64x64 px display, white art | new | M3 | hand-drawn vector art |
| `icon.hud.battle` | Battle start, end and result | 64x64 px display, white art | new | M3 | hand-drawn vector art |
| `icon.hud.tribute` | Tribute and eased grievances | 64x64 px display, white art | new | M3 | hand-drawn vector art |
| `icon.hud.withdraw` | Withdrawing an expedition or a claim | 64x64 px display, white art | new | M3 | hand-drawn vector art |
| `icon.hud.respite` | A settlement under a respite | 64x64 px display, white art | new | M3 | hand-drawn vector art |
| `icon.hud.renown` | Renown standing and epilogue | 64x64 px display, white art | new | M3 | hand-drawn vector art |
| `icon.hud.run_clock` | Time left in the run | 64x64 px display, white art | new | M3 | hand-drawn vector art |
| `icon.hud.deputy` | Deputy actions in the away report | 64x64 px display, white art | new | M4 | hand-drawn vector art |
| `font.ui.compact` | Body face: HUD read-outs, list rows, buttons, notifications | OpenType licensed for embedding; Latin-1, Cyrillic, currency | existing | M1 | licensed font (owner approves) |
| `font.ui.stylised` | Caption face: panel titles and overlays | OpenType licensed for embedding; Latin-1, Cyrillic, currency | existing | M2 | licensed font (owner approves) |
| `font.ui.primary` | Title face: front-end and epilogue titles (its Japanese and Korean companions are not needed) | OpenType licensed for embedding; Latin-1, Cyrillic, currency | existing | M2 | licensed font (owner approves) |
| `strings.en_gb` | English (United Kingdom) string table: HUD, notifications, reports and the front end | UTF-8 JSON, ICU MessageFormat | existing | M1 | written by hand |

*Table 6 — The MVP's UI and text assets.*

### 3.6 Cursors and markers

MVP-101: selection, Leader orders and formations, placing a structure, and the world markers of Leaders, incoming attacks and claimed areas. The turret-aim crosshair is not needed, because the MVP has no command that aims a turret.

| Asset id | What it is | Format and budget | New or existing | First needed | Produced by |
|---|---|---|---|---|---|
| `cursor.world` | Default world cursor | 128x128 px display, white art | existing | M1 | hand-drawn vector art |
| `cursor.highlight` | Cursor over a selectable Leader; also the focus highlight of HUD zones | 128x128 px display, white art | existing | M1 | hand-drawn vector art |
| `cursor.unavailable` | Unavailable: no route, an invalid placement or target | 128x128 px display, white art | existing | M1 | hand-drawn vector art |
| `cursor.place` | Placement cursor for structures | 128x128 px display, white art | existing | M2 | hand-drawn vector art |
| `cursor.pointer` | Pointer arrows of HUD lists and scroll bars | 64x64 px display, white art | existing | M2 | hand-drawn vector art |
| `cursor.world.shadow` | Blurred shadow of the world cursor | derived, BC4 darkening | existing | M1 | derived by the cooker |
| `cursor.highlight.shadow` | Blurred shadow of the highlight cursor | derived, BC4 darkening | existing | M1 | derived by the cooker |
| `cursor.unavailable.shadow` | Blurred shadow of the unavailable cursor | derived, BC4 darkening | existing | M1 | derived by the cooker |
| `cursor.place.shadow` | Blurred shadow of the placement cursor | derived, BC4 darkening | existing | M2 | derived by the cooker |
| `marker.edge_compass` | Off-screen compass of world markers: Leaders from M1, attacks and claims from M3 | 256x256 px, white art | existing | M1 | hand-drawn vector art |
| `marker.edge_compass_empty` | Compass ring of an on-screen world marker | 256x256 px, white art | existing | M1 | hand-drawn vector art |
| `marker.edge_compass_shadow` | Shadow of the compass | 256x256 px, BC4 mask | existing | M1 | glow generator |
| `marker.team_disc` | Colour disc under world markers | 128x128 px, BC4 mask | existing | M1 | glow generator |
| `marker.team_disc_shadow` | Shadow of the colour disc | 128x128 px, BC4 mask | existing | M1 | glow generator |
| `marker.order_trail` | Order trail mark of a Leader's waypoint order | 64x64 px | existing | M1 | hand-drawn vector art |
| `marker.route_arrow` | Arrow heads along route lines | 32x32 px, white art | existing | M1 | hand-drawn vector art |
| `marker.flag_pointer` | Pointer on a Leader's waypoint banner | 64x64 px | existing | M1 | hand-drawn vector art |

*Table 7 — The MVP's cursors and markers.*

### 3.7 Audio

MVP-087, from M3: one sample group per sound, the smallest that suits it. The war horn and the orb's collected group also play at the MVP's attack warning and when an orb becomes a Worker; the structure-complete group and the battle start and end sounds are new, one variant each, and the sound-event data varies their pitch. Ambience, music, interface sounds and every other group keep their priorities.

| Asset id | What it is | Format and budget | New or existing | First needed | Produced by |
|---|---|---|---|---|---|
| `sfx.weapon.laser_bolt.spawn` | Laser shots: every laser bolt fired | 10 variants, mono, positional, 0.2-0.7 s | existing | M3 | synthesised |
| `sfx.unit.worker.die` | Deaths of Workers and Leaders | 8 variants, mono, positional, 0.4-1.3 s | existing | M3 | recorded |
| `sfx.unit.essence_orb.collected` | An orb becomes a Worker at its Wellspring | 1 file, mono, positional, about 2.0 s | existing | M3 | synthesised |
| `sfx.building.dwelling.complete` | A structure is completed | 1 file, mono, positional, 1.5-2.5 s | new | M3 | synthesised |
| `sfx.unit.worker.attack_horn` | The attack warning: the war horn, played two-dimensional | 3 variants, mono, 3.0-4.4 s | existing | M3 | synthesised |
| `sfx.ui.battle.start` | A battle starts | 1 file, 2-D, 2.0-3.0 s | new | M3 | synthesised |
| `sfx.ui.battle.end` | A battle ends in its result | 1 file, 2-D, 2.0-3.0 s | new | M3 | synthesised |

*Table 8 — The MVP's first sound set (MVP-087).*

## 4 Summary

| Category | Asset ids | Files | New files | Existing files | M1 | M2 | M3 | M4 | Generated | Hand-made | Licensed |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Terrain | 5 | 5 | 0 | 5 | 5 | 0 | 0 | 0 | 5 | 0 | 0 |
| Units | 8 | 17 | 1 | 16 | 16 | 1 | 0 | 0 | 6 | 11 | 0 |
| Structures and world objects | 16 | 18 | 10 | 8 | 0 | 18 | 0 | 0 | 18 | 0 | 0 |
| Effects | 7 | 7 | 0 | 7 | 6 | 1 | 0 | 0 | 7 | 0 | 0 |
| UI and text | 31 | 33 | 20 | 13 | 6 | 16 | 10 | 1 | 8 | 22 | 3 |
| Cursors and markers | 17 | 17 | 0 | 17 | 14 | 3 | 0 | 0 | 7 | 10 | 0 |
| Audio | 7 | 25 | 3 | 22 | 0 | 0 | 25 | 0 | 17 | 8 | 0 |
| **Total** | **91** | **122** | **34** | **88** | **47** | **39** | **35** | **1** | **68** | **51** | **3** |

*Table 9 — The MVP's assets by category, by the milestone that first needs them, and by how they are made; every count is of files except the asset ids.*

| Milestone | Asset ids | Files | New files | Generated files | Hand-made files | Licensed files |
|---|---:|---:|---:|---:|---:|---:|
| M1 | 38 | 47 | 0 | 25 | 21 | 1 |
| M2 | 35 | 39 | 20 | 25 | 12 | 2 |
| M3 | 17 | 35 | 13 | 18 | 17 | 0 |
| M4 | 1 | 1 | 1 | 0 | 1 | 0 |

*Table 10 — The MVP's assets by milestone.*

**What the owner makes by hand** (51 files):

- **Worker cards** (7 files): the front and side cards and the four hurt poses for M1, the carried-load overlay for M2. Their silhouette is the first feature of the likeness review.
- **Vector art** (35 files): five cursors, five marker images, the four Leader banner states, the Workers, Leader and turret icons and the check-box tick, and the 17 new HUD icon files (15 asset ids): 14 files for M1, 11 for M2, the nine relations and run icons for M3 and the deputy icon for M4.
- **The Worker death group** (8 files) for M3, recorded; a synthesised set may stand in for it.
- **The en-GB string table**, grown at each milestone; the names of the one resource, the dwelling and the storehouse are confirmed before M2's HUD text is written (MVP scope §7), and the asset ids keep their working names whatever the confirmed names are.

**What the owner approves:**

- **The three fonts** (`font.ui.compact` for M1, `font.ui.stylised` and `font.ui.primary` for M2) are third-party content under a licence: each family needs the owner's approval before its files land, and its licence text travels with them (`AGENTS.md` R14). The open-licence family that the fallback-font placeholder uses is licensed content too and needs the same approval ([GameAssets.md §17](../assets/GameAssets.md#17-fonts)).
- **The recipes of the generated assets**: the biome's colour stops, the sea tile, the panel palette, the mesh-kit silhouettes of the Leader, the Wellspring, the dwelling, the storehouse, the resource sites and the turret, and the synthesiser patches.
- **The likeness review** of PIL-027, by the owner or counsel outside the clean room: the Worker and Leader silhouettes at the end of M1, the structure silhouettes and the HUD layout at the end of M2, and the HUD's additions at the end of M3, each result recorded in the manifest.
- **Each asset before it replaces its placeholder**, as the art lead signs off a new Species design ([art and audio briefs](../assets/art_and_audio_briefs.md) §2.8).
- **Each third-party package that a tool project uses**, recorded in an ADR with its licence (`AGENTS.md` §2).

## 5 Assumptions to confirm

The new rows rest on these readings of the MVP scope; each is a row's spec and changes with the scope.

- **Construction ports.** Workers build a structure at ports on its own mesh: six on the dwelling and four on the storehouse; the turret is built at its four manning ports. MVP-033 does not state the count.
- **Sockets.** The Wellspring carries the Spawn Node's sockets (`door`, `essence_link`, three lights, five ports; R-BLDG-124) and an `entrance` for drop-off; the storehouse's drop-off point is its `entrance`. A resource site carries `mvp.resource.ports_per_unit` (4) ports per unit, so a change of that value changes the two resource-site meshes.
- **Orb glow.** MVP-082 names `fx.glow_cloudy` for the Essence orbs, while the reference draws them with `fx.glow_round` (RND-418); both are `mvp`, and `fx.glow_cloudy` also gives the Wellspring its glow.
- **Territory line.** The long beam strip `fx.beam_long` draws the territory line of MVP-084.
- **Left out.** The title wordmark (the front end sets its title in the title face), the application icon, the turret-aim cursor, the Worker 3-D figure, every reference sound beyond the four groups of §3.7, ambience and the development string table keep their priorities.
