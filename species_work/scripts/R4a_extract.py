"""R4a: extract every fenced code block from the library (except requirements_index).

Usage: python3 -I R4a_extract.py <design_root> <out_dir>
Writes <out_dir>/blocks/<n>.txt and <out_dir>/blocks.tsv (n, file, start_line, end_line, lang, heading, guess).
"""
import os
import re
import sys

root, out = sys.argv[1], sys.argv[2]
os.makedirs(os.path.join(out, "blocks"), exist_ok=True)
rows = []
n = 0
cpp_hint = re.compile(r"\b(struct|class|enum class|namespace|std::|void|const|uint32_t|bool|#include|template|noexcept|\[\[nodiscard\]\])\b")
for dirpath, dirnames, filenames in os.walk(root):
    dirnames[:] = sorted(d for d in dirnames if d != "requirements_index")
    for fn in sorted(filenames):
        if not fn.endswith(".md"):
            continue
        path = os.path.join(dirpath, fn)
        rel = os.path.relpath(path, root)
        with open(path, encoding="utf-8") as f:
            lines = f.read().split("\n")
        heading = ""
        i = 0
        while i < len(lines):
            line = lines[i]
            m = re.match(r"^(\s*)(`{3,})([A-Za-z0-9+_-]*)\s*$", line)
            if line.startswith("#"):
                heading = line.strip()
            if m:
                indent, fence, lang = m.group(1), m.group(2), m.group(3)
                start = i + 1
                j = i + 1
                body = []
                while j < len(lines) and not re.match(r"^\s*" + fence + r"\s*$", lines[j]):
                    body.append(lines[j][len(indent):] if lines[j].startswith(indent) else lines[j])
                    j += 1
                n += 1
                text = "\n".join(body) + "\n"
                guess = lang or ("cpp?" if len(cpp_hint.findall(text)) >= 3 else "")
                with open(os.path.join(out, "blocks", f"{n:04d}.txt"), "w", encoding="utf-8") as g:
                    g.write(text)
                rows.append((n, rel, start, j + 1, lang, heading, guess, len(body), indent))
                i = j + 1
                continue
            i += 1
with open(os.path.join(out, "blocks.tsv"), "w", encoding="utf-8") as g:
    g.write("n\tfile\tstart\tend\tlang\theading\tguess\tlines\tindent\n")
    for r in rows:
        g.write("\t".join(str(x) if not isinstance(x, str) else x.replace("\t", " ") for x in r[:8]) + "\t" + repr(r[8]) + "\n")
print(n, "blocks")
