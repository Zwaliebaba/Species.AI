#!/usr/bin/env python3
"""C-INP helper: the default binding set as data, and the tables generated from it.

Usage: python3 -I C-INP_tables.py <out_dir>

Writes catalogue_match.md, catalogue_menu.md and reverse_index.md into <out_dir>.
Binding notation (input.md section 3.5): key:<name>:<cond>, mouse:<control>:<cond>,
pad:<control>:<cond>, all(a, b), alias(<action id>). Bindings of one action are listed
in order; the first is the primary binding.
"""
import os
import re
import sys

out = sys.argv[1]

# (action id, value kind, keyboard-and-mouse bindings, gamepad bindings, glyph override, row of reference/06)
MATCH = [
    ('action.camera.pan_left', 'Boolean', ['key:a:held', 'key:left:held'], [], '', 'A1'),
    ('action.camera.pan_right', 'Boolean', ['key:d:held', 'key:right:held'], [], '', 'A1'),
    ('action.camera.pan_forward', 'Boolean', ['key:w:held', 'key:up:held'], [], '', 'A1'),
    ('action.camera.pan_back', 'Boolean', ['key:s:held', 'key:down:held'], [], '', 'A1'),
    ('action.camera.pan_stick', 'Axis2', [], ['pad:stick_left:position'], 'left stick (no asset)', 'A1'),
    ('action.camera.fast_pan', 'Boolean', ['key:shift:held'], ['pad:stick_left:held'], '', 'A2'),
    ('action.camera.raise', 'Boolean', ['key:q:held'], ['pad:trigger_right:position'], 'left stick (no asset)', 'A3'),
    ('action.camera.lower', 'Boolean', ['key:e:held'], ['pad:trigger_left:position'], 'right stick (no asset)', 'A3'),
    ('action.camera.height_wheel', 'Axis1', ['mouse:wheel:move'], [], '', 'A3'),
    ('action.camera.aim', 'Axis2', ['mouse:motion:move'], [], 'right stick (no asset)', 'A4'),
    ('action.camera.aim_stick', 'Axis2', [], ['pad:stick_right:position'], '', 'A4'),
    ('action.camera.zoom', 'Boolean', ['key:z:held', 'mouse:middle:held'], ['pad:stick_right:held'], '', 'A5'),
    ('action.camera.zoom_out', 'Boolean', ['key:c:held'], [], '', 'A11'),
    ('action.camera.look_around', 'Boolean', ['key:f2:held'], ['pad:stick_right:held'], '', 'A6'),
    ('action.camera.track_closer', 'Boolean', [], ['pad:dpad_up:held'], '', 'A7'),
    ('action.camera.track_farther', 'Boolean', [], ['pad:dpad_down:held'], '', 'A7'),
    ('action.view.chat_log', 'Boolean', ['key:l:held'], [], '', 'A8'),
    ('action.view.hide_interface', 'Boolean', ['all(key:alt:held, key:x:down)'], [], '', 'A9'),
    ('action.view.screenshot', 'Boolean', ['key:p:down'], [], '', 'A10'),
    ('action.select.pick', 'Boolean', ['mouse:primary:down'], ['pad:face_south:down'], '`glyph.pad.face_south`', 'S1, O10'),
    ('action.select.pick_idle', 'Boolean', ['mouse:primary:down'], ['pad:face_south:down'], '', 'S1'),
    ('action.select.clear', 'Boolean', ['key:space:down'], ['pad:face_east:down'], '`glyph.pad.face_east`', 'S3'),
    ('action.select.circle_begin', 'Boolean', ['mouse:primary:down'], ['pad:face_south:down'], '', 'S4'),
    ('action.select.circle_grow', 'Boolean', ['mouse:primary:held'], ['pad:face_south:held'], '', 'S5'),
    ('action.select.circle_end', 'Boolean', ['mouse:primary:up'], ['pad:face_south:up'], '', 'S6'),
    ('action.power.next', 'Boolean', ['key:bracket_right:down', 'key:tab:down'], ['pad:shoulder_right:down'], '`glyph.pad.shoulder_right`', 'S7'),
    ('action.power.previous', 'Boolean', ['key:bracket_left:down'], ['pad:shoulder_left:down'], '`glyph.pad.shoulder_left`', 'S8'),
] + [
    (f'action.power.slot_{n}', 'Boolean', [f'key:{n}:down'], [], '', 'S9') for n in range(1, 10)
] + [
    ('action.power.activate_slot', 'Boolean', ['mouse:primary:up', 'key:enter:down'], ['pad:face_south:up'], '', 'S11'),
    ('action.power.place', 'Boolean', ['mouse:primary:down'], ['pad:face_south:down'], '', 'T1'),
    ('action.power.cancel', 'Boolean', ['key:c:down'], ['pad:face_north:down'], '`glyph.pad.face_north`', 'T3, T6'),
    ('action.power.end_current', 'Boolean', [], [], '', 'T7'),
    ('action.power.panel_show', 'Boolean', ['key:tab:down'], ['pad:view:down'], '`glyph.pad.face_west`', 'T8'),
    ('action.power.panel_hide', 'Boolean', ['key:tab:up'], ['pad:view:down', 'pad:face_east:down'], '', 'T8'),
    ('action.order.crowd', 'Boolean', ['mouse:secondary:down'], ['pad:face_west:down'], '', 'O1'),
    ('action.order.straight_route', 'Boolean', ['key:control:held'], [], '', 'O1, O6'),
    ('action.order.promote', 'Boolean', ['mouse:secondary:down'], ['pad:face_west:down'], '', 'O2 to O5'),
    ('action.order.special', 'Boolean', ['mouse:secondary:down'], ['pad:face_south:down'], '', 'O6 to O9, T2'),
    ('action.order.move_modifier', 'Boolean', ['key:shift:held'], ['pad:face_west:down'], '', 'O6 to O9'),
    ('action.order.demote', 'Boolean', ['key:c:down'], ['pad:face_north:down'], '', 'T5'),
    ('action.order.scrap', 'Boolean', ['key:c:down'], ['pad:face_north:down'], '', 'T4'),
    ('action.direct.move_hold', 'Boolean', ['mouse:primary:held'], [], '', 'D1'),
    ('action.direct.move_stick', 'Axis2', [], ['pad:stick_left:position'], 'left stick (no asset)', 'D1'),
    ('action.direct.fire', 'Boolean', ['mouse:secondary:held'], ['pad:trigger_right:held'], '', 'D2, D5'),
    ('action.direct.secondary_fire', 'Boolean', ['all(mouse:primary:down, mouse:secondary:held)'], ['pad:trigger_left:down'], '', 'D3'),
    ('action.direct.aim_stick', 'Axis2', [], ['pad:stick_right:position'], '', 'D2'),
    ('action.direct.secondary_fire_directed', 'Axis2', [], [], '`glyph.pad.trigger_left`', 'D3'),
    ('action.direct.set_target', 'Boolean', ['mouse:secondary:held'], ['pad:face_west:held'], '`glyph.pad.face_west`', 'D4, D5'),
    ('action.direct.set_target_end', 'Boolean', ['mouse:secondary:up'], ['pad:face_west:up'], '', 'D1'),
    ('action.direct.move_direction_change', 'Axis2', [], ['pad:stick_left:move'], '', 'camera R-CTRL-487'),
    ('action.match.ready', 'Boolean', ['key:space:down'], ['pad:face_south:down', 'pad:face_east:down'], '', 'M1'),
    ('action.match.chat', 'Boolean', ['key:enter:down'], [], '', 'M2'),
    ('action.match.pause', 'Boolean', ['key:pause:down'], [], '', 'M3'),
    ('action.match.menu', 'Boolean', ['key:escape:down'], ['pad:menu:down'], '', 'M4'),
    ('action.debug.network_stats', 'Boolean', ['key:f9:down'], [], '', 'M5'),
    ('action.ui.pointer_down', 'Boolean', ['mouse:primary:down'], ['all(alias(action.ui.show_cursor), pad:face_south:down)'], '', 'M6'),
    ('action.ui.pointer_held', 'Boolean', ['mouse:primary:held'], ['all(alias(action.ui.show_cursor), pad:face_south:held)'], '', 'M6'),
    ('action.ui.alt_pointer_down', 'Boolean', ['mouse:secondary:down'], [], '', 'M6'),
    ('action.ui.show_cursor', 'Boolean', [], ['pad:shoulder_left:held'], '', 'M6'),
    ('action.device.pad_disconnected', 'Boolean', [], ['pad:connection:up'], '', 'M7'),
    ('action.device.pad_connected', 'Boolean', [], ['pad:connection:down'], '', 'M7'),
]

MENU = [
    ('action.menu.left', 'Boolean', ['key:left:repeat'], ['pad:dpad_left:repeat', 'pad:stick_left_left:repeat', 'pad:stick_right_left:repeat'], 'd-pad left and right (no asset)', 'moves the cursor left; numeric text fields'),
    ('action.menu.right', 'Boolean', ['key:right:repeat'], ['pad:dpad_right:repeat', 'pad:stick_left_right:repeat', 'pad:stick_right_right:repeat'], '', 'moves the cursor right; numeric text fields'),
    ('action.menu.up', 'Boolean', ['key:up:repeat'], ['pad:dpad_up:repeat', 'pad:stick_left_up:repeat', 'pad:stick_right_up:repeat'], '', 'moves the cursor up'),
    ('action.menu.down', 'Boolean', ['key:down:repeat'], ['pad:dpad_down:repeat', 'pad:stick_left_down:repeat', 'pad:stick_right_down:repeat'], '', 'moves the cursor down'),
    ('action.menu.activate', 'Boolean', ['key:enter:down'], ['pad:face_south:down'], '`glyph.pad.face_south`', 'clicks the cursor\'s widget'),
    ('action.menu.front_end_activate', 'Boolean', [], ['pad:menu:down'], '', 'as activate, while at the front end'),
    ('action.menu.close', 'Boolean', ['key:escape:down'], ['pad:view:down', 'pad:face_east:down'], '', 'the back action; Escape outside a match only'),
    ('action.menu.dismiss', 'Boolean', ['mouse:primary:down', 'key:space:down', 'key:enter:down', 'key:escape:down'], ['pad:face_south:down'], '', 'dismisses an in-page dialog and the gamepad-disconnected dialog'),
    ('action.menu.scroll_speedup', 'Boolean', ['key:shift:held'], [], '', 'speeds up a held increment button'),
    ('action.menu.cycle_up', 'Boolean', [], ['pad:shoulder_left:repeat'], '', '`PadMenuCycleUp`: previous board on the leaderboard page'),
    ('action.menu.cycle_down', 'Boolean', [], ['pad:shoulder_right:repeat'], '', '`PadMenuCycleDown`: next board on the leaderboard page'),
    ('action.menu.select_previous', 'Boolean', [], ['pad:trigger_left:down'], '', 'previous view on the leaderboard page'),
    ('action.menu.select_next', 'Boolean', [], ['pad:trigger_right:down'], '', 'next view on the leaderboard page'),
    ('action.menu.toggle_period', 'Boolean', [], ['pad:face_north:down'], '', 'period toggle on the leaderboard page'),
]

LEAF = re.compile(r'(key|mouse|pad):([a-z0-9_]+):([a-z]+)')


def cell(bs):
    return ' · '.join(f'`{b}`' for b in bs) if bs else '—'


def write_catalogue(path, rows, last_header):
    with open(path, 'w', encoding='utf-8') as fh:
        fh.write(f'| # | Action id | Kind | Keyboard and mouse | Gamepad | Glyph override | {last_header} |\n')
        fh.write('|---|---|---|---|---|---|---|\n')
        for i, (a, kind, kb, pad, glyph, extra) in enumerate(rows, 1):
            fh.write(f'| {i} | `{a}` | {kind} | {cell(kb)} | {cell(pad)} | {glyph or "—"} | {extra} |\n')


write_catalogue(os.path.join(out, 'catalogue_match.md'), MATCH, 'reference/06 row')
write_catalogue(os.path.join(out, 'catalogue_menu.md'), MENU, 'Use')

# reverse index: physical control -> actions (condition), operands of all() marked
index = {}
for a, kind, kb, pad, glyph, extra in MATCH + MENU:
    for b in kb + pad:
        operand = b.startswith('all(')
        for dev, ctl, cond in LEAF.findall(b):
            key = f'{dev}:{ctl}'
            index.setdefault(key, []).append(f'`{a}` ({cond}{", operand of all" if operand else ""})')
        if 'alias(' in b:
            tgt = re.search(r'alias\(([a-z_.0-9]+)\)', b).group(1)
            index.setdefault(f'alias:{tgt}', []).append(f'`{a}` (operand of all)')
order = {'key': 0, 'mouse': 1, 'pad': 2, 'alias': 3}
with open(os.path.join(out, 'reverse_index.md'), 'w', encoding='utf-8') as fh:
    fh.write('| Physical input | Actions (condition) | Count |\n|---|---|---|\n')
    for k in sorted(index, key=lambda s: (order[s.split(':')[0]], s)):
        fh.write(f'| `{k}` | {", ".join(index[k])} | {len(index[k])} |\n')
print(len(MATCH), 'match actions,', len(MENU), 'menu actions,', len(index), 'physical inputs')
