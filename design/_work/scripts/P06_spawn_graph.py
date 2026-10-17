"""P06: analyse spawn-link graphs per level.
For each level file: parse SpawnPoint / SpawnLink / SpawnPointMaster lines inside
Buildings blocks (fields: type id x z team fx fz isGlobal [links...]), then from the
master compute: reachable spawn points, hop count (min/max), number of distinct
paths (directed), unreachable spawn points, self links, cycles.
Usage: python3 -I P06_spawn_graph.py <levels_dir> <out_csv>
"""
import sys, os, csv

SPAWN = {'spawnpoint', 'spawnlink', 'spawnpointmaster'}

def parse(path):
    nodes = {}
    inb = False
    with open(path, 'r', encoding='latin-1') as f:
        for ln in f:
            s = ln.strip()
            if s.startswith('Buildings_StartDefinition'): inb = True; continue
            if s.startswith('Buildings_EndDefinition'): inb = False; continue
            if not inb or not s or s.startswith('#'): continue
            t = s.split()
            if t[0].lower() in SPAWN:
                bid = int(t[1]); team = int(t[4])
                links = [int(x) for x in t[8:]]
                nodes[bid] = (t[0].lower(), team, links)
    return nodes

def analyse(nodes):
    masters = [k for k, v in nodes.items() if v[0] == 'spawnpointmaster']
    sps = [k for k, v in nodes.items() if v[0] == 'spawnpoint']
    res = {'masters': len(masters), 'spawnpoints': len(sps), 'links': sum(1 for v in nodes.values() if v[0]=='spawnlink')}
    if not masters:
        res.update(reachable=0, unreachable=len(sps), minhops='', maxhops='', multipath=0, cycle=False, selflink=False)
        return res
    m = masters[0]
    cycle = False; selflink = any(k in v[2] for k, v in nodes.items())
    # count paths (DFS with on-stack detection)
    paths = {}; hops = {}
    def dfs(n, depth, stack):
        nonlocal cycle
        if n in stack: cycle = True; return
        if n not in nodes: return
        if nodes[n][0] == 'spawnpoint':
            paths[n] = paths.get(n, 0) + 1
            hops.setdefault(n, []).append(depth)
        stack.add(n)
        for l in nodes[n][2]:
            if l in nodes: dfs(l, depth + 1, stack)
        stack.discard(n)
    dfs(m, 0, set())
    reach = [s for s in sps if s in paths]
    allh = [h for s in reach for h in hops[s]]
    res.update(reachable=len(reach), unreachable=len(sps) - len(reach),
               minhops=min(allh) if allh else '', maxhops=max(allh) if allh else '',
               multipath=sum(1 for s in reach if paths[s] > 1), cycle=cycle, selflink=selflink,
               unreach_ids=' '.join(str(s) for s in sps if s not in paths),
               multipath_ids=' '.join(f"{s}x{paths[s]}" for s in reach if paths[s] > 1))
    return res

def main():
    d, out = sys.argv[1], sys.argv[2]
    rows = []
    for fn in sorted(os.listdir(d)):
        if not fn.endswith('.txt'): continue
        nodes = parse(os.path.join(d, fn))
        if not nodes: continue
        r = analyse(nodes); r['level'] = fn; rows.append(r)
    keys = ['level','masters','spawnpoints','links','reachable','unreachable','minhops','maxhops','multipath','cycle','selflink','unreach_ids','multipath_ids']
    with open(out, 'w', newline='') as f:
        w = csv.DictWriter(f, fieldnames=keys); w.writeheader()
        for r in rows: w.writerow({k: r.get(k, '') for k in keys})
    for r in rows: print('|'.join(str(r.get(k, '')) for k in keys))

if __name__ == '__main__':
    main()
