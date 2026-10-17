#!/usr/bin/env python3
"""Recount per-file census columns of ORIG 08 over the 49 match maps.

Usage: python3 -I X-ARB_census.py <ORIG 08 levels file>

Reads the generated per-file camera-mount table of the ORIG levels document, drops the
eight non-match files, and prints the per-name mount counts over the 49 match maps and
over all 57 files, so the derivation in p4_arbitration.md A-36 can be checked.
"""
import re
import sys
from collections import Counter

NON_MATCH = {
    'mp_assault_stress', 'mp_benchmark_1', 'mp_sandbox', 'mp_sandbox_all',
    'mp_tankbattle_2P', 'mp_tankbattle_3P', 'mp_tankbattle_4P', 'mp_cts_3p_3',
}
ROW = re.compile(r'^\| `data/levels/(mp_[^`]+)\.txt` \|')


def rows_between(lines, start_marker, header_word):
    out = {}
    active = False
    for line in lines:
        if start_marker in line:
            active = True
            continue
        if active and line.startswith('| File |') and header_word in line:
            continue
        if active:
            m = ROW.match(line)
            if m:
                out[m.group(1)] = [c.strip() for c in line.split('|')[1:-1]]
            elif out and not line.startswith('|'):
                break
    return out


def main():
    if len(sys.argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    lines = open(sys.argv[1], encoding='utf-8').read().split('\n')
    # camera-mount table: header "| File | Camera mounts | ..."
    mounts = {}
    for i, line in enumerate(lines):
        if line.startswith('| File | Camera mounts |'):
            for row in lines[i + 2:]:
                m = ROW.match(row)
                if not m:
                    break
                mounts[m.group(1)] = row.split('|')[2].strip()
            break
    for scope, keep in (('57 files', lambda f: True), ('49 match maps', lambda f: f not in NON_MATCH)):
        c = Counter()
        n = 0
        for f, cell in mounts.items():
            if not keep(f):
                continue
            n += 1
            for name in cell.split():
                if name != '—':
                    c[name] += 1
        print(f'camera mounts over {scope} ({n} files):', dict(sorted(c.items())))
    return 0


if __name__ == '__main__':
    sys.exit(main())
