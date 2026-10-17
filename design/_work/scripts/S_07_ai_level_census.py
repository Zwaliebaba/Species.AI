"""S_07: census of AI-related level objects in shipped Multiwinia levels.

Counts AITarget / AIObjective / AIObjectiveMarker lines per data/levels/mp_*.txt
file, plus AITarget optional-field usage, AIObjectiveMarker field values and the
'noradarcontrol' level option. Data files are parsed as plain text only.
Usage: python3 -I S_07_ai_level_census.py <SRC>/data/levels
"""
import os
import sys
from collections import Counter, defaultdict


def main(levels_dir):
    files = sorted(f for f in os.listdir(levels_dir)
                   if f.lower().startswith("mp_") and f.lower().endswith(".txt"))
    tot = Counter()
    per_mode = defaultdict(Counter)
    ait_fields = Counter()
    ait_link = 0
    ait_mod = Counter()
    ait_nocliff = 0
    marker_rows = []
    obj_rows = []
    noradar = []
    for f in files:
        mode = f.split("_")[1].lower()
        with open(os.path.join(levels_dir, f), encoding="latin-1") as fh:
            for line in fh:
                tok = line.split()
                if not tok:
                    continue
                key = tok[0]
                if key.lower() == "noradarcontrol" or "noradarcontrol" in line.lower():
                    noradar.append((f, line.strip()))
                if key == "AITarget":
                    tot["AITarget"] += 1
                    per_mode[mode]["AITarget"] += 1
                    extra = tok[8:]
                    ait_fields[len(extra)] += 1
                    if len(extra) >= 1 and extra[0] != "-1":
                        ait_link += 1
                    if len(extra) >= 2:
                        ait_mod[extra[1]] += 1
                    if len(extra) >= 3 and extra[2] == "0":
                        ait_nocliff += 1
                elif key == "AIObjective":
                    tot["AIObjective"] += 1
                    per_mode[mode]["AIObjective"] += 1
                    obj_rows.append((f, tok))
                elif key == "AIObjectiveMarker":
                    tot["AIObjectiveMarker"] += 1
                    per_mode[mode]["AIObjectiveMarker"] += 1
                    marker_rows.append((f, tok))
    print("levels scanned:", len(files))
    print("totals:", dict(tot))
    for m, c in sorted(per_mode.items()):
        nfiles = sum(1 for f in files if f.split("_")[1].lower() == m)
        print(f"  mode {m:12s} files={nfiles:2d} {dict(c)}")
    print("AITarget optional-field count histogram:", dict(ait_fields))
    print("AITargets with manual link:", ait_link)
    print("AITarget priorityModifier values:", dict(ait_mod))
    print("AITargets with checkCliffs=0:", ait_nocliff)
    mk_team = Counter(); mk_scan = Counter(); mk_obj = Counter(); mk_arm = Counter(); mk_pick = Counter()
    for f, tok in marker_rows:
        mode = f.split("_")[1].lower()
        mk_team[(mode, tok[4])] += 1
        mk_scan[(mode, tok[8] if len(tok) > 8 else "?")] += 1
        mk_obj[(mode, "objective" if len(tok) > 9 and tok[9] != "-1" else "none")] += 1
        mk_arm[(mode, tok[10] if len(tok) > 10 else "absent")] += 1
        mk_pick[(mode, tok[11] if len(tok) > 11 else "absent")] += 1
    print("Marker team by mode:", dict(mk_team))
    print("Marker scanRange by mode:", dict(mk_scan))
    print("Marker objective link by mode:", dict(mk_obj))
    print("Marker armourObjective by mode:", dict(mk_arm))
    print("Marker pickupOnly by mode:", dict(mk_pick))
    nxt = Counter((f.split("_")[1].lower(), "chain-end" if tok[8] == "-1" else "linked") for f, tok in obj_rows)
    print("AIObjective next links:", dict(nxt))
    print("noradarcontrol mentions:", noradar[:20])


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
