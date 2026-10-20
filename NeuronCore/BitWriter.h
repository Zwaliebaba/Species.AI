#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace Neuron
{

// The bits a value from 0 to _max needs: 0 for 0, so a field that can hold only one value costs nothing.
[[nodiscard]] constexpr std::uint32_t BitsFor(std::uint64_t _max) noexcept
{
  return static_cast<std::uint32_t>(std::bit_width(_max));
}

/// The writing half of the bit-packed message layout (ADR-088 item 5, ADR-093). Fields follow each other with no name,
/// no tag and no alignment, least significant bit first; a field of n bits costs n bits. Byte strings start on a byte
/// boundary, so a reader can hand them out without copying. The codecs generated from the message schemas (ADR-062)
/// write through it; a value that does not fit its field is a defect of the writer, caught by a development check.
class BitWriter
{
public:
  // _bits from 0 to 64; the value must fit in them.
  void WriteBits(std::uint64_t _value, std::uint32_t _bits);
  void WriteBool(bool _value);
  // A count, a length or an enumeration: at most _max, in BitsFor(_max) bits.
  void WriteBounded(std::uint64_t _value, std::uint64_t _max);
  // Two's complement in _bits bits (1 to 64); the value must fit.
  void WriteSigned(std::int64_t _value, std::uint32_t _bits);
  // A bounded length, zero bits to the next byte, then the bytes.
  void WriteBytes(std::span<const std::byte> _bytes, std::size_t _maxBytes);
  void WriteString(std::string_view _utf8, std::size_t _maxBytes);

  [[nodiscard]] std::size_t BitCount() const noexcept
  {
    return m_bitCount;
  }
  // The message so far, its last byte padded with zero bits.
  [[nodiscard]] std::span<const std::byte> Bytes() const noexcept
  {
    return m_bytes;
  }
  [[nodiscard]] std::vector<std::byte> Take() noexcept;

private:
  void AlignToByte();

  std::vector<std::byte> m_bytes;
  std::size_t m_bitCount = 0;
};

} // namespace Neuron
