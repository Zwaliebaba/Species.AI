"""Report which terms already have a glossary entry.

Usage: python3 -I X-ROUTE-b_glossary_check.py <glossary_dir> <term> [<term> ...]

A term has an entry when a table row of a_to_l.md or m_to_z.md starts with
that term (case-insensitive, ignoring a bracketed qualifier).
"""
import os
import re
import sys


def entries(glossary_dir):
    found = {}
    for name in ("a_to_l.md", "m_to_z.md"):
        path = os.path.join(glossary_dir, name)
        with open(path, encoding="utf-8") as fh:
            for line in fh:
                if not line.startswith("| "):
                    continue
                cell = line[2:].split("|", 1)[0].strip()
                key = re.sub(r"\s*\(.*?\)\s*", " ", cell).strip().lower()
                found.setdefault(key, name)
                found.setdefault(cell.lower(), name)
    return found


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    found = entries(argv[1])
    for term in argv[2:]:
        hit = found.get(term.lower())
        print(f"{'HAVE' if hit else 'MISS'}\t{term}\t{hit or ''}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
