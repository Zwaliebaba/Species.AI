"""S09: phrase-table coverage per language file (data/language/*.txt).
Usage: python3 -I S_09_lang_coverage.py <SRC>/data/language
Parsing approximates code/lib/language_table.cpp:LangTable::ParseLanguageFile:
key = first token (split on whitespace, ':' or ','), '#'-led lines are comments,
later duplicates replace earlier ones. Prints a Markdown table."""
import sys, os, re, codecs

def read_text(path):
    raw = open(path, 'rb').read()
    if raw[:2] == b'\xfe\xff':
        return raw[2:].decode('utf-16-be', 'replace'), 'UTF-16 BE'
    if raw[:2] == b'\xff\xfe':
        return raw[2:].decode('utf-16-le', 'replace'), 'UTF-16 LE'
    try:
        raw.decode('ascii'); enc = 'ASCII'
    except UnicodeDecodeError:
        enc = '8-bit'
    return raw.decode('latin-1'), enc

def parse(text):
    table = {}
    dups = 0
    for line in text.splitlines():
        s = line.strip()
        if not s or s.startswith('#'):
            continue
        m = re.match(r'([^\s:,]+)[\s:,]*(.*)$', s)
        if not m:
            continue
        k, v = m.group(1).lower(), m.group(2)
        if k in table:
            dups += 1
        table[k] = v
    return table, dups

PREFIXES = ['multiwinia_', 'dialog_', 'help', 'bootloader_', 'cratename_', 'crate_', 'editor_',
            'taskmanager_', 'tutorial', 'mp_', 'launchpad_', 'authentication_', 'control', 'newcontrols_']

def bucket(k):
    for p in PREFIXES:
        if k.startswith(p):
            return p
    return 'other'

def main(d):
    files = sorted(f for f in os.listdir(d) if f.endswith('.txt'))
    tables = {}
    meta = {}
    for f in files:
        t, enc = read_text(os.path.join(d, f))
        tab, dups = parse(t)
        tables[f[:-4]] = tab
        meta[f[:-4]] = (enc, dups, os.path.getsize(os.path.join(d, f)))
    en = tables['english']
    enk = set(en)
    mwk = {k for k in enk if k.startswith('multiwinia_') or k.startswith('mp_')}
    print('| Language file | Encoding | Size (bytes) | Keys | Duplicate keys | Keys shared with English | Coverage of English keys | `multiwinia_*`/`mp_*` keys covered | Keys absent from English | Values identical to English |')
    print('|---|---|---|---|---|---|---|---|---|---|')
    for lang in tables:
        tab = tables[lang]
        k = set(tab)
        shared = k & enk
        mw = len(k & mwk)
        same = sum(1 for x in shared if tab[x] == en[x]) if lang != 'english' else '—'
        enc, dups, size = meta[lang]
        print(f'| `{lang}.txt` | {enc} | {size} | {len(k)} | {dups} | {len(shared)} | {100.0*len(shared)/len(enk):.1f} % | {mw}/{len(mwk)} ({100.0*mw/len(mwk):.1f} %) | {len(k-enk)} | {same} |')
    print()
    print('English key families:')
    from collections import Counter
    c = Counter(bucket(x) for x in enk)
    print('| Prefix | English keys | ' + ' | '.join(l for l in tables if l != 'english') + ' |')
    print('|---|---|' + '---|' * (len(tables) - 1))
    for p, n in sorted(c.items(), key=lambda x: -x[1]):
        row = []
        for l in tables:
            if l == 'english':
                continue
            have = sum(1 for x in tables[l] if x in enk and bucket(x) == p)
            row.append(str(have))
        print(f'| `{p}` | {n} | ' + ' | '.join(row) + ' |')

if __name__ == '__main__':
    main(sys.argv[1])
