"""Aggregate terrain-survey figures from a markdown table range (no names in the output).

Usage: python3 -I X-DEC-2c_terrain_stats.py <md_file> <start_line> <end_line> [exclude_substring ...]
Rows whose first cell contains any exclude substring (case-insensitive) are skipped.
"""
import statistics
import sys
from pathlib import Path


def main() -> int:
    path = Path(sys.argv[1])
    start, end = int(sys.argv[2]), int(sys.argv[3])
    excl = [s.lower() for s in sys.argv[4:]]
    lines = path.read_text(encoding="utf-8").splitlines()[start - 1:end]
    rows = []
    digests = {}
    for ln in lines:
        if not ln.startswith("| `"):
            continue
        cells = [c.strip() for c in ln.strip().strip("|").split("|")]
        name = cells[0].strip("`").lower()
        if any(e in name for e in excl):
            continue
        n = int(cells[1].split("×")[0])
        cell = float(cells[2])
        extent = float(cells[3])
        tiles = int(cells[4])
        flat = int(cells[5])
        hmin = float(cells[6])
        hmax = float(cells[7])
        land = float(cells[8])
        dig = cells[9].strip("`")
        digests.setdefault(dig, 0)
        digests[dig] += 1
        rows.append((n, cell, extent, tiles, flat, hmin, hmax, land))
    if not rows:
        print("no rows")
        return 1
    cols = list(zip(*rows))
    names = ["grid_n", "cell_wu", "extent_wu", "tiles", "flatten", "h_min", "h_max", "land_pct"]
    print(f"maps={len(rows)} distinct_terrains={len(digests)} shared={sum(1 for v in digests.values() if v > 1)}")
    for nm, col in zip(names, cols):
        print(f"{nm}: min={min(col)} max={max(col)} median={statistics.median(col)} mean={statistics.mean(col):.2f}")
    print(f"tiles_total={sum(cols[3])} flatten_total={sum(cols[4])}")
    samples = [r[0] * r[0] for r in rows]
    print(f"samples: min={min(samples)} max={max(samples)} median={statistics.median(samples)}")
    area = [r[2] * r[2] / 1e6 for r in rows]
    print(f"area_Mwu2: min={min(area):.2f} max={max(area):.2f} median={statistics.median(area):.2f}")
    below = sum(1 for r in rows if r[5] < -10.0)
    print(f"maps_with_min_below_-10={below}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
