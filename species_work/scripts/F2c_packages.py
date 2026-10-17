"""Size the proposed package splits of the F2c routing rows.

Usage: python3 -I F2c_packages.py <orig_dir> <routing_F2c.csv>

Sub-package rule (proposal for the coordinator):
  X-DEC-1  TO-DEC and KEEP-HOOKS rows of 40, and of 41 §1–§2 (lines < 167)
  X-DEC-2  every DEFER-DESIGN row (one DEC per DD topic)
  X-DEC-3  TO-DEC and KEEP-HOOKS rows of 41 §3 (lines 167–603)
  other owners as routed (X-GLO split into its two rows).
Sizes are unique-line bytes (a line read by two rows of one package counts once).
"""
import csv
import os
import sys
from collections import defaultdict


def sub(row):
    owner, action, f, s = row["owner_package"], row["action"], row["orig_file"], int(row["start_line"])
    if owner == "X-DEC":
        if action == "DEFER-DESIGN":
            return "X-DEC-2"
        if f.startswith("41") and 167 <= s <= 603:
            return "X-DEC-3"
        return "X-DEC-1"
    if owner == "X-GLO":
        return "X-GLO-1" if s < 476 else "X-GLO-2"
    return owner or None


def main():
    orig, routing = sys.argv[1], sys.argv[2]
    sizes = {}
    for f in ("40_carry_forward_assessment.md", "41_risks_and_open_questions.md", "90_glossary.md"):
        with open(os.path.join(orig, f), encoding="utf-8") as fi:
            sizes[f] = [len(l.encode("utf-8")) + 1 for l in fi.read().split("\n")]
    lines = defaultdict(set)
    ranges = defaultdict(list)
    with open(routing, encoding="utf-8", newline="") as fo:
        for row in csv.DictReader(fo):
            p = sub(row)
            if not p:
                continue
            f, s, e = row["orig_file"], int(row["start_line"]), int(row["end_line"])
            for n in range(s, e + 1):
                lines[p].add((f, n))
            ranges[p].append(f"{f[:2]}:{s}-{e}")
    for p in sorted(lines):
        total = sum(sizes[f][n - 1] for f, n in lines[p])
        print(f"{p}\t{len(ranges[p])} rows\t{total} bytes\t{total / 1024:.1f} KiB")


if __name__ == "__main__":
    main()
