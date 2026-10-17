"""Gate G1 routing check: merge routing_*.csv, validate, and size the packages.

Usage: python3 -I check_routing.py <WORK dir> <ORIG dir>

Checks
- columns, actions, reason-class codes (from drop_topics.md) and owner packages;
- coverage: per file, routed ranges cover every line from the H1 to EOF;
  ranges overlap only when identical (deliberate KEEP + TO-DEC pairs);
- KEEP / MERGE / KEEP-HOOKS rows have a target_file and an owner_package.
Writes WORK/routing.csv (merged) and WORK/package_budget.csv, prints a report.
Exit status 1 if any check fails.
"""
import csv
import glob
import os
import re
import sys
from collections import defaultdict

COLS = ['orig_file', 'anchor', 'start_line', 'end_line', 'bytes', 'action',
        'target_file', 'target_section', 'owner_package', 'note']
ACTIONS = {'KEEP', 'MERGE', 'DROP-LEGACY', 'DROP-PROVENANCE', 'DEFER-DESIGN',
           'TO-DEC', 'TO-ADR', 'TO-ASSETS', 'KEEP-HOOKS'}
NEEDS_TARGET = {'KEEP', 'MERGE', 'KEEP-HOOKS'}
NEEDS_CODE = {'DROP-LEGACY': 'DL-', 'DROP-PROVENANCE': 'DP-',
              'DEFER-DESIGN': 'DD-', 'KEEP-HOOKS': 'KH-'}
PACKAGES = {'G00', 'G01', 'G02a', 'G02b', 'G03a', 'G03b', 'G04', 'G05', 'G06',
            'G07', 'G08', 'G09', 'G10', 'G11', 'C-SIM', 'C-TER', 'C-RND',
            'C-AST', 'C-VFX', 'C-AEN', 'C-UI', 'C-INP', 'C-CAM', 'C-DAT',
            'C-PLT', 'C-BLD', 'C-DBG', 'X-GLO', 'X-AST', 'X-DEC', 'X-ADR'}
BUDGET_KB = 150


def codes_from(drop_topics):
    text = open(drop_topics, encoding='utf-8').read()
    return set(re.findall(r'`((?:DL|DD|KH|DP)-[A-Z0-9-]+)`', text))


def main():
    work, orig = sys.argv[1], sys.argv[2]
    codes = codes_from(os.path.join(work, 'drop_topics.md'))
    errors, rows = [], []
    for path in sorted(glob.glob(os.path.join(work, 'routing_*.csv'))):
        with open(path, encoding='utf-8', newline='') as f:
            r = csv.DictReader(f)
            if r.fieldnames != COLS:
                errors.append(f'{os.path.basename(path)}: columns {r.fieldnames}')
                continue
            for n, row in enumerate(r, start=2):
                row['_src'] = f'{os.path.basename(path)}:{n}'
                rows.append(row)

    for row in rows:
        src, act = row['_src'], row['action'].strip()
        if act not in ACTIONS:
            errors.append(f'{src}: unknown action {act!r}')
        try:
            row['_s'], row['_e'] = int(row['start_line']), int(row['end_line'])
            if row['_s'] > row['_e']:
                errors.append(f'{src}: start > end')
        except ValueError:
            errors.append(f'{src}: bad line numbers')
            row['_s'] = row['_e'] = 0
        if act in NEEDS_CODE:
            m = re.match(r'((?:DL|DD|KH|DP)-[A-Z0-9-]+)', row['note'].strip())
            if not m or not m.group(1).startswith(NEEDS_CODE[act]):
                errors.append(f'{src}: {act} note must start with a {NEEDS_CODE[act]} code')
            elif m.group(1) not in codes:
                errors.append(f'{src}: unknown reason code {m.group(1)}')
        if act in NEEDS_TARGET and not row['target_file'].strip():
            errors.append(f'{src}: {act} without target_file')
        pkg = row['owner_package'].strip()
        if act.startswith('DROP'):
            continue
        if not pkg:
            errors.append(f'{src}: {act} without owner_package')
        elif pkg not in PACKAGES and not re.match(r'^(G\d\d[a-z]?\d?|C-[A-Z]+\d?)$', pkg):
            errors.append(f'{src}: unknown owner_package {pkg!r}')

    # coverage per file
    by_file = defaultdict(list)
    for row in rows:
        by_file[row['orig_file'].strip()].append(row)
    expected = sorted(os.path.basename(p) for p in
                      glob.glob(os.path.join(orig, '[0-4]*.md')) + [os.path.join(orig, '90_glossary.md')])
    for f in expected:
        if f not in by_file:
            errors.append(f'{f}: not routed at all')
    for f, frows in sorted(by_file.items()):
        path = os.path.join(orig, f)
        if not os.path.exists(path):
            errors.append(f'{f}: no such ORIG file')
            continue
        lines = open(path, encoding='utf-8').read().split('\n')
        first = next((i for i, l in enumerate(lines, 1) if l.startswith('# ')), 1)
        last = len(lines)
        while last > first and not lines[last - 1].strip():
            last -= 1
        spans = sorted({(r['_s'], r['_e']) for r in frows})
        cursor = first
        for s, e in spans:
            if s > cursor:
                errors.append(f'{f}: lines {cursor}-{s - 1} not routed')
            elif s < cursor:
                errors.append(f'{f}: range {s}-{e} overlaps the previous range (ends {cursor - 1})')
            cursor = max(cursor, e + 1)
        if cursor <= last:
            errors.append(f'{f}: lines {cursor}-{last} not routed')

    # merged output and package budgets
    with open(os.path.join(work, 'routing.csv'), 'w', encoding='utf-8', newline='') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(COLS)
        for row in sorted(rows, key=lambda r: (r['orig_file'], r['_s'], r['action'])):
            w.writerow([row[c] for c in COLS])
    budget = defaultdict(int)
    for row in rows:
        if row['action'] in ('KEEP', 'MERGE', 'KEEP-HOOKS', 'TO-DEC', 'TO-ADR',
                             'TO-ASSETS', 'DEFER-DESIGN'):
            try:
                budget[(row['owner_package'].strip(), row['action'])] += int(row['bytes'])
            except ValueError:
                pass
    pk = defaultdict(int)
    for (p, a), b in budget.items():
        if a in ('KEEP', 'MERGE', 'KEEP-HOOKS'):
            pk[p] += b
    with open(os.path.join(work, 'package_budget.csv'), 'w', encoding='utf-8', newline='') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(['owner_package', 'keep_merge_hooks_kb', 'over_budget'])
        for p, b in sorted(pk.items()):
            w.writerow([p, round(b / 1024, 1), 'yes' if b > BUDGET_KB * 1024 else ''])

    acts = defaultdict(int)
    for row in rows:
        acts[row['action']] += 1
    print(f'rows={len(rows)} files={len(by_file)}')
    print('actions: ' + ', '.join(f'{a}={n}' for a, n in sorted(acts.items())))
    print('packages over budget: ' + (', '.join(f'{p} ({b // 1024} KB)' for p, b in sorted(pk.items())
                                               if b > BUDGET_KB * 1024) or 'none'))
    for e in errors[:200]:
        print('ERROR', e)
    print(f'errors={len(errors)}')
    sys.exit(1 if errors else 0)


if __name__ == '__main__':
    main()
