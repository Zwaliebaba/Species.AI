"""Renumber AST requirement ids in document order and update every reference.

Usage: python3 -I C-AST_renumber.py <markdown file>
Definitions are '**AST-NNN**'; every 'AST-NNN' token is remapped.
"""
import re
import sys


def main(path):
    text = open(path, encoding="utf-8").read()
    defs = re.findall(r"\*\*AST-(\d{3})\*\*", text)
    seen = []
    for d in defs:
        if d in seen:
            sys.exit(f"duplicate definition AST-{d}")
        seen.append(d)
    mapping = {old: f"{i + 1:03d}" for i, old in enumerate(seen)}
    refs = set(re.findall(r"AST-(\d{3})", text))
    missing = sorted(r for r in refs if r not in mapping)
    if missing:
        sys.exit("references without definition: " + ", ".join("AST-" + m for m in missing))
    out = re.sub(r"AST-(\d{3})", lambda m: "AST-" + mapping[m.group(1)], text)
    open(path, "w", encoding="utf-8").write(out)
    changed = sum(1 for k, v in mapping.items() if k != v)
    print(f"{len(mapping)} definitions, {changed} renumbered")


if __name__ == "__main__":
    main(sys.argv[1])
