"""Read-only: list explicit <a id="..."> anchors under a design folder and the links that target each one.

Usage: python3 -I P6-X2_anchors.py <design_folder>
"""
import os
import re
import sys

root = sys.argv[1]
anchor_re = re.compile(r'<a id="([^"]+)"></a>')
link_re = re.compile(r'\]\(([^)\s]*)\)')
heading_re = re.compile(r'^(#{1,6})\s+(.*)$')

files = []
for d, _, fs in os.walk(root):
    for f in fs:
        if f.endswith('.md'):
            files.append(os.path.join(d, f))

def slug(text):
    t = text.strip().lower()
    t = re.sub(r'[^\w\- ]', '', t)
    return t.replace(' ', '-')

anchors = []  # (file, line, id, heading_slug_same)
headings = {}
for p in files:
    with open(p, encoding='utf-8') as fh:
        lines = fh.read().split('\n')
    hs = set()
    for i, line in enumerate(lines):
        m = heading_re.match(line)
        if m:
            hs.add(slug(m.group(2)))
    headings[p] = hs
    for i, line in enumerate(lines):
        for m in anchor_re.finditer(line):
            anchors.append((p, i + 1, m.group(1), line.strip() == m.group(0)))

targets = {}
for p in files:
    with open(p, encoding='utf-8') as fh:
        text = fh.read()
    for m in link_re.finditer(text):
        href = m.group(1)
        if '#' not in href:
            continue
        path, frag = href.split('#', 1)
        tgt = os.path.normpath(os.path.join(os.path.dirname(p), path)) if path else p
        targets.setdefault((tgt, frag), []).append(os.path.relpath(p, root))

for p, ln, aid, alone in anchors:
    users = targets.get((os.path.normpath(p), aid), [])
    nonindex = [u for u in users if not u.startswith('requirements_index') and u != 'REQUIREMENTS_INDEX.md']
    dup = aid in headings[p]
    print(f"{os.path.relpath(p, root)}:{ln}\t{aid}\tline-only={alone}\theading-slug-also={dup}\tindex-links={len(users)-len(nonindex)}\tother-links={len(nonindex)}\t{','.join(sorted(set(nonindex)))[:200]}")
