#include "pch.h"

#include "WavFile.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstring>
#include <format>
#include <optional>
#include <string_view>

namespace ContentBuild
{

namespace
{

constexpr std::uint16_t FORMAT_PCM = 1;
constexpr std::uint16_t FORMAT_FLOAT = 3;
constexpr std::uint16_t FORMAT_EXTENSIBLE = 0xFFFE;
constexpr std::size_t FORMAT_BYTES = 16;      // the plain `fmt ` chunk
constexpr std::size_t EXTENSIBLE_BYTES = 40;  // with cbSize, the valid bits, the channel mask and the subformat
constexpr std::size_t SUBFORMAT_OFFSET = 24;  // the subformat GUID, whose first two bytes are its format tag
constexpr std::size_t CHUNK_HEADER_BYTES = 8; // the chunk's id and its size
constexpr std::size_t RIFF_HEADER_BYTES = 12; // "RIFF", its size and "WAVE"
constexpr std::uint32_t BITS_PER_BYTE = 8;

[[nodiscard]] std::uint16_t Read16(std::span<const std::byte> _bytes, std::size_t _at) noexcept
{
  return static_cast<std::uint16_t>(std::to_integer<std::uint32_t>(_bytes[_at]) | (std::to_integer<std::uint32_t>(_bytes[_at + 1]) << 8U));
}

[[nodiscard]] std::uint32_t Read32(std::span<const std::byte> _bytes, std::size_t _at) noexcept
{
  return std::to_integer<std::uint32_t>(_bytes[_at]) | (std::to_integer<std::uint32_t>(_bytes[_at + 1]) << 8U) |
         (std::to_integer<std::uint32_t>(_bytes[_at + 2]) << 16U) | (std::to_integer<std::uint32_t>(_bytes[_at + 3]) << 24U);
}

[[nodiscard]] bool IdIs(std::span<const std::byte> _bytes, std::size_t _at, std::string_view _id) noexcept
{
  for (std::size_t index = 0; index < _id.size(); ++index)
  {
    if (std::to_integer<char>(_bytes[_at + index]) != _id[index])
    {
      return false;
    }
  }
  return true;
}

// A sample of _bits bits at _at, scaled to -1 to 1.
[[nodiscard]] float SampleAt(std::span<const std::byte> _bytes, std::size_t _at, WavEncoding _encoding, std::uint16_t _bits) noexcept
{
  if (_encoding == WavEncoding::FloatPcm)
  {
    return std::bit_cast<float>(Read32(_bytes, _at));
  }
  std::uint32_t raw = 0;
  for (std::uint32_t index = 0; index < _bits / BITS_PER_BYTE; ++index)
  {
    raw |= std::to_integer<std::uint32_t>(_bytes[_at + index]) << (index * BITS_PER_BYTE);
  }
  // Sign-extend from the sample's top bit.
  const std::uint32_t shift = 32U - _bits;
  const auto value = static_cast<std::int32_t>(raw << shift) >> shift;
  return static_cast<float>(static_cast<double>(value) / std::ldexp(1.0, _bits - 1));
}

} // namespace

std::expected<WavFile, std::string> ReadWav(std::span<const std::byte> _bytes)
{
  if (_bytes.size() < RIFF_HEADER_BYTES || !IdIs(_bytes, 0, "RIFF") || !IdIs(_bytes, 8, "WAVE"))
  {
    return std::unexpected(std::string("it is not a RIFF WAVE file"));
  }
  std::optional<std::size_t> format;
  std::size_t formatBytes = 0;
  std::optional<std::size_t> data;
  std::size_t dataBytes = 0;
  std::size_t at = RIFF_HEADER_BYTES;
  while (at + CHUNK_HEADER_BYTES <= _bytes.size())
  {
    const std::size_t size = Read32(_bytes, at + 4);
    const std::size_t body = at + CHUNK_HEADER_BYTES;
    if (size > _bytes.size() - body)
    {
      return std::unexpected(std::format("its chunk at byte {} runs past the end of the file", at));
    }
    if (IdIs(_bytes, at, "fmt "))
    {
      format = body;
      formatBytes = size;
    }
    else if (IdIs(_bytes, at, "data"))
    {
      data = body;
      dataBytes = size;
    }
    at = body + size + (size & 1U); // a chunk of odd size is padded to an even one
  }
  if (!format || formatBytes < FORMAT_BYTES)
  {
    return std::unexpected(std::string("it has no format chunk"));
  }
  if (!data)
  {
    return std::unexpected(std::string("it has no data chunk"));
  }
  std::uint16_t tag = Read16(_bytes, *format);
  if (tag == FORMAT_EXTENSIBLE)
  {
    if (formatBytes < EXTENSIBLE_BYTES)
    {
      return std::unexpected(std::string("its extensible format chunk is short"));
    }
    tag = Read16(_bytes, *format + SUBFORMAT_OFFSET);
  }
  WavFile file;
  file.channels = Read16(_bytes, *format + 2);
  file.sampleRate = Read32(_bytes, *format + 4);
  const std::uint16_t blockAlign = Read16(_bytes, *format + 12);
  file.bitsPerSample = Read16(_bytes, *format + 14);
  if (tag == FORMAT_PCM && (file.bitsPerSample == 16 || file.bitsPerSample == 24 || file.bitsPerSample == 32))
  {
    file.encoding = WavEncoding::IntegerPcm;
  }
  else if (tag == FORMAT_FLOAT && file.bitsPerSample == 32)
  {
    file.encoding = WavEncoding::FloatPcm;
  }
  else
  {
    return std::unexpected(std::format("its format is tag {} at {} bits, not integer or float PCM (AST-079)", tag, file.bitsPerSample));
  }
  const std::uint32_t sampleBytes = file.bitsPerSample / BITS_PER_BYTE;
  if (file.channels == 0 || blockAlign != file.channels * sampleBytes)
  {
    return std::unexpected(
      std::format("its block alignment {} does not hold {} channels of {} bits", blockAlign, file.channels, file.bitsPerSample));
  }
  const std::size_t frames = dataBytes / blockAlign;
  file.samples.reserve(frames * file.channels);
  for (std::size_t sample = 0; sample < frames * file.channels; ++sample)
  {
    file.samples.push_back(SampleAt(_bytes, *data + sample * sampleBytes, file.encoding, file.bitsPerSample));
  }
  return file;
}

} // namespace ContentBuild
