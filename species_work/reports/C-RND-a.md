# C-RND-a report — renderer core, frame graph, world renderers, view and lighting, shaders

## Files written

| File | Size (bytes) | Requirements | Tuning keys | Mermaid | Tables |
|---|---|---|---|---|---|
| `design_next/architecture/components/renderer/README.md` | 36 528 | 33 (RND-001 to RND-045) | 12 | 1 | 10 |
| `design_next/architecture/components/renderer/frame_graph_and_passes.md` | 44 504 | 47 (RND-050 to RND-120) | 26 | 3 | 10 |
| `design_next/architecture/components/renderer/world_renderers.md` | 53 242 | 66 (RND-130 to RND-205) | 115 | 0 | 8 |
| `design_next/architecture/components/renderer/view_lighting_and_fog.md` | 22 884 | 38 (RND-208 to RND-250) | 24 | 0 | 7 |
| `design_next/architecture/components/renderer/shaders.md` | 16 172 | 18 (RND-260 to RND-280) | 0 | 0 | 6 |
| `species_work/scripts/C-RND-a_check.py` | helper | | | | |

Totals: **202 requirements**, **177 tuning keys** (all `rnd.*`, each unique and cited by a requirement of its own file), **4 Mermaid diagrams**, **41 tables** (the ordered pass table of 33 passes, the catalogue of 16 world entries, the blend mapping, frame types, frame-graph resources, per-structure rules, cloud layers, depth-bias variants, shader list, combine modes, test tables). IDs are in four blocks inside RND 001–299 with gaps, none reused. Total output ≈ 173 KB against an input of 90.7 KB: the pass table now carries full pipeline descriptors and every number has a key; no file is near 120 KB, so no split was needed.

The template headings of §6.4 are present in every file (§9 Tuning table added after §8). The component follows the folder plan: the README holds the index and links to C-RND-b's `entity_and_overlay_renderers.md`, `visual_language.md` and `options_and_budgets.md` without writing them.

## Checks run

- `scan_patterns.py forbidden_patterns.tsv`: no hit. `scan_patterns.py soft_patterns.tsv`: no hit (one gameplay "port of" reworded).
- `C-RND-a_check.py`: IDs in range and unique; every cited RND id in my range is defined; every DEC, ADR and asset id exists; every `ref.*` key cited is defined in `reference/`; every R-SIM id cited exists; every `decs_to_cite` Open entry is cited by a requirement paragraph; no truncation words; every link resolves to a file on disk or a package target.
- Mermaid: all 4 blocks parse with mermaid 11.4.1 in headless Chromium (a scratch copy of the corpus checker, run from the session scratchpad).
- To resolve terms only, I read outside my ranges: 23 §9.3 and §13 (which terrain facts C-TER owns), 03 (the Source Node global flag), 09 (the first-time sequencing flag), 20 (clock names), 26 §7.8 and 24 §4.9 (to avoid duplicating Essence-orb and blob-shadow values owned by C-VFX and C-RND-b).

## Decisions cited

- **Open DECs from `decs_to_cite`, all cited by requirements:** DEC-222 (RND-002, RND-081), DEC-224 (README §2, RND-120), DEC-226 (RND-042, RND-118), DEC-247 (RND-138), DEC-325 (RND-227), DEC-326 (RND-135, RND-266), DEC-327 (RND-163), DEC-562 (RND-013).
- Resolved from the list: DEC-007 (RND-002, RND-006), DEC-098 (RND-007, RND-017), DEC-654 (RND-003). Dropped: DEC-059, DEC-336, DEC-337 (RND-045, RND-068, RND-080).
- Further DECs cited where they apply: DEC-012, DEC-013, DEC-015, DEC-024, DEC-038, DEC-040, DEC-052, DEC-082, DEC-101, DEC-118, DEC-200, DEC-227, DEC-309, DEC-316.

## New DEC requests

1. **Ownership-light blink skips team slot 5.** RND-167: the lights show when ⌊(n mod 10) ÷ 2⌋ equals the team slot, so slot 5 (the Visitor faction's slot, DEC-082) never shows ownership lights. Options: A — keep (reference default, stated); B — a blink formula that covers every slot. Decide by: design phase.
2. **Poster field of view.** RND-214: poster tiles use the scale φ ÷ 78 for both axes, so a poster frames the scene differently from the screen (narrower horizontally at 16:9). Options: A — keep (reference default, stated); B — tiles that subdivide the on-screen frustum exactly.
3. **Barrier Emitter burn-layer texture range for emitters that are not solar-linked.** RND-199/RND-200: the input gives the second texture's V range only for solar-linked emitters (t₀ = time ÷ 1.5, 7 repeats). I wrote the most conservative default (t₀ = 0, one repeat). Needs confirmation as an Open DEC.
4. **Clock of three cosmetic animations.** The Barrier Emitter scroll ("game time ÷ 15"), the jump-pad pulse and the Solar Array glow pulse ("|sin t|") are written with the presentation frame time of R-SIM-570, the clock the reference uses for render-time animation; the input does not name the clock for the two pulses. Low priority; confirm with an Open DEC or note.
5. **Possible quirk, not verified:** the reference renormalises normals only for some scaled meshes (scenery, Barrier Emitter posts); walls are drawn scaled with no renormalisation stated. Species always uses unit facet normals (RND-055). If the reference look of scaled walls differs, this needs an Open DEC.

## ADR conflicts and requests

1. **ADR-0011 versus the default of DEC-326.** ADR-0011 says the terrain wire overlay is an edge term computed from barycentric coordinates. DEC-326's default (the reference) is the stylised grid texture mapped once per height-field cell, which does not follow triangle edges. I kept ADR-0011's single pass and made the grid source a permutation: `grid_texture` by default, `barycentric` for DEC-326 option B (RND-135, RND-266). ADR-0011 and ADR-0003 (material 5) need a note that the terrain grid term may come from the grid texture.
2. **ADR-0005 blend list lacks "one, source colour".** The world cursor's ghost models (pass 20, C-RND-b) use it. I listed it as `Ghost` in the blend mapping (RND-058); ADR-0005 needs an amendment, or C-RND-b must find another form.
3. **ADR-0018 closed sampler set.** The flat cloud layers need point filtering with wrapping: emulated by wrapping coordinates in the shader and sampling with point clamp (RND-154, RND-265), which is exact. The blobby cloud layers use linear magnification with point minification and wrapping: specified as linear wrap on mip level 0, which differs only under minification, which the black fog hides beyond 5000 wu. The particle pass (C-VFX) also uses nearest magnification. Request: add a point-wrap sampler to the closed set, or accept the emulation.
4. **ADR-0003 and ADR-0013 omit the specular term.** The reference lights the terrain grid term with a specular colour of 0.5 and a shininess of 40, and the special lighting with a specular colour of 1 and a shininess of 10, with an infinite viewer. I specified specular as a material parameter of `mesh_lit` and `terrain` (RND-233, RND-266, RND-270). The ADR text should mention it.
5. **ADR-0012, UI at native resolution (minor).** To keep the reference overdraw order, the match message (pass 13) is drawn inside the scene passes, at render resolution, before debris and particles (RND-072). This makes no difference at render scale 1.
6. **Clarifications, not conflicts.** ADR-0005 "counter-clockwise is front-facing" is stated as counter-clockwise on the render target, `FrontCounterClockwise = TRUE` (RND-055). ADR-0016's depth-bias rule is given a conversion: `DepthBias` = round((k − 1) × 2²⁴) for each near-plane factor k (RND-220). ADR-0076 replaces the reference screenshot file name; RGB output is kept by writing opaque alpha (RND-114).

## Missing assets

None missing. Asset ids I cite beyond `assets_for_you` (all in the manifest; X-AST should add my files to their consumers): `fx.barrier_field`, `fx.barrier_field_wide`, `fx.dish_signal`, `fx.glow_cloudy`, `fx.glow_round`, `fx.starburst`, `marker.jump_pad`, `mesh.creature.ant_body`, `mesh.creature.centipede_segment`, `mesh.relic.variant`, `mesh.structure.barrier_emitter`, `mesh.structure.beam_dish`, `mesh.structure.relay_mast`, `mesh.structure.source_node`, `mesh.structure.spawn_node`, `mesh.structure.structure_default`, `mesh.structure.turret_head`, `mesh.structure.turret_mount`, `mesh.structure.wall_section`, `mesh.unit.leader_figure`, `mesh.unit.transport_hull`, `mesh.unit.worker_figure`, `sprite.worker.front`, `ui.studio_splash`.

Manifest corrections for X-AST: `fx.beam_short` says "wrap repeat for the sky grid; drawn without mips by the sky and laser-bolt passes". My input has the sky grid with clamp and full mips, and only the laser bolts without mips (RND-151). `mesh.structure.structure_default` is also the port pad mesh (RND-168); its role should say so.

## Cross-document requests

1. **C-SIM (reference/12).** The mesh hit-query contract (ray, sphere, mesh against mesh, centre, radius, radius scale) is in `world_renderers.md` §4.4 as the contract the collision assets serve. Please confirm or own it next to R-SIM-123 and R-SIM-327.
2. **C-AST.** These are cooker rules that I state and key in the renderer: the mesh colour attenuation of 0.85 (`rnd.mesh.colour_attenuation`, RND-160); the conversion of reference-style key colours (red above twice green and twice blue) to tint weight 1 (RND-162); the minimum mesh capacity of 100 nodes and 100 sockets (RND-156); the per-node vertical extent (RND-157); blurred-icon shadow variants with a blur radius of 10 px (`rnd.shadow.icon_blur_radius`, RND-243).
3. **C-PLT.** Confirm that the platform fallback chain matches RND-023 (preferred, then desktop, then 800 × 600 windowed, then a fatal error; after a fallback the settings are written back with the present mode set to uncapped). The colour-depth, refresh-rate, Z-depth and overscan preferences have no Species meaning (no display-mode change, `D32_FLOAT` depth).
4. **C-RND-b.**
   - (a) Please cite my keys instead of defining duplicates: `rnd.depth_bias.*` (RND-219), `rnd.pso.*_cutout` (RND-060), and the pass numbers and catalogue state of `frame_graph_and_passes.md` §4.3.
   - (b) RND-242 and RND-243 point to you for the blob-shadow colour, lift and per-actor sizes, and for the target-marker and submarine icon shadows.
   - (c) The `Ghost` blend (ADR item 2).
   - (d) RND-021, RND-110, RND-144, RND-153, RND-163, RND-191 and RND-229 point to `options_and_budgets.md` for these settings: sky detail, sea detail (1 to 3, default 1, 0 not offered), structure detail (1 to 3), entity detail, tree detail (no UI, default 1), special lighting, poster resolution (default 1), the inverted style, and the display preferences.
   - (e) The Beam Dish beam tubes are in RND-171, so do not specify them twice.
   - (f) Marker icon size (render height ÷ 20) and cursor size (35 px) are referred to in RND-028.
5. **C-VFX.** RND-079, RND-082, RND-221, RND-241 and RND-246 refer to you for: the Essence-orb match sizes and alphas and their one-tick-behind drawing; the Worker ground-shadow values; the shockwave flash (0.1 s); camera shake from laser bolts near the camera; the meaning of the shatter parameters 3.0 (wall) and 0.5 (Relic); tree fire particles, which are emitted through effect events (RND-190).
6. **C-UI.**
   - The outline and fill text styles used by the paused banner and the match message.
   - The hint overlay's 8 px and 2 px lines (pass 17).
   - The chat talk hint.
   - The page-transition duration that the front end passes to `InitialiseTransition` (DEC-316, RND-095).
   - The reference's −0.325 px text projection offset is dropped as a rasterisation detail (glyph placement per ADR-0034).
7. **C-TER.** `terrain.md` must contain what I cite and do not key:
   - an integer sample read outside the grid returns the outside height;
   - the per-triangle ramp look-up (with the "highest height" rule and the colour variation);
   - the shore mask, depth map and flat-water tile mask over the twice-size plane;
   - the selection of wave-mesh vertices for a sea detail, with cell = detail × max(X, Z) ÷ 100;
   - the wave formulae, including the dependency on the predecessor pair, the join fix-up and the truncating colour index;
   - the constants 4.0, 250, 0.25 and 7.0.
8. **G08 (R-MAP).** Map lights (exactly two on every reference map), the special-lighting map effect (three reference Zone Control maps), the Source Node global flag (DEC-309), and the sea border on every map (otherwise the terrain has spanning triangles, RND-132).
9. **G09 (reference/09).** RND-118 refers to you for the loading-view visuals: the studio splash, which fades in over 1 s and out over 1 s once the start-up work is done; the spinning Worker; the menu backdrop drawn behind. It also refers to you for the version string, which shows only in the front end.
10. **C-DBG.** Pass 24 (development drawing, always empty in the reference) and the development overlays (frame rate, entity counts, input-mode box).
11. **C-CAM.** RND-212 cites the camera component for the field-of-view values: 60° free camera, 15° zoom, 50° turret aim, 70° scripted views. The main-menu value (90°) and the clamp to [1°, 180°] are mine.
12. **G01 (R-MODE).** RND-074 refers to the match rules for the content and display time of the match message.

## Names needed that are not in the ledger (proposals)

- **Engine types:**
  - frame and configuration: `FrameKind`, `PosterState`, `TransitionState`, `PresentMode`, `RenderConfig`, `ViewParams`, `FrameInputs`, `PresentationSnapshot`, `StartupError`, `PlatformWindow`, `ScissorRect`, `Ray`;
  - pipeline and frame graph: `BlendState` (`Opaque`, `Alpha`, `Premultiplied`, `Additive`, `Darkening`, `Ink`, `Ghost`), `DepthState`, `CullState`, `SortRule`, `DepthBiasVariant`, `PipelineDescriptor`, `PassTableEntry`, `FrameGraph`, `CompiledFrameGraph`, `FrameGraphError`, `ShaderPermutationId`, `RenderTargetFormats`, `CatalogueKey`, `ResourceAccess`, `SettingGate`, `RecordFunction`, `PassContext`;
  - world renderers: `StructureRenderer`, `TreeGeometry`, `TreeQuad`, `TreeParameters`, `TerrainColourRamp`, `SeaMasks`, `SeaLook`, `SkyDetail`;
  - view and lighting: `FrameView`, `DirectionalLight`, `LightRig`, `LitMaterial`, `Mat4`;
  - shared shader layouts: `FrameConstants`, `DrawConstants`, `MeshInstance`, `QuadInstance`, `SkyLayerInstance`, `WaveVertexPair`, `CombineParams`;
  - the event `DeviceRebuilt`.
- **Functions:** `ProjectToScreen`, `ScreenRay`, `ScissorFromWorld`, `PointInView`, `SphereInView`, `GenerateTreeGeometry`.
- **Terms:** blighted tree (a tree of a Blight Grove), turning blighted, blight timer, burn level, Essence Grove tree, object lighting, special lighting, inverted style (the term of DEC-040), screen transition, match message, sky detail, sea detail, structure detail, tree detail, poster resolution, ownership lights.
- **Catalogue entry names:** `sky`, `terrain`, `terrain_inverted`, `sea_flat`, `sea_flat_inverted`, `sea_waves`, `mesh_lit_fog`, `port_pad`, `glow_additive_fog`, `glow_additive_fog_nodepth`, `alpha_fog`, `dual_alpha_fog`, `dual_additive_clip_fog`, `line_additive_fog`, `fade`, `transition`.
- **Frame-graph resource names:** `scene_colour`, `scene_depth`, `scene_resolved`, `scene_depth_single`, `final_image`, `captured_frame`, `screenshot_readback`, `terrain_heights`, `back_buffer`.
- **Shader names:** `sky`, `terrain`, `sea_flat`, `sea_waves_update`, `sea_waves`, `mesh_lit`, `unlit_textured`, `unlit_dual_texture`, `unlit_vertex_colour`, `screen_line`, `depth_copy`, `final`, `fullscreen_fade`, `screen_transition`.
- **Combine modes:** `modulate`, `replace_then_modulate`.
- **New string ids:** `ui.error.gpu_unsupported`, `ui.error.display_mode`, `ui.error.resolution_rejected`, and `ui.match.paused` ("Match paused").

## Dropped as legacy (one line per group)

- **State machinery of the superseded graphics API.** Dropped: the state names and enable flags, the client-array, vertex-buffer and pre-recorded-list storage modes and their auto-fallbacks, extension checks, texture-environment modes, hints, line-width state, the debug state assertion and the comparison with the old state document. Restated as catalogue entries, material shaders and combine modes.
- **The alternative back end.** Dropped: vertex declarations, strip-to-list conversion, rotating dynamic buffers and their mutex, a never-started 120 Hz water thread, the doubled depth range, negated normals, emulator defects and a water shader bound to a never-written target (DEC-336).
- **Effects never reached in a match.** Dropped: the pixelation effect (and its renderer members, its grab texture and its 16 × 16 depth grid), the deform effect, the water reflection and the gamma ramp, which has no caller (DEC-059).
- **Campaign, debug and networking content.** Dropped: the campaign world frame, the in-fiction corporation logo overlay, the start sequence, the private-demo logo, the sky-face cheat, the synchronisation-error markers and banner, the summon-wheel interface, and campaign structures in the per-structure table (control tower, clone lab, display screen, fence switch, portal). Also dropped: the shadow rules of a dropped campaign trooper and of a dropped campaign structure, mission-file lights, Prologue special lighting, the ×1.1 and ×1.5 near-plane factors and the single-player cursor path (DEC-052).
- **Implementation details with no visible effect.**
  - Renderer and frame: renderer re-creation on applying screen options (replaced by reconfiguration, ADR-0012); unused members (a debug flag, gamma); the initial near and far planes, which are overwritten before use; the array growth step of the sort list.
  - Sky and terrain: the 4 × 4 subdivision of cloud quads (a fog workaround, made unnecessary by per-pixel fog); fog left on after the menu sky; view vectors scaled by distance; the 36-byte vertex layout and the over-sized buffer upload.
  - Meshes: the text mesh format and its strip conversion (replaced by glTF, ADR-0019); the shared hit-test scratch buffer; the debug hit-sphere and socket visualisers, which have no caller; cylinder bounds, which are never used; per-team mesh copies and their cache keys (replaced by the palette tint); the loose-file look-up and the fatal assertion on a missing mesh (DEC-015).
  - Display and screenshots: display-mode bit depth and refresh rates; the colour-depth, refresh, Z-depth and overscan preferences; the literal "wait for vertical retrace" passed at window creation; the first-unused-index screenshot file name and the zlib level (ADR-0076).
  - Material and platform details: the inert combine parameter of the flat sea; the ineffective material diffuse settings (1.0 and 1.2), which vertex-colour tracking overrides; a platform-specific water-update throttle and platform preference defaults.
- **Original names in the mesh statistics.** Dropped: the names of the largest meshes and of those with the most nodes. The counts are kept as budgeting figures (RND-156).
