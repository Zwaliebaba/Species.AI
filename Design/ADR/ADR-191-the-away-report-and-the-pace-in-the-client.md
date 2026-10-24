---
status: proposed
date: 2026-10-24
decision-makers: Species technology lead (M4 task T10; owner decisions O109, O110)
---

# The away report and the pace in the client

## Context and Problem Statement

M4's client part (the [M4 plan](../implementation/m4_plan.md) T10) has four pieces:
- the New world page chooses the pace (MVP-071);
- the client tells the server that its steward gives input (MVP-072);
- the world panel shows the pace;
- the away report opens first on a return whose absence has news, its summary on one page and the deputy's actions one step away (MVP-074, PIL-073, PIL-026, O110).

The server sends the report (ADR-186). The world's menu already shows the epilogue as a page of sections (ADR-170).

## Decision Drivers

* The summary reads in `pil.session.report_read_minutes` (2 minutes), and every deputy action shows its standing order (PIL-026).
* The report is server data; the client only lays it out (PIL-075, O6).
* The page family's rules hold (UI-372).
* Little new UI: the M4 HUD layout is provisional (DEC-1005).

## Considered Options

1. **The epilogue's page.** The away report is a page of the world's menu in the epilogue's form: a row per section in the left box, and the section's lines in the right box. It opens by itself on a return with news, and the menu has its entry.
2. **A HUD card.** The report is a card over the world, like the battle result (ADR-168).

## Decision Outcome

Chosen option: 1. A card holds a few lines; the summary and the actions are tens. The page already passes UI-372 and needs no new layout.

What is built:

* **The pace on the New world page.** A cycler after the neighbors: "While I play" (the session pace) or "Always on" (`FrontEndChoice::pace`). An always-on world is created with its pace in its description (ADR-190) and reached through its headless server; a session-paced world runs on the integrated server as before.
* **Presence reports.** Each frame the client tells its session whether the input router saw any device active (`InputRouter::IsIdle`). A frame with input sends `PresenceReportCommand` when none has gone for `PRESENCE_REPORT_SECONDS` (15 s), numbered as a command (ADR-184). The integrated server's session-paced world reads and ignores it.
* **The session.** `ClientSession::Desc::pace` gives the world's pace (`Pace()`). The session keeps the latest away report (`LatestAwayReport`) and notes a return's report with news (`TakeAwayReportArrived`).
* **The world's menu.** In an always-on world it asks once for the latest away report (`ReadAwayReportCommand`) and keeps it. Its "Away report" row opens it, inactive until a report of an absence has come. A return's report with news opens the report's page over the world; when the epilogue comes in the same frame, the epilogue's page opens first and the report's over it a frame later. The page has two sections:
  - the summary: the absence's length, the Workers and stores at either end, the structures built, each battle with its result, each area moved, the grievances recorded and each freeze with its cause, or that nothing happened;
  - the deputy's actions: each with its time into the absence, the other side and its standing order ("(defense)" or "(your attack)"), or that it took none.
  
  Each section has at most `AWAY_PAGE_LINES_MAX` (8) lines of at most 48 characters, the last counting those left out (`AwayReportTextOf`). Its strings are drafted in `en_us.json` (`ui.away.*`). `AreaName` joins `CladanName` in the HUD's header.
* **The world panel.** In an always-on world its second row says so, with the idle minutes after which the steward counts as away (`ui.hud.always_on`, `mvp.presence.idle_minutes`).
* **Not built.** The plan's `icon.hud.deputy` and a HUD button for the report: the menu's row and the report opening by itself reach it. Both wait for the owner's look at the provisional HUD (DEC-1005).

### Consequences

* Good, because the steward who comes back reads the absence before anything else, in the form the epilogue taught them.
* Bad, because the page shows at most seven deputy actions; a long absence's others are counted, not listed. The server keeps them all (ADR-185).
* Bad, because a report that comes while a menu page is open replaces that page.

### Confirmation

* `GameClientTests/WorldMenuTests`: the summary's lines, each action's time and order, the lines left out counted, a quiet absence, and the menu's away row waiting for a report.
* `GameClientTests/ClientSessionTests`: the menu asks once; a return's report with news opens once and one without news does not; the page passes UI-372 with two sections and no Leave. Presence reports go at most every 15 s and only on input.
* `GameClientTests/FrontEndTests`: the pace cycler after the neighbors, the session pace by default.
* T10's run checklist: the owner is away while an AI settlement attacks, comes back to the report, and reads its summary within 2 minutes.

## Pros and Cons of the Options

### 1 — the epilogue's page

* Good, because it reuses a page that already passes the page family's rules.
* Bad, because it covers the world while it is open.

### 2 — a HUD card

* Good, because the world stays in view.
* Bad, because a card is too small for the summary and the actions.

## More Information

* Owner decisions O109, O110; MVP-071, MVP-072, MVP-074, MVP-102, MVP-103, PIL-026, PIL-073; the [M4 plan](../implementation/m4_plan.md), task T10.
* Amends [ADR-170](ADR-170-the-world-menu-the-epilogue-page-and-leaving-at-m3.md) (the menu's away row and page), [ADR-184](ADR-184-presence-and-absences.md) (the client's reports) and [ADR-186](ADR-186-the-away-report.md) (the client's request and page).
* The code: `GameClient/ClientSession`, `GameClient/WorldMenu`, `GameClient/GameUi`, `GameClient/FrontEnd`, `GameClient/SettlementHud`, `Species/Species.cpp`, `GameData/Strings/en_us.json`.
* **Amendment 2026-10-24 (M4 task T10, O110).** The HUD button is built, since O110 puts the latest report one press away on the HUD, which the menu's row, two presses away, does not. The world panel's always-on row ends in a button with `icon.hud.deputy` (`UiImage::IconDeputy`, drawn from the content build's stand-in of ADR-177 until the owner draws it). It is on the accent plate once a report of an absence has come (`HudModel::awayReport`), and dimmed by `mvp.hud.disabled_alpha` before. A press opens the report's page (`HudZone::AwayReport`, `SettlementVerbResult::openAwayReport`, `SettlementView::TakeAwayReportRequest`), and a press before any report opens nothing. The world panel's height now counts the always-on row, which T10 had drawn past the plate's bottom. `GameClientTests/SettlementHudTests` and `SettlementVerbsTests` cover the button, its row and its press. The report's page still lists the deputy's actions as text lines, without the icon beside each that the asset's row describes.
