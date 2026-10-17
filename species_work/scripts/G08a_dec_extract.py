import sys, re, pathlib
root = pathlib.Path(sys.argv[1])
ids = sys.argv[2:]
files = list(root.rglob('*.md'))
for i in ids:
    pat = re.compile(r'^### DEC-%s\b' % i)
    for f in files:
        if f.name == 'README.md':
            continue
        lines = f.read_text(encoding='utf-8').splitlines()
        for n, l in enumerate(lines):
            if pat.match(l):
                out = [l]
                for m in lines[n+1:]:
                    if m.startswith('### ') or m.startswith('## '):
                        break
                    out.append(m)
                print('\n'.join(out).rstrip())
                print('-----')
