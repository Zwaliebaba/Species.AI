"""X-GLO-1 checks for glossary files.

Usage: python3 -I X-GLO-1_check.py <repo_root> <glossary_md> [<glossary_md> ...]

Checks: alphabetical order per section, duplicate terms, kinds, link targets
(existing file or a package target in species_work/packages/INDEX.md, or a
sibling glossary file), cited DEC and ADR ids, truncation words, file sizes.
"""
import os
import re
import sys

KINDS = {"player concept", "engine term", "data term", "abbreviation"}
TRUNC = [r"\betc\.", r"\band so on\b", r"\band others\b", r"\bvarious\b", r"similar to the above", "…", r"\.\.\."]


def sort_key(term):
    t = re.sub(r"\s*\([^)]*\)", "", term).strip().lower().replace("-", " ")
    return t


def main():
    root = sys.argv[1]
    nxt = os.path.join(root, "design_next")
    index = open(os.path.join(root, "species_work", "packages", "INDEX.md"), encoding="utf-8").read()
    targets = set(re.findall(r"(?:reference|architecture|glossary|assets|decisions|adr)/[\w./-]+\.(?:md|csv)", index))
    dec_readme = open(os.path.join(nxt, "decisions", "README.md"), encoding="utf-8").read()
    decs = set(re.findall(r"\bDEC-\d{3}\b", dec_readme))
    adrs = {f[:4] for f in os.listdir(os.path.join(nxt, "adr")) if re.match(r"\d{4}-", f)}
    problems = 0
    total = 0
    for path in sys.argv[2:]:
        rel_dir = os.path.relpath(os.path.dirname(os.path.abspath(path)), nxt)
        text = open(path, encoding="utf-8").read()
        size = len(text.encode("utf-8"))
        print(f"{path}: {size} bytes")
        if size > 120 * 1024:
            print("  SIZE over 120 KB")
            problems += 1
        section = None
        prev = None
        seen = {}
        for n, line in enumerate(text.split("\n"), 1):
            if line.startswith("## "):
                section = line[3:].strip()
                prev = None
                continue
            if line.startswith("| ") and not line.startswith("| Term |") and section and section not in ("Files", "How to read an entry"):
                cells = [c.strip() for c in line.strip().strip("|").split(" | ")]
                if len(cells) != 4:
                    continue
                term, kind = cells[0], cells[1]
                if kind not in KINDS and os.path.basename(path) != "README.md":
                    print(f"  {n}: bad kind {kind!r}")
                    problems += 1
                if os.path.basename(path) != "README.md":
                    total += 1
                    k = sort_key(term)
                    if prev is not None and k < prev:
                        print(f"  {n}: order: {term!r} after {prev!r}")
                        problems += 1
                    prev = k
                    if term in seen:
                        print(f"  {n}: duplicate {term!r}")
                        problems += 1
                    seen[term] = n
                    if section == "Numbers":
                        if not term[0].isdigit():
                            print(f"  {n}: non-digit term in Numbers")
                    elif sort_key(term)[0].upper() != section[0]:
                        print(f"  {n}: {term!r} in section {section}")
                        problems += 1
            for m in re.finditer(r"\]\(([^)#]+)(#[^)]*)?\)", line):
                tgt = m.group(1)
                if tgt.startswith("http"):
                    continue
                full = os.path.normpath(os.path.join(rel_dir, tgt))
                exists = os.path.exists(os.path.join(nxt, full))
                if not exists and full not in targets:
                    print(f"  {n}: link target not found: {tgt} -> {full}")
                    problems += 1
            for d in re.findall(r"\bDEC-\d{3}\b", line):
                if d not in decs:
                    print(f"  {n}: unknown {d}")
                    problems += 1
            for a in re.findall(r"\bADR-(\d{4})\b", line):
                if a not in adrs:
                    print(f"  {n}: unknown ADR-{a}")
                    problems += 1
            for pat in TRUNC:
                if re.search(pat, line):
                    print(f"  {n}: truncation word {pat}")
                    problems += 1
    print(f"entries: {total}; problems: {problems}")


if __name__ == "__main__":
    main()
