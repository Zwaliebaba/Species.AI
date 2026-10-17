#!/usr/bin/env python3
"""X1a: lexical/arity check of data/scripts/*.txt (and data/purity/buyme.txt) against code/script.cpp
(g_opCodeNames + per-opcode GetNextToken usage). Usage: python3 -I X1a_script_check.py SRC_ROOT"""
import collections, glob, os, sys
# opcode -> minimum tokens after the opcode that are read without TokenAvailable (MISSING => CRASH), optional count
ARITY = {'camcut':(1,0),'cammove':(2,0),'camanim':(1,0),'camfov':(1,1),'cambuildingfocus':(3,0),
 'cambuildingapproach':(4,0),'camlocationfocus':(2,0),'camglobalworldfocus':(0,0),'camreset':(0,0),
 'enterlocation':(1,0),'exitlocation':(0,0),'say':(1,0),'shutup':(0,0),'wait':(0,1),'waitsay':(0,0),
 'waitcam':(0,0),'waitfade':(0,0),'waitrocket':(3,0),'waitplayernotbusy':(0,0),'highlight':(1,0),
 'clearhighlights':(0,0),'triggersound':(1,0),'stopsound':(1,0),'demogesture':(1,0),'giveresearch':(1,0),
 'setmission':(2,0),'gameover':(0,0),'resetresearch':(0,0),'restoreresearch':(0,0),'runcredits':(0,0),
 'setcutscenemode':(1,0),'goddishactivate':(0,0),'goddishdeactivate':(0,0),'goddishspawnspam':(0,0),
 'goddishspawnresearch':(0,0),'triggerspam':(0,0),'puritycontrol':(0,0),'showdarwinlogo':(0,0),
 'showdemoendsequence':(0,0),'permitescape':(0,0),'destroybuilding':(2,0),'activatetrunkport':(1,0),
 'activatetrunkportfull':(1,0)}
SEPS=set(' \t\r\n:,')
def toks(l):
    for ch in '#\n\r':
        i=l.find(ch)
        if i>=0: l=l[:i]
    out,cur=[],''
    for c in l:
        if c in SEPS:
            if cur: out.append(cur); cur=''
        else: cur+=c
    if cur: out.append(cur)
    return out
root=sys.argv[1]
files=sorted(glob.glob(os.path.join(root,'data/scripts/*.txt')))+[os.path.join(root,'data/purity/buyme.txt')]
use=collections.Counter(); problems=[]
for f in files:
    for n,l in enumerate(open(f,encoding='latin-1').read().split('\n'),1):
        t=toks(l)
        if not t: continue
        op=t[0].lower()
        if op not in ARITY: problems.append(f'{os.path.relpath(f,root)}:{n}: unknown opcode {t[0]!r}'); continue
        use[op]+=1
        req,opt=ARITY[op]
        if len(t)-1<req: problems.append(f'{os.path.relpath(f,root)}:{n}: {t[0]} has {len(t)-1} args < {req} (CRASH or NULL)')
        if len(t)-1>req+opt: problems.append(f'{os.path.relpath(f,root)}:{n}: {t[0]} has {len(t)-1} args > {req+opt} (extra ignored)')
print('files',len(files),'opcodes used',len(use)); print(dict(use.most_common()))
print('\n'.join(problems) if problems else 'no problems')
