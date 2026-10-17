#!/usr/bin/env python3
"""S_41: build 41_risks_and_open_questions.md from
  * the collected closing sections (_work/closing/closing_*.md, from collect_closing_sections.py),
  * the curation overlay _work/script_inputs/S_41_overlay.tsv (impact, status, merges) and
    _work/script_inputs/S_41_extra_rows.tsv (design-critical [I] items listed only in prose),
  * the IP category table _work/script_inputs/S_41_ip_categories.tsv,
  * the hand-written template _work/script_inputs/S_41_template.md ({{PLACEHOLDER}} regions),
  * canonical placeholders from 90_glossary.md ("Canonical placeholder names") when that section exists,
    otherwise "see 90" with the S90 draft value from _work/script_inputs/S_90_terms.tsv,
  * _work/drafts/X4_levels.csv for the map-name register.
Self-checks: every parsed unknown row has an overlay row; every merge target exists and is a primary;
every IP row from every document maps to at least one category; every discrepancy row is in a group.
Usage: python3 -I S_41_build.py <DESIGN_DIR>"""
import csv, os, re, sys, importlib.util
from collections import OrderedDict, defaultdict

design = sys.argv[1]
work = os.path.join(design, '_work')
here = os.path.dirname(os.path.abspath(__file__))

def load(name):
    spec = importlib.util.spec_from_file_location(name, os.path.join(here, name + '.py'))
    m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m); return m

pc = load('S_41_parse_closing')
dq = load('S_41_discrepancies')
closing = os.path.join(work, 'closing')

DOCS = OrderedDict()
for fn in sorted(os.listdir(design)):
    m = re.match(r'^(\d\d)_([a-z0-9_]+)\.md$', fn)
    if m and m.group(1) not in ('40', '41', '90', '91', '99'):
        DOCS[m.group(1)] = fn

def dlink(nn, anchor=''):
    return f'[{nn}]({DOCS.get(nn, nn)}{anchor})'

def cell(s):
    return re.sub(r'(?<!\\)\|', r'\\|', s.replace('\n', ' ').strip())

def tsv(path):
    out = []
    for line in open(path, encoding='utf-8'):
        if line.startswith('#') or not line.strip():
            continue
        out.append(line.rstrip('\n').split('\t'))
    return out

# ------------------------------------------------------------------ unknowns
rows = pc.unknown_rows(closing)
for r in tsv(os.path.join(work, 'script_inputs', 'S_41_extra_rows.tsv')):
    rid, nn, tag, q, res = (r + [''] * 5)[:5]
    rows.append(dict(doc=nn, file=DOCS.get(nn, ''), id=rid, question=q, tag=tag, why='', resolve=res, extra=True))
byid = OrderedDict()
for r in rows:
    if r['id'] in byid:
        raise SystemExit(f'duplicate id {r["id"]}')
    byid[r['id']] = r

ov = {}
for r in tsv(os.path.join(work, 'script_inputs', 'S_41_overlay.tsv')):
    r = (r + [''] * 5)[:5]
    ov[r[0]] = dict(impact=r[1], status=r[2], note=r[3], merge=r[4])
missing = [i for i in byid if i not in ov]
extra = [i for i in ov if i not in byid]
if missing or extra:
    raise SystemExit(f'overlay mismatch: missing {missing} extra {extra}')

primary_of = {}
for i, o in ov.items():
    if o['status']:
        primary_of[i] = i
    else:
        t = o['merge']
        if t not in ov or not ov[t]['status']:
            raise SystemExit(f'{i}: merge target {t!r} is not a primary')
        primary_of[i] = t
alsos = defaultdict(list)
for i, p in primary_of.items():
    if p != i:
        alsos[p].append(i)
for p, o in ov.items():           # cross-check declared also-lists
    if o['status'] and o['merge']:
        for a in o['merge'].split(';'):
            if primary_of.get(a) != p:
                raise SystemExit(f'{p}: declared also {a} does not point back')

def tagclass(tag):
    if '[U]' in tag: return 'U'
    if '[I]' in tag or 'design-critical' in tag: return 'I'
    return 'C'

def fmt_status(p):
    o = ov[p]
    if o['status'] == 'Resolved':
        return 'Resolved — ' + o['note']
    return 'Open' + (' — ' + o['note'] if o['note'] else '')

def resolve_text(p):
    parts = [byid[p]['resolve']]
    for a in alsos[p]:
        rr = byid[a]['resolve']
        if rr and rr not in parts and rr not in ('—', '-'):
            parts.append(f'({a}) {rr}')
    return ' · '.join(x for x in parts if x)

def question_text(p):
    q = byid[p]['question']
    if byid[p].get('why'):
        q += f" *Why it matters:* {byid[p]['why']}"
    return q

def ids_text(p):
    return ' / '.join([p] + sorted(alsos[p], key=lambda x: (x[:2], int(re.sub(r'\D', '', x[3:]) or 0))))

reg = []
for nn in list(DOCS):
    doc_rows = [r for r in rows if r['doc'] == nn]
    if not doc_rows:
        continue
    title = open(os.path.join(design, DOCS[nn]), encoding='utf-8').readline().strip().lstrip('# ').strip()
    reg.append(f'\n### 3.{len([x for x in reg if x.startswith(chr(10) + "### 3.")]) + 1} {title}\n')
    prim = [r for r in doc_rows if primary_of[r['id']] == r['id']]
    sec = [r for r in doc_rows if primary_of[r['id']] != r['id']]
    if prim:
        reg.append('| Id(s) | Question | Tag | Impact | How to resolve | Status | Link |')
        reg.append('|---|---|---|---|---|---|---|')
        for r in prim:
            p = r['id']
            link = dlink(nn, '#open-questions-and-unknowns')
            others = sorted({byid[a]['doc'] for a in alsos[p]} - {nn})
            if others:
                link += ', ' + ', '.join(dlink(o, '#open-questions-and-unknowns') for o in others)
            reg.append('| ' + ' | '.join(cell(x) for x in [
                ids_text(p), question_text(p), r['tag'] or '—', ov[p]['impact'], resolve_text(p) or '—',
                fmt_status(p), link]) + ' |')
    if sec:
        reg.append('')
        reg.append('Merged into an entry owned by another document (listed there under all ids): ' +
                   '; '.join(f'{r["id"]} → {primary_of[r["id"]]} ({ov[primary_of[r["id"]]]["status"]})' for r in sec) + '.')

# per-document statistics
dis_rows = dq.rows(closing)
ip_rows = pc.generic_rows(closing, 'ip')
stats_csv = {r['doc'][:2]: r for r in csv.DictReader(open(os.path.join(closing, 'closing_stats.csv')))}
st = ['| Doc | Register rows | Open [U] | Open design-critical [I] | Resolved | Merged into another doc\'s entry | Docs-vs-code rows | IP rows | [U] tags in body + tables |',
      '|---|---|---|---|---|---|---|---|---|']
tot = defaultdict(int)
for nn in DOCS:
    dr = [r for r in rows if r['doc'] == nn]
    c = dict(n=len(dr), u=0, i=0, res=0, mer=0)
    for r in dr:
        p = primary_of[r['id']]
        if p != r['id']:
            c['mer'] += 1
        if ov[p]['status'] == 'Resolved':
            c['res'] += 1
        elif tagclass(r['tag']) == 'U':
            c['u'] += 1
        else:
            c['i'] += 1
    c['dis'] = sum(1 for r in dis_rows if r['doc'] == nn)
    c['ip'] = sum(1 for r in ip_rows if r['doc'] == nn)
    c['tags'] = int(stats_csv.get(nn, {}).get('U_tags', 0) or 0)
    for k, v in c.items():
        tot[k] += v
    st.append(f"| {dlink(nn)} | {c['n']} | {c['u']} | {c['i']} | {c['res']} | {c['mer']} | {c['dis']} | {c['ip']} | {c['tags']} |")
st.append(f"| **Total** | **{tot['n']}** | **{tot['u']}** | **{tot['i']}** | **{tot['res']}** | **{tot['mer']}** | **{tot['dis']}** | **{tot['ip']}** | **{tot['tags']}** |")
nprim = sum(1 for i in ov if ov[i]['status'])
nprim_open = sum(1 for i in ov if ov[i]['status'] == 'Open')
nprim_res = nprim - nprim_open
st.append('')
st.append(f"After de-duplication the register holds **{nprim} distinct entries** ({nprim_open} open, {nprim_res} resolved); "
          f"{tot['mer']} source rows were merged into an entry owned by another document. Counts per document are by source row; "
          f"a row counts as resolved when its (merged) entry is resolved. \"Open design-critical [I]\" counts every open row the owning document "
          f"tagged [I] (owners list only design-critical [I] items in their tables).")

# ------------------------------------------------------------------ discrepancies
gs = dq.groups(closing)
def src_kind(s):
    if s.startswith('docs/'): return 1
    if s.startswith('code/') or s.startswith('contrib/'): return 2
    return 3
sections = {1: [], 2: [], 3: []}
for g in gs:
    s = g[0]['source']
    sections[src_kind(s)].append(g)
def longest(vals):
    vals = [v for v in vals if v]
    return max(vals, key=len) if vals else ''
dis_out = []
heads = {1: '### 4.1 Documents in `docs/` (and the manual)',
         2: '### 4.2 Code comments, declarations and headers (`code/`, `contrib/`)',
         3: '### 4.3 Data-file headers, tools, targets, installers and other artefacts'}
n = 0
for k in (1, 2, 3):
    dis_out.append('\n' + heads[k] + '\n')
    dis_out.append('| # | Source | Says | Code / data truth | Ref | Raised in |')
    dis_out.append('|---|---|---|---|---|---|')
    for g in sorted(sections[k], key=lambda g: (g[0]['source'].lower(), g[0]['claim'])):
        n += 1
        src = g[0]['source']
        claim = longest(r['claim'] for r in g)
        code = longest(r['code'] for r in g)
        notes = [r['note'] for r in g if r.get('note')]
        if notes:
            code += ' ' + ' '.join(notes)
        refs = []
        for r in g:
            if r['ref'] and r['ref'] not in refs:
                refs.append(r['ref'])
        docs_ = sorted({r['doc'] for r in g})
        dis_out.append('| ' + ' | '.join(cell(x) for x in [
            f'D{n:03d}', f'`{src}`' if '/' in src or '.' in src else src, claim, code, ' · '.join(refs) or '—',
            ', '.join(dlink(d, '#docs-vs-code-discrepancies') for d in docs_)]) + ' |')
dis_summary = (f'{len(dis_rows)} source rows from {len({r["doc"] for r in dis_rows})} documents, de-duplicated into **{n} entries** '
               f'({sum(1 for g in gs if len(g) > 1)} entries merge rows raised by several documents). Where wordings differ, the most complete '
               f'statement of each side is kept; refs are the union.')

# ------------------------------------------------------------------ IP
cats = []
for r in tsv(os.path.join(work, 'script_inputs', 'S_41_ip_categories.tsv')):
    r = (r + [''] * 8)[:8]
    cats.append(dict(id=r[0], family=r[1], item=r[2], kind=r[3], where=r[4], terms=[t for t in r[5].split(';') if t and t != '—'],
                     fallback=r[6], rx=re.compile(r[7], re.I) if r[7] else None, rows=[]))
unmapped = []
for r in ip_rows:
    c = r['cells']
    text = ' '.join(c[:3])
    hit = False
    for cat in cats:
        if cat['rx'] and cat['rx'].search(text):
            cat['rows'].append(r); hit = True
    if not hit:
        unmapped.append(r)
if unmapped:
    for r in unmapped:
        print('UNMAPPED IP row', r['doc'], r['cells'][0][:120], file=sys.stderr)
    raise SystemExit(f'{len(unmapped)} IP rows not mapped to a category')

# canonical placeholders (90_glossary.md section "Canonical placeholder names")
def plain(t):
    return re.sub(r'[`*"\u201c\u201d]', '', t).strip().lower()
canon = []
gl = os.path.join(design, '90_glossary.md')
canon_source = 'none'
if os.path.exists(gl):
    text = open(gl, encoding='utf-8').read()
    m = re.search(r'(?ms)^## [^\n]*Canonical placeholder names[^\n]*\n(.*?)(?=^## |\Z)', text)
    if m:
        for head, trs in pc.tables(m.group(1)):
            hl = [h.lower() for h in head]
            if not any('canonical' in h for h in hl):
                continue
            pi = next(i for i, h in enumerate(hl) if 'canonical' in h)
            for tr in trs:
                if len(tr) > pi:
                    canon.append((tr[0], tr[pi]))
        if canon:
            canon_source = '90'

def canonical(cat):
    if canon_source != '90':
        return 'see [90](90_glossary.md) §3' + (f' (owning-document proposal: {cat["fallback"]})' if cat['fallback'] else '')
    vals = []
    for pre in cat['terms']:
        for orig, ph in canon:
            if plain(orig).startswith(pre) and (orig, ph) not in vals:
                vals.append((orig, ph))
    if not vals:
        return f'not named in [90](90_glossary.md) §3 — use: {cat["fallback"]}' if cat['fallback'] else 'not named in 90 §3'
    return '; '.join(f'{o} → {p}' for o, p in vals)

ip_out = ['| Cat | Family | IP items (enumerated) | Kind | Where | Canonical neutral placeholder ([90](90_glossary.md) §3) | Source rows | Raised in |',
          '|---|---|---|---|---|---|---|---|']
for cat in cats:
    props = []
    for r in cat['rows']:
        ph = r['cells'][-1].strip()
        if ph and ph not in props and len(props) < 6 and len(ph) < 160:
            props.append(ph)
    docs_ = sorted({r['doc'] for r in cat['rows']})
    ip_out.append('| ' + ' | '.join(cell(x) for x in [
        cat['id'], cat['family'], cat['item'], cat['kind'], cat['where'], canonical(cat),
        str(len(cat['rows'])), ', '.join(dlink(d, '#ip-items') for d in docs_) or '—']) + ' |')
ip_summary = (f'{len(ip_rows)} IP rows from {len({r["doc"] for r in ip_rows})} documents are mapped onto **{len(cats)} replacement categories** '
              f'(every source row maps to at least one category; the build script aborts otherwise). Canonical placeholders: '
              + (f'read from [90](90_glossary.md) §3 "Canonical placeholder names" ({len(canon)} rows) at build time; where 90 names none, the owning documents\' proposal is given and marked.' if canon_source == '90'
                 else '90_glossary.md §3 was not available at build time, so the column reads "see 90".'))

# map-name register
maps = []
for r in csv.DictReader(open(os.path.join(work, 'drafts', 'X4_levels.csv'), encoding='utf-8')):
    if os.path.basename(r['file']).startswith('mp_'):
        maps.append(r)
mp = ['| Level file | English display name (IP) | Menu | Placeholder pattern |', '|---|---|---|---|']
for r in sorted(maps, key=lambda r: r['file'].lower()):
    base = os.path.basename(r['file'])[:-4]
    mm = re.match(r'mp_([a-z]+)_(\d)p_?(\w*)', base, re.I)
    if mm:
        patt = f'`map_{mm.group(1).lower()}_{mm.group(2)}p_<nn>`'
    else:
        patt = '`map_<mode>_<players>p_<nn>`'
    name = r.get('english_name') or r.get('name') or '—'
    listed = r.get('menu_listed_dir') or r.get('menu_types') or ''
    mp.append(f"| `{r['file']}` | {cell(name) if name else '—'} | {cell(listed) or '—'} | {patt} |")
map_summary = f'{len(maps)} `mp_*` level files (from `_work/drafts/X4_levels.csv`, X4 level catalogue).'

# ------------------------------------------------------------------ assemble
tpl = open(os.path.join(work, 'script_inputs', 'S_41_template.md'), encoding='utf-8').read()
repl = {
    'STATS': '\n'.join(st),
    'REGISTER': '\n'.join(reg),
    'DISCREPANCY_SUMMARY': dis_summary,
    'DISCREPANCIES': '\n'.join(dis_out),
    'IP_SUMMARY': ip_summary,
    'IP': '\n'.join(ip_out),
    'MAP_SUMMARY': map_summary,
    'MAPS': '\n'.join(mp),
    'NPRIM': str(nprim), 'NOPEN': str(nprim_open), 'NRES': str(nprim_res), 'NROWS': str(tot['n']),
    'NDIS': str(n), 'NDISROWS': str(len(dis_rows)), 'NIP': str(len(ip_rows)), 'NCAT': str(len(cats)),
}
def sub(m):
    k = m.group(1)
    if k not in repl:
        raise SystemExit(f'unknown placeholder {k}')
    return repl[k]
out = re.sub(r'\{\{([A-Z_]+)\}\}', sub, tpl)
open(os.path.join(design, '41_risks_and_open_questions.md'), 'w', encoding='utf-8').write(out)
print(f'41 written: {len(out)} chars; register {nprim} entries ({nprim_open} open), {tot["n"]} rows; '
      f'discrepancies {n} entries from {len(dis_rows)} rows; IP {len(ip_rows)} rows -> {len(cats)} categories; canon={canon_source}')
