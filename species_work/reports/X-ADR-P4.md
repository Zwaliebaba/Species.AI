# X-ADR-P4 report: ADR amendments (Phase 4)

Status: in progress.

## 1 Files written

(filled in at the end)

## 2 Item outcomes (phase4_dec_fixes.md)

| Item | Outcome |
|---|---|
| 37 | ADR-0011 amended: the wire overlay is a single-pass grid term with two sources, the stylised grid texture (default, DEC-326 option A) and the barycentric edge term (permutation, option B); title now "chunked terrain, single-pass wire overlay and GPU water"; chunk vertex list gains facet normal and grid coordinates; context says one colour per facet (C-TER note). ADR-0003 material 5 and the edge-mask bullet amended to match (RND-135, RND-264, RND-266). |
| 38 | ADR-0005 amended: blend list gains `ghost` (`ONE`, `SRC_COLOR`) for the placement ghost models (RND-058, RND-365). Also states front-facing as counter-clockwise on the render target, `FrontCounterClockwise = TRUE` (C-RND-a clarification), and the ink-with-alpha-0 = darkening equivalence (C-UI-a item 3). |
| 39 | ADR-0018 amended: no point-wrap sampler is added; point filtering with wrapping is a shader emulation (wrap the coordinates, sample with point clamp-to-edge), which is exact and is what RND-154 and RND-265 specify. The sampler set gains **point-magnify clamp-to-edge** (point magnification, linear minification and mips) for the particle dot and the front-end backdrop sprite (C-AST item 4; VFX README already specifies that filtering). Component fixes F1–F3 below. |
| 40 | ADR-0003 amended: material 4 has a specular term with per-material specular colour and shininess (half vector, viewer at infinity), used by the terrain grid and the special lighting rig. ADR-0013 lighting bullet amended to the RND-233 formula (diffuse + specular, clamped before texture, fog and blend). |
| 64 | `Ghost` blend: as item 38. Blend pairing: as item 84. |
| 83 | ADR-0020 is right and unchanged in substance; amendment note records `_SPECIES_TINT` and `species_team_tint` (AST-034), the authoring helper that writes explicit weights for artist review, and tint masks for cloth art (AST-022). Component fix F5 (renderer); the art briefs, GameAssets and manifest are X-AST-2a's (brief task 2). |
| 84 | ADR-0018 amended: `straight` alpha may pair with the additive blend (`SRC_ALPHA`, `ONE`) for glowing icons, markers and cursors (RND-393, RND-423); `opaque` textures take their material's blend (AST-021). Nearest-magnification sampler: see item 39. Derived variants are never produced at run time (C-RND-b item 3: the `cursor.world` row's "generated at run time" is an asset-row fix for X-AST). Component fix F4 (AST-021). |
| 80 | Closed by item 88 (same three conflicts, settled by C-AEN in the component). |
| 87 | ADR-0025 amended: residency per group by its longest member, music always streamed; interface bank (process lifetime) and match bank (per match) with reference-counted groups; budgets 192 MiB match, 8 MiB interface, 8 MiB streams (about 173 MiB measured); triggers before residency are dropped and counted; MS-ADPCM not needed at these budgets; the reference evicting cache is a reference model only. A residency test added to Confirmation. Status stays **Proposed**: the runtime codec is a choice the verdict authority leaves open (X-ADR-a status rule), and the default is unchanged; C-AEN's suggestion to accept it is left to the team. |
| 88 | ADR-0024 amended to the six player buses of DEC-570 under the pre-master bus, content-built effect buses (insert or send), the XAudio2 reverb with I3DL2 conversion for room reverb, and a settings test over six buses plus master. ADR-0027: the global maximum-distance default follows DEC-566 (10⁹ wu), so the default mix has no maximum distance. ADR-0023: copy decay and patience window are per-event design values with audio-design defaults; the engine owns the arithmetic, budget, caps and steal margin. ADR-0022: layer list follows AEN-002 (`SoundVoice` is the instance, `SourceVoiceSlot` the pooled source voice, `AudioBusGraph` the graph). ADR-0026: issue time on the real-time audio clock (platform clock), start time = issue time + scheduling latency (AEN-006, AEN-024, AEN-148). Stale `reference/10_audio_design.md` paths in 0022, 0023, 0024, 0027 corrected to the folder. |
| 95 | Note only, as the item says; ADR-0028 is authoritative and unchanged in substance. Its amendment note records that both back-ends sit behind one `DeviceBackEnd` interface (INP-105), and the C-INP camera-drag remark is applied: raw mouse deltas are a bindable control (`raw_motion`) with no default binding, not a camera-drag need. |
| 100 | Repeat clock decided as real time (the item's recommendation): ADR-0029 now measures repeat timers in real time on the platform clock (ADR-0074), never in scaled presentation time, matching input.md. Component fix F8 (UI-216). The binding-table merge is a document fix for the routers (not an ADR matter). ADR-0029 also records the radial dead-zone consequence for diagonal stick pan (C-INP item 4, playtest `ref.ctrl.camera.stick_gain`). |
| 102 | ADR-0016 amended: the camera owns the field-of-view values; the renderer owns the near and far planes (RND-211) and the field-of-view clamp (RND-212). Also records the renderer's depth-bias conversion for a raised near plane, round((k − 1) × 2²⁴) (RND-220; C-RND-a clarification). |
| 103 | ADR-0036 amended: camera shake is an additive view offset applied per frame and never stored in the camera pose (VFX-232, CAM-012), decaying over unscaled presentation time; the camera clock is separate, unscaled and runs while the world is held (CAM-030). The pick-ray DEC, the R-SIM-581 / R-CTRL-497 conflict and the new action ids are X-DEC-P4b, X-ARB and the routers' items. |
| 85 | ADR-0017 is right; amended only to state that the cooker never pads a block-compressed source to whole 4 × 4 blocks (AST-068). The five failing asset rows are X-AST-2a's fix (brief task 2). C-AST item 6: the format table now names CPU colour look-up tables (`R8G8B8A8_UNORM_SRGB`, `CpuTable`, AST-072). |

## 3 Report ADR remarks not in an item

| Source | Outcome |
|---|---|
| C-AST item 2 (cloth recolouring at load) | ADR-0018/ADR-0020 are right; ADR-0018 now says no texel colour is recoloured at load; the 8 cloth asset rows are X-AST-2a's; RND-406 is F5. |
| C-AST item 6 (CPU colour tables) | ADR-0017 format table amended (see item 85). |
| C-AST item 7 (RND-393 colour space) | ADR-0013 note added; component fix F6. |
| C-RND-a item 5 (match message inside the scene passes) | ADR-0012 amended: the UI is drawn at native resolution except the match message, which the pass table draws among the scene passes at render resolution (RND-072); identical at render scale 1. |
| C-RND-a item 6 (clarifications) | ADR-0005 front-facing rule and ADR-0016 depth-bias conversion added; ADR-0076 opaque-alpha PNG needs no change (RND-114 keeps the colour channels). |
| C-RND-b item 3 (`cursor.world` run-time shadow) | ADR-0018 is right (offline only, now explicit); asset-row fix for X-AST. |
| C-RND-b item 4 (`QuadInstance` fields) | Fits ADR-0009; component fix F7. |
| C-RND-b item 5 (defaults left open) | ADR-0012 amendment note cites the renderer's setting defaults (vertical sync, brightness 1, HDR highlight limit at the display maximum; RND-442); ADR-0014 unchanged. |
| C-TER note (per-vertex colour) | ADR-0011 context now says one colour per facet. |
| C-UI-a item 1 (tool windows and DPI) | ADR-0033 amended: tool-window lengths × DPI scale × UI scale (UI-170). |
| C-UI-a item 2 (minimum text size) | ADR-0064 amended with the interpolation rule of UI-171. |
| C-UI-a item 3 (ink and darkening) | ADR-0031 and ADR-0005 state the equivalence (UI-180, RND-058). |
| C-UI-a item 4 (relayout and R-META-450) | ADR-0033 note records that the framework relays out without losing state and the options apply then rebuilds (UI-238); no conflict. |
| C-UI-a item 5 (budgets and glyph pages) | ADR-0034 adopts 1024 × 1024 `R8_UNORM` pages, at most 8 (UI-198); ADR-0031 cites the UI budget of UI-240. |
| C-UI-b (deferred page switch; R-CTRL-223) | ADR-0032 is right and unchanged; R-CTRL-223 is a G06 document fix (item 98, routed by X-ROUTE-a). |
| C-VFX (three clock readings; one ExecuteIndirect; ADR-0064 Proposed) | ADR-0036 and ADR-0010 amended; ADR-0064's status is unchanged (the VFX flash and motion rules use its Proposed default). |
| C-CAM (camera clock separate from the presentation clock) | Recorded in ADR-0036. |
| C-INP item 3 (camera-drag premise) | ADR-0028 amended (see item 95). |
| G09b (display-mode options, Apply re-creating the renderer, ADR-0064 for information) | No ADR change: G09b records them as reference behaviour with "no Species equivalent" notes. |
| X-GLO-2 (ADR-0025 against the reference LRU cache) | Settled by C-AEN and now stated in ADR-0025 (item 87); the glossary entries are X-GLO's. |
| G00, G01, G02a1, G02a2, G02b, G03a, G03b, G04, G05, G06, G07, G08a, G11, C-SIM-a, C-SIM-b, C-SIM-c, X-GLO-1 | "None"; nothing to do. |

## 4 Component fixes needed (for the document fixers)

| # | Target | Fix |
|---|---|---|
| F1 | `architecture/components/renderer/shaders.md` RND-262 | add the fourth fixed sampler index of ADR-0018: `kSamplerPointMagnifyClamp` = 3 (point magnification, linear minification and mips, clamp-to-edge). |
| F2 | `architecture/components/asset_pipeline_and_resources.md` §2 `SamplerId`, table 3.3-A row "sampler", AST-025 | add `PointMagnifyClamp` / "point-magnify clamp-to-edge" (art that the look magnifies into hard-edged blocks but minifies smoothly) to the closed set. |
| F3 | same file, AST-027 | `fx.particle_dot` and `sprite.worker.front` use the point-magnify clamp-to-edge sampler (as their sampler, or through a view that names it); delete the sentence that the view samples the nearest level under minification because the closed set lacks the sampler. |
| F4 | same file, AST-021 | pairing list becomes: `straight` with the alpha blend or with the additive blend (glowing icons, markers and cursors, RND-393, RND-423); the other three pairings unchanged. |
| F5 | `architecture/components/renderer/world_renderers.md` RND-162 (last sentence) and `visual_language.md` RND-406 | replace "authored as mostly red key colours ... the mesh cooker converts those facets to tint weight 1" by: team regions are marked by the tint attribute `_SPECIES_TINT` or the material flag `species_team_tint` (AST-034, ADR-0020); key-colour art reaches the cooker only as explicit weights written by the authoring helper. Replace "Cloth banners and flags mark their team area in pure red (255, 0, 0)" by: cloth art carries the team area as grey levels with a companion tint mask (AST-022). Look unchanged (item 83). |
| F6 | `architecture/components/renderer/visual_language.md` RND-393 | the colour-space column gives the glow, beam and darkening mask families as `linear` (`BC4_UNORM`), as AST-020 and ADR-0017 do (C-AST item 7). |
| F8 | `architecture/components/ui_framework/layout_rendering_input_and_localisation.md` UI-216 | "measured in presentation time" becomes "measured in real time on the platform clock (ADR-0074, ADR-0029)", so menu navigation keeps repeating while a match is paused (item 100). |
| F7 | `architecture/components/renderer/shaders.md` RND-262 | the shared `QuadInstance` layout lists the per-end colours of the `Segment` mode (RND-319) and the billboard blend start and length of the `WorldOriented` mode (RND-306) (C-RND-b item 4; fits ADR-0009 unchanged). |

## 5 Windows SDK verification

## 6 Checks

## 7 Not done, and why
