"""X-GLO-2 helper: report renamed or dropped original ledger names found in a file.

Usage: python3 -I X-GLO-2_sweep.py <rename_map.tsv> <file.md>

For each ledger row whose rule is rename or drop, split the original column on
" / " and "," and strip brackets and quotes; report each original form of at
least four characters that occurs word-bounded and case-insensitively in the
file and is not itself the Species term of that row. Hits need a human review:
some originals are plain English words that the Species text also uses.
"""
import csv
import re
import sys


def forms(cell):
    cell = re.sub(r"\([^)]*\)", "", cell)
    cell = cell.replace('"', "").replace("`", "")
    for part in re.split(r"\s*/\s*|,\s*|;\s*", cell):
        part = part.strip()
        if len(part) >= 4:
            yield part


def main():
    rows = list(csv.reader(open(sys.argv[1], encoding="utf-8"), delimiter="\t"))
    text = open(sys.argv[2], encoding="utf-8").read()
    seen = set()
    for r in rows[1:]:
        if len(r) < 6 or r[5] not in ("rename", "drop"):
            continue
        sp = r[2].lower()
        for f in forms(r[0]):
            fl = f.lower()
            if fl in seen or fl == sp or fl in sp.split(" / "):
                continue
            seen.add(fl)
            m = re.search(r"(?<![\w-])" + re.escape(f) + r"(?![\w-])", text, re.I)
            if m:
                s = max(0, m.start() - 40)
                print(f"{f!r}: ...{text[s:m.end() + 40]!r}")


if __name__ == "__main__":
    main()
