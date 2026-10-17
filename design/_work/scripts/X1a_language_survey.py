#!/usr/bin/env python3
"""X1a: survey of data/language/*.txt mirroring code/lib/language_table.cpp:LangTable::ParseLanguageFile.
Usage: python3 -I X1a_language_survey.py SRC_ROOT
Per file: encoding, physical lines, phrases (last wins on duplicate key), duplicate keys, keys only in
this file vs english, _kbd/_xin variant keys, part_ keys, marker usage ([IFMODE ..], [ELSE], [ENDIF],
[KEY...], [ phrase] / [phrase] references), '\\n' escapes, other backslash sequences, '%' format
specifiers, '*' flags, dropped unterminated last line, characters beyond U+00FF."""
import collections, glob, os, re, sys
NSEP = ' \t\n\r:,'
WSEP = ' \f\t\n\r:,'

def load(path):
    d = open(path, 'rb').read()
    if d[:2] == b'\xff\xfe': return 'UTF-16LE', d[2:].decode('utf-16-le'), True
    if d[:2] == b'\xfe\xff': return 'UTF-16BE', d[2:].decode('utf-16-be'), True
    return 'bytes(Latin-1)', d.decode('latin-1'), False

def parse(path):
    enc, text, uni = load(path)
    raw = text.split('\n')
    dropped = None
    if text.endswith('\n'):
        raw = raw[:-1]
    else:
        # TextFileReader keeps the unterminated last line; Unicode readers and TextDataReader drop it
        if uni: dropped = raw[-1]; raw = raw[:-1]
    seps = WSEP if uni else NSEP
    phrases = collections.OrderedDict(); dups = []
    for l in raw:
        for ch in '#\n\r':
            i = l.find(ch)
            if i >= 0: l = l[:i]
        i = 0
        while i < len(l) and l[i] in seps: i += 1
        if i >= len(l): continue
        j = i
        while j < len(l) and l[j] not in NSEP: j += 1     # key token uses the narrow separator set
        key = l[i:j]
        k = j + 1 if j < len(l) else j
        while k < len(l) and l[k] in seps: k += 1
        val = l[k:]
        if key.lower() in phrases: dups.append(key)
        phrases[key.lower()] = (key, val)
    return enc, len(raw), phrases, dups, dropped

def main():
    root = sys.argv[1]
    files = sorted(glob.glob(os.path.join(root, 'data/language/*.txt')))
    res = {os.path.basename(f): parse(f) for f in files}
    eng = res['english.txt'][2]
    for name, (enc, nlines, ph, dups, dropped) in res.items():
        vals = [v for (_, v) in ph.values()]
        markers = collections.Counter()
        for v in vals:
            for m in re.findall(r'\[([^\]]*)\]', v):
                if m.upper().startswith('IFMODE'): markers['[IFMODE ' + m[7:] + ']'] += 1
                elif m.upper() in ('ELSE', 'ENDIF'): markers['[' + m.upper() + ']'] += 1
                elif m.startswith('KEY'): markers['[KEY<control>]'] += 1
                elif m.startswith(' '): markers['[ <phrase>]'] += 1
                else: markers['[<phrase>]'] += 1
        nl = sum(v.count('\\n') for v in vals)
        otherbs = collections.Counter(m for v in vals for m in re.findall(r'\\[^n]', v))
        pct = collections.Counter(m for v in vals for m in re.findall(r'%[-+ #0]*\d*(?:\.\d+)?[a-zA-Z]', v))
        star = collections.Counter(m for v in vals for m in re.findall(r'\*[A-Za-z]', v))
        kbd = sum(1 for k in ph if k.endswith('_kbd')); xin = sum(1 for k in ph if k.endswith('_xin'))
        part = sum(1 for k in ph if k.startswith('part_'))
        empty = sum(1 for v in vals if v == '')
        wide = sum(1 for v in vals for ch in v if ord(ch) > 255)
        missing = len(set(eng) - set(ph)); extra = len(set(ph) - set(eng))
        badkey = [k for (k, _) in ph.values() if not re.fullmatch(r'[A-Za-z0-9_\-\.]+', k)]
        print(f'{name}: enc={enc} lines={nlines} phrases={len(ph)} dupkeys={len(dups)} empty={empty} '
              f'missing_vs_english={missing} extra_vs_english={extra} _kbd={kbd} _xin={xin} part_={part} '
              f'\\n-escapes={nl} chars>U+00FF={wide}')
        print('   markers:', dict(markers))
        if otherbs: print('   other backslash sequences:', dict(otherbs))
        if pct: print('   printf specifiers:', dict(pct.most_common(12)))
        if star: print('   *X flags:', dict(star))
        if dups: print('   duplicate keys (first 8):', dups[:8])
        if badkey: print('   keys with unusual chars (first 8):', badkey[:8])
        if dropped is not None: print('   DROPPED unterminated last line:', repr(dropped[:70]))

if __name__ == '__main__':
    main()
