"""FX-02 helper: list tuning keys, requirement ids, DEC ids and asset ids cited in the given files
that the library does not define. Usage: python3 -I FX-02_keycheck.py <design_root> <file>..."""
import os
import re
import sys

root = sys.argv[1]
files = sys.argv[2:]

defined_keys = set()
defined_reqs = set()
defined_decs = set()
key_row = re.compile(r'^\| `(ref\.[a-z0-9_.]+|[a-z]+\.[a-z0-9_.]+)` \|')
req_def = re.compile(r'\*\*([A-Z]+(?:-[A-Z]+)*-\d{2,3})\*\*')
dec_def = re.compile(r'^#{2,4} DEC-(\d{3})\s')
for dirpath, _dirs, names in os.walk(root):
    for name in names:
        if not name.endswith('.md'):
            continue
        path = os.path.join(dirpath, name)
        with open(path, encoding='utf-8') as fh:
            for line in fh:
                m = key_row.match(line)
                if m:
                    defined_keys.add(m.group(1))
                for r in req_def.findall(line):
                    defined_reqs.add(r)
                m = dec_def.match(line)
                if m:
                    defined_decs.add(m.group(1))

assets = set()
manifest = os.path.join(root, 'assets', 'asset_manifest.csv')
with open(manifest, encoding='utf-8') as fh:
    for line in fh:
        assets.add(line.split(',', 1)[0])

cited_key = re.compile(r'`(ref\.[a-z0-9_.]+)`')
cited_req = re.compile(r'\b((?:R-[A-Z]+|INP|DAT|RND|AEN|UI|VFX|CAM|AST|BLD|DBG|PLT|TER)-\d{3})\b')
cited_dec = re.compile(r'\bDEC-(\d{3})\b')
asset_like = re.compile(r'`((?:sfx|fx|mesh|marker|help|anim|uitex|icon|mapfile|sprite|music|amb|strings|cursor)\.[a-z0-9_.]+)`')
bad = 0
for f in files:
    with open(f, encoding='utf-8') as fh:
        for n, line in enumerate(fh, 1):
            for k in cited_key.findall(line):
                if k not in defined_keys and not k.endswith('.'):
                    print(f'{f}:{n}: undefined key {k}')
                    bad += 1
            for r in cited_req.findall(line):
                if r not in defined_reqs:
                    print(f'{f}:{n}: undefined requirement {r}')
                    bad += 1
            for d in cited_dec.findall(line):
                if d not in defined_decs:
                    print(f'{f}:{n}: undefined DEC-{d}')
                    bad += 1
            for a in asset_like.findall(line):
                if a not in assets:
                    print(f'{f}:{n}: asset id not in manifest {a}')
                    bad += 1
print(f'{bad} problems')
