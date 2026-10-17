---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Audio engine command queue, control rate and callbacks

## Context and Problem Statement

Sound triggers come from game code at frame rate: presentation reacts to simulation events, the UI reacts to the player, and the camera moves the listener. The audio engine's work (ranking instances, evaluating parameters, spatialising, refilling streams, decoding) must not cost the game thread time, and the game thread must never wait for audio. XAudio2 runs its own processing thread and calls Species code from it; a callback that blocks or allocates on that thread causes audible glitches. Sound parameters that follow game values (the audio design's fixed, random and linked parameters with smoothing) need smoothing that depends on elapsed time, not on how often the game calls in.

The general threading model of the simulation and of presentation is deferred to the design phase. This decision covers only the boundary between game code and the audio engine and the audio engine's internal timing.

The problem is how game code talks to the audio engine, at what rate the engine updates, and what XAudio2 callbacks may do.

## Decision Drivers

- The game thread never blocks on audio and never calls XAudio2 directly.
- XAudio2 callbacks never block, allocate or take locks.
- Parameter smoothing and fades depend on elapsed time, not on call frequency.
- Changes that belong together take effect at the same sample.
- Trigger timing is preserved from the game's timeline into the mix.
- No dependence on the deferred threading model beyond "the audio engine owns its own update".

## Considered Options

1. A **lock-free command queue** from game code to the audio engine, an **audio-engine update at a fixed control rate**, and **signal-only callbacks**.
2. **Direct calls** from game code into the audio engine and XAudio2, under a mutex.
3. All audio logic **inside XAudio2's processing-pass callbacks**.

## Decision Outcome

Chosen option: **1, a lock-free command queue, a fixed-rate audio update and signal-only callbacks**, because it isolates the game thread from audio entirely and keeps every callback trivially short.

- **Command queue.** The audio engine's public interface (trigger an event for a source, stop, set an instance or global parameter, set the listener, pause, resume, change a bus gain) only writes a command into a bounded, lock-free single-producer single-consumer queue. Each producing thread has its own queue; the audio update drains all of them. Commands carry their issue time on the **audio clock**, which reads the platform's monotonic clock (ADR-074) and is never scaled by game speed or stopped by pausing; it is not the presentation clock of ADR-036. A full queue drops the newest command and counts the drop in the development overlay.
- **Audio update.** The audio engine runs its update at a **fixed control rate** (an audio engine tuning value), on a thread or recurring task that the audio engine owns. Each update drains the queues, ranks instances (ADR-023), evaluates parameters, computes spatialisation (ADR-027), and applies all voice changes of that update as one operation set (`OperationSet` with one `CommitChanges`), so they take effect at the same sample.
- **Trigger timing.** A trigger's start time is its issue time plus a fixed scheduling latency (an audio engine tuning value); it starts at the first update that reaches its start time, with its offset relative to other triggers preserved (ADR-023).
- **Parameters.** Fixed, random and linked parameters (`SoundParam`) are evaluated at the control rate. Linked parameters read game values that game code sends in commands; the audio engine never reads simulation or presentation state directly. Smoothing is a function of elapsed time from the parameter's current value, and game-speed changes affect only parameters that the audio design links to game speed.
- **Callbacks.** `IXAudio2VoiceCallback` and `IXAudio2EngineCallback` implementations only set atomic flags or push to a lock-free queue (for example "stream buffer free" to the decode task of ADR-025, "voice ended" to the update). They never allocate, lock, log synchronously, create or destroy voices, or call back into game code.
- **Results back to the game.** The few results game code needs (for example "this instance has ended") come back through a second lock-free queue that game code drains once per frame.

### Consequences

- Good, because the game thread's audio cost is the cost of writing commands.
- Good, because callbacks cannot cause glitches by waiting.
- Good, because smoothing, fades and starts behave the same at any frame rate.
- Good, because the decision does not constrain the design phase's threading model beyond the audio engine's own update.
- Bad, because every interaction with audio is asynchronous, so game code cannot query audio state synchronously.
- Bad, because queue capacities are tuning values; a burst beyond capacity loses triggers (counted, and visible in Debug builds).

### Confirmation

- A code check fails when any code outside the audio engine includes XAudio2 headers, or when a callback implementation calls an allocator, a lock or a logging function (by a static-analysis rule on the callback translation units).
- A stress test fires the largest recorded trigger burst and checks for no game-thread waits on audio and no queue drops at the configured capacities.
- A timing test changes the frame rate during a fade and a parameter smoothing and checks that both follow elapsed time.

## Pros and Cons of the Options

### 1. Command queue, fixed-rate update, signal-only callbacks

- Good, because it isolates threads cleanly.
- Bad, because it adds queue plumbing and asynchronous semantics.

### 2. Direct calls under a mutex

- Good, because calls are synchronous and simple to write.
- Bad, because the game thread can wait on the audio thread, and a lock taken in a callback can stall the mix.

### 3. Logic in processing-pass callbacks

- Good, because updates align with the mix quantum.
- Bad, because heavy work on XAudio2's processing thread risks glitches, and Microsoft's guidance is to keep callbacks short.

## More Information

- Technology baseline item 7 (see [the index](README.md#technology-baseline)); source checked 2026-10-17: https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-audio-graph.
- Related: ADR-022 (audio engine), ADR-023 (ranking and starts), ADR-025 (decode task), ADR-027 (spatialisation).
- The threading model of the simulation and of presentation is deferred to the design phase (see [the index](README.md#deferred-to-the-design-phase)).
- **Amendment 2026-10-17 (AGENTS.md, O21).** Dropped-command counts are shown in Debug builds, the configuration with the development overlay.
- **Amendment 2026-10-17.** A command's issue time is read from the real-time audio clock, not from the game-speed-scaled presentation clock, and a trigger starts at its issue time plus the scheduling latency, as the audio engine component specifies (AEN-006, AEN-024, AEN-148).
