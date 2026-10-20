#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Neuron
{

// ADR-088 item 5 and ADR-093: on a reliable stream every message is its length, a minimal unsigned LEB128 of at most
// three bytes, followed by its bytes. A length of zero or above the limit is not a message.
inline constexpr std::size_t FRAME_LENGTH_MAX_BYTES = 3;

void AppendFrame(std::vector<std::byte>& _stream, std::span<const std::byte> _message);

/// Cuts a reliable stream back into its messages, however the stream arrives in pieces. A length that breaks the rules
/// cannot be skipped, because nothing after it can be trusted to start a message: the reader stops, and the connection
/// closes with a protocol error (ADR-088 item 5).
class StreamFrameReader
{
public:
  explicit StreamFrameReader(std::size_t _maxMessageBytes) noexcept
    : m_maxMessageBytes(_maxMessageBytes)
  {
  }

  void Feed(std::span<const std::byte> _bytes);
  // The next whole message, if the stream holds one.
  [[nodiscard]] std::optional<std::vector<std::byte>> Next();
  [[nodiscard]] bool Corrupt() const noexcept
  {
    return m_corrupt;
  }
  // The bytes received and not yet cut into a message: at most a message and its length while Next is called.
  [[nodiscard]] std::size_t PendingBytes() const noexcept
  {
    return m_pending.size() - m_offset;
  }

private:
  std::size_t m_maxMessageBytes;
  std::vector<std::byte> m_pending;
  std::size_t m_offset = 0;
  bool m_corrupt = false;
};

} // namespace Neuron
