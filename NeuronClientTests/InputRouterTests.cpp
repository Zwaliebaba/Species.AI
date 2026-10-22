#include "pch.h"

#include "InputRouter.h"
#include "TestSupport.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace NeuronClientTests
{

namespace
{

// A back-end whose events the test queues, holding what it has pressed as the window's back-end does.
class ScriptedBackEnd final : public Neuron::DeviceBackEnd
{
public:
  void Key(Neuron::KeyCode _key, bool _press, bool _repeat = false)
  {
    Neuron::InputEvent event;
    event.kind = _press ? Neuron::InputEvent::Kind::Press : Neuron::InputEvent::Kind::Release;
    event.control = static_cast<std::uint16_t>(_key);
    event.osRepeat = _repeat;
    Queue(event);
  }
  void Button(Neuron::MouseControl _button, bool _press)
  {
    Neuron::InputEvent event;
    event.kind = _press ? Neuron::InputEvent::Kind::Press : Neuron::InputEvent::Kind::Release;
    event.device = Neuron::InputDevice::Mouse;
    event.control = static_cast<std::uint16_t>(_button);
    Queue(event);
  }
  void Move(std::int32_t _x, std::int32_t _y)
  {
    Neuron::InputEvent event;
    event.kind = Neuron::InputEvent::Kind::Axis;
    event.device = Neuron::InputDevice::Mouse;
    event.control = static_cast<std::uint16_t>(Neuron::MouseControl::Motion);
    event.x = static_cast<float>(_x - m_pointer.x);
    event.y = static_cast<float>(_y - m_pointer.y);
    event.pointer = {_x, _y};
    m_pointer = event.pointer;
    Queue(event);
  }
  void Wheel(float _notches)
  {
    Neuron::InputEvent event;
    event.kind = Neuron::InputEvent::Kind::Axis;
    event.device = Neuron::InputDevice::Mouse;
    event.control = static_cast<std::uint16_t>(Neuron::MouseControl::Wheel);
    event.x = _notches;
    Queue(event);
  }

  void Collect(std::vector<Neuron::InputEvent>& _queue) override
  {
    _queue.insert(_queue.end(), m_pending.begin(), m_pending.end());
    m_pending.clear();
  }
  void ReleaseAll(std::vector<Neuron::InputEvent>& _queue) override
  {
    for (const std::uint16_t key : m_heldKeys)
    {
      Neuron::InputEvent event;
      event.kind = Neuron::InputEvent::Kind::Release;
      event.control = key;
      _queue.push_back(event);
    }
    m_heldKeys.clear();
  }

private:
  void Queue(const Neuron::InputEvent& _event)
  {
    if (_event.device == Neuron::InputDevice::Key && !_event.osRepeat)
    {
      std::erase(m_heldKeys, _event.control);
      if (_event.kind == Neuron::InputEvent::Kind::Press)
      {
        m_heldKeys.push_back(_event.control);
      }
    }
    m_pending.push_back(_event);
  }

  std::vector<Neuron::InputEvent> m_pending;
  std::vector<std::uint16_t> m_heldKeys;
  Neuron::PointPx m_pointer;
};

class RecordingPointer final : public Neuron::PointerHost
{
public:
  void WarpPointer(Neuron::PointPx _clientPosition) override
  {
    warps.push_back(_clientPosition);
  }
  void ConfinePointer(bool _on) override
  {
    confined = _on;
  }

  std::vector<Neuron::PointPx> warps;
  bool confined = false;
};

constexpr Neuron::ActionId FORWARD{0};
constexpr Neuron::ActionId FORWARD_DOWN{1};
constexpr Neuron::ActionId FORWARD_UP{2};
constexpr Neuron::ActionId REMOVE{3};
constexpr Neuron::ActionId MODE{4};
constexpr Neuron::ActionId AIM{5};
constexpr Neuron::ActionId HEIGHT{6};
constexpr Neuron::ActionId PICK{7};

std::vector<Neuron::ActionBindings> Actions()
{
  using enum Neuron::Condition;
  return {
    {"action.camera.pan_forward",
     Neuron::ValueKind::Boolean,
     {Neuron::KeyBinding(Neuron::KeyCode::W, Held), Neuron::KeyBinding(Neuron::KeyCode::Up, Held)}},
    {"forward down", Neuron::ValueKind::Boolean, {Neuron::KeyBinding(Neuron::KeyCode::W, Down)}},
    {"forward up", Neuron::ValueKind::Boolean, {Neuron::KeyBinding(Neuron::KeyCode::W, Up)}},
    {"remove", Neuron::ValueKind::Boolean, {Neuron::ChordOf({Neuron::KeyCode::Control, Neuron::KeyCode::Shift, Neuron::KeyCode::F11})}},
    {"mode",
     Neuron::ValueKind::Boolean,
     {Neuron::AllOf({Neuron::KeyBinding(Neuron::KeyCode::Alt, Held), Neuron::KeyBinding(Neuron::KeyCode::Enter, Down)})}},
    {"action.camera.aim", Neuron::ValueKind::Axis2, {Neuron::MouseBinding(Neuron::MouseControl::Motion, Move)}},
    {"action.camera.height_wheel", Neuron::ValueKind::Axis1, {Neuron::MouseBinding(Neuron::MouseControl::Wheel, Move)}},
    {"action.select.pick", Neuron::ValueKind::Boolean, {Neuron::MouseBinding(Neuron::MouseControl::Primary, Down)}},
  };
}

constexpr Neuron::SizePx CLIENT{800, 600};

} // namespace

TEST_CLASS(InputRouterTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Inp052APressAndAReleaseInOneFrameAreBothSeen)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-052")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-020")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp052APressAndAReleaseInOneFrameAreBothSeen)
  {
    // ADR-028's input test: a press and a release within one frame make down and up true and held false.
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions = Actions();
    Neuron::InputRouter router(actions, keys, nullptr);
    keys.Key(Neuron::KeyCode::W, true);
    keys.Key(Neuron::KeyCode::W, false);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(FORWARD_DOWN) && router.Query(FORWARD_UP), L"both edges in the frame");
    Assert::IsFalse(router.Query(FORWARD), L"not held after it");
    // The first satisfied binding answers: the arrow key holds the same action.
    keys.Key(Neuron::KeyCode::Up, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(FORWARD) && !router.Query(FORWARD_DOWN), L"held by its second binding");
    router.Suppress(FORWARD);
    Assert::IsFalse(router.Query(FORWARD), L"suppressed for the rest of the frame");
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(FORWARD), L"and back in the next");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp133ARepeatFiresOnThePressThenAfterTheDelayAndEveryPeriod)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-133")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp133ARepeatFiresOnThePressThenAfterTheDelayAndEveryPeriod)
  {
    using Ms = std::chrono::milliseconds;
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions{
      {"action.menu.down", Neuron::ValueKind::Boolean, {Neuron::KeyBinding(Neuron::KeyCode::Down, Neuron::Condition::Repeat)}}};
    const Neuron::ActionId down{0};
    Neuron::InputRouter router(actions, keys, nullptr);
    router.SetRepeatTiming({Ms(300), Ms(300)});
    Neuron::ActionValue value;
    keys.Key(Neuron::KeyCode::Down, true);
    router.AdvanceFrame({}, CLIENT, Ms(1000));
    Assert::IsTrue(router.Query(down, &value) && value.x == 1.0f, L"once on the press");
    router.AdvanceFrame({}, CLIENT, Ms(1200));
    Assert::IsFalse(router.Query(down), L"not before the initial delay");
    router.AdvanceFrame({}, CLIENT, Ms(1300));
    Assert::IsTrue(router.Query(down, &value) && value.x == 1.0f, L"at the press + the initial delay");
    router.AdvanceFrame({}, CLIENT, Ms(1950));
    Assert::IsTrue(router.Query(down, &value) && value.x == 2.0f, L"INP-134: the firings of a long frame are its value");
    keys.Key(Neuron::KeyCode::Down, false);
    router.AdvanceFrame({}, CLIENT, Ms(2500));
    Assert::IsFalse(router.Query(down), L"the timer clears on the release");
    keys.Key(Neuron::KeyCode::Down, true);
    keys.Key(Neuron::KeyCode::Down, true, true); // the system's own auto-repeat presses nothing (INP-053)
    router.AdvanceFrame({}, CLIENT, Ms(2600));
    Assert::IsTrue(router.Query(down, &value) && value.x == 1.0f, L"a new press fires once");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp053AnAutoRepeatNeverPresses)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-053")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp053AnAutoRepeatNeverPresses)
  {
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions = Actions();
    Neuron::InputRouter router(actions, keys, nullptr);
    keys.Key(Neuron::KeyCode::W, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(FORWARD_DOWN));
    keys.Key(Neuron::KeyCode::W, true, true);
    keys.Key(Neuron::KeyCode::W, true, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(FORWARD) && !router.Query(FORWARD_DOWN), L"held, and no new press");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp111AFocusLossReleasesEverythingHeld)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-111")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp111AFocusLossReleasesEverythingHeld)
  {
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions = Actions();
    Neuron::InputRouter router(actions, keys, nullptr);
    keys.Key(Neuron::KeyCode::W, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(FORWARD));
    // Alt+Tab: the window loses focus with W held; the release arrives in that frame.
    router.AdvanceFrame({.windowFocused = false}, CLIENT);
    Assert::IsTrue(!router.Query(FORWARD) && router.Query(FORWARD_UP), L"released by the focus loss");
    // Without focus nothing is pressed; with it back, a key held through the loss presses only when pressed again.
    keys.Key(Neuron::KeyCode::W, true);
    router.AdvanceFrame({.windowFocused = false}, CLIENT);
    Assert::IsFalse(router.Query(FORWARD), L"no press without focus");
    router.AdvanceFrame({}, CLIENT);
    Assert::IsFalse(router.Query(FORWARD), L"still released when focus returns");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp137AChordFiresOnceWhateverItsOrder)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-137")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-060")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp137AChordFiresOnceWhateverItsOrder)
  {
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions = Actions();
    Neuron::InputRouter router(actions, keys, nullptr);
    keys.Key(Neuron::KeyCode::F11, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsFalse(router.Query(REMOVE), L"one part");
    keys.Key(Neuron::KeyCode::Shift, true);
    keys.Key(Neuron::KeyCode::Control, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(REMOVE), L"the last parts in any order");
    router.AdvanceFrame({}, CLIENT);
    Assert::IsFalse(router.Query(REMOVE), L"not again while all stay held");
    keys.Key(Neuron::KeyCode::F11, false);
    keys.Key(Neuron::KeyCode::F11, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(REMOVE), L"again when a part goes down again");
    // A conjunction: Alt held and Enter down.
    keys.Key(Neuron::KeyCode::Enter, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsFalse(router.Query(MODE), L"Enter without Alt");
    keys.Key(Neuron::KeyCode::Enter, false);
    keys.Key(Neuron::KeyCode::Alt, true);
    keys.Key(Neuron::KeyCode::Enter, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(MODE), L"Alt held and Enter down");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp156ThePointerFollowsTheMouseAndAWarpMovesNothing)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-156")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-158")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-121")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp156ThePointerFollowsTheMouseAndAWarpMovesNothing)
  {
    ScriptedBackEnd mouse;
    RecordingPointer pointer;
    const std::vector<Neuron::ActionBindings> actions = Actions();
    Neuron::InputRouter router(actions, mouse, &pointer);
    mouse.Move(100, 50);
    mouse.Move(110, 45);
    mouse.Wheel(0.5f);
    mouse.Wheel(1.0f);
    router.AdvanceFrame({}, CLIENT);
    Neuron::ActionValue aim;
    Assert::IsTrue(router.Query(AIM, &aim) && aim.kind == Neuron::ValueKind::Axis2, L"the movement moves");
    Assert::AreEqual(110.0f, aim.x, L"summed from the start");
    Assert::AreEqual(45.0f, aim.y);
    Neuron::ActionValue wheel;
    Assert::IsTrue(router.Query(HEIGHT, &wheel) && wheel.x == 1.5f, L"fractional notches summed");
    Assert::IsTrue(router.Pointer().positionPixels.x == 110 && router.Pointer().positionPixels.y == 45 && router.Pointer().moved);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(!router.Query(AIM) && !router.Query(HEIGHT) && !router.Pointer().moved, L"still: no movement");
    // INP-158: a warp sets the position without a delta, and the system pointer goes there.
    router.WarpPointer({900, 300});
    Assert::IsTrue(router.Pointer().positionPixels.x == 799 && pointer.warps.size() == 1 && pointer.warps[0].x == 799,
                   L"clamped to the client area");
    router.ConfinePointer(true);
    Assert::IsTrue(pointer.confined);
    // A button's down edge.
    mouse.Button(Neuron::MouseControl::Primary, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(PICK), L"the primary button's down");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp060ABindingThatBreaksTheNodeRulesIsLeftOut)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-060")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp060ABindingThatBreaksTheNodeRulesIsLeftOut)
  {
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions{
      {"broken",
       Neuron::ValueKind::Boolean,
       {Neuron::KeyBinding(Neuron::KeyCode::W, Neuron::Condition::Move), Neuron::KeyBinding(Neuron::KeyCode::W, Neuron::Condition::Held)}},
    };
    Neuron::InputRouter router(actions, keys, nullptr);
    keys.Key(Neuron::KeyCode::W, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query({0}), L"the valid binding stays");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp162AReplacingLayerReplacesThePrimaryBindingOnly)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-162")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-065")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-171")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp162AReplacingLayerReplacesThePrimaryBindingOnly)
  {
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions = Actions();
    Neuron::InputRouter router(actions, keys, nullptr);
    const std::vector<Neuron::BindingReplacement> layers{
      {"action.camera.pan_forward", Neuron::KeyBinding(Neuron::KeyCode::Z, Neuron::Condition::Held), "the layout layer fr-FR"},
      {"action.camera.aim", Neuron::KeyBinding(Neuron::KeyCode::A, Neuron::Condition::Held), "binding.action.camera.aim"},
      {"action.nothing", Neuron::KeyBinding(Neuron::KeyCode::A, Neuron::Condition::Held), "binding.action.nothing"},
      {"action.select.pick", Neuron::KeyBinding(Neuron::KeyCode::P, Neuron::Condition::Down), "binding.action.select.pick"},
    };
    ExpectEqual(2, static_cast<std::int64_t>(router.Rebuild(layers)),
                L"INP-171: an analogue action's Boolean and an unknown action are skipped");
    keys.Key(Neuron::KeyCode::Z, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(FORWARD), L"the new primary binding");
    keys.Key(Neuron::KeyCode::Z, false);
    keys.Key(Neuron::KeyCode::W, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsFalse(router.Query(FORWARD), L"the old primary is gone");
    keys.Key(Neuron::KeyCode::W, false);
    keys.Key(Neuron::KeyCode::Up, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(FORWARD), L"INP-165: the arrow key stays");
    Assert::IsTrue(router.BindingsOf(AIM).size() == 1 && router.BindingsOf(AIM)[0].kind == Neuron::InputBinding::Kind::Leaf &&
                     router.BindingsOf(AIM)[0].leaf.device == Neuron::InputDevice::Mouse,
                   L"the aim keeps its motion");
    ExpectEqual(0, static_cast<std::int64_t>(router.Rebuild({})), L"rebuilt without layers");
    Assert::IsTrue(router.BindingsOf(PICK)[0] == Neuron::MouseBinding(Neuron::MouseControl::Primary, Neuron::Condition::Down),
                   L"the defaults again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp139AnAliasIsItsTargetsQuery)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-139")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-138")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-143")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp139AnAliasIsItsTargetsQuery)
  {
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions = Actions();
    Neuron::InputRouter router(actions, keys, nullptr);
    const std::vector<Neuron::BindingReplacement> layers{
      {"forward down", Neuron::AliasOf("action.select.pick"), "test"},
      {"forward up", Neuron::NotOf(Neuron::AliasOf("action.camera.pan_forward")), "test"},
      {"mode", Neuron::AlwaysTrue(), "test"},
    };
    ExpectEqual(3, static_cast<std::int64_t>(router.Rebuild(layers)), L"every replacement applies");
    keys.Button(Neuron::MouseControl::Primary, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(FORWARD_DOWN), L"the alias fires with its target");
    Assert::IsTrue(router.Query(FORWARD_UP), L"not: W is not held");
    Assert::IsTrue(router.Query(MODE), L"value:always_true");
    router.Suppress(PICK);
    Assert::IsFalse(router.Query(FORWARD_DOWN), L"the target's suppression included");

    // DEC-554: a chain back to its own action is refused.
    const std::vector<Neuron::BindingReplacement> cycle{
      {"action.select.pick", Neuron::AliasOf("forward down"), "binding.action.select.pick"},
    };
    ExpectEqual(1, static_cast<std::int64_t>(router.Rebuild(std::vector<Neuron::BindingReplacement>{layers[0], cycle[0]})),
                L"the second link, which closes the cycle, is skipped");
    Assert::IsTrue(router.CheckBinding(FORWARD_DOWN, Neuron::AliasOf("forward down")).has_value(), L"an alias of itself");
    Assert::IsTrue(router.CheckBinding(PICK, Neuron::AliasOf("action.missing")).has_value(), L"an alias of no action");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp140ALatchTogglesOncePerFrame)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-140")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-066")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp140ALatchTogglesOncePerFrame)
  {
    // ADR-064's hold-or-toggle: W's press turns the latch on and S's press turns it off.
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions = Actions();
    Neuron::InputRouter router(actions, keys, nullptr);
    const std::vector<Neuron::BindingReplacement> layers{
      {"mode",
       Neuron::LatchOf(Neuron::KeyBinding(Neuron::KeyCode::W, Neuron::Condition::Down),
                       Neuron::KeyBinding(Neuron::KeyCode::S, Neuron::Condition::Down)),
       "test"},
    };
    ExpectEqual(1, static_cast<std::int64_t>(router.Rebuild(layers)), L"the latch applies");
    router.AdvanceFrame({}, CLIENT);
    Assert::IsFalse(router.Query(MODE), L"it starts false");
    keys.Key(Neuron::KeyCode::W, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(MODE), L"on");
    keys.Key(Neuron::KeyCode::W, false);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(MODE), L"and held on after the release");
    keys.Key(Neuron::KeyCode::S, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsFalse(router.Query(MODE), L"off");
    keys.Key(Neuron::KeyCode::S, false);
    keys.Key(Neuron::KeyCode::W, true);
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(MODE), L"on again");
    static_cast<void>(router.Rebuild(layers));
    Assert::IsFalse(router.Query(MODE), L"Rebuild resets it");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp153TheIdleTimeGrowsWhileNoDeviceIsActive)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-153")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-141")
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-150")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp153TheIdleTimeGrowsWhileNoDeviceIsActive)
  {
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions = Actions();
    Neuron::InputRouter router(actions, keys, nullptr);
    const std::vector<Neuron::BindingReplacement> layers{
      {"forward down", Neuron::IdleOf(Neuron::IdleTest::Reaches, 2), "test"},
      {"forward up", Neuron::IdleOf(Neuron::IdleTest::NoLonger), "test"},
      {"action.camera.height_wheel", Neuron::IdleOf(Neuron::IdleTest::AtLeast, 1), "test"},
    };
    ExpectEqual(3, static_cast<std::int64_t>(router.Rebuild(layers)), L"the idle nodes apply");
    keys.Key(Neuron::KeyCode::W, true);
    router.AdvanceFrame({}, CLIENT, std::chrono::seconds(10));
    Assert::IsFalse(router.IsIdle(), L"a key held is active");
    keys.Key(Neuron::KeyCode::W, false);
    router.AdvanceFrame({}, CLIENT, std::chrono::milliseconds(10'500));
    Assert::IsFalse(router.IsIdle(), L"its release is an event");
    router.AdvanceFrame({}, CLIENT, std::chrono::seconds(11));
    Assert::IsTrue(router.IsIdle() && router.IdleSeconds() == 0.0, L"idle, from 0");
    Assert::IsTrue(router.Query(FORWARD_UP), L"no_longer while the idle time is 0");
    router.AdvanceFrame({}, CLIENT, std::chrono::milliseconds(12'500));
    Neuron::ActionValue value;
    Assert::IsTrue(router.Query(HEIGHT, &value) && value.x == 1.5f, L"at_least 1, with the idle time as its value");
    Assert::IsFalse(router.Query(FORWARD_DOWN), L"not yet 2 s");
    router.AdvanceFrame({}, CLIENT, std::chrono::seconds(13));
    Assert::IsTrue(router.Query(FORWARD_DOWN), L"reaches 2 in the frame it passes 2 s");
    router.AdvanceFrame({}, CLIENT, std::chrono::seconds(14));
    Assert::IsFalse(router.Query(FORWARD_DOWN), L"and only then");
    Assert::IsTrue(router.Mode() == Neuron::InputMode::KeyboardMouse, L"INP-150: the keyboard and mouse's mode");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Inp142ASettingNodeReadsItsSettingOnceAFrame)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"INP-142")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Inp142ASettingNodeReadsItsSettingOnceAFrame)
  {
    ScriptedBackEnd keys;
    const std::vector<Neuron::ActionBindings> actions = Actions();
    double tips = 0.0;
    Neuron::SettingAccess access;
    access.exists = [](std::string_view _key) { return _key == "interface.tips"; };
    access.number = [&tips](std::string_view) -> std::optional<double> { return tips; };
    Neuron::InputRouter router(actions, keys, nullptr, access);
    const std::vector<Neuron::BindingReplacement> layers{
      {"forward down", Neuron::SettingOf("interface.tips"), "test"},
      {"forward up", Neuron::SettingOf("interface.tips", true), "test"},
      {"mode", Neuron::SettingOf("interface.missing"), "test"},
    };
    ExpectEqual(2, static_cast<std::int64_t>(router.Rebuild(layers)), L"a setting the store does not hold is refused");
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(!router.Query(FORWARD_DOWN) && router.Query(FORWARD_UP), L"off, and its negation");
    tips = 1.0;
    Assert::IsFalse(router.Query(FORWARD_DOWN), L"read once a frame, in AdvanceFrame");
    router.AdvanceFrame({}, CLIENT);
    Assert::IsTrue(router.Query(FORWARD_DOWN) && !router.Query(FORWARD_UP), L"on");
  }
};

} // namespace NeuronClientTests
