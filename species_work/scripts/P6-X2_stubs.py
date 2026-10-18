"""Read-only: list requirements whose text is a "(removed: ...)" stub and every file that cites each one.

Usage: python3 -I P6-X2_stubs.py <design_folder>
"""
import os
import re
import sys

root = sys.argv[1]
texts = {}
for d, _, fs in os.walk(root):
    for f in fs:
        if f.endswith('.md'):
            p = os.path.join(d, f)
            with open(p, encoding='utf-8') as fh:
                texts[os.path.relpath(p, root)] = fh.read()

def_rx = re.compile(r'\*\*((?:R-)?[A-Z]{2,6}-\d{3})\*\*([^\n]*)')
stubs = []
for f, t in texts.items():
    for m in def_rx.finditer(t):
        rest = m.group(2)
        if '(removed' in rest[:60]:
            stubs.append((f, m.group(1), rest.strip()[:140]))

for f, rid, rest in sorted(stubs):
    cite_rx = re.compile(r'(?<![\w-])' + re.escape(rid) + r'(?!\d)')
    users = []
    for g, t in texts.items():
        n = len(cite_rx.findall(t))
        if g == f:
            n -= 1  # the definition itself
        if n > 0:
            users.append(f'{g}({n})')
    nonindex = [u for u in users if not u.startswith('requirements_index') and not u.startswith('REQUIREMENTS_INDEX')]
    print(f'{rid}\t{f}\tnonindex={";".join(nonindex) or "-"}\tindex={";".join(u for u in users if u not in nonindex) or "-"}\t{rest}')
