#!/usr/bin/env python3
"""X-ADR-b helper: rewrite baseline section citations into README item citations.

Usage: python3 -I X-ADR-b_fixrefs.py <file> [<file> ...]
"§1.1 and §1.2 of the technology baseline in the [ADR index](README.md)" becomes
"technology baseline items 1.1 and 1.2 in the [ADR index](README.md#technology-baseline)".
"""
import re
import sys

CIT = re.compile(r'((?:§[\d.]+(?:, | and ))*§[\d.]+) of the technology baseline in the \[ADR index\]\(README\.md\)')
UNVER = re.compile(r'\(baseline §12\.1\)')


def fix(m):
    items = m.group(1).replace('§', '')
    word = 'item' if (',' not in items and ' and ' not in items) else 'items'
    return f'technology baseline {word} {items} in the [ADR index](README.md#technology-baseline)'


for path in sys.argv[1:]:
    text = open(path, encoding='utf-8').read()
    new = CIT.sub(fix, text)
    new = UNVER.sub('(see [Items not verified](README.md#items-not-verified))', new)
    if new != text:
        open(path, 'w', encoding='utf-8').write(new)
        print('fixed', path)
