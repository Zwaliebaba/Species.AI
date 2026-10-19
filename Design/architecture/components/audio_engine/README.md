# Audio engine

> Status: Engine baseline · Prefix: AEN · ADRs: [ADR-022](../../../ADR/ADR-022-audio-engine-on-xaudio2.md) · [ADR-023](../../../ADR/ADR-023-virtual-voices-and-voice-stealing.md) · [ADR-024](../../../ADR/ADR-024-audio-buses-and-per-bus-effects.md) · [ADR-025](../../../ADR/ADR-025-audio-formats-codecs-and-residency.md) · [ADR-026](../../../ADR/ADR-026-audio-command-queue-and-callbacks.md) · [ADR-027](../../../ADR/ADR-027-spatialisation-and-distance-attenuation.md) · [ADR-016](../../../ADR/ADR-016-view-projection-and-depth-conventions.md) · [ADR-058](../../../ADR/ADR-058-pack-files-and-mount-policy.md) · [ADR-059](../../../ADR/ADR-059-content-build-and-validators.md) · [ADR-061](../../../ADR/ADR-061-data-schemas-and-versioning.md) · [ADR-067](../../../ADR/ADR-067-logging.md) · [ADR-068](../../../ADR/ADR-068-profiling-and-gpu-timing.md) · [ADR-069](../../../ADR/ADR-069-test-framework.md) · [ADR-074](../../../ADR/ADR-074-platform-clock-with-injectable-time.md) · Related: [audio design](../../../reference/10_audio_design/README.md) · [event model and sound data](../../../reference/10_audio_design/event_model_and_sound_data.md) · [event catalogue](../../../reference/10_audio_design/event_catalogue.md) · [positional, priorities and mix](../../../reference/10_audio_design/positional_priorities_and_mix.md) · [tick timing and main loop](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) · [randomness and integrity](../../../reference/12_simulation_model/07_randomness_and_integrity.md) · [options](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) · [asset pipeline and resources](../asset_pipeline_and_resources.md) · [data schemas](../data_schemas/README.md)

This component turns the sound events that game code raises into samples at the speakers: the binding of (source, event) pairs to layers and sample groups from data, the instances, the ranking onto a bounded set of real XAudio2 source voices with virtual voices and stealing, the buses, player volumes, limiter and effects, X3DAudio spatialisation, resident banks and streams, and the device. The [audio design](../../../reference/10_audio_design/README.md) states what plays; this component states how, precisely enough to implement and test. Every requirement states the Species engine; where what a player hears deliberately differs from the reference, the requirement cites the decision that records the change.

**Notation and units.**

- **Levels** are in dB relative to a voice's full level, 0 dB (DEC-042); an amplitude is 10^(dB ÷ 20), computed with `XAudio2DecibelsToAmplitudeRatio`. A level of the reference ten-step scale v converts to `ref.aud.level.reference_step_db` × (v − 10) dB ([audio design](../../../reference/10_audio_design/README.md) R-AUD-004). The **envelope floor** F = −10 × `ref.aud.level.reference_step_db` = −50 dB is the bottom of that scale: quiet, not silent.
- **Distances** are in world units (`wu`), measured in three dimensions in the right-handed, +Y-up world frame of ADR-016.
- **Audio time** is real time in seconds on the audio clock, which reads the platform's monotonic clock (ADR-074). Game speed and pausing never scale it. Audio durations are presentation durations; no simulation duration is ever measured in audio time.
- **Frames** are sample frames at `aen.format.sample_rate`.
- **Terms** follow the audio design ([audio design](../../../reference/10_audio_design/README.md) §2.1): a **sound source**, an **event** `<source>.<event>`, a **layer**, an **instance** (type `SoundVoice`), an **emitter**. An instance is **real** while a source voice mixes it and **virtual** while it is tracked and advanced without one. A **voice** is one pooled XAudio2 source voice.

## Folder index

| File | Holds | Requirement IDs |
|---|---|---|
| [README.md](README.md) (this file) | responsibilities and boundaries, the interface sketch, the commands and the threading contract, the inputs, the voice back-end contract, the data model, the lifecycle, the trigger gate, the update schedule, the data flow, the tuning table, budgets, failure modes, the acceptance tests and the open decisions | AEN-001 to AEN-123, AEN-264, AEN-300 to AEN-325, AEN-360, AEN-361 |
| [instances_voices_and_envelopes.md](instances_voices_and_envelopes.md) | §4.5 to §4.13: from a command to an instance, ranking, virtual voices and stealing, emitter selection, instance states, envelope arithmetic, play position and loops, start and advance, modulatable values, pitch and the game-speed bend | AEN-130 to AEN-199 |
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

Layer 2 is the one device layer: XAudio2 2.9 with X3DAudio (ADR-022, ADR-027).

**AEN-003** (MUST NOT) The audio engine never writes simulation state, never reads simulation or presentation objects directly, and never includes a simulation header with write access. Everything it knows about the world arrives in commands that presentation code builds from the immutable per-tick snapshots and the presentation events of the simulation (DEC-007, DEC-222; §2.3). A build-time include check enforces this (ADR-022).

**AEN-004** (MUST NOT) The audio engine never draws from the simulation generator. It owns one presentation generator instance (`FxRng`), shared with no other system and used only on its update thread; its draws choose sample-group members, shared-random emitters and random values (DEC-013, ADR-022; [randomness and integrity](../../../reference/12_simulation_model/07_randomness_and_integrity.md) R-SIM-608). The instance is seeded with `aen.rng.match_seed` when the engine is created and again at every `BeginMatch`.

**AEN-005** (MUST) What a participant hears may differ from what another hears, by design: audience filtering by the local team, camera-relative ranking, shared-nearest emitter choice, camera-distance links, virtualisation and dropped commands all depend on the local participant ([audio design](../../../reference/10_audio_design/README.md) R-AUD-001). No simulation rule ever branches on an audio result; game code uses results only for presentation decisions.

**AEN-006** (MUST) The audio engine runs on its own clock (the audio clock), never on the simulation tick or on the presentation clock of ADR-036; envelopes, loop delays and every other timed audio quantity read it, and its update schedule is §4.4. The audio design counts some quantities in audio updates of `ref.sim.frame.audio_update_period` ([tick timing](../../../reference/12_simulation_model/01_tick_timing_and_main_loop.md) R-SIM-570); the engine converts each of them to audio time (AEN-062, AEN-190).

**AEN-007** (MUST) The dedicated server and every headless build link no part of the audio engine (ADR-022). A headless participant raises the same presentation events and plays nothing.

**AEN-008** (MUST) The audio design is the authority for every design rule this component implements: the event set, layer data and defaults, audiences, sharing, envelopes and loops as heard, links, groups, the effect kinds and their behaviour, the distance and panning laws, the buses, their levels and priority classes, and the music hooks ([audio design](../../../reference/10_audio_design/README.md) R-AUD-006). This component cites the design's requirements and `ref.aud.*` keys and never redefines them; it owns the arithmetic, the scheduling and every engine value of its tuning table (§4.21).

## 2 Interfaces

### 2.1 Interface sketch

Everything in the sketch is engine code in namespace `Engine` (`AGENTS.md` R9). The audio design's game concepts reach it only as engine values: presentation code passes the simulation's object handle (`SimObjectId`) as an emitter id (`EmitterId`) and a team as an audience group (`AudienceGroup`), and the audiences, links and match phases are named for what the engine compares and counts.

```cpp
namespace Neuron
{

using AudioSeconds = double; // audio time in s: the audio clock, which reads the platform clock (ADR-074)

// Data tokens of the audio design in brackets; order kept where noted.
enum class SoundPosition : std::uint8_t
{
  Screen2D,   // [2d]
  WorldFixed, // [fix]
  Attached    // [att]
};
enum class SoundSharing : std::uint8_t
{
  PerTrigger,   // [poly]
  SharedRandom, // [share-rnd]
  SharedNearest // [share-near]
};
enum class SoundLoop : std::uint8_t
{
  Once,    // [once]
  Loop,    // [loop]
  NoteLoop // [note]
};
enum class SoundAudience : std::uint8_t
{
  All,        // [all]
  OwnGroup,   // [own]
  OtherGroup, // [enemy]
  NoGroup     // [none]
};
// SoundParamKind, SoundParamUpdate, SoundLink, SoundParam, EffectBinding, SoundLayerDef, SoundEventDef, SoundSourceDef:
// generated from sound.sources: data_schemas/entity_audio_geometry_and_panel_schemas.md §2
enum class EnvelopeStage : std::uint8_t // no decay stage
{
  Attack,
  Hold,
  Release
};
enum class VoiceState : std::uint8_t
{
  Pending,
  Virtual,
  Real,
  Retired
};
// DspFilterKind (its order carries meaning, R-AUD-080), DspParamDesc, DspFilterDef:
// generated from sound.effect_kinds: data_schemas/entity_audio_geometry_and_panel_schemas.md §2
// Order is the priority-class order, highest first (DEC-570); Music is outside the ranking.
enum class AudioBus : std::uint8_t
{
  Ui,
  Announcer,
  Voice,
  World,
  Ambience,
  Music
};
inline constexpr std::size_t AUDIO_BUS_COUNT = 6;
enum class StopMatch : std::uint8_t
{
  ExactEvent,
  SourcePrefix,
  Everything
};
enum class MatchPhase : std::uint8_t
{
  NoMatch,
  Countdown, // the match's grace period (R-AUD-303)
  Running,
  Ended
};

struct EmitterId // the simulation's object handle as the engine names an emitter; EmitterId{} = no emitter
{
  std::uint64_t value = 0;
};
struct AudienceGroup // the group that an audience compares; presentation code passes the team
{
  std::uint8_t value = 0;
};
struct SourceId // a sound source, resolved to its index in the cooked sound data (AEN-064)
{
  std::uint16_t value = 0;
};
struct EventId // a full event id <source>.<event>, resolved to its index in the cooked sound data (AEN-064)
{
  std::uint32_t value = 0;
};
struct BankId // a bank of AEN-074, by its index in the cooked sound data
{
  std::uint8_t value = 0;
};
struct RequestId // chosen by the caller of a query and returned in its AudioResult
{
  std::uint32_t value = 0;
};
struct OperationSetId // the XAudio2 operation set of one update, passed to IXAudio2::CommitChanges (AEN-123)
{
  std::uint32_t value = 0;
};

struct SoundVoiceId // generation 0 = invalid
{
  std::uint32_t slot = 0;
  std::uint32_t generation = 0;
};

struct AudioSettings // the settings of AEN-076, read when the engine is created
{
  std::uint8_t masterVolumeStep;                            // 0 to ref.meta.options.sound.volume_max
  std::array<std::uint8_t, AUDIO_BUS_COUNT> busVolumeSteps; // in AudioBus order
  bool swapStereo;
  bool effectsEnabled;
};

// The cooked sound data that AEN-100 step 1 loads: the SoundSourceDef records and the SoundGroupDef sample groups (§3.1).
struct SoundDataSet;
// The cooked bus graph: the player buses of AEN-220 and the effect buses of AEN-223 with their DSP filter chains.
// VoiceBackend::Build turns it into the AudioBusGraph, the submix voices of §4.16.
struct BusGraphDef;
class SampleCache;     // residency: banks, group reference counts and stream heads (§4.17)
class SourceVoiceSlot; // one pooled IXAudio2SourceVoice with its callback object (AEN-142)

struct VoiceFormat // the format of a voice pool (AEN-142); every format is at aen.format.sample_rate
{
  std::uint8_t channels;         // 1 or 2
  bool msAdpcm;                  // false: 16-bit PCM
  std::uint16_t samplesPerBlock; // MS-ADPCM only (aen.stream.samples_per_block)
};
struct SendLevel // one send of a source voice to an effect bus (AEN-224)
{
  std::uint16_t effectBus; // the effect bus's index in the BusGraphDef
  float amplitude;
};

struct TriggerCommand // AEN-130
{
  SourceId source; // the full id is <source>.<event>
  EventId event;
  EmitterId emitter;                  // EmitterId{} = no emitter
  std::optional<AudienceGroup> group; // nullopt = no team (R-AUD-025)
  Vec3 position;                      // world frame, wu
  Vec3 velocity;                      // wu/s
  AudioSeconds issueTime;             // audio clock at issue (AEN-024)
};

struct StopCommand
{
  EmitterId emitter;
  StopMatch match;
  SourceId source;
  EventId event;
  AudioSeconds issueTime;
};

struct EmitterUpdate // AEN-041
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

class CommandWriter // one per producing thread; every call writes one command
{
public:
  bool Trigger(const TriggerCommand& _command); // false: the queue is full; the command is dropped and counted
  bool Stop(const StopCommand& _command);
  bool UpdateEmitters(std::span<const EmitterUpdate> _updates);
  bool SetListener(const ListenerUpdate& _listener);
  bool SetMatchState(const MatchState& _state);
  bool BeginMatch(AudienceGroup _localGroup); // per-match state starts (AEN-197)
  bool EndMatch();                            // per-match state discarded
  bool SetLoadingShown(bool _shown);          // AEN-111
  bool SetMasterVolume(std::uint8_t _step);   // 0 to ref.meta.options.sound.volume_max
  bool SetBusVolume(AudioBus _bus, std::uint8_t _step);
  bool SetSwapStereo(bool _on);
  bool SetEffectsEnabled(bool _on); // AEN-221
  bool LoadBank(BankId _bank);
  bool ReleaseBank(BankId _bank);
  bool QueryCount(RequestId _request, EmitterId _emitter, EventId _event); // EmitterId{} = any emitter
  bool QueryPlaying(RequestId _request, EmitterId _emitter);
  bool Pause();
  bool Resume();
  bool RestartDevice(); // AEN-259
};

enum class ResultKind : std::uint8_t // AEN-023
{
  Count,       // the answer to QueryCount: request, count
  Playing,     // the answer to QueryPlaying: request, playing
  DeviceReady, // the readiness points of AEN-104
  InterfaceReady,
  MatchReady,
  EmitterTracked, // the tracked-emitter set of AEN-041 gains the emitter
  EmitterUntracked
};

struct AudioResult
{
  ResultKind kind;
  RequestId request;
  std::uint32_t count;
  bool playing;
  EmitterId emitter;
};

class AudioEngine
{
public:
  static std::unique_ptr<AudioEngine> Create(const AudioSettings& _settings, const SoundDataSet& _data, SampleCache& _cache);
  CommandWriter& Producer();                                        // the calling thread's queue (ADR-026)
  void DrainResults(std::function<void(const AudioResult&)> _sink); // game thread, once per frame
  ~AudioEngine();                                                   // AEN-106 teardown order
};

// Layer 2; the only code that includes XAudio2 or X3DAudio headers (ADR-026).
class VoiceBackend
{
public:
  bool Build(const BusGraphDef& _graph); // mastering voice on the default device and format, buses
  void Teardown();
  SourceVoiceSlot* Acquire(const VoiceFormat& _format); // pooled per format: PCM, and MS-ADPCM for streams (AEN-142)
  void Release(SourceVoiceSlot* _voice);                // resets every per-voice setting (DEC-752)
  void SetVolume(SourceVoiceSlot& _voice, float _amplitude, OperationSetId _set);
  void SetFrequencyRatio(SourceVoiceSlot& _voice, float _ratio, OperationSetId _set);
  void SetOutputMatrix(SourceVoiceSlot& _voice, std::span<const float> _matrix, OperationSetId _set);
  void SetFilter(SourceVoiceSlot& _voice, const XAUDIO2_FILTER_PARAMETERS& _filter, OperationSetId _set);
  void SetSends(SourceVoiceSlot& _voice, std::span<const SendLevel> _sends, OperationSetId _set);
  void Submit(SourceVoiceSlot& _voice, const XAUDIO2_BUFFER& _buffer);
  void Start(SourceVoiceSlot& _voice, OperationSetId _set);
  void Stop(SourceVoiceSlot& _voice, OperationSetId _set);
  void ApplyOperationSet(OperationSetId _set); // IXAudio2::CommitChanges
};

} // namespace Neuron
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
| `SetLoadingShown` | the loading-view gate | AEN-111 |
| `SetMasterVolume`, `SetBusVolume` | player volumes | AEN-215 |
| `SetSwapStereo` | swap left and right | AEN-205 |
| `SetEffectsEnabled` | the effects switch | AEN-221 |
| `LoadBank`, `ReleaseBank` | residency | AEN-244 |
| `QueryCount`, `QueryPlaying` | answered in the result queue | AEN-136 |
| `Pause`, `Resume` | stop and resume the advance and output of every instance | AEN-138 |
| `RestartDevice` | rebuilds the graph | AEN-259 |

### 2.3 Inputs from the simulation and presentation

**AEN-040** (MUST) The simulation emits sound cues as typed presentation events at the tick where the rule fires (DEC-007, DEC-222 item 5). Presentation code (the audio bridge, outside this component) turns each cue into a `Trigger` or `Stop` command. The engine depends only on receiving commands in issue order; whether cues arrive in-process or from a server is decided with the threading and networking models (DEC-222, DEC-224). Under the default of DEC-318 the fuse-blink and match-clock sounds are raised by drawing code, so they arrive only while drawn; the engine treats them like any trigger. Interface code raises, as `Trigger` commands, the interface events of the match UI sound source that pages, widgets and HUD panels raise ([pages and screen recipes](../ui_framework/pages_and_screen_recipes.md) UI-315; R-AUD-300); no page, widget or panel plays a sound directly.

**AEN-041** (MUST) Once per frame, presentation sends one `EmitterUpdate` for each emitter that the engine lists as tracked, read from the frame's presentation snapshot ([renderer](../renderer/README.md) RND-018, RND-019): the position (a structure's centre position for a structure), the velocity, the height above the terrain at the position, the age, and the counts of teams and of actors near the position used by the `groups_nearby` and `objects_nearby` links (`EmitterUpdate::groupsNearby`, `EmitterUpdate::objectsNearby`). The engine publishes its tracked-emitter set in the result queue whenever it changes: every emitter of an attached or shared instance, and every emitter whose instance has a linked value that reads emitter data.

**AEN-042** (MUST) Once per frame, presentation sends the listener (the camera's position, front, up and velocity, and the terrain height under the camera for the `height` link of two-dimensional instances) and the match state (phase, local team, game speed, match time remaining). The audio engine itself queries nothing.

**AEN-043** (MUST) Presentation sends these commands in every application loop that shows a frame: the match, the front end and the loading view. The engine's update runs whether or not any arrive (AEN-120).

### 2.4 Voice back-end contract

**AEN-045** (MUST) The voice back-end (`VoiceBackend`) is the engine's one device layer, on XAudio2 with X3DAudio (ADR-022, ADR-027), and the only code that includes their headers (ADR-026):

| Operation | Voice back-end |
|---|---|
| `Build` | one mastering voice on the default device with the default format, the bus graph of §4.16 built from its `BusGraphDef`, and the voice pools of AEN-142 |
| `Acquire`, `Release` | a pooled source voice of a `VoiceFormat`, started with an offset (AEN-148, AEN-182) and released with every setting reset (AEN-046) |
| `SetVolume`, `SetFrequencyRatio`, `SetOutputMatrix`, `SetFilter`, `SetSends` | the voice volume in dB (AEN-214), the frequency ratio (AEN-196), the output matrix of the engine's panning (§4.14), the per-voice filter and the send levels to effect buses (AEN-207, AEN-224); no voice carries an effect chain (ADR-024) |
| `Submit` | buffers into a bank or a stream ring, and silences from one shared silent buffer of `aen.voice.silence_buffer` (AEN-174); XAudio2 callbacks only signal (AEN-025) |
| `ApplyOperationSet` | every voice change of one update, applied as one operation set (AEN-123) |

The real-voice budget, the caps and the music voice pair are §4.6 and §8.2; the listener is `SetListener` (AEN-209).

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
| `DspFilterDef` | 1 | one of the nine DSP filter kinds: its parameter descriptors (§3.4) | compiled into the engine from the effect-kind table `sound.effect_kinds`, with no pack entry (DAT-273); process lifetime |
| `SoundParam` | 4 | the definition of one modulatable value: kind, link, update mode, ranges, smoothing (R-AUD-051 to R-AUD-054) | part of a layer or an effect binding |
| `SoundVoice` | 4 | one instance (§3.3) | the engine's instance table of `aen.instances.capacity` slots; from trigger to retirement |
| `SoundVoiceId` | 4 | a weak handle {slot, generation}; two handles are equal when both fields are; generation 0 is invalid; each new instance takes the next generation of its slot, starting at 1 | value type |
| `SampleCache` | 3 | residency: banks, group reference counts, stream heads (§4.17) | the engine; process lifetime |
| `SampleBank` | 3 | a set of resident groups loaded and released together | `SampleCache`; its scope (AEN-244) |
| `AudioStream` | 3 | the read position and buffer ring of one real streamed instance; XAudio2 decodes its MS-ADPCM buffers | `SampleCache`; while its instance is real |
| `VoiceBackend` | 2 | the XAudio2 engine, mastering voice, voice pools and bus graph | the engine; process lifetime, rebuilt on device loss |
| `SourceVoiceSlot` | 2 | one pooled `IXAudio2SourceVoice` with its callback object | its pool |
| `AudioBusGraph` | 2 | the submix voices of §4.16, which `VoiceBackend::Build` creates from the cooked bus graph `BusGraphDef` | `VoiceBackend`; rebuilt with it |
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

**AEN-070** (MUST) The Species DSP filter kinds are the nine kinds that the bus effects of ADR-024 realise: the three kinds that XAudio2's own effects realise (room reverb, parametric equaliser, and stereo echo with equal delays and no pan delay), the studio reverb, which the event set binds ([event model](../../../reference/10_audio_design/event_model_and_sound_data.md) R-AUD-082), and the five own-algorithm kinds of R-AUD-079. They keep the relative order of R-AUD-077, which carries meaning (R-AUD-080). No other kind exists: the chorus, compressor, distortion, flanger and standard gargle of R-AUD-077 are named by no effect binding and realised by no XAudio2 effect, and a binding that names one is a content-build error (DEC-339). A `DspFilterDef` lists, for each parameter of its kind, in the order of [data schemas](../data_schemas/entity_audio_geometry_and_panel_schemas.md) table 3.5-A: the name, the minimum, the maximum, the declared default, whether it is an integer, and its schema default. A layer's effect binding gives its parameters by position (R-AUD-050), at most `aen.dsp.max_params` of them; an integer parameter takes its evaluated value truncated toward zero. The parameters of the standard kinds 0 to 3 are engine values of ADR-024, given below as range and declared default (i = integer); those of the own-algorithm kinds 4 to 8 are R-AUD-077's. The kinds are realised as follows, with routing as defined in AEN-224:

| # | Kind | Parameters (range, declared default) | Species realisation | Routing |
|---|---|---|---|---|
| 0 | stereo echo | wet/dry mix 0 to 100 %, 50; feedback 0 to 100 %, 50; left delay 1 to 2000 ms, 500; right delay 1 to 2000 ms, 500; pan delay 0 to 1, 0 (i) | built-in `FXEcho` when the left and right delays are equal and the pan delay is 0 (mix ÷ 100, feedback ÷ 100, delay); otherwise Species XAPO `StereoEchoFx` | send |
| 1 | room reverb | room −10000 to 0 (hundredths of a dB), −1000 (i); room high-frequency −10000 to 0, −100 (i); room roll-off factor 0 to 10, 0; decay time 0.1 to 20 s, 1.49; decay high-frequency ratio 0.1 to 2, 0.83; reflections −10000 to 1000, −2602 (i); reflections delay 0 to 0.3 s, 0.01; reverb −10000 to 2000, 200 (i); reverb delay 0 to 0.1 s, 0.01; diffusion 0 to 100 %, 100; density 0 to 100 %, 100; high-frequency reference 20 to 20000 Hz, 5000 | the XAudio2 reverb (`XAudio2CreateReverb`): the 12 parameters, in order, are converted to its native parameters with `ReverbConvertI3DL2ToNative` at a wet/dry mix of 100 | send, with direct and send levels of 1 |
| 2 | parametric equaliser | centre 80 to 16000 Hz, 8000; bandwidth 1 to 36 semitones, 12; gain −15 to 15 dB, 0 | built-in `FXEQ`: band 0 at the centre, gain converted from dB to an amplitude ratio, bandwidth ÷ 12 octaves; bands 1 to 3 at unity gain | insert |
| 3 | studio reverb | input gain −96 to 0 dB, 0; reverb mix −96 to 0 dB, 0; reverb time 0.01 to 3000 ms, 1000; high-frequency time ratio 0.01 to 0.99, 0.01 | Species XAPO `StudioReverbFx` | insert |
| 4 | resonant low-pass | R-AUD-077 | Species XAPO `ResonantLowPassFx` (AEN-230) | insert |
| 5 | bit crusher | R-AUD-077 | Species XAPO `BitCrusherFx` (AEN-231) | insert |
| 6 | gargle | R-AUD-077 | Species XAPO `GargleFx` (AEN-232) | send for the triangle shape, insert for the square shape |
| 7 | echo | R-AUD-077 | Species XAPO `EchoFx` (AEN-233) | send |
| 8 | simple reverb | R-AUD-077 | Species XAPO `SimpleReverbFx` (AEN-234) | insert |

**AEN-071** (MUST) **Schema defaults.** The schema default of every effect parameter is its declared default (AEN-070 for the standard kinds, R-AUD-077 for the own-algorithm kinds), not 0.0. A binding that omits a parameter therefore takes the declared default, where the reference data takes 0.0 (R-AUD-050, R-AUD-078). The choice is deliberate (DEC-567, [data schemas](../data_schemas/README.md) DAT-012):

1. 14 of the 35 effect parameters have a range that excludes 0.0 (the delays of both echoes, the decay time, both high-frequency ratios and the high-frequency reference of the reverbs, the reverb time, the centre and bandwidth of the equaliser, the cut-off and resonance controls, the gargle rate and the bit depth), so a 0.0 default would make an omission a range error, and two of them make a sound unusable: a bit depth of 0 quantises every sample to silence, and an echo delay of 0 has no delay line;
2. every effect binding of the reference event set states all its parameters ([event model](../../../reference/10_audio_design/event_model_and_sound_data.md) R-AUD-082, the 15 effect bindings), so the choice changes nothing in the reference set;
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

**AEN-076** (MUST) The engine reads these settings from the settings store when it is created ([options](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) R-META-429, R-META-433 to R-META-435, R-META-454; [data schemas](../data_schemas/config_input_and_strings.md) table 3.1-A), and takes every later change as the command of §2.2 that the options send; their defaults are those of the defaults table (DEC-100). Every setting is immediate: it takes effect at the next update, and no setting restarts the engine or a sound (R-META-437, AEN-261).

| Setting | Default | Effect |
|---|---|---|
| master volume | `aen.settings.volume_default` (255) | the pre-master gain (AEN-215); `SetMasterVolume` |
| bus volumes, one per bus of R-AUD-221 | `aen.settings.volume_default` (255) | the bus gains (AEN-215); `SetBusVolume` |
| swap left and right | `aen.settings.swap_stereo_default` (off) | AEN-205; `SetSwapStereo` |
| effects switch | `aen.settings.effects_default` (off) | AEN-221; `SetEffectsEnabled` |

The engine has no setting for the output device, the mixing rate or the number of voices: it plays on the default device in that device's format (ADR-022, AEN-260), and its real-voice budget is `aen.voice.budget` (DEC-570, DEC-869).

**AEN-077** (MUST) Every group draw chooses among all the group's members (AEN-180); no setting restricts the members a group may draw (R-AUD-065, DEC-097).

## 4 Behaviour and algorithms

Sections 4.5 to 4.13 are in [instances_voices_and_envelopes.md](instances_voices_and_envelopes.md) and sections 4.14 to 4.19 in [spatialisation_mixing_and_streaming.md](spatialisation_mixing_and_streaming.md).

### 4.1 Lifecycle

**AEN-100** (MUST) The engine is created after the settings store ([options](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md) R-META-402, R-META-478) and initialises in this order:

1. take the nine DSP filter-kind definitions compiled into the engine (AEN-070, DAT-273), then load the cooked sound data: the sound sources with their events, layers and groups; a source kind that has no definitions receives an empty definition, so every source index resolves;
2. build the voice back-end: the XAudio2 engine, the mastering voice on the default device with the default format, the bus graph (§4.16) and the voice pools (AEN-142); if this fails, the engine runs on the silent device (AEN-321);
3. start the update thread (AEN-120) and the stream task (AEN-302);
4. request the interface bank (AEN-074) from the asset system, asynchronously;
5. report each readiness point in the result queue (AEN-104).

**AEN-102** (MUST) A match load requests the match bank. The match loop starts only after the match bank and its stream heads are resident, so no match trigger is dropped for residency once the match runs; the wait is part of the load and is reported as load progress (DEC-226). The wait counts towards the match-entry budget `ast.load.match_entry_budget_s` ([asset pipeline](../asset_pipeline_and_resources.md) AST-130).

**AEN-103** (MUST) The content build validates the sound data (DEC-015, DEC-801, ADR-059): group members are named by asset id; a priority field is ignored (R-AUD-049); an unknown effect kind, an unknown source, event or field, a duplicate key or group, a malformed block and a stray blank line inside a block are build errors with their location (R-AUD-068); effect parameters are cooked in their kind's order, and a binding with more parameters than its kind is a build error (AEN-071).

**AEN-104** (MUST) The engine has three readiness points, each reported once in the result queue: **device ready** (the graph is built, or the silent device is in use), **interface ready** (the interface bank is resident) and **match ready** (the match bank is resident). A trigger issued before the readiness point of its group is dropped, not deferred (AEN-111).

**AEN-106** (MUST) Teardown is the reverse of creation (DEC-221 item 3): stop accepting commands; stop and join the update thread; stop and join the stream task; stop every voice; destroy the source voices, the effect buses, the player buses, the pre-master bus and the mastering voice; release the XAudio2 engine; release the banks. Every wait is bounded by `aen.lifecycle.teardown_wait`, after which the remaining step is abandoned with a diagnostic (DEC-221 item 7).

### 4.2 The trigger gate

**AEN-110** (MUST) A trigger returns at once and is **dropped, not queued**, when the gate of AEN-111 closes on it. The lobby bed therefore starts on the first front-end update after the interface bank, with the bed's stream head, is resident ([front end, HUD, voice and ambience](../../../reference/10_audio_design/front_end_hud_voice_and_ambience.md) R-AUD-321).

**AEN-111** (MUST) A trigger is dropped, counted by reason, and never deferred, when: the bank or stream head of a group of its layers is not resident (AEN-104); the loading-view gate is set (`SetLoadingShown(true)`), which drops every trigger (R-AUD-048, R-AUD-325, DEC-319); or the instance table is full (AEN-300). Stops, queries and every other command pass the gate.

**AEN-112** (MUST) A trigger from any thread is accepted through that thread's queue (AEN-020; ADR-026, DEC-224 item 12).

### 4.3 Where the audio update runs

**AEN-116** (MUST) The update runs on the engine's own thread in every application state, at its own rate, whatever the frame rate; the application loops only send commands (AEN-043). The threading model that places this thread is DEC-224's.

### 4.4 Update schedule and step order

**AEN-120** (MUST) The engine runs one update every `aen.update.period` of audio time, equal to one XAudio2 processing quantum, so the changes of one update take effect within one quantum. Each update reads the audio clock once (T) and uses T, or the elapsed time Δt since the previous update, for every timed quantity. An update that starts late runs once; there are no catch-up updates, and lateness changes only granularity.

**AEN-121** (MUST) The update runs at its own period whatever the application does: while a match is paused, while the frame rate is low, with no device (AEN-321), and while the command-stream recording hook drives the presentation frame delta at its fixed capture rate, because the audio clock stays real (R-SIM-740). The update rate changes only the granularity of timed quantities, never their duration, because every timed quantity reads the audio clock (AEN-120).

**AEN-123** (MUST) The update runs these steps in order:

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

**AEN-264** (MUST) **Start latency.** A trigger reaches the device `aen.update.schedule_latency` after its issue time plus at most one processing quantum and the device's own latency; a burst of triggers is decided in the update that drains it, so no trigger waits for a start quota (AEN-147).

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
| `aen.pitch_bend.time_constant` | 0.17 | s | the smoothing of the game speed that the bend follows | AEN-197 | DEC-751 |
| `aen.spatial.max_distance_default` | 1 000 000 000 | wu | beyond every distance in a map | AEN-062, AEN-202 | DEC-566 |
| `aen.spatial.max_distance_fade_fraction` | 0.1 | of dmax | | AEN-202 | DEC-566 |
| `aen.spatial.doppler_factor_default` | 0 | × | off | AEN-062, AEN-207 | DEC-566 |
| `aen.spatial.filter_factor_default` | 0 | × | off | AEN-062, AEN-207 | DEC-566 |
| `aen.spatial.reverb_factor_default` | 0 | × | off | AEN-062, AEN-207 | DEC-566 |
| `aen.listener.velocity_scale` | 0.2 | × | camera velocity to listener velocity | AEN-209 | — |
| `aen.mix.duck_ramp` | 0.05 | s | | AEN-216 | DEC-570 |
| `aen.mix.limiter.release` | 6 | — | `FXMasteringLimiter` range 1 to 20 | AEN-220 | DEC-567 |
| `aen.mix.limiter.loudness` | 1 000 | — | range 1 to 1 800 | AEN-220 | DEC-567 |
| `aen.dsp.max_params` | 12 | parameters | the largest kind | AEN-070 | — |
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
| `aen.settings.volume_default` | 255 | steps | master and every bus | AEN-076 | DEC-100 |
| `aen.settings.swap_stereo_default` | off | — | | AEN-076 | DEC-100 |
| `aen.settings.effects_default` | off | — | | AEN-076, AEN-221 | DEC-567 |
| `aen.perf.update_cpu_budget` | 0.5 | ms per update | on the reference test machine (DBG-209) at full instance capacity | AEN-303 | — |

## 5 Concurrency and performance budget

**AEN-300** (MUST) The instance table holds `aen.instances.capacity` instances and is allocated when the engine is created; when it is full, a new trigger is dropped and counted. Command and result queues, voice pools and stream rings are also allocated at creation, so the update thread does not allocate during a session except for bank bookkeeping.

**AEN-301** (MUST) Threads and what each touches: producing threads write only their own command queue; the update thread alone owns every instance, every definition at run time and every voice setting; the stream task alone writes stream buffers; the XAudio2 processing thread reads only submitted buffers and resident bank memory, which no thread writes while the bank is resident. No other thread touches audio memory (DEC-224 item 12).

**AEN-302** (MUST) The stream task is a recurring task that the engine owns, woken by buffer-free notices and by seek requests; it reads the next MS-ADPCM buffers of each stream from the packs through the content reader and refills them in the order of the notices, and it never decodes, because XAudio2 decodes ADPCM itself (ADR-025). Which worker runs it follows the threading model (DEC-224, ADR-025).

**AEN-303** (MUST) An update costs O(n log n) in its n live instances (the ranking sort) plus O(r) in its r real voices, and stays within `aen.perf.update_cpu_budget` on the reference test machine (DBG-209) with the table full (test T27).

**AEN-304** (MUST) Memory stays within the budgets of AEN-243: the resident groups of the interface bank within `aen.memory.interface_bank_budget`, those of the match bank within `aen.memory.match_bank_budget`, and every stream ring and stream head together within `aen.memory.stream_budget`. The content build measures every bank of the authored sound set, as cooked, against its budget at every build and fails on an excess (AEN-243, T25); the run-time memory per bank and for streams is shown by the development overlay (AEN-323) and checked by T27. The starting budgets are sized from the durations of the asset manifest: the resident groups hold about 1 888 s of audio in 868 files, about 173 MiB of 16-bit mono at 48 kHz, of which the interface bank takes about 2 MiB; the 34 members of the laser-bolt, grenade-throw and game-over groups, whose durations the manifest does not yet give, add at most about 3.4 MiB; authoring the two-dimensional stings in stereo would add about 14 MiB. The streams need `aen.stream.max_streams` rings of `aen.stream.blocks_per_stream` MS-ADPCM buffers, under 1 MiB, plus the heads.

## 6 Failure modes, diagnostics and telemetry

**AEN-320** (MUST) Failures and their handling; none stops the program (DEC-015):

| Failure | Handling |
|---|---|
| no device, or the graph cannot be built | silent device (AEN-321) |
| device lost or invalidated | rebuild (AEN-259) |
| cooked sound data missing or invalid | an empty sound set and a diagnostic; every trigger does nothing |
| group or member missing | the layer is silent, with a diagnostic (R-AUD-046) |
| bank not resident at a trigger, loading view, table full, queue full | the trigger is dropped and counted by reason (AEN-021, AEN-111, AEN-300) |
| stream corrupt or missing | the instance is retired with a diagnostic (AEN-256) |
| frequency ratio out of range | AEN-196, counted |
| start later than its start time | started at once, counted (AEN-148, AEN-254) |
| stream buffer not read when its voice needs it | the voice plays silence for the gap, counted as an underrun |

**AEN-321** (MUST) **Silent device** (R-AUD-044). With no XAudio2 graph, the engine runs its full update with zero real voices: every instance is virtual, one-shots retire by their patience, loops live until they are stopped or lose their emitter, and queries are answered, so waiting instances never accumulate over a session (DEC-097).

**AEN-322** (MUST) **Profiling** (ADR-068). The update, each of its steps (AEN-123), the stream task and each bank load are bracketed by profiling markers (`ENGINE_PROFILE_SCOPE`); the engine counts the cost of the triggers per entry kind and of stream reads per event id. The audio markers follow ADR-068: they are the `audio.` markers of the marker catalogue of the [diagnostics component](../diagnostics_test_and_performance/README.md).

**AEN-323** (MUST) **Development overlay** (ADR-022). Debug builds show: real and virtual instances per bus; the highest-ranked instances with their scores; the peak level of each player bus; memory per bank and for streams; drops by reason; late starts; one-shots retired unheard; steals per second; stream underruns. Debug builds also reload the cooked sound data while the game runs, at an update boundary; live instances keep the layer copy they started with.

**AEN-324** (MUST) The engine logs in its own category (ADR-067): readiness, bank loads and releases, device changes and rebuilds, and every diagnostic of AEN-320; it writes a memory summary every `ref.sim.frame.memory_log_interval`.

**AEN-325** (MUST) The engine sends no telemetry of its own; its counters are available to the diagnostics component ([diagnostics, test and performance](../diagnostics_test_and_performance/README.md)).

## 7 Acceptance tests

| Test | Kind | Checks | Requirements |
|---|---|---|---|
| T1 queue contract | unit | commands from three producers apply in issue-time order; a full queue drops the newest command, returns false and counts it; no producer call blocks | AEN-020 to AEN-022 |
| T2 callbacks | checker fixture | the callback translation units call no allocator, lock, logger or voice creation, and only the voice back-end includes XAudio2 or X3DAudio headers (`Build\CheckProjectFiles.py`, build tooling table 4.1-B row 14) | AEN-025, AEN-003 |
| T3 integrity | property | a recorded match replayed with audio on and off gives the same per-tick state hashes; audio code includes no simulation-generator or writable simulation header (build tooling table 4.1-B rows 13 and 14) | AEN-003, AEN-004, AEN-005 |
| T4 audience and merge | unit | own and enemy layers follow the local team (R-AUD-024); a no-team event creates only all and no-team layers; a second `worker.spawn` adds an emitter, not an instance | AEN-130, AEN-131 |
| T5 emitter at trigger | unit | a query right after a trigger counts it, for every position kind; a stop in the same frame retires it unheard | AEN-134, AEN-136 |
| T6 counts in every state | unit | counts are correct with pending, virtual, real and retired instances, with and without an emitter, and over empty slots | AEN-136 |
| T7 copy decay | unit | four equal instances of one event score a, a − 2.50, a − 5.00 and a − 7.49 dB | AEN-141 |
| T8 budget and caps | property | over recorded battle trigger streams, real instances never exceed the budget, a bus cap or an event cap | AEN-143, AEN-144 |
| T9 margin and hold | property | no instance changes between virtual and real more than once per hold time except by ending or inaudibility; a candidate within the margin never steals | AEN-145 |
| T10 worked example | unit | the example of AEN-150 gives the outcomes stated | AEN-150 |
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
| T27 performance | performance | an update with 1 024 live instances and 64 real voices stays within `aen.perf.update_cpu_budget` on the reference test machine (DBG-209); memory stays within the budgets | AEN-303, AEN-304 |
| T28 device change | integration | switching and unplugging the default device, running and paused, continues every sound from its position | AEN-259, AEN-261 |
| T29 silent device | unit | with no device, a session of triggers keeps the live count bounded and answers every query | AEN-321 |
| T30 effect defaults | unit | a binding that omits a parameter takes its declared default; a binding with too many parameters or an out-of-range value is a build error | AEN-071 |

## 8 Open decisions

### 8.1 Decisions and architecture records

Open: DEC-044, DEC-049, DEC-109, DEC-318, DEC-320, DEC-321, DEC-322, DEC-566, DEC-567, DEC-965; DEC-339 is Dropped; the others are Resolved.

| Decision | Where | Default specified |
|---|---|---|
| DEC-013 | AEN-004 | own presentation generator |
| DEC-027 | AEN-143 to AEN-145, AEN-220 | binding model kept; priorities and mix replaced |
| DEC-042 | AEN-214 | levels in dB |
| DEC-044 | AEN-076, AEN-215 | the master volume plus the bus volumes; no voice-budget setting |
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
| DEC-339 | AEN-070, AEN-225 | device-level details not specified; the nine Species DSP filter kinds; every effect computed at its running rate |
| DEC-566 | AEN-202, AEN-209 | camera listener, no effective maximum distance |
| DEC-567 | AEN-071, AEN-221, AEN-222 | dry; declared parameter defaults (DAT-012) |
| DEC-568 | AEN-180 | weighted draws without repeats |
| DEC-569, DEC-751 | AEN-197 | per-match, time-based bend |
| DEC-570 | AEN-143, AEN-145 | six buses, 64 voices, 3 dB margin |
| DEC-752 | AEN-046, AEN-136, AEN-221 | no stale voice state |
| DEC-801 | AEN-103 | duplicates are build errors |
| DEC-965 | AEN-133, AEN-138, AEN-140, AEN-178 | next note at the first member wrap; audibility by envelope level; shared note layers merge; no pause |

The audio ADRs, as amended, agree with this component; it applies them as follows:

- **ADR-024 and DEC-570.** The bus graph is data-defined, and its baseline has the six player buses of DEC-570 and R-AUD-221 under the pre-master bus (AEN-220); the bus effects realise the nine DSP filter kinds of AEN-070, room reverb with the XAudio2 reverb.
- **ADR-027 and DEC-566.** The per-event maximum distance exists, and its global default lies beyond every distance in a map (AEN-202), so the default mix has no maximum distance until DEC-566 is decided.
- **ADR-023 and the copy decay.** The copy decay and the patience window are per-event design values whose defaults are the audio design's keys; the engine owns the arithmetic, the budget, the caps, the margin and the hold time (AEN-063).
- **ADR-022 and the type names.** `SoundVoice` names an instance (layer 4), and the pooled source voice is `SourceVoiceSlot` (layer 2).
- **ADR-026.** A command's issue time is the audio clock, a real-time reading of the platform clock, not the game-speed-scaled presentation clock.
- **ADR-025.** The streaming threshold applies to a group's longest member, the banks are those of AEN-074, and their budgets are those of AEN-243. Streams are MS-ADPCM, encoded by the content build tool and decoded by XAudio2; seeks and loops work on whole blocks (AEN-072, AEN-252 to AEN-255).

### 8.2 Music hook

**AEN-360** (MUST) Until DEC-049 is decided, the engine provides the music channel of [music hooks and brief](../../../reference/10_audio_design/music_hooks_and_sound_design_brief.md) R-AUD-400 to R-AUD-405 as a hook: one playing and one pending music instance; a request for the member that is playing is ignored; any other request replaces the pending one, which is retired, and releases the playing track finally; at the first update after the playing track has finished, the pending one starts, with its attack timed from its request (R-AUD-404); each update advances the music instance fully (envelope, pitch, loops and ends by AEN-166 and AEN-175 to AEN-178); music never enters the ranking and is never positioned, bent or effected; a stop reaches it only when it names the `music` source (R-AUD-403).

**AEN-361** (MUST) The music bus's `aen.voice.bus_cap.music` voices are reserved for the music channel, so that a music system can cross-fade two streams with sample-accurate timing in one operation set (ADR-024, R-AUD-414).

### 8.3 Design-phase inputs

1. **Threads and transport.** Where the update thread and the stream task run (DEC-224), and whether emitter and listener updates come from an in-process snapshot or from a server (DEC-222).
2. **A persistent world.** Bank scopes for a world of many regions instead of one match bank: residency per region or biome, streaming ahead of the camera, and a budget per scope (DEC-226); how emitters far from the camera are culled when a region holds many more emitters than a skirmish map.
3. **Listening point and distance limits** (DEC-566, DEC-109), including per-event maximum distances for strategic zoom.
4. **Effects** (DEC-567): dry, reverb send and limiter, or per-event effects, and the release lengths that go with the choice (AEN-228).
5. **Music** (DEC-049) on the reserved music voices (AEN-361).
6. **Options** (DEC-044): which bus volumes the front end shows, and whether the options offer a voice-budget setting.
7. **Pause**: whether a paused skirmish, or a suspended application, pauses audio (AEN-138, DEC-965 item 4).
8. **Development tools**: a sound-tuning tool over the data reload of AEN-323, and whether it has a sound editor, parameter editors and statistics windows; this library does not specify them.
9. **Accessibility** (ADR-064): a mono output option and visual cues for important sounds.
