#include "pch.h"

#include "AudioEngine.h"
#include "AudioMath.h"
#include "Geometry.h"
#include "PlatformClock.h"

#include <chrono>
#include <cmath>
#include <memory>
#include <optional>
#include <set>
#include <thread>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

using Neuron::Nanoseconds;

constexpr Nanoseconds MILLISECOND{1'000'000};

// A device that plays nothing and records what the engine asked of it.
struct RecordingDevice final : Neuron::AudioDevice
{
  struct Started
  {
    const Neuron::SoundAsset* sound = nullptr;
    std::size_t startFrame = 0;
    Neuron::AudioBus bus = Neuron::AudioBus::World;
    float amplitude = 0.0f;
    std::array<float, 2> pan{};
  };

  explicit RecordingDevice(std::uint32_t _voices)
    : busy(_voices, false),
      ended(_voices, false),
      amplitudes(_voices, 0.0f),
      pans(_voices)
  {
  }

  [[nodiscard]] std::uint32_t OutputChannels() const noexcept override
  {
    return 2;
  }
  [[nodiscard]] std::optional<Neuron::AudioVoice> Start(const Neuron::SoundAsset& _sound, std::size_t _startFrame, Neuron::AudioBus _bus,
                                                        float _amplitude, const std::array<float, 2>& _pan) override
  {
    if (refuse)
    {
      return std::nullopt;
    }
    for (std::uint32_t voice = 0; voice < busy.size(); ++voice)
    {
      if (!busy[voice])
      {
        busy[voice] = true;
        ended[voice] = false;
        amplitudes[voice] = _amplitude;
        pans[voice] = _pan;
        started.push_back({&_sound, _startFrame, _bus, _amplitude, _pan});
        return voice;
      }
    }
    return std::nullopt;
  }
  void Set(Neuron::AudioVoice _voice, float _amplitude, const std::array<float, 2>& _pan) override
  {
    amplitudes[_voice] = _amplitude;
    pans[_voice] = _pan;
  }
  void Stop(Neuron::AudioVoice _voice) override
  {
    busy[_voice] = false;
    ++stops;
  }
  [[nodiscard]] bool Ended(Neuron::AudioVoice _voice) noexcept override
  {
    return ended[_voice];
  }
  void SetBusGain(Neuron::AudioBus _bus, float _amplitude) override
  {
    busGains[static_cast<std::size_t>(_bus)] = _amplitude;
  }
  void SetPreMasterGain(float _amplitude) override
  {
    preMasterGain = _amplitude;
  }
  void Commit() override
  {
    ++commits;
  }
  [[nodiscard]] bool Lost() const noexcept override
  {
    return lost;
  }
  [[nodiscard]] bool Rebuild() override
  {
    ++rebuilds;
    lost = stayLost;
    busy.assign(busy.size(), false);
    return !lost;
  }

  std::vector<bool> busy;
  std::vector<bool> ended;
  std::vector<float> amplitudes;
  std::vector<std::array<float, 2>> pans;
  std::vector<Started> started;
  std::array<float, Neuron::AUDIO_BUS_COUNT> busGains{};
  float preMasterGain = 0.0f;
  std::uint32_t stops = 0;
  std::uint32_t commits = 0;
  std::uint32_t rebuilds = 0;
  bool refuse = false;
  bool lost = false;
  bool stayLost = false;
};

std::shared_ptr<const Neuron::SoundAsset> Tone(double _seconds)
{
  auto sound = std::make_shared<Neuron::SoundAsset>();
  sound->samples.assign(static_cast<std::size_t>(_seconds * Neuron::SOUND_SAMPLE_RATE), 0);
  return sound;
}

Neuron::SoundEventDef Event(Neuron::AudioBus _bus, bool _positioned, double _levelDb, std::uint32_t _voiceCap, double _seconds)
{
  Neuron::SoundEventDef event;
  event.name = "test";
  event.members = {Tone(_seconds)};
  event.bus = _bus;
  event.positioned = _positioned;
  event.levelDb = _levelDb;
  event.voiceCap = _voiceCap;
  return event;
}

// An engine updated by the test on a manual clock, through a recording device of _voices voices.
struct Rig
{
  Rig(const Neuron::AudioSettings& _settings, std::vector<Neuron::SoundEventDef> _events, std::uint32_t _voices = 64)
  {
    auto recording = std::make_unique<RecordingDevice>(_voices);
    device = recording.get();
    Neuron::AudioEngine::Desc desc;
    desc.settings = _settings;
    desc.events = std::move(_events);
    desc.device = std::move(recording);
    desc.clock = &clock;
    desc.ownThread = false;
    engine = std::make_unique<Neuron::AudioEngine>(std::move(desc));
  }

  // The clock moves on by _step, then the engine updates.
  void Step(Nanoseconds _step)
  {
    clock.Advance(_step);
    engine->Update();
  }

  // Updates every period until the clock has moved on by _span.
  void Run(Nanoseconds _span)
  {
    const Nanoseconds period{10'000'000};
    for (Nanoseconds moved{0}; moved < _span; moved += period)
    {
      Step(period);
    }
  }

  Neuron::ManualTimeSource clock;
  RecordingDevice* device = nullptr;
  std::unique_ptr<Neuron::AudioEngine> engine;
};

bool Near(double _expected, double _actual, double _tolerance = 1e-5)
{
  return std::abs(_expected - _actual) <= _tolerance;
}

double Amplitude(double _decibels)
{
  return std::pow(10.0, _decibels / 20.0);
}

} // namespace

TEST_CLASS(AudioEngineTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Aen024ATriggerStartsAfterTheScheduleLatency)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-024")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-123")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-137")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen024ATriggerStartsAfterTheScheduleLatency)
  {
    Rig rig({}, {Event(Neuron::AudioBus::World, false, 0.0, 4, 1.0)});
    Assert::IsTrue(rig.engine->Trigger(0, {}), L"the trigger is queued");
    rig.engine->Update();
    rig.Step(10 * MILLISECOND);
    Assert::IsTrue(rig.device->started.empty() && rig.engine->Statistics().live == 0, L"not before the latency");
    rig.Step(10 * MILLISECOND);
    const Neuron::AudioStatistics statistics = rig.engine->Statistics();
    Assert::IsTrue(rig.device->started.size() == 1 && rig.device->started[0].startFrame == 0, L"started in the update it is due");
    Assert::IsTrue(statistics.live == 1 && statistics.real == 1 && statistics.realPerBus[3] == 1 && statistics.updates == 3,
                   L"the statistics count it");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen143TheCapsAndTheBudgetBoundTheRealInstances)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-143")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-144")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-146")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen143TheCapsAndTheBudgetBoundTheRealInstances)
  {
    Neuron::AudioSettings settings;
    settings.voiceBudget = 5;
    Rig rig(settings, {Event(Neuron::AudioBus::World, false, 0.0, 2, 1.0), Event(Neuron::AudioBus::World, false, 0.0, 10, 1.0)});
    for (int n = 0; n < 4; ++n)
    {
      rig.engine->Trigger(0, {});
    }
    for (int n = 0; n < 6; ++n)
    {
      rig.engine->Trigger(1, {});
    }
    rig.Run(30 * MILLISECOND);
    Neuron::AudioStatistics statistics = rig.engine->Statistics();
    Assert::IsTrue(statistics.real == 5 && statistics.live == 10, L"the budget holds five real");
    std::uint32_t firstEvent = 0;
    for (const RecordingDevice::Started& started : rig.device->started)
    {
      firstEvent += started.sound == rig.device->started[0].sound ? 1U : 0U;
    }
    Assert::IsTrue(firstEvent == 2, L"the event cap holds the first event to two");
    rig.Run(160 * MILLISECOND);
    statistics = rig.engine->Statistics();
    Assert::IsTrue(statistics.live == 5 && statistics.retiredUnheard == 5, L"the rest are retired unheard after their patience");

    settings.voiceBudget = 64;
    settings.busCaps[3] = 3;
    Rig bus(settings, {Event(Neuron::AudioBus::World, false, 0.0, 6, 1.0)});
    for (int n = 0; n < 5; ++n)
    {
      bus.engine->Trigger(0, {});
    }
    bus.Run(30 * MILLISECOND);
    Assert::IsTrue(bus.engine->Statistics().real == 3, L"the bus cap holds three real");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen145ALouderCandidateStealsOnceBothHaveHeldTheirStates)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-141")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-145")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen145ALouderCandidateStealsOnceBothHaveHeldTheirStates)
  {
    Neuron::AudioSettings settings;
    settings.patience = Nanoseconds{2'000'000'000};
    Neuron::SoundEventDef shot = Event(Neuron::AudioBus::World, true, 0.0, 1, 3.0);
    shot.minDistance = 100.0;
    Rig rig(settings, {shot});
    rig.engine->Trigger(0, {0.0, 0.0, 10'000.0}); // 40 dB down
    rig.Run(30 * MILLISECOND);
    rig.engine->Trigger(0, {0.0, 0.0, 50.0}); // at full level, due at 60 ms
    rig.Run(240 * MILLISECOND);
    Assert::IsTrue(rig.engine->Statistics().steals == 0 && rig.device->started.size() == 1, L"no steal before the candidate's hold");
    rig.Run(40 * MILLISECOND);
    Assert::IsTrue(rig.engine->Statistics().steals == 1 && rig.device->started.size() == 2, L"the candidate steals the voice");
    Assert::IsTrue(rig.device->amplitudes[0] == 0.0f && rig.device->stops == 0, L"the victim fades first");
    rig.Run(20 * MILLISECOND);
    Assert::IsTrue(rig.device->stops == 1 && !rig.device->busy[0], L"its voice returns after the steal fade");
    Assert::IsTrue(rig.engine->Statistics().real == 1 && rig.engine->Statistics().live == 2, L"the victim plays virtual");

    Rig even(settings, {shot});
    even.engine->Trigger(0, {0.0, 0.0, 50.0});
    even.Run(30 * MILLISECOND);
    even.engine->Trigger(0, {0.0, 0.0, 50.0});
    even.Run(Nanoseconds{1'000'000'000});
    Assert::IsTrue(even.engine->Statistics().steals == 0, L"a copy as loud scores lower and never steals");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen146AOneShotNeverRealStartsAtItsFirstFrame)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-146")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen146AOneShotNeverRealStartsAtItsFirstFrame)
  {
    Rig rig({}, {Event(Neuron::AudioBus::World, false, 0.0, 4, 1.0)});
    rig.device->refuse = true;
    rig.engine->Trigger(0, {});
    rig.Run(100 * MILLISECOND);
    rig.device->refuse = false;
    rig.Step(10 * MILLISECOND);
    Assert::IsTrue(rig.device->started.size() == 1 && rig.device->started[0].startFrame == 0, L"it starts from its first frame");

    rig.device->refuse = true;
    rig.engine->Trigger(0, {});
    rig.Run(200 * MILLISECOND);
    Assert::IsTrue(rig.engine->Statistics().retiredUnheard == 1, L"one never heard is retired after its patience");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen175AOneShotEndsAtItsSamplesEnd)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-175")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-179")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen175AOneShotEndsAtItsSamplesEnd)
  {
    Rig rig({}, {Event(Neuron::AudioBus::World, false, 0.0, 4, 0.1)});
    rig.engine->Trigger(0, {});
    rig.Run(30 * MILLISECOND); // real from 20 ms, the update its start falls in
    rig.Run(80 * MILLISECOND);
    Assert::IsTrue(rig.engine->Statistics().live == 1 && rig.device->stops == 0, L"still playing after 90 ms");
    rig.Run(10 * MILLISECOND);
    Assert::IsTrue(rig.engine->Statistics().live == 0 && rig.device->stops == 1, L"ended at its last frame");

    rig.engine->Trigger(0, {});
    rig.Run(30 * MILLISECOND);
    rig.device->ended[0] = true;
    rig.Step(10 * MILLISECOND);
    Assert::IsTrue(rig.engine->Statistics().live == 0 && rig.device->stops == 2, L"ended when its voice says so");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen180AMemberIsNeverDrawnTwiceInARow)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-180")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen180AMemberIsNeverDrawnTwiceInARow)
  {
    Neuron::SoundEventDef event = Event(Neuron::AudioBus::World, false, 0.0, 32, 1.0);
    event.members = {Tone(1.0), Tone(1.0), Tone(1.0)};
    Rig rig({}, {event});
    for (int n = 0; n < 30; ++n)
    {
      rig.engine->Trigger(0, {});
    }
    rig.Run(30 * MILLISECOND);
    const std::vector<RecordingDevice::Started>& started = rig.device->started;
    Assert::IsTrue(started.size() == 30, L"each starts, in its trigger's order");
    std::set<const Neuron::SoundAsset*> drawn;
    for (std::size_t index = 0; index < started.size(); ++index)
    {
      drawn.insert(started[index].sound);
      Assert::IsTrue(index == 0 || started[index].sound != started[index - 1].sound, L"never the member drawn last");
    }
    Assert::IsTrue(drawn.size() == 3, L"every member is drawn");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen204APositionedSoundIsAttenuatedAndPannedFromTheListener)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-201")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-204")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-209")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-210")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen204APositionedSoundIsAttenuatedAndPannedFromTheListener)
  {
    Neuron::SoundEventDef shot = Event(Neuron::AudioBus::World, true, -15.0, 4, 3.0);
    shot.minDistance = 100.0;
    Rig rig({}, {shot, Event(Neuron::AudioBus::Announcer, false, 0.0, 4, 3.0)});
    Neuron::AudioListener listener;
    listener.position = {100.0, 50.0, 100.0};
    listener.front = {0.0, 0.0, 1.0};
    listener.up = {0.0, 1.0, 0.0};
    rig.engine->SetListener(listener);
    const Neuron::Vec3 right = Neuron::Cross(listener.front, listener.up);
    rig.engine->Trigger(0, listener.position + right * 200.0);
    rig.engine->Trigger(1, {});
    rig.Run(30 * MILLISECOND);
    Assert::IsTrue(rig.device->started.size() == 2, L"both start");
    const RecordingDevice::Started& positioned = rig.device->started[1]; // the announcer ranks first
    Assert::IsTrue(Near(Amplitude(-15.0 + 20.0 * std::log10(0.5)), positioned.amplitude), L"its level less the distance gain");
    Assert::IsTrue(Near(0.0, positioned.pan[0]) && Near(1.0, positioned.pan[1]), L"on the listener's right");
    const RecordingDevice::Started& flat = rig.device->started[0];
    Assert::IsTrue(Near(1.0, flat.amplitude) && Near(Neuron::CenteredPan()[0], flat.pan[0]) && Near(Neuron::CenteredPan()[1], flat.pan[1]),
                   L"a two-dimensional sound is centered at its level");

    listener.front = {0.0, 0.0, -1.0};
    rig.engine->SetListener(listener);
    rig.Step(10 * MILLISECOND);
    Assert::IsTrue(Near(1.0, rig.device->pans[1][0]) && Near(0.0, rig.device->pans[1][1]), L"turned around, it is on the left");
    listener.front = listener.up;
    rig.engine->SetListener(listener);
    rig.Step(10 * MILLISECOND);
    Assert::IsTrue(Near(1.0, rig.device->pans[1][0]), L"a front parallel to up keeps the orientation");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen215TheMasterVolumeSetsThePreMasterGain)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-215")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-076")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen215TheMasterVolumeSetsThePreMasterGain)
  {
    Rig rig({}, {Event(Neuron::AudioBus::World, false, 0.0, 4, 1.0)});
    rig.engine->Update();
    Assert::IsTrue(Near(1.0, rig.device->preMasterGain), L"the top step is unity");
    Assert::IsTrue(Near(Amplitude(-3.0), rig.device->busGains[3]) && Near(Amplitude(-10.0), rig.device->busGains[4]),
                   L"each bus at its level");
    rig.engine->SetMasterVolume(0);
    rig.Step(10 * MILLISECOND);
    Assert::IsTrue(Near(Amplitude(-81.6), rig.device->preMasterGain), L"step 0 is 81.6 dB down");
    rig.engine->SetMasterVolume(400);
    rig.Step(10 * MILLISECOND);
    Assert::IsTrue(Near(1.0, rig.device->preMasterGain) && rig.device->commits == 3, L"a step above the top is the top");

    Neuron::AudioSettings quiet;
    quiet.masterVolumeStep = 128;
    Rig start(quiet, {});
    start.engine->Update();
    Assert::IsTrue(Near(Amplitude(-0.32 * 127.0), start.device->preMasterGain), L"the setting at the start is applied");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen216TheAnnouncerDucksTheWorldBus)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-216")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen216TheAnnouncerDucksTheWorldBus)
  {
    Rig rig({}, {Event(Neuron::AudioBus::Announcer, false, 0.0, 4, 0.5)});
    rig.engine->Trigger(0, {});
    rig.Run(30 * MILLISECOND); // starting at 30 ms, in the update at 20 ms
    Assert::IsTrue(Near(Amplitude(-9.0), rig.device->busGains[3]), L"the world bus is ducked 6 dB");
    Assert::IsTrue(Near(1.0, rig.device->busGains[1]), L"the announcer bus is not");
    rig.Run(Nanoseconds{980'000'000});
    Assert::IsTrue(Near(Amplitude(-9.0), rig.device->busGains[3]), L"still ducked at 1.01 s");
    rig.Run(30 * MILLISECOND);
    Assert::IsTrue(Near(Amplitude(-3.0), rig.device->busGains[3]), L"restored after 1 s");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen021AFullQueueDropsTheNewestCommand)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-020")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-021")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen021AFullQueueDropsTheNewestCommand)
  {
    Neuron::AudioSettings settings;
    settings.commandCapacity = 2;
    Rig rig(settings, {Event(Neuron::AudioBus::World, false, 0.0, 4, 1.0)});
    Assert::IsTrue(rig.engine->Trigger(0, {}) && rig.engine->Trigger(0, {}), L"two fit");
    Assert::IsFalse(rig.engine->Trigger(0, {}), L"the third is dropped");
    Assert::IsTrue(rig.engine->Statistics().commandsDropped == 1, L"and counted");
    rig.engine->Update();
    Assert::IsTrue(rig.engine->SetMasterVolume(200), L"the update drained the queue");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen111UnknownSilentAndOverflowingTriggersAreDropped)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-111")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-300")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen111UnknownSilentAndOverflowingTriggersAreDropped)
  {
    Neuron::AudioSettings settings;
    settings.instanceCapacity = 2;
    Neuron::SoundEventDef silent = Event(Neuron::AudioBus::World, false, 0.0, 4, 1.0);
    silent.members.clear();
    Rig rig(settings, {silent, Event(Neuron::AudioBus::World, false, 0.0, 4, 1.0)});
    rig.engine->Trigger(0, {});
    rig.engine->Trigger(7, {});
    for (int n = 0; n < 3; ++n)
    {
      rig.engine->Trigger(1, {});
    }
    rig.Run(30 * MILLISECOND);
    const Neuron::AudioStatistics statistics = rig.engine->Statistics();
    Assert::IsTrue(statistics.triggersDropped == 3 && statistics.live == 2, L"a silent, an unknown and a third trigger");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen321WithoutADeviceEveryInstanceIsVirtual)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-321")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen321WithoutADeviceEveryInstanceIsVirtual)
  {
    Neuron::ManualTimeSource clock;
    Neuron::AudioEngine::Desc desc;
    desc.events = {Event(Neuron::AudioBus::World, false, 0.0, 4, 1.0)};
    desc.clock = &clock;
    desc.ownThread = false;
    Neuron::AudioEngine engine(std::move(desc));
    engine.Trigger(0, {});
    for (int n = 0; n < 4; ++n)
    {
      clock.Advance(10 * MILLISECOND);
      engine.Update();
    }
    Neuron::AudioStatistics statistics = engine.Statistics();
    Assert::IsTrue(statistics.live == 1 && statistics.real == 0 && !statistics.deviceLost, L"it plays virtual");
    for (int n = 0; n < 20; ++n)
    {
      clock.Advance(10 * MILLISECOND);
      engine.Update();
    }
    statistics = engine.Statistics();
    Assert::IsTrue(statistics.live == 0 && statistics.retiredUnheard == 1 && statistics.updates == 24, L"and is retired unheard");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen259ALostDeviceIsRebuiltAndItsInstancesResume)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-259")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-260")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen259ALostDeviceIsRebuiltAndItsInstancesResume)
  {
    Neuron::AudioSettings settings;
    settings.patience = Nanoseconds{2'000'000'000};
    Rig rig(settings, {Event(Neuron::AudioBus::World, false, 0.0, 4, 3.0)});
    rig.engine->Trigger(0, {});
    rig.Run(30 * MILLISECOND); // real from 20 ms
    rig.device->lost = true;
    rig.device->stayLost = true;
    rig.Step(10 * MILLISECOND);
    Neuron::AudioStatistics statistics = rig.engine->Statistics();
    Assert::IsTrue(statistics.deviceLost && statistics.real == 0 && statistics.live == 1, L"lost, the instance plays virtual");
    Assert::IsTrue(rig.device->rebuilds == 1, L"the device is rebuilt at once");
    rig.Run(500 * MILLISECOND);
    Assert::IsTrue(rig.device->rebuilds == 1, L"and tried again only after a second");
    rig.device->stayLost = false;
    rig.Run(500 * MILLISECOND);
    statistics = rig.engine->Statistics();
    Assert::IsTrue(rig.device->rebuilds == 2 && !statistics.deviceLost && statistics.real == 1, L"rebuilt, it plays again");
    const std::size_t expected = static_cast<std::size_t>((1.04 - 0.02) * Neuron::SOUND_SAMPLE_RATE);
    const std::size_t resumed = rig.device->started.back().startFrame;
    Assert::IsTrue(rig.device->started.size() == 2 && resumed + 48 >= expected && resumed <= expected + 48, L"from where it would be");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen121TheEnginesThreadUpdatesOnItsPeriod)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-121")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-106")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen121TheEnginesThreadUpdatesOnItsPeriod)
  {
    const Neuron::CounterTimeSource clock;
    Neuron::AudioEngine::Desc desc;
    desc.events = {Event(Neuron::AudioBus::World, false, 0.0, 4, 1.0)};
    desc.clock = &clock;
    auto engine = std::make_unique<Neuron::AudioEngine>(std::move(desc));
    engine->Trigger(0, {});
    const Nanoseconds deadline = clock.Now() + Nanoseconds{5'000'000'000};
    while (engine->Statistics().updates < 5 && clock.Now() < deadline)
    {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    Assert::IsTrue(engine->Statistics().updates >= 5, L"the engine's thread updates");
    engine.reset();
  }
};

} // namespace NeuronClientTests
