"""R4a: collect backticked code-like tokens in the library and compare them with the registry and declarations.

Usage: python3 -I R4a_usage.py <design_root> <out_dir>
Writes <out_dir>/tokens.tsv (token, count, files) and <out_dir>/unknown_tokens.tsv, <out_dir>/registry_usage.tsv.
"""
import csv
import os
import re
import sys

root, out = sys.argv[1], sys.argv[2]
reg = {}
with open(os.path.join(out, "registry.tsv"), encoding="utf-8") as f:
    for r in csv.DictReader(f, delimiter="\t"):
        reg.setdefault(r["name"], []).append(r)
        for e in r["enumerators"].split(","):
            if e:
                reg.setdefault(e if "::" in e else r["name"] + "::" + e, []).append(r)
known = set(reg)
enum_members = {}
with open(os.path.join(out, "decls.tsv"), encoding="utf-8") as f:
    for d in csv.DictReader(f, delimiter="\t"):
        known.add(d["name"])
        if d["kind"] == "enum":
            for e in d["detail"].split(","):
                if e:
                    known.add(d["name"] + "::" + e)
                    enum_members.setdefault(e, set()).add(d["name"])
# all identifiers appearing in sketches (members, methods, enumerators)
sketch_idents = set()
with open(os.path.join(out, "blocks.tsv"), encoding="utf-8") as f:
    for r in csv.DictReader(f, delimiter="\t"):
        if r["lang"] in ("cpp", "hlsl"):
            t = open(os.path.join(out, "blocks", f"{int(r['n']):04d}.txt"), encoding="utf-8").read()
            sketch_idents.update(re.findall(r"[A-Za-z_][A-Za-z0-9_]*", t))

tok_re = re.compile(r"`([^`\n]+)`")
ident_re = re.compile(r"^(?:[A-Za-z_][A-Za-z0-9_]*::)*~?[A-Za-z_][A-Za-z0-9_]*(?:\(\))?$")
counts = {}
for dirpath, dirnames, filenames in os.walk(root):
    dirnames[:] = [d for d in dirnames if d != "requirements_index"]
    for fn in filenames:
        if not fn.endswith(".md"):
            continue
        path = os.path.join(dirpath, fn)
        rel = os.path.relpath(path, root)
        in_code = False
        for ln, line in enumerate(open(path, encoding="utf-8"), 1):
            if re.match(r"^\s*```", line):
                in_code = not in_code
                continue
            if in_code:
                continue
            for m in tok_re.finditer(line):
                tok = m.group(1).strip()
                if not ident_re.match(tok):
                    continue
                base = tok[:-2] if tok.endswith("()") else tok
                # code-like: has ::, or PascalCase with an inner capital, or UPPER_CASE with _, or ends in ()
                last = base.split("::")[-1]
                codey = ("::" in base) or tok.endswith("()") or re.match(r"^[A-Z][a-z0-9]+[A-Z]", last) or re.match(r"^[A-Z][A-Z0-9]*_[A-Z0-9_]+$", last) or re.match(r"^[a-z]+[A-Z]", last) or re.match(r"^m_|^g_|^sm_|^_[a-z]", last) or re.match(r"^[A-Z][a-z]+$", last)
                if not codey:
                    continue
                c = counts.setdefault(base, [0, set(), []])
                c[0] += 1
                c[1].add(rel)
                if len(c[2]) < 3:
                    c[2].append(f"{rel}:{ln}")
with open(os.path.join(out, "tokens.tsv"), "w", encoding="utf-8") as g:
    for t in sorted(counts):
        g.write(f"{t}\t{counts[t][0]}\t{len(counts[t][1])}\t{' '.join(counts[t][2])}\n")
unknown = []
for t in sorted(counts):
    last = t.split("::")[-1]
    if t in known or last in known or t in sketch_idents or last in sketch_idents:
        continue
    if re.match(r"^[A-Z][a-z]+$", t):
        continue  # single capitalised word: mostly prose terms (Worker) handled elsewhere
    unknown.append(t)
with open(os.path.join(out, "unknown_tokens.tsv"), "w", encoding="utf-8") as g:
    for t in unknown:
        g.write(f"{t}\t{counts[t][0]}\t{' '.join(counts[t][2])}\n")
with open(os.path.join(out, "registry_usage.tsv"), "w", encoding="utf-8") as g:
    for name in sorted(reg):
        c = counts.get(name)
        files = sorted(c[1]) if c else []
        files_wo = [x for x in files if x != "CONVENTIONS.md"]
        g.write(f"{name}\t{len(files_wo)}\t{'yes' if name in sketch_idents or name.split('::')[-1] in sketch_idents else 'no'}\t{' '.join(files_wo[:4])}\n")
print(len(counts), "tokens;", len(unknown), "unknown")
