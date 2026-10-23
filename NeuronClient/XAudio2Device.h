#pragma once

#include "AudioDevice.h"

#include <cstdint>
#include <memory>

namespace Neuron
{

/// What the XAudio2 device is built with (AEN-142, AEN-220): the voices of each pool (one of mono and one of stereo
/// 16-bit PCM at 48 kHz) and the limiter's parameters.
struct XAudio2DeviceDesc
{
  std::uint32_t voicesPerPool = 72;      // aen.voice.budget + aen.voice.pool_headroom
  float maxFrequencyRatio = 3.0f;        // aen.voice.max_frequency_ratio
  std::uint32_t limiterRelease = 6;      // aen.mix.limiter.release
  std::uint32_t limiterLoudness = 1'000; // aen.mix.limiter.loudness
};

// ADR-022: the in-box XAudio2 2.9 on the default output device, its graph and pools built, or none, logged, when it
// cannot be made (no output device, as on a CI runner), and the engine then plays through the silent device (AEN-100).
// This file and its source are the only ones that include an XAudio2 header (AEN-025, AEN-045).
[[nodiscard]] std::unique_ptr<AudioDevice> CreateXAudio2Device(const XAudio2DeviceDesc& _desc);

} // namespace Neuron
