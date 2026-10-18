"""List tuning keys cited in the given files that no tuning-table row of the library defines.

Usage: python3 -I P6-DAT_keys.py <design root> <file.md> [...]  (read-only)
"""
import pathlib, re, sys
root = pathlib.Path(sys.argv[1])
defined = set()
for p in root.rglob("*.md"):
    for line in p.read_text(encoding="utf-8").split("\n"):
        m = re.match(r"\|\s*`([a-z0-9_.<>]+)`\s*\|", line)
        if m:
            defined.add(m.group(1))
pat = re.compile(r"`((?:ref|ter|rnd|ast|vfx|aen|ui|inp|cam|dat|plt|bld|dbg)\.[a-z0-9_.]+)`")
for name in sys.argv[2:]:
    for i, line in enumerate(pathlib.Path(name).read_text(encoding="utf-8").split("\n"), 1):
        for k in pat.findall(line):
            if k not in defined and not k.endswith("."):
                print(f"{name}:{i}: {k}")
