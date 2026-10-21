#include "pch.h"

#include "FrameProfile.h"
#include "TestSupport.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

using std::chrono::milliseconds;

TEST_CLASS(FrameProfileTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg085TheHistoryKeepsTheNewestFramesFirst)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-085")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg085TheHistoryKeepsTheNewestFramesFirst)
  {
    Neuron::FrameProfile profile({.steps = 1, .historyFrames = 3, .rollupPeriod = milliseconds(1000)});
    for (int frame = 1; frame <= 5; ++frame)
    {
      profile.EndFrame(milliseconds(frame * 10), milliseconds(frame));
    }
    const std::deque<double>& history = profile.FrameTimesMilliseconds();
    Assert::IsTrue(history.size() == 3 && history[0] == 5.0 && history[1] == 4.0 && history[2] == 3.0,
                   L"the three newest frames, in milliseconds, the newest first");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg096ThePercentilesTakeTheLowerIndex)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-096")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg096ThePercentilesTakeTheLowerIndex)
  {
    // ADR-120: the value at the lower index of p × (n − 1) of the sorted history.
    Neuron::FrameProfile profile({.steps = 0, .historyFrames = 200, .rollupPeriod = milliseconds(1000)});
    Assert::IsTrue(profile.FramePercentileMilliseconds(50.0) == 0.0, L"an empty history reads 0");
    for (int frame = 100; frame >= 1; --frame)
    {
      profile.EndFrame(milliseconds(101 - frame), milliseconds(frame));
    }
    Assert::IsTrue(profile.FramePercentileMilliseconds(50.0) == 50.0, L"the 50th of 1 to 100 ms is index 49");
    Assert::IsTrue(profile.FramePercentileMilliseconds(95.0) == 95.0, L"the 95th is index 94");
    Assert::IsTrue(profile.FramePercentileMilliseconds(99.0) == 99.0, L"the 99th is index 98");
    Assert::IsTrue(profile.FramePercentileMilliseconds(100.0) == 100.0, L"the 100th is the longest");
    Assert::IsTrue(Neuron::LowerIndexPercentile({4.0, 2.0, 3.0, 1.0}, 50.0) == 2.0, L"index 1 of four");
    // A frame's work is its time less the renderer's waits, never below nothing.
    Neuron::FrameProfile work({.steps = 0, .historyFrames = 200, .rollupPeriod = milliseconds(1000)});
    work.EndFrame(milliseconds(10), milliseconds(17), milliseconds(12));
    work.EndFrame(milliseconds(27), milliseconds(17), milliseconds(20));
    Assert::IsTrue(work.WorkPercentileMilliseconds(0.0) == 0.0 && work.WorkPercentileMilliseconds(100.0) == 5.0,
                   L"5 ms of work, and none for a frame that waited longer than it lasted");
    Assert::IsTrue(work.FramePercentileMilliseconds(100.0) == 17.0, L"the frame's time keeps its waits");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg082TheSharesAreOfTheLastPeriod)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-082")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-096")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg082TheSharesAreOfTheLastPeriod)
  {
    Neuron::FrameProfile profile({.steps = 2, .historyFrames = 200, .rollupPeriod = milliseconds(1000)});
    profile.EndFrame(milliseconds(0), milliseconds(16)); // starts the first period, which ends at 1000 ms
    profile.AddStep(0, milliseconds(100));
    profile.AddStep(1, milliseconds(300));
    profile.AddStep(7, milliseconds(300)); // a step beyond the profile's is not counted
    profile.EndFrame(milliseconds(990), milliseconds(16));
    Assert::IsTrue(profile.StepSharePercent(0) == 0.0, L"no share before the first roll-over");
    ExpectEqual(0, static_cast<std::int64_t>(profile.Periods()), L"no period has ended");
    profile.EndFrame(milliseconds(1250), milliseconds(16));
    ExpectEqual(1, static_cast<std::int64_t>(profile.Periods()), L"one period has ended");
    ExpectEqual(1250, std::chrono::duration_cast<milliseconds>(profile.LastPeriod()).count(), L"from its start to now");
    Assert::IsTrue(std::abs(profile.StepSharePercent(0) - 8.0) < 1e-9 && std::abs(profile.StepSharePercent(1) - 24.0) < 1e-9,
                   L"each step's time × 100 ÷ the period's length");
    Assert::IsTrue(profile.StepSharePercent(2) == 0.0, L"a step beyond the profile's reads 0");
    // The next period ends one roll-up period after the roll-over, and starts again from nothing.
    profile.EndFrame(milliseconds(2240), milliseconds(16));
    ExpectEqual(1, static_cast<std::int64_t>(profile.Periods()), L"the next period has not ended at 2240 ms");
    profile.EndFrame(milliseconds(2250), milliseconds(16));
    Assert::IsTrue(profile.Periods() == 2 && profile.StepSharePercent(0) == 0.0, L"an empty period gives no share");
  }
};

} // namespace NeuronCoreTests
