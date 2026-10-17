#!/usr/bin/env python3
"""Check that every input range of the package appears in the DEC map's orig_refs.

Usage: python3 -I X-DEC-2b_coverage.py <package.json> <dec_map.csv>
"""
import csv
import json
import sys


def main():
    pkg = json.load(open(sys.argv[1], encoding='utf-8'))
    refs = {}
    with open(sys.argv[2], encoding='utf-8', newline='') as fh:
        for row in csv.DictReader(fh):
            for r in row['orig_refs'].split(';'):
                refs.setdefault(r, []).append(row['dec_id'])
    want = ['%s:%d-%d' % (i['file'], i['start_line'], i['end_line']) for i in pkg['inputs']]
    missing = [w for w in want if w not in refs]
    unknown = sorted(set(refs) - set(want))
    for w in want:
        print('%-48s %s' % (w, ','.join(refs.get(w, ['MISSING']))))
    print('inputs %d, missing %d, refs not in package %d' % (len(want), len(missing), len(unknown)))
    for u in unknown:
        print('not in package:', u)
    sys.exit(1 if missing or unknown else 0)


if __name__ == '__main__':
    main()
