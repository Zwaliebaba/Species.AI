"""X-AST-2b helper: requirement-level consumers for every asset id of the manifest.

Usage:
  python3 -I X-AST-2b_consumers.py <design_root> <manifest_csv> <out_detail_csv> [--apply]

For every citation of an asset id in reference/ and architecture/components/ (backticked ids in prose, as gate 7
counts them; bare ids inside fenced blocks; wildcards with at least two literal segments such as `icon.power.*`;
placeholder patterns such as `icon.power.<kind>`), the consumer is chosen in this order:
  a. the requirement whose block holds the citation (block = from its **ID** to the next blank line, definition or
     heading, as CheckDesign.py reads it);
  b1. the nearest requirement of the smallest enclosing section (the last one defined before the citation, else the
      first one after it), together with
  c. the defined requirement ids that the line ties to the citation: every id of a table row (a row that names
     its rule, a row of tests); in other lines the ids of the same clause before the citation and those in a
     parenthesis right after it; a range of at most 10 ids is expanded; c alone when the smallest section holds no
     requirement;
  b2. otherwise the nearest requirement of an enclosing section, walking up to the level-2 section;
  d. the document path, kept only when no requirement of that document consumes the asset.
Consumers are written sorted: reference requirement ids, then component ids, then document paths.
--apply rewrites the manifest's consumers column in place; otherwise only the detail CSV is written.
"""
import csv, glob, io, os, re, sys
from collections import defaultdict

root, man_path, detail_path = sys.argv[1], sys.argv[2], sys.argv[3]
apply = '--apply' in sys.argv[4:]

FENCE = re.compile(r'^[ \t]*(```|~~~)')
HEAD = re.compile(r'^(#{1,6})\s+(.*?)\s*#*\s*$')

conv = open(os.path.join(root, 'CONVENTIONS.md'), encoding='utf-8').read()
sec = re.search(r'^### 3\.1 Prefix registry\n(.*?)(?=^##)', conv, re.M | re.S)
prefixes = re.findall(r'^\|\s*`([A-Z][A-Z-]*[A-Z])`\s*\|', sec.group(1), re.M)
pfx = '|'.join(sorted((re.escape(p) for p in prefixes), key=len, reverse=True))
def_rx = re.compile(r'\*\*(' + pfx + r')-(\d{3})\*\*')
cite_rx = re.compile(r'(?<![\w-])(' + pfx + r')-(\d{3})(?!\d)')
range_rx = re.compile(r'(?<![\w-])(' + pfx + r')-\d{3}\s*(?:to|–|—|-|…|\.\.)\s*(?:(?:' + pfx + r')-)?\d{3}(?!\d)')

with open(man_path, encoding='utf-8', newline='') as fh:
    reader = csv.DictReader(fh)
    fields = reader.fieldnames
    rows = list(reader)
ids = sorted({r['asset_id'] for r in rows})
idset = set(ids)
stems = sorted({a.split('.')[0] for a in ids}, key=len, reverse=True)
stem_alt = '|'.join(map(re.escape, stems))
tick_rx = re.compile(r'`((?:' + stem_alt + r')\.[a-z0-9_]+(?:\.[a-z0-9_]+)*)`')
pat_rx = re.compile(r'`((?:' + stem_alt + r')(?:\.(?:[a-z0-9_]+|\*|<[a-z_]+>))+)`')
bare_rx = re.compile(r'(?<![\w.])(' + '|'.join(sorted(map(re.escape, ids), key=len, reverse=True)) + r')(?![\w])(?!\.[a-z0-9_])')

# tuning keys (gate 7 skips a backticked id that is a tuning key)
keys = set()
files = sorted(glob.glob(os.path.join(root, 'reference', '**', '*.md'), recursive=True)
               + glob.glob(os.path.join(root, 'architecture', 'components', '**', '*.md'), recursive=True))
texts = {os.path.relpath(p, root).replace(os.sep, '/'): open(p, encoding='utf-8').read() for p in files}
for t in texts.values():
    hdr = None
    for line in t.split('\n'):
        if not line.startswith('|'):
            hdr = None
            continue
        cells = [c.strip() for c in line.strip().strip('|').split('|')]
        if hdr is None:
            hdr = cells[0].lower()
            continue
        if hdr == 'key':
            m = re.match(r'`([a-z0-9_.]+)`$', cells[0])
            if m:
                keys.add(m.group(1))


def expand(pattern):
    """Ids a wildcard or placeholder pattern names; None when the pattern is stem-wide or names nothing."""
    segs = pattern.split('.')
    literal = 0
    for s in segs:
        if s == '*' or s.startswith('<'):
            break
        literal += 1
    if '*' in segs and literal < 2:
        return None          # stem-wide wildcard such as `sfx.*`: a statement about a category, not a citation
    rx = '\\.'.join('[a-z0-9_]+(?:\\.[a-z0-9_]+)*' if s == '*' else '[a-z0-9_]+' if s.startswith('<') else re.escape(s)
                    for s in segs)
    hit = [a for a in ids if re.fullmatch(rx, a)]
    return hit or None


detail = []                                   # (asset, file, line, kind, consumer, rule)
per_asset = defaultdict(lambda: defaultdict(set))   # asset -> file -> set of consumers (ids, or '' for document)
defined = {}
for f, t in texts.items():
    for i, line in enumerate(t.split('\n'), 1):
        for m in def_rx.finditer(line):
            defined[f'{m.group(1)}-{m.group(2)}'] = f
for f, t in texts.items():
    lines = t.split('\n')
    n = len(lines)
    inside = [False] * (n + 1)
    heads = []                                # (line, level)
    defs = []                                 # (line, id)
    fence = False
    for i, line in enumerate(lines, 1):
        if FENCE.match(line):
            fence = not fence
            inside[i] = True
            continue
        inside[i] = fence
        if fence:
            continue
        h = HEAD.match(line)
        if h:
            heads.append((i, len(h.group(1))))
        for m in def_rx.finditer(line):
            defs.append((i, f'{m.group(1)}-{m.group(2)}'))
    # requirement blocks
    block_of = {}
    for ln, rid in defs:
        block_of[ln] = rid
        for j in range(ln + 1, min(ln + 40, n) + 1):
            nxt = lines[j - 1]
            if not nxt.strip() or def_rx.search(nxt) or nxt.startswith('#') or FENCE.match(nxt):
                break
            block_of[j] = rid

    def section(ln, level_cap):
        """(start, end) of the enclosing section of heading level >= level_cap whose level is deepest."""
        out = []
        for idx, (hl, lev) in enumerate(heads):
            if hl > ln:
                break
            end = n + 1
            for hl2, lev2 in heads[idx + 1:]:
                if lev2 <= lev:
                    end = hl2
                    break
            if end > ln:
                out.append((lev, hl, end))
        return sorted(out, reverse=True)      # deepest first

    def nearest(ln, start, end):
        before = [rid for dl, rid in defs if start <= dl < ln]
        if before:
            return before[-1]
        after = [rid for dl, rid in defs if ln < dl < end]
        return after[0] if after else None

    range_full = re.compile(r'(?<![\w-])(' + pfx + r')-(\d{3})\s*(?:to|–|—|-|…|\.\.)\s*(?:(' + pfx + r')-)?(\d{3})(?!\d)')

    def ids_in(text):
        """Defined requirement ids cited in text, in order; a range of at most 10 ids of one prefix is expanded."""
        out = []
        for m in range_full.finditer(text):
            p1, a_, p2, b_ = m.group(1), int(m.group(2)), m.group(3) or m.group(1), int(m.group(4))
            if p1 == p2 and 0 < b_ - a_ < 10:
                out += [f'{p1}-{k:03d}' for k in range(a_, b_ + 1)]
        for p_, num in cite_rx.findall(range_full.sub(' ', text)):
            out.append(f'{p_}-{num}')
        seen = []
        for r_ in out:
            if r_ in defined and r_ not in seen:
                seen.append(r_)
        return seen

    def line_cited(line, pos):
        """Requirement ids that a line ties to the citation at pos: every id of a table row; in other lines, the ids
        of the same clause (between semicolons or sentence ends) that come before the citation, and the ids in a
        parenthesis that opens right after it."""
        if line.lstrip().startswith('|'):
            return ids_in(line)
        cuts = [0] + [m.end() for m in re.finditer(r';\s|\.\s(?=[A-Z`(\[*])', line)] + [len(line) + 1]
        start = max(c for c in cuts if c <= pos)
        end = min(c for c in cuts if c > pos)
        before = line[start:pos]
        after = line[pos:end]
        m = re.match(r'`[^`]*`\s*\(([^()]*)\)', after)
        return ids_in(before + ' ' + (m.group(1) if m else ''))

    def attribute(ln, line, pos=0):
        if ln in block_of:
            return [block_of[ln]], 'a'
        secs = section(ln, 2)
        near = None
        if secs:
            lev, s, e = secs[0]
            near = nearest(ln, s, e)
        cited = line_cited(line, pos)
        if near and cited:
            return [near] + [c for c in cited if c != near], 'b1+c'
        if near:
            return [near], 'b1'
        if cited:
            return cited, 'c'
        for lev, s, e in secs[1:]:
            if lev < 2:
                break
            r = nearest(ln, s, e)
            if r:
                return [r], 'b2'
        return [], 'd'

    for i, line in enumerate(lines, 1):
        found = []
        if inside[i]:
            if FENCE.match(line):
                continue
            for m in bare_rx.finditer(line):
                found.append((m.group(1), 'code', m.start()))
        else:
            for m in tick_rx.finditer(line):
                a = m.group(1)
                if a in keys:
                    continue
                found.append((a, 'id', m.start()))
            for m in pat_rx.finditer(line):
                p = m.group(1)
                if '*' in p or '<' in p:
                    hit = expand(p)
                    if hit:
                        for a in hit:
                            found.append((a, 'pattern ' + p, m.start()))
        if not found:
            continue
        for a, kind, pos in found:
            cons, rule = attribute(i, line, pos)
            if a not in idset:
                detail.append((a, f, i, kind, 'MISSING FROM MANIFEST', rule))
                continue
            if cons:
                for c in cons:
                    per_asset[a][f].add(c)
                    detail.append((a, f, i, kind, c, rule))
            else:
                per_asset[a][f].add('')
                detail.append((a, f, i, kind, f, rule))


def order_key(c):
    if '/' in c:
        return (2, c, 0)
    p, num = c.rsplit('-', 1)
    return (0 if p.startswith('R-') else 1, p, int(num))


consumers = {}
for a in ids:
    out = set()
    for f, cs in per_asset.get(a, {}).items():
        reqs = {c for c in cs if c}
        if reqs:
            out |= reqs
        else:
            out.add(f)
    consumers[a] = sorted(out, key=order_key)

with open(detail_path, 'w', encoding='utf-8', newline='') as fh:
    w = csv.writer(fh, lineterminator='\n')
    w.writerow(['asset_id', 'file', 'line', 'kind', 'consumer', 'rule'])
    for d in sorted(detail):
        w.writerow(d)

uncited = [a for a in ids if not consumers[a]]
missing = sorted({(d[0], d[1], d[2]) for d in detail if d[4] == 'MISSING FROM MANIFEST'})
docs_only = [a for a in ids if consumers[a] and all('/' in c for c in consumers[a])]
from collections import Counter
rules = Counter(d[5] for d in detail if d[4] != 'MISSING FROM MANIFEST')
print('asset ids', len(ids), 'rows', len(rows))
print('citations attributed', sum(rules.values()), dict(rules))
print('ids with no consumer', len(uncited), uncited)
print('ids consumed only by document paths', len(docs_only))
for a in docs_only:
    print('  ', a, consumers[a])
print('cited but missing', len(missing))
for m in missing:
    print('  ', m)
doc_fallbacks = sum(1 for a in ids for c in consumers[a] if '/' in c)
print('document-path consumers remaining', doc_fallbacks)

if apply:
    for r in rows:
        r['consumers'] = ';'.join(consumers[r['asset_id']])
    buf = io.StringIO()
    w = csv.DictWriter(buf, fieldnames=fields, lineterminator='\n')
    w.writeheader()
    for r in rows:
        w.writerow(r)
    with open(man_path, 'w', encoding='utf-8', newline='') as fh:
        fh.write(buf.getvalue())
    print('manifest written', man_path)
