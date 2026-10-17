#!/usr/bin/env python3
"""S_41: parse the collected closing sections (_work/closing/closing_*.md, produced by collect_closing_sections.py)
into row lists. Imported by S_41_build.py; run directly to dump the unknown rows as TSV for curation.
Usage: python3 -I S_41_parse_closing.py <CLOSING_DIR> <OUT_TSV>"""
import os, re, sys

def split_row(line):
    s = line.strip()
    if s.startswith('|'): s = s[1:]
    if s.endswith('|'): s = s[:-1]
    cells, cur, tick, i = [], '', False, 0
    while i < len(s):
        ch = s[i]
        if ch == '`': tick = not tick
        if ch == '\\' and i + 1 < len(s) and s[i + 1] == '|':
            cur += '|'; i += 2; continue
        if ch == '|' and not tick:
            cells.append(cur.strip()); cur = ''
        else:
            cur += ch
        i += 1
    cells.append(cur.strip())
    return cells

def chunks(path):
    """yield (docfile, text) per '<!-- from X -->' chunk"""
    text = open(path, encoding='utf-8').read()
    for m in re.finditer(r'<!-- from (\S+) -->\n(.*?)(?=\n<!-- from |\Z)', text, re.S):
        yield m.group(1), m.group(2)

def tables(text):
    """yield (header_cells, [row_cells...]) for each markdown table in text"""
    lines = text.split('\n'); i = 0
    while i < len(lines):
        if lines[i].startswith('|') and i + 1 < len(lines) and re.match(r'^\|\s*:?-', lines[i + 1]):
            head = split_row(lines[i]); rows = []; i += 2
            while i < len(lines) and lines[i].startswith('|'):
                rows.append(split_row(lines[i])); i += 1
            yield head, rows
        else:
            i += 1

def unknown_rows(closing_dir):
    out = []
    for doc, text in chunks(os.path.join(closing_dir, 'closing_unknowns.md')):
        nn = doc[:2]
        for head, rows in tables(text):
            h = [c.lower() for c in head]
            for r in rows:
                if not r or not re.match(r'^\d\d-[UI]\d+', r[0]): continue
                rid = re.match(r'^(\d\d-[UI]\d+)', r[0]).group(1)
                q = r[1] if len(r) > 1 else ''
                tag = r[2] if len(r) > 2 else ''
                if len(r) >= 5 and 'why it matters' in h:
                    why, res = r[3], r[4]
                elif nn == '00':
                    why, res = 'owner: ' + r[4] if len(r) > 4 else '', r[3]
                else:
                    why, res = '', r[-1] if len(r) > 3 else ''
                out.append(dict(doc=nn, file=doc, id=rid, question=q, tag=tag, why=why, resolve=res))
        # bullet style (23)
        for m in re.finditer(r'^- \*\*(\d\d-[UI]\d+)\*\*\s*(\[[UI]\])?\s*—\s*(.*)$', text, re.M):
            body = m.group(3)
            q, _, res = body.partition('*Resolve*:')
            out.append(dict(doc=nn, file=doc, id=m.group(1), question=q.strip(), tag=m.group(2) or '[I]',
                            why='', resolve=res.strip()))
    return out

def generic_rows(closing_dir, kind):
    out = []
    for doc, text in chunks(os.path.join(closing_dir, f'closing_{kind}.md')):
        for head, rows in tables(text):
            for r in rows:
                if len(r) < 2 or set(''.join(r)) <= set('-: '): continue
                out.append(dict(doc=doc[:2], file=doc, head=head, cells=r))
    return out

if __name__ == '__main__':
    rows = unknown_rows(sys.argv[1])
    with open(sys.argv[2], 'w', encoding='utf-8') as fh:
        fh.write('doc\tid\ttag\tquestion\n')
        for r in rows:
            fh.write(f"{r['doc']}\t{r['id']}\t{r['tag'][:40]}\t{r['question'][:150]}\n")
    from collections import Counter
    print(len(rows), Counter(r['doc'] for r in rows))
    for k in ('discrepancies', 'ip'):
        g = generic_rows(sys.argv[1], k); print(k, len(g), Counter(r['doc'] for r in g))
