"""Check that requirement ids, DEC ids, ADR ids and backticked keys cited in given files exist in the design root.
Usage: python3 -I X-DEC-P4b_check.py <design_root> <file> [<file> ...]"""
import os, re, sys
root = sys.argv[1]
defs, keys, decs = set(), set(), set()
for d, _, fs in os.walk(root):
    for f in fs:
        if not f.endswith('.md'):
            continue
        t = open(os.path.join(d, f), encoding='utf-8').read()
        defs.update(re.findall(r'\*\*((?:R-)?[A-Z]{2,5}-\d{3})\*\*', t))
        decs.update(re.findall(r'^#{2,4} (DEC-\d{3})\s', t, re.M))
        for line in t.split('\n'):
            m = re.match(r'^\|\s*`([a-z][a-z0-9_]*(?:\.[a-z0-9_]+)+)`\s*\|', line)
            if m:
                keys.add(m.group(1))
adrs = {f[:4] for f in os.listdir(os.path.join(root, 'adr')) if re.match(r'\d{4}-', f)}
bad = 0
for p in sys.argv[2:]:
    t = open(p, encoding='utf-8').read()
    t2 = re.sub(r'((?:R-)?[A-Z]{2,5})-(\d{3})\s*(?:to|–)\s*(?:(?:R-)?[A-Z]{2,5}-)?(\d{3})',
                lambda m: f'{m.group(1)}-{m.group(2)} {m.group(1)}-{m.group(3)}', t)
    for rid in sorted(set(re.findall(r'\b((?:R-)?(?:VIS|MODE|UNIT|BLDG|CRATE|ECON|CTRL|BOT|MAP|META|AUD|TUT|SIM|TER|RND|AST|VFX|AEN|UI|INP|CAM|DAT|PLT|BLD|DBG)-\d{3})\b', t2))):
        if rid not in defs:
            print(f'{p}: requirement {rid} not defined'); bad += 1
    for did in sorted(set(re.findall(r'\b(DEC-\d{3})\b', t2))):
        if did not in decs:
            print(f'{p}: {did} not defined'); bad += 1
    for a in sorted(set(re.findall(r'ADR-(\d{4})', t))):
        if a not in adrs:
            print(f'{p}: ADR-{a} not found'); bad += 1
    for k in sorted(set(re.findall(r'`((?:ref|rnd|vfx|ui|inp|cam|ter|aen|dat|plt|bld|dbg|ast)\.[a-z0-9_.]+?)`', t))):
        if k.endswith('.') or k.endswith('*'):
            continue
        if k not in keys:
            print(f'{p}: key {k} not in any tuning table'); bad += 1
print('problems:', bad)
