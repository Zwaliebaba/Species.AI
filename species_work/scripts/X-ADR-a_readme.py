#!/usr/bin/env python3
"""Update design_next/adr/README.md: index rows for 0001-0049 and the "Decided in" column.

Usage: python3 -I X-ADR-a_readme.py <adr_dir>
- Replaces the rows between the index table header and the marker line
  "<!-- X-ADR-b rows -->" with one row per ADR file numbered 0001-0049.
- Sets the "Decided in" cell of baseline rows owned by the 0050-0099 range to links,
  but only for ADR files that exist.
"""
import os
import re
import sys

DECIDED = {
    '1.1': [50], '1.2': [50], '2.1': [51], '2.2 | Language': [52], '2.2 | `import std`': [53],
    '2.3': [52, 51], '3.1': [54], '3.2': [55], '4.7': [68], '5.1': [58],
    '10.1': [64], '10.2': [63], '11.1': [65], '11.2': [71, 72], '11.3': [73],
}


def title_status(path):
    text = open(path, encoding='utf-8').read()
    status = re.search(r'^status: (\w+)', text, re.M).group(1).capitalize()
    title = re.search(r'^# (.+)$', text, re.M).group(1)
    return title, status


def main():
    d = sys.argv[1]
    files = {int(f[:4]): f for f in os.listdir(d) if re.match(r'\d{4}-.*\.md$', f)}
    readme = os.path.join(d, 'README.md')
    text = open(readme, encoding='utf-8').read()

    rows = []
    for n in sorted(k for k in files if 1 <= k <= 49):
        t, s = title_status(os.path.join(d, files[n]))
        rows.append('| [ADR-%04d](%s) | %s | %s | 2026-10-17 |' % (n, files[n], t, s))
    head = '| Number | Title | Status | Date |\n|---|---|---|---|\n'
    marker = '<!-- X-ADR-b rows -->'
    start = text.index(head) + len(head)
    end = text.index(marker)
    text = text[:start] + '\n'.join(rows) + '\n' + text[end:]

    lines = text.split('\n')
    for i, line in enumerate(lines):
        for key, nums in DECIDED.items():
            if line.startswith('| ' + key + ' |'):
                links = ['[ADR-%04d](%s)' % (n, files[n]) for n in nums if n in files]
                if links:
                    cells = line.split(' | ')
                    cells[-1] = ', '.join(links) + ' |'
                    lines[i] = ' | '.join(cells)
    text = '\n'.join(lines)
    open(readme, 'w', encoding='utf-8').write(text)
    print('rows', len(rows))


if __name__ == '__main__':
    main()
