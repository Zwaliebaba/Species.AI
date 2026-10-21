---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M1 task T16), owner (O75, O77)
---

# The Workers and the Essence orbs drawn at M1

## Context and Problem Statement

M1 task T16 draws the crowds (M1 plan table 2 row T16; MVP-080, MVP-082). Its run is a battle of 400 between two AI sides, seen walking, fighting, falling, fading and rising as orbs. T16 lands in three steps:
- T16a, this record: the Workers, the orbs, and what both need;
- T16b: the Leaders and their banners, in its own record;
- T16c: the laser bolts, muzzle flashes, grenades, shockwaves, particles and the order flare, in its own record.

The baseline draws crowds through GPU culling and indirect draws (ADR-009). It also names a presentation clock and generator (ADR-036, VFX-009, VFX-011), and no code holds either yet. O75 chose CPU culling for M1, and the reference Worker look with motion; O77 chose the two AI sides of the run, `fx.glow_round` for the orbs, the shatter's deferral, and the flash and shake at T16.

Five things in the design do not hold as written:
1. **Missing data.** The card variation of R-UNIT-142 reads a pool slot index, which is not replicated (ADR-111).
2. **Handedness.** RND-314's card right axis, front × up, is the reference's left-handed form.
3. **Glow depth.** VFX-118 says a Worker's glows have no depth test "unless stated", and RND-308's `worker_glow` states a test.
4. **Ground corpses.** VFX-118 draws the body only for a living or airborne Worker, while R-UNIT-146 colours every dead one.
5. **The bob.** O75's walk bob has no design values.

## Decision Drivers

* The reference look as written, wherever M1 reaches it (MVP-082), with each deviation named.
* The presentation never writes simulation state, and its randomness never reaches the simulation's generator (DEC-007, DEC-013).
* A fixed number of draw calls per batch, whatever the population (RND-311, RND-455).
* Every value in data (CONVENTIONS §4), in the client's own record (ADR-116).

## Considered Options

1. **Batches of scene quads, culled on the CPU.** The game builds a frame's quads per pass, catalogue entry and texture; the renderer tests each quad, writes the survivors into the upload ring and draws a batch with one call.
2. **ADR-009 as written.** Per-type instance buffers, a compute cull and `ExecuteIndirect`.
3. **One draw per figure.** The reference's own submission (performance baseline H8).

## Decision Outcome

Chosen option: 1, because it is O75's M1 form of ADR-009 and keeps the baseline's fixed call count. Option 2 brings a compute pass and indirect arguments before any measurement asks for them. Option 3 is the cost the baseline exists to avoid.

* **The scene's quads** (`NeuronClient/WorldScene.h`, `SceneQuads`, `QuadRenderer`):
  - A `SceneQuad` holds RND-262's fields in binary64 world units: position, end point, axes, half-sizes, the billboard blend, rotation, colours, UV rectangle, orientation mode and anchor. The renderer makes it a `QuadInstance` relative to the camera (WLD-246).
  - A `QuadBatch` names its pass, its catalogue entry, its texture of the frame's `SceneTextures`, its cut-out threshold and the radius of its view test. The quads of a batch are drawn in the order the game wrote them (RND-313).
  - The vertex shader `UnlitTexturedVS` builds RND-306's four orientation modes, with the `BottomCenter` anchor of RND-305. Its blend towards the camera turns to the camera's left when the instance asks, so a card seen from behind is never drawn mirrored (VFX-117).
  - The pixel shaders:
    - `UnlitTexturedPS` multiplies texture and colour;
    - `UnlitTexturedCutoutPS` discards a texel at or below the threshold and otherwise takes the quad's alpha alone, so crowd cards have hard edges and fade only by their colour (RND-413);
    - `UnlitTexturedMaskPS` reads a BC4 mask as alpha.
  - The textures are uploaded once through the GPU resource factory and again after a device rebuild (RND-035).
* **The CPU view test** (ADR-009's M1 form, O75). Each quad's sphere is tested against the frame's six planes with `BoxMayBeVisible` (RND-234, conservative as RND-236 asks); a segment is tested by its middle and half-length. Only visible quads reach the ring.
* **The catalogue** gains `worker_card`, `worker_ground_shadow`, `worker_glow` (RND-308) and `orb_additive` (VFX-225), and the frame gains `pass_07d_workers` and `pass_11_essence_orbs`, after the coastline waves (RND-070).
  - Cut-out is by discard at M1. RND-060's alpha-to-coverage under multisampling waits for the anti-aliasing options, because coverage from the texel would double the blend of a fading card.
* **The Worker's look** (`GameClient/CrowdLook`), at entity detail 1 and the distance scale factor 2.0, the options' defaults:
  - **Size and detail.** The card's half-size follows R-UNIT-142 and RND-314. The variation step is (the unit id's counter × 7919) mod 10, standing in for the slot index.
  - **Foot and axes** (VFX-117). The foot lies on the terrain and the up axis is the terrain's normal, within the billboard distance of 1,300 wu. An airborne Worker is lifted and tumbles about its front on the scaled clock.
  - **The right axis is up × front.** That is RND-314's front × up restated for Species' right-handed world (ADR-016), so a card facing the camera meets the camera's right with the sign VFX-117 states.
  - **The layers of VFX-118**, in this order, a batch per layer and card texture:
    1. **Ground shadow.** The card laid flat 0.3 wu above the terrain, spanning its horizontal right axis and world +z. It is black at alpha 150 × the detail factor, while the Worker lives and within 2,000 wu.
    2. **Body.** For a living or an airborne Worker, in the colour of RND-317: a dead one at × 0.2 with alpha 2.5 × its fade. A corpse on the ground therefore shows its fragments alone.
    3. **Side strip.** For a card seen nearly edge-on. Its width scales with the card.
    4. **Corpse fragments.** Three half-size black cards drifting 120° apart and rising as the interpolated fade runs down.
    5. **Control glow.** For the local team's controlled or selected Worker: ten stacked cards in the control colour, pulsing on the unscaled clock, never below 0.3. It is tested against depth, as RND-308's entry states.
  - **Texture** (RND-316). The hurt pose 04 for a Burning Worker; variants 01 to 03, by unit id, for the share 0.3 of airborne corpses; the front card otherwise.
  - **The walk bob** (O75). A walking Worker's foot rises along its up axis by `vfx.worker.walk_bob_height` (0.4 wu) × its gait × |sin(`vfx.worker.walk_bob_rate` (6) × T + (unit id mod 10))|. The gait is its speed over the latest tick ÷ `vfx.worker.walk_bob_full_speed` (20 wu/s, a Worker's walk), at most 1. The values are provisional, for the owner to tune. The firing recoil waits for the laser bolts' shooter, in T16c.
* **The orbs** (R-UNIT-330, R-UNIT-331, O77). Two camera-facing quads of `fx.glow_round` in the team colour:
  - the inner quad at half-size 0.5 wu and alpha 127/255;
  - the outer quad at 1.5 wu and 50/255.

  Both grow in Birth by 1.2 − timer ÷ 60, and shrink and fade in Ascent by timer ÷ 900. Their positions are interpolated between the replica's two latest states, as figures are.
* **The interpolation** (`ReplicaTiming`). A figure also carries its interpolated health and its movement over the latest tick; orbs are interpolated as figures are.
* **The presentation clock and generator** (`NeuronClient/PresentationClock`, `FxRng`; ADR-036):
  - **The clock** keeps the real, unscaled and scaled readings of VFX-016, the step clamped to `ref.sim.frame.delta_clamp`. M1 holds no world and runs at game speed 1.
  - **`FxRng`** is SplitMix64 with R-SIM-607's draw forms, seeded from an event's identity by `FxSeed`. T16c's effects use it.
* **The watched sandbox** (O77, ADR-109 amendment). A Debug build's `--watch` gives the steward's side the sandbox's AI too, and the steward commands neither side. The world start records it, and the journal's format becomes 2.
* **The development dots** are drawn only while the overlay is shown (ADR-118 amendment).

### Consequences

* Good, because the run of T16 sees two AI sides fight in the reference's Worker look, and orbs rise from the fallen.
* Good, because the crowd's call count is fixed per layer and texture: at most 30 Worker batches and one orb batch a frame.
* Good, because the look's arithmetic is CPU code that CI tests without a GPU, and the render check draws its four entries on WARP.
* Bad, because the CPU writes every visible quad each frame. That is ADR-009's cost before its GPU cull, measured at T18.
* Bad, because the card variation is not the reference's: neighbours that the reference varies by slot may match here.
* Neutral, because a Debug run without `--watch` keeps the steward's side for T17's verbs.

### Confirmation

* NeuronClientTests: the camera-relative instance and its fields, the conservative view test, `FxRng`'s ranges and seeding, and the clock's clamp and hold (`SceneQuadsTests`).
* GameClientTests: the card's size against RND T3, a living Worker's shadow and body, a corpse's fragments on the ground and in the air, the control glow, the side strip, and the orbs' growth and fade (`CrowdLookTests`).
* GameLogicTests and NeuronCoreTests: the watched sandbox's two AIs, the journal's round trip, and the `--watch` option.
* The render check of ADR-117 on WARP.
* The owner's T16 run, with its checklist at T16c.

## Pros and Cons of the Options

### 1 — batches of scene quads, culled on the CPU

* Good, because it is ADR-009's interface to the passes with its cull on the CPU, and the GPU cull can replace the test without changing the game's side.
* Bad, because the per-frame CPU cost grows with the crowd.

### 2 — ADR-009 as written

* Good, because the CPU cost of a crowd is fixed.
* Bad, because it adds a compute pass, indirect arguments and a property test before M1 has measured the need.

### 3 — one draw per figure

* Good, because it is the simplest to write.
* Bad, because its call count grows with the population, which RND-455 forbids.

## More Information

* Related: [ADR-009](ADR-009-instanced-crowds-sprite-cards-and-shadows.md), [ADR-035](ADR-035-procedural-animation-runtime.md), [ADR-036](ADR-036-effect-events-and-presentation-clock.md), [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md), [ADR-116](ADR-116-presentation-tuning-record.md), [ADR-118](ADR-118-development-overlay-at-m1.md).
* Owner decisions O75 and O77; MVP-080, MVP-082; R-UNIT-142 to R-UNIT-147, R-UNIT-330, R-UNIT-331; RND-305 to RND-318; VFX-016, VFX-117, VFX-118, VFX-225.
* The code: `NeuronClient/SceneQuads`, `QuadRenderer`, `PresentationClock`, `FxRng`, the `UnlitTextured` shaders and `FrameRenderer`; `GameClient/CrowdLook`, `ReplicaTiming` and `WorldPresentation`; `GameLogic/Sandbox` and `CommandJournal`.
