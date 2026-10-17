"""Print the ORIG line ranges of a work package, optionally filtered by file.

Usage: python3 -I X-DEC-2c_extract.py <package.json> <orig_dir> [file_substring]
"""
import json
import sys
from pathlib import Path


def main() -> int:
    pkg = json.loads(Path(sys.argv[1]).read_text(encoding="utf-8"))
    orig = Path(sys.argv[2])
    flt = sys.argv[3] if len(sys.argv) > 3 else None
    cache = {}
    for row in pkg["inputs"]:
        name = row["file"]
        if flt and flt not in name:
            continue
        if name not in cache:
            cache[name] = (orig / name).read_text(encoding="utf-8").splitlines()
        lines = cache[name]
        s, e = int(row["start_line"]), int(row["end_line"])
        print(f"===== {name}:{s}-{e} | {row['note'][:160]}")
        for i in range(s - 1, min(e, len(lines))):
            print(f"{i + 1}: {lines[i]}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
