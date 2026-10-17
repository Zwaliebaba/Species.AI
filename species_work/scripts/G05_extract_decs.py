import sys, re, pathlib
# usage: G05_extract_decs.py <decisions_dir> DEC-001 DEC-002 ...
root = pathlib.Path(sys.argv[1])
want = set(sys.argv[2:])
for f in sorted(root.rglob('*.md')):
    if f.name == 'README.md':
        continue
    text = f.read_text(encoding='utf-8')
    parts = re.split(r'(?m)^(?=### DEC-\d{3})', text)
    for p in parts:
        m = re.match(r'### (DEC-\d{3})', p)
        if m and m.group(1) in want:
            print('=====', f.name)
            print(p.rstrip())
            print()
