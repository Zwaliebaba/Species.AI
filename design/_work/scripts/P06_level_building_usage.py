"""P06: count building types per level file inside Buildings_StartDefinition blocks.
Usage: python3 -I P06_level_building_usage.py <levels_dir> <out_csv> [type ...]
"""
import sys, os, csv, re
from collections import defaultdict

def main():
    levels_dir, out_csv = sys.argv[1], sys.argv[2]
    wanted = [t.lower() for t in sys.argv[3:]]
    rows = []
    usage = defaultdict(lambda: defaultdict(int))
    lines_by = defaultdict(list)
    for fn in sorted(os.listdir(levels_dir)):
        p = os.path.join(levels_dir, fn)
        if not os.path.isfile(p) or not fn.endswith('.txt'):
            continue
        inb = False
        block = 0
        with open(p, 'r', encoding='latin-1') as f:
            for ln in f:
                s = ln.strip()
                if s.startswith('Buildings_StartDefinition'):
                    inb = True; block += 1; continue
                if s.startswith('Buildings_EndDefinition'):
                    inb = False; continue
                if not inb or not s or s.startswith('#'):
                    continue
                tok = s.split()
                t = tok[0]
                usage[t.lower()][fn] += 1
                if not wanted or t.lower() in wanted:
                    lines_by[t.lower()].append((fn, block, s))
    with open(out_csv, 'w', newline='') as f:
        w = csv.writer(f)
        w.writerow(['type', 'level', 'count'])
        for t in sorted(usage):
            for fn in sorted(usage[t]):
                w.writerow([t, fn, usage[t][fn]])
    for t in (wanted or []):
        print('==', t, 'files:', len(usage[t]), 'total:', sum(usage[t].values()))
        print('   ', ' '.join(f"{fn}:{c}" for fn, c in sorted(usage[t].items())))
    if '--lines' in os.environ.get('P06_FLAGS', ''):
        for t in wanted:
            for fn, b, s in lines_by[t][:400]:
                print(t, fn, b, s)

if __name__ == '__main__':
    main()
