"""R4a: list identifiers declared inside `namespace Engine` sketches that contain a game-concept word.

Usage: python3 -I R4a_layer.py <out_dir>
Splits every identifier of an Engine section (types, enumerators, fields, functions, parameters) into words
and reports those that contain a game word (CONVENTIONS 8 test: factions, units, creatures, world objects,
modes, match rules, crates, powers, achievements, teams, match HUD, Power slots, game types).
"""
import csv
import os
import re
import sys

out = sys.argv[1]
GAME = set("""worker workers leader leaders trooper troopers transport transports collector collectors builder builders bomber bombers
essence orb orbs strike waypoint beacon abductor ant ants crawler crawlers egg eggs brood drifter centipede serpent reaper seed pod spider wisp wisps
barrier dish breach console turret turrets mortar pylon splitter hive relic relics vessel launch solar spitter transit gate siege crate crates
supply airstrike allegiance blight grove firebomb frenzy fury meteor missile barrage reinforcement reinforcements shield sickness swarm dilation
power powers achievement achievements bot bots faction factions rogue visitor team teams hud actor actors structure structures catch comeback grace
kill kills lobby flag flags fuel spawn zone zones capture""".split())
with open(os.path.join(out, "blocks.tsv"), encoding="utf-8") as f:
    rows = [r for r in csv.DictReader(f, delimiter="\t") if r["lang"] in ("cpp", "hlsl")]
hits = []
for r in rows:
    n = int(r["n"])
    text = open(os.path.join(out, "blocks", f"{n:04d}.txt"), encoding="utf-8").read()
    ns = None
    for i, line in enumerate(text.split("\n"), 1):
        m = re.match(r"^\s*namespace\s+(\w+)", line)
        if m:
            ns = m.group(1)
            continue
        if re.match(r"^\s*}\s*//\s*namespace", line):
            ns = None
            continue
        if r["lang"] == "hlsl":
            ns = "Engine"
        if ns != "Engine":
            continue
        code = line.split("//")[0]
        for ident in re.findall(r"[A-Za-z_][A-Za-z0-9_]*", code):
            if ident.startswith(("std", "ID3D", "IXAudio", "XAUDIO", "D3D12", "DXGI")):
                continue
            words = re.findall(r"[A-Z]?[a-z]+|[A-Z]+(?![a-z])|\d+", ident.lstrip("_"))
            gw = [w.lower() for w in words if w.lower() in GAME]
            if gw:
                hits.append((r["file"], int(r["start"]) + i, ident, ",".join(gw), code.strip()[:120]))
seen = set()
for h in hits:
    key = (h[0], h[2])
    if key in seen:
        continue
    seen.add(key)
    print("\t".join(str(x) for x in h))
