"""Build WORK/routing_F2a1.csv from per-file spec files and validate coverage.

Usage: python3 -I F2a1_build.py <WORK dir> <ORIG dir>

Spec files: <WORK>/scripts/F2a1_spec_NN.psv, one per ORIG document, lines
    start|end|action|target_file|target_section|owner_package|note
Blank lines and lines starting with '#' are ignored. The first non-comment
line is 'FILE|<orig file name>'.

The anchor is the deepest heading starting at `start`; otherwise the deepest
heading whose extent contains `start`. Bytes follow headings.py (UTF-8 size of
each line plus one newline, lines from split('\\n')).
"""
import csv
import re
import sys
from pathlib import Path

ACTIONS = {'KEEP', 'MERGE', 'DROP-LEGACY', 'DROP-PROVENANCE', 'DEFER-DESIGN',
           'TO-DEC', 'TO-ADR', 'TO-ASSETS', 'KEEP-HOOKS'}
SHARE_OK = {frozenset({'KEEP', 'TO-DEC'}), frozenset({'MERGE', 'TO-DEC'}),
            frozenset({'KEEP-HOOKS', 'TO-DEC'})}
PKG = re.compile(r'^(G0\d[a-z0-9]*|G1[01][a-z0-9]*|C-[A-Z]+[a-z0-9]*|X-AST|X-GLO|X-DEC|X-ADR)$')


def main():
    work, orig = Path(sys.argv[1]), Path(sys.argv[2])
    codes = set(re.findall(r'`((?:DL|DD|KH|DP)-[A-Z0-9-]+)`', (work / 'drop_topics.md').read_text('utf-8')))
    heads = {}
    with open(work / 'headings.csv', encoding='utf-8') as f:
        for r in csv.DictReader(f):
            heads.setdefault(r['file'], []).append(r)
    out_rows, errors = [], []
    for spec in sorted((work / 'scripts').glob('F2a1_spec_*.psv')):
        fname, rows = None, []
        for ln, raw in enumerate(spec.read_text('utf-8').split('\n'), 1):
            if not raw.strip() or raw.startswith('#'):
                continue
            parts = raw.split('|')
            if parts[0] == 'FILE':
                fname = parts[1].strip()
                continue
            if len(parts) != 7:
                errors.append(f'{spec.name}:{ln}: expected 7 fields, got {len(parts)}')
                continue
            rows.append((ln, [p.strip() for p in parts]))
        lines = (orig / fname).read_text('utf-8').split('\n')
        hs = heads[fname]
        h1 = [h for h in hs if h['level'] == '1'][0]
        lo, hi = int(h1['start_line']), int(h1['end_line'])
        cover = {}
        for ln, (a, b, act, tfile, tsec, pkg, note) in rows:
            a, b = int(a), int(b)
            if act not in ACTIONS:
                errors.append(f'{fname}:{a}: bad action {act}')
            if a > b or a < lo or b > hi:
                errors.append(f'{fname}:{a}-{b}: range outside {lo}-{hi}')
            starting = [h for h in hs if int(h['start_line']) == a]
            if starting:
                anchor = max(starting, key=lambda h: int(h['level']))['anchor']
            else:
                cont = [h for h in hs if int(h['start_line']) <= a <= int(h['end_line'])]
                anchor = max(cont, key=lambda h: int(h['level']))['anchor']
            size = sum(len(l.encode('utf-8')) + 1 for l in lines[a - 1:b])
            if act.startswith('DROP'):
                if pkg or tfile:
                    errors.append(f'{fname}:{a}: DROP row has package or target')
            elif act in ('TO-DEC', 'DEFER-DESIGN'):
                if pkg != 'X-DEC':
                    errors.append(f'{fname}:{a}: {act} needs X-DEC')
            elif act == 'TO-ADR':
                if pkg != 'X-ADR':
                    errors.append(f'{fname}:{a}: TO-ADR needs X-ADR')
            elif not PKG.match(pkg) or pkg in ('X-DEC', 'X-ADR'):
                errors.append(f'{fname}:{a}: bad package {pkg!r}')
            if act.startswith('DROP') or act in ('DEFER-DESIGN', 'KEEP-HOOKS'):
                m = re.match(r'((?:DL|DD|KH|DP)-[A-Z0-9]+(?:-[A-Z0-9]+)*)', note)
                code = m.group(1) if m else ''
                if code not in codes:
                    errors.append(f'{fname}:{a}: note must start with a reason code, got {code!r}')
                need = {'DROP-LEGACY': 'DL-', 'DROP-PROVENANCE': 'DP-', 'DEFER-DESIGN': 'DD-', 'KEEP-HOOKS': 'KH-'}[act]
                if not code.startswith(need):
                    errors.append(f'{fname}:{a}: {act} needs a {need} code, got {code!r}')
            for i in range(a, b + 1):
                cover.setdefault(i, []).append((a, b, act))
            out_rows.append([fname, anchor, a, b, size, act, tfile, tsec, pkg, note])
        for i in range(lo, hi + 1):
            c = cover.get(i, [])
            if not c:
                errors.append(f'{fname}:{i}: uncovered')
            elif len(c) > 1:
                rngs = {(x[0], x[1]) for x in c}
                acts = frozenset(x[2] for x in c)
                if len(rngs) != 1 or len(c) != 2 or acts not in SHARE_OK:
                    errors.append(f'{fname}:{i}: bad overlap {c}')
    with open(work / 'routing_F2a1.csv', 'w', newline='', encoding='utf-8') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(['orig_file', 'anchor', 'start_line', 'end_line', 'bytes', 'action',
                    'target_file', 'target_section', 'owner_package', 'note'])
        w.writerows(out_rows)
    # de-duplicate repeated uncovered-line errors into ranges
    print(f'rows={len(out_rows)} errors={len(errors)}')
    for e in errors[:60]:
        print('ERR', e)


if __name__ == '__main__':
    main()
