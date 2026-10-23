#pragma once

#include "AudioDevice.h"

namespace Neuron
{

/// The device of a machine with no audio output, or of an engine whose device could not be built (AEN-100 step 2,
/// AEN-321): it has no voice, so every instance stays virtual and the engine runs its whole update, as on a CI runner.
class SilentAudioDevice final : public AudioDevice
{
public:
  [[nodiscard]] std::uint32_t OutputChannels() const noexcept override
  {
    return 0;
  }
  [[nodiscard]] std::optional<AudioVoice> Start(const SoundAsset& /*_sound*/, std::size_t /*_startFrame*/, AudioBus /*_bus*/,
                                                float /*_amplitude*/, const std::array<float, 2>& /*_pan*/) override
  {
    return std::nullopt;
  }
  void Set(AudioVoice /*_voice*/, float /*_amplitude*/, const std::array<float, 2>& /*_pan*/) override {}
  void Stop(AudioVoice /*_voice*/) override {}
  [[nodiscard]] bool Ended(AudioVoice /*_voice*/) noexcept override
  {
    return false;
  }
  void SetBusGain(AudioBus /*_bus*/, float /*_amplitude*/) override {}
  void SetPreMasterGain(float /*_amplitude*/) override {}
  void Commit() override {}
  [[nodiscard]] bool Lost() const noexcept override
  {
    return false;
  }
  [[nodiscard]] bool Rebuild() override
  {
    return true;
  }
};

} // namespace Neuron
