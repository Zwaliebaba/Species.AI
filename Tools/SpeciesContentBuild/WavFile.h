#pragma once

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <vector>

namespace ContentBuild
{

// The encodings of a WAV source the audio cooker reads (AST-079): integer PCM, or IEEE floating-point PCM.
enum class WavEncoding : std::uint8_t
{
  IntegerPcm,
  FloatPcm
};

/// A WAV file's format and its samples, each scaled to -1 to 1 (an integer sample divided by 2 to the power of its bits
/// less one), interleaved by channel.
struct WavFile
{
  WavEncoding encoding = WavEncoding::IntegerPcm;
  std::uint32_t sampleRate = 0;
  std::uint16_t channels = 0;
  std::uint16_t bitsPerSample = 0;
  std::vector<float> samples;

  [[nodiscard]] std::size_t Frames() const noexcept
  {
    return channels == 0 ? 0 : samples.size() / channels;
  }
};

// A RIFF WAVE file's `fmt ` and `data` chunks, plain or WAVE_FORMAT_EXTENSIBLE with a PCM or IEEE float subformat, at
// 16, 24 or 32 bits of integer PCM or 32 bits of float; or why the bytes are not one. The rate and the channels are
// returned as the file states them, for the cooker to judge (AST-079).
[[nodiscard]] std::expected<WavFile, std::string> ReadWav(std::span<const std::byte> _bytes);

} // namespace ContentBuild
