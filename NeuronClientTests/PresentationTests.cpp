#include "pch.h"

#include "AdapterChoice.h"
#include "DisplayAttempt.h"
#include "PresentPolicy.h"
#include "TestSupport.h"

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

Neuron::AdapterCapabilities Capable(const char* _name)
{
  Neuron::AdapterCapabilities adapter;
  adapter.description = _name;
  adapter.featureLevelMet = true;
  adapter.shaderModel = 0x67;
  adapter.resourceBindingTier = 3;
  adapter.waveOps = true;
  return adapter;
}

} // namespace

TEST_CLASS(PresentationTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd020FirstCapableAdapterIsChosen)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-020")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd020FirstCapableAdapterIsChosen)
  {
    // RND T3: all pass, each single failure, and a software adapter ahead of the hardware.
    std::vector<Neuron::AdapterCapabilities> adapters{Capable("first"), Capable("second")};
    ExpectEqual(0, static_cast<std::int64_t>(Neuron::ChooseAdapter(adapters).adapter.value_or(99)), L"all pass: the first");

    const auto expectRejected = [](const Neuron::AdapterCapabilities& _failing, const std::string& _missing, const wchar_t* _what)
    {
      const std::array pair{_failing, Capable("second")};
      const Neuron::AdapterChoice choice = Neuron::ChooseAdapter(pair);
      Assert::IsTrue(choice.adapter == std::optional<std::size_t>{1}, _what);
      Assert::IsTrue(choice.missingCapability == _missing && choice.rejectedAdapter == "first", _what);
      const std::array alone{_failing};
      Assert::IsFalse(Neuron::ChooseAdapter(alone).adapter.has_value(), _what);
    };
    Neuron::AdapterCapabilities failing = Capable("first");
    failing.featureLevelMet = false;
    expectRejected(failing, "Direct3D feature level 12_1", L"feature level");
    failing = Capable("first");
    failing.shaderModel = 0x65;
    expectRejected(failing, "Shader Model 6.6", L"shader model");
    failing = Capable("first");
    failing.resourceBindingTier = 2;
    expectRejected(failing, "resource binding tier 3", L"binding tier");
    failing = Capable("first");
    failing.waveOps = false;
    expectRejected(failing, "wave operations", L"wave operations");

    Neuron::AdapterCapabilities software = Capable("software");
    software.software = true;
    const std::array withSoftware{software, Capable("hardware")};
    ExpectEqual(1, static_cast<std::int64_t>(Neuron::ChooseAdapter(withSoftware).adapter.value_or(99)), L"a software adapter is skipped");
    const std::array onlySoftware{software};
    const Neuron::AdapterChoice none = Neuron::ChooseAdapter(onlySoftware);
    Assert::IsTrue(!none.adapter.has_value() && none.missingCapability == "a hardware adapter", L"only a software adapter");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd020ShaderModelQueryLoopsDownFromTheHighestKnown)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-020")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd020ShaderModelQueryLoopsDownFromTheHighestKnown)
  {
    // The runtime knows models up to 6.8 and rejects the header's newer ones; the hardware supports 6.6.
    std::vector<std::uint32_t> asked;
    const auto query = [&asked](std::uint32_t _model) -> std::optional<std::uint32_t>
    {
      asked.push_back(_model);
      if (_model > 0x68)
      {
        return std::nullopt;
      }
      return std::min<std::uint32_t>(_model, 0x66);
    };
    ExpectEqual(0x66, Neuron::HighestShaderModel(0x6A, query), L"the supported model");
    ExpectEqual(3, static_cast<std::int64_t>(asked.size()), L"6.10 and 6.9 are rejected, 6.8 answers");
    ExpectEqual(0, Neuron::HighestShaderModel(0x6A, [](std::uint32_t) { return std::optional<std::uint32_t>{}; }), L"every model rejected");
    Assert::IsTrue(Neuron::ShaderModelName(0x66) == "6.6", L"the model's name");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd442PresentModeSelection)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-442")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd442PresentModeSelection)
  {
    // ADR-012's unit test: every combination of the setting, tearing support and the window mode.
    using Neuron::PresentMode;
    using Neuron::WindowMode;
    for (const bool tearing : {false, true})
    {
      for (const WindowMode mode : {WindowMode::Windowed, WindowMode::BorderlessFullScreen})
      {
        Assert::IsTrue(Neuron::SelectPresentPolicy(PresentMode::VerticalSync, tearing, mode) == Neuron::PresentPolicy{1, false},
                       L"vertical sync is Present(1, 0)");
        const bool allow = tearing && mode == WindowMode::BorderlessFullScreen;
        Assert::IsTrue(Neuron::SelectPresentPolicy(PresentMode::Uncapped, tearing, mode) == Neuron::PresentPolicy{0, allow},
                       L"uncapped tears only in borderless full screen with support");
      }
    }
    ExpectEqual(2, Neuron::MaximumFrameLatency(false), L"two frames queued");
    ExpectEqual(1, Neuron::MaximumFrameLatency(true), L"one with low latency");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Rnd023FallbackChainTakesTheNextAttempt)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-023")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Rnd023FallbackChainTakesTheNextAttempt)
  {
    // RND T6: forced failures of each attempt give the next attempt.
    const std::array attempts = Neuron::StartupDisplayAttempts(Neuron::WindowMode::BorderlessFullScreen, {}, {2560, 1440});
    Assert::IsTrue(attempts[0] == Neuron::DisplayAttempt{Neuron::WindowMode::BorderlessFullScreen, {2560, 1440}},
                   L"no stored size: the desktop size (RND-021)");
    Assert::IsTrue(attempts[1] == Neuron::DisplayAttempt{Neuron::WindowMode::BorderlessFullScreen, {2560, 1440}},
                   L"then the desktop size in the stored mode");
    Assert::IsTrue(attempts[2] == Neuron::DisplayAttempt{Neuron::WindowMode::Windowed, {800, 600}}, L"then windowed at 800 x 600");
    const std::array stored = Neuron::StartupDisplayAttempts(Neuron::WindowMode::Windowed, {1600, 900}, {2560, 1440});
    ExpectSize({1600, 900}, stored[0].renderSize, L"a stored size is preferred");

    for (std::size_t failures = 0; failures <= attempts.size(); ++failures)
    {
      std::size_t tried = 0;
      const Neuron::DisplayOutcome outcome =
        Neuron::RunDisplayAttempts(attempts, [&](const Neuron::DisplayAttempt&) { return tried++ >= failures; });
      if (failures < attempts.size())
      {
        Assert::IsTrue(outcome.succeeded == std::optional<std::size_t>{failures}, L"the first attempt that works is used");
      }
      else
      {
        Assert::IsFalse(outcome.succeeded.has_value(), L"every attempt failed: ui.error.display_mode");
      }
      Assert::IsTrue(outcome.resolutionRejected == (failures > 0), L"a failed first attempt queues ui.error.resolution_rejected");
    }
  }
};

} // namespace NeuronClientTests
