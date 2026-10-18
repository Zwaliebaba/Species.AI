"""R4a: find Species terms of CONVENTIONS 8.1-8.10 written with a different capitalisation.

Usage: python3 -I R4a_terms.py <design_root>
Prints term, variant, count and up to three locations. Code spans, link targets and headings are skipped.
"""
import os
import re
import sys

root = sys.argv[1]
conv = open(os.path.join(root, "CONVENTIONS.md"), encoding="utf-8").read().split("\n")
terms = []
section = ""
for ln in conv:
    if ln.startswith("### "):
        section = ln[4:]
    if section[:4] in ("8.1 ", "8.2 ", "8.3 ", "8.4 ", "8.5 ", "8.6 ", "8.7 ", "8.8 ", "8.9 ", "8.10") and ln.startswith("| ") and not ln.startswith("| Species term"):
        t = ln.split("|")[1].strip()
        t = re.sub(r"\s*\(.*\)$", "", t)
        if t and not t.startswith("---") and " " in t:
            terms.append(t)
terms = sorted(set(terms), key=len, reverse=True)
texts = {}
for dirpath, dirnames, filenames in os.walk(root):
    dirnames[:] = [d for d in dirnames if d != "requirements_index"]
    for fn in filenames:
        if fn.endswith(".md"):
            p = os.path.join(dirpath, fn)
            texts[os.path.relpath(p, root)] = open(p, encoding="utf-8").read().split("\n")
for t in terms:
    pat = re.compile(r"(?<![A-Za-z_.`-])" + r"\s+".join(re.escape(w) for w in t.split()) + r"(?![A-Za-z_-])", re.I)
    variants = {}
    for f, lines in texts.items():
        in_code = False
        for i, line in enumerate(lines, 1):
            if line.lstrip().startswith("```"):
                in_code = not in_code
                continue
            if in_code or line.startswith("#"):
                continue
            clean = re.sub(r"`[^`]*`", " ", line)
            clean = re.sub(r"\]\([^)]*\)", "]", clean)
            for m in pat.finditer(clean):
                v = m.group(0)
                if v == t:
                    continue
                # sentence-initial: allow first letter difference only
                start = m.start()
                before = clean[:start].rstrip()
                sentence_start = (not before) or before[-1] in ".:!?|*(-" or before.endswith("**")
                if sentence_start and v[1:] == t[1:] and v[0].upper() == t[0].upper():
                    continue
                if v.lower() == t.lower():
                    variants.setdefault(v, []).append(f"{f}:{i}")
    for v, locs in sorted(variants.items()):
        print(f"{t}\t{v}\t{len(locs)}\t{' '.join(locs[:4])}")
