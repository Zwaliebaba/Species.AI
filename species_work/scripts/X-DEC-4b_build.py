#!/usr/bin/env python3
"""X-DEC-4b: validate the register file, write its index table and the dec map CSV.

Usage:
  python3 -I X-DEC-4b_build.py <register.md> <decisions_dir> <packages/INDEX.md> <out.csv>

Checks: DEC ids in 550-649, unique and ascending; field order; status values; "Decide by"
only on Open entries; every cited DEC id exists in the decisions directory; every Affects
path is a package target listed in INDEX.md (or a folder holding one); the prefix in
parentheses is the registered prefix of that path. Rewrites the block between the index
markers and writes the CSV with the csv module (lineterminator '\n').
"""
import csv
import pathlib
import re
import sys

ORIG_REFS = {
    "DEC-550": ["06_player_control_and_input.md:524-524"],
    "DEC-551": ["06_player_control_and_input.md:525-525", "06_player_control_and_input.md:528-528",
                "06_player_control_and_input.md:531-531"],
    "DEC-552": ["06_player_control_and_input.md:526-526"],
    "DEC-553": ["06_player_control_and_input.md:2131-2174", "06_player_control_and_input.md:2305-2305",
                "06_player_control_and_input.md:2331-2331"],
    "DEC-554": ["06_player_control_and_input.md:2306-2306", "06_player_control_and_input.md:2309-2312",
                "06_player_control_and_input.md:2319-2319", "06_player_control_and_input.md:2331-2331"],
    "DEC-555": ["06_player_control_and_input.md:2315-2315"],
    "DEC-556": ["06_player_control_and_input.md:2333-2333", "06_player_control_and_input.md:531-531"],
    "DEC-557": ["07_ai.md:1372-1372", "07_ai.md:1395-1395"],
    "DEC-558": ["07_ai.md:1398-1398"],
    "DEC-559": ["07_ai.md:1400-1402"],
    "DEC-560": ["08_levels_and_level_design.md:2007-2007"],
    "DEC-561": ["08_levels_and_level_design.md:2008-2008"],
    "DEC-562": ["08_levels_and_level_design.md:2010-2010"],
    "DEC-563": ["08_levels_and_level_design.md:2014-2015"],
    "DEC-564": ["10_audio_design.md:959-959", "10_audio_design.md:961-961", "10_audio_design.md:971-971",
                "10_audio_design.md:1597-1597", "06_player_control_and_input.md:2320-2320"],
    "DEC-565": ["10_audio_design.md:963-974"],
    "DEC-566": ["10_audio_design.md:1289-1290", "10_audio_design.md:1294-1294"],
    "DEC-567": ["10_audio_design.md:1342-1353", "10_audio_design.md:1562-1562", "10_audio_design.md:1593-1593"],
    "DEC-568": ["10_audio_design.md:1586-1586", "10_audio_design.md:1589-1589"],
    "DEC-569": ["10_audio_design.md:1293-1293", "10_audio_design.md:1596-1596"],
    "DEC-570": ["10_audio_design.md:1333-1339", "10_audio_design.md:1591-1592", "10_audio_design.md:1627-1639"],
}

PREFIX_BY_PATH = [
    ("reference/00_", "R-VIS"), ("reference/01_", "R-MODE"), ("reference/02_", "R-UNIT"),
    ("reference/03_", "R-BLDG"), ("reference/04_", "R-CRATE"), ("reference/05_", "R-ECON"),
    ("reference/06_", "R-CTRL"), ("reference/07_", "R-BOT"), ("reference/08_", "R-MAP"),
    ("reference/09_", "R-META"), ("reference/10_", "R-AUD"), ("reference/11_", "R-TUT"),
    ("reference/12_", "R-SIM"), ("architecture/components/terrain", "TER"),
    ("architecture/components/renderer", "RND"), ("architecture/components/asset_pipeline", "AST"),
    ("architecture/components/animation_and_vfx", "VFX"), ("architecture/components/audio_engine", "AEN"),
    ("architecture/components/ui_framework", "UI"), ("architecture/components/input", "INP"),
    ("architecture/components/camera_and_sequences", "CAM"), ("architecture/components/data_schemas", "DAT"),
    ("architecture/components/platform_layer", "PLT"), ("architecture/components/build_tooling", "BLD"),
    ("architecture/components/diagnostics", "DBG"),
]

FIELDS = ["Status", "Affects", "Specified behaviour", "Options", "Recommendation", "Decide by"]


def slug(text):
    s = text.strip().lower()
    s = re.sub(r"[^\w\- ]", "", s)
    return s.replace(" ", "-")


def main(argv):
    if len(argv) != 5:
        print(__doc__)
        return 2
    reg, dec_dir, index_md, out_csv = (pathlib.Path(a) for a in argv[1:])
    text = reg.read_text(encoding="utf-8")
    errors = []

    # Known DEC ids across the register files.
    known = set()
    for p in sorted(dec_dir.rglob("*.md")):
        known.update(re.findall(r"^### (DEC-\d{3})\b", p.read_text(encoding="utf-8"), flags=re.M))

    # Package targets.
    targets = set(re.findall(r"((?:reference|architecture)/[\w./-]+)", index_md.read_text(encoding="utf-8")))

    # Parse entries.
    parts = re.split(r"^(### DEC-\d{3} .+)$", text, flags=re.M)
    entries = []
    for i in range(1, len(parts), 2):
        head = parts[i]
        body = parts[i + 1]
        m = re.match(r"### (DEC-(\d{3})) (.+)$", head)
        dec_id, num, title = m.group(1), int(m.group(2)), m.group(3).strip()
        body = body.split("\n## ")[0]
        fields = re.findall(r"^- (Status|Affects|Specified behaviour|Options|Recommendation|Decide by): ?(.*)$",
                            body, flags=re.M)
        names = [f[0] for f in fields]
        vals = {f[0]: f[1].strip() for f in fields}
        entries.append((dec_id, num, title, names, vals, body))

    prev = 549
    seen = set()
    for dec_id, num, title, names, vals, body in entries:
        if not 550 <= num <= 649:
            errors.append(f"{dec_id}: outside 550-649")
        if dec_id in seen:
            errors.append(f"{dec_id}: duplicate")
        seen.add(dec_id)
        if num <= prev:
            errors.append(f"{dec_id}: not ascending")
        prev = num
        status = vals.get("Status", "")
        expected = FIELDS[:5] + (["Decide by"] if status == "Open" else [])
        if names != expected:
            errors.append(f"{dec_id}: field order {names}, expected {expected}")
        if status not in ("Resolved", "Open", "Dropped"):
            errors.append(f"{dec_id}: bad status {status!r}")
        for path, pfx in re.findall(r"((?:reference|architecture)/[\w./-]+) \(([A-Z-]+)\)", vals.get("Affects", "")):
            if path not in targets and not any(t.startswith(path) for t in targets):
                errors.append(f"{dec_id}: Affects path not a package target: {path}")
            want = next((p for k, p in PREFIX_BY_PATH if path.startswith(k)), None)
            if want != pfx:
                errors.append(f"{dec_id}: prefix {pfx} for {path}, expected {want}")
        if dec_id not in ORIG_REFS:
            errors.append(f"{dec_id}: no orig_refs")
    for ref in sorted(set(re.findall(r"\bDEC-\d{3}\b", text))):
        if ref not in known:
            errors.append(f"cited {ref} does not exist")
    for k in ORIG_REFS:
        if k not in seen:
            errors.append(f"orig_refs for missing entry {k}")

    # Index table.
    rows = ["| ID | Title | Status | Affects |", "|---|---|---|---|"]
    csv_rows = []
    for dec_id, num, title, names, vals, body in entries:
        aff = re.findall(r"((?:reference|architecture)/[\w./-]+) \(([A-Z-]+)\)", vals.get("Affects", ""))
        prefixes = list(dict.fromkeys(p for _, p in aff))
        files = list(dict.fromkeys(f for f, _ in aff))
        anchor = slug(f"{dec_id} {title}")
        rows.append(f"| [{dec_id}](#{anchor}) | {title} | {vals.get('Status', '')} | {', '.join(prefixes)} |")
        csv_rows.append([dec_id, title, vals.get("Status", ""), ";".join(prefixes), ";".join(files),
                         ";".join(ORIG_REFS.get(dec_id, []))])
    block = "<!-- index:start -->\n" + "\n".join(rows) + "\n<!-- index:end -->"
    new_text, n = re.subn(r"<!-- index:start -->.*?<!-- index:end -->", block, text, flags=re.S)
    if n != 1:
        errors.append("index markers not found exactly once")

    if errors:
        for e in errors:
            print("ERROR", e)
        return 1
    reg.write_text(new_text, encoding="utf-8")
    with out_csv.open("w", encoding="utf-8", newline="") as fh:
        w = csv.writer(fh, lineterminator="\n")
        w.writerow(["dec_id", "title", "status", "affects_prefix", "affects_files", "orig_refs"])
        w.writerows(csv_rows)
    counts = {}
    for row in csv_rows:
        counts[row[2]] = counts.get(row[2], 0) + 1
    print(f"OK: {len(entries)} entries {counts}; size {len(new_text.encode('utf-8'))} bytes")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
