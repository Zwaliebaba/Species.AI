#include "pch.h"

#include "Check.h"
#include "Log.h"
#include "Profile.h"
#include "TickLoop.h"

#include <algorithm>
#include <utility>

namespace Neuron
{

namespace
{

double Milliseconds(Nanoseconds _time) noexcept
{
  return static_cast<double>(_time.count()) / 1'000'000.0;
}

} // namespace

TickLoop::TickLoop(Desc _desc)
  : m_desc(std::move(_desc))
{
  ENGINE_CHECK(m_desc.clock != nullptr && m_desc.period.count() > 0);
}

TickLoopStats TickLoop::Run(TickTarget& _target, const std::stop_token& _stop, std::uint64_t _tickLimit)
{
  const PlatformClock& clock = *m_desc.clock;
  TickSchedule schedule(m_desc.period, clock.Now(), m_desc.firstTick);
  TickLoopStats stats;
  while (!_stop.stop_requested() && stats.ticks < _tickLimit)
  {
    const Nanoseconds earliest = schedule.EarliestStart();
    const Nanoseconds now = clock.Now();
    if (now < earliest)
    {
      if (m_desc.waitUntil)
      {
        m_desc.waitUntil(earliest);
      }
      else
      {
        clock.WaitUntil(earliest);
      }
      continue; // the stop request and the reading are taken again after every wait
    }

    const std::uint64_t tick = schedule.NextTick();
    Nanoseconds late = now - schedule.Deadline();
    if (const std::optional<ScheduleStall> stall = schedule.Begin(now))
    {
      late = Nanoseconds{0};
      ++stats.stalls;
      stats.lost += stall->lost;
      if (stall->freeze)
      {
        ++stats.freezes;
      }
      Log(LogCategory::Simulation, LogLevel::Warning, "the tick loop gave up {:.1f} ms before tick {}{}", Milliseconds(stall->lost),
          stall->tick, stall->freeze ? ", a freeze with the cause server stall" : "");
      if (m_desc.onStall)
      {
        m_desc.onStall(*stall);
      }
    }
    if (late.count() > 0)
    {
      ++stats.lateTicks;
      stats.latestStart = std::max(stats.latestStart, late);
    }
    {
      ENGINE_PROFILE_SCOPE("server.tick");
      _target.RunTick(tick);
    }
    ++stats.ticks;
    m_completedTicks.fetch_add(1, std::memory_order_release);
  }
  return stats;
}

} // namespace Neuron
