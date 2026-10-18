"""Renumber the New findings rows of a V1 review file and the "new finding N" references.

Usage: python3 -I V1_renumber.py <review.md>
Drops rows whose File cell is "(not used)", numbers the remaining rows 1..n in order,
and rewrites every "new finding N" / "new findings N and M" reference with the new numbers.
"""

import re
import sys


def main(path):
    with open(path, encoding="utf-8") as handle:
        lines = handle.read().split("\n")

    in_findings = False
    mapping = {}
    out = []
    next_number = 1
    for line in lines:
        if line.startswith("## New findings"):
            in_findings = True
        elif line.startswith("## ") and in_findings:
            in_findings = False
        if in_findings:
            match = re.match(r"^\| (\d+) \| ", line)
            if match:
                old = int(match.group(1))
                if "| (not used) |" in line:
                    continue
                mapping[old] = next_number
                line = "| %d | " % next_number + line[match.end():]
                next_number += 1
            elif line.startswith("Rows 4 and 9 are not used"):
                continue
        out.append(line)

    text = "\n".join(out)

    def repl_pair(match):
        return "new findings %d and %d" % (mapping[int(match.group(1))], mapping[int(match.group(2))])

    def repl_one(match):
        return "new finding %d" % mapping[int(match.group(1))]

    text = re.sub(r"new findings (\d+) and (\d+)", repl_pair, text)
    text = re.sub(r"new finding (\d+)", repl_one, text)
    text = re.sub(r"\(finding (\d+)\)", lambda m: "(finding %d)" % mapping[int(m.group(1))], text)
    text = text.replace("There are 16 new findings", "There are %d new findings" % (next_number - 1))

    with open(path, "w", encoding="utf-8", newline="\n") as handle:
        handle.write(text)
    print("renumbered:", mapping)


if __name__ == "__main__":
    main(sys.argv[1])
