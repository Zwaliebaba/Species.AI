#include "pch.h"

#include "WorldSounds.h"

#include "AssetError.h"
#include "ClientSettings.h"
#include "Log.h"
#include "LooseContent.h"
#include "SoundAsset.h"

#include <array>
#include <cmath>
#include <memory>
#include <utility>

namespace Species
{

namespace
{

struct SoundBinding
{
  GameSound sound;
  std::string_view assetId;
  std::string_view stem;
  Neuron::AudioBus bus;
  bool positioned;
};

// MVP-087 (ADR-173, DEC-1009): each event's group, its bus and whether it is positioned. The attack warning and the
// battle's start and end are two-dimensional on the announcer bus, which ducks the world under them (AEN-216).
constexpr std::array<SoundBinding, GAME_SOUND_COUNT> BINDINGS{{
  {GameSound::LaserShot, "sfx.weapon.laser_bolt.spawn", "audio/sfx/laser_bolt/laser_bolt_spawn", Neuron::AudioBus::World, true},
  {GameSound::Death, "sfx.unit.worker.die", "audio/sfx/worker/worker_die", Neuron::AudioBus::World, true},
  {GameSound::OrbBecameWorker, "sfx.unit.essence_orb.collected", "audio/sfx/essence_orb/essence_orb_collected", Neuron::AudioBus::World,
   true},
  {GameSound::StructureCompleted, "sfx.building.dwelling.complete", "audio/sfx/dwelling/dwelling_complete", Neuron::AudioBus::World, true},
  {GameSound::AttackWarning, "sfx.unit.worker.attack_horn", "audio/sfx/worker/worker_attack_horn", Neuron::AudioBus::Announcer, false},
  {GameSound::BattleStarted, "sfx.ui.battle.start", "audio/ui/ui_battle_start", Neuron::AudioBus::Announcer, false},
  {GameSound::BattleEnded, "sfx.ui.battle.end", "audio/ui/ui_battle_end", Neuron::AudioBus::Announcer, false},
}};

constexpr double NANOSECONDS_PER_SECOND = 1e9;

[[nodiscard]] Neuron::Nanoseconds SecondsToNanoseconds(double _seconds) noexcept
{
  return Neuron::Nanoseconds{std::llround(_seconds * NANOSECONDS_PER_SECOND)};
}

[[nodiscard]] double LevelOf(const TuningPresentation& _presentation, GameSound _sound) noexcept
{
  const auto& sound = _presentation.mvp.sound;
  switch (_sound)
  {
  case GameSound::LaserShot:
    return sound.laserShot.levelDb;
  case GameSound::Death:
    return sound.death.levelDb;
  case GameSound::OrbBecameWorker:
    return sound.orbBecomesWorker.levelDb;
  case GameSound::StructureCompleted:
    return sound.structureCompleted.levelDb;
  case GameSound::AttackWarning:
    return sound.attackWarning.levelDb;
  case GameSound::BattleStarted:
    return sound.battleStarted.levelDb;
  case GameSound::BattleEnded:
    return sound.battleEnded.levelDb;
  }
  return 0.0;
}

// AEN-143: the laser shots and the deaths are the battle's many sounds and take the most an event on the world bus may
// author; the world's other events take its default; the announcer's have no cap beyond their bus's.
[[nodiscard]] std::uint32_t VoiceCapOf(const TuningPresentation& _presentation, GameSound _sound, Neuron::AudioBus _bus) noexcept
{
  const auto& voice = _presentation.aen.voice;
  if (_bus != Neuron::AudioBus::World)
  {
    return static_cast<std::uint32_t>(voice.busCap.announcer);
  }
  const bool many = _sound == GameSound::LaserShot || _sound == GameSound::Death;
  return static_cast<std::uint32_t>(many ? voice.eventCap.worldMax : voice.eventCap.worldDefault);
}

} // namespace

std::string_view SoundStemOf(GameSound _sound) noexcept
{
  return BINDINGS[static_cast<std::size_t>(_sound)].stem;
}

bool IsMemberKey(std::string_view _key, std::string_view _stem) noexcept
{
  if (!_key.starts_with(_stem))
  {
    return false;
  }
  const std::string_view rest = _key.substr(_stem.size());
  if (rest.empty())
  {
    return true;
  }
  return rest.size() >= 2 && rest.front() == '_' && rest.substr(1).find_first_not_of("0123456789") == std::string_view::npos;
}

std::vector<Neuron::SoundEventDef> GameSoundEvents(const Neuron::LooseContent& _content, const TuningPresentation& _presentation)
{
  std::vector<Neuron::SoundEventDef> events;
  events.reserve(BINDINGS.size());
  for (const SoundBinding& binding : BINDINGS)
  {
    Neuron::SoundEventDef event;
    event.name = binding.assetId;
    event.bus = binding.bus;
    event.positioned = binding.positioned;
    event.levelDb = LevelOf(_presentation, binding.sound);
    event.minDistance = _presentation.mvp.sound.minDistance;
    event.voiceCap = VoiceCapOf(_presentation, binding.sound, binding.bus);
    // The index lists the entries in key order, so the members are in member order.
    for (const Neuron::ContentEntry& entry : _content.Entries())
    {
      if (entry.type != Neuron::ContentType::ResidentSound || !IsMemberKey(entry.key, binding.stem))
      {
        continue;
      }
      const auto bytes = _content.Read(entry.key);
      if (!bytes)
      {
        Neuron::Log(Neuron::LogCategory::Audio, Neuron::LogLevel::Error, "a sound: {}", Neuron::FormatAssetError(bytes.error()));
        continue;
      }
      auto sound = Neuron::ReadSoundAsset(*bytes);
      if (!sound)
      {
        Neuron::Log(Neuron::LogCategory::Audio, Neuron::LogLevel::Error, "the sound {} cannot be played: {}", entry.key, sound.error());
        continue;
      }
      event.members.push_back(std::make_shared<const Neuron::SoundAsset>(std::move(*sound)));
    }
    if (event.members.empty())
    {
      Neuron::Log(Neuron::LogCategory::Audio, Neuron::LogLevel::Warning, "the sound {} has no member and is silent (AEN-182)", event.name);
    }
    events.push_back(std::move(event));
  }
  return events;
}

Neuron::AudioSettings AudioSettingsOf(const TuningPresentation& _presentation, unsigned _masterVolume)
{
  const auto& aen = _presentation.aen;
  const auto& aud = _presentation.ref.aud;
  Neuron::AudioSettings settings;
  settings.updatePeriod = SecondsToNanoseconds(aen.update.period);
  settings.scheduleLatency = SecondsToNanoseconds(aen.update.scheduleLatency);
  // R-AUD-061, AEN-146: the patience window counts the reference's audio updates from the instance's start.
  settings.patience =
    SecondsToNanoseconds(static_cast<double>(aud.layer.oneShotPatienceUpdates) * _presentation.ref.sim.frame.audioUpdatePeriod);
  settings.commandCapacity = static_cast<std::size_t>(aen.queue.commandCapacity);
  settings.instanceCapacity = static_cast<std::size_t>(aen.instances.capacity);
  settings.voiceBudget = static_cast<std::uint32_t>(aen.voice.budget);
  const auto& caps = aen.voice.busCap;
  settings.busCaps = {static_cast<std::uint32_t>(caps.ui),       static_cast<std::uint32_t>(caps.announcer),
                      static_cast<std::uint32_t>(caps.voice),    static_cast<std::uint32_t>(caps.world),
                      static_cast<std::uint32_t>(caps.ambience), static_cast<std::uint32_t>(caps.music)};
  settings.busLevelsDb = {aud.bus.ui.level,    aud.bus.announcer.level, aud.bus.voice.level,
                          aud.bus.world.level, aud.bus.ambience.level,  aud.bus.music.level};
  settings.stealMarginDb = aen.voice.stealMarginDb;
  settings.stateHold = SecondsToNanoseconds(aen.voice.stateHold);
  settings.stealFade = SecondsToNanoseconds(aen.voice.stealFade);
  settings.maxDistance = aen.spatial.maxDistanceDefault;
  settings.maxDistanceFadeFraction = aen.spatial.maxDistanceFadeFraction;
  settings.floorDb = aud.mix.floorDb;
  settings.masterStepDb = aud.mix.masterStepDb;
  settings.volumeMax = static_cast<unsigned>(MASTER_VOLUME_MAX); // ref.meta.options.sound.volume_max (ADR-150)
  settings.copyDecay = aud.rank.copyDecay;
  settings.duckWorldDb = aud.bus.announcer.duckWorldDb;
  settings.duckTime = SecondsToNanoseconds(aud.bus.announcer.duckTime);
  settings.rngSeed = static_cast<std::uint64_t>(aen.rng.matchSeed);
  settings.masterVolumeStep = _masterVolume;
  return settings;
}

Neuron::XAudio2DeviceDesc AudioDeviceDescOf(const TuningPresentation& _presentation)
{
  const auto& aen = _presentation.aen;
  Neuron::XAudio2DeviceDesc desc;
  desc.voicesPerPool = static_cast<std::uint32_t>(aen.voice.budget + aen.voice.poolHeadroom);
  desc.maxFrequencyRatio = static_cast<float>(aen.voice.maxFrequencyRatio);
  desc.limiterRelease = static_cast<std::uint32_t>(aen.mix.limiter.release);
  desc.limiterLoudness = static_cast<std::uint32_t>(aen.mix.limiter.loudness);
  return desc;
}

Neuron::AudioListener ListenerOf(const Neuron::CameraPose& _camera) noexcept
{
  // The view's up turns with its pitch about its right, as its front does (FrameView: yaw 0 looks along −Z).
  const double sinPitch = std::sin(_camera.pitchRadians);
  Neuron::AudioListener listener;
  listener.position = _camera.position;
  listener.front = Neuron::ViewDirection(_camera);
  listener.up = {std::sin(_camera.yawRadians) * sinPitch, std::cos(_camera.pitchRadians), std::cos(_camera.yawRadians) * sinPitch};
  return listener;
}

std::optional<SoundCue> CueOf(const EffectEvent& _event) noexcept
{
  switch (_event.kind)
  {
  case EffectEventKind::LaserFired:
    return SoundCue{GameSound::LaserShot, _event.position};
  case EffectEventKind::FigureDied:
    return SoundCue{GameSound::Death, _event.position};
  case EffectEventKind::OrbBecameWorker:
    return SoundCue{GameSound::OrbBecameWorker, _event.position};
  case EffectEventKind::StructureCompleted:
    return SoundCue{GameSound::StructureCompleted, _event.position};
  default:
    return std::nullopt;
  }
}

std::optional<SoundCue> CueOf(const SettlementNotice& _notice) noexcept
{
  switch (_notice.kind)
  {
  case NoticeKind::AttackDeclared:
    return SoundCue{GameSound::AttackWarning, {}};
  case NoticeKind::BattleStarted:
    return SoundCue{GameSound::BattleStarted, {}};
  case NoticeKind::BattleEnded:
    return SoundCue{GameSound::BattleEnded, {}};
  default:
    return std::nullopt;
  }
}

void PlayCues(Neuron::AudioEngine& _engine, std::span<const SoundCue> _cues)
{
  for (const SoundCue& cue : _cues)
  {
    static_cast<void>(_engine.Trigger(static_cast<Neuron::SoundEventId>(cue.sound), cue.position));
  }
}

std::vector<SoundCue> SoundSchedule::Due(std::span<const ReceivedEvent> _events, double _displayTick)
{
  for (const ReceivedEvent& received : _events)
  {
    if (const std::optional<SoundCue> cue = CueOf(received.event); cue.has_value())
    {
      m_waiting.push_back({received.tick, *cue});
    }
  }
  std::vector<SoundCue> due;
  while (!m_waiting.empty() && static_cast<double>(m_waiting.front().tick) - 1.0 <= _displayTick)
  {
    due.push_back(m_waiting.front().cue);
    m_waiting.pop_front();
  }
  return due;
}

} // namespace Species
