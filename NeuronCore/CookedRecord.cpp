#include "pch.h"

#include "Check.h"
#include "CookedRecord.h"

#include <bit>
#include <format>
#include <utility>

namespace Neuron
{

void CookedWriter::U8(std::uint8_t _value)
{
  Little(_value, 1);
}

void CookedWriter::U16(std::uint16_t _value)
{
  Little(_value, 2);
}

void CookedWriter::U32(std::uint32_t _value)
{
  Little(_value, 4);
}

void CookedWriter::U64(std::uint64_t _value)
{
  Little(_value, 8);
}

void CookedWriter::I16(std::int16_t _value)
{
  Little(static_cast<std::uint16_t>(_value), 2);
}

void CookedWriter::I32(std::int32_t _value)
{
  Little(static_cast<std::uint32_t>(_value), 4);
}

void CookedWriter::I64(std::int64_t _value)
{
  Little(static_cast<std::uint64_t>(_value), 8);
}

void CookedWriter::F32(float _value)
{
  Little(std::bit_cast<std::uint32_t>(_value), 4);
}

void CookedWriter::F64(double _value)
{
  Little(std::bit_cast<std::uint64_t>(_value), 8);
}

void CookedWriter::Append(std::span<const std::byte> _bytes)
{
  m_bytes.insert(m_bytes.end(), _bytes.begin(), _bytes.end());
}

void CookedWriter::Text8(std::string_view _text)
{
  ENGINE_CHECK(_text.size() <= 255, "a cooked text of at most 255 bytes");
  U8(static_cast<std::uint8_t>(_text.size()));
  Append(std::as_bytes(std::span(_text.data(), _text.size())));
}

std::vector<std::byte> CookedWriter::Take() noexcept
{
  return std::exchange(m_bytes, {});
}

void CookedWriter::Little(std::uint64_t _value, std::size_t _bytes)
{
  for (std::size_t i = 0; i < _bytes; ++i)
  {
    m_bytes.push_back(static_cast<std::byte>((_value >> (8 * i)) & 0xFF));
  }
}

CookedReader::CookedReader(std::span<const std::byte> _bytes) noexcept
  : m_bytes(_bytes)
{
}

std::uint8_t CookedReader::U8() noexcept
{
  return static_cast<std::uint8_t>(Little(1));
}

std::uint16_t CookedReader::U16() noexcept
{
  return static_cast<std::uint16_t>(Little(2));
}

std::uint32_t CookedReader::U32() noexcept
{
  return static_cast<std::uint32_t>(Little(4));
}

std::uint64_t CookedReader::U64() noexcept
{
  return Little(8);
}

std::int16_t CookedReader::I16() noexcept
{
  return static_cast<std::int16_t>(static_cast<std::uint16_t>(Little(2)));
}

std::int32_t CookedReader::I32() noexcept
{
  return static_cast<std::int32_t>(static_cast<std::uint32_t>(Little(4)));
}

std::int64_t CookedReader::I64() noexcept
{
  return static_cast<std::int64_t>(Little(8));
}

float CookedReader::F32() noexcept
{
  return std::bit_cast<float>(static_cast<std::uint32_t>(Little(4)));
}

double CookedReader::F64() noexcept
{
  return std::bit_cast<double>(Little(8));
}

std::span<const std::byte> CookedReader::Bytes(std::size_t _count) noexcept
{
  if (m_failed || _count > Remaining())
  {
    m_failed = true;
    return {};
  }
  const std::span<const std::byte> bytes = m_bytes.subspan(m_offset, _count);
  m_offset += _count;
  return bytes;
}

std::string_view CookedReader::Text8() noexcept
{
  const std::size_t length = U8();
  const std::span<const std::byte> bytes = Bytes(length);
  return {reinterpret_cast<const char*>(bytes.data()), bytes.size()};
}

std::uint64_t CookedReader::Little(std::size_t _bytes) noexcept
{
  if (m_failed || _bytes > Remaining())
  {
    m_failed = true;
    return 0;
  }
  std::uint64_t value = 0;
  for (std::size_t i = 0; i < _bytes; ++i)
  {
    value |= static_cast<std::uint64_t>(m_bytes[m_offset + i]) << (8 * i);
  }
  m_offset += _bytes;
  return value;
}

std::vector<std::byte> CookRecordEntry(std::string_view _schemaId, SchemaVersion _version, std::uint32_t _records,
                                       std::span<const std::byte> _payload)
{
  CookedWriter entry;
  entry.Text8(_schemaId);
  entry.U16(_version.major);
  entry.U16(_version.minor);
  entry.U32(static_cast<std::uint32_t>(_payload.size()));
  entry.U32(_records);
  entry.Append(_payload);
  return entry.Take();
}

std::expected<std::span<const std::byte>, std::string> OpenCookedRecord(std::span<const std::byte> _entry, std::string_view _schemaId,
                                                                        SchemaVersion _version, std::uint32_t _records)
{
  CookedReader header(_entry);
  const std::string_view schemaId = header.Text8();
  const SchemaVersion version{header.U16(), header.U16()};
  const std::uint32_t payloadBytes = header.U32();
  const std::uint32_t records = header.U32();
  if (header.Failed())
  {
    return std::unexpected(std::string("the entry is shorter than a cooked record's header"));
  }
  if (schemaId != _schemaId)
  {
    return std::unexpected(std::format("the entry is a {} record, not a {} record", schemaId, _schemaId));
  }
  if (version != _version)
  {
    return std::unexpected(std::format("the entry is version {}.{}; this build reads version {}.{} only (DAT-017)", version.major,
                                       version.minor, _version.major, _version.minor));
  }
  if (payloadBytes != header.Remaining())
  {
    return std::unexpected(std::format("the header states {} payload bytes and the entry holds {}", payloadBytes, header.Remaining()));
  }
  if (records != _records)
  {
    return std::unexpected(std::format("the entry holds {} records where {} are read", records, _records));
  }
  return header.Bytes(payloadBytes);
}

} // namespace Neuron
