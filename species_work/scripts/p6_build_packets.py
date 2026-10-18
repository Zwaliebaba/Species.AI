"""Phase 6: cut the fix list into fixer packets (phase6_brief.md §4).

Usage: python3 -I p6_build_packets.py <REPO>

Reads species_work/fix_list.md, assigns each open finding to a packet by its area (OVERRIDE moves the
declaration findings of ruling 6 to P6-DAT), and writes species_work/packets/<ID>.md. Rows whose status is
not `open` are left out; root-area rows stay with the coordinator and are listed on stdout.
"""
import collections
import os
import re
import sys

REPO = sys.argv[1]
WORK = os.path.join(REPO, 'species_work')
CELL = re.compile(r'(?<!\\)\|')

AREA = {
    'reference/00_reference_overview': 'P6-RA', 'reference/01_skirmish_rules_and_modes': 'P6-RA',
    'reference/05_population_flow': 'P6-RA', 'reference/11_tutorial_and_help': 'P6-RA',
    'reference/02_units': 'P6-RB', 'reference/03_buildings_and_world_objects': 'P6-RB',
    'reference/04_supply_crates_powers_and_weapons': 'P6-RC', 'reference/06_controls_camera_and_commands': 'P6-RC',
    'reference/07_bots': 'P6-RC', 'reference/08_maps_and_level_design': 'P6-RC',
    'reference/09_front_end_and_match_flow': 'P6-RD', 'reference/10_audio_design': 'P6-RD',
    'reference/12_simulation_model': 'P6-RE',
    'components/data_schemas': 'P6-DAT',
    'ADR': 'P6-ADR', 'components/build_tooling_and_distribution': 'P6-ADR',
    'components/renderer': 'P6-RND', 'components/asset_pipeline_and_resources': 'P6-RND',
    'components/animation_and_vfx': 'P6-RND', 'components/terrain': 'P6-RND',
    'components/audio_engine': 'P6-ENG1', 'components/input': 'P6-ENG1', 'components/ui_framework': 'P6-ENG1',
    'components/camera_and_sequences': 'P6-ENG1',
    'components/platform_layer': 'P6-ENG2', 'components/diagnostics_test_and_performance': 'P6-ENG2',
    'glossary': 'P6-GLO', 'CONVENTIONS': 'P6-GLO', 'assets': 'P6-GLO',
    'decisions': 'P6-DEC',
}
OVERRIDE = {f'R4a-{n}': 'P6-DAT' for n in (1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 16, 17, 18, 19, 21, 25)}
SEV = {'High': 0, 'Medium': 1, 'Low': 2}


def cells(line):
    return [c.strip() for c in CELL.split(line.strip())][1:-1]


packets = collections.defaultdict(list)
coordinator = []
area = None
for line in open(os.path.join(WORK, 'fix_list.md'), encoding='utf-8').read().split('\n'):
    m = re.match(r'^## (.+)$', line)
    if m:
        area = m.group(1).strip()
        continue
    c = cells(line) if line.startswith('| ') else []
    if area is None or len(c) < 7 or not re.match(r'^R\d[ab]?-\d+$', c[0]):
        continue
    if c[-1] != 'open':
        continue
    pid = OVERRIDE.get(c[0]) or AREA.get(area)
    if pid is None:
        coordinator.append(c[0])
        continue
    packets[pid].append(c)

os.makedirs(os.path.join(WORK, 'packets'), exist_ok=True)
for pid, rows in sorted(packets.items()):
    rows.sort(key=lambda c: (SEV.get(c[1], 3), c[2], c[3]))
    n = collections.Counter(c[1] for c in rows)
    out = [f'# {pid} packet', '',
           f'{len(rows)} findings ({n["High"]} High, {n["Medium"]} Medium, {n["Low"]} Low). Read `species_work/phase6_brief.md` first; '
           'the files you own are in its §4 table. Paths are relative to `design_next/`.', '',
           '| Id | Severity | File | Where | Finding | Proposed fix |', '|---|---|---|---|---|---|']
    out += ['| ' + ' | '.join(c[:6]) + ' |' for c in rows]
    open(os.path.join(WORK, 'packets', f'{pid}.md'), 'w', encoding='utf-8').write('\n'.join(out) + '\n')
    print(f'{pid}: {len(rows)} ({n["High"]} H, {n["Medium"]} M, {n["Low"]} L)')
print('coordinator:', ', '.join(coordinator))
