"""Print requirement blocks by id from a design root.
Usage: python3 -I X-DEC-P4b_req.py <design_root> <ID> [<ID> ...]
A block starts at a line containing **<ID>** and ends before the next **<PREFIX>-NNN** line or heading."""
import os, re, sys
root = sys.argv[1]
ids = sys.argv[2:]
start_re = re.compile(r'\*\*((?:R-)?[A-Z]+-\d{3})\*\*')
files = []
for d, _, fs in os.walk(root):
    for f in fs:
        if f.endswith('.md'):
            files.append(os.path.join(d, f))
for want in ids:
    found = False
    for p in sorted(files):
        lines = open(p, encoding='utf-8').read().split('\n')
        for i, l in enumerate(lines):
            m = start_re.search(l)
            if m and m.group(1) == want and l.lstrip().startswith(('**', '- **', '| **')) or (m and m.group(1) == want and l.startswith('**')):
                out = [l]
                for nl in lines[i + 1:]:
                    if start_re.search(nl) and (nl.startswith('**') or nl.startswith('- **')) or nl.startswith('#'):
                        break
                    out.append(nl)
                print(f'=== {want} @ {os.path.relpath(p, root)}:{i + 1}')
                print('\n'.join(out).rstrip())
                found = True
                break
        if found:
            break
    if not found:
        print(f'=== {want}: NOT FOUND')
