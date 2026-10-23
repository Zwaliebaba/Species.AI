---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (owner decision O106)
---

# The frame page under the settlement panel

## Context and Problem Statement

The development overlay's frame page (DBG-096, [ADR-120](ADR-120-frame-page-toggle-and-sandbox-restart-at-m1.md)) is laid out from the target's top-left corner, 10 pixels in. Its frame-rate line (DBG-095) is right-aligned at the bottom-right corner, 40 pixels up. Both places were chosen at M1, when the battle sandbox had no HUD.

The settlement HUD now holds every corner:
- the settlement panel at the top left ([ADR-152](ADR-152-the-settlement-hud-at-m2.md));
- the notification cards at the top right;
- the result card at the top centre ([ADR-168](ADR-168-the-battle-result-card-at-m3.md));
- the world panel at the bottom left;
- the construction bar at the bottom centre;
- the relations panel at the bottom right ([ADR-166](ADR-166-the-relations-panel-and-the-notifications-of-m3.md)).

The owner's run showed the frame page's rows and graph drawn across the settlement panel, and the frame-rate line across the relations panel's claims. The page's white text, with nothing behind it, could be read over neither panel.

## Decision Drivers

* The overlay is a development tool. It should not hide the HUD it measures, and its text should read over anything.
* Nothing the HUD draws should move for the overlay.
* The page's place should follow the HUD's layout, which moves with the text and UI scales ([ADR-175](ADR-175-the-huds-panels-as-framed-plates-sized-to-their-rows.md)).

## Considered Options

1. **Under the settlement panel.** The page stands below the settlement panel's plate, whatever its height, on a dark see-through backdrop, and the frame-rate line is its first row.
2. **Over the HUD, on a backdrop.** The page stays at the top-left corner and hides the settlement panel while it is shown.
3. **The HUD hidden while the overlay is shown.**

## Decision Outcome

Chosen option: 1 (O106).
- Option 2 hides the panel whose figures a developer is reading the page beside.
- Option 3 takes the HUD's own drawing out of the figures the page reports.

What is built:

* **The place.** `BuildFramePage` (`GameClient/DevelopmentOverlay`) takes the bottom of what the page stands under, in pixels at a DPI scale of 1. `WorldPresentation` passes the settlement panel's zone bottom divided by the DPI scale, or nothing when the HUD shows no settlement panel, as in the battle sandbox. The page's first row is that bottom plus the corner's 10 pixels, or 10 pixels from the top.
* **The frame-rate line.** In a world it is the page's first row, its text unchanged (`FrameRateText`). On the front end, which has no HUD, it keeps DBG-095's bottom-right anchor (`FrameRateLine`).
* **The backdrop.** The page's first rectangle is a black backdrop at 60 % opacity, 6 pixels past the page's content on every side. Its width is measured in the development role at `dbg.overlay.text_size` by the text system that draws the rows (`TextSystem::Layout`). The frame-time graph keeps its own backdrop over it.

### Consequences

* Good, because the page and the HUD can both be read, and the page follows the settlement panel as it grows.
* Bad, because in a short window the page can reach the world panel at the bottom left. At 1280 × 720 the band between the two panels is about 130 pixels, and the page with its graph is about 240.
* Bad, because the page lays out its rows once more each frame to measure them. The text system keeps its layouts, so this costs a look-up.
* It forecloses nothing: a page that the HUD's other zones push elsewhere takes the same parameter.

### Confirmation

* `GameClientTests/DevelopmentOverlayTests`:
  - the page's first row is the frame-rate line;
  - the rows and the graph lie on the backdrop;
  - a page under a panel's bottom of 300 pixels starts at 310.
* The owner's run: with F5, the page stands under the settlement panel, readable, and the relations panel is clear.

## Pros and Cons of the Options

### 1 — under the settlement panel

* Good, because nothing is hidden at 1080 lines and up.
* Bad, because it can still reach the world panel in a short window.

### 2 — over the HUD

* Good, because it never moves.
* Bad, because it hides the settlement panel.

### 3 — the HUD hidden

* Good, because nothing overlaps.
* Bad, because the measured frame is not the game's.

## More Information

* Owner decision O106; DBG-095, DBG-096, DBG-097, DBG-089.
* Amends [ADR-120](ADR-120-frame-page-toggle-and-sandbox-restart-at-m1.md) (the page's place and the frame-rate line's in a world).
* The code: `GameClient/DevelopmentOverlay`, `GameClient/WorldPresentation`, `GameClientTests/DevelopmentOverlayTests`.
