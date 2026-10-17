#!/usr/bin/env python3
"""X-DEC-4a: build the index table of the register file and the DEC map CSV.

Usage: python3 -I X-DEC-4a_build.py <register.md> <dec_map.csv>

The index is regenerated between the "## Index" heading and the first "## 1 " heading,
so the script can run any number of times.
"""
import csv
import re
import sys

ORIG_REFS = {
    "DEC-450": ["01_game_modes.md:206-207", "01_game_modes.md:236-237"],
    "DEC-451": ["01_game_modes.md:239-274"],
    "DEC-452": ["01_game_modes.md:311-334"],
    "DEC-453": ["00_overview.md:289-289", "05_economy_and_population.md:338-338",
                "05_economy_and_population.md:907-907"],
    "DEC-454": ["05_economy_and_population.md:331-334", "05_economy_and_population.md:454-454",
                "05_economy_and_population.md:806-806"],
    "DEC-455": ["04_crates_powerups_and_weapons.md:1467-1467", "05_economy_and_population.md:790-790",
                "05_economy_and_population.md:796-796", "05_economy_and_population.md:835-835"],
    "DEC-456": ["03_buildings_and_world_objects.md:2610-2610"],
    "DEC-457": ["03_buildings_and_world_objects.md:2619-2619"],
    "DEC-458": ["03_buildings_and_world_objects.md:2617-2617"],
    "DEC-459": ["03_buildings_and_world_objects.md:2618-2618"],
    "DEC-460": ["03_buildings_and_world_objects.md:2632-2632"],
    "DEC-461": ["03_buildings_and_world_objects.md:2640-2640"],
    "DEC-462": ["03_buildings_and_world_objects.md:2636-2636"],
    "DEC-463": ["03_buildings_and_world_objects.md:2625-2625"],
    "DEC-464": ["03_buildings_and_world_objects.md:2641-2641"],
    "DEC-465": ["04_crates_powerups_and_weapons.md:1468-1468"],
    "DEC-466": ["03_buildings_and_world_objects.md:2626-2626", "03_buildings_and_world_objects.md:2635-2635",
                "03_buildings_and_world_objects.md:2637-2638"],
}

HEAD = re.compile(r"^### (DEC-\d{3}) (.+)$")
STATUS = re.compile(r"^- Status: (Resolved|Open|Dropped)\s*$")
AFFECTS = re.compile(r"^- Affects: (.+)$")
ITEM = re.compile(r"([A-Za-z0-9_./-]+\.md) \(([A-Z-]+)\)")


def slug(text):
    s = text.lower()
    s = re.sub(r"[^\w\s-]", "", s)
    return re.sub(r"\s", "-", s.strip())


def parse(lines):
    entries, cur = [], None
    for line in lines:
        m = HEAD.match(line)
        if m:
            cur = {"id": m.group(1), "title": m.group(2).strip(), "status": "", "prefixes": [], "files": []}
            entries.append(cur)
            continue
        if cur is None:
            continue
        m = STATUS.match(line)
        if m:
            cur["status"] = m.group(1)
        m = AFFECTS.match(line)
        if m:
            for f, p in ITEM.findall(m.group(1)):
                if f not in cur["files"]:
                    cur["files"].append(f)
                if p not in cur["prefixes"]:
                    cur["prefixes"].append(p)
    return entries


def main():
    md_path, csv_path = sys.argv[1], sys.argv[2]
    with open(md_path, encoding="utf-8") as fh:
        text = fh.read()
    entries = parse(text.splitlines())
    ids = [e["id"] for e in entries]
    assert len(ids) == len(set(ids)), "duplicate DEC id"
    for e in entries:
        assert e["status"], e["id"] + " has no status"
        assert e["files"], e["id"] + " has no affects"
        assert e["id"] in ORIG_REFS, e["id"] + " has no orig refs"
        n = int(e["id"][4:])
        assert 450 <= n <= 549, e["id"] + " outside range"

    rows = ["| ID | Title | Status | Affects |", "|---|---|---|---|"]
    for e in entries:
        anchor = slug(e["id"] + " " + e["title"])
        rows.append("| [{0}](#{1}) | {2} | {3} | {4} |".format(
            e["id"], anchor, e["title"], e["status"], ", ".join(e["prefixes"])))
    index = "\n".join(rows)
    start = text.index("## Index\n")
    end = text.index("\n## 1 ")
    text = text[:start] + "## Index\n\n" + index + "\n" + text[end:]
    with open(md_path, "w", encoding="utf-8") as fh:
        fh.write(text)

    with open(csv_path, "w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["dec_id", "title", "status", "affects_prefix", "affects_files", "orig_refs"])
        for e in entries:
            w.writerow([e["id"], e["title"], e["status"], ";".join(e["prefixes"]),
                        ";".join(e["files"]), ";".join(ORIG_REFS[e["id"]])])

    counts = {}
    for e in entries:
        counts[e["status"]] = counts.get(e["status"], 0) + 1
    print("entries", len(entries), counts)


if __name__ == "__main__":
    main()
