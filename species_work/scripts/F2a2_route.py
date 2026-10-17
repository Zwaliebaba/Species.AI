"""Build WORK/routing_F2a2.csv from a compact routing spec (F2a2: ORIG 06-10).

Usage: python3 -I F2a2_route.py <headings.csv> <orig_dir> <out.csv> [--intros]

Selectors
  'H1'            the H1 line up to the line before the first H2 (title + preamble)
  'S:<heading>'   the section whose heading text starts with <heading> (first match
                  at the given level; level inferred: '1 '..'14 ' = H2, '3.10 ' = H3)
  'I:<n>'         intro of H2 <n>: from the H2 line to the line before its first H3
  'L:a-b'         an explicit line range (anchor = innermost enclosing heading)
Each spec row: (selector, action, owner, target_section, note); the target file is
derived from the owner package.
"""
import csv
import os
import sys

COLS = ['orig_file', 'anchor', 'start_line', 'end_line', 'bytes', 'action',
        'target_file', 'target_section', 'owner_package', 'note']

TARGET = {
    'G06': 'reference/06_controls_camera_and_commands.md',
    'G07': 'reference/07_bots.md',
    'G08': 'reference/08_maps_and_level_design.md',
    'G09': 'reference/09_front_end_and_match_flow.md',
    'G09a': 'reference/09_front_end_and_match_flow/screen_flow_and_match_setup.md',
    'G09b': 'reference/09_front_end_and_match_flow/options_localisation_and_hooks.md',
    'G10': 'reference/10_audio_design.md',
    'G11': 'reference/11_tutorial_and_help.md',
    'C-SIM': 'reference/12_simulation_model.md',
    'C-INP': 'architecture/components/input.md',
    'C-CAM': 'architecture/components/camera_and_sequences.md',
    'C-UI': 'architecture/components/ui_framework.md',
    'C-DBG': 'architecture/components/diagnostics_test_and_performance.md',
    'X-AST': 'assets/asset_manifest.csv',
    'X-DEC': 'decisions/decision_register.md',
    'X-ADR': 'adr/README.md',
}


def load_heads(headcsv, fname):
    return [r for r in csv.DictReader(open(headcsv, encoding='utf-8')) if r['file'] == fname]


def level_of(sel):
    num = sel.split(' ', 1)[0]
    return 2 + num.count('.') if num[0].isdigit() else 2


def resolve(sel, heads, nlines):
    if sel == 'H1':
        h1 = heads[0]
        first_h2 = next(int(h['start_line']) for h in heads if h['level'] == '2')
        return int(h1['start_line']), first_h2 - 1, h1['anchor']
    if sel.startswith('S:'):
        text = sel[2:]
        lvl = level_of(text)
        for h in heads:
            if int(h['level']) != lvl:
                continue
            if text[0].isdigit():
                hit = h['heading'].split(' ', 1)[0] == text
            else:
                hit = h['heading'].startswith(text)
            if hit:
                return int(h['start_line']), int(h['end_line']), h['anchor']
        raise SystemExit(f'no heading for {sel}')
    if sel.startswith('I:'):
        n = sel[2:]
        h2 = next(h for h in heads if h['level'] == '2' and h['heading'].startswith(n + ' '))
        s, e = int(h2['start_line']), int(h2['end_line'])
        h3 = next(h for h in heads if h['level'] == '3' and s < int(h['start_line']) <= e)
        return s, int(h3['start_line']) - 1, h2['anchor']
    if sel.startswith('L:'):
        a, b = (int(x) for x in sel[2:].split('-'))
        inner = None
        for h in heads:
            if int(h['start_line']) <= a <= int(h['end_line']):
                if inner is None or int(h['level']) >= int(inner['level']):
                    inner = h
        return a, b, inner['anchor']
    raise SystemExit(f'bad selector {sel}')


def build(headcsv, orig, fname, spec, writer, show_intros=False):
    heads = load_heads(headcsv, fname)
    raw = open(os.path.join(orig, fname), encoding='utf-8').read().split('\n')
    nlines = len(raw)
    spans = []
    for sel, action, owner, tsec, note in spec:
        s, e, anchor = resolve(sel, heads, nlines)
        b = len('\n'.join(raw[s - 1:e]).encode('utf-8')) + 1
        if action.startswith('DROP'):
            tfile, own = '', ''
        else:
            tfile, own = TARGET[owner], owner
        writer.writerow([fname, anchor, s, e, b, action, tfile, tsec, own, note])
        spans.append((s, e))
        if show_intros and sel.startswith('I:'):
            body = [l for l in raw[s:e] if l.strip()]
            print(f'  intro {fname} {sel} {s}-{e}: {len(body)} non-blank lines; '
                  f'{(body[0][:120] if body else "")}')
    # coverage check (same rule as the G1 gate)
    first = next(i for i, l in enumerate(raw, 1) if l.startswith('# '))
    last = nlines
    while last > first and not raw[last - 1].strip():
        last -= 1
    cursor, errs = first, []
    for s, e in sorted(set(spans)):
        if s > cursor:
            errs.append(f'gap {cursor}-{s - 1}')
        elif s < cursor:
            errs.append(f'overlap {s}-{e} (prev ends {cursor - 1})')
        cursor = max(cursor, e + 1)
    if cursor <= last:
        errs.append(f'gap {cursor}-{last}')
    print(f'{fname}: {len(spec)} rows; coverage ' + ('OK' if not errs else '; '.join(errs)))
    return not errs


def main():
    headcsv, orig, out = sys.argv[1:4]
    show = '--intros' in sys.argv
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import F2a2_spec  # noqa: E402  (spec lives next to this script)
    ok = True
    with open(out, 'w', encoding='utf-8', newline='') as f:
        w = csv.writer(f, lineterminator='\n')
        w.writerow(COLS)
        for fname, spec in F2a2_spec.SPEC:
            ok &= build(headcsv, orig, fname, spec, w, show)
            f.flush()
    sys.exit(0 if ok else 1)


if __name__ == '__main__':
    main()
