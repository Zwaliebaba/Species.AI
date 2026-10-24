---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M2 task T18, owner decisions O80, O96)
---

# The front end and saved worlds at M2

## Context and Problem Statement

M2's T18 builds MVP-103's page as O80 grows it: continue a world, a new world with its seed and site, the display and volume options of O96, and quit. The battle sandbox stays as a Debug build's start-up option.

The plan moves the saves folder from T9 to T18, because the front end starts the first world the programs save ([ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md)). ADR-137 also states that the front end shows the steward why a world cannot be continued.

The client composes one world at start-up today:
- the integrated server, the session and the view are made before the frame loop;
- a Debug build's `--world` gives a new world, and otherwise the sandbox starts.

ADR-083 item 3 gives the client's states: Boot, FrontEnd, Connecting, Loading, InWorld, Leaving and back to FrontEnd, with one teardown for every exit.

A world server given a snapshot folder opens the newest snapshot that validates. When none validates, it opens a new world in that folder ([ADR-099](ADR-099-world-snapshots.md)), whose saves then replace the snapshots it refused. A snapshot written by a later build would be lost that way.

## Decision Drivers

* A world closed with the client continues where it stood (T18's run checklist).
* A world that cannot be continued is never overwritten, and the steward sees why.
* The integrated server stays composed through `IntegratedServer.h` alone (ADR-083 item 2).
* The client never decodes a snapshot; GameClient does not link the server's libraries (ADR-090).

## Considered Options

1. **The front end as the client's first state, a world started on the steward's choice, a folder per world, and a restore-only open** that reports why nothing could be continued.
2. **One saves folder for the one world**: a new world overwrites it.
3. **The front end validates the snapshots itself** before it starts the server.

## Decision Outcome

Chosen option: 1.
- **Option 2** loses a world the steward may want back, and MVP-103 grows an epilogue of ended worlds at M3 (O80).
- **Option 3** puts the snapshot's framing and the world's state versions into the client, which never decodes the world.

* **The client's states** (`Species/Species.cpp`):
  - **The two states.** M2 has two of ADR-083 item 3's states: FrontEnd, then InWorld. The frame loop runs the front end until the steward chooses. A world then starts on the main thread (Connecting and Loading at M2), and the loop runs it until the quit.
  - **One teardown.** Every exit ends a world the same way: the session closes, then the server stops and freezes the world (`EndWorld`).
  - **What waits.** Leaving back to the front end from a world, and a loading page.
* **The Debug start-up options:**
  - `--sandbox` starts the battle sandbox at once, in place of the front end.
  - `--world` starts a new world with the steward's settlement at once.
  - Neither saves. `--seed` and `--watch` apply to both.
  - **The render check.** With either option it counts world frames, as before. With neither, it draws the front end's new world page and counts presented frames (`FrameRenderer::PresentedFrames`). `Build\CheckWorldFrame.py` passes `--sandbox` by default and has `--front-end`, which CI runs too.
* **The saves folder** (`GameClient/WorldSaves`):
  - **Layout.** `saves\` under the user-data root (PLT-016, PLT-017), which `--user-data` moves (PLT-025). It holds a folder per world, `world-0001` and onward, numbered above every folder of that form, so none is reused.
  - **The description.** Each world folder holds `world.txt` in DAT-196's form. It names the steward's site and, once known, the seed. A seed the server draws, or MVP-011's re-roll, reaches the description when the world's terrain arrives (`RecordWorldSeed`).
  - **No folder.** Without a saves folder, a new world runs unsaved and the main page says so.
* **Continue** opens the latest world: the highest-numbered folder that holds a snapshot.
  - **The restore-only open.** The integrated server opens it with `restoreOnly` (`WorldServer::Desc`). When no snapshot validates, the world opens new without its folder: it saves nothing, removes nothing, and keeps the marker of the clean stop.
  - **The fault.** `IntegratedServer::Start` stops the server and returns a `ContinueFault` (GameShared) for the newest snapshot refused: no snapshot, unreadable, damaged, or another version (MVP-095). `Start`'s error is now a `StartFault`, either a record's `AssetError` or a `ContinueFault`.
  - **What the steward sees.** The main page shows the reason (`ui.front.continue_failed`), and the world's folder is left as it was.
  - **The outcome.** `WorldServer::Opened()` gives whether the world was restored, and each refusal with its reason.
* **The front end** (`GameClient/FrontEnd`):
  - **Three pages** of the UI core, on UI-163's page grid in the skeleton of UI-373 (`SkeletonPage`). GameUi's factory makes them for `GamePage::FrontEndMain` and onward, while the front end is open.
  - **The main page:**
    - Continue, inactive with no world, captioned with the world's seed;
    - New world;
    - Options;
    - Quit;
    - a notice for the last fault, or for no saves folder.

    It is the root, so the close action does nothing (UI-383).
  - **The new world's page:**
    - the seed field ([ADR-154](ADR-154-text-input-and-the-page-text-field-at-m2.md)), digits only up to 20; an empty field lets the server draw the seed, and one beyond 64 bits starts nothing and says so;
    - the site, a cycler of the region's four, from `mvp.sandbox.steward_site`;
    - Start;
    - Back.
  - **The options page** (O96):
    - "Play in a window" and vertical sync, which apply at once (`AppWindow::SetMode`, `FrameRenderer::Reconfigure`);
    - anti-aliasing at the next start (ADR-150);
    - the master volume in steps of 10 % of `audio.master_volume`'s 255, stored for M3's sound.

    Each is written to the settings store as it changes, and leaving the page saves the store.
  - **The choice.** The front end holds the steward's choice until the client takes it, and a new choice clears the last fault.
  - **The strings** `ui.front.*` are drafted in `en_us.json` for the owner's review.

### Consequences

* Good, because a world continues where it froze, and one that cannot be continued is kept as it is, with its reason shown.
* Good, because every world keeps its own folder, ready for M3's epilogue of ended worlds.
* Good, because the client still knows nothing of a snapshot's contents.
* Bad, because a world that cannot be continued costs a new world's opening before the server stops.
* Bad, because the client returns to the front end only by quitting and starting again at M2; M3 adds leaving from the world's menu ([ADR-170](ADR-170-the-world-menu-the-epilogue-page-and-leaving-at-m3.md)).
* It forecloses: a new world written over an old one; a client that reads snapshots.

### Confirmation

* GameLogicTests:
  - `SavingTests`: a world opened only to continue is restored from a snapshot that validates. With only a later version's snapshot, it opens new and reports the refusal, and after a run and a freeze the refused snapshot and the marker are still there with nothing added.
  - `IntegratedServerTests`: a world to continue with no snapshot starts no server, gives `ContinueFault::NoSnapshot`, and saves nothing.
* GameClientTests:
  - `WorldSavesTests`:
    - new worlds are numbered and described, and a seed recorded later is read back;
    - Continue picks the highest world that saved a snapshot;
    - a folder whose description does not read, or whose name is of another form, is no world;
    - a new world is numbered above every folder of the form.
  - `FrontEndTests`:
    - each page passes UI-372's validator;
    - Continue is inactive with no world and takes the cursor with one; a choice is taken once and clears the last fault;
    - the new world starts from the seed typed or a drawn one, at the site chosen, and a seed beyond 64 bits starts nothing;
    - the options write the settings and flag the window mode and vertical sync for at once;
    - `SeedOfText`'s limits.
* CI: the render check of the front end on WARP (`CheckWorldFrame.py --front-end`).
* The owner's T18 run:
  - starts a world on a chosen seed and site;
  - closes the client;
  - continues the world where it stood;
  - and checks the options page.

## Pros and Cons of the Options

### 1 — the front end first, a folder per world, a restore-only open

* Good, because nothing the steward saved is overwritten.
* Bad, because the server opens a world it then stops when nothing can be continued.

### 2 — one folder for one world

* Good, because there is nothing to choose between.
* Bad, because a new world loses the old one.

### 3 — the front end validates snapshots

* Good, because no server starts for a world that cannot be continued.
* Bad, because the client would decode the world's framing and versions.

## More Information

* Amends [ADR-083](ADR-083-server-process-ownership-and-threading.md) item 3 (the client's states at M2), [ADR-099](ADR-099-world-snapshots.md) and [ADR-137](ADR-137-saving-the-journal-cut-and-the-world-record-at-m2.md) (the saves folder, the restore-only open); adds `saves\` to table 3.1-A of the [platform layer](../architecture/components/platform_layer.md).
* Owner decisions O80, O96; MVP-005, MVP-011, MVP-012, MVP-075, MVP-095, MVP-103; PLT-016, PLT-017, PLT-025, PLT-030; UI-163, UI-372, UI-373, UI-383.
* The code: `GameClient/FrontEnd`, `WorldSaves`, `GameUi`, `WorldPresentation` (`FrontEndFrame`), `ClientSettings` (`MASTER_VOLUME_MAX`); `GameShared/ContinueFault`; `GameLogic/IntegratedServer`, `WorldServer` (`restoreOnly`, `Opened`); `NeuronClient/FrameRenderer` (`PresentedFrames`); `NeuronCore/StartupOptions` (`--sandbox`); `Species/Species.cpp`; `Build/CheckWorldFrame.py`.
* **Amendment 2026-10-23 (M3 task T14, [ADR-170](ADR-170-the-world-menu-the-epilogue-page-and-leaving-at-m3.md)).** The description also records that the world's run has ended (`world.run_ended = true`, `RecordRunEnded`). The main page gains Past runs, inactive until a run has ended, whose page chooses an ended world's epilogue; that world is continued with `restoreOnly` as Continue's is, and Continue's caption says when the latest world's run has ended. The client leaves a world back to the front end from the world's menu.
* **Amendment 2026-10-23 (owner decision O103, [ADR-157](ADR-157-the-four-settlements-and-their-peace-at-m3.md)).** The New world page also holds a Neighbors cycler, three AI settlements (the default) or none, which the new world's start carries; the description records it (`world.ai_settlements`, 0 for a world described before it), and a description naming more than three is no world.
* **Amendment 2026-10-24 (M4 tasks T9 and T10, [ADR-191](ADR-191-the-always-on-worlds-server-process.md), [ADR-192](ADR-192-the-away-report-and-the-pace-in-the-client.md)).** `WorldSaves` lives in GameShared, which the headless server reads too. A world's description names its pace (`world.pace`); one without is at the session pace. Continue also opens an always-on world whose server runs before its first snapshot. The New world page chooses the pace.
