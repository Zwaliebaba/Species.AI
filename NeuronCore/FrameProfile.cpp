#include "pch.h"

#include "Check.h"
#include "FrameProfile.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace Neuron
{

FrameProfile::FrameProfile(const Desc& _desc)
  : m_historyFrames(_desc.historyFrames),
    m_rollupPeriod(_desc.rollupPeriod),
    m_current(_desc.steps, Nanoseconds{0}),
    m_last(_desc.steps, Nanoseconds{0})
{
  ENGINE_CHECK(m_rollupPeriod > Nanoseconds{0}, "the profile's roll-up period is positive");
}

void FrameProfile::AddStep(std::size_t _step, Nanoseconds _time)
{
  if (_step < m_current.size())
  {
    m_current[_step] += _time;
  }
}

void FrameProfile::EndFrame(Nanoseconds _now, Nanoseconds _frameTime, Nanoseconds _waitTime)
{
  m_frameTimes.push_front(std::chrono::duration<double, std::milli>(_frameTime).count());
  m_workTimes.push_front(std::chrono::duration<double, std::milli>(std::max(_frameTime - _waitTime, Nanoseconds{0})).count());
  while (m_frameTimes.size() > m_historyFrames)
  {
    m_frameTimes.pop_back();
    m_workTimes.pop_back();
  }
  if (!m_started)
  {
    m_started = true;
    m_periodEnd = _now + m_rollupPeriod;
    return;
  }
  if (_now < m_periodEnd)
  {
    return;
  }
  // DBG-082: the period just ended ran from its start to now, and the next ends one roll-up period after now.
  m_lastPeriod = _now - (m_periodEnd - m_rollupPeriod);
  m_periodEnd = _now + m_rollupPeriod;
  m_last = m_current;
  std::ranges::fill(m_current, Nanoseconds{0});
  ++m_periods;
}

double FrameProfile::FramePercentileMilliseconds(double _percent) const
{
  return LowerIndexPercentile({m_frameTimes.begin(), m_frameTimes.end()}, _percent);
}

double FrameProfile::WorkPercentileMilliseconds(double _percent) const
{
  return LowerIndexPercentile({m_workTimes.begin(), m_workTimes.end()}, _percent);
}

double FrameProfile::StepSharePercent(std::size_t _step) const noexcept
{
  if (_step >= m_last.size() || m_lastPeriod <= Nanoseconds{0})
  {
    return 0.0;
  }
  return static_cast<double>(m_last[_step].count()) * 100.0 / static_cast<double>(m_lastPeriod.count());
}

double LowerIndexPercentile(std::vector<double> _values, double _percent)
{
  if (_values.empty())
  {
    return 0.0;
  }
  std::ranges::sort(_values);
  const double share = std::clamp(_percent / 100.0, 0.0, 1.0);
  const auto index = static_cast<std::size_t>(std::floor(share * static_cast<double>(_values.size() - 1)));
  return _values[index];
}

} // namespace Neuron
