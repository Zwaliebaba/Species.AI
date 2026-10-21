---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T13)
---

# The presentation tuning record

## Context and Problem Statement

Owner decision O75 puts the client's presentation values in a client-only record, `tuning.presentation`, with its schema, cooked as `tuning.mvp` is (ADR-098, ADR-108) and never simulation-relevant. M1 task T13 is its first reader: the look of the world frame. The record has three open points:
- which keys it holds, and in what form, given the schema generator's subset;
- which values stay out of it;
- where its generated type lives.

## Decision Drivers

* One source per value: a key in the record is read nowhere else, and a key of `tuning.mvp` is not repeated.
* The generator's subset (ADR-108): objects, integers, counts, ticks and decimals, plus lists of objects and integers, of which only fixed-size records are cooked.
* Neither the content build nor the server may link a client library.

## Considered Options

* 1 — the record in GameShared beside `tuning.mvp`, its vectors, colours and tables as objects of named decimals
* 2 — the record in GameClient
* 3 — lists of decimals, with an extension of the generator

## Decision Outcome

Chosen option: "1".
- Option 2 would make the content build link GameClient, and with it NeuronClient.
- Option 3 extends the generator for three keys, and a list makes the record's cooked size variable, which the generator does not cook.

* **The record**:
  - its schema is `GameData/Schemas/tuning.presentation.schema.json`;
  - its record is `GameData/Records/tuning/presentation.json`, with `x-species-sim: false`;
  - its generated header is `GameShared/TuningPresentation.h`;
  - the content build cooks it under the key `records/tuning/presentation`, beside the tuning record (`RecordCooker`'s registry), and the client loads it with `LoadTuningPresentation`.
  - The server never reads it, and it is not part of the simulation content identity (AST-100).
* **Its keys at T13** are the world frame's: `rnd.frame.*`, `rnd.view.*`, `rnd.fog.*`, `rnd.terrain.*`, `rnd.sea.*`, `rnd.sky.*`, `ter.color.*`, `ter.sea.*`, `ref.sim.match.presentation_seed`, and the two lights `mvp.light.key.*` and `mvp.light.fill.*`. The lights are new keys of the MVP scope's table 3, provisional, for the owner to tune after T13's run (O75).
* **Forms**:
  - A vector is an object of `x`, `y` and `z`, and a colour one of `r`, `g`, `b` and `a`.
  - `ter.sea.blur_taps` is an object of `tap_0` to `tap_10`.
  - `ter.sea.wave.x_terms` and `ter.sea.wave.z_terms` are objects of `first` and `second`, each of `per_wu`, `per_second` and `amplitude`.
  - `ter.sea.wave.color_mix` is an object of `other` and `own`.
  - Decimals use the generator's `sim_decimal` type at binary64, whose name is the generator's; the record's flag keeps it out of the simulation.
  - `rnd.fog.start_fraction` is written as the binary64 nearest 1/3.
* **Out of the record**:
  - The keys the client already reads from `tuning.mvp`: the region's side, cell and chunk; sea level; the tick period.
  - The settings' defaults `rnd.option.*.default`: the sample count of ADR-015, the sea and sky detail and the brightness. Their key names end in a C++ keyword, which the generator refuses, and they belong to the settings store (PLT). Until it exists they are named constants that cite their keys, as `STAGING_BUDGET_MIB` does `rnd.resources.staging_budget_mib`.
  - The engineering sizes of ADR-114, and the development view of ADR-115, which T14 removes.

### Consequences

* Good, because the look is tuned in data, rebuilt by the content build, and read without a parser.
* Good, because the server and the content build see the record's type and never a client library.
* Bad, because a list-valued key is spelt as named leaves, so its record paths are longer than the design's key.
* Bad, because the option defaults are constants until the settings store exists.

### Confirmation

* CI: `GameSharedTests/TuningTests` reads the committed record, checks its values and cooks it back to the authored record. `Build\CheckGeneratedSchemas.py` regenerates the header. The content build cooks the record on every build.

## Pros and Cons of the Options

### 1 — GameShared, named leaves

* Good, because nothing new is needed of the generator or the project edges.
* Bad, because the game's shared library holds a type only the client reads.

### 2 — GameClient

* Good, because the type lives with its one reader.
* Bad, because the content build would link the client.

### 3 — lists of decimals

* Good, because the record would mirror the design's keys exactly.
* Bad, because the generator would need a variable-size cooked form first.

## More Information

* Related: [ADR-062](ADR-062-authoring-syntax-and-schema-language.md), [ADR-098](ADR-098-tuning-in-data-and-the-schema-generator.md), [ADR-108](ADR-108-content-build-loose-content-and-cooked-formats.md), [ADR-115](ADR-115-the-world-frame-at-m1.md).
* Owner decision O75; DAT-016, AST-100; the MVP scope's table 3.
* **Amendment 2026-10-20 (M1 task T15a, [ADR-118](ADR-118-development-overlay-at-m1.md)).** The record gains the development overlay's keys: `dbg.overlay.frame_rate_right` and `dbg.overlay.frame_rate_bottom` (DBG-095), the frame-rate counter's `ref.sim.frame.fps_window` and `ref.sim.frame.fps_initial` (R-SIM-576). The player colours `ref.sim.team_color.<name>.base` (R-SIM-415) that colour the figure dots stay out of it: their last part would generate a type named `Base`, which R2 refuses, so they are named constants that cite their keys. The renderer receives the counter's two values from the client once the record is loaded.
* **Amendment 2026-10-21 (M1 task T15b, [ADR-120](ADR-120-frame-page-toggle-and-sandbox-restart-at-m1.md)).** The record gains the frame page's keys: `dbg.overlay.warning_rgb` (DBG-097), `dbg.profile.rollup_period_ms` (DBG-082), `dbg.profile.frame_history` (DBG-085), `dbg.profile.view.graph_height`, `dbg.profile.view.graph_full_ms` and `dbg.profile.view.graph_marks_ms` (DBG-089), `dbg.telemetry.percentiles` (DBG-096), and the frame's budget, `rnd.budget.target_frame_rate` (RND-454). The two lists of three values are objects with the fields `first`, `second` and `third`, as `ter.sea.blur_taps` is an object of named taps.
* **Amendment 2026-10-21 (M1 task T16, [ADR-121](ADR-121-workers-and-orbs-drawn-at-m1.md)).** The record gains the Worker and orb looks' keys: `ref.unit.worker.*` (R-UNIT-142 to R-UNIT-147), `rnd.crowd.worker_detail_distance` (RND-314), `rnd.pso.worker_cutout` (RND-060), `vfx.worker.*` (VFX-117, VFX-118), with the three provisional walk-bob keys of O75, and `ref.unit.essence_orb.*` (R-UNIT-330, R-UNIT-331). The option defaults `rnd.option.entity_detail.default` (1) and `rnd.option.worker_scale_factor.default` (2.0) stay named constants, as this record's rule for a key ending in a C++ keyword states.
* **Amendment 2026-10-21 (M1 task T16b, [ADR-122](ADR-122-leaders-and-banners-drawn-at-m1.md)).** The record gains the Leaders' and banners' keys: `rnd.tint.factor` (RND-161), `ref.unit.leader.banner_*` (R-UNIT-191), `ref.bldg.cloth_flag.*` and `vfx.cloth.*` (VFX-139 to VFX-142), `ref.unit.flicker.*` and `vfx.flicker.*` (R-UNIT-051, VFX-126), and `vfx.emit.step_rate` (VFX-018). `vfx.flicker.axis_shares` is an object with the fields `front`, `up` and `right`.
* **Amendment 2026-10-21 (M1 task T16c, [ADR-123](ADR-123-weapons-effects-and-particles-at-m1.md)).** The record gains the effects' keys:
  - the weapons' presentation keys `ref.weapon.laser.*`, `ref.weapon.muzzle_flash.*`, `ref.weapon.thrown.*`, `ref.weapon.explosion.*` and `ref.weapon.shockwave.*` (R-CRATE-405 to R-CRATE-475);
  - `rnd.laser.*`, `rnd.shockwave.*`, `rnd.depth_bias.*` and `rnd.frustum.point_tolerance` (RND-219, RND-234, RND-333 to RND-339);
  - `ref.ctrl.camera.shake`, `ref.ctrl.promote.*`, `vfx.screen.*`, `vfx.particle.*`, `vfx.emit.*` and `vfx.motion.order_marker_flare` (VFX-143, VFX-207 to VFX-235);
  - the provisional `vfx.thrown.*`, `vfx.worker.recoil_*` and `rnd.shockwave.disc_segments`.

  A draw "a + U[0, b)" is an object with the fields `base` and `spread`. A velocity of signed and unsigned draws is an object of each axis's range. A particle type's two colours are an object with the fields `first` and `second`. The keys `vfx.particle.default.*` stay out: their segment `default` is a C++ keyword, and no type of M1 leaves a field unset.
* **Amendment 2026-10-21 (M1 task T17a, [ADR-124](ADR-124-battle-verbs-at-m1.md)).** The record gains the battle verbs' input-layer keys: `ref.ctrl.select.circle_growth` and `ref.ctrl.select.circle_max_radius` (R-CTRL-156), `ref.ctrl.order.crowd_interval_ticks` (R-CTRL-160), `ref.ctrl.promote_radius` (R-CTRL-042), `ref.sim.command.control_state_period` (R-SIM-438), and the selection circle's `ref.ctrl.cursor.circle_particles`, `circle_spin` and `circle_lift` (R-CTRL-370).
