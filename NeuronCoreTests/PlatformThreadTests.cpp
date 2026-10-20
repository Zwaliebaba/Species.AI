#include "pch.h"

#include <immintrin.h>

#include "FloatEnvironment.h"
#include "PlatformThread.h"
#include "TestSupport.h"

#include <atomic>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

constexpr unsigned int MXCSR_FLUSH_TO_ZERO = 0x8000;

} // namespace

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
    const unsigned int saved = _mm_getcsr();
    _mm_setcsr(saved | MXCSR_FLUSH_TO_ZERO);
    std::atomic<bool> isDefault{false};
    {
      Neuron::PlatformThread thread = Neuron::PlatformThread::Start(
        "test.float", [&](const std::stop_token&) { isDefault.store(Neuron::IsDefaultFloatEnvironment(), std::memory_order_release); });
    }
    _mm_setcsr(saved);
    Assert::IsTrue(isDefault.load(std::memory_order_acquire), L"a new thread starts in the default environment");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt087FlushToZeroIsNotTheDefaultEnvironment)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-087")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt087FlushToZeroIsNotTheDefaultEnvironment)
  {
    const unsigned int saved = _mm_getcsr();
    Neuron::ApplyDefaultFloatEnvironment();
    Assert::IsTrue(Neuron::IsDefaultFloatEnvironment(), L"the default environment");
    _mm_setcsr(_mm_getcsr() | MXCSR_FLUSH_TO_ZERO);
    const bool flushing = Neuron::IsDefaultFloatEnvironment();
    _mm_setcsr(saved);
    Assert::IsFalse(flushing, L"flush-to-zero fails the check");
  }
};

} // namespace NeuronCoreTests
