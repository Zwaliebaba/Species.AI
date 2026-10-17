"""X-AST-2a follow-up patch (routes RA-263 a, RA-285, RB-326): run once after X-AST-2a_apply.py and X-AST-2a_docs.py.

Usage: python3 -I X-AST-2a_patch.py --design design_next --census <scratch>/census.md
- explainer pictures: "used by" notes follow the picture lists of R-META-225 (manifest and GameAssets);
- "also plays for" / "Also bound to" use the event ids of reference/10 (`collector.spawn`, `meteor.spawn`,
  `abductor.spawn`, `core_device.spawn`);
- members 07 to 13 of `sfx.unit.worker.burning` are silent samples by design (R-AUD-066);
- GameAssets §18.1 is replaced by the census file (regenerated with X-AST-2a_strings.py --markdown).
"""
import argparse
import csv
import os
import sys

ap = argparse.ArgumentParser()
ap.add_argument('--design', required=True)
ap.add_argument('--census', required=True)
a = ap.parse_args()
AD = os.path.join(a.design, 'assets')
MAN = os.path.join(AD, 'asset_manifest.csv')
GA = os.path.join(AD, 'GameAssets.md')
ERR = []

ALL = 'Flag Chain, Launch Race, Relic Run, Siege, Spawn Control, Zone Control'
USED = {
    'Transport': 'Launch Race', 'soft glow': ALL, 'Worker figure': ALL,
    'hurt Worker': 'Flag Chain, Relic Run, Siege, Spawn Control, Zone Control',
    'neutral flag': 'Flag Chain', 'lowered flag': 'Flag Chain', 'locked flag': 'Flag Chain', 'raised flag': 'Flag Chain',
    'fuel pump': 'Launch Race', 'fuel pump arm': 'Flag Chain, Launch Race', 'island outline': 'Launch Race',
    'laser beam': 'Flag Chain, Launch Race, Relic Run, Spawn Control, Zone Control', 'muzzle flash': ALL,
    '+1 score pop-up': 'Relic Run, Zone Control', 'Launch Vessel': 'Launch Race', 'Solar Array': 'Launch Race',
    'Spawn Node': 'Relic Run, Spawn Control, Zone Control', 'Relic': 'Relic Run',
    'Transit Gate': 'Flag Chain, Launch Race, Siege', 'turret barrel': 'Siege', 'turret base': 'Siege',
    'water band': 'Launch Race', 'Core Device': 'Siege', 'Control Zone, back layer': 'Relic Run, Zone Control',
    'Control Zone, front layer': 'Relic Run, Zone Control',
}
RENAME = [('collector.presence_loop (layer 2)', 'collector.spawn (layer 2)'),
          ('meteor.presence_loop (layer 2)', 'meteor.spawn (layer 2)'),
          ('abductor.presence_loop (layer 4)', 'abductor.spawn (layer 4)'),
          ('collector.presence_loop', 'collector.spawn'),
          ('core_device.presence_loop', 'core_device.spawn')]
SILENT = '; silent sample by design: members 07 to 13 of this group are silent, so about half of the burning screams are silent (R-AUD-066)'
SILENT_ROW = '; members 07 to 13 are silent samples, part of the group\'s design (R-AUD-066)'

rows = list(csv.DictReader(open(MAN, encoding='utf-8', newline='')))
cols = list(rows[0].keys())
spec_map = {}
n_used = n_ren = n_sil = 0
for r in rows:
    if r['asset_id'] == 'help.explainer_picture':
        name, size, used = r['spec'].split('; ')
        assert used.startswith('used by '), r['spec']
        if name not in USED:
            ERR.append('no R-META-225 entry for ' + name)
            continue
        new = '%s; %s; used by %s' % (name, size, USED[name])
        spec_map[r['species_filename']] = (r['spec'], new)
        if r['spec'] != new:
            n_used += 1
        r['spec'] = new
    if r['asset_id'].startswith('sfx.') and 'also plays for' in r['role']:
        head, tail = r['role'].split('also plays for', 1)
        for o, n in RENAME:
            if o in tail:
                tail = tail.replace(o, n)
                n_ren += 1
        r['role'] = head + 'also plays for' + tail
    if r['asset_id'] == 'sfx.unit.worker.burning':
        k = int(r['variants'].split()[1])
        if k >= 7 and SILENT not in r['spec']:
            r['spec'] += SILENT
            r['placeholder'] = 'silence'
            n_sil += 1
if n_sil not in (0, 7):
    ERR.append('burning members changed: %d' % n_sil)

t = open(GA, encoding='utf-8').read()
lines = t.split('\n')
for i, l in enumerate(lines):
    c = l.split(' | ')
    if l.startswith('| `content/textures/help/explainer/'):
        f = c[0][3:-1]
        if f in spec_map:
            old, new = spec_map[f]
            if '| %s |' % old in l:
                lines[i] = l.replace('| %s |' % old, '| %s |' % new)
            elif '| %s |' % new not in l:
                ERR.append('explainer row not matched: ' + f)
    if len(c) >= 7 and c[1].startswith('`sfx.') and c[3] not in ('—', ''):
        cell = c[3]
        for o, n in RENAME:
            cell = cell.replace('`%s`' % o, '`%s`' % n)
        if cell != c[3]:
            c[3] = cell
            lines[i] = ' | '.join(c)
    if '| `sfx.unit.worker.burning` |' in l and SILENT_ROW not in l:
        lines[i] = l.replace('reference duration 0.25-4.06 s |', 'reference duration 0.25-4.06 s' + SILENT_ROW + ' |')
        if lines[i] == l:
            ERR.append('burning row not matched')
t = '\n'.join(lines)
left = [o for o, n in RENAME if '`%s`' % o in t]
if left:
    ERR.append('old event names left in GameAssets: %s' % left)
census = open(a.census, encoding='utf-8').read().rstrip('\n')
s0 = t.index('### 18.1 String ids cited by the documents')
s1 = t.index('\n## 19 Maps and thumbnails')
t = t[:s0] + census + '\n' + t[s1:]

if ERR:
    print('\n'.join(ERR), file=sys.stderr)
    sys.exit(1)
with open(MAN, 'w', encoding='utf-8', newline='') as fh:
    w = csv.DictWriter(fh, fieldnames=cols, lineterminator='\n')
    w.writeheader()
    w.writerows(rows)
open(GA, 'w', encoding='utf-8').write(t)
print('explainer specs changed', n_used, 'renames', n_ren, 'silent members', n_sil)
