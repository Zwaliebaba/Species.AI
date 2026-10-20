#include "pch.h"

#include "Development.h"
#include "SimRng.h"
#include "TestSupport.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

constexpr Neuron::SipHashKey DRAW_KEY{0x0706050403020100, 0x0f0e0d0c0b0a0908};
constexpr std::uint64_t PURPOSE = 7;
constexpr std::uint64_t SUBJECT = (std::uint64_t{0x0100} << 48) | 1; // region 0's first object

} // namespace

TEST_CLASS(SimRngTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093ADrawIsSipHashOfItsKeys)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093ADrawIsSipHashOfItsKeys)
  {
    // ADR-082 item 2: SipHash-2-4 of the domain 1, the purpose, the subject, the tick and the ordinal, as five
    // little-endian words; the values are those of an independent reference implementation.
    constexpr std::array<std::uint64_t, 4> EXPECTED{0x15ab62c1e0416f09, 0x7ab10e12dd389ba4, 0xff35e69a0b505f7e, 0x80dc07ed66cba25d};
    Neuron::SimRng rng(DRAW_KEY, PURPOSE, SUBJECT, 600);
    for (const std::uint64_t expected : EXPECTED)
    {
      Assert::IsTrue(rng.NextValue() == expected, L"a reference value");
    }
    ExpectEqual(4, static_cast<std::int64_t>(rng.Ordinal()), L"the ordinal counts the draws");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093TheDrawFormsStayInTheirRanges)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093TheDrawFormsStayInTheirRanges)
  {
    Neuron::SimRng reference(DRAW_KEY, PURPOSE, SUBJECT, 600);
    const std::uint64_t first = reference.NextValue();
    Neuron::SimRng rng(DRAW_KEY, PURPOSE, SUBJECT, 600);
    Assert::IsTrue(rng.NextUint() == static_cast<std::uint32_t>(first >> 32), L"NextUint is the top 32 bits");
    for (int i = 0; i < 10'000; ++i)
    {
      const double unit = rng.NextUnit(3.0);
      Assert::IsTrue(unit >= 0.0 && unit < 3.0, L"NextUnit in [0, r)");
      const double centered = rng.NextSigned(2.0);
      Assert::IsTrue(centered >= -1.0 && centered < 1.0, L"NextSigned in [-r/2, r/2)");
      Assert::IsTrue(rng.NextBelow(7) < 7, L"NextBelow in [0, n)");
    }
    Assert::IsTrue(rng.NextBelow(1) == 0, L"one choice");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093ADrawAddedToOnePurposeChangesNoOther)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093ADrawAddedToOnePurposeChangesNoOther)
  {
    // ADR-082's confirmation: a purpose's values depend on its own keys only, so an extra draw elsewhere changes none.
    Neuron::SimRng before(DRAW_KEY, 8, SUBJECT, 600);
    const std::uint64_t untouched = before.NextValue();
    Neuron::SimRng added(DRAW_KEY, PURPOSE, SUBJECT, 600);
    static_cast<void>(added.NextValue());
    static_cast<void>(added.NextValue());
    Neuron::SimRng after(DRAW_KEY, 8, SUBJECT, 600);
    Assert::IsTrue(after.NextValue() == untouched, L"purpose 8 is unchanged");
    Neuron::SimRng otherTick(DRAW_KEY, 8, SUBJECT, 601);
    Assert::IsTrue(otherTick.NextValue() != untouched, L"the tick is a key");
    Neuron::SimRng otherSubject(DRAW_KEY, 8, SUBJECT + 1, 600);
    Assert::IsTrue(otherSubject.NextValue() != untouched, L"the subject is a key");
    Neuron::SimRng otherKey({1, 2}, 8, SUBJECT, 600);
    Assert::IsTrue(otherKey.NextValue() != untouched, L"the world's key is a key");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093DrawsAreBalancedOverTheStepsKeyPatterns)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093DrawsAreBalancedOverTheStepsKeyPatterns)
  {
    // The first form of ADR-082 item 8, with 2^20 values instead of 10^8: sequential ticks, sequential subjects,
    // ordinals 0 to 15 and four purposes. Each bound is above six standard deviations of a uniform source.
    constexpr std::size_t VALUES = std::size_t{1} << 20;
    std::array<std::uint64_t, 64> ones{};
    std::array<std::uint64_t, 256> buckets{};
    double sumProducts = 0.0;
    double sum = 0.0;
    double sumSquares = 0.0;
    double previous = 0.0;
    std::size_t made = 0;
    for (std::uint64_t purpose = 1; purpose <= 4 && made < VALUES; ++purpose)
    {
      for (std::uint64_t tick = 0; tick < 128 && made < VALUES; ++tick)
      {
        for (std::uint64_t subject = 1; subject <= 128 && made < VALUES; ++subject)
        {
          Neuron::SimRng rng(DRAW_KEY, purpose, SUBJECT + subject, tick);
          for (int ordinal = 0; ordinal < 16; ++ordinal)
          {
            const std::uint64_t value = rng.NextValue();
            for (std::size_t bit = 0; bit < 64; ++bit)
            {
              ones[bit] += (value >> bit) & 1;
            }
            ++buckets[value >> 56];
            const double unit = static_cast<double>(value >> 11) * 0x1.0p-53;
            sumProducts += unit * previous;
            sum += unit;
            sumSquares += unit * unit;
            previous = unit;
            ++made;
          }
        }
      }
    }
    const double count = static_cast<double>(made);
    ExpectEqual(static_cast<std::int64_t>(VALUES), static_cast<std::int64_t>(made), L"values drawn");
    for (const std::uint64_t setBits : ones)
    {
      Assert::IsTrue(std::fabs(static_cast<double>(setBits) / count - 0.5) < 0.003, L"each bit is set half the time");
    }
    double chiSquare = 0.0;
    const double expected = count / 256.0;
    for (const std::uint64_t inBucket : buckets)
    {
      const double difference = static_cast<double>(inBucket) - expected;
      chiSquare += difference * difference / expected;
    }
    Assert::IsTrue(chiSquare < 400.0, L"the top byte is uniform: chi-square below 400 for 255 degrees of freedom");
    const double mean = sum / count;
    const double variance = sumSquares / count - mean * mean;
    const double correlation = (sumProducts / count - mean * mean) / variance;
    Assert::IsTrue(std::fabs(correlation) < 0.006, L"successive values are uncorrelated");
  }

#if ENGINE_DEVELOPMENT
  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp093ASecondContextForOneSubjectAndTickIsADefect)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-093")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp093ASecondContextForOneSubjectAndTickIsADefect)
  {
    Neuron::DrawLedger ledger;
    ledger.BeginTick(600);
    const Neuron::SimRng first(DRAW_KEY, PURPOSE, SUBJECT, 600, &ledger);
    const Neuron::SimRng otherPurpose(DRAW_KEY, PURPOSE + 1, SUBJECT, 600, &ledger);
    Assert::IsTrue(TakeDevelopmentChecks().empty(), L"one context a purpose");
    const Neuron::SimRng second(DRAW_KEY, PURPOSE, SUBJECT, 600, &ledger);
    ExpectEqual(1, static_cast<std::int64_t>(TakeDevelopmentChecks().size()), L"the second context is reported");
    const Neuron::SimRng nextTick(DRAW_KEY, PURPOSE, SUBJECT, 601, &ledger);
    Assert::IsTrue(TakeDevelopmentChecks().empty(), L"a new tick starts a new ledger");
  }
#endif
};

} // namespace NeuronCoreTests
