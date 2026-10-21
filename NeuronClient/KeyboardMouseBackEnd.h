#pragma once

#include "AppWindow.h"
#include "InputEvents.h"

#include <array>
#include <cstdint>
#include <vector>

namespace Neuron
{

/// The keyboard and mouse back-end (ADR-028, INP-105, INP-108 to INP-111) of M1: the window's key, button, pointer and
/// wheel messages as the router's events, in arrival order, with their pump timestamps. Each modifier covers both its
/// keys, so it is released only when neither is held. It is also the router's pointer host, through the window, and it
/// takes the window's close request for the quit (PLT-009, INP-076). Main thread only; the window outlives it.
class KeyboardMouseBackEnd final : public WindowInputSink, public DeviceBackEnd, public PointerHost
{
public:
  explicit KeyboardMouseBackEnd(AppWindow& _window) noexcept;

  void OnWindowMessage(const WindowMessage& _message) override;

  void Collect(std::vector<InputEvent>& _queue) override;
  void ReleaseAll(std::vector<InputEvent>& _queue) override;

  void WarpPointer(PointPx _clientPosition) override;
  void ConfinePointer(bool _on) override;

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
