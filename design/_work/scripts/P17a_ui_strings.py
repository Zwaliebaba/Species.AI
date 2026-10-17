#!/usr/bin/env python3
"""P17a helper: extract LANGUAGEPHRASE keys and literal UnicodeString captions
from the P17a partition files and resolve English text.
Usage: python3 -I P17a_ui_strings.py <SRC> <partitions.json> <out.csv>"""
import sys, re, json, csv
src, parts, out = sys.argv[1], sys.argv[2], sys.argv[3]
files = json.load(open(parts))['P17a_ui_framework_editor']['files']
files = [f for f in files if f.endswith('.cpp') or f.endswith('.h')]
eng = {}
for line in open(src + '/data/language/english.txt', encoding='latin-1'):
    m = re.match(r'^(\S+)\s+(.*)$', line.rstrip('\r\n'))
    if m:
        eng.setdefault(m.group(1), m.group(2).strip())
rows = {}
pat_lp = re.compile(r'(?:IS)?LANGUAGEPHRASE\s*\(\s*"([^"]+)"\s*\)')
pat_us = re.compile(r'UnicodeString\s*\(\s*"([^"]*)"\s*\)')
pat_sp = re.compile(r'SetShortProperties\s*\(\s*"([^"]+)"')
for f in files:
    txt = open(src + '/' + f, encoding='latin-1').read()
    for m in pat_lp.finditer(txt):
        k = m.group(1)
        rows.setdefault(('key', k), set()).add(f)
    for m in pat_us.finditer(txt):
        k = m.group(1)
        if k.strip():
            rows.setdefault(('literal', k), set()).add(f)
    # names passed to CreateValueControl / SetShortProperties may be implicit phrase keys
    for m in re.finditer(r'CreateValueControl\s*\(\s*"([^"]+)"', txt):
        rows.setdefault(('valuecontrol-name', m.group(1)), set()).add(f)
with open(out, 'w', newline='') as fh:
    w = csv.writer(fh)
    w.writerow(['kind', 'key_or_text', 'english_text', 'files'])
    for (kind, k), fs in sorted(rows.items()):
        w.writerow([kind, k, eng.get(k, '') if kind != 'literal' else '', ';'.join(sorted(fs))])
print(len(rows), 'rows')
