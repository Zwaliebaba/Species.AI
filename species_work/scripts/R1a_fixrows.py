"""R1a helper: replace the note (and optionally the class and location) of rows in the scratch TSV.

Usage: python3 -I R1a_fixrows.py <rows.tsv> <edits.tsv>
edits.tsv lines: row<TAB>class<TAB>location<TAB>note (an empty field keeps the old value).
"""
import sys


def main():
    rows_path, edits_path = sys.argv[1:3]
    rows = [l.rstrip("\n").split("\t") for l in open(rows_path, encoding="utf-8") if l.strip()]
    index = {r[0]: r for r in rows}
    for line in open(edits_path, encoding="utf-8"):
        if not line.strip():
            continue
        num, cls, loc, note = line.rstrip("\n").split("\t")
        row = index[num]
        if cls:
            row[1] = cls
        if loc:
            row[2] = loc
        if note:
            row[3] = note
    with open(rows_path, "w", encoding="utf-8") as out:
        for r in rows:
            out.write("\t".join(r) + "\n")


main()
