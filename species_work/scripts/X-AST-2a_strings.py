"""Census of string ids cited in the design set (X-AST-2a helper).
String-id prefixes are read from the 'String-id prefix' column of CONVENTIONS.md (as gate 7 does).
Usage: python3 -I X-AST-2a_strings.py <design_root> <manifest_csv> [--list PREFIX] [--check ID ...]"""
import csv, os, re, sys, collections
root, man = sys.argv[1], sys.argv[2]
args = sys.argv[3:]
conv = open(os.path.join(root, 'CONVENTIONS.md'), encoding='utf-8').read()
pref, col = set(), None
for line in conv.split('\n'):
    if not line.startswith('|'):
        col = None; continue
    cells = [c.strip() for c in line.strip().strip('|').split('|')]
    if 'String-id prefix' in cells:
        col = cells.index('String-id prefix')
    elif col is not None and col < len(cells) and not cells[0].startswith('---'):
        pref.update(re.findall(r'`([a-z_]+(?:\.[a-z0-9_]+)*)`', cells[col]))
roots = {p.split('.')[0] for p in pref}
assets = {r['asset_id'] for r in csv.DictReader(open(man, encoding='utf-8'))}
stems = {a.split('.')[0] for a in assets}
keydef = set()
texts = {}
for dp, dn, fn in os.walk(root):
    if 'requirements_index' in dp:
        continue
    for f in fn:
        if f.endswith('.md'):
            p = os.path.join(dp, f); rel = os.path.relpath(p, root)
            t = open(p, encoding='utf-8').read(); texts[rel] = t
            for m in re.finditer(r'^\|\s*`([a-z0-9_.]+)`\s*\|', t, re.M):
                keydef.add(m.group(1))
ID = re.compile(r'`((?:' + '|'.join(sorted(roots, key=len, reverse=True)) + r')\.[a-z0-9_]+(?:\.[a-z0-9_<>{},]+)*)`')
found = collections.defaultdict(set)
for rel, t in texts.items():
    if rel.startswith(('assets/', 'decisions/', 'adr/', 'glossary/')) or rel == 'CONVENTIONS.md':
        continue
    for m in ID.finditer(t):
        s = m.group(1)
        if s in keydef or s.split('.')[0] in stems or s in assets:
            continue
        found[s].add(rel)
if '--check' in args:
    for s in args[args.index('--check') + 1:]:
        print(s, '->', sorted(found.get(s, [])) or 'NOT CITED')
    sys.exit()
if '--list' in args:
    p = args[args.index('--list') + 1]
    for s in sorted(found):
        if s.startswith(p + '.'):
            print(s, len(found[s]))
    sys.exit()
print('registered string prefixes:', len(pref), 'roots:', sorted(roots))
c = collections.Counter(s.split('.')[0] for s in found)
print('distinct cited ids:', len(found))
for k, v in c.most_common():
    print(k, v)
