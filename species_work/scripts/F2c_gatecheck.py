"""Read-only replica of the G1 checks of check_routing.py, for routing_F2c.csv only.

Usage: python3 -I F2c_gatecheck.py <WORK dir> <ORIG dir>
Imports the constants of check_routing.py (its main() is not run, so the merged
routing.csv and package_budget.csv are not written).
"""
import csv
import importlib.util
import os
import re
import sys
from collections import defaultdict


def main():
    work, orig = sys.argv[1], sys.argv[2]
    spec = importlib.util.spec_from_file_location("cr", os.path.join(work, "scripts", "check_routing.py"))
    cr = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cr)
    codes = cr.codes_from(os.path.join(work, "drop_topics.md"))
    errors = []
    with open(os.path.join(work, "routing_F2c.csv"), encoding="utf-8", newline="") as f:
        r = csv.DictReader(f)
        if r.fieldnames != cr.COLS:
            errors.append(f"columns {r.fieldnames}")
        rows = list(r)
    for n, row in enumerate(rows, start=2):
        act = row["action"].strip()
        if act not in cr.ACTIONS:
            errors.append(f"{n}: action {act}")
        s, e = int(row["start_line"]), int(row["end_line"])
        row["_s"], row["_e"] = s, e
        if s > e:
            errors.append(f"{n}: start > end")
        if act in cr.NEEDS_CODE:
            m = re.match(r"((?:DL|DD|KH|DP)-[A-Z0-9-]+)", row["note"].strip())
            if not m or not m.group(1).startswith(cr.NEEDS_CODE[act]):
                errors.append(f"{n}: {act} note code")
            elif m.group(1) not in codes:
                errors.append(f"{n}: unknown code {m.group(1)}")
        if act in cr.NEEDS_TARGET and not row["target_file"].strip():
            errors.append(f"{n}: {act} without target_file")
        pkg = row["owner_package"].strip()
        if act.startswith("DROP"):
            continue
        if not pkg:
            errors.append(f"{n}: {act} without owner")
        elif pkg not in cr.PACKAGES and not re.match(r"^(G\d\d[a-z]?\d?|C-[A-Z]+\d?)$", pkg):
            errors.append(f"{n}: owner {pkg}")
    by_file = defaultdict(list)
    for row in rows:
        by_file[row["orig_file"]].append(row)
    for f, frows in sorted(by_file.items()):
        lines = open(os.path.join(orig, f), encoding="utf-8").read().split("\n")
        first = next((i for i, l in enumerate(lines, 1) if l.startswith("# ")), 1)
        last = len(lines)
        while last > first and not lines[last - 1].strip():
            last -= 1
        spans = sorted({(x["_s"], x["_e"]) for x in frows})
        cursor = first
        for s, e in spans:
            if s > cursor:
                errors.append(f"{f}: lines {cursor}-{s - 1} not routed")
            elif s < cursor:
                errors.append(f"{f}: range {s}-{e} overlaps previous (ends {cursor - 1})")
            cursor = max(cursor, e + 1)
        if cursor <= last:
            errors.append(f"{f}: lines {cursor}-{last} not routed")
        print(f"{f}: rows={len(frows)} spans={len(spans)} first={first} last_nonblank={last}")
    pk = defaultdict(int)
    for row in rows:
        if row["action"] in ("KEEP", "MERGE", "KEEP-HOOKS"):
            pk[row["owner_package"]] += int(row["bytes"])
    print("keep/merge/hooks KB per owner:", {p: round(b / 1024, 1) for p, b in sorted(pk.items())})
    print(f"errors={len(errors)}")
    for e in errors[:50]:
        print("ERROR", e)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
