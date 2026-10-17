#!/usr/bin/env python3
"""S_41: normalise and de-duplicate the docs-vs-code discrepancy rows of every final document
(_work/closing/closing_discrepancies.md). Groups rows by source artefact and claim similarity.
Usage: python3 -I S_41_discrepancies.py <CLOSING_DIR> <OUT_TSV>   (also importable: groups(closing_dir))"""
import os, re, sys, difflib, importlib.util
here = os.path.dirname(os.path.abspath(__file__))
spec = importlib.util.spec_from_file_location('pc', os.path.join(here, 'S_41_parse_closing.py'))
pc = importlib.util.module_from_spec(spec); spec.loader.exec_module(pc)

PATH = re.compile(r'`?((?:docs|code|data|tools|contrib|targets)/[^`|,;]+?|README\.TXT|MultiwiniaSupport\.ini|_work/build_matrix\.md)`?(?=[\s`|,;(]|$)')

TICK = re.compile(r'`((?:docs|code|data|tools|contrib|targets)/[^`]+)`')

def norm_source(cell, header0):
    m = TICK.search(cell)
    if m:
        return m.group(1).split(':')[0].strip() if m.group(1).startswith('code/') and '::' in m.group(1) else m.group(1).strip()
    m = re.search(r'"([^"]+\.txt)" in `docs/`', cell)
    if m:
        return 'docs/' + m.group(1)
    m = PATH.search(cell)
    if m:
        return m.group(1).strip().rstrip(')').strip()
    m = PATH.search(header0)
    if m:
        return m.group(1).strip()
    return re.sub(r'[`*]', '', cell).strip()

def rows(closing_dir):
    out = []
    prev = ''
    for r in pc.generic_rows(closing_dir, 'discrepancies'):
        c, h = r['cells'], r['head']
        if r['doc'] == '01':          # Doc statement | Code behaviour | Ref
            src = 'docs/multiwinia_manual.pdf' if 'manual' in c[0] or 'manual' in h[0] else norm_source(c[0], h[0])
            if 'english.txt' in c[0]: src = 'data/language/english.txt'
            if 'Press ESC' in c[0]: src = 'UI text "Press ESC to Exit"'
            claim, code, ref = c[0], c[1], c[2] if len(c) > 2 else ''
        elif r['doc'] == '00':
            src, claim, code, ref = norm_source(c[0], ''), c[1], c[2], c[3]
        elif len(c) == 3:
            src, claim, code, ref = norm_source(c[0], ''), c[1], c[2], ''
        else:
            src, claim, code, ref = norm_source(c[0], ''), c[1], c[2], c[3]
        if src.lower() == 'same': src = prev
        prev = src
        out.append(dict(doc=r['doc'], source=src, source_cell=c[0], claim=claim, code=code, ref=ref))
    return out

T1, T2 = 0.55, 0.62
FORCE = {}

def key(s):
    return re.sub(r'[^a-z0-9 ]', ' ', s.lower())

def groups(closing_dir):
    rs = rows(closing_dir)
    gs = []
    for r in rs:
        best = None
        for g in gs:
            g0 = g[0]
            same_src = g0['source'].lower() == r['source'].lower()
            ratio = difflib.SequenceMatcher(None, key(g0['claim'])[:160], key(r['claim'])[:160]).ratio()
            ratio2 = difflib.SequenceMatcher(None, key(g0['code'])[:160], key(r['code'])[:160]).ratio()
            if same_src and (ratio > T1 or ratio2 > T2) or (g0['claim'], g0['code']) in FORCE.get((r['claim'], r['code']), ()):
                best = g; break
        if best is None: gs.append([r])
        else: best.append(r)
    return gs

if __name__ == '__main__':
    gs = groups(sys.argv[1])
    with open(sys.argv[2], 'w', encoding='utf-8') as fh:
        for i, g in enumerate(gs):
            for r in g:
                fh.write(f"G{i:03d}\t{r['doc']}\t{r['source']}\t{r['claim'][:110]}\t{r['code'][:80]}\n")
    print(len(sum(gs, [])), 'rows ->', len(gs), 'groups;', sum(1 for g in gs if len(g) > 1), 'multi-row groups')
