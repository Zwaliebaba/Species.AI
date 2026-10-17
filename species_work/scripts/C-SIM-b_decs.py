import sys, re, pathlib
root = pathlib.Path(sys.argv[1])
ids = sys.argv[2].split(',')
files = list(root.glob('*.md')) + list(root.glob('deferred/*.md'))
for f in files:
    if f.name == 'README.md':
        continue
    text = f.read_text(encoding='utf-8')
    parts = re.split(r'(?m)^(?=### DEC-\d+)', text)
    for p in parts:
        m = re.match(r'### (DEC-\d+)', p)
        if m and m.group(1) in ids:
            print('=== FILE', f.name)
            print(p.strip())
            print()
