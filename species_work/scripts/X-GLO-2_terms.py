"""X-GLO-2 helper: list CONVENTIONS section 8 terms in a letter range that a text uses.

Usage: python3 -I X-GLO-2_terms.py <CONVENTIONS.md> <text.md> <first_letter> <last_letter> [<glossary_md>]

Prints each section-8 term whose first letter is in the range and that occurs in
the text (word-bounded, case-insensitive). With a glossary file, marks the terms
that have no entry there (term cell, qualifiers in brackets ignored).
"""
import re
import sys


def terms_from_conventions(path):
    out = []
    in8 = False
    for line in open(path, encoding="utf-8"):
        if line.startswith("## 8 "):
            in8 = True
            continue
        if line.startswith("## 9 "):
            break
        if in8 and line.startswith("| ") and not line.startswith("| Species term") and not line.startswith("|---"):
            out.append(line.split(" | ")[0].lstrip("| ").strip())
    return out


def entries(path):
    names = set()
    for line in open(path, encoding="utf-8"):
        if line.startswith("| ") and not line.startswith("| Term") and not line.startswith("|---"):
            t = line.split(" | ")[0].lstrip("| ").strip()
            names.add(t.lower())
            names.add(re.sub(r"\s*\([^)]*\)", "", t).strip().lower())
    return names


def main():
    conv, text_path, a, b = sys.argv[1:5]
    gloss = entries(sys.argv[5]) if len(sys.argv) > 5 else None
    text = open(text_path, encoding="utf-8").read()
    for t in sorted(set(terms_from_conventions(conv)), key=str.lower):
        c = t[0].upper()
        if not (a <= c <= b):
            continue
        base = re.sub(r"\s*\([^)]*\)", "", t).strip()
        if re.search(r"(?<![\w-])" + re.escape(base) + r"(?![\w-])", text, re.I):
            mark = ""
            if gloss is not None and t.lower() not in gloss and base.lower() not in gloss:
                mark = "  <-- no entry"
            print(t + mark)


if __name__ == "__main__":
    main()
