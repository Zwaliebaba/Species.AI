#!/usr/bin/env python3
"""S_28 - enumerate every UI class declaration (class X [: public Y]) in the UI source files.

Usage: python3 -I S_28_ui_classes.py <multiwinia_src_root> <out_csv>

Scans contrib/eclipse/*.h|cpp, code/interface/*.h|cpp, code/UI/*.h|cpp, code/game_menu.h|cpp.
Writes CSV: class,base,file,line  (base empty when the class has no base; forward
declarations "class X;" are skipped). Data only; nothing in the source tree is executed.
"""
import csv
import os
import re
import sys

DIRS = ["contrib/eclipse", "code/interface", "code/UI"]
EXTRA = ["code/game_menu.h", "code/game_menu.cpp"]

DECL = re.compile(r"^[ \t]*class[ \t]+([A-Za-z_][A-Za-z_0-9]*)\s*"
                  r"(?::\s*(?:public|protected|private)?\s*([A-Za-z_][A-Za-z_0-9:]*))?"
                  r"[ \t]*(\{|$|//)", re.M)


def strip_block_comments(text):
    # keep line structure so line numbers stay valid
    # tokenise so that "//****" banners are treated as line comments, not block-comment openers
    pat = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"', re.S)

    def repl(m):
        tok = m.group(0)
        if tok.startswith("//") or tok.startswith("/*"):
            return "\n" * tok.count("\n")
        return tok
    return pat.sub(repl, text)


def scan(root, rel):
    path = os.path.join(root, rel)
    with open(path, "r", encoding="latin-1") as fh:
        text = strip_block_comments(fh.read().replace("\r\n", "\n").replace("\r", "\n"))
    rows = []
    for m in DECL.finditer(text):
        name, base = m.group(1), m.group(2) or ""
        line = text.count("\n", 0, m.start()) + 1
        # skip forward declarations such as "class Foo;" (regex already requires { or EOL)
        rest = text[m.end():m.end() + 200]
        if m.group(3) == "" and rest.lstrip().startswith(";"):
            continue
        rows.append((name, base, rel, line))
    return rows


def main():
    root, out = sys.argv[1], sys.argv[2]
    files = []
    for d in DIRS:
        for fn in sorted(os.listdir(os.path.join(root, d))):
            if fn.endswith((".h", ".cpp")):
                files.append(f"{d}/{fn}")
    files += EXTRA
    rows = []
    for rel in files:
        rows.extend(scan(root, rel))
    with open(out, "w", newline="") as fh:
        w = csv.writer(fh)
        w.writerow(["class", "base", "file", "line"])
        w.writerows(rows)
    print(len(rows), "class declarations")


if __name__ == "__main__":
    main()
