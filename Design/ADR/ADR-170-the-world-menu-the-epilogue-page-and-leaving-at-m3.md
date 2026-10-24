---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T14)
---

# The world's menu, the epilogue's page and leaving at M3

## Context and Problem Statement

MVP-063 asks that the epilogue open when the run ends, with the final standing, each settlement's history and the steward's statistics, that the steward may reopen the epilogue of any ended world, and that a new run starts from the front end. [ADR-169](ADR-169-the-epilogue-the-standing-and-the-time-left-at-m3.md), T14's first step, brings the epilogue to the client on request. This second step needs a page for it, a way back to the front end from a world, and the front end's entries for ended worlds.

At M2 the client leaves a world only by quitting ([ADR-155](ADR-155-the-front-end-and-saved-worlds-at-m2.md)), and the close action reaches a world only when a window is open.

These decisions are open:
- how the steward reaches the epilogue and the way out from a world;
- how the epilogue's text is laid out on the page family of [ADR-149](ADR-149-the-ui-core-pages-focus-and-layout-at-m2.md);
- how the client leaves a world and starts another in the same process;
- how the front end knows which worlds have ended, and how it opens one's epilogue.

## Decision Drivers

* The client decodes nothing of a snapshot (ADR-155), and the epilogue is built by the world server on request (ADR-169).
* A world whose run has ended stays read-only across a restart, and its server answers status requests ([ADR-163](ADR-163-the-run-renown-and-the-epilogues-data-at-m3.md)).
* The page family's right box at 16:9 is 0.275 of the width wide (528 px at 1920 × 1080) and 0.66 of the height tall, and its small font is 0.022 of the height (23.8 px): computed from the `ref.meta.layout.*` keys by `ComputePageGrid`. At Bahnschrift's width that is about 48 characters a line and 19 lines. The UI core has no wrapping and no scrolling at M3.
* The epilogue of four settlements in whole sentences is about 18 lines of up to 75 characters.
* The world presentation keeps the terrain it built by the address of the session's terrain, and the HUD keeps the world's notices and results.

## Considered Options

For the way in:
1. **A world menu on the close action** while no window is open: Return to the world, Epilogue, Leave to the front end, Quit.
2. **A HUD button** for each.

For the epilogue's text:
3. **One page of every line**, in the right box.
4. **Sections**: a row in the left box for the standing and for each settlement, each showing its lines in the right box.
5. **A scrolled list**.

For leaving:
6. **The quit's teardown, then the front end** in the same process, the presentation dropping the world's surfaces, looks and HUD.
7. **A restart of the process** onto the front end.

For ended worlds:
8. **A flag in the world's description** that the client writes when it sees the run's end, and the ended world continued read-only to show its epilogue.
9. **The epilogue written to a file** at the run's end, read by the front end.

## Decision Outcome

Chosen options: 1, 4, 6 and 8.
- Option 2 puts four more buttons on a HUD that already fills its edges, and the close action is the reference's way to a menu (UI-383).
- Option 3 does not fit: its lines are half again as long as the box is wide.
- Option 5 needs a scrolled widget the UI core does not have.
- Option 7 loses the window, the device and the settings' state for a step the frame loop can take.
- Option 9 copies the region's state, which ADR-163 and ADR-169 decided against.

* **The world's menu** (`GameClient/WorldMenu`, ADR-083 item 3's Leaving state). While a world runs, the client attaches the menu to the UI (`GameUi::AttachWorldMenu`). The close action with no window open opens its screen and does no more that frame. The menu's root has:
  - Return to the world, which the root's close action also does;
  - Epilogue, inactive until the ended run's epilogue has come;
  - Leave to the front end;
  - Quit.

  The world runs on while the menu is open, as a persistent world does, and the menu's window withholds the world's actions (UI-006).
* **The epilogue's arrival.** The menu reads the valley status every frame. Once it says the run has ended, the menu sends `ReadEpilogueCommand` once and keeps the ended run's epilogue when it comes. Its page then opens by itself, over the menu, once.
* **The epilogue's page** (`EpilogueTextOf`). The left box holds a row for each section, then Leave to the front end, and Back. The right box shows the chosen section, the standing at first, as its title in the medium font and its lines in the small font:
  - the standing: two lines a settlement, `ui.epilogue.place` (place, Cladan, renown) and `ui.epilogue.sources` (the four sources);
  - each settlement's history, in the standing's order: its battles won and lost, its claims made, won, lost and withdrawn, its structures raised and the grievances its Cladan took, its most and last Workers, and its strength at first, at its peak and last in whole resource units;
  - the steward's own section, titled `ui.epilogue.yours`, first holds the steward's statistics (WLD-224): the growth steps and the longest gap between two, and when the territory first reached a halfway line, or that it never did.

  Each line is at most 48 characters, which the tests hold. Four settlements give 6 rows in the left box and at most 8 lines in the right.
* **Leaving.** The client ends the world by the quit's teardown (`EndWorld`: the session closes, the server stops and freezes its world). It then drops the world: the presentation's surfaces, the Leaders', effects', structures' and territories' looks, the fog's mask and the HUD (`WorldPresentation::LeaveWorld`). The next world builds its own, even when its terrain lands at the address of the last. The front end reads the saves again and opens its main page.
* **Ended worlds** (`WorldSaves`). When the menu sees the run's end, the client writes `world.run_ended = true` into the world's description (`RecordRunEnded`). `EndedWorlds` lists the ended worlds that hold a snapshot, the highest number first.
  - The main page has a Past runs row, inactive until a run has ended. Its page lists up to `PAST_RUNS_MAX` (6) of them, each choosing its epilogue (`FrontEndChoice::Kind::Epilogue`).
  - The chosen world is continued with `restoreOnly`, as Continue is, and its epilogue opens by itself as it comes.
  - Continue's caption says when the latest world's run has ended (`ui.front.continue_ended`).
* **The strings** `ui.world.*`, `ui.epilogue.*` and the new `ui.front.*` are drafted in `en_us.json` for the owner's review.

### Consequences

* Good, because the steward reaches the epilogue, the front end and the quit from any world, by the reference's close action.
* Good, because every line of the epilogue fits its box at 16:9 and 4:3 without wrapping or scrolling.
* Good, because leaving reuses the one teardown, and a second world in the process builds everything anew.
* Good, because the client still reads nothing of a snapshot: an ended world's epilogue comes from its own server.
* Bad, because opening a past run's epilogue opens its world, a snapshot's load, before the page shows.
* Bad, because the epilogue shows one section at a time.
* Bad, because the world does not pause behind the menu.
* It forecloses: an epilogue file; a client that restarts to leave a world.

### Confirmation

* `GameClientTests`:
  - `WorldMenuTests`: the epilogue's sections, their lines and their length, the steward's statistics and their absence; the menu returns, leaves and quits, its close action returns, and the epilogue waits for the run's end;
  - `ClientSessionTests`: the epilogue is asked for once when the run ends, kept when it comes and its page opened once; the page of four settlements passes UI-372's validator, its text and rows fit their boxes, and Leave and Back act;
  - `GameUiTests`: the close action opens the menu only in a world, and the menu closes as the world ends;
  - `FrontEndTests`: Past runs is inactive with no ended run and chooses an ended world's epilogue;
  - `WorldSavesTests`: the run's end is written, read back with the seed, and listed only for a world that saved.
* The owner's run: Escape in a world opens the menu, Return and Escape return to it, and Leave reaches the front end, from which Continue or a new world starts. A run's end needs `mvp.run.length_hours` shortened in a Debug build's content. The epilogue then opens by itself, reads in sections, and Past runs reopens it after a restart.

## Pros and Cons of the Options

### 1 — a world menu on the close action

* Good, because it is the reference's way to a menu, and frees the HUD.
* Bad, because a steward who does not know the close action does not find it; the HUD's help waits for M4.

### 2 — HUD buttons

* Good, because they are seen.
* Bad, because the HUD has no room for four more.

### 3 — one page of every line

* Good, because it shows everything at once.
* Bad, because it does not fit.

### 4 — sections

* Good, because each fits, and a settlement's history reads as one block.
* Bad, because the steward chooses what to read.

### 5 — a scrolled list

* Good, because it holds any length.
* Bad, because the UI core has no scrolling yet.

### 6 — the quit's teardown in the same process

* Good, because the frame loop keeps its window, device and settings.
* Bad, because every per-world state of the presentation must be dropped, which `LeaveWorld` lists.

### 7 — a restart

* Good, because nothing of the last world can remain.
* Bad, because the window and device are made again, and the steward sees the start-up.

### 8 — a flag in the description

* Good, because the front end lists ended worlds without reading a snapshot.
* Bad, because a run that ends while no client watches is flagged only when the world is next continued.

### 9 — an epilogue file

* Good, because the epilogue opens at once.
* Bad, because it copies the region's state.

## More Information

* Amends [ADR-083](ADR-083-server-process-ownership-and-threading.md) item 3 (the Leaving state), [ADR-155](ADR-155-the-front-end-and-saved-worlds-at-m2.md) (the front end's Past runs, the description's `world.run_ended`) and [ADR-169](ADR-169-the-epilogue-the-standing-and-the-time-left-at-m3.md) (the page of its second step).
* MVP-062, MVP-063, MVP-103; PIL-125; WLD-224; UI-006, UI-163, UI-372, UI-383.
* The code:
  - `GameClient/WorldMenu` (`EpilogueTextOf`, the menu's and the epilogue's pages), `PageSkeleton` (the page family's frame, shared with the front end), `FrontEnd` (Past runs), `WorldSaves` (`RecordRunEnded`, `EndedWorlds`), `GameUi` (`AttachWorldMenu`, `OpenWorldMenu`, `CloseWorldMenu`), `WorldPresentation` (`LeaveWorld`), `SettlementHud` (`CladanName`);
  - `Species/Species.cpp` (the menu's update and choices, leaving, the epilogue's world);
  - `GameData/Strings/en_us.json`;
  - `GameClientTests/WorldMenuTests`, `ClientSessionTests`, `GameUiTests`, `FrontEndTests`, `WorldSavesTests`.
* **Amendment 2026-10-24 (M4 task T10, [ADR-191](ADR-191-the-away-report-and-the-pace-in-the-client.md)).** The menu has an "Away report" row, and the away report takes the epilogue's page, with its summary and the deputy's actions as its sections and no Leave.
