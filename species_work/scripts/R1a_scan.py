"""R1a helper: scan the review file for hard-fail patterns and IP tokens.

Hits in the note column of a `wrong` row are reported separately, because the
brief allows ORIG quotes there and nowhere else.
Usage: python3 -I R1a_scan.py <review.md> <forbidden_patterns.tsv> <ip_tokens.txt>
"""
import csv
import re
import sys


def main():
    review, patterns_path, tokens_path = sys.argv[1:4]
    pats = []
    with open(patterns_path, encoding="utf-8") as fh:
        for row in csv.DictReader(fh, delimiter="\t"):
            flags = re.I if "i" in (row.get("flags") or "") else 0
            try:
                pats.append((row["pattern"], re.compile(row["pattern"], flags)))
            except re.error:
                pass
    tokens = [t.strip().lower() for t in open(tokens_path, encoding="utf-8") if t.strip()]
    for n, line in enumerate(open(review, encoding="utf-8"), 1):
        allowed = ""
        text = line
        if line.startswith("| ") and "| `wrong` |" in line:
            cells = line.split(" | ")
            allowed = cells[-1]
            text = " | ".join(cells[:-1])
        low = text.lower()
        for src, rx in pats:
            if rx.search(text):
                print(f"PATTERN line {n}: {src}")
        for t in tokens:
            if len(t) >= 4 and re.search(r"(?<![a-z0-9])" + re.escape(t) + r"(?![a-z0-9])", low):
                print(f"TOKEN line {n}: {t}")
        if allowed:
            for src, rx in pats:
                if rx.search(allowed):
                    print(f"(allowed: wrong-row note) line {n}: {src}")


main()
