---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T12)
---

# Structures drawn and the stand-ins at M2

## Context and Problem Statement

M2 task T12 draws the settlement from the replica of [ADR-139](ADR-139-replicating-the-settlement-at-m2.md):
- the structures and resource sites on their port pads, with glow accents in the Cladan's colour (MVP-083, ADR-020);
- a construction site rising with its progress, an inactive structure dark, and the completion flash;
- the port stars of manned ports (R-BLDG-017, R-BLDG-046), and the turret's head and barrels at their aim;
- the carried load on the Worker's card (MVP-082), the orbs drifting home and the Wellspring's glow (`fx.glow_cloudy`, O77).

The owner checks it by running a settlement world. The turret's rounds, casings and muzzle flash land in T12's second step.

These decisions are open:
- where the structures' passes stand in the world frame of [ADR-113](ADR-113-the-frame-at-m1.md), and how a structure is lit and fogged;
- how a structure is made dark, and how a site rises;
- which ports draw their pads and stars;
- what the client draws for a manifest row the owner has not made: the carried load's card is one, and T16 and T17 need 26 more (O88).

## Decision Drivers

* RND-163 to RND-170 state the passes: pass 6 draws the structures' meshes with level fog and the object lighting; pass 8, fogged throughout, draws each structure's ownership lights, its port pads and its status stars.
* The renderer's draw paths are ADR-122's mesh batches and ADR-121's quad batches; a new path is code that M2 does not need.
* MVP-083 says what a viewer sees, and not how far, how dark or how large; O89 makes those presentation keys that the owner tunes after the run.
* AST-082 and AST-083 make a stand-in meet its row as an authored file must, so a row's file can arrive later with no other change.
* R14 allows no image library in the content build, and the build stays fast and deterministic (ADR-108).

## Considered Options

For a structure's look:
1. **ADR-122's mesh batches and ADR-121's quad batches** in two new passes, with a brightness in the mesh instance and fogged permutations of the existing shaders.
2. **A structure renderer of its own**, with its own buffers and shaders.

For the files the owner has not made:
3. **Stand-ins made by the content build**, at the rows' keys and formats, marked as placeholders.
4. **Committed stand-in files** under `GameData/`, drawn once by a script.
5. **No stand-in**: the client skips a texture it cannot read.

## Decision Outcome

Chosen options: 1 and 3.
- Option 2 duplicates the mesh path for no draw that option 1 cannot make.
- Option 4 puts files under the content root that are not the owner's art, and an authored file would have to overwrite them.
- Option 5 breaks AST-082's promise that the game always builds and runs, and leaves T12's load invisible until the owner's file.

* **The passes.** Pass 6 `pass_06_structures_solid` (`ScenePass::Structures`) stands after the coastline waves and before the actors; pass 8 `pass_08_structures_alpha` (`ScenePass::StructureAlpha`) after the Workers. The catalogue gains three entries:
  - `mesh_lit_fog`: the mesh vertex shader and a fogged pixel shader (`MeshLitFogPS`), opaque, depth tested and written, back faces culled;
  - `port_pad`: the same shaders, depth tested without a write and no culling;
  - `glow_additive_fog`: the quad vertex shader and `UnlitTexturedGlowFogPS`, a BC4 mask scaling the quad's alpha times the level fog, added.
  - The level fog is the terrain's: `fog = saturate((fogRange.y − depth) × fogRange.z)` on the view depth. The solid colour is drawn towards the fog colour, and a glow's alpha falls with it, so a glow goes out where the terrain goes black. The actors' passes stay unfogged (RND-323).
* **A mesh instance's brightness.** `MeshInstance` gains a fifth row, `shade`, whose x multiplies the lit colour, and the mesh layout gains `SHADE` at byte 64. Every instance but an inactive structure's is at 1.
* **The structures' look** (`GameClient/StructureLook`), built each frame from the replica's latest structures:
  - **Placement.** Each structure's mesh stands at its position facing its yaw: yaw 0 faces +z and a quarter turn +x, with +x of the mesh to the front's right.
  - **Tint.** The tint is ADR-020's: the team colour × `rnd.tint.factor` at weight 1 for a structure of a team, and weight 0 for a resource site. At M2 a settlement's Cladan colour is its team slot (ADR-130), so `TeamColor(owner)` is its colour.
  - **A construction site** is sunk by `mvp.structure.rise_depth_factor` × its mesh's height × the share of its work still to come, and rises through the terrain as it is built. The height is the vertical extent of the mesh's drawn nodes.
  - **An inactive structure** is drawn at `mvp.structure.inactive_brightness` and shows no lights (MVP-035).
  - **The turret** draws its head (`mesh.structure.turret_head`) turned to its aim, and its barrels at the head's barrel mount, their up vector tilted by the aim's pitch towards the head's front. Its mount is sockets only and is not drawn (RND-170). Its head is tinted at `ref.bldg.turret.tint_weight` by the team that mans it, which is the reference's turret team in ADR-133's capture model (R-BLDG-428). With no crew it is untinted, and its barrels are always untinted. The aim is interpolated between the two latest states, the shorter way round (`InterpolateStructures`).
* **The ports** (R-BLDG-046, RND-168, O89):
  - Each port draws its pad `mesh.structure.structure_default` on the terrain at its socket, `ref.bldg.port.pad_height_wu` above it, and a status star of `fx.starburst`, `ref.bldg.port.status_star_size_wu` across, at the pad's status socket. The star is `status_occupied_rgb` while the replica's port mask shows the port manned, and `status_empty_rgb` otherwise.
  - A turret's star of port i stands at its mount's status socket i.
  - A dwelling's and a storehouse's ports, which serve their construction alone at M2 (ADR-132), are drawn only while the structure is a site (O89).
  - The Wellspring's five ports serve no task at M2, so it draws none: a pad with a star that is never green would show a state that never changes. The Spawn Node's own port look (R-BLDG-131) is not the Wellspring's at M2.
  - The structure-detail setting does not exist until the settings store (T15), so every pad is drawn, as at high detail.
* **Ownership lights** (RND-167, R-BLDG-045): at each light socket of a built, active structure of a team, quads 1 to 9 of `ref.bldg.structure.light_quads`, nested, of half-size `light_size_wu` × i ÷ 10, in the team colour. Quad 0 has no size and is not drawn. They show in the command frames n with ⌊(n mod `light_cycle_ticks`) ÷ `light_ticks_per_team`⌋ equal to the team. A client draws the replica one tick behind (ADR-084), so n is the display tick's whole part, the older of the two states drawn.
* **The Wellspring's glow** (O77): `mvp.wellspring.glow_blobs` quads of `fx.glow_cloudy` in its team's colour circle the centre of its `wellspring_core` node at `glow_radius`, one turn every `glow_turn_seconds` of the presentation clock.
* **The completion flash** (MVP-083): each `StructureCompleted` event starts a flash of `fx.starburst` in the team's colour, of half-size `mvp.structure.flash_size`, centred that half-size above the structure's place. It starts as the display leaves the event's tick less one (ADR-123) and fades over `flash_seconds`. A lost event loses its flash, and the notice of ADR-139 is not lost.
* **The carried load** (MVP-082): a living Worker whose replicated load flag is set draws `sprite.worker.carry_load` over its body card, scaled and turned with it, 0.05 wu nearer the camera so that the depth test keeps it in front, in `mvp.worker.load_color`. It is drawn in its own layer of the Workers' batches, after the bodies.
* **The orbs' clouds** (MVP-023, MVP-025, O77): an orb drifting home or waiting at its Wellspring is wrapped in a quad of `fx.glow_cloudy` of half-size `mvp.orb.cloudy_size` and alpha `cloudy_alpha` in its settlement's colour, in pass 11 after the round glows.
* **The scene's assets.** The scene meshes gain the eleven of the mesh kit (ADR-128) in `SceneMesh`'s order, and the scene textures gain `sprite.worker.carry_load` and `fx.glow_cloudy`. A scene mesh with no vertices, the turret's mount, gets no buffer and raises no error.
* **The stand-ins** (AST-082, AST-083, O88; `PlaceholderTexture` in the content build):
  - A row of the MVP's priority whose file under `GameData/` does not exist gets a stand-in at the key its file would have. Its row must be a texture of BC7 sRGB or BC4 with a pixel size, and its `placeholder` column must name a kind the build draws.
  - The row's format, size (twice it where the row says "author at 2x") and levels (its full chain where the row asks for one) are met.
  - The stand-in passes `CheckTexture` against its row as an authored file must, takes part in the key check (AST-015), and is flagged as a placeholder in the content index.
  - The shapes are white unless stated otherwise:
    - a figure is a diamond in the card's upper part;
    - a glyph is a ring round a dot;
    - a panel is flat grey;
    - a gradient is radial;
    - a variant derived by the cooker is the glyph blurred by 3 texels at its first level.
  - The encoder writes BC7 mode 6 blocks whose two ends are the shape's colour clear and opaque, and BC4 blocks from 0 to 255; each texel's index comes from its coverage of 16 samples. So a stand-in is deterministic and needs no image library.
  - §2.6's glyph initials and panel captions are not drawn: the content build has no text rasteriser, and an edition marked for release refuses a stand-in (AST-085).
  - Every other row, of another priority, kind or format, still has no stand-in, and nothing at M2 loads it.
  - The build prints how many rows have no file and how many of them have a stand-in. The placeholder report of BLD-070 comes with packs.
* **Markers.** The two passes record under `render.structures` and `render.structures.transparent`.
* **The render check** (ADR-117) also runs in a settlement world in CI (`Build\CheckWorldFrame.py --world`), so that passes 6 and 8 draw under the debug layer.
* **A watched settlement world.** Until T17's verbs place structures, a Debug build's `--watch` with `--world` gives the steward's settlement to the scripted steward of ADR-138 from the world's start, and the steward commands nothing, as `--watch` does in the sandbox (O77). The run can then show construction, the rising sites and the flash. On seed 1 the scripted steward completes its first dwelling at 0.65 h of play, its second at 1.37 h and its first storehouse at 1.95 h. These come from the growth scene of ADR-138 on the committed tuning, measured with the harness of this task's other figures. The journal's start already holds the watched flag, which it now keeps for a world too, so its layout and format stay.

### Consequences

* Good, because the structures are drawn by the paths the Leaders and the Workers already use, with three catalogue entries and two pixel shaders, and every look the owner will tune is a key of the presentation record.
* Good, because the build runs and draws with every file of the MVP that the owner has not made, and each authored file replaces its stand-in with no change of code.
* Bad, because a mesh instance is 16 bytes larger for every mesh drawn, actors included.
* Bad, because a construction site sinks through the terrain rather than rising in stages, and its ground-level outline is drawn from the first tick. Stages or a scaffold would be new art.
* Bad, because the stand-ins carry no initials or captions, so one HUD icon looks like the next until the owner's art arrives.
* It forecloses: a structure renderer of its own; stand-in files committed under `GameData/`; drawing a port whose state never changes.

**The measurement in this task.** Both runs used the content build's Linux harness, an unoptimised build at -O1 on four cores, on this session's cloud container:
- 105 entries, 27 of them stand-ins of 1,255,308 bytes in all; 1,360 manifest rows have no authored file;
- two builds byte-identical;
- 0.29 to 0.32 s for a clean build over three runs.

### Confirmation

* `GameClientTests`:
  - **StructureLook:**
    - a site is sunk by its height at no progress and by half at half its work, and is at rest once built;
    - an inactive structure is drawn at the record's brightness, and a resource site is untinted;
    - a dwelling site draws a pad and a star at each port, green for the manned and red for the empty, and none once built;
    - a resource site's ports are drawn always, and the Wellspring's never;
    - the lights of teams 0 to 5 over command frames 0 to 19 show in their pairs of frames, team 5 never, and none for an inactive structure or a site;
    - a turret's head turns to its aim, tinted by its crew at the turret's weight; its barrels stand at the mount, tilted and untinted; and its star stands at the mount's status socket;
    - a completion flash starts as the display leaves the tick before its event, fades by half halfway, and ends once;
    - the Wellspring's blobs circle its crystal;
    - a turret's aim turns the shorter way round between the replica's two states.
  - **CrowdLook:** a carrying Worker's load card is drawn with its body, just in front of it and in the load's colour, and a corpse carries none; drifting and waiting orbs, and no floating one, are wrapped in a cloud.
* `GameLogicTests`: a watched world's settlement is commanded by its scripted steward as an AI, its client still sees it, and the journal's replay gives the same hash on every tick.
* CI's render check draws a settlement world on WARP with the debug layer silent; `Build\CheckContentBuild.py` builds the content twice, identically.
* The owner's run on `Debug|ARM64` (the M2 plan's T12):
  - with `--world --seed 1`, the Wellspring and the resource sites stand on the terrain, lit and fogged, with the Wellspring's glow circling its crystal and its lights blinking in its team's frames;
  - each resource site's ports have pads, and a port's star turns green while a gatherer mans it;
  - gatherers carry a load back on their cards;
  - with `--world --watch --seed 1`, the scripted steward's first dwelling rises out of the ground through its construction, with its ports' pads and stars while it is a site, and flashes once when it is built, about 40 minutes in;
  - without `--world` the sandbox draws as before;
  - the program closes cleanly.

## Pros and Cons of the Options

### 1 — the existing batches in two new passes

* Good, because the mesh and quad renderers, their view tests and their upload rings serve the structures as they are.
* Bad, because every mesh instance carries the brightness row.

### 2 — a structure renderer of its own

* Good, because its instance could hold exactly what a structure needs.
* Bad, because it is a second mesh path to keep in step with the first.

### 3 — stand-ins made by the content build

* Good, because AST-082's promise holds from M2, and nothing but the build knows a stand-in exists.
* Bad, because the build carries an encoder of its own.

### 4 — committed stand-in files

* Good, because the build stays as it was.
* Bad, because files that are not the owner's art sit under the content root until each is overwritten.

### 5 — no stand-in

* Good, because nothing is drawn that is not art.
* Bad, because the load of MVP-082 and T16's HUD would be invisible until the owner's files.

## More Information

* Related: [ADR-020](ADR-020-flat-shading-and-team-tint.md), [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md), [ADR-113](ADR-113-the-frame-at-m1.md), [ADR-117](ADR-117-render-check-on-warp.md), [ADR-121](ADR-121-workers-and-orbs-drawn-at-m1.md), [ADR-122](ADR-122-leaders-and-banners-drawn-at-m1.md), [ADR-123](ADR-123-weapons-effects-and-particles-at-m1.md), [ADR-128](ADR-128-mesh-kit-and-drafted-structure-recipes.md), [ADR-132](ADR-132-placement-and-construction-at-m2.md), [ADR-133](ADR-133-the-gun-turret-at-m2.md), [ADR-139](ADR-139-replicating-the-settlement-at-m2.md).
* The rules: MVP-023, MVP-025, MVP-035, MVP-082, MVP-083; R-BLDG-017, R-BLDG-045, R-BLDG-046, R-BLDG-131, R-BLDG-428; RND-163 to RND-170, RND-323; AST-015, AST-082, AST-083; owner decisions O77, O88 and O89; DEC-1001.
* The code:
  - NeuronClient: `FrameRenderer`, `PipelineCatalog`, `ShaderLayouts`, `SceneQuads`, `MeshRenderer`, `WorldScene`, and the shaders `MeshLitFogPS` and `UnlitTexturedGlowFogPS`;
  - GameClient: `StructureLook`, `CrowdLook`, `ReplicaTiming` and `WorldPresentation`;
  - the content build: `PlaceholderTexture`, `AssetManifest` and `ContentBuild`;
  - GameLogic: `CommandJournal` (`StartWorld`, `ConnectSteward`) and `WorldServer`, for the watched world;
  - `Build/CheckWorldFrame.py`.
* ADR-108, ADR-113 and ADR-122 are amended with this record.
* **Amendment 2026-10-22 (M2 task T12's second step, [ADR-141](ADR-141-the-turrets-shots-drawn-at-m2.md)).** The turret's rounds, muzzle flashes, casings and hits are drawn as ADR-141 records; this record's turret draws its head and barrels as before.
