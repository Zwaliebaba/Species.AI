---
status: proposed
date: 2026-10-20
decision-makers: Species technology lead (M1 task T13)
---

# The render check: the world frame on the WARP adapter in CI

## Context and Problem Statement

CI has no GPU. Every Direct3D 12 call of the world frame (ADR-113 to ADR-115) would otherwise run for the first time on the owner's machine:
- the pipeline states, the root signature and the barriers;
- the copy queue and the scene targets.

The debug layer's silence there is the T13 run's condition (M1 plan). A fault found only there costs a round of the owner's build and run, for each fault. Every task from T14 to T17 adds to the frame, so the cost recurs.

## Decision Drivers

* The check exercises the code the client runs, with the world the client builds, through the client's own start-up.
* No new dependency (AGENTS.md R14), and no new program or tool project unless it buys something.
* No dialog may wait for nobody on the runner.

## Considered Options

* 1 — a Debug-only start-up option of the client, `--render-check <frames>`, that draws the sandbox's world frames on the WARP adapter with the debug layer and GPU-based validation and sets the exit code, run by a CI script
* 2 — a tool program that composes the renderer and the world itself
* 3 — a test of NeuronClientTests that creates a device

## Decision Outcome

Chosen option: "1".
- Option 2 repeats the composition root and the Agility SDK's exports in a second program, which the checker allows only for the client.
- Option 3 runs in the test host, which exports no Agility SDK version, so it would test another runtime than the client's.

* **The option** `--render-check <frames>` (Debug builds only, PLT table 3.3-A amendment) takes a frame count from 1 to 10,000. It makes the run:
  - **Unattended.** Every pre-UI message is logged and no dialog is shown (`SetPlatformMessagesUnattended`).
  - **On WARP.** The renderer probes the WARP adapter alone, with ADR-002's checks but the hardware one, and turns on GPU-based validation (ADR-001, ADR-092).
  - **The client's own run.** The battle sandbox of `--seed`, joined by the client session, the world built by `WorldPresentation`, and the frame loop.

  The loop ends once the count of world frames has been presented, or after 120 s. The client then exits with `plt.lifecycle.exit_code.normal` when every frame was drawn and the debug layer raised no message of warning severity or above, and with `plt.lifecycle.exit_code.fatal` otherwise. The log states which.
* **The CI step** `Build\CheckWorldFrame.py` runs `Species.exe --seed 1 --render-check 10` after the build, prints the run's log, and fails on any exit code but 0.
* **What it cannot show:** the reference GPU's driver, its sample counts, its enhanced barriers, the frame's cost, and the look. The owner's run stays the frame's confirmation.

### Consequences

* Good, because a fault the debug layer sees on WARP fails CI before the owner builds.
* Good, because every later rendering task is checked the same way.
* Bad, because WARP is a software rasteriser, so the check's run lengthens CI's build.
* Bad, because a fault that only one hardware driver shows still reaches the owner.

### Confirmation

* The CI step passes on the change that adds it, with the world frame of T13.

## Pros and Cons of the Options

### 1 — a client option

* Good, because it checks the client as it runs.
* Bad, because the client carries a development path, compiled out of Release.

### 2 — a tool program

* Good, because the client carries nothing for it.
* Bad, because a second composition root drifts from the first.

### 3 — a test

* Good, because it runs with the other suites.
* Bad, because the test host loads another runtime and holds no window of its own.

## More Information

* Related: [ADR-001](ADR-001-direct3d-12-renderer-on-the-agility-sdk.md), [ADR-002](ADR-002-minimum-gpu-capability.md), [ADR-092](ADR-092-first-window-device-and-presentation.md), [ADR-113](ADR-113-the-frame-at-m1.md).
* The code: `Species/Species.cpp`, `NeuronClient/GraphicsDevice`, `NeuronCore/StartupOptions`, `NeuronCore/PlatformMessage`, `Build/CheckWorldFrame.py`.
* **Amendment 2026-10-21 (M1 task T17b, [ADR-125](ADR-125-battle-marks-route-preview-and-world-markers-at-m1.md)).** The render check drives the battle verbs by script, so that what they draw reaches the WARP adapter: it promotes a Worker, orders the new Leader towards the region's centre, then holds a selection circle with the ground point beyond the Workers. Each step is logged. `Build\CheckWorldFrame.py` draws 40 world frames by default, so the script has the ticks it needs.
