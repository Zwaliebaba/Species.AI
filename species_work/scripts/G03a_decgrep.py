import sys, re, pathlib
root = pathlib.Path(sys.argv[1])
kw = [k.lower() for k in sys.argv[2].split('|')]
skip = set(sys.argv[3].split(',')) if len(sys.argv) > 3 else set()
for f in sorted(root.rglob('*.md')):
    text = f.read_text(encoding='utf-8')
    parts = re.split(r'(?m)^### (DEC-\d+)', text)
    for i in range(1, len(parts), 2):
        did, body = parts[i], parts[i+1]
        if did in skip: continue
        b = body.lower()
        hits = [k for k in kw if k in b]
        if hits:
            title = body.split('\n',1)[0].strip()
            m = re.search(r'Status:\s*(\w+)', body)
            print(did, m.group(1) if m else '?', title[:90], '|', ','.join(hits), '|', f.name)
