"""Index table-row ids (first cell) to line numbers inside given ranges.

Usage: python3 -I X-DEC-1_rowindex.py <file> <start-end> [<start-end> ...]
Prints: id<TAB>line
"""
import re
import sys


def main() -> None:
    path = sys.argv[1]
    ranges = []
    for spec in sys.argv[2:]:
        a, b = spec.split("-")
        ranges.append((int(a), int(b)))
    with open(path, encoding="utf-8") as fh:
        lines = fh.readlines()
    cell = re.compile(r"^\|\s*([A-Za-z0-9-]+)\s*\|")
    for a, b in ranges:
        for n in range(a, b + 1):
            m = cell.match(lines[n - 1])
            if m and m.group(1) not in ("#", "Rank", "Id", "Entry"):
                print(f"{m.group(1)}\t{n}")


if __name__ == "__main__":
    main()
