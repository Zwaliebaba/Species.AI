#pragma once

#include "PlatformClock.h"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace Neuron
{

/// The profiling service's frame statistics in their M1 form (DBG-082, DBG-085; M1 task T15b, ADR-120): the frame-time
/// history with each frame's work, its time less the renderer's waits, and the time of each root step of the frame loop
/// over the last roll-up period. Main thread only.
class FrameProfile
{
public:
  struct Desc
  {
    std::size_t steps = 0;                   // the root steps the frame loop times, by index
    std::size_t historyFrames = 200;         // dbg.profile.frame_history
    Nanoseconds rollupPeriod{1'000'000'000}; // dbg.profile.rollup_period_ms
  };

  explicit FrameProfile(const Desc& _desc);

  FrameProfile(const FrameProfile&) = delete;
  FrameProfile& operator=(const FrameProfile&) = delete;

  // A root step's time, added to the current period.
  void AddStep(std::size_t _step, Nanoseconds _time);
  // After every frame: its time, and its work, the time less _waitTime, join the front of the history (DBG-085), and the
  // period rolls over once _now reaches its end (DBG-082). The first frame starts the first period.
  void EndFrame(Nanoseconds _now, Nanoseconds _frameTime, Nanoseconds _waitTime = Nanoseconds{0});

  // DBG-085: the history in milliseconds, newest first.
  [[nodiscard]] const std::deque<double>& FrameTimesMilliseconds() const noexcept
  {
    return m_frameTimes;
  }
  // DBG-096: a percentile of the history (ADR-120's lower index); 0 for an empty history.
  [[nodiscard]] double FramePercentileMilliseconds(double _percent) const;
  // ADR-120: the same percentile of the frames' work, which the frame's budget row reads (RND-454).
  [[nodiscard]] double WorkPercentileMilliseconds(double _percent) const;
  // DBG-096: a root step's last-period time × 100 ÷ the length of the last period; 0 before the first roll-over.
  [[nodiscard]] double StepSharePercent(std::size_t _step) const noexcept;
  [[nodiscard]] Nanoseconds LastPeriod() const noexcept
  {
    return m_lastPeriod;
  }
  // The periods rolled over so far; a caller that reports once a period watches it change.
  [[nodiscard]] std::uint64_t Periods() const noexcept
  {
    return m_periods;
  }

private:
  std::size_t m_historyFrames;
  Nanoseconds m_rollupPeriod;
  std::deque<double> m_frameTimes;
  std::deque<double> m_workTimes;
  std::vector<Nanoseconds> m_current;
  std::vector<Nanoseconds> m_last;
  Nanoseconds m_periodEnd{};
  Nanoseconds m_lastPeriod{};
  std::uint64_t m_periods = 0;
  bool m_started = false;
};

// ADR-120: the percentile of _values by the lower index, the value at ⌊_percent ÷ 100 × (n − 1)⌋ of the sorted values,
// as the performance tool reads its own (ADR-110); 0 for no values.
[[nodiscard]] double LowerIndexPercentile(std::vector<double> _values, double _percent);

} // namespace Neuron
