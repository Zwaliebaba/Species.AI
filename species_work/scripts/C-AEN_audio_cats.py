import csv, re, sys, collections
rows = list(csv.DictReader(open(sys.argv[1], encoding='utf-8')))
cat = collections.defaultdict(float)
grp = collections.defaultdict(float)
stereo_possible = 0.0
for r in rows:
    aid = r['asset_id']
    if not aid.startswith('sfx.') or 'Opus' in r['target_format']:
        continue
    m = re.search(r'([\d.]+) s reference length', r['spec'])
    s = float(m.group(1))
    parts = aid.split('.')
    cat['.'.join(parts[:2])] += s
    grp['.'.join(parts[:3])] += s
    if 'stereo' in r['spec']:
        stereo_possible += s
mib = lambda s: s * 48000 * 2 / 2**20
for k, v in sorted(cat.items()):
    print('%-14s %8.1f s %7.2f MiB' % (k, v, mib(v)))
print('UI may-be-stereo seconds %.1f -> extra %.2f MiB if stereo' % (stereo_possible, mib(stereo_possible)))
top = sorted(grp.items(), key=lambda x: -x[1])[:15]
for k, v in top:
    print('  %-40s %7.2f MiB' % (k, mib(v)))
