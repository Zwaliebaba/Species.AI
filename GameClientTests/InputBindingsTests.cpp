#include "pch.h"

#include "BindingNotation.h"
#include "ClientSettings.h"
#include "InputBindings.h"
#include "TestSupport.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace GameClientTests
{

namespace
{

class SilentBackEnd final : public Neuron::DeviceBackEnd
{
public:
  void Collect(std::vector<Neuron::InputEvent>& /*_queue*/) override {}
  void ReleaseAll(std::vector<Neuron::InputEvent>& /*_queue*/) override {}
};

} // namespace

TEST_CLASS(InputBindingsTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Inp165TheLayoutLayersMoveTheCameraKeysOnly)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-164")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-165")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp165TheLayoutLayersMoveTheCameraKeysOnly)
  {
    SilentBackEnd keys;
    const std::vector<Neuron::ActionBindings> defaults = Species::DefaultBindings();
    Neuron::InputRouter router(defaults, keys, nullptr);
    const std::vector<Neuron::BindingReplacement> french = Species::LayoutLayer("fr-FR");
    ExpectEqual(7, static_cast<std::int64_t>(french.size()), L"table 4.13-B's seven rows");
    ExpectEqual(7, static_cast<std::int64_t>(router.Rebuild(french)), L"every row binds");
    const auto pan = router.BindingsOf(Species::ActionOf(Species::GameAction::CameraPanForward));
    Assert::IsTrue(pan.size() == 2 && pan[0] == Neuron::KeyBinding(Neuron::KeyCode::Z, Neuron::Condition::Held) &&
                     pan[1] == Neuron::KeyBinding(Neuron::KeyCode::Up, Neuron::Condition::Held),
                   L"Z forward on a French keyboard, the arrow kept");
    Assert::IsTrue(Species::LayoutLayer("en-US").empty() && Species::LayoutLayer("en-GB").empty(), L"no layer for the English layouts");
    Assert::IsTrue(Species::LayoutLayer("de-DE")[6].binding == Neuron::KeyBinding(Neuron::KeyCode::Y, Neuron::Condition::Held),
                   L"the German zoom on Y");
    // O108 (ADR-180): the turns take the reference positions of Q and E; the zoom keys, Page Up and Page Down, stay.
    Assert::IsTrue(french[4].action == "action.camera.turn_left" &&
                     french[4].binding == Neuron::KeyBinding(Neuron::KeyCode::A, Neuron::Condition::Held),
                   L"the French turn to the left on A");
    const auto raise = router.BindingsOf(Species::ActionOf(Species::GameAction::CameraRaise));
    Assert::IsTrue(raise.size() == 1 && raise[0] == Neuron::KeyBinding(Neuron::KeyCode::PageUp, Neuron::Condition::Held),
                   L"the zoom out on Page Up with a French layout");

    Neuron::SettingsStore settings(Species::ClientSettingSpecs(), std::nullopt);
    Assert::IsTrue(Species::ActiveLayoutTag(settings, "de-DE") == "de-DE", L"the system's layout while none is set");
    Assert::IsTrue(settings.Set(Species::SETTING_KEYBOARD_LAYOUT, std::string("fr-FR")), L"a layout set");
    Assert::IsTrue(Species::ActiveLayoutTag(settings, "de-DE") == "fr-FR", L"the setting wins (R-META-455)");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp070TheUserLayerIsTheBindingSettings)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-070")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-171")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-162")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp070TheUserLayerIsTheBindingSettings)
  {
    Neuron::SettingsStore settings(Species::ClientSettingSpecs(), std::nullopt);
    Assert::IsTrue(settings.Set("binding.action.select.clear", std::string(R"({"key": "x", "when": "down"})")), L"a user binding");
    Assert::IsTrue(settings.Set("binding.action.camera.zoom", std::string(R"({"key": "x", "when": "sometimes"})")), L"one that breaks");
    Assert::IsTrue(settings.Set("binding.action.camera.aim", std::string(R"({"key": "x", "when": "held"})")), L"one the action refuses");
    Assert::IsTrue(settings.Set("binding.action.camera.pan_left", std::string(R"({"key": "j", "when": "held"})")),
                   L"one over the layout layer");
    const std::vector<Neuron::BindingReplacement> user = Species::UserLayer(settings);
    ExpectEqual(3, static_cast<std::int64_t>(user.size()), L"the entry that does not parse is skipped");

    SilentBackEnd keys;
    const std::vector<Neuron::ActionBindings> defaults = Species::DefaultBindings();
    Neuron::InputRouter router(defaults, keys, nullptr, Species::SettingAccessOf(settings));
    ExpectEqual(7 + 2, static_cast<std::int64_t>(router.Rebuild(Species::BindingLayers(settings, "fr-FR"))),
                L"the layout's seven, then the user's two the router accepts");
    Assert::IsTrue(router.BindingsOf(Species::ActionOf(Species::GameAction::SelectClear))[0] ==
                     Neuron::KeyBinding(Neuron::KeyCode::X, Neuron::Condition::Down),
                   L"the user's binding replaces the primary");
    Assert::IsTrue(router.BindingsOf(Species::ActionOf(Species::GameAction::CameraPanLeft))[0] ==
                     Neuron::KeyBinding(Neuron::KeyCode::J, Neuron::Condition::Held),
                   L"the user layer comes after the layout layer");
    // INP-072: what the user layer stores is the canonical form a binding writes.
    ExpectText(R"({"key": "x", "when": "down"})",
               Neuron::WriteBinding(router.BindingsOf(Species::ActionOf(Species::GameAction::SelectClear))[0]), L"the canonical form");
  }
};

} // namespace GameClientTests
