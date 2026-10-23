#pragma once

#include "AudioDevice.h"
#include "FxRng.h"
#include "PlatformClock.h"
#include "PlatformThread.h"
#include "SoundAsset.h"
#include "SpscQueue.h"
#include "Vec3.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Neuron
{

// An event's index in the engine's table, resolved when the table is made (AEN-064).
using SoundEventId = std::uint16_t;

/// A sound event at the MVP's size (ADR-172): one layer of one sample group, a one-shot (`once`), either at its
/// trigger's position (`fix`) or two-dimensional (`2d`), with a fixed level; its members, its bus, its minimum distance
/// and its cap of real voices (AEN-143).
struct SoundEventDef
{
  std::string name;                                       // for the log and the statistics
  std::vector<std::shared_ptr<const SoundAsset>> members; // the group's members, in member order; none: silent (AEN-182)
  AudioBus bus = AudioBus::World;
  bool positioned = true;
  double levelDb = -15.0;     // ref.aud.layer.level_default
  double minDistance = 100.0; // ref.aud.layer.min_distance_default, wu
  std::uint32_t voiceCap = 4; // aen.voice.event_cap.world_default
};

/// The listener (AEN-209, DEC-566): the camera's position and orientation in the world frame, +Y up.
struct AudioListener
{
  Vec3 position;
  Vec3 front{0.0, 0.0, 1.0};
  Vec3 up{0.0, 1.0, 0.0};
};

/// The audio engine's tuning, from the `aen.*` and `ref.aud.*` keys of the game's records (the engine knows no record).
struct AudioSettings
{
  Nanoseconds updatePeriod{10'000'000};    // aen.update.period
  Nanoseconds scheduleLatency{30'000'000}; // aen.update.schedule_latency
  Nanoseconds patience{150'000'000};       // ref.aud.layer.one_shot_patience_updates × ref.sim.frame.audio_update_period
  std::size_t commandCapacity = 4'096;     // aen.queue.command_capacity
  std::size_t instanceCapacity = 1'024;    // aen.instances.capacity
  std::uint32_t voiceBudget = 64;          // aen.voice.budget
  std::array<std::uint32_t, AUDIO_BUS_COUNT> busCaps{4, 4, 16, 32, 4, 2};             // aen.voice.bus_cap.*
  std::array<double, AUDIO_BUS_COUNT> busLevelsDb{0.0, 0.0, -3.0, -3.0, -10.0, -6.0}; // ref.aud.bus.*.level
  double stealMarginDb = 3.0;                                                         // aen.voice.steal_margin_db
  Nanoseconds stateHold{250'000'000};                                                 // aen.voice.state_hold
  Nanoseconds stealFade{20'000'000};                                                  // aen.voice.steal_fade
  double maxDistance = 1e9;                                                           // aen.spatial.max_distance_default, wu
  double maxDistanceFadeFraction = 0.1;                                               // aen.spatial.max_distance_fade_fraction
  double floorDb = -100.0;                                                            // ref.aud.mix.floor_db
  double masterStepDb = 0.32;                                                         // ref.aud.mix.master_step_db
  unsigned volumeMax = 255;                                                           // ref.meta.options.sound.volume_max
  double copyDecay = 0.75;                                                            // ref.aud.rank.copy_decay
  double duckWorldDb = -6.0;                                                          // ref.aud.bus.announcer.duck_world_db
  Nanoseconds duckTime{1'000'000'000};                                                // ref.aud.bus.announcer.duck_time
  std::uint64_t rngSeed = 1;                                                          // aen.rng.match_seed
  unsigned masterVolumeStep = 255;                                                    // audio.master_volume at the start (AEN-076)
};

/// What the engine published after its latest update (AEN-137, AEN-323).
struct AudioStatistics
{
  std::uint32_t live = 0; // started instances, real and virtual
  std::uint32_t real = 0;
  std::array<std::uint32_t, AUDIO_BUS_COUNT> realPerBus{};
  std::uint64_t updates = 0;
  std::uint64_t commandsDropped = 0; // a full queue (AEN-021)
  std::uint64_t triggersDropped = 0; // a full instance table or an unknown event (AEN-111)
  std::uint64_t retiredUnheard = 0;  // one-shots past their patience that never became real (AEN-146)
  std::uint64_t steals = 0;          // AEN-145
  bool deviceLost = false;           // AEN-259: every instance plays virtual until the device is rebuilt
};

/// The audio engine at the MVP's size (M3 task T15, ADR-022 to ADR-027 as ADR-172 narrows them): the game thread only
/// writes commands into a bounded lock-free queue (ADR-026); the engine's own thread updates every `aen.update.period`
/// on the audio clock, the platform's monotonic clock (AEN-006): it starts the triggers that are due, ranks every live
/// instance by its bus class and its audibility with the copy decay (ADR-023), keeps the real set within the event, bus
/// and budget caps, steals by the margin, retires one-shots past their patience, sets each real voice's distance gain
/// and constant-power pan (ADR-027), ducks the world bus under the announcer, and commits the update's changes together.
/// Without a device, or once the device is lost, every instance is virtual and the update runs alike (AEN-321).
class AudioEngine
{
public:
  struct Desc
  {
    AudioSettings settings;
    std::vector<SoundEventDef> events;
    std::unique_ptr<AudioDevice> device; // none: the silent device
    const TimeSource* clock = nullptr;   // the audio clock, outliving the engine
    bool ownThread = true;               // false: the owner calls Update, as the tests do
  };

  explicit AudioEngine(Desc _desc);
  ~AudioEngine();

  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;

  // The game thread's commands (AEN-020); false when the queue is full and the command is dropped (AEN-021). One
  // thread writes them at the MVP: the client's main thread.
  bool Trigger(SoundEventId _event, const Vec3& _position);
  bool SetListener(const AudioListener& _listener);
  bool SetMasterVolume(unsigned _step);

  // One update at the clock's time (AEN-123): the engine's thread calls it every period, or the owner without one.
  void Update();

  [[nodiscard]] AudioStatistics Statistics() const noexcept;
  [[nodiscard]] std::size_t EventCount() const noexcept
  {
    return m_events.size();
  }

private:
  enum class CommandKind : std::uint8_t
  {
    Trigger,
    Listener,
    MasterVolume
  };

  struct Command
  {
    CommandKind kind = CommandKind::Trigger;
    Nanoseconds issued{0};
    SoundEventId event = 0;
    Vec3 position;
    Vec3 front;
    Vec3 up;
    unsigned step = 0;
  };

  enum class InstanceState : std::uint8_t
  {
    Free,
    Pending,
    Virtual,
    Real
  };

  struct Instance
  {
    InstanceState state = InstanceState::Free;
    SoundEventId event = 0;
    std::uint32_t member = 0;
    Vec3 position;
    Nanoseconds start{0};
    Nanoseconds origin{0}; // when its first frame played: set as it first becomes real (AEN-146)
    Nanoseconds stateSince{0};
    std::optional<AudioVoice> voice;
    bool everReal = false;
    double gain = 1.0;      // the distance gain
    double audibleDb = 0.0; // its level after the distance gain
    double scoreDb = 0.0;   // with the copy decay
    std::uint32_t rank = 0; // its place in the update's order
    bool audible = false;
  };

  struct Fading
  {
    AudioVoice voice = 0;
    Nanoseconds until{0};
  };

  bool Push(const Command& _command);
  void Apply(const Command& _command);
  std::uint32_t DrawMember(SoundEventId _event);
  [[nodiscard]] static std::size_t FramesPlayed(const Instance& _instance, const SoundAsset& _sound, Nanoseconds _now) noexcept;
  void Retire(Instance& _instance);
  void Demote(Instance& _instance, Nanoseconds _now);
  bool TryStart(Instance& _instance, Nanoseconds _now);
  void Evaluate(Nanoseconds _now);
  void Allocate(Nanoseconds _now);
  void Publish();

  AudioSettings m_settings;
  std::vector<SoundEventDef> m_events;
  std::unique_ptr<AudioDevice> m_device;
  const TimeSource* m_clock;
  SpscQueue<Command> m_commands;
  std::vector<Instance> m_instances;
  std::vector<std::uint32_t> m_order;    // the started instances' slots in rank order
  std::vector<std::uint32_t> m_real;     // the real instances' slots
  std::vector<std::uint32_t> m_perEvent; // the real instances of each event
  std::vector<Fading> m_fading;
  std::vector<std::uint32_t> m_lastMember; // per event, the member drawn last (AEN-180)
  AudioListener m_listener;
  unsigned m_masterStep = 255;
  Nanoseconds m_duckUntil{0};
  Nanoseconds m_nextRebuild{0};
  FxRng m_rng;                  // the engine's own presentation generator (AEN-004)
  AudioStatistics m_statistics; // the update thread's
  std::atomic<std::uint64_t> m_commandsDropped{0};
  // The published statistics, each written by the update thread and read by any.
  std::atomic<std::uint32_t> m_publishedLive{0};
  std::atomic<std::uint32_t> m_publishedReal{0};
  std::array<std::atomic<std::uint32_t>, AUDIO_BUS_COUNT> m_publishedRealPerBus{};
  std::atomic<std::uint64_t> m_publishedUpdates{0};
  std::atomic<std::uint64_t> m_publishedTriggersDropped{0};
  std::atomic<std::uint64_t> m_publishedRetiredUnheard{0};
  std::atomic<std::uint64_t> m_publishedSteals{0};
  std::atomic<bool> m_publishedDeviceLost{false};
  PlatformThread m_thread; // last, so that it stops before the rest is destroyed
};

} // namespace Neuron
