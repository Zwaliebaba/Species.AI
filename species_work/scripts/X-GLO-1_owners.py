"""X-GLO-1 helper: map glossary rows to the owning target file of the new set.

Usage: python3 -I X-GLO-1_owners.py <repo_root> <glossary_file> <start_line> <end_line> [extra_start extra_end ...]

For every table row in the given line ranges of the glossary, take each
primary-document link (file#anchor), find the heading line in that ORIG file,
and report which phase-3 package input covers that line and its target file.
"""
import json
import glob
import os
import re
import sys


def slug(text):
    s = text.strip().lower()
    s = re.sub(r"[^\w\- ]", "", s)
    s = s.replace(" ", "-")
    return s


def heading_lines(path):
    out = {}
    counts = {}
    with open(path, encoding="utf-8") as f:
        in_code = False
        for n, line in enumerate(f, 1):
            if line.startswith("```"):
                in_code = not in_code
                continue
            if in_code:
                continue
            m = re.match(r"^(#{1,6})\s+(.*)$", line.rstrip("\n"))
            if m:
                s = slug(m.group(2))
                if s in counts:
                    counts[s] += 1
                    s2 = f"{s}-{counts[s]}"
                else:
                    counts[s] = 0
                    s2 = s
                out[s2] = n
    return out


def main():
    root = sys.argv[1]
    gloss = sys.argv[2]
    ranges = [(int(sys.argv[i]), int(sys.argv[i + 1])) for i in range(3, len(sys.argv), 2)]
    design = os.path.join(root, "design")
    pk_inputs = []
    for p in glob.glob(os.path.join(root, "species_work", "packages", "*.json")):
        d = json.load(open(p, encoding="utf-8"))
        if d.get("phase") != 3 or d["id"].startswith("X-"):
            continue
        for i in d["inputs"]:
            pk_inputs.append((i["file"], i["start_line"], i["end_line"], d["id"], i.get("target_file"), i["action"]))
    cache = {}
    lines = open(gloss, encoding="utf-8").read().split("\n")
    for a, b in ranges:
        for n in range(a, b + 1):
            line = lines[n - 1]
            if not line.startswith("| ") or line.startswith("| Term") or line.startswith("|---"):
                continue
            cells = [c.strip() for c in line.split(" | ")]
            term = cells[0].lstrip("| ").strip()
            links = re.findall(r"\]\(([0-9]{2}_[\w]+\.md)(?:#([^)]+))?\)", line)
            res = []
            for f, anc in links:
                fp = os.path.join(design, f)
                if not os.path.exists(fp):
                    continue
                if f not in cache:
                    cache[f] = heading_lines(fp)
                hl = cache[f].get(anc) if anc else 1
                if hl is None:
                    res.append(f"{f}#{anc}=?")
                    continue
                hits = [x for x in pk_inputs if x[0] == f and x[1] <= hl <= x[2]]
                if hits:
                    res.append(";".join(f"{h[3]}:{h[4]}({h[5]})" for h in hits))
                else:
                    res.append(f"{f[:2]}@{hl}=none")
            print(f"{n}\t{term}\t" + " || ".join(res))


if __name__ == "__main__":
    main()
