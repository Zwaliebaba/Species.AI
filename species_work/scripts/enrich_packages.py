"""Add decs_to_cite and assets_for_you to the Phase 3 package JSONs.

Usage: python3 -I enrich_packages.py <REPO>

decs_to_cite: DECs (with status) whose dec_map affects_files names one of the
package's target files (or its target folder). assets_for_you: asset ids whose
manifest consumers name one of the target files (or the folder).
"""
import csv
import glob
import json
import os
import sys

REPO = sys.argv[1]
WORK = os.path.join(REPO, 'species_work')
decs = list(csv.DictReader(open(os.path.join(WORK, 'dec_map.csv'), encoding='utf-8', newline='')))
assets = list(csv.DictReader(open(os.path.join(REPO, 'design_next', 'assets', 'asset_manifest.csv'),
                                  encoding='utf-8', newline='')))


def norm(v):
    """A legacy single-file path whose document became a folder counts as that folder."""
    v = v.strip().split(' (')[0]
    if v.endswith('.md') and os.path.isdir(os.path.join(REPO, 'design_next', v[:-3])):
        return v[:-3] + '/'
    for folder in FOLDERS:
        if v == folder.rstrip('/') + '.md':
            return folder
    return v


def hits(field, targets):
    for v in [norm(x) for x in field.replace(',', ';').split(';') if x.strip()]:
        for t in targets:
            if v == t:
                return True
            if v.endswith('/') and t.startswith(v):
                # folder-level: the folder README owner takes it, else every package in the folder
                owner = v + 'README.md'
                if owner in ALL_TARGETS:
                    if t == owner:
                        return True
                else:
                    return True
    return False


PKGS = [json.load(open(p, encoding='utf-8')) for p in sorted(glob.glob(os.path.join(WORK, 'packages', '*.json')))]
ALL_TARGETS = {t for p in PKGS if p.get('phase') == 3 for t in p['targets']}
FOLDERS = {t.rsplit('/', 1)[0] + '/' for t in ALL_TARGETS if '/' in t}
claimed = set()


for path in sorted(glob.glob(os.path.join(WORK, 'packages', '*.json'))):
    p = json.load(open(path, encoding='utf-8'))
    if p.get('phase') != 3:
        continue
    t = p['targets']
    p['decs_to_cite'] = sorted({f"{d['dec_id']} ({d['status']})" for d in decs if hits(d.get('affects_files', ''), t)})
    p['assets_for_you'] = sorted({a['asset_id'] for a in assets if hits(a.get('consumers', ''), t)})
    json.dump(p, open(path, 'w', encoding='utf-8'), indent=1, ensure_ascii=False)
    claimed.update(d.split(' ')[0] for d in p['decs_to_cite'])
    print(f"{p['id']:9} decs={len(p['decs_to_cite']):3} (open={sum('Open' in d for d in p['decs_to_cite']):3}) "
          f"assets={len(p['assets_for_you']):3}")

open_unclaimed = [d for d in decs if d['status'] == 'Open' and d['dec_id'] not in claimed]
print(f"Open DECs claimed by no package: {len(open_unclaimed)}")
for d in open_unclaimed:
    print('  UNCLAIMED', d['dec_id'], d['affects_files'][:120])
