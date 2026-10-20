#pragma once

#include "BitReader.h"
#include "BitWriter.h"

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <span>
#include <utility>
#include <vector>

namespace Neuron
{

// ADR-088 item 5 and ADR-093: a message is a type id and a schema version, then the type's own fields.
using MessageTypeId = std::uint16_t;
inline constexpr std::uint32_t MESSAGE_TYPE_BITS = 16;
inline constexpr std::uint32_t MESSAGE_VERSION_BITS = 8;
inline constexpr std::size_t MESSAGE_MAX_BYTES = std::size_t{64} * 1024; // the largest reliable message; blobs travel on blob streams

struct MessageHeader
{
  MessageTypeId type = 0;
  std::uint8_t version = 0;
};

void WriteMessageHeader(BitWriter& _writer, MessageHeader _header);
[[nodiscard]] MessageHeader ReadMessageHeader(BitReader& _reader) noexcept;

// What a message type gives the codec skeleton; the codecs generated from the schemas (ADR-062) fill it in.
template <class T>
concept MessageCodec = requires(const T& _message, T& _decoded, BitWriter& _writer, BitReader& _reader) {
  { T::TYPE } -> std::convertible_to<MessageTypeId>;
  { T::VERSION } -> std::convertible_to<std::uint8_t>;
  { T::MAX_BYTES } -> std::convertible_to<std::size_t>;
  _message.Encode(_writer);
  { _decoded.Decode(_reader) } -> std::same_as<void>;
};

// One message, header and fields, as the transport carries it.
template <MessageCodec T> [[nodiscard]] std::vector<std::byte> EncodeMessage(const T& _message)
{
  BitWriter writer;
  WriteMessageHeader(writer, {T::TYPE, T::VERSION});
  _message.Encode(writer);
  return writer.Take();
}

// Why a message was dropped (ADR-088 item 5): never fatal, always counted.
struct MessageDropCounts
{
  std::uint64_t unknown = 0;      // no handler for its type
  std::uint64_t wrongVersion = 0; // a version its handler does not read
  std::uint64_t oversized = 0;    // larger than its type allows
  std::uint64_t malformed = 0;    // a field out of bounds, a short or overlong payload, bad padding or bad UTF-8

  [[nodiscard]] std::uint64_t Total() const noexcept
  {
    return unknown + wrongVersion + oversized + malformed;
  }
};

/// Decodes received messages and hands each to the handler of its type, once it has decoded whole: a handler never
/// sees a message that fails any check, and no input makes dispatching throw, allocate without a bound or recurse
/// without a limit. Used from one thread at a time.
class MessageDispatcher
{
public:
  template <MessageCodec T> void Register(std::function<void(const T&)> _handler)
  {
    m_handlers[T::TYPE] = Entry{T::VERSION, T::MAX_BYTES, [handler = std::move(_handler)](BitReader& _reader)
                                {
                                  T message{};
                                  message.Decode(_reader);
                                  if (!_reader.Finish())
                                  {
                                    return false;
                                  }
                                  handler(message);
                                  return true;
                                }};
  }

  // True when the message reached its handler; otherwise the reason is counted.
  bool Dispatch(std::span<const std::byte> _message);

  [[nodiscard]] const MessageDropCounts& Drops() const noexcept
  {
    return m_drops;
  }

private:
  struct Entry
  {
    std::uint8_t version = 0;
    std::size_t maxBytes = 0;
    std::function<bool(BitReader&)> decode;
  };

  std::map<MessageTypeId, Entry> m_handlers;
  MessageDropCounts m_drops;
};

} // namespace Neuron
