import sys, re, pathlib
# usage: G01_extract_decs.py <decisions_dir> <id,id,...>
d = pathlib.Path(sys.argv[1])
ids = set(sys.argv[2].split(','))
for f in sorted(d.rglob('*.md')):
    if f.name == 'README.md':
        continue
    text = f.read_text(encoding='utf-8')
    parts = re.split(r'(?m)^(?=### DEC-\d+)', text)
    for p in parts:
        m = re.match(r'### (DEC-\d+)', p)
        if m and m.group(1) in ids:
            print(f'--- {f.name}')
            print(p.strip())
            print()
