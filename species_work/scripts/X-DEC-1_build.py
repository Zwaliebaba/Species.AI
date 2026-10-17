"""Validate the X-DEC-1 register, write its index table and the dec_map CSV.

Usage:
  python3 -I X-DEC-1_build.py <register.md> <dec_map.csv> <packages_dir>

The register is edited in place: the block between the scope paragraph and
"## 1 Headline verdicts" becomes the generated index table.
"""
import csv
import json
import os
import re
import sys

F40 = "40_carry_forward_assessment.md"
F41 = "41_risks_and_open_questions.md"

# Line-number helpers for the ORIG tables (verified with X-DEC-1_rowindex.py).
def D(n): return (F40, 42 + n)
def C(n): return (F40, 74 + n)
def RA(n): return (F40, 98 + n)          # 40 §2.1 R1-R12
def G(n): return (F40, 140 + n)
def B(n): return (F40, 267 + n)
def K(n): return (F40, 504 + n)
def F(n): return (F40, 536 + n)
def U40(n): return (F40, 681 + n)
def L40(n): return (F40, n)
def RK(n): return (F41, 72 + n)          # 41 §1.3 rank n
def RR(n): return (F41, 136 + n)         # 41 §2 risk Rnn
def L41(n): return (F41, n)

R15 = L40(118)
R25 = L40(128)
P2, P14, P15 = L40(376), L40(388), L40(389)

REFS = {
    1: [D(1), K(1), K(15)],
    2: [D(6), P2, B(1), B(3), F(1), C(11), L40(454)],
    3: [D(6), U40(2), RK(1), RR(13)],
    4: [U40(14)],
    5: [RR(12), RK(2), RK(3)],
    6: [RR(13), RR(15)],
    7: [F(24), L40(456), L41(89)],
    8: [F(29), G(12), K(8), C(3), RR(2), RA(7), L41(89)],
    9: [F(6), C(2), RR(6)],
    10: [C(9), F(29), RR(2), L40(456)],
    11: [F(4), F(46), F(29), C(8), RR(4), L40(455), L41(89)],
    12: [F(45), F(29), RR(6), RR(20), L40(457), L41(89)],
    13: [C(1), RR(3), B(13), RA(5), L40(457)],
    14: [P14, F(33), F(10), C(6), L40(460), L41(100), L41(103)],
    15: [C(5), L41(106), L41(123), RR(19)],
    16: [B(11), R15],
    17: [RA(1), K(15), K(19)],
    18: [RA(2), K(2), K(3), K(4), K(13), K(14), G(22), B(3), C(11)],
    19: [RA(3)],
    20: [RA(4)],
    21: [RA(5), K(9), K(10), K(11), G(17), G(18)],
    22: [RA(6), K(18), L41(121)],
    23: [RA(7)],
    24: [RA(8), K(21), R25],
    25: [RA(9), K(20)],
    26: [RA(11), G(34), G(38)],
    27: [RA(12), K(23), G(53), G(55), D(9)],
    28: [R15],
    29: [D(5), K(22), G(48), G(49), G(50), G(51), B(10)],
    30: [G(1), G(2), K(5)],
    31: [G(3), G(4), K(6), L41(107), L41(124)],
    32: [G(6), G(7), K(7)],
    33: [G(8), K(12)],
    34: [G(27)],
    35: [G(28), K(17)],
    36: [G(29), K(16)],
    37: [G(32), K(24)],
    38: [G(41)],
    39: [G(42)],
    40: [G(44)],
    41: [G(60), G(61), G(62), G(63), G(66), G(71), C(12), L41(127)],
    42: [B(9)],
    43: [RA(4)],
    44: [RA(11)],
    45: [D(10), RA(10), G(70), F(55)],
    46: [D(10), G(57), P15],
    47: [D(10), RA(11), G(36), F(7), F(9), U40(12)],
    48: [D(10), G(40), F(23)],
    49: [D(10), RA(12), G(54), F(49), L41(125)],
    50: [G(13), G(15), G(30), G(31), G(37)],
    51: [R25, G(33)],
    52: [D(10), G(69), L41(126)],
    53: [D(10), R25],
    54: [D(10), G(35)],
    55: [D(10), G(10)],
    56: [G(11)],
    57: [G(21)],
    58: [G(26)],
    59: [G(45), D(10)],
    60: [G(64), G(65), C(12)],
    61: [G(68)],
    62: [RA(3)],
    63: [B(12)],
    64: [F(2), B(2), L41(117)],
    65: [F(3)],
    66: [F(5), G(25)],
    67: [F(8)],
    68: [F(11)],
    69: [F(12)],
    70: [F(13)],
    71: [F(14)],
    72: [F(15)],
    73: [F(17), G(19)],
    74: [F(18)],
    75: [F(19)],
    76: [F(20)],
    77: [F(21)],
    78: [F(22)],
    79: [F(25), U40(3), RK(6)],
    80: [F(26), G(23), U40(4)],
    81: [F(27), G(16), U40(3), RK(10)],
    82: [F(28), G(20), U40(5), RK(7)],
    83: [F(32), G(9)],
    84: [F(34)],
    85: [F(35), G(24)],
    86: [F(36), G(5)],
    87: [F(37)],
    88: [F(38)],
    89: [F(38)],
    90: [F(39), B(5), L41(129)],
    91: [F(40)],
    92: [F(41), B(6)],
    93: [F(42), B(4)],
    94: [F(43), B(7), L40(467), U40(10), RK(5), L41(105), L41(122)],
    95: [F(44)],
    96: [F(47), G(14)],
    97: [F(48)],
    98: [F(50)],
    99: [F(51), RR(21)],
    100: [F(52)],
    101: [F(53), G(43)],
    102: [F(54)],
    103: [F(56)],
    104: [F(57), B(8)],
    105: [F(58)],
    106: [U40(1), RK(4), RR(17)],
    107: [RK(8), RR(15)],
    108: [RK(9), RR(14)],
    109: [RK(14)],
    110: [RK(15)],
    111: [RR(14)],
    112: [RR(16), RK(12)],
    113: [L41(97)],
    114: [L41(102), L41(103), L41(121)],
    115: [L41(99), L41(120)],
    116: [L41(104)],
    117: [L41(98), L41(101), L41(108), L41(118), L41(119)],
}

PREFIX_RE = re.compile(r"\b(R-[A-Z]+|TER|RND|AST|VFX|AEN|UI|INP|CAM|DAT|PLT|BLD|DBG)\b")
PATH_RE = re.compile(r"\b((?:reference|architecture)/[\w./-]+)")
FIELDS = ["Status", "Affects", "Specified behaviour", "Options", "Recommendation"]


def parse(text):
    entries = []
    blocks = re.split(r"(?m)^(?=### DEC-\d{3} )", text)
    for blk in blocks:
        m = re.match(r"### (DEC-(\d{3})) (.+)\n", blk)
        if not m:
            continue
        body = blk[m.end():].split("\n## ")[0]
        fields = re.findall(r"(?m)^- ([A-Za-z ]+): (.*)$", body)
        entries.append({"id": m.group(1), "num": int(m.group(2)),
                        "title": m.group(3).strip(), "fields": fields,
                        "body": body})
    return entries


def package_targets(pkg_dir):
    targets = set()
    for name in os.listdir(pkg_dir):
        if name.endswith(".json"):
            with open(os.path.join(pkg_dir, name), encoding="utf-8") as fh:
                for t in json.load(fh).get("targets", []):
                    targets.add(t)
    return targets


def main():
    reg, csv_path, pkg_dir = sys.argv[1:4]
    with open(reg, encoding="utf-8") as fh:
        text = fh.read()
    entries = parse(text)
    ids = {e["id"] for e in entries}
    targets = package_targets(pkg_dir)
    problems = []
    rows = []
    for e in entries:
        names = [f[0] for f in e["fields"]]
        status = dict(e["fields"]).get("Status", "")
        want = FIELDS + (["Decide by"] if status == "Open" else [])
        if names != want:
            problems.append(f"{e['id']}: fields {names}")
        if status not in ("Resolved", "Open", "Dropped"):
            problems.append(f"{e['id']}: status {status!r}")
        for cited in re.findall(r"DEC-\d{3}", e["body"]):
            if cited not in ids:
                problems.append(f"{e['id']}: cites missing {cited}")
        affects = dict(e["fields"]).get("Affects", "")
        prefixes = list(dict.fromkeys(PREFIX_RE.findall(affects)))
        paths = list(dict.fromkeys(PATH_RE.findall(affects)))
        for p in paths:
            if p not in targets and not any(t.startswith(p) or p.startswith(t.rstrip("/") + "/") for t in targets):
                problems.append(f"{e['id']}: path not a package target {p}")
        if not prefixes:
            problems.append(f"{e['id']}: no prefix")
        if e["num"] not in REFS:
            problems.append(f"{e['id']}: no orig refs")
        refs = ";".join(f"{f}:{n}-{n}" for f, n in REFS.get(e["num"], []))
        rows.append([e["id"], e["title"], status, ";".join(prefixes),
                     ";".join(paths) if paths else "reference/", refs])
    nums = [e["num"] for e in entries]
    if nums != sorted(nums) or len(set(nums)) != len(nums):
        problems.append("ids not unique and ascending")
    if any(n < 1 or n > 199 for n in nums):
        problems.append("id outside 001-199")

    index = ["## Index", "", "| ID | Title | Status | Affects |", "|---|---|---|---|"]
    for r in rows:
        index.append(f"| [{r[0]}](#{slug(r[0] + ' ' + r[1])}) | {r[1]} | {r[2]} | {r[3].replace(';', ', ')} |")
    block = "\n".join(index) + "\n\n"
    if "<!-- INDEX -->\n\n" in text:
        text = text.replace("<!-- INDEX -->\n\n", block)
    else:
        text = re.sub(r"(?s)## Index\n.*?(?=## 1 Headline verdicts)", block, text)
    with open(reg, "w", encoding="utf-8", newline="\n") as fh:
        fh.write(text)
    with open(csv_path, "w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["dec_id", "title", "status", "affects_prefix", "affects_files", "orig_refs"])
        w.writerows(rows)
    counts = {}
    for r in rows:
        counts[r[2]] = counts.get(r[2], 0) + 1
    print("entries", len(rows), counts, "bytes", len(text.encode("utf-8")))
    for p in problems:
        print("PROBLEM", p)


def slug(heading):
    s = heading.strip().lower()
    s = re.sub(r"[^\w\- ]", "", s)
    return s.replace(" ", "-")


if __name__ == "__main__":
    main()
