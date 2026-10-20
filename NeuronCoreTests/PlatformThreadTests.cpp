#include "pch.h"

#include <float.h>

#include "FloatEnvironment.h"
#include "PlatformThread.h"
#include "TestSupport.h"

#include <atomic>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

TEST_CLASS(PlatformThreadTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt083ThreadIsNamedAndStopsWhenDestroyed)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-083")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt083ThreadIsNamedAndStopsWhenDestroyed)
  {
    std::string name;
    std::atomic<bool> started{false};
    std::atomic<bool> returned{false};
    {
      Neuron::PlatformThread thread = Neuron::PlatformThread::Start("test.worker",
                                                                    [&](const std::stop_token& _stop)
                                                                    {
                                                                      name = Neuron::CurrentThreadName();
                                                                      started.store(true, std::memory_order_release);
                                                                      while (!_stop.stop_requested())
                                                                      {
                                                                        std::this_thread::yield();
                                                                      }
                                                                      returned.store(true, std::memory_order_release);
                                                                    });
      while (!started.load(std::memory_order_acquire))
      {
        std::this_thread::yield();
      }
      Assert::IsTrue(thread.Joinable(), L"the thread runs");
    }
    Assert::IsTrue(returned.load(std::memory_order_acquire), L"destruction requested stop and joined");
    Assert::AreEqual("test.worker", name.c_str());
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt087EveryThreadStartsInTheDefaultEnvironment)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-087")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt087EveryThreadStartsInTheDefaultEnvironment)
  {
    // The starting thread flushes to zero; the new thread still starts in the default environment (PLT T26).
    unsigned int saved = 0;
    _controlfp_s(&saved, 0, 0);
    unsigned int control = 0;
    _controlfp_s(&control, _DN_FLUSH, _MCW_DN);
    std::atomic<bool> isDefault{false};
    {
      Neuron::PlatformThread thread = Neuron::PlatformThread::Start(
        "test.float", [&](const std::stop_token&) { isDefault.store(Neuron::IsDefaultFloatEnvironment(), std::memory_order_release); });
    }
    _controlfp_s(&control, saved, _MCW_DN);
    Assert::IsTrue(isDefault.load(std::memory_order_acquire), L"a new thread starts in the default environment");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt087FlushToZeroIsNotTheDefaultEnvironment)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-087")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt087FlushToZeroIsNotTheDefaultEnvironment)
  {
    unsigned int saved = 0;
    _controlfp_s(&saved, 0, 0);
    Neuron::ApplyDefaultFloatEnvironment();
    Assert::IsTrue(Neuron::IsDefaultFloatEnvironment(), L"the default environment");
    unsigned int control = 0;
    _controlfp_s(&control, _DN_FLUSH, _MCW_DN);
    const bool flushing = Neuron::IsDefaultFloatEnvironment();
    _controlfp_s(&control, saved, _MCW_DN);
    Assert::IsFalse(flushing, L"flush-to-zero fails the check");
  }
};

} // namespace NeuronCoreTests
