---
status: accepted
date: 2026-10-20
decision-makers: Species technology lead (M0 task T7)
---

# Game skeleton: the world's owners, the integrated server and the tick report

## Context and Problem Statement

ADR-083 names the world's state owners, `WorldSimulation` and `RegionSimulation`, and the one header through which the client composes the in-process server, `IntegratedServer.h`. It also gives every simulated object a stable id: the creating owner's id in the high 16 bits and a counter in the low 48 (item 5). ADR-084 and MVP-090 keep the client to the transport, and ADR-090 puts the owners in GameLogic, the game's messages in GameShared and the client session in GameClient.

M0 task T7 builds the skeleton of each:
- GameShared's first message, the tick report;
- GameLogic's world with one region;
- `IntegratedServer.h`;
- GameClient's session, which shows the server's tick count in the window title until the UI exists.

Its done-when is that the owner runs `Species.exe` and sees the tick count rise at 10 per second (MVP-090, MVP-091, MVP-096).

The decisions leave open:
- how message type ids are shared between the engine and the game;
- the owner ids;
- what the world's `ServerWorld` does at M0;
- what `IntegratedServer.h` exposes;
- how the session reaches the window;
- the title.

## Decision Drivers

* The client includes no server header but `IntegratedServer.h`, and GameClient none at all (ADR-090 item 4).
* The engine knows no game concept, and a game message never collides with an engine message (R9).
* Every object a world creates has its stable id from its creation (ADR-083 item 5, ADR-082).
* A region steps as a region job from the first tick, so more regions add jobs (MVP-096).

## Considered Options

* 1 — a split of the type ids at 0x0100; owner ids by rank; a `WorldServer` adapter over the owners; a pimpl `IntegratedServer`; a session that is given a title callback
* 2 — one registry of every message type id in NeuronCore
* 3 — option 1 with the session holding the window

## Decision Outcome

Chosen option: "1".
- Option 2 would put game message names in the engine (R9).
- Option 3 would make GameClient include the window's header, and with it the declarations that only follow `<windows.h>`, for one call. A callback keeps the session testable without a window.

* **Message type ids.** Ids below `GAME_MESSAGE_TYPE_FIRST` (0x0100) are the engine's, and the game numbers its messages from 0x0100 in `GameMessageType` (GameShared). Type 0 is never a message, and an id is never reused.
* **The tick report** (`TickReport`, 0x0100, version 1, 11 bytes) carries the tick the server has just stepped. It goes to every connection after each tick on the state channel, where a lost report is replaced by the next. The message is written by hand, in the form the generated codecs will take (ADR-062, ADR-093).
* **Stable ids** (`StableId.h`, GameShared):
  - the world owner's id is 1;
  - region *r*'s is 0x0100 + *r*;
  - each owner's `StableIdCounter` issues ids from counter 1, so 0 is never an id;
  - the counter is owner state, saved and hashed with its owner (T8, T10);
  - a steward's id comes from the world owner's counter when its connection is made.
* **The owners** (GameLogic):
  - `WorldSimulation` holds the world tick, the simulation key, the world's command queue, its id counter and its regions. It steps a tick whole: every region as a job on the worker pool, then its own commands (ADR-083 item 6, ADR-085 item 2).
  - `RegionSimulation` holds its index, owner id, id counter, command queue and tick clock. Its step executes its commands and, from M1, the update slices in order (ADR-079 item 2).
  - At M0 the world is one empty region.
* **`WorldServer`** is the game's `ServerWorld` (ADR-095).
  - It creates a new world with a key from `DrawSecretKey` when it opens. T10 loads the snapshot instead.
  - It makes each connection a steward, and numbers its reliable messages as commands of that steward.
  - It submits each to the world owner's queue, which refuses everything at M0 because no command type exists.
  - It publishes the tick report.
  - It logs a freeze until the world record holds one (M2).
* **`IntegratedServer.h`** offers `Start`, `Stop` and `Connect`, with a description of the clock, the tick period and the workers (2 by default, ADR-080 item 7). It includes only NeuronCore's clock and transport headers: the listener, the world and the host are behind a pointer to their parts. The parts end in the reverse order of their making, the host first.
* **The session** (`ClientSession`, GameClient):
  - It owns the client's end of the transport, dispatches what arrives once per frame, and keeps the server's latest tick.
  - When the tick has changed, it calls the title callback that the composition root gives it with `SessionTitle`: "Species — tick *n*", the product alone before the first report.
  - The title is an interim, compiled-in text, as the pre-UI texts of ADR-092 are. It goes when the UI shows the session.
* **`AppWindow::SetTitle`** sets the caption from UTF-8.
* **The composition root** (`Species.exe`):
  - it creates the presentation, starts the integrated server and connects the session to it;
  - it updates the session each frame before the frame is drawn;
  - at the end it closes the session, stops the server, which freezes the world, and then releases the renderer and the window.
* **Checker.** `Build\CheckProjectFiles.py` rule 14 now checks R-SIM-019 for GameShared and GameLogic. `.clang-tidy` names every mutable global and static variable `g_` and every mutable static member `sm_`, so either prefix in a simulation source is mutable static state, and the checker rejects it.

### Consequences

* Good, because the client reaches the server's state only through the transport from the first tick, and the build checks it.
* Good, because the world already steps its region as a job and every object has an owner and an id.
* Bad, because the tick report is the client's only knowledge of the world until replication (ADR-086) arrives at M1.
* It forecloses: a game message id below 0x0100; a stable id issued outside its owner's counter; a GameClient header that names the window.

### Confirmation

* `GameSharedTests` checks the report's round trip and size, a short report dropped as malformed, and the stable id's parts.
* `GameLogicTests` checks the regions stepped as jobs at 0 and 2 workers, the owners' ids, a non-command refused by the world's queue, the reports of 11 ticks to two clients on a manual clock, and the integrated server's rising ticks through the transport on the real clock.
* `GameClientTests` checks the session's latest tick, its one title per change, a message it cannot read counted, and the close.
* The owner runs `Species.exe` and sees the title's tick rise by about 10 a second (M0 plan, T7).
* A `CheckProjectFiles.py` fixture fails on a `g_` variable in GameLogic.

## Pros and Cons of the Options

### 1 — split ids, ranked owners, an adapter, a pimpl server, a title callback

* Good, because each library keeps to its layer.
* Bad, because the split is a convention that only the type of `GameMessageType` holds.

### 2 — one registry in NeuronCore

* Good, because every id is in one file.
* Bad, because the engine would name the game's messages.

### 3 — the session holds the window

* Good, because the composition root wires one pointer less.
* Bad, because GameClient would include the window's header for one call.

## More Information

* Related: [ADR-079](ADR-079-simulation-tick-rate.md), [ADR-080](ADR-080-per-tick-budgets-and-population-caps.md), [ADR-082](ADR-082-counter-based-simulation-random-draws.md), [ADR-083](ADR-083-server-process-ownership-and-threading.md), [ADR-084](ADR-084-client-boundary-through-the-transport.md), [ADR-085](ADR-085-region-partitioned-world-simulation.md), [ADR-090](ADR-090-solution-and-project-layout.md), [ADR-092](ADR-092-first-window-device-and-presentation.md), [ADR-093](ADR-093-transport-interface-and-message-layout.md), [ADR-095](ADR-095-server-host-tick-loop-and-command-queue.md).
* The code is in GameShared (`GameMessages`, `TickReport`, `StableId`), GameLogic (`WorldSimulation`, `RegionSimulation`, `StableIdCounter`, `WorldServer`, `IntegratedServer`), GameClient (`ClientSession`), NeuronClient (`AppWindow::SetTitle`) and `Species.cpp`.
* **Accepted 2026-10-20, at the end of M0.** Every item of the Confirmation is met. The tests and the fixture pass in CI, and in the owner's run the title's tick rose by about 10 a second (`playtests/M0.md`).
* **Amendment 2026-10-20 (M1 task T7, [ADR-106](ADR-106-leaders-formations-and-battle-commands.md)).** `GameMessageType` adds, after `WorldTerrain`, the outcome report `CommandReport` (0x0102), the battle commands `Select` to `ControlState` (0x0103 to 0x010A) and the AI's commands `AiOrderWorker` to `AiLeaderFields` (0x010B to 0x010D).
* **Amendment 2026-10-20 (M1 task T9, [ADR-109](ADR-109-battle-sandbox-command-journal-and-seed-switch.md)).** `GameMessageType` adds the development command `SandboxRestart` (0x010E), which the world server acts on as it arrives rather than queueing it, in a sandbox and a development build only.
* **Amendment 2026-10-20 (M1 task T12, [ADR-111](ADR-111-replication-state-datagrams-and-the-client-replica.md)).** The tick report is retired: replication's state datagrams carry the tick, and a tick with no change sends an empty update. Its type id 0x0100 stays unused as `RetiredTickReport`. `GameMessageType` adds `StateUpdate` (0x010F) and `PresentationEvents` (0x0110). The title shows the replica's tick and each side's figures.
