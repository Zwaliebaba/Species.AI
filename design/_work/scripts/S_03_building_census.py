"""S03: building-type census over shipped level files.
Usage: python3 -I S_03_building_census.py <levels_dir> [--per-map TYPE]
Prints a TSV: type, official_maps, official_lines, dev_mp_maps, dev_mp_lines, campaign_maps, old_maps, modes(official)
'official' = file listed in data/levels/level_enum.txt AND present; 'dev' = other mp_*.txt in levels_dir;
'campaign' = map_*/mission_*; 'old' = files under levels_dir/old.
"""
import sys, os, re, collections

def parse(path):
    types = collections.Counter(); gametypes = []
    inb = False; inopt = False
    with open(path, encoding='latin-1') as f:
        for ln in f:
            s = ln.strip()
            if not s: continue
            tok = s.split()
            t0 = tok[0]
            if t0 == 'MultiwiniaOptions_StartDefinition': inopt = True; continue
            if t0 == 'MultiwiniaOptions_EndDefinition': inopt = False; continue
            if inopt and t0 == 'GameTypes': gametypes = tok[1:]
            if t0 == 'Buildings_StartDefinition': inb = True; continue
            if t0 == 'Buildings_EndDefinition': inb = False; continue
            if inb and not t0.startswith('#'):
                types[t0.lower()] += 1
    return types, gametypes

def main():
    d = sys.argv[1]
    per = None
    if len(sys.argv) > 3 and sys.argv[2] == '--per-map': per = sys.argv[3].lower()
    enum = []
    with open(os.path.join(d, 'level_enum.txt'), encoding='latin-1') as f:
        for ln in f:
            tok = ln.split()
            if tok: enum.append(tok[0].lower())
    files = {fn.lower(): fn for fn in os.listdir(d) if fn.endswith('.txt')}
    official = [files[e] for e in enum if e in files]
    stats = collections.defaultdict(lambda: collections.defaultdict(int))
    modes = collections.defaultdict(set)
    permap = []
    for fn in sorted(files.values()):
        if fn == 'level_enum.txt': continue
        types, gt = parse(os.path.join(d, fn))
        low = fn.lower()
        if fn in official: cat = 'official'
        elif low.startswith('mp_'): cat = 'dev'
        else: cat = 'campaign'
        for t, c in types.items():
            stats[t][cat + '_maps'] += 1; stats[t][cat + '_lines'] += c
            if cat == 'official':
                for g in gt: modes[t].add(g)
            if per == t: permap.append((cat, fn, ' '.join(gt), c))
    od = os.path.join(d, 'old')
    if os.path.isdir(od):
        for fn in sorted(os.listdir(od)):
            if not fn.endswith('.txt'): continue
            types, gt = parse(os.path.join(od, fn))
            for t, c in types.items():
                stats[t]['old_maps'] += 1; stats[t]['old_lines'] += c
                if per == t: permap.append(('old', fn, ' '.join(gt), c))
    print('type\tofficial_maps\tofficial_lines\tdev_mp_maps\tdev_mp_lines\tcampaign_maps\tcampaign_lines\told_maps\tmodes')
    for t in sorted(stats):
        s = stats[t]
        print(f"{t}\t{s['official_maps']}\t{s['official_lines']}\t{s['dev_maps']}\t{s['dev_lines']}\t{s['campaign_maps']}\t{s['campaign_lines']}\t{s['old_maps']}\t{','.join(sorted(modes[t]))}")
    print(f"# official files: {len(official)}; dev mp files: {sum(1 for f in files if f.startswith('mp_') and files[f] not in official)}")
    for row in permap: print('#', *row, sep='\t')

if __name__ == '__main__':
    main()
