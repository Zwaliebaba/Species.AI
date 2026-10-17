"""FX-03 helper: list tuning keys cited in some files that no tuning table defines.

Usage: python3 -I FX-03_keycheck.py <design_root> <file.md> [<file.md> ...]

A key is defined when a table row of any document under <design_root> starts
with "| `key` |". Wildcard keys (containing * or <) are skipped.
"""
import os
import re
import sys

KEY = re.compile(r"`((?:ref|rnd|vfx|aen|ter|cam|dat|plt|bld|dbg|inp|ast|ui)\.[a-z0-9_.]+)`")
ROW = re.compile(r"^\|\s*`([a-z0-9_.]+)`\s*\|")


def main() -> int:
    root, files = sys.argv[1], sys.argv[2:]
    defined = set()
    for dirpath, _dirs, names in os.walk(root):
        for name in names:
            if name.endswith(".md"):
                with open(os.path.join(dirpath, name), encoding="utf-8") as f:
                    for line in f:
                        m = ROW.match(line)
                        if m:
                            defined.add(m.group(1))
    missing = 0
    for path in files:
        with open(path, encoding="utf-8") as f:
            for n, line in enumerate(f, 1):
                for key in KEY.findall(line):
                    key = key.rstrip(".")
                    if key not in defined:
                        print(f"{path}:{n}: {key}")
                        missing += 1
    print(f"{missing} undefined key citations")
    return 0


if __name__ == "__main__":
    sys.exit(main())
