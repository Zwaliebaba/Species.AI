#pragma once

#include "WindowsHeaders.h"

#include <wrl/client.h>

#include "NativeWindowHandle.h"

#include <cstdint>

namespace Neuron
{

enum class TouchKeyboardRequest : std::uint8_t
{
  None,
  Show,
  Hide
};

// ADR-181: a frame's request of the touch keyboard. It shows as a text field starts editing in touch mode, and again on
// a tap while one edits, which brings it back after its own close key; it hides as the editing ends.
[[nodiscard]] constexpr TouchKeyboardRequest TouchKeyboardRequestOf(bool _wasEditing, bool _editing, bool _touchMode, bool _tapped) noexcept
{
  if (_editing && _touchMode && (!_wasEditing || _tapped))
  {
    return TouchKeyboardRequest::Show;
  }
  return _wasEditing && !_editing ? TouchKeyboardRequest::Hide : TouchKeyboardRequest::None;
}

/// The system's touch keyboard for the window (ADR-181), asked for through the window's Windows.UI.ViewManagement input
/// pane. Windows Runtime is initialized on the main thread at the first request to show it, and only then, so a session
/// without touch never initializes it. A failure is logged once and leaves the field typed by any keyboard. Main thread
/// only.
class TouchKeyboard
{
public:
  explicit TouchKeyboard(NativeWindowHandle _window) noexcept
    : m_window(_window)
  {
  }
  TouchKeyboard(const TouchKeyboard&) = delete;
  TouchKeyboard& operator=(const TouchKeyboard&) = delete;
  ~TouchKeyboard();

  // The frame's state: whether a text field is being edited after the UI's update, whether the frame is in touch mode,
  // and whether it had a tap.
  void Follow(bool _editing, bool _touchMode, bool _tapped);

private:
  [[nodiscard]] bool Acquire();
  void Ask(bool _show);

  NativeWindowHandle m_window;
  Microsoft::WRL::ComPtr<IUnknown> m_pane; // the window's InputPane, an IInputPane2 behind IUnknown
  bool m_runtime = false;                  // RoInitialize succeeded, so RoUninitialize is owed
  bool m_unavailable = false;              // the pane could not be had, which is logged once
  bool m_refusalLogged = false;
  bool m_editing = false;
};

} // namespace Neuron
