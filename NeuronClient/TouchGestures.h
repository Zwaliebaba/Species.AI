#pragma once

#include "InputEvents.h"
#include "PlatformClock.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace Neuron
{

/// The touch recognizer's thresholds (ADR-181), in client pixels and real time: the game gives its keys, which are in
/// effective pixels, × the DPI scale (ADR-033).
struct TouchThresholds
{
  double slopPixels = 0.0;           // a contact moved farther than this from where it went down is a drag
  double doubleTapSlopPixels = 0.0;  // a second tap this close to the first completes a double tap
  Nanoseconds tapTime{};             // a contact down and up within this, never past the slop, is a tap
  Nanoseconds holdTime{};            // a contact still for this is a hold
  Nanoseconds doubleTapTime{};       // a second tap ending within this of the first completes a double tap
  double turnThresholdRadians = 0.0; // a manipulation's turn is reported once its sum in the gesture passes this
};

/// A point of the client area in pixels, with the fraction a center of two contacts has.
struct TouchPoint
{
  double x = 0.0;
  double y = 0.0;
};

enum class GesturePhase : std::uint8_t
{
  Begin,
  Move,
  End,
  Cancel
};

/// One contact's drag (ADR-181), reported in every frame from its beginning to its end: begun past the slop, or, held,
/// begun still at the hold time.
struct TouchDrag
{
  GesturePhase phase = GesturePhase::Begin;
  TouchPoint start; // where the contact went down
  TouchPoint point; // where it is
  bool held = false;
};

/// Two contacts' manipulation (ADR-181), reported in every frame from its beginning to its end: their center, and since
/// the previous frame the ratio of their spans and their turn in radians, clockwise on the screen positive. The turn is 0
/// until its sum in the gesture passes the threshold, and only what follows is reported.
struct TouchManipulation
{
  GesturePhase phase = GesturePhase::Begin;
  TouchPoint center;
  double scale = 1.0;
  double turnRadians = 0.0;
};

/// A frame's gestures (ADR-181).
struct TouchFrame
{
  std::vector<TouchPoint> taps;       // where each tap's contact went down, in the order they ended
  std::vector<TouchPoint> doubleTaps; // the second taps that completed a double tap, which are among the taps too
  std::vector<TouchPoint> twoFingerTaps;
  std::optional<TouchDrag> drag;
  std::optional<TouchManipulation> manipulation;
  bool active = false; // a touch event arrived, or a contact was down, in the frame
};

/// The gesture recognizer of ADR-181: a pure function of the touch events, their times and each frame's time, with
/// three or more contacts left to Windows. Main thread only.
class TouchGestures
{
public:
  void SetThresholds(const TouchThresholds& _thresholds) noexcept
  {
    m_thresholds = _thresholds;
  }
  // One touch event of the frame's queue, in order; an event of another device or slot is ignored.
  void Contact(const InputEvent& _event);
  // The frame's gestures at _now, after its events: a hold and the two-finger tap's window are timed here.
  const TouchFrame& Advance(Nanoseconds _now);
  [[nodiscard]] const TouchFrame& Frame() const noexcept
  {
    return m_frame;
  }

private:
  enum class State : std::uint8_t
  {
    Idle,
    Pending,      // one contact, neither a drag nor a hold yet
    Dragging,     // one contact, a drag or a hold's drag
    TwoPending,   // two contacts, still a possible two-finger tap
    Manipulating, // two contacts
    Ignoring      // until every contact is up: after a two-contact gesture, a third contact or a cancel
  };

  struct ContactState
  {
    bool down = false;
    bool moved = false; // past the slop from where it went down
    TouchPoint start;
    TouchPoint point;
    Nanoseconds downTime{};
  };

  [[nodiscard]] std::size_t ContactsDown() const noexcept;
  [[nodiscard]] TouchPoint PairCenter() const noexcept;
  void Down(std::uint16_t _slot, TouchPoint _point, Nanoseconds _time);
  void Move(std::uint16_t _slot, TouchPoint _point);
  void Up(std::uint16_t _slot, bool _lifted, Nanoseconds _time);
  void EmitDrag(GesturePhase _phase, bool _held);
  void BeginManipulation();
  void EmitManipulation(GesturePhase _phase);
  void Tap(TouchPoint _point, Nanoseconds _time);

  TouchThresholds m_thresholds;
  std::array<ContactState, TOUCH_CONTACT_COUNT> m_contacts{};
  State m_state = State::Idle;
  std::array<std::uint16_t, 2> m_pair{}; // the drag's contact first, then the manipulation's second
  bool m_heldDrag = false;
  Nanoseconds m_pairTime{}; // when the first of the pair went down
  double m_lastSpan = 0.0;
  double m_lastAngle = 0.0;
  double m_turnSum = 0.0;
  bool m_turning = false;
  std::optional<TouchPoint> m_lastTap;
  Nanoseconds m_lastTapTime{};
  TouchFrame m_next;  // the frame being gathered
  TouchFrame m_frame; // the frame Advance last gave
};

} // namespace Neuron
