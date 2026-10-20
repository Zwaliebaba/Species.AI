#include "pch.h"

#include "PlatformClock.h"
#include "PlatformLogSink.h"

#include <cmath>
#include <utility>

namespace Neuron
{

namespace
{

/// One high-resolution waitable timer per thread that waits, closed when the thread ends (PLT-052).
class WaitableTimer
{
public:
  WaitableTimer() noexcept
    : m_handle(CreateWaitableTimerExW(nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS))
  {
  }

  ~WaitableTimer()
  {
    if (m_handle != nullptr)
    {
      CloseHandle(m_handle);
    }
  }

  WaitableTimer(const WaitableTimer&) = delete;
  WaitableTimer& operator=(const WaitableTimer&) = delete;

  [[nodiscard]] HANDLE Handle() const noexcept
  {
    return m_handle;
  }

private:
  HANDLE m_handle;
};

std::int64_t ReadCounter() noexcept
{
  LARGE_INTEGER count{};
  QueryPerformanceCounter(&count);
  return count.QuadPart;
}

std::int64_t ReadFrequency() noexcept
{
  LARGE_INTEGER frequency{};
  QueryPerformanceFrequency(&frequency);
  return frequency.QuadPart > 0 ? frequency.QuadPart : 1;
}

} // namespace

CounterTimeSource::CounterTimeSource() noexcept
  : m_originCount(ReadCounter()),
    m_frequency(ReadFrequency())
{
}

Nanoseconds CounterTimeSource::Now() const noexcept
{
  return CounterToNanoseconds(ReadCounter() - m_originCount, m_frequency);
}

Nanoseconds CounterTimeSource::Resolution() const noexcept
{
  return Nanoseconds{(1'000'000'000 + m_frequency - 1) / m_frequency};
}

FixedStepTimeSource::FixedStepTimeSource(std::uint32_t _framesPerSecond) noexcept
  : m_step(Nanoseconds{
      std::llround(1'000'000'000.0 / static_cast<double>(_framesPerSecond > 0 ? _framesPerSecond : FIXED_STEP_FRAMES_PER_SECOND))})
{
}

Nanoseconds FixedStepTimeSource::Now() const noexcept
{
  return Nanoseconds{m_nowNanoseconds.load(std::memory_order_acquire)};
}

void FixedStepTimeSource::FrameBoundary() noexcept
{
  m_nowNanoseconds.fetch_add(m_step.count(), std::memory_order_acq_rel);
}

Nanoseconds ManualTimeSource::Now() const noexcept
{
  return Nanoseconds{m_nowNanoseconds.load(std::memory_order_acquire)};
}

void ManualTimeSource::Advance(Nanoseconds _step) noexcept
{
  if (_step.count() < 0)
  {
    PlatformLog(PlatformLogLevel::Error, "ManualTimeSource::Advance refused a negative step");
    return;
  }
  m_nowNanoseconds.fetch_add(_step.count(), std::memory_order_acq_rel);
}

PlatformClock::PlatformClock()
  : m_source(std::make_unique<CounterTimeSource>())
{
}

PlatformClock::PlatformClock(std::unique_ptr<TimeSource> _source)
  : m_source(_source ? std::move(_source) : std::make_unique<CounterTimeSource>())
{
}

PlatformClock::~PlatformClock() = default;

Nanoseconds PlatformClock::Now() const noexcept
{
  const std::int64_t reading = m_source->Now().count() + m_offsetNanoseconds;
  std::int64_t last = m_lastNanoseconds.load(std::memory_order_acquire);
  while (reading > last)
  {
    if (m_lastNanoseconds.compare_exchange_weak(last, reading, std::memory_order_acq_rel, std::memory_order_acquire))
    {
      return Nanoseconds{reading};
    }
  }
  if (reading < last)
  {
    m_correctedReadings.fetch_add(1, std::memory_order_relaxed);
  }
  return Nanoseconds{last};
}

Nanoseconds PlatformClock::Resolution() const noexcept
{
  return m_source->Resolution();
}

TimeSourceKind PlatformClock::SourceKind() const noexcept
{
  return m_source->Kind();
}

void PlatformClock::FrameBoundary() noexcept
{
  m_source->FrameBoundary();
}

void PlatformClock::WaitUntil(Nanoseconds _deadline) const
{
  if (m_source->Kind() != TimeSourceKind::Real)
  {
    return;
  }
  thread_local WaitableTimer g_waitTimer;
  if (g_waitTimer.Handle() == nullptr)
  {
    return;
  }
  for (Nanoseconds remaining = _deadline - Now(); remaining.count() > 0; remaining = _deadline - Now())
  {
    // A relative due time is negative, in 100-nanosecond units; a remainder below one unit is not waited for.
    LARGE_INTEGER dueTime{};
    dueTime.QuadPart = -(remaining.count() / 100);
    if (dueTime.QuadPart == 0 || !SetWaitableTimerEx(g_waitTimer.Handle(), &dueTime, 0, nullptr, nullptr, nullptr, 0))
    {
      return;
    }
    WaitForSingleObject(g_waitTimer.Handle(), INFINITE);
  }
}

std::chrono::system_clock::time_point PlatformClock::CalendarUtc() const
{
  return std::chrono::system_clock::now();
}

std::chrono::minutes PlatformClock::LocalUtcOffset() const
{
  TIME_ZONE_INFORMATION zone{};
  const DWORD state = GetTimeZoneInformation(&zone);
  LONG biasMinutes = zone.Bias;
  if (state == TIME_ZONE_ID_DAYLIGHT)
  {
    biasMinutes += zone.DaylightBias;
  }
  else if (state == TIME_ZONE_ID_STANDARD)
  {
    biasMinutes += zone.StandardBias;
  }
  // Windows states the bias as UTC minus local time; the offset is its negation.
  return std::chrono::minutes{-biasMinutes};
}

void PlatformClock::ReplaceSource(std::unique_ptr<TimeSource> _source)
{
  if (!_source)
  {
    return;
  }
  const std::int64_t last = Now().count();
  m_offsetNanoseconds = last - _source->Now().count();
  m_source = std::move(_source);
}

std::uint64_t PlatformClock::CorrectedReadings() const noexcept
{
  return m_correctedReadings.load(std::memory_order_relaxed);
}

} // namespace Neuron
