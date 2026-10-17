#!/usr/bin/env python3
"""Check X-ADR-a ADR files: front matter, MADR sections, ADR references, sizes.

Usage: python3 -I X-ADR-a_check.py <adr_dir>
Prints problems and an index table (number | title | status | date) for 0001-0049.
"""
import os
import re
import sys

SECTIONS = [
    '## Context and Problem Statement',
    '## Decision Drivers',
    '## Considered Options',
    '## Decision Outcome',
    '### Consequences',
    '### Confirmation',
    '## Pros and Cons of the Options',
    '## More Information',
]


def main():
    d = sys.argv[1]
    files = sorted(f for f in os.listdir(d) if re.match(r'\d{4}-.*\.md$', f))
    numbers = {int(f[:4]): f for f in files}
    problems = []
    rows = []
    for f in files:
        n = int(f[:4])
        text = open(os.path.join(d, f), encoding='utf-8').read()
        refs = set(int(x) for x in re.findall(r'ADR-(\d{4})', text))
        for r in sorted(refs):
            if r not in numbers:
                problems.append('%s: cites missing ADR-%04d' % (f, r))
        for target in re.findall(r'\]\(([^)#]+\.md)', text):
            if not target.startswith('http') and not os.path.exists(os.path.normpath(os.path.join(d, target))):
                problems.append('%s: broken link %s' % (f, target))
        if n > 49:
            continue
        size = len(text.encode('utf-8'))
        if size > 120 * 1024:
            problems.append('%s: %d bytes' % (f, size))
        m = re.match(r'---\nstatus: (\w+)\ndate: (\S+)\ndecision-makers: (.+)\n(?:owner-decision: \S+\n)?---\n', text)
        if not m:
            problems.append('%s: front matter' % f)
            continue
        status, date, dm = m.groups()
        if status not in ('accepted', 'proposed') or date != '2026-10-17' or dm != 'Species technology lead':
            problems.append('%s: front matter values' % f)
        pos = -1
        for s in SECTIONS:
            p = text.find('\n' + s + '\n')
            if p < 0 or p < pos:
                problems.append('%s: section %s missing or out of order' % (f, s))
            pos = p
        if status == 'proposed' and 'Default while Proposed' not in text:
            problems.append('%s: proposed without default' % f)
        t = re.search(r'^# (.+)$', text, re.M).group(1)
        t = re.sub(r'^ADR-\d{4} ', '', t)
        rows.append('| [ADR-%04d](%s) | %s | %s | %s |' % (n, f, t, status.capitalize(), date))
    for p in problems:
        print('PROBLEM', p)
    print('\n'.join(rows))
    print('files 0001-0049:', len(rows), 'problems:', len(problems))


if __name__ == '__main__':
    main()
