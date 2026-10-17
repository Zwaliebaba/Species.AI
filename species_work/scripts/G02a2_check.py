"""G02a2 self-check: ids, keys, DECs, assets, headings, truncation words.

Usage: python3 -I G02a2_check.py <repo_root> <file> [<file> ...]
"""
import csv
import re
import sys
from pathlib import Path

root = Path(sys.argv[1])
files = [Path(p) for p in sys.argv[2:]]

heads = ["## 1 Purpose and player experience", "## 2 Rules", "## 3 States and flows",
         "## 4 Tuning table", "## 5 Edge cases and interactions", "## 6 Acceptance criteria",
         "## 7 Dependencies", "## 8 Open decisions"]

dec_index = (root / "design_next/decisions/README.md").read_text(encoding="utf-8")
dec_ids = set(re.findall(r"\| (DEC-\d{3}) \|", dec_index))
dec_status = dict(re.findall(r"\| (DEC-\d{3}) \| .*? \| (Open|Resolved|Dropped) \|", dec_index))
adr_ids = {m.group(1) for m in re.finditer(r"ADR-(\d{4})", (root / "design_next/adr/README.md").read_text(encoding="utf-8"))}
assets = set()
with open(root / "design_next/assets/asset_manifest.csv", newline="", encoding="utf-8") as fh:
    for row in csv.DictReader(fh):
        assets.add(row["asset_id"])

defined_ids = {}
key_defs = {}
problems = []
all_text = {}
for f in files:
    t = f.read_text(encoding="utf-8")
    all_text[f] = t
    for h in heads:
        if h not in t:
            problems.append(f"{f.name}: missing heading {h}")
    for m in re.finditer(r"\*\*(R-UNIT-(\d{3}))\*\* \((MUST|SHOULD|MAY|MUST NOT|SHOULD NOT)\)", t):
        rid, n = m.group(1), int(m.group(2))
        if not 300 <= n <= 499:
            problems.append(f"{f.name}: {rid} out of range")
        if rid in defined_ids:
            problems.append(f"{f.name}: {rid} duplicate (also {defined_ids[rid]})")
        defined_ids[rid] = f.name
    sec4 = t.split("## 4 Tuning table", 1)[1].split("## 5 ", 1)[0]
    for line in sec4.splitlines():
        m = re.match(r"\| `(ref\.unit\.[a-z0-9_.]+)` \|", line)
        if m:
            k = m.group(1)
            if k in key_defs:
                problems.append(f"{f.name}: key {k} duplicate (also {key_defs[k]})")
            key_defs[k] = f.name
    for w in ["etc.", "and so on", "and others", "various", "similar to the above", "…"]:
        if w in t:
            problems.append(f"{f.name}: truncation word {w!r}")
    # mermaid captions
    blocks = [m.end() for m in re.finditer(r"```mermaid[\s\S]*?```", t)]
    for end in blocks:
        after = t[end:end + 200].lstrip("\n")
        if not after.startswith("*Figure"):
            problems.append(f"{f.name}: mermaid block without caption at {end}")

ids_sorted = sorted(int(r[-3:]) for r in defined_ids)
gaps = [n for n in range(ids_sorted[0], ids_sorted[-1] + 1) if f"R-UNIT-{n}" not in defined_ids]

used_keys = set()
for f, t in all_text.items():
    for m in re.finditer(r"`(ref\.unit\.[a-z0-9_.]+)`", t):
        used_keys.add((f.name, m.group(1)))
    for m in re.finditer(r"\bR-UNIT-(\d{3})\b", t):
        rid = f"R-UNIT-{m.group(1)}"
        if rid not in defined_ids:
            problems.append(f"{f.name}: reference to undefined {rid}")
    for m in re.finditer(r"\bDEC-(\d{3})\b", t):
        if f"DEC-{m.group(1)}" not in dec_ids:
            problems.append(f"{f.name}: unknown DEC-{m.group(1)}")
    for m in re.finditer(r"\bADR-(\d{4})\b", t):
        if m.group(1) not in adr_ids:
            problems.append(f"{f.name}: unknown ADR-{m.group(1)}")
    for m in re.finditer(r"`((?:fx|icon|mesh|sfx|marker|sprite|ui|help|glyph|font|cursor)\.[a-z0-9_.]+)`", t):
        if m.group(1) not in assets:
            problems.append(f"{f.name}: unknown asset {m.group(1)}")

external_unit_keys = sorted({k for _, k in used_keys if k not in key_defs})
joined = "\n".join(all_text.values())
unused_defs = sorted(k for k in key_defs if joined.count("`" + k + "`") < 2)

print("requirements:", len(defined_ids), "range", ids_sorted[0], "-", ids_sorted[-1], "gaps", gaps)
print("own tuning keys:", len(key_defs))
for f in files:
    t = all_text[f]
    print(f" {f.name}: reqs {len(re.findall(r'\*\*R-UNIT-', t))}, keys {sum(1 for v in key_defs.values() if v == f.name)},"
          f" mermaid {t.count('```mermaid')}, tables {len(re.findall(r'(?m)^\|---', t))}, ACs {len(re.findall(r'\*\*AC-', t))}")
print("ref.unit keys used but not defined here:", external_unit_keys)
print("own keys appearing only in the table:", unused_defs)
decs = set()
for t in all_text.values():
    decs |= set(re.findall(r"\bDEC-\d{3}\b", t))
print("DECs cited:", sorted(decs))
print("Open DECs cited:", sorted(d for d in decs if dec_status.get(d) == "Open"))
print("problems:")
for p in problems:
    print(" ", p)
