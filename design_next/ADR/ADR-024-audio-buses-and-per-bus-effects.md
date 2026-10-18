---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Audio buses, per-bus effects and the mix

## Context and Problem Statement

Players expect separate volume controls for each part of the mix (the interface, the announcer and stings, voice, world sounds, ambience and music; DEC-570), and the mix needs to stay clean when a battle fills it: no clipping, and no wall of noise. The audio design defines filters and effects for sounds (resonant low-pass, bit crushing, amplitude modulation, echo, reverb) that colour whole groups of sounds, and parameters that are authored on a 0-to-10 loudness scale where each step is 5 dB. Music, which the design phase designs from scratch, needs its own streaming path with cross-fades. Effects must sound the same whatever the device's sample rate.

The problem is how the mix is structured, where effects run, and in what units loudness is authored and applied.

## Decision Drivers

- Player volume controls per category, applied at one point each.
- Effects that sound the same at any device sample rate.
- Bounded effect cost, independent of how many sounds are playing.
- No clipping on the output, whatever the battle load.
- Loudness in decibels throughout the engine, with an exact conversion for reference values authored on the 0-to-10 scale.
- A music path with streaming and cross-fade, ready for the design phase's music system.

## Considered Options

1. A **data-defined bus graph of submix voices**, with **effect chains per bus** (XAPOs), reverb **send buses**, and a **limiter** before the mastering voice.
2. **Effect chains per voice**, as authored per sound event.
3. **No buses**: every source voice goes straight to the mastering voice, with volume settings applied per voice.

## Decision Outcome

Chosen option: **1, a bus graph with per-bus effects, sends and a master limiter**, because it gives each player setting and each effect one place in the graph, and its effect cost does not grow with the number of playing sounds.

- **Bus graph.** The buses are submix voices defined in audio data. The baseline graph has the six **player buses** of DEC-570: **UI**, **announcer and stings**, **voice**, **world**, **ambience** and **music**, each feeding a **pre-master** bus that carries the limiter and feeds the mastering voice; plus **effect buses** that the content build creates per player bus and effect chain, each feeding the player bus of the sounds routed to it. The player's volume settings set the gains of the six player buses and of the pre-master bus (master volume).
- **Routing.** Every sound event names its bus. An effect that the audio design attaches to a group of sounds is realised by routing those sounds to an effect bus whose chain has that effect (insert), or by a send to such a bus (send). No source voice carries an effect chain of its own.
- **Per-voice processing.** Source voices use only XAudio2's built-in per-voice filter (for distance and occlusion low-pass filtering, ADR-027) and send levels to the effect buses.
- **Effects.** Bus chains use the built-in effects `FXEQ`, `FXEcho` and `FXMasteringLimiter`, the XAudio2 reverb (`XAudio2CreateReverb`, with room-reverb parameters converted by `ReverbConvertI3DL2ToNative`), and Species XAPO effects that implement the audio design's other effect kinds, including its own filter algorithms (resonant low-pass, bit crusher, amplitude modulation, echo, simple reverb); the audio engine component lists the realisation of every kind. Every effect processes 32-bit float at its bus's actual sample rate and computes its time constants from that rate.
- **Limiter.** `FXMasteringLimiter` on the pre-master bus keeps the output from clipping.
- **Loudness units.** Every volume parameter in the engine is in decibels and is converted with `XAudio2DecibelsToAmplitudeRatio` at the voice. Reference values authored on the 0-to-10 scale are converted once, when data is cooked, with **dB = 5 × (v − 10)**, so 10 is 0 dB and each step is 5 dB.
- **Music path.** The music bus has a streaming voice pair, so one stream can cross-fade into the next with sample-accurate timing (`OperationSet` and `CommitChanges`). What plays and when is the music system's decision, which the design phase makes.

### Consequences

- Good, because each player setting is one gain on one bus.
- Good, because effect cost depends on the number of buses, not on the number of sounds.
- Good, because effects sound the same on 44.1 kHz and 48 kHz devices.
- Good, because the output never clips.
- Bad, because an effect needed by a single sound requires a bus or send for it, which the audio data must declare.
- Bad, because Species XAPO effects are engine code that must be tested against reference signals.

### Confirmation

- Unit tests run each Species XAPO effect over reference signals at 44.1 kHz and 48 kHz and check that delay times, modulation rates and cut-off frequencies match the definitions at both rates.
- A unit test checks the 0-to-10 conversion at every integer step.
- A stress test plays the loudest recorded battle mix and checks that the output stays within full scale after the limiter.
- A settings test checks that each of the six player volume settings and the master volume changes exactly one bus gain.

## Pros and Cons of the Options

### 1. Bus graph with per-bus effects

- Good, because the mix structure is data and maps onto player settings.
- Good, because effect cost is bounded.
- Bad, because per-sound effects need routing.

### 2. Per-voice effect chains

- Good, because any sound can have any effect.
- Bad, because effect cost grows with the number of playing sounds, and stealing voices with effect chains is expensive.

### 3. No buses

- Good, because the graph is trivial.
- Bad, because category volumes must be applied to every voice, nothing can limit the summed output before the mastering voice, and shared effects such as reverb are impossible.

## More Information

- Technology baseline item 7 (see [the index](README.md#technology-baseline)); source checked 2026-10-17: https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-audio-graph.
- Related: ADR-022 (audio engine), ADR-023 (bus caps and audibility), ADR-025 (streaming), ADR-027 (per-voice filtering).
- The audio design (`reference/10_audio_design/`) defines the filter algorithms and their parameters; the audio engine component (`architecture/components/audio_engine/README.md`) defines the bus graph and the Species XAPO effects.
- **Amendment 2026-10-17.** The baseline graph has the six player buses of DEC-570 under the pre-master bus (AEN-220, R-AUD-221), with content-built effect buses (AEN-223, AEN-224); the room-reverb kind uses the XAudio2 reverb with I3DL2 conversion (AEN-070).
