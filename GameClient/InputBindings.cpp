#include "pch.h"

#include "BindingNotation.h"
#include "ClientSettings.h"
#include "InputBindings.h"
#include "Log.h"

#include <array>
#include <cstdint>
#include <expected>
#include <format>
#include <optional>
#include <utility>
#include <variant>

namespace Species
{

std::vector<Neuron::ActionBindings> DefaultBindings()
{
  using enum Neuron::Condition;
  using Neuron::KeyBinding;
  using Neuron::KeyCode;
  using Neuron::MouseBinding;
  using Neuron::MouseControl;
  std::vector<Neuron::ActionBindings> actions(static_cast<std::size_t>(GameAction::Count));
  const auto bind =
    [&actions](GameAction _action, std::string_view _id, Neuron::ValueKind _kind, std::vector<Neuron::InputBinding> _bindings)
  { actions[static_cast<std::size_t>(_action)] = {_id, _kind, std::move(_bindings)}; };
  // Table 3.6-A rows 1 to 4, 6 to 10, 12 and 13 (reference/06 rows A1 to A6).
  bind(GameAction::CameraPanLeft, "action.camera.pan_left", Neuron::ValueKind::Boolean,
       {KeyBinding(KeyCode::A, Held), KeyBinding(KeyCode::Left, Held)});
  bind(GameAction::CameraPanRight, "action.camera.pan_right", Neuron::ValueKind::Boolean,
       {KeyBinding(KeyCode::D, Held), KeyBinding(KeyCode::Right, Held)});
  bind(GameAction::CameraPanForward, "action.camera.pan_forward", Neuron::ValueKind::Boolean,
       {KeyBinding(KeyCode::W, Held), KeyBinding(KeyCode::Up, Held)});
  bind(GameAction::CameraPanBack, "action.camera.pan_back", Neuron::ValueKind::Boolean,
       {KeyBinding(KeyCode::S, Held), KeyBinding(KeyCode::Down, Held)});
  bind(GameAction::CameraFastPan, "action.camera.fast_pan", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Shift, Held)});
  bind(GameAction::CameraRaise, "action.camera.raise", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Q, Held)});
  bind(GameAction::CameraLower, "action.camera.lower", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::E, Held)});
  bind(GameAction::CameraHeightWheel, "action.camera.height_wheel", Neuron::ValueKind::Axis1, {MouseBinding(MouseControl::Wheel, Move)});
  bind(GameAction::CameraAim, "action.camera.aim", Neuron::ValueKind::Axis2, {MouseBinding(MouseControl::Motion, Move)});
  bind(GameAction::CameraZoom, "action.camera.zoom", Neuron::ValueKind::Boolean,
       {KeyBinding(KeyCode::Z, Held), MouseBinding(MouseControl::Middle, Held)});
  bind(GameAction::CameraLookAround, "action.camera.look_around", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::F2, Held)});
  // Rows 19 to 24 (reference/06 rows S1, S3 to S6): the primary button picks, and on the ground begins the circle.
  bind(GameAction::SelectPick, "action.select.pick", Neuron::ValueKind::Boolean, {MouseBinding(MouseControl::Primary, Down)});
  bind(GameAction::SelectPickIdle, "action.select.pick_idle", Neuron::ValueKind::Boolean, {MouseBinding(MouseControl::Primary, Down)});
  bind(GameAction::SelectClear, "action.select.clear", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Space, Down)});
  bind(GameAction::SelectCircleBegin, "action.select.circle_begin", Neuron::ValueKind::Boolean,
       {MouseBinding(MouseControl::Primary, Down)});
  bind(GameAction::SelectCircleGrow, "action.select.circle_grow", Neuron::ValueKind::Boolean, {MouseBinding(MouseControl::Primary, Held)});
  bind(GameAction::SelectCircleEnd, "action.select.circle_end", Neuron::ValueKind::Boolean, {MouseBinding(MouseControl::Primary, Up)});
  // Rows 40 to 45 (rows O1 to O3, O6, O7 and T5) as O82 binds them (ADR-127): the orders of the crowd and of the selected
  // Leader on the primary button, which they share with the selection; the promotion and the toggle on the secondary
  // button, whose press also clears a selection. The verbs' resolver gives each press one meaning; Ctrl and Shift held
  // modify the orders.
  bind(GameAction::OrderCrowd, "action.order.crowd", Neuron::ValueKind::Boolean, {MouseBinding(MouseControl::Primary, Down)});
  bind(GameAction::OrderStraightRoute, "action.order.straight_route", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Control, Held)});
  bind(GameAction::OrderPromote, "action.order.promote", Neuron::ValueKind::Boolean, {MouseBinding(MouseControl::Secondary, Down)});
  bind(GameAction::OrderSpecial, "action.order.special", Neuron::ValueKind::Boolean, {MouseBinding(MouseControl::Primary, Down)});
  bind(GameAction::OrderMoveModifier, "action.order.move_modifier", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Shift, Held)});
  bind(GameAction::OrderDemote, "action.order.demote", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::C, Down)});
  // Table 3.6-B rows 1 to 5 and 7 (UI-204: the arrow keys with repeat, Enter and Esc), and table 3.6-A rows 60 and 61.
  bind(GameAction::MenuLeft, "action.menu.left", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Left, Repeat)});
  bind(GameAction::MenuRight, "action.menu.right", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Right, Repeat)});
  bind(GameAction::MenuUp, "action.menu.up", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Up, Repeat)});
  bind(GameAction::MenuDown, "action.menu.down", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Down, Repeat)});
  bind(GameAction::MenuActivate, "action.menu.activate", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Enter, Down)});
  bind(GameAction::MenuClose, "action.menu.close", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::Escape, Down)});
  bind(GameAction::UiPointerDown, "action.ui.pointer_down", Neuron::ValueKind::Boolean, {MouseBinding(MouseControl::Primary, Down)});
  bind(GameAction::UiPointerHeld, "action.ui.pointer_held", Neuron::ValueKind::Boolean, {MouseBinding(MouseControl::Primary, Held)});
  bind(GameAction::WindowToggleMode, "action.window.toggle_mode", Neuron::ValueKind::Boolean,
       {Neuron::AllOf({KeyBinding(KeyCode::Alt, Held), KeyBinding(KeyCode::Enter, Down)})});
#if ENGINE_DEVELOPMENT
  bind(GameAction::DebugRemoveDevice, "action.debug.remove_device", Neuron::ValueKind::Boolean,
       {Neuron::ChordOf({KeyCode::Control, KeyCode::Shift, KeyCode::F11})});
  bind(GameAction::DebugOverlay, "action.debug.overlay", Neuron::ValueKind::Boolean, {KeyBinding(KeyCode::F5, Down)});
  bind(GameAction::DebugRestartSandbox, "action.debug.restart_sandbox", Neuron::ValueKind::Boolean,
       {Neuron::ChordOf({KeyCode::Control, KeyCode::Shift, KeyCode::F7})});
  bind(GameAction::DebugRestartSandboxNewSeed, "action.debug.restart_sandbox_new_seed", Neuron::ValueKind::Boolean,
       {Neuron::ChordOf({KeyCode::Control, KeyCode::Shift, KeyCode::F8})});
#endif
  return actions;
}

namespace
{

struct LayoutEntry
{
  std::string_view action;
  Neuron::KeyCode german;
  Neuron::KeyCode french;
};

// Table 4.13-B.
constexpr std::array LAYOUT_ENTRIES{
  LayoutEntry{"action.camera.pan_left", Neuron::KeyCode::A, Neuron::KeyCode::Q},
  LayoutEntry{"action.camera.pan_right", Neuron::KeyCode::D, Neuron::KeyCode::D},
  LayoutEntry{"action.camera.pan_forward", Neuron::KeyCode::W, Neuron::KeyCode::Z},
  LayoutEntry{"action.camera.pan_back", Neuron::KeyCode::S, Neuron::KeyCode::S},
  LayoutEntry{"action.camera.raise", Neuron::KeyCode::Q, Neuron::KeyCode::A},
  LayoutEntry{"action.camera.lower", Neuron::KeyCode::E, Neuron::KeyCode::E},
  LayoutEntry{"action.camera.zoom", Neuron::KeyCode::Y, Neuron::KeyCode::W},
};

constexpr std::string_view LAYOUT_GERMAN = "de-DE";
constexpr std::string_view LAYOUT_FRENCH = "fr-FR";

} // namespace

std::string ActiveLayoutTag(const Neuron::SettingsStore& _settings, std::string_view _systemLayoutTag)
{
  const std::string stored =
    _settings.Get(SETTING_KEYBOARD_LAYOUT).has_value() ? _settings.GetText(SETTING_KEYBOARD_LAYOUT) : std::string{};
  return stored.empty() ? std::string(_systemLayoutTag) : stored;
}

std::vector<Neuron::BindingReplacement> LayoutLayer(std::string_view _layoutTag)
{
  std::vector<Neuron::BindingReplacement> layer;
  if (_layoutTag != LAYOUT_GERMAN && _layoutTag != LAYOUT_FRENCH)
  {
    Neuron::Log(Neuron::LogCategory::Input, Neuron::LogLevel::Info, "the keyboard layout {} has no layout layer (INP-164)", _layoutTag);
    return layer;
  }
  const bool german = _layoutTag == LAYOUT_GERMAN;
  for (const LayoutEntry& entry : LAYOUT_ENTRIES)
  {
    layer.push_back({std::string(entry.action), Neuron::KeyBinding(german ? entry.german : entry.french, Neuron::Condition::Held),
                     std::format("the layout layer {}", _layoutTag)});
  }
  return layer;
}

std::vector<Neuron::BindingReplacement> UserLayer(const Neuron::SettingsStore& _settings)
{
  std::vector<Neuron::BindingReplacement> layer;
  for (const std::string& key : _settings.KeysWithPrefix(SETTING_BINDING_FAMILY))
  {
    std::expected<Neuron::InputBinding, std::string> binding = Neuron::ParseBinding(_settings.GetText(key));
    if (!binding.has_value())
    {
      Neuron::Log(Neuron::LogCategory::Input, Neuron::LogLevel::Warning, "{}: the binding is skipped (INP-171): {}", key, binding.error());
      continue;
    }
    layer.push_back({key.substr(SETTING_BINDING_FAMILY.size()), std::move(*binding), key});
  }
  return layer;
}

std::vector<Neuron::BindingReplacement> BindingLayers(const Neuron::SettingsStore& _settings, std::string_view _systemLayoutTag)
{
  std::vector<Neuron::BindingReplacement> layers = LayoutLayer(ActiveLayoutTag(_settings, _systemLayoutTag));
  std::vector<Neuron::BindingReplacement> user = UserLayer(_settings);
  layers.insert(layers.end(), std::make_move_iterator(user.begin()), std::make_move_iterator(user.end()));
  return layers;
}

Neuron::SettingAccess SettingAccessOf(const Neuron::SettingsStore& _settings)
{
  Neuron::SettingAccess access;
  access.exists = [&_settings](std::string_view _key) { return _settings.Knows(_key); };
  access.number = [&_settings](std::string_view _key) -> std::optional<double>
  {
    const std::optional<Neuron::SettingValue> value = _settings.Get(_key);
    if (!value.has_value())
    {
      return std::nullopt;
    }
    if (const auto* flag = std::get_if<bool>(&*value))
    {
      return *flag ? 1.0 : 0.0;
    }
    if (const auto* integer = std::get_if<std::int64_t>(&*value))
    {
      return static_cast<double>(*integer);
    }
    if (const auto* decimal = std::get_if<double>(&*value))
    {
      return *decimal;
    }
    return std::nullopt;
  };
  return access;
}

} // namespace Species
