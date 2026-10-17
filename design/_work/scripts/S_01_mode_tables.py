#!/usr/bin/env python3
"""S_01 - generate the per-mode map tables and the mode-string table for 01_game_modes.md.

Inputs (passed as arguments, read-only):
  argv[1] = _work/drafts/X4_mode_maplists.csv   (menu order per game type, retail + all_in_tree)
  argv[2] = _work/drafts/X4_levels.csv          (one row per level file, 125 columns)
  argv[3] = _work/drafts/P02_game_modes_strings.csv (UI strings used by the mode code)
  argv[4] = output directory (markdown fragments are written there)

Run: python3 -I S_01_mode_tables.py <maplists> <levels> <strings> <outdir>
"""
import csv
import os
import sys

MODES = ["Domination", "KingOfTheHill", "CaptureTheStatue", "Assault",
         "RocketRiot", "Blitzkrieg", "TankBattle"]


def per_team_cap(row):
    """Population cap per team, as TrunkPort/SpawnPoint::PopulationLocked compute it."""
    try:
        cap = int(row["populationcap"]) if row["populationcap"] != "" else -1
    except ValueError:
        cap = -1
    try:
        np_ = int(row["numplayers"])
    except ValueError:
        np_ = 0
    dcap = row["defenderpopulationcap"]
    dcap = int(dcap) if dcap not in ("", None) else -1
    att = [a for a in row["attackingteams"].split() if a != ""]
    if row["menu_types"] == "Assault" and dcap != -1 and np_ > 0:
        n_att = len(att)
        n_def = np_ - n_att
        att_cap = (cap - n_def * dcap) // n_att if n_att else 0
        return "def %d / att %d" % (dcap, att_cap)
    if cap == -1 or np_ == 0:
        return "none"
    return "%d" % (cap // np_)


def coop(row):
    if row["forcecoop"] == "Y":
        g = "G1 %s / G2 %s" % (row["coopgroup1"] or "-", row["coopgroup2"] or "-")
        return "ForceCoop (%s)" % g
    if row["coopmode"] == "Y":
        if row["coopgroup1"] or row["coopgroup2"]:
            return "CoopMode (G1 %s / G2 %s)" % (row["coopgroup1"] or "-", row["coopgroup2"] or "-")
        return "CoopMode"
    return "-"


def mode_objects(mode, row):
    z = row["mwzones_by_team"] or "-"
    sizes = row["mwzone_sizes"] or "-"
    if mode == "KingOfTheHill":
        return "zones %s (%s; r %s)" % (row["mwzones"], z, sizes)
    if mode == "CaptureTheStatue":
        return "zones %s (%s; r %s)" % (row["mwzones"], z, sizes)
    if mode == "Blitzkrieg":
        return "flags %s (%s; r %s)" % (row["mwzones"], z, sizes)
    if mode == "Assault":
        return "bomb %s mode s, stations %s, attackers pos %s, AIObj %s/markers %s" % (
            row["pulsebomb_starttime"] or "-", row["controlstations_by_team"] or "-",
            row["attackingteams"] or "-", row["aiobjectives"], row["aiobjectivemarkers"])
    if mode == "RocketRiot":
        return "rockets %s, fuel stations %s, markers %s" % (
            row["escaperockets_by_team"] or "-", row["fuelstations_by_team"] or "-",
            row["aiobjectivemarkers"])
    if mode == "Domination":
        return "masters %s, links %s, radar %s" % (row["spawnpointmasters"], row["spawnlinks"], row["radardishes"])
    if mode == "TankBattle":
        return "instant units %s" % (row["instantunits"] or "-")
    return "-"


def esc(s):
    return (s or "").replace("|", "\\|")


def main():
    maplists, levels, strings, outdir = sys.argv[1:5]
    lv = {r["file"]: r for r in csv.DictReader(open(levels, newline="", encoding="utf-8"))}
    ml = list(csv.DictReader(open(maplists, newline="", encoding="utf-8")))
    out = []
    counts = {}
    for mode in MODES:
        scope = "retail" if mode != "TankBattle" else "all_in_tree"
        rows = [r for r in ml if r["gametype"] == mode and r["scope"] == scope]
        rows.sort(key=lambda r: int(r["menu_index"]))
        counts[mode] = (len(rows), len([r for r in ml if r["gametype"] == mode and r["scope"] == "all_in_tree"]))
        out.append("#### Map table — %s (%s data set, %d maps, menu order)\n" % (mode, scope, len(rows)))
        out.append("| # | File | Name (IP) | Pl | Diff | PlayTime | PopCap → per team | Co-op | LevelOptions | InvalidCrates | SpawnPts (team:n) | TrunkPorts (pos:n) | Mode objects | Start units | Demo default |")
        out.append("|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|")
        for r in rows:
            x = lv.get(r["file"], {})
            out.append("| %s | `%s` | %s | %s | %s | %s | %s → %s | %s | %s | %s | %s (%s) | %s | %s | %s | %s |" % (
                r["menu_index"], os.path.basename(r["file"]), esc(r["english_name"]), r["numplayers"],
                r["difficulty"] or "-", r["playtime"] or "-",
                x.get("populationcap", "") or "-", per_team_cap(x) if x else "-",
                coop(x) if x else "-", esc(x.get("leveloptions", "")) or "-",
                esc(x.get("invalidcrates", "")) or "-", x.get("spawnpoints", ""),
                x.get("spawnpoints_by_team", "") or "-", x.get("trunkports_by_team", "") or "-",
                esc(mode_objects(mode, x)) if x else "-", x.get("instantunit_total", "") or "0",
                "Y" if x.get("demo_default_allowed") == "Y" else "-"))
        out.append("")
    with open(os.path.join(outdir, "S_01_maptables.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    with open(os.path.join(outdir, "S_01_counts.txt"), "w", encoding="utf-8") as f:
        for k, v in counts.items():
            f.write("%s retail_or_listed=%d all_in_tree=%d\n" % (k, v[0], v[1]))

    # Source-map rows: one per data/levels/mp_*.txt
    lrows = ["| File | Role |", "|---|---|"]
    for f in sorted(k for k in lv if os.path.basename(k).startswith("mp_") and os.path.dirname(k) == "data/levels"):
        x = lv[f]
        gt = x["gametypes_tokens"] or "none"
        ships = "ships in retail data" if x["ships_retail"] == "Y" else "not in retail data"
        if gt in ("none", ""):
            role = "level with `GameTypes none` — never listed by the menu (%s)" % ships
        elif gt == "TankBattle":
            role = "TankBattle arena [B:DEBUG] (mode not permitted in FULL; %s)" % ships
        else:
            role = "%s map, %s players (%s)" % (gt, x["numplayers"], ships)
        lrows.append("| `%s` | %s |" % (f, role))
    with open(os.path.join(outdir, "S_01_levelrows.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(lrows) + "\n")

    srows = list(csv.DictReader(open(strings, newline="", encoding="utf-8")))
    sout = ["| # | String id | First use (file:line at f909630) | English text (IP — rewrite) |", "|---|---|---|---|"]
    for i, r in enumerate(srows, 1):
        sout.append("| %d | `%s` | `%s`:L%s | %s |" % (i, r["key"], r["file"], r["first_line"], esc(r["english"])))
    with open(os.path.join(outdir, "S_01_strings.md"), "w", encoding="utf-8") as f:
        f.write("\n".join(sout) + "\n")


if __name__ == "__main__":
    main()
