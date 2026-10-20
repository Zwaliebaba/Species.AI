#include "pch.h"

#include "Check.h"
#include "StreamFraming.h"

namespace Neuron
{

void AppendFrame(std::vector<std::byte>& _stream, std::span<const std::byte> _message)
{
  ENGINE_ASSERT(!_message.empty() && _message.size() < (std::size_t{1} << (7 * FRAME_LENGTH_MAX_BYTES)));
  std::size_t length = _message.size();
  do
  {
    std::byte part{static_cast<unsigned char>(length & 0x7F)};
    length >>= 7;
    if (length != 0)
    {
      part |= std::byte{0x80};
    }
    _stream.push_back(part);
  } while (length != 0);
  _stream.insert(_stream.end(), _message.begin(), _message.end());
}

void StreamFrameReader::Feed(std::span<const std::byte> _bytes)
{
  if (m_corrupt)
  {
    return;
  }
  // What Next has cut off goes before more arrives, so the buffer holds at most one message and the bytes after it.
  m_pending.erase(m_pending.begin(), m_pending.begin() + static_cast<std::ptrdiff_t>(m_offset));
  m_offset = 0;
  m_pending.insert(m_pending.end(), _bytes.begin(), _bytes.end());
}

std::optional<std::vector<std::byte>> StreamFrameReader::Next()
{
  if (m_corrupt)
  {
    return std::nullopt;
  }
  std::size_t length = 0;
  std::size_t lengthBytes = 0;
  while (true)
  {
    if (m_offset + lengthBytes == m_pending.size())
    {
      return std::nullopt; // the length is not whole yet
    }
    const auto part = std::to_integer<std::size_t>(m_pending[m_offset + lengthBytes]);
    length |= (part & 0x7F) << (7 * lengthBytes);
    ++lengthBytes;
    if ((part & 0x80) == 0)
    {
      // Minimal: a last byte of zero after the first would have been a shorter encoding.
      if (lengthBytes > 1 && part == 0)
      {
        m_corrupt = true;
        return std::nullopt;
      }
      break;
    }
    if (lengthBytes == FRAME_LENGTH_MAX_BYTES)
    {
      m_corrupt = true;
      return std::nullopt;
    }
  }
  if (length == 0 || length > m_maxMessageBytes)
  {
    m_corrupt = true;
    return std::nullopt;
  }
  if (m_pending.size() - m_offset - lengthBytes < length)
  {
    return std::nullopt; // the message is not whole yet
  }
  const auto begin = m_pending.begin() + static_cast<std::ptrdiff_t>(m_offset + lengthBytes);
  std::vector<std::byte> message(begin, begin + static_cast<std::ptrdiff_t>(length));
  m_offset += lengthBytes + length;
  if (m_offset == m_pending.size())
  {
    m_pending.clear();
    m_offset = 0;
  }
  return message;
}

} // namespace Neuron
