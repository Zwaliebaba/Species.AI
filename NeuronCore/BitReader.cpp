#include "pch.h"

#include "BitReader.h"
#include "BitWriter.h"

#include <algorithm>

namespace Neuron
{

std::uint64_t BitReader::ReadBits(std::uint32_t _bits) noexcept
{
  if (m_failed || _bits > 64 || _bits > RemainingBits())
  {
    m_failed = true;
    return 0;
  }
  std::uint64_t value = 0;
  for (std::uint32_t read = 0; read < _bits;)
  {
    const std::size_t bitInByte = m_bitPosition % 8;
    const std::uint32_t chunk = std::min<std::uint32_t>(_bits - read, static_cast<std::uint32_t>(8 - bitInByte));
    const auto byte = std::to_integer<std::uint64_t>(m_bytes[m_bitPosition / 8]);
    value |= ((byte >> bitInByte) & ((std::uint64_t{1} << chunk) - 1)) << read;
    read += chunk;
    m_bitPosition += chunk;
  }
  return value;
}

bool BitReader::ReadBool() noexcept
{
  return ReadBits(1) != 0;
}

std::uint64_t BitReader::ReadBounded(std::uint64_t _max) noexcept
{
  const std::uint64_t value = ReadBits(BitsFor(_max));
  if (value > _max)
  {
    m_failed = true;
    return 0;
  }
  return value;
}

std::int64_t BitReader::ReadSigned(std::uint32_t _bits) noexcept
{
  if (_bits == 0)
  {
    m_failed = true;
    return 0;
  }
  const std::uint64_t value = ReadBits(_bits);
  if (_bits < 64 && (value >> (_bits - 1)) != 0)
  {
    return static_cast<std::int64_t>(value | (~std::uint64_t{0} << _bits)); // sign extension
  }
  return static_cast<std::int64_t>(value);
}

std::span<const std::byte> BitReader::ReadBytes(std::size_t _maxBytes) noexcept
{
  const std::uint64_t length = ReadBounded(_maxBytes);
  if (!SkipPadding() || length > RemainingBits() / 8)
  {
    m_failed = true;
    return {};
  }
  const std::span<const std::byte> bytes = m_bytes.subspan(m_bitPosition / 8, static_cast<std::size_t>(length));
  m_bitPosition += bytes.size() * 8;
  return bytes;
}

std::string_view BitReader::ReadString(std::size_t _maxBytes) noexcept
{
  const std::span<const std::byte> bytes = ReadBytes(_maxBytes);
  const std::string_view text(reinterpret_cast<const char*>(bytes.data()), bytes.size());
  if (!IsWellFormedUtf8(text))
  {
    m_failed = true;
    return {};
  }
  return text;
}

bool BitReader::Enter(std::uint32_t _maxDepth) noexcept
{
  if (m_failed || m_depth >= _maxDepth)
  {
    m_failed = true;
    return false;
  }
  ++m_depth;
  return true;
}

void BitReader::Leave() noexcept
{
  if (m_depth == 0)
  {
    m_failed = true;
    return;
  }
  --m_depth;
}

bool BitReader::Finish() noexcept
{
  if (m_failed || m_depth != 0 || !SkipPadding() || RemainingBits() != 0)
  {
    m_failed = true;
  }
  return !m_failed;
}

// The zero bits that pad the current byte; a one among them means the message is not the one its writer wrote.
bool BitReader::SkipPadding() noexcept
{
  if (m_failed)
  {
    return false;
  }
  const std::size_t bitInByte = m_bitPosition % 8;
  if (bitInByte == 0)
  {
    return true;
  }
  const auto byte = std::to_integer<std::uint32_t>(m_bytes[m_bitPosition / 8]);
  if ((byte >> bitInByte) != 0)
  {
    m_failed = true;
    return false;
  }
  m_bitPosition += 8 - bitInByte;
  return true;
}

bool IsWellFormedUtf8(std::string_view _text) noexcept
{
  std::size_t index = 0;
  while (index < _text.size())
  {
    const auto lead = static_cast<unsigned char>(_text[index]);
    std::size_t length = 0;
    std::uint32_t point = 0;
    std::uint32_t minimum = 0;
    if (lead < 0x80)
    {
      ++index;
      continue;
    }
    if ((lead & 0xE0) == 0xC0)
    {
      length = 2;
      point = lead & 0x1Fu;
      minimum = 0x80;
    }
    else if ((lead & 0xF0) == 0xE0)
    {
      length = 3;
      point = lead & 0x0Fu;
      minimum = 0x800;
    }
    else if ((lead & 0xF8) == 0xF0)
    {
      length = 4;
      point = lead & 0x07u;
      minimum = 0x10000;
    }
    else
    {
      return false;
    }
    if (index + length > _text.size())
    {
      return false;
    }
    for (std::size_t next = 1; next < length; ++next)
    {
      const auto continuation = static_cast<unsigned char>(_text[index + next]);
      if ((continuation & 0xC0) != 0x80)
      {
        return false;
      }
      point = (point << 6) | (continuation & 0x3Fu);
    }
    if (point < minimum || point > 0x10FFFF || (point >= 0xD800 && point <= 0xDFFF))
    {
      return false;
    }
    index += length;
  }
  return true;
}

} // namespace Neuron
