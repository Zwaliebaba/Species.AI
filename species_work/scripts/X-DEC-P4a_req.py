"""Print requirement blocks by id from the design set.
Usage: python3 -I X-DEC-P4a_req.py <design_root> <ID> [<ID> ...]
A block starts at the line that defines **ID** and ends before the next requirement definition or heading."""
import os, re, sys
root = sys.argv[1]
ids = sys.argv[2:]
DEF = re.compile(r'\*\*((?:R-)?[A-Z]+-\d{3})\*\*')
for dp, dn, fn in os.walk(root):
    for n in sorted(fn):
        if not n.endswith('.md'):
            continue
        p = os.path.join(dp, n)
        lines = open(p, encoding='utf-8').read().split('\n')
        for i, l in enumerate(lines):
            m = DEF.search(l)
            if m and m.group(1) in ids and (l.lstrip().startswith('**' + m.group(1)) or l.lstrip().startswith('- **' + m.group(1)) or re.match(r'^\s*(\d+\.|-|\|)?\s*\*\*' + re.escape(m.group(1)), l)):
                out = [l]
                for l2 in lines[i + 1:i + 25]:
                    if DEF.search(l2) and re.match(r'^\s*(\d+\.|-)?\s*\*\*(R-)?[A-Z]+-\d{3}\*\*', l2) or l2.startswith('#'):
                        break
                    out.append(l2)
                print(f'== {os.path.relpath(p, root)}:{i + 1}')
                print('\n'.join(out).strip()[:3000])
