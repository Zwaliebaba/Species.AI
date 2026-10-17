"""Build WORK/routing_F2b1.csv from the routing spec in F2b1_spec.py-style data (embedded below via exec of a spec file).

Usage: python3 -I F2b1_build.py <repo_root> <spec.py> <out.csv>

The spec file defines SPEC = [(file_prefix, selector, action, target_file, target_section, owner, note), ...].
Selectors: 'title' (line 1 to the line before the first H2); a section number ('10.2') or a heading-text
prefix ('Open questions'); 'intro:<number>' (heading line to the line before its first child heading);
or (start, end) for an explicit line range.
Checks: every line of each routed file is covered; overlaps only between rows with identical ranges.
"""
import csv
import os
import sys

ACTIONS = {'KEEP', 'MERGE', 'DROP-LEGACY', 'DROP-PROVENANCE', 'DEFER-DESIGN', 'TO-DEC', 'TO-ADR',
           'TO-ASSETS', 'KEEP-HOOKS'}

root, spec_path, out = sys.argv[1], sys.argv[2], sys.argv[3]
ns = {}
with open(spec_path, encoding='utf-8') as f:
    exec(compile(f.read(), spec_path, 'exec'), ns)
SPEC = ns['SPEC']

heads = {}
for r in csv.DictReader(open(os.path.join(root, 'species_work', 'headings.csv'), encoding='utf-8')):
    heads.setdefault(r['file'], []).append(r)

files = {}
for name in os.listdir(os.path.join(root, 'design')):
    if name[:2].isdigit() and name.endswith('.md'):
        files[name[:2]] = name

lines_cache = {}


def lines_of(fname):
    if fname not in lines_cache:
        with open(os.path.join(root, 'design', fname), encoding='utf-8') as f:
            lines_cache[fname] = f.read().split('\n')
    return lines_cache[fname]


def find(fname, sel):
    hs = heads[fname]
    for h in hs:
        if int(h['level']) < 2:
            continue
        first = h['heading'].split()[0] if h['heading'].split() else ''
        if first == sel or (not sel[0].isdigit() and h['heading'].startswith(sel)):
            return h
    raise SystemExit(f'no heading {sel!r} in {fname}')


def resolve(fname, sel):
    hs = heads[fname]
    if isinstance(sel, tuple):
        return sel
    if sel == 'title':
        first_h2 = min(int(h['start_line']) for h in hs if h['level'] == '2')
        return (1, first_h2 - 1)
    if sel.startswith('intro:'):
        h = find(fname, sel[6:])
        s, e, lvl = int(h['start_line']), int(h['end_line']), int(h['level'])
        kids = [int(k['start_line']) for k in hs if int(k['level']) > lvl and s < int(k['start_line']) <= e]
        return (s, min(kids) - 1)
    h = find(fname, sel)
    return (int(h['start_line']), int(h['end_line']))


rows = []
problems = []
for (fp, sel, action, tfile, tsec, owner, note) in SPEC:
    fname = files[fp]
    s, e = resolve(fname, sel)
    ls = lines_of(fname)
    size = sum(len(l.encode('utf-8')) + 1 for l in ls[s - 1:e])
    if action not in ACTIONS:
        problems.append(f'bad action {action} {fp} {sel}')
    if action.startswith('DROP') and (tfile or owner):
        problems.append(f'DROP row with target/owner {fp} {sel}')
    if action.startswith(('DROP', 'DEFER', 'KEEP-HOOKS')) and not note.split(' ')[0].startswith(('DL-', 'DP-', 'DD-', 'KH-')):
        problems.append(f'note lacks reason code {fp} {sel}')
    rows.append([fname, '' if isinstance(sel, tuple) else sel, s, e, size, action, tfile, tsec, owner, note])

# coverage
for fp in sorted({r[0] for r in rows}):
    n = len(lines_of(fp))
    cov = {}
    ranges = {}
    for r in rows:
        if r[0] != fp:
            continue
        key = (r[2], r[3])
        ranges.setdefault(key, 0)
        ranges[key] += 1
    for (s, e) in ranges:
        for i in range(s, e + 1):
            cov.setdefault(i, []).append((s, e))
    gaps = [i for i in range(1, n + 1) if i not in cov]
    over = sorted({tuple(v) for i, v in cov.items() if len(v) > 1})
    if gaps:
        # compress
        segs, start, prev = [], gaps[0], gaps[0]
        for g in gaps[1:]:
            if g != prev + 1:
                segs.append((start, prev)); start = g
            prev = g
        segs.append((start, prev))
        problems.append(f'{fp}: gaps {segs}')
    if over:
        problems.append(f'{fp}: overlaps {over[:10]}')
    beyond = [k for k in ranges if k[1] > n]
    if beyond:
        problems.append(f'{fp}: beyond end {beyond}')

with open(out, 'w', newline='', encoding='utf-8') as f:
    w = csv.writer(f, lineterminator='\n')
    w.writerow(['orig_file', 'anchor', 'start_line', 'end_line', 'bytes', 'action', 'target_file',
                'target_section', 'owner_package', 'note'])
    w.writerows(rows)

# anchors: replace selector with the real anchor
anchors = {}
for fname, hs in heads.items():
    for h in hs:
        anchors[(fname, int(h['start_line']))] = h['anchor']
with open(out, encoding='utf-8') as f:
    data = list(csv.reader(f))
for r in data[1:]:
    s = int(r[2])
    a = anchors.get((r[0], s))
    if r[1] == 'title':
        r[1] = anchors[(r[0], 1)]
    elif r[1].startswith('intro:'):
        r[1] = (a or '') + ' (intro)'
    elif r[1] == '':
        # explicit range: nearest enclosing heading anchor + line note
        cands = [h for h in heads[r[0]] if int(h['start_line']) <= s <= int(h['end_line'])]
        best = max(cands, key=lambda h: int(h['start_line']))
        r[1] = best['anchor'] + (f' (lines {r[2]}-{r[3]})' if int(best['start_line']) != s or int(best['end_line']) != int(r[3]) else '')
    else:
        r[1] = a
with open(out, 'w', newline='', encoding='utf-8') as f:
    csv.writer(f, lineterminator='\n').writerows(data)

print(f'{len(rows)} rows -> {out}')
for p in problems:
    print('PROBLEM', p)
