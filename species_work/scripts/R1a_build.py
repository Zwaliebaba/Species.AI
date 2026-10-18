"""R1a helper: assemble the review file from the header fragment and the classification rows.

Usage: python3 -I R1a_build.py <sample.csv> <rows.tsv> <header.md> <out.md> <first> <last>
"""
import csv
import sys
from collections import Counter

ORDER = ["present-equal", "present-changed-by-DEC", "dropped-legacy-correct",
         "deferred-design-correct", "missing", "wrong"]


def cell(text):
    return text.replace("|", "\\|").replace("\n", " ")


def main():
    sample, rows_path, header_path, out_path, first, last = sys.argv[1:7]
    facts = list(csv.reader(open(sample, encoding="utf-8")))[1:]
    rows = {}
    for line in open(rows_path, encoding="utf-8"):
        if line.strip():
            num, cls, loc, note = line.rstrip("\n").split("\t")
            rows[int(num)] = (cls, loc, note)
    first, last = int(first), int(last)
    missing = [i for i in range(first, last + 1) if i not in rows]
    if missing:
        sys.exit(f"rows without a classification: {missing}")
    bad = [i for i, r in rows.items() if r[0] not in ORDER]
    if bad:
        sys.exit(f"unknown classification in rows {bad}")
    counts = Counter(r[0] for r in rows.values())
    header = open(header_path, encoding="utf-8").read()
    table = ["| Classification | Facts |", "|---|---|"]
    for c in ORDER:
        table.append(f"| `{c}` | {counts.get(c, 0)} |")
    table.append(f"| total | {sum(counts.values())} |")
    header = header.replace("@@TOTALS@@", "\n".join(table))
    out = [header.rstrip("\n"), ""]
    out.append("| # | orig_file:line | classification | library location | note |")
    out.append("|---|---|---|---|---|")
    for i in range(first, last + 1):
        f, line, _text, _target = facts[i - 1]
        cls, loc, note = rows[i]
        loc = "—" if loc in ("", "—") else loc
        out.append(f"| {i} | {f}:{line} | `{cls}` | {cell(loc)} | {cell(note)} |")
    open(out_path, "w", encoding="utf-8").write("\n".join(out) + "\n")
    print(dict(counts))


main()
