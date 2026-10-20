#include "pch.h"

#include "LogBuffer.h"

#include <cstddef>
#include <cstring>
#include <utility>

namespace Neuron
{

namespace
{

enum class RecordKind : std::uint8_t
{
  Padding,  // the rest of the ring up to its end is unused; only sizeBytes and kind are written
  Message,  // a whole message, or the last part of one
  Continued // a part of a message whose next part follows
};

struct RecordHeader
{
  std::uint32_t sizeBytes; // the header and the message, rounded up to RECORD_ALIGNMENT_BYTES
  RecordKind kind;
  LogLevel level;
  LogCategory category;
  std::uint8_t reserved;
  std::uint32_t messageBytes;
  std::uint32_t reservedWord;
  std::uint64_t sequence;
  std::int64_t timeNanoseconds;
  std::int64_t utcTicks;
};

constexpr std::size_t RECORD_ALIGNMENT_BYTES = 8;
// What a padding record writes, and so the least space any record leaves at the end of a ring.
constexpr std::size_t PADDING_HEADER_BYTES = offsetof(RecordHeader, messageBytes);

static_assert(sizeof(RecordHeader) == 40 && sizeof(RecordHeader) % RECORD_ALIGNMENT_BYTES == 0);
static_assert(PADDING_HEADER_BYTES == RECORD_ALIGNMENT_BYTES);

constexpr std::size_t RecordBytes(std::size_t _messageBytes) noexcept
{
  return (sizeof(RecordHeader) + _messageBytes + RECORD_ALIGNMENT_BYTES - 1) & ~(RECORD_ALIGNMENT_BYTES - 1);
}

constexpr std::size_t DropIndex(LogLevel _level) noexcept
{
  return _level == LogLevel::Trace ? 0 : 1;
}

} // namespace

LogBuffer::LogBuffer(std::size_t _capacityBytes, std::string _threadName, std::function<void()> _requestDrain)
  : m_threadName(std::move(_threadName)),
    m_requestDrain(std::move(_requestDrain))
{
  // Each ring is half the buffer, in whole records' alignment, and never smaller than four largest records need.
  std::size_t ringBytes = (_capacityBytes / 2) & ~(RECORD_ALIGNMENT_BYTES - 1);
  if (ringBytes < 4 * RecordBytes(64))
  {
    ringBytes = 4 * RecordBytes(64);
  }
  for (Ring* ring : {&m_messages, &m_details})
  {
    ring->bytes = std::make_unique<std::byte[]>(ringBytes);
    ring->capacityBytes = ringBytes;
  }
  m_maxPartBytes = ringBytes / 4 - sizeof(RecordHeader);
}

LogWriteResult LogBuffer::TryWrite(LogCategory _category, LogLevel _level, std::int64_t _timeNanoseconds, std::int64_t _utcTicks,
                                   std::string_view _message)
{
  if (_message.size() > m_maxPartBytes)
  {
    _message = _message.substr(0, m_maxPartBytes);
  }
  const RecordHeader header{static_cast<std::uint32_t>(RecordBytes(_message.size())),
                            RecordKind::Message,
                            _level,
                            _category,
                            0,
                            static_cast<std::uint32_t>(_message.size()),
                            0,
                            m_nextSequence,
                            _timeNanoseconds,
                            _utcTicks};
  const bool details = _level < LogLevel::Info;
  Ring& ring = details ? m_details : m_messages;
  const std::size_t limitBytes = _level == LogLevel::Trace ? ring.capacityBytes / 2 : ring.capacityBytes;
  std::uint64_t observedTail = 0;
  if (Append(ring, limitBytes, &header, _message, observedTail))
  {
    ++m_nextSequence;
    m_publishedSequence.store(m_nextSequence, std::memory_order_release);
    return LogWriteResult::Written;
  }
  if (details)
  {
    CountDrop(_category, _level);
    return LogWriteResult::Dropped;
  }
  return LogWriteResult::Full;
}

void LogBuffer::Write(LogCategory _category, LogLevel _level, std::int64_t _timeNanoseconds, std::int64_t _utcTicks,
                      std::string_view _message)
{
  const std::size_t partCount = _message.empty() ? 1 : (_message.size() + m_maxPartBytes - 1) / m_maxPartBytes;
  const bool details = _level < LogLevel::Info;
  Ring& ring = details ? m_details : m_messages;
  if (details && partCount > 1)
  {
    // A trace or debug message is admitted whole or not at all. One padding record at most, because the parts
    // together are shorter than the ring.
    const std::size_t limitBytes = _level == LogLevel::Trace ? ring.capacityBytes / 2 : ring.capacityBytes;
    const std::size_t neededBytes = (partCount + 1) * RecordBytes(m_maxPartBytes);
    const std::uint64_t used = ring.head.load(std::memory_order_relaxed) - ring.tail.load(std::memory_order_acquire);
    if (neededBytes > ring.capacityBytes || used + neededBytes > limitBytes)
    {
      CountDrop(_category, _level);
      return;
    }
  }
  for (std::size_t part = 0; part < partCount; ++part)
  {
    const std::string_view text = _message.substr(part * m_maxPartBytes, m_maxPartBytes);
    const RecordHeader header{static_cast<std::uint32_t>(RecordBytes(text.size())),
                              part + 1 < partCount ? RecordKind::Continued : RecordKind::Message,
                              _level,
                              _category,
                              0,
                              static_cast<std::uint32_t>(text.size()),
                              0,
                              m_nextSequence,
                              _timeNanoseconds,
                              _utcTicks};
    const std::size_t limitBytes = _level == LogLevel::Trace ? ring.capacityBytes / 2 : ring.capacityBytes;
    std::uint64_t observedTail = 0;
    while (!Append(ring, limitBytes, &header, text, observedTail))
    {
      if (details)
      {
        CountDrop(_category, _level);
        return;
      }
      // The ring holds nothing but info and higher records: wait for the drain to free space (DBG-061).
      if (m_requestDrain)
      {
        m_requestDrain();
      }
      ring.tail.wait(observedTail, std::memory_order_acquire);
    }
    ++m_nextSequence;
    m_publishedSequence.store(m_nextSequence, std::memory_order_release);
  }
}

bool LogBuffer::Append(Ring& _ring, std::size_t _limitBytes, const void* _header, std::string_view _message,
                       std::uint64_t& _observedTail) noexcept
{
  RecordHeader header{};
  std::memcpy(&header, _header, sizeof(header));
  const std::uint64_t head = _ring.head.load(std::memory_order_relaxed);
  const std::uint64_t tail = _ring.tail.load(std::memory_order_acquire);
  _observedTail = tail;
  std::size_t position = static_cast<std::size_t>(head % _ring.capacityBytes);
  const std::size_t toEndBytes = _ring.capacityBytes - position;
  const std::size_t paddingBytes = header.sizeBytes > toEndBytes ? toEndBytes : 0;
  if (static_cast<std::size_t>(head - tail) + paddingBytes + header.sizeBytes > _limitBytes)
  {
    return false;
  }
  if (paddingBytes > 0)
  {
    const RecordHeader padding{
      static_cast<std::uint32_t>(paddingBytes), RecordKind::Padding, LogLevel::Trace, LogCategory::Platform, 0, 0, 0, 0, 0, 0};
    std::memcpy(_ring.bytes.get() + position, &padding, PADDING_HEADER_BYTES);
    position = 0;
  }
  std::memcpy(_ring.bytes.get() + position, &header, sizeof(header));
  if (!_message.empty())
  {
    std::memcpy(_ring.bytes.get() + position + sizeof(header), _message.data(), _message.size());
  }
  _ring.head.store(head + paddingBytes + header.sizeBytes, std::memory_order_release);
  return true;
}

void LogBuffer::Drain(std::vector<LogEntry>& _entries)
{
  // Every record below the published sequence is visible in its ring once the sequence is read.
  const std::uint64_t published = m_publishedSequence.load(std::memory_order_acquire);
  std::uint64_t messagesTail = m_messages.tail.load(std::memory_order_relaxed);
  std::uint64_t detailsTail = m_details.tail.load(std::memory_order_relaxed);

  // The next record of a ring, past any padding; false when the ring holds none below the published sequence.
  auto peek = [published](const Ring& _ring, std::uint64_t& _tail, RecordHeader& _header) noexcept
  {
    const std::uint64_t head = _ring.head.load(std::memory_order_acquire);
    while (_tail < head)
    {
      const std::byte* record = _ring.bytes.get() + static_cast<std::size_t>(_tail % _ring.capacityBytes);
      RecordHeader start{};
      std::memcpy(&start, record, PADDING_HEADER_BYTES);
      if (start.kind == RecordKind::Padding)
      {
        _tail += start.sizeBytes;
        continue;
      }
      std::memcpy(&_header, record, sizeof(_header));
      return _header.sequence < published;
    }
    return false;
  };

  while (true)
  {
    RecordHeader messageHeader{};
    RecordHeader detailHeader{};
    const bool hasMessage = peek(m_messages, messagesTail, messageHeader);
    const bool hasDetail = peek(m_details, detailsTail, detailHeader);
    if (!hasMessage && !hasDetail)
    {
      break;
    }
    const bool takeMessage = hasMessage && (!hasDetail || messageHeader.sequence < detailHeader.sequence);
    const Ring& ring = takeMessage ? m_messages : m_details;
    std::uint64_t& tail = takeMessage ? messagesTail : detailsTail;
    const RecordHeader& header = takeMessage ? messageHeader : detailHeader;
    const char* text =
      reinterpret_cast<const char*>(ring.bytes.get() + static_cast<std::size_t>(tail % ring.capacityBytes) + sizeof(RecordHeader));
    if (!m_hasPartial)
    {
      m_partial = LogEntry{header.timeNanoseconds, header.utcTicks, header.category, header.level, &m_threadName, {}};
      m_hasPartial = true;
    }
    m_partial.message.append(text, header.messageBytes);
    if (header.kind == RecordKind::Message)
    {
      _entries.push_back(std::move(m_partial));
      m_partial = LogEntry{};
      m_hasPartial = false;
    }
    tail += header.sizeBytes;
  }
  m_details.tail.store(detailsTail, std::memory_order_release);
  m_messages.tail.store(messagesTail, std::memory_order_release);
  m_messages.tail.notify_all();
}

void LogBuffer::TakeNewDrops(LogDropCounts& _counts) noexcept
{
  for (std::size_t category = 0; category < LOG_CATEGORY_COUNT; ++category)
  {
    for (std::size_t level = 0; level < 2; ++level)
    {
      const std::uint64_t total = m_dropped[category][level].load(std::memory_order_relaxed);
      _counts[category][level] += total - m_reportedDrops[category][level];
      m_reportedDrops[category][level] = total;
    }
  }
}

std::uint64_t LogBuffer::Dropped(LogCategory _category, LogLevel _level) const noexcept
{
  if (_level > LogLevel::Debug)
  {
    return 0;
  }
  return m_dropped[static_cast<std::size_t>(_category)][DropIndex(_level)].load(std::memory_order_relaxed);
}

bool LogBuffer::IsEmpty() const noexcept
{
  return !m_hasPartial && m_messages.head.load(std::memory_order_acquire) == m_messages.tail.load(std::memory_order_acquire) &&
         m_details.head.load(std::memory_order_acquire) == m_details.tail.load(std::memory_order_acquire);
}

void LogBuffer::CountDrop(LogCategory _category, LogLevel _level) noexcept
{
  m_dropped[static_cast<std::size_t>(_category)][DropIndex(_level)].fetch_add(1, std::memory_order_relaxed);
}

} // namespace Neuron
