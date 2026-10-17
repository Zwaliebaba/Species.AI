#!/usr/bin/env python3
"""S_32: enumerate every START_PROFILE section name in the compiled source set.

Usage: python3 -I S_32_profile_sections.py <SRC multiwinia root> <manifest.csv>
Prints a Markdown table: file | enclosing function | section name(s).
Only files whose manifest vcproj_build column is non-empty (compiled) are scanned.
Lines whose START_PROFILE is inside a // comment are skipped.
"""
import csv
import re
import sys
from pathlib import Path

src = Path(sys.argv[1])
manifest = Path(sys.argv[2])

compiled = []
with manifest.open(newline="") as fh:
    for row in csv.DictReader(fh):
        p = row["path"]
        if row.get("vcproj_build") and p.endswith((".cpp", ".h")):
            compiled.append(p)

func_re = re.compile(r"^[A-Za-z_][\w:<>,\s\*&~]*?\b([A-Za-z_]\w*::~?[A-Za-z_]\w*)\s*\(")
free_re = re.compile(r"^(?:static\s+)?(?:inline\s+)?[A-Za-z_][\w<>\*&\s]*\s\*?([A-Za-z_]\w*)\s*\([^;]*$")
prof_re = re.compile(r"START_PROFILE(?:_NO_DIRECT3D)?\s*\((.*?)\)\s*;")

rows = []
total = 0
for rel in sorted(compiled):
    path = src / rel
    if not path.exists():
        continue
    func = "(file scope)"
    for line in path.read_text(encoding="latin-1").splitlines():
        m = func_re.match(line)
        if m:
            func = m.group(1)
        else:
            m2 = free_re.match(line)
            if m2 and not line.startswith((" ", "\t")) and m2.group(1) not in ("if", "while", "for", "switch", "return"):
                func = m2.group(1)
        code = line.split("//", 1)[0]
        if code.lstrip().startswith("#define"):
            continue
        pm = prof_re.search(code)
        if pm:
            arg = pm.group(1).strip()
            if arg.startswith('"') and arg.endswith('"'):
                name = arg.strip('"')
            else:
                name = "<expr: " + arg + ">"
            rows.append((rel, func, name))
            total += 1

print("| File | Function | Section |")
print("|---|---|---|")
for rel, func, name in rows:
    print(f"| `{rel}` | `{func}` | {name} |")
print()
print(f"Total START_PROFILE sites in compiled files: {total}")
