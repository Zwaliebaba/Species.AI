"""Gate G2: merge the decision register and the ADR index, and check references.

Usage: python3 -I g2_merge.py <REPO>

- parses every DEC entry under design_next/decisions/ (heading "### DEC-NNN title",
  then a "- Status: X" line); checks ids are unique and inside the file's range;
- writes design_next/decisions/README.md (generated index);
- merges species_work/dec_map_*.csv into species_work/dec_map.csv;
- inserts species_work/adr_index_X-ADR-b.md rows at the README marker (once);
- checks that every DEC-NNN and ADR-NNNN cited anywhere in design_next exists.
Exit status 1 on any error.
"""
import csv
import glob
import os
import re
import sys

REPO = sys.argv[1]
NEXT = os.path.join(REPO, 'design_next')
WORK = os.path.join(REPO, 'species_work')
RANGES = {
    'verdicts_and_fixes.md': (1, 199), 'deferred/networking.md': (200, 219),
    'deferred/process_tick_and_roadmap.md': (220, 239),
    'deferred/numerics_lobby_terrain_services.md': (240, 259),
    'open_questions.md': (300, 449), 'units_buildings_crates_economy.md': (450, 549),
    'controls_bots_maps_meta_audio.md': (550, 649), 'simulation_terrain_render_assets.md': (650, 749),
    'vfx_audio_ui.md': (750, 799), 'data_sequences_platform_debug.md': (800, 849),
    'review_findings.md': (900, 949),
}
TITLES = {
    'verdicts_and_fixes.md': 'Verdicts, keep-list and fix-list', 'deferred/networking.md': 'Deferred: networking',
    'deferred/process_tick_and_roadmap.md': 'Deferred: process, tick rate and roadmap',
    'deferred/numerics_lobby_terrain_services.md': 'Deferred: numerics, lobby, terrain runtime, services, persistence',
    'open_questions.md': 'Open questions', 'units_buildings_crates_economy.md': 'Units, buildings, crates and population',
    'controls_bots_maps_meta_audio.md': 'Controls, bots, maps, front end and audio',
    'simulation_terrain_render_assets.md': 'Simulation, terrain, rendering and assets',
    'vfx_audio_ui.md': 'Animation and VFX, audio engine, UI', 'data_sequences_platform_debug.md':
    'Data, sequences, platform and diagnostics', 'review_findings.md': 'Review findings',
}
errors = []


def parse_decs():
    decs = []
    ddir = os.path.join(NEXT, 'decisions')
    for path in sorted(glob.glob(os.path.join(ddir, '*.md')) + glob.glob(os.path.join(ddir, 'deferred', '*.md'))):
        rel = os.path.relpath(path, ddir).replace(os.sep, '/')
        if rel == 'README.md':
            continue
        lines = open(path, encoding='utf-8').read().split('\n')
        lo, hi = RANGES.get(rel, (0, -1))
        if rel not in RANGES:
            errors.append(f'{rel}: no DEC range registered')
        for i, line in enumerate(lines):
            m = re.match(r'^#{2,4} DEC-(\d{3})\s+(.*)$', line)
            if not m:
                continue
            n, title = int(m.group(1)), m.group(2).strip()
            status = ''
            for nxt in lines[i + 1:i + 8]:
                s = re.match(r'^- \*{0,2}Status\*{0,2}:\*{0,2}\s*([A-Za-z]+)', nxt)
                if s:
                    status = s.group(1)
                    break
            if not (lo <= n <= hi):
                errors.append(f'{rel}: DEC-{n:03d} outside range {lo:03d}-{hi:03d}')
            if status not in ('Open', 'Resolved', 'Dropped'):
                errors.append(f'{rel}: DEC-{n:03d} has no valid status line')
            decs.append(dict(n=n, title=title, status=status, file=rel))
    seen = {}
    for d in decs:
        if d['n'] in seen:
            errors.append(f"DEC-{d['n']:03d} defined twice ({seen[d['n']]}, {d['file']})")
        seen[d['n']] = d['file']
    return sorted(decs, key=lambda d: d['n'])


def write_index(decs):
    from collections import Counter
    c = Counter(d['status'] for d in decs)
    out = ['# Decision register', '',
           '> Generated at gate G2 by the coordinator. Do not edit by hand: entries live in the files listed below.', '',
           'Every decision uses the format in [CONVENTIONS](../CONVENTIONS.md) §7. **Open** decisions state the behaviour the '
           'documents specify now (the reference default), so implementation never blocks on them. **Resolved** decisions '
           'state the fixed rule. **Dropped** entries record out-of-scope items. Decisions marked "Decide by: design phase" '
           'are answered by the Species design phase.', '',
           f"Totals: **{len(decs)}** decisions: {c['Open']} Open, {c['Resolved']} Resolved, {c['Dropped']} Dropped.", '',
           '## Register files', '', '| File | Scope | Id range | Entries |', '|---|---|---|---|']
    for rel, (lo, hi) in RANGES.items():
        n = sum(1 for d in decs if d['file'] == rel)
        if n:
            out.append(f'| [{rel}]({rel}) | {TITLES[rel]} | DEC-{lo:03d}–{hi:03d} | {n} |')
    out += ['', '## Index', '', '| Id | Title | Status | File |', '|---|---|---|---|']
    for d in decs:
        anchor = re.sub(r'[^\w\- ]', '', f"dec-{d['n']:03d} {d['title']}".lower()).replace(' ', '-')
        out.append(f"| DEC-{d['n']:03d} | [{d['title']}]({d['file']}#{anchor}) | {d['status']} | {d['file']} |")
    open(os.path.join(NEXT, 'decisions', 'README.md'), 'w', encoding='utf-8').write('\n'.join(out) + '\n')


def merge_maps():
    rows, cols = [], ['dec_id', 'title', 'status', 'affects_prefix', 'affects_files', 'orig_refs', 'source_map']
    for p in sorted(glob.glob(os.path.join(WORK, 'dec_map_*.csv'))):
        for r in csv.DictReader(open(p, encoding='utf-8', newline='')):
            r['source_map'] = os.path.basename(p)
            rows.append(r)
    with open(os.path.join(WORK, 'dec_map.csv'), 'w', encoding='utf-8', newline='') as f:
        w = csv.DictWriter(f, fieldnames=cols, extrasaction='ignore', lineterminator='\n')
        w.writeheader()
        w.writerows(rows)
    return len(rows)


def merge_adr_index():
    readme = os.path.join(NEXT, 'adr', 'README.md')
    t = open(readme, encoding='utf-8').read()
    marker = '<!-- X-ADR-b rows -->'
    if marker not in t:
        return 'already merged'
    src = open(os.path.join(WORK, 'adr_index_X-ADR-b.md'), encoding='utf-8').read().split('\n')
    rows = [l for l in src if l.startswith('|') and not re.match(r'^\|\s*(Number|---)', l)]
    t = t.replace(marker, '\n'.join(rows))
    open(readme, 'w', encoding='utf-8').write(t)
    return f'{len(rows)} rows inserted'


def check_refs(decs):
    dec_ids = {d['n'] for d in decs}
    adr_ids = {int(os.path.basename(p)[:4]) for p in glob.glob(os.path.join(NEXT, 'adr', '[0-9][0-9][0-9][0-9]-*.md'))}
    for path in glob.glob(os.path.join(NEXT, '**', '*.md'), recursive=True):
        rel = os.path.relpath(path, NEXT)
        text = open(path, encoding='utf-8').read()
        # id ranges such as 'DEC-450 to DEC-549' describe a file's block, not a citation
        text = re.sub(r'(DEC|ADR)-\d{3,4}\s*(?:to|–|-|…)\s*(?:DEC-|ADR-)?\d{3,4}', ' ', text)
        for m in set(re.findall(r'DEC-(\d{3})\b', text)):
            if int(m) not in dec_ids:
                errors.append(f'{rel}: cites DEC-{m}, which does not exist')
        for m in set(re.findall(r'ADR-(\d{4})\b', text)):
            if int(m) not in adr_ids:
                errors.append(f'{rel}: cites ADR-{m}, which does not exist')
    return len(adr_ids)


def main():
    decs = parse_decs()
    write_index(decs)
    n_map = merge_maps()
    adr_msg = merge_adr_index()
    n_adr = check_refs(decs)
    print(f'DECs: {len(decs)}; dec_map rows: {n_map}; ADR files: {n_adr}; ADR index: {adr_msg}')
    for e in errors:
        print('ERROR', e)
    print(f'errors={len(errors)}')
    sys.exit(1 if errors else 0)


if __name__ == '__main__':
    main()
