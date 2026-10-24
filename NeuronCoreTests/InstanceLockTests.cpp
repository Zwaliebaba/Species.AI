#include "pch.h"

#include "InstanceLock.h"
#include "ProgramStart.h"
#include "TestSupport.h"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

// Names of this test run's own, so that no other process's lock or signal is touched.
constexpr std::wstring_view LOCK_NAME = L"Species.Tests.InstanceLock";
constexpr std::wstring_view SIGNAL_NAME = L"Species.Tests.StopSignal";

} // namespace

TEST_CLASS(InstanceLockTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071OneHolderAtATime)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071OneHolderAtATime)
  {
    // ADR-190, O110: the lock is taken once; a second taker is refused while it is held, and takes it once it has gone.
    Assert::IsFalse(Neuron::InstanceLock::Held(LOCK_NAME), L"nobody holds it yet");
    {
      std::optional<Neuron::InstanceLock> first = Neuron::InstanceLock::TryTake(LOCK_NAME);
      Assert::IsTrue(first.has_value() && Neuron::InstanceLock::Held(LOCK_NAME), L"taken and held");
      Assert::IsFalse(Neuron::InstanceLock::TryTake(LOCK_NAME).has_value(), L"a second taker is refused");
      if (first)
      {
        const Neuron::InstanceLock moved = std::move(*first);
        Assert::IsTrue(Neuron::InstanceLock::Held(LOCK_NAME), L"held while its holder is moved");
      }
    }
    Assert::IsFalse(Neuron::InstanceLock::Held(LOCK_NAME), L"gone once its holder ends");
    Assert::IsTrue(Neuron::InstanceLock::TryTake(LOCK_NAME).has_value(), L"and taken again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071AStopSignalReachesItsMaker)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071AStopSignalReachesItsMaker)
  {
    // ADR-190: a signal no process made cannot be raised; one made is not raised until another raises it.
    Assert::IsFalse(Neuron::StopSignal::Raise(SIGNAL_NAME), L"no process made it");
    const std::optional<Neuron::StopSignal> signal = Neuron::StopSignal::Create(SIGNAL_NAME);
    Assert::IsTrue(signal.has_value(), L"made");
    Assert::IsFalse(signal.has_value() && signal->Wait(0), L"not raised yet");
    Assert::IsTrue(Neuron::StopSignal::Raise(SIGNAL_NAME), L"raised");
    Assert::IsTrue(signal.has_value() && signal->Wait(1'000), L"its maker sees it");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Mvp071ACommandLineQuotesEachArgument)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-071")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Mvp071ACommandLineQuotesEachArgument)
  {
    // ADR-190: each argument is quoted so that CommandLineToArgvW reads it back: backslashes stay as they are inside,
    // and are doubled before a quote, which is escaped, and before the closing quote.
    const std::array<std::wstring, 3> arguments{L"--always-on-world", L"C:\\Users\\A B\\saves\\world-0001\\", L"say \"hi\""};
    const std::wstring line = Neuron::CommandLineOf(std::filesystem::path(L"C:\\Species\\SpeciesServer.exe"), arguments);
    Assert::IsTrue(line == L"\"C:\\Species\\SpeciesServer.exe\" \"--always-on-world\" \"C:\\Users\\A B\\saves\\world-0001\\\\\" "
                           L"\"say \\\"hi\\\"\"",
                   L"the command line");
    Assert::IsTrue(Neuron::ProcessRunning(GetCurrentProcessId()), L"this process runs");
  }
};

} // namespace NeuronCoreTests
