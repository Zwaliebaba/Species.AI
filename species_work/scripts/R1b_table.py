import sys
# usage: R1b_table.py notes.tsv out.md  -> appends the classification table to out.md
notes, out = sys.argv[1], sys.argv[2]
rows = [l.rstrip('\n').split('\t') for l in open(notes, encoding='utf-8') if l.strip()]
rows.sort(key=lambda r: int(r[0]))
with open(out, 'a', encoding='utf-8') as f:
    f.write('| # | orig_file:line | classification | library location | note |\n')
    f.write('|---|---|---|---|---|\n')
    for r in rows:
        n, src, cls, loc, note = (r + [''] * 5)[:5]
        f.write(f'| {n} | {src} | {cls} | {loc} | {note} |\n')
