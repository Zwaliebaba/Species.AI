#include "pch.h"

#include "PlatformClock.h"
#include "PlatformThread.h"
#include "TestSupport.h"

#include <array>
#include <atomic>
#include <memory>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

// A pinned generator, so a failing case reproduces.
std::uint64_t SplitMix64(std::uint64_t& _state) noexcept
{
  std::uint64_t z = (_state += 0x9E3779B97F4A7C15ULL);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
  return z ^ (z >> 31);
}

// floor(_count * 10^9 / _frequency) in 128-bit arithmetic: a product of two 64-bit halves, then long division.
std::uint64_t ReferenceNanoseconds(std::uint64_t _count, std::uint64_t _frequency) noexcept
{
  constexpr std::uint64_t SCALE = 1'000'000'000;
  const std::uint64_t lowProduct = (_count & 0xFFFFFFFFULL) * SCALE;
  const std::uint64_t highProduct = (_count >> 32) * SCALE;
  std::uint64_t low = lowProduct + (highProduct << 32);
  std::uint64_t high = (highProduct >> 32) + (low < lowProduct ? 1 : 0);
  std::uint64_t quotient = 0;
  std::uint64_t remainder = 0;
  for (int bit = 127; bit >= 0; --bit)
  {
    const std::uint64_t next = bit >= 64 ? (high >> (bit - 64)) & 1 : (low >> bit) & 1;
    const bool carry = (remainder >> 63) != 0;
    remainder = (remainder << 1) | next;
    if (carry || remainder >= _frequency)
    {
      remainder -= _frequency;
      if (bit < 64)
      {
        quotient |= 1ULL << bit;
      }
    }
  }
  return quotient;
}

} // namespace

TEST_CLASS(PlatformClockTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt046CounterConversionMatchesWideArithmetic)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-046")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt046CounterConversionMatchesWideArithmetic)
  {
    // Frequencies from 1 kHz to 9.2 GHz, intervals up to ten years at that frequency (PLT T2).
    std::uint64_t state = 0x5EC1E5;
    for (int sample = 0; sample < 20000; ++sample)
    {
      const std::uint64_t frequency = 1000 + SplitMix64(state) % (9'200'000'000ULL - 1000);
      const std::uint64_t count = SplitMix64(state) % (frequency * 315'360'000ULL);
      const auto converted = Neuron::CounterToNanoseconds(static_cast<std::int64_t>(count), static_cast<std::int64_t>(frequency));
      ExpectEqual(static_cast<std::int64_t>(ReferenceNanoseconds(count, frequency)), converted.count(), L"nanoseconds");
    }
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt046ReadingsNeverDecreaseOnEightThreads)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-046")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt046ReadingsNeverDecreaseOnEightThreads)
  {
    // 10^6 readings on each of 8 threads (PLT T1).
    const Neuron::PlatformClock clock;
    Assert::IsTrue(clock.Now().count() >= 0, L"the first reading is not negative");
    std::atomic<int> decreases{0};
    {
      std::vector<Neuron::PlatformThread> threads;
      threads.reserve(8);
      for (int index = 0; index < 8; ++index)
      {
        threads.push_back(Neuron::PlatformThread::Start("test.clock_reader",
                                                        [&clock, &decreases](const std::stop_token&)
                                                        {
                                                          Neuron::Nanoseconds last = clock.Now();
                                                          for (int reading = 0; reading < 1'000'000; ++reading)
                                                          {
                                                            const Neuron::Nanoseconds now = clock.Now();
                                                            if (now < last)
                                                            {
                                                              decreases.fetch_add(1, std::memory_order_relaxed);
                                                            }
                                                            last = now;
                                                          }
                                                        }));
      }
    }
    ExpectEqual(0, decreases.load(), L"decreasing readings");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt047ResolutionIsOneCounterTick)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-047")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt047ResolutionIsOneCounterTick)
  {
    const Neuron::PlatformClock clock;
    Assert::IsTrue(clock.SourceKind() == Neuron::TimeSourceKind::Real, L"the default source is the counter");
    Assert::IsTrue(clock.Resolution().count() >= 1 && clock.Resolution().count() <= 1'000'000, L"a counter tick of at most 1 ms");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt048FixedStepAdvancesOnlyAtFrameBoundaries)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-048")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt048FixedStepAdvancesOnlyAtFrameBoundaries)
  {
    // Without a rate the step is 1/25 s: 100 boundaries are exactly 4 s, and waiting returns at once (PLT T3).
    Neuron::PlatformClock clock(std::make_unique<Neuron::FixedStepTimeSource>(0));
    ExpectEqual(40'000'000, clock.Resolution().count(), L"step");
    ExpectEqual(0, clock.Now().count(), L"before the first boundary");
    for (int frame = 0; frame < 100; ++frame)
    {
      clock.FrameBoundary();
    }
    ExpectEqual(4'000'000'000, clock.Now().count(), L"after 100 boundaries");
    const Neuron::PlatformClock real;
    const Neuron::Nanoseconds before = real.Now();
    clock.WaitUntil(clock.Now() + Neuron::Nanoseconds{60'000'000'000});
    Assert::IsTrue(real.Now() - before < Neuron::Nanoseconds{1'000'000'000}, L"WaitUntil returned at once");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt048ManualSourceAdvancesOnlyWhenTold)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-048")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt048ManualSourceAdvancesOnlyWhenTold)
  {
    auto source = std::make_unique<Neuron::ManualTimeSource>();
    Neuron::ManualTimeSource& manual = *source;
    Neuron::PlatformClock clock(std::move(source));
    ExpectEqual(0, clock.Now().count(), L"start");
    manual.Advance(Neuron::Nanoseconds{5'000'000});
    ExpectEqual(5'000'000, clock.Now().count(), L"after 5 ms");
    manual.Advance(Neuron::Nanoseconds{-1});
    ExpectEqual(5'000'000, clock.Now().count(), L"a negative step is refused");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt051ReplacedSourceContinuesFromTheLastReading)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-051")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt051ReplacedSourceContinuesFromTheLastReading)
  {
    // PLT T4.
    auto first = std::make_unique<Neuron::ManualTimeSource>();
    first->Advance(Neuron::Nanoseconds{3'000'000'000});
    Neuron::PlatformClock clock(std::move(first));
    ExpectEqual(3'000'000'000, clock.Now().count(), L"before the replacement");
    auto second = std::make_unique<Neuron::ManualTimeSource>();
    Neuron::ManualTimeSource& replacement = *second;
    replacement.Advance(Neuron::Nanoseconds{10'000'000'000});
    clock.ReplaceSource(std::move(second));
    ExpectEqual(3'000'000'000, clock.Now().count(), L"right after the replacement");
    replacement.Advance(Neuron::Nanoseconds{1'000'000'000});
    ExpectEqual(4'000'000'000, clock.Now().count(), L"one second later");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt052WaitUntilReachesItsDeadline)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-052")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt052WaitUntilReachesItsDeadline)
  {
    const Neuron::PlatformClock clock;
    const Neuron::Nanoseconds deadline = clock.Now() + Neuron::Nanoseconds{3'000'000};
    clock.WaitUntil(deadline);
    Assert::IsTrue(clock.Now() >= deadline, L"the deadline has passed when WaitUntil returns");
    clock.WaitUntil(deadline - Neuron::Nanoseconds{1'000'000'000});
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt053CalendarTimeIsUtcWithALocalOffset)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-053")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt053CalendarTimeIsUtcWithALocalOffset)
  {
    const Neuron::PlatformClock clock;
    Assert::IsTrue(clock.CalendarUtc().time_since_epoch().count() > 0, L"a calendar time after 1970");
    const auto offset = clock.LocalUtcOffset();
    Assert::IsTrue(offset >= -std::chrono::hours{14} && offset <= std::chrono::hours{14}, L"an offset within 14 hours");
  }
};

} // namespace NeuronCoreTests
