#!/usr/bin/env python3
"""X1a: extract every `Class::Read(TextReader...)` (and Write(TextWriter...)) function body
from the given source files. Usage: X1a_extract_read_funcs.py OUTFILE SRC_FILE...
Brace-matching extraction; prints file, line and body."""
import re, sys

def extract(path, pat):
    txt = open(path, encoding='latin-1').read()
    out = []
    for m in re.finditer(pat, txt):
        start = m.start()
        i = txt.find('{', m.end())
        if i < 0:
            continue
        depth = 0
        j = i
        while j < len(txt):
            if txt[j] == '{': depth += 1
            elif txt[j] == '}':
                depth -= 1
                if depth == 0:
                    break
            j += 1
        line = txt.count('\n', 0, start) + 1
        out.append((line, txt[start:j+1]))
    return out

def main():
    outf = sys.argv[1]
    pat = r'^[A-Za-z_][\w\s\*&]*?\b(\w+)::(Read|Write)\s*\(\s*Text(Reader|Writer)\s*\*[^)]*\)\s*$'
    with open(outf, 'w') as o:
        for p in sys.argv[2:]:
            for line, body in extract(p, re.compile(pat, re.M)):
                o.write(f'===== {p}:{line}\n{body}\n\n')

if __name__ == '__main__':
    main()
