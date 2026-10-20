#pragma once

#include "ContentDiagnostic.h"

#include <cstddef>
#include <cstdint>
#include <expected>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

/// The bytes of a cooked entry as the content build's cookers write them: every multi-byte value little-endian, a float
/// as its IEEE 754 bits, and no pointer or host-dependent value, so the same bytes read the same on every platform
/// (DAT-014, ADR-058).
class CookedWriter
{
public:
  void U8(std::uint8_t _value);
  void U16(std::uint16_t _value);
  void U32(std::uint32_t _value);
  void U64(std::uint64_t _value);
  void I16(std::int16_t _value);
  void I32(std::int32_t _value);
  void I64(std::int64_t _value);
  void F32(float _value);
  void F64(double _value);
  void Append(std::span<const std::byte> _bytes);
  // A text of at most 255 bytes, after its length in one byte, with no terminator.
  void Text8(std::string_view _text);

  [[nodiscard]] std::span<const std::byte> View() const noexcept
  {
    return m_bytes;
  }
  [[nodiscard]] std::vector<std::byte> Take() noexcept;

private:
  void Little(std::uint64_t _value, std::size_t _bytes);

  std::vector<std::byte> m_bytes;
};

/// Reads the bytes a CookedWriter wrote. A read past the end reads 0 and fails the reader, so a loader checks once,
/// when it has read everything, that it read exactly the entry (DAT-050).
class CookedReader
{
public:
  explicit CookedReader(std::span<const std::byte> _bytes) noexcept;

  [[nodiscard]] std::uint8_t U8() noexcept;
  [[nodiscard]] std::uint16_t U16() noexcept;
  [[nodiscard]] std::uint32_t U32() noexcept;
  [[nodiscard]] std::uint64_t U64() noexcept;
  [[nodiscard]] std::int16_t I16() noexcept;
  [[nodiscard]] std::int32_t I32() noexcept;
  [[nodiscard]] std::int64_t I64() noexcept;
  [[nodiscard]] float F32() noexcept;
  [[nodiscard]] double F64() noexcept;
  // The next bytes, as a view onto the entry; empty, and the reader failed, when fewer remain.
  [[nodiscard]] std::span<const std::byte> Bytes(std::size_t _count) noexcept;
  [[nodiscard]] std::string_view Text8() noexcept;

  [[nodiscard]] bool Failed() const noexcept
  {
    return m_failed;
  }
  [[nodiscard]] std::size_t Remaining() const noexcept
  {
    return m_bytes.size() - m_offset;
  }
  // Every byte was read, and no read went past the end.
  [[nodiscard]] bool Finished() const noexcept
  {
    return !m_failed && m_offset == m_bytes.size();
  }

private:
  [[nodiscard]] std::uint64_t Little(std::size_t _bytes) noexcept;

  std::span<const std::byte> m_bytes;
  std::size_t m_offset = 0;
  bool m_failed = false;
};

// DAT-014: a cooked record entry, its header and then its payload. The header holds the schema id after its length in
// one byte, the schema version as two 16-bit values (major, minor), the payload's length and the number of records in
// it, each 32 bits.
[[nodiscard]] std::vector<std::byte> CookRecordEntry(std::string_view _schemaId, SchemaVersion _version, std::uint32_t _records,
                                                     std::span<const std::byte> _payload);

// The payload of a cooked record entry whose header names this schema at this version and holds this many records, as a
// view onto the entry's bytes (DAT-050); why not, otherwise. A loader accepts exactly the version it was generated
// with (DAT-017).
[[nodiscard]] std::expected<std::span<const std::byte>, std::string>
OpenCookedRecord(std::span<const std::byte> _entry, std::string_view _schemaId, SchemaVersion _version, std::uint32_t _records);

} // namespace Neuron
