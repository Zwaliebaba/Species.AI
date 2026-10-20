#pragma once

#include "PlatformClock.h"

#include <cstdint>
#include <optional>

namespace Neuron
{

// ADR-083 item 7: a late loop runs its owed ticks at most twice per period, for up to 20 owed ticks; beyond that the
// schedule starts again at the present, and a gap of 10 s or more is a freeze with the cause "server stall" (PIL-131).
inline constexpr std::uint64_t CATCH_UP_TICKS_PER_PERIOD = 2;
inline constexpr std::uint64_t CATCH_UP_OWED_TICKS_MAX = 20;
inline constexpr Nanoseconds STALL_FREEZE_NANOSECONDS{10'000'000'000};

/// Real time that the schedule gave up: it is not world time (O4).
struct ScheduleStall
{
  std::uint64_t tick = 0; // the first tick of the new schedule
  Nanoseconds lost{0};    // how late that tick was against the old schedule
  bool freeze = false;    // a gap of STALL_FREEZE_NANOSECONDS or more
};

/// When each tick of the server loop starts (ADR-083 item 7). Tick n is due at the anchor plus n tick periods; the
/// loop sleeps to that deadline and never starts a tick before it. A loop that is late runs its owed ticks back to back,
/// but no two starts closer than half a period, and gives up the time beyond CATCH_UP_OWED_TICKS_MAX owed ticks.
/// Pure arithmetic on the readings it is given, so a test drives it with any clock.
class TickSchedule
{
public:
  TickSchedule(Nanoseconds _period, Nanoseconds _start, std::uint64_t _firstTick) noexcept;

  [[nodiscard]] std::uint64_t NextTick() const noexcept
  {
    return m_nextTick;
  }
  [[nodiscard]] Nanoseconds Period() const noexcept
  {
    return m_period;
  }
  // The deadline of the next tick on the current schedule.
  [[nodiscard]] Nanoseconds Deadline() const noexcept;
  // The earliest reading at which the next tick may start: its deadline, and half a period after the last start.
  [[nodiscard]] Nanoseconds EarliestStart() const noexcept;
  // The ticks after the next one whose deadlines have passed at _now.
  [[nodiscard]] std::uint64_t OwedTicks(Nanoseconds _now) const noexcept;

  // The next tick starts at _now, which is at least EarliestStart(). Returns the stall when the schedule had to start
  // again at the present; the tick then starts on the new schedule.
  [[nodiscard]] std::optional<ScheduleStall> Begin(Nanoseconds _now) noexcept;

private:
  Nanoseconds m_period;
  Nanoseconds m_anchor;
  std::uint64_t m_anchorTick;
  std::uint64_t m_nextTick;
  std::optional<Nanoseconds> m_lastStart;
};

} // namespace Neuron
