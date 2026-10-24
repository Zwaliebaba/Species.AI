#include "pch.h"

#include "TouchGestures.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace Neuron
{

namespace
{

double DistanceBetween(TouchPoint _a, TouchPoint _b) noexcept
{
  return std::hypot(_a.x - _b.x, _a.y - _b.y);
}

// The angle from _a to _b on the screen, whose rows grow downwards, so clockwise is positive.
double AngleBetween(TouchPoint _a, TouchPoint _b) noexcept
{
  return std::atan2(_b.y - _a.y, _b.x - _a.x);
}

} // namespace

void TouchGestures::Contact(const InputEvent& _event)
{
  if (_event.device != InputDevice::Touch || _event.control >= TOUCH_CONTACT_COUNT)
  {
    return;
  }
  m_next.active = true;
  const TouchPoint point{static_cast<double>(_event.pointer.x), static_cast<double>(_event.pointer.y)};
  switch (_event.kind)
  {
  case InputEvent::Kind::Press:
    Down(_event.control, point, _event.time);
    break;
  case InputEvent::Kind::Axis:
    Move(_event.control, point);
    break;
  case InputEvent::Kind::Release:
    Move(_event.control, point);
    Up(_event.control, true, _event.time);
    break;
  case InputEvent::Kind::Cancel:
    Up(_event.control, false, _event.time);
    break;
  }
}

const TouchFrame& TouchGestures::Advance(Nanoseconds _now)
{
  // ADR-181: a contact still for the hold time becomes a hold's drag, and two contacts that stay a possible tap past the
  // tap time become a manipulation.
  if (m_state == State::Pending)
  {
    const ContactState& contact = m_contacts[m_pair[0]];
    if (_now - contact.downTime >= m_thresholds.holdTime)
    {
      m_state = State::Dragging;
      m_heldDrag = true;
      EmitDrag(GesturePhase::Begin, true);
    }
  }
  else if (m_state == State::TwoPending && _now - m_pairTime > m_thresholds.tapTime)
  {
    m_state = State::Manipulating;
    BeginManipulation();
  }
  // A drag and a manipulation are reported in every frame of their lives, still or not.
  if (m_state == State::Dragging && !m_next.drag.has_value())
  {
    EmitDrag(GesturePhase::Move, m_heldDrag);
  }
  if (m_state == State::Manipulating)
  {
    const TouchPoint first = m_contacts[m_pair[0]].point;
    const TouchPoint second = m_contacts[m_pair[1]].point;
    const double span = DistanceBetween(first, second);
    const double angle = AngleBetween(first, second);
    const double turn = std::remainder(angle - m_lastAngle, 2.0 * std::numbers::pi);
    double reported = 0.0;
    if (m_turning)
    {
      reported = turn;
    }
    else
    {
      m_turnSum += turn;
      m_turning = std::abs(m_turnSum) >= m_thresholds.turnThresholdRadians;
    }
    const double scale = m_lastSpan > 0.0 && span > 0.0 ? span / m_lastSpan : 1.0;
    m_lastSpan = span;
    m_lastAngle = angle;
    TouchManipulation& manipulation = m_next.manipulation.has_value()
                                        ? *m_next.manipulation
                                        : m_next.manipulation.emplace(TouchManipulation{GesturePhase::Move, PairCenter(), 1.0, 0.0});
    manipulation.center = PairCenter();
    manipulation.scale = scale;
    manipulation.turnRadians = reported;
  }
  m_next.active = m_next.active || ContactsDown() > 0;
  m_frame = std::move(m_next);
  m_next = TouchFrame{};
  return m_frame;
}

std::size_t TouchGestures::ContactsDown() const noexcept
{
  return static_cast<std::size_t>(std::ranges::count_if(m_contacts, [](const ContactState& _contact) { return _contact.down; }));
}

TouchPoint TouchGestures::PairCenter() const noexcept
{
  const TouchPoint first = m_contacts[m_pair[0]].point;
  const TouchPoint second = m_contacts[m_pair[1]].point;
  return {(first.x + second.x) * 0.5, (first.y + second.y) * 0.5};
}

void TouchGestures::Down(std::uint16_t _slot, TouchPoint _point, Nanoseconds _time)
{
  ContactState& contact = m_contacts[_slot];
  if (contact.down)
  {
    return;
  }
  contact = {true, false, _point, _point, _time};
  switch (m_state)
  {
  case State::Idle:
    m_state = State::Pending;
    m_pair[0] = _slot;
    m_heldDrag = false;
    break;
  case State::Pending:
    // A second contact while the first is neither a drag nor a hold: perhaps a two-finger tap.
    m_state = State::TwoPending;
    m_pair[1] = _slot;
    m_pairTime = m_contacts[m_pair[0]].downTime;
    break;
  case State::Dragging:
    // A second contact ends the drag, and the two begin a manipulation at once.
    EmitDrag(GesturePhase::Cancel, m_heldDrag);
    m_state = State::Manipulating;
    m_pair[1] = _slot;
    BeginManipulation();
    break;
  case State::TwoPending:
  case State::Manipulating:
    // A third contact is Windows' (the shell's gestures): the two-contact gesture ends.
    if (m_state == State::Manipulating)
    {
      EmitManipulation(GesturePhase::Cancel);
    }
    m_state = State::Ignoring;
    break;
  case State::Ignoring:
    break;
  }
}

void TouchGestures::Move(std::uint16_t _slot, TouchPoint _point)
{
  ContactState& contact = m_contacts[_slot];
  if (!contact.down)
  {
    return;
  }
  contact.point = _point;
  if (!contact.moved && DistanceBetween(contact.point, contact.start) > m_thresholds.slopPixels)
  {
    contact.moved = true;
  }
  switch (m_state)
  {
  case State::Pending:
    if (contact.moved)
    {
      m_state = State::Dragging;
      m_heldDrag = false;
      EmitDrag(GesturePhase::Begin, false);
    }
    break;
  case State::Dragging:
    if (_slot == m_pair[0])
    {
      EmitDrag(m_next.drag.has_value() ? m_next.drag->phase : GesturePhase::Move, m_heldDrag);
    }
    break;
  case State::TwoPending:
    if (contact.moved)
    {
      m_state = State::Manipulating;
      BeginManipulation();
    }
    break;
  case State::Idle:
  case State::Manipulating:
  case State::Ignoring:
    break;
  }
}

void TouchGestures::Up(std::uint16_t _slot, bool _lifted, Nanoseconds _time)
{
  ContactState& contact = m_contacts[_slot];
  if (!contact.down)
  {
    return;
  }
  const TouchPoint pairCenter = m_state == State::TwoPending ? PairCenter() : TouchPoint{};
  contact.down = false;
  const State state = m_state;
  m_state = ContactsDown() == 0 ? State::Idle : State::Ignoring;
  switch (state)
  {
  case State::Pending:
    if (_lifted && !contact.moved && _time - contact.downTime <= m_thresholds.tapTime)
    {
      Tap(contact.start, _time);
    }
    break;
  case State::Dragging:
    if (_slot == m_pair[0])
    {
      EmitDrag(_lifted ? GesturePhase::End : GesturePhase::Cancel, m_heldDrag);
    }
    break;
  case State::TwoPending:
    // The first lift of a two-finger tap ends it: both down within the tap time and neither past the slop.
    if (_lifted && _time - m_pairTime <= m_thresholds.tapTime && !m_contacts[m_pair[0]].moved && !m_contacts[m_pair[1]].moved)
    {
      m_next.twoFingerTaps.push_back(pairCenter);
    }
    break;
  case State::Manipulating:
    EmitManipulation(_lifted ? GesturePhase::End : GesturePhase::Cancel);
    break;
  case State::Idle:
  case State::Ignoring:
    break;
  }
}

void TouchGestures::EmitDrag(GesturePhase _phase, bool _held)
{
  const ContactState& contact = m_contacts[m_pair[0]];
  // A drag that began in this frame keeps its beginning, unless it also ended in it.
  const GesturePhase phase =
    m_next.drag.has_value() && m_next.drag->phase == GesturePhase::Begin && _phase == GesturePhase::Move ? GesturePhase::Begin : _phase;
  m_next.drag = TouchDrag{phase, contact.start, contact.point, _held};
}

void TouchGestures::BeginManipulation()
{
  const TouchPoint first = m_contacts[m_pair[0]].point;
  const TouchPoint second = m_contacts[m_pair[1]].point;
  m_lastSpan = DistanceBetween(first, second);
  m_lastAngle = AngleBetween(first, second);
  m_turnSum = 0.0;
  m_turning = false;
  EmitManipulation(GesturePhase::Begin);
}

void TouchGestures::EmitManipulation(GesturePhase _phase)
{
  const GesturePhase phase =
    m_next.manipulation.has_value() && m_next.manipulation->phase == GesturePhase::Begin && _phase == GesturePhase::Move
      ? GesturePhase::Begin
      : _phase;
  m_next.manipulation = TouchManipulation{phase, PairCenter(), 1.0, 0.0};
}

void TouchGestures::Tap(TouchPoint _point, Nanoseconds _time)
{
  m_next.taps.push_back(_point);
  // A second tap close to the first and soon after it completes a double tap; a third begins again.
  if (m_lastTap.has_value() && _time - m_lastTapTime <= m_thresholds.doubleTapTime &&
      DistanceBetween(*m_lastTap, _point) <= m_thresholds.doubleTapSlopPixels)
  {
    m_next.doubleTaps.push_back(_point);
    m_lastTap.reset();
    return;
  }
  m_lastTap = _point;
  m_lastTapTime = _time;
}

} // namespace Neuron
