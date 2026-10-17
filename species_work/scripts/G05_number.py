# usage: G05_number.py <file> [--apply]
import sys, re, pathlib
p = pathlib.Path(sys.argv[1])
t = p.read_text(encoding='utf-8')
defs = re.findall(r'\*\*R-ECON-\{([a-z0-9_]+)\}\*\*', t)
dups = {d for d in defs if defs.count(d) > 1}
refs = set(re.findall(r'R-ECON-\{([a-z0-9_]+)\}', t))
undef = sorted(refs - set(defs))
print('definitions', len(defs), 'duplicates', sorted(dups), 'undefined', undef)
if '--apply' in sys.argv and not dups and not undef:
    num = {d: '%03d' % (i + 1) for i, d in enumerate(defs)}
    t2 = re.sub(r'R-ECON-\{([a-z0-9_]+)\}', lambda m: 'R-ECON-' + num[m.group(1)], t)
    p.write_text(t2, encoding='utf-8')
    print('applied; last id', num[defs[-1]])
    for d in defs:
        print(num[d], d)
