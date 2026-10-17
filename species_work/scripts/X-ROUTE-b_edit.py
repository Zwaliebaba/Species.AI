"""Update fields of existing rows of the X-ROUTE-b CSV.

Usage: python3 -I X-ROUTE-b_edit.py <routes.csv> <edits.json>

<edits.json> holds a list of objects, each with `route_id` and the fields to
replace (any of source, to_package, to_file, kind, instruction, status, note).
"""
import csv
import json
import sys

COLUMNS = ["route_id", "source", "to_package", "to_file", "kind",
           "instruction", "status", "note"]


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    with open(argv[1], newline="", encoding="utf-8") as fh:
        rows = list(csv.DictReader(fh))
    with open(argv[2], encoding="utf-8") as fh:
        edits = json.load(fh)
    index = {r["route_id"]: r for r in rows}
    for e in edits:
        row = index.get(e["route_id"])
        if row is None:
            raise SystemExit(f"unknown route id {e['route_id']}")
        for k, v in e.items():
            if k == "route_id":
                continue
            if k not in COLUMNS:
                raise SystemExit(f"bad field {k}")
            row[k] = v
    with open(argv[1], "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=COLUMNS, lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    print(f"updated {len(edits)} rows")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
