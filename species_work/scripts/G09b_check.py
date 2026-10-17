#!/usr/bin/env python3
"""G09b checks for one reference document.

Usage: python3 -I G09b_check.py <doc.md> <design_next root> <asset_manifest.csv>
Checks requirement ids, tuning keys, cross references, DEC/ADR ids, asset ids,
template headings and truncation words. Prints findings; exit 0 always.
"""
import csv
import os
import re
import sys

doc, root, manifest = sys.argv[1], sys.argv[2], sys.argv[3]
text = open(doc, encoding='utf-8').read()
lines = text.split('\n')

# strip fenced code blocks for prose checks
prose, fence = [], False
for ln in lines:
    if re.match(r'^\s*(```|~~~)', ln):
        fence = not fence
        continue
    if not fence:
        prose.append(ln)
prose_text = '\n'.join(prose)

# 1 requirement ids
defs = re.findall(r'^\*\*(R-META-(\d{3}))\*\* \((MUST|MUST NOT|SHOULD|SHOULD NOT|MAY)\)', text, re.M)
ids = [d[0] for d in defs]
nums = [int(d[1]) for d in defs]
print('requirements defined:', len(ids))
dups = sorted({i for i in ids if ids.count(i) > 1})
print('duplicate ids:', dups or 'none')
out = [i for i, n in zip(ids, nums) if not 400 <= n <= 699]
print('out of range:', out or 'none')
gaps = [n for n in range(min(nums), max(nums) + 1) if n not in nums]
print('gaps:', gaps or 'none')
print('order ok:', nums == sorted(nums))

# 2 cited R-META ids exist (own range must be defined here)
cited_meta = set(re.findall(r'R-META-(\d{3})', text))
missing_own = sorted(c for c in cited_meta if 400 <= int(c) <= 699 and 'R-META-' + c not in ids)
print('cited own ids not defined:', missing_own or 'none')

# collect every requirement id defined across design_next
alldefs = set()
for dp, dn, fn in os.walk(root):
    for f in fn:
        if f.endswith('.md'):
            t = open(os.path.join(dp, f), encoding='utf-8').read()
            alldefs.update(re.findall(r'\*\*((?:R-[A-Z]+|[A-Z]{2,4})-\d{3})\*\*', t))
others = set(re.findall(r'\b((?:R-(?:MODE|UNIT|BLDG|CRATE|ECON|CTRL|BOT|MAP|META|AUD|TUT|SIM|VIS))-\d{3})\b', text))
others |= set(re.findall(r'\b((?:RND|UI|AEN|PLT|DAT|INP|DBG)-\d{3})\b', text))
missing_other = sorted(o for o in others if o not in alldefs and not (o.startswith('R-META-') and 400 <= int(o[-3:]) <= 699))
print('cited ids not found in design_next (may be pending):', missing_other or 'none')

# 3 tuning keys
sec4 = text.split('## 4 Tuning table', 1)[1].split('### 4.2', 1)[0]
keys = re.findall(r'^\| `(ref\.meta\.[a-z0-9_.]+)` \|', sec4, re.M)
print('tuning keys defined:', len(keys))
kd = sorted({k for k in keys if keys.count(k) > 1})
print('duplicate keys:', kd or 'none')
sec2 = text.split('## 2 Rules', 1)[1].split('## 3 States and flows', 1)[0]
uncited = [k for k in keys if '`' + k + '`' not in sec2]
print('keys not cited in §2:', uncited or 'none')
used_meta = set(re.findall(r'`(ref\.meta\.[a-z0-9_.]+)`', text))
undefined_meta = sorted(k for k in used_meta if k not in keys)
# check undefined ref.meta keys exist elsewhere (G09a)
elsewhere = {}
for dp, dn, fn in os.walk(os.path.join(root, 'reference')):
    for f in fn:
        if f.endswith('.md') and os.path.join(dp, f) != os.path.abspath(doc):
            t = open(os.path.join(dp, f), encoding='utf-8').read()
            for k in re.findall(r'^\| `(ref\.[a-z0-9_.]+)` \|', t, re.M):
                elsewhere[k] = f
print('ref.meta keys used but not defined here:', [(k, elsewhere.get(k, 'MISSING')) for k in undefined_meta] or 'none')
ext = sorted(set(re.findall(r'`(ref\.(?!meta\.)[a-z0-9_.]+)`', text)))
print('external ref keys:', [(k, elsewhere.get(k, 'MISSING')) for k in ext])
rnd = sorted(set(re.findall(r'`(rnd\.[a-z0-9_.]+)`', text)))
rtext = open(os.path.join(root, 'architecture/components/renderer/options_and_budgets.md'), encoding='utf-8').read()
print('rnd keys missing in renderer options:', [k for k in rnd if k not in rtext] or 'none')

# 4 DEC and ADR ids
decidx = open(os.path.join(root, 'decisions/README.md'), encoding='utf-8').read()
decs = sorted(set(re.findall(r'DEC-(\d{3})', text)))
print('DEC cited:', len(decs), 'missing:', [d for d in decs if '| DEC-' + d + ' |' not in decidx] or 'none')
adrs = sorted(set(re.findall(r'ADR-(\d{4})', text)))
adrfiles = os.listdir(os.path.join(root, 'adr'))
print('ADR cited:', adrs, 'missing:', [a for a in adrs if not any(f.startswith(a + '-') for f in adrfiles)] or 'none')

# 5 asset ids
assets = set()
with open(manifest, newline='', encoding='utf-8') as fh:
    for row in csv.DictReader(fh):
        assets.add(row['asset_id'])
cand = set(re.findall(r'`((?:ui|font|strings|fx|icon|sprite|help|mesh|sfx|map)\.[a-z0-9_.]+)`', text))
asset_like = sorted(c for c in cand if c.split('.')[0] in ('font', 'strings', 'fx', 'icon', 'sprite', 'mesh', 'sfx') or c in ('ui.icon_check',))
print('asset ids cited:', asset_like)
print('asset ids missing from manifest:', [a for a in asset_like if a not in assets] or 'none')

# 6 template headings
need = ['## 1 Purpose and player experience', '## 2 Rules', '## 3 States and flows', '## 4 Tuning table',
        '## 5 Edge cases and interactions', '## 6 Acceptance criteria', '## 7 Dependencies', '## 8 Open decisions']
print('missing headings:', [h for h in need if h not in lines] or 'none')

# 7 truncation words
trunc = re.findall(r'\betc\.|\band so on\b|\band others\b|\bvarious\b|similar to the above|…', prose_text)
print('truncation hits:', trunc or 'none')

# 8 mermaid blocks
print('mermaid blocks:', text.count('```mermaid'))

# 9 links resolve
base = os.path.dirname(os.path.abspath(doc))
bad = []
for m in re.finditer(r'\]\(([^)#\s]+)(#[^)]*)?\)', text):
    target = m.group(1)
    if target.startswith('http'):
        continue
    if not os.path.exists(os.path.normpath(os.path.join(base, target))):
        bad.append(target)
print('unresolved links (may be pending files):', sorted(set(bad)) or 'none')

# 10 string ids defined in 4.3
sec43 = text.split('### 4.3', 1)[1].split('## 5 ', 1)[0]
sdefs = re.findall(r'^\| `([a-z_.0-9]+)` \|', sec43, re.M)
print('string ids defined:', len(sdefs), 'dups:', sorted({s for s in sdefs if sdefs.count(s) > 1}) or 'none')
used_s = set(re.findall(r'`((?:ui|achievement)\.[a-z0-9_.]+)`', sec2))
print('string ids used in §2 but not defined in 4.3:', sorted(s for s in used_s if s not in sdefs and s != 'ui.icon_check'))
print('size bytes:', len(text.encode('utf-8')))
