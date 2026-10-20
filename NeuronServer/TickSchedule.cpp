#include "pch.h"

#include "Check.h"
#include "TickSchedule.h"

#include <algorithm>

namespace Neuron
{

TickSchedule::TickSchedule(Nanoseconds _period, Nanoseconds _start, std::uint64_t _firstTick) noexcept
  : m_period(_period),
    m_anchor(_start),
    m_anchorTick(_firstTick),
    m_nextTick(_firstTick)
{
  ENGINE_ASSERT(_period.count() > 0);
}

Nanoseconds TickSchedule::Deadline() const noexcept
{
  return m_anchor + m_period * static_cast<std::int64_t>(m_nextTick - m_anchorTick);
}

Nanoseconds TickSchedule::EarliestStart() const noexcept
{
  const Nanoseconds deadline = Deadline();
  if (!m_lastStart)
  {
    return deadline;
  }
  return std::max(deadline, *m_lastStart + m_period / static_cast<std::int64_t>(CATCH_UP_TICKS_PER_PERIOD));
}

std::uint64_t TickSchedule::OwedTicks(Nanoseconds _now) const noexcept
{
  const Nanoseconds late = _now - Deadline();
  return late.count() <= 0 ? 0 : static_cast<std::uint64_t>(late / m_period);
}

std::optional<ScheduleStall> TickSchedule::Begin(Nanoseconds _now) noexcept
{
  ENGINE_ASSERT(_now >= EarliestStart());
  std::optional<ScheduleStall> stall;
  if (OwedTicks(_now) > CATCH_UP_OWED_TICKS_MAX)
  {
    const Nanoseconds lost = _now - Deadline();
    m_anchor = _now;
    m_anchorTick = m_nextTick;
    stall = ScheduleStall{m_nextTick, lost, lost >= STALL_FREEZE_NANOSECONDS};
  }
  m_lastStart = _now;
  ++m_nextTick;
  return stall;
}

} // namespace Neuron
