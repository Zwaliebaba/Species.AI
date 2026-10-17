"""Print requirement statements by id (X-AST-2a helper).
Usage: python3 -I X-AST-2a_req.py <design_root> <maxchars> ID [ID ...]"""
import os, re, sys
root, maxc, ids = sys.argv[1], int(sys.argv[2]), sys.argv[3:]
want = set(ids)
for dp, dn, fn in os.walk(root):
    if 'requirements_index' in dp:
        continue
    for f in fn:
        if not f.endswith('.md'):
            continue
        p = os.path.join(dp, f)
        for ln, line in enumerate(open(p, encoding='utf-8'), 1):
            m = re.match(r'^\s*(?:\d+\.\s*)?\*\*([A-Z]+(?:-[A-Z]+)?-\d+)\*\*', line)
            if m and m.group(1) in want:
                print('%s [%s:%d] %s' % (m.group(1), os.path.relpath(p, root), ln, line.strip()[:maxc]))
