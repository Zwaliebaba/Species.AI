#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace Neuron
{

// The cooked resident sound's layout version (ADR-171).
inline constexpr std::uint32_t SOUND_ASSET_VERSION = 1;
// aen.format.sample_rate: every member is cooked at this rate (AEN-072, ADR-025).
inline constexpr std::uint32_t SOUND_SAMPLE_RATE = 48'000;
// A positioned sound is mono and a two-dimensional one mono or stereo (AEN-072).
inline constexpr std::uint16_t SOUND_CHANNELS_MAX = 2;

/// A member of a resident sample group as the content build cooks it (`ContentType::ResidentSound`, AST-080, ADR-025,
/// ADR-171): 16-bit PCM at SOUND_SAMPLE_RATE, its channels interleaved. The engine plays it from memory as it is, so
/// it is never decoded (AEN-243).
struct SoundAsset
{
  std::uint32_t sampleRate = SOUND_SAMPLE_RATE;
  std::uint16_t channels = 1;
  std::vector<std::int16_t> samples;

  [[nodiscard]] std::size_t Frames() const noexcept
  {
    return channels == 0 ? 0 : samples.size() / channels;
  }
};

// The entry's bytes: the version, the rate, the channels, the frame count, then the samples, all little-endian.
[[nodiscard]] std::vector<std::byte> WriteSoundAsset(const SoundAsset& _sound);
// The sound of an entry, or why its bytes are not one: another version, a rate or channel count the engine does not
// play, no frames (AEN-179), or a length that disagrees with its frame count.
[[nodiscard]] std::expected<SoundAsset, std::string> ReadSoundAsset(std::span<const std::byte> _entry);

} // namespace Neuron
