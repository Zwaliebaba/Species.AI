"""FX-11: check that a split folder holds exactly the content of the original document.

Usage: python3 -I FX-11_verify_split.py <original.md> <folder>

Compares requirement ids (defined), tuning keys (defined), headings (### and ##), test ids,
string-id rows and non-boilerplate lines; checks relative links and anchors in the folder.
"""
import glob
import os
import re
import sys
from collections import Counter

orig = open(sys.argv[1], encoding='utf-8').read()
folder = sys.argv[2]
files = sorted(glob.glob(os.path.join(folder, '*.md')))
new = {os.path.basename(f): open(f, encoding='utf-8').read() for f in files}
alltext = '\n'.join(new.values())


def defs(t):
    return Counter(re.findall(r'^\*\*([A-Z]+-\d{3})\*\*', t, re.M))


def keys(t):
    return Counter(re.findall(r'^\| `([a-z][a-z0-9_.]+)` \|', t, re.M))


def heads(t):
    return set(re.findall(r'^#{2,3} (\d[\d.]* .+)$', t, re.M))


def tests(t):
    return Counter(re.findall(r'^\| (T\d+) \|', t, re.M))


ok = True
for name, fn in [('requirement ids', defs), ('tuning keys / string rows', keys), ('tests', tests)]:
    a, b = fn(orig), fn(alltext)
    if a != b:
        ok = False
        print(f'{name}: missing {sorted(set(a) - set(b))} extra {sorted(set(b) - set(a))} '
              f'dup {[k for k in b if b[k] > 1 and a[k] <= 1]}')
    else:
        print(f'{name}: {sum(a.values())} preserved')
ha, hb = heads(orig), heads(alltext)
print('headings missing:', sorted(ha - hb) or 'none')
ok &= not (ha - hb)

# every non-empty original line (link-normalised) appears in the folder
def norm(l):
    return re.sub(r'\]\([^)]*\)', '](…)', l).strip()
newlines = Counter(norm(l) for l in alltext.split('\n'))
lost = [l for l in orig.split('\n') if norm(l) and newlines[norm(l)] == 0]
print('original lines not found verbatim:', len(lost))
for l in lost[:20]:
    print('  LOST', l[:150])
ok &= not lost

# links
LINK = re.compile(r'(?<!!)\[(?:[^\]\[]|\[[^\]]*\])*\]\(([^)\s]+)\)')


def slug(h):
    h = re.sub(r'!?\[([^\]]*)\]\([^)]*\)', r'\1', h)
    h = re.sub(r'<[^>]+>', '', h).strip().lower()
    return re.sub(r'[^\w\- ]', '', h).replace(' ', '-')


def anchors(path):
    out, seen, inside = set(), Counter(), False
    for line in open(path, encoding='utf-8').read().split('\n'):
        if re.match(r'^[ \t]*(```|~~~)', line):
            inside = not inside
            continue
        if inside:
            continue
        m = re.match(r'^(#{1,6})\s+(.*?)\s*#*\s*$', line)
        if m:
            s = slug(m.group(2))
            out.add(s if not seen[s] else f'{s}-{seen[s]}')
            seen[s] += 1
    return out


bad = 0
for f in files:
    for t in LINK.findall(open(f, encoding='utf-8').read()):
        if re.match(r'^[a-z][a-z0-9+.-]*:', t, re.I):
            continue
        path, _, anchor = t.partition('#')
        dest = os.path.normpath(os.path.join(os.path.dirname(f), path)) if path else f
        if not os.path.exists(dest):
            bad += 1
            print('BROKEN', os.path.basename(f), t)
        elif anchor and dest.endswith('.md') and anchor not in anchors(dest):
            bad += 1
            print('ANCHOR', os.path.basename(f), t)
print('broken links:', bad)
ok &= bad == 0
for n, t in new.items():
    print(f'{len(t.encode()):7} {n}')
print('OK' if ok else 'FAIL')
