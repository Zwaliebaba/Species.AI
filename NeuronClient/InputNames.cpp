#include "pch.h"

#include "InputNames.h"

#include <algorithm>
#include <array>

namespace Neuron
{

namespace
{

struct KeyName
{
  KeyCode key;
  std::string_view name;
};

// Table 3.2-A, in its order.
constexpr std::array KEY_NAMES{KeyName{KeyCode::A, "a"},
                               KeyName{KeyCode::B, "b"},
                               KeyName{KeyCode::C, "c"},
                               KeyName{KeyCode::D, "d"},
                               KeyName{KeyCode::E, "e"},
                               KeyName{KeyCode::F, "f"},
                               KeyName{KeyCode::G, "g"},
                               KeyName{KeyCode::H, "h"},
                               KeyName{KeyCode::I, "i"},
                               KeyName{KeyCode::J, "j"},
                               KeyName{KeyCode::K, "k"},
                               KeyName{KeyCode::L, "l"},
                               KeyName{KeyCode::M, "m"},
                               KeyName{KeyCode::N, "n"},
                               KeyName{KeyCode::O, "o"},
                               KeyName{KeyCode::P, "p"},
                               KeyName{KeyCode::Q, "q"},
                               KeyName{KeyCode::R, "r"},
                               KeyName{KeyCode::S, "s"},
                               KeyName{KeyCode::T, "t"},
                               KeyName{KeyCode::U, "u"},
                               KeyName{KeyCode::V, "v"},
                               KeyName{KeyCode::W, "w"},
                               KeyName{KeyCode::X, "x"},
                               KeyName{KeyCode::Y, "y"},
                               KeyName{KeyCode::Z, "z"},
                               KeyName{KeyCode::Digit0, "0"},
                               KeyName{KeyCode::Digit1, "1"},
                               KeyName{KeyCode::Digit2, "2"},
                               KeyName{KeyCode::Digit3, "3"},
                               KeyName{KeyCode::Digit4, "4"},
                               KeyName{KeyCode::Digit5, "5"},
                               KeyName{KeyCode::Digit6, "6"},
                               KeyName{KeyCode::Digit7, "7"},
                               KeyName{KeyCode::Digit8, "8"},
                               KeyName{KeyCode::Digit9, "9"},
                               KeyName{KeyCode::F1, "f1"},
                               KeyName{KeyCode::F2, "f2"},
                               KeyName{KeyCode::F3, "f3"},
                               KeyName{KeyCode::F4, "f4"},
                               KeyName{KeyCode::F5, "f5"},
                               KeyName{KeyCode::F6, "f6"},
                               KeyName{KeyCode::F7, "f7"},
                               KeyName{KeyCode::F8, "f8"},
                               KeyName{KeyCode::F9, "f9"},
                               KeyName{KeyCode::F10, "f10"},
                               KeyName{KeyCode::F11, "f11"},
                               KeyName{KeyCode::F12, "f12"},
                               KeyName{KeyCode::F13, "f13"},
                               KeyName{KeyCode::F14, "f14"},
                               KeyName{KeyCode::F15, "f15"},
                               KeyName{KeyCode::F16, "f16"},
                               KeyName{KeyCode::F17, "f17"},
                               KeyName{KeyCode::F18, "f18"},
                               KeyName{KeyCode::F19, "f19"},
                               KeyName{KeyCode::F20, "f20"},
                               KeyName{KeyCode::F21, "f21"},
                               KeyName{KeyCode::F22, "f22"},
                               KeyName{KeyCode::F23, "f23"},
                               KeyName{KeyCode::F24, "f24"},
                               KeyName{KeyCode::Backspace, "backspace"},
                               KeyName{KeyCode::Tab, "tab"},
                               KeyName{KeyCode::Enter, "enter"},
                               KeyName{KeyCode::Space, "space"},
                               KeyName{KeyCode::Escape, "escape"},
                               KeyName{KeyCode::Insert, "insert"},
                               KeyName{KeyCode::Delete, "delete"},
                               KeyName{KeyCode::Home, "home"},
                               KeyName{KeyCode::End, "end"},
                               KeyName{KeyCode::PageUp, "page_up"},
                               KeyName{KeyCode::PageDown, "page_down"},
                               KeyName{KeyCode::Left, "left"},
                               KeyName{KeyCode::Up, "up"},
                               KeyName{KeyCode::Right, "right"},
                               KeyName{KeyCode::Down, "down"},
                               KeyName{KeyCode::Shift, "shift"},
                               KeyName{KeyCode::Control, "control"},
                               KeyName{KeyCode::Alt, "alt"},
                               KeyName{KeyCode::Meta, "meta"},
                               KeyName{KeyCode::CapsLock, "caps_lock"},
                               KeyName{KeyCode::NumLock, "num_lock"},
                               KeyName{KeyCode::ScrollLock, "scroll_lock"},
                               KeyName{KeyCode::Pause, "pause"},
                               KeyName{KeyCode::PrintScreen, "print_screen"},
                               KeyName{KeyCode::Application, "application"},
                               KeyName{KeyCode::Num0, "num_0"},
                               KeyName{KeyCode::Num1, "num_1"},
                               KeyName{KeyCode::Num2, "num_2"},
                               KeyName{KeyCode::Num3, "num_3"},
                               KeyName{KeyCode::Num4, "num_4"},
                               KeyName{KeyCode::Num5, "num_5"},
                               KeyName{KeyCode::Num6, "num_6"},
                               KeyName{KeyCode::Num7, "num_7"},
                               KeyName{KeyCode::Num8, "num_8"},
                               KeyName{KeyCode::Num9, "num_9"},
                               KeyName{KeyCode::NumMultiply, "num_multiply"},
                               KeyName{KeyCode::NumPlus, "num_plus"},
                               KeyName{KeyCode::NumSeparator, "num_separator"},
                               KeyName{KeyCode::NumMinus, "num_minus"},
                               KeyName{KeyCode::NumDecimal, "num_decimal"},
                               KeyName{KeyCode::NumDivide, "num_divide"},
                               KeyName{KeyCode::Semicolon, "semicolon"},
                               KeyName{KeyCode::Equals, "equals"},
                               KeyName{KeyCode::Comma, "comma"},
                               KeyName{KeyCode::Minus, "minus"},
                               KeyName{KeyCode::Period, "period"},
                               KeyName{KeyCode::Slash, "slash"},
                               KeyName{KeyCode::Backquote, "backquote"},
                               KeyName{KeyCode::BracketLeft, "bracket_left"},
                               KeyName{KeyCode::Backslash, "backslash"},
                               KeyName{KeyCode::BracketRight, "bracket_right"},
                               KeyName{KeyCode::Quote, "quote"},
                               KeyName{KeyCode::Oem8, "oem_8"}};

struct MouseName
{
  MouseControl control;
  std::string_view name;
};

// Table 3.2-B.
constexpr std::array MOUSE_NAMES{MouseName{MouseControl::Primary, "primary"},      MouseName{MouseControl::Secondary, "secondary"},
                                 MouseName{MouseControl::Middle, "middle"},        MouseName{MouseControl::Motion, "motion"},
                                 MouseName{MouseControl::RawMotion, "raw_motion"}, MouseName{MouseControl::Wheel, "wheel"}};

struct ConditionName
{
  Condition condition;
  std::string_view name;
};

// INP-051, in its order.
constexpr std::array CONDITION_NAMES{ConditionName{Condition::Down, "down"},         ConditionName{Condition::Up, "up"},
                                     ConditionName{Condition::Held, "held"},         ConditionName{Condition::Repeat, "repeat"},
                                     ConditionName{Condition::Position, "position"}, ConditionName{Condition::Move, "move"},
                                     ConditionName{Condition::Release, "release"},   ConditionName{Condition::Zero, "zero"},
                                     ConditionName{Condition::Read, "read"}};

bool EqualsIgnoringCase(std::string_view _left, std::string_view _right) noexcept
{
  const auto lower = [](char _c) { return _c >= 'A' && _c <= 'Z' ? static_cast<char>(_c - 'A' + 'a') : _c; };
  return _left.size() == _right.size() && std::ranges::equal(_left, _right, {}, lower, lower);
}

} // namespace

std::optional<KeyCode> KeyOfName(std::string_view _name) noexcept
{
  const auto found = std::ranges::find_if(KEY_NAMES, [_name](const KeyName& _entry) { return EqualsIgnoringCase(_entry.name, _name); });
  return found == KEY_NAMES.end() ? std::nullopt : std::optional<KeyCode>(found->key);
}

std::string_view NameOfKey(KeyCode _key) noexcept
{
  const auto found = std::ranges::find(KEY_NAMES, _key, &KeyName::key);
  return found == KEY_NAMES.end() ? std::string_view{} : found->name;
}

std::optional<MouseControl> MouseControlOfName(std::string_view _name) noexcept
{
  const auto found = std::ranges::find_if(MOUSE_NAMES, [_name](const MouseName& _entry) { return EqualsIgnoringCase(_entry.name, _name); });
  return found == MOUSE_NAMES.end() ? std::nullopt : std::optional<MouseControl>(found->control);
}

std::string_view NameOfMouseControl(MouseControl _control) noexcept
{
  const auto found = std::ranges::find(MOUSE_NAMES, _control, &MouseName::control);
  return found == MOUSE_NAMES.end() ? std::string_view{} : found->name;
}

std::optional<Condition> ConditionOfName(std::string_view _name) noexcept
{
  const auto found =
    std::ranges::find_if(CONDITION_NAMES, [_name](const ConditionName& _entry) { return EqualsIgnoringCase(_entry.name, _name); });
  return found == CONDITION_NAMES.end() ? std::nullopt : std::optional<Condition>(found->condition);
}

std::string_view NameOfCondition(Condition _condition) noexcept
{
  const auto found = std::ranges::find(CONDITION_NAMES, _condition, &ConditionName::condition);
  return found == CONDITION_NAMES.end() ? std::string_view{} : found->name;
}

} // namespace Neuron
