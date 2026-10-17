#!/usr/bin/env python3
"""Write species_work/dec_map_X-DEC-5c.csv for the X-DEC-5c register entries.

Usage: python3 -I X-DEC-5c_decmap.py <out.csv>
"""
import csv
import sys

D29 = '29_data_formats_reference.md'
D30 = '30_scripting_tutorial_and_cutscenes.md'
D32 = '32_debug_test_and_performance.md'

ROWS = [
    ('DEC-800', 'Content and settings writers are lossless, typed and atomic', 'Resolved',
     ['DAT', 'R-MAP', 'PLT', 'BLD'],
     ['architecture/components/data_schemas.md',
      'reference/08_maps_and_level_design/map_editor_hooks.md',
      'architecture/components/platform_layer.md',
      'architecture/components/build_tooling_and_distribution.md'],
     [f'{D29}:248-279', f'{D29}:822-827', f'{D29}:1164-1235', f'{D30}:1120-1133']),
    ('DEC-801', 'Duplicate keys, repeated fields and over-long values are build errors', 'Resolved',
     ['DAT', 'BLD', 'R-MAP', 'AEN', 'UI'],
     ['architecture/components/data_schemas.md',
      'architecture/components/build_tooling_and_distribution.md',
      'reference/08_maps_and_level_design/rules_and_conventions.md',
      'architecture/components/audio_engine.md',
      'architecture/components/ui_framework/layout_rendering_input_and_localisation.md'],
     [f'{D29}:44-55', f'{D29}:135-247', f'{D29}:248-279', f'{D29}:951-1003', f'{D29}:2242-2322']),
    ('DEC-802', 'Maps that match setup does not list', 'Open',
     ['R-MAP', 'R-META', 'DAT', 'BLD', 'DBG'],
     ['reference/08_maps_and_level_design/rules_and_conventions.md',
      'reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md',
      'architecture/components/data_schemas.md',
      'architecture/components/build_tooling_and_distribution.md',
      'architecture/components/diagnostics_test_and_performance.md'],
     [f'{D29}:408-450', f'{D29}:902-919']),
    ('DEC-803', 'Camera animations without content', 'Open',
     ['CAM', 'R-MAP', 'R-TUT', 'DAT'],
     ['architecture/components/camera_and_sequences.md',
      'reference/08_maps_and_level_design/map_editor_hooks.md',
      'reference/11_tutorial_and_help.md',
      'architecture/components/data_schemas.md'],
     [f'{D30}:861-886', f'{D30}:887-902', f'{D30}:1120-1133']),
    ('DEC-804', 'Soak-test sequence and timing rules', 'Resolved',
     ['DBG', 'BLD'],
     ['architecture/components/diagnostics_test_and_performance.md',
      'architecture/components/build_tooling_and_distribution.md'],
     [f'{D32}:479-482']),
]


def main():
    if len(sys.argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    with open(sys.argv[1], 'w', encoding='utf-8', newline='') as fh:
        w = csv.writer(fh, lineterminator='\n')
        w.writerow(['dec_id', 'title', 'status', 'affects_prefix', 'affects_files', 'orig_refs'])
        for dec, title, status, pre, files, refs in ROWS:
            w.writerow([dec, title, status, ';'.join(pre), ';'.join(files), ';'.join(refs)])
    print(f'wrote {len(ROWS)} rows')
    return 0


if __name__ == '__main__':
    sys.exit(main())
