#pragma once

#include "Log.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Neuron
{

/// One record as a drain hands it to the sinks.
struct LogEntry
{
  std::int64_t timeNanoseconds; // the platform clock (ADR-074)
  std::int64_t utcTicks;        // std::chrono::system_clock ticks, for display only
  LogCategory category;
  LogLevel level;
  const std::string* threadName; // the writing thread's buffer owns it, and outlives the drain
  std::string message;
};

enum class LogWriteResult : std::uint8_t
{
  Written,
  Dropped,
  Full
};

// Drop counts per category for the two levels that are ever dropped: trace, then debug (DBG-062).
using LogDropCounts = std::array<std::array<std::uint64_t, 2>, LOG_CATEGORY_COUNT>;

/// The calling thread's log buffer (DBG-061): written by that thread alone, drained by one consumer at a time, and
/// lock-free between the two. It is two rings of half the capacity each: trace and debug records share one, which
/// admits trace records only while it is less than half full and debug records until it is full, and drops what it
/// cannot admit; info, warning, error and fatal records have the other to themselves, and a writer waits for the
/// drain only when that ring, which holds nothing but such records, is full. A sequence number on every record
/// restores the thread's order across the two rings. A message longer than a ring's quarter is written in parts.
class LogBuffer
{
public:
  LogBuffer(std::size_t _capacityBytes, std::string _threadName, std::function<void()> _requestDrain);

  LogBuffer(const LogBuffer&) = delete;
  LogBuffer& operator=(const LogBuffer&) = delete;

  // Producer, the owning thread only. One record of at most MaxPartBytes(); never waits.
  [[nodiscard]] LogWriteResult TryWrite(LogCategory _category, LogLevel _level, std::int64_t _timeNanoseconds, std::int64_t _utcTicks,
                                        std::string_view _message);
  // Producer: the whole message, in parts when it is long; waits only as the class comment says.
  void Write(LogCategory _category, LogLevel _level, std::int64_t _timeNanoseconds, std::int64_t _utcTicks, std::string_view _message);

  // Consumer: appends every complete record written so far, in the thread's order.
  void Drain(std::vector<LogEntry>& _entries);
  // Consumer: adds the drops since the previous call to _counts.
  void TakeNewDrops(LogDropCounts& _counts) noexcept;

  [[nodiscard]] std::uint64_t Dropped(LogCategory _category, LogLevel _level) const noexcept;
  [[nodiscard]] std::size_t MaxPartBytes() const noexcept
  {
    return m_maxPartBytes;
  }
  [[nodiscard]] const std::string& ThreadName() const noexcept
  {
    return m_threadName;
  }
  [[nodiscard]] bool IsEmpty() const noexcept;

  void MarkThreadEnded() noexcept
  {
    m_threadEnded.store(true, std::memory_order_release);
  }
  [[nodiscard]] bool ThreadEnded() const noexcept
  {
    return m_threadEnded.load(std::memory_order_acquire);
  }

private:
  struct Ring
  {
    std::unique_ptr<std::byte[]> bytes;
    std::size_t capacityBytes = 0;
    std::atomic<std::uint64_t> head{0}; // bytes ever written; the producer's
    std::atomic<std::uint64_t> tail{0}; // bytes ever consumed; the consumer's
  };

  bool Append(Ring& _ring, std::size_t _limitBytes, const void* _header, std::string_view _message, std::uint64_t& _observedTail) noexcept;
  void CountDrop(LogCategory _category, LogLevel _level) noexcept;

  Ring m_messages;
  Ring m_details;
  std::size_t m_maxPartBytes = 0;
  std::uint64_t m_nextSequence = 0;
  std::atomic<std::uint64_t> m_publishedSequence{0};
  std::array<std::array<std::atomic<std::uint64_t>, 2>, LOG_CATEGORY_COUNT> m_dropped{};
  LogDropCounts m_reportedDrops{};
  std::string m_threadName;
  std::function<void()> m_requestDrain;
  std::atomic<bool> m_threadEnded{false};
  // Consumer state: the parts of a long message read so far.
  bool m_hasPartial = false;
  LogEntry m_partial{};
};

} // namespace Neuron
