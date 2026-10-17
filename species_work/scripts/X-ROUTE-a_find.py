"""Locate requirement ids, tuning keys or plain terms in design_next.

Usage: python3 -I X-ROUTE-a_find.py <design_next> [-s] <term> [<term> ...]

For each term prints:
  DEF  file:line  where a requirement id (**ID**) or a tuning key (first
                  table cell) is defined, with the start of the line;
  USE  file (n)   the files that mention the term, with counts (only with -s,
                  or when there is no definition).
Paths are relative to <design_next>.
"""
import os
import re
import sys


def load(root):
    docs = {}
    for dp, _dn, fns in os.walk(root):
        for fn in fns:
            if fn.endswith(".md"):
                p = os.path.join(dp, fn)
                rel = os.path.relpath(p, root)
                if rel.startswith("requirements_index") or rel == "REQUIREMENTS_INDEX.md":
                    continue
                with open(p, encoding="utf-8") as fh:
                    docs[rel] = fh.read().split("\n")
    return docs


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    root = argv[1]
    terms = argv[2:]
    show_uses = False
    if terms and terms[0] == "-s":
        show_uses = True
        terms = terms[1:]
    docs = load(root)
    for t in terms:
        print(f"## {t}")
        defs = []
        uses = {}
        esc = re.escape(t)
        if re.fullmatch(r"[A-Z][A-Z-]*-\d{3}", t):
            dre = re.compile(r"\*\*" + esc + r"\*\*")
            ure = re.compile(r"(?<![\w-])" + esc + r"(?!\d)")
        elif re.fullmatch(r"[a-z][\w.]*", t) and "." in t:
            dre = re.compile(r"^\|\s*`" + esc + r"`\s*\|")
            ure = re.compile(r"`" + esc + r"`")
        else:
            dre = None
            ure = re.compile(esc, re.I)
        for rel in sorted(docs):
            for i, line in enumerate(docs[rel], 1):
                if dre and dre.search(line):
                    defs.append((rel, i, line.strip()[:260]))
                n = len(ure.findall(line))
                if n:
                    uses[rel] = uses.get(rel, 0) + n
        for rel, i, line in defs:
            print(f"DEF {rel}:{i}  {line}")
        if show_uses or not defs:
            for rel, n in sorted(uses.items()):
                print(f"USE {rel} ({n})")
            if not uses:
                print("NONE")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
