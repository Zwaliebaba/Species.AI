---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Audio engine on XAudio2 2.9 and X3DAudio

## Context and Problem Statement

Species sound is data-driven: game code fires a (sound source, event) pair, and data decides what plays, with friend and foe variants, sample groups, generative loops and per-event parameters (the audio design in `reference/10_audio_design/`). A battle can request far more simultaneous sounds than any device should play, positional sounds follow units across the battlefield, and the mix needs separate control of effects, ambience, UI and music. The audio engine is a voice graph on XAudio2 under the data-driven (sound source, event) binding of the audio design; it must follow device changes (headphones plugged in, default device switched) without a restart, and must never influence the simulation. The headless dedicated server has no audio at all.

The problem is which audio API the client's audio engine is built on, and how the engine is layered on it.

## Decision Drivers

- A voice graph with source voices, submix buses and effects, processed in floating point off the game thread.
- 3-D positioning (panning, Doppler, distance filtering) for crowds of positional sounds.
- In-box on every supported OS (Windows 11 24H2 and later); no redistributable.
- Automatic following of the default device and of the user's spatial-sound setting.
- A module plan that keeps the binding model separate from voices, sample data, the device and effects.
- Isolation from the simulation; no audio in the dedicated server.

## Considered Options

1. **XAudio2 2.9 with X3DAudio**, in-box, with the engine's binding, instance, sample-data, voice and effect layers on top.
2. An **own mixer on WASAPI** (shared-mode, event-driven `IAudioClient3`).
3. **Commercial audio middleware** with its own authoring tool.

## Decision Outcome

Chosen option: **1, in-box XAudio2 2.9 with X3DAudio**, because it provides the voice graph, effects and 3-D calculations Species needs, ships with every supported OS, and leaves the binding model entirely in Species' own data.

- **API.** The client links `xaudio2.lib` from the Windows SDK and uses the in-box `xaudio2_9.dll`. The XAudio2 redistributable is not installed. X3DAudio and the built-in effects (`XAPOFX`) come from the same library.
- **Graph.** One XAudio2 engine with one **mastering voice**, created on the default device with the default format (no fixed sample rate), so the engine follows device switches and the system spatial-sound format. Under it, one **submix voice per bus** (ADR-024), and **one source voice per playing sound** (ADR-023), fed with resident buffers or streamed packets (ADR-025).
- **Module plan.** The audio engine (`AudioEngine`) is built in five layers, each depending only on the ones below it, listed from the top:
  - layer 5, **binding**: (source, event) lookups into the sound source and event definitions (`SoundSourceDef`, `SoundEventDef`);
  - layer 4, **instances**: playing and virtual sound instances (`SoundVoice`), their parameters (`SoundParam`) and priorities (ADR-023);
  - layer 3, **sample data**: resident banks and streams owned by the asset system (`SampleCache`, `SampleBank`, `AudioStream`; ADR-025);
  - layer 2, **voices and device**: the XAudio2 graph (`AudioBusGraph`), the pooled source voices (`SourceVoiceSlot`), spatialisation (ADR-027) and device handling;
  - layer 1, **effects**: XAPO chains on buses (ADR-024).
- **Threading.** The game thread only enqueues commands; the audio engine applies them on its own update and in XAudio2 callbacks that never block (ADR-026).
- **Device changes and errors.** `IXAudio2EngineCallback::OnCriticalError` and `XAUDIO2_E_DEVICE_INVALIDATED` cause the voices and the graph to be rebuilt on the new default device, with playing instances resubmitted from their stored play positions, so loops and one-shots continue where they were. Device changes are also tested while the game is paused.
- **Isolation.** Audio code never writes simulation state and never draws from the simulation random generator; it uses the presentation random generator (`FxRng`), through its own instance, shared with no other system; its draws choose sample-group members, random emitters and ranged-random parameters (DEC-013). The dedicated server target does not link the audio engine.
- **Development tools.** A development overlay shows real and virtual voices, bus levels and memory per category; definitions reload while the game runs in Debug builds.

### Consequences

- Good, because the voice graph, floating-point mixing, sample-rate conversion, effects and spatial calculations come from the OS, with no redistributable.
- Good, because the engine follows the default device and the user's spatial-sound format without Species code for either.
- Good, because the five-layer plan keeps the data-driven binding model independent of the API below it.
- Bad, because XAudio2 streams nothing itself and decodes only its own formats, so streaming is Species code and the stream format is one that XAudio2 decodes (ADR-025).
- Bad, because the graph's processing quantum (10 ms on Windows) sets a floor on start latency, which the command timing of ADR-026 must respect.

### Confirmation

- A device test switches the default output device and unplugs it during playback, while paused and while running, and checks that playback continues on the new device without a restart.
- A link check confirms that the dedicated server contains no audio symbols and that the client imports only `xaudio2_9.dll` for audio.
- A code check fails when audio code includes the simulation random generator's header or a simulation-state header with write access.

## Pros and Cons of the Options

### 1. XAudio2 2.9 with X3DAudio

- Good, because it is in-box, maintained and designed for games.
- Good, because submix voices map directly onto Species buses.
- Bad, because streaming and non-native codecs need Species code.

### 2. Own mixer on WASAPI

- Good, because every stage of mixing and resampling is under Species' control, and latency can be lower.
- Bad, because the mixer, resampler, gain ramps, limiter, effects and device handling all have to be written and tested, for no audible gain in this game.

### 3. Commercial audio middleware

- Good, because it brings authoring tools, profiling and a mature runtime.
- Bad, because Species' binding model is already data-driven and small; a middleware's own event model would replace it or duplicate it, and adds licence cost.
- Bad, because `AGENTS.md` R14 excludes third-party code from the executable.

## More Information

- Technology baseline item 7 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-versions · https://learn.microsoft.com/uwp/win32-and-com/win32-apis#apis-from-xaudio2_9dll · https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-redistributable · https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-audio-graph.
- Related: ADR-023 (virtual voices), ADR-024 (buses and effects), ADR-025 (codecs and residency), ADR-026 (command queue and callbacks), ADR-027 (spatialisation).
- The audio engine component (`architecture/components/audio_engine/`) specifies each layer; the audio design (`reference/10_audio_design/`) specifies the binding model and the sound data.
- **Amendment 2026-10-17 (AGENTS.md, O21).** Definitions reload in Debug builds only; commercial audio middleware is excluded by `AGENTS.md` R14.
- **Amendment 2026-10-17.** The layer names and numbers follow the audio engine component (AEN-002): `SoundVoice` is the sound instance of layer 4, as CONVENTIONS §8 names it, and the pooled source voice of layer 2 is `SourceVoiceSlot`.
- **Amendment 2026-10-17 (ADR-025, AEN-002).** Layer 3 holds `SampleCache`, `SampleBank` and `AudioStream` and no decoder type: resident sounds are 16-bit PCM and streamed sounds MS-ADPCM, and XAudio2 decodes both (ADR-025), so the executable has no decoder of its own. The consequence on streaming is restated to match: Species code streams the encoded blocks, XAudio2 decodes them.
- **Amendment 2026-10-23 (M3 task T15, [ADR-172](ADR-172-the-audio-engine-at-the-mvps-size.md)).** Layers 2 and 4 are built at the MVP's size: the device interface `AudioDevice` with its XAudio2 implementation and a silent device, and `AudioEngine` with one-shot events of one sample group. Layer 3 is the resident sounds of ADR-171, and layers 1 and 5 (definitions and the music channel) are not built at the MVP. The client links `xaudio2.lib` from the Windows SDK, and the dedicated server links no audio code.
