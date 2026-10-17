"""S04: generate the per-level crate tables embedded in 04_crates_powerups_and_weapons.md.

Usage: python3 -I S_04_crate_tables.py <levels_dir> <P08_crate_pools.csv> <out_md>

Re-derives, per shipped mp_*.txt level, the start-of-game crate pools exactly as
code/worldobject/crate.cpp (Crate::IsValid / CaptureCrate / RunUncapturedCrate) would
compute them in the baseline FULL build (INCLUDE_THEBEAST off, INCLUDE_CRATES_ADVANCED on),
cross-checks the result against drafts/P08_crate_pools.csv, and adds the level facts that
feed the crate system: world size (-> max simultaneous crates = int(worldSizeX/500)),
RestrictionZone count (all zones block random drops; zones with blockPowerups=1 also block
power-up targeting), InvalidCrates, trunkportarmour and spawn-point count.
Dynamic limits (s_crates>=4, ant-nest count, SpaceShip already used) are not applied.
File contents are treated as data only.
"""
import sys, os, csv

NAMES = ["Squad", "Harvester", "Engineer", "Armour", "Airstrike", "Nuke", "Darwinians",
         "PersonalShield", "Subversion", "HotFeet", "GunTurret", "Infection", "MagicalForest",
         "Randomiser", "AntNest", "Plague", "Eggs", "Meteor", "DarkForest", "CrateMania",
         "SpawnMania", "Bitzkreig", "Rocketwinians", "Rage", "RocketTurret", "FlameTurret",
         "SpaceShip", "Triffids", "NapalmStrike", "SlowDown", "SpeedUp"]
BASIC = {"Squad", "Engineer", "Armour", "Airstrike", "Darwinians", "GunTurret", "MagicalForest"}
GOOD_EXCL = {"Infection", "Randomiser", "Plague", "SpaceShip", "Triffids"}
BAD_POOL = ["Infection", "Randomiser", "Plague", "DarkForest", "Triffids", "SpaceShip", "AntNest"]
STANDARD_GOOD = [n for n in NAMES if n not in GOOD_EXCL]          # 26 types
STANDARD_BASIC = [n for n in STANDARD_GOOD if n in BASIC]          # 7 types


def parse(path):
    info = dict(gt="none", tpa=0, inv=set(), sp=0, rz=0, rzp=0, wx=0.0, wz=0.0, players=0)
    in_buildings = False
    with open(path, encoding="latin-1") as f:
        for line in f:
            t = line.split()
            if not t:
                continue
            k = t[0]
            if k == "GameTypes" and len(t) > 1:
                info["gt"] = t[1]
            elif k == "NumPlayers" and len(t) > 1:
                info["players"] = int(t[1])
            elif k == "worldSizeX" and len(t) > 1:
                info["wx"] = float(t[1])
            elif k == "worldSizeZ" and len(t) > 1:
                info["wz"] = float(t[1])
            elif k == "LevelOptions":
                for i, w in enumerate(t):
                    if w.lower() == "trunkportarmour" and i + 1 < len(t):
                        info["tpa"] = int(t[i + 1])
            elif k == "InvalidCrates":
                for w in t[1:]:
                    for n in NAMES:
                        if n.lower() == w.lower():
                            info["inv"].add(n)
            elif k == "Buildings_StartDefinition":
                in_buildings = True
            elif k == "Buildings_EndDefinition":
                in_buildings = False
            elif in_buildings and k == "SpawnPoint":
                info["sp"] += 1
            elif in_buildings and k == "RestrictionZone":
                info["rz"] += 1
                # RestrictionZone id x z team fx fz isGlobal size [blockPowerups]
                if len(t) >= 10 and t[9] == "1":
                    info["rzp"] += 1
    return info


def valid(n, info, basic):
    if basic and n not in BASIC:
        return False
    if n in ("Harvester", "Engineer", "SpawnMania") and info["sp"] == 0:
        return False
    if n == "Armour" and (info["gt"] == "RocketRiot" or info["tpa"] > 0):
        return False
    if info["gt"] == "Assault" and n in ("RocketTurret", "SpaceShip"):
        return False
    return n not in info["inv"]


def pct(x, n):
    return f"{x / n:.3f}" if n else "—"


def main():
    levels, p08, out = sys.argv[1], sys.argv[2], sys.argv[3]
    ref = {r["level"]: r for r in csv.DictReader(open(p08, newline=""))}
    rows, mismatches = [], []
    for fn in sorted(os.listdir(levels)):
        if not (fn.startswith("mp_") and fn.endswith(".txt")):
            continue
        info = parse(os.path.join(levels, fn))
        good = [n for n in STANDARD_GOOD if valid(n, info, False)]
        basic = [n for n in STANDARD_GOOD if valid(n, info, True)]
        bad = [n for n in BAD_POOL if not (n == "SpaceShip" and info["gt"] == "Assault")]
        r = ref.get(fn)
        if r is None or int(r["good_pool_size"]) != len(good) or int(r["bad_pool_size"]) != len(bad) \
                or int(r["basic_pool_size"]) != len(basic):
            mismatches.append(fn)
        excl = [n for n in STANDARD_GOOD if n not in good]
        bexcl = [n for n in STANDARD_BASIC if n not in basic]
        crates_live = info["gt"] in ("Domination", "KingOfTheHill", "CaptureTheStatue", "Assault",
                                     "RocketRiot", "Blitzkrieg")
        rows.append((fn, info, good, basic, bad, excl, bexcl, crates_live))

    with open(out, "w", encoding="utf-8") as f:
        f.write("| Level (`data/levels/`) | Declared GameTypes | Players | World X×Z | Max crates | SpawnPoints | "
                "RestrictionZones (blockPowerups) | trunkportarmour | InvalidCrates | Good pool n | each good % | "
                "Removed from the 26-type standard good pool | Bad pool n | each bad % | Basic-only pool n | each basic % | "
                "Removed from the 7 basic types |\n")
        f.write("|" + "---|" * 17 + "\n")
        for fn, info, good, basic, bad, excl, bexcl, live in rows:
            maxc = int(info["wx"] / 500.0)
            note = "" if live else " (no crates: game type not in FULL lobby)"
            f.write(f"| `data/levels/{fn}`{note} | {info['gt']} | {info['players']} | "
                    f"{info['wx']:.0f}×{info['wz']:.0f} | {maxc} | {info['sp']} | {info['rz']} ({info['rzp']}) | "
                    f"{info['tpa']} | {' '.join(sorted(info['inv'])) or '—'} | {len(good)} | {pct(90.0, len(good))} | "
                    f"{', '.join(excl) or '— (none)'} | {len(bad)} | {pct(10.0, len(bad))} | {len(basic)} | "
                    f"{pct(100.0, len(basic))} | {', '.join(bexcl) or '— (none)'} |\n")
    print(f"{len(rows)} levels written; mismatches vs P08 CSV: {mismatches or 'none'}")


main()
