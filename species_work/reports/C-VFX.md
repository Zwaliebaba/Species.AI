# C-VFX report: animation and VFX runtime

## Split

The output came to about 160 KB, which is over the 120 KB limit, so the package target `architecture/components/animation_and_vfx.md` is now the folder `architecture/components/animation_and_vfx/`. The folder has a `README.md` index and two sub-topic files. No file named `animation_and_vfx.md` exists. Requirement ids are split by file: README 001–099, procedural 100–199, particles 200–299. Ids 300–399 are not used.

## Files written

| File | Size (bytes) | Requirements | Tuning keys | Mermaid | Tables |
|---|---|---|---|---|---|
| `design_next/architecture/components/animation_and_vfx/README.md` | 46 725 | 25 (VFX-001 to VFX-025) | 3 | 3 | 12 |
| `design_next/architecture/components/animation_and_vfx/explainers_sprites_and_procedural_animation.md` | 53 739 | 45 (VFX-100 to VFX-144) | 73 | 2 | 12 |
| `design_next/architecture/components/animation_and_vfx/particles_shatter_and_screen_effects.md` | 63 061 | 38 (VFX-200 to VFX-237) | 173 | 1 | 13 |
| `species_work/scripts/C-VFX_assemble.py` | helper | | | | |
| `species_work/scripts/C-VFX_check.py` | helper | | | | |

**What each file holds:**
- **README:** the responsibilities and boundaries, including the neighbour contracts, the integrity table, the effect inventory and what is dropped. It also holds the C++23 interface sketch, the clocks, the presentation generator, easing, effect definitions, the catalogue entries this component owns, lifetimes, the frame order and the effect-event path.
- **Procedural file:** the explainer player, the sprite and billboard conventions, the Worker layers, the seasonal theme, team colour, the damage flicker, the spider legs, the centipede, the Reaper Serpent, the Crawler strips, the Brood tails and the cloth flags.
- **Particles file:** the GPU particle pool and its 17 types, the effect-event catalogue, the presentation emitters, shatter debris, camera shake, full-screen flashes and photosensitivity.

`C-VFX_assemble.py` built the first version of the folder from scratch parts. The files were edited directly after that, so the folder files themselves are the source.

## Counts

- **Requirements:** 108 (25 + 45 + 38). Ids are unique and increasing in each file, and all are inside the range.
- **Tuning keys:** 249 `vfx.*` keys (3 + 73 + 173). Each key is defined once in `design_next/` and cited by a requirement of its own file. Values that other documents own are cited by key and not duplicated:
  - 106 `ref.*` keys: weapons, creatures, Worker, defences, mode objects, trees, controls, meta, simulation;
  - 3 `rnd.*` keys.
- **Citations:** 196 requirement ids of other documents (R-*, RND, TER) and 31 asset ids.
- **Diagrams:** 6 Mermaid blocks, each with a caption line. All parsed with 0 failures using the scratchpad mermaid parser and `species_work/scripts/G06_mermaid.mjs`.
  - README: the neighbour flowchart, the frame-order flowchart and the effect-event sequence diagram.
  - Procedural file: the explainer panel state diagram and the procedural foot state diagram.
  - Particles file: the particle lifecycle flowchart.

**Enumerations:**

| Enumeration | Count |
|---|---|
| `EffectKind` (table 4.7-A) | 49 enumerators in 48 rows |
| `ParticleKind` (table 4.2-A) | 17 |
| Effect and animation inventory | 36 rows |
| Integrity cases | 11 |
| Reference clocks with their Species readings | 7 |
| Catalogue entries this component owns | 11, plus the depth-bias variant `bias_flag_pointer` |
| Object lifetimes | 7 |
| Neighbour contracts | 7 |
| Worker draw layers | 14 |
| Damage-flicker kinds | 6 |
| Procedural state records | 6 |
| Other procedural motions | 9 |
| Cloth-flag owners | 3 |
| Presentation emitters | 5 |
| Particle idioms | 4 |
| Shatter constants | 11 |
| Shatter sources | 17 |
| Shake sources | 4 |
| Flash sources | 2 |
| Reference limits and their Species counterparts | 8 |
| Failure rows | 20 |
| Acceptance tests | 43 (9 + 16 + 18) |

## Checks run

- Both `scan_patterns.py` runs, with `forbidden_patterns.tsv` and `soft_patterns.tsv`, on all three files: no output.
- `C-VFX_check.py /home/user/Species.AI`: OK. It checks:
  - the §1–§9 headings;
  - the id range, order and uniqueness;
  - that every cited DEC, ADR, asset, key and requirement id exists;
  - that each `vfx.*` key is defined once and cited by a requirement;
  - that every Open entry of `decs_to_cite` is cited by a requirement paragraph;
  - that every asset in `assets_for_you` is cited;
  - no truncation words;
  - that every link resolves to a file or a package target;
  - that each file is 120 KB or smaller.

## Open DECs cited

- **All Open entries of `decs_to_cite`:**
  - DEC-101: seasonal theme (VFX-025, VFX-119);
  - DEC-106: performance envelope, caps and level of detail (VFX-023, VFX-025, VFX-214);
  - DEC-328: saturated bubble colours (VFX-025, VFX-121);
  - DEC-562: weather (VFX-025, VFX-224);
  - DEC-750: explainer loop and visibility; the default is the reference behaviour (VFX-025, VFX-107).
- **Other Open DECs cited where they apply:**
  - DEC-200 and DEC-222: effect transport and snapshot (VFX-006, VFX-025);
  - DEC-228: stress population and fixed-capacity pools (VFX-023, VFX-025, VFX-212, VFX-230);
  - DEC-242: generator algorithm, with one presentation generator per thread (VFX-011).
- **Resolved, stated as the fixed rule:** DEC-007, 008, 009, 012, 013, 015, 029, 098, 118, 461, 654.
- **Dropped, cited in the drop rule:** DEC-059, 062, 336.

## New DEC requests

Each request below has a stated default in the files. The README §8 lists them as design-phase inputs.

1. **Explainer layout on resize** (VFX-108).
   - The reference freezes the panel layout when the panel is created. Species recomputes it on every resize, following ADR-0033.
   - Options: A, relayout (the default); B, freeze as in the reference.
   - G09a's R-META text describes the frozen layout.
2. **Emitter values the reference does not record** (VFX-219, VFX-025). Defaults:
   - control flash on control gained or lost: 1 particle;
   - the demotion burst reuses the promotion burst;
   - the blue spark of a Worker opening a crate: 1 per tick;
   - the fireball-launch flame sizes;
   - the terrain-hit trail of a turret round: 1;
   - the spiralling-rocket burst uses the `Explosion` core rule in one team colour;
   - the order marker emits once per tick.
3. **Damage-flicker cadence** (VFX-126).
   - R-UNIT-051 says "each render frame". The reference cadence is once per 0.1 s of scaled presentation time, and Species keeps that as `vfx.flicker.step`.
   - Options: A, per 0.1 s (the default); B, per frame.
4. **Per-frame emitters become fixed-rate steps** (VFX-018).
   - Every per-frame emission and blend factor in the reference runs at `vfx.emit.step_rate` (60 steps per second).
   - Blend factors convert as 1 − (1 − k)^(60Δ). This makes effects independent of the frame rate, where the reference depends on it.
   - It affects R-BLDG-654 and the cloth relaxation.
5. **Cloth during a pause** (VFX-139). Flags keep their shape while the world is held. The reference keeps animating them at a fixed step, kept here as `vfx.cloth.still_step`.
6. **Accessibility defaults** (VFX-232, VFX-235; ADR-0064 is Proposed):
   - reduced-motion shake scale 0.25;
   - reduced-flash scale 0.25;
   - at most 3 flash onsets in any 1 s of real time;
   - onset threshold alpha 0.1.

   Whether a setting turns shake off entirely is open.
7. **Shard edge line width** `vfx.shatter.edge_width`, 1/64 barycentric (VFX-229). The reference does not record it.
8. **Eaten-orb orbit offset** `vfx.serpent.eaten_offset`, ±10 wu per axis (VFX-122). The reference range is not recorded.
9. **Reference quirks kept as the default look** (VFX-025, VFX-138, VFX-139):
   - one smoothed tail velocity shared by every Brood Drifter;
   - the fixed relaxation factor of large or selected flags, which replaces the factor instead of scaling it;
   - the frame-quantised explainer loop.

   Options: keep the quirk, or correct it.
10. **Particle sub-step limit**: at most 30 sub-steps per frame, which covers game speed 2.0 (VFX-209). The engine sets this baseline. DEC-106 confirms or changes it together with the caps and level-of-detail distances of VFX-212 and VFX-213. No new DEC is needed if DEC-106 covers it.

## ADR conflicts

There are none. ADR-0010, ADR-0035 and ADR-0036 are followed, and ADR-0033, ADR-0064 and ADR-0074 are cited. Three interpretations should be recorded in the ADRs if the coordinator agrees:

- **ADR-0036, "one presentation clock":** the clock gives three readings of one platform time source (ADR-0074). The scaled reading follows game speed and stops while the world is held. The unscaled reading stops while held. The real reading never stops; it drives the UI and explainers (VFX-009, VFX-016).
- **ADR-0010, "one draw per explosion":** all live explosions are drawn with one ExecuteIndirect call, with one argument record per explosion (VFX-237).
- **ADR-0064** is still Proposed. The flash and motion rules of VFX-232 and VFX-235 depend on it.

## Missing assets

None. All 17 assets in `assets_for_you` are cited. Two manifest notes go to X-AST (see below):
- `fx.shard_edges` is not sampled at run time. The shard edge comes from a barycentric mask, as ADR-0010 says, and the asset serves as the art reference only (VFX-229).
- `help.explainer_picture` says "no mips", but R-META-231 samples it "filtered linearly with mip maps".

## Cross-document requests

### Coordinator

The split leaves stale links. 45 files in `design_next/` link to `animation_and_vfx.md`:
- the 7 renderer files and `ui_framework/pages_and_screen_recipes.md`;
- `asset_pipeline_and_resources.md`;
- ADR-0010, ADR-0035 and ADR-0036 (their "More information" sections);
- `CONVENTIONS.md`;
- 6 decision files;
- both glossary files;
- the reference documents of 02, 03, 04, 06, 09, 11 and 12/07;
- the `consumers` column of 46 rows in `asset_manifest.csv`.

Map the links to the folder files as follows:
- `particles_shatter_and_screen_effects.md`: particles, passes 14 and 15, the effect batch, shatter debris, shake and flashes.
- `explainers_sprites_and_procedural_animation.md`: the Worker layers and ground shadow, the flicker, cloth flags, legs, Crawler strips, Brood tails, bubbles and explainers.
- `README.md`: the clocks, the presentation generator, effect events, catalogue entries and general references.

Packages still running may add more such links.

### C-RND

C-RND-a's and C-RND-b's requests to C-VFX are all answered:
- orb sizes and the one-tick-behind rule: VFX-225;
- Worker ground shadow: VFX-118;
- flashes and shake: VFX-232 to VFX-235;
- shatter fractions: table 4.10-A;
- flicker axes: VFX-127;
- Crawler strips: VFX-137;
- cloth entries and the `marker.flag_pointer` bias: VFX-014 and `vfx.cloth.pointer_*`;
- bubbles: VFX-121;
- route-highlight, selection and placement particles: VFX-219 and VFX-220;
- bottom-anchored billboards: VFX-224.

Laser, shockwave and lightning values are cited from G04 and C-RND, not duplicated.

Requests back to C-RND:
- RND-082: the particle pool (pass 15) is drawn at presentation time from the GPU state and is not a sliced simulation pool. Drop it from the list.
- RND-270: "multiplies by the edge mask" should read "lerps the facet colour to white by the edge mask" (VFX-229).
- RND-306: state that the Worker card's billboard blend has no mirroring.
- RND-308: add a textured additive entry with no depth test, `unlit_textured`, `Additive`, `None`. The muzzle flash needs it below size 50 (VFX-222). `effect_gradient_nodepth` is untextured.

### Reference documents

**G02a1:**
- R-UNIT-051 should state the 0.1 s flicker cadence, or cite DEC request 3.
- R-UNIT-147 should describe the ground shadow as the Worker's own card silhouette, not a blob.

**G02b:**
- R-UNIT-539: the Crawler glow takes the worm colour, which is scaled by health, not the plain team colour.
- R-UNIT-578: `ref.unit.spider.max_lifts_per_tick` counts foot-selection runs, not lifts. Emergency lifts do not count (VFX-131).
- The spider gait and leg keys are cited, as G02b asked.

**G03a:**
- R-BLDG-272 says the damped factor "scales" the relax factor. In the reference it replaces the factor (VFX-139).
- Cite the `VesselBooster` and `VesselWrecked` effect values of table 4.7-A.

**G03b:** R-BLDG-654 ("each rendered frame") becomes "each step of VFX-018".

**G04:** R-CRATE-167, R-CRATE-421, R-CRATE-451, R-CRATE-460, R-CRATE-489 and R-CRATE-497 hold emitter literals. Cite the `vfx.emit.*` keys of table 4.7-A for them.

**G06 / C-CAM:**
- R-CTRL-452 lists shake as camera state, and R-CTRL-463 turns the stored front.
- Under ADR-0036, the shake scalar belongs to this component's screen effects. The camera applies the yaw and roll as an additive offset of the frame's view and never stores it (VFX-232).
- R-CTRL-463's "Launch Vessel's launch, scaled by distance" disagrees with the reference events. Every participant receives the vessel's shake ÷ 5 per tick, with no distance factor (VFX-233).

**G09a:**
- The explainer data model, sampling, clip and drawing are cited from R-META-220 to R-META-233. VFX owns only the player's behaviour (VFX-104 to VFX-111).
- R-META's frozen layout is replaced by ADR-0033 relayout (DEC request 1).

### Other components

**X-AST:**
- the role of `fx.shard_edges` (art reference only);
- the mip question for `help.explainer_picture`;
- consumer paths should name the folder files.

**X-GLO:** "19 particle types" becomes 17. Glossary links should point to the folder files.

**C-DAT:**
- the effect-definition schema of VFX-013;
- the explainer schema, which G09a also requested.

**C-AST:**
- cooked mesh nodes need a centre for shatter (VFX-227);
- validate the spider sockets `leg_00` to `leg_07` and `segment_end`, and the Brood `tail_01` to `tail_04` sockets;
- the recoloured flag variants of DEC-098.

**C-UI and G09b:**
- setting ids for the reduced-flash and reduced-motion settings of ADR-0064;
- the UI host of the explainer player, which draws in the UI batch on the real clock.

**Owners of neighbour types:** C-PLT, C-RND, C-UI, C-AST and C-SIM should align with, or rename, the placeholder neighbour types in the README sketch. They are:
- `PlatformTimeSource`: the ADR-0074 time source;
- `PassId` and `InstanceWriter`: the renderer's pass id and instance writer;
- `UiBatch`;
- `MeshAssetId`;
- `MatchSetupView`;
- `TeamColours`;
- `LoadError`.

## Names needed that are not in the ledger

These are proposals. All are new Species identifiers, and none is a rename of an original name.

- **Runtime types:**
  - clocks: `PresentationClock`, `PresentationTime`;
  - effect events: `EffectKind`, `EffectEvent`, `EffectEventId`, `ShatterPayload`;
  - particles: `ParticleKind`, `ParticleBlend`, `TerrainRule`, `ParticleEmission`, `ParticleTypeParams`, `PoolCounts`;
  - screen effects: `ScreenEffects`, `ShakeOffset`;
  - explainers: `ExplainerPlayer`, `ExplainerLayout`;
  - the rest: `VfxRuntime`, `ShatterExplosion`, `ProceduralAnimator`, `FootState`, `LegTuning`.

  `ParticlePool`, `ShatterEffects` and `FxRng` are already in the glossary.
- **GPU structs:** `Particle`, `ParticleType`, `Shard`, `Tumbler`.
- **`ParticleKind` enumerators:** RocketTrail, ExplosionCore, ExplosionDebris, MuzzleFlare, Fire, ControlFlash, Spark, BlueSpark, Casing, MissileTrail, MissileFire, WorkerFire, Leaf, Sickness, SpeedBoostTrail, FireballFlame, Volcano. `MuzzleFlare` avoids a forbidden pattern; `MuzzleFlashEffect` stays the weapons document's name.
- **`EffectKind` enumerators:** Explosion, RocketBurst, StructureDestroyed, LaserHitStructure, RoundHitStructure, RoundHitTerrain, ThrownWeaponTrail, AirstrikeMarkerSmoke, RocketFlame, FireballLaunched, FirebombBurst, LightningStrike, SwarmHit, TurretCooling, TurretShot, TrooperShot, BarrierSpark, ControlGained, ControlLost, LeaderPromoted, LeaderDemoted, CrowdOrder, LeaderOrder, LeaderFormUp, OrderMarkerStep, WorkerBurning, WorkerSpeedBoost, WorkerSick, PowerReachedWorker, WorkerOpeningCrate, WorkerAbducted, ActorHit, Bite, TransportExhaust, CollectorEngine, AbductorEngine, MeteorFire, OrbBroken, TreeBurning, TreeLeaf, SpitterPlantBurning, SmokeColumn, VolcanoEruption, FuelPumpStroke, FuelDepotBoarding, VesselBooster, VesselWrecked, CameraShake, Shatter.
- **Catalogue entries:**
  - `orb_ink`, `orb_additive`, `shard_lit`;
  - `particle_darkening`, `particle_additive`;
  - `cloth_cells`, `cloth_cells_fog`, `cloth_pole`, `cloth_pole_fog`, `cloth_pointer`, `cloth_overlay`;
  - the depth-bias variant `bias_flag_pointer`.

## Dropped as legacy

- Gesture rendering: gesture trails, symbol display and tutorial gesture playback.
- Screen-space distortion and pixelation ghosting (DEC-059), and the second graphics back end's debris and deform look (DEC-336).
- Effects of actor kinds no match creates (DEC-062): the three-legged walker, the Leader shield of orbiting orbs, the tank bob, the debug missile, and the rift's fire, flash and shake.
- The sound-chip envelope sequencer. The audio filter chain set up by a reference effects file moves to C-AEN.
- Emitter sites and shatter callers used only by campaign and legacy content.
- The two particle types used only by dropped content: the placement marker and the rift fire. 19 reference types become 17.
- The extinguisher burst, which no reference pool can produce.
- The spectacle shadow, which is inactive in matches.
- The positional panel-script grammar, now C-DAT's schema, and the per-mode choreographies, now X-AST's explainer assets.
- The single-player centipede band shatter.
- The network relay of the seasonal flag, which is now match state (DEC-118).
