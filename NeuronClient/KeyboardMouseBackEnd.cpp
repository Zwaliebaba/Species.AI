#include "pch.h"

#include "KeyboardMouseBackEnd.h"

#include <algorithm>
#include <utility>

namespace Neuron
{

namespace
{

constexpr std::intptr_t PREVIOUS_STATE_BIT = std::intptr_t{1} << 30; // a key message's auto-repeat mark
constexpr std::intptr_t EXTENDED_KEY_BIT = std::intptr_t{1} << 24;   // the right control and alt keys
constexpr std::intptr_t RIGHT_SHIFT_SCAN_CODE = 0x36;
constexpr std::uintptr_t VIRTUAL_KEY_PACKET = 0xE7; // a character injected as a key, not a key of table 3.2-A
constexpr float WHEEL_UNITS_PER_NOTCH = 120.0f;     // inp.mouse.wheel_units_per_notch (INP-110 item 3)
// UTF-16's surrogate halves (ADR-154).
constexpr char16_t HIGH_SURROGATE_FIRST = 0xD800;
constexpr char16_t HIGH_SURROGATE_LAST = 0xDBFF;
constexpr char16_t LOW_SURROGATE_FIRST = 0xDC00;
constexpr char16_t LOW_SURROGATE_LAST = 0xDFFF;
constexpr char32_t SURROGATE_BASE = 0x10000;
constexpr int SURROGATE_SHIFT = 10;

// The modifier of a key message's virtual-key code: 0 shift, 1 control, 2 alt; none for every other key.
int ModifierOf(std::uintptr_t _virtualKey) noexcept
{
  switch (_virtualKey)
  {
  case VK_SHIFT:
    return 0;
  case VK_CONTROL:
    return 1;
  case VK_MENU:
    return 2;
  default:
    return -1;
  }
}

std::int32_t SignedLow(std::intptr_t _value) noexcept
{
  return static_cast<std::int16_t>(static_cast<std::uint16_t>(_value & 0xFFFF));
}

std::int32_t SignedHigh(std::intptr_t _value) noexcept
{
  return static_cast<std::int16_t>(static_cast<std::uint16_t>((_value >> 16) & 0xFFFF));
}

} // namespace

std::optional<char32_t> JoinUtf16(char16_t& _pendingHigh, char16_t _unit) noexcept
{
  if (_unit >= HIGH_SURROGATE_FIRST && _unit <= HIGH_SURROGATE_LAST)
  {
    _pendingHigh = _unit;
    return std::nullopt;
  }
  const char16_t high = std::exchange(_pendingHigh, char16_t{0});
  if (_unit >= LOW_SURROGATE_FIRST && _unit <= LOW_SURROGATE_LAST)
  {
    if (high == 0)
    {
      return std::nullopt;
    }
    return SURROGATE_BASE +
           ((static_cast<char32_t>(high - HIGH_SURROGATE_FIRST) << SURROGATE_SHIFT) | static_cast<char32_t>(_unit - LOW_SURROGATE_FIRST));
  }
  return char32_t{_unit};
}

KeyboardMouseBackEnd::KeyboardMouseBackEnd(AppWindow& _window) noexcept
  : m_window(_window)
{
}

void KeyboardMouseBackEnd::OnWindowMessage(const WindowMessage& _message)
{
  m_lastTime = _message.time;
  switch (_message.id)
  {
  case WM_KEYDOWN:
  case WM_SYSKEYDOWN:
    Key(_message, true);
    break;
  case WM_KEYUP:
  case WM_SYSKEYUP:
    Key(_message, false);
    break;
  case WM_LBUTTONDOWN:
  case WM_LBUTTONDBLCLK:
    Button(_message, MouseControl::Primary, true);
    break;
  case WM_LBUTTONUP:
    Button(_message, MouseControl::Primary, false);
    break;
  case WM_RBUTTONDOWN:
  case WM_RBUTTONDBLCLK:
    Button(_message, MouseControl::Secondary, true);
    break;
  case WM_RBUTTONUP:
    Button(_message, MouseControl::Secondary, false);
    break;
  case WM_MBUTTONDOWN:
  case WM_MBUTTONDBLCLK:
    Button(_message, MouseControl::Middle, true);
    break;
  case WM_MBUTTONUP:
    Button(_message, MouseControl::Middle, false);
    break;
  case WM_MOUSEMOVE:
  {
    // INP-110 item 4: the movement in client pixels. A warp's own movement message arrives at the position already
    // held, so it makes no event (PLT-068 item 4).
    const PointPx position{SignedLow(_message.lParam), SignedHigh(_message.lParam)};
    if (m_pointerKnown && position.x == m_pointer.x && position.y == m_pointer.y)
    {
      break;
    }
    InputEvent event;
    event.kind = InputEvent::Kind::Axis;
    event.device = InputDevice::Mouse;
    event.control = static_cast<std::uint16_t>(MouseControl::Motion);
    event.time = _message.time;
    event.x = m_pointerKnown ? static_cast<float>(position.x - m_pointer.x) : 0.0f;
    event.y = m_pointerKnown ? static_cast<float>(position.y - m_pointer.y) : 0.0f;
    event.pointer = position;
    m_pending.push_back(event);
    m_pointer = position;
    m_pointerKnown = true;
    break;
  }
  case WM_MOUSEWHEEL:
  {
    // INP-110 item 3: the fine wheel units as notches, unrounded; forward is positive.
    InputEvent event;
    event.kind = InputEvent::Kind::Axis;
    event.device = InputDevice::Mouse;
    event.control = static_cast<std::uint16_t>(MouseControl::Wheel);
    event.time = _message.time;
    event.x = static_cast<float>(SignedHigh(static_cast<std::intptr_t>(_message.wParam))) / WHEEL_UNITS_PER_NOTCH;
    m_pending.push_back(event);
    break;
  }
  case WM_MOUSEHWHEEL:
  {
    // O108 (ADR-180): the horizontal wheel, and a touchpad's two-finger slide sideways, as notches; right is positive.
    InputEvent event;
    event.kind = InputEvent::Kind::Axis;
    event.device = InputDevice::Mouse;
    event.control = static_cast<std::uint16_t>(MouseControl::WheelHorizontal);
    event.time = _message.time;
    event.x = static_cast<float>(SignedHigh(static_cast<std::intptr_t>(_message.wParam))) / WHEEL_UNITS_PER_NOTCH;
    m_pending.push_back(event);
    break;
  }
  case WM_CHAR:
    // UI-007, ADR-154: UTF-16 only at the Win32 boundary; each character with the modifiers held as it arrives.
    if (const std::optional<char32_t> codePoint = JoinUtf16(m_highSurrogate, static_cast<char16_t>(_message.wParam & 0xFFFF)))
    {
      m_text.push_back({*codePoint, m_modifierSides[0] != 0, m_modifierSides[1] != 0, m_modifierSides[2] != 0});
    }
    break;
  case WM_POINTERDOWN:
  case WM_POINTERUPDATE:
  case WM_POINTERUP:
  case WM_POINTERCAPTURECHANGED:
    Contact(_message);
    break;
  case WM_CLOSE:
    m_closeRequested = true; // PLT-009: action.app.quit, which no binding raises (INP-076)
    break;
  default:
    break; // input-method messages and raw input have no consumer at M2 (ADR-119, ADR-154)
  }
}

void KeyboardMouseBackEnd::TakeText(std::vector<TextCharacter>& _out)
{
  _out.insert(_out.end(), m_text.begin(), m_text.end());
  m_text.clear();
}

void KeyboardMouseBackEnd::Key(const WindowMessage& _message, bool _press)
{
  const std::uintptr_t virtualKey = _message.wParam & 0xFF;
  if (virtualKey == 0 || virtualKey == VIRTUAL_KEY_PACKET || _message.wParam >= KEY_CODE_COUNT)
  {
    return;
  }
  bool repeat = _press && (_message.lParam & PREVIOUS_STATE_BIT) != 0;
  if (const int modifier = ModifierOf(virtualKey); modifier >= 0)
  {
    // INP-056: a modifier covers both its keys. The second key of a pair adds no press, and the first released
    // releases nothing while the other is held.
    const bool right =
      virtualKey == VK_SHIFT ? ((_message.lParam >> 16) & 0xFF) == RIGHT_SHIFT_SCAN_CODE : (_message.lParam & EXTENDED_KEY_BIT) != 0;
    const auto side = static_cast<std::uint8_t>(right ? 2 : 1);
    std::uint8_t& sides = m_modifierSides[static_cast<std::size_t>(modifier)];
    const bool heldBefore = sides != 0;
    sides = _press ? static_cast<std::uint8_t>(sides | side) : static_cast<std::uint8_t>(sides & ~side);
    if (!_press && sides != 0)
    {
      return;
    }
    repeat = repeat || (_press && heldBefore);
  }
  m_keysHeld[virtualKey] = _press;
  Push(_press ? InputEvent::Kind::Press : InputEvent::Kind::Release, InputDevice::Key, static_cast<std::uint16_t>(virtualKey),
       _message.time, repeat);
}

void KeyboardMouseBackEnd::Button(const WindowMessage& _message, MouseControl _button, bool _press)
{
  m_buttonsHeld[static_cast<std::size_t>(_button)] = _press;
  Push(_press ? InputEvent::Kind::Press : InputEvent::Kind::Release, InputDevice::Mouse, static_cast<std::uint16_t>(_button),
       _message.time);
}

void KeyboardMouseBackEnd::Contact(const WindowMessage& _message)
{
  // ADR-181: a touch or pen pointer holds a contact slot from its down to its up. A capture change or a canceled pointer
  // ends its contact as a cancel; the capture change's lParam is a window, not a position.
  const auto pointerId = static_cast<std::uint32_t>(GET_POINTERID_WPARAM(_message.wParam));
  const auto held = std::ranges::find_if(m_contacts, [pointerId](const std::optional<std::uint32_t>& _id) { return _id == pointerId; });
  std::optional<std::size_t> slot;
  if (held != m_contacts.end())
  {
    slot = static_cast<std::size_t>(held - m_contacts.begin());
  }
  if (_message.id == WM_POINTERCAPTURECHANGED || IS_POINTER_CANCELED_WPARAM(_message.wParam))
  {
    if (slot.has_value())
    {
      PushContact(InputEvent::Kind::Cancel, *slot, {}, _message.time);
      m_contacts[*slot].reset();
    }
    return;
  }
  const PointPx client = m_window.ClientFromScreen({SignedLow(_message.lParam), SignedHigh(_message.lParam)});
  if (_message.id == WM_POINTERDOWN)
  {
    const auto free = std::ranges::find_if(m_contacts, [](const std::optional<std::uint32_t>& _id) { return !_id.has_value(); });
    if (slot.has_value() || free == m_contacts.end())
    {
      return; // a pointer already down, or more contacts than the recognizer tracks
    }
    *free = pointerId;
    PushContact(InputEvent::Kind::Press, static_cast<std::size_t>(free - m_contacts.begin()), client, _message.time);
    return;
  }
  if (!slot.has_value())
  {
    return;
  }
  if (_message.id == WM_POINTERUP)
  {
    PushContact(InputEvent::Kind::Release, *slot, client, _message.time);
    m_contacts[*slot].reset();
  }
  else if (IS_POINTER_INCONTACT_WPARAM(_message.wParam))
  {
    PushContact(InputEvent::Kind::Axis, *slot, client, _message.time);
  }
}

void KeyboardMouseBackEnd::PushContact(InputEvent::Kind _kind, std::size_t _slot, PointPx _point, Nanoseconds _time)
{
  InputEvent event;
  event.kind = _kind;
  event.device = InputDevice::Touch;
  event.control = static_cast<std::uint16_t>(_slot);
  event.time = _time;
  event.pointer = _point;
  m_pending.push_back(event);
}

void KeyboardMouseBackEnd::Push(InputEvent::Kind _kind, InputDevice _device, std::uint16_t _control, Nanoseconds _time, bool _repeat)
{
  InputEvent event;
  event.kind = _kind;
  event.device = _device;
  event.control = _control;
  event.osRepeat = _repeat;
  event.time = _time;
  m_pending.push_back(event);
}

void KeyboardMouseBackEnd::Collect(std::vector<InputEvent>& _queue)
{
  _queue.insert(_queue.end(), m_pending.begin(), m_pending.end());
  m_pending.clear();
}

void KeyboardMouseBackEnd::ReleaseAll(std::vector<InputEvent>& _queue)
{
  // INP-111: a release for every key and button held, Alt held through Alt+Tab among them.
  for (std::size_t key = 0; key < m_keysHeld.size(); ++key)
  {
    if (std::exchange(m_keysHeld[key], false))
    {
      InputEvent event;
      event.kind = InputEvent::Kind::Release;
      event.control = static_cast<std::uint16_t>(key);
      event.time = m_lastTime;
      _queue.push_back(event);
    }
  }
  m_modifierSides = {};
  for (std::size_t button = 0; button < m_buttonsHeld.size(); ++button)
  {
    if (std::exchange(m_buttonsHeld[button], false))
    {
      InputEvent event;
      event.kind = InputEvent::Kind::Release;
      event.device = InputDevice::Mouse;
      event.control = static_cast<std::uint16_t>(button);
      event.time = m_lastTime;
      _queue.push_back(event);
    }
  }
  // ADR-181: every touch contact down ends as a cancel.
  for (std::size_t slot = 0; slot < m_contacts.size(); ++slot)
  {
    if (std::exchange(m_contacts[slot], std::nullopt).has_value())
    {
      InputEvent event;
      event.kind = InputEvent::Kind::Cancel;
      event.device = InputDevice::Touch;
      event.control = static_cast<std::uint16_t>(slot);
      event.time = m_lastTime;
      _queue.push_back(event);
    }
  }
}

void KeyboardMouseBackEnd::WarpPointer(PointPx _clientPosition)
{
  m_pointer = _clientPosition;
  m_pointerKnown = true;
  m_window.WarpPointer(_clientPosition);
}

void KeyboardMouseBackEnd::ConfinePointer(bool _on)
{
  m_window.ConfinePointer(_on);
}

bool KeyboardMouseBackEnd::TakeCloseRequest() noexcept
{
  return std::exchange(m_closeRequested, false);
}

} // namespace Neuron
