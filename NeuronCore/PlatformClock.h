#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>

namespace Neuron
{

// Time since the clock's origin, the start of the process's clock (ADR-074, PLT-046).
using Nanoseconds = std::chrono::duration<std::int64_t, std::nano>;

enum class TimeSourceKind : std::uint8_t
{
  Real,
  FixedStep,
  Manual
};

/// Where the platform clock takes its readings from (PLT-048): the performance counter, or a source that a test or
/// the fixed-step mode advances.
class TimeSource
{
public:
  virtual ~TimeSource() = default;

  [[nodiscard]] virtual Nanoseconds Now() const noexcept = 0;
  [[nodiscard]] virtual Nanoseconds Resolution() const noexcept = 0;
  [[nodiscard]] virtual TimeSourceKind Kind() const noexcept = 0;
  virtual void FrameBoundary() noexcept {}
};

/// Converts a performance-counter interval to nanoseconds without overflow for any frequency up to 9.2 GHz (PLT-046):
/// whole seconds and the remainder are scaled apart, so no intermediate exceeds 63 bits.
[[nodiscard]] constexpr Nanoseconds CounterToNanoseconds(std::int64_t _elapsedCount, std::int64_t _frequency) noexcept
{
  const std::int64_t whole = _elapsedCount / _frequency;
  const std::int64_t rest = _elapsedCount % _frequency;
  return Nanoseconds{whole * 1'000'000'000 + rest * 1'000'000'000 / _frequency};
}

/// The performance counter, latched at construction (PLT-046).
class CounterTimeSource final : public TimeSource
{
public:
  CounterTimeSource() noexcept;

  [[nodiscard]] Nanoseconds Now() const noexcept override;
  [[nodiscard]] Nanoseconds Resolution() const noexcept override;
  [[nodiscard]] TimeSourceKind Kind() const noexcept override
  {
    return TimeSourceKind::Real;
  }

private:
  std::int64_t m_originCount = 0;
  std::int64_t m_frequency = 1;
};

/// Advances one step per rendered frame and never otherwise (PLT-048): the presentation step of the command-history
/// modes, `dbg.replay.capture_frame_rate` frames per second.
class FixedStepTimeSource final : public TimeSource
{
public:
  explicit FixedStepTimeSource(std::uint32_t _framesPerSecond) noexcept;

  [[nodiscard]] Nanoseconds Now() const noexcept override;
  [[nodiscard]] Nanoseconds Resolution() const noexcept override
  {
    return m_step;
  }
  [[nodiscard]] TimeSourceKind Kind() const noexcept override
  {
    return TimeSourceKind::FixedStep;
  }
  void FrameBoundary() noexcept override;

private:
  Nanoseconds m_step;
  std::atomic<std::int64_t> m_nowNanoseconds{0};
};

/// Advances only when a test says so (PLT-048, DBG-141). Tests only.
class ManualTimeSource final : public TimeSource
{
public:
  [[nodiscard]] Nanoseconds Now() const noexcept override;
  [[nodiscard]] Nanoseconds Resolution() const noexcept override
  {
    return Nanoseconds{1};
  }
  [[nodiscard]] TimeSourceKind Kind() const noexcept override
  {
    return TimeSourceKind::Manual;
  }

  // A negative step is refused and reported on the platform log: the clock never runs backwards.
  void Advance(Nanoseconds _step) noexcept;

private:
  std::atomic<std::int64_t> m_nowNanoseconds{0};
};

// The presentation step of the fixed-step source when no rate is given (`dbg.replay.capture_frame_rate`).
inline constexpr std::uint32_t FIXED_STEP_FRAMES_PER_SECOND = 25;

/// The process's one clock (ADR-074). Readings never decrease, on any thread, whatever the source does.
class PlatformClock
{
public:
  PlatformClock();
  explicit PlatformClock(std::unique_ptr<TimeSource> _source);
  ~PlatformClock();

  PlatformClock(const PlatformClock&) = delete;
  PlatformClock& operator=(const PlatformClock&) = delete;

  // Any thread; lock-free: one source reading, integer arithmetic and one atomic maximum (PLT-108).
  [[nodiscard]] Nanoseconds Now() const noexcept;
  [[nodiscard]] Nanoseconds Resolution() const noexcept;
  [[nodiscard]] TimeSourceKind SourceKind() const noexcept;

  // The frame loop calls it once per rendered frame; the fixed-step source advances here.
  void FrameBoundary() noexcept;

  // Returns once Now() has reached the deadline, at once if it has passed or the source is not the real one (PLT-052).
  void WaitUntil(Nanoseconds _deadline) const;

  // Calendar time, for logs, reports and display only: it never feeds timing (PLT-053).
  [[nodiscard]] std::chrono::system_clock::time_point CalendarUtc() const;
  [[nodiscard]] std::chrono::minutes LocalUtcOffset() const;

  // Start-up and tests only, never while another thread reads the clock. The new source continues from the last
  // reading (PLT-051).
  void ReplaceSource(std::unique_ptr<TimeSource> _source);

  // How many readings the never-decrease rule corrected (PLT-047); the composition root logs it at shutdown.
  [[nodiscard]] std::uint64_t CorrectedReadings() const noexcept;

private:
  std::unique_ptr<TimeSource> m_source;
  std::int64_t m_offsetNanoseconds = 0;
  mutable std::atomic<std::int64_t> m_lastNanoseconds{0};
  mutable std::atomic<std::uint64_t> m_correctedReadings{0};
};

} // namespace Neuron
