#include "pch.h"

#include "TouchGestures.h"

#include <chrono>
#include <cmath>
#include <cstdint>
#include <numbers>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace std::chrono_literals;

namespace NeuronClientTests
{

namespace
{

constexpr double DEGREES = std::numbers::pi / 180.0;

// DEC-1011's first values at a DPI scale of 1: a slop of 10 px, a double tap's slop of 24 px, a tap of 250 ms, a hold of
// 350 ms, a double tap within 300 ms and a turn threshold of 10°.
Neuron::TouchThresholds Thresholds()
{
  return {10.0, 24.0, 250ms, 350ms, 300ms, 10.0 * DEGREES};
}

// A recognizer that a test feeds contacts at whole milliseconds and advances frame by frame.
class Fingers
{
public:
  Fingers()
  {
    m_gestures.SetThresholds(Thresholds());
  }
  void Down(std::uint16_t _slot, double _x, double _y, std::chrono::milliseconds _time)
  {
    Event(Neuron::InputEvent::Kind::Press, _slot, _x, _y, _time);
  }
  void Move(std::uint16_t _slot, double _x, double _y, std::chrono::milliseconds _time)
  {
    Event(Neuron::InputEvent::Kind::Axis, _slot, _x, _y, _time);
  }
  void Up(std::uint16_t _slot, double _x, double _y, std::chrono::milliseconds _time)
  {
    Event(Neuron::InputEvent::Kind::Release, _slot, _x, _y, _time);
  }
  void Cancel(std::uint16_t _slot, std::chrono::milliseconds _time)
  {
    Event(Neuron::InputEvent::Kind::Cancel, _slot, 0.0, 0.0, _time);
  }
  const Neuron::TouchFrame& Frame(std::chrono::milliseconds _now)
  {
    return m_gestures.Advance(_now);
  }

private:
  void Event(Neuron::InputEvent::Kind _kind, std::uint16_t _slot, double _x, double _y, std::chrono::milliseconds _time)
  {
    Neuron::InputEvent event;
    event.kind = _kind;
    event.device = Neuron::InputDevice::Touch;
    event.control = _slot;
    event.time = _time;
    event.pointer = {static_cast<std::int32_t>(_x), static_cast<std::int32_t>(_y)};
    m_gestures.Contact(event);
  }

  Neuron::TouchGestures m_gestures;
};

void ExpectPoint(double _x, double _y, const Neuron::TouchPoint& _point, const wchar_t* _what)
{
  Assert::AreEqual(_x, _point.x, 1e-9, _what);
  Assert::AreEqual(_y, _point.y, 1e-9, _what);
}

// The frame's drag and manipulation, each asserted present.
Neuron::TouchDrag DragOf(const Neuron::TouchFrame& _frame)
{
  Assert::IsTrue(_frame.drag.has_value(), L"a drag in the frame");
  return _frame.drag.value_or(Neuron::TouchDrag{});
}

Neuron::TouchManipulation ManipulationOf(const Neuron::TouchFrame& _frame)
{
  Assert::IsTrue(_frame.manipulation.has_value(), L"a manipulation in the frame");
  return _frame.manipulation.value_or(Neuron::TouchManipulation{});
}

} // namespace

TEST_CLASS(TouchGesturesTests)
{
public:
  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181ATapIsAQuickStillTouch)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181ATapIsAQuickStillTouch)
  {
    Fingers fingers;
    fingers.Down(0, 100, 100, 0ms);
    const Neuron::TouchFrame& down = fingers.Frame(16ms);
    Assert::IsTrue(down.taps.empty() && down.active, L"down: nothing yet, but active");
    fingers.Move(0, 104, 103, 40ms); // within the slop
    fingers.Up(0, 104, 103, 100ms);
    const Neuron::TouchFrame& tapped = fingers.Frame(116ms);
    Assert::IsTrue(tapped.taps.size() == 1 && !tapped.drag.has_value() && tapped.doubleTaps.empty(), L"one tap");
    ExpectPoint(100, 100, tapped.taps[0], L"where it went down");
    Assert::IsFalse(fingers.Frame(132ms).active, L"nothing down, nothing new");
    // Lifted after the tap time and before the hold time: neither a tap nor a hold.
    fingers.Down(0, 300, 300, 1'000ms);
    fingers.Up(0, 300, 300, 1'300ms);
    const Neuron::TouchFrame& slow = fingers.Frame(1'316ms);
    Assert::IsTrue(slow.taps.empty() && !slow.drag.has_value(), L"a slow press is nothing");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181ASecondTapCloseAndSoonIsADoubleTap)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181ASecondTapCloseAndSoonIsADoubleTap)
  {
    Fingers fingers;
    fingers.Down(0, 100, 100, 0ms);
    fingers.Up(0, 100, 100, 80ms);
    Assert::IsTrue(fingers.Frame(96ms).doubleTaps.empty(), L"the first tap alone");
    fingers.Down(1, 110, 105, 200ms);
    fingers.Up(1, 110, 105, 260ms);
    const Neuron::TouchFrame& second = fingers.Frame(276ms);
    Assert::IsTrue(second.taps.size() == 1 && second.doubleTaps.size() == 1, L"the second tap completes a double tap");
    ExpectPoint(110, 105, second.doubleTaps[0], L"at the second tap");
    fingers.Down(0, 110, 105, 400ms);
    fingers.Up(0, 110, 105, 450ms);
    Assert::IsTrue(fingers.Frame(466ms).doubleTaps.empty(), L"a third tap begins again");
    // Too late, then too far.
    fingers.Down(0, 500, 500, 1'000ms);
    fingers.Up(0, 500, 500, 1'050ms);
    static_cast<void>(fingers.Frame(1'066ms));
    fingers.Down(0, 500, 500, 1'300ms);
    fingers.Up(0, 500, 500, 1'360ms);
    Assert::IsTrue(fingers.Frame(1'376ms).doubleTaps.empty(), L"more than 300 ms after the first");
    fingers.Down(0, 530, 500, 1'400ms);
    fingers.Up(0, 530, 500, 1'450ms);
    Assert::IsTrue(fingers.Frame(1'466ms).doubleTaps.empty(), L"more than 24 px from the first");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181AContactPastTheSlopIsADragInEveryFrame)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181AContactPastTheSlopIsADragInEveryFrame)
  {
    Fingers fingers;
    fingers.Down(0, 100, 100, 0ms);
    fingers.Move(0, 108, 100, 10ms);
    Assert::IsFalse(fingers.Frame(16ms).drag.has_value(), L"within the slop");
    fingers.Move(0, 115, 100, 20ms);
    const Neuron::TouchDrag begun = DragOf(fingers.Frame(32ms));
    Assert::IsTrue(begun.phase == Neuron::GesturePhase::Begin && !begun.held, L"begun");
    ExpectPoint(100, 100, begun.start, L"from where it went down");
    ExpectPoint(115, 100, begun.point, L"to where it is");
    Assert::IsTrue(DragOf(fingers.Frame(48ms)).phase == Neuron::GesturePhase::Move, L"reported while still");
    fingers.Up(0, 130, 100, 60ms);
    const Neuron::TouchFrame& lifted = fingers.Frame(64ms);
    Assert::IsTrue(lifted.taps.empty(), L"no tap");
    const Neuron::TouchDrag ended = DragOf(lifted);
    Assert::IsTrue(ended.phase == Neuron::GesturePhase::End, L"ended");
    ExpectPoint(130, 100, ended.point, L"where it lifted");
    Assert::IsFalse(fingers.Frame(80ms).drag.has_value(), L"and gone");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181AStillContactBecomesAHoldsDrag)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181AStillContactBecomesAHoldsDrag)
  {
    Fingers fingers;
    fingers.Down(0, 100, 100, 0ms);
    Assert::IsFalse(fingers.Frame(200ms).drag.has_value(), L"before the hold time");
    const Neuron::TouchDrag held = DragOf(fingers.Frame(360ms));
    Assert::IsTrue(held.phase == Neuron::GesturePhase::Begin && held.held, L"a hold");
    fingers.Move(0, 140, 100, 380ms);
    const Neuron::TouchDrag moved = DragOf(fingers.Frame(392ms));
    Assert::IsTrue(moved.phase == Neuron::GesturePhase::Move && moved.held, L"its drag");
    fingers.Up(0, 140, 100, 400ms);
    const Neuron::TouchFrame& lifted = fingers.Frame(408ms);
    Assert::IsTrue(DragOf(lifted).phase == Neuron::GesturePhase::End && lifted.taps.empty(), L"ended, no tap");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181TwoContactsPinchAndTurnPastTheThreshold)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181TwoContactsPinchAndTurnPastTheThreshold)
  {
    Fingers fingers;
    fingers.Down(0, 100, 100, 0ms);
    fingers.Down(1, 200, 100, 10ms);
    Assert::IsFalse(fingers.Frame(16ms).manipulation.has_value(), L"still a possible two-finger tap");
    fingers.Move(1, 230, 100, 20ms);
    const Neuron::TouchManipulation begun = ManipulationOf(fingers.Frame(32ms));
    Assert::IsTrue(begun.phase == Neuron::GesturePhase::Begin, L"begun past the slop");
    ExpectPoint(165, 100, begun.center, L"their center");
    Assert::AreEqual(1.0, begun.scale, 1e-12, L"from where it began");
    fingers.Move(1, 100 + 160 * std::cos(5.0 * DEGREES), 100 + 160 * std::sin(5.0 * DEGREES), 40ms);
    const Neuron::TouchManipulation spread = ManipulationOf(fingers.Frame(48ms));
    Assert::IsTrue(spread.phase == Neuron::GesturePhase::Move, L"moving");
    Assert::IsTrue(spread.scale > 1.1, L"spread: the spans' ratio above 1");
    Assert::AreEqual(0.0, spread.turnRadians, 1e-12, L"5°: below the threshold");
    // Past 10° in all, the frame that crosses reports nothing, and the next reports its own turn, clockwise positive.
    fingers.Move(1, 100 + 160 * std::cos(12.0 * DEGREES), 100 + 160 * std::sin(12.0 * DEGREES), 60ms);
    Assert::AreEqual(0.0, ManipulationOf(fingers.Frame(64ms)).turnRadians, 1e-12, L"the threshold crossed");
    fingers.Move(1, 100 + 160 * std::cos(20.0 * DEGREES), 100 + 160 * std::sin(20.0 * DEGREES), 70ms);
    const Neuron::TouchManipulation turned = ManipulationOf(fingers.Frame(80ms));
    Assert::AreEqual(8.0 * DEGREES, turned.turnRadians, 0.5 * DEGREES, L"8° clockwise");
    Assert::AreEqual(1.0, turned.scale, 0.01, L"at the same span");
    fingers.Up(0, 100, 100, 90ms);
    Assert::IsTrue(ManipulationOf(fingers.Frame(96ms)).phase == Neuron::GesturePhase::End, L"ended by a lift");
    fingers.Move(1, 300, 300, 100ms);
    const Neuron::TouchFrame& after = fingers.Frame(112ms);
    Assert::IsTrue(!after.drag.has_value() && !after.manipulation.has_value(), L"the last finger drags nothing");
    fingers.Up(1, 300, 300, 120ms);
    static_cast<void>(fingers.Frame(128ms));
    fingers.Down(0, 50, 50, 200ms);
    fingers.Up(0, 50, 50, 250ms);
    Assert::IsTrue(fingers.Frame(256ms).taps.size() == 1, L"then a tap again");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181TwoQuickStillContactsAreATwoFingerTap)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181TwoQuickStillContactsAreATwoFingerTap)
  {
    Fingers fingers;
    fingers.Down(0, 100, 100, 0ms);
    fingers.Down(1, 160, 100, 30ms);
    fingers.Up(0, 100, 100, 120ms);
    const Neuron::TouchFrame& tapped = fingers.Frame(128ms);
    Assert::IsTrue(tapped.twoFingerTaps.size() == 1 && tapped.taps.empty() && !tapped.manipulation.has_value(), L"a two-finger tap");
    ExpectPoint(130, 100, tapped.twoFingerTaps[0], L"at their center");
    fingers.Up(1, 160, 100, 130ms);
    Assert::IsTrue(fingers.Frame(144ms).taps.empty(), L"the second lift taps nothing");
    // Held past the tap time, the two contacts are a manipulation instead.
    fingers.Down(0, 100, 100, 1'000ms);
    fingers.Down(1, 160, 100, 1'030ms);
    Assert::IsTrue(ManipulationOf(fingers.Frame(1'300ms)).phase == Neuron::GesturePhase::Begin, L"a manipulation");
    fingers.Up(0, 100, 100, 1'320ms);
    Assert::IsTrue(fingers.Frame(1'336ms).twoFingerTaps.empty(), L"no tap");
  }

  BEGIN_TEST_METHOD_ATTRIBUTE(Adr181AThirdContactAndACancelEndTheGesture)
  TEST_METHOD_ATTRIBUTE(L"Requirement", L"MVP-100")
  END_TEST_METHOD_ATTRIBUTE()
  TEST_METHOD(Adr181AThirdContactAndACancelEndTheGesture)
  {
    Fingers fingers;
    // A drag that a second contact joins becomes a manipulation in the same frame.
    fingers.Down(0, 100, 100, 0ms);
    fingers.Move(0, 140, 100, 20ms);
    static_cast<void>(fingers.Frame(32ms));
    fingers.Down(1, 240, 100, 40ms);
    const Neuron::TouchFrame& joined = fingers.Frame(48ms);
    Assert::IsTrue(DragOf(joined).phase == Neuron::GesturePhase::Cancel, L"the drag canceled");
    Assert::IsTrue(ManipulationOf(joined).phase == Neuron::GesturePhase::Begin, L"the two begun");
    // A third contact is Windows'.
    fingers.Down(2, 300, 300, 60ms);
    Assert::IsTrue(ManipulationOf(fingers.Frame(64ms)).phase == Neuron::GesturePhase::Cancel, L"canceled");
    fingers.Up(0, 140, 100, 70ms);
    fingers.Up(1, 240, 100, 70ms);
    fingers.Up(2, 300, 300, 70ms);
    const Neuron::TouchFrame& lifted = fingers.Frame(80ms);
    Assert::IsTrue(lifted.taps.empty() && lifted.twoFingerTaps.empty() && !lifted.manipulation.has_value(), L"nothing more");
    // A canceled drag ends as a cancel.
    fingers.Down(0, 100, 100, 200ms);
    fingers.Move(0, 160, 100, 220ms);
    static_cast<void>(fingers.Frame(232ms));
    fingers.Cancel(0, 240ms);
    Assert::IsTrue(DragOf(fingers.Frame(248ms)).phase == Neuron::GesturePhase::Cancel, L"canceled");
    Assert::IsFalse(fingers.Frame(264ms).active, L"and nothing down");
  }
};

} // namespace NeuronClientTests
