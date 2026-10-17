"""List which DEC entries cite each ORIG row of the X-DEC-1 package.

Usage: python3 -I X-DEC-1_coverage.py <dec_map.csv>
"""
import csv
import sys

F40 = "40_carry_forward_assessment.md"
F41 = "41_risks_and_open_questions.md"
ROWS = {}
for n in range(1, 11): ROWS[f"D{n}"] = (F40, 42 + n)
for n in range(1, 13): ROWS[f"C{n}"] = (F40, 74 + n)
for n in range(1, 13): ROWS[f"R{n}"] = (F40, 98 + n)
ROWS["R15"] = (F40, 118); ROWS["R25"] = (F40, 128)
for n in range(1, 73): ROWS[f"G{n}"] = (F40, 140 + n)
for n in range(1, 14): ROWS[f"B{n}"] = (F40, 267 + n)
ROWS["P2"] = (F40, 376); ROWS["P14"] = (F40, 388); ROWS["P15"] = (F40, 389)
for n in range(454, 468): ROWS[f"4.6@{n}"] = (F40, n)
for n in range(1, 26): ROWS[f"K{n}"] = (F40, 504 + n)
for n in range(1, 60): ROWS[f"F{n}"] = (F40, 536 + n)
for n in range(1, 18): ROWS[f"40-U{n}"] = (F40, 681 + n)
for n in range(1, 16): ROWS[f"rank{n}"] = (F41, 72 + n)
for n in range(1, 30): ROWS[f"risk-R{n:02d}"] = (F41, 136 + n)
for n in list(range(97, 110)) + list(range(117, 128)) + [89, 129]: ROWS[f"41@{n}"] = (F41, n)

cited = {}
with open(sys.argv[1], encoding="utf-8") as fh:
    for row in csv.DictReader(fh):
        for ref in row["orig_refs"].split(";"):
            f, span = ref.rsplit(":", 1)
            a, b = (int(x) for x in span.split("-"))
            for n in range(a, b + 1):
                cited.setdefault((f, n), []).append(row["dec_id"])
for key, loc in ROWS.items():
    print(f"{key}\t{','.join(cited.get(loc, [])) or '-'}")
