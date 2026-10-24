# Input: controls and default bindings

> Status: Engine baseline · Prefix: INP · ADRs: [ADR-028](../../../ADR/ADR-028-input-device-back-ends.md) · [ADR-029](../../../ADR/ADR-029-input-event-model-and-binding-contract.md) · [ADR-074](../../../ADR/ADR-074-platform-clock-with-injectable-time.md) · Folder: [Input](README.md)

This document specifies the bindable controls and the default bindings of the input component: the name table of every key, mouse control and gamepad control, the default binding set of the match and UI navigation actions, the glyphs, the reverse index, the development binding layer and the conditional actions, and the notes on the default scheme. Section numbers are shared by the files of this folder, and the notation is that of the [README](README.md).

## 1 Responsibilities and boundaries

The responsibilities and boundaries of the input component are [README](README.md) §1 (INP-001 to INP-010).

## 2 Interfaces

The interface sketch and the router operations are [README](README.md) §2.

## 3 Data model

### 3.2 Device controls and the name table

**INP-055** (MUST) One name table maps every bindable control to exactly one canonical name, and every name to exactly one control; the binding parser, the binding writer, display and saving all use it (DEC-554, ADR-029). Names are lower case. The parser matches control names without regard to case, and the writer always writes the canonical name, so a binding written and read back is the same binding.

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
| `raw_motion` | Axis2 | `move`, `read` | the unaccelerated device movement since the previous frame, in device counts (ADR-028) |
| `wheel` | Axis1 | `move`, `read` | wheel notches since the previous frame, forward positive, fractional for fine-resolution wheels (`inp.mouse.wheel_units_per_notch`) |

> **Amendment 2026-10-24 (owner decision O108, [ADR-180](../../../ADR/ADR-180-the-camera-over-a-point-on-the-ground.md)).** Species adds the name `wheel_horizontal`, Axis1, with the conditions `move` and `read`: the horizontal wheel's notches since the previous frame, tilted or slid to the right positive, fractional as `wheel`'s are. A touchpad's two-finger slide sideways arrives as it, and table 3.2-D gives it `wheel`'s row.

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

### 3.6 Control catalogue: the default binding set

**INP-075** (MUST) The default binding set of the match actions is table 3.6-A, keyed by the action ids of reference/06 table 2.1-A. Each binding is stated with its effective meaning (DEC-553). In every entry of tables 3.6-A and 3.6-B the keyboard and mouse bindings come before the gamepad bindings, so the first keyboard or mouse binding is the primary binding; an action with gamepad bindings only has its first gamepad binding as primary. The "Kind" column is the action's value kind (INP-050).

Table 3.6-A Default bindings of the match actions.

| # | Action id | Kind | Keyboard and mouse | Gamepad | Glyph override | reference/06 row |
|---|---|---|---|---|---|---|
| 1 | `action.camera.pan_left` | Boolean | `key:a:held` · `key:left:held` | — | — | A1 |
| 2 | `action.camera.pan_right` | Boolean | `key:d:held` · `key:right:held` | — | — | A1 |
| 3 | `action.camera.pan_forward` | Boolean | `key:w:held` · `key:up:held` | — | — | A1 |
| 4 | `action.camera.pan_back` | Boolean | `key:s:held` · `key:down:held` | — | — | A1 |
| 5 | `action.camera.pan_stick` | Axis2 | — | `pad:stick_left:position` | `glyph.pad.stick_left` | A1 |
| 6 | `action.camera.fast_pan` | Boolean | `key:shift:held` | `pad:stick_left:held` | — | A2 |
| 7 | `action.camera.raise` | Boolean | `key:q:held` | `pad:trigger_right:position` | `glyph.pad.stick_left` | A3 |
| 8 | `action.camera.lower` | Boolean | `key:e:held` | `pad:trigger_left:position` | `glyph.pad.stick_right` | A3 |
| 9 | `action.camera.height_wheel` | Axis1 | `mouse:wheel:move` | — | — | A3 |
| 10 | `action.camera.aim` | Axis2 | `mouse:motion:move` | — | `glyph.pad.stick_right` | A4 |
| 11 | `action.camera.aim_stick` | Axis2 | — | `pad:stick_right:position` | — | A4 |
| 12 | `action.camera.zoom` | Boolean | `key:z:held` · `mouse:middle:held` | `pad:stick_right:held` | — | A5 |
| 13 | `action.camera.look_around` | Boolean | `key:f2:held` | `pad:stick_right:held` | — | A6 |
| 14 | `action.camera.track_closer` | Boolean | — | `pad:dpad_up:held` | — | A7 |
| 15 | `action.camera.track_farther` | Boolean | — | `pad:dpad_down:held` | — | A7 |
| 16 | `action.view.chat_log` | Boolean | `key:l:held` | — | — | A8 |
| 17 | `action.view.hide_interface` | Boolean | `all(key:alt:held, key:x:down)` | — | — | A9 |
| 18 | `action.view.screenshot` | Boolean | `key:p:down` | — | — | A10 |
| 19 | `action.select.pick` | Boolean | `mouse:primary:down` | `pad:face_south:down` | `glyph.pad.face_south` | S1, O10 |
| 20 | `action.select.pick_idle` | Boolean | `mouse:primary:down` | `pad:face_south:down` | — | S1 |
| 21 | `action.select.clear` | Boolean | `key:space:down` | `pad:face_east:down` | `glyph.pad.face_east` | S3 |
| 22 | `action.select.circle_begin` | Boolean | `mouse:primary:down` | `pad:face_south:down` | — | S4 |
| 23 | `action.select.circle_grow` | Boolean | `mouse:primary:held` | `pad:face_south:held` | — | S5 |
| 24 | `action.select.circle_end` | Boolean | `mouse:primary:up` | `pad:face_south:up` | — | S6 |
| 25 | `action.power.next` | Boolean | `key:bracket_right:down` · `key:tab:down` | `pad:shoulder_right:down` | `glyph.pad.shoulder_right` | S7 |
| 26 | `action.power.previous` | Boolean | `key:bracket_left:down` | `pad:shoulder_left:down` | `glyph.pad.shoulder_left` | S8 |
| 27 | `action.power.slot_1` | Boolean | `key:1:down` | — | — | S9 |
| 28 | `action.power.slot_2` | Boolean | `key:2:down` | — | — | S9 |
| 29 | `action.power.slot_3` | Boolean | `key:3:down` | — | — | S9 |
| 30 | `action.power.slot_4` | Boolean | `key:4:down` | — | — | S9 |
| 31 | `action.power.slot_5` | Boolean | `key:5:down` | — | — | S9 |
| 32 | `action.power.slot_6` | Boolean | `key:6:down` | — | — | S9 |
| 33 | `action.power.slot_7` | Boolean | `key:7:down` | — | — | S9 |
| 34 | `action.power.slot_8` | Boolean | `key:8:down` | — | — | S9 |
| 35 | `action.power.slot_9` | Boolean | `key:9:down` | — | — | S9 |
| 36 | `action.power.activate_slot` | Boolean | `mouse:primary:up` · `key:enter:down` | `pad:face_south:up` | — | S11 |
| 37 | `action.power.place` | Boolean | `mouse:primary:down` | `pad:face_south:down` | — | T1 |
| 38 | `action.power.cancel` | Boolean | `key:c:down` | `pad:face_north:down` | `glyph.pad.face_north` | T3, T6 |
| 39 | `action.power.end_current` | Boolean | — | — | — | T7 |
| 40 | `action.order.crowd` | Boolean | `mouse:secondary:down` | `pad:face_west:down` | — | O1 |
| 41 | `action.order.straight_route` | Boolean | `key:control:held` | — | — | O1, O6 |
| 42 | `action.order.promote` | Boolean | `mouse:secondary:down` | `pad:face_west:down` | — | O2 to O5 |
| 43 | `action.order.special` | Boolean | `mouse:secondary:down` | `pad:face_south:down` | — | O6 to O9, T2 |
| 44 | `action.order.move_modifier` | Boolean | `key:shift:held` | `pad:face_west:down` | — | O6 to O9 |
| 45 | `action.order.demote` | Boolean | `key:c:down` | `pad:face_north:down` | — | T5 |
| 46 | `action.order.scrap` | Boolean | `key:c:down` | `pad:face_north:down` | — | T4 |
| 47 | `action.direct.move_hold` | Boolean | `mouse:primary:held` | — | — | D1 |
| 48 | `action.direct.move_stick` | Axis2 | — | `pad:stick_left:position` | `glyph.pad.stick_left` | D1 |
| 49 | `action.direct.fire` | Boolean | `mouse:secondary:held` | `pad:trigger_right:held` | — | D2, D5 |
| 50 | `action.direct.secondary_fire` | Boolean | `all(mouse:primary:down, mouse:secondary:held)` | `pad:trigger_left:down` | — | D3 |
| 51 | `action.direct.aim_stick` | Axis2 | — | `pad:stick_right:position` | — | D2 |
| 52 | `action.direct.secondary_fire_directed` | Axis2 | — | — | `glyph.pad.trigger_left` | D3 |
| 53 | `action.direct.set_target` | Boolean | `mouse:secondary:held` | `pad:face_west:held` | `glyph.pad.face_west` | D4, D5 |
| 54 | `action.direct.move_direction_change` | Axis2 | — | `pad:stick_left:move` | — | camera R-CTRL-487 |
| 55 | `action.match.ready` | Boolean | `key:space:down` | `pad:face_south:down` · `pad:face_east:down` | — | M1 |
| 56 | `action.match.chat` | Boolean | `key:enter:down` | — | — | M2 |
| 57 | `action.match.pause` | Boolean | `key:pause:down` | — | — | M3 |
| 58 | `action.match.menu` | Boolean | `key:escape:down` | `pad:menu:down` | — | M4 |
| 59 | `action.debug.network_stats` | Boolean | `key:f9:down` | — | — | M5 |
| 60 | `action.ui.pointer_down` | Boolean | `mouse:primary:down` | `all(alias(action.ui.show_cursor), pad:face_south:down)` | — | M6 |
| 61 | `action.ui.pointer_held` | Boolean | `mouse:primary:held` | `all(alias(action.ui.show_cursor), pad:face_south:held)` | — | M6 |
| 62 | `action.ui.alt_pointer_down` | Boolean | `mouse:secondary:down` | — | — | M6 |
| 63 | `action.ui.show_cursor` | Boolean | — | `pad:shoulder_left:held` | — | M6 |
| 64 | `action.device.pad_disconnected` | Boolean | — | `pad:connection:up` | — | M7 |
| 65 | `action.device.pad_connected` | Boolean | — | `pad:connection:down` | — | M7 |

> **Amendment 2026-10-23 (owner decision O107, [ADR-179](../../../ADR/ADR-179-the-camera-turns-only-while-the-middle-button-is-held.md)).** Species adds row 66, `action.camera.look`, Boolean, `mouse:middle:held`, with no gamepad binding and no glyph override: while it is held the camera turns towards the pointer ([camera](../../../reference/06_controls_camera_and_commands/camera.md) R-CTRL-471 step 8 amendment). Row 12, `action.camera.zoom`, keeps `key:z:held` alone. In table 3.8-A, `mouse:middle` therefore names `action.camera.look` (held) and no longer `action.camera.zoom`.

> **Amendment 2026-10-24 (owner decision O108, [ADR-180](../../../ADR/ADR-180-the-camera-over-a-point-on-the-ground.md)).** Row 66, `action.camera.look`, is retired and never reused, since the camera held over a ground point has no mouse-look ([camera](../../../reference/06_controls_camera_and_commands/camera.md) R-CTRL-471 amendment). Rows 7 and 8 zoom out and in, bound to `key:page_up:held` and `key:page_down:held`. Species adds these rows, with no gamepad binding and no glyph override:
>
> | Row | Action | Kind | Keyboard and mouse |
> |---|---|---|---|
> | 67 | `action.camera.drag` | Boolean | `mouse:middle:held` |
> | 68 | `action.camera.turn_left` | Boolean | `key:q:held` |
> | 69 | `action.camera.turn_right` | Boolean | `key:e:held` |
> | 70 | `action.camera.turn_drag` | Boolean | `all(key:alt:held, mouse:middle:held)` |
> | 71 | `action.camera.pan_wheel` | Axis1 | `mouse:wheel_horizontal:move` |
> | 72 | `action.camera.home` | Boolean | `key:home:down` |
> | 73 | `action.camera.reset_heading` | Boolean | `key:backspace:down` |
>
> Rows 67 and 70 share the middle button, and the camera turns while row 70 holds and grabs the ground otherwise. In table 3.8-A, `mouse:middle` names `action.camera.drag` and `action.camera.turn_drag` (held), `key:q` and `key:e` name the turns, `key:page_up` and `key:page_down` the zoom, `key:home` the home flight and `key:backspace` the heading's reset. Touch reaches these actions through gestures, not bindings ([ADR-181](../../../ADR/ADR-181-touch-input.md)).

**INP-076** (MUST) `action.app.quit` has no binding. The platform layer raises it when the window receives a close request, which Alt+F4 also produces (ADR-028, ADR-075, R-CTRL-047 row M8). The key events of Alt and F4 still reach the router as ordinary events.

**INP-077** (MUST) The UI navigation actions, their meanings and their context rules are defined by the UI framework ([layout, rendering, input and localisation](../ui_framework/layout_rendering_input_and_localisation.md) UI-203 and UI-204, ADR-032, ADR-064): `action.menu.up`, `action.menu.down`, `action.menu.left`, `action.menu.right`, `action.menu.activate`, `action.menu.front_end_activate`, `action.menu.close`, `action.menu.dismiss` and `action.menu.scroll_speedup`. Their default bindings are owned here: table 3.6-B is the one binding table of every `action.menu.*` action, and the UI framework cites it. Every screen is usable with the keyboard alone, so rows 1 to 8 bind the arrow keys with repeat, Enter and Esc beside the gamepad bindings (UI-204; see DEC-950); the UI framework applies the Escape binding of `action.menu.close` only outside a match, where Escape is `action.match.menu` (R-META-014), and `action.menu.front_end_activate` only at the front end. The leaderboard page is a hook that no player reaches (R-META-498, DEC-047), so no action of its own is bound. The match actions `action.match.menu`, `action.ui.pointer_down`, `action.ui.pointer_held` and `action.ui.alt_pointer_down` also act on UI windows (R-CTRL-047 rows M4 and M6).

Table 3.6-B Default bindings of the UI navigation actions of UI-203 and UI-204.

| # | Action id | Kind | Keyboard and mouse | Gamepad | Glyph override | Defined by |
|---|---|---|---|---|---|---|
| 1 | `action.menu.left` | Boolean | `key:left:repeat` | `pad:dpad_left:repeat` · `pad:stick_left_left:repeat` · `pad:stick_right_left:repeat` | `glyph.pad.dpad_horizontal` | UI-203, UI-204 |
| 2 | `action.menu.right` | Boolean | `key:right:repeat` | `pad:dpad_right:repeat` · `pad:stick_left_right:repeat` · `pad:stick_right_right:repeat` | — | UI-203, UI-204 |
| 3 | `action.menu.up` | Boolean | `key:up:repeat` | `pad:dpad_up:repeat` · `pad:stick_left_up:repeat` · `pad:stick_right_up:repeat` | — | UI-203, UI-204 |
| 4 | `action.menu.down` | Boolean | `key:down:repeat` | `pad:dpad_down:repeat` · `pad:stick_left_down:repeat` · `pad:stick_right_down:repeat` | — | UI-203, UI-204 |
| 5 | `action.menu.activate` | Boolean | `key:enter:down` | `pad:face_south:down` | `glyph.pad.face_south` | UI-203, UI-204 |
| 6 | `action.menu.front_end_activate` | Boolean | — | `pad:menu:down` | — | UI-203, UI-204 |
| 7 | `action.menu.close` | Boolean | `key:escape:down` | `pad:view:down` · `pad:face_east:down` | — | UI-203, UI-204 |
| 8 | `action.menu.dismiss` | Boolean | `mouse:primary:down` · `key:space:down` · `key:enter:down` · `key:escape:down` | `pad:face_south:down` | — | UI-203, UI-204 |
| 9 | `action.menu.scroll_speedup` | Boolean | `key:shift:held` | — | — | UI-203, UI-204 |

**INP-078** (MUST) `action.power.end_current` and `action.direct.secondary_fire_directed` have no default binding. They are queried like every other action and fire only when the user layer binds them.

**INP-079** (MUST) One default binding is a trigger press binding, which fires once when the trigger goes down: `pad:trigger_left:down` of `action.direct.secondary_fire` (DEC-553). The gamepad bindings of `action.ui.pointer_down` and `action.ui.pointer_held` are conjunctions with an alias of `action.ui.show_cursor`: while the left shoulder is held, the south face button acts as the UI pointer button (DEC-553, R-CTRL-047 row M6).

### 3.7 Glyphs

**INP-080** (MUST) A control's glyph is looked up by its control in table 3.7-A; the UI framework draws the darkening shadow first and the glyph on top. A control without a row has no glyph asset, and the UI shows its display text instead. A glyph override (INP-081) is drawn over the shadow of its row.

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
| `pad:stick_left` | `glyph.pad.stick_left` | `glyph.pad.stick_left_shadow` |
| `pad:stick_right` | `glyph.pad.stick_right` | `glyph.pad.stick_right_shadow` |
| `pad:dpad_left` and `pad:dpad_right` together, as a glyph override only | `glyph.pad.dpad_horizontal` | `glyph.pad.dpad_horizontal_shadow` |
| `key:tab` | `glyph.key.tab` | none |

**INP-081** (MUST) An action may carry one glyph override in the binding set (the "Glyph override" columns of tables 3.6-A and 3.6-B); when an action has several glyph entries, the last one wins. `Glyph(action)` returns the override. The stick and d-pad overrides use `glyph.pad.stick_left`, `glyph.pad.stick_right` and `glyph.pad.dpad_horizontal`, each over its shadow (table 3.7-A); the content build checks every override against the asset manifest (INP-222).

**INP-082** (MUST) The glyph overrides of `action.camera.raise` (the left-stick glyph) and `action.camera.lower` (the right-stick glyph) are part of the default set as table 3.6-A states, although both actions are bound to triggers on the gamepad (see DEC-954).

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
| `key:c` | `action.power.cancel` (down), `action.order.demote` (down), `action.order.scrap` (down) | 3 |
| `key:control` | `action.order.straight_route` (held) | 1 |
| `key:d` | `action.camera.pan_right` (held) | 1 |
| `key:down` | `action.camera.pan_back` (held), `action.menu.down` (repeat) | 2 |
| `key:e` | `action.camera.lower` (held) | 1 |
| `key:enter` | `action.power.activate_slot` (down), `action.match.chat` (down), `action.menu.activate` (down), `action.menu.dismiss` (down) | 4 |
| `key:escape` | `action.match.menu` (down), `action.menu.close` (down), `action.menu.dismiss` (down) | 3 |
| `key:f2` | `action.camera.look_around` (held) | 1 |
| `key:f9` | `action.debug.network_stats` (down) | 1 |
| `key:l` | `action.view.chat_log` (held) | 1 |
| `key:left` | `action.camera.pan_left` (held), `action.menu.left` (repeat) | 2 |
| `key:p` | `action.view.screenshot` (down) | 1 |
| `key:pause` | `action.match.pause` (down) | 1 |
| `key:q` | `action.camera.raise` (held) | 1 |
| `key:right` | `action.camera.pan_right` (held), `action.menu.right` (repeat) | 2 |
| `key:s` | `action.camera.pan_back` (held) | 1 |
| `key:shift` | `action.camera.fast_pan` (held), `action.order.move_modifier` (held), `action.menu.scroll_speedup` (held) | 3 |
| `key:space` | `action.select.clear` (down), `action.match.ready` (down), `action.menu.dismiss` (down) | 3 |
| `key:tab` | `action.power.next` (down) | 1 |
| `key:up` | `action.camera.pan_forward` (held), `action.menu.up` (repeat) | 2 |
| `key:w` | `action.camera.pan_forward` (held) | 1 |
| `key:x` | `action.view.hide_interface` (down, operand of all) | 1 |
| `key:z` | `action.camera.zoom` (held) | 1 |
| `mouse:middle` | `action.camera.zoom` (held) | 1 |
| `mouse:motion` | `action.camera.aim` (move) | 1 |
| `mouse:primary` | `action.select.pick` (down), `action.select.pick_idle` (down), `action.select.circle_begin` (down), `action.select.circle_grow` (held), `action.select.circle_end` (up), `action.power.activate_slot` (up), `action.power.place` (down), `action.direct.move_hold` (held), `action.direct.secondary_fire` (down, operand of all), `action.ui.pointer_down` (down), `action.ui.pointer_held` (held), `action.menu.dismiss` (down) | 12 |
| `mouse:secondary` | `action.order.crowd` (down), `action.order.promote` (down), `action.order.special` (down), `action.direct.fire` (held), `action.direct.secondary_fire` (held, operand of all), `action.direct.set_target` (held), `action.ui.alt_pointer_down` (down) | 7 |
| `mouse:wheel` | `action.camera.height_wheel` (move) | 1 |
| `pad:connection` | `action.device.pad_disconnected` (up), `action.device.pad_connected` (down) | 2 |
| `pad:dpad_down` | `action.camera.track_farther` (held), `action.menu.down` (repeat) | 2 |
| `pad:dpad_left` | `action.menu.left` (repeat) | 1 |
| `pad:dpad_right` | `action.menu.right` (repeat) | 1 |
| `pad:dpad_up` | `action.camera.track_closer` (held), `action.menu.up` (repeat) | 2 |
| `pad:face_east` | `action.select.clear` (down), `action.match.ready` (down), `action.menu.close` (down) | 3 |
| `pad:face_north` | `action.power.cancel` (down), `action.order.demote` (down), `action.order.scrap` (down) | 3 |
| `pad:face_south` | `action.select.pick` (down), `action.select.pick_idle` (down), `action.select.circle_begin` (down), `action.select.circle_grow` (held), `action.select.circle_end` (up), `action.power.activate_slot` (up), `action.power.place` (down), `action.order.special` (down), `action.match.ready` (down), `action.ui.pointer_down` (down, operand of all), `action.ui.pointer_held` (held, operand of all), `action.menu.activate` (down), `action.menu.dismiss` (down) | 13 |
| `pad:face_west` | `action.order.crowd` (down), `action.order.promote` (down), `action.order.move_modifier` (down), `action.direct.set_target` (held) | 4 |
| `pad:menu` | `action.match.menu` (down), `action.menu.front_end_activate` (down) | 2 |
| `pad:shoulder_left` | `action.power.previous` (down), `action.ui.show_cursor` (held) | 2 |
| `pad:shoulder_right` | `action.power.next` (down) | 1 |
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
| `pad:trigger_left` | `action.camera.lower` (position), `action.direct.secondary_fire` (down) | 2 |
| `pad:trigger_right` | `action.camera.raise` (position), `action.direct.fire` (held) | 2 |
| `pad:view` | `action.menu.close` (down) | 1 |
| `alias:action.ui.show_cursor` | `action.ui.pointer_down` (operand of all), `action.ui.pointer_held` (operand of all) | 2 |

### 3.11 Development binding layer and conditional actions

**INP-094** (MUST) Debug builds add a development binding layer on top of the default binding set: the debug actions of [diagnostics, test and performance](../diagnostics_test_and_performance/README.md) DBG-020 and the developer camera actions of the [camera](../camera_and_sequences.md) component (CAM-081, CAM-082), with the bindings of table 3.11-A. The layer is a binding-set record of the layer kind `development` (INP-070), which the content build validates like the default set (INP-170). The router reads it only where the development switch `ENGINE_DEVELOPMENT` is set, that is in Debug builds (ADR-056): a Release build neither reads the layer nor holds its action ids, so a user-layer entry that names one is skipped as `UnknownAction` (INP-171). INP-009 holds for the default set in every build.

Table 3.11-A Development binding layer (Debug builds).

| # | Action id | Kind | Keyboard and mouse | Gamepad | Defined by |
|---|---|---|---|---|---|
| 1 | `action.debug.command_window` | Boolean | `key:f11:down` | — | DBG-020 |
| 2 | `action.debug.drop_packet` | Boolean | `key:f:held` | — | DBG-020 |
| 3 | `action.debug.special_lighting_on` | Boolean | `key:l:held` | — | DBG-020 |
| 4 | `action.debug.special_lighting_off` | Boolean | `key:k:held` | — | DBG-020 |
| 5 | `action.debug.soak_rendering` | Boolean | `key:r:down` | — | DBG-020 |
| 6 | `action.debug.overlay` | Boolean | `key:f5:down` | — | DBG-020 |
| 7 | `action.debug.profile_view` | Boolean | `key:f10:down` | — | DBG-020 |
| 8 | `action.debug.camera_fast` | Boolean | `key:shift:held` | — | CAM-081 |
| 9 | `action.debug.camera_slow` | Boolean | `mouse:secondary:held` | — | CAM-081 |
| 10 | `action.debug.camera_rotate` | Boolean | `mouse:secondary:held` | — | CAM-082 |

**INP-095** (MUST) `Rebuild` appends the layer's entries after those of the default set, in entry order, before the layout layer and the user layer replace primary bindings (INP-162). No action of the layer has a default binding, so each binding of table 3.11-A is its action's primary binding; table 3.11-A is the one list of the development bindings (DBG-020, CAM-084). Its fast key, Shift held, is the key that also speeds up the camera pan (`action.camera.fast_pan`). `key:f10:down` binds like every other key (INP-109, table 6.3-A row 16), which is what lets the profile view use it.

**INP-096** (MUST) The layer shares physical inputs with the default set, and the router resolves no overlap (INP-085): in a Debug build `key:l` feeds `action.view.chat_log` and `action.debug.special_lighting_on`, and both act (DBG-020); `key:shift` adds `action.debug.camera_fast` to its actions of table 3.8-A, and `mouse:secondary` adds `action.debug.camera_slow` and `action.debug.camera_rotate`, which only the developer fly mode reads (CAM-081, CAM-082). The keys `f`, `k`, `r`, `f5`, `f10` and `f11` have no binding in the default set.

**INP-097** (MUST) `action.sequence.skip`, the skip of a presentation timeline (CAM-105), exists only if the design phase adopts the presentation timelines of DEC-051. Until then it is not in the action id table (INP-067), no layer binds it, and an entry that names it is rejected as `UnknownAction`. When DEC-051 adopts timelines, the action joins table 3.6-A with the reference's bindings, `key:enter:down` · `pad:face_east:down`.

## 4 Behaviour and algorithms

### 4.16 Notes on the default scheme

**INP-181** (MUST) The default set overlaps heavily on a few inputs (table 3.8-A): the south face button feeds 13 actions, the primary mouse button 12, the secondary mouse button 7 and the west face button 4. The consumers' context decides the meaning of each press: one secondary press means promote, toggle, order, fire or move by the resolver of reference/06 (R-CTRL-050), and never more than one of them.

**INP-182** (MUST) 11 actions have gamepad bindings only: the stick actions (`action.camera.pan_stick`, `action.camera.aim_stick`, `action.direct.move_stick`, `action.direct.aim_stick`, `action.direct.move_direction_change`), the tracking distance actions, `action.ui.show_cursor`, the two connection actions and `action.menu.front_end_activate`. The analogue camera and unit controls have key and button equivalents. 25 actions have keyboard and mouse bindings only and 36 have both (tables 3.6-A and 3.6-B).

**INP-183** (MUST) Value conventions of the default set: `action.camera.pan_stick`, `action.camera.aim_stick`, `action.direct.move_stick`, `action.direct.aim_stick` and `action.direct.move_direction_change` carry stick units, x positive to the right and y negative forward, up to `inp.pad.stick_left.scale` for the left stick and `inp.pad.stick_right.scale` for the right stick at sensitivity 100; `action.camera.aim` carries client pixels per frame; `action.camera.height_wheel` carries wheel notches per frame, forward positive; `action.camera.raise` and `action.camera.lower` are `Boolean` actions fed by trigger `position` bindings, true while the trigger is above its threshold. The camera's stick gain `ref.ctrl.camera.stick_gain` ([camera](../../../reference/06_controls_camera_and_commands/camera.md)) is defined on these units, and so are the direct-control vectors of the team command state: the sender converts the `action.direct.move_stick` value to world space with the camera's control vector and sends it as `directMoveVector`, by which the Strike team's waypoint moves in wu from the lead Trooper's position, and converts the aiming vector the same way into `directTargetVector`, which the Strike team attacks as the point (x, 0, z) (R-SIM-440, R-CTRL-101, R-UNIT-358; see DEC-854).

**INP-184** (MUST) The input system reads these settings (R-META-455): the keyboard layout (INP-164); the mouse-button count, `ref.meta.settings.mouse_buttons.default` in [options and hooks](../../../reference/09_front_end_and_match_flow/options_localisation_and_hooks.md), which the client reads when it sends the team command state and on which no binding depends; and the settings that setting nodes name, none in the default set. The runtime feature flags that value nodes name are not settings: the router reads them from the edition (INP-143).

**INP-185** (MUST) The input-layer values that the consumers' rules use are owned by their documents and are not repeated here: `ref.ctrl.select.circle_growth`, `ref.ctrl.select.circle_max_radius`, `ref.ctrl.order.crowd_interval_ticks`, `ref.ctrl.select.sound_interval`, `ref.ctrl.order.breach_charge_fraction` ([selection and Power slots](../../../reference/06_controls_camera_and_commands/selection_and_power_slots.md)); `ref.ctrl.promote_radius` ([actions and bindings](../../../reference/06_controls_camera_and_commands/actions_and_bindings.md)); `ref.unit.worker.order_transport_radius` ([Worker](../../../reference/02_units/worker.md)); `ref.unit.leader.formation_toggle_radius` ([Leader and formations](../../../reference/02_units/leader_and_formations.md)); `ref.unit.transport.deploy_min_distance` and `ref.unit.transport.arrival_distance` ([Transport](../../../reference/02_units/transport.md)); and the send period of the team command state, `ref.sim.command.control_state_period` ([Teams and player commands](../../../reference/12_simulation_model/06_teams_and_player_commands.md)).

## 5 Concurrency and performance budget

The budget is [README](README.md) §5.

## 6 Failure modes, diagnostics and telemetry

Failure modes and diagnostics are [README](README.md) §6.

## 7 Acceptance tests

The acceptance tests are [README](README.md) §7.

## 8 Open decisions

The open decisions and design-phase inputs are [README](README.md) §8.
