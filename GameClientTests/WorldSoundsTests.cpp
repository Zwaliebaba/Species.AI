#include "pch.h"

#include "AudioMath.h"
#include "Geometry.h"
#include "LooseContent.h"
#include "SoundAsset.h"
#include "TestSupport.h"
#include "WorldSounds.h"

#include <cmath>
#include <numbers>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

// A cooked resident sound of _frames silent mono frames under _key, so a test tells the members apart by their length.
Neuron::CookedEntry SoundEntry(std::string _key, std::size_t _frames)
{
  Neuron::SoundAsset sound;
  sound.samples.assign(_frames, 0);
  Neuron::CookedEntry entry;
  entry.entry.key = std::move(_key);
  entry.entry.type = Neuron::ContentType::ResidentSound;
  entry.bytes = Neuron::WriteSoundAsset(sound);
  return entry;
}

Species::ReceivedEvent Received(std::uint64_t _tick, Species::EffectEventKind _kind, const Neuron::Vec3& _position = {})
{
  Species::ReceivedEvent received;
  received.tick = _tick;
  received.event.kind = _kind;
  received.event.position = _position;
  return received;
}

bool IsCue(const std::optional<Species::SoundCue>& _cue, Species::GameSound _sound)
{
  return _cue.has_value() && _cue->sound == _sound;
}

bool Near(double _expected, double _actual)
{
  return std::abs(_expected - _actual) <= 1e-9;
}

} // namespace

TEST_CLASS(WorldSoundsTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp087EachEventPlaysItsGroupsMembersOnItsBus)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-087")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-064")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-143")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-182")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp087EachEventPlaysItsGroupsMembersOnItsBus)
  {
    const TemporaryFolder folder;
    std::vector<Neuron::CookedEntry> entries;
    entries.push_back(SoundEntry("audio/sfx/laser_bolt/laser_bolt_spawn_02", 200));
    entries.push_back(SoundEntry("audio/sfx/laser_bolt/laser_bolt_spawn_01", 100));
    entries.push_back(SoundEntry("audio/sfx/worker/worker_die_01", 300));
    entries.push_back(SoundEntry("audio/sfx/worker/worker_die_b_01", 400)); // another group's
    entries.push_back(SoundEntry("audio/ui/ui_battle_start", 500));
    Assert::IsTrue(Neuron::WriteLooseContent(folder.Path(), std::move(entries)).has_value(), L"the content is written");
    const auto content = Neuron::LooseContent::Mount(folder.Path());
    Assert::IsTrue(content.has_value(), L"and mounted");

    const Species::TuningPresentation& presentation = CommittedPresentation();
    const std::vector<Neuron::SoundEventDef> events = Species::GameSoundEvents(*content, presentation);
    Assert::IsTrue(events.size() == Species::GAME_SOUND_COUNT, L"one event per sound, in its order");
    const Neuron::SoundEventDef& laser = events[static_cast<std::size_t>(Species::GameSound::LaserShot)];
    Assert::IsTrue(laser.members.size() == 2 && laser.members[0]->Frames() == 100 && laser.members[1]->Frames() == 200,
                   L"the laser shot's members in key order");
    Assert::IsTrue(laser.bus == Neuron::AudioBus::World && laser.positioned, L"positioned on the world bus");
    Assert::IsTrue(laser.voiceCap == static_cast<std::uint32_t>(presentation.aen.voice.eventCap.worldMax), L"at the world's most");
    Assert::IsTrue(Near(presentation.mvp.sound.laserShot.levelDb, laser.levelDb) &&
                     Near(presentation.mvp.sound.minDistance, laser.minDistance),
                   L"at its level and the minimum distance");
    const Neuron::SoundEventDef& death = events[static_cast<std::size_t>(Species::GameSound::Death)];
    Assert::IsTrue(death.members.size() == 1 && death.members[0]->Frames() == 300, L"another group's member is not the death's");
    const Neuron::SoundEventDef& completed = events[static_cast<std::size_t>(Species::GameSound::StructureCompleted)];
    Assert::IsTrue(completed.members.empty() && completed.positioned, L"a group with no member is silent");
    Assert::IsTrue(completed.voiceCap == static_cast<std::uint32_t>(presentation.aen.voice.eventCap.worldDefault),
                   L"at the world's default cap");
    const Neuron::SoundEventDef& start = events[static_cast<std::size_t>(Species::GameSound::BattleStarted)];
    Assert::IsTrue(start.members.size() == 1 && start.bus == Neuron::AudioBus::Announcer && !start.positioned,
                   L"the battle's start is two-dimensional on the announcer bus");
    Assert::IsTrue(start.voiceCap == static_cast<std::uint32_t>(presentation.aen.voice.busCap.announcer), L"capped by its bus");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr171AGroupsMembersAreItsStemAndNumberedKeys)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-064")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr171AGroupsMembersAreItsStemAndNumberedKeys)
  {
    const std::string_view stem = Species::SoundStemOf(Species::GameSound::Death);
    Assert::IsTrue(stem == "audio/sfx/worker/worker_die", L"the death's stem");
    Assert::IsTrue(Species::IsMemberKey("audio/sfx/worker/worker_die_01", stem), L"a numbered member");
    Assert::IsTrue(Species::IsMemberKey("audio/sfx/worker/worker_die", stem), L"a group of one");
    Assert::IsFalse(Species::IsMemberKey("audio/sfx/worker/worker_die_b_01", stem), L"another group");
    Assert::IsFalse(Species::IsMemberKey("audio/sfx/worker/worker_die_", stem), L"no number");
    Assert::IsFalse(Species::IsMemberKey("audio/sfx/worker/worker_dies", stem), L"a longer name");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp087EventsAndNoticesCueTheirSounds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-087")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-042")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-043")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-045")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp087EventsAndNoticesCueTheirSounds)
  {
    const Neuron::Vec3 at{10.0, 2.0, -30.0};
    const auto cueOf = [&](Species::EffectEventKind _kind) { return Species::CueOf(Received(5, _kind, at).event); };
    Assert::IsTrue(IsCue(cueOf(Species::EffectEventKind::LaserFired), Species::GameSound::LaserShot), L"a laser shot");
    Assert::IsTrue(IsCue(cueOf(Species::EffectEventKind::FigureDied), Species::GameSound::Death), L"a death");
    Assert::IsTrue(IsCue(cueOf(Species::EffectEventKind::OrbBecameWorker), Species::GameSound::OrbBecameWorker),
                   L"an orb becoming a Worker");
    const std::optional<Species::SoundCue> completed = cueOf(Species::EffectEventKind::StructureCompleted);
    Assert::IsTrue(IsCue(completed, Species::GameSound::StructureCompleted) && completed.value_or(Species::SoundCue{}).position == at,
                   L"at the event's position");
    Assert::IsFalse(cueOf(Species::EffectEventKind::FigureHurt).has_value(), L"a hurt plays nothing at the MVP");

    Species::SettlementNotice notice;
    notice.kind = Species::NoticeKind::AttackDeclared;
    Assert::IsTrue(IsCue(Species::CueOf(notice), Species::GameSound::AttackWarning), L"the attack's warning");
    notice.kind = Species::NoticeKind::BattleStarted;
    Assert::IsTrue(IsCue(Species::CueOf(notice), Species::GameSound::BattleStarted), L"a battle starting");
    notice.kind = Species::NoticeKind::BattleEnded;
    Assert::IsTrue(IsCue(Species::CueOf(notice), Species::GameSound::BattleEnded), L"and ending");
    notice.kind = Species::NoticeKind::StructureCompleted;
    Assert::IsFalse(Species::CueOf(notice).has_value(), L"a structure's notice leaves its sound to its event");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr084AnEventsSoundPlaysWithItsDraw)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-087")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr084AnEventsSoundPlaysWithItsDraw)
  {
    Species::SoundSchedule schedule;
    const std::vector<Species::ReceivedEvent> events{Received(10, Species::EffectEventKind::LaserFired),
                                                     Received(10, Species::EffectEventKind::FigureHurt),
                                                     Received(11, Species::EffectEventKind::FigureDied)};
    Assert::IsTrue(schedule.Due(events, 8.5).empty() && schedule.Waiting() == 2, L"nothing before tick 9; a hurt never waits");
    const std::vector<Species::SoundCue> first = schedule.Due({}, 9.0);
    Assert::IsTrue(first.size() == 1 && first[0].sound == Species::GameSound::LaserShot, L"tick 10's as the display reaches 9");
    const std::vector<Species::SoundCue> second = schedule.Due({}, 10.2);
    Assert::IsTrue(second.size() == 1 && second[0].sound == Species::GameSound::Death && schedule.Waiting() == 0, L"then tick 11's");
    static_cast<void>(schedule.Due(events, 0.0));
    schedule.Clear();
    Assert::IsTrue(schedule.Waiting() == 0, L"leaving the world drops what waits");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen209TheListenerIsTheCamera)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-209")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-204")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen209TheListenerIsTheCamera)
  {
    Neuron::CameraPose camera;
    camera.position = {100.0, 800.0, 200.0};
    const Neuron::AudioListener level = Species::ListenerOf(camera);
    Assert::IsTrue(level.position == camera.position, L"at the camera");
    Assert::IsTrue(Near(-1.0, level.front.z) && Near(1.0, level.up.y), L"looking along -Z with +Y up");
    const auto pan = Neuron::StereoPan(camera.position + Neuron::Vec3{50.0, 0.0, 0.0}, level.position, level.front, level.up);
    Assert::IsTrue(pan[1] > 0.99f && pan[0] < 0.01f, L"+X is on its right");

    camera.yawRadians = std::numbers::pi / 6.0;
    camera.pitchRadians = -std::numbers::pi / 3.0;
    const Neuron::AudioListener tilted = Species::ListenerOf(camera);
    Assert::IsTrue(std::abs(Neuron::Dot(tilted.front, tilted.up)) < 1e-12 && Near(1.0, Neuron::Length(tilted.up)),
                   L"its up is a unit vector across its front");
    const Neuron::Vec3 right = Neuron::Normalized(Neuron::Cross(tilted.front, Neuron::Vec3{0.0, 1.0, 0.0}));
    const auto tiltedPan = Neuron::StereoPan(camera.position + right * 50.0, tilted.position, tilted.front, tilted.up);
    Assert::IsTrue(tiltedPan[1] > 0.99f, L"the view's right stays the right as it looks down");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Aen076TheEnginesTuningComesFromThePresentationRecord)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-076")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-146")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"AEN-142")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Aen076TheEnginesTuningComesFromThePresentationRecord)
  {
    const Neuron::AudioSettings settings = Species::AudioSettingsOf(CommittedPresentation(), 200);
    Assert::IsTrue(settings.updatePeriod == Neuron::Nanoseconds{10'000'000}, L"aen.update.period");
    Assert::IsTrue(settings.patience == Neuron::Nanoseconds{150'000'000}, L"three reference audio updates of patience");
    Assert::IsTrue(settings.busCaps == std::array<std::uint32_t, Neuron::AUDIO_BUS_COUNT>{4, 4, 16, 32, 4, 2}, L"the bus caps");
    Assert::IsTrue(Near(-3.0, settings.busLevelsDb[3]) && Near(-10.0, settings.busLevelsDb[4]), L"the bus levels");
    Assert::IsTrue(settings.volumeMax == 255 && settings.masterVolumeStep == 200, L"the master volume's steps");
    const Neuron::XAudio2DeviceDesc device = Species::AudioDeviceDescOf(CommittedPresentation());
    Assert::IsTrue(device.voicesPerPool == 72 && device.limiterRelease == 6 && device.limiterLoudness == 1'000, L"the device's pools");
  }
};

} // namespace GameClientTests
