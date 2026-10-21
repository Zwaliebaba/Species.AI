---
status: proposed
date: 2026-10-21
decision-makers: Species technology lead (M1 task T15b), owner (O75)
---

# The development overlay's frame page, its toggle and the sandbox restart at M1

## Context and Problem Statement

M1 task T15b is the rest of the development overlay after its text (O75, M1 plan table 2 row T15):
- the toggle through the input router;
- the frame page and its budget rows;
- the tick cost, each side's figures and the seed;
- the sandbox's restart.

Its run is the owner toggling the overlay and checking that its figures agree with the log. The requirements are MVP-005, MVP-100 and DBG-095 to DBG-099.

Five things stand in the way:
1. **No profile tree.** The diagnostics component reads the frame page's step shares from the profiling service's tree of DBG-080 to DBG-084. At M1, `ENGINE_PROFILE_SCOPE` emits PIX events only. The in-process marker timing of ADR-110 is for the performance tool, and no game program turns it on.
2. **No tick cost on the client.** The tick cost is the server's, and nothing sends it to the client.
3. **No usable frame budget row.** A budget row on the frame time cannot work on a synchronised display. Every frame there lasts the display period: 16.7 ms at 60 Hz. The 99th percentile of the frame time is then above 16.7 ms from jitter alone.
4. **No restart command to bind.** The debug-command window, where the restart belongs (DBG-117), does not exist.
5. **Missing glyph and timestamps.** EditorFont holds no middle dot (ADR-118). GPU timestamps are not built.

## Decision Drivers

* The overlay exists in Debug builds only (DBG-004), and nothing it measures reaches simulated state (ADR-081).
* No new dependency (AGENTS.md R14), and the engine knows no game step by name (R9).
* A budget row shows a measured value against the budget that the design states for it (DBG-097): RND-454 for the frame, ADR-080 item 6 for the tick.
* The owner checks the overlay against the log, so the two must show the same figures.

## Considered Options

1. **M1's subset.** A frame profile of the frame loop's root steps, a Debug server's statistics message, and compiled-in bindings.
2. **The whole profiling service.** A tree of every profile scope with its roll-ups, and the profile view's tool window of DBG-086 to DBG-089.
3. **Marker timing on in the client.** ADR-110's per-thread tables, taken once a period.

## Decision Outcome

Chosen option: 1, because it meets the frame page of DBG-096 and the budget rows of DBG-097 with what M1 has. Option 2 needs the UI's tool windows and buttons. Option 3 turns on in a game program what ADR-110 keeps to the performance tool. It also measures nested markers rather than the loop's steps.

* **The toggle** (DBG-020, DBG-095, DBG-098).
  - `action.debug.overlay`, `key:f5:down` (input table 3.11-A row 6), is bound in the development layer of a Debug build.
  - It shows and hides the overlay, which starts hidden.
  - M1 has one page, the frame page of DBG-023, shown with the frame-rate line. The page list and its stepping with the menu actions wait for the menus.
  - The figure dots of ADR-118 stay on whether the overlay is shown or not, since they stand in for the crowds until T16.
  - A render check (ADR-117) shows the overlay, so that the debug layer on WARP sees the overlay's draws.
* **The frame profile** (`FrameProfile`, NeuronCore). It is the profiling service's frame statistics in their M1 form.
  - The frame-time history keeps the newest `dbg.profile.frame_history` (200) frames, newest first (DBG-085).
  - Beside each frame it keeps the frame's **work**: its time less the renderer's waits.
  - Each root step's time is summed over a roll-up period of `dbg.profile.rollup_period_ms` (1000 ms) of the platform clock. The period is checked once a frame and rolls over by DBG-082's rule: the length of the period just ended is now − (its end − the period), and the next period ends one period after now.
  - A step's share is its last-period time × 100 ÷ the last period's length (DBG-096).
* **The root steps** (`ClientStep`) are the frame loop's:
  - input: the messages, the router, the actions and the camera;
  - session: the received messages and the replica;
  - presentation: the world's update and the frame's inputs;
  - render: the renderer's recording and submission;
  - wait: the renderer's waits for the frame-latency object, the frame-rate cap, its slot's fence and the present (`FrameRenderer::LastFrameWaits`).

  `ENGINE_PROFILE_SCOPE` stays PIX-only.
* **The percentiles.** `dbg.telemetry.percentiles` (50th, 95th, 99th) take the lower index: the value at ⌊p ÷ 100 × (n − 1)⌋ of the sorted history, as the performance tool reads its own (ADR-110).
* **The frame's budget row** compares the 99th percentile of the frames' work with 1000 ÷ `rnd.budget.target_frame_rate` (16.7 ms). RND-454 budgets the frame's CPU and GPU time, not the display's period. The frame-time row has no budget.
  - A GPU-bound frame shows as wait, not work. The GPU's budget rows (RND-041, RND-454's pass groups) belong to the render page, with GPU timestamps (DBG-080's `ENGINE_PROFILE_GPU_SCOPE`), which M1 does not build.
* **The server's statistics** (`ServerStats`, GameShared).
  - The message type follows `PresentationEvents`. It is version 1 and 31 bytes long, and is sent on the reliable channel.
  - A Debug server sends it to each steward once a second of ticks, that is round(1 ÷ `ref.sim.tick_period`) ticks.
  - It carries the world's seed, the latest tick, the ticks of the period, and the mean and the longest of their wall time in microseconds.
  - A tick's wall time runs from the start of the world's step to the end of its replication. The commands received before the step are not in it.
  - It is measured with the marker clock and never reaches the simulated state, the state hash or the journal.
  - A Release server sends none, and the page then reads "no statistics yet".
* **The tick's budget row** compares the period's longest tick with half the tick period, 50 ms (ADR-080 item 6).
* **The counts** come from the replica's latest view (DBG-096): each team's living Workers and Leaders, from slot 0 to the highest seen, the Essence orbs and the effects. M1 has no structures, laser bolts or particles, so the page shows none.
* **The page's layout.** Every measure is in unscaled pixels times the overlay's text scale *s* (O63).
  - The page starts at (10, 10) from the top-left corner, with its lines a cell and 2 px apart, in this order:
    1. frame;
    2. work;
    3. steps;
    4. a line per team;
    5. orbs and effects;
    6. server.
  - Text is white, and a row over its budget is drawn in `dbg.overlay.warning_rgb` (255, 64, 64).
  - The parts of a line are separated by ", ", which EditorFont holds.
* **The frame-time graph** (DBG-089) is drawn 8 px below the page, not in the profile view's window (DBG-086), which needs the UI's tool windows.
  - It is as wide as the history, one pixel per frame, and `dbg.profile.view.graph_height` (100 px) high, over a black backdrop at alpha 0.5.
  - Each frame is a red line, the newest at the right, of height `dbg.profile.view.graph_height` × its time ÷ `dbg.profile.view.graph_full_ms` (200 ms), at most the graph's height.
  - Grey guide lines are drawn at `dbg.profile.view.graph_marks_ms` (200, 100 and 50 ms), each labelled with its time and its rate, as "200 ms (5 fps)".
* **The drawing.**
  - An `OverlayLine` is anchored to the target's bottom-right corner, as the frame-rate line is, or to its top-left corner (`OverlayCorner`); its distances are `insetX` and `insetY`.
  - The graph is `OverlayRect`s.
  - Pass 29 draws the rectangles first, with the catalogue's `development_solid`: the text's vertex shader and `DevelopmentTextSolidPS`, alpha-blended, reading no texture. The text follows.
* **The restart** (ADR-109, DBG-022 amendment). Until the debug-command window exists, the development layer binds:
  - `action.debug.restart_sandbox`, `chord(key:control, key:shift, key:f7)`, which restarts the sandbox from its seed;
  - `action.debug.restart_sandbox_new_seed`, `chord(key:control, key:shift, key:f8)`, which restarts it from a new seed.

  The client sends `SandboxRestartCommand` on the reliable channel and logs the request. F7 and F8 are in no row of input table 3.11-A or of the default set.
* **The log line.** Every tenth roll-up period (10 s), whether or not the overlay is shown, the client writes the page's figures to the Diagnostics log as one line. The line starts "overlay:" and holds the frame rate, both sets of percentiles, the counts and the server's latest tick and tick times.

### Consequences

* Good, because the owner's T15 run can check the overlay against the log, line for line, and sees the budgets of RND-454 and ADR-080 item 6 while playing.
* Good, because the render check draws the overlay's two pipelines on WARP under the debug layer.
* Bad, because the work row cannot flag a GPU-bound frame until GPU timestamps exist; such a frame shows as a large wait share.
* Bad, because the integrated server's tick time is wall time on the host the client shares, so the client's threads can stretch it. That is what ADR-080 item 6 measures on a host, so the row says what the budget means.
* Neutral, because a Debug build has one more message type and a Release server sends nothing more.

### Confirmation

* NeuronCoreTests: the history, the percentiles, the work and the roll-up (`FrameProfileTests`).
* GameSharedTests: the statistics round-trip (`ServerStatsTests`).
* GameLogicTests: a Debug world server sends one statistics message for its first ten ticks (`WorldTests`).
* GameClientTests: the counts, the page's rows, its layout and graph, the warning colour and the log line (`DevelopmentOverlayTests`).
* The render check of ADR-117 on WARP, with the overlay shown.
* The owner's run: F5 toggles the overlay, its figures agree with the log's "overlay:" lines, and Ctrl+Shift+F7 and Ctrl+Shift+F8 restart the sandbox.

## Pros and Cons of the Options

### 1 — M1's subset

* Good, because it needs nothing that M1 does not build, and it measures the loop's own steps.
* Bad, because the page is fixed in code until the UI hosts it.

### 2 — the whole profiling service

* Good, because it is the component's design as written.
* Bad, because the profile view needs tool windows, buttons and row expansion that the UI does not have at M1.

### 3 — marker timing on in the client

* Good, because the markers exist and their times are recorded without a lock.
* Bad, because ADR-110 keeps marker timing to the performance tool, and nested markers do not sum to the frame.

## More Information

The frame page's other rows wait for their components: the render page for GPU timestamps, and the audio share of DBG-097 (`dbg.overlay.audio_share_warning`) for audio. The DBG-099 world layers and the profile view of DBG-086 to DBG-089 also wait. This ADR is confirmed by the owner's T15 run, as the M1 plan's table 2 states.
