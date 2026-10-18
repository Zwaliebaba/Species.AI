"""V2 helper: check that the requirement stubs removed in round 2 are neither defined nor cited.

Usage: python3 -I species_work/scripts/V2_stubs.py <design root>
Read-only. Skips the generated index (requirements_index/ and REQUIREMENTS_INDEX.md).
"""

import os
import re
import sys

REMOVED = (
    ["AEN-%03d" % n for n in (101, 115, 122, 156, 158, 163, 198, 217, 227, 240, 241, 265, 305, 362)]
    + ["DAT-095", "DBG-005", "PLT-006", "PLT-113"]
    + ["R-META-%03d" % n for n in (70, 416, 418, 419, 420, 428, 438, 492, 499, 500, 501, 502, 503, 504)]
    + ["R-AUD-219", "R-AUD-408"]
)


def files(root):
    for base, dirs, names in os.walk(root):
        dirs[:] = [d for d in dirs if d != "requirements_index"]
        for name in names:
            if name == "REQUIREMENTS_INDEX.md":
                continue
            if name.endswith((".md", ".csv", ".tsv", ".json")):
                yield os.path.join(base, name)


def main():
    root = sys.argv[1]
    pats = {rid: re.compile(r"(?<![A-Z0-9-])" + re.escape(rid) + r"(?![0-9])") for rid in REMOVED}
    hits = {rid: [] for rid in REMOVED}
    for path in files(root):
        with open(path, encoding="utf-8") as f:
            for no, line in enumerate(f, 1):
                for rid, pat in pats.items():
                    if pat.search(line):
                        hits[rid].append("%s:%d: %s" % (os.path.relpath(path, root), no, line.strip()[:160]))
    print("removed stubs checked: %d" % len(REMOVED))
    bad = 0
    for rid in REMOVED:
        if hits[rid]:
            bad += 1
            print("HIT %s" % rid)
            for h in hits[rid]:
                print("   " + h)
    print("ids with hits: %d" % bad)


if __name__ == "__main__":
    main()
