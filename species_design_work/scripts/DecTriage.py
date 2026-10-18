"""Phase 0: triage the Open decisions to owning areas (design-phase prompt §5.4).

Usage: python3 -I DecTriage.py <DESIGN_ROOT> <OUT.md>

Every Open DEC gets an owning area: a deferred-design entry by its topic in ADR/README.md §"Deferred to the design
phase"; otherwise by the first document of its Affects line (reference documents by the ledger ownership table,
components by the area that answers their design-phase inputs). Entries with no mapping are listed for the
coordinator to assign by hand.
"""
import collections
import glob
import os
import re
import sys

ROOT, OUT = os.path.abspath(sys.argv[1]), sys.argv[2]
REF_AREA = {
    '00_reference_overview': 'A0', '01_skirmish_rules_and_modes': 'B9', '02_units': 'B4',
    '03_buildings_and_world_objects': 'B3', '04_supply_crates_powers_and_weapons': 'B4',
    '05_population_flow': 'B1', '06_controls_camera_and_commands': 'C1', '07_bots': 'B7',
    '08_maps_and_level_design': 'A1', '09_front_end_and_match_flow': 'C2', '10_audio_design': 'C4',
    '11_tutorial_and_help': 'C5', '12_simulation_model': 'D2',
}
COMP_AREA = {
    'renderer': 'D8', 'terrain': 'A1', 'asset_pipeline_and_resources': 'E2', 'animation_and_vfx': 'B4',
    'audio_engine': 'C4', 'ui_framework': 'C2', 'input': 'C1', 'camera_and_sequences': 'C1',
    'data_schemas': 'D4', 'platform_layer': 'D1', 'build_tooling_and_distribution': 'D1',
    'diagnostics_test_and_performance': 'D10',
}
TOPIC_AREA = {
    'networking model': 'A2', 'simulation numeric model': 'A2', 'simulation threading and process architecture': 'A2',
    'tick rate': 'A2', 'persistence': 'D4', 'sessions': 'C2', 'online services': 'D5', 'hosting': 'D1',
    'terrain runtime model': 'D8',
}

adr = open(os.path.join(ROOT, 'ADR', 'README.md'), encoding='utf-8').read()
sec = re.search(r'^## Deferred to the design phase\n(.*?)(?=^## |\Z)', adr, re.M | re.S).group(1)
deferred = {}
for line in sec.split('\n'):
    c = [x.strip() for x in line.strip().strip('|').split('|')]
    if len(c) >= 3 and c[0].lower() in TOPIC_AREA:
        ids = set()
        for a, b in re.findall(r'DEC-(\d{3})(?:\s*(?:to|–|-)\s*DEC-(\d{3}))?', c[-1]):
            ids.update(range(int(a), int(b or a) + 1))
        for i in ids:
            deferred[i] = (TOPIC_AREA[c[0].lower()], c[0])


def areas_of(affects):
    out = []
    for m in re.finditer(r'reference/([0-9]{2}_[a-z_]+)|architecture/components/([a-z_]+)', affects):
        a = REF_AREA.get(m.group(1)) if m.group(1) else COMP_AREA.get(m.group(2))
        if a and a not in out:
            out.append(a)
    return out


rows = []
for p in sorted(glob.glob(os.path.join(ROOT, 'decisions', '**', '*.md'), recursive=True)):
    if os.path.basename(p) in ('README.md', 'owner_decisions.md'):
        continue
    t = open(p, encoding='utf-8').read()
    for m in re.finditer(r'^#{2,4} (DEC-(\d{3,4})) (.+?)\n((?:(?!^#{2,4} DEC-).*\n?)*)', t, re.M):
        body = m.group(4)
        st = re.search(r'^- Status:\s*(\w+)', body, re.M)
        if not st or st.group(1) != 'Open':
            continue
        db = re.search(r'^- Decide by:\s*(.+)$', body, re.M)
        af = re.search(r'^- Affects:\s*(.+)$', body, re.M)
        affects = af.group(1) if af else ''
        n = int(m.group(2))
        found = areas_of(affects)
        if n in deferred:
            area, how = deferred[n][0], f'deferred topic: {deferred[n][1]}'
        elif found:
            area, how = found[0], 'Affects'
        else:
            area, how = 'coordinator', 'no mapping'
        rows.append((m.group(1), m.group(3).strip(), (db.group(1).strip() if db else ''), area,
                     ', '.join(a for a in found if a != area), how, os.path.relpath(p, ROOT)))

rows.sort(key=lambda r: (r[3], int(r[0][4:])))
by_area = collections.Counter(r[3] for r in rows)
by_db = collections.Counter((r[3], 'design phase' if r[2].lower().startswith('design phase') else r[2].split()[0].lower()) for r in rows)
out = ['# Decision triage', '',
       f'{len(rows)} Open decisions. Rules: `prompts/species-design-phase.md` §5.4. Each `design phase` decision ends '
       'Resolved, Dropped or re-dated in its owning area; `playtest` and `prototype` decisions stay Open with their '
       'defaults.', '', '| Area | Open | design phase | playtest | prototype |', '|---|---|---|---|---|']
for a in sorted(by_area):
    out.append(f'| {a} | {by_area[a]} | {by_db[(a, "design phase")]} | {by_db[(a, "playtest")]} | {by_db[(a, "prototype")]} |')
out += ['', '| DEC | Title | Decide by | Area | Also affects | Assigned by | Register file |', '|---|---|---|---|---|---|---|']
out += ['| ' + ' | '.join(r) + ' |' for r in rows]
open(OUT, 'w', encoding='utf-8').write('\n'.join(out) + '\n')
print(f'{len(rows)} Open DECs; by area: ' + ', '.join(f'{a} {n}' for a, n in sorted(by_area.items())))
