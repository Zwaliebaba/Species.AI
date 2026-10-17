#!/usr/bin/env python3
"""F1: generate the body of NEXT/CONVENTIONS.md §8 "Naming" from WORK/rename_map.tsv.

Usage: python3 -I F1_naming_section.py <rename_map.tsv> <out.md>

Only Species terms, identifiers and string-id prefixes are written; the original column is never read
into the output. Dropped rows appear only in the short list of names for Dropped decision entries.
"""
import csv
import re
import sys

GROUPS = [
    ('game', 'Game, studio and factions'),
    ('unit', 'Units'),
    ('creature', 'Creatures and hazards'),
    ('building', 'Buildings and world objects'),
    ('mode', 'Modes'),
    ('match', 'Match rules'),
    ('crate', 'Supply crates and their outcomes'),
    ('power', 'Powers'),
    ('achievement', 'Achievements'),
    ('ui', 'User interface, text and camera'),
    ('pattern', 'Naming patterns'),
    ('engine', 'Engine and simulation types'),
]
SKIP_TERM = re.compile(r'^(new |own |commissioned |licensed |re-|HLSL |Species string|Species tagline|platform-neutral|title wordmark|'
                       r'engine-generated|terrain colour ramp|shore-wave|seasonal|explainer choreographies)', re.I)


def group_of(r):
    p, ident, term = r['string_id_prefix'], r['species_identifier'], r['species_term']
    if '<' in ident or ident.endswith('*') or ident.endswith('.txt') or (ident.islower() and '_' in ident):
        return 'pattern'
    if p in ('game',) or p.startswith('faction.') or ident in ('<STUDIO>', 'Bot', 'BotController') or p.startswith('team.') or p == 'bot':
        return 'game'
    for g in ('unit', 'creature', 'building', 'mode', 'match', 'crate', 'power', 'achievement'):
        if p == g or p.startswith(g + '.'):
            return g
    if ident.startswith('CrateOutcome::'):
        return 'crate'
    if ident.startswith('Achievement::'):
        return 'achievement'
    if ident.startswith('MatchMode::'):
        return 'mode'
    if p.startswith('ui') or p.startswith('camera') or p.startswith('sfx') or p.startswith('tutorial') or p.startswith('map'):
        return 'ui'
    return 'engine'


SKIP_EXACT = {'Species achievement names', 'map ids', 'sound event set', 'camera views', 'map'}
DISPLAY = {'<STUDIO>': 'studio name (placeholder)', 'always true': 'always-on feature flag', 'feature flags': 'always-on feature flag', 'feature flag x': 'feature flag',
           'spawn_view_<n>': 'start view of player slot n', 'default_view': 'default camera view', 'previous_view': 'previous camera view'}
KIND = {'game': 'game', 'unit': 'unit', 'creature': 'creature', 'building': 'world object', 'mode': 'mode', 'match': 'match rule',
        'crate': 'crate outcome', 'power': 'power', 'achievement': 'achievement', 'ui': 'UI or text', 'pattern': 'naming pattern',
        'engine': 'engine type'}


def esc(s):
    return s.replace('|', '\\|')


def main():
    src, out = sys.argv[1], sys.argv[2]
    with open(src, encoding='utf-8', newline='') as fh:
        rows = list(csv.DictReader(fh, delimiter='\t'))
    entries = {}
    dropped = []
    known_ids = {r['species_identifier'].strip() for r in rows if 'code form' not in r['note'] and r['rule'] != 'drop'}
    for r in rows:
        term, ident, pref = r['species_term'].strip(), r['species_identifier'].strip(), r['string_id_prefix'].strip()
        if r['rule'] == 'drop':
            if term and re.fullmatch(r'[A-Z][\w-]*(?: [A-Za-z][\w-]*)*', term) and term not in dropped:
                dropped.append(term)
            continue
        note = r['note']
        if note.startswith('form') or term.endswith('(s)') or term in SKIP_EXACT:
            continue
        if 'code form' in note and ident in known_ids:
            continue
        term = DISPLAY.get(term, term)
        if term.startswith('ui '):
            term = 'UI ' + term[3:]
        if not term or SKIP_TERM.match(term) or term.startswith('"'):
            continue
        if not ident and not pref:
            continue
        if ' / ' in term or ', ' in term:
            continue  # combined rows; each member has its own row
        if term in ('Units', 'units/s'):
            continue
        g = group_of(r)
        key = (g, term.lower())
        e = entries.setdefault(key, {'term': term, 'group': g, 'ids': [], 'prefixes': []})
        if ident and ident not in e['ids']:
            e['ids'].append(ident)
        if pref and pref not in e['prefixes']:
            e['prefixes'].append(pref)
    # an identifier listed in a gameplay group is not repeated under engine types
    placed = {i for e in entries.values() if e['group'] != 'engine' for i in e['ids']}
    for k in [k for k, e in entries.items() if e['group'] == 'engine' and e['ids'] and all(i in placed for i in e['ids'])]:
        del entries[k]
    lines = []
    lines.append('Species names come from the rename ledger of the conversion run. A document uses only the names below; '
                 'a name that is missing is proposed in the agent report, never invented in a document.')
    lines.append('')
    lines.append('- **Terms.** Proper Species terms are capitalised as listed (Worker, Spawn Node, Supply crate). '
                 'Generic words (tree, wall, spider, turret) stay lower case in prose.')
    lines.append('- **Identifiers.** Types and enumerators are PascalCase (`SpawnNode`, `CrateOutcome::Shield`); '
                 'files and ids are lower case (`species`, `map_siege_3p_01`). The game name is `Species` in types and namespaces '
                 'and `species` in files and ids. A generic word used as a type name takes its kind as a suffix '
                 '(`TreeObject`, `SpiderCreature`), and no Species identifier repeats an identifier of the reference implementation.')
    lines.append('- **String ids** are dotted, `<prefix>.<item>`, using the prefix listed (`unit.worker.name`, `mode.flag_chain.objective`, '
                 '`ui.lobby.ready`). Player-facing text is always new text.')
    lines.append('- **Tuning keys** are `ref.` followed by the same prefix and a parameter (`ref.unit.worker.move_speed`, '
                 '`ref.mode.spawn_control.match_length`).')
    lines.append('- **Asset ids** come from `assets/asset_manifest.csv`; the patterns below give only the stems that documents may cite.')
    lines.append('')
    n = 0
    for gi, (g, title) in enumerate(GROUPS, 1):
        es = sorted((e for e in entries.values() if e['group'] == g), key=lambda e: e['term'].lower())
        if not es:
            continue
        lines.append('### 8.%d %s' % (gi, title))
        lines.append('')
        lines.append('| Species term | Kind | Identifier | String-id prefix |')
        lines.append('|---|---|---|---|')
        for e in es:
            ids = ', '.join('`%s`' % esc(i) for i in e['ids']) or '—'
            prefs = ', '.join('`%s`' % esc(p) for p in e['prefixes']) or '—'
            lines.append('| %s | %s | %s | %s |' % (esc(e['term']), KIND[g], ids, prefs))
            n += 1
        lines.append('')
    if dropped:
        lines.append('### 8.%d Names used only in Dropped decision entries' % (len(GROUPS) + 1))
        lines.append('')
        lines.append('These name content that the reference library does not specify: ' + ', '.join(sorted(set(dropped))) + '.')
        lines.append('')
    with open(out, 'w', encoding='utf-8', newline='') as fh:
        fh.write('\n'.join(lines).rstrip('\n') + '\n')
    print('vocabulary rows', n, 'dropped names', len(set(dropped)))


if __name__ == '__main__':
    main()
