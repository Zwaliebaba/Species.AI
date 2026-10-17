"""P16: parse data/sounds.txt structure into a per-event CSV and summary stats.
Usage: python3 -I P16_parse_sounds.py <sounds.txt> <out_events.csv> <out_summary.txt>
Data is treated as data only."""
import sys, csv, collections

def main(src, out_csv, out_sum):
    lines = open(src, encoding='latin-1').read().splitlines()
    rows = []
    groups = collections.OrderedDict()
    sample_groups = {}
    i = 0
    cur_group = cur_type = None
    ev = None
    in_effect = None
    def tok(l):
        return l.split()
    while i < len(lines):
        t = tok(lines[i]); i += 1
        if not t or t[0].startswith('#'):
            continue
        k = t[0].upper()
        if cur_group is None:
            if k in ('ENTITY', 'BUILDING', 'OTHER'):
                cur_group, cur_type = k, t[1]
                groups.setdefault((k, t[1]), [])
            elif k == 'SAMPLEGROUP':
                name = t[1]; samples = []
                while i < len(lines):
                    tt = tok(lines[i]); i += 1
                    if not tt: continue
                    if tt[0].upper() == 'END': break
                    samples.append(tt[1] if len(tt) > 1 else '')
                sample_groups[name] = samples
            continue
        # inside a source block
        if ev is None:
            if k == 'END':
                cur_group = cur_type = None
            elif k == 'EVENT':
                ev = {'group': cur_group, 'type': cur_type, 'event': t[1], 'effects': [], 'params': {}}
            continue
        if in_effect is not None:
            if k == 'END':
                ev['effects'].append(in_effect); in_effect = None
            else:
                in_effect[1].append(t)
            continue
        if k == 'END':
            rows.append(ev); groups[(cur_group, cur_type)].append(ev); ev = None
        elif k == 'EFFECT':
            in_effect = (t[1], [])
        elif k in ('VOLUME', 'FREQUENCY', 'ATTACK', 'SUSTAIN', 'RELEASE', 'LOOPDELAY'):
            ev['params'][k] = t[2:]
        else:
            ev[k] = ' '.join(t[1:])
    # write CSV
    with open(out_csv, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['group', 'object_type', 'event', 'SOUNDNAME', 'SOURCETYPE', 'POSITIONTYPE', 'INSTANCETYPE',
                    'LOOPTYPE', 'TEAMMATCHTYPE', 'MINDISTANCE', 'VOLUME', 'FREQUENCY', 'ATTACK', 'SUSTAIN',
                    'RELEASE', 'LOOPDELAY', 'EFFECTS', 'samplegroup_size'])
        for r in rows:
            p = r['params']
            sg = ''
            if r.get('SOURCETYPE') == '1':
                sgl = {k.lower(): v for k, v in sample_groups.items()}
                sg = str(len(sgl[r.get('SOUNDNAME', '').lower()])) if r.get('SOUNDNAME', '').lower() in sgl else 'MISSING'
            w.writerow([r['group'], r['type'], r['event'], r.get('SOUNDNAME', ''), r.get('SOURCETYPE', ''),
                        r.get('POSITIONTYPE', ''), r.get('INSTANCETYPE', ''), r.get('LOOPTYPE', ''),
                        r.get('TEAMMATCHTYPE', ''), r.get('MINDISTANCE', ''),
                        ' '.join(p.get('VOLUME', [])), ' '.join(p.get('FREQUENCY', [])), ' '.join(p.get('ATTACK', [])),
                        ' '.join(p.get('SUSTAIN', [])), ' '.join(p.get('RELEASE', [])), ' '.join(p.get('LOOPDELAY', [])),
                        ';'.join(e[0] for e in r['effects']), sg])
    # summary
    out = []
    cnt = collections.Counter(g for (g, _t) in groups)
    out.append('sources per group: %s' % dict(cnt))
    out.append('events per group: %s' % dict(collections.Counter(r['group'] for r in rows)))
    out.append('total events: %d' % len(rows))
    out.append('sample groups: %d, total sample refs: %d' % (len(sample_groups), sum(len(v) for v in sample_groups.values())))
    for fld in ('SOURCETYPE', 'POSITIONTYPE', 'INSTANCETYPE', 'LOOPTYPE', 'TEAMMATCHTYPE', 'PRIORITY'):
        out.append('%s: %s' % (fld, dict(collections.Counter(r.get(fld, '<absent>') for r in rows))))
    md = [float(r['MINDISTANCE']) for r in rows if 'MINDISTANCE' in r]
    out.append('MINDISTANCE min %.2f max %.2f distinct %s' % (min(md), max(md), sorted(set(md))))
    for prm in ('VOLUME', 'FREQUENCY', 'ATTACK', 'SUSTAIN', 'RELEASE', 'LOOPDELAY'):
        c = collections.Counter()
        upd = collections.Counter(); links = collections.Counter(); smooth = collections.Counter()
        vals = []
        for r in rows:
            v = r['params'].get(prm)
            if v is None:
                c['<absent>'] += 1; continue
            c[v[0]] += 1
            upd[v[-1]] += 1
            if v[0] == 'TypeLinked':
                links[v[6]] += 1; smooth[v[5]] += 1
            if v[0] == 'TypeRangedRandom':
                smooth[v[3]] += 1
            try:
                if v[0] == 'TypeFixedValue': vals.append(float(v[1]))
                elif v[0] == 'TypeRangedRandom': vals += [float(v[1]), float(v[2])]
                elif v[0] == 'TypeLinked': vals += [float(v[2]), float(v[4])]
            except Exception:
                pass
        out.append('%s types: %s | update: %s | links: %s | smooth: %s | output range: %s..%s' % (
            prm, dict(c), dict(upd), dict(links), dict(smooth), min(vals) if vals else '-', max(vals) if vals else '-'))
    eff = collections.Counter(e[0] for r in rows for e in r['effects'])
    out.append('EFFECT usage: %s' % dict(eff))
    out.append('events with effects: %s' % [(r['type'], r['event'], [e[0] for e in r['effects']]) for r in rows if r['effects']])
    out.append('linked params detail: %s' % [(r['type'], r['event'], k, v) for r in rows for k, v in r['params'].items() if v[0] == 'TypeLinked'])
    out.append('loopdelay detail: %s' % [(r['type'], r['event'], r['params']['LOOPDELAY']) for r in rows if 'LOOPDELAY' in r['params']])
    out.append('sustain detail: %s' % [(r['type'], r['event'], r['params']['SUSTAIN']) for r in rows if 'SUSTAIN' in r['params']])
    out.append('attack detail: %s' % [(r['type'], r['event'], r['params']['ATTACK']) for r in rows if 'ATTACK' in r['params']])
    out.append('looptype2 detail: %s' % [(r['type'], r['event']) for r in rows if r.get('LOOPTYPE') == '2'])
    out.append('teammatch!=0: %s' % [(r['type'], r['event'], r.get('TEAMMATCHTYPE')) for r in rows if r.get('TEAMMATCHTYPE') not in ('0', None)])
    out.append('instancetype!=0: %s' % [(r['type'], r['event'], r.get('INSTANCETYPE')) for r in rows if r.get('INSTANCETYPE') not in ('0', None)])
    out.append('positiontype 0/3: %s' % [(r['group'], r['type'], r['event'], r.get('POSITIONTYPE')) for r in rows if r.get('POSITIONTYPE') in ('0', '3')])
    lg = set(k.lower() for k in sample_groups)
    out.append('missing sample groups (case-insensitive): %s' % [(r['type'], r['event'], r.get('SOUNDNAME')) for r in rows if r.get('SOURCETYPE') == '1' and r.get('SOUNDNAME','').lower() not in lg])
    used_groups = set(r.get('SOUNDNAME','').lower() for r in rows if r.get('SOURCETYPE') == '1')
    out.append('unused sample groups (case-insensitive): %s' % sorted(k for k in sample_groups if k.lower() not in used_groups))
    out.append('sample group sizes: min %d max %d' % (min(len(v) for v in sample_groups.values()), max(len(v) for v in sample_groups.values())))
    out.append('duplicate events within a source: %s' % [(g, t, e) for (g, t), evs in groups.items() for e, n in collections.Counter(x['event'] for x in evs).items() if n > 1])
    out.append('sources: %s' % [(g, t, len(evs)) for (g, t), evs in groups.items()])
    open(out_sum, 'w').write('\n'.join(out) + '\n')

if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2], sys.argv[3])
