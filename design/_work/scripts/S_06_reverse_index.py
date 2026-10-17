#!/usr/bin/env python3
"""S06 helper: reverse index of the PC default bindings — for every physical input
(key / mouse button / mouse axis / XInput control) list every ControlType bound to it, with
its condition word, from drafts/P10_input_camera_bindings.csv (kbm_default, xinput_default
and other_default columns = the bindings that parse in data/input/input_preferences.txt).
Conjunction (&&) and chord (++) operands are indexed individually and marked.
Usage: python3 -I S_06_reverse_index.py <bindings.csv> <out.md>"""
import csv, re, sys, collections
CSV_PATH, OUT = sys.argv[1:3]
rows = list(csv.DictReader(open(CSV_PATH, encoding='utf-8')))
idx = collections.OrderedDict()


def add(dev, ctl, cond, kind):
    idx.setdefault(dev, []).append((ctl, cond, kind))


for r in rows:
    for col in ('kbm_default', 'xinput_default', 'other_default'):
        for spec in [p.strip() for p in r[col].split(' | ') if p.strip()]:
            fallback = '[=down' in spec
            spec = re.sub(r'\s*\[=down:[^\]]*\]', '', spec)
            if '++' in spec:
                parts, kind = [p.strip() + ' (chord)' for p in spec.split('++')], 'chord'
            elif '&&' in spec:
                parts, kind = [p.strip() for p in spec.split('&&')], 'and'
            else:
                parts, kind = [spec], ''
            for part in parts:
                chord = part.endswith(' (chord)')
                part = part.replace(' (chord)', '')
                t = part.split()
                if not t:
                    continue
                if t[0] in ('key', 'mouse', 'XInput') and len(t) >= 2:
                    dev = '%s %s' % (t[0], t[1].lower() if t[0] == 'key' else t[1])
                    cond = ' '.join(t[2:]) if len(t) > 2 else ('down/pressed' if chord else '')
                elif t[0] == 'alias':
                    dev, cond = 'alias %s' % t[1], ''
                elif t[0] == 'idle':
                    dev, cond = 'idle', ' '.join(t[1:])
                else:
                    dev, cond = t[0], ' '.join(t[1:])
                k = kind
                if fallback and t[0] == 'XInput':
                    cond += ' ⚑'
                add(dev, r['control'], cond, k)


def order(dev):
    d = dev.split()[0]
    return ({'key': 0, 'mouse': 1, 'XInput': 2}.get(d, 3), dev)


lines = ['| Physical input | Bound controls (condition; "+&&" = operand of a conjunction, "+chord" = operand of a chord) | Count |',
         '|---|---|---|']
for dev in sorted(idx, key=order):
    items = idx[dev]
    cells = []
    for ctl, cond, kind in items:
        tag = {'and': ' +&&', 'chord': ' +chord', '': ''}[kind]
        cells.append('`%s` (%s%s)' % (ctl, cond or '—', tag))
    lines.append('| `%s` | %s | %d |' % (dev, ', '.join(cells), len(items)))
open(OUT, 'w', encoding='utf-8').write('\n'.join(lines) + '\n')
print('physical inputs:', len(idx), 'entries:', sum(len(v) for v in idx.values()))
