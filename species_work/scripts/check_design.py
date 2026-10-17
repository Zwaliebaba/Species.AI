"""Structural gates of prompt §7.3 for the Species reference library.

Usage:
  python3 -I check_design.py --root <DESIGN_ROOT> --repo <REPO> --hard <forbidden.tsv> --soft <soft.tsv>
         --scanner <scan_patterns.py> [--mermaid-js <mermaid.min.js>] [--skip-mermaid]
         [--report <gate_report.md>] [--write-index]

Gates (numbering follows §7.3):
  1  hard-fail pattern scan: zero hits
  2  soft-flag scan: every hit is allowed by CONVENTIONS.md §9 (column "Paths")
  3  relative links resolve, anchors exist (GitHub slugs); no link leaves the root except to
     vision/ and prompts/species-design-phase.md
  4  every Mermaid block parses (headless Chromium, pinned mermaid 11 build)
  5  requirement ids: prefix registered (CONVENTIONS.md §3.1), defined exactly once, every citation exists
  6  DEC and ADR citations exist; every Open DEC is cited inside at least one requirement
  7  asset closure: cited asset ids exist in assets/asset_manifest.csv; every manifest row has a consumer
  8  tuning keys: defined once; each cited by a requirement (in the requirement text or the row's
     requirement column)
  9  truncation words outside code: zero
  10 files <= 120 KB (assets/*.csv and assets/GameAssets.md exempt); every folder has README.md;
     reference and component documents carry their template headings
  11 --write-index writes REQUIREMENTS_INDEX.md at the root
Exit status 1 when any gate fails.
"""
import argparse
import csv
import fnmatch
import glob
import os
import re
import subprocess
import sys
from collections import Counter, defaultdict

ap = argparse.ArgumentParser()
ap.add_argument('--root', required=True)
ap.add_argument('--repo', required=True)
ap.add_argument('--hard', required=True)
ap.add_argument('--soft', required=True)
ap.add_argument('--scanner', required=True)
ap.add_argument('--mermaid-js')
ap.add_argument('--skip-mermaid', action='store_true')
ap.add_argument('--report')
ap.add_argument('--write-index', action='store_true')
A = ap.parse_args()
ROOT = os.path.abspath(A.root)
REPO = os.path.abspath(A.repo)
HERE = os.path.dirname(os.path.abspath(__file__))

REF_H = ['Purpose and player experience', 'Rules', 'States and flows', 'Tuning table',
         'Edge cases and interactions', 'Acceptance criteria', 'Dependencies', 'Open decisions']
CMP_H = ['Responsibilities and boundaries', 'Interfaces', 'Data model', 'Behaviour and algorithms',
         'Concurrency and performance budget', 'Failure modes, diagnostics and telemetry',
         'Acceptance tests', 'Open decisions']
TRUNC = re.compile(r'\betc\b\.?|\band so on\b|\band others\b|\bvarious\b|\bsimilar to the above\b|…', re.I)
FENCE = re.compile(r'^[ \t]*(```|~~~)')
size_exempt = ('assets/GameAssets.md',)

results = {}   # gate -> list of failure strings
info = {}      # gate -> summary line


def fail(g, msg):
    results.setdefault(g, []).append(msg)


def rel(p):
    return os.path.relpath(p, ROOT).replace(os.sep, '/')


MD = sorted(glob.glob(os.path.join(ROOT, '**', '*.md'), recursive=True))
TEXT = {rel(p): open(p, encoding='utf-8').read() for p in MD}


def prose_lines(text):
    """Yield (line_no, line) outside fenced code blocks."""
    inside = False
    for i, line in enumerate(text.split('\n'), 1):
        if FENCE.match(line):
            inside = not inside
            continue
        if not inside:
            yield i, line


def strip_code_spans(line):
    return re.sub(r'`[^`]*`', ' ', line)


def prose_text(text):
    return '\n'.join(l for _, l in prose_lines(text))


# ---------------------------------------------------------------- gate 1 and 2: pattern scans
def scan(pats):
    out = subprocess.run([sys.executable, '-I', A.scanner, pats, ROOT], capture_output=True, text=True).stdout
    hits = []
    for line in out.splitlines():
        parts = line.split(':', 3)
        if len(parts) == 4:
            hits.append((rel(os.path.abspath(parts[0])), int(parts[1]), parts[2], parts[3]))
    return hits


for f, ln, cls, m in scan(A.hard):
    fail(1, f'{f}:{ln} [{cls}] {m}')
info[1] = f'{len(results.get(1, []))} hard-fail hits'


def allowed_terms():
    """CONVENTIONS.md §9 rows: (term, [path globs])."""
    t = TEXT.get('CONVENTIONS.md', '')
    sec = re.search(r'^## 9 Allowed terms\n(.*?)(?=^## |\Z)', t, re.M | re.S)
    rows = []
    if not sec:
        fail(2, 'CONVENTIONS.md has no "## 9 Allowed terms" section')
        return rows
    lines = [l for l in sec.group(1).split('\n') if l.startswith('|')]
    if not lines:
        return rows
    head = [c.strip().lower() for c in lines[0].strip('|').split('|')]
    if 'paths' not in head:
        fail(2, 'CONVENTIONS.md §9 table has no "Paths" column')
        return rows
    pi = head.index('paths')
    for l in lines[2:]:
        cells = [c.strip() for c in l.strip('|').split('|')]
        rows.append((cells[0].lower(), re.findall(r'`([^`]+)`', cells[pi])))
    return rows


ALLOW = allowed_terms()
soft = scan(A.soft)
n_allowed = 0
for f, ln, cls, m in soft:
    ok = any(m.lower().replace(' ', '-') == term.replace(' ', '-') and any(fnmatch.fnmatch(f, g) for g in globs)
             for term, globs in ALLOW)
    if ok:
        n_allowed += 1
    else:
        fail(2, f'{f}:{ln} [{cls}] {m}')
info[2] = f'{len(soft)} soft hits, {n_allowed} allowed by CONVENTIONS §9, {len(results.get(2, []))} not allowed'


# ---------------------------------------------------------------- gate 3: links and anchors
def slug(heading):
    h = re.sub(r'!\[([^\]]*)\]\([^)]*\)', r'\1', heading)
    h = re.sub(r'\[([^\]]*)\]\([^)]*\)', r'\1', h)
    h = re.sub(r'<[^>]+>', '', h)
    h = h.strip().lower()
    h = re.sub(r'[^\w\- ]', '', h)
    return h.replace(' ', '-')


def anchors_of(text):
    out, seen = set(), Counter()
    for _, line in prose_lines(text):
        m = re.match(r'^(#{1,6})\s+(.*?)\s*#*\s*$', line)
        if m:
            s = slug(m.group(2))
            out.add(s if seen[s] == 0 else f'{s}-{seen[s]}')
            seen[s] += 1
        for a in re.findall(r'<a\s+(?:id|name)="([^"]+)"', line):
            out.add(a)
    return out


ANCH = {f: anchors_of(t) for f, t in TEXT.items()}
LINK = re.compile(r'(?<!!)\[(?:[^\]\[]|\[[^\]]*\])*\]\(([^)\s]+)(?:\s+"[^"]*")?\)')
n_links = 0
outside_ok = [os.path.join(REPO, 'vision'), os.path.join(REPO, 'prompts', 'species-design-phase.md')]
for f, t in TEXT.items():
    base = os.path.dirname(os.path.join(ROOT, f))
    for ln, line in prose_lines(t):
        for target in LINK.findall(re.sub(r'`[^`]*`', 'code', line)):
            if re.match(r'^[a-z][a-z0-9+.-]*:', target, re.I):
                continue
            n_links += 1
            path, _, anchor = target.partition('#')
            dest = os.path.normpath(os.path.join(base, path)) if path else os.path.join(ROOT, f)
            if not (dest == ROOT or dest.startswith(ROOT + os.sep)):
                if not any(dest == o or dest.startswith(o + os.sep) for o in outside_ok):
                    fail(3, f'{f}:{ln} link leaves the design root: {target}')
                    continue
            if not os.path.exists(dest):
                fail(3, f'{f}:{ln} broken link: {target}')
                continue
            if anchor:
                if os.path.isdir(dest):
                    dest = os.path.join(dest, 'README.md')
                if dest.endswith('.md'):
                    key = rel(dest) if dest.startswith(ROOT + os.sep) else None
                    anchors = ANCH.get(key) if key else anchors_of(open(dest, encoding='utf-8').read())
                    if anchors is not None and anchor.lower() not in anchors and anchor not in anchors:
                        fail(3, f'{f}:{ln} missing anchor: {target}')
info[3] = f'{n_links} relative links checked'


# ---------------------------------------------------------------- gate 4: mermaid
if A.skip_mermaid:
    info[4] = 'SKIPPED (--skip-mermaid)'
    fail(4, 'Mermaid gate skipped')
elif not A.mermaid_js:
    fail(4, 'no --mermaid-js given')
else:
    env = dict(os.environ)
    env.setdefault('NODE_PATH', '/opt/node22/lib/node_modules')
    r = subprocess.run(['node', os.path.join(HERE, 'mermaid_check.js'), ROOT, A.mermaid_js],
                       capture_output=True, text=True, env=env)
    for line in r.stdout.splitlines():
        if line.startswith('FAIL'):
            fail(4, line[5:])
        elif line.startswith('blocks='):
            info[4] = line
    if r.returncode not in (0, 1):
        fail(4, f'mermaid checker error: {r.stderr.strip()[:300]}')


# ---------------------------------------------------------------- gate 5: requirement ids
def registry():
    t = TEXT.get('CONVENTIONS.md', '')
    sec = re.search(r'^### 3\.1 Prefix registry\n(.*?)(?=^##)', t, re.M | re.S)
    return {m: d for m, d in re.findall(r'^\|\s*`([A-Z][A-Z-]*[A-Z])`\s*\|[^|]*\|\s*`?([^`|]*)`?', sec.group(1), re.M)} if sec else {}


PREFIX = registry()
if not PREFIX:
    fail(5, 'CONVENTIONS.md §3.1 prefix registry not found')
PFX = '|'.join(sorted((re.escape(p) for p in PREFIX), key=len, reverse=True))
DEF = re.compile(r'\*\*(' + PFX + r')-(\d{3})\*\*')
ANYDEF = re.compile(r'\*\*((?:R-)?[A-Z]{2,6})-(\d{3})\*\*')
CITE = re.compile(r'(?<![\w-])(' + PFX + r')-(\d{3})(?!\d)')
RANGE = re.compile(r'(?<![\w-])(' + PFX + r')-\d{3}\s*(?:to|–|—|-|…|\.\.)\s*(?:(?:' + PFX + r')-)?\d{3}(?!\d)')
defs = defaultdict(list)   # id -> [(file, line)]
req_blocks = {}            # id -> text of the requirement paragraph
for f, t in TEXT.items():
    lines = t.split('\n')
    prose = dict(prose_lines(t))
    for ln, line in prose.items():
        for p, n in ANYDEF.findall(line):
            if p not in PREFIX and p not in ('DEC', 'ADR'):
                fail(5, f'{f}:{ln} unregistered prefix {p}-{n}')
        for m in DEF.finditer(line):
            rid = f'{m.group(1)}-{m.group(2)}'
            defs[rid].append((f, ln))
            # the requirement text: from the id to the next definition or blank line
            body = [line[m.start():]]
            for j in range(ln, min(ln + 40, len(lines))):
                nxt = lines[j]
                if not nxt.strip() or DEF.search(nxt) or nxt.startswith('#'):
                    break
                body.append(nxt)
            req_blocks[rid] = '\n'.join(body)
for rid, where in defs.items():
    if len(where) > 1:
        fail(5, f'{rid} defined {len(where)} times: ' + ', '.join(f'{f}:{l}' for f, l in where))
n_cites = 0
for f, t in TEXT.items():
    for ln, line in prose_lines(t):
        line2 = RANGE.sub(' ', line)
        for p, n in CITE.findall(line2):
            n_cites += 1
            if f'{p}-{n}' not in defs:
                fail(5, f'{f}:{ln} cites {p}-{n}, which is not defined')
        for m in re.findall(r'(?<![\w-])(R-[A-Z]+)-\d{3}', line2):
            if m not in PREFIX:
                fail(5, f'{f}:{ln} cites unregistered prefix {m}')
info[5] = f'{len(defs)} requirement ids defined, {n_cites} citations, {len(PREFIX)} prefixes registered'


# ---------------------------------------------------------------- gate 6: DEC and ADR references
dec_status = {}
for f, t in TEXT.items():
    if not f.startswith('decisions/') or f.endswith('README.md'):
        continue
    lines = t.split('\n')
    for i, line in enumerate(lines):
        m = re.match(r'^#{2,4} DEC-(\d{3})\s', line)
        if m:
            st = ''
            for nxt in lines[i + 1:i + 8]:
                s = re.match(r'^- \*{0,2}Status\*{0,2}:\*{0,2}\s*([A-Za-z]+)', nxt)
                if s:
                    st = s.group(1)
                    break
            dec_status[int(m.group(1))] = st
adr_ids = {int(os.path.basename(p)[:4]) for p in glob.glob(os.path.join(ROOT, 'adr', '[0-9][0-9][0-9][0-9]-*.md'))}
for f, t in TEXT.items():
    t2 = re.sub(r'(DEC|ADR)-\d{3,4}\s*(?:to|–|-|…)\s*(?:DEC-|ADR-)?\d{3,4}', ' ', t)
    for m in sorted(set(re.findall(r'DEC-(\d{3})\b', t2))):
        if int(m) not in dec_status:
            fail(6, f'{f}: cites DEC-{m}, which does not exist')
    for m in sorted(set(re.findall(r'ADR-(\d{4})\b', t2))):
        if int(m) not in adr_ids:
            fail(6, f'{f}: cites ADR-{m}, which does not exist')
req_text = '\n'.join(req_blocks.values())
cited_in_req = {int(x) for x in re.findall(r'DEC-(\d{3})\b', req_text)}
open_decs = sorted(n for n, s in dec_status.items() if s == 'Open')
for n in open_decs:
    if n not in cited_in_req:
        fail(6, f'Open DEC-{n:03d} is not cited by any requirement')
info[6] = (f'{len(dec_status)} DECs ({len(open_decs)} Open, {len(open_decs) - len([n for n in open_decs if n not in cited_in_req])}'
           f' of them cited by a requirement), {len(adr_ids)} ADRs')


# ---------------------------------------------------------------- gate 8 first: tuning keys (gate 7 needs them)
key_defs = defaultdict(list)    # key -> [(file, line, requirement cell)]
for f, t in TEXT.items():
    lines = t.split('\n')
    cols = None
    for i, line in enumerate(lines):
        if line.startswith('|') and i + 1 < len(lines) and re.match(r'^\|\s*:?-{3,}', lines[i + 1]):
            head = [c.strip().lower() for c in line.strip().strip('|').split('|')]
            cols = head if head[:2] == ['key', 'value'] else None
            continue
        if cols is None:
            continue
        if not line.startswith('|'):
            cols = None
            continue
        if re.match(r'^\|\s*:?-{3,}', line):
            continue
        cells = [c.strip() for c in line.strip().strip('|').split('|')]
        m = re.match(r'^`([^`]+)`$', cells[0])
        if not m:
            fail(8, f'{f}:{i + 1} tuning row without a single backticked key: {cells[0][:60]}')
            continue
        reqcell = cells[cols.index('requirement')] if 'requirement' in cols and cols.index('requirement') < len(cells) else ''
        key_defs[m.group(1)].append((f, i + 1, reqcell))
for k, where in key_defs.items():
    if len(where) > 1:
        fail(8, f'tuning key {k} defined {len(where)} times: ' + ', '.join(f'{f}:{l}' for f, l, _ in where))
n_uncited = 0
for k, where in key_defs.items():
    if k in req_text:
        continue
    reqs = [f'{p}-{n}' for p, n in CITE.findall(' '.join(c for _, _, c in where))]
    if any(r in defs for r in reqs):
        continue
    n_uncited += 1
    fail(8, f'tuning key {k} ({where[0][0]}:{where[0][1]}) is cited by no requirement')
info[8] = f'{len(key_defs)} tuning keys, {n_uncited} cited by no requirement'


# ---------------------------------------------------------------- gate 7: asset closure
man_path = os.path.join(ROOT, 'assets', 'asset_manifest.csv')
manifest = list(csv.DictReader(open(man_path, encoding='utf-8', newline=''))) if os.path.exists(man_path) else []
if not manifest:
    fail(7, 'assets/asset_manifest.csv missing or empty')
asset_ids = {r['asset_id'] for r in manifest}
cats = sorted({a.split('.')[0] for a in asset_ids}, key=len, reverse=True)
ASSET = re.compile(r'`((?:' + '|'.join(map(re.escape, cats)) + r')\.[a-z0-9_]+(?:\.[a-z0-9_]+)*)`') if cats else None
n_asset_cites = 0
for f, t in TEXT.items():
    if not (f.startswith('reference/') or f.startswith('architecture/components/')) or not ASSET:
        continue
    for ln, line in prose_lines(t):
        for a in ASSET.findall(line):
            if a in key_defs:
                continue
            n_asset_cites += 1
            if a not in asset_ids:
                fail(7, f'{f}:{ln} cites asset {a}, which is not in the manifest')
conv = TEXT.get('CONVENTIONS.md', '')
str_prefixes, col = set(), None
for line in conv.split('\n'):
    if not line.startswith('|'):
        col = None
        continue
    cells = [c.strip() for c in line.strip().strip('|').split('|')]
    if 'String-id prefix' in cells:
        col = cells.index('String-id prefix')
    elif col is not None and col < len(cells) and not cells[0].startswith('---'):
        str_prefixes.update(re.findall(r'`([a-z_]+)(?:\.[a-z0-9_.]+)?`', cells[col]))
for c in sorted(set(cats) & str_prefixes):
    fail(7, f'asset stem "{c}." is also a string-id prefix (CONVENTIONS §8); asset ids and string ids must not share a stem')
for r in manifest:
    if not r.get('consumers', '').strip():
        fail(7, f'manifest row {r["asset_id"]} names no consumer')
info[7] = f'{len(asset_ids)} manifest rows, {n_asset_cites} asset citations in reference and component documents'


# ---------------------------------------------------------------- gate 9: truncation words
for f, t in TEXT.items():
    if f == 'CONVENTIONS.md':
        continue   # it quotes the banned words when stating the rule
    for ln, line in prose_lines(t):
        for m in TRUNC.findall(strip_code_spans(line)):
            fail(9, f'{f}:{ln} truncation word "{m}"')
info[9] = f'{len(results.get(9, []))} truncation words'


# ---------------------------------------------------------------- gate 10: sizes, READMEs, template headings
for p in glob.glob(os.path.join(ROOT, '**', '*'), recursive=True):
    r = rel(p)
    if os.path.isdir(p):
        if not os.path.exists(os.path.join(p, 'README.md')):
            fail(10, f'{r}/ has no README.md')
        continue
    if os.path.getsize(p) > 120 * 1024 and not (r in size_exempt or (r.startswith('assets/') and r.endswith('.csv'))):
        fail(10, f'{r} is {os.path.getsize(p) // 1024} KB (> 120 KB)')
n_templ = 0
for f, t in TEXT.items():
    if os.path.basename(f) == 'README.md':
        continue
    if f.startswith('reference/'):
        heads = REF_H
    elif f.startswith('architecture/components/'):
        heads = CMP_H
    else:
        continue
    n_templ += 1
    have = [m for _, l in prose_lines(t) for m in re.findall(r'^##\s+(?:\d+\s+)?(.+?)\s*$', l)]
    for h in heads:
        if not any(x.lower().startswith(h.lower()) for x in have):
            fail(10, f'{f}: template heading "{h}" missing')
info[10] = f'{len(TEXT)} documents, {n_templ} checked for template headings'


# ---------------------------------------------------------------- gate 11: requirements index
def section_of(f, ln):
    lines = TEXT[f].split('\n')
    for j in range(ln - 1, -1, -1):
        m = re.match(r'^(#{2,4})\s+(.*?)\s*$', lines[j])
        if m:
            return m.group(2)
    return ''


def runs(nums):
    nums = sorted(nums)
    out, start, prev = [], None, None
    for n in nums:
        if start is None:
            start = prev = n
        elif n == prev + 1:
            prev = n
        else:
            out.append((start, prev))
            start = prev = n
    if start is not None:
        out.append((start, prev))
    return out


def one_line(rid):
    t = req_blocks.get(rid, '')
    t = re.sub(r'^\*\*[A-Z-]+-\d{3}\*\*\s*(\((?:MUST|SHOULD|MAY)(?: NOT)?\))?\s*', '', t)
    t = re.sub(r'!?\[([^\]]*)\]\([^)]*\)', r'\1', t)
    t = re.sub(r'\s+', ' ', t.replace('**', '')).strip()
    m = re.match(r'^(.{20,140}?[.;:])(\s|$)', t)
    t = m.group(1) if m else t
    if len(t) > 140:
        t = t[:140].rsplit(' ', 1)[0]
    return t.replace('|', '\\|')


if A.write_index:
    idx_dir = os.path.join(ROOT, 'requirements_index')
    os.makedirs(idx_dir, exist_ok=True)
    for old in glob.glob(os.path.join(idx_dir, '*.md')):
        os.remove(old)
    scope = {}
    for m in re.finditer(r'^\|\s*`([A-Z][A-Z-]*[A-Z])`\s*\|\s*([^|]+?)\s*\|', TEXT.get('CONVENTIONS.md', ''), re.M):
        scope[m.group(1)] = m.group(2)
    by_p = defaultdict(list)
    for rid, where in defs.items():
        by_p[rid.rsplit('-', 1)[0]].append(rid)
    pages = []   # (prefix, page file, first id, last id, count)
    for p in PREFIX:
        rows = []
        for rid in sorted(by_p.get(p, []), key=lambda r: int(r.rsplit('-', 1)[1])):
            f, ln = defs[rid][0]
            sec = section_of(f, ln)
            rows.append((rid, f'| {rid} | {one_line(rid)} | [{os.path.basename(f)}](../{f}#{slug(sec)}) |'))
        head = ['| Id | Requirement (first sentence) | Defined in |', '|---|---|---|']
        chunks, cur, size = [], [], 0
        for rid, row in rows:
            if size + len(row.encode()) > 100 * 1024 and cur:
                chunks.append(cur)
                cur, size = [], 0
            cur.append((rid, row))
            size += len(row.encode()) + 1
        if cur:
            chunks.append(cur)
        for k, chunk in enumerate(chunks, 1):
            name = p.lower() + (f'_{k}' if len(chunks) > 1 else '') + '.md'
            body = [f'# {p} requirements' + (f' (part {k} of {len(chunks)})' if len(chunks) > 1 else ''), '',
                    '> Generated by `check_design.py --write-index`. Do not edit by hand.', '',
                    f'Scope: {scope.get(p, "")}. Back to the [requirements index](../REQUIREMENTS_INDEX.md).', ''] \
                + head + [r for _, r in chunk]
            open(os.path.join(idx_dir, name), 'w', encoding='utf-8').write('\n'.join(body) + '\n')
            pages.append((p, name, chunk[0][0], chunk[-1][0], len(chunk)))
    out = ['# Requirements index', '',
           '> Generated by `check_design.py --write-index`. Do not edit by hand.', '',
           f'{len(defs)} requirements under {len(PREFIX)} prefixes. Every requirement id maps to the document and '
           'section that defines it, with its first sentence. Ids are stable and never reused.', '',
           '| Prefix | Scope | Requirements | Index page |', '|---|---|---|---|']
    for p, name, a, b, n in pages:
        out.append(f'| `{p}` | {scope.get(p, "")} | {n} | [{a} to {b}](requirements_index/{name}) |')
    open(os.path.join(ROOT, 'REQUIREMENTS_INDEX.md'), 'w', encoding='utf-8').write('\n'.join(out) + '\n')
    open(os.path.join(idx_dir, 'README.md'), 'w', encoding='utf-8').write(
        '# Requirements index pages\n\nGenerated pages, one per requirement prefix. Start from the '
        '[requirements index](../REQUIREMENTS_INDEX.md).\n')
    big = max(os.path.getsize(os.path.join(idx_dir, n)) for _, n, *_ in pages) // 1024 if pages else 0
    info[11] = f'REQUIREMENTS_INDEX.md and {len(pages)} index pages written (largest {big} KB)'
else:
    info[11] = 'index not written (no --write-index)'


# ---------------------------------------------------------------- report
names = {1: 'Hard-fail patterns', 2: 'Soft-flag patterns', 3: 'Links and anchors', 4: 'Mermaid',
         5: 'Requirement ids', 6: 'DEC and ADR references', 7: 'Asset closure', 8: 'Tuning keys',
         9: 'Truncation words', 10: 'Sizes, READMEs, templates', 11: 'Requirements index'}
rep = ['# Gate report', '', '| Gate | Result | Failures | Summary |', '|---|---|---|---|']
for g in range(1, 12):
    n = len(results.get(g, []))
    rep.append(f'| {g} {names[g]} | {"PASS" if not n else "FAIL"} | {n} | {info.get(g, "")} |')
for g in range(1, 12):
    if results.get(g):
        rep += ['', f'## Gate {g} {names[g]}', ''] + [f'- {x}' for x in results[g]]
text = '\n'.join(rep) + '\n'
if A.report:
    open(A.report, 'w', encoding='utf-8').write(text)
for g in range(1, 12):
    n = len(results.get(g, []))
    print(f'gate {g:2} {names[g]:28} {"PASS" if not n else "FAIL"} {n:5}  {info.get(g, "")}')
sys.exit(1 if any(results.values()) else 0)
