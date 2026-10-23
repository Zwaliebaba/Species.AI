#pragma once

#include "AudioEngine.h"
#include "ClientSession.h"
#include "EffectEvent.h"
#include "FrameView.h"
#include "SettlementMessages.h"
#include "TuningPresentation.h"
#include "Vec3.h"
#include "XAudio2Device.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace Neuron
{
class LooseContent;
}

namespace Species
{

// MVP-087 (ADR-173): the first sound set's events, in the order of the audio engine's table.
enum class GameSound : std::uint8_t
{
  LaserShot,          // sfx.weapon.laser_bolt.spawn
  Death,              // sfx.unit.worker.die, a Worker's or a Leader's
  OrbBecameWorker,    // sfx.unit.essence_orb.collected
  StructureCompleted, // sfx.building.dwelling.complete, every structure's
  AttackWarning,      // sfx.unit.worker.attack_horn, two-dimensional at the MVP
  BattleStarted,      // sfx.ui.battle.start
  BattleEnded         // sfx.ui.battle.end
};

inline constexpr std::size_t GAME_SOUND_COUNT = 7;

/// A sound to play: its event, and where for a positioned one.
struct SoundCue
{
  GameSound sound = GameSound::LaserShot;
  Neuron::Vec3 position;
};

// The cooked key of each event's sample group without its member's number (ADR-171): the key of its manifest row's file,
// as the content build names it.
[[nodiscard]] std::string_view SoundStemOf(GameSound _sound) noexcept;
// A group's member: the stem itself, or the stem, an underscore and a number (`_01`), and no other key that begins with it.
[[nodiscard]] bool IsMemberKey(std::string_view _key, std::string_view _stem) noexcept;

// AEN-064, ADR-173: the engine's table, one event per GameSound in its order: each group's resident members read from
// _content in key order, its bus, whether it is positioned, its level of mvp.sound.<event>.level_db, the minimum distance
// mvp.sound.min_distance and its cap of real instances (AEN-143). A group with no member that reads is silent (AEN-182).
[[nodiscard]] std::vector<Neuron::SoundEventDef> GameSoundEvents(const Neuron::LooseContent& _content,
                                                                 const TuningPresentation& _presentation);
// The engine's tuning from the presentation record's aen.* and ref.aud.* keys, starting at the master volume step
// _masterVolume (audio.master_volume, AEN-076).
[[nodiscard]] Neuron::AudioSettings AudioSettingsOf(const TuningPresentation& _presentation, unsigned _masterVolume);
// The XAudio2 device's pools and limiter from the same keys (AEN-142, AEN-220).
[[nodiscard]] Neuron::XAudio2DeviceDesc AudioDeviceDescOf(const TuningPresentation& _presentation);
// AEN-209 (DEC-566): the listener is the camera, its front the view's direction and its up the view's, without the
// camera shake's roll.
[[nodiscard]] Neuron::AudioListener ListenerOf(const Neuron::CameraPose& _camera) noexcept;

// The sound an effect event plays, at its position: a laser shot, a death, an orb becoming a Worker or a structure
// completed; none for any other.
[[nodiscard]] std::optional<SoundCue> CueOf(const EffectEvent& _event) noexcept;
// The sound a notice plays, two-dimensional: the warning of an attack by or on the settlement, and its battle starting
// and ending; none for any other.
[[nodiscard]] std::optional<SoundCue> CueOf(const SettlementNotice& _notice) noexcept;

// Each cue triggered on the engine, in order.
void PlayCues(Neuron::AudioEngine& _engine, std::span<const SoundCue> _cues);

/// The world's sounds on the display clock (ADR-173): an effect event of tick t plays its sound as the display time
/// reaches t − 1, with its draw (ADR-084 item 5, ADR-123), in the order the events arrived.
class SoundSchedule
{
public:
  // The cues due at _displayTick, once _events have joined those still waiting.
  [[nodiscard]] std::vector<SoundCue> Due(std::span<const ReceivedEvent> _events, double _displayTick);
  // The world is left: nothing waits.
  void Clear() noexcept
  {
    m_waiting.clear();
  }
  [[nodiscard]] std::size_t Waiting() const noexcept
  {
    return m_waiting.size();
  }

private:
  struct Pending
  {
    std::uint64_t tick = 0;
    SoundCue cue;
  };

  std::deque<Pending> m_waiting;
};

} // namespace Species
