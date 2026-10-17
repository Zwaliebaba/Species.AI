"""FX-07 helper: test candidate identifiers against the hard and soft pattern lists.

Usage: python3 -I FX-07_check_names.py <forbidden_patterns.tsv> <soft_patterns.tsv> <name> [<name> ...]
Prints every candidate that one of the patterns matches, in code form (`Name`) and bare.
"""
import csv
import re
import sys


def load(path):
    pats = []
    with open(path, encoding="utf-8", newline="") as fh:
        for row in csv.reader(fh, delimiter="\t"):
            if not row or row[0] == "pattern":
                continue
            flags = re.IGNORECASE if len(row) > 1 and "i" in row[1] else 0
            try:
                pats.append((row[0], re.compile(row[0], flags)))
            except re.error:
                pass
    return pats


def main():
    hard, soft, names = sys.argv[1], sys.argv[2], sys.argv[3:]
    pats = load(hard) + load(soft)
    hits = 0
    for name in names:
        for form in (name, "`" + name + "`"):
            for src, rx in pats:
                if rx.search(form):
                    print(f"{name}: matched by {src}")
                    hits += 1
                    break
    print(f"{len(names)} names checked, {hits} hits")


if __name__ == "__main__":
    main()
