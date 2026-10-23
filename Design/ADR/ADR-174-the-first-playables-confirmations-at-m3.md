---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T16, owner decision O104)
---

# The first playable's confirmations at M3

## Context and Problem Statement

M3 task T16 confirms or amends each Proposed ADR whose "Confirmed when" names the first playable, the M3 build (O55; the [M3 plan](../implementation/m3_plan.md)'s table 1). Several confirmations ask for what the MVP does not have:
- golden-image tests of reference scenes, which no code of the tree renders or compares (ADR-002, ADR-007, ADR-013, ADR-014, ADR-015, ADR-034, ADR-078);
- the enhanced-barrier executor of ADR-007, which ADR-113 left for the milestone that needs it;
- a QA hardware matrix, a gamepad build, an installer and telemetry.

Others need a measurement or a check that the tree does not take yet:
- a crowd order's latency to its display (ADR-079);
- GPU-based validation over a whole session on real hardware (ADR-006);
- a code check that no barrier is recorded outside the frame graph's executor (ADR-004). The frame with no world records its two back-buffer transitions by hand.

The owner deferred the golden images and the enhanced executor and chose the frame bench for the latency (O104). Four questions remain: how each confirmation is met on the evidence in hand, what the owner's run adds, where the conformance table of ADR-064 lives, and what moves later.

## Decision Drivers

* Each ADR is confirmed or amended on its own "Confirmed when" (the M3 plan §2), not by T16's convenience.
* O104: no golden-image tests and no enhanced executor at M3.
* A check is built where it is small and catches a real slip. A check that needs the deferred infrastructure moves with it.
* The owner runs the build on the reference test machine (O100). What only real hardware shows is that run's item, not CI's.

## Considered Options

For the confirmations whose evidence O104 defers:
1. **Amend each ADR**: it is accepted on what M3 shows when nothing else of its confirmation applies to the MVP. The deferred part moves to the confirmation before the first external build.
2. **Keep every such ADR Proposed** until the deferred infrastructure exists.

For ADR-006's validation over a session:
3. **A Debug start-up option** turns GPU-based validation on for the session's own adapter, and the log counts the debug layer's messages when the session ends.
4. **The operating system's D3DConfig tool** forces validation for the owner's run.

## Decision Outcome

Chosen options: 1 and 3.
- Option 2 would hold records that M3 shows to work hostage to infrastructure that the MVP does not need.
- Option 4 changes a machine-wide setting by hand, and the run would leave no line in the game's log.

What is built and recorded:

* **ADR-079.** The frame bench times a crowd order to its display (`SpeciesFrameBench`, `SettlementView::Reports`).
  - While measuring, it sends an `OrderWorkersCommand` at a frame's start, at most once a second and one at a time, with no Worker selected, so nothing moves.
  - It stops the clock at the present of the first frame whose display time reaches the tick that applied the order. An order not displayed within 5 s is given up on.
  - `Build\RunPerformance.py` adds 2.5 median frame intervals for the input's wait, the queued frame and half a scan-out, as ADR-079's model counts them. It judges the judged scenes at 1,920 × 1,080 against 275 ms at the 95th percentile and fails a run with an order given up on.
* **ADR-015.** `FrameRenderer::SceneSamples` gives the scene's samples per pixel in use, which may fall back below the preferred 4 (RND-442). The frame bench records it with each scene, and the script names it beside the frame's GPU time (ADR-148). The `figures-max-drawn` scene is its battle of `mvp.client.figures_max` figures at 1080p.
* **ADR-002.** `Build\CheckProjectFiles.py` rule 7 holds every shader to Shader Model 6.6 in every build, with no `FXCompile` item setting another, proved by a fixture.
* **ADR-028.** `InputRouterTests` presses and releases Alt, F10 and each mouse button within one frame. Each shows both edges and is not held after it.
* **ADR-004.** Rule 14 allows one barrier call in the tree, in `Transition`, the frame graph's executor in `FrameRenderer.cpp`, proved by a fixture. The frame with no world sends its back buffer's two transitions through `Transition` too.
* **ADR-006.** `--gpu-validation`, a Debug start-up option of `Species.exe` (PLT-107), turns GPU-based validation on for the session's adapter. When the session ends, the log reports `GPU-based validation of the session: <frames> frames drawn, <n> debug-layer messages`, at error severity unless there are none.
* **ADR-064.** The per-guideline conformance table of XAG 3.2 is kept in the UI framework component's README (§7.1), as ADR-064 states. The text-size tests and the flash rules' tests are in hand.
  - The contrast test waits: the HUD's text sits on the owner's panel art, which a test cannot decode, and on the palette of DEC-1005, which is provisional.
  - The capture-based flash test moves with the golden images.
* **The confirmation of each ADR** is recorded in `playtests/M3.md` ("The first playable's confirmations"). Each ADR carries an amendment of this date.
  - Accepted now: ADR-002, ADR-004 and ADR-062.
  - Proposed until the owner's T16 run: ADR-006, ADR-015, ADR-078 and ADR-079's M3 part.
  - Proposed by their own triggers: ADR-007, ADR-013, ADR-014, ADR-028, ADR-034, ADR-064, ADR-077 and ADR-079's later parts.

### Consequences

* Good, because every row of table 1 has its evidence named, and what the owner's run decides is a list, not a search.
* Good, because two slips are now caught in CI: a barrier recorded beside the executor, and a shader compiled at another model.
* Bad, because nothing compares a picture at M3. A change of the colour pipeline or of multisampling is caught by the owner's eye until the golden images exist.
* Bad, because ADR-007's decision of two paths is still half built, and a GPU without Enhanced Barriers is the only kind tested.
* It forecloses nothing: the deferred checks keep their place in their records' confirmations.

### Confirmation

* CI on the T16 build: `NeuronCoreTests/StartupOptionsTests` (`--gpu-validation`), `NeuronClientTests/InputRouterTests` (ADR-028's pairs), `Build\CheckProjectFiles.py` with its 43 fixtures, the render checks, and `Build\RunPerformance.py --self-test` (late and given-up orders fail, and the sample count is recorded).
* The owner's T16 run on the reference test machine:
  - `Build\RunPerformance.py --record playtests/M3.md` records the order latency of each judged scene and the frame's GPU time at the recorded sample count in `figures-max-drawn`;
  - a Debug `Species.exe --gpu-validation` session creates a world with neighbors, fights at least one battle and saves, pressing Ctrl+Shift+F11 once in the battle. The log line reports no message, the picture comes back after the device's removal, and the session continues (ADR-006, ADR-008, ADR-078).

## Pros and Cons of the Options

### 1 — amend each ADR on what M3 shows

* Good, because what is confirmed is said, with its evidence, and what is not keeps its trigger.
* Bad, because an accepted record carries a check that is not yet built, as ADR-008 already does.

### 2 — keep them Proposed

* Good, because nothing is accepted without every check.
* Bad, because the MVP would end with records that its own build shows to work still Proposed.

### 3 — a Debug start-up option

* Good, because the run is the game's own and its result is a line of its log.
* Bad, because it is one more development option, which a Release build ignores (PLT-107).

### 4 — D3DConfig

* Good, because no code changes.
* Bad, because the result is read from a tool's output by hand, and the setting outlives the run.

## More Information

* Owner decisions O55, O98, O100, O104; the [M3 plan](../implementation/m3_plan.md) table 1; MVP-075, MVP-085.
* Amends [ADR-002](ADR-002-minimum-gpu-capability.md), [ADR-004](ADR-004-frame-graph-from-the-ordered-pass-table.md), [ADR-006](ADR-006-root-signature-and-bindless-resource-access.md), [ADR-007](ADR-007-barrier-model.md), [ADR-013](ADR-013-working-colour-space-and-blending.md), [ADR-014](ADR-014-hdr-output.md), [ADR-015](ADR-015-scene-anti-aliasing.md), [ADR-028](ADR-028-input-device-back-ends.md), [ADR-034](ADR-034-text-rendering-and-glyph-cache.md), [ADR-062](ADR-062-authoring-syntax-and-schema-language.md), [ADR-064](ADR-064-accessibility-baseline-xag.md), [ADR-077](ADR-077-client-http-stack.md), [ADR-078](ADR-078-gpu-resource-factory.md), [ADR-079](ADR-079-simulation-tick-rate.md) and [ADR-113](ADR-113-the-frame-at-m1.md).
* The code:
  - `Tools/SpeciesFrameBench`, `GameClient/SettlementView`, `NeuronClient/FrameRenderer`;
  - `NeuronCore/StartupOptions`, `Species/Species.cpp`;
  - `Build/CheckProjectFiles.py`, `Build/RunPerformance.py`;
  - `NeuronClientTests/InputRouterTests`, `NeuronCoreTests/StartupOptionsTests`.
