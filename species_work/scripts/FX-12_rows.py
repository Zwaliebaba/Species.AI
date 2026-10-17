"""FX-12 helper: replace and insert rows of one Markdown table in sorted order.
Usage: python3 -I FX-12_rows.py <file> <section heading line> <rows.tsv>
rows.tsv: one row per line, TAB-separated: op<TAB>term<TAB>row text
  op = add (insert a new row; fails if the term exists) | set (replace the row whose first cell is term)
Sort key: lower case, bracketed qualifiers dropped, hyphen read as a space, compared word by word."""
import re, sys
path, heading, rows_path = sys.argv[1:4]
text = open(path, encoding='utf-8').read()
lines = text.split('\n')
h = lines.index(heading)
i = h + 1
while not lines[i].startswith('|'):
    i += 1
first = i + 2                      # first data row (after header and rule)
last = first
while last < len(lines) and lines[last].startswith('|'):
    last += 1


def key(term):
    t = re.sub(r'\([^)]*\)', ' ', term.lower()).replace('-', ' ').replace('`', '')
    return t.split()


def term_of(row):
    return row.strip().strip('|').split(' | ')[0].strip()


body = lines[first:last]
for n, raw in enumerate(open(rows_path, encoding='utf-8').read().splitlines(), 1):
    if not raw.strip():
        continue
    op, term, row = raw.split('\t')
    terms = [term_of(r) for r in body]
    if op == 'set':
        hits = [j for j, t in enumerate(terms) if t == term]
        if len(hits) != 1:
            sys.exit(f'line {n}: set {term!r}: {len(hits)} rows')
        body[hits[0]] = row
    elif op == 'add':
        if term in terms:
            sys.exit(f'line {n}: add {term!r}: already present')
        k = key(term)
        pos = next((j for j, t in enumerate(terms) if key(t) > k), len(body))
        body.insert(pos, row)
    else:
        sys.exit(f'line {n}: unknown op {op}')
lines[first:last] = body
open(path, 'w', encoding='utf-8').write('\n'.join(lines))
print(f'{path}: {len(body)} rows under {heading!r}')
