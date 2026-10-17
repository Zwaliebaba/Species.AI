# RES-2 report: R9 layering sweep and the last ADR items (Phase 4)

Status: done. Items 1 to 5 of the RES-2 section are applied or closed; nothing was refused.

Inputs read: `AGENTS.md`; `phase4_brief.md` §1, §2, wave B, wave B addition, RES-2; prompt §0, §2, §4, §6, §7.1–§7.2; `p4_residuals.md` (residual 24); `reports/RES-1.md`; every interface sketch under `design_next/architecture/components/`; `CONVENTIONS.md` §8; the glossary; `.github/workflows/build.yml`. No git write command and no publishing or upload tool was used. `design_next/assets/` (X-AST-2b) was not touched. The coordinator's commit `ab5f55c` picked up my first two renderer edits and an early copy of this report while I worked; the files on disk are the final state.

## 1 Items

1. **R9 sweep (residual 24) → done.** Every sketch was inventoried (`RES-2_inventory.py`, `RES-2_gamewords.py`). Types, enumerations and free functions that name or encode a game concept moved from `Engine` to `Species` blocks in the same sketch, qualifying the engine types they use (`Engine::PassContext`); where an `Engine` type held a game type, a generic engine type replaced it. A final scan (`RES-2_edges.py`) finds no `Engine` declaration that names a `Species` type, and no `Species` block that uses an engine name unqualified. Every changed sketch got one sentence that states its layer. Requirement ids, tuning keys and headings are unchanged; the only requirement text changed is AEN-061's second sentence (it names `EmitterId`; its first sentence, which the requirements index quotes, is unchanged). Moved and new identifiers: §2.
2. **Registry scope → done.** CONVENTIONS §8 has a new *Registry* bullet: §8 registers types, enumerations with their enumerators, public free functions, macros and constants; member functions and fields are not registered, the owner's sketch is their record; an enumeration listed without enumerators has them in its sketch; table 8.12-A gives a member as the code form of a term where the term is a member (`FxRng::NextUnit`). To make the registry agree with that rule, table 8.12-B lost its member and field entries (§3). No member was added.
3. **ADR leftovers → done** (numbers, status and MADR form kept; each change has an amendment note, placed last under More Information):
   - **ADR-065:** the symbols bullet: every Release build that the release procedure packages publishes its PE files and PDBs to the symbol store and uploads them to Partner Center; CI builds publish symbols only once the symbol job of an ADR-070 amendment exists, and until then the workflow uploads the build log and test results only. The `LocalDumps` registry file comes with every build output (BLD-113), not from CI; the confirmation's symbol check is a release-procedure check. Note "(`build.yml`, `AGENTS.md` §6)". Technology baseline §11 says the same.
   - **ADR-067:** the debugger-output sink is Debug only (no Development configuration). In the same note: the API is written `Log(LogCategory, LogLevel, std::format_string<Args...>, Args&&...)` as the DBG sketch declares it (it was the lower-case `log(...)`, against `AGENTS.md` §1), and the category list gains the two categories of `LogCategory` it left out (diagnostics, test).
   - **ADR-019:** the Khronos glTF Validator is no longer "for example": it is a tools-solution dependency (`AGENTS.md` §2), pinned at **2.0.0-dev.3.10** under the **Apache License 2.0**, with its licence text kept beside it. The content build tool runs its command-line program as a separate process and reads its JSON report; nothing of it links or ships (R14). Sources, checked 2026-10-17: the npm registry entry (latest 2.0.0-dev.3.10, published 2024-10-22, Apache-2.0; all 29 published versions are 2.0.0-dev releases), the repository's LICENSE and README. The GitHub release page and API were not reachable (proxy 403), so whether that release ships a ready-built Windows archive is not verified. Baseline row 8.1, §8 and "Items not verified" were updated. This is a technology-standard fact consulted for a pin, as the RES-2 item asks.
   - **BLD table 2.1-B:** the `EngineBuildNumber` row reads "CI, which passes `/p:EngineBuildNumber=${{ github.run_number }}` (ADR-070); 0 in a local build" (+18 bytes, file 117,552 bytes, 208 bytes under 115 KiB).
   - **Also ADR-070 (in the spirit of item 3):** its step list matches `build.yml`: the schema-drift step `Build\CheckGeneratedSchemas.py` (O28, guarded on the script) and the build's `/p:EngineBuildNumber=${{ github.run_number }}`. Note "(`build.yml`, O28)".
4. **`MeshRole` → done.** The declaration in `data_schemas/import_and_build_records.md` is the one that stays. `asset_pipeline_and_resources.md` replaces its declaration with a comment that cites it, and CONVENTIONS registers `MeshRole` in the DAT row, with "`MeshRole` is the data schemas'" in the AST row.
5. **Sizes → no split needed.** I did not touch `rules_and_conventions.md` or `screen_flow_and_match_setup.md`; they are reference documents outside my ownership. `build_tooling_and_distribution.md` grew 18 bytes and is still under 115 KiB.

## 2 Moved and new identifiers

**Moved from `Engine` to `Species`** (sketch and registry):
- Renderer: `StructureRenderer` (world_renderers); `ActorRenderer`, `EffectRenderer`, `RouteLineRenderer`, `WorldMarkerRenderer`, `WorldCursorRenderer`, `PowerSlotsPanelRenderer`, `MatchHudRenderer` (entity_and_overlay_renderers); `LookScene` (10 enumerators), `LookCheck`, `LookError`, `RenderLookScene` (visual_language); in the registry only, `PresentationSnapshot`, `MatchHudView`, `RoutePreview`, `RouteHandle`.
- Animation and VFX: `EffectKind` (49 enumerators), `EffectEventId`, `EffectEvent`, `ParticleKind` (17 enumerators), `ExplainerPlayer`, `ExplainerLayout`, `VfxRuntime` (README); `ProceduralAnimator` (explainers).
- Camera and sequences: `CameraMode`, `CameraFrameInputs`, `MatchCamera`, `TimelinePlayer`; in the registry, `CameraState` and `TimelineFrameInputs`.
- UI framework (pages_and_screen_recipes): `PageId`, `NavRequest`, `EnterResult`, `PageContext`, `FrontEndViewModel`, `HudContext`, `FrontEndPage`, `PageHost`, `FrontEndScreen`, `HudPanel`; the events `PageEntered`, `PageLeft`, `NavigationRejected`; the widget kinds `ModeDescription`, `ModePicture`, `ColorPickerRow`, `ModeRow`, `LobbySlotRow`, `SlotOpenControl` (a layer sentence under table 3.3-A).
- Data schemas (registry only, because their sketches were already in `Species`): `MapHeaderView`, `MapDefinitionView`, `MapLoader`, `StructureRecord`, `StructureLink`.
- Table 8.12-A: the Kind column is now the layer column. 60 rows are "engine type" and 127 are "game type": every simulation and reference type, and the moved `MatchCamera`, `FrontEndScreen`, `PowerSlotsPanel`, `SpiderGait`, `WorldCursor`, `WorldMarkers`, `ClothFlag`, `TrailPoint`, `LoadingView`, `HintOverlay`, `TickClock` and `MapLight`.

**New generic `Engine` types** (sketch, plus a term row in table 8.12-A): `SceneSnapshot`, "snapshot interface", which `FrameInputs::snapshot` now holds (renderer README); `MeshRenderer`, "mesh batch", which the game's structure and actor renderers fill (world_renderers); `ParticleTypeId`, "particle-type id", the type of `ParticleEmission::kind` (VFX README); `EmitterId`, "emitter id", and `AudienceGroup`, "audience group" (audio README).

**Renamed in `Engine`** (generic names; the data tokens stay): `SoundAudience` `OwnTeam`, `Enemy`, `NoTeam` → `OwnGroup`, `OtherGroup`, `NoGroup`. `SoundLink` and the DAT `ParamLink` `TeamsNearby`, `UnitCount` → `GroupsNearby`, `ObjectsNearby`. `MatchPhase::Grace` → `Countdown`. The fields `TriggerCommand::team` → `group`, `EmitterUpdate::teamsNearby`, `unitCount` → `groupsNearby`, `objectsNearby`, and `MatchState::localTeam` → `localGroup`. `BeginMatch(_localTeam)` → `BeginMatch(_localGroup)`. The types `SimObjectId` → `EmitterId` and `TeamId` → `AudienceGroup` in the audio sketch. These are new names for the ledger, if the coordinator records engine identifiers there.

**Where the line was drawn.** CONVENTIONS §8 *Namespaces* now states the test. A game concept is a Species term of §8.1 to §8.9, the Power slots or the match HUD, a team, or a game type of 8.12-A. The engine vocabulary is match, map, front end, page, session, tick, frame, camera, sound, effect, particle, mesh, terrain, sea, sky and tree. So `FrameKind` (`Match`, `MapEditor`), `ContentType` (`Map`, `MapIndex`, `ExplainerScript`, `SimRecord`), `MapCameraData`, `MapIdentityEntry`, `TimelineOp`, `MatchState` and the tree generator stay in `Engine`.

## 3 CONVENTIONS §8

- *Namespaces* bullet: the layer test, the two vocabularies, the rule that engine code never names a game type, and that the identifiers of §8.1 to §8.10 are game code except `UiCore`. New *Registry* bullet (item 2). Table 8.12-A has a lead sentence and the caption "Table 8.12-A Engine and game types."; the 8.12-B lead sentence now names the Namespace column as the layer.
- Table 8.12-B: new `Species` rows for the renderer, VFX, UI pages and widget kinds, the camera and the DAT map records; the VFX enumerator rows are now `Species`. Removed: the member and field groups (Simulation "core members" and "team command state fields", UI "public fields", Input "members and aggregate fields" (its nested `InputEvent::Kind` moved to the Input types row), CAM "`MatchCamera` functions"), and the member names inside the RND, UI, terrain and simulation function groups. 164 member and field names were removed in all (the git diff of table 8.12-B lists them). `BuildRoute` was removed because R-SIM-345 writes it `RouteGraph::BuildRoute`. Unqualified simulation functions (`IsPathClear`, `WalkCost`, `PushFromObstacles`, `TryEnterTeleport`, `Bounce`, `Explode`) stay, as their owner writes them. `OptionsScreen`'s `ShowDialog` and `AddMenuControl` went (§4.24 members).
- Glossary: no entry names a moved or renamed identifier, so it is unchanged.

## 4 Files changed (bytes after)

| File (under `design_next/`) | Bytes | Items |
|---|---:|---|
| `CONVENTIONS.md` | 70,744 | 1, 2, 4 |
| `architecture/components/README.md` | 2,856 | 1 (the layer rule; BLD scope "build presets" → "the solutions and their configurations", a leftover of ADR-054) |
| `architecture/components/renderer/README.md` | 37,713 | 1 |
| `architecture/components/renderer/entity_and_overlay_renderers.md` | 69,995 | 1 |
| `architecture/components/renderer/world_renderers.md` | 57,561 | 1 |
| `architecture/components/renderer/visual_language.md` | 30,521 | 1 |
| `architecture/components/animation_and_vfx/README.md` | 49,297 | 1 |
| `architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md` | 55,626 | 1 |
| `architecture/components/animation_and_vfx/particles_shatter_and_screen_effects.md` | 63,753 | 1 (two GPU-struct comments) |
| `architecture/components/camera_and_sequences.md` | 78,640 | 1 |
| `architecture/components/audio_engine/README.md` | 74,197 | 1 |
| `architecture/components/data_schemas/entity_audio_geometry_and_panel_schemas.md` | 38,028 | 1 |
| `architecture/components/ui_framework/README.md` | 21,092 | 1 |
| `architecture/components/ui_framework/pages_and_screen_recipes.md` | 113,946 | 1 |
| `architecture/components/asset_pipeline_and_resources.md` | 114,849 | 4 |
| `architecture/components/build_tooling_and_distribution.md` | 117,552 | 3 |
| `ADR/ADR-019-mesh-source-format-and-cooked-meshes.md` | 8,082 | 3 |
| `ADR/ADR-065-crash-reporting.md` | 7,617 | 3 |
| `ADR/ADR-067-logging.md` | 5,510 | 3 |
| `ADR/ADR-070-continuous-integration.md` | 9,017 | 3 |
| `ADR/README.md` | 53,679 | 3 |

New requirement ids, tuning keys, DECs or ADRs: none. Splits: none. Helper scripts (run with `python3 -I`): `species_work/scripts/RES-2_inventory.py`, `RES-2_gamewords.py`, `RES-2_owners.py`, `RES-2_edges.py`, `RES-2_apply.py`, `RES-2_replace_span.py`, `RES-2_split.py`, `RES-2_rows.py`, `RES-2_kinds.py`, `RES-2_fmtcheck.py`, `RES-2_fmtblock.py`, `RES-2_samedecl.py`. The specs stayed in the session scratchpad.

## 5 Checks

- `scan_patterns.py forbidden_patterns.tsv` on the 21 files: no output. `soft_patterns.tsv`: only `Linux` (ADR-070, BLD, CONVENTIONS), `retail` (ADR/README.md, CONVENTIONS), `lockstep` and `immediate-mode` (CONVENTIONS §9), all allowed by §9 in those paths.
- `python3 Tools/DesignCheck/CheckDesign.py design_next --skip-mermaid` (report `species_work/gate_report_RES-2.md`): gates 1 to 3 and 5 to 11 pass, with 13,361 links, 4,942 ids, 148 cited Open DECs and 3,710 keys; gate 4 is the skipped line. I edited no Mermaid block, so there was no Mermaid check to run.
- clang-format 18 (`.clang-format`) reports every sketch block I rewrote as clean: the renderer, camera, UI pages, DAT, VFX README and explainers blocks. `RES-2_samedecl.py` confirms that the rewritten blocks keep every declaration line, apart from the intended changes.

## 6 Cross-document requests and open points

1. **Members that still name game concepts in `Engine` aggregates.** These are out of the type-level sweep and are not registered: `RenderSettings::workerScaleFactor` and `structureDetail` (renderer options) and `SoakEntry::botTeams` (DBG; `modeId` is a data id). The proposal is to generalise them when the design phase settles those settings; the setting keys are data names and would not change.
2. **Same name declared twice (found by the sweep, not changed):**
   - `EffectKind`: DAT's audio-effect kinds in `Engine` and the VFX effect kinds, now in `Species`. Before the sweep both were in `Engine`, a collision that the move resolves. DAT's `EffectKind` also duplicates AEN's `DspFilterKind`, with `ParametricEqualizer` against `ParametricEq`.
   - DAT's `ParamKind`, `ParamUpdate` and `ParamLink` duplicate AEN's `SoundParamKind`, `SoundParamUpdate` and `SoundLink`.
   - `InputMode` is declared in `input/README.md` and in `data_schemas/config_input_and_strings.md`, both in `Engine`.
   - `CameraTransition` is declared in the camera (`Engine`) and in the map schema (`Species`).
   - `RoutePointKind` has enumerator `GroundPosition` in the simulation and `Ground` in the map schema.
   - The asset pipeline and the data schemas each declare `TextureKind`, `ColorSpace`, `AlphaMode`, `MipPolicy`, `TintMarking` and the three import records, and `SamplerId` against `SamplerKind`; item 4's rule would keep the data schemas' declarations.
   - `SourceLocation` is registered under CAM but declared by DAT.
3. **Registry completeness.** About 80 enumerations are registered without their enumerators; the *Registry* bullet points to the sketches. The Species map and record types of the data schemas are not registered: `MapDifficulty`, `SiegeSide`, `CoopGroup`, `StartPosition`, `CameraViewRecord`, `CameraAnimationRecord`, `CameraAnimationStepRecord`, `StartingForcesRecord`, `RouteRecord`, `ActorStats`, `CrateOutcomeEntry`, `ExplainerKey`, and `Limit` in `Engine`. `TeamPalette`, `InstanceWriter` and `MatchSetupView` stay placeholders.
4. **`data_schemas/README.md` §2** says the generated files "are build outputs and are never edited" and, in the same paragraph, that they are committed source (O28). The coordinator should keep only the O28 statement.
5. **Layout.** The sketches of `audio_engine/README.md`, `asset_pipeline_and_resources.md` and `particles_shatter_and_screen_effects.md` are not in the `.clang-format` layout, which the wave B addition requires. I edited them in their own style and did not reformat them.
6. **ADR-070**'s older amendment note "Job presets carry the product suffix ..." still names presets and flavours; it is history, superseded by the O21 note, and was left unchanged.
7. **Requirements index:** no regeneration is needed; AEN-061's first sentence is unchanged.

## 7 Assumptions and rules bent

- "Team" is treated as a game concept, as the brief lists it. The audio engine therefore names teams as audience groups instead of moving to `Species`, which would have made an engine subsystem game code. The behaviour and the data tokens are unchanged.
- The page framework moved to `Species` with its page ids, instead of becoming a template over the id type, because the brief allows only identifier and namespace changes. A generic page stack is a later engine option.
- Adding a `SceneSnapshot` interface with one implementer goes against `AGENTS.md` R2's "a base class for one derived class is ceremony". The interface is the seam that R9 requires, because the engine cannot name the game's snapshot. I note it rather than leave it silent.
- Beyond the listed items I aligned ADR-070 with `build.yml` and corrected the components index's "build presets". I also reformatted the two VFX blocks that I rewrote to the clang-format layout.
