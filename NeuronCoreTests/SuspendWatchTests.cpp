#include "pch.h"

#include "SuspendWatch.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

TEST_CLASS(SuspendWatchTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071EachSuspendCountsOnceAndAResumeNot)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PIL-131")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071EachSuspendCountsOnceAndAResumeNot)
  {
    // ADR-187: the watch registers for the system's suspend and resume notifications, its count starts at 0, a suspend
    // counts once and a resume not at all; it unregisters when it ends.
    {
      Neuron::SuspendWatch watch;
      Assert::IsTrue(watch.Watching(), L"registered");
      Assert::IsTrue(watch.Suspensions() == 0, L"no suspend yet");
      watch.Notify(PBT_APMSUSPEND);
      watch.Notify(PBT_APMRESUMESUSPEND);
      watch.Notify(PBT_APMRESUMEAUTOMATIC);
      Assert::IsTrue(watch.Suspensions() == 1, L"one suspend");
    }
    Neuron::SuspendWatch again;
    Assert::IsTrue(again.Watching(), L"registered again after the first ended");
  }
};

} // namespace NeuronCoreTests
