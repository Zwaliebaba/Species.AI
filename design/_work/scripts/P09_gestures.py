"""P09 helper: parse data/gestures.txt (Rubine-style trainer sets) and print
per-symbol feature means/min/max. Usage: python3 -I P09_gestures.py <gestures.txt>"""
import sys

def main(path):
    with open(path, 'r', encoding='latin-1') as fh:
        lines = [l.rstrip('\n') for l in fh]
    n = int(lines[0].split()[0])
    idx = 1
    names = ["Squad","Engineer","Grenade","Rocket","AirStrike","Controller","Officer","Armour"]
    print("numSymbols", n)
    for c in range(n):
        nt = int(lines[idx].split()[0]); idx += 1
        toks = lines[idx].split(); idx += 1
        print(f"symbol {c} ({names[c] if c < len(names) else '?'}): trainers={nt} tokens={len(toks)} extra={toks[nt*14:]}")
        vals = [float(t) for t in toks[:nt*14]]
        feats = [vals[t*14:(t+1)*14] for t in range(nt)]
        for f in range(14):
            col = [fs[f] for fs in feats]
            print(f"  f{f:2d} mean={sum(col)/len(col):12.4f} min={min(col):12.4f} max={max(col):12.4f}")

if __name__ == '__main__':
    main(sys.argv[1])
