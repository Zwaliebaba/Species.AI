"""R4a: run clang-format (repo .clang-format) over every cpp/hlsl block and report drift.

Usage: python3 -I R4a_format.py <out_dir> <clang_format_config>
Reads <out_dir>/blocks.tsv and blocks/, writes <out_dir>/format/<n>.diff and prints a summary.
"""
import csv
import difflib
import os
import subprocess
import sys

out, cfg = sys.argv[1], sys.argv[2]
os.makedirs(os.path.join(out, "format"), exist_ok=True)
with open(os.path.join(out, "blocks.tsv"), encoding="utf-8") as f:
    rows = list(csv.DictReader(f, delimiter="\t"))
for r in rows:
    if r["lang"] not in ("cpp", "hlsl"):
        continue
    n = int(r["n"])
    src = open(os.path.join(out, "blocks", f"{n:04d}.txt"), encoding="utf-8").read()
    res = subprocess.run(["clang-format", f"--style=file:{cfg}", "--assume-filename=x.h"], input=src, capture_output=True, text=True)
    fmt = res.stdout
    a = src.splitlines()
    b = fmt.splitlines()
    diff = list(difflib.unified_diff(a, b, lineterm="", n=0))
    changed = sum(1 for d in diff if d.startswith("-") and not d.startswith("---"))
    long_lines = sum(1 for l in a if len(l) > 140)
    tabs = sum(1 for l in a if "\t" in l)
    with open(os.path.join(out, "format", f"{n:04d}.diff"), "w", encoding="utf-8") as g:
        g.write("\n".join(diff) + "\n")
    print(f"{n}\t{r['file']}:{r['start']}\tlines={len(a)}\tchanged={changed}\tlong={long_lines}\ttabs={tabs}")
