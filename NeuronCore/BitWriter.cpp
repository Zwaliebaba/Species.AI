#include "pch.h"

#include "BitWriter.h"
#include "Check.h"

#include <algorithm>
#include <utility>

namespace Neuron
{

void BitWriter::WriteBits(std::uint64_t _value, std::uint32_t _bits)
{
  ENGINE_ASSERT(_bits <= 64);
  ENGINE_ASSERT(_bits == 64 || (_value >> _bits) == 0);
  for (std::uint32_t written = 0; written < _bits;)
  {
    const std::size_t bitInByte = m_bitCount % 8;
    if (bitInByte == 0)
    {
      m_bytes.push_back(std::byte{0});
    }
    const std::uint32_t chunk = std::min<std::uint32_t>(_bits - written, static_cast<std::uint32_t>(8 - bitInByte));
    const std::uint64_t bits = (_value >> written) & ((std::uint64_t{1} << chunk) - 1);
    m_bytes.back() |= static_cast<std::byte>(bits << bitInByte);
    written += chunk;
    m_bitCount += chunk;
  }
}

void BitWriter::WriteBool(bool _value)
{
  WriteBits(_value ? 1 : 0, 1);
}

void BitWriter::WriteBounded(std::uint64_t _value, std::uint64_t _max)
{
  ENGINE_ASSERT(_value <= _max);
  WriteBits(_value, BitsFor(_max));
}

void BitWriter::WriteSigned(std::int64_t _value, std::uint32_t _bits)
{
  ENGINE_ASSERT(_bits >= 1 && _bits <= 64);
  ENGINE_ASSERT(_bits == 64 || (_value >= -(std::int64_t{1} << (_bits - 1)) && _value < (std::int64_t{1} << (_bits - 1))));
  const std::uint64_t mask = _bits == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << _bits) - 1;
  WriteBits(static_cast<std::uint64_t>(_value) & mask, _bits);
}

void BitWriter::WriteBytes(std::span<const std::byte> _bytes, std::size_t _maxBytes)
{
  WriteBounded(_bytes.size(), _maxBytes);
  AlignToByte();
  m_bytes.insert(m_bytes.end(), _bytes.begin(), _bytes.end());
  m_bitCount += _bytes.size() * 8;
}

void BitWriter::WriteString(std::string_view _utf8, std::size_t _maxBytes)
{
  WriteBytes(std::as_bytes(std::span(_utf8)), _maxBytes);
}

std::vector<std::byte> BitWriter::Take() noexcept
{
  m_bitCount = 0;
  return std::exchange(m_bytes, {});
}

void BitWriter::AlignToByte()
{
  m_bitCount = m_bytes.size() * 8;
}

} // namespace Neuron
