---
status: proposed
date: 2026-10-23
decision-makers: Species technology lead (M3 task T15)
---

# The audio engine at the MVP's size

## Context and Problem Statement

M3 task T15 plays the first sound set of MVP-087. Its first step cooks the sounds ([ADR-171](ADR-171-the-first-sound-set-synthesized-and-cooked-at-m3.md)). This second step builds the engine that plays them; the third binds the game's events to it.

ADR-022 to ADR-027 and the audio engine component (AEN) describe a full engine:
- layered events with envelopes, modulation, loops and notes;
- emitters tracked per frame;
- streamed music on its own channel;
- effect buses with Species XAPOs;
- X3DAudio matrices for surround output, Doppler and distance filtering;
- six bus volumes in the options.

The MVP's sound set is seven events, each a one-shot of one sample group. It plays at a fixed level, either at its trigger's position or two-dimensional. Only the master volume is stored (O96). This record decides what of the component is built for them, and how the rest is narrowed.

## Decision Drivers

* ADR-023's caps and virtual voices are what keep a battle of `mvp.client.figures_max` figures from flooding the mix with laser shots. A battle triggers hundreds of shots a second, and the event cap is six.
* ADR-026: the game thread never waits on audio, and an XAudio2 callback only sets a flag.
* AEN-321: with no output device, as on a CI runner, the engine runs its whole update silently.
* O98: the first sound set is synthesized whole, and M3's evidence is the owner hearing each sound on its event.
* Everything not built must be stated as a narrowing, so the component stays the target.

## Considered Options

1. **The full component** before the first sound plays.
2. **The component's layers at the MVP's size**, following its rules wherever the MVP's sounds reach them.
3. **A thin wrapper** that starts an XAudio2 voice per trigger, with no ranking or virtual voices.

## Decision Outcome

Chosen option: **2**.
- Option 1 builds envelopes, emitters, streams, effects and surround for sounds the MVP does not have.
- Option 3 has no caps. A battle would exhaust the voice pools, and the first shots fired, not the loudest, would hold them.

What is built:

* **The layers.**
  - `NeuronClient/AudioDevice` is the device interface: the voice back-end of AEN-045, layer 2 of ADR-022.
  - `XAudio2Device` is its implementation, and the only source that includes an XAudio2 header.
  - `SilentAudioDevice` has no voice.
  - `AudioEngine` holds the instances, the ranking, the allocation and the mix (layer 4).
  - `NeuronCore/SpscQueue` is ADR-026's bounded lock-free queue.
  - `AudioMath` holds ADR-027's laws.
* **The graph** (AEN-220, ADR-024):
  - XAudio2 2.9 in-box on the default output device;
  - a mastering voice, under it the pre-master submix with `FXMasteringLimiter` (release `aen.mix.limiter.release`, loudness `aen.mix.limiter.loudness`), then the six player buses;
  - pools of 72 mono and 72 stereo 16-bit 48 kHz source voices (`aen.voice.budget` + `aen.voice.pool_headroom`), all made when the device is built (AEN-142);
  - the changes of an update committed as one operation set;
  - a voice whose buffer XAudio2 has not yet flushed is skipped when a voice is taken.
* **The threads** (ADR-026, AEN-301):
  - the game's commands (trigger, listener, master volume) go into one queue of `aen.queue.command_capacity`;
  - a full queue drops the newest command and counts it (AEN-021);
  - the engine's thread updates every `aen.update.period` on the platform's monotonic clock, and a late update runs once without catching up (AEN-121);
  - the voice and engine callbacks only set atomic flags, which `Build\CheckProjectFiles.py` checks (AEN-025);
  - the device is destroyed on the engine's thread, which may have rebuilt it.
* **The events** (`SoundEventDef`). Each event is one sample group, played once (`once`), positioned (`fix`) or two-dimensional (`2d`), with:
  - a level in dB;
  - a minimum distance;
  - its bus;
  - its cap of real instances (AEN-143).

  The game resolves its events to indices when it makes the engine (AEN-064). A member is drawn as AEN-180 states: equal weights, the last member excluded, drawn by the engine's own `FxRng` seeded with `aen.rng.match_seed` (AEN-004).
* **The update** (AEN-123):
  1. A trigger starts `aen.update.schedule_latency` after it was issued (AEN-024).
  2. Each instance's audibility is its level plus its distance gain (AEN-140, AEN-201), and inaudible under `ref.aud.mix.floor_db`.
  3. Each further copy of an event scores `ref.aud.rank.copy_decay` lower (AEN-141).
  4. The rank is the bus class, then the score, the start and the slot (AEN-144).
  5. The real set stays within the event caps, the bus caps and `aen.voice.budget`, and a candidate steals by AEN-145's four conditions.
  6. A one-shot ends at its sample's end (AEN-175). It is retired while virtual past its start plus its patience, `ref.aud.layer.one_shot_patience_updates` × `ref.sim.frame.audio_update_period` (AEN-146).
  7. A one-shot never real starts at its first frame. A stolen one resumes where it would be.
* **The mix.**
  - Each real voice's volume and constant-power pan are set every update from the listener, the camera (AEN-204, AEN-209, AEN-210, DEC-566).
  - The bus gains are `ref.aud.bus.<bus>.level`, and the world bus is ducked `ref.aud.bus.announcer.duck_world_db` for `ref.aud.bus.announcer.duck_time` after an announcer start (AEN-216).
  - The master volume sets the pre-master gain, `ref.aud.mix.master_step_db` per step below `ref.meta.options.sound.volume_max` (AEN-215).
* **No device, or a lost one** (AEN-100, AEN-259, AEN-321):
  - When XAudio2 or the output device cannot be made, the engine plays through the silent device.
  - When the engine's critical-error callback fires, every instance plays virtual, and the graph is built again on the default device at once, then every second until it can be.

The narrowings, each kept until a milestone needs it:
- One producing thread, the client's main thread; the component allows one queue per producer.
- No emitter tracking, envelopes, modulation, loops, notes, music, streams or effect buses.
- No sub-update start offset: a trigger starts in the update its start time falls in, up to one period early.
- The duck switches at the update with no `aen.mix.duck_ramp`. XAudio2 ramps a volume change over its own processing pass.
- No X3DAudio: an output of more than two channels takes the stereo pan on its front pair, and there is no Doppler, distance filtering or reverb send. Each is off by default in ADR-027, so the default mix is unchanged.
- No per-bus volume steps: only the master volume is stored at the MVP (O96).
- Dropped triggers are counted together, not per reason.

### Consequences

* Good, because the MVP's sounds play under the component's ranking, caps and stealing, so a battle's mix is the one the design specifies.
* Good, because a machine with no output device runs the same update, and every rule is tested on a recording device without XAudio2.
* Bad, because with the reference's values a one-shot never steals. Its patience, 0.15 s, ends before the candidate's hold, `aen.voice.state_hold` (0.25 s), so a full event or bus drops new one-shots. This is what AEN-145 and AEN-146 state together. The stealing rule acts once a longer patience or a loop exists.
* Bad, because surround output hears the stereo pan on its front pair until X3DAudio is built.
* It forecloses nothing in the component. Each narrowing is a part of the component not yet built, not a different rule.

### Confirmation

* `NeuronClientTests/AudioEngineTests` drives the engine on a manual clock through a recording device:
  - the start after the latency;
  - the event cap, bus cap and budget;
  - a steal only after both holds, its fade and its voice's return;
  - an equally loud copy that never steals;
  - the patience and a never-real instance's first frame;
  - a sound's end, by its length and by its voice;
  - the member draw;
  - the distance gain, the pan and the listener's orientation;
  - the master volume and the bus levels;
  - the duck;
  - the full queue and the dropped triggers;
  - the silent device;
  - a device that fails, built again, with its instance resumed at its position;
  - the engine's own thread.
* `NeuronClientTests/AudioMathTests`: the laws.
* `NeuronCoreTests/SpscQueueTests`: the queue's refusal and its order across two threads. The engine's tests and the queue's were also run under ThreadSanitizer on the Linux build, with no report.
* `Build\CheckProjectFiles.py` rule 14: no XAudio2 include outside the back-end, and no allocation, lock or log call in a callback, each proved by a fixture.
* The owner's T15 run: each sound on its event, at its bus's volume (the M3 plan's T15 row).

## Pros and Cons of the Options

### 1 — the full component

* Good, because nothing is narrowed.
* Bad, because most of it serves sounds the MVP does not have.

### 2 — the layers at the MVP's size

* Good, because the rules the MVP's sounds reach are the component's own.
* Bad, because the narrowings must be tracked until each is built.

### 3 — a thin wrapper

* Good, because it is the least code.
* Bad, because it has no caps, ranking or virtual voices, and a battle exhausts the pools.

## More Information

* Amends [ADR-022](ADR-022-audio-engine-on-xaudio2.md) (its layers 2 and 4 built at the MVP's size), [ADR-023](ADR-023-virtual-voices-and-voice-stealing.md) (built; one-shots never steal with the reference's values), [ADR-024](ADR-024-audio-buses-and-per-bus-effects.md) (the graph without effect buses), [ADR-026](ADR-026-audio-command-queue-and-callbacks.md) (one producer) and [ADR-027](ADR-027-spatialisation-and-distance-attenuation.md) (no X3DAudio at the MVP).
* Owner decisions O96 and O98; MVP-087; AEN-004, AEN-020, AEN-021, AEN-024, AEN-025, AEN-045, AEN-064, AEN-100, AEN-121, AEN-123, AEN-140 to AEN-146, AEN-175, AEN-180, AEN-201, AEN-204, AEN-209, AEN-210, AEN-215, AEN-216, AEN-220, AEN-259, AEN-301, AEN-321.
* The code:
  - `NeuronClient/AudioDevice`, `XAudio2Device`, `SilentAudioDevice`, `AudioEngine` and `AudioMath`;
  - `NeuronCore/SpscQueue`;
  - `Build/CheckProjectFiles.py`;
  - `NeuronClientTests/AudioEngineTests` and `AudioMathTests`;
  - `NeuronCoreTests/SpscQueueTests`.
