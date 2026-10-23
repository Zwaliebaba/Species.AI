---
status: proposed
date: 2026-10-22
decision-makers: Species technology lead (M1's frame interval, owner decision O93)
---

# The frame starts at the latency wait

## Context and Problem Statement

The owner's first rendered run missed M1's frame criterion (`playtests/M1.md`, 2026-10-22 01:22, `Release|ARM64`):
- **The sandbox's battle drawn.** A 99th-percentile frame interval of 22.34 ms against O75's 17.7 ms. The median was 16.64 ms and the longest 35.5 ms, with 31 of 7,198 frames missed and the frame's work 6.10 ms at its 99th percentile.
- **The battle of `mvp.client.figures_max` figures.** 20.46 ms, with its work 9.56 ms at its 99th percentile and 2 frames missed.

O88 took this up at M2's T19. O93 brings it forward, before T14's third step.

The frame loop, as [ADR-092](ADR-092-first-window-device-and-presentation.md) and [ADR-120](ADR-120-frame-page-toggle-and-sandbox-restart-at-m1.md) built it, runs in this order:
1. the window's messages, the input router and the camera's step;
2. the session;
3. the world's update and the frame's inputs;
4. `FrameRenderer::RenderFrame`, which waits for the frame-latency object (ADR-012), the cap and its slot's fence, then records, executes and presents;
5. the frame boundary.

[ADR-126](ADR-126-frame-bench-and-rendered-scenes.md) measures a frame interval from one boundary to the next, after the present. Three things are wrong with that:

1. **The interval reads the work's variation.** The latency object signals when the display takes a queued frame, at a refresh, and the frame's work follows it. So a boundary lies one frame's work after a refresh, and an interval is a period plus the difference between two frames' work.
   - With two frames queued (ADR-012), a frame whose work runs long by up to a period is still shown on time. Its interval still reads long, and from 25 ms it counts as a missed frame (ADR-126's rounding).
   - The measure therefore counts frames that the display did not miss.
2. **The time step is read at a varying phase.** The camera's step (CAM-030) and the presentation clock are read after the previous frame's present and the message pump. So they vary with the work, while the display shows frames a period apart: the content moves unevenly on an even display.
   - The input is read before the wait, so it is older than it needs to be when the frame is shown.
3. **A long frame cannot be explained.** The record has no GPU time, no breakdown of the waits, and nothing from the display.
   - The 31 long intervals could be the display holding a frame: the GPU over its period, the desktop window manager composing O85's window, or work longer than the queue absorbs. They could equally be work spikes that the queue absorbed.
   - The work's 99th percentile of 6.10 ms does not tell these apart. It is the 72nd longest of 7,198 frames, so 31 frames (0.43 %) can lie above 8 ms beside it.

## Decision Drivers

* The judged interval is what the player sees: the step the frame's content moves by, which the display shows a period after the last.
* The input and the time step are read as late as the swap chain allows, as Microsoft's guidance for waitable swap chains states: "Your rendering loop should wait for the swap chain to signal via the waitable object before it begins rendering every frame" (https://learn.microsoft.com/windows/uwp/gaming/reduce-latency-with-dxgi-1-3-swap-chains, checked 2026-10-22).
* A long frame says where its time went: the CPU's work, the GPU, or the display.
* O75's target and scene are unchanged.

## Considered Options

1. **The frame starts at the latency wait.** The interval runs from one frame's start to the next, and the GPU time, the waits apart and the display's statistics are reported.
2. **The displayed interval judged.** The loop is kept, and the interval is read from the swap chain's frame statistics, the refreshes each shown frame stayed on screen.
3. **The queue deepened.** The loop and the measure are kept, and the maximum frame latency is raised to 3, so that longer work spikes are absorbed.

## Decision Outcome

Chosen option: 1. It corrects both the measure and the motion, and it gathers the evidence that the next run needs.
- Option 2 measures the display honestly, but it leaves the uneven time steps. It also rests the judged figure on statistics that Microsoft calls unreliable in many multi-monitor scenarios, and which can lag on hardware flip queues (`IDXGISwapChain::GetFrameStatistics`, checked 2026-10-22).
- Option 3 adds a frame of latency and hides the measuring fault without removing it.

* **The frame's start** (`FrameRenderer::BeginFrame`):
  - A size change of the window is applied (RND-030), and an occluded window is tested (RND-032).
  - The frame then waits for the frame-latency object and, after it, the frame-rate cap. This happens before the frame's messages are pumped.
  - It is suspended while the window is minimized or occluded, or when the wait found the device removed and rebuilt it (ADR-008).
* **One wait per frame presented.** A frame that is begun but not presented keeps its wait for the next frame. That happens when the window is minimized between the two calls. So the latency object's count never drifts below the queue's depth.
* **Rendering** (`RenderFrame`): applies a size change since the start, waits for its slot's fence, records, executes and presents. It begins the frame itself when its caller did not.
* **The loops.** `Species.exe` and the frame bench call `BeginFrame` first and then read the clock. That reading is the frame's start, and the camera's step and the frame interval are taken from it.
* **The frame interval** runs from one frame's start to the next. It holds the frame's work, its fence wait and its present, and the next frame's wait to begin.
  - A frame's work is the interval less those waits.
  - The frame page's history (ADR-120) and the bench (ADR-126) both take this interval.
  - Missed frames, O75's budget and the scene are unchanged.
* **Where a frame's time went** (`FrameTiming`, `FrameRenderer::LastFrameTiming`):
  - **The waits apart.** The latency object, the cap, the fence and the present.
  - **The GPU time.** Two timestamp queries a frame slot, at the first and the last command of the frame's list, resolved into a readback buffer and read once the slot's fence has completed, so two frames later. This is a whole-frame form of DBG-080's GPU scopes. The scopes per pass wait for the render page.
  - **The display's statistics.** After each present, `GetFrameStatistics` gives the presents the display has shown since the last reading and the refreshes they took. A refresh beyond the presents showed no new frame.
  - **When a source is missing.** Without timestamp queries, or without statistics, that figure is absent and nothing else changes.
* **The bench's output** (ADR-126 amendment):
  - each scene's GPU time and the three waits at their percentiles;
  - the presents shown, the refreshes and the refreshes that showed no new frame;
  - the 10 longest frames, each with its work, waits and GPU time.
* **The script's rows** (`Build\RunPerformance.py`):
  - the GPU time at its 99th percentile and longest;
  - the waits at their 99th percentiles;
  - the refreshes that showed no new frame;
  - the 3 longest frames apart.

  All are reported; only the interval is judged.

### Consequences

* Good, because work spikes that the queue absorbs no longer read as missed frames. The judged interval is the step the content moves by, so a pass means even motion, not only frames on time.
* Good, because the input and the camera's step are read after the wait, so a frame is shown up to a period sooner after its input than before.
* Good, because a long frame shows where its time went:
  - work over its period: a long work figure;
  - the GPU over its period: a long GPU time and fence wait;
  - the display or its compositor: refreshes with no new frame while the work and the GPU are short.
* Bad, because the first rendered run's figures were taken on another interval, so they are not comparable with later runs. `playtests/M1.md` keeps them, says how they were measured, and judges M1 on the next run.
* Bad, because work between one and two periods long still reads as one long interval and one short one, though the display may show every frame on time. That is deliberate: the content's step is uneven then too.
* Bad, because every frame adds two timestamp queries, a resolve and a statistics read. Their cost is not measured apart; the bench's work and GPU figures include it.
* It forecloses: waiting for the frame-latency object after a frame's input and update; measuring the frame interval after the present.

### Confirmation

* CI: the render checks on WARP draw through `BeginFrame` and `RenderFrame`, with the timestamp queries, under the debug layer and GPU-based validation, which must stay silent (ADR-117).
* CI: the self-test of `Build\RunPerformance.py`, with the bench's quick run on WARP and a fixture that the new rows are recorded.
* The owner's next rendered run on `Release|ARM64` with the pinned panel, recorded in `playtests/M1.md`.
  - Its sandbox battle is judged against O75's 17.7 ms on the new interval.
  - Its new rows say whether any frame that is still long was the work's, the GPU's or the display's.

## Pros and Cons of the Options

### 1 — the frame starts at the latency wait

* Good, because the measure and the motion are both corrected, and the evidence for a long frame is gathered.
* Bad, because the first run's figures are not comparable with later ones.

### 2 — the displayed interval judged

* Good, because it is exactly what the display showed.
* Bad, because the uneven time steps stay, and the judged figure rests on statistics that Microsoft states are not always reliable.

### 3 — the queue deepened

* Good, because it is a one-line change.
* Bad, because it adds a frame of latency and leaves the measuring fault in place.

## More Information

* Related: [ADR-012](ADR-012-swap-chain-presentation-and-frame-pacing.md), [ADR-092](ADR-092-first-window-device-and-presentation.md), [ADR-117](ADR-117-render-check-on-warp.md), [ADR-120](ADR-120-frame-page-toggle-and-sandbox-restart-at-m1.md), [ADR-126](ADR-126-frame-bench-and-rendered-scenes.md).
* The rules: MVP-085; RND-030, RND-032; CAM-030; DBG-080, DBG-085.
* Owner decisions O75, O85, O88 and O93.
* The code: `NeuronClient/FrameRenderer`; `Species/Species.cpp`; `Tools/SpeciesFrameBench`; `Build/RunPerformance.py`.
* ADR-012, ADR-120 and ADR-126 are amended with this record.
* **Amendment 2026-10-22 (owner decision O100).** The owner's next rendered run is on the x64 laptop that replaces the reference test machine, on `Release|x64`, with its 1,920 × 1,080 panel set to 60 Hz and the battle drawn borderless, so the desktop window manager's composition of O85's window, one of the causes considered above, does not arise there.
