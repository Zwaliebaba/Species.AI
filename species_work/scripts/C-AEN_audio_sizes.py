import csv, re, sys, collections
rows = list(csv.DictReader(open(sys.argv[1], encoding='utf-8')))
tot = collections.defaultdict(lambda: [0, 0.0, 0.0])  # files, seconds, max
groups = collections.defaultdict(set)
longest = []
for r in rows:
    aid = r['asset_id']
    if not re.match(r'^(sfx|amb|music)\.', aid):
        continue
    m = re.search(r'([\d.]+) s reference length', r['spec'])
    secs = float(m.group(1)) if m else None
    fmt = r['target_format']
    key = ('opus' if 'Opus' in fmt else 'pcm', aid.split('.')[0])
    t = tot[key]
    t[0] += 1
    if secs is not None:
        t[1] += secs
        t[2] = max(t[2], secs)
        longest.append((secs, aid, fmt[:40], r['spec'][:40]))
    else:
        print('no length:', aid, r['spec'][:120], fmt[:60])
    groups[key].add(aid)
for k, (n, s, mx) in sorted(tot.items()):
    mib = s * 48000 * 2 / 2**20
    print(k, 'files', n, 'groups', len(groups[k]), 'seconds %.1f' % s, 'mono16@48k MiB %.1f' % mib, 'max %.2f' % mx)
longest.sort(reverse=True)
for x in longest[:12]:
    print(x)
pcm = [x for x in longest if 'Opus' not in x[2]]
print('longest resident:', pcm[:5])
opus = [x for x in longest if 'Opus' in x[2]]
print('shortest opus:', sorted(opus)[:5])
