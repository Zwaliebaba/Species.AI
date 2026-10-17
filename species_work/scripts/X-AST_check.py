#!/usr/bin/env python3
"""X-AST pass 1 checks.

Usage:
  python3 -I X-AST_check.py --drafts <ORIG/_work/drafts> --manifest <asset_manifest.csv> --origin <asset_origin_map.csv>

Checks:
  1. every Species filename lives under content/ and never under a rejected root folder;
  2. no Species file stem equals an original file stem (exact, case-insensitive) or the same
     stem with separators and digits removed (reported separately as near matches);
  3. every original file of the input CSVs appears exactly once in the origin map, and every
     in-scope original maps to exactly one Species file that exists in the manifest;
  4. manifest rows are unique per file, name at least one consumer and use a valid priority.
Prints totals per category and priority.  Exit status 1 on any failure.
"""
import argparse
import collections
import csv
import os
import re
import sys

PRIORITIES = {'skirmish-first-playable', 'launch', 'post-launch'}
REJECTED = ('data/', 'code/', 'docs/', 'contrib/', 'targets/')


def read(path):
    with open(path, newline='', encoding='utf-8') as f:
        return list(csv.DictReader(f))


def stem(p):
    return os.path.splitext(os.path.basename(p))[0].lower()


def loose(s):
    return re.sub(r'[^a-z]', '', s.lower())


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--drafts', required=True)
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--origin', required=True)
    a = ap.parse_args()
    d = a.drafts.rstrip('/') + '/'
    fail = 0
    man = read(a.manifest)
    org = read(a.origin)

    # original file inventory
    originals = []
    for name, col in [('X2a_textures.csv', 'path'), ('X2a_shapes.csv', 'path'), ('X2b_samples.csv', 'path'),
                      ('X4_levels.csv', 'file')]:
        originals += [r[col] for r in read(d + name)]
    thumbs = [r['file'] for r in read(d + 'X4_thumbnails.csv')]
    ostems = set(stem(p) for p in originals + thumbs)
    # sample-group and source names are not file stems, but report them as near matches too
    groups = set()
    for r in read(d + 'X2b_samples.csv'):
        groups.update(g.lower() for g in r['via_groups'].split(';') if g)
    oloose = set(loose(s) for s in ostems | groups)

    # 1 + 4: manifest
    files = collections.Counter(r['species_filename'] for r in man)
    for f, n in files.items():
        if n > 1:
            print('FAIL duplicate manifest file', f)
            fail = 1
    for r in man:
        f = r['species_filename']
        if not f.startswith('content/') or any(('/' + x) in ('/' + f) for x in REJECTED):
            print('FAIL bad root', f)
            fail = 1
        if not r['consumers'].strip():
            print('FAIL no consumer', f)
            fail = 1
        if r['priority'] not in PRIORITIES:
            print('FAIL priority', f, r['priority'])
            fail = 1
        if re.search(r'[A-Z ]', f):
            print('FAIL not lower-case snake_case', f)
            fail = 1

    # 2: stems
    exact, near = [], []
    for f in files:
        s = stem(f)
        if s in ostems:
            exact.append(f)
        elif loose(s) and loose(s) in oloose:
            near.append(f)
    for f in exact:
        print('FAIL stem equals an original file stem:', f)
        fail = 1
    print('stem check: %d exact collisions, %d near matches (letters only)' % (len(exact), len(near)))
    for f in near:
        print('  near match:', f)

    # 3: origin coverage
    seen = collections.Counter()
    in_scope = collections.Counter()
    for r in org:
        if r['original_status'] in ('player-facing', 'string'):
            continue
        for p in [p for p in r['original_paths'].split(';') if p]:
            seen[p] += 1
            if not r['species_filename'].startswith('(not created'):
                in_scope[p] += 1
                if r['species_filename'] not in files:
                    print('FAIL origin row names a file missing from the manifest', r['species_filename'])
                    fail = 1
    inv = collections.Counter(originals)
    for p in inv:
        if seen[p] != 1:
            print('FAIL original %s appears %d times in the origin map' % (p, seen[p]))
            fail = 1
    for p in seen:
        if p not in inv:
            print('FAIL origin map names an unknown original', p)
            fail = 1
    print('originals: %d total, %d in scope (each mapped to exactly one Species file), %d excluded' % (
        len(inv), len(in_scope), len(inv) - len(in_scope)))
    by_src = collections.Counter()
    for name, col in [('X2a_textures.csv', 'path'), ('X2a_shapes.csv', 'path'), ('X2b_samples.csv', 'path'),
                      ('X4_levels.csv', 'file')]:
        rows = read(d + name)
        by_src[name] = (len(rows), sum(1 for r in rows if r[col] in in_scope))
    for k, (n, i) in by_src.items():
        print('  %-20s rows %4d  in scope %4d  excluded %4d' % (k, n, i, n - i))

    # totals
    cat = collections.Counter(r['category'] for r in man)
    pri = collections.Counter((r['category'], r['priority']) for r in man)
    ids = collections.defaultdict(set)
    for r in man:
        ids[r['category']].add(r['asset_id'])
    print('manifest: %d files, %d asset ids' % (len(man), len(set(r['asset_id'] for r in man))))
    for c in cat:
        print('  %-36s files %4d  ids %3d  fp %4d  launch %4d  post %3d' % (
            c, cat[c], len(ids[c]), pri[(c, 'skirmish-first-playable')], pri[(c, 'launch')], pri[(c, 'post-launch')]))
    sys.exit(fail)


if __name__ == '__main__':
    main()
