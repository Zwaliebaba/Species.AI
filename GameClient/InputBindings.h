#pragma once

#include "Development.h"
#include "InputRouter.h"
#include "SettingsStore.h"

#include <cstdint>
#include <string>
#include <string_view>
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
  CameraLook, // O107 (ADR-179): table 3.6-A row 66, the middle button held turns the camera towards the pointer
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
  // M2 task T14 (ADR-149): the UI's navigation of table 3.6-B rows 1 to 4, 5 and 7 (UI-203, UI-204), and the pointer's
  // press and hold of table 3.6-A rows 60 and 61, whose release is the hold's end (UI-035).
  MenuLeft,
  MenuRight,
  MenuUp,
  MenuDown,
  MenuActivate,
  MenuClose,
  UiPointerDown,
  UiPointerHeld,
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
// with the US and UK key positions (ADR-119). In a Debug build the development layer's entries follow (INP-094,
// INP-095).
[[nodiscard]] std::vector<Neuron::ActionBindings> DefaultBindings();

// INP-164: the active keyboard layout: the input.keyboard_layout setting when it is set, else the system's layout tag.
[[nodiscard]] std::string ActiveLayoutTag(const Neuron::SettingsStore& _settings, std::string_view _systemLayoutTag);
// INP-165, table 4.13-B: the layout layer of a layout tag, the camera keys' primary bindings on a German or a French
// keyboard; none for any other tag, which keeps the default set (INP-164).
[[nodiscard]] std::vector<Neuron::BindingReplacement> LayoutLayer(std::string_view _layoutTag);
// INP-070, INP-171 (ADR-151, O94): the user layer, each binding.<action id> setting parsed from its canonical JSON; an
// entry that does not parse is logged with its key and skipped, and the router checks the rest when it rebuilds.
[[nodiscard]] std::vector<Neuron::BindingReplacement> UserLayer(const Neuron::SettingsStore& _settings);
// INP-162: the replacing layers in order, the active layout's, then the user's.
[[nodiscard]] std::vector<Neuron::BindingReplacement> BindingLayers(const Neuron::SettingsStore& _settings,
                                                                    std::string_view _systemLayoutTag);
// INP-060, INP-142: what a setting node may name and read: a key of the client's settings table, and its value as a
// number, a boolean as 1 or 0.
[[nodiscard]] Neuron::SettingAccess SettingAccessOf(const Neuron::SettingsStore& _settings);

} // namespace Species
