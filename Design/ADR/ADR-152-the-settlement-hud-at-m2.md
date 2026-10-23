---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T16, owner decision O95)
---

# The settlement HUD at M2

## Context and Problem Statement

M2's T16 builds the settlement part of MVP-102's HUD: the stores and their capacity, the Workers and the population limit, growth and the cause when it stops, the priorities, the construction bar and queue, notifications of completed structures, the settlement panel with each idle Worker's reason (MVP-037), and the world's information panel with its seed (MVP-005). Its run checklist asks that every figure agree with the server's statistics and the development overlay.

The server already has what the HUD needs:
- **The replica.** It carries the commander's stores, capacity, population and limit every tick ([ADR-139](ADR-139-replicating-the-settlement-at-m2.md)).
- **The status.** The server answers a `ReadSettlementStatusCommand` with a `SettlementStatus` of the next tick: growth, priorities, tasks, idle Workers by reason, upkeep, inactive structures and the queue ([ADR-136](ADR-136-settlement-commands-status-and-statistics-at-m2.md)).
- **The notices.** A completed structure's `SettlementNotice` arrives on the reliable channel ([ADR-139](ADR-139-replicating-the-settlement-at-m2.md)).

No layout or palette exists for the HUD. The owner approves its layout, and O95 makes T16's palette provisional, to be retuned after its run.

## Decision Drivers

* UI-460's recipe D: a HUD panel is an overlay with no focus, drawn in the UI's pass, advanced in the update phase, reading only presentation state and issuing intent only as commands.
* UI-461: its zones come from its advance, never from drawing, so the pointer's hit test (T17) reads the last frame's.
* Every figure agrees with the server: a cost the HUD shows is the cost the server charges.
* Every text from the string table, and every measure and colour from data.

## Considered Options

1. **A HUD class of the game over the UI's batches**, with its model from the replica and the status, and its layout in HUD units.
2. **HUD pages of the UI core** ([ADR-149](ADR-149-the-ui-core-pages-focus-and-layout-at-m2.md)): windows with widgets.

## Decision Outcome

Chosen option: 1.
- Option 2 makes the HUD a window, and a window takes the focus and every world action (UI-006, ADR-149). UI-460 says a HUD panel is not a window.

* **The model** (`HudModelOf`, `GameClient/SettlementHud`):
  - **The figures.** The stores, capacity, population and limit come from the client's own settlement in the replica's latest tick; the rest come from the latest status of its team.
  - **The bar's costs.** They come from `StructureCostMilliRu` (`GameShared/StructureCost`), the function the server's `CostMilliRu` now calls, with the dwellings of the team that the replica holds, built or queued.
  - **The notices.** Each is kept `mvp.hud.notice_seconds` of real time, at most `mvp.hud.notices_max` of them, the newest kept (`AgeNotices`).
  - **No settlement.** With no settlement of the client's, as in the sandbox, the model is empty and nothing is drawn.
* **The status** is asked for every `mvp.hud.status_period` (1 s) of real time while the client has a settlement, through the session's reliable channel. The session keeps the latest `SettlementStatus` (`ClientSession::LatestStatus`) and the world's seed (`WorldSeed`). The server's outcome reports of the requests are ignored by the verbs, whose predictions do not name them.
* **The layout** (`SettlementHud::Layout`):
  - **Units.** HUD units of RND-377's 600-unit-high space, times the UI scale (UI-168), whole pixels.
  - **Panels.** The settlement panel top left, its height from its rows. The construction bar bottom centre, with the queue's squares above it. The notification cards top right. The world's panel bottom left.
  - **Zones.** Each panel is a zone (`HudZone`), and each bar entry is one with its index. `ZoneAt` finds the zone under a position, for T17's pointer over the HUD.
* **The drawing** (`SettlementHud::Draw`), in pass 19 under the UI's windows:
  - **Panels.** The accent strip for the settlement and world panels, the neutral strip for the bar and the queue, each tinted by `mvp.hud.palette.*` (UI-460).
  - **Icons.** `icon.hud.resource`, `icon.hud.growth`, `icon.hud.priority`, `icon.hud.dwelling` and `icon.hud.storehouse`; `icon.power.workers` for the Workers, and `icon.power.gun_turret` for the turret, which has no HUD icon.
  - **Meters** of `icon.hud.meter`: a track and a fill.
  - **Cards** of `icon.hud.notification_card`, fading over `mvp.hud.notice_fade`.
  - **Text** in the body role at `mvp.hud.text_size` HUD units, times the text scale and never below UI-171's minimum.
  - **Colors.** Growth is green while it grows and red when it stops; a cost is red while the stores cannot pay it; the idle reasons are red.
  - **The halo.** `icon.hud.icon_halo`, a BC4 darkening mask, is not drawn: the UI's mask images draw white, and a darkening blend of a mask waits for the Power slots panel, which UI-462 gives it.
* **The strings** (`GameData/Strings/en_us.json`) are drafted for the owner's review:
  - **Ids.** `ui.hud.stores`, `workers`, `growth`, `upkeep`, `inactive`, `priority`, `idle`, `idle_reason`, `waiting`, `build`, `cost`, `completed` and `seed`.
  - **Forms.** They use the message subset's plurals and selects (ADR-146): growth's state, a task, an idle reason and a structure kind are select arguments.
  - **The seed** is inserted as its digits, ungrouped, so that it can be typed again (MVP-005).
* **The keys.** The presentation record gains `mvp.hud.*`, provisional (O95, DEC-1005): the status period, the notices' life, fade and count, the measures, and the palette for RND-403's roles.
* **What waits:**
  - the HUD's verbs: placing from the bar and setting the priorities, which are T17's;
  - the relations panel, the battle result, renown, the run clock, the away report and the epilogue, which are M3's and M4's;
  - the panel generator's own art, which replaces the stand-ins when the owner makes it.

### Consequences

* Good, because the HUD shows the server's own figures: the replica's every tick, the status's once a second, and the costs the server charges.
* Good, because its layout and look are data, which the owner retunes after T16's run without a rebuild.
* Bad, because the growth countdown and the priorities' Workers update once a second, at the status's period, not every tick.
* Bad, because the layout is code arranged by keys, not a layout record; a panel moved to another corner is a code change.
* It forecloses: a HUD that takes the focus; figures computed by the client that the server does not compute.

### Confirmation

* GameClientTests: `SettlementHudTests`:
  - every panel is a zone;
  - the settlement panel's rows follow the idle reasons with Workers;
  - the pointer finds a bar entry;
  - no settlement, no HUD;
  - the meters, cards, icons and text are drawn;
  - notices are aged and capped;
  - the bar's cost is the server's.
* GameLogicTests: the construction and steward tests pass on the shared cost, which changes no cost and no state hash.
* The owner's T16 run, in a `--world` session, beside the development overlay (F5):
  - the stores, capacity, Workers and limit match the overlay and the title;
  - growth reads "Next Worker in" with its seconds while it grows and its cause when it stops;
  - placing a structure (T17) shows its cost, its site in the queue with its progress, and its card when it completes;
  - the idle reasons appear as Workers idle.

## Pros and Cons of the Options

### 1 — a HUD class over the UI's batches

* Good, because it holds no focus and draws in the UI's pass with the UI's renderer.
* Bad, because it repeats some drawing that the UI core's widgets do (strips, text rows).

### 2 — HUD pages of the UI core

* Good, because the widgets exist.
* Bad, because a window takes the focus and every world action.

## More Information

* **Amendment 2026-10-22 (M2 task T17, [ADR-153](ADR-153-the-settlement-verbs-at-m2.md)).** The HUD's verbs are built:
  - each priority row has a lowering and a raising arrow, each a zone, found by `ZoneAt` before its panel;
  - the entry being placed is drawn in the accent strip;
  - a line above the queue shows the server's refusal, the refusal the client foresees while placing, or the structure being placed;
  - the priorities show the shares sent until a status holds them.
* Owner decision O95; DEC-1005; MVP-005, MVP-034, MVP-037, MVP-098, MVP-101, MVP-102; UI-168, UI-171, UI-460, UI-461; RND-377, RND-403.
* The code: `GameClient/SettlementHud`, `ClientSession` (`LatestStatus`, `WorldSeed`), `UiLook` (the HUD's images), `WorldPresentation` (`UpdateHud`, pass 19); `GameShared/StructureCost`; `GameLogic/Construction`.
* **Amendment 2026-10-23 (M3 task T11, [ADR-166](ADR-166-the-relations-panel-and-the-notifications-of-m3.md)).** The HUD asks for the valley's status with the settlement's, shows the relations panel at the bottom right, and draws a card for every kind of notice, each with its icon; `mvp.hud.relations_rows_max` bounds the panel.
* **Amendment 2026-10-23 (M3 task T13, [ADR-168](ADR-168-the-battle-result-card-at-m3.md)).** The result card of the settlement's battles stands at the top centre until a press closes it, the results waiting in order; another battle's public line is a card among the notices.
* **Amendment 2026-10-23 (M3 task T14, [ADR-169](ADR-169-the-epilogue-the-standing-and-the-time-left-at-m3.md)).** The world panel shows, under the seed, the time left in the run and the renown standing with each settlement's sources, from the valley status.
