"""Report where each requirement id is defined (file), or MISSING.
Usage: python3 -I X-DEC-P4a_ids.py <design_root> <ID> [<ID> ...]"""
import os, re, sys
root = sys.argv[1]
want = sys.argv[2:]
where = {}
DEF = re.compile(r'^\s*(?:\d+\.\s+|-\s+|\|\s*)?\*\*((?:R-)?[A-Z]+-\d{3})\*\*')
for dp, dn, fn in os.walk(root):
    for n in fn:
        if n.endswith('.md'):
            p = os.path.join(dp, n)
            for l in open(p, encoding='utf-8'):
                m = DEF.match(l)
                if m:
                    where.setdefault(m.group(1), os.path.relpath(p, root))
for w in want:
    print(w, where.get(w, 'MISSING'))
