#!/usr/bin/env python3
"""X-ADR-b helper: check MADR 4 structure, links and ADR references of ADR files,
and write the index rows.

Usage: python3 -I X-ADR-b_check.py <adr_dir> <first> <last> <index_out.md>
Checks every <adr_dir>/NNNN-*.md with first <= NNNN <= last.
"""
import os
import re
import sys

REQ_SECTIONS = [
    '## Context and Problem Statement',
    '## Decision Drivers',
    '## Considered Options',
    '## Decision Outcome',
    '### Consequences',
    '### Confirmation',
    '## Pros and Cons of the Options',
    '## More Information',
]
LINK = re.compile(r'\]\(([^)\s]+)\)')
ADRREF = re.compile(r'ADR-(\d{4})')


def main():
    adr_dir, first, last, out = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4]
    names = sorted(os.listdir(adr_dir))
    existing = {n[:4] for n in names if re.match(r'\d{4}-.*\.md$', n)}
    errors, rows = [], []
    for n in names:
        m = re.match(r'(\d{4})-.*\.md$', n)
        if not m or not (first <= int(m.group(1)) <= last):
            continue
        path = os.path.join(adr_dir, n)
        text = open(path, encoding='utf-8').read()
        fm = re.match(r'---\n(.*?)\n---\n', text, re.S)
        if not fm:
            errors.append(f'{n}: no front matter')
            continue
        meta = dict(line.split(': ', 1) for line in fm.group(1).splitlines() if ': ' in line)
        for k in ('status', 'date', 'decision-makers'):
            if k not in meta:
                errors.append(f'{n}: front matter lacks {k}')
        if meta.get('status') not in ('accepted', 'proposed'):
            errors.append(f'{n}: status {meta.get("status")!r}')
        if meta.get('date') != '2026-10-17':
            errors.append(f'{n}: date {meta.get("date")!r}')
        pos = -1
        for s in REQ_SECTIONS:
            p = text.find('\n' + s + '\n')
            if p < 0:
                errors.append(f'{n}: missing section {s}')
            elif p < pos:
                errors.append(f'{n}: section out of order {s}')
            pos = max(pos, p)
        if 'Chosen option: "' not in text:
            errors.append(f'{n}: no Chosen option line')
        if meta.get('status') == 'proposed' and 'default the team proceeds with' not in text:
            errors.append(f'{n}: proposed ADR does not state the default')
        for target in LINK.findall(text):
            if target.startswith(('http://', 'https://')):
                continue
            file_part = target.split('#', 1)[0]
            if file_part and not os.path.exists(os.path.join(adr_dir, file_part)):
                errors.append(f'{n}: broken link {target}')
        for ref in set(ADRREF.findall(text)):
            if ref not in existing:
                errors.append(f'{n}: cites missing ADR-{ref}')
        title = re.search(r'^# (.+)$', text, re.M).group(1).strip()
        rows.append(f'| [ADR-{m.group(1)}]({n}) | {title} | {meta["status"].capitalize()} | {meta["date"]} |')
        print(f'{n}\t{meta["status"]}\t{len(text.encode("utf-8"))} bytes')
    with open(out, 'w', encoding='utf-8') as f:
        f.write('# ADR index rows from X-ADR-b (ADR-0050 to ADR-0099)\n\n')
        f.write('Rows for `design_next/adr/README.md`; links are relative to `design_next/adr/`.\n\n')
        f.write('| Number | Title | Status | Date |\n|---|---|---|---|\n')
        f.write('\n'.join(rows) + '\n')
    for e in errors:
        print('ERROR', e)
    return 1 if errors else 0


if __name__ == '__main__':
    sys.exit(main())
