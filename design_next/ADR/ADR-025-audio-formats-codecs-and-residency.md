---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Audio source formats, runtime codecs and residency

## Context and Problem Statement

Species has two kinds of sound data. **Short, frequent sounds** (unit barks, shots, impacts, deaths, UI clicks) play many times a second in a battle and must start without delay. **Long sounds** (music, ambience beds, long loops) play once at a time and are too large to hold decoded in memory. Positional sounds are mono; music, UI and some ambience beds are stereo. Every sound is newly authored (vision brief §5). XAudio2 plays 16-bit and 32-bit-float PCM and its own MS-ADPCM natively; any other codec is decoded by Species code before the samples reach a source voice. The current distribution of the MS-ADPCM encoder tool could not be confirmed.

The problem is which formats sounds are authored in, which formats the runtime plays, which sounds stay resident and which are streamed, and where decoding happens.

## Decision Drivers

- Zero decode cost and no start delay for short, frequent sounds.
- Bounded memory: long sounds are never decoded whole into memory.
- Decoding never runs on the game thread.
- Mono and stereo supported everywhere.
- One sample rate end to end, with conversion only at the mastering voice.
- Every encoder used in the content build is available and maintained.
- Sample-accurate loop points and seeking for long sounds.

## Considered Options

1. **Resident 16-bit PCM** for short sounds and **streamed Opus** for long sounds, with MS-ADPCM as a later option for large effect banks.
2. **Resident MS-ADPCM** for every short sound and streamed Opus for long sounds.
3. **PCM everywhere**, resident and streamed.
4. **Vorbis** for long sounds (and optionally short ones), decoded in software.
5. **xWMA** for long sounds, decoded by XAudio2.

## Decision Outcome

Proposed option: **1, resident 16-bit PCM and streamed Opus**. **Default while Proposed:** the content build and the audio engine use these formats, because they give short sounds zero decode cost and long sounds a maintained, open codec, with no dependency on an encoder whose distribution is unconfirmed.

- **Source format.** Uncompressed WAV at **48 kHz**, 24-bit integer or 32-bit float PCM; mono for every positional sound, stereo for music, UI and stereo ambience beds. The content build resamples nothing: a source at another rate is rejected.
- **Residency by group.** The content build classifies each sample group as a whole: a group is **streamed** when its longest member is longer than the streaming threshold (an audio engine tuning value, in seconds), and every music group is streamed; every other group is **resident**.
- **Resident sounds.** Resident groups are cooked to **16-bit PCM at 48 kHz** and held in memory banks owned by the asset system (`SampleCache`). There are two bank scopes: the **interface** bank (interface sounds, the lobby heartbeat and the heads of the ambience streams and the front-end music stream), loaded at start-up and kept for the process lifetime, and the **match** bank (every other resident group), loaded with each match and released when the match world is torn down. A group named by both banks is loaded once and counted by reference. Nothing in a bank is evicted while its scope lasts. Source voices play resident sounds directly from the bank.
- **Triggers before residency.** A trigger whose group is not yet resident is dropped and counted, never deferred, so a sound never starts late.
- **Streamed sounds.** Streamed groups are cooked to **Opus** (libopus 1.5.2 through vcpkg) at 48 kHz, stored in a Species stream format inside the pack files: Opus packets plus a seek table and the loop points, so seeking and looping are sample-accurate. At run time the audio engine's decoder (`AudioDecoder`) decodes fixed-size blocks into a small ring of PCM buffers per playing stream and submits them to its source voice as `OnBufferEnd` frees them. A stream never allocates its whole decoded length.
- **Decoding thread.** Decoding runs on a decode task owned by the audio engine, never on the game thread and never inside an XAudio2 callback; callbacks only signal it (ADR-026). Which worker threads run the task follows the threading model, which is deferred to the design phase.
- **Budgets.** The audio engine component sets a hard memory budget per bank and one for every stream buffer and stream head together, and the number and size of stream buffers. The baseline budgets are **192 MiB** for the match bank, **8 MiB** for the interface bank and **8 MiB** for streams; the resident groups of the current asset set need about 173 MiB in 16-bit mono at 48 kHz. Exceeding a budget is a content-build error naming the bank.
- **MS-ADPCM.** MS-ADPCM for large resident effect banks is adopted only if memory budgets require it **and** the AdpcmEncode tool's current distribution is confirmed; it then needs a revision of this ADR. At the baseline budgets it is not needed.
- **Not used.** xWMA is not used. Vorbis is not used.

### Consequences

- Good, because short sounds start immediately with no decode work.
- Good, because long sounds cost only their stream buffers in memory.
- Good, because Opus is an open, maintained codec that runs natively at 48 kHz.
- Good, because every encoder in the content build is current and obtainable through vcpkg.
- Bad, because 16-bit PCM is the largest resident format; large effect libraries cost more memory than with ADPCM.
- Bad, because the Opus decoder, the stream format and the stream buffer management are Species code.

### Confirmation

- The content-build validator rejects audio sources that are not 48 kHz WAV, positional sounds that are not mono, resident groups whose longest member exceeds the streaming threshold, and banks whose resident audio exceeds their budget.
- A residency test triggers sounds of a bank before and after it becomes resident and checks that the early triggers are dropped and counted, never played late.
- A streaming test plays every long sound with seeks and loops and checks for gaps, glitches and sample-accurate loop points against the source.
- A profiling test checks that no decoding happens on the game thread during a full match.

## Pros and Cons of the Options

### 1. Resident PCM and streamed Opus

- Good, because it uses the best format for each kind of sound.
- Bad, because resident memory is larger than with a compressed resident format.

### 2. Resident MS-ADPCM and streamed Opus

- Good, because resident memory is up to four times smaller, with decoding done by XAudio2.
- Bad, because the encoder tool's distribution is not confirmed, and ADPCM's quality loss is audible on some bright effects.

### 3. PCM everywhere

- Good, because there is no decoder at all.
- Bad, because music and ambience beds streamed as PCM cost far more disk space and read bandwidth.

### 4. Vorbis

- Good, because it is open, mature and widely supported.
- Bad, because Opus gives better quality per bit, has a current release, and is the IETF standard, so a second open codec adds nothing.

### 5. xWMA

- Good, because XAudio2 decodes it natively.
- Bad, because the encoder tool's current distribution could not be confirmed, and the format is tied to one vendor's tools.

## More Information

- Technology baseline item 8.3 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: https://learn.microsoft.com/windows/win32/api/xaudio2/nf-xaudio2-ixaudio2-createsourcevoice · https://learn.microsoft.com/windows/win32/xaudio2/adpcm-overview · https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-versions · https://github.com/xiph/opus/releases.
- The AdpcmEncode tool's distribution was re-checked on 2026-10-17 against https://learn.microsoft.com/windows/win32/xaudio2/adpcm-overview, which names the tool but not where it is obtained; it remains unverified.
- Related: ADR-022 (audio engine), ADR-023 (voice pools by format), ADR-026 (callbacks and decode signalling), ADR-058 (pack files), ADR-059 (content build).
- **Amendment 2026-10-17.** The record states the rules and budgets of the audio engine component: residency classified per group by its longest member, music always streamed (AEN-072); the interface and match bank scopes with reference-counted groups (AEN-074, AEN-243 to AEN-245); the budgets `aen.memory.match_bank_budget` (192 MiB), `aen.memory.interface_bank_budget` (8 MiB) and `aen.memory.stream_budget` (8 MiB); and the drop rule for triggers that come before residency (AEN-111, AEN-247). The reference engine's evicting sample cache is a reference model that the audio engine component maps onto these banks (AEN-240 to AEN-246); it is not part of the Species engine. The status stays Proposed because the runtime codec remains a choice; the default is unchanged.
