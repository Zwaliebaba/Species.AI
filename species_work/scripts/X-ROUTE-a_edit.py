"""Edit fields of existing rows in the X-ROUTE-a CSV.

Usage: python3 -I X-ROUTE-a_edit.py <routes.csv> <edits.json>

<edits.json> holds a list of objects {"route_id": "RA-nnn", "<field>": "<new
value>", ...}; a field value may also be {"old": "...", "new": "..."} to
replace a substring (it must occur exactly once). The file is rewritten in
place with the same column order.
"""
import csv
import json
import sys


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    path, edits_path = argv[1], argv[2]
    with open(path, newline="", encoding="utf-8") as fh:
        reader = csv.DictReader(fh)
        cols = reader.fieldnames
        rows = list(reader)
    with open(edits_path, encoding="utf-8") as fh:
        edits = json.load(fh)
    index = {r["route_id"]: r for r in rows}
    for e in edits:
        rid = e["route_id"]
        if rid not in index:
            raise SystemExit(f"no row {rid}")
        row = index[rid]
        for k, v in e.items():
            if k == "route_id":
                continue
            if k not in cols:
                raise SystemExit(f"bad field {k}")
            if isinstance(v, dict):
                n = row[k].count(v["old"])
                if n != 1:
                    raise SystemExit(f"{rid}.{k}: substring occurs {n} times")
                row[k] = row[k].replace(v["old"], v["new"])
            else:
                row[k] = v
    with open(path, "w", newline="", encoding="utf-8") as fh:
        w = csv.DictWriter(fh, fieldnames=cols, quoting=csv.QUOTE_MINIMAL, lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    print(f"edited {len(edits)} rows")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
