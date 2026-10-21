#pragma once

#include "Development.h"
#include "InputRouter.h"

#include <cstdint>
#include <vector>

namespace Species
{

// The client's actions, in the order of the router's action ids (INP-067). M1's set is compiled in (O75, ADR-119):
// the camera's rows and the battle verbs of reference/06 table 2.1-A, the M1 stand-in for the window mode, and in Debug
// builds the development layer's entries that M1 binds (O76).
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
  // M1 task T17 (ADR-124): the battle verbs of table 3.6-A rows 19 to 24 and 40 to 45.
  SelectPick,
  SelectPickIdle,
  SelectClear,
  SelectCircleBegin,
  SelectCircleGrow,
  SelectCircleEnd,
  OrderCrowd,
  OrderStraightRoute,
  OrderPromote,
  OrderSpecial,
  OrderMoveModifier,
  OrderDemote,
  WindowToggleMode, // O65, O76: Alt+Enter until the screen options exist (RND-442)
#if ENGINE_DEVELOPMENT
  DebugRemoveDevice,          // O65, O76: Ctrl+Shift+F11 until the debug-command window exists (DBG-117 row 16)
  DebugOverlay,               // DBG-020: F5 shows and hides the development overlay (table 3.11-A row 6)
  DebugRestartSandbox,        // ADR-120: Ctrl+Shift+F7 restarts the sandbox from its seed, until the debug-command window
  DebugRestartSandboxNewSeed, // ADR-120: Ctrl+Shift+F8 restarts it from a new seed
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
