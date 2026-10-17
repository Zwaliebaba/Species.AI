"""FX-08: apply exact text replacements from a spec file.

Usage: python3 -I FX-08_apply.py <root> <spec.json>

spec.json is a list of objects {"file": <path relative to root>, "old": <text>, "new": <text>}
and optionally "count" (default 1): the number of times "old" must occur. Every replacement
of a file is checked before any file is written; a miss or a wrong count stops the run and
writes nothing.
"""
import json
import os
import sys
from collections import OrderedDict

root, spec_path = sys.argv[1], sys.argv[2]
spec = json.load(open(spec_path, encoding='utf-8'))
texts = OrderedDict()
errors = []
for n, item in enumerate(spec):
    path = os.path.join(root, item['file'])
    if path not in texts:
        texts[path] = open(path, encoding='utf-8').read()
    want = item.get('count', 1)
    have = texts[path].count(item['old'])
    if have != want:
        errors.append(f'#{n} {item["file"]}: expected {want}, found {have}: {item["old"][:80]!r}')
        continue
    texts[path] = texts[path].replace(item['old'], item['new'])
if errors:
    print('\n'.join(errors))
    sys.exit(1)
for path, text in texts.items():
    open(path, 'w', encoding='utf-8').write(text)
    print('written', os.path.relpath(path, root), len(text.encode('utf-8')), 'bytes')
