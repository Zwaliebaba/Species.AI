import sys, re
src, lang = sys.argv[1], sys.argv[2]
code = open(src, encoding='latin-1').read()
ids = []
for m in re.finditer(r'LANGUAGEPHRASE\s*\(\s*(?:gamepad\s*\?\s*)?"([^"]+)"(?:\s*:\s*"([^"]+)")?', code):
    for g in m.groups():
        if g and g not in ids: ids.append(g)
for m in re.finditer(r'RenderControlHelp\(\s*\d+\s*,\s*"([^"]+)"', code):
    if m.group(1) not in ids: ids.append(m.group(1))
for m in re.finditer(r'LANGUAGEPHRASE\(\s*gamepad\s*\?\s*"([^"]+)"\s*:\s*"([^"]+)"', code):
    for g in m.groups():
        if g not in ids: ids.append(g)
for m in re.finditer(r'"(multiwinia_[a-z0-9_]+)"', code):
    if m.group(1) not in ids: ids.append(m.group(1))
table = {}
for line in open(lang, encoding='utf-8', errors='replace'):
    line=line.rstrip('\r\n')
    if not line or line.startswith('#'): continue
    parts = re.split(r'\s+', line, 1)
    if len(parts)==2: table[parts[0]] = parts[1]
for i in ids:
    t = table.get(i)
    print(f"{i}\t{'MISSING' if t is None else t[:160]}")
