"""S26 helper: for each match of a regex in C++ files, print file:line, enclosing function, and the line.
Usage: python3 -I S_26_enclosing_fn.py <regex> <file> [<file> ...]"""
import re, sys
pat = re.compile(sys.argv[1])
fnre = re.compile(r'^[A-Za-z_][\w\s\*&<>:,]*?([A-Za-z_]\w*::~?[A-Za-z_]\w*)\s*\(')
for path in sys.argv[2:]:
    try:
        lines = open(path, encoding='latin-1').read().split('\n')
    except OSError:
        continue
    cur = '?'
    for i, l in enumerate(lines, 1):
        m = fnre.match(l)
        if m and not l.rstrip().endswith(';'):
            cur = m.group(1)
        if pat.search(l) and not l.strip().startswith('//'):
            print(f"{path}:{i}\t{cur}\t{l.strip()[:150]}")
