#include "pch.h"

#include "InputBindings.h"

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

} // namespace Species
