#include "pch.h"

#include "PlatformThread.h"
#include "Profile.h"
#include "TestSupport.h"

#include <atomic>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

TEST_CLASS(ProfileTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg080ProfileScopesNestOnAnyThread)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-080")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg080ProfileScopesNestOnAnyThread)
  {
    // The markers reach the PIX runtime, so this also proves that WinPixEventRuntime.dll loads beside the tests.
    std::atomic<int> scopes{0};
    {
      ENGINE_PROFILE_SCOPE("test.outer");
      {
        ENGINE_PROFILE_SCOPE("test.inner");
        scopes.fetch_add(1, std::memory_order_relaxed);
      }
      Neuron::PlatformThread thread = Neuron::PlatformThread::Start("test.profiled",
                                                                    [&](const std::stop_token&)
                                                                    {
                                                                      ENGINE_PROFILE_SCOPE("test.worker");
                                                                      scopes.fetch_add(1, std::memory_order_relaxed);
                                                                    });
    }
    ExpectEqual(2, scopes.load(), L"scopes entered");
  }
};

} // namespace NeuronCoreTests
