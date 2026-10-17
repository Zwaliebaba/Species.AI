---
status: accepted
date: 2026-10-17
decision-makers: Species technology lead
---

# Spatialisation and distance attenuation

## Context and Problem Statement

Positional sounds in Species come from units, buildings and effects spread across the battlefield, heard from the camera. The audio design (`reference/10_audio_design/`) fixes the laws: a sound plays at full level up to its minimum distance and then falls off as minimum distance divided by distance; stereo panning is constant-power, and non-positional (2-D) sounds sit at the centred value. A sound with no maximum distance keeps contributing faintly at any range, which wastes voices on sounds nobody can hear, so Species adds a maximum distance beyond which a sound is silent. Players may use stereo speakers, headphones, multichannel systems or a system spatial-sound format such as Windows Sonic, which XAudio2 2.9 follows automatically through the mastering voice.

The problem is how positional sounds are attenuated, panned and filtered, and which parts of that the engine computes itself.

## Decision Drivers

- The distance law and the stereo panning law of the audio design are reproduced exactly.
- A maximum distance without an audible step at the edge.
- Correct output on stereo, multichannel and system spatial-sound formats.
- Non-positional sounds are never spatialised.
- Every real positional voice is updated every audio update, with no movement threshold.
- Doppler, distance filtering and distance-dependent reverb are available per event without changing the default mix.

## Considered Options

1. **Species computes the distance gain and the stereo pan** from the audio design's laws, and **X3DAudio** supplies multichannel matrices, Doppler factors and filter coefficients.
2. **X3DAudio defaults** for every calculation, including its own distance curve and panning.
3. **Spatial audio objects** through the Windows spatial-audio API (`ISpatialAudioClient`) or an HRTF effect.

## Decision Outcome

Chosen option: **1, Species distance gain and stereo pan, with X3DAudio for the rest**, because it reproduces the specified laws exactly on stereo output and still uses X3DAudio where the output layout needs it.

- **Distance gain.** For each real positional voice the engine computes the gain from the distance *d* between emitter and listener, in world units: **1** for *d* ≤ *d*min; ***d*min / *d*** for *d*min < *d* < *d*max; **0** for *d* ≥ *d*max. Over the last part of the range before *d*max (a fade length that is an audio engine tuning value) the gain is faded linearly to zero, so the edge is not audible. *d*min comes from the sound's definition; *d*max is a per-event value with a global default (audio engine tuning). The global default follows DEC-566: it lies beyond every distance in a map (10⁹ wu), so the default mix has no maximum distance and the gain is exactly *d*min / *d* beyond *d*min; a finite per-event *d*max is the mechanism of DEC-566's alternative. The gain feeds audibility for ranking (ADR-023) and the voice volume.
- **Stereo output.** When the mastering voice has two channels, the engine computes the **constant-power pan** of the audio design from the emitter's direction relative to the listener, and sets the output matrix itself.
- **Other layouts.** For more than two channels, X3DAudio's `X3DAudioCalculate` computes the output matrix, with a constant volume curve so that it carries panning only; the engine's distance gain is applied as the voice volume.
- **Doppler, filtering and reverb.** X3DAudio also computes the Doppler factor (applied to the frequency ratio), the direct-path low-pass coefficient (applied through the per-voice filter) and the reverb send level. Each is scaled by a per-event factor whose default is **0** (off), so the default mix matches the audio design; events that the design gives Doppler, distance filtering or distance reverb set their factor.
- **Non-positional sounds.** 2-D sounds (UI, music, head-relative ambience) bypass spatialisation and play at the centred pan.
- **Updates.** Every real positional voice is recalculated at every audio update (ADR-026), whatever its movement.
- **Listener.** The listener follows the camera's position and orientation, sent as a command each frame; a command with a zero-length front or up vector is rejected and the previous orientation is kept.
- **Not used.** The HRTF effect (`CreateHrtfApo`) and direct use of `ISpatialAudioClient` are not used; the system spatial-sound format applies through the mastering voice.

### Consequences

- Good, because the distance and stereo panning laws are exact, not approximated by a curve.
- Good, because sounds beyond the maximum distance cost no voice, and the edge has no audible step.
- Good, because multichannel and system spatial-sound output work without Species code per format.
- Good, because 2-D sounds can never be heard at a stale position.
- Bad, because the engine maintains two panning paths (stereo law, X3DAudio matrix), which must be tested separately.
- Bad, because the maximum distance and fade length are new tuning values that the audio design must balance.

### Confirmation

- Unit tests check the distance gain at *d*min, at points between *d*min and the fade start, inside the fade and at *d*max, and the stereo pan against the audio design's formula at a set of angles, including 2-D sounds at the centre.
- A test with a four-channel and a six-channel mastering voice checks that X3DAudio's matrices carry no distance attenuation of their own.
- A test checks that with all per-event factors at their defaults, Doppler, filter and send values are neutral.

## Pros and Cons of the Options

### 1. Species gain and stereo pan, X3DAudio for the rest

- Good, because the specified laws are exact.
- Bad, because there are two panning paths.

### 2. X3DAudio defaults throughout

- Good, because it needs the least Species code.
- Bad, because its default curve and panning are not guaranteed to equal the audio design's laws, and matching them with curve points is approximate.

### 3. Spatial audio objects or HRTF

- Good, because headphone listeners get stronger 3-D cues.
- Bad, because it changes how the mix sounds compared with the audio design, adds an object budget to manage, and the system spatial-sound format already applies through the mastering voice.

## More Information

- Technology baseline item 7 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-versions · https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-redistributable.
- Related: ADR-022 (audio engine), ADR-023 (audibility for ranking), ADR-024 (per-voice filter and reverb sends), ADR-026 (update rate and listener commands).
- The audio design (`reference/10_audio_design/`) fixes the distance and panning laws and each sound's minimum distance.
- **Amendment 2026-10-17.** The default of the per-event maximum distance follows DEC-566 (`aen.spatial.max_distance_default`, 10⁹ wu; AEN-202, R-AUD-202), so the default mix keeps the reference law with no maximum distance.
