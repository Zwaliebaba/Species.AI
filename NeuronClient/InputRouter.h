#pragma once

#include "InputEvents.h"
#include "PlatformClock.h"
#include "WindowGeometry.h"

#include <array>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <string_view>
#include <vector>

namespace Neuron
{

// INP-150: the input mode, which chooses a string's variant (UI-223). The MVP has no gamepad (MVP-100), so its mode is
// always KeyboardMouse until the gamepad back-end tracks the device used last (ADR-146).
enum class InputMode : std::uint8_t
{
  KeyboardMouse,
  Gamepad
};

// INP-050: an action's value kind.
enum class ValueKind : std::uint8_t
{
  Boolean,
  Axis1,
  Axis2
};

// INP-051: the conditions a device leaf tests. M1 builds the Boolean edges and holds and the movement conditions of the
// mouse (ADR-119), and M2 repeat for the menus (INP-133, ADR-149); the gamepad's analogue conditions arrive with it.
enum class Condition : std::uint8_t
{
  Down,
  Up,
  Held,
  Move,
  Read,
  Repeat // INP-133: on the press, then after the initial delay and every period while held
};

/// INP-133's timer: inp.repeat.initial_delay and inp.repeat.period, on real time.
struct RepeatTiming
{
  Nanoseconds initialDelay{300'000'000};
  Nanoseconds period{300'000'000};
};

/// A device leaf (INP-060): one condition on one control.
struct InputLeaf
{
  InputDevice device = InputDevice::Key;
  std::uint16_t control = 0;
  Condition condition = Condition::Held;
};

/// A binding (INP-060) of the kinds that M1's compiled-in set uses: a device leaf, a conjunction of leaves (`all`), or a
/// chord of keys and buttons, whose leaves' conditions are not read (`chord`, INP-137).
struct InputBinding
{
  enum class Kind : std::uint8_t
  {
    Leaf,
    All,
    Chord
  };

  Kind kind = Kind::Leaf;
  std::vector<InputLeaf> leaves;
};

[[nodiscard]] InputBinding KeyBinding(KeyCode _key, Condition _condition);
[[nodiscard]] InputBinding MouseBinding(MouseControl _control, Condition _condition);
[[nodiscard]] InputBinding AllOf(std::initializer_list<InputBinding> _operands); // each operand a leaf
[[nodiscard]] InputBinding ChordOf(std::initializer_list<KeyCode> _keys);

/// One action of the binding set and its bindings in order, the first its primary binding (INP-065).
struct ActionBindings
{
  std::string_view id; // the action id, for the log
  ValueKind kind = ValueKind::Boolean;
  std::vector<InputBinding> bindings;
};

struct ActionId
{
  std::uint16_t index = 0;
};

/// INP-090: an action's value in a frame.
struct ActionValue
{
  ValueKind kind = ValueKind::Boolean;
  float x = 0.0f;
  float y = 0.0f;
};

/// INP-156's gates as M1 has them: no UI window and no text entry exist yet.
struct FrameGates
{
  bool windowFocused = true;
};

/// INP-155: the pointer of player 0.
struct PointerState
{
  PointPx positionPixels; // client pixels
  float deltaX = 0.0f;    // this frame's movement, client pixels
  float deltaY = 0.0f;
  float wheelDeltaNotches = 0.0f;
  bool moved = false;
};

/// The input router (`InputRouter`, ADR-029, INP-020) in its M1 form (ADR-119): the actions and bindings it holds are
/// the compiled-in set the game gives it (O75), and it answers whether an action fired in the frame, and with which value,
/// from the state it built at the start of the frame. Main thread only.
class InputRouter
{
public:
  // The actions in ActionId order; _pointer may be null, in a test.
  InputRouter(std::span<const ActionBindings> _actions, DeviceBackEnd& _backEnd, PointerHost* _pointer);

  // INP-101: the first input step of every rendered frame. A focus lost since the previous frame releases everything
  // held (INP-111); while the window has no focus, presses and movement are dropped. _clientSize clamps the pointer, and
  // _realTime advances the repeat timers (INP-133, INP-101 step 7), which fire only on the press while it stands still.
  void AdvanceFrame(const FrameGates& _gates, SizePx _clientSize, Nanoseconds _realTime = Nanoseconds{0});
  void SetRepeatTiming(const RepeatTiming& _timing) noexcept
  {
    m_repeat = _timing;
  }

  // INP-020: true at the first satisfied binding of the action, whose value is returned; a cleared value otherwise.
  [[nodiscard]] bool Query(ActionId _action, ActionValue* _value = nullptr) const;
  // INP-093: the action answers false until the next AdvanceFrame.
  void Suppress(ActionId _action);

  [[nodiscard]] const PointerState& Pointer() const noexcept
  {
    return m_pointer;
  }
  // INP-158: the position, clamped to the client area; the system pointer moves there with no movement event or delta.
  void WarpPointer(PointPx _clientPosition);
  // INP-159: the system pointer held inside the client area while on.
  void ConfinePointer(bool _on);

  [[nodiscard]] std::size_t ActionCount() const noexcept
  {
    return m_actions.size();
  }

private:
  struct ControlState
  {
    bool held = false;
    std::uint16_t presses = 0;
    std::uint16_t releases = 0;
    std::uint16_t repeats = 0; // INP-134: the repeat condition's firings in this frame
    Nanoseconds nextRepeat{};
  };

  void Apply(const InputEvent& _event);
  [[nodiscard]] const ControlState* StateOf(InputDevice _device, std::uint16_t _control) const noexcept;
  [[nodiscard]] bool Evaluate(const InputLeaf& _leaf, ActionValue& _value) const;
  [[nodiscard]] bool Evaluate(const InputBinding& _binding, ActionValue& _value) const;

  std::vector<ActionBindings> m_actions;
  DeviceBackEnd& m_backEnd;
  PointerHost* m_pointerHost;
  std::array<ControlState, KEY_CODE_COUNT> m_keys{};
  std::array<ControlState, MOUSE_BUTTON_COUNT> m_buttons{};
  float m_motionX = 0.0f; // the frame's summed movement and wheel deltas (INP-118)
  float m_motionY = 0.0f;
  float m_wheelNotches = 0.0f;
  PointerState m_pointer;
  SizePx m_clientSize;
  std::vector<bool> m_suppressed;
  std::vector<InputEvent> m_queue;
  RepeatTiming m_repeat;
  bool m_focused = true;
};

} // namespace Neuron
