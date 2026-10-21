#include "pch.h"

#include "InputRouter.h"
#include "Log.h"

#include <algorithm>
#include <utility>

namespace Neuron
{

namespace
{

bool IsAnalogue(const InputLeaf& _leaf) noexcept
{
  return _leaf.device == InputDevice::Mouse && (_leaf.control == static_cast<std::uint16_t>(MouseControl::Motion) ||
                                                _leaf.control == static_cast<std::uint16_t>(MouseControl::RawMotion) ||
                                                _leaf.control == static_cast<std::uint16_t>(MouseControl::Wheel));
}

// Table 3.2-D: a Boolean control accepts down, up and held; a movement control accepts move and read.
bool Accepts(const InputLeaf& _leaf) noexcept
{
  const bool movement = _leaf.condition == Condition::Move || _leaf.condition == Condition::Read;
  if (_leaf.device == InputDevice::Key)
  {
    return !movement && _leaf.control < KEY_CODE_COUNT;
  }
  return IsAnalogue(_leaf) ? movement : !movement && _leaf.control < MOUSE_BUTTON_COUNT;
}

// INP-060: the node kinds' operand rules, for the kinds M1 builds.
bool IsValid(const InputBinding& _binding) noexcept
{
  switch (_binding.kind)
  {
  case InputBinding::Kind::Leaf:
    return _binding.leaves.size() == 1 && Accepts(_binding.leaves[0]);
  case InputBinding::Kind::All:
    return _binding.leaves.size() >= 2 && std::ranges::all_of(_binding.leaves, Accepts) &&
           std::ranges::count_if(_binding.leaves, IsAnalogue) <= 1;
  case InputBinding::Kind::Chord:
    return _binding.leaves.size() >= 2 && std::ranges::none_of(_binding.leaves, IsAnalogue);
  }
  return false;
}

} // namespace

InputBinding KeyBinding(KeyCode _key, Condition _condition)
{
  return {InputBinding::Kind::Leaf, {{InputDevice::Key, static_cast<std::uint16_t>(_key), _condition}}};
}

InputBinding MouseBinding(MouseControl _control, Condition _condition)
{
  return {InputBinding::Kind::Leaf, {{InputDevice::Mouse, static_cast<std::uint16_t>(_control), _condition}}};
}

InputBinding AllOf(std::initializer_list<InputBinding> _operands)
{
  InputBinding all{InputBinding::Kind::All, {}};
  for (const InputBinding& operand : _operands)
  {
    all.leaves.insert(all.leaves.end(), operand.leaves.begin(), operand.leaves.end());
  }
  return all;
}

InputBinding ChordOf(std::initializer_list<KeyCode> _keys)
{
  InputBinding chord{InputBinding::Kind::Chord, {}};
  for (const KeyCode key : _keys)
  {
    chord.leaves.push_back({InputDevice::Key, static_cast<std::uint16_t>(key), Condition::Held});
  }
  return chord;
}

InputRouter::InputRouter(std::span<const ActionBindings> _actions, DeviceBackEnd& _backEnd, PointerHost* _pointer)
  : m_actions(_actions.begin(), _actions.end()),
    m_backEnd(_backEnd),
    m_pointerHost(_pointer),
    m_suppressed(_actions.size(), false)
{
  // The compiled-in set is validated once; a binding the node rules refuse is logged and left out (INP-170's checks).
  for (ActionBindings& action : m_actions)
  {
    const auto refused = std::ranges::remove_if(action.bindings, [](const InputBinding& _binding) { return !IsValid(_binding); });
    if (!refused.empty())
    {
      Log(LogCategory::Input, LogLevel::Error, "{} binding(s) of {} break the node rules and are left out", refused.size(), action.id);
      action.bindings.erase(refused.begin(), refused.end());
    }
  }
}

void InputRouter::AdvanceFrame(const FrameGates& _gates, SizePx _clientSize)
{
  std::ranges::fill(m_suppressed, false);
  for (ControlState& state : m_keys)
  {
    state.presses = 0;
    state.releases = 0;
  }
  for (ControlState& state : m_buttons)
  {
    state.presses = 0;
    state.releases = 0;
  }
  m_motionX = 0.0f;
  m_motionY = 0.0f;
  m_wheelNotches = 0.0f;
  m_clientSize = _clientSize;
  const PointPx before = m_pointer.positionPixels;

  // INP-101 steps 2, 3 and 5. The back-end's releases follow the events it collected, which happened before the focus
  // loss that they answer, so a key pressed and the window left within one frame ends the frame released.
  m_queue.clear();
  m_backEnd.Collect(m_queue);
  if (m_focused && !_gates.windowFocused)
  {
    m_backEnd.ReleaseAll(m_queue);
  }
  m_focused = _gates.windowFocused;
  for (const InputEvent& event : m_queue)
  {
    Apply(event);
  }

  // INP-156: the pointer follows the mouse, clamped to the client area; a clamp moves the system pointer there too.
  const PointPx unclamped = m_pointer.positionPixels;
  m_pointer.positionPixels.x = std::clamp(unclamped.x, 0, std::max(0, static_cast<std::int32_t>(_clientSize.width) - 1));
  m_pointer.positionPixels.y = std::clamp(unclamped.y, 0, std::max(0, static_cast<std::int32_t>(_clientSize.height) - 1));
  if ((m_pointer.positionPixels.x != unclamped.x || m_pointer.positionPixels.y != unclamped.y) && m_pointerHost != nullptr)
  {
    m_pointerHost->WarpPointer(m_pointer.positionPixels);
  }
  m_pointer.deltaX = m_motionX;
  m_pointer.deltaY = m_motionY;
  m_pointer.wheelDeltaNotches = m_wheelNotches;
  m_pointer.moved = m_pointer.positionPixels.x != before.x || m_pointer.positionPixels.y != before.y;
}

void InputRouter::Apply(const InputEvent& _event)
{
  // INP-111: while the window has no focus nothing is pressed and nothing moves; releases still apply.
  if (!m_focused && _event.kind != InputEvent::Kind::Release)
  {
    return;
  }
  if (_event.kind == InputEvent::Kind::Axis)
  {
    if (_event.device != InputDevice::Mouse)
    {
      return;
    }
    if (_event.control == static_cast<std::uint16_t>(MouseControl::Motion))
    {
      m_motionX += _event.x;
      m_motionY += _event.y;
      m_pointer.positionPixels = _event.pointer;
    }
    else if (_event.control == static_cast<std::uint16_t>(MouseControl::Wheel))
    {
      m_wheelNotches += _event.x;
    }
    return;
  }
  const auto stateIn = [&_event](auto& _states) -> ControlState*
  { return _event.control < _states.size() ? &_states[_event.control] : nullptr; };
  ControlState* state = _event.device == InputDevice::Key ? stateIn(m_keys) : stateIn(m_buttons);
  if (state == nullptr || _event.osRepeat)
  {
    return; // INP-053: an auto-repeat never presses and never changes held
  }
  if (_event.kind == InputEvent::Kind::Press)
  {
    ++state->presses;
    state->held = true;
  }
  else
  {
    ++state->releases;
    state->held = false;
  }
}

const InputRouter::ControlState* InputRouter::StateOf(InputDevice _device, std::uint16_t _control) const noexcept
{
  if (_device == InputDevice::Key)
  {
    return _control < m_keys.size() ? &m_keys[_control] : nullptr;
  }
  return _control < m_buttons.size() ? &m_buttons[_control] : nullptr;
}

bool InputRouter::Evaluate(const InputLeaf& _leaf, ActionValue& _value) const
{
  if (IsAnalogue(_leaf))
  {
    const bool wheel = _leaf.control == static_cast<std::uint16_t>(MouseControl::Wheel);
    const float x = wheel ? m_wheelNotches : m_motionX;
    const float y = wheel ? 0.0f : m_motionY;
    if (_leaf.control == static_cast<std::uint16_t>(MouseControl::RawMotion))
    {
      return false; // no raw movement is collected at M1, and no default binding reads it (ADR-028)
    }
    _value = {wheel ? ValueKind::Axis1 : ValueKind::Axis2, x, y};
    return _leaf.condition == Condition::Read || x != 0.0f || y != 0.0f;
  }
  const ControlState* state = StateOf(_leaf.device, _leaf.control);
  if (state == nullptr)
  {
    return false;
  }
  switch (_leaf.condition)
  {
  case Condition::Down:
    return state->presses > 0;
  case Condition::Up:
    return state->releases > 0;
  case Condition::Held:
    return state->held;
  case Condition::Move:
  case Condition::Read:
    return false;
  }
  return false;
}

bool InputRouter::Evaluate(const InputBinding& _binding, ActionValue& _value) const
{
  switch (_binding.kind)
  {
  case InputBinding::Kind::Leaf:
    return Evaluate(_binding.leaves[0], _value);
  case InputBinding::Kind::All:
  {
    // INP-060: every operand holds; the value is the analogue operand's.
    ActionValue value;
    for (const InputLeaf& leaf : _binding.leaves)
    {
      ActionValue operand;
      if (!Evaluate(leaf, operand))
      {
        return false;
      }
      if (IsAnalogue(leaf))
      {
        value = operand;
      }
    }
    _value = value;
    return true;
  }
  case InputBinding::Kind::Chord:
  {
    // INP-137: every part held and at least one gone down in this frame.
    bool wentDown = false;
    for (const InputLeaf& part : _binding.leaves)
    {
      const ControlState* state = StateOf(part.device, part.control);
      if (state == nullptr || !state->held)
      {
        return false;
      }
      wentDown = wentDown || state->presses > 0;
    }
    return wentDown;
  }
  }
  return false;
}

bool InputRouter::Query(ActionId _action, ActionValue* _value) const
{
  ActionValue value;
  bool fired = false;
  if (_action.index < m_actions.size() && !m_suppressed[_action.index])
  {
    for (const InputBinding& binding : m_actions[_action.index].bindings)
    {
      ActionValue candidate;
      if (Evaluate(binding, candidate))
      {
        value = candidate;
        fired = true;
        break;
      }
    }
  }
  if (_value != nullptr)
  {
    *_value = fired ? value : ActionValue{};
  }
  return fired;
}

void InputRouter::Suppress(ActionId _action)
{
  if (_action.index < m_suppressed.size())
  {
    m_suppressed[_action.index] = true;
  }
}

void InputRouter::WarpPointer(PointPx _clientPosition)
{
  _clientPosition.x = std::clamp(_clientPosition.x, 0, std::max(0, static_cast<std::int32_t>(m_clientSize.width) - 1));
  _clientPosition.y = std::clamp(_clientPosition.y, 0, std::max(0, static_cast<std::int32_t>(m_clientSize.height) - 1));
  m_pointer.positionPixels = _clientPosition;
  if (m_pointerHost != nullptr)
  {
    m_pointerHost->WarpPointer(_clientPosition);
  }
}

void InputRouter::ConfinePointer(bool _on)
{
  if (m_pointerHost != nullptr)
  {
    m_pointerHost->ConfinePointer(_on);
  }
}

} // namespace Neuron
