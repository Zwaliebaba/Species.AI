"""RES-2 helper: for each identifier of CONVENTIONS table 8.12-A, list the folders that cite it in backticks.

Usage: python3 -I RES-2_owners.py <design root>
"""
import os
import re
import sys


def main():
    root = sys.argv[1]
    conv = open(os.path.join(root, 'CONVENTIONS.md'), encoding='utf-8').read().split('\n')
    start = next(i for i, l in enumerate(conv) if l.startswith('### 8.12'))
    rows = []
    for l in conv[start + 1:]:
        if l.startswith('Table 8.12-B'):
            break
        m = re.match(r'^\| (.+?) \| (.+?) \| `([^`]+)`', l)
        if m:
            rows.append((m.group(1), m.group(2), m.group(3)))
    texts = {}
    for r, _, names in os.walk(root):
        for n in names:
            if n.endswith('.md') and n != 'CONVENTIONS.md':
                p = os.path.join(r, n)
                texts[os.path.relpath(p, root)] = open(p, encoding='utf-8').read()
    for term, kind, ident in rows:
        base = ident.split('::')[0]
        pat = re.compile(r'`' + re.escape(ident) + r'[`(:<]|\b(class|struct|enum class|enum)\s+' + re.escape(base) + r'\b')
        hits = []
        for p, t in sorted(texts.items()):
            if pat.search(t):
                parts = p.split('/')
                key = '/'.join(parts[:3]) if parts[0] == 'architecture' else '/'.join(parts[:2])
                if key not in hits:
                    hits.append(key)
        print(f'{ident}\t{term}\t{"; ".join(hits)}')


if __name__ == '__main__':
    main()
