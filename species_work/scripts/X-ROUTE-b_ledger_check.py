"""Report which proposed names are already registered.

Usage: python3 -I X-ROUTE-b_ledger_check.py <rename_map.tsv> <CONVENTIONS.md> <name> [<name> ...]

A name is registered when it equals (case-insensitive) a species_term,
species_identifier or string_id_prefix cell of the ledger, or appears as a
backticked identifier or a first-column term of a CONVENTIONS §8 table.
"""
import csv
import re
import sys


def main(argv):
    if len(argv) < 4:
        print(__doc__)
        return 2
    known = set()
    with open(argv[1], encoding="utf-8") as fh:
        for row in csv.DictReader(fh, delimiter="\t"):
            for col in ("species_term", "species_identifier", "string_id_prefix"):
                for part in re.split(r"[;,]\s*", row.get(col) or ""):
                    part = part.strip().strip("`")
                    if part:
                        known.add(part.lower())
    with open(argv[2], encoding="utf-8") as fh:
        text = fh.read()
    for m in re.finditer(r"`([^`]+)`", text):
        known.add(m.group(1).lower())
    for line in text.splitlines():
        if line.startswith("| "):
            known.add(line[2:].split("|", 1)[0].strip().lower())
    have = miss = 0
    for name in argv[3:]:
        ok = name.lower() in known
        have += ok
        miss += not ok
        print(f"{'HAVE' if ok else 'MISS'}\t{name}")
    print(f"# {have} registered, {miss} missing")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
