#include "pch.h"

#include "MessageCodec.h"

namespace Neuron
{

void WriteMessageHeader(BitWriter& _writer, MessageHeader _header)
{
  _writer.WriteBits(_header.type, MESSAGE_TYPE_BITS);
  _writer.WriteBits(_header.version, MESSAGE_VERSION_BITS);
}

MessageHeader ReadMessageHeader(BitReader& _reader) noexcept
{
  MessageHeader header;
  header.type = static_cast<MessageTypeId>(_reader.ReadBits(MESSAGE_TYPE_BITS));
  header.version = static_cast<std::uint8_t>(_reader.ReadBits(MESSAGE_VERSION_BITS));
  return header;
}

bool MessageDispatcher::Dispatch(std::span<const std::byte> _message)
{
  if (_message.size() > MESSAGE_MAX_BYTES)
  {
    ++m_drops.oversized;
    return false;
  }
  BitReader reader(_message);
  const MessageHeader header = ReadMessageHeader(reader);
  if (reader.Failed())
  {
    ++m_drops.malformed;
    return false;
  }
  const auto found = m_handlers.find(header.type);
  if (found == m_handlers.end())
  {
    ++m_drops.unknown;
    return false;
  }
  const Entry& entry = found->second;
  if (header.version != entry.version)
  {
    ++m_drops.wrongVersion;
    return false;
  }
  if (_message.size() > entry.maxBytes)
  {
    ++m_drops.oversized;
    return false;
  }
  if (!entry.decode(reader))
  {
    ++m_drops.malformed;
    return false;
  }
  return true;
}

} // namespace Neuron
