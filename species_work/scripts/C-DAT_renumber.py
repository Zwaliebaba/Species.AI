"""Renumber DAT ids contiguously within each file's block. Usage: python3 -I C-DAT_renumber.py <folder>"""
import os, re, sys
folder = sys.argv[1]
order = [('README.md', 1), ('map_schema.md', 70), ('map_structure_records.md', 140),
         ('config_input_and_strings.md', 190), ('entity_audio_geometry_and_panel_schemas.md', 260),
         ('import_and_build_records.md', 350)]
texts = {f: open(os.path.join(folder, f), encoding='utf-8').read() for f, _ in order}
mapping = {}
ranges = {}
for f, start in order:
    ids = [int(i) for i in re.findall(r'\*\*DAT-(\d{3})\*\*', texts[f])]
    for k, old in enumerate(ids):
        mapping[old] = start + k
    ranges[f] = (start, start + len(ids) - 1)
def sub(m):
    n = int(m.group(1))
    return 'DAT-%03d' % mapping.get(n, n)
for f, _ in order:
    s = re.sub(r'DAT-(\d{3})', sub, texts[f])
    open(os.path.join(folder, f), 'w', encoding='utf-8').write(s)
for f, (a, b) in ranges.items():
    print(f, 'DAT-%03d to DAT-%03d' % (a, b))
changed = {k: v for k, v in mapping.items() if k != v}
print(len(changed), 'ids renumbered')
