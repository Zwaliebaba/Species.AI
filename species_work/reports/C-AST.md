# C-AST report — asset pipeline and resources

Date: 2026-10-17. Package: `species_work/packages/C-AST.json`. No publishing, sharing or upload tool was used; no git write command was run.

## Files written

| File | Size | Notes |
|---|---:|---|
| `design_next/architecture/components/asset_pipeline_and_resources.md` | 107,224 B | component template §6.4 (§1–§8) plus §9 Tuning table, as the renderer files do |
| `species_work/scripts/C-AST_check.py` | | self-check: ids, keys, DEC/ADR/asset/requirement references, Open DECs, headings, truncation, links, size |
| `species_work/scripts/C-AST_renumber.py` | | renumbers AST ids in document order and remaps every reference |

Inputs read: every KEEP and MERGE range of the package (25 §2 intro, §2.1, §2.3, §2.4, §2.6, §2.7, §2.9–§2.13, §5.1, §7 intro, §7.1, §7.2, §7.4; 29 §10.1; 31 §8.7, §8.8), the shared reads, the ADRs of the texture, mesh, audio-format, pack, content-build, identity and schema decisions, and the cross-document sections of X-AST, C-RND-a, C-RND-b, G03a and G08b (plus the C-AST items in X-ADR-a, X-DEC-2b, X-DEC-3, X-DEC-5a and X-GLO-1).

## Checks

- Forbidden scan: 0 hits. Soft scan: 0 hits (two "shipped" reworded).
- `C-AST_check.py`: OK (126 ids AST-001–AST-126, unique, ascending, all in 001–299; every cited AST, R-, RND and TER id defined; every DEC and ADR exists; every Open `decs_to_cite` cited by a requirement; every asset id in the manifest; every `ast.*` key unique in `design_next/` and cited by a requirement; every cited `rnd.*`/`ref.*` key defined; no truncation words; every link on disk or a planned target).
- Mermaid: 4 of 4 blocks parse (mermaid 11.4.1 through `G06_mermaid.mjs`).
- No match against `original_strings.txt` or `ip_tokens.txt`.
- `accept_package.py`: only "report missing" before this file existed.

## Counts

- Requirements: 126 (AST-001 to AST-126).
- Tuning keys: 25 `ast.*` keys; 12 keys of other documents cited by name (`rnd.mesh.*`, `rnd.shadow.icon_blur_radius`, `rnd.look.blur_normaliser`, `rnd.crawler.trail_*`, `rnd.pso.*_cutout`, `ref.map.thumbnail.aspect`).
- Diagrams: 4 (pipeline overview flowchart; key look-up and override flowchart; load state machine; asynchronous-load sequence).
- Enumerations and tables: 15 captioned tables (manifest columns 15; cookers by category; import-record fields 11; texture kinds 6; manifest `alpha_mode` mapping 14; derivations 5; usage conventions 12; socket kinds 34; audio classes 5; other content types 9; pack set 5; caches 7; placeholder generators 16; budgets 5; boundaries 8) plus the event, failure-mode (17 rows), acceptance-test (30), open-decision (19 rows) and tuning tables; closed C++ enumerations `ContentType` (15), `TextureKind`, `ColourSpace`, `AlphaMode`, `MipPolicy`, `SamplerId`, `Derivation`, `TintMarking`, `PackCodec`, `ReadPriority`, `LoadState`.

## Open DECs cited

All Open entries of `decs_to_cite`: DEC-204 (AST-063, AST-086, AST-100), DEC-224 (AST-010, AST-011, AST-118), DEC-226 (AST-009, AST-011, AST-108, AST-112, AST-117, AST-118), DEC-246 (AST-051, AST-099), DEC-329 (AST-090), DEC-330 (AST-039, AST-041). Also Open: DEC-047, DEC-317. Resolved stated as rules: DEC-007, DEC-012, DEC-015, DEC-098, DEC-099, DEC-100, DEC-461, DEC-563, DEC-800, DEC-801. Dropped, listed in §8: DEC-332, DEC-340.

## New DEC requests

None. The open choices found are either covered by existing entries (DEC-329, DEC-330, DEC-226, DEC-204, DEC-317) or are ADR conflicts (next section). Two engine-baseline values were set without an ORIG source and should be confirmed by profiling, not by a DEC: `ast.pack.zstd_level` (19) and the run-time budgets of table 5-A (`ast.load.*`, `ast.mount.budget_ms`).

## ADR conflicts

I followed the ADRs in each case; the other documents need aligning or the ADR an amendment.

1. **ADR-0020 versus key-colour team marking.** ADR-0020 marks team regions explicitly (vertex weight or material flag) and rejects recolouring a reserved colour range "at cook or run time". RND-162 (last sentence) and RND-406, `art_and_audio_briefs.md` §2.5, GameAssets §2.4 ("team parts painted mostly red") and the manifest's mesh `alpha_mode` "opaque (team parts painted mostly red)" make the cooker infer tint weight 1 from red keys. AST-034/AST-035 name the attribute `_SPECIES_TINT` and the material flag `species_team_tint`, forbid inference, and allow an authoring helper that writes explicit weights from key colours for the artist to review.
2. **ADR-0018/ADR-0020 versus load-time cloth recolouring.** The 8 cloth rows (`marker.leader_banner`, `marker.flag_post_state`) say "team area recoloured per team at load" and "pure-red texels take the team colour". AST-022 uses a companion tint mask and shader tint instead (see Missing assets 2).
3. **ADR-0018 blend pairing versus RND-393/RND-423** (raised by C-RND-b too): straight-alpha markers, cursors and icons drawn with the `Additive` blend are a build error under AST-021, which states the ADR rule unchanged. ADR-0018 needs an amendment that allows `straight` with the additive (source alpha, one) blend, or those assets must change.
4. **ADR-0018 closed sampler set.** Besides C-RND-a's point-wrap request, there is no sampler that magnifies by nearest and minifies linearly, which `fx.particle_dot` (particle batches) and `sprite.worker.front` (front-end backdrop) use. AST-027 gives them a `point` view with point clamp, which differs under minification. Request: add a "linear minification, point magnification, clamp" sampler.
5. **ADR-0017 versus GameAssets §2.4 "padded to whole 4x4 blocks by the cooker".** ADR-0017 makes the validator reject block-compressed sources whose dimensions are not multiples of 4, and AST-068 follows it. Manifest rows that fail (top level including the 2× authoring scale): `help.explainer_picture` (140×110, 140×90, 158×256, 181×256, 207×256, 238×256, 50×256, 512×310), `help.general_page` (one member 300×298), `ui.logo_banner` (788×298), `ui.logo_emblem` (554×388), `ui.title_wordmark` (1318×244).
6. **ADR-0017 format table (minor).** CPU look-up tables (`terrain.ramp`, `terrain.shore_wave_gradient`) are `R8G8B8A8_UNORM_SRGB`. That is a format of the ADR's table, but there it is listed only for pixel-exact UI art; "exact data" lists `R8_UNORM` and `R8G8_UNORM`. The ADR could name CPU colour tables.
7. **RND-393 colour-space column.** It lists glow, beam and darkening families as sRGB. ADR-0017/ADR-0018 make single-channel masks `BC4_UNORM` and linear, and AST-020 follows that. Sampling is unchanged under ADR-0013, because both are read as stored values.

## Missing assets (X-AST pass 2)

1. **Derived-variant rows** (AST-028, AST-029). Each needs its own row, with the parent as its source and the placeholder "derived by the cooker": the blurred shadow of every shadowed cursor of R-CTRL-350 table 2.1-A (candidates `cursor.world`, `cursor.highlight`, `cursor.select`, `cursor.move_target`, `cursor.place`, `cursor.enter`, `cursor.unavailable`, `cursor.turret_aim`, `cursor.pointer`), of `marker.crate_landing` and of `marker.barrage_target`; and the Crawler trail of `sprite.crawler.segment`. Proposed ids `<parent id>.shadow` and `sprite.crawler.segment.trail`; keys `<parent key>@shadow` and `@trail`.
2. **Companion tint masks** for the cloth art (AST-022): `marker.leader_banner` (4 members) and `marker.flag_post_state` (4 members). Proposed ids `marker.leader_banner_tint` and `marker.flag_post_state_tint`; files `content/textures/icons/markers/leader_banner_<state>_tint.png` and `flag_post_<state>_tint.png`; BC4_UNORM, linear, 64×64 px; placeholder "full weight over the art's opaque area".
3. **Collision assets** for every collidable mesh (AST-044, ADR-0019). Proposed id root `collision.` mirroring the mesh id, for example `collision.structure.spawn_node`. Placeholder "primitive collision at the mesh's bounding shape". Candidate meshes (G03a/G03b to confirm from R-BLDG-042/043): `mesh.structure.spawn_node`, `source_node`, `relay_mast`, `beam_dish`, `command_console`, `command_console_linked`, `solar_array`, `grid_pylon`, `transit_gate`, `core_device`, `fuel_pump_housing`, `fuel_line_foot`, `fuel_depot`, `turret_head`, `turret_barrel`, `wall_section`, `barrier_emitter`, `hive_mound`, `spitter_plant_head`, and the post-launch `_alt` bodies. C-SIM should confirm whether any actor mesh is read by a mesh-against-mesh test.

## Cross-document requests

1. **C-RND-a.**
   - (a) RND-162's last sentence and RND-406 should point to explicit tint weights (AST-034) instead of key-colour inference (ADR conflict 1).
   - (b) `rnd.shadow.icon_blur_radius` (10) is not a radius. In the derivation it multiplies the 5-tap weights (a gain), and the taps cover ±2 px (AST-073); please rename the key or reword it.
   - (c) RND-422 says the kernel is applied "once horizontally and twice vertically". The input has one vertical pass whose weights are doubled (`ast.derive.blur_vertical_gain`); please align RND-422.
   - (d) For the coordinator: the same value 10 is also keyed as `ref.ctrl.cursor.shadow_blur`, `ref.crate.render.landing_marker_blur` and `ref.hazard.missile_barrage.marker_shadow_blur`.
2. **C-RND-b.** RND-393: change the colour-space column for the single-channel families to linear (conflict 7), and deal with the straight-alpha-with-additive pairing (conflict 3). The passes should request the variant ids `level0` (RND-153, RND-339, RND-378) and `point` (AST-026, AST-027).
3. **C-VFX.** The cloth-flag tint colours from 25 §2.13 need a home in the cloth renderer. In the team area the drawn colour is the tint colour × the area's grey level (the input's rule is: a pure-red texel of red value *r* becomes colour × *r* ÷ 255). The tint colour is (128, 128, 128) for no team, the team colour while the flag is selected or highlighted, and the team colour ÷ 2 otherwise. Also: the particle pass uses the `point` view of `fx.particle_dot` (AST-027), and effect definitions reload through `AssetReloaded` in development builds (ADR-0036, AST-121).
4. **G11 / C-UI.**
   - The explainer panel's layout when it is rebuilt (25 §2.12): scale = screen height ÷ 1300; x = screen width ÷ 2 − scale × width ÷ 2; y = screen height − 1.15 × height × scale; size width × scale by height × scale; border 5, title 25, caption 15.
   - C-UI: layout reads the display size of 2× art from `TextureSize` (AST-024, AST-113).
   - C-UI: the UI elements that need the `Greyscale`, `ToColour` or `ReplaceColour` processing (run-time UI helpers in the input) must declare them as derived variants (AST-075).
5. **C-AEN.** Please key the streaming threshold (X-AST proposes 20 s) and the resident budgets per category; AST-080 and AST-081 cite them by name only. The sample cache's banks follow load groups (AST-114).
6. **C-DAT.**
   - The import-record schemas (texture AST-019, mesh AST-033, audio AST-050).
   - The engine asset table record (AST-017).
   - The simulation data record (AST-046).
   - The list of record types the simulation reads (AST-098).
   - The baked tree crowns in the map schema (AST-047).
   - A "marked for release" flag on edition manifests (AST-085, AST-090), unless C-BLD owns it.
   - X-AST request 9 is answered: maps, explainer scripts and string tables are JSON (ADR-0062), and the cooked mesh is a `MeshAsset` pack entry with no container of its own (AST-052).
7. **C-BLD.**
   - The edition release flag.
   - The CI jobs of AST-066 (reproducibility) and AST-105 (no loose-mount code in profile and shipping binaries).
   - Installing the mount manifests in the pack folder (AST-056).
   - The repair function that uses the check of AST-107.
8. **C-PLT.** The start-up errors `ui.error.content_missing` and `ui.error.content_damaged` (AST-102) may appear before the UI framework exists, so the platform message path must show them. The loose-content start-up option exists in `Debug` and `Development` builds only (AST-105).
9. **C-DBG.** The performance tests for table 5-A and the "reference test machine" they assume (AST-125); the overlay content of AST-126.
10. **C-TER.** The shore mask is registered as a run-time texture through `ImageProducer` (AST-062). `terrain.ramp` and `terrain.shore_wave_gradient` are `CpuTable` entries (AST-072). A custom map's embedded ramp and gradient images arrive as CPU tables (AST-063).
11. **C-SIM (reference/12).** The simulation reads socket transforms from the `SimRecord` (AST-043, AST-046) and hit geometry from collision assets (AST-044). The member order of `mesh.relic.variant` is simulation data (AST-016).
12. **G03a / G03b.** Please confirm the collidable-mesh list (Missing assets 3) and the consumers of `gate_exit` and `stem` (table 3.8-A, DEC-330).
13. **G06.** Please list which cursors of table 2.1-A are shadowed, so that the derived-variant rows can be sized.
14. **X-AST (pass 2), manifest and GameAssets corrections.**
   - (a) The rows of ADR conflict 5.
   - (b) The cloth rows' `target_format` and `alpha_mode` text should name the tint mask.
   - (c) Mesh rows: "colours attenuated at load" should read at cook (RND-160, AST-076), and "team parts painted mostly red" should read explicit tint weights.
   - (d) The `cursor.world` spec says "blurred darkening shadow generated at run time"; it is derived offline.
   - (e) `mesh.creature.spider_body` has sockets `leg_00` to `leg_07`; AST-038 numbers from 01, so they become `leg_01` to `leg_08`.
   - (f) `sprite.visitor.front` has `alpha_mode` "straight alpha blend" while its `spec` gives an alpha-test threshold of 0.02; please choose one.
   - (g) GameAssets §2.4: "padded to whole 4x4 blocks" becomes "rejected", "one recolour per team at load" becomes a tint mask, and "colours attenuated at load" becomes at cook.
   - (h) The placeholder rule of the assets README should add that editions marked for release contain no placeholder (AST-085).
   - (i) X-AST request 10 is answered: the texture profiles are adopted, with BC4 linear; the 2× authoring rule is adopted (AST-024); the 20 s threshold goes to C-AEN; the placeholder rule is adopted (AST-082 to AST-085).
15. **X-GLO.** The "RGBA8 image" glossary entry mentions colour keying. Species has none; please align it with AST-061.

## Names needed that are not in the ledger (proposals)

- **Engine types (namespace `species::assets`):**
  - identity: `AssetId`, `VariantId`, `AssetKey`, `ContentKey`, `Sha256Digest`, `AssetError`;
  - mounting: `MountOptions`, `MountOverride`, `MountManifest`, `MountSet`, `EntryRef`, `PackEntry`;
  - content types and import records: `ContentType`, `TextureKind`, `ColourSpace`, `AlphaMode`, `MipPolicy`, `SamplerId`, `Derivation`, `TextureView`, `DerivedVariant`, `TextureImportRecord`, `TintMarking`, `MeshImportRecord`, `AudioImportRecord`, `PackCodec`;
  - reading and caching: `ReadPriority`, `ContentReader`, `LoadState`, `TextureSize`, `LoadGroup`, `TextureHandle`, `MeshHandle`, `RuntimeTextureId`, `ImageProducer`.
- **Events:** `AssetLoaded`, `AssetFailed`, `LoadGroupProgress`, `LoadGroupCompleted`, `AssetReloaded`.
- **Terms:** import record, view, derived variant, placeholder, residency list, load group, engine asset table, collision asset, simulation data record, mount manifest, content manifest, asset index, loose-content directory, run-time texture.
- **Fixed names:**
  - pack ids `startup`, `locale`, `core`, `audio`, `update_<nn>`;
  - the glTF attribute `_SPECIES_TINT`, the material member `species_team_tint` and the socket prefix `socket_`;
  - variant ids `level0` and `point`, with `shadow` and `trail` proposed.
- **New string ids:** `ui.error.content_missing` ("A game file is missing. Repair or reinstall the game.") and `ui.error.content_damaged` ("A game file is damaged. Repair or reinstall the game.").

## Dropped as legacy (one line per group)

- **Archive container and its tooling:** proprietary solid archives decompressed whole into memory; the archive tool, its version and switches; the per-archive entry cap; the start-up ordering that kept the non-thread-safe extractor single-threaded; silent skipping of missing or failing archives; dropping of zero-byte entries. Replaced by ADR-0058 packs (DEC-340).
- **Loose-file precedence in every build:** loose files beat archives everywhere; the compiled-out mod-directory search; editor saves into the data folder. Replaced by the development-only loose mount (AST-105) and the no-write rule (AST-106).
- **Edition archive contents:** the per-edition archive lists, the extension filter, the exclusion lists, the branding and patch archives with no builder, and the never-opened ambient-lighting archive (DEC-332).
- **Upload path:** load-time decoding of a superseded image format with bottom-up rows; rescaling to power-of-two, square or maximum sizes at upload; the compressed-upload flag that nothing requests.
- **Colour keys:** magenta-to-transparent-grey keying and green-channel-as-alpha conversion done in place on stored images, with their double-conversion hazard after a flush. Replaced by real alpha and mask kinds.
- **Texture cache:** per-call-site processing flags; the name-keyed cache whose first requester fixed the processing; case-variant duplicate entries; the lock taken only while the loading screen draws (DEC-098, DEC-224).
- **GPU state handling:** flush and regeneration of GPU state on a screen-mode change; precompiled geometry lists and their name generator; mesh copies per animating instance and per team; the dead rich-save thumbnail and the dead font up-scale.
- **Run-time texture assembly:** the Crawler atlas built at start-up; cursor and icon shadows blurred and registered at run time; banner and flag recolouring by pure-red key; gesture-overlay textures; the pixelation capture; the deform and water-reflection targets of the alternative back end. The kept ones are now offline derived variants or tint masks.
- **Sample handling:** the lazily decoding sample cache with LRU eviction, the two-folder name search for samples, the hand-run unused-sample scripts and the debug sound-editor billboard.
- **Pixel operations:** the dilate filter (no in-scope use), the byte-OR blit, red/blue swapping for captures and line drawing for gesture textures. Greyscale, to-colour and colour-replace are kept as offline derivations only.
- **Recommendation 8's lock-step data-contract clause** (DD-NET): replaced by the simulation content identity, whose exchange is DEC-204.
