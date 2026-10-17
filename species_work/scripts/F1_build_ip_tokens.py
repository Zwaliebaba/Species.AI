#!/usr/bin/env python3
"""F1: build WORK/ip_tokens.txt, the plain-text source of the hashed IP denylist (X-TOOLS).

Usage: python3 -I F1_build_ip_tokens.py <ORIG dir> <WORK dir> <f1 dir>

One token or phrase per line, normalised: NFKD ASCII-folded, lower case, whitespace collapsed.
Punctuation inside a token (::, _, ., -, /) is kept. Lines have at most five words, so a scan that
hashes every 1–5-word n-gram of the text can match every line.
Contents: IP names and their forms; distinctive map display names; level file stems; distinctive
original identifiers; distinctive string keys; distinctive original strings of up to five words.
Generic English is left out (a single plain English word, or a phrase made only of plain English
words that is not an explicit IP name).
"""
import csv
import os
import re
import sys
import unicodedata

IP_NAMES = '''
multiwinia; multiwinian; multiwinians; multiwina; darwinia; darwinian; darwinians; darwinnian; introversion; introversion software;
sepulveda; dr sepulveda; dr s; protologic; evilinian; evilinians; futurewinian; futurewinians; rocketwinian; rocketwinians; futurewar;
futurewar shape format; mw; dws; systemiv; system iv; defcon; subversion; prison architect; scanner sombre; virii; triffid; triffids;
triffidegg; squadie; squadies; squaddie; insertion squad; insertionsquad; soul destroyer; souldestroyer; spore generator; sporegenerator;
space invader; space invaders; spaceinvader; smell of napalm; pokey playground; speccy; zx spectrum; dmacrew; wyndham;
darwin research associates; trueskill; xbla; xbox live arcade; xboxlivearcade; cinemaware; msn_oberon; ambx; carrybillboard;
redshirt; redshirt2; 0b9432af; f58175c7-e99c-4151-800d-7bb5e89e4960; 80.175.29.66; savedarwinia; survival of the flattest;
powerup limit reached; two powerups max; gq/s; hail to the king; unclad skies; master of multiwinia; blitz master; blitzmaster;
hot feet; hotfeet; mega rage; megarage; spawn mania; spawnmania; crate mania; cratemania; magical forest; magicalforest; napalm strike;
napalmstrike; personal shield; personalshield; nuke barrage; ants nest; antnest; trunk port; trunkport; pulse bomb; pulsebomb;
wall buster; wallbuster; battle cannon; battlecannon; pattern buffer; patternbuffer; rocket riot; rocketriot; capture the statue;
capturethestatue; kingofthehill; tankbattle; blitzkrieg; blitzkreig; blitkrieg; bitzkreig; koth; aslt; mwtitle; mwlogo; mwhelp;
mwsounds; ivlogo; tresk; trash80; dma_sc; twoataris; speccyloader; loaderamiga; gdspeech; multilobbyamb; shipheartbeat; worldviewdry;
worldviewwet; worldamb; godofcode; puritycontrol; accessallareas; attractmode; unlockall; game_unlockall; autopatch; multiwiniasupport;
shapeexporter; xcrashreport; icecaps; uplink console; spirit grove; armour duel; lightning war; crate drop!; wmd control panel
'''
GENERIC_MAP_NAMES = {'gridlock', 'melting pot', 'scorched earth', 'the last stand', 'the hard way', 'leap of faith', 'beyond the pale',
                     'the wheel', 'stress test', 'price of war', 'heart of chaos', 'bridge of light', 'pillar of light',
                     'confrontation point', 'pursuit of victory', 'the bitter front', 'the second beginning', 'echoes of the future',
                     'the walled garden', 'barriers to progress'}
IP_STEMS = ['multiwin', 'darwinia', 'darwinn', 'sepulveda', 'futurewinian', 'evilinian', 'rocketwinian', 'virii', 'triffid', 'squadie',
            'squaddie', 'souldestroyer', 'sporegenerator', 'spaceinvader', 'trunkport', 'pulsebomb', 'wallbuster', 'insertionsquad',
            'bitzkreig', 'blitzkreig', 'blitkrieg', 'blitzkrieg', 'spawnmania', 'cratemania', 'hotfeet', 'megarage', 'systemiv',
            'introversion', 'eclipse', 'mwhelp', 'mwtitle', 'mwlogo', 'trueskill', 'redshirt', 'speccy', 'pokey', 'cratereward',
            'taskmanager', 'futurewar', 'koth', 'soulrain', 'syncrand', 'syncfrand', 'syncsfrand']


def norm(s):
    s = unicodedata.normalize('NFKD', s).encode('ascii', 'ignore').decode('ascii').lower()
    return re.sub(r'\s+', ' ', s).strip()


def main():
    orig, work, f1 = sys.argv[1:4]
    with open(os.path.join(f1, 'prose_vocab.tsv'), encoding='utf-8', newline='') as fh:
        vocab = {r['word']: int(r['lower_count']) for r in csv.DictReader(fh, delimiter='\t')}

    def english(w):
        return vocab.get(w.lower().strip("'"), 0) >= 3

    def parts(tok):
        return [p for p in re.split(r'_|::|(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])', tok) if p]

    def has_stem(t):
        t = t.lower()
        return any(s in t for s in IP_STEMS)

    out = {}

    def add(tok, why):
        t = norm(tok)
        if not t or len(t.split()) > 5:
            return
        out.setdefault(t, why)

    multi_names = []
    for n in IP_NAMES.replace('\n', ' ').split(';'):
        if n.strip():
            add(n, 'ip-name')
            if ' ' in n.strip():
                multi_names.append(norm(n))
    # map display names and level file stems
    with open(os.path.join(f1, 'map_names.tsv'), encoding='utf-8', newline='') as fh:
        for r in csv.DictReader(fh, delimiter='\t'):
            if norm(r['display_name']) not in GENERIC_MAP_NAMES:
                add(r['display_name'], 'map-name')
    with open(os.path.join(orig, '_work', 'drafts', 'X4_levels.csv'), encoding='utf-8', newline='') as fh:
        for r in csv.DictReader(fh):
            add(os.path.splitext(os.path.basename(r['file']))[0], 'level-file')
    # identifiers that the hard-fail list carries (note column starts with the identifier)
    with open(os.path.join(work, 'forbidden_patterns.tsv'), encoding='utf-8', newline='') as fh:
        for r in csv.DictReader(fh, delimiter='\t'):
            if r['class'] != 'identifier':
                continue
            tok = r['note'].split(' (')[0]
            if has_stem(tok):
                add(tok, 'identifier')
    # compound identifiers that the ledger renames because they are IP (90 §2 and §3 rows)
    with open(os.path.join(work, 'rename_map.tsv'), encoding='utf-8', newline='') as fh:
        for r in csv.DictReader(fh, delimiter='\t'):
            if '90 §' not in r['note']:
                continue
            for tok in re.findall(r'\b[A-Z][a-z0-9]+(?:[A-Z][a-z0-9]+)+\b', r['original']):
                add(tok, 'identifier')
    # string keys (distinctive ones)
    with open(os.path.join(f1, 'string_keys.tsv'), encoding='utf-8', newline='') as fh:
        for r in csv.DictReader(fh, delimiter='\t'):
            k = r['key']
            if r['covered_by'] == 'family pattern' or has_stem(k):
                add(k, 'string-key')
    # original strings of three to five words that carry a non-English or IP word
    with open(os.path.join(work, 'original_strings.txt'), encoding='utf-8') as fh:
        for line in fh:
            s = line.strip()
            ws = s.split()
            if 3 <= len(ws) <= 5 and (has_stem(s) or any(n in s for n in multi_names)):
                add(s, 'string')
    # never list a Species term or identifier
    species = set()
    with open(os.path.join(work, 'rename_map.tsv'), encoding='utf-8', newline='') as fh:
        for r in csv.DictReader(fh, delimiter='\t'):
            if r['rule'] == 'drop':
                continue
            for v in (r['species_term'], r['species_identifier']):
                if v:
                    species.add(norm(v))
    lines = sorted(t for t in out if t not in species)
    with open(os.path.join(work, 'ip_tokens.txt'), 'w', encoding='utf-8', newline='') as fh:
        fh.write('\n'.join(lines) + '\n')
    by = {}
    for t in lines:
        by[out[t]] = by.get(out[t], 0) + 1
    print('ip tokens', len(lines), by)


if __name__ == '__main__':
    main()
