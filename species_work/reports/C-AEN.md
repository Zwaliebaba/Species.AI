# C-AEN report: audio engine component

## Files written

| File | Size | Notes |
|---|---|---|
| `design_next/architecture/components/audio_engine.md` | 116.9 KB (about 117 000 bytes) | one file, component template §6.4; **no split**: it is under 120 KB, so the many inbound links to `audio_engine.md` (G10, R-META, ADRs, DECs) stay valid. It is above the package budget of 105.5 KB. |
| `species_work/scripts/C-AEN_check.py` | | self-check (ids, keys, DEC/ADR/asset ids, Open DECs cited, R-* ids, headings, truncation, links, captions, size) |
| `species_work/scripts/C-AEN_dec_extract.py`, `C-AEN_audio_sizes.py`, `C-AEN_audio_cats.py` | | DEC extraction; manifest audio durations and resident sizes |

Checks: forbidden scan 0 hits; soft scan 0 hits; `C-AEN_check.py` 0 problems; the 4 Mermaid blocks parse with mermaid 11.4.1 (`G06_mermaid.mjs`).

## Counts

- Requirements: 164 (AEN-001 to AEN-362, ascending, unique, in range).
- Tuning keys: 85 (`aen.*`, each defined once and cited by a requirement; no `ref.*` key defined).
- Diagrams: 4 (update steps flowchart, instance states, envelope stages, data flow). The reference class diagram became the type table of AEN-060; I removed my Species class diagram to stay under 120 KB.
- Enumerations: 12 C++ enumerations in the interface sketch; 17 tables (layers, commands, back-end mapping, types, layer engine fields, 14 effect kinds, banks, settings, reference update steps, envelope, cache, residency mapping, tuning table, failures, tests, decisions).
- Tests: 30 (T1 to T30).

## Open DECs cited (by requirements)

DEC-044 (AEN-215), DEC-049 (AEN-360), DEC-109 (AEN-157), DEC-221 (AEN-106), DEC-222 (AEN-003, AEN-040), DEC-224 (AEN-112, AEN-116, AEN-301), DEC-226 (AEN-102), DEC-318 (AEN-040), DEC-320 (AEN-197), DEC-321 (AEN-185), DEC-322 (AEN-191), DEC-566 (AEN-202, AEN-209), DEC-567 (AEN-071, AEN-221, AEN-222). Resolved ones cited as rules: DEC-013, DEC-027, DEC-042, DEC-097, DEC-323, DEC-568, DEC-569, DEC-570, DEC-751, DEC-752, DEC-801. DEC-339 (Dropped) is cited in AEN-230 and §8.1. I also cite DEC-004, DEC-007, DEC-015, DEC-100.

## Known items

### Resident sample banks against the reference cache (ADR-0025)

- **Reference model** (AEN-240, AEN-241): one cache keyed by sample name; whole decoded buffer allocated at creation; incremental decode on first play on the mixing thread; mono only; a lock per playing handle; every frame, least-recently-used eviction of unlocked samples above 60 MiB, which samples in use can exceed.
- **Mapped onto ADR-0025** (AEN-243 to AEN-247, mapping table AEN-246): groups classified at content build (resident if the longest member is at most 20 s, music always streamed); two banks, **interface** (UI groups, lobby heartbeat and the ambience stream heads; process lifetime) and **match** (every other resident group; loaded with each match, released at teardown); reference-counted groups; hard budgets checked by the content build; triggers before their bank is resident are dropped (AEN-111).
- **Measured** from the manifest: the resident groups hold about 1 888 s in 868 files, about 173 MiB of 16-bit mono at 48 kHz (interface about 2 MiB; stereo stings would add about 14 MiB). So the match bank is about 3 times the reference's 60 MiB cache, which held about one eighth of the reference library.
- **What should change:** the reference cache should **not** be carried. ADR-0025 stays the model. It should be amended, and could then be Accepted:
  1. the streaming threshold applies to a group's longest member;
  2. state the bank scopes and their reference counting;
  3. state the budgets set here: match bank 192 MiB, interface bank 8 MiB, streams 8 MiB;
  4. state the drop rule for triggers that come before residency.

  At these budgets MS-ADPCM is not needed.

### DSP defaults (X-DEC-5c extra fact 3)

The schema default of every effect parameter is its **declared default**, not 0.0 (AEN-071, cites DEC-567, R-AUD-078). The reasons:
- 21 of the 63 parameters have ranges that exclude 0.0;
- a bit depth of 0 or an echo delay of 0 is unusable;
- every reference binding (FX-01 to FX-15) states all its parameters;
- no reference data is imported;
- effects are off by default.

## ADR conflicts (settled in the spec; the ADR text should change)

1. **ADR-0024 against DEC-570:** the baseline graph has the six player buses of DEC-570 under a pre-master bus (AEN-220). ADR-0024's four-bus text should say six. It should also list the XAudio2 I3DL2 reverb (`XAudio2CreateReverb` with `ReverbConvertI3DL2ToNative`), which I use for the room-reverb kind (AEN-070).
2. **ADR-0027 against DEC-566:** the per-event maximum distance exists. Its global default is 10⁹ wu, the reference's effective maximum, so the default mix has no maximum distance (AEN-202). ADR-0027 should say that its default follows DEC-566.
3. **ADR-0023, ownership of the copy decay:** the copy decay and the patience window are **design values** per event, with defaults `ref.aud.rank.copy_decay` and `ref.aud.layer.one_shot_patience_updates` (G10). The engine owns the arithmetic, budget, caps, margin and hold (AEN-063). ADR-0023's sentence "the factors are audio engine tuning values" should change.
4. **ADR-0022, names:** ADR-0022 puts `SoundVoice` in the voices-and-device layer. The ledger maps the reference instance type to `SoundVoice`. I follow the ledger (instances) and name the pooled source voice `SourceVoiceSlot`. ADR-0022 should change its parenthesis.
5. **ADR-0026 (clarification):** a command's issue time is the real-time audio clock (platform clock), not the game-speed-scaled presentation clock (AEN-024).

## New DEC requests

1. **Note re-trigger timing** (AEN-178): the next note starts at the first member wrap after the loop delay, not exactly when the delay ends. Default: the reference behaviour. Option: start exactly at the delay.
2. **Audibility during an attack** (AEN-140): an instance that has been real ranks by its current envelope level, so a slow attack ranks low and can be stolen. Option: rank by the level target during the attack.
3. **Shared note layers** (AEN-133): a duplicate merges into its own note, so no next note plays. Confirm that no Species layer combines `note` with shared sharing, or make the content build reject it.
4. **Audio during pause and suspension** (AEN-138, §8.3 item 7): the reference never pauses audio. Default: no pause.

## Missing assets

None. The groups that G10 R-AUD-413 needs are G10's request.

## Cross-document requests

- **G10:**
  - **AEN-140:** R-AUD-215 "while it plays / while it waits" is read as "has ever been real / has not". Please confirm.
  - **AEN-166:** R-AUD-056 says "from silence". The exact start is the floor of −50 dB.
  - **AEN-146:** patience is counted from the start time (trigger time + 0.03 s).
  - **AEN-062:** the per-event engine fields (copy decay, patience window, event cap, maximum distance, Doppler, filter and reverb factors) are event data. Please list them, or point to C-DAT.
- **C-DAT:** please add to the sound-event schema:
  - the engine fields of AEN-062;
  - effect-parameter schema defaults equal to the declared defaults (AEN-071);
  - integer parameters truncated toward zero.

  Please also add the cooked stream format (AEN-072) and the audio settings entries (AEN-076).
- **G09b:**
  - The audio settings are now keyed: `aen.settings.output_default`, `volume_default`, `voices_default`, `mix_rate_reader_default` (22 050 Hz), `output_mix_rate_reader_default` (44 100 Hz), `output_buffer_first_run` (512), `output_buffer_reader_default` (2 000), `swap_stereo_default`, `sample_memory_level_default`, `effects_default`.
  - R-META-431 and §5 item 1 say "44 100 Hz at the audio engine's readers". The engine's own reader defaults to 22 050 Hz; 44 100 Hz is the reader of the secondary output path.
  - R-META-454 item 1 keeps the sample-memory level "honoured". Species ignores it (R-AUD-065, AEN-077).
  - The restart rule: AEN-260 is the reference model; AEN-259 and AEN-261 give the Species continuation.
- **C-AST:**
  - asynchronous bank loads from pack files (AEN-244);
  - the content-build steps:
    - group classification by the longest member;
    - bank composition and budget validation;
    - Opus stream cooking with seek tables and loop points;
    - rejection of zero-length, non-48 kHz and positioned stereo members.
- **X-AST:**
  - GameAssets §13 and §15 say "Opus stream for files over 20 s", but the manifest applies the rule per group: all Abductor presence-loop members stream, some of them 15.1 s long. Please word it per group (AEN-072).
  - The resident total is about 173 MiB.
- **C-PLT:** the audio clock is the platform clock (ADR-0074). A device-arrival notice should reach the engine (AEN-259). The audio settings live in the settings store.
- **C-DBG:**
  - the development overlay contents (AEN-323);
  - the profiling markers and per-event cost counters (AEN-322);
  - performance test T27 with `aen.perf.update_cpu_budget`.
- **C-SIM-c / coordinator:** presentation must build `EmitterUpdate` records from the snapshot, including the teams-nearby and unit-count inputs (AEN-041). R-SIM-581 step 13 ("audio engine") now only sends commands. The producer of these records belongs to the DEC-222 boundary.
- **X-ADR:** the ADR text changes listed under ADR conflicts.
- **X-GLO:** please add these terms:
  - real and virtual instance, voice pool, steal margin, state hold;
  - effect bus, send and insert routing;
  - stream head, interface bank, match bank;
  - audio clock, start time, silent device.

  X-GLO-2 asked for the "sample cache" and "LRU" entries to be updated: the reference rule is AEN-241 and the Species rule AEN-243.

## Names not in the ledger (proposals)

- **Types:**
  - binding and instances: `SoundLayerDef`, `SoundGroupDef`, `SoundVoiceId`, `EffectBinding`;
  - sample data: `SampleBank`, `AudioStream`, `BankId`;
  - voices: `SourceVoiceSlot`, `VoiceBackend`, `AudioBusGraph`;
  - commands and results: `CommandWriter`, `TriggerCommand`, `StopCommand`, `EmitterUpdate`, `ListenerUpdate`, `MatchState`, `AudioResult`.
- **Enumerations:**
  - G10's proposals, adopted: `SoundPosition`, `SoundSharing`, `SoundLoop`, `SoundAudience`, `SoundParamKind`, `SoundParamUpdate`, `SoundLink`, `DspFilterKind`, `AudioBus`;
  - new: `EnvelopeStage`, `VoiceState`, `StopMatch`, `MatchPhase`.
- **Species XAPOs:** `ChorusFx`, `CompressorFx`, `DistortionFx`, `StereoEchoFx`, `FlangerFx`, `StandardGargleFx`, `StudioReverbFx`, `ResonantLowPassFx`, `BitCrusherFx`, `GargleFx`, `EchoFx`, `SimpleReverbFx`.

## Dropped as legacy (one line per group)

- **Device back-ends:** the interchangeable 3-D and stereo output libraries, the alternative mixer, the console back-end and the null-library classes. They are replaced by one XAudio2 voice back-end; DEC-339 covers the driver details.
- **Device buffering:** ring sizes, play-cursor polling and margins, channel health, buffer restore, the one-ring silence tail and its stale-counter case. They are device mechanics; Species submits buffers.
- **Hardware positioning:** the hardware positioning path, its retry and its error dialog. The setting has no Species equivalent.
- **The alternative mixer's internals:** its volume curve, pan law, resampler, block limiter and record-to-file. It is never the default path.
- **Old components:** archive names, loader-thread affinity masks, library and codec versions, the case-insensitive file look-up.
- **Development tooling:** the sound editor and its windows, the parameter editors, the statistics and profile windows, the effect processor, the editor position kind, definition propagation, the "modified definitions" quit check, the disabled verify step and the never-running options readout. They are development tools that 40 does not keep; §8.3 item 8 records them.
- **Campaign and other out-of-scope content:** the campaign-only sources (narrator, gesture and the other single-player source) and the world, editor, loader, start-sequence and credits loops.
- **Implementation details:** the sort-array size, the channel search seed, the initial priority value and the fixed local array of effect types. None has an observable effect.
