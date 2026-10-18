"""R1a helper: show sampled facts with their routing row and ORIG context.

Usage: python3 -I R1a_show.py <sample.csv> <routing.csv> <orig_dir> <first> <last> [context]
Rows are numbered from 1 (first data row after the header).
"""
import csv
import os
import sys


def main():
    sample, routing, orig, first, last = sys.argv[1:6]
    ctx = int(sys.argv[6]) if len(sys.argv) > 6 else 2
    rows = list(csv.reader(open(sample, encoding="utf-8")))[1:]
    routes = list(csv.DictReader(open(routing, encoding="utf-8")))
    cache = {}
    for i in range(int(first), int(last) + 1):
        f, line, text, target = rows[i - 1]
        ln = int(line)
        print(f"=== #{i} {f}:{ln} -> {target}")
        for r in routes:
            if r["orig_file"] == f and int(r["start_line"]) <= ln <= int(r["end_line"]):
                print(f"  ROUTE {r['action']} [{r['start_line']}-{r['end_line']}] {r['target_file']} | {r['target_section']} | {r['note'][:300]}")
        if f not in cache:
            cache[f] = open(os.path.join(orig, f), encoding="utf-8").read().split("\n")
        lines = cache[f]
        for k in range(max(1, ln - ctx), min(len(lines), ln + ctx) + 1):
            mark = ">>" if k == ln else "  "
            t = lines[k - 1]
            lim = 2000 if k == ln else 400
            print(f"{mark}{k}: {t[:lim]}")
        print()


main()
