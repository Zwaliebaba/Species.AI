"""Renumber R-BOT requirement ids in definition order across the given files.

Usage: python3 -I G07_renumber.py <mapping_out> <file1> [<file2> ...]
Files are processed in the order given; every definition **R-BOT-NNN** gets the
next sequential number, and every R-BOT-NNN token in all files is rewritten.
"""
import re
import sys

out_path = sys.argv[1]
files = sys.argv[2:]
texts = {f: open(f, encoding="utf-8").read() for f in files}

mapping = {}
n = 0
for f in files:
    for m in re.finditer(r"\*\*R-BOT-(\d{3})\*\*", texts[f]):
        old = m.group(1)
        if old in mapping:
            sys.exit(f"duplicate definition R-BOT-{old} in {f}")
        n += 1
        mapping[old] = f"{n:03d}"

missing = set()
def sub(m):
    old = m.group(1)
    if old not in mapping:
        missing.add(old)
        return m.group(0)
    return "R-BOT-" + mapping[old]

for f in files:
    new = re.sub(r"R-BOT-(\d{3})", sub, texts[f])
    open(f, "w", encoding="utf-8").write(new)

with open(out_path, "w", encoding="utf-8") as fh:
    for old, new in mapping.items():
        fh.write(f"{old}\t{new}\n")

if missing:
    print("undefined references:", sorted(missing))
print(f"{n} requirements renumbered")
