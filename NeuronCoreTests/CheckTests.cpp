#include "pch.h"

#include "Check.h"
#include "Development.h"
#include "TestSupport.h"

#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

TEST_CLASS(CheckTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Bld014DevelopmentSwitchFollowsTheConfiguration)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"BLD-014")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Bld014DevelopmentSwitchFollowsTheConfiguration)
  {
    // BLD T2: 1 in Debug, where NDEBUG is not defined, and 0 in Release.
#if defined(NDEBUG)
    ExpectEqual(0, ENGINE_DEVELOPMENT, L"ENGINE_DEVELOPMENT in Release");
#else
    ExpectEqual(1, ENGINE_DEVELOPMENT, L"ENGINE_DEVELOPMENT in Debug");
#endif
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg072FailedDevelopmentCheckReachesTheTestHarness)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-072")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg072FailedDevelopmentCheckReachesTheTestHarness)
  {
    const int units = 2;
    ENGINE_ASSERT(units == 3);
    const std::vector<std::string> checks = TakeDevelopmentChecks();
#if ENGINE_DEVELOPMENT
    ExpectEqual(1, static_cast<std::int64_t>(checks.size()), L"failed checks");
    ExpectContains(checks.front(), "units == 3", L"the condition");
    ExpectContains(checks.front(), "CheckTests.cpp", L"the file");
#else
    ExpectEqual(0, static_cast<std::int64_t>(checks.size()), L"development checks are compiled out of Release");
#endif
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg070PassingChecksDoNothing)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-070")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg070PassingChecksDoNothing)
  {
    const std::vector<int> values{1, 2, 3};
    ENGINE_ASSERT(values.size() == 3);
    ENGINE_CHECK(!values.empty());
    ENGINE_CHECK(values.front() == 1, "the first value is {}", values.front());
    ENGINE_CHECK_INDEX(2, values.size());
    ExpectEqual(0, static_cast<std::int64_t>(TakeDevelopmentChecks().size()), L"failed checks");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Dbg070CheckMessageIsFormattedFromItsArguments)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DBG-070")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dbg070CheckMessageIsFormattedFromItsArguments)
  {
    ExpectText("index 4 of 3", Neuron::CheckMessage("index {} of {}", 4, 3), L"a formatted message");
    Assert::IsTrue(Neuron::CheckMessage().empty(), L"a plain check has no message of its own");
  }
};

} // namespace NeuronCoreTests
