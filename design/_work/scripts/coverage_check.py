#!/usr/bin/env python3
"""Phase 4 coverage verification.

Joins manifest.csv against (1) every coverage fragment in _work/coverage/*.csv, (2) every source reference cited in the
final documents (design/NN_*.md, excluding generated index/matrix files), (3) the data-format registry, (4) the world-object
class list extracted from the source tree, (5) the asset catalogue, and (6) written out-of-scope justifications.

Reports:
  (a) live source files never cited
  (b) data files never cited
  (c) data formats without a grammar in 29_data_formats_reference.md
  (d) world-object classes without a behaviour spec (02/03/04) or missing from the class diagram (21)
  (e) textures/shapes/sounds/etc. without a catalogue entry (25, or 10 for audio)
  plus: cited paths that do not exist, oversize verbatim excerpts, [U]/[I]/[C] counts.

Usage: python3 -I coverage_check.py <SRC_ROOT> <DESIGN_DIR> [--write-matrix]
Exit code 0 iff (a)-(e) are all empty after justifications.
"""
import csv, glob, json, os, re, sys, collections, fnmatch

SRC, DESIGN = sys.argv[1], sys.argv[2]
WRITE_MATRIX = '--write-matrix' in sys.argv
WORK = os.path.join(DESIGN, '_work')

manifest = list(csv.DictReader(open(os.path.join(WORK, 'manifest.csv'))))
by_lower = {r['path'].lower(): r for r in manifest}

# ---------- final documents ----------
GENERATED = {'91_cross_reference_index.md', '99_coverage_matrix.md', 'README.md'}
docs = sorted(p for p in glob.glob(os.path.join(DESIGN, '*.md')))
doc_text = {os.path.basename(p): open(p, encoding='utf-8', errors='replace').read() for p in docs}
cite_docs = {k: v for k, v in doc_text.items() if k not in GENERATED}

ROOTS = ('code/', 'data/', 'contrib/', 'tools/', 'targets/', 'docs/')
known_dirs = set()
for r in manifest:
    parts = r['path'].lower().split('/')
    for i in range(1, len(parts)):
        known_dirs.add('/'.join(parts[:i]))
root_files = {r['path'].lower() for r in manifest if '/' not in r['path']}

def extract_paths(text):
    """Return set of lower-case manifest-relative paths (files or dirs) cited in text."""
    found = set()
    cands = []
    # backticked spans may contain spaces (e.g. `docs/cpu cycles.txt`)
    for span in re.findall(r'`([^`\n]+)`', text):
        cands.append(span)
    # bare paths (no spaces)
    cands += re.findall(r'(?<![\w/])((?:code|data|contrib|tools|targets|docs)/[\w\-./+]+)', text)
    for c in cands:
        c = c.strip().strip('"\'').replace('\\', '/')
        if c.startswith('multiwinia/'): c = c[len('multiwinia/'):]
        if not c.lower().startswith(ROOTS) and c.lower() not in root_files:
            continue
        # strip ":Symbol" / ":L123" / "#anchor" suffixes and trailing punctuation
        base = re.split(r'[:#](?![/\\])', c, 1)[0].rstrip('.,;)')
        low = base.lower()
        # glob-ish citations like code/worldobject/darwinian.* or data/levels/mp_*
        if any(ch in low for ch in '*?['):
            found.add(('glob', low)); continue
        found.add(('path', low.rstrip('/')))
    return found

cited_by_doc = {}
for name, text in cite_docs.items():
    cited_by_doc[name] = extract_paths(text)

cited_files = collections.defaultdict(set)   # path -> docs
cited_dirs = collections.defaultdict(set)
bad_citations = collections.defaultdict(set)
for name, items in cited_by_doc.items():
    for kind, p in items:
        if kind == 'glob':
            hits = [m for m in by_lower if fnmatch.fnmatchcase(m, p)]
            if not hits: bad_citations[p].add(name)
            for h in hits: cited_files[h].add(name)
        elif p in by_lower:
            cited_files[p].add(name)
        elif p in known_dirs:
            cited_dirs[p].add(name)
        else:
            # tolerate a cited stem without extension: code/foo -> code/foo.cpp/.h
            stem_hits = [m for m in by_lower if m.rsplit('.', 1)[0] == p]
            # a bare (un-backticked) match truncated at a space in a real file name, e.g. "docs/sound changes.txt"
            space_hits = [m for m in by_lower if m.startswith(p + ' ')]
            if stem_hits:
                for h in stem_hits: cited_files[h].add(name)
            elif not space_hits:
                bad_citations[p].add(name)

# ---------- coverage fragments ----------
depth_rank = {'full': 3, 'skimmed': 2, 'not-read': 1}
frag_depth = {}
frag_src = collections.defaultdict(set)
frag_sections = collections.defaultdict(set)
for fp in sorted(glob.glob(os.path.join(WORK, 'coverage', '*.csv'))):
    agent = os.path.basename(fp)[:-4]
    try:
        for row in csv.DictReader(open(fp, encoding='utf-8', errors='replace')):
            p = (row.get('path') or '').strip().replace('\\', '/')
            if p.startswith('multiwinia/'): p = p[len('multiwinia/'):]
            low = p.lower().rstrip('/')
            d = (row.get('depth') or '').strip().lower()
            targets = [low] if low in by_lower else [m for m in by_lower if m.startswith(low + '/')] if low in known_dirs else []
            for t in targets:
                if depth_rank.get(d, 0) > depth_rank.get(frag_depth.get(t, ''), 0):
                    frag_depth[t] = d
                frag_src[t].add(agent)
                if row.get('sections'): frag_sections[t].add(row['sections'][:80])
    except Exception as e:
        print('WARN cannot read fragment', fp, e)

# ---------- justifications ----------
# _work/out_of_scope.csv: pattern,justification   (fnmatch on lower-case path)
just_rules = []
jp = os.path.join(WORK, 'out_of_scope.csv')
if os.path.exists(jp):
    for row in csv.DictReader(open(jp, encoding='utf-8')):
        just_rules.append((row['pattern'].lower(), row['justification']))
def justification(p):
    for pat, j in just_rules:
        if fnmatch.fnmatchcase(p, pat): return j
    return ''

def is_live_source(r):
    return r['type'].startswith('source/') and r['vcproj_build'] and r['vcproj_build'] != 'Debug;Release;DebugDirect3D;ReleaseDirect3D'

# ---------- (a) live source never cited ----------
gap_a = [r['path'] for r in manifest if is_live_source(r) and r['path'].lower() not in cited_files and not justification(r['path'].lower())]

# ---------- (b) data files never cited ----------
gap_b = [r['path'] for r in manifest if r['area'] == 'data' and r['path'].lower() not in cited_files and not justification(r['path'].lower())]

# ---------- (c) data formats without grammar ----------
reg = []
rp = os.path.join(WORK, 'formats_registry.csv')
if os.path.exists(rp):
    for row in csv.DictReader(open(rp, encoding='utf-8')):
        reg.append(row)
doc29 = doc_text.get('29_data_formats_reference.md', '')
fmt_of = {}
gap_c_fmt = []
for row in reg:
    anchor = 'fmt-' + row['format_id']
    if (f'id="{anchor}"' not in doc29) and (f'{{#{anchor}}}' not in doc29):
        gap_c_fmt.append(row['format_id'])
unclassified = []
for r in manifest:
    if r['area'] != 'data': continue
    low = r['path'].lower()
    hit = None
    for row in reg:
        if any(fnmatch.fnmatchcase(low, pat.strip().lower()) for pat in row['patterns'].split(';') if pat.strip()):
            hit = row['format_id']; break
    if hit: fmt_of[low] = hit
    elif not justification(low): unclassified.append(r['path'])
gap_c = [f'format without grammar: {f}' for f in gap_c_fmt] + [f'data file with no registered format: {p}' for p in unclassified]

# ---------- (d) world-object classes ----------
classes = {}
for h in sorted(glob.glob(os.path.join(SRC, 'code', 'worldobject', '*.h'))):
    txt = open(h, encoding='latin-1').read()
    txt = re.sub(r'//[^\n]*', '', txt); txt = re.sub(r'/\*.*?\*/', '', txt, flags=re.S)
    for m in re.finditer(r'\bclass\s+(\w+)\s*(?::\s*([^{;]+))?\{', txt):
        name, bases = m.group(1), (m.group(2) or '').strip()
        classes[name] = (os.path.relpath(h, SRC).replace(os.sep, '/'), re.sub(r'\s+', ' ', bases))
spec_docs = ''.join(doc_text.get(d, '') for d in ('02_units_and_entities.md', '03_buildings_and_world_objects.md', '04_crates_powerups_and_weapons.md'))
diagram = doc_text.get('21_simulation_and_entity_model.md', '')
mermaid21 = '\n'.join(re.findall(r'```mermaid(.*?)```', diagram, flags=re.S))
gap_d = []
for name, (path, bases) in sorted(classes.items()):
    has_spec = re.search(r'^#{2,5}\s.*\b' + re.escape(name) + r'\b', spec_docs, flags=re.M) is not None
    in_diag = re.search(r'\b' + re.escape(name) + r'\b', mermaid21) is not None
    j = justification('class:' + name.lower())
    if (not has_spec or not in_diag) and not j:
        gap_d.append(f'{name} ({path}) spec={"Y" if has_spec else "N"} diagram={"Y" if in_diag else "N"}')

# ---------- (e) asset catalogue ----------
cat25 = doc_text.get('25_textures_and_assets.md', '')
cat10 = doc_text.get('10_audio_design.md', '')
cat25_paths = {p for k, p in extract_paths(cat25) if k == 'path'}
cat10_paths = {p for k, p in extract_paths(cat10) if k == 'path'}
ASSET_DIRS = ['data/shaders/', 'data/textures/', 'data/shapes/', 'data/sprites/', 'data/animations/', 'data/terrain/',
              'data/icons/', 'data/thumbnails/', 'data/mwhelp/', 'data/sepulveda/']
AUDIO_DIRS = ['data/sounds/', 'data/mwsounds/']
gap_e = []
for r in manifest:
    low = r['path'].lower()
    if any(low.startswith(d) for d in ASSET_DIRS):
        if low.endswith('.txt') and low.startswith('data/mwhelp/'):
            continue  # help text files are a data format, checked in (b)/(c)
        if low not in cat25_paths and not justification('catalogue:' + low):
            gap_e.append(r['path'])
    elif any(low.startswith(d) for d in AUDIO_DIRS):
        if low not in cat25_paths and low not in cat10_paths and not justification('catalogue:' + low):
            gap_e.append(r['path'])

# ---------- excerpts & tags ----------
oversize = []
for name, text in doc_text.items():
    for m in re.finditer(r'Verbatim excerpt[^\n]*\n```[^\n]*\n(.*?)```', text, flags=re.S):
        n = m.group(1).count('\n')
        if n > 15: oversize.append(f'{name}: {n} lines at offset {m.start()}')
tag_counts = {t: sum(len(re.findall(re.escape(t), v)) for v in cite_docs.values()) for t in ('[C]', '[I]', '[U]')}

# ---------- report ----------
def section(title, items, limit=400):
    out = [f'### {title} — {len(items)}']
    out += [f'- {i}' for i in items[:limit]]
    if len(items) > limit: out.append(f'- … {len(items) - limit} more (see JSON)')
    return '\n'.join(out) + '\n'
rep = ['# Coverage report (generated by _work/scripts/coverage_check.py)\n',
       f'Documents scanned for citations: {len(cite_docs)} ({", ".join(sorted(cite_docs))})\n',
       f'Manifest entries: {len(manifest)}; live source files: {sum(1 for r in manifest if is_live_source(r))}; data files: {sum(1 for r in manifest if r["area"] == "data")}\n',
       f'Distinct manifest files cited: {len(cited_files)}; directories cited: {len(cited_dirs)}\n',
       f'Evidence tags in documents: {tag_counts}\n',
       f'World-object classes found in code/worldobject/*.h: {len(classes)}\n',
       section('(a) live source files never cited', gap_a),
       section('(b) data files never cited', gap_b),
       section('(c) data formats without a grammar', gap_c),
       section('(d) world-object classes without behaviour spec / diagram entry', gap_d),
       section('(e) assets without a catalogue entry', gap_e),
       section('Cited paths that do not exist in the manifest (possible errors)', sorted(f'{p}  ← {", ".join(sorted(d))}' for p, d in bad_citations.items())),
       section('Verbatim excerpts over 15 lines', oversize)]
open(os.path.join(WORK, 'coverage_report.md'), 'w').write('\n'.join(rep))
json.dump({'a': gap_a, 'b': gap_b, 'c': gap_c, 'd': gap_d, 'e': gap_e, 'bad_citations': {k: sorted(v) for k, v in bad_citations.items()},
           'oversize_excerpts': oversize, 'tags': tag_counts, 'classes': {k: list(v) for k, v in classes.items()}},
          open(os.path.join(WORK, 'coverage_report.json'), 'w'), indent=1)

# ---------- 99 coverage matrix ----------
if WRITE_MATRIX:
    hdr = open(os.path.join(WORK, '99_header.md'), encoding='utf-8').read() if os.path.exists(os.path.join(WORK, '99_header.md')) else '# 99 — Coverage matrix\n'
    lines = [hdr.rstrip() + '\n',
             f'\n_Generated by `_work/scripts/coverage_check.py`. Gaps at generation: (a) {len(gap_a)}, (b) {len(gap_b)}, (c) {len(gap_c)}, (d) {len(gap_d)}, (e) {len(gap_e)}._\n']
    groups = collections.OrderedDict()
    for r in manifest:
        groups.setdefault(r['area'], []).append(r)
    for area, rs in groups.items():
        lines.append(f'\n## {area} ({len(rs)} entries)\n\n| Path | Build | Cited in | Depth (agents) | Out-of-scope justification |\n|---|---|---|---|---|')
        for r in rs:
            low = r['path'].lower()
            docs_ = ', '.join(sorted(d.split('_')[0] for d in cited_files.get(low, set())))
            dep = frag_depth.get(low, '')
            ag = ','.join(sorted(frag_src.get(low, set())))
            build = 'live' if is_live_source(r) else (r['vcproj_build'] and 'excluded') or ('n/a' if not r['type'].startswith('source/') else 'not-in-project')
            j = justification(low)
            if not j and r['type'].startswith('source/') and not is_live_source(r):
                j = 'Not compiled into any PC configuration (absent from, or excluded in, Darwinia.vcproj) — [B:DEAD]; see _work/build_matrix.md §6.'
            lines.append(f"| `{r['path']}` | {build} | {docs_} | {dep}{' ('+ag+')' if ag else ''} | {j} |")
    open(os.path.join(DESIGN, '99_coverage_matrix.md'), 'w', encoding='utf-8').write('\n'.join(lines) + '\n')

print(f"(a) {len(gap_a)}  (b) {len(gap_b)}  (c) {len(gap_c)}  (d) {len(gap_d)}  (e) {len(gap_e)}  bad-citations {len(bad_citations)}  oversize {len(oversize)}  tags {tag_counts}")
sys.exit(0 if not (gap_a or gap_b or gap_c or gap_d or gap_e) else 1)
