#pragma once

#include "PlatformClock.h"
#include "TickSchedule.h"

#include <atomic>
#include <cstdint>
#include <functional>
#include <limits>
#include <stop_token>

namespace Neuron
{

/// What the loop runs: one whole tick at a time, on the loop's thread (ADR-079 item 2).
class TickTarget
{
public:
  virtual ~TickTarget() = default;

  virtual void RunTick(std::uint64_t _tick) = 0;
};

struct TickLoopStats
{
  std::uint64_t ticks = 0;
  std::uint64_t lateTicks = 0; // started after their deadline
  Nanoseconds latestStart{0};  // the largest delay of a start after its deadline
  std::uint64_t stalls = 0;    // schedules started again at the present
  std::uint64_t freezes = 0;   // stalls of STALL_FREEZE_NANOSECONDS or more
  Nanoseconds lost{0};         // real time given up, which is not world time
};

/// The server's tick loop (ADR-083 item 7): it sleeps to each tick's deadline, never busy-waits and never takes its
/// rate from a client's frames. It runs on the thread that calls Run.
class TickLoop
{
public:
  struct Desc
  {
    const PlatformClock* clock = nullptr;
    Nanoseconds period{0}; // `ref.sim.tick_period`
    std::uint64_t firstTick = 0;
    // Returns once the clock reads at least the deadline; the clock's own wait when empty. A test whose time source
    // is manual advances it here.
    std::function<void(Nanoseconds)> waitUntil;
    // Told of every stall, on the loop's thread, after it is logged; a freeze is the world's to record (PIL-131).
    std::function<void(const ScheduleStall&)> onStall;
  };

  explicit TickLoop(Desc _desc);

  // Runs ticks until stop is requested or _tickLimit ticks have run; the first is due at once.
  TickLoopStats Run(TickTarget& _target, const std::stop_token& _stop,
                    std::uint64_t _tickLimit = std::numeric_limits<std::uint64_t>::max());

  // Any thread: the ticks completed so far.
  [[nodiscard]] std::uint64_t CompletedTicks() const noexcept
  {
    return m_completedTicks.load(std::memory_order_acquire);
  }

private:
  Desc m_desc;
  std::atomic<std::uint64_t> m_completedTicks{0};
};

} // namespace Neuron
