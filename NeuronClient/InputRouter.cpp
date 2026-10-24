#include "pch.h"

#include "InputNames.h"
#include "InputRouter.h"
#include "Log.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <utility>

namespace Neuron
{

namespace
{

// The deepest chain of aliases a query follows; Rebuild refuses a cycle (DEC-554), so a chain this long is a defect.
constexpr std::uint32_t ALIAS_DEPTH_MAX = 32;

bool IsAnalogue(const InputLeaf& _leaf) noexcept
{
  return _leaf.device == InputDevice::Mouse && (_leaf.control == static_cast<std::uint16_t>(MouseControl::Motion) ||
                                                _leaf.control == static_cast<std::uint16_t>(MouseControl::RawMotion) ||
                                                _leaf.control == static_cast<std::uint16_t>(MouseControl::Wheel) ||
                                                _leaf.control == static_cast<std::uint16_t>(MouseControl::WheelHorizontal));
}

bool IsWheel(const InputLeaf& _leaf) noexcept
{
  return _leaf.control == static_cast<std::uint16_t>(MouseControl::Wheel) ||
         _leaf.control == static_cast<std::uint16_t>(MouseControl::WheelHorizontal);
}

bool IsBooleanCondition(Condition _condition) noexcept
{
  return _condition == Condition::Down || _condition == Condition::Up || _condition == Condition::Held || _condition == Condition::Repeat;
}

// The control's name, for a diagnostic; empty for a control the name table does not hold.
std::string_view ControlName(const InputLeaf& _leaf) noexcept
{
  if (_leaf.device == InputDevice::Key)
  {
    return NameOfKey(static_cast<KeyCode>(_leaf.control));
  }
  return _leaf.control <= static_cast<std::uint16_t>(MouseControl::WheelHorizontal)
           ? NameOfMouseControl(static_cast<MouseControl>(_leaf.control))
           : std::string_view{};
}

// Table 3.2-D: a key or mouse button accepts down, up, held and repeat; pointer movement, raw movement and the wheel
// accept move and read. The kind is the condition's.
std::optional<std::string> CheckLeaf(const InputLeaf& _leaf, ValueKind& _kind)
{
  const std::string_view control = ControlName(_leaf);
  if (control.empty())
  {
    return std::format("the control {} of the {} has no name (INP-055)", _leaf.control,
                       _leaf.device == InputDevice::Key ? "keyboard" : "mouse");
  }
  const bool movement = IsAnalogue(_leaf);
  const bool accepted =
    movement ? (_leaf.condition == Condition::Move || _leaf.condition == Condition::Read) : IsBooleanCondition(_leaf.condition);
  if (!accepted)
  {
    return std::format("{} does not accept the condition {} (table 3.2-D)", control, NameOfCondition(_leaf.condition));
  }
  _kind = !movement ? ValueKind::Boolean : IsWheel(_leaf) ? ValueKind::Axis1 : ValueKind::Axis2;
  return std::nullopt;
}

std::string_view KindName(InputBinding::Kind _kind) noexcept
{
  switch (_kind)
  {
  case InputBinding::Kind::Leaf:
    return "leaf";
  case InputBinding::Kind::All:
    return "all";
  case InputBinding::Kind::Chord:
    return "chord";
  case InputBinding::Kind::Not:
    return "not";
  case InputBinding::Kind::Alias:
    return "alias";
  case InputBinding::Kind::Latch:
    return "latch";
  case InputBinding::Kind::Idle:
    return "idle";
  case InputBinding::Kind::Setting:
    return "setting";
  case InputBinding::Kind::Value:
    return "value";
  }
  return "node";
}

} // namespace

bool InputBinding::operator==(const InputBinding& _other) const
{
  return kind == _other.kind && leaf.device == _other.leaf.device && leaf.control == _other.leaf.control &&
         leaf.condition == _other.leaf.condition && operands == _other.operands && name == _other.name && idle == _other.idle &&
         seconds == _other.seconds && negate == _other.negate;
}

InputBinding KeyBinding(KeyCode _key, Condition _condition)
{
  InputBinding binding;
  binding.leaf = {InputDevice::Key, static_cast<std::uint16_t>(_key), _condition};
  return binding;
}

InputBinding MouseBinding(MouseControl _control, Condition _condition)
{
  InputBinding binding;
  binding.leaf = {InputDevice::Mouse, static_cast<std::uint16_t>(_control), _condition};
  return binding;
}

InputBinding AllOf(std::initializer_list<InputBinding> _operands)
{
  InputBinding all;
  all.kind = InputBinding::Kind::All;
  all.operands.assign(_operands.begin(), _operands.end());
  return all;
}

InputBinding ChordOf(std::initializer_list<KeyCode> _keys)
{
  InputBinding chord;
  chord.kind = InputBinding::Kind::Chord;
  for (const KeyCode key : _keys)
  {
    chord.operands.push_back(KeyBinding(key, Condition::Held));
  }
  return chord;
}

InputBinding NotOf(InputBinding _operand)
{
  InputBinding node;
  node.kind = InputBinding::Kind::Not;
  node.operands.push_back(std::move(_operand));
  return node;
}

InputBinding AliasOf(std::string_view _actionId)
{
  InputBinding node;
  node.kind = InputBinding::Kind::Alias;
  node.name = std::string(_actionId);
  return node;
}

InputBinding LatchOf(InputBinding _on, InputBinding _off)
{
  InputBinding node;
  node.kind = InputBinding::Kind::Latch;
  node.operands.push_back(std::move(_on));
  node.operands.push_back(std::move(_off));
  return node;
}

InputBinding IdleOf(IdleTest _test, std::uint32_t _seconds)
{
  InputBinding node;
  node.kind = InputBinding::Kind::Idle;
  node.idle = _test;
  node.seconds = _test == IdleTest::NoLonger ? 0 : _seconds;
  return node;
}

InputBinding SettingOf(std::string_view _key, bool _negate)
{
  InputBinding node;
  node.kind = InputBinding::Kind::Setting;
  node.name = std::string(_key);
  node.negate = _negate;
  return node;
}

InputBinding AlwaysTrue()
{
  InputBinding node;
  node.kind = InputBinding::Kind::Value;
  return node;
}

InputRouter::InputRouter(std::span<const ActionBindings> _actions, DeviceBackEnd& _backEnd, PointerHost* _pointer, SettingAccess _settings)
  : m_defaults(_actions.begin(), _actions.end()),
    m_actions(m_defaults),
    m_settings(std::move(_settings)),
    m_backEnd(_backEnd),
    m_pointerHost(_pointer),
    m_suppressed(_actions.size(), false)
{
  // The compiled-in set is validated once; a binding the node rules refuse is logged and left out (INP-170's checks).
  for (std::size_t index = 0; index < m_defaults.size(); ++index)
  {
    std::vector<InputBinding>& bindings = m_defaults[index].bindings;
    const ActionId action{static_cast<std::uint16_t>(index)};
    const auto refused = std::ranges::remove_if(bindings,
                                                [&](const InputBinding& _binding)
                                                {
                                                  const std::optional<std::string> error = CheckBinding(action, _binding);
                                                  if (error.has_value())
                                                  {
                                                    Log(LogCategory::Input, LogLevel::Error, "a default binding of {} is left out: {}",
                                                        m_defaults[index].id, *error);
                                                  }
                                                  return error.has_value();
                                                });
    bindings.erase(refused.begin(), refused.end());
  }
  static_cast<void>(Rebuild({}));
}

std::optional<ActionId> InputRouter::FindAction(std::string_view _id) const noexcept
{
  for (std::size_t index = 0; index < m_actions.size(); ++index)
  {
    if (m_actions[index].id == _id)
    {
      return ActionId{static_cast<std::uint16_t>(index)};
    }
  }
  return std::nullopt;
}

std::span<const InputBinding> InputRouter::BindingsOf(ActionId _action) const noexcept
{
  return _action.index < m_actions.size() ? std::span<const InputBinding>(m_actions[_action.index].bindings)
                                          : std::span<const InputBinding>{};
}

std::optional<std::string> InputRouter::CheckNode(const InputBinding& _node, ValueKind& _kind) const
{
  _kind = ValueKind::Boolean;
  switch (_node.kind)
  {
  case InputBinding::Kind::Leaf:
    return CheckLeaf(_node.leaf, _kind);
  case InputBinding::Kind::All:
  {
    if (_node.operands.size() < 2)
    {
      return std::string("an all needs two or more operands");
    }
    std::size_t analogue = 0;
    for (const InputBinding& operand : _node.operands)
    {
      if (operand.kind == InputBinding::Kind::All)
      {
        return std::string("an all cannot hold an all");
      }
      ValueKind kind = ValueKind::Boolean;
      if (std::optional<std::string> error = CheckNode(operand, kind))
      {
        return error;
      }
      if (kind != ValueKind::Boolean)
      {
        ++analogue;
        _kind = kind;
      }
    }
    return analogue > 1 ? std::optional<std::string>("an all holds at most one analogue operand") : std::nullopt;
  }
  case InputBinding::Kind::Chord:
    if (_node.operands.size() < 2)
    {
      return std::string("a chord needs two or more parts");
    }
    for (const InputBinding& part : _node.operands)
    {
      if (part.kind != InputBinding::Kind::Leaf || IsAnalogue(part.leaf) || ControlName(part.leaf).empty())
      {
        return std::string("a chord's part is a key or a mouse button");
      }
    }
    return std::nullopt;
  case InputBinding::Kind::Not:
  {
    if (_node.operands.size() != 1)
    {
      return std::string("a not has one operand");
    }
    const InputBinding& operand = _node.operands[0];
    if (operand.kind == InputBinding::Kind::All || operand.kind == InputBinding::Kind::Chord)
    {
      return std::format("a not cannot hold an {}", KindName(operand.kind));
    }
    ValueKind kind = ValueKind::Boolean;
    if (std::optional<std::string> error = CheckNode(operand, kind))
    {
      return error;
    }
    return kind == ValueKind::Boolean ? std::nullopt : std::optional<std::string>("a not's operand must be Boolean");
  }
  case InputBinding::Kind::Alias:
    return FindAction(_node.name).has_value() ? std::nullopt : std::optional<std::string>(std::format("{} is not an action", _node.name));
  case InputBinding::Kind::Latch:
  {
    if (_node.operands.size() != 2)
    {
      return std::string("a latch has an on and an off operand");
    }
    for (const InputBinding& operand : _node.operands)
    {
      if (operand.kind == InputBinding::Kind::Latch)
      {
        return std::string("a latch cannot hold a latch");
      }
      ValueKind kind = ValueKind::Boolean;
      if (std::optional<std::string> error = CheckNode(operand, kind))
      {
        return error;
      }
      if (kind != ValueKind::Boolean)
      {
        return std::string("a latch's operands must be Boolean");
      }
    }
    return std::nullopt;
  }
  case InputBinding::Kind::Idle:
    if (_node.idle != IdleTest::NoLonger && _node.seconds < 1)
    {
      return std::string("an idle test needs a whole number of seconds of at least 1");
    }
    _kind = _node.idle == IdleTest::AtLeast ? ValueKind::Axis1 : ValueKind::Boolean;
    return std::nullopt;
  case InputBinding::Kind::Setting:
    if (!m_settings.exists || !m_settings.exists(_node.name))
    {
      return std::format("the setting {} is not in the settings store or its defaults", _node.name);
    }
    return std::nullopt;
  case InputBinding::Kind::Value:
    // INP-143: no edition flag record exists at M2, so no feature flag is on/off flag of it.
    return _node.name.empty() ? std::nullopt
                              : std::optional<std::string>(std::format("{} is not a feature flag of the edition", _node.name));
  }
  return std::string("an unknown node");
}

bool InputRouter::AliasReaches(const InputBinding& _node, std::string_view _target, std::uint32_t _depth) const
{
  if (_depth > ALIAS_DEPTH_MAX)
  {
    return true;
  }
  if (_node.kind == InputBinding::Kind::Alias)
  {
    if (_node.name == _target)
    {
      return true;
    }
    if (const std::optional<ActionId> next = FindAction(_node.name))
    {
      return std::ranges::any_of(m_actions[next->index].bindings,
                                 [&](const InputBinding& _binding) { return AliasReaches(_binding, _target, _depth + 1); });
    }
    return false;
  }
  return std::ranges::any_of(_node.operands, [&](const InputBinding& _operand) { return AliasReaches(_operand, _target, _depth); });
}

std::optional<std::string> InputRouter::CheckBinding(ActionId _action, const InputBinding& _binding) const
{
  if (_action.index >= m_actions.size())
  {
    return std::string("no such action");
  }
  ValueKind kind = ValueKind::Boolean;
  if (std::optional<std::string> error = CheckNode(_binding, kind))
  {
    return error;
  }
  // INP-061: an analogue action takes no Boolean binding.
  if (m_actions[_action.index].kind != ValueKind::Boolean && kind == ValueKind::Boolean)
  {
    return std::format("{} is analogue and takes no Boolean binding", m_actions[_action.index].id);
  }
  // DEC-554: an alias chain that leads back to its own action.
  if (AliasReaches(_binding, m_actions[_action.index].id, 0))
  {
    return std::format("an alias chain leads back to {}", m_actions[_action.index].id);
  }
  return std::nullopt;
}

void InputRouter::AssignSlots(InputBinding& _node)
{
  switch (_node.kind)
  {
  case InputBinding::Kind::Latch:
    _node.slot = static_cast<std::uint32_t>(m_slotState.size());
    m_slotState.push_back(false);
    m_latchNodes.push_back(&_node);
    break;
  case InputBinding::Kind::Setting:
    _node.slot = static_cast<std::uint32_t>(m_slotState.size());
    m_slotState.push_back(false);
    m_settingNodes.push_back(&_node);
    break;
  case InputBinding::Kind::Alias:
    _node.slot = FindAction(_node.name).value_or(ActionId{}).index; // checked when it loaded
    break;
  default:
    break;
  }
  for (InputBinding& operand : _node.operands)
  {
    AssignSlots(operand);
  }
}

std::size_t InputRouter::Rebuild(std::span<const BindingReplacement> _replacements)
{
  m_actions = m_defaults;
  std::size_t applied = 0;
  for (const BindingReplacement& replacement : _replacements)
  {
    const std::optional<ActionId> action = FindAction(replacement.action);
    if (!action.has_value())
    {
      Log(LogCategory::Input, LogLevel::Warning, "{}: {} is not an action, so the binding is skipped (INP-171)", replacement.source,
          replacement.action);
      continue;
    }
    if (const std::optional<std::string> error = CheckBinding(*action, replacement.binding))
    {
      Log(LogCategory::Input, LogLevel::Warning, "{}: the binding is skipped (INP-171): {}", replacement.source, *error);
      continue;
    }
    // INP-065: replacing removes the primary binding and puts the new one at index 0, or appends it to an empty list.
    std::vector<InputBinding>& bindings = m_actions[action->index].bindings;
    if (bindings.empty())
    {
      bindings.push_back(replacement.binding);
    }
    else
    {
      bindings[0] = replacement.binding;
    }
    ++applied;
  }
  // INP-066: the state table is made again, and every latch starts false.
  m_slotState.clear();
  m_latchNodes.clear();
  m_settingNodes.clear();
  for (ActionBindings& action : m_actions)
  {
    for (InputBinding& binding : action.bindings)
    {
      AssignSlots(binding);
    }
  }
  return applied;
}

void InputRouter::AdvanceFrame(const FrameGates& _gates, SizePx _clientSize, Nanoseconds _realTime)
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
  m_wheelHorizontalNotches = 0.0f;
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
  // INP-112: the frame's characters bypass the queue and the bindings; an unfocused window's are dropped.
  m_text.clear();
  if (m_textSource != nullptr)
  {
    m_textSource->TakeText(m_text);
    if (!_gates.windowFocused)
    {
      m_text.clear();
    }
  }
  m_focused = _gates.windowFocused;
  for (const InputEvent& event : m_queue)
  {
    Apply(event);
  }
  // ADR-181: the frame's touch gestures; a tap is the primary button's press and release at its point in this frame, so
  // that every reader of the primary press takes it as a click.
  m_touchSuppressed = false;
  const TouchFrame& touch = m_touch.Advance(_realTime);
  if (!touch.taps.empty() && m_focused)
  {
    const TouchPoint tap = touch.taps.back();
    m_pointer.positionPixels = {static_cast<std::int32_t>(std::lround(tap.x)), static_cast<std::int32_t>(std::lround(tap.y))};
    ControlState& primary = m_buttons[static_cast<std::size_t>(MouseControl::Primary)];
    ++primary.presses;
    ++primary.releases;
  }
  // INP-101 step 7, INP-133, INP-134: each control's repeat timer fires on the press, then at the press + the initial
  // delay and every period after while it is held; its firings in the frame are the condition's value.
  const auto advance = [this, _realTime](ControlState& _state)
  {
    _state.repeats = 0;
    if (_state.presses > 0)
    {
      _state.repeats = 1;
      _state.nextRepeat = _realTime + m_repeat.initialDelay;
      return;
    }
    if (!_state.held || m_repeat.period <= Nanoseconds{0})
    {
      return;
    }
    while (_state.nextRepeat <= _realTime && _state.repeats < std::numeric_limits<std::uint16_t>::max())
    {
      ++_state.repeats;
      _state.nextRepeat += m_repeat.period;
    }
  };
  std::ranges::for_each(m_keys, advance);
  std::ranges::for_each(m_buttons, advance);

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

  // INP-151: the keyboard or the mouse was active when a control is held or had an event, or the pointer or the wheel
  // moved.
  const auto activeControl = [](const ControlState& _state) { return _state.held || _state.presses > 0 || _state.releases > 0; };
  const bool active = std::ranges::any_of(m_keys, activeControl) || std::ranges::any_of(m_buttons, activeControl) || m_motionX != 0.0f ||
                      m_motionY != 0.0f || m_wheelNotches != 0.0f || m_wheelHorizontalNotches != 0.0f;
  UpdateEnvironment(_realTime, active, touch.active);
}

const TouchFrame& InputRouter::Touch() const noexcept
{
  static const TouchFrame NONE;
  return m_touchSuppressed ? NONE : m_touch.Frame();
}

void InputRouter::UpdateEnvironment(Nanoseconds _realTime, bool _active, bool _touchActive)
{
  // INP-150: the keyboard and the mouse are player 0's, and no gamepad exists at M2, so an active frame keeps
  // KeyboardMouse; a frame with touch is Touch's (ADR-181), whose taps also press the primary button.
  if (_touchActive)
  {
    m_mode = InputMode::Touch;
  }
  else if (_active)
  {
    m_mode = InputMode::KeyboardMouse;
  }
  // INP-153: the idle time grows by the frame's real time while frames stay idle, and is 0 in an active frame.
  const double elapsed = m_lastRealTime.has_value() ? std::chrono::duration<double>(_realTime - *m_lastRealTime).count() : 0.0;
  m_lastRealTime = _realTime;
  m_previousIdleSeconds = m_idleSeconds;
  m_idle = !_active && !_touchActive;
  m_idleSeconds = !m_idle ? 0.0 : m_wasIdle ? m_idleSeconds + std::max(elapsed, 0.0) : 0.0;
  m_wasIdle = m_idle;
  // INP-101 step 7, INP-140: each latch once a frame, state := (state and not off) or (not state and on).
  for (const InputBinding* latch : m_latchNodes)
  {
    ActionValue ignored;
    const bool on = Evaluate(latch->operands[0], ignored, 0);
    const bool off = Evaluate(latch->operands[1], ignored, 0);
    const bool state = m_slotState[latch->slot];
    m_slotState[latch->slot] = (state && !off) || (!state && on);
  }
  // INP-101 step 8, INP-142: the named settings' values, read once a frame; true above 0, or not when negated.
  for (const InputBinding* setting : m_settingNodes)
  {
    const std::optional<double> value = m_settings.number ? m_settings.number(setting->name) : std::nullopt;
    const bool above = value.has_value() && *value > 0.0;
    m_slotState[setting->slot] = setting->negate ? !above : above;
  }
}

void InputRouter::Apply(const InputEvent& _event)
{
  // INP-111: while the window has no focus nothing is pressed and nothing moves; releases and cancels still apply.
  if (!m_focused && _event.kind != InputEvent::Kind::Release && _event.kind != InputEvent::Kind::Cancel)
  {
    return;
  }
  // ADR-181: a touch contact goes to the recognizer, and the pointer follows it.
  if (_event.device == InputDevice::Touch)
  {
    m_touch.Contact(_event);
    if (_event.kind != InputEvent::Kind::Cancel)
    {
      m_pointer.positionPixels = _event.pointer;
    }
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
    else if (_event.control == static_cast<std::uint16_t>(MouseControl::WheelHorizontal))
    {
      m_wheelHorizontalNotches += _event.x;
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
    const bool wheel = IsWheel(_leaf);
    const float x = _leaf.control == static_cast<std::uint16_t>(MouseControl::WheelHorizontal) ? m_wheelHorizontalNotches
                    : wheel                                                                    ? m_wheelNotches
                                                                                               : m_motionX;
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
  case Condition::Repeat:
    _value = {ValueKind::Boolean, static_cast<float>(state->repeats), 0.0f};
    return state->repeats > 0;
  case Condition::Move:
  case Condition::Read:
  case Condition::Position:
  case Condition::Release:
  case Condition::Zero:
    return false;
  }
  return false;
}

bool InputRouter::Evaluate(const InputBinding& _binding, ActionValue& _value, std::uint32_t _depth) const
{
  switch (_binding.kind)
  {
  case InputBinding::Kind::Leaf:
    return Evaluate(_binding.leaf, _value);
  case InputBinding::Kind::All:
  {
    // INP-136, INP-090: every operand in order, false at the first false one; the value is the analogue operand's, or
    // else a repeat's firings.
    ActionValue value;
    for (const InputBinding& operand : _binding.operands)
    {
      ActionValue result;
      if (!Evaluate(operand, result, _depth))
      {
        return false;
      }
      if (result.kind != ValueKind::Boolean || (value.kind == ValueKind::Boolean && result.x != 0.0f))
      {
        value = result;
      }
    }
    _value = value;
    return true;
  }
  case InputBinding::Kind::Chord:
  {
    // INP-137: every part held and at least one gone down in this frame.
    bool wentDown = false;
    for (const InputBinding& part : _binding.operands)
    {
      const ControlState* state = StateOf(part.leaf.device, part.leaf.control);
      if (state == nullptr || !state->held)
      {
        return false;
      }
      wentDown = wentDown || state->presses > 0;
    }
    return wentDown;
  }
  case InputBinding::Kind::Not:
  {
    // INP-138.
    ActionValue ignored;
    return !Evaluate(_binding.operands[0], ignored, _depth);
  }
  case InputBinding::Kind::Alias:
    // INP-139: the target's query, its suppression included, with its value.
    return _depth < ALIAS_DEPTH_MAX && QueryAt(_binding.slot, _value, _depth + 1);
  case InputBinding::Kind::Latch:
  case InputBinding::Kind::Setting:
    return m_slotState[_binding.slot];
  case InputBinding::Kind::Idle:
  {
    // INP-141.
    const auto seconds = static_cast<double>(_binding.seconds);
    switch (_binding.idle)
    {
    case IdleTest::AtLeast:
      _value = {ValueKind::Axis1, static_cast<float>(m_idleSeconds), 0.0f};
      return m_idleSeconds >= seconds;
    case IdleTest::Reaches:
      return m_previousIdleSeconds < seconds && m_idleSeconds >= seconds;
    case IdleTest::NoLonger:
      return m_idleSeconds == 0.0;
    }
    return false;
  }
  case InputBinding::Kind::Value:
    return _binding.name.empty(); // INP-143: always_true; no feature flag passes the load at M2
  }
  return false;
}

bool InputRouter::QueryAt(std::size_t _action, ActionValue& _value, std::uint32_t _depth) const
{
  _value = {};
  if (_action >= m_actions.size() || m_suppressed[_action])
  {
    return false;
  }
  for (const InputBinding& binding : m_actions[_action].bindings)
  {
    ActionValue candidate;
    if (Evaluate(binding, candidate, _depth))
    {
      _value = candidate;
      return true;
    }
  }
  return false;
}

bool InputRouter::Query(ActionId _action, ActionValue* _value) const
{
  ActionValue value;
  const bool fired = QueryAt(_action.index, value, 0);
  if (_value != nullptr)
  {
    *_value = value;
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
