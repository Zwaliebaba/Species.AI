#include "pch.h"

#include "PlatformMessage.h"
#include "RunId.h"
#include "TestSupport.h"

#include <array>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronCoreTests
{

namespace
{

constexpr std::array TEXTS{
  Neuron::CompiledText{"ui.error.gpu_unsupported", "The adapter {adapter} lacks {capability}."},
  Neuron::CompiledText{"ui.error.internal", "Report run {runId}; {unknown} stays as it is, and so does an unclosed {brace"},
};

} // namespace

TEST_CLASS(PlatformMessageTests)
{
public:
  TEST_METHOD_CLEANUP(Cleanup)
  {
    ExpectNoDevelopmentCheck();
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt039CompiledInTextTakesItsArguments)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-039")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt039CompiledInTextTakesItsArguments)
  {
    const std::array arguments{Neuron::MessageArgument{"capability", "Shader Model 6.6"}, Neuron::MessageArgument{"adapter", "GPU 1"}};
    ExpectText("The adapter GPU 1 lacks Shader Model 6.6.", Neuron::ComposePlatformMessage(TEXTS, "ui.error.gpu_unsupported", arguments),
               L"named arguments in any order");
    const std::array runId{Neuron::MessageArgument{"runId", "00000000000000ff"}};
    ExpectText("Report run 00000000000000ff; {unknown} stays as it is, and so does an unclosed {brace",
               Neuron::ComposePlatformMessage(TEXTS, "ui.error.internal", runId), L"an unknown or unclosed argument stays");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt039IdWithoutTextShowsTheIdAndArguments)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-039")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt039IdWithoutTextShowsTheIdAndArguments)
  {
    const std::array arguments{Neuron::MessageArgument{"seconds", "5"}};
    ExpectText("ui.error.unknown seconds=5", Neuron::ComposePlatformMessage(TEXTS, "ui.error.unknown", arguments), L"no compiled-in text");
    ExpectText("ui.error.unknown", Neuron::ComposePlatformMessage({}, "ui.error.unknown", {}), L"no texts and no arguments");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt036RunIdIsSixteenHexDigits)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-036")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt036RunIdIsSixteenHexDigits)
  {
    ExpectText("00000000000000ff", Neuron::FormatRunId(255), L"leading zeros");
    ExpectText("fedcba9876543210", Neuron::FormatRunId(0xFEDC'BA98'7654'3210), L"lowercase");
    // Two draws from the cryptographic generator are equal with a chance of one in 2^64.
    Assert::IsTrue(Neuron::DrawRunId() != Neuron::DrawRunId(), L"each draw is new");
  }
};

} // namespace NeuronCoreTests
