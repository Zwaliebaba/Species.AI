#pragma once

#include "InputEvents.h"
#include "PlatformClock.h"
#include "WindowGeometry.h"

#include <array>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <span>
#include <string>
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
// mouse (ADR-119), and M2 repeat for the menus (INP-133, ADR-149). Position, release and zero are the gamepad's analogue
// conditions: they have their names (INP-055), and no control of the MVP accepts them (MVP-100, table 3.2-D).
enum class Condition : std::uint8_t
{
  Down,
  Up,
  Held,
  Move,
  Read,
  Repeat, // INP-133: on the press, then after the initial delay and every period while held
  Position,
  Release,
  Zero
};

// INP-060, INP-141: the tests of an idle node.
enum class IdleTest : std::uint8_t
{
  AtLeast,
  Reaches,
  NoLonger
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

/// A binding node (INP-060): a tree whose root is the binding. Nodes are immutable once loaded (INP-066); the state of a
/// latch and the value of a setting live in the router's state table, at the node's slot.
struct InputBinding
{
  enum class Kind : std::uint8_t
  {
    Leaf,
    All,
    Chord,
    Not,
    Alias,
    Latch,
    Idle,
    Setting,
    Value
  };

  Kind kind = Kind::Leaf;
  InputLeaf leaf;                     // Leaf; a chord's part is a leaf whose condition is not read (INP-137)
  std::vector<InputBinding> operands; // All and Chord: two or more; Not: one; Latch: on, then off
  std::string name;                   // Alias: the action id; Setting: the setting key; Value: a feature flag, empty for always_true
  IdleTest idle = IdleTest::AtLeast;  // Idle
  std::uint32_t seconds = 0;          // Idle's at_least and reaches: a whole number of seconds, at least 1
  bool negate = false;                // Setting
  std::uint32_t slot = 0;             // Latch and Setting: the state table's slot, which Rebuild assigns

  bool operator==(const InputBinding& _other) const;
};

[[nodiscard]] InputBinding KeyBinding(KeyCode _key, Condition _condition);
[[nodiscard]] InputBinding MouseBinding(MouseControl _control, Condition _condition);
[[nodiscard]] InputBinding AllOf(std::initializer_list<InputBinding> _operands);
[[nodiscard]] InputBinding ChordOf(std::initializer_list<KeyCode> _keys);
[[nodiscard]] InputBinding NotOf(InputBinding _operand);
[[nodiscard]] InputBinding AliasOf(std::string_view _actionId);
[[nodiscard]] InputBinding LatchOf(InputBinding _on, InputBinding _off);
[[nodiscard]] InputBinding IdleOf(IdleTest _test, std::uint32_t _seconds = 0);
[[nodiscard]] InputBinding SettingOf(std::string_view _key, bool _negate = false);
[[nodiscard]] InputBinding AlwaysTrue();

/// What a binding may name, which the router checks when it loads one (INP-060, INP-142, INP-143).
struct SettingAccess
{
  std::function<bool(std::string_view)> exists;                  // the key is in the store or its defaults
  std::function<std::optional<double>(std::string_view)> number; // its value as a number; true is 1, false 0
};

/// One entry of a replacing layer (INP-162): the binding that replaces its action's primary binding, and where it came
/// from, for the log (the layout layer's tag, or the user layer's setting key).
struct BindingReplacement
{
  std::string action; // the action id
  InputBinding binding;
  std::string source;
};

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

/// The input router (`InputRouter`, ADR-029, INP-020; ADR-119, ADR-151): it builds the action bindings from the
/// compiled-in default set the game gives it (O75) and the replacing layers (INP-162), and it answers whether an action
/// fired in the frame, and with which value, from the state it built at the start of the frame. Main thread only.
class InputRouter
{
public:
  // The default set: the actions in ActionId order, with the development layer's entries in a Debug build (INP-095).
  // _pointer may be null, in a test. A default binding the node rules refuse is logged and left out (INP-170's checks).
  InputRouter(std::span<const ActionBindings> _actions, DeviceBackEnd& _backEnd, PointerHost* _pointer, SettingAccess _settings = {});

  // INP-066, INP-162: the bindings again from the default set, then each replacement in order as its action's primary
  // binding (INP-065), the layout layer's before the user layer's. A replacement that names no action or breaks the node
  // rules is skipped with a warning naming its source (INP-171). The latches' states are reset. Returns the replacements
  // applied.
  std::size_t Rebuild(std::span<const BindingReplacement> _replacements);
  // INP-067: the action of an id, or none.
  [[nodiscard]] std::optional<ActionId> FindAction(std::string_view _id) const noexcept;
  // INP-060, INP-061, INP-172: why _binding cannot bind the action, or none when it can.
  [[nodiscard]] std::optional<std::string> CheckBinding(ActionId _action, const InputBinding& _binding) const;
  [[nodiscard]] std::span<const InputBinding> BindingsOf(ActionId _action) const noexcept;

  // INP-101: the first input step of every rendered frame. A focus lost since the previous frame releases everything
  // held (INP-111); while the window has no focus, presses and movement are dropped. _clientSize clamps the pointer, and
  // _realTime advances the repeat timers (INP-133, INP-101 step 7), which fire only on the press while it stands still.
  void AdvanceFrame(const FrameGates& _gates, SizePx _clientSize, Nanoseconds _realTime = Nanoseconds{0});
  // INP-112 (ADR-154): the source of text input, whose characters each frame forwards to the UI; none drops them.
  void SetTextSource(TextSource* _source) noexcept
  {
    m_textSource = _source;
  }
  // The frame's characters, in order, while the window has focus; the UI core reads them (UI-007, UI-038).
  [[nodiscard]] std::span<const TextCharacter> Text() const noexcept
  {
    return m_text;
  }
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

  // INP-150: the class of the last active device; always KeyboardMouse until a gamepad back-end exists (MVP-100).
  [[nodiscard]] InputMode Mode() const noexcept
  {
    return m_mode;
  }
  // INP-153: whether no device was active in this frame, and the idle time the idle nodes read.
  [[nodiscard]] bool IsIdle() const noexcept
  {
    return m_idle;
  }
  [[nodiscard]] double IdleSeconds() const noexcept
  {
    return m_idleSeconds;
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
  [[nodiscard]] bool Evaluate(const InputBinding& _binding, ActionValue& _value, std::uint32_t _depth) const;
  [[nodiscard]] bool QueryAt(std::size_t _action, ActionValue& _value, std::uint32_t _depth) const;
  [[nodiscard]] std::optional<std::string> CheckNode(const InputBinding& _node, ValueKind& _kind) const;
  [[nodiscard]] bool AliasReaches(const InputBinding& _node, std::string_view _target, std::uint32_t _depth) const;
  void AssignSlots(InputBinding& _node);
  void UpdateEnvironment(Nanoseconds _realTime, bool _active);

  std::vector<ActionBindings> m_defaults;
  std::vector<ActionBindings> m_actions;
  SettingAccess m_settings;
  std::vector<const InputBinding*> m_latchNodes;   // INP-140: the nodes whose state AdvanceFrame updates
  std::vector<const InputBinding*> m_settingNodes; // INP-142: the nodes whose setting AdvanceFrame reads
  std::vector<bool> m_slotState;                   // the state table, indexed by a node's slot (INP-066)
  InputMode m_mode = InputMode::KeyboardMouse;
  bool m_idle = false;
  bool m_wasIdle = false;
  double m_idleSeconds = 0.0;
  double m_previousIdleSeconds = 0.0;
  std::optional<Nanoseconds> m_lastRealTime;
  DeviceBackEnd& m_backEnd;
  PointerHost* m_pointerHost;
  TextSource* m_textSource = nullptr;
  std::vector<TextCharacter> m_text;
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
