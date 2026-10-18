"""R4a: parse the CONVENTIONS section 8 naming registry and compare it with the sketch declarations.

Usage: python3 -I R4a_registry.py <conventions.md> <out_dir>
Reads <out_dir>/decls.tsv; writes <out_dir>/registry.tsv and <out_dir>/registry_compare.txt.
"""
import csv
import os
import re
import sys

conv, out = sys.argv[1], sys.argv[2]
lines = open(conv, encoding="utf-8").read().split("\n")
reg = {}  # name -> list of (section, owner, namespace, group, enumerators)
section = ""


def add(name, sec, owner, ns, group, enums=None):
    reg.setdefault(name, []).append((sec, owner, ns, group, enums or []))


for ln in lines:
    if ln.startswith("### "):
        section = ln[4:].strip()
    if not ln.startswith("|") or ln.startswith("|---"):
        continue
    cells = [c.strip() for c in ln.strip().strip("|").split("|")]
    if section.startswith("8.12") and len(cells) == 4 and cells[0] not in ("Owner", "Species term"):
        if cells[1] in ("engine type", "game type"):
            # table 8.12-A: term | kind | identifier | prefix
            for tok in re.findall(r"`([^`]+)`", cells[2]):
                add(tok, "8.12-A", cells[0], "Engine" if cells[1] == "engine type" else "Species", cells[1])
            continue
        owner, ns, group, names = cells
        if group == "data names":
            continue
        ns = ns.strip("`")
        # walk names: `Name` optionally followed by (`A`, `B`)
        pos = 0
        pat = re.compile(r"`([^`]+)`(\s*\(([^)]*)\))?")
        for m in pat.finditer(names):
            name = m.group(1)
            enums = []
            if m.group(3):
                enums = re.findall(r"`([^`]+)`", m.group(3))
                # skip enumerators being re-added as names: consume the parenthesised part
            add(name, "8.12-B", owner, ns, group, enums)
        # remove enumerator tokens that were added as standalone names by finditer overlap
    elif section[:4] in ("8.1 ", "8.2 ", "8.3 ", "8.4 ", "8.5 ", "8.6 ", "8.7 ", "8.8 ", "8.9 ", "8.10") and len(cells) == 4 and cells[0] != "Species term":
        for tok in re.findall(r"`([^`]+)`", cells[2]):
            add(tok, section.split()[0], cells[0], "Species" if tok != "UiCore" else "Engine", cells[1])

# The finditer above also matches enumerators inside parentheses as separate names when the
# parenthesised list contains backticks; drop names that only ever appear as enumerators.
enum_members = set()
for name, entries in reg.items():
    for e in entries:
        for en in e[4]:
            enum_members.add(en)
with open(os.path.join(out, "registry.tsv"), "w", encoding="utf-8") as g:
    g.write("name\tsection\towner\tnamespace\tgroup\tenumerators\n")
    for name in sorted(reg):
        for e in reg[name]:
            g.write("\t".join([name, e[0], e[1], e[2], e[3], ",".join(e[4])]) + "\n")

with open(os.path.join(out, "decls.tsv"), encoding="utf-8") as f:
    decls = list(csv.DictReader(f, delimiter="\t"))

TYPE_KINDS = {"class", "struct", "enum", "using", "forward", "concept", "union", "cbuffer"}
by_name = {}
for d in decls:
    by_name.setdefault(d["name"], []).append(d)

o = []
o.append("== Declared at namespace or file scope (type, function, constant, macro) but not registered ==")
for name, ds in sorted(by_name.items()):
    tops = [d for d in ds if d["kind"] in TYPE_KINDS | {"function", "constant", "macro"} and d["scope"] in ("namespace", "file")]
    if not tops:
        continue
    if name in reg or name in ("Engine", "Species"):
        continue
    locs = sorted(set(f"{d['file']}:{d['line']}({d['kind']},{d['namespace']})" for d in tops))
    o.append(f"{name}\t" + "; ".join(locs))
o.append("")
o.append("== Nested types declared (not at namespace scope) ==")
for name, ds in sorted(by_name.items()):
    for d in ds:
        if d["kind"] in TYPE_KINDS and d["scope"] not in ("namespace", "file"):
            o.append(f"{name}\t{d['file']}:{d['line']}\t{d['kind']} in {d['scope']}\treg={'yes' if name in reg else 'no'}")
o.append("")
o.append("== Namespace mismatch: registered layer vs declared namespace ==")
for name, ds in sorted(by_name.items()):
    if name not in reg:
        continue
    reg_ns = set(e[2] for e in reg[name] if e[2] in ("Engine", "Species"))
    for d in ds:
        if d["kind"] in TYPE_KINDS | {"function", "constant"} and d["scope"] in ("namespace", "file"):
            dns = d["namespace"].split("::")[0]
            if reg_ns and dns not in reg_ns:
                o.append(f"{name}\tregistered {sorted(reg_ns)}\tdeclared {d['namespace']} at {d['file']}:{d['line']}")
o.append("")
o.append("== Enumerator mismatch: registered enumerators vs declared ==")
for name, ds in sorted(by_name.items()):
    if name not in reg:
        continue
    for e in reg[name]:
        if not e[4]:
            continue
        for d in ds:
            if d["kind"] == "enum":
                declared = [x for x in d["detail"].split(",") if x]
                if declared != e[4]:
                    o.append(f"{name}\treg({e[1]})={e[4]}\tdecl {d['file']}:{d['line']}={declared}")
o.append("")
o.append("== Declared more than once (types at namespace scope) ==")
for name, ds in sorted(by_name.items()):
    tops = [d for d in ds if d["kind"] in TYPE_KINDS - {"forward"} and d["scope"] in ("namespace", "file")]
    files = sorted(set((d["file"], d["line"], d["kind"], d["namespace"], d["detail"][:120]) for d in tops))
    if len(files) > 1:
        o.append(name)
        for f in files:
            o.append("    " + " | ".join(f))
o.append("")
o.append("== Registered type names (8.12) never declared in any sketch ==")
for name in sorted(reg):
    if "::" in name or name in enum_members:
        continue
    if name not in by_name:
        o.append(f"{name}\t" + "; ".join(sorted(set(e[1] + '/' + e[2] for e in reg[name]))))
open(os.path.join(out, "registry_compare.txt"), "w", encoding="utf-8").write("\n".join(o) + "\n")
print(len(reg), "registered names")
