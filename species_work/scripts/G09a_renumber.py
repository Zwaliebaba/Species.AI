"""G09a: renumber R-META requirement ids across files.
Usage: python3 -I G09a_renumber.py <file>:<start> [<file>:<start> ...]
Definitions (**R-META-NNN**) in each file are renumbered in order from <start>;
every R-META-NNN token in every listed file is rewritten through one mapping."""
import re, sys
specs = [a.rsplit(':', 1) for a in sys.argv[1:]]
texts = {p: open(p, encoding='utf-8').read() for p, _ in specs}
mapping = {}
for p, start in specs:
    n = int(start)
    for m in re.finditer(r'\*\*R-META-(\d{3})\*\*', texts[p]):
        old = m.group(1)
        if old in mapping:
            print('DUPLICATE definition', old, p); sys.exit(1)
        mapping[old] = '%03d' % n
        n += 1
    print(p, 'defs', n - int(start), 'last', '%03d' % (n - 1))
missing = set()
for p in texts:
    for m in re.finditer(r'R-META-(\d{3})', texts[p]):
        if m.group(1) not in mapping:
            missing.add((p, m.group(1)))
if missing:
    print('MISSING', sorted(missing)); sys.exit(1)
for p in texts:
    t = re.sub(r'R-META-(\d{3})', lambda m: 'R-META-' + mapping[m.group(1)], texts[p])
    open(p, 'w', encoding='utf-8').write(t)
print('renumbered', sum(1 for k, v in mapping.items() if k != v), 'ids')
