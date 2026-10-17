"""P08: compute per-level crate outcome pools (baseline FULLGAME build, INCLUDE_THEBEAST off).
Usage: python3 -I P08_crate_pools.py <levels_dir> <out_csv>
Mirrors code/worldobject/crate.cpp: Crate::IsValid / CaptureCrate / RunUncapturedCrate.
Dynamic conditions (s_crates>=4, ant-nest count>=4, spaceship used) are NOT applied (start-of-game state).
"""
import sys, os, re, csv

NAMES = ["Squad","Harvester","Engineer","Armour","Airstrike","Nuke","Darwinians","PersonalShield",
         "Subversion","HotFeet","GunTurret","Infection","MagicalForest","Randomiser","AntNest","Plague",
         "Eggs","Meteor","DarkForest","CrateMania","SpawnMania","Bitzkreig","Rocketwinians","Rage",
         "RocketTurret","FlameTurret","SpaceShip","Triffids","NapalmStrike","SlowDown","SpeedUp"]
BASIC = {"Squad","Engineer","Armour","Airstrike","Darwinians","GunTurret","MagicalForest"}
GOOD_EXCL = {"Infection","Randomiser","Plague","SpaceShip","Triffids"}
BAD_POOL = ["Infection","Randomiser","Plague","DarkForest","Triffids","SpaceShip","AntNest"]

def parse(path):
    gt=None; tpa=0; inv=set(); sp=0; inb=False
    with open(path, encoding='latin-1') as f:
        for line in f:
            t=line.split()
            if not t: continue
            if t[0]=="GameTypes": gt=t[1] if len(t)>1 else None
            elif t[0]=="LevelOptions":
                for i,w in enumerate(t):
                    if w.lower()=="trunkportarmour" and i+1<len(t): tpa=int(t[i+1])
            elif t[0]=="InvalidCrates":
                for w in t[1:]:
                    for n in NAMES:
                        if n.lower()==w.lower(): inv.add(n)
            elif t[0]=="Buildings_StartDefinition": inb=True
            elif t[0]=="Buildings_EndDefinition": inb=False
            elif inb and t[0]=="SpawnPoint": sp+=1
    return gt,tpa,inv,sp

def valid(n, gt, tpa, inv, sp, basic):
    if basic and n not in BASIC: return False
    if n in ("Harvester","Engineer","SpawnMania") and sp==0: return False
    if n=="Armour" and (gt=="RocketRiot" or tpa>0): return False
    if gt=="Assault" and n in ("RocketTurret","SpaceShip"): return False
    return n not in inv

def main():
    d, out = sys.argv[1], sys.argv[2]
    rows=[]
    for fn in sorted(os.listdir(d)):
        if not fn.startswith("mp_") or not fn.endswith(".txt"): continue
        gt,tpa,inv,sp = parse(os.path.join(d,fn))
        good=[n for n in NAMES if n not in GOOD_EXCL and valid(n,gt,tpa,inv,sp,False)]
        bad=[n for n in BAD_POOL if not (n=="SpaceShip" and gt=="Assault")]
        basic=[n for n in NAMES if n not in GOOD_EXCL and valid(n,gt,tpa,inv,sp,True)]
        rows.append(dict(level=fn, gametype=gt, spawnpoints=sp, trunkportarmour=tpa,
            invalidcrates=";".join(sorted(inv)), good_pool_size=len(good),
            good_each_pct=round(90.0/len(good),3) if good else "", bad_pool_size=len(bad),
            bad_each_pct=round(10.0/len(bad),3), basic_pool_size=len(basic),
            basic_each_pct=round(100.0/len(basic),3) if basic else "",
            good_pool=";".join(good), basic_pool=";".join(basic)))
    with open(out,"w",newline="") as f:
        w=csv.DictWriter(f, fieldnames=list(rows[0].keys())); w.writeheader(); w.writerows(rows)
    for r in rows: print(r["level"], r["gametype"], r["spawnpoints"], r["good_pool_size"], r["bad_pool_size"], r["basic_pool_size"], r["invalidcrates"])
main()
