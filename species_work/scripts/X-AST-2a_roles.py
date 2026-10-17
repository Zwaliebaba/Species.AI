"""X-AST-2a: settle the pass-1 open roles that the documents now state (run once).
Usage: python3 -I X-AST-2a_roles.py --design design_next"""
import argparse, csv, os, sys
ap = argparse.ArgumentParser(); ap.add_argument('--design', required=True); a = ap.parse_args()
AD = os.path.join(a.design, 'assets')
ROLE = {
    'sprite.worker.state_overlay': ('Overlay sprite drawn above a Worker in an altered control state',
                                    'Overlay card drawn over a Worker while it has a Convert link (R-UNIT-147)'),
    'mesh.structure.turret_barrel': ('turret barrel set per turret type: gun, rocket, flame, and the shared cannon barrel',
                                     'turret barrel set per turret type, in member order: gun barrel (gun and mortar turrets), '
                                     'rocket barrel (rocket turret), flame barrel (flame turret), cannon barrel (Convert Turret)'),
    'mesh.structure.bot_nav_node': ('bot navigation node marker',
                                    'bot navigation node marker, drawn and picked in the map editor only'),
    'font.ui.primary': ('Primary UI face, Latin and Cyrillic scripts (the reference face that also carries Japanese and Korean)',
                        'Title face (`FontRole::Title`: front-end captions and titles, UI-195), Latin and Cyrillic scripts, with its Japanese and Korean companions'),
    'font.ui.stylised': ('Stylised display face (the second reference face)',
                         'Caption face (`FontRole::Caption`: tool-window titles, the session-browser header, player names in rows, overlays, UI-195)'),
    'font.ui.compact': ('Compact face for dense panels (the third reference face)',
                        'Body face (`FontRole::Body`: button captions, field values, list rows, the chat, scrolling text, message dialogs, UI-195)'),
}
SPEC = {'sprite.worker.state_overlay': ('64x64 px; drawn over the Worker card; role to be confirmed against reference/02_units/worker.md',
                                        '64x64 px; drawn over the Worker card')}
err = []
p = os.path.join(AD, 'asset_manifest.csv')
rows = list(csv.DictReader(open(p, encoding='utf-8', newline='')))
cols = list(rows[0])
for r in rows:
    if r['asset_id'] in ROLE:
        o, n = ROLE[r['asset_id']]
        if r['role'] == o: r['role'] = n
        elif r['role'] != n: err.append('manifest role ' + r['asset_id'])
    if r['asset_id'] in SPEC:
        o, n = SPEC[r['asset_id']]
        if r['spec'] == o: r['spec'] = n
        elif r['spec'] != n: err.append('manifest spec ' + r['asset_id'])
g = os.path.join(AD, 'GameAssets.md')
lines = open(g, encoding='utf-8').read().split('\n')
for aid, (o, n) in list(ROLE.items()) + list(SPEC.items()):
    hit = [i for i, l in enumerate(lines) if l.startswith('|') and '| `%s` |' % aid in l and '| %s |' % o in l]
    if not hit and not any('| %s |' % n in l for l in lines):
        err.append('GameAssets ' + aid)
    for i in hit:
        lines[i] = lines[i].replace('| %s |' % o, '| %s |' % n)
if err:
    print(err, file=sys.stderr); sys.exit(1)
with open(p, 'w', encoding='utf-8', newline='') as fh:
    w = csv.DictWriter(fh, fieldnames=cols, lineterminator='\n'); w.writeheader(); w.writerows(rows)
open(g, 'w', encoding='utf-8').write('\n'.join(lines))
print('ok')
