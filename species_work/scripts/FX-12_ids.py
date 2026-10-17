"""FX-12 helper: inventory of dotted-id roots per kind across the design root.
Usage: python3 -I FX-12_ids.py <design_root>
Prints tuning-key roots (and ui.* key topics), string-id roots seen in prose, schema ids,
setting keys, action-id groups and profiling-marker roots, as far as their tables can be found."""
import collections, glob, os, re, sys
ROOT = os.path.abspath(sys.argv[1])
MD = sorted(glob.glob(os.path.join(ROOT, '**', '*.md'), recursive=True))
TEXT = {os.path.relpath(p, ROOT): open(p, encoding='utf-8').read() for p in MD}

def cells_of(line):
    return [c.strip() for c in re.split(r'(?<!\\)\|', line.strip().strip('|'))]

keys = {}
for f, t in TEXT.items():
    lines = t.split('\n'); cols = None
    for i, line in enumerate(lines):
        if line.startswith('|') and i + 1 < len(lines) and re.match(r'^\|\s*:?-{3,}', lines[i + 1]):
            head = [c.lower() for c in cells_of(line)]
            cols = head if head[:2] == ['key', 'value'] else None
            continue
        if cols is None: continue
        if not line.startswith('|'): cols = None; continue
        if re.match(r'^\|\s*:?-{3,}', line): continue
        m = re.match(r'^`([^`]+)`$', cells_of(line)[0])
        if m: keys[m.group(1)] = f
roots = collections.Counter(k.split('.')[0] for k in keys)
print('TUNING-KEY ROOTS', dict(roots))
ui_topics = collections.Counter(k.split('.')[1] for k in keys if k.startswith('ui.'))
print('UI KEY TOPICS', sorted(ui_topics))
# dotted ids in code spans
ids = collections.defaultdict(set)
for f, t in TEXT.items():
    for tok in re.findall(r'`([a-z][a-z0-9_]*(?:\.[a-z0-9_<>*]+)+)`', t):
        ids[tok].add(f)
nonkey = {i: fs for i, fs in ids.items() if i not in keys and not i.startswith('ref.')}
r2 = collections.Counter(i.split('.')[0] for i in nonkey)
print('NON-KEY DOTTED ROOTS', sorted(r2.items(), key=lambda x: -x[1]))
ui_str = sorted({i.split('.')[1] for i in nonkey if i.startswith('ui.') and len(i.split('.')) > 2})
print('UI NON-KEY TOPICS', ui_str)
print('UI TOPIC OVERLAP', sorted(set(ui_str) & set(ui_topics)))
both = sorted(set(nonkey) & set(keys))
for i in sorted(nonkey):
    if i.split('.')[0] in sys.argv[2:]:
        print('  ', i, sorted(nonkey[i])[:2])

# ---- kind sets and cross-kind clashes (setting keys, schema ids, markers, action ids vs tuning keys)
def table_after(text, caption_start):
    i = text.find(caption_start)
    if i < 0: return []
    rows = []
    for line in text[i:].split('\n')[1:]:
        if line.startswith('|'): rows.append(cells_of(line))
        elif rows: break
    return rows[2:]
dat = TEXT.get('architecture/components/data_schemas/config_input_and_strings.md', '')
settings = set()
for r in table_after(dat, 'Table 3.1-A Settings'):
    settings.update(re.findall(r'`([a-z][a-z0-9_]*(?:\.[a-z0-9_<>]+)+)`', r[1]))
readme = TEXT.get('architecture/components/data_schemas/README.md', '')
schemas = set()
for r in table_after(readme, 'Table 1-A Record-type registry'):
    schemas.update(re.findall(r'`([a-z][a-z0-9_]*(?:\.[a-z0-9_]+)+)`', r[0]))
dbg = TEXT.get('architecture/components/diagnostics_test_and_performance.md', '')
markers = set()
i = dbg.find('### 3.4 Profile marker catalogue')
for line in dbg[i:].split('\n')[1:]:
    if line.startswith('### '): break
    if line.startswith('| ') and not line.startswith('| Area'):
        markers.update(re.findall(r'`([a-z][a-z0-9_]*(?:\.[a-z0-9_]+)+)`', cells_of(line)[1]))
actions = {i for i in ids if i.startswith('action.')}
print('SETTINGS', len(settings), sorted({s.split('.')[0] for s in settings}))
print('SCHEMAS', len(schemas), sorted({s.split('.')[0] for s in schemas}))
print('MARKERS', len(markers), sorted({s.split('.')[0] for s in markers}))
print('ACTION GROUPS', sorted({a.split('.')[1] for a in actions if len(a.split('.')) > 2}))
kinds = {'tuning': set(keys), 'setting': settings, 'schema': schemas, 'marker': markers, 'action': actions}
names = list(kinds)
for a in range(len(names)):
    for b in range(a + 1, len(names)):
        x = kinds[names[a]] & kinds[names[b]]
        if x: print('CLASH', names[a], names[b], sorted(x))
other = set(nonkey) - settings - schemas - markers - actions
for k in ('setting', 'schema', 'marker'):
    x = kinds[k] & other
    print('ALSO-ELSEWHERE', k, sorted(x)[:20])
