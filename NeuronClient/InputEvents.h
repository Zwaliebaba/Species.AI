#pragma once

#include "PlatformClock.h"
#include "WindowGeometry.h"

#include <cstdint>
#include <vector>

namespace Neuron
{

// The bindable keys of the input component's table 3.2-A (INP-056). The values are the Windows virtual-key codes of
// the active layout, so the keyboard back-end maps a key message without a table; a modifier covers both its keys.
enum class KeyCode : std::uint16_t
{
  Backspace = 0x08,
  Tab = 0x09,
  Enter = 0x0D,
  Shift = 0x10,
  Control = 0x11,
  Alt = 0x12,
  Pause = 0x13,
  CapsLock = 0x14,
  Escape = 0x1B,
  Space = 0x20,
  PageUp = 0x21,
  PageDown = 0x22,
  End = 0x23,
  Home = 0x24,
  Left = 0x25,
  Up = 0x26,
  Right = 0x27,
  Down = 0x28,
  PrintScreen = 0x2C,
  Insert = 0x2D,
  Delete = 0x2E,
  Digit0 = 0x30,
  Digit1,
  Digit2,
  Digit3,
  Digit4,
  Digit5,
  Digit6,
  Digit7,
  Digit8,
  Digit9,
  A = 0x41,
  B,
  C,
  D,
  E,
  F,
  G,
  H,
  I,
  J,
  K,
  L,
  M,
  N,
  O,
  P,
  Q,
  R,
  S,
  T,
  U,
  V,
  W,
  X,
  Y,
  Z,
  Meta = 0x5B,
  Application = 0x5D,
  Num0 = 0x60,
  Num1,
  Num2,
  Num3,
  Num4,
  Num5,
  Num6,
  Num7,
  Num8,
  Num9,
  NumMultiply = 0x6A,
  NumPlus = 0x6B,
  NumSeparator = 0x6C,
  NumMinus = 0x6D,
  NumDecimal = 0x6E,
  NumDivide = 0x6F,
  F1 = 0x70,
  F2,
  F3,
  F4,
  F5,
  F6,
  F7,
  F8,
  F9,
  F10,
  F11,
  F12,
  F13,
  F14,
  F15,
  F16,
  F17,
  F18,
  F19,
  F20,
  F21,
  F22,
  F23,
  F24,
  NumLock = 0x90,
  ScrollLock = 0x91,
  Semicolon = 0xBA,
  Equals = 0xBB,
  Comma = 0xBC,
  Minus = 0xBD,
  Period = 0xBE,
  Slash = 0xBF,
  Backquote = 0xC0,
  BracketLeft = 0xDB,
  Backslash = 0xDC,
  BracketRight = 0xDD,
  Quote = 0xDE,
  Oem8 = 0xDF
};

inline constexpr std::uint16_t KEY_CODE_COUNT = 256; // every key code is below it

// The mouse controls of table 3.2-B (INP-055).
enum class MouseControl : std::uint8_t
{
  Primary,
  Secondary,
  Middle,
  Motion,
  RawMotion,
  Wheel
};

inline constexpr std::uint8_t MOUSE_BUTTON_COUNT = 3; // primary, secondary and middle

enum class InputDevice : std::uint8_t
{
  Key,
  Mouse
};

/// One event of a frame's ordered queue (ADR-029, INP-115): a press or a release of a key or mouse button, with the
/// operating system's auto-repeat flag on keys, or a movement or wheel sample. A pointer movement also carries the
/// pointer's client position after it.
struct InputEvent
{
  enum class Kind : std::uint8_t
  {
    Press,
    Release,
    Axis
  };

  Kind kind = Kind::Press;
  InputDevice device = InputDevice::Key;
  std::uint16_t control = 0; // a KeyCode or a MouseControl
  bool osRepeat = false;     // INP-053: never a new press
  Nanoseconds time{};        // the platform time at which the pump retrieved it (PLT-008)
  float x = 0.0f;            // an axis sample's delta: client pixels for motion, notches for the wheel (INP-183)
  float y = 0.0f;
  PointPx pointer; // a motion sample's pointer position after it, in client pixels
};

/// A device back-end of the input component (INP-105): every event since the previous collection, in order, and on a
/// focus loss a release for everything it holds pressed (INP-111).
class DeviceBackEnd
{
public:
  virtual ~DeviceBackEnd() = default;
  virtual void Collect(std::vector<InputEvent>& _queue) = 0;
  virtual void ReleaseAll(std::vector<InputEvent>& _queue) = 0;
};

/// A character of text input (UI-007, INP-112): a Unicode scalar value, with the modifiers held when it arrived. The
/// editing keys Backspace, Enter and Escape arrive as characters too (UI-066).
struct TextCharacter
{
  char32_t codePoint = 0;
  bool shift = false;
  bool control = false;
  bool alt = false;
};

/// A source of text input (INP-112, ADR-154): the characters the window received since the previous call, in order. They
/// never enter the event queue.
class TextSource
{
public:
  virtual ~TextSource() = default;
  virtual void TakeText(std::vector<TextCharacter>& _out) = 0;
};

/// The window's pointer services that input uses (PLT-068 items 3 and 4, INP-158, INP-159).
class PointerHost
{
public:
  virtual ~PointerHost() = default;
  // Moves the system pointer to the client position without producing a movement event.
  virtual void WarpPointer(PointPx _clientPosition) = 0;
  virtual void ConfinePointer(bool _on) = 0;
};

} // namespace Neuron
