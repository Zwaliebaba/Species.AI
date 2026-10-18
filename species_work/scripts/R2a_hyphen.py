"""R2a helper: re-run the multi-word hard-fail patterns with hyphens, underscores and
line-free whitespace allowed between words, over the R2a scope, to find spellings
that evade the literal-space patterns.

Usage: python3 -I R2a_hyphen.py <forbidden_patterns.tsv> <design_root>

Prints <file>:<line>  <pattern row number>  <matched text>.
"""
import re
import sys
from pathlib import Path

SCOPE = ["reference", "glossary", "CONVENTIONS.md", "README.md", "REQUIREMENTS_INDEX.md"]


def files(root: Path):
    for item in SCOPE:
        p = root / item
        if p.is_file():
            yield p
        elif p.is_dir():
            yield from sorted(p.rglob("*.md"))


def main():
    pat_file = Path(sys.argv[1])
    root = Path(sys.argv[2])
    rows = []
    for n, line in enumerate(pat_file.read_text(encoding="utf-8").splitlines(), 1):
        if n == 1 or not line.strip():
            continue
        parts = line.split("\t")
        pat, flags = parts[0], parts[1] if len(parts) > 1 else ""
        if " " not in pat and r"\s" not in pat:
            continue
        loose = pat.replace(r"\s+", "[-_ ]+").replace(" ", "[-_ ]")
        if loose == pat and r"\s+" not in pat:
            continue
        try:
            rx_loose = re.compile(loose, re.I if "i" in flags else 0)
            rx_strict = re.compile(pat, re.I if "i" in flags else 0)
        except re.error:
            continue
        rows.append((n, rx_loose, rx_strict))
    for f in files(root):
        rel = f.relative_to(root)
        for ln, text in enumerate(f.read_text(encoding="utf-8").splitlines(), 1):
            for n, rxl, rxs in rows:
                for m in rxl.finditer(text):
                    if not rxs.search(m.group(0)):
                        print(f"{rel}:{ln}\trow {n}\t{m.group(0)}")


if __name__ == "__main__":
    main()
