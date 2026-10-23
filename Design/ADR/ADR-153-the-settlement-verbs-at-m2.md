---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T17)
---

# The settlement verbs at M2

## Context and Problem Statement

M2's T17 builds the client's side of T8's commands ([ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md)):
- placing a structure, with its preview and its cost shown, the placement cursors and the reason a place is refused;
- setting the three priorities;
- the pointer over the HUD.

The M2 plan states three constraints. Placing is a mode that takes the primary press before the battle verbs, which give that press an order or the selection circle (O82, [ADR-127](ADR-127-left-button-verbs-and-order-feedback-at-m1.md)). The secondary press leaves the mode, as it releases a selection. A press over the HUD reaches no verb. Its run checklist asks that a placement never send an order, and that an order never place.

The server decides every placement: it checks the structure's footprint against the territory, the centre ring, the slope, the other structures and the stores (MVP-032, [ADR-132](ADR-132-placement-and-construction-at-m2.md)). The client holds the replica's stores, the territory's cells and its Wellspring, and the latest status's queue ([ADR-152](ADR-152-the-settlement-hud-at-m2.md)). It does not hold the structures' simulation records, so it cannot repeat the footprint checks.

The reference draws a structure placed by a Power as a ghost model at the pointer ground point, blended as source plus destination times source colour, with no depth writes (R-CTRL-366 row 1b). RND-365 draws that ghost in pass 20's world-space part, with an in-shader depth test against `scene_depth_single`. Neither pass 20's world-space part nor `scene_depth_single` exists yet.

## Decision Drivers

* Every refusal shown is the server's, or a foreseen one that the server would also give.
* A placement and an order never share a press.
* The ghost stands where, and as, the server will place the structure.
* No renderer work beyond what the ghost needs now.

## Considered Options

1. **The settlement verbs beside the battle verbs.**
   - **Presses.** They take a press over the HUD and the presses of placing, before the battle verbs.
   - **Predictions.** What the client foresees drives the cursor, the ghost and a line in the HUD. Every placement press is sent, and the reason shown after it is the server's.
2. **As option 1, but a foreseen refusal holds the press back**, as a route with no walkable way holds an order back (ADR-125).
3. **The placement mode inside the battle verbs.**

## Decision Outcome

Chosen option: 1.
- **Option 2.** The client's checks read the territory's cell under the pointer and the latest status, which is up to a second old. The server reads the footprint and the stores of its own tick. A held-back press would refuse places that the server would take, at the territory's edge and as the stores fill. The route preview's walkability is the server's own rule on the same navigation data; these predictions are not.
- **Option 3.** It would tangle the selection's prediction (ADR-124) with a mode that selects nothing.

* **The verbs** (`GameClient/SettlementVerbs`):
  - **HUD presses.** A primary press over the HUD's zones of the last frame (UI-461, [ADR-152](ADR-152-the-settlement-hud-at-m2.md)) reaches no battle verb. On a construction bar entry it enters placing that structure, or leaves the mode when that structure is the one being placed. On a priority's arrow it sends `SetPrioritiesCommand`.
  - **Placing** takes the primary and the secondary press before the battle verbs.
    - The primary press on the region sends `PlaceStructureCommand` at the pointer ground point and stays in the mode.
    - The secondary press, on the region or the HUD, leaves the mode.
  - **The secondary press** reaches the battle verbs only when neither the HUD nor placing takes it.
  - **No settlement.** With none of the client's, as in the sandbox, there is no mode and nothing is sent.
* **The prediction** (`PredictPlacement`) follows MVP-032's order and checks:
  - the latest status's queue against `mvp.construction.queue_max` (QueueFull);
  - the territory's cell under the point (OutsideTerritory);
  - the point inside the centre ring about the Wellspring (InCenterRing). The server measures from the footprint's edge, so a point inside the ring is refused whatever the footprint;
  - the replica's stores against `StructureCostMilliRu` (CannotAfford).

  It never decides: the press is sent whatever it foresees.
* **The refusal.** A placement's outcome that is not applied carries its reason in `CommandReport::reason` ([ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md)). A reason outside `PlacementRefusal` shows as not buildable. The refusal is shown for `mvp.hud.notice_seconds`, fading as a card does, and a placement applied clears it.
* **The priorities** (MVP-036):
  - **The step.** An arrow moves its share by `mvp.hud.priority_step` (10 %) with `ShiftPriority`. A raise takes half the step from each other task, the larger giving the odd point; a lower gives half to each, the smaller taking the odd point. No share passes 0 or 100, and the shares always sum to 100.
  - **What a press moves from.** A press moves from the shares last sent, else the status's. The HUD shows the shares sent until a status of the change's tick or later holds them. A change the server does not apply is dropped.
* **The HUD** ([ADR-152](ADR-152-the-settlement-hud-at-m2.md) amended):
  - **Arrows.** Each priority row has a lowering and a raising arrow at its right end, each a zone. `ZoneAt` returns the last zone laid out that holds the point, so an arrow before its panel.
  - **Arrow art.** The arrows are `cursor.pointer`'s art, pointing down and flipped for up, in a neutral strip (UI-076). An arrow that cannot move its share is drawn at `mvp.hud.disabled_alpha`.
  - **The bar.** The entry being placed is drawn in the accent strip.
  - **The line above the queue** (`ui.hud.placing`, `ui.hud.refused`) shows, in this order: the server's refusal; while placing, the refusal the client foresees; or the structure being placed.
* **The cursors** (R-CTRL-350):
  - **Placing.** While placing, the cursor is `cursor.place` with its shadow. Its size is `mvp.cursor.place_size` (48 px, as the owner's other cursors), centred on `ref.ctrl.cursor.set.place_hotspot`, white. It is `cursor.unavailable` off the region and while the client foresees a refusal.
  - **Over the HUD**, the cursor is `cursor.world`, the standard pointer of UI-465.
  - **`cursor.pointer`.** The M2 plan's T17 row names it for the pointer over the HUD. UI-465 and the asset's manifest row make it the arrow art of HUD lists and scroll bars, never a pointer, so it draws the priorities' arrows.
* **The ghost** (R-CTRL-366 row 1b, RND-365):
  - **The model.** While placing with the pointer on the region, the structure's mesh stands at the pointer ground point, a turret's head as the MVP assets make it (`mesh.structure.turret_head`, "also the placement preview").
  - **Its facing.** It faces away from the Wellspring, as the server places it (ADR-132), not unrotated as row 1b has it for a Power's turret, which no settlement places. It is tinted in its team's colour, at `mvp.structure.ghost_brightness`.
  - **The draw.** It is drawn in pass 12, before the route preview, with the new catalogue entry `cursor_ghost`: the lit mesh shaders with no fog, and the new blend state `Ghost` (one, source colour; RND-058). It is depth-tested by the hardware and writes no depth. Pass 20's world-space part, with its in-shader test against `scene_depth_single` (RND-309), waits for a second user.
  - **The preview.** No route is previewed while placing or with the pointer over the HUD, where no press orders.
* **What waits:**
  - rebuilding a destroyed turret's site from the HUD (`RebuildTurretCommand`), as no settlement fights another before M3;
  - key bindings for the bar's entries;
  - the placement sounds;
  - R-CTRL-371's animated placement cursor.

### Consequences

* Good, because a placement and an order never share a press, and the HUD never passes a press to the world.
* Good, because every reason shown after a press is the server's, with the client's foresight shown before it.
* Good, because the ghost shows the structure where and as it will stand.
* Bad, because a press on a place the client foresees refused still costs a command that the server refuses and journals.
* Bad, because the ghost is drawn in pass 12 under the markers, not in pass 20, and is hidden by terrain in front of it as the scene is.
* It forecloses: a client that refuses a placement for the server; a ghost that writes depth.

### Confirmation

* GameClientTests:
  - `SettlementVerbsTests`: every shift sums to 100 and stays within 0 to 100; each foreseen refusal in MVP-032's order and the dwelling's escalation; the mode entered and left from the bar, kept after a placement and left by the secondary press, with the presses taken and the ghost's facing; a foreseen refusal still sent; the server's reason shown for its time and cleared by a placement applied; the arrows' shares sent, moved from those sent and shown until a status of their tick.
  - `SettlementHudTests`: the arrows are zones found before their panel, drawn six times; the entry being placed in the accent strip; the line above the queue; the shares sent fill the meters.
  - `StructureLookTests`: the ghost's batch in pass 12 with `cursor_ghost`, its yaw, colour and brightness, and a turret's head.
  - `SettlementCameraTests`: the placement cursor's size, hotspot and colour.
* The owner's T17 run, in a `--world` session:
  - places each structure from the bar and sees each refusal's reason;
  - moves the priorities with the arrows, with the result visible in the idle reasons and the task counts;
  - checks that a placement never sends an order, that an order never places, and that a press over the HUD reaches no verb.

## Pros and Cons of the Options

### 1 — the settlement verbs beside the battle verbs, sending every press

* Good, because the server's reason is always the one shown after a press.
* Bad, because a press the client foresees refused still costs a command.

### 2 — a foreseen refusal holds the press back

* Good, because no command is spent on a place the client foresees refused.
* Bad, because the client's checks are not the server's, and would refuse places the server takes.

### 3 — the mode inside the battle verbs

* Good, because one class reads the presses.
* Bad, because it tangles the selection's prediction with a mode that selects nothing.

## More Information

* Amends [ADR-152](ADR-152-the-settlement-hud-at-m2.md) (the HUD's arrows, the entry being placed, the line above the queue) and [ADR-005](ADR-005-pipeline-state-catalogue.md)'s catalogue (the `cursor_ghost` entry of RND-365, in pass 12 at M2).
* MVP-032, MVP-034, MVP-036, MVP-101, MVP-102; O82; R-CTRL-350, R-CTRL-366; RND-058, RND-365; UI-076, UI-461, UI-465.
* The code: `GameClient/SettlementVerbs`, `SettlementView` (the presses, the cursor), `SettlementHud` (the arrows, the line), `StructureLook` (`AppendGhost`), `UiLook` (`cursor.pointer`), `WorldPresentation`; `NeuronClient/PipelineCatalog` (`BlendState::Ghost`), `FrameRenderer` (`cursor_ghost`, pass 12's meshes).
