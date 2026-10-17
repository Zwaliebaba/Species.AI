"""S05: per-map economy table for 05_economy_and_population.md.

Parses every data/levels/mp_*.txt (top level only, not old/) and prints Markdown tables:
  1. per-map population/spawn economy (GameTypes, NumPlayers, PopulationCap (last line wins,
     as LevelFile::ParseMultiwiniaOptions), DefenderPopulationCap, per-team cap, Assault caps,
     coop, LevelOptions, spawn points (total / owned at start), master->spawnpoint hop range,
     SpawnPopulationLocks, trunk ports (total / owned), starting Darwinians per team position,
     other starting units).
  2. trunk-port reinforcement configuration per map (whether AdvanceTrunkPorts runs once or
     twice per tick, count per wave, armour cap, turrets).
Usage: python3 -I S_05_economy_tables.py <levels_dir> [csv_out]
Data only; no code is executed from the level files.
"""
import sys, os, csv

SPAWN = {'spawnpoint', 'spawnlink', 'spawnpointmaster'}

# data/multiwinia.txt defaults (verified by hand; see 05 §6)
MODE_DEFAULTS = {
    'RocketRiot': dict(timer=60, count=40, maxArmour=2, turretFreq=7, maxTurrets=3),
    'Blitzkrieg': dict(timer=60, count=40, maxArmour=2, turretFreq=2, maxTurrets=2),
}


def parse_level(path):
    res = dict(gametypes='', numplayers=-1, popcap=-1, popcap_lines=[], defcap=-1,
               options=[], effects=[], coop=False, attacking=[], official=False,
               buildings=[], instant=[])
    section = None
    with open(path, 'r', encoding='latin-1') as f:
        for ln in f:
            s = ln.strip()
            if not s or s.startswith('#'):
                continue
            t = s.split()
            w = t[0]
            if w.endswith('_StartDefinition'):
                section = w[:-len('_StartDefinition')]
                continue
            if w.endswith('_EndDefinition'):
                section = None
                continue
            if section == 'MultiwiniaOptions':
                lw = w.lower()
                if lw == 'gametypes':
                    res['gametypes'] = t[1] if len(t) > 1 else ''
                elif lw == 'numplayers':
                    res['numplayers'] = int(t[1])
                elif lw == 'populationcap':
                    res['popcap'] = int(t[1]); res['popcap_lines'].append(int(t[1]))
                elif lw == 'defenderpopulationcap':
                    res['defcap'] = int(t[1])
                elif lw == 'leveloptions':
                    res['options'] = t[1:]
                elif lw == 'effects':
                    res['effects'] = t[1:]
                elif lw == 'coopmode':
                    res['coop'] = True
                elif lw == 'attackingteams':
                    res['attacking'] = [int(x) for x in t[1:]]
                elif lw == 'officiallevel':
                    res['official'] = True
            elif section == 'Buildings':
                if len(t) >= 8:
                    try:
                        res['buildings'].append((w.lower(), int(t[1]), int(t[4]), [int(x) for x in t[8:]] if w.lower() in SPAWN else []))
                    except ValueError:
                        res['buildings'].append((w.lower(), -1, -1, []))
            elif section == 'InstantUnits':
                if len(t) >= 13:
                    # Type team x z count inUnit state spread wpx wpz routeId ? runAsTask
                    res['instant'].append((w, int(t[1]), int(t[4]), int(t[12])))
    return res


TAKES_ARG = {'trunkportreinforcements', 'trunkportarmour', 'attackerreinforcements',
             'defenderreinforcements', 'defenderreinforcementtype', 'attackerreinforcementtype',
             'airstrikecapturezones', 'futurewinianteam', 'attackreinforcementoption',
             'defendreinforcementoption'}


def level_options(opts):
    """Turn the LevelOptions token list into a dict (mirrors LevelFile::ParseMultiwiniaOptions)."""
    takes_arg = {'trunkportreinforcements', 'trunkportarmour', 'attackerreinforcements',
                 'defenderreinforcements', 'defenderreinforcementtype', 'attackerreinforcementtype',
                 'airstrikecapturezones', 'futurewinianteam', 'attackreinforcementoption',
                 'defendreinforcementoption'}
    d = {}
    i = 0
    while i < len(opts):
        o = opts[i].lower()
        if o in takes_arg and i + 1 < len(opts):
            d[o] = opts[i + 1]; i += 2
        else:
            d[o] = '1'; i += 1
    return d


def hops(buildings):
    nodes = {b[1]: b for b in buildings if b[0] in SPAWN}
    masters = [k for k, v in nodes.items() if v[0] == 'spawnpointmaster']
    if not masters:
        return None
    out = {}
    def dfs(n, d, stack):
        if n in stack or n not in nodes:
            return
        if nodes[n][0] == 'spawnpoint':
            out.setdefault(n, []).append(d)
        stack.add(n)
        for l in nodes[n][3]:
            dfs(l, d + 1, stack)
        stack.discard(n)
    dfs(masters[0], 0, set())
    allh = [h for v in out.values() for h in v]
    return (min(allh), max(allh)) if allh else None


def main():
    d = sys.argv[1]
    csv_out = sys.argv[2] if len(sys.argv) > 2 else None
    rows = []
    for fn in sorted(os.listdir(d)):
        p = os.path.join(d, fn)
        if not (fn.startswith('mp_') and fn.endswith('.txt') and os.path.isfile(p)):
            continue
        L = parse_level(p)
        opts = level_options(L['options'])
        sps = [b for b in L['buildings'] if b[0] == 'spawnpoint']
        tps = [b for b in L['buildings'] if b[0] == 'trunkport']
        locks = [b for b in L['buildings'] if b[0] == 'spawnpopulationlock']
        n = L['numplayers']
        cap = L['popcap']
        per = (cap // n) if (cap != -1 and n > 0) else None
        assault = ''
        if L['gametypes'] == 'Assault' and L['defcap'] != -1 and n > 0:
            na = len(L['attacking']) or 1
            nd = n - na
            att = (cap - nd * L['defcap']) // na
            assault = f"def {L['defcap']} / att {att}"
        darw = {}
        other = []
        for (typ, team, cnt, task) in L['instant']:
            if typ.lower() == 'darwinian':
                darw[team] = darw.get(team, 0) + cnt
            else:
                other.append(f"{typ}{'(task)' if task else ''}×{cnt}@{team}")
        sp_owned = {}
        for b in sps:
            if b[2] != 255 and b[2] != -1:
                sp_owned[b[2]] = sp_owned.get(b[2], 0) + 1
        seeded = {}
        for b in L['buildings']:
            if b[0] in ('spawnpoint', 'solarpanel') and b[2] not in (255, -1):
                seeded[b[2]] = seeded.get(b[2], 0) + 5
        tp_owned = {}
        for b in tps:
            tp_owned[b[2]] = tp_owned.get(b[2], 0) + 1
        h = hops(L['buildings'])
        rows.append(dict(
            level=fn[:-4], gametypes=L['gametypes'], official='Y' if L['official'] else '',
            numplayers=n, popcap=cap if len(L['popcap_lines']) <= 1 else f"{cap} (lines {' '.join(map(str, L['popcap_lines']))})",
            percap=per if per is not None else 'none', assault=assault,
            coop='Y' if L['coop'] else '', attacking=' '.join(map(str, L['attacking'])),
            options=' '.join(f"{k}={v}" if k in TAKES_ARG else k for k, v in opts.items()),
            effects=' '.join(L['effects']),
            spawnpoints=len(sps), sp_start=' '.join(f"t{k}:{v}" for k, v in sorted(sp_owned.items())),
            hops=f"{h[0]}–{h[1]}" if h else '',
            locks=len(locks),
            trunkports=len(tps), tp_owners=' '.join(f"t{k}:{v}" for k, v in sorted(tp_owned.items())),
            darwinians=' '.join(f"t{k}:{v}" for k, v in sorted(darw.items())),
            seeded=' '.join(f"t{k}:{v}" for k, v in sorted(seeded.items())),
            other_units='; '.join(other)))
    keys = list(rows[0].keys())
    if csv_out:
        with open(csv_out, 'w', newline='') as f:
            w = csv.DictWriter(f, fieldnames=keys); w.writeheader(); w.writerows(rows)
    # Table 1
    print('| Level | GameTypes | Off. | NumPlayers | PopulationCap | Per-team cap | Assault caps | Co-op | LevelOptions | SpawnPoints (owned at start) | Hops | Pop-locks | TrunkPorts (owners) | InstantUnits Darwinians per position | Port-seeded Darwinians per position | Other start units |')
    print('|' + '---|' * 16)
    for r in rows:
        print(f"| `{r['level']}` | {r['gametypes']} | {r['official']} | {r['numplayers']} | {r['popcap']} | {r['percap']} | {r['assault']} | {r['coop']} | {r['options']} | {r['spawnpoints']}{' (' + r['sp_start'] + ')' if r['sp_start'] else ''} | {r['hops']} | {r['locks']} | {r['trunkports']}{' (' + r['tp_owners'] + ')' if r['tp_owners'] else ''} | {r['darwinians']} | {r['seeded']} | {r['other_units']} |")
    # Table 2: trunk-port configuration
    print()
    print('| Level | GameTypes | AdvanceTrunkPorts calls / tick | Darwinians per owned port per wave | Armour cap | Turrets | Side tasks per wave |')
    print('|---|---|---|---|---|---|---|')
    for r in rows:
        L = parse_level(os.path.join(d, r['level'] + '.txt'))
        opts = level_options(L['options'])
        gt = r['gametypes']
        tpr = int(opts.get('trunkportreinforcements', -1))
        tpa = int(opts.get('trunkportarmour', -1))
        level_call = tpr > 0 or tpa > 0
        mode_call = gt in ('RocketRiot', 'Blitzkrieg')
        calls = (1 if level_call else 0) + (1 if mode_call else 0)
        if calls == 0 or r['trunkports'] == 0:
            continue
        if gt in MODE_DEFAULTS:
            md = MODE_DEFAULTS[gt]
            cnt = f"{md['count']} (option ReinforcementCount)"
            arm = f"{md['maxArmour']} (option MaxArmour)"
            tur = 'none (nogunturrets)' if 'nogunturrets' in opts else f"every {md['turretFreq']} waves, cap {md['maxTurrets']}" + (' (AirStrike when ≥ half the flags)' if gt == 'Blitzkrieg' else '')
        else:
            if gt == 'Assault':
                ac = opts.get('attackerreinforcements', str(tpr)); dc = opts.get('defenderreinforcements', str(tpr))
                cnt = f"attacker {ac} / defender {dc}"
            else:
                cnt = str(tpr)
            arm = str(tpa) if tpa > 0 else ('unlimited (0)' if tpa == 0 else 'none')
            tur = 'none'
        side = []
        if 'defenderreinforcementtype' in opts:
            side.append(f"defenders: {opts['defenderreinforcementtype']}" + (f" option {opts['defendreinforcementoption']}" if 'defendreinforcementoption' in opts else ''))
        if 'attackerreinforcementtype' in opts:
            side.append(f"{'attackers' if gt == 'Assault' else 'every team'}: {opts['attackerreinforcementtype']}" + (f" option {opts['attackreinforcementoption']}" if 'attackreinforcementoption' in opts else ''))
        print(f"| `{r['level']}` | {gt} | {calls}{' (double-run bug)' if calls == 2 else ''} | {cnt} | {arm} | {tur} | {'; '.join(side) if side else '—'} |")


if __name__ == '__main__':
    main()
