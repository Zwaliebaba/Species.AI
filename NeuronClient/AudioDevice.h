#pragma once

#include "SoundAsset.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace Neuron
{

// AEN §2.2 (ADR-024, DEC-570): the six player buses, in the order of their priority classes, highest first; music is
// outside the ranking.
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

// A pooled source voice of a device.
using AudioVoice = std::uint32_t;

/// The voices and the bus graph the audio engine plays through (ADR-022 layer 2, AEN-220): the mastering voice, the
/// pre-master bus with its limiter, a submix per player bus, and pools of source voices made when the device is built
/// (AEN-142), none made or destroyed in a session. The engine's update thread alone calls it (AEN-301); a voice's end
/// and a critical error arrive from the device's own callbacks as flags (AEN-025). Every change of an update takes
/// effect at once, at Commit (ADR-026).
class AudioDevice
{
public:
  virtual ~AudioDevice() = default;

  AudioDevice() = default;
  AudioDevice(const AudioDevice&) = delete;
  AudioDevice& operator=(const AudioDevice&) = delete;

  // The output's channels; 0 for a device that plays nothing (AEN-321).
  [[nodiscard]] virtual std::uint32_t OutputChannels() const noexcept = 0;
  // A free voice of _sound's channel count plays it from _startFrame on _bus at _amplitude, panned to _pan (left and
  // right); none when the pool of its format has no free voice.
  [[nodiscard]] virtual std::optional<AudioVoice> Start(const SoundAsset& _sound, std::size_t _startFrame, AudioBus _bus, float _amplitude,
                                                        const std::array<float, 2>& _pan) = 0;
  // A playing voice's amplitude and pan.
  virtual void Set(AudioVoice _voice, float _amplitude, const std::array<float, 2>& _pan) = 0;
  // A voice stops and returns to its pool.
  virtual void Stop(AudioVoice _voice) = 0;
  // A voice's sound has played to its end since it started.
  [[nodiscard]] virtual bool Ended(AudioVoice _voice) noexcept = 0;
  // A player bus's gain and the pre-master's (the master volume), as amplitudes.
  virtual void SetBusGain(AudioBus _bus, float _amplitude) = 0;
  virtual void SetPreMasterGain(float _amplitude) = 0;
  // The update's changes take effect together (one operation set).
  virtual void Commit() = 0;
  // The device was lost or failed since it was built (AEN-259); the engine then plays every instance virtual.
  [[nodiscard]] virtual bool Lost() const noexcept = 0;
  // AEN-259, AEN-260: the graph and the pools made again on the current default device, every voice free; false while
  // there is still none, and the engine tries again later.
  [[nodiscard]] virtual bool Rebuild() = 0;
};

} // namespace Neuron
