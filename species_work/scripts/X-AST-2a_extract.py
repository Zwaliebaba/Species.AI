"""Print the asset sections of the Phase 3 reports (X-AST-2a helper).
Usage: python3 -I X-AST-2a_extract.py <reports_dir> [pattern]"""
import os, re, sys
d = sys.argv[1]
pat = re.compile(sys.argv[2] if len(sys.argv) > 2 else r'missing assets|events and assets', re.I)
for fn in sorted(os.listdir(d)):
    if not fn.endswith('.md') or fn.startswith(('X-AST', 'F', 'X-DEC', 'X-ADR')):
        continue
    lines = open(os.path.join(d, fn), encoding='utf-8').read().split('\n')
    i = 0
    while i < len(lines):
        m = re.match(r'^(#+)\s', lines[i])
        if m and pat.search(lines[i]):
            lvl = len(m.group(1))
            out = [lines[i]]
            j = i + 1
            while j < len(lines):
                m2 = re.match(r'^(#+)\s', lines[j])
                if m2 and len(m2.group(1)) <= lvl:
                    break
                out.append(lines[j]); j += 1
            print('=====', fn)
            print('\n'.join(out).strip())
            i = j
        else:
            i += 1
