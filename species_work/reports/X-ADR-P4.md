# X-ADR-P4 report: ADR amendments (Phase 4)

Status: done. Owner files only: `design_next/ADR/*.md` and `design_next/ADR/README.md`. No git write, no publishing. External source used: Microsoft Learn only, for the Windows SDK release (§5).

## 1 Files written

New record:
- `design_next/ADR/ADR-077-client-http-stack.md` (8,343 B), Proposed, default the in-box WinHTTP (item 116).

Amended records (each has an "Amendment 2026-10-17" note as the last bullet of *More Information*; MADR 4 structure and numbers unchanged; sizes after the edit):

| ADR | Bytes | ADR | Bytes | ADR | Bytes |
|---|---|---|---|---|---|
| 0003 | 8,977 | 0022 | 7,591 | 0051 | 6,667 |
| 0005 | 8,072 | 0023 | 6,513 | 0054 | 5,585 |
| 0010 | 7,030 | 0024 | 6,768 | 0057 | 4,534 |
| 0011 | 7,163 | 0025 | 8,855 | 0061 | 6,296 |
| 0012 | 8,207 | 0026 | 6,997 | 0062 | 5,909 |
| 0013 | 7,507 | 0027 | 7,144 | 0063 | 7,032 |
| 0016 | 6,493 | 0028 | 7,411 | 0064 | 5,176 |
| 0017 | 7,677 | 0029 | 7,966 | 0065 | 6,259 |
| 0018 | 8,217 | 0031 | 5,894 | 0066 | 4,620 |
| 0020 | 7,116 | 0033 | 6,525 | 0068 | 4,824 |
| 0034 | 9,000 | 0036 | 8,242 | 0069 | 4,335 |
| 0050 | 5,343 | 0070 | 4,964 | 0071 | 6,058 |
| 0074 | 4,520 | 0076 | 3,904 | | |

That is 38 amended records. ADR-035 (5,955 B) had only a stale path corrected (`animation_and_vfx.md` → `animation_and_vfx/`), with no change of substance and so no amendment note.

`design_next/ADR/README.md` (49,508 B): the index gains ADR-077 and ADR-011's new title, plus a line saying that amended records carry an amendment note. Technology baseline: new summary row **2.4 Windows SDK**, a new §2 bullet with its sources, §1 "launcher" wording corrected (item 116), the §3 preset-set line aligned with ADR-054 (item 108), and one new line under "Items not verified" (the WinHTTP behaviours of ADR-077).

Helper scripts in `species_work/scripts/`, named with this agent's id followed by `_extract`, `_apply`, `_paths` and `_ids` (run with `python3 -I`): extract pulls out the ADR sections of the reports; apply makes exact, unique-match replacements from a spec file; paths checks paths and links in ADRs; ids checks that cited requirement and DEC ids exist.

## 2 Item outcomes (phase4_dec_fixes.md)

| Item | Outcome |
|---|---|
| 37 | **ADR-011 amended.** The wire overlay is a single-pass grid term with two sources: the stylised grid texture is the default (DEC-326 option A) and the barycentric edge term is a permutation (option B). Title is now "…chunked terrain, single-pass wire overlay and GPU water". The chunk vertex list gains facet normal and grid coordinates, and the context says one colour per facet (C-TER note). **ADR-003** material 5 and its edge-mask bullet were amended to match (RND-135, RND-264, RND-266). |
| 38 | **ADR-005 amended.** The blend list gains `ghost` (`ONE`, `SRC_COLOR`) for the placement ghost models (RND-058, RND-365). Two more additions: front-facing is stated as counter-clockwise on the render target with `FrontCounterClockwise = TRUE` (C-RND-a clarification), and an ink draw with alpha 0 is stated to equal darkening (C-UI-a item 3). |
| 39 | **ADR-018 amended.** No point-wrap sampler is added. Point filtering with wrapping is done in the shader: wrap the coordinates, then sample with point clamp-to-edge. This is exact and is what RND-154 and RND-265 already specify, so the renderer is unchanged. The closed set gains **point-magnify clamp-to-edge** (point under magnification, linear under minification and between mips) for `fx.particle_dot` and `sprite.worker.front` (also C-AST item 4). The VFX README already specifies that filtering; AST-027 does not. Component fixes F1–F3. |
| 40 | **ADR-003 amended**: material 4 has a specular term with per-material specular colour and shininess (half vector, viewer at infinity), used by the terrain grid and the special lighting rig (RND-229, RND-233, RND-266, RND-270). **ADR-013**: the lighting bullet now follows the RND-233 formula (diffuse plus specular, clamped before texture, fog and blend). |
| 64 | `Ghost` blend: as item 38. Blend pairing: as item 84. |
| 70 | **ADR-063 amended** (item direction followed). The record now covers only the mechanism: per-key fallback, a coverage report per edition, and the offered-language list built from that report. Which languages ship is product policy in DEC-099. The sign-off exception is removed, so the record agrees with DEC-099, R-META-463, AST-088 and BLD-081. G09b's minor alignment is applied: the start-up order is stored setting → OS preferred languages with tag fallback → `en-GB` → first offered language (R-META-465). Two tests were added under Confirmation. |
| 80 | Closed by item 88: the same three conflicts, already settled by C-AEN in the component. |
| 83 | ADR-020 is right and its substance is unchanged. The amendment note records `_SPECIES_TINT` and `species_team_tint` (AST-034), the authoring helper that writes explicit weights for the artist to review, and tint masks for cloth art (AST-022). Renderer fix: F5. The art briefs, GameAssets and the manifest belong to X-AST-2a (its brief, task 2). |
| 84 | **ADR-018 amended.** `straight` alpha may pair with the additive blend (`SRC_ALPHA`, `ONE`) for glowing icons, markers and cursors (RND-393, RND-423). `opaque` textures take their material's blend (AST-021). Derived variants are offline only, never produced at load or at run time. Team colour on textured art comes from the tint mask, never from recolouring at load. For the nearest-magnification sampler, see item 39. Component fix: F4. |
| 85 | ADR-017 is right. It is amended only to state that the cooker never pads a block-compressed source to whole 4 × 4 blocks (AST-068); the five failing asset rows are X-AST-2a's. C-AST item 6 is also applied: the format table names CPU colour look-up tables (`R8G8B8A8_UNORM_SRGB`, uncompressed, never uploaded; `CpuTable`, AST-072). |
| 87 | **ADR-025 amended.** Residency is decided per group by its longest member, and music is always streamed. There are two banks with reference-counted groups: interface (process lifetime) and match (per match). Budgets are 192 MiB match, 8 MiB interface and 8 MiB streams; about 173 MiB is measured. Triggers that arrive before residency are dropped and counted. MS-ADPCM is not needed at these budgets. The evicting reference cache is a reference model only. A residency test was added. **The status stays Proposed**: the X-ADR-a rule is "Proposed where the verdict authority leaves the codec choice", and the default is unchanged. Whether to accept it, as C-AEN suggests, is left to the team. |
| 88 | **ADR-024**: six player buses of DEC-570 under the pre-master bus; content-built effect buses (insert or send); the XAudio2 reverb with I3DL2 conversion for room reverb (AEN-070, AEN-220, AEN-223, AEN-224); the settings test now covers six buses plus master. **ADR-027**: the global maximum-distance default follows DEC-566 (10⁹ wu, AEN-202). **ADR-023**: copy decay and patience window are per-event design values with audio-design defaults (AEN-063). **ADR-022**: the layer list follows AEN-002 (`SoundVoice` is the instance, `SourceVoiceSlot` the pooled source voice, `AudioBusGraph` the graph). **ADR-026**: issue time is read on the real-time audio clock, and start time = issue time + scheduling latency (AEN-006, AEN-024, AEN-148). Stale `reference/10_audio_design.md` paths were corrected in 0022, 0023, 0024 and 0027. |
| 92 | No ADR change. ADR-032 already requires keyboard-only operation (its Confirmation runs a keyboard-only navigation test), so the new Resolved DEC belongs to X-DEC-P4b. |
| 95 | Note only, as the item says. ADR-028 is authoritative; its amendment note records the single `DeviceBackEnd` interface (INP-105). The C-INP camera-drag remark is applied: raw mouse deltas are a bindable control (`raw_motion`) with no default binding. |
| 98 | No ADR change. ADR-032 ("drawing never changes state") is right; R-CTRL-223 and R-META-080/406 are document fixes for the routers. |
| 100 | Repeat clock decided as **real time**, the item's recommendation. ADR-029 now measures repeat timers on the platform clock (ADR-074), never in scaled presentation time, as input.md does. Component fix: F8 (UI-216). The binding-table merge is a document fix, not an ADR matter. ADR-029 also records the radial dead zone's effect on diagonal stick pan for playtest (C-INP item 4). |
| 102 | **ADR-016 amended**: the camera owns the field-of-view values (CAM-005, CAM-012); the renderer owns the near and far planes (RND-211) and the clamp (RND-212). It also records the depth-bias conversion for a raised near plane, round((k − 1) × 2²⁴) (RND-220; a C-RND-a clarification). |
| 103 | **ADR-036 amended**: camera shake is an additive view offset applied each frame, never stored in the pose, and it decays over unscaled presentation time (VFX-232, CAM-012). The presentation clock has scaled, unscaled and real readings (VFX-009, VFX-016), and the camera clock is separate, unscaled and runs while the world is held (CAM-030). The pick-ray DEC, R-SIM-581 against R-CTRL-497 and the new action ids belong to X-DEC-P4b, X-ARB and the routers. |
| 108 | **ADR-054**: presets are named `<os>-<compiler>-<flavour>-<product>` with one product per configure (ADR-056), and workflow presets `ci-<tier>-<job>` (BLD-006). **ADR-070**: per-change jobs use the product-suffixed presets. **ADR-056**: no change needed. **README §3**: preset-set line aligned with ADR-054. **ADR-063**: see item 70. **ADR-057**: the identity is split into a generated header (every field except the content hash) and the packaged `build_identity.json` (every field), composed at start-up; the channel tag is not part of the identity (BLD-019 to BLD-021). **ADR-071**: the install list gains `build_identity.json`, `channel_tag.txt` and `licences\` (BLD-023). **Windows SDK**: verified and added (§5). |
| 111 | No ADR change. The minimum-specification DEC cites ADR-050 and ADR-002 as they are. |
| 112 | **ADR-066**: "benchmark scene" becomes "repeatable performance scene" (DBG-183). For consistency, **ADR-069**'s "benchmark executables" now reads "performance-test executables with repeatable scenes". |
| 116 | **ADR-074**: the source can be replaced after the settings load in `Debug` and `Development` builds, with continuity (PLT-049, PLT-051, DBG-241). **ADR-065**: the crash path belongs to PLT (PLT-118 to PLT-125); BLD owns symbols and LocalDumps settings and DBG owns the black box; no user name appears and paths use placeholders (PLT-035, PLT-125). **ADR-050**: there is no launcher; the platform start-up check shows a notice and continues (PLT-040). The README §1 line was fixed to match. **ADR-071**: user data gains `maps\` (R-MAP-391; this also closes G08b's gap) and `diagnostics\` (PLT-017). **ADR-066**: the HTTPS client is PLT's, on ADR-077. **New ADR-077**: in-box WinHTTP, HTTPS only (redirects too), OS certificate validation and TLS policy, system proxy, no implicit credentials, runs on the PLT worker thread, and a second back-end for a non-Windows server behind the same interface. Status is **Proposed** because the baseline has no HTTP item; the default is WinHTTP. |
| 119 | **ADR-061**: simulation values are stored in the representation of the current numeric default (DEC-241), binary32-rounded or binary64 per field, until DEC-240 decides (DAT-010, DAT-023). Omitted fields take their declared defaults (DAT-012, DEC-567). Engine definition tables such as the effect-kind table are compiled into the engine with no pack entry (DAT-273). **ADR-062**: the draft 2020-12 is pinned (DAT-015); the strict parser is Species-owned (RFC 8259 with I-JSON) and the validator is generated, with no third-party JSON library (DAT-019 to DAT-022); numbers use `std::from_chars` and `std::to_chars` (DAT-023, DAT-041). The decision driver and one consequence were updated to match. The AEN-060 and AST table 3.10-A citations of DAT-273 are fix F11. |

## 3 Report ADR remarks not in an item

| Source | Outcome |
|---|---|
| C-AST item 2 (cloth recoloured at load) | ADR-018 and ADR-020 are right; ADR-018 now says nothing is recoloured at load. The 8 cloth rows belong to X-AST-2a; RND-406 is F5. |
| C-AST item 6 (CPU colour tables) | ADR-017 format table amended (item 85). |
| C-AST item 7 (RND-393 colour space) | ADR-013 note added; component fix F6. |
| C-RND-a item 5 (match message inside the scene passes) | ADR-012 amended: the UI is drawn at native resolution except the match message, which is drawn among the scene passes at render resolution (RND-072). |
| C-RND-a item 6 (clarifications) | Front-facing rule added to ADR-005 and the depth-bias conversion to ADR-016. ADR-076 now says posters are written with opaque alpha (RND-114) and records PLT-097's numeric file-name suffix. |
| C-RND-b item 3 (`cursor.world` run-time shadow) | ADR-018 is right (offline only, now explicit); the asset row is X-AST's fix. |
| C-RND-b item 4 (`QuadInstance` fields) | Fits ADR-009; component fix F7. |
| C-RND-b item 5 (defaults the ADRs leave open) | ADR-012's note cites RND-442's defaults (vertical sync, brightness 1, HDR highlight limit at the display maximum). |
| C-TER note (per-vertex colour) | ADR-011 context now says one colour per facet. |
| C-UI-a items 1–5 | ADR-033: tool-window lengths × DPI scale × UI scale (UI-170), plus a relayout note (UI-238). ADR-064: minimum text interpolation (UI-171). ADR-031 and ADR-005: ink with alpha 0 equals darkening (UI-180); ADR-031 cites the UI budget (UI-240). ADR-034: atlas pages are 1024 × 1024 `R8_UNORM`, at most 8 (UI-198). |
| C-UI-b (deferred switch; R-CTRL-223) | ADR-032 is right and unchanged; R-CTRL-223 is a G06 fix (item 98). |
| C-VFX (clock readings; one ExecuteIndirect; ADR-064 Proposed) | ADR-036 and ADR-010 amended (VFX-237). ADR-064's status is unchanged; its components line now names VFX. |
| C-CAM (camera clock) | Recorded in ADR-036. |
| C-DBG (ADR-068 overlay in `Debug`; ADR-065 crash commands) | ADR-068 now includes `Debug` (DBG-004). ADR-065's crash commands match DBG-117, so no change was needed. |
| C-INP items 1–4 | Items 95 and 100. |
| C-PLT items 1–5 | Item 116. |
| C-DAT items 1–3 | Item 119. |
| C-BLD items 1–6 | Item 108. |
| C-AEN | Items 87 and 88. |
| G08b (no map folder in ADR-071) | Closed: `maps\` added (item 116). |
| G09b (display-mode options, Apply re-creating the renderer) | No ADR change; recorded there as reference behaviour with "no Species equivalent" notes. |
| G10 items 1–3 | Item 88. |
| X-GLO-2 (ADR-025 against the reference cache) | Settled in ADR-025 (item 87); the glossary wording belongs to X-GLO. |
| G00–G07, G08a, G11, C-SIM-a/b/c, X-GLO-1, X-AST | No ADR conflict reported. X-AST's assumptions (JSON maps and explainers, UTF-8 JSON string tables) agree with ADR-062 and ADR-063. |

## 4 Component fixes needed (for the document fixers)

| # | Target | Fix |
|---|---|---|
| F1 | `architecture/components/renderer/shaders.md` RND-262 | Add the fourth fixed sampler index of ADR-018, `kSamplerPointMagnifyClamp` = 3 (point magnification, linear minification and mips, clamp-to-edge). |
| F2 | `architecture/components/asset_pipeline_and_resources.md`: §2 `SamplerId`, the "sampler" row of table 3.3-A, AST-025 | Add `PointMagnifyClamp`, "point-magnify clamp-to-edge" (art that the look magnifies into hard-edged blocks but minifies smoothly), to the closed set. |
| F3 | same file, AST-027 | `fx.particle_dot` and `sprite.worker.front` use the point-magnify clamp-to-edge sampler, either as their sampler or through a view that names it. Delete the sentence that the view samples the nearest level under minification because the set lacks the sampler. |
| F4 | same file, AST-021 | The pairing list becomes: `straight` with the alpha blend, or with the additive blend for glowing icons, markers and cursors (RND-393, RND-423). The other pairings are unchanged. |
| F5 | `renderer/world_renderers.md` RND-162 (last sentence) and `renderer/visual_language.md` RND-406 | Team regions are marked by `_SPECIES_TINT` or `species_team_tint` (AST-034, ADR-020). Key-colour art reaches the cooker only as explicit weights written by the authoring helper. Cloth art carries the team area as grey levels with a companion tint mask (AST-022), not pure red. The look is unchanged (item 83). |
| F6 | `renderer/visual_language.md` RND-393 | The glow, beam and darkening mask families are `linear` (`BC4_UNORM`), as AST-020 and ADR-017 say (C-AST item 7). |
| F7 | `renderer/shaders.md` RND-262 | The `QuadInstance` layout lists the per-end colours of `Segment` (RND-319) and the billboard blend start and length of `WorldOriented` (RND-306). |
| F8 | `ui_framework/layout_rendering_input_and_localisation.md` UI-216 | "measured in presentation time" becomes "measured in real time on the platform clock (ADR-074, ADR-029)", so menus keep repeating while a match is paused (item 100). |
| F9 | `build_tooling_and_distribution.md` tuning row `bld.pin.windows_sdk` (BLD-029) | Value becomes `10.0.26100.9457` (Windows SDK for Windows 11, September 2026). Remove "it is not yet in the technology baseline"; cite baseline item 2.4 / ADR-051. |
| F10 | `platform_layer.md`: the service table row 15 "HTTPS client" and PLT-099 | Cite ADR-077 next to ADR-066. |
| F11 | `audio_engine.md` AEN-060 and `asset_pipeline_and_resources.md` table 3.10-A | Cite DAT-273: the effect-kind table (`DspFilterDef`) is a schema-defined table compiled into the engine, with no pack entry (item 119; C-DAT item 3). |

Coordinator note: `species_work/standards_baseline.md` (not mine) does not yet have the Windows SDK item. The README copy is now ahead of it; copy §5 below into it if the working baseline must stay identical.

## 5 Windows SDK verification (item 108)

Sources: Microsoft Learn only, checked 2026-10-17.
- https://learn.microsoft.com/windows/apps/windows-sdk/ shows two supported series, 28000+ and 26100+; 22621 left support on 2025-11-12. The 10.0.28000 SDK "can target Windows 11, version 26H1, in addition to previous Windows releases".
- https://learn.microsoft.com/windows/apps/windows-sdk/downloads has the following stable releases:
  - latest 26100-series release: **10.0.26100.9457**, September 2026;
  - latest 28000-series release: 10.0.28000.2957, September 2026.
- https://learn.microsoft.com/windows/apps/windows-sdk/release-notes gives these milestones in the 26100 series:
  - 10.0.26100.1742 (2024-09-24) corresponds to the 24H2 public release;
  - 10.0.26100.6584 (September 2025) corresponds to the 25H2 public release.
  - The first stable 28000 release is 10.0.28000.1721 (March 2026), a "major version bump".

**Recommendation recorded (medium confidence; the facts are primary, the choice is a judgement):** pin **10.0.26100.9457**. It is the series of the OS floor (24H2, build 26100) and of the primary test targets (25H2 and 26H2 share that servicing branch). Move to the 28000 series only when a component needs an API that only it provides, and re-check at each milestone. This is recorded in README summary row 2.4, the README §2 bullet with its source URLs, and ADR-051.

## 6 Checks

- `scan_patterns.py forbidden_patterns.tsv design_next/ADR/*.md`: no output.
- `scan_patterns.py soft_patterns.tsv design_next/ADR/*.md`: only terms allowed by CONVENTIONS §9 in their listed paths:
  - `Linux` in `ADR/*.md`: 0051, 0052 and README lines 99, 143, 151 and 152; line 152 is the new preset line about a Linux dedicated server;
  - `immediate-mode` in 0030;
  - `retail` in `ADR/README.md`.
- The ids helper: every requirement id and DEC id cited in the ADR files is defined (0 problems).
- The paths helper: no stale backticked path, no broken relative link, and no `ui.`/`map.` asset stem in the ADR files.
- Every tuning key cited in the new text exists in its owner's tuning table (16 keys checked).
- Sizes: the largest ADR is 9.0 KB and the README 49.5 KB, far below the limit.

## 7 Not done, and why

- **ADR-077 WinHTTP details are not verified.** The brief allows Microsoft documentation only for the Windows SDK, so I did not check the WinHTTP behaviours the record relies on: system and automatic proxy, OS certificate validation, and disabling automatic logon. This is listed under "Items not verified" in the README and in ADR-077; confirm it before accepting the record.
- **No status changes.** ADR-025 stays Proposed (item 87). No other status was changed.
- **Component and asset documents were not edited.** They are not my files; the fixes are listed in §4 and in the X-AST-2a notes under items 83, 84 and 85.
