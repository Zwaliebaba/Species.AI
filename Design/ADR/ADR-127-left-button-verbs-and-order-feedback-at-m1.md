---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M1 task T17c), owner (O82)
---

# The left-button verbs and the order feedback at M1

## Context and Problem Statement

The owner's first run of the battle verbs (M1 task T17, [ADR-124](ADR-124-battle-verbs-at-m1.md), [ADR-125](ADR-125-battle-marks-route-preview-and-world-markers-at-m1.md)) found the crowd order working but the controls and their feedback wrong for the steward:
1. **The order is on the secondary button.** The reference gives every order to the secondary button (R-CTRL-042, R-CTRL-050), and a primary press on the ground with Workers selected releases them. The owner found the right-button order unnatural and asked for the primary button.
2. **The selection is hard to see.** The circle grows with the time it is held (R-CTRL-156), so a quick press and drag catches a few Workers. Their glow (R-UNIT-147) is drawn on their cards and hidden by the Workers in front, and the badge shows no count (ADR-125).
3. **An order leaves nothing behind.** The preview and the badge vanish as the order is sent, the order's flare is brief, and the ordered Workers walk out of the crowd's middle at 20 wu/s with their glow gone.

O82 settles the controls. The feedback follows the plan the owner approved with it.

## Decision Drivers

* The owner's controls, with every deviation from the reference named (MVP-101 as amended).
* One press, one meaning, one command (R-CTRL-048).
* The input layer never writes simulation state, and the presentation never touches the simulation's route store (R-CTRL-151, R-SIM-358).
* Every value in data (ADR-116); player-facing text in GameFont (MVP-100).
* No new catalogue entry, shader or asset where an existing one serves.

## Considered Options

1. **Resolve each button in the verbs, and draw the feedback with the existing passes.** The primary press orders; the secondary press clears, toggles or promotes; the circle, the discs, the ordered route and the count reuse the quad passes, the masks of ADR-125 and the overlay's text.
2. **Rebind the actions only.** Bind `action.order.crowd` and `action.order.special` to the primary button and keep both resolvers.
3. **Keep the reference's buttons** and add only the feedback.

## Decision Outcome

Chosen option: 1, because a press of the primary button already means a selection, so the order and the selection have to be resolved together. Option 2 sends two commands for one press (a `Select` and an order), which R-CTRL-048 forbids. Option 3 is what O82 rejected.

* **The bindings** (table 3.6-A rows 40 to 45 as O82 binds them): `action.order.crowd` and `action.order.special` on the primary button's press; `action.order.promote` on the secondary button's; the rest as ADR-124 binds them.
* **The primary press** resolves to one meaning, tried in this order:
  1. an own Leader under the pointer is selected, unless it is selected already (row S1, R-CTRL-062); it is never an order's target, so R-CTRL-057's Leader target is not built;
  2. with a Leader selected, its order to the pointer ground point (rows O6 and O7, R-CTRL-043);
  3. with Workers selected, the crowd order (row O1, R-CTRL-160);
  4. with nothing selected, one `Select` of nothing and the circle begins (rows S1 and S4, R-CTRL-048).

  An order is not sent off the region or while the route cannot be walked (ADR-125), and the press then does nothing else.
* **The secondary press** resolves to one meaning, tried in this order: the toggle of an own Leader under the pointer while no Workers are selected and nothing or that Leader is selected (row O3); with anything else selected, a `Select` of nothing, row S3's effect; with nothing selected, the promotion (row O2). Space still clears and C still demotes.
* **The circle** (R-CTRL-156 as O82 amends it):
  - **Its radius** is the larger of its growth on the real clock and the farthest the pointer ground point has been dragged from its centre, up to `ref.ctrl.select.circle_max_radius`. It never shrinks, since the selection it sends is additive.
  - **Its release** resets the crowd-order time only when it ends a circle (R-CTRL-157), since the primary press now also orders.
  - **Its fade.** The released circle stays shown and its flashes shrink with its strength, which falls from 1 to 0 over `mvp.selection.circle_fade_seconds` (0.4 s, provisional) of real time.
* **The selection discs.** A `marker.team_disc` lies on the terrain under each of the local team's living Workers under control, in `vfx.worker.control_glow_color` at `mvp.selection.disc_alpha` (0.35, provisional), of `mvp.selection.disc_size_factor` (0.8, provisional) × the Worker card's half-size (R-UNIT-142). It is drawn in pass 7d before the Worker layers with `effect_additive_nodepth`, so the cards drawn after it cover it and no other Worker's card hides it.
* **The badge's count** (R-CTRL-366):
  - **The text.** The local team's living Workers under control in the frame's figures, in white GameFont at the overlay's text scale, while the badge shows Workers.
  - **Its place.** Its cell's left edge at the right edge of the badge's shadow, its middle level with the badge's centre. `OverlayCorner::PixelLeftMiddle` anchors a line in physical pixels that do not scale with the text.
  - **Its drawing.** A second `OverlayRenderer`, holding GameFont's page (`font.ui.primary`), draws it in pass 20 between the badge and the cursor with the text entry of ADR-118. M2 task T14's UI pass replaces it.
* **The ordered route** (RND-351, RND-352; amends O78's first answer):
  - **Kept.** When a crowd order or an order to the selected Leader is sent, the preview, while shown and walkable, is kept as an ordered route with the fade value 1. Its points are the client's own preview, so no route travels on the wire.
  - **Fading.** Its fade value falls by `rnd.route.fade_rate` (0.25, the reference's) a second of real time, and the route is dropped at 0. Its spots, arrows and mark are drawn at their alpha × the fade value.
  - **Dropped** with every route kept when a new world is entered or the client commands no team.
* **The destination mark.** The preview and each ordered route end in `marker.order_trail` on the terrain at their last point, turned into its slope, in the colour of their last spot, of the trail marks' size (R-UNIT-186), with `route_arrow` in pass 12.
* **The arrows' place** (DEC-959 item 4). An arrow lies at the point of its leg whose projection is the middle of the leg's projection: the fraction w₀ ÷ (w₀ + w₁) of the leg, from its ends' depths ahead of the camera; the leg's middle while an end lies behind it.
* **The keys** in the presentation record: `mvp.selection.disc_size_factor`, `disc_alpha` and `circle_fade_seconds`, and `rnd.route.fade_rate`.
* **The lights** are retuned in the same task, so that the ground sits darker and calmer than the crowds: a softer warm key from higher and a dim grey fill from the opposite side, in WLD-183's pattern ([ADR-115](ADR-115-the-world-frame-at-m1.md) amendment).
* **The render check** (ADR-117) orders its Leader with the primary press, so the ordered route, the discs and the count reach the WARP adapter.

### Consequences

* Good, because a selection and its order are both on the primary button, and the right button is never needed to move.
* Good, because the steward sees how many Workers the circle caught, where each one stands and where the order sent them.
* Good, because nothing new reaches the simulation or the wire, and no catalogue entry, shader or asset is added.
* Bad, because a stray primary press with something selected is an order, where the reference made it a release.
* Bad, because a Leader cannot be ordered to another Leader's position; the steward orders it to the ground beside it.
* Bad, because the discs and the destination mark have no depth test, so they show through a hill in front, as the trail marks do (ADR-125).
* Neutral, because the provisional keys wait for the owner's T17c run.

### Confirmation

* GameClientTests:
  - `BattleVerbsTests`: the primary press's orders with Shift and Ctrl, its selection of another Leader, the secondary press's toggle, clear and promotion, the drag-sized circle and its fade, the crowd-order interval kept across an order's release, the orders held back while the route cannot be walked, and the prediction;
  - `RoutePreviewTests`: the ordered route kept and fading over 4 s, never kept into the sea and dropped by a reset, the destination mark, and the arrows at the screen's middle of their legs;
  - `CrowdLookTests`: the selection disc;
  - `WorldMarkersTests`: the badge's count and its place.
* CI's render check with the scripted verbs (`Build\CheckWorldFrame.py`).
* The owner's T17c run.

## Pros and Cons of the Options

### 1 — one resolver a button, the feedback with the existing passes

* Good, because each press keeps one meaning and one command.
* Bad, because the verbs no longer follow the reference's buttons, which the tests and the design now state as Species' own.

### 2 — rebind the actions only

* Good, because it is a change of data alone.
* Bad, because one primary press would both select and order.

### 3 — keep the reference's buttons

* Good, because the reference holds as written.
* Bad, because the owner rejected it after playing it.

## More Information

* Related: [ADR-116](ADR-116-presentation-tuning-record.md), [ADR-117](ADR-117-render-check-on-warp.md), [ADR-118](ADR-118-development-overlay-at-m1.md), [ADR-121](ADR-121-workers-and-orbs-drawn-at-m1.md), [ADR-124](ADR-124-battle-verbs-at-m1.md), [ADR-125](ADR-125-battle-marks-route-preview-and-world-markers-at-m1.md).
* Owner decisions O78 and O82; MVP-100, MVP-101; R-CTRL-041 to R-CTRL-043, R-CTRL-048, R-CTRL-050, R-CTRL-057, R-CTRL-062, R-CTRL-155 to R-CTRL-157, R-CTRL-160, R-CTRL-366; R-UNIT-142, R-UNIT-147, R-UNIT-186; RND-351 to RND-353; DEC-959.
* The code: `GameClient/BattleVerbs`, `InputBindings`, `SettlementView`, `RoutePreview`, `CrowdLook`, `WorldMarkers`, `EffectsLook` and `WorldPresentation`; `NeuronClient/OverlayRenderer`, `FrameRenderer` and `WorldScene.h`.
