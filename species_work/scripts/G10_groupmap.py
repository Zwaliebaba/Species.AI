"""G10: propose a mapping from reference sound names (sample or group) to
Species asset ids, using the asset origin map and the sound-event CSV.

Usage: python3 -I G10_groupmap.py <X2b_sound_events.csv> <asset_origin_map.csv>
"""
import csv
import re
import sys


def stem(p):
    s = p.split('/')[-1]
    return s[:-4].lower() if s.lower().endswith('.ogg') else s.lower()


def blocks(bp):
    out = []
    for part in bp.split('||'):
        part = part.strip()
        m = re.match(r'\[(Sample|Group) ([^\]]+)\]\s*(\S+)(.*)', part)
        if m:
            out.append((m.group(1), m.group(2).strip(), m.group(3), m.group(4).strip()))
    return out


def main():
    rows = list(csv.DictReader(open(sys.argv[1], newline='', encoding='utf-8')))
    amap = {}
    for r in csv.DictReader(open(sys.argv[2], newline='', encoding='utf-8')):
        a = r['asset_id']
        if a.split('.')[0] in ('sfx', 'amb'):
            for p in r['original_paths'].split(';'):
                if p.strip():
                    amap.setdefault(a, set()).add(stem(p.strip()))
    by_stem = {}
    for a, ss in amap.items():
        for s in ss:
            by_stem.setdefault(s, set()).add(a)
    names = {}
    for r in rows:
        if r['in_data'] != 'yes' or r['fired_in_FULL'] != 'yes':
            continue
        samples = {stem(p) for p in r['samples'].split(';') if p.strip()}
        bl = blocks(r['blueprint'])
        for kind, name, mode, rest in bl:
            key = (kind, name.lower())
            cand = set()
            if kind == 'Sample':
                cand = by_stem.get(name.lower(), set())
            else:
                for s in samples:
                    cand |= by_stem.get(s, set())
            names.setdefault(key, {'cands': set(), 'pairs': []})
            if len(bl) == 1 or kind == 'Sample':
                names[key]['cands'] |= cand
            else:
                names[key]['cands'] |= {('?', c) for c in cand}
            names[key]['pairs'].append(r['object_type'] + ' ' + r['event'])
    for (kind, name), v in sorted(names.items()):
        c = sorted(str(x) for x in v['cands'])
        print(kind, name, '=>', ' '.join(c), '| used by', ', '.join(v['pairs']))


if __name__ == '__main__':
    main()
