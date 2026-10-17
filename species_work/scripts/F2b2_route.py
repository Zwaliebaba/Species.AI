"""Resolve one ORIG file's routing definitions into rows of routing_F2b2.csv.

Usage: python3 -I F2b2_route.py <headings.csv> <orig_dir> <defs.txt> <routing.csv>

defs.txt: first non-comment line is `FILE <orig file name>`; then one row per
line, fields separated by ' | ':
    ref | action | target_file | target_section | owner_package | note
ref forms:
    L<a>-<b>         explicit line range
    H<n>:<prefix>    heading of level n whose text starts with <prefix>
    I<n>:<prefix>    intro lines of that heading (heading line up to the line
                     before its first child heading)
    TOP              line 1 up to the line before the first H2
    =                same range as the previous row (shared-range twin row)
The script replaces all rows for that file in routing.csv, then checks that
the union of ranges covers the H1 extent and that overlaps occur only between
rows with identical ranges.
"""
import csv, os, sys

ACTIONS = {'KEEP', 'MERGE', 'DROP-LEGACY', 'DROP-PROVENANCE', 'DEFER-DESIGN',
           'TO-DEC', 'TO-ADR', 'TO-ASSETS', 'KEEP-HOOKS'}
COLS = ['orig_file', 'anchor', 'start_line', 'end_line', 'bytes', 'action',
        'target_file', 'target_section', 'owner_package', 'note']


def main():
    hpath, odir, dpath, rpath = sys.argv[1:5]
    with open(dpath, encoding='utf-8') as f:
        lines = [l.rstrip('\n') for l in f if l.strip() and not l.startswith('#')]
    fname = lines[0].split(None, 1)[1].strip()
    with open(hpath, encoding='utf-8') as f:
        heads = [r for r in csv.DictReader(f) if r['file'] == fname]
    for h in heads:
        h['level'] = int(h['level']); h['start_line'] = int(h['start_line']); h['end_line'] = int(h['end_line'])
    with open(os.path.join(odir, fname), encoding='utf-8') as f:
        text = f.read().split('\n')
    h1 = [h for h in heads if h['level'] == 1][0]
    first_h2 = min(h['start_line'] for h in heads if h['level'] == 2)

    def find(level, prefix):
        c = [h for h in heads if h['level'] == level and h['heading'].startswith(prefix)]
        if len(c) != 1:
            sys.exit(f'{fname}: ref H{level}:{prefix!r} matched {len(c)} headings')
        return c[0]

    rows, prev = [], None
    for l in lines[1:]:
        parts = [p.strip() for p in l.split(' | ')]
        if len(parts) != 6:
            sys.exit(f'bad def line ({len(parts)} fields): {l}')
        ref, action, tfile, tsec, owner, note = parts
        if action not in ACTIONS:
            sys.exit(f'bad action {action}')
        if ref == '=':
            a, b, anchor = prev
        elif ref == 'TOP':
            a, b, anchor = 1, first_h2 - 1, h1['anchor']
        elif ref.startswith('L'):
            a, b = map(int, ref[1:].split('-')); anchor = ''
            for h in heads:
                if h['start_line'] == a:
                    anchor = h['anchor']
            if not anchor:
                inner = [h for h in heads if h['start_line'] <= a <= h['end_line']]
                anchor = max(inner, key=lambda h: h['level'])['anchor']
        elif ref[0] in 'HI':
            lvl, prefix = ref[1:].split(':', 1)
            h = find(int(lvl), prefix)
            a, b, anchor = h['start_line'], h['end_line'], h['anchor']
            if ref[0] == 'I':
                kids = [k['start_line'] for k in heads if k['level'] > h['level'] and a < k['start_line'] <= b]
                b = min(kids) - 1
                anchor = h['anchor']
        else:
            sys.exit(f'bad ref {ref}')
        prev = (a, b, anchor)
        size = sum(len(x.encode('utf-8')) + 1 for x in text[a - 1:b])
        if action.startswith('DROP'):
            if tfile or tsec or owner:
                sys.exit(f'DROP row must have empty target/owner: {l}')
        rows.append({'orig_file': fname, 'anchor': anchor, 'start_line': a, 'end_line': b,
                     'bytes': size, 'action': action, 'target_file': tfile,
                     'target_section': tsec, 'owner_package': owner, 'note': note})
    # coverage check
    lo, hi = h1['start_line'], h1['end_line']
    cover = [0] * (hi + 2)
    ranges = {}
    for r in rows:
        ranges.setdefault((r['start_line'], r['end_line']), 0)
    for (a, b) in ranges:
        for i in range(a, b + 1):
            cover[i] += 1
    gaps = [i for i in range(lo, hi + 1) if cover[i] == 0]
    over = [i for i in range(lo, hi + 1) if cover[i] > 1]
    if gaps or over:
        def runs(xs):
            out, s = [], None
            for i, x in enumerate(xs):
                if s is None: s = x
                if i == len(xs) - 1 or xs[i + 1] != x + 1:
                    out.append(f'{s}-{x}'); s = None
            return out
        sys.exit(f'{fname}: gaps {runs(gaps)[:20]} overlaps {runs(over)[:20]}')
    old = []
    if os.path.exists(rpath):
        with open(rpath, encoding='utf-8') as f:
            old = [r for r in csv.DictReader(f) if r['orig_file'] != fname]
    allrows = old + rows
    order = {}
    for r in allrows:
        order.setdefault(r['orig_file'], len(order))
    allrows.sort(key=lambda r: (r['orig_file'], int(r['start_line'])))
    with open(rpath, 'w', newline='', encoding='utf-8') as f:
        w = csv.DictWriter(f, fieldnames=COLS, lineterminator='\n')
        w.writeheader()
        for r in allrows:
            w.writerow(r)
    from collections import Counter
    c = Counter(r['action'] for r in rows)
    print(f'{fname}: {len(rows)} rows, coverage {lo}-{hi} OK; ' + ', '.join(f'{k} {v}' for k, v in sorted(c.items())))


if __name__ == '__main__':
    main()
