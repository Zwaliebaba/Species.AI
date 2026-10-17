"""G10: statistics over the live in-scope layers of the sound-event CSV.

Usage: python3 -I G10_stats.py <X2b_sound_events.csv>
"""
import csv
import re
import sys
from collections import Counter

rows = list(csv.DictReader(open(sys.argv[1], newline='', encoding='utf-8')))
layers = Counter()
vols = []
kinds = Counter()
fields = Counter()
mds = []
for r in rows:
    if r['in_data'] != 'yes' or r['fired_in_FULL'] != 'yes':
        continue
    bl = [b.strip() for b in r['blueprint'].split('||')]
    layers[len(bl)] += 1
    for b in bl:
        m = re.search(r' md(\S+)', b)
        mds.append(float(m.group(1)))
        for k in ('v', 'f', 'a', 's', 'r', 'ld'):
            mm = re.search(r' %s=(\S+(?: \S+\.\.\S+(?:->\S+\.\.\S+)?)?)' % k, b)
            if mm:
                spec = mm.group(1)
                if spec.startswith('rnd'):
                    t = 'random'
                elif re.match(r'[A-Za-z]', spec):
                    t = 'linked'
                else:
                    t = 'fixed'
                fields[(k, t)] += 1
                if '/once' in b.split(' %s=' % k)[1].split(' ')[0] or spec.endswith('/once'):
                    fields[(k, 'once')] += 1
                if k == 'v' and t == 'fixed':
                    vols.append(float(spec.replace('/once', '')))
print('layers per pair', dict(layers))
print('fields', sorted(fields.items()))
print('fixed vol n=%d min=%.2f max=%.2f mean=%.2f' % (len(vols), min(vols), max(vols), sum(vols) / len(vols)))
print('vol <8 count', sum(1 for v in vols if v < 8), '8..10', sum(1 for v in vols if v >= 8))
print('md min max', min(mds), max(mds), 'distinct', len(set(mds)), '>=450', sorted(m for m in mds if m >= 450))
