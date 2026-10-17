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

---

## 8 Second pass (AGENTS.md)

Brief: `phase4_brief.md` section "X-ADR-P4, second pass". Owner decisions O21–O23. Files are now `design_next/ADR/ADR-NNN-<slug>.md`. Every amended record keeps its number and MADR 4 form and gains an "Amendment 2026-10-17 (AGENTS.md, O21)" note as a bullet of *More Information*. Microsoft documentation was used to verify the Microsoft facts stated; each one cites its URL. No git writes and no publishing.

### 8.1 One line per ADR

Status changes:
- ADR-030, ADR-054, ADR-055, ADR-056 and ADR-069 are now **Accepted**, decided by the owner's AGENTS.md.
- ADR-053 is now **Rejected**.
- ADR-025, ADR-065, ADR-076 and ADR-077 stay **Proposed**: AGENTS.md narrows them but leaves an in-box choice open.

| ADR | Change |
|---|---|
| 001 | The Agility SDK is an allow-listed NuGet package restored from `packages.config`. No `d3dx12.h` and no DirectX-Headers; barriers and heap descriptions are written by hand; COM objects are held in `Microsoft::WRL::ComPtr` (R12, R14). The debug layer runs only in Debug, on developer machines and in a GPU CI job. The DirectX-Headers source is removed. |
| 003 | Shaders are compiled by `FXCompile` with the Windows SDK's `dxc` (SM 6.6, `-HV 2021`, warnings as errors). Settings are identical in Debug and Release. Sources: `<Lib>/Shader/<Shader><Stage>.hlsl`. Output: `<Lib>/CompiledShader/<Shader><Stage>.h` with `g_<Shader><Stage>` byte arrays embedded in the exe. Not used: shaders in packs, `.cso`, `dxcompiler.dll`, `d3dcompiler_47.dll`, a pinned redistributable DXC. Each permutation is its own `<Shader><Permutation><Stage>.hlsl`. Cost noted: a larger executable with resident bytecode. Verified: Visual Studio uses `dxc` for SM 6 and writes headers with `/Fh` and `/Vn`. |
| 005 | Pipeline creation outside start-up is an error in Debug and is logged in Release. |
| 007 | The two-path validation runs as a Debug test run on a GPU agent. |
| 010 | Third-party VFX middleware is excluded by R14. |
| 017 | DirectXTex is a dependency of the content build tool only (NuGet, development tool). The executable reads DDS with Species code. |
| 022 | Definitions reload in Debug only. Audio middleware is excluded by R14. |
| 025 | Retitled. Long sounds are streamed MS-ADPCM, decoded by XAudio 2.9 and encoded by the content build tool (AdpcmEncode distribution unconfirmed). Seek and loop work on 32–512-sample blocks. No third-party codec; Opus is removed. xWMA is not used (encoder unconfirmed). Status stays Proposed. Verified: XAudio 2.9 formats on PC (PCM, float, ADPCM, xWMA) and ADPCM 4:1 with 32–512 samples per block. |
| 026 | Drop counts are shown in Debug. |
| 030 | **Accepted**: R14 excludes UI middleware, so `UiCore` is the decision. |
| 036 | Effect definitions reload in Debug only. |
| 050 | AVX2 CPU floor (Haswell 2013, Excavator 2015) set by `/arch:AVX2` (R16). The illegal-instruction failure cannot be reported by the start-up check. |
| 051 | Retitled. Toolset `v145` (VS 2026; MSVC 14.50+, minor version not pinned, printed by CI). Settings identical in both configurations: `/std:c++latest`, `/permissive-`, `/W4` with `/WX` always, `/fp:precise`, `/arch:AVX2`, x64, plus `/Zi` (never `/ZI`, because `/guard:cf` excludes it) and the mitigations. Allowed differences: `/MDd` vs `/MD`, incremental linking vs `/OPT:REF`/`/OPT:ICF`/LTCG. Removed: clang-cl, the analysis configuration and the server compilers; a non-Windows server needs a later ADR. The FMA-contraction caveat of R16 is recorded against the numeric model (DEC-240/241). Verified: `v145` and the `/guard:cf`–`/ZI` incompatibility. |
| 052 | Retitled. `/std:c++latest` everywhere; code is written to C++23, and a newer facility needs an amendment. One ABI unit (everything built from source). The portable subset is guidance only. Accepted (switch decided by AGENTS.md). |
| 053 | **Rejected**: R7 allows only `.h`/`.cpp` and §4 fixes hand-ordered standard includes, so there are no module interface units and no `import std`. The option and its CI job are removed. |
| 054 | **Accepted**, retitled "one MSBuild solution (`.slnx`) of `.vcxproj` projects, with the x64 configurations Debug and Release only" (no pipe character in the title, so the index table stays intact). Contents: functional filters, flat folders with the two shader subfolders, `v145`, the exact Debug/Release differences of §3, built through the solution (`/t:<Project>`), `$(SolutionDir)` paths, NuGet restore, never-committed outputs, and `Build\CheckProjectFiles.py`. The first projects' layout goes in a later ADR (§2). CMake, Ninja, presets and the extra flavours are removed. Verified: `.slnx` needs MSBuild 17.12+. |
| 055 | **Accepted**, retitled. Allow-list table with pins: `Microsoft.Direct3D.D3D12` 1.619.6, `Microsoft.GameInput` 3.5.283, `WinPixEventRuntime` 1.0.240308001. `packages.config` restored with `/t:Restore /p:RestorePackagesConfig=true` into `packages\` (never committed). Explicit exclusion list follows R14. Tools under `Tools/` and `Build/` keep their own pinned dependencies. The licence list covers the three packages. vcpkg is removed. Verified: `packages.config` restore needs MSBuild 16.5+. |
| 056 | **Accepted**, retitled. Products are projects (client, server, content build tool, `<Project>Tests`). Debug and Release only. A table lists what each configuration compiles in. Development features sit under `ENGINE_DEVELOPMENT`, derived from `NDEBUG`. Logging, black box, crash context, PIX markers and telemetry are in both. The cost is stated: no optimised build with development tools. `SPECIES_PRODUCT` and the extra flavours are removed. |
| 057 | Identity records product and configuration. The header is `BuildIdentity.h`, generated by a solution build step (R7). |
| 058 | The default chunk codec is the in-box Compression API, XPRESS with Huffman encoding (`Cabinet.dll`, Windows 8+, buffer mode, one call per chunk). Stored chunks are used where compression does not help. No Zstandard. Packs hold no shaders. Loose files are Debug-only. A DirectStorage back-end needs an allow-list decision. Verified on Learn: the algorithms and the selection guidance. |
| 059 | The content build tool is a project of the solution under `Tools/`, a development tool outside R14. Shaders are no longer content. Its CI job comes by amendment. The Mermaid label edit removes only the word "shaders"; the syntax is unchanged. |
| 062 | The generator is part of the Tools/ content build tool. Generated C++ follows AGENTS naming (R8 camelCase aggregate fields, R9 namespaces, R11 American spelling, e.g. schema `colour` → field `color`; schema names keep their spelling). |
| 065 | The dedicated server is a Windows build, so WER covers it. The third-party crash SDK option is marked as excluded by R14 and its source is removed. |
| 066 | Telemetry is in both configurations. The analytics SDK option is excluded by R14. |
| 067 | Log levels per configuration (Release: info and above; Debug: debug and trace). The logging-library option is excluded by R14. |
| 068 | Markers come from `ENGINE_PROFILE_SCOPE(name)` and `ENGINE_PROFILE_GPU_SCOPE(commandList, name)`. They stay in Debug and Release, so `WinPixEventRuntime.dll` ships. The overlay is Debug-only. Confirmations changed accordingly. |
| 069 | **Accepted**, retitled. Microsoft C++ Unit Test Framework, run by `vstest.console.exe`. One `<Project>Tests` per library, `namespace <Project>Tests`, `SuiteSmoke` placeholder. Requirement id in the PascalCase method name and as a test-method attribute. Property and golden tests in Species code. Fuzz harnesses are `<Project>Fuzz` projects with `/fsanitize=address` and `/fsanitize=fuzzer` (experimental; verified). The third-party framework, CTest and the presets are removed. |
| 070 | Retitled. CI is the committed `build.yml`. Windows job: toolchain print, solution locate, `CheckProjectFiles`, `Debug\|x64` build with `/warnaserror`, vstest suites requiring DLLs, pinned clang-tidy 22.1.8. Linux job: clang-format 18. Release is not built. A NuGet restore step and further guarded jobs come by amendment: design checks, content build, soak, fuzzing, reproducibility, performance, GPU golden-image and validation. Preset jobs, analysis and `import std` builds are removed. |
| 071 | The install gains `WinPixEventRuntime.dll` and holds no shader files. The Agility SDK stays in `D3D12\`. The GameInput redistributable is run by the installer, never by the game. Licences cover the three packages. `diagnostics\` is Debug-only. |
| 073 | Signing applies to Release builds where they are built; CI builds Debug only. |
| 074 | The command-history time-source replacement is Debug-only. |
| 076 | The encoder choice is fixed by R14 (libpng and single-header writers excluded). The record stays Proposed for the video-capture part only. |
| 077 | The server is a Windows build and uses WinHTTP. The libcurl option is excluded by R14. The record stays Proposed (the in-box WinRT HTTP client remains an alternative). |
| README | Index regenerated from the files (11 titles or statuses changed, see below). Baseline: removed CMake, Ninja, presets, vcpkg, the Linux server compilers, GoogleTest, Zstandard, Opus, the redistributable DXC, DirectX-Headers and the Sentry option. Added: rows 1.3 (AVX2), 2.3 (settings), 3.3 (clang-tidy 22.1.8 / clang-format 18 from `build.yml`), 3.4 (tests), 5.4 (Compression API). Rewrote rows 2.1, 2.2, 3.1, 3.2, 4.5, 4.7, 5.1, 5.2, 8.3 and 11.1. Rewrote §2 and §3; edited §1, §4, §5, §8 and §11. "Items not verified" lost the GCC, cppreference and cmake.org lines and gained the SDK `dxc` version. Intro: AGENTS.md wins (O21). |

Index changes: ADR-025, 030, 051, 052, 053, 054, 055, 056, 058, 069 and 070.

The other records need no change: 002, 004, 006, 008, 009, 011–016, 018–021, 023, 024, 027–029, 031–035, 060, 061, 063, 064 and 072. Their only "development" mentions name the development overlay or a development command, which are Debug features. Code identifiers in all 64 ADRs were checked against AGENTS §1. The only non-conforming names were `SPECIES_PROFILE_SCOPE`, `SPECIES_PRODUCT` and `SPECIES_IMPORT_STD`; all are gone. SDK names and data names (`_SPECIES_TINT`, `species_team_tint`, asset ids, keys) are unchanged.

### 8.2 Verification on Microsoft documentation (2026-10-17)

- **Toolset and solution:**
  - VS 2026 ships `v145` with MSVC 14.50, and 14.51 is the default since May 2026.
  - MSBuild builds `.slnx` from 17.12.
  - `msbuild -t:restore -p:RestorePackagesConfig=true` restores `packages.config` projects (MSBuild 16.5+).
- **Shaders:**
  - Visual Studio uses `dxc` when Shader Model 6 is selected and writes byte arrays in headers (`/Fh`, `/Vn`).
  - The Windows SDK provides the DXC compiler for Direct3D 12.
  - SM 6.6 was announced on 2021-04-20.
- **Tests:** the CppUnitTest header and library sit under `VC\Auxiliary\VS\UnitTest` and support test-method attributes; `vstest.console.exe` returns 0 when it finds no tests; `/fsanitize=fuzzer` is experimental, from VS 2022 17.0.
- **Compression API:** MSZIP, XPRESS, XPRESS_HUFF and LZMS from Windows 8 (`Cabinet.dll`), with the selection guidance.
- **XAudio2:** XAudio 2.9 on PC accepts 16-bit PCM, float, 16-bit ADPCM and xWMA; ADPCM has 32–512 samples per block, up to 4:1, and is decoded in software.
- **Linker:** `/guard:cf` is incompatible with `/ZI`.
- **Not verifiable:** the exact `dxc` version that SDK 10.0.26100.9457 installs is not published on Learn. It is recorded under "Items not verified"; ADR-003 relies only on SM 6.6 support, which predates the SDK series, and CI records the toolchain.

### 8.3 Proposals for AGENTS.md (owner decisions; not applied)

1. **Compute shader suffix:** `CS` (compute) as a third stage suffix next to `VS` and `PS` in §2; the renderer and VFX have compute shaders.
2. **Permutation files:** each permutation as its own `<Shader><Permutation><Stage>.hlsl` file in `<Lib>/Shader/`, with one shared include file per shader (for example `.hlsli`) in the same folder; §2 names only `.hlsl` sources.
3. **Fuzz projects:** `<Project>Fuzz` projects for fuzz harnesses, with ASan and fuzzer settings identical in both configurations of that project.
4. **The content build tool's own dependency:** §2 says "The build depends on the Windows SDK, the MSVC standard library and the three Microsoft packages", while R14 exempts development tools. The content build tool (a solution project under `Tools/`) needs DirectXTex for BC7 encoding. I propose §2 says "the executable's build", or that the tool gets a solution of its own under `Tools/`. ADR-017, ADR-055 and ADR-059 follow the R14 tool exception.
5. **Restore step in CI:** `build.yml` needs a guarded NuGet restore step, `/t:Restore /p:RestorePackagesConfig=true` before the build, once a project has a `packages.config`. Today the Build step does not restore. ADR-070 records it as an amendment to come.
6. **The `ENGINE_DEVELOPMENT` switch** derived from `NDEBUG` in one engine header (ADR-056). It is a macro family of its own; it may want a line in §4 next to the Windows macro header rule.

### 8.4 Coordinator notes

- File slugs no longer match three titles. Suggested renames, which need link repointing, so I did not do them:
  - `ADR-054-build-system-cmake-presets.md` → `ADR-054-build-system-msbuild-solution.md`;
  - `ADR-055-dependencies-vcpkg-manifest-mode.md` → `ADR-055-dependencies-three-nuget-packages.md`;
  - `ADR-056-editions-build-flavours-and-feature-flags.md` → `ADR-056-products-configurations-and-feature-flags.md`.
- `species_work/standards_baseline.md` (not mine) still holds the CMake, vcpkg, server-compiler, Opus and DXC-redistributable lines; the README copy is now the aligned one.
- The `decision-makers` front matter of the records decided by AGENTS.md reads "Species technology lead; decided by the owner's AGENTS.md (O21)".

### 8.5 Checks (second pass)

- `scan_patterns.py forbidden_patterns.tsv design_next/ADR/*.md`: no output.
- `soft_patterns.tsv`: only allowed terms remain (CONVENTIONS §9 `ADR/*.md` paths): `Linux` in ADR-070 (CI hosting), `immediate-mode` in ADR-030, `retail` in `ADR/README.md`. "porting" in ADR-052 and "Explorer" in ADR-069 were reworded.
- `check_design.py --skip-mermaid`: no failure names an ADR file (gates 1, 3, 7, 9 and 10 pass). The failures elsewhere are other packages' items: one soft hit in `reference/03`, three undefined requirement ids, 14 uncited Open DECs and two uncited tuning keys.
- The ids helper finds 0 undefined requirement or DEC ids in the ADRs; the paths helper finds no broken link.
- Largest file: `ADR/README.md` at 52.5 KB.

### 8.6 Component fixes needed (second pass, F12 onwards)

These are for the wave B fixers. The general AGENTS.md naming rewrite of every sketch (O23) is the "Wave B addition" of the brief and is not repeated here; the lines below are the changes the amended ADRs require.

| # | Target | Fix |
|---|---|---|
| F12 | `architecture/components/build_tooling_and_distribution.md`, the whole document. Ids: BLD-001, BLD-003, BLD-005 to BLD-008, BLD-013, BLD-014, BLD-017, BLD-019, BLD-020, BLD-023, BLD-024, BLD-027 to BLD-038, BLD-043, BLD-044, BLD-046, BLD-048, BLD-082, BLD-086, BLD-089 to BLD-091, BLD-095, BLD-096, BLD-100, BLD-118, the §4.13 tuning table, §6, §7, §8 | Rewrite to ADR-054/055/056/070. One `.slnx` with `.vcxproj` and filters; `v145`; Debug and Release only, differing only in AGENTS §3 settings. Replace the preset tables 2.1-A and 2.1-B by solution, projects and configurations. Products are projects (client, server, content build tool, `<Project>Tests`), with no `SPECIES_PRODUCT`. Switches derive from `NDEBUG` (`ENGINE_DEVELOPMENT`). NuGet `packages.config` for the three packages replaces the vcpkg manifest and triplets. BLD-028: `/std:c++latest` in place of `/std:c++23`. BLD-033: drop clang-cl and the analysis builds. CI jobs follow `build.yml` plus the amendments of ADR-070. BLD-043: the texture library is a NuGet dependency of the content build tool; the shader compiler is the SDK `dxc` via `FXCompile`, not pinned through a manifest. BLD-019/020: the generated header is `BuildIdentity.h` from a build step; rename `SPECIES_BUILD_NUMBER` and `SPECIES_SOURCE_REVISION` to `ENGINE_*` (or build-step properties). Sketch namespace `species::build` → `Engine` (O23). F9 (`bld.pin.windows_sdk` = 10.0.26100.9457) still applies. |
| F13 | `diagnostics_test_and_performance.md` (or its split folder, item 113). Ids: DBG-004, DBG-020, DBG-041, DBG-049, DBG-064, DBG-070, DBG-072, DBG-080, DBG-095, DBG-098, DBG-140, DBG-181, DBG-206, the §4.11 tuning table, §7, §8.3, and the macro block | DBG-004 table: two columns (Debug, Release). The development facilities are Debug-only. PIX markers, GPU timestamps, logging at info and above, the black box and telemetry are in both. Debug-level records are Debug-only. Rename the macros: `SPECIES_ASSERT` → `ENGINE_ASSERT`, `SPECIES_CHECK` → `ENGINE_CHECK`, `SPECIES_CHECK_INDEX` → `ENGINE_CHECK_INDEX`, `SPECIES_PROFILE_SCOPE` → `ENGINE_PROFILE_SCOPE`, `SPECIES_PROFILE_GPU_SCOPE` → `ENGINE_PROFILE_GPU_SCOPE`. DBG-140: tests on the Microsoft C++ Unit Test Framework with `vstest.console.exe`, `<Project>Tests`, `SuiteSmoke`, requirement-id attributes (ADR-069). Fuzzing uses `<Project>Fuzz` projects. Soak and performance CI jobs follow ADR-070 (guarded, added by amendment). |
| F14 | `architecture/components/asset_pipeline_and_resources.md`: table 3.11-A shader row, AST-051, AST-057 table 3.13-A, AST-055, AST-095, the `ast.pack.zstd_level` key, AST-080, AST-006, AST-105 to AST-107, AST-121, §8 | Shaders are not content: remove the `Shader` content type, the shader row and the "shader blobs" of the `startup` pack (ADR-003, ADR-058). AST-055: codecs become stored and XPRESS with Huffman encoding (Windows Compression API). Replace `ast.pack.zstd_level` by the Compression API setting, if any. AST-080: streamed sounds are MS-ADPCM, not Opus (ADR-025). AST-006/105–107/121: "Profile and shipping builds" → "Release builds"; loose files are Debug-only. |
| F15 | `architecture/components/audio_engine.md`: AEN-060, AEN-072 (stream format), AEN-246, §4.20, AEN-323, the streaming requirements (§4.18, AEN-252 and the `aen.stream.*` keys) | Streams are MS-ADPCM decoded by XAudio2. Remove the Opus packets, pre-skip, packet duration and the Species decoder task. Seek and loop work at block granularity; the samples per block is a key (32–512, default 512); loop points sit on block boundaries (ADR-025). AEN-323: "Development builds" → "Debug builds". |
| F16 | `architecture/components/renderer/shaders.md` RND-260, RND-261, RND-263; `renderer/README.md` RND-011, RND-041; `frame_graph_and_passes.md` RND-076 | RND-260: compiled by `FXCompile` with the SDK `dxc` into embedded byte arrays (ADR-003), not "the pinned DXC". RND-261/263: one source file per shader, stage and permutation, `<Shader><Permutation><Stage>.hlsl` in PascalCase (`SkyVS`, `TerrainInvertedPS`, `MeshLitFogPS`, `SeaWavesUpdateCS`), each producing `g_<Shader><Stage>`. RND-011/076: development flags and banners are Debug-only; "shipping builds" → "Release builds". The shared C++/HLSL header (RND-262) is a `.h` in the renderer project folder. |
| F17 | `architecture/components/platform_layer.md`: PLT-017, PLT-022, PLT-026, PLT-049, PLT-051, PLT-107, PLT-130, §7 | `Debug` and `Development` → Debug only; `Profile`/`Shipping` → Release (ADR-056, ADR-074). The server is a Windows build (ADR-051). |
| F18 | `architecture/components/camera_and_sequences.md` CAM-080, CAM-125; `ui_framework/pages_and_screen_recipes.md` UI-482; `animation_and_vfx/` and `ui_framework/` hot-reload lines | Developer fly mode, diagnostics overlay and hot reload exist in Debug builds only (ADR-056). |
| F19 | `architecture/components/data_schemas/README.md` DAT-052 and §2 | Loose files and the development record overlay are Debug-only. "`tools` product" → the content build tool project under `Tools/`. Remove the vcpkg mention. The generated C++ follows AGENTS naming: aggregate fields in camelCase, American spelling, `Engine`/`Species` namespaces (ADR-062). |
| F20 | `architecture/components/input.md`, `ui_framework/`, `renderer/`, `audio_engine.md`, `platform_layer.md` | Wherever a requirement says "development builds" or names the `Development`/`Profile`/`Shipping` flavours, use Debug/Release (the scan found the ids in F13–F19; the coordinator's gate scan can list any rest). |
| F21 | `design_next/CONVENTIONS.md` §8.12 (FX-12) | Engine macros are `ENGINE_*` (`ENGINE_ASSERT`, `ENGINE_CHECK`, `ENGINE_CHECK_INDEX`, `ENGINE_PROFILE_SCOPE`, `ENGINE_PROFILE_GPU_SCOPE`, `ENGINE_DEVELOPMENT`); `SPECIES_*` macros are removed; C++ files are named for their primary type (`BuildIdentity.h`); shader file names follow `<Shader><Stage>.hlsl`. `_SPECIES_TINT` stays a glTF attribute (data). |
| F22 | `design_next/glossary/a_to_l.md` §A and `m_to_z.md` §P (X-GLO) | Entries that name the build flavours (`Development`, `Profile`, `Shipping`) become Debug and Release; add "content build tool" as a development tool under `Tools/` if the glossary names the `tools` product. |
| F23 | `decisions/data_sequences_platform_debug.md` DEC-803 and `decisions/deferred/networking.md` DEC-204 (decision owners) | Replace the flavour names in their text with Debug/Release. |
