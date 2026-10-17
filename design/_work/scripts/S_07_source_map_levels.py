"""S_07: emit Source-map table rows (markdown) for every shipped level file that
places AI buildings (AITarget / AIObjective / AIObjectiveMarker), checking each
path exists in manifest.csv.
Usage: python3 -I S_07_source_map_levels.py <SRC>/data/levels <OUT>/_work/manifest.csv
"""
import csv
import os
import sys


def main(levels_dir, manifest):
    with open(manifest, newline="", encoding="utf-8") as fh:
        paths = {row["path"] for row in csv.DictReader(fh)}
    rows = []
    for f in sorted(os.listdir(levels_dir), key=str.lower):
        if not (f.lower().startswith("mp_") and f.lower().endswith(".txt")):
            continue
        n_t = n_o = n_m = 0
        opts = ""
        with open(os.path.join(levels_dir, f), encoding="latin-1") as fh:
            for line in fh:
                tok = line.split()
                if not tok:
                    continue
                if tok[0] == "AITarget":
                    n_t += 1
                elif tok[0] == "AIObjective":
                    n_o += 1
                elif tok[0] == "AIObjectiveMarker":
                    n_m += 1
                if "noradarcontrol" in line.lower():
                    opts = "; level option `noradarcontrol` (disables AI radar aiming)"
        if n_t + n_o + n_m == 0:
            continue
        rel = "data/levels/" + f
        flag = "" if rel in paths else " (NOT IN MANIFEST)"
        parts = [f"{n_t} AITarget"]
        if n_o:
            parts.append(f"{n_o} AIObjective")
        if n_m:
            parts.append(f"{n_m} AIObjectiveMarker")
        rows.append(f"| `{rel}`{flag} | level data: places {', '.join(parts)}{opts} |")
    print("\n".join(rows))
    print(f"<!-- {len(rows)} level rows -->", file=sys.stderr)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1], sys.argv[2]))
