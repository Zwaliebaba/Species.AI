#include "pch.h"

#include "PlatformThread.h"
#include "TestSupport.h"
#include "TickLoop.h"
#include "TickSchedule.h"

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <optional>
#include <stop_token>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronServerTests
{

namespace
{

using Neuron::Nanoseconds;

constexpr Nanoseconds PERIOD{100}; // a small period keeps the schedule arithmetic readable

// Records when each tick started, and lets a test make a tick take time.
class RecordingTarget final : public Neuron::TickTarget
{
public:
  explicit RecordingTarget(ManualClock& _clock)
    : m_clock(_clock)
  {
  }

  void RunTick(std::uint64_t _tick) override
  {
    ticks.push_back(_tick);
    starts.push_back(m_clock.Now());
    if (duringTick)
    {
      duringTick(_tick);
    }
  }

  std::vector<std::uint64_t> ticks;
  std::vector<Nanoseconds> starts;
  std::function<void(std::uint64_t)> duringTick;

private:
  ManualClock& m_clock;
};

Neuron::TickLoop::Desc LoopOn(ManualClock& _clock, Nanoseconds _period, std::uint64_t _firstTick = 0)
{
  Neuron::TickLoop::Desc desc;
  desc.clock = &_clock.Clock();
  desc.period = _period;
  desc.firstTick = _firstTick;
  desc.waitUntil = [&_clock](Nanoseconds _deadline) { _clock.AdvanceTo(_deadline); };
  return desc;
}

} // namespace

TEST_CLASS(TickScheduleTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp092EachTickIsDueOnePeriodAfterTheLast)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-092")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp092EachTickIsDueOnePeriodAfterTheLast)
  {
    Neuron::TickSchedule schedule(PERIOD, Nanoseconds{1000}, 7);
    for (std::int64_t n = 0; n < 5; ++n)
    {
      ExpectEqual(1000 + n * 100, schedule.Deadline().count(), L"the deadline of tick 7 + n");
      ExpectEqual(schedule.Deadline().count(), schedule.EarliestStart().count(), L"on time, a tick starts at its deadline");
      Assert::IsFalse(schedule.Begin(schedule.Deadline()).has_value());
    }
    ExpectEqual(12, static_cast<std::int64_t>(schedule.NextTick()), L"the next tick");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp092ALateLoopCatchesUpAtTwiceTheRate)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-092")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp092ALateLoopCatchesUpAtTwiceTheRate)
  {
    Neuron::TickSchedule schedule(PERIOD, Nanoseconds{0}, 0);
    // The first tick starts 5.5 periods late: 5 ticks are owed.
    ExpectEqual(5, static_cast<std::int64_t>(schedule.OwedTicks(Nanoseconds{550})), L"owed ticks");
    std::optional<Nanoseconds> previous;
    Nanoseconds now{550};
    std::uint64_t caughtUp = 0;
    for (std::uint64_t i = 0; i < 20; ++i)
    {
      now = std::max(now, schedule.EarliestStart());
      if (previous)
      {
        Assert::IsTrue(now - *previous >= PERIOD / 2, L"no two starts closer than half a period");
      }
      Assert::IsTrue(now >= schedule.Deadline(), L"never before the deadline");
      if (caughtUp == 0 && now == schedule.Deadline())
      {
        caughtUp = schedule.NextTick();
      }
      Assert::IsFalse(schedule.Begin(now).has_value(), L"5 owed ticks are caught up, not given up");
      previous = now;
    }
    // Each start gains half a period: tick k starts at 550 + 50k and is on time once that reaches 100k.
    ExpectEqual(11, static_cast<std::int64_t>(caughtUp), L"the first tick back on its deadline");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp092MoreThanTwentyOwedTicksStartTheScheduleAgain)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-092")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp092MoreThanTwentyOwedTicksStartTheScheduleAgain)
  {
    {
      Neuron::TickSchedule schedule(PERIOD, Nanoseconds{0}, 0);
      // 20 owed ticks are caught up.
      Assert::IsFalse(schedule.Begin(Nanoseconds{20 * 100 + 99}).has_value());
    }
    Neuron::TickSchedule schedule(PERIOD, Nanoseconds{0}, 0);
    const Nanoseconds now{21 * 100 + 30};
    const std::optional<Neuron::ScheduleStall> stall = schedule.Begin(now);
    Assert::IsTrue(stall.has_value(), L"21 owed ticks start the schedule again");
    const Neuron::ScheduleStall given = stall.value_or(Neuron::ScheduleStall{});
    ExpectEqual(0, static_cast<std::int64_t>(given.tick), L"the stall's tick");
    ExpectEqual(now.count(), given.lost.count(), L"the real time given up");
    Assert::IsFalse(given.freeze, L"2.1 periods of 100 ns is no freeze");
    ExpectEqual((now + PERIOD).count(), schedule.Deadline().count(), L"the next tick is due a period after the present");
    ExpectEqual(1, static_cast<std::int64_t>(schedule.NextTick()), L"no tick number is skipped");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Pil131AGapOfTenSecondsIsAFreeze)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-131")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Pil131AGapOfTenSecondsIsAFreeze)
  {
    for (const auto& [lateNanoseconds, freeze] : {std::pair{9'999'999'999LL, false}, std::pair{10'000'000'000LL, true}})
    {
      Neuron::TickSchedule schedule(TEST_TICK_PERIOD, Nanoseconds{0}, 0);
      const std::optional<Neuron::ScheduleStall> stall = schedule.Begin(Nanoseconds{lateNanoseconds});
      Assert::IsTrue(stall.has_value(), L"a gap of seconds is given up");
      Assert::AreEqual(freeze, stall.value_or(Neuron::ScheduleStall{}).freeze, L"10 s or more is a freeze with the cause server stall");
    }
  }
};

TEST_CLASS(TickLoopTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp092TheLoopRuns600TicksOnTheDeadline)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-092")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp092TheLoopRuns600TicksOnTheDeadline)
  {
    ManualClock clock;
    clock.Advance(Nanoseconds{12'345});
    const Nanoseconds start = clock.Now();
    RecordingTarget target(clock);
    Neuron::TickLoop loop(LoopOn(clock, TEST_TICK_PERIOD, 40));
    const Neuron::TickLoopStats stats = loop.Run(target, std::stop_token{}, 600);

    ExpectEqual(600, static_cast<std::int64_t>(stats.ticks), L"ticks run");
    ExpectEqual(600, static_cast<std::int64_t>(loop.CompletedTicks()), L"ticks completed");
    ExpectEqual(0, static_cast<std::int64_t>(stats.lateTicks), L"late ticks");
    for (std::size_t n = 0; n < 600; ++n)
    {
      ExpectEqual(static_cast<std::int64_t>(40 + n), static_cast<std::int64_t>(target.ticks[n]), L"tick number");
      ExpectEqual((start + TEST_TICK_PERIOD * static_cast<std::int64_t>(n)).count(), target.starts[n].count(),
                  L"each tick starts on its deadline");
    }
    // 600 ticks of 0.1 s are a minute of world time.
    ExpectEqual(59'900'000'000, (target.starts.back() - start).count(), L"the last start");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp092ATickThatOverrunsLetsTheNextStartAtOnce)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-092")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp092ATickThatOverrunsLetsTheNextStartAtOnce)
  {
    ManualClock clock;
    RecordingTarget target(clock);
    target.duringTick = [&clock](std::uint64_t _tick)
    {
      if (_tick == 3)
      {
        clock.Advance(Nanoseconds{150});
      }
    };
    Neuron::TickLoop loop(LoopOn(clock, PERIOD));
    const Neuron::TickLoopStats stats = loop.Run(target, std::stop_token{}, 7);
    const std::vector<std::int64_t> expected{0, 100, 200, 300, 450, 500, 600};
    for (std::size_t n = 0; n < expected.size(); ++n)
    {
      ExpectEqual(expected[n], target.starts[n].count(), L"tick start");
    }
    ExpectEqual(1, static_cast<std::int64_t>(stats.lateTicks), L"late ticks");
    ExpectEqual(50, stats.latestStart.count(), L"the latest start");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Pil131AStallOfThirtySecondsIsAFreeze)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-131")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Pil131AStallOfThirtySecondsIsAFreeze)
  {
    ManualClock clock;
    RecordingTarget target(clock);
    target.duringTick = [&clock](std::uint64_t _tick)
    {
      if (_tick == 10)
      {
        clock.Advance(Nanoseconds{30'000'000'000});
      }
    };
    std::vector<Neuron::ScheduleStall> stalls;
    Neuron::TickLoop::Desc desc = LoopOn(clock, TEST_TICK_PERIOD);
    desc.onStall = [&stalls](const Neuron::ScheduleStall& _stall) { stalls.push_back(_stall); };
    Neuron::TickLoop loop(std::move(desc));
    const Neuron::TickLoopStats stats = loop.Run(target, std::stop_token{}, 20);

    ExpectEqual(1, static_cast<std::int64_t>(stalls.size()), L"one stall");
    Assert::IsTrue(stalls[0].freeze, L"recorded as a freeze");
    ExpectEqual(11, static_cast<std::int64_t>(stalls[0].tick), L"the stall's tick");
    ExpectEqual(29'900'000'000, stalls[0].lost.count(), L"30 s less the tick period that was due anyway");
    ExpectEqual(1, static_cast<std::int64_t>(stats.freezes), L"freezes");
    // After the stall the ticks follow the new schedule, one period apart.
    for (std::size_t n = 12; n < 20; ++n)
    {
      ExpectEqual(TEST_TICK_PERIOD.count(), (target.starts[n] - target.starts[n - 1]).count(), L"the period after a stall");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp092AStallOfTwoSecondsIsCaughtUpWithinTwoSeconds)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-092")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp092AStallOfTwoSecondsIsCaughtUpWithinTwoSeconds)
  {
    // ADR-083's confirmation at M1 (O68), on the injected clock: tick 10 takes 2 s. The ticks it leaves owed run at most
    // twice per period, none is given up, and the loop is back on its deadlines within 2 s of the stall's end.
    ManualClock clock;
    RecordingTarget target(clock);
    target.duringTick = [&clock](std::uint64_t _tick)
    {
      if (_tick == 10)
      {
        clock.Advance(Nanoseconds{2'000'000'000});
      }
    };
    std::vector<Neuron::ScheduleStall> stalls;
    Neuron::TickLoop::Desc desc = LoopOn(clock, TEST_TICK_PERIOD);
    desc.onStall = [&stalls](const Neuron::ScheduleStall& _stall) { stalls.push_back(_stall); };
    Neuron::TickLoop loop(std::move(desc));
    const Neuron::TickLoopStats stats = loop.Run(target, std::stop_token{}, 70);

    Assert::IsTrue(stalls.empty() && stats.stalls == 0 && stats.freezes == 0, L"nothing given up");
    const Nanoseconds stallEnd = target.starts[10] + Nanoseconds{2'000'000'000};
    std::optional<std::size_t> caughtUp;
    for (std::size_t n = 11; n < target.starts.size(); ++n)
    {
      Assert::IsTrue(target.starts[n] - target.starts[n - 1] >= TEST_TICK_PERIOD / 2, L"at most twice the rate");
      const Nanoseconds deadline = target.starts[0] + TEST_TICK_PERIOD * static_cast<std::int64_t>(n);
      if (!caughtUp && target.starts[n] == deadline)
      {
        caughtUp = n;
      }
      if (caughtUp)
      {
        ExpectEqual(deadline.count(), target.starts[n].count(), L"on its deadline once caught up");
      }
    }
    Assert::IsTrue(caughtUp.has_value(), L"the loop catches up");
    // Each start gains half a period on its deadline: tick 11 starts 1.9 s late and tick 49 is on time, 1.9 s later.
    ExpectEqual(49, static_cast<std::int64_t>(caughtUp.value_or(0)), L"the first tick back on its deadline");
    Assert::IsTrue(target.starts[caughtUp.value_or(0)] - stallEnd <= Nanoseconds{2'000'000'000}, L"within 2 s");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp092OnTheRealClockNoTickStartsBeforeItsDeadline)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-092")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp092OnTheRealClockNoTickStartsBeforeItsDeadline)
  {
    const Neuron::PlatformClock clock;
    constexpr Nanoseconds REAL_PERIOD{10'000'000}; // 10 ms keeps the test short
    struct Target final : Neuron::TickTarget
    {
      explicit Target(const Neuron::PlatformClock& _clock)
        : source(_clock)
      {
      }
      void RunTick(std::uint64_t) override
      {
        starts.push_back(source.Now());
      }
      const Neuron::PlatformClock& source;
      std::vector<Nanoseconds> starts;
    } target(clock);
    Neuron::TickLoop::Desc desc;
    desc.clock = &clock;
    desc.period = REAL_PERIOD;
    Neuron::TickLoop loop(std::move(desc));
    const Nanoseconds start = clock.Now();
    std::atomic<bool> done{false};
    Neuron::PlatformThread thread = Neuron::PlatformThread::Start("test.tick",
                                                                  [&](const std::stop_token& _stop)
                                                                  {
                                                                    static_cast<void>(loop.Run(target, _stop, 30));
                                                                    done.store(true);
                                                                  });
    thread.Join();
    Assert::IsTrue(done.load(), L"the loop ended after its 30 ticks");
    for (std::size_t n = 0; n < target.starts.size(); ++n)
    {
      Assert::IsTrue(target.starts[n] >= start + REAL_PERIOD * static_cast<std::int64_t>(n), L"a tick never starts early");
    }
    Assert::IsTrue(clock.Now() - start >= REAL_PERIOD * 29, L"30 ticks take at least 29 periods");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp092AStopRequestEndsTheLoop)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-092")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp092AStopRequestEndsTheLoop)
  {
    ManualClock clock;
    RecordingTarget target(clock);
    std::stop_source stop;
    target.duringTick = [&stop](std::uint64_t _tick)
    {
      if (_tick == 4)
      {
        stop.request_stop();
      }
    };
    Neuron::TickLoop loop(LoopOn(clock, PERIOD));
    const Neuron::TickLoopStats stats = loop.Run(target, stop.get_token());
    ExpectEqual(5, static_cast<std::int64_t>(stats.ticks), L"the tick that saw the request completes, and no other starts");
  }
};

} // namespace NeuronServerTests
