#pragma once

#include "Development.h"
#include "InputRouter.h"

#include <cstdint>
#include <vector>

namespace Species
{

// The client's actions, in the order of the router's action ids (INP-067). M1's set is compiled in (O75, ADR-119):
// the camera's rows of reference/06 table 2.1-A, the M1 stand-in for the window mode, and in Debug builds the
// development layer's entries that M1 binds (O76).
enum class GameAction : std::uint16_t
{
  CameraPanLeft,
  CameraPanRight,
  CameraPanForward,
  CameraPanBack,
  CameraFastPan,
  CameraRaise,
  CameraLower,
  CameraHeightWheel,
  CameraAim,
  CameraZoom,
  CameraLookAround,
  WindowToggleMode, // O65, O76: Alt+Enter until the screen options exist (RND-442)
#if ENGINE_DEVELOPMENT
  DebugRemoveDevice, // O65, O76: Ctrl+Shift+F11 until the debug-command window exists (DBG-117 row 16)
#endif
  Count
};

[[nodiscard]] constexpr Neuron::ActionId ActionOf(GameAction _action) noexcept
{
  return {static_cast<std::uint16_t>(_action)};
}

// The default binding set of the input component's table 3.6-A for these actions, keyboard and mouse only (MVP-100),
// with the US and UK key positions: no layout layer and no user layer at M1 (ADR-119). In a Debug build the
// development layer's entries follow (INP-094, INP-095).
[[nodiscard]] std::vector<Neuron::ActionBindings> DefaultBindings();

} // namespace Species
