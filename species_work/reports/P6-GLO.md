# P6-GLO report

## Summary

- **Findings:** 84 in the packet (3 High, 27 Medium, 54 Low): **77 fixed** (R3a-3 and R4b-43 also carry a request for the part outside my files), **5 fixed differently** (R4a-57 also carries a request), **0 rejected**, **2 request** (R3b-22, R4b-70).
- **Requests from other packets:** 17 numbered items applied (P6-ADR 2; P6-DAT 1, 2, 3; P6-DEC 3, 5; P6-ENG1 4; P6-ENG2 4; P6-RA 3; P6-RB 15, 16, 17, 19; P6-RC 13; P6-RD 15, 16; P6-RE 3), plus P6-RND 12 and 13 and the coordinator's relay items (V1 new findings 6 and 7, R4b-82, R2b-166). One sub-item is held back (the six cursor rows of P6-RC 13, see below) and one is rejected (the `FrameRenderer` members of P6-RND 12).
- **Rulings applied:** 1 and O30 ("tool project" in CONVENTIONS §8.14 and a glossary row; NuGet only), 6 (registry follows the one declaration, with "generated" DAT groups and pointers), 7 (R9 example sentence in §8 *Namespaces*), 8 (§5: the tick rate through `ref.sim.tick_period`), 9 (§3: DEC id forms, the reserved block, the design phase's four-digit ids written as "DEC-NNNN, from one thousand upward"); the removed-requirement convention is stated in §3.
- **Files edited:** `design_next/CONVENTIONS.md`; `design_next/glossary/README.md`, `a_to_l.md`, `m_to_z.md`; `design_next/assets/GameAssets.md`, `README.md`, `art_and_audio_briefs.md`, `asset_manifest.csv`.
- **Helper scripts** (`species_work/scripts/`, run with `python3 -I`): `P6-GLO_assets.py` (the manifest rows and the matching GameAssets table cells; every replacement asserts its old text), `P6-GLO_strings.py` (mechanical count of cited string ids for GameAssets §18.1), `P6-GLO_consumers.py` and `P6-GLO_deadcons.py` (manifest consumers against the library), `P6-GLO_cites.py` (citations of given asset ids), `P6-GLO_registry.py` (sketch declarations against CONVENTIONS), `P6-GLO_order.py` (glossary order). Besides the manifest, the 8.12-B block of CONVENTIONS, the §18.1 family table and the mechanical table-cell changes of GameAssets were written by script (each asserting the exact old text); every other change is an Edit.
- **Gates** (`python3 -I Tools/DesignCheck/CheckDesign.py design_next --skip-mermaid`, last run after all edits): gates 1–3 and 6–11 PASS; gate 4 skipped (no diagram touched). Gate 5 FAILS with 34 hits, all in the generated `requirements_index/` (it cites stubs such as AEN-101, R-META-416, PLT-113 that the coordinator is deleting); none is in or from my files, and regenerating the index clears them. Gate 7: 438 asset ids, 1397 manifest rows, every row consumed.

## Findings

| Id | Status | Note |
|---|---|---|
| R2a-1 | fixed | Status line: "Reference library conventions. They bind every document of this library and every later change to it." |
| R2a-2 | fixed | §8 opening sentence as proposed. |
| R4b-48 | fixed | Collector row: the Worker is the node owner's, subject to that team's cap; it waits at a population-locked node (DEC-080). |
| R3b-22 | request | No component specifies the maths library, and writing one is outside my files. CONVENTIONS now has one owner group "Engine maths library" with all six types and the rule that DEC-241 items 6, 7, 9 and 10 state their numeric rules until a component does. The specification itself is request 1. |
| R3a-3 | fixed (+ request) | §3 "Restated rules": a restating requirement names its owner ("restates R-…"), the owner governs, and the design phase gives the restating id its owner's verdict. Marking the existing restatements in the reference documents is request 2. |
| R4b-6 | fixed | §5 defines a nominal duration ("nominal X s", DEC-241 item 3, DEC-859) and the derived value (ticks × `ref.sim.tick_period`). Rows 7 to 20 of the finding are already labelled "nominal" in their documents (checked). |
| R2a-81 | fixed-differently | The registered XAPOs now match the files: `StereoEchoFx`, `StudioReverbFx`, `ResonantLowPassFx`, `BitCrusherFx`, `GargleFx`, `EchoFx`, `SimpleReverbFx` (AEN-070, P6-ENG1 request 4). The proposed set (`ReverbFx`, `LowPassFx`) names effects no document declares. Recording the set in ADR-024 is P6-ENG1's request 5 to P6-ADR. |
| R4a-27 | fixed | §8.7 gains gun, rocket and flame turret (outcome): `CrateOutcome::TurretGun`/`TurretRocket`/`TurretFlame`, prefixes `crate.turret_gun`/`crate.turret_rocket`/`crate.turret_flame`, as reference/04 and the ledger write them. |
| R2b-251 | fixed | GameAssets status: "Asset list (consumers named by requirement id)". |
| R2b-254 | fixed | Socket vocabulary: "Indexed sockets count from 01 without gaps, legs included." |
| R4b-70 | request | `match.kill_milestone` stays registered (§8.6) and in GameAssets §18.1, which now says what it is and that no rule shows it yet. The rule that shows it belongs in reference/01 (request 3). |
| R2b-252 | fixed | §3 note: "every row with no reference counterpart (…)". |
| R2b-249 | fixed | assets README status: "Asset overview". |
| R2b-250 | fixed | Briefs status: "Style guide and category briefs". |
| R2b-257 | fixed | The per-file "N s reference length" is gone from all 898 audio rows; each group keeps a target duration (R2b-258). |
| R4b-64 | fixed | `music.front_end.theme` and `music.match.bed`: priority `post-launch`, role "authored only if DEC-049 selects option A (R-AUD-407)", in the manifest and GameAssets §16; §2.5 and §16 say so. Their duplicated spec strings are cleaned too. |
| R3a-5 | fixed | Glossary "host" (R-MODE-010, R-META-087, R-SIM-523). |
| R3a-4 | fixed | Glossary "participant" (R-SIM-530), with DEC-200 for the Species authority model. |
| R3a-6 | fixed | Glossary "session", "private local session", "hosted session" (R-META-029, DEC-200, DEC-243). |
| R3a-7 | fixed | Glossary "friend (friendship test)" (R-SIM-412 to R-SIM-414). |
| R2a-9 | fixed | LRU: the reference-cache clause is deleted; owner is the UI layout document only. |
| R4b-38 | fixed | Essence orb: 61, 301–500, 901 ticks. |
| R4b-26 | fixed | Audio engine: one update per `aen.update.period` (AEN-120); also states that the engine has no decoder (AEN-251). |
| R4b-29 | fixed | dt: the tick period at every game speed (R-SIM-361, DEC-039). |
| R4b-30 | fixed | Game speed: merged with P6-RE request 3 ("how many world ticks run per tick period of real time … every world tick has the same length and the same dt", DEC-039, R-SIM-548). |
| R2a-10 | fixed | Sample cache: reference sentence deleted. |
| R4b-37 | fixed | Strike team: 601 ticks, nominal 60 s (also V1 new finding 6). |
| R4b-28 | fixed | Tick period: advances game time by the tick period at every game speed (DEC-039). |
| R4b-27 | fixed | Tick: as proposed, with `ref.sim.tick_period` (ruling 8). |
| R4b-60 | fixed | Time limit: "or none (no limit) where the mode offers it; none is never written as 0 (R-MODE-074, DEC-120)". |
| R4a-51 | fixed | One "maths types" group (`Vec2`, `Vec3`, `Mat3x4`, `Mat4`, `Grid2D`, `Ray`); the 8.12-A rows and the RND/DBG entries are removed. |
| R4a-53 | fixed | The "turret" row has the prefix `building.turret`. |
| R4a-49 | fixed | The three `Ui*Pointer*` rows are deleted; the UI core row lists `PointerEventKind` (`PrimaryDown`, `PrimaryUp`, `AltDown`). |
| R4a-50 | fixed | The 8.12-A rows of `AppWindow` and `WindowMode` are removed; the PLT row carries them with their terms. The 8.12 intro now states the rule (a name of 8.12-B is not repeated in 8.12-A; its term follows it in brackets), and the same was done for the terrain, sound and DSP records, `ColorRgba` and `SceneSnapshot`. |
| R4a-57 | fixed-differently (+ request) | `Turret`: the *Words of a term* rule states the one exception (the base structure of the `TurretType` family), which keeps CONVENTIONS consistent with reference/03, reference/12 (its diagram) and the ledger. `SnowFlake` → `Snowflake` is a real fix but needs the same change in six documents and the ledger, so it is request 4; CONVENTIONS keeps `SnowFlake` until then. |
| R4a-42 | fixed | BLD free-functions group (`CurrentBuildIdentity`, `DisplayVersion`, `FullIdentityText`, `VersionResourceQuad`, `ChannelTag`). |
| R4a-41 | fixed | DBG free-functions group with all twelve; `FatalExit` and `CheckFailed` leave the types row. |
| R4a-40 | fixed | "`WindowEvent`, the variant of the event structs `Resized`, … and `EndOfSession`". |
| R4a-39 | fixed | PLT free-functions group: all 21 that the sketch declares now (the 18 of the finding plus `InstallFatalHandlers`, `CheckOperatingSystem`, `SetPlatformLogSink` of P6-ENG2). |
| R4a-44 | fixed | `UiWindow` (UI window) in the UI core row. |
| R4a-37 | fixed | DAT types row: `Severity`, `SchemaVersion`, `LoadError` (with enumerators and its other users), `SettingType`, `ApplyScope`, `SettingsFileContent`, `Limit` with `Limit::Form`, `EngineAssetTable`, `EditionManifest`; DAT free functions `WriteTelemetrySession`, `ReadQueuedTelemetrySession`. |
| R4a-38 | fixed-differently | The DAT `Species` group "map, structure and game records" registers `MapDifficulty`, `SiegeSide`, `CoopGroup` (with enumerators), `StartPosition`, `StartingForcesRecord`, `RouteRecord`, `ActorStats`, `CrateOutcomeEntry`, `ExplainerKey` and P6-DAT's additions. `CameraViewRecord`, `CameraAnimationRecord` and `CameraAnimationStepRecord` no longer exist: the camera records are now the `Engine` types `CameraView`, `CameraAnimationStep`, `CameraAnimationDef`, in the DAT "terrain and camera records" group. |
| R2b-256 | fixed | Every mesh and collision budget is rounded up to a class (100, 300, 1000, 3000, 10000, 25000) in GameAssets and the manifest; §11, the briefs and the README say so. Written without digit separators so that the validator reads the number as one token. |
| R2b-255 | fixed | Second option: the vocabulary lists the unindexed forms, as their consumer rules name them (`muzzle`, `casing_eject` R-UNIT-342, `tail` R-UNIT-594, `status` R-BLDG-017). Indexing them would have broken those rules. |
| R2b-259 | fixed | §13 intro: the groups fit the bank budgets of AEN-243, measured on the authored set (AEN-304, ADR-025). |
| R2b-258 | fixed | "target duration about X-Y s", widened outwards to tenths (a single value rounded), in GameAssets §13 to §15 and the manifest. |
| R4a-67 | fixed | §13.40 Event column uses the §8.7 terms. |
| R2b-264 | fixed | Same change, in GameAssets and the manifest roles. |
| R2b-253 | fixed | "String ids added for the assets, with the documents that cite them". |
| R4b-69 | fixed-differently | `ui.dialog.sound_settings_failed` is removed instead of cited: the sound options apply no longer restarts the device (AEN-261, R-META-437), so the failure it names cannot happen. |
| R2b-263 | fixed-differently | §8.7 registers the turret outcomes with the string-id form the owning document uses, `crate.turret_gun` (matching `CrateOutcome::TurretGun` and the ledger). The asset ids `icon.power.gun_turret` and `sfx.crate.outcome.gun_turret` are a separate kind named for what the asset shows; both serve all three turret outcomes (R4b-65), so they keep their names. Renaming either side would touch finished files for no behavioural change. |
| R2b-261 | fixed | "second graphics back end of the reference (not built)". |
| R2b-262 | fixed | Merged into "edition and store features (not built)", 1 texture and 8 text entries. |
| R2b-260 | fixed | Briefs §12: "Variant counts are proposed (DEC-564, DEC-097)"; the matching manifest specs say "variant count proposed (DEC-564)" or "(DEC-097)". |
| R4b-66 | fixed | `icon.power.airstrike`: "airstrike and Firebomb Run outcomes and powers"; the airstrike sting's role says the Firebomb Run plays it until DEC-565 decides. |
| R4b-65 | fixed | `icon.power.gun_turret`: "gun, rocket and flame turret outcomes and powers"; the gun-turret sting likewise. |
| R4b-68 | fixed | Spec: "positional, following the source, as the fuelled layer; 2-D as a layer of the lobby bed". |
| R4b-67 | fixed | Role "Mode complexity badge (simple, medium, hard) on the mode buttons", files `ui_complexity_simple/medium/hard.png`. The asset id `uitex.difficulty_badge` is kept, because documents cite it and ids are stable. |
| R3a-25 | fixed | "base actor advance" (R-SIM-111, R-UNIT-024). |
| R3a-27 | fixed | "command frame" (R-SIM-510, R-SIM-549). |
| R3a-26 | fixed | "draw value" (R-MODE-057). |
| R3a-20 | fixed | "held (world held)" (R-SIM-550, R-SIM-549). |
| R3a-29 | fixed | A row "local team" beside "local player team". |
| R3a-18 | fixed | "object serial" (R-SIM-108, R-SIM-138). |
| R3a-23 | fixed | "obstacle push" (R-SIM-380, R-SIM-381). |
| R3a-21 | fixed | "population-locked" (R-ECON-052, R-ECON-074). |
| R3a-19 | fixed | "presentation clock" (R-SIM-573, ADR-036, DEC-024); `PresentationClock` carries the term in 8.12-B. |
| R3a-28 | fixed | "sequencer", also called the command sequencer (R-SIM-510, R-SIM-444). |
| R3a-24 | fixed | "structure push" (R-BLDG-033, R-BLDG-020). |
| R3a-22 | fixed | "used slot (used team)" (R-SIM-395, R-SIM-221). |
| R1b-7 | fixed | "API" abbreviation row. |
| R2a-11 | fixed | "flavour" is gone; "configuration (build)" with `BuildConfiguration`. |
| R2a-12 | fixed | Both rows drop the slot counts and name `PageId`. |
| R4b-41 | fixed | 151 ticks. |
| R4b-40 | fixed | 151 ticks. |
| R4b-39 | fixed | 151 ticks. |
| R4b-42 | fixed | 151 ticks. |
| R4b-46 | fixed | 101–150 ticks. |
| R4b-44 | fixed | 21–30, 11 and 11 ticks. |
| R4b-31 | fixed | Countdown: one tick per command frame, unchanged by game speed (R-MODE-040, R-SIM-552). |
| R4b-61 | fixed | "crate drop period … none means no Supply crates". The unused "crate drop mode" row became "crate placement", the option's name in R-MODE-074 (same defect class). |
| R4b-45 | fixed | 51 ticks; one tick added and one subtracted (R-UNIT-179, R-UNIT-180). |
| R4b-43 | fixed (+ request) | Glossary: 51 ticks, 50 or 51 requests. DEC-002's list is request 5. |
| R4b-55 | fixed | "a numeric Transport cap of 1 or more". |

## Requests from other packets applied

| From | Request | Applied |
|---|---|---|
| P6-ADR 2 | BLD names in CONVENTIONS | (a) `species_contentbuild.exe`; `species.exe` added beside it. (b) The `BuildIdentityFields.h` sentence as given. (c) "tools solution" → "tool project" in §8.14 (also V1 R3b-1). |
| P6-DAT 1 | 8.12-B registry moves | Applied group by group: AST "content types" (`ContentType`, `PackCodec`, pointer); AEN pointer sentence; new DAT groups "import records (generated)", "sound records (generated)", "terrain and camera records of a map (generated)"; DAT types row; soak group renames; DBG pointer; DAT `Species` additions; the §8 *Data names* sentence and list wording. Two adjustments: `DspFilterKind` has the nine enumerators of the files now (P6-ENG1 4 is later and matches the files), and `MeshRole` sits in the import-records group, since it is generated from `import.mesh`, with the note "read by the asset pipeline's cookers". |
| P6-DAT 2 | `SourceLocation` under DAT | Applied (CAM row points to it). |
| P6-DAT 3 | drop `screen` from the 8-A setting roots | Applied; the remaining roots were checked against data-schemas table 3.1-A. |
| P6-DEC 3 | "excluded outcomes" row | Applied; the "Comeback Bonus" row also says the draw is over the members the map does not exclude (coordinator relay 3, DEC-452 Resolved under O18). |
| P6-DEC 5 | GameAssets §2.5 cites DEC-229 | Applied. |
| P6-ENG1 4 | AEN registry; 8.12-A deletions; INP data names | Applied. `AUDIO_BUS_COUNT` is registered in the AEN types row. `UnknownFlag` is not added: the INP diagnostic classes are not registered (only `LoadDiagnostic`). |
| P6-ENG2 4 | PLT and DBG registry and terms | (a) `ModeSize` removed. (b) `PlatformLogSink`, `PlatformLogLevel` and the three functions added. (c) `WindowsHeaders.h` in a PLT "files" group (a header is a code file, not a data name). (d) `SoakHost` added. (e) the four terms added to §8.14. |
| P6-RA 3 | remove the 16 objective-card pictures | Checked first: no document cites any of the 16 ids. Rows removed from the manifest and GameAssets §12; §2.5, §3, §12, §20, the README and the briefs adjusted (the 16 reference pictures move to §20 "drawn by no rule"). |
| P6-RB 15 | eight 8.12-A game types | Applied in alphabetical order. |
| P6-RB 16 | glossary "structure-detail level" | Applied as given. |
| P6-RB 17 | Manning "operating-port state" | Applied. |
| P6-RB 19 | `help.hint.spawn_nodes` | Removed with P6-RA 3. |
| P6-RC 13 | asset removals, consumers, §8.10, Supply crate glossary | Applied: `icon.power.laser`, `icon.power.slots_upgrade`, `icon.hud.slot_discard` removed (uncited); consumers of `cursor.pointer` and `icon.power.grenade` updated; §8.10 rows Improved, Objectives complete and Unlocked deleted, with their glossary rows; the Supply crate row cites DEC-859 for the expiry. `icon.power.rocket` lost its last consumer with the requested change, so its row is removed too. **Held back:** the six rows `cursor.select`, `cursor.move_target`, `cursor.enter` and their shadows, because RND-366 (`renderer/entity_and_overlay_renderers.md`, P6-RND finished) still cites the three shadows, and removing them fails gate 7. Their consumers now name RND-366 only; request 6 removes them. |
| P6-RD 15 | six outcome sting groups | Added after the 25 bound stings, `post-launch`, consumers `R-AUD-139;R-AUD-411`, role "… bound only if DEC-565 binds it"; GameAssets §13.40 is now "(31 files)" with a note. |
| P6-RD 16 | string counts | `ui.options.*` 42 → 30 and `ui.dialog.*` 6 → 5 in §18.1, from a mechanical recount (which also updated `crate.*`, `mode.*`, `power.*`, `ui.match.*`, `ui.power_slots.*`, `ui.lobby.*` and `unit.*`; 601 → 590 ids). The reference-category table: display options 13 → 7 and general options 11 → 5 (total 221 → 209); the six sound strings are assumed to be counted under "general options", since the table has no sound row; the total is exact either way. |
| P6-RE 3 | Time Dilation, Time Rush, game speed | Applied (also V1 new finding 7). |
| P6-RND 12 | renderer and VFX registry | Applied, with three adjustments: the GPU-resource types form their own RND group "GPU resources (ADR-078)" rather than joining the frame group; `ParticleSlot.h` is registered with the VFX GPU structs, since it is the VFX runtime's header; `ColorRgba` and `SceneSnapshot` move out of 8.12-A into the RND row (with their terms), so each is written once. **Rejected:** the `FrameRenderer` members `Resources` and `SetPalette`, because §8 *Registry* does not register members. |
| P6-RND 13 | assets | Nothing to add; the cursor rows are as under P6-RC 13. |
| Coordinator relay 2 | V1 new findings 6 and 7 | Done (Strike team 601 ticks; Time Dilation and Time Rush 151 world ticks). |
| Coordinator relay 3 | R4b-82 glossary | Done ("excluded outcomes" and "Comeback Bonus"). |
| Coordinator relay 4 | R2b-166 dropped DSP kinds | Done: `ChorusFx`, `CompressorFx`, `DistortionFx`, `FlangerFx`, `StandardGargleFx` are gone, and `DspFilterKind` lists the nine kinds; the glossary "DSP filter definition" says nine. |

Conflicts resolved by the files as they now are: P6-DAT 1's 14 DSP enumerators against P6-ENG1 4's nine (the nine are in DAT §2 now); R2a-81's proposed XAPO names against the files (the files win); R2b-263's `crate.gun_turret` against R4a-27's `crate.turret_gun` (reference/04, `CrateOutcome::TurretGun` and the ledger use `turret_gun`).

Further consistency edits in my files, no request asked for them: glossary "audio decoder" removed (the engine has no decoder, AEN-251) and "stream head" no longer says "decoded"; "always-on feature flag" and "feature flag" restated from INP-143 and BLD table 3.2-B; "DSP" states the dry mix of AEN-222; "hint overlay" no longer mentions Supply-crate hints (R-CRATE-146); "reference test machine" follows DBG-209 and DEC-971; "soak entry" names the automated participants; "product (build)" no longer counts test projects as products; "action bindings" drops a stale action count; "FPS" writes the tick rate through `ref.sim.tick_period`; new rows "tool project" and "crate placement". CONVENTIONS §8 *Namespaces* gains the ruling-7 sentence (`SoakEntry::automatedParticipants`, `RenderSettings::decorationDetail`, `RenderSettings::crowdCardDistanceScale`, `FrameKind::Loading`, `FrameKind::Editor`), §8 gains a *One declaration per type* bullet (ruling 6), and §8.14 gains the new glossary terms. GameAssets §2.5 says "Strike team (with its grenade weapon)": no rocket weapon remains.

## Requests

1. **Coordinator (P6-ENG2 has finished), the maths library (R3b-22).** Specify `Vec2`, `Vec3`, `Mat3x4`, `Mat4`, `Grid2D` and `Ray` in one engine component, a section of `architecture/components/platform_layer.md` or a short new component: element type (binary32 or binary64), layout and alignment for the shared C++/HLSL header (RND-262), DirectXMath interop (ADR-016), the operations, the ray–triangle test that TER-017 cites, and the numeric rules of DEC-241 items 6, 7, 9 and 10 as the baseline. Then change the owner cell of CONVENTIONS 8.12-B row "Engine maths library" to that component and delete its last sentence.
2. **Coordinator, restated rules (R3a-3).** CONVENTIONS §3 now requires "restates R-…" on a restating requirement. Mark the pairs the reviewer found: R-MODE-302 (restates R-BLDG-275 to R-BLDG-288), R-MODE-403 (R-BLDG-329 to R-BLDG-336), R-UNIT-318 (R-ECON-069), R-BLDG-024 (R-ECON-014, R-ECON-015, R-UNIT-090), R-CTRL-074 (R-UNIT-096), R-MAP-062 and R-MODE-369 (R-ECON-052 to R-ECON-055), R-ECON-084 (R-BLDG-637); in each case decide which side owns the rule and add the marker to the other.
3. **Coordinator (P6-RA has finished), `reference/01_skirmish_rules_and_modes/README.md` R-MODE-067 (R4b-70).** Append: "and, for the local team only, the centre-screen message `match.kill_milestone` with the milestone value as its parameter when the team's kill total passes a value of `ref.sim.team.kill_milestones` ([reference/12](../12_simulation_model/04_containers_and_update_order.md) R-SIM-221)". Then in GameAssets §18.1, row `match.kill_milestone`, set the last cell to "[01_skirmish_rules_and_modes/README](../reference/01_skirmish_rules_and_modes/README.md)". If the coordinator decides instead that no message is shown, delete the CONVENTIONS §8.6 row "kill milestone" and that GameAssets row.
4. **Coordinator, `SnowFlake` → `Snowflake` (R4a-57).** One rename in every place, in one change: `reference/04_supply_crates_powers_and_weapons/weapons_and_projectiles.md` (R-CRATE-500 and the class diagram line `SimObject <|-- SnowFlake`), `reference/12_simulation_model/03_entity_model_and_identity.md` (diagram line, table row 89, the spawner row), `reference/12_simulation_model/04_containers_and_update_order.md` R-SIM-234, `architecture/components/renderer/entity_and_overlay_renderers.md` (the snowflake row), CONVENTIONS 8.12-A row "snowflake", and the ledger row "snowflake". Two diagrams change, so run gate 4 afterwards. If the rename is not wanted, nothing needs to change.
5. **Coordinator (P6-DEC has finished), `decisions/verdicts_and_fixes.md` DEC-002 (R4b-43).** In the list of tick counts, "Spawn Surge 50 or 51 (DEC-304)" becomes "Spawn Surge 51 (50 or 51 requests, DEC-304)".
6. **Coordinator (P6-RND has finished), `architecture/components/renderer/entity_and_overlay_renderers.md` RND-366, then my asset rows (P6-RC 13).** In RND-366 replace "(`cursor.world.shadow`, `cursor.highlight.shadow`, `cursor.select.shadow`, `cursor.move_target.shadow`, `cursor.place.shadow`, `cursor.enter.shadow`, `cursor.unavailable.shadow`)" with "(`cursor.world.shadow`, `cursor.highlight.shadow`, `cursor.place.shadow`, `cursor.unavailable.shadow`; R-CTRL-350)". Then delete the six manifest rows `cursor.select`, `cursor.move_target`, `cursor.enter`, `cursor.select.shadow`, `cursor.move_target.shadow`, `cursor.enter.shadow` and their six rows in GameAssets §8; in GameAssets §3 and the assets README the Cursors row becomes 29 ids, 41 files, 30 first-playable, the totals 432 ids, 1391 files, 644 first-playable, and the reference column 25 (§3 total 1436); the §3 note's "10 derived-variant rows" becomes 7; GameAssets §20 "drawn by no rule" becomes 24 textures (totals 139 and 1092) and the README's "808 reference files" becomes 811.
7. **Coordinator, optional consistency.** `reference/00_reference_overview.md` R-VIS-173 says "Supply crate drop rate and placement mode"; the options are "crate drop period" and "crate placement" (R-MODE-074), as the glossary now says.

## Notes

- **O30 and BLD-043.** O30 says the Khronos glTF Validator is not used and tool dependencies come from NuGet only. `build_tooling_and_distribution.md` BLD-043 (P6-ADR) still says the validator is "fetched and checked against its recorded SHA-256 by MSBuild tasks". My glossary row "tool project" states the O30 rule; the BLD text is the coordinator's to reconcile.
- **Registry names that no other document uses.** `ThrownWeaponKind` (`AirstrikeBomb`), the `SimObjectId::Group` enumerators, `TeamId`, `ControlHintOverlay`, `CrashReporter` and `GracePeriod` are registered but written nowhere else. They are term-to-identifier mappings of glossary terms and outside my findings, so I left them.
- **Ledger.** The ledger still has rows for identifiers the library no longer declares (`AlwaysTrue`, `UiPointerDown`, `UiPointerHeld`, `UiAltPointerDown`, `PadMenuCycleUp`, `PadMenuCycleDown`). No original name appears in my files (gates 1 and 2 pass).
- **GameAssets size.** `GameAssets.md` is about 199 KB; gate 10 accepts it as it did before.
