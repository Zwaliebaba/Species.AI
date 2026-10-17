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
# manual curation: rows are addressed by (doc, substring of claim)
MERGE = [
    [('00', 'Ticks at 25 FPS'), ('20', '"SERVER: Ticks at 25 FPS"'), ('21', 'the server sleeps until'), ('22', 'the server ticks at **25 FPS**'), ('31', 'server ticks at 25 FPS'), ('32', 'Ticks at 25 FPS')],
    [('20', 'main-thread affinity'), ('20', 'worker affinity'), ('31', 'affinity')],
    [('09', 'only the Speccy font'), ('25', 'only Speccy has Cyrillic'), ('28', 'only speccy had Cyrillic'), ('31', 'only Speccy has Cyrillic')],
    [('02', 'RATE = "secs to reload"'), ('29', '`SPED` in m/s')],
    [('06', 'grammar lists no'), ('29', 'grammar without')],
    [('07', 'with an AI running locally'), ('22', '`ServerToClient` index')],
    [('23', 'Landscape has `GenerateLightMap`'), ('24', '`Landscape::GenerateLightMap'), ('25', '`Landscape::ApplyLightmap`')],
    [('24', 'lists `GL_TEXTURE_MAG_FILTER` twice'), ('25', 'MAG/MIN filters')],
    [('04', 'events Squad'), ('10', '`CratePowerup` event list')],
    [('04', 'suggest a bad effect'), ('10', 'new crate event `Expired`')],
    [('00', 'gradually floats away'), ('02', 'gradually float away'), ('05', 'gradually floats away')],
    [('02', 'creatures such as Triffids'), ],
]
SPLIT = [('01', 'Co-op "(4 players required)', 'coop'), ('08', 'co-op "(4 players required)', 'coop'),
         ('01', 'CTS: a shattered statue', 'statue'), ('03', 'a shattered statue', 'statue'),
         ('20', 'CLIENT: Runs heavy weight', 'thread'), ('21', 'runs as a separate thread', 'thread')]
NOTES = {
    ('09', 'only the Speccy font'): 'Resolved here: drafts disagreed ("distinct atlases" in 25 vs "byte-identical" in 28); MD5 of `data/textures/unicode/{editor,speccy,square}.0410.042f.bmp` and `.0430.044f.bmp` shows the three Cyrillic pairs are byte-identical copies [C], so the doc\'s "copied to Square and Editor" is accurate; only the packaging differs (`tools/language.list` ships the speccy pair only).',
}

def key(s):
    return re.sub(r'[^a-z0-9 ]', ' ', s.lower())

def _find(rs, doc, sub):
    for r in rs:
        if r['doc'] == doc and sub in r['claim'] or (r['doc'] == doc and sub in r['source_cell']):
            return r
    raise SystemExit(f'discrepancy curation: no row {doc} {sub!r}')

def groups(closing_dir):
    rs = rows(closing_dir)
    split_ids = {id(_find(rs, d, sub)): lab for d, sub, lab in SPLIT}
    for (d, sub), note in NOTES.items():
        _find(rs, d, sub)['note'] = note
    gs = []
    for r in rs:
        if id(r) in split_ids:
            label = split_ids[id(r)]
            for g in gs:
                if g[0].get('split') == label: g.append(r); break
            else:
                r['split'] = label; gs.append([r])
            continue
        best = None
        for g in gs:
            g0 = g[0]
            if g0.get('split'): continue
            same_src = g0['source'].lower() == r['source'].lower()
            ratio = difflib.SequenceMatcher(None, key(g0['claim'])[:160], key(r['claim'])[:160]).ratio()
            ratio2 = difflib.SequenceMatcher(None, key(g0['code'])[:160], key(r['code'])[:160]).ratio()
            if same_src and (ratio > T1 or ratio2 > T2) or (g0['claim'], g0['code']) in FORCE.get((r['claim'], r['code']), ()):
                best = g; break
        if best is None: gs.append([r])
        else: best.append(r)
    # forced merges
    for spec in MERGE:
        members = [_find(rs, d, sub) for d, sub in spec]
        target = next(g for g in gs if members[0] in g)
        for m in members[1:]:
            g = next(g for g in gs if m in g)
            if g is not target:
                target.extend(g); gs.remove(g)
    return gs

if __name__ == '__main__':
    gs = groups(sys.argv[1])
    with open(sys.argv[2], 'w', encoding='utf-8') as fh:
        for i, g in enumerate(gs):
            for r in g:
                fh.write(f"G{i:03d}\t{r['doc']}\t{r['source']}\t{r['claim'][:110]}\t{r['code'][:80]}\n")
    print(len(sum(gs, [])), 'rows ->', len(gs), 'groups;', sum(1 for g in gs if len(g) > 1), 'multi-row groups')
