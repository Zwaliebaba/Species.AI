"""FX-06: add the setting-key column to the settings register (table §4.2) of
reference/09 options_localisation_and_hooks.md (route RB-231; RB-137 rows).

Usage: python3 -I FX-06_register.py <path to options_localisation_and_hooks.md>
"""
import sys

path = sys.argv[1]
t = open(path, encoding='utf-8').read()

intro_old = 'Every setting that the reference stores and reads, with its page, its default and the document that owns the default. "Hidden" means that no page shows it.'
intro_new = ('Every setting that the reference stores and reads, and the two Species accessibility settings of R-META-511, with its setting key '
             '([data schemas](../../architecture/components/data_schemas/config_input_and_strings.md) table 3.1-A), its page, its default and the document '
             'that owns the default. "Hidden" means that no page shows it; an inert entry is read and written back and has no effect.')
assert t.count(intro_old) == 1
t = t.replace(intro_old, intro_new)

keys = {
    'screen width and height': '`render.render_width`, `render.render_height`',
    'play in a window': '`render.window_mode`',
    'refresh rate': '`screen.refresh_rate`',
    'colour bits': '`screen.colour_bits`',
    'depth bits': '`screen.depth_bits`',
    'vertical-sync request, overscan': '`screen.vsync_request`, `screen.overscan`',
    'sea, sky, structure and unit detail': '`render.sea_detail`, `render.sky_detail`, `render.structure_detail`, `render.entity_detail`',
    'terrain detail, pixel effect': '`render.terrain_detail`, `render.pixel_effect`',
    'tree detail, special lighting, screenshot tile factor, inverted style, Worker distance scale factor':
        '`render.tree_detail`, `render.special_lighting`, `render.poster_resolution`, `render.inverted_style`, `render.worker_scale_factor`',
    'audio output': '`audio.output`',
    'mixing rate': '`audio.mix_rate`',
    'master volume': '`audio.master_volume`',
    'bus volumes': '`audio.bus_volume.ui`, `audio.bus_volume.announcer`, `audio.bus_volume.voice`, `audio.bus_volume.world`, `audio.bus_volume.ambience`, `audio.bus_volume.music`',
    'sound voices': '`audio.voices`',
    'swap left and right': '`audio.swap_stereo`',
    'sample-memory level, effects switch, output buffer length': '`audio.sample_memory_level`, `audio.effects`, `audio.output_buffer`',
    'in-match tips': '`interface.tips`',
    'use forwarded port': '`network.port_forwarding`',
    'hosting port, joining port': '`network.host_port`, `network.join_port`',
    'text language': '`interface.language`',
    'control help, difficulty, large menus': '`interface.control_help`, `interface.difficulty`, `interface.large_menus`',
    'automatic camera': '`camera.automatic`',
    'seasonal theme': '`interface.seasonal_theme`',
    'player name': '`player.name`',
    'preferred colour': '`player.preferred_colour`',
    'session name': '`network.session_name`',
    'four browser filters': '`browser.show_incompatible`, `browser.show_full`, `browser.show_password`, `browser.show_custom_maps`',
    'manual session entries': '`browser.session.<n>.host`, `browser.session.<n>.port`, `browser.session.<n>.name`',
    'control scheme': '`input.control_scheme`',
    'keyboard layout': '`input.keyboard_layout`',
    'mouse-button count': '`input.mouse_buttons`',
    'selection lock': '`input.selection_lock`',
    'language test': '`debug.language_test`',
    'command-history record and replay, bot-information overlay': '`debug.command_history`, `debug.bot_overlay`, `debug.bot_overlay_team`',
}
inert = {
    'vertical-sync request, overscan': ('no effect (R-META-456)', 'inert entries (R-META-456)'),
    'terrain detail, pixel effect': ('no effect (R-META-456)', 'inert entries (R-META-456)'),
    'seasonal theme': ('no effect (R-META-449 item 5)', 'inert entry (R-META-449 item 5)'),
}

start = t.index('| Setting | Page | Default | Owner of the default |')
end = t.index('\n\n### 4.3', start)
lines = t[start:end].split('\n')
out = ['| Setting | Setting key | Page | Default | Owner of the default |', '|---|---|---|---|---|']
seen = set()
for line in lines[2:]:
    cells = line.split(' | ')
    name = cells[0][2:]
    if name not in keys:
        sys.exit(f'no key for row: {name}')
    seen.add(name)
    rest = ' | '.join(cells[1:])
    if name in inert:
        a, b = inert[name]
        if a not in rest:
            sys.exit(f'inert text missing: {name}')
        rest = rest.replace(a, b)
    out.append(f'| {name} | {keys[name]} | {rest}')
missing = set(keys) - seen
if missing:
    sys.exit(f'rows not found: {missing}')
out.append('| reduced flashing, reduced motion | `interface.reduced_flash`, `interface.reduced_motion` | to be placed (DEC-044) | off | this document (R-META-511, DEC-964) |')
t = t[:start] + '\n'.join(out) + t[end:]
open(path, 'w', encoding='utf-8', newline='\n').write(t)
print(f'register rewritten: {len(out) - 2} rows')
