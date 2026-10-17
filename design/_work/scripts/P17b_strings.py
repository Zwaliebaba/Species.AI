#!/usr/bin/env python3
"""P17b: list every LANGUAGEPHRASE / ISLANGUAGEPHRASE / AddOption string id used in
the given source files and look up its English text.

Usage: python3 -I P17b_strings.py <english.txt> <OUT_CSV> <src files...>
"""
import csv
import re
import sys

eng_path, out_path, srcs = sys.argv[1], sys.argv[2], sys.argv[3:]

table = {}
with open(eng_path, encoding="latin-1") as fh:
    for line in fh:
        line = line.rstrip("\r\n")
        if not line or line.startswith("#"):
            continue
        m = re.match(r'^(\S+)\s+(.*)$', line)
        if m:
            table.setdefault(m.group(1), m.group(2).strip())

pat = re.compile(r'(?:LANGUAGEPHRASE|ISLANGUAGEPHRASE|AddOption|GameMenuButton|MenuCloseButton|DoesPhraseExist)\s*\(\s*"([^"]+)"')
rows = []
seen = set()
for s in srcs:
    with open(s, encoding="latin-1") as fh:
        for n, line in enumerate(fh, 1):
            for m in pat.finditer(line):
                k = m.group(1)
                key = (k, s)
                if key in seen:
                    continue
                seen.add(key)
                rows.append({"id": k, "english": table.get(k, "<not in english.txt>"),
                             "file": s.split("multiwinia/")[-1], "line": n})
with open(out_path, "w", newline="") as fh:
    w = csv.DictWriter(fh, fieldnames=["id", "english", "file", "line"])
    w.writeheader()
    for r in rows:
        w.writerow(r)
print(len(rows), "rows,", len({r['id'] for r in rows}), "distinct ids")
