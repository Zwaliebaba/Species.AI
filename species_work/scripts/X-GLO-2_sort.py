"""X-GLO-2 helper: sort the entry rows of each glossary section in place.

Usage: python3 -I X-GLO-2_sort.py <glossary_md>

Order rule (glossary README): alphabetical by term, word by word, ignoring
case and any qualifier in brackets, reading a hyphen as a space. The sort is
stable, so rows with equal keys keep their order. Prints moved rows.
"""
import re
import sys


def key(row):
    term = row.split(" | ")[0].lstrip("| ").strip()
    return re.sub(r"\s*\([^)]*\)", "", term).strip().lower().replace("-", " ")


def main():
    path = sys.argv[1]
    lines = open(path, encoding="utf-8").read().split("\n")
    out = []
    i = 0
    moved = 0
    while i < len(lines):
        line = lines[i]
        out.append(line)
        if line.startswith("|---") and out[-2].startswith("| Term |"):
            j = i + 1
            rows = []
            while j < len(lines) and lines[j].startswith("| "):
                rows.append(lines[j])
                j += 1
            srt = sorted(rows, key=key)
            for a, b in zip(rows, srt):
                if a != b:
                    moved += 1
            out.extend(srt)
            i = j
            continue
        i += 1
    open(path, "w", encoding="utf-8").write("\n".join(out))
    print(f"rows in changed positions: {moved}")


if __name__ == "__main__":
    main()
