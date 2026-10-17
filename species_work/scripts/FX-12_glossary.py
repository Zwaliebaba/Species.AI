"""FX-12 helper: add, replace and relink glossary rows.
Usage: python3 -I FX-12_glossary.py <glossary_dir> <spec.json>
spec.json: a list of operations, each a dict:
  {"op": "add", "term", "kind", "def", "owners"}       insert in sorted position (file and letter by term)
  {"op": "set", "term", "kind"?, "def"?, "owners"?}    replace cells of the row whose Term cell equals term
  {"op": "relink", "term", "old", "new"}               replace a substring of that row's Owning-document cell
Sort key: lower case, bracketed qualifiers dropped, hyphen read as a space, word by word.
Fails (exit 1) without writing when a term to add exists or a term to change is missing."""
import json, os, re, sys
gdir, spec_path = sys.argv[1], sys.argv[2]
files = {'a': os.path.join(gdir, 'a_to_l.md'), 'm': os.path.join(gdir, 'm_to_z.md')}
text = {k: open(p, encoding='utf-8').read().split('\n') for k, p in files.items()}
errors = []


def key(term):
    t = re.sub(r'\([^)]*\)', ' ', term.lower()).replace('-', ' ').replace('`', '')
    return t.split()


def cells(line):
    return [c.strip() for c in line.strip().strip('|').split(' | ')]


def row(term, kind, d, owners):
    for v in (term, kind, d, owners):
        if '\n' in v or ' | ' in v:
            errors.append(f'bad cell in {term!r}')
    return f'| {term} | {kind} | {d} | {owners} |'


def find(term):
    for k, lines in text.items():
        for i, line in enumerate(lines):
            if line.startswith('| ') and cells(line)[0] == term:
                return k, i
    return None, None


def section(term):
    first = re.sub(r'[^0-9A-Za-z]', '', term)[:1].upper()
    if first.isdigit():
        return 'a', '## Numbers'
    return ('a' if first <= 'L' else 'm'), f'## {first}'


for op in json.load(open(spec_path, encoding='utf-8')):
    term = op['term']
    k, i = find(term)
    if op['op'] == 'add':
        if k:
            errors.append(f'add {term!r}: exists in {k}'); continue
        f, head = section(term)
        lines = text[f]
        if head not in lines:
            errors.append(f'add {term!r}: no section {head}'); continue
        h = lines.index(head)
        j = h + 1
        while not lines[j].startswith('|'):
            j += 1
        j += 2
        end = j
        while end < len(lines) and lines[end].startswith('|'):
            end += 1
        pos = next((n for n in range(j, end) if key(cells(lines[n])[0]) > key(term)), end)
        lines.insert(pos, row(term, op['kind'], op['def'], op['owners']))
    elif op['op'] == 'set':
        if not k:
            errors.append(f'set {term!r}: missing'); continue
        c = cells(text[k][i])
        text[k][i] = row(term, op.get('kind', c[1]), op.get('def', c[2]), op.get('owners', c[3]))
    elif op['op'] == 'relink':
        if not k:
            errors.append(f'relink {term!r}: missing'); continue
        c = cells(text[k][i])
        if op['old'] not in c[3]:
            errors.append(f'relink {term!r}: old link not in its owners'); continue
        text[k][i] = row(c[0], c[1], c[2], c[3].replace(op['old'], op['new']))
    else:
        errors.append(f'unknown op {op}')
if errors:
    print('\n'.join(errors)); sys.exit(1)
for k, p in files.items():
    open(p, 'w', encoding='utf-8').write('\n'.join(text[k]))
    print(p, os.path.getsize(p))
