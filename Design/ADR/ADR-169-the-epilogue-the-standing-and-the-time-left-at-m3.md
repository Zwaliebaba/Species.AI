---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T14)
---

# The epilogue, the standing and the time left at M3

## Context and Problem Statement

M3 task T14 shows the run on the client: the renown standing by source (MVP-062), the time left in the run (MVP-060), and the epilogue when the run ends (MVP-063). The epilogue holds the final standing, each settlement's history (battles, claims, structures, its strength over the run) and the steward's statistics. [ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md) keeps the epilogue's data as the region's own state, and copies nothing for it. T14's first step puts that data and the run on the client; its second step is the epilogue's page, leaving to the front end and the front end's epilogue entries.

These decisions are open:
- whether the standing and the time left need a message of their own;
- how the epilogue reaches the client, and how much of the statistics it carries;
- what "the steward's statistics" are at the MVP.

## Decision Drivers

* The valley status (ADR-165) already carries the run's end, its quiet minutes and whether it has ended, and every settlement's renown by source, once a second.
* MVP-064's statistics hold a sample every `mvp.stats.sample_minutes` (5 minutes) and every event. An 8-hour run has 96 samples a settlement and hundreds of events.
* A reliable message is at most `MESSAGE_MAX_BYTES` (64 KiB), and the client decodes nothing of the world's state (ADR-155).
* The M2 plan's growth measures are the growth steps and the largest gap between two (`playtests/M2.md`), and contact is the first halfway line (ADR-163).

## Considered Options

For the standing and the time left:
1. **The valley status**, read once a second, the time left counted down from the replica's tick between statuses.
2. **A run message** of their own, sent at each change.

For the epilogue:
3. **A summary the server builds** on request, as it builds the valley status (`ReadEpilogueCommand`, `Epilogue`): each settlement's counts and at most 48 strength samples.
4. **The statistics whole**, sent on request.
5. **A file** the server writes into the world's folder at the run's end.

## Decision Outcome

Chosen options: 1 and 3.
- Option 2 sends again what the valley status carries.
- Option 4 sends hundreds of entries the page summarizes anyway, and grows with the run.
- Option 5 copies the region's state, which ADR-163 decided against, and the snapshot already holds it.

* **The standing** (`StandingOf`) orders the valley status's renown by its total, the lower site first on a tie, as the server's `Standing` does. The world panel, under the seed, lists it with each settlement's swatch, place, total and the four sources, the steward's own in the positive colour.
* **The time left** (`RunTimeOf`) counts from the run's end tick down to the replica's tick, rounded up to whole seconds: running, then in the quiet minutes from their first tick, in the negative colour, then ended. The world panel shows it above the standing (`ui.hud.time_left`, `ui.hud.standing`, `ui.hud.renown`).
* **The epilogue** (`EpilogueOf`, MVP-063). A steward who commands a settlement asks with `ReadEpilogueCommand`, read outside the queue as a status request is. It is answered after the next tick, after the valley status, also while the run has ended (ADR-163's read-only world answers status requests). It holds:
  - the tick, the team, the run's end tick and whether it has ended;
  - every settlement's history in the standing's order: its renown by source; the battles it won and lost on either side, from each `BattleEnded` entry's result; its claims made, won, lost and withdrawn; its structures completed; the new grievances its Cladan took; its most Workers and its Workers at the last record; and at most `EPILOGUE_SAMPLES_MAX` (48) of its strength samples, evenly taken, the first and the last among them;
  - the steward's statistics: its settlement's growth steps (the M2 plan's three kinds), the largest gap between two in ticks, and the tick its territory first reached a halfway line, 0 for never.

  At its bound it is 1,805 bytes.
* **The client** keeps the latest epilogue (`ClientSession::LatestEpilogue`).

### Consequences

* Good, because the standing and the time left need no new message, and the time left counts down every frame.
* Good, because the epilogue is built from the run's own records, on request, at a bound far under a message's.
* Bad, because the strength over the run is at most 48 points, a sample every 10 minutes of an 8-hour run.
* Bad, because the standing lags the renown by up to a second, at the valley status's pace.
* It forecloses: a run message; the statistics on the client; an epilogue file.

### Confirmation

* `GameSharedTests`: the request and the epilogue round-trip at their bounds; a strength below zero does not decode.
* `GameLogicTests` (`RunTests`): the epilogue of a region's records lists every settlement in the standing's order, with its claims and their ends, the battle won by one side and lost by the other, the grievance taken, its first halfway line and its Workers and strength; a world whose run has ended answers the request with the ended run's epilogue.
* `GameClientTests`: the standing's order and totals; the time left running, in the quiet minutes, rounded up, and ended at its tick or by the server; the world panel's rows; the session keeps the latest epilogue.
* The owner's run waits for the second step's page: the standing and the time left show from the first valley status.

## Pros and Cons of the Options

### 1 — the valley status

* Good, because it is there once a second.
* Bad, because it lags a change by up to a second.

### 2 — a run message

* Good, because a change arrives at once.
* Bad, because it duplicates the valley status.

### 3 — a summary on request

* Good, because it is bounded and built from the records.
* Bad, because the page cannot show what the summary leaves out.

### 4 — the statistics whole

* Good, because nothing is left out.
* Bad, because it grows with the run and the client decodes the server's records.

### 5 — a file

* Good, because the front end reads it without a server.
* Bad, because it copies the state.

## More Information

* Amends [ADR-152](ADR-152-the-settlement-hud-at-m2.md) (the world panel's time left and standing) and [ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md) (the epilogue's summary on request).
* Owner decisions O80, O101; MVP-060 to MVP-064; PIL-125.
* The code:
  - `GameShared/RelationMessages` (`ReadEpilogueCommand`, `EpilogueLine`, `Epilogue`), `GameMessages`;
  - `GameLogic/SettlementCommands` (`EpilogueOf`), `WorldServer` (the request and its answer);
  - `GameClient/ClientSession` (`LatestEpilogue`), `SettlementHud` (`StandingOf`, `RenownTotal`, `RunTimeOf`, the world panel);
  - `GameData/Strings/en_us.json` (`ui.hud.time_left`, `ui.hud.standing`, `ui.hud.renown`);
  - `GameSharedTests/RelationMessageTests`, `GameLogicTests/RunTests`, `GameClientTests/SettlementHudTests`, `ClientSessionTests`.
