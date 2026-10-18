# P6-RND report

## Summary

All 80 findings of the packet are addressed: **72 fixed**, **7 fixed-differently**, **0 rejected** and **1 request**. Some fixed findings also need a change in a file outside the packet; their notes name the request.

**Files edited:**

- `design_next/architecture/components/renderer/`: README.md, frame_graph_and_passes.md, options_and_budgets.md, shaders.md, view_lighting_and_fog.md, world_renderers.md, entity_and_overlay_renderers.md and visual_language.md. `.gitkeep` is deleted.
- `design_next/architecture/components/asset_pipeline_and_resources.md`.
- `design_next/architecture/components/animation_and_vfx/`: README.md, explainers_sprites_and_procedural_animation.md and particles_shatter_and_screen_effects.md.
- `design_next/architecture/components/terrain/`: README.md, height_field_generation.md and edges_normals_colours_and_sea.md.
- New: `design_next/ADR/ADR-078-gpu-resource-factory.md`. It is Proposed, in MADR 4 form, with front matter like ADR-077's and a "Confirmed when" line.

**Helper scripts** (in `species_work/scripts/`, each writing only P6-RND files or nothing):

- `P6-RND_vectors.py`: read-only; computes terrain sets C and D from `Tools/DesignModels/TerrainGenerator.py`.
- `P6-RND_particle_bound.py`: read-only.
- `P6-RND_replace_block.py`: an exact-once block replacement.
- `P6-RND_normalize.py`.
- `P6-RND_drop_column.py`: drops one column of one table.

**Gates:**

- `python3 -I Tools/DesignCheck/CheckDesign.py design_next --skip-mermaid`: every gate passes.
- The same command without `--skip-mermaid`: every gate passes, and so do the 266 Mermaid blocks. The asset pipeline's figure 1 changed, so this run was required.
- clang-format 18.1.3 (through `species_work/scripts/P6-DAT_format.py`): every `cpp` block of the four areas is clean, 0 blocks differ.

**Size warning:** `asset_pipeline_and_resources.md` is 122,192 bytes, 688 bytes under the 122,880-byte limit of gate 10. The next addition to it needs a split or a trim.

**Ruling 2 (GPU resources):**

- `Engine::GpuResourceFactory` is declared in `renderer/README.md` §2, together with `GpuResourceId`, `GpuResourceError`, `GpuTextureDesc`, `GpuBufferDesc` and `GpuSubresourceData`.
- Its rules are RND-009; the device rebuild is RND-035. RND-001 is amended to own the queues, the heaps and the factory.
- `AssetCache` has a two-phase life. The constructor takes `MountSet&, ContentReader&`; then come `AttachGpu(GpuResourceFactory&)`, `DetachGpu()` and `OnDeviceRebuilt`.
- AST-116, AST-119, AST-122, VFX-003, VFX-205, ParticlePool, ShatterEffects, TerrainRenderer::Build and SeaRenderer::Build all go through the factory.
- The names match what P6-ENG2 (PLT-038 step 13, PLT-043) and P6-ENG1 (glyph cache, UI-198) already cite. The cited items are `Engine::GpuResourceFactory`, `AssetCache::AttachGpu`/`DetachGpu`, the `DeviceRebuilt` event and `CompiledFrameGraph::Import`.

**Ruling 7 (R9):** only the members are renamed: `crowdCardDistanceScale` and `decorationDetail`. The setting keys `render.worker_scale_factor` and `render.structure_detail`, and the tuning key `rnd.option.worker_scale_factor.default`, keep their names because they are data names. **No key rename is needed**, so the DAT row noted by P6-DAT and P6-X stays as it is.

## Findings

| Id | Status | Note |
|---|---|---|
| R1b-4 | fixed | VFX-108 now states the observable behaviour: the panel is laid out again by the R-META-224 formulas whenever the screen mode changes. The Species widget is also laid out again on DPI and text-scale changes (DEC-961, ADR-033). DEC-961 option B is still wrong: request 9. |
| R3b-2 | fixed | Applied as ruling 2 (see the summary). The new ADR-078 is Proposed. The factory section lives in renderer README §2: the abstract `GpuResourceFactory`, RND-009 (six rules, and the staging budget `rnd.resources.staging_budget_mib` = 96 MiB) and RND-035 (rebuild). AST-002, AST-010, AST-116, AST-119 and AST-122 and the §2.1 `AssetCache` are two-phase. `FrameRenderer::Create(AppWindow&, const RenderConfig&)` no longer takes `AssetCache&`; `Resources()` exposes the factory. VFX-003 is updated. PLT-038 and UI-003 were done by P6-ENG2 and P6-ENG1 and checked against these names. The ADR index row is request 1. |
| R4a-12 | fixed | `RenderSettings::crowdCardDistanceScale` replaces `workerScaleFactor`. The `Species` renderers map it, and an R9 paragraph after RND-442 says so. The keys keep their data names (summary). |
| R2b-142 | fixed-differently | Set C is now a smooth area over the seeded synthetic set A, not one sample: centre (1350, 45.0, 1350), half-width 300 wu, threshold 18.3. Table 7.2-G is its first differing evaluation in pass 1, at sample (68, 68). TER-145 quotes its counts, 2 888 above-threshold evaluations of which 377 differ. Set D keeps its synthetic flat field (it never came from a map), but the Add value 24.1, which echoed reference content, is now 18.7, and table 7.2-H is recomputed. The T2 values 24.1 and 11.7 become 18.7 → 18.700000762939453 and 18.3 → 18.299999237060547. All values come from the conformance model (`Tools/DesignModels/TerrainGenerator.py`). The model's constants are request 4. |
| R3b-24 | fixed | VFX README T6 and particles T2 state their tolerances. The positional bound comes from `P6-RND_particle_bound.py`. |
| R2b-161 | fixed | VFX-113 rewritten: additive sprites carry intensity on black, and under the additive blend black adds nothing (AST-032, RND-423). |
| R4a-28 | fixed | The shaders are `ParticleEmitCS.hlsl`, `ParticleSimulateCS.hlsl`, `ParticleCompactCS.hlsl` and `ShardUpdateCS.hlsl` (byte arrays `g_<Shader>CS`). They are listed in RND-263 as CS rows with no axes, and named in the particles §2 intro, the threading contract and VFX-237. RND-261 says where they live. CONVENTIONS is request 12. |
| R3b-17 | fixed | New AST-127: a content library linked by the client, the server and the tools, and a client resource library linked by the client only. The server creates no GPU resource, sound bank or glyph page. BLD-006 already requires this split. The table 3.1-A wording is optional request 14. |
| R3b-18 | fixed | New AST-128 gives the pack's byte layout: little-endian, header table 3.12-A (128 bytes, magic `SPCPACK\0`), chunk record table 3.12-B (24 bytes), entry record table 3.12-C (57 + *k* + *s* bytes), the TOC order and padding, and what the TOC hash covers. New AST-129 makes both manifests cooked records, `assets.mount_manifest` and `assets.content_manifest`. ADR-058 is request 15; the DAT registry is request 3. |
| R2b-117 | fixed | RND-023 rewritten: borderless at the desktop size or windowed; no display mode is ever set; the PLT-112 chain applies; only the size in effect is written back, with no vertical-sync write-back (ruling 4). PLT-112 still writes `Uncapped` back: request 17. |
| R3b-21 | fixed | New renderer §3.5 "The presentation snapshot". RND-018 (production, retention and readers; the baseline under DEC-222 option A) and RND-019 (records, table 3.5-A). A Species sketch declares `MatchSetupView` and `PresentationSnapshot final : Engine::SceneSnapshot`. The renderer and VFX data contracts cite it. CAM-004 and AEN-041 are request 18; DEC-222 is request 10. |
| R4a-15 | fixed | `FrameKind` is {`Match`, `MainMenu`, `Loading`, `Editor`}. RND-067 and RND-118 use the new names; `LoadingView` and `MapEditor` stay game types. The AEN `CommandWriter::SetLoadingView` is request 19. |
| R2b-118 | fixed | §8.2 is retitled "Direct3D 12 realisation per component" and has a "Species realisation" column; "device loss" is now "device removal (ADR-008)". |
| R4a-29 | fixed | `QuadInstance` is in the shared-header form: `Float3`, `float` and `Uint32` fields, no default initialisers, and enumerations stored as `Uint32`. Its comment names `FrameConstants.h` (RND-262). |
| R3b-20 | fixed | New RND-053: the game layer builds the ordered pass table and hands it to the engine. New RND-054: each entry's record function receives `const SceneSnapshot*`, and game recorders reach their `PresentationSnapshot` through one `static_cast` adapter. |
| R2b-121 | fixed | RND-055 now holds pipeline state only: rasteriser, blend and depth per catalogue entry. Fog and light values are shader constants. The reference-mesh winding is gone. |
| R2b-123 | fixed | RND-071 is the positive rule: only the passes of the pass table draw. |
| R3b-23 | request | Every test section of the renderer, AST and VFX files that holds golden-image tests now has a harness note citing the default tolerance and reference configuration of DBG §4.8; terrain has none. That rule is not in DBG yet: request 16. |
| R3b-19 | fixed | frame_graph §2 declares `PassContext`, `RecordFunction`, `ResourceAccess`, `CatalogKey`, `SettingGate`, `RenderTargetFormats`, `ShaderPermutationId`, `UploadAllocation`, `CompiledFrameGraph` (with `Import`) and `FrameGraphError`, the last with failure rows in §6. |
| R2b-140 | fixed | RND-453 is an "Item / Species rule" table: the reference column and `rnd.budget.reference_font_atlases` are deleted. The headings are kept because the index links to their anchors (request 2). |
| R3b-25 | fixed | RND-454 sets provisional pass-group budgets on the reference test machine: `rnd.budget.world_ms` 3.0, `rnd.budget.actors_ms` 4.0, `rnd.budget.effects_ms` 3.0 and `rnd.budget.overlays_ms` 2.0, which is 12.0 ms and leaves 4.7 ms. VFX-023 uses the effects budget; T7 reports until DEC-106 and DEC-971 are decided. UI-473 is request 18. |
| R4a-14 | fixed | The member is now `RenderSettings::decorationDetail` (a `DetailLevel`), and the `Species` structure renderers read it (RND-445). CONVENTIONS 8.14 is request 12. |
| R2b-133 | fixed-differently | RND-226 to RND-230 now say: two directional lights in the frame constants, per-face diffuse, no specular term; the shininess values and keys are removed. The one-frame light lag stays as the stated default of the Open DEC-325, because it is observable behaviour and the DEC records the choice (ruling 3). |
| R2b-128 | fixed | Replaced with "Mesh budgets are per asset ([GameAssets] §11) and per frame (RND-454)". |
| R4a-23 | fixed | RND-187 to RND-189 use the glossary terms essence tree, blight tree and corrupted tree, with the R-BLDG-622 to R-BLDG-627 citations. CONVENTIONS 8.14 is request 12; R-CRATE-206 is request 6. |
| R3b-26 | fixed | New AST-130 (SHOULD): enter a skirmish match within `ast.load.match_entry_budget_s`, 10 s provisional, on the reference test machine; T31 reports until DEC-971. Generation's share is `ter.perf.generation_budget_ms`, 2000 ms provisional (TER-242, terrain §4.17 row). T26 is timed against that share, and AST design input 7 is updated. The AEN-102 match-bank share is request 18. |
| R2b-149 | fixed | TER-230 restated, and the "In the reference content" column of table 4.16-A deleted. Each construct is a Species rule, for example "integer overflow in the tile generator's step → unsigned 32-bit arithmetic that wraps (TER-068)". |
| R2b-143 | fixed-differently | Table 4.9-A's reference column is replaced by an "Effect" column that states what each operation does, without reference values. A column of authored-map ranges would have kept the reference figures. |
| R2b-157 | fixed | VFX-006's case column now describes each case by its Species behaviour. |
| R2b-158 | fixed | Both clauses of VFX-008 deleted. |
| R4a-61 | fixed-differently | There is no engine "team" type. The engine has `FrameRenderer::SetPalette(std::span<const ColorRgba>)` (RND-016); `Species::ExplainerTeamColors {friendly, enemy}` feeds `ExplainerPlayer::Draw`; `MatchSetupView` is declared in renderer §3.5 (R3b-21). CONVENTIONS is request 12. |
| R3b-58 | fixed | The placeholder note is removed; every type the sketch uses now has an owner. |
| R2b-160 | fixed | VFX-103 cites the Species explainer JSON (GameAssets §12, `panels.explainer`); the reference format sentence is deleted. |
| R2b-162 | fixed | `vfx.cloth.still_step` and its sentence deleted. DEC-955 option B still cites the key: request 11. |
| R2b-159 | fixed | `vfx.budget.reference_particle_start` and `vfx.budget.reference_particle_growth` deleted. VFX-214 states the fixed-capacity pool of ADR-010 only. |
| R4a-33 | fixed | The particles §2 records are rewritten as `ParticleSlot.h` (one field per line) and pass clang-format. |
| R3b-54 | fixed | `AssetCache::Unregister(RuntimeTextureId)` added. `ImageProducer` declares `Width`, `Height`, `Format`, `Sampler` and `WritePixels`. |
| R3b-63 | fixed | The table 1-A UI row now reads "nothing". |
| R4a-32 | fixed | The §2.1 sketch is rewritten and passes clang-format: one enumerator per line, comments not aligned. |
| R2b-156 | fixed | The §8.1 rows DEC-332 and DEC-340 now read "Dropped; Species defines its own packs (ADR-058)". |
| R2b-267 | fixed | `renderer/.gitkeep` deleted. |
| R2b-119 | fixed | RND-045 is now a positive MUST: exactly the frame types and passes of the frame graph, on Direct3D 12 (ADR-001), with one code path. |
| R4a-36 | fixed | The fields are renamed `FrameInputs::extrapolationSliceIndex` and `presentationSequence`, and keep their R-SIM citations. |
| R3b-49 | fixed | `StartupError` declared: `NoCapableAdapter`, `DeviceCreationFailed`, `SwapChainFailed`, `PipelineCreationFailed`, `FrameGraphInvalid`. RND-020 and RND-023 cite it. |
| R4a-35 | fixed | Renderer §2 states that `ViewParams` is filled each frame from `CameraFrameOutput`, field by field. |
| R2b-120 | fixed | "an emulation layer" removed from the DEC-059 row. |
| R2b-136 | fixed | The paragraph now says faces are flat-shaded (ADR-020). |
| R2b-137 | fixed | RND-328 states the positive rule only (see DEC-062). |
| R2b-124 | fixed | "Pass numbers are stable ids; unused numbers stay unassigned." |
| R2b-122 | fixed | RND-056 rewritten: the depth state is stated per pass in the pass table. |
| R2b-125 | fixed | RND-068 states the positive rule only. |
| R2b-126 | fixed | RND-080 states the positive rule only. |
| R2b-127 | fixed | "The banner's text is the string `ui.match.paused` of the string table, in the player's language." |
| R2b-141 | fixed | RND-443 rewritten without the texture rescale and the terrain storage mode. |
| R2b-139 | fixed | RND-452 per P6-RD request 8. |
| R4a-54 | fixed-differently | `DetailLevel` {`None` = 0, `High`, `Medium`, `Low`} types `seaDetail`, `skyDetail`, `decorationDetail` and `entityDetail`. `SkyRenderer::Record` and terrain `SelectWaveVertexPairs` take a `DetailLevel`, and `SkyDetail` is gone. `treeDetail` stays `std::uint8_t`: it is the exponent (1 or more) of the leaf-alpha boost (RND-191), not a level. |
| R2b-135 | fixed-differently | The combine modes are named by what they compute: the data names `product` and `texture_color`, the `COMBINE` axis values `PRODUCT` and `TEXTURE_COLOR` (RND-263, RND-267, RND-268, RND-061, world_renderers). Short data names were chosen rather than the proposed phrases. CONVENTIONS is request 12. |
| R3b-53 | fixed | T1 names the check step `Build/CheckShaders.py`, which recompiles every shader with `dxc -HV 202x`. T3 compares the `dxc -Fc` buffer definitions with RendererTests' `offsetof`/`sizeof`. RND-260 states the second compile and `-Zpr`. |
| R4a-63 | fixed | The shared header is `FrameConstants.h`, in `namespace Engine`, with a type prelude for HLSL and C++ (RND-262). The VFX records are in `ParticleSlot.h`. |
| R2b-132 | fixed | RND-219 and RND-220 state a bias factor *k* and its `DepthBias` conversion, with no near-plane narrative; the float-depth formula was checked on Microsoft Learn (URL in ADR-016). ADR-016 still describes the near-plane look: request 15. |
| R2b-134 | fixed | "three reference maps" removed. |
| R2b-138 | fixed | The triangle average removed from RND-427. |
| R2b-129 | fixed | RND-132 states the map rule and cites R-MAP-085. |
| R2b-130 | fixed | RND-145 reads the presentation clock (ADR-036, VFX-009). |
| R3b-50 | fixed | Lines at −4000 + 80 × *k* for *k* = 0 to 124; there is no line at 6000. |
| R2b-131 | fixed | "Team parts carry explicit tint weights (ADR-020); no colour test exists at run time." |
| R4b-35 | fixed | RND-173 cites `ref.bldg.jump_pad.arrow_length`, `arrow_half_width`, `brightness_base` and `brightness_pulse`. The four `rnd.structure.jump_pad.*` keys are deleted. The clock wording of R-BLDG-475 is request 7. |
| R4b-36 | fixed | The six tree keys are deleted, and RND-182, RND-186, RND-190 and RND-191 cite `ref.bldg.tree.branch_width`, `child_reach`, `sway_amplitude`, `blight_sway_amplitude`, `burn_flare_min_size` and `leaf_alpha_boost`. |
| R3b-51 | fixed | T12 compares against a conformance implementation written from RND-181 to RND-184 and kept with the test suite. |
| R2b-154 | fixed | The count is deleted from TER-062. |
| R2b-155 | fixed-differently | TER-240 and TER-241 now use the **performance terrain** that T26 defines: 334 × 334 samples, 25 tiles of 257 × 257 with *p* = 2, 6 flatten areas, 6 527 800 calls, 0.89 MB of heights and 2.68 MB of normals. Memory then grows to the `ter.grid.max_samples_per_side` limit. The content-build limit (32 768 per side) was not used, because it is a validation bound, not a performance target. |
| R2b-150 | fixed | TER-153 now states the map rule (R-MAP-085) without figures; the table row reads "65 536 cell sizes from the origin". TER-155 is restated the same way. |
| R2b-151 | fixed | The figures are deleted from TER-180. |
| R2b-152 | fixed | TER-185: every other presentation consumer draws from its own named stream (DEC-242). |
| R2b-153 | fixed | TER-221 states the Species behaviour: an option change regenerates nothing, and a device removal rebuilds buffers with the same colours. The TER-222 narrative is deleted. |
| R2b-144 | fixed | The count is replaced by the bound that follows from *k* = ceilLog2(*req* − 1). |
| R2b-147 | fixed | TER-068 states the unsigned 32-bit arithmetic only. |
| R2b-148 | fixed | TER-073 states the defined conversion; TER-072 also loses "In the reference". |
| R2b-145 | fixed | The column is dropped from table 4.7-A, and so are the reference-content sentences of TER-111, TER-112 and TER-124. |
| R2b-146 | fixed | The column is dropped from table 4.8-A. |

## Requests from other packets applied

| Request | Where | Applied | Note |
|---|---|---|---|
| P6-DAT 8(a) | AST table 3.3-A and AST-019 | yes | `color`; AST-019 cites DAT table 3.1-A with the meanings of table 3.3-A |
| P6-DAT 8(b) | AST table 3.5-A | yes | parameters `color`, `threshold`, `old_color`, `new_color` |
| P6-DAT 8(c) | AST §2.1 sketch | yes | the whole sketch passes clang-format |
| P6-DAT 8(d) | AST-001 | yes | now the wording of P6-ADR 1(b) below |
| P6-DAT 8(e) | terrain pseudocode `posX`/`posZ` | no (optional) | `posX` is a pseudocode variable, and `posXi`/`posZi` (table 7.2-B) are derived from it; `positionXi` would read worse. The values are the `TerrainTile` fields either way. |
| P6-DAT note, P6-X 4 | `render.worker_scale_factor` | nothing to do | only the C++ member was renamed; no key changes |
| P6-RB 4 | VFX-124 | yes | removed stub kept, because DEC-871 cites it; `vfx.orb.broken_fade`, `vfx.orb.break_particles` and the `OrbBroken` effect row deleted |
| P6-RB 14 | AST mesh-cook pseudocode, VFX-130, every formula | yes | every `normalise(` is `Normalize(` |
| P6-RD 8 | RND-452 | yes | |
| P6-RE 5 | RND-453, RND-442 | yes | the slow-frames row deleted; "(R-SIM-575)" dropped |
| P6-RC 12 | RND-367, RND-422, terrain README "Writer precision" | yes | `ref.ctrl.cursor.set.world_size`; three shadow cursors removed; the bullet replaced verbatim. The optional camera-shake split is not applied, as P6-RC allowed. |
| P6-ADR 1, as superseded by O30 (coordinator relay) | AST-076, AST-091 | yes | AST-076: "with the importer's own checks of ADR-019 (no third-party validator, O30)"; AST-091: "passes the importer's checks of ADR-019"; no other Khronos or validator mention in the four areas |
| P6-ADR 1(b) (coordinator relay) | AST-001 | yes | "which the content build tool runs: a development tool, a tool project under `Tools/` that never ships …" |
| P6-DEC 8 | DEC rows of visual_language, renderer README, VFX README, particles | yes | DEC-342 "reference art and audio"; DEC-336 "other renderers are not specified; Species has one renderer (ADR-001)"; DEC-962 "Species defaults for …" |
| P6-ENG2 5a | renderer README T1 | yes | kind "checker fixture"; table 4.1-B rows 13 and 11, BLD-045 |
| P6-ENG2 5b | VFX README T1 | yes | as requested |
| P6-ENG2 5c | RND-020, AST-002 against PLT-038 | checked | step 11: mount and CPU loads; step 13: device, factory, swap chain, then `AttachGpu`; consistent |
| P6-ENG2 5d (optional) | RND-454, T7, options design input 1, terrain TER-242 and T26, AST design input 7, VFX-023 | yes | "on the reference test machine (DBG-209)" |
| P6-ENG2 8, P6-ENG1 7 | names | checked | `Engine::GpuResourceFactory`, `AssetCache::AttachGpu`/`DetachGpu`, `DeviceRebuilt`, `CompiledFrameGraph::Import` match what PLT and UI cite |

## Requests

1. **Coordinator, `design_next/ADR/README.md`.**
   - After the ADR-077 row of the index, add: `| [ADR-078](ADR-078-gpu-resource-factory.md) | GPU resource factory: one creation seam for every client GPU resource | Proposed | 2026-10-18 |`.
   - Extend the baseline sentence ("… and ADR-050 to ADR-077 …") to say that ADR-078 onward are records added after the baseline (P6-ADR request 8 covers the wording).
2. **Coordinator, requirements index.** Regenerate `REQUIREMENTS_INDEX.md` and `requirements_index/`.
   - **New:** RND-009, RND-018, RND-019, RND-035, RND-053, RND-054, AST-127, AST-128, AST-129 and AST-130.
   - **Stub:** VFX-124. Keep it, because DEC-871 cites it.
   - **First sentences changed:** RND-001, RND-023, RND-045, RND-055, RND-056, RND-068, RND-071, RND-080, RND-091, RND-219, RND-220, RND-226 to RND-230, RND-443, RND-452 to RND-454, VFX-023, VFX-103, VFX-108, VFX-113, VFX-214, AST-001, AST-002, AST-076, TER-062, TER-068, TER-145, TER-153, TER-155, TER-221, TER-230 and TER-240 to TER-242, among others.
   - `requirements_index/vfx.md` still quotes the deleted keys in its VFX-214 row; regeneration fixes that.
   - At regeneration, optionally retitle `options_and_budgets.md` "4.3 The reference options pages" and "4.4 Reference caps and costs" to "4.3 The options pages" and "4.4 Caps and costs", and update the index anchors with them. I kept the headings so that the index links stay valid.
3. **Coordinator (P6-DAT has finished), `architecture/components/data_schemas/README.md` table 1-A.** After the `assets.engine_table` row, add:
   - `| \`assets.mount_manifest\` | the ordered pack list of one edition, with pack kinds and patch targets | generated by the content build | \`mount_manifest.bin\` beside the packs | no | [asset pipeline](../asset_pipeline_and_resources.md) §3.13, AST-129 |`
   - `| \`assets.content_manifest\` | the entry hashes of every pack and the content identity | generated by the content build | \`content_manifest.bin\` beside the packs | no | [asset pipeline](../asset_pipeline_and_resources.md) §3.14, AST-129 |`
4. **Coordinator, `Tools/DesignModels/TerrainGenerator.py`** (the conformance model of terrain §7.2; R2b-142). `species_work/scripts/P6-RND_vectors.py` computes every value below with the model's own functions.
   - **`SET_C` and the T12 check.** `SET_C` becomes a smooth area over set A: generate `set_a()`, then apply `FLATTEN_SMOOTH` with centre (1350.0, 45.0, 1350.0), half-width 300.0 and threshold 18.3, with the two in-place passes of TER-141.
     - The T12 check compares the first evaluation of pass 1 at sample (68, 68).
     - Its inputs: cur 89.06810992480656; neighbours W 62.64300369864562, E 91.00231727471159, N 90.58030110349861, S 62.691642555223666.
     - It also checks the counts over the whole area: 2 888 above-threshold evaluations, 377 of them differing when the neighbours are summed first.
   - **`EXPECTED_C`** = `{'v': 45.0, 'sums': [107.64300369864563, 198.64532097335723, 289.22562207685587, 351.91726463207954], 'result': 58.65287743867992, 'neighbours first': 58.652877438679916}`.
   - **`SET_D_AREAS`**: the second entry `(60, 24.1, 60, 8, FLATTEN_ADD)` becomes `(60, 18.7, 60, 8, FLATTEN_ADD)`.
   - **`EXPECTED_D`**, rows after areas 2 to 5:
     - after area 2: `[[5, 5, 5], [5, 23.700000762939453, 5], [5, 5, 5]]`;
     - after area 3: `[[5, 5, 5], [5, 19.700000762939453, 5], [5, 5, 5]]`;
     - after area 4: `[[2, 2, 2], [2, 19.700000762939453, 2], [2, 2, 2]]`;
     - after area 5: `[[-2.5, -2.1083333227369523, -2.5499999788072376], [-2.1083333227369523, -0.9888888818246349, -1.358333322736952], [-2.549999978807237, -1.358333322736952, -3.619444440912318]]`.
   - **`EXPECTED_T2`**: `(24.1, 24.100000381469727), (11.7, 11.699999809265137)` become `(18.7, 18.700000762939453), (18.3, 18.299999237060547)`.
5. **Coordinator, informational (P6-X 4, P6-DAT note).** `render.worker_scale_factor`, `render.structure_detail` and `rnd.option.worker_scale_factor.default` are not renamed; only the C++ members are. The data-schemas row stays as written.
6. **Coordinator (P6-RC has finished), `reference/04_supply_crates_powers_and_weapons/crates_and_powers.md` R-CRATE-206** (R4a-23). "A blighted tree follows the blight rules" becomes "A blight tree follows the blight rules" (the glossary term; RND-188).
7. **Coordinator (P6-RB has finished), `reference/03_buildings_and_world_objects/defences_and_props.md` R-BLDG-475** (R4b-35). "× |sin t| (0.3 + 0.7 × |sin t|) over the presentation time t." becomes "× |sin t| (0.3 + 0.7 × |sin t|) over the presentation frame time t (DEC-959 item 2)." This matches RND-173.
8. **P6-DEC, optional, `decisions/deferred/numerics_lobby_terrain_services.md` DEC-247 items 7 and 10.** TER-240 and TER-241 no longer quote reference-terrain totals. They use the performance terrain of terrain T26: 334 × 334 samples and 25 tiles of 257 × 257, making 6 527 800 calls and holding 0.89 MB of heights. Item 10 may add "the performance terrain of terrain T26 makes 6 527 800 calls". No change is required: the items are design inputs.
9. **P6-DEC, `decisions/engine_gap_rules.md` DEC-961, Options** (R1b-4). "B — the reference behaviour: the place and size are computed once, when the panel is created, so a panel open across a resize keeps the old layout." becomes "B — the reference behaviour: the panel is laid out again by the R-META-224 formulas when the screen mode changes, and not when the DPI or the text scale changes." The Recommendation's "and a panel computed once is drawn in the wrong place after a resize" becomes "and a panel laid out only on screen-mode changes keeps a stale size after a DPI or text-scale change".
10. **P6-DEC, `decisions/deferred/process_tick_and_roadmap.md` DEC-222, "Specified behaviour"** (R3b-21). Append: "Until it is decided, every presentation reader reads the baseline snapshot contract of [renderer](../../architecture/components/renderer/README.md) RND-018 and RND-019: the game layer produces one immutable `PresentationSnapshot` per rendered frame, after the frame's update slices, and the engine renderer sees it only through `Engine::SceneSnapshot`."
11. **P6-DEC, `decisions/engine_gap_rules.md` DEC-955, Options (line 100)** (R2b-162; the key is deleted). "a cloth flag keeps relaxing with a step of `vfx.cloth.still_step` (0.1 s) per frame" becomes "a cloth flag keeps relaxing with a fixed step of 0.1 s per frame".
12. **P6-GLO, `design_next/CONVENTIONS.md`.**
    - **8.12-B, Renderer `Engine`, "frame, configuration and view":** after `StartupError` add "(`NoCapableAdapter`, `DeviceCreationFailed`, `SwapChainFailed`, `PipelineCreationFailed`, `FrameGraphInvalid`)". After `FrameKind` add "(`Match`, `MainMenu`, `Loading`, `Editor`)". Add `ColorRgba`, `SceneSnapshot`, `GpuResourceFactory`, `GpuResourceId`, `GpuResourceError` (`InvalidDescription`, `OutOfVideoMemory`, `StagingFull`, `DeviceRebuilding`, `UnknownResource`), `GpuTextureDesc`, `GpuBufferDesc`, `GpuSubresourceData`, and the `FrameRenderer` members `Resources` and `SetPalette`.
    - **8.12-B, "pipeline and frame graph":** add `UploadAllocation`.
    - **8.12-B, "world renderers and batches":** delete `SkyDetail` (`DetailLevel` replaces it).
    - **8.12-B, "shader layouts and options":** after `DetailLevel` add "(`None`, `High`, `Medium`, `Low`)". Add the shared headers `FrameConstants.h` and `ParticleSlot.h` and the shared-header type aliases `Float2`, `Float3`, `Float4`, `Float3x3`, `Float3x4`, `Float4x4`, `Uint32`.
    - **8.12-B, "shaders and permutation macros":** `COMBINE` (`MODULATE`, `REPLACE_THEN_MODULATE`) becomes `COMBINE` (`PRODUCT`, `TEXTURE_COLOR`). Add the compute shaders `ParticleEmit`, `ParticleSimulate`, `ParticleCompact` and `ShardUpdate`, which VFX owns (RND-263).
    - **8.12-B, Renderer data names:** "combine modes `modulate`, `replace_then_modulate`" becomes "combine modes `product`, `texture_color`".
    - **8.12-B, Renderer `Species`:** add `MatchSetupView` (renderer §3.5).
    - **8.12-B, Animation and VFX `Species` types:** add `ExplainerTeamColors`.
    - **8.12-B, `EffectKind` enumerators:** delete `OrbBroken`.
    - **8.14 Renderer:** "blighted tree", "Essence Grove tree" and "turning blighted" become "blight tree", "essence tree" and "corrupted tree". Keep "structure detail" and "Worker distance scale factor" as setting terms: the members are `decorationDetail` and `crowdCardDistanceScale`, and the keys are unchanged.
    - **8.12-A:** `LoadingView` and `MapEditor` stay game types; the engine uses `FrameKind::Loading` and `FrameKind::Editor`.
13. **P6-GLO, `design_next/assets/`.** No new asset rows. The cursor-shadow rows are already P6-RC request 13.
14. **P6-ADR, `architecture/components/build_tooling_and_distribution.md` table 3.1-A (optional, R3b-17).**
    - Server row, "Links": "the engine core, the game rules, the server library, the platform layer without a window" becomes "the engine core, the content library of AST-127, the game rules, the server library, the platform layer without a window".
    - Client row, "Links": after "the renderer" insert ", the client resource library of AST-127".
15. **P6-ADR, ADRs.**
    - **ADR-058 "Layout" (R3b-18).** Add the note "Amendment 2026-10-18. The byte layout is fixed in [asset pipeline](../architecture/components/asset_pipeline_and_resources.md) AST-128 (tables 3.12-A to 3.12-C: little-endian, a 128-byte header with the magic `SPCPACK\0`, 24-byte chunk records, entry records of 57 + key + sidecar bytes, the TOC hash over the TOC as stored). The mount manifest and the content manifest are cooked records (`assets.mount_manifest`, `assets.content_manifest`, AST-129)."
    - **ADR-016 "Depth bias" (R2b-132).** "Where the look is defined as a near plane raised by a factor *k*, the renderer converts it to a constant bias of round((*k* − 1) × 2²⁴) units with no slope term (RND-220)." becomes "A depth-biased variant has a bias factor *k* above 1 (RND-219), which the renderer converts to a constant bias of round((*k* − 1) × 2²⁴) units with no slope term (RND-220)." "and never gives less than the raised plane" becomes "so the draw wins depth ties against unbiased coplanar surfaces". Add the note "Amendment 2026-10-18. The depth-bias variants are stated as bias factors (RND-219, RND-220), not as a raised near plane."
    - **ADR-003 (optional).** Mention that shaders compile with `-Zpr` (row-major packing, RND-260), so that the C++ and HLSL matrix layouts agree.
16. **P6-ENG2, `architecture/components/diagnostics_test_and_performance/test_harness_soak_and_telemetry.md` §4.8, golden-image row** (R3b-23). "fixed scenes rendered on the pinned reference configuration and compared with per-test tolerances; reference images are reviewed on change;" becomes:
    > "fixed scenes rendered on the pinned reference configuration, the WARP adapter of the Windows SDK until the GPU agent of ADR-070 exists, and compared with a tolerance: by default each 8-bit channel of every pixel within one code value of the reference image; a test may state another tolerance only in its own row. A reference image is produced by the test's own scene on the reference configuration and is reviewed and committed with the change that alters it;"

    The harness notes of renderer, VFX and AST already cite "the default tolerance and the pinned reference configuration of DBG §4.8".
17. **P6-ENG2, `architecture/components/platform_layer.md` PLT-112** (R2b-117, ruling 4).
    - "After any successful attempt the settings in effect are written back; after a fallback the present-mode setting is written back as `Uncapped` (RND-023); then the store is saved (R-META-424)." becomes "After any successful attempt the size and the window mode in effect are written back and the store is saved (RND-023, R-META-424); a fallback never changes the present-mode setting."
    - In the figure, `F["Write back the settings in effect, present mode Uncapped; save the store"]` becomes `F["Write back the size and window mode in effect; save the store"]`. This matches R-META-423 and RND-023.
18. **P6-ENG1.**
    - **(a) `camera_and_sequences.md` CAM-004 (R3b-21).** "the presentation snapshot of the newest tick" becomes "the presentation snapshot of the frame ([renderer](renderer/README.md) RND-018, RND-019)".
    - **(b) `audio_engine/README.md` AEN-041 (R3b-21).** "read from the newest immutable snapshot" becomes "read from the frame's presentation snapshot ([renderer](../renderer/README.md) RND-018, RND-019)".
    - **(c) `audio_engine/README.md` AEN-102 (R3b-26).** Append: "The wait counts towards the match-entry budget `ast.load.match_entry_budget_s` ([asset pipeline](../asset_pipeline_and_resources.md) AST-130)."
    - **(d) `ui_framework/pages_and_screen_recipes.md` UI-473 (R3b-25).** "and their millisecond budgets are set from the prototype measurement together with the frame budgets (DEC-106)." becomes "and their millisecond budgets lie within the overlay pass group's provisional `rnd.budget.overlays_ms` (2.0 ms, [renderer](../renderer/options_and_budgets.md) RND-454), which the prototype measurement confirms or changes (DEC-106)."
19. **P6-ENG1, `audio_engine/README.md` (R4a-15).** In the §2.1 sketch, `bool SetLoadingView(bool _shown);           // AEN-111` becomes `bool SetLoadingShown(bool _shown); // AEN-111` (reformat), and in the table "`SetLoadingView` | the loading-view gate" becomes "`SetLoadingShown` | the loading-view gate". Any prose use follows. The renderer's frame kind is now `FrameKind::Loading`.
