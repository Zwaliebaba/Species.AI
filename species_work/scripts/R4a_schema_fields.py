"""R4a: compare schema field keys of the data-schema tables with the fields that sketches declare.

Usage: python3 -I R4a_schema_fields.py <design_root> <out_dir>
A table whose caption names a type in backticks, for example "Table 3.4-B Fields of a terrain tile (`TerrainTile`)",
is matched with the struct of that name in decls.tsv. A key `position_x` generates the field `positionX` (ADR-062).
"""
import csv
import os
import re
import sys

root, out = sys.argv[1], sys.argv[2]
fields = {}
with open(os.path.join(out, "decls.tsv"), encoding="utf-8") as f:
    for d in csv.DictReader(f, delimiter="\t"):
        if d["kind"] == "field" and d["scope"].startswith(("struct ", "class ")):
            t = d["scope"].split(" ", 1)[1]
            fields.setdefault(t, {}).setdefault(d["file"], []).append(d["name"])


def camel(k):
    parts = k.split("_")
    return parts[0] + "".join(p[:1].upper() + p[1:] for p in parts[1:])


for dirpath, dirnames, filenames in os.walk(root):
    dirnames[:] = [d for d in dirnames if d != "requirements_index"]
    for fn in sorted(filenames):
        if not fn.endswith(".md"):
            continue
        p = os.path.join(dirpath, fn)
        rel = os.path.relpath(p, root)
        lines = open(p, encoding="utf-8").read().split("\n")
        for i, ln in enumerate(lines):
            m = re.match(r"^Table (\S+) .*\(`([A-Z][A-Za-z0-9]+)`\)", ln)
            if not m:
                continue
            table, typ = m.group(1), m.group(2)
            j = i + 1
            while j < len(lines) and not lines[j].startswith("|"):
                j += 1
            if j >= len(lines):
                continue
            header = [c.strip() for c in lines[j].strip().strip("|").split("|")]
            if "Key" not in header:
                continue
            k = header.index("Key")
            keys = []
            j += 2
            while j < len(lines) and lines[j].startswith("|"):
                cells = [c.strip() for c in lines[j].strip().strip("|").split("|")]
                if len(cells) > k:
                    for key in re.findall(r"`([a-z0-9_]+)`", cells[k]):
                        keys.append(key)
                j += 1
            gen = [camel(x) for x in keys]
            if typ in fields:
                for f2, names in fields[typ].items():
                    missing = [g for g in gen if g not in names]
                    extra = [n for n in names if n not in gen]
                    print(f"{rel} table {table} `{typ}` vs {f2}: schema-only={missing} sketch-only={extra}")
            else:
                print(f"{rel} table {table} `{typ}`: no sketch fields ({len(keys)} keys)")
