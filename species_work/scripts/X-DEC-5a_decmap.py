#!/usr/bin/env python3
"""Build species_work/dec_map_X-DEC-5a.csv from the X-DEC-5a register file.

Usage: python3 -I X-DEC-5a_decmap.py <register.md> <out.csv>

Columns: dec_id, title, status, affects_prefix, affects_files, orig_refs.
Titles, statuses and Affects come from the register; orig_refs are listed below.
"""
import csv
import re
import sys

ORIG_REFS = {
    'DEC-650': ['21_simulation_and_entity_model.md:1100-1113'],
    'DEC-651': ['21_simulation_and_entity_model.md:1201-1205'],
    'DEC-652': ['22_determinism_and_networking.md:1045-1056',
                '22_determinism_and_networking.md:1789-1834'],
    'DEC-653': ['22_determinism_and_networking.md:1394-1406',
                '22_determinism_and_networking.md:1789-1834'],
    'DEC-654': ['22_determinism_and_networking.md:1789-1834'],
}

HEAD = re.compile(r'^### (DEC-\d{3}) (.+?)\s*$')
STATUS = re.compile(r'^- Status: (\w+)')
AFFECTS = re.compile(r'^- Affects: (.+)$')
ITEM = re.compile(r'((?:reference|architecture)/[\w./-]+)\s*\(([A-Z][A-Z-]*)\)')


def main(src, out):
    rows, cur = [], None
    with open(src, encoding='utf-8') as f:
        for line in f:
            m = HEAD.match(line)
            if m:
                cur = {'dec_id': m.group(1), 'title': m.group(2)}
                rows.append(cur)
                continue
            if cur is None:
                continue
            m = STATUS.match(line)
            if m:
                cur['status'] = m.group(1)
                continue
            m = AFFECTS.match(line)
            if m:
                files, prefixes = [], []
                for path, prefix in ITEM.findall(m.group(1)):
                    if path not in files:
                        files.append(path)
                    if prefix not in prefixes:
                        prefixes.append(prefix)
                cur['affects_files'] = ';'.join(files)
                cur['affects_prefix'] = ';'.join(prefixes)
    missing = [r['dec_id'] for r in rows if r['dec_id'] not in ORIG_REFS]
    extra = sorted(set(ORIG_REFS) - {r['dec_id'] for r in rows})
    if missing or extra:
        sys.exit(f'orig_refs mismatch: missing {missing}, extra {extra}')
    cols = ['dec_id', 'title', 'status', 'affects_prefix', 'affects_files', 'orig_refs']
    with open(out, 'w', encoding='utf-8', newline='') as f:
        w = csv.DictWriter(f, fieldnames=cols, lineterminator='\n')
        w.writeheader()
        for r in rows:
            r['orig_refs'] = ';'.join(ORIG_REFS[r['dec_id']])
            w.writerow({c: r.get(c, '') for c in cols})
    print(f'{len(rows)} rows written to {out}')


if __name__ == '__main__':
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    main(sys.argv[1], sys.argv[2])
