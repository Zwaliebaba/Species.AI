#!/usr/bin/env python3
"""Extract the replacement text of p4_arbitration.md for the clean-form scan.

Usage: python3 -I X-ARB_extract_replacements.py <p4_arbitration.md> <out.md>

Copies, for every entry, the lines from its "**Replacements.**" marker up to the next
"## " heading. The ORIG evidence sections of the working file may quote original
names; the replacement text is what document fixers paste into design_next and must
pass the forbidden and soft pattern scans. Each output line is prefixed with the source
line number so a hit can be traced back.
"""
import sys


def main():
    if len(sys.argv) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    src, out = sys.argv[1], sys.argv[2]
    keep = False
    rows = []
    with open(src, encoding='utf-8') as f:
        for n, line in enumerate(f, 1):
            if line.startswith('## '):
                keep = False
            if line.startswith('**Replacements.**'):
                keep = True
                continue
            if keep:
                rows.append(f'{n}\t{line}')
    with open(out, 'w', encoding='utf-8') as f:
        f.writelines(rows)
    print(f'{len(rows)} lines written to {out}')
    return 0


if __name__ == '__main__':
    sys.exit(main())
