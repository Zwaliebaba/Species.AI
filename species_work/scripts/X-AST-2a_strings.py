"""Census of string ids cited in the design set (X-AST-2a helper).
String-id prefixes are read from the 'String-id prefix' column of CONVENTIONS.md (as gate 7 does).
Usage: python3 -I X-AST-2a_strings.py <design_root> <manifest_csv> [--list PREFIX] [--check ID ...] [--markdown OUT]"""
import csv, os, re, sys, collections
root, man = sys.argv[1], sys.argv[2]
args = sys.argv[3:]
conv = open(os.path.join(root, 'CONVENTIONS.md'), encoding='utf-8').read()
pref, col = set(), None
for line in conv.split('\n'):
    if not line.startswith('|'):
        col = None; continue
    cells = [c.strip() for c in line.strip().strip('|').split('|')]
    if 'String-id prefix' in cells:
        col = cells.index('String-id prefix')
    elif col is not None and col < len(cells) and not cells[0].startswith('---'):
        pref.update(re.findall(r'`([a-z_]+(?:\.[a-z0-9_]+)*)`', cells[col]))
roots = {p.split('.')[0] for p in pref}
assets = {r['asset_id'] for r in csv.DictReader(open(man, encoding='utf-8'))}
stems = {a.split('.')[0] for a in assets}
keydef = set()
texts = {}
for dp, dn, fn in os.walk(root):
    if 'requirements_index' in dp:
        continue
    for f in fn:
        if f.endswith('.md'):
            p = os.path.join(dp, f); rel = os.path.relpath(p, root)
            t = open(p, encoding='utf-8').read(); texts[rel] = t
            hdr = None
            for line in t.split('\n'):
                if not line.startswith('|'):
                    hdr = None; continue
                cells = [c.strip() for c in line.strip().strip('|').split('|')]
                if hdr is None:
                    hdr = cells[0].lower(); continue
                if hdr == 'key':
                    m = re.match(r'`([a-z0-9_.]+)`$', cells[0])
                    if m:
                        keydef.add(m.group(1))
ROOTS = roots | {'team', 'tutorial', 'faction', 'bot', 'help', 'option', 'options'}
ID = re.compile(r'`((?:' + '|'.join(sorted(ROOTS, key=len, reverse=True)) + r')\.[a-z0-9_]+(?:\.[a-z0-9_<>{},]+)*)`')
found = collections.defaultdict(set)
for rel, t in texts.items():
    if rel.startswith(('assets/', 'decisions/', 'adr/', 'glossary/')) or rel == 'CONVENTIONS.md':
        continue
    for m in ID.finditer(t):
        s = m.group(1)
        if s in keydef or s.split('.')[0] in stems or s in assets:
            continue
        found[s].add(rel)
for s_ in [x for x in found if x.startswith('camera.') and not x.startswith('camera.view')]:
    del found[s_]  # marker and sequence names, not string ids
if '--markdown' in args:
    out = args[args.index('--markdown') + 1]
    def fam(x):
        seg = x.split('.')
        return '.'.join(seg[:2]) if seg[0] in ('ui', 'match', 'mode', 'tutorial') and len(seg) > 2 else seg[0]
    fams = collections.defaultdict(set)
    for x in found:
        fams[fam(x)].add(x)
    for x in [x for x in found if '*' in x]:
        del found[x]  # wildcards name families, not ids
    def link(d):
        return '[%s](../%s)' % (os.path.relpath(d, 'reference').replace('../architecture/components/', '')[:-3]
                                if d.startswith('reference/') else d.replace('architecture/components/', '')[:-3], d)
    def docs(ids):
        c = collections.Counter(d for x in ids for d in found[x])
        if not c:
            return ''
        top = sorted(c.items(), key=lambda kv: (-kv[1], kv[0]))[0][0]
        return link(top) + (' and %d more' % (len(c) - 1) if len(c) > 1 else '')
    L = ['### 18.1 String ids cited by the documents', '',
         'The reference documents and engine components cite %d string ids and id patterns (such as `ui.input.key.<name>`). '
         'Each needs new en-GB text in `strings.en_gb` and a translation in every other table, except the development '
         'captions `ui.dev.*`, which live only in `strings.dev_en_gb`. The owning document defines each id and its '
         'meaning; this table sizes the writing by family (counted mechanically from the documents).' % len(found), '',
         '| Family | Ids | Main owner |', '|---|---:|---|']
    for f in sorted(fams):
        L.append('| `%s.*` | %d | %s |' % (f, len(fams[f]), docs(fams[f])))
    NAMED = [
        (['ui.repair.passed', 'ui.repair.failed'], 'the result of the installation repair check'),
        (['ui.error.os_unsupported', 'ui.error.user_data_unavailable', 'ui.error.user_data_inside_install',
          'ui.error.language_missing', 'ui.error.internal'], 'start-up errors of the platform layer, also compiled into the executable'),
        (['ui.error.content_missing', 'ui.error.content_damaged'], 'a game file is missing or damaged'),
        (['ui.error.gpu_unsupported', 'ui.error.display_mode', 'ui.error.resolution_rejected'], 'graphics start-up and display-mode errors'),
        (['ui.match.cheats_used'], 'the marker shown and recorded when development commands changed a match'),
        (['ui.match.paused'], 'the match-paused message'),
        (['ui.dialog.close', 'ui.dialog.error', 'ui.dialog.success'], 'message-dialog captions and button'),
        (['ui.dialog.sound_settings_failed'], 'the sound-settings failure that the audio engine shows through the message dialog'),
        (['ui.input.key.<name>', 'ui.input.mouse.<name>', 'ui.input.pad.<name>', 'ui.input.condition.<condition>', 'ui.input.not'],
         'display names of the 113 keys, 6 mouse controls, 25 gamepad controls and 9 binding conditions, and the negation word'),
        (['bot.label', 'match.team_left', 'match.player_left', 'ui.lobby.player_joined', 'ui.team.invalid_name',
          'faction.rogue.name', 'faction.visitor.name'], 'team, player and faction names and messages'),
        (['team.colour.green', 'team.colour.red', 'team.colour.yellow', 'team.colour.blue', 'team.colour.orange',
          'team.colour.purple', 'team.colour.cyan', 'team.colour.pink'], 'the eight team colour names'),
    ]
    L += ['', 'String ids named in pass 2a, with the documents that cite them:', '',
          '| String ids | Text | Main owner |', '|---|---|---|']
    for ids, what in NAMED:
        cited = [x for x in ids if x in found]
        where = docs(cited) if cited else 'not yet cited'
        if cited and len(cited) < len(ids):
            where += '; not yet cited: ' + ', '.join('`%s`' % x for x in ids if x not in found)
        L.append('| %s | %s | %s |' % (', '.join('`%s`' % x for x in ids), what, where))
    open(out, 'w', encoding='utf-8').write('\n'.join(L) + '\n')
    print('families', len(fams), 'ids', len(found))
    sys.exit()
if '--check' in args:
    for s in args[args.index('--check') + 1:]:
        print(s, '->', sorted(found.get(s, [])) or 'NOT CITED')
    sys.exit()
if '--list' in args:
    p = args[args.index('--list') + 1]
    for s in sorted(found):
        if s.startswith(p + '.'):
            print(s, len(found[s]))
    sys.exit()
print('registered string prefixes:', len(pref), 'roots:', sorted(roots))
c = collections.Counter(s.split('.')[0] for s in found)
print('distinct cited ids:', len(found))
for k, v in c.most_common():
    print(k, v)
