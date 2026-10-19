---
status: proposed
date: 2026-10-17
decision-makers: Species technology lead
---

# Audio source formats, runtime formats and residency: resident 16-bit PCM and streamed MS-ADPCM, decoded by XAudio2

## Context and Problem Statement

Species has two kinds of sound data. **Short, frequent sounds** (unit barks, shots, impacts, deaths, UI clicks) play many times a second in a battle and must start without delay. **Long sounds** (music, ambience beds, long loops) play once at a time and are too large to hold decoded in memory. Positional sounds are mono; music, UI and some ambience beds are stereo. Every sound is newly authored (vision brief §5). The executable may contain no third-party codec (`AGENTS.md` R14), so the runtime formats are those the in-box XAudio 2.9 decodes itself: 16-bit and 32-bit-float PCM, MS-ADPCM and xWMA. The current distribution of Microsoft's ADPCM and xWMA encoder tools could not be confirmed.

The problem is which formats sounds are authored in, which formats the runtime plays, which sounds stay resident and which are streamed, and who encodes and decodes them.

## Decision Drivers

- Zero decode cost and no start delay for short, frequent sounds.
- Bounded memory: long sounds are never decoded whole into memory.
- No third-party codec in the executable; decoding by XAudio2 itself.
- Mono and stereo supported everywhere.
- One sample rate end to end, with conversion only at the mastering voice.
- Every encoder used in the content build is available, or is Species code in the content build tool.
- Loop points and seeking for long sounds at a known resolution.

## Considered Options

1. **Resident 16-bit PCM** for short sounds and **streamed MS-ADPCM** for long sounds, both decoded by XAudio2.
2. **Resident MS-ADPCM** for every short sound and streamed MS-ADPCM for long sounds.
3. **PCM everywhere**, resident and streamed.
4. **xWMA** for long sounds, decoded by XAudio2.
5. A **third-party codec** (Opus or Vorbis) for long sounds, decoded by Species code.

## Decision Outcome

Proposed option: **1, resident 16-bit PCM and streamed MS-ADPCM**. **Default while Proposed:** the content build and the audio engine use these formats, because they give short sounds zero decode cost and long sounds a fourfold size reduction, decoded by XAudio2 with no codec in the executable. **Confirmed when:** the sounds of the first playable (O20) are cooked into these formats within their residency budgets and pass the residency, streaming, encoder and profiling tests of the Confirmation section; it is checked before the audio cooker writes its first bank, because the cooker and the stream format depend on it.

- **Source format.** Uncompressed WAV at **48 kHz**, 24-bit integer or 32-bit float PCM; mono for every positional sound, stereo for music, UI and stereo ambience beds. The content build resamples nothing: a source at another rate is rejected.
- **Residency by group.** The content build classifies each sample group as a whole: a group is **streamed** when its longest member is longer than the streaming threshold (an audio engine tuning value, in seconds), and every music group is streamed; every other group is **resident**.
- **Resident sounds.** Resident groups are cooked to **16-bit PCM at 48 kHz** and held in memory banks owned by the asset system (`SampleCache`). There are two bank scopes: the **interface** bank (interface sounds, the lobby heartbeat and the heads of the ambience streams and the front-end music stream), loaded at start-up and kept for the process lifetime, and the **match** bank (every other resident group), loaded with each match and released when the match world is torn down. A group named by both banks is loaded once and counted by reference. Nothing in a bank is evicted while its scope lasts. Source voices play resident sounds directly from the bank.
- **Triggers before residency.** A trigger whose group is not yet resident is dropped and counted, never deferred, so a sound never starts late.
- **Streamed sounds.** Streamed groups are cooked to **MS-ADPCM** at 48 kHz (`ADPCMWAVEFORMAT`), stored in a Species stream format inside the pack files: the format block, the loop points and the ADPCM data. XAudio2 decodes ADPCM itself, so the audio engine streams the encoded blocks from the packs into a small ring of buffers per playing stream and submits them to the stream's source voice as `OnBufferEnd` frees them; a stream never holds its whole length in memory. Seeking and looping work on whole ADPCM blocks: the samples per block (32 to 512) are chosen per group, 512 by default, and a looping group uses a block size on which its loop points fall; the content build rejects a loop point that is not on a block boundary.
- **Encoder.** The content build tool encodes MS-ADPCM itself, because the distribution of Microsoft's ADPCM encoder tool is not confirmed; the encoder is development-tool code and never ships ([ADR-055](ADR-055-dependencies-three-nuget-packages.md)).
- **Streaming thread.** Reading stream blocks from the packs runs on an I/O task owned by the audio engine, never on the game thread and never inside an XAudio2 callback; callbacks only signal it (ADR-026). Decoding runs inside XAudio2. Which worker threads run the task follows the threading model, which is deferred to the design phase.
- **Budgets.** The audio engine component sets a hard memory budget per bank and one for every stream buffer and stream head together, and the number and size of stream buffers. The baseline budgets are **192 MiB** for the match bank (`aen.memory.match_bank_budget`), **8 MiB** for the interface bank (`aen.memory.interface_bank_budget`) and **8 MiB** for streams (`aen.memory.stream_budget`); the content build checks the authored set against them, and exceeding a budget is a content-build error naming the bank.
- **Resident MS-ADPCM.** MS-ADPCM for large resident effect banks is adopted only if memory budgets require it, by a revision of this ADR. At the baseline budgets it is not needed.
- **Not used.** xWMA (its encoder's distribution is not confirmed) and every third-party codec (`AGENTS.md` R14).

### Consequences

- Good, because short sounds start immediately with no decode work.
- Good, because long sounds cost only their stream buffers in memory and about a quarter of their PCM size on disk, and XAudio2 decodes them with no Species decoder.
- Good, because the executable contains no codec.
- Bad, because 16-bit PCM is the largest resident format; large effect libraries cost more memory than with ADPCM.
- Bad, because MS-ADPCM is lossy at a fixed ratio; its artefacts can be audible on bright music, which a listening test must accept.
- Bad, because loop points and seeking are limited to block boundaries, and the ADPCM encoder is Species code with its own tests.

### Confirmation

- The content-build validator rejects audio sources that are not 48 kHz WAV, positional sounds that are not mono, resident groups whose longest member exceeds the streaming threshold, banks whose resident audio exceeds their budget, and looping streamed groups whose loop points are not on block boundaries.
- A residency test triggers sounds of a bank before and after it becomes resident and checks that the early triggers are dropped and counted, never played late.
- A streaming test plays every long sound with seeks and loops and checks for gaps and glitches and for the loop points against the source.
- An encoder test round-trips reference signals through the Species ADPCM encoder and XAudio2's decoder and checks the error against a bound.
- A profiling test checks that no stream read happens on the game thread during a full match.

## Pros and Cons of the Options

### 1. Resident PCM and streamed MS-ADPCM

- Good, because both formats are decoded by XAudio2, and each kind of sound gets a fitting format.
- Bad, because MS-ADPCM's quality is below that of a modern perceptual codec.

### 2. Resident MS-ADPCM and streamed MS-ADPCM

- Good, because resident memory is up to four times smaller.
- Bad, because ADPCM's quality loss is audible on some bright effects, and the memory is not needed at the baseline budgets.

### 3. PCM everywhere

- Good, because there is no encoder at all.
- Bad, because music and ambience beds streamed as PCM cost four times the disk space and read bandwidth of ADPCM.

### 4. xWMA

- Good, because XAudio2 decodes it natively, at a better quality per bit than ADPCM.
- Bad, because the encoder tool's current distribution could not be confirmed, and the format is tied to one vendor's tools.

### 5. A third-party codec

- Good, because Opus or Vorbis gives the best quality per bit.
- Bad, because `AGENTS.md` R14 excludes third-party code from the executable.

## More Information

- Technology baseline item 8.3 (see [the index](README.md#technology-baseline)); sources checked 2026-10-17: XAudio2 source voices accept PCM, IEEE float and MS-ADPCM formats (https://learn.microsoft.com/windows/win32/api/xaudio2/nf-xaudio2-ixaudio2-createsourcevoice); XAudio 2.9 supports 16-bit PCM, 32-bit float PCM, 16-bit ADPCM and xWMA on PC (https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-redistributable · https://learn.microsoft.com/windows/win32/xaudio2/xaudio2-versions); ADPCM in XAudio2 compresses up to 4:1 with 32 to 512 samples per block, and XAudio2 decodes it in software (https://learn.microsoft.com/windows/win32/xaudio2/adpcm-overview).
- The ADPCM overview names the AdpcmEncode tool but not where it is obtained; its distribution remains unverified, which is why the content build encodes ADPCM itself.
- Related: ADR-022 (audio engine), ADR-023 (voice pools by format), ADR-026 (callbacks and stream signalling), ADR-058 (pack files), ADR-059 (content build).
- **Amendment 2026-10-17.** The record states the rules and budgets of the audio engine component: residency classified per group by its longest member, music always streamed (AEN-072); the interface and match bank scopes with reference-counted groups (AEN-074, AEN-243 to AEN-245); the budgets `aen.memory.match_bank_budget` (192 MiB), `aen.memory.interface_bank_budget` (8 MiB) and `aen.memory.stream_budget` (8 MiB); and the drop rule for triggers that come before residency (AEN-111, AEN-247).
- **Amendment 2026-10-17 (AGENTS.md, O21).** No codec is compiled into the executable: streamed sounds are MS-ADPCM, decoded by XAudio2 2.9 and encoded by the content build tool. The status stays Proposed because the runtime formats remain a choice among the in-box ones; the default is resident 16-bit PCM and streamed MS-ADPCM.
- **Amendment 2026-10-18 (owner decision O55).** Since O55 the first playable of the "Confirmed when" line is the MVP's M3 build ([MVP scope](../implementation/mvp_scope.md) §3, §5.3), not the skirmish of O20, and the evidence is gathered on that build: the sounds are the first sound set of MVP-087, which M3 SHOULD play, and the profiling test runs over a full session of a run in place of a full match; if the M3 build has no sound set, the confirmation waits for the first build that has one.
