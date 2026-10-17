"""X-AST-2b helper: manifest row edits and the origin map (run before X-AST-2b_consumers.py --apply).

Usage: python3 -I X-AST-2b_apply.py <manifest_csv> <origin_map_csv>

1. Streamed audio rows: the runtime format is MS-ADPCM decoded by XAudio2 (ADR-025, AEN-072, AST-080), never a
   third-party codec (residual 32).
2. Rows that nothing consumes are removed (task 3): uitex.logo_banner. Its origin-map row becomes a "not created" row.
3. The land colour ramp and the shore-wave gradient that no match map names (R-MAP-086, residual A-36) say so in their
   role and move to post-launch.
The script is idempotent: each edit checks the text it replaces.
"""
import csv, io, sys

man_path, origin_path = sys.argv[1], sys.argv[2]


def load(p):
    with open(p, encoding='utf-8', newline='') as fh:
        r = csv.DictReader(fh)
        return r.fieldnames, list(r)


def save(p, fields, rows):
    buf = io.StringIO()
    w = csv.DictWriter(buf, fieldnames=fields, lineterminator='\n')
    w.writeheader()
    for r in rows:
        w.writerow(r)
    with open(p, 'w', encoding='utf-8', newline='') as fh:
        fh.write(buf.getvalue())


fields, rows = load(man_path)
OLD_STREAM = 'Opus stream in the pack (decoded on a worker thread)'
NEW_STREAM = 'MS-ADPCM stream in the pack, 48 kHz, decoded by XAudio2'
n_stream = 0
for r in rows:
    if r['target_format'] == OLD_STREAM:
        r['target_format'] = NEW_STREAM
        n_stream += 1
assert all('opus' not in v.lower() for r in rows for v in r.values()), 'a third-party codec is still named'

REMOVE = {'uitex.logo_banner'}
removed = [r for r in rows if r['asset_id'] in REMOVE]
rows = [r for r in rows if r['asset_id'] not in REMOVE]

UNUSED = {
    'content/textures/terrain/terrain_ramp_14.png':
        'Terrain colour ramp: colour by slope (x) and height (y), read on the CPU when the terrain mesh is built; '
        "no match map names file 14; the map editor's terrain panel lists it (R-MAP-086, R-MAP-325)",
    'content/textures/terrain/shore_wave_gradient_08.png':
        'Shore-wave colour gradient: one row read on the CPU by the sea renderer; no match map names file 8; the map '
        "editor's terrain panel lists it (R-MAP-086, R-MAP-325)",
}
n_unused = 0
for r in rows:
    if r['species_filename'] in UNUSED:
        r['role'] = UNUSED[r['species_filename']]
        r['priority'] = 'post-launch'
        n_unused += 1
assert n_unused == 2, n_unused

# 4. The two research-level icons: R-CRATE-014 now settles their role (drawn by no rule of a match, DEC-115).
LEVEL = {'icon.power.laser': 'Research-level icon of the laser level; drawn by no rule of a match (R-CRATE-014, DEC-115)',
         'icon.power.slots_upgrade': 'Research-level icon of the Power-slots level; drawn by no rule of a match '
                                     '(R-CRATE-014, DEC-115)'}
for r in rows:
    if r['asset_id'] in LEVEL:
        r['role'] = LEVEL[r['asset_id']]
        r['spec'] = r['spec'].replace('; role to be confirmed against reference/04', '')
save(man_path, fields, rows)

ofields, orows = load(origin_path)
gone = {r['species_filename'] for r in removed}
n_origin = 0
for o in orows:
    if o['species_filename'] in gone:
        o['species_filename'] = '(not created: drawn by no rule)'
        o['asset_id'] = ''
        n_origin += 1
save(origin_path, ofields, orows)
print('stream rows', n_stream, 'removed rows', len(removed), [r['asset_id'] for r in removed],
      'terrain rows', n_unused, 'origin rows', n_origin, 'manifest rows now', len(rows))
