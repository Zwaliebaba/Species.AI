# Audio engine

> Status: Engine baseline · Prefix: AEN · ADRs: [ADR-022](../../../ADR/ADR-022-audio-engine-on-xaudio2.md) · [ADR-023](../../../ADR/ADR-023-virtual-voices-and-voice-stealing.md) · [ADR-024](../../../ADR/ADR-024-audio-buses-and-per-bus-effects.md) · [ADR-025](../../../ADR/ADR-025-audio-formats-codecs-and-residency.md) · [ADR-026](../../../ADR/ADR-026-audio-command-queue-and-callbacks.md) · [ADR-027](../../../ADR/ADR-027-spatialisation-and-distance-attenuation.md) · [ADR-016](../../../ADR/ADR-016-view-projection-and-depth-conventions.md) · [ADR-058](../../../ADR/ADR-058-pack-files-and-mount-policy.md) · [ADR-059](../../../ADR/ADR-059-content-build-and-validators.md) · [ADR-061](../../../ADR/ADR-061-data-schemas-and-versioning.md) · [ADR-067](../../../ADR/ADR-067-logging.md) · [ADR-068](../../../ADR/ADR-068-profiling-and-gpu-timing.md) · [ADR-069](../../../ADR/ADR-069-test-framework.md) · [ADR-074](../../../ADR/ADR-074-platform-clock-with-injectable-time.md) · Related: [audio design](../../../reference/10_audio_design/README.md) · [event model and sound data](../../../reference/10_audio_design/event_model_and_sound_data.md) · [event catalogue](../../../reference/10_audio_design/event_catalogue.md) · [positional, priorities and mix](../../../reference/10_audio_design/positional_priorities_and_mix.md) · [tick timing and main loop](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) · [randomness and integrity](../../../reference/12_simulation_model/07_randomness_and_integrity.md) · [options](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) · [asset pipeline and resources](../asset_pipeline_and_resources.md) · [data schemas](../data_schemas/README.md)

This component turns the sound events that game code raises into samples at the speakers: the binding of (source, event) pairs to layers and sample groups from data, the instances, the ranking onto a bounded set of real XAudio2 source voices with virtual voices and stealing, the buses, player volumes, limiter and effects, X3DAudio spatialisation, resident banks and streams, and the device. The [audio design](../../../reference/10_audio_design/README.md) states what plays; this component states how, precisely enough to implement and test. Where the Species engine differs from the reference engine, the reference behaviour is stated as the **reference model** next to the Species rule.

**Notation and units.**

- **Levels** are in dB relative to a voice's full level, 0 dB (DEC-042); an amplitude is 10^(dB ÷ 20), computed with `XAudio2DecibelsToAmplitudeRatio`. A level of the reference ten-step scale v converts to `ref.aud.level.reference_step_db` × (v − 10) dB ([audio design](../../../reference/10_audio_design/README.md) R-AUD-004). The **envelope floor** F = −10 × `ref.aud.level.reference_step_db` = −50 dB is the bottom of that scale: quiet, not silent.
- **Distances** are in world units (`wu`), measured in three dimensions in the right-handed, +Y-up world frame of ADR-016.
- **Audio time** is real time in seconds on the audio clock, which reads the platform's monotonic clock (ADR-074). Game speed and pausing never scale it. Audio durations are presentation durations; no simulation duration is ever measured in audio time.
- **Frames** are sample frames at `aen.format.sample_rate`.
- **Terms** follow the audio design ([audio design](../../../reference/10_audio_design/README.md) §2.1): a **sound source**, an **event** `<source>.<event>`, a **layer**, an **instance** (type `SoundVoice`), an **emitter**. An instance is **real** while a source voice mixes it and **virtual** while it is tracked and advanced without one. A **voice** is one pooled XAudio2 source voice.

## Folder index

| File | Holds | Requirement IDs |
|---|---|---|
| [README.md](README.md) (this file) | responsibilities and boundaries, the interface sketch, the commands and the threading contract, the inputs, the voice back-end contract, the data model, the lifecycle, the trigger gate, the update schedule, the data flow, the tuning table, budgets, failure modes, the acceptance tests and the open decisions | AEN-001 to AEN-123, AEN-264, AEN-265, AEN-300 to AEN-325, AEN-360 to AEN-362 |
| [instances_voices_and_envelopes.md](instances_voices_and_envelopes.md) | §4.5 to §4.13: from a command to an instance, ranking, virtual voices and stealing, the reference allocation and emitter selection, instance states, envelope arithmetic, play position and loops, start and advance, modulatable values, pitch and the game-speed bend | AEN-130 to AEN-199 |
| [spatialisation_mixing_and_streaming.md](spatialisation_mixing_and_streaming.md) | §4.14 to §4.19: spatialisation, the volume law and gain stages, buses, routing and effects, sample residency, streaming, device loss and restart | AEN-201 to AEN-261 |

Section numbers are those of the whole component: a section cited as §4.16 is in the file that the index names for it.

## 1 Responsibilities and boundaries

**AEN-001** (MUST) The audio engine (`AudioEngine`) owns: the sound data at run time (source, event, layer, sample-group and effect-kind definitions, §3); the command queues from game code and the result queue back to it (§2); the sound instances, their envelopes, loops, play positions and modulatable values (§4.8 to §4.13); the ranking of instances, the real-voice budget, the per-bus and per-event caps, virtualisation and stealing (§4.6); the spatialisation (§4.14); the bus graph, the player volumes, ducking, the limiter and the bus effects (§4.15, §4.16); the residency of sample data in resident banks and streams, and the stream task that reads streamed sounds from the packs for XAudio2 to decode (§4.17, §4.18); the XAudio2 device, its loss and its rebuilding (§4.19); and its diagnostics (§6).

**AEN-002** (MUST) The engine is built in five layers, each depending only on the layers below it (ADR-022):

| Layer | Contents | Species types |
|---|---|---|
| 5 binding | (source, event) look-up, audience filter, layer copy, shared-voice merge | `SoundSourceDef`, `SoundEventDef`, `SoundLayerDef`, `SoundGroupDef` |
| 4 instances | instances, envelopes, loops, play positions, modulatable values, ranking, virtual and real states | `SoundVoice`, `SoundVoiceId`, `SoundParam` |
| 3 sample data | resident banks, streams | `SampleCache`, `SampleBank`, `AudioStream` |
| 2 voices and device | the XAudio2 graph, pooled source voices, spatialisation, device loss | `VoiceBackend`, `SourceVoiceSlot`, `AudioBusGraph` |
| 1 effects | XAPO chains on buses | `DspFilterDef` and the Species XAPOs of §4.16 |

The reference engine has the same separation, with interchangeable device back-ends in place of layer 2; Species has exactly one voice back-end, on XAudio2 2.9 and X3DAudio.

**AEN-003** (MUST NOT) The audio engine never writes simulation state, never reads simulation or presentation objects directly, and never includes a simulation header with write access. Everything it knows about the world arrives in commands that presentation code builds from the immutable per-tick snapshots and the presentation events of the simulation (DEC-007, DEC-222; §2.3). A build-time include check enforces this (ADR-022).

**AEN-004** (MUST NOT) The audio engine never draws from the simulation generator. It owns one presentation generator instance (`FxRng`), shared with no other system and used only on its update thread; its draws choose sample-group members, shared-random emitters and random values (DEC-013, ADR-022; [randomness and integrity](../../../reference/12_simulation_model/07_randomness_and_integrity.md) R-SIM-608). The instance is seeded with `aen.rng.match_seed` when the engine is created and again at every `BeginMatch`.

**AEN-005** (MUST) What a participant hears may differ from what another hears, by design: audience filtering by the local team, camera-relative ranking, shared-nearest emitter choice, camera-distance links, virtualisation and dropped commands all depend on the local participant ([audio design](../../../reference/10_audio_design/README.md) R-AUD-001). No simulation rule ever branches on an audio result; game code uses results only for presentation decisions.

**AEN-006** (MUST) The audio engine runs on its own clock (the audio clock), never on the simulation tick or on the presentation clock of ADR-036. In the reference model the audio logic advances from an accumulator of the presentation frame delta (`ref.sim.frame.audio_update_period`, `ref.sim.frame.delta_clamp`, [tick timing](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) R-SIM-570, R-SIM-571), while envelopes, loop delays and sample-use stamps read the real clock; §4.4 states the reference accumulator and the Species schedule.

**AEN-007** (MUST) The dedicated server and every headless build link no part of the audio engine (ADR-022). A headless participant raises the same presentation events and plays nothing.

**AEN-008** (MUST) The audio design is the authority for every design rule this component implements: the event set, layer data and defaults, audiences, sharing, envelopes and loops as heard, links, groups, the 14 effect kinds and their behaviour, the distance and panning laws, the buses, their levels and priority classes, and the music hooks ([audio design](../../../reference/10_audio_design/README.md) R-AUD-006). This component cites the design's requirements and `ref.aud.*` keys and never redefines them; it owns the arithmetic, the scheduling and every engine value of its tuning table (§4.21).

## 2 Interfaces

### 2.1 Interface sketch

Everything in the sketch is engine code in namespace `Engine` (`AGENTS.md` R9). The audio design's game concepts reach it only as engine values: presentation code passes the simulation's object handle (`SimObjectId`) as an emitter id (`EmitterId`) and a team as an audience group (`AudienceGroup`), and the audiences, links and match phases are named for what the engine compares and counts.

```cpp
namespace Engine
{

using Seconds = double;   // audio time (platform clock, ADR-074)
using Decibels = float;

// Data tokens of the audio design in brackets; order kept where noted.
enum class SoundPosition : std::uint8_t
{
  Screen2d, WorldFixed, Attached   // [2d, fix, att]
};
enum class SoundSharing : std::uint8_t
{
  PerTrigger, SharedRandom, SharedNearest   // [poly, share-rnd, share-near]
};
enum class SoundLoop : std::uint8_t
{
  Once, Loop, NoteLoop   // [once, loop, note]
};
enum class SoundAudience : std::uint8_t
{
  All, OwnGroup, OtherGroup, NoGroup   // [all, own, enemy, none]
};
enum class SoundParamKind : std::uint8_t
{
  Fixed, Random, Linked
};
enum class SoundParamUpdate : std::uint8_t
{
  Continuous, PerNote
};
enum class SoundLink : std::uint8_t
{
  None, Height, PosX, Altitude, PosZ, Speed, CameraDistance, ObjectAge, GroupsNearby, TimeRemaining, ObjectsNearby
};
enum class EnvelopeStage : std::uint8_t
{
  Attack, Hold, Release   // no decay stage
};
enum class VoiceState : std::uint8_t
{
  Pending, Virtual, Real, Retired
};
// Order carries meaning: own-algorithm kinds run in this order in a chain (R-AUD-077, R-AUD-080).
enum class DspFilterKind : std::uint8_t
{
  Chorus, Compressor, Distortion, StereoEcho, Flanger, StandardGargle, RoomReverb, ParametricEq, StudioReverb,
  ResonantLowPass, BitCrusher, Gargle, Echo, SimpleReverb
};
// Order is the priority-class order, highest first (DEC-570); Music is outside the ranking.
enum class AudioBus : std::uint8_t
{
  Ui, Announcer, Voice, World, Ambience, Music
};
enum class StopMatch : std::uint8_t
{
  ExactEvent, SourcePrefix, Everything
};
enum class MatchPhase : std::uint8_t
{
  NoMatch, Countdown, Running, Ended   // Countdown: the match's grace period (R-AUD-303)
};

struct EmitterId   // the simulation's object handle as the engine names an emitter; EmitterId{} = no emitter
{
  std::uint64_t value = 0;
};
struct AudienceGroup   // the group that an audience compares; presentation code passes the team
{
  std::uint8_t value = 0;
};

struct SoundVoiceId   // generation 0 = invalid
{
  std::uint32_t slot = 0;
  std::uint32_t generation = 0;
};

struct TriggerCommand   // AEN-130
{
  SourceId source;                      // the full id is <source>.<event>
  EventId event;
  EmitterId emitter;                    // EmitterId{} = no emitter
  std::optional<AudienceGroup> group;   // nullopt = no team (R-AUD-025)
  Vec3 position;                        // world frame, wu
  Vec3 velocity;                        // wu/s
  Seconds issueTime;                    // audio clock at issue (AEN-024)
};

struct StopCommand
{
  EmitterId emitter;
  StopMatch match;
  SourceId source;
  EventId event;
  Seconds issueTime;
};

struct EmitterUpdate   // AEN-041
{
  EmitterId emitter;
  bool present;
  Vec3 position;
  Vec3 velocity;
  float heightAboveTerrain;
  float age;
  std::uint16_t groupsNearby;
  std::uint16_t objectsNearby;
};

struct ListenerUpdate
{
  Vec3 position;
  Vec3 front;
  Vec3 up;
  Vec3 velocity;
  float heightAboveTerrain;
};

struct MatchState
{
  MatchPhase phase;
  std::optional<AudienceGroup> localGroup;
  float gameSpeed;
  float timeRemaining;
};

class CommandWriter   // one per producing thread; every call writes one command
{
public:
  bool Trigger(const TriggerCommand& _command);   // false: the queue is full; the command is dropped and counted
  bool Stop(const StopCommand& _command);
  bool UpdateEmitters(std::span<const EmitterUpdate> _updates);
  bool SetListener(const ListenerUpdate& _listener);
  bool SetMatchState(const MatchState& _state);
  bool BeginMatch(AudienceGroup _localGroup);   // per-match state starts (AEN-197)
  bool EndMatch();                              // per-match state discarded
  bool SetLoadingView(bool _shown);             // AEN-111
  bool SetMasterVolume(std::uint8_t _step);     // 0 to ref.meta.options.sound.volume_max
  bool SetBusVolume(AudioBus _bus, std::uint8_t _step);
  bool SetSwapStereo(bool _on);
  bool SetEffectsEnabled(bool _on);             // AEN-221
  bool LoadBank(BankId _bank);
  bool ReleaseBank(BankId _bank);
  bool QueryCount(RequestId _request, EmitterId _emitter, EventId _event);   // EmitterId{} = any emitter
  bool QueryPlaying(RequestId _request, EmitterId _emitter);
  bool Pause();
  bool Resume();
  bool RestartDevice();                         // AEN-259
};

struct AudioResult
{
  ResultKind kind;
  RequestId request;
  std::uint32_t count;
  bool playing;
  SoundVoiceId voice;
};

class AudioEngine
{
public:
  static std::unique_ptr<AudioEngine> Create(const AudioSettings& _settings, const SoundDataSet& _data, SampleCache& _cache);
  CommandWriter& Producer();                                          // the calling thread's queue (ADR-026)
  void DrainResults(std::function<void(const AudioResult&)> _sink);   // game thread, once per frame
  ~AudioEngine();                                                     // AEN-106 teardown order
};

// Layer 2; the only code that includes XAudio2 or X3DAudio headers (ADR-026). Op names an XAudio2 operation set.
class VoiceBackend
{
public:
  bool Build(const BusGraphDef& _graph);                    // mastering voice on the default device and format, buses
  void Teardown();
  SourceVoiceSlot* Acquire(const VoiceFormat& _format);     // pooled per format: PCM, and MS-ADPCM for streams (AEN-142)
  void Release(SourceVoiceSlot* _voice);                    // resets every per-voice setting (DEC-752)
  void SetVolume(SourceVoiceSlot& _voice, float _amplitude, Op _set);
  void SetFrequencyRatio(SourceVoiceSlot& _voice, float _ratio, Op _set);
  void SetOutputMatrix(SourceVoiceSlot& _voice, std::span<const float> _matrix, Op _set);
  void SetFilter(SourceVoiceSlot& _voice, const XAUDIO2_FILTER_PARAMETERS& _filter, Op _set);
  void SetSends(SourceVoiceSlot& _voice, std::span<const SendLevel> _sends, Op _set);
  void Submit(SourceVoiceSlot& _voice, const XAUDIO2_BUFFER& _buffer);
  void Start(SourceVoiceSlot& _voice, Op _set);
  void Stop(SourceVoiceSlot& _voice, Op _set);
  void ApplyOperationSet(Op _set);                          // IXAudio2::CommitChanges
};

} // namespace Engine
```

### 2.2 Commands, results and the threading contract

**AEN-020** (MUST) Game code never calls XAudio2 and never waits for audio. Every public operation of `CommandWriter` writes one command into the calling thread's bounded, lock-free single-producer single-consumer queue of `aen.queue.command_capacity` commands and returns (ADR-026). Any thread may produce; each producing thread has its own queue.

**AEN-021** (MUST) When a queue is full, the newest command is dropped, the call returns false and the drop is counted per command kind in the development overlay (§6). The audio update drains every queue at each update.

**AEN-022** (MUST) Commands from all queues are applied in the order of their issue time; commands with equal issue times are applied in queue creation order and, within one queue, in write order.

**AEN-023** (MUST) Results return through one lock-free result queue of `aen.queue.result_capacity` results, which game code drains once per frame: query answers (AEN-136), readiness changes (AEN-104) and the tracked-emitter set (AEN-041). A full result queue drops the newest result and counts it.

**AEN-024** (MUST) Every command carries its issue time, the audio clock read by the producer when it writes the command. A trigger's **start time** is its issue time plus `aen.update.schedule_latency`, so the order and spacing of triggers are preserved into the mix (AEN-148).

**AEN-025** (MUST) XAudio2 callbacks (`IXAudio2VoiceCallback`, `IXAudio2EngineCallback`) only set atomic flags or push to lock-free queues: a buffer start, buffer end, loop end or stream-end notice to the update, a stream-buffer-free notice to the stream task, a critical-error notice to the update. They never allocate, lock, log synchronously, create or destroy voices, or call game code (ADR-026). The XAudio2 processing thread touches only the sample memory and buffers that the engine handed it.

| Command | Effect | Requirements |
|---|---|---|
| `Trigger` | raises an event: zero or more instances | AEN-130 to AEN-133 |
| `Stop` | stops instances by emitter and event | AEN-134 |
| `UpdateEmitters` | positions, velocities and link inputs of emitters; absence or `present = false` means the emitter has left the match world | AEN-041, AEN-185 |
| `SetListener` | the listener (camera) | AEN-209 |
| `SetMatchState` | match phase, local team, game speed, time remaining | AEN-196, AEN-197 |
| `BeginMatch`, `EndMatch` | the per-match audio state | AEN-197 |
| `SetLoadingView` | the loading-view gate | AEN-111 |
| `SetMasterVolume`, `SetBusVolume` | player volumes | AEN-215 |
| `SetSwapStereo` | swap left and right | AEN-205 |
| `SetEffectsEnabled` | the effects switch | AEN-221 |
| `LoadBank`, `ReleaseBank` | residency | AEN-244 |
| `QueryCount`, `QueryPlaying` | answered in the result queue | AEN-136 |
| `Pause`, `Resume` | stop and resume the advance and output of every instance | AEN-138 |
| `RestartDevice` | rebuilds the graph | AEN-259 |

### 2.3 Inputs from the simulation and presentation

**AEN-040** (MUST) The simulation emits sound cues as typed presentation events at the tick where the rule fires (DEC-007, DEC-222 item 5). Presentation code (the audio bridge, outside this component) turns each cue into a `Trigger` or `Stop` command. The engine depends only on receiving commands in issue order; whether cues arrive in-process or from a server is decided with the threading and networking models (DEC-222, DEC-224). Under the default of DEC-318 the fuse-blink and match-clock sounds are raised by drawing code, so they arrive only while drawn; the engine treats them like any trigger. Interface code raises, as `Trigger` commands, the interface events of the match UI sound source that pages, widgets and HUD panels raise ([pages and screen recipes](../ui_framework/pages_and_screen_recipes.md) UI-315; R-AUD-300); no page, widget or panel plays a sound directly.

**AEN-041** (MUST) Once per frame, presentation sends one `EmitterUpdate` for each emitter that the engine lists as tracked, read from the newest immutable snapshot: the position (a structure's centre position for a structure), the velocity, the height above the terrain at the position, the age, and the counts of teams and of actors near the position used by the `teams_nearby` and `unit_count` links. The engine publishes its tracked-emitter set in the result queue whenever it changes: every emitter of an attached or shared instance, and every emitter whose instance has a linked value that reads emitter data.

**AEN-042** (MUST) Once per frame, presentation sends the listener (the camera's position, front, up and velocity, and the terrain height under the camera for the `height` link of two-dimensional instances) and the match state (phase, local team, game speed, match time remaining). The audio engine itself queries nothing.

**AEN-043** (MUST) Presentation sends these commands in every application loop that shows a frame: the match, the front end and the loading view. The engine's update runs whether or not any arrive (AEN-120).

### 2.4 Voice back-end contract

**AEN-045** (MUST) The voice back-end (`VoiceBackend`) replaces the reference engine's device contract as follows:

| Reference device operation | Species voice back-end |
|---|---|
| create the device with a mixing rate, a channel count, a hardware-positioning flag and per-channel ring sizes | `Build`: one mastering voice on the default device with the default format, the bus graph of §4.16, and the voice pools of AEN-142 |
| N − 1 effect channels and one music channel | the real-voice budget, caps and the music voice pair of §4.6 and §8.2 |
| per-channel pull of sample data through a fill callback | buffer submission per voice (§4.10); callbacks only signal (AEN-025) |
| master volume 0 to 255 applied to every channel | the gain of the pre-master bus (AEN-215) |
| per-channel volume on the ten-step scale | the voice volume in dB (AEN-214) |
| per-channel playback frequency | the voice frequency ratio (AEN-196) |
| per-channel world, head-relative or disabled positioning, position, velocity, minimum distance | the distance gain and output matrix computed by the engine (§4.14) |
| per-channel effect enable, parameter update and disable | routing to effect buses (§4.16); no voice carries an effect chain (ADR-024) |
| listener position, orientation and velocity | `SetListener` (AEN-209) |
| reset a channel for a new sound | `Acquire` from the pool and a start with an offset (AEN-148, AEN-182) |
| device pump: apply deferred settings and refill rings | one operation set per update, applied with `ApplyOperationSet` (AEN-123) |
| silence writer | a shared silent buffer of `aen.voice.silence_buffer` (AEN-174) |

**AEN-046** (MUST) Voices are released with every per-voice setting reset: volume, frequency ratio, output matrix, filter, send levels and buffers. A voice given to a new instance starts from that instance's own settings only (DEC-752).

## 3 Data model

### 3.1 Types, ownership and lifetimes

**AEN-060** (MUST) The engine uses these types; the effect-kind definitions are the schema-defined table that the code generator compiles into the engine, with no pack entry (DAT-273):

| Type | Layer | Role | Owner and lifetime |
|---|---|---|---|
| `SoundSourceDef` | 5 | the event definitions of one sound source: an actor kind, a structure kind or one of the eleven non-object sources of [event catalogue](../../../reference/10_audio_design/event_catalogue.md) R-AUD-121, in that order | cooked sound data; process lifetime, reloadable in Debug builds |
| `SoundEventDef` | 5 | one event: its full id `<source>.<event>` and its layers | as above |
| `SoundLayerDef` | 5 | one layer: the fields of [event model](../../../reference/10_audio_design/event_model_and_sound_data.md) R-AUD-049 plus the engine fields of AEN-062 | as above |
| `SoundGroupDef` | 5 | a sample group: its asset id, its members, their weights, resident or streamed | as above |
| `DspFilterDef` | 1 | one of the 14 effect kinds: its parameter descriptors (§3.4) | compiled into the engine from the effect-kind table `sound.effect_kinds`, with no pack entry (DAT-273); process lifetime |
| `SoundParam` | 4 | the definition of one modulatable value: kind, link, update mode, ranges, smoothing (R-AUD-051 to R-AUD-054) | part of a layer or an effect binding |
| `SoundVoice` | 4 | one instance (§3.3) | the engine's instance table of `aen.instances.capacity` slots; from trigger to retirement |
| `SoundVoiceId` | 4 | a weak handle {slot, generation}; two handles are equal when both fields are; generation 0 is invalid; each new instance takes the next generation of its slot, starting at 1 | value type |
| `SampleCache` | 3 | residency: banks, group reference counts, stream heads (§4.17) | the engine; process lifetime |
| `SampleBank` | 3 | a set of resident groups loaded and released together | `SampleCache`; its scope (AEN-244) |
| `AudioStream` | 3 | the read position and buffer ring of one real streamed instance; XAudio2 decodes its MS-ADPCM buffers | `SampleCache`; while its instance is real |
| `VoiceBackend` | 2 | the XAudio2 engine, mastering voice, voice pools and bus graph | the engine; process lifetime, rebuilt on device loss |
| `SourceVoiceSlot` | 2 | one pooled `IXAudio2SourceVoice` with its callback object | its pool |
| `AudioBusGraph` | 2 | the submix voices of §4.16 | `VoiceBackend` |
| `CommandWriter`, `AudioResult` | — | the queues of §2.2 | the engine; process lifetime |

**AEN-061** (MUST) The engine holds no simulation object pointer. Emitters are named only by `EmitterId`, the engine form of the simulation's object handle (`SimObjectId`) that presentation code passes (`AGENTS.md` R9), and their state is the latest `EmitterUpdate` the engine has received (AEN-041).

### 3.2 Layer data used by the engine

**AEN-062** (MUST) Besides the fields of [event model](../../../reference/10_audio_design/event_model_and_sound_data.md) R-AUD-049, a layer's event data carries these engine fields; a field that the data omits takes the default shown:

| Field | Default | Used by |
|---|---|---|
| copy decay (× per louder copy) | `ref.aud.rank.copy_decay` (0.75) | AEN-141 |
| patience window of a `once` layer | `ref.aud.layer.one_shot_patience_updates` × `ref.sim.frame.audio_update_period` (0.15 s) | AEN-146 |
| per-event real-voice cap | the bus default of AEN-143 | AEN-143 |
| maximum distance | `aen.spatial.max_distance_default` | AEN-202 |
| Doppler, distance-filter and distance-reverb factors | `aen.spatial.doppler_factor_default`, `aen.spatial.filter_factor_default`, `aen.spatial.reverb_factor_default` | AEN-207 |

**AEN-063** (MUST) The copy decay is an authored design value of each event, and its default is the audio design's key `ref.aud.rank.copy_decay`; the engine owns the ranking arithmetic that applies it (AEN-141) and defines no copy-decay key of its own. The patience window is likewise an event value whose default derives from the audio design's `ref.aud.layer.one_shot_patience_updates`.

**AEN-064** (MUST) Event ids are matched exactly (R-AUD-022); source and event ids, group asset ids and effect-kind names are resolved to indices when the sound data is cooked, so no run-time look-up compares strings.

### 3.3 Instance state

**AEN-066** (MUST) A `SoundVoice` holds: its handle; its layer; its full event id; its state (`Pending`, `Virtual`, `Real`, `Retired`); its envelope stage and envelope timer; the current output and the restart flag of each of its six modulatable values; its effect-parameter outputs; its emitter set and selected emitter; its position and velocity; its trigger time and start time; its play position in frames and its current group member; whether it has ever been real; the time of its last change between virtual and real; its source voice while real and its stream while real and streamed; a pending note re-trigger time; and its audibility and score of the current update.

**AEN-067** (MUST) A new instance copies its definition from its layer ([event model](../../../reference/10_audio_design/event_model_and_sound_data.md) R-AUD-049): a field the data omits takes the design default, and the engine fields take AEN-062's defaults. Its restart flag is set, its envelope stage is `Attack`, its play position is 0 and its state is `Pending` until its start time (AEN-148).

### 3.4 Effect kinds

**AEN-070** (MUST) A `DspFilterDef` lists, for each parameter of its kind, in the order of [event model](../../../reference/10_audio_design/event_model_and_sound_data.md) R-AUD-077: the name, the minimum, the maximum, the declared default, whether it is an integer, and its schema default. A layer's effect binding gives its parameters by position (R-AUD-050), at most `aen.dsp.max_params` of them; an integer parameter takes its evaluated value truncated toward zero. The 14 kinds are realised as follows, with routing as defined in AEN-224:

| # | Kind | Parameters | Species realisation | Routing |
|---|---|---|---|---|
| 0 | chorus | 7 | Species XAPO `ChorusFx` | send |
| 1 | compressor | 6 | Species XAPO `CompressorFx` | insert |
| 2 | distortion | 5 | Species XAPO `DistortionFx` | insert |
| 3 | stereo echo | 5 | built-in `FXEcho` when the left and right delays are equal and the pan delay is 0 (mix ÷ 100, feedback ÷ 100, delay); otherwise Species XAPO `StereoEchoFx` | send |
| 4 | flanger | 7 | Species XAPO `FlangerFx` | send |
| 5 | standard gargle | 2 | Species XAPO `StandardGargleFx` | insert |
| 6 | room reverb | 13 | the XAudio2 reverb (`XAudio2CreateReverb`), its first 12 parameters converted with `ReverbConvertI3DL2ToNative` at a wet/dry mix of 100; the quality parameter has no counterpart | send, with direct and send levels of 1 |
| 7 | parametric equaliser | 3 | built-in `FXEQ`: band 0 at the centre, gain converted from dB to an amplitude ratio, bandwidth ÷ 12 octaves; bands 1 to 3 at unity gain | insert |
| 8 | studio reverb | 4 | Species XAPO `StudioReverbFx` | insert |
| 9 | resonant low-pass | 3 | Species XAPO `ResonantLowPassFx` (AEN-230) | insert |
| 10 | bit crusher | 1 | Species XAPO `BitCrusherFx` (AEN-231) | insert |
| 11 | gargle | 3 | Species XAPO `GargleFx` (AEN-232) | send for the triangle shape, insert for the square shape |
| 12 | echo | 3 | Species XAPO `EchoFx` (AEN-233) | send |
| 13 | simple reverb | 1 | Species XAPO `SimpleReverbFx` (AEN-234) | insert |

**AEN-071** (MUST) **Schema defaults.** The schema default of every effect parameter is its declared default of R-AUD-077, not 0.0. A binding that omits a parameter therefore takes the declared default, where the reference takes 0.0 (R-AUD-050, R-AUD-078). The choice is deliberate (DEC-567, [data schemas](../data_schemas/README.md) DAT-012):

1. 21 of the 63 effect parameters have a range that excludes 0.0 (among them every delay, cut-off, centre frequency, bandwidth, rate, ratio, attack, release, decay time, bit depth and reverb time), so a 0.0 default would make an omission a range error, and two of them make a sound unusable: a bit depth of 0 quantises every sample to silence, and an echo delay of 0 has no delay line;
2. every effect binding of the reference event set states all its parameters ([event model](../../../reference/10_audio_design/event_model_and_sound_data.md) R-AUD-082, FX-01 to FX-15), so the choice changes nothing in the reference set;
3. no reference sound data is imported (R-AUD-003), so parity with omitted parameters has no value;
4. effects are off by default (DEC-567, R-AUD-076), so no default is audible until effects are enabled.

The content build rejects a binding with more parameters than its kind has, and a parameter value outside its kind's range (DEC-015, ADR-059).

### 3.5 Sample groups, banks and the stream format

**AEN-072** (MUST) Every group member is cooked at `aen.format.sample_rate`, mono for every positioned layer and mono or stereo for two-dimensional layers (ADR-025). The content build marks a group **streamed** when its longest member is longer than `aen.residency.stream_threshold`, and marks every music group streamed; every other group is **resident**: its members are cooked to 16-bit PCM and stored in banks. A streamed member is cooked to MS-ADPCM at 48 kHz and stored as one Species stream: a header (channel count, sample rate, total frames, the samples per block `aen.stream.samples_per_block`, the loop start and loop end frames) followed by the ADPCM blocks in order (ADR-025). Every block holds the same number of frames, so a frame position maps to its block by division and no seek table exists. Loop points lie on block boundaries. XAudio2 decodes the blocks; the engine contains no decoder and no codec (ADR-025, AGENTS.md R14).

**AEN-073** (MUST) With the asset manifest of this library, the streamed groups are `amb.lobby_bed`, `amb.match_bed`, `amb.match_bed_b`, `amb.match_bed_c` and `sfx.creature.abductor.presence_loop`, and the music groups `music.front_end.theme`, `music.match.bed`, `music.match.sting_victory` and `music.match.sting_defeat`; every other `sfx.` group is resident. The longest resident member is about 18 s (`sfx.building.spitter_plant.burning`).

**AEN-074** (MUST) Resident groups belong to these banks; a group named by both banks is loaded once and counted by reference (AEN-245):

| Bank | Groups | Scope |
|---|---|---|
| interface | `sfx.ui.match_ui.chat_message`, `sfx.ui.match_ui.countdown_tick`, `sfx.ui.match_ui.error`, `sfx.ui.match_ui.menu_back`, `sfx.ui.match_ui.menu_select`, `sfx.ui.match_ui.selection_circle`, `sfx.ui.match_ui.unit_highlight`, `sfx.ui.match_ui.unit_select`, `sfx.ui.power_input.begin`, `sfx.ui.power_input.confirm`, `sfx.ui.power_slots.discard`, `sfx.ui.power_slots.icon_hover`, and the lobby-bed heartbeat `sfx.building.launch_vessel.fuelled_b`; plus the stream heads (AEN-253) of `amb.lobby_bed`, `amb.match_bed`, `amb.match_bed_b`, `amb.match_bed_c` and `music.front_end.theme` | loaded at start-up, held for the process lifetime |
| match | every other resident group of the [event catalogue](../../../reference/10_audio_design/event_catalogue.md), among them the laser-bolt groups `sfx.weapon.laser_bolt.spawn`, `sfx.weapon.laser_bolt.hit_structure` and `sfx.weapon.laser_bolt.kill` (DEC-564), the grenade throw `sfx.unit.trooper.throw_grenade` and the game-over announcement `sfx.ui.match_ui.game_over` (DEC-097); plus the stream heads of `sfx.creature.abductor.presence_loop`, `music.match.bed`, `music.match.sting_victory` and `music.match.sting_defeat` | loaded during each match load, released when the match world is torn down |

### 3.6 Settings read by the engine

**AEN-076** (MUST) The engine reads these settings from the settings store when it is created and when it is restarted ([options](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) R-META-429 to R-META-439, R-META-454); the reference defaults are those of the defaults table (DEC-100):

| Setting | Reference default | Species effect |
|---|---|---|
| audio output | `aen.settings.output_default` (the platform back end) | a value that names no back end selects the silent device (AEN-321) |
| master volume | `aen.settings.volume_default` (255) | the pre-master gain (AEN-215) |
| bus volumes, one per bus of R-AUD-221 | `aen.settings.volume_default` (255) | the bus gains (AEN-215) |
| voices | `aen.settings.voices_default` (32) | none: the real-voice budget is `aen.voice.budget` (DEC-570) |
| mixing rate | `aen.settings.mix_rate_reader_default` (22 050 Hz) at the engine's reader; `aen.settings.output_mix_rate_reader_default` (44 100 Hz) at the reader of the secondary output path | none: the mastering voice takes the device's format (ADR-022) |
| output buffer length | `aen.settings.output_buffer_first_run` (512) written on first run; `aen.settings.output_buffer_reader_default` (2 000) at its readers | none |
| hardware positioning | off | none |
| swap left and right | `aen.settings.swap_stereo_default` (off) | AEN-205; read at every update |
| sample-memory level | `aen.settings.sample_memory_level_default` (1) | none (AEN-077) |
| effects switch | `aen.settings.effects_default` (off) | AEN-221 |

**AEN-077** (MUST) In the reference model the sample-memory level restricts the choice of every group draw: level 2 draws among the first `aen.reference.sample_memory.level2_fraction` (½) of a group's members and level 3 among the first `aen.reference.sample_memory.level3_fraction` (¼), the member count truncated and at least 1, while level 1 uses all members; it is read at every draw and never changes the cache budget. In Species every draw uses all members (R-AUD-065, DEC-097), and the setting is read and ignored.

## 4 Behaviour and algorithms

Sections 4.5 to 4.13 are in [instances_voices_and_envelopes.md](instances_voices_and_envelopes.md) and sections 4.14 to 4.19 in [spatialisation_mixing_and_streaming.md](spatialisation_mixing_and_streaming.md).

### 4.1 Lifecycle

**AEN-100** (MUST) The engine is created after the settings store ([options](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) R-META-402, R-META-478) and initialises in this order:

1. take the 14 effect-kind definitions compiled into the engine (DAT-273), then load the cooked sound data: the sound sources with their events, layers and groups; a source kind that has no definitions receives an empty definition, so every source index resolves;
2. build the voice back-end: the XAudio2 engine, the mastering voice on the default device with the default format, the bus graph (§4.16) and the voice pools (AEN-142); if this fails, the engine runs on the silent device (AEN-321);
3. start the update thread (AEN-120) and the stream task (AEN-302);
4. request the interface bank (AEN-074) from the asset system, asynchronously;
5. report each readiness point in the result queue (AEN-104).

**AEN-101** (MUST) In the reference model the engine object is created on the loading worker with no device; the sample archives load on a sound-loading thread after the boot work; the device comes up on the main thread at the first front-end frame, after the effect-kind and then the sound definitions, without waiting for the samples, so triggers are dropped until they are loaded (AEN-110); every later frame runs the cache clean-up (AEN-241). A match launch waits for the engine, and the loading view for the sounds when the front end requires it.

**AEN-102** (MUST) A match load requests the match bank. The match loop starts only after the match bank and its stream heads are resident, so no match trigger is dropped for residency once the match runs; the wait is part of the load and is reported as load progress (DEC-226).

**AEN-103** (MUST) The content build applies the reference loader's rules as validation (DEC-015, DEC-801, ADR-059): sample names become asset ids; a priority field is ignored (R-AUD-049); an unknown effect kind, an unknown source, event or field, a duplicate key or group, a malformed block and a stray blank line inside a block are build errors with their location (R-AUD-068); effect parameters are taken by position, and a binding with more parameters than its kind is a build error (AEN-071). The reference loader resolves an unknown effect kind to an invalid index and checks unknown sources and fields only in its non-release builds.

**AEN-104** (MUST) The engine has three readiness points, each reported once in the result queue: **device ready** (the graph is built, or the silent device is in use), **interface ready** (the interface bank is resident) and **match ready** (the match bank is resident). A trigger issued before the readiness point of its group is dropped, not deferred (AEN-111).

**AEN-106** (MUST) Teardown is the reverse of creation (DEC-221 item 3): stop accepting commands; stop and join the update thread; stop and join the stream task; stop every voice; destroy the source voices, the effect buses, the player buses, the pre-master bus and the mastering voice; release the XAudio2 engine; release the banks. Every wait is bounded by `aen.lifecycle.teardown_wait`, after which the remaining step is abandoned with a diagnostic (DEC-221 item 7).

### 4.2 The trigger gate

**AEN-110** (MUST) In the reference model a trigger returns at once, and is **dropped, not queued**, when: the engine has no channel table; the samples are not loaded yet, so not even the front-end sounds play before every archive is read; a loading view is showing; or the call comes from any thread other than the main thread. The lobby bed starts on the first front-end update at which the gate is open ([front end, HUD, voice and ambience](../../../reference/10_audio_design/front_end_hud_voice_and_ambience.md) R-AUD-321).

**AEN-111** (MUST) In Species a trigger is dropped, counted by reason, and never deferred, when: the bank or stream head of a group of its layers is not resident (AEN-104); the loading-view gate is set (`SetLoadingView(true)`), which drops every trigger (R-AUD-048, R-AUD-325, DEC-319); or the instance table is full (AEN-300). Stops, queries and every other command pass the gate.

**AEN-112** (MUST) A trigger from any thread is accepted (AEN-020); the reference's main-thread-only contract is replaced by per-thread queues (ADR-026, DEC-224 item 12).

### 4.3 Where the audio update runs

**AEN-115** (MUST) In the reference model the audio logic is advanced by the loops that draw frames: once per rendered frame in the match loop after the simulation step, and once more after rendering when the frame rate is below `ref.sim.frame.low_fps_audio_threshold` ([tick timing](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) R-SIM-581); once per frame in the front-end loop; and once per loading frame in the loading view, whose frames last `ref.sim.frame.loading_frame_period`.

**AEN-116** (MUST) In Species the update runs on the engine's own thread in every application state, at its own rate, whatever the frame rate; the application loops only send commands (AEN-043). The threading model that places this thread is DEC-224's.

### 4.4 Update schedule and step order

**AEN-120** (MUST) The engine runs one update every `aen.update.period` of audio time, equal to one XAudio2 processing quantum, so the changes of one update take effect within one quantum. Each update reads the audio clock once (T) and uses T, or the elapsed time Δt since the previous update, for every timed quantity. An update that starts late runs once; there are no catch-up updates, and lateness changes only granularity.

**AEN-121** (MUST) In the reference model each advance adds the presentation frame delta (clamped to `ref.sim.frame.delta_clamp`) to an accumulator and runs exactly one audio update of `ref.sim.frame.audio_update_period` when the accumulator holds at least that much, keeping the surplus; there is no catch-up loop. While the game is paused the last frame delta keeps being added (R-SIM-571), and the command-stream recording hook drives a fixed capture rate (R-SIM-740). At or below 20 frames per second one update runs per frame and the backlog grows; below the low-frame-rate threshold the second advance adds the frame delta again, so the time is counted twice. Envelopes and loop delays read the real clock, so the update rate changes only their granularity and the rate of starts. With no device, no update runs.

**AEN-122** (MUST) The reference update runs these steps in order; the Species step of AEN-123 that replaces each is given:

| # | Reference step | Species |
|---|---|---|
| 1 | advance the music instance; when it has finished, promote the pending music request (§8.2) | step 3 |
| 2 | development step of the sound editor | none (§8.3) |
| 3 | size a working array for the instance ids | none |
| 4 | pass 1: expire one-shots out of patience; remove attached instances with no emitter, without release; compute every instance's audibility | steps 4 and 5 |
| 5 | sort by audibility, loudest first | step 6 |
| 6 | pass 2: priorities with the copy decay; remember the best instance with no voice and a priority above 0 | step 6 |
| 7 | give that one instance a voice, evicting the lowest-ranked occupant (AEN-156) | step 6 |
| 8 | advance every playing instance: envelope, pitch, effect parameters, position | steps 5 and 7 |
| 9 | clear the flag that makes every voice re-send its volume once after a master-volume change | step 7 (every update sends every value) |
| 10 | set the listener from the camera | step 1 |
| 11 | device pump: apply deferred positioning and refill every voice's buffer, running the fill callbacks | steps 7 and 8 |

**AEN-123** (MUST) The Species update runs these steps in order:

1. drain the command queues (AEN-022): match state, listener, swap, volumes, effects switch, bank requests, emitter updates, then stops (AEN-134) and triggers (AEN-130) in issue order, then queries (AEN-136);
2. advance the per-match pitch-bend state (AEN-197) and the ducking state (AEN-216);
3. advance the music channel (§8.2);
4. start the pending instances whose start time is at most T + `aen.update.period` (AEN-148); resolve each shared instance's emitter (AEN-157) and each attached instance's emitter (AEN-185); retire the instances due for retirement (AEN-146);
5. for every started instance: evaluate its values (AEN-188), advance its envelope (AEN-166) and play position (AEN-173), and compute its distance gain and audibility (AEN-140, AEN-202);
6. rank the instances and decide the real set (AEN-141 to AEN-147);
7. for every real instance: compute and set its volume, frequency ratio, output matrix, filter and send levels (AEN-196, AEN-204, AEN-214, AEN-224) and submit its next buffers (AEN-174);
8. apply every voice change of this update as one XAudio2 operation set, so they take effect at the same sample (ADR-026);
9. publish query answers, readiness changes, the tracked-emitter set (AEN-041) and statistics (AEN-137).

```mermaid
flowchart TD
  A["update at audio time T"] --> Q["1 drain queues: state, listener, emitters, stops, triggers, queries"]
  Q --> P["2 pitch-bend and ducking state"]
  P --> M["3 music channel"]
  M --> S["4 start pending instances due by T + period; resolve emitters; retire"]
  S --> E["5 values, envelope, play position, distance gain, audibility"]
  E --> R["6 rank by class and score; caps; steal with margin and hold"]
  R --> V["7 real voices: volume, ratio, matrix, filter, sends, buffers"]
  V --> O["8 one operation set for every voice change"]
  O --> X["9 results and statistics"]
```

Diagram 4-A: one audio update (AEN-123).

### 4.20 Data flow from event to output

```mermaid
flowchart LR
  SIM["simulation: presentation events per tick"] --> BR["audio bridge: Trigger and Stop commands"]
  UI["interface code"] --> BR
  SNAP["immutable snapshot"] --> EU["emitter and listener updates"]
  BR --> Q["per-thread command queues"]
  EU --> Q
  Q --> UPD["audio update thread at the control rate"]
  DATA["cooked sound data: sources, events, layers, groups, effect kinds"] --> UPD
  BANK["resident banks"] --> VB
  UPD --> RANK["instances: values, envelopes, ranking, virtual or real"]
  RANK --> VB["voice back-end: pooled source voices"]
  STR["stream task: MS-ADPCM buffers from the packs, decoded by XAudio2"] --> VB
  VB --> FXB["effect buses"] --> PB["player buses"]
  VB --> PB
  PB --> PM["pre-master: master gain and limiter"] --> MV["mastering voice: default device"]
  UPD --> RES["result queue: queries, readiness, tracked emitters"]
```

Diagram 4-D: from a raised event to the device (AEN-020 to AEN-043, AEN-123, AEN-220).

**AEN-264** (MUST) **Start latency (Species).** A trigger reaches the device `aen.update.schedule_latency` after its issue time plus at most one processing quantum and the device's own latency; a burst of triggers is decided in the update that drains it, so no trigger waits for a start quota (AEN-147).

**AEN-265** (MUST) **Start latency (reference model).** A trigger waits for the next audio update (0 to one update period), then loses up to `ref.aud.layer.one_shot_patience_updates` updates while better candidates start one per update (`ref.aud.reference.starts_per_update`), then the device's write-to-play gap; a burst of 30 explosions takes 1.5 s to start (DEC-570), and a two-dimensional interface sound, which outranks every positioned sound below the top of the scale, usually starts at the next update.

### 4.21 Tuning table

Every value is an engine value of this component; the reference values are the starting values (DEC-004). Keys of other documents are cited, not repeated: `ref.aud.*` in the [audio design](../../../reference/10_audio_design/README.md) files, `ref.sim.frame.*` and `ref.sim.speed.normal` in [tick timing](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md), `ref.meta.options.sound.*` in [options](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md).

| key | value | unit | range or notes | requirement | DEC |
|---|---|---|---|---|---|
| `aen.format.sample_rate` | 48 000 | Hz | every cooked member (ADR-025) | AEN-072, AEN-148 | — |
| `aen.update.period` | 0.01 | s | 100 updates per second; one processing quantum | AEN-120, AEN-148 | DEC-224 |
| `aen.update.schedule_latency` | 0.03 | s | added to every issue time | AEN-024, AEN-264 | DEC-570 |
| `aen.queue.command_capacity` | 4 096 | commands | per producing thread | AEN-020 | — |
| `aen.queue.result_capacity` | 1 024 | results | | AEN-023 | — |
| `aen.instances.capacity` | 1 024 | instances | live instances | AEN-060, AEN-300 | — |
| `aen.rng.match_seed` | 1 | seed | the engine's presentation generator | AEN-004 | DEC-013 |
| `aen.lifecycle.teardown_wait` | 1 | s | per teardown wait | AEN-106 | DEC-221 |
| `aen.voice.budget` | 64 | voices | real instances at once | AEN-142, AEN-143 | DEC-570 |
| `aen.voice.pool_headroom` | 8 | voices | per format pool, for fades | AEN-142 | — |
| `aen.voice.bus_cap.ui` | 4 | voices | never stolen | AEN-143 | DEC-570 |
| `aen.voice.bus_cap.announcer` | 4 | voices | | AEN-143 | DEC-570 |
| `aen.voice.bus_cap.voice` | 16 | voices | | AEN-143 | DEC-570 |
| `aen.voice.bus_cap.world` | 32 | voices | | AEN-143 | DEC-570 |
| `aen.voice.bus_cap.ambience` | 4 | voices | never taken by world instances | AEN-143 | DEC-570 |
| `aen.voice.bus_cap.music` | 2 | voices | the music voice pair | AEN-143, AEN-361 | DEC-570 |
| `aen.voice.event_cap.voice` | 3 | voices | per event on the voice bus | AEN-143 | DEC-570 |
| `aen.voice.event_cap.world_default` | 4 | voices | per event on the world bus | AEN-143 | DEC-570 |
| `aen.voice.event_cap.world_max` | 6 | voices | the most an event may author | AEN-143 | DEC-570 |
| `aen.voice.steal_margin_db` | 3 | dB | | AEN-145 | DEC-570 |
| `aen.voice.state_hold` | 0.25 | s | minimum time between state changes | AEN-145 | — |
| `aen.voice.steal_fade` | 0.02 | s | fade-out of a victim; fade-in mid-sound | AEN-145, AEN-149 | — |
| `aen.voice.max_starts_per_update` | 0 | instances | 0 = no cap | AEN-147 | DEC-570 |
| `aen.voice.max_frequency_ratio` | 3.0 | × | above it the previous ratio stays | AEN-142, AEN-196 | — |
| `aen.voice.min_frequency_ratio` | 1/1024 | × | XAudio2's minimum | AEN-196 | — |
| `aen.voice.silence_buffer` | 48 000 | frames | 1 s, repeated as needed | AEN-045, AEN-174 | — |
| `aen.voice.loop_lookahead` | 0.05 | s | | AEN-174 | — |
| `aen.envelope.epsilon` | 1e-6 | — | equality tolerance of envelope fractions and delays | AEN-166, AEN-176 | — |
| `aen.pitch_bend.time_constant` | 0.17 | s | the reference constant with 30 world sounds | AEN-197 | DEC-751 |
| `aen.spatial.max_distance_default` | 1 000 000 000 | wu | the reference's effective maximum | AEN-062, AEN-202, AEN-210 | DEC-566 |
| `aen.spatial.max_distance_fade_fraction` | 0.1 | of dmax | | AEN-202 | DEC-566 |
| `aen.spatial.doppler_factor_default` | 0 | × | off | AEN-062, AEN-207 | DEC-566 |
| `aen.spatial.filter_factor_default` | 0 | × | off | AEN-062, AEN-207 | DEC-566 |
| `aen.spatial.reverb_factor_default` | 0 | × | off | AEN-062, AEN-207 | DEC-566 |
| `aen.listener.velocity_scale` | 0.2 | × | camera velocity to listener velocity | AEN-209 | — |
| `aen.reference.voice.position_threshold` | 5 | % of the listener distance | reference model only | AEN-210 | — |
| `aen.mix.duck_ramp` | 0.05 | s | | AEN-216 | DEC-570 |
| `aen.mix.limiter.release` | 6 | — | `FXMasteringLimiter` range 1 to 20 | AEN-220 | DEC-567 |
| `aen.mix.limiter.loudness` | 1 000 | — | range 1 to 1 800 | AEN-220 | DEC-567 |
| `aen.dsp.max_params` | 13 | parameters | the largest kind | AEN-070 | — |
| `aen.fx.full_scale` | 32 768 | 16-bit units | float ±1.0 | AEN-225 | — |
| `aen.fx.lowpass.cutoff_divisor` | 3 850 | — | f = e^(p0 ÷ 3850 + 4.7) | AEN-230 | — |
| `aen.fx.lowpass.cutoff_offset` | 4.7 | — | | AEN-230 | — |
| `aen.fx.lowpass.q_divisor` | 4 | — | Q = e^(p1 ÷ 4) − 1 | AEN-230 | — |
| `aen.fx.lowpass.q_floor` | 1e-5 | — | | AEN-230 | — |
| `aen.fx.lowpass.max_ratio` | 0.499 | of the rate | | AEN-230 | — |
| `aen.fx.lowpass.clamp` | 32 765 | 16-bit units | | AEN-230 | — |
| `aen.fx.lowpass.initial_cutoff` | 1 000 | Hz | with Q 1 and gain 1 | AEN-230 | — |
| `aen.fx.denormal_offset` | 1e-26 | 16-bit units | added after each block | AEN-230 | — |
| `aen.fx.bitcrusher.clamp` | 32 767 | 16-bit units | | AEN-231 | — |
| `aen.fx.echo.buffer_seconds` | 3 | s | | AEN-233 | — |
| `aen.fx.echo.clamp` | 32 766 | 16-bit units | | AEN-233 | — |
| `aen.fx.reverb.history_seconds` | 3 | s | | AEN-234 | — |
| `aen.fx.reverb.tap_delays_ms` | 1.1, 3, 11, 19, 29, 46 | ms | in tap order | AEN-234 | — |
| `aen.fx.reverb.tap_gains` | 0.36, 0.25, 0.34, 0.23, 0.32, 0.21 | × | in tap order | AEN-234 | — |
| `aen.fx.reverb.dry` | 0.23 | × | | AEN-234 | — |
| `aen.fx.reverb.wet` | 0.67 | × | | AEN-234 | — |
| `aen.fx.reverb.clamp` | 32 766 | 16-bit units | | AEN-234 | — |
| `aen.residency.stream_threshold` | 20 | s | longest member of a group | AEN-072 | — |
| `aen.memory.interface_bank_budget` | 8 | MiB | | AEN-243 | — |
| `aen.memory.match_bank_budget` | 192 | MiB | | AEN-243, AEN-304 | — |
| `aen.memory.stream_budget` | 8 | MiB | stream rings and heads | AEN-243, AEN-304 | — |
| `aen.stream.block_frames` | 4 800 | frames | length of one ring buffer, 0.1 s, rounded up to whole ADPCM blocks (5 120 frames at 512 samples per block) | AEN-252 | — |
| `aen.stream.blocks_per_stream` | 3 | blocks | | AEN-252 | — |
| `aen.stream.max_streams` | 16 | streams | real at once | AEN-252 | — |
| `aen.stream.samples_per_block` | 512 | frames per ADPCM block | 32 to 512, chosen per group so that its loop points fall on block boundaries | AEN-072, AEN-254, AEN-255 | — |
| `aen.reference.cache.budget` | 60 | MiB | 62 914 560 bytes; reference model | AEN-241, AEN-246 | — |
| `aen.reference.cache.max_channels` | 8 | channels | reference model | AEN-240 | — |
| `aen.reference.cache.max_frames` | 268 435 456 | frames | reference model | AEN-240 | — |
| `aen.reference.cache.max_rate` | 192 000 | Hz | reference model | AEN-240 | — |
| `aen.reference.cache.max_samples` | 536 870 912 | samples | channels × frames; reference model | AEN-240 | — |
| `aen.reference.sample_memory.level2_fraction` | 0.5 | of the members | reference model | AEN-077 | DEC-097 |
| `aen.reference.sample_memory.level3_fraction` | 0.25 | of the members | reference model | AEN-077 | DEC-097 |
| `aen.settings.output_default` | platform | — | the platform back end | AEN-076 | DEC-100 |
| `aen.settings.volume_default` | 255 | steps | master and every bus | AEN-076 | DEC-100 |
| `aen.settings.voices_default` | 32 | voices | no Species effect | AEN-076 | DEC-100 |
| `aen.settings.mix_rate_reader_default` | 22 050 | Hz | engine reader; no Species effect | AEN-076 | DEC-100 |
| `aen.settings.output_mix_rate_reader_default` | 44 100 | Hz | secondary output path; no Species effect | AEN-076 | DEC-100 |
| `aen.settings.output_buffer_first_run` | 512 | frames | no Species effect | AEN-076 | DEC-100 |
| `aen.settings.output_buffer_reader_default` | 2 000 | frames | no Species effect | AEN-076 | DEC-100 |
| `aen.settings.swap_stereo_default` | off | — | | AEN-076 | DEC-100 |
| `aen.settings.sample_memory_level_default` | 1 | level | no Species effect | AEN-076 | DEC-097 |
| `aen.settings.effects_default` | off | — | | AEN-076, AEN-221 | DEC-567 |
| `aen.perf.update_cpu_budget` | 0.5 | ms per update | on the minimum CPU at full instance capacity | AEN-303 | — |

## 5 Concurrency and performance budget

**AEN-300** (MUST) The instance table holds `aen.instances.capacity` instances and is allocated when the engine is created; when it is full, a new trigger is dropped and counted. Command and result queues, voice pools and stream rings are also allocated at creation, so the update thread does not allocate during a session except for bank bookkeeping.

**AEN-301** (MUST) Threads and what each touches: producing threads write only their own command queue; the update thread alone owns every instance, every definition at run time and every voice setting; the stream task alone writes stream buffers; the XAudio2 processing thread reads only submitted buffers and resident bank memory, which no thread writes while the bank is resident. No other thread touches audio memory (DEC-224 item 12).

**AEN-302** (MUST) The stream task is a recurring task that the engine owns, woken by buffer-free notices and by seek requests; it reads the next MS-ADPCM buffers of each stream from the packs through the content reader and refills them in the order of the notices, and it never decodes, because XAudio2 decodes ADPCM itself (ADR-025). Which worker runs it follows the threading model (DEC-224, ADR-025).

**AEN-303** (MUST) An update costs O(n log n) in its n live instances (the ranking sort) plus O(r) in its r real voices, and stays within `aen.perf.update_cpu_budget` on the minimum CPU with the table full (test T27).

**AEN-304** (MUST) Memory stays within the budgets of AEN-243. With the reference durations of the asset manifest, the resident groups hold about 1 888 s of audio in 868 files, about 173 MiB of 16-bit mono at 48 kHz, of which the interface bank takes about 2 MiB; the 34 files of the newly bound laser-bolt, grenade-throw and game-over groups, whose durations are not yet authored, add at most about 3.4 MiB by their reference durations; authoring the two-dimensional stings in stereo would add about 14 MiB. The streams need 16 rings of 3 MS-ADPCM buffers, under 1 MiB, plus the heads. The match bank is therefore about three times the reference cache budget, which held about one eighth of the reference library (474 MiB decoded) and whose largest single sample, a 344.6 s music track, decoded to 29 MiB.

**AEN-305** (MUST) In the reference model the whole audio logic and the mixing run on the main thread, the sample cache is guarded by a lock, triggers from other threads are dropped, and a buffer counter is shared between the device callback and the main thread without synchronisation (DEC-224 item 7).

## 6 Failure modes, diagnostics and telemetry

**AEN-320** (MUST) Failures and their handling; none stops the program (DEC-015):

| Failure | Handling |
|---|---|
| no device, or the graph cannot be built, or the output setting names no back end | silent device (AEN-321) |
| device lost or invalidated | rebuild (AEN-259) |
| cooked sound data missing or invalid | an empty sound set and a diagnostic; every trigger does nothing |
| group or member missing | the layer is silent, with a diagnostic (R-AUD-046) |
| bank not resident at a trigger, loading view, table full, queue full | the trigger is dropped and counted by reason (AEN-021, AEN-111, AEN-300) |
| stream corrupt or missing | the instance is retired with a diagnostic (AEN-256) |
| frequency ratio out of range | AEN-196, counted |
| start later than its start time | started at once, counted (AEN-148, AEN-254) |
| stream buffer not read when its voice needs it | the voice plays silence for the gap, counted as an underrun |

**AEN-321** (MUST) **Silent device** (R-AUD-044). With no XAudio2 graph, the engine runs its full update with zero real voices: every instance is virtual, one-shots retire by their patience, loops live until they are stopped or lose their emitter, and queries are answered. In the reference model every device call is a no-op and no update runs, so waiting instances are never freed and accumulate for the whole session.

**AEN-322** (MUST) **Profiling** (ADR-068). The update, each of its steps (AEN-123), the stream task and each bank load are bracketed by profiling markers (`ENGINE_PROFILE_SCOPE`); the engine counts the cost of the triggers per entry kind and of stream reads per event id. These replace the reference engine's always-compiled profile nodes: one per trigger entry kind, one per event measuring its buffer-fill cost, and one per update step.

**AEN-323** (MUST) **Development overlay** (ADR-022). Debug builds show: real and virtual instances per bus; the highest-ranked instances with their scores; the peak level of each player bus; memory per bank and for streams; drops by reason; late starts; one-shots retired unheard; steals per second; stream underruns. Debug builds also reload the cooked sound data while the game runs, at an update boundary; live instances keep the layer copy they started with.

**AEN-324** (MUST) The engine logs in its own category (ADR-067): readiness, bank loads and releases, device changes and rebuilds, and every diagnostic of AEN-320; it writes a memory summary every `ref.sim.frame.memory_log_interval`, as the reference's memory log does.

**AEN-325** (MUST) The engine sends no telemetry of its own; its counters are available to the diagnostics component ([diagnostics, test and performance](../diagnostics_test_and_performance/README.md)).

## 7 Acceptance tests

| Test | Kind | Checks | Requirements |
|---|---|---|---|
| T1 queue contract | unit | commands from three producers apply in issue-time order; a full queue drops the newest command, returns false and counts it; no producer call blocks | AEN-020 to AEN-022 |
| T2 callbacks | static analysis | the callback translation units call no allocator, lock, logger or voice creation, and only the voice back-end includes XAudio2 or X3DAudio headers | AEN-025, AEN-003 |
| T3 integrity | property | a recorded match replayed with audio on and off gives the same per-tick state hashes; audio code includes no simulation-generator or writable simulation header | AEN-003, AEN-004, AEN-005 |
| T4 audience and merge | unit | own and enemy layers follow the local team (R-AUD-024); a no-team event creates only all and no-team layers; a second `worker.spawn` adds an emitter, not an instance | AEN-130, AEN-131 |
| T5 emitter at trigger | unit | a query right after a trigger counts it, for every position kind; a stop in the same frame retires it unheard | AEN-134, AEN-136 |
| T6 counts in every state | unit | counts are correct with pending, virtual, real and retired instances, with and without an emitter, and over empty slots | AEN-136 |
| T7 copy decay | unit | four equal instances of one event score a, a − 2.50, a − 5.00 and a − 7.49 dB | AEN-141 |
| T8 budget and caps | property | over recorded battle trigger streams, real instances never exceed the budget, a bus cap or an event cap | AEN-143, AEN-144 |
| T9 margin and hold | property | no instance changes between virtual and real more than once per hold time except by ending or inaudibility; a candidate within the margin never steals | AEN-145 |
| T10 worked examples | unit | the Species example of AEN-150 and the reference model of AEN-158 give the outcomes stated | AEN-150, AEN-156, AEN-158 |
| T11 patience | unit | a one-shot that never gets a voice is retired at the first update at or after start + 0.15 s; a loop is never retired while virtual | AEN-146 |
| T12 start spacing | unit | ten triggers 5 ms apart start 5 ms apart in the output, within one frame, in order; a burst of 30 starts in one update | AEN-148, AEN-147 |
| T13 loop resume | capture | a loop stolen and given back resumes at its advanced position, fading in, with no discontinuity | AEN-145, AEN-149, AEN-173 |
| T14 envelope | unit | levels at sampled times of attack, hold and release equal AEN-166 within 0.01 dB, starting and ending at −50 dB | AEN-166, AEN-167 |
| T15 note loop | unit | a note layer with loop delay 0 spawns its next note at release; with a delay it spawns at the first member wrap at or after the delay; a final stop cancels it | AEN-178, AEN-169 |
| T16 smoothing | unit | a per-note pitch 0.75 to 1.35 with smoothing 0.66 starts within 0.88 to 1.16 (DEC-322 default) or 0.75 to 1.35 (option B); continuous smoothing is the same at update periods of 0.01 and 0.05 s | AEN-190, AEN-191 |
| T17 group draw | property | weighted draws match their weights within statistical tolerance and never repeat a member of a group of two or more | AEN-180 |
| T18 pitch bend | unit | game speed 0.25 settles at 0.5× and 0 at 0.1×; two-dimensional, UI and announcer instances stay at 1.0×; the next match starts at 1.0×; smoothing is the same with 1 and 30 sounds | AEN-197 |
| T19 distance and pan | unit | gains at 50 to 2000 wu equal R-AUD-203; the stereo pan at 0°, ±45° and ±90°, centred and swapped, equals AEN-204 and AEN-205; with defaults the maximum distance changes no gain below 10⁸ wu | AEN-201, AEN-202, AEN-204, AEN-205 |
| T20 other layouts | unit | with four- and six-channel mastering voices X3DAudio's matrices carry no distance attenuation, and the Doppler, filter and reverb values are neutral with default factors | AEN-206, AEN-207 |
| T21 gains | unit | master and bus steps 255, 128 and 0 give 0, −40.64 and −81.6 dB; each setting changes one bus gain; a sting ducks the world bus by 6 dB for 1 s | AEN-215, AEN-216 |
| T22 limiter | stress | the loudest recorded battle stays within full scale after the pre-master, and a signal below full scale passes unchanged | AEN-220 |
| T23 effects | unit | each Species XAPO matches AEN-230 to AEN-234 at 44.1 and 48 kHz with the same delays, rates and cut-offs; with the switch off no bus processes a sample; a send-routed echo equals x × (1 − w) + echo × w | AEN-221, AEN-224, AEN-225, AEN-230 to AEN-234 |
| T24 voice reset | unit | a voice reused after an instance with sends and a filter starts with neutral settings | AEN-046, AEN-135 |
| T25 residency | content build | the manifest's banks fit their budgets; a bank above budget, a positioned stereo member, a source not at 48 kHz, a zero-length member and a streamed loop point off a block boundary are build errors; a trigger before its bank is resident is dropped and counted | AEN-072 to AEN-074, AEN-111, AEN-243 |
| T26 streaming | capture | every streamed member plays with seeks and loops with no gap, its loop points on block boundaries and its seeks on the block start at or before the position; no stream read runs on a game thread during a full match | AEN-252 to AEN-257 |
| T27 performance | performance | an update with 1 024 live instances and 64 real voices stays within `aen.perf.update_cpu_budget` on the minimum CPU; memory stays within the budgets | AEN-303, AEN-304 |
| T28 device change | integration | switching and unplugging the default device, running and paused, continues every sound from its position | AEN-259, AEN-261 |
| T29 silent device | unit | with no device, a session of triggers keeps the live count bounded and answers every query | AEN-321 |
| T30 effect defaults | unit | a binding that omits a parameter takes its declared default; a binding with too many parameters or an out-of-range value is a build error | AEN-071 |

## 8 Open decisions

### 8.1 Decisions and architecture records

Open: DEC-044, DEC-049, DEC-109, DEC-221, DEC-222, DEC-224, DEC-226, DEC-318, DEC-320, DEC-321, DEC-322, DEC-566, DEC-567, DEC-965; DEC-339 is Dropped; the others are Resolved.

| Decision | Where | Default specified |
|---|---|---|
| DEC-013 | AEN-004 | own presentation generator |
| DEC-027 | AEN-143 to AEN-145, AEN-220 | binding model kept; priorities and mix replaced |
| DEC-042 | AEN-214 | levels in dB |
| DEC-044 | AEN-076, AEN-215 | the reference option set plus the bus volumes; no voice-budget setting |
| DEC-049 | AEN-360 | the music channel as a hook only |
| DEC-097 | AEN-077, AEN-247, AEN-321 | the audio corrections |
| DEC-109 | AEN-157 | crowd radius `ref.aud.crowd.nearest_radius` |
| DEC-221 | AEN-100, AEN-106 | creation and teardown order |
| DEC-222 | AEN-003, AEN-040 to AEN-042 | commands built from snapshots and events |
| DEC-224 | AEN-112, AEN-116, AEN-301, AEN-302 | own update thread and stream task |
| DEC-226 | AEN-102, AEN-104, AEN-111 | readiness points; early triggers dropped |
| DEC-318 | AEN-040 | drawing code raises fuse-blink and clock sounds |
| DEC-320 | AEN-197 | the match-end pitch drop kept |
| DEC-321 | AEN-185 | hard cut |
| DEC-322 | AEN-191 | compressed start of smoothed values |
| DEC-323 | AEN-130, AEN-136 | emitter recorded at trigger |
| DEC-339 | AEN-230 | device-level details not specified |
| DEC-566 | AEN-202, AEN-209 | camera listener, no effective maximum distance |
| DEC-567 | AEN-071, AEN-221, AEN-222 | dry; declared parameter defaults (DAT-012) |
| DEC-568 | AEN-180 | weighted draws without repeats |
| DEC-569, DEC-751 | AEN-197 | per-match, time-based bend |
| DEC-570 | AEN-143, AEN-145 | six buses, 64 voices, 3 dB margin |
| DEC-752 | AEN-046, AEN-136, AEN-221 | no stale voice state |
| DEC-801 | AEN-103 | duplicates are build errors |
| DEC-965 | AEN-133, AEN-138, AEN-140, AEN-178 | next note at the first member wrap; audibility by envelope level; shared note layers merge; no pause |

The audio ADRs, as amended, agree with this component; it applies them as follows:

- **ADR-024 and DEC-570.** The bus graph is data-defined, and its baseline has the six player buses of DEC-570 and R-AUD-221 under the pre-master bus (AEN-220); room reverb uses the XAudio2 reverb with I3DL2 conversion (AEN-070).
- **ADR-027 and DEC-566.** The per-event maximum distance exists, and its global default is the reference's effective maximum (AEN-202), so the default mix has no maximum distance until DEC-566 is decided.
- **ADR-023 and the copy decay.** The copy decay and the patience window are per-event design values whose defaults are the audio design's keys; the engine owns the arithmetic, the budget, the caps, the margin and the hold time (AEN-063).
- **ADR-022 and the type names.** `SoundVoice` names an instance (layer 4), and the pooled source voice is `SourceVoiceSlot` (layer 2).
- **ADR-026.** A command's issue time is the audio clock, a real-time reading of the platform clock, not the game-speed-scaled presentation clock.
- **ADR-025.** The streaming threshold applies to a group's longest member, the banks are those of AEN-074, and their budgets are those of AEN-243. Streams are MS-ADPCM, encoded by the content build tool and decoded by XAudio2; seeks and loops work on whole blocks (AEN-072, AEN-252 to AEN-255).

### 8.2 Music hook (KH-MUSIC)

**AEN-360** (MUST) Until DEC-049 is decided, the engine provides the music channel of [music hooks and brief](../../../reference/10_audio_design/music_hooks_and_sound_design_brief.md) R-AUD-400 to R-AUD-405 as a hook: one playing and one pending music instance; a request for the member that is playing is ignored; any other request replaces the pending one, which is retired, and releases the playing track finally; at the first update after the playing track has finished, the pending one starts, with its attack timed from its request (R-AUD-404); each update advances the music instance fully (envelope, pitch, loops and ends by AEN-166 and AEN-175 to AEN-178); music never enters the ranking and is never positioned, bent or effected; a stop reaches it only when it names the `music` source (R-AUD-403).

**AEN-361** (MUST) The music bus's `aen.voice.bus_cap.music` voices are reserved for the music channel, so that a music system can cross-fade two streams with sample-accurate timing in one operation set (ADR-024, R-AUD-405).

**AEN-362** (MUST) In the reference model the music channel is a separate device buffer that is never positioned, starts at full level, has no volume of its own beyond the master volume, and is reached by no skirmish rule (R-AUD-400).

### 8.3 Design-phase inputs

1. **Threads and transport.** Where the update thread and the stream task run (DEC-224), and whether emitter and listener updates come from an in-process snapshot or from a server (DEC-222).
2. **A persistent world.** Bank scopes for a world of many regions instead of one match bank: residency per region or biome, streaming ahead of the camera, and a budget per scope (DEC-226); how emitters far from the camera are culled when a region holds many more emitters than a skirmish map.
3. **Listening point and distance limits** (DEC-566, DEC-109), including per-event maximum distances for strategic zoom.
4. **Effects** (DEC-567): dry, reverb send and limiter, or per-event effects, and the release lengths that go with the choice (AEN-228).
5. **Music** (DEC-049) on the reserved music voices (AEN-361).
6. **Options** (DEC-044): which bus volumes the front end shows, and whether a voice-budget setting replaces the reference voices setting.
7. **Pause**: whether a paused skirmish, or a suspended application, pauses audio (AEN-138, DEC-965 item 4).
8. **Development tools**: a sound-tuning tool over the data reload of AEN-323; the reference's sound editor, parameter editors, statistics and profile windows are development tools that this library does not specify.
9. **Accessibility** (ADR-064): a mono output option and visual cues for important sounds.
