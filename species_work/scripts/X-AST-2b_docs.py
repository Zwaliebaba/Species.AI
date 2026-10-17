"""X-AST-2b: aligns GameAssets.md, README.md and art_and_audio_briefs.md with the pass-2b manifest.

Usage: python3 -I X-AST-2b_docs.py <design_root> <origin_map_csv>
Run after X-AST-2b_apply.py and X-AST-2b_consumers.py --apply. Every replacement states how often its old text must
occur; the script writes nothing when one does not match. The summary tables and GameAssets §18.1 are regenerated from
the manifest, the origin map and the documents, so a second run changes nothing.
"""
import collections, csv, os, re, sys

root, origin = sys.argv[1], sys.argv[2]
AD = os.path.join(root, 'assets')
man = list(csv.DictReader(open(os.path.join(AD, 'asset_manifest.csv'), encoding='utf-8', newline='')))
ERR = []


def rep(text, old, new, n=1, where=''):
    if old == new:
        return text
    c, c_new = text.count(old), text.count(new)
    if (old in new and c_new >= n) or (c == 0 and c_new >= 1):
        return text                                  # already applied
    if c != n:
        ERR.append('%s: expected %d of %r, found %d' % (where, n, old[:100], c))
        return text
    return text.replace(old, new)


def table_end(lines, i):
    while i < len(lines) and lines[i].startswith('|'):
        i += 1
    return i


# ------------------------------------------------------------------ counts from the manifest and the origin map
CATS = ['Unit sprites', 'VFX textures', 'UI panels and title', 'HUD, supply-crate and power icons', 'Cursors and markers',
        'Input glyphs', 'Terrain ramps, water and waves', 'Meshes', 'Help images and explainer animations',
        'Audio by source and event', 'UI sounds', 'Ambience', 'Music', 'Fonts', 'String table', 'Maps and thumbnails']
ANCH = {c: '%d-%s' % (i + 4, re.sub(r'[^a-z0-9 -]', '', c.lower()).replace(' ', '-')) for i, c in enumerate(CATS)}
cat_of = {r['species_filename']: r['category'] for r in man}
orig = collections.Counter()
excluded_files = 0
for o in csv.DictReader(open(origin, encoding='utf-8', newline='')):
    f = o['species_filename']
    if f in cat_of and o['original_count'].isdigit() and o['original_status'] != 'player-facing':
        orig[cat_of[f]] += int(o['original_count'])
    elif f.startswith('(not created') and o['original_status'] not in ('player-facing', 'string'):
        excluded_files += int(o['original_count']) if o['original_count'].isdigit() else 1
ids = collections.defaultdict(set)
files = collections.Counter()
pr = collections.defaultdict(collections.Counter)
for r in man:
    ids[r['category']].add(r['asset_id'])
    files[r['category']] += 1
    pr[r['category']][r['priority']] += 1
assert set(files) == set(CATS), set(files) ^ set(CATS)
summ = ['| Category | Asset ids | Files | first-playable | launch | post-launch | Reference files replaced |',
        '|---|---:|---:|---:|---:|---:|---:|']
T = collections.Counter()
for c in CATS:
    vals = [len(ids[c]), files[c], pr[c]['skirmish-first-playable'], pr[c]['launch'], pr[c]['post-launch'], orig[c]]
    T.update(dict(zip('abcdef', vals)))
    summ.append('| [%s](#%s) | %d | %d | %d | %d | %d | %d |' % (c, ANCH[c], *vals))
summ.append('| **Total** | **%d** | **%d** | **%d** | **%d** | **%d** | **%d** |' % tuple(T[k] for k in 'abcdef'))


def replace_table(text, heading, header_prefix, new_rows, where):
    lines = text.split('\n')
    s0 = lines.index(heading)
    h = next(i for i in range(s0, len(lines)) if lines[i].startswith(header_prefix))
    e = table_end(lines, h)
    return '\n'.join(lines[:h] + new_rows + lines[e:])


# ------------------------------------------------------------------ string-id census (GameAssets §18.1)
def census():
    conv = open(os.path.join(root, 'CONVENTIONS.md'), encoding='utf-8').read()
    pref, col = set(), None
    for line in conv.split('\n'):
        if not line.startswith('|'):
            col = None
            continue
        cells = [c.strip() for c in line.strip().strip('|').split('|')]
        if 'String-id prefix' in cells:
            col = cells.index('String-id prefix')
        elif col is not None and col < len(cells) and not cells[0].startswith('---'):
            pref.update(re.findall(r'`([a-z_]+(?:\.[a-z0-9_]+)*)`', cells[col]))
    roots = {p.split('.')[0] for p in pref}
    assets = {r['asset_id'] for r in man}
    stems = {a.split('.')[0] for a in assets}
    keydef, texts = set(), {}
    for dp, dn, fn in os.walk(root):
        if 'requirements_index' in dp:
            continue
        for f in fn:
            if f.endswith('.md'):
                p = os.path.join(dp, f)
                rel = os.path.relpath(p, root).replace(os.sep, '/')
                t = open(p, encoding='utf-8').read()
                texts[rel] = t
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
                            keydef.add(m.group(1))
    ROOTS = roots | {'team', 'tutorial', 'faction', 'bot', 'help', 'option', 'options'}
    ID = re.compile(r'`((?:' + '|'.join(sorted(ROOTS, key=len, reverse=True)) + r')\.[a-z0-9_]+(?:\.[a-z0-9_<>{},]+)*)`')
    found = collections.defaultdict(set)
    for rel, t in texts.items():
        if rel.startswith(('assets/', 'decisions/', 'ADR/', 'adr/', 'glossary/')) or rel == 'CONVENTIONS.md':
            continue
        for m in ID.finditer(t):
            s = m.group(1)
            if s in keydef or s.split('.')[0] in stems or s in assets:
                continue
            found[s].add(rel)
    for s_ in [x for x in found if x.startswith('camera.') and not x.startswith('camera.view')]:
        del found[s_]           # marker and sequence names, not string ids
    for x in [x for x in found if '*' in x]:
        del found[x]            # wildcards name families, not ids

    def fam(x):
        seg = x.split('.')
        return '.'.join(seg[:2]) if seg[0] in ('ui', 'match', 'mode', 'tutorial') and len(seg) > 2 else seg[0]

    fams = collections.defaultdict(set)
    for x in found:
        fams[fam(x)].add(x)

    def link(d):
        name = (os.path.relpath(d, 'reference') if d.startswith('reference/')
                else d.replace('architecture/components/', ''))[:-3]
        return '[%s](../%s)' % (name, d)

    def docs(ids_):
        c = collections.Counter(d for x in ids_ for d in found[x])
        if not c:
            return ''
        # the overview summarises the other documents, so it is the main owner only when nothing else cites the ids
        top = sorted(c.items(), key=lambda kv: (kv[0] == 'reference/00_reference_overview.md', -kv[1], kv[0]))[0][0]
        return link(top) + (' and %d more' % (len(c) - 1) if len(c) > 1 else '')

    L = ['### 18.1 String ids cited by the documents', '',
         'The reference documents and engine components cite %d string ids and id patterns (such as '
         '`ui.input.key.<name>`). Each needs new en-GB text in `strings.en_gb` and a translation in every other table, '
         'except the development captions `ui.dev.*`, which live only in `strings.dev_en_gb`. The owning document '
         'defines each id and its meaning; this table sizes the writing by family (counted mechanically from the '
         'documents).' % len(found), '',
         '| Family | Ids | Main owner |', '|---|---:|---|']
    for f in sorted(fams):
        L.append('| `%s.*` | %d | %s |' % (f, len(fams[f]), docs(fams[f])))
    TOOLTIPS = ['strike_team', 'collector', 'builder', 'transport', 'airstrike', 'missile_barrage', 'shield', 'convert',
                'speed_boost', 'turret_gun', 'swarm', 'essence_grove', 'hive', 'brood', 'meteor_shower', 'blight_grove',
                'fury', 'turret_rocket', 'turret_flame', 'firebomb_run']
    NAMED = [
        (['ui.repair.passed', 'ui.repair.failed'], 'the result of the installation repair check'),
        (['ui.error.os_unsupported', 'ui.error.user_data_unavailable', 'ui.error.user_data_inside_install',
          'ui.error.language_missing', 'ui.error.internal'],
         'start-up errors of the platform layer, also compiled into the executable'),
        (['ui.error.content_missing', 'ui.error.content_damaged'], 'a game file is missing or damaged'),
        (['ui.error.gpu_unsupported', 'ui.error.display_mode', 'ui.error.resolution_rejected'],
         'graphics start-up and display-mode errors'),
        (['ui.match.cheats_used'], 'the marker shown and recorded when development commands changed a match'),
        (['ui.match.paused'], 'the match-paused message'),
        (['ui.dialog.close', 'ui.dialog.error', 'ui.dialog.success'], 'message-dialog captions and button'),
        (['ui.dialog.sound_settings_failed'],
         'the sound-settings failure that the audio engine shows through the message dialog'),
        (['ui.input.key.<name>', 'ui.input.mouse.<name>', 'ui.input.pad.<name>', 'ui.input.condition.<condition>',
          'ui.input.not'],
         'display names of the 113 keys, 6 mouse controls, 25 gamepad controls and 9 binding conditions, and the '
         'negation word'),
        (['bot.label', 'match.team_left', 'match.player_left', 'ui.lobby.player_joined', 'ui.team.invalid_name',
          'faction.rogue.name', 'faction.visitor.name'], 'team, player and faction names and messages'),
        (['team.colour.green', 'team.colour.red', 'team.colour.yellow', 'team.colour.blue', 'team.colour.orange',
          'team.colour.purple', 'team.colour.cyan', 'team.colour.pink'], 'the eight team colour names'),
        (['match.kill_milestone'],
         'the kill-milestone message (proposed id; the owning match-rules document assigns it)'),
        (['crate.%s.tooltip' % o for o in TOOLTIPS],
         'the Power help text of each of the 20 outcomes that can be delivered as a Power, shown while the Power is '
         'Waiting (R-CRATE-011, R-CTRL-236)'),
        (['ui.dev.command.dump_bindings'], 'the caption of the development command that writes the bindings to the log'),
    ]
    L += ['', 'String ids named by the asset passes, with the documents that cite them:', '',
          '| String ids | Text | Main owner |', '|---|---|---|']
    for ids_, what in NAMED:
        cited = [x for x in ids_ if x in found]
        where = docs(cited) if cited else 'not yet cited'
        if cited and len(cited) < len(ids_):
            where += '; not yet cited: ' + ', '.join('`%s`' % x for x in ids_ if x not in found)
        L.append('| %s | %s | %s |' % (', '.join('`%s`' % x for x in ids_), what, where))
    return L, found


# =================================================================== GameAssets.md
pg = os.path.join(AD, 'GameAssets.md')
t = open(pg, encoding='utf-8').read()
W = 'GameAssets'
t = rep(t, '> Status: Asset list, pass 2a (consumers named by document; pass 2b replaces them with requirement IDs)',
        '> Status: Asset list, pass 2b (consumers named by requirement id)', 1, W)
t = rep(t, 'expands every row to one line per file, with the full format strings and the consuming documents.',
        'expands every row to one line per file, with the full format strings and its consumers: the requirement ids '
        'that use the asset, and the path of a document that cites it only outside its requirements.', 1, W)
t = rep(t, '| UI panels, title wordmark, logos, front-end decoration, small UI icons, the application icon |',
        '| UI panels, title wordmark, logo emblem, front-end decoration, small UI icons, the application icon |', 1, W)
t = rep(t, '| Opus stream, decoded on a worker thread: every member of a group whose longest member is over 20 s',
        '| MS-ADPCM stream at 48 kHz, decoded by XAudio2, with no codec in the executable (ADR-025, AST-080): every '
        'member of a group whose longest member is over 20 s', 1, W)
t = rep(t, 'the map editor\'s camera-view marker and the few rows whose role still needs confirmation.',
        'the map editor\'s camera-view marker and its bot navigation node, the land colour ramp and the shore-wave '
        'gradient that no match map names (R-MAP-086), and the two research-level icons that no rule of a match '
        'draws (R-CRATE-014).', 1, W)
t = rep(t, 'or an Opus stream for every member of a group whose longest member is over 20 s (`aen.residency.stream_threshold`, AEN-072)',
        'or an MS-ADPCM stream, decoded by XAudio2, for every member of a group whose longest member is over 20 s '
        '(`aen.residency.stream_threshold`, AEN-072, ADR-025)', 1, W)
t = rep(t, 'or an Opus stream for every member of a group whose longest member is over 20 s (AEN-072).',
        'or an MS-ADPCM stream, decoded by XAudio2, for every member of a group whose longest member is over 20 s '
        '(AEN-072, ADR-025).', 1, W)
lines = t.split('\n')
lines = [l for l in lines if not l.startswith('| `content/textures/ui/ui_logo_banner.png` |')]
t = '\n'.join(lines)
t = rep(t, '| `icon.power.laser` | Laser upgrade power level |',
        '| `icon.power.laser` | Research-level icon of the laser level; drawn by no rule of a match (R-CRATE-014, DEC-115) |', 1, W)
t = rep(t, '| `icon.power.slots_upgrade` | Power-slots power level |',
        '| `icon.power.slots_upgrade` | Research-level icon of the Power-slots level; drawn by no rule of a match (R-CRATE-014, DEC-115) |', 1, W)
t = rep(t, 'author at 2x (256x256); role to be confirmed against reference/04 |', 'author at 2x (256x256) |', 2, W)
t = rep(t, 'The seasonal theme forces ramp 06, sea tile 02 and wave gradient 04. Theme numbers carry no fixed biome;',
        'The seasonal theme forces ramp 06, sea tile 02 and wave gradient 04. No match map names ramp 14 or wave '
        'gradient 08; the map editor lists them with the others, so they are post-launch (R-MAP-086, R-MAP-325). '
        'Theme numbers carry no fixed biome;', 1, W)
t = rep(t, '| 14 | skirmish-first-playable: 1<br>launch: 13 | generated ramp |',
        '| 14 | skirmish-first-playable: 1<br>launch: 12<br>post-launch: 1 (file 14) | generated ramp |', 1, W)
t = rep(t, '| 8 | skirmish-first-playable: 1<br>launch: 7 | generated ramp |',
        '| 8 | skirmish-first-playable: 1<br>launch: 6<br>post-launch: 1 (file 08) | generated ramp |', 1, W)
t = rep(t, 'the jump pad, trees and the socket-only turret mount have none.',
        'the jump pad, trees and the turret mount have none: the mount is a socket carrier with no geometry '
        '(AST-045), and the turret\'s sphere, mesh and ray tests read its head and barrel assets (table 2.12-A).', 1, W)
t = replace_table(t, '## 3 Summary', '| Category |', summ, W)
# §18 and §18.1
L181, found = census()
lines = t.split('\n')
s = lines.index('### 18.1 String ids cited by the documents')
e = lines.index('## 19 Maps and thumbnails')
lines = lines[:s] + L181 + [''] + lines[e:]
t = '\n'.join(lines)
# §20: the removed row
t = rep(t, '| store and branding art | 1 |  |  |  |  | 1 |',
        '| store and branding art | 1 |  |  |  |  | 1 |\n| drawn by no rule | 1 |  |  |  |  | 1 |', 1, W)
t = rep(t, '| **Total** | **115** | **201** | **404** | **67** | **281** | **1068** |',
        '| **Total** | **116** | **201** | **404** | **67** | **281** | **1069** |', 1, W)
GA = t

# =================================================================== README.md
pr_ = os.path.join(AD, 'README.md')
r_ = open(pr_, encoding='utf-8').read()
W = 'README'
r_ = rep(r_, '> Status: Asset overview, pass 2a', '> Status: Asset overview, pass 2b', 1, W)
r_ = rep(r_, 'Consumers are documents in passes 1 and 2a; pass 2b replaces them with requirement IDs.',
         'Consumers are the requirement ids that use each asset, separated by `;`; where a document cites the asset '
         'only outside its requirements (a dependency list, a data-model table), its path stands in.', 1, W)
r_ = rep(r_, 'its path stands in. The asset pipeline consumes every row and is not listed per row |',
         'its path stands in. The asset pipeline consumes every row and the audio engine loads every sample group '
         '(AEN-074); neither is listed per row except where one of its requirements names the asset |', 1, W)
rs = ['| Category | Asset ids | Files | first-playable | launch | post-launch |', '|---|---:|---:|---:|---:|---:|']
for row in summ[2:-1]:
    c = row.split(' | ')
    rs.append('| %s | %s |' % (re.sub(r'^\| \[(.*)\]\(#.*\)$', r'\1', c[0]), ' | '.join(c[1:6])))
rs.append('| **Total** | ' + ' | '.join(summ[-1].split(' | ')[1:6]) + ' |')
lines = r_.split('\n')
h = next(i for i, l in enumerate(lines) if l.startswith('| Category | Asset ids |'))
lines = lines[:h] + rs + lines[table_end(lines, h):]
r_ = '\n'.join(lines)
r_ = rep(r_, '787 reference files and 281 reference text entries need no Species counterpart',
         '788 reference files and 281 reference text entries need no Species counterpart', 1, W)
r_ = rep(r_, '| Ambience | 2-D layered loops; a group streams when its longest member is over 20 s (AEN-072) |',
         '| Ambience | 2-D layered loops; a group streams, as MS-ADPCM decoded by XAudio2, when its longest member is '
         'over 20 s (AEN-072, ADR-025) |', 1, W)
r_ = rep(r_, '| Music | New; stereo 48 kHz; streamed; slots proposed, final list set by the design phase |',
         '| Music | New; stereo 48 kHz; streamed as MS-ADPCM, decoded by XAudio2; slots proposed, final list set by the '
         'design phase |', 1, W)

# =================================================================== art_and_audio_briefs.md
pb = os.path.join(AD, 'art_and_audio_briefs.md')
b = open(pb, encoding='utf-8').read()
W = 'briefs'
b = rep(b, '> Status: Style guide and category briefs, pass 2a', '> Status: Style guide and category briefs, pass 2b', 1, W)
b = rep(b, '**21 files, 14 asset ids**: accent and neutral panel strips (about 64x512), a blurred fill strip, the menu '
           'highlight beam, the title wordmark, two logo marks, a studio splash placeholder,',
        '**%d files, %d asset ids**: accent and neutral panel strips (about 64x512), a blurred fill strip, the menu '
        'highlight beam, the title wordmark, the logo emblem, a studio splash placeholder,'
        % (files['UI panels and title'], len(ids['UI panels and title'])), 1, W)
b = rep(b, '- The wordmark and logos are new identity design for Species;',
        '- The wordmark and the logo emblem are new identity design for Species;', 1, W)
b = rep(b, 'form the frost set that the seasonal theme forces. The environment artist gives each number one biome look; '
           'maps pick by number.',
        'form the frost set that the seasonal theme forces. The environment artist gives each number one biome look; '
        'maps pick by number. No match map picks ramp 14 or wave gradient 08; the map editor offers them for new maps, '
        'so they come last (post-launch).', 1, W)

if ERR:
    print('\n'.join(ERR))
    sys.exit(1)
for p, s_ in ((pg, GA), (pr_, r_), (pb, b)):
    with open(p, 'w', encoding='utf-8', newline='') as fh:
        fh.write(s_)
print('written; totals', [T[k] for k in 'abcdef'], 'excluded reference files', excluded_files,
      'string ids', len(found))
