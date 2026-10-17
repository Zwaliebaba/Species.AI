# Input

> Status: Engine baseline · Prefix: INP · ADRs: [ADR-0028](../../adr/0028-input-device-back-ends.md) · [ADR-0029](../../adr/0029-input-event-model-and-binding-contract.md) · [ADR-0032](../../adr/0032-ui-focus-graph-page-stack-and-input.md) · [ADR-0064](../../adr/0064-accessibility-baseline-xag.md) · [ADR-0074](../../adr/0074-platform-clock-with-injectable-time.md) · [ADR-0075](../../adr/0075-application-window-and-dpi-awareness.md) · [ADR-0059](../../adr/0059-content-build-and-validators.md) · [ADR-0061](../../adr/0061-data-schemas-and-versioning.md) · [ADR-0062](../../adr/0062-authoring-syntax-and-schema-language.md) · [ADR-0063](../../adr/0063-localisation.md) · [ADR-0067](../../adr/0067-logging.md) · [ADR-0069](../../adr/0069-test-framework.md) · Related: [Controls, camera and commands](../../reference/06_controls_camera_and_commands/README.md) · [Actions and bindings](../../reference/06_controls_camera_and_commands/actions_and_bindings.md) · [Camera](../../reference/06_controls_camera_and_commands/camera.md) · [Teams and player commands](../../reference/12_simulation_model/06_teams_and_player_commands.md) · [UI framework](ui_framework/README.md) · [Platform layer](platform_layer.md) · [Data schemas](data_schemas/README.md)

This component turns device input into Species actions. It owns the device back-ends, the ordered event queue, the device state of each frame, the assignment of gamepads to local players, the binding graph that maps device conditions to action ids, the binding sets and their layers, the name table of every bindable control, the pointer, the input mode and the idle time. It answers one question for the rest of the client: did action *X* fire in this frame, and with which value. What each action means, and which player command it produces, is specified by [actions and bindings](../../reference/06_controls_camera_and_commands/actions_and_bindings.md); this component never defines a second list of match actions.

Notation used throughout:

- A **frame** is one rendered frame of the client loop. Input timings (repeat, idle) are seconds of real time read from the platform clock (ADR-0074): they are presentation timings, never simulation durations, and they keep running while a match is paused (R-CTRL-006).
- **Binding notation.** This document writes a device condition as `device:control:condition`, for example `key:a:held`, `mouse:primary:down` or `pad:stick_left:position`, and composite bindings as `all(…)`, `chord(…)`, `not(…)`, `alias(<action id>)` and `latch(…)`. Bindings of one action are listed in order and separated by `·`; the first is the **primary** binding. §3.5 maps the notation one to one onto the binding-set schema.
- **Stick units** are the unit of stick payloads after the response curve (§4.6); pushing a stick forward gives a negative y.

## 1 Responsibilities and boundaries

**INP-001** (MUST) The input component owns: the gamepad back-end on GameInput and the keyboard and mouse back-end (ADR-0028, §4.2); the per-frame ordered event queue (ADR-0029, §4.3); the device state derived from it (§4.4); the local players and their gamepad assignment (§4.10); the binding graph, its node kinds and their evaluation (§3.3, §4.5 to §4.9); the action bindings (`ActionBindings`) of every action, with their glyph overrides and suppression flags (§3.4); the name table of every bindable key, mouse control and gamepad control (§3.2); the binding layers and the keyboard-layout table (§4.13); the pointer (§4.12); the input mode and the idle time (§4.11); and the rebinding services that the rebinding hook uses (§4.15).

**INP-002** (MUST) Game code never reads a device. It asks the input router (`InputRouter`) whether an action fired in the current frame and may receive the action's value: its kind, its axis values and the device that produced it. Every answer comes from the state that the router built at the start of the frame; asking is free of side effects (DEC-554).

**INP-003** (MUST) The match actions are exactly the action ids of [actions and bindings](../../reference/06_controls_camera_and_commands/actions_and_bindings.md) table 2.1-A (R-CTRL-030). This component binds them (table 3.6-A) and adds only the UI navigation and hook-page actions of table 3.6-B, which that document leaves to the UI framework (R-CTRL-035). Action ids are the stable names that bindings, rebinding and saved settings use (ADR-0029).

**INP-004** (MUST) Player commands originate only in the input layer. The match input pump (`MatchInputPump`) and the sampler of the team command state (`TeamCommandState`) turn actions into members of `PlayerCommand` and into fields of the team command state, by the rules of [actions and bindings](../../reference/06_controls_camera_and_commands/actions_and_bindings.md) (DEC-023); the router itself produces actions, never commands. Simulation code links no input interface, never queries an action, never reads the pointer, a device, the input mode or the camera, and never issues a player command (DEC-008, DEC-119, R-CTRL-003, R-CTRL-004). One click therefore yields one command on the sender and one effect on every participant.

**INP-005** (MUST NOT) The input component never writes simulation state, never reads it, and never changes a Power, a selection or a unit locally. Local feedback (sounds, previews, camera modes) is the consumers' business (R-CTRL-022, DEC-007).

**INP-006** (MUST) Text entry does not pass through the binding graph. Text characters and input-method composition go to the focused text field through the UI framework's text service, with the modifier state of each character (ADR-0028, ADR-0032).

**INP-007** (MUST) The headless server links neither GameInput nor any input back-end (ADR-0028). Bots issue no input actions (R-CTRL-005).

**INP-008** (MUST) The neighbours of this component own: the window, its focus, its close request and the system cursor's visibility over the client area ([platform layer](platform_layer.md), ADR-0075); widget focus, page navigation, UI repeat timers and input-mode captions ([UI framework](ui_framework/README.md), ADR-0032); the meaning, context and command of every match action (reference/06); the file format of the binding sets ([data schemas](data_schemas/README.md), ADR-0061, ADR-0062); and the settings store (`SettingsStore`) that holds the user layer (DEC-250).

**INP-009** (MUST) The default binding set binds only the actions of tables 3.6-A and 3.6-B. It holds the one match control scheme (`ControlScheme`, R-CTRL-002): no alternative scheme (DEC-056), no gesture input and no icon Power bar (DEC-055), no loader mini-programs or attract mode (DEC-054), and no developer key other than `action.debug.network_stats`.

**INP-010** (MUST) Every device class is remapped through the same binding graph, and the graph offers a latch node for hold-or-toggle options (§4.8), so remapping and toggle options are done once for every device (ADR-0064).

## 2 Interfaces

### 2.1 Interface sketch

```cpp
namespace species::input {

enum class DeviceClass : std::uint8_t { KeyboardMouse, Gamepad };
enum class InputMode   : std::uint8_t { KeyboardMouse, Gamepad };            // INP-150
enum class ValueKind   : std::uint8_t { Boolean, Axis1, Axis2 };             // INP-050
enum class Condition   : std::uint8_t { Down, Up, Held, Repeat,               // Boolean conditions
                                        Position, Move, Release, Zero, Read }; // analogue conditions, INP-051

struct DeviceId      { std::uint64_t value; };    // stable identity from a back-end (INP-102)
struct LocalPlayerId { std::uint8_t index; };     // player 0 owns the keyboard and the mouse (INP-140)
struct ActionId      { std::uint16_t index; };    // interned from the action id string when bindings load

enum class KeyCode      : std::uint16_t;           // table 3.2-A
enum class MouseControl : std::uint8_t { Primary, Secondary, Middle, Motion, RawMotion, Wheel };  // table 3.2-B
enum class PadControl   : std::uint8_t;            // table 3.2-C

struct InputEvent {                                // INP-110
  enum class Kind : std::uint8_t { Press, Release, Axis, Connect, Disconnect };
  Kind          kind;
  bool          os_repeat;                         // keyboard auto-repeat; never a new press (INP-106)
  DeviceClass   device_class;
  DeviceId      device;
  std::uint16_t control;                           // a KeyCode, MouseControl or PadControl value
  std::int64_t  timestamp_ns;                      // platform clock (ADR-0074)
  float         x, y;                              // Axis: a sample (stick, trigger) or a delta (motion, wheel)
};

struct ActionValue {                               // INP-091
  ValueKind               kind = ValueKind::Boolean;
  float                   x = 0.0f, y = 0.0f;
  std::optional<DeviceId> device;
};

struct FrameGates {                                // INP-160, supplied by the UI framework and the platform layer
  bool ui_window_open;                             // as the UI left it at the end of the previous frame
  bool text_entry_open;                            // chat input or a focused text field
  bool window_focused;
};

struct PointerState {                              // INP-160
  Vec2  position_px;                               // client-area pixels
  float wheel_z;                                   // accumulated wheel notches
  Vec2  delta_px;                                  // this frame's movement
  float delta_z;
  bool  moved;                                     // delta_px is not zero
};

class InputRouter {
public:
  // Frame lifecycle (INP-100, INP-103). Main thread only.
  void AdvanceFrame(const FrameGates& gates);
  void AdvanceRepeatPass();

  // Queries: const and free of side effects (INP-002).
  bool Query(LocalPlayerId, ActionId, ActionValue* value = nullptr) const;
  bool QueryBinding(LocalPlayerId, const BindingNode&, ActionValue* value = nullptr) const;
  void Suppress(ActionId);                         // false for the rest of the frame (INP-093)
  InputMode Mode(LocalPlayerId) const;
  bool   IsIdle() const;
  double IdleSeconds() const;
  const PointerState& Pointer() const;
  void   WarpPointer(Vec2 position_px);            // moves the pointer without a motion delta (INP-164)
  void   ConfinePointer(bool on);                  // holds the system pointer inside the client area (INP-165)
  std::optional<DeviceId> AssignedGamepad(LocalPlayerId) const;
  bool   AssignedGamepadConnected(LocalPlayerId) const;
  std::string_view ActiveLayout() const;           // keyboard-layout tag (INP-171)

  // Binding sets (INP-120 to INP-131, INP-180 to INP-184).
  LoadReport Rebuild(const BindingSources& sources);   // default set, layout layer, user layer
  std::expected<void, LoadDiagnostic> ReplacePrimaryBinding(ActionId, const BindingNode& binding);
  std::optional<BindingNode> CaptureFirstActiveInput(LocalPlayerId, DeviceClassMask, bool instant) const;
  std::optional<BindingDescription> Describe(ActionId) const;
  std::optional<AssetId> Glyph(ActionId) const;
  void ClearBindings();                            // keeps glyph overrides
  void DumpBindings(LogSink&) const;               // diagnostics (INP-252)

  // Notifications, raised inside AdvanceFrame.
  Signal<LocalPlayerId, InputMode> on_mode_changed;
  Signal<std::string_view>         on_layout_changed;
};

class DeviceBackEnd {                              // INP-101
public:
  virtual ~DeviceBackEnd() = default;
  virtual bool Initialise(PlatformServices&) = 0;
  virtual void Collect(EventSink&) = 0;            // every event since the last call, in order
  virtual void ReleaseAll(EventSink&) = 0;         // release events for everything held (INP-107)
  virtual void Shutdown() = 0;
};

}  // namespace species::input
```

### 2.2 Router operations

**INP-020** (MUST) The router's operations behave as follows:

| Operation | Behaviour |
|---|---|
| `Query(player, action, value)` | false if the action is suppressed in this frame; otherwise the action's bindings are evaluated **in order** for that player, and the result is true at the **first** binding that is satisfied, whose value is returned. An action is the logical OR of its bindings, and its value comes from the first satisfied binding. When no binding is satisfied, the value is cleared (kind Boolean, axes 0, no device) |
| `QueryBinding(player, node, value)` | evaluates one binding node; composite nodes use it for their operands (§4.8) |
| `Suppress(action)` | the action answers false until the next `AdvanceFrame`; an alias of it answers false as well (INP-093) |
| `Rebuild(sources)` | clears every binding and rebuilds the action bindings from the default set, the active layout layer and the user layer, in that order (§4.13), and returns every diagnostic (§6.1) |
| `ReplacePrimaryBinding(action, binding)` | validates the binding (§4.14) and, when it is accepted, replaces the action's primary binding, or appends it when the action has none; a rejected binding returns its diagnostic and changes nothing |
| `CaptureFirstActiveInput(player, classes, instant)` | the first control of the allowed device classes, in name-table order, that went down in this frame, as a leaf binding with the condition `down` when `instant` is set and `held` otherwise (§4.15) |
| `Describe(action)` | the display description of the first binding of the action that has one (§4.15) |
| `Glyph(action)` | the action's glyph override, if the binding set gives one (§3.7) |
| `IsIdle()`, `IdleSeconds()`, `Mode(player)` | §4.11 |
| `ClearBindings()` | removes every binding of every action; glyph overrides stay |
| `DumpBindings(sink)` | writes every action with its bindings in canonical form and the count of bindings (§6.4) |

### 2.3 Events and data contracts

**INP-021** (MUST) The router's inputs are: the device events of the back-ends (§4.2, §4.3); the frame gates, which the UI framework and the platform layer supply before `AdvanceFrame`; the values of the settings that setting nodes and the keyboard-layout setting name (§4.8, §4.13); and the layout-change and focus-change notices of the keyboard and mouse back-end.

**INP-022** (MUST) The router's outputs are: action results and values (by query); the pointer state; the input mode of each local player and its change notice; the idle state; the active keyboard layout and its change notice; the assignment and connection state of each local player's gamepad; and text events, which it forwards unchanged to the UI framework's text service (INP-006).

**INP-023** (MUST) The default binding set and the layout layers are content: authored as JSON records, validated by the content build and loaded as cooked records (ADR-0059, ADR-0061, ADR-0062, DEC-015). The user layer is a run-time settings file in the settings store (DEC-250) and is validated by the router when it loads (§4.14).

**INP-024** (MUST) Every display text of input (key names, control names, condition words, the chord separator) is a string id of the string table with its fallback (ADR-0063, DEC-099), never a name that the parser accepts, and display text is never fed back into parsing (ADR-0029).

### 2.4 Threading contract

**INP-025** (MUST) The router lives on the main thread, which runs the platform layer's message loop (ADR-0075). `AdvanceFrame`, `AdvanceRepeatPass`, every query, `Suppress`, `WarpPointer`, `ConfinePointer` and every binding operation run on the main thread only. Between two `AdvanceFrame` calls the router's frame state is immutable.

**INP-026** (MUST) The keyboard and mouse back-end receives its input on the main thread, forwarded by the platform layer (ADR-0075). The gamepad back-end reads gamepad readings on the main thread inside `AdvanceFrame`; GameInput's device notices may arrive on another thread and are written into a single-producer, single-consumer mailbox that `AdvanceFrame` drains. No other thread touches router state.

**INP-027** (MUST) Events that arrive while the main thread is busy (a loading view, a long frame) stay in their back-end's buffer until the next `AdvanceFrame`; the platform layer keeps pumping the window during loading views, so no event is lost while a frame is not running.

### 2.5 Helper abstractions

**INP-028** (MUST) The component provides these helpers on top of `Query`; none of them holds state between frames:

| Helper | Behaviour |
|---|---|
| `ActionAxis` | composes up to six Boolean actions (a positive and a negative action per axis x, y and z) into a vector: each component is (positive − negative) × a sensitivity |
| `StickAxis` | the value of an `Axis2` action × a sensitivity, or zero when the action did not fire |
| `PriorityAxis` | the value of a first source unless it is zero, otherwise the value of a second source |
| `ActionTrigger` | a callable that queries one action for one player; the UI framework's keyboard shortcuts are built on it |

No match rule of reference/06 uses the three axis helpers; the stateful hold-or-toggle helper is the latch node of §4.8.

## 3 Data model

### 3.1 Value kinds and conditions

**INP-050** (MUST) Every action, binding and device condition has one value kind:

| Kind | Meaning | Value |
|---|---|---|
| `Boolean` | button-like | none beyond the result; a Boolean action fed by an analogue binding carries that binding's value (INP-061) |
| `Axis1` | one axis | x: a trigger, the wheel, the idle time |
| `Axis2` | two axes | x and y: pointer motion, a stick |

**INP-051** (MUST) A device leaf tests one condition on one control. The conditions are:

| Condition | Applies to | True in a frame when | Value |
|---|---|---|---|
| `down` | a Boolean view (table 3.2-D) | a press event of the control occurred in this frame | — |
| `up` | a Boolean view | a release event of the control occurred in this frame | — |
| `held` | a Boolean view | the control is pressed after the frame's last event | — |
| `repeat` | a Boolean view | the control went down in this frame, or it is held and at least one repeat instant falls in this frame (§4.7) | x = the number of firings in this frame |
| `position` | an analogue control | the control is outside its rest zone at the end of the frame | the processed value (§4.6) |
| `move` | an analogue control | the value changed since the end of the previous frame by more than the control's move threshold (table 3.2-D) | the change |
| `release` | an analogue control | the control is at rest at the end of the frame, and it was not at rest at the end of the previous frame or at some sample of this frame | — |
| `zero` | an analogue control | the control is at rest at the end of the frame | — |
| `read` | an analogue control | always | the current value |

**INP-052** (MUST) A press and a release of one control within one frame make both `down` and `up` true for that frame, and `held` false, so a press shorter than a frame is never lost (ADR-0029, DEC-096, R-CTRL-010). Two presses within one frame make `down` true once; the router keeps the count of presses for diagnostics.

**INP-053** (MUST) An event that the operating system's keyboard auto-repeat produces never satisfies `down`, never creates a new press and never changes `held`; the `repeat` condition uses the router's own timers (§4.7, ADR-0029).

### 3.2 Device controls and the name table

**INP-055** (MUST) One name table maps every bindable control to exactly one canonical name, and every name to exactly one control; the binding parser, the binding writer, display and saving all use it (DEC-554, ADR-0029). Names are lower case. The parser matches control names without regard to case, and the writer always writes the canonical name, so a binding written and read back is the same binding.

**INP-056** (MUST) Keys are identified by the key the active keyboard layout reports, so letter and punctuation keys follow the labels of the active layout; the keyboard-layout layer (§4.13) restores the reference key positions for camera control. Each modifier name covers both of its keys (`shift`, `control`, `alt`). The keyboard names are those of table 3.2-A; a key outside the table has no name, cannot be bound and produces no event.

Table 3.2-A Keyboard names (display text `ui.input.key.<name>`).

| Group | Canonical names |
|---|---|
| letters | `a` to `z` |
| digits | `0` to `9` (top row) |
| function keys | `f1` to `f24`; `f10` is an ordinary key (DEC-096) |
| editing and navigation | `backspace`, `tab`, `enter`, `space`, `escape`, `insert`, `delete`, `home`, `end`, `page_up`, `page_down`, `left`, `up`, `right`, `down` |
| modifiers and locks | `shift`, `control`, `alt`, `meta`, `caps_lock`, `num_lock`, `scroll_lock` |
| system | `pause`, `print_screen`, `application` |
| numeric pad | `num_0` to `num_9`, `num_multiply`, `num_plus`, `num_separator`, `num_minus`, `num_decimal`, `num_divide` |
| punctuation | `semicolon`, `equals`, `comma`, `minus`, `period`, `slash`, `backquote`, `bracket_left`, `backslash`, `bracket_right`, `quote`, `oem_8` |

The numeric-pad minus key and the separator key have their own names and codes, and `meta` resolves to its own key code (DEC-554).

Table 3.2-B Mouse controls (display text `ui.input.mouse.<name>`).

| Name | Kind | Conditions | Value |
|---|---|---|---|
| `primary`, `secondary`, `middle` | Boolean | `down`, `up`, `held`, `repeat` | — |
| `motion` | Axis2 | `move`, `read` | the pointer's movement since the previous frame, in client pixels, y positive downwards |
| `raw_motion` | Axis2 | `move`, `read` | the unaccelerated device movement since the previous frame, in device counts (ADR-0028) |
| `wheel` | Axis1 | `move`, `read` | wheel notches since the previous frame, forward positive, fractional for fine-resolution wheels (`inp.mouse.wheel_units_per_notch`) |

Table 3.2-C Gamepad controls, named by position (display text `ui.input.pad.<name>`).

| Name | Kind | Boolean view | Analogue conditions | Sensitivity option |
|---|---|---|---|---|
| `face_south`, `face_east`, `face_west`, `face_north` | Boolean | the button | — | no |
| `shoulder_left`, `shoulder_right` | Boolean | the button | — | no |
| `dpad_left`, `dpad_right`, `dpad_up`, `dpad_down` | Boolean | the direction | — | no |
| `menu`, `view` | Boolean | the button | — | no |
| `trigger_left`, `trigger_right` | Axis1 | the trigger above its threshold | `position`, `move`, `release`, `zero`, `read` | yes |
| `stick_left`, `stick_right` | Axis2 | the stick's press | `position`, `move`, `release`, `zero`, `read` | yes |
| `stick_left_left`, `stick_left_right`, `stick_left_up`, `stick_left_down`, `stick_right_left`, `stick_right_right`, `stick_right_up`, `stick_right_down` | Boolean | the stick deflected in that direction beyond `inp.pad.stick_button.threshold` | — | no |
| `connection` | Boolean | the local player's gamepad is connected (§4.10) | — | no |

Table 3.2-D Conditions accepted per control kind, and thresholds.

| Control | Boolean conditions | Analogue conditions | Rest zone | Move threshold |
|---|---|---|---|---|
| key, mouse button, gamepad button, d-pad, stick direction | `down`, `up`, `held`, `repeat` | none | — | — |
| `connection` | `down`, `up`, `held` | none | — | — |
| `trigger_left`, `trigger_right` | `down`, `up`, `held`, `repeat` | all five | below `inp.pad.trigger.threshold` | any change |
| `stick_left`, `stick_right` | `down`, `up`, `held`, `repeat` (the press) | all five | inside the radial dead zone (§4.6) | `inp.pad.stick.move_threshold` on either axis |
| `motion`, `raw_motion`, `wheel` | none | `move`, `read` | — | any change |

A condition that a control does not accept is rejected when the binding loads (DEC-553, DEC-554).

### 3.3 Binding nodes

**INP-060** (MUST) A binding is a tree of nodes of these kinds:

| Node | Operands | Result kind | Rules |
|---|---|---|---|
| device leaf (`InputSourceNode`) | a device (`key`, `mouse` or `pad`), a control, a condition and, on sticks and triggers only, a sensitivity | `Boolean` for a Boolean condition; the control's kind for an analogue condition | the condition must be one the control accepts (table 3.2-D); the sensitivity is a whole percentage within the range of `inp.binding.sensitivity`, default 100 |
| `all` | two or more operands of any kind except `all` | the kind of its analogue operand, otherwise `Boolean` | at most one operand may be analogue |
| `chord` | two or more parts, each a key, mouse button or gamepad button without a condition | `Boolean` | true when every part is held and at least one part went down in this frame (INP-112) |
| `not` | one `Boolean` operand: a device leaf, `not`, `alias`, `latch`, `idle` (reaches or no longer), `setting` or `value` | `Boolean` | the operand must be `Boolean` |
| `alias` | an action id | `Boolean` | true when that action fires; an alias chain that leads back to its own action is rejected (DEC-554) |
| `latch` | an `on` operand and an `off` operand, both `Boolean` | `Boolean` | §4.8 |
| `idle` | `at_least` N, `reaches` N or `no_longer`; N a whole number of seconds of at least 1 | `Axis1` for `at_least` (x = the idle time), otherwise `Boolean` | §4.8 |
| `setting` | a setting name and an optional negation | `Boolean` | the setting must exist in the settings store or its defaults when the binding loads; it can be negated once |
| `value` | `AlwaysTrue` or `FeatureFlagX` | `Boolean` | `AlwaysTrue` is always true; `FeatureFlagX` is true while the feature flag `FeatureFlagX` is above 0 |

**INP-061** (MUST) An action accepts a binding by the action's kind:

| Action kind | Accepts |
|---|---|
| `Boolean` | `Boolean` bindings; and analogue bindings, which are true while their condition holds and pass their value on |
| `Axis1` | analogue bindings; an `Axis2` binding gives its x |
| `Axis2` | analogue bindings; an `Axis1` binding gives its x, with y = 0 |

A `Boolean` binding on an `Axis1` or `Axis2` action is rejected when it loads (§6.1).

### 3.4 Action bindings

**INP-065** (MUST) For each action the router holds an ordered list of bindings, an optional glyph override and a suppression flag. The order of the list is the order in which bindings were accepted; index 0 is the **primary** binding, which the layout layer, the user layer and rebinding replace. Binding an entry appends it to the list; replacing removes the primary and puts the new binding at index 0, or appends it when the list is empty.

**INP-066** (MUST) The router owns the action bindings. `Rebuild` creates them at start-up and recreates them on a keyboard-layout change, on restoring the defaults and after the user layer changes; binding nodes are immutable after loading. The run-time state of a binding (repeat timers per device, latch states) lives in a state table owned by the router, indexed by node, and `Rebuild` resets it.

**INP-067** (MUST) The action id table is built from tables 3.6-A and 3.6-B when bindings load; every action id is interned once. An entry that names an action id outside the table is rejected (§6.1).

### 3.5 Binding-set schema

**INP-070** (MUST) The default binding set and each layout layer are records of one binding-set schema ([data schemas](data_schemas/README.md), ADR-0061, ADR-0062) with these fields: the schema version; the layer kind (`default` or `layout`); for a layout layer, its layout tag (§4.13); and a list of entries, each with an action id, a list of binding nodes and an optional glyph asset id. A layout-layer entry holds exactly one binding, which replaces the action's primary binding. The user layer is a group of settings-store entries, one per action, whose value is one binding node in canonical form (DEC-250).

**INP-071** (MUST) Each node is a JSON object with exactly one node-kind property; the notation of this document maps onto it one to one:

| Notation | JSON node |
|---|---|
| `key:a:held` | `{"key": "a", "when": "held"}` |
| `mouse:primary:down` | `{"mouse": "primary", "when": "down"}` |
| `pad:stick_left:position`, sensitivity 150 | `{"pad": "stick_left", "when": "position", "sensitivity": 150}` |
| `all(x, y)` | `{"all": [x, y]}` |
| `chord(key:control, key:c)` | `{"chord": [{"key": "control"}, {"key": "c"}]}` |
| `not(x)` | `{"not": x}` |
| `alias(action.ui.show_cursor)` | `{"alias": "action.ui.show_cursor"}` |
| `latch(on: x, off: y)` | `{"latch": {"on": x, "off": y}}` |
| `idle:at_least:60`, `idle:reaches:60`, `idle:no_longer` | `{"idle": "at_least", "seconds": 60}`, `{"idle": "reaches", "seconds": 60}`, `{"idle": "no_longer"}` |
| `setting:Name`, `setting:not:Name` | `{"setting": "Name"}`, `{"setting": "Name", "negate": true}` |
| `value:AlwaysTrue`, `value:FeatureFlagX` | `{"value": "AlwaysTrue"}`, `{"value": "FeatureFlagX"}` |

**INP-072** (MUST) The canonical writer writes the properties in the order of the table, the canonical control names, and the sensitivity only when it differs from the default. Writing a binding and parsing it back yields the same binding (ADR-0029, DEC-800).

**INP-073** (MUST) A node with an unknown property, with no or with two node-kind properties, or without a required property is rejected (§6.1); nothing falls back to a default condition (DEC-096).

### 3.6 Control catalogue: the default binding set

**INP-075** (MUST) The default binding set of the match actions is table 3.6-A, keyed by the action ids of reference/06 table 2.1-A. Each binding is stated with its effective meaning (DEC-553). In every entry the keyboard and mouse bindings come before the gamepad bindings, so the first keyboard or mouse binding is the primary binding; an action with gamepad bindings only has its first gamepad binding as primary. The "Kind" column is the action's value kind (INP-050).

Table 3.6-A Default bindings of the match actions.

| # | Action id | Kind | Keyboard and mouse | Gamepad | Glyph override | reference/06 row |
|---|---|---|---|---|---|---|
| 1 | `action.camera.pan_left` | Boolean | `key:a:held` · `key:left:held` | — | — | A1 |
| 2 | `action.camera.pan_right` | Boolean | `key:d:held` · `key:right:held` | — | — | A1 |
| 3 | `action.camera.pan_forward` | Boolean | `key:w:held` · `key:up:held` | — | — | A1 |
| 4 | `action.camera.pan_back` | Boolean | `key:s:held` · `key:down:held` | — | — | A1 |
| 5 | `action.camera.pan_stick` | Axis2 | — | `pad:stick_left:position` | left stick (no asset) | A1 |
| 6 | `action.camera.fast_pan` | Boolean | `key:shift:held` | `pad:stick_left:held` | — | A2 |
| 7 | `action.camera.raise` | Boolean | `key:q:held` | `pad:trigger_right:position` | left stick (no asset) | A3 |
| 8 | `action.camera.lower` | Boolean | `key:e:held` | `pad:trigger_left:position` | right stick (no asset) | A3 |
| 9 | `action.camera.height_wheel` | Axis1 | `mouse:wheel:move` | — | — | A3 |
| 10 | `action.camera.aim` | Axis2 | `mouse:motion:move` | — | right stick (no asset) | A4 |
| 11 | `action.camera.aim_stick` | Axis2 | — | `pad:stick_right:position` | — | A4 |
| 12 | `action.camera.zoom` | Boolean | `key:z:held` · `mouse:middle:held` | `pad:stick_right:held` | — | A5 |
| 13 | `action.camera.zoom_out` | Boolean | `key:c:held` | — | — | A11 |
| 14 | `action.camera.look_around` | Boolean | `key:f2:held` | `pad:stick_right:held` | — | A6 |
| 15 | `action.camera.track_closer` | Boolean | — | `pad:dpad_up:held` | — | A7 |
| 16 | `action.camera.track_farther` | Boolean | — | `pad:dpad_down:held` | — | A7 |
| 17 | `action.view.chat_log` | Boolean | `key:l:held` | — | — | A8 |
| 18 | `action.view.hide_interface` | Boolean | `all(key:alt:held, key:x:down)` | — | — | A9 |
| 19 | `action.view.screenshot` | Boolean | `key:p:down` | — | — | A10 |
| 20 | `action.select.pick` | Boolean | `mouse:primary:down` | `pad:face_south:down` | `glyph.pad.face_south` | S1, O10 |
| 21 | `action.select.pick_idle` | Boolean | `mouse:primary:down` | `pad:face_south:down` | — | S1 |
| 22 | `action.select.clear` | Boolean | `key:space:down` | `pad:face_east:down` | `glyph.pad.face_east` | S3 |
| 23 | `action.select.circle_begin` | Boolean | `mouse:primary:down` | `pad:face_south:down` | — | S4 |
| 24 | `action.select.circle_grow` | Boolean | `mouse:primary:held` | `pad:face_south:held` | — | S5 |
| 25 | `action.select.circle_end` | Boolean | `mouse:primary:up` | `pad:face_south:up` | — | S6 |
| 26 | `action.power.next` | Boolean | `key:bracket_right:down` · `key:tab:down` | `pad:shoulder_right:down` | `glyph.pad.shoulder_right` | S7 |
| 27 | `action.power.previous` | Boolean | `key:bracket_left:down` | `pad:shoulder_left:down` | `glyph.pad.shoulder_left` | S8 |
| 28 | `action.power.slot_1` | Boolean | `key:1:down` | — | — | S9 |
| 29 | `action.power.slot_2` | Boolean | `key:2:down` | — | — | S9 |
| 30 | `action.power.slot_3` | Boolean | `key:3:down` | — | — | S9 |
| 31 | `action.power.slot_4` | Boolean | `key:4:down` | — | — | S9 |
| 32 | `action.power.slot_5` | Boolean | `key:5:down` | — | — | S9 |
| 33 | `action.power.slot_6` | Boolean | `key:6:down` | — | — | S9 |
| 34 | `action.power.slot_7` | Boolean | `key:7:down` | — | — | S9 |
| 35 | `action.power.slot_8` | Boolean | `key:8:down` | — | — | S9 |
| 36 | `action.power.slot_9` | Boolean | `key:9:down` | — | — | S9 |
| 37 | `action.power.activate_slot` | Boolean | `mouse:primary:up` · `key:enter:down` | `pad:face_south:up` | — | S11 |
| 38 | `action.power.place` | Boolean | `mouse:primary:down` | `pad:face_south:down` | — | T1 |
| 39 | `action.power.cancel` | Boolean | `key:c:down` | `pad:face_north:down` | `glyph.pad.face_north` | T3, T6 |
| 40 | `action.power.end_current` | Boolean | — | — | — | T7 |
| 41 | `action.power.panel_show` | Boolean | `key:tab:down` | `pad:view:down` | `glyph.pad.face_west` | T8 |
| 42 | `action.power.panel_hide` | Boolean | `key:tab:up` | `pad:view:down` · `pad:face_east:down` | — | T8 |
| 43 | `action.order.crowd` | Boolean | `mouse:secondary:down` | `pad:face_west:down` | — | O1 |
| 44 | `action.order.straight_route` | Boolean | `key:control:held` | — | — | O1, O6 |
| 45 | `action.order.promote` | Boolean | `mouse:secondary:down` | `pad:face_west:down` | — | O2 to O5 |
| 46 | `action.order.special` | Boolean | `mouse:secondary:down` | `pad:face_south:down` | — | O6 to O9, T2 |
| 47 | `action.order.move_modifier` | Boolean | `key:shift:held` | `pad:face_west:down` | — | O6 to O9 |
| 48 | `action.order.demote` | Boolean | `key:c:down` | `pad:face_north:down` | — | T5 |
| 49 | `action.order.scrap` | Boolean | `key:c:down` | `pad:face_north:down` | — | T4 |
| 50 | `action.direct.move_hold` | Boolean | `mouse:primary:held` | — | — | D1 |
| 51 | `action.direct.move_stick` | Axis2 | — | `pad:stick_left:position` | left stick (no asset) | D1 |
| 52 | `action.direct.fire` | Boolean | `mouse:secondary:held` | `pad:trigger_right:held` | — | D2, D5 |
| 53 | `action.direct.secondary_fire` | Boolean | `all(mouse:primary:down, mouse:secondary:held)` | `pad:trigger_left:down` | — | D3 |
| 54 | `action.direct.aim_stick` | Axis2 | — | `pad:stick_right:position` | — | D2 |
| 55 | `action.direct.secondary_fire_directed` | Axis2 | — | — | `glyph.pad.trigger_left` | D3 |
| 56 | `action.direct.set_target` | Boolean | `mouse:secondary:held` | `pad:face_west:held` | `glyph.pad.face_west` | D4, D5 |
| 57 | `action.direct.set_target_end` | Boolean | `mouse:secondary:up` | `pad:face_west:up` | — | D1 |
| 58 | `action.direct.move_direction_change` | Axis2 | — | `pad:stick_left:move` | — | camera R-CTRL-487 |
| 59 | `action.match.ready` | Boolean | `key:space:down` | `pad:face_south:down` · `pad:face_east:down` | — | M1 |
| 60 | `action.match.chat` | Boolean | `key:enter:down` | — | — | M2 |
| 61 | `action.match.pause` | Boolean | `key:pause:down` | — | — | M3 |
| 62 | `action.match.menu` | Boolean | `key:escape:down` | `pad:menu:down` | — | M4 |
| 63 | `action.debug.network_stats` | Boolean | `key:f9:down` | — | — | M5 |
| 64 | `action.ui.pointer_down` | Boolean | `mouse:primary:down` | `all(alias(action.ui.show_cursor), pad:face_south:down)` | — | M6 |
| 65 | `action.ui.pointer_held` | Boolean | `mouse:primary:held` | `all(alias(action.ui.show_cursor), pad:face_south:held)` | — | M6 |
| 66 | `action.ui.alt_pointer_down` | Boolean | `mouse:secondary:down` | — | — | M6 |
| 67 | `action.ui.show_cursor` | Boolean | — | `pad:shoulder_left:held` | — | M6 |
| 68 | `action.device.pad_disconnected` | Boolean | — | `pad:connection:up` | — | M7 |
| 69 | `action.device.pad_connected` | Boolean | — | `pad:connection:down` | — | M7 |

**INP-076** (MUST) `action.app.quit` has no binding. The platform layer raises it when the window receives a close request, which Alt+F4 also produces (ADR-0028, ADR-0075, R-CTRL-047 row M8). The key events of Alt and F4 still reach the router as ordinary events.

**INP-077** (MUST) The default binding set of the UI navigation and hook-page actions is table 3.6-B. The UI framework consumes these actions in its update phase (ADR-0032); the three leaderboard actions serve a hook page with no data source (DEC-047, R-META-500 to R-META-502). The match actions `action.match.menu`, `action.ui.pointer_down`, `action.ui.pointer_held` and `action.ui.alt_pointer_down` also act on UI windows (R-CTRL-047 rows M4 and M6).

Table 3.6-B Default bindings of the UI navigation and hook-page actions.

| # | Action id | Kind | Keyboard and mouse | Gamepad | Glyph override | Use |
|---|---|---|---|---|---|---|
| 1 | `action.menu.left` | Boolean | — | `pad:dpad_left:repeat` · `pad:stick_left_left:repeat` · `pad:stick_right_left:repeat` | d-pad left and right (no asset) | moves focus left |
| 2 | `action.menu.right` | Boolean | — | `pad:dpad_right:repeat` · `pad:stick_left_right:repeat` · `pad:stick_right_right:repeat` | — | moves focus right |
| 3 | `action.menu.up` | Boolean | — | `pad:dpad_up:repeat` · `pad:stick_left_up:repeat` · `pad:stick_right_up:repeat` | — | moves focus up |
| 4 | `action.menu.down` | Boolean | — | `pad:dpad_down:repeat` · `pad:stick_left_down:repeat` · `pad:stick_right_down:repeat` | — | moves focus down |
| 5 | `action.menu.activate` | Boolean | — | `pad:face_south:down` | `glyph.pad.face_south` | presses the focused widget |
| 6 | `action.menu.close` | Boolean | — | `pad:view:down` · `pad:face_east:down` | — | invokes the window's Back |
| 7 | `action.menu.start` | Boolean | — | `pad:menu:down` | — | the gamepad menu button on a front-end window |
| 8 | `action.menu.dismiss_help` | Boolean | `mouse:primary:down` · `key:space:down` · `key:enter:down` · `key:escape:down` | `pad:face_south:down` | — | closes a front-end help pop-up |
| 9 | `action.menu.scroll_speedup` | Boolean | `key:shift:held` | — | — | speeds up a held scroller or spinner |
| 10 | `action.menu.cycle_up` | Boolean | — | `pad:shoulder_left:repeat` | — | `PadMenuCycleUp`: previous board on the leaderboard page |
| 11 | `action.menu.cycle_down` | Boolean | — | `pad:shoulder_right:repeat` | — | `PadMenuCycleDown`: next board on the leaderboard page |
| 12 | `action.menu.select_previous` | Boolean | — | `pad:trigger_left:down` | — | previous view on the leaderboard page |
| 13 | `action.menu.select_next` | Boolean | — | `pad:trigger_right:down` | — | next view on the leaderboard page |
| 14 | `action.menu.toggle_period` | Boolean | — | `pad:face_north:down` | — | period toggle on the leaderboard page |

**INP-078** (MUST) `action.power.end_current` and `action.direct.secondary_fire_directed` have no default binding. They are queried like every other action and fire only when the user layer binds them.

**INP-079** (MUST) Three default bindings are trigger press bindings, which fire once when the trigger goes down: `pad:trigger_left:down` of `action.direct.secondary_fire`, and the trigger bindings of `action.menu.select_previous` and `action.menu.select_next` (DEC-553). The gamepad bindings of `action.ui.pointer_down` and `action.ui.pointer_held` are conjunctions with an alias of `action.ui.show_cursor`: while the left shoulder is held, the south face button acts as the UI pointer button (DEC-553, R-CTRL-047 row M6).

### 3.7 Glyphs

**INP-080** (MUST) A control's glyph is looked up by its control in table 3.7-A; the UI framework draws the darkening shadow first and the glyph on top. A control without a row has no glyph asset, and the UI shows its display text instead.

Table 3.7-A Glyph assets by control.

| Control | Glyph | Shadow |
|---|---|---|
| `pad:face_south` | `glyph.pad.face_south` | `glyph.pad.face_shadow` |
| `pad:face_east` | `glyph.pad.face_east` | `glyph.pad.face_shadow` |
| `pad:face_west` | `glyph.pad.face_west` | `glyph.pad.face_shadow` |
| `pad:face_north` | `glyph.pad.face_north` | `glyph.pad.face_shadow` |
| `pad:shoulder_left` | `glyph.pad.shoulder_left` | `glyph.pad.shoulder_left_shadow` |
| `pad:shoulder_right` | `glyph.pad.shoulder_right` | `glyph.pad.shoulder_right_shadow` |
| `pad:trigger_left` | `glyph.pad.trigger_left` | `glyph.pad.trigger_shadow` |
| `pad:trigger_right` | `glyph.pad.trigger_right` | `glyph.pad.trigger_shadow` |
| `key:tab` | `glyph.key.tab` | none |

**INP-081** (MUST) An action may carry one glyph override in the binding set (the "Glyph override" columns of tables 3.6-A and 3.6-B); when an action has several glyph entries, the last one wins. `Glyph(action)` returns the override. The overrides that name a stick or d-pad glyph have no asset yet; until one exists, the UI falls back to the display text of the action's primary gamepad control.

**INP-082** (MUST) The glyph overrides of `action.camera.raise` (the left-stick glyph) and `action.camera.lower` (the right-stick glyph) are part of the default set as table 3.6-A states, although both actions are bound to triggers on the gamepad.

### 3.8 Reverse index

**INP-085** (MUST) Several actions share one physical input. The router never resolves the overlap: one press satisfies every action bound to it, and the consumers' context rules decide which action applies, for the secondary button and the west and south face buttons by the resolver of reference/06 (R-CTRL-050). Table 3.8-A lists, for every physical input of the default set, the actions bound to it; an operand of a conjunction is marked.

Table 3.8-A Physical input to actions, default binding set.

| Physical input | Actions (condition) | Count |
|---|---|---|
| `key:1` | `action.power.slot_1` (down) | 1 |
| `key:2` | `action.power.slot_2` (down) | 1 |
| `key:3` | `action.power.slot_3` (down) | 1 |
| `key:4` | `action.power.slot_4` (down) | 1 |
| `key:5` | `action.power.slot_5` (down) | 1 |
| `key:6` | `action.power.slot_6` (down) | 1 |
| `key:7` | `action.power.slot_7` (down) | 1 |
| `key:8` | `action.power.slot_8` (down) | 1 |
| `key:9` | `action.power.slot_9` (down) | 1 |
| `key:a` | `action.camera.pan_left` (held) | 1 |
| `key:alt` | `action.view.hide_interface` (held, operand of all) | 1 |
| `key:bracket_left` | `action.power.previous` (down) | 1 |
| `key:bracket_right` | `action.power.next` (down) | 1 |
| `key:c` | `action.camera.zoom_out` (held), `action.power.cancel` (down), `action.order.demote` (down), `action.order.scrap` (down) | 4 |
| `key:control` | `action.order.straight_route` (held) | 1 |
| `key:d` | `action.camera.pan_right` (held) | 1 |
| `key:down` | `action.camera.pan_back` (held) | 1 |
| `key:e` | `action.camera.lower` (held) | 1 |
| `key:enter` | `action.power.activate_slot` (down), `action.match.chat` (down), `action.menu.dismiss_help` (down) | 3 |
| `key:escape` | `action.match.menu` (down), `action.menu.dismiss_help` (down) | 2 |
| `key:f2` | `action.camera.look_around` (held) | 1 |
| `key:f9` | `action.debug.network_stats` (down) | 1 |
| `key:l` | `action.view.chat_log` (held) | 1 |
| `key:left` | `action.camera.pan_left` (held) | 1 |
| `key:p` | `action.view.screenshot` (down) | 1 |
| `key:pause` | `action.match.pause` (down) | 1 |
| `key:q` | `action.camera.raise` (held) | 1 |
| `key:right` | `action.camera.pan_right` (held) | 1 |
| `key:s` | `action.camera.pan_back` (held) | 1 |
| `key:shift` | `action.camera.fast_pan` (held), `action.order.move_modifier` (held), `action.menu.scroll_speedup` (held) | 3 |
| `key:space` | `action.select.clear` (down), `action.match.ready` (down), `action.menu.dismiss_help` (down) | 3 |
| `key:tab` | `action.power.next` (down), `action.power.panel_show` (down), `action.power.panel_hide` (up) | 3 |
| `key:up` | `action.camera.pan_forward` (held) | 1 |
| `key:w` | `action.camera.pan_forward` (held) | 1 |
| `key:x` | `action.view.hide_interface` (down, operand of all) | 1 |
| `key:z` | `action.camera.zoom` (held) | 1 |
| `mouse:middle` | `action.camera.zoom` (held) | 1 |
| `mouse:motion` | `action.camera.aim` (move) | 1 |
| `mouse:primary` | `action.select.pick` (down), `action.select.pick_idle` (down), `action.select.circle_begin` (down), `action.select.circle_grow` (held), `action.select.circle_end` (up), `action.power.activate_slot` (up), `action.power.place` (down), `action.direct.move_hold` (held), `action.direct.secondary_fire` (down, operand of all), `action.ui.pointer_down` (down), `action.ui.pointer_held` (held), `action.menu.dismiss_help` (down) | 12 |
| `mouse:secondary` | `action.order.crowd` (down), `action.order.promote` (down), `action.order.special` (down), `action.direct.fire` (held), `action.direct.secondary_fire` (held, operand of all), `action.direct.set_target` (held), `action.direct.set_target_end` (up), `action.ui.alt_pointer_down` (down) | 8 |
| `mouse:wheel` | `action.camera.height_wheel` (move) | 1 |
| `pad:connection` | `action.device.pad_disconnected` (up), `action.device.pad_connected` (down) | 2 |
| `pad:dpad_down` | `action.camera.track_farther` (held), `action.menu.down` (repeat) | 2 |
| `pad:dpad_left` | `action.menu.left` (repeat) | 1 |
| `pad:dpad_right` | `action.menu.right` (repeat) | 1 |
| `pad:dpad_up` | `action.camera.track_closer` (held), `action.menu.up` (repeat) | 2 |
| `pad:face_east` | `action.select.clear` (down), `action.power.panel_hide` (down), `action.match.ready` (down), `action.menu.close` (down) | 4 |
| `pad:face_north` | `action.power.cancel` (down), `action.order.demote` (down), `action.order.scrap` (down), `action.menu.toggle_period` (down) | 4 |
| `pad:face_south` | `action.select.pick` (down), `action.select.pick_idle` (down), `action.select.circle_begin` (down), `action.select.circle_grow` (held), `action.select.circle_end` (up), `action.power.activate_slot` (up), `action.power.place` (down), `action.order.special` (down), `action.match.ready` (down), `action.ui.pointer_down` (down, operand of all), `action.ui.pointer_held` (held, operand of all), `action.menu.activate` (down), `action.menu.dismiss_help` (down) | 13 |
| `pad:face_west` | `action.order.crowd` (down), `action.order.promote` (down), `action.order.move_modifier` (down), `action.direct.set_target` (held), `action.direct.set_target_end` (up) | 5 |
| `pad:menu` | `action.match.menu` (down), `action.menu.start` (down) | 2 |
| `pad:shoulder_left` | `action.power.previous` (down), `action.ui.show_cursor` (held), `action.menu.cycle_up` (repeat) | 3 |
| `pad:shoulder_right` | `action.power.next` (down), `action.menu.cycle_down` (repeat) | 2 |
| `pad:stick_left` | `action.camera.pan_stick` (position), `action.camera.fast_pan` (held), `action.direct.move_stick` (position), `action.direct.move_direction_change` (move) | 4 |
| `pad:stick_left_down` | `action.menu.down` (repeat) | 1 |
| `pad:stick_left_left` | `action.menu.left` (repeat) | 1 |
| `pad:stick_left_right` | `action.menu.right` (repeat) | 1 |
| `pad:stick_left_up` | `action.menu.up` (repeat) | 1 |
| `pad:stick_right` | `action.camera.aim_stick` (position), `action.camera.zoom` (held), `action.camera.look_around` (held), `action.direct.aim_stick` (position) | 4 |
| `pad:stick_right_down` | `action.menu.down` (repeat) | 1 |
| `pad:stick_right_left` | `action.menu.left` (repeat) | 1 |
| `pad:stick_right_right` | `action.menu.right` (repeat) | 1 |
| `pad:stick_right_up` | `action.menu.up` (repeat) | 1 |
| `pad:trigger_left` | `action.camera.lower` (position), `action.direct.secondary_fire` (down), `action.menu.select_previous` (down) | 3 |
| `pad:trigger_right` | `action.camera.raise` (position), `action.direct.fire` (held), `action.menu.select_next` (down) | 3 |
| `pad:view` | `action.power.panel_show` (down), `action.power.panel_hide` (down), `action.menu.close` (down) | 3 |
| `alias:action.ui.show_cursor` | `action.ui.pointer_down` (operand of all), `action.ui.pointer_held` (operand of all) | 2 |

### 3.9 Action values and suppression

**INP-090** (MUST) An action value (`ActionValue`) holds the value kind, the x and y components and the device that produced it. The value of a fired action is the value of its first satisfied binding (INP-020): a device leaf gives its control's value under its condition (INP-051); `all` gives its first operand's value, replaced by its analogue operand's value when it has one; `alias` gives the target action's value; the other composite nodes give no axis value.

**INP-091** (MUST) Values are floating-point and never truncated or rounded (ADR-0029). A Boolean action fed by an analogue binding carries that binding's value; an `Axis1` action reads x; an `Axis2` action fed by an `Axis1` binding has y = 0 (INP-061).

**INP-092** (MUST) A value carries the identity of the device that produced it; a gamepad value always names the player's assigned gamepad (§4.10), and a keyboard or mouse value names the keyboard or the mouse.

**INP-093** (MUST) `Suppress(action)` makes the action answer false, with a cleared value, for the rest of the frame; an `alias` of a suppressed action is false as well. `AdvanceFrame` clears every suppression flag first. The UI input pump suppresses the match's use of a primary press that lands on a UI window, so that the press is withheld from the match actions for the rest of the frame (R-CTRL-047 row M6).

### 3.10 Class diagram

```mermaid
classDiagram
  class InputRouter {
    +AdvanceFrame(gates)
    +AdvanceRepeatPass()
    +Query(player, action, value) bool
    +QueryBinding(player, node, value) bool
    +Suppress(action)
    +Mode(player) InputMode
    +Pointer() PointerState
    +WarpPointer(position)
    +ConfinePointer(on)
    +Rebuild(sources) LoadReport
    +ReplacePrimaryBinding(action, binding)
    +CaptureFirstActiveInput(player, classes, instant)
    +Describe(action)
    +Glyph(action)
  }
  class DeviceBackEnd {
    <<interface>>
    +Initialise(platform) bool
    +Collect(sink)
    +ReleaseAll(sink)
    +Shutdown()
  }
  class GamepadBackEnd
  class KeyboardMouseBackEnd
  class InputEventQueue {
    +events : ordered list of InputEvent
    +dropped : count
  }
  class DeviceState {
    +held, presses, releases per control
    +analogue end value, previous value
  }
  class ActionBindings {
    +bindings : ordered list of BindingNode
    +glyph : asset id
    +suppressed : flag
  }
  class BindingNode {
    <<variant>>
  }
  class InputSourceNode
  class AllNode
  class ChordNode
  class NotNode
  class AliasNode
  class LatchNode
  class IdleNode
  class SettingNode
  class ValueNode
  class BindingStateTable {
    +repeat timers per binding and device
    +latch states
  }
  class PadAssignment {
    +assigned gamepad per local player
  }
  class InputPointer
  class MatchInputPump
  class UiInputPump
  DeviceBackEnd <|-- GamepadBackEnd
  DeviceBackEnd <|-- KeyboardMouseBackEnd
  InputRouter o-- DeviceBackEnd
  InputRouter *-- InputEventQueue
  InputRouter *-- DeviceState
  InputRouter *-- ActionBindings
  InputRouter *-- BindingStateTable
  InputRouter *-- PadAssignment
  InputRouter *-- InputPointer
  ActionBindings o-- BindingNode
  BindingNode <|-- InputSourceNode
  BindingNode <|-- AllNode
  BindingNode <|-- ChordNode
  BindingNode <|-- NotNode
  BindingNode <|-- AliasNode
  BindingNode <|-- LatchNode
  BindingNode <|-- IdleNode
  BindingNode <|-- SettingNode
  BindingNode <|-- ValueNode
  MatchInputPump ..> InputRouter : queries actions
  UiInputPump ..> InputRouter : queries and suppresses
```

Caption: Diagram 3.10-A, the input component's types: two back-ends feed one event queue, the router derives the device state, and consumers query actions through the binding nodes (INP-001, INP-060, INP-065).
