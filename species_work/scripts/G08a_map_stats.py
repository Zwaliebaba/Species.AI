"""G08a helper: anonymous per-map rows (mode, players, ...) transcribed from the package inputs.
No map names. Prints per-mode / per-player-count aggregates used in rules_and_conventions.md."""
import sys
from collections import Counter, defaultdict
# fields: mode, players, diff, playtime(min or None), cap(None=unset), defcap, coop('-','allowed','allowed+groups','forced+groups'),
# world, max_crates, rz, rz_block, good, bad, basic, start_total, spawn_nodes, tg_total, tg_neutral
R = [
# Spawn Control
('SC',2,'b',None,2000,None,'-',2700,5,0,0,26,7,7,80,8,0,0),
('SC',2,'b',None,1800,None,'-',2000,4,0,0,26,7,7,0,10,1,1),
('SC',2,'i',None,2000,None,'-',2000,4,0,0,26,7,7,90,8,0,0),
('SC',2,'i',10,2000,None,'-',2677,5,0,0,26,7,7,0,9,1,1),
('SC',2,'i',10,1500,None,'-',2754,5,0,0,26,7,7,80,10,0,0),
('SC',3,'b',10,1800,None,'-',2345,4,0,0,26,7,7,120,10,0,0),
('SC',3,'a',None,1800,None,'-',6000,12,3,0,26,7,7,120,15,0,0),
('SC',4,'i',15,2000,None,'allowed',3000,6,0,0,26,7,7,160,10,0,0),
('SC',4,'a',10,None,None,'allowed+groups',3000,6,0,0,26,7,7,160,12,0,0),
# Zone Control
('ZC',2,'b',None,2000,None,'-',2700,5,0,0,26,7,7,80,6,0,0),
('ZC',2,'i',10,1500,None,'-',2754,5,0,0,26,7,7,80,10,0,0),
('ZC',3,'b',5,1800,None,'-',2345,4,0,0,26,7,7,120,9,0,0),
('ZC',3,'i',10,1800,None,'-',3000,6,3,0,26,7,7,135,9,1,1),
('ZC',3,'i',10,1800,None,'-',2800,5,0,0,26,7,7,120,9,1,1),
('ZC',3,'i',10,1800,None,'-',2750,5,0,0,23,7,6,0,0,6,0),
('ZC',4,'b',None,2000,None,'allowed',3000,6,0,0,26,7,7,160,12,4,4),
('ZC',4,'i',10,2000,None,'allowed',3000,6,0,0,26,7,7,160,10,0,0),
('ZC',4,'a',10,None,None,'allowed+groups',3000,6,0,0,26,7,7,160,12,0,0),
# Relic Run
('RR',2,'b',None,2000,None,'-',2332,4,0,0,26,7,7,80,6,0,0),
('RR',2,'b',None,2000,None,'-',2000,4,0,0,26,7,7,80,4,0,0),
('RR',2,'i',None,2000,None,'-',2332,4,0,0,26,7,7,80,4,0,0),
('RR',2,'i',None,2000,None,'-',2000,4,0,0,26,7,7,80,4,0,0),
('RR',3,'i',None,1800,None,'-',3000,6,0,0,26,7,7,120,6,0,0),
('RR',3,'i',None,1500,None,'-',2999,5,0,0,26,7,7,120,9,0,0),
('RR',4,'a',None,2000,None,'-',3000,6,0,0,26,7,7,160,12,0,0),
('RR',4,'a',None,2000,None,'allowed',3428,6,0,0,26,7,7,160,8,0,0),
('RR',4,'a',None,2000,None,'-',2720,5,0,0,26,7,7,160,8,0,0),
# Siege
('SG',2,'b',15,2000,350,'-',3000,6,0,0,18,6,4,327,0,5,0),
('SG',2,'i',15,2000,None,'-',5143,10,1,1,17,6,3,308,0,3,0),
('SG',2,'a',15,2000,275,'-',2991,5,1,1,21,6,5,209,0,6,0),
('SG',2,'a',15,1600,250,'-',5000,10,2,2,21,6,5,139,0,7,0),
('SG',3,'i',15,1800,None,'forced+groups',2549,5,0,0,22,6,6,260,0,8,0),
('SG',3,'a',15,1500,250,'forced+groups',5000,10,2,2,21,6,5,182,0,9,0),
('SG',4,'a',15,1600,200,'forced+groups',4070,8,1,1,21,6,5,170,0,8,0),
('SG',4,'a',20,1500,200,'forced+groups',7000,14,2,2,21,6,5,158,0,10,0),
# Launch Race
('LR',2,'b',None,1500,None,'-',2000,4,0,0,22,7,5,0,0,2,0),
('LR',2,'i',None,1500,None,'-',5000,10,2,0,25,7,6,0,2,2,0),
('LR',2,'i',None,1800,None,'-',3756,7,0,0,22,7,5,0,0,2,0),
('LR',3,'b',None,1800,None,'-',5000,10,3,0,22,7,5,0,0,3,0),
('LR',3,'i',None,1800,None,'-',4285,8,4,0,22,7,5,0,0,3,0),
('LR',3,'a',None,1800,None,'-',4500,9,0,0,22,7,5,24,0,3,0),
('LR',4,'a',None,1800,None,'-',4186,8,4,0,25,7,6,0,12,4,0),
# Flag Chain
('FC',2,'b',None,1500,None,'-',3500,7,0,0,25,7,6,80,1,2,0),
('FC',2,'i',None,2000,None,'-',3000,6,0,0,25,7,6,0,4,3,1),
('FC',2,'a',None,2000,None,'-',4000,8,0,0,25,7,6,16,6,2,0),
('FC',2,'a',None,1800,None,'-',3000,6,0,0,26,7,7,72,3,3,1),
('FC',3,'b',None,2000,None,'-',2938,5,3,0,23,7,6,0,0,3,0),
('FC',3,'i',None,1500,None,'-',4000,8,0,0,23,7,6,45,0,3,0),
('FC',4,'a',None,1400,None,'-',4000,8,4,0,23,7,6,0,0,4,0),
]
print('maps', len(R))
by = defaultdict(list)
for r in R: by[(r[0], r[1])].append(r)
pc = Counter(r[1] for r in R); print('players', dict(pc))
for m in ['SC','ZC','RR','SG','LR','FC']:
    rows=[r for r in R if r[0]==m]
    print('==', m, len(rows), dict(Counter(r[1] for r in rows)))
    for p in (2,3,4):
        g=by[(m,p)]
        if not g: continue
        print(' ',p,'n',len(g),'diff',dict(Counter(r[2] for r in g)),'pt',[r[3] for r in g],
              'cap',[r[4] for r in g],'perteam',[ (r[4]//p if r[4] else None) for r in g],
              'world',sorted(r[7] for r in g),'maxcr',sorted(r[8] for r in g),
              'rz',[(r[9],r[10]) for r in g],'pools',sorted((r[11],r[12],r[13]) for r in g),
              'start',sorted(r[14] for r in g),'sn',sorted(r[15] for r in g),'tg',[(r[16],r[17]) for r in g])
    assert all(r[8]==r[7]//500 for r in rows), m
print('rz total', sum(r[9] for r in R), 'maps with rz', sum(1 for r in R if r[9]), 'blocking', sum(r[10] for r in R))
print('caps set', Counter(r[4] for r in R))
print('playtime', Counter(r[3] for r in R if r[3]))
print('coop', Counter(r[6] for r in R))
print('world sizes', Counter(r[7] for r in R).most_common(6), min(r[7] for r in R), max(r[7] for r in R))
print('maps with spawn nodes', sum(1 for r in R if r[15]), Counter(r[0] for r in R if r[15]))
print('tg maps', sum(1 for r in R if r[16]), 'tg total', sum(r[16] for r in R), 'neutral', sum(r[17] for r in R))
