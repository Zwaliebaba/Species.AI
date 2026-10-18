"""R4a: check every ledger identifier (Species columns only) against the registry and library use.

Usage: python3 -I R4a_ledger.py <ledger_species_cols.tsv> <design_root> <out_dir>
The input holds kind, species_term, species_identifier, string_id_prefix, rule (no original names).
"""
import csv
import os
import re
import sys

ledger, root, out = sys.argv[1], sys.argv[2], sys.argv[3]
reg = {}
with open(os.path.join(out, "registry.tsv"), encoding="utf-8") as f:
    for r in csv.DictReader(f, delimiter="\t"):
        reg.setdefault(r["name"], []).append(r)
corpus = {}
for dirpath, dirnames, filenames in os.walk(root):
    dirnames[:] = [d for d in dirnames if d != "requirements_index"]
    for fn in filenames:
        if fn.endswith(".md"):
            p = os.path.join(dirpath, fn)
            corpus[os.path.relpath(p, root)] = open(p, encoding="utf-8").read()
rows = []
with open(ledger, encoding="utf-8") as f:
    for line in f:
        parts = line.rstrip("\n").split("\t")
        if len(parts) < 5:
            continue
        kind, term, ident, prefix, rule = parts[:5]
        if rule != "rename" or not ident or ident in ("—", "-"):
            continue
        for tok in re.split(r"[,;] *", ident):
            tok = tok.strip().strip("`")
            if not re.match(r"^[A-Za-z_][A-Za-z0-9_:<>]*$", tok):
                continue
            uses = [k for k, v in corpus.items() if re.search(r"(?<![A-Za-z0-9_])" + re.escape(tok) + r"(?![A-Za-z0-9_])", v)]
            uses_wo = [u for u in uses if u != "CONVENTIONS.md"]
            rows.append((tok, term, "reg" if tok in reg else "UNREG", len(uses_wo), " ".join(sorted(uses_wo)[:3])))
with open(os.path.join(out, "ledger_check.tsv"), "w", encoding="utf-8") as g:
    for r in rows:
        g.write("\t".join(str(x) for x in r) + "\n")
print(len(rows))
