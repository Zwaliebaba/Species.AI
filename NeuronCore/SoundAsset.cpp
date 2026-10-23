#include "pch.h"

#include "CookedRecord.h"
#include "SoundAsset.h"

#include <format>

namespace Neuron
{

std::vector<std::byte> WriteSoundAsset(const SoundAsset& _sound)
{
  CookedWriter out;
  out.U32(SOUND_ASSET_VERSION);
  out.U32(_sound.sampleRate);
  out.U16(_sound.channels);
  out.U32(static_cast<std::uint32_t>(_sound.Frames()));
  for (const std::int16_t sample : _sound.samples)
  {
    out.I16(sample);
  }
  return out.Take();
}

std::expected<SoundAsset, std::string> ReadSoundAsset(std::span<const std::byte> _entry)
{
  CookedReader in(_entry);
  if (const std::uint32_t version = in.U32(); version != SOUND_ASSET_VERSION)
  {
    return std::unexpected(std::format("the cooked sound is version {}; this build reads version {} only", version, SOUND_ASSET_VERSION));
  }
  SoundAsset sound;
  sound.sampleRate = in.U32();
  sound.channels = in.U16();
  const std::uint32_t frames = in.U32();
  if (in.Failed())
  {
    return std::unexpected(std::string("the cooked sound is shorter than its header"));
  }
  if (sound.sampleRate != SOUND_SAMPLE_RATE || sound.channels == 0 || sound.channels > SOUND_CHANNELS_MAX)
  {
    return std::unexpected(std::format("the cooked sound is {} Hz with {} channels; the engine plays {} Hz with 1 or 2 (AEN-072)",
                                       sound.sampleRate, sound.channels, SOUND_SAMPLE_RATE));
  }
  if (frames == 0)
  {
    return std::unexpected(std::string("the cooked sound has no sample frames (AEN-179)"));
  }
  const std::size_t samples = static_cast<std::size_t>(frames) * sound.channels;
  if (in.Remaining() != samples * sizeof(std::int16_t))
  {
    return std::unexpected(std::format("the cooked sound holds {} bytes of samples for {} frames", in.Remaining(), frames));
  }
  sound.samples.resize(samples);
  for (std::int16_t& sample : sound.samples)
  {
    sample = in.I16();
  }
  return sound;
}

} // namespace Neuron
