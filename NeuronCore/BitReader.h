#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace Neuron
{

/// The reading half of the bit-packed message layout (ADR-088 item 5, ADR-093), for input that may be hostile. Every
/// read is bounded by the buffer and by the limit its field states; the first read that breaks either fails the reader,
/// and every later read returns zero, false or an empty view, so a codec decodes to the end and asks Finish once.
/// Nothing a message holds makes the reader allocate, recurse without a limit or read outside its buffer.
class BitReader
{
public:
  explicit BitReader(std::span<const std::byte> _bytes) noexcept
    : m_bytes(_bytes)
  {
  }

  [[nodiscard]] std::uint64_t ReadBits(std::uint32_t _bits) noexcept;
  [[nodiscard]] bool ReadBool() noexcept;
  // A value from 0 to _max in BitsFor(_max) bits; a larger one fails the reader.
  [[nodiscard]] std::uint64_t ReadBounded(std::uint64_t _max) noexcept;
  [[nodiscard]] std::int64_t ReadSigned(std::uint32_t _bits) noexcept;
  // A view into the buffer, valid as long as the buffer is; a length above _maxBytes or nonzero padding fails.
  [[nodiscard]] std::span<const std::byte> ReadBytes(std::size_t _maxBytes) noexcept;
  // As ReadBytes, and the bytes must be well-formed UTF-8.
  [[nodiscard]] std::string_view ReadString(std::size_t _maxBytes) noexcept;

  // A nested value: at most _maxDepth levels open at once (ADR-088 item 5); each successful Enter has its Leave.
  [[nodiscard]] bool Enter(std::uint32_t _maxDepth) noexcept;
  void Leave() noexcept;

  // A codec's own bound on a field it has read, such as a finite real or a nonzero id (ADR-087 item 2): fails the
  // reader as a read beyond a stated limit does.
  void Fail() noexcept
  {
    m_failed = true;
  }

  // The end of a message: no read failed, nothing but zero padding is left, and every nested value is closed.
  [[nodiscard]] bool Finish() noexcept;
  [[nodiscard]] bool Failed() const noexcept
  {
    return m_failed;
  }
  [[nodiscard]] std::size_t RemainingBits() const noexcept
  {
    return m_bytes.size() * 8 - m_bitPosition;
  }

private:
  bool SkipPadding() noexcept;

  std::span<const std::byte> m_bytes;
  std::size_t m_bitPosition = 0;
  std::uint32_t m_depth = 0;
  bool m_failed = false;
};

// RFC 3629: no overlong form, no surrogate and nothing above U+10FFFF.
[[nodiscard]] bool IsWellFormedUtf8(std::string_view _text) noexcept;

} // namespace Neuron
