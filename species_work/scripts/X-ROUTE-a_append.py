"""Append routing rows to the X-ROUTE-a CSV.

Usage: python3 -I X-ROUTE-a_append.py <routes.csv> <rows.json>

<rows.json> holds a list of objects with the keys source, to_package, to_file,
kind, instruction, status, note. Route ids continue from the last RA-nnn in
<routes.csv>; the file and its header are created when missing.
"""
import csv
import json
import os
import sys

COLUMNS = ["route_id", "source", "to_package", "to_file", "kind",
           "instruction", "status", "note"]
KINDS = {"cite", "fix", "add", "remove", "glossary", "ledger", "conventions"}
STATUSES = {"open", "done", "superseded"}


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    routes_path, rows_path = argv[1], argv[2]
    last = 0
    if os.path.exists(routes_path):
        with open(routes_path, newline="", encoding="utf-8") as fh:
            for row in csv.DictReader(fh):
                rid = row.get("route_id", "")
                if rid.startswith("RA-"):
                    last = max(last, int(rid[3:]))
    with open(rows_path, encoding="utf-8") as fh:
        rows = json.load(fh)
    for i, row in enumerate(rows):
        missing = [c for c in COLUMNS[1:] if c not in row]
        if missing:
            raise SystemExit(f"row {i}: missing {missing}")
        if row["kind"] not in KINDS:
            raise SystemExit(f"row {i}: bad kind {row['kind']}")
        if row["status"] not in STATUSES:
            raise SystemExit(f"row {i}: bad status {row['status']}")
        if row["status"] != "open" and not row["note"].strip():
            raise SystemExit(f"row {i}: {row['status']} needs evidence in note")
    new_file = not os.path.exists(routes_path)
    with open(routes_path, "a", newline="", encoding="utf-8") as fh:
        writer = csv.writer(fh, quoting=csv.QUOTE_MINIMAL, lineterminator="\n")
        if new_file:
            writer.writerow(COLUMNS)
        for row in rows:
            last += 1
            writer.writerow([f"RA-{last:03d}"] + [row[c] for c in COLUMNS[1:]])
    print(f"appended {len(rows)} rows; last id RA-{last:03d}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
