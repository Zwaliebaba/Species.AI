"""Check which terms already have a glossary entry.

Usage: python3 -I X-ROUTE-a_glo.py <glossary_dir> <term> [<term> ...]

A term is "present" when a table row's first cell equals it (case-insensitive,
ignoring a trailing parenthesised qualifier) and "near" when a first cell
contains it. Prints one line per term with the file it belongs in
(a_to_l.md for a digit or A-L, m_to_z.md otherwise).
"""
import os
import re
import sys


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    gdir = argv[1]
    entries = []
    for fn in ("a_to_l.md", "m_to_z.md"):
        with open(os.path.join(gdir, fn), encoding="utf-8") as fh:
            for line in fh:
                if line.startswith("| ") and not line.startswith("| Term") and not line.startswith("|---"):
                    cell = line.split("|")[1].strip()
                    entries.append((fn, cell))
    for t in argv[2:]:
        tl = t.lower().strip()
        home = "a_to_l.md" if (tl[:1].isdigit() or "a" <= tl[:1] <= "l") else "m_to_z.md"
        exact = [c for f, c in entries if re.sub(r"\s*\(.*\)$", "", c.lower()) == tl or c.lower() == tl]
        near = [c for f, c in entries if tl in c.lower() and c not in exact]
        if exact:
            print(f"PRESENT  {t}  -> {exact}")
        elif near:
            print(f"NEAR     {t}  ({home}) -> {near[:6]}")
        else:
            print(f"MISSING  {t}  ({home})")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
