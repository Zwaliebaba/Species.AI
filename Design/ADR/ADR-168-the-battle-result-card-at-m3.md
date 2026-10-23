---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T13)
---

# The battle result card at M3

## Context and Problem Statement

M3 task T13 shows MVP-048's battle results on the HUD: the result of every battle the steward's settlement is a side of, and the public line of every other battle, on the result card (`icon.hud.result_card`). T10 sends each result after its battle's end (`BattleResultNotice`, [ADR-165](ADR-165-the-valleys-status-notices-and-battle-results-at-m3.md)): whole for a side, the public line for every other steward.

Its done-when is the owner's run: a result agrees with the server's statistics.

These decisions are open:
- where the result appears, and how long it stays;
- what another battle's public line looks like, since the valley run counts about thirty battles a run;
- how the card is closed, since the HUD draws and the verbs take the presses (ADR-153).

## Decision Drivers

* A battle's result is the news the steward waits for after the warning and the fight: it must not age away unread.
* On O102's values the valley run of seeds 1 to 8 had 28 to 31 battles a run, 12 to 20 of them the steward's (`playtests/M3.md`), so 9 to 17 a run were between AI settlements.
* The HUD's layout and palette are provisional until the owner judges them (O95, DEC-1005).
* The verbs take the HUD's presses, and the HUD reads the verbs' state each frame (`HudVerbs`).

## Considered Options

For the steward's own results:
1. **A card at the top centre that stays until a press closes it**, the results waiting in the order they came.
2. **A notification card** among the others, gone after `mvp.hud.notice_seconds`.
3. **A window of the UI core** ([ADR-149](ADR-149-the-ui-core-pages-focus-and-layout-at-m2.md)).

For another battle's public line:
4. **A notification card** with one sentence: who beat whom, in a raid or for an area.
5. **The result card**, waiting for its press like the steward's own.

For the close:
6. **The verbs count the presses on the card**, and the HUD closes one result for each press it has not applied.
7. **The HUD takes the press** itself.

## Decision Outcome

Chosen options: 1, 4 and 6.
- Option 2 loses a result the steward did not look at in six seconds.
- Option 3 adds a window for a record the card holds in ten rows.
- Option 5 would have the steward close 9 to 17 cards a run for battles it took no part in.
- Option 7 gives the HUD a second press path beside the verbs' (UI-461).

* **The rows** (`ResultRowsOf`), each a row of the HUD:
  - the outcome for the steward's settlement, victory or defeat, and the battle: who raided whom, or who fought whom for which area;
  - its end (the objective held, the loss cap, a rout, a withdrawal or the time) and its length;
  - the Workers each side lost;
  - the stores the attacker carried off, and the Essence the defender took from the attackers who fell in its territory, each when not none;
  - the area that passed to the winner, when it moved;
  - the grievance the attack gave the defender, in whole units of weight, when not none;
  - the defender's respite in whole minutes from the battle's end, when one started;
  - the renown the battle gave its winner, when not none;
  - the close, with how many results wait after it.
* **The card** is `mvp.hud.result_width` wide (222 HUD units, the art's 400 px at a 1080-pixel client) at the top centre, a row high per row, drawn with `icon.hud.result_card` over the neutral panel strip, over the rest of the HUD. It is a zone of its own (`HudZone::Result`).
* **The queue.** The HUD takes the session's results each frame (`TakeBattleResults`). The settlement's own wait in the order they came, the first on the card. Each other battle's public line is a card among the notices (`ui.hud.result_public`), aged as they are. The queue is dropped while the client has no settlement.
* **The close.** A primary press on the card counts in the verbs (`HudVerbs::resultsClosed`), and the HUD closes the first result for each press it has not applied. The presses before the HUD's first update close none.
* **The strings** `ui.hud.result_title` to `ui.hud.result_public`, and the key `mvp.hud.result_width`, provisional with the HUD's layout (DEC-1005).

### Consequences

* Good, because the steward's own result waits until read, and the record's every line is on it.
* Good, because the AI settlements' battles reach the steward as one line each, as MVP-048 asks, without a press.
* Bad, because a run with many battles of the steward's own may queue results that the steward closes one by one.
* Bad, because a public line ages away like any notice, so a steward who looks away misses it; the M3 record does not list them.
* It forecloses: a result window; a result log of the run before the epilogue (T14).

### Confirmation

* `GameClientTests`:
  - `SettlementHudTests`: a won raid's rows give every line, a defeat's outcome is lost, and a lost claim battle that moved its area lists the area and nothing moved; the card is a zone at the top centre, `mvp.hud.result_width` wide and a row high per row, drawn with its frame, and absent with no result; the committed sentences of the card and the public line;
  - `ClientSessionTests`: the HUD takes the session's results, its own waiting in order and the public line among the cards; each press the verbs count closes the first, and the presses before the HUD closed none.
* The owner's run on `Debug|ARM64`, in a world with the AI settlements (the M3 plan's T13): after a battle of their own the card shows its result, which agrees with the server's statistics of the battle; another battle's line arrives as a card; a press closes the card.

## Pros and Cons of the Options

### 1 — a card that stays

* Good, because it waits until read.
* Bad, because it covers the top centre until closed.

### 2 — a notification card

* Good, because it needs no press.
* Bad, because it is gone in six seconds.

### 3 — a window

* Good, because it can hold any length.
* Bad, because it is a window for ten rows.

### 4 — a notice for a public line

* Good, because it asks nothing of the steward.
* Bad, because it ages away.

### 5 — the result card for a public line

* Good, because every result reads alike.
* Bad, because the steward closes cards for battles it was not in.

### 6 — the verbs count the presses

* Good, because the presses keep one path.
* Bad, because the HUD must not apply the presses before its first update.

### 7 — the HUD takes the press

* Good, because it is direct.
* Bad, because the HUD becomes a second taker of presses.

## More Information

* Amends:
  - [ADR-152](ADR-152-the-settlement-hud-at-m2.md): the result card and the public lines among the cards;
  - [ADR-153](ADR-153-the-settlement-verbs-at-m2.md): a press on the result card closes its result;
  - [ADR-165](ADR-165-the-valleys-status-notices-and-battle-results-at-m3.md): the HUD takes the session's results.
* Owner decisions O95, O101; DEC-1005; MVP-046, MVP-048, MVP-102; PIL-045, PIL-046; UI-461.
* The code:
  - `GameClient/SettlementHud` (`ResultRowsOf`, the card, the queue, the public lines), `SettlementVerbs` (`resultsClosed`);
  - `GameShared/TuningPresentation` (`mvp.hud.result_width`); `GameData/Strings/en_us.json` (`ui.hud.result_title` to `ui.hud.result_public`);
  - `GameClientTests/SettlementHudTests`, `ClientSessionTests`.
