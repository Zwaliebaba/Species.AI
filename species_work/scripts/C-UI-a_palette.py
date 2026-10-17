"""C-UI-a: move palette values from the UI-187/UI-188 role tables into the tuning table.

Usage: python3 -I C-UI-a_palette.py <layout_rendering_input_and_localisation.md>
"""
import re
import sys

path = sys.argv[1]
s = open(path, encoding='utf-8').read()

uses = {
    'window_body_tint': 'UI-054', 'accent_light': 'UI-054, UI-057, UI-081', 'accent_dark': 'UI-054, UI-057',
    'accent_pressed_top': 'UI-057', 'accent_pressed_bottom': 'UI-057', 'frame_outer': 'UI-054',
    'button_fill': 'UI-057', 'button_edge_light': 'UI-057', 'borderless_edge_light': 'UI-059',
    'inset_edge_light': 'UI-061, UI-070', 'button_edge_dark': 'UI-057, UI-060', 'text_normal': 'UI-057, UI-059, UI-081, UI-102, UI-119',
    'text_disabled': 'UI-057', 'text_ink': 'UI-054, UI-057, UI-059, UI-191', 'text_ink_disabled': 'UI-057',
    'text_outline': 'UI-108, UI-189, UI-192', 'well_fill': 'UI-070', 'inset_fill': 'UI-061', 'close_fill': 'UI-060',
    'close_hot': 'UI-060', 'close_edge': 'UI-060', 'scroll_track': 'UI-076', 'scroll_outline': 'UI-076',
    'scroll_thumb': 'UI-076', 'scroll_thumb_hot': 'UI-076', 'scroll_thumb_pressed': 'UI-076', 'check_box': 'UI-081, UI-125',
    'check_tick': 'UI-081', 'fe_caption': 'UI-108, UI-117, UI-121, UI-124, UI-125', 'fe_caption_focus': 'UI-108, UI-117, UI-119, UI-121, UI-124, UI-125',
    'fe_caption_inactive': 'UI-105, UI-117', 'fe_caption_alt': 'UI-108', 'fe_focus_band': 'UI-108, UI-117',
    'fe_row_band': 'UI-117', 'fe_title_outline': 'UI-113', 'fe_title_shadow': 'UI-113', 'fe_beam_top': 'UI-114',
    'fe_beam_bottom': 'UI-114', 'fe_beam_outline': 'UI-114', 'fe_box_tint': 'UI-115', 'fe_box_glow': 'UI-115',
    'fe_box_outline': 'UI-115', 'fe_box_inner_alpha': 'UI-115', 'fe_box_shadow_edge': 'UI-115', 'fe_flash': 'UI-107',
    'fe_text_box': 'UI-122',
}

def convert(block_id, default_unit):
    global s
    m = re.search(r'(\*\*' + block_id + r'\*\* \(MUST\)[^\n]*\n\n)(\| Token key \| Reference value \| Role \|\n\|---\|---\|---\|\n)((?:\|[^\n]*\n)+)', s)
    assert m, block_id
    rows_out, tune = [], []
    for line in m.group(3).splitlines():
        cells = [c.strip() for c in line.strip('|').split('|')]
        key, value, role = cells
        name = key.strip('`').split('.')[-1]
        unit = default_unit
        val = value
        note = ''
        mm = re.match(r'^(.*?) \((0 to 1|bytes)\)(.*)$', value)
        if mm:
            val = mm.group(1)
            unit = 'RGBA, 0 to 1' if mm.group(2) == '0 to 1' else 'RGBA8'
            note = mm.group(3).strip(' ,')
        elif ', ×' in value:
            val, note = value.split(', ×', 1)
            note = '× ' + note.strip()
        if name in ('fe_box_inner_alpha',):
            unit = 'alpha'
        if name == 'fe_flash':
            unit = 'RGB, 0 to 1'
        rows_out.append(f'| {key} | {role} |')
        notes = (note + '; ' if note else '') + role
        tune.append(f'| {key} | {val} | {unit} | {notes} | {block_id}, {uses[name]} | — |')
    new_table = '| Token key | Role |\n|---|---|\n' + '\n'.join(rows_out) + '\n'
    s = s[:m.start(2)] + new_table + s[m.end(3):]
    return tune

t1 = convert('UI-187', 'RGBA8')
t2 = convert('UI-188', 'RGBA, 0 to 1')
s = s.replace('**UI-187** (MUST) The window-layer tokens and their reference values:',
              '**UI-187** (MUST) The window-layer tokens are the following; their reference values are in §9.1:')
s = s.replace('**UI-188** (MUST) The front-end tokens and their reference values (0 to 1, except where bytes are stated):',
              '**UI-188** (MUST) The front-end tokens are the following; their reference values are in §9.1:')
s = s.replace('The palette keys are listed with their values and roles in UI-187 and UI-188; each appears there once, and the summary below repeats their requirement links.\n',
              'The palette tokens of UI-187 and UI-188 are the `ui.palette.*` rows; their values form the default palette (UI-186).\n')
old = '| `ui.palette.*` (46 tokens) | UI-187, UI-188 | RGBA bytes or 0 to 1, as stated | the default palette | UI-054 to UI-125, UI-187, UI-188 | — |\n'
assert old in s
s = s.replace(old, '\n'.join(t1 + t2) + '\n')
open(path, 'w', encoding='utf-8').write(s)
print(len(t1), len(t2))
