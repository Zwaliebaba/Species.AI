#pragma once

#include "AppWindow.h"
#include "InputEvents.h"

#include <array>
#include <cstdint>
#include <optional>
#include <vector>

namespace Neuron
{

// UI-007 (ADR-154): a UTF-16 code unit of a character message as a Unicode scalar value. A high surrogate is held in
// _pendingHigh until its low half arrives; a low half with no high one before it gives none, and so does a high half
// that a unit other than a low half follows.
[[nodiscard]] std::optional<char32_t> JoinUtf16(char16_t& _pendingHigh, char16_t _unit) noexcept;

/// The keyboard and mouse back-end (ADR-028, INP-105, INP-108 to INP-111) of M1: the window's key, button, pointer and
/// wheel messages as the router's events, in arrival order, with their pump timestamps. Each modifier covers both its
/// keys, so it is released only when neither is held. It is also the router's pointer host, through the window, and it
/// takes the window's close request for the quit (PLT-009, INP-076), and, from M2 task T18, its text source: each
/// character message as a Unicode scalar value, a surrogate pair joined (UI-007, ADR-154). Main thread only; the window
/// outlives it.
class KeyboardMouseBackEnd final : public WindowInputSink, public DeviceBackEnd, public PointerHost, public TextSource
{
public:
  explicit KeyboardMouseBackEnd(AppWindow& _window) noexcept;

  void OnWindowMessage(const WindowMessage& _message) override;

  void Collect(std::vector<InputEvent>& _queue) override;
  void ReleaseAll(std::vector<InputEvent>& _queue) override;

  void WarpPointer(PointPx _clientPosition) override;
  void ConfinePointer(bool _on) override;

  void TakeText(std::vector<TextCharacter>& _out) override;

  // PLT-009: the window was asked to close since the previous call.
  [[nodiscard]] bool TakeCloseRequest() noexcept;
  // The quit of an end-of-session notice (PLT-042), which arrives as a window event rather than a message.
  void RequestClose() noexcept
  {
    m_closeRequested = true;
  }

private:
  void Key(const WindowMessage& _message, bool _press);
  void Button(const WindowMessage& _message, MouseControl _button, bool _press);
  void Push(InputEvent::Kind _kind, InputDevice _device, std::uint16_t _control, Nanoseconds _time, bool _repeat = false);

  AppWindow& m_window;
  std::vector<InputEvent> m_pending;
  std::vector<TextCharacter> m_text;
  char16_t m_highSurrogate = 0; // the first half of a surrogate pair, until its second arrives (JoinUtf16)
  std::array<bool, KEY_CODE_COUNT> m_keysHeld{};
  std::array<bool, MOUSE_BUTTON_COUNT> m_buttonsHeld{};
  // The left and right keys of shift, control and alt, by modifier: bit 0 the left key, bit 1 the right one.
  std::array<std::uint8_t, 3> m_modifierSides{};
  PointPx m_pointer;
  bool m_pointerKnown = false;
  Nanoseconds m_lastTime{};
  bool m_closeRequested = false;
};

} // namespace Neuron
