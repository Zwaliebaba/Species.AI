#include "pch.h"

#include "ClientSettings.h"
#include "TestSupport.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

TEST_CLASS(ClientSettingsTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Dat193EveryDefaultFitsItsSetting)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-193")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"DAT-195")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-442")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Dat193EveryDefaultFitsItsSetting)
  {
    const std::vector<Neuron::SettingSpec> specs = Species::ClientSettingSpecs();
    for (const Neuron::SettingSpec& spec : specs)
    {
      Assert::IsTrue(spec.key.ends_with('.') || Neuron::IsSettingKey(spec.key), L"every key is a setting key");
      Assert::IsTrue(!spec.defaultValue.has_value() || Neuron::SettingValueFits(spec, *spec.defaultValue), L"every default fits");
    }
    // RND-442's defaults, read back as the renderer and the presentation take them.
    const Neuron::SettingsStore store(specs, std::nullopt);
    const Species::RenderSettings render = Species::RenderSettingsOf(store);
    Assert::IsFalse(render.uncapped, L"vertical sync");
    Assert::IsFalse(render.frameRateCap.has_value(), L"no frame-rate cap");
    Assert::IsFalse(render.lowLatency, L"low latency off");
    ExpectEqual(4, render.sceneSamplesPreferred, L"4x anti-aliasing");
    Assert::IsFalse(render.windowed, L"borderless full screen");
    Assert::IsTrue(render.renderSize == Neuron::SizePx{}, L"no render size until the first start writes one");
    const Species::DisplayOptions display = Species::DisplayOptionsOf(store);
    ExpectEqual(1, display.seaDetail, L"sea detail 1");
    ExpectEqual(1, display.entityDetail, L"entity detail 1");
    Assert::AreEqual(2.0, display.workerScaleFactor, L"the Worker distance scale factor");
    Assert::AreEqual(1.0, display.brightness, L"brightness 1");
    Assert::IsFalse(display.reducedFlash || display.reducedMotion, L"R-META-511: both accessibility settings off");
    Assert::IsFalse(display.uiScale.has_value() || display.textScale.has_value(), L"the scales take the record's defaults");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Plt080TheStartWritesTheDisplaySettingsBack)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"PLT-080")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"RND-021")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Plt080TheStartWritesTheDisplaySettingsBack)
  {
    Neuron::SettingsStore store(Species::ClientSettingSpecs(), std::nullopt);
    Species::WriteBackDisplay(store, true, {1600, 900});
    Assert::IsTrue(Species::RenderSettingsOf(store).windowed, L"the mode it started in");
    Assert::IsTrue((Species::RenderSettingsOf(store).renderSize == Neuron::SizePx{1600, 900}), L"and its render size");

    // Each option the renderer reads, set as the options page will set it.
    Assert::IsTrue(store.Set(Species::SETTING_ANTI_ALIASING, std::string(Species::ANTI_ALIASING_OFF)), L"anti-aliasing off");
    Assert::IsTrue(store.Set(Species::SETTING_FRAME_RATE_CAP, std::int64_t{90}), L"a cap of 90");
    Assert::IsTrue(store.Set(Species::SETTING_PRESENT_MODE, std::string(Species::PRESENT_MODE_UNCAPPED)), L"uncapped");
    Assert::IsTrue(store.Set(Species::SETTING_REDUCED_MOTION, true), L"reduced motion");
    Assert::IsFalse(store.Set(Species::SETTING_ENTITY_DETAIL, std::int64_t{4}), L"no entity detail above 3");
    const Species::RenderSettings render = Species::RenderSettingsOf(store);
    ExpectEqual(1, render.sceneSamplesPreferred, L"one scene sample");
    Assert::IsTrue(render.frameRateCap == 90.0f, L"the cap");
    Assert::IsTrue(render.uncapped, L"the present mode");
    Assert::IsTrue(Species::DisplayOptionsOf(store).reducedMotion, L"the presentation's option");
  }
};

} // namespace GameClientTests
